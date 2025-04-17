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
 * THE SOFTWARE.
 */

#ifndef __SMI_CMD_H__
#define __SMI_CMD_H__

#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#endif

#include "smi_cmd_ioctl.h"
#include "smi_device_handle.h"
#include "smi_handle.h"
#ifndef _WIN64
#include "gim_ioctl.h"
#endif


#ifndef __linux__
#ifdef _WIN64
#pragma pack(push, 8)
#define SMI_IOCTL 0
#else
#pragma pack(push, 1)
#endif
#endif

/* Code definitions */
enum smi_cmd_code {
	SMI_CMD_CODE_HANDSHAKE					= SMI_IOCTL | 0x00000001,
	SMI_CMD_CODE_GET_SERVER_STATIC_INFO			= SMI_IOCTL | 0x00000002,
	SMI_CMD_CODE_GET_VF_PARTITIONING_INFO			= SMI_IOCTL | 0x00000004,
	SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO			= SMI_IOCTL | 0x00000005,
	SMI_CMD_CODE_GET_VF_STATIC_INFO				= SMI_IOCTL | 0x00000006,
	SMI_CMD_CODE_GET_VF_DYNAMIC_INFO			= SMI_IOCTL | 0x00000007,
	SMI_CMD_CODE_CREATE_EVENT				= SMI_IOCTL | 0x00000008 | SMI_CMD_FD_CLI2SER_MASK,
	SMI_CMD_CODE_GET_ECC_STATUS				= SMI_IOCTL | 0x00000009,
	SMI_CMD_CODE_GET_HANDLE					= SMI_IOCTL | 0x0000000A,
	SMI_CMD_CODE_GET_BAD_PAGE_INFO				= SMI_IOCTL | 0x0000000B,
	SMI_CMD_CODE_GET_GUEST_DATA				= SMI_IOCTL | 0x0000000C,
	SMI_CMD_CODE_GET_DFC_FW_TABLE				= SMI_IOCTL | 0x0000000D,
	SMI_CMD_CODE_GET_PCIE_INFO				= SMI_IOCTL | 0x0000000E,
	SMI_CMD_CODE_GET_UCODE_ERR_RECORDS			= SMI_IOCTL | 0x0000000F,
	SMI_CMD_CODE_GET_VF_UCODE_INFO				= SMI_IOCTL | 0x00000010,
	SMI_CMD_CODE_GET_PARTITION_PROFILE_INFO			= SMI_IOCTL | 0x00000011,
	SMI_CMD_CODE_GET_BLOCK_ECC_STATUS			= SMI_IOCTL | 0x00000012,
	SMI_CMD_CODE_GET_GPU_FW_INFO				= SMI_IOCTL | 0x00000013,
	SMI_CMD_CODE_GET_SMI_DATA  				= SMI_IOCTL | 0x00000014,
	SMI_CMD_CODE_GET_LINK_METRICS				= SMI_IOCTL | 0x00000015,
	SMI_CMD_CODE_GET_LINK_TOPOLOGY				= SMI_IOCTL | 0x00000016,
	SMI_CMD_CODE_GET_XGMI_FB_SHARING_CAPS			= SMI_IOCTL | 0x00000017,
	SMI_CMD_CODE_GET_XGMI_FB_SHARING_MODE_INFO		= SMI_IOCTL | 0x00000018,
	SMI_CMD_CODE_SET_XGMI_FB_SHARING_MODE			= SMI_IOCTL | 0x00000019,
	SMI_CMD_CODE_READ_EVENT					= SMI_IOCTL | 0x0000001A,
	SMI_CMD_CODE_DESTROY_EVENT				= SMI_IOCTL | 0x0000001B,
	SMI_CMD_CODE_GET_RAS_FEATURE_INFO			= SMI_IOCTL | 0x0000001C,
	SMI_CMD_CODE_SET_VF_PARTITIONING_INFO			= SMI_IOCTL | 0x0000001D,
	SMI_CMD_CODE_GET_METRICS_TABLE				= SMI_IOCTL | 0x0000001E,
	SMI_CMD_CODE_CLEAR_VF_FB				= SMI_IOCTL | 0x0000001F,
	SMI_CMD_CODE_SET_XGMI_FB_SHARING_MODE_VER_2		= SMI_IOCTL | 0x00000020,
	SMI_CMD_CODE_GET_ACCELERATOR_PARTITION_PROFILE_CONFIG	= SMI_IOCTL | 0x00000021,
	SMI_CMD_CODE_GET_GPU_ACCELERATOR_PARTITION		= SMI_IOCTL | 0x00000022,
	SMI_CMD_CODE_GET_CURR_MEMORY_PARTITION_SETTING		= SMI_IOCTL | 0x00000023,
	SMI_CMD_CODE_SET_GPU_ACCELERATOR_PARTITION_SETTING	= SMI_IOCTL | 0x00000024,
	SMI_CMD_CODE_SET_GPU_MEMORY_PARTITION_SETTING		= SMI_IOCTL | 0x00000025,
	SMI_CMD_CODE_GET_SOC_PSTATE				= SMI_IOCTL | 0x00000026,
	SMI_CMD_CODE_SET_SOC_PSTATE				= SMI_IOCTL | 0x00000027,
	SMI_CMD_CODE_GET_GPU_DRIVER_MODEL			= SMI_IOCTL | 0x00000028,
	SMI_CMD_CODE_SET_GPU_POWER_CAP				= SMI_IOCTL | 0x00000029,
	SMI_CMD_CODE_GET_CPER					= SMI_IOCTL | 0x0000002A,
	SMI_CMD_CODE_GET_VBIOS_INFO				= SMI_IOCTL | 0x0000002B,
	SMI_CMD_CODE_GET_BOARD_INFO				= SMI_IOCTL | 0x0000002C,
	SMI_CMD_CODE_GET_ASIC_INFO				= SMI_IOCTL | 0x0000002D,
	SMI_CMD_CODE_GET_VRAM_INFO				= SMI_IOCTL | 0x0000002E,
	SMI_CMD_CODE_GET_GPU_DRIVER_INFO			= SMI_IOCTL | 0x0000002F,
	SMI_CMD_CODE_GET_POWER_CAP_INFO				= SMI_IOCTL | 0x00000030,
	SMI_CMD_CODE_GET_PF_FB_INFO				= SMI_IOCTL | 0x00000031,
	SMI_CMD_CODE_GET_GPU_CACHE_INFO				= SMI_IOCTL | 0x00000032,
	SMI_CMD_CODE__MAX					= 0xffffffff
};

/* Version Info */

#define SMI_VERSION_ALPHA_0 0x00000002
#define SMI_VERSION_BETA_0  0x00000003
#define SMI_VERSION_BETA_1  0x00000004
#define SMI_VERSION_BETA_2  0x00000005
#define SMI_VERSION_BETA_3  0x00000006
#define SMI_VERSION_BETA_4  0x00000007

#define SMI_MAX_CMD_V1 256
#define SMI_MAX_CMD SMI_MAX_CMD_V1

#define SMI_VERSION_BETA_1_NUM_CMD SMI_MAX_CMD

#define SMI_VERSION_MIN SMI_VERSION_BETA_0
#define SMI_VERSION_MAX SMI_VERSION_BETA_4

#define SMI_NOT_SUPPORTED -1

#define SMI_CONTAINER(type, name, size)                                                       \
	union {                                                                               \
		type name;                                                             \
		uint8_t	    raw_##name[size];                                                 \
	}


#define SMI_CONTAINER_ARRAY(type, name, size, num)                                            \
	union {                                                                               \
		type name[num];                                                        \
		uint8_t	    raw_##name[size];                                                 \
	}

// Mapped AMDSMI library defines
#define SMI_MAX_DEVICES 32
#define SMI_MAX_VF_COUNT 32
#define SMI_PF_INDEX	(SMI_MAX_VF_COUNT - 1)
#define SMI_MAX_MM_IP_COUNT 8
#define SMI_MAX_STRING_LENGTH 256
#define SMI_MAX_DRIVER_INFO_RSVD 64
#define SMI_MAX_CACHE_TYPES 10
#define SMI_MAX_NUM_XGMI_PHYSICAL_LINK 64
#define SMI_MAX_NUM_PM_POLICIES 32
#define SMI_EVENT_MSG_SIZE 256
#define SMI_DFC_FW_NUMBER_OF_ENTRIES 9
#define SMI_MAX_WHITE_LIST_ELEMENTS 16
#define SMI_MAX_BLACK_LIST_ELEMENTS 64
#define SMI_MAX_TA_WHITE_LIST_ELEMENTS 8
#define SMI_MAX_ERR_RECORDS 10
#define SMI_MAX_PROFILE_COUNT 16
#define SMI_MAX_UUID_ELEMENTS 16
#define SMI_MAX_NAME 32

/**
 * @brief string format
 */
#define SMI_TIME_FORMAT "%02d:%02d:%02d.%03d"
#define SMI_DATE_FORMAT "%04d-%02d-%02d:%02d:%02d:%02d.%03d"
//! YYYY-MM-DD:HH:MM:SS.MSC
#define SMI_MAX_DATE_LENGTH 32


#define SMI_MAX_NUM_METRICS_V1 255
#define SMI_MAX_NUM_METRICS SMI_MAX_NUM_METRICS_V1
#define SMI_MAX_BAD_PAGE_RECORD_V1 512
#define SMI_MAX_BAD_PAGE_RECORD_V2 16384
#define SMI_MAX_BAD_PAGE_RECORD SMI_MAX_BAD_PAGE_RECORD_V2

#define SMI_MAX_CP_PROFILE_RESOURCES  32
#define SMI_MAX_ACCELERATOR_PARTITIONS 8
#define SMI_MAX_ACCELERATOR_PROFILE 32
#define SMI_MAX_NUM_NUMA_NODES 32

#define SMI_MAX_CPER_SIZE (10*1024)
#define SMI_MAX_CPER_HDRS 10

// >>>>>>>>>>>>>>>>>>>> ENUM TYPE DEFINITIONS >>>>>>>>>>>>>>>>>>>>

// Mapped AMDSMI library enums

enum smi_status {
	SMI_STATUS_SUCCESS = 0,  //!< Call succeeded
	// Library usage errors
	SMI_STATUS_INVAL = 1,  //!< Invalid parameters
	SMI_STATUS_NOT_SUPPORTED = 2,  //!< Command not supported
	SMI_STATUS_NOT_YET_IMPLEMENTED = 3,  //!< Not implemented yet
	SMI_STATUS_FAIL_LOAD_MODULE = 4,  //!< Fail to load lib
	SMI_STATUS_FAIL_LOAD_SYMBOL = 5,  //!< Fail to load symbol
	SMI_STATUS_DRM_ERROR = 6,  //!< Error when call libdrm
	SMI_STATUS_API_FAILED = 7,  //!< API call failed
	SMI_STATUS_TIMEOUT = 8,  //!< Timeout in API call
	SMI_STATUS_RETRY = 9,  //!< Retry operation
	SMI_STATUS_NO_PERM = 10,  //!< Permission Denied
	SMI_STATUS_INTERRUPT = 11,  //!< An interrupt occurred during execution of function
	SMI_STATUS_IO = 12,  //!< I/O Error
	SMI_STATUS_ADDRESS_FAULT = 13,  //!< Bad address
	SMI_STATUS_FILE_ERROR = 14,  //!< Problem accessing a file
	SMI_STATUS_OUT_OF_RESOURCES = 15,  //!< Not enough memory
	SMI_STATUS_INTERNAL_EXCEPTION = 16,  //!< An internal exception was caught
	SMI_STATUS_INPUT_OUT_OF_BOUNDS = 17,  //!< The provided input is out of allowable or safe range
	SMI_STATUS_INIT_ERROR = 18,  //!< An error occurred when initializing internal data structures
	SMI_STATUS_REFCOUNT_OVERFLOW = 19,  //!< An internal reference counter exceeded INT32_MAX
	SMI_STATUS_MORE_DATA = 20,
	// Processor related errors
	SMI_STATUS_BUSY = 30,  //!< Processor busy
	SMI_STATUS_NOT_FOUND = 31,  //!< Processor not found
	SMI_STATUS_NOT_INIT = 32,  //!< Processor not initialized
	SMI_STATUS_NO_SLOT = 33,  //!< No more free slot
	SMI_STATUS_DRIVER_NOT_LOADED = 34, //!< Processor driver not loaded
	// Data and size errors
	SMI_STATUS_NO_DATA = 40,  //!< No data was found for a given input
	SMI_STATUS_INSUFFICIENT_SIZE = 41,  //!< Not enough resources were available for the operation
	SMI_STATUS_UNEXPECTED_SIZE = 42,  //!< An unexpected amount of data was read
	SMI_STATUS_UNEXPECTED_DATA = 43,  //!< The data read or provided to function is not what was expected
	//esmi errors
	SMI_STATUS_NON_AMD_CPU = 44, //!< System has different cpu than AMD
	SMI_STATUS_NO_ENERGY_DRV = 45, //!< Energy driver not found
	SMI_STATUS_NO_MSR_DRV = 46, //!< MSR driver not found
	SMI_STATUS_NO_HSMP_DRV = 47, //!< HSMP driver not found
	SMI_STATUS_NO_HSMP_SUP = 48, //!< HSMP not supported
	SMI_STATUS_NO_HSMP_MSG_SUP = 49, //!< HSMP message/feature not supported
	SMI_STATUS_HSMP_TIMEOUT = 50,  //!< HSMP message is timedout
	SMI_STATUS_NO_DRV = 51,  //!< No Energy and HSMP driver present
	SMI_STATUS_FILE_NOT_FOUND = 52, //!< file or directory not found
	SMI_STATUS_ARG_PTR_NULL = 53,   //!< Parsed argument is invalid
	SMI_STATUS_AMDGPU_RESTART_ERR = 54, //!< AMDGPU restart failed
	SMI_STATUS_SETTING_UNAVAILABLE = 55, //!< Setting is not available

	// General errors
	SMI_STATUS_MAP_ERROR = 0xFFFFFFFE,  //!< The internal library error did not map to a status code
	SMI_STATUS_UNKNOWN_ERROR = 0xFFFFFFFF,  //!< An unknown error occurred
};

enum smi_driver_model_type {
	SMI_DRIVER_MODEL_TYPE_WDDM = 0,
	SMI_DRIVER_MODEL_TYPE_WDM  = 1,
	SMI_DRIVER_MODEL_TYPE_MCDM = 2,
	SMI_DRIVER_MODEL_TYPE__MAX = 3,
};

enum smi_guard_state {
	SMI_GUARD_STATE_NORMAL   = 0,
	SMI_GUARD_STATE_FULL     = 1,
	SMI_GUARD_STATE_OVERFLOW = 2,
};

enum smi_vf_sched_state {
	SMI_VF_STATE_UNAVAILABLE,
	SMI_VF_STATE_AVAILABLE,
	SMI_VF_STATE_ACTIVE,
	SMI_VF_STATE_SUSPENDED,
	SMI_VF_STATE_FULLACCESS,
	SMI_VF_STATE_DEFAULT_AVAILABLE,
};

enum smi_metric_unit {
	SMI_METRIC_UNIT_COUNTER,
	SMI_METRIC_UNIT_UINT,
	SMI_METRIC_UNIT_BOOL,
	SMI_METRIC_UNIT_MHZ,
	SMI_METRIC_UNIT_PERCENT,
	SMI_METRIC_UNIT_MILLIVOLT,
	SMI_METRIC_UNIT_CELSIUS,
	SMI_METRIC_UNIT_WATT,
	SMI_METRIC_UNIT_JOULE,
	SMI_METRIC_UNIT_GBPS,
	SMI_METRIC_UNIT_MBITPS,
	SMI_METRIC_UNIT_PCIE_GEN,
	SMI_METRIC_UNIT_PCIE_LANES,
	SMI_METRIC_UNIT_UNKNOWN
};

enum smi_metric_name {
	SMI_METRIC_NAME_METRIC_ACC_COUNTER,
	SMI_METRIC_NAME_FW_TIMESTAMP,
	SMI_METRIC_NAME_CLK_GFX,
	SMI_METRIC_NAME_CLK_SOC,
	SMI_METRIC_NAME_CLK_MEM,
	SMI_METRIC_NAME_CLK_VCLK,
	SMI_METRIC_NAME_CLK_DCLK,

	SMI_METRIC_NAME_USAGE_GFX,
	SMI_METRIC_NAME_USAGE_MEM,
	SMI_METRIC_NAME_USAGE_MM,
	SMI_METRIC_NAME_USAGE_VCN,
	SMI_METRIC_NAME_USAGE_JPEG,

	SMI_METRIC_NAME_VOLT_GFX,
	SMI_METRIC_NAME_VOLT_SOC,
	SMI_METRIC_NAME_VOLT_MEM,

	SMI_METRIC_NAME_TEMP_HOTSPOT_CURR,
	SMI_METRIC_NAME_TEMP_HOTSPOT_LIMIT,
	SMI_METRIC_NAME_TEMP_MEM_CURR,
	SMI_METRIC_NAME_TEMP_MEM_LIMIT,
	SMI_METRIC_NAME_TEMP_VR_CURR,
	SMI_METRIC_NAME_TEMP_SHUTDOWN,

	SMI_METRIC_NAME_POWER_CURR,
	SMI_METRIC_NAME_POWER_LIMIT,

	SMI_METRIC_NAME_ENERGY_SOCKET,
	SMI_METRIC_NAME_ENERGY_CCD,
	SMI_METRIC_NAME_ENERGY_XCD,
	SMI_METRIC_NAME_ENERGY_AID,
	SMI_METRIC_NAME_ENERGY_MEM,

	SMI_METRIC_NAME_THROTTLE_SOCKET_ACTIVE,
	SMI_METRIC_NAME_THROTTLE_VR_ACTIVE,
	SMI_METRIC_NAME_THROTTLE_MEM_ACTIVE,

	SMI_METRIC_NAME_PCIE_BANDWIDTH,
	SMI_METRIC_NAME_PCIE_L0_TO_RECOVERY_COUNT,
	SMI_METRIC_NAME_PCIE_REPLAY_COUNT,
	SMI_METRIC_NAME_PCIE_REPLAY_ROLLOVER_COUNT,
	SMI_METRIC_NAME_PCIE_NAK_SENT_COUNT,
	SMI_METRIC_NAME_PCIE_NAK_RECEIVED_COUNT,

	SMI_METRIC_NAME_CLK_GFX_MAX_LIMIT,
	SMI_METRIC_NAME_CLK_SOC_MAX_LIMIT,
	SMI_METRIC_NAME_CLK_MEM_MAX_LIMIT,
	SMI_METRIC_NAME_CLK_VCLK_MAX_LIMIT,
	SMI_METRIC_NAME_CLK_DCLK_MAX_LIMIT,

	SMI_METRIC_NAME_CLK_GFX_MIN_LIMIT,
	SMI_METRIC_NAME_CLK_SOC_MIN_LIMIT,
	SMI_METRIC_NAME_CLK_MEM_MIN_LIMIT,
	SMI_METRIC_NAME_CLK_VCLK_MIN_LIMIT,
	SMI_METRIC_NAME_CLK_DCLK_MIN_LIMIT,

	SMI_METRIC_NAME_CLK_GFX_LOCKED,

	SMI_METRIC_NAME_CLK_GFX_DS_DISABLED,
	SMI_METRIC_NAME_CLK_MEM_DS_DISABLED,
	SMI_METRIC_NAME_CLK_SOC_DS_DISABLED,
	SMI_METRIC_NAME_CLK_VCLK_DS_DISABLED,
	SMI_METRIC_NAME_CLK_DCLK_DS_DISABLED,

	SMI_METRIC_NAME_PCIE_LINK_SPEED,
	SMI_METRIC_NAME_PCIE_LINK_WIDTH,

	SMI_METRIC_NAME_UNKNOWN
};

enum smi_metric_category {
	SMI_METRIC_CATEGORY_ACC_COUNTER,
	SMI_METRIC_CATEGORY_FREQUENCY,
	SMI_METRIC_CATEGORY_ACTIVITY,
	SMI_METRIC_CATEGORY_TEMPERATURE,
	SMI_METRIC_CATEGORY_POWER,
	SMI_METRIC_CATEGORY_ENERGY,
	SMI_METRIC_CATEGORY_THROTTLE,
	SMI_METRIC_CATEGORY_PCIE,
	SMI_METRIC_CATEGORY_UNKNOWN
};

enum smi_memory_partition_type {
	SMI_MEMORY_PARTITION_UNKNOWN = 0,
	SMI_MEMORY_PARTITION_NPS1 = 1,  //!< NPS1 - All CCD & XCD data is interleaved
							//!< across all 8 HBM stacks (all stacks/1).
	SMI_MEMORY_PARTITION_NPS2 = 2,  //!< NPS2 - 2 sets of CCDs or 4 XCD interleaved
							//!< across the 4 HBM stacks per AID pair
							//!< (8 stacks/2).
	SMI_MEMORY_PARTITION_NPS4 = 4,  //!< NPS4 - Each XCD data is interleaved across
							//!< across 2 (or single) HBM stacks
							//!< (8 stacks/8 or 8 stacks/4).
	SMI_MEMORY_PARTITION_NPS8 = 8,  //!< NPS8 - Each XCD uses a single HBM stack
							//!< (8 stacks/8). Or each XCD uses a single
							//!< HBM stack & CCDs share 2 non-interleaved
							//!< HBM stacks on its AID
							//!< (AID[1,2,3] = 6 stacks/6).
};

enum smi_accelerator_partition_resource_type {
	SMI_ACCELERATOR_XCC,
	SMI_ACCELERATOR_ENCODER,
	SMI_ACCELERATOR_DECODER,
	SMI_ACCELERATOR_DMA,
	SMI_ACCELERATOR_JPEG,
	SMI_ACCELERATOR_MAX
};

enum smi_accelerator_partition_type {
	SMI_ACCELERATOR_PARTITION_INVALID = 0,
	SMI_ACCELERATOR_PARTITION_SPX,        //!< Single GPU mode (SPX)- All XCCs work
										//!< together with shared memory
	SMI_ACCELERATOR_PARTITION_DPX,        //!< Dual GPU mode (DPX)- Half XCCs work
										//!< together with shared memory
	SMI_ACCELERATOR_PARTITION_TPX,        //!< Triple GPU mode (TPX)- One-third XCCs
										//!< work together with shared memory
	SMI_ACCELERATOR_PARTITION_QPX,        //!< Quad GPU mode (QPX)- Quarter XCCs
										//!< work together with shared memory
	SMI_ACCELERATOR_PARTITION_CPX,        //!< Core mode (CPX)- Per-chip XCC with
										//!< shared memory
	SMI_ACCELERATOR_PARTITION_MAX
};

enum smi_temperature_type {
	SMI_TEMPERATURE_TYPE_EDGE,
	SMI_TEMPERATURE_TYPE_FIRST = SMI_TEMPERATURE_TYPE_EDGE,
	SMI_TEMPERATURE_TYPE_HOTSPOT,
	SMI_TEMPERATURE_TYPE_JUNCTION = SMI_TEMPERATURE_TYPE_HOTSPOT,
	SMI_TEMPERATURE_TYPE_VRAM,
	SMI_TEMPERATURE_TYPE_HBM_0,
	SMI_TEMPERATURE_TYPE_HBM_1,
	SMI_TEMPERATURE_TYPE_HBM_2,
	SMI_TEMPERATURE_TYPE_HBM_3,
	SMI_TEMPERATURE_TYPE_PLX,
	SMI_TEMPERATURE_TYPE__MAX = SMI_TEMPERATURE_TYPE_PLX
};

enum smi_clk_type {
	SMI_CLK_TYPE_SYS = 0x0,	//!< System clock
	SMI_CLK_TYPE_FIRST = SMI_CLK_TYPE_SYS,
	SMI_CLK_TYPE_GFX = SMI_CLK_TYPE_SYS,
	SMI_CLK_TYPE_DF,		//!< Data Fabric clock (for ASICs
					//!< running on a separate clock)
	SMI_CLK_TYPE_DCEF,		//!< Display Controller Engine clock
	SMI_CLK_TYPE_SOC,
	SMI_CLK_TYPE_MEM,
	SMI_CLK_TYPE_PCIE,
	SMI_CLK_TYPE_VCLK0,
	SMI_CLK_TYPE_VCLK1,
	SMI_CLK_TYPE_DCLK0,
	SMI_CLK_TYPE_DCLK1,
	SMI_CLK_TYPE__MAX = SMI_CLK_TYPE_DCLK1
};

enum smi_vram_type {
	SMI_VRAM_TYPE_UNKNOWN = 0,
	// HBM
	SMI_VRAM_TYPE_HBM = 1,
	SMI_VRAM_TYPE_HBM2 = 2,
	SMI_VRAM_TYPE_HBM2E = 3,
	SMI_VRAM_TYPE_HBM3 = 4,
	// DDR
	SMI_VRAM_TYPE_DDR2 = 10,
	SMI_VRAM_TYPE_DDR3 = 11,
	SMI_VRAM_TYPE_DDR4 = 12,
	// GDDR
	SMI_VRAM_TYPE_GDDR1 = 17,
	SMI_VRAM_TYPE_GDDR2 = 18,
	SMI_VRAM_TYPE_GDDR3 = 19,
	SMI_VRAM_TYPE_GDDR4 = 20,
	SMI_VRAM_TYPE_GDDR5 = 21,
	SMI_VRAM_TYPE_GDDR6 = 22,
	SMI_VRAM_TYPE_GDDR7 = 23,
};

enum smi_vram_vendor {
	SMI_VRAM_VENDOR_SAMSUNG,
	SMI_VRAM_VENDOR_INFINEON,
	SMI_VRAM_VENDOR_ELPIDA,
	SMI_VRAM_VENDOR_ETRON,
	SMI_VRAM_VENDOR_NANYA,
	SMI_VRAM_VENDOR_HYNIX,
	SMI_VRAM_VENDOR_MOSEL,
	SMI_VRAM_VENDOR_WINBOND,
	SMI_VRAM_VENDOR_ESMT,
	SMI_VRAM_VENDOR_MICRON,
	SMI_VRAM_VENDOR_UNKNOWN
};

enum smi_card_form_factor {
	SMI_CARD_FORM_FACTOR_PCIE,
	SMI_CARD_FORM_FACTOR_OAM,
	SMI_CARD_FORM_FACTOR_CEM,
	SMI_CARD_FORM_FACTOR_UNKNOWN
};

enum smi_fw_block {
	SMI_FW_ID_SMU = 1,
	SMI_FW_ID_FIRST = SMI_FW_ID_SMU,
	SMI_FW_ID_CP_CE,
	SMI_FW_ID_CP_PFP,
	SMI_FW_ID_CP_ME,
	SMI_FW_ID_CP_MEC_JT1,
	SMI_FW_ID_CP_MEC_JT2,
	SMI_FW_ID_CP_MEC1,
	SMI_FW_ID_CP_MEC2,
	SMI_FW_ID_RLC,
	SMI_FW_ID_SDMA0,
	SMI_FW_ID_SDMA1,
	SMI_FW_ID_SDMA2,
	SMI_FW_ID_SDMA3,
	SMI_FW_ID_SDMA4,
	SMI_FW_ID_SDMA5,
	SMI_FW_ID_SDMA6,
	SMI_FW_ID_SDMA7,
	SMI_FW_ID_VCN,
	SMI_FW_ID_UVD,
	SMI_FW_ID_VCE,
	SMI_FW_ID_ISP,
	SMI_FW_ID_DMCU_ERAM, //!< eRAM
	SMI_FW_ID_DMCU_ISR,  //!< ISR
	SMI_FW_ID_RLC_RESTORE_LIST_GPM_MEM,
	SMI_FW_ID_RLC_RESTORE_LIST_SRM_MEM,
	SMI_FW_ID_RLC_RESTORE_LIST_CNTL,
	SMI_FW_ID_RLC_V,
	SMI_FW_ID_MMSCH,
	SMI_FW_ID_PSP_SYSDRV,
	SMI_FW_ID_PSP_SOSDRV,
	SMI_FW_ID_PSP_TOC,
	SMI_FW_ID_PSP_KEYDB,
	SMI_FW_ID_DFC,
	SMI_FW_ID_PSP_SPL,
	SMI_FW_ID_DRV_CAP,
	SMI_FW_ID_MC,
	SMI_FW_ID_PSP_BL,
	SMI_FW_ID_CP_PM4,
	SMI_FW_ID_RLC_P,
	SMI_FW_ID_SEC_POLICY_STAGE2,
	SMI_FW_ID_REG_ACCESS_WHITELIST,
	SMI_FW_ID_IMU_DRAM,
	SMI_FW_ID_IMU_IRAM,
	SMI_FW_ID_SDMA_TH0,
	SMI_FW_ID_SDMA_TH1,
	SMI_FW_ID_CP_MES,
	SMI_FW_ID_MES_STACK,
	SMI_FW_ID_MES_THREAD1, //!< FW_ID_MES_THREAD1 = CP_MES_KIQ
	SMI_FW_ID_MES_THREAD1_STACK, //! FW_ID_MES_THREAD1_STACK = MES_KIQ_STACK
	SMI_FW_ID_RLX6,
	SMI_FW_ID_RLX6_DRAM_BOOT,
	SMI_FW_ID_RS64_ME,
	SMI_FW_ID_RS64_ME_P0_DATA,
	SMI_FW_ID_RS64_ME_P1_DATA,
	SMI_FW_ID_RS64_PFP,
	SMI_FW_ID_RS64_PFP_P0_DATA,
	SMI_FW_ID_RS64_PFP_P1_DATA,
	SMI_FW_ID_RS64_MEC,
	SMI_FW_ID_RS64_MEC_P0_DATA,
	SMI_FW_ID_RS64_MEC_P1_DATA,
	SMI_FW_ID_RS64_MEC_P2_DATA,
	SMI_FW_ID_RS64_MEC_P3_DATA,
	SMI_FW_ID_PPTABLE,
	SMI_FW_ID_PSP_SOC,
	SMI_FW_ID_PSP_DBG,
	SMI_FW_ID_PSP_INTF,
	SMI_FW_ID_RLX6_CORE1,
	SMI_FW_ID_RLX6_DRAM_BOOT_CORE1,
	SMI_FW_ID_RLCV_LX7,
	SMI_FW_ID_RLC_SAVE_RESTORE_LIST,
	SMI_FW_ID_ASD,
	SMI_FW_ID_TA_RAS,
	SMI_FW_ID_XGMI,
	SMI_FW_ID_RLC_SRLG,
	SMI_FW_ID_RLC_SRLS,
	SMI_FW_ID_SMC,
	SMI_FW_ID_DMCU,
	SMI_FW_ID_PSP_RAS,
	SMI_FW_ID_P2S_TABLE,
	SMI_FW_ID__MAX
};

enum smi_guard_type {
	SMI_GUARD_EVENT_FLR,
	SMI_GUARD_EVENT_EXCLUSIVE_MOD,
	SMI_GUARD_EVENT_EXCLUSIVE_TIMEOUT,
	SMI_GUARD_EVENT_ALL_INT,
	SMI_GUARD_EVENT__MAX
};

enum smi_link_type {
	SMI_LINK_TYPE_INTERNAL,
	SMI_LINK_TYPE_XGMI,
	SMI_LINK_TYPE_PCIE,
	SMI_LINK_TYPE_NOT_APPLICABLE,
	SMI_LINK_TYPE_UNKNOWN
};

enum smi_link_status {
	SMI_LINK_STATUS_ENABLED = 0,
	SMI_LINK_STATUS_DISABLED = 1,
	SMI_LINK_STATUS_ERROR = 2
};

enum smi_ecc_correction_schema_support {
	SMI_RAS_ECC_SUPPORT_PARITY = (1 << 0),
	SMI_RAS_ECC_SUPPORT_CORRECTABLE = (1 << 1),
	SMI_RAS_ECC_SUPPORT_UNCORRECTABLE = (1 << 2),
	SMI_RAS_ECC_SUPPORT_POISON = (1 << 3)
};

enum smi_gpu_block {
	SMI_GPU_BLOCK_INVALID =	0,				//!< Used to indicate an
									//!< invalid block
	SMI_GPU_BLOCK_FIRST =	(1 << 0),

	SMI_GPU_BLOCK_UMC =		SMI_GPU_BLOCK_FIRST,		//!< UMC block
	SMI_GPU_BLOCK_SDMA =		(1 << 1),			//!< SDMA block
	SMI_GPU_BLOCK_GFX =		(1 << 2),			//!< GFX block
	SMI_GPU_BLOCK_MMHUB =	(1 << 3),			//!< MMHUB block
	SMI_GPU_BLOCK_ATHUB =	(1 << 4),			//!< ATHUB block
	SMI_GPU_BLOCK_PCIE_BIF =	(1 << 5),			//!< PCIE_BIF block
	SMI_GPU_BLOCK_HDP =		(1 << 6),			//!< HDP block
	SMI_GPU_BLOCK_XGMI_WAFL =	(1 << 7),			//!< XGMI block
	SMI_GPU_BLOCK_DF =		(1 << 8),			//!< DF block
	SMI_GPU_BLOCK_SMN =		(1 << 9),			//!< SMN block
	SMI_GPU_BLOCK_SEM =		(1 << 10),			//!< SEM block
	SMI_GPU_BLOCK_MP0 =		(1 << 11),			//!< MP0 block
	SMI_GPU_BLOCK_MP1 =		(1 << 12),			//!< MP1 block
	SMI_GPU_BLOCK_FUSE =		(1 << 13),			//!< Fuse block
	SMI_GPU_BLOCK_MCA =		(1 << 14),			//!< MCA block
	SMI_GPU_BLOCK_VCN =		(1 << 15),			//!< VCN block
	SMI_GPU_BLOCK_JPEG =		(1 << 16),			//!< JPEG block
	SMI_GPU_BLOCK_IH =		(1 << 17),			//!< IH block
	SMI_GPU_BLOCK_MPIO =		(1 << 18),			//!< MPIO block
	SMI_GPU_BLOCK_LAST =		SMI_GPU_BLOCK_MPIO,
};

enum smi_profile_capability_type {
	SMI_PROFILE_CAPABILITY_MEMORY = 0,		//!< memory
	SMI_PROFILE_CAPABILITY_ENCODE = 1,		//!< encode engine
	SMI_PROFILE_CAPABILITY_DECODE = 2,		//!< decode engine
	SMI_PROFILE_CAPABILITY_COMPUTE = 3,		//!< compute engine
	SMI_PROFILE_CAPABILITY__MAX,
};

enum smi_xgmi_fb_sharing_mode {
	SMI_XGMI_FB_SHARING_MODE_CUSTOM  = 0,
	SMI_XGMI_FB_SHARING_MODE_1	    = 1,
	SMI_XGMI_FB_SHARING_MODE_2	    = 2,
	SMI_XGMI_FB_SHARING_MODE_4	    = 4,
	SMI_XGMI_FB_SHARING_MODE_8	    = 8,
	SMI_XGMI_FB_SHARING_MODE_UNKNOWN = 0xFFFFFFFF
};

enum smi_mm_ip {
	SMI_MM_UVD,
	SMI_MM_VCE,
	SMI_MM_VCN,
	SMI_MM__MAX
};

enum smi_driver {
	SMI_DRIVER_LIBGV,
	SMI_DRIVER_KMD,
	SMI_DRIVER_AMDGPUV,
	SMI_DRIVER_AMDGPU,
	SMI_DRIVER_VMWGPUV,
	SMI_DRIVER__MAX
};

// IOCTL input/output enums

enum smi_debug_level {
	SMI_DBGL_ERROR = 0,
	SMI_DBGL_WARN,
	SMI_DBGL_INFO,
	SMI_DBGL_DEBUG,
	SMI_DBGL_TRACE,
	SMI_DBGL__MAX
};

enum smi_ras_err_status {
	SMI_RAS_ERR_STATUS_NONE = 0,	//!< No current errors
	SMI_RAS_ERR_STATUS_DISABLED,	//!< ECC is disabled
	SMI_RAS_ERR_STATUS_PARITY,	//!< ECC errors present, but type unknown
	SMI_RAS_ERR_STATUS_SING_C,	//!< Single correctable error
	SMI_RAS_ERR_STATUS_MULT_UC,	//!< Multiple uncorrectable errors
	SMI_RAS_ERR_STATUS_POISON,	//!< Firmware detected error and isolated
					//!< page. Treat as uncorrectable.
	SMI_RAS_ERR_STATE_ENABLED,	//!< ECC is enabled
	SMI_RAS_ERR_STATE_UNKNOWN = 0xFFFFFFFF
};

enum smi_data_query_type {
	SMI_POWER_MANAGEMENT,
	SMI_RAS_CAPS
};

// >>>>>>>>>>>>>>>>>>>> INPUT/OUTPUT STRUCTS >>>>>>>>>>>>>>>>>>>>

// Mapped AMDSMI library structures and unions
struct smi_vram_info {
	enum smi_vram_type vram_type;
	enum smi_vram_vendor vram_vendor;
	uint32_t vram_size;
	uint32_t vram_bit_width;
	uint64_t reserved[6];
};

struct smi_pcie_info {
	struct pcie_static__ {
		uint16_t max_pcie_width; //!< maximum number of PCIe lanes
		uint32_t max_pcie_speed; //!< maximum PCIe speed
		uint32_t pcie_interface_version; //!< PCIe interface version
		enum smi_card_form_factor slot_type; //!< card form factor
		uint32_t max_pcie_interface_version; //!< maximum PCIe link generation
		uint64_t reserved[9];
	} pcie_static;
	struct pcie_metric__ {
		uint16_t pcie_width; //!< current PCIe width
		uint32_t pcie_speed; //!< current PCIe speed in MT/s
		uint32_t pcie_bandwidth; //!< current PCIe bandwidth in Mb/s
		uint64_t pcie_replay_count; //!< total number of the replays issued on the PCIe link
		uint64_t pcie_l0_to_recovery_count; //!< total number of times the PCIe link transitioned from L0 to the recovery state
		uint64_t pcie_replay_roll_over_count; //!< total number of replay rollovers issued on the PCIe link
		uint64_t pcie_nak_sent_count; //!< total number of NAKs issued on the PCIe link by the device
		uint64_t pcie_nak_received_count; //!< total number of NAKs issued on the PCIe link by the receiver
		uint32_t pcie_lc_perf_other_end_recovery_count;  //!< PCIe other end recovery counter
		uint64_t reserved[12];
	} pcie_metric;
	uint64_t reserved[32];
};

struct smi_power_cap_info {
	uint64_t power_cap;
	uint64_t default_power_cap;
	uint64_t dpm_cap;
	uint64_t min_power_cap;
	uint64_t max_power_cap;
	uint64_t reserved[3];
};

struct smi_vbios_info {
	char name[SMI_MAX_STRING_LENGTH];
	char build_date[SMI_MAX_DATE_LENGTH];
	char part_number[SMI_MAX_STRING_LENGTH];
	char version[SMI_MAX_STRING_LENGTH];
	uint64_t reserved[68];
};

struct smi_pf_fb_info {
	uint32_t total_fb_size;	 /**< Total GPU fb size in MB */
	uint32_t pf_fb_reserved; /**< Total fb consumed by PF */
	uint32_t pf_fb_offset;	 /**< PF FB offset */
	uint32_t fb_alignment;	 /**< FB alignment */
	uint32_t max_vf_fb_usable;	/**< Maximum usable fb size in MB */
	uint32_t min_vf_fb_usable;	/**< Minimum usable fb size in MB */
	uint64_t reserved[5];
};

struct smi_asic_info {
	char market_name[SMI_MAX_STRING_LENGTH];
	char vendor_name[SMI_MAX_STRING_LENGTH];
	char asic_serial[SMI_MAX_STRING_LENGTH];
	uint64_t reserved[64];
	uint32_t vendor_id;
	uint32_t subvendor_id;	//!< The subsystem vendor id
	uint64_t device_id;	//!< The unique id of a GPU
	uint32_t rev_id;
	uint32_t oam_id;
	uint32_t num_of_compute_units;   //< 0xFFFFFFFF if not supported
	uint64_t target_graphics_version;  //< 0xFFFFFFFFFFFFFFFF if not supported
	uint32_t subsystem_id;	//!< The subsystem device id
	uint64_t reserved_2[10];
};

struct smi_board_info {
	char  model_number[SMI_MAX_STRING_LENGTH];
	char  product_serial[SMI_MAX_STRING_LENGTH];
	char  fru_id[SMI_MAX_STRING_LENGTH];
	char  product_name[SMI_MAX_STRING_LENGTH];
	char  manufacturer_name[SMI_MAX_STRING_LENGTH];
	uint64_t reserved[64];
};


struct smi_gpu_cache_info {
	uint32_t num_cache_types;
	struct cache__ {
		uint32_t cache_properties;
		uint32_t cache_size; /* In KB */
		uint32_t cache_level;
		uint32_t max_num_cu_shared; /* Indicates how many Compute Units share this cache instance */
		uint32_t num_cache_instance; /* total number of instance of this cache type */
		uint32_t reserved[3];
	} cache[SMI_MAX_CACHE_TYPES];
	uint32_t reserved[15];
};

struct smi_fw_info {
	uint8_t num_fw_info;
	struct fw_info_list__ {
		enum smi_fw_block fw_id;
		uint64_t fw_version;
		uint64_t reserved[2];
	} fw_info_list[SMI_FW_ID__MAX];
	uint32_t reserved[7];
};

union smi_bdf {
	struct bdf__ {
		uint64_t function_number : 3;
		uint64_t device_number : 5;
		uint64_t bus_number : 8;
		uint64_t domain_number : 48;
	} bdf;
	uint64_t as_uint;
};

struct smi_vf_handle {
	uint64_t handle;
};

struct smi_vf_fb_info {
	uint32_t fb_offset; /**< Offset in MB from start of the framebuffer */
	uint32_t fb_size;   /**< Size in MB Must be divisible by 16 and not less than 256 */
	uint64_t reserved[3];
};

struct smi_partition_info {
	struct smi_vf_handle id;
	struct smi_vf_fb_info fb;
	uint64_t reserved[3];
};

struct smi_vf_info {
	struct smi_vf_fb_info fb;
	uint32_t gfx_timeslice; /**< Graphics timeslice in us, maximum value is 1000 ms*/
	uint64_t reserved[27];
};

struct smi_sched_info {
	uint64_t		flr_count;
	uint64_t		boot_up_time; //!< in microseconds
	uint64_t		shutdown_time;
	uint64_t		reset_time;
	enum smi_vf_sched_state state;
	char			last_boot_start[SMI_MAX_DATE_LENGTH];
	char			last_boot_end[SMI_MAX_DATE_LENGTH];
	char			last_shutdown_start[SMI_MAX_DATE_LENGTH];
	char			last_shutdown_end[SMI_MAX_DATE_LENGTH];
	char			last_reset_start[SMI_MAX_DATE_LENGTH];
	char			last_reset_end[SMI_MAX_DATE_LENGTH];
	char			current_active_time[SMI_MAX_DATE_LENGTH]; //!< Current session VF time, reset after guest reload
	char			current_running_time[SMI_MAX_DATE_LENGTH];
	char			total_active_time[SMI_MAX_DATE_LENGTH]; /**< Cumulate across entire server lifespan, reset after host reload */
	char			total_running_time[SMI_MAX_DATE_LENGTH]; /**< Not implemented */
	uint64_t reserved[11];
};

struct smi_guard_info {
	uint8_t enabled;
	struct guard__ {
		enum smi_guard_state state;
		/* amount of monitor event after enabled */
		uint32_t amount;
		/* threshold of events in the interval(seconds) */
		uint64_t interval;
		uint32_t threshold;
		/* current number of events in the interval*/
		uint32_t active;
		uint32_t reserved[4];
	} guard[SMI_GUARD_EVENT__MAX];
	uint32_t reserved[6];
};

struct smi_vf_data {
	struct smi_sched_info sched;
	struct smi_guard_info guard;
	uint64_t reserved[8];
};

struct smi_engine_usage {
	uint32_t gfx_activity;
	uint32_t umc_activity;
	uint32_t mm_activity;
	uint64_t reserved[6];
};

struct smi_power_info {
	uint64_t socket_power;
	uint64_t gfx_voltage; //!< GFX voltage measurement in mV
	uint64_t soc_voltage; //!< SOC voltage measurement in mV
	uint64_t mem_voltage; //!< MEM voltage measurement in mV
	uint64_t reserved[4];
};

struct smi_error_count {
	uint64_t correctable_count;	//!< Accumulated correctable errors
	uint64_t uncorrectable_count;	//!< Accumulated uncorrectable errors
	uint64_t deferred_count;	//!< Accumulated deferred errors
	uint64_t reserved[5];
};

struct smi_guest_data {
	char driver_version[SMI_MAX_DRIVER_INFO_RSVD];
	uint32_t fb_usage; /**<  guest framebuffer usage in MB */
	uint64_t reserved[23];
};

struct smi_link_metrics {
	uint32_t num_links; //!< number of links
	struct links__ {
		union smi_bdf bdf;
		uint32_t bit_rate; //!< current link speed in Gb/s
		uint32_t max_bandwidth; //!< max bandwidth of the link
		enum smi_link_type link_type; //!< type of the link
		uint64_t read; //!< total data received for each link in KB
		uint64_t write; //!< total data transfered for each link in KB
		uint64_t reserved[2];
	} links[SMI_MAX_NUM_XGMI_PHYSICAL_LINK];
	uint64_t reserved[7];
};

struct smi_link_topology {
	uint64_t weight; //!< link weight
	enum smi_link_status link_status; //!< HW status of the link
	enum smi_link_type link_type; //!< type of the link
	uint8_t  num_hops; //!< number of hops
	uint8_t  fb_sharing; //!< framebuffer sharing flag
	uint32_t reserved[10];
};

union smi_xgmi_fb_sharing_caps {
	struct cap__ {
		uint32_t mode_custom_cap :1;
		uint32_t mode_1_cap      :1;
		uint32_t mode_2_cap      :1;
		uint32_t mode_4_cap      :1;
		uint32_t mode_8_cap      :1;
		uint32_t reserved        :27;
	} cap;
	uint32_t xgmi_fb_sharing_cap_mask;
};

struct smi_event_entry {
	struct smi_vf_handle	fcn_id;
	uint64_t		dev_id;
	uint64_t		timestamp; //!< UTC microseconds
	uint64_t		data;
	uint32_t		category;
	uint32_t		subcode;
	uint32_t		level;
	char			date[SMI_MAX_DATE_LENGTH]; //!< UTC date and time
	char			message[SMI_EVENT_MSG_SIZE];
	uint64_t		reserved[6];
};

struct smi_ras_feature {
	uint32_t ras_eeprom_version;
	uint32_t supported_ecc_correction_schema; //!< ecc_correction_schema mask used with enum smi_ecc_correction_schema_support flags
	uint64_t reserved[3];
};

struct smi_eeprom_table_record {
	uint64_t  retired_page; //!< Bad page frame address
	uint64_t ts;
	unsigned char err_type;
	union {
		unsigned char bank;
		unsigned char cu;
	};
	unsigned char mem_channel;
	unsigned char mcumc_id;
	uint32_t reserved[3];
};

struct smi_dfc_fw_header {
	uint32_t dfc_fw_version;
	uint32_t dfc_fw_total_entries;
	uint32_t dfc_gart_wr_guest_min;
	uint32_t dfc_gart_wr_guest_max;
	uint32_t reserved[12];
};

struct smi_dfc_fw_white_list {
	uint32_t oldest;
	uint32_t latest;
};

struct smi_dfc_fw_ta_uuid {
	uint8_t ta_uuid[SMI_MAX_UUID_ELEMENTS];
};

struct smi_dfc_fw_data {
	uint32_t dfc_fw_type;
	uint32_t verification_enabled;
	uint32_t customer_ordinal;
	uint32_t reserved[13];
	union {
		struct smi_dfc_fw_white_list white_list[SMI_MAX_WHITE_LIST_ELEMENTS];
		struct smi_dfc_fw_ta_uuid ta_white_list[SMI_MAX_TA_WHITE_LIST_ELEMENTS];
	};
	uint32_t black_list[SMI_MAX_BLACK_LIST_ELEMENTS];
};

struct smi_dfc_fw {
	struct smi_dfc_fw_header header;
	struct smi_dfc_fw_data data[SMI_DFC_FW_NUMBER_OF_ENTRIES];
};

struct smi_fw_load_error_record {
	uint64_t timestamp;     //!< UTC microseconds
	uint32_t vf_idx;
	uint32_t fw_id;
	uint16_t status;        //!< amdsmi_guest_fw_load_status_t
	uint32_t reserved[3];
};

struct smi_fw_error_record {
	uint8_t num_err_records;
	struct smi_fw_load_error_record err_records[SMI_MAX_ERR_RECORDS];
	uint64_t reserved[7];
};

struct smi_profile_caps_info {
	uint64_t total;
	uint64_t available;
	uint64_t optimal;
	uint64_t min_value;
	uint64_t max_value;
	uint64_t reserved[2];
};

struct smi_profile_info {
	uint8_t profile_count;
	uint8_t current_profile_index;
	struct profiles__ {
		uint32_t vf_count;
		struct smi_profile_caps_info profile_caps[SMI_PROFILE_CAPABILITY__MAX];
	} profiles[SMI_MAX_PROFILE_COUNT];
	uint32_t reserved[6];
};

union smi_nps_caps {
	struct nps_flags__ {
		uint32_t nps1_cap :1; // bool 1 = true; 0 = false;
		uint32_t nps2_cap :1; // bool 1 = true; 0 = false;
		uint32_t nps4_cap :1; // bool 1 = true; 0 = false;
		uint32_t nps8_cap :1; // bool 1 = true; 0 = false;
		uint32_t reserved :28; // bool 1 = true; 0 = false;
	} nps_flags;
	uint32_t nps_cap_mask;
};

struct smi_memory_partition_config {
	union smi_nps_caps partition_caps;
	enum smi_memory_partition_type mp_mode;
	uint32_t num_numa_ranges;
	struct numa_range__ {
		enum smi_vram_type memory_type;
		uint64_t start;
		uint64_t end;
	} numa_range[SMI_MAX_NUM_NUMA_NODES];
	uint64_t reserved[11];
};


struct smi_accelerator_partition_profile {
	enum smi_accelerator_partition_type  profile_type;	//!< SPX, DPX, QPX, CPX and so on
	uint32_t num_partitions;				//!< On MI300X, SPX: 1, DPX: 2, QPX: 4, CPX: 8
	union smi_nps_caps memory_caps;				//!< Memory capabilities of the profile
	uint32_t profile_index;					//!< The index in the profiles array in struct smi_accelerator_partition_profile
	uint32_t num_resources;					//!< length of array resources
	uint32_t resources[SMI_MAX_ACCELERATOR_PARTITIONS][SMI_MAX_CP_PROFILE_RESOURCES];
	uint64_t reserved[13];
};

struct smi_accelerator_partition_resource_profile {
	uint32_t profile_index;
	enum smi_accelerator_partition_resource_type resource_type;
	uint32_t partition_resource;					//!< The resources a partition can be used, which may be shared
	uint32_t num_partitions_share_resource;				//!< If it is greater than 1, then resource is shared.
	uint64_t reserved[6];
};

struct smi_accelerator_partition_profile_config {
	uint32_t num_profiles;	//!< The length of profiles array
	uint32_t num_resource_profiles;
	struct smi_accelerator_partition_resource_profile resource_profiles[SMI_MAX_CP_PROFILE_RESOURCES];
	uint32_t default_profile_index;	//!< The index of the default profile in the profiles array
	struct smi_accelerator_partition_profile profiles[SMI_MAX_ACCELERATOR_PROFILE];
	uint64_t reserved[30];
};

struct smi_dpm_policy_entry {
	uint32_t policy_id;
	char policy_description[SMI_MAX_NAME];
	uint64_t reserved[3];
};

struct smi_dpm_policy {
	/**
	 * The number of supported policies
	 */
	uint32_t num_supported;

	/**
	 * The current policy index
	 */
	uint32_t cur;

	/**
	 * List of policies.
	 * Only the first num_supported policies are valid.
	 */
	struct smi_dpm_policy_entry policies[SMI_MAX_NUM_PM_POLICIES];
	uint64_t reserved[7];
};

// IOCTL input/output structures and unions

struct smi_handshake {
	uint32_t version; //!< AMDSMI API version
};

struct smi_device_info {
	smi_device_handle_t dev_id;
};

struct smi_device_info_ex {
	smi_device_handle_t dev_id;
	uint64_t reserved[3];
};

struct smi_device_pair_info {
	struct smi_device_info src;
	struct smi_device_info dst;
};

struct smi_event_set_s {
	uint32_t		num_handles;
	uint32_t		signaled_device_index;
	smi_event_handle_t	*handles;
	void			*_private;
	smi_device_handle_t	*devices;
	uint64_t reserved[4];
};

struct smi_ras_common_if {
	enum smi_gpu_block block;
	enum smi_ras_err_status type;
	uint32_t sub_block_index;
	/* ras block name */
	char name[SMI_MAX_STRING_LENGTH];
	uint64_t reserved[6];
};

struct smi_ras_query_if {
	smi_device_handle_t dev_id;
	struct smi_ras_common_if head;
	uint64_t reserved[6];
};

struct smi_gpu_driver_model {
	enum smi_driver_model_type model;
	uint64_t reserved[3];
};

struct smi_set_gpu_power_cap {
	smi_device_handle_t dev_id;
	uint64_t power_cap;
	uint32_t reserved[2];
};

struct smi_vf_partition_info {
	uint32_t num_vf_enabled; // number of vf configured
	uint32_t num_vf_supported; // number of vf supported
	struct smi_partition_info partition[SMI_MAX_VF_COUNT];
};

struct smi_vf_partition_config {
	smi_device_handle_t dev_id;
	uint32_t num_vf_enable;
};

struct smi_vf_static_info {
	union smi_bdf bdf;
	struct smi_vf_info config;
};

struct smi_ecc_info {
	struct smi_error_count err_count;
	uint64_t reserved[11];
};

/**
 *  \brief  For a given PF or VF return parent function.
 *
 *  \param [in] handle - PF or VF handle
 *
 *  \return PF parent handle
 */
static inline uint64_t make_parent_handle(uint64_t handle)
{
	uint64_t parent_id = handle >> 32;

	/* PF handle has format ( parent = pf_id | self = pf_id ) */
	return parent_id << 32 | parent_id;
}

struct smi_temp_measure {
	uint32_t temp[SMI_TEMPERATURE_TYPE__MAX+1];
	uint64_t reserved[4];
};

struct smi_temp_limit {
	uint32_t temp[SMI_TEMPERATURE_TYPE__MAX+1];
	uint64_t reserved[4];
};

struct smi_temp_shutdown  {
	uint32_t temp[SMI_TEMPERATURE_TYPE__MAX+1];
	uint64_t reserved[4];
};

struct smi_clock_measure {
	uint32_t max_clk[SMI_CLK_TYPE__MAX+1];
	uint32_t cur_clk[SMI_CLK_TYPE__MAX+1];
	uint32_t avg_clk[SMI_CLK_TYPE__MAX+1];
	uint32_t min_clk[SMI_CLK_TYPE__MAX+1];
	uint8_t clk_locked[SMI_CLK_TYPE__MAX+1];
	uint8_t clk_deep_sleep[SMI_CLK_TYPE__MAX+1];
	uint64_t reserved[9];
};

struct smi_gpu_caps {
	struct {
		uint32_t gfxip_major;
		uint32_t gfxip_minor;
		uint16_t gfxip_cu_count;
		uint32_t reserved[5];
	} gfx;
	struct {
		uint8_t mm_ip_count;
		uint8_t mm_ip_list[SMI_MAX_MM_IP_COUNT];
		uint32_t reserved[5];
	} mm;
	bool ras_supported;
	uint8_t max_vf_num;
	uint32_t gfx_ip_count;
	uint32_t dma_ip_count;
	uint32_t ras_eeprom_version;
	uint32_t supported_ecc_correction_schema;
	uint32_t reserved[3];
};

struct smi_driver_info {
	uint32_t id;
	uint32_t version_len;
	uint64_t reserved_1[7];
	char version[SMI_MAX_STRING_LENGTH];
	char libgv_build_date[SMI_MAX_DATE_LENGTH];
	char driver_date[SMI_MAX_DATE_LENGTH];
	uint64_t reserved_2[64];
};

struct smi_server_static_info {
	enum smi_debug_level debug_level;
	uint32_t num_devices;
	struct {
		smi_device_handle_t	dev_id;
		union smi_bdf		bdf;
		bool			failed;
		char			padding[3];
		uint32_t		reserved[3];
	} devices[SMI_MAX_DEVICES];
	uint64_t reserved[383]; // expanded to MAX IOCTL copy size
};

struct smi_gpu_performance_info {
	struct smi_engine_usage usage;
	struct smi_power_info power;
	struct smi_clock_measure clock;
	struct smi_temp_measure temp;
	struct smi_temp_limit temp_limit;
	struct smi_temp_shutdown temp_shutdown;
	uint64_t reserved[440]; // expanded to MAX IOCTL copy size
};

struct smi_data_query {
	struct smi_device_info dev;
	enum smi_data_query_type type;
};

union smi_data {
	bool power_management_enabled;
	uint64_t ras_caps;
};

struct smi_event_set_config {
	smi_device_handle_t	dev_id;
	smi_event_handle_t	event_set;
	uint64_t		event_mask;
};

struct smi_get_handle_info {
	union smi_bdf 	bdf;
};

struct smi_get_handle_resp {
	smi_device_handle_t dev_id;
	struct smi_vf_handle vf_id;
};

struct smi_guest_info {
	struct smi_guest_data guest_data;
};

struct smi_xgmi_fb_sharing {
	smi_device_handle_t src_dev;
	smi_device_handle_t dst_dev;
	uint32_t mode;
	uint32_t reserved;
};

struct smi_xgmi_fb_sharing_flag {
	uint8_t fb_sharing;
};

struct smi_set_xgmi_fb_sharing_mode {
	smi_device_handle_t dev_id;
	uint32_t mode;
	uint32_t reserved[3];
};

struct smi_set_xgmi_fb_custom_sharing_mode {
	uint32_t num_processors;
	smi_device_handle_t dev_id_list[SMI_MAX_DEVICES];
};

struct smi_metric {
	union {
		struct {
			enum smi_metric_unit unit : 16;
			enum smi_metric_name name : 16;
			enum smi_metric_category category : 16;
			uint64_t flag_counter : 1;
			uint64_t flag_chiplet_metric : 1;
			uint64_t flag_data_filter_inst : 1;
			uint64_t flag_data_filter_acc : 1;
			uint64_t flag_reserved : 12;
		} metric;
		uint64_t code;
	} metric_union;
	uint32_t vf_mask; //!< Mask of all active VFs + PF that this metric applies to
	uint64_t val;
	uint64_t reserved[5];
};

struct smi_metrics {
	uint32_t num_metric;
	struct smi_metric metric[SMI_MAX_NUM_METRICS];
};

struct smi_metrics_table {
	smi_device_handle_t dev_id;
	uint32_t size;
	struct smi_metrics *metrics;
	uint32_t reserved[9];
};

struct smi_bad_page_record {
	uint32_t num_bad_page;
	struct smi_eeprom_table_record bad_page[SMI_MAX_BAD_PAGE_RECORD];
};

struct smi_bad_page_info {
	smi_device_handle_t dev_id;
	uint32_t size;
	struct smi_bad_page_record *bad_pages;
	uint32_t reserved[9];
};

struct smi_set_gpu_accelerator_partition_setting {
	smi_device_handle_t dev_id;
	uint32_t index;
	uint32_t reserved[3];
};

struct smi_set_gpu_memory_partition_setting {
	smi_device_handle_t dev_id;
	enum smi_memory_partition_type mode;
	uint32_t reserved[3];
};

struct smi_profile_configs {
	smi_device_handle_t dev_id;
	struct smi_accelerator_partition_profile_config *profile_configs;
	uint32_t reserved[10];
};

struct smi_accelerator_partition_profile_cap {
	struct smi_accelerator_partition_profile config;
	uint32_t partition_id[SMI_MAX_ACCELERATOR_PROFILE];
	uint32_t reserved[2];
};

struct smi_set_dpm_policy {
	smi_device_handle_t dev_id;
	uint32_t policy_id;
	uint32_t reserved[3];
};

#pragma pack(push, 1)
struct smi_cper_guid{
    unsigned char b[16];
};

enum smi_cper_error_severity{
    SMI_CPER_SEV_NON_FATAL_UNCORRECTED = 0,
    SMI_CPER_SEV_FATAL                 = 1,
    SMI_CPER_SEV_NON_FATAL_CORRECTED   = 2,
    SMI_CPER_SEV_NUM                   = 3,

    SMI_CPER_SEV_UNUSED = 10,
};

struct smi_cper_timestamp {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t flag;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t century;
};

struct smi_cper_hdr{
    char                     signature[4];  /* "CPER"  */
    uint16_t                 revision;
    uint32_t                 signature_end; /* 0xFFFFFFFF */
    uint16_t                 sec_cnt;
    enum smi_cper_error_severity error_severity;
    union {
        struct {
            uint32_t platform_id  : 1;
            uint32_t timestamp    : 1;
            uint32_t partition_id : 1;
            uint32_t reserved     : 29;
        } valid_bits;
        uint32_t valid_mask;
    };
    uint32_t              record_length;    /* Total size of CPER Entry */
    struct smi_cper_timestamp timestamp;
    char                  platform_id[16];
    struct smi_cper_guid      partition_id;     /* Reserved */
    char                  creator_id[16];
    struct smi_cper_guid      notify_type;      /* CMC, MCE */
    char                  record_id[8];     /* Unique CPER Entry ID */
    uint32_t              flags;            /* Reserved */
    uint64_t              persistence_info; /* Reserved */
    uint8_t               reserved[12];     /* Reserved */
};
#pragma pack(pop)

struct smi_cper {
	char 		cper_data[SMI_MAX_CPER_SIZE];
	uint64_t 	buf_size;
	uint32_t	cper_hdrs[SMI_MAX_CPER_HDRS];
	uint64_t 	entry_count;
	uint64_t	cursor;
};

struct smi_cper_config {
	smi_device_handle_t	dev_id;
	uint32_t 		severity_mask;
	uint64_t		input_cursor;
	struct smi_cper	 	*cper;
	uint32_t		reserved[4];
};

#ifndef __linux__
#pragma pack(pop)
#endif

#endif // __SMI_CMD_H__
