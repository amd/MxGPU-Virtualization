/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <iostream>

#include "json/json.h"

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_bad_pages_command.h"
#include "smi_cli_templates.h"
#include "smi_cli_api_base.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

#define AMDSMI_STATUS_NO_DATA 40

auto constexpr
bad_pages_header_csv {",bad_page,retired_bad_page,timestamp,mem_channel,mcumc_id"};

int AmdSmiBadPagesCommand::bad_pages_command(uint64_t processors,
		std::string &out_string, std::string* gpu_id)
{
	if (AmdSmiPlatform::getInstance().is_guest() || AmdSmiPlatform::getInstance().is_baremetal()) {
		return COMMAND_NOT_SUPPORTED_ON_PLATFORM;
	}
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bad_pages_command(processors,
			  arg, out_string, gpu_id);
	return ret;
}

void AmdSmiBadPagesCommand::execute_command()
{
	unsigned int i;
	std::string out{};
	std::string formatted_string{};
	std::string gpu_index_str{};
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	std::string headers{};
	nlohmann::ordered_json values_json{};
	std::vector<std::pair<int, std::string>> gpu_data;
	bool foundSuccessfulCall = false;

	for (i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		int gpu_index = arg.devices[i]->get_gpu_index();
		int ret;
		if (arg.output == OutputFormat::csv) {
			gpu_index_str =	string_format("%d", gpu_index);
			ret = bad_pages_command(gpu_bdf, out, &gpu_index_str);
		} else
			ret = bad_pages_command(gpu_bdf, out);
		if (ret == 0) {
			if (arg.output == OutputFormat::csv) {
				out.append("\n");
				gpu_data.emplace_back(gpu_index, out);
			} else {
				gpu_data.emplace_back(gpu_index, out);
			}
			out.clear();
			if (!foundSuccessfulCall) {
				foundSuccessfulCall = true;
			}
		} else if (ret != AMDSMI_STATUS_NO_DATA) {
			std::string param{"bad-pages"};
			int error = handle_exceptions(ret, param, arg);
			if (error == PARAM_NOT_SUPPORTED_ON_PLATFORM) {
				throw SmiToolParameterNotSupportedException(param);
			}
		}
	}

	if (foundSuccessfulCall) {
		if (arg.output == OutputFormat::csv) {
			headers.append("gpu").append(bad_pages_header_csv);
			formatted_string.append(headers).append("\n");
		}

		for (const auto& bp : gpu_data) {
			const auto& gpu_index = bp.first;
			const auto& bad_pages_out = bp.second;
			if (arg.output == OutputFormat::json) {
				json["gpu"] = gpu_index;
				values_json = nlohmann::ordered_json::parse(bad_pages_out);
				json["bad_pages"] = values_json;
				values_json.clear();
				json_format.insert(json_format.end(), json);
			} else if (arg.output == OutputFormat::human) {
				formatted_string += string_format(gpuTemplate, gpu_index);
			}
			formatted_string += bad_pages_out;
		}
	} else {
		throw SmiToolSMILIBErrorException(AMDSMI_STATUS_NO_DATA);
	}

	if (arg.output == OutputFormat::json) {
		formatted_string = json_format.dump(4);
	}
	if (arg.is_file) {
		write_to_file(arg.file_path, formatted_string);
	} else {
		std::cout << formatted_string.c_str() << std::endl;
	}
};