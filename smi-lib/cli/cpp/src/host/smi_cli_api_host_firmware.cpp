/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdsmi.h"
#include "smi_cli_api_host.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"

#include "json/json.h"

#include <algorithm>
#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_FW_INFO)(amdsmi_processor_handle, amdsmi_fw_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_FW_ERROR_RECORDS)(amdsmi_processor_handle,
		amdsmi_fw_error_record_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_vf_handle_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_FW_INFO)(amdsmi_vf_handle_t, amdsmi_fw_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_FW_INFO)(amdsmi_processor_handle,
		amdsmi_nic_fw_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_FW_INFO host_amdsmi_get_fw_info;
extern AMDSMI_GET_FW_ERROR_RECORDS host_amdsmi_get_fw_error_records;
extern AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
extern AMDSMI_GET_VF_FW_INFO host_amdsmi_get_vf_fw_info;
extern AMDSMI_GET_NIC_FW_INFO host_amdsmi_get_nic_fw_info;

std::string host_fill_fw_list(Arguments arg, std::string value)
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

std::string host_fill_fw_error_records(Arguments arg, std::string value)
{
	std::string out{};

	if (arg.output == json) {
		auto err_records_list_json = nlohmann::ordered_json::array();
		err_records_list_json.push_back(nlohmann::ordered_json::object( {
			{ "timestamp", value.c_str() },
			{ "vf_idx", value.c_str() },
			{ "fw_id", value.c_str() },
			{ "status", value.c_str() } }));
		out = err_records_list_json.dump(4);
	} else if (arg.output == csv) {
		out += string_format(
				   "%d,%d,%d,%d", value.c_str(), value.c_str(),
				   value.c_str(), value.c_str());
	} else {
		out += string_format(
				   fwErrorRecordsTemplate, value.c_str(), value.c_str(),
				   value.c_str(), value.c_str());
	}

	return out;
}

std::string host_fill_vf_fw_list(Arguments arg, std::string value)
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
				   "%s,%s,%s,%s\n", value.c_str(), value.c_str(), value.c_str(),
				   value.c_str());
	} else {
		out += string_format(
				   fwTemplate, value.c_str(), value.c_str(),
				   value.c_str());
	}

	return out;
}

int AmdSmiApiHost::amdsmi_firmware_fw_list_command(uint64_t processor_bdf, Arguments arg,
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

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	auto fw_list_json = nlohmann::ordered_json::array();

	if (arg.output == human) {
		out += fwListTemplate;
	}

	ret = host_amdsmi_get_fw_info(processor, &fw_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		host_fill_fw_list(arg, "N/A");
		return ret;
	}
	bool is_error_records = (std::find(arg.options.begin(), arg.options.end(), "error-records") !=
							 arg.options.end()) ||
							(std::find(arg.options.begin(), arg.options.end(), "e") !=
							 arg.options.end()) ||
							arg.all_arguments;
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
				if (is_error_records) {
					out += string_format(
							   "%s,%s,%s,,,\n", (*gpu_id).c_str(), fw_name_str.c_str(),
							   fw_version_str.c_str());
				} else {
					out += string_format(
							   "%s,%s,%s\n", (*gpu_id).c_str(), fw_name_str.c_str(),
							   fw_version_str.c_str());
				}
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

int AmdSmiApiHost::amdsmi_firmware_err_rec_command(uint64_t processor_bdf, Arguments arg,
		std::string &out_string)
{
	std::string out{};
	amdsmi_status_t ret;
	auto fw_list_json = nlohmann::ordered_json::array();
	auto err_records_list_json = nlohmann::ordered_json::array();
	std::map<uint32_t,std::string> values{};
	std::map<uint32_t,std::string> moutput{};
	std::vector<std::tuple<uint64_t,uint64_t,uint32_t,uint16_t>> error_records{};
	amdsmi_fw_error_record_t err_records;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_get_fw_error_records(processor,
										   &err_records);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		if (ret == AMDSMI_STATUS_NOT_SUPPORTED) {
			out_string += host_fill_fw_error_records(arg, "N/A");
		}
		return ret;
	}

	if (arg.output == human) {
		out += fwErrorRecordListTemplate;
	}

	uint8_t num_err_records = err_records.num_err_records;
	if (num_err_records > 0) {
		for (uint8_t err_rec_iterator = 0; err_rec_iterator < num_err_records;
				err_rec_iterator++) {
			auto timestamp =
				err_records.err_records[err_rec_iterator].timestamp;
			auto vf_idx = err_records.err_records[err_rec_iterator].vf_idx;
			auto fw_id = err_records.err_records[err_rec_iterator].fw_id;
			auto status = err_records.err_records[err_rec_iterator].status;

			if (arg.output == json) {
				err_records_list_json.push_back(nlohmann::ordered_json::object( {
					{ "timestamp", timestamp },
					{ "vf_idx", vf_idx },
					{ "fw_id", fw_id },
					{ "status", status } }));
			} else if (arg.output == csv) {
				out += string_format(
						   "%d,%d,%d,%d\n", timestamp, vf_idx,
						   fw_id, status);
			} else {
				out += string_format(
						   fwErrorRecordsTemplate, timestamp, vf_idx,
						   fw_id, status);
			}
		}
	}
	if (arg.output == json) {
		nlohmann::ordered_json fw_values;
		fw_values = { { "error_records", err_records_list_json } };
		out += fw_values.dump(4);
	}
	out_string += out;

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_firmware_vf_fw_list_command(std::string device, Arguments arg,
		std::string *gpu_id, std::string *vf_id, std::string &out)
{
	std::string out_put{};
	amdsmi_fw_info_t fw_info;
	std::string fw_name_str;
	std::string fw_version_str;
	amdsmi_status_t ret;
	auto fw_list_json = nlohmann::ordered_json::array();
	nlohmann::ordered_json fw_json;
	amdsmi_bdf_t vf_bdf;
	amdsmi_vf_handle_t vf_handle;

	vf_bdf.bdf.domain_number = std::stoi(device.substr(0, 4), nullptr, 16);
	vf_bdf.bdf.bus_number = std::stoi(device.substr(5, 2), nullptr, 16);
	vf_bdf.bdf.device_number = std::stoi(device.substr(8, 2), nullptr, 16);
	vf_bdf.bdf.function_number = std::stoi(device.substr(11), nullptr, 16);

	host_amdsmi_get_vf_handle_from_bdf(vf_bdf, &vf_handle);

	if (arg.output == human) {
		out_put += fwListVfTemplate;
	}

	ret = host_amdsmi_get_vf_fw_info(vf_handle, &fw_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		if (arg.output == json) {
			fw_json["gpu"] = *gpu_id;
			fw_json["vf"] = *vf_id;
			fw_json["fw_list"] = fw_list_json;
			out += fw_json.dump(4);
		}
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
				out_put += string_format(
							   "%s,%s,%s,%s\n", (*gpu_id).c_str(), (*vf_id).c_str(), fw_name_str.c_str(),
							   fw_version_str.c_str());
			} else {
				out_put += string_format(
							   fwVfTemplate, fw_iterator, fw_name_str.c_str(),
							   fw_version_str.c_str());
			}
		}
	}

	if (arg.output == json) {
		fw_json["gpu"] = *gpu_id;
		fw_json["vf"] = *vf_id;
		fw_json["fw_list"] = fw_list_json;
		out += fw_json.dump(4);
	} else {
		out += out_put;
	}

	return AMDSMI_STATUS_SUCCESS;
}

int AmdSmiApiHost::amdsmi_get_nic_fw_info_command(uint64_t processor_bdf, Arguments arg,
		std::string& out)
{
	int ret;
	amdsmi_nic_fw_info_t nic_fw_info;
	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	ret = host_amdsmi_get_nic_fw_info(processor, &nic_fw_info);
	if (ret != AMDSMI_STATUS_SUCCESS || nic_fw_info.num_fw == 0) {
		if (ret == AMDSMI_STATUS_DRIVER_NOT_LOADED && arg.options.size() <= 1 && !arg.all_arguments) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			return ret;
		}
		out = "";
		return ret;
	}

	std::string fw_type_str;

	if (arg.output == json) {
		auto fw_list_json = nlohmann::ordered_json::array();

		for (uint32_t i = 0; i < nic_fw_info.num_fw; ++i) {
			get_string_from_enum_nic_fw_type(nic_fw_info.fw[i].type, fw_type_str);
			std::transform(fw_type_str.begin(), fw_type_str.end(), fw_type_str.begin(), ::tolower);
			fw_list_json.push_back(nlohmann::ordered_json::object({
				{ "type", fw_type_str.c_str() },
				{ "name", nic_fw_info.fw[i].fw.name },
				{ "version", nic_fw_info.fw[i].fw.version }
			}));
		}

		out = fw_list_json.dump(4);
	} else if (arg.output == human) {
		std::string formatted_output{fwNicListTemplate};

		for (uint32_t i = 0; i < nic_fw_info.num_fw; ++i) {
			get_string_from_enum_nic_fw_type(nic_fw_info.fw[i].type, fw_type_str);
			std::string fw_name_str{nic_fw_info.fw[i].fw.name};
			std::transform(fw_name_str.begin(), fw_name_str.end(), fw_name_str.begin(), ::toupper);
			formatted_output += string_format(fwNicTemplate, i,
				fw_type_str.c_str(),
				fw_name_str.c_str(),
				nic_fw_info.fw[i].fw.version);
		}

		out = formatted_output;
	}

	return ret;
}
