/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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