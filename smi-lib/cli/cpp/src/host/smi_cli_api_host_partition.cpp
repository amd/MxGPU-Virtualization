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
#include "smi_cli_templates.h"
#include "smi_cli_platform.h"
#include "smi_cli_helpers.h"

#include "tabulate/tabulate.hpp"

typedef amdsmi_status_t (*AMDSMI_GET_PARTITION_PROFILE_CONFIG)(amdsmi_processor_handle,
		amdsmi_accelerator_partition_profile_config_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_MEMORY_PARTITION_CAPS)(amdsmi_processor_handle,
		amdsmi_nps_caps_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CURR_MEMORY_PARTITION)(amdsmi_processor_handle,
		amdsmi_memory_partition_type_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CURR_ACCELERATOR_PARTITION)(amdsmi_processor_handle,
		amdsmi_accelerator_partition_profile_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_MEMORY_PARTITION_CONFIG)(amdsmi_processor_handle,
		amdsmi_memory_partition_config_t *);


extern AMDSMI_GET_PARTITION_PROFILE_CONFIG host_amdsmi_get_partition_profile_config;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_MEMORY_PARTITION_CAPS host_amdsmi_get_memory_partition_caps;
extern AMDSMI_GET_CURR_MEMORY_PARTITION host_amdsmi_get_curr_memory_partition;
extern AMDSMI_GET_CURR_ACCELERATOR_PARTITION host_amdsmi_get_partition_profile;
extern AMDSMI_GET_MEMORY_PARTITION_CONFIG host_amdsmi_get_gpu_memory_partition_config;

std::string host_fill_memory_partition(Arguments arg, std::string value)
{
	std::string out = string_format(memoryPartitioningTemplate, value.c_str(), value.c_str());
	return out;
}

int AmdSmiApiHost::amdsmi_get_accelerator_partition_command(uint64_t processor_bdf, Arguments arg,
		std::vector<tabulate::Table::Row_t> &rows, std::vector<tabulate::Table::Row_t> &resource_rows,
		std::string &gpu_id)
{
	int ret = 0;
	amdsmi_accelerator_partition_profile_config_t profile_configs;
	amdsmi_accelerator_partition_profile_t curr_profile;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_partition_profile_config(processor, &profile_configs);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	uint32_t partition_ids[AMDSMI_MAX_ACCELERATOR_PARTITIONS];
	ret = host_amdsmi_get_partition_profile(processor, &curr_profile, partition_ids);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	bool first_row = true;
	for (uint8_t i = 0; i < profile_configs.num_profiles; i++) {
		std::string profile_index = string_format("%d", profile_configs.profiles[i].profile_index);
		std::string partition_type_str;
		get_string_from_enum_accelerator_partition_type(profile_configs.profiles[i].profile_type,
				partition_type_str);

		std::string curr_partition_type_str;
		get_string_from_enum_accelerator_partition_type(curr_profile.profile_type, curr_partition_type_str);

		std::vector<std::string> possible_memory_caps = decode_memory_caps(
				profile_configs.profiles[i].memory_caps.nps_cap_mask);
		std::string possible_memory_human_readable = possible_memory_caps_to_human_readable(
				possible_memory_caps);

		std::string num_partition = string_format("%d", profile_configs.profiles[i].num_partitions);
		std::string num_of_resources = string_format("%d", profile_configs.profiles[i].num_resources);

		bool first_resource_in_partition = true;

		std::string partition_ids_str{};
		for (int i = 0; i < curr_profile.num_partitions; i++) {
			partition_ids_str += string_format("%d", partition_ids[i]);
			if (i != curr_profile.num_partitions - 1) {
				partition_ids_str += ",";
			}
		}

		if (partition_type_str == curr_partition_type_str) {
			partition_type_str += "*";
		} else {
			partition_ids_str = "N/A";
		}

		for (uint8_t k = 0; k < profile_configs.profiles[i].num_resources; k++) {
			std::string resource_index = string_format("%d", profile_configs.profiles[i].resources[0][k]);
			std::string resource_instances;
			std::string resources_shared;
			std::string resource_type_str;

			for (uint8_t m = 0; m < profile_configs.num_resource_profiles; m++) {
				if (profile_configs.profiles[i].resources[0][k] ==
						profile_configs.resource_profiles[m].profile_index) {
					get_string_from_enum_resource_type(profile_configs.resource_profiles[m].resource_type,
													   resource_type_str);
					resource_instances = string_format("%d", profile_configs.resource_profiles[m].partition_resource);
					resources_shared = string_format("%d",
													 profile_configs.resource_profiles[m].num_partitions_share_resource);
					break;
				}
			}

			if (first_resource_in_partition) {
				if (first_row) {
					rows.push_back({gpu_id, profile_index, possible_memory_human_readable, partition_type_str, partition_ids_str, num_partition, num_of_resources, resource_index, resource_type_str, resource_instances, resources_shared});
					first_row = false;
				} else {
					rows.push_back({"", profile_index, possible_memory_human_readable, partition_type_str, partition_ids_str, num_partition, num_of_resources, resource_index, resource_type_str, resource_instances, resources_shared});
				}
				first_resource_in_partition = false;
			} else {
				rows.push_back({ "","", "", "", "", "", "", resource_index, resource_type_str, resource_instances, resources_shared});
			}
		}
	}
	// //Add table_resources only once
	static bool table_resources_added = false;
	if (!table_resources_added) {
		table_resources_added = true;
		for (uint8_t i = 0; i < profile_configs.num_resource_profiles; i++) {
			std::string resource_type_str;
			get_string_from_enum_resource_type(profile_configs.resource_profiles[i].resource_type,
											   resource_type_str);
			std::string resource_instances = string_format("%d",
											 profile_configs.resource_profiles[i].partition_resource);
			std::string resources_shared = string_format("%d",
										   profile_configs.resource_profiles[i].num_partitions_share_resource);
			std::string resources_index = string_format("%d",
										  profile_configs.resource_profiles[i].profile_index);
			resource_rows.push_back({resources_index, resource_type_str, resource_instances, resources_shared});
		}
	}
	return ret;
}

int AmdSmiApiHost::amdsmi_get_memory_partition_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	std::string curr_mp_setting_str;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;


	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	amdsmi_memory_partition_config_t memory_partition_config;
	ret = host_amdsmi_get_gpu_memory_partition_config(processor, &memory_partition_config);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	get_string_from_enum_mp_setting(memory_partition_config.mp_mode, curr_mp_setting_str);
	std::vector<std::string> possible_memory_caps = decode_memory_caps(
			memory_partition_config.partition_caps.nps_cap_mask);


	std::string possible_memory_human_readable = possible_memory_caps_to_human_readable(
			possible_memory_caps);
	formatted_string = string_format("%s|%s", possible_memory_human_readable.c_str(),
									 curr_mp_setting_str.c_str());
	return ret;
}

int AmdSmiApiHost::amdsmi_get_current_partition_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;


	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	//memory
	amdsmi_memory_partition_config_t memory_partition_config;
	ret = host_amdsmi_get_gpu_memory_partition_config(processor, &memory_partition_config);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	std::string curr_mp_setting_str;
	get_string_from_enum_mp_setting(memory_partition_config.mp_mode, curr_mp_setting_str);

	//accelerator type
	amdsmi_accelerator_partition_profile_t curr_profile;
	uint32_t partition_ids[AMDSMI_MAX_ACCELERATOR_PARTITIONS];
	ret = host_amdsmi_get_partition_profile(processor, &curr_profile, partition_ids);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	std::string curr_partition_type_str;
	get_string_from_enum_accelerator_partition_type(curr_profile.profile_type, curr_partition_type_str);

	//accelerator profile index
	std::string profile_index_str{string_format("%d",curr_profile.profile_index)};

	//partition ids
	std::string partition_ids_str{};
	for (int i = 0; i < curr_profile.num_partitions; i++) {
		partition_ids_str += string_format("%d", partition_ids[i]);
		if (i != curr_profile.num_partitions - 1) {
			partition_ids_str += ",";
		}
	}

	formatted_string = string_format("%s|%s|%s|%s", curr_mp_setting_str.c_str(),
									 curr_partition_type_str.c_str(), profile_index_str.c_str(), partition_ids_str.c_str());

	return ret;
}