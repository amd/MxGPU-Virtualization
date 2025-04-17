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
#include "smi_cli_helpers.h"
#include "smi_cli_version_command.h"
#include "smi_cli_api_host.h"
#include "smi_cli_exception.h"

#include "json/json.h"

auto constexpr version_header {"tool_name,tool_version,lib_version"};

int AmdSmiVersionCommand::version_command(std::string &out)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_version_command(arg, out);
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
		ret = version_command(out);
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
		ret = version_command(formatted_string);
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
		ret = version_command(formatted_string);
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