/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */
#include <iostream>

#include "amdsmi.h"
#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_exception.h"
#include "smi_cli_fabric_command.h"
#include "smi_cli_platform.h"
#include "smi_cli_templates.h"
#include "smi_cli_api_base.h"
#include "json/json.h"
#include <stdint.h>

AmdSmiFabricCommand::AmdSmiFabricCommand(Arguments args) : AmdSmiCommands(args)
{
}

int AmdSmiFabricCommand::fabric_command_topology(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_fabric_topology_command(processor_bdf, arg,
			formatted_string);
	return ret;
}


int AmdSmiFabricCommand::fabric_command_telemetry(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_fabric_telemetry_command(processor_bdf, arg,
			formatted_string);
	return ret;
}

void AmdSmiFabricCommand::fabric_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string options_string{};
	std::string out{};


	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();

		if ((std::find(arg.options.begin(), arg.options.end(), "topology") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "T") != arg.options.end()) ||
			arg.all_arguments) {
			ret = fabric_command_topology(gpu_bdf, formatted_string);
			std::string param{"topology"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				options_string += formatted_string;
			}
			formatted_string.clear();
		}
		if ((std::find(arg.options.begin(), arg.options.end(), "telemetry") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end()) ||
			arg.all_arguments) {
			ret = fabric_command_telemetry(gpu_bdf, formatted_string);
			std::string param{"telemetry"};

			// Check if telemetry was explicitly requested
			bool telemetry_explicitly_requested =
				(std::find(arg.options.begin(), arg.options.end(), "telemetry") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end());

			if (ret == AMDSMI_STATUS_API_FAILED || ret == AMDSMI_STATUS_NOT_SUPPORTED) {
				// API_FAILED or NOT_SUPPORTED - either the UALOE/IFOE driver is not
				// loaded, or this build of the library does not include UALOE support.
				if (telemetry_explicitly_requested) {
					std::cerr << "Fabric telemetry is not available. Possible reasons:" << std::endl;
					std::cerr << "  - UALOE/IFOE driver is not loaded" << std::endl;
					std::cerr << "  - This build of the library does not include UALOE support" << std::endl;
					// Continue to output any successfully collected topology data
				} else if (arg.all_arguments && i == 0) {
					// Telemetry was implicitly attempted as part of the full
					// fabric report; show a brief note once so the user knows
					// why the telemetry section is missing.
					std::cerr << "Note: UALOE/IFOE driver is not loaded. Fabric telemetry will not be available." << std::endl;
				}
			} else if (ret == PARAM_NOT_SUPPORTED_ON_PLATFORM) {
				// Explicitly not supported on this platform
				if (telemetry_explicitly_requested) {
					std::cerr << "Fabric telemetry is not supported on this platform." << std::endl;
					// Continue to output any successfully collected topology data
				}
			} else if (ret == 0) {
				// Success - add telemetry data to output
				options_string += formatted_string;
			} else {
				// Other errors - use standard error handling
				int error = handle_exceptions(ret, param, arg);
				if (error == 0 && !formatted_string.empty()) {
					options_string += formatted_string;
				}
			}
			formatted_string.clear();
		}
		if (!options_string.empty()) {
			out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
			out += options_string;
			options_string.clear();
		}
		options_string.clear();
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiFabricCommand::fabric_command_json()
{

	int ret;
	unsigned int i;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	nlohmann::ordered_json option_json;
	std::string out{};
	std::string result{};
	std::string formatted_string{};
	for (i = 0; i < arg.devices.size(); i++) {
		json = {};
		option_json = {};
		nlohmann::ordered_json values_json;
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();

		if ((std::find(arg.options.begin(), arg.options.end(), "topology") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "T") != arg.options.end()) ||
			arg.all_arguments) {
			std::string param{"topology"};
			ret = fabric_command_topology(gpu_bdf, out);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				values_json = nlohmann::ordered_json::parse(out);
				option_json["topology"] = values_json;
				out.clear();
			}
			out.clear();
		}
		if ((std::find(arg.options.begin(), arg.options.end(), "telemetry") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end()) ||
			arg.all_arguments) {
			ret = fabric_command_telemetry(gpu_bdf, formatted_string);
			std::string param{"telemetry"};

			// Check if telemetry was explicitly requested
			bool telemetry_explicitly_requested =
				(std::find(arg.options.begin(), arg.options.end(), "telemetry") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "t") != arg.options.end());

			if (ret == AMDSMI_STATUS_API_FAILED || ret == AMDSMI_STATUS_NOT_SUPPORTED) {
				// API_FAILED or NOT_SUPPORTED - either the UALOE/IFOE driver is not
				// loaded, or this build of the library does not include UALOE support.
				if (telemetry_explicitly_requested) {
					nlohmann::ordered_json error_json;
					error_json["error"] = "Fabric telemetry is not available";
					error_json["possible_reasons"] = {
						"UALOE/IFOE driver is not loaded",
						"This build of the library does not include UALOE support"
					};
					std::cerr << error_json.dump(4) << std::endl;
					// Continue to output any successfully collected topology data
				}
			} else if (ret == PARAM_NOT_SUPPORTED_ON_PLATFORM) {
				// Explicitly not supported on this platform
				if (telemetry_explicitly_requested) {
					nlohmann::ordered_json error_json;
					error_json["error"] = "Fabric telemetry is not supported on this platform";
					std::cerr << error_json.dump(4) << std::endl;
					// Continue to output any successfully collected topology data
				}
			} else if (ret == 0) {
				// Success - add telemetry data to output
				values_json = nlohmann::ordered_json::parse(formatted_string);
				option_json["telemetry"] = values_json;
			} else {
				int error = handle_exceptions(ret, param, arg);
				if (error == 0 && !formatted_string.empty()) {
					values_json = nlohmann::ordered_json::parse(formatted_string);
					option_json["telemetry"] = values_json;
				}
			}
			formatted_string.clear();
		}
		if (!option_json.empty()) {
			json["gpu"] = arg.devices[i]->get_gpu_index();
			for (auto& [key, value] : option_json.items()) {
				json[key] = value;
			}
			json_format.insert(json_format.end(), json);
		}
	}

	out = json_format.dump(4);

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}

}


void AmdSmiFabricCommand::execute_command()
{
	unsigned int gpu_count;
	AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));
	if (AmdSmiPlatform::getInstance().is_mixxx()
			&& AmdSmiPlatform::getInstance().is_host()) {
		if (arg.output == human) {
			fabric_command_human();
		} else if (arg.output == json) {
			std::string param{"--json"};
			fabric_command_json();
		} else if (arg.output == csv) {
			std::string param{"--csv"};
			throw SmiToolParameterNotSupportedException(param);
		}
	} else {
		std::string command{"fabric"};
		throw SmiToolCommandNotSupportedException(command);
	}
}
