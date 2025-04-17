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
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <map>
#include <fstream>
#include <memory>

// Year should follow the IP driver package version: 22.40/23.10 and similar
#define AMDSMI_TOOL_VERSION_YEAR 24
// Major version should be changed for every command change (adding new commands, deprecating commands, modifying input/output format)
#define AMDSMI_TOOL_VERSION_MAJOR 17
// Minor version should be updated for each command change, but without adding new commands,
// deleting existing commands or modifying input/output format of existing commands
#define AMDSMI_TOOL_VERSION_MINOR 1
// Release version should be set to 0 as default and can be updated by the PMs for each CSP point release
#define AMDSMI_TOOL_VERSION_RELEASE 1

#define AMDSMI_TOOL_VERSION_CREATE_STRING(YEAR, MAJOR, MINOR, RELEASE) (#YEAR "." #MAJOR "." #MINOR "." #RELEASE)
#define AMDSMI_TOOL_VERSION_EXPAND_PARTS(YEAR_STR, MAJOR_STR, MINOR_STR, RELEASE_STR) AMDSMI_TOOL_VERSION_CREATE_STRING(YEAR_STR, MAJOR_STR, MINOR_STR, RELEASE_STR)
#define AMDSMI_TOOL_VERSION_STRING AMDSMI_TOOL_VERSION_EXPAND_PARTS(AMDSMI_TOOL_VERSION_YEAR, AMDSMI_TOOL_VERSION_MAJOR, AMDSMI_TOOL_VERSION_MINOR, AMDSMI_TOOL_VERSION_RELEASE)
#define AMDSMI_TOOL_NAME "AMD SMI tool"

#define AMDSMI_NOT_SUPP_UINT8_RETVAL 255

/**
 * @brief Format string into specified format
 *
 * @param format format in which to format string
 * @param args list of arguments
 * @return std::string formated string
 */
template <typename... Args>
std::string string_format(const std::string &format, Args... args)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
				 1; // Extra space for '\0'
	if (size_s <= 0) {
		throw std::runtime_error("Error during formatting.");
	}
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), args...);
	return std::string(buf.get(),
					   buf.get() + size - 1); // We don't want the '\0' inside
}

/**
 * @brief Converts amdsmi_bdf_t struct into human readable format
 *
 * @param bdf amdsmi_bdf_t structure
 * @return std::string amdsmi_bdf_t in human readable format
 */
std::string convert_bdf_to_string(uint64_t function_number, uint64_t device_number,
								  uint64_t bus_number, uint64_t domain_number);

/**
 * @brief Converts enum name to string
 *
 * @param fw_block amdsmi_fw_block_t structure
 * @return std::string of amdsmi_fw_block_t
 */
std::string get_string_from_enum_fw_block(int fw_block);


std::string get_string_from_enum_ecc_blocks(int ecc_block);

/**
 * @brief Converts enum name to string
 *
 * @param vf_state amdsmi_vf_sched_state_t structure
 * @return std::string of amdsmi_vf_sched_state_t
 */
std::string get_string_from_enum_vf_sched_state(int vf_state);

/**
 * @brief Converts enum name to string
 *
 * @param vf_state amdsmi_guard_state_t structure
 * @return std::string of amdsmi_guard_state_t
 */
std::string get_string_from_enum_vf_guard_state(int vf_state);

/**
 * @brief Converts enum name to string
 *
 * @param guard_type amdsmi_guard_type_t structure
 * @return std::string of amdsmi_guard_type_t
 */
std::string get_string_from_enum_vf_guard_type(int guard_type);

/**
 * @brief Converts enum name to string
 *
 * @param vram_type amdsmi_vram_type_t structure
 * @return std::string of amdsmi_vram_type_t
 */
std::string get_string_from_enum_vram_type(int vram_type);

/**
 * @brief Converts enum name to string
 *
 * @param guard_type amdsmi_vram_vendor_type_t structure
 * @return std::string of amdsmi_vram_vendor_type_t
 */
std::string get_string_from_enum_vram_vendor_type(int vram_vendor_type);

/**
 * @brief Converts enum name to string
 *
 * @param driver_model amdsmi_driver_model_type_t structure
 * @return std::string of amdsmi_driver_model_type_t
 */
std::string get_string_from_enum_driver_model(int driver_model);

/**
 * @brief Converts enum name to string
 *
 * @param severity_mask amdsmi_cper_sev_t structure
 * @return std::string of amdsmi_cper_sev_t
 */
std::string get_string_from_enum_cper_severity_mask(int severity_mask);

/**
 * @brief Converts enum name to string
 *
 * @param partition_type amdsmi_accelerator_partition_type_t structure
 * @return std::string of amdsmi_accelerator_partition_type_t
 */
std::string get_string_from_enum_accelerator_partition_type(int partition_type);

/**
 * @brief Converts enum name to string
 *
 * @param mp_setting amdsmi_memory_partition_type_t structure
 * @return std::string of amdsmi_memory_partition_type_t
 */
std::string get_string_from_enum_mp_setting(int mp_setting);

/**
 * @brief Converts enum name to string
 *
 * @param mp_setting amdsmi_accelerator_partition_resource_type_t structure
 * @return std::string of amdsmi_accelerator_partition_resource_type_t
 */
std::string get_string_from_enum_resource_type(int resource_type);

/**
 * @brief Decodes memory partition settings
 *
 * @param[in] nps_cap_mask a 32-bit integer mask representing NPS setting
 * @return A vector of strings representing the possible NPS configurations
 */
std::vector<std::string> decode_memory_caps(uint32_t nps_cap_mask);

/**
 * @brief Converts a vector of possible memory partition configurations to a human-readable string
 *
 * @param[in] nps_modes a vector of strings representing the possible NPS setting
 * @return A string representing the possible NPS settings in a human-readable format
 */
std::string possible_memory_caps_to_human_readable(const std::vector<std::string>& nps_modes);

std::string transform_fw(int fw_block_id, uint32_t uversion);

const char* getFileName(const char* fullPath);

std::string amdsmi_get_error_message(int error_code);

std::string decimal_string_to_hex(const std::string& decimalStr);

std::vector<std::map<std::string, std::string>> get_vf_tree();

std::string convert_slot_type_to_string(uint32_t pcie_slot_type);

std::string format_link_type(const int& link_type);

std::string format_link_status(const int& link_status);

std::tuple<std::string, std::string, std::string> getGpuVfIndexFromVfId(std::string vf_id);

std::vector<std::string> transform_ecc_correction_schema(uint32_t flag);

std::vector<std::string> transform_cache_properties(uint32_t initial_property);

int csv_recursion(std::string& main_buffer, const std::vector<std::vector<std::string>> &results);
int csv_recursive_function(std::string &main_buff, std::vector<std::string> prefix, int i,
						   const std::vector<std::vector<std::string>> &results, std::map<int, int> order_map);

std::vector<std::string> split_string(const std::string &s, char delim);

/**
 * @brief Converts all chars to be lower case
 *
 * @param str char* to convert
 */
void to_lower_case(char* str);

/**
 * @brief Used for table alignment
 *
 * @param table
 */
void align_table(std::vector<std::vector<std::string>> &table);

/**
 * @brief Return the number of lines that row need to be splited
 *
 * @param row vector of std::string elements
 * @param max_lenght max lenght of element in row
 * @return int number of lines
 */
int num_of_lines(std::vector<std::string> row, int max_lenght);

/**
 * @brief Split given string into multiple by size
 *
 * @param str string string into vector of strings size line_size
 * @param line_size size fo new strings
 * @return std::vector<std::string> vector of strings size line_size
 */
std::vector<std::string> split_string_by_size(const std::string &str, int line_size);

int convert_bytes_to_megabytes(uint64_t bytes);

std::vector<std::string> splitString(const std::string& s, const std::string& delimiter,
									 bool skipEmptyParts);

/**
 * @brief Write string to given file
 *
 * @param file_name Name of file to which to write
 * @param string String which to write
 * @param enable_append optional param that enable append to file
 */
void write_to_file(std::string file_name, std::string string, bool enable_append = false);

/**
	 * @brief Check if a string is in BDF format
	 *
	 * @param[in] s String for check
	 * @return true if string is in BDF format, else false
	 */
bool is_BDF(std::string s);
/**
	 * @brief Check if a string is in UUID format
	 *
	 * @param[in] s String for check
	 * @return true if string is in UUID format, else false
	 */
bool is_UUID(std::string s);
