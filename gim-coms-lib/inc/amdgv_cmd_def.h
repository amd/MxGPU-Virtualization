
/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _AMDGV_CMD_H_
#define _AMDGV_CMD_H_

#include "gim_ioctl.h"

#define AMDGV_INTERFACE_MAJOR_VERSION 1
#define AMDGV_INTERFACE_MINOR_VERSION 1

#define AMDGV_CMD_HANDLER_NAME "amdgv-cmd-handle"
#define AMDGV_CMD_HANDLE_PATH  "/dev/"AMDGV_CMD_HANDLER_NAME
#define AMDGV_CMD_MAX_IN_SIZE 128
#define AMDGV_CMD_MAX_OUT_SIZE 1600
#define AMDGV_CMD_MAX_GPU_NUM 32
#define AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP 32
#define AMDGV_CMD_PATH_LEN 100
#define AMDGV_CMD_DIAG_DATA_DEF_PATH "/var/log/"
#define AMDGV_CMD_MAX_DIAG_DATA_BUFFER_SIZE (4*1024*1024)
#define AMDGV_CMD_VERSION 2

#define AMDGV_RAS_MAX_NUM_SAFE_RANGES 64

#define __gim_max(a, b) ((int)(a) > (int)(b) ? (a) : (b))
#define AMDGV_MAX_FB_REGIONS \
	__gim_max((int)AMDGV_FB_PF_MAX_SUPPORTED, (int)AMDGV_FB_VF_MAX_SUPPORTED)

#define AMDGV_CMD_ALL_DEVICE ":"
#define AMDGV_COMMON_IOCTL_NODE_SIZE 40
#define AMDGV_COMMON_IOCTL_CMD_SIZE 80
#define AMDGV_COMMON_IOCTL_OUT_SIZE 1000
#define AMDGV_CMD_ALL_DEVICE_BDF -1

/* indicate this cmd has a shm msg send with input */
#define AMDGV_CMD_SHM_CLI2SER_MASK (1 << 22)
/* indicate this cmd expects a shm msg for output */
#define AMDGV_CMD_SHM_SER2CLI_MASK (1 << 23)
#define AMDGV_CMD_SHM_CLI2SER(cmd) \
	((GIM_IOCTL_GET_TYPE(cmd) == AMDGV_IOCTL) && ((*(uint32_t *)cmd) & AMDGV_CMD_SHM_CLI2SER_MASK))
#define AMDGV_CMD_SHM_SER2CLI(cmd) \
	((GIM_IOCTL_GET_TYPE(cmd) == AMDGV_IOCTL) && ((*(uint32_t *)cmd) & AMDGV_CMD_SHM_SER2CLI_MASK))

enum amdgv_cmd_type {
	RAS_CMD = 0 << 16,
	COMMON_CMD = 1 << 16,
	DCORE_CMD = 2 << 16,
};

enum amdgv_cmd_ras_id {
	AMDGV_CMD_QUERY_INTERFACE_VERSION = AMDGV_IOCTL | RAS_CMD,
	AMDGV_CMD_GET_DEVICES_INFO = AMDGV_IOCTL | 0x001,
	AMDGV_CMD_GET_BLOCK_ECC_STATUS = AMDGV_IOCTL | 0x002,
	AMDGV_CMD_RAS_INJECT_ERROR = AMDGV_IOCTL | 0x003,
	AMDGV_CMD_RAS_ENABLE = AMDGV_IOCTL | 0x004,
	AMDGV_CMD_GET_BAD_PAGES = AMDGV_IOCTL | 0x005,
	AMDGV_CMD_CLEAR_BAD_PAGE_INFO = AMDGV_IOCTL | 0x006,
	AMDGV_CMD_GPU_RESET = AMDGV_IOCTL | 0x007,
	AMDGV_CMD_GET_FB_PF_REGIONS = AMDGV_IOCTL | 0x008,
	AMDGV_CMD_GET_FB_VF_REGIONS = AMDGV_IOCTL | 0x009,
	AMDGV_CMD_GET_VF_BDF = AMDGV_IOCTL | 0x00a,
	AMDGV_CMD_RAS_TA_LOAD = AMDGV_IOCTL | 0x00b,
	AMDGV_CMD_RAS_TA_UNLOAD = AMDGV_IOCTL | 0x00c,
	AMDGV_CMD_RAS_GET_SAFE_FB_ADDRESS_RANGES = AMDGV_IOCTL | 0x00d,
	AMDGV_CMD_TRANSLATE_FB_ADDRESS = AMDGV_IOCTL | 0x00e,
	AMDGV_CMD_RAS_RESET_ALL_ERROR_COUNTS = AMDGV_IOCTL | 0x00f,
	AMDGV_CMD_SUPPORTED_MAX
};

enum amdgv_cmd_common_id {
	AMDGV_CMD_COMMON_IOCTL = AMDGV_IOCTL | COMMON_CMD,
	AMDGV_CMD_PSP_VBFLASH_COPY = AMDGV_IOCTL | COMMON_CMD | 0x1 | AMDGV_CMD_SHM_CLI2SER_MASK,
	AMDGV_CMD_PSP_VBFLASH_PROCESS = AMDGV_IOCTL | COMMON_CMD | 0x2,
	AMDGV_CMD_PSP_VBFLASH_STATUS = AMDGV_IOCTL | COMMON_CMD | 0x3,
};

/* command code for dcore_drv ioctl */
enum rdl_cmd_code {
	RDL_CMD_START_TRAP_GPU_HANG = 0x00000001,	  /* Used when host thread starts to wait for a reset event from kernel */
	RDL_CMD_NOTIFY_DUMP_DONE = 0x00000002,		  /* Used to notify kernel that the data dumping of host thread has been finished */
	RDL_CMD_GET_DIAG_DATA = 0x00000003,	  /* Used when host try to clean or get the existing host/fw part legacy data */
	RDL_CMD_STOP_TRAP_GPU_HANG = 0x00000004,	  /* Used when host try to stop the monitor thread */
	RDL_CMD_GET_FFBM_DATA = 0x00000005,			  /* Used when copy ffbm spa data */
	RDL_CMD_CODE_MAX
};

enum amdgv_cmd_rdl_id {
	AMDGV_CMD_DCORE_IOCTL = AMDGV_IOCTL | DCORE_CMD,
	AMDGV_CMD_RDL_START_TRAP_GPU_HANG = AMDGV_CMD_DCORE_IOCTL | RDL_CMD_START_TRAP_GPU_HANG,
	AMDGV_CMD_RDL_NOTIFY_DUMP_DONE = AMDGV_CMD_DCORE_IOCTL | RDL_CMD_NOTIFY_DUMP_DONE,
	AMDGV_CMD_RDL_GET_DIAG_DATA = AMDGV_CMD_DCORE_IOCTL | RDL_CMD_GET_DIAG_DATA | AMDGV_CMD_SHM_SER2CLI_MASK,
	AMDGV_CMD_RDL_GET_FFBM_DATA = AMDGV_CMD_DCORE_IOCTL | RDL_CMD_GET_FFBM_DATA | AMDGV_CMD_SHM_SER2CLI_MASK,
	AMDGV_CMD_RDL_STOP_TRAP_GPU_HANG = AMDGV_CMD_DCORE_IOCTL | RDL_CMD_STOP_TRAP_GPU_HANG,
};

enum amdgv_cmd_response {
	AMDGV_CMD__SUCCESS = 0,
	AMDGV_CMD__SUCCESS_EXEED_BUFFER,
	AMDGV_CMD__ERROR_UKNOWN_CMD,
	AMDGV_CMD__ERROR_VERSION,
	AMDGV_CMD__ERROR_INVALID_INPUT,
	AMDGV_CMD__ERROR_DRV_INIT_FAIL,
	AMDGV_CMD__ERROR_GENERIC
};

enum amdgv_cmd_asic_type {
	AMDGV_CMD_CHIP_MI300X = 9,
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
	AMDGV_RAS_BLOCK__MCA,
	AMDGV_RAS_BLOCK__VCN,
	AMDGV_RAS_BLOCK__JPEG,
	AMDGV_RAS_BLOCK__IH,
	AMDGV_RAS_BLOCK__MPIO,
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

enum amdgv_ras_ta_load_status {
	AMDGV_RAS_TA_STATUS_NO_CHANGE,
	AMDGV_RAS_TA_STATUS_UPGRADED,
	AMDGV_RAS_TA_STATUS_DOWNGRADED,
	AMDGV_RAS_TA_STATUS_LOADED
};

struct amdgv_fb_region_area {
	union {
		enum amdgv_fb_pf_regions_area pf;
		enum amdgv_fb_vf_regions_area vf;
	};
};

// Input structures
struct amdgv_cmd_dev_handle {
	uint64_t dev_handle;
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
	union {
		uint64_t value;
		struct {
			uint32_t method;
			uint32_t vf_idx;	// vf index (31 for PF)
		};
	};
	uint32_t chiplet;
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

// Output structures
struct amdgv_cmd_dev_info {
	uint64_t dev_handle;
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

struct amdgv_cmd_ecc_count {
	uint32_t corr_error_cnt;
	uint32_t uncorr_error_cnt;
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

struct amdgv_query_interface_version_req {
	uint32_t reserved[8];
};

struct amdgv_query_interface_version_rsp {
	uint8_t major_ver;    // interface major
	uint8_t minor_ver;    // interface minor
	uint8_t reserved[26];
};

struct amdgv_cmd_ras_ta_load_req {
    struct amdgv_cmd_dev_handle dev;
	uint32_t version;
	uint32_t data_len;
	uint64_t data_addr;
	uint32_t reserved[2];
};

struct amdgv_cmd_ras_ta_load_rsp {
	uint64_t ras_session_id;	// starts from 0
	enum amdgv_ras_ta_load_status ta_status;
	uint32_t reserved[5];
};

struct amdgv_cmd_ras_ta_unload_req {
	struct amdgv_cmd_dev_handle dev;
	uint64_t ras_session_id;
	uint32_t reserved[4];
};

struct amdgv_cmd_ras_safe_fb_address_ranges_rsp {
	uint32_t num_ranges;
	uint32_t reserved[3];
	struct {
		uint64_t start;
		uint64_t size;
		uint32_t reserved[2];
	} range[AMDGV_RAS_MAX_NUM_SAFE_RANGES];
};

enum amdgv_fb_addr_type {
	AMDGV_FB_ADDR_SOC_PHY,  /* SPA */
	AMDGV_FB_ADDR_BANK,
	AMDGV_FB_ADDR_VF_PHY,   /* GPA */
};

struct amdgv_fb_bank_addr {
	uint32_t stack_id; /* SID */
	uint32_t bank_group;
	uint32_t bank;
	uint32_t row;
	uint32_t column;
	uint32_t channel;
	uint32_t subchannel; /* Also called Pseudochannel (PC) */
	uint32_t reserved[3];
};

struct amdgv_fb_vf_phy_addr {
	uint32_t vf_idx;
	uint64_t addr;
};

struct amdgv_cmd_translate_fb_address_req {
	struct amdgv_cmd_dev_handle dev;
	enum amdgv_fb_addr_type src_addr_type;
	enum amdgv_fb_addr_type dest_addr_type;
	union {
		struct amdgv_fb_bank_addr bank_addr;
		uint64_t soc_phy_addr;
		struct amdgv_fb_vf_phy_addr vf_phy_addr;
	};
};

struct amdgv_cmd_translate_fb_address_rsp {
	union {
		struct amdgv_fb_bank_addr bank_addr;
		uint64_t soc_phy_addr; /* In Bytes */
		struct amdgv_fb_vf_phy_addr vf_phy_addr;
	};
};

struct amdgv_cmd_debug_data_dir_path {
	uint8_t abs_dir_path[AMDGV_CMD_PATH_LEN];
};

#pragma pack(push, 8)
union amdgv_cmd_device_bdf {
	struct {
		uint32_t function	 : 3;
		uint32_t device		 : 5;
		uint32_t bus		 : 8;
		uint32_t domain		 : 16;
	};
	uint32_t u32_all;
};

struct amdgv_cmd_ctx {
	uintptr_t input_buffer_addr;
	uintptr_t output_buffer_addr;
};

/* header for cmd with shm msg, should be placed at beginning of buff_raw */
struct amdgv_cmd_shm_info {
	uint32_t buffer_size;
	uintptr_t buffer_addr;
};

struct amdgv_cmd_vbflash_info {
	struct amdgv_cmd_shm_info shm_info;
	union amdgv_cmd_device_bdf bdf;
};

struct amdgv_common_ioctl_in{
	union amdgv_cmd_device_bdf bdf;
	uint8_t ctl_node[AMDGV_COMMON_IOCTL_NODE_SIZE];
	uint8_t ctl_cmd[AMDGV_COMMON_IOCTL_CMD_SIZE];
};

struct amdgv_common_ioctl_out{
	uint32_t buffer_size;
	uint8_t clt_output[AMDGV_COMMON_IOCTL_OUT_SIZE];
};

struct cmd_header {
	uint32_t cmd_id;
	uint32_t thread_fd;
	uint32_t primary_fd;
	uint32_t pid;
	uint32_t reserved[8];
};

struct amdgv_cmd {
	uint32_t cmd_id;
	uint32_t input_size;
	uint32_t output_size;
	uint8_t version;
	uint8_t cmd_res;
	uint32_t pid;
	uint32_t reserved[3];
	uint8_t input_buff_raw[AMDGV_CMD_MAX_IN_SIZE];
	uint8_t output_buff_raw[AMDGV_CMD_MAX_OUT_SIZE];
};
#pragma pack(pop)

#endif //_AMDGV_CMD_H_
