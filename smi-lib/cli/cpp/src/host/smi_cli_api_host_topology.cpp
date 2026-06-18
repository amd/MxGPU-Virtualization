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

#define MAX_CPU_SET_SIZE 16
#include <sstream>
#include <limits.h>
#include <cstring>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_TOPOLOGY)(amdsmi_processor_handle,
		amdsmi_processor_handle,
		amdsmi_link_topology_t *);
typedef amdsmi_status_t (*AMDSMI_TOPO_GET_P2P_STATUS)(amdsmi_processor_handle,amdsmi_processor_handle,
		amdsmi_link_type_t*, amdsmi_p2p_capability_t*);
typedef amdsmi_status_t (*AMDSMI_TOPO_GET_LINK_TYPE)(amdsmi_processor_handle,
		amdsmi_processor_handle,
		uint64_t *, amdsmi_link_type_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_DEVICE_BDF)(amdsmi_processor_handle,
		amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_AI_NIC_NUMA_INFO)(amdsmi_processor_handle,
		amdsmi_nic_numa_info_t *);
typedef amdsmi_status_t (*AMDSMI_TOPO_GET_NUMA_NODE_NUMBER)(amdsmi_processor_handle,
		uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CPU_AFFINITY_WITH_SCOPE)(amdsmi_processor_handle,
		uint32_t, uint64_t *, amdsmi_affinity_scope_t);


extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_NIC_PROCESSOR_HANDLES host_amdsmi_get_nic_processor_handles;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_LINK_TOPOLOGY host_amdsmi_get_link_topology;
extern AMDSMI_TOPO_GET_P2P_STATUS host_amdsmi_topo_get_p2p_status;
extern AMDSMI_TOPO_GET_LINK_TYPE host_amdsmi_topo_get_link_type;
extern AMDSMI_GET_NIC_DEVICE_BDF host_amdsmi_get_nic_device_bdf;
extern AMDSMI_GET_AI_NIC_NUMA_INFO host_amdsmi_get_nic_numa_info;
extern AMDSMI_TOPO_GET_NUMA_NODE_NUMBER host_amdsmi_topo_get_numa_node_number;
extern AMDSMI_GET_CPU_AFFINITY_WITH_SCOPE host_amdsmi_get_cpu_affinity_with_scope;


std::vector<std::vector<amdsmi_link_topology_t>> topology;
std::vector<std::vector<amdsmi_link_type_t>> nic_topology;
std::vector<std::vector<amdsmi_p2p_capability_t>> p2p_capability;

amdsmi_link_topology_t get_empty_topology_info() {
	amdsmi_link_topology_t info{};
	info.weight = UINT64_MAX;
	info.num_hops = UINT8_MAX;
	info.fb_sharing = UINT8_MAX;
	info.link_type = AMDSMI_LINK_TYPE_UNKNOWN;
	return info;
}

amdsmi_link_type_t get_empty_nic_topology_info() {
	return AMDSMI_LINK_TYPE_UNKNOWN;
}

amdsmi_p2p_capability_t get_empty_p2p_capability_info() {
	amdsmi_p2p_capability_t info{};
	info.is_iolink_coherent = UINT8_MAX;
	info.is_iolink_atomics_32bit = UINT8_MAX;
	info.is_iolink_atomics_64bit = UINT8_MAX;
	info.is_iolink_dma = UINT8_MAX;
	info.is_iolink_bi_directional = UINT8_MAX;
	return info;
}

int AmdSmiApiHost::initTopology(Arguments arg,
					std::vector<std::string>& bdf_vector, std::vector<std::string>& nic_bdf_vector)
{
	amdsmi_link_topology_t topology_info;
	amdsmi_link_topology_t topology_empty_info = get_empty_topology_info();
	amdsmi_link_type_t nic_topology_info;
	amdsmi_link_type_t nic_topology_empty_info = get_empty_nic_topology_info();
	amdsmi_p2p_capability_t p2p_capability_info;
	amdsmi_p2p_capability_t p2p_capability_empty_info = get_empty_p2p_capability_info();
	amdsmi_link_type_t p2p_link_type;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_bdf_t nic_bdf;
	amdsmi_bdf_t bdf;
	amdsmi_processor_handle gpu_handle;
	amdsmi_processor_handle nic_handle;
	amdsmi_socket_handle socket = NULL;
	std::string bdf_string{};
	std::string nic_bdf_string{};
	unsigned int gpu_count;
	amdsmi_processor_handle *processors = nullptr;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	int ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		free(processors);
		processors = nullptr;
		return ret;
	}

	for (unsigned int j = 0; j < gpu_count; j++) {
		ret = host_amdsmi_get_gpu_device_bdf(processors[j], &bdf);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			free(processors);
			processors = nullptr;
			return ret;
		}
		bdf_string = string_format(
						 "%04x:%02x:%02x.%01x", bdf.bdf.domain_number, bdf.bdf.bus_number, bdf.bdf.device_number,
						 bdf.bdf.function_number);
		bdf_vector.push_back(bdf_string);
		bdf_string.clear();
	}

	if (arg.nic_devices.size() > 0)	{
		for (unsigned int i = 0; i < arg.nic_devices.size(); i++) {
			std::vector<amdsmi_link_type_t> nic_inner_vector;
			unsigned int nic_index = arg.nic_devices[i]->get_gpu_index();

			ret = amdsmi_get_nic_processor_from_index(&nic_handle, nic_index);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
				free(processors);
				processors = nullptr;
				return ret;
			}

			ret = host_amdsmi_get_nic_device_bdf(nic_handle, &nic_bdf);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
				free(processors);
				processors = nullptr;
				return ret;
			}
			nic_bdf_string = string_format(
								"%04x:%02x:%02x.%01x", nic_bdf.bdf.domain_number, nic_bdf.bdf.bus_number, nic_bdf.bdf.device_number,
								nic_bdf.bdf.function_number);
			nic_bdf_vector.push_back(nic_bdf_string);
			for (unsigned int j = 0; j < gpu_count; j++) {
				ret = host_amdsmi_topo_get_link_type(nic_handle, processors[j], nullptr, &nic_topology_info);
				if (ret != AMDSMI_STATUS_SUCCESS) {
					Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__,
												__LINE__);
					nic_inner_vector.push_back(nic_topology_empty_info);
				} else {
					nic_inner_vector.push_back(nic_topology_info);
				}
			}
			nic_topology.push_back(nic_inner_vector);
		}
	}

	for (unsigned int i = 0; i < arg.devices.size(); i++) {

		std::vector<amdsmi_link_topology_t> inner_vector;
		std::vector<amdsmi_p2p_capability_t> inner_vector_p2p_cap;

		tmp_bdf.as_uint = arg.devices[i]->get_bdf();

		ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &gpu_handle);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			free(processors);
			processors = nullptr;
			return ret;
		}

		for (unsigned int j = 0; j < gpu_count; j++) {
			ret = host_amdsmi_get_link_topology(gpu_handle,processors[j],
												&topology_info);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__,
										  __LINE__);
				inner_vector.push_back(topology_empty_info);
			} else {
				inner_vector.push_back(topology_info);
			}

			ret = host_amdsmi_topo_get_p2p_status(gpu_handle, processors[j], &p2p_link_type,
												&p2p_capability_info);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__,
										  __LINE__);
				inner_vector_p2p_cap.push_back(p2p_capability_empty_info);
			} else {
				inner_vector_p2p_cap.push_back(p2p_capability_info);
			}
		}
		topology.push_back(inner_vector);
		p2p_capability.push_back(inner_vector_p2p_cap);
	}

	free(processors);
	processors = nullptr;
	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_weight_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	out.append(topologyWeightTemplate);
	out.append(string_format("%-13s", " "));

	for (i = 0; i < gpu_count; i++) {
		out.append(string_format("%-13s",
								 bdf_vector[i].c_str()));
	}

	out.append("\n");
	for (i = 0; i < arg.devices.size(); i++) {
		int gpu_index = arg.devices[i]->get_gpu_index();
		out.append(string_format("%-13s",
								 bdf_vector[gpu_index].c_str()));

		for (j = 0; j < gpu_count; j++) {
			if (topology[i][j].weight == UINT64_MAX) {
				out.append(string_format("%-13s", "N/A"));
			} else {
				out.append(string_format("%-13ld", topology[i][j].weight));
			}
		}
		out.append("\n");
	}
	out.append("\n");

	return 0;
}

int AmdSmiApiHost::amdsmi_get_hops_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	out.append(topologyHopsTemplate);
	out.append(string_format("%-13s", " "));

	for (i = 0; i < gpu_count; i++) {
		out.append(string_format("%-13s",
								 bdf_vector[i].c_str()));
	}
	out.append("\n");
	for (i = 0; i < arg.devices.size(); i++) {
		int gpu_index = arg.devices[i]->get_gpu_index();
		out.append(string_format("%-13s",
								 bdf_vector[gpu_index].c_str()));

		for (j = 0; j < gpu_count; j++) {
			if (topology[i][j].num_hops == UINT8_MAX) {
				out.append(string_format("%-13s", "N/A"));
			} else {
				out.append(string_format("%-13ld", topology[i][j].num_hops));
			}
		}
		out.append("\n");
	}
	out.append("\n");

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_fb_sharing_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));


	out.append(topologyFbSharingTemplate);
	out.append(string_format("%-13s", " "));

	for (i = 0; i < gpu_count; i++) {
		out.append(string_format("%-13s",
								 bdf_vector[i].c_str()));
	}
	out.append("\n");
	for (i = 0; i < arg.devices.size(); i++) {
		int gpu_index = arg.devices[i]->get_gpu_index();
		out.append(string_format("%-13s",
								 bdf_vector[gpu_index].c_str()));

		for (j = 0; j < gpu_count; j++) {
			if (topology[i][j].fb_sharing == UINT8_MAX) {
				out.append(string_format("%-13s", "N/A"));
			} else {
				out.append(string_format("%-13s", topology[i][j].fb_sharing ? "ENABLED" : "DISABLED"));
			}
		}
		out.append("\n");
	}
	out.append("\n");

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_link_type_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	out.append(topologyLinkTypeTemplate);
	out.append(string_format("%-13s", " "));

	for (i = 0; i < gpu_count; i++) {
		out.append(string_format("%-13s",
								 bdf_vector[i].c_str()));
	}
	out.append("\n");
	for (i = 0; i < arg.devices.size(); i++) {
		int gpu_index = arg.devices[i]->get_gpu_index();
		out.append(string_format("%-13s",
								 bdf_vector[gpu_index].c_str()));

		for (j = 0; j < gpu_count; j++) {
			std::string  link_type_string;
			format_link_type(topology[i][j].link_type, link_type_string);
			if (gpu_index == j) {
				link_type_string = "SELF";
			}
			out.append(string_format("%-13s",
									 link_type_string.c_str()));
		}
		out.append("\n");
	}
	out.append("\n");

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_coherent_p2p_capability_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	out.append(topologyCoherentTemplate);
	out.append(string_format("%-13s", " "));

	for (i = 0; i < gpu_count; i++) {
		out.append(string_format("%-13s",
								 bdf_vector[i].c_str()));
	}

	out.append("\n");
	for (i = 0; i < arg.devices.size(); i++) {
		int gpu_index = arg.devices[i]->get_gpu_index();
		out.append(string_format("%-13s",
								 bdf_vector[gpu_index].c_str()));

		for (j = 0; j < gpu_count; j++) {
			std::string coherent_string;
			if(gpu_index == j) {
				coherent_string = "SELF";
			} else if (p2p_capability[i][j].is_iolink_coherent == UINT8_MAX) {
				coherent_string = "N/A";
			} else {
				coherent_string = p2p_capability[i][j].is_iolink_coherent == 1 ? "C" : "NC";
			}

			out.append(string_format("%-13s",
									 coherent_string.c_str()));
		}
		out.append("\n");
	}
	out.append("\n");

	return 0;
}

int AmdSmiApiHost::amdsmi_get_atomics_p2p_capability_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	out.append(topologyAtomicsTemplate);
	out.append(string_format("%-13s", " "));

	for (i = 0; i < gpu_count; i++) {
		out.append(string_format("%-13s",
								 bdf_vector[i].c_str()));
	}

	out.append("\n");
	for (i = 0; i < arg.devices.size(); i++) {
		int gpu_index = arg.devices[i]->get_gpu_index();
		out.append(string_format("%-13s",
								 bdf_vector[gpu_index].c_str()));

		for (j = 0; j < gpu_count; j++) {
			std::string atomics_string;
			if(gpu_index == j) {
				atomics_string = "SELF";
			} else if (p2p_capability[i][j].is_iolink_atomics_64bit == UINT8_MAX ||
					   p2p_capability[i][j].is_iolink_atomics_32bit == UINT8_MAX) {
				atomics_string = "N/A";
			} else {
				atomics_string = p2p_capability[i][j].is_iolink_atomics_64bit == 1 ? "64" : "";
				if(p2p_capability[i][j].is_iolink_atomics_64bit == 1) {
					atomics_string += p2p_capability[i][j].is_iolink_atomics_32bit == 1 ? ",32" : "";
				} else{
					atomics_string += p2p_capability[i][j].is_iolink_atomics_32bit == 1 ? "32" : "";
				}
			}

			out.append(string_format("%-13s",
									 atomics_string.c_str()));
		}
		out.append("\n");
	}
	out.append("\n");

	return 0;
}

int AmdSmiApiHost::amdsmi_get_dma_p2p_capability_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	out.append(topologyDmaTemplate);
	out.append(string_format("%-13s", " "));

	for (i = 0; i < gpu_count; i++) {
		out.append(string_format("%-13s",
								 bdf_vector[i].c_str()));
	}

	out.append("\n");
	for (i = 0; i < arg.devices.size(); i++) {
		int gpu_index = arg.devices[i]->get_gpu_index();
		out.append(string_format("%-13s",
								 bdf_vector[gpu_index].c_str()));

		for (j = 0; j < gpu_count; j++) {
			std::string dma_string;
			if(gpu_index == j) {
				dma_string = "SELF";
			} else if (p2p_capability[i][j].is_iolink_dma == UINT8_MAX) {
				dma_string = "N/A";
			} else {
				dma_string = p2p_capability[i][j].is_iolink_dma == 1 ? "TRUE" : "FALSE";
			}

			out.append(string_format("%-13s",
									 dma_string.c_str()));
		}
		out.append("\n");
	}
	out.append("\n");

	return 0;
}

int AmdSmiApiHost::amdsmi_get_bi_directional_p2p_capability_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	out.append(topologyBiDirectionalTemplate);
	out.append(string_format("%-13s", " "));

	for (i = 0; i < gpu_count; i++) {
		out.append(string_format("%-13s",
								 bdf_vector[i].c_str()));
	}

	out.append("\n");
	for (i = 0; i < arg.devices.size(); i++) {
		int gpu_index = arg.devices[i]->get_gpu_index();
		out.append(string_format("%-13s",
								 bdf_vector[gpu_index].c_str()));

		for (j = 0; j < gpu_count; j++) {
			std::string bi_directional_string;
			if(gpu_index == j) {
				bi_directional_string = "SELF";
			} else if (p2p_capability[i][j].is_iolink_bi_directional == UINT8_MAX) {
				bi_directional_string = "N/A";
			} else {
				bi_directional_string = p2p_capability[i][j].is_iolink_bi_directional == 1 ? "TRUE" : "FALSE";
			}

			out.append(string_format("%-13s",
									 bi_directional_string.c_str()));
		}
		out.append("\n");
	}
	out.append("\n");

	return 0;
}

int AmdSmiApiHost::amdsmi_get_all_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int gpu_count;
	nlohmann::ordered_json output = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	for (int i = 0; i < arg.devices.size(); i++) {
		json = {};
		int gpu_index = arg.devices[i]->get_gpu_index();
		bool option_found = false;
		nlohmann::ordered_json links_array = nlohmann::ordered_json::array();

		for (int j = 0; j < gpu_count; j++) {
			nlohmann::ordered_json link_topology = {};
			bool link_option_found = false;

			if (std::find(arg.options.begin(), arg.options.end(), "weight") != arg.options.end() ||
					arg.all_arguments) {
				if (topology[i][j].weight == UINT64_MAX) {
					link_topology["weight"] = "N/A";
				} else {
					link_topology["weight"] = topology[i][j].weight;
				}
				link_option_found = true;
			}

			if (std::find(arg.options.begin(), arg.options.end(), "link-type") != arg.options.end() ||
					arg.all_arguments) {
				std::string  link_type_string;
				format_link_type(topology[i][j].link_type, link_type_string);
				link_topology["link_type"] = link_type_string;
				link_option_found = true;
			}
			if (std::find(arg.options.begin(), arg.options.end(), "hops") != arg.options.end() ||
					arg.all_arguments) {
				if (topology[i][j].num_hops == UINT8_MAX) {
					link_topology["num_hops"] = "N/A";
				} else {
					link_topology["num_hops"] = topology[i][j].num_hops;
				}
				link_option_found = true;
			}
			if (std::find(arg.options.begin(), arg.options.end(), "fb-sharing") != arg.options.end() ||
					arg.all_arguments) {
				if (topology[i][j].fb_sharing == UINT8_MAX) {
					link_topology["fb_sharing"] = "N/A";
				} else {
					link_topology["fb_sharing"] = topology[i][j].fb_sharing ? "ENABLED" : "DISABLED";
				}
				link_option_found = true;
			}

			if (std::find(arg.options.begin(), arg.options.end(), "coherent") != arg.options.end() ||
					arg.all_arguments) {
				std::string coherent_status_string;
				if(gpu_index == j) {
					coherent_status_string = "SELF";
				} else if (p2p_capability[i][j].is_iolink_coherent == UINT8_MAX) {
					coherent_status_string = "N/A";
				} else {
					coherent_status_string = p2p_capability[i][j].is_iolink_coherent == 1 ? "C" : "NC";
				}
				link_topology["coherent"] =  coherent_status_string;
				link_option_found = true;
			}

			if (std::find(arg.options.begin(), arg.options.end(), "atomics") != arg.options.end() ||
					arg.all_arguments) {

				std::string atomics_string;
				if(gpu_index == j) {
					atomics_string = "SELF";
				} else if (p2p_capability[i][j].is_iolink_atomics_64bit == UINT8_MAX ||
						   p2p_capability[i][j].is_iolink_atomics_32bit == UINT8_MAX) {
					atomics_string = "N/A";
				} else {
					atomics_string = p2p_capability[i][j].is_iolink_atomics_64bit == 1 ? "64" : "";
					if(p2p_capability[i][j].is_iolink_atomics_64bit == 1) {
						atomics_string += p2p_capability[i][j].is_iolink_atomics_32bit == 1 ? ",32" : "";
					} else{
						atomics_string += p2p_capability[i][j].is_iolink_atomics_32bit == 1 ? "32" : "";
					}
				}
				link_topology["atomics"] =  atomics_string;
				link_option_found = true;
			}

			if (std::find(arg.options.begin(), arg.options.end(), "dma") != arg.options.end() ||
					arg.all_arguments) {
				std::string dma_status_string;
				if(gpu_index == j) {
					dma_status_string = "SELF";
				} else if (p2p_capability[i][j].is_iolink_dma == UINT8_MAX) {
					dma_status_string = "N/A";
				} else {
					dma_status_string = p2p_capability[i][j].is_iolink_dma == 1 ? "TRUE" : "FALSE";
				}
				link_topology["dma"] =  dma_status_string;
				link_option_found = true;
			}

			if (std::find(arg.options.begin(), arg.options.end(), "bi-dir") != arg.options.end() ||
					arg.all_arguments) {
				std::string bi_dir_status_string;
				if(gpu_index == j) {
					bi_dir_status_string = "SELF";
				} else if (p2p_capability[i][j].is_iolink_bi_directional == UINT8_MAX) {
					bi_dir_status_string = "N/A";
				} else {
					bi_dir_status_string = p2p_capability[i][j].is_iolink_bi_directional == 1 ? "TRUE" : "FALSE";
				}
				link_topology["bi-dir"] =  bi_dir_status_string;
				link_option_found = true;
			}

			if (link_option_found) {
				nlohmann::ordered_json ordered_link;
				ordered_link["gpu"] = j;
				ordered_link["bdf"] = bdf_vector[j].c_str();
				for (auto& [key, value] : link_topology.items()) {
					ordered_link[key] = value;
				}
				links_array.insert(links_array.end(), ordered_link);
				option_found = true;
			}
		}

		if (option_found) {
			json["gpu"] = gpu_index;
			json["bdf"] = bdf_vector[gpu_index].c_str();
			json["links"] = links_array;
		output.insert(output.end(), json);
		}
	}

	out = output.dump(4);

	return 0;
}

int AmdSmiApiHost::amdsmi_get_nic_link_type_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::vector<std::string> nic_bdf_vector, std::string& out)
{
	int ret = 0;
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;
	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	if (arg.output == json) {
		nlohmann::ordered_json output = nlohmann::ordered_json::array();

		for (i = 0; i < arg.nic_devices.size(); i++) {
			nlohmann::ordered_json nic_json;
			int nic_index = arg.nic_devices[i]->get_gpu_index();
			nic_json["nic"] = nic_index;
			nic_json["bdf"] = nic_bdf_vector[i].c_str();
			nic_json["links"] = nlohmann::ordered_json::array();

			for (j = 0; j < gpu_count; j++) {
				nlohmann::ordered_json link_json;
				std::string nic_link_type_string;
				get_string_from_enum_nic_topo_link_type(nic_topology[i][j], nic_link_type_string);

				link_json["gpu"] = j;
				link_json["bdf"] = bdf_vector[j].c_str();
				link_json["link_type"] = nic_link_type_string;
				nic_json["links"].push_back(link_json);
			}
			output.push_back(nic_json);
		}
		out = output.dump(4);
	} else if (arg.output == human) {
		out.append(topologyNicLinkTypeTemplate);
		out.append(string_format("%-13s", " "));

		for (i = 0; i < gpu_count; i++) {
			out.append(string_format("%-13s", bdf_vector[i].c_str()));
		}
		out.append("\n");

		// NIC rows with link types
		for (i = 0; i < arg.nic_devices.size(); i++) {
			out.append(string_format("%-13s", nic_bdf_vector[i].c_str()));

			for (j = 0; j < gpu_count; j++) {
				std::string nic_link_type_string;
				get_string_from_enum_nic_topo_link_type(nic_topology[i][j], nic_link_type_string);
				out.append(string_format("%-13s", nic_link_type_string.c_str()));
			}
			out.append("\n");
		}
		out.append("\n");
	}

	return ret;
}

int AmdSmiApiHost::amdsmi_get_nic_numa_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::vector<std::string> nic_bdf_vector, std::string& out)
{
	int ret = 0;
	unsigned int i;
	amdsmi_processor_handle nic_handle;
	amdsmi_nic_numa_info_t nic_numa_info;
	nlohmann::ordered_json json_output = nlohmann::ordered_json::array();

	if (arg.output == human) {
		out.append(topologyNumaTemplate);
		out.append(string_format("%-13s%-13s%-13s\n"," ", "NUMA", "CPU AFFINITY"));
	}

	for (i = 0; i < arg.nic_devices.size(); i++) {
		int nic_index = arg.nic_devices[i]->get_gpu_index();
		bool numa_info_valid = true;
		ret = amdsmi_get_nic_processor_from_index(&nic_handle, nic_index);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			return ret;
		}
		ret = host_amdsmi_get_nic_numa_info(nic_handle, &nic_numa_info);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			numa_info_valid = false;
		}

		if (arg.output == json) {
			nlohmann::ordered_json nic_json;
			nic_json["nic"] = nic_index;
			nic_json["bdf"] = nic_bdf_vector[i].c_str();
			if (numa_info_valid) {
				nic_json["numa_node"] = nic_numa_info.node;
				nic_json["cpu_affinity"] = nic_numa_info.affinity;
			} else {
				nic_json["numa_node"] = "N/A";
				nic_json["cpu_affinity"] = "N/A";
			}
			json_output.push_back(nic_json);
		} else if (arg.output == human) {
			if (numa_info_valid) {
				out.append(string_format("%-13s%-13d%-13s\n",
				nic_bdf_vector[i].c_str(),
				nic_numa_info.node,
				string_format("[%s]", nic_numa_info.affinity).c_str()));
			} else {
				out.append(string_format("%-13s%-13s%-13s\n",
				nic_bdf_vector[i].c_str(),
				"N/A",
				"N/A"));
			}
		}
	}

	if (arg.output == json) {
		out = json_output.dump(4);
	}

	return AMDSMI_STATUS_SUCCESS;
}
