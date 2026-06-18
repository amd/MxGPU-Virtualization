/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_df.h"
#include "mi200/DF/df_3_6_offset.h"
#include "mi200/DF/df_3_6_sh_mask.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

static bool df_v3_6_query_ras_poison_mode(struct amdgv_adapter *adapt)
{
	uint32_t hw_assert_msklo, hw_assert_mskhi;
	uint32_t v0, v1, v28, v31;

	hw_assert_msklo = RREG32_SOC15(DF, 0,
				mmDF_CS_UMC_AON0_HardwareAssertMaskLow);
	hw_assert_mskhi = RREG32_SOC15(DF, 0,
				mmDF_NCS_PG0_HardwareAssertMaskHigh);

	v0 = REG_GET_FIELD(hw_assert_msklo,
		DF_CS_UMC_AON0_HardwareAssertMaskLow, HWAssertMsk0);
	v1 = REG_GET_FIELD(hw_assert_msklo,
		DF_CS_UMC_AON0_HardwareAssertMaskLow, HWAssertMsk1);
	v28 = REG_GET_FIELD(hw_assert_mskhi,
		DF_NCS_PG0_HardwareAssertMaskHigh, HWAssertMsk28);
	v31 = REG_GET_FIELD(hw_assert_mskhi,
		DF_NCS_PG0_HardwareAssertMaskHigh, HWAssertMsk31);

	if (v0 && v1 && v28 && v31)
		return true;
	else if (!v0 && !v1 && !v28 && !v31)
		return false;
	else {
		AMDGV_WARN("DF poison setting is inconsistent(%d:%d:%d:%d)!\n",
				v0, v1, v28, v31);
		return false;
	}
}

const struct amdgv_df_funcs df_v3_6_funcs = {
	.query_ras_poison_mode = df_v3_6_query_ras_poison_mode,
};

static int df_v3_6_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int df_v3_6_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int df_v3_6_sw_init(struct amdgv_adapter *adapt)
{
	adapt->df.funcs = &df_v3_6_funcs;

	return 0;
}

static int df_v3_6_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_df_v3_6_func = {
	.name = "mi200_df_func",
	.sw_init = df_v3_6_sw_init,
	.sw_fini = df_v3_6_sw_fini,
	.hw_init = df_v3_6_hw_init,
	.hw_fini = df_v3_6_hw_fini,
};
