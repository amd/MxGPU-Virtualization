/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

 #ifndef AMDGV_MES_H
 #define AMDGV_MES_H

 #include "amdgv_gfx.h"
#include "amdgv_oss_wrapper.h"


#define AMDGV_MES_MAX_COMPUTE_PIPES        8
#define AMDGV_MES_MAX_GFX_PIPES            2
#define AMDGV_MES_MAX_SDMA_PIPES           2

#define AMDGV_MES_PROC_CTX_SIZE 0x1000 /* one page area */
#define AMDGV_MES_GANG_CTX_SIZE 0x1000 /* one page area */

#define AMDGV_MES_RING_FRAME_SIZE 1024
#define AMDGV_MES_RING_FRAME_NUMBER 256

#define AMDGV_MES_NUM_REG_SEGMENTS 5

enum amdgv_mes_priority_level {
	AMDGV_MES_PRIORITY_LEVEL_LOW       = 0,
	AMDGV_MES_PRIORITY_LEVEL_NORMAL    = 1,
	AMDGV_MES_PRIORITY_LEVEL_MEDIUM    = 2,
	AMDGV_MES_PRIORITY_LEVEL_HIGH      = 3,
	AMDGV_MES_PRIORITY_LEVEL_REALTIME  = 4,
	AMDGV_MES_PRIORITY_NUM_LEVELS
};

enum amdgv_mes_pipe {
	AMDGV_MES_SCHED_PIPE = 0,
	AMDGV_MES_KIQ_PIPE,
	AMDGV_MAX_MES_PIPES = 2,
};

#define AMDGV_MAX_MES_INST_PIPES \
	(AMDGV_MAX_MES_PIPES * AMDGV_MAX_GC_INSTANCES)

#define AMDGV_MES_INST(xcc_id, pipe) \
    ((xcc_id * AMDGV_MAX_MES_PIPES) + pipe)

#define amdgv_mes_kiq_hw_init(adapt, xcc_id) \
	(adapt)->mes.kiq_hw_init((adapt), (xcc_id))
#define amdgv_mes_kiq_hw_fini(adapt, xcc_id) \
	(adapt)->mes.kiq_hw_fini((adapt), (xcc_id))

struct amdgv_mes {
    struct amdgv_adapter *adapt;
	uint32_t   kiq_version;
	uint32_t   sched_version;
    struct amdgv_ring              ring[AMDGV_MAX_MES_INST_PIPES];
	uint32_t   total_max_queue;

    /* MES context */
    uint64_t  default_process_quantum;
	uint64_t  default_gang_quantum;

    uint32_t  vmid_mask_gfxhub;
	uint32_t  vmid_mask_mmhub;
	uint32_t  gfx_hqd_mask[AMDGV_MES_MAX_GFX_PIPES];
	uint32_t  compute_hqd_mask[AMDGV_MES_MAX_COMPUTE_PIPES];
	uint32_t  sdma_hqd_mask[AMDGV_MES_MAX_SDMA_PIPES];
	uint32_t  aggregated_doorbells[AMDGV_MES_PRIORITY_NUM_LEVELS];
	void      *mqd_backup[AMDGV_MAX_MES_INST_PIPES];
	uint64_t  mqd_backup_gpu_addr[AMDGV_MAX_MES_INST_PIPES];
	uint64_t  *mqd_backup_ptr[AMDGV_MAX_MES_INST_PIPES];

    uint32_t  sch_ctx_offs[AMDGV_MAX_MES_INST_PIPES];
	uint64_t  sch_ctx_gpu_addr[AMDGV_MAX_MES_INST_PIPES];
	uint64_t  *sch_ctx_ptr[AMDGV_MAX_MES_INST_PIPES];
	uint32_t  query_status_fence_offs[AMDGV_MAX_MES_INST_PIPES];
	uint64_t  query_status_fence_gpu_addr[AMDGV_MAX_MES_INST_PIPES];
	uint64_t  *query_status_fence_ptr[AMDGV_MAX_MES_INST_PIPES];

	int       (*kiq_hw_init)(struct amdgv_adapter *adapt, uint32_t xcc_id);
	int       (*kiq_hw_fini)(struct amdgv_adapter *adapt, uint32_t xcc_id);

    struct amdgv_mes_funcs *funcs;

    /* cooperative dispatch */
	bool      enable_coop_mode;
	bool      enable_legacy_queue_map;
	int       master_xcc_ids[AMDGV_MAX_MES_INST_PIPES];
};

struct mes_map_legacy_queue_input {
	uint32_t    xcc_id;
	uint32_t    queue_type;
	uint32_t    doorbell_offset;
	uint32_t    pipe_id;
	uint32_t    queue_id;
	uint64_t    mqd_addr;
	uint64_t    wptr_addr;
};

struct mes_unmap_legacy_queue_input {
	uint32_t                           xcc_id;
	enum amdgv_unmap_queues_action     action;
	uint32_t                           queue_type;
	uint32_t                           doorbell_offset;
	uint32_t                           pipe_id;
	uint32_t                           queue_id;
	uint64_t                           trail_fence_addr;
	uint64_t                           trail_fence_data;
};

enum mes_misc_opcode {
	MES_MISC_OP_WRITE_REG,
	MES_MISC_OP_READ_REG,
	MES_MISC_OP_WRM_REG_WAIT,
	MES_MISC_OP_WRM_REG_WR_WAIT,
	MES_MISC_OP_SET_SHADER_DEBUGGER,
	MES_MISC_OP_CHANGE_CONFIG,
};

struct mes_reset_queue_input {
	uint32_t	xcc_id;
	uint32_t	queue_type;
	uint32_t	doorbell_offset;
	bool    	use_mmio;
	uint32_t	me_id;
	uint32_t	pipe_id;
	uint32_t	queue_id;
	uint64_t	mqd_addr;
	uint64_t	wptr_addr;
	uint32_t	vmid;
	bool    	legacy_gfx;
	bool    	is_kq;
};

struct mes_misc_op_input {
	uint32_t                 xcc_id;
	enum mes_misc_opcode     op;

	union {
		struct {
			uint32_t                  reg_offset;
			uint64_t                  buffer_addr;
		} read_reg;

		struct {
			uint32_t                  reg_offset;
			uint32_t                  reg_value;
		} write_reg;

		struct {
			uint32_t                   ref;
			uint32_t                   mask;
			uint32_t                   reg0;
			uint32_t                   reg1;
		} wrm_reg;

		struct {
			uint64_t process_context_addr;
			union {
				struct {
					uint32_t single_memop : 1;
					uint32_t single_alu_op : 1;
					uint32_t reserved: 29;
					uint32_t process_ctx_flush: 1;
				};
				uint32_t u32all;
			} flags;
			uint32_t spi_gdbg_per_vmid_cntl;
			uint32_t tcp_watch_cntl[4];
			uint32_t trap_en;
		} set_shader_debugger;

		struct {
			union {
				struct {
					uint32_t limit_single_process : 1;
					uint32_t enable_hws_logging_buffer : 1;
					uint32_t reserved : 30;
				};
				uint32_t all;
			} option;
			struct {
				uint32_t tdr_level;
				uint32_t tdr_delay;
			} tdr_config;
		} change_config;
	};
};

struct mes_query_status_input {
	uint32_t    xcc_id;
	uint64_t    gpu_addr;
	uint32_t    fence_id;
};

struct amdgv_mes_funcs {
    int (*map_legacy_queue)(struct amdgv_mes *mes,
        struct mes_map_legacy_queue_input *input);

    int (*unmap_legacy_queue)(struct amdgv_mes *mes,
        struct mes_unmap_legacy_queue_input *input);

    int (*misc_op)(struct amdgv_mes *mes,
            struct mes_misc_op_input *input);

	int (*query_status)(struct amdgv_mes* mes,
			struct mes_query_status_input *input);

	int (*reset_hw_queue)(struct amdgv_mes *mes,
			struct mes_reset_queue_input *input);
};

int amdgv_mes_init(struct amdgv_adapter *adapt);
int amdgv_mes_fini(struct amdgv_adapter *adapt);
int amdgv_mes_map_legacy_queue(struct amdgv_adapter *adapt,
    								struct amdgv_ring *ring, uint32_t xcc_id);
int amdgv_mes_unmap_legacy_queue(struct amdgv_adapter *adapt,
									struct amdgv_ring *ring,
									enum amdgv_unmap_queues_action action,
									uint64_t gpu_addr, uint64_t seq, uint32_t xcc_id);
int amdgv_mes_reset_legacy_queue(struct amdgv_adapter *adapt,
									struct amdgv_ring *ring,
									uint32_t vmid,
									bool use_mmio_doorbell,
									uint32_t xcc_id);

#endif
