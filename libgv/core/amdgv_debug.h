/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_DEBUG_H
#define AMDGV_DEBUG_H

#define is_debug_mode_default()			(adapt->debug.mode & AMDGV_DEBUG_MODE_DEFAULT)
#define is_debug_mode_vf_flr_hang()		(adapt->debug.mode & AMDGV_DEBUG_MODE_VF_FLR_HANG)
#define is_debug_mode_whole_gpu_reset_hang()	(adapt->debug.mode & AMDGV_DEBUG_MODE_WHOLE_GPU_RESET_HANG)
#define is_debug_mode_multi_vf()		(adapt->debug.mode & AMDGV_DEBUG_MODE_MULTI_VF)
#define is_debug_mode_ras_smu()			(adapt->debug.mode & AMDGV_DEBUG_MODE_RAS_SMU)
#define is_debug_mode_conditional_hang()	(adapt->debug.mode & AMDGV_DEBUG_MODE_CONDITIONAL_HANG)
#define is_debug_mode_hang()			(adapt->debug.mode & AMDGV_DEBUG_MODE_HANG)
#define is_debug_mode_hang_ras_smu()		(adapt->debug.mode & AMDGV_DEBUG_MODE_HANG_RAS_SMU)


struct amdgv_debug_cond_param {
	uint32_t wgr_count;
	uint32_t flr_count;
	uint64_t last_flr_ts;
	uint64_t last_wgr_ts;
};


/* TODO: Merge "Break Point" feature with regular debug mode features. */
struct amdgv_debug {
	uint32_t mode;
	struct amdgv_debug_cond_param cond;
	bool in_live_debugging;
};

int amdgv_debug_test_and_hang_flr(struct amdgv_adapter *adapt);
int amdgv_debug_test_and_hang_wgr(struct amdgv_adapter *adapt);
void amdgv_debug_set_mode(struct amdgv_adapter *adapt, enum amdgv_debug_mode mode);

#endif