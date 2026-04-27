/* * Copyright (C) 2023-2025 Advanced Micro Devices. All rights reserved.
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

#define MAX_CPU_SET_SIZE 16
#include <regex>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ASIC_INFO)(amdsmi_processor_handle,
		amdsmi_asic_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PCI_BANDWIDTH)(amdsmi_processor_handle,
		amdsmi_pcie_bandwidth_t *);
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
typedef amdsmi_status_t (*AMDSMI_GET_BAD_PAGE_THRESHOLD)(amdsmi_processor_handle,
		uint32_t *);
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
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_PLPD)(amdsmi_processor_handle,
		amdsmi_dpm_policy_t *);
typedef amdsmi_status_t (*AMDSMI_SET_XGMI_PLPD)(amdsmi_processor_handle,
		uint32_t);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PTL_STATE)(amdsmi_processor_handle, bool *);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_PTL_STATE)(amdsmi_processor_handle, bool);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PTL_FORMATS)(amdsmi_processor_handle,
		amdsmi_ptl_data_format_t *, amdsmi_ptl_data_format_t *);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_PTL_FORMATS)(amdsmi_processor_handle,
		amdsmi_ptl_data_format_t, amdsmi_ptl_data_format_t);
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
typedef amdsmi_status_t (*AMDSMI_GET_GPU_METRICS)(amdsmi_processor_handle, uint32_t *,
		amdsmi_metric_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VIRTUALIZATION_MODE)(amdsmi_processor_handle,
		amdsmi_virtualization_mode_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CPU_AFFINITY_WITH_SCOPE)(amdsmi_processor_handle,
		uint32_t, uint64_t *, amdsmi_affinity_scope_t);
typedef amdsmi_status_t (*AMDSMI_TOPO_GET_NUMA_NODE_NUMBER)(amdsmi_processor_handle,
		uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES_BY_TYPE)(amdsmi_socket_handle,
		processor_type_t, amdsmi_processor_handle*, uint32_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_ASIC_INFO)(amdsmi_processor_handle,
		amdsmi_nic_asic_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_BUS_INFO)(amdsmi_processor_handle,
		amdsmi_nic_bus_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_DRIVER_INFO)(amdsmi_processor_handle,
		amdsmi_nic_driver_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_NUMA_INFO)(amdsmi_processor_handle,
		amdsmi_nic_numa_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_PORT_INFO)(amdsmi_processor_handle,
		amdsmi_nic_port_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_RDMA_DEV_INFO)(amdsmi_processor_handle,
		amdsmi_nic_rdma_devices_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_DEVICE_BDF)(amdsmi_processor_handle,
		amdsmi_bdf_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;

extern AMDSMI_GET_GPU_ASIC_INFO host_amdsmi_get_gpu_asic_info;
extern AMDSMI_GET_PCIE_INFO host_amdsmi_get_pcie_info;
extern AMDSMI_GET_GPU_PCI_BANDWIDTH host_amdsmi_get_gpu_pci_bandwidth;
extern AMDSMI_GET_FB_LAYOUT host_amdsmi_get_fb_layout;
extern AMDSMI_GET_GPU_VBIOS_INFO host_amdsmi_get_gpu_vbios_info;
extern AMDSMI_GET_GPU_BOARD_INFO host_amdsmi_get_gpu_board_info;

extern AMDSMI_GET_POWER_CAP_INFO host_amdsmi_get_power_cap_info;
extern AMDSMI_GET_TEMP_METRIC host_amdsmi_get_temp_metric;
extern AMDSMI_GET_GPU_DRIVER_INFO host_amdsmi_get_gpu_driver_info;
extern AMDSMI_GET_GPU_DRIVER_MODEL host_amdsmi_get_gpu_driver_model;
extern AMDSMI_GET_GPU_RAS_FEATURE_INFO host_amdsmi_get_gpu_ras_feature_info;
extern AMDSMI_GET_BAD_PAGE_THRESHOLD host_amdsmi_get_bad_page_threshold;
extern AMDSMI_GET_DFC_FW_TABLE host_amdsmi_get_dfc_fw_table;

extern AMDSMI_GET_NUM_VF host_amdsmi_get_num_vf;
extern AMDSMI_GET_GPU_VRAM_INFO host_amdsmi_get_gpu_vram_info;
extern AMDSMI_GET_GPU_CACHE_INFO host_amdsmi_get_gpu_cache_info;
extern AMDSMI_GET_SOC_PSTATE host_amdsmi_get_soc_pstate;
extern AMDSMI_SET_SOC_PSTATE host_amdsmi_set_soc_pstate;
extern AMDSMI_GET_XGMI_PLPD host_amdsmi_get_xgmi_plpd;
extern AMDSMI_SET_XGMI_PLPD host_amdsmi_set_xgmi_plpd;
extern AMDSMI_GET_GPU_PTL_STATE host_amdsmi_get_gpu_ptl_state;
extern AMDSMI_SET_GPU_PTL_STATE host_amdsmi_set_gpu_ptl_state;
extern AMDSMI_GET_GPU_PTL_FORMATS host_amdsmi_get_gpu_ptl_formats;
extern AMDSMI_SET_GPU_PTL_FORMATS host_amdsmi_set_gpu_ptl_formats;

extern AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
extern AMDSMI_GET_VF_INFO host_amdsmi_get_vf_info;

extern AMDSMI_GET_GPU_ECC_ENABLED host_amdsmi_get_gpu_ecc_enabled;

extern AMDSMI_GET_CURR_ACCELERATOR_PARTITION host_amdsmi_get_partition_profile;
extern AMDSMI_GET_MEMORY_PARTITION_CONFIG host_amdsmi_get_gpu_memory_partition_config;

extern AMDSMI_GET_GPU_METRICS host_amdsmi_get_gpu_metrics;

extern AMDSMI_GET_GPU_VIRTUALIZATION_MODE host_amdsmi_get_gpu_virtualization_mode;

extern AMDSMI_GET_CPU_AFFINITY_WITH_SCOPE host_amdsmi_get_cpu_affinity_with_scope;
extern AMDSMI_TOPO_GET_NUMA_NODE_NUMBER host_amdsmi_topo_get_numa_node_number;
extern AMDSMI_GET_PROCESSOR_HANDLES_BY_TYPE host_amdsmi_get_processor_handles_by_type;
extern AMDSMI_GET_NIC_ASIC_INFO host_amdsmi_get_nic_asic_info;
extern AMDSMI_GET_NIC_BUS_INFO host_amdsmi_get_nic_bus_info;
extern AMDSMI_GET_NIC_DRIVER_INFO host_amdsmi_get_nic_driver_info;
extern AMDSMI_GET_NIC_NUMA_INFO host_amdsmi_get_nic_numa_info;
extern AMDSMI_GET_NIC_PORT_INFO host_amdsmi_get_nic_port_info;
extern AMDSMI_GET_NIC_RDMA_DEV_INFO host_amdsmi_get_nic_rdma_dev_info;
extern AMDSMI_GET_NIC_DEVICE_BDF host_amdsmi_get_nic_device_bdf;


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
			{ "version", value.c_str() },
			{ "boot_firmware",  value.c_str() }
		};

		formatted_string = vbios_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s", value.c_str(), value.c_str(),
							   value.c_str(), value.c_str(), value.c_str());
	} else {
		formatted_string = string_format(
							   staticVbiosTemplate, value.c_str(), value.c_str(),
							   value.c_str(), value.c_str(), value.c_str());
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
			{ "bad_page_threshold", value.c_str() },
			{ "ecc_correction_schema", value.c_str() }
		};

		nlohmann::ordered_json ras_json;
		ras_json["feature"] = feature_json;

		out = ras_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s", value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(
				  staticRasTemplateHost, value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str() );
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
		nlohmann::ordered_json max_vram_bandwidth{};
		max_vram_bandwidth["value"] = value.c_str();
		max_vram_bandwidth["unit"] = "N/A";
		nlohmann::ordered_json vram_info_json = { { "vram_type", value.c_str() },
			{ "vram_vendor", value.c_str()},
			{ "vram_size",  vram_size},
			{ "vram_bit_width",  value.c_str()},
			{ "max_vram_bandwidth",  max_vram_bandwidth}
		};

		out = vram_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s", value.c_str(),
				  value.c_str(),
				  value.c_str(),
				  value.c_str(),
				  value.c_str());
	} else {
		out = string_format(
				  staticVramTemplate, value.c_str(),
				  value.c_str(),
				  value.c_str(),
				  value.c_str(),
				  value.c_str(), value.c_str(), " ");
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
		nlohmann::ordered_json dpm_info_json = {
			{ "num_supported", "N/A" },
			{ "current_id", "N/A" },
			{ "policies", dpm_list_json }
		};
		out = dpm_info_json.dump(4);
	} else if (arg.output == csv) {
		out += string_format("%s,%s,%s,%s",
							 value.c_str(), value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(staticPolicyHeaderTemplate, value.c_str(), value.c_str());
		out += string_format(staticPolicyInfoTemplate, value.c_str(), value.c_str(), "[]");
	}

	return out;
}

std::string host_fill_virtualization_mode(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		out = value;
	} else if(arg.output == csv) {
		out = string_format(",%s", value);
	} else {
		out = string_format(staticVirtualizationModeTemplate, value);
	}

	return out;
}

std::string host_fill_numa(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json numa_info_json{};
		numa_info_json["node"] = value.c_str();
		numa_info_json["cpu_affinity"] = "N/A";
		numa_info_json["socket_affinity"] = "N/A";
		out = numa_info_json.dump(4);
	} else if(arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s", value.c_str(), "N/A",
							"N/A", "N/A", "N/A");
	} else {
		out = string_format(staticNumaTemplate_NA, value.c_str());
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
	std::string num_of_compute_units{};
	if (asic.num_of_compute_units == UINT_MAX) {
		num_of_compute_units = "N/A";
	} else {
		num_of_compute_units = string_format("%ld", asic.num_of_compute_units);
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
			{ "oam_id", asic.oam_id },
			{ "num_of_compute_units", num_of_compute_units.c_str()}
		};

		if (asic.oam_id == UINT_MAX) {
			asic_json["oam_id"] = oam_id;
		} else {
			asic_json["oam_id"] = asic.oam_id;
		}

		if (asic.num_of_compute_units == UINT_MAX) {
			asic_json["num_of_compute_units"] = num_of_compute_units;
		} else {
			asic_json["num_of_compute_units"] = asic.num_of_compute_units;
		}

		out = asic_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
							asic.market_name,
							vendor_id_hex.c_str(), vendor_name.c_str(),
							subvendor_id_hex.c_str(), device_id_hex.c_str(),
							subsystem_id_hex.c_str(), rev_id_hex.c_str(),
							serial_id_hex.c_str(), oam_id.c_str(), num_of_compute_units.c_str());
	} else {
		out = string_format(
				  staticAsicTemplate, asic.market_name, vendor_id_hex.c_str(),
				  vendor_name.c_str(), subvendor_id_hex.c_str(), device_id_hex.c_str(), subsystem_id_hex.c_str(),
				  rev_id_hex.c_str(),
				  serial_id_hex.c_str(), oam_id.c_str(), num_of_compute_units.c_str());
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

	amdsmi_pcie_bandwidth_t pcie_bw;
	int bw_ret = host_amdsmi_get_gpu_pci_bandwidth(processor, &pcie_bw);

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

		if (bw_ret == AMDSMI_STATUS_SUCCESS && pcie_bw.transfer_rate.num_supported > 0) {
			nlohmann::ordered_json levels_json = nlohmann::ordered_json::array();
			for (uint32_t i = 0; i < pcie_bw.transfer_rate.num_supported; i++) {
				double rate_gts = pcie_bw.transfer_rate.frequency[i] / 1000000000.0;
				nlohmann::ordered_json pcie_speed{};
				pcie_speed["value"] = rate_gts;
				pcie_speed["unit"] = "GT/s";
				nlohmann::ordered_json level = {
					{ "level", i },
					{ "speed", pcie_speed },
					{ "width", pcie_bw.lanes[i] }
				};
				levels_json.push_back(level);
			}
			bus_json["pcie_levels"] = levels_json;
		}

		formatted_string = bus_json.dump(4);
	} else if (arg.output == csv) {
		std::string base_csv = string_format(",%s,%s,%s,%s,%s,%s", bdf_string.c_str(),
										 pcie_lanes.c_str(), pcie_info_GTs_value_string.c_str(),
										 pcie_interface_version.c_str(), pcie_slot_type.c_str(), max_pcie_interface_version.c_str());
		if (bw_ret == AMDSMI_STATUS_SUCCESS && pcie_bw.transfer_rate.num_supported > 0) {
			for (uint32_t i = 0; i < pcie_bw.transfer_rate.num_supported; i++) {
				double rate_gts = pcie_bw.transfer_rate.frequency[i] / 1000000000.0;
				if (i == 0) {
					formatted_string = string_format("%s,%u,%g,%u", base_csv.c_str(),
						i, rate_gts, pcie_bw.lanes[i]);
				} else {
					formatted_string += "\n";
					formatted_string += string_format("%s,%u,%g,%u", base_csv.c_str(),
						i, rate_gts, pcie_bw.lanes[i]);
				}
			}
		} else {
			formatted_string = base_csv;
		}
	} else {
		std::string pcie_info_GTs_value_string_unit = pcie_info_GTs_value_string == "N/A" ? "" : "GT/s";
		formatted_string = string_format(
							   staticBusTemplate, bdf_string.c_str(), pcie_lanes.c_str(),
							   pcie_info_GTs_value_string.c_str(), pcie_info_GTs_value_string_unit.c_str(),
							   pcie_interface_version.c_str(), pcie_slot_type.c_str(), max_pcie_interface_version.c_str());

		if (bw_ret == AMDSMI_STATUS_SUCCESS && pcie_bw.transfer_rate.num_supported > 0) {
			formatted_string += "        PCIE_LEVELS:\n";
			for (uint32_t i = 0; i < pcie_bw.transfer_rate.num_supported; i++) {
				double rate_gts = pcie_bw.transfer_rate.frequency[i] / 1000000000.0;
				formatted_string += string_format(
					"            %d: %g GT/s x%u\n", i, rate_gts, pcie_bw.lanes[i]);
			}
		}
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
			{ "version", vbios_info.version },
			{ "boot_firmware", vbios_info.boot_firmware }
		};

		formatted_string = vbios_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s", vbios_info.name, vbios_info.build_date,
							   vbios_info.part_number, vbios_info.version, vbios_info.boot_firmware);
	} else {
		formatted_string = string_format(
							   staticVbiosTemplate, vbios_info.name, vbios_info.build_date,
							   vbios_info.part_number, vbios_info.version, vbios_info.boot_firmware);
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

int AmdSmiApiHost::amdsmi_get_limit_info_command(uint64_t processor_bdf, Arguments &arg,
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

	bool ptl_enabled = false;
	bool ptl_supported = false;
	amdsmi_ptl_data_format_t format1, format2;
	std::string format1_str, format2_str;

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

	std::string ptl_status_str{};
	std::string ptl_formats_str{};

	ret = host_amdsmi_get_gpu_ptl_state(processor, &ptl_enabled);
	if (ret == AMDSMI_STATUS_SUCCESS) {
		arg.ptl_supported = true;
		ptl_supported = true;
		ptl_status_str = ptl_enabled ? "ENABLED" : "DISABLED";

		if (ptl_enabled) {
			ret = host_amdsmi_get_gpu_ptl_formats(processor, &format1, &format2);
			if (ret == AMDSMI_STATUS_SUCCESS) {
				get_string_from_enum_ptl_format(format1, format1_str);
				get_string_from_enum_ptl_format(format2, format2_str);
				if (arg.output == csv) {
					ptl_formats_str = string_format("[%s,%s]", format1_str.c_str(), format2_str.c_str());
				} else {
					ptl_formats_str = string_format("%s,%s", format1_str.c_str(), format2_str.c_str());
				}
			} else {
				return ret;
			}
		} else {
			ptl_formats_str = "N/A";
		}
	} else if (ret == AMDSMI_STATUS_NOT_SUPPORTED) {
		ret = AMDSMI_STATUS_SUCCESS;
	} else {
		return ret;
	}

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

		if (ptl_supported) {
			limit_json["ptl"] = ptl_status_str;
			limit_json["ptl_format"] = ptl_formats_str;
		}
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
							   vram_shutdown_temperature_string.c_str());
		if (ptl_supported) {
			formatted_string += string_format(",%s,%s", ptl_status_str.c_str(), ptl_formats_str.c_str());
		}
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
							   vram_shutdown_temperature_string.c_str(),vram_shutdown_temperature_string_unit.c_str());
		if (ptl_supported) {
			formatted_string += string_format("        PTL: %s\n        PTL_FORMAT: %s\n", ptl_status_str.c_str(), ptl_formats_str.c_str());
		}
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
	int bad_page_threshold_ret;
	int ret;
	nlohmann::ordered_json ras_json;

	amdsmi_ras_feature_t ras_feature;
	uint32_t bad_page_threshold;
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
	bad_page_threshold_ret = host_amdsmi_get_bad_page_threshold(processor, &bad_page_threshold);

	std::vector<std::string> ecc_correction_schema;
	std::string ras_eeprom_version_str;
	std::string bad_page_threshold_str{ string_format("%u", bad_page_threshold) };
	std::vector<std::string> schema{"parity_schema","single_bit_schema","double_bit_schema","poison_schema"};
	if (ret_ras_info != AMDSMI_STATUS_SUCCESS || bad_page_threshold_ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_ras_info(arg, "N/A");
	} else {
		ras_eeprom_version_str = string_format("0x%X", ras_feature.ras_eeprom_version);
		transform_ecc_correction_schema(
			ras_feature.ecc_correction_schema_flag, ecc_correction_schema);

		nlohmann::ordered_json supperted_schemas_json;
		nlohmann::ordered_json gpu_blocks_json;

		if (arg.output == json) {
			ras_json["eeprom_version"] = ras_eeprom_version_str.c_str();
			ras_json["bad_page_threshold"] = bad_page_threshold_str.c_str();
			for(int i = 0; i < ecc_correction_schema.size(); i++) {
				ras_json[schema[i].c_str()] = ecc_correction_schema[i].c_str();
			}
		}
		if(arg.output == human) {
			formatted_string = string_format(
								   staticRasTemplateHost, ras_eeprom_version_str.c_str(), bad_page_threshold_str.c_str(), ecc_correction_schema[0].c_str(),
								   ecc_correction_schema[1].c_str()
								   ,ecc_correction_schema[2].c_str(),ecc_correction_schema[3].c_str());
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
			for(int i = 0; i < ecc_correction_schema.size(); i++) {
				formatted_string += string_format(
										",%s,%s,%s,%s,%s,%s\n", block_str.c_str(), status.c_str(), ras_eeprom_version_str.c_str(), bad_page_threshold_str.c_str(),
										schema[i].c_str(), ecc_correction_schema[i].c_str());
			}
		} else {
			formatted_string.append(string_format(staticRasBlockTemplate, block_str.c_str(), status.c_str()));
		}
	}
	if (arg.output == json) {
		ras_json["block_state"] = blocks_values;
		formatted_string = ras_json.dump(4);
	}

	return (ret_ras_info == AMDSMI_STATUS_SUCCESS || bad_page_threshold_ret == AMDSMI_STATUS_SUCCESS
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

	std::string dfc_gart_wr_guest_min_str = AmdSmiPlatform::getInstance().getInstance().is_nv() ?
											string_format("%ld", dfc_info.header.dfc_gart_wr_guest_min) :
											"N/A";
	std::string dfc_gart_wr_guest_max_str = AmdSmiPlatform::getInstance().getInstance().is_nv() ?
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
			if (AmdSmiPlatform::getInstance().getInstance().is_nv()) {
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
				if (AmdSmiPlatform::getInstance().getInstance().is_nv()) {
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
		if (AmdSmiPlatform::getInstance().getInstance().is_nv()) {
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
	std::string vram_size_mb_string{ string_format("%lu", vram_info.vram_size) };
	std::string vram_bit_width_string{};
	if (vram_info.vram_bit_width == UINT_MAX) {
		vram_bit_width_string = "N/A";
	} else {
		vram_bit_width_string = string_format("%u",vram_info.vram_bit_width);
	}

	std::string max_vram_bw_str{"N/A"};
	std::string max_vram_bw_unit{""};
	uint64_t max_vram_bw{UINT_MAX};
	if (AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi350()) {
		std::vector<amdsmi_metric_t> max_bw{};
		amdsmi_metric_t *metrics;
		uint32_t metric_size = AMDSMI_MAX_NUM_METRICS;

		ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, NULL);
		if (ret == AMDSMI_STATUS_SUCCESS) {
			metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t)*metric_size);
			ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, &metrics[0]);

			if (ret == AMDSMI_STATUS_SUCCESS) {
				auto it = std::find_if(metrics, metrics + metric_size, [](const amdsmi_metric_t& metric) {
					return metric.name == AMDSMI_METRIC_NAME_MAX_DRAM_BANDWIDTH
						   && !(metric.flags & AMDSMI_METRIC_TYPE_ACC);
				});

				if (it != metrics + metric_size) {
					max_vram_bw = it->val;
					max_vram_bw_str = string_format("%d", max_vram_bw);
					max_vram_bw_unit = "GB/s";
				}

			}
			free(metrics);
		}
	}

	if (arg.output == json) {
		nlohmann::ordered_json vram_size{};
		vram_size["value"] = vram_info.vram_size;
		vram_size["unit"] = vram_size_mb_string == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json vram_max_bandwidth{};
		if (max_vram_bw != UINT_MAX) {
			vram_max_bandwidth["value"] = max_vram_bw;
			vram_max_bandwidth["unit"] = max_vram_bw_unit;
		} else {
			vram_max_bandwidth["value"] = "N/A";
			vram_max_bandwidth["unit"] = "N/A";
		}
		nlohmann::ordered_json vram_info_json = { { "type", vram_type_str.c_str() },
			{ "vendor", vram_info.vram_vendor == "UNKNOWN" ? "N/A" :
				vram_info.vram_vendor },
			{ "size",  vram_size },
			{ "bit_width", vram_info.vram_bit_width },
			{ "max_bandwidth", vram_max_bandwidth }
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
							   vram_info.vram_vendor,
							   vram_size_mb_string.c_str(),
							   vram_bit_width_string.c_str(), max_vram_bw_str.c_str());
	} else {
		std::string vram_size_mb_string_unit = vram_size_mb_string == "N/A" ? "" : "MB";
		formatted_string = string_format(
							   staticVramTemplate, vram_type_str.c_str(),
							   vram_info.vram_vendor,
							   vram_size_mb_string.c_str(), vram_size_mb_string_unit.c_str(),
							   vram_bit_width_string.c_str(), max_vram_bw_str.c_str(), max_vram_bw_unit.c_str());
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

	std::string num_supported_str = string_format("%d", policy.num_supported);
	std::string curr_str = string_format("%d", policy.current);

	if (arg.output == human) {
		formatted_string += string_format(staticPolicyHeaderTemplate, num_supported_str.c_str(),
						  curr_str.c_str());
	}

	for (uint8_t i = 0; i < policy.num_supported; i++) {
		std::string dpm_description = string_format("%s", policy.policies[i].policy_description);
		std::string policy_id_str = string_format("%d", policy.policies[i].policy_id);

		if (arg.output == json) {
			dpm_list_json.push_back(nlohmann::ordered_json::object({
				{ "policy_id", policy.policies[i].policy_id },
				{ "policy_description", dpm_description }
			}));
		} else if (arg.output == csv) {
			formatted_string += string_format(
									",%s,%s,%s,%s\n", num_supported_str.c_str(),
									curr_str.c_str(), policy_id_str.c_str(), dpm_description.c_str());
		} else {
			formatted_string += string_format(
									staticPolicyInfoTemplate, policy_id_str.c_str(),
									dpm_description.c_str());
		}
	}

	if (arg.output == json) {
		nlohmann::ordered_json dpm_info_json = { { "num_supported", policy.num_supported},
			{ "current_id", policy.current},
			{ "policies", dpm_list_json}
		};
		formatted_string = dpm_info_json.dump(4);
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_plpd(uint64_t processor_bdf, Arguments arg,
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
		return ret;
	}

	ret = host_amdsmi_get_xgmi_plpd(processor, &policy);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_soc_pstate(arg, "N/A");
		return ret;
	}

	if (arg.output == human) {
		formatted_string += string_format(
								staticPlpdsHeaderTemplate, policy.num_supported,
								policy.current);
	}

	for(uint8_t i = 0; i < policy.num_supported; i++) {
		std::string dpm_description = string_format("%s", policy.policies[i].policy_description);
		std::string policy_id_str = string_format("%d", policy.policies[i].policy_id);
		if(arg.output == json) {
			dpm_list_json.push_back(nlohmann::ordered_json::object( {
				{ "policy_id", policy.policies[i].policy_id},
				{ "policy_description", dpm_description} }));
		} else if (arg.output == csv) {
			formatted_string += string_format(
									",%d,%d,%d,%s\n", policy.num_supported,
									policy.current, policy_id_str.c_str(), dpm_description.c_str());
		} else {
			formatted_string += string_format(
									staticPolicyInfoTemplate, policy_id_str.c_str(),
									dpm_description.c_str());
		}
	}

	if (arg.output == json) {
		nlohmann::ordered_json dpm_info_json = {
			{ "num_supported", policy.num_supported },
			{ "current_id", policy.current },
			{ "policies", dpm_list_json }
		};
		formatted_string = dpm_info_json.dump(4);
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_virtualization_mode_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_virtualization_mode_t mode;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_virtualization_mode(processor, &mode);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_virtualization_mode(arg, "N/A");
		return ret;
	}

	std::string virtualization_mode_string;
	switch (mode) {
	case AMDSMI_VIRTUALIZATION_MODE_HOST:
		virtualization_mode_string = "HOST";
		break;
	case AMDSMI_VIRTUALIZATION_MODE_GUEST:
		virtualization_mode_string = "GUEST";
		break;
	case AMDSMI_VIRTUALIZATION_MODE_PASSTHROUGH:
		virtualization_mode_string = "PASSTHROUGH";
		break;
	default:
		virtualization_mode_string = "N/A";
		break;
	}

	if (arg.output == json) {
		formatted_string = virtualization_mode_string;
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s", virtualization_mode_string.c_str());
	} else {
		formatted_string = string_format(staticVirtualizationModeTemplate,
										 virtualization_mode_string.c_str());
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_numa_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	uint32_t numa_node;
#ifdef _WIN64
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	long num_processors = sysInfo.dwNumberOfProcessors;
#else
	long num_processors = sysconf(_SC_NPROCESSORS_CONF);
#endif
	uint32_t cpu_set_size_loc = (num_processors + 63) / 64;  // Ceiling division
	amdsmi_affinity_scope_t scope = AMDSMI_AFFINITY_SCOPE_NODE;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;
	auto cpu_list = nlohmann::ordered_json::array();
	std::string formatted_substring = "";

	if (cpu_set_size_loc < 1)
		cpu_set_size_loc = 1;
	if (cpu_set_size_loc > MAX_CPU_SET_SIZE)
		cpu_set_size_loc = MAX_CPU_SET_SIZE;

	uint64_t *cpu_set = (uint64_t*)calloc(cpu_set_size_loc, sizeof(uint64_t));
	if (!cpu_set) {
		Logger::getInstance().log(LogLevel::Error, AMDSMI_STATUS_OUT_OF_RESOURCES,
								  __FUNCTION__, __FILE__, __LINE__);
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(cpu_set);
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_topo_get_numa_node_number(processor, &numa_node);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_numa(arg, "N/A");
		free(cpu_set);

		return ret;
	}

	ret = host_amdsmi_get_cpu_affinity_with_scope(processor, cpu_set_size_loc, cpu_set, scope);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_numa(arg, string_format("%d", numa_node));
	} else {
		for(int i = 0; i < cpu_set_size_loc; ++i) {
			auto maskRanges = bitmaskToRangesList(cpu_set[i],
												  sizeof(cpu_set[0]) * CHAR_BIT * i);
			uint64_t mask{0};
			std::vector<std::string> ranges_vec{};
			nlohmann::ordered_json numajson{};
			for (const auto& [subMask, rangeStr] : maskRanges) {
				if (arg.output == csv) {
					formatted_substring =
						string_format(",%016lx,%s", subMask, rangeStr.c_str());
					formatted_string += string_format(",%d,%d", numa_node, i)
										+ formatted_substring + ",N/A\n";
				} else {
					mask |= subMask;
					ranges_vec.push_back(rangeStr);
				}
			}
			if (arg.output == json) {
				cpu_list.push_back(nlohmann::ordered_json::object( {
					{ "bitmask", string_format("%016lx", mask)},
					{ "core_range", ranges_vec}
				}));
			} else if (arg.output == human) {
				std::string joined_range;
				for (size_t idx = 0; idx < ranges_vec.size(); ++idx) {
					if (idx > 0) joined_range += ", ";
					joined_range += ranges_vec[idx];
				}
				joined_range = string_format("[%s]", joined_range.c_str());
				formatted_substring += string_format(staticCpuListTemplate,
													 i, mask, joined_range.c_str());
			}
		}
		if (arg.output == json) {
			nlohmann::ordered_json numa_info_json{};
			nlohmann::ordered_json cpu_affinity{};
			cpu_affinity["cpu_list"] = cpu_list;
			numa_info_json["node"] = numa_node;
			numa_info_json["cpu_affinity"] = cpu_affinity;
			numa_info_json["socket_affinity"] = "N/A";

			formatted_string = numa_info_json.dump(4);
		} else if (arg.output == human) {
			formatted_string = string_format(staticNumaTemplate,
											 numa_node,
											 formatted_substring.c_str());
		}
	}
	free(cpu_set);

	return ret;
}
std::string host_fill_nic_asic_info(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json asic_json = {
			{ "vendor_id", value.c_str() },
			{ "subvendor_id", value.c_str() },
			{ "device_id", value.c_str() },
			{ "subsystem_id", value.c_str() },
			{ "revision", value.c_str() },
			{ "permanent_address", value.c_str() },
			{ "product_name", value.c_str() },
			{ "part_number", value.c_str() },
			{ "serial_number", value.c_str() },
			{ "vendor_name", value.c_str() }
		};

		out = asic_json.dump(4);
	} else if (arg.output == human) {
		out = string_format(
				  nicStaticAsicTemplate, value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str());
	}

	return out;
}
std::string host_fill_nic_bus_info(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json max_pcie_speed{};
		max_pcie_speed["value"] = value.c_str();
		max_pcie_speed["unit"] = value.c_str();
		nlohmann::ordered_json bus_json = {
			{ "bdf", value.c_str() },
			{ "max_pcie_width", value.c_str() },
			{ "max_pcie_speed", max_pcie_speed },
			{ "pcie_interface_version", value.c_str() },
			{ "slot_type", value.c_str() }
		};

		out = bus_json.dump(4);
	} else if (arg.output == human) {
		out = string_format(
				  nicStaticBusTemplate, value.c_str(), value.c_str(), value.c_str(), "", value.c_str(), value.c_str());
	}

	return out;
}
std::string host_fill_nic_driver_info(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json driver_json = {
			{ "name", value.c_str() },
			{ "version", value.c_str() }
		};

		out = driver_json.dump(4);
	} else if (arg.output == human) {
		out = string_format(
				  nicStaticDriverTemplate, value.c_str(), value.c_str());
	}

	return out;
}
std::string host_fill_nic_numa_info(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json numa_json = {
			{ "node", value.c_str() },
			{ "affinity", value.c_str() }
		};

		out = numa_json.dump(4);
	} else if (arg.output == human) {
		out = string_format(
				  nicStaticNumaTemplate, value.c_str(), value.c_str());
	}

	return out;
}
std::string host_fill_nic_port_info(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		auto ports_json = nlohmann::ordered_json::array();
		nlohmann::ordered_json mtu{};
		mtu["value"] = value.c_str();
		mtu["unit"] = value.c_str();
		nlohmann::ordered_json link_speed{};
		link_speed["value"] = value.c_str();
		link_speed["unit"] = value.c_str();
		nlohmann::ordered_json port_json = {
			{ "bdf", value.c_str() },
			{ "port_num", value.c_str() },
			{ "type", value.c_str() },
			{ "flavour", value.c_str() },
			{ "netdev", value.c_str() },
			{ "ifindex", value.c_str() },
			{ "mac_address", value.c_str() },
			{ "carrier", value.c_str() },
			{ "mtu", mtu },
			{ "link_state", value.c_str() },
			{ "link_speed", link_speed },
			{ "active_fec", value.c_str() },
			{ "autoneg", value.c_str() },
			{ "pause_autoneg", value.c_str() },
			{ "pause_rx", value.c_str() },
			{ "pause_tx", value.c_str() }
		};
		ports_json.push_back(port_json);

		out = ports_json.dump(4);
	} else if (arg.output == human) {
		out = nicStaticPortHeaderTemplate;
		out += string_format(
				  nicStaticPortTemplate, 0, value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), "", value.c_str(), value.c_str(), "",
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str());
	}

	return out;
}
std::string host_fill_nic_rdma_dev_info(Arguments arg, std::string value)
{
	std::string formatted_string{};
	std::string out{};
	if (arg.output == json) {
		auto rdma_devices_json = nlohmann::ordered_json::array();
		auto rdma_ports_json = nlohmann::ordered_json::array();
		nlohmann::ordered_json rdma_port_json = {
			{ "netdev", value.c_str() },
			{ "state", value.c_str() },
			{ "rdma_port", value.c_str() },
			{ "max_mtu", value.c_str() },
			{ "active_mtu", value.c_str() }
		};
		rdma_ports_json.push_back(rdma_port_json);
		nlohmann::ordered_json rdma_device_json = {
			{ "rdma_dev", value.c_str() },
			{ "node_guid", value.c_str() },
			{ "node_type", value.c_str() },
			{ "sys_image_guid", value.c_str() },
			{ "fw_ver", value.c_str() },
			{ "ports", rdma_ports_json }
		};
		rdma_devices_json.push_back(rdma_device_json);

		out = rdma_devices_json.dump(4);
	} else if (arg.output == human) {
		formatted_string = nicStaticRdmaDevHeaderTemplate;
		formatted_string.append(string_format(
			nicStaticRdmaDevTemplate, 0, value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str()
		));
		formatted_string.append(string_format(
			nicStaticRdmaPortTemplate, 0, value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str()
		));
		out.append(formatted_string);
	}

	return out;
}
int AmdSmiApiHost::amdsmi_get_nic_asic_info_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	int ret;
	amdsmi_nic_asic_info_t nic_asic_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	ret = host_amdsmi_get_nic_asic_info(processor, &nic_asic_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			return ret;
		}
		out = host_fill_nic_asic_info(arg, "N/A");
		return ret;
	}
	std::string vendor_id_hex = string_format("0x%X", nic_asic_info.vendor_id);
	std::string subvendor_id_hex = string_format("0x%X", nic_asic_info.subvendor_id);
	std::string device_id_hex = string_format("0x%X", nic_asic_info.device_id);
	std::string subsystem_id_hex = string_format("0x%X", nic_asic_info.subsystem_id);
	std::string revision_hex = string_format("0x%X", nic_asic_info.revision);

	if (arg.output == json) {
		nlohmann::ordered_json asic_json = {
			{ "vendor_id", vendor_id_hex },
			{ "subvendor_id", subvendor_id_hex },
			{ "device_id", device_id_hex },
			{ "subsystem_id", subsystem_id_hex },
			{ "revision", revision_hex },
			{ "permanent_address", nic_asic_info.permanent_address },
			{ "product_name", nic_asic_info.product_name },
			{ "part_number", nic_asic_info.part_number },
			{ "serial_number", nic_asic_info.serial_number },
			{ "vendor_name", nic_asic_info.vendor_name }
		};

		out = asic_json.dump(4);
	} else if (arg.output == human) {
		out = string_format(
			nicStaticAsicTemplate, vendor_id_hex.c_str(), subvendor_id_hex.c_str(), device_id_hex.c_str(), 
			subsystem_id_hex.c_str(), revision_hex.c_str(), nic_asic_info.permanent_address,
			nic_asic_info.product_name, nic_asic_info.part_number, nic_asic_info.serial_number, nic_asic_info.vendor_name);
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_nic_bus_info_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	int ret;
	amdsmi_nic_bus_info_t nic_bus_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	ret = host_amdsmi_get_nic_bus_info(processor, &nic_bus_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			return ret;
		}
		out = host_fill_nic_bus_info(arg, "N/A");
		return ret;
	}
	std::string pcie_interface_version_str{
		nic_bus_info.pcie_interface_version == "" ? "N/A" : nic_bus_info.pcie_interface_version
	};
	std::string slot_type_str{
		nic_bus_info.slot_type == "" ? "N/A" : nic_bus_info.slot_type
	};

	std::string max_pcie_speed_str{
		string_format("%u", nic_bus_info.max_pcie_speed)
	};

	std::string bdf_str = convert_bdf_to_string(
		nic_bus_info.bdf.bdf.function_number,
		nic_bus_info.bdf.bdf.device_number,
		nic_bus_info.bdf.bdf.bus_number,
		nic_bus_info.bdf.bdf.domain_number
	);

	if (arg.output == json) {
		nlohmann::ordered_json max_pcie_speed{};
		max_pcie_speed["value"] = nic_bus_info.max_pcie_speed;
		max_pcie_speed["unit"] = max_pcie_speed_str == "N/A" ? "N/A" : "GT/s";

		nlohmann::ordered_json bus_json = {
			{ "bdf", bdf_str },
			{ "max_pcie_width", nic_bus_info.max_pcie_width },
			{ "max_pcie_speed", max_pcie_speed },
			{ "pcie_interface_version", pcie_interface_version_str.c_str() },
			{ "slot_type", slot_type_str.c_str() }
		};

		out = bus_json.dump(4);
	} else if (arg.output == human) {
		std::string max_pcie_speed_str_unit{
			max_pcie_speed_str == "N/A" ? "" : "GT/s"
		};

		std::string max_pcie_width_str {
			string_format("%u", nic_bus_info.max_pcie_width)
		};

		out = string_format(
			nicStaticBusTemplate, bdf_str.c_str(), max_pcie_width_str.c_str(), max_pcie_speed_str.c_str(), max_pcie_speed_str_unit.c_str(), pcie_interface_version_str.c_str(), slot_type_str.c_str()
		);
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_nic_driver_info_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	int ret;
	amdsmi_nic_driver_info_t nic_driver_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	ret = host_amdsmi_get_nic_driver_info(processor, &nic_driver_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			return ret;
		}
		out = host_fill_nic_driver_info(arg, "N/A");
		return ret;
	}
	if (arg.output == json) {
		nlohmann::ordered_json driver_json = {
			{ "name", nic_driver_info.name },
			{ "version", nic_driver_info.version }
		};

		out = driver_json.dump(4);
	} else if (arg.output == human) {
		out = string_format(nicStaticDriverTemplate, nic_driver_info.name, nic_driver_info.version);
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_nic_numa_info_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	int ret;
	amdsmi_nic_numa_info_t nic_numa_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	ret = host_amdsmi_get_nic_numa_info(processor, &nic_numa_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			return ret;
		}
		out = host_fill_nic_numa_info(arg, "N/A");
		return ret;
	}
	if (arg.output == json) {
		auto affinity_json = nlohmann::ordered_json::array();
		affinity_json.push_back(nic_numa_info.affinity);
		nlohmann::ordered_json numa_json = {
			{ "node", nic_numa_info.node },
			{ "affinity", affinity_json }
		};

		out = numa_json.dump(4);
	} else if (arg.output == human) {
		std::string affinity_str{
			string_format("[%s]", nic_numa_info.affinity)
		};
		std::string node_str {
			string_format("%u", nic_numa_info.node)
		};
		out = string_format(nicStaticNumaTemplate, node_str.c_str(), affinity_str.c_str());
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_nic_port_info_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	int ret;
	amdsmi_nic_port_info_t nic_port_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	std::string bdf_str;
	std::string active_fec_modes_str{};
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	ret = host_amdsmi_get_nic_port_info(processor, &nic_port_info);
	if (ret != AMDSMI_STATUS_SUCCESS || nic_port_info.num_ports == 0) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			return ret;
		}
		out = host_fill_nic_port_info(arg, "N/A");
		return AMDSMI_STATUS_SUCCESS;
	}

	if (arg.output == json) {
		auto ports_json = nlohmann::ordered_json::array();

		for (uint32_t i = 0; i < nic_port_info.num_ports; i++) {
			bdf_str = convert_bdf_to_string(
						  nic_port_info.ports[i].bdf.bdf.function_number,
						  nic_port_info.ports[i].bdf.bdf.device_number,
						  nic_port_info.ports[i].bdf.bdf.bus_number,
						  nic_port_info.ports[i].bdf.bdf.domain_number
					  );
			nlohmann::ordered_json mtu_json;
			if (nic_port_info.ports[i].mtu != UINT16_MAX) {
				mtu_json = {
					{ "value", nic_port_info.ports[i].mtu },
					{ "unit", "B" }
				};
			} else {
				mtu_json = "N/A";
			}

			nlohmann::ordered_json link_speed_json;
			if (nic_port_info.ports[i].link_speed != UINT32_MAX) {
				link_speed_json = {
					{ "value", nic_port_info.ports[i].link_speed },
					{ "unit", "Mb/s" }
				};
			} else {
				link_speed_json = "N/A";
			}

			active_fec_modes_str = ::FecModesToString(nic_port_info.ports[i].active_fec);

			nlohmann::ordered_json port_json = {
				{ "bdf", bdf_str },
				{ "port_num", nic_port_info.ports[i].port_num },
				{ "type", nic_port_info.ports[i].type },
				{ "flavour", nic_port_info.ports[i].flavour },
				{ "netdev", nic_port_info.ports[i].netdev },
				{ "ifindex", nic_port_info.ports[i].ifindex },
				{ "mac_address", nic_port_info.ports[i].mac_address },
				{ "carrier", nic_port_info.ports[i].carrier },
				{ "mtu", mtu_json },
				{ "link_state", nic_port_info.ports[i].link_state },
				{ "link_speed", link_speed_json },
				{ "active_fec", active_fec_modes_str },
				{ "autoneg", strcmp(nic_port_info.ports[i].autoneg, "on") == 0 ? "ON" : strcmp(nic_port_info.ports[i].autoneg, "off") == 0 ? "OFF" : "N/A" },
				{ "pause_autoneg", strcmp(nic_port_info.ports[i].pause_autoneg, "on") == 0 ? "ON" : strcmp(nic_port_info.ports[i].pause_autoneg, "off") == 0 ? "OFF" : "N/A" },
				{ "pause_rx", strcmp(nic_port_info.ports[i].pause_rx, "on") == 0 ? "ON" : strcmp(nic_port_info.ports[i].pause_rx, "off") == 0 ? "OFF" : "N/A" },
				{ "pause_tx", strcmp(nic_port_info.ports[i].pause_tx, "on") == 0 ? "ON" : strcmp(nic_port_info.ports[i].pause_tx, "off") == 0 ? "OFF" : "N/A" }
			};

			ports_json.push_back(port_json);
		}

		out = ports_json.dump(4);
	} else if (arg.output == human) {
		std::string active_fec_modes_str{};
		std::string formatted_output{nicStaticPortHeaderTemplate};
		for (uint32_t i = 0; i < nic_port_info.num_ports; i++) {
			bdf_str = convert_bdf_to_string(
						  nic_port_info.ports[i].bdf.bdf.function_number,
						  nic_port_info.ports[i].bdf.bdf.device_number,
						  nic_port_info.ports[i].bdf.bdf.bus_number,
						  nic_port_info.ports[i].bdf.bdf.domain_number
					  );
			std::string port_mtu_str{
				(nic_port_info.ports[i].mtu != UINT16_MAX) ? string_format("%u", nic_port_info.ports[i].mtu) : "N/A"
			};
			std::string port_mtu_str_unit{
				port_mtu_str == "N/A" ? "" : "B"
			};
			std::string port_link_speed_str{
				(nic_port_info.ports[i].link_speed != UINT32_MAX) ? string_format("%u", nic_port_info.ports[i].link_speed) : "N/A"
			};
			std::string port_link_speed_str_unit{
				port_link_speed_str == "N/A" ? "" : "Mb/s"
			};

			std::string port_num_str {
				string_format("%u", nic_port_info.ports[i].port_num)
			};

			std::string ifindex_str {
				string_format("%u", nic_port_info.ports[i].ifindex)
			};

			std::string carrier_str {
				string_format("%u", nic_port_info.ports[i].carrier)
			};

			active_fec_modes_str = ::FecModesToString(nic_port_info.ports[i].active_fec);


			formatted_output += string_format(
				nicStaticPortTemplate,
				i,
				bdf_str.c_str(),
				port_num_str.c_str(),
				nic_port_info.ports[i].type,
				nic_port_info.ports[i].flavour,
				nic_port_info.ports[i].netdev,
				ifindex_str.c_str(),
				nic_port_info.ports[i].mac_address,
				carrier_str.c_str(),
				port_mtu_str.c_str(),
				port_mtu_str_unit.c_str(),
				nic_port_info.ports[i].link_state,
				port_link_speed_str.c_str(),
				port_link_speed_str_unit.c_str(),
				active_fec_modes_str.c_str(),
				strcmp(nic_port_info.ports[i].autoneg, "on") == 0 ? "ON" : strcmp(nic_port_info.ports[i].autoneg, "off") == 0 ? "OFF" : "N/A",
				strcmp(nic_port_info.ports[i].pause_autoneg, "on") == 0 ? "ON" : strcmp(nic_port_info.ports[i].pause_autoneg, "off") == 0 ? "OFF" : "N/A",
				strcmp(nic_port_info.ports[i].pause_rx, "on") == 0 ? "ON" : strcmp(nic_port_info.ports[i].pause_rx, "off") == 0 ? "OFF" : "N/A",
				strcmp(nic_port_info.ports[i].pause_tx, "on") == 0 ? "ON" : strcmp(nic_port_info.ports[i].pause_tx, "off") == 0 ? "OFF" : "N/A"
			);
		}

		out = formatted_output;
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_nic_rdma_devices_info_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	int ret;
	std::string formatted_string{};
	amdsmi_nic_rdma_devices_info_t nic_rdma_devices_info;
	out = {};
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	ret = host_amdsmi_get_nic_rdma_dev_info(processor, &nic_rdma_devices_info);
	if (ret != AMDSMI_STATUS_SUCCESS || nic_rdma_devices_info.num_rdma_dev == 0) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			return ret;
		}
		out = host_fill_nic_rdma_dev_info(arg, "N/A");
		return AMDSMI_STATUS_SUCCESS;
	}
	std::string max_mtu_str{};
	std::string active_mtu_str{};

	if (arg.output == json) {
		auto rdma_devices_json = nlohmann::ordered_json::array();
		for (uint32_t i = 0; i < nic_rdma_devices_info.num_rdma_dev; ++i) {
			auto rdma_ports_json = nlohmann::ordered_json::array();
			for (uint32_t j = 0; j < nic_rdma_devices_info.rdma_dev_info[i].num_rdma_ports; ++j) {
				max_mtu_str = (nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].max_mtu == UINT16_MAX) ?
							  "N/A" :
							  string_format(
								  "%u", nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].max_mtu);
				active_mtu_str = (nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].active_mtu == UINT16_MAX)
								 ? "N/A" :
								 string_format(
									 "%u", nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].active_mtu);
				nlohmann::ordered_json rdma_port_json = {
					{ "netdev", nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].netdev },
					{ "state", nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].state },
					{ "rdma_port", nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].rdma_port },
					{ "max_mtu", max_mtu_str.c_str()},
					{ "active_mtu", active_mtu_str.c_str() }
				};
				rdma_ports_json.push_back(rdma_port_json);
			}
			nlohmann::ordered_json rdma_device_json = {
				{ "rdma_dev", nic_rdma_devices_info.rdma_dev_info[i].rdma_dev },
				{ "node_guid", nic_rdma_devices_info.rdma_dev_info[i].node_guid },
				{ "node_type", nic_rdma_devices_info.rdma_dev_info[i].node_type },
				{ "sys_image_guid", nic_rdma_devices_info.rdma_dev_info[i].sys_image_guid },
				{ "fw_ver", nic_rdma_devices_info.rdma_dev_info[i].fw_ver },
				{ "ports", rdma_ports_json }
			};
			rdma_devices_json.push_back(rdma_device_json);
		}

		out = rdma_devices_json.dump(4);
	} else if (arg.output == human) {
		formatted_string = nicStaticRdmaDevHeaderTemplate;
		for (uint32_t i = 0; i < nic_rdma_devices_info.num_rdma_dev; ++i) {
			formatted_string.append(string_format(
								nicStaticRdmaDevTemplate,
								i,
								nic_rdma_devices_info.rdma_dev_info[i].rdma_dev,
								nic_rdma_devices_info.rdma_dev_info[i].node_guid,
								nic_rdma_devices_info.rdma_dev_info[i].node_type,
								nic_rdma_devices_info.rdma_dev_info[i].sys_image_guid,
								nic_rdma_devices_info.rdma_dev_info[i].fw_ver
							));
			for (uint32_t j = 0; j < nic_rdma_devices_info.rdma_dev_info[i].num_rdma_ports; ++j) {
				max_mtu_str = (nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].max_mtu == UINT16_MAX) ? "N/A" :
						string_format(
							"%u", nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].max_mtu);
				active_mtu_str = (nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].active_mtu == UINT16_MAX) ? "N/A" :
						string_format(
							"%u", nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].active_mtu);
				std::string rdma_port_str {
					string_format("%u", nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].rdma_port)
				};

				formatted_string.append(string_format(
					nicStaticRdmaPortTemplate,
					j,
					nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].netdev,
					nic_rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].state,
					rdma_port_str.c_str(),
					max_mtu_str.c_str(),
					active_mtu_str.c_str()
				));
			}
			out.append(formatted_string);
		}
	}


	return ret;
}
