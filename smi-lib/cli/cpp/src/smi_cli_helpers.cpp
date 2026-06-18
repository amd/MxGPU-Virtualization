/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <cstring>

#include "smi_cli_exception.h"
#include "smi_cli_helpers.h"
#include "smi_cli_api_base.h"
#include "smi_cli_platform.h"

std::string convert_bdf_to_string(uint64_t function_number, uint64_t device_number,
								  uint64_t bus_number, uint64_t domain_number)
{
	return string_format("%04x:%02x:%02x.%01x", domain_number, bus_number,
						 device_number, function_number);
}

std::string get_string_from_enum_vf_sched_state(int vf_state)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_vf_sched_state(vf_state, out);
	return out;
}

std::string get_string_from_enum_vf_guard_state(int vf_state)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_vf_guard_state(vf_state, out);
	return out;
}

std::string get_string_from_enum_vf_guard_type(int guard_type)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_vf_guard_type(guard_type, out);
	return out;
}

std::string get_string_from_enum_accelerator_partition_type(int partition_type)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_accelerator_partition_type(
		partition_type, out);
	return out;
}

std::string get_string_from_enum_mp_setting(int mp_setting)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_mp_setting(mp_setting, out);
	return out;
}

std::string get_string_from_enum_resource_type(int resource_type)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_resource_type(resource_type, out);
	return out;
}

std::string get_string_from_enum_vram_type(int vram_type)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_vram_type(vram_type, out);
	return out;
}

std::string get_string_from_enum_driver_model(int driver_model)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_driver_model(driver_model,
			out);
	return out;
}

std::string get_string_from_enum_cper_severity_mask(int severity_mask)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_cper_severity_mask(severity_mask,
			out);
	return out;
}

std::string get_string_from_enum_fw_block(int fw_block)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_fw_block(fw_block, out);
	return out;
}

std::string get_string_from_enum_ecc_blocks(int ecc_block)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_ecc_blocks(ecc_block, out);
	return out;
}

std::string get_string_from_enum_tdi_state(int tdi_state)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_tdi_state(tdi_state, out);
	return out;
}

std::string get_string_from_enum_cc_mode(int cc_mode)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_cc_mode(cc_mode, out);
	return out;
}

std::string transform_fw(int fw_block_id, uint32_t uversion)
{
	std::string uversion_str;
	switch(fw_block_id) {
	case 1: //"SMU"
		if (AmdSmiPlatform::getInstance().is_mi300() || AmdSmiPlatform::getInstance().is_mi200() || AmdSmiPlatform::getInstance().is_mi350()) {
			uversion_str += string_format("%d.%d.%d.%d",(uversion >> 24) & 0xFF,
										  (uversion >> 16) & 0xFF,
										  (uversion >> 8) & 0xFF,
										  uversion & 0xFF);
		} else {
			uversion_str = (uversion & 0xFF) > 0 ? string_format("%d.", uversion & 0xFF) : "";
			uversion_str += string_format("%d.%d.%d", (uversion >> 8) & 0xFF,
										  (uversion >> 16) & 0xFF,
										  (uversion >> 24) & 0xFF);
		}
		break;
	case 28: //"MMSCH"
		uversion_str = string_format("%d.%d.%d", (uversion >> 24) & 0xFF,
									 (uversion >> 16) & 0xFF,
									 uversion & 0xFFFF);
		break;
	case 19: //"UVD"
		uversion_str = string_format("%d.%d.%d.%d", (uversion >> 30) & 0x3,
									 (uversion >> 24) & 0x3F,
									 (uversion >> 8) & 0xFF,
									 (uversion & 0xFF));
		break;
	case 20: // "VCE"
		uversion_str = string_format("%d.%d.%d", (uversion >> 20) & 0xFFF,
									 (uversion >> 8) & 0xFFF,
									 uversion & 0xFF);
		break;
	case 29: //"PSP_SYSDRV"
	case 30: //"PSP_SOSDRV"
	case 32: //"PSP_KEYDB"
	case 33: //"DFC"
	case 34: //"PSP_SPL"
	case 37: //"PSP_BL"
	case 41: //"REG_ACCESS_WHITELIST"
	case 65: //"PSP_SOC"
	case 66: //"PSP_DBG"
	case 67: //"PSP_INTF"
	case 73: //"TA_RAS"
	case 81: //"PSP_RAS"
		uversion_str = string_format("%x.%x.%x.%x", (uversion >> 24) & 0xFF,
									 (uversion >> 16) & 0xFF,
									 (uversion >> 8) & 0xFF,
									 uversion & 0xFF);
		break;
	case 18: //"VCN"
		uversion_str = string_format("%d.%d.%d.%d", (uversion >> 24) & 0xF,
									 (uversion >> 20) & 0xF,
									 (uversion >> 12) & 0xFF,
									 uversion & 0xFFF);
		break;
	case 42: //"IMU_DRAM"
	case 43: //"IMU_IRAM"
	case 83: //"PLDM_VERSION"
		uversion_str = string_format("%d.%d.%d.%d", (uversion >> 24) & 0xFF,
									 (uversion >> 16) & 0xFF,
									 (uversion >> 8) & 0xFF,
									 uversion & 0xFF);
		break;
	case 24: //"RLC_RESTORE_LIST_GPM_MEM"
	case 25: //"RLC_RESTORE_LIST_SRM_MEM"
	case 26: //"RLC_RESTORE_LIST_CNTL"
		uversion_str = string_format("0x%x", uversion);
		break;
	case 46: //"CP_MES"
	case 48: //"MES_STACK"
	case 49: //"MES_THREAD1"
	case 50: //"MES_THREAD1_STACK"
		uversion_str = string_format("0x%08x", uversion);
		break;
	default:
		uversion_str = string_format("0x%x", uversion);
	}
	return uversion_str;
}

const char* getFileName(const char* fullPath)
{
	const char* lastSlash = strrchr(fullPath, '/');
	const char* lastBackslash = strrchr(fullPath, '\\');

	const char* fileNameStart = (lastSlash != nullptr) ? lastSlash + 1 :
								(lastBackslash != nullptr) ? lastBackslash + 1 : fullPath;

	return fileNameStart;
}

std::string amdsmi_get_error_message(int error_code)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_error_message(error_code, out);
	return out;
}

std::string decimal_string_to_hex(const std::string& decimalStr)
{
	uint64_t decimalNum = 0;
	std::stringstream ss;
	try {
		decimalNum = std::stoull(decimalStr);
		ss << "0x" << std::hex << std::uppercase << decimalNum;
	} catch(...) {
		ss << "N/A";
	}
	return ss.str();
}

std::vector<std::map<std::string, std::string>> get_vf_tree()
{
	std::vector<std::map<std::string, std::string>> vfs_map;
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_vf_tree(vfs_map);
	return vfs_map;
}

std::string convert_slot_type_to_string(uint32_t pcie_slot_type)
{
	if(pcie_slot_type == 0) {
		return "PCIE";
	}
	if(pcie_slot_type == 1) {
		return "OAM";
	}
	return "N/A";
}

std::string format_link_type(const int& link_type)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().format_link_type(link_type, out);
	return out;
}

std::string format_link_status(const int& link_status, bool legend)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().format_link_status(link_status, legend, out);
	return out;
}

bool is_ID(std::string s)
{
	std::regex isID("^[0-9a-fA-F]+:[0-9a-fA-F]+$");
	return std::regex_match(s, isID);
}
bool is_BDF(std::string s)
{
	std::regex isBDF("[0-9a-fA-F]{4}[:.][0-9a-fA-F]{2}[:.][0-9a-fA-F]{2}[:.][0-9a-fA-F]$");
	return std::regex_match(s, isBDF);
}

bool is_UUID(std::string s)
{
	std::regex isUUID(
		"[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
	return std::regex_match(s, isUUID);
}

std::tuple<std::string, std::string, std::string>
getGpuVfIndexFromVfId(std::string vf_id)
{
	if (vf_id.empty()) {
		throw SmiToolMissingParameterValueException("--vf");
	}

	if (!(is_BDF(vf_id) || is_UUID(vf_id) || is_ID(vf_id))) {
		throw SmiToolInvalidParameterValueException(vf_id);
	}

	std::tuple<std::string, std::string, std::string> gpu_vf_indexes;
	std::string vf_id_cleaned = vf_id;
	vf_id_cleaned.erase(std::remove_if(vf_id_cleaned.begin(), vf_id_cleaned.end(),
	[](char c) {
		return c == ':' || c == '.';
	}),
	vf_id_cleaned.end());
	std::string vf_id_at_cleaned {};
	bool foundGpuIndex { false };
	std::vector<std::map<std::string, std::string>> vfs = get_vf_tree();
	for (const auto& vf : vfs) {
		vf_id_at_cleaned = vf.at("vf_bdf");
		vf_id_at_cleaned.erase(std::remove_if(vf_id_at_cleaned.begin(), vf_id_at_cleaned.end(),
		[](char c) {
			return c == ':' || c == '.';
		}),
		vf_id_at_cleaned.end());
		if(vf_id == vf.at("vf_bdf") || vf_id == vf.at("vf_uuid") || vf_id == vf.at("vf_id")
				|| vf_id_cleaned == vf_id_at_cleaned) {
			std::get<0>(gpu_vf_indexes) = vf.at("gpu");
			std::get<1>(gpu_vf_indexes) = vf.at("vf");
			std::get<2>(gpu_vf_indexes) = vf.at("vf_bdf");
			foundGpuIndex = true;
		}
	}
	if(!foundGpuIndex) {
		if (is_ID(vf_id)) {
			unsigned int gpu_count;
			int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));
			if (ret != 0) {
				throw SmiToolSMILIBErrorException(ret);
			}
			size_t gpu_index = vf_id.find(":");
			std::string vf_gpu_index = vf_id.substr(0, gpu_index);
			if (std::stoi(vf_gpu_index) >= gpu_count) {
				throw SmiToolDeviceNotFoundException(vf_gpu_index);
			} else {
				std::string vf_index = vf_id;
				vf_index.erase(vf_index.begin(), vf_index.end() - gpu_index);
				throw SmiToolDeviceNotFoundException(vf_index);
			}
		} else {
			throw SmiToolDeviceNotFoundException(vf_id);
		}
	}
	return gpu_vf_indexes;
}

std::vector<std::string> transform_ecc_correction_schema(uint32_t flag)
{
	std::vector<std::string> out;
	AmdSmiApiBase::CreateAmdSmiApiObject().transform_ecc_correction_schema(flag, out);
	return out;
}

std::vector<std::string> transform_cache_properties(uint32_t initial_property)
{
	std::vector<std::string> property;
	AmdSmiApiBase::CreateAmdSmiApiObject().transform_cache_properties(initial_property, property);
	return property;
}
int csv_recursive_function(std::string &main_buff, std::vector<std::string> prefix, int i,
						   const std::vector<std::vector<std::string>> &results, std::map<int, int> order_map)
{
	std::vector<std::string> local_buff{};
	if (i == results.size()) {
		for (auto p : prefix) {
			main_buff += p;
		}
		main_buff += "\n";
		return 0;
	}
	int index = order_map[i];

	if (results[i].size() == 1) {
		prefix[index] += results[i][0];
		csv_recursive_function(main_buff, prefix,(i + 1), results, order_map);
	} else {
		local_buff = prefix;
		for (int j = 0; j < results[i].size(); j++) {
			prefix[index] += results[i][j];
			csv_recursive_function(main_buff, prefix,(i + 1), results, order_map);
			prefix.clear();
			prefix = local_buff;
		}
	}
	return -1;
}
int csv_recursion(std::string& main_buffer, const std::vector<std::vector<std::string>> &results)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().csv_recursion(main_buffer, results);
	return ret;
}

std::vector<std::string> split_string(const std::string &s, char delim)
{
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> elems;
	while (std::getline(ss, item, delim)) {
		elems.push_back(std::move(item));
	}
	return elems;
}

void to_lower_case(char* str)
{
	for (size_t i = 0; i < strlen(str); ++i) {
		str[i] = std::tolower(static_cast<unsigned char>(str[i]));
	}
}

int convert_bytes_to_megabytes(uint64_t bytes)
{
	return int((bytes / 1024) / 1024);
}

void align_table(std::vector<std::vector<std::string>> &table)
{
	size_t num_of_rows{table.size()};
	size_t num_of_columns{table[0].size()};
	std::vector<size_t> fill_sizes{};

	for (int i = 0; i < num_of_columns; i++) {
		size_t max = 0;
		for (int j = 0; j < num_of_rows; j++) {
			if (table[j][i].size() > max) {
				max = table[j][i].size();
			}
		}
		fill_sizes.push_back(max + 2);
	}

	for (int i = 0; i < table.size(); i++) {
		for (int j = 0; j < table[i].size(); j++) {
			while (table[i][j].size() < fill_sizes[j]) {
				table[i][j].append(" ");
			}
		}
	}
}

int num_of_lines(std::vector<std::string> row, int max_lenght)
{
	size_t num_of_lines{};
	size_t current_num_of_lines{};
	for (auto x : row) {
		current_num_of_lines = (x.size() / max_lenght) + 1;
		if (num_of_lines < current_num_of_lines) {
			num_of_lines = current_num_of_lines;
		}
	}
	return static_cast<int>(num_of_lines);
}

std::vector<std::string> split_string_by_size(const std::string &str, int line_size)
{
	std::vector<std::string> result{};
	for (int i = 0; i < str.size(); i += line_size) {
		result.push_back(str.substr(i, line_size));
	}
	return result;
}

void write_to_file(std::string file_name, std::string string, bool enable_append)
{
	if (file_name.size() == 0) {
		throw SmiToolMissingParameterValueException("--file=");
	}
	if (!enable_append) {
		std::ifstream file(file_name.c_str());
		if (file.good()) {
			std::cout << "File alredy exists. Do you want to overwrite current file? Any for yes, 'n' for no:";
			std::string in{};
			std::cin >> in;
			if (in == "n") {
				std::string new_file_name{};
				std::cout << "New file name:";
				std::cin >> new_file_name;
				std::ofstream file{new_file_name.c_str()};
				if (file.is_open()) {
					file << string.c_str();
				} else {
					throw SmiToolInvalidFilePathException(new_file_name);
				}
			} else {
				std::ofstream file{file_name.c_str()};
				if (file.is_open()) {
					file << string.c_str();
				} else {
					throw SmiToolInvalidFilePathException(file_name);
				}
			}
		} else {
			std::ofstream file{file_name.c_str()};
			if (file.is_open()) {
				file << string.c_str() << "\n";
			} else {
				throw SmiToolInvalidFilePathException(file_name);
			}
		}
	} else {
		std::ofstream file{file_name.c_str(), std::ios::app};
		if (file.is_open()) {
			file << string.c_str() << "\n";
		} else {
			throw SmiToolInvalidFilePathException(file_name);
		}
	}
}

std::vector<std::pair<uint64_t, std::string>> bitmaskToRangesList(uint64_t mask, int bitOffset) {
	std::vector<std::pair<uint64_t, std::string>> results;

	if (mask == 0) {
		return results;
	}

	int startRange = -1;
	uint64_t currentMask = 0;

	for (int bit = 0; bit < 64; ++bit) {
		bool bitSet = (mask & (1ULL << bit)) != 0;

		if (bitSet) {
			if (startRange == -1) {
				startRange = bit;
			}
			currentMask |= (1ULL << bit);
		} else {
			if (startRange != -1) {
				int endRange = bit - 1;
				int startWithOffset = startRange + bitOffset;
				int endWithOffset = endRange + bitOffset;

				std::stringstream ss;
				if (startWithOffset == endWithOffset) {
					ss << startWithOffset;
				} else {
					ss << startWithOffset << "-" << endWithOffset;
				}

				results.emplace_back(currentMask, ss.str());

				currentMask = 0;
				startRange = -1;
			}
		}
	}

	if (startRange != -1) {
		int startWithOffset = startRange + bitOffset;
		int endWithOffset = 63 + bitOffset;

		std::stringstream ss;
		if (startWithOffset == endWithOffset) {
			ss << startWithOffset;
		} else {
			ss << startWithOffset << "-" << endWithOffset;
		}

		results.emplace_back(currentMask, ss.str());
	}

	return results;
}

std::string ThrottlerDataToString(uint64_t data) {
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().ThrottlerDataToString(data, out);
	return out;
}

std::string FecModesToString(uint32_t fec) {
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().FecModesToString(fec, out);
	return out;
}

std::string get_string_from_enum_nic_topo_link_type(int nic_link_type)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_nic_topo_link_type(nic_link_type, out);
	return out;
}

std::string get_string_from_enum_nic_fw_type(int nic_fw_type)
{
	std::string out;
	AmdSmiApiBase::CreateAmdSmiApiObject().get_string_from_enum_nic_fw_type(nic_fw_type, out);
	return out;
}

int get_index_from_main_gpu(int &gpu_index)
{
	return AmdSmiApiBase::CreateAmdSmiApiObject().get_index_from_main_gpu(gpu_index);
}