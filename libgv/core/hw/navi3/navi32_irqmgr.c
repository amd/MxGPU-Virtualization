/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv_oss.h>
#include <amdgv_oss_wrapper.h>
#include <amdgv_irqmgr.h>
#include <amdgv_sched.h>
#include <amdgv_sched_internal.h>
#include <amdgv_guard.h>

#include "navi32_reg_inc.h"
#include "navi32_nbio_mapper.h"
#include "navi32_irqmgr.h"
#include "navi32_gpuiov.h"
#define CONFIG_HVVM_MAILBOX
static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

#define NAVI32_MSIX_TABLE_ENTRY_SIZE_DWORD 4
#define NAVI32_MSIX_TABLE_ENTRY_COUNT 4

static const char *gfxhub_client_ids[] = {
	"CB/DB",
	"Reserved",
	"GE1",
	"GE2",
	"CPF",
	"CPC",
	"CPG",
	"RLC",
	"TCP",
	"SQC (inst)",
	"SQC (data)",
	"SQG",
	"Reserved",
	"SDMA0",
	"SDMA1",
	"GCR",
	"SDMA2",
	"SDMA3",
};

static const char *mmhub_client_ids[][2] = {
	[0][0] = "VMC",
	[4][0] = "DCEDMC",
	[5][0] = "DCEVGA",
	[6][0] = "MP0",
	[7][0] = "MP1",
	[8][0] = "MPIO",
	[16][0] = "HDP",
	[17][0] = "LSDMA",
	[18][0] = "JPEG",
	[19][0] = "VCNU0",
	[21][0] = "VSCH",
	[22][0] = "VCNU1",
	[23][0] = "VCN1",
	[32+20][0] = "VCN0",
	[2][1] = "DBGUNBIO",
	[3][1] = "DCEDWB",
	[4][1] = "DCEDMC",
	[5][1] = "DCEVGA",
	[6][1] = "MP0",
	[7][1] = "MP1",
	[8][1] = "MPIO",
	[10][1] = "DBGU0",
	[11][1] = "DBGU1",
	[12][1] = "DBGU2",
	[13][1] = "DBGU3",
	[14][1] = "XDP",
	[15][1] = "OSSSYS",
	[16][1] = "HDP",
	[17][1] = "LSDMA",
	[18][1] = "JPEG",
	[19][1] = "VCNU0",
	[20][1] = "VCN0",
	[21][1] = "VSCH",
	[22][1] = "VCNU1",
	[23][1] = "VCN1",
};

static const char *navi32_client_id_to_name(uint32_t client, uint32_t cid, uint32_t rw)
{
	const char *client_name = NULL;
	switch (client) {
	case IH_IV_CLIENTID_GFX:
		if (cid >= ARRAY_SIZE(gfxhub_client_ids))
			client_name = "Unknown";
		else
			client_name = gfxhub_client_ids[cid];
		break;
	case IH_IV_CLIENTID_VMC:
		if (cid >= ARRAY_SIZE(mmhub_client_ids) || rw > 1)
			client_name =  "Unknown";
		else
			client_name =  mmhub_client_ids[cid][rw];
		break;
	default:
		client_name =  "Unknown";
		break;
	}

	return client_name;
}

static int navi32_ih_iv_ring_enable(struct amdgv_adapter *adapt, bool enable)
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

		amdgv_irqmgr_disable(adapt);

		/* set rptr, wptr to 0 */
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_RPTR), 0);
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_WPTR), 0);
	}

	return 0;
}

static uint32_t navi32_ih_get_wptr(struct amdgv_adapter *adapt)
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

static void navi32_ih_set_rptr(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.ih.use_doorbell)
		WDOORBELL32(adapt->irqmgr.ih.doorbell_index, adapt->irqmgr.ih.rptr);
	else
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_RPTR), adapt->irqmgr.ih.rptr);
}

static uint32_t navi32_ih_get_rptr(struct amdgv_adapter *adapt)
{
	uint32_t rptr;

	rptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_RPTR));

	return (rptr & adapt->irqmgr.ih.ptr_mask);
}

static void navi32_ih_decode_iv(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	/* wptr/rptr are in bytes! */
	uint32_t ring_index = adapt->irqmgr.ih.rptr >> 2;
	uint32_t dw[8];

	dw[0] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 0]);
	dw[1] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 1]);
	dw[2] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 2]);
	dw[3] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 3]);
	dw[4] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 4]);
	dw[5] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 5]);
	dw[6] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 6]);
	dw[7] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 7]);

	entry->client_id = dw[0] & 0xff;
	entry->src_id = (dw[0] >> 8) & 0xff;
	entry->ring_id = (dw[0] >> 16) & 0xff;
	entry->vm_id = (dw[0] >> 24) & 0xf;
	entry->vm_id_src = (dw[0] >> 31);
	entry->timestamp = dw[1] | ((uint64_t)(dw[2] & 0xffff) << 32);
	entry->timestamp_src = dw[2] >> 31;
	entry->pas_id = dw[3] & 0xffff;
	entry->pasid_src = dw[3] >> 31;
	entry->src_data[0] = dw[4];
	entry->src_data[1] = dw[5];
	entry->src_data[2] = dw[6];
	entry->src_data[3] = dw[7];

	/* wptr/rptr are in bytes! */
	adapt->irqmgr.ih.rptr += 32;
}

static void navi32_set_gpu_timer_ref_frequency(struct amdgv_adapter *adapt)
{
	uint32_t timer_pulse_en;
	uint32_t timer_pulse_width;
	uint32_t reference_clock = 100; // 100MHz
	uint32_t pwr_disp_timer_global_control =
		RREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_GLOBAL_CONTROL));

	timer_pulse_en = REG_GET_FIELD(pwr_disp_timer_global_control,
			PWR_DISP_TIMER_GLOBAL_CONTROL, DISP_TIMER_PULSE_EN);
	timer_pulse_width = REG_GET_FIELD(pwr_disp_timer_global_control,
			PWR_DISP_TIMER_GLOBAL_CONTROL, DISP_TIMER_PULSE_WIDTH);
	if (!timer_pulse_en || timer_pulse_width != reference_clock) {
		pwr_disp_timer_global_control = REG_SET_FIELD(pwr_disp_timer_global_control,
					PWR_DISP_TIMER_GLOBAL_CONTROL, DISP_TIMER_PULSE_EN, 1);

		pwr_disp_timer_global_control = REG_SET_FIELD(pwr_disp_timer_global_control,
					PWR_DISP_TIMER_GLOBAL_CONTROL, DISP_TIMER_PULSE_WIDTH,
					reference_clock);
		WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_GLOBAL_CONTROL),
					pwr_disp_timer_global_control);
	}
}

static void navi32_irqmgr_ack_timer_interrupt(struct amdgv_adapter *adapt)
{
	uint32_t pwr_display_timer_control =
		 REG_SET_FIELD(0, PWR_DISP_TIMER_CONTROL, DISP_TIMER_INT_STAT_AK, 1);
	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_CONTROL), pwr_display_timer_control);
}

static void navi32_irqmgr_next_timer_interrupt(struct amdgv_adapter *adapt, uint64_t micro_seconds)
{
	uint32_t pwr_ih_control;
	uint32_t pwr_display_timer_control = 0;
	uint32_t timer_ticks = 1;

	navi32_set_gpu_timer_ref_frequency(adapt);

	pwr_ih_control = RREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL));
	pwr_ih_control = REG_SET_FIELD(pwr_ih_control, PWR_IH_CONTROL,
			DISP_TIMER_TRIGGER_MASK, micro_seconds ? 0 : 1);
	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL), pwr_ih_control);

	pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
									DISP_TIMER_INT_TYPE, 1); // level-based
	pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
									DISP_TIMER_INT_MASK, 1);
	if (micro_seconds) {
		if (micro_seconds > NAVI3_TIMER_MICROSEC_DELAY) {
			timer_ticks = micro_seconds - NAVI3_TIMER_MICROSEC_DELAY;
		}
		pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
									DISP_TIMER_INT_COUNT,  timer_ticks);
		WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_CONTROL), pwr_display_timer_control);
		oss_udelay(NAVI3_TIMER_MICROSEC_DELAY);

		pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
										DISP_TIMER_INT_MASK, 0);
		pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
										DISP_TIMER_INT_ENABLE, 1);
		WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_CONTROL), pwr_display_timer_control);
	} else {
		WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_CONTROL), pwr_display_timer_control);
	}
}

static int navi32_ih_process(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	if (NAVI3_IH_IV_CLIENTID_GRBM_CP == entry->client_id) {
		if (NAVI3_IH_IV_SRCID_CP_EOP_INTERRUPT == entry->src_id &&
				 adapt->irqmgr.ih_submission_interrupt_handler) {
			adapt->irqmgr.ih_submission_interrupt_handler(adapt->irqmgr.ih_submission_interrupt_context);
			return 0;
		}
	} else if (NAVI3_IH_IV_CLIENTID_GFX == entry->client_id) {
		if (NAVI3_IH_IV_SRCID_SDMA_CTXEMPTY == entry->src_id) {
			return 0; // ignore it
		} else if (NAVI3_IH_IV_SRCID_SDMA_TRAP == entry->src_id &&
				 adapt->irqmgr.ih_submission_interrupt_handler) {
			adapt->irqmgr.ih_submission_interrupt_handler(adapt->irqmgr.ih_submission_interrupt_context);
			return 0;
		}
	} else if (NAVI3_IH_IV_CLIENTID_PWR == entry->client_id) {
		if (NAVI3_IH_IV_SRCID_DISP_TIMER == entry->src_id) {
			navi32_irqmgr_ack_timer_interrupt(adapt);
			if (adapt->irqmgr.ih_gpu_timer_handler) {
				adapt->irqmgr.ih_gpu_timer_handler(adapt->irqmgr.ih_gpu_timer_context);
			}
			return 0;
		}
	}
	return amdgv_ih_iv_ring_entry_process(adapt, entry);
}

static void navi32_mbox_irq_source_enable(struct amdgv_adapter *adapt, bool enable)
{
	int val;
	uint32_t tmp, idx_vf;
	uint32_t offset;

	val = enable ? 1 : 0;

	/* enable/disable mailbox ack and valid interrupts for each VF */
	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		offset = SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, BIF_BX, MAILBOX_INT_CNTL);
		tmp = RREG32(offset);
		tmp = REG_SET_FIELD(tmp, BIF_BX_PF_MAILBOX_INT_CNTL, VALID_INT_EN, val);
		tmp = REG_SET_FIELD(tmp, BIF_BX_PF_MAILBOX_INT_CNTL, ACK_INT_EN, val);
		WREG32(offset, tmp);
	}
	if (adapt->flags & AMDGV_FLAG_USE_PF) {
		/* enable/disable mailbox ack and valid interrupts for PF */
		offset = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_INT_CNTL);
		tmp = RREG32(offset);
		tmp = REG_SET_FIELD(tmp, BIF_BX_PF_MAILBOX_INT_CNTL, VALID_INT_EN, val);
		tmp = REG_SET_FIELD(tmp, BIF_BX_PF_MAILBOX_INT_CNTL, ACK_INT_EN, val);
		WREG32(offset, tmp);
	}

	/* enable/disable BIF_VMHV_MAILBOX for each VF
	 * set TRN_ACK_INTR_EN and RCV_VALID_INTR_EN to 1/0
	 * This requires write all bits to 0 (except these two bits)
	 */
	tmp = 0;
	tmp = REG_SET_FIELD(tmp, BIF_BX_PF_BIF_VMHV_MAILBOX, VMHV_MAILBOX_RCV_VALID_INTR_EN, val);
	tmp = REG_SET_FIELD(tmp, BIF_BX_PF_BIF_VMHV_MAILBOX, VMHV_MAILBOX_TRN_ACK_INTR_EN, val);
	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		WREG32(SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, BIF_BX, BIF_VMHV_MAILBOX),
		       tmp);
	}
	if (adapt->flags & AMDGV_FLAG_USE_PF) {
		/* enable/disable BIF_VMHV_MAILBOX for PF */
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_BIF_VMHV_MAILBOX), tmp);
	}
}

static void navi32_handle_page_fault(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	uint64_t addr;
	uint32_t tmp, src_id, ring_id, vm_id, client_id, rw;

	addr = (uint64_t)entry->src_data[0] << 12;
	addr |= ((uint64_t)entry->src_data[1] & 0x1f) << 44;

	src_id = entry->src_id;
	ring_id = entry->ring_id;
	vm_id = entry->vm_id;

	switch (entry->client_id) {
	case IH_IV_CLIENTID_GFX: /* cover GC/GFXHUB */
		tmp = RREG32(adapt->vmhub[VM_GFXHUB].fault_status);
		client_id = REG_GET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_STATUS, CID);
		addr = ((uint64_t)RREG32(adapt->vmhub[VM_GFXHUB].fault_addr_hi) & 0xF) << 32;
		addr |= (uint64_t)RREG32(adapt->vmhub[VM_GFXHUB].fault_addr_lo);

		AMDGV_ERROR("Page fault: client_id=%s (0x%x), src_id=0x%x, ring_id=0x%x, vm_id=0x%x, address=0x%016llx, "
			   "VM_L2_PROTECTION_FAULT_STATUS=0x%08x\n",
			   navi32_client_id_to_name(entry->client_id, client_id, 0), client_id, src_id, ring_id, vm_id, addr, tmp);
		tmp = RREG32(adapt->vmhub[VM_GFXHUB].fault_cntl);
		tmp |= 1; /* to clear the page fault */
		WREG32(adapt->vmhub[VM_GFXHUB].fault_cntl, tmp);
		break;

	case IH_IV_CLIENTID_VMC: /* cover MMHUB0 */
		tmp = RREG32(adapt->vmhub[VM_MMHUB0].fault_status);
		client_id = REG_GET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_STATUS, CID);
		rw = REG_GET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_STATUS, RW);
		addr = ((uint64_t)RREG32(adapt->vmhub[VM_MMHUB0].fault_addr_hi) & 0xF) << 32;
		addr |= (uint64_t)RREG32(adapt->vmhub[VM_MMHUB0].fault_addr_lo);

		AMDGV_ERROR("Page fault: client_id=%s (0x%x), src_id=0x%x, ring_id=0x%x, vm_id=0x%x, address=0x%016llx, "
			   "VM_L2_PROTECTION_FAULT_STATUS=0x%08x\n",
			   navi32_client_id_to_name(entry->client_id, client_id, rw), client_id, src_id, ring_id, vm_id, addr, tmp);
		tmp = RREG32(adapt->vmhub[VM_MMHUB0].fault_cntl);
		tmp |= 1; /* to clear the page fault */
		WREG32(adapt->vmhub[VM_MMHUB0].fault_cntl, tmp);
		break;
	default:
		AMDGV_ERROR("Unknown/Unhandled DF Interrupt Received: client_id=0x%x, address=0x%016llx\n",
			   entry->client_id, addr);
		break;
	}
}

static int navi32_toggle_disp_timer2(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t interrupt_status;

	interrupt_status = RREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL));
	interrupt_status = REG_SET_FIELD(interrupt_status,
		PWR_IH_CONTROL,
		DISP_TIMER2_TRIGGER_MASK,
		enable ? 0 : 1);

	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL), interrupt_status);

	return 0;
}

static void navi32_unregister_interrupt(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.intr_regrt_info == NULL)
		return;

	/* remove interrupt handler from OS */
	oss_unregister_interrupt(adapt->dev, adapt->irqmgr.intr_regrt_info);

	/* free interrupt entries */
	oss_free(adapt->irqmgr.intr_regrt_info->intr_entries);

	/* free interrupt registration info */
	oss_free(adapt->irqmgr.intr_regrt_info);

	adapt->irqmgr.intr_regrt_info = NULL;
}

static int navi32_hv_event_process(struct amdgv_adapter *adapt)
{
	uint32_t intr_bits;
	uint32_t sta_bits;
	uint32_t active_vf;
	uint8_t event;
	enum amdgv_sched_event_id sched_event;
#ifdef CONFIG_HVVM_MAILBOX
	uint32_t idx_vf;
	uint32_t valid_bits;
#endif
	uint32_t vcn_poison_bits;
	union amdgv_sched_event_data event_data;

	oss_memset(&event_data, 0, sizeof(union amdgv_sched_event_data));
	vcn_poison_bits = AMDGV_UVD_HANG_SELF_RECOVERED_INTR |
		 AMDGV_UVD1_HANG_SELF_RECOVERED_INTR |
		 AMDGV_UVD1_CMD_COMPLETE_INTR;

	if (adapt->status != AMDGV_STATUS_HW_INIT)
		return 0;

	/* todo: check spinlock oss APIs definition */
	oss_spin_lock(adapt->irqmgr.hv_event_lock);

	/* save intr setting and disable interrupt */
	amdgv_gpuiov_get_intr(adapt, &intr_bits);
	amdgv_gpuiov_set_intr(adapt, 0);

	amdgv_gpuiov_get_intr_status(adapt, &sta_bits);
	amdgv_gpuiov_clear_intr_status(adapt, sta_bits);

	if (sta_bits & AMDGV_GFX_HANG_NEED_FLR_INTR) {
		sta_bits &= ~AMDGV_GFX_HANG_NEED_FLR_INTR;
		amdgv_gpuiov_get_active_vf_idx(adapt, NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV, &active_vf);
		amdgv_sched_queue_event(adapt, active_vf, AMDGV_EVENT_HW_SCHED_RESET_VF,
					AMDGV_SCHED_BLOCK_GFX);

		amdgv_guard_add_active_event(adapt, active_vf, AMDGV_GUARD_EVENT_ALL_INT);
	}

	/*
	 * These interrupts are generated by engine read on the poisoned address.
	 * Following event procssing is based on actual poison detected and don't need to be guarded.
	 */
	if (sta_bits & vcn_poison_bits) {
		sta_bits &= ~vcn_poison_bits;

		event_data.fed_data.src = AMDGV_FED_SRC_VCN;
		amdgv_sched_queue_event_ex(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_RAS_FED,
					AMDGV_SCHED_BLOCK_ALL, event_data);
	}

	if (sta_bits & AMDGV_UVD_HANG_NEED_FLR_INTR) {
		sta_bits &= ~AMDGV_UVD_HANG_NEED_FLR_INTR;
		amdgv_gpuiov_get_active_vf_idx(adapt, NAVI32_HW_SCHED_BLOCK_VCN_SCH0_MMSCH, &active_vf);
		amdgv_sched_queue_event(adapt, active_vf, AMDGV_EVENT_HW_SCHED_RESET_VF,
					AMDGV_SCHED_BLOCK_VCN);

		amdgv_guard_add_active_event(adapt, active_vf, AMDGV_GUARD_EVENT_ALL_INT);
	}

	if (sta_bits & AMDGV_UVD1_HANG_NEED_FLR_INTR) {
		sta_bits &= ~AMDGV_UVD1_HANG_NEED_FLR_INTR;
		amdgv_gpuiov_get_active_vf_idx(adapt, NAVI32_HW_SCHED_BLOCK_VCN1_SCH1_MMSCH, &active_vf);
		amdgv_sched_queue_event(adapt, active_vf, AMDGV_EVENT_HW_SCHED_RESET_VF,
					AMDGV_SCHED_BLOCK_VCN1);

		amdgv_guard_add_active_event(adapt, active_vf, AMDGV_GUARD_EVENT_ALL_INT);
	}

	if (sta_bits & NAVI32_JPEG_HANG_NEED_FLR_INTR) {
		sta_bits &= ~NAVI32_JPEG_HANG_NEED_FLR_INTR;
		amdgv_gpuiov_get_active_vf_idx(adapt, NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH, &active_vf);
		amdgv_sched_queue_event(adapt, active_vf, AMDGV_EVENT_HW_SCHED_RESET_VF,
					AMDGV_SCHED_BLOCK_JPEG);

		amdgv_guard_add_active_event(adapt, active_vf, AMDGV_GUARD_EVENT_ALL_INT);
	}

	/* active_vf and sched_block are not actually needed, this interrupt does not need to be guarded */
	if (sta_bits & AMDGV_MMSCH_GENERAL_EVENT_NOTIFICATION_0_INTR) {
		sta_bits &= ~AMDGV_MMSCH_GENERAL_EVENT_NOTIFICATION_0_INTR;
		amdgv_sched_queue_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION,
					AMDGV_SCHED_BLOCK_VCN);
	}

	if (sta_bits & AMDGV_MMSCH_GENERAL_EVENT_NOTIFICATION_1_INTR) {
		sta_bits &= ~AMDGV_MMSCH_GENERAL_EVENT_NOTIFICATION_1_INTR;
		amdgv_sched_queue_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION,
					AMDGV_SCHED_BLOCK_VCN1);
	}

#ifdef CONFIG_HVVM_MAILBOX
	if (sta_bits & AMDGV_HVVM_MAILBOX_TRN_ACK_INTR) {
		sta_bits &= ~AMDGV_HVVM_MAILBOX_TRN_ACK_INTR;
		amdgv_gpuiov_set_hvvm_mbox_valid(adapt, 0);
	}

	if (sta_bits & AMDGV_HVVM_MAILBOX_RCV_VALID_INTR) {
		sta_bits &= ~AMDGV_HVVM_MAILBOX_RCV_VALID_INTR;

		/* get msg valid bitmap for valid bit of 1PF + 16VF */
		amdgv_gpuiov_get_hvvm_mbox_msg_valid(adapt, &valid_bits);
		idx_vf = 0;
		while (valid_bits) {
			if (valid_bits & 1) {
				amdgv_mailbox_hvvm_receive_msg(adapt, idx_vf, &event, false);
				sched_event = amdgv_hvvm_mb_get_valid_event(adapt, event);

				if (sched_event == AMDGV_EVENT_CUR_VF_CTX_EMPTY) {
					amdgv_sched_queue_event(adapt, idx_vf, AMDGV_EVENT_CUR_VF_CTX_EMPTY,
						AMDGV_SCHED_BLOCK_GFX);
					break;
				} else {
					AMDGV_WARN("Unknown hv intr received, event id:%d\n", event);
				}
			}

			valid_bits = valid_bits >> 1;
			idx_vf++;
		}
		amdgv_gpuiov_set_hvvm_mbox_valid(adapt, 0);
	}
#endif

	/* restore interrupt */
	amdgv_gpuiov_set_intr(adapt, intr_bits);

	oss_spin_unlock(adapt->irqmgr.hv_event_lock);

	if (sta_bits != 0)
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_IOV_UNHANDLED_HV_INTR,
			      AMDGV_LOG_DATA_32_32(sta_bits, intr_bits));

	return OSS_IRQ_HANDLED;
}

static void navi32_ih_iv_ring_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t interrupt_cntl, ih_cntl;
	uint32_t ih_rb_cntl;
	uint32_t rptr, doorbell_aper;
	uint64_t rb_base_addr, wptr_off;
	uint32_t idx_vf;

	/* Program IH_RB_BASE */
	if (adapt->irqmgr.ih.use_bus_addr)
		rb_base_addr = adapt->irqmgr.ih.rb_dma_addr;
	else
		rb_base_addr = adapt->irqmgr.ih.gpu_addr;
	AMDGV_DEBUG("ih_rb_addr=0x%llx\n", rb_base_addr);
	/* Ring Buffer base = [39:8] of 40-bit address of
	 * the beginning of the ring buffer
	 *
	 * This is what HW (RTL) expects: 64-bit shifted down by 8 FIRST
	 * - lower 32-bit of the shifted down (40b) addr
	 *     is programmed in IH_RB_BASE
	 * - upper 32-bit (actually, upper 8-bit) of shifted down (40b) addr
	 *     is programmed in IH_RB_BASE_HI.
	 *   HW (rtl) will do: (((IH_RB_BASE_HI << 32) | HI_RB_BASE) << 8)
	 */
	rb_base_addr = rb_base_addr >> 8;
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_BASE), lower_32_bits(rb_base_addr));
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_BASE_HI), upper_32_bits(rb_base_addr));

	/* Use the physical address of the iv ring base as the IH
	 * dummy read address
	 */
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL2), rb_base_addr);

	/* enable IH interrupt */
	interrupt_cntl = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL));
	interrupt_cntl =
		REG_SET_FIELD(interrupt_cntl, INTERRUPT_CNTL, IH_DUMMY_RD_OVERRIDE, 0);
	interrupt_cntl = REG_SET_FIELD(interrupt_cntl, INTERRUPT_CNTL, IH_REQ_NONSNOOP_EN,
				       adapt->irqmgr.ih.use_bus_addr ? 0 : 1);
	interrupt_cntl = REG_SET_FIELD(interrupt_cntl, INTERRUPT_CNTL, GEN_IH_INT_EN, 1);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL), interrupt_cntl);

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
	if (adapt->irqmgr.ih.use_bus_addr) {
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SPACE, 1);
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SNOOP, 1);
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_RO, 0);
	} else {
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SPACE, 4);
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SNOOP, 0);
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_RO, 1);
	}
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_RB_CNTL), ih_rb_cntl);

	/* Program IH_CNTL */
	ih_cntl = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_CNTL));
	ih_cntl = REG_SET_FIELD(ih_cntl, IH_CNTL, MC_WR_CLEAN_CNT, 0x10);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, regIH_CNTL), ih_cntl);

	/* set the writeback address */
	if (adapt->irqmgr.ih.use_bus_addr) {
		wptr_off = adapt->irqmgr.ih.rb_dma_addr + (adapt->irqmgr.ih.wptr_offs * 4);
	} else {
		wptr_off = adapt->irqmgr.ih.gpu_addr + (adapt->irqmgr.ih.wptr_offs * 4);
	}
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

	/* NOTE: changed behavior!!!
	 *  - Always enable BIF DOORBELL aperture for VFs/PF
	 *  - Enabling BIF DOORBELL aperture is independent of
	 *       "adapt->irqmgr.ih.use_doorbell"
	 *  - RLC_V and MMSCH always expect doorbell for job scheduling
	 */
	/* Enable BIF doorbell aperture for each VF */
	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		doorbell_aper = RREG32(SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, RCC,
								   RCC_DOORBELL_APER_EN));
		doorbell_aper = REG_SET_FIELD_NBIO_BLOCK(doorbell_aper, idx_vf, RCC,
							 RCC_DOORBELL_APER_EN,
							 BIF_DOORBELL_APER_EN, 1);
		WREG32(SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, RCC, RCC_DOORBELL_APER_EN),
		       doorbell_aper);
	}
	if (adapt->flags & AMDGV_FLAG_USE_PF) {
		/* Enable BIF doorbell aperture for PF (in case, we use PF) */
		doorbell_aper = RREG32(
			SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_DOORBELL_APER_EN));
		doorbell_aper =
			REG_SET_FIELD(doorbell_aper, RCC_DEV0_EPF0_RCC_DOORBELL_APER_EN,
				      BIF_DOORBELL_APER_EN, 1);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_DOORBELL_APER_EN),
		       doorbell_aper);
	}

	if (adapt->irqmgr.ih.use_doorbell)
		WDOORBELL32(adapt->irqmgr.ih.doorbell_index, 0);
}

static void navi32_ih_iv_ring_hw_fini(struct amdgv_adapter *adapt)
{
	uint32_t interrupt_cntl;

	interrupt_cntl = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL));
	interrupt_cntl =
		REG_SET_FIELD(interrupt_cntl, INTERRUPT_CNTL, IH_DUMMY_RD_OVERRIDE, 1);
	interrupt_cntl = REG_SET_FIELD(interrupt_cntl, INTERRUPT_CNTL, IH_DUMMY_RD_EN, 0);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL), interrupt_cntl);
}

static int navi32_register_interrupt(struct amdgv_adapter *adapt)
{
	struct oss_intr_regrt_entry *intr_entries;
	struct oss_intr_regrt_info *intr_regrt_info;

	intr_regrt_info = oss_malloc(sizeof(struct oss_intr_regrt_info));
	if (intr_regrt_info == NULL) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct oss_intr_regrt_info));
		return -1;
	}

	intr_entries = oss_malloc(sizeof(struct oss_intr_regrt_entry) * 4);
	if (intr_entries == NULL) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct oss_intr_regrt_entry) * 4);
		oss_free(intr_regrt_info);
		return -1;
	}

	/* register MSI vector 0 interrupt handler */
	intr_entries[0].idx_msi_vector = 0;

	if (!adapt->irqmgr.disable_parse_ih) {
		intr_entries[0].intr_cb_type = OSS_INTR_CB_REGULAR;
		intr_entries[0].int_cb.interrupt_cb = amdgv_ih_process_handle;
		intr_entries[0].context = (void *)adapt;
	} else {
		/* interrupt callback parameters contains decoded IH entry */
		intr_entries[0].intr_cb_type = OSS_INTR_CB_DECODED;
		intr_entries[0].int_cb.interrupt_cb2 = amdgv_ih_process_handle2;
		intr_entries[0].context = (void *)adapt;
	}

	/* register MSI vector 1 interrupt handler */
	intr_entries[1].idx_msi_vector = 1;
	intr_entries[1].intr_cb_type = OSS_INTR_CB_REGULAR;
	intr_entries[1].int_cb.interrupt_cb = amdgv_hv_event_handle;
	intr_entries[1].context = (void *)adapt;

	/* register MSI vector 2 interrupt handler */
	intr_entries[2].idx_msi_vector = 2;
	intr_entries[2].intr_cb_type = OSS_INTR_CB_REGULAR;
	intr_entries[2].int_cb.interrupt_cb = amdgv_hv_event_handle;
	intr_entries[2].context = (void *)adapt;

	/* register MSI vector 3 interrupt handler */
	intr_entries[3].idx_msi_vector = 3;
	intr_entries[3].intr_cb_type = OSS_INTR_CB_REGULAR;
	intr_entries[3].int_cb.interrupt_cb = amdgv_ras_fatal_error_handle;
	intr_entries[3].context = (void *)adapt;

	intr_regrt_info->intr_type = OSS_INTR_TYPE_MSIX;
	intr_regrt_info->num_msi_vectors = 4;
	intr_regrt_info->num_intr_entry = 4;
	intr_regrt_info->intr_entries = intr_entries;

	/* register interrupt handler to OS */
	if (oss_register_interrupt(adapt->dev, intr_regrt_info) != 0) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_REGISTER_INTERRUPT_FAIL, 0);
		goto fail;
	}

	adapt->irqmgr.intr_regrt_info = intr_regrt_info;

	return 0;

fail:
	/* free interrupt entries */
	oss_free(intr_regrt_info->intr_entries);

	/* free interrupt registration info */
	oss_free(intr_regrt_info);

	return -1;
}

static const struct amdgv_ih_funcs navi32_ih_funcs = {
	.get_wptr = navi32_ih_get_wptr,
	.decode_iv = navi32_ih_decode_iv,
	.set_rptr = navi32_ih_set_rptr,
	.get_rptr = navi32_ih_get_rptr,
	.process = navi32_ih_process,
	.handle_page_fault = navi32_handle_page_fault,
	.enable_iv_ring = navi32_ih_iv_ring_enable,
	.enable_mbox = navi32_mbox_irq_source_enable,
	.toggle_disp_timer2 = navi32_toggle_disp_timer2,
	.register_interrupt = navi32_register_interrupt,
	.unregister_interrupt = navi32_unregister_interrupt,
	.start_gpu_timer = navi32_irqmgr_next_timer_interrupt,
};

static int navi32_irqmgr_hw_init_internal_set(struct amdgv_adapter *adapt)
{
	int r = 0;
	if (!adapt->irqmgr.disable_parse_ih) {
		r = amdgv_ih_ring_set(adapt);
	}
	return r;
}

static int navi32_irqmgr_enable_interrupt(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.disable_parse_ih)
		return 0;

	if (navi32_irqmgr_hw_init_internal_set(adapt))
		return AMDGV_FAILURE;

	/* initialize IH hw block */
	navi32_ih_iv_ring_hw_init(adapt);

	/* enable IH interrupts */
	navi32_ih_iv_ring_enable(adapt, true);

	/* enable IRQ source */
	navi32_mbox_irq_source_enable(adapt, true);

	return 0;
}

static int navi32_irqmgr_disable_interrupt(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.disable_parse_ih)
		return 0;

	/* disable IRQ source */
	navi32_mbox_irq_source_enable(adapt, false);

	/* disable IH interrupts */
	navi32_ih_iv_ring_enable(adapt, false);

	/* finish IH hw block */
	navi32_ih_iv_ring_hw_fini(adapt);

	return 0;
}

static void navi32_write_virtualized_interrupt(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t idx_table, uint64_t message_address, 
	uint32_t message_data, uint32_t vector_control, bool is_direct_write)
{
	uint32_t offset, *tab;
	struct amdgv_vf_device *vf;
	uint32_t messageAddressLow = (uint32_t)(message_address & 0xFFFFFFFF);
	uint32_t messageAddressHigh = (uint32_t)(message_address >> 32);
	struct amdgv_virtualized_interrupt_info *vf_irq_info;

	if (idx_vf >= AMDGV_MAX_VF_NUM)
		return;

	if(idx_table >= NAVI32_MSIX_TABLE_ENTRY_COUNT)
	{
		amdgv_put_log(idx_vf, AMDGV_LOG_DRIVER_INVALID_VALUE, (uint64_t)idx_table);
		return;
	}

	/* if this is called when vf is in full access, update immediately */
	if (is_full_access_vf(idx_vf))
		is_direct_write = true;

	/* if is_direct_write is true, write the interrupt table directly to the VF */
	if (is_direct_write) {
		vf = &adapt->array_vf[idx_vf];

		/* enable MMIO register write, FB, DOORBELL VF access */
		if (amdgv_gpuiov_get_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE) != true)
		{
			amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, true);
		}

		offset = SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, RCC, GFXMSIX_VECT0_ADDR_LO);
		AMDGV_INFO("GFXMSIX_VECT0_ADDR_LO offset: 0x%x\n", offset);
		tab = (uint32_t *)vf->res.mmio + offset + idx_table * NAVI32_MSIX_TABLE_ENTRY_SIZE_DWORD;
		oss_mm_write32(tab + 0, messageAddressLow);
		oss_mm_write32(tab + 1, messageAddressHigh);
		oss_mm_write32(tab + 2, message_data);
		oss_mm_write32(tab + 3, vector_control);

		tab = (uint32_t *)vf->res.mmio + offset + idx_table * NAVI32_MSIX_TABLE_ENTRY_SIZE_DWORD;
		AMDGV_INFO("Read back interrupt table entry %d: 0x%x, 0x%x, 0x%x, 0x%x\n", idx_table,
			oss_mm_read32(tab + 0), oss_mm_read32(tab + 1), 
			oss_mm_read32(tab + 2), oss_mm_read32(tab + 3));
	} else {
		vf_irq_info = &adapt->irqmgr.virtualized_interrupt_info_db[idx_vf];
		if (vf_irq_info->msix_tab == NULL)
		{
			vf_irq_info->msix_tab = oss_zalloc(NAVI32_MSIX_TABLE_ENTRY_COUNT * NAVI32_MSIX_TABLE_ENTRY_SIZE_DWORD * sizeof(uint32_t));
			if (vf_irq_info->msix_tab == NULL)
				return;
		}

		vf_irq_info->table_entry_update |= 1 << idx_table;
		vf_irq_info->msix_tab[idx_table * NAVI32_MSIX_TABLE_ENTRY_SIZE_DWORD] = messageAddressLow;
		vf_irq_info->msix_tab[idx_table * NAVI32_MSIX_TABLE_ENTRY_SIZE_DWORD + 1] = messageAddressHigh;
		vf_irq_info->msix_tab[idx_table * NAVI32_MSIX_TABLE_ENTRY_SIZE_DWORD + 2] = message_data;
		vf_irq_info->msix_tab[idx_table * NAVI32_MSIX_TABLE_ENTRY_SIZE_DWORD + 3] = vector_control;
	}
}

static void navi32_update_virtualized_interrupt(struct amdgv_adapter *adapt, uint32_t idx_vf) 
{
	struct amdgv_virtualized_interrupt_info *vf_irq_info;
	int i, entry;
	uint32_t offset, *tab;
	struct amdgv_vf_device *vf;

	if (idx_vf >= AMDGV_MAX_VF_NUM)
		return;

	vf_irq_info = &adapt->irqmgr.virtualized_interrupt_info_db[idx_vf];
	if (vf_irq_info->msix_tab == NULL)
		return;

	vf = &adapt->array_vf[idx_vf];
	if (vf == NULL)
		return;

	/* enable MMIO register write, FB, DOORBELL VF access */
	if (amdgv_gpuiov_get_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE) != true)
	{
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, true);
	}

	offset = SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, RCC, GFXMSIX_VECT0_ADDR_LO);
	AMDGV_INFO("GFXMSIX_VECT0_ADDR_LO offset: 0x%x\n", offset);

	for (i = 0, entry = 0; i < NAVI32_MSIX_TABLE_ENTRY_COUNT; i++, entry += 4) {
		tab = (uint32_t *)vf->res.mmio + entry + offset;
		if (vf_irq_info->table_entry_update & (1 << i)) {
			oss_mm_write32(tab + 0, vf_irq_info->msix_tab[entry + 0]);
			oss_mm_write32(tab + 1, vf_irq_info->msix_tab[entry + 1]);
			oss_mm_write32(tab + 2, vf_irq_info->msix_tab[entry + 2]);
			oss_mm_write32(tab + 3, vf_irq_info->msix_tab[entry + 3]);
		}
	}

	for (i = 0, entry = 0; i < NAVI32_MSIX_TABLE_ENTRY_COUNT; i++, entry += 4) {
		tab = (uint32_t *)vf->res.mmio + entry + offset;
		AMDGV_INFO("Table entry count: %d table offset: 0x%x\n", entry, tab);
		AMDGV_INFO("Read back interrupt table entry %d: 0x%x, 0x%x, 0x%x, 0x%x\n", i, 
			oss_mm_read32(tab + 0), oss_mm_read32(tab + 1), 
			oss_mm_read32(tab + 2), oss_mm_read32(tab + 3));
	}

	// clear this VF's pending interrupt info
	vf_irq_info->table_entry_update = 0;
	oss_memset(vf_irq_info->msix_tab, 0, NAVI32_MSIX_TABLE_ENTRY_COUNT * NAVI32_MSIX_TABLE_ENTRY_SIZE_DWORD * sizeof(uint32_t));
}

static int navi32_vf_disp_timer2_control(struct amdgv_adapter *adapt, uint32_t idx_vf, bool enable)
{
	uint32_t pwr_disp_timer2_control;
	uint32_t mmio_byte_off;

	if (idx_vf == AMDGV_PF_IDX || idx_vf >= adapt->max_num_vf) {
		AMDGV_ERROR("%s: invalid idx_vf %u\n", __func__, idx_vf);
		return AMDGV_FAILURE;
	}

	mmio_byte_off = SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER2_CONTROL) * 4;

	pwr_disp_timer2_control = oss_mm_read32((uint8_t *)adapt->array_vf[idx_vf].res.mmio + mmio_byte_off);
	if (enable)
		pwr_disp_timer2_control = REG_SET_FIELD(pwr_disp_timer2_control, PWR_DISP_TIMER2_CONTROL, DISP_TIMER_INT_ENABLE, 1);
	else
		pwr_disp_timer2_control = REG_SET_FIELD(pwr_disp_timer2_control, PWR_DISP_TIMER2_CONTROL, DISP_TIMER_INT_DISABLE, 1);
	oss_mm_write32((uint8_t *)adapt->array_vf[idx_vf].res.mmio + mmio_byte_off, pwr_disp_timer2_control);

	return 0;
}

static int navi32_irqmgr_sw_init(struct amdgv_adapter *adapt)
{
	if (amdgv_irqmgr_sw_init(adapt))
		return AMDGV_FAILURE;

	adapt->irqmgr.ih_funcs = &navi32_ih_funcs;
	adapt->irqmgr.hv_event_process = navi32_hv_event_process;

	adapt->irqmgr.ih.use_doorbell = true;
	adapt->irqmgr.ih.doorbell_index = (adapt->doorbell_index.ih) << 1;

	adapt->irqmgr.write_virtualized_interrupt = navi32_write_virtualized_interrupt;
	adapt->irqmgr.update_virtualized_interrupt = navi32_update_virtualized_interrupt;

	adapt->irqmgr.vf_disp_timer2_control = navi32_vf_disp_timer2_control;

	/* register interrupt handler */
	if (!amdgv_in_live_update_seq()) {
		if (navi32_register_interrupt(adapt) < 0) {
			return AMDGV_FAILURE;
		}
	} else {
		return 0;
	}

	adapt->irqmgr.enable_hw_interrupt = navi32_irqmgr_enable_interrupt;
	adapt->irqmgr.disable_hw_interrupt = navi32_irqmgr_disable_interrupt;

	return 0;
}

static int navi32_irqmgr_sw_fini(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	adapt->irqmgr.enable_hw_interrupt = NULL;
	adapt->irqmgr.disable_hw_interrupt = NULL;
	adapt->irqmgr.write_virtualized_interrupt = NULL;
	adapt->irqmgr.update_virtualized_interrupt = NULL;
	adapt->irqmgr.vf_disp_timer2_control = NULL;

	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_NUM; idx_vf++) {
		if (adapt->irqmgr.virtualized_interrupt_info_db[idx_vf].msix_tab) {
			oss_free(adapt->irqmgr.virtualized_interrupt_info_db[idx_vf].msix_tab);
			adapt->irqmgr.virtualized_interrupt_info_db[idx_vf].msix_tab = NULL;
		}
	}
	
	navi32_unregister_interrupt(adapt);

	return amdgv_irqmgr_sw_fini(adapt);
}

static int navi32_irqmgr_hw_init(struct amdgv_adapter *adapt)
{
	return navi32_irqmgr_enable_interrupt(adapt);
}

static int navi32_irqmgr_hw_fini(struct amdgv_adapter *adapt)
{
	return navi32_irqmgr_disable_interrupt(adapt);
}

static int navi32_irqmgr_pre_reset(struct amdgv_adapter *adapt)
{

	if (adapt->irqmgr.disable_parse_ih)
		return 0;

	amdgv_irqmgr_disable(adapt);

	return 0;
}

struct amdgv_init_func navi32_irqmgr_func = {
	.name = "navi32_irqmgr_func",
	.sw_init = navi32_irqmgr_sw_init,
	.sw_fini = navi32_irqmgr_sw_fini,
	.hw_init = navi32_irqmgr_hw_init,
	.hw_fini = navi32_irqmgr_hw_fini,
	.pre_reset = navi32_irqmgr_pre_reset,
	.hw_live_init = navi32_irqmgr_hw_init_internal_set,
};
