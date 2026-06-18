/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>

#include "amdsmi.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"
#include "smi_cli_node_command.h"
#include "smi_cli_helpers.h"
#include "smi_cli_api_base.h"
#include "smi_cli_templates.h"

#include "json/json.h"

auto constexpr baseboard_csv_header {",baseboard_temperature_ubb_fpga,baseboard_temperature_ubb_front,baseboard_temperature_ubb_back,"
	"baseboard_temperature_ubb_oam7,baseboard_temperature_ubb_ibc,baseboard_temperature_ubb_ufpga,baseboard_temperature_ubb_oam1,"
	"baseboard_temperature_oam_0_1_hsc,baseboard_temperature_oam_2_3_hsc,baseboard_temperature_oam_4_5_hsc,baseboard_temperature_oam_6_7_hsc,"
	"baseboard_temperature_ubb_fpga_0v72_vr,baseboard_temperature_ubb_fpga_3v3_vr,baseboard_temperature_retimer_0_1_2_3_1v2_vr,"
	"baseboard_temperature_retimer_4_5_6_7_1v2_vr,baseboard_temperature_retimer_0_1_0v9_vr,baseboard_temperature_retimer_4_5_0v9_vr,"
	"baseboard_temperature_retimer_2_3_0v9_vr,baseboard_temperature_retimer_6_7_0v9_vr,baseboard_temperature_oam_0_1_2_3_3v3_vr,"
	"baseboard_temperature_oam_4_5_6_7_3v3_vr,baseboard_temperature_ibc_hsc,baseboard_temperature_ibc,"
	"baseboard_power_ubb,baseboard_power_ubb_threshold"};
auto constexpr node_header_csv {"node"};
auto constexpr power_management_header_csv {",power_management_limit,power_management_status"};

int AmdSmiNodeCommand::node_command_baseboard(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_baseboard_command(processor,
			  arg, formatted_string);
	return ret;
}

int AmdSmiNodeCommand::node_command_npm(uint64_t processor, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_node_npm_info_command(processor, arg, formatted_string);
	return ret;
}

void AmdSmiNodeCommand::node_command_human()
{
	int ret;
	bool is_supported = false;
	std::string formatted_string{};
	std::string out{};

	out += NodeHeaderTemplate;

	if ((std::find(arg.options.begin(), arg.options.end(), "power-management") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end()) ||
		arg.all_arguments) {
		uint64_t gpu_bdf = arg.devices[0]->get_bdf();
		ret = node_command_npm(gpu_bdf, formatted_string);
		is_supported = (ret == 0);
		std::string param{"power-management"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += formatted_string;
			formatted_string.clear();
		}
		formatted_string.clear();
	}

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		if ((std::find(arg.options.begin(), arg.options.end(), "baseboard") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
				arg.all_arguments) {
			ret = node_command_baseboard(gpu_bdf, formatted_string);
			is_supported = (ret == 0);
			std::string param{"baseboard"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				out += formatted_string;
				formatted_string.clear();
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

void AmdSmiNodeCommand::node_command_json()
{
	int ret;
	unsigned int i;
	bool is_supported = false;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	std::string out{};
	std::string result{};

	if ((std::find(arg.options.begin(), arg.options.end(), "power-management") != arg.options.end()) ||
		(std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end()) ||
		arg.all_arguments) {
		nlohmann::ordered_json values_json;
		uint64_t gpu_bdf = arg.devices[0]->get_bdf();

		ret = node_command_npm(gpu_bdf, out);
		is_supported = (ret == 0);
		std::string param{"power-management"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			values_json = nlohmann::ordered_json::parse(out);
			json["power_management"] = values_json;
			out.clear();
		}
		if (!json.empty()) {
			json_format.insert(json_format.end(), json);
		}
	}

	for (i = 0; i < arg.devices.size(); i++) {
		if ((std::find(arg.options.begin(), arg.options.end(), "baseboard") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
				arg.all_arguments) {
			json = {};
			nlohmann::ordered_json values_json;
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();

			std::string param{"baseboard"};
			ret = node_command_baseboard(gpu_bdf, out);
			is_supported = (ret == 0);
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				values_json = nlohmann::ordered_json::parse(out);
				json["baseboard"] = values_json;
				out.clear();
			}
			out.clear();
			if (!json.empty()) {
				json_format.insert(json_format.end(), json);
			}
		}
		json.clear();
	}

	nlohmann::ordered_json result_json;
	result_json["node"] = json_format;
	result = result_json.dump(4);
	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << '\n';
	}
}

void AmdSmiNodeCommand::node_command_csv()
{
	int ret;
	bool is_supported = false;
	std::string header{};
	std::string formatted_string{};
	std::string values{};
	std::string out{};

	std::vector<std::vector<std::string>> results;
	std::string output_buffer{};
	std::string node_id{"0"};
	header.append(node_header_csv);

	uint64_t gpu_bdf = arg.devices[0]->get_bdf();
	if ((std::find(arg.options.begin(), arg.options.end(), "power-management") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end()) ||
			arg.all_arguments) {
		ret = node_command_npm(gpu_bdf, formatted_string);
		is_supported = (ret == 0);
		std::string param{"n"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			header.append(power_management_header_csv);
			results.push_back({formatted_string});
			formatted_string.clear();
		}
		formatted_string.clear();
	}

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();
		int gpu_id = arg.devices[i]->get_gpu_index();

		if ((std::find(arg.options.begin(), arg.options.end(), "baseboard") != arg.options.end()) ||
				(std::find(arg.options.begin(), arg.options.end(), "b") != arg.options.end()) ||
				arg.all_arguments) {
			ret = node_command_baseboard(gpu_bdf, formatted_string);
			is_supported = (ret == 0);
			std::string param{"baseboard"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				header.append(baseboard_csv_header);
				results.push_back({formatted_string});
				formatted_string.clear();
			}
		}
		if (i == 0) {
			out.append(header);
			out.append("\n");
		}
		csv_recursion(output_buffer, results);
		out.append(node_id);
		out.append(output_buffer);
		results.clear();
		output_buffer.clear();
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
	out.clear();
	header.clear();

}

void AmdSmiNodeCommand::execute_command()
{
	if (AmdSmiPlatform::getInstance().is_mi350() && AmdSmiPlatform::getInstance().getInstance().is_host()) {
		// Check if at least one node feature is available before proceeding
		if (arg.all_arguments) {
			std::string output{};
			uint64_t gpu_bdf = arg.devices[0]->get_bdf();
			int npm_ret = node_command_npm(gpu_bdf, output);
			int baseboard_ret = node_command_baseboard(gpu_bdf, output);

			// Check if both features return "not supported" status
			bool npm_not_supported = (npm_ret == AMDSMI_STATUS_NOT_SUPPORTED ||
									  npm_ret == PARAM_NOT_SUPPORTED_ON_PLATFORM);
			bool baseboard_not_supported = (baseboard_ret == AMDSMI_STATUS_NOT_SUPPORTED ||
											baseboard_ret == PARAM_NOT_SUPPORTED_ON_PLATFORM);

			// If both features are not supported, the whole command is not supported
			if (npm_not_supported && baseboard_not_supported) {
				std::string command{"node"};
				throw SmiToolCommandNotSupportedException(command);
			}
		}

		if (arg.output == human) {
			node_command_human();
		} else if (arg.output == json) {
			node_command_json();
		} else if (arg.output == csv) {
			node_command_csv();
		}
	} else {
		std::string command{"node"};
		throw SmiToolCommandNotSupportedException(command);
	}
}
