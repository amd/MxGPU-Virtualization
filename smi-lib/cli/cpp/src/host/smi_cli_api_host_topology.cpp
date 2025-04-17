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
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_exception.h"

#include "json/json.h"

#include <sstream>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_TOPOLOGY)(amdsmi_processor_handle,
		amdsmi_processor_handle,
		amdsmi_link_topology_t *);


extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_LINK_TOPOLOGY host_amdsmi_get_link_topology;

std::vector<std::vector<amdsmi_link_topology_t>> topology;


int AmdSmiApiHost::initTopology(std::vector<std::shared_ptr<Device> > devices,
								std::vector<std::string>& bdf_vector)
{
	amdsmi_link_topology_t topology_info;
	amdsmi_link_topology_t topology_empty_info{};
	amdsmi_bdf_t tmp_bdf;
	amdsmi_bdf_t bdf;
	amdsmi_processor_handle gpu_handle;
	amdsmi_socket_handle socket = NULL;
	std::string bdf_string{};
	unsigned int gpu_count;
	amdsmi_processor_handle *processors;

	amdsmi_get_gpu_count(gpu_count);

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	int ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		free(processors);
		return ret;
	}

	for (unsigned int i = 0; i < devices.size(); i++) {

		std::vector<amdsmi_link_topology_t> inner_vector;

		tmp_bdf.as_uint = devices[i]->get_bdf();

		ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &gpu_handle);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			free(processors);
			return ret;
		}

		for (unsigned int j = 0; j < gpu_count; j++) {
			if(i == (devices.size() - 1)) {
				ret = host_amdsmi_get_gpu_device_bdf(processors[j], &bdf);
				if (ret != AMDSMI_STATUS_SUCCESS) {
					Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
					free(processors);
					return ret;
				}
				bdf_string = string_format(
								 "%04x:%02x:%02x.%01x", bdf.bdf.domain_number, bdf.bdf.bus_number, bdf.bdf.device_number,
								 bdf.bdf.function_number);
				bdf_vector.push_back(bdf_string);
				bdf_string.clear();
			}
			ret = host_amdsmi_get_link_topology(gpu_handle,processors[j],
												&topology_info);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__,
										  __LINE__);
				inner_vector.push_back(topology_empty_info);
			} else {
				inner_vector.push_back(topology_info);
			}
		}
		topology.push_back(inner_vector);
	}

	free(processors);
	return ret;
}

int AmdSmiApiHost::amdsmi_get_weight_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_gpu_count(gpu_count);

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
			out.append(string_format("%-13ld",
									 topology[i][j].weight));
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

	amdsmi_get_gpu_count(gpu_count);

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
			out.append(string_format("%-13ld",
									 topology[i][j].num_hops));
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

	amdsmi_get_gpu_count(gpu_count);


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
			out.append(string_format("%-13s",
									 topology[i][j].fb_sharing ? "ENABLED" : "DISABLED"));
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

	amdsmi_get_gpu_count(gpu_count);

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
			out.append(string_format("%-13s",
									 link_type_string.c_str()));
		}
		out.append("\n");
	}
	out.append("\n");

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_link_status_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i;
	unsigned int j;
	unsigned int gpu_count;

	amdsmi_get_gpu_count(gpu_count);

	out.append(topologyLinkStatusTemplate);
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
			std::string link_status_string;
			if(gpu_index == j) {
				link_status_string = "N/A";
			} else {
				format_link_status(topology[i][j].link_status, link_status_string);
			}

			out.append(string_format("%-13s",
									 link_status_string.c_str()));
		}
		out.append("\n");
	}
	out.append("\n");

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_all_topology_command(Arguments arg,
		std::vector<std::string> bdf_vector, std::string& out)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int gpu_count;
	nlohmann::ordered_json output = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;

	amdsmi_get_gpu_count(gpu_count);

	for (int i = 0; i < arg.devices.size(); i++) {
		json = {};
		int gpu_index = arg.devices[i]->get_gpu_index();
		json["gpu"] = gpu_index;
		json["bdf"] = bdf_vector[gpu_index].c_str();
		json["links"] = nlohmann::ordered_json::array();
		for (int j = 0; j < gpu_count; j++) {
			nlohmann::ordered_json link_topology = {};
			link_topology["gpu"] = j;
			link_topology["bdf"] = bdf_vector[j].c_str();

			if (std::find(arg.options.begin(), arg.options.end(), "weight") != arg.options.end() ||
					arg.all_arguments) {
				link_topology["weight"] = topology[i][j].weight;
			}

			if (std::find(arg.options.begin(), arg.options.end(), "link-status") != arg.options.end() ||
					arg.all_arguments) {
				std::string link_status_string;
				if(i  == j) {
					link_status_string = "N/A";
				} else {
					format_link_status(topology[i][j].link_status, link_status_string);
				}
				link_topology["link_status"] =  link_status_string;
			}

			if (std::find(arg.options.begin(), arg.options.end(), "link-type") != arg.options.end() ||
					arg.all_arguments) {
				std::string  link_type_string;
				format_link_type(topology[i][j].link_type, link_type_string);
				link_topology["link_type"] = link_type_string;
			}
			if (std::find(arg.options.begin(), arg.options.end(), "hops") != arg.options.end() ||
					arg.all_arguments) {
				link_topology["num_hops"] = topology[i][j].num_hops;
			}
			if (std::find(arg.options.begin(), arg.options.end(), "fb-sharing") != arg.options.end() ||
					arg.all_arguments) {
				link_topology["fb_sharing"] = topology[i][j].fb_sharing ? "ENABLED" : "DISABLED";
			}
			json["links"].insert(json["links"].end(), link_topology);
		}

		output.insert(output.end(), json);
	}

	out = output.dump(4);

	return 0;
}
