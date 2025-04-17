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
#include <sstream>
#include <regex>
#include <stdexcept>
#include <map>

#include "json/json.h"

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_static_command.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_api_base.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

auto constexpr vf_csv_header {"gpu,vf,fb_offset,fb_size,gfx_timeslice"};
auto constexpr
asic_csv_header {",asic_market_name,asic_vendor_id,asic_vendor_name,asic_subvendor_id,asic_device_id,asic_subsystem_id,asic_rev_id,asic_serial,oam_id"};
auto constexpr
bus_csv_header {",bus_bdf,max_pcie_width,max_pcie_speed,pcie_interface_version,slot_type,max_pcie_interface_version"};
auto constexpr vbios_csv_header {",vbios_name,vbios_build_date,vbios_part_number,vbios_version"};
auto constexpr
board_csv_header {",board_model_number,board_product_serial,board_fru_id,board_manufacturer_name,board_product_name"};
auto constexpr limit_csv_header {
	",max_power,min_power,socket_power,slowdown_edge_temperature,slowdown_hotspot_temperature,slowdown_mem_temperature,"
	"shutdown_edge_temperature,shutdown_hotspot_temperature,shutdown_mem_temperature"
};
auto constexpr driver_csv_header {",driver_name,driver_version,driver_date,driver_model"};
auto constexpr
ras_csv_header {",block,block_ecc_status,ras_eeprom_version,schema,schema_status"};
auto constexpr
dfc_ucode_csv_header {",version,gart_wr_guest_min,gart_wr_guest_max,dfc_fw_type,verification,customer_ordinal,white_list_latest,white_list_oldest,black_list_1,black_list_2,black_list_3,black_list_4"};
auto constexpr
fb_info_csv_header {",total_fb_size,pf_fb_reserved,pf_fb_offset,fb_alignment,max_vf_fb_usable,min_vf_fb_usable"};
auto constexpr num_vf_csv_header {",num_vf_supported,num_vf_enabled"};
auto constexpr vram_csv_header {",vram_type,vram_vendor,vram_size,vram_bit_width"};
auto constexpr vf_nested_csv_header {"gpu,vf,fb_offset,fb_size,gfx_timeslice"};
auto constexpr
header_cache {",cache,cache_properties,cache_size,cache_level,max_num_cu_shared,num_cache_instance"};
auto constexpr header_process_isolation {",process_isolation"};
auto constexpr header_static_partition {",accelerator_partition,memory_partition,partition_id"};
auto constexpr header_soc_pstate {",num_supported,current_id,policy_id,policy_description"};

int AmdSmiStaticCommand::static_command_asic(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_asic_info_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_bus(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bus_info_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_vbios(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vbios_info_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_board_host(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_board_info_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_limit(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_limit_info_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_driver(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_driver_info_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_ras_host(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_ras_info_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_dfc(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_dfc_info_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_fb_info(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_fb_info_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_num_vf(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_num_vf_command(processor, arg,
			  formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_vram(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vram_info_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_cache(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_cache_info_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_process_isolation(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_process_isolation(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_partition(uint64_t processors,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_static_partition_command(processors,
			  arg,
			  formatted_string);
	return ret;
}
int AmdSmiStaticCommand::static_command_soc_pstate(uint64_t processor,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_soc_pstate(processor,
			  arg, formatted_string);
	return ret;
}


void AmdSmiStaticCommand::static_command_json()
{
	int ret;
	unsigned int i;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	std::string out{};
	std::string result{};

	if (arg.is_vf) {
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		out += string_format(
				   vfNestedTemplate, std::get<0>(indexes).c_str(),
				   std::get<1>(indexes).c_str());
		vf_bdf = std::get<2>(indexes).c_str();
		ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vf_info_static_command(vf_bdf,
			  arg, out);
		json = nlohmann::ordered_json::parse(out);
		json_format.insert(json_format.end(), json);
		out = json_format.dump(4);
		if (arg.is_file) {
			write_to_file(arg.file_path, out);
		} else {
			std::cout << out << std::endl;
		}
	} else {
		for (i = 0; i < arg.devices.size(); i++) {
			json = {};
			json["gpu"] = arg.devices[i]->get_gpu_index();
			nlohmann::ordered_json values_json;
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();

			if ((std::find(arg.options.begin(), arg.options.end(), "asic") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "a") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"asic"};
				ret = static_command_asic(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["asic"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "bus") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"bus"};
				ret = static_command_bus(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["bus"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "vbios") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "V") !=  arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"vbios"};
				ret = static_command_vbios(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["vbios"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "limit") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "l") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"limit"};
				ret = static_command_limit(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["limit"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "driver") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "d") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"driver"};
				ret = static_command_driver(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["driver"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "board") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "B") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"board"};
				ret = static_command_board_host(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["board"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "ras") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "r") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"ras"};
				ret = static_command_ras_host(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["ras"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "dfc-ucode") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "D") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"dfc-ucode"};
				ret = static_command_dfc(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["dfc"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "fb-info") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "f") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"fb-info"};
				ret = static_command_fb_info(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["fb_info"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "num-vf") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "n") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"num-vf"};
				ret = static_command_num_vf(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["num-vf"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "vram") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "v") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"vram"};
				ret = static_command_vram(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["vram"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "cache") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"cache"};
				ret = static_command_cache(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["cache_info"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "soc-pstate") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "ps") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"soc-pstate"};
				ret = static_command_soc_pstate(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["soc_pstate"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "partition") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"partition"};
				ret = static_command_partition(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					json["partition"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "process-isolation") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "R") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"process-isolation"};
				ret = static_command_process_isolation(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					json["process_isolation"] = out;
					out.clear();
				}
				out.clear();
			}
			json_format.insert(json_format.end(), json);
		}

		result = json_format.dump(4);
		if (arg.is_file) {
			write_to_file(arg.file_path, result);
		} else {
			std::cout << std::setw(4) << result << '\n';
		}
	}
}


void AmdSmiStaticCommand::static_command_human()
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
		ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vf_info_static_command(vf_bdf,
			  arg, out);
	} else {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();

			if ((std::find(arg.options.begin(), arg.options.end(), "asic") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "a") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_asic(gpu_bdf, formatted_string);
				std::string param{"asic"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "bus") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_bus(gpu_bdf, formatted_string);
				std::string param{"bus"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "vbios") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "V") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_vbios(gpu_bdf, formatted_string);
				std::string param{"vbios"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "limit") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "l") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_limit(gpu_bdf, formatted_string);
				std::string param{"limit"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "driver") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "d") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_driver(gpu_bdf, formatted_string);
				std::string param{"driver"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "board") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "B") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_board_host(gpu_bdf, formatted_string);
				std::string param{"board"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "ras") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "r") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_ras_host(gpu_bdf, formatted_string);
				std::string param{"ras"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "dfc-ucode") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "D") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_dfc(gpu_bdf, formatted_string);
				std::string param{"dfc-ucode"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "fb-info") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "f") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_fb_info(gpu_bdf, formatted_string);
				std::string param{"fb-info"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "num-vf") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "n") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_num_vf(gpu_bdf, formatted_string);
				std::string param{"num-vf"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "vram") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "v") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_vram(gpu_bdf, formatted_string);
				std::string param{"vram"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "cache") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_cache(gpu_bdf, formatted_string);
				std::string param{"cache"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "soc-pstate") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "ps") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_soc_pstate(gpu_bdf, formatted_string);
				std::string param{"soc-pstate"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "partition") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_partition(gpu_bdf, formatted_string);
				std::string param{"partition"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "process-isolation") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "R") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_process_isolation(gpu_bdf, formatted_string);
				std::string param{"process-isolation"};
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

void AmdSmiStaticCommand::static_command_csv()
{
	int ret;
	std::string header{};
	std::string formatted_string{};
	std::string values{};
	std::string out{};

	std::vector<std::vector<std::string>> results;
	std::string output_buffer{};

	if (arg.is_vf) {
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		formatted_string += string_format(
								"%s,%s", std::get<0>(indexes).c_str(),
								std::get<1>(indexes).c_str());
		vf_bdf = std::get<2>(indexes).c_str();
		ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vf_info_static_command(vf_bdf,
			  arg, formatted_string);
		header.append(vf_nested_csv_header);
		values.append(formatted_string);
		out.append(header).append("\n");
		out.append(values).append("\n");
	} else {
		header.append("gpu");
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			std::string gpu_id{string_format("%d",i)};
			results.push_back({gpu_id});

			if ((std::find(arg.options.begin(), arg.options.end(), "asic") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "a") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_asic(gpu_bdf, formatted_string);
				std::string param{"asic"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(asic_csv_header);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "bus") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_bus(gpu_bdf, formatted_string);
				std::string param{"bus"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(bus_csv_header);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "vbios") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "V") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_vbios(gpu_bdf, formatted_string);
				std::string param{"vbios"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(vbios_csv_header);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "limit") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "l") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_limit(gpu_bdf, formatted_string);
				std::string param{"limit"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(limit_csv_header);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "driver") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "d") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_driver(gpu_bdf, formatted_string);
				std::string param{"driver"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(driver_csv_header);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "board") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "B") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_board_host(gpu_bdf, formatted_string);
				std::string param{"board"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(board_csv_header);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "ras") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "r") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_ras_host(gpu_bdf, formatted_string);
				std::string param{"ras"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(ras_csv_header);
					std::vector<std::string> ras_data{split_string(formatted_string, '\n')};
					std::vector<std::string> ras_rows{};
					while (!ras_data.empty()) {
						std::string first{};
						first = ras_data.front();
						ras_data.erase(ras_data.begin());
						ras_rows.push_back(first);
					}
					results.push_back(ras_rows);
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "dfc-ucode") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "D") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_dfc(gpu_bdf, formatted_string);
				std::string param{"dfc-ucode"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(dfc_ucode_csv_header);
					std::vector<std::string> dfc_data{split_string(formatted_string, '\n')};
					std::vector<std::string> dfc_rows{};
					while (!dfc_data.empty()) {
						std::string first{};
						first = dfc_data.front();
						dfc_data.erase(dfc_data.begin());
						dfc_rows.push_back(first);
					}
					results.push_back(dfc_rows);
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "fb-info") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "f") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_fb_info(gpu_bdf, formatted_string);
				std::string param{"fb-info"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(fb_info_csv_header);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "num-vf") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "n") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_num_vf(gpu_bdf, formatted_string);
				std::string param{"num-vf"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(num_vf_csv_header);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "vram") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "v") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_vram(gpu_bdf, formatted_string);
				std::string param{"vram"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(vram_csv_header);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "cache") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "c") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_cache(gpu_bdf, formatted_string);
				std::string param{"cache"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(header_cache);
					std::vector<std::string> cache_data{split_string(formatted_string, '\n')};
					std::vector<std::string> cache_rows{};
					while (!cache_data.empty()) {
						std::string first{};
						first = cache_data.front();
						cache_data.erase(cache_data.begin());
						cache_rows.push_back(first);
					}
					results.push_back(cache_rows);
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "soc-pstate") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "ps") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_soc_pstate(gpu_bdf, formatted_string);
				std::string param{"soc-pstate"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(header_soc_pstate);
					std::vector<std::string> dpm_data{split_string(formatted_string, '\n')};
					std::vector<std::string> policy_rows{};
					while (!dpm_data.empty()) {
						std::string first{};
						first = dpm_data.front();
						dpm_data.erase(dpm_data.begin());
						policy_rows.push_back(first);
					}
					results.push_back(policy_rows);
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "partition") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_partition(gpu_bdf, formatted_string);
				std::string param{"partition"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(header_static_partition);
					std::vector<std::string> partition_data{split_string(formatted_string, '\n')};
					std::vector<std::string> partition_rows{};
					while (!partition_data.empty()) {
						std::string first{};
						first = partition_data.front();
						partition_data.erase(partition_data.begin());
						partition_rows.push_back(first);
					}
					results.push_back(partition_rows);
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "process-isolation") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "R") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_process_isolation(gpu_bdf, formatted_string);
				std::string param{"process-isolation"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(header_process_isolation);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if (i == 0) {
				out.append(header);
				out.append("\n");
			}
			csv_recursion(output_buffer, results);
			out.append(output_buffer);
			results.clear();
			output_buffer.clear();
		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
	out.clear();
	header.clear();
}

void AmdSmiStaticCommand::execute_command()
{
	if (arg.output == json) {
		static_command_json();
	}
	if (arg.output == csv) {
		if (arg.all_arguments || arg.options.size() > 1) {
			std::cout << "FETCHING THE DATA...This may take a while...\n" << std::endl;
		}
		static_command_csv();
	}
	if (arg.output == human) {
		static_command_human();
	}
};
