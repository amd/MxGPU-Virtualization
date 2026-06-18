/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"


class AmdSmiMonitorCommand : public AmdSmiCommands
{
public:
	AmdSmiMonitorCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	void monitor_command_json();
	void monitor_command_human();
	void monitor_command_csv();
	void monitor_command_watch();

	int monitor_command_power_usage(uint64_t processor,
									std::string &formatted_string);
	int monitor_command_temperature(uint64_t processor,
									std::string &formatted_string);
	int monitor_command_gfx(uint64_t processor,
							std::string &formatted_string);
	int monitor_command_mem(uint64_t processor,
							std::string &formatted_string);
	int monitor_command_encoder(uint64_t processor,
								std::string &formatted_string);
	int monitor_command_decoder(uint64_t processor,
								std::string &formatted_string);
	int monitor_command_ecc(uint64_t processor,
							std::string &formatted_string);
	int monitor_command_vram_usage(uint64_t processor,
								   std::string &formatted_string);
	int monitor_command_pcie(uint64_t processor,
							 std::string &formatted_string);
	int monitor_command_process(uint64_t processor,
								std::string &formatted_string, int &proc_num, int gpu_id);
};
