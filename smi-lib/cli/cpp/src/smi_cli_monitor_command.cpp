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

#include "json/json.h"
#include "tabulate/tabulate.hpp"

#include "smi_cli_helpers.h"
#include "smi_cli_monitor_command.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"
#include "smi_cli_platform.h"

auto constexpr gpu_header_csv {"gpu"};
auto constexpr power_usage_csv{",power_usage"};
auto constexpr temperature_csv{",hotspot_temperature,memory_temperature"};
auto constexpr gfx_csv{",gfx_util,gfx_clock"};
auto constexpr mem_csv{",mem_util,mem_clock"};
auto constexpr encoder_csv{",encoder_util,vclk"};
auto constexpr decoder_csv{",decoder_util,dclk"};
auto constexpr ecc_csv{",total_correctable_count,total_uncorrectable_count"};
auto constexpr vram_csv{",vram_used,vram_total"};
auto constexpr pcie_csv{",pcie_bw,pcie_replay"};
auto constexpr monitor_process_general_header_csv {",pid,name,mem_usage"};
auto constexpr monitor_process_engine_header_csv {",gfx,enc"};

std::string format_monitor_table(tabulate::Table &table)
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
		 .font_align(tabulate::FontAlign::right);
	std::string out = table.str();
	return out;
}

int AmdSmiMonitorCommand::monitor_command_power_usage(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_power_usage_monitor_command(processor,
			  arg,
			  formatted_string);
	return ret;
}

int AmdSmiMonitorCommand::monitor_command_temperature(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_temperature_monitor_command(processor,
			  arg,
			  formatted_string);
	return ret;
}

int AmdSmiMonitorCommand::monitor_command_gfx(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_gfx_monitor_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiMonitorCommand::monitor_command_mem(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_mem_monitor_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiMonitorCommand::monitor_command_encoder(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_encoder_monitor_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiMonitorCommand::monitor_command_decoder(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_decoder_monitor_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiMonitorCommand::monitor_command_ecc(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_ecc_monitor_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiMonitorCommand::monitor_command_vram_usage(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vram_usage_monitor_command(processor,
			  arg,
			  formatted_string);
	return ret;
}

int AmdSmiMonitorCommand::monitor_command_pcie(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_pcie_info_monitor_command(processor,
			  arg,
			  formatted_string);
	return ret;
}

int AmdSmiMonitorCommand::monitor_command_process(uint64_t processor, std::string &formatted_string,
		int &proc_num, int gpu_id)
{
	if (AmdSmiPlatform::getInstance().is_host()) {
		throw SmiToolInvalidParameterException("process");
	}
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_all_arguments_process_command(processor,
			  arg, formatted_string, proc_num, gpu_id);
	return ret;
}

void AmdSmiMonitorCommand::monitor_command_json()
{
	int ret;
	nlohmann::ordered_json json{};
	nlohmann::ordered_json values_json{};
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	std::string out{};
	std::string result{};

	int proc_num = 0;
	int prev_proc_num = 0;

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		json = {};
		json["gpu"] = arg.devices[i]->get_gpu_index();
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		if ((std::find(arg.options.begin(), arg.options.end(), "power-usage") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end())
				|| arg.all_arguments) {
			std::string param{"power-usage"};
			ret = monitor_command_power_usage(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["power_usage"] = values_json;
				values_json.clear();
				out.clear();
			}
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "temperature") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end())
				|| arg.all_arguments) {
			std::string param{"temperature"};
			ret = monitor_command_temperature(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["temperature"] = values_json;
				values_json.clear();
				out.clear();
			}
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "gfx") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end())
				|| arg.all_arguments) {
			std::string param{"gfx"};
			ret = monitor_command_gfx(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["gfx"] = values_json;
				values_json.clear();
				out.clear();
			}
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "mem") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "m") != arg.options.end())
				|| arg.all_arguments) {
			std::string param{"mem"};
			ret = monitor_command_mem(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["mem"] = values_json;
				values_json.clear();
				out.clear();
			}
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "encoder") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "n") != arg.options.end())
				|| arg.all_arguments) {
			std::string param{"encoder"};
			ret = monitor_command_encoder(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["encoder"] = values_json;
				values_json.clear();
				out.clear();
			}
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "decoder") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "d") != arg.options.end())
				|| arg.all_arguments) {
			std::string param{"decoder"};
			ret = monitor_command_decoder(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["decoder"] = values_json;
				values_json.clear();
				out.clear();
			}
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "ecc") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "e") != arg.options.end())
				|| arg.all_arguments) {
			std::string param{"ecc"};
			ret = monitor_command_ecc(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["ecc"] = values_json;
				values_json.clear();
				out.clear();
			}
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "pcie") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "r") != arg.options.end())
				|| arg.all_arguments) {
			std::string param{"pcie"};
			ret = monitor_command_pcie(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["pcie"] = values_json;
				out.clear();
			}
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "vram-usage") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "v") != arg.options.end())
				|| arg.all_arguments) {
			std::string param{"vram-usage"};
			ret = monitor_command_vram_usage(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["vram_usage"] = values_json;
				values_json.clear();
				out.clear();
			}
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "process") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "q") != arg.options.end())) {
			ret = monitor_command_process(gpu_bdf, out, proc_num, i);
			std::string param{"process"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				if (out.size() != 0) {
					values_json = nlohmann::ordered_json::parse(out);
				}
				json["gpu"] = arg.devices[i]->get_gpu_index();
				json["process_list"] = values_json;
				out.clear();
				values_json.clear();
			}
		}
		json_format.insert(json_format.end(), json);
	}
	result = json_format.dump(4);

	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << std::endl;
	}
}

void AmdSmiMonitorCommand::monitor_command_human()
{
	int ret;

	tabulate::Table table;

	std::string formatted_string{};
	tabulate::Table::Row_t header;
	tabulate::Table::Row_t current_row;

	int proc_num = 0;
	int prev_proc_num = 0;

	header.push_back("GPU");
	for (int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		std::string gpu_id{string_format("%d", arg.devices[i]->get_gpu_index())};
		current_row.push_back(gpu_id.c_str());

		if ((std::find(arg.options.begin(), arg.options.end(), "power-usage") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_power_usage(gpu_bdf, formatted_string);
			std::string param{"power-usage"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				header.push_back("POWER");
				current_row.push_back(formatted_string);
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "temperature") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_temperature(gpu_bdf, formatted_string);
			std::string param{"temperature"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				header.push_back("HOTSPOT_TEMP");
				header.push_back("MEM_TEMP");

				std::vector<std::string> cells{};
				cells = split_string(formatted_string, ',');
				for (auto c : cells) {
					current_row.push_back(c);
				}
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "gfx") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_gfx(gpu_bdf, formatted_string);
			std::string param{"gfx"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::vector<std::string> cells{};
				cells = split_string(formatted_string, ',');

				header.push_back("GFX_UTIL");
				header.push_back("GFX_CLOCK");

				for (auto c : cells) {
					current_row.push_back(c);
				}
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "mem") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "m") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_mem(gpu_bdf, formatted_string);
			std::string param{"mem"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::vector<std::string> cells{};
				cells = split_string(formatted_string, ',');

				header.push_back("MEM_UTIL");
				header.push_back("MEM_CLOCK");

				for (auto c : cells) {
					current_row.push_back(c);
				}
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "encoder") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "n") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_encoder(gpu_bdf, formatted_string);
			std::string param{"encoder"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::vector<std::string> cells{};
				cells = split_string(formatted_string, ',');

				if (AmdSmiPlatform::getInstance().is_mi300()) {
					header.push_back("ENC_UTIL");
					header.push_back("VCLK");
				} else {
					for (int i = 0; i < cells.size() / 2; i++) {
						header.push_back(string_format("ENC_UTIL_%d", i));
						header.push_back(string_format("VCLK_%d", i));
					}
				}

				for (auto c : cells) {
					current_row.push_back(c);
				}
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "decoder") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "d") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_decoder(gpu_bdf, formatted_string);
			std::string param{"decoder"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::vector<std::string> cells{};
				cells = split_string(formatted_string, ',');

				if (AmdSmiPlatform::getInstance().is_mi300()) {
					header.push_back("DEC_UTIL");
					header.push_back("DCLK");
				} else {
					for (int i = 0; i < cells.size() / 2; i++) {
						header.push_back(string_format("DEC_UTIL_%d", i));
						header.push_back(string_format("DCLK_%d", i));
					}
				}

				for (auto c : cells) {
					current_row.push_back(c);
				}
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "ecc") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "e") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_ecc(gpu_bdf, formatted_string);
			std::string param{"ecc"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				header.push_back("CORRECTABLE_ECC");
				header.push_back("UNCORRECTABLE_ECC");

				std::vector<std::string> cells{};
				cells = split_string(formatted_string, ',');
				for (auto c : cells) {
					current_row.push_back(c);
				}
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "pcie") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "r") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_pcie(gpu_bdf, formatted_string);
			std::string param{"pcie"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				header.push_back("PCIE_REPLAY");
				header.push_back("PCIE_BW");

				std::vector<std::string> cells{};
				cells = split_string(formatted_string, ',');
				for (auto c : cells) {
					current_row.push_back(c);
				}
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "vram-usage") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "v") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_vram_usage(gpu_bdf, formatted_string);
			std::string param{"vram-usage"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				header.push_back("VRAM_USED");
				header.push_back("VRAM_TOTAL");

				std::vector<std::string> cells{};
				cells = split_string(formatted_string, ',');
				for (auto c : cells) {
					current_row.push_back(c);
				}
			}
			formatted_string.clear();
		}

		if (i == 0) {
			table.add_row(header);
		}
		table.add_row(current_row);
		current_row.clear();
	}

	std::string out{format_monitor_table(table)};
	std::string process_table_str{};
	if ((std::find(arg.options.begin(), arg.options.end(), "process") != arg.options.end())
			|| (std::find(arg.options.begin(), arg.options.end(), "q") != arg.options.end())) {
		for (int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			std::string gpu_id{string_format("%d",i)};
			ret = monitor_command_process(gpu_bdf, formatted_string, proc_num, i);
			std::string param{"process"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				process_table_str += formatted_string;
				formatted_string.clear();
			}
		}
	}

	if (process_table_str.size() != 0) {
		out.append("\n").append("\n").append("PROCESS_INFO:\n").append(process_table_str).append("\n");
	} else {
		out.append("\n");
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
		out.clear();
	} else {
		std::cout << out.c_str() << std::endl;
		out.clear();
	}
}

void AmdSmiMonitorCommand::monitor_command_csv()
{
	int ret;
	std::string headers{};
	std::string values{};
	std::string formatted_string{};
	std::string out{};

	std::vector<std::vector<std::string>> results;
	std::string output_buffer{};
	headers.append(gpu_header_csv);

	int proc_num = 0;
	int prev_proc_num = 0;

	for (int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		std::string gpu_id{string_format("%d", arg.devices[i]->get_gpu_index())};
		results.push_back({gpu_id});

		if ((std::find(arg.options.begin(), arg.options.end(), "power-usage") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_power_usage(gpu_bdf, formatted_string);
			std::string param{"power-usage"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(power_usage_csv);
				std::vector<std::string> power_usage_data{split_string(formatted_string, '\n')};
				std::vector<std::string> power_usage_rows{};
				while (!power_usage_data.empty()) {
					std::string first{power_usage_data.front()};
					power_usage_data.erase(power_usage_data.begin());
					power_usage_rows.push_back(first);
				}
				results.push_back(power_usage_rows);
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "temperature") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_temperature(gpu_bdf, formatted_string);
			std::string param{"temperature"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(temperature_csv);
				std::vector<std::string> temperature_data{split_string(formatted_string, '\n')};
				std::vector<std::string> temperature_rows{};
				while (!temperature_data.empty()) {
					std::string first{temperature_data.front()};
					temperature_data.erase(temperature_data.begin());
					temperature_rows.push_back(first);
				}
				results.push_back(temperature_rows);
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "gfx") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_gfx(gpu_bdf, formatted_string);
			std::string param{"gfx"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(gfx_csv);
				std::vector<std::string> gfx_data{split_string(formatted_string, '\n')};
				std::vector<std::string> gfx_rows{};
				while (!gfx_data.empty()) {
					std::string first{gfx_data.front()};
					gfx_data.erase(gfx_data.begin());
					gfx_rows.push_back(first);
				}
				results.push_back(gfx_rows);
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "mem") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "m") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_mem(gpu_bdf, formatted_string);
			std::string param{"mem"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(mem_csv);
				std::vector<std::string> mem_data{split_string(formatted_string, '\n')};
				std::vector<std::string> mem_rows{};
				while (!mem_data.empty()) {
					std::string first{mem_data.front()};
					mem_data.erase(mem_data.begin());
					mem_rows.push_back(first);
				}
				results.push_back(mem_rows);
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "encoder") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "n") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_encoder(gpu_bdf, formatted_string);
			std::string param{"encoder"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(encoder_csv);
				std::vector<std::string> encoder_data{split_string(formatted_string, '\n')};
				std::vector<std::string> encoder_rows{};
				while (!encoder_data.empty()) {
					std::string first{encoder_data.front()};
					encoder_data.erase(encoder_data.begin());
					encoder_rows.push_back(first);
				}
				results.push_back(encoder_rows);
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "decoder") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "d") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_decoder(gpu_bdf, formatted_string);
			std::string param{"decoder"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(decoder_csv);
				std::vector<std::string> decoder_data{split_string(formatted_string, '\n')};
				std::vector<std::string> decoder_rows{};
				while (!decoder_data.empty()) {
					std::string first{decoder_data.front()};
					decoder_data.erase(decoder_data.begin());
					decoder_rows.push_back(first);
				}
				results.push_back(decoder_rows);
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "ecc") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "e") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_ecc(gpu_bdf, formatted_string);
			std::string param{"ecc"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(ecc_csv);
				std::vector<std::string> ecc_data{split_string(formatted_string, '\n')};
				std::vector<std::string> ecc_rows{};
				while (!ecc_data.empty()) {
					std::string first{ecc_data.front()};
					ecc_data.erase(ecc_data.begin());
					ecc_rows.push_back(first);
				}
				results.push_back(ecc_rows);
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "pcie") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "r") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_pcie(gpu_bdf, formatted_string);
			std::string param{"pcie"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(pcie_csv);
				std::vector<std::string> pcie_data{split_string(formatted_string, '\n')};
				std::vector<std::string> pcie_rows{};
				while (!pcie_data.empty()) {
					std::string first{pcie_data.front()};
					pcie_data.erase(pcie_data.begin());
					pcie_rows.push_back(first);
				}
				results.push_back(pcie_rows);
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "vram-usage") != arg.options.end())
				|| (std::find(arg.options.begin(), arg.options.end(), "v") != arg.options.end())
				|| arg.all_arguments) {
			ret = monitor_command_vram_usage(gpu_bdf, formatted_string);
			std::string param{"vram-usage"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(vram_csv);
				std::vector<std::string> vram_data{split_string(formatted_string, '\n')};
				std::vector<std::string> vram_rows{};
				while (!vram_data.empty()) {
					std::string first{vram_data.front()};
					vram_data.erase(vram_data.begin());
					vram_rows.push_back(first);
				}
				results.push_back(vram_rows);
			}
			formatted_string.clear();
		}

		if (i == 0) {
			out.append(headers).append("\n");
		}

		csv_recursion(output_buffer, results);
		out.append(output_buffer);
		results.clear();
		output_buffer.clear();
		headers.clear();
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "process") != arg.options.end())
			|| (std::find(arg.options.begin(), arg.options.end(), "q") != arg.options.end())) {
		out.append("\n");
		headers.append(gpu_header_csv);
		for (int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			std::string gpu_id{string_format("%d",i)};
			results.push_back({gpu_id});
			ret = monitor_command_process(gpu_bdf, formatted_string, proc_num, i);
			std::string param{"process"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(monitor_process_general_header_csv);
				headers.append(monitor_process_engine_header_csv);
				std::vector<std::string> process_data{split_string(formatted_string, '\n')};
				std::vector<std::string> process_rows{};
				while (!process_data.empty()) {
					std::string first{process_data.front()};
					process_data.erase(process_data.begin());
					process_rows.push_back(first);
				}
				results.push_back(process_rows);
			}
			formatted_string.clear();

			if (i == 0) {
				out.append(headers).append("\n");
			}

			csv_recursion(output_buffer, results);
			out.append(output_buffer);
			results.clear();
			output_buffer.clear();
			headers.clear();
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

void AmdSmiMonitorCommand::monitor_command_watch()
{
	bool should_break{false};
	uint8_t current_iteration{0};
	std::time_t program_start{std::time(nullptr)};
	std::time_t start_timestamp{std::time(nullptr)};

	while (true) {
		std::time_t start_timestamp{std::time(nullptr)};
		if (should_break) {
			break;
		}
		while(std::difftime(std::time(nullptr), start_timestamp) < arg.watch) {}

		monitor_command_human();

		current_iteration++;
		if (current_iteration == arg.iterations) {
			should_break = true;
			break;
		}

		if (arg.watch_time > -1) {
			if (std::difftime(std::time(nullptr), program_start) >= arg.watch_time) {
				should_break = true;
				break;
			}
		}
	}

}

void AmdSmiMonitorCommand::execute_command()
{
	bool should_wait = [](Arguments arg) {
		bool wait{false};
		if (AmdSmiPlatform::getInstance().is_baremetal() || AmdSmiPlatform::getInstance().is_guest()) {
			if (arg.all_arguments) {
				return true;
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "gfx") != arg.options.end())
					|| (std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end())) {
				return true;
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "mem") != arg.options.end())
					|| (std::find(arg.options.begin(), arg.options.end(), "m") != arg.options.end())) {
				return true;
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "power-usage") != arg.options.end())
					|| (std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end())) {
				return true;
			}
		}
		return wait;
	}
	(arg);

	if (should_wait) {
		std::time_t start_timestamp{std::time(nullptr)};
		while(std::difftime(std::time(nullptr), start_timestamp) < 1.1) {}
	}

	if (arg.watch > -1) {
		if (arg.output == json) {
			throw SmiToolInvalidParameterException("--json");
		}
		if (arg.output == csv) {
			throw SmiToolInvalidParameterException("--csv");
		}
		std::cout << "If you want to exit looping, press 'CTRL' + 'C' \n\n";
		monitor_command_watch();
	} else if (arg.output == json) {
		monitor_command_json();
	} else if (arg.output == csv) {
		monitor_command_csv();
	} else if (arg.output == human) {
		monitor_command_human();
	}
}