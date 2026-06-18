/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_commands.h"
#include "smi_cli_parser.h"

class AmdSmiStaticCommand : public AmdSmiCommands
{
public:
	AmdSmiStaticCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	void static_command_json();
	void static_command_human();
	void static_command_csv();

	int static_command_nic_asic(uint64_t processors, std::string &formatted_string);
	int static_command_nic_bus(uint64_t processors, std::string &formatted_string);
	int static_command_nic_driver(uint64_t processors, std::string &formatted_string);
	int static_command_nic_numa(uint64_t processors, std::string &formatted_string);
	int static_command_nic_port(uint64_t processors, std::string &formatted_string);
	int static_command_nic_rdma_devices(uint64_t processors, std::string &formatted_string);

	int static_command_asic(uint64_t processors, std::string &formatted_string);
	int static_command_bus(uint64_t processors,
						   std::string &formatted_string);
	int static_command_vbios(uint64_t processors,
							 std::string &formatted_string);
	int static_command_board_host(uint64_t processors,
								  std::string &formatted_string);
	int static_command_limit(uint64_t processors,
							 std::string &formatted_string);
	int static_command_driver(uint64_t processors,
							  std::string &formatted_string);
	int static_command_ras_host(uint64_t processors,
								std::string &formatted_string);
	int static_command_dfc(uint64_t processors,
						   std::string &formatted_string);
	int static_command_fb_info(uint64_t processors,
							   std::string &formatted_string);
	int static_command_num_vf(uint64_t processors,
							  std::string &formatted_string);
	int static_command_vram(uint64_t processors,
							std::string &formatted_string);
	int static_command_cache(uint64_t processors,
							 std::string &formatted_string);
	int static_command_process_isolation(uint64_t processors,
										 std::string &formatted_string);
	int static_command_partition(uint64_t processors, std::string &formatted_string);
	int static_command_soc_pstate(uint64_t processors,
					  std::string &formatted_string);
	int static_command_virtualization_mode(uint64_t processors,
					  std::string &formatted_string);
	int static_command_numa(uint64_t processors,
					std::string &formatted_string);
	int static_command_vf_hbm_info(std::string vf_handle,
					std::string &formatted_string);
	int static_command_xgmi_plpd(uint64_t processors, std::string &formatted_string);
	int static_command_ptl(uint64_t processors, std::string &formatted_string);
};
