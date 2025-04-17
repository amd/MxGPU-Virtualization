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
#include "interface/amdsmi.h"
#include "smi_cli_api_guest.h"
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_exception.h"

#include "json/json.h"
#include "tabulate/tabulate.hpp"

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PROCESS_LIST)(amdsmi_processor_handle, uint32_t *,
		amdsmi_proc_info_t *);


extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF guest_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_PROCESS_LIST guest_amdsmi_get_gpu_process_list;

std::string format_process_table(tabulate::Table &table)
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

int amdsmi_get_process_command_info(uint64_t processor, Arguments arg,
									std::vector<amdsmi_proc_info_t> &process_info, int gpu_id, int &proc_num)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor_handle;
	amdsmi_bdf_t tmp_bdf;
	uint32_t max_processes = 0;
	tmp_bdf.as_uint = processor;
	auto process_list_json = nlohmann::ordered_json::array();
	std::vector<amdsmi_proc_info_t> process_info_list;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	std::time_t start_timestamp{std::time(nullptr)};
	while(std::difftime(std::time(nullptr), start_timestamp) < 1.1) {}
	ret = guest_amdsmi_get_gpu_process_list(processor_handle, &max_processes, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	// Allocate memory for the process info list
	amdsmi_proc_info_t *info_list = (amdsmi_proc_info_t*)malloc(max_processes * sizeof(
										amdsmi_proc_info_t));
	if (info_list == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = guest_amdsmi_get_gpu_process_list(processor_handle, &max_processes, info_list);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		free(info_list);
		return ret;
	}

	// Copy the process info list to the output vector
	for (uint32_t i = 0; i < max_processes; i++) {
		process_info_list.push_back(info_list[i]);
	}

	if (arg.process_map.size() != 0) {
		for (const auto& map_elem : arg.process_map) {
			if (map_elem.second == ProcessType::name) {
				auto it = std::find_if(process_info_list.begin(),
				process_info_list.end(), [&map_elem](const amdsmi_proc_info_t& proc) {
					return proc.name == map_elem.first;
				});

				if (it != process_info_list.end()) {
					process_info.push_back(*it);
					proc_num +=1;
				} else {
					if (arg.process_map.size() == 1) {
						return INVALID_PARAM_VALUE;
					} else {
						proc_num +=1;
					}
				}
			} else if (map_elem.second == ProcessType::pid) {
				auto it = std::find_if(process_info_list.begin(),
				process_info_list.end(), [&map_elem](const amdsmi_proc_info_t& proc) {
					return std::to_string(proc.pid) == map_elem.first;
				});
				if (it != process_info_list.end()) {
					process_info.push_back(*it);
					proc_num +=1;
				} else {
					if (arg.process_map.size() == 1) {
						return INVALID_PARAM_VALUE;
					} else {
						proc_num +=1;
					}
				}
			}
		}
	} else {
		process_info = process_info_list;
	}

	if (process_info.size() == 0) {
		free(info_list);
		return INVALID_PARAM_VALUE;
	}

	free(info_list);
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_all_arguments_process_command(uint64_t processor, Arguments arg,
		std::string &out_string, int &proc_num, int gpu_id)
{
	std::vector<amdsmi_proc_info_t> process_info;
	int ret = amdsmi_get_process_command_info(processor, arg, process_info, gpu_id, proc_num);
	if (ret != 0 || process_info.size() == 0) {
		if (arg.watch > -1) {
			std::string value = "N/A";
			out_string += string_format(
							  "%s,%s,%s,%s,%s\n", value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str());
			return 0;
		}
		return ret;
	}
	auto process_list_json = nlohmann::ordered_json::array();
	tabulate::Table table;

	if (arg.output == human) {
		tabulate::Table::Row_t header;
		header.push_back("Name");
		header.push_back("GPU");
		header.push_back("PID");
		header.push_back("MEM_USAGE");
		header.push_back("GFX");
		header.push_back("ENC");
		table.add_row(header);
	}

	for (uint32_t i = 0; i < process_info.size(); i++) {
		int proc_mem = convert_bytes_to_megabytes(process_info[i].mem);
		std::string mem = { string_format("%lld", proc_mem) };
		std::string gfx = { string_format("%lld", process_info[i].engine_usage.gfx) };
		std::string enc = { string_format("%lld", process_info[i].engine_usage.enc) };
		if (arg.watch > -1) {
			std::string pid{string_format("%ld", process_info[i].pid)};
			if (arg.command == "monitor") {
				std::string gpu_id_str{string_format("%d", gpu_id)};
				std::string mem_unit{mem == "N/A" ? "" : "MB"};
				std::string gfx_unit{gfx == "N/A" ? "" : "%"};
				std::string enc_unit{enc == "N/A" ? "" : "%"};
				std::string mem_usage{string_format("%s %s", mem.c_str(), mem_unit.c_str())};
				std::string gfx_str{string_format("%s %s", gfx.c_str(), gfx_unit.c_str())};
				std::string enc_str{string_format("%s %s", enc.c_str(), enc_unit.c_str())};
				table.add_row({process_info[i].name, gpu_id_str.c_str(), pid.c_str(),
							   mem_usage.c_str(),gfx_str.c_str(), enc_str.c_str()});
			} else {
				out_string += string_format(
								  "%s,%s,%s,%s,%s\n", pid.c_str(), process_info[i].name, mem.c_str(), gfx.c_str(), enc.c_str());
			}
		} else if (arg.output == json) {
			nlohmann::ordered_json mem_json{};
			mem_json["value"] = proc_mem;
			mem_json["unit"] = mem == "N/A" ? "N/A" : "MB";
			nlohmann::ordered_json gfx_json{};
			gfx_json["value"] = process_info[i].engine_usage.gfx;
			gfx_json["unit"] = gfx == "N/A" ? "N/A" : "%";
			nlohmann::ordered_json enc_json{};
			enc_json["value"] = process_info[i].engine_usage.enc;
			enc_json["unit"] = enc == "N/A" ? "N/A" : "%";
			nlohmann::ordered_json json_usage = { { "gfx", gfx_json },
				{ "enc", enc_json }
			};
			nlohmann::ordered_json process_info_json{nlohmann::ordered_json::object( {
					{ "pid", process_info[i].pid },
					{ "name", process_info[i].name },
					{ "mem_usage", mem_json },
					{ "usage", json_usage } })};

			process_list_json.push_back(nlohmann::ordered_json::object( {{"process_info", process_info_json}}));
		} else {
			std::string pid{string_format("%ld", process_info[i].pid)};
			if (arg.output == csv) {
				std::string gpu_id_str{string_format("%d", gpu_id)};
				out_string += string_format(
								  "%s,%s,%s,%s,%s,%s\n",gpu_id_str.c_str(), pid.c_str(), process_info[i].name, mem.c_str(),
								  gfx.c_str(), enc.c_str());
			} else {
				std::string gpu_id_str{string_format("%d", gpu_id)};
				std::string mem_unit{mem == "N/A" ? "" : "MB"};
				std::string gfx_unit{gfx == "N/A" ? "" : "%"};
				std::string enc_unit{enc == "N/A" ? "" : "%"};
				std::string mem_usage{string_format("%s %s", mem.c_str(), mem_unit.c_str())};
				std::string gfx_str{string_format("%s %s", gfx.c_str(), gfx_unit.c_str())};
				std::string enc_str{string_format("%s %s", enc.c_str(), enc_unit.c_str())};
				table.add_row({process_info[i].name, gpu_id_str.c_str(), pid.c_str(),
							   mem_usage.c_str(),gfx_str.c_str(), enc_str.c_str()});
			}
		}
	}

	if (arg.watch > -1 && arg.command == "monitor"
			|| arg.output == human && arg.watch == -1) {
		out_string += format_process_table(table);
	}

	if (arg.output == json) {
		out_string += process_list_json.dump(4);
	}
	return ret;
}

int AmdSmiApiGuest::amdsmi_get_general_process_command(uint64_t processor, Arguments arg,
		std::string &out_string, int &proc_num, int gpu_id)
{
	std::vector<amdsmi_proc_info_t> process_info;
	int ret = amdsmi_get_process_command_info(processor, arg, process_info, gpu_id, proc_num);
	if (ret != 0 || process_info.size() == 0) {
		if (arg.watch > -1) {
			std::string value = "N/A";
			out_string += string_format(
							  "%s,%s,%s\n", value.c_str(), value.c_str(), value.c_str());
			return 0;
		}
		return ret;
	}
	auto process_list_json = nlohmann::ordered_json::array();
	tabulate::Table table;

	if (arg.output == human) {
		tabulate::Table::Row_t header;
		header.push_back("Name");
		header.push_back("GPU");
		header.push_back("PID");
		header.push_back("MEM_USAGE");
		table.add_row(header);
	}

	for (uint32_t i = 0; i < process_info.size(); i++) {
		int proc_mem = convert_bytes_to_megabytes(process_info[i].mem);
		std::string mem = { string_format("%lld", proc_mem) };
		std::string mem_unit = mem == "N/A" ? "N/A" : "MB";
		if (arg.watch > -1) {
			std::string pid{string_format("%ld", process_info[i].pid)};
			out_string += string_format(
							  "%s,%s,%s\n", pid.c_str(), process_info[i].name, mem.c_str());
		} else if (arg.output == json) {
			nlohmann::ordered_json mem_json{};
			mem_json["value"] = proc_mem;
			mem_json["unit"] = mem_unit;
			nlohmann::ordered_json process_info_json{ nlohmann::ordered_json::object( {
					{ "pid", process_info[i].pid },
					{ "name", process_info[i].name },
					{ "mem_usage", mem_json} }) };
			process_list_json.push_back(nlohmann::ordered_json::object( {{"process_info", process_info_json}}));
		} else {
			std::string pid = { string_format("%ld", process_info[i].pid) };
			if (arg.output == csv) {
				std::string gpu_id_str{string_format("%d", gpu_id)};
				out_string += string_format(
								  "%s,%s,%s,%s\n", gpu_id_str.c_str(), pid.c_str(), process_info[i].name, mem.c_str());
			} else {
				std::string gpu_id_str{string_format("%d", gpu_id)};
				std::string mem_unit{mem == "N/A" ? "" : "MB"};
				std::string mem_usage{string_format("%s %s", mem.c_str(), mem_unit.c_str())};
				table.add_row({process_info[i].name, gpu_id_str.c_str(), pid.c_str(), mem_usage.c_str()});
			}
		}
	}

	if (arg.output == human && arg.watch <= -1) {
		out_string += format_process_table(table);
	}

	if (arg.output == json) {
		out_string += process_list_json.dump(4);
	}

	return ret;
}

int AmdSmiApiGuest::amdsmi_get_engine_process_command(uint64_t processor, Arguments arg,
		std::string &out_string, int &proc_num, int gpu_id)
{
	std::vector<amdsmi_proc_info_t> process_info;
	int ret = amdsmi_get_process_command_info(processor, arg, process_info, gpu_id, proc_num);
	if (ret != 0 || process_info.size() == 0) {
		if (arg.watch > -1) {
			std::string value = "N/A";
			out_string += string_format(
							  "%s,%s,%s\n", value.c_str(), value.c_str(), value.c_str());
			return 0;
		}
		return ret;
	}
	auto process_list_json = nlohmann::ordered_json::array();
	tabulate::Table table;

	if (arg.output == human) {
		tabulate::Table::Row_t header;
		header.push_back("Name");
		header.push_back("GPU");
		header.push_back("GFX");
		header.push_back("ENC");
		table.add_row(header);
	}

	for (uint32_t i = 0; i < process_info.size(); i++) {
		std::string gfx = { string_format("%lld", process_info[i].engine_usage.gfx) };
		std::string enc = { string_format("%lld", process_info[i].engine_usage.enc) };
		if (arg.watch > -1) {
			out_string += string_format(
							  "%s,%s\n", gfx.c_str(), enc.c_str());
		} else if (arg.output == json) {
			nlohmann::ordered_json gfx_json{};
			gfx_json["value"] = process_info[i].engine_usage.gfx;
			gfx_json["unit"] = gfx == "N/A" ? "N/A" : "%";
			nlohmann::ordered_json enc_json{};
			enc_json["value"] = process_info[i].engine_usage.enc;
			enc_json["unit"] = enc == "N/A" ? "N/A" : "%";
			nlohmann::ordered_json json_usage = { { "gfx", gfx_json },
				{ "enc", enc_json }
			};
			nlohmann::ordered_json process_info_json{nlohmann::ordered_json::object({ { "usage", json_usage } })};
			process_list_json.push_back(nlohmann::ordered_json::object( {{"process_info", process_info_json}}));
		} else {
			if (arg.output == csv) {
				std::string gpu_id_str{string_format("%d", gpu_id)};
				out_string += string_format(
								  "%s,%s,%s\n", gpu_id_str.c_str(), gfx.c_str(), enc.c_str());
			} else {
				std::string gfx_unit{gfx == "N/A" ? "" : "%"};
				std::string enc_unit{enc == "N/A" ? "" : "%"};
				std::string gpu_id_str{string_format("%d", gpu_id)};
				std::string gfx_str{string_format("%s %s", gfx.c_str(), gfx_unit.c_str())};
				std::string enc_str{string_format("%s %s", enc.c_str(), enc_unit.c_str())};
				table.add_row({process_info[i].name, gpu_id_str.c_str(), gfx_str.c_str(), enc_str.c_str()});
			}
		}
	}

	if (arg.output == human && arg.watch <= -1) {
		out_string += format_process_table(table);
	}

	if (arg.output == json) {
		out_string += process_list_json.dump(4);
	}

	return ret;
}
