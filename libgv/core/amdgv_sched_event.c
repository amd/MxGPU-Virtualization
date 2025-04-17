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
#include "amdgv_list.h"
#include "amdgv_mailbox.h"
#include "amdgv_guard.h"
#include "amdgv_vfmgr.h"
#include "amdgv_notify.h"
#include "amdgv_psp_gfx_if.h"
#include "amdgv_gpumon_internal.h"
#include "amdgv_irqmgr.h"
#include "amdgv_ras_eeprom.h"
#include "amdgv_xgmi.h"
#include "amdgv_mca.h"

static const uint32_t this_block = AMDGV_SCHEDULER_BLOCK;

/* stop processing the remaining events and release the current event */
#define AMDGV_EVENT_STOP_AND_RELEASE 1
/* stop processing the remaining events and keep the current event */
#define AMDGV_EVENT_STOP_AND_KEEP 2

/**
* amdgv_sched_full_access_status - full access status
*
* - full access timeout:
* 		SCHED_FULL_ACCESS_TIMED_OUT
* - full accesss on going and no timeout:
* 		SCHED_FULL_ACCESS_ON_GOING
* - no full access on going and no timeout:
* 		SCHED_FULL_ACCESS_NOT_ENTERED
*/
enum amdgv_sched_full_access_status {
	SCHED_FULL_ACCESS_TIMED_OUT = 0,
	SCHED_FULL_ACCESS_NOT_ENTERED,
	SCHED_FULL_ACCESS_ON_GOING,
};

static const char *amdgv_event_name(uint32_t event)
{
	switch (event) {
	case AMDGV_EVENT_REQ_GPU_INIT:
		return "REQ_GPU_INIT";
	case AMDGV_EVENT_REL_GPU_INIT:
		return "REL_GPU_INIT";
	case AMDGV_EVENT_REQ_GPU_FINI:
		return "REQ_GPU_FINI";
	case AMDGV_EVENT_REL_GPU_FINI:
		return "REL_GPU_FINI";
	case AMDGV_EVENT_REQ_GPU_RESET:
		return "REQ_GPU_RESET";
	case AMDGV_EVENT_REQ_GPU_INIT_DATA:
		return "REQ_GPU_INIT_DATA";
	case AMDGV_EVENT_SCHED_FORCE_RESET_VF:
		return "SCHED_FORCE_RESET_VF";
	case AMDGV_EVENT_SCHED_RESET_VF:
		return "SCHED_RESET_VF";
	case AMDGV_EVENT_HW_SCHED_RESET_VF:
		return "HW_SCHED_RESET_VF";
	case AMDGV_EVENT_SCHED_SUSPEND_VF:
		return "SCHED_SUSPEND_VF";
	case AMDGV_EVENT_SCHED_RESUME_VF:
		return "SCHED_RESUME_VF";
	case AMDGV_EVENT_SCHED_REMOVE_VF:
		return "SCHED_REMOVE_VF";
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU:
		return "SCHED_FORCE_RESET_GPU";
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL:
		return "SCHED_FORCE_RESET_GPU_INTERNAL";
	case AMDGV_EVENT_SCHED_STOP_VF:
		return "SCHED_STOP_VF";
	case AMDGV_EVENT_SCHED_INIT_VF_FB:
		return "SCHED_INIT_VF_FB";
	case AMDGV_EVENT_SCHED_RAS_UMC:
		return "SCHED_RAS_UMC";
	case AMDGV_EVENT_SCHED_SUSPEND:
		return "SCHED_SUSPEND";
	case AMDGV_EVENT_SCHED_RESUME:
		return "SCHED_RESUME";
	case AMDGV_EVENT_SCHED_SUSPEND_LIVE:
		return "SCHED_SUSPEND_LIVE";
	case AMDGV_EVENT_SCHED_RESUME_LIVE:
		return "SCHED_RESUME_LIVE";
	case AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC:
		return "SCHED_FW_LIVE_UPDATE_DFC";
	case AMDGV_EVENT_SCHED_GPUMON:
		return "SCHED_GPUMON";
	case AMDGV_EVENT_SCHED_SET_VF_ACCESS:
		return "SCHED_SET_VF_ACCESS";
	case AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION:
		return "SCHED_MMSCH_GENERAL_NOTIFICATION";
	case AMDGV_EVENT_SCHED_PSP_VF_GATE:
		return "SCHED_PSP_VF_GATE";
	case AMDGV_EVENT_HANDLE_CRASH:
		return "AMDGV_EVENT_HANDLE_CRASH";
	case AMDGV_EVENT_COLLECT_DIAG_DATA:
		return "COLLECT_DIAG_DATA";
	case AMDGV_EVENT_ENTER_POWER_SAVING:
		return "ENTER_POWER_SAVING";
	case AMDGV_EVENT_EXIT_POWER_SAVING:
		return "EXIT_POWER_SAVING";
	case AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION:
		return "SCHED_RAS_POISON_CONSUMPTION";
	case AMDGV_EVENT_SCHED_RAS_POISON_CREATION:
		return "SCHED_RAS_POISON_CREATION";
	case AMDGV_EVENT_SCHED_RAS_FED:
		return "SCHED_RAS_FED";
	case AMDGV_EVENT_INVALID_EVENT:
		return "INVALID_EVENT";
	case AMDGV_EVENT_CUR_VF_CTX_EMPTY:
		return "CUR_VF_CTX_EMPTY";
	case AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY:
		return "SCHED_UPDATE_TOPOLOGY";
	case AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS:
		return "SCHED_UPDATE_MCA_BANKS";
	case AMDGV_EVENT_SCHED_GET_TOPOLOGY:
		return "SCHED_GET_TOPOLOGY";
	case AMDGV_EVENT_SCHED_PSP_VF_CMD_RELAY:
		return "SCHED_PSP_CMD_RELAY";
	case AMDGV_EVENT_SCHED_RMA:
		return "SCHED_RMA";
	case AMDGV_EVENT_SCHED_VF_REQ_RAS_ERROR_COUNT:
		return "SCHED_VF_REQ_RAS_ERROR_COUNT";
	case AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP:
		return "SCHED_VF_REQ_RAS_CPER_DUMP";
	case AMDGV_EVENT_REQ_GPU_DEBUG:
		return "REQ_GPU_DEBUG";
	case AMDGV_EVENT_REL_GPU_DEBUG:
		return "REL_GPU_DEBUG";
	default:
		break;
	}

	return "UNKNOWN";
}

static int amdgv_sched_psp_set_mb_int(struct amdgv_adapter *adapt, int idx_vf, bool enable)
{
	int ret = 0;

	if (!amdgv_psp_vfgate_support(adapt) || idx_vf == AMDGV_PF_IDX)
		return 0;

	if (NEED_SWITCH_TO_PF(adapt)) {
		/* Switch to PF so we can set mailbox interrupt through psp */
		ret = amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX,
						       AMDGV_SCHED_BLOCK_GFX);

		if (ret) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_SCHED_WORLD_SWITCH_FAIL, 0);
			return ret;
		}
	}

	amdgv_psp_set_mb_int(adapt, idx_vf, enable);

	if (NEED_SWITCH_TO_PF(adapt))
		amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);

	adapt->psp_mb_int_status = enable;

	return ret;
}

static int wait_psp_mb_int_status_in_hive(void *context)
{
	struct amdgv_hive_info *hive = (struct amdgv_hive_info *)context;
	struct amdgv_adapter *adapt;
	bool all_adapt_enabled = true;
	/* wait for all adapter to enable */
	amdgv_list_for_each_entry(adapt, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
		all_adapt_enabled &= adapt->psp_mb_int_status;
	}

	/* return 0 when hit */
	return (int)!all_adapt_enabled;
}

static int amdgv_sched_psp_set_mb_int_in_hive(struct amdgv_adapter *adapt, int cur_idx_vf,
					      bool enable)
{
	struct amdgv_adapter *adapt_i = adapt;
	struct amdgv_hive_info *hive;
	union amdgv_sched_event_data data;
	uint32_t tmp_idx_vf;
	uint32_t vf_select;
	int ret;
	int wait_ret = 0;
	bool send_event = false;
	bool execute_cmd = false;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive) {
		AMDGV_ERROR("XGMI: node 0x%llx, can not match hive "
			    "0x%llx in the hive list.\n",
			    adapt->xgmi.node_id, adapt->xgmi.hive_id);
		return AMDGV_FAILURE;
	}

	/* because of the sync problem, here is the priciple to make the VF gate status sync in hive
	 * 1. we don't wait the events to return, no matter it is to enable or to disable, otherwise there may have deadlock
	 * 2. only the first adapter which enables VF gate in the hive will inform other adapters, others will only take care of themselves.
	 * 3. only the last adapter which disables VF gate in the hive will inform other adapters, others SHOULD NOT disable themselves
	 * 4. enable VF gate will wait for other adapter to really enable VF gate through flags instead of event signals
	 * 5. disable VF gate will not wait, leave for the adapter itself to decide when to execute disable command
	 */

	oss_mutex_lock(hive->mcm_hive_lock);

	if (enable) {
		execute_cmd = true;
		if (oss_atomic_inc_return(&hive->psp_mb_cmd_ref_cnt) == 1)
			send_event = true;
	} else {
		if (oss_atomic_dec_return(&hive->psp_mb_cmd_ref_cnt) == 0) {
			send_event = true;
			execute_cmd = true;
		} else {
			send_event = false;
			execute_cmd = false;
		}
	}

	amdgv_list_for_each_entry(adapt_i, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
		if (adapt_i == adapt && execute_cmd) {
			for (tmp_idx_vf = 0; tmp_idx_vf < adapt_i->num_vf; tmp_idx_vf++) {
				if (tmp_idx_vf == cur_idx_vf || is_active_vf(tmp_idx_vf)) {
					ret = amdgv_sched_psp_set_mb_int(adapt_i, tmp_idx_vf,
									 enable);
					AMDGV_ASSERT(ret == 0);
				}
			}
		} else if (send_event) {	/* send event to others */
			vf_select = 0;
			for (tmp_idx_vf = 0; tmp_idx_vf < adapt_i->num_vf; tmp_idx_vf++) {
				/* is_active_vf(tmp_idx_vf) for other adapters */
				if ((adapt_i->sched.array_vf[tmp_idx_vf].state == AMDGV_SCHED_ACTIVE)) {
					vf_select |= (1 << tmp_idx_vf);
				}
			}

			if (vf_select != 0) {
				AMDGV_DEBUG("XGMI: set psp mailbox interrupt "
					    "on node: 0x%llx VFs: %d\n",
					    adapt_i->xgmi.node_id, vf_select);

				data.psp_vf_gate_data.enable = !enable;
				data.psp_vf_gate_data.vf_select = vf_select;
				ret = amdgv_sched_queue_event_ex(
					adapt_i, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_PSP_VF_GATE,
					AMDGV_SCHED_BLOCK_GFX, data);
				AMDGV_ASSERT(ret == 0);
			}
		}
	}

	oss_mutex_unlock(hive->mcm_hive_lock);

	/* enable case, need wait */
	if (enable) {
		wait_ret = amdgv_wait_for(adapt, wait_psp_mb_int_status_in_hive, hive, AMDGV_TIMEOUT(TIMEOUT_PSP_REG), 0);
		if (wait_ret) {
			AMDGV_WARN("XGMI: wait for psp mailbox interrupt status timed out\n");
			return 0;	/* this probably not a big issue, let's continue */
		}
	}

	return 0;
}

static int amdgv_sched_psp_get_mb_status(struct amdgv_adapter *adapt, int idx_vf,
					 struct psp_mb_status *mb_status)
{
	int ret = 0;

	if (!amdgv_psp_vfgate_support(adapt) || idx_vf == AMDGV_PF_IDX)
		return 0;

	if (NEED_SWITCH_TO_PF(adapt)) {
		/* Switch to PF so we can get psp mailbox interrupt status through psp */
		ret = amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX,
						       AMDGV_SCHED_BLOCK_GFX);

		if (ret) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_SCHED_WORLD_SWITCH_FAIL, 0);
			return ret;
		}
	}

	amdgv_psp_get_mb_int_status(adapt, idx_vf, mb_status);

	if (NEED_SWITCH_TO_PF(adapt))
		amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);

	return ret;
}

static int amdgv_sched_clear_vf_fb(struct amdgv_adapter *adapt, int idx_vf, uint8_t pattern)
{
	int ret = 0;
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	if (adapt->misc.dma_engine == AMDGV_DMA_ENGINE_CP_DMA) {
		/* Switch to PF so we can use CP_DMA to access fb */
		for_each_id (world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					  adapt, idx_vf, AMDGV_SCHED_BLOCK_GFX)) {
			world_switch = &adapt->sched.world_switch[world_switch_id];
			ret = amdgv_sched_world_context_switch_to_vf(adapt, AMDGV_PF_IDX, world_switch);

			if (ret) {
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_SCHED_WORLD_SWITCH_FAIL, 0);
				return ret;
			}
		}
	}

	ret = amdgv_misc_clear_vf_fb(adapt, idx_vf, pattern);

	if (adapt->misc.dma_engine == AMDGV_DMA_ENGINE_CP_DMA) {
		for_each_id (world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					  adapt, idx_vf, AMDGV_SCHED_BLOCK_GFX)) {
			world_switch = &adapt->sched.world_switch[world_switch_id];
			amdgv_sched_world_context_save(adapt, world_switch);
		}
	}

	return ret;
}

static int amdgv_sched_event_queue_init(struct amdgv_adapter *adapt)
{
	int size;

	size = sizeof(struct amdgv_sched_event_wrapper) * AMDGV_EVENT_QUEUE_ENTRY_NUM;

	adapt->sched.event_queue = oss_alloc_memory(size);
	if (adapt->sched.event_queue == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, size);
		return AMDGV_FAILURE;
	}

	oss_memset(adapt->sched.event_queue, 0, size);

	adapt->sched.queue_rptr = 0;
	adapt->sched.queue_wptr = 0;

	adapt->sched.queue_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
	if (adapt->sched.queue_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		oss_free(adapt->sched.event_queue);
		return AMDGV_FAILURE;
	}

	return 0;
}

static void amdgv_sched_event_queue_fini(struct amdgv_adapter *adapt)
{
	unsigned char rptr, wptr;

	if (adapt->sched.queue_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->sched.queue_lock);
		adapt->sched.queue_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->sched.event_queue) {
		wptr = adapt->sched.queue_wptr;
		rptr = adapt->sched.queue_rptr;
		for (; wptr != rptr; rptr++)
			if (adapt->sched.event_queue[rptr].entry)
				oss_free(adapt->sched.event_queue[rptr].entry);

		oss_free_memory(adapt->sched.event_queue);
		adapt->sched.event_queue = NULL;
	}
}


static bool amdgv_sched_event_print_log_in_info(enum amdgv_sched_event_id event_id)
{
	bool ret;

	switch (event_id) {
	/* periodic runtime events should not be printed on "info" level*/
	case AMDGV_EVENT_SCHED_GPUMON:
	case AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS:
	case AMDGV_EVENT_CUR_VF_CTX_EMPTY:
		ret = false;
		break;
	default:
		ret = true;
		break;
	}

	return ret;
}

static int amdgv_sched_event_is_event_invalid(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   enum amdgv_sched_event_id event_id,
					   enum amdgv_sched_block sched_block)
{
	// All RAS_FED and FORCE_RESET_GPU must be queued with PF index
	switch (event_id) {
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU:
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL:
	case AMDGV_EVENT_SCHED_RAS_FED:
		if (idx_vf != AMDGV_PF_IDX)
			return true;
		break;
	default:
		break;
	}

	return false;
}

static int amdgv_sched_event_queue_push_ex(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   enum amdgv_sched_event_id event_id,
					   enum amdgv_sched_block sched_block, event_t signal,
					   union amdgv_sched_event_data data,
					   bool wake_event_thread)
{
	struct amdgv_sched_event *event;
	unsigned char rptr, wptr;

	if (amdgv_sched_event_print_log_in_info(event_id))
		AMDGV_DEBUG("queue %s request from %s for %s\n", amdgv_event_name(event_id),
			   amdgv_idx_to_str(idx_vf), amdgv_sched_block_to_name(sched_block));
	else
		AMDGV_DEBUG("queue %s request from %s for %s\n", amdgv_event_name(event_id),
			    amdgv_idx_to_str(idx_vf), amdgv_sched_block_to_name(sched_block));

	if (amdgv_sched_event_is_event_invalid(adapt, idx_vf, event_id, sched_block)) {
		AMDGV_ERROR("Event %s request from %s for %s is pushed with incorrect parameters\n",
				amdgv_event_name(event_id), amdgv_idx_to_str(idx_vf),
				amdgv_sched_block_to_name(sched_block));
	}

	oss_spin_lock_irq(adapt->sched.queue_lock);

	if (event_id != AMDGV_EVENT_EXIT_POWER_SAVING && adapt->pp.is_in_powersaving == true) {
		AMDGV_INFO("queue %s request from %s for %s DENIED \n",
			   amdgv_event_name(event_id), amdgv_idx_to_str(idx_vf),
			   amdgv_sched_block_to_name(sched_block));
		oss_spin_unlock_irq(adapt->sched.queue_lock);
		return AMDGV_FAILURE;
	}

	wptr = adapt->sched.queue_wptr;
	rptr = adapt->sched.queue_rptr;

	AMDGV_DEBUG("rptr=%u, wptr=%u\n", rptr, wptr);

	if ((unsigned char)(wptr + 1) == rptr) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_SCHED_EVENT_QUEUE_FULL,
				AMDGV_ERROR_32_32(rptr, wptr));
		oss_spin_unlock_irq(adapt->sched.queue_lock);
		return AMDGV_FAILURE;
	}

	adapt->sched.event_queue[wptr].entry =
		(struct amdgv_sched_event_entry *)oss_malloc_atomic(
			sizeof(struct amdgv_sched_event_entry));
	if (adapt->sched.event_queue[wptr].entry == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct amdgv_sched_event_entry));
		oss_spin_unlock_irq(adapt->sched.queue_lock);
		return AMDGV_FAILURE;
	}

	adapt->sched.queue_wptr++;
	event = &adapt->sched.event_queue[wptr].event;

	AMDGV_DEBUG("rptr=%u, wptr=%u\n", rptr, adapt->sched.queue_wptr);

	event->idx_vf = idx_vf;
	event->id = event_id;
	event->sched_block = sched_block;
	event->timestamp = oss_get_time_stamp();
	event->signal = signal;
	event->data = data;
	event->status = AMDGV_EVENT_STATUS_NORMAL;

	oss_spin_unlock_irq(adapt->sched.queue_lock);

	if (wake_event_thread) {
		/* wake up event queue process thread */
		oss_signal_event(adapt->sched.event);
	}

	return 0;
}

static struct amdgv_list_head *
amdgv_sched_event_queue_pop_event_list(struct amdgv_adapter *adapt,
				       struct amdgv_list_head *head)
{
	uint8_t rptr, wptr;
	struct amdgv_sched_event_entry *entry = NULL;

	AMDGV_INIT_LIST_HEAD(head);

	oss_spin_lock_irq(adapt->sched.queue_lock);

	rptr = adapt->sched.queue_rptr;
	wptr = adapt->sched.queue_wptr;

	AMDGV_DEBUG("rptr=%u, wptr=%u\n", rptr, wptr);

	for (; wptr != rptr; rptr++) {
		AMDGV_DEBUG("rptr=%u, wptr=%u\n", rptr, wptr);

		entry = adapt->sched.event_queue[rptr].entry;
		adapt->sched.event_queue[rptr].entry = NULL;

		/* copy event to event list */
		oss_memcpy(&entry->event, &adapt->sched.event_queue[rptr].event,
			   sizeof(struct amdgv_sched_event));
		AMDGV_INIT_LIST_HEAD(&entry->list);

		AMDGV_DEBUG("%s event = 0x%x\n", amdgv_idx_to_str(entry->event.idx_vf),
			    entry->event.id);

		amdgv_list_add_tail(&entry->list, head);
		adapt->sched.queue_rptr++;
	}

	oss_spin_unlock_irq(adapt->sched.queue_lock);
	return head;
}

static void amdgv_sched_free_event(struct amdgv_adapter *adapt,
				   struct amdgv_sched_event *event)
{
	struct amdgv_sched_event_entry *p;

	p = amdgv_list_entry(event, struct amdgv_sched_event_entry, event);

	oss_free(p);
}

static void amdgv_sched_dump_event_list(struct amdgv_adapter *adapt)
{
}

static bool amdgv_sched_is_find_event(struct amdgv_adapter *adapt, uint32_t idx_vf, enum amdgv_sched_event_id event_id)
{
	uint8_t rptr, wptr;

	struct amdgv_sched_event *event;
	struct amdgv_list_head *event_head;
	struct amdgv_sched_event_entry *entry;

	uint32_t event_list_idx;

	// Check event queue
	rptr = adapt->sched.queue_rptr;
	wptr = adapt->sched.queue_wptr;
	for (; wptr != rptr; rptr++) {
		event = &adapt->sched.event_queue[rptr].event;
		if ((event->id == event_id) && (event->idx_vf == idx_vf))
			return true;
	}

	// Check event list
	for (event_list_idx = AMDGV_SCHED_EVENT_LIST_0; event_list_idx < AMDGV_SCHED_EVENT_LIST_MAX; event_list_idx++) {
		event_head = &adapt->sched.event_list[event_list_idx];

		amdgv_list_for_each_entry (entry, event_head, struct amdgv_sched_event_entry, list) {
			if ((entry->event.id == event_id) && (entry->event.idx_vf == idx_vf))
				return true;
		}
	}

	return false;
}

static void amdgv_sched_mark_event_in_ring(struct amdgv_adapter *adapt,
					     struct amdgv_sched_event *event,
					     enum amdgv_event_status status,
						 bool match_idx_vf)
{
	uint8_t rptr, wptr;
	struct amdgv_sched_event *e = NULL;

	oss_spin_lock_irq(adapt->sched.queue_lock);

	rptr = adapt->sched.queue_rptr;
	wptr = adapt->sched.queue_wptr;

	AMDGV_DEBUG("rptr=%u, wptr=%u\n", rptr, wptr);

	for (; wptr != rptr; rptr++) {
		AMDGV_DEBUG("rptr=%u, wptr=%u\n", rptr, wptr);
		e = &adapt->sched.event_queue[rptr].event;
		if (e->id != event->id)
			continue;
		if (match_idx_vf && (e->idx_vf != event->idx_vf))
			continue;
		e->status = status;
	}

	oss_spin_unlock_irq(adapt->sched.queue_lock);
}

static void amdgv_sched_finish_event(struct amdgv_adapter *adapt,
				     uint32_t event_list_idx,
				     struct amdgv_sched_event *event)
{
	struct amdgv_list_head *event_head;
	struct amdgv_sched_event_entry *e, *t;

	event_head = &adapt->sched.event_list[event_list_idx];
	amdgv_list_for_each_entry_safe (e, t, event_head,
					struct amdgv_sched_event_entry, list) {
		if (e->event.id == event->id) {
			e->event.status = AMDGV_EVENT_STATUS_FINISHED;
		}
	}
}

void amdgv_sched_remove_stale_events_after_wgr(struct amdgv_adapter *adapt)
{
	struct amdgv_sched_event e = { 0 };
	e.idx_vf = AMDGV_PF_IDX;

	e.id = AMDGV_EVENT_SCHED_FORCE_RESET_GPU;
	amdgv_sched_finish_event(adapt, AMDGV_SCHED_EVENT_LIST_0, &e);
	amdgv_sched_mark_event_in_ring(adapt, &e, AMDGV_EVENT_STATUS_FINISHED, false);

	e.id = AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL;
	amdgv_sched_finish_event(adapt, AMDGV_SCHED_EVENT_LIST_0, &e);
	amdgv_sched_mark_event_in_ring(adapt, &e, AMDGV_EVENT_STATUS_FINISHED, false);

	e.id = AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION;
	amdgv_sched_finish_event(adapt, AMDGV_SCHED_EVENT_LIST_0, &e);
	amdgv_sched_mark_event_in_ring(adapt, &e, AMDGV_EVENT_STATUS_FINISHED, false);

	e.id = AMDGV_EVENT_SCHED_RAS_POISON_CREATION;
	amdgv_sched_finish_event(adapt, AMDGV_SCHED_EVENT_LIST_0, &e);
	amdgv_sched_mark_event_in_ring(adapt, &e, AMDGV_EVENT_STATUS_FINISHED, false);

	e.id = AMDGV_EVENT_SCHED_VF_REQ_RAS_ERROR_COUNT;
	amdgv_sched_finish_event(adapt, AMDGV_SCHED_EVENT_LIST_4, &e);
	amdgv_sched_mark_event_in_ring(adapt, &e, AMDGV_EVENT_STATUS_FINISHED, false);

	e.id = AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP;
	amdgv_sched_finish_event(adapt, AMDGV_SCHED_EVENT_LIST_4, &e);
	amdgv_sched_mark_event_in_ring(adapt, &e, AMDGV_EVENT_STATUS_FINISHED, false);
}

static void amdgv_sched_remove_duplicated_event(struct amdgv_adapter *adapt,
						uint32_t event_list_idx,
						struct amdgv_sched_event_entry *new_entry)
{
	struct amdgv_list_head *event_head;
	struct amdgv_sched_event_entry *e, *t;

	event_head = &adapt->sched.event_list[event_list_idx];

	/* remove the older duplicated event */
	amdgv_list_for_each_entry_safe (e, t, event_head, struct amdgv_sched_event_entry,
					list) {
		if (e->event.idx_vf == new_entry->event.idx_vf) {
			amdgv_list_del(&e->list);
			oss_free(e);
		}
	}
}

static void amdgv_sched_event_arrange_event_list(struct amdgv_adapter *adapt,
						 struct amdgv_list_head *head)
{
	struct amdgv_sched_event_entry *entry, *tmp;

	amdgv_list_for_each_entry_safe (entry, tmp, head, struct amdgv_sched_event_entry,
					list) {
		AMDGV_DEBUG("%s event = 0x%x\n", amdgv_idx_to_str(entry->event.idx_vf),
			    entry->event.id);

		switch (entry->event.id) {
		case AMDGV_EVENT_SCHED_FORCE_RESET_GPU:
		case AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL:
		case AMDGV_EVENT_SCHED_RAS_UMC:
		case AMDGV_EVENT_EXIT_POWER_SAVING:
		case AMDGV_EVENT_ENTER_POWER_SAVING:
		case AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION:
		case AMDGV_EVENT_SCHED_RAS_POISON_CREATION:
		case AMDGV_EVENT_SCHED_RAS_FED:
			amdgv_sched_remove_duplicated_event(adapt, AMDGV_SCHED_EVENT_LIST_0,
							    entry);

			amdgv_list_move_tail(
				&entry->list,
				&adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_0]);
			break;
		case AMDGV_EVENT_SCHED_SUSPEND_VF:
		case AMDGV_EVENT_SCHED_RESUME_VF:
		case AMDGV_EVENT_SCHED_REMOVE_VF:
		case AMDGV_EVENT_SCHED_STOP_VF:
		case AMDGV_EVENT_SCHED_SUSPEND:
		case AMDGV_EVENT_SCHED_RESUME:
		case AMDGV_EVENT_SCHED_SUSPEND_LIVE:
		case AMDGV_EVENT_SCHED_RESUME_LIVE:
		case AMDGV_EVENT_CUR_VF_CTX_EMPTY:
		case AMDGV_EVENT_COLLECT_DIAG_DATA:
			amdgv_list_move_tail(
				&entry->list,
				&adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_1]);
			break;
		case AMDGV_EVENT_REL_GPU_INIT:
		case AMDGV_EVENT_REL_GPU_FINI:
		case AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY:
		case AMDGV_EVENT_REL_GPU_DEBUG:
			amdgv_sched_remove_duplicated_event(adapt, AMDGV_SCHED_EVENT_LIST_2,
							    entry);

			amdgv_list_move_tail(
				&entry->list,
				&adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_2]);
			break;
		case AMDGV_EVENT_SCHED_RMA:
			amdgv_sched_remove_duplicated_event(adapt, AMDGV_SCHED_EVENT_LIST_3,
					entry);
		/* fall through - Compiler does not allow this comment inside ifndef */
		case AMDGV_EVENT_SCHED_RESET_VF:
		case AMDGV_EVENT_SCHED_FORCE_RESET_VF:
		case AMDGV_EVENT_HW_SCHED_RESET_VF:
		case AMDGV_EVENT_SCHED_INIT_VF_FB:
		case AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC:
		case AMDGV_EVENT_SCHED_SET_VF_ACCESS:
		case AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION:
		case AMDGV_EVENT_SCHED_PSP_VF_GATE:
		case AMDGV_EVENT_SCHED_PSP_VF_CMD_RELAY:
		case AMDGV_EVENT_HANDLE_CRASH:
			amdgv_list_move_tail(
				&entry->list,
				&adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_3]);
			break;
		case AMDGV_EVENT_REQ_GPU_INIT:
		case AMDGV_EVENT_REQ_GPU_FINI:
		case AMDGV_EVENT_REQ_GPU_RESET:
		case AMDGV_EVENT_REQ_GPU_INIT_DATA:
		case AMDGV_EVENT_SCHED_VF_REQ_RAS_ERROR_COUNT:
		case AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP:
		case AMDGV_EVENT_REQ_GPU_DEBUG:
			amdgv_sched_remove_duplicated_event(adapt, AMDGV_SCHED_EVENT_LIST_4,
							    entry);

			amdgv_list_move_tail(
				&entry->list,
				&adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_4]);
			break;
		case AMDGV_EVENT_SCHED_GPUMON:
		case AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS:
		case AMDGV_EVENT_SCHED_GET_TOPOLOGY:
			amdgv_list_move_tail(
				&entry->list,
				&adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_5]);
			break;
		default:
			AMDGV_WARN("unknown event id = %d\n", entry->event.id);
			amdgv_list_del(&entry->list);
			oss_free(entry);
			break;
		}
	}

	amdgv_list_del(head);

	adapt->sched.curr_event_list_idx = 0;
	adapt->sched.next_event = NULL;

	amdgv_sched_dump_event_list(adapt);
}

static bool amdgv_sched_event_queue_empty(struct amdgv_adapter *adapt)
{
	bool empty;

	oss_spin_lock_irq(adapt->sched.queue_lock);
	empty = (adapt->sched.queue_rptr == adapt->sched.queue_wptr);
	oss_spin_unlock_irq(adapt->sched.queue_lock);

	return empty;
}

static struct amdgv_sched_event *amdgv_sched_pick_up_next_event(struct amdgv_adapter *adapt)
{
	int i;
	struct amdgv_list_head *event_list = NULL;
	struct amdgv_sched_event_entry *next_event;

	struct amdgv_list_head queue_events;

	/* Need to pick up new QUEUED events and arrange them into the list */
	if (!amdgv_sched_event_queue_empty(adapt)) {
		/* pop out all events from event queue */
		amdgv_sched_event_queue_pop_event_list(adapt, &queue_events);

		/* Dispatch popped out events to various event lists.
		 * According to the arrangement rules,
		 * remove duplicated VF or event.
		 */
		amdgv_sched_event_arrange_event_list(adapt, &queue_events);
	}

	/* if next_event is null, search the next event from the beginning */
	if (adapt->sched.next_event == NULL) {
		/* try to find out the first non-empty event list */
		for (i = 0; i < AMDGV_SCHED_EVENT_LIST_MAX; i++) {
			if (!amdgv_list_empty(&adapt->sched.event_list[i])) {
				event_list = &adapt->sched.event_list[i];
				adapt->sched.curr_event_list_idx = i;
				break;
			}
		}

		if (event_list == NULL) {
			amdgv_sched_dump_event_list(adapt);
			return NULL;
		}

		next_event = amdgv_list_first_entry(event_list, struct amdgv_sched_event_entry,
						    list);
	} else {
		next_event = adapt->sched.next_event;
	}

	i = adapt->sched.curr_event_list_idx;
	event_list = &adapt->sched.event_list[i];

	/* next_event is the last event of event list,
	   move to next event list to pick up next event */
	if (next_event->list.next == event_list) {
		/* try to find the next non-empty event list */
		for (i++; i < AMDGV_SCHED_EVENT_LIST_MAX; i++) {
			if (!amdgv_list_empty(&adapt->sched.event_list[i])) {
				event_list = &adapt->sched.event_list[i];
				adapt->sched.curr_event_list_idx = i;
				break;
			}
		}

		if (i == AMDGV_SCHED_EVENT_LIST_MAX) {
			adapt->sched.curr_event_list_idx = 0;
			adapt->sched.next_event = NULL;
		} else {
			adapt->sched.next_event = amdgv_list_first_entry(
				event_list, struct amdgv_sched_event_entry, list);
		}
	} else {
		adapt->sched.next_event = amdgv_list_entry(
			next_event->list.next, struct amdgv_sched_event_entry, list);
	}

	AMDGV_DEBUG("next event entry = %p, event = %p\n", next_event, &next_event->event);

	/* remove next_event from event_list */
	amdgv_list_del(&next_event->list);

	amdgv_sched_dump_event_list(adapt);

	return &next_event->event;
}

static void amdgv_sched_push_back_event(struct amdgv_adapter *adapt,
					struct amdgv_sched_event *event)
{
	struct amdgv_sched_event_entry *entry;

	entry = amdgv_list_entry(event, struct amdgv_sched_event_entry, event);

	if (amdgv_sched_event_print_log_in_info(event->id)) {
		AMDGV_INFO("%s event = %s (0x%x)\n", amdgv_idx_to_str(event->idx_vf),
			amdgv_event_name(event->id), event->id);
	} else {
		AMDGV_DEBUG("%s event = %s (0x%x)\n", amdgv_idx_to_str(event->idx_vf),
			amdgv_event_name(event->id), event->id);
	}

	switch (entry->event.id) {
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU:
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL:
	case AMDGV_EVENT_SCHED_RAS_UMC:
	case AMDGV_EVENT_EXIT_POWER_SAVING:
	case AMDGV_EVENT_ENTER_POWER_SAVING:
	case AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION:
	case AMDGV_EVENT_SCHED_RAS_POISON_CREATION:
	case AMDGV_EVENT_SCHED_RAS_FED:
		adapt->sched.curr_event_list_idx = AMDGV_SCHED_EVENT_LIST_0;
		amdgv_list_add(&entry->list,
			       &adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_0]);
		break;
	case AMDGV_EVENT_SCHED_SUSPEND_VF:
	case AMDGV_EVENT_SCHED_RESUME_VF:
	case AMDGV_EVENT_SCHED_REMOVE_VF:
	case AMDGV_EVENT_SCHED_STOP_VF:
	case AMDGV_EVENT_SCHED_SUSPEND:
	case AMDGV_EVENT_SCHED_RESUME:
	case AMDGV_EVENT_SCHED_SUSPEND_LIVE:
	case AMDGV_EVENT_SCHED_RESUME_LIVE:
	case AMDGV_EVENT_CUR_VF_CTX_EMPTY:
	case AMDGV_EVENT_COLLECT_DIAG_DATA:
		adapt->sched.curr_event_list_idx = AMDGV_SCHED_EVENT_LIST_1;
		amdgv_list_add(&entry->list,
			       &adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_1]);
		break;
	case AMDGV_EVENT_REL_GPU_INIT:
	case AMDGV_EVENT_REL_GPU_FINI:
	case AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY:
	case AMDGV_EVENT_REL_GPU_DEBUG:
		adapt->sched.curr_event_list_idx = AMDGV_SCHED_EVENT_LIST_2;
		amdgv_list_add(&entry->list,
			       &adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_2]);
		break;
	case AMDGV_EVENT_SCHED_RESET_VF:
	case AMDGV_EVENT_SCHED_FORCE_RESET_VF:
	case AMDGV_EVENT_HW_SCHED_RESET_VF:
	case AMDGV_EVENT_SCHED_INIT_VF_FB:
	case AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC:
	case AMDGV_EVENT_SCHED_SET_VF_ACCESS:
	case AMDGV_EVENT_SCHED_PSP_VF_GATE:
	case AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION:
	case AMDGV_EVENT_SCHED_PSP_VF_CMD_RELAY:
	case AMDGV_EVENT_HANDLE_CRASH:
	case AMDGV_EVENT_SCHED_RMA:
		adapt->sched.curr_event_list_idx = AMDGV_SCHED_EVENT_LIST_3;
		amdgv_list_add(&entry->list,
			       &adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_3]);
		break;
	case AMDGV_EVENT_REQ_GPU_INIT:
	case AMDGV_EVENT_REQ_GPU_FINI:
	case AMDGV_EVENT_REQ_GPU_RESET:
	case AMDGV_EVENT_REQ_GPU_INIT_DATA:
	case AMDGV_EVENT_REQ_GPU_DEBUG:
		adapt->sched.curr_event_list_idx = AMDGV_SCHED_EVENT_LIST_4;
		amdgv_list_add(&entry->list,
			       &adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_4]);
		break;
	case AMDGV_EVENT_SCHED_GPUMON:
	case AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS:
	case AMDGV_EVENT_SCHED_GET_TOPOLOGY:
		adapt->sched.curr_event_list_idx = AMDGV_SCHED_EVENT_LIST_5;
		amdgv_list_add(&entry->list,
			       &adapt->sched.event_list[AMDGV_SCHED_EVENT_LIST_5]);
		break;
	default:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_INVALID_VALUE,
				entry->event.id);
		oss_free(entry);
		return;
	}

	adapt->sched.next_event = entry;

	amdgv_sched_dump_event_list(adapt);
}

#ifdef WS_RECORD
static int amdgv_sched_record_queue_init(struct amdgv_adapter *adapt)
{
	int size;

	size = sizeof(struct amdgv_record_entity) * AMDGV_RECORD_QUEUE_ENTRY_NUM;

	adapt->sched.record_queue = oss_alloc_memory(size);
	if (adapt->sched.record_queue == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, size);
		return AMDGV_FAILURE;
	}

	oss_memset(adapt->sched.record_queue, 0, size);

	adapt->sched.record_queue_rptr = 0;
	adapt->sched.record_queue_wptr = 0;

	adapt->sched.record_queue_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
	if (adapt->sched.record_queue_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		oss_free(adapt->sched.record_queue);
		return AMDGV_FAILURE;
	}

	return 0;
}

static void amdgv_sched_record_queue_fini(struct amdgv_adapter *adapt)
{
	if (adapt->sched.record_queue_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->sched.record_queue_lock);
		adapt->sched.record_queue_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->sched.record_queue) {
		oss_free_memory(adapt->sched.record_queue);
		adapt->sched.record_queue = NULL;
	}
}

static struct amdgv_record_name amdgv_record_names[] = {
	{ AMDGV_RECORD_INIT_START, "INIT_S" },
	{ AMDGV_RECORD_INIT_END, "INIT_F" },
	{ AMDGV_RECORD_LOAD_START, "LOAD_S" },
	{ AMDGV_RECORD_LOAD_END, "LOAD_F" },
	{ AMDGV_RECORD_IDLE_START, "IDLE_S" },
	{ AMDGV_RECORD_IDLE_END, "IDLE_F" },
	{ AMDGV_RECORD_SAVE_START, "SAVE_S" },
	{ AMDGV_RECORD_SAVE_END, "SAVE_F" },
	{ AMDGV_RECORD_RUN_START, "RUN_S" },
	{ AMDGV_RECORD_RUN_END, "RUN_F" },
	{ AMDGV_RECORD_SHUTDOWN_START, "SHUTDOWN_S" },
	{ AMDGV_RECORD_SHUTDOWN_END, "SHUTDOWN_F" },
	{ AMDGV_RECORD_LOAD_RLCV_STATE_START, "LOAD_RLCV_STATE_S" },
	{ AMDGV_RECORD_LOAD_RLCV_STATE_END, "LOAD_RLCV_STATE_F" },
	{ AMDGV_RECORD_SAVE_RLCV_STATE_START, "SAVE_RLCV_STATE_S" },
	{ AMDGV_RECORD_SAVE_RLCV_STATE_END, "SAVE_RLCV_STATE_F" },
	{ AMDGV_RECORD_ENABLE_MMSCH_VFGATE_START, "ENABLE_MMSCH_VFGATE_S" },
	{ AMDGV_RECORD_ENABLE_MMSCH_VFGATE_END, "ENABLE_MMSCH_VFGATE_F" },
	{ AMDGV_RECORD_DISABLE_MMSCH_VFGATE_START, "DISABLE_MMSCH_VFGATE_S" },
	{ AMDGV_RECORD_DISABLE_MMSCH_VFGATE_END, "DISABLE_MMSCH_VFGATE_F" },
	{ AMDGV_RECORD_ENABLE_AUTO_SCHED_START, "ENABLE_AUTO_SCHED_S" },
	{ AMDGV_RECORD_ENABLE_AUTO_SCHED_END, "ENABLE_AUTO_SCHED_F" },
	{ AMDGV_RECORD_DISABLE_AUTO_SCHED_START, "DISABLE_AUTO_SCHED_S" },
	{ AMDGV_RECORD_DISABLE_AUTO_SCHED_END, "DISABLE_AUTO_SCHED_F" },
	{ AMDGV_RECORD_EVENT_NOTIFICATION_START, "NOTIFICATION_S" },
	{ AMDGV_RECORD_EVENT_NOTIFICATION_END, "NOTIFICATION_E" },
};

static const char *amdgv_sched_record_to_name(enum amdgv_record_status status)
{
	int i, count;

	count = ARRAY_SIZE(amdgv_record_names);
	for (i = 0; i < count; ++i) {
		if (amdgv_record_names[i].id == status)
			return amdgv_record_names[i].name;
	}
	return "UNKNOWN";
}

static int amdgv_sched_record_queue_flush(struct amdgv_adapter *adapt)
{
	struct amdgv_record_entity *record;
	uint16_t rptr, wptr;
	int cursor = 0;
	static unsigned long long index;

	oss_memset(adapt->record_buf, 0, MAX_RECORD_LENGTH);
	oss_spin_lock(adapt->sched.record_queue_lock);

	wptr = adapt->sched.record_queue_wptr;
	rptr = adapt->sched.record_queue_rptr;

	if (wptr == rptr) {
		oss_spin_unlock(adapt->sched.record_queue_lock);
		return 0;
	}

	for (; wptr != rptr; rptr++) {
		record = &adapt->sched.record_queue[rptr];
		cursor += oss_vsnprintf(adapt->record_buf + cursor, MAX_RECORD_LENGTH - cursor,
					"%10lld %20lld %10s %10s %30s\n", index,
					record->time_stamp,
					amdgv_hw_sched_id_to_name(adapt, record->hw_sched_id),
					amdgv_idx_to_str(record->idx_vf),
					amdgv_sched_record_to_name(record->status));
		index++;
		adapt->sched.record_queue_rptr++;
		if (cursor >= MAX_RECORD_LENGTH)
			AMDGV_WARN("dump contents is truncated due to small buffer\n");
	}
	oss_spin_unlock(adapt->sched.record_queue_lock);
	if (oss_store_record(adapt->record_buf, adapt->bdf, false))
		AMDGV_WARN("store record failed\n");
	return 0;
}
#endif

/* Set mmsched interrupts */
static int amdgv_sched_set_mmsch_vfgate(struct amdgv_adapter *adapt, int idx_vf, bool enable)
{
	int ret = 0;
	uint32_t hw_sched_id = 0;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		if (IS_HW_SCHED_TYPE_MM(hw_sched_id) &&
		    amdgv_gpuiov_set_mmsch_vfgate(adapt, idx_vf, hw_sched_id, enable)) {
			AMDGV_WARN("Failed to %s mmsch int for VF%d\n",
				   enable ? "enable" : "disable", idx_vf);
			ret |= AMDGV_FAILURE;
		}
	}

	return ret;
}

static void amdgv_sched_notify_vf_full_access(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t msg_data[MAILBOX_DATA_LEN_3];

	msg_data[0] = AMDGV_EVENT_READY_TO_ACCESS_GPU;
	/* checksum key */
	msg_data[2] = 0;
	amdgv_mailbox_send_msg(adapt, idx_vf, msg_data, MAILBOX_DATA_LEN_3, true);
}

static int amdgv_sched_handle_req_gpu_reset(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	amdgv_time_log_update_vf_cumulative_running_time_after_flr(adapt, idx_vf);
	amdgv_time_log_note_vf_init_start(adapt, idx_vf);

	adapt->array_vf[idx_vf].vf_status = AMDGV_VF_STATUS_UNDER_INIT_AFTER_RESET;

	amdgv_sched_context_init(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);

	if (AMDGV_PF_IDX != idx_vf) {
		if (amdgv_vbios_upload_image_to_vf(adapt, idx_vf))
			AMDGV_WARN("upload vbios image to vf failed\n");

		if (amdgv_vfmgr_update_pf2vf_message(adapt, idx_vf))
			AMDGV_WARN("update pf2vf message failed\n");
	}

	return 0;
}

static void amdgv_sched_notify_vf_init_data_ready(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t msg_data[MAILBOX_DATA_LEN_2];

	msg_data[0] = MB_RES_MSG_GPU_INIT_DATA_READY;
	msg_data[1] = GPU_INIT_DATA_READY_V1;

	amdgv_mailbox_send_msg(adapt, idx_vf, msg_data, MAILBOX_DATA_LEN_2, true);
}

static int amdgv_sched_handle_req_gpu_init_data(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (amdgv_vfmgr_copy_ip_data_to_vf(adapt, idx_vf, false))
		AMDGV_WARN("upload IP discovery data to vf failed\n");

	if (adapt->fw_load_type == AMDGV_FW_LOAD_BY_GIM_PHASE_2) {
		if (amdgv_host_upload_fw_to_vf(adapt, idx_vf))
			AMDGV_WARN("upload FW image to vf failed\n");
	}

	if (amdgv_vbios_upload_image_to_vf(adapt, idx_vf))
		AMDGV_WARN("upload vbios image to vf failed\n");

	if (amdgv_vfmgr_update_pf2vf_message(adapt, idx_vf))
		AMDGV_WARN("update pf2vf message failed\n");

	adapt->array_vf[idx_vf].gpu_init_data_ready = true;

	amdgv_sched_notify_vf_init_data_ready(adapt, idx_vf);

	amdgv_mca_cache_notify_event(adapt,
				     MCA_CACHE_EVENT_GUEST_LOAD,
				     idx_vf, 0);
	amdgv_vfmgr_cper_notify_event(adapt,
				      VFMGR_CPER_EVENT_GUEST_LOAD,
				      idx_vf);

	if (adapt->misc.clean_scratch_registers) {
		adapt->misc.clean_scratch_registers(adapt, idx_vf);
	}

	return 0;
}

static void amdgv_sched_notify_vf_ras_poison_ready(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t msg_data[MAILBOX_DATA_LEN_2] = { 0 };

	msg_data[0] = MB_RES_MSG_RAS_POISON_READY;
	amdgv_mailbox_send_msg(adapt, idx_vf, msg_data, MAILBOX_DATA_LEN_2, true);
	amdgv_mailbox_wait_trn_msg_ack(adapt);
}

static void amdgv_sched_notify_vf_req_ras_error_count_ready(struct amdgv_adapter *adapt,
							    uint32_t idx_vf)
{
	uint32_t msg_data[MAILBOX_DATA_LEN_3] = { 0 };

	msg_data[0] = MB_RES_MSG_RAS_ERROR_COUNT_READY;
	/* checksum key */
	msg_data[2] = 0;
	amdgv_mailbox_send_msg(adapt, idx_vf, msg_data, MAILBOX_DATA_LEN_3, true);
	amdgv_mailbox_wait_trn_msg_ack(adapt);
}

static void amdgv_sched_notify_vf_req_cper_dump_ready(struct amdgv_adapter *adapt,
						      uint32_t idx_vf)
{
	uint32_t msg_data[MAILBOX_DATA_LEN_3] = { 0 };

	msg_data[0] = MB_REQ_RAS_CPER_DUMP_READY;
	/* checksum key */
	msg_data[2] = 0;
	amdgv_mailbox_send_msg(adapt, idx_vf, msg_data, MAILBOX_DATA_LEN_3, true);
	amdgv_mailbox_wait_trn_msg_ack(adapt);
}

static void amdgv_sched_notify_vf_fail(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t msg_data[MAILBOX_DATA_LEN_3] = { 0 };

	msg_data[0] = MB_RES_MSG_FAIL;
	/* checksum key */
	msg_data[2] = 0;
	amdgv_mailbox_send_msg(adapt, idx_vf, msg_data, MAILBOX_DATA_LEN_3, true);
	amdgv_mailbox_wait_trn_msg_ack(adapt);
}

static int amdgv_sched_handle_ras_poison_consumption(struct amdgv_adapter *adapt, struct amdgv_sched_event *event)
{
	int ret = AMDGV_FAILURE;

	if (adapt->ecc.poison_consumption)
		ret = adapt->ecc.poison_consumption(adapt, event);

	if (amdgv_vfmgr_update_pf2vf_message(adapt, event->idx_vf))
		AMDGV_WARN("update pf2vf message failed\n");

	amdgv_sched_notify_vf_ras_poison_ready(adapt, event->idx_vf);

	return ret;
}

static int amdgv_sched_handle_ras_poison_creation(struct amdgv_adapter *adapt, struct amdgv_sched_event *event)
{
	int ret = AMDGV_FAILURE;

	if (adapt->ecc.poison_creation)
		ret = adapt->ecc.poison_creation(adapt, event);

	return ret;
}

static int amdgv_sched_sanitize_vf_ras_req(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   enum amdgv_sched_event_id event_id,
					   uint32_t guard_id)
{
	/* VF that has never requested init data cannot process this data.
	 * Assume the VF is malicious or abnormal and ignore it. */
	 if (!is_active_vf(idx_vf) && !is_full_access_vf(idx_vf)) {
		AMDGV_DEBUG("Invalid for non-active %s.\n", amdgv_idx_to_str(idx_vf));
		return AMDGV_FAILURE;
	}

	if (amdgv_guard_is_event_full(adapt, idx_vf, guard_id) == AMDGV_GUARD_EVENT_OVERFLOW)
		return AMDGV_FAILURE;

	/* Guest is misbehaving, ignore it now. */
	if (amdgv_guard_add_active_event(adapt, idx_vf, guard_id) == AMDGV_EVENT_OVERFLOW) {
		amdgv_put_error(idx_vf, AMDGV_ERROR_SCHED_IGNORE_EVENT, event_id);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int amdgv_sched_handle_vf_req_ras_error_count(struct amdgv_adapter *adapt,
						     uint32_t idx_vf)
{
	int ret = AMDGV_FAILURE;

	ret = amdgv_sched_sanitize_vf_ras_req(adapt, idx_vf,
					      AMDGV_EVENT_SCHED_VF_REQ_RAS_ERROR_COUNT,
					      AMDGV_GUARD_EVENT_RAS_ERR_COUNT);
	if (ret)
		return ret;

	amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_CE);

	ret = amdgv_vfmgr_dump_ras_error_counts(adapt, idx_vf);
	if (!ret)
		amdgv_sched_notify_vf_req_ras_error_count_ready(adapt, idx_vf);
	else
		amdgv_sched_notify_vf_fail(adapt, idx_vf);

	return 0;
}

static int amdgv_sched_handle_vf_req_cper_dump(struct amdgv_adapter *adapt,
					       uint32_t idx_vf, uint64_t rptr)
{
	int ret;
	bool allow_again;

	ret = amdgv_sched_sanitize_vf_ras_req(adapt, idx_vf,
					      AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP,
					      AMDGV_GUARD_EVENT_RAS_CPER_DUMP);
	if (ret)
		return ret;

	amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_CE);

	ret = amdgv_vfmgr_dump_cpers(adapt, idx_vf, rptr, &allow_again);
	if (ret) {
		amdgv_sched_notify_vf_fail(adapt, idx_vf);
		return ret;
	}

	/* Host determined that guest is pending for more CPERs.
	 * Do not penalize for additional messages */
	if (allow_again) {
		amdgv_guard_dec_active_event(adapt, idx_vf, AMDGV_GUARD_EVENT_ALL_INT);
		amdgv_guard_dec_active_event(adapt, idx_vf, AMDGV_GUARD_EVENT_RAS_CPER_DUMP);
	}

	amdgv_sched_notify_vf_req_cper_dump_ready(adapt, idx_vf);

	return 0;
}

static int amdgv_sched_handle_req_gpu_init(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	adapt->array_vf[idx_vf].vf_status = AMDGV_VF_STATUS_START_INIT;

	amdgv_time_log_note_vf_init_start(adapt, idx_vf);

	if (amdgv_sched_context_init(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL))
		return AMDGV_FAILURE;

	if (!(adapt->array_vf[idx_vf].gpu_init_data_ready)) {
		if (adapt->fw_load_type == AMDGV_FW_LOAD_BY_GIM_PHASE_2) {
			if (amdgv_host_upload_fw_to_vf(adapt, idx_vf))
				AMDGV_WARN("upload vbios image to vf failed\n");
		}

		if (amdgv_vfmgr_copy_ip_data_to_vf(adapt, idx_vf, false))
			AMDGV_WARN("upload IP discovery data to vf failed\n");

		if (amdgv_vbios_upload_image_to_vf(adapt, idx_vf))
			AMDGV_WARN("upload vbios image to vf failed\n");

		if (amdgv_vfmgr_update_pf2vf_message(adapt, idx_vf))
			AMDGV_WARN("update pf2vf message failed\n");
	}

	adapt->array_vf[idx_vf].gpu_init_data_ready = false;

	return 0;
}

static int amdgv_sched_handle_req_gpu_fini(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	/* Don't load/run this VF if it is already in reset */
	if (is_avail_vf(idx_vf)) {
		/* still notify ready to access to unblock guest */
		amdgv_sched_notify_vf_full_access(adapt, idx_vf);
		/* normally we should not enter this code path, in case some
		 * corner case, add warning here */
		AMDGV_WARN("Attempt to requst fini for inactive VF, skip\n");
		return AMDGV_FAILURE;
	}

	adapt->array_vf[idx_vf].vf_status = AMDGV_VF_STATUS_START_UNINIT;

	amdgv_sched_remove_vf(adapt, idx_vf);

	amdgv_time_log_note_vf_fini_start(adapt, idx_vf);

	return amdgv_sched_context_load(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);
}

static int amdgv_sched_handle_rel_gpu_init(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   bool skip_save)
{
	int ret;

	amdgv_time_log_note_vf_init_end(adapt, idx_vf);

	ret = 0;

	/* get fw attestation info*/
	amdgv_psp_get_fw_attestation_info(adapt, idx_vf);

	if (!skip_save)
		ret = amdgv_sched_context_save(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);

	adapt->array_vf[idx_vf].vf_status = AMDGV_VF_STATUS_END_INIT;

	return ret;
}

static int amdgv_sched_handle_rel_gpu_fini(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = 0;

	ret = amdgv_sched_context_save(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);

	if (!adapt->lock_world_switch && amdgv_sched_shutdown_vf(adapt, idx_vf))
		return AMDGV_FAILURE;

	amdgv_psp_clear_vf_fw(adapt, idx_vf);

	amdgv_time_log_note_vf_fini_end(adapt, idx_vf);
	amdgv_time_log_update_vf_cumulative_running_time(adapt, idx_vf);

	adapt->array_vf[idx_vf].vf_status = AMDGV_VF_STATUS_END_UNINIT;
	set_to_avail_vf(idx_vf);

	return ret;
}

/**
 * amdgv_sched_full_access_left_time: - check full access status
 *
 * return value
 * not in full access
 * 		SCHED_FULL_ACCESS_NOT_ENTERED
 * full access has timed out
 * 		SCHED_FULL_ACCESS_TIMED_OUT
 * full access has time left
 * 		SCHED_FULL_ACCESS_ON_GOING
 */
static enum amdgv_sched_full_access_status amdgv_sched_full_access_left_time(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t *time_remain)
{
	int64_t left_time, used_time;

	/* it is not in full access mode */
	if (!is_vf_in_full_access(idx_vf))
		return SCHED_FULL_ACCESS_NOT_ENTERED;

	/* In chain live update mode, driver won't wait for full access mode exit
	 * because the interrupt has already been disabled. Driver will save the
	 * the start_time_full_access and restore it after live update finish.
	 */
	if (adapt->in_chain_live_update)
		return SCHED_FULL_ACCESS_NOT_ENTERED;

	if (oss_atomic_read(adapt->in_ecc_recovery)) {
		/* Trigger full access timeout early. VF cannot msg host in fatal error.
		 * Timeout handling (FLR) will also be canceled by fatal error check. */
		AMDGV_WARN("Cancel full access due to fatal error\n");
		return SCHED_FULL_ACCESS_TIMED_OUT;
	}

	AMDGV_DEBUG("now: %llu, start: %llu\n", oss_get_time_stamp(),
		    adapt->sched.start_time_full_access);

	if (adapt->sched.enable_per_partition_full_access)
		used_time = oss_get_time_stamp() - adapt->sched.array_vf[idx_vf].start_time_full_access;
	else
		used_time = oss_get_time_stamp() - adapt->sched.start_time_full_access;

	left_time = adapt->sched.allow_time_full_access - used_time;

	AMDGV_DEBUG("used_time: %lld, left_time: %lld\n", used_time, left_time);

	/* full access mode timed out */
	if (left_time < 0) {
		AMDGV_DEBUG("timeout\n");
		return SCHED_FULL_ACCESS_TIMED_OUT;
	}

	*time_remain = left_time;
	return SCHED_FULL_ACCESS_ON_GOING;
}

static bool amdgv_sched_is_event_pair(struct amdgv_adapter *adapt,
				      struct amdgv_sched_event *event)
{
	enum amdgv_sched_event_id event_id = adapt->sched.event_id_full_access;

	if (adapt->sched.enable_per_partition_full_access) {
		event_id = adapt->sched.array_vf[event->idx_vf].event_id_full_access;
	}

	switch (event_id) {
	case AMDGV_EVENT_REQ_GPU_INIT:
	case AMDGV_EVENT_REQ_GPU_RESET:
		if (event->id == AMDGV_EVENT_REL_GPU_INIT)
			return true;
		break;
	case AMDGV_EVENT_REQ_GPU_FINI:
		if (event->id == AMDGV_EVENT_REL_GPU_FINI)
			return true;
		break;
	default:
		break;
	}

	return false;
}

/* Following VM shutdown of PV2VF mailbox failures a VF can be orphaned
 * and never be shutdown. Destroy the PSP ring and if the VF is active
 * reset the GPU
 */
static int amdgv_sched_handle_orphan_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (adapt->psp.psp_ring_destroy && adapt->psp.psp_ring_destroy(adapt))
		AMDGV_WARN("psp ring destroy failed!\n");

	/* avoid to reset VF which is already not active */
	if (is_active_vf(idx_vf)) {
		amdgv_sched_remove_vf(adapt, idx_vf);

		adapt->array_vf[idx_vf].skip_run = true;
		/* switch to idx_vf for FLR. and ignore failure */
		if (amdgv_sched_context_switch_to_vf(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL) !=
		    0)
			AMDGV_WARN("Switch to %s Fail.\n", amdgv_idx_to_str(idx_vf));
		adapt->array_vf[idx_vf].skip_run = false;

		set_to_avail_vf(idx_vf);

		if (amdgv_sched_reset_vf(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL) != 0)
			return AMDGV_FAILURE;
	}

	return 0;
}

static int amdgv_sched_enter_full_access(struct amdgv_adapter *adapt,
					 struct amdgv_sched_event *event)
{
	int ret = 0;
	struct amdgv_sched_world_switch *abnormal_world_switch;

	AMDGV_INFO("start processing full access.\n");

	amdgv_sched_toggle_skip_next_punish(adapt, event->idx_vf, true);

	amdgv_sched_stop(adapt, event->idx_vf);

	// PF soft FLR need another force reset before entering full access.
	if ((event->id == AMDGV_EVENT_REQ_GPU_RESET) && (event->idx_vf == AMDGV_PF_IDX)) {
		// need to switch to PF first
		if (!amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX,
							AMDGV_SCHED_BLOCK_ALL))
			amdgv_sched_reset_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL);
	}

	if (!amdgv_sched_is_state_ok(adapt, event->idx_vf)) {
		/* identify which VF causes the failure */
		amdgv_sched_world_context_get_abnormal_world_switch(adapt,
								    &abnormal_world_switch);
		if (abnormal_world_switch != NULL) {
			AMDGV_WARN(
				"VF%d hung is encountered when stop world switch on block(%d)\n",
				abnormal_world_switch->curr_idx_vf,
				abnormal_world_switch->sched_block);
		}


		/*
		 * if the VF requested GPU_FINI causes the FLR,
		 * then reject the finish request
		 */
		if (event->id == AMDGV_EVENT_REQ_GPU_FINI && abnormal_world_switch &&
		    event->idx_vf == abnormal_world_switch->curr_idx_vf) {
			/* the reset will set the abnormal_world_switch->curr_idx_vf to PF,
			 * so the judgement need to happen before the reset */
			amdgv_sched_reset_vf_auto(adapt);
			/* Now we are in event3 so guest is unloading, but VF is reset.
			 * We can't init-run because guest is unloading, it will not init
			 * the VF. We can't just not send the READY message
			 * because guest is waiting for the message, it may wait for very
			 * long. So here, we send READY message to guest to break its
			 * waiting, but will not activate VF and not enter full access
			 * so that GIM will not be blocked for seconds.*/
			amdgv_sched_notify_vf_full_access(adapt, event->idx_vf);
			return AMDGV_FAILURE;
		} else
			amdgv_sched_reset_vf_auto(adapt);

	}

	/* On both req_gpu_reset and req_gpu_init we can have a VF that
	 * is hung or orphaned (never sent mb messages 3/4) and in that
	 * case it may be reset to ensure the state of the VF is clean
	 * to enter full access mode
	 */
	if ((event->id == AMDGV_EVENT_REQ_GPU_INIT) ||
	    (event->id == AMDGV_EVENT_REQ_GPU_RESET)) {
		if (amdgv_sched_handle_orphan_vf(adapt, event->idx_vf)) {
			AMDGV_WARN("Failed to reset orphan VF\n");
			amdgv_sched_reset_vf_auto(adapt);
			return AMDGV_FAILURE;
		}
	}

	/* Ensure the event guard is not FULL for FLR (it may have happened when
	 * dealing with orphan VF
	 */
	if (amdgv_guard_is_event_full(adapt, event->idx_vf, AMDGV_GUARD_EVENT_FLR) ==
	    AMDGV_GUARD_EVENT_OVERFLOW) {
		return AMDGV_FAILURE;
	}

	/* Enable psp mailbox ints */
	if (adapt->xgmi.phy_nodes_num > 1 && adapt->xgmi.set_mb_in_hive)
		ret = amdgv_sched_psp_set_mb_int_in_hive(adapt, event->idx_vf,
							 true);
	else
		ret = amdgv_sched_psp_set_mb_int(adapt, event->idx_vf, true);
	AMDGV_ASSERT(ret == 0);

	/* enable MMIO register write, FB, DOORBELL VF access */
	amdgv_gpuiov_set_vf_access(adapt, event->idx_vf, AMDGV_VF_ACCESS_ALL, true);

	if (adapt->sched.rlc_safe_mode)
		adapt->sched.rlc_safe_mode(adapt, true);

	switch (event->id) {
	case AMDGV_EVENT_REQ_GPU_INIT:
		/* enable mmsch for init/reset event only */
		ret = amdgv_sched_set_mmsch_vfgate(adapt, event->idx_vf, true);
		if (ret)
			break;

		ret = amdgv_mmsch_config_vf(adapt, event->idx_vf);
		if (ret)
			AMDGV_WARN("Failed to config mmsch features. Performance may be impacted\n");

		if (adapt->flags & AMDGV_FLAG_USE_PF) {
			ret = amdgv_misc_load_dfc(adapt);
			if (ret)
				break;
		}

		ret = amdgv_sched_handle_req_gpu_init(adapt, event->idx_vf);
		break;
	case AMDGV_EVENT_REQ_GPU_RESET:
		/* enable mmsch for init/reset event only */
		ret = amdgv_sched_set_mmsch_vfgate(adapt, event->idx_vf, true);
		if (ret)
			break;

		/* enable VF FB access */
		if (adapt->flags & AMDGV_FLAG_VF_FB_PROTECTION)
			amdgv_gpuiov_set_vf_access(adapt, event->idx_vf, AMDGV_VF_ACCESS_FB,
						   true);

		if (event->idx_vf != AMDGV_PF_IDX)
			ret = amdgv_mmsch_config_vf(adapt, event->idx_vf);

		if (ret)
			AMDGV_WARN("Failed to config mmsch features. Performance may be impacted\n");

		ret = amdgv_sched_handle_req_gpu_reset(adapt, event->idx_vf);

		/* reprogram VF's golden setting */
		amdgv_misc_reprogram_golden_settings(adapt, event->idx_vf);
		break;
	case AMDGV_EVENT_REQ_GPU_FINI:
		ret = amdgv_sched_handle_req_gpu_fini(adapt, event->idx_vf);
		break;
	default:
		AMDGV_ASSERT(false);
		break;
	}

	if ((ret != 0) || !amdgv_sched_is_state_ok(adapt, event->idx_vf)) {
		if (adapt->sched.rlc_safe_mode)
			adapt->sched.rlc_safe_mode(adapt, false);

		/* Disable all VF gates, this device failed to be granted full
		 * access mode
		 */
		amdgv_gpuiov_set_vf_access(adapt, event->idx_vf, AMDGV_VF_ACCESS_DOORBELL,
					   false);
		amdgv_gpuiov_set_vf_access(adapt, event->idx_vf,
					   AMDGV_VF_ACCESS_MMIO_REG_WRITE, false);
		ret = amdgv_sched_set_mmsch_vfgate(adapt, event->idx_vf, false);

		AMDGV_WARN("%s hung while processing %s!\n", amdgv_idx_to_str(event->idx_vf),
			   amdgv_event_name(event->id));

		if (amdgv_sched_is_find_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_RAS_FED)) {
			AMDGV_INFO("RAS FED is pending in event queue, abort reset routines and queue REQ_GPU_RESET instead\n");
			amdgv_sched_queue_event(adapt, event->idx_vf, AMDGV_EVENT_REQ_GPU_RESET, AMDGV_SCHED_BLOCK_ALL);
			return AMDGV_FAILURE;
		}

		ret = amdgv_sched_reset_vf_auto(adapt);

		/* Only try to to remove the PSP VF gate if the reset
		 * has been succesful, as this requires to switch into the
		 * PF. If it does not the driver is an unrecoverable error
		 * and will be handled by the event process
		 */
		if (!ret) {
			if (adapt->xgmi.phy_nodes_num > 1  && adapt->xgmi.set_mb_in_hive)
				ret = amdgv_sched_psp_set_mb_int_in_hive(
				    adapt, event->idx_vf, false);
			else
				ret = amdgv_sched_psp_set_mb_int(adapt, event->idx_vf, false);
			AMDGV_ASSERT(ret == 0);
		}

		return AMDGV_FAILURE;
	}

	if (adapt->sched.enable_per_partition_full_access) {
		/* set the logic sched fa mask to avoid other VF using common sched enter full access */
		adapt->sched.logical_sched_fa_mask |= adapt->sched.array_vf[event->idx_vf].world_switch_mask;
		adapt->sched.array_vf[event->idx_vf].in_full_access = true;
		adapt->sched.array_vf[event->idx_vf].start_time_full_access = oss_get_time_stamp();
		adapt->sched.array_vf[event->idx_vf].event_id_full_access = event->id;
		AMDGV_DEBUG("VF: %d enter full access, current VF WS mask: 0x%llx, logic sched full access mask: 0x%llx",
				event->idx_vf,
				adapt->sched.array_vf[event->idx_vf].world_switch_mask,
				adapt->sched.logical_sched_fa_mask);
	} else {
		adapt->sched.in_full_access = true;
		adapt->sched.idx_vf_full_access = event->idx_vf;
		adapt->sched.start_time_full_access = oss_get_time_stamp();
		adapt->sched.event_id_full_access = event->id;
	}

	amdgv_gpuiov_toggle_rlcg_vf_interface(adapt, event->idx_vf, true);

	/* Notify the VF of being in exclusive mode */
	amdgv_sched_notify_vf_full_access(adapt, event->idx_vf);

	AMDGV_INFO("%s entered full access mode.\n", amdgv_idx_to_str(event->idx_vf));
	if (AMDGV_EVENT_REQ_GPU_RESET == event->id &&
	    AMDGV_PF_IDX == event->idx_vf) {
		amdgv_irqmgr_disable_hw_interrupt(adapt);
		amdgv_device_func_hw_engine_fini(adapt);
		amdgv_device_func_hw_engine_init(adapt);
		amdgv_irqmgr_enable_hw_interrupt(adapt);
	}
	return 0;
}

static void amdgv_sched_exit_full_access(struct amdgv_adapter *adapt,
					 struct amdgv_sched_event *event)
{
	bool vf_init = false;
	bool one_vf = false;
	int ret = 0;
	uint32_t hw_sched_id;
	struct psp_mb_status mb_status = { 0 };

	if ((amdgv_sched_active_vf_num(adapt) == 0) ||
	    (adapt->sched.num_vf_per_gfx_sched == 1))
		one_vf = true;

	if (adapt->sched.rlc_safe_mode)
		adapt->sched.rlc_safe_mode(adapt, false);

	switch (event->id) {
	case AMDGV_EVENT_REL_GPU_INIT:
		ret = amdgv_sched_handle_rel_gpu_init(adapt, event->idx_vf, one_vf);
		vf_init = true; /* NOTE: will check VF ready in "add_vf" */

		/* TODO: we need a better way of checking if the VF has been
		 * initialized properly but for now just check that CP is running
		 * on the VF
		 * Do the check here before we close the CF page and switch to PF
		 * only if rel_gpu_init has succeeded
		 */
		if (!ret && !adapt->sched.cp_sched_state(adapt, event->idx_vf)) {
			AMDGV_WARN("CP scheduler of %s is not initialized.\n",
				   amdgv_idx_to_str(event->idx_vf));
			vf_init = false;

			if (one_vf)
				ret = amdgv_sched_context_save(adapt, event->idx_vf,
							       AMDGV_SCHED_BLOCK_GFX);

			/* need to call SHUTDOWN_VF to let RLCV really do the registers saving
			 * in the later SAVE_GPU_STATE  call, otherwise golden CSA feature will
			 * skip many registers during SAVE_GPU . vf_init false means the
			 * previous event 1 2 are not from guest KMD
			 * */
			if (!ret) {
				ret = amdgv_sched_shutdown_vf(adapt, event->idx_vf);

				if (ret)
					amdgv_put_error(event->idx_vf,
							AMDGV_ERROR_SCHED_SHUTDOWN_VF_FAIL,
							event->idx_vf);
			}
		}

		/* disable mmsch vfgate */
		ret = amdgv_sched_set_mmsch_vfgate(adapt, event->idx_vf, false);
		break;
	case AMDGV_EVENT_REL_GPU_FINI:
		/* disable Doorbell VF access */
		amdgv_gpuiov_set_vf_access(adapt, event->idx_vf, AMDGV_VF_ACCESS_DOORBELL,
					   false);

		/* disable VF FB access */
		if (adapt->flags & AMDGV_FLAG_VF_FB_PROTECTION)
			amdgv_gpuiov_set_vf_access(adapt, event->idx_vf, AMDGV_VF_ACCESS_FB,
						   false);

		ret = amdgv_sched_handle_rel_gpu_fini(adapt, event->idx_vf);
		amdgv_live_info_prepare_reset(adapt);

		/* clear vf fb on vm shutdown */
		if ((adapt->flags & AMDGV_FLAG_FB_CLEAN_ON_SHUTDOWN) &&
			amdgv_sched_clear_vf_fb(adapt, event->idx_vf, 0x00)) {
			AMDGV_WARN("clear vf fb failed\n");
		}
		break;
	default:
		AMDGV_ASSERT(false);
		break;
	}

	amdgv_gpuiov_toggle_rlcg_vf_interface(adapt, event->idx_vf, false);
	/* disable VF mmio write access from now*/
	amdgv_gpuiov_set_vf_access(adapt, event->idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE,
				   false);

	/* check if vf is hung */
	if ((ret != 0) || !amdgv_sched_is_state_ok(adapt, event->idx_vf)) {
		AMDGV_WARN("hung detected, try to reset vf.\n");

		vf_init = false;
		/* reset vf */
		if (amdgv_sched_reset_vf_auto(adapt))
			goto failed;
	}

	/* Get psp mailbox status */
	amdgv_sched_psp_get_mb_status(adapt, event->idx_vf, &mb_status);

	/* Log to mailbox failure record if there is failure */
	if (mb_status.status != AMDGV_MB_STATUS_OK) {
		amdgv_psp_save_mb_error_record(adapt, event->idx_vf, &mb_status);

		amdgv_put_error(event->idx_vf, AMDGV_ERROR_FW_MAILBOX_ERROR,
						AMDGV_ERROR_16_16_32((uint16_t)event->idx_vf, (uint16_t)mb_status.status, mb_status.fw_id));
	}

	if (adapt->xgmi.phy_nodes_num > 1  && adapt->xgmi.set_mb_in_hive)
		ret = amdgv_sched_psp_set_mb_int_in_hive(adapt, event->idx_vf,
							 false);
	else
		ret = amdgv_sched_psp_set_mb_int(adapt, event->idx_vf, false);
	AMDGV_ASSERT(ret == 0);

	if (vf_init) {
		/* NOTE: "add_vf" checks "sched_block" and takes care of
		 *   o "set_to_active_vf"
		 *   o "save_rlcv_state" for "sched_block" = GFX engine
		 */
		if (amdgv_sched_add_vf(adapt, event->idx_vf)) {
			amdgv_sched_queue_event(adapt, event->idx_vf,
						AMDGV_EVENT_SCHED_RESET_VF,
						AMDGV_SCHED_BLOCK_ALL);
			goto failed;
		}
	}

	if (!one_vf) {
		/* one time loop for active vfs */
		if (!adapt->lock_world_switch) {
			if (amdgv_sched_context_one_time_loop(adapt, event->idx_vf) != 0)
				AMDGV_WARN("one time loop failed\n");
		}
	}

	amdgv_sched_toggle_skip_next_punish(adapt, event->idx_vf, true);

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, event->idx_vf)) {
		if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode == AMDGV_SCHED_HYBRID_LIQUID_MODE)
			amdgv_gpuiov_ctx_empty_intr_control(adapt, hw_sched_id, true);
	}

failed:
	if (adapt->sched.enable_per_partition_full_access) {
		/* unset logical sched fa mask for this VF */
		adapt->sched.logical_sched_fa_mask &= ~adapt->sched.array_vf[event->idx_vf].world_switch_mask;
		adapt->sched.array_vf[event->idx_vf].in_full_access = false;
		adapt->sched.array_vf[event->idx_vf].start_time_full_access = 0;
		AMDGV_DEBUG("VF: %d exit full access, current VF WS mask: 0x%llx, logic sched full access mask: 0x%llx",
				event->idx_vf,
				adapt->sched.array_vf[event->idx_vf].world_switch_mask,
				adapt->sched.logical_sched_fa_mask);
	} else {
		adapt->sched.in_full_access = false;
		adapt->sched.idx_vf_full_access = AMDGV_INVALID_IDX_VF;
		adapt->sched.start_time_full_access = 0;
	}

	AMDGV_INFO("%s exited full access.\n", amdgv_idx_to_str(event->idx_vf));
	return;
}

static void amdgv_sched_exit_full_access_timeout(struct amdgv_adapter *adapt, uint32_t vf_idx)
{
	int ret;
	uint32_t idx_vf = adapt->sched.idx_vf_full_access;
	uint32_t event_id = adapt->sched.event_id_full_access;
	struct psp_mb_status mb_status = { 0 };

	if (adapt->sched.enable_per_partition_full_access) {
		idx_vf = vf_idx;
		event_id = adapt->sched.array_vf[idx_vf].event_id_full_access;
	}


	AMDGV_INFO("%s full access timed out!\n", amdgv_idx_to_str(idx_vf));

	set_to_avail_vf(idx_vf);

	if (adapt->sched.rlc_safe_mode)
		adapt->sched.rlc_safe_mode(adapt, false);

	/* disable doorbell and MMIO register write VF access */
	amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_DOORBELL, false);
	amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, false);

	amdgv_put_error_ext(idx_vf, AMDGV_ERROR_DRIVER_FULL_ACCESS_TIMEOUT, idx_vf,
						adapt->sched.start_time_full_access, oss_get_time_stamp());

	ret = amdgv_guard_add_active_event(adapt, idx_vf, AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT);
	if (ret == AMDGV_FAILURE)
		AMDGV_WARN("Add active timeout event failed!\n");

	/* disable mmsched interrupts */
	if ((event_id == AMDGV_EVENT_REQ_GPU_INIT) ||
	    (event_id == AMDGV_EVENT_REQ_GPU_RESET)) {
		ret = amdgv_sched_set_mmsch_vfgate(adapt, idx_vf, false);
		if (ret)
			goto out;
	}

	/* Skip world context saving process if MMIO protection feature enabled. */
	if (!adapt->gpuiov.funcs->set_vf_access)
		/* try idle and save vf before flr */
		amdgv_sched_context_save(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);

	/* reset vf */
	ret = amdgv_sched_reset_vf(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);
	if (ret)
		goto out;

	/* Get vf fw attestation*/
	amdgv_psp_get_fw_attestation_info(adapt, idx_vf);

	/* Get psp mailbox status */
	amdgv_sched_psp_get_mb_status(adapt, idx_vf, &mb_status);

	/* Log to mailbox failure record if there is failure */
	if (mb_status.status != AMDGV_MB_STATUS_OK) {
		amdgv_psp_save_mb_error_record(adapt, idx_vf, &mb_status);

		amdgv_put_error(idx_vf, AMDGV_ERROR_FW_MAILBOX_ERROR,
						AMDGV_ERROR_16_16_32((uint16_t)idx_vf, (uint16_t)mb_status.status, mb_status.fw_id));
	}

	/* one time loop */
	ret = amdgv_sched_context_one_time_loop(adapt, idx_vf);
	if (ret) {
		AMDGV_WARN("one time loop failed\n");
		goto out;
	}

out:
	if (adapt->xgmi.phy_nodes_num > 1 && adapt->xgmi.set_mb_in_hive)
		ret = amdgv_sched_psp_set_mb_int_in_hive(adapt, idx_vf, false);
	else
		ret = amdgv_sched_psp_set_mb_int(adapt, idx_vf, false);
	AMDGV_ASSERT(ret == 0);

	amdgv_gpuiov_toggle_rlcg_vf_interface(adapt, idx_vf, false);

	if (adapt->sched.enable_per_partition_full_access) {
		/* unset logical sched fa mask for this VF */
		adapt->sched.logical_sched_fa_mask &= ~adapt->sched.array_vf[idx_vf].world_switch_mask;
		adapt->sched.array_vf[idx_vf].in_full_access = false;
		adapt->sched.array_vf[idx_vf].start_time_full_access = 0;
	} else {
		adapt->sched.in_full_access = false;
		adapt->sched.idx_vf_full_access = AMDGV_INVALID_IDX_VF;
		adapt->sched.start_time_full_access = 0;
	}
	adapt->array_vf[idx_vf].gpu_init_data_ready = false;

	/* Restart the scheduler if we are succesful even if no
	 * vf are active (PF must be scheduled)
	 * For partition full access, only restart schedulers
	 * of current VF
	 */
	if (ret == 0) {
		if (adapt->sched.enable_per_partition_full_access)
			amdgv_sched_start(adapt, idx_vf);
		else
			amdgv_sched_start_all(adapt);
	}
}

static int amdgv_sched_toggle_full_access_for_debug(struct amdgv_adapter *adapt, int idx_vf, bool enable)
{
	int ret = 0;

	if (enable) {
		/* notify other VFs not to submit new fences */
		amdgv_mailbox_notify_gpu_debug(adapt, idx_vf, !enable);

		/* simulate vf full access mode before it enters live update save */
		ret = amdgv_sched_psp_set_mb_int(adapt, idx_vf, enable);
		/* enable MMIO register write, FB, DOORBELL VF access */
		ret = amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_ALL, enable);

		if (adapt->sched.rlc_safe_mode)
			ret = adapt->sched.rlc_safe_mode(adapt, enable);

		/* enable mmsch for init/reset event only */
		ret = amdgv_sched_set_mmsch_vfgate(adapt, idx_vf, enable);
		/* enable rlcg vfgate*/
		ret = amdgv_gpuiov_toggle_rlcg_vf_interface(adapt, idx_vf, enable);
	} else {
		if (adapt->sched.rlc_safe_mode)
			ret = adapt->sched.rlc_safe_mode(adapt, enable);

		/* disable mmsch vfgate */
		ret = amdgv_sched_set_mmsch_vfgate(adapt, idx_vf, enable);
		/* simulate vf full access mode before it enters live update save */
		ret = amdgv_sched_psp_set_mb_int(adapt, idx_vf, enable);
		/* disable rlcg vfgate*/
		ret = amdgv_gpuiov_toggle_rlcg_vf_interface(adapt, idx_vf, enable);
		/* disable VF mmio write access from now */
		ret = amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, enable);
		/* notify other VFs to continue */
		amdgv_mailbox_notify_gpu_debug(adapt, idx_vf, !enable);
		/* start ws and clear beyond_time_cycle */
		amdgv_sched_clear_timeslice(adapt, idx_vf);
		amdgv_sched_start(adapt, idx_vf);
	}

	return ret;
}

static int amdgv_sched_enter_power_saving(struct amdgv_adapter *adapt)
{
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	uint32_t world_switch_id = 0;
	struct amdgv_sched_world_switch *world_switch;

	uint32_t idx_tmp = AMDGV_PF_IDX;
	int i = 0;
	uint32_t hw_sched_id = 0;

	for (i = 0; i < adapt->num_vf; i++) {
		if (is_active_vf(i))
			return AMDGV_ERROR_GPUMON_VF_BUSY;
	}

	ret = amdgv_sched_stop_all(adapt);
	if (!ret) {
		adapt->lock_world_switch = true;
		for (world_switch_id = 0; world_switch_id < adapt->sched.num_world_switch;
		     world_switch_id++) {
			world_switch = &adapt->sched.world_switch[world_switch_id];
			amdgv_sched_world_context_get_curr_vf(adapt, world_switch, &idx_tmp);
			if (idx_tmp != AMDGV_PF_IDX)
				ret = amdgv_sched_world_context_switch_to_vf(
					adapt, AMDGV_PF_IDX, world_switch);

			for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
				/* PF will be invalid after BACO power saving, mark them as SHUTDOWN */
				adapt->sched.array_vf[AMDGV_PF_IDX].cur_vf_state[hw_sched_id] =
					AMDGV_SHUTDOWN_GPU;
				adapt->sched.hw_state_machine[hw_sched_id].cur_gpu_state =
					AMDGV_SHUTDOWN_GPU;
			}
		}

		amdgv_sched_world_context_clear_state_rst(adapt);

		/* parameter should be checked before we call into pp_function */
		ret = adapt->pp.pp_funcs->enter_power_saving(adapt);
		if (ret && !amdgv_sched_is_unrecov_err(adapt))
			adapt->lock_world_switch = false;
	}

	return ret;
}

static int amdgv_sched_exit_power_saving(struct amdgv_adapter *adapt)
{
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	/* parameter should be checked before we call into pp_function */
	ret = adapt->pp.pp_funcs->exit_power_saving(adapt);
	if (!ret)
		adapt->lock_world_switch = false;

	return ret;
}

static int amdgv_sched_handle_fed(struct amdgv_adapter *adapt,
			struct amdgv_sched_event *event)
{
	if (adapt->ecc.poison_consumption) {
		adapt->ecc.poison_consumption(adapt, event);
	}
	return 0;
}

static void amdgv_sched_check_vf2pf_data(struct amdgv_adapter *adapt,
			uint32_t idx_vf)
{
	struct amd_sriov_msg_vf2pf_info *vf2pf_info = NULL;
	bool guest_driver_version_supported = true;

	vf2pf_info = (struct amd_sriov_msg_vf2pf_info *)
			oss_zalloc(sizeof(struct amd_sriov_msg_vf2pf_info));
	if (vf2pf_info == NULL)
		AMDGV_WARN("Failed to allocate vf2pf_info\n");
	else {
		if (amdgv_vfmgr_retrieve_vf2pf_message(adapt, idx_vf, vf2pf_info))
			AMDGV_WARN("retrieve vf2pf message failed\n");
		else
			amdgv_vfmgr_print_vf2pf_message(adapt, idx_vf, vf2pf_info);

		if (adapt->psp.dfc_check_guest_version &&
			adapt->psp.dfc_check_guest_version(adapt, vf2pf_info->driver_version)) {
			AMDGV_ERROR("guest version check failed\n");
			guest_driver_version_supported = false;
		}
		if (!guest_driver_version_supported) {
			amdgv_sched_queue_stop_vf(adapt, idx_vf);
		}
		amdgv_mmsch_reconfig_vf(adapt, idx_vf, vf2pf_info);
		oss_free(vf2pf_info);
	}
}

/* NOTE: This is a destructive event that permanently stops guest services.
* This logic will need to be re-written if runtime RMA recovery is required.
*/
static int amdgv_sched_event_handle_rma(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;

	if (adapt->ecc.bad_page_detection_mode & BIT(AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return 0;

	amdgv_sched_stop_all(adapt);

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		/* diable all VF access */
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_FB, false);
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_DOORBELL, false);
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, false);
		amdgv_gpuiov_toggle_rlcg_vf_interface(adapt, idx_vf, false);

		if (!is_active_vf(idx_vf))
			continue;

		amdgv_sched_remove_vf(adapt, idx_vf);
		amdgv_sched_shutdown_vf(adapt, idx_vf);
		set_to_unavail_vf(idx_vf);
	}

	if (amdgv_ras_eeprom_is_gpu_bad(adapt)) {
		amdgv_device_generate_rma_cper(adapt);
		amdgv_device_report_rma_to_fw(adapt);
		amdgv_device_set_status(adapt, AMDGV_STATUS_HW_RMA);
	} else {
		amdgv_device_set_status(adapt, AMDGV_STATUS_HW_HIVE_RMA);
	}

	return 0;
}

static int handle_event_in_full_access(struct amdgv_adapter *adapt,
				       struct amdgv_sched_event *event)
{
	int ret = 0, i = 0;
	uint32_t world_switch_id = 0;
	struct amdgv_sched_vf_info *vf_info;

	if (is_unavail_vf(event->idx_vf) &&
	    ((event->id != AMDGV_EVENT_SCHED_FORCE_RESET_GPU) &&
	     (event->id != AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL) &&
		 (event->id != AMDGV_EVENT_SCHED_RMA) &&
	     (event->id != AMDGV_EVENT_SCHED_INIT_VF_FB) &&
	     (event->id != AMDGV_EVENT_SCHED_RAS_UMC) &&
	     (event->id != AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION) &&
	     (event->id != AMDGV_EVENT_SCHED_RAS_POISON_CREATION) &&
	     (event->id != AMDGV_EVENT_SCHED_SUSPEND) &&
	     (event->id != AMDGV_EVENT_SCHED_RESUME) &&
	     (event->id != AMDGV_EVENT_SCHED_SUSPEND_LIVE) &&
	     (event->id != AMDGV_EVENT_SCHED_RESUME_LIVE) &&
	     (event->id != AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC) &&
	     (event->id != AMDGV_EVENT_SCHED_GPUMON) &&
	     (event->id != AMDGV_EVENT_COLLECT_DIAG_DATA) &&
	     (event->id != AMDGV_EVENT_EXIT_POWER_SAVING) &&
	     (event->id != AMDGV_EVENT_ENTER_POWER_SAVING) &&
	     (event->id != AMDGV_EVENT_SCHED_SET_VF_ACCESS) &&
	     (event->id != AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION) &&
	     (event->id != AMDGV_EVENT_SCHED_PSP_VF_GATE) &&
	     (event->id != AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY) &&
	     (event->id != AMDGV_EVENT_SCHED_GET_TOPOLOGY) &&
	     (event->id != AMDGV_EVENT_SCHED_VF_REQ_RAS_ERROR_COUNT) &&
	     (event->id != AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP)))
		return 0;

	/*
	 * If the scheduler is locked in live update, defer all events till we get a resume
	 */
	if (adapt->lock_world_switch &&
	    (event->id != AMDGV_EVENT_EXIT_POWER_SAVING) &&
		(event->id != AMDGV_EVENT_SCHED_RESUME_LIVE) &&
		// No need to defer events if scheduler is locked due to live update
		(!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) ||
		  (adapt->live_update_state == AMDGV_LIVE_UPDATE_NONE))) {
		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		goto out;
	}

	/* If the scheduler is locked defer all events till we get a exit power saving */
	if (adapt->pp.is_in_powersaving && (event->id != AMDGV_EVENT_EXIT_POWER_SAVING)) {
		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		goto out;
	}

	switch (event->id) {
	case AMDGV_EVENT_SCHED_RAS_UMC:
		amdgv_ecc_check_for_errors(adapt, event);
		break;
	case AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION:
		amdgv_sched_handle_ras_poison_consumption(adapt, event);
		break;
	case AMDGV_EVENT_SCHED_RAS_POISON_CREATION:
		amdgv_sched_handle_ras_poison_creation(adapt, event);
		break;
	case AMDGV_EVENT_REQ_GPU_INIT_DATA:
	case AMDGV_EVENT_REQ_GPU_INIT:
	case AMDGV_EVENT_REQ_GPU_FINI:
	case AMDGV_EVENT_REQ_GPU_RESET:
	case AMDGV_EVENT_SCHED_GPUMON:
	case AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION:
	case AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS:
	case AMDGV_EVENT_REQ_GPU_DEBUG:
		/* If in full access, wait until current fullaccess is exited */
		if ((adapt->flags & AMDGV_FLAG_USE_PF) && (event->idx_vf == AMDGV_PF_IDX) &&
		    (event->id == AMDGV_EVENT_REQ_GPU_RESET) &&
		    adapt->reset.reset_notify_vf_pending) {
			/* ignore if we are in WGR and waiting for PF re-init done
			 * and handle it in amdgv_sched_world_context_init_pf_state
			 */
			break;
		}

		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		break;

	case AMDGV_EVENT_SCHED_SUSPEND:
	case AMDGV_EVENT_SCHED_RESUME:
		if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) && (adapt->live_update_state == AMDGV_LIVE_UPDATE_SAVE)) {
			// No need to resume scheduler during GPUV live update shutdown (after vmphu save)
			ret = AMDGV_EVENT_STOP_AND_RELEASE;
		} else if (!is_full_access_vf(AMDGV_PF_IDX)) {
			amdgv_sched_push_back_event(adapt, event);
			ret = AMDGV_EVENT_STOP_AND_KEEP;
		}
		break;

	case AMDGV_EVENT_SCHED_SUSPEND_LIVE:
		adapt->lock_world_switch = true;
		if (adapt->sched.enable_per_partition_full_access) {
			for (i = 0; i < adapt->num_vf; i++) {
				if (is_vf_in_full_access(i)) {
					vf_info = &adapt->sched.array_vf[i];
					vf_info->used_time_full_access =
						oss_get_time_stamp() - vf_info->start_time_full_access;
				}
			}
		} else {
			adapt->sched.used_time_full_access =
				oss_get_time_stamp() - adapt->sched.start_time_full_access;
		}

		/* For asics that have multiple hw sched, manually
		 * set other world_switchs' switch_running to false.
		 */
		for_each_id(world_switch_id,
			amdgv_sched_get_world_switch_mask(adapt, event->idx_vf))
				adapt->sched.world_switch[world_switch_id].switch_running = false;

		if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) {
			AMDGV_WARN("Warning: VF %d is in full access. \n", adapt->sched.idx_vf_full_access);

			if (!amdgv_sched_context_save_all(adapt, AMDGV_SCHED_BLOCK_ALL) ||
				!amdgv_sched_world_context_all_states_ok(adapt))
				amdgv_sched_reset_vf_auto(adapt);

			/* switch to PF for all blocks */
			if (0 == amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL))
				AMDGV_INFO("Switch to PF OK.\n");
			else
				AMDGV_ERROR("Switch to PF Fail.\n");
		}

		ret = AMDGV_EVENT_STOP_AND_RELEASE;
		/* Flag used to skip fullaccess_timeout check in sched_event_process_thread()
		 * if doing live update in full access mode. When start processing live update,
		 * interrupt will be disabled and host won't receive any mailbox events from guest.
		 * Thus, full access check needs to be skipped. The full access related infos
		 * will be saved an restored after live update so host will still be in full access
		 * when loading the host driver back.
		 */
		adapt->sched.skip_full_access_timeout_check = true;
		break;

	case AMDGV_EVENT_SCHED_RESUME_LIVE:
		/* Note: this event is not expected to occur during GPUV live update in full access,
		 * since we restore VF full access info and state during RESUME_LIVE.
		 * So this event would only occur in non full access.
		 */
		adapt->lock_world_switch = false;
		if (adapt->sched.enable_per_partition_full_access) {
			for (i = 0; i < adapt->num_vf; i++) {
				if (is_vf_in_full_access(i)) {
					vf_info = &adapt->sched.array_vf[i];
					vf_info->start_time_full_access =
						oss_get_time_stamp() - vf_info->used_time_full_access;
					vf_info->used_time_full_access = 0;
				}
			}
		} else {
			adapt->sched.start_time_full_access = oss_get_time_stamp() - adapt->sched.used_time_full_access;
			adapt->sched.used_time_full_access = 0;
		}
		adapt->sched.skip_full_access_timeout_check = false;

		if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) && (adapt->live_update_state == AMDGV_LIVE_UPDATE_ULTRALITE)) {
			AMDGV_INFO("Live Update Restore: VF%u continue full access mode.\n", adapt->sched.idx_vf_full_access);
			if (amdgv_sched_context_switch_to_vf(adapt, adapt->sched.idx_vf_full_access, AMDGV_SCHED_BLOCK_ALL))
				AMDGV_ERROR("Switch to VF %u fail\n", adapt->sched.idx_vf_full_access);
		}

		ret = AMDGV_EVENT_STOP_AND_RELEASE;
		break;

	case AMDGV_EVENT_REL_GPU_INIT:
		if (is_full_access_vf(event->idx_vf)) {
			/* if event isn't paired, it's abnormal in guest drv.*/
			if (!amdgv_sched_is_event_pair(adapt, event)) {
				AMDGV_INFO(
					"Received REL_GPU_INIT without a corresponding REQ_GPU_INIT\n");
				goto out;
			}

			/* if PF is used, and we just finish whole GPU reset,
			 * program vf mc settings before exiting full access */
			if (adapt->flags & AMDGV_FLAG_USE_PF &&
			    event->idx_vf == AMDGV_PF_IDX) {
				if (amdgv_reset_program_vf_mc_settings(adapt)) {
					AMDGV_WARN("Failed to program vf mc settings\n");
				}
			}

			if (adapt->misc.update_dummy_page_addr)
				adapt->misc.update_dummy_page_addr(adapt, event->idx_vf);

			amdgv_sched_exit_full_access(adapt, event);

			/* if PF is used, notify VF that reset has completed
			   until PF reset has finished */
			if (adapt->flags & AMDGV_FLAG_USE_PF &&
			    event->idx_vf == AMDGV_PF_IDX) {
				amdgv_reset_mailbox_notify_after_pf(adapt);
			}
			if (event->idx_vf != AMDGV_PF_IDX) {
				amdgv_sched_check_vf2pf_data(adapt, event->idx_vf);
			}
		}
		break;

	case AMDGV_EVENT_REL_GPU_FINI:
		if (is_full_access_vf(event->idx_vf)) {
			/* if event isn't paired, it's abnormal in guest drv.*/
			if (!amdgv_sched_is_event_pair(adapt, event)) {
				AMDGV_INFO(
					"Received REL_GPU_FINI without a corresponding REQ_GPU_FINI\n");
				amdgv_sched_exit_full_access_timeout(adapt, event->idx_vf);
				goto out;
			}

			amdgv_sched_exit_full_access(adapt, event);

		}
		break;

	case AMDGV_EVENT_SCHED_FORCE_RESET_VF:
		if (is_full_access_vf(event->idx_vf)) {
			AMDGV_DEBUG(
				"VF not in full access. Skip force reset! Please report.\n");
		} else {
			/* push the event back to event list,
			   wait to exit full access. */
			amdgv_sched_push_back_event(adapt, event);
			ret = AMDGV_EVENT_STOP_AND_KEEP;
		}
		break;

	case AMDGV_EVENT_SCHED_INIT_VF_FB:
		if (is_full_access_vf(AMDGV_PF_IDX)) {
			/*
			 * We need to be in PF context for these operations.
			 * If we are already in PF full access,
			 * program the settings here and initialize VF FB
			 */
			if (amdgv_vfmgr_init_vf_fb(adapt, event->idx_vf, false,
						   event->data.vf_fb_data.pattern,
						   event->data.vf_fb_data.flag))
				AMDGV_WARN("Failed to init vf fb in full access\n");
		} else {
			/* push the event back to event list,
			 * wait to exit full access.
			 */
			amdgv_sched_push_back_event(adapt, event);
			ret = AMDGV_EVENT_STOP_AND_KEEP;
		}
		break;

	case AMDGV_EVENT_HW_SCHED_RESET_VF:
		amdgv_irqmgr_dump_temperature(adapt);
		/* push the event back to event list,
		   wait to exit full access. */
		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		break;
	case AMDGV_EVENT_SCHED_RESET_VF:
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU:
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL:
	case AMDGV_EVENT_SCHED_RMA:
	case AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC:
	case AMDGV_EVENT_SCHED_SET_VF_ACCESS:
	case AMDGV_EVENT_SCHED_PSP_VF_GATE:
		/* push the event back to event list,
		   wait to exit full access. */
		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		break;

	case AMDGV_EVENT_SCHED_SUSPEND_VF:
		if (is_full_access_vf(event->idx_vf)) {
			AMDGV_WARN("Cannot suspend %s in full access.\n",
				   amdgv_idx_to_str(event->idx_vf));
		} else if (is_active_vf(event->idx_vf)) {
			/* if the VF is active VF, move VF out of active list */
			set_to_suspend_vf(event->idx_vf);
			amdgv_sched_remove_vf(adapt, event->idx_vf);
		}

		break;

	case AMDGV_EVENT_SCHED_RESUME_VF:
		if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) && (adapt->live_update_state == AMDGV_LIVE_UPDATE_SAVE)) {
			AMDGV_WARN("GPUV live update GPUV live update save, skip resume %s\n",
				   amdgv_idx_to_str(event->idx_vf));
		} else if (is_full_access_vf(event->idx_vf)) {
			AMDGV_WARN("Cannot resume %s in full access.\n",
				   amdgv_idx_to_str(event->idx_vf));
		} else if (is_suspend_vf(event->idx_vf)) {
			amdgv_sched_add_vf(adapt, event->idx_vf);
			set_to_active_vf(event->idx_vf);

			if (AMDGV_PF_IDX != event->idx_vf) {
				if (amdgv_vfmgr_update_pf2vf_message(adapt, event->idx_vf))
					AMDGV_WARN("update pf2vf message failed\n");
			}
		}
		break;

	case AMDGV_EVENT_SCHED_REMOVE_VF:
		if (is_full_access_vf(event->idx_vf))
			amdgv_sched_exit_full_access_timeout(adapt, event->idx_vf);

		/* if the VF is active VF, move VF out of world switching. */
		if (is_active_vf(event->idx_vf))
			amdgv_sched_remove_vf(adapt, event->idx_vf);

		set_to_unavail_vf(event->idx_vf);
		break;

	case AMDGV_EVENT_SCHED_STOP_VF:
		if (is_full_access_vf(event->idx_vf))
			break;

		/* if the VF is active VF, move VF out of world switching. */
		if (is_active_vf(event->idx_vf)) {
			AMDGV_INFO("In fullaccess set stop VF%d event \n", event->idx_vf);
			adapt->array_vf[event->idx_vf].unshutdown = true;
			amdgv_sched_event_queue_push_ex(adapt, event->idx_vf,
							AMDGV_EVENT_HANDLE_CRASH,
							AMDGV_SCHED_BLOCK_ALL,
							OSS_INVALID_HANDLE, event->data, true);
		}
		break;
	case AMDGV_EVENT_HANDLE_CRASH:
		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		break;

	case AMDGV_EVENT_COLLECT_DIAG_DATA:
		*event->data.diag_data.result =
			amdgv_diag_data_collect(adapt, adapt->bdf,
							event->data.diag_data.data,
							event->data.diag_data.size);
		break;

	case AMDGV_EVENT_EXIT_POWER_SAVING:
	case AMDGV_EVENT_ENTER_POWER_SAVING:
		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		break;
	case AMDGV_EVENT_SCHED_RAS_FED:
		amdgv_sched_handle_fed(adapt, event);
		break;
	case AMDGV_EVENT_CUR_VF_CTX_EMPTY:
	case AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY:
	case AMDGV_EVENT_SCHED_GET_TOPOLOGY:
		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		break;
	case AMDGV_EVENT_SCHED_VF_REQ_RAS_ERROR_COUNT:
		ret = amdgv_sched_handle_vf_req_ras_error_count(adapt, event->idx_vf);
		break;
	case AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP:
		ret = amdgv_sched_handle_vf_req_cper_dump(adapt, event->idx_vf,
							  event->data.cper_vf.rptr);
		break;
	default:
		break;
	}

out:
	return ret;
}

static int handle_event_in_non_full_access(struct amdgv_adapter *adapt,
					   struct amdgv_sched_event *event)
{
	int ret = 0;
	struct amdgv_sched_world_switch *world_switch;
	uint32_t vf_id;
	uint32_t world_switch_id = 0;

	if (is_unavail_vf(event->idx_vf) &&
	    ((event->id != AMDGV_EVENT_SCHED_FORCE_RESET_GPU) &&
	     (event->id != AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL) &&
		 (event->id != AMDGV_EVENT_SCHED_RMA) &&
	     (event->id != AMDGV_EVENT_SCHED_INIT_VF_FB) &&
	     (event->id != AMDGV_EVENT_SCHED_RAS_UMC) &&
	     (event->id != AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION) &&
	     (event->id != AMDGV_EVENT_SCHED_RAS_POISON_CREATION) &&
	     (event->id != AMDGV_EVENT_SCHED_SUSPEND) &&
	     (event->id != AMDGV_EVENT_SCHED_RESUME) &&
	     (event->id != AMDGV_EVENT_SCHED_SUSPEND_LIVE) &&
	     (event->id != AMDGV_EVENT_SCHED_RESUME_LIVE) &&
	     (event->id != AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC) &&
	     (event->id != AMDGV_EVENT_SCHED_GPUMON) &&
	     (event->id != AMDGV_EVENT_SCHED_PSP_VF_CMD_RELAY) &&
	     (event->id != AMDGV_EVENT_COLLECT_DIAG_DATA) &&
	     (event->id != AMDGV_EVENT_EXIT_POWER_SAVING) &&
	     (event->id != AMDGV_EVENT_ENTER_POWER_SAVING) &&
	     (event->id != AMDGV_EVENT_SCHED_SET_VF_ACCESS) &&
	     (event->id != AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION) &&
	     (event->id != AMDGV_EVENT_SCHED_PSP_VF_GATE) &&
	     (event->id != AMDGV_EVENT_SCHED_RAS_FED) &
	     (event->id != AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY) &&
		 (event->id != AMDGV_EVENT_SCHED_GET_TOPOLOGY) &&
	     (event->id != AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS)))
		return 0;

	/*
	 * If the scheduler is locked defer all events till we get a resume
	 */
	if (adapt->lock_world_switch && (event->id != AMDGV_EVENT_SCHED_RESUME) &&
	    (event->id != AMDGV_EVENT_EXIT_POWER_SAVING) &&
		(event->id != AMDGV_EVENT_SCHED_RESUME_LIVE) &&
		(!(adapt->debug.in_live_debugging && event->id == AMDGV_EVENT_REL_GPU_DEBUG)) &&
		// No need to defer events if scheduler is locked due to live update
		(!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) ||
		  (adapt->live_update_state == AMDGV_LIVE_UPDATE_NONE))) {
		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		goto out;
	}

	/* If the scheduler is locked defer all events till we get a exit power saving */
	if (adapt->pp.is_in_powersaving && (event->id != AMDGV_EVENT_EXIT_POWER_SAVING)) {
		amdgv_sched_push_back_event(adapt, event);
		ret = AMDGV_EVENT_STOP_AND_KEEP;
		goto out;
	}

	switch (event->id) {
	case AMDGV_EVENT_REQ_GPU_INIT_DATA:
		/* reprogram VF's golden setting */
		amdgv_misc_reprogram_golden_settings(adapt, event->idx_vf);

		if (adapt->flags & AMDGV_FLAG_VF_FB_PROTECTION)
			amdgv_gpuiov_set_vf_access(adapt, event->idx_vf, AMDGV_VF_ACCESS_FB,
						   true);

		ret = amdgv_mmsch_preconfig_vf(adapt, event->idx_vf);
		if (ret)
			AMDGV_WARN("Failed to pre-config mmsch features. Performance may be impacted\n");

		amdgv_sched_handle_req_gpu_init_data(adapt, event->idx_vf);
		break;
	case AMDGV_EVENT_REQ_GPU_FINI:
		/* only active VF can do FINI */
		if (!is_active_vf(event->idx_vf)) {
			AMDGV_WARN("REQ_GPU_FINI is not valid for non-active %s.\n",
				   amdgv_idx_to_str(event->idx_vf));
			goto out;
		}

		/* enter full access mode to complete this event.
		 * then stop processing the remaining events and try to fetch
		 * new events.
		 */
		if (!amdgv_sched_enter_full_access(adapt, event))
			ret = AMDGV_EVENT_STOP_AND_RELEASE;
		break;

	case AMDGV_EVENT_REQ_GPU_INIT:
		/* unavailable VF cannot do INIT */
		if (is_unavail_vf(event->idx_vf)) {
			AMDGV_DEBUG("REQ_GPU_INIT is not valid for unavailable %s\n",
				    amdgv_idx_to_str(event->idx_vf));
			goto out;
		}

		/* enter full access mode to complete this event.
		 * then stop processing the remaining events and try to fetch
		 * new events.
		 */
		if (!amdgv_sched_enter_full_access(adapt, event))
			ret = AMDGV_EVENT_STOP_AND_RELEASE;

		break;

	case AMDGV_EVENT_REQ_GPU_RESET:
		/* only available or active VF can do RESET */
		if (!(is_avail_vf(event->idx_vf) || is_active_vf(event->idx_vf))) {
			AMDGV_DEBUG("%s must be active or available for REQ_GPU_RESET\n",
				    amdgv_idx_to_str(event->idx_vf));
			goto out;
		}

		/* enter full access mode to complete this event.
		 * then stop processing the remaining events and try to fetch
		 * new events.
		 */
		if (!amdgv_sched_enter_full_access(adapt, event))
			ret = AMDGV_EVENT_STOP_AND_RELEASE;

		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ERROR_RESET_VF,
				  "Reset %s requested by guest OS. REQ_GPU_RESET on %s\n",
				  amdgv_idx_to_str(event->idx_vf),
				  amdgv_sched_block_to_name(event->sched_block));

		break;

	case AMDGV_EVENT_REL_GPU_INIT:
	case AMDGV_EVENT_REL_GPU_FINI:
		break;

	case AMDGV_EVENT_SCHED_SUSPEND:
		// No need to suspend again if scheduler is already locked (such as after vmphu save)
		if (adapt->lock_world_switch) {
			ret = AMDGV_EVENT_STOP_AND_RELEASE;
			break;
		}

		amdgv_sched_stop_all(adapt);

		if (!amdgv_sched_world_context_all_states_ok(adapt))
			amdgv_sched_reset_vf_auto(adapt);

		adapt->lock_world_switch = true;

		ret = AMDGV_EVENT_STOP_AND_RELEASE;
		break;

	case AMDGV_EVENT_SCHED_SUSPEND_LIVE:
		if ((adapt->flags & AMDGV_FLAG_USE_PF) ||
		    !((adapt->sched.num_vf_per_gfx_sched == 1) &&
		      (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH))) {
			amdgv_sched_stop_all(adapt);

			if (!amdgv_sched_world_context_all_states_ok(adapt))
				amdgv_sched_reset_vf_auto(adapt);
		} else {
			for_each_id(world_switch_id,
				     amdgv_sched_get_world_switch_mask(adapt, event->idx_vf))
				adapt->sched.world_switch[world_switch_id].switch_running =
					false;
		}

		adapt->lock_world_switch = true;

		if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) {
			/* switch to PF for all blocks */
			if (0 == amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL))
				AMDGV_INFO("Switch to PF OK.\n");
			else
				AMDGV_ERROR("Switch to PF Fail.\n");
		}

		ret = AMDGV_EVENT_STOP_AND_RELEASE;
		break;

	case AMDGV_EVENT_SCHED_RESUME_LIVE:
		if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) && (adapt->live_update_state == AMDGV_LIVE_UPDATE_RESTORE)) {
			AMDGV_INFO("Import scheduler data.\n");
			amdgv_import_data_by_op(adapt, AMDGV_LIVE_INFO_DATA__SCHED);

			AMDGV_INFO("Import unprocessed events.\n");
			amdgv_import_data_by_op(adapt, AMDGV_LIVE_INFO_DATA__UNPROCESSED_EVENT);

			if (is_in_full_access()) {
				AMDGV_INFO("Live Update Restore: VF%u continue full access mode.\n", adapt->sched.idx_vf_full_access);
				adapt->sched.start_time_full_access = oss_get_time_stamp();

				if (amdgv_sched_context_switch_to_vf(adapt, adapt->sched.idx_vf_full_access, AMDGV_SCHED_BLOCK_ALL))
					AMDGV_ERROR("Switch to VF %u fail\n", adapt->sched.idx_vf_full_access);

				/* re-enter VF full access mode */
				amdgv_sched_psp_set_mb_int(adapt, adapt->sched.idx_vf_full_access, true);
				amdgv_gpuiov_set_vf_access(adapt, adapt->sched.idx_vf_full_access, AMDGV_VF_ACCESS_ALL, true);
				if (adapt->sched.rlc_safe_mode)
					adapt->sched.rlc_safe_mode(adapt, true);
				amdgv_sched_set_mmsch_vfgate(adapt, adapt->sched.idx_vf_full_access, true);
				amdgv_gpuiov_toggle_rlcg_vf_interface(adapt, adapt->sched.idx_vf_full_access, true);
			}
		} else {
			/* switch_running set to false in SUSPEND_LIVE,
			 * so set it back to true in RESUME_LIVE
			 */
			for_each_id(world_switch_id,
				amdgv_sched_get_world_switch_mask(adapt, event->idx_vf))
					adapt->sched.world_switch[world_switch_id].switch_running = true;
		}

		/* fall through */
	case AMDGV_EVENT_SCHED_RESUME:
		adapt->lock_world_switch = false;

		ret = AMDGV_EVENT_STOP_AND_RELEASE;
		break;

	case AMDGV_EVENT_SCHED_INIT_VF_FB:
		amdgv_sched_stop(adapt, event->idx_vf);

		if (!amdgv_sched_is_state_ok(adapt, event->idx_vf))
			amdgv_sched_reset_vf_auto(adapt);

		if (amdgv_sched_context_switch_gfx_to_pf(adapt, event->idx_vf) != 0)
			amdgv_sched_reset_vf_auto(adapt);

		if (amdgv_vfmgr_init_vf_fb(adapt, event->idx_vf, false,
					   event->data.vf_fb_data.pattern,
					   event->data.vf_fb_data.flag))
			AMDGV_WARN("Failed to init vf fb in non-full access\n");

		if (amdgv_sched_context_save(adapt, event->idx_vf, AMDGV_SCHED_BLOCK_GFX))
			amdgv_sched_reset_vf_auto(adapt);

		break;

	case AMDGV_EVENT_SCHED_FORCE_RESET_VF:
		/* skip if vf is not active */
		if (!(is_active_vf(event->idx_vf))) {
			AMDGV_DEBUG("Ignore FORCE_RESET_VF for non-active %s\n",
				    amdgv_idx_to_str(event->idx_vf));
			goto out;
		}

		amdgv_sched_stop(adapt, event->idx_vf);

		if (!amdgv_sched_is_state_ok(adapt, event->idx_vf))
			amdgv_sched_reset_vf_auto(adapt);

		if (is_active_vf(event->idx_vf) &&
			amdgv_sched_context_switch_to_vf_saved(adapt, event->idx_vf,
						     AMDGV_SCHED_BLOCK_ALL) != 0)
			amdgv_sched_reset_vf_auto(adapt);

		if (is_active_vf(event->idx_vf))
			amdgv_sched_reset_vf(adapt, event->idx_vf, AMDGV_SCHED_BLOCK_ALL);

		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_FORCED_RESET_VF,
				  "Reset %s forced (FORCE_RESET_VF) on %s",
				  amdgv_idx_to_str(event->idx_vf),
				  amdgv_sched_block_to_name(event->sched_block));

		/* if the VF is active VF, move VF out of active list and
		   move to available state. */
		if (is_active_vf(event->idx_vf)) {
			amdgv_sched_remove_vf(adapt, event->idx_vf);
		}

		set_to_avail_vf(event->idx_vf);
		break;

	case AMDGV_EVENT_SCHED_RESET_VF:
		/* SCHED_RESET_VF and HW_SCHED_RESET_VF are sent from
		   world switch, the request to reset VF must be an
		   active VF. If the vf become avail, that means the vf
		   has been reset before. */
		if (is_avail_vf(event->idx_vf))
			break;

		amdgv_sched_stop(adapt, event->idx_vf);

		if (!amdgv_sched_is_state_ok(adapt, event->idx_vf))
			amdgv_sched_reset_vf_auto(adapt);

		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ERROR_RESET_VF,
				  "Reset %s initiated by scheduler (SCHED_RESET_VF) on %s",
				  amdgv_idx_to_str(event->idx_vf),
				  amdgv_sched_block_to_name(event->sched_block));

		break;

	case AMDGV_EVENT_HW_SCHED_RESET_VF:
		/* skip if vf is not active */
		if (!((adapt->flags & AMDGV_FLAG_USE_PF) && event->idx_vf == AMDGV_PF_IDX) && !(is_active_vf(event->idx_vf))) {
			AMDGV_DEBUG("Ignore HW_SCHED_RESET_VF for non-active VFs: %s\n",
					amdgv_idx_to_str(event->idx_vf));
			goto out;
		}

		/* Corresponding sched_block is already abnormal when processing this event.
		 * Mark corresponding world switch stopped and abnormal to avoid redundant
		 * timeout when attempting to stop scheduler */
		for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, event->idx_vf)) {
			world_switch = &adapt->sched.world_switch[world_switch_id];
			if (world_switch && world_switch->switch_running && world_switch->sched_block == event->sched_block) {
				world_switch->curr_vf_state = AMDGV_VF_CONTEXT_ABNORMAL;
				amdgv_sched_dump_gpu_state(adapt);
			}
		}

		amdgv_sched_stop(adapt, event->idx_vf);

		amdgv_irqmgr_dump_temperature(adapt);
		amdgv_sched_reset_vf_auto(adapt);

		amdgv_notify_shim(
			adapt->dev, AMDGV_NOTIFICATION_ERROR_RESET_VF,
			"Reset %s initiated by HW scheduler (HW_SCHED_RESET_VF) on %s",
			amdgv_idx_to_str(event->idx_vf),
			amdgv_sched_block_to_name(event->sched_block));

		break;

	case AMDGV_EVENT_SCHED_SUSPEND_VF:
		/* if the VF is active VF, move VF out of active list */
		if (is_active_vf(event->idx_vf)) {
			set_to_suspend_vf(event->idx_vf);
			amdgv_sched_stop(adapt, event->idx_vf);
			amdgv_sched_remove_vf(adapt, event->idx_vf);
		} else {
			AMDGV_WARN("Cannot suspend non-active %s\n",
				   amdgv_idx_to_str(event->idx_vf));
		}

		break;

	case AMDGV_EVENT_SCHED_RESUME_VF:
		if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) && (adapt->live_update_state == AMDGV_LIVE_UPDATE_SAVE)) {
			AMDGV_WARN("GPUV live update save, skip resume %s\n",
				   amdgv_idx_to_str(event->idx_vf));
		} else if (is_suspend_vf(event->idx_vf)) {
			amdgv_sched_stop(adapt, event->idx_vf);
			amdgv_sched_add_vf(adapt, event->idx_vf);
			set_to_active_vf(event->idx_vf);

			if (AMDGV_PF_IDX != event->idx_vf) {
				if (amdgv_vfmgr_update_pf2vf_message(adapt, event->idx_vf))
					AMDGV_WARN("update pf2vf message failed\n");
			}
		} else {
			AMDGV_WARN("resuming non-suspended %s\n",
				   amdgv_idx_to_str(event->idx_vf));
		}
		break;

	case AMDGV_EVENT_SCHED_REMOVE_VF:
		/* if the VF is active VF, move VF out of world switching. */
		if (is_active_vf(event->idx_vf)) {
			amdgv_sched_remove_vf(adapt, event->idx_vf);
		}

		set_to_unavail_vf(event->idx_vf);
		break;

	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU:
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL:
		amdgv_sched_stop_all(adapt);

		if (!amdgv_sched_gpu_reset_wrap(adapt,
						event->id == AMDGV_EVENT_SCHED_FORCE_RESET_GPU ?
						1 : 0))
			AMDGV_INFO("finish %s reset.\n", amdgv_idx_to_str(event->idx_vf));

		/* xgmi hive case need to sync flag unsets so unset is done in amdgv_sched_gpu_chain_reset */
		if (!(adapt->xgmi.phy_nodes_num > 1) && oss_atomic_read(adapt->in_ecc_recovery))
			oss_atomic_set(adapt->in_ecc_recovery, 0);

		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_FORCED_WHOLE_GPU_RESET,
				  "Whole GPU reset forced by %s on %s",
				  amdgv_idx_to_str(event->idx_vf),
				  amdgv_sched_block_to_name(event->sched_block));
		break;

	case AMDGV_EVENT_SCHED_STOP_VF:
		/* if the VF is active VF, move VF out of active list */
		if (is_active_vf(event->idx_vf)) {
			amdgv_sched_stop(adapt, event->idx_vf);

			if (!amdgv_sched_is_state_ok(adapt, event->idx_vf)) {
				AMDGV_DEBUG("Hang detected while stopping %s\n",
					    amdgv_idx_to_str(event->idx_vf));
				amdgv_sched_reset_vf_auto(adapt);
			}

			amdgv_sched_remove_vf(adapt, event->idx_vf);

			amdgv_sched_shutdown_vf(adapt, event->idx_vf);

		}
		/* if the VF is suspended VF, shutdown GPU directly*/
		else if (is_suspend_vf(event->idx_vf)) {
			amdgv_sched_shutdown_vf(adapt, event->idx_vf);
		}
		adapt->array_vf[event->idx_vf].unshutdown = false;
		set_to_avail_vf(event->idx_vf);
		break;

	case AMDGV_EVENT_HANDLE_CRASH:
		amdgv_sched_stop(adapt, event->idx_vf);
		if (!amdgv_sched_is_state_ok(adapt, event->idx_vf)) {
			AMDGV_DEBUG("Hang detected while stopping %s\n",
				    amdgv_idx_to_str(event->idx_vf));
			amdgv_sched_reset_vf_auto(adapt);
		}

		if (amdgv_sched_handle_orphan_vf(adapt, event->idx_vf)) {
			AMDGV_WARN("Failed to reset orphan VF\n");
			amdgv_sched_reset_vf_auto(adapt);
			return AMDGV_FAILURE;
		}
		adapt->array_vf[event->idx_vf].unshutdown = false;
		break;

	case AMDGV_EVENT_SCHED_RAS_UMC:
		amdgv_irqmgr_dump_temperature(adapt);
		amdgv_ecc_check_for_errors(adapt, event);
		break;

	case AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION:
			amdgv_sched_handle_ras_poison_consumption(adapt, event);
			break;
	case AMDGV_EVENT_SCHED_RAS_POISON_CREATION:
			amdgv_sched_handle_ras_poison_creation(adapt, event);
			break;

	case AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC:
		if (amdgv_psp_live_update_fw(adapt, AMDGV_FIRMWARE_ID__DFC_FW))
			ret = AMDGV_FAILURE;
		break;

	case AMDGV_EVENT_SCHED_GPUMON:
		if (adapt->reset.in_xgmi_chain_reset) {
			/* push back smi event if in chain reset */
			amdgv_sched_push_back_event(adapt, event);
			ret = AMDGV_EVENT_STOP_AND_KEEP;
		} else {
			amdgv_gpumon_handle_sched_event(adapt, event);
		}
		break;

	case AMDGV_EVENT_SCHED_SET_VF_ACCESS:
		ret = amdgv_gpuiov_set_vf_access(adapt, event->idx_vf,
						 event->data.vf_access_data.vf_access_select,
						 event->data.vf_access_data.enable);
		break;

	case AMDGV_EVENT_SCHED_PSP_VF_GATE:
		amdgv_sched_stop_all(adapt);
		for (vf_id = 0; vf_id < adapt->num_vf; vf_id++) {
			if (event->data.psp_vf_gate_data.vf_select & (1 << vf_id)) {
				ret = amdgv_sched_psp_set_mb_int(
					adapt, vf_id, !event->data.psp_vf_gate_data.enable);
			}
		}
		break;

	case AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION:
		ret = amdgv_mmsch_read_all_output(adapt, event->idx_vf, event->sched_block);
		break;

	case AMDGV_EVENT_SCHED_PSP_VF_CMD_RELAY:
		if (amdgv_psp_vf_cmd_relay(adapt, event->idx_vf))
			ret = AMDGV_FAILURE;
		break;

	case AMDGV_EVENT_COLLECT_DIAG_DATA:
		*event->data.diag_data.result =
			amdgv_diag_data_collect(adapt, adapt->bdf,
							event->data.diag_data.data,
							event->data.diag_data.size);
		break;

	case AMDGV_EVENT_EXIT_POWER_SAVING:
		*event->data.gpumon_data.result = amdgv_sched_exit_power_saving(adapt);
		if (*event->data.gpumon_data.result) {
			AMDGV_ERROR("Failed to exit power saving\n");
		}
		break;

	case AMDGV_EVENT_ENTER_POWER_SAVING:
		*event->data.gpumon_data.result = amdgv_sched_enter_power_saving(adapt);
		if (*event->data.gpumon_data.result) {
			AMDGV_ERROR("Failed to enter power saving\n");
		} else
			ret = AMDGV_EVENT_STOP_AND_RELEASE;
		break;
	case AMDGV_EVENT_SCHED_RAS_FED:
		amdgv_sched_handle_fed(adapt, event);
		break;
	case AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY:
		amdgv_xgmi_update_topology(adapt);
		break;
	case AMDGV_EVENT_SCHED_GET_TOPOLOGY:
		amdgv_xgmi_get_topology_info(adapt);
		break;
	case AMDGV_EVENT_CUR_VF_CTX_EMPTY:
		amdgv_sched_world_switch_signal_vf_idle(adapt);
		break;

	case AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS:
		ret = amdgv_mca_get_new_banks(adapt, event->data.mca_bank.error_type);
		break;
	case AMDGV_EVENT_SCHED_RMA:
		ret = amdgv_sched_event_handle_rma(adapt);
		break;
	case AMDGV_EVENT_SCHED_VF_REQ_RAS_ERROR_COUNT:
		ret = amdgv_sched_handle_vf_req_ras_error_count(adapt, event->idx_vf);
		break;
	case AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP:
		ret = amdgv_sched_handle_vf_req_cper_dump(adapt, event->idx_vf,
							  event->data.cper_vf.rptr);
		break;

	case AMDGV_EVENT_REQ_GPU_DEBUG:
		if (adapt->debug.in_live_debugging) {
			AMDGV_WARN("already have one vf in live debug mode\n");
			goto out;
		}
		// stop ws and switch to current vf
		amdgv_sched_stop_all(adapt);
		if (amdgv_sched_context_switch_to_vf(adapt, event->idx_vf, AMDGV_SCHED_BLOCK_ALL) == 0) {
			// enable vf full access
			amdgv_sched_toggle_full_access_for_debug(adapt, event->idx_vf, true);
			adapt->debug.in_live_debugging = true;
			adapt->lock_world_switch = true;
		}
		break;

	case AMDGV_EVENT_REL_GPU_DEBUG:
		if (!adapt->debug.in_live_debugging) {
			AMDGV_WARN("vf is NOT in live debug mode\n");
			goto out;
		}
		// disable vf full access
		amdgv_sched_toggle_full_access_for_debug(adapt, event->idx_vf, false);
		adapt->debug.in_live_debugging = false;
		adapt->lock_world_switch = false;
		break;

	default:
		break;
	}

out:
	return ret;
}

/*
   Events are no longer processed during driver fini.
   Need to signal user clients and return failure to all outstanding events
*/
static int amdgv_sched_process_event_fini(struct amdgv_adapter *adapt)
{
	struct amdgv_list_head events;
	struct amdgv_sched_event *event;

	amdgv_sched_event_queue_pop_event_list(adapt, &events);
	amdgv_sched_event_arrange_event_list(adapt, &events);

	while ((event = amdgv_sched_pick_up_next_event(adapt)) != NULL) {
		AMDGV_INFO("Skipped %s request from %s for %s due to driver unload\n",
				amdgv_event_name(event->id), amdgv_idx_to_str(event->idx_vf),
				amdgv_sched_block_to_name(event->sched_block));
		if (event->signal != OSS_INVALID_HANDLE) {
			oss_signal_event_with_flag(event->signal, EVENT_FLAGS_SKIPPED);
		}
		amdgv_sched_free_event(adapt, event);
	}

	return 0;
}

static int amdgv_sched_process_event(struct amdgv_adapter *adapt)
{
	int stop;
	int i;
	struct amdgv_sched_event *event;
	struct amdgv_vf_device *vf_device;

	do {
		/* Pick up event from event list according to
		 * event list priority.
		 * Event List priority:
		 * 1. SCHED_SUSPEND_VF/SCHED_RESUME_VF/REMOVE_VF
		 * 2. REL_GPU_INIT/REL_GPU_FINI
		 * 3. SCHED_RESET_VF/SCHED_FORCE_RESET_VF
		 * 4. REQ_GPU_INIT/REQ_GPU_FINI/REQ_GPU_RESET
		 */
		while ((event = amdgv_sched_pick_up_next_event(adapt)) != NULL) {
			/* Add the event to diagnosis data trace log */
			AMDGV_DIAG_DATA_TRACE_LOG_EVENT(event->idx_vf, event->sched_block,
						   event->id);

			if (adapt->bp_mode == AMDGV_BP_MODE_1 && AMDGV_PF_IDX == event->idx_vf &&
				adapt->array_vf[AMDGV_PF_IDX].vf_status == AMDGV_VF_STATUS_END_INIT) {
				AMDGV_ERROR("BP_MODE1 is enabled skip all PF events and fake signal this event\n");
				if (event->signal != OSS_INVALID_HANDLE)
					oss_signal_event_with_flag(event->signal, EVENT_FLAGS_SKIPPED);
				amdgv_sched_free_event(adapt, event);
				continue;
			}

			if (event->status == AMDGV_EVENT_STATUS_FINISHED) {
				AMDGV_DEBUG("Skip stale event %s\n", amdgv_event_name(event->id));
				if (event->signal != OSS_INVALID_HANDLE)
					oss_signal_event_with_flag(event->signal, EVENT_FLAGS_SKIPPED);
				amdgv_sched_free_event(adapt, event);
				continue;
			}

			/* all events in queue are clean when they are inited (no unrecov_err)
			 * but after they pushed to queue it is possible that we had a unrecov_err
			 * thus we need to drop those events that not handled after unrecov_err
			 * happened
			 */
			if (amdgv_sched_is_unrecov_err(adapt) && (event->idx_vf != AMDGV_PF_IDX)) {
				AMDGV_WARN("Unrecov Err happened before handling %s from %s. Dropping this event\n",
						amdgv_event_name(event->id), amdgv_idx_to_str(event->idx_vf));

				if (event->signal != OSS_INVALID_HANDLE)
					oss_signal_event_with_flag(event->signal, EVENT_FLAGS_SKIPPED);

				amdgv_sched_free_event(adapt, event);
				continue;
			}

			if (amdgv_sched_event_print_log_in_info(event->id))
				AMDGV_INFO("process %s request from %s for %s\n",
					   amdgv_event_name(event->id),
					   amdgv_idx_to_str(event->idx_vf),
					   amdgv_sched_block_to_name(event->sched_block));
			else
				AMDGV_DEBUG("process %s request from %s for %s\n",
					    amdgv_event_name(event->id),
					    amdgv_idx_to_str(event->idx_vf),
					    amdgv_sched_block_to_name(event->sched_block));

			vf_device = &adapt->array_vf[event->idx_vf];

			/* We expect REQ_GPU_INIT_DATA comes first
			 * then REQ_GPU_INIT followed.
			 * If any other event break the squence
			 * we should clear the flag.
			 */
			if (vf_device->gpu_init_data_ready == true &&
			    event->id != AMDGV_EVENT_REQ_GPU_INIT)
				vf_device->gpu_init_data_ready = false;

			/**
			 * If any VF sharing common engine with current VF is
			 * in full access : handle as full access
			 */
			if (is_any_share_engine_vf_in_full_access(event->idx_vf)) {
				stop = handle_event_in_full_access(adapt, event);
			} else {
				stop = handle_event_in_non_full_access(adapt, event);
			}

			/* Stop processing the remaining events, wait for new events */
			if (stop == AMDGV_EVENT_STOP_AND_RELEASE) {
				/* current event is completed */
				if (event->signal != OSS_INVALID_HANDLE)
					oss_signal_event(event->signal);

				amdgv_sched_free_event(adapt, event);

				break;
			} else if (stop == AMDGV_EVENT_STOP_AND_KEEP) {
				/* current event is pushed back */
				break;
			}

			/* if signal is a valid handle, it means there
			 * may be a user client to wait the event until
			 * finished.
			 */
			if (event->signal != OSS_INVALID_HANDLE)
				oss_signal_event(event->signal);

			amdgv_sched_free_event(adapt, event);
		}

	} while (!adapt->lock_world_switch && !amdgv_sched_event_queue_empty(adapt));

	/* Ensure world switch is re-activated after event processing.
	 * unless the scheduler is locked or we are in exclusive mode
	 */

	if (adapt->status == AMDGV_STATUS_HW_INIT && !adapt->lock_world_switch &&
	    !is_in_full_access()) {
		/* for partition full access, re-active WS for each partition */
		if (adapt->sched.enable_per_partition_full_access) {
			for (i = 0; i < adapt->num_vf; i++) {
				if (!is_any_share_engine_vf_in_full_access(i))
					amdgv_sched_start(adapt, i);
			}
		} else {
			amdgv_sched_start_all(adapt);
		}
	}

	return 0;
}

static enum amdgv_sched_full_access_status amdgv_sched_partition_full_access_check_and_process(struct amdgv_adapter *adapt, int64_t *time_remain)
{
	int i;
	int64_t left_time;
	int64_t left_time_min;
	bool timed_out = false;
	enum amdgv_sched_full_access_status status;

	/* assign int64 manximun */
	left_time_min = 0x7fffffffffffffffL;

	/* partition status:
	 * all out of full access and no timeout
	 * 		return SCHED_FULL_ACCESS_NOT_ENTERED
	 * 		- no reschedule, wait for signal without timeout
	 * any full access timeout
	 * 		return SCHED_FULL_ACCESS_TIMED_OUT
	 * 		- reschedule immediately
	 * any in full access and no timeout
	 * 		return SCHED_FULL_ACCESS_ON_GOING
	 * 		*time_remain set to min left time
	 * 		- reschedule after time_remain
	 */
	for (i = 0; i < adapt->num_vf; i++) {
		status = amdgv_sched_full_access_left_time(adapt, i, &left_time);
		if (status == SCHED_FULL_ACCESS_ON_GOING) {
			/* in full access and has left time */
			if (left_time < left_time_min) {
				left_time_min = left_time;
			}
		} else if (status == SCHED_FULL_ACCESS_TIMED_OUT) {
			/* full access mode timed out */
			amdgv_sched_exit_full_access_timeout(adapt, i);
			timed_out = true;
		}
	}

	if (timed_out)
		return SCHED_FULL_ACCESS_TIMED_OUT;

	if (left_time_min == 0x7fffffffffffffffL)
		return SCHED_FULL_ACCESS_NOT_ENTERED;

	*time_remain = left_time_min;
	return SCHED_FULL_ACCESS_ON_GOING;
}


static enum amdgv_sched_full_access_status amdgv_sched_full_access_check_and_process(struct amdgv_adapter *adapt, int64_t *time_remain)
{
	enum amdgv_sched_full_access_status status = SCHED_FULL_ACCESS_NOT_ENTERED;

	/* If gim is suspended during full access, skip the timeout check*/
	if (adapt->sched.skip_full_access_timeout_check)
		return SCHED_FULL_ACCESS_NOT_ENTERED;

	if (adapt->sched.enable_per_partition_full_access) {
		status = amdgv_sched_partition_full_access_check_and_process(adapt, time_remain);
	} else {
		status = amdgv_sched_full_access_left_time(adapt, AMDGV_INVALID_IDX_VF, time_remain);

		if (status == SCHED_FULL_ACCESS_TIMED_OUT) {
			/* full access timed out */
			amdgv_sched_exit_full_access_timeout(adapt, AMDGV_INVALID_IDX_VF);
		}
	}

	return status;
}


static int amdgv_sched_event_process_thread(void *context)
{
	int ret, state;
	int64_t timeout = 0;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	enum amdgv_sched_full_access_status full_access_status;

	/* Thread gets created in SW init, before any VF are active or
	 * the HW is available. Block here till thread received wake-up
	 * in HW INIT
	 */
	do {
		ret = oss_wait_event(adapt->sched.event, 0);
	} while (ret == OSS_EVENT_STATE_INTERRUPTED);

	oss_mutex_lock(adapt->sched.event_thread_lock);

	while (!oss_thread_should_stop(adapt->sched.event_thread)) {
		/* process received events from event queue */
		amdgv_sched_process_event(adapt);


		/* timeout value is only meaningful if SCHED_FULL_ACCESS_ON_GOING*/
		timeout = 0;
		full_access_status = amdgv_sched_full_access_check_and_process(adapt, &timeout);

		/*
		 * - reschedule immediately if out of full access
		 * - and at least 1 timeout:
		 * 		SCHED_FULL_ACCESS_TIMED_OUT
		 * - wait for mininum left time if there is full accesss time remaining:
		 * 		SCHED_FULL_ACCESS_ON_GOING
		 * - idle wait without timeout in other cases:
		 * 		SCHED_FULL_ACCESS_NOT_ENTERED
		 */
		if (full_access_status == SCHED_FULL_ACCESS_TIMED_OUT)
			continue;
		else if (full_access_status == SCHED_FULL_ACCESS_ON_GOING)
			adapt->event_thread_status = AMDGV_EVENT_THREAD_WAITING;
		else
			/* SCHED_FULL_ACCESS_NOT_ENTERED */
			adapt->event_thread_status = AMDGV_EVENT_THREAD_IDLE;

		oss_mutex_unlock(adapt->sched.event_thread_lock);

		/* wait for new events or up to the exclusive mode
		 * timeout
		 */
		state = oss_wait_event(adapt->sched.event,
			 full_access_status == SCHED_FULL_ACCESS_ON_GOING ? timeout : 0);
		adapt->event_thread_status = AMDGV_EVENT_THREAD_BUSY;
		oss_mutex_lock(adapt->sched.event_thread_lock);

		/* We timed out for exclusive mode before we got a state
		 * to get out of it. Process time-out.
		 * Keep full access time check here to align with past behaviour
		 */
		if (state == OSS_EVENT_STATE_TIMEOUT)
			amdgv_sched_full_access_check_and_process(adapt, &timeout);

	}

	oss_mutex_unlock(adapt->sched.event_thread_lock);
	AMDGV_INFO("event process thread exiting!\n");

	adapt->sched.event_thread = OSS_INVALID_HANDLE;
	return 0;
}

#ifdef WS_RECORD
static int amdgv_sched_record_process_thread(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	bool enabled = false;

	while (!oss_thread_should_stop(adapt->sched.record_thread)) {
		if (adapt->flags & AMDGV_FLAG_WS_RECORD) {
			enabled = true;
			amdgv_sched_record_queue_flush(adapt);
			if (adapt->flags & AMDGV_FLAG_DEBUG_DUMP_ENABLE)
				amdgv_sched_debug_dump_data_flush(adapt);
		} else {
			if (enabled) {
				amdgv_sched_record_queue_fini(adapt);
				amdgv_sched_record_queue_init(adapt);
				enabled = false;
				adapt->gpuiov.auto_ws_record_rptr = 0;
				AMDGV_INFO("suspend ws record queue flush\n");
			}
		}
		oss_msleep(1000);
	}

	AMDGV_INFO("record process thread exiting!\n");
	adapt->sched.record_thread = OSS_INVALID_HANDLE;
	return 0;
}
#endif

int amdgv_sched_queue_event_process_suspend(struct amdgv_adapter *adapt)
{
	if (adapt->sched.event_thread_lock != OSS_INVALID_HANDLE)
		oss_mutex_lock(adapt->sched.event_thread_lock);
	return 0;
}

void amdgv_sched_queue_event_process_resume(struct amdgv_adapter *adapt)
{
	if (adapt->sched.event_thread_lock != OSS_INVALID_HANDLE)
		oss_mutex_unlock(adapt->sched.event_thread_lock);
}

int amdgv_sched_queue_event(struct amdgv_adapter *adapt, uint32_t idx_vf,
			    enum amdgv_sched_event_id event_id,
			    enum amdgv_sched_block sched_block)
{
	union amdgv_sched_event_data data;
	oss_memset(&data, 0, sizeof(union amdgv_sched_event_data));
	return amdgv_sched_queue_event_ex(adapt, idx_vf, event_id, sched_block, data);
}

int amdgv_sched_queue_event_no_signal(struct amdgv_adapter *adapt, uint32_t idx_vf,
			    enum amdgv_sched_event_id event_id,
			    enum amdgv_sched_block sched_block)
{
	union amdgv_sched_event_data data;
	oss_memset(&data, 0, sizeof(union amdgv_sched_event_data));
	return amdgv_sched_queue_event_ex_no_signal(adapt, idx_vf, event_id, sched_block, data);
}

int amdgv_sched_queue_event_and_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
				     enum amdgv_sched_event_id event_id,
				     enum amdgv_sched_block sched_block)
{
	union amdgv_sched_event_data data;
	oss_memset(&data, 0, sizeof(union amdgv_sched_event_data));
	return amdgv_sched_queue_event_and_wait_ex(adapt, idx_vf, event_id, sched_block, data);
}

static bool amdgv_sched_allow_queue_in_unrecov_err(struct amdgv_adapter *adapt,
						   enum amdgv_sched_event_id event_id,
						   union amdgv_sched_event_data data)
{
	int i = 0;

	/* allow chain reset from other adapt */
	if (event_id == AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL)
		return true;

	/* Allow a limited number of GPUMON events to be scheduled even in bad GPU state. */
	if (event_id == AMDGV_EVENT_SCHED_GPUMON) {
		for (i = 0; i < gpumon_unrecov_err_whitelist_len; i++)
			if (gpumon_unrecov_err_whitelist[i] == data.gpumon_data.type)
				return true;
	}

	return false;
}

static int amdgv_sched_sanitize_queue_event(struct amdgv_adapter *adapt, uint32_t idx_vf,
					    enum amdgv_sched_event_id event_id,
					    union amdgv_sched_event_data data)
{
	int ret = 0;

	if (amdgv_sched_is_unrecov_err(adapt) &&
	    !amdgv_sched_allow_queue_in_unrecov_err(adapt, event_id, data))
		return AMDGV_FAILURE;

	/* VF Guard */
	if (AMDGV_EVENT_REQ_GPU_INIT <= event_id &&
	    event_id <= AMDGV_EVENT_REL_GPU_DEBUG) {
		if (event_id == AMDGV_EVENT_REQ_GPU_INIT ||
		    event_id == AMDGV_EVENT_REQ_GPU_RESET ||
		    event_id == AMDGV_EVENT_REQ_GPU_DEBUG) {
			ret = amdgv_guard_add_active_event(adapt, idx_vf,
							   AMDGV_GUARD_EVENT_EXCLUSIVE_MOD);
			if (ret == AMDGV_EVENT_OVERFLOW) {
				amdgv_put_error(idx_vf, AMDGV_ERROR_SCHED_IGNORE_EVENT,
						event_id);
				return AMDGV_FAILURE;
			}
		} else if (event_id == AMDGV_EVENT_REQ_GPU_FINI) {
			ret = amdgv_guard_add_active_event(adapt, idx_vf,
							   AMDGV_GUARD_EVENT_EXCLUSIVE_MOD);
			if (!is_active_vf(idx_vf) && (ret == AMDGV_EVENT_OVERFLOW)) {
				amdgv_put_error(idx_vf, AMDGV_ERROR_SCHED_IGNORE_EVENT,
						event_id);
				return AMDGV_FAILURE;
			}
		}

		if (amdgv_guard_is_event_full(adapt, idx_vf,
					      AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT)) {
			amdgv_put_error(idx_vf, AMDGV_ERROR_SCHED_IGNORE_EVENT, event_id);
			return AMDGV_FAILURE;
		}

		if (amdgv_guard_is_event_full(adapt, idx_vf, AMDGV_GUARD_EVENT_FLR) ==
			    AMDGV_GUARD_EVENT_OVERFLOW &&
		    (event_id == AMDGV_EVENT_REQ_GPU_RESET ||
		     event_id == AMDGV_EVENT_REQ_GPU_INIT)) {
			amdgv_put_error(idx_vf, AMDGV_ERROR_GUARD_EVENT_OVERFLOW, event_id);
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

int amdgv_sched_queue_event_ex(struct amdgv_adapter *adapt, uint32_t idx_vf,
			       enum amdgv_sched_event_id event_id,
			       enum amdgv_sched_block sched_block,
			       union amdgv_sched_event_data data)
{
	int ret;

	ret = amdgv_sched_sanitize_queue_event(adapt, idx_vf, event_id, data);
	if (ret)
		return ret;

	/* push event to event queue */
	ret = amdgv_sched_event_queue_push_ex(adapt, idx_vf, event_id, sched_block,
					       OSS_INVALID_HANDLE, data, true);

	return ret;
}

int amdgv_sched_queue_event_ex_no_signal(struct amdgv_adapter *adapt, uint32_t idx_vf,
			       enum amdgv_sched_event_id event_id,
			       enum amdgv_sched_block sched_block,
			       union amdgv_sched_event_data data)
{
	int ret;

	ret = amdgv_sched_sanitize_queue_event(adapt, idx_vf, event_id, data);
	if (ret)
		return ret;

	/* push event to event queue */
	ret = amdgv_sched_event_queue_push_ex(adapt, idx_vf, event_id, sched_block,
					       OSS_INVALID_HANDLE, data, false);

	return ret;
}

int amdgv_sched_queue_event_and_wait_ex(struct amdgv_adapter *adapt, uint32_t idx_vf,
					enum amdgv_sched_event_id event_id,
					enum amdgv_sched_block sched_block,
					union amdgv_sched_event_data data)
{
	event_t signal;
	int ret;

	ret = amdgv_sched_sanitize_queue_event(adapt, idx_vf, event_id, data);
	if (ret)
		return ret;

	/* forbid recursive queue_and_wait in event_thread, which will lead to deadlock */
	if (oss_is_current_running_thread(adapt->sched.event_thread)) {
		AMDGV_ERROR("Recursive push_event_and_wait in event_thread detected!\n");
		return AMDGV_FAILURE;
	}

	signal = oss_event_init();
	if (signal == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_EVENT_FAIL, 0);
		return AMDGV_FAILURE;
	}

	/* push event to event queue */
	if (amdgv_sched_event_queue_push_ex(adapt, idx_vf, event_id, sched_block, signal,
					    data, true)) {
		oss_event_fini(signal);
		return AMDGV_FAILURE;
	}

	/* wait for event to be completed processing */
	do {
		ret = oss_wait_event(signal, 0);
	} while (ret == OSS_EVENT_STATE_INTERRUPTED);

	oss_event_fini(signal);

	if (ret != OSS_EVENT_STATE_WAKE_UP)
		return AMDGV_FAILURE;

	return 0;
}

int amdgv_sched_event_queue_process_init(struct amdgv_adapter *adapt)
{
	int i;
	thread_t event_thread;

	for (i = 0; i < AMDGV_SCHED_EVENT_LIST_MAX; i++)
		AMDGV_INIT_LIST_HEAD(&adapt->sched.event_list[i]);

	adapt->sched.event = oss_event_init();
	if (adapt->sched.event == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_EVENT_FAIL, 0);
		return AMDGV_FAILURE;
	}

	adapt->sched.event_thread_lock = oss_mutex_init();
	if (adapt->sched.event_thread_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		oss_event_fini(adapt->sched.event);
		adapt->sched.event = OSS_INVALID_HANDLE;
		return AMDGV_FAILURE;
	}

	/* init event queue */
	if (amdgv_sched_event_queue_init(adapt)) {
		oss_event_fini(adapt->sched.event);
		oss_mutex_fini(adapt->sched.event_thread_lock);
		adapt->sched.event = OSS_INVALID_HANDLE;
		adapt->sched.event_thread_lock = OSS_INVALID_HANDLE;
		return AMDGV_FAILURE;
	}

	adapt->sched.in_full_access = false;
	adapt->sched.idx_vf_full_access = AMDGV_INVALID_IDX_VF;
	adapt->sched.used_time_full_access = 0;

	if (adapt->opt.allow_time_full_access == 0)
		adapt->sched.allow_time_full_access =
			(adapt->sched.num_vf_per_gfx_sched == 1) ?
				AMDGV_SCHED_EXCLUSIVE_TIMEOUT_MS_1VF * 1000 :
				AMDGV_SCHED_EXCLUSIVE_TIMEOUT_MS_COMMON * 1000;
	else
		adapt->sched.allow_time_full_access = adapt->opt.allow_time_full_access * 1000;

	if (adapt->flags & AMDGV_FLAG_EMU_MODE)
		adapt->sched.allow_time_full_access =
			adapt->sched.allow_time_full_access * 1000;

	AMDGV_INFO("allowed time for full access is %dms\n",
		   adapt->sched.allow_time_full_access / 1000);

	/* create event process thread */
	event_thread = oss_create_thread(amdgv_sched_event_process_thread, (void *)adapt,
					 "sched_event_thread");
	if (event_thread == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_THREAD_FAIL, 0);
		goto failed;
	}

	adapt->sched.event_thread = event_thread;

	return 0;

failed:
	oss_event_fini(adapt->sched.event);
	oss_mutex_fini(adapt->sched.event_thread_lock);
	adapt->sched.event = OSS_INVALID_HANDLE;
	adapt->sched.event_thread_lock = OSS_INVALID_HANDLE;
	amdgv_sched_event_queue_fini(adapt);
	return AMDGV_FAILURE;
}

#ifdef WS_RECORD
int amdgv_sched_record_queue_process_init(struct amdgv_adapter *adapt)
{
	thread_t record_thread;

	if (amdgv_sched_record_queue_init(adapt))
		return AMDGV_FAILURE;

	record_thread = oss_create_thread(amdgv_sched_record_process_thread, (void *)adapt,
					  "record_thread");
	if (record_thread == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_THREAD_FAIL, 0);
		goto failed;
	}

	adapt->sched.record_thread = record_thread;

	return 0;

failed:
	amdgv_sched_record_queue_fini(adapt);
	return AMDGV_FAILURE;
}

void amdgv_sched_record_queue_process_fini(struct amdgv_adapter *adapt)
{
	if (adapt->sched.record_thread != OSS_INVALID_HANDLE)
		oss_close_thread(adapt->sched.record_thread);
	adapt->sched.record_thread = OSS_INVALID_HANDLE;

	amdgv_sched_record_queue_fini(adapt);
}
#endif

void amdgv_sched_event_queue_process_fini(struct amdgv_adapter *adapt)
{
	/* signal event forever permanently makes any subsquent
	 * calls to wait_event() return immediatelly.
	 * This prevents a deadlock in closing the thread if the
	 * signal races the kernel exit check
	 */
	if (adapt->sched.event != OSS_INVALID_HANDLE)
		oss_signal_event_forever(adapt->sched.event);

	/* wait for thread exited and fini the thread */
	if (adapt->sched.event_thread != OSS_INVALID_HANDLE)
		oss_close_thread(adapt->sched.event_thread);
	adapt->sched.event_thread = OSS_INVALID_HANDLE;

	amdgv_sched_process_event_fini(adapt);

	amdgv_sched_event_queue_fini(adapt);

	if (adapt->sched.event != OSS_INVALID_HANDLE) {
		oss_event_fini(adapt->sched.event);
		adapt->sched.event = OSS_INVALID_HANDLE;
	}

	if (adapt->sched.event_thread_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->sched.event_thread_lock);
		adapt->sched.event_thread_lock = OSS_INVALID_HANDLE;
	}
}

int amdgv_sched_export_unprocessed_event_size(struct amdgv_adapter *adapt,
					      uint32_t *unprocessed_event_count)
{
	uint32_t i = 0;

	/* Push all the events from sched.event_queue to sched.event_list */
	struct amdgv_list_head events;
	struct amdgv_list_head *event_list = NULL;

	if (unprocessed_event_count == NULL) {
		AMDGV_INFO("unprocessed_event_count is NULL!.\n");
		return AMDGV_FAILURE;
	}

	*unprocessed_event_count = 0;

	if (!amdgv_sched_event_queue_empty(adapt)) {
		/* pop out all events from event queue */
		amdgv_sched_event_queue_pop_event_list(adapt, &events);

		/* Dispatch popped out events to various event lists.
		 * According to the arrangement rules,
		 * remove duplicated VF or event.
		 */
		amdgv_sched_event_arrange_event_list(adapt, &events);
	}

	/* Get event count left in event_list */
	for (i = 0; i < AMDGV_SCHED_EVENT_LIST_MAX; i++) {
		event_list = &adapt->sched.event_list[i];
		while (event_list->next != &adapt->sched.event_list[i]) {
			event_list = event_list->next;
			*unprocessed_event_count += 1;
		}
	}

	AMDGV_INFO("Unprocessed event count is %d.\n", *unprocessed_event_count);

	return 0;
}

enum amdgv_live_info_status amdgv_sched_export_unprocessed_event(struct amdgv_adapter *adapt,
					 struct amdgv_live_info_unprocessed_event *event_info)
{
	struct amdgv_sched_event *event;
	uint32_t unprocessed_event_size = 0;
	uint32_t i = 0;

	if (event_info == NULL) {
		AMDGV_INFO("event_info is NULL!\n");
		return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
	}

	amdgv_sched_export_unprocessed_event_size(adapt, &unprocessed_event_size);

	/* Export the unprocessed event from sched.event_list, sched.event_queue has been
	 * pushed into the list when exporting the unprocessed event size
	 */
	while ((event = amdgv_sched_pick_up_next_event(adapt)) != NULL) {
		AMDGV_INFO("save %s request from %s for %s\n", amdgv_event_name(event->id),
			   amdgv_idx_to_str(event->idx_vf),
			   amdgv_sched_block_to_name(event->sched_block));
		if (event->signal != OSS_INVALID_HANDLE)
			oss_signal_event(event->signal);

		if (i >= AMDGV_EVENT_QUEUE_ENTRY_NUM) {
			AMDGV_WARN(
				"No room left for exporting unprocessed event, maximum is %d\n",
				AMDGV_EVENT_QUEUE_ENTRY_NUM);
			break;
		}

		/* Do not export SCHED_GPUMON, since it calls from user application */
		if (event->id != AMDGV_EVENT_SCHED_GPUMON) {
			event_info->unprocessed_events[i].idx_vf = event->idx_vf;
			event_info->unprocessed_events[i].id = event->id;
			event_info->unprocessed_events[i].sched_id = event->sched_block;
			i++;
		}

		amdgv_sched_free_event(adapt, event);
	}

	event_info->unprocessed_event_count = i;

	if (!amdgv_sched_event_queue_empty(adapt))
		AMDGV_INFO("New events detected after saving!.\n");

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_sched_import_unprocessed_event(struct amdgv_adapter *adapt,
					 struct amdgv_live_info_unprocessed_event *event_info)
{
	uint32_t i;
	struct amdgv_live_info_sched_event *event;
	union amdgv_sched_event_data data;
	oss_memset(&data, 0, sizeof(union amdgv_sched_event_data));

	for (i = 0; i < event_info->unprocessed_event_count; i++) {
		event = &event_info->unprocessed_events[i];
		if (amdgv_sched_event_queue_push_ex(adapt, event->idx_vf, event->id, event->sched_id, OSS_INVALID_HANDLE, data, true)) {
			AMDGV_WARN("Failed to queue unprocessed event!\n");
		}
	}

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}
