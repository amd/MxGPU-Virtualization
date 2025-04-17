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

#ifndef AMDGV_SCHED_H
#define AMDGV_SCHED_H

#include "amdgv.h"
#include "amdgv_mcp.h"
#include "amdgv_gpumon.h"
#include "amdgv_ras.h"

struct amdgv_live_info;
struct amdgv_live_info_unprocessed_event;
struct amdgv_live_info_sched;

/* default gfx time slice is 6ms */
#define DEFAULT_GFX_TIME_SLICE			6000

/* get asic time slice for gfx*/
#define GET_GFX_TIME_SLICE(adapt, num_vf)		((adapt->sched.get_asic_time_slice) ? \
				(adapt->sched.get_asic_time_slice(adapt, AMDGV_SCHED_BLOCK_GFX, num_vf)) : DEFAULT_GFX_TIME_SLICE)
/* whether need to switch to pf when submite PSP command */
#define NEED_SWITCH_TO_PF(adapt) ((adapt->psp.need_switch_to_pf != NULL) ? \
				(adapt->psp.need_switch_to_pf(adapt)) : true)
#define RAS_NEED_SWITCH_TO_PF(adapt) ((adapt->psp.ras_need_switch_to_pf != NULL) ? \
				(adapt->psp.ras_need_switch_to_pf(adapt)) : true)
/* Infinite timeout is used if self-switch is not enabled */
#define DEFAULT_GFX_TIME_SLICE_1VF	(0xffffffff)
/* default gfx time slice is 500ms if only 1VF */
#define DEFAULT_GFX_TIME_SLICE_1VF_SELF_SW		(500*1000)

/* default multimedia (uvd, vce, vcn) time slice is 255ms */
#define DEFAULT_MM_TIME_SLICE	255000
/* get multimedia slice is 255ms, since mm use same policy, use UVD for convenient*/
#define GET_MM_TIME_SLICE(adapt) 		((adapt->sched.get_asic_time_slice) ? \
				(adapt->sched.get_asic_time_slice(adapt, AMDGV_SCHED_BLOCK_UVD)) : DEFAULT_MM_TIME_SLICE)

/* different kinds of allowed VF assignment for sched engines (GFX, VCN etc.) */
#define AMDGV_SCHED_ALLOWED_VF_ASSIGNMENT_ALL  (0xFFFFFFFF)

#define amdgv_sched_default_gfx_time_slice(adapt, num_vf)                                     \
	((num_vf == 1) ? DEFAULT_GFX_TIME_SLICE_1VF : GET_GFX_TIME_SLICE(adapt, num_vf))

#define AUTO_SCHED_PERF_LOG_CSA_OFFSET		0x1A800

enum amdgv_sched_spatial_partition {
	AMDGV_SCHED_SPATIAL_PARTITION_0,
	AMDGV_SCHED_SPATIAL_PARTITION_1,
	AMDGV_SCHED_SPATIAL_PARTITION_2,
	AMDGV_SCHED_SPATIAL_PARTITION_3,
	AMDGV_SCHED_SPATIAL_PARTITION_4,
	AMDGV_SCHED_SPATIAL_PARTITION_5,
	AMDGV_SCHED_SPATIAL_PARTITION_6,
	AMDGV_SCHED_SPATIAL_PARTITION_7,
	AMDGV_SCHED_SPATIAL_PARTITION_MAX,
	AMDGV_SCHED_SPATIAL_PARTITION_ALL,
	AMDGV_SCHED_SPATIAL_PARTITION_INVALID
};

enum amdgv_sched_event_id {
	AMDGV_EVENT_READY_TO_ACCESS_GPU = 1,
	AMDGV_EVENT_IP_DATA_READY = 4,
	AMDGV_EVENT_TEXT_MESSAGE = 255,

	AMDGV_EVENT_REQ_GPU_INIT = 0xef00,
	AMDGV_EVENT_REL_GPU_INIT,
	AMDGV_EVENT_REQ_GPU_FINI,
	AMDGV_EVENT_REL_GPU_FINI,
	AMDGV_EVENT_REQ_GPU_RESET,
	AMDGV_EVENT_LOG_VF_ERROR,
	AMDGV_EVENT_REQ_GPU_INIT_DATA,
	AMDGV_EVENT_READY_TO_RESET,
	AMDGV_EVENT_REQ_GPU_DEBUG,
	AMDGV_EVENT_REL_GPU_DEBUG,

	AMDGV_EVENT_SCHED_FORCE_RESET_VF     = 0xff01,
	AMDGV_EVENT_SCHED_RESET_VF	     = 0xff02,
	AMDGV_EVENT_HW_SCHED_RESET_VF	     = 0xff03,
	AMDGV_EVENT_SCHED_SUSPEND_VF	     = 0xff04,
	AMDGV_EVENT_SCHED_RESUME_VF	     = 0xff05,
	AMDGV_EVENT_SCHED_REMOVE_VF	     = 0xff06,
	AMDGV_EVENT_SCHED_FORCE_RESET_GPU    = 0xff07,
	AMDGV_EVENT_SCHED_STOP_VF	     = 0xff08,
	AMDGV_EVENT_SCHED_INIT_VF_FB	     = 0xff09,
	AMDGV_EVENT_SCHED_RAS_UMC	     = 0xff0a,
	AMDGV_EVENT_SCHED_SUSPEND	     = 0xff0b,
	AMDGV_EVENT_SCHED_RESUME	     = 0xff0c,
	AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC = 0xff0d,
	AMDGV_EVENT_SCHED_GPUMON,
	AMDGV_EVENT_SCHED_SET_VF_ACCESS,
	AMDGV_EVENT_SCHED_MMSCH_GENERAL_NOTIFICATION,
	AMDGV_EVENT_SCHED_PSP_VF_GATE,
	AMDGV_EVENT_SCHED_PSP_VF_CMD_RELAY,
	AMDGV_EVENT_HANDLE_CRASH,
	AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL,
	/* Used for gim live update */
	AMDGV_EVENT_SCHED_SUSPEND_LIVE	     = 0xff20,
	AMDGV_EVENT_SCHED_RESUME_LIVE	     = 0xff21,

	/* Used for diagnosis data Event */
	AMDGV_EVENT_COLLECT_DIAG_DATA     = 0xff40,
	AMDGV_EVENT_ENTER_POWER_SAVING       = 0xff50,
	AMDGV_EVENT_EXIT_POWER_SAVING       = 0xff51,
	/* Used for data poison */
	AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION	    = 0xff60,
	AMDGV_EVENT_SCHED_RAS_FED	        = 0xff61,
	/* HW-LIQUID */
	AMDGV_EVENT_CUR_VF_CTX_EMPTY,
	AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY,
	AMDGV_EVENT_SCHED_UPDATE_MCA_BANKS,
	AMDGV_EVENT_SCHED_GET_TOPOLOGY,
	AMDGV_EVENT_SCHED_RMA,
	AMDGV_EVENT_SCHED_VF_REQ_RAS_ERROR_COUNT,
	AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP,
	AMDGV_EVENT_SCHED_RAS_POISON_CREATION,
	AMDGV_EVENT_INVALID_EVENT = 0xffffffff,
};

enum amdgv_vf_context_state {
	AMDGV_VF_CONTEXT_CLEAR	  = 0,
	AMDGV_VF_CONTEXT_LOADED	  = 1,
	AMDGV_VF_CONTEXT_SAVED	  = 2,
	AMDGV_VF_CONTEXT_ABNORMAL = 3,
};

enum amdgv_ras_fed_src {
	AMDGV_FED_SRC_GC_RLC,
	AMDGV_FED_SRC_VCN,
	AMDGV_FED_SRC_DF_SYNC_FLOOD,
};

struct gim_dev_data;
union amdgv_sched_event_data {
	struct {
		/* pattern to be filled */
		uint8_t pattern;
		/* 0: for vf fb init, 1: for vf fb clear */
		uint8_t flag;
		int    *result;
	} vf_fb_data;
	struct {
		union {
			void *ptr;
			int   val;
			struct {
				enum amdgv_set_vf_opt_type opt_type;
				struct amdgv_vf_option    *opt;
			} vf;
			struct {
				int (*cb)(struct gim_dev_data *, int *, int *, int *);
				void *dev;
				struct {
					int *speed;
					int *width;
					int *max_vf;
				} output; /* caller read data from output section */
			} pcie_confs;
			struct {
				uint8_t *num_err_records;
				struct amdgv_psp_mb_err_record *err_records;
			} fw_err_records;

			struct {
				uint32_t idx_vf;
				uint8_t *num_fw;
				struct amdgv_firmware_info *fws;
			} fw_info;
			struct {
				enum amdgv_pp_pm_policy p_type;
				enum amdgv_gpumon_policy_soc_pstate level;
			} pm_info;
			struct {
				struct amdgv_gpumon_smu_dpm_policy *dpm_policy;
			} pm_policy;
			struct {
				uint8_t *locked;
				enum AMDGV_PP_CLK_DOMAIN clk_domain;
			} clk_locked;
			struct {
				struct amdgv_adapter *dest_adapt;
				 struct amdgv_gpumon_link_topology_info *topology_info;
			} xgmi_node_topology;
			struct {
				enum amdgv_gpumon_xgmi_fb_sharing_mode mode;
				uint8_t *is_sharing_enabled;
				struct amdgv_adapter *dest_adapt;
			} xgmi_fb_sharing_mode_info;
			struct {
				enum amdgv_gpumon_xgmi_fb_sharing_mode mode;
				uint32_t sharing_mask;
			} xgmi_set_fb_sharing_mode;
			struct {
				uint32_t accelerator_partition_profile_index;
			} ap;
			struct {
				enum amdgv_memory_partition_mode memory_partition_mode;
			} mp;
			struct {
				uint32_t spatial_partition_num;
			} sp;
			struct {
				uint32_t idx_config;
				struct amdgv_gpumon_partition_config *partition_config;
			} partition_config_info;
			struct {
				union {
					struct {
						uint64_t rptr;
						uint64_t *wptr;
						uint64_t *avail_count;
						uint64_t *size;
					} get_count;
					struct {
						void *buf;
						uint64_t rptr;
						uint64_t buf_size;
						uint64_t *write_count;
						uint64_t *overflow_count;
						uint64_t *left_size;
					} get_entries;
				};
			} cper;
		};

		int  type;
		int *result; /* if this GPUMON event successed */
	} gpumon_data;
	struct {
		uint32_t vf_access_select;
		bool enable;
	} vf_access_data;
	struct {
		bool enable;
		uint32_t vf_select;
	} psp_vf_gate_data;
	struct {
		void *data;
		uint32_t size;
		uint32_t *result;
	} diag_data;
	struct {
		int error_type;
	} mca_bank;
	struct {
		enum amdgv_ras_fed_src src;
		/* not used for now in gc poison */
		uint32_t idx_vf;
		union {
			struct {
				uint32_t rlc         : 1;
				uint32_t utcl2       : 1;
				uint32_t ge          : 1;
				uint32_t cpc         : 1;
				uint32_t cpf         : 1;
				uint32_t cpg         : 1;
				uint32_t sdma0       : 1;
				uint32_t sdma1       : 1;
				uint32_t gl2c        : 1;
				uint32_t reserved    : 23;
			} gc_rlc_ip_bits;
			uint32_t u32_all;
		} data;
	} fed_data;
	struct {
		uint64_t rptr;
	} cper_vf;
	struct {
		union {
			struct {
				enum amdgv_ras_block block;
			} consumption;
			/* Nothing for creation yet. */
		};
	} poison;
};

enum amdgv_event_status {
	AMDGV_EVENT_STATUS_NORMAL = 0,
	AMDGV_EVENT_STATUS_FINISHED = 1,
};

struct amdgv_sched_event {
	/* the index of vf */
	uint32_t idx_vf;

	/* sched event id */
	enum amdgv_sched_event_id id;

	/* status of the event */
	enum amdgv_event_status status;

	/* sched block id */
	enum amdgv_sched_block sched_block;

	/* received event timestamp */
	uint64_t timestamp;

	/* oss event signal */
	event_t signal;

	/* event data */
	union amdgv_sched_event_data data;
};

/* MM HW scheduler mode. And MM scheduler is not configurable */
#define AMDGV_SCHED_FRAME_LOOP_MODE 0

/* used by manual switch, it is used to record
 * the present switch data of a vf.
 */
struct amdgv_sched_active_vf_entry {
	struct amdgv_list_head list;
	uint32_t	       idx_vf;
	uint64_t	       time_slice;
	uint64_t	       last_time_slice;
	bool		       dummy_vf;
	uint64_t	       start_ts;
	uint64_t	       beyond_time_cycle;
	uint64_t	       init_time;
	uint64_t	       total_time;
	bool		       skip_next_punish;
	uint32_t	       skip_cnt;
};

struct amdgv_sched_world_switch;

struct amdgv_sched_world_switch_funcs {
	int (*init)(struct amdgv_adapter *adapt, struct amdgv_sched_world_switch *world_switch,
		    enum amdgv_sched_mode sched_mode);
	void (*fini)(struct amdgv_adapter *adapt, struct amdgv_sched_world_switch *world_switch);

	int (*start)(struct amdgv_adapter *adapt, struct amdgv_sched_world_switch *world_switch);
	int (*stop)(struct amdgv_adapter *adapt, struct amdgv_sched_world_switch *world_switch);

	int (*add_vf)(struct amdgv_adapter *adapt, struct amdgv_sched_world_switch *world_switch,
		      uint32_t idx_vf);
	int (*remove_vf)(struct amdgv_adapter *adapt, struct amdgv_sched_world_switch *world_switch,
			 uint32_t idx_vf);

	int (*update_time_slice)(struct amdgv_adapter *adapt, struct amdgv_sched_world_switch *world_switch,
				 uint32_t idx_vf);
	int (*set_vf_num)(struct amdgv_adapter *adapt, struct amdgv_sched_world_switch *world_switch,
			  uint32_t num_vf);
};

struct amdgv_sched_block_map {
	uint32_t world_switch_mask;
	uint32_t hw_sched_mask;
};

struct amdgv_sched_vf_info {
	/* the state (unavail, avail, active,
	 * suspend) of vf
	 */
	enum amdgv_sched_state state;

	uint32_t time_slice[AMDGV_SCHED_BLOCK_MAX];
	uint8_t cur_vf_state[AMDGV_MAX_NUM_HW_SCHED];

	uint32_t world_switch_mask;
	uint32_t hw_sched_mask;
	struct amdgv_sched_block_map block_map[AMDGV_SCHED_BLOCK_MAX];
	uint32_t xcc_mask;

	// for partition full access
	bool			  in_full_access;
	enum amdgv_sched_event_id event_id_full_access;
	uint64_t		  start_time_full_access;
	uint64_t		  used_time_full_access;
};

enum self_switch_status {
	DEFAULT_DISABLE = 0,
	TRIGGER_ENABLED = 1,
	TRIGGER_DISABLED = 2,
};

/* world_switch represents a independent world switch scheme
 * of a hardware engine (GFX,UVD,VCE).
 */
struct amdgv_sched_world_switch {
	/* whether world switch is enabled */
	bool enabled;

	/* whether world switch is running */
	bool switch_running;

	/* which logical engine the world switch represents to */
	enum amdgv_sched_block sched_block;
	enum amdgv_sched_mode  sched_mode;

	/* indicate time_quanta options for HW Auto-scheduling */
	uint32_t time_quanta_option;

	/* current active vf */
	uint32_t curr_idx_vf;

	/* the context state of the current active vf */
	enum amdgv_vf_context_state curr_vf_state;
	bool			    use_active_status;

	/* bitfield indicates if a vf has even been INITed */
	uint32_t vf_inited;

	/* the world-switch backend functions */
	const struct amdgv_sched_world_switch_funcs *funcs;

	/* bitfield of allowed VF idx to be assigned to this sched block */
	uint32_t allowed_vf_assignment;

	/*
		Logical to HW scheduler mask. Each bit is index
		to hw_sched_table of an identical scheduler
	*/
	uint32_t hw_sched_mask;
	/* All the VF busy status used in hybrid liquid mode */
	uint32_t vf_busy_status;

	/* bitfield indicates if a VF exhausted its timeslice in hybrid liquid mode */
	uint32_t vf_timeout;

	/* customized minimum time slice of each VF in hybrid liquid mode */
	int hliquid_min_ts;

	int (*bulk_goto_state)(struct amdgv_adapter *adapt, uint32_t target_vf,
			uint32_t hw_sched_mask, uint32_t target_state);

	/* manual switch backend */
	struct {
		/* manual switch lock */
		mutex_t switching_lock;

		/* manual switch timer */
		timer_t timer;

		/* the work thread of world switch */
		thread_t switch_thread;

		/* signal to trigger processing world switch  */
		event_t switch_event;

		/* fairness mode */
		bool fairness_mode;

		/* the binding adapter */
		struct amdgv_adapter *adapt;

		/* active vf list in manual switch */
		struct amdgv_list_head active_vf_list;

		/* active vf entry in manual switch */
		struct amdgv_sched_active_vf_entry array_vf[AMDGV_MAX_VF_SLOT];

		/* handle run time self switch on/off */
		bool self_switch_trigger;

	} manual;
	struct {
		/* handle run time self switch on/off */
		enum self_switch_status self_switch_trigger;
	} auto_sched;
};

enum mcp_ws_auto_run {
	MCP_WS_AUTO_RUN_DISABLED = 0,
	MCP_WS_AUTO_RUN_ENABLED  = 1,
};

struct world_switch_state {
	uint32_t cur_gpu_state;
	uint32_t cur_vf_id;
	uint32_t mode;
	rwsema_t ws_lock;
	int (*goto_state)(struct amdgv_adapter *adapt, uint32_t target_vf,
			  uint32_t hw_sched_id, uint32_t target_state);
};

/* event list entry */
struct amdgv_sched_event_entry {
	struct amdgv_sched_event event;
	struct amdgv_list_head	 list;
};

enum {
	AMDGV_SCHED_EVENT_LIST_0   = 0,
	AMDGV_SCHED_EVENT_LIST_1   = 1,
	AMDGV_SCHED_EVENT_LIST_2   = 2,
	AMDGV_SCHED_EVENT_LIST_3   = 3,
	AMDGV_SCHED_EVENT_LIST_4   = 4,
	AMDGV_SCHED_EVENT_LIST_5   = 5,
	AMDGV_SCHED_EVENT_LIST_MAX = 6,
};

enum amdgv_sched_event_thread_status {
	AMDGV_EVENT_THREAD_IDLE    = 0,
	AMDGV_EVENT_THREAD_BUSY    = 1,
	AMDGV_EVENT_THREAD_WAITING = 2,
};

struct amdgv_sched_event_wrapper {
	struct amdgv_sched_event event;
	struct amdgv_sched_event_entry *entry;
};

struct amdgv_sched_spatial_part {
	// from table
	uint32_t idx_vf_mask;
	uint32_t hw_sched_mask;
	uint32_t xcc_mask;

	// based on assignment
	uint32_t world_switch_mask;
	struct amdgv_sched_world_switch *ws_map[AMDGV_SCHED_BLOCK_MAX];
};

struct amdgv_sched {
	uint32_t gfx_mode;

	/* the array of sched event, used as ring buffer */
	struct amdgv_sched_event_wrapper *event_queue;

	/* rptr and wptr of event ring buffer */
	unsigned char queue_rptr;
	unsigned char queue_wptr;

	/* event ring buffer */
	spin_lock_t queue_lock;

	/* the handle of event process thread */
	thread_t event_thread;

	/* the signal of which the process thread waiting for */
	event_t event;

	/* event process lock */
	spin_lock_t event_thread_lock;
#ifdef WS_RECORD
	/* the array of record entity, used as ring buffer */
	struct amdgv_record_entity *record_queue;

	/* rptr and wptr of record ring buffer */
	uint16_t record_queue_rptr;
	uint16_t record_queue_wptr;

	/* record ring buffer */
	spin_lock_t record_queue_lock;

	/* the handle of record flush thread */
	thread_t record_thread;
#endif
	/* priority event list */
	struct amdgv_list_head event_list[AMDGV_SCHED_EVENT_LIST_MAX];

	int				curr_event_list_idx;
	struct amdgv_sched_event_entry *next_event;

	bool			  in_full_access;
	uint32_t		  idx_vf_full_access;
	enum amdgv_sched_event_id event_id_full_access;
	uint64_t		  start_time_full_access;
	uint32_t		  allow_time_full_access;
	uint64_t		  used_time_full_access;

	/* unrecoverable error flag */
	bool unrecov_err;

	/* Flag to enable parallel hw scheduler switching of logical scheduler */
	bool enable_bulk_goto_state;

	uint32_t num_world_switch;
	uint32_t num_vf_per_gfx_sched;

	/* Global array of logical scheduler instances. Partitions are mapped to groups of logical schedulers */
	struct amdgv_sched_world_switch world_switch[AMDGV_MAX_NUM_WORLD_SWITCH];

	/* State machine for each HW scheduler */
	struct world_switch_state hw_state_machine[AMDGV_MAX_NUM_HW_SCHED];

	/* each entry record the state and time slice of each vf */
	struct amdgv_sched_vf_info array_vf[AMDGV_MAX_VF_SLOT];

	/* Scheduler spatial partition configuration*/
	uint32_t num_spatial_partitions;
	struct amdgv_sched_spatial_part spatial_part[AMDGV_SCHED_SPATIAL_PARTITION_MAX];
	bool enable_per_partition_full_access;
	uint32_t logical_sched_fa_mask;
	bool skip_full_access_timeout_check;

	void (*dump_gpu_state)(struct amdgv_adapter *adapt);
	uint32_t (*get_asic_time_slice)(struct amdgv_adapter *adapt, enum amdgv_sched_block sched_block, uint32_t num_vf);
	uint32_t (*cp_sched_state)(struct amdgv_adapter *adapt, uint32_t idx_vf);
	/* Enable/Disable CG */
	int (*cg_control)(struct amdgv_adapter *adapt, bool enable);
	/* Safe Mode RLC */
	int (*rlc_safe_mode)(struct amdgv_adapter *adapt, bool enable);
	void (*unhalt_gpu_state)(struct amdgv_adapter *adapt, uint32_t hw_sched_id);
	int (*reconfig_mapping_tables)(struct amdgv_adapter *adapt, uint32_t num_vf);
	int (*reconfig_gfx_time_quantion_option)(struct amdgv_adapter *adapt);
	int (*setup_vf_timeslice)(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t timeslice, enum amdgv_sched_block sched_block);
	int (*setup_default_vfs_timeslice)(struct amdgv_adapter *adapt);
	int (*asymmetric_fb_reconfig)(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t fb_size);
};

int amdgv_sched_init(struct amdgv_adapter *adapt);
void amdgv_sched_fini(struct amdgv_adapter *adapt);

int amdgv_sched_queue_remove_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_queue_stop_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_queue_init_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_queue_flr_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_queue_suspend_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_queue_resume_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_queue_suspend(struct amdgv_adapter *adapt);
int amdgv_sched_queue_resume(struct amdgv_adapter *adapt);
int amdgv_sched_queue_suspend_ex(struct amdgv_adapter *adapt, struct amdgv_lock_sched_opt opt);
int amdgv_sched_queue_resume_ex(struct amdgv_adapter *adapt, struct amdgv_lock_sched_opt opt);
int amdgv_sched_queue_force_reset_gpu(struct amdgv_adapter *adapt);

uint32_t amdgv_sched_get_idx_part_mask(struct amdgv_adapter *adapt, uint32_t idx_vf);
uint32_t amdgv_sched_get_vf_mask_by_part(struct amdgv_adapter *adapt, uint32_t idx_part);
uint32_t amdgv_sched_get_world_switch_mask(struct amdgv_adapter *adapt, uint32_t idx_vf);
uint32_t amdgv_sched_get_world_switch_mask_by_sched_block(struct amdgv_adapter *adapt,
							uint32_t idx_vf, enum amdgv_sched_block sched_block);
uint32_t amdgv_sched_get_hw_sched_mask_by_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
uint32_t amdgv_sched_get_hw_sched_mask_by_sched_block(struct amdgv_adapter *adapt, uint32_t idx_vf,
							enum amdgv_sched_block sched_block);
enum amdgv_sched_block amdgv_sched_get_world_switch_by_hw_sched_id(struct amdgv_adapter *adapt,
	uint32_t hw_sched_id, struct amdgv_sched_world_switch **world_switch);
uint32_t amdgv_sched_get_world_switch(struct amdgv_adapter *adapt, uint32_t idx_part,
	enum amdgv_sched_block sched_block, struct amdgv_sched_world_switch **world_switch);
int amdgv_sched_get_xcc_mask_by_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_sched_context_init(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   enum amdgv_sched_block sched_block);
int amdgv_sched_context_load(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   enum amdgv_sched_block sched_block);
int amdgv_sched_context_save(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   enum amdgv_sched_block sched_block);
int amdgv_sched_context_save_all(struct amdgv_adapter *adapt,
				   enum amdgv_sched_block sched_block);
int amdgv_sched_context_switch_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   enum amdgv_sched_block sched_block);
int amdgv_sched_context_switch_to_vf_saved(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   enum amdgv_sched_block sched_block);
int amdgv_sched_context_switch_gfx_to_pf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_context_one_time_loop(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_context_clear_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   enum amdgv_sched_block sched_block);
int amdgv_sched_reset(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     enum amdgv_sched_block sched_block);

int amdgv_sched_start(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_start_auto(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_start_all(struct amdgv_adapter *adapt);
int amdgv_sched_stop(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_stop_all(struct amdgv_adapter *adapt);
int amdgv_sched_add_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_remove_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_clear_timeslice(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_sched_toggle_skip_next_punish(struct amdgv_adapter *adapt, uint32_t idx_vf,
					bool enable);
int amdgv_sched_set_vf_num(struct amdgv_adapter *adapt, uint32_t num_vf);
bool amdgv_sched_is_state_ok(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_sched_update_time_slice(struct amdgv_adapter *adapt, enum amdgv_sched_block sched_block,
				  uint32_t idx_vf);
enum amdgv_sched_state amdgv_sched_get_vf_status(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_sched_queue_event(struct amdgv_adapter *adapt, uint32_t idx_vf,
			    enum amdgv_sched_event_id event_id,
			    enum amdgv_sched_block sched_block);
int amdgv_sched_queue_event_and_wait(struct amdgv_adapter *adapt, uint32_t idx_vf,
				     enum amdgv_sched_event_id event_id,
				     enum amdgv_sched_block sched_block);
int amdgv_sched_queue_event_ex(struct amdgv_adapter *adapt, uint32_t idx_vf,
			       enum amdgv_sched_event_id event_id,
			       enum amdgv_sched_block sched_block,
			       union amdgv_sched_event_data data);
int amdgv_sched_queue_event_and_wait_ex(struct amdgv_adapter *adapt, uint32_t idx_vf,
					enum amdgv_sched_event_id event_id,
					enum amdgv_sched_block sched_block,
					union amdgv_sched_event_data data);

int amdgv_sched_queue_event_no_signal(struct amdgv_adapter *adapt, uint32_t idx_vf,
			    enum amdgv_sched_event_id event_id,
			    enum amdgv_sched_block sched_block);
int amdgv_sched_queue_event_ex_no_signal(struct amdgv_adapter *adapt, uint32_t idx_vf,
			       enum amdgv_sched_event_id event_id,
			       enum amdgv_sched_block sched_block,
			       union amdgv_sched_event_data data);

int amdgv_sched_world_switch_signal_vf_idle(struct amdgv_adapter *adapt);

int amdgv_sched_world_switch_signal_vf_idle(struct amdgv_adapter *adapt);

int amdgv_sched_world_switch_start_pf(struct amdgv_adapter *adapt);
int amdgv_sched_init_pf_state(struct amdgv_adapter *adapt);
int amdgv_sched_init_pf_state_early(struct amdgv_adapter *adapt);
int amdgv_sched_init_pf_state_late(struct amdgv_adapter *adapt);

void amdgv_sched_print_vfs_running_time(struct amdgv_adapter *adapt);

const char *amdgv_sched_block_to_name(enum amdgv_sched_block sched_block);
const char *amdgv_hw_sched_id_to_name(struct amdgv_adapter *adapt, uint32_t hw_sched_id);
int amdgv_sched_export_unprocessed_event_size(struct amdgv_adapter *adapt,
					      uint32_t *unprocessed_event_count);

enum amdgv_live_info_status amdgv_sched_export_unprocessed_event(struct amdgv_adapter *adapt, struct amdgv_live_info_unprocessed_event *event_info);
enum amdgv_live_info_status amdgv_sched_import_unprocessed_event(struct amdgv_adapter *adapt, struct amdgv_live_info_unprocessed_event *event_info);

int amdgv_sched_lock(struct amdgv_adapter *adapt);
int amdgv_sched_park(struct amdgv_adapter *adapt);
int amdgv_sched_unpark(struct amdgv_adapter *adapt);

int amdgv_sched_set_time_quanta_option(struct amdgv_adapter *adapt,
				       enum amdgv_sched_block sched_block, uint32_t opt);

int amdgv_sched_get_time_quanta_option(struct amdgv_adapter *adapt,
				       enum amdgv_sched_block sched_block, uint32_t *opt);

uint32_t amdgv_sched_bandwidth_to_time_slice(struct amdgv_adapter *adapt,
					     enum amdgv_mm_engine engine_id,
					     uint32_t bandwidth);

uint32_t amdgv_sched_time_slice_to_bandwidth(struct amdgv_adapter *adapt,
					     enum amdgv_mm_engine engine_id,
					     uint32_t time_slice);

void amdgv_sched_setup_self_switch(struct amdgv_adapter *adapt, bool enable);

uint32_t amdgv_sched_get_asic_time_slice(struct amdgv_adapter *adapt, enum amdgv_sched_block sched_block, uint32_t num_vf);

int amdgv_sched_set_time_slice(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t time_slice,
							   enum amdgv_sched_block sched_block);
int amdgv_sched_reconfig_mapping_tables(struct amdgv_adapter *adapt, uint32_t num_vf);
int amdgv_sched_part_mapping_init(struct amdgv_adapter *adapt);

int amdgv_sched_get_hliquid_min_ts(struct amdgv_adapter *adapt);
int amdgv_sched_set_hliquid_min_ts(struct amdgv_adapter *adapt, int hliquid_min_ts);
int amdgv_sched_set_auto_sched_debug_log(struct amdgv_adapter *adapt, enum amdgv_auto_sched_log_op op, bool enable);
int amdgv_sched_read_perf_log_data(struct amdgv_adapter *adapt);
#ifdef WS_RECORD
void amdgv_sched_debug_dump_data_flush(struct amdgv_adapter *adapt);
#endif
enum amdgv_live_info_status amdgv_sched_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_sched *sched_info);
enum amdgv_live_info_status amdgv_sched_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_sched *sched_info);
int amdgv_sched_setup_vf_timeslice(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t timeslice, enum amdgv_sched_block sched_block);
int amdgv_sched_setup_default_vfs_timeslice(struct amdgv_adapter *adapt);
void amdgv_sched_remove_stale_events_after_wgr(struct amdgv_adapter *adapt);
int amdgv_sched_get_sched_mode(struct amdgv_adapter *adapt, enum amdgv_sched_block sched_block, enum amdgv_sched_mode *sched_mode);
void amdgv_sched_set_unrecov_err(struct amdgv_adapter *adapt);
void amdgv_sched_clear_unrecov_err(struct amdgv_adapter *adapt);
bool amdgv_sched_is_unrecov_err(struct amdgv_adapter *adapt);
#endif
