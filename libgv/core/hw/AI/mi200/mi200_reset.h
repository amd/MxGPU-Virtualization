/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI200_RESET_H
#define MI200_RESET_H

int mi200_reset_trigger_whole_gpu_reset(struct amdgv_adapter *adapt);
void mi200_clear_dummy_mode_after_reset(struct amdgv_adapter *adapt);

#endif
