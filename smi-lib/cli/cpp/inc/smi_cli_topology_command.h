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

class AmdSmiTopologyCommand : public AmdSmiCommands
{
private:
	std::vector<std::string> bdf_vector;
public:
	AmdSmiTopologyCommand(Arguments args);
	void execute_command();

	void topology_command_human();
	void topology_command_json();

	int topology_command_weight(std::string &formatted_string);
	int topology_command_hops(std::string &formatted_string);
	int topology_command_fb_sharing(std::string &formatted_string);
	int topology_command_link_type(std::string &formatted_string);
	int topology_command_link_status(std::string &formatted_string);
	int topology_command_all_status(std::string &formatted_string);
};