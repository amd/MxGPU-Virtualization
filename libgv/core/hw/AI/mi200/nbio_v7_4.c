/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "mi200/NBIO/nbio_7_4_offset.h"
#include "mi200/NBIO/nbio_7_4_sh_mask.h"
#include "mi200/NBIO/nbio_7_4_0_smn.h"
#include "amdgv_device.h"
#include "amdgv_ras.h"
#include "amdgv_nbio.h"
#include "nbio_v7_4.h"

#define smnPCIE_LC_CNTL		0x11140280
#define smnPCIE_LC_CNTL3	0x111402d4
#define smnPCIE_LC_CNTL6	0x111402ec
#define smnPCIE_LC_CNTL7	0x111402f0
#define smnNBIF_MGCG_CTRL_LCLK	0x1013a21c
#define smnRCC_BIF_STRAP3	0x1012348c
#define RCC_BIF_STRAP3__STRAP_VLINK_ASPM_IDLE_TIMER_MASK	0x0000FFFFL
#define RCC_BIF_STRAP3__STRAP_VLINK_PM_L1_ENTRY_TIMER_MASK	0xFFFF0000L
#define smnRCC_BIF_STRAP5	0x10123494
#define RCC_BIF_STRAP5__STRAP_VLINK_LDN_ENTRY_TIMER_MASK	0x0000FFFFL
#define smnBIF_CFG_DEV0_EPF0_DEVICE_CNTL2	0x1014008c
#define BIF_CFG_DEV0_EPF0_DEVICE_CNTL2__LTR_EN_MASK			0x0400L
#define smnBIF_CFG_DEV0_EPF0_PCIE_LTR_CAP	0x10140324
#define smnPSWUSP0_PCIE_LC_CNTL2		0x111402c4
#define smnRCC_EP_DEV0_0_EP_PCIE_TX_LTR_CNTL	0x10123538
#define smnRCC_BIF_STRAP2	0x10123488
#define RCC_BIF_STRAP2__STRAP_LTR_IN_ASPML1_DIS_MASK	0x00004000L
#define RCC_BIF_STRAP3__STRAP_VLINK_ASPM_IDLE_TIMER__SHIFT	0x0
#define RCC_BIF_STRAP3__STRAP_VLINK_PM_L1_ENTRY_TIMER__SHIFT	0x10
#define RCC_BIF_STRAP5__STRAP_VLINK_LDN_ENTRY_TIMER__SHIFT	0x0

/*
 * These are nbio v7_4_1 registers mask. Temporarily define these here since
 * nbio v7_4_1 header is incomplete.
 */
#define GPU_HDP_FLUSH_DONE__RSVD_ENG0_MASK	0x00001000L /* Don't use.  Firmware uses this bit internally */
#define GPU_HDP_FLUSH_DONE__RSVD_ENG1_MASK	0x00002000L
#define GPU_HDP_FLUSH_DONE__RSVD_ENG2_MASK	0x00004000L
#define GPU_HDP_FLUSH_DONE__RSVD_ENG3_MASK	0x00008000L
#define GPU_HDP_FLUSH_DONE__RSVD_ENG4_MASK	0x00010000L
#define GPU_HDP_FLUSH_DONE__RSVD_ENG5_MASK	0x00020000L
#define GPU_HDP_FLUSH_DONE__RSVD_ENG6_MASK	0x00040000L
#define GPU_HDP_FLUSH_DONE__RSVD_ENG7_MASK	0x00080000L
#define GPU_HDP_FLUSH_DONE__RSVD_ENG8_MASK	0x00100000L

#define mmBIF_MMSCH1_DOORBELL_RANGE                     0x01dc
#define mmBIF_MMSCH1_DOORBELL_RANGE_BASE_IDX            2
//BIF_MMSCH1_DOORBELL_RANGE
#define BIF_MMSCH1_DOORBELL_RANGE__OFFSET__SHIFT        0x2
#define BIF_MMSCH1_DOORBELL_RANGE__SIZE__SHIFT          0x10
#define BIF_MMSCH1_DOORBELL_RANGE__OFFSET_MASK          0x00000FFCL
#define BIF_MMSCH1_DOORBELL_RANGE__SIZE_MASK            0x001F0000L

#define BIF_MMSCH1_DOORBELL_RANGE__OFFSET_MASK          0x00000FFCL
#define BIF_MMSCH1_DOORBELL_RANGE__SIZE_MASK            0x001F0000L

#define mmBIF_MMSCH1_DOORBELL_RANGE_ALDE                0x01d8
#define mmBIF_MMSCH1_DOORBELL_RANGE_ALDE_BASE_IDX       2
//BIF_MMSCH1_DOORBELL_ALDE_RANGE
#define BIF_MMSCH1_DOORBELL_RANGE_ALDE__OFFSET__SHIFT   0x2
#define BIF_MMSCH1_DOORBELL_RANGE_ALDE__SIZE__SHIFT     0x10
#define BIF_MMSCH1_DOORBELL_RANGE_ALDE__OFFSET_MASK     0x00000FFCL
#define BIF_MMSCH1_DOORBELL_RANGE_ALDE__SIZE_MASK       0x001F0000L

#define mmRCC_DEV0_EPF0_STRAP0_ALDE			0x0015
#define mmRCC_DEV0_EPF0_STRAP0_ALDE_BASE_IDX		2

#define mmBIF_DOORBELL_INT_CNTL_ALDE 			0x00fe
#define mmBIF_DOORBELL_INT_CNTL_ALDE_BASE_IDX 		2
#define BIF_DOORBELL_INT_CNTL_ALDE__DOORBELL_INTERRUPT_DISABLE__SHIFT	0x18
#define BIF_DOORBELL_INT_CNTL_ALDE__DOORBELL_INTERRUPT_DISABLE_MASK	0x01000000L

#define mmBIF_INTR_CNTL_ALDE 				0x0101
#define mmBIF_INTR_CNTL_ALDE_BASE_IDX 			2

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static void nbio_v7_4_handle_ras_controller_intr_no_bifring(struct amdgv_adapter *adapt)
{
	uint32_t bif_doorbell_intr_cntl = 0;

	if (adapt->asic_type == CHIP_MI200)
		bif_doorbell_intr_cntl = RREG32_SOC15(NBIO, 0, mmBIF_DOORBELL_INT_CNTL_ALDE);
	else
		bif_doorbell_intr_cntl = RREG32_SOC15(NBIO, 0, mmBIF_DOORBELL_INT_CNTL);

	AMDGV_DEBUG("bif_doorbell_int_cntl 0x%x, ras_cntlr_interrupt_status %d\n",
			bif_doorbell_intr_cntl,
			REG_GET_FIELD(bif_doorbell_intr_cntl, BIF_DOORBELL_INT_CNTL, RAS_CNTLR_INTERRUPT_STATUS));

	if (REG_GET_FIELD(bif_doorbell_intr_cntl,
		BIF_DOORBELL_INT_CNTL, RAS_CNTLR_INTERRUPT_STATUS)) {
		/* driver has to clear the interrupt status when bif ring is disabled */
		bif_doorbell_intr_cntl = REG_SET_FIELD(bif_doorbell_intr_cntl,
						BIF_DOORBELL_INT_CNTL,
						RAS_CNTLR_INTERRUPT_CLEAR, 1);
		if (adapt->asic_type == CHIP_MI200)
			WREG32_SOC15(NBIO, 0, mmBIF_DOORBELL_INT_CNTL_ALDE, bif_doorbell_intr_cntl);
		else
			WREG32_SOC15(NBIO, 0, mmBIF_DOORBELL_INT_CNTL, bif_doorbell_intr_cntl);

		/* TODO: handle ras controller interrupt */
	}
}

static void nbio_v7_4_handle_ras_err_event_athub_intr_no_bifring(struct amdgv_adapter *adapt)
{
	uint32_t bif_doorbell_intr_cntl = 0;

	if (adapt->asic_type == CHIP_MI200)
		bif_doorbell_intr_cntl = RREG32_SOC15(NBIO, 0, mmBIF_DOORBELL_INT_CNTL_ALDE);
	else
		bif_doorbell_intr_cntl = RREG32_SOC15(NBIO, 0, mmBIF_DOORBELL_INT_CNTL);

	AMDGV_DEBUG("bif_doorbell_int_cntl 0x%x, ras_athub_err_event_interrupt_status %d\n",
			bif_doorbell_intr_cntl,
			REG_GET_FIELD(bif_doorbell_intr_cntl, BIF_DOORBELL_INT_CNTL, RAS_ATHUB_ERR_EVENT_INTERRUPT_STATUS));

	if (REG_GET_FIELD(bif_doorbell_intr_cntl,
		BIF_DOORBELL_INT_CNTL, RAS_ATHUB_ERR_EVENT_INTERRUPT_STATUS)) {
		/* driver has to clear the interrupt status when bif ring is disabled */
		bif_doorbell_intr_cntl = REG_SET_FIELD(bif_doorbell_intr_cntl,
						BIF_DOORBELL_INT_CNTL,
						RAS_ATHUB_ERR_EVENT_INTERRUPT_CLEAR, 1);

		if (adapt->asic_type == CHIP_MI200)
			WREG32_SOC15(NBIO, 0, mmBIF_DOORBELL_INT_CNTL_ALDE, bif_doorbell_intr_cntl);
		else
			WREG32_SOC15(NBIO, 0, mmBIF_DOORBELL_INT_CNTL, bif_doorbell_intr_cntl);

		/* handle poison consumption check */
		amdgv_ecc_get_poison_stat(adapt);

		/* handle global_ras_isr */
		amdgv_ecc_check_global_ras_errors(adapt);
	}
}

static int nbio_v7_4_set_ras_controller_irq_state(struct amdgv_adapter *adapt,
							bool state)
{
	/* The ras_controller_irq enablement should be done in psp bl when it
	 * tries to enable ras feature. Driver only need to set the correct interrupt
	 * vector for bare-metal and sriov use case respectively
	 */
	uint32_t bif_intr_cntl;

	if (adapt->asic_type == CHIP_MI200)
		bif_intr_cntl = RREG32_SOC15(NBIO, 0, mmBIF_INTR_CNTL_ALDE);
	else
		bif_intr_cntl = RREG32_SOC15(NBIO, 0, mmBIF_INTR_CNTL);

	if (state == true) {
		/* set interrupt vector select bit to 1 to select
		 * vetcor 4 for bare metal case */
		bif_intr_cntl = REG_SET_FIELD(bif_intr_cntl,
					      BIF_INTR_CNTL,
					      RAS_INTR_VEC_SEL, 1);

		if (adapt->asic_type == CHIP_MI200)
			WREG32_SOC15(NBIO, 0, mmBIF_INTR_CNTL_ALDE, bif_intr_cntl);
		else
			WREG32_SOC15(NBIO, 0, mmBIF_INTR_CNTL, bif_intr_cntl);

	}

	return 0;
}

static int nbio_v7_4_set_ras_err_event_athub_irq_state(struct amdgv_adapter *adapt,
							bool state)
{
	/* The ras_controller_irq enablement should be done in psp bl when it
	 * tries to enable ras feature. Driver only need to set the correct interrupt
	 * vector for bare-metal and sriov use case respectively
	 */
	uint32_t bif_intr_cntl;

	if (adapt->asic_type == CHIP_MI200)
		bif_intr_cntl = RREG32_SOC15(NBIO, 0, mmBIF_INTR_CNTL_ALDE);
	else
		bif_intr_cntl = RREG32_SOC15(NBIO, 0, mmBIF_INTR_CNTL);

	if (state == true) {
		/* set interrupt vector select bit to 1 to select
		 * vetcor 4 for sriov case */
		bif_intr_cntl = REG_SET_FIELD(bif_intr_cntl,
					      BIF_INTR_CNTL,
					      RAS_INTR_VEC_SEL, 1);

		if (adapt->asic_type == CHIP_MI200)
			WREG32_SOC15(NBIO, 0, mmBIF_INTR_CNTL_ALDE, bif_intr_cntl);
		else
			WREG32_SOC15(NBIO, 0, mmBIF_INTR_CNTL, bif_intr_cntl);
	}

	return 0;
}

#define smnPARITY_ERROR_STATUS_UNCORR_GRP2_ALDE	0x13b20030
#define smnRAS_GLOBAL_STATUS_LO_ALDE            0x13b20020

static void nbio_v7_4_query_ras_error_count(struct amdgv_adapter *adapt,
					void *ras_error_status)
{
	uint32_t global_sts, central_sts, int_eoi, parity_sts;
	uint32_t corr, fatal, non_fatal;
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;

	global_sts = RREG32_PCIE(smnRAS_GLOBAL_STATUS_LO_ALDE);

	corr = REG_GET_FIELD(global_sts, RAS_GLOBAL_STATUS_LO, ParityErrCorr);
	fatal = REG_GET_FIELD(global_sts, RAS_GLOBAL_STATUS_LO, ParityErrFatal);
	non_fatal = REG_GET_FIELD(global_sts, RAS_GLOBAL_STATUS_LO,
				ParityErrNonFatal);

	parity_sts = RREG32_PCIE(smnPARITY_ERROR_STATUS_UNCORR_GRP2_ALDE);

	if (corr)
		err_data->ce_count++;
	if (fatal)
		err_data->ue_count++;

	if (corr || fatal || non_fatal) {
		central_sts = RREG32_PCIE(smnBIFL_RAS_CENTRAL_STATUS);

		/* clear error status register */
		WREG32_PCIE(smnRAS_GLOBAL_STATUS_LO_ALDE, global_sts);

		if (fatal) {
			/* clear parity fatal error indication field */
			WREG32_PCIE(smnPARITY_ERROR_STATUS_UNCORR_GRP2_ALDE, parity_sts);
		}

		if (REG_GET_FIELD(central_sts, BIFL_RAS_CENTRAL_STATUS,
				BIFL_RasContller_Intr_Recv)) {
			/* clear interrupt status register */
			WREG32_PCIE(smnBIFL_RAS_CENTRAL_STATUS, central_sts);
			int_eoi = RREG32_PCIE(smnIOHC_INTERRUPT_EOI);
			int_eoi = REG_SET_FIELD(int_eoi,
					IOHC_INTERRUPT_EOI, SMI_EOI, 1);
			WREG32_PCIE(smnIOHC_INTERRUPT_EOI, int_eoi);
		}
	}
}

const struct amdgv_nbio_ras nbio_v7_4_ras = {
	.handle_ras_controller_intr_no_bifring = nbio_v7_4_handle_ras_controller_intr_no_bifring,
	.handle_ras_err_event_athub_intr_no_bifring = nbio_v7_4_handle_ras_err_event_athub_intr_no_bifring,
	.set_ras_controller_irq_state = nbio_v7_4_set_ras_controller_irq_state,
	.set_ras_err_event_athub_irq_state = nbio_v7_4_set_ras_err_event_athub_irq_state,
	.query_ras_error_count = nbio_v7_4_query_ras_error_count,
};

const struct nbio_hdp_flush_reg nbio_v7_4_hdp_flush_reg = {
	.ref_and_mask_cp0 = GPU_HDP_FLUSH_DONE__CP0_MASK,
	.ref_and_mask_cp1 = GPU_HDP_FLUSH_DONE__CP1_MASK,
	.ref_and_mask_cp2 = GPU_HDP_FLUSH_DONE__CP2_MASK,
	.ref_and_mask_cp3 = GPU_HDP_FLUSH_DONE__CP3_MASK,
	.ref_and_mask_cp4 = GPU_HDP_FLUSH_DONE__CP4_MASK,
	.ref_and_mask_cp5 = GPU_HDP_FLUSH_DONE__CP5_MASK,
	.ref_and_mask_cp6 = GPU_HDP_FLUSH_DONE__CP6_MASK,
	.ref_and_mask_cp7 = GPU_HDP_FLUSH_DONE__CP7_MASK,
	.ref_and_mask_cp8 = GPU_HDP_FLUSH_DONE__CP8_MASK,
	.ref_and_mask_cp9 = GPU_HDP_FLUSH_DONE__CP9_MASK,
	.ref_and_mask_sdma0 = GPU_HDP_FLUSH_DONE__RSVD_ENG1_MASK,
	.ref_and_mask_sdma1 = GPU_HDP_FLUSH_DONE__RSVD_ENG2_MASK,
	.ref_and_mask_sdma2 = GPU_HDP_FLUSH_DONE__RSVD_ENG3_MASK,
	.ref_and_mask_sdma3 = GPU_HDP_FLUSH_DONE__RSVD_ENG4_MASK,
	.ref_and_mask_sdma4 = GPU_HDP_FLUSH_DONE__RSVD_ENG5_MASK,
	.ref_and_mask_sdma5 = GPU_HDP_FLUSH_DONE__RSVD_ENG6_MASK,
	.ref_and_mask_sdma6 = GPU_HDP_FLUSH_DONE__RSVD_ENG7_MASK,
	.ref_and_mask_sdma7 = GPU_HDP_FLUSH_DONE__RSVD_ENG8_MASK,
};

static uint32_t nbio_v7_4_get_hdp_flush_req_offset(struct amdgv_adapter *adapt)
{
	return SOC15_REG_OFFSET(NBIO, 0, mmGPU_HDP_FLUSH_REQ);
}

static uint32_t nbio_v7_4_get_hdp_flush_done_offset(struct amdgv_adapter *adapt)
{
	return SOC15_REG_OFFSET(NBIO, 0, mmGPU_HDP_FLUSH_DONE);
}

const struct amdgv_nbio_funcs nbio_v7_4_funcs = {
	.get_hdp_flush_req_offset = nbio_v7_4_get_hdp_flush_req_offset,
	.get_hdp_flush_done_offset = nbio_v7_4_get_hdp_flush_done_offset,
};

void nbio_v7_4_set_ras_funcs(struct amdgv_adapter *adapt)
{
	adapt->nbio.ras = &nbio_v7_4_ras;
	adapt->nbio.hdp_flush_reg = &nbio_v7_4_hdp_flush_reg;
	adapt->nbio.funcs = &nbio_v7_4_funcs;
}
