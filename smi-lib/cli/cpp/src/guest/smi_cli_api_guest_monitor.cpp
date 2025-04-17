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
#include "interface/amdsmi.h"
#include "smi_cli_api_guest.h"
#include "smi_cli_parser.h"
#include "smi_cli_device.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"
#include "smi_cli_helpers.h"

#include "json/json.h"

#include <limits.h>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_CLOCK_INFO)(amdsmi_processor_handle, amdsmi_clk_type_t,
		amdsmi_clk_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
		amdsmi_temperature_type_t, amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CLOCK_INFO)(amdsmi_processor_handle, amdsmi_clk_type_t,
		amdsmi_clk_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
		amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle, amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VRAM_USAGE)(amdsmi_processor_handle,
		amdsmi_vram_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle, amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
		amdsmi_error_count_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF guest_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_POWER_INFO guest_amdsmi_get_power_info;
extern AMDSMI_GET_TEMP_METRIC guest_amdsmi_get_temp_metric;
extern AMDSMI_GET_CLOCK_INFO guest_amdsmi_get_clock_info;
extern AMDSMI_GET_GPU_ACTIVITY guest_amdsmi_get_gpu_activity;
extern AMDSMI_GET_GPU_VRAM_USAGE guest_amdsmi_get_gpu_vram_usage;
extern AMDSMI_GET_PCIE_INFO guest_amdsmi_get_pcie_info;
extern AMDSMI_GET_GPU_TOTAL_ECC_COUNT guest_amdsmi_get_gpu_total_ecc_count;


std::string guest_fill_power_usage(Arguments arg, std::string value = "N/A")
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json power_json{};
		power_json["value"] = value.c_str();
		power_json["unit"] = "N/A";
		out = power_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s", value.c_str());
	} else {
		out = string_format("%s %s", value.c_str(), "N/A");
	}
	return out;
}

std::string guest_fill_gpu_mem_temperature(Arguments arg, std::string value = "N/A")
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json temperature_json{};

		nlohmann::ordered_json gpu_temp{};
		gpu_temp["value"] = value.c_str();
		gpu_temp["unit"] = "N/A";
		temperature_json["hotspot_temperature"] = gpu_temp;

		nlohmann::ordered_json mem_temp{};
		mem_temp["value"] = value.c_str();
		mem_temp["unit"] = "N/A";
		temperature_json["mem_temperature"] = gpu_temp;
		out = temperature_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s, %s", value.c_str(), value.c_str());
	} else {
		out = string_format("%s, %s", value.c_str(), value.c_str());
	}

	return out;
}

std::string guest_fill_gfx(Arguments arg, std::string value = "N/A")
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json result{};

		result["util"] = value.c_str();
		result["clk"] = value.c_str();

		out = result.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s", value.c_str(), value.c_str());
	} else {
		out = string_format("%s,%s", value.c_str(), value.c_str());
	}
	return out;
}

std::string guest_fill_mem(Arguments arg, std::string value = "N/A")
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json result{};

		result["util"] = value.c_str();
		result["clk"] = value.c_str();

		out = result.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s", value.c_str(), value.c_str());
	} else {
		out = string_format("%s,%s", value.c_str(), value.c_str());
	}
	return out;
}

std::string guest_fill_empty_ecc(Arguments arg, std::string value = "N/A")
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json result{};

		result["total_correctable_count"] = value.c_str();
		result["total_uncorrectable_count"] = value.c_str();

		out = result.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s", value.c_str(), value.c_str());
	} else {
		out = string_format("%s,%s", value.c_str(), value.c_str());
	}

	return out;
}

std::string guest_fill_pcie_info(Arguments arg, std::string value = "N/A")
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json result{};

		result["bandwidth"] = value.c_str();
		result["replay_count"] = value.c_str();

		out = result.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s", value.c_str(), value.c_str());
	} else {
		out = string_format("%s,%s", value.c_str(), value.c_str());
	}

	return out;
}

std::string guest_fill_usage_empty(Arguments arg, std::string value = "N/A")
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json vram_total_json{};
		vram_total_json["value"] = value;
		vram_total_json["unit"] =  value;
		nlohmann::ordered_json vram_used_json{};
		vram_used_json["value"] = value;
		vram_used_json["unit"] = value;
		nlohmann::ordered_json vram_usage_json = { { "fb_total", vram_total_json }, { "fb_used", vram_used_json } };

		out = vram_usage_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s", value.c_str(),value.c_str());
	} else {
		out = string_format("%s,%s", value, value);
	}

	return out;
}

std::string guest_fill_encoder(Arguments arg, std::string value = "N/A")
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json result{};

		result["util"] = value.c_str();
		result["clk"] = value.c_str();

		out = result.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s", value.c_str(), value.c_str());
	} else {
		out = string_format("%s,%s", value.c_str(), value.c_str());
	}
	return out;
}

int AmdSmiApiGuest::amdsmi_get_power_usage_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
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
		formatted_string = guest_fill_power_usage(arg);
		return ret;
	}

	ret = guest_amdsmi_get_power_info(processor, &power_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_power_usage(arg);
		return ret;
	}
	std::string socket_power_str{ string_format( "%ld", power_info.socket_power) };

	if (arg.output == json) {
		nlohmann::ordered_json socket_power{};
		socket_power["value"] = power_info.socket_power;
		socket_power["unit"] = socket_power_str == "N/A" ? "N/A" : "W";

		formatted_string = socket_power.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s", socket_power_str.c_str());
	} else {
		std::string socket_power_unit = socket_power_str == "N/A" ? "" : "W";
		formatted_string = string_format("%s %s", socket_power_str.c_str(), socket_power_unit.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_temperature_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	if (AmdSmiPlatform::getInstance().is_guest()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}

	amdsmi_status_t ret;

	int64_t junction_temperature;
	int64_t vram_temperature;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_gpu_mem_temperature(arg, "N/A");
		return ret;
	}

	std::string junction_temperature_string{};
	ret = guest_amdsmi_get_temp_metric(processor, AMDSMI_TEMPERATURE_TYPE_HOTSPOT, AMDSMI_TEMP_CURRENT,
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

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json temperature_json{};

		nlohmann::ordered_json edge_temperature_json{};
		if (junction_temperature == UINT_MAX) {
			edge_temperature_json["value"] = junction_temperature_string;
		} else {
			edge_temperature_json["value"] =junction_temperature;
		}
		edge_temperature_json["unit"] = junction_temperature_string == "N/A" ? "N/A" : "C";
		temperature_json["hotspot"] = edge_temperature_json;
		nlohmann::ordered_json vram_temperature_json{};

		vram_temperature_json["value"] = vram_temperature;
		vram_temperature_json["unit"] = vram_temperature_string == "N/A" ? "N/A" : "C";
		temperature_json["mem"] = vram_temperature_json;

		formatted_string = temperature_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s", junction_temperature_string.c_str(),
										 vram_temperature_string.c_str());
	} else {
		std::string edge_temperature_string_unit = junction_temperature_string == "N/A" ? "" : "C";
		std::string vram_temperature_string_unit = vram_temperature_string == "N/A" ? "" : "C";
		std::string gpu_temp{string_format("%s %s",  junction_temperature_string.c_str(), edge_temperature_string_unit.c_str())};
		std::string mem_temp{string_format("%s %s",  vram_temperature_string.c_str(), vram_temperature_string_unit.c_str())};
		formatted_string = string_format("%s, %s", gpu_temp, mem_temp);
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_gfx_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;
	amdsmi_clk_info_t clock_measure_gfx;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	nlohmann::ordered_json result{};

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_gfx(arg, "N/A");
		return ret;
	}

	std::string gfx_clk{};
	ret = guest_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_GFX, &clock_measure_gfx);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		gfx_clk = "N/A";
	} else {
		gfx_clk = string_format("%ld", clock_measure_gfx.clk);
	}

	std::string gfx_activity{};
	amdsmi_engine_usage_t engine_usage;
	ret = guest_amdsmi_get_gpu_activity(processor, &engine_usage);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		gfx_activity = "N/A";
	} else {
		gfx_activity = string_format("%ld", engine_usage.gfx_activity);
	}

	if (arg.output == json) {
		nlohmann::ordered_json gfx_clk_json{};
		gfx_clk_json["value"] = clock_measure_gfx.clk ;
		gfx_clk_json["unit"] = gfx_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json gfx_activity_json{};
		gfx_activity_json["value"] = engine_usage.gfx_activity;
		gfx_activity_json["unit"] =  gfx_activity == "N/A" ? "N/A" : "%";

		result["util"] = gfx_activity_json;
		result["clk"] = gfx_clk_json;
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s", gfx_activity.c_str(), gfx_clk.c_str());
	} else {
		std::string gfx_util_unit{gfx_activity == "N/A" ? "" : "%"};
		std::string gfx_util_str{string_format("%s %s", gfx_activity.c_str(), gfx_util_unit.c_str() )};
		std::string gfx_clock_unit{gfx_clk == "N/A" ? "" : "MHz"};
		std::string gfx_clock_str{string_format("%s %s", gfx_clk.c_str(), gfx_clock_unit.c_str())};
		formatted_string = string_format("%s, %s", gfx_util_str.c_str(), gfx_clock_str.c_str());
	}

	if (arg.output == json) {
		formatted_string = result.dump(4);
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_mem_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;
	amdsmi_clk_info_t clock_measure_mem;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	nlohmann::ordered_json result{};

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_mem(arg, "N/A");
		return ret;
	}

	std::string mem_clk{};
	ret = guest_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_MEM, &clock_measure_mem);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		mem_clk = "N/A";
	} else {
		mem_clk = string_format("%ld", clock_measure_mem.clk);
	}

	std::string mem_activity{};
	amdsmi_engine_usage_t engine_usage;
	ret = guest_amdsmi_get_gpu_activity(processor, &engine_usage);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		mem_activity = "N/A";
	} else {
		mem_activity = string_format("%ld", engine_usage.umc_activity);
	}

	if (arg.output == json) {
		nlohmann::ordered_json mem_clk_json{};
		mem_clk_json["value"] = clock_measure_mem.clk ;
		mem_clk_json["unit"] = mem_clk == "N/A" ? "N/A" : "MHz";
		nlohmann::ordered_json mem_activity_json{};
		mem_activity_json["value"] = engine_usage.umc_activity;
		mem_activity_json["unit"] =  mem_activity == "N/A" ? "N/A" : "%";

		result["util"] = mem_activity_json;
		result["clk"] = mem_clk_json;
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s", mem_activity.c_str(), mem_clk.c_str());
	} else {
		std::string mem_util_unit{mem_activity == "N/A" ? "" : "%"};
		std::string mem_util_str{string_format("%s %s", mem_activity.c_str(), mem_util_unit.c_str() )};
		std::string mem_clock_unit{mem_clk == "N/A" ? "" : "MHz"};
		std::string mem_clock_str{string_format("%s %s", mem_clk.c_str(), mem_clock_unit.c_str())};
		formatted_string = string_format("%s, %s", mem_util_str.c_str(), mem_clock_str.c_str());
	}

	if (arg.output == json) {
		formatted_string = result.dump(4);
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_encoder_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	nlohmann::ordered_json result{};

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_encoder(arg, "N/A");
		return ret;
	}

	amdsmi_clk_info_t clock_measure_encoder0;
	ret = guest_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK0, &clock_measure_encoder0);
	std::string encoder_clk0{};
	if (ret != AMDSMI_STATUS_SUCCESS) {
		encoder_clk0 = "N/A";
	} else {
		encoder_clk0 = string_format("%ld", clock_measure_encoder0.clk);
	}

	std::string encoder_activity{};
	amdsmi_engine_usage_t engine_usage;
	ret = guest_amdsmi_get_gpu_activity(processor, &engine_usage);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		encoder_activity = "N/A";
	} else {
		encoder_activity = string_format("%ld", engine_usage.mm_activity);
	}


	amdsmi_clk_info_t clock_measure_encoder1;
	ret = guest_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK1, &clock_measure_encoder1);
	std::string encoder_clk1{};
	if (ret != AMDSMI_STATUS_SUCCESS) {
		encoder_clk1 = "N/A";
	} else {
		encoder_clk1 = string_format("%ld", clock_measure_encoder1.clk);
	}

	if (arg.output == json) {
		nlohmann::ordered_json encoder_clk_json0{};
		encoder_clk_json0["value"] = clock_measure_encoder0.clk;
		encoder_clk_json0["unit"] = encoder_clk0 == "N/A" ? "N/A" : "MHz";

		nlohmann::ordered_json encoder_clk_json1{};
		encoder_clk_json1["value"] = clock_measure_encoder1.clk;
		encoder_clk_json1["unit"] = encoder_clk1 == "N/A" ? "N/A" : "MHz";

		nlohmann::ordered_json encoder_activity_json{};
		encoder_activity_json["unit"] = engine_usage.mm_activity;
		encoder_activity_json["value"] = encoder_activity == "N/A" ? "N/A" : "%";

		result["vclk_0"] = nlohmann::ordered_json::object({{"util", encoder_activity_json},
			{"clk", encoder_clk_json0} });

		result["vclk_1"] = nlohmann::ordered_json::object({{"util", encoder_activity_json},
			{"clk", encoder_clk_json1} });
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s\n,%s,%s", encoder_activity.c_str(), encoder_clk0.c_str(),
										 encoder_activity.c_str(), encoder_clk1.c_str());
	} else {
		std::string encoder_util_unit{encoder_activity == "N/A" ? "" : "%"};
		std::string encoder_util_str{string_format("%s %s", encoder_activity.c_str(), encoder_util_unit.c_str() )};
		std::string encoder_clock_unit0{encoder_clk0 == "N/A" ? "" : "MHz"};
		std::string encoder_clock_unit1{encoder_clk1 == "N/A" ? "" : "MHz"};
		std::string encoder_clock_str0{string_format("%s %s", encoder_clk0.c_str(), encoder_clock_unit0.c_str())};
		std::string encoder_clock_str1{string_format("%s %s", encoder_clk1.c_str(), encoder_clock_unit1.c_str())};
		formatted_string = string_format("%s, %s, %s, %s", encoder_util_str.c_str(),
										 encoder_clock_str0.c_str(), encoder_util_str.c_str(), encoder_clock_str1.c_str());
	}

	if (arg.output == json) {
		formatted_string = result.dump(4);
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_ecc_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_error_count_t total_error_count;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_empty_ecc(arg);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_total_ecc_count(processor, &total_error_count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_empty_ecc(arg);
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

	if (arg.output == json) {
		nlohmann::ordered_json error_count_json{};
		error_count_json["total_correctable_count"] = total_error_count.correctable_count;
		error_count_json["total_uncorrectable_count"] = total_error_count.uncorrectable_count;

		formatted_string = error_count_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s", total_error_correctable_count.c_str(),
										 total_error_uncorrectable_count.c_str());
	} else {
		formatted_string = string_format("%s,%s", total_error_correctable_count.c_str(),
										 total_error_uncorrectable_count.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_vram_usage_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	amdsmi_vram_usage_t vram_usage;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_usage_empty(arg, "N/A");
		return ret;
	}

	ret = guest_amdsmi_get_gpu_vram_usage(processor, &vram_usage);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_usage_empty(arg, "N/A");
		return ret;
	}

	std::string vram_total_str{ string_format("%lld", vram_usage.vram_total) };
	std::string vram_used_str{ string_format("%lld", vram_usage.vram_used) };

	if (arg.output == json) {
		nlohmann::ordered_json vram_total_json{};
		vram_total_json["value"] = vram_usage.vram_total;
		vram_total_json["unit"] =  vram_total_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json vram_used_json{};
		vram_used_json["value"] = vram_usage.vram_used;
		vram_used_json["unit"] = vram_used_str == "N/A" ? "N/A" : "MB";
		nlohmann::ordered_json vram_usage_json = { { "vram_total", vram_total_json }, { "vram_used", vram_used_json }, };

		formatted_string = vram_usage_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s", vram_total_str.c_str(),vram_used_str.c_str());
	} else {
		std::string vram_total_str_unit = vram_total_str == "N/A" ? "" : "MB";
		std::string vram_used_str_unit = vram_used_str == "N/A" ? "" : "MB";
		std::string vram_used{string_format("%s %s", vram_used_str.c_str(), vram_used_str_unit.c_str())};
		std::string vram_total{string_format("%s %s", vram_total_str.c_str(),vram_total_str_unit.c_str())};
		formatted_string = string_format("%s,%s", vram_used.c_str(), vram_total.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}


int AmdSmiApiGuest::amdsmi_get_pcie_info_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_pcie_info_t pcie_info;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_pcie_info(arg);
		return ret;
	}

	ret = guest_amdsmi_get_pcie_info(processor, &pcie_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_pcie_info(arg);
		return ret;
	}

	std::string pcie_bandwidth = (pcie_info.pcie_metric.pcie_bandwidth == UINT_MAX) ? "N/A" :
								 string_format("%ld", pcie_info.pcie_metric.pcie_bandwidth);

	std::string pcie_replay_count = (pcie_info.pcie_metric.pcie_replay_count == UINT64_MAX) ? "N/A" :
									string_format("%lld", pcie_info.pcie_metric.pcie_replay_count);

	if (arg.output == json) {
		nlohmann::ordered_json current_bandwidth{};
		if(pcie_info.pcie_metric.pcie_bandwidth == UINT_MAX) {
			current_bandwidth["value"] = "N/A";
		} else {
			current_bandwidth["value"] = pcie_info.pcie_metric.pcie_bandwidth;
		}
		current_bandwidth["unit"] = pcie_bandwidth == "N/A" ? "N/A" : "Mb/s";
		nlohmann::ordered_json pcie_info_json{};
		pcie_info_json["bandwidth"] = current_bandwidth;
		if (pcie_info.pcie_metric.pcie_replay_count == UINT64_MAX) {
			pcie_info_json["replay_count"] = "N/A";
		} else {
			pcie_info_json["replay_count"] = pcie_info.pcie_metric.pcie_replay_count;
		}
		formatted_string = pcie_info_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s", pcie_replay_count.c_str(), pcie_bandwidth.c_str());
	} else {
		std::string bw_unut{pcie_bandwidth == "N/A" ? "" : "Mb/s"};
		formatted_string = string_format("%s,%s %s", pcie_replay_count.c_str(), pcie_bandwidth.c_str(),
										 bw_unut.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}