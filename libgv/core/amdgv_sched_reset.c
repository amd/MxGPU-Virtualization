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
#include "amdgv_sched_internal.h"
#include "amdgv_reset.h"
#include "amdgv_guard.h"
#include "amdgv_notify.h"
#include "amdgv_task_barrier.h"
#include "amdgv_ras_eeprom.h"
#include "amdgv_psp_gfx_if.h"

static const uint32_t this_block = AMDGV_SCHEDULER_BLOCK;

/* after notify vf, wait 10 ms */
#define AMDGV_NOTIFY_VF_WAIT_TIME 10

/* call back to wait one specific guest to be reset ready */
static int amdgv_wait_guest_reset_ready_cb(void *context)
{
	bool *ready = (bool *)context;
	return !(*ready == true);
}

static int amdgv_sched_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf,
			      enum amdgv_sched_block sched_block, bool notify_vf)
{
	int ret = 0;

	amdgv_debug_test_and_hang_flr(adapt);

	if (!(adapt->flags & AMDGV_FLAG_USE_PF) && (idx_vf == AMDGV_PF_IDX))
		return AMDGV_FAILURE;

	/* Disable the flag on libgv side as RLCV will disable it after flr */
	if (adapt->flags & AMDGV_FLAG_DEBUG_DUMP_ENABLE)
		adapt->flags &= ~AMDGV_FLAG_DEBUG_DUMP_ENABLE;

	/* Skip for MI300 series as sched_stop_all affects VFs with workload */
	if (!(adapt->flags & AMDGV_FLAG_SKIP_DIAG_DATA) &&
		!(adapt->flags & AMDGV_FLAG_DISABLE_DCORE_DEBUG))
		oss_signal_reset_happened(adapt, idx_vf);

	/* Collect the diagnosis data logs */
	if (amdgv_diag_data_cache_dump(adapt, idx_vf,
					       AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_FLR))
		AMDGV_WARN("Unable to collect diagnosis data log on VF FLR\n");

	/* Notify dcore module can get diagnosis data cache */
	if (!(adapt->flags & AMDGV_FLAG_SKIP_DIAG_DATA) &&
		!(adapt->flags & AMDGV_FLAG_DISABLE_DCORE_DEBUG))
		oss_signal_diag_data_ready(adapt);

	amdgv_reset_notify_engine_status(adapt, idx_vf);

	if (notify_vf) {
		amdgv_reset_mailbox_notify_vf(adapt, idx_vf, false);
		/* it's safer to wait guest to response,
		   but no matter it responsed or not, we need to go on reset */
		adapt->array_vf[idx_vf].ready_to_reset = false;
		ret = amdgv_wait_for(adapt, amdgv_wait_guest_reset_ready_cb,
				     (void *)&adapt->array_vf[idx_vf].ready_to_reset,
				     AMDGV_TIMEOUT(TIMEOUT_GUEST_IDH_RESP), 0);
	}

	ret = amdgv_reset_vf_flr(adapt, idx_vf);

	if (ret)
		return ret;

	if (adapt->flags & AMDGV_FLAG_VF_FB_PROTECTION)
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_FB, false);

	amdgv_sched_context_clear_state(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);

	amdgv_guard_add_active_event(adapt, idx_vf, AMDGV_GUARD_EVENT_FLR);

	if (adapt->flags & AMDGV_FLAG_USE_LEGACY_FLR_SEQUENCE) {
		// Issue IDLE + SAVE so that RLCV can restore CSA size in SRAM
		if (amdgv_sched_context_save(adapt, idx_vf, sched_block))
			return AMDGV_FAILURE;
	}

	/* If ECC and FFBM enabled, try replace page after FLR is entirely done */
	if (adapt->ffbm.enabled && adapt->ecc.enabled) {
		oss_mutex_lock(adapt->ecc.unhandled_bps_lock);
		/* pop from stack */
		while (adapt->ecc.last_err_bps_cnt > 0) {
			if (amdgv_ffbm_replace_bad_pages(adapt, &adapt->ecc.last_err_bps[--adapt->ecc.last_err_bps_cnt], 1)) {
				AMDGV_ERROR("Failed to replace bad pages!\n");
				/* check if RMA criteria is hit */
				if (amdgv_ras_eeprom_is_gpu_bad(adapt))
					amdgv_device_set_status(adapt, AMDGV_STATUS_HW_RMA);
				/* FLR fail will lead to WGR */
				ret = AMDGV_FAILURE;
				break;
			}
		}
		oss_mutex_unlock(adapt->ecc.unhandled_bps_lock);
	}

	if (notify_vf) {
		amdgv_reset_mailbox_notify_vf(adapt, idx_vf, true);
	}

	return ret;
}

/* callback to wait for all active guest to reset ready */
static int amdgv_wait_all_guest_reset_ready_cb(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	int idx_vf;
	for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
		if (!is_active_or_suspend_vf(adapt, idx_vf))
			continue;

		if (adapt->array_vf[idx_vf].ready_to_reset == false)
			return AMDGV_FAILURE;
	}

	return 0;
}

static int amdgv_sched_whole_gpu_reset(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf, i;
	uint32_t hw_sched_id;
	int ret = 0;

	/* Query all ras block ecc errors in ecc gpu reset */
	if (oss_atomic_read(adapt->in_ecc_recovery)) {
		amdgv_ecc_query_ras_errors(adapt);
	}

	amdgv_debug_test_and_hang_wgr(adapt);

	if (!oss_atomic_read(adapt->in_ecc_recovery)) {
		/* Collect the diagnosis data logs */
		if (amdgv_diag_data_cache_dump(adapt, AMDGV_PF_IDX,
			    AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_RESET))
			AMDGV_WARN("Unable to collect diagnosis data log on GPU reset\n");
	}

	amdgv_reset_notify_engine_status(adapt, AMDGV_PF_IDX);

	/* handle PF first if PF is used */
	if (adapt->flags & AMDGV_FLAG_USE_PF) {
		AMDGV_DEBUG("notify reset PF\n");
		amdgv_reset_mailbox_notify_vf(adapt, AMDGV_PF_IDX, false);
	}

	/* VFs will be invalid after whole GPU reset, mark them as SHUTDOWN */
	for (hw_sched_id = 0; hw_sched_id < AMDGV_MAX_NUM_HW_SCHED; ++hw_sched_id) {
		for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++) {
			adapt->sched.array_vf[idx_vf].cur_vf_state[hw_sched_id] =
				AMDGV_SHUTDOWN_GPU;
		}
		adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state = AMDGV_SHUTDOWN_GPU;
	}

	/* handle VF */
	for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
		if (!is_active_or_suspend_vf(adapt, idx_vf))
			continue;

		if (!oss_atomic_read(adapt->in_ecc_recovery)) {
			AMDGV_DEBUG("notify reset %s\n", amdgv_idx_to_str(idx_vf));
			amdgv_reset_mailbox_notify_vf(adapt, idx_vf, false);
			adapt->array_vf[idx_vf].ready_to_reset = false;
		}
	}

	if (!oss_atomic_read(adapt->in_ecc_recovery)) {
		/* it's safer to wait guest to response,
			but no matter it responsed or not, we need to go on reset */
		amdgv_wait_for(adapt, amdgv_wait_all_guest_reset_ready_cb, (void *)adapt,
				AMDGV_TIMEOUT(TIMEOUT_GUEST_IDH_RESP_GPU_RESET), 0);
	}

	amdgv_sched_world_context_clear_state_rst(adapt);

	/* reset the whole GPU */
	ret = amdgv_reset_gpu(adapt);

	if (ret)
		return AMDGV_FAILURE;

	/* diable VF FB access */
	if (adapt->flags & AMDGV_FLAG_VF_FB_PROTECTION) {
		for (i = 0; i < adapt->num_vf; i++) {
			amdgv_gpuiov_set_vf_access(adapt, i, AMDGV_VF_ACCESS_FB, false);
		}
	}

	/* handle VF */
	for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
		if (!is_active_or_suspend_vf(adapt, idx_vf))
			continue;

		AMDGV_DEBUG("notify %s whole GPU reset completion\n",
			    amdgv_idx_to_str(idx_vf));
		/* notify vf reset completion */
		amdgv_reset_mailbox_notify_vf(adapt, idx_vf, true);

		/* remove vf out of active vf list */
		if (is_active_vf(idx_vf))
			amdgv_sched_remove_vf(adapt, idx_vf);

		set_to_avail_vf(idx_vf);
	}

	return 0;
}

/*
 * When the amdgv_sched_gpu_chain_reset function is called ,  it may have 4 different cases:
 *
 * Case 1: reset_all = 1 and in_chain_reset = 0:
 * This is a normal case one adapter starting chain reset.  For this case,
 * Current adapter will set global variable in_chain_reset = 1.
 * Then broadcast internal_WGR (reset_all=0) to all other adapters
 * Then do WGR on itself adapter.
 *
 * Case 2: reset_all = 0 and in_chain_reset = 1:
 * This is a normal case the adapter get the broadcasted internal_WGR (reset_all=0).  For this case,
 * Current adapter just do itself WGR.
 *
 * Case 3: reset_all = 1 and in_chain_reset = 1:
 * This case may happen
 * •	when one adapter is doing RAS/sysfs WGR, another adapter is doing FLR fail caused WGR
 * •	or multiple adapters are doing FLR fail caused WGR together.  The FLR fail may be from
 * event 1, evnet5 or other evnets. For this case,  it already has another adapter set in_chain_reset=1 and
 * broadcasted internal_WGR to all other adapters. Current adapter is one of the broadcasted adapters.
 * But current adapter was doing FLR, it has no chance to handle interal_WGR yet.
 * So for this case, we will just do current adapter itself WGR.  And then clean up the redundant interal_WGR.
 *
 * Case 4: reset_all = 0 and in_chain_reset = 0:
 * Ideally the case should not happen.
 * The redundant interal_WGR should already be cleaned on case3 handling.
 * But in case somehow the redundant interal_WGR isn’t cleaned, make the case 4 happen.
 * This should be an orphan reset request, just drop it.
 */
static int amdgv_sched_gpu_chain_reset(struct amdgv_adapter *adapt, bool reset_all)
{
	struct amdgv_adapter *adapt_next = NULL;
	struct amdgv_hive_info *hive;
	int ret = 0;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive) {
		AMDGV_ERROR("XGMI: node 0x%llx, can not match hive "
			    "0x%llx in the hive list.\n",
			    adapt->xgmi.node_id, adapt->xgmi.hive_id);
		return AMDGV_FAILURE;
	}

	if (reset_all) {
		/* set hive->in_chain_reset at the first chain reset request */
		oss_spin_lock(hive->chain_reset_lock);
		if (hive->in_chain_reset) {
			oss_spin_unlock(hive->chain_reset_lock);
			goto self_reset;
		}

		hive->in_chain_reset = true;
		oss_spin_unlock(hive->chain_reset_lock);

		amdgv_list_for_each_entry (adapt_next, &hive->adapt_list, struct amdgv_adapter,
					   xgmi.head) {
			if (adapt_next == adapt)
				continue;

			AMDGV_INFO("notify chain reset on node 0x%llx\n", adapt_next->xgmi.node_id);
			adapt_next->reset.in_xgmi_chain_reset = true;
			if (amdgv_sched_queue_event(adapt_next, AMDGV_PF_IDX,
						    AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL, 0))
				AMDGV_WARN("notify chain reset on node 0x%llx failed\n",
					   adapt_next->xgmi.node_id);
		}
	}

self_reset:
	if (hive->in_chain_reset) {
		adapt->reset.in_xgmi_chain_reset = true;
		ret = amdgv_sched_whole_gpu_reset(adapt);

		/* if any reset failed, mark hive bad to drop any further reset */
		if (ret)
			amdgv_xgmi_mark_hive_bad(adapt);
		task_barrier_enter(&hive->tb_chain_reset, hive->number_adapters);

		amdgv_sched_remove_stale_events_after_wgr(adapt);
		/* clear the global reset flag by master */
		if (adapt->xgmi.master_adapt == adapt) {
			oss_spin_lock(hive->chain_reset_lock);
			hive->in_chain_reset = 0;
			oss_spin_unlock(hive->chain_reset_lock);
		}

		oss_atomic_set(adapt->in_ecc_recovery, 0);

		task_barrier_exit(&hive->tb_chain_reset, hive->number_adapters);
	} else {
		AMDGV_INFO("drop orphan reset request\n");
	}

	return ret;
}

int amdgv_sched_gpu_reset_wrap(struct amdgv_adapter *adapt, bool reset_all)
{
	int ret = AMDGV_FAILURE;

	if (amdgv_xgmi_is_hive_bad(adapt) ||
		 (adapt->xgmi.phy_nodes_num == 1 && amdgv_ras_eeprom_is_gpu_bad(adapt))) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_RESET_GPU_HIVE_FAILED, 0);
		return ret;
	}

	amdgv_sched_stop_all(adapt);

	if (adapt->xgmi.phy_nodes_num > 1)
		ret = amdgv_sched_gpu_chain_reset(adapt, reset_all);
	else
		ret = amdgv_sched_whole_gpu_reset(adapt);

	return ret;
}

static void amdgv_sched_reset_vf_sched_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	/*
	 * if the VF is active VF, move VF out of active list and
	 * move to available state.
	 */
	if (is_active_vf(idx_vf)) {
		amdgv_sched_remove_vf(adapt, idx_vf);
	}

	set_to_avail_vf(idx_vf);
}

int amdgv_sched_reset_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 enum amdgv_sched_block sched_block)
{
	int ret;
	bool notify_vf;
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	if (oss_atomic_read(adapt->in_ecc_recovery)) {
		/* Fatal error interrupt will queue a reset. */
		return AMDGV_FAILURE;
	}

	if (adapt->flags & AMDGV_FLAG_VF_HANG_GPU_RESET) {
		AMDGV_INFO("force reset enabled! Trigger whole_gpu_reset\n");
		goto whole_gpu_reset;
	}

	amdgv_time_log_note_vf_reset_start(adapt, idx_vf);

	AMDGV_INFO("start %s reset\n", amdgv_idx_to_str(idx_vf));

	notify_vf = (is_active_vf(idx_vf) ? true : false) ||
		    adapt->array_vf[idx_vf].vf_status == AMDGV_VF_STATUS_START_INIT;

	ret = amdgv_sched_vf_flr(adapt, idx_vf, sched_block, notify_vf);
	if (ret) {
		AMDGV_INFO("failed %s FLR\n", amdgv_idx_to_str(idx_vf));
		goto whole_gpu_reset;
	}

	if (!(adapt->flags & AMDGV_FLAG_USE_LEGACY_FLR_SEQUENCE) && (idx_vf != AMDGV_PF_IDX)) {
		for_each_id (world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					adapt, idx_vf, AMDGV_SCHED_BLOCK_GFX)) {
			world_switch = &adapt->sched.world_switch[world_switch_id];
			if (amdgv_sched_world_context_load(adapt, AMDGV_PF_IDX,  world_switch))
				return AMDGV_FAILURE;
			if (amdgv_sched_world_context_save(adapt, world_switch))
				return AMDGV_FAILURE;
		}
	}

	amdgv_psp_clear_vf_fw(adapt, idx_vf);

	AMDGV_INFO("finish %s reset\n", amdgv_idx_to_str(idx_vf));

	amdgv_time_log_note_vf_reset_end(adapt, idx_vf);

	return 0;

whole_gpu_reset:
	AMDGV_INFO("Trying whole gpu reset ...\n");

	amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ERROR_WHOLE_GPU_RESET,
			  "Whole GPU reset triggered by failed FLR on %s.",
			  amdgv_idx_to_str(idx_vf));

	ret = amdgv_sched_gpu_reset_wrap(adapt, 1);

	amdgv_time_log_note_vf_reset_end(adapt, idx_vf);

	return ret;
}

int amdgv_sched_reset_vf_auto(struct amdgv_adapter *adapt)
{
	int ret;
	struct amdgv_sched_world_switch *abnormal_world_switch = NULL;
	uint32_t abnormal_idx_vf = AMDGV_INVALID_IDX_VF;
	uint32_t curr_vf_state;
	uint32_t hw_sched_id;
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	if (adapt->flags & AMDGV_FLAG_VF_HANG_GPU_RESET) {
		AMDGV_INFO("force reset enabled! Trigger whole_gpu_reset\n");
		goto whole_gpu_reset__auto;
	}

	if (oss_atomic_read(adapt->in_ecc_recovery)) {
		/* Fatal error interrupt will queue a reset. */
		return AMDGV_FAILURE;
	}

	//check all GPUIOV control blocks

	for (hw_sched_id = 0; hw_sched_id < adapt->gpuiov.num_ctrl_blocks;
			hw_sched_id++) {
		amdgv_sched_world_context_get_hw_curr_state(adapt, hw_sched_id,
								&curr_vf_state);
		if (curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL) {
			if (amdgv_sched_get_world_switch_by_hw_sched_id(
					adapt, hw_sched_id, &abnormal_world_switch)) {
				AMDGV_ERROR(
					"HW Scheduler doesn't belong to any logical scheduler!");
				goto whole_gpu_reset__auto;
			}
			break;
		}
	}

	if (!abnormal_world_switch) {
		AMDGV_INFO("No engine in abnormal state, skip reset here\n");
		return 0;
	}

	abnormal_idx_vf = abnormal_world_switch->curr_idx_vf;

	AMDGV_INFO("start reset auto on VF%d (%s engine hung)\n",
		   abnormal_world_switch->curr_idx_vf,
		   amdgv_sched_block_to_name(abnormal_world_switch->sched_block));

	for_each_id(world_switch_id,
		     amdgv_sched_get_world_switch_mask(adapt, abnormal_idx_vf)) {
		adapt->sched.world_switch[world_switch_id].switch_running = false;
	}

	/* record time for VF */
	if (abnormal_idx_vf != AMDGV_INVALID_IDX_VF)
		amdgv_time_log_note_vf_reset_start(adapt, abnormal_idx_vf);

	if (!(adapt->flags & AMDGV_FLAG_USE_PF) && (abnormal_idx_vf == AMDGV_PF_IDX)) {
		AMDGV_INFO("PF is currently the active FCN on hung engine\n");
		goto whole_gpu_reset__auto;
	}
	if (AMDGV_IS_IDX_INVALID(abnormal_idx_vf)) {
		AMDGV_INFO("INVALID VFID on hung engine\n");
		goto whole_gpu_reset__auto;
	}

	//Sync HW schedulers on logical scheduler
	if (amdgv_sched_world_context_sync_abnormal_sched(adapt, abnormal_idx_vf,
							  abnormal_world_switch))
		goto whole_gpu_reset__auto;

	//Switch all other Schedulers to abonormal VF
	for_each_id(world_switch_id,
		     amdgv_sched_get_world_switch_mask(adapt, abnormal_idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch == abnormal_world_switch)
			continue;

		if (amdgv_sched_world_context_switch_to_vf(adapt, abnormal_idx_vf,
							   world_switch)) {
			AMDGV_INFO("Unable to context_switch to %s on %s engine\n",
				   amdgv_idx_to_str(abnormal_idx_vf),
				   amdgv_sched_block_to_name(world_switch->sched_block));
			goto whole_gpu_reset__auto;
		}
	}

	/* always notify VM of FLR start and completion
	 * because for "VF reset auto", different VM could be active
	 * on different "sched_id". So make sure VF that will be FLR knows
	 */
	ret = amdgv_sched_vf_flr(adapt, abnormal_idx_vf, AMDGV_SCHED_BLOCK_ALL, true);

	amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ERROR_RESET_VF,
			  "Reset %s initiated from reset_vf_auto on %s",
			  amdgv_idx_to_str(abnormal_idx_vf),
			  amdgv_sched_block_to_name(AMDGV_SCHED_BLOCK_ALL));

	if (ret) {
		AMDGV_INFO("amdgv_sched_vf_flr() returned error %d\n", ret);
		goto whole_gpu_reset__auto;
	}

	/* set the vf to AVAIL state */
	amdgv_sched_reset_vf_sched_state(adapt, abnormal_idx_vf);

	if (!(adapt->flags & AMDGV_FLAG_USE_LEGACY_FLR_SEQUENCE) && (abnormal_idx_vf != AMDGV_PF_IDX)) {
		for_each_id (world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					  adapt, abnormal_idx_vf, AMDGV_SCHED_BLOCK_GFX)) {
			world_switch = &adapt->sched.world_switch[world_switch_id];
			if (amdgv_sched_world_context_load(adapt, AMDGV_PF_IDX,  world_switch))
					return AMDGV_FAILURE;
			if (amdgv_sched_world_context_save(adapt, world_switch))
					return AMDGV_FAILURE;
		}
	}

	/* save the current vf's context */
	for_each_id(world_switch_id,
		     amdgv_sched_get_world_switch_mask(adapt, abnormal_idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			if (amdgv_sched_world_context_get_hw_curr_state(adapt, hw_sched_id,
									&curr_vf_state))
				continue;
			if (curr_vf_state == AMDGV_VF_CONTEXT_LOADED) {
				if (amdgv_sched_world_context_save(adapt, world_switch)) {
					AMDGV_INFO("failed to save vf after FLR\n");
					goto whole_gpu_reset__auto;
				}
			}
		}
	}

	amdgv_psp_clear_vf_fw(adapt, abnormal_idx_vf);

	AMDGV_INFO("finish VF reset auto.\n");

	/* end recording for VF */
	amdgv_time_log_note_vf_reset_end(adapt, abnormal_idx_vf);

	return 0;

whole_gpu_reset__auto:
	AMDGV_INFO("Trying whole gpu reset ...\n");

	amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ERROR_WHOLE_GPU_RESET,
			  "Whole GPU reset triggered by failed VF reset auto.");

	ret = amdgv_sched_gpu_reset_wrap(adapt, 1);

	/* end recording for VF even for failure */
	if (abnormal_idx_vf != AMDGV_INVALID_IDX_VF)
		amdgv_time_log_note_vf_reset_end(adapt, abnormal_idx_vf);

	return ret;
}
