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
#include "smi_cli_exception.h"

#include "json/json.h"

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_UUID)(amdsmi_processor_handle, unsigned int *,
		char *);

extern AMDSMI_GET_PROCESSOR_HANDLES guest_amdsmi_get_processor_handles;
extern AMDSMI_GET_GPU_DEVICE_BDF guest_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_GPU_DEVICE_UUID guest_amdsmi_get_gpu_device_uuid;

int AmdSmiApiGuest::list_command(int format, unsigned int gpu_index, std::string bdf,
								 char *uuid, std::string &formatted_string)
{
	if (format == csv) {
		formatted_string = string_format(
							   "%d,%s,%s", gpu_index, bdf.c_str(), uuid);
	} else {
		formatted_string = string_format(
							   gpuListTemplate, gpu_index, bdf.c_str(), uuid);
	}
	return AMDSMI_STATUS_SUCCESS;
}


int AmdSmiApiGuest::amdsmi_get_list_command(Arguments arg, std::string& out)
{
	amdsmi_bdf_t tmp_bdf;
	char uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int length = AMDSMI_GPU_UUID_SIZE;
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;
	auto vf_list_json = nlohmann::ordered_json::array();
	auto gpu_list_json = nlohmann::ordered_json::array();
	nlohmann::ordered_json json_vfs;
	std::string formatted_string{};
	amdsmi_processor_handle *processors;

	out = {};

	amdsmi_get_gpu_count(gpu_count);

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	int ret = guest_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(processors);
		throw SmiToolSMILIBErrorException(ret);
	}

	if (arg.output == csv) {
		out.append({"gpu,gpu_bdf,gpu_uuid\n"});
	}

	for (unsigned int i = 0; i < gpu_count; i++) {
		int ret = guest_amdsmi_get_gpu_device_bdf(processors[i], &tmp_bdf);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			free(processors);
			throw SmiToolSMILIBErrorException(ret);
		}
		ret = guest_amdsmi_get_gpu_device_uuid(processors[i], &length, uuid);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			free(processors);
			throw SmiToolSMILIBErrorException(ret);
		}
		std::string bdf{ convert_bdf_to_string(tmp_bdf.function_number, tmp_bdf.device_number, tmp_bdf.bus_number, tmp_bdf.domain_number) };

		if (arg.output == json) {
			gpu_list_json.push_back(nlohmann::ordered_json::object({{ "gpu", i }, { "bdf", bdf },
				{ "uuid", uuid }}));
		} else if (arg.output == csv) {
			list_command(arg.output, i, bdf, uuid, formatted_string);
			out.append(formatted_string).append("\n");
		} else {
			list_command(arg.output, i, bdf, uuid, formatted_string);
			out += formatted_string;
		}
		vf_list_json.clear();
	}

	if (arg.output == json) {
		out += gpu_list_json.dump(4);
	}

	free(processors);
	return ret;
}


