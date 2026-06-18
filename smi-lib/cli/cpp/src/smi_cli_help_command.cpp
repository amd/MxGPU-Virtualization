/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

std::string AmdSmiHelpCommand::get_fabric_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_fabric_help_message(true);
	return formatted_string;
}

std::string AmdSmiHelpCommand::get_confidential_compute_help_message(AmdSmiHelpInfo &info_helper)
{
	std::string formatted_string = info_helper.get_confidential_compute_help_message(true);
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
			{"confidential-compute", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_confidential_compute_help_message(info);
				}
			},
			{"node", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_node_help_message(info);
				}
			},
			{"fabric", [this](AmdSmiHelpInfo& info, Arguments arg)
				{
					return get_fabric_help_message(info);
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
