/** Copyright (C) 2025 Advanced Micro Devices. All rights reserved.
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
#include "smi_cli_exception.h"

#include "json/json.h"

#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_FABRIC_INFO)(amdsmi_processor_handle,
		amdsmi_fabric_info_t *);
typedef amdsmi_status_t (*AMDSMI_ALLOC_FABRIC_TELEMETRY)(amdsmi_processor_handle,
		uint32_t, amdsmi_fabric_telemetry_t **);
typedef amdsmi_status_t (*AMDSMI_GET_FABRIC_TELEMETRY_DATA)(amdsmi_processor_handle,
		amdsmi_fabric_telemetry_t *);
typedef amdsmi_status_t (*AMDSMI_FREE_FABRIC_TELEMETRY)(amdsmi_processor_handle,
		amdsmi_fabric_telemetry_t *);

extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_FABRIC_INFO host_amdsmi_get_gpu_fabric_info;
extern AMDSMI_ALLOC_FABRIC_TELEMETRY host_amdsmi_alloc_fabric_telemetry;
extern AMDSMI_GET_FABRIC_TELEMETRY_DATA host_amdsmi_get_fabric_telemetry_data;
extern AMDSMI_FREE_FABRIC_TELEMETRY host_amdsmi_free_fabric_telemetry;

int AmdSmiApiHost::amdsmi_get_fabric_topology_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_fabric_info_t fabric_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_gpu_fabric_info(processor, &fabric_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	// Format BDF string
	std::string bdf_string = string_format(
		"%04x:%02x:%02x.%01x",
		fabric_info.bdf.bdf.domain_number,
		fabric_info.bdf.bdf.bus_number,
		fabric_info.bdf.bdf.device_number,
		fabric_info.bdf.bdf.function_number);

	// Helper lambda: format the 16-byte ppod_id as a lowercase hex UUID string.
	auto format_ppod_id = [](const uint8_t id[AMDSMI_FABRIC_PPOD_ID_SIZE]) {
		std::stringstream ss;
		ss << std::hex << std::setfill('0');
		for (uint32_t i = 0; i < AMDSMI_FABRIC_PPOD_ID_SIZE; i++) {
			ss << std::setw(2) << static_cast<unsigned int>(id[i]);
			// Standard UUID dashes: 8-4-4-4-12
			if (i == 3 || i == 5 || i == 7 || i == 9) {
				ss << '-';
			}
		}
		return ss.str();
	};

	uint16_t major = AMDSMI_FABRIC_VERSION_MAJOR(fabric_info.info.version);
	uint16_t minor = AMDSMI_FABRIC_VERSION_MINOR(fabric_info.info.version);
	std::string version_str = string_format("%u.%u", major, minor);

	// Schema-stable output: all fields are declared up front with safe
	// defaults (zero for numeric, empty string for text) so both JSON and
	// text always emit the same keys/rows regardless of fabric version.
	// Version-specific blocks populate the values they understand.
	// (Same pattern as amdsmi_get_policy_command for ras_policy.)
	uint32_t accelerator_id = 0;
	uint32_t fabric_type = 0;
	uint32_t bandwidth = 0;
	uint32_t latency = 0;
	uint32_t ppod_size = 0;
	uint32_t vpod_id = 0;
	uint32_t vpod_size = 0;
	uint32_t addr_mode = 0;
	uint32_t accel_state = 0;
	std::string ppod_id_str{};
	nlohmann::ordered_json vpod_active_accels_json = nlohmann::ordered_json::array();
	nlohmann::ordered_json local_accels_json = nlohmann::ordered_json::array();

	std::string accel_id_str{};
	std::string fabric_type_str{};
	std::string bandwidth_str{};
	std::string latency_str{};
	std::string ppod_size_str{};
	std::string vpod_id_str{};
	std::string vpod_size_str{};
	std::string addr_mode_str{};
	std::string accel_state_str{};
	std::string vpod_active_str{};
	std::string local_accels_str{};

	if (major == 1) {
		auto& v1 = fabric_info.info.fabric_info.v1;

		accelerator_id = v1.accelerator_id;
		fabric_type    = v1.fabric_type;
		bandwidth      = v1.bandwidth;
		latency        = v1.latency;
		ppod_size      = v1.ppod_size;
		vpod_id        = v1.vpod_id;
		vpod_size      = v1.vpod_size;
		addr_mode      = v1.addr_mode;
		accel_state    = v1.accel_state;
		ppod_id_str    = format_ppod_id(v1.ppod_id);

		// Decode vpod_active_accelerators bitmap into a sorted list of IDs.
		for (uint32_t word_idx = 0; word_idx < AMDSMI_FABRIC_ACTIVE_ACCELERATORS_BITMAP_SIZE; word_idx++) {
			uint32_t word = v1.vpod_active_accelerators[word_idx];
			for (uint32_t bit = 0; bit < 32; bit++) {
				if (word & (1U << bit)) {
					uint32_t accel_id = (word_idx * 32) + bit;
					vpod_active_accels_json.push_back(accel_id);
					if (!vpod_active_str.empty()) vpod_active_str += ", ";
					vpod_active_str += string_format("%u", accel_id);
				}
			}
		}

		// local_accelerators: driver fills unsupported slots with 0xFF bytes
		// (sentinel 0xFFFFFFFF).  Skip sentinel entries; if all are sentinel
		// report "N/A" consistent with other unsupported fields in the CLI.
		for (uint32_t i = 0; i < AMDSMI_FABRIC_MAX_LOCAL_GPUS; i++) {
			if (v1.local_accelerators[i] == 0xFFFFFFFFu)
				continue;
			local_accels_json.push_back(v1.local_accelerators[i]);
			if (!local_accels_str.empty()) local_accels_str += ", ";
			local_accels_str += string_format("%u", v1.local_accelerators[i]);
		}
		if (local_accels_json.empty()) {
			local_accels_json = "N/A";
			local_accels_str  = "N/A";
		}

		accel_id_str    = string_format("%u", accelerator_id);
		fabric_type_str = string_format("%u", fabric_type);
		bandwidth_str   = string_format("%u", bandwidth);
		latency_str     = string_format("%u", latency);
		ppod_size_str   = string_format("%u", ppod_size);
		vpod_id_str     = string_format("%u", vpod_id);
		vpod_size_str   = string_format("%u", vpod_size);
		addr_mode_str   = string_format("%u", addr_mode);
		accel_state_str = string_format("%u", accel_state);
	}

	if (arg.output == json) {
		nlohmann::ordered_json fabric_json;
		fabric_json["bdf"] = bdf_string;
		fabric_json["version"] = version_str;
		fabric_json["accelerator_id"] = accelerator_id;
		fabric_json["fabric_type"] = fabric_type;

		nlohmann::ordered_json bandwidth_json;
		bandwidth_json["value"] = bandwidth;
		bandwidth_json["unit"] = "Mb/s";
		fabric_json["bandwidth"] = bandwidth_json;

		nlohmann::ordered_json latency_json;
		latency_json["value"] = latency;
		latency_json["unit"] = "ns";
		fabric_json["latency"] = latency_json;

		fabric_json["physical_pod_id"] = ppod_id_str;
		fabric_json["physical_pod_size"] = ppod_size;
		fabric_json["virtual_pod_id"] = vpod_id;
		fabric_json["virtual_pod_size"] = vpod_size;
		fabric_json["virtual_pod_active_accelerators"] = vpod_active_accels_json;
		fabric_json["local_accelerators"] = local_accels_json;
		fabric_json["address_mode"] = addr_mode;
		fabric_json["accelerator_state"] = accel_state;

		formatted_string = fabric_json.dump(4);
	} else {
		formatted_string = string_format(
			staticFabricTemplate,
			bdf_string.c_str(),
			version_str.c_str(),
			accel_id_str.c_str(),
			fabric_type_str.c_str(),
			bandwidth_str.c_str(), "Mb/s",
			latency_str.c_str(), "ns",
			ppod_id_str.c_str(),
			ppod_size_str.c_str(),
			vpod_id_str.c_str(),
			vpod_size_str.c_str(),
			vpod_active_str.c_str(),
			local_accels_str.c_str(),
			addr_mode_str.c_str(),
			accel_state_str.c_str()
		);
	}

	return ret;
}

// Helper function to get category name as string
const char* get_telemetry_category_name(amdsmi_fabric_telemetry_category_t category) {
	switch (category) {
		case AMDSMI_FABRIC_TELEMETRY_CATEGORY_INVALID:
			return "INVALID";
		case AMDSMI_FABRIC_TELEMETRY_CATEGORY_UALOE:
			return "UALOE";
		case AMDSMI_FABRIC_TELEMETRY_CATEGORY_SWITCH:
			return "SWITCH";
		case AMDSMI_FABRIC_TELEMETRY_CATEGORY_CRYPTO:
			return "CRYPTO";
		case AMDSMI_FABRIC_TELEMETRY_CATEGORY_PFC:
			return "PFC";
		case AMDSMI_FABRIC_TELEMETRY_CATEGORY_NETPORT:
			return "NETPORT";
		case AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_UALOE:
			return "DERIVED_UALOE";
		case AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_NETPORT:
			return "DERIVED_NETPORT";
		case AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX:
		default:
			return "UNKNOWN";
	}
}

// Helper function to format fabric telemetry output
std::string host_fill_fabric_telemetry(Arguments arg, amdsmi_fabric_telemetry_t *telemetry) {
	std::string out{};

	// Safety check
	if (!telemetry) {
		return "Error: null telemetry pointer";
	}

	if (arg.output == json) {
		nlohmann::ordered_json root;
		nlohmann::ordered_json fabric_telemetry;
		nlohmann::ordered_json telemetry_data = nlohmann::ordered_json::array();

		// Process each telemetry dataset safely
		for (unsigned int cat = 0; cat < AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX; cat++) {
			if (telemetry->datasets[cat] != nullptr) {
				nlohmann::ordered_json category_data;
				amdsmi_fabric_telemetry_dataset_t* dataset = telemetry->datasets[cat];

				// Verify dataset pointer validity
				try {
					category_data["category"] = get_telemetry_category_name(dataset->category);
					category_data["generation_count"] = dataset->generation_count;
					category_data["timestamp"] = (double)dataset->timestamp.tv_sec +
												(double)dataset->timestamp.tv_nsec / 1e9;
					category_data["instance_count"] = dataset->instance_count;

					nlohmann::ordered_json instances = nlohmann::ordered_json::array();

					// Safe bounds checking for instances
					if (dataset->instance_count > 0 && dataset->instances != nullptr) {
						for (unsigned int i = 0; i < dataset->instance_count; i++) {
							nlohmann::ordered_json instance;
							amdsmi_fabric_telemetry_instance_t* inst = &dataset->instances[i];

							try {
								instance["name"] = "instance_" + std::to_string(i);
								instance["logical_index"] = inst->logical_idx;
								instance["item_count"] = inst->item_count;

								nlohmann::ordered_json items = nlohmann::ordered_json::array();

								// Safe bounds checking for items
								if (inst->item_count > 0 && inst->items != nullptr) {
									for (unsigned int j = 0; j < inst->item_count; j++) {
										nlohmann::ordered_json item;
										item["id"] = inst->items[j].id;
										item["value"] = inst->items[j].value;
										items.push_back(item);
									}
								}
								instance["items"] = items;
							} catch (...) {
								instance["error"] = "Instance data access failed";
							}
							instances.push_back(instance);
						}
					}
					category_data["instances"] = instances;
					telemetry_data.push_back(category_data);
				} catch (...) {
					nlohmann::ordered_json error_data;
					error_data["error"] = "Dataset access failed for category " + std::to_string(cat);
					telemetry_data.push_back(error_data);
				}
			}
		}

		out = telemetry_data.dump(4);
	} else {
		// Human-readable format using templates
		out = "    TELEMETRY:\n";

		// Process each telemetry dataset safely
		for (unsigned int cat = 0; cat < AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX; cat++) {
			if (telemetry->datasets[cat] != nullptr) {
				amdsmi_fabric_telemetry_dataset_t* dataset = telemetry->datasets[cat];

				try {
					// Format timestamp with nanosecond precision
					std::stringstream timestamp_ss;
					timestamp_ss << std::fixed << std::setprecision(9)
						<< (double)dataset->timestamp.tv_sec + (double)dataset->timestamp.tv_nsec / 1e9;

					out += string_format(fabricTelemetryCategoryTemplate,
										get_telemetry_category_name(dataset->category),
										std::to_string(dataset->generation_count).c_str(),
										timestamp_ss.str().c_str(),
										std::to_string(dataset->instance_count).c_str());

					// Safe bounds checking for instances
					if (dataset->instance_count > 0 && dataset->instances != nullptr) {
						for (unsigned int i = 0; i < dataset->instance_count; i++) {
							amdsmi_fabric_telemetry_instance_t* inst = &dataset->instances[i];

							try {
								out += string_format(fabricTelemetryInstanceTemplate, i,
													std::to_string(inst->logical_idx).c_str(),
													std::to_string(inst->item_count).c_str());

								// Safe bounds checking for items
								if (inst->item_count > 0 && inst->items != nullptr) {
									for (unsigned int j = 0; j < inst->item_count; j++) {
										out += string_format(fabricTelemetryItemTemplate, j,
															std::to_string(inst->items[j].id).c_str(),
															std::to_string(inst->items[j].value).c_str());
									}
								}
							} catch (...) {
								out += "        ERROR: Instance data access failed\n";
							}
						}
					}
				} catch (...) {
					out += "    ERROR: Dataset access failed for category " + std::to_string(cat) + "\n";
				}
			}
		}
	}

	return out;
}

int AmdSmiApiHost::amdsmi_get_fabric_telemetry_command(uint64_t processor_bdf, Arguments arg, std::string& out)
{
	amdsmi_fabric_telemetry_t *telemetry = nullptr;
	int ret;
	amdsmi_processor_handle processor = nullptr;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;


	if (!host_amdsmi_get_processor_handles || !host_amdsmi_alloc_fabric_telemetry ||
		!host_amdsmi_get_fabric_telemetry_data || !host_amdsmi_free_fabric_telemetry) {
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}

	amdsmi_socket_handle socket = NULL;
	uint32_t gpu_count = 0;

	// First, get the count of processors
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS || gpu_count == 0) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	// Allocate array for processor handles
	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(
		sizeof(amdsmi_processor_handle) * gpu_count);
	if (processors == NULL) {
		return AMDSMI_STATUS_OUT_OF_RESOURCES;
	}

	// Get all processor handles (like Python's amdsmi_get_processor_handles())
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, processors);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		free(processors);
		return ret;
	}

	// Use the first processor handle for mock driver
	processor = processors[0];
	free(processors);

	Logger::getInstance().log(LogLevel::Info, AMDSMI_STATUS_SUCCESS,
		__FUNCTION__, __FILE__, __LINE__);

	// Allocate telemetry structure
	// Create mask for all categories: (1 << MAX) - 1 sets bits 0 through MAX-1
	uint32_t category_mask = (1U << AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX) - 1;

	ret = host_amdsmi_alloc_fabric_telemetry(processor, category_mask, &telemetry);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	// Verify allocation succeeded
	if (!telemetry) {
		return AMDSMI_STATUS_INVAL;
	}


	// Get telemetry data
	ret = host_amdsmi_get_fabric_telemetry_data(processor, telemetry);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		host_amdsmi_free_fabric_telemetry(processor, telemetry);
		return ret;
	}

	// Format output based on format type
	out = host_fill_fabric_telemetry(arg, telemetry);

	// Free telemetry
	host_amdsmi_free_fabric_telemetry(processor, telemetry);

	return AMDSMI_STATUS_SUCCESS;
}
