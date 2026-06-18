/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "navi3/NBIO/nbio_4_3_0_offset.h"
#include "navi3/NBIO/nbio_4_3_0_sh_mask.h"
#include "amdgv_device.h"
#include "amdgv_ras.h"
#include "amdgv_nbio.h"
#include "navi32_nbio.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static void navi32_nbio_handle_ras_controller_intr_no_bifring(struct amdgv_adapter *adapt)
{
	uint32_t bif_doorbell_intr_cntl;

	bif_doorbell_intr_cntl = RREG32_SOC15(NBIO, 0,
			regBIF_BX0_BIF_DOORBELL_INT_CNTL);
	AMDGV_DEBUG("bif_doorbell_intr_cntl 0x%x, ras_cntlr_interrupt_status %d\n",
			bif_doorbell_intr_cntl,
			REG_GET_FIELD(bif_doorbell_intr_cntl, BIF_DOORBELL_INT_CNTL,
				RAS_CNTLR_INTERRUPT_STATUS));

	if (REG_GET_FIELD(bif_doorbell_intr_cntl,
			  BIF_DOORBELL_INT_CNTL,
			  RAS_CNTLR_INTERRUPT_STATUS)) {
		/* driver has to clear the interrupt status when bif ring is disabled */
		bif_doorbell_intr_cntl = REG_SET_FIELD(bif_doorbell_intr_cntl,
						BIF_DOORBELL_INT_CNTL,
						RAS_CNTLR_INTERRUPT_CLEAR, 1);
		WREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL,
				bif_doorbell_intr_cntl);

		/* TODO: handle ras controller interrupt */
	}
}

static void navi32_nbio_handle_ras_err_event_athub_intr_no_bifring(struct amdgv_adapter *adapt)
{
	uint32_t bif_doorbell_intr_cntl;

	union amdgv_sched_event_data event_data;
	oss_memset(&event_data, 0, sizeof(union amdgv_sched_event_data));

	bif_doorbell_intr_cntl = RREG32_SOC15(NBIO, 0,
			regBIF_BX0_BIF_DOORBELL_INT_CNTL);
	AMDGV_DEBUG("bif_doorbell_intr_cntl 0x%x, ras_athub_err_event_interrupt_status %d\n",
			bif_doorbell_intr_cntl,
			REG_GET_FIELD(bif_doorbell_intr_cntl, BIF_DOORBELL_INT_CNTL,
				RAS_ATHUB_ERR_EVENT_INTERRUPT_STATUS));

	if (REG_GET_FIELD(bif_doorbell_intr_cntl,
			  BIF_DOORBELL_INT_CNTL,
			  RAS_ATHUB_ERR_EVENT_INTERRUPT_STATUS)) {
		/* driver has to clear the interrupt status when bif ring is disabled */
		bif_doorbell_intr_cntl = REG_SET_FIELD(bif_doorbell_intr_cntl,
						BIF_DOORBELL_INT_CNTL,
						RAS_ATHUB_ERR_EVENT_INTERRUPT_CLEAR, 1);
		WREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL,
				bif_doorbell_intr_cntl);

		/* handle poison consumption check */
		amdgv_ecc_get_poison_stat(adapt);

		adapt->ecc.fatal_error = true;
		oss_atomic_set(adapt->in_ecc_recovery, 1);
		event_data.fed_data.src = AMDGV_FED_SRC_DF_SYNC_FLOOD;

		amdgv_sched_queue_event_ex(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_RAS_FED,
					AMDGV_SCHED_BLOCK_ALL, event_data);
	}
}

static int navi32_nbio_set_ras_err_event_athub_irq_state(struct amdgv_adapter *adapt,
							bool state)
{
	/* use vector 4 for fatal error interrupt */
	uint32_t bif_intr_cntl;

	bif_intr_cntl = RREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_INTR_CNTL);
	AMDGV_DEBUG("regBIF_BX0_BIF_INTR_CNTL 0x%x, RAS_INTR_VEC_SEL %d\n",
			bif_intr_cntl,
			REG_GET_FIELD(bif_intr_cntl, BIF_BX0_BIF_INTR_CNTL,
				RAS_INTR_VEC_SEL));
	if (state == true) {
		/* set interrupt vector select bit to 1 to select
		 * vetcor 4 for bare metal case */
		bif_intr_cntl = REG_SET_FIELD(bif_intr_cntl,
					      BIF_INTR_CNTL,
					      RAS_INTR_VEC_SEL, 1);
		WREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_INTR_CNTL, bif_intr_cntl);
	} else {
		AMDGV_WARN("ras_err_event_athub_irq_state got state=false, nothing to be done.\n");
	}


	return 0;
}

const struct nbio_hdp_flush_reg navi32_nbio_hdp_flush_reg = {
	.ref_and_mask_cp0 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP0_MASK,
	.ref_and_mask_cp1 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP1_MASK,
	.ref_and_mask_cp2 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP2_MASK,
	.ref_and_mask_cp3 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP3_MASK,
	.ref_and_mask_cp4 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP4_MASK,
	.ref_and_mask_cp5 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP5_MASK,
	.ref_and_mask_cp6 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP6_MASK,
	.ref_and_mask_cp7 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP7_MASK,
	.ref_and_mask_cp8 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP8_MASK,
	.ref_and_mask_cp9 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP9_MASK,
	.ref_and_mask_sdma0 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__SDMA0_MASK,
	.ref_and_mask_sdma1 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__SDMA1_MASK,
};

static uint32_t navi32_nbio_get_hdp_flush_req_offset(struct amdgv_adapter *adapt)
{
	return SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_GPU_HDP_FLUSH_REQ);
}

static uint32_t navi32_nbio_get_hdp_flush_done_offset(struct amdgv_adapter *adapt)
{
	return SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_GPU_HDP_FLUSH_DONE);
}

void navi32_hdp_flush(struct amdgv_adapter *adapt)
{
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_HDP_MEM_COHERENCY_FLUSH_CNTL), 0x0);
}

const struct amdgv_nbio_funcs navi32_nbio_funcs = {
	.get_hdp_flush_req_offset = navi32_nbio_get_hdp_flush_req_offset,
	.get_hdp_flush_done_offset = navi32_nbio_get_hdp_flush_done_offset,
	.hdp_flush = navi32_hdp_flush,
};

const struct amdgv_nbio_ras navi32_nbio_ras = {
	.handle_ras_controller_intr_no_bifring =
		navi32_nbio_handle_ras_controller_intr_no_bifring,
	.handle_ras_err_event_athub_intr_no_bifring =
		navi32_nbio_handle_ras_err_event_athub_intr_no_bifring,
	.set_ras_err_event_athub_irq_state =
		navi32_nbio_set_ras_err_event_athub_irq_state,
};

void navi32_nbio_set_ras_funcs(struct amdgv_adapter *adapt)
{
	adapt->nbio.ras = &navi32_nbio_ras;
	adapt->nbio.hdp_flush_reg = &navi32_nbio_hdp_flush_reg;
	adapt->nbio.funcs = &navi32_nbio_funcs;
}

void navi32_nbio_get_vram_vendor(struct amdgv_adapter *adapt)
{
	uint32_t scratch, scratch_vendor_id_mask;
	scratch_vendor_id_mask = 0xF;
	scratch = RREG32_SOC15(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_4);
	adapt->vram_info.vram_vendor = scratch & scratch_vendor_id_mask;
}

uint32_t navi32_nbio_get_total_vram_size(struct amdgv_adapter *adapt)
{
	return RREG32_SOC15(NBIO, 0, regRCC_DEV0_EPF0_RCC_CONFIG_MEMSIZE);
}
