/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv_guard.h>
#include <amdgv_irqmgr.h>
#include <amdgv_oss.h>
#include <amdgv_oss_wrapper.h>
#include <amdgv_sched.h>

#include "mi300.h"
#include "mi300_irqmgr.h"

#include "mi300/NBIO/nbio_7_9_0_offset.h"
#include "mi300/NBIO/nbio_7_9_0_sh_mask.h"
#include "mi300_gpuiov.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

static int mi300_ih_iv_ring_enable(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t ih_rb_cntl = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_CNTL));

	if (enable) {
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RB_ENABLE, 1);
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, ENABLE_INTR, 1);
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_CNTL), ih_rb_cntl);
		adapt->irqmgr.ih.enabled = true;
	} else {
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RB_ENABLE, 0);
		ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, ENABLE_INTR, 0);
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_CNTL), ih_rb_cntl);

		/* set rptr, wptr to 0 */
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_RPTR), 0);
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR), 0);
		adapt->irqmgr.ih.enabled = false;
		adapt->irqmgr.ih.rptr = 0;
	}

	return 0;
}

static uint32_t mi300_ih_get_wptr(struct amdgv_adapter *adapt)
{
	uint32_t wptr, tmp;

	wptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR));
	if (REG_GET_FIELD(wptr, IH_RB_WPTR, RB_OVERFLOW)) {
		wptr = REG_SET_FIELD(wptr, IH_RB_WPTR, RB_OVERFLOW, 0);

		AMDGV_WARN("IH ring buffer overflow (0x%08X, 0x%08X, 0x%08X)\n", wptr,
			   adapt->irqmgr.ih.rptr, wptr & adapt->irqmgr.ih.ptr_mask);

		adapt->irqmgr.ih.rptr = wptr & adapt->irqmgr.ih.ptr_mask;
		adapt->irqmgr.ih_funcs->set_rptr(adapt);

		tmp = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_CNTL));
		tmp = REG_SET_FIELD(tmp, IH_RB_CNTL, WPTR_OVERFLOW_CLEAR, 1);
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_CNTL), tmp);
	}

	return (wptr & adapt->irqmgr.ih.ptr_mask);
}

static void mi300_ih_set_rptr(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.ih.use_doorbell)
		WDOORBELL32(adapt->irqmgr.ih.doorbell_index, adapt->irqmgr.ih.rptr);
	else
		WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_RPTR), adapt->irqmgr.ih.rptr);
}

static uint32_t mi300_ih_get_rptr(struct amdgv_adapter *adapt)
{
	uint32_t rptr;

	rptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_RPTR));

	return (rptr & adapt->irqmgr.ih.ptr_mask);
}

static void mi300_ih_decode_iv(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
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

static void mi300_set_gpu_timer_ref_frequency(struct amdgv_adapter *adapt)
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

static void mi300_irqmgr_ack_timer_interrupt(struct amdgv_adapter *adapt)
{
	uint32_t pwr_display_timer_control =
		 REG_SET_FIELD(0, PWR_DISP_TIMER_CONTROL, DISP_TIMER_INT_STAT_AK, 1);
	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_CONTROL), pwr_display_timer_control);
}

static void mi300_irqmgr_next_timer_interrupt(struct amdgv_adapter *adapt, uint64_t micro_seconds)
{
	uint32_t pwr_ih_control;
	uint32_t pwr_display_timer_control = 0;
	uint32_t timer_ticks = 1;

	mi300_set_gpu_timer_ref_frequency(adapt);

	pwr_ih_control = RREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL));
	pwr_ih_control = REG_SET_FIELD(pwr_ih_control, PWR_IH_CONTROL,
			DISP_TIMER_TRIGGER_MASK, micro_seconds ? 0 : 1);
	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_IH_CONTROL), pwr_ih_control);

	pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
									DISP_TIMER_INT_TYPE, 1); // level-based
	pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
									DISP_TIMER_INT_MASK, 1);
	if (micro_seconds) {
		if (micro_seconds > MI300_TIMER_MICROSEC_DELAY) {
			timer_ticks = micro_seconds - MI300_TIMER_MICROSEC_DELAY;
		}
		pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
									DISP_TIMER_INT_COUNT,  timer_ticks);
		WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_CONTROL), pwr_display_timer_control);
		oss_udelay(MI300_TIMER_MICROSEC_DELAY);

		pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
										DISP_TIMER_INT_MASK, 0);
		pwr_display_timer_control = REG_SET_FIELD(pwr_display_timer_control, PWR_DISP_TIMER_CONTROL,
										DISP_TIMER_INT_ENABLE, 1);
		WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_CONTROL), pwr_display_timer_control);
	} else {
		WREG32(SOC15_REG_OFFSET(SMUIO, 0, regPWR_DISP_TIMER_CONTROL), pwr_display_timer_control);
	}
}

static int mi300_ih_process(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	if (MI300_IH_IV_CLIENTID_GRBM_CP == entry->client_id) {
		if (MI300_IH_IV_SRCID_CP_EOP_INTERRUPT == entry->src_id &&
				 adapt->irqmgr.ih_submission_interrupt_handler) {
			adapt->irqmgr.ih_submission_interrupt_handler(adapt->irqmgr.ih_submission_interrupt_context);
			return 0;
		}
	} else if (MI300_IH_IV_CLIENTID_SDMA0 == entry->client_id) {
		if (MI300_IH_IV_SRCID_SDMA_CTXEMPTY == entry->src_id) {
			return 0; // ignore it
		} else if (MI300_IH_IV_SRCID_SDMA_TRAP == entry->src_id &&
				 adapt->irqmgr.ih_submission_interrupt_handler) {
			adapt->irqmgr.ih_submission_interrupt_handler(adapt->irqmgr.ih_submission_interrupt_context);
			return 0;
		}
	} else if (MI300_IH_IV_CLIENTID_PWR == entry->client_id) {
		if (MI300_IH_IV_SRCID_DISP_TIMER == entry->src_id) {
			mi300_irqmgr_ack_timer_interrupt(adapt);
			if (adapt->irqmgr.ih_gpu_timer_handler) {
				adapt->irqmgr.ih_gpu_timer_handler(adapt->irqmgr.ih_gpu_timer_context);
			}
			return 0;
		}
	}
	return amdgv_ih_iv_ring_entry_process(adapt, entry);
}

static void mi300_mbox_irq_source_enable(struct amdgv_adapter *adapt, bool enable)
{
	int val;
	uint32_t tmp;

	val = enable ? 1 : 0;

	/* enable or disable mailbox ack and valid interrupts */
	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_INT_CNTL));

	tmp = REG_SET_FIELD(tmp, BIF_BX_PF0_MAILBOX_INT_CNTL, VALID_INT_EN, val);
	tmp = REG_SET_FIELD(tmp, BIF_BX_PF0_MAILBOX_INT_CNTL, ACK_INT_EN, val);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_MAILBOX_INT_CNTL), tmp);
}


static void mi300_handle_page_fault(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	uint64_t addr;
	uint32_t tmp, src_id, ring_id, vm_id;

	addr = (uint64_t)entry->src_data[0] << 12;
	addr |= ((uint64_t)entry->src_data[1] & 0x1f) << 44;

	src_id = entry->src_id;
	ring_id = entry->ring_id;
	vm_id = entry->vm_id;

	switch (entry->client_id) {
	case IH_IV_CLIENTID_UTCL2: /* cover GC/GFXHUB */
		tmp = adapt->vmhub[0].fault_status;
		AMDGV_ERROR("DMAR: trapped by GMC as page fault, client_id=%u, src_id=%u, "
			    "ring_id=%u, vm_id=%u address=0x%016llx, "
			    "VM_L2_PROTECTION_FAULT_STATUS=%08x\n",
			    entry->client_id, src_id, ring_id, vm_id, addr, RREG32(tmp));
		tmp = RREG32(adapt->vmhub[0].fault_cntl);
		tmp |= 1; /* to clear the page fault */
		WREG32(adapt->vmhub[0].fault_cntl, tmp);
		break;

	case IH_IV_CLIENTID_VMC: /* cover MMHUB0 */
		tmp = adapt->vmhub[1].fault_status;
		AMDGV_ERROR("DMAR: trapped by GMC as page fault, client_id=%u, src_id=%u, "
			    "ring_id=%u, vm_id=%u address=0x%016llx, "
			    "VM_L2_PROTECTION_FAULT_STATUS=%08x\n",
			    entry->client_id, src_id, ring_id, vm_id, addr, RREG32(tmp));
		tmp = RREG32(adapt->vmhub[1].fault_cntl);
		tmp |= 1; /* to clear the page fault */
		WREG32(adapt->vmhub[1].fault_cntl, tmp);
		break;
	default:
		AMDGV_ERROR("Unknown/Unhandled Interrupt Received: client_id=%u "
			    "address=0x%016llx\n",
			    entry->client_id, addr);
		break;
	}
}


// Add the rest of the FW sources
static int mi300_hv_event_process(struct amdgv_adapter *adapt)
{
	uint32_t intr_bits;
	uint32_t sta_bits;
	uint32_t active_vf;

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
		amdgv_gpuiov_get_active_vf_idx(adapt, MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV,
					       &active_vf);
		amdgv_sched_queue_event(adapt, active_vf, AMDGV_EVENT_HW_SCHED_RESET_VF,
					AMDGV_SCHED_BLOCK_GFX);

		amdgv_guard_add_active_event(adapt, active_vf, AMDGV_GUARD_EVENT_ALL_INT);
	}

	if (sta_bits & AMDGV_UVD_HANG_NEED_FLR_INTR) {
		sta_bits &= ~AMDGV_UVD_HANG_NEED_FLR_INTR;
		amdgv_gpuiov_get_active_vf_idx(adapt, MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH,
					       &active_vf);
		amdgv_sched_queue_event(adapt, active_vf, AMDGV_EVENT_HW_SCHED_RESET_VF,
					AMDGV_SCHED_BLOCK_VCN);

		amdgv_guard_add_active_event(adapt, active_vf, AMDGV_GUARD_EVENT_ALL_INT);
	}

	/* restore interrupt */
	amdgv_gpuiov_set_intr(adapt, intr_bits);

	oss_spin_unlock(adapt->irqmgr.hv_event_lock);

	if (sta_bits != 0)
		AMDGV_INFO("some interrupts(0x%x) are not handled\n", intr_bits);

	return 0;
}

static void mi300_ih_iv_ring_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t interrupt_cntl, ih_cntl;
	uint32_t ih_doorbell_range = 0;
	uint32_t ih_doorbell_ctrl = 0;
	uint32_t ih_rb_cntl;
	uint32_t rptr, doorbell_aper;
	uint64_t rb_base_addr, wptr_off;

	/* Program IH_RB_BASE */
	if (adapt->irqmgr.ih.use_bus_addr)
		rb_base_addr = adapt->irqmgr.ih.rb_dma_addr;
	else
		rb_base_addr = adapt->irqmgr.ih.gpu_addr;
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_BASE), rb_base_addr >> 8);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_BASE_HI), rb_base_addr >> 40);
	AMDGV_DEBUG("rb_base_addr=0x%llx\n", rb_base_addr);

	/* Use the physical address of the iv ring base as the IH
	 * dummy read address
	 */
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL2), rb_base_addr >> 8);

	/* enable IH interrupt */
	interrupt_cntl = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL));
	interrupt_cntl =
		REG_SET_FIELD(interrupt_cntl, BIF_BX0_INTERRUPT_CNTL, IH_REQ_NONSNOOP_EN,
			      adapt->irqmgr.ih.use_bus_addr ? 0 : 1);
	interrupt_cntl =
		REG_SET_FIELD(interrupt_cntl, BIF_BX0_INTERRUPT_CNTL, GEN_IH_INT_EN, 1);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL), interrupt_cntl);

	/* Program IH_RB_CNTL */
	ih_rb_cntl = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_CNTL));
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RB_ENABLE, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, ENABLE_INTR, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RB_SIZE,
				   adapt->irqmgr.ih.ring_size_log2);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, WPTR_OVERFLOW_CLEAR, 1);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, WPTR_OVERFLOW_ENABLE, 1);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, WPTR_WRITEBACK_ENABLE, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RB_GPU_TS_ENABLE, 1);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, RPTR_REARM, 1);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SWAP, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_VMID, 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SPACE,
				   adapt->irqmgr.ih.use_bus_addr ? 1 : 4);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_SNOOP,
				   adapt->irqmgr.ih.use_bus_addr ? 1 : 0);
	ih_rb_cntl = REG_SET_FIELD(ih_rb_cntl, IH_RB_CNTL, MC_RO,
				   adapt->irqmgr.ih.use_bus_addr ? 0 : 1);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_CNTL), ih_rb_cntl);

	/* Program IH_CNTL */
	ih_cntl = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_CNTL));
	ih_cntl = REG_SET_FIELD(ih_cntl, IH_CNTL, MC_WR_CLEAN_CNT, 0x10);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_CNTL), ih_cntl);

	/* set the writeback address */
	if (adapt->irqmgr.ih.use_bus_addr)
		wptr_off = adapt->irqmgr.ih.rb_dma_addr + (adapt->irqmgr.ih.wptr_offs * 4);
	else
		wptr_off = adapt->irqmgr.ih.gpu_addr + (adapt->irqmgr.ih.wptr_offs * 4);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR_ADDR_LO), lower_32_bits(wptr_off));
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR_ADDR_HI), upper_32_bits(wptr_off));
	AMDGV_DEBUG("writeback_addr=0x%llx\n", wptr_off);

	/* set rptr, wptr to 0 */
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_RPTR), 0);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR), 0);

	/* set rptr with doorbell reg */
	rptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_DOORBELL_RPTR));
	/* use doorbell */
	if (adapt->irqmgr.ih.use_doorbell) {
		/* doorbell index */
		rptr = REG_SET_FIELD(rptr, IH_DOORBELL_RPTR, OFFSET,
				     adapt->irqmgr.ih.doorbell_index);
		rptr = REG_SET_FIELD(rptr, IH_DOORBELL_RPTR, ENABLE, 1);
	} else {
		rptr = REG_SET_FIELD(rptr, IH_DOORBELL_RPTR, ENABLE, 0);
	}
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_DOORBELL_RPTR), rptr);

	if (adapt->irqmgr.ih.use_doorbell) {
		/* assign ih doorbell range */
		ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range, DOORBELL0_CTRL_ENTRY_0,
						  BIF_DOORBELL0_RANGE_OFFSET_ENTRY,
						  adapt->irqmgr.ih.doorbell_index);
		ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range, DOORBELL0_CTRL_ENTRY_0,
						  BIF_DOORBELL0_RANGE_SIZE_ENTRY, 0x8);

		ih_doorbell_ctrl = REG_SET_FIELD(ih_doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						 S2A_DOORBELL_PORT1_ENABLE, 1);
		ih_doorbell_ctrl = REG_SET_FIELD(ih_doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						 S2A_DOORBELL_PORT1_AWID, 0);
		ih_doorbell_ctrl = REG_SET_FIELD(ih_doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						 S2A_DOORBELL_PORT1_RANGE_OFFSET, 0);
		ih_doorbell_ctrl = REG_SET_FIELD(ih_doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						 S2A_DOORBELL_PORT1_RANGE_SIZE, 0x8);
		ih_doorbell_ctrl = REG_SET_FIELD(ih_doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						 S2A_DOORBELL_PORT1_AWADDR_31_28_VALUE, 0);

		WREG32_SOC15(NBIO, 0, regDOORBELL0_CTRL_ENTRY_0, ih_doorbell_range);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regS2A_DOORBELL_ENTRY_3_CTRL),
		       ih_doorbell_ctrl);

		/* Enable BIF doorbell aperture */
		doorbell_aper = RREG32(
			SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_DOORBELL_APER_EN));
		doorbell_aper =
			REG_SET_FIELD(doorbell_aper, RCC_DEV0_EPF0_RCC_DOORBELL_APER_EN,
				      BIF_DOORBELL_APER_EN, 1);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_DOORBELL_APER_EN),
		       doorbell_aper);

		WDOORBELL32(adapt->irqmgr.ih.doorbell_index, 0);
	}
}

int mi300_irqmgr_restore_and_init(struct amdgv_adapter *adapt,
				  struct mi300_irqmgr_reset_state *reset_state)
{
	int ret;
	if (adapt->irqmgr.disable_parse_ih == false)
		return 0;

	WREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_HOST_PATH_CNTL),
	       reset_state->hdp_host_path_cntl);

	/* IH_RB_BASE */
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_BASE), reset_state->rb_base_addr);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_BASE_HI), reset_state->rb_base_addr_hi);

	/* Setup IH dummy reads */
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL2),
	       reset_state->interrupt_cntl2);

	/* enable IH interrupt */
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL),
	       reset_state->interrupt_cntl);

	/* Program IH_RB_CNTL */
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_CNTL), reset_state->ih_rb_cntl);

	/* Program IH_CNTL */
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_CNTL), reset_state->ih_cntl);

	/* set the writeback address */
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR_ADDR_LO), reset_state->wptr_off_lo);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR_ADDR_HI), reset_state->wptr_off_hi);

	/* set rptr, wptr */
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_RPTR), reset_state->rptr);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR), reset_state->wptr);

	/* set rptr with doorbell reg */
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_DOORBELL_RPTR), reset_state->doorbell_rptr);

	/* Enable BIF doorbell aperture */
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_DOORBELL_APER_EN),
	       reset_state->doorbell_aper);

	/* enable IH interrupts */
	ret = mi300_ih_iv_ring_enable(adapt, true);
	if (ret)
		return ret;

	/* enable IRQ source */
	mi300_mbox_irq_source_enable(adapt, true);

	return 0;
}

void mi300_irqmgr_save_and_fini(struct amdgv_adapter *adapt,
				struct mi300_irqmgr_reset_state *reset_state)
{
	if (adapt->irqmgr.disable_parse_ih == false)
		return;

	/* disable IRQ source */
	mi300_mbox_irq_source_enable(adapt, false);

	/* read rptr and wptr before disable the ring or they will be zero */
	reset_state->rptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_RPTR));
	AMDGV_INFO("rptr = 0x%08x!\n", reset_state->rptr);
	reset_state->wptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR));
	AMDGV_INFO("wptr = 0x%08x!\n", reset_state->wptr);

	/* disable IH interrupts */
	mi300_ih_iv_ring_enable(adapt, false);

	reset_state->hdp_host_path_cntl =
		RREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_HOST_PATH_CNTL));

	/* IH_RB_BASE */
	reset_state->rb_base_addr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_BASE));
	reset_state->rb_base_addr_hi = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_BASE_HI));

	reset_state->interrupt_cntl2 =
		RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL2));

	/* IH interrupt */
	reset_state->interrupt_cntl =
		RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_INTERRUPT_CNTL));

	/* IH_RB_CNTL */
	reset_state->ih_rb_cntl = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_CNTL));

	/* IH_CNTL */
	reset_state->ih_cntl = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_CNTL));

	/* writeback address */
	reset_state->wptr_off_lo = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR_ADDR_LO));
	reset_state->wptr_off_hi = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RB_WPTR_ADDR_HI));

	/* rptr doorbell reg */
	reset_state->doorbell_rptr = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_DOORBELL_RPTR));

	/* BIF doorbell aperture */
	reset_state->doorbell_aper =
		RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_DOORBELL_APER_EN));
}

static int mi300_register_interrupt(struct amdgv_adapter *adapt)
{
	struct oss_intr_regrt_entry *intr_entries;
	struct oss_intr_regrt_info *intr_regrt_info;

	intr_regrt_info = oss_malloc(sizeof(struct oss_intr_regrt_info));
	if (intr_regrt_info == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct oss_intr_regrt_info));
		return -1;
	}

	intr_entries = oss_malloc(sizeof(struct oss_intr_regrt_entry) * 4);
	if (intr_entries == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
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
		AMDGV_ERROR("failed to register interrupt handler!\n");
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

static void mi300_unregister_interrupt(struct amdgv_adapter *adapt)
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

static const struct amdgv_ih_funcs mi300_ih_funcs = {
	.get_wptr = mi300_ih_get_wptr,
	.decode_iv = mi300_ih_decode_iv,
	.set_rptr = mi300_ih_set_rptr,
	.get_rptr = mi300_ih_get_rptr,
	.process = mi300_ih_process,
	.enable_iv_ring = mi300_ih_iv_ring_enable,
	.enable_mbox = mi300_mbox_irq_source_enable,
	.handle_page_fault = mi300_handle_page_fault,
	.register_interrupt = mi300_register_interrupt,
	.unregister_interrupt = mi300_unregister_interrupt,
	.start_gpu_timer = mi300_irqmgr_next_timer_interrupt,
};

static int mi300_irqmgr_hw_init_internal_set(struct amdgv_adapter *adapt)
{
	if (amdgv_ih_ring_set(adapt))
		return AMDGV_FAILURE;
	return 0;
}

static int mi300_irqmgr_enable_interrupt(struct amdgv_adapter *adapt)
{
	uint32_t intr_bits;

	mi300_irqmgr_golden_init(adapt);

	/* config GPUIOV interrupt */
	amdgv_gpuiov_get_intr(adapt, &intr_bits);
	intr_bits &= ~AMDGV_UVD_HANG_NEED_FLR_INTR;
	amdgv_gpuiov_set_intr(adapt, intr_bits);

	if (mi300_irqmgr_hw_init_internal_set(adapt))
		return AMDGV_FAILURE;

	/* initialize IH hw block */
	mi300_ih_iv_ring_hw_init(adapt);

	/* enable IH interrupts */
	mi300_ih_iv_ring_enable(adapt, true);

	/* enable IRQ source */
	mi300_mbox_irq_source_enable(adapt, true);

	return 0;
}

static int mi300_irqmgr_disable_interrupt(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.disable_parse_ih)
		return 0;

	/* disable IRQ source */
	mi300_mbox_irq_source_enable(adapt, false);

	/* disable IH interrupts */
	mi300_ih_iv_ring_enable(adapt, false);

	return 0;
}

static int mi300_irqmgr_sw_init(struct amdgv_adapter *adapt)
{
	if (amdgv_irqmgr_sw_init(adapt))
		return AMDGV_FAILURE;

	adapt->irqmgr.ih_funcs = &mi300_ih_funcs;
	adapt->irqmgr.hv_event_process = mi300_hv_event_process;

	adapt->irqmgr.ih.use_doorbell = true;

	adapt->irqmgr.ih.doorbell_index = (adapt->doorbell_index.ih) << 1;

	if (!adapt->opt.skip_hw_init) {
		/* register interrupt handler */
		if (mi300_register_interrupt(adapt) < 0) {
			AMDGV_ERROR("failed to register interrupt!\n");
			return -1;
		}
	}

	adapt->irqmgr.enable_hw_interrupt = mi300_irqmgr_enable_interrupt;
	adapt->irqmgr.disable_hw_interrupt = mi300_irqmgr_disable_interrupt;

	return 0;
}

static int mi300_irqmgr_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->irqmgr.enable_hw_interrupt = NULL;
	adapt->irqmgr.disable_hw_interrupt = NULL;

	if (!adapt->fini_opt.skip_hw_fini) {
		mi300_unregister_interrupt(adapt);
	}
	return amdgv_irqmgr_sw_fini(adapt);
}

void mi300_irqmgr_golden_init(struct amdgv_adapter *adapt)
{
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_STORM_CLIENT_LIST_CNTL), 0x00040000);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_INT_FLOOD_CNTL), 8);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_RETRY_INT_CAM_CNTL), 0x00015810);
}

static int mi300_irqmgr_hw_init(struct amdgv_adapter *adapt)
{
	return mi300_irqmgr_enable_interrupt(adapt);
}

static int mi300_irqmgr_hw_fini(struct amdgv_adapter *adapt)
{
	return mi300_irqmgr_disable_interrupt(adapt);
}

struct amdgv_init_func mi300_irqmgr_func = {
	.name = "mi300_irqmgr_func",
	.sw_init = mi300_irqmgr_sw_init,
	.sw_fini = mi300_irqmgr_sw_fini,
	.hw_init = mi300_irqmgr_hw_init,
	.hw_fini = mi300_irqmgr_hw_fini,
	.hw_live_init = mi300_irqmgr_hw_init_internal_set,
};
