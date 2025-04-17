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

#include "json/json.h"

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

auto constexpr list_header_csv {"gpu,gpu_bdf,gpu_uuid,vf,vf_bdf,vf_uuid\n"};

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_UUID)(amdsmi_processor_handle, unsigned int *,
		char *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_BDF)(amdsmi_vf_handle_t, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_UUID)(amdsmi_vf_handle_t, unsigned int *, char *);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint32_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_PARTITION_INFO)(amdsmi_processor_handle, unsigned int,
		amdsmi_partition_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_GPU_DEVICE_UUID host_amdsmi_get_gpu_device_uuid;
extern AMDSMI_GET_VF_BDF host_amdsmi_get_vf_bdf;
extern AMDSMI_GET_VF_UUID host_amdsmi_get_vf_uuid;
extern AMDSMI_GET_NUM_VF host_amdsmi_get_num_vf;
extern AMDSMI_GET_VF_PARTITION_INFO host_amdsmi_get_vf_partition_info;


int AmdSmiApiHost::list_command(int format, unsigned int gpu_index, std::string bdf,
								char *uuid, std::string &formatted_string)
{
	if (format == csv) {
		formatted_string = string_format(
							   "%d,%s,%s,", gpu_index, bdf.c_str(), uuid);
	} else {
		formatted_string = string_format(
							   gpuListTemplate, gpu_index, bdf.c_str(), uuid);
	}
	return AMDSMI_STATUS_SUCCESS;
}


int AmdSmiApiHost::amdsmi_get_list_command(Arguments arg, std::string& out)
{
	amdsmi_bdf_t tmp_bdf;
	char uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int length = AMDSMI_GPU_UUID_SIZE;
	amdsmi_socket_handle socket = NULL;
	auto vf_list_json = nlohmann::ordered_json::array();
	auto gpu_list_json = nlohmann::ordered_json::array();
	nlohmann::ordered_json json_vfs;
	std::string formatted_string{};
	amdsmi_processor_handle processor;
	out = {};

	if (arg.output == csv) {
		out.append(list_header_csv);
	}

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		int64_t gpu_bdf = arg.devices[i]->get_bdf();
		tmp_bdf.as_uint = gpu_bdf;
		int ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			return ret;
		}
		ret = host_amdsmi_get_gpu_device_bdf(processor, &tmp_bdf);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			throw SmiToolSMILIBErrorException(ret);
		}
		ret = host_amdsmi_get_gpu_device_uuid(processor, &length, uuid);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			throw SmiToolSMILIBErrorException(ret);
		}
		std::string bdf{ convert_bdf_to_string(tmp_bdf.bdf.function_number, tmp_bdf.bdf.device_number, tmp_bdf.bdf.bus_number, tmp_bdf.bdf.domain_number) };

		// get the vf info
		std::string vf_string{};
		std::vector<std::string> csv_vf{};
		amdsmi_bdf_t vf_bdf;
		char vf_uuid[AMDSMI_GPU_UUID_SIZE];
		uint32_t num_vf_supported;
		uint32_t num_vf_enabled;
		amdsmi_partition_info_t partitions[AMDSMI_MAX_VF_COUNT];
		host_amdsmi_get_num_vf(processor, &num_vf_enabled, &num_vf_supported);
		host_amdsmi_get_vf_partition_info(processor, num_vf_enabled, partitions);

		for (uint8_t j = 0; j < num_vf_enabled; j++) {
			unsigned int vf_length = AMDSMI_GPU_UUID_SIZE;
			ret = host_amdsmi_get_vf_bdf(partitions[j].id, &vf_bdf);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				throw SmiToolSMILIBErrorException(ret);
			}
			std::string vfbdf{ convert_bdf_to_string(
								   vf_bdf.bdf.function_number, vf_bdf.bdf.device_number, vf_bdf.bdf.bus_number, vf_bdf.bdf.domain_number) };
			ret = host_amdsmi_get_vf_uuid(partitions[j].id,&vf_length, vf_uuid);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				throw SmiToolSMILIBErrorException(ret);
			}
			if (arg.output == json) {
				vf_list_json.push_back(nlohmann::ordered_json::object({{ "vf", j },
					{ "bdf", vfbdf },
					{ "uuid", vf_uuid }}));
			} else if (arg.output == csv) {
				std::string vf{ string_format("%d,%s,%s\n", j, vfbdf.c_str(), vf_uuid) };
				csv_vf.push_back(vf);
			} else {
				vf_string += string_format(
								 vfListTemplate, j, vfbdf.c_str(), vf_uuid);
			}
		}
		if (arg.output == json) {
			gpu_list_json.push_back(nlohmann::ordered_json::object({{ "gpu", arg.devices[i]->get_gpu_index() }, { "bdf", bdf },
				{ "uuid", uuid }, { "vfs", vf_list_json }}));
		} else if (arg.output == csv) {
			list_command(arg.output, arg.devices[i]->get_gpu_index(), bdf, uuid, formatted_string);
			if (csv_vf.size() > 0) {
				for (auto vf : csv_vf) {
					out.append(formatted_string);
					out.append(vf);
				}
			} else {
				out.append(formatted_string);
			}
		} else {
			list_command(arg.output, arg.devices[i]->get_gpu_index(), bdf, uuid, formatted_string);
			out += formatted_string;
			out += vf_string;
		}
		vf_list_json.clear();
	}

	if (arg.output == json) {
		out += gpu_list_json.dump(4);
	}
	return 0;
}

