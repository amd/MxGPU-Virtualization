/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
			|| AmdSmiPlatform::getInstance().is_mi200() || AmdSmiPlatform::getInstance().is_mi350())) {
		ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_xgmi_metric_command(arg, formatted_string);
	}
	return ret;
}

int AmdSmiXgmiCommand::source_gpu_status_command_xgmi(std::string &formatted_string)
{
	int ret = PARAM_NOT_SUPPORTED_ON_PLATFORM;
	if (AmdSmiPlatform::getInstance().is_host() && (AmdSmiPlatform::getInstance().is_mi300()
			|| AmdSmiPlatform::getInstance().is_mi200() || AmdSmiPlatform::getInstance().is_mi350())) {
		ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_source_gpu_xgmi_status_command(arg, formatted_string);
	}
	return ret;
}

int AmdSmiXgmiCommand::xgmi_link_status_command(std::string &formatted_string)
{
	int ret = PARAM_NOT_SUPPORTED_ON_PLATFORM;
	if (AmdSmiPlatform::getInstance().is_host() && (AmdSmiPlatform::getInstance().is_mi300()
			|| AmdSmiPlatform::getInstance().is_mi200() || AmdSmiPlatform::getInstance().is_mi350())) {
		ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_xgmi_link_status_command(arg, formatted_string);
	}
	return ret;
}


void AmdSmiXgmiCommand::xgmi_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};

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

	if ((std::find(arg.options.begin(), arg.options.end(), "source-status") != arg.options.end()) ||
			arg.all_arguments) {
		ret = source_gpu_status_command_xgmi(formatted_string);
		std::string param{"source-status"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += formatted_string;
			formatted_string.clear();
		}
		formatted_string.clear();
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "link-status") != arg.options.end()) ||
			arg.all_arguments) {
		ret = xgmi_link_status_command(formatted_string);
		std::string param{"link-status"};
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
	AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));
	if ((AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200() || AmdSmiPlatform::getInstance().is_mi350())
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
