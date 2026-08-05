/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv_guard.h>
#include <amdgv_irqmgr.h>
#include <amdgv_oss.h>
#include <amdgv_oss_wrapper.h>
#include <amdgv_sched.h>

#include "osssys/ih_v7_1.h"

#include "asic_reg/OSSSYS/osssys_7_1_0_offset.h"
#include "asic_reg/OSSSYS/osssys_7_1_0_sh_mask.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

static int ih_v7_1_ih_iv_ring_enable(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t ih_rb_cntl = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_CNTL));

	if (enable) {
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RB_ENABLE, 1);
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, ENABLE_INTR, 1);
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_CNTL), ih_rb_cntl);
		adapt->irqmgr.ih.enabled = true;
	} else {
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RB_ENABLE, 0);
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, ENABLE_INTR, 0);
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_CNTL), ih_rb_cntl);

		/* set rptr, wptr to 0 */
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_RPTR), 0);
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_WPTR), 0);
		adapt->irqmgr.ih.enabled = false;
		adapt->irqmgr.ih.rptr = 0;
	}

	return 0;
}

static uint32_t ih_v7_1_ih_get_wptr(struct amdgv_adapter *adapt)
{
	uint32_t wptr, tmp;

	wptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_WPTR));
	if (REG_GET_FIELD(wptr, IH_RB_WPTR, RB_OVERFLOW)) {
		wptr = REG_SET_FIELD(wptr, IH_RB_WPTR, RB_OVERFLOW, 0);

		AMDGV_WARN("IH ring buffer overflow (0x%08X, 0x%08X, 0x%08X)\n", wptr,
			   adapt->irqmgr.ih.rptr, wptr & adapt->irqmgr.ih.ptr_mask);

		adapt->irqmgr.ih.rptr = wptr & adapt->irqmgr.ih.ptr_mask;
		adapt->irqmgr.ih_funcs->set_rptr(adapt);

		tmp = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_CNTL));
		tmp = REG_SET_FIELD(tmp, IH_RB_CNTL, WPTR_OVERFLOW_CLEAR, 1);
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_CNTL), tmp);
	}

	return (wptr & adapt->irqmgr.ih.ptr_mask);
}

static void ih_v7_1_ih_set_rptr(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.ih.use_doorbell)
		WDOORBELL32(adapt->irqmgr.ih.doorbell_index, adapt->irqmgr.ih.rptr);
	else
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_RPTR), adapt->irqmgr.ih.rptr);
}

static uint32_t ih_v7_1_ih_get_rptr(struct amdgv_adapter *adapt)
{
	uint32_t rptr;

	rptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_RPTR));

	return (rptr & adapt->irqmgr.ih.ptr_mask);
}

static void ih_v7_1_mbox_irq_source_enable(struct amdgv_adapter *adapt, bool enable)
{
	amdgv_mailbox_irq_source_enable(adapt, enable);
}

static int ih_v7_1_ih_node_to_xcc_inst(struct amdgv_adapter *adapt, uint32_t ih_node)
{
	int logic_xcc;
	uint32_t xcc = (ih_node & 0x7) - 2 + (ih_node >> 3) * 4;

	for (logic_xcc = 0; logic_xcc < adapt->mcp.gfx.num_xcc; logic_xcc++) {
		if (xcc == GET_INST(GC, logic_xcc))
			return logic_xcc;
	}

	AMDGV_ERROR("Couldn't find xcc mapping from IH node");
	return AMDGV_FAILURE;
}

static void ih_v7_1_handle_page_fault(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	uint64_t addr;
	uint64_t tmp;
	uint32_t src_id, ring_id, vm_id;
	uint32_t hub_id, mid;
	int xcc_id;

	addr = (uint64_t)entry->src_data[0] << 12;
	addr |= ((uint64_t)entry->src_data[1] & 0x1f) << 44;

	src_id = entry->src_id;
	ring_id = entry->ring_id;
	vm_id = entry->vm_id;

	switch (entry->client_id) {
	case IH_IV_CLIENTID_UTCL2: /* cover GC/GFXHUB */
		xcc_id = ih_v7_1_ih_node_to_xcc_inst(adapt, entry->node_id);
		if (xcc_id < 0 || xcc_id >= adapt->mcp.gfx.num_xcc)
			xcc_id = 0;

		hub_id = AMDGV_GFXHUB(xcc_id);

		tmp = RREG32(adapt->vmhub[hub_id].vm_l2_pro_fault_status_hi);
		tmp <<= 32;
		tmp |= RREG32(adapt->vmhub[hub_id].vm_l2_pro_fault_status);

		AMDGV_ERROR("DMAR: trapped by GMC as page fault, client_id=%u, src_id=%u, "
			    "ring_id=%u, vm_id=%u address=0x%016llx, "
			    "VM_L2_PROTECTION_FAULT_STATUS=0x%016llx\n",
			    entry->client_id, src_id, ring_id, vm_id, addr, tmp);

		if (adapt->vmhub[hub_id].vmhub_funcs->clear_protection_fault)
			adapt->vmhub[hub_id].vmhub_funcs->clear_protection_fault(adapt, xcc_id);
		else if (adapt->vmhub[hub_id].fault_cntl) {
			tmp = RREG32(adapt->vmhub[hub_id].fault_cntl);
			tmp |= 1; /* to clear the page fault */
			WREG32(adapt->vmhub[hub_id].fault_cntl, tmp);
		}

		break;

	case IH_IV_CLIENTID_VMC: /* cover MMHUB0 */
		mid = entry->node_id / 4;
		if (mid >= adapt->mmhub.num_instances)
			mid = 0;

		hub_id = AMDGV_MMHUB0(mid);

		tmp = RREG32(adapt->vmhub[hub_id].vm_l2_pro_fault_status_hi);
		tmp <<= 32;
		tmp |= RREG32(adapt->vmhub[hub_id].vm_l2_pro_fault_status);

		AMDGV_ERROR("DMAR: trapped by GMC as page fault, client_id=%u, src_id=%u, "
			    "ring_id=%u, vm_id=%u address=0x%016llx, "
			    "VM_L2_PROTECTION_FAULT_STATUS=0x%016llx\n",
			    entry->client_id, src_id, ring_id, vm_id, addr, tmp);

		if (adapt->vmhub[hub_id].vmhub_funcs->clear_protection_fault)
			adapt->vmhub[hub_id].vmhub_funcs->clear_protection_fault(adapt, mid);
		else if (adapt->vmhub[hub_id].fault_cntl) {
			tmp = RREG32(adapt->vmhub[hub_id].fault_cntl);
			tmp |= 1; /* to clear the page fault */
			WREG32(adapt->vmhub[hub_id].fault_cntl, tmp);
		}

		break;
	default:
		AMDGV_ERROR("Unknown/Unhandled Interrupt Received: client_id=%u "
			    "address=0x%016llx\n",
			    entry->client_id, addr);
		break;
	}
}

static int ih_v7_1_hv_event_process(struct amdgv_adapter *adapt)
{
	uint32_t intr_bits;
	uint32_t sta_bits;
	uint32_t active_vf = 0;
#ifdef CONFIG_HVVM_MAILBOX
	uint32_t idx_vf;
	uint32_t valid_bits;
#endif

	if (adapt->status != AMDGV_STATUS_HW_INIT)
		return 0;

	oss_spin_lock(adapt->irqmgr.hv_event_lock);

	/* save intr setting and disable interrupt */
	amdgv_gpuiov_get_intr(adapt, &intr_bits);
	amdgv_gpuiov_set_intr(adapt, 0);

	amdgv_gpuiov_get_intr_status(adapt, &sta_bits);
	amdgv_gpuiov_clear_intr_status(adapt, sta_bits);

	if (sta_bits & AMDGV_GFX_HANG_NEED_FLR_INTR) {
		sta_bits &= ~AMDGV_GFX_HANG_NEED_FLR_INTR;
		AMDGV_ERROR("HW SCHED RESET IS NOT SUPPORTED\n");
		// amdgv_gpuiov_get_active_vf_idx(adapt, GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH0_RLCV,
		// 			       &active_vf);
		// amdgv_sched_queue_event(adapt, active_vf, AMDGV_EVENT_HW_SCHED_RESET_VF,
		// 			AMDGV_SCHED_BLOCK_GFX);

		amdgv_guard_add_active_event(adapt, active_vf, AMDGV_GUARD_EVENT_ALL_INT);
	}

	if (sta_bits & AMDGV_UVD_HANG_NEED_FLR_INTR) {
		sta_bits &= ~AMDGV_UVD_HANG_NEED_FLR_INTR;
		AMDGV_ERROR("HW SCHED RESET IS NOT SUPPORTED\n");
		// amdgv_gpuiov_get_active_vf_idx(adapt, GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH0_MMSCH,
		// 			       &active_vf);
		// amdgv_sched_queue_event(adapt, active_vf, AMDGV_EVENT_HW_SCHED_RESET_VF,
		// 			AMDGV_SCHED_BLOCK_VCN);
		amdgv_guard_add_active_event(adapt, active_vf, AMDGV_GUARD_EVENT_ALL_INT);
	}

#ifdef CONFIG_HVVM_MAILBOX
	if (sta_bits & AMDGV_HVVM_MAILBOX_TRN_ACK_INTR) {
		sta_bits &= ~AMDGV_HVVM_MAILBOX_TRN_ACK_INTR,
			amdgv_gpuiov_set_mbox_valid(adapt, 0);
	}

	if (sta_bits & AMDGV_HVVM_MAILBOX_RCV_VALID_INTR) {
		sta_bits &= ~AMDGV_HVVM_MAILBOX_RCV_VALID_INTR;

		/* get msg valid bitmap for valid bit of 1PF + 16VF */
		amdgv_gpuiov_get_mbox_msg_valid(adapt, &valid_bits);
		for (idx_vf = 0; (idx_vf < 17) && (valid_bits != 0); idx_vf++) {
			if (valid_bits & 1) {
				amdgv_hvvm_mailbox_receive_msg(adapt, idx_vf & event, true);
				amdgv_sched_queue_event(adapt, idx_vf, event);

				amdgv_guard_add_active_event(adapt, idx_vf,
							     AMDGV_GUARD_EVENT_ALL_INT);
			}

			valid_bits >> 1;
		}
	}
#endif

	/* restore interrupt */
	amdgv_gpuiov_set_intr(adapt, intr_bits);

	oss_spin_unlock(adapt->irqmgr.hv_event_lock);

	if (sta_bits != 0)
		AMDGV_INFO("some interrupts(0x%x) are not handled\n", intr_bits);

	return 0;
}

static void ih_v7_1_ih_iv_ring_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t ih_cntl;
	uint32_t ih_rb_cntl;
	uint32_t rptr;
	uint64_t rb_base_addr, wptr_off;

	/* Program IH_RB_BASE */
	if (adapt->irqmgr.ih.use_bus_addr)
		rb_base_addr = adapt->irqmgr.ih.rb_dma_addr;
	else
		rb_base_addr = adapt->irqmgr.ih.gpu_addr;
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_BASE), rb_base_addr >> 8);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_BASE_HI), rb_base_addr >> 40);
	AMDGV_DEBUG("rb_base_addr=0x%llx\n", rb_base_addr);

	/* Use the physical address of the iv ring base as the IH
	 * dummy read address */
	amdgv_nbio_enable_ih_interrupt(adapt, adapt->irqmgr.ih.use_bus_addr, rb_base_addr >> 8);

	/* Program IH_RB_CNTL */
	ih_rb_cntl = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_CNTL));
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RB_ENABLE, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, ENABLE_INTR, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RB_SIZE,
				   adapt->irqmgr.ih.ring_size_log2);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, WPTR_OVERFLOW_CLEAR, 1);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, WPTR_OVERFLOW_ENABLE, 1);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, WPTR_WRITEBACK_ENABLE, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RPTR_REARM, 1);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SWAP, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_VMID, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SPACE,
				   adapt->irqmgr.ih.use_bus_addr ? 2 : 4);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SNOOP,
				   adapt->irqmgr.ih.use_bus_addr ? 1 : 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_RO,
				   adapt->irqmgr.ih.use_bus_addr ? 0 : 1);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_CNTL), ih_rb_cntl);

	/* Program IH_CNTL */
	ih_cntl = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_CNTL));
	ih_cntl = REG_SET_FIELD(ih_cntl, IH_CNTL, MC_WR_CLEAN_CNT, 0x10);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_CNTL), ih_cntl);

	/* set the writeback address */
	if (adapt->irqmgr.ih.use_bus_addr)
		wptr_off = adapt->irqmgr.ih.rb_dma_addr + (adapt->irqmgr.ih.wptr_offs * 4);
	else
		wptr_off = adapt->irqmgr.ih.gpu_addr + (adapt->irqmgr.ih.wptr_offs * 4);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_WPTR_ADDR_LO), lower_32_bits(wptr_off));
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_WPTR_ADDR_HI), upper_32_bits(wptr_off));
	AMDGV_DEBUG("writeback_addr=0x%llx\n", wptr_off);

	/* set rptr, wptr to 0 */
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_RPTR), 0);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_WPTR), 0);

	/* set rptr with doorbell reg */
	rptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_DOORBELL_RPTR));
	/* use doorbell */
	if (adapt->irqmgr.ih.use_doorbell) {
		/* doorbell index */
		rptr = REG_SET_FIELD(rptr, IH_DOORBELL_RPTR, OFFSET,
				     adapt->irqmgr.ih.doorbell_index);
		rptr = REG_SET_FIELD(rptr, IH_DOORBELL_RPTR, ENABLE, 1);
	} else {
		rptr = REG_SET_FIELD(rptr, IH_DOORBELL_RPTR, ENABLE, 0);
	}
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_DOORBELL_RPTR), rptr);

	amdgv_nbio_ih_doorbell_range(adapt, adapt->irqmgr.ih.use_doorbell,
				     adapt->irqmgr.ih.doorbell_index);

	amdgv_nbio_enable_doorbell_aperture(adapt, true);

	if (adapt->irqmgr.ih.use_doorbell)
		WDOORBELL32(adapt->irqmgr.ih.doorbell_index, 0);
}

static const struct amdgv_ih_funcs ih_v7_1_ih_funcs = {
	.get_wptr = ih_v7_1_ih_get_wptr,
	.decode_iv = amdgv_irqmgr_decode_iv,
	.set_rptr = ih_v7_1_ih_set_rptr,
	.get_rptr = ih_v7_1_ih_get_rptr,
	.process = amdgv_ih_iv_ring_entry_process,
	.enable_iv_ring = ih_v7_1_ih_iv_ring_enable,
	.enable_mbox = ih_v7_1_mbox_irq_source_enable,
	.handle_page_fault = ih_v7_1_handle_page_fault,
};

static int ih_v7_1_sw_init(struct amdgv_adapter *adapt)
{
	if (amdgv_irqmgr_sw_init(adapt))
		return AMDGV_FAILURE;

	adapt->irqmgr.ih_funcs = &ih_v7_1_ih_funcs;
	adapt->irqmgr.hv_event_process = ih_v7_1_hv_event_process;

	adapt->irqmgr.ih.use_doorbell = true;
	adapt->irqmgr.ih.doorbell_index = AMDGV_IH_V7_DOORBELL_IH << 1;

	if (!adapt->opt.skip_hw_init) {
		/* register interrupt handler */
		if (amdgv_irqmgr_register_interrupt(adapt)) {
			AMDGV_ERROR("failed to register interrupt!\n");
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

static int ih_v7_1_sw_fini(struct amdgv_adapter *adapt)
{
	if (!adapt->fini_opt.skip_hw_fini) {
		amdgv_irqmgr_unregister_interrupt(adapt);
	}

	return amdgv_irqmgr_sw_fini(adapt);
}

static void ih_v7_1_golden_init(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* IH storm client — aligned with upstream: read-modify-write */
	tmp = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_STORM_CLIENT_LIST_CNTL));
	tmp = REG_SET_FIELD(tmp, IH_STORM_CLIENT_LIST_CNTL,
			    CLIENT18_IS_STORM_CLIENT, 1);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_STORM_CLIENT_LIST_CNTL), tmp);

	/* IH flood control — aligned with upstream: read-modify-write */
	tmp = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_INT_FLOOD_CNTL));
	tmp = REG_SET_FIELD(tmp, IH_INT_FLOOD_CNTL, FLOOD_CNTL_ENABLE, 1);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_INT_FLOOD_CNTL), tmp);

	/* IH retry CAM — aligned with upstream: read-modify-write, CAM_SIZE=0xF */
	tmp = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RETRY_INT_CAM_CNTL));
	tmp = REG_SET_FIELD(tmp, IH_RETRY_INT_CAM_CNTL, ENABLE, 1);
	tmp = REG_SET_FIELD(tmp, IH_RETRY_INT_CAM_CNTL, CAM_SIZE, 0xF);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RETRY_INT_CAM_CNTL), tmp);
}

static int ih_v7_1_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t intr_bits;

	ih_v7_1_golden_init(adapt);

	/* config GPUIOV interrupt */
	amdgv_gpuiov_get_intr(adapt, &intr_bits);
	intr_bits &= ~AMDGV_UVD_HANG_NEED_FLR_INTR;
	amdgv_gpuiov_set_intr(adapt, intr_bits);

	if (amdgv_ih_ring_set(adapt))
		return AMDGV_FAILURE;

	/* initialize IH hw block */
	ih_v7_1_ih_iv_ring_hw_init(adapt);

	/* enable IH interrupts */
	ih_v7_1_ih_iv_ring_enable(adapt, true);

	/* Enable MP0 ASP (PSP) interrupt now that IH is ready to receive it */
	if (adapt->psp.enable_interrupt)
		adapt->psp.enable_interrupt(adapt, true);

	return 0;
}

static int ih_v7_1_hw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.disable_parse_ih)
		return 0;

	/* Disable MP0 ASP (PSP) interrupt now that IH is no longer ready to receive it */
	if (adapt->psp.enable_interrupt)
		adapt->psp.enable_interrupt(adapt, false);

	/* disable IH interrupts */
	ih_v7_1_ih_iv_ring_enable(adapt, false);

	return 0;
}

struct amdgv_init_func ih_v7_1_func = {
	.name = "ih_v7_1_func",
	.sw_init = ih_v7_1_sw_init,
	.sw_fini = ih_v7_1_sw_fini,
	.hw_init = ih_v7_1_hw_init,
	.hw_fini = ih_v7_1_hw_fini,
};
