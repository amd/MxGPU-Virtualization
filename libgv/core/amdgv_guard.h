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

#ifndef AMDGV__GUARD_H
#define AMDGV__GUARD_H

#include <amdgv_basetypes.h>
#include <amdgv_api.h>

#define AMDGV_EVENT_OVERFLOW -100

#define AMDGV_DEFAULT_FLR_INTERVAL		  (60 * 1000 * 1000)
#define AMDGV_DEFAULT_FLR_THRESHOLD		  3
#define AMDGV_DEFAULT_INTERRUPT_INTERVAL	  (60 * 1000 * 1000)
#define AMDGV_DEFAULT_INTERRUPT_THRESHOLD	  56
#define AMDGV_DEFAULT_EXCLUSIVE_INTERVAL	  (60 * 1000 * 1000)
#define AMDGV_DEFAULT_EXCLUSIVE_THRESHOLD	  9
#define AMDGV_DEFAULT_EXCLUSIVE_TIMEOUT_INTERVAL  (500 * 1000 * 1000)
#define AMDGV_DEFAULT_EXCLUSIVE_TIMEOUT_THRESHOLD 2
#define AMDGV_DEFAULT_RAS_TELEMETRY_INTERVAL	  (60 * 1000 * 1000)
#define AMDGV_DEFAULT_RAS_TELEMETRY_THRESHOLD	  15

struct amdgv_monitor_event {
	/* event name */
	char name[32];

	spin_lock_t lock;
	/*
	 * 0: the event number is normal
	 * 1: the event number is full
	 * 2: the event number is overflow
	 */
	uint32_t state;
	/* amount of monitor event after enabled */
	uint32_t amount;

	/* threshold of events in the interval(microseconds) */
	uint64_t interval;
	uint32_t threshold;
	/* current number of events in the interval*/
	uint32_t active;

	/* first active record index */
	uint32_t origin_idx;
	/* time record in microseconds */
	uint64_t *record_array;
};

struct amdgv_vf_guard {
	/* 0: the guard is disabled
	 * 1: the guard is enabled
	 */
	uint32_t state;

	/* amount of overflow event */
	uint32_t ov_event;

	struct amdgv_monitor_event event[AMDGV_GUARD_EVENT_MAX];
};

int amdgv_guard_mon_init(struct amdgv_adapter *adapt);
int amdgv_guard_mon_remove(struct amdgv_adapter *adapt);

int amdgv_guard_vf_init(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_guard_vf_remove(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_guard_vf_reset(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_guard_add_active_event(struct amdgv_adapter *adapt, uint32_t idx_vf,
				 uint32_t event_id);
int amdgv_guard_dec_active_event(struct amdgv_adapter *adapt, uint32_t idx_vf,
				 uint32_t event_id);
uint32_t amdgv_guard_is_event_full(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint32_t event_id);

int amdgv_guard_get_vf_info(struct amdgv_adapter *adapt, uint32_t idx_vf,
			    struct amdgv_guard_info *info);

int amdgv_guard_set_vf_info(struct amdgv_adapter *adapt, uint32_t idx_vf,
			    struct amdgv_guard_info *info);

#endif
