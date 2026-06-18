/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"

class AmdSmiCommands
{
protected:
	Arguments arg;
	Logger &log_err = Logger::getInstance();

	unsigned int gpu_count;

public:
	AmdSmiCommands(Arguments args);
	void execute_command();
};
