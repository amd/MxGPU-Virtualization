/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
asic_csv_header {",asic_market_name,asic_vendor_id,asic_vendor_name,asic_subvendor_id,asic_device_id,asic_subsystem_id,asic_rev_id,asic_serial,oam_id,num_of_compute_units"};
auto constexpr
bus_csv_header {",bus_bdf,max_pcie_width,max_pcie_speed,pcie_interface_version,slot_type,max_pcie_interface_version"};
auto constexpr
bus_csv_header_pcie {",bus_bdf,max_pcie_width,max_pcie_speed,pcie_interface_version,slot_type,max_pcie_interface_version,pcie_level,pcie_speed,pcie_width"};
auto constexpr ifwi_csv_header {",ifwi_name,ifwi_build_date,ifwi_part_number,ifwi_version,ifwi_boot_firmware"};
auto constexpr
board_csv_header {",board_model_number,board_product_serial,board_fru_id,board_manufacturer_name,board_product_name"};
auto constexpr limit_csv_header {
	",max_power_ppt0,min_power_ppt0,socket_power_ppt0,max_power_ppt1,min_power_ppt1,socket_power_ppt1,"
	"slowdown_edge_temperature,slowdown_hotspot_temperature,slowdown_mem_temperature,"
	"shutdown_edge_temperature,shutdown_hotspot_temperature,shutdown_mem_temperature"
};
auto constexpr driver_csv_header {",driver_name,driver_version,driver_date,driver_model"};
auto constexpr
ras_csv_header {",block,block_ecc_status,ras_eeprom_version,bad_page_threshold,schema,schema_status"};
auto constexpr
dfc_ucode_csv_header {",version,gart_wr_guest_min,gart_wr_guest_max,dfc_fw_type,verification,customer_ordinal,white_list_latest,white_list_oldest,black_list_1,black_list_2,black_list_3,black_list_4"};
auto constexpr
fb_info_csv_header {",total_fb_size,pf_fb_reserved,pf_fb_offset,fb_alignment,max_vf_fb_usable,min_vf_fb_usable"};
auto constexpr num_vf_csv_header {",num_vf_supported,num_vf_enabled"};
auto constexpr vram_csv_header {",vram_type,vram_vendor,vram_size,vram_bit_width,vram_max_bandwidth"};
auto constexpr vf_nested_csv_header {",fb_offset,fb_size,gfx_timeslice"};
auto constexpr vf_hbm_info_csv_header {",phy_addr,phy_size,numa_id,name"};
auto constexpr
header_cache {",cache,cache_properties,cache_size,cache_level,max_num_cu_shared,num_cache_instance"};
auto constexpr header_process_isolation {",process_isolation"};
auto constexpr header_static_partition {",accelerator_partition,memory_partition,partition_id"};
auto constexpr header_soc_pstate {",num_supported,current_id,policy_id,policy_description"};
auto constexpr header_virtualization_mode {",mode"};
auto constexpr header_numa {",numa_node,numa_cpu_affinity_list,numa_cpu_affinity_bitmask,numa_cpu_affinity_core_range,numa_socket_affinity"};
auto constexpr header_xgmi_plpd {",num_supported,current_id,policy_id,policy_description"};

int AmdSmiStaticCommand::static_command_nic_asic(uint64_t processors,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_nic_asic_info_command(processors,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_nic_bus(uint64_t processors,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_nic_bus_info_command(processors,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_nic_driver(uint64_t processors,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_nic_driver_info_command(processors,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_nic_numa(uint64_t processors,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_nic_numa_info_command(processors,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_nic_port(uint64_t processors,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_nic_port_info_command(processors,
			  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_nic_rdma_devices(uint64_t processors,
		std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_nic_rdma_devices_info_command(processors,
			  arg, formatted_string);
	return ret;
}

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

int AmdSmiStaticCommand::static_command_vf_hbm_info(std::string vf_handle, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vf_hbm_info_command(vf_handle, arg,
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

int AmdSmiStaticCommand::static_command_virtualization_mode(uint64_t processor,
	std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_virtualization_mode_command(processor,
		  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_numa(uint64_t processor,
					std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_numa_command(processor,
		  arg, formatted_string);
	return ret;
}

int AmdSmiStaticCommand::static_command_xgmi_plpd(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_plpd(processor,
			  arg, formatted_string);
	return ret;
}

void AmdSmiStaticCommand::static_command_json()
{
	int ret;
	unsigned int i;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	nlohmann::ordered_json option_json;
	std::string out{};
	std::string result{};

	if (arg.is_vf) {
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		vf_bdf = std::get<2>(indexes).c_str();
		uint64_t gpu_index = static_cast<uint64_t>(std::stoi(std::get<0>(indexes)));
		uint64_t vf_index = static_cast<uint64_t>(std::stoi(std::get<1>(indexes)));

		json = {};
		json["gpu"] = gpu_index;
		json["vf"] = vf_index;

		if ((std::find(arg.options.begin(), arg.options.end(), "fb-info") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "f") != arg.options.end()) ||
				arg.all_arguments) {
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vf_info_static_command(vf_bdf,
				  arg, out);
			if (ret == 0) {
				nlohmann::ordered_json values_json = nlohmann::ordered_json::parse(out);
				if (values_json.contains("fb_info")) {
					json["fb_info"] = values_json["fb_info"];
				}
			}
			out.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "hbm-info") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "hbm") != arg.options.end()) ||
				arg.all_arguments) {
			ret = static_command_vf_hbm_info(vf_bdf, out);
			std::string param{"hbm-info"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				nlohmann::ordered_json values_json = nlohmann::ordered_json::parse(out);
				if (values_json.contains("hbm_info")) {
					json["hbm_info"] = values_json["hbm_info"];
				}
			}
			out.clear();
		}

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
			option_json = {};
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
					option_json["asic"] = values_json;
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
					option_json["bus"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "vbios") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "V") !=  arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "ifwi") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "I") !=  arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"ifwi"};
				ret = static_command_vbios(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["ifwi"] = values_json;
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
					option_json["limit"] = values_json;
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
					option_json["driver"] = values_json;
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
					option_json["board"] = values_json;
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
					option_json["ras"] = values_json;
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
					option_json["dfc"] = values_json;
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
					option_json["fb_info"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "num-vf") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "nv") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"num-vf"};
				ret = static_command_num_vf(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["num-vf"] = values_json;
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
					option_json["vram"] = values_json;
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
					option_json["cache_info"] = values_json;
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
					option_json["soc_pstate"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "xgmi-plpd") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "pd") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"xgmi-plpd"};
				ret = static_command_xgmi_plpd(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["xgmi-plpd"] = values_json;
					out.clear();
				} else if (error == COMMAND_NOT_SUPPORTED_AND_ALL_ARGS) {
					out.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "partition") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"partition"};
				ret = static_command_partition(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["partition"] = values_json;
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
					option_json["process_isolation"] = out;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "virtualization-mode") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "m") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"virtualization-mode"};
				ret = static_command_virtualization_mode(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					option_json["virtualization_mode"] = out;
					out.clear();
				} else if (error == COMMAND_NOT_SUPPORTED_AND_ALL_ARGS) {
					out.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "numa") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"numa"};
				ret = static_command_numa(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					nlohmann::ordered_json json_numa = nlohmann::ordered_json::parse(out);
					option_json["numa"] = json_numa;
					out.clear();
				} else if (error == COMMAND_NOT_SUPPORTED_AND_ALL_ARGS) {
					out.clear();
				}
			}
			if (!option_json.empty()) {
				json["gpu"] = arg.devices[i]->get_gpu_index();
				for (auto& [key, value] : option_json.items()) {
					json[key] = value;
				}
				json_format.insert(json_format.end(), json);
			}
		}
		for (i = 0; i < arg.nic_devices.size(); i++) {
			json = {};
			option_json = {};
			nlohmann::ordered_json values_json;
			uint64_t nic_bdf = arg.nic_devices[i]->get_bdf();

			if ((std::find(arg.options.begin(), arg.options.end(), "asic") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "a") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"asic"};
				ret = static_command_nic_asic(nic_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["asic"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "bus") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"bus"};
				ret = static_command_nic_bus(nic_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["bus"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "driver") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "d") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"driver"};
				ret = static_command_nic_driver(nic_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["driver"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "numa") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"numa"};
				ret = static_command_nic_numa(nic_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["numa"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "port") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "po") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"port"};
				ret = static_command_nic_port(nic_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["ports"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "rdma-devices") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "rd") != arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"rdma-devices"};
				ret = static_command_nic_rdma_devices(nic_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					option_json["rdma_devices"] = values_json;
					out.clear();
				}
				out.clear();
			}
			if (!option_json.empty()) {
				json["nic"] = arg.nic_devices[i]->get_gpu_index();
				for (auto& [key, value] : option_json.items()) {
					json[key] = value;
				}
				json_format.insert(json_format.end(), json);
			}
			option_json = {};
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
	std::string options_string{};
	std::string out{};

	if (arg.is_vf) {
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		out += string_format(
				   vfNestedTemplate, std::get<0>(indexes).c_str(),
				   std::get<1>(indexes).c_str());
		vf_bdf = std::get<2>(indexes).c_str();

		if ((std::find(arg.options.begin(), arg.options.end(), "fb-info") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "f") != arg.options.end()) ||
				arg.all_arguments) {
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vf_info_static_command(vf_bdf,
				  arg, formatted_string);
			out += formatted_string;
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "hbm-info") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "hbm") != arg.options.end()) ||
				arg.all_arguments) {
			ret = static_command_vf_hbm_info(vf_bdf, formatted_string);
			std::string param{"hbm-info"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}
	} else {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();

			if ((std::find(arg.options.begin(), arg.options.end(), "asic") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "a") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_asic(gpu_bdf, formatted_string);
				std::string param{"asic"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
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
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "vbios") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "V") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "ifwi") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "I") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_vbios(gpu_bdf, formatted_string);
				std::string param{"ifwi"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
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
					options_string += formatted_string;
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
					options_string += formatted_string;
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
					options_string += formatted_string;
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
					options_string += formatted_string;
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
					options_string += formatted_string;
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
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "num-vf") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "nv") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_num_vf(gpu_bdf, formatted_string);
				std::string param{"num-vf"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
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
					options_string += formatted_string;
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
					options_string += formatted_string;
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
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "xgmi-plpd") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "pd") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_xgmi_plpd(gpu_bdf, formatted_string);
				std::string param{"xgmi-plpd"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
					formatted_string.clear();
				} else if (error == COMMAND_NOT_SUPPORTED_AND_ALL_ARGS) {
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
					options_string += formatted_string;
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
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "virtualization-mode") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "m") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_virtualization_mode(gpu_bdf, formatted_string);
				std::string param{"virtualization_mode"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "numa") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_numa(gpu_bdf, formatted_string);
				std::string param{"numa"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if (!options_string.empty()) {
				out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
				out += options_string;
				options_string.clear();
			}
			options_string.clear();
		}
		for (unsigned int i = 0; i < arg.nic_devices.size(); i++) {
			int nic_index = arg.nic_devices[i]->get_gpu_index();
			uint64_t nic_bdf = arg.nic_devices[i]->get_bdf();
			if ((std::find(arg.options.begin(), arg.options.end(), "asic") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "a") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_nic_asic(nic_bdf, formatted_string);
				std::string param{"nic_asic"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "bus") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_nic_bus(nic_bdf, formatted_string);
				std::string param{"nic_bus"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "driver") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "d") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_nic_driver(nic_bdf, formatted_string);
				std::string param{"nic_driver"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "numa") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_nic_numa(nic_bdf, formatted_string);
				std::string param{"nic_numa"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "port") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "po") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_nic_port(nic_bdf, formatted_string);
				std::string param{"nic_port"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "rdma-devices") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "rd") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_nic_rdma_devices(nic_bdf, formatted_string);
				std::string param{"nic_rdma_devices"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					options_string += formatted_string;
				}
				formatted_string.clear();
			}
			if (!options_string.empty()) {
				out += string_format("NIC: %d\n", nic_index);
				out += options_string;
				options_string.clear();
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
		vf_bdf = std::get<2>(indexes).c_str();

		header.append("gpu,vf");
		std::string gpu_vf_str = string_format("%s,%s", std::get<0>(indexes).c_str(),
											   std::get<1>(indexes).c_str());
		results.push_back({gpu_vf_str});

		if ((std::find(arg.options.begin(), arg.options.end(), "fb-info") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "f") != arg.options.end()) ||
				arg.all_arguments) {
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vf_info_static_command(vf_bdf,
				  arg, formatted_string);
			header.append(vf_nested_csv_header);
			results.push_back({formatted_string});
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "hbm-info") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "hbm") != arg.options.end()) ||
				arg.all_arguments) {
			ret = static_command_vf_hbm_info(vf_bdf, formatted_string);
			std::string param{"hbm-info"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				header.append(vf_hbm_info_csv_header);
				results.push_back({formatted_string});
			}
			formatted_string.clear();
		}

		out.append(header);
		out.append("\n");
		csv_recursion(output_buffer, results);
		out.append(output_buffer);
		results.clear();
		output_buffer.clear();
	} else {
		header.append("gpu");
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			int gpu_id = arg.devices[i]->get_gpu_index();

			std::string gpu_id_str{string_format("%d",gpu_id)};
			results.push_back({gpu_id_str});

			if ((std::find(arg.options.begin(), arg.options.end(), "asic") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "a") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_asic(gpu_bdf, formatted_string);
				std::string param{"asic"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(asic_csv_header);
					if (!AmdSmiPlatform::getInstance().is_host()) {
						header.append(",target_graphics_version");
					}
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
					if (formatted_string.find('\n') != std::string::npos) {
						header.append(bus_csv_header_pcie);
						std::vector<std::string> bus_data{split_string(formatted_string, '\n')};
						std::vector<std::string> bus_rows{};
						while (!bus_data.empty()) {
							std::string first{};
							first = bus_data.front();
							bus_data.erase(bus_data.begin());
							bus_rows.push_back(first);
						}
						results.push_back(bus_rows);
					} else {
						header.append(bus_csv_header);
						results.push_back({formatted_string});
					}
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "vbios") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "V") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "ifwi") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "I") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_vbios(gpu_bdf, formatted_string);
				std::string param{"ifwi"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(ifwi_csv_header);
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
					if(arg.ptl_supported) {
						header.append(",ptl,ptl_format");
					}
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
					(std::find(arg.options.begin(), arg.options.end(), "nv") != arg.options.end()) ||
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
			if ((std::find(arg.options.begin(), arg.options.end(), "xgmi-plpd") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "pd") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_xgmi_plpd(gpu_bdf, formatted_string);
				std::string param{"xgmi-plpd"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(header_xgmi_plpd);
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
			if ((std::find(arg.options.begin(), arg.options.end(), "virtualization-mode") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "m") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_virtualization_mode(gpu_bdf, formatted_string);
				std::string param{"virtualization-mode"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(header_virtualization_mode);
					results.push_back({formatted_string});
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "numa") != arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "u") != arg.options.end()) ||
					arg.all_arguments) {
				ret = static_command_numa(gpu_bdf, formatted_string);
				std::string param{"numa"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					header.append(header_numa);
					std::vector<std::string> numa_data{split_string(formatted_string, '\n')};
					std::vector<std::string> numa_rows{};
					while (!numa_data.empty()) {
						std::string first{};
						first = numa_data.front();
						numa_data.erase(numa_data.begin());
						numa_rows.push_back(first);
					}
					results.push_back(numa_rows);
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
