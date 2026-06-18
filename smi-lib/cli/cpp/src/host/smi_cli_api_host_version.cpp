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

#include "json/json.h"

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_LIB_VERSION)(amdsmi_version_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
	amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_INFO)(amdsmi_processor_handle,
		amdsmi_driver_info_t *);

extern AMDSMI_GET_LIB_VERSION host_amdsmi_get_lib_version;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DRIVER_INFO host_amdsmi_get_gpu_driver_info;

int AmdSmiApiHost::amdsmi_get_version_command(uint64_t processor_bdf, Arguments arg,
		std::string &out_string)
{
	std::string out{};
	amdsmi_version_t version;
	amdsmi_driver_info_t driver_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_driver_model_type_t driver_model;
	tmp_bdf.as_uint = processor_bdf;
	amdsmi_status_t ret;
	nlohmann::ordered_json json_out;


	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_lib_version(&version);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	std::string driver_version{};
	ret = host_amdsmi_get_gpu_driver_info(processor, &driver_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		driver_version = "N/A";
		return ret;
	} else {
		driver_version = driver_info.driver_version;
	}

	std::string amdsmi_lib_ver_str = string_format("%ld.%ld.%ld", version.major,
									 version.minor, version.release);

	if (arg.output == json) {
		json_out = {
			{"tool_name", AMDSMI_TOOL_NAME},
			{"tool_version", AMDSMI_TOOL_VERSION_STRING},
			{"lib_version", amdsmi_lib_ver_str.c_str()},
			{"driver_version", driver_version.c_str()}
		};
	} else if (arg.output == csv) {
		out += string_format(
				   "%s,%s,%s,%s", AMDSMI_TOOL_NAME,
				   AMDSMI_TOOL_VERSION_STRING,
				   amdsmi_lib_ver_str.c_str(),
				   driver_version.c_str());
	} else {
		out += string_format(
				   versionTemplate, AMDSMI_TOOL_NAME,
				   AMDSMI_TOOL_VERSION_STRING,
				   amdsmi_lib_ver_str.c_str(),
				   driver_version.c_str());
	}

	if (arg.output == json) {
		out_string += json_out.dump(4);
	} else {
		out_string += out;
	}

	return AMDSMI_STATUS_SUCCESS;
}
