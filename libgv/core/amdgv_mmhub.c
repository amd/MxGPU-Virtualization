/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include "amdgv_mmhub.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;


uint64_t amdgv_mmhub_get_mc_fb_offset(struct amdgv_adapter *adapt)
{
	if (adapt->mmhub.funcs && adapt->mmhub.funcs->get_mc_fb_offset)
		return adapt->mmhub.funcs->get_mc_fb_offset(adapt);
	else
		AMDGV_ERROR("Unable to get MC FB offset\n");

	return 0;
}

int amdgv_mmhub_get_fb_location(struct amdgv_adapter *adapt, uint64_t *base, uint64_t *top)
{
	if (adapt->mmhub.funcs && adapt->mmhub.funcs->get_fb_location)
		return adapt->mmhub.funcs->get_fb_location(adapt, base, top);
	else
		AMDGV_ERROR("Unable to get FB location\n");

	return AMDGV_FAILURE;
}

int amdgv_mmhub_gart_enable(struct amdgv_adapter *adapt)
{
	if (adapt->mmhub.funcs && adapt->mmhub.funcs->gart_enable)
		return adapt->mmhub.funcs->gart_enable(adapt);

	return AMDGV_FAILURE;
}

int amdgv_mmhub_gart_disable(struct amdgv_adapter *adapt)
{
	if (adapt->mmhub.funcs && adapt->mmhub.funcs->gart_disable)
		return adapt->mmhub.funcs->gart_disable(adapt);

	return AMDGV_FAILURE;
}

int amdgv_mmhub_query_ras_error_count(struct amdgv_adapter *adapt, void *err_data)
{
	if (adapt->mmhub.funcs && adapt->mmhub.funcs->query_ras_error_count) {
		adapt->mmhub.funcs->query_ras_error_count(adapt, &err_data);
		return 0;
	} else {
		return AMDGV_FAILURE;
	}
}

int amdgv_mmhub_get_xgmi_info(struct amdgv_adapter *adapt)
{
	if (adapt->mmhub.funcs && adapt->mmhub.funcs->get_xgmi_info)
		return adapt->mmhub.funcs->get_xgmi_info(adapt);
	else
		AMDGV_ERROR("Unable to get XGMI info\n");

	return AMDGV_FAILURE;
}
