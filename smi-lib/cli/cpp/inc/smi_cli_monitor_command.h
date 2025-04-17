/* * Copyright (C) 2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
