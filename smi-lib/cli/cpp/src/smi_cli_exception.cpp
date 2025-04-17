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

#include "smi_cli_exception.h"
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"

#include "json/json.h"

SmiToolInvalidCommandException::SmiToolInvalidCommandException(std::string cmd) : command{cmd}
{
	message = string_format("Command '%s' is invalid. Run 'help' for more info.", command.c_str());
}

int SmiToolInvalidCommandException::get_error_code()
{
	return error_code;
}

std::string SmiToolInvalidCommandException::get_message()
{
	return message;
}

SmiToolInvalidParameterException::SmiToolInvalidParameterException(std::string param) : parameter{param}
{
	message = string_format("Parameter '%s' is invalid. Run 'help' for more info.", parameter.c_str());
}

int SmiToolInvalidParameterException::get_error_code()
{
	return error_code;
}
std::string SmiToolInvalidParameterException::get_message()
{
	return message;
}

int SmiToolDeviceNotFoundException::get_error_code()
{
	return error_code;
}
std::string SmiToolDeviceNotFoundException::get_message()
{
	return message;
}
SmiToolDeviceNotFoundException::SmiToolDeviceNotFoundException(std::string param)
{
	message = string_format("Can not find a device with the corresponding identifier: \'%s\'",
							param.c_str());
}


SmiToolInvalidFilePathException::SmiToolInvalidFilePathException(std::string path) : path{path}
{
	message = string_format("Path '%s' cannot be found.", path.c_str());
}

int SmiToolInvalidFilePathException::get_error_code()
{
	return error_code;
}
std::string SmiToolInvalidFilePathException::get_message()
{
	return message;
}

SmiToolInvalidParameterValueException::SmiToolInvalidParameterValueException(
	std::string parametervalue) : parametervalue{parametervalue}
{
	message = string_format("Value '%s' is not of valid type or format. Run 'help' for more info.",
							parametervalue.c_str());
}
int SmiToolInvalidParameterValueException::get_error_code()
{
	return error_code;
}
std::string SmiToolInvalidParameterValueException::get_message()
{
	return message;
}

SmiToolMissingParameterValueException::SmiToolMissingParameterValueException(
	std::string parameter) : parameter{parameter}
{
	message = string_format("Parameter '%s' requires a value. Run 'help' for more info.",
							parameter.c_str());
}
int SmiToolMissingParameterValueException::get_error_code()
{
	return error_code;
}
std::string SmiToolMissingParameterValueException::get_message()
{
	return message;
}

SmiToolCommandNotSupportedException::SmiToolCommandNotSupportedException(
	std::string command) : command{command}
{
	message = string_format("Command '%s' is not supported on the system. Run 'help' for more info.",
							command.c_str());
}
int SmiToolCommandNotSupportedException::get_error_code()
{
	return error_code;
}
std::string SmiToolCommandNotSupportedException::get_message()
{
	return message;
}


SmiToolParameterNotSupportedException::SmiToolParameterNotSupportedException(
	std::string param) : parameter{param}
{
	message = string_format("Parameter '%s' is not supported on the system. Run 'help' for more info.",
							parameter.c_str());
}
int SmiToolParameterNotSupportedException::get_error_code()
{
	return error_code;
}
std::string SmiToolParameterNotSupportedException::get_message()
{
	return message;
}

SmiToolRequiredCommandException::SmiToolRequiredCommandException(std::string param) : parameter{param}
{
	message = string_format("Command '%s' requires a target argument. Run '--help' for more info.",
							parameter.c_str());
}
int SmiToolRequiredCommandException::get_error_code()
{
	return error_code;
}
std::string SmiToolRequiredCommandException::get_message()
{
	return message;
}

SmiToolNotEnoughMemException::SmiToolNotEnoughMemException()
{
	message = "Not enough memory.";
}
int SmiToolNotEnoughMemException::get_error_code()
{
	return error_code;
}
std::string SmiToolNotEnoughMemException::get_message()
{
	return message;
}

SmiToolUnknownErrorException::SmiToolUnknownErrorException(std::string param) : parameter{param}
{
	message = string_format("Command '%s' requires a target argument. Run '--help' for more info.",
							parameter.c_str());
}
int SmiToolUnknownErrorException::get_error_code()
{
	return error_code;
}
std::string SmiToolUnknownErrorException::get_message()
{
	return message;
}

SmiToolSMILIBErrorException::SmiToolSMILIBErrorException(int error_code) : smilib_error_code{error_code}
{
	error_code = -1000 - smilib_error_code;
	message = string_format("SMI-LIB has returned error %d - %s.", error_code,
							SMI_LIB_ERROR_MESSAGES[smilib_error_code].c_str());
}
int SmiToolSMILIBErrorException::get_error_code()
{
	error_code = -1000 - smilib_error_code;
	return error_code;
}
std::string SmiToolSMILIBErrorException::get_message()
{
	return message;
}

void print_errors(SmiToolException &e, OutputFormat format, std::string file_path)
{
	if (e.get_error_code() == -4) {
		file_path = "";
	}
	std::string out{};
	if (format == json) {
		nlohmann::ordered_json json;
		json["error_message"] = e.get_message();
		json["error_code"] = e.get_error_code();
		out = json.dump(4);
	} else if (format == csv) {
		std::string error_code_string{string_format("%d", e.get_error_code())};
		out.append("error_message,error_code").append("\n").append(e.get_message()).append(",").append(
			   error_code_string).append("\n");
	} else {
		std::string error_code_string{string_format("%d", e.get_error_code())};
		out.append(e.get_message()).append(" Error code: ").append(error_code_string);
	}

	if (file_path != "") {
		write_to_file(file_path, out);
	} else {
		std::cerr << out.c_str() << std::endl;
	}
}

int handle_exceptions(int ret, std::string param, Arguments arg)
{
	if (ret == PARAM_NOT_SUPPORTED_ON_PLATFORM || ret == 2) {
		if (arg.all_arguments) {
			return PARAM_NOT_SUPPORTED_ON_PLATFORM;
		} else {
			throw SmiToolParameterNotSupportedException(param);
		}
	} else if (ret == COMMAND_NOT_SUPPORTED_ON_PLATFORM) {
		throw SmiToolCommandNotSupportedException(param);
	} else if (ret == INVALID_PARAM_VALUE) {
		throw SmiToolInvalidParameterValueException(param);
	} else if (ret != 0) {
		if (arg.command != "bad-pages" && arg.options.size() != 1) {
			return 0;
		} else {
			throw SmiToolSMILIBErrorException(ret);
		}
	} else {
		return 0;
	}
}