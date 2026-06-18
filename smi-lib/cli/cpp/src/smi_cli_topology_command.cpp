/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_exception.h"
#include "smi_cli_topology_command.h"
#include "smi_cli_platform.h"
#include "smi_cli_templates.h"
#include "smi_cli_api_base.h"
#include "json/json.h"

AmdSmiTopologyCommand::AmdSmiTopologyCommand(Arguments args) : AmdSmiCommands(args)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().initTopology(arg, bdf_vector, nic_bdf_vector);
	if (ret != 0) {
		throw SmiToolSMILIBErrorException(ret);
	}
}

int AmdSmiTopologyCommand::topology_command_weight(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_weight_topology_command(arg,
			  bdf_vector, formatted_string);
	return ret;
}
int AmdSmiTopologyCommand::topology_command_hops(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_hops_topology_command(arg,
			  bdf_vector, formatted_string);
	return ret;

}
int AmdSmiTopologyCommand::topology_command_fb_sharing(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_fb_sharing_topology_command(
				  arg, bdf_vector, formatted_string);
	return ret;
}
int AmdSmiTopologyCommand::topology_command_link_type(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_link_type_topology_command(arg,
			  bdf_vector, formatted_string);
	return ret;
}

int AmdSmiTopologyCommand::p2p_capability_command_coherent(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_coherent_p2p_capability_command(
				  arg, bdf_vector, formatted_string);
	return ret;
}

int AmdSmiTopologyCommand::p2p_capability_command_atomics(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_atomics_p2p_capability_command(
				  arg, bdf_vector, formatted_string);
	return ret;
}

int AmdSmiTopologyCommand::p2p_capability_command_dma(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_dma_p2p_capability_command(
				  arg, bdf_vector, formatted_string);
	return ret;
}

int AmdSmiTopologyCommand::p2p_capability_command_bi_directional(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bi_directional_p2p_capability_command(
				  arg, bdf_vector, formatted_string);
	return ret;
}

int AmdSmiTopologyCommand::topology_command_all_status(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_all_topology_command(
				  arg, bdf_vector, formatted_string);
	return ret;
}

int AmdSmiTopologyCommand::nic_topology_command_link_type(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_nic_link_type_topology_command(arg,
			  bdf_vector, nic_bdf_vector, formatted_string);
	return ret;
}

int AmdSmiTopologyCommand::nic_topology_command_numa(std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_nic_numa_topology_command(arg,
			  bdf_vector, nic_bdf_vector, formatted_string);
	return ret;
}

void AmdSmiTopologyCommand::topology_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};

	if (arg.devices_type == GPU_TYPE || arg.devices_type == ALL_TYPE) {
		if ((std::find(arg.options.begin(), arg.options.end(), "weight") !=
				arg.options.end()) ||
				arg.all_arguments) {
			ret = topology_command_weight(formatted_string);
			std::string param{"weight"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "hops") !=
				arg.options.end()) ||
				arg.all_arguments) {
			ret = topology_command_hops(formatted_string);
			std::string param{"hops"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "fb-sharing") !=
				arg.options.end()) ||
				arg.all_arguments) {
			ret = topology_command_fb_sharing(formatted_string);
			std::string param{"fb-sharing"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "link-type") !=
				arg.options.end()) ||
				arg.all_arguments) {
			ret = topology_command_link_type(formatted_string);
			std::string param{"link-type"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "coherent") !=
				arg.options.end()) ||
				arg.all_arguments) {
			ret = p2p_capability_command_coherent(formatted_string);
			std::string param{"coherent"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "atomics") !=
				arg.options.end()) ||
				arg.all_arguments) {
			ret = p2p_capability_command_atomics(formatted_string);
			std::string param{"atomics"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "dma") !=
				arg.options.end()) ||
				arg.all_arguments) {
			ret = p2p_capability_command_dma(formatted_string);
			std::string param{"dma"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}

		if ((std::find(arg.options.begin(), arg.options.end(), "bi-dir") !=
				arg.options.end()) ||
				arg.all_arguments) {
			ret = p2p_capability_command_bi_directional(formatted_string);
			std::string param{"bi-dir"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}
	}
	if (arg.devices_type == NIC_TYPE || arg.devices_type == ALL_TYPE) {
		if ((std::find(arg.options.begin(), arg.options.end(), "link-type") != arg.options.end()) ||
				arg.all_arguments) {
			ret = nic_topology_command_link_type(formatted_string);
			std::string param{"nic-link-type"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}
		if ((std::find(arg.options.begin(), arg.options.end(), "numa") != arg.options.end()) ||
				arg.all_arguments) {
			ret = nic_topology_command_numa(formatted_string);
			std::string param{"numa"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
			}
			formatted_string.clear();
		}
	}
	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiTopologyCommand::topology_command_json()
{
	int ret;
	int error = 0;
	std::string out{};
	std::string param{"topology"};
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();

	if (arg.devices_type == GPU_TYPE || arg.devices_type == ALL_TYPE) {
		std::string gpu_out{};
		ret = topology_command_all_status(gpu_out);
		error = handle_exceptions(ret, param, arg);
		if (error == 0 && !gpu_out.empty()) {
			nlohmann::ordered_json gpu_json = nlohmann::ordered_json::parse(gpu_out);
			for (auto& item : gpu_json) {
				json_format.push_back(item);
			}
		}
	}

	if (arg.devices_type == NIC_TYPE || arg.devices_type == ALL_TYPE) {
		if ((std::find(arg.options.begin(), arg.options.end(), "link-type") != arg.options.end()) ||
				arg.all_arguments) {
			std::string nic_out{};
			ret = nic_topology_command_link_type(nic_out);
			std::string nic_param{"nic-link-type"};
			error = handle_exceptions(ret, nic_param, arg);
			if (error == 0 && !nic_out.empty()) {
				nlohmann::ordered_json nic_json = nlohmann::ordered_json::parse(nic_out);
				for (auto& item : nic_json) {
					json_format.push_back(item);
				}
			}
		}
		if ((std::find(arg.options.begin(), arg.options.end(), "numa") != arg.options.end()) ||
				arg.all_arguments) {
			std::string nic_out{};
			ret = nic_topology_command_numa(nic_out);
			std::string nic_param{"numa"};
			error = handle_exceptions(ret, nic_param, arg);
			if (error == 0 && !nic_out.empty()) {
				nlohmann::ordered_json nic_json = nlohmann::ordered_json::parse(nic_out);
				for (auto& item : nic_json) {
					json_format.push_back(item);
				}
			}
		}
	}

	out = json_format.dump(4);

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}

}


void AmdSmiTopologyCommand::execute_command()
{
	unsigned int gpu_count = 0;
	unsigned int nic_count = 0;
	unsigned int brcm_nic_count = 0;

	AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));
	AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_device_count(nic_count, static_cast<int>(DeviceType::NIC));
	AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_device_count(brcm_nic_count, static_cast<int>(DeviceType::BRCM_NIC));
	nic_count += brcm_nic_count;
	if ((AmdSmiPlatform::getInstance().getInstance().is_mi350()
			|| AmdSmiPlatform::getInstance().is_mi300()
			|| AmdSmiPlatform::getInstance().is_mi200())
			&& AmdSmiPlatform::getInstance().getInstance().is_host()) {
		if ((gpu_count > 1) || (nic_count >= 1 && gpu_count >= 1)) {
			if (arg.output == human) {
				topology_command_human();
			} else if (arg.output == json) {
				topology_command_json();
			} else if (arg.output == csv) {
				std::string param{"--csv"};
				throw SmiToolParameterNotSupportedException(param);
			}
		} else {
			std::string command{"topology"};
			throw SmiToolCommandNotSupportedException(command);
		}
	} else {
		std::string command{"topology"};
		throw SmiToolCommandNotSupportedException(command);
	}
}
