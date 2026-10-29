/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "gpuiov_v9_0.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

#ifdef WS_RECORD
static void gpuiov_v9_0_record_cmd(struct amdgv_adapter *adapt,
				uint32_t hw_sched_mask, uint32_t idx_vf,
				uint32_t cmd, bool completed)
{
	enum amdgv_record_status status;
	uint32_t hw_sched_id;

	if (!(adapt->flags & AMDGV_FLAG_WS_RECORD))
		return;

	switch (cmd) {
	case AMDGV_IDLE_GPU:
		status = completed ? AMDGV_RECORD_IDLE_END : AMDGV_RECORD_IDLE_START;
		break;
	case AMDGV_SAVE_GPU_STATE:
		status = completed ? AMDGV_RECORD_SAVE_END : AMDGV_RECORD_SAVE_START;
		break;
	case AMDGV_LOAD_GPU_STATE:
		status = completed ? AMDGV_RECORD_LOAD_END : AMDGV_RECORD_LOAD_START;
		break;
	case AMDGV_RUN_GPU:
		status = completed ? AMDGV_RECORD_RUN_END : AMDGV_RECORD_RUN_START;
		break;
	case AMDGV_INIT_GPU:
		status = completed ? AMDGV_RECORD_INIT_END : AMDGV_RECORD_INIT_START;
		break;
	case AMDGV_SHUTDOWN_GPU:
		status = completed ? AMDGV_RECORD_SHUTDOWN_END :
			AMDGV_RECORD_SHUTDOWN_START;
		break;
	default:
		return;
	}

	for_each_id(hw_sched_id, hw_sched_mask)
		amdgv_gpuiov_record_queue_push(adapt, idx_vf, hw_sched_id, status);
}
#endif

static int gpuiov_v9_0_batch_send(struct amdgv_adapter *adapt, uint32_t hw_sched_mask,
				  uint32_t next_state, uint32_t next_vf)
{
	uint32_t hw_sched_id = 0, xcc_mask = 0;
	int ret = 0;

	for_each_id (hw_sched_id, hw_sched_mask) {
		if (IS_HW_SCHED_TYPE_GFX(hw_sched_id))
			xcc_mask |= BIT(adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst);
	}

	if (oss_atomic_read(adapt->in_sync_flood)) {
		AMDGV_DEBUG("Skip bulk GPUIOV command during sync flood: "
				"cmd=0x%x vf=%u hw_sched_mask=0x%x\n",
				next_state, next_vf, hw_sched_mask);
		return AMDGV_FAILURE;
	}

#ifdef WS_RECORD
	gpuiov_v9_0_record_cmd(adapt, hw_sched_mask, next_vf, next_state, false);
#endif

	ret = amdgv_iovm_drv_gpuiov_set_command(adapt, xcc_mask, 0, next_vf, AMDGV_INVALID_IDX_VF,
						next_state);

	AMDGV_DEBUG("GPUIOV command: idx_vf=%d xcc_mask=%x cmd=0x%x\n",
			next_vf, xcc_mask, next_state);
#ifdef WS_RECORD
	if (!ret)
		gpuiov_v9_0_record_cmd(adapt, hw_sched_mask, next_vf, next_state, true);
#endif
	return ret;
}

static int gpuiov_v9_0_bulk_goto_state_manual(struct amdgv_adapter *adapt,
					      uint32_t target_vf,
					      uint32_t hw_sched_mask,
					      uint32_t target_state)
{
	struct world_switch_state *hw_state;
	uint64_t histogram_time_start;
	uint32_t hw_sched_id, next_state, next_vf, acquired_id;
	uint32_t primary_hw_sched = amdgv_ffs(hw_sched_mask) - 1;
	int ret = 0;
	uint32_t transitions_left = 8;

	for_each_id(hw_sched_id, hw_sched_mask) {
		if (oss_rwsema_write_trylock(adapt->sched.hw_state_machine[hw_sched_id].ws_lock)) {
			amdgv_put_log(target_vf, AMDGV_LOG_IOV_WS_REENTRANT_ERROR, 0);
			for_each_id(acquired_id, hw_sched_mask & (BIT(hw_sched_id) - 1))
				oss_rwsema_write_unlock(adapt->sched.hw_state_machine[acquired_id].ws_lock);
			return AMDGV_LOG_IOV_WS_REENTRANT_ERROR;
		}
    }

	while (hw_sched_mask) {
		hw_state = &adapt->sched.hw_state_machine[primary_hw_sched];

		if (hw_state->cur_gpu_state == target_state &&
			hw_state->cur_vf_id == target_vf)
			goto ws_exit;

		ret = world_switch_get_next_state(adapt, primary_hw_sched,
							target_vf, target_state,
							hw_state->cur_gpu_state,
							hw_state->cur_vf_id,
							&next_state,
							&next_vf);
		if (ret)
			goto ws_exit;

		histogram_time_start = oss_get_time_stamp();

		ret = gpuiov_v9_0_batch_send(adapt, hw_sched_mask, next_state, next_vf);
		if (ret)
			goto ws_exit;

		for_each_id (hw_sched_id, hw_sched_mask) {
			hw_state = &adapt->sched.hw_state_machine[hw_sched_id];
			hw_state->cur_gpu_state = next_state;
			hw_state->cur_vf_id = next_vf;
			adapt->sched.array_vf[next_vf].cur_vf_state[hw_sched_id] = next_state;
			amdgv_update_histogram(adapt, next_vf,
							hw_sched_id, next_state,
							histogram_time_start);
		}

		transitions_left--;
		if (!transitions_left) {
			AMDGV_ERROR("Potential infinite loop in bulk world switch\n");
			ret = AMDGV_LOG_IOV_WS_INFINITE_LOOP;
			goto ws_exit;
		}
	}

ws_exit:
	for_each_id(hw_sched_id, hw_sched_mask)
		oss_rwsema_write_unlock(adapt->sched.hw_state_machine[hw_sched_id].ws_lock);

	return ret;
}

void gpuiov_v9_0_set_bulk_goto_state(struct amdgv_adapter *adapt,
				     struct amdgv_sched_world_switch *world_switch)
{
	if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX)
		world_switch->bulk_goto_state = gpuiov_v9_0_bulk_goto_state_manual;
}
