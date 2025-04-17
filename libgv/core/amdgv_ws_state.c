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

//#define DEBUG_WS_STATE_MACHINE
//#define DEBUG_WS_STATE_MACHINE_AUTO
//#define DEBUG_WS_STATE_MACHINE_MANUAL

#define DEBUG_WS_STATE_MACHINE_MANUAL_LOG (defined(DEBUG_WS_STATE_MACHINE) || defined(DEBUG_WS_STATE_MACHINE_MANUAL))
#define DEBUG_WS_STATE_MACHINE_AUTO_LOG (defined(DEBUG_WS_STATE_MACHINE) || defined(DEBUG_WS_STATE_MACHINE_AUTO))

#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "amdgv_diag_data_trace_log.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

static int amdgv_world_switch_do_enable_auto_sched(struct amdgv_adapter *adapt,
						   uint32_t hw_sched_id, uint32_t cur_vf,
						   uint32_t cur_state, uint32_t target_vf)
{
	int i;
	uint32_t active_vfs;
	/* do not enable if there is no active VF */
	amdgv_gpuiov_get_active_vfs(adapt, hw_sched_id, &active_vfs);
	if (active_vfs != 0) {
		/* If cannot set debug dump by sysnode, automatically enable it when enable_auto_sched. */
		if (adapt->flags & AMDGV_FLAG_USE_PF &&
			adapt->flags & AMDGV_FLAG_DEBUG_DUMP_ENABLE) {
			amdgv_sched_set_auto_sched_debug_log(adapt, AMDGV_AUTO_SCHED_DEBUG_DUMP, true);
		}
		if (amdgv_gpuiov_enable_auto_sched(adapt, hw_sched_id)) {
			AMDGV_ERROR("WSSM: Failed to move from %s to AUTO state for VF%d\n",
				    amdgv_gpuiov_cmd_to_name(adapt, cur_state, hw_sched_id), cur_vf);
			return -1;
		}
	}
	adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id = target_vf;
	adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state = AMDGV_ENABLE_AUTO_HW_SWITCH;
	for (i = 0; i < AMDGV_MAX_VF_SLOT; ++i) { /* Set all to RUN */
		if (adapt->array_vf[i].auto_run == AMDGV_WS_AUTO_RUN_ENABLED)
			adapt->sched.array_vf[i].cur_vf_state[hw_sched_id] = AMDGV_RUN_GPU;
	}
	return 0;
}

#define cmd_allow_time() adapt->gpuiov.allow_time_cmd_complete

static int world_switch_get_next_state(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					uint32_t target_vf, uint32_t target_state,
					uint32_t cur_state, uint32_t cur_vf,
					uint32_t *next_state, uint32_t *next_vf)
{
	switch (cur_state) {
	case AMDGV_IDLE_GPU: /* IDLE must go to SAVE */
		*next_state = AMDGV_SAVE_GPU_STATE;
		*next_vf = cur_vf;
		break;

	/*
	* Complex state changes from SAVE / SHUTDOWN states
	*
	*      IDLE -> SAVE ----\ /-- INIT --\
	*                        *            * RUN
	*    SAVE -> SHUTDOWN --/ \-- LOAD --/
	*
	* ie SAVE of one VF can go to INIT of different VF or LOAD of same VF state
	* SHUTDOWN can also go to either INIT or LOAD state
	* Both INIT and LOAD go to RUN state.
	*
	* The decision is based on the current state of the specific VF
	*
	* Note that transition from SAVE or SHUTDOWN is almost the same.
	* The big difference is that only SAVE can change to SHUTDOWN.
	* Check SAVE --> SHUTDOWN first (and separately)
	* By the very nature of the flow, SHUTDOWN can only be reached as a target
	* state
	*
	*/
	case AMDGV_SAVE_GPU_STATE:
		if (target_state == AMDGV_SHUTDOWN_GPU && cur_vf == target_vf) {
			*next_state = AMDGV_SHUTDOWN_GPU;
			*next_vf = cur_vf;
			break; // We are done.
		} else if (target_state == AMDGV_SHUTDOWN_GPU && cur_vf != target_vf
				&& is_suspend_vf(target_vf)) {
			*next_state = AMDGV_SHUTDOWN_GPU;
			*next_vf = target_vf;
			break; // we are done.
		}
		/* NOTE:  --> No break; statement
		* The break; statement is explicitly not present here so that if the
		* transition is not SAVE --> SHUTDOWN it can be processed as common
		* code with the SHUTDOWN
		* --> case See explanation above
		*/

		// fall through
		goto shutdown_gpu;
	case AMDGV_SHUTDOWN_GPU: /* Note this is SHUTDOWN and SAVE case */
shutdown_gpu:
		if (target_state == AMDGV_SHUTDOWN_GPU && cur_vf != target_vf &&
		adapt->sched.array_vf[target_vf].cur_vf_state[hw_sched_id] ==
			AMDGV_SAVE_GPU_STATE) {
			*next_state = AMDGV_SHUTDOWN_GPU;
		} else if (adapt->sched.array_vf[target_vf].cur_vf_state[hw_sched_id] ==
			AMDGV_SAVE_GPU_STATE) {
			*next_state = AMDGV_LOAD_GPU_STATE;
		} else {
			*next_state = AMDGV_INIT_GPU;
		}
		*next_vf = target_vf;
		break;

	/* Both INIT and LOAD must go to RUN state */
	case AMDGV_LOAD_GPU_STATE:
	case AMDGV_INIT_GPU:
		*next_state = AMDGV_RUN_GPU;
		*next_vf = cur_vf;
		break;

	/* RUN goes to IDLE */
	case AMDGV_RUN_GPU:
		*next_state = AMDGV_IDLE_GPU;
		*next_vf = cur_vf;
		break;

	default:
		AMDGV_ERROR(
			"MANUAL: Unknown current GPU state %d on %s. Marking scheduler as abnormal.\n",
			adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state,
			amdgv_hw_sched_id_to_name(adapt, hw_sched_id));
		return AMDGV_FAILURE;
	}

	return 0;
}

/*
 * World Switch State Machine for engines running in Manual mode such as Gfx.
 */
int world_switch_bulk_goto_state_manual(struct amdgv_adapter *adapt, uint32_t target_vf,
					  uint32_t hw_sched_mask,
					  uint32_t target_state)
{
	int ret = 0;
#if DEBUG_WS_STATE_MACHINE_MANUAL_LOG
	const char *psched;
#endif
	uint32_t loop_okay = 20 * adapt->gpuiov.num_ctrl_blocks;
	uint32_t hw_sched_id;
	uint32_t initial_hw_sched_mask = hw_sched_mask;
	uint32_t bad_hw_sched_mask = 0;
	uint64_t histogram_time_start[AMDGV_MAX_NUM_HW_SCHED];
	uint32_t next_vf[AMDGV_MAX_NUM_HW_SCHED] = { 0 };
	uint32_t next_state[AMDGV_MAX_NUM_HW_SCHED] = { 0 };
	uint64_t wait_start, wait_delta, wait_left;
	uint32_t wait_hw_sched_mask;
	uint32_t completed_hw_sched_id = 0;

#if DEBUG_WS_STATE_MACHINE_MANUAL_LOG
	for_each_id(hw_sched_id, hw_sched_mask) {
		psched = amdgv_hw_sched_id_to_name(adapt, hw_sched_id);
		AMDGV_INFO(
			"MANUAL:  (*%s*) Request %s state for VF%d, current GPU(%s) VF(%s)\n", psched,
			amdgv_gpuiov_cmd_to_name(adapt, target_state, hw_sched_id), target_vf,
			amdgv_gpuiov_cmd_to_name(adapt, adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state, hw_sched_id),
			amdgv_gpuiov_cmd_to_name(adapt, adapt->sched.array_vf[target_vf].cur_vf_state[hw_sched_id], hw_sched_id));
	}
#endif

	for_each_id(hw_sched_id, initial_hw_sched_mask) {
		if (oss_rwsema_write_trylock(adapt->sched.hw_state_machine[hw_sched_id].ws_lock)) {
			amdgv_put_error(target_vf, AMDGV_ERROR_IOV_WS_REENTRANT_ERROR, 0);
			return AMDGV_ERROR_IOV_WS_REENTRANT_ERROR;
		}
	}

next_goto_state:

	/* 1) Get the next State */
	for_each_id(hw_sched_id, hw_sched_mask) {
		if (adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state == target_state &&
			adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id == target_vf) {
				//State change done. Take hw_sched out of mask
				hw_sched_mask &= ~(1 << hw_sched_id);
				continue;
		}

		ret = world_switch_get_next_state(adapt, hw_sched_id,
				target_vf, target_state,
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state,
				adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id,
				&(next_state[hw_sched_id]), &(next_vf[hw_sched_id]));

		if (ret) {
			/* Mark HW sched as bad and continue */
			bad_hw_sched_mask |= (1 << hw_sched_id);
			hw_sched_mask &= ~(1 << hw_sched_id);
		}

		if (!(loop_okay--)) {
			AMDGV_ERROR("Potential Infinite loop.  Too long in state machine\n");
			ret = AMDGV_ERROR_IOV_WS_INFINITE_LOOP;
			goto ws_exit;
		}
	}

	/* 2) Send cmds to FW */
	for_each_id(hw_sched_id, hw_sched_mask) {
#if DEBUG_WS_STATE_MACHINE_MANUAL_LOG
		psched = amdgv_hw_sched_id_to_name(adapt, hw_sched_id);
		AMDGV_INFO("MANUAL:    %s - Switch from VF%d(%s) to VF%d(%s)\n",
			psched,
			adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id,
			amdgv_gpuiov_cmd_to_name(adapt, adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state, hw_sched_id),
			next_vf[hw_sched_id], amdgv_gpuiov_cmd_to_name(adapt, next_state[hw_sched_id], hw_sched_id));
#endif
		/* diagnosis data trace log entry */
		AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id,
		hw_sched_id, next_state[hw_sched_id]);

		histogram_time_start[hw_sched_id] = oss_get_time_stamp();

		switch (next_state[hw_sched_id]) {
		case AMDGV_IDLE_GPU:
			ret = amdgv_gpuiov_idle_vf_no_wait(adapt, next_vf[hw_sched_id], hw_sched_id);
			break;
		case AMDGV_RUN_GPU:
			ret = amdgv_gpuiov_run_vf_no_wait(adapt, next_vf[hw_sched_id], hw_sched_id);
			break;
		case AMDGV_SAVE_GPU_STATE:
			ret = amdgv_gpuiov_save_vf_no_wait(adapt, next_vf[hw_sched_id], hw_sched_id);
			break;
		case AMDGV_INIT_GPU:
			ret = amdgv_gpuiov_init_vf_no_wait(adapt, next_vf[hw_sched_id], hw_sched_id);
			break;
		case AMDGV_LOAD_GPU_STATE:
			ret = amdgv_gpuiov_load_vf_no_wait(adapt, next_vf[hw_sched_id], hw_sched_id);
			break;
		case AMDGV_SHUTDOWN_GPU:
			ret = amdgv_gpuiov_shutdown_vf_no_wait(adapt, next_vf[hw_sched_id], hw_sched_id);
			break;
		default:
			AMDGV_ERROR(
				"MANUAL: Unknown next GPU state %d on %s. Marking scheduler as abnormal.\n",
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state,
				amdgv_hw_sched_id_to_name(adapt, hw_sched_id));
			ret = AMDGV_ERROR_IOV_CMD_ERROR;
			break;
		}

		if (ret) {
			/* Mark HW SCHED as bad and continue */
			bad_hw_sched_mask |= (1 << hw_sched_id);
			hw_sched_mask &= ~(1 << hw_sched_id);
		}
	}

	wait_start = oss_get_time_stamp();
	wait_left = cmd_allow_time();
	wait_hw_sched_mask = hw_sched_mask;

	/* 3) Wait for all engines to finish */
	while (wait_hw_sched_mask) {
		ret = wait_for_first_cmd_complete(adapt,
			wait_hw_sched_mask, &completed_hw_sched_id, wait_left);
		wait_delta = oss_get_time_stamp() - wait_start;

		//Remove hung schedulers from mask
		if (ret || (wait_delta >= cmd_allow_time())) {
			bad_hw_sched_mask |= wait_hw_sched_mask;
			hw_sched_mask &= ~(wait_hw_sched_mask);

			for_each_id(hw_sched_id, wait_hw_sched_mask) {
				AMDGV_ERROR(
					"WSSM: %s Timeout moving from VF%d(%s) to VF%d(%s)\n",
					amdgv_hw_sched_id_to_name(adapt, hw_sched_id),
					adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id,
					amdgv_gpuiov_cmd_to_name(
						adapt, adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state, hw_sched_id),
					next_vf[hw_sched_id],
					amdgv_gpuiov_cmd_to_name(adapt, next_state[hw_sched_id], hw_sched_id));
			}

			amdgv_sched_dump_gpu_state(adapt);
			goto wait_out;
		}

		wait_left = cmd_allow_time() - wait_delta;

		adapt->sched.hw_state_machine[completed_hw_sched_id].cur_gpu_state = next_state[completed_hw_sched_id];
		adapt->sched.hw_state_machine[completed_hw_sched_id].cur_vf_id = next_vf[completed_hw_sched_id];
		adapt->sched.array_vf[next_vf[completed_hw_sched_id]].cur_vf_state[completed_hw_sched_id] =
								next_state[completed_hw_sched_id];

		amdgv_update_histogram(adapt, next_vf[completed_hw_sched_id], completed_hw_sched_id,
			next_state[completed_hw_sched_id], histogram_time_start[completed_hw_sched_id]);

#ifdef WS_RECORD
		switch (next_state[completed_hw_sched_id]) {
		case AMDGV_IDLE_GPU:
			amdgv_gpuiov_record_queue_push(adapt, next_vf[completed_hw_sched_id], completed_hw_sched_id, AMDGV_RECORD_IDLE_END);
			break;
		case AMDGV_RUN_GPU:
			amdgv_gpuiov_record_queue_push(adapt, next_vf[completed_hw_sched_id], completed_hw_sched_id, AMDGV_RECORD_RUN_END);
			break;
		case AMDGV_SAVE_GPU_STATE:
			amdgv_gpuiov_record_queue_push(adapt, next_vf[completed_hw_sched_id], completed_hw_sched_id, AMDGV_RECORD_SAVE_END);
			break;
		case AMDGV_INIT_GPU:
			amdgv_gpuiov_record_queue_push(adapt, next_vf[completed_hw_sched_id], completed_hw_sched_id, AMDGV_RECORD_INIT_END);
			break;
		case AMDGV_LOAD_GPU_STATE:
			amdgv_gpuiov_record_queue_push(adapt, next_vf[completed_hw_sched_id], completed_hw_sched_id, AMDGV_RECORD_LOAD_END);
			break;
		case AMDGV_SHUTDOWN_GPU:
			amdgv_gpuiov_record_queue_push(adapt, next_vf[completed_hw_sched_id], completed_hw_sched_id, AMDGV_RECORD_SHUTDOWN_END);
			break;
		default:
			AMDGV_ERROR("Invalid WS record entry\n");
			break;
		}
#endif

		wait_hw_sched_mask &= ~(1 << completed_hw_sched_id);
	}

wait_out:
	/* 4) Restart the loop */
	if (hw_sched_mask)
		goto next_goto_state;

ws_exit:
	if (bad_hw_sched_mask)
		ret = AMDGV_FAILURE;

	for_each_id (hw_sched_id, initial_hw_sched_mask) {
		oss_rwsema_write_unlock(adapt->sched.hw_state_machine[hw_sched_id].ws_lock);
	}
	return ret;
}

/*
 * World Switch State Machine for engines running in Manual mode such as Gfx.
 */
static int world_switch_goto_state_manual(struct amdgv_adapter *adapt, uint32_t target_vf,
					  uint32_t hw_sched_id, uint32_t target_state)
{
	int ret = 0;
#if DEBUG_WS_STATE_MACHINE_MANUAL_LOG
	const char *psched;
#endif
	uint32_t cur_vf = 0;
	uint32_t cur_state;
	uint32_t next_vf = 0;
	uint32_t next_state = 0;
	uint32_t loop_okay = 20;

	if (oss_rwsema_write_trylock(adapt->sched.hw_state_machine[hw_sched_id].ws_lock)) {
		amdgv_put_error(target_vf, AMDGV_ERROR_IOV_WS_REENTRANT_ERROR, 0);
		return AMDGV_ERROR_IOV_WS_REENTRANT_ERROR;
	}

#if DEBUG_WS_STATE_MACHINE_MANUAL_LOG
	psched = amdgv_hw_sched_id_to_name(adapt, hw_sched_id);

	AMDGV_INFO("MANUAL:  (*%s*) Request %s state for VF%d, current GPU(%s) VF(%s)\n",
		   psched, amdgv_gpuiov_cmd_to_name(adapt, target_state, hw_sched_id), target_vf,
		   amdgv_gpuiov_cmd_to_name(
			   adapt, adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state,
			   hw_sched_id),
		   amdgv_gpuiov_cmd_to_name(
			   adapt, adapt->sched.array_vf[target_vf].cur_vf_state[hw_sched_id],
			   hw_sched_id));
#endif

	while (adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state != target_state ||
	       adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id != target_vf) {
		cur_vf = adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id;
		cur_state = adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state;
		next_vf = cur_vf;

		if (!(loop_okay--)) {
			AMDGV_ERROR("Potential Infinite loop.  Too long in state machine\n");
			ret = AMDGV_ERROR_IOV_WS_INFINITE_LOOP;
			goto ws_exit;
		}

		switch (cur_state) {
		case AMDGV_IDLE_GPU: /* IDLE must go to SAVE */
			next_state = AMDGV_SAVE_GPU_STATE;
			break;

		/*
		 * Complex state changes from SAVE / SHUTDOWN states
		 *
		 *      IDLE -> SAVE ----\ /-- INIT --\
		 *                        *            * RUN
		 *    SAVE -> SHUTDOWN --/ \-- LOAD --/
		 *
		 * ie SAVE of one VF can go to INIT of different VF or LOAD of same VF state
		 * SHUTDOWN can also go to either INIT or LOAD state
		 * Both INIT and LOAD go to RUN state.
		 *
		 * The decision is based on the current state of the specific VF
		 *
		 * Note that transition from SAVE or SHUTDOWN is almost the same.
		 * The big difference is that only SAVE can change to SHUTDOWN.
		 * Check SAVE --> SHUTDOWN first (and separately)
		 * By the very nature of the flow, SHUTDOWN can only be reached as a target
		 * state
		 *
		 */
		case AMDGV_SAVE_GPU_STATE:
			if (target_state == AMDGV_SHUTDOWN_GPU && cur_vf == target_vf) {
				next_state = AMDGV_SHUTDOWN_GPU;
				break; // We are done.
			} else if (target_state == AMDGV_SHUTDOWN_GPU && cur_vf != target_vf &&
				   is_suspend_vf(target_vf)) {
				next_state = AMDGV_SHUTDOWN_GPU;
				next_vf = target_vf;
				break; // we are done.
			}
			/* NOTE:  --> No break; statement
			 * The break; statement is explicitly not present here so that if the
			 * transition is not SAVE --> SHUTDOWN it can be processed as common
			 * code with the SHUTDOWN
			 * --> case See explanation above
			 */

			// fall through
			goto shutdown_gpu;
		case AMDGV_SHUTDOWN_GPU: /* Note this is SHUTDOWN and SAVE case */
shutdown_gpu:
			if (target_state == AMDGV_SHUTDOWN_GPU && cur_vf != target_vf &&
			    adapt->sched.array_vf[target_vf].cur_vf_state[hw_sched_id] ==
				    AMDGV_SAVE_GPU_STATE) {
				next_state = AMDGV_SHUTDOWN_GPU;
			} else if (adapt->sched.array_vf[target_vf].cur_vf_state[hw_sched_id] ==
				   AMDGV_SAVE_GPU_STATE) {
				next_state = AMDGV_LOAD_GPU_STATE;
			} else {
				next_state = AMDGV_INIT_GPU;
			}
			next_vf = target_vf;
			break;

		/* Both INIT and LOAD must go to RUN state */
		case AMDGV_LOAD_GPU_STATE:
		case AMDGV_INIT_GPU:
			next_state = AMDGV_RUN_GPU;
			break;

		/* RUN goes to IDLE */
		case AMDGV_RUN_GPU:
			next_state = AMDGV_IDLE_GPU;
			break;

		default:
			AMDGV_WARN(
				"MANUAL: Unknown current GPU state %d.  Don't know how to continue\n",
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state);
			/* assign an invalid state */
			next_state = 0;
			ret = AMDGV_ERROR_IOV_CMD_ERROR;
			goto ws_exit;
		}

#if DEBUG_WS_STATE_MACHINE_MANUAL_LOG
		AMDGV_INFO("MANUAL:    %s - Switch from VF%d(%s) to VF%d(%s)\n", psched,
			   cur_vf,
			   amdgv_gpuiov_cmd_to_name(
				   adapt, adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state,
				   hw_sched_id),
			   next_vf, amdgv_gpuiov_cmd_to_name(adapt, next_state, hw_sched_id));
#endif
		/* diagnosis data trace log entry */
		AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(cur_vf, hw_sched_id, next_state);
		switch (next_state) {
		case AMDGV_IDLE_GPU:
			if (amdgv_gpuiov_idle_vf(adapt, next_vf, hw_sched_id)) {
				ret = AMDGV_ERROR_IOV_WS_IDLE_TIMEOUT;
				goto ws_exit;
			}
			break;
		case AMDGV_RUN_GPU:
			if (amdgv_gpuiov_run_vf(adapt, next_vf, hw_sched_id)) {
				ret = AMDGV_ERROR_IOV_WS_RUN_TIMEOUT;
				goto ws_exit;
			}
			break;
		case AMDGV_SAVE_GPU_STATE:
			if (amdgv_gpuiov_save_vf(adapt, next_vf, hw_sched_id)) {
				ret = AMDGV_ERROR_IOV_WS_SAVE_TIMEOUT;
				goto ws_exit;
			}
			break;
		case AMDGV_INIT_GPU:
			if (amdgv_gpuiov_init_vf(adapt, next_vf, hw_sched_id)) {
				ret = AMDGV_ERROR_IOV_WS_LOAD_TIMEOUT;
				goto ws_exit;
			}
			break;
		case AMDGV_LOAD_GPU_STATE:
			if (amdgv_gpuiov_load_vf(adapt, next_vf, hw_sched_id)) {
				ret = AMDGV_ERROR_IOV_WS_LOAD_TIMEOUT;
				goto ws_exit;
			}
			break;
		case AMDGV_SHUTDOWN_GPU:
			if (amdgv_gpuiov_shutdown_vf(adapt, next_vf, hw_sched_id)) {
				ret = AMDGV_ERROR_IOV_WS_SHUTDOWN_TIMEOUT;
				goto ws_exit;
			}
			break;
		default:
			AMDGV_WARN(
				"MANUAL: Unknown next GPU state %d.  Don't know how to continue\n",
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state);
			ret = AMDGV_ERROR_IOV_CMD_ERROR;
			goto ws_exit;
		}
		adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state = next_state;
		adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id = next_vf;
	}

ws_exit:
	if (ret) {
		AMDGV_ERROR("WSSM: Failed to move from VF%d(%s) to VF%d(%s)\n", cur_vf,
			    amdgv_gpuiov_cmd_to_name(
				    adapt, adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state,
				    hw_sched_id),
			    next_vf, amdgv_gpuiov_cmd_to_name(adapt, next_state, hw_sched_id));

		amdgv_sched_dump_gpu_state(adapt);
	}

	oss_rwsema_write_unlock(adapt->sched.hw_state_machine[hw_sched_id].ws_lock);
	return ret;
}

/*
 * World Switch State Machine to support engines that are running in auto-scheduler mode
 */
static int world_switch_goto_state_auto(struct amdgv_adapter *adapt, uint32_t target_vf,
					uint32_t hw_sched_id, uint32_t target_state)
{
	int ret = 0;
	int i;
	int auto_needs_idle;
	const char *psched;
	uint32_t cur_vf;
	uint32_t cur_state;
	uint32_t loop_okay = 20;
	uint32_t hw_vf_state;
	struct amdgv_sched_world_switch *world_switch;
	uint32_t active_vfs;

	if (amdgv_sched_get_world_switch_by_hw_sched_id(adapt, hw_sched_id, &world_switch))
		return AMDGV_FAILURE;

	if (oss_rwsema_write_trylock(adapt->sched.hw_state_machine[hw_sched_id].ws_lock)) {
		amdgv_put_error(target_vf, AMDGV_ERROR_IOV_WS_REENTRANT_ERROR, 0);
		return AMDGV_ERROR_IOV_WS_REENTRANT_ERROR;
	}

	psched = amdgv_hw_sched_id_to_name(adapt, hw_sched_id);

	if (target_vf == -1 && adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state !=
				       AMDGV_ENABLE_AUTO_HW_SWITCH) {
		amdgv_gpuiov_get_active_vf_idx(adapt, hw_sched_id, &target_vf);

		// FIX This may be wrong??? Save intermediate state locally?
		world_switch->curr_idx_vf = target_vf;
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
		AMDGV_INFO("Target VF is not known, get from hardware\n");
		AMDGV_INFO("  This requires a fix to MMSCH\n");
		AMDGV_INFO("Use target_vf = VF%d\n", target_vf);
#endif
	}

#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
	AMDGV_INFO(
		"AUTO:  (*%s*) Request %s state for VF%d, current GPU(%s) VF(%s)\n", psched,
		amdgv_gpuiov_cmd_to_name(adapt, target_state, hw_sched_id), target_vf,
		amdgv_gpuiov_cmd_to_name(
			adapt, adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state, hw_sched_id),
		target_vf == -1 ? "UNKNOWN" :
				  amdgv_gpuiov_cmd_to_name(
					  adapt, adapt->sched.array_vf[target_vf].cur_vf_state[hw_sched_id],
					  hw_sched_id));
#endif

	while (adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state != target_state ||
	       adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id != target_vf) {
		cur_vf = adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id;
		cur_state = adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state;

		if (!(loop_okay--)) {
			AMDGV_ERROR("Potential Infinite loop.  Too long in state machine\n");
			ret = AMDGV_ERROR_IOV_WS_INFINITE_LOOP;
			goto ws_exit;
		}

		switch (cur_state) {
		case AMDGV_IDLE_GPU: /* IDLE must go to SAVE */
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
			AMDGV_INFO("AUTO:    %s - Switch from VF%d(IDLE) to VF%d(SAVE)\n",
				   psched, cur_vf, cur_vf);
#endif
			/* diagnosis data trace log entry */
			AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(cur_vf, hw_sched_id,
						       AMDGV_SAVE_GPU_STATE);

			if (amdgv_gpuiov_save_vf(adapt, cur_vf, hw_sched_id)) {
				AMDGV_ERROR(
					"WSSM: Failed to move from IDLE to SAVE state for VF%d\n",
					cur_vf);
				ret = AMDGV_ERROR_IOV_WS_SAVE_TIMEOUT;
				goto ws_exit;
			}
			adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
				AMDGV_SAVE_GPU_STATE;
			break;

		/*
		 * Complex state changes from SAVE / SHUTDOWN states
		 *
		 *      IDLE -> SAVE ----\ /-- INIT --\
		 *                        *            * RUN
		 *    SAVE -> SHUTDOWN --/ \-- LOAD --/
		 *
		 * ie SAVE of one VF can go to INIT of different VF or LOAD of same VF state
		 * SHUTDOWN can also go to either INIT or LOAD state
		 * Both INIT and LOAD go to RUN state.
		 *
		 * The decision is based on the current state of the specific VF
		 *
		 * Note that transition from SAVE or SHUTDOWN is almost the same.
		 * The big difference is that only SAVE can change to SHUTDOWN.
		 * Check SAVE --> SHUTDOWN first (and separately)
		 * By the very nature of the flow, SHUTDOWN can only be reached as a target
		 * state
		 *
		 */
		/* Due to feature in MMSCH SAVE can also go to ENABLE auto switch */
		case AMDGV_SAVE_GPU_STATE:
			if (target_state == AMDGV_SHUTDOWN_GPU && cur_vf == target_vf) {
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
				AMDGV_INFO(
					"AUTO:    %s - Switch from VF%d(SAVE) to VF%d(SHUTDOWN)\n",
					psched, cur_vf, cur_vf);
#endif
				/* diagnosis data trace log entry */
				AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(cur_vf, hw_sched_id,
							       AMDGV_SHUTDOWN_GPU);

				if (amdgv_gpuiov_shutdown_vf(adapt, cur_vf, hw_sched_id)) {
					AMDGV_ERROR(
						"WSSM: Failed to move from SAVE to SHUTDOWN state for VF%d\n",
						cur_vf);
					ret = AMDGV_ERROR_IOV_WS_SHUTDOWN_TIMEOUT;
					goto ws_exit;
				}
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
					AMDGV_SHUTDOWN_GPU;
				break; // We are done.
			}
			/* This needs to be moved to RUN case */
			if (target_state == AMDGV_ENABLE_AUTO_HW_SWITCH) {
				if (IS_HW_SCHED_TYPE_GFX(hw_sched_id) && (adapt->sched.num_vf_per_gfx_sched == 1)
					&& (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH)) {
					target_state = AMDGV_RUN_GPU;
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
					AMDGV_INFO("AUTO:    %s - Self Switch is Disabled, Replace Target State with RUN\n", psched);
#endif
					goto load_gpu;
				}
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
				AMDGV_INFO(
					"AUTO:    %s - Switch from VF%d(SAVE) to VF%d(ENABLE_AUTO)\n",
					psched, cur_vf, cur_vf);
#endif
				/* diagnosis data trace log entry */
				for (i = 0; i < adapt->num_vf; ++i) {
					AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(i, hw_sched_id,
								       AMDGV_RUN_GPU);
				}

				if (amdgv_world_switch_do_enable_auto_sched(adapt, hw_sched_id,
									    cur_vf, cur_state,
									    target_vf)) {
					AMDGV_ERROR(
						"WSSM: Failed to move from SAVE to AUTO state for VF%d\n",
						cur_vf);
					ret = AMDGV_FAILURE;
					goto ws_exit;
				}
				break;
			}

			/* NOTE:  --> No break; statement
			 * The break; statement is explicitly not present here so that if the
			 * transition is not SAVE --> SHUTDOWN it can be processed as common
			 * code with the SHUTDOWN
			 * --> case See explanation above
			 */

			// fall through
			goto shutdown_gpu;
		case AMDGV_SHUTDOWN_GPU: /* Note this is SHUTDOWN and SAVE case */
shutdown_gpu:
			if (cur_state == AMDGV_SHUTDOWN_GPU &&
			    target_state == AMDGV_ENABLE_AUTO_HW_SWITCH) {
				if (IS_HW_SCHED_TYPE_GFX(hw_sched_id) && (adapt->sched.num_vf_per_gfx_sched == 1)
					&& (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH)) {
					target_state = AMDGV_RUN_GPU;
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
					AMDGV_INFO("AUTO:    %s - Self Switch is Disabled, Replace Target State with RUN\n", psched);
#endif
					goto load_gpu;
				}
				if (amdgv_world_switch_do_enable_auto_sched(adapt, hw_sched_id,
									    cur_vf, cur_state,
									    target_vf)) {
					AMDGV_ERROR(
						"WSSM: Failed to move from SHUTDOWN to AUTO state for VF%d\n",
						cur_vf);
					ret = AMDGV_FAILURE;
					goto ws_exit;
				}
				break;
			}
			if (target_vf >= AMDGV_MAX_VF_SLOT) {
				AMDGV_ERROR("The VF %d exceeds the boudary", target_vf);
				ret = AMDGV_FAILURE;
				goto ws_exit;
			}
			if (adapt->sched.array_vf[target_vf].cur_vf_state[hw_sched_id] ==
				    AMDGV_SAVE_GPU_STATE &&
					(target_vf == AMDGV_PF_IDX ? (world_switch->vf_inited & 1 << target_vf)
						: is_active_vf(target_vf))) {
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
				AMDGV_INFO(
					"AUTO:    %s - Switch from VF%d(%s) to VF%d(LOAD)\n",
					psched, cur_vf,
					adapt->sched.array_vf[cur_vf].cur_vf_state[hw_sched_id] ==
							AMDGV_SAVE_GPU_STATE ?
						"SAVE" :
						"SHUTDOWN",
					target_vf);
#endif
				/* diagnosis data trace log entry */
				AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(target_vf, hw_sched_id,
							       AMDGV_LOAD_GPU_STATE);

load_gpu:
				if (amdgv_gpuiov_load_vf(adapt, target_vf, hw_sched_id)) {
					AMDGV_ERROR("WSSM: Failed to LOAD VF%d", target_vf);
					ret = AMDGV_ERROR_IOV_WS_LOAD_TIMEOUT;
					goto ws_exit;
				}
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
					AMDGV_LOAD_GPU_STATE;
			} else {
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
				AMDGV_INFO(
					"AUTO:    %s - Switch from VF%d(%s) to VF%d(INIT)\n",
					psched, cur_vf,
					adapt->sched.array_vf[cur_vf].cur_vf_state[hw_sched_id] ==
							AMDGV_SAVE_GPU_STATE ?
						"SAVE" :
						"SHUTDOWN",
					target_vf);
#endif
				/* diagnosis data trace log entry */
				AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(target_vf, hw_sched_id,
							       AMDGV_INIT_GPU);

				if (amdgv_gpuiov_init_vf(adapt, target_vf, hw_sched_id)) {
					AMDGV_ERROR("WSSM: Failed to INIT VF%d\n", target_vf);
					ret = AMDGV_ERROR_IOV_WS_LOAD_TIMEOUT;
					goto ws_exit;
				}
				if ((adapt->bp_mode == AMDGV_BP_MODE_1) && (AMDGV_PF_IDX == target_vf)) {
					AMDGV_INFO("Scheduler stop on first PF INIT\n");
					amdgv_sched_queue_suspend(adapt);
				}
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
					AMDGV_INIT_GPU;
			}
			adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id = target_vf;
			break;

		/* Both INIT and LOAD must go to RUN state */
		case AMDGV_LOAD_GPU_STATE:
		case AMDGV_INIT_GPU:
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
			AMDGV_INFO("AUTO:    %s - Switch from VF%d(%s) to VF%d(RUN)\n", psched,
				   cur_vf,
				   adapt->sched.array_vf[cur_vf].cur_vf_state[hw_sched_id] ==
						   AMDGV_LOAD_GPU_STATE ?
					   "LOAD" :
					   "INIT",
				   cur_vf);
#endif
			/* diagnosis data trace log entry */
			AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(cur_vf, hw_sched_id, AMDGV_RUN_GPU);

			if (amdgv_gpuiov_run_vf(adapt, cur_vf, hw_sched_id)) {
				AMDGV_ERROR("WSSM: Failed to switch to RUN state for VF%d\n",
					    cur_vf);
				ret = AMDGV_ERROR_IOV_WS_RUN_TIMEOUT;
				goto ws_exit;
			}
			adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
				AMDGV_RUN_GPU;
			break;

		/* RUN can go to IDLE or to Enable_auto switch */
		/* Due to MMSCH feature, RUN --> ENABLE is not implemented yet */
		case AMDGV_RUN_GPU:
			if (target_state == AMDGV_ENABLE_AUTO_HW_SWITCH &&
					world_switch->sched_block != AMDGV_SCHED_BLOCK_GFX) {
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
				AMDGV_INFO(
					"AUTO:    %s - Switch from VF%d(RUN) to VF%d(ENABLE_AUTO)\n",
					psched, cur_vf, cur_vf);
#endif
				/* diagnosis data trace log entry */
				AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(cur_vf, hw_sched_id,
							       AMDGV_ENABLE_AUTO_HW_SWITCH);

				if (amdgv_world_switch_do_enable_auto_sched(adapt, hw_sched_id,
									    cur_vf, cur_state,
									    target_vf)) {
					AMDGV_ERROR(
						"WSSM: Failed to move from RUN to AUTO state for VF%d\n",
						cur_vf);
					ret = AMDGV_FAILURE;
					goto ws_exit;
				}
			} else {
				if (IS_HW_SCHED_TYPE_GFX(hw_sched_id) && (adapt->sched.num_vf_per_gfx_sched == 1)
					&& (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH)
					&& (target_state == AMDGV_ENABLE_AUTO_HW_SWITCH)) {
					target_state = AMDGV_RUN_GPU;
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
				AMDGV_INFO("AUTO:    %s - Self Switch is Disabled, Replace Target State with RUN\n", psched);
#endif
				}
				/* diagnosis data trace log entry */
				AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(cur_vf, hw_sched_id,
								AMDGV_IDLE_GPU);
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
				AMDGV_INFO("AUTO:    %s - Switch from VF%d(RUN) to VF%d(IDLE)\n",
					psched, cur_vf, cur_vf);
#endif
				if (amdgv_gpuiov_idle_vf(adapt, cur_vf, hw_sched_id)) {
					AMDGV_ERROR("WSSM: Failed to IDLE VF%d\n", cur_vf);
					ret = AMDGV_ERROR_IOV_WS_IDLE_TIMEOUT;
					goto ws_exit;
				}
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
					AMDGV_IDLE_GPU;
			}
			/* Mark world switch stopped so it could be reactivated later. */
			world_switch->switch_running = false;
			break;

		case AMDGV_ENABLE_AUTO_HW_SWITCH:
#if DEBUG_WS_STATE_MACHINE_AUTO_LOG
			AMDGV_INFO(
				"AUTO:    %s - Switch from ENABLE_AUTO to DISABLE_AUTO (Same as IDLE)\n",
				psched);
#endif
			amdgv_gpuiov_get_active_vfs(adapt, hw_sched_id, &active_vfs);
			if (active_vfs != 0) {
				if (IS_HW_SCHED_TYPE_GFX(hw_sched_id) && (adapt->sched.num_vf_per_gfx_sched == 1)
					&& (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH)
					&& (target_state == AMDGV_DISABLE_AUTO_HW_SCHED)) {
					adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
						AMDGV_RUN_GPU;
					target_state = AMDGV_SAVE_GPU_STATE;
					break;
				}
				if (amdgv_gpuiov_disable_auto_sched(adapt, hw_sched_id)) {
					AMDGV_ERROR(
						"WSSM: Failed to move from ENABLE_AUTO to DISABLE_AUTO state for VF%d\n",
						cur_vf);
					ret = AMDGV_FAILURE;
					amdgv_gpuiov_get_active_vf_idx(adapt, hw_sched_id, &cur_vf);
					adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id = cur_vf;
					world_switch->curr_idx_vf = cur_vf;
					goto ws_exit;
				}

				/* FW will not respond to GIM right away. Need delay to wait engine
				 * idle */
				if (amdgv_gpuiov_wait_auto_sched_stop(adapt, hw_sched_id)) {
					AMDGV_ERROR(
						"WSSM: Failed to wait engine idle after DISABLE_AUTO for VF%d\n",
						cur_vf);
					ret = AMDGV_FAILURE;
					amdgv_gpuiov_get_active_vf_idx(adapt, hw_sched_id, &cur_vf);
					adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id = cur_vf;
					world_switch->curr_idx_vf = cur_vf;
					goto ws_exit;
				}
				/* If cannot set debug dump by sysnode, automatically disable it when disable_auto_sched. */
				if (adapt->flags & AMDGV_FLAG_USE_PF &&
					adapt->flags & AMDGV_FLAG_DEBUG_DUMP_ENABLE)
					amdgv_sched_set_auto_sched_debug_log(adapt, AMDGV_AUTO_SCHED_DEBUG_DUMP, false);
				/*
				 * Cur VF is not known after leaving AUTO switch state
				 * Need to ask hardware
				 */
				amdgv_gpuiov_get_active_vf_idx(adapt, hw_sched_id, &cur_vf);
			} else {
				cur_vf = AMDGV_PF_IDX;
			}
			adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id = cur_vf;
			if (target_vf == -1)
				target_vf = cur_vf;
			// FIX This may be wrong???
			world_switch->curr_idx_vf = cur_vf;
			adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
				AMDGV_DISABLE_AUTO_HW_SCHED;
			/* Mark world switch stopped so it could be reactivated later. */
			world_switch->switch_running = false;
			break;

		case AMDGV_DISABLE_AUTO_HW_SCHED:
			auto_needs_idle = 0;
			amdgv_sched_world_context_get_hw_curr_state(adapt, hw_sched_id,
								    &hw_vf_state); // MMSCH fix

			if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX)
				hw_vf_state = AMDGV_VF_CONTEXT_SAVED;
			if (hw_vf_state != AMDGV_VF_CONTEXT_SAVED) {
				AMDGV_DEBUG(
					"AUTO:    %s - Need to IDLE VF%d after disabling auto-switch\n",
					psched, cur_vf);

				auto_needs_idle = 1;
				if (hw_vf_state != AMDGV_VF_CONTEXT_CLEAR) {
					/* diagnosis data trace log entry */
					AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(cur_vf, hw_sched_id,
								       AMDGV_IDLE_GPU);

					if (amdgv_gpuiov_idle_vf(adapt, cur_vf, hw_sched_id)) {
						AMDGV_ERROR("WSSM: Failed to IDLE VF%d\n",
							    cur_vf);
						ret = AMDGV_ERROR_IOV_WS_IDLE_TIMEOUT;
						goto ws_exit;
					}
				}
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
					AMDGV_IDLE_GPU;
			} else {
				AMDGV_DEBUG(
					"AUTO:    %s - All VFs in SAVE state after diabling auto-switch\n",
					psched);
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
					AMDGV_SAVE_GPU_STATE;
			}

			for (i = 0; i < AMDGV_MAX_VF_SLOT; ++i) { /* Set all to IDLE */
				if (adapt->array_vf[i].auto_run == AMDGV_WS_AUTO_RUN_ENABLED ||	/* Make sure vf_state of PF is correct whether active or not. */
					(i == AMDGV_PF_IDX && adapt->flags & AMDGV_FLAG_USE_PF)) {
					if ((i == cur_vf) && auto_needs_idle) {
						/* diagnosis data trace log entry */
						AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(i, hw_sched_id,
									       AMDGV_IDLE_GPU);

						adapt->sched.array_vf[i]
							.cur_vf_state[hw_sched_id] =
							AMDGV_IDLE_GPU;
						AMDGV_DEBUG(
							"AUTO:    %s - Mark VF%d as IDLE\n",
							psched, i);
					} else {
						/* diagnosis data trace log entry */
						AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(
							i, hw_sched_id, AMDGV_SAVE_GPU_STATE);

						adapt->sched.array_vf[i]
							.cur_vf_state[hw_sched_id] =
							AMDGV_SAVE_GPU_STATE;
						AMDGV_DEBUG(
							"AUTO:    %s - Mark VF%d as SAVE\n",
							psched, i);
					}
				}
			}

			break;

		default:
			AMDGV_WARN(
				"AUTO: Unknown current GPU state %d.  Don't know how to continue\n",
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state);
			ret = AMDGV_ERROR_IOV_CMD_ERROR;
			goto ws_exit;
		}
	}

ws_exit:
	if (ret)
		amdgv_sched_dump_gpu_state(adapt);
	oss_rwsema_write_unlock(adapt->sched.hw_state_machine[hw_sched_id].ws_lock);
	return ret;
}

int amdgv_hw_sched_state_run(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id)
{
	/*
	 * If it is already in the RUN state then advance to IDLE state
	 * to support self-switch going from RUN VF0 --> RUN VF0
	 *
	 * DOES THIS BELOW IN MANUAL SWITCH only?
	 */
	if ((adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state == AMDGV_RUN_GPU) &&
	    (IS_HW_SCHED_TYPE_GFX(hw_sched_id))) {
		AMDGV_INFO("Already RUNing, IDLE the VF first\n");
		if (amdgv_gpuiov_idle_vf(adapt,
					 adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id,
					 hw_sched_id)) {
			amdgv_sched_dump_gpu_state(adapt);
			return AMDGV_ERROR_IOV_WS_IDLE_TIMEOUT;
		}
		adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state = AMDGV_IDLE_GPU;
	}

	/*
	 * Switch states until we get to the RUN state.
	 */
	if (adapt->array_vf[idx_vf].skip_run)
		return adapt->sched.hw_state_machine[hw_sched_id].goto_state(
			adapt, idx_vf, hw_sched_id, AMDGV_LOAD_GPU_STATE);
	else
		return adapt->sched.hw_state_machine[hw_sched_id].goto_state(
			adapt, idx_vf, hw_sched_id, AMDGV_RUN_GPU);
}

int amdgv_logical_sched_state_run(struct amdgv_adapter *adapt, uint32_t idx_vf,
			   struct amdgv_sched_world_switch *world_switch)
{
	int ret;
	uint32_t hw_sched_id;
	bool mark_bad = false;

	if (!world_switch->bulk_goto_state) {
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			if (amdgv_hw_sched_state_run(adapt, idx_vf, hw_sched_id))
				mark_bad = true;
		}
		if (mark_bad)
			return AMDGV_FAILURE;
		else
			return 0;
	}

	/*
	 * If it is already in the RUN state then advance to IDLE state
	 * to support self-switch going from RUN VF0 --> RUN VF0
	 *
	 * DOES THIS BELOW IN MANUAL SWITCH only?
	 */
	if (world_switch->curr_vf_state == AMDGV_VF_CONTEXT_LOADED &&
		world_switch->curr_idx_vf == idx_vf) {
		ret = world_switch->bulk_goto_state(adapt, idx_vf,
			world_switch->hw_sched_mask, AMDGV_IDLE_GPU);
		if (ret)
			return ret;
	}

	/*
	 * Switch states until we get to the RUN state.
	 */
	if (adapt->array_vf[idx_vf].skip_run)
		ret = world_switch->bulk_goto_state(adapt, idx_vf, world_switch->hw_sched_mask, AMDGV_LOAD_GPU_STATE);
	else
		ret = world_switch->bulk_goto_state(adapt, idx_vf, world_switch->hw_sched_mask, AMDGV_RUN_GPU);

	return ret;
}

int amdgv_hw_sched_state_run_auto(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  uint32_t hw_sched_id)
{
	int i;

	if (IS_HW_SCHED_TYPE_GFX(hw_sched_id)) {
		if (idx_vf == -1) {
			AMDGV_DEBUG(
				"Target VF not specified.  Check if anything is capable of running\n");
			for (i = 0; i < AMDGV_MAX_VF_SLOT; ++i) { /* Set all to IDLE */
				if (adapt->array_vf[i].auto_run == AMDGV_WS_AUTO_RUN_ENABLED) {
					AMDGV_DEBUG("VF%d is able to RUN in auto switch mode\n", i);
					idx_vf = i;
					break;
				}
			}
		}

		if (amdgv_sched_active_vf_num(adapt) == 1 && adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode == AMDGV_SCHED_LIQUID_MODE)
			/* If only 1 VF is active, disable auto world switch in liquid mode until another active VF is added. */
			return adapt->sched.hw_state_machine[hw_sched_id].goto_state(
				adapt, idx_vf, hw_sched_id, AMDGV_RUN_GPU);

		if (idx_vf == -1) {
			AMDGV_DEBUG("No active VF, run PF.\n");
			idx_vf = AMDGV_PF_IDX;
			return adapt->sched.hw_state_machine[hw_sched_id].goto_state(
				adapt, idx_vf, hw_sched_id, AMDGV_RUN_GPU);
		}
	}
	/*
	 * Switch states until we get to the RUN state.
	 */
	return adapt->sched.hw_state_machine[hw_sched_id].goto_state(
		adapt, idx_vf, hw_sched_id, AMDGV_ENABLE_AUTO_HW_SWITCH);
}

int amdgv_hw_sched_state_pause(struct amdgv_adapter *adapt, uint32_t idx_vf,
			       uint32_t hw_sched_id)
{
	/*
	 * Switch states until we get to the RUN state.
	 */
	return adapt->sched.hw_state_machine[hw_sched_id].goto_state(
		adapt, idx_vf, hw_sched_id, AMDGV_SAVE_GPU_STATE);
}

int amdgv_logical_sched_state_pause(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     struct amdgv_sched_world_switch *world_switch)
{
	uint32_t hw_sched_id;
	bool mark_bad = false;


	if (!world_switch->bulk_goto_state) {
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			if (amdgv_hw_sched_state_pause(adapt, idx_vf, hw_sched_id))
				mark_bad = true;
		}
	} else {
		return world_switch->bulk_goto_state(adapt, idx_vf, world_switch->hw_sched_mask, AMDGV_SAVE_GPU_STATE);
	}

	if (mark_bad)
		return AMDGV_FAILURE;
	else
		return 0;
}

int amdgv_hw_sched_state_shutdown(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  uint32_t hw_sched_id)
{
	/*
	 * Switch states until we get to the RUN state.
	 */
	return adapt->sched.hw_state_machine[hw_sched_id].goto_state(
		adapt, idx_vf, hw_sched_id, AMDGV_SHUTDOWN_GPU);
}

int amdgv_logical_sched_state_shutdown(struct amdgv_adapter *adapt, uint32_t idx_vf,
				struct amdgv_sched_world_switch *world_switch)
{
	uint32_t hw_sched_id;
	bool mark_bad = false;

	if (!world_switch->bulk_goto_state) {
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			if (amdgv_hw_sched_state_shutdown(adapt, idx_vf, hw_sched_id))
				mark_bad = true;
		}
	} else {
		return world_switch->bulk_goto_state(adapt, idx_vf, world_switch->hw_sched_mask, AMDGV_SHUTDOWN_GPU);
	}

	if (mark_bad)
		return AMDGV_FAILURE;
	else
		return 0;
}

/*
 * Software init of structure
 */
int amdgv_hw_sched_init(struct amdgv_adapter *adapt)
{
	uint32_t hw_sched_id;
	uint32_t idx_vf;

	/*
	 * MMSCH is currently the only engine using auto-scheduling.
	 * RLC_V is not implemented yet.
	 *
	 * The MMSCH is not consistent with the World Switch sequence and requires
	 * a slightly modified sequence.  Hence the MMSCH uses a different state
	 * machine.  When the MMSCH and RLC_V are in sync then only one state
	 * machine should be required.
	 */
	for (hw_sched_id = 0; hw_sched_id < AMDGV_MAX_NUM_HW_SCHED; ++hw_sched_id) {
		if (adapt->sched.hw_state_machine[hw_sched_id].mode == AMDGV_WS_MODE_AUTO) {
			/* Support MMSCH in auto-scheduling mode */
			adapt->sched.hw_state_machine[hw_sched_id].goto_state =
				&world_switch_goto_state_auto;
		} else {
			/* Manual scheduling for Gfx */
			adapt->sched.hw_state_machine[hw_sched_id].goto_state =
				&world_switch_goto_state_manual;
		}
		adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state = AMDGV_SHUTDOWN_GPU;
		adapt->sched.array_vf[AMDGV_PF_IDX].cur_vf_state[hw_sched_id] =
			AMDGV_SHUTDOWN_GPU;
	}
	for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
		for (hw_sched_id = 0; hw_sched_id < AMDGV_MAX_NUM_HW_SCHED; ++hw_sched_id) {
			adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id] =
				AMDGV_SHUTDOWN_GPU;
		}
		adapt->array_vf[idx_vf].auto_run = AMDGV_WS_AUTO_RUN_DISABLED;
	}
	return 0;
}

enum amdgv_live_info_status amdgv_ws_state_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ws_state *ws_state)
{
	uint32_t hw_sched_id;

	/* We now need to save each hw_sched info individually as some asics
	 * may have multiple hw schedulers of same type
	 */
	for (hw_sched_id = 0; hw_sched_id < adapt->gpuiov.num_ctrl_blocks;
			hw_sched_id++) {
		ws_state[hw_sched_id].cur_gpu_state =
			adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state;
		ws_state[hw_sched_id].cur_vf_id =
			adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id;
		ws_state[hw_sched_id].mode =
			adapt->sched.hw_state_machine[hw_sched_id].mode;
	}
	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_ws_state_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ws_state *ws_state)
{
	uint32_t hw_sched_id;

	/* We now need to save each hw_sched info individually as some asics
	 * may have multiple hw schedulers of same type
	 */
	for (hw_sched_id = 0; hw_sched_id < adapt->gpuiov.num_ctrl_blocks;
			hw_sched_id++) {
		adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
			ws_state[hw_sched_id].cur_gpu_state;
		adapt->sched.hw_state_machine[hw_sched_id].cur_vf_id =
			ws_state[hw_sched_id].cur_vf_id;
		adapt->sched.hw_state_machine[hw_sched_id].mode =
			ws_state[hw_sched_id].mode;
	}
	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}
