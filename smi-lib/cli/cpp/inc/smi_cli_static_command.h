/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
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

#include "smi_cli_commands.h"
#include "smi_cli_parser.h"

class AmdSmiStaticCommand : public AmdSmiCommands
{
public:
	AmdSmiStaticCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	void static_command_json();
	void static_command_human();
	void static_command_csv();

	int static_command_asic(uint64_t processors, std::string &formatted_string);
	int static_command_bus(uint64_t processors,
						   std::string &formatted_string);
	int static_command_vbios(uint64_t processors,
							 std::string &formatted_string);
	int static_command_board_host(uint64_t processors,
								  std::string &formatted_string);
	int static_command_limit(uint64_t processors,
							 std::string &formatted_string);
	int static_command_driver(uint64_t processors,
							  std::string &formatted_string);
	int static_command_ras_host(uint64_t processors,
								std::string &formatted_string);
	int static_command_dfc(uint64_t processors,
						   std::string &formatted_string);
	int static_command_fb_info(uint64_t processors,
							   std::string &formatted_string);
	int static_command_num_vf(uint64_t processors,
							  std::string &formatted_string);
	int static_command_vram(uint64_t processors,
							std::string &formatted_string);
	int static_command_cache(uint64_t processors,
							 std::string &formatted_string);
	int static_command_process_isolation(uint64_t processors,
										 std::string &formatted_string);
	int static_command_partition(uint64_t processors, std::string &formatted_string);
	int static_command_soc_pstate(uint64_t processors,
								  std::string &formatted_string);
};