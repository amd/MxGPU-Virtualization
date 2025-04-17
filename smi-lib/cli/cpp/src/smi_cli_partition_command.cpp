/* * Copyright (C) 2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <iostream>

#include "tabulate/tabulate.hpp"

#include "smi_cli_helpers.h"
#include "smi_cli_partition_command.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"
#include "smi_cli_api_host.h"
#include "smi_cli_templates.h"
#include "smi_cli_platform.h"

int AmdSmiPartitionCommand::accelerator_partition_command(uint64_t processor,
		std::vector<tabulate::Table::Row_t> &rows, std::vector<tabulate::Table::Row_t> &resource_rows,
		std::string &gpu_id)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_accelerator_partition_command(processor,
			  arg,
			  rows, resource_rows, gpu_id);
	return ret;
}

int AmdSmiPartitionCommand::memory_partition_command(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_memory_partition_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiPartitionCommand::current_partition_command(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_current_partition_command(processor,
			  arg, formatted_string);
	return ret;
}

std::string format_accelerator_table(tabulate::Table &table)
{
	table.format().font_style({tabulate::FontStyle::bold})
		 .border_top("")
		 .border_bottom("")
		 .border_left("")
		 .border_right("")
		 .corner("")
		 .column_separator("")
		 .padding_top(0)
		 .padding_left(1)
		 .padding_right(1)
		 .padding_bottom(0)
		 .font_align(tabulate::FontAlign::left);
	std::string out = table.str();
	return out;
}

void AmdSmiPartitionCommand::partition_command_human()
{
	int ret;
	std::string out{};
	std::vector<tabulate::Table::Row_t> all_rows;
	std::vector<tabulate::Table::Row_t> resource_rows;
	bool header_added = false;
	std::string formatted_string{};
	int error = 0;

	if ((std::find(arg.options.begin(), arg.options.end(), "current") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end()) ||
			arg.all_arguments) {
		tabulate::Table table;
		tabulate::Table::Row_t header;
		tabulate::Table::Row_t current_row;
		for (int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			std::string gpu_id{string_format("%d", arg.devices[i]->get_gpu_index())};
			ret = current_partition_command(gpu_bdf, formatted_string);
			std::string param{"current"};
			error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::vector<std::string> cells{};
				cells = split_string(formatted_string, '|');

				header.push_back("GPU");
				header.push_back("MEMORY");
				header.push_back("ACCELERATOR_TYPE");
				header.push_back("ACCELERATOR_PROFILE_INDEX");
				header.push_back("PARTITION_ID");

				current_row.push_back(gpu_id);
				for (auto c : cells) {
					current_row.push_back(c);
				}

				formatted_string.clear();
			}
			if (i == 0) {
				table.add_row(header);
			}
			table.add_row(current_row);
			current_row.clear();
			formatted_string.clear();
		}
		if (table.size() != 0 && error == 0) {
			out.append(" CURRENT_PARTITION:\n").append(format_accelerator_table(table)).append("\n\n");
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "memory") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "m") != arg.options.end()) ||
			arg.all_arguments) {
		tabulate::Table table;
		tabulate::Table::Row_t header;
		tabulate::Table::Row_t current_row;
		for (int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			std::string gpu_id{string_format("%d", arg.devices[i]->get_gpu_index())};
			ret = memory_partition_command(gpu_bdf, formatted_string);
			std::string param{"memory"};
			error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::vector<std::string> cells{};
				cells = split_string(formatted_string, '|');

				header.push_back("GPU");
				header.push_back("MEMORY_PARTITION_CAPS");
				header.push_back("CURRENT_MEMORY_PARTITION");

				current_row.push_back(gpu_id);
				for (auto c : cells) {
					current_row.push_back(c);
				}

				formatted_string.clear();
			}
			if (i == 0) {
				table.add_row(header);
			}
			table.add_row(current_row);
			current_row.clear();
			formatted_string.clear();
		}

		if (table.size() != 0 && error == 0) {
			out.append(" MEMORY_PARTITION:\n").append(format_accelerator_table(table)).append("\n\n");
		}
	}

	for (int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		std::string gpu_id{string_format("%d", arg.devices[i]->get_gpu_index())};

		if ((std::find(arg.options.begin(), arg.options.end(), "accelerator") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "a") != arg.options.end())
				|| arg.all_arguments) {
			ret = accelerator_partition_command(gpu_bdf, all_rows, resource_rows, gpu_id);
			std::string param{"accelerator"};
			error = handle_exceptions(ret, param, arg);
			if (i == 0 && error == 0) {
				out += " ACCELERATOR_PARTITION_PROFILES:\n";
			}
			if (error == 0 && i == arg.devices.size() - 1) {
				//Add tables and header only once
				tabulate::Table profiles_table;
				profiles_table.add_row({"GPU", "PROFILE_INDEX", "MEMORY_PARTITION_CAPS", "ACCELERATOR_TYPE", "PARTITION_ID",
										"NUM_PARTITIONS", "NUM_RESOURCES", "RESOURCE_INDEX", "RESOURCE_TYPE", "RESOURCE_INSTANCES", "RESOURCES_SHARED"});
				for (const auto &row : all_rows) {
					profiles_table.add_row(row);
				}
				out += format_accelerator_table(profiles_table);

				tabulate::Table resources_table;
				resources_table.add_row({"RESOURCE_INDEX", "RESOURCE_TYPE", "RESOURCE_INSTANCES", "RESOURCE_SHARED"});
				for (const auto &row : resource_rows) {
					resources_table.add_row(row);
				}
				out = out +  "\n\n" + " ACCELERATOR_PARTITION_RESOURCES:\n" + format_accelerator_table(
						  resources_table) + "\n\n";
			}
		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
		out.clear();
	} else {
		std::cout << out.c_str() << std::endl;
		out.clear();
	}
}

void AmdSmiPartitionCommand::execute_command()
{
	if (AmdSmiPlatform::getInstance().is_mi300()
			&& AmdSmiPlatform::getInstance().getInstance().is_host()) {
		if (arg.output == json) {
			throw SmiToolInvalidParameterException("--json");
		}
		if (arg.output == csv) {
			throw SmiToolInvalidParameterException("--csv");
		}
		if (arg.output == human) {
			partition_command_human();
		}
	} else {
		std::string command{"partition"};
		throw SmiToolCommandNotSupportedException(command);
	}
}