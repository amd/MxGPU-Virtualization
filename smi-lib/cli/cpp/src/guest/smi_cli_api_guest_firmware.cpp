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
#include "interface/amdsmi.h"
#include "smi_cli_api_guest.h"
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
typedef amdsmi_status_t (*AMDSMI_GET_FW_INFO)(amdsmi_processor_handle, amdsmi_fw_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF guest_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_FW_INFO guest_amdsmi_get_fw_info;

std::string guest_fill_fw_list(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		auto fw_list_json = nlohmann::ordered_json::array();
		fw_list_json.push_back(nlohmann::ordered_json::object( {
			{ "fw_id", value.c_str() },
			{ "fw_version", value.c_str() } }));
		out = fw_list_json.dump(4);
	} else if (arg.output == csv) {
		out += string_format(
				   "%s,%s,%s\n", value.c_str(), value.c_str(),
				   value.c_str());
	} else {
		out = fwListTemplate;
		out += string_format(
				   fwTemplate, value.c_str(), value.c_str(),
				   value.c_str());
	}

	return out;
}

int AmdSmiApiGuest::amdsmi_firmware_fw_list_command(uint64_t processor_bdf, Arguments arg,
		std::string &out_string, std::string *gpu_id)
{
	std::string out{};
	amdsmi_fw_info_t fw_info;
	std::string fw_name_str;
	std::string fw_version_str;
	amdsmi_status_t ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = guest_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	auto fw_list_json = nlohmann::ordered_json::array();

	if (arg.output == human) {
		out += fwListTemplate;
	}

	ret = guest_amdsmi_get_fw_info(processor, &fw_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		guest_fill_fw_list(arg, "N/A");
		return ret;
	}
	uint8_t num_fw = fw_info.num_fw_info;
	for (uint8_t fw_iterator = 0; fw_iterator < num_fw; fw_iterator++) {
		auto fw_id = fw_info.fw_info_list[fw_iterator].fw_id;
		auto fw_version = fw_info.fw_info_list[fw_iterator].fw_version;

		if (fw_version != 0) {
			fw_version_str =
				transform_fw(
					fw_id, fw_version);
			get_string_from_enum_fw_block(fw_id, fw_name_str);
			if (arg.output == json) {
				fw_list_json.push_back(nlohmann::ordered_json::object( {
					{ "fw_id", fw_name_str.c_str() },
					{ "fw_version", fw_version_str.c_str() } }));
			} else if (arg.output == csv) {
				out += string_format(
						   "%s,%s,%s\n", (*gpu_id).c_str(), fw_name_str.c_str(),
						   fw_version_str.c_str());
			} else {
				out += string_format(
						   fwTemplate, fw_iterator, fw_name_str.c_str(),
						   fw_version_str.c_str());
			}
		}
	}

	if (arg.output == json) {
		out_string += fw_list_json.dump(4);
	} else {
		out_string += out;
	}

	return AMDSMI_STATUS_SUCCESS;
}
