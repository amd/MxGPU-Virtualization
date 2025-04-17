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
#include <dirent.h>
#include <sys/stat.h>

#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0777)
#endif

namespace fs = std::filesystem;

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GPU_GET_CPER_ENTRIES)(amdsmi_processor_handle, uint32_t, char*,
		uint64_t *,
		amdsmi_cper_hdr**, uint64_t *, uint64_t *);

extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GPU_GET_CPER_ENTRIES host_amdsmi_gpu_get_cper_entries;
extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;

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

std::string generate_file_name(int error_severity, amdsmi_cper_guid_t *notify_type,
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
		if (std::memcmp(notify_type, &boot_guid, 16) == 0) {
			prefix = "boot-";
		}
		if (std::memcmp(notify_type, &mce_guid, 16) == 0) {
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

	for(auto severity : severities) {
		if (severity == "nonfatal-uncorrected") {
			severity_mask |= AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED;
		} else if(severity == "nonfatal-corrected") {
			severity_mask |= AMDSMI_CPER_SEV_NON_FATAL_CORRECTED;
		} else if(severity == "fatal") {
			severity_mask |= AMDSMI_CPER_SEV_FATAL;
		} else if(severity == "all") {
			severity_mask |= AMDSMI_CPER_SEV_NUM;
			break;
		} else {
			severity_mask |= AMDSMI_CPER_SEV_UNUSED;
		}
	}
	return severity_mask;
}


void count_and_replace_oldest_files(const std::string& folder_path,
									const std::unordered_map<std::string, std::string>& file_timestamp_map, int file_limit)
{
	std::vector<std::pair<fs::path, std::string>> files;
	int file_count = 0;

	DIR* dir = opendir(folder_path.c_str());
	if (dir == nullptr) {
	    std::cerr << "Error: Unable to open directory " << folder_path << std::endl;
	    return;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
	    std::string filename = entry->d_name;

	    // Skip "." and ".." entries
	    if (filename == "." || filename == "..") {
		continue;
	    }

	    std::string full_path = folder_path + "/" + filename;

	    // Check if the entry is a regular file
	    struct stat file_stat;
	    if (stat(full_path.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
			auto it = file_timestamp_map.find(filename);
			if (it != file_timestamp_map.end()) {
				files.emplace_back(full_path, it->second);
				++file_count;
			}
	    }
	}

	closedir(dir);

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

int process_cper_entries(amdsmi_processor_handle processor, uint32_t severity_mask,
						 uint64_t* cursor,
						 const std::string& folder_name, int gpu_id,
						 std::unordered_map<std::string, std::string>& file_timestamp_map)
{
	int ret;
	std::string file_name;
	std::string cper_timestamp;
	std::string severity_mask_string;
	static int total_cper_count = 0;

	char cper_data[1024*1024];   // the buffer to hold the raw cper data
	amdsmi_cper_hdr* cper_hdrs[1024];  // the buffer to hold the parsed cper headers
	uint64_t buf_size = sizeof(cper_data);
	uint64_t entry_count {1024}; //sizeof(cper_hdrs) / sizeof(cper_hdrs[0]);

	do {
		ret = host_amdsmi_gpu_get_cper_entries(processor, severity_mask, cper_data, &buf_size, cper_hdrs,
											   &entry_count, cursor);
		if (ret != AMDSMI_STATUS_SUCCESS && ret != AMDSMI_STATUS_MORE_DATA) {
			return ret;
		}

		for (uint32_t i = 0; i < entry_count; i++) {
			++total_cper_count;
			file_name = generate_file_name(cper_hdrs[i]->error_severity, &cper_hdrs[i]->notify_type,
										   total_cper_count);
			cper_timestamp = print_cper_timestamp(&cper_hdrs[i]->timestamp);
			std::string path = folder_name + "/" + file_name;

			std::ofstream outFile(path, std::ios::binary);
			outFile.write((char*)cper_hdrs[i], cper_hdrs[i]->record_length);
			outFile.close();

			severity_mask_string = get_string_from_enum_cper_severity_mask(cper_hdrs[i]->error_severity);
			printf("%s \t %d \t\t %s \t %s\n", cper_timestamp.c_str(), gpu_id, severity_mask_string.c_str(),
				   file_name.c_str());

			file_timestamp_map[file_name] = cper_timestamp;
		}
	} while (ret == AMDSMI_STATUS_MORE_DATA);
	return ret;
}

int AmdSmiApiHost::amdsmi_get_cper_entries_command(Arguments arg,
		std::string& out)
{
	int ret;
	amdsmi_socket_handle socket = NULL;
	unsigned int gpu_count;

	uint32_t severity_mask = AMDSMI_CPER_SEV_NUM;
	uint64_t cursor[AMDSMI_MAX_DEVICES] {0}; // The cursor to get more data

	std::string folder_name {""};
	std::unordered_map<std::string, std::string> file_timestamp_map;

	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	amdsmi_processor_handle *processors = (amdsmi_processor_handle *)malloc(sizeof(
			amdsmi_processor_handle)*gpu_count);
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

	printf("Press CTRL + C when you want to stop\n\n");

	printf(" timestamp \t\t gpu_id \t severity \t\t file_name\n");

	if (std::find(arg.options.begin(), arg.options.end(), "follow") != arg.options.end()) {
		while (true) {
			for (unsigned gpu_id = 0; gpu_id < gpu_count; gpu_id++) {
				ret = process_cper_entries(processors[gpu_id], severity_mask, 
				&cursor[gpu_id], folder_name, gpu_id, file_timestamp_map);
			}


			// Sleep for a while before the next iteration
			std::this_thread::sleep_for(std::chrono::seconds(1));
			if (std::find(arg.options.begin(), arg.options.end(), "file_limit") != arg.options.end()) {
				if (folder_name != "") {
					count_and_replace_oldest_files(folder_name, file_timestamp_map, arg.file_limit);
				}
			}
		}
	} else {
		for (unsigned gpu_id = 0; gpu_id < gpu_count; gpu_id++) {
				ret = process_cper_entries(processors[gpu_id], severity_mask, 
				&cursor[gpu_id], folder_name, gpu_id, file_timestamp_map);
		}
		if (std::find(arg.options.begin(), arg.options.end(), "file_limit") != arg.options.end()) {
			if (folder_name != "") {
				count_and_replace_oldest_files(folder_name, file_timestamp_map, arg.file_limit);
			}
		}
	}

	out = "\n";
	return 0;
}
