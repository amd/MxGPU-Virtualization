/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <cstdarg>

#include "smi_cli_logger_err.h"

Logger& Logger::getInstance()
{
	static Logger instance;
	return instance;
}

Logger::Logger()
{
	bool isFile = false;
	const std::string file_name = "default_log.txt";
	if(isFile)
		logFile.open(file_name, std::ios::app);
}

std::string Logger::getLogLevelString(LogLevel level)
{
	switch (level) {
	case LogLevel::Info:
		return "INFO";
	case LogLevel::Warning:
		return "WARNING";
	case LogLevel::Error:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

void Logger::log(LogLevel level, int ret, const char* parent_fun, const char* file_name, int line)
{
#if ENABLE_LOGGER
	std::string typeOfMsg = getLogLevelString(level);

	if (logFile.is_open()) {
		logFile << "[" << typeOfMsg << "] " << parent_fun << "(" <<
				getFileName(file_name) << ", " << line << ") -> " <<
				amdsmi_get_error_message(ret) << ": " << ret << std::endl;
		logFile.flush();
	} else {
		std::cout << "[" << typeOfMsg << "] " << parent_fun << "(" <<
				  getFileName(file_name) << ", " << line << ") -> " <<
				  amdsmi_get_error_message(ret) << ": " << ret << std::endl;
	}
#endif
}

Logger::~Logger()
{
	if (logFile.is_open()) {
		logFile.close();
	}
}