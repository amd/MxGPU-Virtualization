/* * Copyright (C) 2026 Advanced Micro Devices. All rights reserved.
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
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

#include <limits.h>

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
typedef amdsmi_status_t (*AMDSMI_GET_MEMORY_PARTITION_CONFIG)(amdsmi_processor_handle,
	amdsmi_memory_partition_config_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CURR_ACCELERATOR_PARTITION)(amdsmi_processor_handle,
	amdsmi_accelerator_partition_profile_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
	amdsmi_error_count_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
	amdsmi_temperature_type_t,
	amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle,
	amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_CAP_INFO)(amdsmi_processor_handle, uint32_t,
	amdsmi_power_cap_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_METRICS)(amdsmi_processor_handle, uint32_t *,
	amdsmi_metric_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
	amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint32_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_PARTITION_INFO)(amdsmi_processor_handle, unsigned int,
		amdsmi_partition_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_BDF)(amdsmi_vf_handle_t, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_BDF)(amdsmi_bdf_t,
	amdsmi_vf_handle_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GUEST_DATA)(amdsmi_vf_handle_t,
	amdsmi_guest_data_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_INFO)(amdsmi_vf_handle_t, amdsmi_vf_info_t *);

extern AMDSMI_GET_LIB_VERSION host_amdsmi_get_lib_version;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DRIVER_INFO host_amdsmi_get_gpu_driver_info;
extern AMDSMI_GET_GPU_VBIOS_INFO host_amdsmi_get_gpu_vbios_info;
extern AMDSMI_GET_GPU_ASIC_INFO host_amdsmi_get_gpu_asic_info;
extern AMDSMI_GET_MEMORY_PARTITION_CONFIG host_amdsmi_get_gpu_memory_partition_config;
extern AMDSMI_GET_CURR_ACCELERATOR_PARTITION host_amdsmi_get_partition_profile;
extern AMDSMI_GET_GPU_TOTAL_ECC_COUNT host_amdsmi_get_gpu_total_ecc_count;
extern AMDSMI_GET_TEMP_METRIC host_amdsmi_get_temp_metric;
extern AMDSMI_GET_POWER_INFO host_amdsmi_get_power_info;
extern AMDSMI_GET_POWER_CAP_INFO host_amdsmi_get_power_cap_info;
extern AMDSMI_GET_GPU_METRICS host_amdsmi_get_gpu_metrics;
extern AMDSMI_GET_GPU_ACTIVITY host_amdsmi_get_gpu_activity;
extern AMDSMI_GET_NUM_VF host_amdsmi_get_num_vf;
extern AMDSMI_GET_VF_PARTITION_INFO host_amdsmi_get_vf_partition_info;
extern AMDSMI_GET_VF_BDF host_amdsmi_get_vf_bdf;
extern AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
extern AMDSMI_GET_GUEST_DATA host_amdsmi_get_guest_data;
extern AMDSMI_GET_VF_INFO host_amdsmi_get_vf_info;

int AmdSmiApiHost::amdsmi_get_default_version_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
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

	ret = host_amdsmi_get_lib_version(&version);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		amdsmi_lib_ver_str = "N/A";
	} else {
		amdsmi_lib_ver_str = string_format("%ld.%ld.%ld", version.major,
		version.minor, version.release);
	}

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_driver_info(processor, &driver_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		driver_version = "N/A";
	} else {
		driver_version = driver_info.driver_version;
	}

	ret = host_amdsmi_get_gpu_vbios_info(processor, &boot_firmware_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		boot_firmware_version = "N/A";
	} else {
		boot_firmware_version = boot_firmware_info.boot_firmware;
	}

	formatted_string = string_format("%s,%s,%s,%s", amdsmi_lib_ver_str.c_str(), tool_version.c_str(),
		driver_version.c_str(), boot_firmware_version.c_str());

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_default_bdf_command(uint64_t index, Arguments arg, std::string &formatted_string)
{
	int ret;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_processor_handle processor;

	int64_t gpu_bdf = arg.devices[index]->get_bdf();
	tmp_bdf.as_uint = gpu_bdf;
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_device_bdf(processor, &tmp_bdf);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		throw SmiToolSMILIBErrorException(ret);
	}

	formatted_string = convert_bdf_to_string(tmp_bdf.bdf.function_number, tmp_bdf.bdf.device_number,
			tmp_bdf.bdf.bus_number, tmp_bdf.bdf.domain_number);

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_default_vf_data_command(uint64_t index, Arguments arg, uint64_t &num_vfs,
	std::vector<std::vector<std::string>> &vf_data)
{
	amdsmi_processor_handle processor;
	amdsmi_vf_handle_t vf_handle;
	amdsmi_guest_data_t guest_data;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_bdf_t vf_bdf;
	uint32_t num_vf_supported;
	uint32_t num_vf_enabled;
	int ret{};
	amdsmi_partition_info_t partitions[AMDSMI_MAX_VF_COUNT];

	// Initialize num_vfs to 0 to handle the case where no VFs are enabled
	num_vfs = 0;

	int64_t gpu_bdf = arg.devices[index]->get_bdf();
	tmp_bdf.as_uint = gpu_bdf;
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_num_vf(processor, &num_vf_enabled, &num_vf_supported);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	ret = host_amdsmi_get_vf_partition_info(processor, num_vf_enabled, partitions);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	num_vfs = num_vf_enabled;
	for (uint8_t i = 0; i < num_vf_enabled; i++) {
		std::vector<std::string> vf_data_row;
		ret = host_amdsmi_get_vf_bdf(partitions[i].id, &vf_bdf);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			throw SmiToolSMILIBErrorException(ret);
		}
		std::string vfbdf{ convert_bdf_to_string(vf_bdf.bdf.function_number, vf_bdf.bdf.device_number,
				vf_bdf.bdf.bus_number, vf_bdf.bdf.domain_number) };
		vf_data_row.push_back(vfbdf);

		ret = host_amdsmi_get_vf_handle_from_bdf(vf_bdf, &vf_handle);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			return ret;
		}

		ret = host_amdsmi_get_guest_data(vf_handle, &guest_data);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			vf_data_row.push_back("N/A");
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

		amdsmi_vf_info_t config;

		ret = host_amdsmi_get_vf_info(vf_handle, &config);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			vf_data_row.push_back("N/A");
			return ret;
		}

		std::string fb_offset_str =
			string_format("%u", config.fb.fb_offset);
		std::string fb_size_str =
			string_format("%u", config.fb.fb_size);

		vf_data_row.push_back(fb_offset_str);
		vf_data_row.push_back(fb_usage_str);
		vf_data_row.push_back(fb_size_str);
		vf_data_row.push_back(driver_version_str);
		vf_data.push_back(vf_data_row);
		vf_data_row.clear();
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_default_gpu_name_oam_id_command(uint64_t processor_bdf, Arguments arg,
	std::string &formatted_string)
{
	int ret;
	amdsmi_asic_info_t asic;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	std::string oam_id{"N/A"};
	std::string gpu_name{"N/A"};
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_asic_info(processor, &asic);
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

int AmdSmiApiHost::amdsmi_get_default_partition_mode_command(uint64_t processor_bdf, Arguments arg,
	std::string &formatted_string)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;


	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	amdsmi_memory_partition_config_t memory_partition_config;
	ret = host_amdsmi_get_gpu_memory_partition_config(processor, &memory_partition_config);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}
	std::string curr_mp_setting_str{"N/A"};
	get_string_from_enum_mp_setting(memory_partition_config.mp_mode, curr_mp_setting_str);

	amdsmi_accelerator_partition_profile_t curr_profile;
	uint32_t partition_ids[AMDSMI_MAX_ACCELERATOR_PARTITIONS];
	ret = host_amdsmi_get_partition_profile(processor, &curr_profile, partition_ids);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	std::string curr_partition_type_str{"N/A"};
	get_string_from_enum_accelerator_partition_type(curr_profile.profile_type, curr_partition_type_str);

	formatted_string = string_format("%s,%s", curr_partition_type_str.c_str(), curr_mp_setting_str.c_str());

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_default_uec_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	amdsmi_status_t ret;
	amdsmi_error_count_t total_error_count;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	ret = host_amdsmi_get_gpu_total_ecc_count(processor, &total_error_count);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	formatted_string = string_format("%lld", total_error_count.uncorrectable_count);

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_default_temperature_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	int ret;
	int64_t junction_temperature;
	int64_t vram_temperature;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A,N/A";
		return ret;
	}

	std::string junction_temperature_string{"N/A"};
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

	formatted_string = string_format("%s,%s", junction_temperature_string.c_str(), vram_temperature_string.c_str());

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_default_power_usage_command(uint64_t processor_bdf, Arguments arg, std::string &formatted_string)
{
	amdsmi_status_t ret;

	amdsmi_power_info_t power_info;
	amdsmi_power_cap_info_t power_cap_info;
	uint32_t sensor_ind = 0;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	std::string socket_power_str{"N/A"};
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	ret = host_amdsmi_get_power_info(processor, &power_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	if (power_info.socket_power != UINT_MAX) {
		socket_power_str = string_format("%lld", power_info.socket_power);
	}

	ret = host_amdsmi_get_power_cap_info(processor, sensor_ind, &power_cap_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A";
		return ret;
	}

	std::string max_power_cap_string = power_cap_info.max_power_cap == -1 ?
						"N/A" : string_format("%lld", power_cap_info.max_power_cap);

	if (max_power_cap_string != "N/A") {
		formatted_string = string_format("%s/%s W", socket_power_str.c_str(), max_power_cap_string.c_str());
	} else {
		formatted_string = string_format("%s W", socket_power_str.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_default_utilization_command(uint64_t processor_bdf, Arguments arg,std::string &formatted_string)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	bool setup_template{true};
	bool metric_table{false};

	std::vector<amdsmi_metric_t> gfx_util{};
	std::vector<amdsmi_metric_t> mem_util{};

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = "N/A,N/A";
		return ret;
	}

	if (AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200() ||
		AmdSmiPlatform::getInstance().is_mi350()) {
		amdsmi_metric_t *metrics;
		uint32_t metric_size = AMDSMI_MAX_NUM_METRICS;

		ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, nullptr);
		if (ret == AMDSMI_STATUS_SUCCESS) {
			metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t)*metric_size);
			if (metrics == nullptr) {
				throw SmiToolNotEnoughMemException();
			}
			ret = host_amdsmi_get_gpu_metrics(processor, &metric_size, &metrics[0]);

			if (ret != AMDSMI_STATUS_SUCCESS) {
				metric_table = false;
				setup_template = true;
			} else {
				metric_table = true;
				for (int i = 0; i < metric_size; i++) {
					if ((metrics[i].name == AMDSMI_METRIC_NAME_USAGE_GFX)
							&& !(metrics[i].flags & AMDSMI_METRIC_TYPE_ACC)) {
						gfx_util.push_back(metrics[i]);
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
		if (gfx_util.size() == 0) {
			setup_template = true;
		} else {
			setup_template = false;

			std::string gfx_util_str{"N/A"};
			std::string mem_util_str{"N/A"};

			if (gfx_util.empty() == false) {
				gfx_util_str = (gfx_util[0].val == UINT64_MAX) ? "N/A" : string_format("%llu %%", gfx_util[0].val);
			}
			if (mem_util.empty() == false) {
				mem_util_str = (mem_util[0].val == UINT64_MAX) ? "N/A" : string_format("%llu %%", mem_util[0].val);
			}

			formatted_string = string_format("%s,%s", gfx_util_str.c_str(), mem_util_str.c_str());
		}
	}

	if (setup_template) {
		amdsmi_engine_usage_t engine_usage;

		std::string gfx_activity{};
		std::string mem_activity{};
		ret = host_amdsmi_get_gpu_activity(processor, &engine_usage);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			gfx_activity = "N/A";
			mem_activity = "N/A";
		} else {
			gfx_activity = string_format("%ld", engine_usage.gfx_activity);
			mem_activity = string_format("%ld", engine_usage.umc_activity);
		}

		std::string gfx_util_str{(gfx_activity == "N/A") ? gfx_activity : string_format("%s %%", gfx_activity.c_str())};
		std::string mem_util_str{(mem_activity == "N/A") ? mem_activity : string_format("%s %%", mem_activity.c_str())};
		formatted_string = string_format("%s,%s", gfx_util_str.c_str(), mem_util_str.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}
