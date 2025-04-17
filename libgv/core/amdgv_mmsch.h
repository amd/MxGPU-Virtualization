/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_MMSCH_H
#define AMDGV_MMSCH_H

#include "amdgv_api.h"

/* NOTE: In MMSCH, index 0 is PF index */
#define AMDGV_MMSCH_PF_IDX 0

#define CONVERT_TO_MMSCH_IDX_VF(idx_vf) ((uint32_t)(idx_vf + 1) % (AMDGV_PF_IDX + 1))
#define CONVERT_TO_LIBGV_IDX_VF(idx_vf) ((uint32_t)(idx_vf - 1) % (AMDGV_PF_IDX + 1))

/* reserve 4 engine slots in LibGV's header */
#define AMDGV_MMSCH_MAX_VCN_ENGINE 4

/* all FB access in MMSCH must be 256byte aligned */
#define AMDGV_MMSCH_FB_ALIGNMENT 256

/* size of SRAM in byte */
#define AMDGV_MMSCH_SRAM_DUMP_SIZE 4096

#define MMSCH_HV_CMD__SET_CMD_BUFFER_ADDR_HI  0x00000001
#define MMSCH_HV_CMD__SET_CMD_BUFFER_ADDR_LO  0x00000002
#define MMSCH_HV_CMD__NEW_CMD_READY           0x00000003

#define MMSCH_HV_RESP__DONE                   0x00000001
#define MMSCH_HV_RESP__UNKNOWN_CMD            0x00000002
#define MMSCH_HV_RESP__NOT_READY              0x00000003

/* command buffer type */
enum amdgv_mmsch_cmd_buffer_type {
	AMDGV_MMSCH_CMD_TYPE_INVALID = 0,
	AMDGV_MMSCH_CMD_TYPE_BANDWIDTH_CONFIG = 0x01,
	AMDGV_MMSCH_CMD_TYPE_RB_DECOUPLE      = 0x02,
	AMDGV_MMSCH_CMD_TYPE_MAX,

	AMDGV_MMSCH_CMD_TYPE_SRAM_DUMP = AMDGV_MMSCH_CMD_TYPE_MAX,
};


#pragma pack(push, 1)

/* command buffer */
struct amdgv_mmsch_cmd_buffer {
	uint32_t cmd_type;
	uint32_t cmd_info_addr_hi;
	uint32_t cmd_info_addr_lo;
	uint32_t cmd_info_size;
	uint32_t cmd_status;
};

/* data struct submitted in command buffer */
struct amdgv_mmsch_vcn_job_limit {
	uint32_t decode_max_dimension_pixels;  // e.g. 1920 pixels
	uint32_t decode_max_frame_pixels;      // e.g. 2073600 for 1920x1080 or 1080x1920
	uint32_t encode_max_dimension_pixels;  // e.g. 1920 pixels
	uint32_t encode_max_frame_pixels;      // e.g. 2073600 for 1920x1080 or 1080x1920
};

struct amdgv_mmsch_vcn_vf_bandwidth {
	struct amdgv_mmsch_vcn_job_limit job_limit;
	uint32_t partition_usecs; // e.g. 25000 usecs
};

struct amdgv_mmsch_vcn_vf_status {
	union {
		struct {
			uint32_t job_limit_not_supported :  1;
			uint32_t job_ignored             :  1;
			uint32_t reserved                : 30;
		};
		uint32_t allbits;
	};
};

struct amdgv_mmsch_bandwidth_config {
	// Input
	union {
		struct {
			uint32_t time_partition_enable : 1;
			uint32_t job_limit_enable : 1;
			uint32_t reserved : 30;
		};
		uint32_t allbits;
	} flags;
	uint32_t vcn_enabled_vfs[AMDGV_MMSCH_MAX_VCN_ENGINE]; // bit0: PF, bit1: VF0, …
	struct amdgv_mmsch_vcn_vf_bandwidth vcn_vf_bandwidth[AMDGV_MMSCH_MAX_VCN_ENGINE][AMDGV_MAX_VF_SLOT]; // 0: PF, 1: VF0, …

	// Output
	struct amdgv_mmsch_vcn_vf_status vcn_vf_status[AMDGV_MMSCH_MAX_VCN_ENGINE][AMDGV_MAX_VF_SLOT]; // 0: PF, 1: VF0, …
	uint32_t output_gpu_timestamp_hi;
	uint32_t output_gpu_timestamp_lo;
};

struct amdgv_mmsch_rb_decouple {
	uint32_t enabled_vfs;
	uint32_t av1_disabled;
	uint32_t reserved[62];
};

#pragma pack(pop)


struct amdgv_mmsch_funcs {
	/* feature enablement check */
	int (*check_enabled_features)(struct amdgv_adapter *adapt);

	/* handshake */
	int (*send_mailbox)(struct amdgv_adapter *adapt, uint32_t cmd, uint32_t data);
	int (*notify_mmsch)(struct amdgv_adapter *adapt);

	/* bandwidth config */
	int (*get_default_bandwidth_config)(struct amdgv_adapter *adapt, struct amdgv_mmsch_bandwidth_config *bw_cfg);
	int (*read_bandwidth_config)(struct amdgv_adapter *adapt);
	int (*write_bandwidth_config)(struct amdgv_adapter *adapt, uint32_t mmsch_idx_vf);
	int (*clear_bandwidth_config_vf_status)(struct amdgv_adapter *adapt, uint32_t mmsch_idx_vf);
	int (*reconfigure_vf_assignment)(struct amdgv_adapter *adapt);

	/* rb decouple */
	int (*write_rb_decouple)(struct amdgv_adapter *adapt);
};

struct amdgv_mmsch {
	/* ASIC info */
	uint32_t max_vcn_engine;

	/* mmsch cmd buffer */
	struct amdgv_memmgr_mem *cmd_buffer_mem;

	/* mmsch funtionality enable flag */
	union {
		struct {
			uint32_t cmd_buffer : 1;
			uint32_t support_shutdown_cmd : 1;
			uint32_t reserved : 30;
		};
		uint32_t allbits;
	} is_feature_enabled;

	/* bandwidth config */
	struct amdgv_memmgr_mem *bandwidth_config_mem;
	uint32_t bandwidth_config_size;
	struct amdgv_mmsch_bandwidth_config bandwidth_config;

	/* rb decouple */
	uint32_t default_rb_decouple_enabled_vfs;
	uint32_t default_rb_decouple_av1_disabled;
	struct amdgv_memmgr_mem *rb_decouple_mem;
	struct amdgv_mmsch_rb_decouple rb_decouple;

	/* sram dump */
	struct amdgv_memmgr_mem *sram_dump_mem;

	/* private ASIC related mmsch funcs, do not use outside amdgv_mmsch.c */
	const struct amdgv_mmsch_funcs *mmsch_funcs;
};

struct amdgv_sched_world_switch;

/* helper function */
int amdgv_mmsch_vcn_engine_idx_to_world_switch(struct amdgv_adapter *adapt,
				uint32_t vcn_engine_idx, struct amdgv_sched_world_switch **world_switch);
int amdgv_mmsch_sched_block_to_vcn_engine_idx(struct amdgv_adapter *adapt,
				enum amdgv_sched_block sched_block, uint32_t *vcn_engine_idx);

/* set MMSCH cmd buffer before SRIOV is enabled */
int amdgv_mmsch_set_cmd_buffer_address(struct amdgv_adapter *adapt, uint64_t cmd_gpu_addr);

/* submit MMSCH cmd at run time to configure MM engines */
int amdgv_mmsch_submit_cmd(struct amdgv_adapter *adapt,
				enum amdgv_mmsch_cmd_buffer_type type, uint32_t libgv_idx_vf);

/* a wrapper function for all mmsch command submissions during VF init/reinit */
int amdgv_mmsch_config_vf(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf);

/* a wrapper function for all mmsch command submissions !after! VF init */
int amdgv_mmsch_reconfig_vf(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf,
				struct amd_sriov_msg_vf2pf_info *vf2pf_msg);

/* a wrapper function for all mmsch command submissions !before! VF init data */
int amdgv_mmsch_preconfig_vf(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf);

/* populate default bandwidth config accoriding to common config and ASIC config */
int amdgv_mmsch_get_default_bandwidth_config(struct amdgv_adapter *adapt,
				struct amdgv_mmsch_bandwidth_config *bw_cfg);

/* clear output status bit for reinit */
int amdgv_mmsch_clear_bandwidth_config_vf_status(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf);
int amdgv_mmsch_clear_rb_decouple_status(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf);

/* read output section of bandwidth config from framebuffer */
int amdgv_mmsch_read_bandwidth_config(struct amdgv_adapter *adapt);

/* read all MMSCH output after receive an interrupt */
int amdgv_mmsch_read_all_output(struct amdgv_adapter *adapt, uint32_t idx_vf, enum amdgv_sched_block sched_block);

/* update PF2VF message */
int amdgv_mmsch_update_pf2vf_msg(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf);

/* debug function */
int amdgv_mmsch_dump_bandwidth_config(struct amdgv_adapter *adapt);

int amdgv_mmsch_sram_dump(struct amdgv_adapter *adapt);

/* check MMSCH features such as cmd_buffer */
int amdgv_mmsch_check_enabled_features(struct amdgv_adapter *adapt);

/* reconfigure VF assignment according to VF number dynamically */
int amdgv_mmsch_reconfigure_vf_assignment(struct amdgv_adapter *adapt);

#endif
