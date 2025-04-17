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


int AmdSmiRasCommand::ras_command_cper(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_cper_entries_command(arg, formatted_string);
	return ret;
}

void AmdSmiRasCommand::ras_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};

	if ((std::find(arg.options.begin(), arg.options.end(), "cper") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end()) ||
			arg.all_arguments) {
		ret = ras_command_cper(formatted_string);
		std::string param{"cper"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out.append(formatted_string);
			formatted_string.clear();
		}
		formatted_string.clear();
	}

	std::cout << out.c_str() << std::endl;
}


void AmdSmiRasCommand::execute_command()
{

	if (AmdSmiPlatform::getInstance().getInstance().is_mi300()
			&& AmdSmiPlatform::getInstance().getInstance().is_host()) {

		if (arg.output == human) {
			ras_command_human();
		} else if (arg.output == json) {
			std::string param{"--json"};
			throw SmiToolParameterNotSupportedException(param);
		} else if(arg.output == csv) {
			std::string param{"--csv"};
			throw SmiToolParameterNotSupportedException(param);
		}
	} else {
		std::string command{"ras"};
		throw SmiToolCommandNotSupportedException(command);
	}
};
