/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
		} else {
			AmdSmiCommands cmd(parsed_arguments);
			cmd.execute_command();
		}
	} catch (SmiToolException &e) {
		print_errors(e, parsed_arguments.output, parsed_arguments.file_path);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
