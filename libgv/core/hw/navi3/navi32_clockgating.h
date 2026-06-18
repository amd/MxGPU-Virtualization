/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI32_CLOCKGATING_H
#define NAVI32_CLOCKGATING_H

int navi32_gc_control_power_features(struct amdgv_adapter *adapt, bool enable);

int navi32_clockgating_sw_init(struct amdgv_adapter *adapt);

int navi32_clockgating_hw_init(struct amdgv_adapter *adapt);

int navi32_clockgating_sw_fini(struct amdgv_adapter *adapt);

int navi32_clockgating_hw_fini(struct amdgv_adapter *adapt);

#endif
