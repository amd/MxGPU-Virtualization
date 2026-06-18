/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <ai.h>

#include "mi200/mi200_ip_offset.h"

void mi200_reg_base_init(struct amdgv_adapter *adapt)
{
	/*
	 * HW has more IP blocks,  only initialized the block needed by our
	 * driver
	 */
	uint32_t i;
	for (i = 0 ; i < MAX_INSTANCE ; ++i) {
		adapt->reg_offset[GC_HWIP][i] = (uint32_t *)(&(GC_BASE.instance[i]));
		adapt->reg_offset[HDP_HWIP][i] = (uint32_t *)(&(HDP_BASE.instance[i]));
		adapt->reg_offset[MMHUB_HWIP][i] = (uint32_t *)(&(MMHUB_BASE.instance[i]));
		adapt->reg_offset[ATHUB_HWIP][i] = (uint32_t *)(&(ATHUB_BASE.instance[i]));
		adapt->reg_offset[NBIO_HWIP][i] = (uint32_t *)(&(NBIO_BASE.instance[i]));
		adapt->reg_offset[NBIF_HWIP][i] = (uint32_t *)(&(NBIO_BASE.instance[i]));
		adapt->reg_offset[MP0_HWIP][i] = (uint32_t *)(&(MP0_BASE.instance[i]));
		adapt->reg_offset[MP1_HWIP][i] = (uint32_t *)(&(MP1_BASE.instance[i]));
		adapt->reg_offset[DF_HWIP][i] = (uint32_t *)(&(DF_BASE.instance[i]));
		adapt->reg_offset[OSSSYS_HWIP][i] = (uint32_t *)(&(OSSSYS_BASE.instance[i]));
		adapt->reg_offset[SDMA0_HWIP][i] = (uint32_t *)(&(SDMA0_BASE.instance[i]));
		adapt->reg_offset[SDMA1_HWIP][i] = (uint32_t *)(&(SDMA1_BASE.instance[i]));
		adapt->reg_offset[SDMA2_HWIP][i] = (uint32_t *)(&(SDMA2_BASE.instance[i]));
		adapt->reg_offset[SDMA3_HWIP][i] = (uint32_t *)(&(SDMA3_BASE.instance[i]));
		adapt->reg_offset[SDMA4_HWIP][i] = (uint32_t *)(&(SDMA4_BASE.instance[i]));
		adapt->reg_offset[SMUIO_HWIP][i] = (uint32_t *)(&(SMUIO_BASE.instance[i]));
		adapt->reg_offset[THM_HWIP][i] = (uint32_t *)(&(THM_BASE.instance[i]));
		adapt->reg_offset[UMC_HWIP][i] = (uint32_t *)(&(UMC_BASE.instance[i]));
		adapt->reg_offset[VCN_HWIP][i] = (uint32_t *)(&(VCN_BASE.instance[i]));
	}
}

static int mi200_doorbell_index_init(struct amdgv_adapter *adapt)
{
	adapt->doorbell_index.kiq = AMDGV_MI200_DOORBELL_KIQ;
	adapt->doorbell_index.mec_ring0 = AMDGV_MI200_DOORBELL_MEC_RING0;
	adapt->doorbell_index.mec_ring1 = AMDGV_MI200_DOORBELL_MEC_RING1;
	adapt->doorbell_index.mec_ring2 = AMDGV_MI200_DOORBELL_MEC_RING2;
	adapt->doorbell_index.mec_ring3 = AMDGV_MI200_DOORBELL_MEC_RING3;
	adapt->doorbell_index.mec_ring4 = AMDGV_MI200_DOORBELL_MEC_RING4;
	adapt->doorbell_index.mec_ring5 = AMDGV_MI200_DOORBELL_MEC_RING5;
	adapt->doorbell_index.mec_ring6 = AMDGV_MI200_DOORBELL_MEC_RING6;
	adapt->doorbell_index.mec_ring7 = AMDGV_MI200_DOORBELL_MEC_RING7;
	adapt->doorbell_index.userqueue_start =
			AMDGV_MI200_DOORBELL_USERQUEUE_START;
	adapt->doorbell_index.userqueue_end =
			AMDGV_MI200_DOORBELL_USERQUEUE_END;
	adapt->doorbell_index.gfx_ring0 = AMDGV_MI200_DOORBELL_GFX_RING0;
	adapt->doorbell_index.sdma_engine[0] =
			AMDGV_MI200_DOORBELL_sDMA_ENGINE0;
	adapt->doorbell_index.sdma_engine[1] =
			AMDGV_MI200_DOORBELL_sDMA_ENGINE1;
	adapt->doorbell_index.sdma_engine[2] =
			AMDGV_MI200_DOORBELL_sDMA_ENGINE2;
	adapt->doorbell_index.sdma_engine[3] =
			AMDGV_MI200_DOORBELL_sDMA_ENGINE3;
	adapt->doorbell_index.sdma_engine[4] =
			AMDGV_MI200_DOORBELL_sDMA_ENGINE4;
	adapt->doorbell_index.sdma_engine[5] =
			AMDGV_MI200_DOORBELL_sDMA_ENGINE5;
	adapt->doorbell_index.sdma_engine[6] =
			AMDGV_MI200_DOORBELL_sDMA_ENGINE6;
	adapt->doorbell_index.sdma_engine[7] =
			AMDGV_MI200_DOORBELL_sDMA_ENGINE7;
	adapt->doorbell_index.ih = AMDGV_MI200_DOORBELL_IH;
	adapt->doorbell_index.uvd_vce.uvd_ring0_1 =
			AMDGV_MI200_DOORBELL64_UVD_RING0_1;
	adapt->doorbell_index.uvd_vce.uvd_ring2_3 =
			AMDGV_MI200_DOORBELL64_UVD_RING2_3;
	adapt->doorbell_index.uvd_vce.uvd_ring4_5 =
			AMDGV_MI200_DOORBELL64_UVD_RING4_5;
	adapt->doorbell_index.uvd_vce.uvd_ring6_7 =
			AMDGV_MI200_DOORBELL64_UVD_RING6_7;
	adapt->doorbell_index.uvd_vce.vce_ring0_1 =
			AMDGV_MI200_DOORBELL64_VCE_RING0_1;
	adapt->doorbell_index.uvd_vce.vce_ring2_3 =
			AMDGV_MI200_DOORBELL64_VCE_RING2_3;
	adapt->doorbell_index.uvd_vce.vce_ring4_5 =
			AMDGV_MI200_DOORBELL64_VCE_RING4_5;
	adapt->doorbell_index.uvd_vce.vce_ring6_7 =
			AMDGV_MI200_DOORBELL64_VCE_RING6_7;
	adapt->doorbell_index.vcn.vcn_ring0_1 = AMDGV_MI200_DOORBELL64_VCN0_1;
	adapt->doorbell_index.vcn.vcn_ring2_3 = AMDGV_MI200_DOORBELL64_VCN2_3;
	adapt->doorbell_index.vcn.vcn_ring4_5 = AMDGV_MI200_DOORBELL64_VCN4_5;
	adapt->doorbell_index.vcn.vcn_ring6_7 = AMDGV_MI200_DOORBELL64_VCN6_7;

	adapt->doorbell_index.first_non_cp =
			AMDGV_MI200_DOORBELL64_FIRST_NON_CP;
	adapt->doorbell_index.last_non_cp =
			AMDGV_MI200_DOORBELL64_LAST_NON_CP;

	adapt->doorbell_index.max_assignment =
			AMDGV_MI200_DOORBELL_MAX_ASSIGNMENT << 1;
	adapt->doorbell_index.sdma_doorbell_range = 20;

	return 0;
}

/* do nothing in these interfaces */
static int mi200_doorbell_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi200_doorbell_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi200_doorbell_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_doorbell_func = {
	.name = "mi200_doorbell_func",
	.sw_init = mi200_doorbell_index_init,
	.sw_fini = mi200_doorbell_sw_fini,
	.hw_init = mi200_doorbell_hw_init,
	.hw_fini = mi200_doorbell_hw_fini,
};
