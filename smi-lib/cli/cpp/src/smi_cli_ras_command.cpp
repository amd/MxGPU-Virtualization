/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include "json/json.h"
#include <map>

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_ras_command.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_api_base.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

auto constexpr ras_policy_header_v_4_0 {
"gpu,policy_major_version,policy_minor_version,"
"policy_dram_non_critical_region_threshold,policy_dram_critical_region_threshold"
};

int AmdSmiRasCommand::ras_command_cper(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_cper_entries_command(arg, formatted_string);
	return ret;
}

int AmdSmiRasCommand::ras_command_afid(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_cper_afid_command(arg, formatted_string);
	return ret;
}

int AmdSmiRasCommand::ras_command_policy(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_policy_command(processor,
			arg, formatted_string);
	return ret;
}

void AmdSmiRasCommand::ras_command_csv()
{
	int ret;
	std::string headers{};
	std::string values{};
	std::string formatted_string{};
	std::string out{};

	if ((std::find(arg.options.begin(), arg.options.end(), "policy") != arg.options.end()) ||
		arg.all_arguments) {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			int gpu_id = arg.devices[i]->get_gpu_index();

			std::string gpu_id_str{string_format("%d",gpu_id)};
			values.append(gpu_id_str);
			ret = ras_command_policy(gpu_bdf, formatted_string);
			std::string param{"policy"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				values.append(formatted_string).append("\n");
				formatted_string.clear();
			}
		}
	}
	out.append(ras_policy_header_v_4_0).append("\n");
	out.append(values);
	values.clear();

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
	out.clear();
}

void AmdSmiRasCommand::ras_command_json()
{
	int ret;
	int i = 0;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	std::string out{};
	std::string result{};
	nlohmann::ordered_json values_json;
	if ((std::find(arg.options.begin(), arg.options.end(), "policy") != arg.options.end()) ||
		arg.all_arguments) {
		for (i = 0; i < arg.devices.size(); i++) {
			json = {};
			json["gpu"] = arg.devices[i]->get_gpu_index();
			nlohmann::ordered_json values_json;
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			ret = ras_command_policy(gpu_bdf, out);
			std::string param{"policy"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				values_json = nlohmann::ordered_json::parse(out);
				out.clear();
				json["policy"] = values_json;
			}
		}
		json_format.insert(json_format.end(), json);
		json.clear();
	}

	result = json_format.dump(4);
	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << '\n';
	}
}

void AmdSmiRasCommand::ras_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};

	if ((std::find(arg.options.begin(), arg.options.end(), "cper") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end())) {
		ret = ras_command_cper(formatted_string);
		std::string param{"cper"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out.append(formatted_string);
			formatted_string.clear();
		}
		formatted_string.clear();
	}
	if ((std::find(arg.options.begin(), arg.options.end(), "afid") != arg.options.end())) {
		ret = ras_command_afid(formatted_string);
		std::string param{"afid"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out.append(formatted_string);
			formatted_string.clear();
		}
		formatted_string.clear();
	}
	if ((std::find(arg.options.begin(), arg.options.end(), "policy") != arg.options.end()) ||
		arg.all_arguments) {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			ret = ras_command_policy(gpu_bdf, formatted_string);
			if (ret == 2 && arg.all_arguments) {
				throw SmiToolRequiredCommandException(std::string("ras"));
			}
			std::string param{"policy"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out.append(formatted_string);
				formatted_string.clear();
			}
			formatted_string.clear();
		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiRasCommand::execute_command()
{

	if ((AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi350())
			&& AmdSmiPlatform::getInstance().is_host()) {
		if (arg.output == human) {
			ras_command_human();
		} else if (arg.output == json) {
			ras_command_json();
		} else if(arg.output == csv) {
			ras_command_csv();
		}
	} else {
		std::string command{"ras"};
		throw SmiToolCommandNotSupportedException(command);
	}
};
