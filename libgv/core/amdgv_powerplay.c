/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;

int amdgv_powerplay_mode2_reset(struct amdgv_adapter *adapt)
{
	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->gpu_mode2_reset) {
		return adapt->pp.pp_funcs->gpu_mode2_reset(adapt);
	} else {
		AMDGV_ERROR("Mode2 reset callback not implemented!\n");
		return AMDGV_FAILURE;
	}
}

int amdgv_powerplay_mode0_reset(struct amdgv_adapter *adapt)
{
	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->gpu_mode0_reset) {
		return adapt->pp.pp_funcs->gpu_mode0_reset(adapt);
	} else {
		AMDGV_ERROR("Mode0 reset callback not implemented!\n");
		return AMDGV_FAILURE;
	}
}

int amdgv_powerplay_flr_reset(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->trigger_vf_flr) {
		return adapt->pp.pp_funcs->trigger_vf_flr(adapt, idx_vf);
	} else {
		AMDGV_ERROR("VF FLR reset callback not implemented!\n");
		return AMDGV_FAILURE;
	}
}
