/*
 * Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __SMI_UTILS_H__
#define __SMI_UTILS_H__

#include <stdbool.h>
#include <stdlib.h>

#include "common/smi_cmd.h"
#include "amdsmi.h"
#include "common/smi_handle.h"
#include "common/smi_device_handle.h"

enum smi_metric_type {
	SMI_METRIC_TYPE_COUNTER = 0x30,
	SMI_METRIC_TYPE_CHIPLET,
	SMI_METRIC_TYPE_INST,
	SMI_METRIC_TYPE_ACC
};

/**
 *  \brief  Util function for dispaching IOCTL call to the KMD.
 *
 *  \note   The data for input should be stored in the global handle.input buff,
 *          and the result of the KMD command will be stored in the
 * handle.output buff.
 *
 *  \param [in] cmd_code - Code of the command issued to the KMD.
 *
 *  \param [in] input_size - Size of the input structure expected by the
 * command.
 *
 *  \param [in] output_size - Size of the output structure returned by the
 * command.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_request(smi_req_ctx *smi_req, uint32_t cmd_code, size_t input_size,
		size_t output_size);

/**
 *  \brief  Util function for querying vf partitioning info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] handle - Amdsmi processor handle.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_vf_partitioning_info(smi_req_ctx *smi_req, smi_device_handle_t handle);

/**
 *  \brief  Util function for querying vf static info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] vf_handle - VF handle to query.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_vf_static_info(smi_req_ctx *smi_req, smi_device_handle_t vf_handle);

/**
 *  \brief  Util function for querying gpu performance info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] handle - Amdsmi processor handle.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_gpu_performance_info(smi_req_ctx *smi_req, smi_device_handle_t handle);

/**
 *  \brief  Util function for querying ecc info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] handle - Amdsmi processor handle.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_ecc_error_count(smi_req_ctx *smi_req, smi_device_handle_t handle);

/**
 *  \brief  Util function for querying vf dynamic info.
 *
 *  \note   The result of the KMD command will be stored in the handle.output
 * buff.
 *
 *  \param [in] vf_handle - VF handle to query.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_ioctl_get_vf_dynamic_info(smi_req_ctx *smi_req, smi_device_handle_t vf_handle);

/**
 *  \brief  Util function for converting pcie speed from pcie type.
 *
 *  \param [in] pcie_type - Pcie type.
 *
 *  \param [out] pcie_speed - Pcie speed.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_get_pcie_speed_from_pcie_type(uint32_t pcie_type, uint32_t *pcie_speed, uint64_t dev_id);

/**
 *  \brief  Util function for converting status from code to coresponding string message.
 *
 *  \param [in] status - Status code.
 *
 *  \param [out] out - String message.
 *
 *  \return AMDSMI_RET_CODE indicating result.
 */
amdsmi_status_t amdsmi_get_string_from_status_enum(amdsmi_status_t status, const char **out);

/**
 *  \brief  Maps Physical Function (PF) device ID to corresponding Virtual Function (VF) device ID.
 *
 *  \details This function takes a PF device ID and returns the corresponding VF device ID
 *
 *  \param [in] pf_device_id - The Physical Function device ID to be mapped.
 *
 *  \param [out] vf_device_id - Pointer to store the corresponding Virtual Function device ID.
 *                              Must be allocated by caller.
 *
 *  \return SMI_RET_CODE indicating result.
 */
int smi_get_vf_device_id_from_pf(uint64_t pf_device_id, uint64_t *vf_device_id);

/**
 *  \brief  Generates uuid for device with specified parameters
 *
 *  \param [out] str      String buffer where to output generated uuid
 *
 *  \param [in]  serial   Asic serial
 *
 *  \param [in]  did      Device ID
 *
 *  \param [in]  idx      PF/VF index
 *
 *  \return SMI_RET_CODE indicating result.
 */
int smi_uuid_gen(char *str, uint64_t serial, uint16_t did, uint8_t idx);

/**
 *  \brief  Checks that the passed UUID has a valid format.
 *
 *  \param [in]  uuid The UUID to check for
 *
 *  \return true if uuid format is valid or false if it is not
 */
bool is_uuid_valid(const char *uuid);



#pragma pack(push, 1)

#define CPER_HDR_REV_1          (0x100)
#define CPER_SEC_MINOR_REV_1    (0x01)
#define CPER_SEC_MAJOR_REV_22   (0x22)
#define CPER_MAX_OAM_COUNT      (8)

#define CPER_CTX_TYPE_CRASH     (1)
#define CPER_CTX_TYPE_BOOT      (9)

#define CPER_CREATOR_ID_AMDGV	"amdgv"


typedef struct {
	unsigned char b[16];
} guid_t;


#define GUID_INIT(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)                 \
{ { (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff, \
   (b) & 0xff, ((b) >> 8) & 0xff,                                          \
   (c) & 0xff, ((c) >> 8) & 0xff,                                          \
   (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) } };


#define CPER_NOTIFY_MCE                                               \
	GUID_INIT(0xE8F56FFE, 0x919C, 0x4cc5, 0xBA, 0x88, 0x65, 0xAB, \
		  0xE1, 0x49, 0x13, 0xBB)
#define CPER_NOTIFY_CMC                                               \
	GUID_INIT(0x2DCE8BB1, 0xBDD7, 0x450e, 0xB9, 0xAD, 0x9C, 0xF4, \
		  0xEB, 0xD4, 0xF8, 0x90)
#define BOOT_TYPE                                                     \
	GUID_INIT(0x3D61A466, 0xAB40, 0x409a, 0xA6, 0x98, 0xF3, 0x62, \
		  0xD4, 0x64, 0xB3, 0x8F)

#define AMD_CRASHDUMP                                                 \
	GUID_INIT(0x32AC0C78, 0x2623, 0x48F6, 0xB0, 0xD0, 0x73, 0x65, \
		  0x72, 0x5F, 0xD6, 0xAE)
#define AMD_GPU_NONSTANDARD_ERROR                                     \
	GUID_INIT(0x32AC0C78, 0x2623, 0x48F6, 0x81, 0xA2, 0xAC, 0x69, \
		  0x17, 0x80, 0x55, 0x1D)
#define PROC_ERR_SECTION_TYPE                                         \
	GUID_INIT(0xDC3EA0B0, 0xA144, 0x4797, 0xB9, 0x5B, 0x53, 0xFA, \
		  0x24, 0x2B, 0x6E, 0x1D)

enum cper_error_severity {
	CPER_SEV_NON_FATAL_UNCORRECTED = 0,
	CPER_SEV_FATAL                 = 1,
	CPER_SEV_NON_FATAL_CORRECTED   = 2,
	CPER_SEV_NUM                   = 3,

	CPER_SEV_UNUSED = 10,
};

enum cper_aca_reg {
	CPER_ACA_REG_CTL_LO    = 0,
	CPER_ACA_REG_CTL_HI    = 1,
	CPER_ACA_REG_STATUS_LO = 2,
	CPER_ACA_REG_STATUS_HI = 3,
	CPER_ACA_REG_ADDR_LO   = 4,
	CPER_ACA_REG_ADDR_HI   = 5,
	CPER_ACA_REG_MISC0_LO  = 6,
	CPER_ACA_REG_MISC0_HI  = 7,
	CPER_ACA_REG_CONFIG_LO = 8,
	CPER_ACA_REG_CONFIG_HI = 9,
	CPER_ACA_REG_IPID_LO   = 10,
	CPER_ACA_REG_IPID_HI   = 11,
	CPER_ACA_REG_SYND_LO   = 12,
	CPER_ACA_REG_SYND_HI   = 13,

	CPER_ACA_REG_COUNT     = 32,
};

struct cper_sec_desc {
	uint32_t sec_offset;     /* Offset from the start of CPER entry */
	uint32_t sec_length;
	uint8_t  revision_minor; /* CPER_SEC_MINOR_REV_1 */
	uint8_t  revision_major; /* CPER_SEC_MAJOR_REV_22 */
	union {
		struct {
			uint8_t fru_id		: 1;
			uint8_t fru_text	: 1;
			uint8_t reserved	: 6;
		} valid_bits;
		uint8_t valid_mask;
	};
	uint8_t reserved;
	union {
		struct {
			uint32_t primary		: 1;
			uint32_t reserved1		: 2;
			uint32_t exceed_err_threshold	: 1;
			uint32_t latent_err		: 1;
			uint32_t reserved2		: 27;
		} flag_bits;
		uint32_t flag_mask;
	};
	guid_t				sec_type;
	char				fru_id[16];
	enum cper_error_severity	severity;
	char				fru_text[20];
};

struct cper_sec_nonstd_err_hdr {
	union {
		struct {
			uint64_t apic_id		: 1;
			uint64_t fw_id			: 1;
			uint64_t err_info_cnt		: 6;
			uint64_t err_context_cnt	: 6;
		} valid_bits;
		uint64_t valid_mask;
	};
	uint64_t apic_id;
	char     fw_id[48];
};

struct cper_sec_nonstd_err_info {
	guid_t error_type;
	union {
		struct {
			uint64_t ms_chk			: 1;
			uint64_t target_addr_id		: 1;
			uint64_t req_id			: 1;
			uint64_t resp_id		: 1;
			uint64_t instr_ptr		: 1;
			uint64_t reserved		: 59;
		} valid_bits;
		uint64_t        valid_mask;
	};
	union {
		struct {
			uint64_t err_type_valid		: 1;
			uint64_t pcc_valid		: 1;
			uint64_t uncorr_valid		: 1;
			uint64_t precise_ip_valid	: 1;
			uint64_t restartable_ip_valid	: 1;
			uint64_t overflow_valid		: 1;
			uint64_t reserved1		: 10;
			uint64_t err_type		: 2;
			uint64_t pcc			: 1;
			uint64_t uncorr			: 1;
			uint64_t precised_ip		: 1;
			uint64_t restartable_ip		: 1;
			uint64_t overflow		: 1;
			uint64_t reserved2		: 41;
		} ms_chk_bits;
		uint64_t ms_chk_mask;
	};
	uint64_t target_addr_id;
	uint64_t req_id;
	uint64_t resp_id;
	uint64_t instr_ptr;
};

struct cper_sec_nonstd_err_ctx {
	uint16_t reg_ctx_type;
	uint16_t reg_arr_size;
	uint32_t msr_addr;
	uint64_t mm_reg_addr;
	uint32_t reg_dump[CPER_ACA_REG_COUNT];
};

struct cper_sec_nonstd_err {
	struct cper_sec_nonstd_err_hdr  hdr;
	struct cper_sec_nonstd_err_info info;
	struct cper_sec_nonstd_err_ctx  ctx;
};

struct cper_sec_crashdump_hdr {
	uint64_t reserved1;
	uint64_t reserved2;
	char     fw_id[48];
	uint64_t reserved3[8];
};

struct cper_sec_crashdump_reg_data {
	uint32_t status_lo;
	uint32_t status_hi;
	uint32_t addr_lo;
	uint32_t addr_hi;
	uint32_t ipid_lo;
	uint32_t ipid_hi;
	uint32_t synd_lo;
	uint32_t synd_hi;
};

struct cper_sec_crashdump_body_fatal {
	uint16_t                           reg_ctx_type;
	uint16_t                           reg_arr_size;
	uint32_t                           reserved1;
	uint64_t                           reserved2;
	struct cper_sec_crashdump_reg_data data;
};

struct cper_sec_crashdump_body_boot {
	uint16_t reg_ctx_type;
	uint16_t reg_arr_size;
	uint32_t reserved1;
	uint64_t reserved2;
	uint64_t msg[CPER_MAX_OAM_COUNT];
};

struct cper_sec_crashdump_fatal {
	struct cper_sec_crashdump_hdr        hdr;
	struct cper_sec_crashdump_body_fatal body;
};

struct cper_sec_crashdump_boot {
	struct cper_sec_crashdump_hdr       hdr;
	struct cper_sec_crashdump_body_boot body;
};

#pragma pack(pop)

/**
 *  \brief  Compares two GUIDs for equality.
 *
 *  \param [in] guid1 - Pointer to the first GUID.
 *  \param [in] guid2 - Pointer to the second GUID.
 *
 *  \return true if the GUIDs are equal, false otherwise.
 */
bool guid_equals(const guid_t* guid1,const guid_t* guid2);

/**
 *  \brief  Prints a hex dump of the given memory region.
 *
 *  \param [in] data - Pointer to the memory region to dump.
 *  \param [in] size - Size of the memory region in bytes.
 *  \param [in] register_array - Pointer to the register array for additional context.
 */
void amdsmi_get_register_array(const uint8_t* data, size_t size, uint64_t *register_array);

/**
 *  \brief  Creates a sysfs PCI device path prefix for a given processor handle.
 *
 *  \param [in] processor_handle - Handle to the processor for which to generate the path.
 *  \param [out] out_path - Buffer to store the generated path.
 *  \param [in] out_path_size - Size of the output buffer in bytes.
 *
 *  \return 0 on success, error code otherwise.
 */
int make_sysfs_pci_device_prefix(amdsmi_processor_handle processor_handle, char *out_path, size_t out_path_size);

/**
 *  \brief  Parses a CPU list string and sets corresponding bits in the CPU set.
 *
 *  \param [in] cpu_list - String containing CPU list (e.g., "0,2-4,7").
 *  \param [out] cpu_set - Array of 64-bit integers representing the CPU bit mask.
 *  \param [in] cpu_set_size - Size of the cpu_set array in elements.
 *
 *  \return 0 on success, error code otherwise.
 */
int parse_cpu_list(const char *cpu_list, uint64_t *cpu_set, uint32_t cpu_set_size);

/**
 *  \brief  Checks if a command is supported for a given device ID.
 *
 *  \param [in] device_id - The device ID to check.
 *
 *  \return AMDSMI_STATUS_SUCCESS if the command is supported, AMDSMI_STATUS_NOT_SUPPORTED otherwise.
 */
amdsmi_status_t is_cmd_supported(uint64_t device_id);

#endif // __SMI_UTILS_H__
