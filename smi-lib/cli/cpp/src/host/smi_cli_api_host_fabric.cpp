/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t*,
							amdsmi_processor_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
								amdsmi_processor_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_FABRIC_INFO)(amdsmi_processor_handle,
						      amdsmi_fabric_info_t*);
typedef amdsmi_status_t (*AMDSMI_ALLOC_FABRIC_TELEMETRY)(amdsmi_processor_handle, uint32_t,
							 amdsmi_fabric_telemetry_t**);
typedef amdsmi_status_t (*AMDSMI_GET_FABRIC_TELEMETRY_DATA)(amdsmi_processor_handle,
							    amdsmi_fabric_telemetry_t*);
typedef amdsmi_status_t (*AMDSMI_FREE_FABRIC_TELEMETRY)(amdsmi_processor_handle,
							amdsmi_fabric_telemetry_t*);

extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_FABRIC_INFO host_amdsmi_get_gpu_fabric_info;
extern AMDSMI_ALLOC_FABRIC_TELEMETRY host_amdsmi_alloc_fabric_telemetry;
extern AMDSMI_GET_FABRIC_TELEMETRY_DATA host_amdsmi_get_fabric_telemetry_data;
extern AMDSMI_FREE_FABRIC_TELEMETRY host_amdsmi_free_fabric_telemetry;

static const char* fabric_type_to_string(amdsmi_fabric_type_t type)
{
	switch (type) {
	case AMDSMI_FABRIC_TYPE_UALOE:
		return "UALOE";
	case AMDSMI_FABRIC_TYPE_UALINK:
		return "UALINK";
	default:
		return "UNKNOWN";
	}
}

static const char* fabric_address_mode_to_string(amdsmi_fabric_npa_address_mode_t mode)
{
	switch (mode) {
	case AMDSMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_ALIASING:
		return "ALIASING";
	case AMDSMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_IDENTIFICATION:
		return "SOURCE_IDENTIFICATION";
	default:
		return "UNKNOWN";
	}
}

static const char* fabric_accel_state_to_string(amdsmi_fabric_accelerator_vpod_state_t state)
{
	switch (state) {
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_UNCONFIGURED:
		return "UNCONFIGURED";
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_CONFIGURED:
		return "CONFIGURED";
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_READY:
		return "READY";
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_ACTIVE:
		return "ACTIVE";
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_ERROR:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

static std::string format_lane_en_bitmap(const uint8_t* bitmap, uint8_t num_stations,
					 bool uppercase_prefix)
{
	uint32_t end = num_stations > 0 ? num_stations : AMDSMI_FABRIC_MAX_BITMAP_SIZE;

	if (end > AMDSMI_FABRIC_MAX_BITMAP_SIZE)
		end = AMDSMI_FABRIC_MAX_BITMAP_SIZE;

	while (end > 0 && bitmap[end - 1] == 0)
		end--;

	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	if (uppercase_prefix)
		ss << std::uppercase;
	ss << (uppercase_prefix ? "0X" : "0x");

	if (end == 0) {
		ss << "0";
		return ss.str();
	}

	for (uint32_t i = 0; i < end; i++)
		ss << std::setw(2) << static_cast<unsigned int>(bitmap[i]);

	return ss.str();
}

int AmdSmiApiHost::amdsmi_get_fabric_topology_command(uint64_t processor_bdf, Arguments arg,
						      std::string& formatted_string)
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
	std::string bdf_string =
	    string_format("%04x:%02x:%02x.%01x", fabric_info.bdf.bdf.domain_number,
			  fabric_info.bdf.bdf.bus_number, fabric_info.bdf.bdf.device_number,
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

	uint16_t major		      = AMDSMI_FABRIC_VERSION_MAJOR(fabric_info.fabric_version);
	uint16_t minor		      = AMDSMI_FABRIC_VERSION_MINOR(fabric_info.fabric_version);
	std::string version_json_str  = string_format("%u.%u", major, minor);
	std::string version_human_str = minor == 0 ? string_format("%u", major) : version_json_str;

	// Schema-stable output: all fields are declared up front with safe
	// defaults (zero for numeric, empty string for text) so both JSON and
	// text always emit the same keys/rows regardless of fabric version.
	uint32_t accelerator_id			   = 0;
	amdsmi_fabric_type_t fabric_type	   = AMDSMI_FABRIC_TYPE_UNKNOWN;
	uint32_t bandwidth			   = 0;
	uint32_t latency			   = 0;
	uint32_t ppod_size			   = 0;
	uint32_t vpod_id			   = 0;
	uint32_t vpod_size			   = 0;
	amdsmi_fabric_npa_address_mode_t addr_mode = AMDSMI_FABRIC_NPA_ADDRESS_MODE_UNKNOWN;
	amdsmi_fabric_accelerator_vpod_state_t accel_state =
	    AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_UNKNOWN;
	uint32_t station_flags = 0;
	uint8_t num_stations   = 0;
	std::string ppod_id_str {};
	nlohmann::ordered_json vpod_active_accels_json = nlohmann::ordered_json::array();
	nlohmann::ordered_json local_accels_json       = nlohmann::ordered_json::array();

	std::string accel_id_str {"0"};
	std::string fabric_type_str {"UNKNOWN"};
	std::string bandwidth_str {"0"};
	std::string latency_str {"0"};
	std::string ppod_size_str {"0"};
	std::string vpod_id_str {"0"};
	std::string vpod_size_str {"0"};
	std::string addr_mode_str {"UNKNOWN"};
	std::string accel_state_str {"UNKNOWN"};
	std::string vpod_active_str {};
	std::string local_accels_str {"N/A"};
	std::string station_flags_str {"0"};
	std::string lane_en_bitmap_human {"0X0"};
	std::string lane_en_bitmap_json {"0x0"};

	/* UAL firmware currently reports 0.1; v1 is the only layout in the union.
	 * Treat major 0 and 1 as the v1 schema so unconfigured 0.1 nodes are not
	 * rendered as empty defaults. */
	if (major == 0 || major == 1) {
		auto& v1      = fabric_info.fabric_info.v1;
		auto& ppod    = v1.ppod;
		auto& vpod    = v1.vpod;
		auto& station = v1.station;

		accelerator_id = ppod.accelerator_id;
		fabric_type    = v1.fabric_type;
		bandwidth      = ppod.bandwidth;
		latency	       = ppod.latency;
		ppod_size      = ppod.ppod_size;
		vpod_id	       = vpod.vpod_id;
		vpod_size      = vpod.vpod_size;
		addr_mode      = vpod.addr_mode;
		accel_state    = v1.accel_state;
		station_flags  = station.station_flags;
		num_stations   = station.num_stations;
		ppod_id_str    = format_ppod_id(ppod.ppod_id);

		fabric_type_str = fabric_type_to_string(fabric_type);
		addr_mode_str	= fabric_address_mode_to_string(addr_mode);
		accel_state_str = fabric_accel_state_to_string(accel_state);

		for (uint32_t word_idx = 0;
		     word_idx < AMDSMI_FABRIC_ACTIVE_ACCELERATORS_BITMAP_SIZE; word_idx++) {
			uint32_t word = vpod.vpod_active_accelerators[word_idx];
			for (uint32_t bit = 0; bit < 32; bit++) {
				if (word & (1U << bit)) {
					uint32_t accel_id = (word_idx * 32) + bit;
					vpod_active_accels_json.push_back(accel_id);
					if (!vpod_active_str.empty())
						vpod_active_str += ",";
					vpod_active_str += string_format("%u", accel_id);
				}
			}
		}

		for (uint32_t i = 0; i < AMDSMI_FABRIC_MAX_LOCAL_GPUS; i++) {
			if (ppod.local_accelerators[i] == 0xFFFFFFFFu)
				continue;
			local_accels_json.push_back(ppod.local_accelerators[i]);
			if (!local_accels_str.empty() && local_accels_str != "N/A")
				local_accels_str += ", ";
			else if (local_accels_str == "N/A")
				local_accels_str.clear();
			local_accels_str += string_format("%u", ppod.local_accelerators[i]);
		}
		if (local_accels_json.empty())
			local_accels_str = "N/A";

		accel_id_str	  = string_format("%u", accelerator_id);
		bandwidth_str	  = string_format("%u", bandwidth);
		latency_str	  = string_format("%u", latency);
		ppod_size_str	  = string_format("%u", ppod_size);
		vpod_id_str	  = string_format("%u", vpod_id);
		vpod_size_str	  = string_format("%u", vpod_size);
		station_flags_str = string_format("%u", station_flags);
		lane_en_bitmap_human =
		    format_lane_en_bitmap(station.lane_en_bitmap, num_stations, true);
		lane_en_bitmap_json =
		    format_lane_en_bitmap(station.lane_en_bitmap, num_stations, false);
	}

	if (arg.output == json) {
		nlohmann::ordered_json fabric_json;
		nlohmann::ordered_json ppod_json;
		nlohmann::ordered_json vpod_json;
		nlohmann::ordered_json station_json;

		fabric_json["bdf"]	   = bdf_string;
		fabric_json["version"]	   = version_json_str;
		fabric_json["fabric_type"] = fabric_type_str;

		ppod_json["accelerator_id"]    = accelerator_id;
		ppod_json["physical_pod_id"]   = ppod_id_str;
		ppod_json["physical_pod_size"] = ppod_size;

		nlohmann::ordered_json bandwidth_json;
		bandwidth_json["value"] = bandwidth;
		bandwidth_json["unit"]	= "Mb/s";
		ppod_json["bandwidth"]	= bandwidth_json;

		nlohmann::ordered_json latency_json;
		latency_json["value"]		= latency;
		latency_json["unit"]		= "ns";
		ppod_json["latency"]		= latency_json;
		ppod_json["local_accelerators"] = local_accels_json;

		vpod_json["virtual_pod_id"]		     = vpod_id;
		vpod_json["virtual_pod_size"]		     = vpod_size;
		vpod_json["virtual_pod_active_accelerators"] = vpod_active_accels_json;
		vpod_json["accelerator_state"]		     = accel_state_str;
		vpod_json["address_mode"]		     = addr_mode_str;

		station_json["flags"]	       = station_flags;
		station_json["lane_en_bitmap"] = lane_en_bitmap_json;

		fabric_json["ppod"]    = ppod_json;
		fabric_json["vpod"]    = vpod_json;
		fabric_json["station"] = station_json;

		formatted_string = fabric_json.dump(4);
	} else {
		formatted_string =
		    string_format(staticFabricTopologyHeaderTemplate, bdf_string.c_str(),
				  version_human_str.c_str(), fabric_type_str.c_str());
		formatted_string += string_format(staticFabricPpodTemplate, accel_id_str.c_str(),
						  ppod_id_str.c_str(), ppod_size_str.c_str(),
						  local_accels_str.c_str(), bandwidth_str.c_str(),
						  "Mb/s", latency_str.c_str(), "ns");
		formatted_string += string_format(staticFabricVpodTemplate, vpod_id_str.c_str(),
						  vpod_size_str.c_str(), vpod_active_str.c_str(),
						  accel_state_str.c_str(), addr_mode_str.c_str());
		formatted_string +=
		    string_format(staticFabricStationTemplate, station_flags_str.c_str(),
				  lane_en_bitmap_human.c_str());
	}

	return ret;
}

// Helper function to get category name as string
const char* get_telemetry_category_name(amdsmi_fabric_telemetry_category_t category)
{
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
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_IFOE_DEBUG:
		return "IFOE_DEBUG";
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_PHY:
		return "PHY";
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX:
	default:
		return "UNKNOWN";
	}
}

// Helper function to format fabric telemetry output
std::string host_fill_fabric_telemetry(Arguments arg, amdsmi_fabric_telemetry_t* telemetry)
{
	std::string out {};

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
				amdsmi_fabric_telemetry_dataset_t* dataset =
				    telemetry->datasets[cat];

				// Verify dataset pointer validity
				try {
					category_data["category"] =
					    get_telemetry_category_name(dataset->category);
					category_data["generation_count"] =
					    dataset->generation_count;
					category_data["timestamp"] =
					    (double)dataset->timestamp.tv_sec +
					    (double)dataset->timestamp.tv_nsec / 1e9;
					category_data["instance_count"] = dataset->instance_count;

					nlohmann::ordered_json instances =
					    nlohmann::ordered_json::array();

					// Safe bounds checking for instances
					if (dataset->instance_count > 0 &&
					    dataset->instances != nullptr) {
						for (unsigned int i = 0;
						     i < dataset->instance_count; i++) {
							nlohmann::ordered_json instance;
							amdsmi_fabric_telemetry_instance_t* inst =
							    &dataset->instances[i];

							try {
								instance["name"] =
								    "instance_" + std::to_string(i);
								instance["logical_index"] =
								    inst->logical_idx;
								instance["item_count"] =
								    inst->item_count;

								nlohmann::ordered_json items =
								    nlohmann::ordered_json::array();

								// Safe bounds checking for items
								if (inst->item_count > 0 &&
								    inst->items != nullptr) {
									for (unsigned int j = 0;
									     j < inst->item_count;
									     j++) {
										nlohmann::
										    ordered_json
											item;
										item["id"] =
										    inst->items[j]
											.id;
										item["value"] =
										    inst->items[j]
											.value;
										items.push_back(
										    item);
									}
								}
								instance["items"] = items;
							} catch (...) {
								instance["error"] =
								    "Instance data access failed";
							}
							instances.push_back(instance);
						}
					}
					category_data["instances"] = instances;
					telemetry_data.push_back(category_data);
				} catch (...) {
					nlohmann::ordered_json error_data;
					error_data["error"] =
					    "Dataset access failed for category " +
					    std::to_string(cat);
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
				amdsmi_fabric_telemetry_dataset_t* dataset =
				    telemetry->datasets[cat];

				try {
					// Format timestamp with nanosecond precision
					std::stringstream timestamp_ss;
					timestamp_ss
					    << std::fixed << std::setprecision(9)
					    << (double)dataset->timestamp.tv_sec +
						   (double)dataset->timestamp.tv_nsec / 1e9;

					out += string_format(
					    fabricTelemetryCategoryTemplate,
					    get_telemetry_category_name(dataset->category),
					    std::to_string(dataset->generation_count).c_str(),
					    timestamp_ss.str().c_str(),
					    std::to_string(dataset->instance_count).c_str());

					// Safe bounds checking for instances
					if (dataset->instance_count > 0 &&
					    dataset->instances != nullptr) {
						for (unsigned int i = 0;
						     i < dataset->instance_count; i++) {
							amdsmi_fabric_telemetry_instance_t* inst =
							    &dataset->instances[i];

							try {
								out += string_format(
								    fabricTelemetryInstanceTemplate,
								    i,
								    std::to_string(
									inst->logical_idx)
									.c_str(),
								    std::to_string(inst->item_count)
									.c_str());

								// Safe bounds checking for items
								if (inst->item_count > 0 &&
								    inst->items != nullptr) {
									for (unsigned int j = 0;
									     j < inst->item_count;
									     j++) {
										out += string_format(
										    fabricTelemetryItemTemplate,
										    j,
										    std::to_string(
											inst->items
											    [j]
												.id)
											.c_str(),
										    std::to_string(
											inst
											    ->items
												[j]
											    .value)
											.c_str());
									}
								}
							} catch (...) {
								out += "        ERROR: Instance "
								       "data access failed\n";
							}
						}
					}
				} catch (...) {
					out += "    ERROR: Dataset access failed for category " +
					       std::to_string(cat) + "\n";
				}
			}
		}
	}

	return out;
}

int AmdSmiApiHost::amdsmi_get_fabric_telemetry_command(uint64_t processor_bdf, Arguments arg,
						       std::string& out)
{
	amdsmi_fabric_telemetry_t* telemetry = nullptr;
	int ret;
	amdsmi_processor_handle processor = nullptr;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	if (!host_amdsmi_get_processor_handles || !host_amdsmi_alloc_fabric_telemetry ||
	    !host_amdsmi_get_fabric_telemetry_data || !host_amdsmi_free_fabric_telemetry) {
		return AMDSMI_STATUS_NOT_SUPPORTED;
	}

	amdsmi_socket_handle socket = NULL;
	uint32_t gpu_count	    = 0;

	// First, get the count of processors
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS || gpu_count == 0) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	// Allocate array for processor handles
	amdsmi_processor_handle* processors =
	    (amdsmi_processor_handle*)malloc(sizeof(amdsmi_processor_handle) * gpu_count);
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

	Logger::getInstance().log(LogLevel::Info, AMDSMI_STATUS_SUCCESS, __FUNCTION__, __FILE__,
				  __LINE__);

	// Allocate telemetry structure. Request every AMDSMI category; the library
	// narrows this to the categories the driver implements.
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
