/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_MODEL)(amdsmi_processor_handle,
		amdsmi_driver_model_type_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_FEATURE_INFO)(amdsmi_processor_handle,
		amdsmi_ras_feature_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle,
		amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_CAP_INFO)(amdsmi_processor_handle,
		 uint32_t, amdsmi_power_cap_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
		amdsmi_temperature_type_t,
		amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
		amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PROCESS_ISOLATION)(amdsmi_processor_handle,
		uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VIRTUALIZATION_MODE)(amdsmi_processor_handle,
		amdsmi_virtualization_mode_t *);

extern AMDSMI_GET_PROCESSOR_HANDLES guest_amdsmi_get_processor_handles;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF guest_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_POWER_INFO guest_amdsmi_get_power_info;
extern AMDSMI_GET_POWER_CAP_INFO guest_amdsmi_get_power_cap_info;
extern AMDSMI_GET_TEMP_METRIC guest_amdsmi_get_temp_metric;
extern AMDSMI_GET_GPU_ASIC_INFO guest_amdsmi_get_gpu_asic_info;
extern AMDSMI_GET_GPU_DEVICE_BDF guest_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_GPU_DEVICE_UUID guest_amdsmi_get_gpu_device_uuid;
extern AMDSMI_GET_GPU_VBIOS_INFO guest_amdsmi_get_gpu_vbios_info;
extern AMDSMI_GET_GPU_DRIVER_INFO guest_amdsmi_get_gpu_driver_info;
extern AMDSMI_GET_GPU_DRIVER_MODEL guest_amdsmi_get_gpu_driver_model;
extern AMDSMI_GET_PCIE_INFO guest_amdsmi_get_pcie_info;
extern AMDSMI_GET_GPU_RAS_FEATURE_INFO guest_amdsmi_get_gpu_ras_feature_info;

extern AMDSMI_GET_GPU_ACTIVITY guest_amdsmi_get_gpu_activity;
extern AMDSMI_GET_GPU_PROCESS_ISOLATION guest_amdsmi_get_gpu_process_isolation;
extern AMDSMI_GET_GPU_VIRTUALIZATION_MODE guest_amdsmi_get_gpu_virtualization_mode;

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
			{ "oam_id", value.c_str() },
			{ "num_of_compute_units", value.c_str() },
			{ "target_graphics_version", value.c_str() }
		};

		out = asic_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", value.c_str(),
							value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(staticAsicTemplate, value.c_str(),
							value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str(), value.c_str());
		out += string_format(staticAsicTargetGraphicsVersionTemplate, value.c_str());
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
			{ "version", value.c_str() },
			{ "boot_firmware", value.c_str() }
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

std::string guest_fill_driver_info(Arguments arg, std::string value)
{
	std::string out{};
	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json driver_json = { { "name", value.c_str() }, { "version", value.c_str()},
			{ "date", value.c_str() }, { "model", value.c_str() }
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
			{ "pcie_interface_version", value.c_str() },
			{ "slot_type", value.c_str() },
			{ "max_pcie_interface_version", value.c_str() }
		};

		out = bus_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s", value.c_str(),
							value.c_str(), value.c_str(),
							value.c_str(), value.c_str(), value.c_str());
	} else {
		out = string_format(
				  staticBusTemplate, value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(), value.c_str());
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
			{ "ecc_correction_schema", "N/A" }
		};

		nlohmann::ordered_json ras_json;
		ras_json["feature"] = feature_json;

		out = ras_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s,%s", value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(),
				  "N/A",
				  "N/A" );
	} else {
		out = string_format(
				  staticRasTemplateHost, value.c_str(), value.c_str(), value.c_str(),
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

		nlohmann::ordered_json ppt_json{};
		ppt_json["max_power"] = value.c_str();
		ppt_json["min_power"] = value.c_str();
		ppt_json["socket_power"] = value.c_str();

		nlohmann::ordered_json limit_json = { { "ppt0", ppt_json } };
		limit_json["ppt1"] = ppt_json;
		limit_json["slowdown_edge_temperature"] = value.c_str();
		limit_json["slowdown_hotspot_temperature"] = value.c_str();
		limit_json["slowdown_vram_temperature"] = value.c_str();
		limit_json["shutdown_edge_temperature"] = value.c_str();
		limit_json["shutdown_hotspot_temperature"] = value.c_str();
		limit_json["shutdown_vram_temperature"] = value.c_str();
		out = limit_json.dump();
	} else if (arg.output == csv) {
		out = string_format(
				  ",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str() );
	} else {
		out = string_format(
				  staticLimitTemplate,
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str(),
				  value.c_str(), value.c_str(), value.c_str(), value.c_str() );
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

std::string guest_fill_virtualization_mode(Arguments arg, std::string value)
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
	std::string serial_id_hex;
	try {
		uint64_t serial_val = std::stoull(asic.asic_serial);
		serial_id_hex = (serial_val == UINT64_MAX) ? "N/A" : decimal_string_to_hex(asic.asic_serial);
	} catch (...) {
		serial_id_hex = "N/A";
	}
	std::string oam_id = static_cast<int32_t>(asic.oam_id) == -1 ? "N/A" : string_format("%d", asic.oam_id);
	std::string num_of_compute_units = asic.num_of_compute_units == -1 ? "N/A" : string_format("%d", asic.num_of_compute_units);
	std::string subsystem_id = asic.subsystem_id == -1 ? "N/A" : string_format("%d", asic.subsystem_id);

	std::string subvendor_id{};
	if (asic.subvendor_id == UINT_MAX) {
		subvendor_id = "N/A";
	} else {
		subvendor_id = string_format("%ld", asic.subvendor_id);
	}

	// Expose target graphics version in the "gfx" string form. The raw value is
	// packed as (major << 16) | (minor << 8) | step, matching KMD's
	// GfxIpFullVersion. E.g. 0x0B0000 -> "gfx1100", 0x0B0501 -> "gfx1151".
	std::string target_graphics_version;
	if (asic.target_graphics_version == UINT64_MAX) {
		target_graphics_version = "N/A";
	} else {
		uint64_t gfx_major = (asic.target_graphics_version >> 16) & 0xFF;
		uint64_t gfx_minor = (asic.target_graphics_version >> 8) & 0xFF;
		uint64_t gfx_step = asic.target_graphics_version & 0xFF;
		target_graphics_version = string_format("gfx%llu%llu%llx", gfx_major, gfx_minor, gfx_step);
	}

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json asic_json = { { "market_name", asic.market_name },
			{ "vendor_id", vendor_id_hex },
			{ "vendor_name", asic.vendor_name },
			{ "subvendor_id", subvendor_id.c_str() },
			{ "device_id", device_id_hex },
			{ "subsystem_id", subsystem_id.c_str() },
			{ "rev_id", rev_id_hex },
			{ "asic_serial", serial_id_hex },
			{ "oam_id", oam_id.c_str() },
			{ "num_of_compute_units", num_of_compute_units.c_str() },
			{ "target_graphics_version", target_graphics_version.c_str() }
		};

		out = asic_json.dump(4);
	} else if (arg.output == csv) {
		out = string_format(",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", asic.market_name,
							vendor_id_hex.c_str(), asic.vendor_name, subvendor_id.c_str(), device_id_hex.c_str(),
							subsystem_id.c_str(), rev_id_hex.c_str(),
							serial_id_hex.c_str(), oam_id.c_str(), num_of_compute_units.c_str(),
							target_graphics_version.c_str());
	} else {
		out = string_format(
				  staticAsicTemplate, asic.market_name, vendor_id_hex.c_str(), asic.vendor_name, subvendor_id.c_str(),
				  device_id_hex.c_str(), subsystem_id.c_str(), rev_id_hex.c_str(), serial_id_hex.c_str(), oam_id.c_str(), num_of_compute_units.c_str());
		out += string_format(staticAsicTargetGraphicsVersionTemplate, target_graphics_version.c_str());
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
			{ "version", vbios_info.version },
			{ "boot_firmware", vbios_info.boot_firmware }
		};

		formatted_string = vbios_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s", vbios_name.c_str(), vbios_info.build_date,
							   vbios_info.part_number, vbios_info.version, vbios_info.boot_firmware);
	} else {
		formatted_string = string_format(
							   staticVbiosTemplate, vbios_name.c_str(), vbios_info.build_date,
							   vbios_info.part_number, vbios_info.version, vbios_info.boot_firmware);
	}

	return ret;
}

int AmdSmiApiGuest::amdsmi_get_limit_info_command(uint64_t processor_bdf, Arguments &arg,
		std::string &formatted_string)
{
	if (AmdSmiPlatform::getInstance().is_guest()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}
	int ret;

	amdsmi_power_info_t power_limit;
	amdsmi_power_cap_info_t power_cap;
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
		power_limit_string = "N/A";
	} else {
		power_limit_string = string_format(
								 "%ld", power_limit.power_limit);
	}
	ret = guest_amdsmi_get_power_cap_info(processor, 0, &power_cap);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		max_board_power_limit_string = "N/A";
	} else {
		max_board_power_limit_string = string_format(
										   "%ld", power_cap.power_cap);
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

		nlohmann::ordered_json ppt0_json{};
		ppt0_json["max_power"] = power_cap.power_cap;
		ppt0_json["min_power"] = "N/A";
		ppt0_json["socket_power"] = power_limit.power_limit;
		nlohmann::ordered_json ppt1_json{};
		ppt1_json["max_power"] = "N/A";
		ppt1_json["min_power"] = "N/A";
		ppt1_json["socket_power"] = "N/A";

		nlohmann::ordered_json limit_json = { { "ppt0", ppt0_json } };
		limit_json["ppt1"] = ppt1_json;
		limit_json["slowdown_edge_temperature"] = therm_limit_edge_string.c_str();
		limit_json["slowdown_hotspot_temperature"] = therm_limit_junction;
		limit_json["slowdown_vram_temperature"] = "N/A";
		limit_json["shutdown_edge_temperature"] = "N/A";
		limit_json["shutdown_hotspot_temperature"] = "N/A";
		limit_json["shutdown_vram_temperature"] = "N/A";
		formatted_string = limit_json.dump();
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
							   max_board_power_limit_string.c_str(), "N/A",
							   power_limit_string.c_str(), "N/A", "N/A", "N/A",
							   therm_limit_edge_string.c_str(),
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
							   "N/A", "", "N/A", "", "N/A", "",
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
	amdsmi_driver_model_type_t driver_model_type;
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

	ret = guest_amdsmi_get_gpu_driver_model(processor, &driver_model_type);
	if (ret == AMDSMI_STATUS_NOT_SUPPORTED) {
		driver_model_type = AMDSMI_DRIVER_MODEL_TYPE__MAX;  //using max as default value
		ret = AMDSMI_STATUS_SUCCESS;
	} else if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_driver_info(arg, "N/A");
		return ret;
	}

	std::string driver_model;
	switch (driver_model_type) {
	case AMDSMI_DRIVER_MODEL_TYPE_WDDM:
		driver_model = "WDDM";
		break;
	case AMDSMI_DRIVER_MODEL_TYPE_WDM:
		driver_model = "WDM";
		break;
	case AMDSMI_DRIVER_MODEL_TYPE_MCDM:
		driver_model = "MCDM";
		break;
	default:
		driver_model = "N/A";
		break;
	}

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};

		nlohmann::ordered_json driver_json = { { "name", driver_info.driver_name }, { "version", driver_info.driver_version },
			{ "date", driver_info.driver_date }, { "model", driver_model.c_str() }
		};

		formatted_string = driver_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s", driver_info.driver_name, driver_info.driver_version, driver_info.driver_date, driver_model.c_str());
	} else {
		formatted_string = string_format(
							   driverHostInfoTemplate, driver_info.driver_name, driver_info.driver_version,
							   driver_info.driver_date, driver_model.c_str());
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
	std::string max_pcie_width{ string_format(
								"%d", pcie_info.pcie_static.max_pcie_width) };
	std::string pcie_info_GTs_value_string{
		string_format("%d", pcie_info.pcie_static.max_pcie_speed / 1000)
	};

	std::string pcie_interface_version;
	if (pcie_info.pcie_static.pcie_interface_version != -1)
		pcie_interface_version = string_format("%d", pcie_info.pcie_static.pcie_interface_version);
	else
		pcie_interface_version = "N/A";

	std::string slot_type = convert_slot_type_to_string(pcie_info.pcie_static.slot_type);

	std::string max_pcie_interface_version;
	if (pcie_info.pcie_static.max_pcie_interface_version != -1)
		max_pcie_interface_version = string_format("%d", pcie_info.pcie_static.max_pcie_interface_version);
	else
		max_pcie_interface_version = "N/A";

	if (arg.output == json) {
		nlohmann::ordered_json values_json{};
		nlohmann::ordered_json max_pcie_speed{};
		max_pcie_speed["value"] = pcie_info.pcie_static.max_pcie_speed / 1000;
		max_pcie_speed["unit"] = pcie_info_GTs_value_string == "N/A" ? "N/A" : "GT/s";
		nlohmann::ordered_json bus_json = { { "bdf", bdf_string },
			{ "max_pcie_width", pcie_info.pcie_static.max_pcie_width },
			{ "max_pcie_speed", max_pcie_speed },
			{ "pcie_interface_version", pcie_info.pcie_static.pcie_interface_version },
			{ "slot_type", slot_type },
			{ "max_pcie_interface_version", pcie_info.pcie_static.max_pcie_interface_version }
		};

		formatted_string = bus_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(",%s,%s,%s,%s,%s,%s", bdf_string.c_str(),
										 max_pcie_width.c_str(),pcie_info_GTs_value_string.c_str(),
										 pcie_interface_version.c_str(), slot_type.c_str(),
										 max_pcie_interface_version.c_str());
	} else {
		std::string pcie_info_GTs_value_string_unit = pcie_info_GTs_value_string == "N/A" ? "" : "GT/s";
		formatted_string = string_format(
							   staticBusTemplate, bdf_string.c_str(), max_pcie_width.c_str(),
							   pcie_info_GTs_value_string.c_str(), pcie_info_GTs_value_string_unit.c_str(),
							   pcie_interface_version.c_str(), slot_type.c_str(),
							   max_pcie_interface_version.c_str());
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
	std::string ras_eeprom_version_str{ string_format("%u", ras_feature.ras_eeprom_version) };

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
			{ "ecc_correction_schema", "N/A" }
		};

		nlohmann::ordered_json ras_json;
		ras_json["feature"] = feature_json;

		formatted_string = ras_json.dump(4);
	} else if (arg.output == csv) {
		formatted_string = string_format(
							   ",%s,%s,%s,%s,%s,%s,%s", dram_ecc_str.c_str(), sram_ecc_str.c_str(),
							   poisoning_str.c_str(), need_reboot_str.c_str(), "N/A",
							   "N/A",
							   "N/A" );
	} else {
		formatted_string = string_format(
							   staticRasTemplateHost, dram_ecc_str.c_str(), sram_ecc_str.c_str(),
							   poisoning_str.c_str(), need_reboot_str.c_str(), "N/A",
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

int AmdSmiApiGuest::amdsmi_get_virtualization_mode_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_virtualization_mode_t mode;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_virtualization_mode(processor, &mode);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = guest_fill_virtualization_mode(arg, "N/A");
		return ret;
	}

	std::string virtualization_mode_string;
	switch (mode)
	{
	case AMDSMI_VIRTUALIZATION_MODE_HOST:
		virtualization_mode_string = "HOST";
		break;
	case AMDSMI_VIRTUALIZATION_MODE_GUEST:
		virtualization_mode_string = "GUEST";
		break;
	case AMDSMI_VIRTUALIZATION_MODE_PASSTHROUGH:
		virtualization_mode_string = "PASSTHROUGH";
		break;
	case AMDSMI_VIRTUALIZATION_MODE_BAREMETAL:
		virtualization_mode_string = "BAREMETAL";
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
		formatted_string = string_format(staticVirtualizationModeTemplate, virtualization_mode_string.c_str());
	}

	return ret;
}
