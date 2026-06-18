/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"

class AmdSmiBadPagesCommand : public AmdSmiCommands
{
public:
	AmdSmiBadPagesCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();
	int bad_pages_command(uint64_t processors, std::string &out_string, std::string* gpu_id=nullptr);
};