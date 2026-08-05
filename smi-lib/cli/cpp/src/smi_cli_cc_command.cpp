/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */


#include "smi_cli_cc_command.h"
#include "smi_cli_helpers.h"
#include "smi_cli_templates.h"
#include "smi_cli_api_base.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

#include "json/json.h"

auto constexpr cc_mode_csv_header {",cc-mode"};

int AmdSmiCCCommand::cc_command_vf_get_tdi_state(std::string vf_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vf_tdi_state_command(vf_bdf,
			  arg, formatted_string);
	return ret;
}

int AmdSmiCCCommand::cc_command_get_mode(uint64_t processors, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_cc_mode_command(processors,
			  arg, formatted_string);
	return ret;
}

void AmdSmiCCCommand::cc_command_json()
{
	int ret;
	unsigned int i;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	nlohmann::ordered_json option_json;
	std::string out{};
	std::string result{};

	if (arg.is_vf) {
		nlohmann::ordered_json values_json;
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		vf_bdf = std::get<2>(indexes).c_str();
		json = {};
		uint64_t gpu_index = std::stoi(std::get<0>(indexes));
		uint64_t vf_index = std::stoi(std::get<1>(indexes));
		json["gpu"] = gpu_index;
		json["vf"] = vf_index;

		std::string param_tdi{"tdi-state"};
		ret = cc_command_vf_get_tdi_state(vf_bdf, out);
		int error_tdi = handle_exceptions(ret, param_tdi, arg);
		if (error_tdi == 0) {
			values_json = nlohmann::ordered_json::parse(out);
			json["tdi_state"] = values_json;
			out.clear();
		}
		out.clear();

		if (!json.empty()) {
			json_format.insert(json_format.end(), json);
		}
		result = json_format.dump(4);
	} else {
		for (i = 0; i < arg.devices.size(); i++) {
			json = {};
			option_json = {};
			nlohmann::ordered_json values_json;
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();

			std::string param_mode{"mode"};
			ret = cc_command_get_mode(gpu_bdf, out);
			int error_mode = handle_exceptions(ret, param_mode, arg);
			if (error_mode == 0) {
				values_json = nlohmann::ordered_json::parse(out);
				option_json["cc_mode"] = values_json;
				out.clear();
			}
			out.clear();

			if (!option_json.empty()) {
				json["gpu"] = arg.devices[i]->get_gpu_index();
				for (auto& [key, value] : option_json.items()) {
					json[key] = value;
				}
				json_format.insert(json_format.end(), json);
			}
		}

		result = json_format.dump(4);
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << '\n';
	}
}

void AmdSmiCCCommand::cc_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};
	std::string options_string{};

	if (arg.is_vf) {
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		out += string_format(
				   vfNestedTemplate, std::get<0>(indexes).c_str(),
				   std::get<1>(indexes).c_str());
		vf_bdf = std::get<2>(indexes).c_str();

		ret = cc_command_vf_get_tdi_state(vf_bdf, formatted_string);
		std::string param_tdi{"tdi-state"};
		int error_tdi = handle_exceptions(ret, param_tdi, arg);
		if (error_tdi == 0) {
			out += formatted_string;
		}
		formatted_string.clear();
	} else {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();

			ret = cc_command_get_mode(gpu_bdf, formatted_string);
			std::string param_mode{"mode"};
			int error_mode = handle_exceptions(ret, param_mode, arg);
			if (error_mode == 0) {
				options_string += formatted_string;
			}
			formatted_string.clear();

			if (!options_string.empty()) {
				out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
				out += options_string;
				options_string.clear();
			}
			options_string.clear();
		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiCCCommand::cc_command_csv()
{
	int ret;
	std::string header{};
	std::string formatted_string{};
	std::string out{};

	if (arg.is_vf) {
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		vf_bdf = std::get<2>(indexes).c_str();
		uint64_t gpu_index = std::stoi(std::get<0>(indexes));
		uint64_t vf_index = std::stoi(std::get<1>(indexes));

		header.append("gpu,vf,tdi_state");
		out.append(header).append("\n");

		std::string values{};
		values.append(string_format("%d,%d", (int)gpu_index, (int)vf_index));

		ret = cc_command_vf_get_tdi_state(vf_bdf, formatted_string);
		std::string param_tdi{"tdi-state"};
		int error_tdi = handle_exceptions(ret, param_tdi, arg);
		if (error_tdi == 0) {
			values.append(formatted_string);
			formatted_string.clear();
		}
		formatted_string.clear();

		out.append(values).append("\n");
	} else {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			int gpu_id = arg.devices[i]->get_gpu_index();
			std::string values{};

			std::string gpu_id_str{string_format("%d",gpu_id)};
			values.append(gpu_id_str);

			if (i == 0) {
				header.append("gpu");
				header.append(cc_mode_csv_header);
			}

			ret = cc_command_get_mode(gpu_bdf, formatted_string);
			std::string param_mode{"mode"};
			int error_mode = handle_exceptions(ret, param_mode, arg);
			if (error_mode == 0) {
				values.append(formatted_string);
				formatted_string.clear();
			}
			formatted_string.clear();

			if (i == 0) {
				out.append(header).append("\n");
			}
			out.append(values).append("\n");
		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}

	out.clear();
	header.clear();
}

void AmdSmiCCCommand::execute_command()
{
	if (!(AmdSmiPlatform::getInstance().is_mixxx() &&
	      AmdSmiPlatform::getInstance().is_host())) {
		std::string command{"confidential-compute"};
		throw SmiToolCommandNotSupportedException(command);
	}

	if (arg.output == json) {
		cc_command_json();
	}
	if (arg.output == csv) {
		cc_command_csv();
	}
	if (arg.output == human) {
		cc_command_human();
	}
}
