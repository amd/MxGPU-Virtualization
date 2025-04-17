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
#include "smi_cli_metric_command.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

#include "json/json.h"

#include <sstream>
#include <limits.h>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
		amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle,
		amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CLOCK_INFO)(amdsmi_processor_handle, amdsmi_clk_type_t,
		amdsmi_clk_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
		amdsmi_temperature_type_t,
		amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
		amdsmi_error_count_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VRAM_USAGE)(amdsmi_processor_handle,
		amdsmi_vram_usage_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF guest_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_ACTIVITY guest_amdsmi_get_gpu_activity;
extern AMDSMI_GET_POWER_INFO guest_amdsmi_get_power_info;
extern AMDSMI_GET_CLOCK_INFO guest_amdsmi_get_clock_info;
extern AMDSMI_GET_TEMP_METRIC guest_amdsmi_get_temp_metric;
extern AMDSMI_GET_GPU_TOTAL_ECC_COUNT guest_amdsmi_get_gpu_total_ecc_count;
extern AMDSMI_GET_PCIE_INFO guest_amdsmi_get_pcie_info;
extern AMDSMI_GET_GPU_DEVICE_BDF guest_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_PROCESSOR_HANDLES guest_amdsmi_get_processor_handles;
extern AMDSMI_GET_GPU_VRAM_USAGE guest_amdsmi_get_gpu_vram_usage;

std::string guest_fill_usage(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s",value.c_str(), value.c_str(),
							value.c_str());
	} else if (arg.output == json) {
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

std::string guest_fill_fb_usage(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.watch > -1) {
		out = string_format("%s,%s", value.c_str(), value.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json vram_total_json{};
		vram_total_json["value"] = value.c_str();
		vram_total_json["unit"] = "N/A";
		nlohmann::ordered_json vram_used_json{};
		vram_used_json["value"] = value.c_str();
		vram_used_json["unit"] = "N/A";
		nlohmann::ordered_json vram_usage_json = { { "fb_total", vram_total_json },
			{ "fb_used", vram_used_json },
		};

		out = vram_usage_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,", value.c_str(), value.c_str());
	} else {
		out = string_format(metricFbUsageTemplate, value.c_str(),"", value.c_str(), "");
	}
	return out;
}

std::string guest_fill_power(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s", value.c_str(),
							value.c_str(), value.c_str(), value.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json socket_power{};
		socket_power["value"] = value.c_str();
		socket_power["unit"] = "N/A";
		nlohmann::ordered_json gfx_voltage{};
		gfx_voltage["value"] = value.c_str();
		gfx_voltage["unit"] = "N/A";
		nlohmann::ordered_json soc_voltage{};
		soc_voltage["value"] = value.c_str();
		soc_voltage["unit"] = "N/A";
		nlohmann::ordered_json mem_voltage{};
		mem_voltage["value"] = value.c_str();
		mem_voltage["unit"] = "N/A";

		nlohmann::ordered_json power_info_json = { { "socket_power", socket_power } };
		power_info_json["voltage"] = gfx_voltage;
		power_info_json["soc_voltage"] = soc_voltage;
		power_info_json["mem_voltage"] = mem_voltage;

		out = power_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s", value.c_str(), value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(
				  metricGuestPowerMeasureTemplate, value.c_str(), "", value.c_str(), "", value.c_str(), "",
				  value.c_str(), "");
	}
	return out;
}

std::string guest_fill_clock(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
							value.c_str(), value.c_str(), value.c_str(), value.c_str(), "N/A",
							value.c_str(), value.c_str(), value.c_str(), value.c_str(), "N/A",
							value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							"N/A",
							value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							"N/A");
	} else if (arg.output == json) {
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
			{ "max_clk", gfx_max_clk_json },
			{ "min_clk", gfx_min_clk_json },
			{ "clk_locked", value.c_str() }
		};

		nlohmann::ordered_json mem = { { "clk", mem_clk_json },
			{ "max_clk", mem_max_clk_json },
			{ "min_clk", mem_min_clk_json },
			{ "clk_locked", value.c_str() }
		};

		nlohmann::ordered_json vclk0 = { { "clk", vclk0_clk_json },
			{ "max_clk", vclk0_max_clk_json },
			{ "min_clk", vclk0_min_clk_json },
			{ "clk_locked", value.c_str() }
		};

		nlohmann::ordered_json vclk1 = { { "clk", vclk1_clk_json },
			{ "max_clk", vclk1_max_clk_json },
			{ "min_clk", vclk1_min_clk_json },
			{ "clk_locked", value.c_str() }
		};
		nlohmann::ordered_json clock = { { "gfx", gfx }, { "mem", mem }, { "vclk0", vclk0 }, { "vclk1", vclk1 }
		};
		out = clock.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", value.c_str(),
							value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format( metricClockMeasureHostTemplate, value.c_str(), "", value.c_str(), "",
							 value.c_str(), "",
							 value.c_str(), value.c_str(), value.c_str(), "", value.c_str(), "", value.c_str(), "",
							 value.c_str(), value.c_str());

		out += string_format( metricVCLK0ClockMeasureHostTemplate, value.c_str(), "", value.c_str(), "",
							  value.c_str(), "", value.c_str(), value.c_str());

		out += string_format( metricVCLK1ClockMeasureHostTemplate, value.c_str(), "", value.c_str(), "",
							  value.c_str(), "", value.c_str(), value.c_str());
	}
	return out;
}

std::string guest_fill_temperature(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s", value.c_str(),
							value.c_str(), value.c_str());
	} else if (arg.output == json) {
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

std::string guest_fill_ecc(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s,%s", value.c_str(),
							value.c_str(), value.c_str(), "N/A", "N/A");
	} else if (arg.output == json) {
		nlohmann::ordered_json error_count_json = {
			{ "total_correctable_count", value.c_str() },
			{ "total_uncorrectable_count", value.c_str()},
			{ "total_deferred_count", value.c_str()},
			{ "cache_correctable_count", value.c_str() },
			{ "cache_uncorrectable_count", value.c_str() }
		};

		out = error_count_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format( ",%s,%s,%s,%s,%s", value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							 value.c_str());
	} else {
		out = string_format(metricEccErrorCountTemplate, value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str());
	}
	return out;
}

std::string guest_fill_pcie(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.watch > -1) {
		out = string_format("%s,%s", value.c_str(),
							value.c_str());
	} else if (arg.output == json) {
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
			{ "pcie_replay_count", value.c_str() },
			{ "pcie_l0_to_recovery_count", value.c_str() },
			{ "pcie_replay_roll_over_count", value.c_str() },
			{ "pcie_nak_sent_count", value.c_str() },
			{ "pcie_nak_received_count", value.c_str() }
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



int AmdSmiApiGuest::amdsmi_get_usage_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_engine_usage_t engine_usage;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		out = guest_fill_usage(arg, "N/A");
		return ret;
	}

	ret = guest_amdsmi_get_gpu_activity(processor, &engine_usage);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		out = guest_fill_usage(arg, "N/A");
		return ret;
	}

	std::string mm_activity = (engine_usage.mm_activity == INT32_MAX) ? "N/A" :
							  string_format(
								  "%ld", engine_usage.mm_activity);
	std::string gfx_activity{ string_format(
								  "%ld", engine_usage.gfx_activity) };
	std::string umc_activity{ string_format(
								  "%ld", engine_usage.umc_activity) };

	if (arg.watch > -1) {
		out = string_format("%s,%s,%s",gfx_activity.c_str(), umc_activity.c_str(),
							mm_activity.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
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
			mm_activity["value"] = mm_activity;
		} else {
			mm_activity["value"] = engine_usage.mm_activity;
		}
		mm_activity["unit"] = mm_activity == "N/A" ? "N/A" : "%";
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
							gfx_activity.c_str(), umc_activity.c_str(), mm_activity.c_str(), "N/A", "N/A", "N/A", "N/A");
	} else {
		std::string gfx_activity_unit = gfx_activity == "N/A" ? "" : "%";
		std::string umc_activity_unit = umc_activity == "N/A" ? "" : "%";
		std::string mm_activity_unit = mm_activity == "N/A" ? "" : "%";
		out = string_format(
				  metricUsageTemplate, gfx_activity.c_str(), gfx_activity_unit.c_str(), umc_activity.c_str(),
				  umc_activity_unit.c_str(),
				  mm_activity.c_str(), mm_activity_unit.c_str());
		out.append(metricVcnUsageHeaderTemplate);
		out.append(string_format(metricVcnUsageTemplate, "N/A", "N/A"));
		out.append(metricJpegUsageHeaderTemplate);
		out.append(string_format(metricJpegUsageTemplate, "N/A", "N/A"));
		out.append(metricJpegUsageFooterTemplate);
	}

	return ret;

}

int AmdSmiApiGuest::amdsmi_get_power_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	if (AmdSmiPlatform::getInstance().is_guest()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}

	amdsmi_status_t ret;

	amdsmi_power_info_t power_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_power(arg, "N/A");
		return ret;
	}

	ret = guest_amdsmi_get_power_info(processor, &power_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_power(arg, "N/A");
		return ret;
	}

	std::string voltage_str = (power_info.gfx_voltage == UINT_MAX) ? "N/A" :
							  string_format(
								  "%ld", power_info.gfx_voltage);
	std::string soc_voltage_str = (power_info.soc_voltage == UINT_MAX) ? "N/A" :
								  string_format(
									  "%ld", power_info.soc_voltage);
	std::string mem_voltage_str = (power_info.mem_voltage == UINT_MAX) ? "N/A" :
								  string_format(
									  "%ld", power_info.mem_voltage);
	std::string socket_power_str{ string_format(
									  "%ld", power_info.socket_power) };

	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s", socket_power_str.c_str(),
							voltage_str.c_str(), soc_voltage_str.c_str(), mem_voltage_str.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
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
		power_info_json["voltage"] = gfx_voltage;

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

		out = power_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s", socket_power_str.c_str(),
				  voltage_str.c_str(), soc_voltage_str.c_str(), mem_voltage_str.c_str());
	} else {
		std::string socket_power_unit = socket_power_str == "N/A" ? "" : "W";
		std::string voltage_unit = voltage_str == "N/A" ? "" : "mV";
		std::string soc_voltage_unit = soc_voltage_str == "N/A" ? "" : "mV";
		std::string mem_voltage_unit = mem_voltage_str == "N/A" ? "" : "mV";
		out = string_format(
				  metricGuestPowerMeasureTemplate, socket_power_str.c_str(), socket_power_unit.c_str(),
				  voltage_str.c_str(), voltage_unit.c_str(), soc_voltage_str.c_str(),
				  soc_voltage_unit.c_str(), mem_voltage_str.c_str(), mem_voltage_unit.c_str());
	}
	return ret;
}


int AmdSmiApiGuest::amdsmi_get_clock_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	if (AmdSmiPlatform::getInstance().is_guest()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}

	amdsmi_status_t ret;
	amdsmi_clk_info_t clock_measure_gfx, clock_measure_mem;
	amdsmi_clk_info_t clock_measure_vclk0, clock_measure_vclk1;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_clock(arg, "N/A");
		return ret;
	}

	std::string gfx_clk{};
	std::string gfx_max_clk{};
	std::string gfx_min_clk{};
	ret = guest_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_GFX,
									  &clock_measure_gfx);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		gfx_clk = "N/A";
		gfx_max_clk = "N/A";
		gfx_min_clk = "N/A";
	} else {
		gfx_clk = string_format(
					  "%ld", clock_measure_gfx.clk);
		gfx_max_clk = string_format(
						  "%ld", clock_measure_gfx.max_clk);
		gfx_min_clk = clock_measure_gfx.min_clk == -1 ? "N/A" : string_format("%ld",
					  clock_measure_gfx.min_clk);
	}
	std::string mem_clk{};
	std::string mem_max_clk{};
	std::string mem_min_clk{};
	ret = guest_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_MEM,
									  &clock_measure_mem);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		mem_clk = "N/A";
		mem_max_clk = "N/A";
		mem_min_clk = "N/A";
	} else {
		mem_clk = string_format(
					  "%ld", clock_measure_mem.clk);
		mem_max_clk = string_format(
						  "%ld", clock_measure_mem.max_clk);
		mem_min_clk = clock_measure_mem.min_clk == -1 ? "N/A" : string_format("%ld",
					  clock_measure_mem.min_clk);
	}
	std::string vclk0_clk{};
	std::string vclk0_max_clk{};
	std::string vclk0_min_clk{};
	ret = guest_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK0,
									  &clock_measure_vclk0);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		vclk0_clk = "N/A";
		vclk0_max_clk = "N/A";
		vclk0_min_clk = "N/A";
	} else {
		vclk0_clk = string_format(
						"%ld", clock_measure_vclk0.clk);
		vclk0_max_clk = string_format(
							"%ld", clock_measure_vclk0.max_clk);
		vclk0_min_clk = clock_measure_vclk0.min_clk == -1 ? "N/A" : string_format("%ld",
						clock_measure_vclk0.min_clk);
	}
	std::string vclk1_clk{};
	std::string vclk1_max_clk{};
	std::string vclk1_min_clk{};
	ret = guest_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK1,
									  &clock_measure_vclk1);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		vclk1_clk = "N/A";
		vclk1_max_clk = "N/A";
		vclk1_min_clk = "N/A";
	} else {
		vclk1_clk = string_format(
						"%ld", clock_measure_vclk1.clk);
		vclk1_max_clk = string_format(
							"%ld", clock_measure_vclk1.max_clk);
		vclk1_min_clk = clock_measure_vclk1.min_clk == -1 ? "N/A" : string_format("%ld",
						clock_measure_vclk1.min_clk);
	}

	std::string is_clk_gfx_locked { "N/A" };
	std::string is_clk_mem_locked { "N/A" };
	std::string is_clk_vclk0_locked { "N/A" };
	std::string is_clk_vclk1_locked { "N/A" };

	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
							gfx_clk.c_str(), gfx_min_clk.c_str(), gfx_max_clk.c_str(), is_clk_gfx_locked.c_str(), "N/A",
							mem_clk.c_str(), mem_min_clk.c_str(), mem_max_clk.c_str(), is_clk_mem_locked.c_str(), "N/A",
							vclk0_clk.c_str(), vclk0_min_clk.c_str(), vclk0_max_clk.c_str(), is_clk_vclk0_locked.c_str(),
							"N/A",
							vclk1_clk.c_str(), vclk1_min_clk.c_str(), vclk1_max_clk.c_str(), is_clk_vclk1_locked.c_str(),
							"N/A");
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json gfx_clk_json{};
		gfx_clk_json["value"] = clock_measure_gfx.clk ;
		gfx_clk_json["unit"] = gfx_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json gfx_min_clk_json{};
		gfx_min_clk_json["value"] = gfx_min_clk;
		gfx_min_clk_json["unit"] = gfx_min_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json gfx_max_clk_json{};
		gfx_max_clk_json["value"] = clock_measure_gfx.max_clk;
		gfx_max_clk_json["unit"] = gfx_max_clk == "N/A" ? "N/A" : "MHz";

		nlohmann::ordered_json mem_clk_json{};
		mem_clk_json["value"] = clock_measure_mem.clk ;
		mem_clk_json["unit"] = mem_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json mem_min_clk_json{};
		mem_min_clk_json["value"] = mem_min_clk;
		mem_min_clk_json["unit"] = mem_min_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json mem_max_clk_json{};
		mem_max_clk_json["value"] = clock_measure_mem.max_clk;
		mem_max_clk_json["unit"] = mem_max_clk == "N/A" ? "N/A" : "MHz";

		nlohmann::ordered_json vclk0_clk_json{};
		vclk0_clk_json["value"] = clock_measure_vclk0.clk ;
		vclk0_clk_json["unit"] = vclk0_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json vclk0_min_clk_json{};
		vclk0_min_clk_json["value"] = vclk0_min_clk;
		vclk0_min_clk_json["unit"] = vclk0_min_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json vclk0_max_clk_json{};
		vclk0_max_clk_json["value"] = clock_measure_vclk0.max_clk;
		vclk0_max_clk_json["unit"] = vclk0_max_clk == "N/A" ? "N/A" : "MHz";

		nlohmann::ordered_json vclk1_clk_json{};
		vclk1_clk_json["value"] = clock_measure_vclk1.clk ;
		vclk1_clk_json["unit"] = vclk1_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json vclk1_min_clk_json{};
		vclk1_min_clk_json["value"] = vclk1_min_clk;
		vclk1_min_clk_json["unit"] = vclk1_min_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json vclk1_max_clk_json{};
		vclk1_max_clk_json["value"] = clock_measure_vclk1.max_clk;
		vclk1_max_clk_json["unit"] = vclk1_max_clk == "N/A" ? "N/A" : "MHz";

		nlohmann::ordered_json gfx = { { "clk", gfx_clk_json },
			{ "max_clk", gfx_max_clk_json },
			{ "min_clk", gfx_min_clk_json },
			{ "clk_locked", is_clk_gfx_locked.c_str() }
		};

		nlohmann::ordered_json mem = { { "clk", mem_clk_json },
			{ "max_clk", mem_max_clk_json },
			{ "min_clk", mem_min_clk_json },
			{ "clk_locked", is_clk_mem_locked.c_str() },
			{ "clk_deep_sleep", "N/A" }
		};

		nlohmann::ordered_json vclk0 = { { "clk", vclk0_clk_json },
			{ "max_clk", vclk0_max_clk_json },
			{ "min_clk", vclk0_min_clk_json },
			{ "clk_locked", is_clk_vclk0_locked.c_str() },
			{ "clk_deep_sleep", "N/A" }
		};

		nlohmann::ordered_json vclk1 = { { "clk", vclk1_clk_json },
			{ "max_clk", vclk1_max_clk_json },
			{ "min_clk", vclk1_min_clk_json },
			{ "clk_locked", is_clk_vclk1_locked.c_str() },
			{ "clk_deep_sleep", "N/A" }
		};
		nlohmann::ordered_json clock = { { "gfx", gfx }, { "mem", mem }, { "vclk0", vclk0 }, { "vclk1", vclk1 }
		};
		out = clock.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
							gfx_clk.c_str(), gfx_min_clk.c_str(), gfx_max_clk.c_str(), is_clk_gfx_locked.c_str(), "N/A",
							mem_clk.c_str(), mem_min_clk.c_str(), mem_max_clk.c_str(), is_clk_mem_locked.c_str(), "N/A",
							vclk0_clk.c_str(), vclk0_min_clk.c_str(), vclk0_max_clk.c_str(), is_clk_vclk0_locked.c_str(), "N/A",
							vclk1_clk.c_str(), vclk1_min_clk.c_str(), vclk1_max_clk.c_str(), is_clk_vclk1_locked.c_str(),
							"N/A");
	} else {
		std::string gfx_clk_unit = gfx_clk == "N/A" ? "" : "MHz";
		std::string gfx_max_clk_unit = gfx_max_clk == "N/A" ? "" : "MHz";
		std::string gfx_min_clk_unit = gfx_min_clk == "N/A" ? "" : "MHz";

		std::string mem_clk_unit = mem_clk == "N/A" ? "" : "MHz";
		std::string mem_max_clk_unit = mem_max_clk == "N/A" ? "" : "MHz";
		std::string mem_min_clk_unit = mem_min_clk == "N/A" ? "" : "MHz";

		std::string vclk0_clk_unit = vclk0_clk == "N/A" ? "" : "MHz";
		std::string vclk0_max_clk_unit = vclk0_max_clk == "N/A" ? "" : "MHz";
		std::string vclk0_min_clk_unit = vclk0_min_clk == "N/A" ? "" : "MHz";

		std::string vclk1_clk_unit = vclk1_clk == "N/A" ? "" : "MHz";
		std::string vclk1_max_clk_unit = vclk1_max_clk == "N/A" ? "" : "MHz";
		std::string vclk1_min_clk_unit = vclk1_min_clk == "N/A" ? "" : "MHz";
		out = string_format(
				  metricClockMeasureHostTemplate, gfx_clk.c_str(),gfx_clk_unit.c_str(),
				  gfx_min_clk.c_str(),gfx_min_clk_unit.c_str(), gfx_max_clk.c_str(),gfx_max_clk_unit.c_str(),
				  is_clk_gfx_locked.c_str(), "N/A",
				  mem_clk.c_str(), mem_clk_unit.c_str(), mem_min_clk.c_str(), mem_min_clk_unit.c_str(),
				  mem_max_clk.c_str(), mem_max_clk_unit.c_str(),
				  is_clk_mem_locked.c_str(), "N/A");

		out += string_format(
				   metricVCLK0ClockMeasureHostTemplate, vclk0_clk.c_str(), vclk0_clk_unit.c_str(),
				   vclk0_min_clk.c_str(), vclk0_min_clk_unit.c_str(), vclk0_max_clk.c_str(),
				   vclk0_max_clk_unit.c_str(),
				   is_clk_vclk0_locked.c_str(), "N/A");

		out += string_format(
				   metricVCLK1ClockMeasureHostTemplate, vclk1_clk.c_str(), vclk1_clk_unit.c_str(),
				   vclk1_min_clk.c_str(), vclk1_min_clk_unit.c_str(), vclk1_max_clk.c_str(),
				   vclk1_max_clk_unit.c_str(),
				   is_clk_vclk1_locked.c_str(), "N/A");
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_temperature_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	if (AmdSmiPlatform::getInstance().is_guest()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}

	amdsmi_status_t ret;
	int64_t edge_temperature;
	int64_t junction_temperature;
	int64_t vram_temperature;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_temperature(arg, "N/A");
		return ret;
	}

	std::string edge_temperature_string{};
	ret = guest_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_CURRENT, &edge_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		edge_temperature_string = "N/A";
	} else {
		edge_temperature_string = (edge_temperature == UINT_MAX) ? "N/A" :
								  string_format(
									  "%lld", edge_temperature);
	}
	std::string junction_temperature_string{};
	ret = guest_amdsmi_get_temp_metric(processor,
									   AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
									   AMDSMI_TEMP_CURRENT,
									   &junction_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		junction_temperature_string = "N/A";
	} else {
		junction_temperature_string = string_format("%lld", junction_temperature);
	}
	std::string vram_temperature_string{};
	ret = guest_amdsmi_get_temp_metric(
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

int AmdSmiApiGuest::amdsmi_get_ecc_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	if (AmdSmiPlatform::getInstance().is_guest()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}

	amdsmi_status_t ret;
	amdsmi_error_count_t error_count;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_ecc(arg, "N/A");
		return ret;
	}

	ret = guest_amdsmi_get_gpu_total_ecc_count(processor,
		  &error_count);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_ecc(arg, "N/A");
		return ret;
	}

	std::string error_correctable_count{ string_format(
			"%lld", error_count.correctable_count) };
	std::string error_uncorrectable_count{ string_format(
			"%lld", error_count.uncorrectable_count) };
	std::string error_deferred_count = (error_count.deferred_count == -1) ? "N/A" : string_format(
										   "%lld", error_count.deferred_count);

	if (arg.watch > -1) {
		out = string_format("%s,%s,%s,%s,%s", error_correctable_count.c_str(),
							error_uncorrectable_count.c_str(), error_deferred_count.c_str(), "N/A", "N/A");
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json error_count_json = {
			{ "total_correctable_count", error_count.correctable_count },
			{ "total_uncorrectable_count", error_count.uncorrectable_count },
			{ "total_deferred_count", error_deferred_count },
			{ "cache_correctable_count", "N/A" },
			{ "cache_uncorrectable_count", "N/A" }
		};

		out = error_count_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s", error_correctable_count.c_str(),
				  error_uncorrectable_count.c_str(), error_deferred_count.c_str(), "N/A", "N/A");
	} else {
		out = string_format(
				  metricEccErrorCountTemplate, error_correctable_count.c_str(),
				  error_uncorrectable_count.c_str(), error_deferred_count.c_str(), "N/A", "N/A");
	}

	return ret;
}

int AmdSmiApiGuest::amdsmi_get_pcie_metric_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	if (AmdSmiPlatform::getInstance().is_guest()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}

	amdsmi_status_t ret;
	amdsmi_pcie_info_t pcie_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_pcie(arg, "N/A");
		return ret;
	}

	ret = guest_amdsmi_get_pcie_info(processor, &pcie_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_pcie(arg, "N/A");
		return ret;
	}

	std::string pcie_info_pcie_width{ string_format(
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
		out = string_format("%s,%s", pcie_info_pcie_width.c_str(),
							pcie_info_pcie_speed.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json values_json{};
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
			pcie_info_json["pcie_replay_count"] = "N/A";
		} else {
			pcie_info_json["pcie_replay_count"] = pcie_info.pcie_metric.pcie_replay_count;
		}
		if(pcie_info.pcie_metric.pcie_l0_to_recovery_count == UINT64_MAX) {
			pcie_info_json["pcie_l0_to_recovery_count"] = "N/A";
		} else {
			pcie_info_json["pcie_l0_to_recovery_count"] = pcie_info.pcie_metric.pcie_l0_to_recovery_count;
		}
		if(pcie_info.pcie_metric.pcie_replay_roll_over_count == UINT64_MAX) {
			pcie_info_json["pcie_replay_roll_over_count"] = "N/A";
		} else {
			pcie_info_json["pcie_replay_roll_over_count"] = pcie_info.pcie_metric.pcie_replay_roll_over_count;
		}
		if(pcie_info.pcie_metric.pcie_nak_sent_count == UINT64_MAX) {
			pcie_info_json["pcie_nak_sent_count"] = "N/A";
		} else {
			pcie_info_json["pcie_nak_sent_count"] = pcie_info.pcie_metric.pcie_nak_sent_count;
		}
		if(pcie_info.pcie_metric.pcie_nak_received_count == UINT64_MAX) {
			pcie_info_json["pcie_nak_received_count"] = "N/A";
		} else {
			pcie_info_json["pcie_nak_received_count"] = pcie_info.pcie_metric.pcie_nak_received_count;
		}

		out = pcie_info_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s,%s,%s", pcie_info_pcie_width.c_str(),
				  pcie_info_pcie_speed.c_str(), pcie_bandwidth.c_str(), pcie_replay_count.c_str(),
				  pcie_l0_to_recovery_count.c_str(), pcie_replay_roll_over_count.c_str(), pcie_nak_sent_count.c_str(),
				  pcie_nak_received_count.c_str());
	} else {
		std::string pcie_info_pcie_speed_unit = pcie_info_pcie_speed == "N/A" ? "" : "GT/s";
		std::string pcie_bandwidth_unit = pcie_bandwidth == "N/A" ? "" : "Mb/s";

		out = string_format(
				  pcieInfoTemplate, pcie_info_pcie_width.c_str(),
				  pcie_info_pcie_speed.c_str(), pcie_info_pcie_speed_unit.c_str(), pcie_bandwidth.c_str(),
				  pcie_bandwidth_unit.c_str(), pcie_replay_count.c_str(),
				  pcie_l0_to_recovery_count.c_str(), pcie_replay_roll_over_count.c_str(), pcie_nak_sent_count.c_str(),
				  pcie_nak_received_count.c_str());
	}

	return ret;
}

int AmdSmiApiGuest::amdsmi_get_fb_usage_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	amdsmi_vram_usage_t vram_usage;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_fb_usage(arg, "N/A");
		return ret;
	}

	ret = guest_amdsmi_get_gpu_vram_usage(processor, &vram_usage);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_fb_usage(arg, "N/A");
		return ret;
	}

	std::string vram_total_str{ string_format("%lld", vram_usage.vram_total) };
	std::string vram_used_str{ string_format("%lld", vram_usage.vram_used) };

	if (arg.watch > -1) {
		out = string_format("%s,%s", vram_total_str.c_str(), vram_used_str.c_str());
	} else if (arg.output == json) {
		nlohmann::ordered_json vram_total_json{};
		vram_total_json["value"] = vram_usage.vram_total;
		vram_total_json["unit"] =  vram_total_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json vram_used_json{};
		vram_used_json["value"] = vram_usage.vram_used;
		vram_used_json["unit"] = vram_used_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json vram_usage_json = { { "fb_total", vram_total_json },
			{ "fb_used", vram_used_json },
		};

		out = vram_usage_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s",
				  vram_total_str.c_str(),vram_used_str.c_str());
	} else {
		std::string vram_total_str_unit = vram_total_str == "N/A" ? "" : "MB";
		std::string vram_used_str_unit = vram_used_str == "N/A" ? "" : "MB";
		out = string_format(
				  metricFbUsageTemplate, vram_total_str.c_str(),vram_total_str_unit.c_str(), vram_used_str.c_str(),
				  vram_used_str_unit.c_str());
	}

	return ret;
}