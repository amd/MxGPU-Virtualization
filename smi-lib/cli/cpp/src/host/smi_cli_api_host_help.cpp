/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "amdsmi.h"
#include "smi_cli_api_host.h"
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_exception.h"

#include <sstream>
#include <cstring>
#include <algorithm>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

template<typename T>
struct EnumToString {
	std::unordered_map<T, std::string> data;
	std::string operator()(T value) const
	{
		auto it = data.find(value);
		return it != data.end() ? it->second : "Unknown";
	}
};

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_UUID)(amdsmi_processor_handle, unsigned int *,
		char *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_BDF)(amdsmi_vf_handle_t, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_UUID)(amdsmi_vf_handle_t, unsigned int *, char *);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint32_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_PARTITION_INFO)(amdsmi_processor_handle, unsigned int,
		amdsmi_partition_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_GPU_DEVICE_UUID host_amdsmi_get_gpu_device_uuid;
extern AMDSMI_GET_VF_BDF host_amdsmi_get_vf_bdf;
extern AMDSMI_GET_VF_UUID host_amdsmi_get_vf_uuid;
extern AMDSMI_GET_NUM_VF host_amdsmi_get_num_vf;
extern AMDSMI_GET_VF_PARTITION_INFO host_amdsmi_get_vf_partition_info;


int AmdSmiApiHost::amdsmi_get_bdf_from_gpu_index(uint64_t &processor_bdf, int index)
{
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;
	amdsmi_processor_handle *processors;
	amdsmi_bdf_t tmp_bdf;
	int ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		exit(1);
	}
	if (index >= gpu_count) {
		exit(1);
	}
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	amdsmi_processor_handle gpu_handle = processors[index];
	ret = host_amdsmi_get_gpu_device_bdf(gpu_handle, &tmp_bdf);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(processors);
		exit(1);
	}
	processor_bdf = tmp_bdf.as_uint;
	free(processors);
	return 0;
}

int AmdSmiApiHost::amdsmi_get_bdf_from_uuid_or_bdf(uint64_t &processor_bdf, int &gpu_index,
		std::string device, int type)
{
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;
	amdsmi_processor_handle *processors;
	amdsmi_bdf_t tmp_bdf;
	int ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		exit(1);
	}
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(processors);
		exit(1);
	}
	if (type == BDF) {
		for (int i = 0; i < gpu_count; i++) {
			amdsmi_bdf_t bdf;
			ret = host_amdsmi_get_gpu_device_bdf(processors[i], &bdf);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				free(processors);
				exit(1);
			}

			std::string bdf_string = string_format(
										 "%04x%02x%02x%01x", bdf.bdf.domain_number, bdf.bdf.bus_number, bdf.bdf.device_number,
										 bdf.bdf.function_number);
			std::string device_id_parsed = device;
			device_id_parsed.erase(std::remove_if(device_id_parsed.begin(), device_id_parsed.end(),
			[](char c) {
				return c == ':' || c == '.';
			}), device_id_parsed.end());

			if (bdf_string == " ") {
				free(processors);
				exit(1);
			}
			if (strcmp(bdf_string.c_str(), device_id_parsed.c_str()) == 0) {
				gpu_index = i;
				ret = host_amdsmi_get_gpu_device_bdf(processors[i], &tmp_bdf);
				if (ret != AMDSMI_STATUS_SUCCESS) {
					free(processors);
					exit(1);
				}
				processor_bdf = tmp_bdf.as_uint;
				return AMDSMI_STATUS_SUCCESS;
			}
		}
		free(processors);
		throw SmiToolDeviceNotFoundException(device);
	} else {
		unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;
		char uuid[AMDSMI_GPU_UUID_SIZE] = { 0 };
		amdsmi_processor_handle gpu_handle = nullptr;
		for (int i = 0; i < gpu_count; i++) {
			ret = host_amdsmi_get_gpu_device_uuid(processors[i], &uuid_length, uuid);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				free(processors);
				exit(1);
			}
			if (strcmp(uuid, device.c_str()) == 0) {
				gpu_index = i;
				gpu_handle = processors[i];
				ret = host_amdsmi_get_gpu_device_bdf(processors[i], &tmp_bdf);
				if (ret != AMDSMI_STATUS_SUCCESS) {
					free(processors);
					exit(1);
				}
				processor_bdf = tmp_bdf.as_uint;
				return AMDSMI_STATUS_SUCCESS;
			}
		}
		if (gpu_handle == nullptr) {
			free(processors);
			throw SmiToolDeviceNotFoundException(device);
		}
	}

	free(processors);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_gpu_count(unsigned int &gpu_count)
{
	amdsmi_socket_handle socket = NULL;
	int ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		exit(1);
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_vf_tree(std::vector<std::map<std::string, std::string>> &out)
{
	amdsmi_processor_handle *processors;
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;
	amdsmi_status_t ret;
	std::map<std::string, std::string> vf_map;
	ret = host_amdsmi_get_processor_handles(
			  socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		exit(1);
	}
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_processor_handles(
			  socket, &gpu_count, &processors[0]);
	for (int i = 0; i < gpu_count; i++) {
		uint32_t num_vf_enabled;
		amdsmi_bdf_t vf_bdf;
		char vf_uuid[AMDSMI_GPU_UUID_SIZE];
		uint32_t num_vf_supported;
		amdsmi_partition_info_t partitions[AMDSMI_MAX_VF_COUNT];
		host_amdsmi_get_num_vf(processors[i], &num_vf_enabled,
							   &num_vf_supported);
		host_amdsmi_get_vf_partition_info(
			processors[i], num_vf_enabled, partitions);

		for(uint8_t j = 0; j < num_vf_enabled; j++) {
			unsigned int vf_length = AMDSMI_GPU_UUID_SIZE;
			host_amdsmi_get_vf_bdf(partitions[j].id,
								   &vf_bdf);
			std::string vfbdf{ convert_bdf_to_string(
								   vf_bdf.bdf.function_number, vf_bdf.bdf.device_number, vf_bdf.bdf.bus_number, vf_bdf.bdf.domain_number) };
			host_amdsmi_get_vf_uuid(partitions[j].id,
									&vf_length, vf_uuid);
			vf_map["gpu"] = std::to_string(i);
			vf_map["vf"] = std::to_string(j);
			vf_map["vf_id"] = std::to_string(i) + ":" + std::to_string(j);
			vf_map["vf_bdf"] = vfbdf;
			vf_map["vf_uuid"] = vf_uuid;
			out.push_back(vf_map);
		}
	}

	free(processors);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_error_message(int error_code, std::string& out)
{
	switch (error_code) {
	case AMDSMI_STATUS_INVAL:
		out = "Invalid parameters";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_NOT_SUPPORTED:
		out = "Command not supported";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_NOT_YET_IMPLEMENTED:
		out = "Not implemented yet";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_FAIL_LOAD_MODULE:
		out = "Fail to load lib";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_FAIL_LOAD_SYMBOL:
		out = "Fail to load symbol";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_DRM_ERROR:
		out = "Error when call libdrm";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_API_FAILED:
		out = "API call failed";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_TIMEOUT:
		out = "Timeout in API call";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_RETRY:
		out = "Retry operation";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_NO_PERM:
		out = "Permission Denied";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_INTERRUPT:
		out = "An interrupt occurred during execution of function";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_IO:
		out = "I/O Error";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_ADDRESS_FAULT:
		out = "Bad address";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_FILE_ERROR:
		out = "Problem accessing a file";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_OUT_OF_RESOURCES:
		out = "Not enough memory";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_INTERNAL_EXCEPTION:
		out = "An internal exception was caught";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS:
		out = "The provided input is out of allowable or safe range";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_INIT_ERROR:
		out = "An error occurred when initializing internal data structures";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_REFCOUNT_OVERFLOW:
		out = "An internal reference counter exceeded INT32_MAX";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_BUSY:
		out = "Processor busy";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_NOT_FOUND:
		out = "Processor Not found";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_NOT_INIT:
		out = "Processor not initialized";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_NO_SLOT:
		out = "No more free slot";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_NO_DATA:
		out = "No data was found for a given input";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_INSUFFICIENT_SIZE:
		out = "Not enough resources were available for the operation";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_UNEXPECTED_SIZE:
		out = "An unexpected amount of data was read";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_STATUS_UNEXPECTED_DATA:
		out = "The data read or provided to function is not what was expected";
		return AMDSMI_STATUS_SUCCESS;
	case static_cast<int>(AMDSMI_STATUS_MAP_ERROR):
		out = "The internal library error did not map to a status code";
		return AMDSMI_STATUS_SUCCESS;
	case static_cast<int>(AMDSMI_STATUS_UNKNOWN_ERROR):
		out = "An unknown error occurred";
		return AMDSMI_STATUS_SUCCESS;
	default:
		out = "Unknown error";
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::format_link_type(const int& link_type, std::string& out)
{
	switch(link_type) {
	case AMDSMI_LINK_TYPE_PCIE:
		out = "PCIE";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_LINK_TYPE_XGMI:
		out = "XGMI";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_LINK_TYPE_NOT_APPLICABLE:
		out = "N/A";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_LINK_TYPE_UNKNOWN:
		out = "UNKNOWN";
		return AMDSMI_STATUS_SUCCESS;
	default:
		out = "N/A";
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::format_link_status(const int& link_status, std::string& out)
{
	switch(link_status) {
	case AMDSMI_LINK_STATUS_ENABLED:
		out = "ENABLED";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_LINK_STATUS_DISABLED:
		out = "DISABLED";
		return AMDSMI_STATUS_SUCCESS;
	case AMDSMI_LINK_STATUS_ERROR:
		out = "N/A";
		return AMDSMI_STATUS_SUCCESS;
	default:
		out = "N/A";
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::transform_ecc_correction_schema(uint32_t flag, std::vector<std::string>& out)
{

	uint32_t flags_parity = AMDSMI_RAS_ECC_SUPPORT_PARITY & flag;
	uint32_t flags_correctable = AMDSMI_RAS_ECC_SUPPORT_CORRECTABLE & flag;
	uint32_t flags_uncorrectable = AMDSMI_RAS_ECC_SUPPORT_UNCORRECTABLE & flag;
	uint32_t flags_poison = AMDSMI_RAS_ECC_SUPPORT_POISON & flag;

	if(AMDSMI_RAS_ECC_SUPPORT_PARITY == flags_parity) {
		out.push_back("ENABLED");
	} else {
		out.push_back("DISABLED");
	}
	if(AMDSMI_RAS_ECC_SUPPORT_CORRECTABLE == flags_correctable) {
		out.push_back("ENABLED");
	} else {
		out.push_back("DISABLED");
	}
	if(AMDSMI_RAS_ECC_SUPPORT_UNCORRECTABLE == flags_uncorrectable) {
		out.push_back("ENABLED");
	} else {
		out.push_back("DISABLED");
	}
	if(AMDSMI_RAS_ECC_SUPPORT_POISON == flags_poison) {
		out.push_back("ENABLED");
	} else {
		out.push_back("DISABLED");
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_fw_block(int fw_block, std::string& out)
{
	EnumToString<amdsmi_fw_block_t> fw_blocks;
	fw_blocks.data = {
		{AMDSMI_FW_ID_SMU, "SMU"},
		{AMDSMI_FW_ID_CP_CE, "CP_CE"},
		{AMDSMI_FW_ID_CP_PFP, "CP_PFP"},
		{AMDSMI_FW_ID_CP_ME, "CP_ME"},
		{AMDSMI_FW_ID_CP_MEC_JT1, "CP_MEC_JT1"},
		{AMDSMI_FW_ID_CP_MEC_JT2, "CP_MEC_JT2"},
		{AMDSMI_FW_ID_CP_MEC1, "CP_MEC1"},
		{AMDSMI_FW_ID_CP_MEC2, "CP_MEC2"},
		{AMDSMI_FW_ID_RLC, "RLC"},
		{AMDSMI_FW_ID_SDMA0, "SDMA0"},
		{AMDSMI_FW_ID_SDMA1, "SDMA1"},
		{AMDSMI_FW_ID_SDMA2, "SDMA2"},
		{AMDSMI_FW_ID_SDMA3, "SDMA3"},
		{AMDSMI_FW_ID_SDMA4, "SDMA4"},
		{AMDSMI_FW_ID_SDMA5, "SDMA5"},
		{AMDSMI_FW_ID_SDMA6, "SDMA6"},
		{AMDSMI_FW_ID_SDMA7, "SDMA7"},
		{AMDSMI_FW_ID_VCN, "VCN"},
		{AMDSMI_FW_ID_UVD, "UVD"},
		{AMDSMI_FW_ID_VCE, "VCE"},
		{AMDSMI_FW_ID_ISP, "ISP"},
		{AMDSMI_FW_ID_DMCU_ERAM, "DMCU_ERAM"}, //!< eRAM
		{AMDSMI_FW_ID_DMCU_ISR, "DMCU_ISR"},  //!< ISR
		{AMDSMI_FW_ID_RLC_RESTORE_LIST_GPM_MEM, "RLC_RESTORE_LIST_GPM_MEM"},
		{AMDSMI_FW_ID_RLC_RESTORE_LIST_SRM_MEM, "RLC_RESTORE_LIST_SRM_MEM"},
		{AMDSMI_FW_ID_RLC_RESTORE_LIST_CNTL, "RLC_RESTORE_LIST_CNTL"},
		{AMDSMI_FW_ID_RLC_V, "RLC_V"},
		{AMDSMI_FW_ID_MMSCH, "MMSCH"},
		{AMDSMI_FW_ID_PSP_SYSDRV, "PSP_SYSDRV"},
		{AMDSMI_FW_ID_PSP_SOSDRV, "PSP_SOSDRV"},
		{AMDSMI_FW_ID_PSP_TOC, "PSP_TOC"},
		{AMDSMI_FW_ID_PSP_KEYDB, "PSP_KEYDB"},
		{AMDSMI_FW_ID_DFC, "DFC"},
		{AMDSMI_FW_ID_PSP_SPL, "PSP_SPL"},
		{AMDSMI_FW_ID_DRV_CAP, "DRV_CAP"},
		{AMDSMI_FW_ID_MC, "MC"},
		{AMDSMI_FW_ID_PSP_BL, "PSP_BL"},
		{AMDSMI_FW_ID_CP_PM4, "CP_PM4"},
		{AMDSMI_FW_ID_RLC_P, "RLC_P"},
		{AMDSMI_FW_ID_SEC_POLICY_STAGE2, "SEC_POLICY_STAGE2"},
		{AMDSMI_FW_ID_REG_ACCESS_WHITELIST, "REG_ACCESS_WHITELIST"},
		{AMDSMI_FW_ID_IMU_DRAM, "IMU_DRAM"},
		{AMDSMI_FW_ID_IMU_IRAM, "IMU_IRAM"},
		{AMDSMI_FW_ID_SDMA_TH0, "SDMA_TH0"},
		{AMDSMI_FW_ID_SDMA_TH1, "SDMA_TH1"},
		{AMDSMI_FW_ID_CP_MES, "CP_MES"},
		{AMDSMI_FW_ID_MES_STACK, "MES_STACK"},
		{AMDSMI_FW_ID_MES_THREAD1, "MES_THREAD1"},
		{AMDSMI_FW_ID_MES_THREAD1_STACK, "MES_THREAD1_STACK"},
		{AMDSMI_FW_ID_RLX6, "RLX6"},
		{AMDSMI_FW_ID_RLX6_DRAM_BOOT, "RLX6_DRAM_BOOT"},
		{AMDSMI_FW_ID_RS64_ME, "RS64_ME"},
		{AMDSMI_FW_ID_RS64_ME_P0_DATA, "RS64_ME_P0_DATA"},
		{AMDSMI_FW_ID_RS64_ME_P1_DATA, "RS64_ME_P1_DATA"},
		{AMDSMI_FW_ID_RS64_PFP, "RS64_PFP"},
		{AMDSMI_FW_ID_RS64_PFP_P0_DATA, "RS64_PFP_P0_DATA"},
		{AMDSMI_FW_ID_RS64_PFP_P1_DATA, "RS64_PFP_P1_DATA"},
		{AMDSMI_FW_ID_RS64_MEC, "RS64_MEC"},
		{AMDSMI_FW_ID_RS64_MEC_P0_DATA, "RS64_MEC_P0_DATA"},
		{AMDSMI_FW_ID_RS64_MEC_P1_DATA, "RS64_MEC_P1_DATA"},
		{AMDSMI_FW_ID_RS64_MEC_P2_DATA, "RS64_MEC_P2_DATA"},
		{AMDSMI_FW_ID_RS64_MEC_P3_DATA, "RS64_MEC_P3_DATA"},
		{AMDSMI_FW_ID_PPTABLE, "PPTABLE"},
		{AMDSMI_FW_ID_PSP_SOC, "PSP_SOC"},
		{AMDSMI_FW_ID_PSP_DBG, "PSP_DBG"},
		{AMDSMI_FW_ID_PSP_INTF, "PSP_INTF"},
		{AMDSMI_FW_ID_RLX6_CORE1, "RLX6_CORE1"},
		{AMDSMI_FW_ID_RLX6_DRAM_BOOT_CORE1, "RLX6_DRAM_BOOT_CORE1"},
		{AMDSMI_FW_ID_RLCV_LX7, "RLCV_LX7"},
		{AMDSMI_FW_ID_RLC_SAVE_RESTORE_LIST, "RLC_SAVE_RESTORE_LIST"},
		{AMDSMI_FW_ID_ASD, "ASD"},
		{AMDSMI_FW_ID_TA_RAS, "TA_RAS"},
		{AMDSMI_FW_ID_XGMI, "XGMI"},
		{AMDSMI_FW_ID_RLC_SRLG, "RLC_SRLG"},
		{AMDSMI_FW_ID_RLC_SRLS, "RLC_SRLS"},
		{AMDSMI_FW_ID_SMC, "SMC"},
		{AMDSMI_FW_ID_DMCU, "DMCU"},
		{AMDSMI_FW_ID_PSP_RAS, "PSP_RAS"},
		{AMDSMI_FW_ID_P2S_TABLE, "P2S_TABLE"}
	};

	out = fw_blocks((amdsmi_fw_block_t)fw_block);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_vf_sched_state(int vf_state, std::string& out)
{
	EnumToString<amdsmi_vf_sched_state_t> vf_sched_state;
	vf_sched_state.data = {
		{AMDSMI_VF_STATE_UNAVAILABLE, "UNAVAILABLE"},
		{AMDSMI_VF_STATE_AVAILABLE, "AVAILABLE"},
		{AMDSMI_VF_STATE_ACTIVE, "ACTIVE"},
		{AMDSMI_VF_STATE_SUSPENDED, "SUSPENDED"},
		{AMDSMI_VF_STATE_FULLACCESS, "FULL ACCESS"},
		{AMDSMI_VF_STATE_DEFAULT_AVAILABLE, "DEFAULT AVAILABLE"}
	};

	out = vf_sched_state((amdsmi_vf_sched_state_t)vf_state);

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_vf_guard_state(int vf_state, std::string& out)
{
	EnumToString<amdsmi_guard_state_t> vf_guard_state;
	vf_guard_state.data = {
		{AMDSMI_GUARD_STATE_NORMAL, "NORMAL"},
		{AMDSMI_GUARD_STATE_FULL, "FULL"},
		{AMDSMI_GUARD_STATE_OVERFLOW, "OVERFLOW"}
	};

	out = vf_guard_state((amdsmi_guard_state_t)vf_state);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_vf_guard_type(int guard_type, std::string& out)
{
	EnumToString<amdsmi_guard_type_t> vf_guard_type;
	vf_guard_type.data = {
		{AMDSMI_GUARD_EVENT_FLR, "FLR"},
		{AMDSMI_GUARD_EVENT_EXCLUSIVE_MOD, "EXCLUSIVE_MODE"},
		{AMDSMI_GUARD_EVENT_EXCLUSIVE_TIMEOUT, "EXCLUSIVE_TIMEOUT"},
		{AMDSMI_GUARD_EVENT_ALL_INT, "ALLOWED_INTERRUPT"}
	};

	out = vf_guard_type((amdsmi_guard_type_t)guard_type);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_vram_type(int vram_type, std::string& out)
{
	EnumToString<amdsmi_vram_type_t> enum_vram_type;
	enum_vram_type.data = {
		{AMDSMI_VRAM_TYPE_UNKNOWN, "UNKNOWN"},
		{AMDSMI_VRAM_TYPE_HBM, "HBM"},
		{AMDSMI_VRAM_TYPE_HBM2, "HBM2"},
		{AMDSMI_VRAM_TYPE_HBM2E, "HBM2E"},
		{AMDSMI_VRAM_TYPE_HBM3, "HBM3"},
		{AMDSMI_VRAM_TYPE_DDR2, "DDR2"},
		{AMDSMI_VRAM_TYPE_DDR3, "DDR3"},
		{AMDSMI_VRAM_TYPE_DDR4, "DDR4"},
		{AMDSMI_VRAM_TYPE_GDDR1, "GDDR1"},
		{AMDSMI_VRAM_TYPE_GDDR2, "GDDR2"},
		{AMDSMI_VRAM_TYPE_GDDR3, "GDDR3"},
		{AMDSMI_VRAM_TYPE_GDDR4, "GDDR4"},
		{AMDSMI_VRAM_TYPE_GDDR5, "GDDR5"},
		{AMDSMI_VRAM_TYPE_GDDR6, "GDDR6"},
		{AMDSMI_VRAM_TYPE_GDDR7, "GDDR7"}
	};

	out = enum_vram_type((amdsmi_vram_type_t)vram_type);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_accelerator_partition_type(int partition_type,
		std::string& out)
{
	EnumToString<amdsmi_accelerator_partition_type_t> acc_partition_type;
	acc_partition_type.data = {
		{AMDSMI_ACCELERATOR_PARTITION_INVALID, "INVALID"},
		{AMDSMI_ACCELERATOR_PARTITION_SPX, "SPX"},
		{AMDSMI_ACCELERATOR_PARTITION_DPX, "DPX"},
		{AMDSMI_ACCELERATOR_PARTITION_TPX, "TPX"},
		{AMDSMI_ACCELERATOR_PARTITION_QPX, "QPX"},
		{AMDSMI_ACCELERATOR_PARTITION_CPX, "CPX"}
	};

	out = acc_partition_type((amdsmi_accelerator_partition_type_t)partition_type);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_mp_setting(int mp_setting, std::string& out)
{
	EnumToString<amdsmi_memory_partition_type_t> enum_mp_setting;
	enum_mp_setting.data = {
		{AMDSMI_MEMORY_PARTITION_NPS1, "NPS1"},
		{AMDSMI_MEMORY_PARTITION_NPS2, "NPS2"},
		{AMDSMI_MEMORY_PARTITION_NPS4, "NPS4"},
		{AMDSMI_MEMORY_PARTITION_NPS8, "NPS8"},
	};

	out = enum_mp_setting((amdsmi_memory_partition_type_t)mp_setting);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_resource_type(int resource_type, std::string& out)
{
	EnumToString<amdsmi_accelerator_partition_resource_type_t> enum_resource_type;
	enum_resource_type.data = {
		{AMDSMI_ACCELERATOR_XCC, "XCC"},
		{AMDSMI_ACCELERATOR_ENCODER, "ENCODER"},
		{AMDSMI_ACCELERATOR_DECODER, "DECODER"},
		{AMDSMI_ACCELERATOR_DMA, "DMA"},
		{AMDSMI_ACCELERATOR_JPEG, "JPEG"},
		{AMDSMI_ACCELERATOR_MAX, "MAX"}
	};

	out = enum_resource_type((amdsmi_accelerator_partition_resource_type_t)resource_type);
	return AMDSMI_STATUS_SUCCESS;
}

std::vector<std::string> decode_memory_caps(uint32_t nps_cap_mask)
{
	std::vector<std::string> nps_modes;
	if (nps_cap_mask & AMDSMI_MEMORY_PARTITION_NPS1) {
		nps_modes.push_back("NPS1");
	}
	if (nps_cap_mask & AMDSMI_MEMORY_PARTITION_NPS2) {
		nps_modes.push_back("NPS2");
	}
	if (nps_cap_mask & AMDSMI_MEMORY_PARTITION_NPS4) {
		nps_modes.push_back("NPS4");
	}
	if (nps_cap_mask & AMDSMI_MEMORY_PARTITION_NPS8) {
		nps_modes.push_back("NPS8");
	}
	return nps_modes;
}

std::string possible_memory_caps_to_human_readable(const std::vector<std::string>& nps_modes)
{
	std::string result;
	for (size_t i = 0; i < nps_modes.size(); i++) {
		result += nps_modes[i];
		if (i < nps_modes.size() - 1) {
			result += ",";
		}
	}
	return result;
}

int AmdSmiApiHost::get_string_from_enum_vram_vendor_type(int vram_vendor_type, std::string& out)
{
	EnumToString<amdsmi_vram_vendor_t> enum_vram_ven_type;
	enum_vram_ven_type.data = {
		{AMDSMI_VRAM_VENDOR_SAMSUNG, "SAMSUNG"},
		{AMDSMI_VRAM_VENDOR_INFINEON, "INFINEON"},
		{AMDSMI_VRAM_VENDOR_ELPIDA, "ELPIDA"},
		{AMDSMI_VRAM_VENDOR_ETRON, "ETRON"},
		{AMDSMI_VRAM_VENDOR_NANYA, "NANYA"},
		{AMDSMI_VRAM_VENDOR_HYNIX, "HYNIX"},
		{AMDSMI_VRAM_VENDOR_MOSEL, "MOSEL"},
		{AMDSMI_VRAM_VENDOR_WINBOND, "WINBOND"},
		{AMDSMI_VRAM_VENDOR_ESMT, "ESMT"},
		{AMDSMI_VRAM_VENDOR_MICRON, "MICRON"},
	};

	out = enum_vram_ven_type((amdsmi_vram_vendor_t)vram_vendor_type);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_driver_model(int driver_model, std::string& out)
{
	EnumToString<amdsmi_driver_model_type_t> enum_driver_model;
	enum_driver_model.data = {
		{AMDSMI_DRIVER_MODEL_TYPE_WDDM, "WDDM"},
		{AMDSMI_DRIVER_MODEL_TYPE_WDM, "WDM"},
		{AMDSMI_DRIVER_MODEL_TYPE_MCDM, "MCDM"}
	};

	out = enum_driver_model((amdsmi_driver_model_type_t)driver_model);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::get_string_from_enum_cper_severity_mask(int severity_mask, std::string& out)
{
	EnumToString<amdsmi_cper_sev_t> enum_severity_mask;
	enum_severity_mask.data = {
		{AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED, "NONFATAL-UNCORRECTED"},
		{AMDSMI_CPER_SEV_FATAL, "FATAL               "},
		{AMDSMI_CPER_SEV_NON_FATAL_CORRECTED, "NONFATAL-CORRECTED  "}
	};

	out = enum_severity_mask((amdsmi_cper_sev_t)severity_mask);
	return AMDSMI_STATUS_SUCCESS;
}

std::vector<std::string> splitString(const std::string& s,
									 const std::string& delimiter, bool skipEmptyParts)
{
	std::vector<std::string> result;
	size_t last = 0, next = 0;
	while ((next = s.find(delimiter, last)) != std::string::npos) {
		if(!skipEmptyParts || next != last)
			result.push_back(s.substr(last, next - last));
		last = next + delimiter.length();
	}
	if (!skipEmptyParts || last != s.size())
		result.push_back(s.substr(last));
	return result;
}

int AmdSmiApiHost::get_string_from_enum_ecc_blocks(int ecc_block, std::string& out)
{
	EnumToString<amdsmi_gpu_block_t> ecc_blocks;
	ecc_blocks.data = {
		{AMDSMI_GPU_BLOCK_UMC, "UMC"},
		{AMDSMI_GPU_BLOCK_SDMA, "SDMA"},
		{AMDSMI_GPU_BLOCK_GFX, "GFX"},
		{AMDSMI_GPU_BLOCK_MMHUB, "MMHUB"},
		{AMDSMI_GPU_BLOCK_ATHUB, "ATHUB"},
		{AMDSMI_GPU_BLOCK_PCIE_BIF, "PCIE_BIF"},
		{AMDSMI_GPU_BLOCK_HDP, "HDP"},
		{AMDSMI_GPU_BLOCK_XGMI_WAFL, "XGMI_WAFL"},
		{AMDSMI_GPU_BLOCK_DF, "DF"},
		{AMDSMI_GPU_BLOCK_SMN, "SMN"},
		{AMDSMI_GPU_BLOCK_SEM, "SEM"},
		{AMDSMI_GPU_BLOCK_MP0, "MP0"},
		{AMDSMI_GPU_BLOCK_MP1, "MP1"},
		{AMDSMI_GPU_BLOCK_FUSE, "FUSE"},
		{AMDSMI_GPU_BLOCK_MCA, "MCA"},
		{AMDSMI_GPU_BLOCK_VCN, "VCN"},
		{AMDSMI_GPU_BLOCK_JPEG, "JPEG"},
		{AMDSMI_GPU_BLOCK_IH, "IH"},
		{AMDSMI_GPU_BLOCK_MPIO, "MPIO"}
	};

	out = ecc_blocks((amdsmi_gpu_block_t)ecc_block);
	return AMDSMI_STATUS_SUCCESS;
}
int AmdSmiApiHost::transform_cache_properties(uint32_t initial_property,
		std::vector<std::string>& properties)
{
	uint32_t property_data_cache = AMDSMI_CACHE_PROPERTY_DATA_CACHE & initial_property;
	uint32_t property_inst_cache = AMDSMI_CACHE_PROPERTY_INST_CACHE & initial_property;
	uint32_t property_cpu_cache = AMDSMI_CACHE_PROPERTY_CPU_CACHE & initial_property;
	uint32_t property_simd_cache = AMDSMI_CACHE_PROPERTY_SIMD_CACHE & initial_property;

	if(AMDSMI_CACHE_PROPERTY_DATA_CACHE == property_data_cache) {
		properties.push_back("DATA_CACHE");
	}
	if(AMDSMI_CACHE_PROPERTY_INST_CACHE == property_inst_cache) {
		properties.push_back("INST_CACHE");
	}
	if(AMDSMI_CACHE_PROPERTY_CPU_CACHE == property_cpu_cache) {
		properties.push_back("CPU_CACHE");
	}
	if(AMDSMI_CACHE_PROPERTY_SIMD_CACHE == property_simd_cache) {
		properties.push_back("SIMD_CACHE");
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::csv_recursion(std::string& main_buffer,
								 const std::vector<std::vector<std::string>> &results)
{
	std::vector<std::string> prefix{};
	std::vector<std::vector<std::string>> sorted_results = results;

	sort( sorted_results.begin(), sorted_results.end(),
	[](const std::vector<std::string> &a, const std::vector<std::string> &b) {
		return a.size() > b.size();
	});

	std::map<int, int> order_map;
	for (int i = 0; i < sorted_results.size(); i++) {
		for (int j = 0; j < sorted_results.size(); j++) {
			if (sorted_results[i] == results[j]) {
				order_map[i] = j;
				prefix.push_back("");
			}
		}
	}

	csv_recursive_function(main_buffer, prefix, 0, sorted_results, order_map);
	return 0;
}
