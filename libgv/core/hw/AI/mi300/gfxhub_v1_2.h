/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_GFXHUB_V1_2_H
#define AMDGV_GFXHUB_V1_2_H

void gfxhub_v1_2_gart_enable(struct amdgv_adapter *adapt);
void gfxhub_v1_2_gart_fini(struct amdgv_adapter *adapt);
void gfxhub_v1_2_vmhub_hook(struct amdgv_adapter *adapt);
void gfxhub_v1_2_enable_system_context(struct amdgv_adapter *adapt);
uint64_t gfxhub_v1_2_get_mc_fb_offset(struct amdgv_adapter *adapt);
void gfxhub_v1_2_enable_xgmi(struct amdgv_adapter *adapt);
void gfxhub_v1_2_init(struct amdgv_adapter *adapt);
#endif
