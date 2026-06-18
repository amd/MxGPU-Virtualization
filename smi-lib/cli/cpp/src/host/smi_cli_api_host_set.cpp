/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
typedef amdsmi_status_t (*AMDSMI_SET_XGMI_PLPD)(amdsmi_processor_handle,
		uint32_t);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_PTL_STATE)(amdsmi_processor_handle, bool);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_PTL_FORMATS)(amdsmi_processor_handle,
		amdsmi_ptl_data_format_t, amdsmi_ptl_data_format_t);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO)(amdsmi_processor_handle,
		amdsmi_processor_handle,
		amdsmi_xgmi_fb_sharing_mode_t, uint8_t *);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_TOPOLOGY)(amdsmi_processor_handle,
		amdsmi_processor_handle,
		amdsmi_link_topology_t *);
typedef amdsmi_status_t (*AMDSMI_SET_NUM_VF)(amdsmi_processor_handle, uint32_t);
typedef amdsmi_status_t (*AMDSMI_SET_CC_MODE)(amdsmi_processor_handle,
		amdsmi_cc_mode_t);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO host_amdsmi_set_xgmi_fb_sharing_mode_info;
extern AMDSMI_SET_XGMI_FB_SHARING_MODE_V2 host_amdsmi_set_xgmi_fb_sharing_mode_v2;
extern AMDSMI_SET_ACCELERATOR_PARTITION host_amdsmi_set_gpu_accelerator_partition_command;
extern AMDSMI_SET_MEMORY_PARTITION host_amdsmi_set_gpu_memory_partition_command;
extern AMDSMI_SET_SOC_PSTATE host_amdsmi_set_soc_pstate;
extern AMDSMI_SET_POWER_CAP host_amdsmi_set_power_cap;
extern AMDSMI_SET_XGMI_PLPD host_amdsmi_set_xgmi_plpd;
extern AMDSMI_SET_GPU_PTL_STATE host_amdsmi_set_gpu_ptl_state;
extern AMDSMI_SET_GPU_PTL_FORMATS host_amdsmi_set_gpu_ptl_formats;
extern AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO host_amdsmi_get_xgmi_fb_sharing_mode_info;
extern AMDSMI_GET_LINK_TOPOLOGY host_amdsmi_get_link_topology;
extern AMDSMI_SET_NUM_VF host_amdsmi_set_num_vf;
extern AMDSMI_SET_CC_MODE host_amdsmi_set_cc_mode;



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

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		goto end;
	}
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
		uint8_t is_fb_sharing_enabled{0};
		amdsmi_link_topology_t topology_info;
		ret = host_amdsmi_set_xgmi_fb_sharing_mode_info(processors[0], mode);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			goto end;
		}
		for (uint32_t i = 1; i < gpu_count; i++) {
			for (uint32_t j = 0; j < gpu_count; j++) {
				ret = host_amdsmi_get_xgmi_fb_sharing_mode_info(processors[i], processors[j], mode, &is_fb_sharing_enabled);
				if (ret != AMDSMI_STATUS_SUCCESS) {
						goto end;
				}
				ret = host_amdsmi_get_link_topology(processors[i], processors[j], &topology_info);
				if (ret != AMDSMI_STATUS_SUCCESS) {
						goto end;
				}
				if (is_fb_sharing_enabled != topology_info.fb_sharing) {
					ret = host_amdsmi_set_xgmi_fb_sharing_mode_info(processors[i], mode);
					if (ret != AMDSMI_STATUS_SUCCESS) {
						goto end;
					}
					break;
				}
			}
		}
	} else {
		ret = host_amdsmi_set_xgmi_fb_sharing_mode_v2(processor_list, i, mode);
	}

end:
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
		throw SmiToolInvalidParameterValueException(arg.pstate_set);
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

int AmdSmiApiHost::amdsmi_set_plpd_command(uint64_t processor_bdf, Arguments arg)
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

	if (arg.plpd_set == "0") {
		mode = 0;
	} else if(arg.plpd_set == "1") {
		mode = 1;
	} else if(arg.plpd_set == "2") {
		mode = 2;
	} else {
		throw SmiToolInvalidParameterValueException(arg.plpd_set);
	}

	ret = host_amdsmi_set_xgmi_plpd(processor, mode);

	return ret;
}

int AmdSmiApiHost::amdsmi_set_ptl_status_command(uint64_t processor_bdf, Arguments arg)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	bool enable;

	tmp_bdf.as_uint = processor_bdf;
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	// Convert string to bool
	std::string status_lower = arg.ptl_status_set;
	std::transform(status_lower.begin(), status_lower.end(), status_lower.begin(), ::tolower);

	if (status_lower == "enabled" || status_lower == "enable" || status_lower == "1" || status_lower == "true") {
		enable = true;
	} else if (status_lower == "disabled" || status_lower == "disable" || status_lower == "0" || status_lower == "false") {
		enable = false;
	} else {
		throw SmiToolInvalidParameterValueException(arg.ptl_status_set);
	}

	ret = host_amdsmi_set_gpu_ptl_state(processor, enable);

	return ret;
}

int AmdSmiApiHost::amdsmi_set_ptl_format_command(uint64_t processor_bdf, Arguments arg)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_ptl_data_format_t format1, format2;

	tmp_bdf.as_uint = processor_bdf;
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	// Parse format string "FORMAT1,FORMAT2"
	size_t comma_pos = arg.ptl_format_set.find(',');
	if (comma_pos == std::string::npos) {
		throw SmiToolInvalidParameterValueException(arg.ptl_format_set);
	}

	std::string format1_str = arg.ptl_format_set.substr(0, comma_pos);
	std::string format2_str = arg.ptl_format_set.substr(comma_pos + 1);

	// Convert strings to uppercase for comparison
	std::transform(format1_str.begin(), format1_str.end(), format1_str.begin(), ::toupper);
	std::transform(format2_str.begin(), format2_str.end(), format2_str.begin(), ::toupper);

	// Map format strings to enum
	auto parse_format = [](const std::string& fmt) -> amdsmi_ptl_data_format_t {
		if (fmt == "I8") return AMDSMI_PTL_DATA_FORMAT_I8;
		if (fmt == "F16") return AMDSMI_PTL_DATA_FORMAT_F16;
		if (fmt == "BF16") return AMDSMI_PTL_DATA_FORMAT_BF16;
		if (fmt == "F32") return AMDSMI_PTL_DATA_FORMAT_F32;
		if (fmt == "F64") return AMDSMI_PTL_DATA_FORMAT_F64;
		if (fmt == "F8") return AMDSMI_PTL_DATA_FORMAT_F8;
		if (fmt == "VECTOR") return AMDSMI_PTL_DATA_FORMAT_VECTOR;
		return AMDSMI_PTL_DATA_FORMAT_INVALID;
	};

	format1 = parse_format(format1_str);
	format2 = parse_format(format2_str);

	if (format1 == AMDSMI_PTL_DATA_FORMAT_INVALID || format2 == AMDSMI_PTL_DATA_FORMAT_INVALID) {
		throw SmiToolInvalidParameterValueException(arg.ptl_format_set);
	}

	ret = host_amdsmi_set_gpu_ptl_formats(processor, format1, format2);

	return ret;
}

int AmdSmiApiHost::amdsmi_set_num_vf_command(uint64_t processor_bdf, Arguments arg)
{
	int ret, i = 0;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	uint32_t num_vf = static_cast<uint32_t>(std::stoul(arg.num_vf));
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	ret = host_amdsmi_set_num_vf(processor, num_vf);

	return ret;
}

int AmdSmiApiHost::amdsmi_set_cc_mode_command(uint64_t processor_bdf, Arguments arg)
{
	int ret;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_processor_handle processor;
	amdsmi_cc_mode_t mode;
	tmp_bdf.as_uint = processor_bdf;

	if (arg.cc_mode_setting == "OFF") {
		mode = AMDSMI_CC_MODE_OFF;
	} else if (arg.cc_mode_setting == "ON") {
		mode = AMDSMI_CC_MODE_ON;
	} else if(arg.cc_mode_setting == "DEV") {
		mode = AMDSMI_CC_MODE_DEV;
	} else {
		throw SmiToolInvalidParameterValueException(arg.cc_mode_setting);
	}

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	ret = host_amdsmi_set_cc_mode(processor, mode);

	return ret;
}
