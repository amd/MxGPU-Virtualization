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
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

#include "json/json.h"

#include <sstream>
#include <climits>
#include <map>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif
#include <limits.h>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ASIC_INFO)(amdsmi_processor_handle,
		amdsmi_asic_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_UUID)(amdsmi_processor_handle, unsigned int *,
		char *);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint8_t *, uint8_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VBIOS_INFO)(amdsmi_processor_handle,
		amdsmi_vbios_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_INFO)(amdsmi_processor_handle,
		amdsmi_driver_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_FEATURE_INFO)(amdsmi_processor_handle,
		amdsmi_ras_feature_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle,
		amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
		amdsmi_temperature_type_t,
		amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
		amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PROCESS_ISOLATION)(amdsmi_processor_handle,
		uint32_t *);

extern AMDSMI_GET_PROCESSOR_HANDLES guest_amdsmi_get_processor_handles;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF guest_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_POWER_INFO guest_amdsmi_get_power_info;
extern AMDSMI_GET_TEMP_METRIC guest_amdsmi_get_temp_metric;
extern AMDSMI_GET_GPU_ASIC_INFO guest_amdsmi_get_gpu_asic_info;
extern AMDSMI_GET_GPU_DEVICE_BDF guest_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_GPU_DEVICE_UUID guest_amdsmi_get_gpu_device_uuid;
extern AMDSMI_GET_GPU_VBIOS_INFO guest_amdsmi_get_gpu_vbios_info;
extern AMDSMI_GET_GPU_DRIVER_INFO guest_amdsmi_get_gpu_driver_info;
extern AMDSMI_GET_PCIE_INFO guest_amdsmi_get_pcie_info;
extern AMDSMI_GET_GPU_RAS_FEATURE_INFO guest_amdsmi_get_gpu_ras_feature_info;

extern AMDSMI_GET_GPU_ACTIVITY guest_amdsmi_get_gpu_activity;
extern AMDSMI_GET_GPU_PROCESS_ISOLATION guest_amdsmi_get_gpu_process_isolation;

std::string guest_fill_asic_info(Arguments arg, std::string value)
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
		out = string_format("%s,%s,%s,%s,%s,%s", value.c_str(),
							value.c_str(),value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(staticAsicTemplate, value.c_str(),
							value.c_str(),value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str());
	}

	return out;
}

std::string guest_fill_vbios_info(Arguments arg, std::string value)
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

std::string guest_fill_driver_info(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json driver_json = { { "name", value.c_str() }, { "version", value.c_str()},
			{ "date", value.c_str() }
		};

		out = driver_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s", value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(
				  driverHostInfoTemplate, value.c_str(), value.c_str(), value.c_str());
	}

	return out;
}

std::string guest_fill_bus_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json max_pcie_speed{};
		max_pcie_speed["value"] = value.c_str();
		max_pcie_speed["unit"] = "N/A";
		nlohmann::ordered_json bus_json = { { "bdf", value.c_str() },
			{ "max_pcie_lanes", value.c_str() },
			{ "max_pcie_speed", max_pcie_speed },
			{ "pcie_interface_version", "N/A" },
			{ "slot_type", "N/A" }
		};

		out = bus_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s", value.c_str(),
							value.c_str(), value.c_str(),
							"N/A", "N/A");
	} else {
		out = string_format(
				  staticBusTemplate, value.c_str(), value.c_str(),
				  value.c_str(), "N/A", "N/A", "N/A");
	}

	return out;
}

std::string guest_fill_ras_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json feature_json = { { "dram_ecc", value.c_str() },
			{ "sram_ecc", value.c_str() },
			{ "poisoning", value.c_str() },
			{ "needs_reboot", value.c_str() },
			{ "ras_eeprom_version", "N/A" },
			{ "supported_ecc_correction_schema", "N/A" }
		};

		nlohmann::ordered_json ras_json;
		ras_json["feature"] = feature_json;

		out = ras_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s", value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  "N/A",
				  "N/A" );
	} else {
		out = string_format(
				  staticRasTemplateHost, value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  "N/A",
				  "N/A" );
	}

	return out;
}

std::string guest_fill_limit_info(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json limit_json = { { "max_power", value.c_str() } };
		limit_json["current_power"] = value.c_str();
		limit_json["slowdown_edge_temperature"] = value.c_str();
		limit_json["slowdown_hotspot_temperature"] = value.c_str();
		limit_json["slowdown_vram_temperature"] = value.c_str();
		limit_json["shutdown_edge_temperature"] = value.c_str();
		limit_json["shutdown_hotspot_temperature"] = value.c_str();
		limit_json["shutdown_vram_temperature"] = value.c_str();
		out = limit_json.dump();
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s,%s,%s", value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str() );
	} else {
		out = string_format(
				  staticLimitTemplate, value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  value.c_str(), value.c_str() );
	}

	return out;
}

std::string guest_fill_process_isolation(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		out = value;
	} else if (arg.output == csv) {
		out = string_format(",%s", value.c_str());
	} else {
		out = string_format(
				  staticProcessIsolate, value.c_str());
	}

	return out;
}


int AmdSmiApiGuest::amdsmi_get_asic_info_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	amdsmi_asic_info_t asic;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	int ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_asic_info(processor, &asic);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		out = guest_fill_asic_info(arg, "N/A");
		return ret;
	}

	std::string vendor_id_hex =
		string_format("0x%X", asic.vendor_id);
	std::string device_id_hex =
		string_format("0x%X", asic.device_id);
	std::string rev_id_hex =
		string_format("0x%X", asic.rev_id);
	std::string serial_id_hex =
		decimal_string_to_hex(asic.asic_serial);
	std::string oam_id{"N/A"};

	std::string subvendor_id{};
	if (asic.subvendor_id == UINT_MAX) {
		subvendor_id = "N/A";
	} else {
		subvendor_id = string_format("%ld", asic.subvendor_id);
	}

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json asic_json = { { "market_name", asic.market_name },
			{ "vendor_id", vendor_id_hex },
			{ "vendor_name", asic.vendor_name },
			{ "subvendor_id", subvendor_id.c_str() },
			{ "device_id", device_id_hex },
			{ "subsystem_id", "N/A" },
			{ "rev_id", rev_id_hex },
			{ "asic_serial", serial_id_hex },
			{ "oam_id", oam_id }
		};

		out = asic_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s",asic.market_name,
							vendor_id_hex.c_str(), asic.vendor_name, subvendor_id.c_str(), device_id_hex.c_str(),
							"N/A", rev_id_hex.c_str(),
							serial_id_hex.c_str(), oam_id.c_str());
	} else {
		out = string_format(
				  staticAsicTemplate, asic.market_name, vendor_id_hex.c_str(), asic.vendor_name, subvendor_id.c_str(),
				  device_id_hex.c_str(), "N/A", rev_id_hex.c_str(), serial_id_hex.c_str(), oam_id.c_str());
	}

	return ret;
}

int AmdSmiApiGuest::amdsmi_get_vbios_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_vbios_info_t vbios_info;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_vbios_info(processor, &vbios_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_vbios_info(arg, "N/A");
		return ret;
	}

	std::string vbios_name = vbios_info.name ? vbios_info.name : "N/A";

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json vbios_json = { { "name", vbios_name },
			{ "build_date", vbios_info.build_date },
			{ "part_number", vbios_info.part_number },
			{ "version", vbios_info.version }
		};

		formatted_string = vbios_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s", vbios_name.c_str(), vbios_info.build_date,
							   vbios_info.part_number, vbios_info.version);
	} else {
		formatted_string = string_format(
							   staticVbiosTemplate, vbios_name.c_str(), vbios_info.build_date,
							   vbios_info.part_number, vbios_info.version);
	}

	return ret;
}

int AmdSmiApiGuest::amdsmi_get_limit_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	if (AmdSmiPlatform::getInstance().is_guest()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}
	int ret;

	amdsmi_power_info_t power_limit;
	int64_t therm_limit_edge;
	int64_t therm_limit_junction;

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		guest_fill_limit_info(arg, "N/A");
		return ret;
	}

	std::string max_board_power_limit_string{};
	std::string power_limit_string{};
	ret = guest_amdsmi_get_power_info(processor, &power_limit);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		max_board_power_limit_string = "N/A";
		power_limit_string = "N/A";
	} else {
		max_board_power_limit_string = string_format(
										   "%ld", power_limit.max_board_power_limit);
		power_limit_string = string_format(
								 "%ld", power_limit.power_limit);
	}

	ret = guest_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_CRITICAL, &therm_limit_edge);
	std::string therm_limit_edge_string;
	if (ret != AMDSMI_STATUS_SUCCESS) {
		therm_limit_edge_string = "N/A";
	} else {
		if(therm_limit_edge == UINT_MAX) {
			therm_limit_edge_string = "N/A";
		} else {
			therm_limit_edge_string = string_format("%lld", therm_limit_edge);
		}
	}

	ret = guest_amdsmi_get_temp_metric(processor,
									   AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
									   AMDSMI_TEMP_CRITICAL,
									   &therm_limit_junction);
	std::string therm_limit_junction_string;
	if (ret != AMDSMI_STATUS_SUCCESS) {
		therm_limit_junction_string = "N/A";
	} else {
		if(therm_limit_junction == UINT_MAX) {
			therm_limit_junction_string = "N/A";
		} else {
			therm_limit_junction_string = string_format("%lld", therm_limit_junction);
		}
	}

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json limit_json = { { "max_power", power_limit.max_board_power_limit } };
		limit_json["current_power"] = power_limit.power_limit;
		limit_json["slowdown_edge_temperature"] = therm_limit_edge_string.c_str();
		limit_json["slowdown_hotspot_temperature"] = therm_limit_junction;
		limit_json["slowdown_vram_temperature"] = "N/A";
		limit_json["shutdown_edge_temperature"] = "N/A";
		limit_json["shutdown_hotspot_temperature"] = "N/A";
		limit_json["shutdown_vram_temperature"] = "N/A";
		formatted_string = limit_json.dump();
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s,%s,%s,%s", max_board_power_limit_string.c_str(),
							   power_limit_string.c_str(), therm_limit_edge_string.c_str(),
							   therm_limit_junction_string.c_str(), "N/A", "N/A", "N/A", "N/A");
	} else {
		std::string max_board_power_limit_string_unit = max_board_power_limit_string == "N/A" ? "" : "W";
		std::string power_limit_string_unit = power_limit_string == "N/A" ? "" : "W";
		std::string therm_limit_edge_string_unit = therm_limit_edge_string == "N/A" ? "" : "C";
		std::string therm_limit_junction_string_unit = therm_limit_junction_string == "N/A" ? "" : "C";
		formatted_string = string_format(
							   staticLimitTemplate, max_board_power_limit_string.c_str(),
							   max_board_power_limit_string_unit.c_str(), "N/A", "",
							   power_limit_string.c_str(), power_limit_string_unit.c_str(),
							   therm_limit_edge_string.c_str(), therm_limit_edge_string_unit.c_str(),
							   therm_limit_junction_string.c_str(), therm_limit_junction_string_unit.c_str(), "N/A", "", "N/A", "",
							   "N/A", "", "N/A", "");
	}

	return ret;
}


int AmdSmiApiGuest::amdsmi_get_driver_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_driver_info_t driver_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_driver_info(processor, &driver_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_driver_info(arg, "N/A");
		return ret;
	}

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json driver_json = { { "name", driver_info.driver_name }, { "version", driver_info.driver_version },
			{ "date", driver_info.driver_date }
		};

		formatted_string = driver_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s", driver_info.driver_name, driver_info.driver_version, driver_info.driver_date);
	} else {
		formatted_string = string_format(
							   driverHostInfoTemplate, driver_info.driver_name, driver_info.driver_version,
							   driver_info.driver_date);
	}

	return ret;

}

int AmdSmiApiGuest::amdsmi_get_bus_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_pcie_info_t pcie_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_pcie_info(processor, &pcie_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_bus_info(arg, "N/A");
		return ret;
	}

	amdsmi_bdf_t bdf;
	ret = guest_amdsmi_get_gpu_device_bdf(processor, &bdf);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	std::string bdf_string = string_format(
								 "%04x:%02x:%02x.%01x", bdf.domain_number, bdf.bus_number, bdf.device_number,
								 bdf.function_number);
	std::string pcie_width{ string_format(
								"%d", pcie_info.pcie_static.max_pcie_width) };
	std::string pcie_info_GTs_value_string{
		string_format("%d", pcie_info.pcie_static.max_pcie_speed / 1000)
	};

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json max_pcie_speed{};
		max_pcie_speed["value"] = pcie_info.pcie_static.max_pcie_speed / 1000;
		max_pcie_speed["unit"] = pcie_info_GTs_value_string == "N/A" ? "N/A" : "GT/s";
		nlohmann::ordered_json bus_json = { { "bdf", bdf_string },
			{ "max_pcie_width", pcie_info.pcie_static.max_pcie_width },
			{ "max_pcie_speed", max_pcie_speed },
			{ "pcie_interface_version", "N/A" },
			{ "slot_type", "N/A" }
		};

		formatted_string = bus_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s,%s,%s,%s", bdf_string.c_str(),
										 pcie_width.c_str(),pcie_info_GTs_value_string.c_str(),
										 "N/A", "N/A");
	} else {
		std::string pcie_info_GTs_value_string_unit = pcie_info_GTs_value_string == "N/A" ? "" : "GT/s";
		formatted_string = string_format(
							   staticBusTemplate, bdf_string.c_str(), pcie_width.c_str(),
							   pcie_info_GTs_value_string.c_str(), pcie_info_GTs_value_string_unit.c_str(), "N/A",
							   "N/A");
	}

	return ret;
}

int AmdSmiApiGuest::amdsmi_get_ras_info_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;

	amdsmi_ras_feature_t ras_feature;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	if (AmdSmiPlatform::getInstance().is_guest() || AmdSmiPlatform::getInstance().is_baremetal()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_ras_feature_info(processor,
		  &ras_feature);

	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_ras_info(arg, "N/A");
		return ret;
	}

	bool dram_ecc = ras_feature.ras_info.dram_ecc;
	bool sram_ecc = ras_feature.ras_info.sram_ecc;
	bool poisoning = ras_feature.ras_info.poisoning;
	std::string ras_eeprom_version_str{ string_format(
											"%u", ras_feature.ras_eeprom_version) };
	std::string ecc_correction_schema_flag_str;
	// transform_ecc_correction_schema(
	// 	ras_feature.ecc_correction_schema_flag, ecc_correction_schema_flag_str);

	std::string dram_ecc_str{ ras_feature.ras_info.dram_ecc ? "True" : "False" };
	std::string sram_ecc_str{ ras_feature.ras_info.sram_ecc ? "True" : "False" };
	std::string poisoning_str{ ras_feature.ras_info.poisoning ? "True" : "False" };
	std::string need_reboot_str{ ras_feature.needs_reboot ? "True" : "False" };

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json feature_json = { { "dram_ecc", dram_ecc },
			{ "sram_ecc", sram_ecc },
			{ "poisoning_scheme", poisoning },
			{ "needs_reboot", ras_feature.needs_reboot },
			{ "eeprom_version", "N/A" },
			{ "supported_ecc_correction_schema", "N/A" }
		};

		nlohmann::ordered_json ras_json;
		ras_json["feature"] = feature_json;

		formatted_string = ras_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s,%s", dram_ecc_str.c_str(), sram_ecc_str.c_str(),
							   poisoning_str.c_str(), need_reboot_str.c_str(),
							   "N/A",
							   "N/A" );
	} else {
		formatted_string = string_format(
							   staticRasTemplateHost, dram_ecc_str.c_str(), sram_ecc_str.c_str(),
							   poisoning_str.c_str(), need_reboot_str.c_str(),
							   "N/A",
							   "N/A" );
	}

	return ret;
}

int AmdSmiApiGuest::amdsmi_get_process_isolation(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;

	uint32_t pisolate;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_process_isolation(processor, &pisolate);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_process_isolation(arg, "N/A");
		return ret;
	}

	std::string pisolate_str = pisolate == 0 ? "Disabled" : "Enabled";

	if (arg.output == json) {
		formatted_string = pisolate_str;
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s", pisolate_str.c_str());
	} else {
		formatted_string = string_format(
							   staticProcessIsolate, pisolate_str.c_str());
	}

	return ret;
}
