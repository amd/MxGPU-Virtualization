/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
