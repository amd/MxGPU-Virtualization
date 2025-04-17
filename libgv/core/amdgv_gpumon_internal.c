/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amdgv_gpumon.h"
#include "amdgv_gpumon_internal.h"

enum amdgv_gpumon_type gpumon_unrecov_err_whitelist[] = {
	GPUMON_CPER_GET_ENTRIES,
	GPUMON_CPER_GET_COUNT,
};

uint32_t gpumon_unrecov_err_whitelist_len = ARRAY_SIZE(gpumon_unrecov_err_whitelist);

int amdgv_set_accelerator_partition_profile(struct amdgv_adapter *adapt,
	    uint32_t profile_index)
{
	int ret;
	int event_ret = 0;

	union amdgv_sched_event_data data;
	data.gpumon_data.ap.accelerator_partition_profile_index = profile_index;
	data.gpumon_data.type = GPUMON_SET_ACCELERATOR_PARTITION_PROFILE;
	data.gpumon_data.result = &event_ret;
	if (!(adapt->gpumon.funcs)) {
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	if (!(adapt->gpumon.funcs->set_accelerator_partition_profile)) {
		return AMDGV_ERROR_GPUMON_INVALID_MODE;
	}

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						AMDGV_EVENT_SCHED_GPUMON,
						AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_set_memory_partition_mode(struct amdgv_adapter *adapt,
	    enum amdgv_memory_partition_mode memory_partition_mode)
{
	int ret;
	int event_ret = 0;
	union amdgv_sched_event_data data;

	data.gpumon_data.mp.memory_partition_mode = memory_partition_mode;
	data.gpumon_data.type = GPUMON_SET_MEMORY_PARTITION_MODE;
	data.gpumon_data.result = &event_ret;
	if (!(adapt->gpumon.funcs &&
	      adapt->gpumon.funcs->set_memory_partition_mode)) {
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						AMDGV_EVENT_SCHED_GPUMON,
						AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}
