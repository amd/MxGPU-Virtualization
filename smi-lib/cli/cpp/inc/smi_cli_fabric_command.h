/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "smi_cli_commands.h"
#include "smi_cli_parser.h"

class AmdSmiFabricCommand : public AmdSmiCommands
{
private:
public:
	AmdSmiFabricCommand(Arguments args);
	void execute_command();

	void fabric_command_human();
	void fabric_command_json();

	int fabric_command_telemetry(uint64_t processor, std::string &formatted_string);
	int fabric_command_topology(uint64_t processor, std::string &formatted_string);
};
