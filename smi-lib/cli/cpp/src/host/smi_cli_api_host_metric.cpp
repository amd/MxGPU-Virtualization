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

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
		amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle, uint32_t,
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

	bool setup_template{true};
	bool metric_table{false};
	std::vector<amdsmi_metric_t> jpeg_chiplet{};
	std::vector<amdsmi_metric_t> vcn_chiplet{};
	std::vector<amdsmi_metric_t> mm_chiplet{};
	std::vector<amdsmi_metric_t> mem_chiplet{};
	std::vector<amdsmi_metric_t> gfx_chiplet{};

	if ((AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200())
			&& arg.watch == -1) {
		amdsmi_metric_t *metrics;
		uint32_t metric_size = AMDSMI_MAX_NUM_METRICS;

		ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, NULL);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			metric_table = false;
			setup_template = true;
		}
		metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t)*metric_size);
		if (metrics == NULL) {
			throw SmiToolNotEnoughMemException();
		}
		ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, &metrics[0]);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			metric_table = false;
			setup_template = true;
		}

		if (ret == AMDSMI_STATUS_SUCCESS) {
			metric_table = true;
			setup_template = false;
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
			if (jpeg_chiplet.size() == 0 || vcn_chiplet.size() == 0 || mem_chiplet.size() != 1
					|| gfx_chiplet.size() != 1) {
				setup_template = true;
			}
		} else {
			metric_table = false;
			setup_template = true;
		}
		free(metrics);
	}
	if (metric_table == false || setup_template == true) {
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
	uint32_t sensor_ind = 0;
	bool is_power_management_enabled;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_power(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_power_info(processor, sensor_ind, &power_info);
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


int AmdSmiApiHost::amdsmi_get_clock_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_clk_info_t clock_measure_gfx, clock_measure_mem;
	amdsmi_clk_info_t clock_measure_vclk0, clock_measure_vclk1;
	amdsmi_clk_info_t clock_measure_dclk0, clock_measure_dclk1;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	std::string gfx_clk{};
	std::string gfx_max_clk{};
	std::string gfx_min_clk{};
	std::string is_clk_gfx_locked{"N/A"};
	std::string gfx_clk_deep_sleep{"N/A"};

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = host_fill_clock(arg, "N/A");
		return ret;
	}

	ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_GFX,
									 &clock_measure_gfx);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		gfx_clk = "N/A";
		gfx_max_clk = "N/A";
	} else {
		gfx_clk = string_format(
					  "%ld", clock_measure_gfx.clk);
		gfx_max_clk = string_format(
						  "%ld", clock_measure_gfx.max_clk);
	}
	std::string mem_clk{};
	std::string mem_max_clk{};
	std::string mem_min_clk{};
	std::string is_clk_mem_locked{"N/A"};
	std::string mem_clk_deep_sleep{"N/A"};
	ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_MEM,
									 &clock_measure_mem);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		mem_clk = "N/A";
		mem_max_clk = "N/A";
	} else {
		mem_clk = string_format(
					  "%ld", clock_measure_mem.clk);
		mem_max_clk = string_format(
						  "%ld", clock_measure_mem.max_clk);
	}
	std::string vclk0_clk{};
	std::string vclk0_max_clk{};
	std::string vclk0_min_clk{};
	std::string is_clk_vclk0_locked{"N/A"};
	std::string vclk0_clk_deep_sleep{"N/A"};
	ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK0,
									 &clock_measure_vclk0);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		vclk0_min_clk = "N/A";
		vclk0_clk = "N/A";
		vclk0_max_clk = "N/A";
	} else {
		vclk0_clk = clock_measure_vclk0.clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk0.clk);
		vclk0_max_clk = clock_measure_vclk0.max_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk0.max_clk);
		vclk0_min_clk = clock_measure_vclk0.min_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk0.min_clk);
	}
	std::string vclk1_clk{};
	std::string vclk1_max_clk{};
	std::string vclk1_min_clk{};
	std::string is_clk_vclk1_locked{"N/A"};
	std::string vclk1_clk_deep_sleep{"N/A"};
	ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK1,
									 &clock_measure_vclk1);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		vclk1_min_clk = "N/A";
		vclk1_clk = "N/A";
		vclk1_max_clk = "N/A";
	} else {
		vclk1_clk = clock_measure_vclk1.clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk1.clk);
		vclk1_max_clk = clock_measure_vclk1.max_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk1.max_clk);
		vclk1_min_clk = clock_measure_vclk1.min_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk1.min_clk);
	}

	std::string dclk0_clk{};
	std::string dclk0_max_clk{};
	std::string dclk0_min_clk{};
	std::string is_clk_dclk0_locked{"N/A"};
	std::string dclk0_clk_deep_sleep{"N/A"};

	std::string dclk1_clk{};
	std::string dclk1_max_clk{};
	std::string dclk1_min_clk{};
	std::string is_clk_dclk1_locked{"N/A"};
	std::string dclk1_clk_deep_sleep{"N/A"};

	bool setup_template{true};
	bool metric_table{false};

	std::vector<std::vector<std::string>> output_rows{};
	std::map<std::string, std::vector<amdsmi_metric_t>> gfx_chiplet{};
	std::map<std::string, std::vector<amdsmi_metric_t>> mem_chiplet{};
	std::map<std::string, std::vector<amdsmi_metric_t>> vclk_chiplet{};
	std::map<std::string, std::vector<amdsmi_metric_t>> dclk_chiplet{};
	nlohmann::ordered_json result{};

	if ((AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200())
			&& arg.watch == -1) {
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
				metric_table = false;
				setup_template = true;
			} else {
				metric_table = true;
				for (int i = 0; i < metric_size; i++) {
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						gfx_chiplet["clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX_MIN_LIMIT
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						gfx_chiplet["min_clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX_MAX_LIMIT
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						gfx_chiplet["max_clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX_LOCKED
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						gfx_chiplet["clk_locked"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_GFX_DS_DISABLED
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						gfx_chiplet["deep_sleep"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_MEM
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						mem_chiplet["clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_MEM_MIN_LIMIT
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						mem_chiplet["min_clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_MEM_MAX_LIMIT
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						mem_chiplet["max_clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_MEM_DS_DISABLED
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						mem_chiplet["deep_sleep"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_VCLK
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						vclk_chiplet["clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_VCLK_MIN_LIMIT
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						vclk_chiplet["min_clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_VCLK_MAX_LIMIT
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						vclk_chiplet["max_clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_VCLK_DS_DISABLED
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						vclk_chiplet["deep_sleep"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_DCLK
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						dclk_chiplet["clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_DCLK_MIN_LIMIT
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						dclk_chiplet["min_clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_DCLK_MAX_LIMIT
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						dclk_chiplet["max_clk"].push_back(metrics[i]);
					}
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_DCLK_DS_DISABLED
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						dclk_chiplet["deep_sleep"].push_back(metrics[i]);
					}
				}
			}
			free(metrics);
		} else {
			metric_table = false;
			setup_template = true;
		}
	}

	if (metric_table) {
		std::vector<std::size_t> gfx_chiplet_size = {gfx_chiplet["clk"].size(), gfx_chiplet["min_clk"].size(), gfx_chiplet["max_clk"].size(),
													 gfx_chiplet["clk_locked"].size(), gfx_chiplet["deep_sleep"].size()
													};
		std::vector<std::size_t> mem_chiplet_size = {mem_chiplet["clk"].size(), mem_chiplet["min_clk"].size(), mem_chiplet["max_clk"].size(),
													 mem_chiplet["deep_sleep"].size()
													};
		std::size_t gfx_cur_size = gfx_chiplet_size[0];
		std::size_t mem_cur_size = mem_chiplet_size[0];
		if (std::any_of(gfx_chiplet_size.cbegin(), gfx_chiplet_size.cend(), [&](int i) {
		return i != gfx_cur_size;
	})) {
			setup_template = true;
		} else if (std::any_of(mem_chiplet_size.cbegin(), mem_chiplet_size.cend(), [&](int i) {
		return i != mem_cur_size;
	})) {
			setup_template = true;
		} else if (gfx_cur_size == 0 || mem_cur_size == 0) {
			setup_template = true;
		} else {
			setup_template = false;
			if (arg.output == json) {
				for (int i = 0; i < gfx_cur_size; i++) {
					nlohmann::ordered_json gfx_clk_json{};
					if (gfx_chiplet["clk"][i].val == UINT64_MAX) {
						gfx_clk_json["value"] = "N/A";
					} else {
						gfx_clk_json["value"] = gfx_chiplet["clk"][i].val;
					}
					gfx_clk_json["unit"] = string_format("%llu",
														 gfx_chiplet["clk"][i].val) == "N/A" ? "N/A" : "MHz";
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
					result[string_format("gfx_%d",i)] = nlohmann::ordered_json::object( {
						{"clk", gfx_clk_json },
						{"min_clk", gfx_min_clk_json },
						{"max_clk", gfx_max_clk_json },
						{"clk_locked", gfx_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" : gfx_chiplet["clk_locked"][i].val ? "ENABLED" : "DISABLED" },
						{"deep_sleep", gfx_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : gfx_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED" }});
				}
				for (int i = 0; i < mem_cur_size; i++) {
					nlohmann::ordered_json mem_clk_json{};
					if (mem_chiplet["clk"][i].val == UINT64_MAX) {
						mem_clk_json["value"] = "N/A";
					} else {
						mem_clk_json["value"] = mem_chiplet["clk"][i].val;
					}
					mem_clk_json["unit"] = string_format("%llu",
														 mem_chiplet["clk"][i].val) == "N/A" ? "N/A" :  "MHz";
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
					result[string_format("mem_%d",i)] = nlohmann::ordered_json::object( {
						{"clk", mem_clk_json },
						{"min_clk", mem_min_clk_json },
						{"max_clk", mem_max_clk_json },
						{"clk_locked", "N/A" },
						{"deep_sleep", mem_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : mem_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED" }});
				}
			} else if (arg.output == csv) {
				std::vector<std::string> value_rows{};
				for (int i = 0; i < gfx_cur_size; i++) {
					value_rows.push_back(string_format(",%d,%s,%s,%s,%s,%s", i,
													   gfx_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   gfx_chiplet["clk"][i].val),
													   gfx_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   gfx_chiplet["min_clk"][i].val),
													   gfx_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   gfx_chiplet["max_clk"][i].val),
													   gfx_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" : gfx_chiplet["clk_locked"][i].val ?
													   "ENABLED" : "DISABLED",
													   gfx_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : gfx_chiplet["deep_sleep"][i].val ?
													   "DISABLED" : "ENABLED"));
				}
				output_rows.push_back(value_rows);
				value_rows.clear();
				for (int i = 0; i < mem_cur_size; i++) {
					value_rows.push_back(string_format(",%d,%s,%s,%s,%s,%s", i,
													   mem_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   mem_chiplet["clk"][i].val),
													   mem_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   mem_chiplet["min_clk"][i].val),
													   mem_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   mem_chiplet["max_clk"][i].val),
													   "N/A",
													   mem_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : mem_chiplet["deep_sleep"][i].val ?
													   "DISABLED" : "ENABLED"));
				}
				output_rows.push_back(value_rows);
			} else {
				out.append(metricClockMeasureHostHeaderTemplate);
				for (int i = 0; i < gfx_cur_size; i++) {
					out.append(string_format(metricChipletGfxClockMeasureHostTemplate, i,
											 gfx_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 gfx_chiplet["clk"][i].val),
											 gfx_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 gfx_chiplet["min_clk"][i].val),
											 gfx_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 gfx_chiplet["max_clk"][i].val),
											 gfx_chiplet["clk_locked"][i].val == UINT64_MAX ? "N/A" : gfx_chiplet["clk_locked"][i].val ?
											 "ENABLED" : "DISABLED",
											 gfx_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : gfx_chiplet["deep_sleep"][i].val ?
											 "DISABLED" : "ENABLED"));
				}
				for (int i = 0; i < mem_cur_size; i++) {
					out.append(string_format(metricChipletMemClockMeasureHostTemplate, i,
											 mem_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 mem_chiplet["clk"][i].val),
											 mem_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 mem_chiplet["min_clk"][i].val),
											 mem_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 mem_chiplet["max_clk"][i].val),
											 "N/A",
											 mem_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : mem_chiplet["deep_sleep"][i].val ?
											 "DISABLED" : "ENABLED"));
				}
			}
		}
	}

	if (setup_template) {
		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_GFX,
										 &clock_measure_gfx);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			gfx_clk = "N/A";
			gfx_max_clk = "N/A";
			gfx_min_clk = "N/A";
			is_clk_gfx_locked = "N/A";
			gfx_clk_deep_sleep = "N/A";
		} else {
			gfx_clk = string_format(
						  "%ld", clock_measure_gfx.clk);
			gfx_max_clk = string_format(
							  "%ld", clock_measure_gfx.max_clk);
			gfx_min_clk = string_format(
							  "%ld", clock_measure_gfx.min_clk);
			if (clock_measure_gfx.clk_locked != AMDSMI_NOT_SUPP_UINT8_RETVAL) {
				is_clk_gfx_locked = string_format("%s", clock_measure_gfx.clk_locked ? "ENABLED" : "DISABLED");
			}
			if (clock_measure_gfx.clk_deep_sleep != AMDSMI_NOT_SUPP_UINT8_RETVAL) {
				gfx_clk_deep_sleep = string_format("%s", clock_measure_gfx.clk_deep_sleep ? "ENABLED" : "DISABLED");
			}
		}

		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_MEM,
										 &clock_measure_mem);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			mem_clk = "N/A";
			mem_max_clk = "N/A";
			mem_min_clk = "N/A";
			mem_clk_deep_sleep = "N/A";
		} else {
			mem_clk = string_format(
						  "%ld", clock_measure_mem.clk);
			mem_max_clk = string_format(
							  "%ld", clock_measure_mem.max_clk);
			mem_min_clk = string_format(
							  "%ld", clock_measure_mem.min_clk);
			if (clock_measure_mem.clk_deep_sleep != AMDSMI_NOT_SUPP_UINT8_RETVAL) {
				mem_clk_deep_sleep = string_format("%s", clock_measure_mem.clk_deep_sleep ? "ENABLED" : "DISABLED");
			}
		}
		if (arg.output == json) {
			nlohmann::ordered_json gfx_clk_json{};
			gfx_clk_json["value"] = clock_measure_gfx.clk ;
			gfx_clk_json["unit"] = gfx_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json gfx_min_clk_json{};
			gfx_min_clk_json["value"] = clock_measure_gfx.min_clk;
			gfx_min_clk_json["unit"] = gfx_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json gfx_max_clk_json{};
			gfx_max_clk_json["value"] = clock_measure_gfx.max_clk;
			gfx_max_clk_json["unit"] = gfx_max_clk == "N/A" ? "N/A" : "MHz";
			result["gfx"] = nlohmann::ordered_json::object( {
				{"clk", gfx_clk_json },
				{ "min_clk", gfx_min_clk_json },
				{ "max_clk", gfx_max_clk_json },
				{ "clk_locked", is_clk_gfx_locked.c_str() },
				{ "deep_sleep", gfx_clk_deep_sleep.c_str() }});

			nlohmann::ordered_json mem_clk_json{};
			mem_clk_json["value"] = clock_measure_mem.clk;
			mem_clk_json["unit"] = mem_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json mem_min_clk_json{};
			mem_min_clk_json["value"] = clock_measure_mem.min_clk;
			mem_min_clk_json["unit"] = mem_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json mem_max_clk_json{};
			mem_max_clk_json["value"] = clock_measure_mem.max_clk;
			mem_max_clk_json["unit"] = mem_max_clk == "N/A" ? "N/A" : "MHz";
			result["mem"] = nlohmann::ordered_json::object( {
				{"clk", mem_clk_json },
				{ "min_clk", mem_min_clk_json },
				{ "max_clk", mem_max_clk_json },
				{ "clk_locked", is_clk_mem_locked.c_str() },
				{ "deep_sleep", mem_clk_deep_sleep.c_str() }});
		} else if (arg.output == csv) {
			out.append(string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",gfx_clk.c_str(),gfx_min_clk.c_str(),
									 gfx_max_clk.c_str(), is_clk_gfx_locked.c_str(),
									 gfx_clk_deep_sleep.c_str(), mem_clk.c_str(),
									 mem_min_clk.c_str(), mem_max_clk.c_str(),
									 is_clk_mem_locked.c_str(), mem_clk_deep_sleep.c_str()));
		} else {
			std::string gfx_clk_unit = gfx_clk == "N/A" ? "" : "MHz";
			std::string gfx_min_clk_unit = gfx_min_clk == "N/A" ? "" : "MHz";
			std::string gfx_max_clk_unit = gfx_max_clk == "N/A" ? "" : "MHz";

			std::string mem_clk_unit = mem_clk == "N/A" ? "" : "MHz";
			std::string mem_min_clk_unit = mem_min_clk == "N/A" ? "" : "MHz";
			std::string mem_max_clk_unit = mem_max_clk == "N/A" ? "" : "MHz";
			out.append(string_format(metricClockMeasureHostTemplate, gfx_clk.c_str(),
									 gfx_clk_unit.c_str(),
									 gfx_min_clk.c_str(), gfx_min_clk_unit.c_str(), gfx_max_clk.c_str(), gfx_max_clk_unit.c_str(),
									 is_clk_gfx_locked.c_str(), gfx_clk_deep_sleep.c_str(),
									 mem_clk.c_str(),mem_clk_unit.c_str(), mem_min_clk.c_str(), mem_min_clk_unit.c_str(),
									 mem_max_clk.c_str(), mem_max_clk_unit.c_str(),
									 is_clk_mem_locked.c_str(), mem_clk_deep_sleep.c_str()));
		}
	}

	if (metric_table) {
		std::vector<std::size_t> vclk_chiplet_size = {vclk_chiplet["clk"].size(), vclk_chiplet["min_clk"].size(), vclk_chiplet["max_clk"].size(),
													  vclk_chiplet["deep_sleep"].size()
													 };
		std::size_t vclk_cur_size = vclk_chiplet_size[0];
		if (std::any_of(vclk_chiplet_size.cbegin(), vclk_chiplet_size.cend(), [&](int i) {
		return i != vclk_cur_size;
	})) {
			setup_template = true;
		} else if (vclk_cur_size == 0) {
			setup_template = true;
		} else {
			setup_template = false;
			if (arg.output == json) {
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

					result[string_format("vclk_%d", i)] = nlohmann::ordered_json::object( {
						{"clk", vclk_clk_json },
						{"min_clk", vclk_min_clk_json },
						{"max_clk", vclk_max_clk_json },
						{"clk_locked", "N/A" },
						{"deep_sleep", vclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : vclk_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED" }});
				}
			} else if (arg.output == csv) {
				std::vector<std::string> value_rows{};
				for (int i = 0; i < vclk_cur_size; i++) {
					value_rows.push_back(string_format(",%d,%s,%s,%s,%s,%s", i,
													   vclk_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   vclk_chiplet["clk"][i].val),
													   vclk_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   vclk_chiplet["min_clk"][i].val),
													   vclk_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   vclk_chiplet["max_clk"][i].val),
													   "N/A",
													   vclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : mem_chiplet["deep_sleep"][i].val ?
													   "DISABLED" : "ENABLED"));
				}
				output_rows.push_back(value_rows);
			} else {
				for (int i = 0; i < vclk_cur_size; i++) {
					out.append(string_format(metricChipletVCLKClockMeasureHostTemplate, i,
											 vclk_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 vclk_chiplet["clk"][i].val),
											 vclk_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 vclk_chiplet["min_clk"][i].val),
											 vclk_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 vclk_chiplet["max_clk"][i].val),
											 "N/A",
											 vclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : vclk_chiplet["deep_sleep"][i].val
											 ?
											 "DISABLED" : "ENABLED"));
				}
			}
		}
	}

	if (setup_template) {
		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK0,
										 &clock_measure_vclk0);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			vclk0_clk = "N/A";
			vclk0_max_clk = "N/A";
			vclk0_min_clk = "N/A";
			vclk0_clk_deep_sleep = "N/A";
		} else {
			vclk0_clk = clock_measure_vclk0.clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk0.clk);
			vclk0_max_clk = clock_measure_vclk0.max_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk0.max_clk);
			vclk0_min_clk = clock_measure_vclk0.min_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk0.min_clk);

			if (clock_measure_vclk0.clk_deep_sleep != AMDSMI_NOT_SUPP_UINT8_RETVAL) {
				vclk0_clk_deep_sleep = string_format(
										   "%s", clock_measure_vclk0.clk_deep_sleep ? "ENABLED" : "DISABLED");
			}
		}

		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK1,
										 &clock_measure_vclk1);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			vclk1_clk = "N/A";
			vclk1_max_clk = "N/A";
			vclk1_min_clk = "N/A";
			vclk1_clk_deep_sleep = "N/A";
		} else {
			vclk1_clk = clock_measure_vclk1.clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk1.clk);
			vclk1_max_clk = clock_measure_vclk1.max_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk1.max_clk);
			vclk1_min_clk = clock_measure_vclk1.min_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_vclk1.min_clk);

			if (clock_measure_vclk1.clk_deep_sleep != AMDSMI_NOT_SUPP_UINT8_RETVAL) {
				vclk1_clk_deep_sleep = string_format(
										   "%s", clock_measure_vclk1.clk_deep_sleep ? "ENABLED" : "DISABLED");
			}
		}

		if (arg.output == json) {
			nlohmann::ordered_json vclk0_clk_json{};
			vclk0_clk_json["value"] = vclk0_clk;
			vclk0_clk_json["unit"] = vclk0_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk0_min_clk_json{};
			vclk0_min_clk_json["value"] = vclk0_min_clk;
			vclk0_min_clk_json["unit"] = vclk0_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk0_max_clk_json{};
			vclk0_max_clk_json["value"] = vclk0_max_clk;
			vclk0_max_clk_json["unit"] = vclk0_max_clk == "N/A" ? "N/A" : "MHz";
			result["vclk0"] = nlohmann::ordered_json::object( {
				{"clk", vclk0_clk_json },
				{ "min_clk", vclk0_min_clk_json },
				{ "max_clk", vclk0_max_clk_json },
				{ "clk_locked", is_clk_vclk0_locked.c_str() },
				{ "deep_sleep", vclk0_clk_deep_sleep.c_str() }});
			nlohmann::ordered_json vclk1_clk_json{};
			vclk1_clk_json["value"] =  vclk1_clk;
			vclk1_clk_json["unit"] = vclk1_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk1_min_clk_json{};
			vclk1_min_clk_json["value"] = vclk1_min_clk;
			vclk1_min_clk_json["unit"] = vclk1_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk1_max_clk_json{};
			vclk1_max_clk_json["value"] = vclk1_max_clk;
			vclk1_max_clk_json["unit"] = vclk1_max_clk == "N/A" ? "N/A" : "MHz";
			result["vclk1"] = nlohmann::ordered_json::object( {
				{"clk", vclk1_clk_json },
				{ "min_clk", vclk1_min_clk_json },
				{ "max_clk", vclk1_max_clk_json },
				{ "clk_locked", is_clk_vclk1_locked.c_str() },
				{ "deep_sleep", vclk1_clk_deep_sleep.c_str() }});
		} else if (arg.output == csv) {
			out.append(string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",vclk0_clk.c_str(),
									 vclk0_min_clk.c_str(),
									 vclk0_max_clk.c_str(),
									 is_clk_vclk0_locked.c_str(), vclk0_clk_deep_sleep.c_str(),
									 vclk1_clk.c_str(), vclk1_min_clk.c_str(), vclk1_max_clk.c_str(), is_clk_vclk1_locked.c_str(),
									 vclk1_clk_deep_sleep.c_str()));
		} else {
			std::string vclk0_clk_unit = vclk0_clk == "N/A" ? "" : " MHz";
			std::string vclk0_max_clk_unit = vclk0_max_clk == "N/A" ? "" : " MHz";
			std::string vclk0_min_clk_unit = vclk0_min_clk == "N/A" ? "" : " MHz";

			std::string vclk1_clk_unit = vclk1_clk == "N/A" ? "" : " MHz";
			std::string vclk1_max_clk_unit = vclk1_max_clk == "N/A" ? "" : " MHz";
			std::string vclk1_min_clk_unit = vclk1_min_clk == "N/A" ? "" : " MHz";
			out.append(string_format(metricVCLK0ClockMeasureHostTemplate, vclk0_clk.c_str(),
									 vclk0_clk_unit.c_str(),
									 vclk0_min_clk.c_str(), vclk0_min_clk_unit.c_str(), vclk0_max_clk.c_str(),
									 vclk0_max_clk_unit.c_str(),
									 is_clk_vclk0_locked.c_str(), vclk0_clk_deep_sleep.c_str()));
			out.append(string_format(metricVCLK1ClockMeasureHostTemplate, vclk1_clk.c_str(),
									 vclk1_clk_unit.c_str(),
									 vclk1_min_clk.c_str(),vclk1_min_clk_unit.c_str(),vclk1_max_clk.c_str(),vclk1_max_clk_unit.c_str(),
									 is_clk_vclk1_locked.c_str(), vclk1_clk_deep_sleep.c_str()));
		}
	}

	if (metric_table) {
		std::vector<std::size_t> dclk_chiplet_size = {dclk_chiplet["clk"].size(), dclk_chiplet["min_clk"].size(), dclk_chiplet["max_clk"].size(),
													  dclk_chiplet["deep_sleep"].size()
													 };
		std::size_t dclk_cur_size = dclk_chiplet_size[0];
		if (std::any_of(dclk_chiplet_size.cbegin(), dclk_chiplet_size.cend(), [&](int i) {
		return i != dclk_cur_size;
	})) {
			setup_template = true;
		} else if (dclk_cur_size == 0) {
			setup_template = true;
		} else {
			setup_template = false;
			if (arg.output == json) {
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
					result[string_format("dclk_%d",i)] = nlohmann::ordered_json::object( {
						{"clk", dclk_clk_json },
						{"min_clk", dclk_min_clk_json },
						{"max_clk", dclk_max_clk_json },
						{"clk_locked", "N/A" },
						{"deep_sleep", dclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : dclk_chiplet["deep_sleep"][i].val ? "DISABLED" : "ENABLED" }});
				}
			} else if (arg.output == csv) {
				std::vector<std::string> value_rows{};
				for (int i = 0; i < dclk_cur_size; i++) {
					value_rows.push_back(string_format(",%d,%s,%s,%s,%s,%s", i,
													   dclk_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   dclk_chiplet["clk"][i].val),
													   dclk_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   dclk_chiplet["min_clk"][i].val),
													   dclk_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu",
														   dclk_chiplet["max_clk"][i].val),
													   "N/A",
													   dclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : mem_chiplet["deep_sleep"][i].val ?
													   "DISABLED" : "ENABLED"));
				}
				output_rows.push_back(value_rows);
			} else {
				for (int i = 0; i < dclk_cur_size; i++) {
					out.append(string_format(metricChipletDCLKClockMeasureHostTemplate, i,
											 dclk_chiplet["clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 dclk_chiplet["clk"][i].val),
											 dclk_chiplet["min_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 dclk_chiplet["min_clk"][i].val),
											 dclk_chiplet["max_clk"][i].val == UINT64_MAX ? "N/A" : string_format("%llu MHz",
												 dclk_chiplet["max_clk"][i].val),
											 "N/A",
											 dclk_chiplet["deep_sleep"][i].val == UINT64_MAX ? "N/A" : dclk_chiplet["deep_sleep"][i].val
											 ?
											 "DISABLED" : "ENABLED"));
				}
			}
		}
	}

	if (setup_template) {
		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_DCLK0,
										 &clock_measure_dclk0);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			dclk0_clk = "N/A";
			dclk0_max_clk = "N/A";
			dclk0_min_clk = "N/A";
			dclk0_clk_deep_sleep = "N/A";
		} else {
			dclk0_clk = clock_measure_dclk0.clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_dclk0.clk);
			dclk0_max_clk = clock_measure_dclk0.max_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_dclk0.max_clk);
			dclk0_min_clk = clock_measure_dclk0.min_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_dclk0.min_clk);

			if (clock_measure_dclk0.clk_deep_sleep != AMDSMI_NOT_SUPP_UINT8_RETVAL) {
				dclk0_clk_deep_sleep = string_format(
										   "%s", clock_measure_dclk0.clk_deep_sleep ? "ENABLED" : "DISABLED");
			}
		}

		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_DCLK1,
										 &clock_measure_dclk1);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			dclk1_clk = "N/A";
			dclk1_max_clk = "N/A";
			dclk1_min_clk = "N/A";
			dclk1_clk_deep_sleep = "N/A";
		} else {
			dclk1_clk = clock_measure_dclk1.clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_dclk1.clk);
			dclk1_max_clk = clock_measure_dclk1.max_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_dclk1.max_clk);
			dclk1_min_clk = clock_measure_dclk1.min_clk == UINT_MAX ? "N/A" : string_format("%ld", clock_measure_dclk1.min_clk);

			if (clock_measure_dclk1.clk_deep_sleep != AMDSMI_NOT_SUPP_UINT8_RETVAL) {
				dclk1_clk_deep_sleep = string_format(
										   "%s", clock_measure_dclk1.clk_deep_sleep ? "ENABLED" : "DISABLED");
			}
		}

		if (arg.output == json) {
			nlohmann::ordered_json dclk0_clk_json{};
			dclk0_clk_json["value"] = dclk0_clk;
			dclk0_clk_json["unit"] = dclk0_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk0_min_clk_json{};
			dclk0_min_clk_json["value"] = dclk0_min_clk;
			dclk0_min_clk_json["unit"] = dclk0_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk0_max_clk_json{};
			dclk0_max_clk_json["value"] = dclk0_max_clk;
			dclk0_max_clk_json["unit"] = dclk0_max_clk == "N/A" ? "N/A" : "MHz";
			result["dclk0"] = nlohmann::ordered_json::object( {
				{"clk", dclk0_clk_json },
				{ "min_clk", dclk0_min_clk_json },
				{ "max_clk", dclk0_max_clk_json },
				{ "clk_locked", is_clk_dclk0_locked.c_str() },
				{ "deep_sleep", dclk0_clk_deep_sleep.c_str() }});
			nlohmann::ordered_json dclk1_clk_json{};
			dclk1_clk_json["value"] = dclk1_clk;
			dclk1_clk_json["unit"] = dclk1_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk1_min_clk_json{};
			dclk1_min_clk_json["value"] = dclk1_min_clk;
			dclk1_min_clk_json["unit"] = dclk1_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk1_max_clk_json{};
			dclk1_max_clk_json["value"] = dclk1_max_clk;
			dclk1_max_clk_json["unit"] = dclk1_max_clk == "N/A" ? "N/A" : "MHz";
			result["dclk1"] = nlohmann::ordered_json::object( {
				{"clk", dclk1_clk_json },
				{ "min_clk", dclk1_min_clk_json },
				{ "max_clk", dclk1_max_clk_json },
				{ "clk_locked", is_clk_dclk1_locked.c_str() },
				{ "deep_sleep", dclk1_clk_deep_sleep.c_str() }});
		} else if (arg.output == csv) {
			out.append(string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",dclk0_clk.c_str(),
									 dclk0_min_clk.c_str(),
									 dclk0_max_clk.c_str(),
									 is_clk_dclk0_locked.c_str(), dclk0_clk_deep_sleep.c_str(),
									 dclk1_clk.c_str(), dclk1_min_clk.c_str(), dclk1_max_clk.c_str(), is_clk_dclk1_locked.c_str(),
									 dclk1_clk_deep_sleep.c_str()));
		} else {
			std::string dclk0_clk_unit = dclk0_clk == "N/A" ? "" : " MHz";
			std::string dclk0_min_clk_unit = dclk0_min_clk == "N/A" ? "" : " MHz";
			std::string dclk0_max_clk_unit = dclk0_max_clk == "N/A" ? "" : " MHz";

			std::string dclk1_clk_unit = dclk1_clk == "N/A" ? "" : " MHz";
			std::string dclk1_min_clk_unit = dclk1_min_clk == "N/A" ? "" : " MHz";
			std::string dclk1_max_clk_unit = dclk1_max_clk == "N/A" ? "" : " MHz";
			out.append(string_format(metricDCLK0ClockMeasureHostTemplate, dclk0_clk.c_str(),
									 dclk0_clk_unit.c_str(),
									 dclk0_min_clk.c_str(), dclk0_min_clk_unit.c_str(),dclk0_max_clk.c_str(), dclk0_max_clk_unit.c_str(),
									 is_clk_dclk0_locked.c_str(),
									 dclk0_clk_deep_sleep.c_str()));
			out.append(string_format(metricDCLK1ClockMeasureHostTemplate, dclk1_clk.c_str(),
									 dclk1_clk_unit.c_str(),
									 dclk1_min_clk.c_str(),dclk1_min_clk_unit.c_str(),dclk1_max_clk.c_str(), dclk1_max_clk_unit.c_str(),
									 is_clk_dclk1_locked.c_str(),
									 dclk1_clk_deep_sleep.c_str()));
		}
	}

	if (metric_table) {
		if (arg.output == json) {
			out = result.dump(4);
		} else if (arg.output == csv) {
			csv_recursion(out, output_rows);
		}
	} else  {
		if (arg.watch > -1) {
			out = string_format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
								gfx_clk.c_str(), gfx_min_clk.c_str(), gfx_max_clk.c_str(), is_clk_gfx_locked.c_str(),
								gfx_clk_deep_sleep.c_str(),
								mem_clk.c_str(), mem_min_clk.c_str(), mem_max_clk.c_str(), is_clk_mem_locked.c_str(),
								mem_clk_deep_sleep.c_str(),
								vclk0_clk.c_str(), vclk0_min_clk.c_str(), vclk0_max_clk.c_str(), is_clk_vclk0_locked.c_str(),
								vclk0_clk_deep_sleep.c_str(),
								vclk1_clk.c_str(), vclk1_min_clk.c_str(), vclk1_max_clk.c_str(), is_clk_vclk1_locked.c_str(),
								vclk1_clk_deep_sleep.c_str(),
								dclk0_clk.c_str(), dclk0_min_clk.c_str(), dclk0_max_clk.c_str(), is_clk_dclk0_locked.c_str(),
								dclk0_clk_deep_sleep.c_str(),
								dclk1_clk.c_str(), dclk1_min_clk.c_str(), dclk1_max_clk.c_str(), is_clk_dclk1_locked.c_str(),
								dclk1_clk_deep_sleep.c_str());
		} else if (arg.output == json) {
			nlohmann::ordered_json values_json{};
			nlohmann::ordered_json gfx_clk_json{};
			gfx_clk_json["value"] = gfx_clk;
			gfx_clk_json["unit"] = gfx_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json gfx_min_clk_json{};
			gfx_min_clk_json["value"] = gfx_min_clk;
			gfx_min_clk_json["unit"] = gfx_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json gfx_max_clk_json{};
			gfx_max_clk_json["value"] = gfx_max_clk;
			gfx_max_clk_json["unit"] = gfx_max_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json gfx = { { "clk", gfx_clk_json },
				{ "min_clk", gfx_min_clk_json },
				{ "max_clk", gfx_max_clk_json },
				{ "clk_locked", is_clk_gfx_locked.c_str() },
				{ "deep_sleep", gfx_clk_deep_sleep.c_str() }
			};

			nlohmann::ordered_json mem_clk_json{};
			mem_clk_json["value"] = mem_clk;
			mem_clk_json["unit"] = mem_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json mem_min_clk_json{};
			mem_min_clk_json["value"] = mem_min_clk;
			mem_min_clk_json["unit"] = mem_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json mem_max_clk_json{};
			mem_max_clk_json["value"] =  mem_max_clk;
			mem_max_clk_json["unit"] = mem_max_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json mem = { { "clk", mem_clk_json },
				{ "min_clk", mem_min_clk_json },
				{ "max_clk", mem_max_clk_json },
				{ "clk_locked", is_clk_mem_locked.c_str() },
				{ "deep_sleep", mem_clk_deep_sleep.c_str() }
			};

			nlohmann::ordered_json vclk0_clk_json{};
			vclk0_clk_json["value"] = vclk0_clk;
			vclk0_clk_json["unit"] = vclk0_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk0_min_clk_json{};
			vclk0_min_clk_json["value"] = vclk0_min_clk;
			vclk0_min_clk_json["unit"] = vclk0_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk0_max_clk_json{};
			vclk0_max_clk_json["value"] = vclk0_max_clk;
			vclk0_max_clk_json["unit"] = vclk0_max_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk0 = { { "clk", vclk0_clk_json },
				{ "min_clk", vclk0_min_clk_json },
				{ "max_clk", vclk0_max_clk_json },
				{ "clk_locked", is_clk_vclk0_locked.c_str() },
				{ "deep_sleep", vclk0_clk_deep_sleep.c_str() }
			};

			nlohmann::ordered_json vclk1_clk_json{};
			vclk1_clk_json["value"] = vclk1_clk;
			vclk1_clk_json["unit"] = vclk1_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk1_min_clk_json{};
			vclk1_min_clk_json["value"] = vclk1_min_clk;
			vclk1_min_clk_json["unit"] = vclk1_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk1_max_clk_json{};
			vclk1_max_clk_json["value"] = vclk1_max_clk;
			vclk1_max_clk_json["unit"] = vclk1_max_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json vclk1 = { { "clk", vclk1_clk_json },
				{ "min_clk", vclk1_min_clk_json },
				{ "max_clk", vclk1_max_clk_json },
				{ "clk_locked", is_clk_vclk1_locked.c_str() },
				{ "deep_sleep", vclk1_clk_deep_sleep.c_str() }
			};

			nlohmann::ordered_json dclk0_clk_json{};
			dclk0_clk_json["value"] = dclk0_clk;
			dclk0_clk_json["unit"] = dclk0_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk0_min_clk_json{};
			dclk0_min_clk_json["value"] = dclk0_min_clk;
			dclk0_min_clk_json["unit"] = dclk0_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk0_max_clk_json{};
			dclk0_max_clk_json["value"] = dclk0_max_clk;
			dclk0_max_clk_json["unit"] = dclk0_max_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk0 = { { "clk", dclk0_clk_json },
				{ "min_clk", dclk0_min_clk_json },
				{ "max_clk", dclk0_max_clk_json },
				{ "clk_locked", is_clk_dclk0_locked.c_str()},
				{ "deep_sleep", dclk0_clk_deep_sleep.c_str() }
			};

			nlohmann::ordered_json dclk1_clk_json{};
			dclk1_clk_json["value"] = dclk1_clk;
			dclk1_clk_json["unit"] = dclk1_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk1_min_clk_json{};
			dclk1_min_clk_json["value"] = dclk1_min_clk;
			dclk1_min_clk_json["unit"] = dclk1_min_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk1_max_clk_json{};
			dclk1_max_clk_json["value"] = dclk1_max_clk;
			dclk1_max_clk_json["unit"] = dclk1_max_clk == "N/A" ? "N/A" : "MHz";
			nlohmann::ordered_json dclk1 = { { "clk", dclk1_clk_json },
				{ "min_clk", dclk1_min_clk_json },
				{ "max_clk", dclk1_max_clk_json },
				{ "clk_locked", is_clk_dclk1_locked.c_str()},
				{ "deep_sleep", dclk1_clk_deep_sleep.c_str() }
			};

			nlohmann::ordered_json clock = { { "gfx", gfx }, { "mem", mem }, { "vclk0", vclk0 }, { "vclk1", vclk1 }, { "dclk0", dclk0 }, { "dclk1", dclk1 }
			};
			out = clock.dump(4);
		} else if (arg.output == csv) {
			out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
								gfx_clk.c_str(), gfx_min_clk.c_str(), gfx_max_clk.c_str(), is_clk_gfx_locked.c_str(),
								gfx_clk_deep_sleep.c_str(),
								mem_clk.c_str(), mem_min_clk.c_str(), mem_max_clk.c_str(), is_clk_mem_locked.c_str(),
								mem_clk_deep_sleep.c_str(),
								vclk0_clk.c_str(), vclk0_min_clk.c_str(), vclk0_max_clk.c_str(), is_clk_vclk0_locked.c_str(),
								vclk0_clk_deep_sleep.c_str(),
								vclk1_clk.c_str(), vclk1_min_clk.c_str(), vclk1_max_clk.c_str(), is_clk_vclk1_locked.c_str(),
								vclk1_clk_deep_sleep.c_str(),
								dclk0_clk.c_str(), dclk0_min_clk.c_str(), dclk0_max_clk.c_str(), is_clk_dclk0_locked.c_str(),
								dclk0_clk_deep_sleep.c_str(),
								dclk1_clk.c_str(), dclk1_min_clk.c_str(), dclk1_max_clk.c_str(), is_clk_dclk1_locked.c_str(),
								dclk1_clk_deep_sleep.c_str());
		} else {
			std::string gfx_clk_unit = gfx_clk == "N/A" ? "" : "MHz";
			std::string gfx_min_clk_unit = gfx_min_clk == "N/A" ? "" : "MHz";
			std::string gfx_max_clk_unit = gfx_max_clk == "N/A" ? "" : "MHz";

			std::string mem_clk_unit = mem_clk == "N/A" ? "" : "MHz";
			std::string mem_min_clk_unit = mem_min_clk == "N/A" ? "" : "MHz";
			std::string mem_max_clk_unit = mem_max_clk == "N/A" ? "" : "MHz";

			std::string vclk0_clk_unit = vclk0_clk == "N/A" ? "" : "MHz";
			std::string vclk0_max_clk_unit = vclk0_max_clk == "N/A" ? "" : "MHz";
			std::string vclk0_min_clk_unit = vclk0_min_clk == "N/A" ? "" : "MHz";

			std::string vclk1_clk_unit = vclk1_clk == "N/A" ? "" : "MHz";
			std::string vclk1_max_clk_unit = vclk1_max_clk == "N/A" ? "" : "MHz";
			std::string vclk1_min_clk_unit = vclk1_min_clk == "N/A" ? "" : "MHz";

			std::string dclk0_clk_unit = dclk0_clk == "N/A" ? "" : "MHz";
			std::string dclk0_min_clk_unit = dclk0_min_clk == "N/A" ? "" : "MHz";
			std::string dclk0_max_clk_unit = dclk0_max_clk == "N/A" ? "" : "MHz";

			std::string dclk1_clk_unit = dclk1_clk == "N/A" ? "" : "MHz";
			std::string dclk1_min_clk_unit = dclk1_min_clk == "N/A" ? "" : "MHz";
			std::string dclk1_max_clk_unit = dclk1_max_clk == "N/A" ? "" : "MHz";
			out = string_format(
					  metricClockMeasureHostTemplate, gfx_clk.c_str(), gfx_clk_unit.c_str(),
					  gfx_min_clk.c_str(), gfx_min_clk_unit.c_str(), gfx_max_clk.c_str(), gfx_max_clk_unit.c_str(),
					  is_clk_gfx_locked.c_str(), gfx_clk_deep_sleep.c_str(),
					  mem_clk.c_str(),mem_clk_unit.c_str(), mem_min_clk.c_str(), mem_min_clk_unit.c_str(),
					  mem_max_clk.c_str(), mem_max_clk_unit.c_str(),
					  is_clk_mem_locked.c_str(), mem_clk_deep_sleep.c_str());

			out += string_format(
					   metricVCLK0ClockMeasureHostTemplate, vclk0_clk.c_str(), vclk0_clk_unit.c_str(),
					   vclk0_min_clk.c_str(), vclk0_min_clk_unit.c_str(), vclk0_max_clk.c_str(),
					   vclk0_max_clk_unit.c_str(),
					   is_clk_vclk0_locked.c_str(), vclk0_clk_deep_sleep.c_str());

			out += string_format(
					   metricVCLK1ClockMeasureHostTemplate, vclk1_clk.c_str(), vclk1_clk_unit.c_str(),
					   vclk1_min_clk.c_str(),vclk1_min_clk_unit.c_str(),vclk1_max_clk.c_str(),vclk1_max_clk_unit.c_str(),
					   is_clk_vclk1_locked.c_str(), vclk1_clk_deep_sleep.c_str());
			out += string_format(
					   metricDCLK0ClockMeasureHostTemplate, dclk0_clk.c_str(),dclk0_clk_unit.c_str(),
					   dclk0_min_clk.c_str(), dclk0_min_clk_unit.c_str(),dclk0_max_clk.c_str(), dclk0_max_clk_unit.c_str(),
					   is_clk_dclk0_locked.c_str(),
					   dclk0_clk_deep_sleep.c_str()
				   );
			out += string_format(
					   metricDCLK1ClockMeasureHostTemplate, dclk1_clk.c_str(),dclk1_clk_unit.c_str(),
					   dclk1_min_clk.c_str(),dclk1_min_clk_unit.c_str(),dclk1_max_clk.c_str(), dclk1_max_clk_unit.c_str(),
					   is_clk_dclk1_locked.c_str(),
					   dclk1_clk_deep_sleep.c_str()
				   );
		}
	}

	return AMDSMI_STATUS_SUCCESS;
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
	if (std::any_of(driver_version_str.begin(), driver_version_str.end(), [](char c) { return !std::isprint(c); })) {
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
