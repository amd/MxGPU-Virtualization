/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"


class AmdSmiDefaultCommand : public AmdSmiCommands
{
public:
	AmdSmiDefaultCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	int default_command_version(uint64_t processor_bdf, std::string &formatted_string);
	int default_command_bdf(uint64_t index, Arguments arg, std::string &formatted_string);
	int default_command_vf_data(uint64_t index, Arguments arg, uint64_t &num_vfs,
			std::vector<std::vector<std::string>> &vf_data);
	int default_command_gpu_name(uint64_t processor_bdf, std::string &formatted_string);
	int default_command_get_partition_mode(uint64_t processor_bdf, std::string &formatted_string);
	int default_command_uec(uint64_t processor_bdf, std::string &formatted_string);
	int default_command_temperature(uint64_t processor_bdf, std::string &formatted_string);
	int default_command_power_usage(uint64_t processor_bdf, std::string &formatted_string);
	int default_command_utilization(uint64_t processor_bdf, std::string &formatted_string);
	int default_command_pcie_info(uint64_t processor_bdf, std::string &formatted_string);
	int default_command_fb_usage(uint64_t processor_bdf, std::string &formatted_string);
	int default_get_process_info(uint64_t processor_bdf, std::string &formatted_string, int &proc_num, int gpu_id);

private:
	struct DefaultCommandData;
	void gather_default_command_data(DefaultCommandData &data);
	void gather_gpu_row_data(size_t i, DefaultCommandData &data);
	void align_default_tables(DefaultCommandData &data);
	void print_default_command_output(const DefaultCommandData &data);
	void print_version_block(const DefaultCommandData &data, size_t total_width) const;
	void print_gpu_vf_tables(const DefaultCommandData &data) const;
	void print_process_table(const DefaultCommandData &data) const;
};
