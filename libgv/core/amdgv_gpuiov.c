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

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

#define cmd_allow_time() adapt->gpuiov.allow_time_cmd_complete

static struct amdgv_id_mask_name amdgv_gpuiov_cmd_names[] = {
	{ AMDGV_INIT_GPU, AMDGV_NAME_MASK_HW_ALL, "INIT" },
	{ AMDGV_RUN_GPU, AMDGV_NAME_MASK_HW_ALL, "RUN" },
	{ AMDGV_IDLE_GPU, AMDGV_NAME_MASK_HW_ALL, "IDLE" },
	{ AMDGV_SAVE_GPU_STATE, AMDGV_NAME_MASK_HW_ALL, "SAVE" },
	{ AMDGV_LOAD_GPU_STATE, AMDGV_NAME_MASK_HW_ALL, "LOAD" },
	{ AMDGV_CONTEXT_SWITCH, AMDGV_NAME_MASK_HW_ALL, "CONTEXT SWITCH" },
	{ AMDGV_CONFIG_AUTO_HW_SCHED_MODE, AMDGV_NAME_MASK_HW_ALL,
	  "CONFIG HW_AUTO_SCHED_MODE" },
	{ AMDGV_ENABLE_AUTO_HW_SWITCH, AMDGV_NAME_MASK_HW_ALL, "ENABLE HW_AUTO_SCHED" },
	{ AMDGV_DISABLE_AUTO_HW_SCHED, AMDGV_NAME_MASK_HW_ALL, "DISABLE HW_AUTO_SCHED" },
	{ AMDGV_DISABLE_AUTO_HW_SCHED_AND_SWITCH, AMDGV_NAME_MASK_HW_ALL,
	  "DISABLE HW_AUTO_SCHED AND SWITCH" },
	{ AMDGV_SAVE_RLCV_STATE, AMDGV_NAME_MASK_HW_GFX, "SAVE RLCV STATE" },
	{ AMDGV_LOAD_RLCV_STATE, AMDGV_NAME_MASK_HW_GFX, "LOAD RLCV STATE" },
	{ AMDGV_ENABLE_MMSCH_VFGATE, AMDGV_NAME_MASK_HW_MM, "MMSCH_VFGATE_ENABLE" },
	{ AMDGV_DISABLE_MMSCH_VFGATE, AMDGV_NAME_MASK_HW_MM, "MMSCH_VFGATE_DISABLE" },
	{ AMDGV_CLEAR_VF_STATE, AMDGV_NAME_MASK_HW_ALL, "CLEAR VF STATE" },
	{ AMDGV_SHUTDOWN_GPU, AMDGV_NAME_MASK_HW_ALL, "SHUTDOWN VF" },
	{ AMDGV_EVENT_NOTIFICATION, AMDGV_NAME_MASK_HW_ALL, "EVENT NOTIFICATION" },
	{ AMDGV_TRANSFER_VF_DATA, AMDGV_NAME_MASK_HW_ALL, "TRANSFER VF DATA" },
};

static struct amdgv_id_name amdgv_gpuiov_status_names[] = {
	{ AMDGV_CMD_STATUS_DONE, "DONE/NOT_EXECUTE" },
	{ AMDGV_CMD_STATUS_UNSUPPORTED, "Unsupported Request. CMD not recognized" },
	{ AMDGV_CMD_STATUS_ABORTED, "CMD Aborted. Error processing CMD" },
	{ AMDGV_CMD_STATUS_IDLING_ENGINE, "IDLING_ENGINE" },
	{ AMDGV_CMD_STATUS_SAVING_STATE, "SAVING_STATE" },
	{ AMDGV_CMD_STATUS_LOADING_STATE, "LOADING_STATE" },
	{ AMDGV_CMD_STATUS_ENABLING_ENGINE, "ENABLING_ENGINE" },
	{ AMDGV_CMD_STATUS_INITING_ENGINE, "INITING_ENGINE" },
	{ AMDGV_CMD_STATUS_PENDING_EXECUTE, "PENDING_EXECUTE" },
};

#ifdef DEBUG_WORLD_SWITCH
static int check_state_transition(struct amdgv_adapter *adapt, int next, uint32_t idx_vf,
				  uint32_t hw_sched_id)
{
	int ret = 0;
	int adapter_state = adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state;
	int current = adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id];

	current &= 0xF;
	next &= 0xF;

#ifdef DEBUG_WORLD_SWITCH_VERBOSE
	AMDGV_INFO("[%s] Switch from %s to %s for VF%d.  Adp is currently in %s state\n",
		   amdgv_hw_sched_id_to_name(adapt, hw_sched_id),
		   amdgv_gpuiov_cmd_to_name(adapt, current, hw_sched_id),
		   amdgv_gpuiov_cmd_to_name(adapt, next, hw_sched_id), idx_vf,
		   amdgv_gpuiov_cmd_to_name(adapt, adapter_state, hw_sched_id));
#endif
	if (adapter_state != current) {
		if (!((adapter_state == AMDGV_SAVE_GPU_STATE) &&
		      (current == AMDGV_SHUTDOWN_GPU)) &&
		    !((adapter_state == AMDGV_ENABLE_AUTO_HW_SWITCH) &&
		      (current == AMDGV_RUN_GPU)) &&
		    !((adapter_state == AMDGV_SHUTDOWN_GPU) &&
		      (current == AMDGV_SAVE_GPU_STATE))) {
			AMDGV_ERROR(
				"[%s] VF%d - Bad global state mismatch adp = %s vs cur = %s, switching to %s on %s\n",
				amdgv_hw_sched_id_to_name(adapt, hw_sched_id), idx_vf,
				amdgv_gpuiov_cmd_to_name(adapt, adapter_state, hw_sched_id),
				amdgv_gpuiov_cmd_to_name(adapt, current, hw_sched_id),
				amdgv_gpuiov_cmd_to_name(adapt, next, hw_sched_id),
				amdgv_hw_sched_id_to_name(adapt, hw_sched_id));
		}
	}

	switch (next) {
	case AMDGV_INIT_GPU:
		if (current != AMDGV_SHUTDOWN_GPU)
			ret = AMDGV_FAILURE;
		break;

	case AMDGV_RUN_GPU:
		if (current != AMDGV_INIT_GPU && current != AMDGV_LOAD_GPU_STATE)
			ret = AMDGV_FAILURE;
		break;

	case AMDGV_IDLE_GPU:
		if (current != AMDGV_RUN_GPU)
			ret = AMDGV_FAILURE;
		break;

	case AMDGV_LOAD_GPU_STATE:
		if (current != AMDGV_SAVE_GPU_STATE && current != AMDGV_SAVE_RLCV_STATE)
			ret = AMDGV_FAILURE;
		break;

	case AMDGV_SAVE_GPU_STATE:
		if (current != AMDGV_IDLE_GPU)
			ret = AMDGV_FAILURE;
		break;

	case AMDGV_SAVE_RLCV_STATE:
		AMDGV_INFO("[%s] VF%d - %s does not affect the WS sequence",
			   amdgv_hw_sched_id_to_name(adapt, hw_sched_id), idx_vf,
			   amdgv_gpuiov_cmd_to_name(adapt, next, hw_sched_id));
		break;

	case AMDGV_LOAD_RLCV_STATE:
		AMDGV_INFO("[%s] VF%d - %s does not affect the WS sequence",
			   amdgv_hw_sched_id_to_name(adapt, hw_sched_id), idx_vf,
			   amdgv_gpuiov_cmd_to_name(adapt, next, hw_sched_id));
		break;
	case AMDGV_SHUTDOWN_GPU:
		if (current != AMDGV_SAVE_GPU_STATE)
			ret = AMDGV_FAILURE;
		break;

	default:
		AMDGV_ERROR("[%s] VF%d - State change from %s to %s NOT TRACKED",
			    amdgv_hw_sched_id_to_name(adapt, hw_sched_id), idx_vf,
			    amdgv_gpuiov_cmd_to_name(adapt, current, hw_sched_id),
			    amdgv_gpuiov_cmd_to_name(adapt, next, hw_sched_id));
	}

	if (ret) {
		AMDGV_ERROR("[%s] VF%d - INVALID state change from \"%s\" to \"%s\"\n",
			    amdgv_hw_sched_id_to_name(adapt, hw_sched_id), idx_vf,
			    amdgv_gpuiov_cmd_to_name(adapt, current, hw_sched_id),
			    amdgv_gpuiov_cmd_to_name(adapt, next, hw_sched_id));
	}
#ifdef DEBUG_WORLD_SWITCH_VERBOSE
	else {
		AMDGV_INFO(
			"[%s] VF%d - Valid state change request from %s(ADP) \"%s\"(VF) to \"%s\"\n",
			amdgv_hw_sched_id_to_name(adapt, hw_sched_id), idx_vf,
			amdgv_gpuiov_cmd_to_name(adapt, adapter_state, hw_sched_id),
			amdgv_gpuiov_cmd_to_name(adapt, current, hw_sched_id),
			amdgv_gpuiov_cmd_to_name(adapt, next, hw_sched_id));
	}
#endif

	return 0;
}

#endif

void amdgv_bp_mode_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint32_t hw_sched_id, uint32_t ws_cmd)
{
	AMDGV_WARN(
		"Paused at break point: %s scheduler: %s, vf: %d\n",
		amdgv_gpuiov_cmd_to_name(adapt, ws_cmd, hw_sched_id),
		amdgv_hw_sched_id_to_name(adapt, hw_sched_id), idx_vf);

	adapt->bp_info.is_in_bp = true;
	adapt->bp_info.ws_cmd = ws_cmd;
	adapt->bp_info.hw_sched_id = hw_sched_id;
	adapt->bp_info.idx_vf = idx_vf;
	while (!adapt->bp_go_flag) {
		oss_msleep(10);
	}
	AMDGV_INFO("Leave break point: %s scheduler: %s, vf: %d\n",
		amdgv_gpuiov_cmd_to_name(adapt, ws_cmd, hw_sched_id),
		amdgv_hw_sched_id_to_name(adapt, hw_sched_id), idx_vf);

	/* bp_info is cleared whenever it's not paused at break point
	 * clear bp_go_flag so it pauses at next ws command
	 */
	adapt->bp_info.is_in_bp = false;
	adapt->bp_info.ws_cmd = 0;
	adapt->bp_info.hw_sched_id = 0;
	adapt->bp_info.idx_vf = 0;
	adapt->bp_go_flag = 0;
}

int amdgv_gpuiov_update_vf_busy_status(struct amdgv_adapter *adapt)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id, world_switch_id;

	if (!adapt->gpuiov.funcs->update_vf_busy_status)
		return 0;

	for (world_switch_id = 0; world_switch_id < adapt->sched.num_world_switch;
	     world_switch_id++) {
		/* only for hybrid liquid mode on GFX IP */
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->sched_block != AMDGV_SCHED_BLOCK_GFX)
			continue;

		world_switch->vf_busy_status = 0;
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			world_switch->vf_busy_status |=
				adapt->gpuiov.funcs->update_vf_busy_status(adapt, hw_sched_id);
		}
	}

	return 0;
}

void amdgv_gpuiov_get_usable_fb_size(struct amdgv_adapter *adapt, uint32_t *total_fb_usable)
{
	*total_fb_usable = adapt->gpuiov.total_fb_usable;
}

void amdgv_gpuiov_get_total_avail_fb_size(struct amdgv_adapter *adapt,
					  uint32_t *total_fb_avail)
{
	*total_fb_avail = adapt->gpuiov.total_fb_avail;
}

void amdgv_gpuiov_get_resv_area(struct amdgv_adapter *adapt, uint64_t *addr, uint64_t *size)
{
	/* convert from in units of MB to in units of bytes */
	*addr = adapt->gpuiov.resv_addr;
	*addr <<= 20;
	*size = adapt->gpuiov.resv_size;
	*size <<= 20;
}

void amdgv_gpuiov_set_total_fb_consumed(struct amdgv_adapter *adapt, uint16_t size)
{
	if (adapt->gpuiov.funcs->set_total_fb_consumed)
		adapt->gpuiov.funcs->set_total_fb_consumed(adapt, size);
}

void amdgv_gpuiov_get_total_fb_consumed(struct amdgv_adapter *adapt, uint16_t *size)
{
	if (adapt->gpuiov.funcs->get_total_fb_consumed)
		adapt->gpuiov.funcs->get_total_fb_consumed(adapt, size);
}

int amdgv_gpuiov_toggle_rlcg_vf_interface(struct amdgv_adapter *adapt, uint32_t idx_vf, bool enable)
{
	if (adapt->gpuiov.funcs->toggle_rlcg_vf_interface)
		return adapt->gpuiov.funcs->toggle_rlcg_vf_interface(adapt, idx_vf, enable);
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_set_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t offset,
			   uint32_t size)
{
	if (adapt->gpuiov.funcs->set_vf_fb)
		return adapt->gpuiov.funcs->set_vf_fb(adapt, idx_vf, offset, size);
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_get_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *offset,
			   uint32_t *size, uint32_t *real_size)
{
	if (adapt->gpuiov.funcs->get_vf_fb)
		return adapt->gpuiov.funcs->get_vf_fb(adapt, idx_vf, offset, size, real_size);
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_set_vf_access(struct amdgv_adapter *adapt, uint32_t idx_vf,
			       uint32_t vf_access_select, bool enable)
{
	int ret = AMDGV_FAILURE;
	if (adapt->gpuiov.funcs->set_vf_access) {
		oss_mutex_lock(adapt->set_vf_access_lock);
		ret = adapt->gpuiov.funcs->set_vf_access(adapt, idx_vf, vf_access_select,
							 enable);
		oss_mutex_unlock(adapt->set_vf_access_lock);
	}
	return ret;
}

bool amdgv_gpuiov_get_vf_access(struct amdgv_adapter *adapt, uint32_t idx_vf,
			       uint32_t vf_access_select)
{
	/* assume access allowed if get_vf_access not available */
	bool ret = true;
	if (adapt->gpuiov.funcs->get_vf_access)
		ret = adapt->gpuiov.funcs->get_vf_access(adapt, idx_vf, vf_access_select);
	return ret;
}

bool amdgv_gpuiov_is_cmd_complete(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	return adapt->gpuiov.funcs->is_cmd_complete(adapt, hw_sched_id);
}

void amdgv_dump_gpuiov_cmd_status(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	if (adapt->gpuiov.funcs->dump_gpuiov_cmd_status)
		adapt->gpuiov.funcs->dump_gpuiov_cmd_status(adapt, hw_sched_id);
}

void amdgv_update_histogram(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint32_t hw_sched_id, enum amdgv_gpuiov_cmd cmd,
				   uint64_t cmd_start_time)
{
	struct amdgv_histogram *hist;
	struct amdgv_time_log *pf_time_log;
	uint32_t *range_values;
	uint64_t cmd_time_delta;
	int bucket;

	if (IS_HW_SCHED_TYPE_MM(hw_sched_id))
		return;

	cmd_time_delta = oss_get_time_stamp() - cmd_start_time;

	pf_time_log = &adapt->array_vf[idx_vf].time_log[hw_sched_id];

	switch (cmd) {
	case AMDGV_LOAD_GPU_STATE:
		hist = &adapt->array_vf[idx_vf].time_log[hw_sched_id].gfx_load_latency_us;
		range_values = pf_time_log->gfx_load_latency_us.range;
		break;

	case AMDGV_IDLE_GPU:
		hist = &adapt->array_vf[idx_vf].time_log[hw_sched_id].gfx_idle_latency_us;
		range_values = pf_time_log->gfx_idle_latency_us.range;
		break;

	case AMDGV_RUN_GPU:
		hist = &adapt->array_vf[idx_vf].time_log[hw_sched_id].gfx_run_latency_us;
		range_values = pf_time_log->gfx_run_latency_us.range;
		break;

	case AMDGV_SAVE_GPU_STATE:
		hist = &adapt->array_vf[idx_vf].time_log[hw_sched_id].gfx_save_latency_us;
		range_values = pf_time_log->gfx_save_latency_us.range;
		break;

	default:
		return;
	}

	hist->total++;
	for (bucket = 0; bucket < AMDGV_HISTOGRAM_SIZE - 1; bucket++)
		if (cmd_time_delta < range_values[bucket])
			break;

	hist->count[bucket]++;
}

static int amdgv_wait_detect_hang(struct amdgv_adapter *adapt, amdgv_wait_cb_t cb_func, void *cb_context, uint64_t timeout)
{
	if (adapt->gfx.funcs->wait_detect_hang)
		return adapt->gfx.funcs->wait_detect_hang(adapt, cb_func, cb_context, timeout);
	else
		return amdgv_wait_for(adapt, cb_func, cb_context, timeout, AMDGV_WAIT_FLAG_USLEEP);
}

static int wait_cmd_complete_cb(void *context)
{
	struct amdgv_gpuiov_wait_context *wc = (struct amdgv_gpuiov_wait_context *)context;
	struct amdgv_adapter *adapt = wc->adapt;
	uint32_t hw_sched_id = wc->hw_sched_id;

	/* return 0 when hit */
	return !amdgv_gpuiov_is_cmd_complete(adapt, hw_sched_id);
}

/* timeout - in microseconds. */
static int wait_cmd_complete(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id, uint64_t timeout)
{
	int wait_ret = 0;
	struct amdgv_gpuiov_wait_context wc;

	wc.adapt = adapt;
	wc.hw_sched_id = hw_sched_id;

	if (IS_HW_SCHED_TYPE_MM(hw_sched_id) || !adapt->gfx.hang_detection_supported)
		wait_ret = amdgv_wait_for(adapt, wait_cmd_complete_cb, (void *)&wc, timeout,
			AMDGV_WAIT_FLAG_USLEEP);
	else
		wait_ret = amdgv_wait_detect_hang(adapt, wait_cmd_complete_cb, (void *)&wc, timeout);

	/* Add to diagnosis data */
	AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_END(
		idx_vf, hw_sched_id,
		adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd,
		adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status, wait_ret);

	if (adapt->bp_mode == AMDGV_BP_MODE_2 && !(adapt->is_user_ws_cmd))
		amdgv_bp_mode_wait(adapt, idx_vf, hw_sched_id,
				   adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd);

	if (!wait_ret)
		return 0;

	amdgv_put_error(idx_vf, AMDGV_ERROR_IOV_CMD_TIMEOUT, timeout);
	amdgv_dump_gpuiov_cmd_status(adapt, hw_sched_id);

	return AMDGV_FAILURE;
}

static int wait_for_first_cmd_complete_cb(void *context)
{
	struct amdgv_gpuiov_wait_for_first_context *wc =
		(struct amdgv_gpuiov_wait_for_first_context *)context;
	struct amdgv_adapter *adapt = wc->adapt;
	uint32_t hw_sched_id;

	wc->complete_hw_sched_id = 0;

	for_each_id (hw_sched_id, wc->hw_sched_mask) {
		if (amdgv_gpuiov_is_cmd_complete(adapt, hw_sched_id)) {
			wc->complete_hw_sched_id = hw_sched_id;
			return 0;
		}
	}

	return AMDGV_FAILURE;
}

static bool if_gfx_engine_in_mask(struct amdgv_adapter *adapt, uint32_t hw_sched_mask)
{
	uint32_t hw_sched_id;
	for_each_id (hw_sched_id, hw_sched_mask) {
		if (IS_HW_SCHED_TYPE_GFX(hw_sched_id)) {
			return true;
		}
	}
	return false;
}

/*
 *  timeout - in microseconds.
*/
int wait_for_first_cmd_complete(struct amdgv_adapter *adapt,
		uint32_t hw_sched_mask, uint32_t *complete_hw_sched_id, uint64_t timeout)
{
	int wait_ret = 0;
	uint32_t hw_sched_id;
	uint32_t idx_vf;
	struct amdgv_gpuiov_wait_for_first_context wc = { 0 };

	wc.adapt = adapt;
	/* Assume all HW Schedulers are pending at the start */
	wc.hw_sched_mask = hw_sched_mask;

	if (!if_gfx_engine_in_mask(adapt, hw_sched_mask) || !adapt->gfx.hang_detection_supported)
		wait_ret = amdgv_wait_for(adapt, wait_for_first_cmd_complete_cb, (void *)&wc, timeout,
			AMDGV_WAIT_FLAG_USLEEP);
	else
		wait_ret = amdgv_wait_detect_hang(adapt, wait_for_first_cmd_complete_cb, (void *)&wc, timeout);

	if (wait_ret) {
		for_each_id(hw_sched_id, hw_sched_mask) {
			amdgv_gpuiov_get_active_vf_idx(adapt, hw_sched_id, &idx_vf);
			AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_END(idx_vf, hw_sched_id,
				adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd,
				adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status, wait_ret);
			amdgv_put_error(idx_vf, AMDGV_ERROR_IOV_CMD_TIMEOUT, timeout);
			amdgv_dump_gpuiov_cmd_status(adapt, hw_sched_id);

			if (adapt->bp_mode == AMDGV_BP_MODE_2 && !(adapt->is_user_ws_cmd)) {
				amdgv_bp_mode_wait(adapt, idx_vf, hw_sched_id,
						adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd);
			}
		}

		return AMDGV_FAILURE;
	}

	if (adapt->bp_mode == AMDGV_BP_MODE_2 && !(adapt->is_user_ws_cmd)) {
		for_each_id(hw_sched_id, hw_sched_mask) {
			amdgv_gpuiov_get_active_vf_idx(adapt, hw_sched_id, &idx_vf);
			amdgv_bp_mode_wait(adapt, idx_vf, hw_sched_id,
					adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd);
		}
	}

	amdgv_gpuiov_get_active_vf_idx(adapt, wc.complete_hw_sched_id, &idx_vf);
	AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_END(idx_vf, wc.complete_hw_sched_id,
		adapt->gpuiov.ctrl_blocks[wc.complete_hw_sched_id].last_cmd,
		adapt->gpuiov.ctrl_blocks[wc.complete_hw_sched_id].last_status, wait_ret);

	*complete_hw_sched_id = wc.complete_hw_sched_id;
	return 0;
}

#ifdef WS_RECORD
void amdgv_gpuiov_record_queue_push(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   uint32_t hw_sched_id,
					   enum amdgv_record_status status)
{
	struct amdgv_record_entity *record;
	uint16_t rptr, wptr;

	if (!(adapt->flags & AMDGV_FLAG_WS_RECORD))
		return;

	oss_spin_lock(adapt->sched.record_queue_lock);

	wptr = adapt->sched.record_queue_wptr;
	rptr = adapt->sched.record_queue_rptr;

	if (((uint16_t)(wptr + 1) == rptr)) {
		AMDGV_ERROR("record queue is full: wptr = %lld; rptr = %lld\n", wptr, rptr);
		oss_spin_unlock(adapt->sched.record_queue_lock);
		return;
	}

	adapt->sched.record_queue_wptr++;
	record = &adapt->sched.record_queue[wptr];

	record->time_stamp = oss_get_time_stamp();
	record->idx_vf = idx_vf;
	record->hw_sched_id = hw_sched_id;
	record->status = status;

	oss_spin_unlock(adapt->sched.record_queue_lock);

	return;
}
#endif

int amdgv_gpuiov_init_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_id)
{
#ifdef DEBUG_WORLD_SWITCH
	if (check_state_transition(adapt, AMDGV_INIT_GPU, idx_vf, hw_sched_id))
		return AMDGV_FAILURE;
#endif
	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_INIT_GPU, hw_sched_id, idx_vf,
				     AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, AMDGV_RECORD_INIT_START);
#endif

	return 0;
}

int amdgv_gpuiov_init_vf(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t hw_sched_id)
{
	int ret;

	amdgv_gpuiov_init_vf_no_wait(adapt, idx_vf, hw_sched_id);

	ret = wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time());
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, AMDGV_RECORD_INIT_END);
#endif
	if (!ret)
		adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id] = AMDGV_INIT_GPU;

	return ret;
}

int amdgv_gpuiov_load_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_id)
{
#ifdef DEBUG_WORLD_SWITCH
	if (check_state_transition(adapt, AMDGV_LOAD_GPU_STATE, idx_vf, hw_sched_id))
		return AMDGV_FAILURE;
#endif

	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_LOAD_GPU_STATE, hw_sched_id, idx_vf,
				     AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, AMDGV_RECORD_LOAD_START);
#endif

	return 0;
}

int amdgv_gpuiov_load_vf(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t hw_sched_id)
{
	uint64_t load_time_start;

	load_time_start = oss_get_time_stamp();

	amdgv_gpuiov_load_vf_no_wait(adapt, idx_vf, hw_sched_id);

	if (wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time()) != AMDGV_FAILURE) {
#ifdef WS_RECORD
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					       AMDGV_RECORD_LOAD_END);
#endif
		amdgv_update_histogram(adapt, idx_vf, hw_sched_id, AMDGV_LOAD_GPU_STATE,
				       load_time_start);

		if (idx_vf >= AMDGV_MAX_VF_SLOT) {
			AMDGV_ERROR("The VF %d exceeds the boudary", idx_vf);
			return AMDGV_FAILURE;
		}

		adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id] = AMDGV_LOAD_GPU_STATE;
		return 0;
	}
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_idle_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_id)
{
#ifdef DEBUG_WORLD_SWITCH
	if (check_state_transition(adapt, AMDGV_IDLE_GPU, idx_vf, hw_sched_id))
		return AMDGV_FAILURE;
#endif

	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_IDLE_GPU, hw_sched_id, idx_vf,
				     AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, AMDGV_RECORD_IDLE_START);
#endif

	return 0;
}

int amdgv_gpuiov_idle_vf(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t hw_sched_id)
{
	uint64_t idle_time_start;

	idle_time_start = oss_get_time_stamp();

	amdgv_gpuiov_idle_vf_no_wait(adapt, idx_vf, hw_sched_id);

	if (wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time()) != AMDGV_FAILURE) {
#ifdef WS_RECORD
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					       AMDGV_RECORD_IDLE_END);
#endif
		if (idx_vf >= AMDGV_MAX_VF_SLOT) {
			AMDGV_ERROR("The VF %d exceeds the boudary", idx_vf);
			return AMDGV_FAILURE;
		}
		adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id] = AMDGV_IDLE_GPU;
		amdgv_update_histogram(adapt, idx_vf, hw_sched_id, AMDGV_IDLE_GPU,
				       idle_time_start);

		if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode == AMDGV_SCHED_HYBRID_LIQUID_MODE)
			amdgv_gpuiov_update_vf_busy_status(adapt);

		return 0;
	}
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_save_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_id)
{
#ifdef DEBUG_WORLD_SWITCH
	if (check_state_transition(adapt, AMDGV_SAVE_GPU_STATE, idx_vf, hw_sched_id))
		return AMDGV_FAILURE;
#endif

	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_SAVE_GPU_STATE, hw_sched_id, idx_vf,
				     AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, AMDGV_RECORD_SAVE_START);
#endif

	return 0;
}

int amdgv_gpuiov_save_vf(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t hw_sched_id)
{
	uint64_t save_time_start;

	save_time_start = oss_get_time_stamp();

	amdgv_gpuiov_save_vf_no_wait(adapt, idx_vf, hw_sched_id);

	if (wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time()) != AMDGV_FAILURE) {
#ifdef WS_RECORD
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					       AMDGV_RECORD_SAVE_END);
#endif
		amdgv_update_histogram(adapt, idx_vf, hw_sched_id, AMDGV_SAVE_GPU_STATE,
				       save_time_start);

		adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id] = AMDGV_SAVE_GPU_STATE;
		return 0;
	}
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_run_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			uint32_t hw_sched_id)
{

#ifdef DEBUG_WORLD_SWITCH
	if (check_state_transition(adapt, AMDGV_RUN_GPU, idx_vf, hw_sched_id))
		return AMDGV_FAILURE;
#endif

	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_RUN_GPU, hw_sched_id, idx_vf,
				     AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, AMDGV_RECORD_RUN_START);
#endif

	return 0;
}

int amdgv_gpuiov_run_vf(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t hw_sched_id)
{
	uint64_t run_time_start;

	run_time_start = oss_get_time_stamp();

	amdgv_gpuiov_run_vf_no_wait(adapt, idx_vf, hw_sched_id);

	if (wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time()) != AMDGV_FAILURE) {
#ifdef WS_RECORD
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					       AMDGV_RECORD_RUN_END);
#endif
		amdgv_update_histogram(adapt, idx_vf, hw_sched_id, AMDGV_RUN_GPU,
				       run_time_start);

		adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id] = AMDGV_RUN_GPU;


		return 0;
	}
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_load_rlcv_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
				 uint32_t hw_sched_id)
{
	int ret;

	AMDGV_ASSERT(IS_HW_SCHED_TYPE_GFX(hw_sched_id));
#ifdef DEBUG_WORLD_SWITCH
	if (check_state_transition(adapt, AMDGV_LOAD_RLCV_STATE, idx_vf, hw_sched_id))
		return AMDGV_FAILURE;
#endif

	if (adapt->flags & AMDGV_FLAG_USE_LEGACY_FLR_SEQUENCE)
		return 0;

	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_LOAD_RLCV_STATE, hw_sched_id, idx_vf,
				     AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
				       AMDGV_RECORD_LOAD_RLCV_STATE_START);
#endif
	ret = wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time());
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
				       AMDGV_RECORD_LOAD_RLCV_STATE_END);
#endif
	return ret;
}

int amdgv_gpuiov_save_rlcv_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
				 uint32_t hw_sched_id)
{
	int ret;

	AMDGV_ASSERT(IS_HW_SCHED_TYPE_GFX(hw_sched_id));
#ifdef DEBUG_WORLD_SWITCH
	if (check_state_transition(adapt, AMDGV_SAVE_RLCV_STATE, idx_vf, hw_sched_id))
		return AMDGV_FAILURE;
#endif

	if (adapt->flags & AMDGV_FLAG_USE_LEGACY_FLR_SEQUENCE)
		return 0;

	if (adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd != AMDGV_SAVE_GPU_STATE &&
	    adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd != AMDGV_RUN_GPU)
		return 0;

	adapt->reset.saved_rlcv_state = true;

	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_SAVE_RLCV_STATE, hw_sched_id, idx_vf,
				     AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
				       AMDGV_RECORD_SAVE_RLCV_STATE_START);
#endif
	ret = wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time());
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
				       AMDGV_RECORD_SAVE_RLCV_STATE_END);
#endif
	return ret;
}


int amdgv_gpuiov_transfer_vf_data(struct amdgv_adapter *adapt,
				  uint32_t idx_vf, uint32_t hw_sched_id, bool to_export)
{
	int ret = 0;

	if (adapt->gpuiov.funcs->transfer_vf_data
			&& idx_vf != AMDGV_PF_IDX) {
		ret = adapt->gpuiov.funcs->transfer_vf_data(adapt, hw_sched_id, idx_vf, to_export);

		if (ret != AMDGV_FAILURE) {
#ifdef WS_RECORD
			amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					to_export ? AMDGV_RECORD_EXPORT_VF_DATA_START :
					AMDGV_RECORD_IMPORT_VF_DATA_START);
#endif
			ret = wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time());
#ifdef WS_RECORD
			amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					to_export ? AMDGV_RECORD_EXPORT_VF_DATA_END :
					AMDGV_RECORD_IMPORT_VF_DATA_END);
#endif
		} else {
			AMDGV_ERROR("Failed to complete TRANSFER_VF_DATA.\n");
		}
	} else {
		AMDGV_ERROR("Unable to start TRANSFER_VF_DATA.\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_gpuiov_set_mmsch_vfgate(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  uint32_t hw_sched_id, bool enable)
{
	int ret;
	enum amdgv_gpuiov_cmd cmd =
		enable ? AMDGV_ENABLE_MMSCH_VFGATE : AMDGV_DISABLE_MMSCH_VFGATE;

	if (adapt->gpuiov.funcs->set_mmsch_vfgate && idx_vf != AMDGV_PF_IDX) {
		ret = adapt->gpuiov.funcs->set_mmsch_vfgate(adapt, cmd, hw_sched_id, idx_vf,
							    AMDGV_INVALID_IDX_VF);

		/* unsupported by fw. Don't wait for resp */
		if (ret != AMDGV_FAILURE) {
#ifdef WS_RECORD
			amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, enable ? AMDGV_RECORD_ENABLE_MMSCH_VFGATE_START :
					AMDGV_RECORD_DISABLE_MMSCH_VFGATE_START);
#endif
			ret = wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time());
#ifdef WS_RECORD
			amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, enable ? AMDGV_RECORD_ENABLE_MMSCH_VFGATE_END :
					AMDGV_RECORD_DISABLE_MMSCH_VFGATE_END);
#endif
			return ret;
		}
	}

	return 0;
}

int amdgv_gpuiov_shutdown_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id)
{
	if (adapt->flags & AMDGV_FLAG_USE_LEGACY_FLR_SEQUENCE)
		return 0;

#ifdef DEBUG_WORLD_SWITCH
	if (check_state_transition(adapt, AMDGV_SHUTDOWN_GPU, idx_vf, hw_sched_id))
		return AMDGV_FAILURE;
#endif

	if (IS_HW_SCHED_TYPE_GFX(hw_sched_id) ||
	     (IS_HW_SCHED_TYPE_MM(hw_sched_id) && adapt->mmsch.is_feature_enabled.support_shutdown_cmd)) {
		adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_SHUTDOWN_GPU, hw_sched_id, idx_vf,
				     AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, AMDGV_RECORD_SHUTDOWN_START);
#endif
	}

	return 0;
}

int amdgv_gpuiov_shutdown_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id)
{
	int ret = 0;

#ifdef DEBUG_WORLD_SWITCH
	if (check_state_transition(adapt, AMDGV_SHUTDOWN_GPU, idx_vf, hw_sched_id))
		return AMDGV_FAILURE;
#endif

	if (adapt->flags & AMDGV_FLAG_USE_LEGACY_FLR_SEQUENCE)
		return 0;

	if (IS_HW_SCHED_TYPE_GFX(hw_sched_id) ||
	     (IS_HW_SCHED_TYPE_MM(hw_sched_id) && adapt->mmsch.is_feature_enabled.support_shutdown_cmd)) {
		adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_SHUTDOWN_GPU, hw_sched_id, idx_vf,
					     AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					       AMDGV_RECORD_SHUTDOWN_START);
#endif
		ret = wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time());
#ifdef WS_RECORD
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					       AMDGV_RECORD_SHUTDOWN_END);
#endif
	}

	if (!ret)
		adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id] = AMDGV_SHUTDOWN_GPU;

	return ret;
}

int amdgv_gpuiov_event_notification(struct amdgv_adapter *adapt, uint32_t idx_vf,
				    uint32_t hw_sched_id, enum amdgv_gpuiov_event_id event_id,
					uint32_t value)
{
	int ret = AMDGV_FAILURE;

	if (adapt->gpuiov.funcs->set_event_notification) {
		ret = adapt->gpuiov.funcs->set_event_notification(adapt, hw_sched_id, idx_vf,
								  event_id, value);
#ifdef WS_RECORD
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					       AMDGV_RECORD_EVENT_NOTIFICATION_START);
#endif
		if (!ret)
			ret = wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time());
#ifdef WS_RECORD
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id,
					       AMDGV_RECORD_EVENT_NOTIFICATION_END);
#endif
	} else {
		AMDGV_WARN("Event Notification is not supported.\n");
		return 0;
	}

	return ret;
}

int amdgv_gpuiov_world_switch_oneshot(struct amdgv_adapter *adapt, uint32_t idx_vf,
				      uint32_t next_idx_vf, uint32_t hw_sched_id)
{
	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_CONTEXT_SWITCH, hw_sched_id, idx_vf,
				     next_idx_vf);
	return wait_cmd_complete(adapt, idx_vf, hw_sched_id, cmd_allow_time());
}

int amdgv_config_auto_sched_params(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	int ret = 0;
	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_CONFIG_AUTO_HW_SCHED_MODE, hw_sched_id,
				AMDGV_INVALID_IDX_VF, AMDGV_INVALID_IDX_VF);

	ret = wait_cmd_complete(adapt, AMDGV_PF_IDX, hw_sched_id, cmd_allow_time());

	return ret;
}

int amdgv_gpuiov_config_auto_sched_mode(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					enum amdgv_sched_mode sched_mode)
{
	int ret = 0, sum = 0;
	struct scheduler_memory_descriptor *sched_mem_desc = &adapt->gpuiov.sched_cfg;

	if (IS_HW_SCHED_TYPE_MM(hw_sched_id))
		/* configure HW auto-scheduling mode
		 * HW Auto-Sched Mode is only for GFX HW scheduler
		 * MM HW scheduler mode is not configurable
		 */
		return 0;

	sched_mem_desc->scheduler_mode = sched_mode <= AMDGV_SCHED_MAX_HW_SCHED_MODE ? sched_mode : 0;
	sched_mem_desc->auto_config.ws_cmd_timeout = AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_GFX) ? AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_GFX) : 5 * 100000; //500 ms
	sched_mem_desc->auto_config.max_debit = 2 * 100000;
	if (sched_mode == AMDGV_SCHED_LIQUID_MODE) {
		sched_mem_desc->auto_config.max_skipped_cycle = 8;
		sched_mem_desc->auto_config.busy_check_interval = 100; // 100 us
	}

	if (adapt->gpuiov.perf_log_mem && !sched_mem_desc->feature_flags.flags.config_perf_data_log) {
		sched_mem_desc->feature_flags.flags.config_perf_data_log = 1;
		sched_mem_desc->perf_data_log.buffer_location = amdgv_memmgr_get_gpu_addr(adapt->gpuiov.perf_log_mem);
		sched_mem_desc->perf_data_log.buffer_size = amdgv_memmgr_get_size(adapt->gpuiov.perf_log_mem);
	}
	if (adapt->gpuiov.debug_dump_mem && !sched_mem_desc->feature_flags.flags.config_debug_dump_log) {
		sched_mem_desc->feature_flags.flags.config_debug_dump_log = 1;
		sched_mem_desc->debug_dump_log.buffer_location = amdgv_memmgr_get_gpu_addr(adapt->gpuiov.debug_dump_mem);
		sched_mem_desc->debug_dump_log.buffer_size = amdgv_memmgr_get_size(adapt->gpuiov.debug_dump_mem);
	}

	sched_mem_desc->version = SCHEDULER_DESCRIPTOR_VERSION;

	/* RLCV expects sum + checksum = 0 for validation */
	sum = amd_sriov_msg_checksum(sched_mem_desc, sizeof(struct scheduler_memory_descriptor), 0, sched_mem_desc->checksum);
	sched_mem_desc->checksum = (0x100 - (sum & 0xFF)) & 0xFF;

	ret = amdgv_gpuiov_set_scheduler_config_descriptor(adapt, hw_sched_id, sched_mem_desc);
	if (ret) {
		AMDGV_ERROR("Failed to set scheduler config descriptor\n");
		return ret;
	}

	ret = amdgv_config_auto_sched_params(adapt, hw_sched_id);

	return ret;
}

int amdgv_gpuiov_enable_auto_sched(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	int ret;

	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_ENABLE_AUTO_HW_SWITCH, hw_sched_id,
				     AMDGV_INVALID_IDX_VF, AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, AMDGV_PF_IDX, hw_sched_id,
				       AMDGV_RECORD_ENABLE_AUTO_SCHED_START);
#endif
	ret = wait_cmd_complete(adapt, AMDGV_PF_IDX, hw_sched_id, cmd_allow_time());
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, AMDGV_PF_IDX, hw_sched_id,
				       AMDGV_RECORD_ENABLE_AUTO_SCHED_END);
#endif
	return ret;
}

int amdgv_gpuiov_disable_auto_sched(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	int ret;
	adapt->gpuiov.funcs->set_cmd(adapt, AMDGV_DISABLE_AUTO_HW_SCHED, hw_sched_id,
				     AMDGV_INVALID_IDX_VF, AMDGV_INVALID_IDX_VF);
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, AMDGV_PF_IDX, hw_sched_id,
				       AMDGV_RECORD_DISABLE_AUTO_SCHED_START);
#endif
	ret = wait_cmd_complete(adapt, AMDGV_PF_IDX, hw_sched_id, cmd_allow_time());
#ifdef WS_RECORD
	amdgv_gpuiov_record_queue_push(adapt, AMDGV_PF_IDX, hw_sched_id,
				       AMDGV_RECORD_DISABLE_AUTO_SCHED_END);
#endif
	return ret;
}

int amdgv_gpuiov_get_intr(struct amdgv_adapter *adapt, uint32_t *intr_bits)
{
	if (adapt->gpuiov.funcs->get_intr_bits)
		return adapt->gpuiov.funcs->get_intr_bits(adapt, intr_bits);
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_set_intr(struct amdgv_adapter *adapt, uint32_t intr_bits)
{
	if (adapt->gpuiov.funcs->set_intr_bits)
		return adapt->gpuiov.funcs->set_intr_bits(adapt, intr_bits);
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_get_intr_status(struct amdgv_adapter *adapt, uint32_t *sta_bits)
{
	if (adapt->gpuiov.funcs->get_intr_status)
		return adapt->gpuiov.funcs->get_intr_status(adapt, sta_bits);
	return AMDGV_FAILURE;
}

int amdgv_gpuiov_clear_intr_status(struct amdgv_adapter *adapt, uint32_t sta_bits)
{
	if (adapt->gpuiov.funcs->clear_intr_status)
		return adapt->gpuiov.funcs->clear_intr_status(adapt, sta_bits);
	return AMDGV_FAILURE;
}

bool amdgv_gpuiov_intr_status(struct amdgv_adapter *adapt, enum amdgv_gpuiov_intr_id intr_id)
{
	int ret;
	uint32_t intr_status;

	ret = amdgv_gpuiov_get_intr_status(adapt, &intr_status);
	if (ret)
		return false;

	return (intr_status & intr_id);
}

int amdgv_gpuiov_update_hvvm_mbox_index(struct amdgv_adapter *adapt, uint8_t index)
{
	return adapt->gpuiov.funcs->update_hvvm_mbox_index(adapt, index);
}

int amdgv_gpuiov_rcv_hvvm_mbox_msg(struct amdgv_adapter *adapt, uint8_t *msg_data)
{
	return adapt->gpuiov.funcs->rcv_hvvm_mbox_msg(adapt, msg_data);
}

int amdgv_gpuiov_trn_hvvm_mbox_data(struct amdgv_adapter *adapt, uint8_t msg_data)
{
	return adapt->gpuiov.funcs->trn_hvvm_mbox_data(adapt, msg_data);
}

int amdgv_gpuiov_set_hvvm_mbox_valid(struct amdgv_adapter *adapt, int8_t valid)
{
	return adapt->gpuiov.funcs->set_hvvm_mbox_valid(adapt, valid);
}

int amdgv_gpuiov_get_hvvm_mbox_msg_valid(struct amdgv_adapter *adapt, uint32_t *valid)
{
	return adapt->gpuiov.funcs->get_hvvm_mbox_msg_valid(adapt, valid);
}

int amdgv_gpuiov_set_hvvm_mbox_ack(struct amdgv_adapter *adapt)
{
	return adapt->gpuiov.funcs->set_hvvm_mbox_ack(adapt);
}

int amdgv_gpuiov_auto_sched_add_vf(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				   uint32_t idx_vf)
{
	adapt->gpuiov.funcs->add_active_vf(adapt, hw_sched_id, idx_vf);
	return 0;
}

int amdgv_gpuiov_auto_sched_remove_vf(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				      uint32_t idx_vf)
{
	adapt->gpuiov.funcs->remove_active_vf(adapt, hw_sched_id, idx_vf);
	return 0;
}

int amdgv_gpuiov_get_time_quanta_definition(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					    int index, uint8_t *quanta_option)
{
	adapt->gpuiov.funcs->get_time_quanta_definition(adapt, hw_sched_id, index,
							quanta_option);
	return 0;
}

int amdgv_gpuiov_set_time_quanta_definition(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					    int index, uint8_t quanta_option)
{
	adapt->gpuiov.funcs->set_time_quanta_definition(adapt, hw_sched_id, index,
							quanta_option);
	return 0;
}

int amdgv_gpuiov_get_time_quanta_option(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					uint32_t *time_quanta_option)
{
	if (adapt->gpuiov.funcs->get_time_quanta_option) {
		return adapt->gpuiov.funcs->get_time_quanta_option(adapt, hw_sched_id,
								   time_quanta_option);
	}
	return 0;
}

int amdgv_gpuiov_set_time_quanta_option(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					uint32_t time_quanta_option)
{
	if (adapt->gpuiov.funcs->set_time_quanta_option) {
		return adapt->gpuiov.funcs->set_time_quanta_option(adapt, hw_sched_id,
								   time_quanta_option);
	}
	return 0;
}

int amdgv_gpuiov_get_time_quanta_index(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       uint32_t hw_sched_id, uint32_t *time_quanta_index)
{
	adapt->gpuiov.funcs->get_time_quanta_index(adapt, idx_vf, hw_sched_id,
						   time_quanta_index);
	return 0;
}

int amdgv_gpuiov_set_time_quanta_index(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       uint32_t hw_sched_id, uint32_t time_quanta_index)
{
	adapt->gpuiov.funcs->set_time_quanta_index(adapt, idx_vf, hw_sched_id,
						   time_quanta_index);
	return 0;
}

int amdgv_gpuiov_get_active_vfs(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				uint32_t *active_vfs)
{
	return adapt->gpuiov.funcs->get_active_vfs(adapt, hw_sched_id, active_vfs);
}

int amdgv_gpuiov_set_active_vfs(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				uint32_t active_vfs)
{
	return adapt->gpuiov.funcs->set_active_vfs(adapt, hw_sched_id, active_vfs);
}

int amdgv_gpuiov_get_active_vf_idx(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				   uint32_t *idx_vf)
{
	return adapt->gpuiov.funcs->get_active_vf_idx(adapt, hw_sched_id, idx_vf);
}

int amdgv_gpuiov_get_active_vf_status(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				      uint8_t *status)
{
	return adapt->gpuiov.funcs->get_active_vf_status(adapt, hw_sched_id, status);
}

int amdgv_gpuiov_set_scheduler_config_descriptor(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					  struct scheduler_memory_descriptor *sched_cfg)
{
	if (adapt->gpuiov.funcs->set_scheduler_config_descriptor)
		return adapt->gpuiov.funcs->set_scheduler_config_descriptor(adapt, hw_sched_id, sched_cfg);

	return AMDGV_FAILURE;
}

int amdgv_gpuiov_wait_auto_sched_stop(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	if (!adapt->gpuiov.funcs->wait_auto_sched_stop) {
		oss_msleep(50);
		return 0;
	}

	return adapt->gpuiov.funcs->wait_auto_sched_stop(adapt, hw_sched_id);
}

int amdgv_gpuiov_set_sriov_vf_num(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	int ret;

	/* Check if it is needed to do the change */
	if (num_vf == adapt->num_vf)
		return 0;

	/* this routine can only fail if num_vf> max_vf */
	if (num_vf > adapt->max_num_vf)
		goto error;

	oss_pci_disable_sriov(adapt->dev);

	ret = oss_pci_enable_sriov(adapt->dev, num_vf);
	if (ret < 0)
		goto error;

	return 0;

error:
	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_IOV_ENABLE_SRIOV_FAIL, 0);

	return AMDGV_ERROR_IOV_ENABLE_SRIOV_FAIL;
}

bool amdgv_gpuiov_is_sched_mode_supported(struct amdgv_adapter *adapt,
					 struct amdgv_gpuiov_hw_sched_static_config hw_sched_config, enum amdgv_sched_mode sched_mode)
{
	if (sched_mode <= AMDGV_SCHED_BEGIN || sched_mode >= AMDGV_SCHED_END)
		return false;

	return (hw_sched_config.supported_sched_modes & (1 << sched_mode));
}

const char *amdgv_gpuiov_cmd_to_name_default(struct amdgv_adapter *adapt, uint32_t cmd, uint32_t hw_sched_id)
{
	int i, count;
	enum amdgv_gpuiov_hw_sched_type hw_sched_type = adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_sched_type;

	count = ARRAY_SIZE(amdgv_gpuiov_cmd_names);

	for (i = 0; i < count; ++i) {
		if (amdgv_gpuiov_cmd_names[i].id == (cmd & 0x0F) &&
			(1 << hw_sched_type) & amdgv_gpuiov_cmd_names[i].sched_mask)
			return amdgv_gpuiov_cmd_names[i].name;
	}

	return "UNKNOWN CMD";
}

const char *amdgv_gpuiov_cmd_to_name(struct amdgv_adapter *adapt, uint32_t cmd, uint32_t hw_sched_id)
{
	if (adapt->gpuiov.funcs->cmd_to_name)
		return adapt->gpuiov.funcs->cmd_to_name(adapt, cmd, hw_sched_id);
	return amdgv_gpuiov_cmd_to_name_default(adapt, cmd, hw_sched_id);
}

const char *amdgv_gpuiov_status_to_name(uint32_t status)
{
	int i, count;

	count = ARRAY_SIZE(amdgv_gpuiov_status_names);
	for (i = 0; i < count; ++i) {
		if (amdgv_gpuiov_status_names[i].id == status)
			return amdgv_gpuiov_status_names[i].name;
	}
	return "UNKNOWN_CMD_STATUS";
}

void amdgv_gpuiov_ctx_empty_intr_control(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					 bool enable)
{
	if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_block != AMDGV_SCHED_BLOCK_GFX)
		return;

	if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode != AMDGV_SCHED_HYBRID_LIQUID_MODE &&
		adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode != AMDGV_SCHED_LIQUID_MODE)
		return;

	if (adapt->gpuiov.funcs->ctx_empty_intr_control == NULL)
		return;

	adapt->gpuiov.funcs->ctx_empty_intr_control(adapt, hw_sched_id, enable);
}

int amdgv_gpuiov_ctrl_block_setup(struct amdgv_adapter *adapt, struct amdgv_gpuiov_hw_sched_static_config *hw_sched_config,
					 uint32_t size)
{
	uint32_t i, block_num = 0;

	for (i = 0; i < size; i++) {
		if (adapt->gpuiov.funcs->skip_ctrl_block && adapt->gpuiov.funcs->skip_ctrl_block(adapt, i))
			continue;

		adapt->gpuiov.ctrl_blocks[block_num].name = hw_sched_config[i].name;
		adapt->gpuiov.ctrl_blocks[block_num].hw_sched_type = hw_sched_config[i].hw_sched_type;
		adapt->gpuiov.ctrl_blocks[block_num].sched_block = hw_sched_config[i].sched_block;
		adapt->gpuiov.ctrl_blocks[block_num].sched_mode = hw_sched_config[i].default_sched_mode;
		adapt->gpuiov.ctrl_blocks[block_num].hw_inst = hw_sched_config[i].hw_inst;
		adapt->gpuiov.ctrl_blocks[block_num].pci_gpuiov_offset = hw_sched_config[i].pci_gpuiov_offset;
		block_num++;
	}

	adapt->gpuiov.num_ctrl_blocks = block_num;

	AMDGV_INFO("Discovered %d HW schedulers\n", adapt->gpuiov.num_ctrl_blocks);

	for (i = 0; i < adapt->gpuiov.num_ctrl_blocks; i++) {
		if (adapt->gpuiov.ctrl_blocks[i].sched_block != AMDGV_SCHED_BLOCK_GFX)
			continue;

		if (amdgv_gpuiov_is_sched_mode_supported(adapt, hw_sched_config[i], adapt->opt.gfx_sched_mode)) {
			/* override default schedule mode with "sysfs" opt */
			adapt->gpuiov.ctrl_blocks[i].sched_mode = adapt->opt.gfx_sched_mode;
		} else {
			AMDGV_INFO("GFX sched mode %d is not supported on current asic\n", adapt->opt.gfx_sched_mode);
		}
	}

	return 0;
}

int amdgv_gpuiov_init(struct amdgv_adapter *adapt)
{
	if (adapt->opt.allow_time_cmd_complete == 0) {
		if (!((adapt->flags & AMDGV_FLAG_SIM_MODE) || (adapt->flags & AMDGV_FLAG_EMU_MODE)))
			adapt->gpuiov.allow_time_cmd_complete = 500 * 1000;
		else
			adapt->gpuiov.allow_time_cmd_complete = 4 * 60 * 1000 * 1000;
	} else
		adapt->gpuiov.allow_time_cmd_complete =
			(uint64_t)adapt->opt.allow_time_cmd_complete * 1000;

	AMDGV_INFO("allowed time for gpuiov cmd completion is %dms\n",
		   adapt->gpuiov.allow_time_cmd_complete / 1000);

	return 0;
}

int amdgv_gpuiov_fini(struct amdgv_adapter *adapt)
{
	return 0;
}
