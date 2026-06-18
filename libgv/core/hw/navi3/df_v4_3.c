/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_ras.h>

#include "navi32_reg_inc.h"
#include "df_v4_3.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

static bool df_v4_3_query_ras_poison_mode(struct amdgv_adapter *adapt)
{
	uint32_t hw_assert_msklo, hw_assert_mskhi;
	uint32_t v0, v1, v28, v31;

	hw_assert_msklo = RREG32_SOC15(DF, 0,
				regDF_CS_UMC_AON0_HardwareAssertMaskLow);
	hw_assert_mskhi = RREG32_SOC15(DF, 0,
				regDF_NCS_PG0_HardwareAssertMaskHigh);

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

const struct amdgv_df_funcs df_v4_3_funcs = {
	.query_ras_poison_mode = df_v4_3_query_ras_poison_mode,
};

static int df_v4_3_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int df_v4_3_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int df_v4_3_sw_init(struct amdgv_adapter *adapt)
{
	adapt->df.funcs = &df_v4_3_funcs;

	return 0;
}

static int df_v4_3_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func navi32_df_v4_3_func = {
	.name = "navi32_df_func",
	.sw_init = df_v4_3_sw_init,
	.sw_fini = df_v4_3_sw_fini,
	.hw_init = df_v4_3_hw_init,
	.hw_fini = df_v4_3_hw_fini,
};
