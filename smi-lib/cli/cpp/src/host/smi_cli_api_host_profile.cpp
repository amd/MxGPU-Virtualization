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
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"

#include "json/json.h"

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PARTITION_PROFILE_INFO)(amdsmi_processor_handle,
		amdsmi_profile_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_PARTITION_PROFILE_INFO host_amdsmi_get_partition_profile_info;


int AmdSmiApiHost::amdsmi_get_profile_command(uint64_t processor_bdf, Arguments arg, int gpu_index,
		std::string &formatted_string)
{
	int ret;
	unsigned int i;
	amdsmi_profile_info_t profile_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_partition_profile_info(processor, &profile_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	uint32_t curr_profile_vf_count = profile_info.profiles[profile_info.current_profile_index].vf_count;

	if (arg.output == OutputFormat::json) {
		nlohmann::ordered_json profiles_json = nlohmann::ordered_json::array();
		nlohmann::ordered_json profile_info_json = {};
		nlohmann::ordered_json profile_caps_json = {};

		for (i = 0; i < profile_info.profile_count; i++) {
			auto profile = profile_info.profiles[i];
			profile_info_json = {};
			profile_info_json["vf_count"] = profile.vf_count;
			nlohmann::ordered_json compute_available{};
			compute_available["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].available;
			compute_available["unit"] = string_format(
											"%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].available) == "N/A" ? "N/A" :
										"GFLOPS";
			nlohmann::ordered_json compute_max{};
			compute_max["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].max_value;
			compute_max["unit"] = string_format(
									  "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].max_value) == "N/A" ? "N/A" :
								  "GFLOPS";
			nlohmann::ordered_json compute_min{};
			compute_min["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].min_value;
			compute_min["unit"] = string_format(
									  "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].min_value) == "N/A" ? "N/A" :
								  "GFLOPS";
			nlohmann::ordered_json compute_optimal{};
			compute_optimal["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].optimal;
			compute_optimal["unit"] = string_format(
										  "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].optimal) == "N/A" ? "N/A" :
									  "GFLOPS";
			nlohmann::ordered_json compute_total{};
			compute_total["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].total;
			compute_total["unit"] = string_format(
										"%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].total) == "N/A" ? "N/A" : "GFLOPS";

			nlohmann::ordered_json memory_available{};
			memory_available["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].available;
			memory_available["unit"] = string_format(
										   "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].available) == "N/A" ? "N/A" : "MB";
			nlohmann::ordered_json memory_max{};
			memory_max["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].max_value;
			memory_max["unit"] = string_format(
									 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].max_value) == "N/A" ? "N/A" : "MB";
			nlohmann::ordered_json memory_min{};
			memory_min["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].min_value;
			memory_min["unit"] = string_format(
									 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].min_value) == "N/A" ? "N/A" : "MB";
			nlohmann::ordered_json memory_optimal{};
			memory_optimal["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].optimal;
			memory_optimal["unit"] = string_format(
										 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].optimal) == "N/A" ? "N/A" : "MB";
			nlohmann::ordered_json memory_total{};
			memory_total["value"] = profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].total;
			memory_total["unit"] = string_format(
									   "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].total) == "N/A" ? "N/A" : "MB";

			profile_caps_json = {
				{
					"compute",
					{
						{ "available", compute_available },
						{ "max", compute_max },
						{ "min", compute_min },
						{ "optimal", compute_optimal },
						{ "total", compute_total }
					}
				},
				{
					"decode",
					{
						{ "available", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].available },
						{ "max", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].max_value },
						{ "min", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].min_value },
						{ "optimal", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].optimal },
						{ "total", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].total }
					}
				},
				{
					"encode",
					{
						{ "available", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].available },
						{ "max", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].max_value },
						{ "min", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].min_value },
						{ "optimal", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].optimal },
						{ "total", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].total }
					}
				},
				{
					"memory",
					{
						{ "available",  memory_available },
						{ "max",  memory_max },
						{ "min",  memory_min },
						{ "optimal", memory_optimal },
						{ "total",  memory_total }
					}
				}
			};
			profile_info_json["profile_caps"] = profile_caps_json;

			if (profile.vf_count == curr_profile_vf_count)
				profile_info_json["current_profile"] = "YES";
			else {
				profile_info_json["current_profile"] = "NO";
			}

			profiles_json.insert(profiles_json.end(), profile_info_json);
		}
		formatted_string = profiles_json.dump(4);
	} else if (arg.output == OutputFormat::csv) {
		for (i = 0; i < profile_info.profile_count; i++) {
			auto profile = profile_info.profiles[i];
			std::string current_profile;
			if (profile.vf_count == curr_profile_vf_count)
				current_profile = "YES";
			else {
				current_profile = "NO";
			}

			formatted_string +=
				string_format("%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%s\n",
							  gpu_index,
							  profile.vf_count,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].available,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].max_value,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].min_value,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].optimal,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].total,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].available,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].max_value,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].min_value,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].optimal,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].total,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].available,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].max_value,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].min_value,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].optimal,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].total,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].available,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].max_value,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].min_value,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].optimal,
							  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].total,
							  current_profile.c_str()
							 );
		}
	} else {
		formatted_string += string_format(gpuTemplate, gpu_index);
		for (i = 0; i < profile_info.profile_count; i++) {
			auto profile = profile_info.profiles[i];
			std::string current_profile;
			if (profile.vf_count == curr_profile_vf_count)
				current_profile = "YES";
			else {
				current_profile = "NO";
			}
			std::string available_unit = string_format(
											 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].available) == "N/A" ? "" : "GFLOPS";
			std::string max_value_unit = string_format(
											 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].max_value) == "N/A" ? "" : "GFLOPS";
			std::string min_value_unit = string_format(
											 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].min_value) == "N/A" ? "" : "GFLOPS";
			std::string optimal_unit = string_format(
										   "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].optimal) == "N/A" ? "" : "GFLOPS";
			std::string total_unit = string_format(
										 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].total) == "N/A" ? "" : "GFLOPS";

			std::string available_mem_unit = string_format(
												 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].available) == "N/A" ? "" : "MB";
			std::string max_value_mem_unit = string_format(
												 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].max_value) == "N/A" ? "" : "MB";
			std::string min_value_mem_unit = string_format(
												 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].min_value) == "N/A" ? "" : "MB";
			std::string optimal_mem_unit = string_format(
											   "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].optimal) == "N/A" ? "" : "MB";
			std::string total_mem_unit = string_format(
											 "%lld", profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].total) == "N/A" ? "" : "MB";

			formatted_string += string_format(profileTemplate, profile.vf_count,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].available,
											  available_unit.c_str(),
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].max_value,
											  max_value_unit.c_str(),
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].min_value,
											  min_value_unit.c_str(),
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].optimal,
											  optimal_unit.c_str(),
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_COMPUTE].total,
											  total_unit.c_str(),
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].available,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].max_value,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].min_value,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].optimal,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_DECODE].total,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].available,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].max_value,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].min_value,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].optimal,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_ENCODE].total,
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].available,
											  available_mem_unit.c_str(),
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].max_value,
											  max_value_mem_unit.c_str(),
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].min_value,
											  min_value_mem_unit.c_str(),
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].optimal,
											  optimal_mem_unit.c_str(),
											  profile.profile_caps[AMDSMI_PROFILE_CAPABILITY_MEMORY].total,
											  total_mem_unit.c_str(),
											  current_profile.c_str()
											 );
		}
	}

	return ret;
}
