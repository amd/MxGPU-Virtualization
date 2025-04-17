/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amdgv_oss_wrapper.h"
#include "amdgv_guard.h"
#include "amdgv_error.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int amdgv_guard_delete_expired_event(struct amdgv_vf_guard *guard);


uint32_t amdgv_guard_is_event_full(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint32_t event_id)
{
	struct amdgv_vf_device *vf;
	struct amdgv_monitor_event *event;

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_WARN("check an invalid %s for event(0x%x)\n", amdgv_idx_to_str(idx_vf),
			   event_id);
		return AMDGV_GUARD_EVENT_NORMAL;
	}

	vf = &adapt->array_vf[idx_vf];
	if (vf->guard == NULL)
		return AMDGV_GUARD_EVENT_NORMAL;

	if (vf->guard->state == AMDGV_GUARD_DISABLED)
		return AMDGV_GUARD_EVENT_NORMAL;

	event = &vf->guard->event[event_id];

	return event->state;
}

int amdgv_guard_dec_active_event(struct amdgv_adapter *adapt, uint32_t idx_vf,
				 uint32_t event_id)
{
	struct amdgv_vf_device *vf;
	struct amdgv_monitor_event *event;

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_WARN("add active event(0x%x) for an invalid %s\n", event_id,
			   amdgv_idx_to_str(idx_vf));
		return AMDGV_FAILURE;
	}

	vf = &adapt->array_vf[idx_vf];

	if (vf->guard == NULL)
		return AMDGV_FAILURE;

	if (vf->guard->state == AMDGV_GUARD_DISABLED)
		return 0;

	event = &vf->guard->event[event_id];

	oss_spin_lock_irq(event->lock);
	if ((event->state == AMDGV_GUARD_EVENT_NORMAL) && (event->active))
		event->active--;
	oss_spin_unlock_irq(event->lock);

	AMDGV_INFO("Decremented active event %s for %s\n", event->name,
		   amdgv_idx_to_str(idx_vf));

	return 0;
}

int amdgv_guard_add_active_event(struct amdgv_adapter *adapt, uint32_t idx_vf,
				 uint32_t event_id)
{
	uint32_t idx;
	uint64_t curr_time;
	struct amdgv_vf_device *vf;
	struct amdgv_monitor_event *event;

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_WARN("add active event(0x%x) for an invalid %s\n", event_id,
			   amdgv_idx_to_str(idx_vf));
		return AMDGV_FAILURE;
	}

	vf = &adapt->array_vf[idx_vf];

	if (vf->guard == NULL)
		return AMDGV_FAILURE;

	if (vf->guard->state == AMDGV_GUARD_DISABLED) {
		return AMDGV_FAILURE;
	}
	amdgv_guard_delete_expired_event(vf->guard);
	event = &vf->guard->event[event_id];
	oss_spin_lock_irq(event->lock);

	if (event->active >= event->threshold) {
		vf->guard->ov_event++;
		event->state = AMDGV_GUARD_EVENT_OVERFLOW;
		oss_spin_unlock_irq(event->lock);
		AMDGV_WARN("Event(%s) of %s is overflow\n", event->name,
			   amdgv_idx_to_str(idx_vf));
		return AMDGV_EVENT_OVERFLOW;
	}

	curr_time = oss_get_time_stamp();

	idx = (event->origin_idx + event->active) % event->threshold;
	event->record_array[idx] = curr_time;

	event->active++;
	event->amount++;

	if (event->active == event->threshold)
		event->state = AMDGV_GUARD_EVENT_FULL;

	oss_spin_unlock_irq(event->lock);
	return 0;
}

static void amdgv_guard_set_event_default_interval(struct amdgv_monitor_event *event,
						   uint32_t event_id)
{
	switch (event_id) {
	case AMDGV_GUARD_EVENT_FLR:
		event->interval = AMDGV_DEFAULT_FLR_INTERVAL;
		break;

	case AMDGV_GUARD_EVENT_EXCLUSIVE_MOD:
		event->interval = AMDGV_DEFAULT_EXCLUSIVE_INTERVAL;
		break;

	case AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT:
		event->interval = AMDGV_DEFAULT_EXCLUSIVE_TIMEOUT_INTERVAL;
		break;

	case AMDGV_GUARD_EVENT_ALL_INT:
		event->interval = AMDGV_DEFAULT_INTERRUPT_INTERVAL;
		break;

	case AMDGV_GUARD_EVENT_RAS_ERR_COUNT:
		event->interval = AMDGV_DEFAULT_RAS_TELEMETRY_INTERVAL;
		break;

	case AMDGV_GUARD_EVENT_RAS_CPER_DUMP:
		event->interval = AMDGV_DEFAULT_RAS_TELEMETRY_INTERVAL;
		break;
	}
}

static void amdgv_guard_set_event_default_threshold(struct amdgv_monitor_event *event,
						    uint32_t event_id)
{
	switch (event_id) {
	case AMDGV_GUARD_EVENT_FLR:
		event->threshold = AMDGV_DEFAULT_FLR_THRESHOLD;
		break;

	case AMDGV_GUARD_EVENT_EXCLUSIVE_MOD:
		event->threshold = AMDGV_DEFAULT_EXCLUSIVE_THRESHOLD;
		break;

	case AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT:
		event->threshold = AMDGV_DEFAULT_EXCLUSIVE_TIMEOUT_THRESHOLD;
		break;

	case AMDGV_GUARD_EVENT_ALL_INT:
		event->threshold = AMDGV_DEFAULT_INTERRUPT_THRESHOLD;
		break;

	case AMDGV_GUARD_EVENT_RAS_ERR_COUNT:
		event->threshold = AMDGV_DEFAULT_RAS_TELEMETRY_THRESHOLD;
		break;

	case AMDGV_GUARD_EVENT_RAS_CPER_DUMP:
		event->threshold = AMDGV_DEFAULT_RAS_TELEMETRY_THRESHOLD;
		break;
	}
}

static void amdgv_guard_set_event_name(struct amdgv_monitor_event *event, uint32_t event_id)
{
	switch (event_id) {
	case AMDGV_GUARD_EVENT_FLR:
		oss_memcpy(event->name, "FLR", 4);
		break;

	case AMDGV_GUARD_EVENT_EXCLUSIVE_MOD:
		oss_memcpy(event->name, "EXCLUSIVE", 10);
		break;

	case AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT:
		oss_memcpy(event->name, "EXCLUSIVE_TIMEOUT", 18);
		break;

	case AMDGV_GUARD_EVENT_ALL_INT:
		oss_memcpy(event->name, "INTERRUPT", 10);
		break;

	case AMDGV_GUARD_EVENT_RAS_ERR_COUNT:
		oss_memcpy(event->name, "RAS_ERR_COUNT", 14);
		break;

	case AMDGV_GUARD_EVENT_RAS_CPER_DUMP:
		oss_memcpy(event->name, "RAS_CPER_DUMP", 14);
		break;
	}
}

static uint32_t amdgv_guard_get_max_threshold(uint32_t event_id)
{
	uint32_t max_threshold;

	switch (event_id) {
	case AMDGV_GUARD_EVENT_FLR:
		max_threshold = AMDGV_GUARD_MAX_FLR;
		break;
	case AMDGV_GUARD_EVENT_EXCLUSIVE_MOD:
		max_threshold = AMDGV_GUARD_MAX_EXCLUSIVE_MOD;
		break;
	case AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT:
		max_threshold = AMDGV_GUARD_MAX_EXCLUSIVE_TIMEOUT;
		break;
	case AMDGV_GUARD_EVENT_ALL_INT:
		max_threshold = AMDGV_GUARD_MAX_ALL_INT;
		break;
	case AMDGV_GUARD_EVENT_RAS_ERR_COUNT:
		max_threshold = AMDGV_GUARD_MAX_RAS_TELEMETRY;
		break;
	case AMDGV_GUARD_EVENT_RAS_CPER_DUMP:
		max_threshold = AMDGV_GUARD_MAX_RAS_TELEMETRY;
		break;
	default:
		max_threshold = 0;
		break;
	}

	return max_threshold;
}

int amdgv_guard_vf_init(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int i;
	struct amdgv_monitor_event *event;
	struct amdgv_vf_device *vf;
	uint32_t max_threshold;

	vf = &adapt->array_vf[idx_vf];
	vf->guard = oss_zalloc(sizeof(struct amdgv_vf_guard));
	if (!vf->guard) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct amdgv_vf_guard));
		return AMDGV_FAILURE;
	}

	for (i = 0; i < AMDGV_GUARD_EVENT_MAX; i++) {
		event = &vf->guard->event[i];

		amdgv_guard_set_event_name(event, i);
		amdgv_guard_set_event_default_interval(event, i);
		amdgv_guard_set_event_default_threshold(event, i);

		max_threshold = amdgv_guard_get_max_threshold(i);
		event->record_array = oss_zalloc(max_threshold * sizeof(int64_t));
		if (!event->record_array) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
					max_threshold * sizeof(int64_t));
			goto failed;
		}

		event->lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
		if (event->lock == OSS_INVALID_HANDLE) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL,
					0);
			goto failed;
		}
		event->state = AMDGV_GUARD_EVENT_NORMAL;
	}

	if (adapt->flags & AMDGV_FLAG_SENSITIVE_EVENT_GUARD)
		vf->guard->state = AMDGV_GUARD_ENABLED;
	else
		vf->guard->state = AMDGV_GUARD_DISABLED;

	return 0;

failed:
	for (; i > 0; i--) {
		if (event->record_array) {
			oss_free(event->record_array);
			event->record_array = NULL;
		}

		if (event->lock)
			oss_spin_lock_fini(event->lock);
	}

	oss_free(vf->guard);
	vf->guard = NULL;
	return AMDGV_FAILURE;
}

int amdgv_guard_vf_remove(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int i;
	struct amdgv_monitor_event *event;
	struct amdgv_vf_device *vf;

	vf = &adapt->array_vf[idx_vf];

	if (vf->guard->state == AMDGV_GUARD_ENABLED)
		vf->guard->state = AMDGV_GUARD_DISABLED;
	else
		AMDGV_WARN("event guard of %s was disabled!!\n", amdgv_idx_to_str(idx_vf));

	for (i = 0; i < AMDGV_GUARD_EVENT_MAX; i++) {
		event = &vf->guard->event[i];
		event->state = AMDGV_GUARD_EVENT_NORMAL;
		oss_free(event->record_array);
		event->record_array = NULL;
		oss_spin_lock_fini(event->lock);
	}

	oss_free(vf->guard);
	vf->guard = NULL;

	return 0;
}

int amdgv_guard_vf_reset(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int i;
	struct amdgv_monitor_event *event;
	struct amdgv_vf_device *vf;

	vf = &adapt->array_vf[idx_vf];

	if (!vf->guard)
		goto failed;

	for (i = 0; i < AMDGV_GUARD_EVENT_MAX; i++) {
		event = &vf->guard->event[i];

		oss_spin_lock_irq(event->lock);

		event->amount = 0;
		event->active = 0;
		event->origin_idx = 0;

		if (!event->record_array) {
			oss_spin_unlock_irq(event->lock);
			goto failed;
		}
		oss_memset(event->record_array, 0, event->threshold * sizeof(int64_t));
		event->state = AMDGV_GUARD_EVENT_NORMAL;

		oss_spin_unlock_irq(event->lock);
	}

	vf->guard->ov_event = 0;

	return 0;

failed:
	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GUARD_RESET_FAIL, idx_vf);
	return AMDGV_FAILURE;
}

static int amdgv_guard_delete_expired_event(struct amdgv_vf_guard *guard)
{
	int i;
	uint32_t idx;
	uint64_t curr;
	uint64_t delta;
	struct amdgv_monitor_event *event;

	for (i = 0; i < AMDGV_GUARD_EVENT_MAX; i++) {
		event = &guard->event[i];

		oss_spin_lock_irq(event->lock);
		while (event->active > 0) {
			idx = event->origin_idx;

			curr = oss_get_time_stamp();
			delta = curr - event->record_array[idx];

			if (delta < event->interval) {
				break;
			} else {
				event->active--;
				event->state = AMDGV_GUARD_EVENT_NORMAL;
				idx = (idx + 1) % event->threshold;
				event->origin_idx = idx;
			}
		}
		oss_spin_unlock_irq(event->lock);
	}

	return 0;
}

int amdgv_guard_set_vf_info(struct amdgv_adapter *adapt, uint32_t idx_vf,
			    struct amdgv_guard_info *info)
{
	struct amdgv_monitor_event *event;
	struct amdgv_vf_guard *guard;
	uint32_t max_threshold;

	if ((info->type > AMDGV_GUARD_ALL) || (AMDGV_IS_IDX_INVALID(idx_vf))) {
		AMDGV_WARN("wrong para guard type(%d) %s\n", info->type,
			   amdgv_idx_to_str(idx_vf));
		return AMDGV_FAILURE;
	}

	guard = adapt->array_vf[idx_vf].guard;

	if (info->type == AMDGV_GUARD_ALL) {
		guard->state = info->parm.general.state;

		return 0;
	}

	if (info->type < AMDGV_GUARD_EVENT_MAX) {
		event = &guard->event[info->type];

		max_threshold = amdgv_guard_get_max_threshold(info->type);
		if (info->parm.event.threshold > max_threshold) {
			AMDGV_WARN("%s guard threshold %d is larger than max threshold %d\n",
				   event->name, info->parm.event.threshold, max_threshold);
			return AMDGV_FAILURE;
		}

		event->state = info->parm.event.state;
		event->interval = info->parm.event.interval;
		event->threshold = info->parm.event.threshold;
	}

	return 0;
}

int amdgv_guard_get_vf_info(struct amdgv_adapter *adapt, uint32_t idx_vf,
			    struct amdgv_guard_info *info)
{
	struct amdgv_monitor_event *event;
	struct amdgv_vf_guard *guard;

	if ((info->type > AMDGV_GUARD_ALL) || (AMDGV_IS_IDX_INVALID(idx_vf))) {
		AMDGV_WARN("wrong para guard type(%d) %s\n", info->type,
			   amdgv_idx_to_str(idx_vf));
		return AMDGV_FAILURE;
	}

	guard = adapt->array_vf[idx_vf].guard;

	if (idx_vf == AMDGV_PF_IDX && info->type == AMDGV_GUARD_ALL) {
		info->parm.general.state = 0;
		info->parm.general.ov_event = 0;
		return 0;
	}

	if (idx_vf == AMDGV_PF_IDX) {
		info->parm.event.state = 0;
		info->parm.event.interval = 0;
		info->parm.event.threshold = 0;
		info->parm.event.active = 0;
		info->parm.event.amount = 0;
		return 0;
	}

	amdgv_guard_delete_expired_event(guard);

	if (info->type == AMDGV_GUARD_ALL) {
		info->parm.general.state = guard->state;
		info->parm.general.ov_event = guard->ov_event;
		return 0;
	}

	if (info->type < AMDGV_GUARD_EVENT_MAX) {
		event = &guard->event[info->type];

		info->parm.event.state = event->state;
		info->parm.event.interval = event->interval;
		info->parm.event.threshold = event->threshold;

		info->parm.event.active = event->active;
		info->parm.event.amount = event->amount;
	}

	return 0;
}
