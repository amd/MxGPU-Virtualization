/* * Copyright (C) 2025 Advanced Micro Devices. All rights reserved.
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
 #include "amdsmi.h"
 #include "smi_cli_api_host.h"
 #include "smi_cli_parser.h"
 #include "smi_cli_logger_err.h"
 #include "smi_cli_templates.h"
 #include "smi_cli_device.h"
 #include "smi_cli_platform.h"
 
 #include "json/json.h"

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_CC_MODE)(amdsmi_processor_handle,
		amdsmi_cc_mode_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TDI_STATE)(amdsmi_vf_handle_t,
		amdsmi_tdi_state_t *);

typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_vf_handle_t *);

typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_VF_INDEX)(amdsmi_processor_handle,
		uint32_t, amdsmi_vf_handle_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
extern AMDSMI_GET_VF_HANDLE_FROM_VF_INDEX host_amdsmi_get_vf_handle_from_vf_index;
extern AMDSMI_GET_CC_MODE host_amdsmi_get_cc_mode;
extern AMDSMI_GET_TDI_STATE host_amdsmi_get_tdi_state;

std::string host_fill_tdi_state(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json tdi_state_json = value.c_str();
		out = tdi_state_json.dump(4);
	}
	else if (arg.output == csv) {
		out = string_format(",%s", value.c_str());
	}
	else {
		out = string_format(tdiStateTemplate, value.c_str());
	}
	return out;
}

std::string host_fill_cc_mode(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		nlohmann::ordered_json cc_mode_json = value.c_str();
		out = cc_mode_json.dump(4);
	}
	else if (arg.output == csv) {
		out = string_format(",%s", value.c_str());
	}
	else {
		out = string_format(ccModeTemplate, value.c_str());
	}
	return out;
}

int AmdSmiApiHost::amdsmi_get_vf_tdi_state_command(std::string vf_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_vf_handle_t vf_handle;
	amdsmi_tdi_state_t tdi_state;
	std::string tdi_state_str;

	tmp_bdf.bdf.domain_number = std::stoi(vf_bdf.substr(0, 4), nullptr, 16);
	tmp_bdf.bdf.bus_number = std::stoi(vf_bdf.substr(5, 2), nullptr, 16);
	tmp_bdf.bdf.device_number = std::stoi(vf_bdf.substr(8, 2), nullptr, 16);
	tmp_bdf.bdf.function_number = std::stoi(vf_bdf.substr(11), nullptr, 16);

	ret = host_amdsmi_get_vf_handle_from_bdf(tmp_bdf, &vf_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_tdi_state(vf_handle, &tdi_state);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_tdi_state(arg, "N/A");
		return ret;
	}

	get_string_from_enum_tdi_state(tdi_state, tdi_state_str);

	if (arg.output == json) {
		nlohmann::ordered_json tdi_state_json = tdi_state_str.c_str();
		formatted_string = tdi_state_json.dump(4);
	}
	else if (arg.output == csv) {
		formatted_string = string_format(",%s", tdi_state_str.c_str());
	}
	else {
		formatted_string = string_format(tdiStateTemplate, tdi_state_str.c_str());
	}
	return ret;
}

int AmdSmiApiHost::amdsmi_get_cc_mode_command(uint64_t processor_bdf, Arguments arg,
		std::string &formatted_string)
{
	int ret;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	amdsmi_cc_mode_t cc_mode;
	tmp_bdf.as_uint = processor_bdf;
	std::string cc_mode_str;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_cc_mode(processor, &cc_mode);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		formatted_string = host_fill_cc_mode(arg, "N/A");
		return ret;
	}

	get_string_from_enum_cc_mode(cc_mode, cc_mode_str);

	if (arg.output == json) {
		nlohmann::ordered_json cc_mode_json = cc_mode_str.c_str();
		formatted_string = cc_mode_json.dump(4);
	}
	else if (arg.output == csv) {
		formatted_string = string_format(",%s", cc_mode_str.c_str());
	}
	else {
		formatted_string = string_format(ccModeTemplate, cc_mode_str.c_str());
	}

	return ret;
}
