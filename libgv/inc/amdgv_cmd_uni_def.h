/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_CMD_UNI_DEF__H_
#define AMDGV_CMD_UNI_DEF__H_

#define AMDGV_RAS_MAX_NUM_SAFE_RANGES 64
#define AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP 32
#define AMDGV_CMD_MAX_IN_SIZE 128
#define AMDGV_CMD_MAX_OUT_SIZE 1600
#define AMDGV_INTERFACE_MAJOR_VERSION  3
#define AMDGV_INTERFACE_MINOR_VERSION  1
#define AMDGV_CMD_VERSION_V1 1
#define AMDGV_CMD_VERSION_V2 2
#define AMDGV_CMD_MAX_GPU_NUM 32

enum unify_ioctl_type {
	AMDGV_UNI_IOCTL = 2 << 24,
};

enum amdgv_cmd_ras_id {
	AMDGV_CMD_QUERY_INTERFACE_VERSION = AMDGV_UNI_IOCTL | 0x0,
	AMDGV_CMD_GET_DEVICES_INFO = AMDGV_UNI_IOCTL | 0x001,
	AMDGV_CMD_GET_BLOCK_ECC_STATUS = AMDGV_UNI_IOCTL | 0x002,
	AMDGV_CMD_RAS_INJECT_ERROR = AMDGV_UNI_IOCTL | 0x003,
	AMDGV_CMD_RAS_ENABLE = AMDGV_UNI_IOCTL | 0x004,
	AMDGV_CMD_GET_BAD_PAGES = AMDGV_UNI_IOCTL | 0x005,
	AMDGV_CMD_CLEAR_BAD_PAGE_INFO = AMDGV_UNI_IOCTL | 0x006,
	AMDGV_CMD_GPU_RESET = AMDGV_UNI_IOCTL | 0x007,
	AMDGV_CMD_GET_FB_PF_REGIONS = AMDGV_UNI_IOCTL | 0x008,
	AMDGV_CMD_GET_FB_VF_REGIONS = AMDGV_UNI_IOCTL | 0x009,
	AMDGV_CMD_GET_VF_BDF = AMDGV_UNI_IOCTL | 0x00a,
	AMDGV_CMD_RAS_TA_LOAD = AMDGV_UNI_IOCTL | 0x00b,
	AMDGV_CMD_RAS_TA_UNLOAD = AMDGV_UNI_IOCTL | 0x00c,
	AMDGV_CMD_RAS_GET_SAFE_FB_ADDRESS_RANGES = AMDGV_UNI_IOCTL | 0x00d,
	AMDGV_CMD_TRANSLATE_FB_ADDRESS = AMDGV_UNI_IOCTL | 0x00e,
	AMDGV_CMD_RAS_RESET_ALL_ERROR_COUNTS = AMDGV_UNI_IOCTL | 0x00f,
	AMDGV_CMD_GET_LINK_TOPOLOGY = AMDGV_UNI_IOCTL | 0x011,
	AMDGV_CMD_GET_CPER_RECORDS = AMDGV_UNI_IOCTL | 0x012,
	AMDGV_CMD_GET_RAS_POLICY_INFO = AMDGV_UNI_IOCTL | 0x013,
	AMDGV_CMD_SUPPORTED_MAX
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
	AMDGV_CMD_CHIP_MI200 = 2,
	AMDGV_CMD_CHIP_NAVI32 = 8,
	AMDGV_CMD_CHIP_MI300X = 9,
	AMDGV_CMD_CHIP_MI308X = 11,
	AMDGV_CMD_CHIP_MI350X = 12,
	AMDGV_CMD_CHIP_MI325X = 13,
	AMDGV_CMD_CHIP_UNKNOWN,
	AMDGV_CMD_CHIP_LAST,
};

enum amdgv_ras_ta_load_status {
	AMDGV_RAS_TA_STATUS_NO_CHANGE,
	AMDGV_RAS_TA_STATUS_UPGRADED,
	AMDGV_RAS_TA_STATUS_DOWNGRADED,
	AMDGV_RAS_TA_STATUS_LOADED
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
	AMDGV_RAS_BLOCK__MMSCH,
	AMDGV_RAS_BLOCK_MAX
};

enum amdgv_ras_error_type {
    AMDGV_RAS_TYPE_ERROR__NONE = 0,
    AMDGV_RAS_TYPE_ERROR__PARITY = 1,
    AMDGV_RAS_TYPE_ERROR__SINGLE_CORRECTABLE = 2,
    AMDGV_RAS_TYPE_ERROR__MULTI_UNCORRECTABLE = 4,
    AMDGV_RAS_TYPE_ERROR__POISON = 8,
};

enum amdgv_ras_eeprom_err_type {
	AMDGV_RAS_EEPROM_ERR_PLACE_HOLDER,
	AMDGV_RAS_EEPROM_ERR_RECOVERABLE,
	AMDGV_RAS_EEPROM_ERR_NON_RECOVERABLE
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

struct amdgv_cmd_ecc_count {
    uint32_t corr_error_cnt;
    uint32_t uncorr_error_cnt;
    uint32_t deferred_error_cnt;
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
	AMDGV_FB_ADDR_SOC_PHY, /* SPA */
	AMDGV_FB_ADDR_BANK,
	AMDGV_FB_ADDR_VF_PHY, /* GPA */
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

struct amdgv_cmd_dev_handle {
    uint64_t dev_handle;
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

typedef struct amdgv_dev_pair_info {
	struct amdgv_cmd_dev_handle src;
	struct amdgv_cmd_dev_handle dst;
} amdgv_dev_pair_info;

typedef struct amdgv_dev_link_topology {
	uint64_t weight;     //!< link weight
	uint8_t link_status; //!< HW status of the link
	uint8_t link_type;   //!< type of the link
	uint8_t num_hops;    //!< number of hops
	uint8_t fb_sharing;  //!< framebuffer sharing flag
	uint32_t reserved[7];
} amdgv_dev_link_topology;

struct amdgv_cmd_ras_ta_load_req {
	struct amdgv_cmd_dev_handle dev;
	uint32_t version;
	uint32_t data_len;
	uint64_t data_addr;
	uint32_t reserved[2];
};

struct amdgv_cmd_ras_ta_load_rsp {
	uint64_t ras_session_id; // starts from 0
	enum amdgv_ras_ta_load_status ta_status;
	uint32_t reserved[5];
};

struct amdgv_cmd_ras_ta_unload_req {
	struct amdgv_cmd_dev_handle dev;
	uint64_t ras_session_id;
	uint32_t reserved[4];
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

struct amdgv_cmd_req_bad_pages_group {
    struct amdgv_cmd_dev_handle device;
    uint32_t group_index;
};

struct amdgv_get_cper_records_input {
	uint8_t *buf;
	uint64_t dev_handle;
	uint64_t buf_size;
	uint64_t rptr;
};

struct amdgv_get_cper_records_output {
	uint64_t write_count;
	uint64_t overflow_count;
	uint64_t left_size;
};

struct amdgv_query_interface_version_req {
	uint32_t reserved[8];
};

struct amdgv_query_interface_version_rsp {
	uint8_t major_ver;    // interface major
	uint8_t minor_ver;    // interface minor
	uint8_t reserved[26];
};

struct amdgv_cmd_dev_info {
    uint64_t dev_handle;
    uint32_t bdf;
    uint32_t ecc_enabled;
    uint32_t ecc_supported;
    uint32_t vf_num;
    uint32_t asic_type;
};

struct amdgv_cmd_dev_info_ex {
    uint32_t oam_id;
    uint32_t ras_eeprom_version;	/* RAS EEPROM version (0 if unavailable) */
    uint32_t reserved[1];
};

struct amdgv_cmd_devices_info {
    struct amdgv_cmd_dev_info devs[AMDGV_CMD_MAX_GPU_NUM];
    uint8_t dev_num;
	// below is supported when cmd_id version >= 2
    struct amdgv_cmd_dev_info_ex devs_ex[AMDGV_CMD_MAX_GPU_NUM];
};

#pragma pack(push, 8)
struct amdgv_uni_cmd {
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

struct amdgv_cmd_ras_policy_info {
	uint8_t minor_version;
	uint8_t major_version;
	uint8_t padding[2];
	uint16_t dram_non_critical_region_threshold;	// Non-critical region UCE threshold
	uint16_t dram_critical_region_threshold;		// Critical region UCE threshold
	uint32_t reserved[8];
};

uint8_t amdgv_handle_uni_cmd(void *data, struct amdgv_uni_cmd *cmd);

#endif
