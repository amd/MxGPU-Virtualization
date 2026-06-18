/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_GFXHUB_V1_0_H
#define AMDGV_GFXHUB_V1_0_H

void gfxhub_v1_0_gart_enable(struct amdgv_adapter *adapt);
void gfxhub_v1_0_gart_fini(struct amdgv_adapter *adapt);
void gfxhub_v1_0_xgmi_init(struct amdgv_adapter *adapt, int index);
void gfxhub_v1_0_vmhub_hook(struct amdgv_adapter *adapt);
void gfxhub_v1_0_enable_system_context(struct amdgv_adapter *adapt);

#endif
