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

#ifndef AMDGV_SCHED_INTERNAL_H
#define AMDGV_SCHED_INTERNAL_H

#include "amdgv_sched.h"

#ifndef INLINE
#define INLINE (static inline)
#endif

#define AMDGV_SCHED_EXCLUSIVE_TIMEOUT_MS_1VF 3000
#define AMDGV_SCHED_EXCLUSIVE_TIMEOUT_MS_COMMON 1500

#define set_to_avail_vf(idx_vf) adapt->sched.array_vf[(idx_vf)].state = AMDGV_SCHED_AVAIL

#define set_to_active_vf(idx_vf) adapt->sched.array_vf[(idx_vf)].state = AMDGV_SCHED_ACTIVE

#define set_to_suspend_vf(idx_vf) adapt->sched.array_vf[(idx_vf)].state = AMDGV_SCHED_SUSPEND

#define set_to_unavail_vf(idx_vf) adapt->sched.array_vf[(idx_vf)].state = AMDGV_SCHED_UNAVAL

#define is_in_full_access() adapt->sched.in_full_access

#define has_vf_in_full_access() (adapt->sched.logical_sched_fa_mask ? true : false)

#define is_any_vf_in_full_access() \
		(adapt->sched.enable_per_partition_full_access ? \
		has_vf_in_full_access() : is_in_full_access())

#define is_any_share_engine_vf_in_full_access(idx_vf) \
		(adapt->sched.enable_per_partition_full_access ? \
		(adapt->sched.logical_sched_fa_mask & adapt->sched.array_vf[(idx_vf)].world_switch_mask) : is_in_full_access())

#define is_vf_in_full_access(idx_vf) \
		(adapt->sched.enable_per_partition_full_access ? \
		(adapt->sched.array_vf[(idx_vf)].in_full_access) : is_in_full_access())

#define is_avail_vf(idx_vf) (adapt->sched.array_vf[(idx_vf)].state == AMDGV_SCHED_AVAIL)

#define is_active_vf(idx_vf) (adapt->sched.array_vf[(idx_vf)].state == AMDGV_SCHED_ACTIVE)

#define is_full_access_vf(idx_vf) \
		(adapt->sched.enable_per_partition_full_access ? \
		(adapt->sched.array_vf[(idx_vf)].in_full_access) : (adapt->sched.idx_vf_full_access == (idx_vf)))

#define is_suspend_vf(idx_vf) (adapt->sched.array_vf[(idx_vf)].state == AMDGV_SCHED_SUSPEND)

#define is_unavail_vf(idx_vf) (adapt->sched.array_vf[(idx_vf)].state == AMDGV_SCHED_UNAVAL)

INLINE uint32_t amdgv_sched_active_vf_num(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf, count = 0;

	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++)
		if is_active_vf (idx_vf)
			count++;

	return count;
}

INLINE uint32_t amdgv_sched_active_vf_ex_pf_num(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf, count = 0;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++)
		if is_active_vf (idx_vf)
			count++;

	return count;
}

INLINE bool is_active_or_suspend_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return (is_active_vf(idx_vf) || is_suspend_vf(idx_vf));
}

INLINE bool amdgv_sched_vf_assigned_to_vm(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;

	if (!(adapt->flags & AMDGV_FLAG_USE_PF))
		return oss_get_assigned_vf_count(adapt->dev, false) > 0;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		if is_active_vf(idx_vf)
			return true;
	}

	return false;
}


int amdgv_sched_world_switch_init(struct amdgv_adapter *adapt);
void amdgv_sched_world_switch_fini(struct amdgv_adapter *adapt);

int amdgv_sched_world_switch_remap_vf_assignment(struct amdgv_adapter *adapt);
int amdgv_sched_world_switch_start(struct amdgv_adapter *adapt,
						   struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_switch_stop(struct amdgv_adapter *adapt,
						   struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_switch_shutdown_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
						   struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_switch_add_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
						   struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_switch_remove_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
						   struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_switch_toggle_skip_next_punish(struct amdgv_adapter *adapt, uint32_t idx_vf,
				struct amdgv_sched_world_switch *world_switch, bool enable);
int amdgv_sched_world_switch_set_vf_num(struct amdgv_adapter *adapt, uint32_t num_vf,
								struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_switch_update_time_slice(struct amdgv_adapter *adapt,
					       uint32_t idx_vf,
						   struct amdgv_sched_world_switch *world_switch);

int amdgv_sched_world_context_get_curr_vf(struct amdgv_adapter *adapt,
					  struct amdgv_sched_world_switch *world_switch,
					  uint32_t *curr_idx_vf);
int amdgv_sched_world_context_get_hw_curr_state(struct amdgv_adapter *adapt,
					     uint32_t hw_sched_id,
					     uint32_t *curr_vf_state);
int amdgv_sched_world_context_switch_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_context_one_time_loop(struct amdgv_adapter *adapt,
					   struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_context_init(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_context_load(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_context_save(struct amdgv_adapter *adapt,
				   struct amdgv_sched_world_switch *world_switch);
void amdgv_sched_world_context_clear_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   struct amdgv_sched_world_switch *world_switch);
void amdgv_sched_world_context_clear_state_rst(struct amdgv_adapter *adapt);
int amdgv_sched_world_switch_config_auto_sched_mode(struct amdgv_adapter *adapt,
					  struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_manual_switch_clear_time_slice(struct amdgv_sched_world_switch *world_switch, uint32_t idx_vf);
bool amdgv_sched_world_context_is_state_ok(struct amdgv_adapter *adapt,
						struct amdgv_sched_world_switch *world_switch);
bool amdgv_sched_world_context_all_states_ok(struct amdgv_adapter *adapt);
int amdgv_sched_world_context_get_abnormal_world_switch(struct amdgv_adapter *adapt,
				struct amdgv_sched_world_switch **world_switch);
int amdgv_sched_world_context_set_vf_abnormal(struct amdgv_adapter *adapt, uint32_t idx_vf,
					      struct amdgv_sched_world_switch *world_switch);

int amdgv_sched_queue_event_process_suspend(struct amdgv_adapter *adapt);
void amdgv_sched_queue_event_process_resume(struct amdgv_adapter *adapt);
int amdgv_sched_event_queue_process_init(struct amdgv_adapter *adapt);
void amdgv_sched_event_queue_process_fini(struct amdgv_adapter *adapt);

#ifdef WS_RECORD
int amdgv_sched_record_queue_process_init(struct amdgv_adapter *adapt);
void amdgv_sched_record_queue_process_fini(struct amdgv_adapter *adapt);
#endif

int amdgv_sched_reset_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 enum amdgv_sched_block sched_block);
int amdgv_sched_reset_vf_auto(struct amdgv_adapter *adapt);
int amdgv_sched_gpu_reset_wrap(struct amdgv_adapter *adapt, bool reset_all);

void amdgv_sched_dump_gpu_state(struct amdgv_adapter *adapt);

int amdgv_sched_world_switch_list_active(struct amdgv_adapter *adapt);
int amdgv_sched_world_switch_sched_mask_start(struct amdgv_adapter *adapt,
		uint32_t sched_mask);

int amdgv_sched_world_switch_reset(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     struct amdgv_sched_world_switch *world_switch);
int amdgv_sched_world_context_sync_abnormal_sched(struct amdgv_adapter *adapt, uint32_t abnormal_idx_vf,
	struct amdgv_sched_world_switch *logical_world_switch);
int64_t amdgv_sched_world_switch_calculate_time_slice(struct amdgv_adapter *adapt,
							     struct amdgv_sched_world_switch *world_switch,
							     uint32_t idx_vf);
#endif
