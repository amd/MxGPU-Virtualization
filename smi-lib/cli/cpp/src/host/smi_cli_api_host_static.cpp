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
#include "amdsmi.h"
#include "smi_cli_api_host.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_platform.h"

#include "json/json.h"

#include <map>
#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif
#include <limits.h>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ASIC_INFO)(amdsmi_processor_handle,
		amdsmi_asic_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_FB_LAYOUT)(amdsmi_processor_handle,
		amdsmi_pf_fb_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VBIOS_INFO)(amdsmi_processor_handle,
		amdsmi_vbios_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_BOARD_INFO)(amdsmi_processor_handle,
		amdsmi_board_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_CAP_INFO)(amdsmi_processor_handle, uint32_t,
		amdsmi_power_cap_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
		amdsmi_temperature_type_t,
		amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_INFO)(amdsmi_processor_handle,
		amdsmi_driver_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_MODEL)(amdsmi_processor_handle,
		amdsmi_driver_model_type_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_FEATURE_INFO)(amdsmi_processor_handle,
		amdsmi_ras_feature_t *);
typedef amdsmi_status_t (*AMDSMI_GET_DFC_FW_TABLE)(amdsmi_processor_handle, amdsmi_dfc_fw_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint32_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VRAM_INFO)(amdsmi_processor_handle,
		amdsmi_vram_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_CACHE_INFO)(amdsmi_processor_handle,
		amdsmi_gpu_cache_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_SOC_PSTATE)(amdsmi_processor_handle,
		amdsmi_dpm_policy_t *);
typedef amdsmi_status_t (*AMDSMI_SET_SOC_PSTATE)(amdsmi_processor_handle,
		uint32_t);
typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_vf_handle_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_INFO)(amdsmi_vf_handle_t, amdsmi_vf_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_COUNT)(amdsmi_processor_handle, amdsmi_gpu_block_t,
		amdsmi_error_count_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_ENABLED)(amdsmi_processor_handle,
		uint64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CURR_ACCELERATOR_PARTITION)(amdsmi_processor_handle,
		amdsmi_accelerator_partition_profile_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_MEMORY_PARTITION_CONFIG)(amdsmi_processor_handle,
		amdsmi_memory_partition_config_t *);


extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;

extern AMDSMI_GET_GPU_ASIC_INFO host_amdsmi_get_gpu_asic_info;
extern AMDSMI_GET_PCIE_INFO host_amdsmi_get_pcie_info;
extern AMDSMI_GET_FB_LAYOUT host_amdsmi_get_fb_layout;
extern AMDSMI_GET_GPU_VBIOS_INFO host_amdsmi_get_gpu_vbios_info;
extern AMDSMI_GET_GPU_BOARD_INFO host_amdsmi_get_gpu_board_info;

extern AMDSMI_GET_POWER_CAP_INFO host_amdsmi_get_power_cap_info;
extern AMDSMI_GET_TEMP_METRIC host_amdsmi_get_temp_metric;
extern AMDSMI_GET_GPU_DRIVER_INFO host_amdsmi_get_gpu_driver_info;
extern AMDSMI_GET_GPU_DRIVER_MODEL host_amdsmi_get_gpu_driver_model;
extern AMDSMI_GET_GPU_RAS_FEATURE_INFO host_amdsmi_get_gpu_ras_feature_info;
extern AMDSMI_GET_DFC_FW_TABLE host_amdsmi_get_dfc_fw_table;

extern AMDSMI_GET_NUM_VF host_amdsmi_get_num_vf;
extern AMDSMI_GET_GPU_VRAM_INFO host_amdsmi_get_gpu_vram_info;
extern AMDSMI_GET_GPU_CACHE_INFO host_amdsmi_get_gpu_cache_info;
extern AMDSMI_GET_SOC_PSTATE host_amdsmi_get_soc_pstate;
extern AMDSMI_SET_SOC_PSTATE host_amdsmi_set_soc_pstate;

extern AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
extern AMDSMI_GET_VF_INFO host_amdsmi_get_vf_info;

extern AMDSMI_GET_GPU_ECC_ENABLED host_amdsmi_get_gpu_ecc_enabled;

extern AMDSMI_GET_CURR_ACCELERATOR_PARTITION host_amdsmi_get_partition_profile;
extern AMDSMI_GET_MEMORY_PARTITION_CONFIG host_amdsmi_get_gpu_memory_partition_config;

const std::vector<amdsmi_gpu_block_t> ecc_blocks{AMDSMI_GPU_BLOCK_UMC, AMDSMI_GPU_BLOCK_SDMA, AMDSMI_GPU_BLOCK_GFX, AMDSMI_GPU_BLOCK_MMHUB,
		  AMDSMI_GPU_BLOCK_ATHUB, AMDSMI_GPU_BLOCK_PCIE_BIF, AMDSMI_GPU_BLOCK_HDP, AMDSMI_GPU_BLOCK_XGMI_WAFL,
		  AMDSMI_GPU_BLOCK_DF, AMDSMI_GPU_BLOCK_SMN, AMDSMI_GPU_BLOCK_SEM, AMDSMI_GPU_BLOCK_MP0,
		  AMDSMI_GPU_BLOCK_MP1, AMDSMI_GPU_BLOCK_FUSE, AMDSMI_GPU_BLOCK_MCA, AMDSMI_GPU_BLOCK_VCN,
		  AMDSMI_GPU_BLOCK_JPEG, AMDSMI_GPU_BLOCK_IH, AMDSMI_GPU_BLOCK_MPIO};

std::string host_fill_asic_info(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json asic_json = { { "market_name", value.c_str() },
			{ "vendor_id", value.c_str() },
			{ "vendor_name", value.c_str() },
			{ "subvendor_id", value.c_str() },
			{ "device_id", value.c_str() },
			{ "subsystem_id", value.c_str() },
			{ "rev_id", value.c_str() },
			{ "asic_serial", value.c_str() },
			{ "oam_id", value.c_str() }
		};

		out = asic_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format("%s,%s,%s,%s,%s,%s",value.c_str(),
							value.c_str(),"N/A", "N/A",value.c_str(), value.c_str(),
							value.c_str(), value.c_str());
	} else {
		out = string_format(
				  staticAsicTemplate, value.c_str(), value.c_str(),"N/A", "N/A",
				  value.c_str(), value.c_str(), value.c_str(), value.c_str());
	}

	return out;
}

std::string host_fill_vbios_info(Arguments arg, std::string value)
{
	std::string formatted_string{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json vbios_json = { { "name", value.c_str() },
			{ "build_date", value.c_str() },
			{ "part_number", value.c_str() },
			{ "version", value.c_str() }
		};

		formatted_string = vbios_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s", value.c_str(), value.c_str(),
							   value.c_str(), value.c_str());
	} else {
		formatted_string = string_format(
							   staticVbiosTemplate, value.c_str(), value.c_str(),
							   value.c_str(), value.c_str());
	}
	return formatted_string;
}


std::string host_fill_board_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json board_json = { { "model_number", value.c_str() },
			{ "product_serial", value.c_str() },
			{ "fru_id", value.c_str() },
			{ "product_name", value.c_str() },
			{ "manufacturer_name", value.c_str() }
		};

		out = board_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s",
				  value.c_str(), value.c_str(),
				  value.c_str(),
				  value.c_str(),
				  value.c_str());
	} else {
		out = string_format(
				  staticBoardTemplate, value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str());
	}

	return out;
}

std::string host_fill_driver_info(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json driver_json = { { "driver_name", value.c_str() }, { "driver_version", value.c_str()},
			{ "driver_date", value.c_str() }, {"driver_model", value.c_str() }
		};

		out = driver_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s", value.c_str(), value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(
				  driverHostInfoTemplate, value.c_str(), value.c_str(), value.c_str(), value.c_str());
	}

	return out;
}

std::string host_fill_bus_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json max_pcie_speed{};
		max_pcie_speed["value"] = value.c_str();
		max_pcie_speed["unit"] = value.c_str();
		nlohmann::ordered_json bus_json = { { "bdf", value.c_str() },
			{ "max_pcie_width", value.c_str() },
			{ "max_pcie_speed",  max_pcie_speed},
			{ "pcie_interface_version", value.c_str() },
			{ "slot_type", value.c_str() },
			{ "max_pcie_interface_version", value.c_str() }
		};

		out = bus_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s", value.c_str(),
							value.c_str(), value.c_str(),
							"N/A", "N/A", "N/A");
	} else {
		out = string_format(
				  staticBusTemplate, value.c_str(), value.c_str(),
				  value.c_str(), "N/A", "N/A", "N/A", "N/A");
	}

	return out;
}

std::string host_fill_ras_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json feature_json = {
			{ "ras_eeprom_version", value.c_str() },
			{ "supported_ecc_correction_schema", value.c_str() }
		};

		nlohmann::ordered_json ras_json;
		ras_json["feature"] = feature_json;

		out = ras_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s", value.c_str(), value.c_str());
	} else {
		out = string_format(
				  staticRasTemplateHost, value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str() );
	}

	return out;
}

std::string host_fill_fb_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json total_fb_size{};
		total_fb_size["value"] = value.c_str();
		total_fb_size["unit"] = "N/A";
		nlohmann::ordered_json pf_fb_reserved{};
		pf_fb_reserved["value"] = value.c_str();
		pf_fb_reserved["unit"] = "N/A";
		nlohmann::ordered_json pf_fb_offset{};
		pf_fb_offset["value"] = value.c_str();
		pf_fb_offset["unit"] = "N/A";
		nlohmann::ordered_json fb_alignment{};
		fb_alignment["value"] = value.c_str();
		fb_alignment["unit"] = "N/A";
		nlohmann::ordered_json max_vf_fb_usable{};
		max_vf_fb_usable["value"] = value.c_str();
		max_vf_fb_usable["unit"] = "N/A";
		nlohmann::ordered_json min_vf_fb_usable{};
		min_vf_fb_usable["value"] = value.c_str();
		min_vf_fb_usable["unit"] = "N/A";

		nlohmann::ordered_json fb_info_json = {
			{ "total_fb_size", total_fb_size },
			{ "pf_fb_reserved", pf_fb_reserved },
			{ "pf_fb_offset", pf_fb_offset },
			{ "fb_alignment", fb_alignment },
			{ "max_vf_fb_usable", max_vf_fb_usable },
			{ "min_vf_fb_usable", min_vf_fb_usable },
		};

		out = fb_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s", value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str());
	} else {
		out = string_format(
				  staticFbInfoTemplate, value.c_str(),"", value.c_str(),"",
				  value.c_str(),"", value.c_str(),"", value.c_str(),"",
				  value.c_str(),"");
	}

	return out;
}

std::string host_fill_num_vf(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json num_vf_json = { { "num_vf_supported", value.c_str() },
			{ "num_vf_enabled", value.c_str() }
		};

		values_json["num_vf"] = num_vf_json;
		out = values_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s", value.c_str(),
				  value.c_str());
	} else {
		out = string_format(
				  staticNumVfTemplate, value.c_str(),
				  value.c_str());
	}

	return out;
}

std::string host_fill_vram_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json vram_size{};
		vram_size["value"] = value.c_str();
		vram_size["unit"] = "N/A";
		nlohmann::ordered_json vram_info_json = { { "vram_type", value.c_str() },
			{ "vram_vendor", value.c_str()},
			{ "vram_size",  vram_size},
			{ "vram_bit_width",  value.c_str()}
		};

		out = vram_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s", value.c_str(),
				  value.c_str(),
				  value.c_str(),
				  value.c_str());
	} else {
		out = string_format(
				  staticVramTemplate, value.c_str(),
				  value.c_str(),
				  value.c_str(),
				  value.c_str(),
				  value.c_str());
	}

	return out;
}

std::string host_fill_vf_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json static_vf_json;
		nlohmann::ordered_json fb_offset{};
		fb_offset["value"] = value.c_str();
		fb_offset["unit"] = "N/A";
		nlohmann::ordered_json fb_size{};
		fb_size["value"] = value.c_str();
		fb_size["unit"] = "N/A";
		nlohmann::ordered_json timeslice{};
		timeslice["value"] = value.c_str();
		timeslice["unit"] = "N/A";
		static_vf_json = { { "gpu", "N/A"}, { "vf", "N/A" },
			{ "fb_offset", fb_offset },
			{ "fb_size", fb_size  },
			{ "timeslice", timeslice  }
		};
		out = static_vf_json.dump(4);
	} else if (arg.output == csv) {
		out += string_format(
				   ",%s,%s,%s",
				   value.c_str(), value.c_str(), value.c_str());
	} else {
		out += string_format(
				   staticVfTemplate, value.c_str(), "", value.c_str(), "",
				   value.c_str(), "");
	}

	return out;
}

std::string host_fill_limit_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json max_power{};
		nlohmann::ordered_json min_power{};
		max_power["value"] = value.c_str();
		max_power["unit"] = "N/A";
		min_power["value"] = value.c_str();
		min_power["unit"] = "N/A";
		nlohmann::ordered_json socket_power{};
		socket_power["value"] = value.c_str();
		socket_power["unit"] = "N/A";
		nlohmann::ordered_json slowdown_edge_temperature{};
		slowdown_edge_temperature["value"] = value.c_str();
		slowdown_edge_temperature["unit"] = "N/A";
		nlohmann::ordered_json slowdown_hotspot_temperature{};
		slowdown_hotspot_temperature["value"] = value.c_str();
		slowdown_hotspot_temperature["unit"] = "N/A";
		nlohmann::ordered_json slowdown_vram_temperature{};
		slowdown_vram_temperature["value"] = value.c_str();
		slowdown_vram_temperature["unit"] = "N/A";
		nlohmann::ordered_json shutdown_edge_temperature{};
		shutdown_edge_temperature["value"] = value.c_str();
		shutdown_edge_temperature["unit"] = "N/A";
		nlohmann::ordered_json shutdown_hotspot_temperature{};
		shutdown_hotspot_temperature["value"] = value.c_str();
		shutdown_hotspot_temperature["unit"] = "N/A";
		nlohmann::ordered_json shutdown_vram_temperature{};
		shutdown_vram_temperature["value"] = value.c_str();
		shutdown_vram_temperature["unit"] = "N/A";

		nlohmann::ordered_json limit_json = { { "max_power",  max_power} };
		limit_json["socket_power"] = socket_power;
		limit_json["slowdown_edge_temperature"] = slowdown_edge_temperature;
		limit_json["slowdown_hotspot_temperature"] = slowdown_hotspot_temperature;
		limit_json["slowdown_mem_temperature"] = slowdown_vram_temperature;
		limit_json["shutdown_edge_temperature"] = shutdown_edge_temperature;
		limit_json["shutdown_hotspot_temperature"] = shutdown_hotspot_temperature;
		limit_json["shutdown_mem_temperature"] = shutdown_vram_temperature;

		out = limit_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s,%s,%s,%s", value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str() );
	} else {
		out = string_format(
				  staticLimitTemplate, value.c_str(), value.c_str(), "", value.c_str(),"",
				  value.c_str(), "",value.c_str(),"",
				  value.c_str(), "",value.c_str(),"",
				  value.c_str(),"", value.c_str(),"" );
	}

	return out;
}

std::string host_fill_cache_info(Arguments arg, std::string value)
{
	std::string out{};


	if (arg.output == json) {
		auto cache_list_json = nlohmann::ordered_json::array();
		out = cache_list_json.dump(4);
	} else if(arg.output == csv) {
		out += string_format(
				   "%s,%s,%s,%s,%s,%s", value.c_str(),
				   value.c_str(), value.c_str(), value.c_str(),
				   value.c_str(), value.c_str());
	} else {
		out = staticCacheHeaderTemplate;
		out += string_format(
				   staticCacheInfoTemplate, value.c_str(), value.c_str(),
				   value.c_str(), "", value.c_str(),
				   value.c_str(),
				   value.c_str());
	}

	return out;
}

std::string host_fill_partition(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json static_partition_json{};
		static_partition_json["accelerator_partition"] = value.c_str();
		static_partition_json["memory_partition"] = value.c_str();
		static_partition_json["partition_id"] = value.c_str();
		out = static_partition_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format("%s,%s,%s", value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(staticPartitionTemplate, value.c_str(), value.c_str(), value.c_str());
	}

	return out;
}
std::string host_fill_soc_pstate(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		auto dpm_list_json = nlohmann::ordered_json::array();
		nlohmann::ordered_json dpm_info_json = { { "num_supported", "N/A"},
			{ "current_id", "N/A"},
			{ "policies", dpm_list_json}
		};
		out = dpm_info_json.dump(4);
	} else if(arg.output == csv) {
		out += string_format(
				   "%s,%s,%s,%s,%s,%s", value.c_str(),
				   value.c_str(), value.c_str(), value.c_str(),
				   value.c_str(), value.c_str());
	} else {
		out = staticCacheHeaderTemplate;
		out += string_format(
				   staticCacheInfoTemplate, value.c_str(), value.c_str(),
				   value.c_str(), "", value.c_str(),
				   value.c_str(),
				   value.c_str());
	}

	return out;
}

int AmdSmiApiHost::amdsmi_get_asic_info_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	int ret;
	amdsmi_asic_info_t asic;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_asic_info(processor, &asic);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_asic_info(arg, "N/A");
		return ret;
	}

	std::string vendor_id_hex =
		string_format("0x%X", asic.vendor_id);
	std::string vendor_name =
		string_format("%s", asic.vendor_name);
	vendor_name.erase(std::remove(vendor_name.begin(), vendor_name.end(), ','), vendor_name.end());
	std::string device_id_hex =
		string_format("0x%X", asic.device_id);
	std::string rev_id_hex =
		string_format("0x%X", asic.rev_id);
	std::string serial_id_hex = asic.asic_serial;

	std::string subvendor_id_hex{};
	if (asic.subvendor_id == UINT_MAX) {
		subvendor_id_hex = "N/A";
	} else {
		subvendor_id_hex = string_format("0x%X", asic.subvendor_id);
	}
	std::string subsystem_id_hex{};
	if (asic.subsystem_id == UINT_MAX) {
		subsystem_id_hex = "N/A";
	} else {
		subsystem_id_hex = string_format("0x%X", asic.subsystem_id);
	}

	std::string oam_id{};
	if (asic.oam_id == UINT_MAX) {
		oam_id = "N/A";
	} else {
		oam_id = string_format("%ld", asic.oam_id);
	}

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json asic_json = { { "market_name", asic.market_name },
			{ "vendor_id", vendor_id_hex },
			{ "vendor_name", vendor_name.c_str() },
			{ "subvendor_id", subvendor_id_hex.c_str() },
			{ "device_id", device_id_hex },
			{ "subsystem_id", subsystem_id_hex.c_str() },
			{ "rev_id", rev_id_hex },
			{ "asic_serial", serial_id_hex },
			{ "oam_id", asic.oam_id }
		};

		if (asic.oam_id == UINT_MAX) {
			asic_json["oam_id"] = oam_id;
		} else {
			asic_json["oam_id"] = asic.oam_id;
		}

		out = asic_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s",
							asic.market_name,
							vendor_id_hex.c_str(), vendor_name.c_str(),
							subvendor_id_hex.c_str(), device_id_hex.c_str(),
							subsystem_id_hex.c_str(), rev_id_hex.c_str(),
							serial_id_hex.c_str(), oam_id.c_str());
	} else {
		out = string_format(
				  staticAsicTemplate, asic.market_name, vendor_id_hex.c_str(),
				  vendor_name.c_str(), subvendor_id_hex.c_str(), device_id_hex.c_str(), subsystem_id_hex.c_str(),
				  rev_id_hex.c_str(),
				  serial_id_hex.c_str(), oam_id.c_str());
	}

	return ret;
}


int AmdSmiApiHost::amdsmi_get_bus_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_pcie_info_t pcie_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_pcie_info(processor, &pcie_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_bus_info(arg, "N/A");
		return ret;
	}

	amdsmi_bdf_t bdf;
	ret = host_amdsmi_get_gpu_device_bdf(processor, &bdf);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	std::string bdf_string = string_format(
								 "%04x:%02x:%02x.%01x", bdf.bdf.domain_number, bdf.bdf.bus_number, bdf.bdf.device_number,
								 bdf.bdf.function_number);
	std::ostringstream pcie_interface_version_value;
	pcie_interface_version_value << "Gen " << pcie_info.pcie_static.pcie_interface_version;
	std::string pcie_interface_version = pcie_interface_version_value.str();

	std::ostringstream max_pcie_interface_version_value{"N/A"};
	if (pcie_info.pcie_static.max_pcie_interface_version != UINT_MAX) {
		max_pcie_interface_version_value << "Gen " << pcie_info.pcie_static.max_pcie_interface_version;
	}
	std::string max_pcie_interface_version = max_pcie_interface_version_value.str();

	std::string pcie_slot_type = convert_slot_type_to_string(
									 pcie_info.pcie_static.slot_type);
	std::string pcie_lanes{ string_format(
								"%d", pcie_info.pcie_static.max_pcie_width) };
	std::string pcie_info_GTs_value_string{
		string_format("%d", pcie_info.pcie_static.max_pcie_speed / 1000)
	};

	if (arg.output == json) {
		nlohmann::ordered_json max_pcie_speed{};
		max_pcie_speed["value"] = pcie_info.pcie_static.max_pcie_speed / 1000;
		max_pcie_speed["unit"] = pcie_info_GTs_value_string == "N/A" ? "N/A" : "GT/s";
		nlohmann::ordered_json bus_json = { { "bdf", bdf_string },
			{ "max_pcie_width", pcie_info.pcie_static.max_pcie_width },
			{ "max_pcie_speed",  max_pcie_speed},
			{ "pcie_interface_version", pcie_interface_version },
			{ "slot_type", pcie_slot_type },
			{ "max_pcie_interface_version", max_pcie_interface_version }
		};

		formatted_string = bus_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s,%s,%s,%s,%s", bdf_string.c_str(),
										 pcie_lanes.c_str(),pcie_info_GTs_value_string.c_str(),
										 pcie_interface_version.c_str(), pcie_slot_type.c_str(), max_pcie_interface_version.c_str());
	} else {
		std::string pcie_info_GTs_value_string_unit = pcie_info_GTs_value_string == "N/A" ? "" : "GT/s";
		formatted_string = string_format(
							   staticBusTemplate, bdf_string.c_str(), pcie_lanes.c_str(),
							   pcie_info_GTs_value_string.c_str(), pcie_info_GTs_value_string_unit.c_str(),
							   pcie_interface_version.c_str(), pcie_slot_type.c_str(), max_pcie_interface_version.c_str());
	}

	return ret;
}


int AmdSmiApiHost::amdsmi_get_vbios_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_vbios_info_t vbios_info;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_vbios_info(processor, &vbios_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_vbios_info(arg, "N/A");
		return ret;
	}

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json vbios_json = { { "name", vbios_info.name },
			{ "build_date", vbios_info.build_date },
			{ "part_number", vbios_info.part_number },
			{ "version", vbios_info.version }
		};

		formatted_string = vbios_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s", vbios_info.name, vbios_info.build_date,
							   vbios_info.part_number, vbios_info.version);
	} else {
		formatted_string = string_format(
							   staticVbiosTemplate, vbios_info.name, vbios_info.build_date,
							   vbios_info.part_number, vbios_info.version);
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_board_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_board_info_t board_info;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_board_info(processor, &board_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_board_info(arg, "N/A");
		return ret;
	}

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json board_json = { { "model_number", board_info.model_number },
			{ "product_serial", board_info.product_serial },
			{ "fru_id", board_info.fru_id },
			{ "product_name", board_info.product_name},
			{ "manufacturer_name", board_info.manufacturer_name }
		};

		formatted_string = board_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s",
							   board_info.model_number, board_info.product_serial,
							   board_info.fru_id,
							   board_info.product_name,
							   board_info.manufacturer_name);
	} else {
		formatted_string = string_format(
							   staticBoardTemplate, board_info.model_number,
							   board_info.product_serial, board_info.fru_id,
							   board_info.product_name, board_info.manufacturer_name);
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_limit_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;

	amdsmi_power_cap_info_t power_cap_info;
	uint32_t sensor_ind = 0;
	int64_t therm_limit_edge;
	int64_t therm_limit_junction;
	int64_t therm_limit_vram;

	int64_t edge_shutdown_temperature;
	int64_t junction_shutdown_temperature;
	int64_t vram_shutdown_temperature;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}


	ret = host_amdsmi_get_power_cap_info(processor, sensor_ind, &power_cap_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_limit_info(arg, "N/A");
		return ret;
	}
	ret = host_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_CRITICAL, &therm_limit_edge);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_limit_info(arg, "N/A");
		return ret;
	}
	ret = host_amdsmi_get_temp_metric(processor,
									  AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
									  AMDSMI_TEMP_CRITICAL,
									  &therm_limit_junction);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_limit_info(arg, "N/A");
		return ret;
	}
	ret = host_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_VRAM, AMDSMI_TEMP_CRITICAL, &therm_limit_vram);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_limit_info(arg, "N/A");
		return ret;
	}
	ret = host_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_SHUTDOWN, &edge_shutdown_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_limit_info(arg, "N/A");
		return ret;
	}
	ret = host_amdsmi_get_temp_metric(processor,
									  AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
									  AMDSMI_TEMP_SHUTDOWN,
									  &junction_shutdown_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_limit_info(arg, "N/A");
		return ret;
	}
	ret = host_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_VRAM, AMDSMI_TEMP_SHUTDOWN, &vram_shutdown_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_limit_info(arg, "N/A");
		return ret;
	}
	std::string power_cap_string = power_cap_info.power_cap == -1 ?
								   "N/A" :
								   string_format("%lld", power_cap_info.power_cap);
	std::string max_power_cap_string = power_cap_info.max_power_cap == -1 ?
									   "N/A" :
									   string_format("%lld", power_cap_info.max_power_cap);

	std::string min_power_cap_string = power_cap_info.min_power_cap == -1 ?
									   "N/A" :
									   string_format("%lld", power_cap_info.min_power_cap);

	std::string therm_limit_edge_string;
	if(therm_limit_edge == UINT_MAX) {
		therm_limit_edge_string = "N/A";
	} else {
		therm_limit_edge_string = string_format("%lld", therm_limit_edge);
	}
	std::string therm_limit_junction_string;
	if (therm_limit_junction == UINT_MAX) {
		therm_limit_junction_string = "N/A";
	} else {
		therm_limit_junction_string = string_format("%lld", therm_limit_junction);
	}
	std::string therm_limit_vram_string;
	if (therm_limit_vram == UINT_MAX) {
		therm_limit_vram_string = "N/A";
	} else {
		therm_limit_vram_string = string_format("%lld", therm_limit_vram);
	}
	std::string edge_shutdown_temperature_string;
	if (edge_shutdown_temperature == UINT_MAX) {
		edge_shutdown_temperature_string = "N/A";
	} else {
		edge_shutdown_temperature_string = string_format("%lld", edge_shutdown_temperature);
	}

	std::string junction_shutdown_temperature_string = junction_shutdown_temperature == UINT_MAX ?
		"N/A" : string_format("%lld", junction_shutdown_temperature);
	std::string vram_shutdown_temperature_string = vram_shutdown_temperature == UINT_MAX ? "N/A" :
		string_format("%lld", vram_shutdown_temperature);

	if (arg.output == json) {
		nlohmann::ordered_json max_power{};
		nlohmann::ordered_json min_power{};
		nlohmann::ordered_json socket_power{};
		if (power_cap_info.power_cap == UINT64_MAX) {
			socket_power["value"] = "N/A";
			socket_power["unit"] = "N/A";
		} else {
			socket_power["value"] = power_cap_info.power_cap;
			socket_power["unit"] = "W";
		}
		if (power_cap_info.max_power_cap == UINT64_MAX) {
			max_power["value"] = "N/A";
			max_power["unit"] = "N/A";
		} else {
			max_power["value"] = power_cap_info.max_power_cap;
			max_power["unit"] = "W";
		}
		if (power_cap_info.min_power_cap == UINT64_MAX) {
			min_power["value"] = "N/A";
			min_power["unit"] = "N/A";
		} else {
			min_power["value"] = power_cap_info.min_power_cap;
			min_power["unit"] = "W";
		}

		nlohmann::ordered_json slowdown_edge_temperature{};
		if (therm_limit_edge_string == "N/A") {
			slowdown_edge_temperature["value"] = "N/A";
		} else {
			slowdown_edge_temperature["value"] = therm_limit_edge;
		}
		slowdown_edge_temperature["unit"] = therm_limit_edge_string == "N/A" ? "N/A" : "C";
		nlohmann::ordered_json slowdown_hotspot_temperature{};
		if (therm_limit_junction_string == "N/A") {
			slowdown_hotspot_temperature["value"] = "N/A";
		} else {
			slowdown_hotspot_temperature["value"] = therm_limit_junction;
		}
		slowdown_hotspot_temperature["unit"] = therm_limit_junction_string == "N/A" ? "N/A" : "C";
		nlohmann::ordered_json slowdown_vram_temperature{};
		if (therm_limit_vram_string == "N/A") {
			slowdown_vram_temperature["value"] = "N/A";
		} else {
			slowdown_vram_temperature["value"] = therm_limit_vram;
		}
		slowdown_vram_temperature["unit"] = therm_limit_vram_string == "N/A" ? "N/A" : "C";
		nlohmann::ordered_json shutdown_edge_temperature{};
		if (edge_shutdown_temperature_string == "N/A") {
			shutdown_edge_temperature["value"] = "N/A";
		} else {
			shutdown_edge_temperature["value"] = edge_shutdown_temperature;
		}
		shutdown_edge_temperature["unit"] = edge_shutdown_temperature_string == "N/A" ? "N/A" : "C";
		nlohmann::ordered_json shutdown_hotspot_temperature{};
		if (junction_shutdown_temperature_string == "N/A") {
			shutdown_hotspot_temperature["value"] = "N/A";
		} else {
			shutdown_hotspot_temperature["value"] = junction_shutdown_temperature;
		}
		shutdown_hotspot_temperature["unit"] = junction_shutdown_temperature_string == "N/A" ? "N/A" : "C";
		nlohmann::ordered_json shutdown_vram_temperature{};
		if (vram_shutdown_temperature_string == "N/A") {
			shutdown_vram_temperature["value"] = "N/A";
		} else {
			shutdown_vram_temperature["value"] = vram_shutdown_temperature;
		}
		shutdown_vram_temperature["unit"] = vram_shutdown_temperature_string == "N/A" ? "N/A" : "C";

		nlohmann::ordered_json limit_json = { { "max_power",  max_power} };
		limit_json["min_power"] = min_power;
		limit_json["socket_power"] = socket_power;
		limit_json["slowdown_edge_temperature"] = slowdown_edge_temperature;
		limit_json["slowdown_hotspot_temperature"] = slowdown_hotspot_temperature;
		limit_json["slowdown_mem_temperature"] = slowdown_vram_temperature;
		limit_json["shutdown_edge_temperature"] = shutdown_edge_temperature;
		limit_json["shutdown_hotspot_temperature"] = shutdown_hotspot_temperature;
		limit_json["shutdown_mem_temperature"] = shutdown_vram_temperature;
		formatted_string = limit_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s,%s,%s,%s,%s", max_power_cap_string.c_str(),
							   min_power_cap_string.c_str(),
							   power_cap_string.c_str(), therm_limit_edge_string.c_str(),
							   therm_limit_junction_string.c_str(),
							   therm_limit_vram_string.c_str(),
							   edge_shutdown_temperature_string.c_str(),
							   junction_shutdown_temperature_string.c_str(),
							   vram_shutdown_temperature_string.c_str() );
	} else {
		std::string max_power_cap_string_uint = max_power_cap_string == "N/A" ? "" : "W";
		std::string min_power_cap_string_uint = min_power_cap_string == "N/A" ? "" : "W";
		std::string power_cap_string_unit = power_cap_string == "N/A" ? "" : "W";
		std::string therm_limit_edge_string_unit = therm_limit_edge_string == "N/A" ? "" : "C";
		std::string therm_limit_junction_string_unit = therm_limit_junction_string == "N/A" ? "" : "C";
		std::string therm_limit_vram_string_unit = therm_limit_vram_string == "N/A" ? "" : "C";
		std::string edge_shutdown_temperature_string_unit = edge_shutdown_temperature_string == "N/A" ? "" :
			"C";
		std::string junction_shutdown_temperature_string_unit = junction_shutdown_temperature_string ==
			"N/A" ? "" : "C";
		std::string vram_shutdown_temperature_string_unit = vram_shutdown_temperature_string == "N/A" ? "" :
			"C";
		formatted_string = string_format(
							   staticLimitTemplate, max_power_cap_string.c_str(),
							   max_power_cap_string_uint.c_str(), min_power_cap_string.c_str(),
							   min_power_cap_string_uint.c_str(), power_cap_string.c_str(),
							   power_cap_string_unit.c_str(),
							   therm_limit_edge_string.c_str(),therm_limit_edge_string_unit.c_str(),
							   therm_limit_junction_string.c_str(), therm_limit_junction_string_unit.c_str(),
							   therm_limit_vram_string.c_str(),therm_limit_vram_string_unit.c_str(),
							   edge_shutdown_temperature_string.c_str(),edge_shutdown_temperature_string_unit.c_str(),
							   junction_shutdown_temperature_string.c_str(), junction_shutdown_temperature_string_unit.c_str(),
							   vram_shutdown_temperature_string.c_str(),vram_shutdown_temperature_string_unit.c_str() );
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_driver_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_driver_info_t driver_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_driver_model_type_t driver_model;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_driver_info(processor, &driver_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_driver_info(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_gpu_driver_model(processor, &driver_model);
	if (ret == AMDSMI_STATUS_NOT_SUPPORTED) {
		driver_model = AMDSMI_DRIVER_MODEL_TYPE__MAX;  //using max as default value
		ret = AMDSMI_STATUS_SUCCESS;
	} else if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_driver_info(arg, "N/A");
		return ret;
	}

	to_lower_case(driver_info.driver_name);

	std::string driver_model_str;

	if (driver_model == AMDSMI_DRIVER_MODEL_TYPE__MAX) {
		driver_model_str = "N/A";
	} else {
		get_string_from_enum_driver_model(driver_model, driver_model_str);
	}

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json driver_json = { { "name", driver_info.driver_name }, { "version", driver_info.driver_version },
			{ "date", driver_info.driver_date }, {"model", driver_model_str.c_str()}
		};

		formatted_string = driver_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s", driver_info.driver_name, driver_info.driver_version, driver_info.driver_date,
							   driver_model_str.c_str());
	} else {
		formatted_string = string_format(
							   driverHostInfoTemplate, driver_info.driver_name, driver_info.driver_version,
							   driver_info.driver_date, driver_model_str.c_str());
	}

	return ret;

}

int AmdSmiApiHost::amdsmi_get_ras_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret_ras_info;
	int ret_ecc_enabled;
	int ret;
	nlohmann::ordered_json ras_json;

	amdsmi_ras_feature_t ras_feature;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret_ras_info = host_amdsmi_get_gpu_ras_feature_info(processor,
				   &ras_feature);
	std::vector<std::string> ecc_correction_schema_flag;
	std::string ras_eeprom_version_str;
	std::vector<std::string> schema{"parity_schema","single_bit_schema","double_bit_schema","poison_schema"};
	if (ret_ras_info != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_ras_info(arg, "N/A");
	} else {
		ras_eeprom_version_str = string_format("0x%X", ras_feature.ras_eeprom_version);
		transform_ecc_correction_schema(
			ras_feature.supported_ecc_correction_schema, ecc_correction_schema_flag);

		nlohmann::ordered_json supperted_schemas_json;
		nlohmann::ordered_json gpu_blocks_json;

		if (arg.output == json) {
			ras_json["eeprom_version"] = ras_eeprom_version_str.c_str();
			for(int i = 0; i < ecc_correction_schema_flag.size(); i++) {
				ras_json[schema[i].c_str()] = ecc_correction_schema_flag[i].c_str();
			}
		}
		if(arg.output == human) {
			formatted_string = string_format(
								   staticRasTemplateHost, ras_eeprom_version_str.c_str(), ecc_correction_schema_flag[0].c_str(),
								   ecc_correction_schema_flag[1].c_str()
								   ,ecc_correction_schema_flag[2].c_str(),ecc_correction_schema_flag[3].c_str());
		}
	}
	uint64_t enabled_blocks{};
	ret_ecc_enabled = host_amdsmi_get_gpu_ecc_enabled(processor, &enabled_blocks);
	nlohmann::ordered_json blocks_values{};
	for (auto block : ecc_blocks) {
		std::string block_str{};
		std::string status{};
		if (ret_ecc_enabled != AMDSMI_STATUS_SUCCESS) {
			block_str = "N/A";
			status = "N/A";
		} else {
			get_string_from_enum_ecc_blocks(block, block_str);
			status = {(enabled_blocks & block) ? "ENABLED" : "DISABLED"};
		}

		if (arg.output == json) {
			blocks_values[block_str] = status;
		} else if (arg.output == csv) {
			for(int i = 0; i < ecc_correction_schema_flag.size(); i++) {
				formatted_string += string_format(
										",%s,%s,%s,%s,%s\n", block_str.c_str(), status.c_str(), ras_eeprom_version_str.c_str(),
										schema[i].c_str(), ecc_correction_schema_flag[i].c_str());
			}
		} else {
			formatted_string.append(string_format(staticRasBlockTemplate, block_str.c_str(), status.c_str()));
		}
	}
	if (arg.output == json) {
		ras_json["block_state"] = blocks_values;
		formatted_string = ras_json.dump(4);
	}

	return (ret_ras_info == AMDSMI_STATUS_SUCCESS
			|| ret_ecc_enabled == AMDSMI_STATUS_SUCCESS) ? AMDSMI_STATUS_SUCCESS : ret_ras_info;
}

int AmdSmiApiHost::amdsmi_get_dfc_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;
	amdsmi_dfc_fw_t dfc_info;
	std::string version_str;
	std::string verification_value = "unknown";
	std::string customer_ordinal_str;
	auto data_list_json = nlohmann::ordered_json::array();
	auto white_list_json = nlohmann::ordered_json::array();
	std::vector<std::string> black_list_versions = { };
	auto iter_white_list = 0;
	bool found;
	nlohmann::ordered_json values_json{};
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_dfc_fw_table(processor, &dfc_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}
	version_str = transform_fw(
					  AMDSMI_FW_ID_DFC, dfc_info.header.dfc_fw_version);

	std::string dfc_gart_wr_guest_min_str = AmdSmiPlatform::getInstance().getInstance().is_nv32() ?
											string_format("%ld", dfc_info.header.dfc_gart_wr_guest_min) :
											"N/A";
	std::string dfc_gart_wr_guest_max_str = AmdSmiPlatform::getInstance().getInstance().is_nv32() ?
											string_format("%ld", dfc_info.header.dfc_gart_wr_guest_max) :
											"N/A";

	if (arg.output == human) {
		formatted_string += string_format(
								staticDfcHeaderTemplate, version_str.c_str(), dfc_gart_wr_guest_min_str.c_str(),
								dfc_gart_wr_guest_max_str.c_str());
	}
	if (arg.output == csv && dfc_info.header.dfc_fw_total_entries == 0) {
		formatted_string += string_format(
								",%s,%s,%s,,,,,,,,", version_str.c_str(), dfc_gart_wr_guest_min_str.c_str(),
								dfc_gart_wr_guest_max_str.c_str());
	}

	for (auto i = 0; i < dfc_info.header.dfc_fw_total_entries; i++) {
		if (dfc_info.data[i].dfc_fw_type > 0) {
			found = true;
			if (dfc_info.data[i].verification_enabled == 1) {
				verification_value = "ENABLED";
			} else if (dfc_info.data[i].verification_enabled == 0) {
				verification_value = "DISABLED";
			}
			if (AmdSmiPlatform::getInstance().getInstance().is_nv32()) {
				customer_ordinal_str = string_format("%ld", dfc_info.data[i].customer_ordinal);
			} else {
				customer_ordinal_str = "N/A";
			}

			if (arg.output == human) {
				formatted_string += string_format(
										staticDfcDataTemplate, dfc_info.data[i].dfc_fw_type,
										verification_value.c_str(), customer_ordinal_str.c_str());
				formatted_string += staticDfcWhiteListHeaderTemplate;
			}

			for (auto dfc_white_list_elem : dfc_info.data[i].white_list) {
				if (dfc_white_list_elem.latest != 0 &&
						dfc_white_list_elem.oldest != 0) {
					if (arg.output == human) {
						formatted_string +=
							string_format(
								staticDfcWhiteListElementTemplate,
								string_format("0x%X", dfc_white_list_elem.latest).c_str(),
								string_format("0x%X", dfc_white_list_elem.oldest).c_str());
						formatted_string +=
							staticDfcBlackListHeaderTemplate;
					}

					if (arg.output == csv) {
						// version,dfc_gart_wr_guest_min_str,dfc_gart_wr_guest_max_str,dfc_fw_type,verification,wl_latest,wl_oldest
						formatted_string.append(string_format(",%s,%s,%s,%d,%s,%s,%s,%s",
															  version_str.c_str(),
															  dfc_gart_wr_guest_min_str.c_str(),
															  dfc_gart_wr_guest_max_str.c_str(),
															  dfc_info.data[i].dfc_fw_type,
															  verification_value.c_str(),
															  customer_ordinal_str.c_str(),
															  string_format("0x%X", dfc_white_list_elem.latest).c_str(),
															  string_format("0x%X", dfc_white_list_elem.oldest).c_str()));
					}
					int index = 1;
					std::string current_bl_ver{};
					for (int k = (4 * iter_white_list);
							k < (4 * (iter_white_list + 1)); k++) {
						if(dfc_info.data[i].black_list[k] > 0)
							current_bl_ver = string_format("0x%X",dfc_info.data[i].black_list[k]);
						if (arg.output == json) {
							if(dfc_info.data[i].black_list[k] > 0)
								black_list_versions.push_back(
													   string_format("0x%X", dfc_info.data[i].black_list[k]));
						} else if (arg.output == csv) {
							if(dfc_info.data[i].black_list[k] > 0)
								formatted_string.append(string_format(",%s", current_bl_ver.c_str()));
							else
								formatted_string.append(",");
						} else {
							if(dfc_info.data[i].black_list[k] > 0) {
								formatted_string +=
									string_format(
										staticDfcBlackListElementTemplate,
										index,
										current_bl_ver.c_str());
								index++;
							}
						}
					}
					++iter_white_list;

					if (arg.output == json) {
						white_list_json.push_back(
						nlohmann::ordered_json::object( {
							{
								"white_list_latest",
								string_format("0x%X", dfc_white_list_elem.latest)
							},
							{
								"white_list_oldest",
								string_format("0x%X", dfc_white_list_elem.oldest)
							},
							{
								"black_list",
								black_list_versions
							} }));
						black_list_versions.clear();
					}
					if (arg.output == csv) {
						formatted_string += "\n";
					}
				}
			}
			if (arg.output == json) {
				if (AmdSmiPlatform::getInstance().getInstance().is_nv32()) {
					data_list_json.push_back(nlohmann::ordered_json::object( {
						{ "dfc_fw_type", dfc_info.data[i].dfc_fw_type },
						{ "verification", verification_value },
						{ "customer_ordinal", dfc_info.data[i].customer_ordinal },
						{ "white_list", white_list_json } }));
				} else {
					data_list_json.push_back(nlohmann::ordered_json::object( {
						{ "dfc_fw_type", dfc_info.data[i].dfc_fw_type },
						{ "verification", verification_value },
						{ "customer_ordinal", "N/A" },
						{ "white_list", white_list_json } }));
				}
			}
		}
	}

	if (arg.output == csv && formatted_string == "") {
		formatted_string += string_format(
								",%s,%s,%s,,,,,,,,", version_str.c_str(), dfc_gart_wr_guest_min_str.c_str(),
								dfc_gart_wr_guest_max_str.c_str());
	}

	if (arg.output == json) {
		nlohmann::ordered_json header;
		if (AmdSmiPlatform::getInstance().getInstance().is_nv32()) {
			header = { { "version", version_str.c_str() },
				{ "gart_wr_guest_min", dfc_info.header.dfc_gart_wr_guest_min },
				{ "gart_wr_guest_max", dfc_info.header.dfc_gart_wr_guest_max }
			};
		} else {
			header = { { "version", version_str.c_str() },
				{ "gart_wr_guest_min", "N/A" },
				{ "gart_wr_guest_max", "N/A" }
			};
		}
		nlohmann::ordered_json dfc_json = { { "header", header },
			{ "data", data_list_json }
		};
		formatted_string = dfc_json.dump(4);
	}
	if (!found && arg.output == human) {
		formatted_string += staticDfcDataTemplateEmpty;
		formatted_string += staticDfcWhiteListHeaderTemplate;
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_fb_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;

	amdsmi_pf_fb_info_t fb_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_fb_layout(processor, &fb_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_fb_info(arg, "N/A");
		return ret;
	}

	std::string total_fb_size_str{ string_format(
									   "%ld", fb_info.total_fb_size) };
	std::string pf_fb_reserved_str{ string_format(
										"%ld", fb_info.pf_fb_reserved) };
	std::string pf_fb_offset_str{ string_format(
									  "%ld", fb_info.pf_fb_offset) };
	std::string fb_alignment_str{ string_format(
									  "%ld", fb_info.fb_alignment) };
	std::string max_vf_fb_usable_str{ string_format(
										  "%ld", fb_info.max_vf_fb_usable) };
	std::string min_vf_fb_usable_str{ string_format(
										  "%ld", fb_info.min_vf_fb_usable) };

	if (arg.output == json) {
		nlohmann::ordered_json total_fb_size{};
		total_fb_size["value"] = fb_info.total_fb_size;
		total_fb_size["unit"] = total_fb_size_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json pf_fb_reserved{};
		pf_fb_reserved["value"] = fb_info.pf_fb_reserved;
		pf_fb_reserved["unit"] = pf_fb_reserved_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json pf_fb_offset{};
		pf_fb_offset["value"] = fb_info.pf_fb_offset;
		pf_fb_offset["unit"] = pf_fb_offset_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json fb_alignment{};
		fb_alignment["value"] = fb_info.fb_alignment;
		fb_alignment["unit"] = fb_alignment_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json max_vf_fb_usable{};
		max_vf_fb_usable["value"] = fb_info.max_vf_fb_usable;
		max_vf_fb_usable["unit"] = max_vf_fb_usable_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json min_vf_fb_usable{};
		min_vf_fb_usable["value"] = fb_info.min_vf_fb_usable;
		min_vf_fb_usable["unit"] = min_vf_fb_usable_str == "N/A" ? "N/A" : "MB";

		nlohmann::ordered_json fb_info_json = {
			{ "total_fb_size", total_fb_size },
			{ "pf_fb_reserved", pf_fb_reserved },
			{ "pf_fb_offset", pf_fb_offset },
			{ "fb_alignment", fb_alignment },
			{ "max_vf_fb_usable", max_vf_fb_usable },
			{ "min_vf_fb_usable", min_vf_fb_usable },
		};

		formatted_string = fb_info_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s,%s", total_fb_size_str.c_str(), pf_fb_reserved_str.c_str(),
							   pf_fb_offset_str.c_str(), fb_alignment_str.c_str(), max_vf_fb_usable_str.c_str(),
							   min_vf_fb_usable_str.c_str());
	} else {
		std::string total_fb_size_str_unit = total_fb_size_str == "N/A" ? "" : "MB";
		std::string pf_fb_reserved_str_unit = pf_fb_reserved_str == "N/A" ? "" : "MB";
		std::string pf_fb_offset_str_unit = pf_fb_offset_str == "N/A" ? "" : "MB";
		std::string fb_alignment_str_unit = fb_alignment_str == "N/A" ? "" : "MB";
		std::string max_vf_fb_usable_str_unit = max_vf_fb_usable_str == "N/A" ? "" : "MB";
		std::string min_vf_fb_usable_str_unit = min_vf_fb_usable_str == "N/A" ? "" : "MB";
		formatted_string = string_format(
							   staticFbInfoTemplate, total_fb_size_str.c_str(), total_fb_size_str_unit.c_str(),
							   pf_fb_reserved_str.c_str(), pf_fb_reserved_str_unit.c_str(),
							   pf_fb_offset_str.c_str(), pf_fb_offset_str_unit.c_str(), fb_alignment_str.c_str(),
							   fb_alignment_str_unit.c_str(), max_vf_fb_usable_str.c_str(), max_vf_fb_usable_str_unit.c_str(),
							   min_vf_fb_usable_str.c_str(), min_vf_fb_usable_str_unit.c_str());
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_num_vf_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;

	uint32_t num_vf_supported;
	uint32_t num_vf_enabled;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_num_vf(processor, &num_vf_enabled,
								 &num_vf_supported);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_num_vf(arg, "N/A");
		return ret;
	}

	std::string num_vf_supported_string{
		string_format("%d", num_vf_supported)
	};
	std::string num_vf_enabled_string{ string_format(
										   "%d", num_vf_enabled) };

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json num_vf_json = { { "supported", num_vf_supported },
			{ "enabled", num_vf_enabled }
		};

		formatted_string = num_vf_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s", num_vf_supported_string.c_str(),
							   num_vf_enabled_string.c_str());
	} else {
		formatted_string = string_format(
							   staticNumVfTemplate, num_vf_supported_string.c_str(),
							   num_vf_enabled_string.c_str());
	}

	return ret;
}


int AmdSmiApiHost::amdsmi_get_vram_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;

	amdsmi_vram_info_t vram_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_vram_info(processor, &vram_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_vram_info(arg, "N/A");
		return ret;
	}

	std::string vram_type_str;
	get_string_from_enum_vram_type(vram_info.vram_type, vram_type_str);
	std::string vram_vendor_type_str;
	if (vram_info.vram_vendor == AMDSMI_VRAM_VENDOR_UNKNOWN) {
		vram_vendor_type_str = "N/A";
	} else {
		get_string_from_enum_vram_vendor_type(vram_info.vram_vendor, vram_vendor_type_str);
	}

	std::string vram_size_mb_string{ string_format("%lu", vram_info.vram_size) };
	std::string vram_bit_width_string{};
	if (vram_info.vram_bit_width == UINT_MAX) {
		vram_bit_width_string = "N/A";
	} else {
		vram_bit_width_string = string_format("%u",vram_info.vram_bit_width);
	}

	if (arg.output == json) {
		nlohmann::ordered_json vram_size{};
		vram_size["value"] = vram_info.vram_size;
		vram_size["unit"] = vram_size_mb_string == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json vram_info_json = { { "type", vram_type_str.c_str() },
			{ "vendor", vram_vendor_type_str.c_str() },
			{ "size",  vram_size },
			{ "bit_width", vram_info.vram_bit_width }
		};

		if (vram_info.vram_bit_width == UINT_MAX) {
			vram_info_json["bit_width"] = vram_bit_width_string;
		} else {
			vram_info_json["bit_width"] = vram_info.vram_bit_width;
		}

		formatted_string = vram_info_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s", vram_type_str.c_str(),
							   vram_vendor_type_str.c_str(),
							   vram_size_mb_string.c_str(),
							   vram_bit_width_string.c_str());
	} else {
		std::string vram_size_mb_string_unit = vram_size_mb_string == "N/A" ? "" : "MB";
		formatted_string = string_format(
							   staticVramTemplate, vram_type_str.c_str(),
							   vram_vendor_type_str.c_str(),
							   vram_size_mb_string.c_str(), vram_size_mb_string_unit.c_str(),
							   vram_bit_width_string.c_str());

	}

	return ret;
}
int AmdSmiApiHost::amdsmi_get_cache_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	auto cache_list_json = nlohmann::ordered_json::array();
	amdsmi_gpu_cache_info_t cache_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_cache_info(processor, &cache_info);
	if(arg.output == human) {
		formatted_string = staticCacheHeaderTemplate;
	}

	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_cache_info(arg, "N/A");
		return ret;
	}

	for(uint8_t i = 0; i < cache_info.num_cache_types; i++) {

		std::vector<std::string> properties;
		transform_cache_properties(cache_info.cache[i].cache_properties, properties);

		std::string cache_properties_string{};

		std::string cache_size_string{
			string_format("%u", cache_info.cache[i].cache_size)
		};
		std::string cache_level_string{
			string_format("%u", cache_info.cache[i].cache_level)
		};
		std::string max_num_cu_shared_string{
			string_format("%u",  cache_info.cache[i].max_num_cu_shared)
		};
		std::string num_cache_instance_string{
			string_format("%u", cache_info.cache[i].num_cache_instance)
		};
		if(arg.output == json) {
			auto cache_properties = nlohmann::ordered_json::array();
			for(uint8_t j = 0; j < properties.size(); j++) {
				cache_properties.push_back(properties[j].c_str());
			}
			nlohmann::ordered_json cache_size{};
			cache_size["value"] = cache_info.cache[i].cache_size;
			cache_size["unit"] = cache_size_string == "N/A" ? "N/A" : "KB";

			cache_list_json.push_back(nlohmann::ordered_json::object( {
				{ "cache", i},
				{ "cache_properties", cache_properties},
				{ "cache_size", cache_size},
				{ "cache_level", cache_info.cache[i].cache_level },
				{ "max_num_cu_shared", cache_info.cache[i].max_num_cu_shared },
				{ "num_cache_instance", cache_info.cache[i].num_cache_instance } }));
		} else if (arg.output == csv) {
			for(uint8_t j = 0; j < properties.size(); j++) {

				formatted_string += string_format(
										",%d,%s,%s,%s,%s,%s\n", i, properties[j].c_str(),
										cache_size_string.c_str(), cache_level_string.c_str(),
										max_num_cu_shared_string.c_str(),
										num_cache_instance_string.c_str());
			}
		} else {
			for(uint8_t j = 0; j < properties.size(); j++) {
				if(j != 0) {
					cache_properties_string+=", ";
				}
				cache_properties_string+= properties[j];
			}
			std::string cache_size_string_unit = cache_size_string == "N/A" ? "" : "KB";
			formatted_string += string_format(
									staticCacheInfoTemplate, i, cache_properties_string.c_str(),
									cache_size_string.c_str(), cache_size_string_unit.c_str(), cache_level_string.c_str(),
									max_num_cu_shared_string.c_str(),
									num_cache_instance_string.c_str());
		}
	}
	if (arg.output == json) {
		nlohmann::ordered_json cache_info_json = cache_list_json;
		formatted_string = cache_info_json.dump(4);
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_vf_info_static_command(std::string device, Arguments arg,
		std::string& out)
{
	int ret;
	amdsmi_bdf_t vf_bdf;
	amdsmi_vf_handle_t vf_handle;
	amdsmi_vf_info_t config;
	std::string gfx_timeslice_us_str;

	vf_bdf.bdf.domain_number = std::stoi(device.substr(0, 4), nullptr, 16);
	vf_bdf.bdf.bus_number = std::stoi(device.substr(5, 2), nullptr, 16);
	vf_bdf.bdf.device_number = std::stoi(device.substr(8, 2), nullptr, 16);
	vf_bdf.bdf.function_number = std::stoi(device.substr(11), nullptr, 16);

	ret = host_amdsmi_get_vf_handle_from_bdf(vf_bdf, &vf_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_vf_info(vf_handle, &config);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_vf_info(arg, "N/A");
		return ret;
	}

	if(config.gfx_timeslice != -1) {
		gfx_timeslice_us_str = string_format("%u", config.gfx_timeslice);
	} else {
		gfx_timeslice_us_str = "N/A";
	}
	std::string fb_offset_str =
		string_format("%u", config.fb.fb_offset);
	std::string fb_size_str =
		string_format("%u", config.fb.fb_size);

	std::tuple<std::string, std::string, std::string> indexes =
		getGpuVfIndexFromVfId(device);

	if (arg.output == json) {
		nlohmann::ordered_json static_vf_json;
		nlohmann::ordered_json fb_offset{};
		fb_offset["value"] = config.fb.fb_offset;
		fb_offset["unit"] = fb_offset_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json fb_size{};
		fb_size["value"] = config.fb.fb_size;
		fb_size["unit"] = fb_size_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json timeslice{};
		timeslice["value"] = gfx_timeslice_us_str.c_str();
		timeslice["unit"] = gfx_timeslice_us_str == "N/A" ? "N/A" : "us";
		static_vf_json = { { "gpu", std::stoul(std::get<0>(indexes).c_str()) }, { "vf", std::stoul(std::get<1>(indexes).c_str()) },
			{ "fb_offset", fb_offset },
			{ "fb_size", fb_size  },
			{ "gfx_timeslice", timeslice  }
		};
		out = static_vf_json.dump(4);
	} else if (arg.output == csv) {
		out += string_format(
				   ",%s,%s,%s",
				   fb_offset_str.c_str(), fb_size_str.c_str(), gfx_timeslice_us_str.c_str());
	} else {
		std::string fb_offset_str_unit = fb_offset_str == "N/A" ? "" : "MB";
		std::string fb_size_str_unit = fb_size_str == "N/A" ? "" : "MB";
		std::string gfx_timeslice_us_str_unit = gfx_timeslice_us_str == "N/A" ? "" : "us";
		out += string_format(
				   staticVfTemplate, fb_offset_str.c_str(), fb_offset_str_unit.c_str(), fb_size_str.c_str(),
				   fb_size_str_unit.c_str(),
				   gfx_timeslice_us_str.c_str(), gfx_timeslice_us_str_unit.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_static_partition_command(uint64_t processor_bdf, Arguments arg,
		std::string &out)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		host_fill_partition(arg, out);
		return ret;
	}

	amdsmi_accelerator_partition_profile_t curr_profile;
	uint32_t partition_ids[AMDSMI_MAX_ACCELERATOR_PARTITIONS];
	ret = host_amdsmi_get_partition_profile(processor, &curr_profile, partition_ids);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_partition(arg, out);
		return ret;
	}

	std::string curr_partition_type_str;
	get_string_from_enum_accelerator_partition_type(curr_profile.profile_type, curr_partition_type_str);

	std::string partition_ids_str{};
	for (int i = 0; i < curr_profile.num_partitions; i++) {
		partition_ids_str += string_format("%d", partition_ids[i]);
		if (i != curr_profile.num_partitions - 1) {
			partition_ids_str += ",";
		}
	}

	amdsmi_memory_partition_config_t memory_partition_config;
	ret = host_amdsmi_get_gpu_memory_partition_config(processor, &memory_partition_config);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_partition(arg, out);
		return ret;
	}

	std::string mp_mode_str{};
	get_string_from_enum_mp_setting(memory_partition_config.mp_mode, mp_mode_str);

	if (arg.output == json) {
		nlohmann::ordered_json static_partition_json{};
		static_partition_json["accelerator_partition"] = curr_partition_type_str;
		static_partition_json["memory_partition"] = mp_mode_str;
		auto partition_id_list_json = nlohmann::ordered_json::array();
		for (int i = 0; i < curr_profile.num_partitions; i++) {
			partition_id_list_json.push_back(partition_ids[i]);
		}
		static_partition_json["partition_id"] = partition_id_list_json;
		out = static_partition_json.dump(4);
	} else if (arg.output == csv) {
		for (int i = 0; i < curr_profile.num_partitions; i++) {
			out += string_format(",%s,%s,%d\n", curr_partition_type_str.c_str(), mp_mode_str.c_str(),
								 partition_ids[i]);
		}
	} else {
		out = string_format(staticPartitionTemplate, curr_partition_type_str.c_str(), mp_mode_str.c_str(),
							partition_ids_str.c_str());
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_soc_pstate(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	auto dpm_list_json = nlohmann::ordered_json::array();
	amdsmi_dpm_policy_t policy;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		formatted_string = host_fill_soc_pstate(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_soc_pstate(processor, &policy);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_soc_pstate(arg, "N/A");
		return ret;
	}

	if (arg.output == human) {
		formatted_string += string_format(
								staticPolicyHeaderTemplate, policy.num_supported,
								policy.cur);
	}

	for(uint8_t i = 0; i < policy.num_supported; i++) {
		std::string cache_properties_string{};

		std::string dpm_description{
			string_format("%s", policy.policies[i].policy_description)
		};

		if(arg.output == json) {
			dpm_list_json.push_back(nlohmann::ordered_json::object( {
				{ "policy_id", policy.policies[i].policy_id},
				{ "policy_description", dpm_description} }));
		} else if (arg.output == csv) {
			formatted_string += string_format(
									",%d,%d,%d,%s\n", policy.num_supported,
									policy.cur, policy.policies[i].policy_id, dpm_description.c_str());
		} else {
			formatted_string += string_format(
									staticPolicyInfoTemplate, policy.policies[i].policy_id,
									dpm_description.c_str());
		}
	}

	if (arg.output == json) {
		nlohmann::ordered_json dpm_info_json = { { "num_supported", policy.num_supported},
			{ "current_id", policy.cur},
			{ "policies", dpm_list_json}
		};
		formatted_string = dpm_info_json.dump(4);
	}

	return ret;
}
