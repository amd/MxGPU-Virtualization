/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"

class AmdSmiProfileCommand : public AmdSmiCommands
{
public:
	AmdSmiProfileCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	void profile_command_json();
	void profile_command_human();
	void profile_command_csv();

	int get_profile_info(uint64_t processors, int gpu_index, std::string &formatted_string);
};