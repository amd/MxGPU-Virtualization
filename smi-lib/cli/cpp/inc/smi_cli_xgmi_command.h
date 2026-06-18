/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_commands.h"
#include "smi_cli_parser.h"

class AmdSmiXgmiCommand : public AmdSmiCommands
{
public:
	AmdSmiXgmiCommand(Arguments args) : AmdSmiCommands(args) {};

	void execute_command();
	void xgmi_command_human();
	void xgmi_command_json();
	int xgmi_command_caps(std::string &formatted_string);
	int xgmi_command_fb_sharing(std::string &formatted_string);
	int set_xgmi_command_fb_sharing(std::string &formatted_string);
	int metric_command_xgmi(std::string &formatted_string);
	int source_gpu_status_command_xgmi(std::string &formatted_string);
	int xgmi_link_status_command(std::string &formatted_string);
	int xgmi_command_all(std::string &formatted_string);

};