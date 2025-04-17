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
#include <iostream>
#include <functional>

#include "smi_cli_help_command.h"
#include "smi_cli_help_info.h"

std::string AmdSmiHelpCommand::get_list_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_list_help_message(true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_static_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_static_help_message(true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_bad_page_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_bad_page_help_message(true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_firmware_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_firmware_help_message(true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_metric_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_metric_help_message(true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_process_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_process_help_message(true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_profile_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_profile_help_message(true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_version_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_version_help_message(true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_event_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_event_help_message(true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_xgmi_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_xgmi_help_message(true);
	return formatted_string;
}

std::string AmdSmiHelpCommand::get_topology_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_topology_help_message(true);
	return formatted_string;
}

std::string AmdSmiHelpCommand::get_partition_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_partition_help_message(true);
	return formatted_string;
}

std::string AmdSmiHelpCommand::get_reset_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_reset_help_message(true);
	return formatted_string;
}

std::string AmdSmiHelpCommand::get_set_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_set_help_message(true);
	return formatted_string;
}

std::string AmdSmiHelpCommand::get_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_help_message();
	return formatted_string;
}

std::string AmdSmiHelpCommand::get_monitor_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_monitor_help_message(true);
	return formatted_string;
}

std::string AmdSmiHelpCommand::get_ras_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_ras_help_message(true);
	return formatted_string;
}

void AmdSmiHelpCommand::execute_command()
{
	AmdSmiHelpInfo info_helper;
	std::string out{};
	if(arg.options.size() == 0) {
		out = get_help_message(info_helper);
	} else {
		std::map<std::string, std::function<std::string(AmdSmiHelpInfo&)>> command_map{
			{"list", std::bind(&AmdSmiHelpCommand::get_list_help_message, this, std::placeholders::_1)},
			{"discovery", std::bind(&AmdSmiHelpCommand::get_list_help_message, this, std::placeholders::_1)},
			{"static", std::bind(&AmdSmiHelpCommand::get_static_help_message, this, std::placeholders::_1)},
			{"bad-pages", std::bind(&AmdSmiHelpCommand::get_bad_page_help_message, this, std::placeholders::_1)},
			{"firmware", std::bind(&AmdSmiHelpCommand::get_firmware_help_message, this, std::placeholders::_1)},
			{"ucode", std::bind(&AmdSmiHelpCommand::get_firmware_help_message, this, std::placeholders::_1)},
			{"metric", std::bind(&AmdSmiHelpCommand::get_metric_help_message, this, std::placeholders::_1)},
			{"process", std::bind(&AmdSmiHelpCommand::get_process_help_message, this, std::placeholders::_1)},
			{"profile", std::bind(&AmdSmiHelpCommand::get_profile_help_message, this, std::placeholders::_1)},
			{"version", std::bind(&AmdSmiHelpCommand::get_version_help_message, this, std::placeholders::_1)},
			{"event", std::bind(&AmdSmiHelpCommand::get_event_help_message, this, std::placeholders::_1)},
			{"xgmi", std::bind(&AmdSmiHelpCommand::get_xgmi_help_message, this, std::placeholders::_1)},
			{"topology", std::bind(&AmdSmiHelpCommand::get_topology_help_message, this, std::placeholders::_1)},
			{"partition", std::bind(&AmdSmiHelpCommand::get_partition_help_message, this, std::placeholders::_1)},
			{"reset", std::bind(&AmdSmiHelpCommand::get_reset_help_message, this, std::placeholders::_1)},
			{"set", std::bind(&AmdSmiHelpCommand::get_set_help_message, this, std::placeholders::_1)},
			{"monitor", std::bind(&AmdSmiHelpCommand::get_monitor_help_message, this, std::placeholders::_1)},
			{"ras", std::bind(&AmdSmiHelpCommand::get_ras_help_message, this, std::placeholders::_1)},
		};
		out = command_map[arg.options[0]](info_helper);
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
		out.clear();
	} else {
		std::cout << out.c_str() << std::endl;
		out.clear();
	}
};