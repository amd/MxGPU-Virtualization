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

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"


class AmdSmiMetricCommand : public AmdSmiCommands
{
public:
	AmdSmiMetricCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	void metric_command_json();
	void metric_command_human();
	void metric_command_csv();
	void metric_command_watch();

	int metric_command_usage(uint64_t processor,
							 std::string &formatted_string);
	int metric_command_power(uint64_t processor,
							 std::string &formatted_string);
	int metric_command_clock(uint64_t processor,
							 std::string &formatted_string);
	int metric_command_temperature(uint64_t processor,
								   std::string &formatted_string);
	int metric_command_ecc(uint64_t processor,
						   std::string &formatted_string);
	int metric_command_ecc_block(uint64_t processor,
								 std::string &formatted_string);
	int metric_command_pcie(uint64_t processor,
							std::string &formatted_string);
	int metric_vf_command_schedule(std::string vf_handle,
								   std::string &formatted_string);
	int metric_vf_command_guard(std::string vf_handle,
								std::string &formatted_string);
	int metric_vf_command_guest_data(std::string vf_handle,
									 std::string &formatted_string);
	int metric_command_fb_usage(uint64_t processor,
								std::string &formatted_string);
	int metric_command_energy(uint64_t processor,
							  std::string &formatted_string);

private:
	bool is_vf_schedule = false;
	bool is_vf_guard_info = false;
	bool is_vf_guest_data = false;

};
