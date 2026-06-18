/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI32_RESET_H
#define NAVI32_RESET_H

/* do whole gpu reset without context save&restore */
int navi32_reset_whole_gpu_reset(struct amdgv_adapter *adapt, uint32_t mode);
int navi32_reset_enter_power_saving(struct amdgv_adapter *adapt);
int navi32_reset_exit_power_saving(struct amdgv_adapter *adapt);
int navi32_reset_grbm_soft_reset_stage_1(struct amdgv_adapter *adapt, bool gl2c_bit);
int navi32_reset_grbm_soft_reset_stage_2(struct amdgv_adapter *adapt);

#endif
