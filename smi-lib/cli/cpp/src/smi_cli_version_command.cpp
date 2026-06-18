/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_cli_helpers.h"
#include "smi_cli_version_command.h"
#include "smi_cli_api_host.h"
#include "smi_cli_exception.h"

#include "json/json.h"

auto constexpr version_header {"tool_name,tool_version,lib_version,driver_version"};

int AmdSmiVersionCommand::version_command(uint64_t processor_bdf, Arguments arg, std::string &out)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_version_command(processor_bdf, arg, out);
	return ret;
}

void AmdSmiVersionCommand::version_command_json()
{
	int ret;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	std::string out{};
	std::string result{};
	nlohmann::ordered_json values_json;

	if ((std::find(arg.options.begin(), arg.options.end(), "version") !=
			arg.options.end() ||
			std::find(arg.options.begin(), arg.options.end(), "v") !=
			arg.options.end()) ||
			arg.all_arguments) {
		std::string param{"version"};
		uint64_t processor_bdf = arg.devices[0]->get_bdf();
		ret = version_command(processor_bdf, arg, out);
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			values_json = nlohmann::ordered_json::parse(out);
			out.clear();
			json["version"] = values_json;
		}
	}

	json_format.insert(json_format.end(), json);
	json.clear();

	result = json_format.dump(4);
	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << '\n';
	}
}

void AmdSmiVersionCommand::version_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};
	if ((std::find(arg.options.begin(), arg.options.end(), "version") !=
			arg.options.end() ||
			std::find(arg.options.begin(), arg.options.end(), "v") !=
			arg.options.end()) ||
			arg.all_arguments) {
		uint64_t processor_bdf = arg.devices[0]->get_bdf();
		ret = version_command(processor_bdf, arg, formatted_string);
		std::string param{"version"};
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

void AmdSmiVersionCommand::version_command_csv()
{
	int ret;
	std::string headers{};
	std::string values{};
	std::string formatted_string{};
	std::string out{};
	std::string gpu_id{};
	std::string vf_id{};

	if ((std::find(arg.options.begin(), arg.options.end(), "version") !=
			arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "v") !=
			 arg.options.end()) ||
			arg.all_arguments) {
		uint64_t processor_bdf = arg.devices[0]->get_bdf();
		ret = version_command(processor_bdf, arg, formatted_string);
		std::string param{"version"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			headers.append(version_header);
			values.append(formatted_string);
			formatted_string.clear();
		}
	}
	out.append(headers).append("\n");
	out.append(values);
	values.clear();

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
	out.clear();
}

void AmdSmiVersionCommand::execute_command()
{
	if (arg.output == json) {
		version_command_json();
	}
	if (arg.output == csv) {
		version_command_csv();
	}
	if (arg.output == human) {
		version_command_human();
	}
};
