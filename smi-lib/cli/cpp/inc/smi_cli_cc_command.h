/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */
 #pragma once

 #include "smi_cli_commands.h"
 #include "smi_cli_parser.h"
 
class AmdSmiCCCommand : public AmdSmiCommands
{
public:
	AmdSmiCCCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();
	void cc_command_human();
	void cc_command_json();
	void cc_command_csv();

	int cc_command_get_mode(uint64_t processors, std::string &formatted_string);
	int cc_command_vf_get_tdi_state(std::string vf_bdf, std::string &formatted_string);

};
