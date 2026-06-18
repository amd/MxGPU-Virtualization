/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_commands.h"
#include "smi_cli_parser.h"

class AmdSmiNodeCommand : public AmdSmiCommands
{
public:
	AmdSmiNodeCommand(Arguments args) : AmdSmiCommands(args) {};

	void execute_command();

	void node_command_human();
	void node_command_json();
	void node_command_csv();

	int node_command_baseboard(uint64_t processor,
							 std::string &formatted_string);
	int node_command_npm(uint64_t node, std::string &formatted_string);

};
