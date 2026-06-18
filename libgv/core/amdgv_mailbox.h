/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_MAILBOX_H
#define AMDGV_MAILBOX_H

#include "amdgv_oss_wrapper.h"

struct amdgv_adapter;

/* Message length */
enum amdgv_mailbox_data_len {
	MAILBOX_DATA_LEN_1 = 1,
	MAILBOX_DATA_LEN_2,
	MAILBOX_DATA_LEN_3,
	MAILBOX_DATA_LEN_4 /* this is maximum supported by platform */
};

struct amdgv_mailbox_funcs {
	int (*get_index)(struct amdgv_adapter *adapt, uint32_t *idx_vf);
	int (*update_index)(struct amdgv_adapter *adapt, uint32_t idx_vf);

	int (*rcv_msg)(struct amdgv_adapter *adapt, uint32_t idx_vf, int offset,
		       uint32_t *msg_data);
	int (*trn_msg)(struct amdgv_adapter *adapt, uint32_t idx_vf, int offset,
		       uint32_t msg_data);

	int (*trn_msg_valid)(struct amdgv_adapter *adapt, uint32_t idx_vf, bool valid);
	int (*ack_msg)(struct amdgv_adapter *adapt, uint32_t idx_vf);

	int (*reset)(struct amdgv_adapter *adapt);

	/* save/restore mailbox state if needed when flr a vf */
	int (*save_state)(struct amdgv_adapter *adapt, uint32_t idx_vf);
	int (*restore_state)(struct amdgv_adapter *adapt, uint32_t idx_vf);
	int (*peek_ack)(struct amdgv_adapter *adapt);

	void (*irq_source_enable)(struct amdgv_adapter *adapt, bool enable);
};

struct amdgv_mailbox {
	spin_lock_t lock;
	spin_lock_t hvvm_lock;

	/* the trn and ack message buffer len of mailbox  */
	uint32_t msg_buf_len;

	struct {
		bool rcv_msg_acked;
		uint32_t rcv_ack_count;
		uint32_t saved_ack_count;
		uint32_t msg_trn_dw0;
		uint32_t msg_trn_dw1;
		uint32_t msg_trn_dw2;
		uint32_t msg_trn_dw3;
	} state_vf[AMDGV_MAX_VF_SLOT];

	const struct amdgv_mailbox_funcs *funcs;
};

int amdgv_mailbox_receive_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *msg_data,
			      int msg_len, bool need_ack);
int amdgv_mailbox_receive_ack(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t timeout);
int amdgv_mailbox_send_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *msg_data,
			   int msg_len, bool need_valid);
int amdgv_mailbox_clear_valid_msg(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_mailbox_save_state(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_mailbox_restore_state(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_mailbox_hvvm_receive_msg(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint8_t *msg_data, bool need_ack);
int amdgv_mailbox_hvvm_send_msg(struct amdgv_adapter *adapt, uint32_t idx_vf, uint8_t msg_data,
				bool need_valid);
int amdgv_mailbox_hvvm_clear_valid_msg(struct amdgv_adapter *adapt);

int amdgv_mailbox_init(struct amdgv_adapter *adapt, const struct amdgv_mailbox_funcs *funcs);
int amdgv_mailbox_fini(struct amdgv_adapter *adapt);

enum amdgv_sched_event_id amdgv_mailbox_get_valid_vf_event(struct amdgv_adapter *adapt,
							   uint32_t event);

enum amdgv_sched_event_id amdgv_hvvm_mb_get_valid_event(struct amdgv_adapter *adapt,
							   uint32_t event);

const char *amdgv_mailbox_rcv_idh_to_name(uint32_t idh);
const char *amdgv_mailbox_trn_idh_to_name(uint32_t idh);

int amdgv_mailbox_notify_gpu_debug(struct amdgv_adapter *adapt, uint32_t idx_vf, bool completion);

int amdgv_mailbox_wait_trn_msg_ack(struct amdgv_adapter *adapt);

int amdgv_mailbox_irq_source_enable(struct amdgv_adapter *adapt, bool enable);
#endif
