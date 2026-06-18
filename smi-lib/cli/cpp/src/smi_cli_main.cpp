/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_cli_commands.h"
#include "smi_cli_parser.h"
#include "smi_cli_static_command.h"
#include "smi_cli_list_command.h"
#include "smi_cli_help_command.h"
#include "smi_cli_version_command.h"
#include "smi_cli_bad_pages_command.h"
#include "smi_cli_firmware_command.h"
#include "smi_cli_metric_command.h"
#include "smi_cli_monitor_command.h"
#include "smi_cli_topology_command.h"
#include "smi_cli_xgmi_command.h"
#include "smi_cli_event_command.h"
#include "smi_cli_profile_command.h"
#include "smi_cli_process_command.h"
#include "smi_cli_reset_command.h"
#include "smi_cli_set_command.h"
#include "smi_cli_exception.h"
#include "smi_cli_partition_command.h"
#include "smi_cli_ras_command.h"
#include "smi_cli_node_command.h"
#include "smi_cli_fabric_command.h"
#include "smi_cli_cc_command.h"
#include "smi_cli_default_command.h"

int main(int argc, char **argv)
{
	Arguments parsed_arguments;

	try {
		AmdSmiParser parser;

		parser.parse_arg(argc, argv, parsed_arguments);

		if (parsed_arguments.command == "help") {
			AmdSmiHelpCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "version") {
			AmdSmiVersionCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "list" ||
				   parsed_arguments.command == "discovery") {
			AmdSmiListCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "static") {
			AmdSmiStaticCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "bad-pages") {
			AmdSmiBadPagesCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "ucode" ||
				   parsed_arguments.command == "firmware") {
			AmdSmiFirmwareCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "metric") {
			AmdSmiMetricCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "topology") {
			AmdSmiTopologyCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "xgmi") {
			AmdSmiXgmiCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "event") {
			AmdSmiEventCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "profile") {
			AmdSmiProfileCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "process") {
			AmdSmiProcessCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "reset") {
			AmdSmiResetCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "set") {
			AmdSmiSetCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "monitor") {
			AmdSmiMonitorCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "partition") {
			AmdSmiPartitionCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "ras") {
			AmdSmiRasCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "node") {
			AmdSmiNodeCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "fabric") {
			AmdSmiFabricCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else if (parsed_arguments.command == "confidential-compute") {
			AmdSmiCCCommand cmd(parsed_arguments);
			cmd.execute_command();
		} else {
			AmdSmiDefaultCommand cmd(parsed_arguments);
			cmd.execute_command();
		}
	} catch (SmiToolException &e) {
		print_errors(e, parsed_arguments.output, parsed_arguments.file_path);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
