/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
	std::string get_list_help_message(AmdSmiHelpInfo &info_helper, Arguments arg);
	std::string get_static_help_message(AmdSmiHelpInfo &info_helper, Arguments arg);
	std::string get_bad_page_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_firmware_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_metric_help_message(AmdSmiHelpInfo &info_helper, Arguments arg);
	std::string get_process_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_profile_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_version_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_event_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_xgmi_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_topology_help_message(AmdSmiHelpInfo &info_helper, Arguments arg);
	std::string get_partition_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_reset_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_set_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_monitor_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_ras_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_node_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_fabric_help_message(AmdSmiHelpInfo &info_helper);
	std::string get_confidential_compute_help_message(AmdSmiHelpInfo &info_helper);
};
