/* * Copyright (C) 2023-2025 Advanced Micro Devices. All rights reserved.
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

std::string AmdSmiHelpCommand::get_list_help_message(AmdSmiHelpInfo &info_helper, Arguments arg)
{
	std::string formatted_string = info_helper.get_list_help_message(arg, true);
	return formatted_string;
}
std::string AmdSmiHelpCommand::get_static_help_message(AmdSmiHelpInfo &info_helper, Arguments arg)
{
	std::string formatted_string = info_helper.get_static_help_message(arg, true);
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
std::string AmdSmiHelpCommand::get_metric_help_message(AmdSmiHelpInfo &info_helper, Arguments arg)
{
	std::string formatted_string = info_helper.get_metric_help_message(arg, true);
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

std::string AmdSmiHelpCommand::get_topology_help_message(AmdSmiHelpInfo &info_helper, Arguments arg)
{
	std::string formatted_string = info_helper.get_topology_help_message(arg, true);
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

std::string AmdSmiHelpCommand::get_node_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_node_help_message(true);
	return formatted_string;
}

void AmdSmiHelpCommand::execute_command()
{
	AmdSmiHelpInfo info_helper(arg);
	std::string out{};
	if(arg.options.size() == 0) {
		out = get_help_message(info_helper);
	} else {
		std::map<std::string, std::function<std::string(AmdSmiHelpInfo&, Arguments)>> command_map{
			{"list", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_list_help_message(info, arg);
				}
			},
			{"discovery", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_list_help_message(info, arg);
				}
			},
			{"static", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_static_help_message(info, arg);
				}
			},
			{"bad-pages", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_bad_page_help_message(info);
				}
			},
			{"firmware", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_firmware_help_message(info);
				}
			},
			{"ucode", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_firmware_help_message(info);
				}
			},
			{"metric", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_metric_help_message(info, arg);
				}
			},
			{"process", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_process_help_message(info);
				}
			},
			{"profile", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_profile_help_message(info);
				}
			},
			{"version", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_version_help_message(info);
				}
			},
			{"event", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_event_help_message(info);
				}
			},
			{"xgmi", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_xgmi_help_message(info);
				}
			},
			{"topology", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_topology_help_message(info, arg);
				}
			},
			{"partition", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_partition_help_message(info);
				}
			},
			{"reset", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_reset_help_message(info);
				}
			},
			{"set", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_set_help_message(info);
				}
			},
			{"monitor", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_monitor_help_message(info);
				}
			},
			{"ras", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_ras_help_message(info);
				}
			},
			{"node", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_node_help_message(info);
				}
			},
		};
		out = command_map[arg.options[0]](info_helper, arg);
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
		out.clear();
	} else {
		std::cout << out.c_str() << std::endl;
		out.clear();
	}
};
