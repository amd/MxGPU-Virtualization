/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
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
#include <limits>
#include <cstdint>
#include <regex>

#include "json/json.h"

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_metric_command.h"
#include "smi_cli_templates.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"
#include "smi_cli_platform.h"

auto constexpr gpu_header_csv {"gpu"};
auto constexpr vf_header_csv {"vf"};
auto constexpr
usage_header_csv {",gfx_activity,umc_activity,mm_activity,vcn,vcn_activity,jpeg,jpeg_activity"};
auto constexpr
power_header_csv {",socket_power,gfx_voltage,soc_voltage,mem_voltage,power_management"};
auto constexpr
power_header_csv_guest_bm {",socket_power,voltage,soc_voltage,mem_voltage"};
auto constexpr clock_header_csv {
	",gfx_clk,gfx_min_clk,gfx_max_clk,gfx_clk_locked,gfx_clk_deep_sleep,mem_clk,mem_min_clk,mem_max_clk,mem_clk_locked,mem_clk_deep_sleep,"
	"vclk_0_clk,vclk_0_min_clk,vclk_0_max_clk,vclk_0_clk_locked,vclk_0_clk_deep_sleep,vclk_1_clk,vclk_1_min_clk,vclk_1_max_clk,vclk_1_clk_locked,vclk_1_clk_deep_sleep,"
	"dclk_0_clk,dclk_0_min_clk,dclk_0_max_clk,dclk_0_clk_locked,dclk_0_clk_deep_sleep,dclk_1_clk,dclk_1_min_clk,dclk_1_max_clk,dclk_1_clk_locked,dclk_1_clk_deep_sleep"
};
auto constexpr clock_header_csv_mi {
	",gfx,gfx_clk,gfx_min_clk,gfx_max_clk,gfx_clk_locked,gfx_clk_deep_sleep,mem,mem_clk,mem_min_clk,mem_max_clk,mem_clk_locked,mem_clk_deep_sleep,"
	"vclk,vclk_clk,vclk_min_clk,vclk_max_clk,vclk_clk_locked,vclk_clk_deep_sleep,dclk,dclk_clk,dclk_min_clk,dclk_max_clk,dclk_clk_locked,dclk_clk_deep_sleep"
};
auto constexpr clock_header_csv_guest_bm {
	",gfx_clk,gfx_min_clk,gfx_max_clk,gfx_clk_locked,gfx_clk_deep_sleep,mem_clk,mem_min_clk,mem_max_clk,mem_clk_locked,mem_clk_deep_sleep,"
	"vclk_0_clk,vclk_0_min_clk,vclk_0_max_clk,vclk_0_clk_locked,vclk_0_clk_deep_sleep,vclk_1_clk,vclk_1_min_clk,vclk_1_max_clk,vclk_1_clk_locked,vclk_1_clk_deep_sleep"
};
auto constexpr temperature_header_csv {",edge_temperature,hotspot_temperature,vram_temperature"};
auto constexpr
ecc_header_csv {",total_correctable_count,total_uncorrectable_count,total_deferred_count,cache_correctable_count,cache_uncorrectable_count"};
auto constexpr ecc_block_header_csv{",block,correctable_count,uncorrectable_count,deferred_count"};
auto constexpr
pcie_header_csv {",pcie_current_width,pcie_current_speed,pcie_current_bandwidth,pcie_replay_count,pcie_l0_to_recovery_count,pcie_replay_roll_over_count,pcie_nak_sent_count,pcie_nak_received_count"};

auto constexpr
vf_schedule_header {",boot_up_time,flr_count,vf_state,last_boot_start,last_boot_end,last_shutdown_start,last_shutdown_end,shutdown_time,last_reset_start,last_reset_end,reset_time,current_active_time,current_running_time,total_active_time,total_running_time"};
auto constexpr vf_guard_header {",enabled,guard_type,guard_state,amount,interval,threshold,active"};
auto constexpr vf_guest_data_header {",driver_version,fb_usage"};

auto constexpr fb_usage_header_csv {",fb_total,fb_used"};
auto constexpr energy_header_csv {",energy"};

int AmdSmiMetricCommand::metric_command_usage(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_usage_metric_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_command_power(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_power_metric_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_command_clock(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_clock_metric_command(processor,
			  arg, formatted_string);
	return ret;
}

int
AmdSmiMetricCommand::metric_command_temperature(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_temperature_metric_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_command_ecc(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_ecc_metric_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_command_ecc_block(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_ecc_block_metric_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_command_pcie(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_pcie_metric_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_vf_command_schedule(std::string vf_handle,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_schedule_metric_command(vf_handle,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_vf_command_guard(std::string vf_handle,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_guard_metric_command(vf_handle,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_vf_command_guest_data(std::string vf_handle,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_guest_data_metric_command(vf_handle,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_command_fb_usage(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_fb_usage_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiMetricCommand::metric_command_energy(uint64_t processor,
		std::string &formatted_string)
{
	if (!(AmdSmiPlatform::getInstance().is_host() && AmdSmiPlatform::getInstance().is_mi300()))
		return 2;

	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_energy_metric_command(processor,
			  arg, formatted_string);
	return ret;
}

void AmdSmiMetricCommand::metric_command_json()
{
	int ret;
	unsigned int i;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	nlohmann::ordered_json values_json;
	std::string out{};
	std::string result;
	if (arg.is_vf) {
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		vf_bdf = std::get<2>(indexes).c_str();
		json = {};
		json["gpu"] = std::stoi(std::get<0>(indexes).c_str());
		json["vf"] = std::stoi(std::get<1>(indexes).c_str());
		if (is_vf_schedule || arg.all_arguments) {
			ret = metric_vf_command_schedule(vf_bdf, out);
			std::string param{"vf_schedule"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				values_json = nlohmann::ordered_json::parse(out);
				json["schedule"] = values_json;
				out.clear();
			}
		}
		if (is_vf_guard_info || arg.all_arguments) {
			ret = metric_vf_command_guard(vf_bdf, out);
			std::string param{"vf_command_guard"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				values_json = nlohmann::ordered_json::parse(out);
				json["guard"] = values_json;
				out.clear();
			}
		}
		if (is_vf_guest_data || arg.all_arguments) {
			ret = metric_vf_command_guest_data(vf_bdf, out);
			std::string param{"guest_data"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				values_json = nlohmann::ordered_json::parse(out);
				json["guest_data"] = values_json;
				out.clear();
			}

		}
		json_format.insert(json_format.end(), json);
		result = json_format.dump(4);
	} else {
		for (i = 0; i < arg.devices.size(); i++) {
			json = {};
			json["gpu"] = arg.devices[i]->get_gpu_index();
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			if ((std::find(arg.options.begin(), arg.options.end(), "usage") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"usage"};
				ret = metric_command_usage(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["usage"] = values_json;
					out.clear();
				}
			}

			if ((std::find(arg.options.begin(), arg.options.end(), "fb-usage") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "fb") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_fb_usage(gpu_bdf, out);
				std::string param{"fb-usage"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["fb_usage"] = values_json;
					out.clear();
				}
			}

			if ((std::find(arg.options.begin(), arg.options.end(), "power") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_power(gpu_bdf, out);
				std::string param{"power"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["power"] = values_json;
					out.clear();
				}
			}

			if ((std::find(arg.options.begin(), arg.options.end(), "clock") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_clock(gpu_bdf, out);
				std::string param{"clock"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["clock"] = values_json;
					out.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "temperature") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_temperature(gpu_bdf, out);
				std::string param{"temperature"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["temperature"] = values_json;
					out.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "pcie") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "P") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_pcie(gpu_bdf, out);
				std::string param{"pcie"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["pcie"] = values_json;
					out.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "ecc") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "e") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_ecc(gpu_bdf, out);
				std::string param{"ecc"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (out != "") {
						values_json = nlohmann::ordered_json::parse(out);
						json["ecc"] = values_json;
						out.clear();
					}
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "ecc-block") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "k") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_ecc_block(gpu_bdf, out);
				std::string param{"ecc-blocks"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (out != "") {
						values_json = nlohmann::ordered_json::parse(out);
						json["ecc_blocks"] = values_json;
						out.clear();
					}
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "energy") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "E") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_energy(gpu_bdf, out);
				std::string param{"energy"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (out != "") {
						values_json = nlohmann::ordered_json::parse(out);
						json["energy"] = values_json;
						out.clear();
					}
				}
			}

			json_format.insert(json_format.end(), json);
		}
		result = json_format.dump(4);
	}
	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << '\n';
	}
}

void AmdSmiMetricCommand::metric_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};
	if (arg.is_vf) {
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		out += string_format(
				   vfNestedTemplate, std::get<0>(indexes).c_str(),
				   std::get<1>(indexes).c_str());
		vf_bdf = std::get<2>(indexes).c_str();

		if (is_vf_schedule || arg.all_arguments) {
			ret = metric_vf_command_schedule(vf_bdf, formatted_string);
			std::string param{"schedule"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
				formatted_string.clear();
			}
		}
		if (is_vf_guard_info || arg.all_arguments) {
			ret = metric_vf_command_guard(vf_bdf, formatted_string);
			std::string param{"command_guard"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
				formatted_string.clear();
			}
		}
		if (is_vf_guest_data || arg.all_arguments) {
			ret = metric_vf_command_guest_data(vf_bdf, formatted_string);
			std::string param{"guest_data"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
				formatted_string.clear();
			}
		}
	} else {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
			if ((std::find(arg.options.begin(), arg.options.end(), "usage") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_usage(gpu_bdf, formatted_string);
				std::string param{"usage"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "fb-usage") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "fb") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_fb_usage(gpu_bdf, formatted_string);
				std::string param{"fb-usage"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "power") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "p") !=  arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_power(gpu_bdf, formatted_string);
				std::string param{"power"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "clock") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_clock(gpu_bdf, formatted_string);
				std::string param{"clock"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "temperature") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_temperature(gpu_bdf, formatted_string);
				std::string param{"temperature"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "pcie") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "P") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_pcie(gpu_bdf, formatted_string);
				std::string param{"pcie"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "ecc") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "e") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_ecc(gpu_bdf, formatted_string);
				std::string param{"ecc"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "ecc-block") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "k") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_ecc_block(gpu_bdf, formatted_string);
				std::string param{"ecc-block"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}

			if ((std::find(arg.options.begin(), arg.options.end(), "energy") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "E") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_energy(gpu_bdf, formatted_string);
				std::string param{"energy"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}

		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}

}

void AmdSmiMetricCommand::metric_command_csv()
{
	int ret;
	std::string headers{};
	std::string values{};
	std::string formatted_string{};
	std::string out{};

	std::vector<std::vector<std::string>> results;
	std::string output_buffer{};
	headers.append(gpu_header_csv);

	if (arg.is_vf) {
		headers.append(",").append(vf_header_csv);
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		vf_bdf = std::get<2>(indexes).c_str();
		if (is_vf_schedule || arg.all_arguments) {
			ret = metric_vf_command_schedule(vf_bdf, formatted_string);
			std::string param{"schedule"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(vf_schedule_header);
				std::vector<std::string> usage_data{split_string(formatted_string, '\n')};
				std::vector<std::string> usage_rows{};
				while (!usage_data.empty()) {
					std::string first{};
					first = usage_data.front();
					usage_data.erase(usage_data.begin());
					usage_rows.push_back(first);
				}
				results.push_back(usage_rows);
			}
			formatted_string.clear();
		}
		if (is_vf_guard_info || arg.all_arguments) {
			ret = metric_vf_command_guard(vf_bdf, formatted_string);
			std::string param{"command_guard"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(vf_guard_header);
				std::vector<std::string> usage_data{split_string(formatted_string, '\n')};
				std::vector<std::string> usage_rows{};
				while (!usage_data.empty()) {
					std::string first{};
					first = usage_data.front();
					usage_data.erase(usage_data.begin());
					usage_rows.push_back(first);
				}
				results.push_back(usage_rows);
			}
			formatted_string.clear();
		}
		if (is_vf_guest_data || arg.all_arguments) {
			ret = metric_vf_command_guest_data(vf_bdf, formatted_string);
			std::string param{"guest_data"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				headers.append(vf_guest_data_header);
				std::vector<std::string> usage_data{split_string(formatted_string, '\n')};
				std::vector<std::string> usage_rows{};
				while (!usage_data.empty()) {
					std::string first{};
					first = usage_data.front();
					usage_data.erase(usage_data.begin());
					usage_rows.push_back(first);
				}
				results.push_back(usage_rows);
			}
			formatted_string.clear();
		}
		out.append(headers).append("\n");

		csv_recursion(output_buffer, results);
		out.append(output_buffer);
		results.clear();
		output_buffer.clear();
	} else {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			std::string gpu_id{string_format("%d",i)};
			results.push_back({gpu_id});

			if ((std::find(arg.options.begin(), arg.options.end(), "usage") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_usage(gpu_bdf, formatted_string);
				std::string param{"usage"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					headers.append(usage_header_csv);
					std::vector<std::string> usage_data{split_string(formatted_string, '\n')};
					std::vector<std::string> usage_rows{};
					while (!usage_data.empty()) {
						std::string first{};
						first = usage_data.front();
						usage_data.erase(usage_data.begin());
						usage_rows.push_back(first);
					}
					results.push_back(usage_rows);
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "fb-usage") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "fb") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_fb_usage(gpu_bdf, formatted_string);
				std::string param{"fb-usage"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					headers.append(fb_usage_header_csv);
					results.push_back({formatted_string});
				}
				formatted_string.clear();
			}

			if ((std::find(arg.options.begin(), arg.options.end(), "power") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "p") !=  arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_power(gpu_bdf, formatted_string);
				std::string param{"power"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (AmdSmiPlatform::getInstance().is_host()) {
						headers.append(power_header_csv);
					} else {
						headers.append(power_header_csv_guest_bm);
					}
					results.push_back({formatted_string});
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "clock") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_clock(gpu_bdf, formatted_string);
				std::string param{"clock"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (AmdSmiPlatform::getInstance().is_host() && AmdSmiPlatform::getInstance().is_mi300()) {
						headers.append(clock_header_csv_mi);
					} else if (AmdSmiPlatform::getInstance().is_host()) {
						headers.append(clock_header_csv);
					} else {
						headers.append(clock_header_csv_guest_bm);
					}
					std::vector<std::string> clock_data{split_string(formatted_string, '\n')};
					std::vector<std::string> clock_rows{};
					while (!clock_data.empty()) {
						std::string first{};
						first = clock_data.front();
						clock_data.erase(clock_data.begin());
						clock_rows.push_back(first);
					}
					results.push_back(clock_rows);
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "temperature") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_temperature(gpu_bdf, formatted_string);
				std::string param{"temperature"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					headers.append(temperature_header_csv);
					results.push_back({formatted_string});
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "pcie") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "P") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_pcie(gpu_bdf, formatted_string);
				std::string param{"pcie"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					headers.append(pcie_header_csv);
					results.push_back({formatted_string});
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "ecc") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "e") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_ecc(gpu_bdf, formatted_string);
				std::string param{"ecc"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if(AmdSmiPlatform::getInstance().is_baremetal()) {
						headers.append(ecc_header_csv);
						results.push_back({formatted_string});
					} else {
						headers.append(ecc_header_csv);
						std::vector<std::string> ecc_data{split_string(formatted_string, '\n')};
						std::vector<std::string> ecc_rows{};
						while (!ecc_data.empty()) {
							std::string first{};
							first = ecc_data.front();
							ecc_data.erase(ecc_data.begin());
							ecc_rows.push_back(first);
						}
						results.push_back(ecc_rows);
					}

				}
				formatted_string.clear();
			}

			if ((std::find(arg.options.begin(), arg.options.end(), "ecc-block") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "k") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_ecc_block(gpu_bdf, formatted_string);
				std::string param{"ecc"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if(AmdSmiPlatform::getInstance().is_baremetal()) {
						headers.append(ecc_block_header_csv);
						results.push_back({formatted_string});
					} else {
						headers.append(ecc_block_header_csv);
						std::vector<std::string> ecc_data{split_string(formatted_string, '\n')};
						std::vector<std::string> ecc_rows{};
						while (!ecc_data.empty()) {
							std::string first{};
							first = ecc_data.front();
							ecc_data.erase(ecc_data.begin());
							ecc_rows.push_back(first);
						}
						results.push_back(ecc_rows);
					}

				}
				formatted_string.clear();
			}

			if ((std::find(arg.options.begin(), arg.options.end(), "energy") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "E") != arg.options.end()) ||
					arg.all_arguments) {
				ret = metric_command_energy(gpu_bdf, formatted_string);
				std::string param{"energy"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					headers.append(energy_header_csv);
					results.push_back({formatted_string});
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

void AmdSmiMetricCommand::metric_command_watch()
{
	int ret;
	std::string out{};
	std::string formatted_string{};
	int MAX_LINE_LENGTH{32};
	std::vector<std::string> first_row{};
	std::vector<std::string> second_row{};
	std::vector<std::string> third_row{};
	std::vector<std::string> fourth_row{};
	std::vector<std::vector<std::string>> table;
	std::vector<std::vector<std::string>> value_rows;
	uint8_t current_iteration{0};
	bool should_break{false};
	std::time_t program_start{std::time(nullptr)};

	while (true) {
		std::time_t start_timestamp{std::time(nullptr)};
		if (should_break) {
			break;
		}
		if (arg.is_vf) {
			std::string vf_bdf;
			std::tuple<std::string, std::string, std::string> indexes =
				getGpuVfIndexFromVfId(arg.vf_id);
			vf_bdf = std::get<2>(indexes).c_str();
			std::time_t timestamp{std::time(nullptr)};

			std::string timestamp_sec{string_format("%lld",timestamp)};
			first_row.push_back("timestamp");
			second_row.push_back(" ");
			third_row.push_back("seconds");
			fourth_row.push_back(timestamp_sec.c_str());

			first_row.push_back("gpu");
			second_row.push_back(" ");
			third_row.push_back("index");
			fourth_row.push_back(std::get<0>(indexes).c_str());

			first_row.push_back("vf");
			second_row.push_back(" ");
			third_row.push_back("index");
			fourth_row.push_back(std::get<1>(indexes).c_str());

			if (is_vf_guard_info || arg.all_arguments) {
				ret = metric_vf_command_guard(vf_bdf, formatted_string);
				std::string param{"guard"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					first_row.push_back("guard_flr");
					second_row.push_back("ie,gt,s,a,i,t,a");
					third_row.push_back("bool,str,str,int,int(ms),int,int");

					first_row.push_back("guard_exclusive_mode");
					second_row.push_back("ie,gt,s,a,i,t,a");
					third_row.push_back("bool,str,str,int,int(ms),int,int");


					first_row.push_back("guard_exclusive_timeout");
					second_row.push_back("ie,gt,s,a,i,t,a");
					third_row.push_back("bool, str, str, int, int (ms), int, int");


					first_row.push_back("guard_allowed_interrupt");
					second_row.push_back("ie,gt,s,a,i,t,a");
					third_row.push_back("bool, str, str, int, int (ms), int, int");


					std::vector<std::string> guest_data{split_string(formatted_string, ',')};
					fourth_row.push_back((guest_data[0] + "," + guest_data[1] + "," + guest_data[2] + "," +
										  guest_data[3]
										  + "," + guest_data[4] + "," + guest_data[5] + "," + guest_data[6]).c_str());
					fourth_row.push_back((guest_data[7] + "," + guest_data[8] + "," + guest_data[9] + "," +
										  guest_data[10]
										  + "," + guest_data[11] + "," + guest_data[12] + "," + guest_data[13]).c_str());
					fourth_row.push_back((guest_data[14] + "," + guest_data[15] + "," + guest_data[16] + "," +
										  guest_data[17]
										  + "," + guest_data[18] + "," + guest_data[19] + "," + guest_data[20]).c_str());
					fourth_row.push_back((guest_data[21] + "," + guest_data[22] + "," + guest_data[23] + "," +
										  guest_data[24]
										  + "," + guest_data[25] + "," + guest_data[26] + "," + guest_data[27]).c_str());
				}
			}

			if (is_vf_schedule || arg.all_arguments) {
				ret = metric_vf_command_schedule(vf_bdf, formatted_string);
				std::string param{"schedule"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					first_row.push_back("schedule");
					second_row.push_back("but,fc,s,lbs,lbe,lss,lse,st,lrs,lre,rt,cat,crt,tat,trt");
					third_row.push_back("date");
					fourth_row.push_back(formatted_string.c_str());
				}
			}

			if (is_vf_guest_data || arg.all_arguments) {
				ret = metric_vf_command_guest_data(vf_bdf, formatted_string);
				std::string param{"guest"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					first_row.push_back("guest_data");
					second_row.push_back("dv,fu");
					third_row.push_back("str, int(MB)");
					fourth_row.push_back(formatted_string.c_str());
				}
			}
			value_rows.push_back(fourth_row);
			fourth_row.clear();

		} else {
			std::time_t start_timestamp{std::time(nullptr)};
			for (unsigned int i = 0; i < arg.devices.size(); i++) {
				uint64_t gpu_handle = arg.devices[i]->get_bdf();
				std::time_t timestamp{std::time(nullptr)};
				std::string timestamp_sec{string_format("%lld",timestamp)};
				if (i == 0) {
					first_row.push_back("timestamp");
					second_row.push_back(" ");
					third_row.push_back("seconds");
					first_row.push_back("gpu");
					second_row.push_back(" ");
					third_row.push_back("index");
				}
				fourth_row.push_back(timestamp_sec.c_str());
				std::string gpu_index(string_format("%d", arg.devices[i]->get_gpu_index()));
				fourth_row.push_back(gpu_index.c_str());

				if ((std::find(arg.options.begin(), arg.options.end(), "usage") != arg.options.end()) ||
						(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end()) ||
						arg.all_arguments) {
					ret = metric_command_usage(gpu_handle, formatted_string);
					std::string param{"usage"};
					int error = handle_exceptions(ret, param, arg);
					if (error == 0) {
						if (i == 0) {
							first_row.push_back("usage");
							second_row.push_back("gu,mu,miu");
							third_row.push_back("%");
						}
						fourth_row.push_back(formatted_string.c_str());
					}
					formatted_string.clear();

				}

				if ((std::find(arg.options.begin(), arg.options.end(), "fb-usage") != arg.options.end()) ||
						(std::find(arg.options.begin(), arg.options.end(), "fb") != arg.options.end()) ||
						arg.all_arguments) {
					ret = metric_command_fb_usage(gpu_handle, formatted_string);
					std::string param{"fb-usage"};
					int error = handle_exceptions(ret, param, arg);
					if (error == 0) {
						if (i == 0) {
							first_row.push_back("fb-usage");
							second_row.push_back("vt,vu");
							third_row.push_back("MB");
						}
						fourth_row.push_back(formatted_string.c_str());
					}
					formatted_string.clear();
				}

				if ((std::find(arg.options.begin(), arg.options.end(), "power") != arg.options.end()) ||
						(std::find(arg.options.begin(), arg.options.end(), "p") !=  arg.options.end()) ||
						arg.all_arguments) {
					std::string param{"power"};
					ret = metric_command_power(gpu_handle, formatted_string);
					int error = handle_exceptions(ret, param, arg);
					if (error == 0) {
						if (i == 0) {
							first_row.push_back("power");
							if (AmdSmiPlatform::getInstance().is_host()) {
								second_row.push_back("asp,gv,sv,mv,pm");
								third_row.push_back("Watts,mV,mV,mV,str");
							} else {
								second_row.push_back("asp,gv,sv,mv");
								third_row.push_back("Watts,mV,mV,mV");
							}
						}
						fourth_row.push_back(formatted_string.c_str());
					}
					formatted_string.clear();
				}

				if ((std::find(arg.options.begin(), arg.options.end(), "clock") != arg.options.end()) ||
						(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end()) ||
						arg.all_arguments) {
					std::string param{"clock"};
					ret = metric_command_clock(gpu_handle, formatted_string);
					int error = handle_exceptions(ret, param, arg);
					if (error == 0) {
						std::vector<std::string> clocks{};
						clocks = split_string(formatted_string, ',');
						if (i == 0) {
							first_row.push_back("gfx_clock");
							first_row.push_back("mem_clock");
							first_row.push_back("vclk_0_clock");
							first_row.push_back("vclk_1_clock");
							second_row.push_back("gcc,gmc,gmc,gcl,gcds");
							second_row.push_back("mcc,mmc,mmc,mcl,mcds");
							second_row.push_back("vcc,vmc,vmc,vcl,vcds");
							second_row.push_back("vcc,vmc,vmc,vcl,vcds");
							third_row.push_back("MHz");
							third_row.push_back("MHz");
							third_row.push_back("MHz");
							third_row.push_back("MHz");
							first_row.push_back("dclk_0_clock");
							first_row.push_back("dclk_1_clock");
							second_row.push_back("dcc,dmc,dmc,dcl,dcds");
							second_row.push_back("dcc,dmc,dmc,dcl,dcds");
							third_row.push_back("MHz");
							third_row.push_back("MHz");
						}
						fourth_row.push_back((clocks[0] + "," + clocks[1] + "," + clocks[2] + "," + clocks[3] + "," +
											  clocks[4]).c_str());
						fourth_row.push_back((clocks[5] + "," + clocks[6] + "," + clocks[7] + "," + clocks[8] + "," +
											  clocks[9]).c_str());
						fourth_row.push_back((clocks[10] + "," + clocks[11] + "," + clocks[12] + "," + clocks[13] + "," +
											  clocks[14]).c_str());
						fourth_row.push_back((clocks[15] + "," + clocks[16] + "," + clocks[17] + "," + clocks[18] + "," +
											  clocks[19]).c_str());
						fourth_row.push_back((clocks[20] + "," + clocks[21] + "," + clocks[22] + "," + clocks[23] + "," +
												clocks[24]).c_str());
						fourth_row.push_back((clocks[25] + "," + clocks[26] + "," + clocks[27] + "," + clocks[28] + "," +
												clocks[29]).c_str());
					}
				}

				if ((std::find(arg.options.begin(), arg.options.end(), "temperature") != arg.options.end()) ||
						(std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end()) ||
						arg.all_arguments) {
					std::string param{"temperature"};
					ret = metric_command_temperature(gpu_handle, formatted_string);
					int error = handle_exceptions(ret, param, arg);
					if (error == 0) {
						if (i == 0) {
							first_row.push_back("temperature");
							second_row.push_back("et,ht,vt");
							third_row.push_back("Celsius");
						}
						fourth_row.push_back(formatted_string.c_str());
					}
				}
				if ((std::find(arg.options.begin(), arg.options.end(), "pcie") != arg.options.end()) ||
						(std::find(arg.options.begin(), arg.options.end(), "P") != arg.options.end()) ||
						arg.all_arguments) {
					ret = metric_command_pcie(gpu_handle, formatted_string);
					std::string param{"pcie"};
					int error = handle_exceptions(ret, param, arg);
					if (error == 0) {
						if (i == 0) {
							first_row.push_back("pcie");
							second_row.push_back("pcw,pcs");
							third_row.push_back("int(lanes), int(MT/s)");
						}
						fourth_row.push_back(formatted_string.c_str());
					}
				}
				if ((std::find(arg.options.begin(), arg.options.end(), "ecc") != arg.options.end()) ||
						(std::find(arg.options.begin(), arg.options.end(), "e") != arg.options.end()) ||
						arg.all_arguments) {
					ret = metric_command_ecc(gpu_handle, formatted_string);
					std::string param{"ecc"};
					int error = handle_exceptions(ret, param, arg);
					if (error == 0) {
						if (i == 0) {
							first_row.push_back("ecc");
							second_row.push_back("tc,tu,td,cc,cu");
							third_row.push_back("int");
						}
						fourth_row.push_back(formatted_string.c_str());
					}
				}
				value_rows.push_back(fourth_row);
				fourth_row.clear();
			}
		}

		int result_lines{num_of_lines(first_row, MAX_LINE_LENGTH)};
		std::vector<std::vector<std::string>> first_table(result_lines,
				std::vector<std::string>(first_row.size(), " "));

		for (int i = 0; i < first_row.size(); i++) {
			std::vector<std::string> res{split_string_by_size(first_row[i], MAX_LINE_LENGTH)};
			for (int j = 0; j < res.size(); j++) {
				first_table[j][i] = res[j];
			}
		}

		result_lines = num_of_lines(second_row, MAX_LINE_LENGTH);
		std::vector<std::vector<std::string>> second_table(result_lines,
				std::vector<std::string>(second_row.size(), " "));

		for (int i = 0; i < second_row.size(); i++) {
			std::vector<std::string> res{split_string_by_size(second_row[i], MAX_LINE_LENGTH)};
			for (int j = 0; j < res.size(); j++) {
				second_table[j][i] = res[j];
			}
		}

		result_lines = num_of_lines(third_row, MAX_LINE_LENGTH);
		std::vector<std::vector<std::string>> third_table(result_lines,
				std::vector<std::string>(third_row.size(), " "));

		for (int i = 0; i < third_row.size(); i++) {
			std::vector<std::string> res{split_string_by_size(third_row[i], MAX_LINE_LENGTH)};
			for (int j = 0; j < res.size(); j++) {
				third_table[j][i] = res[j];
			}
		}

		for (auto elem : first_table) {
			table.push_back(elem);
		}
		for (auto elem : second_table) {
			table.push_back(elem);
		}
		for (auto elem : third_table) {
			table.push_back(elem);
		}

		for (auto fourth_row : value_rows) {
			result_lines = num_of_lines(fourth_row, MAX_LINE_LENGTH);
			std::vector<std::vector<std::string>> fourth_table(result_lines,
					std::vector<std::string>(fourth_row.size(), " "));

			for (int i = 0; i < fourth_row.size(); i++) {
				std::vector<std::string> res{split_string_by_size(fourth_row[i], MAX_LINE_LENGTH)};
				for (int j = 0; j < res.size(); j++) {
					fourth_table[j][i] = res[j];
				}
			}

			for (auto elem : fourth_table) {
				table.push_back(elem);
			}
		}
		value_rows.clear();

		align_table(table);

		for (auto row : table) {
			for (auto elem : row) {
				out.append(elem.c_str());
			}
			out.append("\n");
		}

		while(std::difftime(std::time(nullptr), start_timestamp) < arg.watch) {}

		if (arg.is_file) {
			write_to_file(arg.file_path, out, true);
		} else {
			std::cout << out.c_str() << std::endl;
		}

		out.clear();
		first_row.clear();
		second_row.clear();
		third_row.clear();
		table.clear();

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

void AmdSmiMetricCommand::execute_command()
{
	bool should_wait = [](Arguments arg) {
		bool wait{false};
		if (AmdSmiPlatform::getInstance().is_baremetal() || AmdSmiPlatform::getInstance().is_guest()) {
			if (arg.all_arguments) {
				return true;
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "fb-usage") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "fb") != arg.options.end())) {
				return true;
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "usage") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end())) {
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

	if ((std::find(arg.options.begin(), arg.options.end(), "schedule") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "s") != arg.options.end())) {
		is_vf_schedule = true;
	}
	if ((std::find(arg.options.begin(), arg.options.end(), "guard") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "G") != arg.options.end())) {
		is_vf_guard_info = true;
	}
	if ((std::find(arg.options.begin(), arg.options.end(), "guest-data") != arg.options.end())
			|| (std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end())) {
		is_vf_guest_data = true;
	}

	if (arg.watch > -1) {
		if (arg.output == json) {
			throw SmiToolInvalidParameterException("--json");
		}
		if (arg.output == csv) {
			throw SmiToolInvalidParameterException("--csv");
		}
		std::cout << "If you want to exit looping, press 'CTRL' + 'C' \n\n";
		metric_command_watch();
	} else if (arg.output == json) {
		metric_command_json();
	} else if (arg.output == csv) {
		if ((arg.options.size() > 1 || arg.all_arguments) && AmdSmiPlatform::getInstance().is_mi300()) {
			std::string cmd_name{"metric --csv"};
			throw SmiToolCommandNotSupportedException(cmd_name);
		} else {
			metric_command_csv();
		}
	} else if (arg.output == human) {
		metric_command_human();
	}

};
