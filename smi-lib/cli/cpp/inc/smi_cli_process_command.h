/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"

class AmdSmiProcessCommand : public AmdSmiCommands
{
public:
	AmdSmiProcessCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	void process_command_json();
	void process_command_human();
	void process_command_csv();
	void process_command_watch();

	int process_command_all_arguments(uint64_t processor, std::string &formatted_string,
									  int &proc_num,
									  int gpu_id);
	int process_command_general(uint64_t processor, std::string &formatted_string, int &proc_num,
								int gpu_id);
	int process_command_engine(uint64_t processor, std::string &formatted_string, int &proc_num,
							   int gpu_id);
};