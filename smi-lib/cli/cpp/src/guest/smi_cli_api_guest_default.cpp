/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "interface/amdsmi.h"
#include "smi_cli_api_guest.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_exception.h"

#include <limits.h>
#include <cstdint>
#include <ctime>
#include <vector>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
	amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_INFO)(amdsmi_processor_handle,
	amdsmi_driver_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_LIB_VERSION)(amdsmi_version_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VBIOS_INFO)(amdsmi_processor_handle,
	amdsmi_vbios_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ASIC_INFO)(amdsmi_processor_handle,
	amdsmi_asic_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
	amdsmi_error_count_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
	amdsmi_temperature_type_t,
	amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle,
	amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_CAP_INFO)(amdsmi_processor_handle, uint32_t,
	amdsmi_power_cap_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
	amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
	amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VRAM_USAGE)(amdsmi_processor_handle,
	amdsmi_vram_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PROCESS_LIST)(amdsmi_processor_handle, uint32_t *,
	amdsmi_proc_info_t *);


extern AMDSMI_GET_LIB_VERSION guest_amdsmi_get_lib_version;
extern AMDSMI_GET_GPU_DEVICE_BDF guest_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF guest_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DRIVER_INFO guest_amdsmi_get_gpu_driver_info;
extern AMDSMI_GET_GPU_VBIOS_INFO guest_amdsmi_get_gpu_vbios_info;
extern AMDSMI_GET_GPU_ASIC_INFO guest_amdsmi_get_gpu_asic_info;
extern AMDSMI_GET_GPU_TOTAL_ECC_COUNT guest_amdsmi_get_gpu_total_ecc_count;
extern AMDSMI_GET_TEMP_METRIC guest_amdsmi_get_temp_metric;
extern AMDSMI_GET_POWER_INFO guest_amdsmi_get_power_info;
extern AMDSMI_GET_POWER_CAP_INFO guest_amdsmi_get_power_cap_info;
extern AMDSMI_GET_GPU_ACTIVITY guest_amdsmi_get_gpu_activity;
extern AMDSMI_GET_PCIE_INFO guest_amdsmi_get_pcie_info;
extern AMDSMI_GET_GPU_VRAM_USAGE guest_amdsmi_get_gpu_vram_usage;
extern AMDSMI_GET_GPU_PROCESS_LIST guest_amdsmi_get_gpu_process_list;


int AmdSmiApiGuest::amdsmi_get_default_version_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	int ret;
	amdsmi_driver_info_t driver_info;
	amdsmi_processor_handle processor;
	amdsmi_version_t version;
	amdsmi_vbios_info_t boot_firmware_info;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	std::string driver_version{"N/A"};
	std::string amdsmi_lib_ver_str{"N/A"};
	std::string boot_firmware_version{"N/A"};
	std::string tool_version{AMDSMI_TOOL_VERSION_STRING};

	ret = guest_amdsmi_get_lib_version(&version);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		amdsmi_lib_ver_str = "N/A";
	} else {
		amdsmi_lib_ver_str = string_format("%ld.%ld.%ld", version.major,
		version.minor, version.release);
	}

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_driver_info(processor, &driver_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		driver_version = "N/A";
	} else {
		driver_version = driver_info.driver_version;
	}

	ret = guest_amdsmi_get_gpu_vbios_info(processor, &boot_firmware_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		boot_firmware_version = "N/A";
	} else {
		boot_firmware_version = boot_firmware_info.boot_firmware;
	}

	formatted_string = string_format("%s,%s,%s,%s", amdsmi_lib_ver_str.c_str(), tool_version.c_str(), driver_version.c_str(), boot_firmware_version.c_str());

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_bdf_command(uint64_t index, Arguments arg, std::string &formatted_string)
{
	int ret;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_processor_handle processor;

	int64_t gpu_bdf = arg.devices[index]->get_bdf();
	tmp_bdf.as_uint = gpu_bdf;
	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_device_bdf(processor, &tmp_bdf);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		throw SmiToolSMILIBErrorException(ret);
	}

	formatted_string = convert_bdf_to_string(tmp_bdf.bdf.function_number, tmp_bdf.bdf.device_number, tmp_bdf.bdf.bus_number, tmp_bdf.bdf.domain_number);

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_gpu_name_oam_id_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	int ret;
	amdsmi_asic_info_t asic;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	std::string oam_id{"N/A"};
	std::string gpu_name{"N/A"};
	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = guest_amdsmi_get_gpu_asic_info(processor, &asic);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		oam_id = "N/A";
		gpu_name = "N/A";
	} else {
		if (asic.oam_id == UINT32_MAX || static_cast<int32_t>(asic.oam_id) == -1) {
			oam_id = "N/A";
		} else {
			oam_id = string_format("%ld", asic.oam_id);
		}
		gpu_name = asic.market_name;
	}

	formatted_string = string_format("%s,%s", gpu_name.c_str(), oam_id.c_str());

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_partition_mode_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	// Partition mode is not available on guest side
	formatted_string = "N/A";
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_uec_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	amdsmi_status_t ret;
	amdsmi_error_count_t total_error_count;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	ret = guest_amdsmi_get_gpu_total_ecc_count(processor, &total_error_count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	formatted_string = string_format("%lld", total_error_count.uncorrectable_count);

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_temperature_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	int ret;
	int64_t junction_temperature;
	int64_t vram_temperature;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A,N/A";
		return ret;
	}

	std::string junction_temperature_string{"N/A"};
	ret = guest_amdsmi_get_temp_metric(processor,
							AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
							AMDSMI_TEMP_CURRENT,
							&junction_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		junction_temperature_string = "N/A";
	} else {
		junction_temperature_string = string_format("%lld", junction_temperature);
	}

	std::string vram_temperature_string{"N/A"};
	ret = guest_amdsmi_get_temp_metric(
			  processor, AMDSMI_TEMPERATURE_TYPE_VRAM, AMDSMI_TEMP_CURRENT, &vram_temperature);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		vram_temperature_string = "N/A";
	} else {
		vram_temperature_string = string_format("%lld", vram_temperature);
	}

	formatted_string = string_format("%s,%s", junction_temperature_string.c_str(), vram_temperature_string.c_str());

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_power_usage_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_power_info_t power_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	std::string socket_power_str{"N/A"};
	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	ret = guest_amdsmi_get_power_info(processor, &power_info);
	if (ret == AMDSMI_STATUS_SUCCESS && power_info.socket_power != UINT_MAX) {
		socket_power_str = string_format("%lld", power_info.socket_power);
	}

	formatted_string = string_format("%s W", socket_power_str.c_str());

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_pcie_info_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	int ret;
	amdsmi_pcie_info_t pcie_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	ret = guest_amdsmi_get_pcie_info(processor, &pcie_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	std::string pcie_width{ string_format(
		"%d", pcie_info.pcie_static.max_pcie_width) };
	std::string pcie_info_GTs_value_string{ string_format("%d",
		pcie_info.pcie_static.max_pcie_speed / 1000) };


	formatted_string = string_format("%s,%s", pcie_width.c_str(), pcie_info_GTs_value_string.c_str());

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_utilization_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A,N/A";
		return ret;
	}

	// Guest does not have access to GPU metrics table, use activity API directly
	amdsmi_engine_usage_t engine_usage;

	std::string gfx_activity{"N/A"};
	std::string mem_activity{"N/A"};
	ret = guest_amdsmi_get_gpu_activity(processor, &engine_usage);
	if (ret == AMDSMI_STATUS_SUCCESS) {
		gfx_activity = string_format("%ld", engine_usage.gfx_activity);
		mem_activity = string_format("%ld", engine_usage.umc_activity);
	}

	std::string gfx_util_str{(gfx_activity == "N/A") ? gfx_activity : string_format("%s %%", gfx_activity.c_str())};
	std::string mem_util_str{(mem_activity == "N/A") ? mem_activity : string_format("%s %%", mem_activity.c_str())};
	formatted_string = string_format("%s,%s", gfx_util_str.c_str(), mem_util_str.c_str());

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_fb_usage_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	amdsmi_vram_usage_t vram_usage;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A,N/A";
		return ret;
	}

	ret = guest_amdsmi_get_gpu_vram_usage(processor, &vram_usage);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A,N/A";
		return ret;
	}

	formatted_string = string_format("%d/%d MB", vram_usage.vram_used, vram_usage.vram_total);

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiGuest::amdsmi_get_default_process_info_command(uint64_t processor_bdf, Arguments arg,
	std::string &formatted_string, int &proc_num, int gpu_id)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle processor_handle;
	amdsmi_bdf_t tmp_bdf;
	uint32_t max_processes = 0;
	tmp_bdf.as_uint = processor_bdf;
	std::vector<amdsmi_proc_info_t> process_info_list;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		formatted_string = "N/A";
		return ret;
	}

	std::time_t start_timestamp{std::time(nullptr)};
	while (std::difftime(std::time(nullptr), start_timestamp) < 1.1) {}
	ret = guest_amdsmi_get_gpu_process_list(processor_handle, &max_processes, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		formatted_string = "N/A";
		return ret;
	}

	amdsmi_proc_info_t *info_list = (amdsmi_proc_info_t *)malloc(max_processes * sizeof(amdsmi_proc_info_t));
	if (info_list == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = guest_amdsmi_get_gpu_process_list(processor_handle, &max_processes, info_list);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		free(info_list);
		formatted_string = "N/A";
		return ret;
	}

	for (uint32_t i = 0; i < max_processes; i++) {
		process_info_list.push_back(info_list[i]);
	}
	free(info_list);

	if (process_info_list.empty()) {
		formatted_string = "N/A";
		return INVALID_PARAM_VALUE;
	}

	for (uint32_t i = 0; i < process_info_list.size(); i++) {
		int proc_mem = convert_bytes_to_megabytes(process_info_list[i].mem);
		std::string pid{string_format("%ld", process_info_list[i].pid)};
		std::string mem = {string_format("%lld", proc_mem)};
		std::string gfx = {string_format("%lld", process_info_list[i].engine_usage.gfx)};
		std::string enc = {string_format("%lld", process_info_list[i].engine_usage.enc)};
		std::string gpu_id_str{string_format("%d", gpu_id)};
		std::string mem_unit{mem == "N/A" ? "" : "MB"};
		std::string gfx_unit{gfx == "N/A" ? "" : "%"};
		std::string enc_unit{enc == "N/A" ? "" : "%"};
		std::string mem_usage{string_format("%s %s", mem.c_str(), mem_unit.c_str())};
		std::string gfx_str{string_format("%s %s", gfx.c_str(), gfx_unit.c_str())};
		std::string enc_str{string_format("%s %s", enc.c_str(), enc_unit.c_str())};
		formatted_string += string_format("%s,%s,%s,%s,%s,%s\n", gpu_id_str.c_str(), pid.c_str(),
			process_info_list[i].name, mem_usage.c_str(), gfx_str.c_str(), enc_str.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}
