/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI32_GFX_H
#define NAVI32_GFX_H

uint32_t navi32_gfx_cu_count(struct amdgv_adapter *adapt);
uint32_t navi32_gfx_atc_ats_invalidate(struct amdgv_adapter *adapt);
void navi32_gfx_program_golden_settings(struct amdgv_adapter *adapt);
void navi32_gfx_halt_gpu_state(struct amdgv_adapter *adapt);
void navi32_gfx_unhalt_gpu_state(struct amdgv_adapter *adapt);
int navi32_gfx_check_rlc_autoload_complete(struct amdgv_adapter *adapt);

#endif
