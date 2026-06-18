/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <iostream>
#include "smi_cli_commands.h"
#include "smi_cli_parser.h"

class AmdSmiRasCommand : public AmdSmiCommands
{
public:
	AmdSmiRasCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();
	void ras_command_human();
	void ras_command_json();
	void ras_command_csv();

	int ras_command_cper(std::string &formatted_string);
	int ras_command_afid(std::string &formatted_string);
	int ras_command_policy(uint64_t processors, std::string &formatted_string);

};
