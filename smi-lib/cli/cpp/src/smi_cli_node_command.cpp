/* * Copyright (C) 2025 Advanced Micro Devices. All rights reserved.
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

#include "smi_cli_platform.h"
#include "smi_cli_exception.h"
#include "smi_cli_node_command.h"
#include "smi_cli_helpers.h"
#include "smi_cli_api_base.h"
#include "smi_cli_templates.h"

#include "json/json.h"

auto constexpr baseboard_csv_header {"baseboard_temperature_ubb_fpga,baseboard_temperature_ubb_front,baseboard_temperature_ubb_back,baseboard_temperature_ubb_oam7,baseboard_temperature_ubb_ibc,baseboard_temperature_ubb_ufpga,baseboard_temperature_ubb_oam1,baseboard_temperature_oam_0_1_hsc,baseboard_temperature_oam_2_3_hsc,baseboard_temperature_oam_4_5_hsc,baseboard_temperature_oam_6_7_hsc,baseboard_temperature_ubb_fpga_0v72_vr,baseboard_temperature_ubb_fpga_3v3_vr,baseboard_temperature_retimer_0_1_2_3_1v2_vr,baseboard_temperature_retimer_4_5_6_7_1v2_vr,baseboard_temperature_retimer_0_1_0v9_vr,baseboard_temperature_retimer_4_5_0v9_vr,baseboard_temperature_retimer_2_3_0v9_vr,baseboard_temperature_retimer_6_7_0v9_vr,baseboard_temperature_oam_0_1_2_3_3v3_vr,baseboard_temperature_oam_4_5_6_7_3v3_vr,baseboard_temperature_ibc_hsc,baseboard_temperature_ibc"};

int AmdSmiNodeCommand::node_command_baseboard(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_baseboard_command(processor,
			  arg, formatted_string);
	return ret;
}

void AmdSmiNodeCommand::node_command_human()
{
	int ret;
	bool is_supported = false;
	std::string formatted_string{};
	std::string out{};

	out += NodeHeaderTemplate;

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		if ((std::find(arg.options.begin(), arg.options.end(), "baseboard") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
				arg.all_arguments) {
			ret = node_command_baseboard(gpu_bdf, formatted_string);
			is_supported = (ret == 0);
			std::string param{"baseboard"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
				formatted_string.clear();
			}
			formatted_string.clear();
		}
	}

	if (!is_supported) {
		std::string command{"node"};
		throw SmiToolCommandNotSupportedException(command);
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiNodeCommand::node_command_json()
{
	int ret;
	unsigned int i;
	bool is_supported = false;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	std::string out{};
	std::string result{};
	for (i = 0; i < arg.devices.size(); i++) {
		json = {};
		nlohmann::ordered_json values_json;
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();

		if ((std::find(arg.options.begin(), arg.options.end(), "baseboard") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
				arg.all_arguments) {
			std::string param{"baseboard"};
			ret = node_command_baseboard(gpu_bdf, out);
			is_supported = (ret == 0);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				values_json = nlohmann::ordered_json::parse(out);
				json[string_format("baseboard_%d", arg.devices[i]->get_gpu_index())] = values_json;
				out.clear();
			}
			out.clear();
		}
		json_format.insert(json_format.end(), json);
	}

	if (!is_supported) {
		std::string command{"node"};
		throw SmiToolCommandNotSupportedException(command);
	}

	nlohmann::ordered_json result_json;
	result_json["node"] = json_format;
	result = result_json.dump(4);
	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << '\n';
	}
}

void AmdSmiNodeCommand::node_command_csv()
{
	int ret;
	bool is_supported = false;
	std::string header{};
	std::string formatted_string{};
	std::string values{};
	std::string out{};

	std::vector<std::vector<std::string>> results;
	std::string output_buffer{};
	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		int gpu_id = arg.devices[i]->get_gpu_index();

		if ((std::find(arg.options.begin(), arg.options.end(), "baseboard") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
				arg.all_arguments) {
			ret = node_command_baseboard(gpu_bdf, formatted_string);
			is_supported = (ret == 0);
			std::string param{"baseboard"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				header.append(baseboard_csv_header);
				results.push_back({formatted_string});
				formatted_string.clear();
			}
		}
		if (i == 0) {
			out.append(header);
			out.append("\n");
		}
		csv_recursion(output_buffer, results);
		out.append(output_buffer);
		results.clear();
		output_buffer.clear();
	}

	if (!is_supported) {
		std::string command{"node"};
		throw SmiToolCommandNotSupportedException(command);
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
	out.clear();
	header.clear();

}

void AmdSmiNodeCommand::execute_command()
{
	if ((AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200())
			&& AmdSmiPlatform::getInstance().getInstance().is_host()) {
		if (arg.output == human) {
			node_command_human();
		} else if (arg.output == json) {
			node_command_json();
		} else if (arg.output == csv) {
			node_command_csv();
		}
	} else {
		std::string command{"node"};
		throw SmiToolCommandNotSupportedException(command);
	}
}
