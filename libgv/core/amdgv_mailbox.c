/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_oss_wrapper.h"
#include "amdgv_device.h"
#include "amdgv_gpuiov.h"
#include "amdgv_sched.h"
#include "amdgv_diag_data_trace_log.h"

static struct amdgv_id_name amdgv_mailbox_rcv_names[] = {
	{ MB_REQ_MSG_REQ_GPU_INIT_ACCESS, "REQ_GPU_INIT" },
	{ MB_REQ_MSG_REL_GPU_INIT_ACCESS, "REL_GPU_INIT" },
	{ MB_REQ_MSG_REQ_GPU_FINI_ACCESS, "REQ_GPU_FINI" },
	{ MB_REQ_MSG_REL_GPU_FINI_ACCESS, "REL_GPU_FINI" },
	{ MB_REQ_MSG_REQ_GPU_RESET_ACCESS, "REQ_GPU_RESET" },
	{ MB_REQ_MSG_REQ_GPU_INIT_DATA, "REQ_GPU_INIT_DATA" },
	{ MB_REQ_MSG_LOG_VF_ERROR, "LOG_VF_ERROR" },
	{ MB_REQ_MSG_READY_TO_RESET, "READY_TO_RESET" },
	{ MB_REQ_MSG_RAS_POISON, "RAS_POISON" },
	{ MB_REQ_RAS_ERROR_COUNT, "REQ_RAS_ERROR_COUNT" },
	{ MB_REQ_RAS_CPER_DUMP, "REQ_RAS_CPER_DUMP" },
	{ MB_REQ_RAS_REMOTE_CMD, "REQ_RAS_REMOTE_CMD" },
	{ MB_REQ_RAS_BAD_PAGES, "REQ_RAS_BAD_PAGES" },
	{ MB_REQ_MSG_REQ_GPU_DEBUG, "REQ_GPU_DEBUG" },
	{ MB_REQ_MSG_REL_GPU_DEBUG, "REL_GPU_DEBUG" },
	{ MB_REQ_RAS_CHK_CRITI, "RAS_CHK_CRITI_REGION" },
	{ MB_REQ_MSG_REQ_GPU_INIT_XCHG_REGION, "REQ_GPU_INIT_XCHG_REGION" },
};

static struct amdgv_id_name amdgv_mailbox_trn_names[] = {
	{ MB_RES_MSG_CLR_MSG_BUF, "CLEAR_BUF" },
	{ MB_RES_MSG_READY_TO_ACCESS_GPU, "READY_TO_ACCESS_GPU" },
	{ MB_RES_MSG_FLR_NOTIFICATION, "NOTIFY_FLR_START" },
	{ MB_RES_MSG_FLR_NOTIFICATION_COMPLETION, "NOTIFY_FLR_COMPLETE" },
	{ MB_RES_MSG_GPU_INIT_DATA_READY, "NOTIFY_GPU_INIT_DATA_READY" },
	{ MB_RES_MSG_RAS_POISON_READY, "NOTIFY_RAS_POISON_READY" },
	{ MB_RES_MSG_GPU_RMA, "GPU_RMA" },
	{ MB_RES_MSG_GPU_DEBUG_NOTIFICATION, "GPU_DEBUG_NOTIFICATION" },
	{ MB_RES_MSG_GPU_DEBUG_NOTIFICATION_COMPLETION, "GPU_DEBUG_NOTIFICATION_COMPLETION" },
	{ MB_RES_MSG_REQ_GPU_INIT_XCHG_REGION_READY, "REQ_GPU_INIT_XCHG_REGION" },
};

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

int amdgv_mailbox_receive_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *msg_data,
			      int msg_len, bool need_ack)
{
	int i;

	if (!msg_data) {
		AMDGV_ERROR("Message data storage not allocated\n", idx_vf);
		return AMDGV_FAILURE;
	}

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_ERROR("Invalid index: index %d does not belong to any VF or PF\n",
			    idx_vf);
		return AMDGV_FAILURE;
	}

	if (msg_len > (int)adapt->mailbox.msg_buf_len)
		AMDGV_WARN("Message too long: only first %d considered\n",
			   adapt->mailbox.msg_buf_len);

	oss_spin_lock_irq(adapt->mailbox.lock);

	adapt->mailbox.funcs->update_index(adapt, idx_vf);

	for (i = 0; i < msg_len; i++)
		adapt->mailbox.funcs->rcv_msg(adapt, idx_vf, i, &msg_data[i]);

	/* clear message before ack */
	for (i = 0; i < (int)adapt->mailbox.msg_buf_len; i++)
		adapt->mailbox.funcs->trn_msg(adapt, idx_vf, i, 0);

	adapt->mailbox.funcs->trn_msg_valid(adapt, idx_vf, false);

	if (need_ack) {
		adapt->mailbox.funcs->ack_msg(adapt, idx_vf);
		adapt->mailbox.state_vf[idx_vf].rcv_ack_count++;
	}

	/* diagnosis data Log */
	AMDGV_DIAG_DATA_TRACE_LOG_MB(idx_vf, msg_data[0], AMDGV_DIAG_DATA_TRACE_MB_DIR_VF_TO_PF);

	oss_spin_unlock_irq(adapt->mailbox.lock);

	return 0;
}

int amdgv_mailbox_send_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *msg_data,
			   int msg_len, bool need_valid)
{
	int i;

	if (!msg_data) {
		AMDGV_ERROR("Message data storage not allocated\n", idx_vf);
		return AMDGV_FAILURE;
	}

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_ERROR("Invalid index: index %d does not belong to any VF or PF\n",
			    idx_vf);
		return AMDGV_FAILURE;
	}

	if (msg_len > (int)adapt->mailbox.msg_buf_len)
		AMDGV_WARN("Message too long: only first %d considered\n",
			   adapt->mailbox.msg_buf_len);

	oss_spin_lock_irq(adapt->mailbox.lock);

	adapt->mailbox.funcs->update_index(adapt, idx_vf);

	for (i = 0; i < msg_len; i++)
		adapt->mailbox.funcs->trn_msg(adapt, idx_vf, i, msg_data[i]);

	if (need_valid)
		adapt->mailbox.funcs->trn_msg_valid(adapt, idx_vf, true);

	/* diagnosis data Log */
	AMDGV_DIAG_DATA_TRACE_LOG_MB(idx_vf, msg_data[0], AMDGV_DIAG_DATA_TRACE_MB_DIR_PF_TO_VF);

	oss_spin_unlock_irq(adapt->mailbox.lock);

	return 0;
}

int amdgv_mailbox_clear_valid_msg(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int i;

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_ERROR("Invalid index: index %d does not belong to any VF or PF\n",
			    idx_vf);
		return AMDGV_FAILURE;
	}

	oss_spin_lock_irq(adapt->mailbox.lock);

	adapt->mailbox.funcs->update_index(adapt, idx_vf);

	for (i = 0; i < (int)adapt->mailbox.msg_buf_len; i++)
		adapt->mailbox.funcs->trn_msg(adapt, idx_vf, i, 0);

	adapt->mailbox.funcs->trn_msg_valid(adapt, idx_vf, false);

	oss_spin_unlock_irq(adapt->mailbox.lock);

	return 0;
}

int amdgv_mailbox_save_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_ERROR("Invalid index: index %d does not belong to any VF or PF\n",
			    idx_vf);
		return AMDGV_FAILURE;
	}

	oss_spin_lock_irq(adapt->mailbox.lock);

	adapt->mailbox.funcs->update_index(adapt, idx_vf);

	adapt->mailbox.funcs->save_state(adapt, idx_vf);

	oss_spin_unlock_irq(adapt->mailbox.lock);

	return 0;
}

int amdgv_mailbox_restore_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_ERROR("Invalid index: index %d does not belong to any VF or PF\n",
			    idx_vf);
		return AMDGV_FAILURE;
	}

	oss_spin_lock_irq(adapt->mailbox.lock);

	adapt->mailbox.funcs->update_index(adapt, idx_vf);

	adapt->mailbox.funcs->restore_state(adapt, idx_vf);

	oss_spin_unlock_irq(adapt->mailbox.lock);

	return 0;
}

/* new mailbox for HV<-->VM msg */
int amdgv_mailbox_hvvm_receive_msg(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint8_t *msg_data, bool need_ack)
{
	if (!msg_data) {
		AMDGV_ERROR("Message data storage not allocated\n", idx_vf);
		return AMDGV_FAILURE;
	}

	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_ERROR("Invalid index: index %d does not belong to any VF or PF\n",
			    idx_vf);
		return AMDGV_FAILURE;
	}

	oss_spin_lock_irq(adapt->mailbox.hvvm_lock);

	amdgv_gpuiov_update_hvvm_mbox_index(adapt, idx_vf);
	amdgv_gpuiov_rcv_hvvm_mbox_msg(adapt, msg_data);

	if (need_ack)
		amdgv_gpuiov_set_hvvm_mbox_ack(adapt);

	oss_spin_unlock_irq(adapt->mailbox.hvvm_lock);

	return 0;
}

int amdgv_mailbox_hvvm_send_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, uint8_t msg_data,
				bool need_valid)
{
	if (AMDGV_IS_IDX_INVALID(idx_vf)) {
		AMDGV_ERROR("Invalid index: index %d does not belong to any VF or PF\n",
			    idx_vf);
		return AMDGV_FAILURE;
	}

	oss_spin_lock_irq(adapt->mailbox.hvvm_lock);

	amdgv_gpuiov_update_hvvm_mbox_index(adapt, idx_vf);

	amdgv_gpuiov_trn_hvvm_mbox_data(adapt, msg_data);

	if (need_valid)
		amdgv_gpuiov_set_hvvm_mbox_valid(adapt, 1);

	oss_spin_unlock_irq(adapt->mailbox.hvvm_lock);

	return 0;
}

int amdgv_mailbox_hvvm_clear_valid_msg(struct amdgv_adapter *adapt)
{
	oss_spin_lock_irq(adapt->mailbox.hvvm_lock);
	amdgv_gpuiov_set_hvvm_mbox_valid(adapt, 0);
	oss_spin_unlock_irq(adapt->mailbox.hvvm_lock);

	return 0;
}

enum amdgv_sched_event_id amdgv_hvvm_mb_get_valid_event(struct amdgv_adapter *adapt,
							uint32_t event)
{
	enum amdgv_sched_event_id sched_event;

	switch (event) {
	case HVVM_MB_MSG_CTX_EMPTY:
		sched_event = AMDGV_EVENT_CUR_VF_CTX_EMPTY;
		break;
	default:
		sched_event = AMDGV_EVENT_INVALID_EVENT;
	}

	return sched_event;
}

enum amdgv_sched_event_id amdgv_mailbox_get_valid_vf_event(struct amdgv_adapter *adapt,
							   uint32_t event)
{
	enum amdgv_sched_event_id sched_event = AMDGV_EVENT_INVALID_EVENT;

	switch (event) {
	case MB_REQ_MSG_REQ_GPU_INIT_ACCESS:
		sched_event = AMDGV_EVENT_REQ_GPU_INIT;
		break;
	case MB_REQ_MSG_REL_GPU_INIT_ACCESS:
		sched_event = AMDGV_EVENT_REL_GPU_INIT;
		break;
	case MB_REQ_MSG_REQ_GPU_FINI_ACCESS:
		sched_event = AMDGV_EVENT_REQ_GPU_FINI;
		break;
	case MB_REQ_MSG_REL_GPU_FINI_ACCESS:
		sched_event = AMDGV_EVENT_REL_GPU_FINI;
		break;
	case MB_REQ_MSG_REQ_GPU_RESET_ACCESS:
		sched_event = AMDGV_EVENT_REQ_GPU_RESET;
		break;
	case MB_RES_MSG_TEXT_MESSAGE:
		sched_event = AMDGV_EVENT_TEXT_MESSAGE;
		break;
	case MB_REQ_MSG_LOG_VF_ERROR:
		sched_event = AMDGV_EVENT_LOG_VF_ERROR;
		break;
	case MB_REQ_MSG_READY_TO_RESET:
		sched_event = AMDGV_EVENT_READY_TO_RESET;
		break;
	case MB_REQ_MSG_REQ_GPU_INIT_DATA:
		sched_event = AMDGV_EVENT_REQ_GPU_INIT_DATA;
		break;
	case MB_REQ_MSG_PSP_VF_CMD_RELAY:
		sched_event = AMDGV_EVENT_SCHED_PSP_VF_CMD_RELAY;
		break;
	case MB_REQ_MSG_RAS_POISON:
		sched_event = AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION;
		break;
	case MB_REQ_RAS_ERROR_COUNT:
		sched_event = AMDGV_EVENT_SCHED_VF_REQ_RAS_ERROR_COUNT;
		break;
	case MB_REQ_RAS_CPER_DUMP:
		sched_event = AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP;
		break;
	case MB_REQ_RAS_BAD_PAGES:
		sched_event = AMDGV_EVENT_SCHED_VF_REQ_RAS_BAD_PAGES;
		break;
	case MB_REQ_RAS_REMOTE_CMD:
		sched_event = AMDGV_EVENT_SCHED_VF_RAS_REMOTE_CMD;
		break;
	case MB_REQ_MSG_REQ_GPU_DEBUG:
		// req_gpu_debug & rel_gpu_debug are only valid when debug_mode is multi-vf case
		if (is_debug_mode_multi_vf())
			sched_event = AMDGV_EVENT_REQ_GPU_DEBUG;
		break;
	case MB_REQ_MSG_REL_GPU_DEBUG:
		if (is_debug_mode_multi_vf())
			sched_event = AMDGV_EVENT_REL_GPU_DEBUG;
		break;
	case MB_REQ_RAS_CHK_CRITI:
		sched_event = AMDGV_EVENT_SCHED_VF_REQ_RAS_CHK_CRITI_REGION;
		break;
	case MB_REQ_MSG_PTL_UPDATE:
		sched_event = AMDGV_EVENT_VF_REQ_PTL_UPDATE;
		break;
	case MB_REQ_MSG_REQ_GPU_INIT_XCHG_REGION:
		sched_event = AMDGV_EVENT_SCHED_VF_REQ_GPU_INIT_XCHG_REGION;
		break;
	default:
		sched_event = AMDGV_EVENT_INVALID_EVENT;
	}

	return sched_event;
}

const char *amdgv_mailbox_rcv_idh_to_name(uint32_t idh)
{
	int i, count;

	count = ARRAY_SIZE(amdgv_mailbox_rcv_names);
	for (i = 0; i < count; ++i) {
		if (amdgv_mailbox_rcv_names[i].id == idh)
			return amdgv_mailbox_rcv_names[i].name;
	}
	return "UNKNOWN_RCV_IDH";
}

const char *amdgv_mailbox_trn_idh_to_name(uint32_t idh)
{
	int i, count;

	count = ARRAY_SIZE(amdgv_mailbox_trn_names);
	for (i = 0; i < count; ++i) {
		if (amdgv_mailbox_trn_names[i].id == idh)
			return amdgv_mailbox_trn_names[i].name;
	}
	return "UNKNOWN_TRN_IDH";
}

int amdgv_mailbox_init(struct amdgv_adapter *adapt, const struct amdgv_mailbox_funcs *funcs)
{
	adapt->mailbox.lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
	if (adapt->mailbox.lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		return AMDGV_FAILURE;
	}
	adapt->mailbox.hvvm_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
	if (adapt->mailbox.hvvm_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		return AMDGV_FAILURE;
	}

	adapt->mailbox.funcs = funcs;

	return 0;
}

int amdgv_mailbox_fini(struct amdgv_adapter *adapt)
{
	if (adapt->mailbox.lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->mailbox.lock);
		adapt->mailbox.lock = OSS_INVALID_HANDLE;
	}

	if (adapt->mailbox.hvvm_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->mailbox.hvvm_lock);
		adapt->mailbox.hvvm_lock = OSS_INVALID_HANDLE;
	}

	adapt->mailbox.funcs = NULL;

	return 0;
}

int amdgv_mailbox_notify_gpu_debug(struct amdgv_adapter *adapt, uint32_t idx_vf, bool completion)
{
	uint32_t i = 0;
	uint32_t msg_data[MAILBOX_DATA_LEN_1] = {0};

	for (i = 0; i < adapt->num_vf; i++) {
		/* skip current vf */
		if (i == idx_vf) {
			continue;
		}

		/* notify vf gpu_debug completion */
		if (completion) {
			msg_data[0] = MB_RES_MSG_GPU_DEBUG_NOTIFICATION_COMPLETION;
			AMDGV_DEBUG("notify %s gpu debug notification completion\n", amdgv_idx_to_str(i));
		} else {
			msg_data[0] = MB_RES_MSG_GPU_DEBUG_NOTIFICATION;
			AMDGV_DEBUG("notify %s gpu debug notification\n", amdgv_idx_to_str(i));
		}

		amdgv_mailbox_send_msg(adapt, i, msg_data, MAILBOX_DATA_LEN_1, true);
	}

	return 0;
}

static int amdgv_mailbox_wait_trn_msg_ack_cb(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	return !adapt->mailbox.funcs->peek_ack(adapt);
}

/*
 * Wait function for mailbox ack from vf after a mailbox message is sent to vf
 * Wait until ack received or timeout reached
 */
int amdgv_mailbox_wait_trn_msg_ack(struct amdgv_adapter *adapt)
{
	uint32_t wait_flag = 0;
	struct amdgv_wait_for_cb_context cb_context = { 0 };

	if (!adapt->mailbox.funcs->peek_ack) {
		return AMDGV_FAILURE;
	}

	cb_context.ctx = (void *)adapt;
	cb_context.type = AMDGV_WAIT_FOR_MB_TRN_MSG_ACK;

	return amdgv_wait_for(adapt, amdgv_mailbox_wait_trn_msg_ack_cb, &cb_context,
				 AMDGV_TIMEOUT(TIMEOUT_CMD_RESP), wait_flag);
}

int amdgv_mailbox_irq_source_enable(struct amdgv_adapter *adapt, bool enable)
{
	if (!adapt->mailbox.funcs->irq_source_enable) {
		AMDGV_ERROR("Unable to enable IRQ source\n");
		return AMDGV_FAILURE;
	} else {
		adapt->mailbox.funcs->irq_source_enable(adapt, enable);
	}

	return 0;
}
