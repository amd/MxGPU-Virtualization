/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_commands.h"
#include "smi_cli_parser.h"

class AmdSmiTopologyCommand : public AmdSmiCommands
{
private:
	std::vector<std::string> bdf_vector;
	std::vector<std::string> nic_bdf_vector;
public:
	AmdSmiTopologyCommand(Arguments args);
	void execute_command();

	void topology_command_human();
	void topology_command_json();

	int topology_command_weight(std::string &formatted_string);
	int topology_command_hops(std::string &formatted_string);
	int topology_command_fb_sharing(std::string &formatted_string);
	int topology_command_link_type(std::string &formatted_string);
	int p2p_capability_command_coherent(std::string &formatted_string);
	int p2p_capability_command_atomics(std::string &formatted_string);
	int p2p_capability_command_dma(std::string &formatted_string);
	int p2p_capability_command_bi_directional(std::string &formatted_string);

	int topology_command_all_status(std::string &formatted_string);

	int nic_topology_command_link_type(std::string &formatted_string);
	int nic_topology_command_numa(std::string &formatted_string);
};
