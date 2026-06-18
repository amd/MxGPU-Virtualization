/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>

#include "smi_cli_helpers.h"

enum class LogLevel {
	Info,
	Warning,
	Error
};

class Logger
{
public:
	/**
	 * @brief Get the static class object
	 *
	 * @return static instance of Logger class
	 */
	static Logger& getInstance();

	Logger(Logger const&) = delete;
	Logger(Logger&&) = delete;
	Logger& operator=(Logger const&) = delete;
	Logger& operator=(Logger&&) = delete;

	void log(LogLevel level, int ret, const char* parent_fun, const char* file_name, int line);

private:
	Logger();
	std::ofstream logFile;
	std::string getLogLevelString(LogLevel level);
	~Logger();
};

#endif  // LOGGER_H
