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

#include "amdgv_device.h"
#include "amdgv_debug.h"

static const uint32_t this_block = AMDGV_API_BLOCK;

static void amdgv_debug_enter_hang(struct amdgv_adapter *adapt)
{
	AMDGV_WARN("Entered debug hang. Disable debug hang or unload driver to escape.\n");

	while (is_debug_mode_hang())
		oss_msleep(5000);

	AMDGV_INFO("Releasing debug hang.\n");
}

static int amdgv_debug_hang_flr(struct amdgv_adapter *adapt)
{
	amdgv_debug_enter_hang(adapt);

	return 0;
}

static int amdgv_debug_hang_wgr(struct amdgv_adapter *adapt)
{
	amdgv_debug_enter_hang(adapt);

	return 0;
}

static int amdgv_debug_cond_hang_flr(struct amdgv_adapter *adapt)
{
	uint64_t curr_time = oss_get_time_stamp();

	if ((curr_time - adapt->debug.cond.last_flr_ts) > AMDGV_RESET_DEFAULT_RESET_INTERVAL) {
		adapt->debug.cond.last_flr_ts = curr_time;
		adapt->debug.cond.flr_count++;
		return 0;
	}

	amdgv_debug_hang_flr(adapt);

	return 0;
}

static int amdgv_debug_cond_hang_wgr(struct amdgv_adapter *adapt)
{
	uint64_t curr_time = oss_get_time_stamp();

	if ((curr_time - adapt->debug.cond.last_wgr_ts) > AMDGV_RESET_DEFAULT_RESET_INTERVAL) {
		adapt->debug.cond.last_wgr_ts = curr_time;
		adapt->debug.cond.wgr_count++;
		return 0;
	}

	amdgv_debug_hang_wgr(adapt);

	return 0;
}

int amdgv_debug_test_and_hang_flr(struct amdgv_adapter *adapt)
{
	if (!is_debug_mode_vf_flr_hang())
		return 0;

	if (is_debug_mode_conditional_hang())
		amdgv_debug_cond_hang_flr(adapt);
	else
		amdgv_debug_hang_flr(adapt);

	return 0;
}

int amdgv_debug_test_and_hang_wgr(struct amdgv_adapter *adapt)
{
	if (!is_debug_mode_whole_gpu_reset_hang())
		return 0;

	if (is_debug_mode_conditional_hang())
		amdgv_debug_cond_hang_wgr(adapt);
	else
		amdgv_debug_hang_wgr(adapt);

	return 0;
}

void amdgv_debug_set_mode(struct amdgv_adapter *adapt, enum amdgv_debug_mode mode)
{
	adapt->debug.mode = mode;
	AMDGV_INFO("LibGV Debug mode set to 0x%x\n", adapt->debug.mode);
}
