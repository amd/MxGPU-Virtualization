/*
 * Copyright (c) 2017-2023 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amdgv_sched_internal.h"
#include "amdgv_gpumon_internal.h"
#include "amdgv_list.h"
#include "amdgv_gpuiov.h"
#include "amdgv_vfmgr.h"

static const uint32_t this_block = AMDGV_SCHEDULER_BLOCK;

int (*amdgv_schedule_vfs) (struct amdgv_sched_world_switch *world_switch,
			struct amdgv_list_head *active_list,
			int *vf_idx, uint64_t *ts);

#define MAX_VF_SKIP_CNT (8)
#define HLIQUID_ALL_VF_IDLE_MIN_TS (500)
/**********************************************************************
 * Helper functions
 **********************************************************************/
/* Check the manual active list to find out if it empty. Note
 * than in fairness mode a dummy VF is still in the list. So
 * we check that they are not dummy
 */
static bool amdgv_sched_active_list_empty(struct amdgv_sched_world_switch *world_switch)
{
	struct amdgv_sched_active_vf_entry *entry;

	if (amdgv_list_empty(&world_switch->manual.active_vf_list))
		if (!world_switch->manual.fairness_mode)
			return true;

	amdgv_list_for_each_entry (entry, &world_switch->manual.active_vf_list,
				   struct amdgv_sched_active_vf_entry, list) {
		if (!entry->dummy_vf)
			return false;
	}

	return true;
}

static inline bool amdgv_sched_is_one_active_vf(struct amdgv_sched_world_switch *world_switch)
{
	struct amdgv_adapter *adapt = world_switch->manual.adapt;
	uint32_t vfs;
	uint32_t mask = (1 << adapt->num_vf) - 1;

	vfs = world_switch->vf_inited & mask;

	return vfs && (!(vfs & (vfs - 1)));
}

void amdgv_sched_dump_gpu_state(struct amdgv_adapter *adapt)
{
	if (adapt->sched.dump_gpu_state)
		adapt->sched.dump_gpu_state(adapt);
}

static void amdgv_sched_start_record_vf_time(struct amdgv_adapter *adapt,
						 struct amdgv_sched_world_switch *world_switch,
						 uint32_t idx_vf, bool is_init)
{
	struct amdgv_sched_active_vf_entry *entry;

	entry = &world_switch->manual.array_vf[idx_vf];
	entry->start_ts = oss_get_time_stamp();

	amdgv_gpumon_update_load_start_time(adapt, idx_vf, world_switch, is_init);
}

static void amdgv_sched_stop_record_vf_time(struct amdgv_adapter *adapt,
						struct amdgv_sched_world_switch *world_switch,
						uint32_t idx_vf)
{
	struct amdgv_sched_active_vf_entry *entry;
	struct amdgv_time_log *time_log;
	struct amdgv_histogram *run_summation_us;
	int64_t duration;
	uint32_t hw_sched_id;
	uint64_t active_time, time_max, bucket, start, interval, curr_ts = oss_get_time_stamp();

	entry = &world_switch->manual.array_vf[idx_vf];

	if (entry->start_ts) {
		// update histogram data
		if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX) {
			for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
				time_log = &adapt->array_vf[idx_vf].time_log[hw_sched_id];
				run_summation_us = &time_log->gfx_run_summation_us;

				start = adapt->array_vf[AMDGV_PF_IDX]
						.time_log[hw_sched_id]
						.gfx_run_summation_us.start;
				interval = adapt->array_vf[AMDGV_PF_IDX]
						   .time_log[hw_sched_id]
						   .gfx_run_summation_us.interval;

				active_time = curr_ts - entry->start_ts;
				time_max = start + interval * AMDGV_HISTOGRAM_SIZE;

				if (active_time >= start && active_time < time_max) {
					bucket = (active_time - start) / interval;
				} else {
					bucket = active_time < start ?
							 0 :
							 AMDGV_HISTOGRAM_SIZE - 1;
					/* If active_time is out of the range and less than
					* start time update first bucket, otherwise last one
					*/
				}
				run_summation_us->count[bucket]++;
				run_summation_us->total++;
			}
		}

		// update world switch time credit
		entry->total_time += curr_ts - entry->start_ts;
		duration = curr_ts - (entry->start_ts + entry->last_time_slice);
		if (duration > 0) {
			/* No Punishment under BP Mode */
			if (entry->skip_next_punish || adapt->bp_mode != AMDGV_BP_MODE_DISABLE)
				entry->skip_next_punish = false;
			else
				entry->beyond_time_cycle += duration;
		}
		entry->start_ts = 0;
	}
	if (!entry->dummy_vf)
		amdgv_gpumon_update_save_end_time(adapt, idx_vf, world_switch);
	return;
}

/**********************************************************************
 * World switch internal APIs
 **********************************************************************/
static int amdgv_init_world_context(struct amdgv_adapter *adapt, uint32_t idx_vf,
					struct amdgv_sched_world_switch *world_switch)
{
	bool mark_bad = false;

	AMDGV_ASSERT(world_switch->curr_vf_state == AMDGV_VF_CONTEXT_SAVED ||
			 world_switch->curr_vf_state == AMDGV_VF_CONTEXT_CLEAR);

	AMDGV_DEBUG("the current is %s, curr_vf_state=%d, to init %s\n",
			amdgv_idx_to_str(world_switch->curr_idx_vf), world_switch->curr_vf_state,
			amdgv_idx_to_str(idx_vf));

	world_switch->curr_idx_vf = idx_vf;

	world_switch->vf_inited |= 1 << idx_vf;

	amdgv_sched_start_record_vf_time(adapt, world_switch, idx_vf, true);

	if (amdgv_logical_sched_state_run(adapt, idx_vf, world_switch))
		goto failed;

	if (adapt->vbios.golden_init)
		adapt->vbios.golden_init(adapt);

	world_switch->curr_vf_state = AMDGV_VF_CONTEXT_LOADED;

	if (mark_bad)
		goto failed;

	return 0;

failed:
	world_switch->curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
	return AMDGV_FAILURE;
}

static int amdgv_load_world_context(struct amdgv_adapter *adapt, uint32_t idx_vf,
					struct amdgv_sched_world_switch *world_switch)
{
	AMDGV_ASSERT(world_switch->curr_vf_state == AMDGV_VF_CONTEXT_SAVED ||
			 world_switch->curr_vf_state == AMDGV_VF_CONTEXT_CLEAR);

	AMDGV_DEBUG("the current is %s, curr_vf_state=%d, to load %s\n",
			amdgv_idx_to_str(world_switch->curr_idx_vf), world_switch->curr_vf_state,
			amdgv_idx_to_str(idx_vf));

	world_switch->curr_idx_vf = idx_vf;

	amdgv_sched_start_record_vf_time(adapt, world_switch, idx_vf, false);

	if (amdgv_logical_sched_state_run(adapt, idx_vf, world_switch))
		goto failed;

	world_switch->curr_vf_state = AMDGV_VF_CONTEXT_LOADED;
	return 0;

failed:
	world_switch->switch_running = false;
	world_switch->curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
	return AMDGV_FAILURE;
}

static int amdgv_save_world_context(struct amdgv_adapter *adapt,
					struct amdgv_sched_world_switch *world_switch)
{
	uint32_t idx_vf;

	AMDGV_ASSERT(world_switch->curr_vf_state == AMDGV_VF_CONTEXT_LOADED ||
			 world_switch->curr_vf_state == AMDGV_VF_CONTEXT_CLEAR);

	AMDGV_DEBUG("save %s, curr_vf_state=%d\n", amdgv_idx_to_str(world_switch->curr_idx_vf),
			world_switch->curr_vf_state);

	idx_vf = world_switch->curr_idx_vf;

	if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_LOADED) {

		if (amdgv_logical_sched_state_pause(adapt, idx_vf, world_switch))
			goto failed;

		world_switch->curr_vf_state = AMDGV_VF_CONTEXT_SAVED;
	}

	amdgv_sched_stop_record_vf_time(adapt, world_switch, idx_vf);
	return 0;

failed:
	world_switch->switch_running = false;
	world_switch->curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
	amdgv_sched_stop_record_vf_time(adapt, world_switch, idx_vf);
	return AMDGV_FAILURE;
}

static int amdgv_switch_world_context(struct amdgv_adapter *adapt, uint32_t idx_vf,
					  struct amdgv_sched_world_switch *world_switch)
{
	int ret;

	if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL)
		return AMDGV_FAILURE;

	if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_LOADED) {
		/* the current active vf is the target vf */
		if (world_switch->curr_idx_vf == idx_vf)
			return 0;

		/* idle, save vf */
		if (amdgv_save_world_context(adapt, world_switch) != 0)
			return AMDGV_FAILURE;
	}

	if (world_switch->vf_inited & (1 << idx_vf))
		/* load, run vf */
		ret = amdgv_load_world_context(adapt, idx_vf, world_switch);
	else
		/* init run vf */
		ret = amdgv_init_world_context(adapt, idx_vf, world_switch);

	return ret;
}

int64_t
amdgv_sched_world_switch_calculate_time_slice(struct amdgv_adapter *adapt,
						  struct amdgv_sched_world_switch *world_switch,
						  uint32_t idx_vf)
{
	struct amdgv_sched_active_vf_entry *entry;
	int64_t time_slice;
	int64_t min_time_slice;
	bool dummy;

	AMDGV_ASSERT(world_switch->sched_block < AMDGV_SCHED_BLOCK_MAX);

	entry = &world_switch->manual.array_vf[idx_vf];

	/* save the dummy and time_slice from the current entry.
	 * If we are processing a dummy VF. Calculate the
	 * timeslice based on the world-switch overhead to the PF
	 */
	dummy = entry->dummy_vf;
	time_slice = entry->time_slice;
	min_time_slice = entry->time_slice / 2;

	if (dummy)
		entry = &world_switch->manual.array_vf[AMDGV_PF_IDX];

	if (world_switch->manual.fairness_mode) {
		/* If a VF is given single VF status, check if self-switch
		 * is enabled. If it is, reschedule the VF every 500ms, if not
		 * do not re-schedule the world-switch timer
		 */
		if (entry->time_slice == DEFAULT_GFX_TIME_SLICE_1VF) {
			/* Beyond time cycle is ignored in single VF mode */
			entry->beyond_time_cycle = 0;
			entry->last_time_slice =
				GET_GFX_TIME_SLICE(adapt, adapt->sched.num_vf_per_gfx_sched);

			goto out;
		}

		time_slice -= entry->beyond_time_cycle;

		/* If beyond_time_cycle overruns time_slice,
		 * will return 0 to skip scheduling it.
		 * And if it gets a time_slice smaller than min_time_slice,
		 * it can to be scheduled with min_time_slice,
		 * but the credit of delta will be updated to beyond_time_cycle.
		 */
		if (time_slice <= 0) {
			entry->last_time_slice = 0;
			entry->beyond_time_cycle -= entry->time_slice;
			AMDGV_DEBUG("skip scheduling %s\n", amdgv_idx_to_str(entry->idx_vf));
		} else {
			if (time_slice <= min_time_slice) {
				entry->last_time_slice = min_time_slice;
				entry->beyond_time_cycle = (min_time_slice - time_slice);
				AMDGV_DEBUG("punish %s %dus\n",
						amdgv_idx_to_str(entry->idx_vf),
						entry->time_slice - min_time_slice);
			} else {
				entry->last_time_slice = time_slice;
				entry->beyond_time_cycle = 0;
				AMDGV_DEBUG("punish %s %dus\n",
						amdgv_idx_to_str(entry->idx_vf),
						entry->time_slice - time_slice);
			}
		}
		/* For dummy VF, we only care about the cumulated overhead
		 * on the last world-switch, we need to give a bit more of leeway
		 * to the PF time slice.
		 */
		if (dummy)
			entry->beyond_time_cycle = 0;
	} else {
		entry->last_time_slice = entry->time_slice;
	}
out:
	return entry->last_time_slice;
}

static int amdgv_schedule_vfs_default (struct amdgv_sched_world_switch *world_switch,
				struct amdgv_list_head *active_list, int *vf_idx, uint64_t *ts)
{

	struct amdgv_sched_active_vf_entry *entry;
	struct amdgv_adapter *adapt = world_switch->manual.adapt;

re_schedule:
	entry = amdgv_list_first_entry(active_list,
					   struct amdgv_sched_active_vf_entry, list);
	/* pf may be set to suspend but yet removed
	 * from world switch, so dont schedual pf here
	 * since it is going to be removed
	 */
	if ((entry->idx_vf == AMDGV_PF_IDX) &&
		 (!is_active_vf(AMDGV_PF_IDX))) {
		amdgv_list_move_tail(&entry->list, active_list);
		entry = amdgv_list_first_entry(active_list, struct amdgv_sched_active_vf_entry, list);
	}

	*vf_idx = entry->idx_vf;
	*ts = amdgv_sched_world_switch_calculate_time_slice(adapt, world_switch,
								   entry->idx_vf);

	if (*ts == 0) {
		AMDGV_DEBUG("skip scheduling %s\n", amdgv_idx_to_str(entry->idx_vf));
		amdgv_list_move_tail(&entry->list, &world_switch->manual.active_vf_list);

		goto re_schedule;
	}

	if (world_switch->manual.fairness_mode && entry->dummy_vf) {
		AMDGV_DEBUG("load dummy %s, time slice = %d\n",
				amdgv_idx_to_str(entry->idx_vf), *ts);
		amdgv_list_move_tail(&entry->list, &world_switch->manual.active_vf_list);
		*vf_idx = AMDGV_PF_IDX;
	}

	return 0;
}
static int amdgv_schedule_vfs_liquid (struct amdgv_sched_world_switch *world_switch,
				struct amdgv_list_head *active_list, int *vf_idx, uint64_t *ts)
{
	uint32_t vf_status = 0;
	struct amdgv_sched_active_vf_entry *entry;
	struct amdgv_adapter *adapt = world_switch->manual.adapt;

	/* Hybrid liquid mode busy status has 2 parts
	 * 1. vf_busy_status got from HW
	 * 2. the current VF timeout
	 */
	vf_status = (world_switch->vf_busy_status | world_switch->vf_timeout) &
			 world_switch->vf_inited;

	/* Only hybrid liquid mode update the vf_status, otherwise vf_status = 0
	 *
	 * The first busy VF in the list is priority to get the GPU.
	 * when this VF was switched out, it will be moved to list tail.
	 *
	 * If all VF is idle, run the VF one by one.
	 */

	entry = amdgv_list_first_entry(active_list,
					   struct amdgv_sched_active_vf_entry, list);
	if ((entry->idx_vf == AMDGV_PF_IDX) &&
		 (!is_active_vf(AMDGV_PF_IDX))) {
		amdgv_list_move_tail(&entry->list, active_list);
	}

	if (!vf_status)
		entry = amdgv_list_first_entry(active_list,
					   struct amdgv_sched_active_vf_entry, list);
	else
		amdgv_list_for_each_entry (entry, active_list,
				   struct amdgv_sched_active_vf_entry, list) {
			if (vf_status & (1 << entry->idx_vf))
				break;

			if (entry == amdgv_list_last_entry(active_list,
					struct amdgv_sched_active_vf_entry, list)) {
				AMDGV_DEBUG("Failed to get a valid VF, run the first VF in the list\n");
				entry = amdgv_list_first_entry(active_list,
						   struct amdgv_sched_active_vf_entry, list);

				break;
			}

			if ((entry->idx_vf == AMDGV_PF_IDX) &&
				(!is_active_vf(AMDGV_PF_IDX)))
				continue;

			if (entry->skip_cnt >= MAX_VF_SKIP_CNT)
				break;

			entry->skip_cnt++;
		}

	entry->skip_cnt = 0;

	*vf_idx = entry->idx_vf;
	*ts = entry->time_slice;

	return 0;
}

/**********************************************************************
 * World switch manual functions
 **********************************************************************/
static int amdgv_sched_manual_switch_process(void *context)
{
	uint32_t sched_block;
	struct amdgv_sched_world_switch *world_switch =
		(struct amdgv_sched_world_switch *)context;
	struct amdgv_adapter *adapt = world_switch->manual.adapt;
	struct amdgv_sched_active_vf_entry *entry;
	int idx_vf = 0;
	uint32_t vf_status = 0;
	long long left_ts = 0;
	uint64_t time_slice = 0;

	if (GET_GFX_TIME_SLICE(adapt, adapt->sched.num_vf_per_gfx_sched) ==
			DEFAULT_GFX_TIME_SLICE_1VF &&
		(adapt->in_live_update))
		return 0;

	sched_block = world_switch->sched_block;

	oss_mutex_lock(world_switch->manual.switching_lock);

	/* check if switch has been paused */
	if (world_switch->switch_running == false)
		goto out;

	if (world_switch->curr_idx_vf == adapt->force_switch_vf_idx &&
		world_switch->curr_vf_state == AMDGV_VF_CONTEXT_LOADED)
		goto out;

	/* If the active_vf_list is empty, but the world-switch thead is enabled
	 * load the PF. If a VF sends REQ_GPU_INIT
	 * the PF will get unloaded to process full access mode, and world switch
	 * will be reloaded, thus we don't need to re-init the timer here.
	 */
	if (amdgv_sched_active_list_empty(world_switch)) {
		if (world_switch->curr_vf_state != AMDGV_VF_CONTEXT_LOADED) {
			idx_vf = AMDGV_PF_IDX;
			goto load_fcn;
		}

		goto out;
	}

	if (world_switch->sched_mode == AMDGV_SCHED_HYBRID_LIQUID_MODE) {
		if (amdgv_sched_is_one_active_vf(world_switch) &&
			world_switch->curr_vf_state == AMDGV_VF_CONTEXT_LOADED)
			goto out;

		vf_status = world_switch->vf_busy_status | world_switch->vf_timeout;
		entry = &world_switch->manual.array_vf[world_switch->curr_idx_vf];

		/* if the VF hliquid_min_ts is set, priority to HLIQUID_ALL_VF_IDLE_MIN_TS */
		if (world_switch->hliquid_min_ts) {
			left_ts = world_switch->hliquid_min_ts - (oss_get_time_stamp() - entry->start_ts);
		/* the VF min ts only valid when all VF idle */
		} else if (!vf_status) {
			left_ts = HLIQUID_ALL_VF_IDLE_MIN_TS - (oss_get_time_stamp() - entry->start_ts);
		}

		if (left_ts > 0 && world_switch->curr_vf_state == AMDGV_VF_CONTEXT_LOADED) {
			if (oss_timer_start(world_switch->manual.timer, left_ts, OSS_TIMER_TYPE_ONE_TIME))
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_HRTIMER_START_FAIL, 0);
			goto out;
		}
	}

	/* if the current vf's context hasn't been saved,
	 * save its context before switch to other vf.
	 */
	if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_LOADED) {
		entry = &world_switch->manual.array_vf[world_switch->curr_idx_vf];
		if (AMDGV_IS_IDX_INVALID(entry->idx_vf)) {
			amdgv_put_error(entry->idx_vf, AMDGV_ERROR_DRIVER_INVALID_VALUE,
					entry->idx_vf);

			AMDGV_ASSERT(0);
		}

		/* idle and save the current active vf */
		if (amdgv_save_world_context(adapt, world_switch)) {
			AMDGV_WARN("save current VF's world context failed.\n");
			goto reset_vf;
		}
		if (!AMDGV_IS_IDX_INVALID(entry->idx_vf)) {
			/* Do not add the PF into the active list unless we have enabled
			* USE_PF and the PF is active. Otherwise this is a PF that has been
			* added during idle.
			*/
			if ((entry->idx_vf != AMDGV_PF_IDX) ||
				((adapt->flags & AMDGV_FLAG_USE_PF) && is_active_vf(AMDGV_PF_IDX)))
				amdgv_list_move_tail(&entry->list,
							&world_switch->manual.active_vf_list);
		}
	} else {
		entry = amdgv_list_last_entry(&world_switch->manual.active_vf_list,
						  struct amdgv_sched_active_vf_entry, list);
		if (entry != NULL && entry->dummy_vf && world_switch->manual.fairness_mode) {
			amdgv_sched_stop_record_vf_time(adapt, world_switch, entry->idx_vf);
		}
	}

		/* pick up the next vf to be loading to GPU */
	if (adapt->force_switch_vf_idx >= AMDGV_MAX_VF_SLOT) {
		if (amdgv_schedule_vfs(world_switch,
			&world_switch->manual.active_vf_list, &idx_vf, &time_slice))
			goto out;
	} else {
		/* debug mode, force switch to specified VF */
		amdgv_list_for_each_entry (entry, &world_switch->manual.active_vf_list,
					   struct amdgv_sched_active_vf_entry, list) {
			idx_vf = entry->idx_vf;
			if (entry->idx_vf == adapt->force_switch_vf_idx) {
				time_slice = GET_GFX_TIME_SLICE(
					adapt, adapt->sched.num_vf_per_gfx_sched);
				break;
			}
		}
	}

load_fcn:
	AMDGV_DEBUG("load %s, time slice = %d\n", amdgv_idx_to_str(idx_vf), time_slice);

	if (world_switch->sched_mode == AMDGV_SCHED_HYBRID_LIQUID_MODE)
		world_switch->vf_timeout &= ~(1 << idx_vf);

	/* load the next vf's context */
	if (amdgv_load_world_context(adapt, idx_vf, world_switch)) {
		AMDGV_WARN("load next VF's world context failed.\n");
		goto reset_vf;
	}

	if ((adapt->bp_mode == AMDGV_BP_MODE_1) && (world_switch->curr_idx_vf == AMDGV_PF_IDX) &&
		adapt->array_vf[idx_vf].vf_status == AMDGV_VF_STATUS_END_INIT &&
		world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX) {
		AMDGV_INFO("GFX world switch paused at first PF Run\n");
		world_switch->switch_running = false;
		adapt->bp_gfx_ws_pause_flag = 1;
	}

	/* Check the 1VF magic time slice again. If set then avoid re-scheduling.
	 * The event thread will wake world-switch again when a new event is added.
	 * For example, FLR, ADD_VF, SUSPEND_VF, etc
	 */
	if (time_slice == DEFAULT_GFX_TIME_SLICE_1VF)
		goto out;

	if (oss_timer_start(world_switch->manual.timer, time_slice, OSS_TIMER_TYPE_ONE_TIME)) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_HRTIMER_START_FAIL, 0);
	}

out:
	oss_mutex_unlock(world_switch->manual.switching_lock);
	return 0;

reset_vf:
	/* queue SCHED_RESET_VF to event process to reset vf */
	amdgv_sched_queue_event(adapt, world_switch->curr_idx_vf, AMDGV_EVENT_SCHED_RESET_VF,
				sched_block);
	oss_mutex_unlock(world_switch->manual.switching_lock);
	return 0;
}

static int amdgv_sched_manual_switch_timer_isr(void *context)
{
	struct amdgv_sched_world_switch *world_switch =
		(struct amdgv_sched_world_switch *)context;

	/*
	 * VF timeout, force to run this VF in the next cycle.
	 */
	if (world_switch->sched_mode == AMDGV_SCHED_HYBRID_LIQUID_MODE)
		world_switch->vf_timeout |= 1 << (world_switch->curr_idx_vf);

	oss_signal_event(world_switch->manual.switch_event);

	return 0;
}

static int amdgv_sched_manual_switch_work_thread(void *context)
{
	struct amdgv_sched_world_switch *world_switch =
		(struct amdgv_sched_world_switch *)context;

	while (!oss_thread_should_stop(world_switch->manual.switch_thread)) {
		amdgv_sched_manual_switch_process(context);

		/* Wait for the next event */
		oss_wait_event(world_switch->manual.switch_event, 0);
	}

	return 0;
}

static int amdgv_sched_manual_switch_add_vf(struct amdgv_adapter *adapt,
						struct amdgv_sched_world_switch *world_switch,
						uint32_t idx_vf)
{
	struct amdgv_sched_active_vf_entry *entry;
	uint32_t time_slice, hw_sched_id;
	int ret = 0;

	time_slice = adapt->sched.array_vf[idx_vf].time_slice[world_switch->sched_block];
	if (time_slice == 0) {
		AMDGV_DEBUG("%s time_slice is 0\n", amdgv_idx_to_str(idx_vf));
		/* There is no time_slice allowed for this VF!
		 * This not an ERROR! consistent with old behavior
		 * - Don't add this VF to manual scheduling
		 */
		return 0;
	}

	oss_mutex_lock(world_switch->manual.switching_lock);

	entry = &world_switch->manual.array_vf[idx_vf];

	/* add idx_vf to active_vfs */
	set_to_active_vf(idx_vf);

	if (world_switch->manual.fairness_mode && idx_vf != AMDGV_PF_IDX) {
		AMDGV_DEBUG("fairness_mode=true. No need to add %s to "
				"manual_scheduling on sched_block=%d\n",
				amdgv_idx_to_str(idx_vf), world_switch->sched_block);
		entry->dummy_vf = false;
		entry->time_slice = time_slice;
		goto out;
	}

	AMDGV_DEBUG("add %s to manual_sched (time_quanta=%d (ms))\n", amdgv_idx_to_str(idx_vf),
			(uint8_t)(time_slice / 1000));

	AMDGV_INIT_LIST_HEAD(&entry->list);
	entry->idx_vf = idx_vf;
	entry->time_slice = time_slice;
	entry->dummy_vf = false;

	if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX &&
		entry->idx_vf != AMDGV_PF_IDX)
		entry->skip_cnt = 0;

	/* add new vf to the tail of active vf list */
	amdgv_list_add_tail(&entry->list, &world_switch->manual.active_vf_list);

out:
	for_each_id(hw_sched_id, world_switch->hw_sched_mask)
		amdgv_gpuiov_save_rlcv_state(adapt, idx_vf,  hw_sched_id);

	oss_mutex_unlock(world_switch->manual.switching_lock);

	/* start world switch processing immediately, even on dummy VF */
	oss_signal_event(world_switch->manual.switch_event);

	return ret;
}

static int amdgv_sched_manual_switch_remove_vf(struct amdgv_adapter *adapt,
						   struct amdgv_sched_world_switch *world_switch,
						   uint32_t idx_vf)
{
	int ret = 0;
	struct amdgv_sched_active_vf_entry *entry;
	uint32_t hw_sched_id;

	oss_mutex_lock(world_switch->manual.switching_lock);

	entry = &world_switch->manual.array_vf[idx_vf];

	if (AMDGV_IS_IDX_INVALID(entry->idx_vf)) {
		AMDGV_WARN("%s is invalid and cannot be removed\n",
			   amdgv_idx_to_str(entry->idx_vf));
		ret = AMDGV_FAILURE;
		goto out;
	}

	/* if the vf to be removed is the current active vf */
	if (world_switch->curr_idx_vf == entry->idx_vf &&
		world_switch->curr_vf_state == AMDGV_VF_CONTEXT_LOADED) {
		/* save the current active vf's context */
		if (amdgv_save_world_context(adapt, world_switch) != 0) {
			/* queue SCHED_RESET_VF to event process to reset vf */
			amdgv_sched_queue_event(adapt, world_switch->curr_idx_vf,
						AMDGV_EVENT_SCHED_RESET_VF,
						world_switch->sched_block);

			ret = AMDGV_FAILURE;
		}
	}

	if (world_switch->manual.fairness_mode && idx_vf != AMDGV_PF_IDX) {
		AMDGV_DEBUG("fairness_mode=true. No need to remove %s from "
				"manual_scheduling on sched_block=%d\n",
				amdgv_idx_to_str(idx_vf), world_switch->sched_block);
		entry->dummy_vf = true;
		goto out;
	}

	AMDGV_DEBUG("remove %s from manual_scheduling on sched_block=%d\n",
			amdgv_idx_to_str(idx_vf), world_switch->sched_block);

	amdgv_list_del(&entry->list);

	world_switch->vf_inited &= ~(1 << idx_vf);

	if (entry->idx_vf != AMDGV_PF_IDX)
		entry->idx_vf = AMDGV_INVALID_IDX_VF;

out:
	for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
		amdgv_gpuiov_save_rlcv_state(adapt, idx_vf, hw_sched_id);
	}

	oss_mutex_unlock(world_switch->manual.switching_lock);

	/* wake up the world-switch event after removing a VF */
	oss_signal_event(world_switch->manual.switch_event);

	return ret;
}

static int
amdgv_sched_manual_switch_update_time_slice(struct amdgv_adapter *adapt,
						struct amdgv_sched_world_switch *world_switch,
						uint32_t idx_vf)
{
	struct amdgv_sched_active_vf_entry *entry;
	uint32_t time_slice;

	oss_mutex_lock(world_switch->manual.switching_lock);

	entry = &world_switch->manual.array_vf[idx_vf];
	time_slice = adapt->sched.array_vf[idx_vf].time_slice[world_switch->sched_block];

	if (AMDGV_IS_IDX_INVALID(entry->idx_vf))
		goto out;

	entry->time_slice = time_slice;
out:
	oss_mutex_unlock(world_switch->manual.switching_lock);
	return 0;
}

static int amdgv_sched_manual_switch_init(struct amdgv_adapter *adapt,
					  struct amdgv_sched_world_switch *world_switch,
					  enum amdgv_sched_mode sched_mode)
{
	int i;
	struct amdgv_sched_active_vf_entry *entry;

	world_switch->use_active_status = false;

	world_switch->manual.switching_lock = oss_mutex_init();
	if (world_switch->manual.switching_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		return AMDGV_FAILURE;
	}

	world_switch->manual.timer = oss_timer_init_ex(amdgv_sched_manual_switch_timer_isr,
							   (void *)world_switch, adapt->dev);
	if (world_switch->manual.timer == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_TIMER_FAIL, 0);
		goto timer_failed;
	}

	world_switch->manual.switch_event = oss_event_init();
	if (world_switch->manual.switch_event == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_EVENT_FAIL, 0);
		goto event_failed;
	}

	AMDGV_INIT_LIST_HEAD(&world_switch->manual.active_vf_list);
	world_switch->manual.adapt = adapt;

	for (i = 0; i < AMDGV_MAX_VF_SLOT; i++)
		world_switch->manual.array_vf[i].idx_vf = AMDGV_INVALID_IDX_VF;

	if (sched_mode != AMDGV_SCHED_FAIRNESS) {
		world_switch->manual.fairness_mode = false;

		entry = &world_switch->manual.array_vf[AMDGV_PF_IDX];
		AMDGV_INIT_LIST_HEAD(&entry->list);
		entry->idx_vf = AMDGV_PF_IDX;
	} else {
		world_switch->manual.fairness_mode = true;

		/* add all enabled VF to active vf list */
		for (i = 0; i < adapt->num_vf; i++) {
			if (world_switch->allowed_vf_assignment & (1 << i)) {
				entry = &world_switch->manual.array_vf[i];
				AMDGV_INIT_LIST_HEAD(&entry->list);
				entry->idx_vf = i;
				entry->time_slice = amdgv_sched_default_gfx_time_slice(
					adapt, adapt->sched.num_vf_per_gfx_sched);
				entry->start_ts = 0;
				entry->beyond_time_cycle = 0;
				entry->dummy_vf = true;
				entry->init_time = oss_get_time_stamp();
				entry->total_time = 0;
				entry->skip_next_punish = false;
				amdgv_list_add_tail(&entry->list,
							&world_switch->manual.active_vf_list);
			}
		}

		/* init PF, but don't add to active list as PF is not
		 * in fairness group
		 * */
		entry = &world_switch->manual.array_vf[AMDGV_PF_IDX];
		AMDGV_INIT_LIST_HEAD(&entry->list);
		entry->idx_vf = AMDGV_PF_IDX;
		entry->time_slice = amdgv_sched_default_gfx_time_slice(
			adapt, adapt->sched.num_vf_per_gfx_sched);
		entry->start_ts = 0;
		entry->beyond_time_cycle = 0;
		entry->dummy_vf = false;
		entry->init_time = oss_get_time_stamp();
		entry->total_time = 0;
		entry->skip_next_punish = false;

		AMDGV_INFO("enable fair scheduling mode for %s scheduler\n",
			   amdgv_sched_block_to_name(world_switch->sched_block));
	}

	world_switch->vf_inited = 0;

	world_switch->vf_busy_status = 0;
	world_switch->vf_timeout = 0;
	world_switch->hliquid_min_ts = 0;

	world_switch->manual.self_switch_trigger = false;

	if (world_switch->sched_mode == AMDGV_SCHED_HYBRID_LIQUID_MODE) {
		AMDGV_INFO("hybrid liquid mode enabled\n");
		amdgv_schedule_vfs = amdgv_schedule_vfs_liquid;
	}
	else
		amdgv_schedule_vfs = amdgv_schedule_vfs_default;

	world_switch->manual.switch_thread =
		oss_create_thread(amdgv_sched_manual_switch_work_thread, (void *)world_switch,
				  "sched_switch_thread");
	if (world_switch->manual.switch_thread == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_THREAD_FAIL, 0);
		goto failed;
	}

	return 0;

failed:
	oss_event_fini(world_switch->manual.switch_event);
	world_switch->manual.switch_event = OSS_INVALID_HANDLE;

event_failed:
	oss_timer_close(world_switch->manual.timer);
	world_switch->manual.timer = OSS_INVALID_HANDLE;

timer_failed:
	oss_mutex_fini(world_switch->manual.switching_lock);
	world_switch->manual.switching_lock = OSS_INVALID_HANDLE;

	return AMDGV_FAILURE;
}

static void amdgv_sched_manual_switch_fini(struct amdgv_adapter *adapt,
					   struct amdgv_sched_world_switch *world_switch)
{
	int i;
	struct amdgv_sched_active_vf_entry *entry;

	AMDGV_ASSERT(world_switch->switch_running == false);

	/* Permanently mark the world switch event to avoid deadlocks
	 * if we are in single VF mode
	 */
	oss_signal_event_forever(world_switch->manual.switch_event);
	oss_close_thread(world_switch->manual.switch_thread);
	world_switch->manual.switch_thread = OSS_INVALID_HANDLE;

	oss_event_fini(world_switch->manual.switch_event);
	world_switch->manual.switch_event = OSS_INVALID_HANDLE;

	for (i = 0; i < AMDGV_MAX_VF_SLOT; i++) {
		entry = &world_switch->manual.array_vf[i];
		if (AMDGV_IS_IDX_INVALID(entry->idx_vf))
			continue;

		amdgv_list_del(&entry->list);
		entry->idx_vf = AMDGV_INVALID_IDX_VF;
	}

	if (world_switch->manual.switching_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(world_switch->manual.switching_lock);
		world_switch->manual.switching_lock = OSS_INVALID_HANDLE;
	}

	oss_timer_close(world_switch->manual.timer);
	amdgv_list_del(&world_switch->manual.active_vf_list);
}

int amdgv_sched_manual_switch_clear_time_slice(struct amdgv_sched_world_switch *world_switch, uint32_t idx_vf)
{
	if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX) {
		world_switch->manual.array_vf[idx_vf].beyond_time_cycle = 0;
		world_switch->manual.array_vf[idx_vf].start_ts = 0;
		world_switch->manual.array_vf[idx_vf].last_time_slice = 0;
		world_switch->manual.array_vf[idx_vf].total_time = 0;
		world_switch->manual.array_vf[idx_vf].skip_next_punish = false;
	}

	return 0;
}

int amdgv_sched_world_switch_list_active(struct amdgv_adapter *adapt)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t world_switch_id;
	int ret = 0;

	for (world_switch_id = 0; world_switch_id < AMDGV_MAX_NUM_WORLD_SWITCH;
		 world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		ret |= world_switch->switch_running << world_switch_id;
	}
	return ret;
}

int amdgv_sched_world_switch_sched_mask_start(struct amdgv_adapter *adapt,
						  uint32_t world_switch_mask)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t world_switch_id;

	for (world_switch_id = 0; world_switch_id < AMDGV_MAX_NUM_WORLD_SWITCH;
		 world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		if (!world_switch->enabled || !(world_switch_mask & (1 << world_switch_id)))
			continue;

		world_switch->funcs->start(adapt, world_switch);
	}

	return 0;
}

static int amdgv_sched_manual_switch_start(struct amdgv_adapter *adapt,
					   struct amdgv_sched_world_switch *world_switch)
{
	oss_mutex_lock(world_switch->manual.switching_lock);

	if (world_switch->switch_running) {
		/* Self switch can be triggered in runtime, if that is the case
		 * schedule a world-switch to wake up and re-enable
		 */
		if (adapt->sched.num_vf_per_gfx_sched == 1 &&
			world_switch->manual.self_switch_trigger) {
			world_switch->manual.self_switch_trigger = false;
			goto notify;
		}

		/* Otherwise there is nothing else to do */
		goto out;
	}

	if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL) {
		AMDGV_INFO("world_switch type %s, hw_sched_mask=0x%x is abnormal.\n",
				amdgv_sched_block_to_name(world_switch->sched_block),
				world_switch->hw_sched_mask);
		goto out;
	}

	world_switch->switch_running = true;

notify:
	/* start world switching immediately */
	oss_signal_event(world_switch->manual.switch_event);

out:
	oss_mutex_unlock(world_switch->manual.switching_lock);
	return 0;
}

static int amdgv_sched_manual_switch_stop(struct amdgv_adapter *adapt,
					  struct amdgv_sched_world_switch *world_switch)
{
	struct amdgv_sched_active_vf_entry *entry;

	oss_mutex_lock(world_switch->manual.switching_lock);

	if (!world_switch->switch_running &&
		world_switch->curr_vf_state != AMDGV_VF_CONTEXT_LOADED)
		goto out;

	world_switch->switch_running = false;

	/* cancel pending timer */
	oss_timer_pause(world_switch->manual.timer);

	/* no vf has been loaded to GPU */
	if (world_switch->curr_vf_state != AMDGV_VF_CONTEXT_LOADED)
		goto out;

	entry = &world_switch->manual.array_vf[world_switch->curr_idx_vf];
	if (AMDGV_IS_IDX_INVALID(entry->idx_vf)) {
		amdgv_put_error(entry->idx_vf, AMDGV_ERROR_DRIVER_INVALID_VALUE,
				entry->idx_vf);

		AMDGV_ASSERT(0);
	}

	if (amdgv_save_world_context(adapt, world_switch) != 0)
		goto reset_vf;

	if (!AMDGV_IS_IDX_INVALID(entry->idx_vf)) {
		/* Don't add the the PF to the active list unless we
		* have enabled USE_PF and the PD is an active function.
		* This can happen if the PF was being scheduled during
		* idle or dummy function.
		*/
		if ((entry->idx_vf != AMDGV_PF_IDX) ||
			((adapt->flags & AMDGV_FLAG_USE_PF) && is_active_vf(AMDGV_PF_IDX)))
			amdgv_list_move_tail(&entry->list, &world_switch->manual.active_vf_list);
	}
out:
	oss_mutex_unlock(world_switch->manual.switching_lock);
	return 0;

reset_vf:
	/* Reset will be handled after this function,
	 * => should not queue EVENT_SCHED_RESET_VF or any event
	 */

	oss_mutex_unlock(world_switch->manual.switching_lock);
	return AMDGV_FAILURE;
}

static int amdgv_sched_manual_switch_set_vf_num(struct amdgv_adapter *adapt,
						struct amdgv_sched_world_switch *world_switch,
						uint32_t num_vf)
{
	uint32_t i;
	struct amdgv_sched_active_vf_entry *entry;

	if (world_switch->manual.fairness_mode) {
		/* remove all vf from active vf list,
		 * should not reset PF when set_vf_num
		 */
		for (i = 0; i < adapt->max_num_vf; i++) {
			entry = &world_switch->manual.array_vf[i];
			if (AMDGV_IS_IDX_INVALID(entry->idx_vf))
				continue;
			amdgv_list_del(&entry->list);
			entry->idx_vf = AMDGV_INVALID_IDX_VF;
		}

		/* add all enabled VF to active vf list */
		for (i = 0; i < num_vf; i++) {
			if (world_switch->allowed_vf_assignment & (1 << i)) {
				entry = &world_switch->manual.array_vf[i];
				AMDGV_INIT_LIST_HEAD(&entry->list);
				entry->idx_vf = i;
				entry->time_slice = amdgv_sched_default_gfx_time_slice(
					adapt, adapt->sched.num_vf_per_gfx_sched);
				entry->start_ts = 0;
				entry->beyond_time_cycle = 0;
				entry->dummy_vf = true;
				entry->init_time = oss_get_time_stamp();
				entry->total_time = 0;
				entry->skip_next_punish = false;
				amdgv_list_add_tail(&entry->list,
							&world_switch->manual.active_vf_list);
			}
		}
	}

	return 0;
}

const struct amdgv_sched_world_switch_funcs amdgv_sched_manual_switch_funcs = {
	.init = amdgv_sched_manual_switch_init,
	.fini = amdgv_sched_manual_switch_fini,
	.start = amdgv_sched_manual_switch_start,
	.stop = amdgv_sched_manual_switch_stop,
	.add_vf = amdgv_sched_manual_switch_add_vf,
	.remove_vf = amdgv_sched_manual_switch_remove_vf,
	.update_time_slice = amdgv_sched_manual_switch_update_time_slice,
	.set_vf_num = amdgv_sched_manual_switch_set_vf_num,
};

/**********************************************************************
 * World switch auto functions
 **********************************************************************/
static int amdgv_sched_auto_switch_add_vf(struct amdgv_adapter *adapt,
					  struct amdgv_sched_world_switch *world_switch,
					  uint32_t idx_vf)
{
	struct amdgv_vf_device *entry;
	uint32_t time_quanta_option;
	uint8_t vf_quanta; /* ms */
	int vf_quanta_index = -1;
	int i;
	int ret = 0;
	uint32_t hw_sched_id = 0;
	enum amdgv_sched_block sched_block = world_switch->sched_block;

	/* select "time_quanta_index" for this VF (based on time_slice) */
	entry = &adapt->array_vf[idx_vf];

	if (entry->time_slice[sched_block] == 0) {
		vf_quanta_index = 0;
	} else {
		time_quanta_option = adapt->time_quanta_option[sched_block];
		/* use default timeslice if setup vf timeslice being used */
		if ((sched_block == AMDGV_SCHED_BLOCK_GFX) && (adapt->sched.setup_vf_timeslice))
			vf_quanta = ((uint8_t)(GET_GFX_TIME_SLICE(adapt, adapt->sched.num_vf_per_gfx_sched) / 1000));
		else
			vf_quanta = ((uint8_t)(entry->time_slice[sched_block] / 1000));
		if (vf_quanta == 0)
			vf_quanta_index = 0;
		for (i = 0; vf_quanta_index < 0 && i < 4; i++) {
			if (vf_quanta <= (uint8_t)(time_quanta_option & 0xFF))
				vf_quanta_index = i;
			time_quanta_option = time_quanta_option >> 8;
		}
		if (vf_quanta_index < 0) {
			vf_quanta_index = 3;
			time_quanta_option = time_quanta_option & 0x00FFFFFF;
			time_quanta_option = time_quanta_option | ((uint32_t)vf_quanta << 24);
			adapt->time_quanta_option[sched_block] = time_quanta_option;
		}
	}

	/* if "sysfs" opt changes, reconfigure time_quanta_option */
	if (world_switch->time_quanta_option != adapt->time_quanta_option[sched_block]) {
		world_switch->time_quanta_option = adapt->time_quanta_option[sched_block];

		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			amdgv_gpuiov_set_time_quanta_option(adapt, hw_sched_id,
								world_switch->time_quanta_option);
		}
	}

	for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
		/* configure time_quanta_index for this VF */
		amdgv_gpuiov_set_time_quanta_index(adapt, idx_vf, hw_sched_id,
						   vf_quanta_index);
	}

	AMDGV_DEBUG("add %s to auto_scheduling on sched_block=%d\n", amdgv_idx_to_str(idx_vf),
			sched_block);
	set_to_active_vf(idx_vf);
	/* add idx_vf to active_vfs */

	for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
		/* need to toggle GPUIOV cmd AUTO_HW_SWITCH to propagate change */
		amdgv_gpuiov_auto_sched_add_vf(adapt, hw_sched_id, idx_vf);
		if (IS_HW_SCHED_TYPE_GFX(hw_sched_id))
			amdgv_gpuiov_save_rlcv_state(adapt, idx_vf, hw_sched_id);
		adapt->array_vf[idx_vf].auto_run = AMDGV_WS_AUTO_RUN_ENABLED;
	}

	return ret;
}

static int amdgv_sched_auto_switch_remove_vf(struct amdgv_adapter *adapt,
						 struct amdgv_sched_world_switch *world_switch,
						 uint32_t idx_vf)
{
	uint32_t hw_sched_id = 0;

	AMDGV_DEBUG("remove %s from auto_scheduling on sched_block=%d\n",
			amdgv_idx_to_str(idx_vf), world_switch->sched_block);
	world_switch->vf_inited &= ~(1 << idx_vf);

	/* remove idx_vf from active_vfs */
	adapt->array_vf[idx_vf].auto_run = AMDGV_WS_AUTO_RUN_DISABLED;

	for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
		amdgv_gpuiov_auto_sched_remove_vf(adapt, hw_sched_id, idx_vf);
		if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX)
			amdgv_gpuiov_save_rlcv_state(adapt, idx_vf, hw_sched_id);
	}

	return 0;
}

static int amdgv_sched_auto_switch_update_time_slice(struct amdgv_adapter *adapt,
						 struct amdgv_sched_world_switch *world_switch,
						 uint32_t idx_vf)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	uint32_t time_slice = entry->time_slice[world_switch->sched_block];
	int ret = 0;

	if (world_switch->sched_block != AMDGV_SCHED_BLOCK_GFX)
		return 0;

	if (world_switch->sched_mode > AMDGV_SCHED_MAX_HW_SCHED_MODE)
		return AMDGV_FAILURE;

	ret = amdgv_sched_setup_vf_timeslice(adapt, idx_vf, time_slice, world_switch->sched_block);
	if (ret)
		return ret;

	ret = amdgv_sched_world_switch_config_auto_sched_mode(adapt, world_switch);

	return ret;
}

static int amdgv_sched_auto_switch_start(struct amdgv_adapter *adapt,
					 struct amdgv_sched_world_switch *world_switch)
{
	uint32_t hw_sched_id = 0;
	int ret = 0;

	if (adapt->force_switch_vf_idx < AMDGV_MAX_VF_SLOT) {
		AMDGV_DEBUG("force switch enabled\n");
		if (world_switch->enabled && world_switch->switch_running)
			world_switch->funcs->stop(adapt, world_switch);
		if (world_switch->curr_idx_vf != adapt->force_switch_vf_idx &&
			amdgv_sched_world_context_load(adapt, adapt->force_switch_vf_idx, world_switch)) {
			ret = AMDGV_FAILURE;
			goto failed;
		}
		return 0;
	}

	if (world_switch->switch_running) {
		if ((world_switch->auto_sched.self_switch_trigger == DEFAULT_DISABLE) || (adapt->sched.num_vf_per_gfx_sched != 1))
			return 0;
	}

	if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL) {
		AMDGV_INFO("world_switch type %s, hw_sched_mask=0x%x is abnormal.\n",
				amdgv_sched_block_to_name(world_switch->sched_block),
				world_switch->hw_sched_mask);
		return 0;
	}

	/* firmware will send interrupt to pf,
	 * if fails to start auto switch.
	 */
	for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
		if (IS_HW_SCHED_TYPE_GFX(hw_sched_id)) {
			/* Handling with 1vf self-switch case
			 * case 1: flag is DEFAULT_DISABLE -> do nothing and skip
			 * case 2: flag is TRIGGER_ENABLE -> reconfig time quanta
			 * case 3: flag is TRIGGER_DISABLE -> do SAVE(DISABLE AUTO_SCHED) first and then reconfig time quanta
			 */
			if (adapt->sched.num_vf_per_gfx_sched == 1 && world_switch->auto_sched.self_switch_trigger != DEFAULT_DISABLE) {
				if (world_switch->auto_sched.self_switch_trigger == TRIGGER_DISABLED)
					adapt->sched.hw_state_machine[hw_sched_id].goto_state(adapt, -1, hw_sched_id, AMDGV_VF_CONTEXT_SAVED);
				amdgv_sched_setup_vf_timeslice(adapt, 0, GET_GFX_TIME_SLICE(adapt, adapt->sched.num_vf_per_gfx_sched), AMDGV_SCHED_BLOCK_GFX);
				world_switch->auto_sched.self_switch_trigger = DEFAULT_DISABLE;
			}
			/* Always reconfig auto sched mode so that RLCV won't need to save/restore config.
			 * Time quanta will be restored after FLR so we don't need to reconfig it here.
			 */
			ret = amdgv_hw_sched_state_pause(adapt, -1, hw_sched_id);
			if (ret)
				goto failed;
			ret = amdgv_gpuiov_config_auto_sched_mode(adapt, hw_sched_id, world_switch->sched_mode);
			if (ret)
				goto failed;
		}

		/*
		* amdgv_hw_sched_state_run_auto() needs a valid VF id to start but the
		* calling function does not provide one yet.
		* Replace the VF Id with -1 for now so that the State Machine knows
		* that no VF was provided.
		*
		* When MMSCH firmware properly supports it, we can use a valid VF Id
		*/
		ret = amdgv_hw_sched_state_run_auto(adapt, -1, hw_sched_id);
		if (ret)
			goto failed;
	}

	world_switch->switch_running = true;

	return 0;

failed:
	/* Host driver should handle WS failure if auto schedule is not started properly */
	amdgv_sched_queue_event(adapt, world_switch->curr_idx_vf, AMDGV_EVENT_SCHED_RESET_VF,
		world_switch->sched_block);

	return ret;
}

static int amdgv_sched_auto_switch_stop(struct amdgv_adapter *adapt,
					struct amdgv_sched_world_switch *world_switch)
{
	int ret = 0;
	uint32_t hw_sched_id = 0;
	uint32_t curr_idx_vf = AMDGV_INVALID_IDX_VF;

	if (!world_switch->switch_running &&
		world_switch->curr_vf_state != AMDGV_VF_CONTEXT_LOADED)
		return 0;

	if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL) {
		amdgv_sched_world_context_get_curr_vf(adapt, world_switch, &curr_idx_vf);
		world_switch->curr_idx_vf = curr_idx_vf;
		for_each_id(hw_sched_id, world_switch->hw_sched_mask)
			adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id = curr_idx_vf;

		goto out;
	}

	/* When disable auto-scheduling is issued,
	 * MMSCH will respond to GIM right away.
	 * If the engine was running a job, MMSCH will let it complete,
	 * then idle/save (or timeout and trigger FLR).
	 * MMSCH will not schedule any more jobs on the engine unless GIM sends
	 *  the RUN cmd for a specific VF or re-enables auto-scheduling.
	 *
	 * So if GIM wants to make sure the engine is idle after
	 * disabling auto-sched, it should issue IDLE cmd after it.
	 * MMSCH will not respond to IDLE cmd until the engine is idle.
	 */
	for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
		ret = amdgv_hw_sched_state_pause(adapt, -1, hw_sched_id);
		if (ret) {
			world_switch->curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
			goto out;
		}
	}

	world_switch->curr_vf_state = AMDGV_VF_CONTEXT_SAVED;
out:
	amdgv_sched_stop_record_vf_time(adapt, world_switch, world_switch->curr_idx_vf);

	world_switch->switch_running = false;

	return ret;
}

static int amdgv_sched_auto_switch_init(struct amdgv_adapter *adapt,
					struct amdgv_sched_world_switch *world_switch,
					enum amdgv_sched_mode sched_mode)
{
	world_switch->use_active_status = true;
	world_switch->auto_sched.self_switch_trigger = DEFAULT_DISABLE;
	return 0;
}

static void amdgv_sched_auto_switch_fini(struct amdgv_adapter *adapt,
					 struct amdgv_sched_world_switch *world_switch)
{
	return;
}

const struct amdgv_sched_world_switch_funcs amdgv_sched_auto_switch_funcs = {
	.init = amdgv_sched_auto_switch_init,
	.fini = amdgv_sched_auto_switch_fini,
	.start = amdgv_sched_auto_switch_start,
	.stop = amdgv_sched_auto_switch_stop,
	.add_vf = amdgv_sched_auto_switch_add_vf,
	.remove_vf = amdgv_sched_auto_switch_remove_vf,
	.update_time_slice = amdgv_sched_auto_switch_update_time_slice,
};

/**********************************************************************
 * World switch high level APIs
 **********************************************************************/
static void amdgv_sched_world_switch_clear_vf_assignment(struct amdgv_adapter *adapt)
{
	uint32_t i;
	struct amdgv_sched_world_switch *world_switch;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		world_switch = &adapt->sched.world_switch[i];
		world_switch->allowed_vf_assignment = 0;
	}
}

int amdgv_sched_world_switch_remap_vf_assignment(struct amdgv_adapter *adapt)
{
	uint32_t idx_part;
	struct amdgv_sched_world_switch *world_switch = NULL;
	uint32_t world_switch_id;
	bool hit = false;

	amdgv_sched_world_switch_clear_vf_assignment(adapt);

	for (idx_part = 0; idx_part < adapt->sched.num_spatial_partitions; idx_part++) {
		hit = false;
		for (world_switch_id = 0; world_switch_id < adapt->sched.num_world_switch;
			 world_switch_id++) {
			world_switch = &adapt->sched.world_switch[world_switch_id];

			if (world_switch->hw_sched_mask &
				adapt->sched.spatial_part[idx_part].hw_sched_mask) {
				world_switch->allowed_vf_assignment |=
					adapt->sched.spatial_part[idx_part].idx_vf_mask;
				hit = true;
			}
		}

		if (!hit) {
			AMDGV_ERROR("Unable to remap VFs to world_switch after VF Number Change!\n");
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

static int amdgv_sched_world_switch_init_sw_config(struct amdgv_adapter *adapt)
{
	uint32_t hw_sched_id;
	uint32_t i, idx_part;
	struct amdgv_sched_world_switch *world_switch = NULL;
	uint32_t start_idx = 0;
	uint32_t idx = 0;
	bool hit = false;

	oss_memset(adapt->sched.world_switch, 0,
		   sizeof(struct amdgv_sched_world_switch) * AMDGV_MAX_NUM_WORLD_SWITCH);

	for (idx_part = 0; idx_part < adapt->sched.num_spatial_partitions; idx_part++) {
		if (idx >= AMDGV_MAX_NUM_WORLD_SWITCH) {
			AMDGV_ERROR("world_switch array is full. Unable to createanymore logical schedulers\n");
			return AMDGV_FAILURE;
		}

		for_each_id(hw_sched_id, adapt->sched.spatial_part[idx_part].hw_sched_mask) {
			hit = false;

			//Special case. Check for shared world_switch across partitions.
			for (i = 0; i < AMDGV_MAX_NUM_WORLD_SWITCH; i++) {
				world_switch = &adapt->sched.world_switch[i];

				if (!world_switch->enabled) {
					continue;
				}

				/*
				 * Partition explicitly requested this hw resource.
				 * Since world_switch is already created,
				 * just add partition VFs to allowed list.
				*/
				if (world_switch->hw_sched_mask & (1 << hw_sched_id)) {
					world_switch->allowed_vf_assignment |=
						adapt->sched.spatial_part[idx_part].idx_vf_mask;
					hit = true;
					break;
				}
			}

			if (!hit) {
				for (i = start_idx; i < AMDGV_MAX_NUM_WORLD_SWITCH; i++) {
					world_switch = &adapt->sched.world_switch[i];

					if (!world_switch->enabled) {
						continue;
					}

					//If world_switch is same type as request hw_sched, add it.
					if ((world_switch->sched_block ==
						 adapt->gpuiov.ctrl_blocks[hw_sched_id]
							 .sched_block)) {
						world_switch->hw_sched_mask |=
							(1 << hw_sched_id);
						hit = true;
						break;
					}
				}
			}

			//If not world_switch found, create a new entry
			if (!hit) {
				world_switch = &adapt->sched.world_switch[idx];
				world_switch->enabled = true;

				world_switch->allowed_vf_assignment |=
					adapt->sched.spatial_part[idx_part].idx_vf_mask;
				world_switch->hw_sched_mask |= (1 << hw_sched_id);
				world_switch->sched_block =
					adapt->gpuiov.ctrl_blocks[hw_sched_id]
						.sched_block;
				world_switch->curr_idx_vf = AMDGV_PF_IDX;
				world_switch->curr_vf_state = AMDGV_VF_CONTEXT_CLEAR;
				world_switch->time_quanta_option = 0;
				world_switch->use_active_status = false;

				if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode == AMDGV_SCHED_FAIRNESS &&
					adapt->sched.enable_bulk_goto_state)
					world_switch->bulk_goto_state = world_switch_bulk_goto_state_manual;

				idx++;
			}
		}

		/*
		 * Move next partition's world_switch start index after
		 * the last configured world_switch entry.
		 * Except for special cases, world_switch should not be shared across partitions.
		 */
		start_idx = idx;
	}

	adapt->sched.num_world_switch = idx;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		if (adapt->sched.world_switch[i].enabled) {
			AMDGV_INFO("Created world_switch[%d] type=%s hw_sched_mask=0x%x\n", i,
				   amdgv_sched_block_to_name(
					   adapt->sched.world_switch[i].sched_block),
				   adapt->sched.world_switch[i].hw_sched_mask);
		}
	}

	return 0;
}

int amdgv_sched_world_switch_init(struct amdgv_adapter *adapt)
{
	int i;
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	struct amdgv_gpuiov_ctrl_block *hw_sched_table = adapt->gpuiov.ctrl_blocks;

	amdgv_sched_world_switch_init_sw_config(adapt);

	adapt->bp_mode = adapt->opt.bp_debug_mode;
	if (adapt->bp_mode != AMDGV_BP_MODE_DISABLE) {
		AMDGV_INFO("Host Driver Break Point Mode %d\n", adapt->bp_mode);
	}

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		world_switch = &adapt->sched.world_switch[i];
		if (world_switch->enabled == false)
			break;

		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			world_switch->sched_mode = hw_sched_table[hw_sched_id].sched_mode;
			if (world_switch->sched_mode <= AMDGV_SCHED_MAX_HW_SCHED_MODE) {
				world_switch->funcs = &amdgv_sched_auto_switch_funcs;
				adapt->sched.hw_state_machine[hw_sched_id].mode =
					AMDGV_WS_MODE_AUTO;
			} else {
				world_switch->funcs = &amdgv_sched_manual_switch_funcs;
				adapt->sched.hw_state_machine[hw_sched_id].mode =
					AMDGV_WS_MODE_MANUAL;
			}
		}

		if (world_switch->funcs->init(adapt, world_switch, world_switch->sched_mode))
			goto fail;
	}

	/* Initialize the World Switch State Machine */
	amdgv_hw_sched_init(adapt);

	return 0;

fail:
	i--;
	for (; i >= 0; i--) {
		world_switch = &adapt->sched.world_switch[i];
		world_switch->funcs->fini(adapt, world_switch);
	}
	return AMDGV_FAILURE;
}

void amdgv_sched_world_switch_fini(struct amdgv_adapter *adapt)
{
	struct amdgv_sched_world_switch *world_switch;
	int i;

	for (i = 0; i < AMDGV_MAX_NUM_HW_SCHED; i++) {
		world_switch = &adapt->sched.world_switch[i];

		if (!world_switch->enabled)
			continue;

		world_switch->funcs->fini(adapt, world_switch);
		world_switch->enabled = false;
	}

	return;
}

int amdgv_sched_world_switch_start(struct amdgv_adapter *adapt,
				   struct amdgv_sched_world_switch *world_switch)
{
	if (world_switch->enabled)
		world_switch->funcs->start(adapt, world_switch);

	return 0;
}

int amdgv_sched_world_switch_stop(struct amdgv_adapter *adapt,
				  struct amdgv_sched_world_switch *world_switch)
{
	if (world_switch->enabled)
		world_switch->funcs->stop(adapt, world_switch);

	return 0;
}

int amdgv_sched_world_switch_shutdown_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
					 struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return 0;

	if (amdgv_logical_sched_state_shutdown(adapt, idx_vf, world_switch)) {
		AMDGV_ERROR("Cannot shutdown VF%d on sched_block %s\n",
			idx_vf, amdgv_sched_block_to_name(world_switch->sched_block));
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_sched_world_switch_add_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
					struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return 0;

	return world_switch->funcs->add_vf(adapt, world_switch, idx_vf);
}

int amdgv_sched_world_switch_remove_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return 0;

	return world_switch->funcs->remove_vf(adapt, world_switch, idx_vf);
}

int amdgv_sched_world_switch_toggle_skip_next_punish(
	struct amdgv_adapter *adapt, uint32_t idx_vf,
	struct amdgv_sched_world_switch *world_switch, bool enable)
{
	if (world_switch->sched_mode == AMDGV_SCHED_FAIRNESS)
		world_switch->manual.array_vf[idx_vf].skip_next_punish = enable;

	return 0;
}

int amdgv_sched_world_switch_set_vf_num(struct amdgv_adapter *adapt, uint32_t num_vf,
					struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled || !world_switch->funcs->set_vf_num)
		return 0;

	AMDGV_ASSERT(world_switch->switch_running == false);

	world_switch->funcs->set_vf_num(adapt, world_switch, num_vf);

	return 0;
}

int amdgv_sched_world_switch_update_time_slice(struct amdgv_adapter *adapt, uint32_t idx_vf,
						   struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled || !world_switch->funcs->update_time_slice)
		return 0;

	return world_switch->funcs->update_time_slice(adapt, world_switch, idx_vf);
}

int amdgv_sched_world_switch_config_auto_sched_mode(
	struct amdgv_adapter *adapt, struct amdgv_sched_world_switch *world_switch)
{
	uint32_t hw_sched_id = 0;
	bool switch_running = world_switch->switch_running;
	int ret = 0;

	if (!world_switch->enabled)
		return 0;

	for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
		if (world_switch->sched_mode <= AMDGV_SCHED_MAX_HW_SCHED_MODE) {
			if (switch_running) {
				ret = amdgv_hw_sched_state_pause(adapt, -1, hw_sched_id);
				world_switch->switch_running = false;
			}
			if (!ret)
				ret = amdgv_gpuiov_config_auto_sched_mode(adapt, hw_sched_id,
								world_switch->sched_mode);
			if (!ret && switch_running) {
				ret = amdgv_hw_sched_state_run_auto(adapt, -1, hw_sched_id);
				world_switch->switch_running = true;
			}
		}
	}

	return ret;
}

int amdgv_sched_world_context_get_curr_vf(struct amdgv_adapter *adapt,
					  struct amdgv_sched_world_switch *world_switch,
					  uint32_t *curr_idx_vf)
{
	uint32_t tmp_curr_state;
	uint32_t hw_sched_id = 0;

	if (!world_switch->enabled)
		return AMDGV_FAILURE;

	if (IS_SCHED_BLOCK_MM(world_switch->sched_block)) {
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			if (adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state !=
				AMDGV_ENABLE_AUTO_HW_SWITCH)
				continue;

			amdgv_sched_world_context_get_hw_curr_state(adapt, hw_sched_id,
									&tmp_curr_state);
			if (tmp_curr_state != AMDGV_VF_CONTEXT_ABNORMAL) {
				adapt->sched.hw_state_machine[hw_sched_id].goto_state(
					adapt, -1, hw_sched_id, AMDGV_SAVE_GPU_STATE);
				AMDGV_INFO(
					"Stop auto scheduler before query current vf for hw_sched_id=%d",
					hw_sched_id);
			}
		}
	}

	*curr_idx_vf = world_switch->curr_idx_vf;

	if (world_switch->use_active_status && world_switch->switch_running) {
		//A logical auto scheduler is either a collection of hw schedulers with a single assigned VF
		//Or a single HW scheduler with multiple VFs. In both scenarios, the active func ID across
		//the entire logical sched will be synchronized. We can query just a single engine.
		hw_sched_id = amdgv_ffs(world_switch->hw_sched_mask) - 1;
		if (hw_sched_id != 0xFFFFFFFF)
			return amdgv_gpuiov_get_active_vf_idx(adapt, hw_sched_id, curr_idx_vf);
	}

	return 0;
}

int amdgv_sched_world_context_get_hw_curr_state(struct amdgv_adapter *adapt,
						uint32_t hw_sched_id, uint32_t *curr_vf_state)
{
	struct amdgv_sched_world_switch *world_switch;

	if (amdgv_sched_get_world_switch_by_hw_sched_id(adapt, hw_sched_id, &world_switch))
		return AMDGV_FAILURE;

	if (!world_switch->enabled)
		return AMDGV_FAILURE;

	*curr_vf_state = world_switch->curr_vf_state;

	if (world_switch->use_active_status && world_switch->switch_running) {
		int ret;
		uint8_t status;

		ret = amdgv_gpuiov_get_active_vf_status(adapt, hw_sched_id, &status);
		if (ret)
			*curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
		else if (status == AMDGV_ACTIVE_FCN_STALLED)
			*curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
		else if (status == AMDGV_ACTIVE_FCN_IDLE)
			*curr_vf_state = AMDGV_VF_CONTEXT_CLEAR;
		else if (status == AMDGV_ACTIVE_FCN_IDLING)
			*curr_vf_state = AMDGV_VF_CONTEXT_CLEAR;
		else if (status == AMDGV_ACTIVE_FCN_SAVE)
			*curr_vf_state = AMDGV_VF_CONTEXT_SAVED;
		else if (status == AMDGV_ACTIVE_FCN_LOAD)
			*curr_vf_state = AMDGV_VF_CONTEXT_LOADED;
		else if (status == AMDGV_ACTIVE_FCN_ACTIVE)
			*curr_vf_state = AMDGV_VF_CONTEXT_LOADED;
		else
			*curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
	}

	return 0;
}

int amdgv_sched_world_context_switch_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return 0;

	return amdgv_switch_world_context(adapt, idx_vf, world_switch);
}

int amdgv_sched_world_context_one_time_loop(struct amdgv_adapter *adapt,
						struct amdgv_sched_world_switch *world_switch)
{
	int ret;
	uint64_t time_slice;
	struct amdgv_sched_active_vf_entry *entry;

	if (amdgv_list_empty(&world_switch->manual.active_vf_list))
		return 0;

	amdgv_list_for_each_entry(entry, &world_switch->manual.active_vf_list,
				   struct amdgv_sched_active_vf_entry, list) {
		if (entry->dummy_vf)
			continue;

		if (adapt->array_vf[entry->idx_vf].unshutdown)
			continue;

		/* only if Windows PF participates in world switch, give 6ms to all the VF's */
		if (adapt->flags & AMDGV_FLAG_USE_PF) {
			time_slice = DEFAULT_GFX_TIME_SLICE;
		} else {
			time_slice = amdgv_sched_world_switch_calculate_time_slice(
				adapt, world_switch, entry->idx_vf);
			/* Give VF the minimum time slice instead of skipping */
			if (time_slice == 0)
				time_slice = world_switch->manual.array_vf[entry->idx_vf]
							 .time_slice /
						 2;
		}

		ret = amdgv_sched_world_context_load(adapt, entry->idx_vf, world_switch);
		if (ret)
			return ret;

		oss_msleep(time_slice / 1000);

		ret = amdgv_sched_world_context_save(adapt, world_switch);
		if (ret)
			return ret;
	}

	return 0;
}
int amdgv_sched_world_context_init(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return 0;

	return amdgv_init_world_context(adapt, idx_vf, world_switch);
}

int amdgv_sched_world_context_load(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return 0;

	return amdgv_load_world_context(adapt, idx_vf, world_switch);
}

int amdgv_sched_world_context_save(struct amdgv_adapter *adapt,
				   struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return 0;

	return amdgv_save_world_context(adapt, world_switch);
}

void amdgv_sched_world_context_clear_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return;

	AMDGV_ASSERT(!world_switch->switch_running);

	world_switch->curr_vf_state = AMDGV_VF_CONTEXT_CLEAR;
	world_switch->vf_inited &= ~(1 << world_switch->curr_idx_vf);
	amdgv_sched_stop_record_vf_time(adapt, world_switch, idx_vf);
}

/* clear context before whole gpu reset */
void amdgv_sched_world_context_clear_state_rst(struct amdgv_adapter *adapt)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t idx_vf;
	uint32_t world_switch_id;

	/* clear current VF state */
	for (world_switch_id = 0; world_switch_id < AMDGV_MAX_NUM_WORLD_SWITCH;
		 world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		if (!world_switch->enabled)
			continue;

		AMDGV_ASSERT(!world_switch->switch_running);

		world_switch->curr_vf_state = AMDGV_VF_CONTEXT_CLEAR;
	}

	/* clear active VF's state of world_switch */
	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++) {
		if (!is_active_or_suspend_vf(adapt, idx_vf))
			continue;

		for (world_switch_id = 0; world_switch_id < AMDGV_MAX_NUM_WORLD_SWITCH;
			 world_switch_id++) {
			world_switch = &adapt->sched.world_switch[world_switch_id];

			if (!world_switch->enabled)
				continue;

			AMDGV_ASSERT(!world_switch->switch_running);

			world_switch->vf_inited &= ~(1 << idx_vf);
		}
	}
}

int amdgv_sched_world_switch_start_pf(struct amdgv_adapter *adapt)
{
	return amdgv_sched_add_vf(adapt, AMDGV_PF_IDX);
}

bool amdgv_sched_world_context_is_state_ok(struct amdgv_adapter *adapt,
					   struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return true;

	AMDGV_ASSERT(!world_switch->switch_running);

	if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL)
		return false;
	else
		return true;
}

bool amdgv_sched_world_context_all_states_ok(struct amdgv_adapter *adapt)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t world_switch_id = 0;

	for (world_switch_id = 0; world_switch_id < AMDGV_MAX_NUM_WORLD_SWITCH;
		 world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		if (!world_switch->enabled)
			continue;

		AMDGV_ASSERT(!world_switch->switch_running);

		if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL)
			return false;
	}

	return true;
}

int amdgv_sched_world_context_set_vf_abnormal(struct amdgv_adapter *adapt, uint32_t idx_vf,
						  struct amdgv_sched_world_switch *world_switch)
{
	if (!world_switch->enabled)
		return AMDGV_FAILURE;

	world_switch->curr_idx_vf = idx_vf;

	world_switch->curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;

	return 0;
}

int amdgv_sched_world_context_get_abnormal_world_switch(
	struct amdgv_adapter *adapt, struct amdgv_sched_world_switch **world_switch)
{
	uint32_t i;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		*world_switch = &adapt->sched.world_switch[i];

		if (!(*world_switch)->enabled)
			continue;

		if ((*world_switch)->curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL) {
			return 0;
		}
	}

	*world_switch = NULL;
	return 0;
}

void amdgv_sched_setup_self_switch(struct amdgv_adapter *adapt, bool enable)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t world_switch_id = 0;

	for (world_switch_id = 0; world_switch_id < AMDGV_MAX_NUM_WORLD_SWITCH;
		 world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->sched_mode == AMDGV_SCHED_SOLID_MODE)
			world_switch->auto_sched.self_switch_trigger = enable ? TRIGGER_ENABLED : TRIGGER_DISABLED;
		else
			world_switch->manual.self_switch_trigger = true;
	}
	/* Wake up main event thread */
	oss_signal_event(adapt->sched.event);
}

int amdgv_sched_set_time_slice(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint32_t time_slice, enum amdgv_sched_block sched_block)
{
	uint32_t ret = AMDGV_FAILURE;
	uint32_t world_switch_id;
	uint32_t tmp_idx_vf;
	uint32_t idx_vf_mask;
	struct amdgv_sched_world_switch *world_switch;

	/* API call is expected to fail if the logical scheduler
	 * isn't a manual scheduler.
	 * Because lower level world_switch functions will not
	 * return failure, we have to make this check here.
	 */
	if (idx_vf == 0xffffffff)
		idx_vf_mask = 0xffffffff;
	else
		idx_vf_mask = (1 << idx_vf);

	for (world_switch_id = 0; world_switch_id < adapt->sched.num_world_switch;
		 world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		if (!(world_switch->allowed_vf_assignment & idx_vf_mask) ||
			world_switch->sched_block != sched_block)
			continue;

		if (!world_switch->enabled)
			return AMDGV_FAILURE;
	}

	/* Special case, update all VFs, across all spatial partitions */
	if (idx_vf == 0xffffffff) {
		for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++) {
			adapt->array_vf[idx_vf].time_slice[sched_block] = time_slice;
			ret = amdgv_sched_update_time_slice(adapt, sched_block, idx_vf);
			if (ret)
				return ret;
		}
	} else {
		/* Check for fairness for each world_switch of this scheduler type */
		for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
							  adapt, idx_vf, sched_block)) {
			world_switch = &adapt->sched.world_switch[world_switch_id];
			if (world_switch->manual.fairness_mode) {
				/* Update all VFs within this world_switch */
				for_each_id(tmp_idx_vf, world_switch->allowed_vf_assignment) {
					adapt->array_vf[tmp_idx_vf].time_slice[sched_block] =
						time_slice;
					ret = amdgv_sched_update_time_slice(adapt, sched_block,
										tmp_idx_vf);
					if (ret)
						return ret;
				}
			} else if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX) {
				adapt->array_vf[idx_vf].time_slice[sched_block] = time_slice;
				ret = amdgv_sched_update_time_slice(adapt, sched_block,
									idx_vf);
				if (ret)
					return ret;
			} else {
				/* Currently only gfx auto scheduler support set_time_slice */
				return AMDGV_FAILURE;
			}
		}
	}

	return ret;
}

int amdgv_sched_world_switch_reset(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   struct amdgv_sched_world_switch *world_switch)
{
	int ret = 0;
	uint32_t hw_sched_id = 0;
	enum amdgv_sched_block sched_block = world_switch->sched_block;

	if (adapt->reset.saved_rlcv_state && (sched_block == AMDGV_SCHED_BLOCK_GFX)) {
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			ret = amdgv_gpuiov_load_rlcv_state(adapt, idx_vf, hw_sched_id);
			if (ret) {
				AMDGV_ERROR("failed to load RLCV state\n");
				goto out;
			}
		}
	}

	if (adapt->gpuiov.funcs->set_event_notification &&
		(sched_block == AMDGV_SCHED_BLOCK_GFX)) {
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			ret = amdgv_gpuiov_event_notification(adapt, idx_vf, hw_sched_id,
								  AMDGV_EVENT_GFX_FLR, 0);
			if (ret) {
				AMDGV_ERROR("failed to send event notification\n");
				goto out;
			}
		}
	}

	/*
	* After an FLR, RLCV versions 92 and above
	* will leave the GPU in a state similar to SAVE.
	* The State Machine does not know that an FLR has
	* occurred and its internal state must be updated.
	*/
	for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
		adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
			AMDGV_SAVE_GPU_STATE;
		adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id] = AMDGV_SAVE_GPU_STATE;

		if (amdgv_hw_sched_state_shutdown(adapt, idx_vf, hw_sched_id)) {
			amdgv_put_error(idx_vf, AMDGV_ERROR_SCHED_SHUTDOWN_VF_FAIL, idx_vf);
			ret = AMDGV_FAILURE;
			goto out;
		}
	}

out:
	return ret;
}

int amdgv_sched_world_context_sync_abnormal_sched(struct amdgv_adapter *adapt,
						  uint32_t abnormal_idx_vf,
						  struct amdgv_sched_world_switch *world_switch)
{
	int ret;
	uint32_t hw_sched_id;
	uint32_t curr_idx_vf;
	uint16_t curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
	uint8_t status;

	if (!world_switch->hw_sched_mask) {
		AMDGV_ERROR("Invalid logical scheduler\n");
		return 0;
	}

	//MM schedulers are either a collection of hw schedulers with a single assigned VF
	//Or a single HW scheduler with multiple VFs. In both scenarios, the active func ID across
	//the entire logical sched will be synchronized.
	if (world_switch->sched_mode <= AMDGV_SCHED_MAX_HW_SCHED_MODE ||
		((world_switch->hw_sched_mask & (world_switch->hw_sched_mask - 1)) == 0)) {
		AMDGV_DEBUG("HW Scheduler Sync not required\n");
		return 0;
	}

	AMDGV_DEBUG("Start Sync HW schedulers on abnormal logical scheduler\n");

	for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
		ret = amdgv_gpuiov_get_active_vf_status(adapt, hw_sched_id, &status);
		if (ret)
			curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
		else if (status == AMDGV_ACTIVE_FCN_STALLED)
			curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
		else if (status == AMDGV_ACTIVE_FCN_IDLE)
			curr_vf_state = AMDGV_VF_CONTEXT_CLEAR;
		else if (status == AMDGV_ACTIVE_FCN_IDLING)
			curr_vf_state = AMDGV_VF_CONTEXT_CLEAR;
		else if (status == AMDGV_ACTIVE_FCN_SAVE)
			curr_vf_state = AMDGV_VF_CONTEXT_SAVED;
		else if (status == AMDGV_ACTIVE_FCN_LOAD)
			curr_vf_state = AMDGV_VF_CONTEXT_LOADED;
		else if (status == AMDGV_ACTIVE_FCN_ACTIVE)
			curr_vf_state = AMDGV_VF_CONTEXT_LOADED;
		else
			curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;

		amdgv_gpuiov_get_active_vf_idx(adapt, hw_sched_id, &curr_idx_vf);

		if ((curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL) &&
			curr_idx_vf != abnormal_idx_vf) {
			//Multiple engines are in abnormal state on different active VFs.
			//It is not possible to perform sync.
			AMDGV_ERROR(
				"Multiple VFs are in an abnormal state. Cannot perform HW sched sync\n");
			goto failed;
		}

		if (curr_idx_vf == abnormal_idx_vf)
			continue;

		if (curr_vf_state == AMDGV_VF_CONTEXT_LOADED) {
			if (amdgv_hw_sched_state_pause(adapt, curr_idx_vf, hw_sched_id))
				goto failed;
		}

		if (amdgv_hw_sched_state_run(adapt, abnormal_idx_vf, hw_sched_id))
			goto failed;
	}

	AMDGV_DEBUG("Finish Sync HW schedulers on abnormal logical scheduler\n");

	return 0;

failed:
	AMDGV_ERROR("Failed to synchronize all hw schedulers in logical sched block=%s",
			amdgv_sched_block_to_name(world_switch->sched_block));
	return AMDGV_FAILURE;
}

int amdgv_sched_world_switch_signal_vf_idle(struct amdgv_adapter *adapt)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t world_switch_id;

	for (world_switch_id = 0; world_switch_id < adapt->sched.num_world_switch;
		 world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		if (world_switch->sched_block != AMDGV_SCHED_BLOCK_GFX)
			continue;

		if (world_switch->switch_running) {
			oss_timer_pause(world_switch->manual.timer);
			oss_signal_event(world_switch->manual.switch_event);
		}
	}

	return 0;
}

enum amdgv_live_info_status amdgv_sched_ws_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_world_switch *world_switch)
{
	uint32_t idx_live_data, idx_vf, i;
	struct amdgv_sched_world_switch *adapter_world_switch;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		adapter_world_switch = &adapt->sched.world_switch[i];

		world_switch[i].curr_idx_vf =
			adapter_world_switch->curr_idx_vf;
		world_switch[i].curr_vf_state =
			adapter_world_switch->curr_vf_state;

		if (!adapter_world_switch->enabled)
			continue;

		for (idx_live_data = 0; idx_live_data < adapt->num_vf + 1; idx_live_data++) {
			if (idx_live_data >= AMDGV_MAX_VF_LIVE) {
				AMDGV_ERROR("WS export live data error, slot# %u, %u live update slots\n", idx_live_data, AMDGV_MAX_VF_LIVE);
				return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
			}

			idx_vf = idx_live_data;
			if (idx_live_data == adapt->num_vf)
				idx_vf = AMDGV_PF_IDX;
			world_switch[i].vf_inited[idx_live_data] =
				adapter_world_switch->vf_inited &
						(1 << idx_vf) ?
					1 :
					0;
			world_switch[i]
				.manual_array_vf_idx_vf[idx_live_data] =
				adapter_world_switch->manual
					.array_vf[idx_vf]
					.idx_vf;
			world_switch[i]
				.manual_array_vf_dummy_vf[idx_live_data] =
				adapter_world_switch->manual
					.array_vf[idx_vf]
					.dummy_vf;
		}
	}

	world_switch->allow_time_cmd_complete = adapt->gpuiov.allow_time_cmd_complete;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_sched_ws_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_world_switch *world_switch)
{
	uint32_t idx_live_data, idx_vf, i, hw_sched_id;
	struct amdgv_sched_world_switch *adapter_world_switch;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		adapter_world_switch = &adapt->sched.world_switch[i];

		adapter_world_switch->curr_idx_vf =
			world_switch[i].curr_idx_vf;
		adapter_world_switch->curr_vf_state =
			world_switch[i].curr_vf_state;

		if (!adapter_world_switch->enabled)
			continue;

		for (idx_live_data = 0; idx_live_data < adapt->num_vf + 1; idx_live_data++) {
			if (idx_live_data >= AMDGV_MAX_VF_LIVE) {
				AMDGV_ERROR("WS import live data error, slot# %u, %u live update slots\n", idx_live_data, AMDGV_MAX_VF_LIVE);
				return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
			}

			idx_vf = idx_live_data;
			if (idx_live_data == adapt->num_vf)
				idx_vf = AMDGV_PF_IDX;
			adapter_world_switch->vf_inited |=
				world_switch[i].vf_inited[idx_live_data] ?
					(1 << idx_vf) :
					0;
			adapter_world_switch->manual.array_vf[idx_vf]
				.idx_vf =
				world_switch[i]
					.manual_array_vf_idx_vf[idx_live_data];
			adapter_world_switch->manual.array_vf[idx_vf]
				.dummy_vf =
				world_switch[i]
					.manual_array_vf_dummy_vf[idx_live_data];
		}

		for_each_id(hw_sched_id, adapter_world_switch->hw_sched_mask) {
			amdgv_gpuiov_get_time_quanta_option(adapt, hw_sched_id,
				&adapter_world_switch->time_quanta_option);
		}
	}

	adapt->gpuiov.allow_time_cmd_complete = world_switch->allow_time_cmd_complete;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}
