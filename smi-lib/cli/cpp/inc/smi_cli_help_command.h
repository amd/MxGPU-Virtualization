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
#include "smi_cli_help_info.h"

class AmdSmiHelpCommand : public AmdSmiCommands
{
public:
	AmdSmiHelpCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();
	std::string get_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_list_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_static_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_bad_page_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_firmware_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_metric_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_process_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_profile_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_version_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_event_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_xgmi_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_topology_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_partition_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_reset_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_set_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_monitor_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_ras_help_message(AmdSmiHelpInfo &info_helper);
};
