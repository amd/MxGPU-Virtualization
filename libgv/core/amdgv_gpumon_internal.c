/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_gpumon.h"
#include "amdgv_gpumon_internal.h"

#define MIXING_K      31   /* coprime to 255 */
#define MIXING_K_INV 181   /* 31^(-1) mod 255 */

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
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	if (!(adapt->gpumon.funcs->set_accelerator_partition_profile)) {
		return AMDGV_LOG_GPUMON_INVALID_MODE;
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
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						AMDGV_EVENT_SCHED_GPUMON,
						AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_set_cc_mode(struct amdgv_adapter *adapt,
	    enum amdgv_cc_mode cc_mode)
{
	int ret;
	int event_ret = 0;
	union amdgv_sched_event_data data;

	data.gpumon_data.cc_mode = cc_mode;
	data.gpumon_data.type = GPUMON_SET_CC_MODE;
	data.gpumon_data.result = &event_ret;
	if (!(adapt->gpumon.funcs &&
	      adapt->gpumon.funcs->set_cc_mode)) {
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	}

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						AMDGV_EVENT_SCHED_GPUMON,
						AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}


/**
 * amdgv_gpumon_fcn_ref_id_encode - Encode a per-VF function reference ID (unit ID)
 * @serial:   64-bit GPU serial number used as a per-card salt
 * @vf_index: VF index on this card (0 .. num_vf-1)
 *
 * Generates an 8-bit "unit ID" that uniquely identifies a (card, VF) pair in
 * the range [1, 255]:
 *
 *
 * Return: encoded unit ID in the range [1, 255].
 */

uint8_t amdgv_gpumon_fcn_ref_id_encode(uint64_t serial, uint8_t vf_index)
{
	uint8_t h;
	if (vf_index == 0xff)
		return 0;

	h = (uint8_t)((serial ^ (serial >> 32)) % 255);
	return (uint8_t)((vf_index * MIXING_K + h) % 255 + 1);
}