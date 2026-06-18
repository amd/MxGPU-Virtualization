/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_MMHUB_V1_8_H
#define AMDGV_MMHUB_V1_8_H

void mmhub_v1_8_gart_enable(struct amdgv_adapter *adapt);
void mmhub_v1_8_init(struct amdgv_adapter *adapt);
void mmhub_v1_8_gart_fini(struct amdgv_adapter *adapt);
void mmhub_v1_8_enable_xgmi(struct amdgv_adapter *adapt);
void mmhub_v1_8_set_ras_funcs(struct amdgv_adapter *adapt);
void mmhub_v1_8_dirtybit_control(struct amdgv_adapter *adapt, bool enable);

#endif
