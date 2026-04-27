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
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_metric_command.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

#include "json/json.h"

#include <cstdint>
#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

#include <limits.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cinttypes>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
		amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle,
		amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_IS_GPU_POWER_MANAGEMENT_ENABLED)(amdsmi_processor_handle, bool *);
typedef amdsmi_status_t (*AMDSMI_GET_CLOCK_INFO)(amdsmi_processor_handle, amdsmi_clk_type_t,
		amdsmi_clk_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
		amdsmi_temperature_type_t,
		amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
		amdsmi_error_count_t *);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_COUNT)(amdsmi_processor_handle, amdsmi_gpu_block_t,
		amdsmi_error_count_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_ENABLED)(amdsmi_processor_handle,
		uint64_t *);

typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_METRICS)(amdsmi_processor_handle,
		amdsmi_link_metrics_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_vf_handle_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_DATA)(amdsmi_vf_handle_t, amdsmi_vf_data_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GUEST_DATA)(amdsmi_vf_handle_t,
		amdsmi_guest_data_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_METRICS)(amdsmi_processor_handle, uint32_t *,
		amdsmi_metric_t *);

typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint32_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CURR_ACCELERATOR_PARTITION)(amdsmi_processor_handle,
		amdsmi_accelerator_partition_profile_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_PORT_STATISTICS)(amdsmi_processor_handle, uint32_t,
		uint32_t *, amdsmi_nic_stat_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_PORT_INFO)(amdsmi_processor_handle,
		amdsmi_nic_port_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_VENDOR_STATISTICS)(amdsmi_processor_handle, uint32_t,
		uint32_t *, amdsmi_nic_stat_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_RDMA_PORT_STATISTICS)(amdsmi_processor_handle, uint32_t,
		uint32_t *, amdsmi_nic_stat_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_RDMA_DEV_INFO)(amdsmi_processor_handle,
		amdsmi_nic_rdma_devices_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_ACTIVITY host_amdsmi_get_gpu_activity;
extern AMDSMI_GET_POWER_INFO host_amdsmi_get_power_info;
extern AMDSMI_IS_GPU_POWER_MANAGEMENT_ENABLED host_amdsmi_is_gpu_power_management_enabled;
extern AMDSMI_GET_CLOCK_INFO host_amdsmi_get_clock_info;
extern AMDSMI_GET_TEMP_METRIC host_amdsmi_get_temp_metric;
extern AMDSMI_GET_GPU_TOTAL_ECC_COUNT host_amdsmi_get_gpu_total_ecc_count;

extern AMDSMI_GET_GPU_ECC_COUNT host_amdsmi_get_gpu_ecc_count;
extern AMDSMI_GET_GPU_ECC_ENABLED host_amdsmi_get_gpu_ecc_enabled;

extern AMDSMI_GET_PCIE_INFO host_amdsmi_get_pcie_info;
extern AMDSMI_GET_LINK_METRICS host_amdsmi_get_link_metrics;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
extern AMDSMI_GET_VF_DATA host_amdsmi_get_vf_data;
extern AMDSMI_GET_GUEST_DATA host_amdsmi_get_guest_data;
extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_GPU_METRICS host_amdsmi_get_gpu_metrics;

extern AMDSMI_GET_NUM_VF host_amdsmi_get_num_vf;
extern AMDSMI_GET_CURR_ACCELERATOR_PARTITION host_amdsmi_get_partition_profile;
extern AMDSMI_GET_NIC_PORT_STATISTICS host_amdsmi_get_nic_port_statistics;
extern AMDSMI_GET_NIC_PORT_INFO host_amdsmi_get_nic_port_info;
extern AMDSMI_GET_NIC_VENDOR_STATISTICS host_amdsmi_get_nic_vendor_statistics;
extern AMDSMI_GET_NIC_RDMA_PORT_STATISTICS host_amdsmi_get_nic_rdma_port_statistics;
extern AMDSMI_GET_NIC_RDMA_DEV_INFO host_amdsmi_get_nic_rdma_dev_info;

constexpr int MAX_AID_NUM{4};

const std::vector<amdsmi_gpu_block_t> ecc_blocks{AMDSMI_GPU_BLOCK_UMC, AMDSMI_GPU_BLOCK_SDMA, AMDSMI_GPU_BLOCK_GFX, AMDSMI_GPU_BLOCK_MMHUB,
		  AMDSMI_GPU_BLOCK_ATHUB, AMDSMI_GPU_BLOCK_PCIE_BIF, AMDSMI_GPU_BLOCK_HDP, AMDSMI_GPU_BLOCK_XGMI_WAFL,
		  AMDSMI_GPU_BLOCK_DF, AMDSMI_GPU_BLOCK_SMN, AMDSMI_GPU_BLOCK_SEM, AMDSMI_GPU_BLOCK_MP0,
		  AMDSMI_GPU_BLOCK_MP1, AMDSMI_GPU_BLOCK_FUSE, AMDSMI_GPU_BLOCK_MCA, AMDSMI_GPU_BLOCK_VCN,
		  AMDSMI_GPU_BLOCK_JPEG, AMDSMI_GPU_BLOCK_IH, AMDSMI_GPU_BLOCK_MPIO};

std::string host_fill_usage(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json gfx_activity{};
		gfx_activity["value"] = value.c_str();
		gfx_activity["unit"] = "N/A";
		nlohmann::ordered_json umc_activity{};
		umc_activity["value"] = value.c_str();
		umc_activity["unit"] = "N/A";
		nlohmann::ordered_json engine_usage_json = { { "gfx_activity", gfx_activity },
			{ "umc_activity", umc_activity }
		};
		nlohmann::ordered_json mm_activity{};
		mm_activity["value"] = value.c_str();
		mm_activity["unit"] = "N/A";
		engine_usage_json["mm_activity"] = mm_activity;

		out = engine_usage_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s",
							value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(
				  metricUsageTemplate, value.c_str(), "", value.c_str(), "", value.c_str(), "");
	}
	return out;
}

std::string host_fill_power(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json socket_power{};
		socket_power["value"] = value.c_str();
		socket_power["unit"] = "N/A";
		nlohmann::ordered_json power_info_json = { { "socket_power", socket_power} };

		nlohmann::ordered_json gfx_voltage{};
		gfx_voltage["value"] = value.c_str();
		gfx_voltage["unit"] = "N/A";
		power_info_json["gfx_voltage"] = gfx_voltage;

		nlohmann::ordered_json soc_voltage{};
		soc_voltage["value"] = value.c_str();
		soc_voltage["unit"] = "N/A";
		power_info_json["soc_voltage"] = soc_voltage;

		nlohmann::ordered_json mem_voltage{};
		mem_voltage["value"] = value.c_str();
		mem_voltage["unit"] = "N/A";
		power_info_json["mem_voltage"] = mem_voltage;
		power_info_json["power_management"] = value.c_str();

		out = power_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s", value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(metricPowerMeasureTemplate, value.c_str(), "", value.c_str(), "", value.c_str(),
							"",	value.c_str(),"", value.c_str());
	}
	return out;
}

std::string host_fill_clock(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json gfx_clk_json{};
		gfx_clk_json["value"] = value.c_str();
		gfx_clk_json["unit"] = "N/A";
		nlohmann::ordered_json gfx_min_clk_json{};
		gfx_min_clk_json["value"] = value.c_str();
		gfx_min_clk_json["unit"] = "N/A";
		nlohmann::ordered_json gfx_max_clk_json{};
		gfx_max_clk_json["value"] = value.c_str();
		gfx_max_clk_json["unit"] = "N/A";

		nlohmann::ordered_json mem_clk_json{};
		mem_clk_json["value"] = value.c_str();
		mem_clk_json["unit"] = "N/A";
		nlohmann::ordered_json mem_min_clk_json{};
		mem_min_clk_json["value"] = value.c_str();
		mem_min_clk_json["unit"] = "N/A";
		nlohmann::ordered_json mem_max_clk_json{};
		mem_max_clk_json["value"] = value.c_str();
		mem_max_clk_json["unit"] = "N/A";

		nlohmann::ordered_json vclk0_clk_json{};
		vclk0_clk_json["value"] = value.c_str();
		vclk0_clk_json["unit"] = "N/A";
		nlohmann::ordered_json vclk0_min_clk_json{};
		vclk0_min_clk_json["value"] = value.c_str();
		vclk0_min_clk_json["unit"] = "N/A";
		nlohmann::ordered_json vclk0_max_clk_json{};
		vclk0_max_clk_json["value"] = value.c_str();
		vclk0_max_clk_json["unit"] = "N/A";

		nlohmann::ordered_json vclk1_clk_json{};
		vclk1_clk_json["value"] = value.c_str();
		vclk1_clk_json["unit"] = "N/A";
		nlohmann::ordered_json vclk1_min_clk_json{};
		vclk1_min_clk_json["value"] = value.c_str();
		vclk1_min_clk_json["unit"] = "N/A";
		nlohmann::ordered_json vclk1_max_clk_json{};
		vclk1_max_clk_json["value"] = value.c_str();
		vclk1_max_clk_json["unit"] = "N/A";

		nlohmann::ordered_json gfx = { { "clk", gfx_clk_json },
			{ "min_clk", gfx_min_clk_json},
			{ "max_clk", gfx_max_clk_json},
			{ "clk_locked", value.c_str() }
		};

		nlohmann::ordered_json mem = { { "clk", mem_clk_json },
			{ "min_clk", mem_min_clk_json },
			{ "max_clk", mem_max_clk_json },
			{ "clk_locked", value.c_str() }
		};

		nlohmann::ordered_json vclk0 = { { "clk", vclk0_clk_json },
			{ "min_clk", vclk0_min_clk_json},
			{ "max_clk", vclk0_max_clk_json },
			{ "clk_locked", vclk0_max_clk_json }
		};

		nlohmann::ordered_json vclk1 = { { "clk", vclk1_clk_json },
			{ "min_clk", vclk1_min_clk_json },
			{ "max_clk", vclk1_max_clk_json },
			{ "clk_locked", value.c_str() }
		};
		nlohmann::ordered_json clock = { { "gfx", gfx }, { "mem", mem }, { "vclk0", vclk0 }, { "vclk1", vclk1 } };
		out = clock.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", value.c_str(),
							value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(metricClockMeasureHostTemplate, value.c_str(), "", value.c_str(), "",
							value.c_str(), "",
							value.c_str(), value.c_str(), value.c_str(), "", value.c_str(), "", value.c_str(), "",
							value.c_str(), value.c_str());

		out += string_format(metricVCLK0ClockMeasureHostTemplate, value.c_str(), "", value.c_str(), "",
							 value.c_str(), "", value.c_str(), value.c_str());

		out += string_format(metricVCLK1ClockMeasureHostTemplate, value.c_str(), "", value.c_str(), "",
							 value.c_str(), "", value.c_str(), value.c_str());
	}
	return out;
}

std::string host_fill_temperature(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json temperature_json{};

		nlohmann::ordered_json edge_temperature_json{};
		edge_temperature_json["value"] = value.c_str();
		edge_temperature_json["unit"] = "N/A";
		temperature_json["edge"] = edge_temperature_json;

		nlohmann::ordered_json hotspot_temperature_json{};
		hotspot_temperature_json["value"] = value.c_str();
		hotspot_temperature_json["unit"] =  "N/A";
		temperature_json["hotspot"] = hotspot_temperature_json;

		nlohmann::ordered_json vram_temperature_json{};
		vram_temperature_json["value"] = value.c_str();
		vram_temperature_json["unit"] =  "N/A";
		temperature_json["mem"] = vram_temperature_json;

		out = temperature_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s", value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(metricThermalMeasureTemplate, value.c_str(), "", value.c_str(), "",
							value.c_str(), "");
	}
	return out;
}

std::string host_fill_ecc(Arguments arg, std::string value)
{
	std::string out{};
	nlohmann::ordered_json error_count_json{};
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s", value.c_str(), value.c_str(), value.c_str(), value.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json error_count_json = {
			{ "total_correctable_count", value.c_str() },
			{ "total_uncorrectable_count", value.c_str()},
			{ "total_deferred_count", value.c_str()},
			{ "cache_correctable_count", value.c_str() },
			{ "cache_uncorrectable_count", value.c_str() }
		};
	} else if (arg.output == csv) {
		out = string_format( ",%s,%s,%s,%s,%s", value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							 value.c_str());
	} else {
		out = string_format(
				  metricEccErrorCountTemplate, value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str());
	}

	return out;
}

std::string host_fill_ecc_block(Arguments arg, std::string value)
{
	auto ecc_blocks_json = nlohmann::ordered_json::array();
	std::string out{};

	for (auto block : ecc_blocks) {
		std::string block_str{};
		block_str = get_string_from_enum_ecc_blocks(block);

		if (arg.output == json) {
			ecc_blocks_json.push_back(nlohmann::ordered_json::object( {
				{ "block", block_str.c_str() },
				{ "correctable_count", value.c_str() },
				{ "uncorrectable_count", value.c_str() } }));
		} else if (arg.output == csv) {
			out.append(string_format(",%s,%s,%s", value.c_str(), value.c_str(), value.c_str()));
		} else {
			out.append(string_format(metricEccBlockErrorCountTemplate, value.c_str(), value.c_str(),
									 value.c_str()));
		}
	}
	return out;
}

std::string host_fill_pcie(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json current_speed{};
		current_speed["value"] = value.c_str();
		current_speed["unit"] = "N/A";

		nlohmann::ordered_json current_bandwidth{};
		current_bandwidth["value"] = value.c_str();
		current_bandwidth["unit"] = "N/A";

		nlohmann::ordered_json pcie_info_json = { { "width", value.c_str()},
			{ "speed", current_speed },
			{ "bandwidth", current_bandwidth },
			{ "replay_count", value.c_str() },
			{ "l0_to_recovery_count", value.c_str() },
			{ "replay_roll_over_count", value.c_str() },
			{ "nak_sent_count", value.c_str() },
			{ "nak_received_count", value.c_str() }
		};

		out = pcie_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s,%s,%s", value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(
				  pcieInfoTemplate, value.c_str(),
				  value.c_str(), "", value.c_str(), "", value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str());
	}
	return out;
}

std::string host_fill_schedule(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.watch > -1) {
		out = string_format(
				  "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json boot_up_time{};
		boot_up_time["value"] = value.c_str();
		boot_up_time["unit"] = "N/A";
		nlohmann::ordered_json shutdown_time{};
		shutdown_time["value"] = value.c_str();
		shutdown_time["unit"] = "N/A";
		nlohmann::ordered_json reset_time{};
		reset_time["value"] = value.c_str();
		reset_time["unit"] = "N/A";
		nlohmann::ordered_json schedule_info_json = { { "boot_up_time", boot_up_time },
			{ "flr_count", value.c_str() },
			{ "vf_state", value.c_str() },
			{ "last_boot_start", value.c_str() },
			{ "last_boot_end", value.c_str() },
			{ "last_shutdown_start", value.c_str() },
			{ "last_shutdown_end", value.c_str() },
			{ "shutdown_time", shutdown_time },
			{ "last_reset_start", value.c_str() },
			{ "last_reset_end", value.c_str() },
			{ "reset_time", reset_time },
			{ "active_time", value.c_str() },
			{ "running_time", value.c_str() },
			{ "total_active_time", value.c_str() },
			{ "total_running_time", value.c_str() }
		};
		out = schedule_info_json.dump(4);
	} else if (arg.output == csv) {
		out += string_format(
				   ",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", value.c_str(),
				   value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				   value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				   value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(
				  metricScheduleTemplate, value.c_str(), "", value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), "", value.c_str(), value.c_str(), value.c_str(), "",
				  value.c_str(), value.c_str(), value.c_str(), value.c_str());
	}
	return out;
}

std::string host_fill_guest_data(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.watch > -1) {
		out = string_format(
				  "%s,%s", value.c_str(), value.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json fb_usage_json{};
		fb_usage_json["value"] = value.c_str();
		fb_usage_json["unit"] = "N/A";
		nlohmann::ordered_json guest_json_out = {
			{ "driver_version", value.c_str() },
			{ "fb_usage", fb_usage_json }
		};
		out = guest_json_out.dump(4);
	} else if (arg.output == csv) {
		out += string_format(
				   ",%s,%s", value.c_str(), value.c_str());
	} else {
		out = string_format(
				  metricGuestDataTemplate, value.c_str(), value.c_str(), "");
	}
	return out;
}

std::string host_fill_energy(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json energy_json{};
		energy_json["value"] = value.c_str();
		energy_json["unit"] = value.c_str();
		nlohmann::ordered_json energy_info_json = { { "total_energy_consumption", energy_json} };
		out = energy_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s", value.c_str());
	} else {
		out = string_format(metricPowerEnergyTemplate, value.c_str());
	}
	return out;
}

std::string host_fill_throttle(Arguments arg, std::string value)
{
	std::string out{};
	std::string unit_str = value == "N/A" ? "" : "%";
	std::string json_unit_str = value == "N/A" ? "N/A" : "%";
	std::string pct_str = string_format("%s %s", value.c_str(), unit_str.c_str());
	if (arg.watch > -1) {
		out = string_format(
				  "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
				  value.c_str(),
				  value.c_str(), pct_str.c_str(), value.c_str(),
				  value.c_str(), pct_str.c_str(), value.c_str(),
				  value.c_str(), pct_str.c_str(), value.c_str(),
				  value.c_str(), pct_str.c_str(), value.c_str(),
				  value.c_str(), pct_str.c_str(), value.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json activity_json;
		activity_json["value"] = value.c_str();
		activity_json["unit"] = json_unit_str.c_str();

		nlohmann::ordered_json throttle_json;
		throttle_json["accumulation_counter"] = value.c_str();
		throttle_json["prochot_violation_accumulated"] = value.c_str();
		throttle_json["prochot_violation_activity"] = activity_json;
		throttle_json["prochot_violation_status"] = value.c_str();
		throttle_json["ppt_violation_accumulated"] = value.c_str();
		throttle_json["ppt_violation_activity"] = activity_json;
		throttle_json["ppt_violation_status"] = value.c_str();
		throttle_json["socket_thermal_violation_accumulated"] = value.c_str();
		throttle_json["socket_thermal_violation_activity"] = activity_json;
		throttle_json["socket_thermal_violation_status"] = value.c_str();
		throttle_json["vr_thermal_violation_accumulated"] = value.c_str();
		throttle_json["vr_thermal_violation_activity"] = activity_json;
		throttle_json["vr_thermal_violation_status"] = value.c_str();
		throttle_json["hbm_thermal_violation_accumulated"] = value.c_str();
		throttle_json["hbm_thermal_violation_activity"] = activity_json;
		throttle_json["hbm_thermal_violation_status"] = value.c_str();

		out = throttle_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
				  value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(
				  ThrottleInfoHeaderTemplate, value.c_str(),
				  value.c_str(), value.c_str(), unit_str.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), unit_str.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), unit_str.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), unit_str.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), unit_str.c_str(), value.c_str());
	}

	return out;
}

std::string host_fill_metric_nic_rdma_dev_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		auto rdma_devices_json = nlohmann::ordered_json::array();
		auto rdma_ports_json = nlohmann::ordered_json::array();
		nlohmann::ordered_json result_json;
		nlohmann::ordered_json rdma_port_json = {
			{"statistics", value.c_str()}
		};
		rdma_ports_json.push_back(rdma_port_json);

		nlohmann::ordered_json rdma_device_json = {
			{"rdma_dev", value.c_str()},
			{"ports", rdma_ports_json}
		};
		rdma_devices_json.push_back(rdma_device_json);
		result_json["rdma_devices"] = rdma_devices_json;
		out = result_json.dump(4);
	} else if (arg.output == csv) {

	} else {
		out += metricNicRdmaStatsHeaderTemplate;
		out += string_format(metricNicRdmaDeviceTemplate, 0, value.c_str());
		out += "                    PORT_0:\n";
		out += string_format("                        STATISTICS: %s\n", value.c_str());
	}

	return out;
}

std::string host_fill_nic_port_netdev_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		auto ports_json = nlohmann::ordered_json::array();
		nlohmann::ordered_json port_json = {
			{"netdev", value.c_str()},
			{"vendor_statistics", value.c_str()},
			{"statistics", value.c_str()}
		};
		ports_json.push_back(port_json);

		nlohmann::ordered_json result_json = {
			{"ports", ports_json}
		};
		out = result_json.dump(4);
	} else if (arg.output == human) {
		out += metricNicPortStatsHeaderTemplate;
		out += string_format(metricNicPortTemplate, 0, value.c_str());
		out += string_format("            VENDOR_STATISTICS: %s\n", value.c_str());
		out += string_format("            STATISTICS: %s\n", value.c_str());
	}
	return out;
}

int AmdSmiApiHost::amdsmi_get_usage_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_engine_usage_t engine_usage;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_usage(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_gpu_activity(processor, &engine_usage);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_usage(arg, "N/A");
		return ret;
	}

	std::string mm_activity_str = (engine_usage.mm_activity == INT32_MAX) ? "N/A" :
								  string_format(
									  "%ld", engine_usage.mm_activity);
	std::string mm_activity_percent = (mm_activity_str != "N/A") ? "%" : "";
	std::string gfx_activity{ string_format(
								  "%ld", engine_usage.gfx_activity) };
	std::string gfx_activity_percent = (mm_activity_str == "N/A") ? "%" : "";
	std::string umc_activity{ string_format(
								  "%ld", engine_usage.umc_activity) };
	std::string umc_activity_percent = (mm_activity_str == "N/A") ? "%" : "";

	bool metric_table{false};
	std::vector<amdsmi_metric_t> jpeg_chiplet{};
	std::vector<amdsmi_metric_t> vcn_chiplet{};
	std::vector<amdsmi_metric_t> mm_chiplet{};
	std::vector<amdsmi_metric_t> mem_chiplet{};
	std::vector<amdsmi_metric_t> gfx_chiplet{};

	if ((AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200() || AmdSmiPlatform::getInstance().is_mi350())
			&& arg.watch == -1) {
		amdsmi_metric_t *metrics;
		uint32_t metric_size = AMDSMI_MAX_NUM_METRICS;

		ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, NULL);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			metric_table = false;
		}
		metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t)*metric_size);
		if (metrics == NULL) {
			throw SmiToolNotEnoughMemException();
		}
		ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, &metrics[0]);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			metric_table = false;
		}

		if (ret == AMDSMI_STATUS_SUCCESS) {
			metric_table = true;
			for (int i = 0; i < metric_size; i++) {
				if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_VCN)
						&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					vcn_chiplet.push_back(metrics[i]);
				}
				if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_JPEG)
						&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					jpeg_chiplet.push_back(metrics[i]);
				}
				if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_MM)
						&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					mm_chiplet.push_back(metrics[i]);
				}
				if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_MEM)
						&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					mem_chiplet.push_back(metrics[i]);
				}
				if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_GFX)
						&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					gfx_chiplet.push_back(metrics[i]);
				}
			}
		} else {
			metric_table = false;
		}
		free(metrics);
	}
	if (metric_table == false) {
		if (arg.watch > -1) {
			out = string_format("%s,%s,%s",gfx_activity.c_str(), umc_activity.c_str(),
								mm_activity_str.c_str());
		} else if (arg.output == json) {
			nlohmann::ordered_json gfx_activity{};
			gfx_activity["value"] = engine_usage.gfx_activity;
			gfx_activity["unit"] =  gfx_activity == "N/A" ? "N/A" : "%";
			nlohmann::ordered_json umc_activity{};
			umc_activity["value"] = engine_usage.umc_activity;
			umc_activity["unit"] = umc_activity == "N/A" ? "N/A" : "%";
			nlohmann::ordered_json mm_activity{};
			nlohmann::ordered_json engine_usage_json = { { "gfx_activity", gfx_activity },
				{ "umc_activity", umc_activity }
			};

			if(engine_usage.mm_activity == INT32_MAX) {
				mm_activity["value"] = mm_activity_str;
			} else {
				mm_activity["value"] = engine_usage.mm_activity;
			}
			mm_activity["unit"] = mm_activity_str == "N/A" ? "N/A" : "%";
			engine_usage_json["mm_activity"] = mm_activity;

			auto vcn_json = nlohmann::ordered_json::array();
			nlohmann::ordered_json vjson{};
			vjson["value"] = "N/A";
			vjson["unit"] = "N/A";
			vcn_json.push_back( vjson );

			auto jpeg_json = nlohmann::ordered_json::array();
			nlohmann::ordered_json jjson{};
			jjson["value"] = "N/A";
			jjson["unit"] = "N/A";
			jpeg_json.push_back(jjson);

			engine_usage_json["vcn_activity"] = vcn_json;
			engine_usage_json["jpeg_activity"] = jpeg_json;

			out = engine_usage_json.dump(4);
		} else if (arg.output == csv) {
			out = string_format(",%s,%s,%s,%s,%s,%s,%s",
								gfx_activity.c_str(), umc_activity.c_str(), mm_activity_str.c_str(), "N/A", "N/A", "N/A", "N/A");
		} else {
			std::string gfx_activity_unit = gfx_activity == "N/A" ? "" : "%";
			std::string umc_activity_unit = umc_activity == "N/A" ? "" : "%";
			std::string mm_activity_unit = mm_activity_str == "N/A" ? "" : "%";
			out = string_format(
					  metricUsageTemplate, gfx_activity.c_str(), gfx_activity_unit.c_str(), umc_activity.c_str(),
					  umc_activity_unit.c_str(),
					  mm_activity_str.c_str(), mm_activity_unit.c_str());
			out.append(metricVcnUsageHeaderTemplate);
			out.append(string_format(metricVcnUsageTemplate, "N/A", "N/A"));
			out.append(metricJpegUsageHeaderTemplate);
			out.append(string_format(metricJpegUsageTemplate, "N/A", "N/A"));
			out.append(metricJpegUsageFooterTemplate);
		}
	} else {
		std::string mem_activity{string_format("%ld", mem_chiplet[0].val)};
		gfx_activity = string_format("%ld", gfx_chiplet[0].val);
		if (mm_chiplet.size() > 1) {
			mm_activity_str = string_format("%ld", mm_chiplet[0].val);
		} else {
			mm_activity_str = "N/A";
		}

		std::string mem_activity_percent{mem_chiplet[0].unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : ""};
		mm_activity_percent = (mem_chiplet[0].unit == AMDSMI_METRIC_UNIT_PERCENT
							   && mm_activity_str != "N/A" ) ? "%" : "";
		gfx_activity_percent = mem_chiplet[0].unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : "";

		if (arg.watch > -1) {
			out = string_format("%s,%s,%s",gfx_activity.c_str(), umc_activity.c_str(),
								mm_activity_str.c_str());
		} else if (arg.output == json) {
			auto values_json = nlohmann::ordered_json::array();
			nlohmann::ordered_json gfx_activity{};
			gfx_activity["value"] = gfx_chiplet[0].val;
			gfx_activity["unit"] = gfx_activity == "N/A" ? "N/A" : "%";
			nlohmann::ordered_json umc_activity{};
			umc_activity["value"] = mem_chiplet[0].val;
			umc_activity["unit"] = mem_activity == "N/A" ? "N/A" : "%";
			nlohmann::ordered_json mm_activity{};
			nlohmann::ordered_json engine_usage_json = { { "gfx_activity", gfx_activity},
				{ "umc_activity", umc_activity},
			};

			if(mm_chiplet.size() > 1) {
				mm_activity["value"] = mm_chiplet[0].val;
			} else {
				mm_activity["value"] = "N/A";
			}
			mm_activity["unit"] = mm_activity_str == "N/A" ? "N/A" : "%";
			engine_usage_json["mm_activity"] = mm_activity;

			auto vcn_json = nlohmann::ordered_json::array();
			for (int i = 0; i < vcn_chiplet.size(); i++) {
				nlohmann::ordered_json vjson{};
				vjson["value"] = vcn_chiplet[i].val;
				vjson["unit"] = string_format("%lld",vcn_chiplet[i].val) == "N/A" ? "N/A" : "%";
				vcn_json.push_back( vjson );
			}
			auto jpeg_json = nlohmann::ordered_json::array();
			for (int i = 0; i < jpeg_chiplet.size(); i++) {
				nlohmann::ordered_json jjson{};
				jjson["value"] = jpeg_chiplet[i].val;
				jjson["unit"] = string_format("%lld",jpeg_chiplet[i].val) == "N/A" ? "N/A" : "%";
				jpeg_json.push_back(jjson);
			}

			engine_usage_json["vcn_activity"] = vcn_json;
			engine_usage_json["jpeg_activity"] = jpeg_json;

			out = engine_usage_json.dump(4);
		} else if (arg.output == csv) {
			std::vector<std::vector<std::string>> output_rows{};
			std::vector<std::string> value_rows{};

			output_rows.push_back({string_format(",%s,%s,%s", gfx_activity.c_str(), mem_activity.c_str(), mm_activity_str.c_str())});
			for (int i = 0; i < vcn_chiplet.size(); i++) {
				value_rows.push_back(string_format(",%d,%ld", i, vcn_chiplet[i].val));
			}

			output_rows.push_back(value_rows);
			value_rows.clear();
			for (int i = 0; i < jpeg_chiplet.size(); i++) {
				value_rows.push_back(string_format(",%d,%ld", i, jpeg_chiplet[i].val));
			}

			output_rows.push_back(value_rows);
			value_rows.clear();

			csv_recursion(out, output_rows);
		} else {
			out.append(string_format(
						   metricUsageTemplate, gfx_activity.c_str(), gfx_activity_percent.c_str(),mem_activity.c_str(),
						   mem_activity_percent.c_str(), mm_activity_str.c_str(), mm_activity_percent.c_str()));
			out.append(metricVcnUsageHeaderTemplate);
			for (int i = 0; i < vcn_chiplet.size(); i++) {
				std::string vcn_chiplet_val = string_format("%d", vcn_chiplet[i].val);
				std::string unit{vcn_chiplet[i].unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : ""};
				out.append(string_format(metricVcnUsageTemplate, vcn_chiplet_val.c_str(), unit.c_str()));
				if (i < vcn_chiplet.size() - 1) {
					out.append(commaTemplate);
				}
			}
			out.append(metricJpegUsageHeaderTemplate);
			for (int i = 0; i < jpeg_chiplet.size(); i++) {
				std::string jpeg_chiplet_val = string_format("%d", jpeg_chiplet[i].val);
				std::string unit{jpeg_chiplet[i].unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : ""};
				out.append(string_format(metricJpegUsageTemplate, jpeg_chiplet_val.c_str(),
										 unit.c_str()));
				if (i < jpeg_chiplet.size() - 1) {
					out.append(commaTemplate);
				}
			}
			out.append(metricJpegUsageFooterTemplate);
		}
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_power_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;

	amdsmi_power_info_t power_info;
	bool is_power_management_enabled;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_power(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_power_info(processor, &power_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_power(arg, "N/A");
		return ret;
	}

	std::string voltage_str = (power_info.gfx_voltage == UINT_MAX) ? "N/A" :
							  string_format(
								  "%lld", power_info.gfx_voltage);
	std::string soc_voltage_str = (power_info.soc_voltage == UINT_MAX) ? "N/A" :
								  string_format(
									  "%lld", power_info.soc_voltage);
	std::string mem_voltage_str = (power_info.mem_voltage == UINT_MAX) ? "N/A" :
								  string_format(
									  "%lld", power_info.mem_voltage);
	std::string socket_power_str{ string_format(
		"%lld", power_info.socket_power) };

	std::string is_power_management_enabled_str;
	ret = host_amdsmi_is_gpu_power_management_enabled(processor,
		&is_power_management_enabled);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		is_power_management_enabled_str = "N/A";
	} else
		is_power_management_enabled_str = is_power_management_enabled ? "ENABLED" : "DISABLED";

	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s,%s", socket_power_str.c_str(),
							voltage_str.c_str(), soc_voltage_str.c_str(), mem_voltage_str.c_str(),
							is_power_management_enabled_str.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json socket_power{};
		socket_power["value"] = power_info.socket_power;
		socket_power["unit"] = socket_power_str == "N/A" ? "N/A" : "W";
		nlohmann::ordered_json power_info_json = { { "socket_power", socket_power} };

		nlohmann::ordered_json gfx_voltage{};
		if(power_info.gfx_voltage == UINT_MAX) {
			gfx_voltage["value"] = voltage_str;
		} else {
			gfx_voltage["value"] = power_info.gfx_voltage;
		}
		gfx_voltage["unit"] = voltage_str == "N/A" ? "N/A" : "mV";
		power_info_json["gfx_voltage"] = gfx_voltage;

		nlohmann::ordered_json soc_voltage{};
		if(power_info.soc_voltage == UINT_MAX) {
			soc_voltage["value"] = soc_voltage_str;
		} else {
			soc_voltage["value"] = power_info.soc_voltage;
		}
		soc_voltage["unit"] = soc_voltage_str == "N/A" ? "N/A" : "mV";
		power_info_json["soc_voltage"] = soc_voltage;

		nlohmann::ordered_json mem_voltage{};
		if(power_info.mem_voltage == UINT_MAX) {
			mem_voltage["value"] = mem_voltage_str;
		} else {
			mem_voltage["value"] = power_info.mem_voltage;
		}
		mem_voltage["unit"] = mem_voltage_str == "N/A" ? "N/A" : "mV";
		power_info_json["mem_voltage"] = mem_voltage;

		power_info_json["power_management"] = is_power_management_enabled ? "ENABLED" : "DISABLED";

		out = power_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s", socket_power_str.c_str(),
				  voltage_str.c_str(), soc_voltage_str.c_str(), mem_voltage_str.c_str(),
				  is_power_management_enabled_str.c_str());
	} else {
		std::string socket_power_unit = socket_power_str == "N/A" ? "" : "W";
		std::string voltage_unit = voltage_str == "N/A" ? "" : "mV";
		std::string soc_voltage_unit = soc_voltage_str == "N/A" ? "" : "mV";
		std::string mem_voltage_unit = mem_voltage_str == "N/A" ? "" : "mV";
		out = string_format(
				  metricPowerMeasureTemplate, socket_power_str.c_str(), socket_power_unit.c_str(),
				  voltage_str.c_str(), voltage_unit.c_str(), soc_voltage_str.c_str(),
				  soc_voltage_unit.c_str(),
				  mem_voltage_str.c_str()
				  ,mem_voltage_unit.c_str(),
				  is_power_management_enabled_str.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}


void fill_clk_info(std::map<std::string, std::string> &clk_map)
{
	clk_map["clk"] = "N/A";
	clk_map["max_clk"] = "N/A";
	clk_map["min_clk"] = "N/A";
	clk_map["clk_locked"] = "N/A";
	clk_map["deep_sleep"] = "N/A";
}

std::string format_clock_value(uint64_t value, uint64_t inval_value)
{
	return (value != inval_value) ? string_format("%lld", value) : "N/A";
}

amdsmi_status_t get_clk_info(amdsmi_processor_handle processor, amdsmi_clk_info_t &clock_measure,
							 std::map<std::string, std::string> &clk_map, amdsmi_clk_type_t clk_type)
{
	amdsmi_status_t ret;

	ret = host_amdsmi_get_clock_info(processor, clk_type, &clock_measure);
	if (ret == AMDSMI_STATUS_SUCCESS) {
		clk_map["clk"] = format_clock_value(clock_measure.clk, UINT32_MAX);
		clk_map["max_clk"] = format_clock_value(clock_measure.max_clk, UINT32_MAX);
		clk_map["min_clk"] = format_clock_value(clock_measure.min_clk, UINT32_MAX);
		clk_map["clk_locked"] = "N/A";
		clk_map["deep_sleep"] = "N/A";

		if (clock_measure.clk_locked != UINT8_MAX) {
			clk_map["clk_locked"] = clock_measure.clk_locked ? "LOCKED" : "UNLOCKED";
		}

		if (clock_measure.clk_deep_sleep != UINT8_MAX) {
			clk_map["deep_sleep"] = clock_measure.clk_deep_sleep ? "ENABLED" : "DISABLED";
		}
	} else {
		return ret;
	}
	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t get_metric_gfx_mem_clock(amdsmi_processor_handle processor, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_clk_info_t gfx_clk{}, mem_clk{};
	std::map<std::string, std::string> gfx_clk_str{}, mem_clk_str{};

	ret = get_clk_info(processor, gfx_clk, gfx_clk_str, AMDSMI_CLK_TYPE_GFX);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fill_clk_info(gfx_clk_str);
	}

	ret = get_clk_info(processor, mem_clk, mem_clk_str, AMDSMI_CLK_TYPE_MEM);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fill_clk_info(mem_clk_str);
	}

	mem_clk_str["clk_locked"] = "N/A";
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
							gfx_clk_str["clk"].c_str(), gfx_clk_str["min_clk"].c_str(), gfx_clk_str["max_clk"].c_str(),
							gfx_clk_str["clk_locked"].c_str(), gfx_clk_str["deep_sleep"].c_str(), mem_clk_str["clk"].c_str(),
							mem_clk_str["min_clk"].c_str(), mem_clk_str["max_clk"].c_str(), mem_clk_str["clk_locked"].c_str(),
							mem_clk_str["deep_sleep"].c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json result{};
		nlohmann::ordered_json gfx_clk_json{};
		if (gfx_clk.clk == UINT64_MAX) {
			gfx_clk_json["value"] = "N/A";
			gfx_clk_json["unit"] = "N/A";
		} else {
			gfx_clk_json["value"] = gfx_clk.clk;
			gfx_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json gfx_max_clk_json{};
		if (gfx_clk.max_clk == UINT64_MAX) {
			gfx_max_clk_json["value"] = "N/A";
			gfx_max_clk_json["unit"] = "N/A";
		} else {
			gfx_max_clk_json["value"] = gfx_clk.max_clk;
			gfx_max_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json gfx_min_clk_json{};
		if (gfx_clk.min_clk == UINT64_MAX) {
			gfx_min_clk_json["value"] = "N/A";
			gfx_min_clk_json["unit"] = "N/A";
		} else {
			gfx_min_clk_json["value"] = gfx_clk.min_clk;
			gfx_min_clk_json["unit"] = "MHz";
		}
		result["gfx"] = nlohmann::ordered_json::object({ { "clk", gfx_clk_json },
			{ "min_clk", gfx_min_clk_json },
			{ "max_clk", gfx_max_clk_json },
			{ "clk_locked", gfx_clk_str["clk_locked"] },
			{ "deep_sleep", gfx_clk_str["deep_sleep"] }
		});

		nlohmann::ordered_json mem_clk_json{};
		if (mem_clk.clk == UINT64_MAX) {
			mem_clk_json["value"] = "N/A";
			mem_clk_json["unit"] = "N/A";
		} else {
			mem_clk_json["value"] = mem_clk.clk;
			mem_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json mem_max_clk_json{};
		if (mem_clk.max_clk == UINT64_MAX) {
			mem_max_clk_json["value"] = "N/A";
			mem_max_clk_json["unit"] = "N/A";
		} else {
			mem_max_clk_json["value"] = mem_clk.max_clk;
			mem_max_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json mem_min_clk_json{};
		if (mem_clk.min_clk == UINT64_MAX) {
			mem_min_clk_json["value"] = "N/A";
			mem_min_clk_json["unit"] = "N/A";
		} else {
			mem_min_clk_json["value"] = mem_clk.min_clk;
			mem_min_clk_json["unit"] = "MHz";
		}
		result["mem"] = nlohmann::ordered_json::object({ { "clk", mem_clk_json },
			{ "min_clk", mem_min_clk_json },
			{ "max_clk", mem_max_clk_json },
			{ "clk_locked", mem_clk_str["clk_locked"] },
			{ "deep_sleep", mem_clk_str["deep_sleep"] }
		});

		out = result.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
							gfx_clk_str["clk"].c_str(), gfx_clk_str["min_clk"].c_str(), gfx_clk_str["max_clk"].c_str(),
							gfx_clk_str["clk_locked"].c_str(), gfx_clk_str["deep_sleep"].c_str(), mem_clk_str["clk"].c_str(),
							mem_clk_str["min_clk"].c_str(), mem_clk_str["max_clk"].c_str(), mem_clk_str["clk_locked"].c_str(),
							mem_clk_str["deep_sleep"].c_str());
	} else {
		std::string gfx_clk_unit = gfx_clk_str["clk"] == "N/A" ? "" : "MHz";
		std::string gfx_min_clk_unit = gfx_clk_str["min_clk"] == "N/A" ? "" : "MHz";
		std::string gfx_max_clk_unit = gfx_clk_str["max_clk"] == "N/A" ? "" : "MHz";
		std::string mem_clk_unit = mem_clk_str["clk"] == "N/A" ? "" : "MHz";
		std::string mem_min_clk_unit = mem_clk_str["min_clk"] == "N/A" ? "" : "MHz";
		std::string mem_max_clk_unit = mem_clk_str["max_clk"] == "N/A" ? "" : "MHz";

		out = string_format(metricClockMeasureHostTemplate, gfx_clk_str["clk"].c_str(),
							gfx_clk_unit.c_str(),
							gfx_clk_str["min_clk"].c_str(), gfx_min_clk_unit.c_str(), gfx_clk_str["max_clk"].c_str(),
							gfx_max_clk_unit.c_str(), gfx_clk_str["clk_locked"].c_str(), gfx_clk_str["deep_sleep"].c_str(),
							mem_clk_str["clk"].c_str(), mem_clk_unit.c_str(), mem_clk_str["min_clk"].c_str(),
							mem_min_clk_unit.c_str(),
							mem_clk_str["max_clk"].c_str(), mem_max_clk_unit.c_str(), mem_clk_str["clk_locked"].c_str(),
							mem_clk_str["deep_sleep"].c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t get_metric_vclk_clock(amdsmi_processor_handle processor, Arguments arg,
									  std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_clk_info_t vclk0_clk{}, vclk1_clk{};
	std::map<std::string, std::string> vclk0_clk_str{}, vclk1_clk_str{};

	ret = get_clk_info(processor, vclk0_clk, vclk0_clk_str, AMDSMI_CLK_TYPE_VCLK0);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fill_clk_info(vclk0_clk_str);
	}

	ret = get_clk_info(processor, vclk1_clk, vclk1_clk_str, AMDSMI_CLK_TYPE_VCLK1);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fill_clk_info(vclk1_clk_str);
	}

	vclk0_clk_str["clk_locked"] = "N/A";
	vclk1_clk_str["clk_locked"] = "N/A";
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
							vclk0_clk_str["clk"].c_str(), vclk0_clk_str["min_clk"].c_str(),
							vclk0_clk_str["max_clk"].c_str(), vclk0_clk_str["clk_locked"].c_str(),
							vclk0_clk_str["deep_sleep"].c_str(),
							vclk1_clk_str["clk"].c_str(), vclk1_clk_str["min_clk"].c_str(), vclk1_clk_str["max_clk"].c_str(),
							vclk1_clk_str["clk_locked"].c_str(), vclk1_clk_str["deep_sleep"].c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json result{};
		nlohmann::ordered_json vclk0_clk_json{};
		if (vclk0_clk.clk == UINT64_MAX) {
			vclk0_clk_json["value"] = "N/A";
			vclk0_clk_json["unit"] = "N/A";
		} else {
			vclk0_clk_json["value"] = vclk0_clk.clk;
			vclk0_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json vclk0_max_clk_json{};
		if (vclk0_clk.max_clk == UINT64_MAX) {
			vclk0_max_clk_json["value"] = "N/A";
			vclk0_max_clk_json["unit"] = "N/A";
		} else {
			vclk0_max_clk_json["value"] = vclk0_clk.max_clk;
			vclk0_max_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json vclk0_min_clk_json{};
		if (vclk0_clk.min_clk == UINT64_MAX) {
			vclk0_min_clk_json["value"] = "N/A";
			vclk0_min_clk_json["unit"] = "N/A";
		} else {
			vclk0_min_clk_json["value"] = vclk0_clk.min_clk;
			vclk0_min_clk_json["unit"] = "MHz";
		}
		result["vclk_0"] = nlohmann::ordered_json::object({ { "clk", vclk0_clk_json },
			{ "min_clk", vclk0_min_clk_json },
			{ "max_clk", vclk0_max_clk_json },
			{ "clk_locked", vclk0_clk_str["clk_locked"] },
			{ "deep_sleep", vclk0_clk_str["deep_sleep"] }
		});

		nlohmann::ordered_json vclk1_clk_json{};
		if (vclk1_clk.clk == UINT64_MAX) {
			vclk1_clk_json["value"] = "N/A";
			vclk1_clk_json["unit"] = "N/A";
		} else {
			vclk1_clk_json["value"] = vclk1_clk.clk;
			vclk1_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json vclk1_max_clk_json{};
		if (vclk1_clk.max_clk == UINT64_MAX) {
			vclk1_max_clk_json["value"] = "N/A";
			vclk1_max_clk_json["unit"] = "N/A";
		} else {
			vclk1_max_clk_json["value"] = vclk1_clk.max_clk;
			vclk1_max_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json vclk1_min_clk_json{};
		if (vclk1_clk.min_clk == UINT64_MAX) {
			vclk1_min_clk_json["value"] = "N/A";
			vclk1_min_clk_json["unit"] = "N/A";
		} else {
			vclk1_min_clk_json["value"] = vclk1_clk.min_clk;
			vclk1_min_clk_json["unit"] = "MHz";
		}
		result["vclk_1"] = nlohmann::ordered_json::object({ { "clk", vclk1_clk_json },
			{ "min_clk", vclk1_min_clk_json },
			{ "max_clk", vclk1_max_clk_json },
			{ "clk_locked", vclk1_clk_str["clk_locked"] },
			{ "deep_sleep", vclk1_clk_str["deep_sleep"] }
		});
		out = result.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
							vclk0_clk_str["clk"].c_str(), vclk0_clk_str["min_clk"].c_str(),
							vclk0_clk_str["max_clk"].c_str(), vclk0_clk_str["clk_locked"].c_str(),
							vclk0_clk_str["deep_sleep"].c_str(),
							vclk1_clk_str["clk"].c_str(), vclk1_clk_str["min_clk"].c_str(), vclk1_clk_str["max_clk"].c_str(),
							vclk1_clk_str["clk_locked"].c_str(), vclk1_clk_str["deep_sleep"].c_str());
	} else {
		std::string vclk0_clk_unit = vclk0_clk_str["clk"] == "N/A" ? "" : "MHz";
		std::string vclk0_min_clk_unit = vclk0_clk_str["min_clk"] == "N/A" ? "" : "MHz";
		std::string vclk0_max_clk_unit = vclk0_clk_str["max_clk"] == "N/A" ? "" : "MHz";
		std::string vclk1_clk_unit = vclk1_clk_str["clk"] == "N/A" ? "" : "MHz";
		std::string vclk1_min_clk_unit = vclk1_clk_str["min_clk"] == "N/A" ? "" : "MHz";
		std::string vclk1_max_clk_unit = vclk1_clk_str["max_clk"] == "N/A" ? "" : "MHz";

		out.append(string_format(metricVCLK0ClockMeasureHostTemplate, vclk0_clk_str["clk"].c_str(),
								 vclk0_clk_unit.c_str(), vclk0_clk_str["min_clk"].c_str(), vclk0_min_clk_unit.c_str(),
								 vclk0_clk_str["max_clk"].c_str(), vclk0_max_clk_unit.c_str(), vclk0_clk_str["clk_locked"].c_str(),
								 vclk0_clk_str["deep_sleep"].c_str()));
		out.append(string_format(metricVCLK1ClockMeasureHostTemplate, vclk1_clk_str["clk"].c_str(),
								 vclk1_clk_unit.c_str(), vclk1_clk_str["min_clk"].c_str(), vclk1_min_clk_unit.c_str(),
								 vclk1_clk_str["max_clk"].c_str(), vclk1_max_clk_unit.c_str(), vclk1_clk_str["clk_locked"].c_str(),
								 vclk1_clk_str["deep_sleep"].c_str()));
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t get_metric_dclk_clock(amdsmi_processor_handle processor, Arguments arg,
									  std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_clk_info_t  dclk0_clk{}, dclk1_clk{};
	std::map<std::string, std::string> dclk0_clk_str{}, dclk1_clk_str{};

	ret = get_clk_info(processor, dclk0_clk, dclk0_clk_str, AMDSMI_CLK_TYPE_DCLK0);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fill_clk_info(dclk0_clk_str);
	}

	ret = get_clk_info(processor, dclk1_clk, dclk1_clk_str, AMDSMI_CLK_TYPE_DCLK1);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		fill_clk_info(dclk1_clk_str);
	}


	dclk0_clk_str["clk_locked"] = "N/A";
	dclk1_clk_str["clk_locked"] = "N/A";
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",  dclk0_clk_str["clk"].c_str(),
							dclk0_clk_str["min_clk"].c_str(), dclk0_clk_str["max_clk"].c_str(),
							dclk0_clk_str["clk_locked"].c_str(),
							dclk0_clk_str["deep_sleep"].c_str(), dclk1_clk_str["clk"].c_str(), dclk1_clk_str["min_clk"].c_str(),
							dclk1_clk_str["max_clk"].c_str(), dclk1_clk_str["clk_locked"].c_str(),
							dclk1_clk_str["deep_sleep"].c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json result{};
		nlohmann::ordered_json dclk0_clk_json{};
		if (dclk0_clk.clk == UINT64_MAX) {
			dclk0_clk_json["value"] = "N/A";
			dclk0_clk_json["unit"] = "N/A";
		} else {
			dclk0_clk_json["value"] = dclk0_clk.clk;
			dclk0_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json dclk0_max_clk_json{};
		if (dclk0_clk.max_clk == UINT64_MAX) {
			dclk0_max_clk_json["value"] = "N/A";
			dclk0_max_clk_json["unit"] = "N/A";
		} else {
			dclk0_max_clk_json["value"] = dclk0_clk.max_clk;
			dclk0_max_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json dclk0_min_clk_json{};
		if (dclk0_clk.min_clk == UINT64_MAX) {
			dclk0_min_clk_json["value"] = "N/A";
			dclk0_min_clk_json["unit"] = "N/A";
		} else {
			dclk0_min_clk_json["value"] = dclk0_clk.min_clk;
			dclk0_min_clk_json["unit"] = "MHz";
		}
		result["dclk_0"] = nlohmann::ordered_json::object({ { "clk", dclk0_clk_json },
			{ "min_clk", dclk0_min_clk_json },
			{ "max_clk", dclk0_max_clk_json },
			{ "clk_locked", dclk0_clk_str["clk_locked"] },
			{ "deep_sleep", dclk0_clk_str["deep_sleep"] }
		});

		nlohmann::ordered_json dclk1_clk_json{};
		if (dclk1_clk.clk == UINT64_MAX) {
			dclk1_clk_json["value"] = "N/A";
			dclk1_clk_json["unit"] = "N/A";
		} else {
			dclk1_clk_json["value"] = dclk1_clk.clk;
			dclk1_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json dclk1_max_clk_json{};
		if (dclk1_clk.max_clk == UINT64_MAX) {
			dclk1_max_clk_json["value"] = "N/A";
			dclk1_max_clk_json["unit"] = "N/A";
		} else {
			dclk1_max_clk_json["value"] = dclk1_clk.max_clk;
			dclk1_max_clk_json["unit"] = "MHz";
		}
		nlohmann::ordered_json dclk1_min_clk_json{};
		if (dclk1_clk.min_clk == UINT64_MAX) {
			dclk1_min_clk_json["value"] = "N/A";
			dclk1_min_clk_json["unit"] = "N/A";
		} else {
			dclk1_min_clk_json["value"] = dclk1_clk.min_clk;
			dclk1_min_clk_json["unit"] = "MHz";
		}
		result["dclk_1"] = nlohmann::ordered_json::object({ { "clk", dclk1_clk_json },
			{ "min_clk", dclk1_min_clk_json },
			{ "max_clk", dclk1_max_clk_json },
			{ "clk_locked", dclk1_clk_str["clk_locked"] },
			{ "deep_sleep", dclk1_clk_str["deep_sleep"] }
		});

		out = result.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", dclk0_clk_str["clk"].c_str(),
							dclk0_clk_str["min_clk"].c_str(), dclk0_clk_str["max_clk"].c_str(),
							dclk0_clk_str["clk_locked"].c_str(),
							dclk0_clk_str["deep_sleep"].c_str(), dclk1_clk_str["clk"].c_str(), dclk1_clk_str["min_clk"].c_str(),
							dclk1_clk_str["max_clk"].c_str(), dclk1_clk_str["clk_locked"].c_str(),
							dclk1_clk_str["deep_sleep"].c_str());
	} else {
		std::string dclk0_clk_unit = dclk0_clk_str["clk"] == "N/A" ? "" : "MHz";
		std::string dclk0_min_clk_unit = dclk0_clk_str["min_clk"] == "N/A" ? "" : "MHz";
		std::string dclk0_max_clk_unit = dclk0_clk_str["max_clk"] == "N/A" ? "" : "MHz";
		std::string dclk1_clk_unit = dclk1_clk_str["clk"] == "N/A" ? "" : "MHz";
		std::string dclk1_min_clk_unit = dclk1_clk_str["min_clk"] == "N/A" ? "" : "MHz";
		std::string dclk1_max_clk_unit = dclk1_clk_str["max_clk"] == "N/A" ? "" : "MHz";

		out.append(string_format(metricDCLK0ClockMeasureHostTemplate, dclk0_clk_str["clk"].c_str(),
								 dclk0_clk_unit.c_str(), dclk0_clk_str["min_clk"].c_str(), dclk0_min_clk_unit.c_str(),
								 dclk0_clk_str["max_clk"].c_str(), dclk0_max_clk_unit.c_str(), dclk0_clk_str["clk_locked"].c_str(),
								 dclk0_clk_str["deep_sleep"].c_str()));
		out.append(string_format(metricDCLK1ClockMeasureHostTemplate, dclk1_clk_str["clk"].c_str(),
								 dclk1_clk_unit.c_str(), dclk1_clk_str["min_clk"].c_str(), dclk1_min_clk_unit.c_str(),
								 dclk1_clk_str["max_clk"].c_str(), dclk1_max_clk_unit.c_str(), dclk1_clk_str["clk_locked"].c_str(),
								 dclk1_clk_str["deep_sleep"].c_str()));
	}

	return AMDSMI_STATUS_SUCCESS;
}

amdsmi_status_t get_metric_clock_data(uint64_t processor_bdf, Arguments arg, std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_clock(arg, "N/A");
		return ret;
	}

	std::string gfx_mem_out{};
	ret = get_metric_gfx_mem_clock(processor, arg, gfx_mem_out);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_clock(arg, "N/A");
		return ret;
	}

	std::string vclk_out{};
	ret = get_metric_vclk_clock(processor, arg, vclk_out);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_clock(arg, "N/A");
		return ret;
	}

	std::string dclk_out{};
	ret = get_metric_dclk_clock(processor, arg, dclk_out);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_clock(arg, "N/A");
		return ret;
	}

	if (arg.watch > -1) {
		out.append(gfx_mem_out).append(",").append(vclk_out).append(",").append(dclk_out);
	} else if (arg.output == json) {
		nlohmann::ordered_json gfx_mem_json = nlohmann::ordered_json::parse(gfx_mem_out);
		nlohmann::ordered_json vclk_json = nlohmann::ordered_json::parse(vclk_out);
		nlohmann::ordered_json dclk_json = nlohmann::ordered_json::parse(dclk_out);

		nlohmann::ordered_json combined_json = gfx_mem_json;
		combined_json.insert(vclk_json.begin(), vclk_json.end());
		combined_json.insert(dclk_json.begin(), dclk_json.end());

		out = combined_json.dump(4);

	} else if (arg.output == csv) {
		out.append(gfx_mem_out).append(vclk_out).append(dclk_out);
	} else {
		out.append(gfx_mem_out).append(vclk_out).append(dclk_out);
	}

	return AMDSMI_STATUS_SUCCESS;
}

std::string get_clk_deep_sleep(amdsmi_processor_handle processor, amdsmi_clk_type_t clk_type)
{
	std::string deep_sleep{"N/A"};
	amdsmi_status_t ret;
	amdsmi_clk_info_t clock_measure;

	ret = host_amdsmi_get_clock_info(processor, clk_type, &clock_measure);
	if (ret == AMDSMI_STATUS_SUCCESS) {
		if (clock_measure.clk_deep_sleep != UINT8_MAX) {
			deep_sleep = clock_measure.clk_deep_sleep ? "ENABLED" : "DISABLED";
		}
	}
	return deep_sleep;
}

amdsmi_status_t get_metric_ext_clock_data(uint64_t processor_bdf, Arguments arg, std::string& out)
{
	std::vector<std::vector<std::string>> output_rows{};
	std::map<std::string, std::vector<amdsmi_metric_t>> gfx_chiplet{};
	std::map<std::string, std::vector<amdsmi_metric_t>> mem_chiplet{};
	std::map<std::string, std::vector<amdsmi_metric_t>> vclk_chiplet{};
	std::map<std::string, std::vector<amdsmi_metric_t>> dclk_chiplet{};

	amdsmi_status_t ret;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_clock(arg, "N/A");
		return ret;
	}

	amdsmi_metric_t *metrics;
	uint32_t metric_size = AMDSMI_MAX_NUM_METRICS;
	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, NULL);
	if (ret == AMDSMI_STATUS_SUCCESS) {
		metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t)*metric_size);
		if (metrics == NULL) {
			throw SmiToolNotEnoughMemException();
		}
		ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, &metrics[0]);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			free(metrics);
			metrics = NULL;
			return ret;
		} else {
			for (int i = 0; i < metric_size; i++) {
				if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX
						&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					gfx_chiplet["clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX_MIN_LIMIT
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					gfx_chiplet["min_clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX_MAX_LIMIT
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					gfx_chiplet["max_clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX_LOCKED
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					gfx_chiplet["clk_locked"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX_DS_DISABLED
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					gfx_chiplet["deep_sleep"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_MEM
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					mem_chiplet["clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_MEM_MIN_LIMIT
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					mem_chiplet["min_clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_MEM_MAX_LIMIT
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					mem_chiplet["max_clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_MEM_DS_DISABLED
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					mem_chiplet["deep_sleep"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_VCLK
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					vclk_chiplet["clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_VCLK_MIN_LIMIT
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					vclk_chiplet["min_clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_VCLK_MAX_LIMIT
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					vclk_chiplet["max_clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_VCLK_DS_DISABLED
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					vclk_chiplet["deep_sleep"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_DCLK
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					dclk_chiplet["clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_DCLK_MIN_LIMIT
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					dclk_chiplet["min_clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_DCLK_MAX_LIMIT
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					dclk_chiplet["max_clk"].push_back(metrics[i]);
				} else if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_DCLK_DS_DISABLED
						   && !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
					dclk_chiplet["deep_sleep"].push_back(metrics[i]);
				}
			}
			free(metrics);
			metrics = NULL;
		}
	} else {
		out = host_fill_clock(arg, "N/A");
		return ret;
	}

	std::size_t gfx_cur_size = std::min({gfx_chiplet["clk"].size(), gfx_chiplet["min_clk"].size(), gfx_chiplet["max_clk"].size()});
	std::size_t mem_cur_size = std::min({mem_chiplet["clk"].size(), mem_chiplet["min_clk"].size(), mem_chiplet["max_clk"].size()});
	std::size_t vclk_cur_size = std::min({vclk_chiplet["clk"].size(), vclk_chiplet["min_clk"].size(), vclk_chiplet["max_clk"].size()});
	std::size_t dclk_cur_size = std::min({dclk_chiplet["clk"].size(), dclk_chiplet["min_clk"].size(), dclk_chiplet["max_clk"].size()});

	if (arg.output == json) {
		nlohmann::ordered_json result{};
		for (int i = 0; i < gfx_cur_size; i++) {
			nlohmann::ordered_json gfx_clk_json{};
			if (gfx_chiplet["clk"][i].val == UINT64_MAX) {
				gfx_clk_json["value"] = "N/A";
			} else {
				gfx_clk_json["value"] = gfx_chiplet["clk"][i].val;
			}
			gfx_clk_json["unit"] = string_format("%llu", gfx_chiplet["clk"][i].val) == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json gfx_min_clk_json{};
			if (gfx_chiplet["min_clk"][i].val == UINT64_MAX) {
				gfx_min_clk_json["value"] = "N/A";
			} else {
				gfx_min_clk_json["value"] = gfx_chiplet["min_clk"][i].val;
			}
			gfx_min_clk_json["unit"] = string_format("%llu",
									   gfx_chiplet["min_clk"][i].val) == "N/A" ? "N/A" :  "MHz";
			nlohmann::ordered_json gfx_max_clk_json{};
			if (gfx_chiplet["max_clk"][i].val == UINT64_MAX) {
				gfx_max_clk_json["value"] = "N/A";
			} else {
				gfx_max_clk_json["value"] = gfx_chiplet["max_clk"][i].val;
			}
			gfx_max_clk_json["unit"] = string_format("%llu",
									   gfx_chiplet["max_clk"][i].val) == "N/A" ? "N/A" :  "MHz";

			std::string clk_locked{"N/A"};
			std::string clk_deep_sleep{"N/A"};
			if (gfx_chiplet["clk_locked"].size()) {
				clk_locked = gfx_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 gfx_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			if(gfx_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = gfx_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 gfx_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_GFX);
			}
			result[string_format("gfx_%d",i)] = nlohmann::ordered_json::object( {
				{"clk", gfx_clk_json },
				{"min_clk", gfx_min_clk_json },
				{"max_clk", gfx_max_clk_json },
				{"clk_locked", clk_locked },
				{"deep_sleep",  clk_deep_sleep}});
		}
		for (int i = 0; i < mem_cur_size; i++) {
			nlohmann::ordered_json mem_clk_json{};
			if (mem_chiplet["clk"][i].val == UINT64_MAX) {
				mem_clk_json["value"] = "N/A";
			} else {
				mem_clk_json["value"] = mem_chiplet["clk"][i].val;
			}
			mem_clk_json["unit"] = string_format("%llu", mem_chiplet["clk"][i].val) == "N/A" ? "N/A" :  "MHz";
			nlohmann::ordered_json mem_min_clk_json{};
			if (mem_chiplet["min_clk"][i].val == UINT64_MAX) {
				mem_min_clk_json["value"] = "N/A";
			} else {
				mem_min_clk_json["value"] = mem_chiplet["min_clk"][i].val;
			}
			mem_min_clk_json["unit"] = string_format("%llu",
									   mem_chiplet["min_clk"][i].val) == "N/A" ? "N/A" :  "MHz";
			nlohmann::ordered_json mem_max_clk_json{};
			if (mem_chiplet["max_clk"][i].val == UINT64_MAX) {
				mem_max_clk_json["value"] = "N/A";
			} else {
				mem_max_clk_json["value"] = mem_chiplet["max_clk"][i].val;
			}
			mem_max_clk_json["unit"] = string_format("%llu",
									   mem_chiplet["max_clk"][i].val) == "N/A" ? "N/A" :  "MHz";
			std::string clk_locked{"N/A"};
			if (mem_chiplet["clk_locked"].size()) {
				clk_locked = mem_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 mem_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			std::string clk_deep_sleep{"N/A"};
			if(mem_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = mem_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 mem_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_MEM);
			}
			result[string_format("mem_%d",i)] = nlohmann::ordered_json::object( {
				{"clk", mem_clk_json },
				{"min_clk", mem_min_clk_json },
				{"max_clk", mem_max_clk_json },
				{"clk_locked", "N/A" },
				{"deep_sleep", clk_deep_sleep }});

			for (int i = 0; i < vclk_cur_size; i++) {
				nlohmann::ordered_json vclk_clk_json{};
				if (vclk_chiplet["clk"][i].val == UINT64_MAX) {
					vclk_clk_json["value"] = "N/A";
				} else {
					vclk_clk_json["value"] = vclk_chiplet["clk"][i].val;
				}
				vclk_clk_json["unit"] = string_format("%llu",
													  vclk_chiplet["clk"][i].val) == "N/A" ? "N/A" : "MHz";
				nlohmann::ordered_json vclk_min_clk_json{};
				if (vclk_chiplet["min_clk"][i].val == UINT64_MAX) {
					vclk_min_clk_json["value"] = "N/A";
				} else {
					vclk_min_clk_json["value"] = vclk_chiplet["min_clk"][i].val;
				}
				vclk_min_clk_json["unit"] = string_format("%llu",
											vclk_chiplet["min_clk"][i].val) == "N/A" ? "N/A" : "MHz";
				nlohmann::ordered_json vclk_max_clk_json{};
				if (vclk_chiplet["max_clk"][i].val == UINT64_MAX) {
					vclk_max_clk_json["value"] = "N/A";
				} else {
					vclk_max_clk_json["value"] = vclk_chiplet["max_clk"][i].val;
				}
				vclk_max_clk_json["unit"] = string_format("%llu",
											vclk_chiplet["max_clk"][i].val) == "N/A" ? "N/A" : "MHz";
				std::string clk_locked{"N/A"};
				std::string clk_deep_sleep{"N/A"};
				if (vclk_chiplet["clk_locked"].size()) {
					clk_locked = vclk_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
								 vclk_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
				}
				if (vclk_chiplet["deep_sleep"].size()) {
					clk_deep_sleep = vclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
									 vclk_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
				} else {
					clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_VCLK0);
				}
				result[string_format("vclk_%d", i)] = nlohmann::ordered_json::object( {
					{"clk", vclk_clk_json },
					{"min_clk", vclk_min_clk_json },
					{"max_clk", vclk_max_clk_json },
					{"clk_locked", "N/A" },
					{"deep_sleep",  clk_deep_sleep }});
			}

			for (int i = 0; i < dclk_cur_size; i++) {
				nlohmann::ordered_json dclk_clk_json{};
				if (dclk_chiplet["clk"][i].val == UINT64_MAX) {
					dclk_clk_json["value"] = "N/A";
				} else {
					dclk_clk_json["value"] = dclk_chiplet["clk"][i].val;
				}
				dclk_clk_json["unit"] = string_format("%llu",
													  dclk_chiplet["clk"][i].val) == "N/A" ? "N/A" : "MHz";
				nlohmann::ordered_json dclk_min_clk_json{};
				if (dclk_chiplet["min_clk"][i].val == UINT64_MAX) {
					dclk_min_clk_json["value"] = "N/A";
				} else {
					dclk_min_clk_json["value"] = dclk_chiplet["min_clk"][i].val;
				}
				dclk_min_clk_json["unit"] = string_format("%llu",
											dclk_chiplet["min_clk"][i].val) == "N/A" ? "N/A" : "MHz";
				nlohmann::ordered_json dclk_max_clk_json{};
				if (dclk_chiplet["max_clk"][i].val == UINT64_MAX) {
					dclk_max_clk_json["value"] = "N/A";
				} else {
					dclk_max_clk_json["value"] = dclk_chiplet["max_clk"][i].val;
				}
				dclk_max_clk_json["unit"] = string_format("%llu",
											dclk_chiplet["max_clk"][i].val) == "N/A" ? "N/A" : "MHz";
				std::string clk_locked{"N/A"};
				std::string clk_deep_sleep{"N/A"};
				if (dclk_chiplet["clk_locked"].size()) {
					clk_locked = dclk_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
								 dclk_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
				}
				if (dclk_chiplet["deep_sleep"].size()) {
					clk_deep_sleep = dclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
									 dclk_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
				} else {
					clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_DCLK0);
				}
				result[string_format("dclk_%d",i)] = nlohmann::ordered_json::object( {
					{"clk", dclk_clk_json },
					{"min_clk", dclk_min_clk_json },
					{"max_clk", dclk_max_clk_json },
					{"clk_locked", "N/A" },
					{"deep_sleep", clk_deep_sleep }});

				out = result.dump(4);
			}
		}
	} else if (arg.output == csv) {
		std::vector<std::vector<std::string>> output_rows{};
		std::vector<std::string> value_rows{};
		for (int i = 0; i < gfx_cur_size; i++) {
			std::string clk_locked{"N/A"};
			std::string clk_deep_sleep{"N/A"};
			if (gfx_chiplet["clk_locked"].size()) {
				clk_locked = gfx_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 gfx_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			if(gfx_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = gfx_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 gfx_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_GFX);
			}
			value_rows.push_back(string_format(",%d,%s,%s,%s,%s,%s", i,
											   gfx_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   gfx_chiplet["clk"][i].val).c_str(),
											   gfx_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   gfx_chiplet["min_clk"][i].val).c_str(),
											   gfx_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   gfx_chiplet["max_clk"][i].val).c_str(), clk_locked.c_str(), clk_deep_sleep.c_str()));
		}
		output_rows.push_back(value_rows);
		value_rows.clear();
		for (int i = 0; i < mem_cur_size; i++) {
			std::string clk_locked{"N/A"};
			std::string clk_deep_sleep{"N/A"};
			if (mem_chiplet["clk_locked"].size()) {
				clk_locked = mem_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 mem_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			if(mem_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = mem_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 mem_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_MEM);
			}
			value_rows.push_back(string_format(",%d,%s,%s,%s,%s,%s", i,
											   mem_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   mem_chiplet["clk"][i].val).c_str(),
											   mem_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   mem_chiplet["min_clk"][i].val).c_str(),
											   mem_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   mem_chiplet["max_clk"][i].val).c_str(), "N/A", clk_deep_sleep.c_str()));
		}
		output_rows.push_back(value_rows);
		value_rows.clear();

		for (int i = 0; i < vclk_cur_size; i++) {
			std::string clk_locked{"N/A"};
			std::string clk_deep_sleep{"N/A"};
			if (vclk_chiplet["clk_locked"].size()) {
				clk_locked = vclk_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 vclk_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			if(vclk_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = vclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 vclk_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_VCLK0);
			}
			value_rows.push_back(string_format(",%d,%s,%s,%s,%s,%s", i,
											   vclk_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   vclk_chiplet["clk"][i].val).c_str(),
											   vclk_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   vclk_chiplet["min_clk"][i].val).c_str(),
											   vclk_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   vclk_chiplet["max_clk"][i].val).c_str(), "N/A", clk_deep_sleep.c_str()));
		}
		output_rows.push_back(value_rows);
		value_rows.clear();

		for (int i = 0; i < dclk_cur_size; i++) {
			std::string clk_locked{"N/A"};
			std::string clk_deep_sleep{"N/A"};
			if (dclk_chiplet["clk_locked"].size()) {
				clk_locked = dclk_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 dclk_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			if(dclk_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = dclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 dclk_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_DCLK0);
			}
			value_rows.push_back(string_format(",%d,%s,%s,%s,%s,%s", i,
											   dclk_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   dclk_chiplet["clk"][i].val).c_str(),
											   dclk_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   dclk_chiplet["min_clk"][i].val).c_str(),
											   dclk_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
													   dclk_chiplet["max_clk"][i].val).c_str(), "N/A", clk_deep_sleep.c_str()));
		}
		output_rows.push_back(value_rows);
		value_rows.clear();

		csv_recursion(out, output_rows);
	} else {
		out.append(metricClockMeasureHostHeaderTemplate);

		for (int i = 0; i < gfx_cur_size; i++) {
			std::string clk_locked{"N/A"};
			std::string clk_deep_sleep{"N/A"};
			if (gfx_chiplet["clk_locked"].size()) {
				clk_locked = gfx_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 gfx_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			if(gfx_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = gfx_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 gfx_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_GFX);
			}
			out.append(string_format(metricChipletGfxClockMeasureHostTemplate, i,
									 gfx_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 gfx_chiplet["clk"][i].val).c_str(),
									 gfx_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 gfx_chiplet["min_clk"][i].val).c_str(),
									 gfx_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 gfx_chiplet["max_clk"][i].val).c_str(), clk_locked.c_str(), clk_deep_sleep.c_str()));
		}
		for (int i = 0; i < mem_cur_size; i++) {
			std::string clk_locked{"N/A"};
			std::string clk_deep_sleep{"N/A"};
			if (mem_chiplet["clk_locked"].size()) {
				clk_locked = mem_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 mem_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			if(mem_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = mem_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 mem_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_MEM);
			}
			out.append(string_format(metricChipletMemClockMeasureHostTemplate, i,
									 mem_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 mem_chiplet["clk"][i].val).c_str(),
									 mem_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 mem_chiplet["min_clk"][i].val).c_str(),
									 mem_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 mem_chiplet["max_clk"][i].val).c_str(), "N/A", clk_deep_sleep.c_str()));
		}

		for (int i = 0; i < vclk_cur_size; i++) {
			std::string clk_locked{"N/A"};
			std::string clk_deep_sleep{"N/A"};
			if (vclk_chiplet["clk_locked"].size()) {
				clk_locked = vclk_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 vclk_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			if(vclk_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = vclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 vclk_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_VCLK0);
			}
			out.append(string_format(metricChipletVCLKClockMeasureHostTemplate, i,
									 vclk_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 vclk_chiplet["clk"][i].val).c_str(),
									 vclk_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 vclk_chiplet["min_clk"][i].val).c_str(),
									 vclk_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 vclk_chiplet["max_clk"][i].val).c_str(), "N/A", clk_deep_sleep.c_str()));
		}

		for (int i = 0; i < dclk_cur_size; i++) {
			std::string clk_locked{"N/A"};
			std::string clk_deep_sleep{"N/A"};
			if (dclk_chiplet["clk_locked"].size()) {
				clk_locked = dclk_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" :
							 dclk_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED";
			}
			if(dclk_chiplet["deep_sleep"].size()) {
				clk_deep_sleep = dclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" :
								 dclk_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED";
			} else {
				clk_deep_sleep = get_clk_deep_sleep(processor, AMDSMI_CLK_TYPE_DCLK0);
			}
			out.append(string_format(metricChipletDCLKClockMeasureHostTemplate, i,
									 dclk_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 dclk_chiplet["clk"][i].val).c_str(),
									 dclk_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 dclk_chiplet["min_clk"][i].val).c_str(),
									 dclk_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
											 dclk_chiplet["max_clk"][i].val).c_str(), "N/A", clk_deep_sleep.c_str()));
		}
	}
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_clock_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	bool use_metric{false};

	if ((AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi350())
			&& arg.watch == -1) {
		use_metric = true;
	}

	if (use_metric) {
		ret = get_metric_ext_clock_data(processor_bdf, arg, out);
	} else {
		ret = get_metric_clock_data(processor_bdf, arg, out);
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_temperature_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	int64_t edge_temperature;
	int64_t junction_temperature;
	int64_t vram_temperature;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_temperature(arg, "N/A");
		return ret;
	}
	std::string edge_temperature_string{};
	ret = host_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_CURRENT, &edge_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		edge_temperature_string = "N/A";
	} else {
		edge_temperature_string = (edge_temperature == UINT_MAX) ? "N/A" :
								  string_format(
									  "%lld", edge_temperature);
	}
	std::string junction_temperature_string{};
	ret = host_amdsmi_get_temp_metric(processor,
									  AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
									  AMDSMI_TEMP_CURRENT,
									  &junction_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		junction_temperature_string = "N/A";
	} else {
		junction_temperature_string = string_format("%lld", junction_temperature);
	}
	std::string vram_temperature_string{};
	ret = host_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_VRAM, AMDSMI_TEMP_CURRENT, &vram_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		vram_temperature_string = "N/A";
	} else {
		vram_temperature_string = string_format("%lld", vram_temperature);
	}

	if (arg.watch > -1) {
		out = string_format("%s,%s,%s", edge_temperature_string.c_str(),
							junction_temperature_string.c_str(), vram_temperature_string.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json temperature_json{};

		nlohmann::ordered_json edge_temperature_json{};
		if(edge_temperature == UINT_MAX) {
			edge_temperature_json["value"] = edge_temperature_string;
		} else {
			edge_temperature_json["value"] = edge_temperature;
		}
		edge_temperature_json["unit"] = edge_temperature_string == "N/A" ? "N/A" : "C";
		temperature_json["edge"] = edge_temperature_json;

		nlohmann::ordered_json hotspot_temperature_json{};
		hotspot_temperature_json["value"] = junction_temperature;
		hotspot_temperature_json["unit"] = junction_temperature_string == "N/A" ? "N/A" : "C";
		temperature_json["hotspot"] = hotspot_temperature_json;
		nlohmann::ordered_json vram_temperature_json{};
		vram_temperature_json["value"] = vram_temperature;
		vram_temperature_json["unit"] = vram_temperature_string == "N/A" ? "N/A" : "C";
		temperature_json["mem"] = vram_temperature_json;

		out = temperature_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s", edge_temperature_string.c_str(),
				  junction_temperature_string.c_str(), vram_temperature_string.c_str());
	} else {
		std::string edge_temperature_string_unit = edge_temperature_string == "N/A" ? "" : "C";
		std::string junction_temperature_string_unit = junction_temperature_string == "N/A" ? "" : "C";
		std::string vram_temperature_string_unit = vram_temperature_string == "N/A" ? "" : "C";
		out = string_format(
				  metricThermalMeasureTemplate, edge_temperature_string.c_str(), edge_temperature_string_unit.c_str(),
				  junction_temperature_string.c_str(), junction_temperature_string_unit.c_str(),
				  vram_temperature_string.c_str(),  vram_temperature_string_unit.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_ecc_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_error_count_t total_error_count;
	amdsmi_error_count_t umc_error_count;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_ecc(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_gpu_total_ecc_count(processor, &total_error_count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_ecc(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_gpu_ecc_count(processor, AMDSMI_GPU_BLOCK_UMC, &umc_error_count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_ecc(arg, "N/A");
		return ret;
	}

	std::string total_error_correctable_count{};
	if (total_error_count.correctable_count == UINT64_MAX) {
		total_error_correctable_count = string_format("%s", "N/A");
	} else {
		total_error_correctable_count = string_format("%lld", total_error_count.correctable_count);
	}

	std::string total_error_uncorrectable_count{};
	if (total_error_count.uncorrectable_count == UINT64_MAX) {
		total_error_uncorrectable_count = string_format("%s", "N/A");
	} else {
		total_error_uncorrectable_count = string_format("%lld", total_error_count.uncorrectable_count);
	}

	std::string total_error_deferred_count{};
	if (total_error_count.deferred_count == UINT64_MAX) {
		total_error_deferred_count = string_format("%s", "N/A");
	} else {
		total_error_deferred_count = string_format("%lld", total_error_count.deferred_count);
	}

	uint64_t cache_correctable{};
	std::string cache_correctable_str{};
	if (total_error_count.correctable_count == UINT64_MAX
			|| umc_error_count.correctable_count == UINT64_MAX) {
		cache_correctable_str = string_format("%s", "N/A");
	} else {
		cache_correctable = total_error_count.correctable_count - umc_error_count.correctable_count;
		cache_correctable_str = string_format( "%lld", cache_correctable);
	}

	uint64_t cache_uncorrectable{};
	std::string cache_uncorrectable_str{};
	if (total_error_count.uncorrectable_count == UINT64_MAX
			|| umc_error_count.uncorrectable_count == UINT64_MAX) {
		cache_uncorrectable_str = string_format("%s", "N/A");
	} else {
		cache_uncorrectable = total_error_count.uncorrectable_count - umc_error_count.uncorrectable_count;
		cache_uncorrectable_str = string_format( "%lld", cache_uncorrectable);
	}

	nlohmann::ordered_json error_count_json{};
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s,%s", total_error_correctable_count.c_str(),
							total_error_uncorrectable_count.c_str(), total_error_deferred_count.c_str(),
							cache_correctable_str.c_str(), cache_uncorrectable_str.c_str());
	} else if (arg.output == human) {
		out = string_format(
				  metricEccErrorCountTemplate, total_error_correctable_count.c_str(),
				  total_error_uncorrectable_count.c_str(), total_error_deferred_count.c_str(),
				  cache_correctable_str.c_str(),
				  cache_uncorrectable_str.c_str());
	} else if (arg.output == json) {
		error_count_json["total_correctable_count"] = total_error_count.correctable_count;
		error_count_json["total_uncorrectable_count"] = total_error_count.uncorrectable_count;
		if (total_error_count.deferred_count == UINT64_MAX) {
			error_count_json["total_deferred_count"] = "N/A";
		} else {
			error_count_json["total_deferred_count"] = total_error_count.deferred_count;
		}
		error_count_json["cache_correctable_count"] = cache_correctable;
		error_count_json["cache_uncorrectable_count"] = cache_uncorrectable;
	} else if (arg.output == csv) {
		out.append(string_format(",%s,%s,%s,%s,%s\n", total_error_correctable_count.c_str(),
								 total_error_uncorrectable_count.c_str(), total_error_deferred_count.c_str(),
								 cache_correctable_str.c_str(), cache_uncorrectable_str.c_str()));
	}

	if (arg.output == json) {
		out = error_count_json.dump(4);
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_ecc_block_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;
	bool display_output = false;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_ecc_block(arg, "N/A");
		return ret;
	}

	nlohmann::ordered_json ecc_blocks_json{};
	if (arg.output == human) {
		out.append(metricEccBlockErrorCountHeaderTemplate);
	}

	uint64_t enabled_blocks{};
	ret = host_amdsmi_get_gpu_ecc_enabled(processor, &enabled_blocks);
	if (ret != AMDSMI_STATUS_SUCCESS && ret != AMDSMI_STATUS_NOT_SUPPORTED) {
		return ret;
	} else if (enabled_blocks == 0) {
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}

	amdsmi_error_count_t block_error_count;
	std::string block_str{};
	for (auto block : ecc_blocks) {
		if (enabled_blocks & block) {
			get_string_from_enum_ecc_blocks(block, block_str);
			ret = host_amdsmi_get_gpu_ecc_count(processor, block, &block_error_count);
			if (ret == AMDSMI_STATUS_SUCCESS) {
				display_output = true;
				std::string block_correctable_errors{string_format("%lld", block_error_count.correctable_count)};
				std::string block_uncorrectable_errors{string_format("%lld", block_error_count.uncorrectable_count)};
				std::string block_deferred_errors = (block_error_count.deferred_count == -1
													 || block_error_count.deferred_count == UINT64_MAX) ? "N/A" : string_format("%lld",
															 block_error_count.deferred_count);

				if (arg.watch > -1) {
				} else if (arg.output == json) {
					ecc_blocks_json[block_str.c_str()] = nlohmann::ordered_json::object( {
						{ "correctable_count", block_error_count.correctable_count },
						{ "uncorrectable_count", block_error_count.uncorrectable_count },
						{ "deferred_count", (block_error_count.deferred_count == -1 || block_error_count.deferred_count == UINT64_MAX) ? "N/A" : string_format("%lld", block_error_count.deferred_count) } });
				} else if (arg.output == csv) {
					out.append(string_format(",%s,%s,%s,%s\n", block_str.c_str(), block_correctable_errors.c_str(),
											 block_uncorrectable_errors.c_str(), block_deferred_errors.c_str()));
				} else if (arg.output == human) {
					out.append(string_format(metricEccBlockErrorCountTemplate, block_str.c_str(),
											 block_correctable_errors.c_str(), block_uncorrectable_errors.c_str(),
											 block_deferred_errors.c_str()));
				}
			} else if (ret == AMDSMI_STATUS_NOT_SUPPORTED) {
				if (arg.watch > -1) {
				} else if (arg.output == json) {
					ecc_blocks_json[block_str.c_str()] = nlohmann::ordered_json::object( {
						{ "correctable_count", "N/A" },
						{ "uncorrectable_count", "N/A" },
						{ "deferred_count", "N/A "} });
				} else if (arg.output == csv) {
					out.append(string_format(",%s,%s,%s,%s\n", "N/A", "N/A", "N/A", "N/A"));
				} else if (arg.output == human) {
					out.append(string_format(metricEccBlockErrorCountTemplate, "N/A",
											 "N/A", "N/A", "N/A"));
				}
			} else {
				return ret;
			}
		}
	}
	nlohmann::ordered_json error_count_json{};

	if (display_output) {
		if (arg.output == json) {
			out = ecc_blocks_json.dump(4);
		}
	} else {
		out = "";
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_pcie_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_pcie_info_t pcie_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_pcie(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_pcie_info(processor, &pcie_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_pcie(arg, "N/A");
		return ret;
	}

	std::string pcie_info_pcie_lanes{ string_format(
										  "%ld", pcie_info.pcie_metric.pcie_width) };
	std::string pcie_info_pcie_speed{ string_format(
										  "%lld", pcie_info.pcie_metric.pcie_speed / 1000) };

	std::string pcie_bandwidth = (pcie_info.pcie_metric.pcie_bandwidth == UINT_MAX) ? "N/A" :
								 string_format(
									 "%ld", pcie_info.pcie_metric.pcie_bandwidth);

	std::string pcie_replay_count = (pcie_info.pcie_metric.pcie_replay_count == UINT64_MAX) ? "N/A" :
									string_format(
										"%lld", pcie_info.pcie_metric.pcie_replay_count);
	std::string pcie_l0_to_recovery_count = (pcie_info.pcie_metric.pcie_l0_to_recovery_count ==
											UINT64_MAX) ? "N/A" :
											string_format(
													"%lld", pcie_info.pcie_metric.pcie_l0_to_recovery_count);
	std::string pcie_replay_roll_over_count = (pcie_info.pcie_metric.pcie_replay_roll_over_count ==
			UINT64_MAX || pcie_info.pcie_metric.pcie_replay_roll_over_count == UINT_MAX) ? "N/A" :
			string_format(
				"%lld", pcie_info.pcie_metric.pcie_replay_roll_over_count);

	std::string pcie_nak_sent_count = (pcie_info.pcie_metric.pcie_nak_sent_count == UINT64_MAX
									   || pcie_info.pcie_metric.pcie_nak_sent_count == UINT_MAX) ?
									  "N/A" :
									  string_format(
										  "%lld", pcie_info.pcie_metric.pcie_nak_sent_count);
	std::string pcie_nak_received_count = (pcie_info.pcie_metric.pcie_nak_received_count == UINT64_MAX)
										  ? "N/A" :
										  string_format(
												  "%lld", pcie_info.pcie_metric.pcie_nak_received_count);

	if (arg.watch > -1) {
		out = string_format("%s,%s", pcie_info_pcie_lanes.c_str(),
							pcie_info_pcie_speed.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json current_speed{};
		current_speed["value"] = pcie_info.pcie_metric.pcie_speed / 1000;
		current_speed["unit"] = pcie_info_pcie_speed == "N/A" ? "N/A" : "GT/s";
		nlohmann::ordered_json pcie_info_json = { { "width", pcie_info.pcie_metric.pcie_width },
			{ "speed", current_speed }
		};
		nlohmann::ordered_json current_bandwidth{};
		if(pcie_info.pcie_metric.pcie_bandwidth == UINT_MAX) {
			current_bandwidth["value"] = "N/A";
		} else {
			current_bandwidth["value"] = pcie_info.pcie_metric.pcie_bandwidth;
		}
		current_bandwidth["unit"] = pcie_bandwidth == "N/A" ? "N/A" : "Mb/s";
		pcie_info_json["bandwidth"] = current_bandwidth;
		if(pcie_info.pcie_metric.pcie_replay_count == UINT64_MAX) {
			pcie_info_json["replay_count"] = "N/A";
		} else {
			pcie_info_json["replay_count"] = pcie_info.pcie_metric.pcie_replay_count;
		}
		if(pcie_info.pcie_metric.pcie_l0_to_recovery_count == UINT64_MAX) {
			pcie_info_json["l0_to_recovery_count"] = "N/A";
		} else {
			pcie_info_json["l0_to_recovery_count"] = pcie_info.pcie_metric.pcie_l0_to_recovery_count;
		}
		if(pcie_info.pcie_metric.pcie_replay_roll_over_count == UINT64_MAX) {
			pcie_info_json["replay_roll_over_count"] = "N/A";
		} else {
			pcie_info_json["replay_roll_over_count"] = pcie_info.pcie_metric.pcie_replay_roll_over_count;
		}
		if(pcie_info.pcie_metric.pcie_nak_sent_count == UINT64_MAX) {
			pcie_info_json["nak_sent_count"] = "N/A";
		} else {
			pcie_info_json["nak_sent_count"] = pcie_info.pcie_metric.pcie_nak_sent_count;
		}
		if(pcie_info.pcie_metric.pcie_nak_received_count == UINT64_MAX) {
			pcie_info_json["nak_received_count"] = "N/A";
		} else {
			pcie_info_json["nak_received_count"] = pcie_info.pcie_metric.pcie_nak_received_count;
		}

		out = pcie_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s,%s,%s", pcie_info_pcie_lanes.c_str(),
				  pcie_info_pcie_speed.c_str(), pcie_bandwidth.c_str(), pcie_replay_count.c_str(),
				  pcie_l0_to_recovery_count.c_str(), pcie_replay_roll_over_count.c_str(), pcie_nak_sent_count.c_str(),
				  pcie_nak_received_count.c_str());
	} else {
		std::string pcie_info_pcie_speed_unit = pcie_info_pcie_speed == "N/A" ? "" : "GT/s";
		std::string pcie_bandwidth_unit = pcie_bandwidth == "N/A" ? "" : "Mb/s";

		out = string_format(
				  pcieInfoHostTemplate,  pcie_info_pcie_lanes.c_str(),
				  pcie_info_pcie_speed.c_str(), pcie_info_pcie_speed_unit.c_str(), pcie_bandwidth.c_str(),
				  pcie_bandwidth_unit.c_str(), pcie_replay_count.c_str(),
				  pcie_l0_to_recovery_count.c_str(), pcie_replay_roll_over_count.c_str(), pcie_nak_sent_count.c_str(),
				  pcie_nak_received_count.c_str());
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_schedule_metric_command(std::string device, Arguments arg,
		std::string& out)
{
	amdsmi_bdf_t vf_bdf;
	amdsmi_vf_handle_t vf_handle;
	amdsmi_status_t ret;
	amdsmi_vf_data_t vf_data;

	vf_bdf.bdf.domain_number = std::stoi(device.substr(0, 4), nullptr, 16);
	vf_bdf.bdf.bus_number = std::stoi(device.substr(5, 2), nullptr, 16);
	vf_bdf.bdf.device_number = std::stoi(device.substr(8, 2), nullptr, 16);
	vf_bdf.bdf.function_number = std::stoi(device.substr(11), nullptr, 16);

	ret = host_amdsmi_get_vf_handle_from_bdf(vf_bdf, &vf_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_schedule(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_vf_data(vf_handle, &vf_data);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_schedule(arg, "N/A");
		return ret;
	}

	uint64_t boot_up_time = vf_data.sched.boot_up_time;
	uint64_t flr_count = vf_data.sched.flr_count;
	std::string vf_state_str;
	get_string_from_enum_vf_sched_state(vf_data.sched.state, vf_state_str);
	std::string last_boot_start = vf_data.sched.last_boot_start;
	std::string last_boot_end = vf_data.sched.last_boot_end;
	std::string last_shutdown_start = vf_data.sched.last_shutdown_start;
	std::string last_shutdown_end = vf_data.sched.last_shutdown_end;
	uint64_t shutdown_time = vf_data.sched.shutdown_time;
	std::string last_reset_start = vf_data.sched.last_reset_start;
	std::string last_reset_end = vf_data.sched.last_reset_end;
	uint64_t reset_time = vf_data.sched.reset_time;
	std::string current_active_time = vf_data.sched.current_active_time;
	std::string current_running_time = vf_data.sched.current_running_time;
	std::string total_active_time = vf_data.sched.total_active_time;
	std::string total_running_time = vf_data.sched.total_running_time;

	std::string boot_up_time_str =
		string_format("%lu", boot_up_time);
	std::string flr_count_str =
		string_format("%lu", flr_count);
	std::string shutdown_time_str = (last_shutdown_start == "--N/A--") ? "--N/A--" :
									string_format("%lu",
											shutdown_time);
	std::string reset_time_str = (last_reset_start == "--N/A--") ? "--N/A--" : string_format("%lu",
								 reset_time);

	if (arg.watch > -1) {
		out = string_format(
				  "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", boot_up_time_str.c_str(), flr_count_str.c_str(),
				  vf_state_str.c_str(), last_boot_start.c_str(), last_boot_end.c_str(),
				  last_shutdown_start.c_str(), last_shutdown_end.c_str(), shutdown_time_str.c_str(),
				  last_reset_start.c_str(), last_reset_end.c_str(), reset_time_str.c_str(),
				  current_active_time.c_str(), current_running_time.c_str(), total_active_time.c_str(),
				  total_running_time.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json boot_up_time{};
		boot_up_time["value"] = boot_up_time_str.c_str();
		boot_up_time["unit"] = boot_up_time_str == "--N/A--" ? "N/A" : "us";
		nlohmann::ordered_json shutdown_time{};
		shutdown_time["value"] = shutdown_time_str.c_str();
		shutdown_time["unit"] = shutdown_time_str == "--N/A--" ? "N/A" : "us";
		nlohmann::ordered_json reset_time{};
		reset_time["value"] = reset_time_str.c_str();
		reset_time["unit"] = reset_time_str == "--N/A--" ? "N/A" : "us";
		nlohmann::ordered_json schedule_info_json = { { "boot_up_time", boot_up_time },
			{ "flr_count", flr_count_str.c_str() },
			{ "vf_state", vf_state_str.c_str() },
			{ "last_boot_start", last_boot_start.c_str() },
			{ "last_boot_end", last_boot_end.c_str() },
			{ "last_shutdown_start", last_shutdown_start.c_str() },
			{ "last_shutdown_end", last_shutdown_end.c_str() },
			{ "shutdown_time", shutdown_time },
			{ "last_reset_start", last_reset_start.c_str() },
			{ "last_reset_end", last_reset_end.c_str() },
			{ "reset_time", reset_time },
			{ "active_time", current_active_time.c_str() },
			{ "running_time", current_running_time.c_str() },
			{ "total_active_time", total_active_time.c_str() },
			{ "total_running_time", total_running_time.c_str() }
		};
		out = schedule_info_json.dump(4);
	} else if (arg.output == csv) {
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(device);
		out = string_format(
				  "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", std::get<0>(indexes).c_str(),
				  std::get<1>(indexes).c_str(), boot_up_time_str.c_str(), flr_count_str.c_str(),
				  vf_state_str.c_str(), last_boot_start.c_str(), last_boot_end.c_str(),
				  last_shutdown_start.c_str(), last_shutdown_end.c_str(), shutdown_time_str.c_str(),
				  last_reset_start.c_str(), last_reset_end.c_str(), reset_time_str.c_str(),
				  current_active_time.c_str(), current_running_time.c_str(), total_active_time.c_str(),
				  total_running_time.c_str());
	} else {
		std::string boot_up_time_str_unit = boot_up_time_str == "--N/A--" ? "" : " us";
		std::string shutdown_time_str_unit = shutdown_time_str == "--N/A--" ? "" : " us";
		std::string reset_time_str_unit = reset_time_str == "--N/A--" ? "" : " us";
		out = string_format(
				  metricScheduleTemplate, boot_up_time_str.c_str(), boot_up_time_str_unit.c_str(),
				  flr_count_str.c_str(),
				  vf_state_str.c_str(), last_boot_start.c_str(), last_boot_end.c_str(),
				  last_shutdown_start.c_str(), last_shutdown_end.c_str(), shutdown_time_str.c_str(),
				  shutdown_time_str_unit.c_str(),
				  last_reset_start.c_str(), last_reset_end.c_str(), reset_time_str.c_str(),
				  reset_time_str_unit.c_str(),
				  current_active_time.c_str(), current_running_time.c_str(), total_active_time.c_str(),
				  total_running_time.c_str());
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_guard_metric_command(std::string device, Arguments arg,
		std::string& out)
{
	amdsmi_bdf_t vf_bdf;
	amdsmi_vf_handle_t vf_handle;
	amdsmi_status_t ret;
	amdsmi_vf_data_t vf_data;
	nlohmann::ordered_json guard_types_json{};
	std::string vf_state_str;
	std::string guard_info_str;
	uint32_t amount;
	uint64_t interval;
	uint32_t threshold;
	uint32_t active;

	vf_bdf.bdf.domain_number = std::stoi(device.substr(0, 4), nullptr, 16);
	vf_bdf.bdf.bus_number = std::stoi(device.substr(5, 2), nullptr, 16);
	vf_bdf.bdf.device_number = std::stoi(device.substr(8, 2), nullptr, 16);
	vf_bdf.bdf.function_number = std::stoi(device.substr(11), nullptr, 16);

	ret = host_amdsmi_get_vf_handle_from_bdf(vf_bdf, &vf_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	ret = host_amdsmi_get_vf_data(vf_handle, &vf_data);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	amdsmi_guard_type_t guard_type = AMDSMI_GUARD_EVENT_FLR;
	std::string is_enabled_str{vf_data.guard.enabled ? "TRUE" : "FALSE"};
	std::tuple<std::string, std::string, std::string> indexes =
		getGpuVfIndexFromVfId(device);
	std::string gpu_id_str = std::get<0>(indexes);
	std::string vf_id_str = std::get<1>(indexes);

	for (auto guard : vf_data.guard.guard) {
		get_string_from_enum_vf_guard_state(guard.state, vf_state_str);
		amount = guard.amount;
		interval = guard.interval;
		threshold = guard.threshold;
		active = guard.active;

		std::string amount_str =
			string_format("%u", amount);
		std::string interval_str =
			string_format("%lu", interval);
		std::string threshold_str =
			string_format("%u", threshold);
		std::string active_str =
			string_format("%u", active);

		std::string guard_type_str;
		get_string_from_enum_vf_guard_type(guard_type, guard_type_str);

		guard_type = static_cast<amdsmi_guard_type_t>(static_cast<int>(guard_type) + 1);
		if (arg.watch > -1) {
			if (guard_info_str.size() > 0) {
				guard_info_str.append(",");
			}
			guard_info_str += string_format("%s,%s,%s,%s,%s,%s,%s", is_enabled_str.c_str(),
											guard_type_str.c_str(), vf_state_str.c_str(), amount_str.c_str(), interval_str.c_str(),
											threshold_str.c_str(), active_str.c_str());
		} else if (arg.output == json) {
			nlohmann::ordered_json intervals{};
			intervals["value"] = interval;
			intervals["unit"] = interval_str == "N/A" ? "N/A" : "s";
			nlohmann::ordered_json guard_json = { { "guard_state", vf_state_str.c_str() },
				{ "amount", amount },
				{ "interval", intervals },
				{ "threshold", threshold },
				{ "active",  active }
			};
			guard_types_json[guard_type_str.c_str()] = guard_json;
		} else if (arg.output == csv) {
			bool is_vf_schedule = ((std::find(arg.options.begin(), arg.options.end(),
											  "schedule") != arg.options.end()) ||
								   (std::find(arg.options.begin(), arg.options.end(), "s") != arg.options.end()));
			if(is_vf_schedule || arg.all_arguments) {
				guard_info_str.append(out.c_str()).append(",");
			} else if (!arg.all_arguments) {
				guard_info_str.append(gpu_id_str.c_str()).append(",").append(vf_id_str.c_str()).append(",");
			}
			guard_info_str += string_format(
								  "%s,%s,%s,%s,%s,%s,%s\n", is_enabled_str.c_str(), guard_type_str.c_str(), vf_state_str.c_str(),
								  amount_str.c_str(), interval_str.c_str(), threshold_str.c_str(), active_str.c_str());
		} else {
			std::string interval_str_unit = interval_str == "N/A" ? "" : "s";
			guard_info_str += string_format(
								  metricGuardInfoTemplate, guard_type_str.c_str(), vf_state_str.c_str(),
								  amount_str.c_str(), interval_str.c_str(), interval_str_unit.c_str(), threshold_str.c_str(),
								  active_str.c_str());
		}
	}

	if (arg.watch > -1) {
		out = guard_info_str;
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json guard_json_out = {
			{ "enabled",  is_enabled_str },
			{ "guard_info", guard_types_json }
		};
		out = guard_json_out.dump(4);
	} else if (arg.output == csv) {
		out = guard_info_str;
	} else {
		out = string_format(
				  metricGuardTemplate, is_enabled_str.c_str(), guard_info_str.c_str());
	}
	return ret;
}

int AmdSmiApiHost::amdsmi_get_metric_command_per_partition(uint64_t processor_bdf,
		uint64_t vf_index, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	uint32_t num_vf_supported;
	uint32_t num_vf_enabled;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;
	uint32_t max_aid_num = MAX_AID_NUM;
	amdsmi_accelerator_partition_profile_t curr_profile;
	uint32_t partition_ids[AMDSMI_MAX_ACCELERATOR_PARTITIONS];

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	ret = host_amdsmi_get_num_vf(processor, &num_vf_enabled, &num_vf_supported);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	ret = host_amdsmi_get_partition_profile(processor, &curr_profile, partition_ids);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	std::string num_partition = string_format("%d", curr_profile.num_partitions);
	uint32_t num_partitions = curr_profile.num_partitions;

	// Calculate AID assignment per VF
	auto get_aids_for_vf = [&](uint32_t vf_idx, uint32_t num_vf) -> std::vector<int> {
		std::vector<int> aids;
		if (num_vf == 1)
		{
			// All AIDs for the only VF
			for (int i = 0; i < (int)max_aid_num; ++i) aids.push_back(i);
		} else if (num_vf == 2)
		{
			// Each VF gets two AIDs
			aids.push_back(vf_idx * 2);
			aids.push_back(vf_idx * 2 + 1);
		} else if (num_vf == 4)
		{
			// Each VF gets one AID
			aids.push_back(vf_idx);
		} else if (num_vf == 8)
		{
			// Each two VFs share an AID
			aids.push_back(vf_idx / 2);
		}
		return aids;
	};

	std::vector<amdsmi_metric_t> vcn_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> jpeg_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> temp_aid_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> temp_hbm_metrics_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> vclk_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> vclk_min_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> vclk_max_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> dclk_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> dclk_min_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> dclk_max_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> sclk_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> sclk_min_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> sclk_max_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> gfx_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> gfx_min_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> gfx_max_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> gfx_locked_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> gfx_usage_chiplet_per_partition{};
	std::vector<amdsmi_metric_t> temp_xcd_chiplet_per_partition{};

	std::vector<amdsmi_metric_t> throttle_ppt_a{}, throttle_ppt_b{};
	std::vector<amdsmi_metric_t> throttle_thm_a{}, throttle_thm_b{};
	std::vector<amdsmi_metric_t> throttle_total_a{}, throttle_total_b{};
	std::vector<amdsmi_metric_t> throttle_util_a{}, throttle_util_b{};
	uint64_t acc_counter_a = UINT64_MAX, acc_counter_b = UINT64_MAX;

	amdsmi_metric_t *metrics;
	uint32_t metric_size = AMDSMI_MAX_NUM_METRICS;

	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t) * metric_size);
	if (metrics == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, &metrics[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(metrics);
		return ret;
	}

	for (uint32_t i = 0; i < metric_size; i++) {
		if (!(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC) &&
				(metrics[i].flags & AMDSMI_METRIC_TYPE_CHIPLET)) {
			switch (metrics[i].name) {
				case AMDSMI_METRIC_NAME_USAGE_VCN:
					vcn_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_USAGE_JPEG:
					jpeg_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_TEMP_AID:
					temp_aid_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_TEMP_MEM_CURR:
					temp_hbm_metrics_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_VCLK:
					vclk_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_VCLK_MIN_LIMIT:
					vclk_min_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_VCLK_MAX_LIMIT:
					vclk_max_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_DCLK:
					dclk_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_DCLK_MIN_LIMIT:
					dclk_min_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_DCLK_MAX_LIMIT:
					dclk_max_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_SOC:
					sclk_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_SOC_MIN_LIMIT:
					sclk_min_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_SOC_MAX_LIMIT:
					sclk_max_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_GFX:
					gfx_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_GFX_MIN_LIMIT:
					gfx_min_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_GFX_MAX_LIMIT:
					gfx_max_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_CLK_GFX_LOCKED:
					gfx_locked_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_USAGE_GFX:
					gfx_usage_chiplet_per_partition.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_TEMP_XCD:
					temp_xcd_chiplet_per_partition.push_back(metrics[i]);
					break;
				default:
					break;
			}
		}
		switch (metrics[i].name) {
		case AMDSMI_METRIC_NAME_METRIC_ACC_COUNTER:
			acc_counter_a = metrics[i].val;
			break;
		case AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_PPT:
			if (metrics[i].res_group == AMDSMI_METRIC_RES_GROUP_XCP
					&& metrics[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC)
				throttle_ppt_a.push_back(metrics[i]);
			break;
		case AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_THM:
			if (metrics[i].res_group == AMDSMI_METRIC_RES_GROUP_XCP
					&& metrics[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC)
				throttle_thm_a.push_back(metrics[i]);
			break;
		case AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_TOTAL:
			if (metrics[i].res_group == AMDSMI_METRIC_RES_GROUP_XCP
					&& metrics[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC)
				throttle_total_a.push_back(metrics[i]);
			break;
		case AMDSMI_METRIC_NAME_GFX_CLK_LOW_UTILIZATION:
			if (metrics[i].res_group == AMDSMI_METRIC_RES_GROUP_XCP
					&& metrics[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC)
				throttle_util_a.push_back(metrics[i]);
			break;
		default:
			break;
		}
	}
	free(metrics);
	metrics = NULL;

	auto violation_ts_start = std::chrono::steady_clock::now();
	uint64_t violation_ts_delta_us = 0;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	uint32_t metric_size_b = AMDSMI_MAX_NUM_METRICS;
	if (host_amdsmi_get_gpu_metrics(processor, &metric_size_b, NULL) == AMDSMI_STATUS_SUCCESS) {
		amdsmi_metric_t *metrics_b = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t) * metric_size_b);
		if (metrics_b != NULL) {
			if (host_amdsmi_get_gpu_metrics(processor, &metric_size_b, &metrics_b[0]) == AMDSMI_STATUS_SUCCESS) {
				for (uint32_t i = 0; i < metric_size_b; i++) {
					switch (metrics_b[i].name) {
					case AMDSMI_METRIC_NAME_METRIC_ACC_COUNTER:
						acc_counter_b = metrics_b[i].val;
						break;
					case AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_PPT:
						if (metrics_b[i].res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& metrics_b[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC)
							throttle_ppt_b.push_back(metrics_b[i]);
						break;
					case AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_THM:
						if (metrics_b[i].res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& metrics_b[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC)
							throttle_thm_b.push_back(metrics_b[i]);
						break;
					case AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_TOTAL:
						if (metrics_b[i].res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& metrics_b[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC)
							throttle_total_b.push_back(metrics_b[i]);
						break;
					case AMDSMI_METRIC_NAME_GFX_CLK_LOW_UTILIZATION:
						if (metrics_b[i].res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& metrics_b[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC)
							throttle_util_b.push_back(metrics_b[i]);
						break;
					default:
						break;
					}
				}
				auto violation_ts_end = std::chrono::steady_clock::now();
				violation_ts_delta_us = std::chrono::duration_cast<std::chrono::microseconds>(
					violation_ts_end - violation_ts_start).count();
			}
			free(metrics_b);
		}
	}

	if (arg.output == json) {
		nlohmann::ordered_json result_json;
		for (uint32_t vf_curr_index = 0; vf_curr_index < num_vf_enabled; vf_curr_index++) {
			if (vf_curr_index == vf_index) {
				auto aids = get_aids_for_vf(vf_curr_index, num_vf_enabled);
				nlohmann::ordered_json aid_json;
				for (auto aid_index : aids) {
					// VCLK
					for (const auto& vclk : vclk_chiplet_per_partition) {
						if (vclk.res_group == AMDSMI_METRIC_RES_GROUP_AID && vclk.res_instance == aid_index) {
							aid_json["vclk"] = {
								{"value", vclk.val},
								{"unit", vclk.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : ""}
							};
						}
					}
					for (const auto& vclk_min : vclk_min_chiplet_per_partition) {
						if (vclk_min.res_group == AMDSMI_METRIC_RES_GROUP_AID && vclk_min.res_instance == aid_index) {
							aid_json["vclk_min"] = {
								{"value", vclk_min.val},
								{"unit", vclk_min.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : ""}
							};
						}
					}
					for (const auto& vclk_max : vclk_max_chiplet_per_partition) {
						if (vclk_max.res_group == AMDSMI_METRIC_RES_GROUP_AID && vclk_max.res_instance == aid_index) {
							aid_json["vclk_max"] = {
								{"value", vclk_max.val},
								{"unit", vclk_max.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : ""}
							};
						}
					}
					// DCLK
					for (const auto& dclk : dclk_chiplet_per_partition) {
						if (dclk.res_group == AMDSMI_METRIC_RES_GROUP_AID && dclk.res_instance == aid_index) {
							aid_json["dclk"] = {
								{"value", dclk.val},
								{"unit", dclk.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : ""}
							};
						}
					}
					for (const auto& dclk_min : dclk_min_chiplet_per_partition) {
						if (dclk_min.res_group == AMDSMI_METRIC_RES_GROUP_AID && dclk_min.res_instance == aid_index) {
							aid_json["dclk_min"] = {
								{"value", dclk_min.val},
								{"unit", dclk_min.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : ""}
							};
						}
					}
					for (const auto& dclk_max : dclk_max_chiplet_per_partition) {
						if (dclk_max.res_group == AMDSMI_METRIC_RES_GROUP_AID && dclk_max.res_instance == aid_index) {
							aid_json["dclk_max"] = {
								{"value", dclk_max.val},
								{"unit", dclk_max.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : ""}
							};
						}
					}
					// SCLK
					for (const auto& sclk : sclk_chiplet_per_partition) {
						if (sclk.res_group == AMDSMI_METRIC_RES_GROUP_AID && sclk.res_instance == aid_index) {
							aid_json["sclk"] = {
								{"value", sclk.val},
								{"unit", sclk.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : ""}
							};
						}
					}
					for (const auto& sclk_min : sclk_min_chiplet_per_partition) {
						if (sclk_min.res_group == AMDSMI_METRIC_RES_GROUP_AID && sclk_min.res_instance == aid_index) {
							aid_json["sclk_min"] = {
								{"value", sclk_min.val},
								{"unit", sclk_min.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : ""}
							};
						}
					}
					for (const auto& sclk_max : sclk_max_chiplet_per_partition) {
						if (sclk_max.res_group == AMDSMI_METRIC_RES_GROUP_AID && sclk_max.res_instance == aid_index) {
							aid_json["sclk_max"] = {
								{"value", sclk_max.val},
								{"unit", sclk_max.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : ""}
							};
						}
					}
					// VCN
					for (const auto& vcn : vcn_chiplet_per_partition) {
						if (vcn.res_group == AMDSMI_METRIC_RES_GROUP_AID &&
								vcn.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_ENGINE &&
								vcn.res_instance == aid_index) {
							aid_json["vcn_activity"] = {
								{"value", vcn.val},
								{"unit", vcn.unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : ""}
							};
						}
					}
					// JPEG (can be multiple)
					nlohmann::ordered_json jpeg_array = nlohmann::ordered_json::array();
					for (const auto& jpeg : jpeg_chiplet_per_partition) {
						if (jpeg.res_group == AMDSMI_METRIC_RES_GROUP_AID &&
								jpeg.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_ENGINE &&
								jpeg.res_instance == aid_index) {
							jpeg_array.push_back({
								{"value", jpeg.val},
								{"unit", jpeg.unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : ""}
							});
						}
					}
					if (!jpeg_array.empty()) {
						aid_json["jpeg_activity"] = jpeg_array;
					}
					// Temperature
					if (temp_aid_chiplet_per_partition.size() != 0) {
						for (const auto& temp : temp_aid_chiplet_per_partition) {
							if (temp.res_group == AMDSMI_METRIC_RES_GROUP_AID &&
								temp.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_NA &&
								temp.res_instance == aid_index) {
								aid_json["temperature"] = {
									{"value", temp.val},
									{"unit", temp.unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
								};
							}
						}
					} else {
						aid_json["temperature"] = {
							{"value", "N/A"},
							{"unit", "N/A"}
						};
					}
					// HBM Temp metrics (can be multiple)
					nlohmann::ordered_json hbm_temp_metrics_array = nlohmann::ordered_json::array();
					if (temp_hbm_metrics_chiplet_per_partition.size() != 0) {
						for (const auto& temp : temp_hbm_metrics_chiplet_per_partition) {
							if (temp.res_group == AMDSMI_METRIC_RES_GROUP_AID &&
								temp.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_HBM &&
								temp.res_instance == aid_index) {
								hbm_temp_metrics_array.push_back({
									{"value", temp.val},
									{"unit", temp.unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
								});
							}
						}
					} else {
						hbm_temp_metrics_array.push_back({
							{"value", "N/A"},
							{"unit", "N/A"}
						});
					}
					if (!hbm_temp_metrics_array.empty()) {
						aid_json["hbm_temperature"] = hbm_temp_metrics_array;
					}
					result_json[string_format("aid_%d", aid_index)] = aid_json;
				}
				// Iterate over XCPs assigned to this VF and AID
				uint32_t num_xcp = num_partitions;
				uint32_t num_vf = num_vf_enabled;
				uint32_t xcps_per_vf = num_xcp / num_vf;
				uint32_t xcp_start = vf_curr_index * xcps_per_vf;
				uint32_t xcp_end = xcp_start + xcps_per_vf;

				for (uint32_t xcp_id = xcp_start; xcp_id < xcp_end; xcp_id++) {
					nlohmann::ordered_json xcp_json;

					// GFX clocks
					for (const auto& gfx : gfx_chiplet_per_partition) {
						if (gfx.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && gfx.res_instance == xcp_id) {
							nlohmann::ordered_json gfx_json;
							gfx_json["value"] = gfx.val;
							gfx_json["unit"] = gfx.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							xcp_json["gfx"].push_back(gfx_json);
						}
					}
					for (const auto& gfx_min : gfx_min_chiplet_per_partition) {
						if (gfx_min.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx_min.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && gfx_min.res_instance == xcp_id) {
							nlohmann::ordered_json gfx_min_json;
							gfx_min_json["value"] = gfx_min.val;
							gfx_min_json["unit"] = gfx_min.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							xcp_json["gfx_min"].push_back(gfx_min_json);
						}
					}
					for (const auto& gfx_max : gfx_max_chiplet_per_partition) {
						if (gfx_max.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx_max.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && gfx_max.res_instance == xcp_id) {
							nlohmann::ordered_json gfx_max_json;
							gfx_max_json["value"] = gfx_max.val;
							gfx_max_json["unit"] = gfx_max.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							xcp_json["gfx_max"].push_back(gfx_max_json);
						}
					}
					for (const auto& gfx_locked : gfx_locked_chiplet_per_partition) {
						if (gfx_locked.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx_locked.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && gfx_locked.res_instance == xcp_id) {
							std::string locked_val = gfx_locked.val == UINT64_MAX ? "N/A" : (gfx_locked.val ? "ENABLED" :
													 "DISABLED");
							xcp_json["gfx_locked"].push_back(locked_val);
						}
					}
					for (const auto& gfx_usage : gfx_usage_chiplet_per_partition) {
						if (gfx_usage.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx_usage.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && gfx_usage.res_instance == xcp_id) {
							nlohmann::ordered_json gfx_usage_json;
							gfx_usage_json["value"] = gfx_usage.val;
							gfx_usage_json["unit"] = gfx_usage.unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : "";
							xcp_json["gfx_usage"].push_back(gfx_usage_json);
						}
					}
					if (temp_xcd_chiplet_per_partition.size() != 0) {
						for (const auto& temp : temp_xcd_chiplet_per_partition) {
							if (temp.res_group == AMDSMI_METRIC_RES_GROUP_XCP && temp.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && temp.res_instance == xcp_id) {
								nlohmann::ordered_json temp_json;
								temp_json["value"] = temp.val;
								temp_json["unit"] = temp.unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
								xcp_json["temperature"].push_back(temp_json);
							}
						}
					} else {
						nlohmann::ordered_json temp_json;
						temp_json["value"] = "N/A";
						temp_json["unit"] = "N/A";
						xcp_json["temperature"].push_back(temp_json);
					}
					std::vector<const amdsmi_metric_t*> m_ppt_a, m_ppt_b;
					std::vector<const amdsmi_metric_t*> m_thm_a, m_thm_b;
					std::vector<const amdsmi_metric_t*> m_total_a, m_total_b;
					std::vector<const amdsmi_metric_t*> m_util_a, m_util_b;
					for (const auto& m : throttle_ppt_a) { if (m.res_instance == xcp_id) m_ppt_a.push_back(&m); }
					for (const auto& m : throttle_ppt_b) { if (m.res_instance == xcp_id) m_ppt_b.push_back(&m); }
					for (const auto& m : throttle_thm_a) { if (m.res_instance == xcp_id) m_thm_a.push_back(&m); }
					for (const auto& m : throttle_thm_b) { if (m.res_instance == xcp_id) m_thm_b.push_back(&m); }
					for (const auto& m : throttle_total_a) { if (m.res_instance == xcp_id) m_total_a.push_back(&m); }
					for (const auto& m : throttle_total_b) { if (m.res_instance == xcp_id) m_total_b.push_back(&m); }
					for (const auto& m : throttle_util_a) { if (m.res_instance == xcp_id) m_util_a.push_back(&m); }
					for (const auto& m : throttle_util_b) { if (m.res_instance == xcp_id) m_util_b.push_back(&m); }
					amdsmi_metric_t dummy_metric{};
					dummy_metric.val = UINT64_MAX;
					if (m_ppt_a.empty()) { m_ppt_a.push_back(&dummy_metric); m_ppt_b.push_back(&dummy_metric); }
					if (m_thm_a.empty()) { m_thm_a.push_back(&dummy_metric); m_thm_b.push_back(&dummy_metric); }
					if (m_total_a.empty()) { m_total_a.push_back(&dummy_metric); m_total_b.push_back(&dummy_metric); }
					if (m_util_a.empty()) { m_util_a.push_back(&dummy_metric); m_util_b.push_back(&dummy_metric); }
					uint32_t num_violations = static_cast<uint32_t>(
						std::max({m_ppt_a.size(), m_thm_a.size(), m_total_a.size(), m_util_a.size()}));
					auto pad_to = [&dummy_metric](std::vector<const amdsmi_metric_t*> &v, uint32_t n) {
						while (v.size() < n) v.push_back(&dummy_metric);
					};
					pad_to(m_ppt_a, num_violations); pad_to(m_ppt_b, num_violations);
					pad_to(m_thm_a, num_violations); pad_to(m_thm_b, num_violations);
					pad_to(m_total_a, num_violations); pad_to(m_total_b, num_violations);
					pad_to(m_util_a, num_violations); pad_to(m_util_b, num_violations);

					auto to_json_acc = [](uint64_t v) -> nlohmann::ordered_json {
						if (v == UINT64_MAX) return "N/A";
						return v;
					};
					auto to_json_pct_obj = [](const std::string &s) -> nlohmann::ordered_json {
						nlohmann::ordered_json obj{};
						if (s == "N/A") {
							obj["value"] = "N/A";
							obj["unit"] = "N/A";
						} else {
							obj["value"] = std::stoull(s);
							obj["unit"] = "%";
						}
						return obj;
					};

					for (uint32_t i = 0; i < num_violations; i++) {
						std::string pwr_pct = violation_compute_pct(m_ppt_a[i]->val,
							m_ppt_b[i]->val, acc_counter_a, acc_counter_b, violation_ts_delta_us);
						xcp_json["gfx_clk_below_host_limit_power_violation_accumulated"].push_back(
							to_json_acc(m_ppt_b[i]->val));
						xcp_json["gfx_clk_below_host_limit_power_violation_activity"].push_back(to_json_pct_obj(pwr_pct));
						xcp_json["gfx_clk_below_host_limit_power_violation_status"].push_back(
							violation_is_active(m_ppt_a[i]->val, m_ppt_b[i]->val));

						std::string thm_pct = violation_compute_pct(m_thm_a[i]->val,
							m_thm_b[i]->val, acc_counter_a, acc_counter_b, violation_ts_delta_us);
						xcp_json["gfx_clk_below_host_limit_thermal_violation_accumulated"].push_back(
							to_json_acc(m_thm_b[i]->val));
						xcp_json["gfx_clk_below_host_limit_thermal_violation_activity"].push_back(to_json_pct_obj(thm_pct));
						xcp_json["gfx_clk_below_host_limit_thermal_violation_status"].push_back(
							violation_is_active(m_thm_a[i]->val, m_thm_b[i]->val));

						std::string total_pct = violation_compute_pct(m_total_a[i]->val,
							m_total_b[i]->val, acc_counter_a, acc_counter_b, violation_ts_delta_us);
						xcp_json["total_gfx_clk_below_host_limit_violation_accumulated"].push_back(
							to_json_acc(m_total_b[i]->val));
						xcp_json["total_gfx_clk_below_host_limit_violation_activity"].push_back(to_json_pct_obj(total_pct));
						xcp_json["total_gfx_clk_below_host_limit_violation_status"].push_back(
							violation_is_active(m_total_a[i]->val, m_total_b[i]->val));

						std::string low_pct = violation_compute_pct(m_util_a[i]->val,
							m_util_b[i]->val, acc_counter_a, acc_counter_b, violation_ts_delta_us);
						xcp_json["low_utilization_violation_accumulated"].push_back(
							to_json_acc(m_util_b[i]->val));
						xcp_json["low_utilization_violation_activity"].push_back(to_json_pct_obj(low_pct));
						xcp_json["low_utilization_violation_status"].push_back(
							violation_is_active(m_util_a[i]->val, m_util_b[i]->val));
					}
					if (!xcp_json.empty()) {
						result_json[string_format("xcp_%d", xcp_id)] = xcp_json;
					}
				}
			}
		}
		out = result_json.dump(4);
	} else {
		out = metricPerPartitionTemplate;

		for (uint32_t vf_curr_index = 0; vf_curr_index < num_vf_enabled; vf_curr_index++) {
			if (vf_curr_index == vf_index) {
				auto aids = get_aids_for_vf(vf_curr_index, num_vf_enabled);
				for (auto aid_index : aids) {
					std::string aid_index_string = string_format("%d", aid_index);
					out += string_format(AIDTemplate, aid_index_string.c_str());

					for (const auto& vclk : vclk_chiplet_per_partition) {
						if (vclk.res_group == AMDSMI_METRIC_RES_GROUP_AID && vclk.res_instance == aid_index) {
							std::string vclk_unit = vclk.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							std::string vclk_chiplet_val = string_format("%d", vclk.val);
							out += string_format(VCLKPerPartitionTemplate, vclk_chiplet_val.c_str(), vclk_unit.c_str());
						}
					}
					for (const auto& vclk_min : vclk_min_chiplet_per_partition) {
						if (vclk_min.res_group == AMDSMI_METRIC_RES_GROUP_AID && vclk_min.res_instance == aid_index) {
							std::string vclk_min_unit = vclk_min.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							std::string vclk_min_chiplet_val = string_format("%d", vclk_min.val);
							out += string_format(VCLKMinPerPartitionTemplate, vclk_min_chiplet_val.c_str(),
												 vclk_min_unit.c_str());
						}
					}
					for (const auto& vclk_max : vclk_max_chiplet_per_partition) {
						if (vclk_max.res_group == AMDSMI_METRIC_RES_GROUP_AID && vclk_max.res_instance == aid_index) {
							std::string vclk_max_unit = vclk_max.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							std::string vclk_max_chiplet_val = string_format("%d", vclk_max.val);
							out += string_format(VCLKMaxPerPartitionTemplate, vclk_max_chiplet_val.c_str(),
												 vclk_max_unit.c_str());
						}
					}
					for (const auto& dclk : dclk_chiplet_per_partition) {
						if (dclk.res_group == AMDSMI_METRIC_RES_GROUP_AID && dclk.res_instance == aid_index) {
							std::string dclk_unit = dclk.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							std::string dclk_chiplet_val = string_format("%d", dclk.val);
							out += string_format(DCLKPerPartitionTemplate, dclk_chiplet_val.c_str(), dclk_unit.c_str());
						}
					}
					for (const auto& dclk_min : dclk_min_chiplet_per_partition) {
						if (dclk_min.res_group == AMDSMI_METRIC_RES_GROUP_AID && dclk_min.res_instance == aid_index) {
							std::string dclk_min_unit = dclk_min.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							std::string dclk_min_chiplet_val = string_format("%d", dclk_min.val);
							out += string_format(DCLKMinPerPartitionTemplate, dclk_min_chiplet_val.c_str(),
												 dclk_min_unit.c_str());
						}
					}
					for (const auto& dclk_max : dclk_max_chiplet_per_partition) {
						if (dclk_max.res_group == AMDSMI_METRIC_RES_GROUP_AID && dclk_max.res_instance == aid_index) {
							std::string dclk_max_unit = dclk_max.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							std::string dclk_max_chiplet_val = string_format("%d", dclk_max.val);
							out += string_format(DCLKMaxPerPartitionTemplate, dclk_max_chiplet_val.c_str(),
												 dclk_max_unit.c_str());
						}
					}
					for (const auto& sclk : sclk_chiplet_per_partition) {
						if (sclk.res_group == AMDSMI_METRIC_RES_GROUP_AID && sclk.res_instance == aid_index) {
							std::string sclk_unit = sclk.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							std::string sclk_chiplet_val = string_format("%d", sclk.val);
							out += string_format(SCLKPerPartitionTemplate, sclk_chiplet_val.c_str(), sclk_unit.c_str());
						}
					}
					for (const auto& sclk_min : sclk_min_chiplet_per_partition) {
						if (sclk_min.res_group == AMDSMI_METRIC_RES_GROUP_AID && sclk_min.res_instance == aid_index) {
							std::string sclk_min_unit = sclk_min.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							std::string sclk_min_chiplet_val = string_format("%d", sclk_min.val);
							out += string_format(SCLKMinPerPartitionTemplate, sclk_min_chiplet_val.c_str(),
												 sclk_min_unit.c_str());
						}
					}
					for (const auto& sclk_max : sclk_max_chiplet_per_partition) {
						if (sclk_max.res_group == AMDSMI_METRIC_RES_GROUP_AID && sclk_max.res_instance == aid_index) {
							std::string sclk_max_unit = sclk_max.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
							std::string sclk_max_chiplet_val = string_format("%d", sclk_max.val);
							out += string_format(SCLKMaxPerPartitionTemplate, sclk_max_chiplet_val.c_str(),
												 sclk_max_unit.c_str());
						}
					}

					for (const auto& vcn : vcn_chiplet_per_partition) {
						if (vcn.res_group == AMDSMI_METRIC_RES_GROUP_AID &&
								vcn.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_ENGINE &&
								vcn.res_instance == aid_index) {
							std::string vcn_unit = vcn.unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : "";
							std::string vcn_chiplet_val = string_format("%d", vcn.val);
							out += string_format(activityPerPartitionTemplate, vcn_chiplet_val.c_str(), vcn_unit.c_str());
						}
					}

					out += metricJpegUsagePerPartitionHeaderTemplate;

					// Collect all matching JPEG metrics for this aid_index
					std::vector<const amdsmi_metric_t*> matching_jpegs;
					for (const auto& jpeg : jpeg_chiplet_per_partition) {
						if (jpeg.res_group == AMDSMI_METRIC_RES_GROUP_AID &&
								jpeg.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_ENGINE &&
								jpeg.res_instance == aid_index) {
							matching_jpegs.push_back(&jpeg);
						}
					}
					for (size_t i = 0; i < matching_jpegs.size(); i++) {
						const auto& jpeg = *matching_jpegs[i];
						std::string jpeg_unit = jpeg.unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : "";
						std::string jpeg_chiplet_val = string_format("%d", jpeg.val);
						out += string_format(metricJpegUsagePerPartitionTemplate, jpeg_chiplet_val.c_str(),
											 jpeg_unit.c_str());
						if (i + 1 < matching_jpegs.size()) {
							out += commaTemplate;
						}
					}
					out += metricJpegUsageFooterTemplate;

					if (temp_aid_chiplet_per_partition.size() != 0 ) {
						for (const auto& temp_aid : temp_aid_chiplet_per_partition) {
							if (temp_aid.res_group == AMDSMI_METRIC_RES_GROUP_AID &&
								temp_aid.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_NA &&
								temp_aid.res_instance == aid_index) {
								std::string temp_aid_unit = temp_aid.unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
								std::string temp_aid_chiplet_val = string_format("%d", temp_aid.val);
								out += string_format(TemperaturePerPartitionTemplate, temp_aid_chiplet_val.c_str(), temp_aid_unit.c_str());
							}
						}
					} else {
						out += "                TEMPERATURE: N/A\n";
					}

					out += metricHbmTempPerPartitionHeaderTemplate;

					// Collect all matching HBM_TEMP metrics for this aid_index
					std::vector<const amdsmi_metric_t*> matching_hbm_temp_metrics;
					for (const auto& hbm_temp_metrics : temp_hbm_metrics_chiplet_per_partition) {
						if (hbm_temp_metrics.res_group == AMDSMI_METRIC_RES_GROUP_AID &&
							hbm_temp_metrics.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_HBM &&
							hbm_temp_metrics.res_instance == aid_index) {
							matching_hbm_temp_metrics.push_back(&hbm_temp_metrics);
						}
					}
					if (matching_hbm_temp_metrics.size() != 0) {
						for (size_t i = 0; i < matching_hbm_temp_metrics.size(); i++) {
							const auto& hbm_temp_metrics = *matching_hbm_temp_metrics[i];
							std::string hbm_temp_aid_unit = hbm_temp_metrics.unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
							std::string hbm_temp_aid_chiplet_val = string_format("%d", hbm_temp_metrics.val);
							out += string_format(metricHbmTempPerPartitionTemplate, hbm_temp_aid_chiplet_val.c_str(), hbm_temp_aid_unit.c_str());
							if (i + 1 < matching_hbm_temp_metrics.size()) {
								out += commaTemplate;
							}
						}
					} else {
						out += "N/A";
					}
					out += metricJpegUsageFooterTemplate;
				}

				// Calculate XCPs assigned to this VF
				uint32_t num_xcp = num_partitions;
				uint32_t num_vf = num_vf_enabled;
				uint32_t xcps_per_vf = num_xcp / num_vf;
				uint32_t xcp_start = vf_curr_index * xcps_per_vf;
				uint32_t xcp_end = xcp_start + xcps_per_vf;

				for (uint32_t xcp_id = xcp_start; xcp_id < xcp_end; xcp_id++) {
					std::string xcp_index_string = string_format("%d", xcp_id);
					out += string_format(XCPTemplate, xcp_index_string.c_str());

					out += metricGFXCLKPerPartitionHeaderTemplate;

					// GFX (can be multiple per XCP)
					std::vector<const amdsmi_metric_t*> matching_gfx;
					for (const auto& gfx : gfx_chiplet_per_partition) {
						if (gfx.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && gfx.res_instance == xcp_id) {
							matching_gfx.push_back(&gfx);
						}
					}
					for (size_t i = 0; i < matching_gfx.size(); i++) {
						const auto& gfx = *matching_gfx[i];
						std::string gfx_unit = gfx.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
						std::string gfx_chiplet_val = string_format("%d", gfx.val);
						out += string_format(GFXPerPartitionTemplate, gfx_chiplet_val.c_str(), gfx_unit.c_str());
						if (i + 1 < matching_gfx.size()) {
							out += commaTemplate;
						}
					}

					out += metricJpegUsageFooterTemplate;
					out += metricGFXMinCLKPerPartitionHeaderTemplate;

					// GFX_MIN (can be multiple per XCP)
					std::vector<const amdsmi_metric_t*> matching_gfx_min;
					for (const auto& gfx_min : gfx_min_chiplet_per_partition) {
						if (gfx_min.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx_min.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && gfx_min.res_instance == xcp_id) {
							matching_gfx_min.push_back(&gfx_min);
						}
					}
					for (size_t i = 0; i < matching_gfx_min.size(); i++) {
						const auto& gfx_min = *matching_gfx_min[i];
						std::string gfx_min_unit = gfx_min.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
						std::string gfx_min_chiplet_val = string_format("%d", gfx_min.val);
						out += string_format(GFXMinPerPartitionTemplate, gfx_min_chiplet_val.c_str(), gfx_min_unit.c_str());
						if (i + 1 < matching_gfx_min.size()) {
							out += commaTemplate;
						}
					}
					out += metricJpegUsageFooterTemplate;
					out += metricGFXMaxCLKPerPartitionHeaderTemplate;

					// GFX_MAX (can be multiple per XCP)
					std::vector<const amdsmi_metric_t*> matching_gfx_max;
					for (const auto& gfx_max : gfx_max_chiplet_per_partition) {
						if (gfx_max.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx_max.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC &&  gfx_max.res_instance == xcp_id) {
							matching_gfx_max.push_back(&gfx_max);
						}
					}
					for (size_t i = 0; i < matching_gfx_max.size(); i++) {
						const auto& gfx_max = *matching_gfx_max[i];
						std::string gfx_max_unit = gfx_max.unit == AMDSMI_METRIC_UNIT_MHZ ? "MHz" : "";
						std::string gfx_max_chiplet_val = string_format("%d", gfx_max.val);
						out += string_format(GFXMaxPerPartitionTemplate, gfx_max_chiplet_val.c_str(), gfx_max_unit.c_str());
						if (i + 1 < matching_gfx_max.size()) {
							out += commaTemplate;
						}
					}
					out += metricJpegUsageFooterTemplate;
					out += metricGFXLockedCLKPerPartitionHeaderTemplate;

					// GFX_LOCKED (can be multiple per XCP)
					std::vector<const amdsmi_metric_t*> matching_gfx_locked;
					for (const auto& gfx_locked : gfx_locked_chiplet_per_partition) {
						if (gfx_locked.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx_locked.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && gfx_locked.res_instance == xcp_id) {
							matching_gfx_locked.push_back(&gfx_locked);
						}
					}
					for (size_t i = 0; i < matching_gfx_locked.size(); i++) {
						const auto& gfx_locked = *matching_gfx_locked[i];
						std::string gfx_locked_val = gfx_locked.val == UINT64_MAX ? "N/A" : (gfx_locked.val ? "ENABLED" :
													 "DISABLED");
						out += string_format(GFXLockedPerPartitionTemplate, gfx_locked_val.c_str());
						if (i + 1 < matching_gfx_locked.size()) {
							out += commaTemplate;
						}
					}

					out += metricJpegUsageFooterTemplate;
					out += metricGFXUsagePerPartitionHeaderTemplate;

					// GFX Usage (can be multiple per XCP)
					std::vector<const amdsmi_metric_t*> matching_gfx_usage;
					for (const auto& gfx_usage : gfx_usage_chiplet_per_partition) {
						if (gfx_usage.res_group == AMDSMI_METRIC_RES_GROUP_XCP
								&& gfx_usage.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && gfx_usage.res_instance == xcp_id) {
							matching_gfx_usage.push_back(&gfx_usage);
						}
					}
					for (size_t i = 0; i < matching_gfx_usage.size(); i++) {
						const auto& gfx_usage = *matching_gfx_usage[i];
						std::string gfx_usage_unit = gfx_usage.unit == AMDSMI_METRIC_UNIT_PERCENT ? "%" : "";
						std::string gfx_usage_val = string_format("%d", gfx_usage.val);
						out += string_format(GFXUsagePerPartitionTemplate, gfx_usage_val.c_str(), gfx_usage_unit.c_str());
						if (i + 1 < matching_gfx_usage.size()) {
							out += commaTemplate;
						}
					}
					out += metricJpegUsageFooterTemplate;
					out += metricTempPerPartitionHeaderTemplate;

					// temperature (can be multiple per XCP)
					std::vector<const amdsmi_metric_t*> matching_temp_xcd;
					for (const auto& temp : temp_xcd_chiplet_per_partition) {
						if (temp.res_group == AMDSMI_METRIC_RES_GROUP_XCP && temp.res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_XCC && temp.res_instance == xcp_id) {
							matching_temp_xcd.push_back(&temp);
						}
					}
					if (matching_temp_xcd.size() != 0) {
						for (size_t i = 0; i < matching_temp_xcd.size(); i++) {
							const auto& temp = *matching_temp_xcd[i];
							std::string temp_unit = temp.unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
							std::string temp_val = string_format("%d", temp.val);
							out += string_format(TempXcdPerPartitionTemplate, temp_val.c_str(), temp_unit.c_str());
							if (i + 1 < matching_temp_xcd.size()) {
								out += commaTemplate;
							}
						}
					} else {
						out += "N/A";
					}
					out += metricJpegUsageFooterTemplate;

					std::vector<const amdsmi_metric_t*> m_ppt_a, m_ppt_b;
					std::vector<const amdsmi_metric_t*> m_thm_a, m_thm_b;
					std::vector<const amdsmi_metric_t*> m_total_a, m_total_b;
					std::vector<const amdsmi_metric_t*> m_util_a, m_util_b;
					for (const auto& m : throttle_ppt_a) { if (m.res_instance == xcp_id) m_ppt_a.push_back(&m); }
					for (const auto& m : throttle_ppt_b) { if (m.res_instance == xcp_id) m_ppt_b.push_back(&m); }
					for (const auto& m : throttle_thm_a) { if (m.res_instance == xcp_id) m_thm_a.push_back(&m); }
					for (const auto& m : throttle_thm_b) { if (m.res_instance == xcp_id) m_thm_b.push_back(&m); }
					for (const auto& m : throttle_total_a) { if (m.res_instance == xcp_id) m_total_a.push_back(&m); }
					for (const auto& m : throttle_total_b) { if (m.res_instance == xcp_id) m_total_b.push_back(&m); }
					for (const auto& m : throttle_util_a) { if (m.res_instance == xcp_id) m_util_a.push_back(&m); }
					for (const auto& m : throttle_util_b) { if (m.res_instance == xcp_id) m_util_b.push_back(&m); }
					amdsmi_metric_t dummy_metric{};
					dummy_metric.val = UINT64_MAX;
					if (m_ppt_a.empty()) { m_ppt_a.push_back(&dummy_metric); m_ppt_b.push_back(&dummy_metric); }
					if (m_thm_a.empty()) { m_thm_a.push_back(&dummy_metric); m_thm_b.push_back(&dummy_metric); }
					if (m_total_a.empty()) { m_total_a.push_back(&dummy_metric); m_total_b.push_back(&dummy_metric); }
					if (m_util_a.empty()) { m_util_a.push_back(&dummy_metric); m_util_b.push_back(&dummy_metric); }
					uint32_t num_violations = static_cast<uint32_t>(
						std::max({m_ppt_a.size(), m_thm_a.size(), m_total_a.size(), m_util_a.size()}));
					auto pad_to = [&dummy_metric](std::vector<const amdsmi_metric_t*> &v, uint32_t n) {
						while (v.size() < n) v.push_back(&dummy_metric);
					};
					pad_to(m_ppt_a, num_violations); pad_to(m_ppt_b, num_violations);
					pad_to(m_thm_a, num_violations); pad_to(m_thm_b, num_violations);
					pad_to(m_total_a, num_violations); pad_to(m_total_b, num_violations);
					pad_to(m_util_a, num_violations); pad_to(m_util_b, num_violations);
					std::string val;

					std::string pwr_acc_str = metricGfxClkBelowHostLimitPowerAccPerPartitionHeaderTemplate;
					std::string thm_acc_str = metricGfxClkBelowHostLimitThermalAccPerPartitionHeaderTemplate;
					std::string total_acc_str = metricTotalGfxClkBelowHostLimitAccPerPartitionHeaderTemplate;
					std::string util_acc_str = metricLowUtilizationAccPerPartitionHeaderTemplate;

					std::string pwr_status_str = metricGfxClkBelowHostLimitPowerStatusPerPartitionHeaderTemplate;
					std::string thm_status_str = metricGfxClkBelowHostLimitThermalStatusPerPartitionHeaderTemplate;
					std::string total_status_str = metricTotalGfxClkBelowHostLimitStatusPerPartitionHeaderTemplate;
					std::string util_status_str = metricLowUtilizationStatusPerPartitionHeaderTemplate;

					std::string pwr_activity_str = metricGfxClkBelowHostLimitPowerActivityPerPartitionHeaderTemplate;
					std::string thm_activity_str = metricGfxClkBelowHostLimitThermalActivityPerPartitionHeaderTemplate;
					std::string total_activity_str = metricTotalGfxClkBelowHostLimitActivityPerPartitionHeaderTemplate;
					std::string util_activity_str = metricLowUtilizationActivityPerPartitionHeaderTemplate;

					for (uint32_t i = 0; i < num_violations; i++) {
						val = (m_ppt_b[i]->val == UINT64_MAX) ? "N/A" :
							string_format("%" PRIu64, m_ppt_b[i]->val);
						std::string pwr_pct = violation_compute_pct(m_ppt_a[i]->val,
							m_ppt_b[i]->val, acc_counter_a, acc_counter_b, violation_ts_delta_us);
						std::string pwr_active = violation_is_active(m_ppt_a[i]->val, m_ppt_b[i]->val);
						pwr_acc_str += string_format(AccXcdPerPartitionTemplate, val.c_str());
						pwr_status_str += (pwr_active == "N/A") ? "N/A" : ((pwr_active == "TRUE") ? "ACTIVE" : "INACTIVE");
						pwr_activity_str += (pwr_pct == "N/A") ? "N/A" : string_format("%s %%", pwr_pct.c_str());

						val = (m_thm_b[i]->val == UINT64_MAX) ? "N/A" :
							string_format("%" PRIu64, m_thm_b[i]->val);
						std::string thm_pct = violation_compute_pct(m_thm_a[i]->val,
							m_thm_b[i]->val, acc_counter_a, acc_counter_b, violation_ts_delta_us);
						std::string thm_active = violation_is_active(m_thm_a[i]->val, m_thm_b[i]->val);
						thm_acc_str += string_format(AccXcdPerPartitionTemplate, val.c_str());
						thm_status_str += (thm_active == "N/A") ? "N/A" : ((thm_active == "TRUE") ? "ACTIVE" : "INACTIVE");
						thm_activity_str += (thm_pct == "N/A") ? "N/A" : string_format("%s %%", thm_pct.c_str());

						val = (m_total_b[i]->val == UINT64_MAX) ? "N/A" :
							string_format("%" PRIu64, m_total_b[i]->val);
						std::string total_pct = violation_compute_pct(m_total_a[i]->val,
							m_total_b[i]->val, acc_counter_a, acc_counter_b, violation_ts_delta_us);
						std::string total_active = violation_is_active(m_total_a[i]->val, m_total_b[i]->val);
						total_acc_str += string_format(AccXcdPerPartitionTemplate, val.c_str());
						total_status_str += (total_active == "N/A") ? "N/A" : ((total_active == "TRUE") ? "ACTIVE" : "INACTIVE");
						total_activity_str += (total_pct == "N/A") ? "N/A" : string_format("%s %%", total_pct.c_str());

						val = (m_util_b[i]->val == UINT64_MAX) ? "N/A" :
							string_format("%" PRIu64, m_util_b[i]->val);
						std::string util_pct = violation_compute_pct(m_util_a[i]->val,
							m_util_b[i]->val, acc_counter_a, acc_counter_b, violation_ts_delta_us);
						std::string util_active = violation_is_active(m_util_a[i]->val, m_util_b[i]->val);
						util_acc_str += string_format(AccXcdPerPartitionTemplate, val.c_str());
						util_status_str += (util_active == "N/A") ? "N/A" : ((util_active == "TRUE") ? "ACTIVE" : "INACTIVE");
						util_activity_str += (util_pct == "N/A") ? "N/A" : string_format("%s %%", util_pct.c_str());

						if (i + 1 < num_violations) {
							pwr_acc_str += commaTemplate;
							thm_acc_str += commaTemplate;
							total_acc_str += commaTemplate;
							util_acc_str += commaTemplate;
							pwr_status_str += commaTemplate;
							thm_status_str += commaTemplate;
							total_status_str += commaTemplate;
							util_status_str += commaTemplate;
							pwr_activity_str += commaTemplate;
							thm_activity_str += commaTemplate;
							total_activity_str += commaTemplate;
							util_activity_str += commaTemplate;
						}
					}
					pwr_acc_str += metricJpegUsageFooterTemplate;
					thm_acc_str += metricJpegUsageFooterTemplate;
					total_acc_str += metricJpegUsageFooterTemplate;
					util_acc_str += metricJpegUsageFooterTemplate;
					pwr_status_str += metricJpegUsageFooterTemplate;
					thm_status_str += metricJpegUsageFooterTemplate;
					total_status_str += metricJpegUsageFooterTemplate;
					util_status_str += metricJpegUsageFooterTemplate;
					pwr_activity_str += metricJpegUsageFooterTemplate;
					thm_activity_str += metricJpegUsageFooterTemplate;
					total_activity_str += metricJpegUsageFooterTemplate;
					util_activity_str += metricJpegUsageFooterTemplate;

					out += pwr_acc_str;
					out += pwr_activity_str;
					out += pwr_status_str;
					out += thm_acc_str;
					out += thm_activity_str;
					out += thm_status_str;
					out += total_acc_str;
					out += total_activity_str;
					out += total_status_str;
					out += util_acc_str;
					out += util_activity_str;
					out += util_status_str;
				}
			}
		}
	}
	return ret;
}

int AmdSmiApiHost::amdsmi_get_guest_data_metric_command(std::string device, Arguments arg,
		std::string& out)
{
	amdsmi_bdf_t vf_bdf;
	amdsmi_vf_handle_t vf_handle;
	amdsmi_status_t ret;
	amdsmi_guest_data_t guest_data;

	vf_bdf.bdf.domain_number = std::stoi(device.substr(0, 4), nullptr, 16);
	vf_bdf.bdf.bus_number = std::stoi(device.substr(5, 2), nullptr, 16);
	vf_bdf.bdf.device_number = std::stoi(device.substr(8, 2), nullptr, 16);
	vf_bdf.bdf.function_number = std::stoi(device.substr(11), nullptr, 16);

	ret = host_amdsmi_get_vf_handle_from_bdf(vf_bdf, &vf_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_guest_data(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_guest_data(vf_handle, &guest_data);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_guest_data(arg, "N/A");
		return ret;
	}

	std::string driver_version_str = reinterpret_cast<char *>(guest_data.driver_version);

	// Check for non-printable characters
	if (std::any_of(driver_version_str.begin(), driver_version_str.end(), [](char c) {
	return !std::isprint(c);
	})) {
		driver_version_str = "N/A";
	}

	if (driver_version_str == "") {
		driver_version_str = "N/A";
	}
	uint32_t fb_usage = guest_data.fb_usage;
	std::string fb_usage_str =
		string_format("%u", fb_usage);

	if (arg.watch > -1) {
		out = string_format("%s,%s", driver_version_str.c_str(), fb_usage_str.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json fb_usage_json{};
		fb_usage_json["value"] = fb_usage;
		fb_usage_json["unit"] = fb_usage_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json guest_json_out = {
			{ "driver_version", driver_version_str.c_str() },
			{ "fb_usage", fb_usage_json }
		};
		out = guest_json_out.dump(4);
	} else if (arg.output == csv) {
		bool is_vf_schedule = ((std::find(arg.options.begin(), arg.options.end(),
										  "schedule") != arg.options.end()) ||
							   (std::find(arg.options.begin(), arg.options.end(), "s") != arg.options.end()));
		bool is_vf_guard_info = ((std::find(arg.options.begin(), arg.options.end(),
											"guard") != arg.options.end()) ||
								 (std::find(arg.options.begin(), arg.options.end(), "G") != arg.options.end()));
		if(!is_vf_schedule && !is_vf_guard_info && !arg.all_arguments) {
			std::tuple<std::string, std::string, std::string> indexes =
				getGpuVfIndexFromVfId(device);
			out.append(std::get<0>(indexes).c_str()).append(",").append(std::get<1>
					(indexes).c_str());
			out += string_format(
					   ",%s,%s", driver_version_str.c_str(), fb_usage_str.c_str());
		} else if (is_vf_guard_info || arg.all_arguments) {
			std::string newString = "";
			size_t pos = 0;
			while (pos < out.length() - 1) {
				size_t newlinePos = out.find('\n', pos);
				if (newlinePos == std::string::npos) {
					newString += out.substr(pos) + string_format(
									 ",%s,%s\n", driver_version_str.c_str(), fb_usage_str.c_str());
					break;
				}
				newString += out.substr(pos, newlinePos - pos) + string_format(
								 ",%s,%s\n", driver_version_str.c_str(), fb_usage_str.c_str());
				pos = newlinePos + 1;
			}
			out = newString;
		} else {
			out = string_format(
					  ",%s,%s", driver_version_str.c_str(), fb_usage_str.c_str());
		}
	} else {
		std::string fb_usage_str_unit = fb_usage_str == "N/A" ? "" : "MB";

		out = string_format(
				  metricGuestDataTemplate, driver_version_str.c_str(), fb_usage_str.c_str(),
				  fb_usage_str_unit.c_str());
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_energy_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_energy(arg, "N/A");
		return ret;
	}

	amdsmi_metric_t *metrics;
	uint32_t metric_size = AMDSMI_MAX_NUM_METRICS;
	uint64_t energy = 0;

	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_energy(arg, "N/A");
		return ret;
	}

	metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t)*metric_size);
	if (metrics == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, &metrics[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(metrics);
		out = host_fill_energy(arg, "N/A");
		return ret;
	}

	if (ret == AMDSMI_STATUS_SUCCESS) {
		for (int i = 0; i < metric_size; i++) {
			if ((metrics[i].name == AMDSMI_METRIC_NAME_ENERGY_SOCKET)
					&& (metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
				energy = metrics[i].val;
			}
		}
	}

	std::string energy_str{ string_format(
								"%lu", energy) };

	if (arg.watch > -1) {
		out = string_format("%s", energy_str.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json energy_json{};
		energy_json["value"] = energy;
		energy_json["unit"] = energy_str == "N/A" ? "N/A" : "J";
		nlohmann::ordered_json energy_info_json = { { "total_energy_consumption", energy_json} };
		out = energy_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s", energy_str.c_str());
	} else {
		std::string energy_unit = energy_str == "N/A" ? "" : "J";
		out = string_format(
				  metricPowerEnergyTemplate, energy_str.c_str(), energy_unit.c_str());
	}

	free(metrics);
	return AMDSMI_STATUS_SUCCESS;
}



int AmdSmiApiHost::amdsmi_get_gpuboard_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_metric_t *metrics;
	uint32_t metric_size = AMDSMI_MAX_NUM_METRICS;
	bool is_supported = false;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	std::vector<amdsmi_metric_t> node_temp_retimer{};
	std::vector<amdsmi_metric_t> node_temp_ibc_temp{};
	std::vector<amdsmi_metric_t> node_temp_ibc_2_temp{};
	std::vector<amdsmi_metric_t> node_temp_vdd18_vr_temp{};
	std::vector<amdsmi_metric_t> node_temp_04_hbm_b_vr_temp{};
	std::vector<amdsmi_metric_t> node_temp_04_hbm_d_vr_temp{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_vdd0{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_vdd1{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_vdd2{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_vdd3{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_soc_a{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_soc_c{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_socio_a{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_socio_c{};
	std::vector<amdsmi_metric_t> vr_temp_vdd_085_hbm{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_11_hbm_b{};
	std::vector<amdsmi_metric_t> vr_temp_vddcr_11_hbm_d{};
	std::vector<amdsmi_metric_t> vr_temp_vdd_usr{};
	std::vector<amdsmi_metric_t> vr_temp_vddio_11_e32{};

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}
	metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t)*metric_size);
	if (metrics == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, &metrics[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	for (uint32_t i = 0; i < metric_size; i++) {
		if (!(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC) &&
			(metrics[i].flags & AMDSMI_METRIC_TYPE_INST)) {
			switch (metrics[i].name) {
				case AMDSMI_METRIC_NAME_NODE_TEMP_RETIMER:
					node_temp_retimer.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_NODE_TEMP_IBC_TEMP:
					node_temp_ibc_temp.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_NODE_TEMP_IBC_2_TEMP:
					node_temp_ibc_2_temp.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_NODE_TEMP_VDD18_VR_TEMP:
					node_temp_vdd18_vr_temp.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_B_VR_TEMP:
					node_temp_04_hbm_b_vr_temp.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_D_VR_TEMP:
					node_temp_04_hbm_d_vr_temp.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD0:
					vr_temp_vddcr_vdd0.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD1:
					vr_temp_vddcr_vdd1.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD2:
					vr_temp_vddcr_vdd2.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD3:
					vr_temp_vddcr_vdd3.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_A:
					vr_temp_vddcr_soc_a.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_C:
					vr_temp_vddcr_soc_c.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_A:
					vr_temp_vddcr_socio_a.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_C:
					vr_temp_vddcr_socio_c.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDD_085_HBM:
					vr_temp_vdd_085_hbm.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_B:
					vr_temp_vddcr_11_hbm_b.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_D:
					vr_temp_vddcr_11_hbm_d.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDD_USR:
					vr_temp_vdd_usr.push_back(metrics[i]);
					break;
				case AMDSMI_METRIC_NAME_VR_TEMP_VDDIO_11_E32:
					vr_temp_vddio_11_e32.push_back(metrics[i]);
					break;
				default:
					break;
			}
		}
	}

	free(metrics);
	if (arg.watch > -1) {
	} else if (arg.output == json) {
		nlohmann::ordered_json gpuboard_json;

		if (node_temp_retimer.size() == 0) {
			gpuboard_json["node_temp_retimer"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < node_temp_retimer.size(); i++) {
				if (node_temp_retimer[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_retimer[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["node_temp_retimer"] = {
						{"value", node_temp_retimer[i].val},
						{"unit", node_temp_retimer[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (node_temp_ibc_temp.size() == 0) {
			gpuboard_json["node_temp_ibc_temp"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < node_temp_ibc_temp.size(); i++) {
				if (node_temp_ibc_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_ibc_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["node_temp_ibc_temp"] = {
						{"value", node_temp_ibc_temp[i].val},
						{"unit", node_temp_ibc_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (node_temp_ibc_2_temp.size() == 0) {
			gpuboard_json["node_temp_ibc_2_temp"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < node_temp_ibc_2_temp.size(); i++) {
				if (node_temp_ibc_2_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_ibc_2_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["node_temp_ibc_2_temp"] = {
						{"value", node_temp_ibc_2_temp[i].val},
						{"unit", node_temp_ibc_2_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (node_temp_vdd18_vr_temp.size() == 0) {
			gpuboard_json["node_temp_vdd18_vr_temp"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < node_temp_vdd18_vr_temp.size(); i++) {
				if (node_temp_vdd18_vr_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_vdd18_vr_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["node_temp_vdd18_vr_temp"] = {
						{"value", node_temp_vdd18_vr_temp[i].val},
						{"unit", node_temp_vdd18_vr_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (node_temp_04_hbm_b_vr_temp.size() == 0) {
			gpuboard_json["node_temp_04_hbm_b_vr_temp"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < node_temp_04_hbm_b_vr_temp.size(); i++) {
				if (node_temp_04_hbm_b_vr_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_04_hbm_b_vr_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["node_temp_04_hbm_b_vr_temp"] = {
						{"value", node_temp_04_hbm_b_vr_temp[i].val},
						{"unit", node_temp_04_hbm_b_vr_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (node_temp_04_hbm_d_vr_temp.size() == 0) {
			gpuboard_json["node_temp_04_hbm_d_vr_temp"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < node_temp_04_hbm_d_vr_temp.size(); i++) {
				if (node_temp_04_hbm_d_vr_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_04_hbm_d_vr_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["node_temp_04_hbm_d_vr_temp"] = {
						{"value", node_temp_04_hbm_d_vr_temp[i].val},
						{"unit", node_temp_04_hbm_d_vr_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_vdd0.size() == 0) {
			gpuboard_json["vr_temp_vddcr_vdd0"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd0.size(); i++) {
				if (vr_temp_vddcr_vdd0[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd0[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_vdd0"] = {
						{"value", vr_temp_vddcr_vdd0[i].val},
						{"unit", vr_temp_vddcr_vdd0[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_vdd1.size() == 0) {
			gpuboard_json["vr_temp_vddcr_vdd1"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd1.size(); i++) {
				if (vr_temp_vddcr_vdd1[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd1[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_vdd1"] = {
						{"value", vr_temp_vddcr_vdd1[i].val},
						{"unit", vr_temp_vddcr_vdd1[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_vdd2.size() == 0) {
			gpuboard_json["vr_temp_vddcr_vdd2"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd2.size(); i++) {
				if (vr_temp_vddcr_vdd2[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd2[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_vdd2"] = {
						{"value", vr_temp_vddcr_vdd2[i].val},
						{"unit", vr_temp_vddcr_vdd2[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_vdd3.size() == 0) {
			gpuboard_json["vr_temp_vddcr_vdd3"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd3.size(); i++) {
				if (vr_temp_vddcr_vdd3[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd3[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_vdd3"] = {
						{"value", vr_temp_vddcr_vdd3[i].val},
						{"unit", vr_temp_vddcr_vdd3[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_soc_a.size() == 0) {
			gpuboard_json["vr_temp_vddcr_soc_a"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_soc_a.size(); i++) {
				if (vr_temp_vddcr_soc_a[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_soc_a[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_soc_a"] = {
						{"value", vr_temp_vddcr_soc_a[i].val},
						{"unit", vr_temp_vddcr_soc_a[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_soc_c.size() == 0) {
			gpuboard_json["vr_temp_vddcr_soc_c"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_soc_c.size(); i++) {
				if (vr_temp_vddcr_soc_c[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_soc_c[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_soc_c"] = {
						{"value", vr_temp_vddcr_soc_c[i].val},
						{"unit", vr_temp_vddcr_soc_c[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_socio_a.size() == 0) {
			gpuboard_json["vr_temp_vddcr_socio_a"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_socio_a.size(); i++) {
				if (vr_temp_vddcr_socio_a[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_socio_a[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_socio_a"] = {
						{"value", vr_temp_vddcr_socio_a[i].val},
						{"unit", vr_temp_vddcr_socio_a[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_socio_c.size() == 0) {
			gpuboard_json["vr_temp_vddcr_socio_c"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_socio_c.size(); i++) {
				if (vr_temp_vddcr_socio_c[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_socio_c[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_socio_c"] = {
						{"value", vr_temp_vddcr_socio_c[i].val},
						{"unit", vr_temp_vddcr_socio_c[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vdd_085_hbm.size() == 0) {
			gpuboard_json["vr_temp_vdd_085_hbm"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vdd_085_hbm.size(); i++) {
				if (vr_temp_vdd_085_hbm[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vdd_085_hbm[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vdd_085_hbm"] = {
						{"value", vr_temp_vdd_085_hbm[i].val},
						{"unit", vr_temp_vdd_085_hbm[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_11_hbm_b.size() == 0) {
			gpuboard_json["vr_temp_vddcr_11_hbm_b"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_11_hbm_b.size(); i++) {
				if (vr_temp_vddcr_11_hbm_b[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_11_hbm_b[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_11_hbm_b"] = {
						{"value", vr_temp_vddcr_11_hbm_b[i].val},
						{"unit", vr_temp_vddcr_11_hbm_b[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddcr_11_hbm_d.size() == 0) {
			gpuboard_json["vr_temp_vddcr_11_hbm_d"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_11_hbm_d.size(); i++) {
				if (vr_temp_vddcr_11_hbm_d[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_11_hbm_d[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddcr_11_hbm_d"] = {
						{"value", vr_temp_vddcr_11_hbm_d[i].val},
						{"unit", vr_temp_vddcr_11_hbm_d[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vdd_usr.size() == 0) {
			gpuboard_json["vr_temp_vdd_usr"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vdd_usr.size(); i++) {
				if (vr_temp_vdd_usr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vdd_usr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vdd_usr"] = {
						{"value", vr_temp_vdd_usr[i].val},
						{"unit", vr_temp_vdd_usr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		if (vr_temp_vddio_11_e32.size() == 0) {
			gpuboard_json["vr_temp_vddio_11_e32"] = {
				{"value", "N/A"},
				{"unit", "N/A"}
			};
		} else {
			for (uint32_t i = 0; i < vr_temp_vddio_11_e32.size(); i++) {
				if (vr_temp_vddio_11_e32[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddio_11_e32[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					gpuboard_json["vr_temp_vddio_11_e32"] = {
						{"value", vr_temp_vddio_11_e32[i].val},
						{"unit", vr_temp_vddio_11_e32[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : ""}
					};
				}
			}
		}

		formatted_string = gpuboard_json.dump(4);
	} else if (arg.output == csv) {
		if (node_temp_retimer.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < node_temp_retimer.size(); i++) {
				if (node_temp_retimer[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_retimer[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", node_temp_retimer[i].val);
				}
			}
		}

		if (node_temp_ibc_temp.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < node_temp_ibc_temp.size(); i++) {
				if (node_temp_ibc_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_ibc_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", node_temp_ibc_temp[i].val);
				}
			}
		}

		if (node_temp_ibc_2_temp.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < node_temp_ibc_2_temp.size(); i++) {
				if (node_temp_ibc_2_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_ibc_2_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", node_temp_ibc_2_temp[i].val);
				}
			}
		}

		if (node_temp_vdd18_vr_temp.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < node_temp_vdd18_vr_temp.size(); i++) {
				if (node_temp_vdd18_vr_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_vdd18_vr_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", node_temp_vdd18_vr_temp[i].val);
				}
			}
		}

		if (node_temp_04_hbm_b_vr_temp.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < node_temp_04_hbm_b_vr_temp.size(); i++) {
				if (node_temp_04_hbm_b_vr_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_04_hbm_b_vr_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", node_temp_04_hbm_b_vr_temp[i].val);
				}
			}
		}

		if (node_temp_04_hbm_d_vr_temp.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < node_temp_04_hbm_d_vr_temp.size(); i++) {
				if (node_temp_04_hbm_d_vr_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_04_hbm_d_vr_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", node_temp_04_hbm_d_vr_temp[i].val);
				}
			}
		}

		if (vr_temp_vddcr_vdd0.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd0.size(); i++) {
				if (vr_temp_vddcr_vdd0[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd0[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_vdd0[i].val);
				}
			}
		}

		if (vr_temp_vddcr_vdd1.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd1.size(); i++) {
				if (vr_temp_vddcr_vdd1[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd1[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_vdd1[i].val);
				}
			}
		}

		if (vr_temp_vddcr_vdd2.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd2.size(); i++) {
				if (vr_temp_vddcr_vdd2[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd2[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_vdd2[i].val);
				}
			}
		}

		if (vr_temp_vddcr_vdd3.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd3.size(); i++) {
				if (vr_temp_vddcr_vdd3[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd3[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_vdd3[i].val);
				}
			}
		}

		if (vr_temp_vddcr_soc_a.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_soc_a.size(); i++) {
				if (vr_temp_vddcr_soc_a[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_soc_a[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_soc_a[i].val);
				}
			}
		}

		if (vr_temp_vddcr_soc_c.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_soc_c.size(); i++) {
				if (vr_temp_vddcr_soc_c[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_soc_c[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_soc_c[i].val);
				}
			}
		}

		if (vr_temp_vddcr_socio_a.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_socio_a.size(); i++) {
				if (vr_temp_vddcr_socio_a[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_socio_a[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_socio_a[i].val);
				}
			}
		}

		if (vr_temp_vddcr_socio_c.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_socio_c.size(); i++) {
				if (vr_temp_vddcr_socio_c[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_socio_c[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_socio_c[i].val);
				}
			}
		}

		if (vr_temp_vdd_085_hbm.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vdd_085_hbm.size(); i++) {
				if (vr_temp_vdd_085_hbm[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vdd_085_hbm[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vdd_085_hbm[i].val);
				}
			}
		}

		if (vr_temp_vddcr_11_hbm_b.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_11_hbm_b.size(); i++) {
				if (vr_temp_vddcr_11_hbm_b[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_11_hbm_b[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_11_hbm_b[i].val);
				}
			}
		}

		if (vr_temp_vddcr_11_hbm_d.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_11_hbm_d.size(); i++) {
				if (vr_temp_vddcr_11_hbm_d[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_11_hbm_d[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddcr_11_hbm_d[i].val);
				}
			}
		}

		if (vr_temp_vdd_usr.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vdd_usr.size(); i++) {
				if (vr_temp_vdd_usr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vdd_usr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vdd_usr[i].val);
				}
			}
		}

		if (vr_temp_vddio_11_e32.size() == 0) {
			formatted_string += string_format(",%s", "N/A");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddio_11_e32.size(); i++) {
				if (vr_temp_vddio_11_e32[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddio_11_e32[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					formatted_string += string_format(",%d", vr_temp_vddio_11_e32[i].val);
				}
			}
		}

	} else {
		formatted_string = GpuBoardHeaderTemplate;

		if (node_temp_retimer.size() == 0) {
			formatted_string += string_format(gpuboardNodeTempRetimerTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < node_temp_retimer.size(); i++) {
				if (node_temp_retimer[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_retimer[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string node_temp_retimer_unit = node_temp_retimer[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string node_temp_retimer_val = string_format("%d", node_temp_retimer[i].val);
					formatted_string += string_format(gpuboardNodeTempRetimerTemplate, node_temp_retimer_val.c_str(), node_temp_retimer_unit.c_str());
				}
			}
		}

		if (node_temp_ibc_temp.size() == 0) {
			formatted_string += string_format(gpuboardNodeTempIbcTempTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < node_temp_ibc_temp.size(); i++) {
				if (node_temp_ibc_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_ibc_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string node_temp_ibc_temp_unit = node_temp_ibc_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string node_temp_ibc_temp_val = string_format("%d", node_temp_ibc_temp[i].val);
					formatted_string += string_format(gpuboardNodeTempIbcTempTemplate, node_temp_ibc_temp_val.c_str(), node_temp_ibc_temp_unit.c_str());
				}
			}
		}

		if (node_temp_ibc_2_temp.size() == 0) {
			formatted_string += string_format(gpuboardNodeTempIbc2TempTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < node_temp_ibc_2_temp.size(); i++) {
				if (node_temp_ibc_2_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_ibc_2_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string node_temp_ibc_2_temp_unit = node_temp_ibc_2_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string node_temp_ibc_2_temp_val = string_format("%d", node_temp_ibc_2_temp[i].val);
					formatted_string += string_format(gpuboardNodeTempIbc2TempTemplate, node_temp_ibc_2_temp_val.c_str(), node_temp_ibc_2_temp_unit.c_str());
				}
			}
		}

		if (node_temp_vdd18_vr_temp.size() == 0) {
			formatted_string += string_format(gpuboardNodeTempVdd18VrTempTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < node_temp_vdd18_vr_temp.size(); i++) {
				if (node_temp_vdd18_vr_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_vdd18_vr_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string node_temp_vdd18_vr_temp_unit = node_temp_vdd18_vr_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string node_temp_vdd18_vr_temp_val = string_format("%d", node_temp_vdd18_vr_temp[i].val);
					formatted_string += string_format(gpuboardNodeTempVdd18VrTempTemplate, node_temp_vdd18_vr_temp_val.c_str(), node_temp_vdd18_vr_temp_unit.c_str());
				}
			}
		}

		if (node_temp_04_hbm_b_vr_temp.size() == 0) {
			formatted_string += string_format(gpuboardNodeTemp04HbmBVrTempTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < node_temp_04_hbm_b_vr_temp.size(); i++) {
				if (node_temp_04_hbm_b_vr_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_04_hbm_b_vr_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string node_temp_04_hbm_b_vr_temp_unit = node_temp_04_hbm_b_vr_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string node_temp_04_hbm_b_vr_temp_val = string_format("%d", node_temp_04_hbm_b_vr_temp[i].val);
					formatted_string += string_format(gpuboardNodeTemp04HbmBVrTempTemplate, node_temp_04_hbm_b_vr_temp_val.c_str(), node_temp_04_hbm_b_vr_temp_unit.c_str());
				}
			}
		}

		if (node_temp_04_hbm_d_vr_temp.size() == 0) {
			formatted_string += string_format(gpuboardNodeTemp04HbmDVrTempTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < node_temp_04_hbm_d_vr_temp.size(); i++) {
				if (node_temp_04_hbm_d_vr_temp[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && node_temp_04_hbm_d_vr_temp[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string node_temp_04_hbm_d_vr_temp_unit = node_temp_04_hbm_d_vr_temp[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string node_temp_04_hbm_d_vr_temp_val = string_format("%d", node_temp_04_hbm_d_vr_temp[i].val);
					formatted_string += string_format(gpuboardNodeTemp04HbmDVrTempTemplate, node_temp_04_hbm_d_vr_temp_val.c_str(), node_temp_04_hbm_d_vr_temp_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_vdd0.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcrVdd0Template, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd0.size(); i++) {
				if (vr_temp_vddcr_vdd0[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd0[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_vdd0_unit = vr_temp_vddcr_vdd0[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_vdd0_val = string_format("%d", vr_temp_vddcr_vdd0[i].val);
					formatted_string += string_format(gpuboardVrTempVddcrVdd0Template, vr_temp_vddcr_vdd0_val.c_str(), vr_temp_vddcr_vdd0_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_vdd1.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcrVdd1Template, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd1.size(); i++) {
				if (vr_temp_vddcr_vdd1[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd1[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_vdd1_unit = vr_temp_vddcr_vdd1[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_vdd1_val = string_format("%d", vr_temp_vddcr_vdd1[i].val);
					formatted_string += string_format(gpuboardVrTempVddcrVdd1Template, vr_temp_vddcr_vdd1_val.c_str(), vr_temp_vddcr_vdd1_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_vdd2.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcrVdd2Template, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd2.size(); i++) {
				if (vr_temp_vddcr_vdd2[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd2[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_vdd2_unit = vr_temp_vddcr_vdd2[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_vdd2_val = string_format("%d", vr_temp_vddcr_vdd2[i].val);
					formatted_string += string_format(gpuboardVrTempVddcrVdd2Template, vr_temp_vddcr_vdd2_val.c_str(), vr_temp_vddcr_vdd2_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_vdd3.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcrVdd3Template, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_vdd3.size(); i++) {
				if (vr_temp_vddcr_vdd3[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_vdd3[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_vdd3_unit = vr_temp_vddcr_vdd3[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_vdd3_val = string_format("%d", vr_temp_vddcr_vdd3[i].val);
					formatted_string += string_format(gpuboardVrTempVddcrVdd3Template, vr_temp_vddcr_vdd3_val.c_str(), vr_temp_vddcr_vdd3_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_soc_a.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcrSocATemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_soc_a.size(); i++) {
				if (vr_temp_vddcr_soc_a[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_soc_a[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_soc_a_unit = vr_temp_vddcr_soc_a[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_soc_a_val = string_format("%d", vr_temp_vddcr_soc_a[i].val);
					formatted_string += string_format(gpuboardVrTempVddcrSocATemplate, vr_temp_vddcr_soc_a_val.c_str(), vr_temp_vddcr_soc_a_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_soc_c.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcrSocCTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_soc_c.size(); i++) {
				if (vr_temp_vddcr_soc_c[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_soc_c[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_soc_c_unit = vr_temp_vddcr_soc_c[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_soc_c_val = string_format("%d", vr_temp_vddcr_soc_c[i].val);
					formatted_string += string_format(gpuboardVrTempVddcrSocCTemplate, vr_temp_vddcr_soc_c_val.c_str(), vr_temp_vddcr_soc_c_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_socio_a.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcrSocioATemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_socio_a.size(); i++) {
				if (vr_temp_vddcr_socio_a[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_socio_a[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_socio_a_unit = vr_temp_vddcr_socio_a[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_socio_a_val = string_format("%d", vr_temp_vddcr_socio_a[i].val);
					formatted_string += string_format(gpuboardVrTempVddcrSocioATemplate, vr_temp_vddcr_socio_a_val.c_str(), vr_temp_vddcr_socio_a_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_socio_c.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcrSocioCTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_socio_c.size(); i++) {
				if (vr_temp_vddcr_socio_c[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_socio_c[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_socio_c_unit = vr_temp_vddcr_socio_c[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_socio_c_val = string_format("%d", vr_temp_vddcr_socio_c[i].val);
					formatted_string += string_format(gpuboardVrTempVddcrSocioCTemplate, vr_temp_vddcr_socio_c_val.c_str(), vr_temp_vddcr_socio_c_unit.c_str());
				}
			}
		}

		if (vr_temp_vdd_085_hbm.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVdd085HbmTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vdd_085_hbm.size(); i++) {
				if (vr_temp_vdd_085_hbm[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vdd_085_hbm[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vdd_085_hbm_unit = vr_temp_vdd_085_hbm[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vdd_085_hbm_val = string_format("%d", vr_temp_vdd_085_hbm[i].val);
					formatted_string += string_format(gpuboardVrTempVdd085HbmTemplate, vr_temp_vdd_085_hbm_val.c_str(), vr_temp_vdd_085_hbm_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_11_hbm_b.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcr11HbmBTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_11_hbm_b.size(); i++) {
				if (vr_temp_vddcr_11_hbm_b[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_11_hbm_b[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_11_hbm_b_unit = vr_temp_vddcr_11_hbm_b[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_11_hbm_b_val = string_format("%d", vr_temp_vddcr_11_hbm_b[i].val);
					formatted_string += string_format(gpuboardVrTempVddcr11HbmBTemplate, vr_temp_vddcr_11_hbm_b_val.c_str(), vr_temp_vddcr_11_hbm_b_unit.c_str());
				}
			}
		}

		if (vr_temp_vddcr_11_hbm_d.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddcr11HbmDTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddcr_11_hbm_d.size(); i++) {
				if (vr_temp_vddcr_11_hbm_d[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddcr_11_hbm_d[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddcr_11_hbm_d_unit = vr_temp_vddcr_11_hbm_d[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddcr_11_hbm_d_val = string_format("%d", vr_temp_vddcr_11_hbm_d[i].val);
					formatted_string += string_format(gpuboardVrTempVddcr11HbmDTemplate, vr_temp_vddcr_11_hbm_d_val.c_str(), vr_temp_vddcr_11_hbm_d_unit.c_str());
				}
			}
		}

		if (vr_temp_vdd_usr.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddUsrTemplate, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vdd_usr.size(); i++) {
				if (vr_temp_vdd_usr[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vdd_usr[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vdd_usr_unit = vr_temp_vdd_usr[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vdd_usr_val = string_format("%d", vr_temp_vdd_usr[i].val);
					formatted_string += string_format(gpuboardVrTempVddUsrTemplate, vr_temp_vdd_usr_val.c_str(), vr_temp_vdd_usr_unit.c_str());
				}
			}
		}

		if (vr_temp_vddio_11_e32.size() == 0) {
			formatted_string += string_format(gpuboardVrTempVddio11E32Template, "N/A", "");
		} else {
			for (uint32_t i = 0; i < vr_temp_vddio_11_e32.size(); i++) {
				if (vr_temp_vddio_11_e32[i].res_group == AMDSMI_METRIC_RES_GROUP_SYSTEM && vr_temp_vddio_11_e32[i].res_subgroup == AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD) {
					is_supported = true;
					std::string vr_temp_vddio_11_e32_unit = vr_temp_vddio_11_e32[i].unit == AMDSMI_METRIC_UNIT_CELSIUS ? "C" : "";
					std::string vr_temp_vddio_11_e32_val = string_format("%d", vr_temp_vddio_11_e32[i].val);
					formatted_string += string_format(gpuboardVrTempVddio11E32Template, vr_temp_vddio_11_e32_val.c_str(), vr_temp_vddio_11_e32_unit.c_str());
				}
			}
		}
	}

	if (is_supported) {
		return ret;
	} else {
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}
}

int AmdSmiApiHost::amdsmi_get_port_netdev_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_processor_handle processor;
	amdsmi_nic_port_info_t nic_port_info;
	int ret;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_nic_port_netdev_info(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_nic_port_info(processor, &nic_port_info);
	if (ret != AMDSMI_STATUS_SUCCESS || nic_port_info.num_ports == 0) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			return ret;
		}
		out = host_fill_nic_port_netdev_info(arg, "N/A");
		return ret != AMDSMI_STATUS_SUCCESS ? ret : AMDSMI_STATUS_SUCCESS;
	}

	if (arg.output == json) {
		nlohmann::ordered_json result_json;

		nlohmann::ordered_json ports_array = nlohmann::ordered_json::array();
		for (uint32_t port_idx = 0; port_idx < nic_port_info.num_ports; port_idx++) {
			nlohmann::ordered_json port_json;
			std::string netdev_name = (nic_port_info.ports[port_idx].netdev[0] != '\0') ?
									  nic_port_info.ports[port_idx].netdev : "N/A";
			port_json["netdev"] = netdev_name;

			nlohmann::ordered_json nic_statistics_json;

			uint32_t num_vendor_stats = 0;
			ret = host_amdsmi_get_nic_vendor_statistics(processor, port_idx, &num_vendor_stats, nullptr);
			if (ret == AMDSMI_STATUS_SUCCESS && num_vendor_stats > 0) {
				std::vector<amdsmi_nic_stat_t> vendor_stats(num_vendor_stats);
				ret = host_amdsmi_get_nic_vendor_statistics(processor, port_idx, &num_vendor_stats,
						vendor_stats.data());
				if (ret == AMDSMI_STATUS_SUCCESS) {
					for (uint32_t i = 0; i < num_vendor_stats; i++) {
						std::string stat_name = vendor_stats[i].name;
						std::transform(stat_name.begin(), stat_name.end(), stat_name.begin(), ::tolower);
						nic_statistics_json[stat_name] = vendor_stats[i].value;
					}
				}
			}
			port_json["vendor_statistics"] = nic_statistics_json;

			nlohmann::ordered_json port_statistics_json;

			uint32_t num_port_stats = 0;
			ret = host_amdsmi_get_nic_port_statistics(processor, port_idx, &num_port_stats, nullptr);
			if (ret == AMDSMI_STATUS_SUCCESS && num_port_stats > 0) {
				std::vector<amdsmi_nic_stat_t> port_stats(num_port_stats);
				ret = host_amdsmi_get_nic_port_statistics(processor, port_idx, &num_port_stats, port_stats.data());
				if (ret == AMDSMI_STATUS_SUCCESS) {
					for (uint32_t i = 0; i < num_port_stats; i++) {
						std::string stat_name = port_stats[i].name;
						std::transform(stat_name.begin(), stat_name.end(), stat_name.begin(), ::tolower);
						port_statistics_json[stat_name] = port_stats[i].value;
					}
				}
			}
			port_json["statistics"] = port_statistics_json;
			ports_array.push_back(port_json);
		}
		result_json["ports"] = ports_array;

		out = result_json.dump(4);
	} else if (arg.output == human) {
		out.append(metricNicPortStatsHeaderTemplate);
		for (uint32_t port_idx = 0; port_idx < nic_port_info.num_ports; port_idx++) {
			std::string netdev_name = (nic_port_info.ports[port_idx].netdev[0] != '\0') ?
									  nic_port_info.ports[port_idx].netdev : "N/A";
			out.append(string_format(metricNicPortTemplate, port_idx, netdev_name.c_str()));

			out.append(metricNicVendorStatsHeaderTemplate);

			uint32_t num_vendor_stats = 0;
			ret = host_amdsmi_get_nic_vendor_statistics(processor, port_idx, &num_vendor_stats, nullptr);
			if (ret == AMDSMI_STATUS_SUCCESS && num_vendor_stats > 0) {
				std::vector<amdsmi_nic_stat_t> vendor_stats(num_vendor_stats);
				ret = host_amdsmi_get_nic_vendor_statistics(processor, port_idx, &num_vendor_stats,
						vendor_stats.data());
				if (ret == AMDSMI_STATUS_SUCCESS) {
					for (uint32_t i = 0; i < num_vendor_stats; i++) {
						std::string stat_name = vendor_stats[i].name;
						std::transform(stat_name.begin(), stat_name.end(), stat_name.begin(), ::toupper);
						std::string value_str = string_format("%llu", vendor_stats[i].value);
						out.append(string_format("                    %s: %s\n", stat_name.c_str(), value_str.c_str()));
					}
				}
			}

			out.append(metricNicPortStatisticsHeaderTemplate);
			uint32_t num_port_stats = 0;
			ret = host_amdsmi_get_nic_port_statistics(processor, port_idx, &num_port_stats, nullptr);
			if (ret == AMDSMI_STATUS_SUCCESS && num_port_stats > 0) {
				std::vector<amdsmi_nic_stat_t> port_stats(num_port_stats);
				ret = host_amdsmi_get_nic_port_statistics(processor, port_idx, &num_port_stats, port_stats.data());
				if (ret == AMDSMI_STATUS_SUCCESS) {
					for (uint32_t i = 0; i < num_port_stats; i++) {
						std::string stat_name = port_stats[i].name;
						std::transform(stat_name.begin(), stat_name.end(), stat_name.begin(), ::toupper);
						std::string value_str = string_format("%llu", port_stats[i].value);
						out.append(string_format("                    %s: %s\n", stat_name.c_str(), value_str.c_str()));
					}
				}
			}
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_port_rdma_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_processor_handle processor;
	amdsmi_nic_port_info_t nic_port_info;
	amdsmi_nic_rdma_devices_info_t nic_rdma_devices_info;
	int ret;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_metric_nic_rdma_dev_info(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_nic_port_info(processor, &nic_port_info);
	if (ret != AMDSMI_STATUS_SUCCESS || nic_port_info.num_ports == 0) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			return ret;
		}
		out = host_fill_metric_nic_rdma_dev_info(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_nic_rdma_dev_info(processor, &nic_rdma_devices_info);
	if (ret != AMDSMI_STATUS_SUCCESS || nic_rdma_devices_info.num_rdma_dev == 0) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			return ret;
		}
		out = host_fill_metric_nic_rdma_dev_info(arg, "N/A");
		return ret;
	}
	if (arg.output == json) {
		nlohmann::ordered_json result_json;
		nlohmann::ordered_json rdma_devices_array = nlohmann::ordered_json::array();
		for (uint32_t rdma_dev_idx = 0; rdma_dev_idx < nic_rdma_devices_info.num_rdma_dev; rdma_dev_idx++) {
			nlohmann::ordered_json rdma_device_json;
			rdma_device_json["rdma_dev"] = nic_rdma_devices_info.rdma_dev_info[rdma_dev_idx].rdma_dev;

			nlohmann::ordered_json rdma_ports_array = nlohmann::ordered_json::array();
			for (uint32_t rdma_port_idx = 0;
					rdma_port_idx < nic_rdma_devices_info.rdma_dev_info[rdma_dev_idx].num_rdma_ports; rdma_port_idx++) {
				nlohmann::ordered_json rdma_port_json;

				nlohmann::ordered_json rdma_port_statistics_json;
				uint8_t rdma_port_num =
					nic_rdma_devices_info.rdma_dev_info[rdma_dev_idx].rdma_port_info[rdma_port_idx].rdma_port;

				uint32_t num_rdma_stats = 0;
				ret = host_amdsmi_get_nic_rdma_port_statistics(processor, rdma_port_idx, &num_rdma_stats, nullptr);
				if (ret == AMDSMI_STATUS_SUCCESS && num_rdma_stats > 0) {
					std::vector<amdsmi_nic_stat_t> rdma_stats(num_rdma_stats);
					uint32_t actual_stats = num_rdma_stats;

					ret = host_amdsmi_get_nic_rdma_port_statistics(processor, rdma_port_idx, &actual_stats,
							rdma_stats.data());
					if (ret == AMDSMI_STATUS_SUCCESS) {
						for (uint32_t i = 0; i < actual_stats; i++) {
							std::string stat_name = rdma_stats[i].name;
							std::transform(stat_name.begin(), stat_name.end(), stat_name.begin(), ::tolower);
							rdma_port_statistics_json[stat_name] = rdma_stats[i].value;
						}
					}
				}
				rdma_port_json["statistics"] = rdma_port_statistics_json;
				rdma_ports_array.push_back(rdma_port_json);
			}
			rdma_device_json["ports"] = rdma_ports_array;
			rdma_devices_array.push_back(rdma_device_json);
		}
		result_json["rdma_devices"] = rdma_devices_array;
		out = result_json.dump(4);
	} else if (arg.output == human) {
		out.append(metricNicRdmaStatsHeaderTemplate);
		for (uint32_t rdma_dev_idx = 0; rdma_dev_idx < nic_rdma_devices_info.num_rdma_dev; rdma_dev_idx++) {
			out.append(string_format(metricNicRdmaDeviceTemplate, rdma_dev_idx,
										nic_rdma_devices_info.rdma_dev_info[rdma_dev_idx].rdma_dev));

			for (uint32_t rdma_port_idx = 0;
					rdma_port_idx < nic_rdma_devices_info.rdma_dev_info[rdma_dev_idx].num_rdma_ports; rdma_port_idx++) {
				uint8_t rdma_port_num =
					nic_rdma_devices_info.rdma_dev_info[rdma_dev_idx].rdma_port_info[rdma_port_idx].rdma_port;
				out.append(string_format(metricNicRdmaPortTemplate, rdma_port_idx));

				uint32_t num_rdma_stats = 0;
				ret = host_amdsmi_get_nic_rdma_port_statistics(processor, rdma_port_idx, &num_rdma_stats, nullptr);
				if (ret == AMDSMI_STATUS_SUCCESS && num_rdma_stats > 0) {
					std::vector<amdsmi_nic_stat_t> rdma_stats(num_rdma_stats);
					uint32_t actual_stats = num_rdma_stats;
					ret = host_amdsmi_get_nic_rdma_port_statistics(processor, rdma_port_idx, &actual_stats,
							rdma_stats.data());

					if (ret == AMDSMI_STATUS_SUCCESS) {
						for (uint32_t i = 0; i < actual_stats; i++) {
							std::string stat_name = rdma_stats[i].name;
							std::transform(stat_name.begin(), stat_name.end(), stat_name.begin(), ::toupper);
							std::string value_str = string_format("%llu", rdma_stats[i].value);
							out.append(string_format("                            %s: %s\n", stat_name.c_str(),
														value_str.c_str()));
						}
					}
				}
			}
		}
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_throttle_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_throttle(arg, "N/A");
		return ret;
	}

	auto find_metric = [](const amdsmi_metric_t *metrics, uint32_t count,
			amdsmi_metric_name_t name) -> uint64_t {
		for (uint32_t i = 0; i < count; i++) {
			if (metrics[i].name == name)
				return metrics[i].val;
		}
		return UINT64_MAX;
	};

	uint32_t metric_size_a = AMDSMI_MAX_NUM_METRICS;
	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size_a, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_throttle(arg, "N/A");
		return ret;
	}
	amdsmi_metric_t *metrics_a = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t) * metric_size_a);
	if (metrics_a == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size_a, &metrics_a[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(metrics_a);
		out = host_fill_throttle(arg, "N/A");
		return ret;
	}
	uint64_t acc_counter_a = find_metric(metrics_a, metric_size_a, AMDSMI_METRIC_NAME_METRIC_ACC_COUNTER);
	uint64_t prochot_a = find_metric(metrics_a, metric_size_a, AMDSMI_METRIC_NAME_THROTTLE_PROCHOT_ACTIVE);
	uint64_t ppt_a = find_metric(metrics_a, metric_size_a, AMDSMI_METRIC_NAME_THROTTLE_PPT_ACTIVE);
	uint64_t socket_a = find_metric(metrics_a, metric_size_a, AMDSMI_METRIC_NAME_THROTTLE_SOCKET_ACTIVE);
	uint64_t vr_a = find_metric(metrics_a, metric_size_a, AMDSMI_METRIC_NAME_THROTTLE_VR_ACTIVE);
	uint64_t hbm_a = find_metric(metrics_a, metric_size_a, AMDSMI_METRIC_NAME_THROTTLE_MEM_ACTIVE);
	free(metrics_a);

	auto ts_start = std::chrono::steady_clock::now();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	uint32_t metric_size_b = AMDSMI_MAX_NUM_METRICS;
	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size_b, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_throttle(arg, "N/A");
		return ret;
	}
	amdsmi_metric_t *metrics_b = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t) * metric_size_b);
	if (metrics_b == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_gpu_metrics(processor, &metric_size_b, &metrics_b[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(metrics_b);
		out = host_fill_throttle(arg, "N/A");
		return ret;
	}
	uint64_t acc_counter_b = find_metric(metrics_b, metric_size_b, AMDSMI_METRIC_NAME_METRIC_ACC_COUNTER);
	uint64_t prochot_b = find_metric(metrics_b, metric_size_b, AMDSMI_METRIC_NAME_THROTTLE_PROCHOT_ACTIVE);
	uint64_t ppt_b = find_metric(metrics_b, metric_size_b, AMDSMI_METRIC_NAME_THROTTLE_PPT_ACTIVE);
	uint64_t socket_b = find_metric(metrics_b, metric_size_b, AMDSMI_METRIC_NAME_THROTTLE_SOCKET_ACTIVE);
	uint64_t vr_b = find_metric(metrics_b, metric_size_b, AMDSMI_METRIC_NAME_THROTTLE_VR_ACTIVE);
	uint64_t hbm_b = find_metric(metrics_b, metric_size_b, AMDSMI_METRIC_NAME_THROTTLE_MEM_ACTIVE);
	free(metrics_b);

	auto ts_end = std::chrono::steady_clock::now();
	uint64_t ts_delta_us = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_start).count();

	std::string accumulation_counter = (acc_counter_b == UINT64_MAX) ? "N/A" :
		string_format("%" PRIu64, acc_counter_b);

	std::string prochot_pct = violation_compute_pct(prochot_a, prochot_b,
		acc_counter_a, acc_counter_b, ts_delta_us);
	std::string ppt_pct = violation_compute_pct(ppt_a, ppt_b,
		acc_counter_a, acc_counter_b, ts_delta_us);
	std::string socket_pct = violation_compute_pct(socket_a, socket_b,
		acc_counter_a, acc_counter_b, ts_delta_us);
	std::string vr_pct = violation_compute_pct(vr_a, vr_b,
		acc_counter_a, acc_counter_b, ts_delta_us);
	std::string hbm_pct = violation_compute_pct(hbm_a, hbm_b,
		acc_counter_a, acc_counter_b, ts_delta_us);

	std::string prochot_active = violation_is_active(prochot_a, prochot_b);
	std::string ppt_active = violation_is_active(ppt_a, ppt_b);
	std::string socket_active = violation_is_active(socket_a, socket_b);
	std::string vr_active = violation_is_active(vr_a, vr_b);
	std::string hbm_active = violation_is_active(hbm_a, hbm_b);

	std::string prochot_acc = (prochot_b == UINT64_MAX) ? "N/A" :
		string_format("%" PRIu64, prochot_b);
	std::string ppt_acc = (ppt_b == UINT64_MAX) ? "N/A" :
		string_format("%" PRIu64, ppt_b);
	std::string socket_acc = (socket_b == UINT64_MAX) ? "N/A" :
		string_format("%" PRIu64, socket_b);
	std::string vr_acc = (vr_b == UINT64_MAX) ? "N/A" :
		string_format("%" PRIu64, vr_b);
	std::string hbm_acc = (hbm_b == UINT64_MAX) ? "N/A" :
		string_format("%" PRIu64, hbm_b);

	std::string prochot_pct_unit = prochot_pct == "N/A" ? "" : "%";
	std::string ppt_pct_unit = ppt_pct == "N/A" ? "" : "%";
	std::string socket_pct_unit = socket_pct == "N/A" ? "" : "%";
	std::string vr_pct_unit = vr_pct == "N/A" ? "" : "%";
	std::string hbm_pct_unit = hbm_pct == "N/A" ? "" : "%";

	if (arg.watch > -1) {
		out = string_format(
				  "%s,%s,%s %s,%s,%s,%s %s,%s,%s,%s %s,%s,%s,%s %s,%s,%s,%s %s,%s",
				  accumulation_counter.c_str(),
				  prochot_acc.c_str(), prochot_pct.c_str(), prochot_pct_unit.c_str(), prochot_active.c_str(),
				  ppt_acc.c_str(), ppt_pct.c_str(), ppt_pct_unit.c_str(), ppt_active.c_str(),
				  socket_acc.c_str(), socket_pct.c_str(), socket_pct_unit.c_str(), socket_active.c_str(),
				  vr_acc.c_str(), vr_pct.c_str(), vr_pct_unit.c_str(), vr_active.c_str(),
				  hbm_acc.c_str(), hbm_pct.c_str(), hbm_pct_unit.c_str(), hbm_active.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json throttle_json;

		if (acc_counter_b == UINT64_MAX) {
			throttle_json["accumulation_counter"] = "N/A";
		} else {
			throttle_json["accumulation_counter"] = acc_counter_b;
		}

		auto to_json_val = [](const std::string &v) -> nlohmann::ordered_json {
			if (v == "N/A") return "N/A";
			return std::stoull(v);
		};

		auto make_activity_pct = [](const std::string &pct) {
			nlohmann::ordered_json obj{};
			if (pct == "N/A") {
				obj["value"] = "N/A";
				obj["unit"] = "N/A";
			} else {
				obj["value"] = std::stoull(pct);
				obj["unit"] = "%";
			}
			return obj;
		};

		throttle_json["prochot_violation_accumulated"] = to_json_val(prochot_acc);
		throttle_json["prochot_violation_activity"] = make_activity_pct(prochot_pct);
		throttle_json["prochot_violation_status"] = prochot_active;
		throttle_json["ppt_violation_accumulated"] = to_json_val(ppt_acc);
		throttle_json["ppt_violation_activity"] = make_activity_pct(ppt_pct);
		throttle_json["ppt_violation_status"] = ppt_active;
		throttle_json["socket_thermal_violation_accumulated"] = to_json_val(socket_acc);
		throttle_json["socket_thermal_violation_activity"] = make_activity_pct(socket_pct);
		throttle_json["socket_thermal_violation_status"] = socket_active;
		throttle_json["vr_thermal_violation_accumulated"] = to_json_val(vr_acc);
		throttle_json["vr_thermal_violation_activity"] = make_activity_pct(vr_pct);
		throttle_json["vr_thermal_violation_status"] = vr_active;
		throttle_json["hbm_thermal_violation_accumulated"] = to_json_val(hbm_acc);
		throttle_json["hbm_thermal_violation_activity"] = make_activity_pct(hbm_pct);
		throttle_json["hbm_thermal_violation_status"] = hbm_active;

		out = throttle_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
				  accumulation_counter.c_str(),
				  prochot_acc.c_str(), prochot_pct.c_str(), prochot_active.c_str(),
				  ppt_acc.c_str(), ppt_pct.c_str(), ppt_active.c_str(),
				  socket_acc.c_str(), socket_pct.c_str(), socket_active.c_str(),
				  vr_acc.c_str(), vr_pct.c_str(), vr_active.c_str(),
				  hbm_acc.c_str(), hbm_pct.c_str(), hbm_active.c_str());
	} else {
		out = string_format(
				  ThrottleInfoHeaderTemplate, accumulation_counter.c_str(),
				  prochot_acc.c_str(),
				  prochot_pct.c_str(), prochot_pct_unit.c_str(),
				  prochot_active.c_str(),
				  ppt_acc.c_str(),
				  ppt_pct.c_str(), ppt_pct_unit.c_str(),
				  ppt_active.c_str(),
				  socket_acc.c_str(),
				  socket_pct.c_str(), socket_pct_unit.c_str(),
				  socket_active.c_str(),
				  vr_acc.c_str(),
				  vr_pct.c_str(), vr_pct_unit.c_str(),
				  vr_active.c_str(),
				  hbm_acc.c_str(),
				  hbm_pct.c_str(), hbm_pct_unit.c_str(),
				  hbm_active.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}
