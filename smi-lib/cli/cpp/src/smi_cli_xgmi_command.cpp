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

#include "smi_cli_platform.h"
#include "smi_cli_exception.h"
#include "smi_cli_xgmi_command.h"
#include "smi_cli_helpers.h"
#include "smi_cli_api_base.h"
#include "smi_cli_templates.h"

int AmdSmiXgmiCommand::xgmi_command_caps(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_caps_xgmi_command(arg,
			  formatted_string);
	return ret;
}

int AmdSmiXgmiCommand::xgmi_command_fb_sharing(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_fb_sharing_xgmi_command(arg,
			  formatted_string);
	return ret;
}

int AmdSmiXgmiCommand::set_xgmi_command_fb_sharing(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_fb_sharing_xgmi_command(arg,
			  formatted_string);
	return ret;
}

int AmdSmiXgmiCommand::xgmi_command_all(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_all_xgmi_command(arg,
			  formatted_string);
	return ret;
}

int AmdSmiXgmiCommand::metric_command_xgmi(std::string &formatted_string)
{
	int ret = PARAM_NOT_SUPPORTED_ON_PLATFORM;
	if (AmdSmiPlatform::getInstance().is_host() && (AmdSmiPlatform::getInstance().is_mi300()
			|| AmdSmiPlatform::getInstance().is_mi200())) {
		ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_xgmi_metric_command(arg, formatted_string);
	}
	return ret;
}

void AmdSmiXgmiCommand::xgmi_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};

	if ((std::find(arg.options.begin(), arg.options.end(), "set") != arg.options.end())) {
		ret = set_xgmi_command_fb_sharing(formatted_string);
		std::string param{"set xgmi"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out.append(formatted_string);
			formatted_string.clear();
		} else {
			formatted_string.clear();
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "caps") !=
			arg.options.end()) ||
			arg.all_arguments) {
		ret = xgmi_command_caps(formatted_string);
		std::string param{"--caps"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out.append(formatted_string);
			formatted_string.clear();
		} else {
			formatted_string.clear();
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "fb-sharing") != arg.options.end())
			&& (std::find(arg.options.begin(), arg.options.end(), "set") == arg.options.end()) ||
			arg.all_arguments) {
		ret = xgmi_command_fb_sharing(formatted_string);
		std::string param{"--fb-sharing"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out.append(formatted_string);
			formatted_string.clear();
		} else {
			formatted_string.clear();
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "metric") != arg.options.end()) ||
			arg.all_arguments) {
		ret = metric_command_xgmi(formatted_string);
		std::string param{"metric"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += formatted_string;
			formatted_string.clear();
		}
		formatted_string.clear();
	}


	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiXgmiCommand::xgmi_command_json()
{
	if ((std::find(arg.options.begin(), arg.options.end(), "set") != arg.options.end())) {
		std::string param{"--json"};
		throw SmiToolParameterNotSupportedException(param);
	}
	int ret;
	std::string out{};
	std::string param{"xgmi"};

	ret = xgmi_command_all(out);
	handle_exceptions(ret, param, arg);

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiXgmiCommand::execute_command()
{
	unsigned int gpu_count;
	AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_gpu_count(gpu_count);
	if ((AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200())
			&& AmdSmiPlatform::getInstance().getInstance().is_host()) {
		if (gpu_count > 1) {
			if (arg.output == human) {
				xgmi_command_human();
			} else if (arg.output == json) {
				std::string param{"--json"};
				xgmi_command_json();
			} else if (arg.output == csv) {
				std::string param{"--csv"};
				throw SmiToolParameterNotSupportedException(param);
			}
		} else {
			std::string command{"xgmi"};
			throw SmiToolCommandNotSupportedException(command);
		}
	} else {
		std::string command{"xgmi"};
		throw SmiToolCommandNotSupportedException(command);
	}
}
