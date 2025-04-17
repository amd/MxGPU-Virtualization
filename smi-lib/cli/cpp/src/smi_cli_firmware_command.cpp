/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <iostream>
#include <sstream>
#include <regex>
#include <stdexcept>

#include "json/json.h"

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_firmware_command.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_api_base.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

auto constexpr fw_list_header_csv {",fw_id,fw_version"};
auto constexpr
error_records_header_csv {",error_record_timestamp,error_record_vf_idx,error_record_status"};
auto constexpr gpu_header_csv {"gpu"};
auto constexpr vf_header_csv {",vf"};

int AmdSmiFirmwareCommand::firmware_command_fw_list(uint64_t processor,
		std::string &out, std::string *gpu_id)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_firmware_fw_list_command(processor,
			  arg, out, gpu_id);
	return ret;
}

int AmdSmiFirmwareCommand::firmware_command_err_rec(uint64_t processor, std::string &out)
{
	if (AmdSmiPlatform::getInstance().is_guest() || AmdSmiPlatform::getInstance().is_baremetal()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}

	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_firmware_err_rec_command(processor,
			  arg, out);
	return ret;
}

int AmdSmiFirmwareCommand::firmware_command_vf_fw_list(std::string vf_handle,
		std::string &out_string, std::string *gpu_id, std::string *vf_id)
{
	if (AmdSmiPlatform::getInstance().is_guest() || AmdSmiPlatform::getInstance().is_baremetal()) {
		return PARAM_NOT_SUPPORTED_ON_PLATFORM;
	}

	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_firmware_vf_fw_list_command(vf_handle,
			  arg, gpu_id, vf_id, out_string);
	return ret;
}

void AmdSmiFirmwareCommand::firmware_command_json()
{
	int ret;
	unsigned int i;
	nlohmann::ordered_json json_format = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;
	std::string out{};
	std::string result{};
	nlohmann::ordered_json values_json;

	if (arg.is_vf) {
		nlohmann::ordered_json firmware_vf_json;
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		vf_bdf = std::get<2>(indexes).c_str();
		std::string gpu_index = std::get<0>(indexes).c_str();
		std::string vf_index =  std::get<1>(indexes).c_str();
		std::string param{"vf-fw-list"};
		ret = firmware_command_vf_fw_list(vf_bdf, out, &gpu_index, &vf_index);
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			firmware_vf_json = nlohmann::ordered_json::parse(out);
			json_format.insert(json_format.end(), firmware_vf_json);
		}
		else if (error == PARAM_NOT_SUPPORTED_ON_PLATFORM){
			throw SmiToolParameterNotSupportedException(param);
		}
		else{
			log_err.log(LogLevel::Error, ret, __FUNCTION__, __FILE__,
					__LINE__);
		}

		if (arg.is_file) {
			write_to_file(arg.file_path, json_format.dump(4));
		} else {
			std::cout << std::setw(4) << json_format.dump(4) << '\n';
		}
	} else {
		for (i = 0; i < arg.devices.size(); i++) {
			json["gpu"] = arg.devices[i]->get_gpu_index();
			nlohmann::ordered_json values_json;
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();

			if ((std::find(arg.options.begin(), arg.options.end(), "fw-list") !=
					arg.options.end() ||
					std::find(arg.options.begin(), arg.options.end(), "ucode-list") !=
					arg.options.end() ||
					std::find(arg.options.begin(), arg.options.end(), "f") !=
					arg.options.end()) ||
					arg.all_arguments) {
				std::string param{"fw-list"};
				ret = firmware_command_fw_list(gpu_bdf, out);
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					values_json = nlohmann::ordered_json::parse(out);
					out.clear();
					json["fw_list"] = values_json;
				}
				out.clear();
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "error-records") !=
					arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "e") !=
					 arg.options.end()) ||
					arg.all_arguments) {

				ret = firmware_command_err_rec(gpu_bdf, out);
				std::string param{"error-records"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					if (!out.empty()) {
						values_json = nlohmann::ordered_json::parse(out);
						out.clear();
						json["error_records"] = values_json;
					}
				}
				out.clear();
			}
			json_format.insert(json_format.end(), json);
			json.clear();
		}
		result = json_format.dump(4);
	}
	if (arg.is_file) {
		write_to_file(arg.file_path, result);
	} else {
		std::cout << std::setw(4) << result << '\n';
	}
}

void AmdSmiFirmwareCommand::firmware_command_human()
{
	int ret;
	std::string formatted_string{};
	std::string out{};

	if (arg.is_vf) {
		nlohmann::ordered_json firmware_vf_json;
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		vf_bdf = std::get<2>(indexes).c_str();
		ret = firmware_command_vf_fw_list(vf_bdf, out);
		std::string param{"vf-fw-list"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			out += string_format(
				   vfNestedTemplate, std::get<0>(indexes).c_str(),
				   std::get<1>(indexes).c_str());
			out += formatted_string;
			formatted_string.clear();
			}
		else if (error == PARAM_NOT_SUPPORTED_ON_PLATFORM){
			throw SmiToolParameterNotSupportedException(param);
		}
		else{
			log_err.log(LogLevel::Error, ret, __FUNCTION__, __FILE__,
					__LINE__);
		}
	} else {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			out += string_format(gpuTemplate, arg.devices[i]->get_gpu_index());
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();

			if ((std::find(arg.options.begin(), arg.options.end(), "fw-list") !=
					arg.options.end() ||
					std::find(arg.options.begin(), arg.options.end(), "ucode-list") !=
					arg.options.end() ||
					std::find(arg.options.begin(), arg.options.end(), "f") !=
					arg.options.end()) ||
					arg.all_arguments) {
				ret = firmware_command_fw_list(gpu_bdf, formatted_string);
				std::string param{"fw-list"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "error-records") !=
					arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "e") !=
					 arg.options.end()) ||
					arg.all_arguments) {
				ret = firmware_command_err_rec(gpu_bdf, formatted_string);
				std::string param{"error-records"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					out += formatted_string;
					formatted_string.clear();
				}
			}
		}
	}

	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
}

void AmdSmiFirmwareCommand::firmware_command_csv()
{
	int ret;
	std::string headers{};
	std::string values{};
	std::string formatted_string{};
	std::string out{};
	std::string gpu_id{};
	headers.append(gpu_header_csv);
	std::string vf_id{};

	if (arg.is_vf) {
		std::string vf_bdf;
		std::string gfx_timeslice_us_str;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		gpu_id = std::get<0>(indexes).c_str();
		vf_id = std::get<1>(indexes).c_str();
		vf_bdf = std::get<2>(indexes).c_str();
		ret = firmware_command_vf_fw_list(vf_bdf, formatted_string, &gpu_id, &vf_id);
		std::string param{"vf-fw-list"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			headers.append(vf_header_csv);
			headers.append(fw_list_header_csv);
			out.append(headers).append(formatted_string);
			formatted_string.clear();
		}
		else if (error == PARAM_NOT_SUPPORTED_ON_PLATFORM){
			throw SmiToolParameterNotSupportedException(param);
		}
		else{
			log_err.log(LogLevel::Error, ret, __FUNCTION__, __FILE__,
					__LINE__);
		}
	} else {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			gpu_id = string_format("%d", arg.devices[i]->get_gpu_index());
			if ((std::find(arg.options.begin(), arg.options.end(), "fw-list") !=
					arg.options.end() ||
					std::find(arg.options.begin(), arg.options.end(), "ucode-list") !=
					arg.options.end() ||
					std::find(arg.options.begin(), arg.options.end(), "f") !=
					arg.options.end()) ||
					arg.all_arguments) {
				ret = firmware_command_fw_list(gpu_bdf, formatted_string, &gpu_id);
				std::string param{"fw-list"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					headers.append(fw_list_header_csv);
					values.append(formatted_string);
					formatted_string.clear();
				}
			}
			if ((std::find(arg.options.begin(), arg.options.end(), "error-records") !=
					arg.options.end()) ||
					(std::find(arg.options.begin(), arg.options.end(), "e") !=
					 arg.options.end()) ||
					arg.all_arguments) {
				ret = firmware_command_err_rec(gpu_bdf, formatted_string);
				std::string param{"error-records"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					headers.append(error_records_header_csv);
					if(formatted_string.size() > 0) {
						values.append(",").append(formatted_string);
					} else {
						if (!((std::find(arg.options.begin(), arg.options.end(), "fw-list") !=
								arg.options.end() ||
								std::find(arg.options.begin(), arg.options.end(), "ucode-list") !=
								arg.options.end() ||
								std::find(arg.options.begin(), arg.options.end(), "f") !=
								arg.options.end()) ||
								arg.all_arguments)) {
							values.append(gpu_id.c_str()).append(",,,,\n");
						}
					}
					formatted_string.clear();
				}
			}
			if (i == 0) {
				out.append(headers).append("\n");
			}
			out.append(values);
			values.clear();
		}
	}
	if (arg.is_file) {
		write_to_file(arg.file_path, out);
	} else {
		std::cout << out.c_str() << std::endl;
	}
	out.clear();
}

void AmdSmiFirmwareCommand::execute_command()
{
	if(AmdSmiPlatform::getInstance().is_guest()) {
		throw SmiToolCommandNotSupportedException("firmware");
	}

	if (arg.output == json) {
		firmware_command_json();
	}
	if (arg.output == csv) {
		firmware_command_csv();
	}
	if (arg.output == human) {
		firmware_command_human();
	}
};
