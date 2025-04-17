/* * Copyright (C) 2024 Advanced Micro Devices. All rights reserved.
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
#include "smi_cli_device.h"
#include "smi_cli_platform.h"
#include "smi_cli_helpers.h"
#include "smi_cli_exception.h"

#include "json/json.h"

#include <limits.h>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle, uint32_t,
		amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
		amdsmi_temperature_type_t,
		amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CLOCK_INFO)(amdsmi_processor_handle, amdsmi_clk_type_t,
		amdsmi_clk_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
		amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_METRICS)(amdsmi_processor_handle, uint32_t *,
		amdsmi_metric_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
		amdsmi_error_count_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_COUNT)(amdsmi_processor_handle, amdsmi_gpu_block_t,
		amdsmi_error_count_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_POWER_INFO host_amdsmi_get_power_info;
extern AMDSMI_GET_TEMP_METRIC host_amdsmi_get_temp_metric;
extern AMDSMI_GET_CLOCK_INFO host_amdsmi_get_clock_info;
extern AMDSMI_GET_GPU_ACTIVITY host_amdsmi_get_gpu_activity;
extern AMDSMI_GET_GPU_METRICS host_amdsmi_get_gpu_metrics;
extern AMDSMI_GET_GPU_TOTAL_ECC_COUNT host_amdsmi_get_gpu_total_ecc_count;
extern AMDSMI_GET_GPU_ECC_COUNT host_amdsmi_get_gpu_ecc_count;
extern AMDSMI_GET_PCIE_INFO host_amdsmi_get_pcie_info;


std::string host_fill_power_usage(Arguments arg, std::string value = "N/A")
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

std::string host_fill_gpu_mem_temperature(Arguments arg, std::string value = "N/A")
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

std::string host_fill_gfx(Arguments arg, std::string value = "N/A")
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


std::string host_fill_mem(Arguments arg, std::string value = "N/A")
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

std::string host_fill_encoder(Arguments arg, std::string value = "N/A")
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

std::string host_fill_decoder(Arguments arg, std::string value = "N/A")
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

std::string host_fill_empty_ecc(Arguments arg, std::string value = "N/A")
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

std::string host_fill_pcie_info(Arguments arg, std::string value = "N/A")
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json result{};

		result["bandwidth"] = value.c_str();
		result["pcie_replay_count"] = value.c_str();

		out = result.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s", value.c_str(), value.c_str());
	} else {
		out = string_format("%s,%s", value.c_str(), value.c_str());
	}

	return out;
}

int AmdSmiApiHost::amdsmi_get_power_usage_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_power_info_t power_info;
	uint32_t sensor_ind = 0;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_power_usage(arg);
		return ret;
	}

	ret = host_amdsmi_get_power_info(processor, sensor_ind, &power_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_power_usage(arg);
		return ret;
	}

	if (arg.output == json) {
		nlohmann::ordered_json power{};
		if (power_info.socket_power == UINT_MAX) {
			power["value"] = "N/A";
			power["unit"] = "N/A";
		} else {
			power["value"] = power_info.socket_power;
			power["unit"] = "W";
		}
		formatted_string = power.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%lld", power_info.socket_power);
	} else {
		std::string socket_power_str{ string_format(
										  "%lld", power_info.socket_power) };
		std::string socket_power_unit = socket_power_str == "N/A" ? "" : "W";
		formatted_string = string_format("%s %s", socket_power_str.c_str(), socket_power_unit.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_temperature_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	int64_t hotspot_temperature;
	int64_t vram_temperature;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_gpu_mem_temperature(arg, "N/A");
		return ret;
	}

	std::string hostspot_temperature_string{};
	ret = host_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_HOTSPOT, AMDSMI_TEMP_CURRENT, &hotspot_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		hostspot_temperature_string = "N/A";
	} else {
		hostspot_temperature_string = (hotspot_temperature == UINT_MAX) ? "N/A" : string_format( "%lld",
									  hotspot_temperature);
	}
	std::string vram_temperature_string{};
	ret = host_amdsmi_get_temp_metric(
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
		if(hotspot_temperature == UINT_MAX) {
			edge_temperature_json["value"] = hostspot_temperature_string;
		} else {
			edge_temperature_json["value"] = hotspot_temperature;
		}

		edge_temperature_json["unit"] = hostspot_temperature_string == "N/A" ? "N/A" : "C";
		temperature_json["hotspot"] = edge_temperature_json;
		nlohmann::ordered_json vram_temperature_json{};

		vram_temperature_json["value"] = vram_temperature;
		vram_temperature_json["unit"] = vram_temperature_string == "N/A" ? "N/A" : "C";
		temperature_json["mem"] = vram_temperature_json;

		formatted_string = temperature_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s", hostspot_temperature_string.c_str(),
										 vram_temperature_string.c_str());
	} else {
		std::string edge_temperature_string_unit = hostspot_temperature_string == "N/A" ? "" : "C";
		std::string vram_temperature_string_unit = vram_temperature_string == "N/A" ? "" : "C";
		std::string gpu_temp{string_format("%s %s",  hostspot_temperature_string.c_str(), edge_temperature_string_unit.c_str())};
		std::string mem_temp{string_format("%s %s",  vram_temperature_string.c_str(), vram_temperature_string_unit.c_str())};
		formatted_string = string_format("%s, %s", gpu_temp.c_str(), mem_temp.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_gfx_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	bool setup_template{true};
	bool metric_table{false};

	nlohmann::ordered_json result{};

	std::vector<amdsmi_metric_t> gfx_clock{};
	std::vector<amdsmi_metric_t> gfx_util{};

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_gfx(arg, "N/A");
		return ret;
	}

	if (AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200()) {
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
						gfx_clock.push_back(metrics[i]);
					}
					if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_GFX)
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						gfx_util.push_back(metrics[i]);
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
		if (gfx_clock.size() == 0 || gfx_util.size() == 0) {
			setup_template = true;
		} else {
			setup_template = false;
			if (arg.output == json) {
				nlohmann::ordered_json gfx_activity{};
				gfx_activity["value"] = gfx_util[0].val;
				gfx_activity["unit"] = gfx_util[0].val == UINT64_MAX ? "N/A" : "%";
				nlohmann::ordered_json gfx_clock_json{};
				if (gfx_clock[0].val == UINT64_MAX) {
					gfx_clock_json["value"] = "N/A";
				} else {
					gfx_clock_json["value"] = gfx_clock[0].val;
				}
				gfx_clock_json["unit"] =  gfx_clock[0].val == UINT64_MAX ? "N/A" : "MHz";

				result["util"] = gfx_activity;
				result["value"] = gfx_clock_json;
			} else if (arg.output == csv) {
				formatted_string = string_format(",%d,%d",  gfx_util[0].val, gfx_clock[0].val);
			} else {
				std::string gfx_util_unit{gfx_util[0].val == UINT64_MAX ? "" : "%"};
				std::string gfx_util_str{string_format("%d %s", gfx_util[0].val, gfx_util_unit.c_str() )};
				std::string gfx_clock_unit{gfx_clock[0].val == UINT64_MAX ? "" : "MHz"};
				std::string gfx_clock_str{string_format("%d %s", gfx_clock[0].val, gfx_clock_unit.c_str())};
				formatted_string.append(string_format("%s,%s,", gfx_util_str.c_str(), gfx_clock_str.c_str()));
			}
		}
	}

	if (setup_template) {
		amdsmi_clk_info_t clock_measure_gfx;
		amdsmi_engine_usage_t engine_usage;
		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_GFX,
										 &clock_measure_gfx);
		std::string gfx_clk{};
		if (ret != AMDSMI_STATUS_SUCCESS) {
			gfx_clk = "N/A";
		} else {
			gfx_clk = string_format("%ld", clock_measure_gfx.clk);
		}

		std::string gfx_activity{};
		ret = host_amdsmi_get_gpu_activity(processor, &engine_usage);
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
	}

	if (arg.output == json) {
		formatted_string = result.dump(4);
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_mem_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	bool setup_template{true};
	bool metric_table{false};

	nlohmann::ordered_json result{};
	std::vector<amdsmi_metric_t> mem_clock{};
	std::vector<amdsmi_metric_t> mem_util{};

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_mem(arg, "N/A");
		return ret;
	}

	if (AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200()) {
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
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_MEM
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						mem_clock.push_back(metrics[i]);
					}
					if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_MEM)
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						mem_util.push_back(metrics[i]);
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
		if (mem_clock.size() == 0 || mem_util.size() == 0) {
			setup_template = true;
		} else {
			setup_template = false;
			if (arg.output == json) {
				nlohmann::ordered_json mem_activity{};
				mem_activity["value"] = mem_util[0].val;
				mem_activity["unit"] = mem_util[0].val == UINT64_MAX ? "N/A" : "%";

				nlohmann::ordered_json mem_clock_json{};
				if (mem_clock[0].val == UINT64_MAX) {
					mem_clock_json["value"] = "N/A";
				} else {
					mem_clock_json["value"] = mem_clock[0].val;
				}
				mem_clock_json["unit"] =  mem_clock[0].val == UINT64_MAX ? "N/A" : "MHz";

				result["util"] = mem_activity;
				result["clk"] = mem_clock_json;
			} else if (arg.output == csv) {
				formatted_string = string_format(",%d,%d", mem_util[0].val, mem_clock[0].val);
			} else {
				std::string mem_util_unit{mem_util[0].val == UINT64_MAX ? "" : "%"};
				std::string mem_util_str{string_format("%d %s", mem_util[0].val, mem_util_unit.c_str() )};
				std::string mem_clock_unit{mem_clock[0].val == UINT64_MAX ? "" : "MHz"};
				std::string mem_clock_str{string_format("%d %s", mem_clock[0].val, mem_clock_unit.c_str())};

				formatted_string.append(string_format("%s,%s,", mem_util_str.c_str(), mem_clock_str.c_str()));
			}
		}
	}

	if (setup_template) {
		amdsmi_clk_info_t clock_measure_mem;
		amdsmi_engine_usage_t engine_usage;
		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_MEM,
										 &clock_measure_mem);
		std::string mem_clk{};
		if (ret != AMDSMI_STATUS_SUCCESS) {
			mem_clk = "N/A";
		} else {
			mem_clk = string_format(
						  "%ld", clock_measure_mem.clk);
		}

		std::string mem_activity{};
		ret = host_amdsmi_get_gpu_activity(processor, &engine_usage);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			mem_activity = "N/A";
		} else {
			mem_activity = string_format("%ld", engine_usage.umc_activity);
		}

		if (arg.output == json) {
			nlohmann::ordered_json mem_clk_json{};
			mem_clk_json["value"] = clock_measure_mem.clk;
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
	}

	if (arg.output == json) {
		formatted_string = result.dump(4);
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_encoder_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	bool setup_template{true};
	bool metric_table{false};

	nlohmann::ordered_json result{};
	std::vector<amdsmi_metric_t> encoder_clock{};
	std::vector<amdsmi_metric_t> encoder_util{};

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_encoder(arg, "N/A");
		return ret;
	}

	if (AmdSmiPlatform::getInstance().is_mi300()) {
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
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_VCLK
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						encoder_clock.push_back(metrics[i]);
					}
					if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_JPEG)
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						encoder_util.push_back(metrics[i]);
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
		if (encoder_clock.size() == 0 || encoder_util.size() == 0) {
			setup_template = true;
		} else {
			setup_template = false;
			if (arg.output == json) {
				nlohmann::ordered_json encoder_activity{};
				encoder_activity["value"] = encoder_util[0].val;
				encoder_activity["unit"] = encoder_util[0].val == UINT64_MAX ? "N/A" : "%";
				nlohmann::ordered_json encoder_clock_json{};
				if (encoder_clock[0].val == UINT64_MAX) {
					encoder_clock_json["value"] = "N/A";
				} else {
					encoder_clock_json["value"] = encoder_clock[0].val;
				}
				encoder_clock_json["unit"] = encoder_clock[0].val == UINT64_MAX ? "N/A" : "MHz";

				result["util"] = encoder_activity;
				result["clk"] = encoder_clock_json;
			} else if (arg.output == csv) {
				formatted_string = string_format(",%d,%d", encoder_util[0].val, encoder_clock[0].val);
			} else {
				std::string encoder_util_unit{encoder_util[0].val == UINT64_MAX ? "" : "%"};
				std::string encoder_util_str{string_format("%d %s", encoder_util[0].val, encoder_util_unit.c_str() )};
				std::string encoder_clock_unit{encoder_clock[0].val == UINT64_MAX ? "" : "MHz"};
				std::string encoder_clock_str{string_format("%d %s", encoder_clock[0].val, encoder_clock_unit.c_str())};

				formatted_string.append(string_format("%s,%s,", encoder_util_str.c_str(),
													  encoder_clock_str.c_str()));
			}
		}
	}

	if (setup_template) {
		amdsmi_clk_info_t clock_measure_encoder0;
		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK0, &clock_measure_encoder0);
		std::string encoder_clk0{};
		if (ret != AMDSMI_STATUS_SUCCESS || clock_measure_encoder0.clk == UINT_MAX) {
			encoder_clk0 = "N/A";
		} else {
			encoder_clk0 = string_format("%ld", clock_measure_encoder0.clk);
		}

		amdsmi_engine_usage_t engine_usage;
		std::string encoder_activity{};
		ret = host_amdsmi_get_gpu_activity(processor, &engine_usage);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			encoder_activity = "N/A";
		} else {
			encoder_activity = string_format("%ld", engine_usage.mm_activity);
		}

		amdsmi_clk_info_t clock_measure_encoder1;
		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_VCLK1, &clock_measure_encoder1);
		std::string encoder_clk1{};
		if (ret != AMDSMI_STATUS_SUCCESS || clock_measure_encoder1.clk == UINT_MAX) {
			encoder_clk1 = "N/A";
		} else {
			encoder_clk1 = string_format("%ld", clock_measure_encoder1.clk);
		}

		if (arg.output == json) {
			nlohmann::ordered_json encoder_clk_json0{};
			encoder_clk_json0["value"] = encoder_clk0;
			encoder_clk_json0["unit"] = encoder_clk0 == "N/A" ? "N/A" : "MHz";

			nlohmann::ordered_json encoder_clk_json1{};
			encoder_clk_json1["value"] = encoder_clk1;
			encoder_clk_json1["unit"] = encoder_clk1 == "N/A" ? "N/A" : "MHz";

			nlohmann::ordered_json encoder_activity_json{};
			encoder_activity_json["value"] = encoder_activity;
			encoder_activity_json["unit"] = encoder_activity == "N/A" ? "N/A" : "%";

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
	}

	if (arg.output == json) {
		formatted_string = result.dump(4);
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_decoder_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	bool setup_template{true};
	bool metric_table{false};

	nlohmann::ordered_json result{};
	std::vector<amdsmi_metric_t> decoder_clock{};
	std::vector<amdsmi_metric_t> decoder_util{};

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_decoder(arg, "N/A");
		return ret;
	}

	if (AmdSmiPlatform::getInstance().is_mi300()) {
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
					if (metrics[i].name == AMDSMI_METRIC_NAME_CLK_DCLK
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						decoder_clock.push_back(metrics[i]);
					}
					if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_VCN)
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						decoder_util.push_back(metrics[i]);
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
		if (decoder_clock.size() == 0 || decoder_util.size() == 0) {
			setup_template = true;
		} else {
			setup_template = false;
			if (arg.output == json) {
				nlohmann::ordered_json decoder_activity{};
				decoder_activity["value"] = decoder_util[0].val;
				decoder_activity["unit"] = decoder_util[0].val == UINT64_MAX ? "N/A" : "%";
				nlohmann::ordered_json decoder_clock_json{};
				if (decoder_clock[0].val == UINT64_MAX) {
					decoder_clock_json["value"] = "N/A";
				} else {
					decoder_clock_json["value"] = decoder_clock[0].val;
				}
				decoder_clock_json["unit"] = decoder_clock[0].val == UINT64_MAX ? "N/A" : "MHz";

				result["util"] = decoder_activity;
				result["clk"] = decoder_clock_json;
			} else if (arg.output == csv) {
				formatted_string = string_format(",%d,%d", decoder_util[0].val, decoder_clock[0].val);
			} else {
				std::string decoder_util_unit{decoder_util[0].val == UINT64_MAX ? "" : "%"};
				std::string decoder_util_str{string_format("%d %s", decoder_util[0].val, decoder_util_unit.c_str() )};
				std::string decoder_clock_unit{decoder_clock[0].val == UINT64_MAX ? "" : "MHz"};
				std::string decoder_clock_str{string_format("%d %s", decoder_clock[0].val, decoder_clock_unit.c_str())};

				formatted_string.append(string_format("%s,%s,", decoder_util_str.c_str(),
													  decoder_clock_str.c_str()));
			}
		}
	}

	if (setup_template) {
		amdsmi_clk_info_t clock_measure_decoder0;
		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_DCLK0, &clock_measure_decoder0);
		std::string decoder_clk0{};
		if (ret != AMDSMI_STATUS_SUCCESS || clock_measure_decoder0.clk == UINT_MAX) {
			decoder_clk0 = "N/A";
		} else {
			decoder_clk0 = string_format("%ld", clock_measure_decoder0.clk);
		}

		std::string decoder_activity{"N/A"};

		amdsmi_clk_info_t clock_measure_decoder1;
		ret = host_amdsmi_get_clock_info(processor, AMDSMI_CLK_TYPE_DCLK1, &clock_measure_decoder1);
		std::string decoder_clk1{};
		if (ret != AMDSMI_STATUS_SUCCESS || clock_measure_decoder1.clk == UINT_MAX) {
			decoder_clk1 = "N/A";
		} else {
			decoder_clk1 = string_format("%ld", clock_measure_decoder1.clk);
		}

		if (arg.output == json) {
			nlohmann::ordered_json decoder_clk_json0{};
			decoder_clk_json0["value"] = decoder_clk0;
			decoder_clk_json0["unit"] = decoder_clk0 == "N/A" ? "N/A" : "MHz";

			nlohmann::ordered_json decoder_clk_json1{};
			decoder_clk_json1["value"] = decoder_clk1;
			decoder_clk_json1["unit"] = decoder_clk1 == "N/A" ? "N/A" : "MHz";

			nlohmann::ordered_json decoder_activity_json{};
			decoder_activity_json["unit"] = "N/A";
			decoder_activity_json["value"] = "N/A";

			result["dclk_0"] = nlohmann::ordered_json::object({{"util", decoder_activity_json},
				{"clk", decoder_clk_json0} });

			result["dclk_1"] = nlohmann::ordered_json::object({{"util", decoder_activity_json},
				{"clk", decoder_clk_json1} });
		} else if (arg.output == csv) {
			formatted_string = string_format(",%s,%s\n,%s,%s", decoder_activity.c_str(), decoder_clk0.c_str(),
											 decoder_activity.c_str(), decoder_clk1.c_str());
		} else {
			std::string decoder_activity_str{"N/A"};
			std::string decoder_clock_unit0{decoder_clk0 == "N/A" ? "" : "MHz"};
			std::string decoder_clock_unit1{decoder_clk1 == "N/A" ? "" : "MHz"};
			std::string decoder_clock_str0{string_format("%s %s", decoder_clk0.c_str(), decoder_clock_unit0.c_str())};
			std::string decoder_clock_str1{string_format("%s %s", decoder_clk1.c_str(), decoder_clock_unit1.c_str())};
			formatted_string = string_format("%s, %s, %s, %s", decoder_activity_str.c_str(),
											 decoder_clock_str0.c_str(), decoder_activity_str.c_str(), decoder_clock_str1.c_str());
		}
	}

	if (arg.output == json) {
		formatted_string = result.dump(4);
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_ecc_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;
	amdsmi_error_count_t total_error_count;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_empty_ecc(arg);
		return ret;
	}

	ret = host_amdsmi_get_gpu_total_ecc_count(processor, &total_error_count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_empty_ecc(arg);
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
		if (total_error_count.correctable_count == UINT64_MAX) {
			error_count_json["total_correctable_count"] = "N/A";
		} else {
			error_count_json["total_correctable_count"] = total_error_count.correctable_count;
		}
		if (total_error_count.uncorrectable_count == UINT64_MAX) {
			error_count_json["total_correctable_count"] = "N/A";
		} else {
			error_count_json["total_correctable_count"] = total_error_count.uncorrectable_count;
		}
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

int AmdSmiApiHost::amdsmi_get_pcie_info_monitor_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_pcie_info_t pcie_info;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_pcie_info(arg);
		return ret;
	}

	ret = host_amdsmi_get_pcie_info(processor, &pcie_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_pcie_info(arg);
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
		std::string bs_unut{pcie_bandwidth == "N/A" ? "" : "Mb/s"};
		formatted_string = string_format("%s, %s %s", pcie_replay_count.c_str(), pcie_bandwidth.c_str(),
										 bs_unut.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}
