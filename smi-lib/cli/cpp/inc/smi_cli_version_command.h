/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"

class AmdSmiVersionCommand : public AmdSmiCommands
{
public:
	AmdSmiVersionCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	void version_command_json();
	void version_command_human();
	void version_command_csv();

	int version_command(uint64_t processor_bdf, Arguments arg, std::string &out_string);
};
