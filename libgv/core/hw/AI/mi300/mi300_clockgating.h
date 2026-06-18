/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_CLOCKGATING_H
#define MI300_CLOCKGATING_H

int mi300_clockgating_hw_init(struct amdgv_adapter *adapt);
void mi300_clockgating_spm_mgcg(struct amdgv_adapter *adapt, int xcc_id, bool enable);

#endif
