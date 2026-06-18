/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include "amdgv_gfxhub.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

int amdgv_gfxhub_gart_enable(struct amdgv_adapter *adapt)
{
	if (adapt->gfxhub.funcs && adapt->gfxhub.funcs->gart_enable)
		return adapt->gfxhub.funcs->gart_enable(adapt);
	else
		AMDGV_ERROR("Unable to enable GART\n");

	return AMDGV_FAILURE;
}

int amdgv_gfxhub_gart_disable(struct amdgv_adapter *adapt)
{
	if (adapt->gfxhub.funcs && adapt->gfxhub.funcs->gart_disable)
		return adapt->gfxhub.funcs->gart_disable(adapt);
	else
		AMDGV_ERROR("Unable to disable GART\n");

	return AMDGV_FAILURE;
}
