/* * Copyright (C) 2025 Advanced Micro Devices. All rights reserved.
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
#include "smi_cli_api_host.h"
#include "amdsmi.h"
#include "json/json.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

#include <sys/stat.h>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <string>
#include <climits>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0777)
#endif

#define CPER_DATA_BUFFER_SIZE 4096
#define CPER_RAW_DATA_BUFFER_SIZE (1024 * 1024)
#define CPER_HDRS_ARRAY_SIZE 1024

struct CperEntryInfo {
	std::string timestamp;
	amdsmi_cper_timestamp_t cper_timestamp;
	int gpu_id;
	int error_severity;
	amdsmi_cper_guid_t notify_type;
	std::string severity_string;
	char cper_data[CPER_DATA_BUFFER_SIZE];
	uint32_t record_length;

	CperEntryInfo(std::string ts, amdsmi_cper_timestamp_t cts, int gpu, int sev, amdsmi_cper_guid_t nt,std::string sev_str, char* cd, uint32_t rl)
		: timestamp(ts), cper_timestamp(cts), gpu_id(gpu), error_severity(sev), notify_type(nt),
		  severity_string(sev_str), record_length(rl) {
			std::memcpy(cper_data, cd, std::min(sizeof(cper_data), (size_t)rl));
		}

	bool operator<(const CperEntryInfo& other) const {
		if (cper_timestamp.year != other.cper_timestamp.year) return cper_timestamp.year < other.cper_timestamp.year;
		if (cper_timestamp.month != other.cper_timestamp.month) return cper_timestamp.month < other.cper_timestamp.month;
		if (cper_timestamp.day != other.cper_timestamp.day) return cper_timestamp.day < other.cper_timestamp.day;
		if (cper_timestamp.hours != other.cper_timestamp.hours) return cper_timestamp.hours < other.cper_timestamp.hours;
		if (cper_timestamp.minutes != other.cper_timestamp.minutes) return cper_timestamp.minutes < other.cper_timestamp.minutes;
		return cper_timestamp.seconds < other.cper_timestamp.seconds;
	}
};

namespace fs = std::filesystem;

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_CPER_ENTRIES)(amdsmi_processor_handle, uint32_t, char*,
		uint64_t *,
		amdsmi_cper_hdr_t**, uint64_t *, uint64_t *);

typedef amdsmi_status_t (*AMDSMI_GET_AFIDS_FROM_CPER)(char*cper_buffer, uint32_t buf_size, uint64_t *afids, uint32_t *num_afids);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_POLICY_INFO)(amdsmi_processor_handle,
		amdsmi_gpu_ras_policy_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_GPU_CPER_ENTRIES host_amdsmi_get_gpu_cper_entries;
extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_AFIDS_FROM_CPER host_amdsmi_get_afids_from_cper;
extern AMDSMI_GET_GPU_RAS_POLICY_INFO host_amdsmi_get_gpu_ras_policy_info;

#define GUID_INIT(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)                 \
{ { (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff, \
   (b) & 0xff, ((b) >> 8) & 0xff,                                          \
   (c) & 0xff, ((c) >> 8) & 0xff,                                          \
   (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) } };

#define CPER_NOTIFY_MCE                                               \
	GUID_INIT(0xE8F56FFE, 0x919C, 0x4cc5, 0xBA, 0x88, 0x65, 0xAB, \
		  0xE1, 0x49, 0x13, 0xBB)
#define CPER_NOTIFY_CMC                                               \
	GUID_INIT(0x2DCE8BB1, 0xBDD7, 0x450e, 0xB9, 0xAD, 0x9C, 0xF4, \
		  0xEB, 0xD4, 0xF8, 0x90)
#define BOOT_TYPE                                                     \
	GUID_INIT(0x3D61A466, 0xAB40, 0x409a, 0xA6, 0x98, 0xF3, 0x62, \
		  0xD4, 0x64, 0xB3, 0x8F)

amdsmi_cper_guid_t boot_guid = BOOT_TYPE;
amdsmi_cper_guid_t mce_guid = CPER_NOTIFY_MCE;

std::string generate_file_name(int error_severity, const amdsmi_cper_guid_t notify_type,
							   int error_count = 1)
{
	std::string prefix;

	switch (error_severity) {
	case AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED:
		prefix = "uncorrected-";
		break;
	case AMDSMI_CPER_SEV_NON_FATAL_CORRECTED:
		prefix = "corrected-";
		break;
	case AMDSMI_CPER_SEV_FATAL:
		if (std::memcmp(&notify_type, &boot_guid, 16) == 0) {
			prefix = "boot-";
		}
		if (std::memcmp(&notify_type, &mce_guid, 16) == 0) {
			prefix = "fatal-";
		}
		break;
	default:
		prefix = "unknown-";
		break;
	}

	std::string file_name = prefix + std::to_string(error_count) + ".cper";
	return file_name;
}

std::string print_cper_timestamp(amdsmi_cper_timestamp_t *timestamp)
{
	if (timestamp == NULL) {
		printf("Invalid timestamp\n");
		return "";
	}

	std::string formatted_string = string_format(" %02d/%02d/20%02d  %02d:%02d:%02d",
								   timestamp->day, timestamp->month, timestamp->year, timestamp->hours, timestamp->minutes,
								   timestamp->seconds);

	return formatted_string;
}

bool create_directory(const std::string& path)
{
	if (MKDIR(path.c_str()) == -1) {
#ifdef _WIN32
		if (errno == EEXIST) {
#else
		struct stat info;
		if (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) {
#endif
			// Directory already exists
			return true;
		} else {
			std::cerr << "Error creating directory: " << path << std::endl;
			return false;
		}
	}
	return true;
}

uint32_t convert_to_severity_mask(std::vector<std::string>& severities)
{
	uint32_t severity_mask = 0;

	for (auto& severity : severities) {
		if (severity == "nonfatal-uncorrected") {
			severity_mask |= (1 << AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED);
		} else if (severity == "fatal") {
			severity_mask |= (1 << AMDSMI_CPER_SEV_FATAL);
		} else if (severity == "nonfatal-corrected") {
			severity_mask |= (1 << AMDSMI_CPER_SEV_NON_FATAL_CORRECTED);
		} else if (severity == "all") {
			severity_mask = (1 << AMDSMI_CPER_SEV_NUM);
			break;
		} else {
			severity_mask |= (1 << AMDSMI_CPER_SEV_UNUSED);
		}
	}
	return severity_mask;
}


int extract_number_from_filename(const std::string& filename)
{
	std::regex re(R"([-_]([0-9]+)\.cper)");
	std::smatch match;

	if (std::regex_search(filename, match, re) && match.size() > 1) {
		return std::stoi(match.str(1)); // Extract and convert the numeric part
	}
	return std::numeric_limits<int>::max(); // Max value if pattern doesn't match
}

void count_and_replace_oldest_files(const std::string& folder_path,
									const std::unordered_map<std::string, std::string>& file_timestamp_map, int file_limit)
{
	std::vector<std::pair<fs::path, std::string>> files;
	int file_count = 0;

	// Collect all files and their corresponding timestamps
	for (const auto& entry : fs::directory_iterator(folder_path)) {
		if (fs::is_regular_file(entry.status())) {
			std::string filename = entry.path().filename().string();
			auto it = file_timestamp_map.find(filename);
			if (it != file_timestamp_map.end()) {
				files.emplace_back(entry.path(), it->second);
				++file_count;
			}
		}
	}

	// If the number of files exceeds the limit, delete the oldest files based on timestamp
	if (file_count > file_limit) {
		std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
			return a.second < b.second;
		});

		while (file_count > file_limit) {
			fs::remove(files.front().first);
			files.erase(files.begin());
			--file_count;
		}
	}
}

int process_cper_entries(amdsmi_processor_handle processor, uint32_t severity_mask, uint64_t* cursor, const std::string& folder_name,
						 int gpu_id, std::vector<CperEntryInfo>& entries_info)
{
	int ret;
	std::string file_name;
	std::string cper_timestamp;
	std::string severity_mask_string;
	static int total_cper_count = 0;

	char cper_data[CPER_RAW_DATA_BUFFER_SIZE];   // the buffer to hold the raw cper data
	amdsmi_cper_hdr_t* cper_hdrs[CPER_HDRS_ARRAY_SIZE];  // the buffer to hold the parsed cper headers
	uint64_t buf_size = sizeof(cper_data);
	uint64_t entry_count = CPER_HDRS_ARRAY_SIZE; //sizeof(cper_hdrs) / sizeof(cper_hdrs[0]);

	do {
		ret = host_amdsmi_get_gpu_cper_entries(processor, severity_mask, cper_data, &buf_size, cper_hdrs,
											   &entry_count, cursor);
		if (ret != AMDSMI_STATUS_SUCCESS && ret != AMDSMI_STATUS_MORE_DATA) {
			return ret;
		}

		amdsmi_cper_hdr_t* local_cper_hdrs[CPER_HDRS_ARRAY_SIZE];
		size_t offset = 0;
		for (uint32_t j = 0; j < entry_count; j++) {
		    local_cper_hdrs[j] = reinterpret_cast<amdsmi_cper_hdr_t*>(cper_data + offset);
		    offset += cper_hdrs[j]->record_length;
		}

		for (uint32_t i = 0; i < entry_count; i++) {
			std::string cper_timestamp = print_cper_timestamp(&local_cper_hdrs[i]->timestamp);
			int error_severity = local_cper_hdrs[i]->error_severity;
			amdsmi_cper_guid_t notify_type = local_cper_hdrs[i]->notify_type;
			std::string severity_mask_string = get_string_from_enum_cper_severity_mask(error_severity);

			char byte_array[CPER_DATA_BUFFER_SIZE];
			std::memcpy(byte_array, local_cper_hdrs[i], local_cper_hdrs[i]->record_length);
			amdsmi_cper_hdr_t local_hdr = *local_cper_hdrs[i];

			entries_info.emplace_back(cper_timestamp, local_hdr.timestamp, gpu_id, local_hdr.error_severity,
						  local_hdr.notify_type, severity_mask_string, byte_array, local_hdr.record_length);

		}

	} while (ret == AMDSMI_STATUS_MORE_DATA);

	return ret;
}

int AmdSmiApiHost::amdsmi_get_cper_entries_command(Arguments arg, std::string& out)
{
	int ret;
	amdsmi_socket_handle socket = NULL;
	unsigned int gpu_count;
	int total_cper_count = 0;

	uint32_t severity_mask = AMDSMI_CPER_SEV_NUM;
	uint64_t cursor[AMDSMI_MAX_DEVICES] {0}; // The cursor to get more data

	std::string folder_name {""};
	std::vector<CperEntryInfo> all_entries_info;
	std::unordered_map<std::string, std::string> file_timestamp_map;

	uint64_t afids[MAX_NUMBER_OF_AFIDS_PER_RECORD];
	uint32_t num_afids = 0;
	std::string out_afids = {};

	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(
			amdsmi_processor_handle) * gpu_count);
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}
	severity_mask = convert_to_severity_mask(arg.severities);
	if (arg.folder_name != "") {
		folder_name = arg.folder_name;
		create_directory(folder_name);
	} else {
		printf("WARNING: No cper files will be dumped unless the --folder=<folder_name> is specified\n\n");
	}

	if (std::find(arg.options.begin(), arg.options.end(), "follow") != arg.options.end()) {
		printf("Press CTRL + C when you want to stop\n\n");
		printf("%-24s %-8s %-24s %-24s %s\n","timestamp", "gpu_id", "severity", "file_name", "list of afids");
		while (true) {
			// Iterate through specified devices or all GPUs if none specified
			for (uint32_t i = 0; i < arg.devices.size(); i++) {
				uint32_t gpu_id = arg.devices[i]->get_gpu_index();
				ret  = process_cper_entries(processors[gpu_id], severity_mask, &cursor[gpu_id], folder_name, gpu_id, all_entries_info);
			}

			std::sort(all_entries_info.begin(), all_entries_info.end(), [](const CperEntryInfo& a,
			const CperEntryInfo& b) {
				return a < b;
			});

			for (auto& entry : all_entries_info) {
				++total_cper_count;
				std::string file_name = generate_file_name(entry.error_severity, entry.notify_type,
										total_cper_count);

				ret = host_amdsmi_get_afids_from_cper(entry.cper_data, entry.record_length, afids, &num_afids);
				if (ret != AMDSMI_STATUS_SUCCESS) {
					return ret;
				}

				for(uint32_t i = 0; i < num_afids; i++) {
					out_afids += std::to_string(afids[i]) + " ";
				}

				out = string_format(RasCperTemplate, entry.timestamp.c_str(), entry.gpu_id,
					   entry.severity_string.c_str(), file_name.c_str(), out_afids.c_str());
				printf("%s", out.c_str());
				out_afids.clear();

				std::string path = folder_name + "/" + file_name;
				std::ofstream outFile(path, std::ios::binary);
				outFile.write(entry.cper_data, entry.record_length);

				outFile.close();

				file_timestamp_map[file_name] = entry.timestamp;
			}

			if (std::find(arg.options.begin(), arg.options.end(), "file-limit") != arg.options.end()) {
				if (!folder_name.empty()) {
					count_and_replace_oldest_files(folder_name, file_timestamp_map, arg.file_limit);
				}
			}
			all_entries_info.clear();
			// Sleep for a while before the next iteration
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

	} else {
		printf("%-24s %-8s %-24s %-24s %s\n","timestamp", "gpu_id", "severity", "file_name", "list of afids");
		// Iterate through specified devices or all GPUs if none specified
		for (uint32_t i = 0; i < arg.devices.size(); i++) {
			uint32_t gpu_id = arg.devices[i]->get_gpu_index();
			ret  = process_cper_entries(processors[gpu_id], severity_mask, &cursor[gpu_id], folder_name, gpu_id, all_entries_info);
		}

		std::sort(all_entries_info.begin(), all_entries_info.end(), [](const CperEntryInfo& a,
		const CperEntryInfo& b) {
			return a < b;
		});

		for (auto& entry : all_entries_info) {
			++total_cper_count;
			std::string file_name = generate_file_name(entry.error_severity, entry.notify_type,
									total_cper_count);

			ret = host_amdsmi_get_afids_from_cper(entry.cper_data, entry.record_length, afids, &num_afids);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				return ret;
			}

			for (uint32_t i = 0; i < num_afids; i++) {
				out_afids += std::to_string(afids[i]) + " ";
			}

			out = string_format(RasCperTemplate, entry.timestamp.c_str(), entry.gpu_id,
					entry.severity_string.c_str(), file_name.c_str(), out_afids.c_str());
			printf("%s", out.c_str());
			out_afids.clear();

			std::string path = folder_name + "/" + file_name;
			std::ofstream outFile(path, std::ios::binary);
			outFile.write(entry.cper_data, entry.record_length);
			outFile.close();

			file_timestamp_map[file_name] = entry.timestamp;
		}

		if (std::find(arg.options.begin(), arg.options.end(), "file-limit") != arg.options.end()) {
			if (!folder_name.empty()) {
				count_and_replace_oldest_files(folder_name, file_timestamp_map, arg.file_limit);
			}
		}
	}

	free(processors);
	out = "\n";
	return 0;
}

int AmdSmiApiHost::amdsmi_get_cper_afid_command(Arguments arg, std::string& out)
{
	int ret = 0;
	uint64_t afids[MAX_NUMBER_OF_AFIDS_PER_RECORD];
	uint32_t num_afids = MAX_NUMBER_OF_AFIDS_PER_RECORD;

	std::ifstream file(arg.cper_file_path, std::ios::binary | std::ios::ate);
	if (!file) {
		throw SmiToolInvalidFilePathException(arg.cper_file_path);
	}

	std::streamsize fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(fileSize);

	if (fileSize <= 0 || fileSize == LLONG_MAX) {
		throw SmiToolInvalidFilePathException(arg.cper_file_path);
	}

	if (!file.read(buffer.data(), fileSize)) {
		throw SmiToolInvalidFilePathException(arg.cper_file_path);
	}

	ret = host_amdsmi_get_afids_from_cper(buffer.data(), buffer.size(), afids, &num_afids);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	for(uint32_t i = 0; i < num_afids; i++) {
		out += std::to_string(afids[i]) + " ";
	}

	out += "\n";

	return ret;
}

int AmdSmiApiHost::amdsmi_get_policy_command(uint64_t processor_bdf, Arguments arg, std::string& out_string)
{
	int ret = 0;
	amdsmi_gpu_ras_policy_info_t ras_policy_info;
	nlohmann::ordered_json json_out;
	uint8_t major_version = 0;
	uint8_t minor_version = 0;
	uint16_t dram_non_critical_region_threshold = 0;
	uint16_t dram_critical_region_threshold = 0;
	std::string major_version_str{};
	std::string minor_version_str{};
	std::string dram_non_critical_region_threshold_str{};
	std::string dram_critical_region_threshold_str{};

	amdsmi_processor_handle processor;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = processor_bdf;
	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}
	ret = host_amdsmi_get_gpu_ras_policy_info(processor, &ras_policy_info);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}
	major_version = ras_policy_info.major_version;
	minor_version = ras_policy_info.minor_version;

	if(ras_policy_info.major_version == 4 && ras_policy_info.minor_version == 0) {
		dram_non_critical_region_threshold = ras_policy_info.policy_data.v4_0.dram_non_critical_region_threshold;
		dram_critical_region_threshold = ras_policy_info.policy_data.v4_0.dram_critical_region_threshold;
		major_version_str = string_format("%u", major_version);
		minor_version_str = string_format("%u", minor_version);
		dram_non_critical_region_threshold_str = string_format("%u", dram_non_critical_region_threshold);
		dram_critical_region_threshold_str = string_format("%u", dram_critical_region_threshold);
	}

	if (arg.output == json) {
		json_out = {
			{"major_version", major_version},
			{"minor_version", minor_version},
			{"dram_non_critical_region_threshold", dram_non_critical_region_threshold},
			{"dram_critical_region_threshold", dram_critical_region_threshold}
		};
		out_string = json_out.dump(4);
	} else if (arg.output == csv) {
		out_string = string_format(",%s,%s,%s,%s", major_version_str.c_str(),
						     minor_version_str.c_str(),
						     dram_non_critical_region_threshold_str.c_str(),
						     dram_critical_region_threshold_str.c_str());
	} else {
		out_string = string_format(rasPolicyTemplate, major_version_str.c_str(),
							minor_version_str.c_str(),
							dram_non_critical_region_threshold_str.c_str(),
							dram_critical_region_threshold_str.c_str());
	}

	return AMDSMI_STATUS_SUCCESS;
}

