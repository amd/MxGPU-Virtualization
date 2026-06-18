/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_commands.h"
#include "smi_cli_parser.h"

class AmdSmiFirmwareCommand : public AmdSmiCommands
{
public:
	AmdSmiFirmwareCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	void firmware_command_json();
	void firmware_command_human();
	void firmware_command_csv();

	int firmware_command_fw_list(uint64_t processors,
								 std::string &out_string,
								 std::string *gpu_id = nullptr);
	int firmware_command_err_rec(uint64_t processors,
								 std::string &out_string);
	int firmware_command_vf_fw_list(std::string vf_handle,
									std::string &out_string,
									std::string *gpu_id = nullptr, std::string *vf_id = nullptr);
	int firmware_command_nic_fw(uint64_t processor_bdf,
								std::string &out_string);
};