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

#ifndef AMDGV_GPUIOV_H
#define AMDGV_GPUIOV_H

#include "amdgv_sched.h"
#include "rlcv_sched_config_interface.h"

enum amdgv_gpuiov_hw_sched_type {
	AMDGV_HW_SCHED_TYPE_GFX,
	AMDGV_HW_SCHED_TYPE_MM,
	AMDGV_HW_SCHED_TYPE_MAX
};

#define AMDGV_NAME_MASK_HW_GFX (1 << AMDGV_HW_SCHED_TYPE_GFX)
#define AMDGV_NAME_MASK_HW_MM (1 << AMDGV_HW_SCHED_TYPE_MM)
#define AMDGV_NAME_MASK_HW_ALL (AMDGV_NAME_MASK_HW_GFX | AMDGV_NAME_MASK_HW_MM)

#define IS_HW_SCHED_TYPE_GFX(hw_sched_id) \
(adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_sched_type == AMDGV_HW_SCHED_TYPE_GFX)

#define IS_HW_SCHED_TYPE_MM(hw_sched_id) \
(adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_sched_type == AMDGV_HW_SCHED_TYPE_MM)

/* This is unified gpu iov command for different ASICs,
 * their specific definitions are different.
 */

enum amdgv_gpuiov_cmd {
	AMDGV_IDLE_GPU			       = 0x01,
	AMDGV_SAVE_GPU_STATE		       = 0x02,
	AMDGV_LOAD_GPU_STATE		       = 0x03,
	AMDGV_RUN_GPU			       = 0x04,
	AMDGV_CONTEXT_SWITCH		       = 0x05,
	AMDGV_ENABLE_AUTO_HW_SWITCH	       = 0x06,
	AMDGV_INIT_GPU			       = 0x07,
	AMDGV_SAVE_RLCV_STATE		       = 0x08,
	AMDGV_LOAD_RLCV_STATE		       = 0x09,
	AMDGV_ENABLE_MMSCH_VFGATE	       = 0x08,
	AMDGV_DISABLE_MMSCH_VFGATE	       = 0x09,
	AMDGV_CLEAR_VF_STATE		       = 0x0A,
	AMDGV_DISABLE_AUTO_HW_SCHED	       = 0x0B,
	AMDGV_DISABLE_AUTO_HW_SCHED_AND_SWITCH = 0x0C,
	AMDGV_SHUTDOWN_GPU		       = 0x0D,
	AMDGV_CONFIG_AUTO_HW_SCHED_MODE	       = 0x0F,
	AMDGV_EVENT_NOTIFICATION           = 0x09,
	AMDGV_TRANSFER_VF_DATA             = 0x0A,
};

enum amdgv_gpuiov_cmd_status {
	AMDGV_CMD_STATUS_DONE		 = 0x00,
	AMDGV_CMD_STATUS_UNSUPPORTED	 = 0x01,
	AMDGV_CMD_STATUS_ABORTED	 = 0x02,
	AMDGV_CMD_STATUS_IDLING_ENGINE	 = 0x11,
	AMDGV_CMD_STATUS_SAVING_STATE	 = 0x12,
	AMDGV_CMD_STATUS_LOADING_STATE	 = 0x13,
	AMDGV_CMD_STATUS_ENABLING_ENGINE = 0x14,
	AMDGV_CMD_STATUS_INITING_ENGINE	 = 0x15,
	AMDGV_CMD_STATUS_PENDING_EXECUTE = 0xFF,
};

/* For ACTIVE_FCN_ID_STATUS:
 *
 * Please search the reg spec for GPUIOV_ACTIVE_FCN_ID
 *
 * The status is updated by MMSCH/RLCV as it idles/saves/loads/runs
 *  or detects stall for the engine during auto-scheduling.
 * The status is also updated during manual scheduling
 *
 * For IDLING: MMSCH/RLCV is waiting for engine to be IDLE which should
 *  take max 1 job time or timeout if the engine is hung
 */
enum amdgv_gpuiov_active_fcn_status {
	AMDGV_ACTIVE_FCN_IDLE	 = 0x00,
	AMDGV_ACTIVE_FCN_ACTIVE	 = 0x01,
	AMDGV_ACTIVE_FCN_IDLING	 = 0x02,
	AMDGV_ACTIVE_FCN_SAVE	 = 0x03,
	AMDGV_ACTIVE_FCN_LOAD	 = 0x04,
	AMDGV_ACTIVE_FCN_STALLED = 0x05,
};

enum amdgv_gpuiov_intr_id {
	AMDGV_GFX_CMD_COMPLETE_INTR	    = (1 << 0),
	AMDGV_GFX_HANG_SELF_RECOVERED_INTR  = (1 << 1),
	AMDGV_GFX_HANG_NEED_FLR_INTR	    = (1 << 2),
	AMDGV_GFX_VM_BUSY_TRANSITION_INTR   = (1 << 3),
	AMDGV_UVD_CMD_COMPLETE_INTR	    = (1 << 8),
	AMDGV_UVD_HANG_SELF_RECOVERED_INTR  = (1 << 9),
	AMDGV_UVD_HANG_NEED_FLR_INTR	    = (1 << 10),
	AMDGV_UVD_VM_BUSY_TRANSITION_INTR   = (1 << 11),
	AMDGV_UVD1_CMD_COMPLETE_INTR	    = (1 << 12),
	AMDGV_UVD1_HANG_SELF_RECOVERED_INTR = (1 << 13),
	AMDGV_UVD1_HANG_NEED_FLR_INTR	    = (1 << 14),
	AMDGV_UVD1_VM_BUSY_TRANSITION_INTR  = (1 << 15),
	AMDGV_VCE_CMD_COMPLETE_INTR	    = (1 << 16),
	AMDGV_VCE_HANG_SELF_RECOVERED_INTR  = (1 << 17),
	AMDGV_VCE_HANG_NEED_FLR_INTR	    = (1 << 18),
	AMDGV_VCE_VM_BUSY_TRANSITION_INTR   = (1 << 19),
	AMDGV_HVVM_MAILBOX_TRN_ACK_INTR	    = (1 << 24),
	AMDGV_HVVM_MAILBOX_RCV_VALID_INTR   = (1 << 25),
};

enum amdgv_gpuiov_event_id {
	AMDGV_EVENT_GFX_FLR 		= 0x0,
	AMDGV_EVENT_TS_LOG			= 0x1,
	AMDGV_EVENT_PERF_LOG		= 0x2,
	AMDGV_EVENT_DEBUG_LOG		= 0x3,
};

/* repurpose UVD_VM_BUSY bit for MMSCH interrupt */
#define AMDGV_MMSCH_GENERAL_EVENT_NOTIFICATION_0_INTR AMDGV_UVD_VM_BUSY_TRANSITION_INTR
#define AMDGV_MMSCH_GENERAL_EVENT_NOTIFICATION_1_INTR AMDGV_UVD1_VM_BUSY_TRANSITION_INTR

struct amdgv_gpuiov_funcs {
	/* issue a gpuiov command to adapter */
	int (*set_cmd)(struct amdgv_adapter *adapt, enum amdgv_gpuiov_cmd cmd,
		       uint32_t hw_sched_id, uint32_t idx_vf, uint32_t next_idx_vf);
	bool (*is_cmd_complete)(struct amdgv_adapter *adapt, uint32_t hw_sched_id);

	void (*dump_gpuiov_cmd_status)(struct amdgv_adapter *adapt,
				       uint32_t hw_sched_id);
	/* fb settings */
	int (*set_total_fb_consumed)(struct amdgv_adapter *adapt, uint16_t total_fb_consumed);
	int (*get_total_fb_consumed)(struct amdgv_adapter *adapt, uint16_t *total_fb_consumed);
	int (*set_vf_fb)(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t offset,
			 uint32_t size);
	int (*get_vf_fb)(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *offset,
			 uint32_t *size, uint32_t *real_size);
	int (*set_csa)(struct amdgv_adapter *adapt, uint32_t csa_base);

	int (*get_vm_busy_status)(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				  uint32_t *vm_busy_status);

	/* enable or disable interrupt via gpuiov */
	int (*intr_enable)(struct amdgv_adapter *adapt, enum amdgv_gpuiov_intr_id intr_id,
			   bool enable);
	int (*get_intr_bits)(struct amdgv_adapter *adapt, uint32_t *bits);
	int (*set_intr_bits)(struct amdgv_adapter *adapt, uint32_t bits);

	int (*get_intr_status)(struct amdgv_adapter *adapt, uint32_t *bits);
	int (*clear_intr_status)(struct amdgv_adapter *adapt, uint32_t bits);

	/* hvvm mbox configuration */
	int (*get_hvvm_mbox_index)(struct amdgv_adapter *adapt, uint8_t *indexsched_id);
	int (*update_hvvm_mbox_index)(struct amdgv_adapter *adapt, uint8_t index);

	int (*rcv_hvvm_mbox_msg)(struct amdgv_adapter *adapt, uint8_t *msg_data);
	int (*trn_hvvm_mbox_data)(struct amdgv_adapter *adapt, uint8_t msg_data);

	int (*set_hvvm_mbox_valid)(struct amdgv_adapter *adapt, uint8_t bits);
	int (*set_hvvm_mbox_ack)(struct amdgv_adapter *adapt);

	int (*get_hvvm_mbox_msg_valid)(struct amdgv_adapter *adapt, uint32_t *bits);

	/* get or set active vfs, auto scheduling staffs */
	int (*add_active_vf)(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
			     uint32_t idx_vf);
	int (*remove_active_vf)(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				uint32_t idx_vf);

	int (*get_active_vfs)(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
			      uint32_t *active_vfs);
	int (*set_active_vfs)(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
			      uint32_t active_vfs);
	int (*get_active_vf_idx)(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				 uint32_t *vf_idx);
	int (*get_active_vf_status)(struct amdgv_adapter *adapt,
				    uint32_t hw_sched_id, uint8_t *status);
	int (*wait_auto_sched_stop)(struct amdgv_adapter *adapt,
				    uint32_t hw_sched_id);
	/* get or set time quanta index */
	int (*get_time_quanta_index)(struct amdgv_adapter *adapt, uint32_t idx_vf,
				     uint32_t hw_sched_id,
				     uint32_t *time_quanta_index);
	int (*set_time_quanta_index)(struct amdgv_adapter *adapt, uint32_t idx_vf,
				     uint32_t hw_sched_id,
				     uint32_t time_quanta_index);

	/* get or set 32-bit time quanta option */
	int (*get_time_quanta_option)(struct amdgv_adapter *adapt,
				      uint32_t hw_sched_id,
				      uint32_t *time_quanta_option);
	int (*set_time_quanta_option)(struct amdgv_adapter *adapt,
				      uint32_t hw_sched_id,
				      uint32_t time_quanta_option);

	/* get or set time quanta definitions */
	int (*get_time_quanta_definition)(struct amdgv_adapter *adapt,
					  uint32_t hw_sched_id, int index,
					  uint8_t *quanta_option);
	int (*set_time_quanta_definition)(struct amdgv_adapter *adapt,
					  uint32_t hw_sched_id, int index,
					  uint8_t quanta_option);

	int (*set_scheduler_config_descriptor)(struct amdgv_adapter *adapt,
					  uint32_t hw_sched_id, struct scheduler_memory_descriptor *sched_cfg);

	/* set time slice via second 32-bit of time quanta index */
	int (*set_time_slice)(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       uint32_t hw_sched_id,
				       uint32_t time_slice);

	/*set VF FB/doorbell/mmio protection */
	int (*set_vf_access)(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t vf_access_select, bool is_true);
	bool (*get_vf_access)(struct amdgv_adapter *adapt, uint32_t idx_vf,
				 uint32_t vf_access_select);
	int (*set_mmsch_vfgate)(struct amdgv_adapter *adapt, enum amdgv_gpuiov_cmd cmd,
				uint32_t hw_sched_id, uint32_t idx_vf,
				uint32_t next_idx_vf);
	int (*toggle_rlcg_vf_interface)(struct amdgv_adapter *adapt, uint32_t idx_vf, bool enable);

	int (*get_config_info)(struct amdgv_adapter *adapt);

	int (*set_event_notification)(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				uint32_t idx_vf, enum amdgv_gpuiov_event_id event_id, uint32_t value);

	int (*transfer_vf_data)(struct amdgv_adapter *adapt,
				uint32_t hw_sched_id, uint32_t idx_vf, bool to_export);
	uint32_t (*update_vf_busy_status)(struct amdgv_adapter *adapt, uint32_t hw_sched_id);
	void (*ctx_empty_intr_control)(struct amdgv_adapter *adapt, uint32_t hw_sched_id, bool enable);
	int (*setup_sched_debug_log)(struct amdgv_adapter *adapt, enum amdgv_auto_sched_log_op op);

	const char *(*cmd_to_name)(struct amdgv_adapter *adapt, uint32_t cmd, uint32_t hw_sched_id);
	bool (*skip_ctrl_block)(struct amdgv_adapter *adapt, int idx);
};

struct amdgv_gpuiov_ctrl_block {
	const char *name;
	enum amdgv_gpuiov_hw_sched_type hw_sched_type;
	enum amdgv_sched_block sched_block;
	enum amdgv_sched_mode sched_mode;
	uint8_t offset;
	uint8_t hw_inst;
	uint8_t pci_gpuiov_offset;

	enum amdgv_gpuiov_cmd		last_cmd;
	union {
		enum amdgv_gpuiov_cmd_status last_status : 8;
		uint32_t last_status_reg;
	};
};

struct amdgv_gpuiov_hw_sched_static_config {
	const char *name;
	enum amdgv_gpuiov_hw_sched_type hw_sched_type;
	enum amdgv_sched_block sched_block;
	enum amdgv_sched_mode default_sched_mode;
	uint8_t pci_gpuiov_offset;
	uint8_t hw_inst;
	uint8_t supported_sched_modes;	// Bitfield of amdgv_sched_mode
};

struct amdgv_gpuiov {
	/* gpuiov position in pci config space */
	uint32_t pos;

	/* total avail framebuffer (in MB) */
	uint32_t total_fb_avail;

	/* total usable fb (in MB) */
	uint32_t total_fb_usable;

	/* memory descriptor of the CSA */
	struct amdgv_memmgr_mem *csa_fb_mem;

	/* the start address of reservation area */
	uint64_t resv_addr;

	/* the total size of reservation area (in MB) */
	uint32_t resv_size;

	uint32_t csa_data;
	uint32_t csa_offset;
	uint32_t csa_size_per_vf;
	uint32_t csa_max_vf_num;

	/* allowed time for completing a gpuiov cmd (in us) */
	uint64_t allow_time_cmd_complete;

	uint32_t num_ctrl_blocks;
	struct amdgv_gpuiov_ctrl_block  ctrl_blocks[AMDGV_MAX_NUM_HW_SCHED];

	struct scheduler_memory_descriptor sched_cfg;
	struct amdgv_memmgr_mem *sched_cfg_mem;

	struct amdgv_memmgr_mem *debug_dump_mem;
	struct amdgv_memmgr_mem *perf_log_mem;
#ifdef WS_RECORD
	uint32_t auto_ws_record_rptr;
#endif

	const struct amdgv_gpuiov_funcs *funcs;
};

struct amdgv_gpuiov_wait_context{
	struct amdgv_adapter *adapt;
	uint32_t hw_sched_id;
};

struct amdgv_gpuiov_wait_for_first_context {
	struct amdgv_adapter *adapt;
	/* CB Input: mask */
	uint32_t hw_sched_mask;
	/* CB Output: first completed sched id*/
	uint32_t complete_hw_sched_id;
};

int wait_for_first_cmd_complete(struct amdgv_adapter *adapt,
		uint32_t hw_sched_mask, uint32_t *complete_hw_sched_id, uint64_t timeout);

int amdgv_gpuiov_init(struct amdgv_adapter *adapt);

int amdgv_gpuiov_fini(struct amdgv_adapter *adapt);

void amdgv_gpuiov_get_usable_fb_size(struct amdgv_adapter *adapt, uint32_t *total_fb_usable);
void amdgv_gpuiov_get_total_avail_fb_size(struct amdgv_adapter *adapt,
					  uint32_t *total_fb_avail);
void amdgv_gpuiov_get_resv_area(struct amdgv_adapter *adapt, uint64_t *addr, uint64_t *size);
void amdgv_gpuiov_set_total_fb_consumed(struct amdgv_adapter *adapt, uint16_t size);
void amdgv_gpuiov_get_total_fb_consumed(struct amdgv_adapter *adapt, uint16_t *size);
int amdgv_gpuiov_set_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t offset,
			   uint32_t size);
int amdgv_gpuiov_get_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *offset,
			   uint32_t *size, uint32_t *real_size);
bool amdgv_gpuiov_is_cmd_complete(struct amdgv_adapter *adapt,
				  uint32_t hw_sched_id);
void amdgv_dump_gpuiov_cmd_status(struct amdgv_adapter *adapt,
				  uint32_t hw_sched_id);
int amdgv_gpuiov_init_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_id);
int amdgv_gpuiov_load_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_id);
int amdgv_gpuiov_idle_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_id);
int amdgv_gpuiov_save_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_id);
int amdgv_gpuiov_run_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			uint32_t hw_sched_id);
int amdgv_gpuiov_load_rlcv_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
				 uint32_t hw_sched_id);
int amdgv_gpuiov_save_rlcv_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
				 uint32_t hw_sched_id);
int amdgv_gpuiov_shutdown_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id);
int amdgv_gpuiov_config_auto_sched_mode(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id,
					enum amdgv_sched_mode sched_mode);
int amdgv_gpuiov_enable_auto_sched(struct amdgv_adapter *adapt,
				   uint32_t hw_sched_id);
int amdgv_gpuiov_disable_auto_sched(struct amdgv_adapter *adapt,
				    uint32_t hw_sched_id);
int amdgv_gpuiov_auto_sched_add_vf(struct amdgv_adapter *adapt,
				   uint32_t hw_sched_id, uint32_t idx_vf);
int amdgv_gpuiov_auto_sched_remove_vf(struct amdgv_adapter *adapt,
				      uint32_t hw_sched_id, uint32_t idx_vf);

int amdgv_gpuiov_idle_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id);
int amdgv_gpuiov_run_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id);
int amdgv_gpuiov_save_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id);
int amdgv_gpuiov_init_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id);
int amdgv_gpuiov_load_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id);
int amdgv_gpuiov_shutdown_vf_no_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id);

int amdgv_gpuiov_bulk_run_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_mask);
int amdgv_gpuiov_bulk_save_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_mask);
int amdgv_gpuiov_bulk_idle_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_mask);
int amdgv_gpuiov_bulk_load_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_mask);
int amdgv_gpuiov_bulk_init_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_mask);
int amdgv_gpuiov_bulk_save_rlcv_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_mask);
int amdgv_gpuiov_bulk_load_rlcv_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_mask);
int amdgv_gpuiov_bulk_shutdown_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			 uint32_t hw_sched_mask);

int amdgv_gpuiov_get_intr(struct amdgv_adapter *adapt, uint32_t *intr_bits);
int amdgv_gpuiov_set_intr(struct amdgv_adapter *adapt, uint32_t intr_bits);
int amdgv_gpuiov_get_intr_status(struct amdgv_adapter *adapt, uint32_t *sta_bits);
int amdgv_gpuiov_clear_intr_status(struct amdgv_adapter *adapt, uint32_t sta_bits);
int amdgv_gpuiov_update_hvvm_mbox_index(struct amdgv_adapter *adapt, uint8_t index);
int amdgv_gpuiov_rcv_hvvm_mbox_msg(struct amdgv_adapter *adapt, uint8_t *msg_data);
int amdgv_gpuiov_trn_hvvm_mbox_data(struct amdgv_adapter *adapt, uint8_t msg_data);
int amdgv_gpuiov_set_hvvm_mbox_valid(struct amdgv_adapter *adapt, int8_t valid);
int amdgv_gpuiov_set_hvvm_mbox_ack(struct amdgv_adapter *adapt);
int amdgv_gpuiov_get_hvvm_mbox_msg_valid(struct amdgv_adapter *adapt, uint32_t *valid);

int amdgv_gpuiov_set_hvvm_mbox_valid(struct amdgv_adapter *adapt, int8_t valid);

int amdgv_gpuiov_get_active_vfs(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				uint32_t *active_vfs);
int amdgv_gpuiov_set_active_vfs(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				uint32_t active_vfs);
int amdgv_gpuiov_get_time_quanta_index(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       uint32_t hw_sched_id,
				       uint32_t *time_quanta_index);
int amdgv_gpuiov_set_time_quanta_index(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       uint32_t hw_sched_id,
				       uint32_t time_quanta_index);
int amdgv_gpuiov_get_time_quanta_option(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id,
					uint32_t *time_quanta_option);
int amdgv_gpuiov_set_time_quanta_option(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id,
					uint32_t time_quanta_option);
int amdgv_gpuiov_get_time_quanta_definition(struct amdgv_adapter *adapt,
					    uint32_t hw_sched_id, int index,
					    uint8_t *quanta_option);
int amdgv_gpuiov_set_time_quanta_definition(struct amdgv_adapter *adapt,
					    uint32_t hw_sched_id, int index,
					    uint8_t quanta_option);
int amdgv_gpuiov_get_active_vf_idx(struct amdgv_adapter *adapt,
				   uint32_t hw_sched_id, uint32_t *idx_vf);
int amdgv_gpuiov_get_active_vf_status(struct amdgv_adapter *adapt,
				      uint32_t hw_sched_id, uint8_t *status);
int amdgv_gpuiov_set_scheduler_config_descriptor(struct amdgv_adapter *adapt,
					  uint32_t hw_sched_id, struct scheduler_memory_descriptor *sched_cfg);
int amdgv_gpuiov_wait_auto_sched_stop(struct amdgv_adapter *adapt,
				      uint32_t hw_sched_id);
int amdgv_gpuiov_set_sriov_vf_num(struct amdgv_adapter *adapt, uint32_t num_vf);
bool amdgv_gpuiov_is_sched_mode_supported(struct amdgv_adapter *adapt,
					 struct amdgv_gpuiov_hw_sched_static_config hw_sched_config, enum amdgv_sched_mode sched_mode);
const char *amdgv_gpuiov_cmd_to_name_default(struct amdgv_adapter *adapt, uint32_t cmd, uint32_t hw_sched_id);
const char *amdgv_gpuiov_cmd_to_name(struct amdgv_adapter *adapt, uint32_t cmd, uint32_t hw_sched_id);
const char *amdgv_gpuiov_status_to_name(uint32_t status);
int amdgv_gpuiov_transfer_vf_data(struct amdgv_adapter *adapt,
				  uint32_t idx_vf, uint32_t hw_sched_id, bool to_export);
int amdgv_gpuiov_set_mmsch_vfgate(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  uint32_t hw_sched_id, bool enable);
int amdgv_gpuiov_world_switch_oneshot(struct amdgv_adapter *adapt,
				      uint32_t idx_vf, uint32_t next_idx_vf,
				      uint32_t hw_sched_id);
int amdgv_config_auto_sched_params(struct amdgv_adapter *adapt, uint32_t hw_sched_id);
bool amdgv_gpuiov_intr_status(struct amdgv_adapter *adapt,
			      enum amdgv_gpuiov_intr_id intr_id);

int amdgv_gpuiov_event_notification(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id, enum amdgv_gpuiov_event_id event_id, uint32_t value);

int amdgv_gpuiov_set_vf_access(struct amdgv_adapter *adapt, uint32_t idx_vf,
			       uint32_t vf_access_select, bool enable);
bool amdgv_gpuiov_get_vf_access(struct amdgv_adapter *adapt, uint32_t idx_vf,
			       uint32_t vf_access_select);
int amdgv_gpuiov_toggle_rlcg_vf_interface(struct amdgv_adapter *adapt, uint32_t idx_vf, bool enable);

void amdgv_gpuiov_ctx_empty_intr_control(struct amdgv_adapter *adapt, uint32_t hw_sched_id, bool enable);

int amdgv_gpuiov_ctrl_block_setup(struct amdgv_adapter *adapt, struct amdgv_gpuiov_hw_sched_static_config *hw_sched_config,
					 uint32_t size);

/**
 * amdgv_gpuiov_set_vf_access - set the  vf access status
 *
 * @adapt : amdgv_adapter
 * @idx_vf: index for the target vf
 * @area: 0 --fb , 1 --doorbell, 2 --mmio
 * @enable: true -- enable the access . false -- disable the access
 *
 */
int amdgv_hw_sched_state_run(struct amdgv_adapter *adapt, uint32_t idx_vf,
			   uint32_t hw_sched_id);
int amdgv_hw_sched_state_run_auto(struct amdgv_adapter *adapt, uint32_t idx_vf,
				uint32_t hw_sched_id);
int amdgv_hw_sched_state_pause(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     uint32_t hw_sched_id);
int amdgv_hw_sched_state_shutdown(struct amdgv_adapter *adapt, uint32_t idx_vf,
				uint32_t hw_sched_id);

int amdgv_logical_sched_state_run(struct amdgv_adapter *adapt, uint32_t idx_vf,
			   struct amdgv_sched_world_switch *world_switch);
int amdgv_logical_sched_state_pause(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     struct amdgv_sched_world_switch *world_switch);
int amdgv_logical_sched_state_shutdown(struct amdgv_adapter *adapt, uint32_t idx_vf,
				struct amdgv_sched_world_switch *world_switch);

int world_switch_bulk_goto_state_manual(struct amdgv_adapter *adapt, uint32_t target_vf,
					  uint32_t hw_sched_mask,
					  uint32_t target_state);

int amdgv_gpuiov_update_vf_busy_status(struct amdgv_adapter *adapt);

void amdgv_update_histogram(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint32_t hw_sched_id, enum amdgv_gpuiov_cmd cmd,
				   uint64_t cmd_start_time);

int amdgv_gpuiov_set_debug_dump_reg(struct amdgv_adapter *adapt);
void amdgv_bp_mode_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint32_t hw_sched_id, uint32_t ws_cmd);
#endif

