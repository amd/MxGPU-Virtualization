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

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_exception.h"
#include "smi_cli_topology_command.h"
#include "smi_cli_platform.h"
#include "smi_cli_templates.h"
#include "smi_cli_api_base.h"

AmdSmiTopologyCommand::AmdSmiTopologyCommand(Arguments args) : AmdSmiCommands(args)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().initTopology(arg.devices, bdf_vector);
}

int AmdSmiTopologyCommand::topology_command_weight(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_weight_topology_command(arg,
			  bdf_vector, formatted_string);
	return ret;
}
int AmdSmiTopologyCommand::topology_command_hops(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_hops_topology_command(arg,
			  bdf_vector, formatted_string);
	return ret;

}
int AmdSmiTopologyCommand::topology_command_fb_sharing(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_fb_sharing_topology_command(
				  arg, bdf_vector, formatted_string);
	return ret;
}
int AmdSmiTopologyCommand::topology_command_link_type(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_link_type_topology_command(arg,
			  bdf_vector, formatted_string);
	return ret;
}
int AmdSmiTopologyCommand::topology_command_link_status(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_link_status_topology_command(
				  arg, bdf_vector, formatted_string);
	return ret;
}

int AmdSmiTopologyCommand::topology_command_all_status(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_all_topology_command(
				  arg, bdf_vector, formatted_string);
	return ret;
}

void AmdSmiTopologyCommand::topology_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};

	if ((std::find(arg.options.begin(), arg.options.end(), "weight") !=
			arg.options.end()) ||
			arg.all_arguments) {
		ret = topology_command_weight(formatted_string);
		std::string param{"weight"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += formatted_string;
		}
		formatted_string.clear();
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "hops") !=
			arg.options.end()) ||
			arg.all_arguments) {
		ret = topology_command_hops(formatted_string);
		std::string param{"hops"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += formatted_string;
		}
		formatted_string.clear();
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "fb-sharing") !=
			arg.options.end()) ||
			arg.all_arguments) {
		ret = topology_command_fb_sharing(formatted_string);
		std::string param{"fb-sharing"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += formatted_string;
		}
		formatted_string.clear();
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "link-type") !=
			arg.options.end()) ||
			arg.all_arguments) {
		ret = topology_command_link_type(formatted_string);
		std::string param{"link-type"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += formatted_string;
		}
		formatted_string.clear();
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "link-status") !=
			arg.options.end()) ||
			arg.all_arguments) {
		ret = topology_command_link_status(formatted_string);
		std::string param{"link-status"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += formatted_string;
		}
		formatted_string.clear();
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiTopologyCommand::topology_command_json()
{

	int ret;
	std::string out{};
	std::string param{"topology"};

	ret = topology_command_all_status(out);
	handle_exceptions(ret, param, arg);

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}

}


void AmdSmiTopologyCommand::execute_command()
{
	unsigned int gpu_count;
	AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_gpu_count(gpu_count);
	if ((AmdSmiPlatform::getInstance().getInstance().is_mi300()
			|| AmdSmiPlatform::getInstance().is_mi200())
			&& AmdSmiPlatform::getInstance().getInstance().is_host()) {
		if (gpu_count > 1) {
			if (arg.output == human) {
				topology_command_human();
			} else if (arg.output == json) {
				std::string param{"--json"};
				topology_command_json();
			} else if (arg.output == csv) {
				std::string param{"--csv"};
				throw SmiToolParameterNotSupportedException(param);
			}
		} else {
			std::string command{"topology"};
			throw SmiToolCommandNotSupportedException(command);
		}
	} else {
		std::string command{"topology"};
		throw SmiToolCommandNotSupportedException(command);
	}
}
