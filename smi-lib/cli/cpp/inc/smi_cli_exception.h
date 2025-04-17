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
#pragma once

#include <exception>
#include <string>
#include <map>

#include "smi_cli_parser.h"

typedef enum AmdSmiCustomErrors {
	PARAM_NOT_SUPPORTED_ON_PLATFORM = -101,
	COMMAND_NOT_SUPPORTED_AND_ALL_ARGS = -102,
	COMMAND_NOT_SUPPORTED_ON_PLATFORM = -103,
	INVALID_PARAM_VALUE = -104
} amdsmi_tool_custom_errors;

class SmiToolException : public std::exception
{
public:
	virtual int get_error_code() = 0;
	virtual std::string get_message() = 0;
};

class SmiToolInvalidCommandException : public SmiToolException
{
public:
	SmiToolInvalidCommandException(std::string cmd);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-1};
	std::string command;
	std::string message;
};

class SmiToolInvalidParameterException : public SmiToolException
{
public:
	SmiToolInvalidParameterException(std::string param);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-2};
	std::string parameter;
	std::string message;
};

class SmiToolDeviceNotFoundException : public SmiToolException
{
public:
	SmiToolDeviceNotFoundException(std::string param);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-3};
	std::string path;
	std::string message;
};

class SmiToolInvalidFilePathException : public SmiToolException
{
public:
	SmiToolInvalidFilePathException(std::string path);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-4};
	std::string path;
	std::string message;
};

class SmiToolInvalidParameterValueException : public SmiToolException
{
public:
	SmiToolInvalidParameterValueException(std::string parametervalue);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-5};
	std::string message;
	std::string parametervalue;
};

class SmiToolMissingParameterValueException : public SmiToolException
{
public:
	SmiToolMissingParameterValueException(std::string parameter);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-6};
	std::string path;
	std::string message;
	std::string parameter;
};

class SmiToolCommandNotSupportedException : public SmiToolException
{
public:
	SmiToolCommandNotSupportedException(std::string command);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-7};
	std::string message;
	std::string command;
};

class SmiToolParameterNotSupportedException : public SmiToolException
{
public:
	SmiToolParameterNotSupportedException(std::string param);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-8};
	std::string message;
	std::string parameter;
};

class SmiToolRequiredCommandException : public SmiToolException
{
public:
	SmiToolRequiredCommandException(std::string param);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-9};
	std::string message;
	std::string parameter;
};

class SmiToolUnknownErrorException : public SmiToolException
{
public:
	SmiToolUnknownErrorException(std::string param);
	int get_error_code();
	std::string get_message();
private:
	int error_code{-100};
	std::string message;
	std::string parameter;
};

class SmiToolNotEnoughMemException : public SmiToolException
{
public:
	SmiToolNotEnoughMemException();
	int get_error_code();
	std::string get_message();
private:
	int error_code{-101};
	std::string message;
	std::string parameter;
};

class SmiToolSMILIBErrorException : public SmiToolException
{
public:
	std::map<int, std::string> SMI_LIB_ERROR_MESSAGES {
		{ 0, "Sucess" },
		{ 1, "Invalid parameters" },
		{ 2, "Command not supported" },
		{ 3, "Not implemented yet" },
		{ 4, "Fail to load lib" },
		{ 5, "Fail to load symbol"  },
		{ 6, "Error when call libdrm" },
		{ 7, "API call failed"  },
		{ 8, "Timeout in API call" },
		{ 9, "Retry operation" },
		{ 10, "Permission Denied" },
		{ 11, "An interrupt occurred during execution of function" },
		{ 12, "I/O Error" },
		{ 13, "Bad address" },
		{ 14, "Problem accessing a file" },
		{ 15, "Not enough memory" },
		{ 16, "An internal exception was caught"  },
		{ 17, "The provided input is out of allowable or safe range" },
		{ 18, "An error occurred when initializing internal data structures" },
		{ 19, "An internal reference counter exceeded INT32_MAX"  },
		{ 30, "Device busy" },
		{ 31, "Device Not found" },
		{ 32, "Device not initialized" },
		{ 33, "No slot available" },
		{ 40, "No data was found for a given input" },
		{ 41, "Not enough resources were available for the operation" },
		{ 42, "An unexpected amount of data was read" },
		{ 43, "The data read or provided to function is not what was expected" },
		{ -2, "The internal library error did not map to a status code" },
		{ -1, "An unknown error occurred" }
	};
	SmiToolSMILIBErrorException(int error_code);
	int get_error_code();
	std::string get_message();
private:
	int error_code;
	std::string message;
	int smilib_error_code;
};

void print_errors(SmiToolException &e, OutputFormat format, std::string file_path = "");

int handle_exceptions(int ret, std::string param, Arguments arg);