/*
 * Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
 */

#ifndef __AMDGV_CMD_H__
#define __AMDGV_CMD_H__

#define AMDGV_CMD_HANDLER_NAME "amdgv-cmd-handle"
#define AMDGV_CMD_HANDLE_PATH  "/dev/"AMDGV_CMD_HANDLER_NAME
#define AMDGV_CMD_MAX_IN_SIZE 128
#define AMDGV_CMD_MAX_OUT_SIZE 1200
#define AMDGV_CMD_MAX_GPU_NUM 32
#define AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP 32
#define AMDGV_CMD_PATH_LEN 100
#define AMDGV_CMD_DIAG_DATA_DEF_PATH "/var/log/"
#define AMDGV_CMD_MAX_DIAG_DATA_BUFFER_SIZE (4*1024*1024)
#define AMDGV_CMD_VERSION 1
#define AMDGV_CMD_VERSION_V2 3

#define __gim_max(a, b) ((int)(a) > (int)(b) ? (a) : (b))
#define AMDGV_MAX_FB_REGIONS \
    __gim_max(AMDGV_FB_PF_MAX_SUPPORTED, AMDGV_FB_VF_MAX_SUPPORTED)

enum amdgv_cmd_id {
    AMDGV_CMD_GET_DEVICES_INFO = 1,
    AMDGV_CMD_GET_BLOCK_ECC_STATUS,
    AMDGV_CMD_RAS_INJECT_ERROR,
    AMDGV_CMD_RAS_ENABLE,
    AMDGV_CMD_GET_BAD_PAGES,
    AMDGV_CMD_CLEAR_BAD_PAGE_INFO,
    AMDGV_CMD_GPU_RESET,
    AMDGV_CMD_GET_FB_PF_REGIONS,
    AMDGV_CMD_GET_FB_VF_REGIONS,
    AMDGV_CMD_GET_DIAG_DATA,
    AMDGV_CMD_GET_VF_BDF,
    AMDGV_CMD_RAS_TA_LOAD,
    AMDGV_CMD_RAS_TA_UNLOAD,
    AMDGV_CMD_SET_BP_MODE,
    AMDGV_CMD_GET_BP_MODE,
    AMDGV_CMD_GET_CURRENT_BP,
    AMDGV_CMD_BP_GO,
    AMDGV_CMD_SEND_WS_CMD,
    AMDGV_CMD_GET_DEVICES_EX_INFO,
    AMDGV_CMD_SUPPORTED_MAX
};

enum amdgv_cmd_response {
    AMDGV_CMD__SUCCESS = 0,
    AMDGV_CMD__ERROR_UKNOWN_CMD,
    AMDGV_CMD__ERROR_VERSION,
    AMDGV_CMD__ERROR_INVALID_INPUT,
    AMDGV_CMD__ERROR_DRV_INIT_FAIL,
    AMDGV_CMD__ERROR_GENERIC
};

/* explilcitly assign index for api consistency */
enum amdgv_cmd_asic_type {
	AMDGV_CMD_CHIP_MI300X = 9,
	AMDGV_CMD_CHIP_MI308X = 11,
	AMDGV_CMD_CHIP_UNKNOWN,
	AMDGV_CMD_CHIP_LAST,
};

enum amdgv_ras_block {
    AMDGV_RAS_BLOCK__UMC = 0,
    AMDGV_RAS_BLOCK__SDMA,
    AMDGV_RAS_BLOCK__GFX,
    AMDGV_RAS_BLOCK__MMHUB,
    AMDGV_RAS_BLOCK__ATHUB,
    AMDGV_RAS_BLOCK__PCIE_BIF,
    AMDGV_RAS_BLOCK__HDP,
    AMDGV_RAS_BLOCK__XGMI_WAFL,
    AMDGV_RAS_BLOCK__DF,
    AMDGV_RAS_BLOCK__SMN,
    AMDGV_RAS_BLOCK__SEM,
    AMDGV_RAS_BLOCK__MP0,
    AMDGV_RAS_BLOCK__MP1,
    AMDGV_RAS_BLOCK__FUSE,
    AMDGV_RAS_BLOCK_MAX
};

enum amdgv_ras_error_type {
    AMDGV_RAS_TYPE_ERROR__NONE = 0,
    AMDGV_RAS_TYPE_ERROR__PARITY = 1,
    AMDGV_RAS_TYPE_ERROR__SINGLE_CORRECTABLE = 2,
    AMDGV_RAS_TYPE_ERROR__MULTI_UNCORRECTABLE = 4,
    AMDGV_RAS_TYPE_ERROR__POISON = 8,
};

enum amdgv_ecc_type_support {
	AMDGV_RAS_MEM_ECC_SUPPORT = 0,
	AMDGV_RAS_SRAM_ECC_SUPPORT,
	AMDGV_RAS_POISON_ECC_SUPPORT
};

enum amdgv_ras_eeprom_err_type {
	AMDGV_RAS_EEPROM_ERR_PLACE_HOLDER,
	AMDGV_RAS_EEPROM_ERR_RECOVERABLE,
	AMDGV_RAS_EEPROM_ERR_NON_RECOVERABLE
};

enum amdgv_fb_pf_regions_area {
    AMDGV_FB_REGION_PF_DATA_EXCHANGE = 0,
    AMDGV_FB_REGION_PF_IP_DISCOVERY,
    AMDGV_FB_REGION_TMR,
    AMDGV_FB_REGION_CSA,
    AMDGV_FB_PF_MAX_SUPPORTED
};

enum amdgv_fb_vf_regions_area {
    AMDGV_FB_REGION_VF = 0,
    AMDGV_FB_REGION_VF_DATA_EXCHANGE,
    AMDGV_FB_REGION_VF_IP_DISCOVERY,
    AMDGV_FB_VF_MAX_SUPPORTED
};

struct amdgv_fb_region_area {
    union {
		enum amdgv_fb_pf_regions_area pf;
		enum amdgv_fb_vf_regions_area vf;
	};
};

// Input structures
struct amdgv_cmd_dev_handle {
    uint32_t dev_handle;
};

struct amdgv_cmd_dev_block_info {
    struct amdgv_cmd_dev_handle dev;
    enum amdgv_ras_block block_id;
    uint32_t subblock_id;
};

struct amdgv_cmd_ras_inject_error {
    struct amdgv_cmd_dev_block_info device_info;
    uint64_t address;
    enum amdgv_ras_error_type error_type;
    uint64_t value;
    uint32_t mask;
};

struct amdgv_cmd_ras_enable_ecc {
    struct amdgv_cmd_dev_handle device;
    uint8_t passphrase[8];
};

struct amdgv_cmd_req_bad_pages_group {
    struct amdgv_cmd_dev_handle device;
    uint32_t group_index;
};

struct amdgv_cmd_vf_info {
    struct amdgv_cmd_dev_handle device;
    uint32_t vf_index;
};

struct amdgv_cmd_debug_data_dir_path {
    uint8_t abs_dir_path[AMDGV_CMD_PATH_LEN];
};

struct amdgv_cmd {
    uint32_t input_size;
    uint32_t output_size;
    uint8_t version;
    uint8_t cmd_res;
    enum amdgv_cmd_id cmd_id;
    uint32_t reserved[4];
    uint8_t input_buff_raw[AMDGV_CMD_MAX_IN_SIZE];
    uint8_t output_buff_raw[AMDGV_CMD_MAX_OUT_SIZE];
} __attribute__ ((aligned (8)));

// Output structures
struct amdgv_cmd_dev_info {
    uint32_t dev_handle;
    uint32_t bdf;
    uint32_t ecc_enabled;
    uint32_t ecc_supported;
    uint32_t vf_num;
    uint32_t asic_type;
};

struct amdgv_cmd_devices_info {
    struct amdgv_cmd_dev_info devs[AMDGV_CMD_MAX_GPU_NUM];
    uint8_t dev_num;
};

// device expand info
struct amdgv_cmd_dev_ex_info {
    uint32_t dev_handle;
    uint32_t bdf;
    uint32_t ecc_enabled;
    uint32_t ecc_supported;
    uint32_t vf_num;
    uint32_t asic_type;
    uint32_t oam_id;
    uint32_t reserved[2];
};

struct amdgv_cmd_devices_ex_info {
    struct amdgv_cmd_dev_ex_info devs[AMDGV_CMD_MAX_GPU_NUM];
    uint8_t dev_num;
};

struct amdgv_cmd_ecc_count {
    uint32_t corr_error_cnt;
    uint32_t uncorr_error_cnt;
    uint32_t deferred_error_cnt;
};

struct amdgv_cmd_bad_page_record {
    union {
		uint64_t address;
		uint64_t offset;
	};
	uint64_t retired_page;
	uint64_t ts;

	enum amdgv_ras_eeprom_err_type err_type;

	union {
		unsigned char bank;
		unsigned char cu;
	};

	unsigned char mem_channel;
	unsigned char mcumc_id;
};

struct amdgv_cmd_bad_pages_info {
    uint32_t group_index;
    uint32_t bp_in_group;
    uint32_t bp_total_cnt;
    struct amdgv_cmd_bad_page_record records[AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP];
};

struct amdgv_fb_region_area_info {
    uint64_t start;
    uint64_t size;
    struct amdgv_fb_region_area region;
};

struct amdgv_cmd_fb_regions {
    uint32_t reg_cnt;
    struct amdgv_fb_region_area_info region_areas[AMDGV_MAX_FB_REGIONS];
};

union amdgv_cmd_device_bdf {
	struct {
		uint32_t function	 : 3;
		uint32_t device		 : 5;
		uint32_t bus		 : 8;
		uint32_t domain		 : 16;
	};
    uint32_t u32_all;
};

struct amdgv_cmd_ras_ta_load_req {
    struct amdgv_cmd_dev_handle dev;
    uint32_t version;
    uint32_t data_len;
    uint64_t data_addr;
    uint32_t reserved[5];
};

struct amdgv_cmd_ras_ta_load_rsp {
    uint64_t ras_session_id;
    uint32_t reserved[10];
};

struct amdgv_cmd_ras_ta_unload {
    struct amdgv_cmd_dev_handle dev;
    uint64_t ras_session_id;
    uint32_t reserved[5];
};

/* Break Point Debug Mode:
 * 0: default value, bp_mode is off
 * 1: pause at first init/run PF
 * 2: pause at every world switch event */
enum amdgv_cmd_bp_mode {
	AMDGV_BP_MODE_DISABLE   = 0,
	AMDGV_BP_MODE_1         = 1,
	AMDGV_BP_MODE_2         = 2,
	AMDGV_BP_MODE_MAX,
};

struct amdgv_cmd_set_bp_mode {
	struct amdgv_cmd_dev_handle dev;
	enum amdgv_cmd_bp_mode bp_mode;
};

/* World switch events we are allowing user to send through BP tool */
enum amdgv_cmd_ws_cmd {
	AMDGV_IDLE_GPU                  = 0x01,
	AMDGV_SAVE_GPU_STATE            = 0x02,
	AMDGV_LOAD_GPU_STATE            = 0x03,
	AMDGV_RUN_GPU                   = 0x04,
	AMDGV_ENABLE_AUTO_HW_SWITCH     = 0x06,
	AMDGV_INIT_GPU                  = 0x07,
	AMDGV_DISABLE_AUTO_HW_SCHED     = 0x0B,
	AMDGV_SHUTDOWN_GPU              = 0x0D,
};

/* This is a reference for all hw_sched_id on commonly used asics:

	NV:
	VCN_SCH0_MMSCH		= 0,
	GFX_SCH0_RLCV		= 1,
	VCN1_SCH1_MMSCH		= 2,	(only on NV21 & NV32)
	JPEG_SCH0_MMSCH		= 3,	(only on NV32)

	MI200:
	UVD_SCH0_MMSCH		= 0,
	VCE_SCH0_MMSCH		= 1,
	GFX_SCH0_RLCV		= 2,
	UVD_SCH1_MMSCH		= 3,

	MI300:
	VCN_SCH0_MMSCH 		= 0,
	JPEG_SCH0_MMSCH 	= 1,
	JPEG1_SCH0_MMSCH 	= 2,
	VCN_SCH1_MMSCH 		= 3,
	JPEG_SCH1_MMSCH 	= 4,
	JPEG1_SCH1_MMSCH 	= 5,
	VCN_SCH2_MMSCH 		= 6,
	JPEG_SCH2_MMSCH 	= 7,
	JPEG1_SCH2_MMSCH 	= 8,
	VCN_SCH3_MMSCH 		= 9,
	JPEG_SCH3_MMSCH 	= 10,
	JPEG1_SCH3_MMSCH 	= 11,
	GFX_SCH0_RLCV 		= 12,
	GFX_SCH1_RLCV 		= 13,
	GFX_SCH2_RLCV 		= 14,
	GFX_SCH3_RLCV 		= 15,
	GFX_SCH4_RLCV 		= 16,
	GFX_SCH5_RLCV 		= 17,
	GFX_SCH6_RLCV 		= 18,
	GFX_SCH7_RLCV 		= 19,
*/
struct amdgv_cmd_bp_info {
	bool is_in_bp;
	enum amdgv_cmd_ws_cmd ws_cmd;
	uint32_t hw_sched_id;
	uint32_t idx_vf;
};

struct amdgv_cmd_user_ws_cmd {
	struct amdgv_cmd_dev_handle dev;
	enum amdgv_cmd_ws_cmd ws_cmd;
	uint32_t hw_sched_id;
	uint32_t idx_vf;
};

#endif
