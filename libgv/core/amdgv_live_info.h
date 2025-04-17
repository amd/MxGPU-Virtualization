/*
 * Copyright (c) 2020-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_LIVE_INFO_H
#define AMDGV_LIVE_INFO_H

#include "amdgv_basetypes.h"
#include "amdgv_gpumon.h"

extern struct amdgv_adapter *adapt;

#define LIVE_INFO_VERSION_MAJOR		2
#define LIVE_INFO_VERSION_MINOR		1

#define LIVE_INFO_VERSION_MAJOR_MASK	0xffff0000
#define LIVE_INFO_VERSION_MAJOR_SHIFT	0x10
#define LIVE_INFO_VERSION_MINOR_MASK	0x0000ffff
#define LIVE_INFO_VERSION_MINOR_SHIFT	0

#define LIVE_INFO_HEADER_VERSION \
	((LIVE_INFO_VERSION_MAJOR << LIVE_INFO_VERSION_MAJOR_SHIFT) |\
	 (LIVE_INFO_VERSION_MINOR << LIVE_INFO_VERSION_MINOR_SHIFT))

#define LIVE_INFO_GET_HEADER_VERSION(major, minor) \
	((major << LIVE_INFO_VERSION_MAJOR_SHIFT) |\
	 (minor << LIVE_INFO_VERSION_MINOR_SHIFT))

#define LIVE_INFO_UNKNOWN_TABLE_VERSION	0

#define MEM_ALLOCS_MAX 128
#define VBIOS_IMAGE_BYTE_SIZE 0x10000
#define AMDGV_MAX_VF_LIVE 9
#define AMDGV_LIVE_MAX_XCD_NUM 16
#define AMDGV_LIVE_MAX_RAS_BLOCK 32

/* Use pcie config space 0x108 to store the live info hash */
#define LIVE_INFO_HASH_ADDR 0x108

#define LIVE_INFO_DELAY_CHECK_USEC 500000

enum amdgv_live_info_data {
	/* critical state*/
	AMDGV_LIVE_INFO_DATA__CRITICAL_STATE = 0,
	/* static data op */
	AMDGV_LIVE_INFO_DATA__VBIOS,
	AMDGV_LIVE_INFO_DATA__IP_DISCOVERY,
	AMDGV_LIVE_INFO_DATA__MEMMGR,
	AMDGV_LIVE_INFO_DATA__GPUMON,
	/* export/import params after adapter created and before sw_init begins */
	AMDGV_LIVE_INFO_DATA__MODULE_PARAM_PRE,
	/* export/import params after sw_init*/
	AMDGV_LIVE_INFO_DATA__MODULE_PARAM_POST,
	/* import ecc bad page info after memmgr info imported, cause reserved pages are in the mem list */
	AMDGV_LIVE_INFO_DATA__ECC,
	AMDGV_LIVE_INFO_DATA__GPUIOV,
	/* dynamic data op */
	AMDGV_LIVE_INFO_DATA__XGMI,
	AMDGV_LIVE_INFO_DATA__FW_INFO,
	AMDGV_LIVE_INFO_DATA__POWERPLAY,
	AMDGV_LIVE_INFO_DATA__WORLD_SWITCH,
	AMDGV_LIVE_INFO_DATA__WS_STATE,
	AMDGV_LIVE_INFO_DATA__UNPROCESSED_EVENT,
	AMDGV_LIVE_INFO_DATA__PSP,
	AMDGV_LIVE_INFO_DATA__FFBM,
	AMDGV_LIVE_INFO_DATA__VF,
	AMDGV_LIVE_INFO_DATA__SCHED,
	AMDGV_LIVE_INFO_DATA__VF_EXTEND,
	AMDGV_LIVE_INFO_DATA__RING,
	AMDGV_LIVE_INFO_DATA__MCA,
	AMDGV_LIVE_INFO_DATA__END,
};

/* IDs for memory allocation
 * Note: if new ID is added, remember to also update
 * amdgv_mem_id_name function
 */
enum amdgv_mem_id {
	/* PSP */
	MEM_PSP_CMD_BUF_0            = 0x0,
	MEM_PSP_CMD_BUF_1            = 0x1,
	MEM_PSP_CMD_BUF_2            = 0x2,
	MEM_PSP_CMD_BUF_3            = 0x3,
	MEM_PSP_CMD_BUF_4            = 0x4,
	MEM_PSP_CMD_BUF_5            = 0x5,
	MEM_PSP_CMD_BUF_6            = 0x6,
	MEM_PSP_CMD_BUF_7            = 0x7,
	MEM_PSP_CMD_BUF_8            = 0x8,
	MEM_PSP_CMD_BUF_9            = 0x9,
	MEM_PSP_CMD_BUF_10           = 0xa,
	MEM_PSP_CMD_BUF_11           = 0xb,
	MEM_PSP_CMD_BUF_12           = 0xc,
	MEM_PSP_CMD_BUF_13           = 0xd,
	MEM_PSP_CMD_BUF_14           = 0xe,
	MEM_PSP_CMD_BUF_15           = 0xf,
	MEM_PSP_RING                 = 0x10,
	MEM_PSP_FENCE                = 0x11,
	MEM_PSP_PRIVATE              = 0x12,
	MEM_PSP_RAS                  = 0x13,
	MEM_PSP_TMR                  = 0x14,
	MEM_PSP_XGMI                 = 0x15,
	MEM_PSP_VBFLASH              = 0x16,
	MEM_PSP_DUMMY                = 0x17,
	/* SMU */
	MEM_SMC_PPTABLE              = 0x100,
	MEM_SMC_WATERMARK_TABLE      = 0x101,
	MEM_SMC_METRICS_TABLE        = 0x102,
	MEM_SMU_CONFIG_TABLE         = 0x103,
	MEM_SMC_OVERRIDE_TABLE       = 0x104,
	MEM_SMC_I2C_TABLE            = 0x105,
	MEM_SMC_ACTIVITY_TABLE       = 0x106,
	MEM_SMC_PM_STATUS_TABLE      = 0x107,
	MEM_SMU_ECC_INFO_TABLE       = 0x108,
	MEM_SMU_DRIVER_TABLE         = 0x109,
	MEM_SMU_TOOL_TABLE           = 0x10a,
	/* GPUIOV */
	MEM_GPUIOV_CSA               = 0x200,
	MEM_GPUIOV_SCHED_LOG         = 0x201,
	MEM_GPUIOV_SCHED_CFG_DESC	 = 0x202,
	/* IRGMGR */
	MEM_IRQMGR_IH_RING           = 0x300,
	/* ECC */
	MEM_ECC_BAD_PAGE             = 0x400,
	/* MMSCH */
	MEM_MMSCH_CMD_BUFFER         = 0x500,
	MEM_MMSCH_BW_CFG             = 0x501,
	MEM_MMSCH_RB_DECOUPLE        = 0x502,
	MEM_MMSCH_SRAM_DUMP          = 0x5FF,
	/* GFX */
	MEM_GFX_WB                   = 0x601,
	MEM_GFX_EOP                  = 0x602,
	MEM_GFX_MQD                  = 0x603,
	MEM_GFX_IB                   = 0x604,

	MEM_KIQ_RING                 = 0x610,
	MEM_KIQ_EOP                  = 0x611,
	MEM_KIQ_MQD                  = 0x612,

	MEM_COMPUTE0_RING            = 0x620,
	MEM_COMPUTE1_RING            = 0x621,
	MEM_COMPUTE2_RING            = 0x622,
	MEM_COMPUTE3_RING            = 0x623,

	MEM_COMPUTE0_MQD             = 0x628,
	MEM_COMPUTE1_MQD             = 0x629,
	MEM_COMPUTE2_MQD             = 0x62a,
	MEM_COMPUTE3_MQD             = 0x62b,

	MEM_SDMA0_RING               = 0x630,
	MEM_SDMA1_RING               = 0x631,

	MEM_SDMA0_MQD                = 0x634,
	MEM_SDMA1_MQD                = 0x635,

	/* FW */
	MEM_PFP_FW                   = 0x700,
	MEM_ME_FW                    = 0x701,
	MEM_MEC1_FW                  = 0x702,
	MEM_MES_P0_DATA_FW           = 0x703,
	MEM_MES_P0_UCODE_FW          = 0x704,
	MEM_MES_P1_DATA_FW           = 0x705,
	MEM_MES_P1_UCODE_FW          = 0x706,
	MEM_RS64_PFP_UCODE           = 0x707,
	MEM_RS64_PFP_DATA            = 0x708,
	MEM_RS64_ME_UCODE            = 0x709,
	MEM_RS64_ME_DATA             = 0x70a,
	MEM_RS64_MEC1_UCODE          = 0x70b,
	MEM_RS64_MEC1_DATA           = 0x70c,

	/* Live Migration */
	MEM_MIGRATION_PSP_STATIC_DATA  = 0x800,
	MEM_MIGRATION_PSP_DYNAMIC_DATA = 0x801,
	MEM_GC_DIRTY_BIT_PLANE         = 0X802,

	MEM_ID_UNKNOWN               = 0xffffffff,
};

/* memory block owner */
enum amdgv_mem_owner {
	MEM_OWNER_PF        = 0x10,
	MEM_OWNER_GPU       = 0x20,
	MEM_OWNER_SYS       = 0x30,
};

#pragma pack(push, 1)

struct live_info_table_header {
	uint32_t structure_size;
	uint32_t table_version;
	uint8_t reserved[8]; // 0x10 align
};

struct amdgv_live_info_param {
	struct live_info_table_header header;
	uint32_t num_vf;
	uint32_t fw_load_type;
	uint32_t log_level;
	uint32_t log_mask;
	uint64_t flags;
	uint32_t gfx_sched_mode;
	uint32_t allow_time_full_access;
	uint32_t perf_mon_enable;
	uint32_t debug_mode;
	uint8_t customized_vf_config_mode;
	uint32_t accelerator_partition_mode;
	uint32_t memory_partition_mode;
	uint32_t partition_full_access_enable;
	uint32_t bad_page_record_threshold;
	uint32_t max_cper_count;
	int32_t ras_vf_telemetry_policy;
	uint8_t reserved[47]; // 0x80 align
};

struct amdgv_live_info_vbios {
	struct live_info_table_header header;
	uint8_t image[VBIOS_IMAGE_BYTE_SIZE];
	uint32_t image_size;
	uint32_t sec_version;
	uint8_t reserved[104]; //0x10080 align
};

struct amdgv_live_info_fw_info {
	struct live_info_table_header header;
	uint32_t fw_info_smu;
	uint32_t fw_info_cp_ce;
	uint32_t fw_info_cp_pfp;
	uint32_t fw_info_cp_me;
	uint32_t fw_info_cp_mec_jt1;
	uint32_t fw_info_cp_mec_jt2;
	uint32_t fw_info_cp_mec1;
	uint32_t fw_info_cp_mec2;
	uint32_t fw_info_rlc;
	uint32_t fw_info_sdma0;
	uint32_t fw_info_sdma1;
	uint32_t fw_info_sdma2;
	uint32_t fw_info_sdma3;
	uint32_t fw_info_sdma4;
	uint32_t fw_info_sdma5;
	uint32_t fw_info_sdma6;
	uint32_t fw_info_sdma7;
	uint32_t fw_info_vcn;
	uint32_t fw_info_uvd;
	uint32_t fw_info_vce;
	uint32_t fw_info_isp;
	uint32_t fw_info_dmcu_eram;
	uint32_t fw_info_dmcu_isr;
	uint32_t fw_info_rlc_restore_list_gpm_mem;
	uint32_t fw_info_rlc_restore_list_srm_mem;
	uint32_t fw_info_rlc_restore_list_cntl;
	uint32_t fw_info_rlc_v;
	uint32_t fw_info_mmsch;
	uint32_t fw_info_psp_sysdrv;
	uint32_t fw_info_psp_sosdrv;
	uint32_t fw_info_psp_toc;
	uint32_t fw_info_psp_keydb;
	uint32_t fw_info_dfc_fw;
	uint32_t fw_info_psp_spl;
	uint32_t smu_fw_version;
	uint8_t reserved[100]; // 0x100 align
};

/* Memory allocation node info */
struct amdgv_live_info_memmgr_mem {
	enum amdgv_mem_id id;            /* id to track mem block user */
	enum amdgv_mem_owner owner;      /* mem owner */
	uint64_t alloc_off;              /* offset to the end of the alloc */
	uint64_t len;                    /* length of the allocation */
	uint64_t align;                  /* alignment of the allocation */
};

struct amdgv_live_info_memmgr {
	struct live_info_table_header header;
	// mc info
	uint64_t mc_fb_loc_addr;
	uint64_t mc_fb_top_addr;
	uint64_t mc_sys_loc_addr;
	uint64_t mc_sys_top_addr;
	uint64_t mc_agp_loc_addr;
	uint64_t mc_agp_top_addr;
	// memmgr pf
	uint64_t memmgr_pf_mc_base;
	uint64_t memmgr_pf_offset;
	uint64_t memmgr_pf_size;
	uint64_t memmgr_pf_align;
	uint64_t memmgr_pf_tom;
	uint8_t memmgr_pf_down;
	// memmgr gpu
	uint64_t memmgr_gpu_mc_base;
	uint64_t memmgr_gpu_offset;
	uint64_t memmgr_gpu_size;
	uint64_t memmgr_gpu_align;
	uint64_t memmgr_gpu_tom;
	uint8_t memmgr_gpu_down;
	// memmgr sys
	uint64_t memmgr_sys_mc_base;
	uint64_t memmgr_sys_offset;
	uint64_t memmgr_sys_size;
	uint64_t memmgr_sys_align;
	uint64_t memmgr_sys_tom;
	uint8_t memmgr_sys_down;
	/* mem allocs */
	uint32_t mem_allocs_count;
	struct amdgv_live_info_memmgr_mem mem_allocs[MEM_ALLOCS_MAX];
	uint8_t reserved[65]; // 0x1100 align
};

struct amdgv_live_info_eeprom_table_record {
	uint64_t offset;
	uint64_t retired_page;
	uint64_t ts;

	uint32_t err_type;

	unsigned char bank;

	unsigned char mem_channel;
	unsigned char mcumc_id;
};

struct amdgv_live_info_ecc {
	struct live_info_table_header header;
	uint32_t enabled; // deprecated, placeholder for live update backward compatibility
	uint32_t correctable_error_num;
	uint32_t uncorrectable_error_num;

	struct amdgv_live_info_eeprom_table_record bps[EEPROM_MAX_ECC_PAGE_RECORD];
	/* the count of entries */
	int count;
	/* last reserved entry's index + 1 */
	int last_reserved;

	uint8_t reserved[64]; // 0xc80 align
};

struct amdgv_live_info_powerplay {
	struct live_info_table_header header;
	uint64_t smu_features;
	uint32_t clock_gating_features_flags;
	// smu clk info
	uint32_t socclk;
	uint32_t dcefclk;
	uint8_t reserved[92]; // 0x80 align
};

struct amdgv_live_info_sched_event {
	/* the index of vf */
	uint32_t idx_vf;

	/* sched event id */
	uint32_t id;

	/* sched block id */
	uint32_t sched_id;
};

struct amdgv_live_info_sched {
	struct live_info_table_header header;
	uint32_t		  in_full_access;
	uint32_t		  idx_vf_full_access;
	uint32_t		  event_id_full_access;
	uint32_t		  allow_time_full_access;
	uint64_t		  used_time_full_access;
	uint32_t		  lock_world_switch;

	/* unrecoverable error flag */
	uint32_t unrecov_err;

	/* whether need to check the next vf software live state */
	uint32_t check_alive;
	uint32_t logical_sched_fa_mask;
	uint8_t reserved[72]; // 0x80 align
};

struct amdgv_live_info_unprocessed_event {
	struct live_info_table_header header;
	uint32_t unprocessed_event_count;
	struct amdgv_live_info_sched_event unprocessed_events[AMDGV_EVENT_QUEUE_ENTRY_NUM];
	uint8_t reserved[108]; // 0xc80 align
};

struct amdgv_live_info_gpumon {
	struct live_info_table_header header;
	// product info
	uint8_t valid;
	uint8_t model_number[STRLEN_NORMAL];
	uint8_t product_serial[STRLEN_NORMAL];
	uint8_t fru_id[STRLEN_NORMAL];
	uint8_t reserved[15]; // 0x80 align
};

struct amdgv_live_info_vf {
	struct live_info_table_header header;
	// vf config
	uint8_t configured;
	/* bandwidth is in MB */
	uint32_t mm_bandwidth_hevc;
	uint32_t mm_bandwidth_vce;
	uint32_t mm_bandwidth_hevc1;
	uint32_t mm_bandwidth_vcn;
	/* time slice is in microseconds */
	uint32_t time_slice_gfx;
	uint32_t time_slice_uvd;
	uint32_t time_slice_vce;
	uint32_t time_slice_uvd1;
	uint32_t time_slice_vcn;
	uint32_t time_slice_vcn1;
	/* whether vf is waiting ack from vm*/
	uint8_t waiting_ack;
	/* whether vf is ready to be reset */
	uint8_t ready_to_reset;
	uint32_t vf_status;
	uint8_t gpu_init_data_ready;
	uint8_t cur_vf_state[AMDGV_MAX_NUM_HW_SCHED];
	/* used for update pf2vf message */
	uint64_t retired_page;
	/* used for calc offset for store new retired page in vf */
	uint32_t bp_block_size;
	uint8_t vram_lost;
	uint32_t auto_run;

	// vf sched
	uint32_t	state;
	uint8_t	reset_notify_vf_pending;
	uint32_t mm_bandwidth_jpeg;
	uint8_t reserved[6]; // 0x80 align
};

struct amdgv_live_info_world_switch {
	struct live_info_table_header header;
	uint32_t	curr_vf_state;
	uint32_t	curr_idx_vf;
	uint8_t		vf_inited[AMDGV_MAX_VF_LIVE];
	uint32_t	manual_array_vf_idx_vf[AMDGV_MAX_VF_LIVE];
	uint32_t	manual_array_vf_dummy_vf[AMDGV_MAX_VF_LIVE];

	/* allowed time for completing a gpuiov cmd (in us) */
	uint64_t allow_time_cmd_complete;
	uint8_t		switch_running;
	uint8_t reserved[14]; // 0x80 align
};

struct amdgv_live_info_critical_state {
	struct live_info_table_header header;
	uint32_t gpu_status;
	uint8_t reserved[108]; // 0x80 align
};

struct amdgv_live_info_ws_state {
	struct live_info_table_header header;
	uint32_t cur_gpu_state;
	uint32_t cur_vf_id;
	uint32_t mode;

	uint8_t reserved[4]; // 0x20 align
};

struct amdgv_live_info_xgmi {
	struct live_info_table_header header;
	uint64_t node_id;
	uint64_t hive_id;
	uint64_t node_segment_size;
	uint32_t phy_node_id;
	uint32_t phy_nodes_num;
	uint8_t connected_to_cpu;
	uint32_t socket_id;
	uint8_t fb_sharing_mode;
	uint8_t topology_status;
	uint8_t reserved[9]; // 0x40 align
};

struct amdgv_live_info_psp {
	struct live_info_table_header header;
	uint8_t xgmi_initialized;
	uint32_t xgmi_session_id;
	uint32_t asd_session_id;
	uint8_t ras_initialized;
	uint32_t ras_session_id;
	uint64_t attestation_db_gpu_addr;
	uint8_t reserved[90]; // 0x80 align
};

#define AMDGV_MAX_LIVE_UPDATE_BLOCK_NUM 512

struct amdgv_live_ffbm_block {
	uint64_t gpa;
	uint64_t spa;
	uint64_t size;
	uint8_t fragment;
	uint8_t permission;
	uint8_t vf_idx;
	uint8_t type;
	uint8_t applied;
	uint8_t reserved[3]; // 0x20 align
};

struct amdgv_live_info_ffbm {
	struct live_info_table_header header;
	struct amdgv_live_ffbm_block blocks[AMDGV_MAX_LIVE_UPDATE_BLOCK_NUM];
	uint16_t used_blocks;
	uint8_t bad_block_count;
	uint8_t reserved_block_count;
	uint8_t default_fragment;
	uint8_t max_reserved_block;
	uint8_t share_tmr;
	uint8_t reserved[73]; // 0x4060 align
};

struct amdgv_live_info_vf_extend {
	struct live_info_table_header header;
	// per partition full access mode
	uint64_t used_time_full_access;
	uint32_t event_id_full_access;
	uint8_t in_full_access;

	uint8_t reserved[99]; //0x80 align
};

// Per VF
struct live_info_mca_cache {
	uint8_t enabled;
	struct {
		uint32_t ce_count;
		uint32_t ue_count;
		uint32_t de_count;
		uint32_t ce_overflow_count;
		uint32_t ue_overflow_count;
		uint32_t de_overflow_count;
	} err_count[AMDGV_LIVE_MAX_RAS_BLOCK];

	uint8_t reserved[15]; // 0x310
};


// Per RAS BLOCK
struct live_info_mca_ecc {
	struct {
		uint32_t new_ce_count;
		uint32_t total_ce_count;
		uint32_t new_ue_count;
		uint32_t total_ue_count;
		uint32_t new_de_count;
		uint32_t total_de_count;
	} err_count[AMDGV_LIVE_MAX_XCD_NUM];
}; // 0x180

struct amdgv_live_info_mca {
	struct live_info_table_header header;
	struct live_info_mca_cache mca_cache[AMDGV_MAX_VF_LIVE];
	struct live_info_mca_ecc mca_ecc[AMDGV_LIVE_MAX_RAS_BLOCK];

	uint8_t reserved[96]; // 0x4C00
};

#define LIVE_INFO_MAX_GC_INSTANCES 8
#define LIVE_INFO_MAX_SDMA_RINGS 8
#define LIVE_INFO_MAX_COMPUTE_RINGS 16

struct amdgv_live_info_ring {
	struct live_info_table_header header;
	struct {
		uint32_t rb_rptr;
	} irqmgr[LIVE_INFO_MAX_GC_INSTANCES];
	struct {
		uint64_t rb_wptr;
	} sdma[LIVE_INFO_MAX_SDMA_RINGS];
	struct {
		uint64_t rb_wptr;
	} kiq[LIVE_INFO_MAX_GC_INSTANCES];
	struct {
		uint64_t rb_wptr;
	} compute[LIVE_INFO_MAX_COMPUTE_RINGS];

	uint8_t reserved[80]; // 0x80 bytes align
};

#define AMDGV_MAX_LIVE_INFO_DATA         256
#define AMDGV_LIVE_INFO_COMMON_DATA_SIZE 0x0001D9A0
#define AMDGV_GPU_DATA_HASH_SIZE         32

// v1
#define AMDGV_GPU_DATA_SIZE                (0x10ULL << 20) //16M
#define AMDGV_AGP_MEM_SIZE                 (0x8ULL << 20)  //8M
#define AMDGV_LIVE_INFO_MEMO_HEADER_SIZE   0x480
#define AMDGV_LIVE_INFO_MEMO_SIZE          (AMDGV_LIVE_INFO_MEMO_HEADER_SIZE + AMDGV_LIVE_INFO_COMMON_DATA_SIZE)
#define AMDGV_LIVE_INFO_MEMO_RESERVED_SIZE (AMDGV_GPU_DATA_SIZE - AMDGV_LIVE_INFO_MEMO_SIZE - AMDGV_AGP_MEM_SIZE)

#define AMDGV_GPU_DATA_V2_SIZE             (0x1ULL << 20) //1M
#define AMDGV_LIVE_INFO_FILE_HEADER_SIZE   0x420
#define AMDGV_LIVE_INFO_V2_SIZE            (AMDGV_LIVE_INFO_FILE_HEADER_SIZE + AMDGV_LIVE_INFO_COMMON_DATA_SIZE)
#define AMDGV_LIVE_INFO_V2_RESERVED_SIZE   (AMDGV_GPU_DATA_V2_SIZE - AMDGV_LIVE_INFO_V2_SIZE)

// update to use v2
#define AMDGV_LIVE_INFO_FILE_SIZE          (AMDGV_LIVE_INFO_FILE_HEADER_SIZE + AMDGV_LIVE_INFO_COMMON_DATA_SIZE)
#define AMDGV_LIVE_INFO_FILE_RESERVED_SIZE (AMDGV_GPU_DATA_FILE_SIZE - AMDGV_LIVE_INFO_FILE_SIZE)

/*
 * Note: No live update structs for the following since they do not need export:
 *   AMDGV_LIVE_INFO_DATA__IP_DISCOVERY
 *   AMDGV_LIVE_INFO_DATA__GPUIOV
 */
#define amdgv_live_update_common_data() {                                                                                                                                     \
	struct amdgv_live_info_critical_state    crtical_state;                            /* op:AMDGV_LIVE_INFO_DATA__CRITICAL_STATE,          offset:00000000, size:00000080 */ \
	struct amdgv_live_info_vbios             vbios;                                    /* op:AMDGV_LIVE_INFO_DATA__VBIOS,                   offset:00000080, size:00010080 */ \
	struct amdgv_live_info_memmgr            memmgr;                                   /* op:AMDGV_LIVE_INFO_DATA__MEMMGR,                  offset:00010100, size:00001100 */ \
	struct amdgv_live_info_gpumon            gpumon;                                   /* op:AMDGV_LIVE_INFO_DATA__GPUMON,                  offset:00011200, size:00000080 */ \
	struct amdgv_live_info_param             module_param;                             /* op:AMDGV_LIVE_INFO_DATA__MODULE_PARAM_PRE + POST, offset:00011280, size:00000080 */ \
	struct amdgv_live_info_ecc               ecc;                                      /* op:AMDGV_LIVE_INFO_DATA__ECC,                     offset:00011300, size:00000c80 */ \
	struct amdgv_live_info_fw_info           fw_info;                                  /* op:AMDGV_LIVE_INFO_DATA__FW_INFO,                 offset:00011F80, size:00000100 */ \
	struct amdgv_live_info_powerplay         powerplay;                                /* op:AMDGV_LIVE_INFO_DATA__POWERPLAY,               offset:00012080, size:00000080 */ \
	struct amdgv_live_info_world_switch      world_switch[AMDGV_MAX_NUM_WORLD_SWITCH]; /* op:AMDGV_LIVE_INFO_DATA__WORLD_SWITCH,            offset:00012100, size:00001000 */ \
	struct amdgv_live_info_ws_state          ws_state[AMDGV_MAX_NUM_HW_SCHED];         /* op:AMDGV_LIVE_INFO_DATA__WS_STATE,                offset:00013100, size:00000400 */ \
	struct amdgv_live_info_unprocessed_event event;                                    /* op:AMDGV_LIVE_INFO_DATA__UNPROCESSED_EVENT,       offset:00013500, size:00000c80 */ \
	struct amdgv_live_info_psp               psp_info;                                 /* op:AMDGV_LIVE_INFO_DATA__PSP,                     offset:00014180, size:00000080 */ \
	struct amdgv_live_info_ffbm              ffbm_info;                                /* op:AMDGV_LIVE_INFO_DATA__FFBM,                    offset:00014200, size:00004060 */ \
	struct amdgv_live_info_vf                vf_info[AMDGV_MAX_VF_LIVE];               /* op:AMDGV_LIVE_INFO_DATA__VF,                      offset:00018260, size:00000480 */ \
	struct amdgv_live_info_sched             sched;                                    /* op:AMDGV_LIVE_INFO_DATA__SCHED,                   offset:000186E0, size:00000080 */ \
	struct amdgv_live_info_xgmi              xgmi;                                     /* op:AMDGV_LIVE_INFO_DATA__XGMI,                    offset:00018760, size:00000040 */ \
	struct amdgv_live_info_vf_extend         vf_info_extend[AMDGV_MAX_VF_LIVE];        /* op:AMDGV_LIVE_INFO_DATA__VF_EXTEND,               offset:000187A0, size:00000480 */ \
	struct amdgv_live_info_ring              ring;                                     /* op:AMDGV_LIVE_INFO_DATA__RING,                    offset:00018C20, size:00000180 */ \
	struct amdgv_live_info_mca               mca;                                      /* op:AMDGV_LIVE_INFO_DATA__MCA,                     offset:00018DA0, size:00004C00 */ \
	/*                                                                                    Total                                             offset:0001D9A0                */ \
}

// v1
struct amdgv_gpu_data_header {
	char signature[9]; //"GPU DATA"
	uint32_t hash;
	uint64_t hash_addr;
	uint32_t size;    //LIVE INFO DATA SIZE
	uint32_t version; //LIVE_INFO_HEADER_VERSION
	uint32_t gpu_num; //total gpu num, only stored at the first gpu memory
	uint32_t bdf;
	uint32_t op_num; //total live update data info types
	uint32_t op_offset[AMDGV_MAX_LIVE_INFO_DATA];
	uint8_t reserved[87]; // 0x480 align
};

// v1
struct amdgv_gpu_data {
	struct amdgv_gpu_data_header header;                  // offset:00000000, size:00000480
	struct amdgv_live_update_common_data();               // offset:00000480, szie:0001D9A0
	uint8_t reserved[AMDGV_LIVE_INFO_MEMO_RESERVED_SIZE]; // offset:0001DE20
	char agp_mem[AMDGV_AGP_MEM_SIZE];                     // AGP memory
};

// remove and use amdgv_gpu_data_v2
struct amdgv_gpu_data_file_header {
	uint32_t idx;
	uint32_t hash;
	uint64_t hash_addr;
	uint32_t size;
	uint32_t bdf;
	uint32_t op_num;
	uint32_t op_offset[AMDGV_MAX_LIVE_INFO_DATA];
	uint8_t reserved[4];
};

// remove and use amdgv_gpu_data_v2
struct amdgv_gpu_data_file {
	struct amdgv_gpu_data_file_header header;           // offset:00000000, size:00000420
	struct amdgv_live_update_common_data();             // offset:00000420, szie:0001D9A0
	uint8_t reserved[AMDGV_LIVE_INFO_V2_RESERVED_SIZE]; // offset:0001DDC0
};

#define LIVE_INFO_SIG_STR "GPU DATA"
#define LIVE_INFO_SIG_SIZE 9

/*
 * unified live update header
 *
 * [ file header ][ data0 header | data0 ][ data1 header | data1 ]...
 */
struct amdgv_live_update_file_header {
	char     signature[LIVE_INFO_SIG_SIZE];
	uint32_t version;
	uint32_t gpu_num;
	uint8_t  reserved[7];
};

struct amdgv_gpu_data_header_v2 {
	uint32_t idx;
	uint32_t hash;
	uint64_t hash_addr;
	/*
	 * header.size is for hash purpose and should not be used for allocation
	 * anything before header.size is excluded from hashing
	 */
	uint32_t size;
	uint32_t bdf;
	uint32_t op_num;
	uint32_t op_offset[AMDGV_MAX_LIVE_INFO_DATA];
	uint8_t  reserved[4];
};

struct amdgv_gpu_data_v2 {
	struct  amdgv_gpu_data_header_v2 header;            // offset:00000000, size:00000420
	struct  amdgv_live_update_common_data();            // offset:00000420, szie:0001D9A0
	uint8_t reserved[AMDGV_LIVE_INFO_V2_RESERVED_SIZE]; // offset:0001DDC0
};

enum amdgv_live_info_status amdgv_live_info_init_metadata(struct amdgv_adapter *adapt);
int amdgv_live_info_export_size(struct amdgv_adapter *adapt, uint32_t *size);

int amdgv_live_info_export_data(struct amdgv_adapter *adapt,
					  uint32_t data_op,
					  void *data,
					  enum amdgv_live_info_status *status);
int amdgv_live_info_import_data(struct amdgv_adapter *adapt,
					  uint32_t data_op,
					  void *data,
					  enum amdgv_live_info_status *status);

enum amdgv_live_info_status amdgv_import_data_by_op(struct amdgv_adapter *adapt, uint32_t data_op);
enum amdgv_live_info_status amdgv_export_data(struct amdgv_adapter *adapt);
enum amdgv_live_info_status amdgv_import_data(struct amdgv_adapter *adapt);

enum amdgv_live_info_status amdgv_sched_ws_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_world_switch *world_switch);
enum amdgv_live_info_status amdgv_sched_ws_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_world_switch *world_switch);
enum amdgv_live_info_status amdgv_ws_state_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ws_state *ws_state);
enum amdgv_live_info_status amdgv_ws_state_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ws_state *ws_state);
void amdgv_live_info_prepare_reset(struct amdgv_adapter *adapt);
#pragma pack(pop) // Restore previous packing option

/* assertion at compile time */
#ifdef __linux__
#define stringification(s)  _stringification(s)
#define _stringification(s) #s

_Static_assert(
	sizeof(struct live_info_table_header) == 0x10,
	"live_info_table_header must be 0x10 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_param) == 0x80,
	"amdgv_live_info_param must be 0x80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_vbios) == 0x10080,
	"amdgv_live_info_vbios must be 0x10080 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_fw_info) == 0x100,
	"amdgv_live_info_fw_info must be 0x100 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_memmgr) == 0x1100,
	"amdgv_live_info_memmgr must be 0x1100 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_ecc) == 0xc80,
	"amdgv_live_info_ecc must be 0xc80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_powerplay) == 0x80,
	"amdgv_live_info_powerplay must be 0x80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_sched) == 0x80,
	"amdgv_live_info_sched must be 0x80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_unprocessed_event) == 0xc80,
	"amdgv_live_info_unprocessed_event must be 0xc80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_gpumon) == 0x80,
	"amdgv_live_info_gpumon must be 0x80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_vf) == 0x80,
	"amdgv_live_info_vf_config must be 0x80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_world_switch) == 0x80,
	"amdgv_live_info_world_switch must be 0x80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_critical_state) == 0x80,
	"amdgv_live_info_critical_state must be 0x80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_ws_state) == 0x20,
	"amdgv_live_info_ws_state must be 0x80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_psp) == 0x80,
	"amdgv_live_info_psp must be 0x80 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_ffbm) == 0x4060,
	"amdgv_live_info_ffbm must be 0x4060 byte align");

_Static_assert(
	sizeof(struct amdgv_gpu_data_header) == 0x480,
	"amdgv_gpu_data_header must be 0x480 byte align");

_Static_assert(
	sizeof(struct amdgv_live_info_ring) == 0x180,
	"amdgv_live_info_ring must be 0x180 byte align");

_Static_assert(
	sizeof(struct amdgv_gpu_data) == 0x1000000,
	"amdgv_gpu_data must be 0x1000000 byte align");

struct __common_data_size_check {
	struct amdgv_live_update_common_data();
};
_Static_assert(
	sizeof(struct __common_data_size_check) == AMDGV_LIVE_INFO_COMMON_DATA_SIZE,
	"amdgv_live_update_common_data size must match AMDGV_LIVE_INFO_COMMON_DATA_SIZE");

#undef _stringification
#undef stringification
#endif


#endif
