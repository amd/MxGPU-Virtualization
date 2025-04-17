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
#include "smi_cli_profile_command.h"
#include "smi_cli_platform.h"
#include "smi_cli_templates.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"

auto constexpr profile_csv_header {
	"gpu, vf_count, c_available,c_max,c_min,c_optimal,c_total,d_available,d_max,d_min,"
	"d_optimal,d_total,e_available,e_max,e_min,e_optimal,e_total,m_available,m_m"
	"ax,m_min,m_optimal,m_total,current_profile"
};

int AmdSmiProfileCommand::get_profile_info(uint64_t processor,
		int gpu_index,
		std::string &out_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_profile_command(processor, arg,
			  gpu_index, out_string);
	return ret;
}

void AmdSmiProfileCommand::profile_command_json()
{
	int ret;
	unsigned int i;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json profile_info_json;
	nlohmann::ordered_json json;
	std::string out{};
	std::string result{};

	for (i = 0; i < arg.devices.size(); i++) {
		json = {};
		json["gpu"] = arg.devices[i]->get_gpu_index();
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		profile_info_json = {};

		ret = get_profile_info(gpu_bdf, arg.devices[i]->get_gpu_index(), out);
		std::string param{"profile"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			profile_info_json = nlohmann::ordered_json::parse(out);
			json["profile_info"] = profile_info_json;

			json_format.insert(json_format.end(), json);
		}
	}
	result = json_format.dump(4);

	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << '\n';
	}

}

void AmdSmiProfileCommand::profile_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		ret = get_profile_info(gpu_bdf, arg.devices[i]->get_gpu_index(),
							   formatted_string);
		std::string param{"profile"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += formatted_string;
			formatted_string.clear();
		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiProfileCommand::profile_command_csv()
{
	int ret;
	std::string formatted_string{};
	std::string out{};
	out.append(profile_csv_header).append("\n");
	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();

		ret = get_profile_info(gpu_bdf, arg.devices[i]->get_gpu_index(),
							   formatted_string);
		std::string param{"profile"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out.append(formatted_string);
			formatted_string.clear();
		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiProfileCommand::execute_command()
{
	if(AmdSmiPlatform::getInstance().is_linux()) {
		std::string command("profile");
		throw SmiToolCommandNotSupportedException(command);
	}

	if(AmdSmiPlatform::getInstance().is_guest() || AmdSmiPlatform::getInstance().is_baremetal()) {
		std::string command("profile");
		throw SmiToolCommandNotSupportedException(command);
	}

	if (arg.output == OutputFormat::json) {
		profile_command_json();
	}
	if (arg.output == OutputFormat::csv) {
		profile_command_csv();
	}
	if (arg.output == OutputFormat::human) {
		profile_command_human();
	}
};