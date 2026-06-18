/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"

#include "tabulate/tabulate.hpp"

class AmdSmiPartitionCommand : public AmdSmiCommands
{
public:
	AmdSmiPartitionCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	int accelerator_partition_command(uint64_t processor, std::vector<tabulate::Table::Row_t> &rows,
									  std::vector<tabulate::Table::Row_t> &resource_rows, std::string &gpu_id);
	int memory_partition_command(uint64_t processor, std::string &formatted_string);
	int current_partition_command(uint64_t processor, std::string &formatted_string);
	int global_partition_command(uint64_t processor, std::vector<tabulate::Table::Row_t> &rows,
								 std::string &gpu_id);

	void partition_command_human();
};
