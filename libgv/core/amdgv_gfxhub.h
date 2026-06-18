/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AMDGV_GFXHUB_H__
#define __AMDGV_GFXHUB_H__


struct amdgv_gfxhub_funcs {
	int (*gart_enable)(struct amdgv_adapter *adapt);
	int (*gart_disable)(struct amdgv_adapter *adapt);
};

struct amdgv_gfxhub {
	const struct amdgv_gfxhub_funcs *funcs;
};

int amdgv_gfxhub_gart_enable(struct amdgv_adapter *adapt);
int amdgv_gfxhub_gart_disable(struct amdgv_adapter *adapt);
#endif
