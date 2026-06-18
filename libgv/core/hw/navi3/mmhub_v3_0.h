/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_MMHUB_V3_0_H
#define AMDGV_MMHUB_V3_0_H

void mmhub_v3_0_gart_enable(struct amdgv_adapter *adapt);
void mmhub_v3_0_init(struct amdgv_adapter *adapt);
void mmhub_v3_0_gart_fini(struct amdgv_adapter *adapt);

#endif
