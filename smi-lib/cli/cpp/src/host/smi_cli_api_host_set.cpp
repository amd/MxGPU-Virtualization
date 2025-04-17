/* * Copyright (C) 2024 Advanced Micro Devices. All rights reserved.
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
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_exception.h"

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_SET_XGMI_FB_SHARING_MODE_V2)(amdsmi_processor_handle, uint32_t,
		amdsmi_xgmi_fb_sharing_mode_t);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO)(amdsmi_processor_handle,
		amdsmi_xgmi_fb_sharing_mode_t);
typedef amdsmi_status_t (*AMDSMI_SET_ACCELERATOR_PARTITION)(amdsmi_processor_handle, uint32_t);
typedef amdsmi_status_t (*AMDSMI_SET_MEMORY_PARTITION)(amdsmi_processor_handle,
		amdsmi_memory_partition_type_t);
typedef amdsmi_status_t (*AMDSMI_SET_SOC_PSTATE)(amdsmi_processor_handle,
		uint32_t);
typedef amdsmi_status_t (*AMDSMI_SET_POWER_CAP)(amdsmi_processor_handle, uint32_t,
		uint64_t);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO host_amdsmi_set_xgmi_fb_sharing_mode_info;
extern AMDSMI_SET_XGMI_FB_SHARING_MODE_V2 host_amdsmi_set_xgmi_fb_sharing_mode_v2;
extern AMDSMI_SET_ACCELERATOR_PARTITION host_amdsmi_set_gpu_accelerator_partition_command;
extern AMDSMI_SET_MEMORY_PARTITION host_amdsmi_set_gpu_memory_partition_command;
extern AMDSMI_SET_SOC_PSTATE host_amdsmi_set_soc_pstate;
extern AMDSMI_SET_POWER_CAP host_amdsmi_set_power_cap;

int AmdSmiApiHost::amdsmi_set_xgmi_fb_sharing_mode_command(std::vector<uint64_t> bfd_list,
		Arguments arg)
{
	int ret = 0;
	uint32_t i = 0;
	amdsmi_processor_handle processor;
	amdsmi_processor_handle *processor_list;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_xgmi_fb_sharing_mode_t mode;

	processor_list = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) *
					 bfd_list.size());
	if (processor_list == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	for (auto bdf: bfd_list) {
		tmp_bdf.as_uint = bdf;
		ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			free(processor_list);
			return ret;
		}
		processor_list[i] = processor;
		i++;
	}


	amdsmi_socket_handle socket = NULL;
	amdsmi_processor_handle *processors;
	unsigned int gpu_count;

	amdsmi_get_gpu_count(gpu_count);
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);

	if (arg.fb_sharing_mode == "CUSTOM") {
		mode = AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM;
	} else if (arg.fb_sharing_mode == "MODE_1") {
		mode = AMDSMI_XGMI_FB_SHARING_MODE_1;
	} else if(arg.fb_sharing_mode == "MODE_2") {
		mode = AMDSMI_XGMI_FB_SHARING_MODE_2;
	} else if(arg.fb_sharing_mode == "MODE_4") {
		mode = AMDSMI_XGMI_FB_SHARING_MODE_4;
	} else if(arg.fb_sharing_mode == "MODE_8") {
		mode = AMDSMI_XGMI_FB_SHARING_MODE_8;
	} else {
		free(processors);
		free(processor_list);
		throw SmiToolInvalidParameterValueException(arg.fb_sharing_mode);
	}

	if (bfd_list.empty()) {
		for (unsigned int j = 0; j < gpu_count; j++) {
			ret = host_amdsmi_set_xgmi_fb_sharing_mode_info(processors[j], mode);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				free(processor_list);
				free(processors);
				return ret;
			}
		}
	} else {
		ret = host_amdsmi_set_xgmi_fb_sharing_mode_v2(processor_list, i, mode);
	}

	free(processor_list);
	free(processors);

	return ret;
}

int AmdSmiApiHost::amdsmi_set_accelerator_partition_command(uint64_t processor_bdf, Arguments arg)
{
	int ret;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_processor_handle processor;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_set_gpu_accelerator_partition_command(processor,
		  arg.accelerator_partition_setting);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_set_memory_partition_command(uint64_t processor_bdf, Arguments arg)
{
	int ret;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_processor_handle processor;
	amdsmi_memory_partition_type_t setting;
	tmp_bdf.as_uint = processor_bdf;

	if (arg.memory_partition_setting == "NPS1") {
		setting = AMDSMI_MEMORY_PARTITION_NPS1;
	} else if (arg.memory_partition_setting == "NPS2") {
		setting =  AMDSMI_MEMORY_PARTITION_NPS2;
	} else if(arg.memory_partition_setting == "NPS4") {
		setting = AMDSMI_MEMORY_PARTITION_NPS4;
	} else if(arg.memory_partition_setting == "NPS8") {
		setting = AMDSMI_MEMORY_PARTITION_NPS8;
	} else {
		setting = AMDSMI_MEMORY_PARTITION_UNKNOWN;
	}

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_set_gpu_memory_partition_command(processor, setting);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_set_soc_pstate_command(uint64_t processor_bdf, Arguments arg)
{
	int ret, i = 0;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	uint32_t mode;

	tmp_bdf.as_uint = processor_bdf;
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	if (arg.pstate_set == "0") {
		mode = 0;
	} else if(arg.pstate_set == "1") {
		mode = 1;
	} else if(arg.pstate_set == "2") {
		mode = 2;
	} else if(arg.pstate_set == "3") {
		mode = 3;
	} else {
		return INVALID_PARAM_VALUE;
	}

	ret = host_amdsmi_set_soc_pstate(processor, mode);

	return ret;
}

int AmdSmiApiHost::amdsmi_set_power_cap_command(uint64_t processor_bdf, Arguments arg)
{
	int ret;
	uint32_t sensor_ind = 0;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;

	tmp_bdf.as_uint = processor_bdf;
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	ret = host_amdsmi_set_power_cap(processor, sensor_ind, arg.power_cap_set);
	return ret;
}
