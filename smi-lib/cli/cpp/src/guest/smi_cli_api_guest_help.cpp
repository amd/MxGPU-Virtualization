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
#include "interface/amdsmi.h"
#include "smi_cli_api_guest.h"
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"

#include <vector>
#include <iostream>
#include <sstream>
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

extern AMDSMI_GET_PROCESSOR_HANDLES guest_amdsmi_get_processor_handles;
extern AMDSMI_GET_GPU_DEVICE_BDF guest_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_GPU_DEVICE_UUID guest_amdsmi_get_gpu_device_uuid;


int AmdSmiApiGuest::amdsmi_get_bdf_from_gpu_index(uint64_t &processor_bdf, int index)
{
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;
	amdsmi_processor_handle *processors;
	amdsmi_bdf_t tmp_bdf;
	int ret = guest_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		//FreeLib();
		//error
		exit(1);
	}
	if (index >= gpu_count) {
		//error
		exit(1);
	}
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = guest_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	amdsmi_processor_handle gpu_handle = processors[index];
	ret = guest_amdsmi_get_gpu_device_bdf(gpu_handle, &tmp_bdf);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		//FreeLib();
		//error
		free(processors);
		exit(1);
	}
	processor_bdf = tmp_bdf.as_uint;
	free(processors);

	return 0;
}

int AmdSmiApiGuest::amdsmi_get_bdf_from_uuid_or_bdf(uint64_t &processor_bdf, int &gpu_index,
		std::string device, int type)
{
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;
	amdsmi_processor_handle *processors;
	amdsmi_bdf_t tmp_bdf;
	int ret = guest_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		//FreeLib();
		//error
		exit(1);
	}
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = guest_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		//FreeLib();
		//error
		free(processors);
		exit(1);
	}
	if (type == BDF) {
		for (int i = 0; i < gpu_count; i++) {
			amdsmi_bdf_t bdf;
			ret = guest_amdsmi_get_gpu_device_bdf(processors[i], &bdf);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				free(processors);
				exit(1);
			}

			std::string bdf_string = string_format(
										 "%04x:%02x:%02x.%01x", bdf.domain_number, bdf.bus_number, bdf.device_number,
										 bdf.function_number);
			if (bdf_string == " ") {
				free(processors);
				//FreeLib();
				//error
				exit(1);
			}
			if (strcmp(bdf_string.c_str(), device.c_str()) == 0) {
				gpu_index = i;
				ret = guest_amdsmi_get_gpu_device_bdf(processors[i], &tmp_bdf);
				if (ret != AMDSMI_STATUS_SUCCESS) {
					free(processors);
					//FreeLib();
					//error
					exit(1);
				}
				processor_bdf = tmp_bdf.as_uint;
				return AMDSMI_STATUS_SUCCESS;
			}
		}
	} else {
		unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;
		char uuid[AMDSMI_GPU_UUID_SIZE] = { 0 };
		amdsmi_processor_handle gpu_handle = nullptr;
		for (int i = 0; i < gpu_count; i++) {
			ret = guest_amdsmi_get_gpu_device_uuid(processors[i], &uuid_length, uuid);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				//FreeLib();
				//error
				free(processors);
				exit(1);
			}
			if (strcmp(uuid, device.c_str()) == 0) {
				gpu_index = i;
				gpu_handle = processors[i];
				ret = guest_amdsmi_get_gpu_device_bdf(processors[i], &tmp_bdf);
				if (ret != AMDSMI_STATUS_SUCCESS) {
					//FreeLib();
					//error
					free(processors);
					exit(1);
				}
				processor_bdf = tmp_bdf.as_uint;
				return AMDSMI_STATUS_SUCCESS;
			}
		}
		if (gpu_handle == nullptr) {
			//error
			free(processors);
			std::cout << "error uuid is not valid" << std::endl;
			exit(1);
		}
	}

	free(processors);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_gpu_count(unsigned int &gpu_count)
{
	amdsmi_socket_handle socket = NULL;
	int ret = guest_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		//FreeLib();
		//error
		exit(1);
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_error_message(int error_code, std::string& out)
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



int AmdSmiApiGuest::get_string_from_enum_fw_block(int fw_block, std::string& out)
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
int AmdSmiApiGuest::csv_recursion(std::string& main_buffer,
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
