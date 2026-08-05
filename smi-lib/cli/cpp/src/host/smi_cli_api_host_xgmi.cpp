/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdsmi.h"
#include "smi_cli_api_host.h"
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_exception.h"
#include "smi_cli_platform.h"

#include "json/json.h"

#include <algorithm>
#include <climits>
#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

#include <set>

typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_FB_SHARING_CAPS)(amdsmi_processor_handle,
		amdsmi_xgmi_fb_sharing_caps_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO)(amdsmi_processor_handle,
		amdsmi_processor_handle,
		amdsmi_xgmi_fb_sharing_mode_t, uint8_t *);

typedef amdsmi_status_t (*AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO)(amdsmi_processor_handle,
		amdsmi_xgmi_fb_sharing_mode_t);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_METRICS)(amdsmi_processor_handle,
		amdsmi_link_metrics_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ASIC_INFO)(amdsmi_processor_handle,
		amdsmi_asic_info_t *);

extern AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
extern AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
extern AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
extern AMDSMI_GET_XGMI_FB_SHARING_CAPS host_amdsmi_get_xgmi_fb_sharing_caps;
extern AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO host_amdsmi_get_xgmi_fb_sharing_mode_info;
extern AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO host_amdsmi_set_xgmi_fb_sharing_mode_info;
extern AMDSMI_GET_LINK_METRICS host_amdsmi_get_link_metrics;
extern AMDSMI_GET_GPU_ASIC_INFO host_amdsmi_get_gpu_asic_info;

struct amdsmi_link_metrics_values_t {
	uint64_t read;
	uint64_t write;
};

namespace
{

struct GpuSortKey {
	unsigned int gpu_index;
	uint32_t     oam_id;       // UINT32_MAX if unsupported
	uint64_t     bdf_as_uint;  // amdsmi_bdf_t::as_uint
};

int collect_gpu_sort_keys(std::vector<GpuSortKey>& keys)
{
	unsigned int gpu_count = 0;
	amdsmi_socket_handle socket = NULL;
	int ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}
	if (gpu_count == 0) {
		keys.clear();
		return AMDSMI_STATUS_SUCCESS;
	}

	std::vector<amdsmi_processor_handle> processors(gpu_count);
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, processors.data());
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	keys.clear();
	keys.reserve(gpu_count);
	for (unsigned int i = 0; i < gpu_count; i++) {
		GpuSortKey k{};
		k.gpu_index = i;
		k.oam_id = UINT32_MAX;
		k.bdf_as_uint = 0;

		amdsmi_bdf_t bdf{};
		int rc = host_amdsmi_get_gpu_device_bdf(processors[i], &bdf);
		if (rc == AMDSMI_STATUS_SUCCESS) {
			k.bdf_as_uint = bdf.as_uint;
		}

		amdsmi_asic_info_t asic{};
		rc = host_amdsmi_get_gpu_asic_info(processors[i], &asic);
		if (rc == AMDSMI_STATUS_SUCCESS) {
			k.oam_id = asic.oam_id;
		} else {
			// Log and keep going so the rest of the table still renders.
			Logger::getInstance().log(LogLevel::Error, rc, __FUNCTION__, __FILE__, __LINE__);
		}

		keys.push_back(k);
	}
	return AMDSMI_STATUS_SUCCESS;
}

void sort_keys_by_phy_id(std::vector<GpuSortKey>& keys)
{
	std::stable_sort(keys.begin(), keys.end(),
	[](const GpuSortKey& a, const GpuSortKey& b) {
		// Unsupported oam_id (UINT32_MAX) sorts last; bdf, then index break ties.
		if (a.oam_id != b.oam_id) {
			return a.oam_id < b.oam_id;
		}
		if (a.bdf_as_uint != b.bdf_as_uint) {
			return a.bdf_as_uint < b.bdf_as_uint;
		}
		return a.gpu_index < b.gpu_index;
	});
}

void sort_keys_by_bdf(std::vector<GpuSortKey>& keys)
{
	std::stable_sort(keys.begin(), keys.end(),
	[](const GpuSortKey& a, const GpuSortKey& b) {
		if (a.bdf_as_uint != b.bdf_as_uint) {
			return a.bdf_as_uint < b.bdf_as_uint;
		}
		return a.gpu_index < b.gpu_index;
	});
}

} // namespace

int build_gpu_display_order(const Arguments& arg, std::vector<unsigned int>& order)
{
	order.clear();

	unsigned int gpu_count = 0;
	amdsmi_socket_handle socket = NULL;
	int rc = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (rc != AMDSMI_STATUS_SUCCESS) {
		return rc;
	}

	std::vector<GpuSortKey> keys;
	int ret = collect_gpu_sort_keys(keys);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		order.reserve(gpu_count);
		for (unsigned int i = 0; i < gpu_count; i++) {
			order.push_back(i);
		}
		return ret;
	}

	if (arg.sort_order == SORT_BDF) {
		sort_keys_by_bdf(keys);
	} else {
		sort_keys_by_phy_id(keys);
	}
	order.reserve(keys.size());
	for (const auto& k : keys) {
		order.push_back(k.gpu_index);
	}
	return AMDSMI_STATUS_SUCCESS;
}

int sort_arg_devices_for_display(Arguments& arg)
{
	if (arg.devices.size() < 2) {
		return AMDSMI_STATUS_SUCCESS;
	}

	std::vector<unsigned int> order;
	int ret = build_gpu_display_order(arg, order);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	std::vector<int> rank_of(order.size(), -1);
	for (size_t pos = 0; pos < order.size(); pos++) {
		if (order[pos] < rank_of.size()) {
			rank_of[order[pos]] = static_cast<int>(pos);
		}
	}

	std::stable_sort(arg.devices.begin(), arg.devices.end(),
	[&rank_of](const std::shared_ptr<Device>& a, const std::shared_ptr<Device>& b) {
		int idx_a = a->get_gpu_index();
		int idx_b = b->get_gpu_index();
		int ra = (idx_a >= 0 && static_cast<size_t>(idx_a) < rank_of.size()) ? rank_of[idx_a] : INT_MAX;
		int rb = (idx_b >= 0 && static_cast<size_t>(idx_b) < rank_of.size()) ? rank_of[idx_b] : INT_MAX;
		return ra < rb;
	});

	return AMDSMI_STATUS_SUCCESS;
}

std::string get_xgmi_caps_empty_table()
{
	return "XGMI_CONFIGURATION_SUPPORT_CAPABILITY: N/A";
}

int AmdSmiApiHost::amdsmi_get_caps_xgmi_command(Arguments arg, std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle gpu_handle;
	out.append("XGMI_CONFIGURATION_SUPPORT_CAPABILITY:\n");
	out.append(string_format("%13s", " "));
	out.append(string_format("%-13s%-13s%-13s%-13s%-13s\n",
							 "MODE_1", "MODE_2", "MODE_4", "MODE_8", "MODE_CUSTOM"));

	for (uint8_t i = 0; i < arg.devices.size(); i++) {
		amdsmi_bdf_t tmp_bdf;
		tmp_bdf.as_uint = arg.devices[i]->get_bdf();

		ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &gpu_handle);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			out = get_xgmi_caps_empty_table();
			return ret;
		}

		std::string bdf{ convert_bdf_to_string(tmp_bdf.bdf.function_number, tmp_bdf.bdf.device_number, tmp_bdf.bdf.bus_number, tmp_bdf.bdf.domain_number) };
		amdsmi_xgmi_fb_sharing_caps_t xgmi_caps;
		ret = host_amdsmi_get_xgmi_fb_sharing_caps(gpu_handle, &xgmi_caps);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			out = get_xgmi_caps_empty_table();
			return ret;
		}
		std::string mode_1_caps{ xgmi_caps.cap.mode_1_cap ? "SUPPORTED" : "UNSUPPORTED" };
		std::string mode_2_caps{ xgmi_caps.cap.mode_2_cap ? "SUPPORTED" : "UNSUPPORTED" };
		std::string mode_4_caps{ xgmi_caps.cap.mode_4_cap ? "SUPPORTED" : "UNSUPPORTED" };
		std::string mode_8_caps{ xgmi_caps.cap.mode_8_cap ? "SUPPORTED" : "UNSUPPORTED" };
		std::string mode_custom_cap{ xgmi_caps.cap.mode_custom_cap ? "SUPPORTED" : "UNSUPPORTED" };
		out.append(string_format("%-13s", bdf.c_str()));
		out.append(string_format("%-13s", mode_1_caps.c_str()));
		out.append(string_format("%-13s", mode_2_caps.c_str()));
		out.append(string_format("%-13s", mode_4_caps.c_str()));
		out.append(string_format("%-13s", mode_8_caps.c_str()));
		out.append(string_format("%-13s", mode_custom_cap.c_str()));
		out.append("\n");
	}
	out.append("\n");
	return ret;
}

// Returns the column iteration order for one xgmi sub-table.
// Empty vector means "no reordering"; callers fall back to natural order.
static std::vector<unsigned int> resolve_column_order(const Arguments& arg)
{
	std::vector<unsigned int> order;
	build_gpu_display_order(arg, order);
	return order;
}

int AmdSmiApiHost::amdsmi_get_fb_sharing_xgmi_command(Arguments arg, std::string& out)
{
	auto xgmi_mode_to_str = [](const amdsmi_xgmi_fb_sharing_mode_t& mode) {
		std::string mode_s{};
		switch (mode) {
		case AMDSMI_XGMI_FB_SHARING_MODE_1:
			mode_s = "MODE_1";
			break;
		case AMDSMI_XGMI_FB_SHARING_MODE_2:
			mode_s = "MODE_2";
			break;
		case AMDSMI_XGMI_FB_SHARING_MODE_4:
			mode_s = "MODE_4";
			break;
		case AMDSMI_XGMI_FB_SHARING_MODE_8:
			mode_s = "MODE_8";
			break;
		case AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM:
			mode_s = "MODE_CUSTOM";
			break;
		default:
			mode_s = " ";
			break;
		}
		return mode_s;
	};

	unsigned int gpu_count;
	amdsmi_processor_handle *dst_handles;
	std::vector<std::string> bdf_list{};
	amdsmi_socket_handle socket = NULL;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	dst_handles = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (dst_handles == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	int ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &dst_handles[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		free(dst_handles);
		return ret;
	}

	for (uint8_t i = 0; i < gpu_count; i++) {
		amdsmi_bdf_t tmp_bdf;
		int ret = host_amdsmi_get_gpu_device_bdf(dst_handles[i],
				  &tmp_bdf);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			free(dst_handles);
			return ret;
		}
		std::string bdf{ convert_bdf_to_string(tmp_bdf.bdf.function_number, tmp_bdf.bdf.device_number, tmp_bdf.bdf.bus_number, tmp_bdf.bdf.domain_number) };
		bdf_list.push_back(bdf);
	}
	size_t matrix_rows = arg.devices.size();
	uint8_t matrix_columns = gpu_count;

	std::vector<uint8_t> requested_indexes{};
	for (uint8_t i = 0; i < arg.devices.size(); i++) {
		int gpu_index = arg.devices[i]->get_gpu_index();
		requested_indexes.push_back(gpu_index);
	}

	std::vector<amdsmi_xgmi_fb_sharing_mode_t> modes {};

	if (AmdSmiPlatform::getInstance().is_mi200()) {
		modes = {AMDSMI_XGMI_FB_SHARING_MODE_1,
				 AMDSMI_XGMI_FB_SHARING_MODE_2, AMDSMI_XGMI_FB_SHARING_MODE_4
				};
	} else {
		modes = {AMDSMI_XGMI_FB_SHARING_MODE_1,
				 AMDSMI_XGMI_FB_SHARING_MODE_2, AMDSMI_XGMI_FB_SHARING_MODE_4, AMDSMI_XGMI_FB_SHARING_MODE_8, AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM
				};
	}

	std::vector<unsigned int> column_order = resolve_column_order(arg);

	for (auto mode : modes) {
		std::vector<uint8_t> visit_dev_src_indexes = requested_indexes;
		std::vector<uint8_t> visit_dev_dst_indexes{};
		for (auto idx : column_order) {
			visit_dev_dst_indexes.push_back(static_cast<uint8_t>(idx));
		}
		if (visit_dev_dst_indexes.empty()) {
			for (uint8_t i = 0; i < gpu_count; i++) {
				visit_dev_dst_indexes.push_back(i);
			}
		}
		uint8_t last_group_src_index{0};
		uint8_t last_group_dst_index{0};
		uint8_t header_row_index{0};
		uint8_t column_row_index{0};

		std::vector<std::string> header_row{};
		header_row.push_back(" ");
		for (auto bdf : bdf_list) {
			header_row.push_back(bdf);
		}

		std::vector<std::vector<bool>> group_matrix(matrix_columns, std::vector<bool>(matrix_columns,
									false));
		std::vector<std::vector<std::string>> mode_matrix((matrix_columns+1),
										   std::vector<std::string>((matrix_columns+1), "N/A"));

		mode_matrix[header_row_index] = header_row;

		int i = 0;
		std::vector<bool> src_indexes_moved(visit_dev_dst_indexes.size(), false);
		std::vector<bool> dst_indexes_moved(visit_dev_dst_indexes.size(), false);

		uint8_t current_src_index;
		uint8_t current_dst_index;
		while (i < visit_dev_src_indexes.size()) {
			current_src_index = visit_dev_src_indexes[i];
			amdsmi_processor_handle src_dev = dst_handles[current_src_index];
			uint8_t diagonal_start{0};
			if (matrix_rows == matrix_columns) {
				diagonal_start = i;
			}
			uint8_t j = diagonal_start;
			while (j >= diagonal_start && j < visit_dev_dst_indexes.size()) {
				current_dst_index = visit_dev_dst_indexes[j];
				amdsmi_processor_handle dst_dev = dst_handles[current_dst_index];
				uint8_t is_fb_sharing_enabled{0};
				ret = host_amdsmi_get_xgmi_fb_sharing_mode_info(src_dev,
						dst_dev, mode, &is_fb_sharing_enabled);
				if (ret != AMDSMI_STATUS_SUCCESS) {
					Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
					std::string mode_str{xgmi_mode_to_str(mode)};
					out.append(string_format("%s TABLE:\n %s",
											 mode_str.c_str(), "N/A"));
				}
				if (is_fb_sharing_enabled != 0) {
					group_matrix[current_src_index][current_dst_index] = true;
					if (current_src_index != current_dst_index) {
						group_matrix[current_dst_index][current_src_index] = true;
					}
					if (matrix_rows > 1) {
						if (src_indexes_moved[current_src_index] == false) {
							auto it{std::find(visit_dev_src_indexes.begin(), visit_dev_src_indexes.end(), current_src_index)};
							visit_dev_src_indexes.erase(it);
							std::vector<uint8_t>::iterator insert_it{visit_dev_src_indexes.begin() + last_group_src_index};
							visit_dev_src_indexes.insert(insert_it, current_src_index);
							last_group_src_index += 1;
							src_indexes_moved[current_src_index] = true;
						}
						auto it{std::find(visit_dev_src_indexes.begin(), visit_dev_src_indexes.end(), current_dst_index)};
						if (current_src_index != current_dst_index &&
								(matrix_rows == matrix_columns || it != visit_dev_src_indexes.end())) {
							if (src_indexes_moved[current_dst_index] == false) {
								auto it{std::find(visit_dev_src_indexes.begin(), visit_dev_src_indexes.end(), current_dst_index)};
								visit_dev_src_indexes.erase(it);
								std::vector<uint8_t>::iterator insert_it{visit_dev_src_indexes.begin() + last_group_src_index};
								visit_dev_src_indexes.insert(insert_it, current_dst_index);
								last_group_src_index += 1;
								src_indexes_moved[current_dst_index] = true;
							}
							if (dst_indexes_moved[current_src_index] == false) {
								auto it{std::find(visit_dev_dst_indexes.begin(), visit_dev_dst_indexes.end(), current_src_index)};
								visit_dev_src_indexes.erase(it);
								std::vector<uint8_t>::iterator insert_it{visit_dev_dst_indexes.begin() + last_group_dst_index};
								visit_dev_dst_indexes.insert(insert_it, current_src_index);
								last_group_dst_index += 1;
								dst_indexes_moved[current_src_index] = true;
							}
						}
					}
					if (dst_indexes_moved[current_dst_index] == false) {
						auto it{std::find(visit_dev_dst_indexes.begin(), visit_dev_dst_indexes.end(), current_dst_index)};
						visit_dev_dst_indexes.erase(it);
						std::vector<uint8_t>::iterator insert_it{visit_dev_dst_indexes.begin() + last_group_dst_index};
						visit_dev_dst_indexes.insert(insert_it, current_dst_index);
						last_group_dst_index += 1;
						dst_indexes_moved[current_dst_index] = true;
					}
				} else {
					group_matrix[current_src_index][current_dst_index] = false;
					if (current_src_index != current_dst_index) {
						group_matrix[current_dst_index][current_src_index] = false;
					}
				}
				if (last_group_dst_index > j) {
					j = last_group_dst_index;
				} else {
					j += 1;
				}
			}
			src_indexes_moved[current_src_index] = true;
			if (last_group_src_index > i) {
				i = last_group_src_index;
			} else {
				i += 1;
			}
		}
		// group_matrix is keyed on GPU index, so restore --sort order for output.
		visit_dev_dst_indexes.clear();
		for (auto idx : column_order) {
			visit_dev_dst_indexes.push_back(static_cast<uint8_t>(idx));
		}
		if (visit_dev_dst_indexes.empty()) {
			for (uint8_t k = 0; k < gpu_count; k++) {
				visit_dev_dst_indexes.push_back(k);
			}
		}
		visit_dev_src_indexes = requested_indexes;

		for (uint8_t i = 1; i < header_row.size(); i++) {
			mode_matrix[0][i] = bdf_list[visit_dev_dst_indexes[i-1]];
		}
		for (uint8_t i = 0; i < visit_dev_src_indexes.size(); i++) {
			current_src_index = visit_dev_src_indexes[i];
			std::string gpu_header{bdf_list[current_src_index]};
			mode_matrix[i+1][column_row_index] = gpu_header;
			uint8_t diagonal_start{0};
			if (matrix_rows == matrix_columns) {
				diagonal_start = i;
			}
			for (uint8_t j = diagonal_start; j < visit_dev_dst_indexes.size(); j++) {
				current_dst_index = visit_dev_dst_indexes[j];
				uint8_t is_fb_sharing_enabled{0};
				if (group_matrix[current_src_index][current_dst_index] == false) {
					amdsmi_processor_handle src_dev = dst_handles[current_src_index];
					amdsmi_processor_handle dst_dev = dst_handles[current_dst_index];
					ret = host_amdsmi_get_xgmi_fb_sharing_mode_info(src_dev,
							dst_dev, mode, &is_fb_sharing_enabled);
					if (ret != AMDSMI_STATUS_SUCCESS) {
						std::string mode_str{xgmi_mode_to_str(mode)};
						out.append(string_format("%s TABLE:\n %s",
												 mode_str.c_str(), "N/A"));
					}
					if (is_fb_sharing_enabled != 0) {
						group_matrix[current_src_index][current_dst_index] = true;
					} else {
						group_matrix[current_src_index][current_dst_index] = false;
					}
					if (current_dst_index < (matrix_rows+1) && current_src_index != current_dst_index) {
						group_matrix[current_dst_index][current_src_index] =
							group_matrix[current_src_index][current_dst_index];
					}
				}
				std::string is_enabled_str{group_matrix[current_src_index][current_dst_index] ? "ON" : "OFF"};
				mode_matrix[i+1][j+1] = is_enabled_str;
				if (j < matrix_rows && i != j) {
					mode_matrix[j+1][i+1] = mode_matrix[i+1][j+1];
				}
			}
		}

		std::string mode_str{xgmi_mode_to_str(mode)};

		out.append(string_format("%s TABLE:\n",
								 mode_str.c_str()));
		for (uint8_t i = 0; i <= arg.devices.size(); i++) {
			for (uint8_t j = 0; j <= gpu_count; j++) {
				out.append(string_format("%-13s",
										 mode_matrix[i][j].c_str()));
			}
			out.append("\n");
		}
		out.append("\n");

	}

	free(dst_handles);
	return ret;
}

void print_first_row(std::string &out,
					 std::vector<std::string> bdf_vector,
					 const std::vector<unsigned int>& column_order)
{
	out.append(string_format("%-10s%-13s%-9s%-14s%-10s", " ",
							 "bdf","bit_rate","max_bandwidth","link_type"));
	for (auto idx : column_order) {
		if (idx < bdf_vector.size()) {
			out.append(string_format("%-13s",
									 bdf_vector[idx].c_str()));
		}
	}

	out.append("\n");
}

void print_gpu_row(const char *spaces,int gpu_index,
				   std::string &out,std::vector<std::string> bdf_vector,
				   amdsmi_link_metrics_t link_metrics)
{

	out.append(string_format(spaces,"GPU",gpu_index));

	out.append(string_format("%-13s",
							 bdf_vector[gpu_index].c_str()));

	uint32_t bit_rate = link_metrics.links[0].bit_rate;
	out.append(string_format("%-ld%-7s",
							 bit_rate," Gb/s"));
	uint32_t max_bandwidth = link_metrics.links[0].max_bandwidth;
	out.append(string_format("%-ld%-11s",
							 max_bandwidth, " Gb/s"));
	amdsmi_link_type_t link_type = link_metrics.links[0].link_type;
	std::string link_type_string = format_link_type(link_type);
	out.append(string_format("%-13s\n",
							 link_type_string.c_str()));
}

void print_read_write_row(const char *read_write,int gpu_index, int i,
						  std::string &out, std::vector<std::map<int, amdsmi_link_metrics_values_t>> xgmi_matrix,
						  const std::vector<unsigned int>& column_order)
{
	out.append(string_format("%-3s%-5s%-48s", " ",
							 read_write," "));

	for (auto j : column_order) {
		if(gpu_index == static_cast<int>(j)) {
			out.append(string_format("%-13s",
									 "SELF"));
		} else {
			auto write = xgmi_matrix[i][j].write;
			out.append(string_format("%-lld%-12s",
									 write, " KB"));
		}
	}
	out.append("\n");
}

void print_read_row(const char *read_write,int gpu_index, int i,
					std::string &out, std::vector<std::map<int, amdsmi_link_metrics_values_t>> xgmi_matrix,
					const std::vector<unsigned int>& column_order)
{
	out.append(string_format("%-3s%-5s%-48s", " ",
							 read_write," "));

	for (auto j : column_order) {
		if(gpu_index == static_cast<int>(j)) {
			out.append(string_format("%-13s",
									 "SELF"));
		} else {
			auto read = xgmi_matrix[i][j].read;
			out.append(string_format("%-lld%-12s",
									 read, " KB"));
		}
	}
	out.append("\n");
}


void print_all_xgmi_table(size_t gpu_number,
						  std::string &out, std::vector<std::string> bdf_vector,
						  amdsmi_link_metrics_t link_metrics,
						  std::vector<std::map<int, amdsmi_link_metrics_values_t>> xgmi_matrix,
						  std::vector<std::shared_ptr<Device> > devices, unsigned int gpu_count,
						  const std::vector<unsigned int>& column_order)
{

	unsigned int i;
	int gpu_index;

	out.append(metricXgmiLinkMetricTableTemplate);
	print_first_row(out, bdf_vector, column_order);

	for (i = 0; i < gpu_number; i++) {
		gpu_index = devices[i]->get_gpu_index();
		print_gpu_row("%-s%-7d",gpu_index,out,bdf_vector,link_metrics);
		print_read_row("READ", gpu_index, i, out, xgmi_matrix, column_order);
		print_read_write_row("WRITE", gpu_index, i, out, xgmi_matrix, column_order);
	}
	out.append("\n");
}

int AmdSmiApiHost::amdsmi_get_xgmi_metric_command(Arguments arg, std::string& out)
{
	amdsmi_status_t ret;
	std::string bdf_string{};
	std::vector<std::string> bdf_vector;
	std::vector<std::map<int, amdsmi_link_metrics_values_t>> xgmi_matrix;
	amdsmi_link_metrics_values_t link_metrics_value_for_specific_bdf;
	std::map<int, amdsmi_link_metrics_values_t> bdf_value_map;
	amdsmi_link_metrics_t link_metrics;
	unsigned int i;
	unsigned int j;
	size_t gpu_number;
	amdsmi_processor_handle processor_handle;
	amdsmi_bdf_t bdf;
	int bdf_index;
	amdsmi_bdf_t tmp_bdf;
	unsigned int gpu_count;
	amdsmi_processor_handle *processors;
	amdsmi_socket_handle socket = NULL;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));

	gpu_number = arg.devices.size();

	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		free(processors);
		return ret;
	}

	for (i = 0; i < gpu_count; i++) {
		ret = host_amdsmi_get_gpu_device_bdf(processors[i], &tmp_bdf);
		bdf_string = string_format(
						 "%04x:%02x:%02x.%01x", tmp_bdf.bdf.domain_number, tmp_bdf.bdf.bus_number, tmp_bdf.bdf.device_number,
						 tmp_bdf.bdf.function_number);
		bdf_vector.push_back(bdf_string);
		bdf_string.clear();
	}

	for (i = 0; i < gpu_number; i++) {
		tmp_bdf.as_uint = arg.devices[i]->get_bdf();

		ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &processor_handle);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			free(processors);
			return ret;
		}
		ret = host_amdsmi_get_link_metrics(processor_handle, &link_metrics);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			free(processors);
			return ret;
		}
		uint32_t num_links = link_metrics.num_links;
		for (j = 0; j < num_links; j++) {
			bdf = link_metrics.links[j].bdf;
			std::string bdf_str = string_format("%04x:%02x:%02x.%01x", bdf.bdf.domain_number,
												bdf.bdf.bus_number,
												bdf.bdf.device_number, bdf.bdf.function_number);
			bdf_index = 0;
			auto it = std::find(bdf_vector.begin(), bdf_vector.end(), bdf_str);

			if (it != bdf_vector.end()) {
				bdf_index = it - bdf_vector.begin();
				link_metrics_value_for_specific_bdf.read = link_metrics.links[j].read;
				link_metrics_value_for_specific_bdf.write = link_metrics.links[j].write;
				bdf_value_map[bdf_index] = link_metrics_value_for_specific_bdf;
			} else {
				link_metrics_value_for_specific_bdf.read = link_metrics.links[j].read;
				link_metrics_value_for_specific_bdf.write = link_metrics.links[j].write;
				bdf_value_map[i] = link_metrics_value_for_specific_bdf;
			}
		}
		xgmi_matrix.push_back(bdf_value_map);
	}

	std::vector<unsigned int> column_order = resolve_column_order(arg);
	if (column_order.empty()) {
		for (unsigned int k = 0; k < gpu_count; k++) {
			column_order.push_back(k);
		}
	}

	print_all_xgmi_table(gpu_number, out, bdf_vector, link_metrics,
						 xgmi_matrix, arg.devices, gpu_count, column_order);

	free(processors);
	return ret;
}

int AmdSmiApiHost::amdsmi_get_source_gpu_xgmi_status_command(Arguments arg, std::string& out)
{
	amdsmi_status_t ret;
	amdsmi_processor_handle gpu_handle;
	amdsmi_link_metrics_t link_metrics;
	amdsmi_bdf_t tmp_bdf;
	tmp_bdf.as_uint = arg.devices[0]->get_bdf();

	ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &gpu_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		out = get_xgmi_caps_empty_table();
		return ret;
	}

	ret = host_amdsmi_get_link_metrics(gpu_handle, &link_metrics);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		out = get_xgmi_caps_empty_table();
		return ret;
	}

	out.append("SOURCE_GPU_XGMI_PORTS:\n");
	out.append(string_format("%-13s%-15s%-13s\n",
							 "GPU", "BDF", "PORTS"));
	out.append("                            ");
	out.append(string_format("%-3d",0 ));
	for(int j=1; j < link_metrics.num_links; j++) {
		out.append(string_format(" %-3d",j ));
	}
	out.append("\n");

	for (uint8_t i = 0; i < arg.devices.size(); i++) {
		amdsmi_bdf_t tmp_bdf;
		tmp_bdf.as_uint = arg.devices[i]->get_bdf();

		ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &gpu_handle);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			out = get_xgmi_caps_empty_table();
			return ret;
		}

		std::string bdf{ convert_bdf_to_string(tmp_bdf.bdf.function_number, tmp_bdf.bdf.device_number, tmp_bdf.bdf.bus_number, tmp_bdf.bdf.domain_number) };

		ret = host_amdsmi_get_link_metrics(gpu_handle, &link_metrics);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			out = get_xgmi_caps_empty_table();
			return ret;
		}

		std::string gpu_bdf_string = "GPU" + std::to_string(i);
		out.append(string_format("%-13s%-15s", gpu_bdf_string.c_str(), bdf.c_str()));
		std::string link_status_string = {};
		for(int j=0; j<link_metrics.num_links; j++) {
			format_link_status(link_metrics.links[j].link_status, true, link_status_string);
			out.append(string_format("%-4s", link_status_string.c_str()));
		}
		out.append("\n");
	}

	out.append(
		"\nLegend:\n"
		" N/A = Not supported \n"
		" U / D / X = Link is Up / Down / Disabled\n\n"
	);

	return 0;
}
int AmdSmiApiHost::amdsmi_get_xgmi_link_status_command(Arguments arg, std::string& out)
{
	amdsmi_status_t ret;
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;
	amdsmi_processor_handle *processors;
	std::vector<std::string> bdf_vector;

	amdsmi_get_gpu_count(gpu_count);
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(processors);
		out = get_xgmi_caps_empty_table();
		return ret;
	}

	for (unsigned int i = 0; i < gpu_count; i++) {
		amdsmi_bdf_t tmp_bdf;
		ret = host_amdsmi_get_gpu_device_bdf(processors[i], &tmp_bdf);
		std::string bdf_str = convert_bdf_to_string(tmp_bdf.bdf.function_number, tmp_bdf.bdf.device_number,
							  tmp_bdf.bdf.bus_number, tmp_bdf.bdf.domain_number);
		bdf_vector.push_back(bdf_str);
	}

	const std::vector<std::shared_ptr<Device>>& devices = arg.devices.size() ? arg.devices : [&] {
		std::vector<std::shared_ptr<Device>> all;
		for (unsigned int i = 0; i < gpu_count; i++)
			all.push_back(std::make_shared<Device>(i, DeviceIdentifierType::INDEX, DeviceType::GPU));
		return all;
	}();

	std::vector<unsigned int> column_order;
	build_gpu_display_order(arg, column_order);
	if (column_order.empty()) {
		for (unsigned int k = 0; k < gpu_count; k++) {
			column_order.push_back(k);
		}
	}

	out.append("XGMI_LINK_STATUS:\n");
	out.append(string_format("%-13s%-15s", "GPU", "BDF"));
	for (auto idx : column_order) {
		if (idx < bdf_vector.size()) {
			out.append(string_format("%-15s", bdf_vector[idx].c_str()));
		}
	}
	out.append("\n");

	for (size_t i = 0; i < devices.size(); i++) {
		amdsmi_bdf_t tmp_bdf;
		tmp_bdf.as_uint = devices[i]->get_bdf();
		amdsmi_processor_handle gpu_handle;
		ret = host_amdsmi_get_processor_handle_from_bdf(tmp_bdf, &gpu_handle);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			continue;
		}
		std::string row_bdf = convert_bdf_to_string(tmp_bdf.bdf.function_number, tmp_bdf.bdf.device_number,
							  tmp_bdf.bdf.bus_number, tmp_bdf.bdf.domain_number);

		amdsmi_link_metrics_t link_metrics;
		ret = host_amdsmi_get_link_metrics(gpu_handle, &link_metrics);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			continue;
		}

		std::string gpu_name = "GPU" + std::to_string(devices[i]->get_gpu_index());
		out.append(string_format("%-13s%-15s", gpu_name.c_str(), row_bdf.c_str()));

		// Map link BDF to status
		std::map<std::string, std::string> bdf_to_status;
		for (uint32_t j = 0; j < link_metrics.num_links; j++) {
			std::string link_bdf = convert_bdf_to_string(
									   link_metrics.links[j].bdf.bdf.function_number,
									   link_metrics.links[j].bdf.bdf.device_number,
									   link_metrics.links[j].bdf.bdf.bus_number,
									   link_metrics.links[j].bdf.bdf.domain_number);
			std::string link_status_string;
			format_link_status(link_metrics.links[j].link_status, true, link_status_string);
			bdf_to_status[link_bdf] = link_status_string;
		}

		for (auto idx : column_order) {
			if (idx >= bdf_vector.size()) {
				continue;
			}
			const std::string& col_bdf = bdf_vector[idx];
			if (row_bdf == col_bdf) {
				out.append(string_format("%-15s", "SELF"));
			} else {
				auto it = bdf_to_status.find(col_bdf);
				if (it != bdf_to_status.end())
					out.append(string_format("%-15s", it->second.c_str()));
				else
					out.append(string_format("%-15s", "D"));
			}
		}
		out.append("\n");
	}

	out.append(
		"\nLegend:\n"
		" SELF = Current GPU\n"
		" N/A = Not supported \n"
		" U / D / X = Link is Up / Down / Disabled\n"
	);

	free(processors);
	return 0;
}

int amdsmi_get_bdf(int index, std::string& bdf)
{
	unsigned int gpu_count;
	amdsmi_socket_handle socket = NULL;
	amdsmi_processor_handle *processors;
	amdsmi_bdf_t tmp_bdf;
	int ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		exit(1);
	}

	if (index >= gpu_count) {
		exit(1);
	}
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}

	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);
	amdsmi_processor_handle gpu_handle = processors[index];
	ret = host_amdsmi_get_gpu_device_bdf(gpu_handle, &tmp_bdf);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		free(processors);
		exit(1);
	}

	bdf = convert_bdf_to_string(tmp_bdf.bdf.function_number, tmp_bdf.bdf.device_number,
								tmp_bdf.bdf.bus_number,
								tmp_bdf.bdf.domain_number);
	free(processors);
	return 0;
}

int AmdSmiApiHost::amdsmi_get_all_xgmi_command(Arguments arg, std::string& out)
{
	int ret;
	unsigned int i = 0;
	unsigned int j = 0;
	amdsmi_socket_handle socket = NULL;
	amdsmi_processor_handle *processors;
	unsigned int gpu_count;
	amdsmi_link_metrics_t link_metrics;
	std::string bdf_string{};

	nlohmann::ordered_json output = nlohmann::ordered_json::array();
	nlohmann::ordered_json json;

	amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, NULL);
	processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*gpu_count);
	if (processors == NULL) {
		throw SmiToolNotEnoughMemException();
	}
	ret = host_amdsmi_get_processor_handles(socket, &gpu_count, &processors[0]);


	amdsmi_bdf_t tmp_bdf;
	std::vector<std::string> bdf_vector;

	for (i = 0; i < gpu_count; i++) {
		ret = host_amdsmi_get_gpu_device_bdf(processors[i], &tmp_bdf);
		bdf_string = string_format("%04x:%02x:%02x.%01x", tmp_bdf.bdf.domain_number, tmp_bdf.bdf.bus_number,
								   tmp_bdf.bdf.device_number,
								   tmp_bdf.bdf.function_number);
		bdf_vector.push_back(bdf_string);
		bdf_string.clear();
	}

	for (int i = 0; i < arg.devices.size(); i++) {
		json = {};
		int gpu_index = arg.devices[i]->get_gpu_index();
		json["gpu"] = gpu_index;

		std::string gpu_bdf = {};
		amdsmi_get_bdf(gpu_index, gpu_bdf);

		json["bdf"] = gpu_bdf;

		amdsmi_xgmi_fb_sharing_caps_t xgmi_caps;
		ret = host_amdsmi_get_xgmi_fb_sharing_caps(processors[gpu_index], &xgmi_caps);
		if (ret != AMDSMI_STATUS_SUCCESS) {
			Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		}

		std::string mode_1_caps{ xgmi_caps.cap.mode_1_cap ? "SUPPORTED" : "UNSUPPORTED" };
		std::string mode_2_caps{ xgmi_caps.cap.mode_2_cap ? "SUPPORTED" : "UNSUPPORTED" };
		std::string mode_4_caps{ xgmi_caps.cap.mode_4_cap ? "SUPPORTED" : "UNSUPPORTED" };
		std::string mode_8_caps{ xgmi_caps.cap.mode_8_cap ? "SUPPORTED" : "UNSUPPORTED" };
		std::string mode_custom_cap{ xgmi_caps.cap.mode_custom_cap ? "SUPPORTED" : "UNSUPPORTED" };

		if (std::find(arg.options.begin(), arg.options.end(), "caps") != arg.options.end() ||
				arg.all_arguments) {
			std::string param{"caps"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				json["caps"]["mode_1"] = mode_1_caps;
				json["caps"]["mode_2"] = mode_2_caps;
				json["caps"]["mode_4"] = mode_4_caps;
				json["caps"]["mode_8"] = mode_8_caps;
				json["caps"]["mode_custom"] = mode_custom_cap;
			}
		}

		if (std::find(arg.options.begin(), arg.options.end(), "metric") != arg.options.end() ||
				arg.all_arguments) {

			json["link_metrics"] = {};

			ret = host_amdsmi_get_link_metrics(processors[gpu_index], &link_metrics);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			}
			std::string param{"metric"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				json["link_metrics"] = {};
				json["link_metrics"]["bit_rate"]["value"] = link_metrics.links[0].bit_rate;
				json["link_metrics"]["bit_rate"]["unit"] = "Gb/s";

				json["link_metrics"]["max_bandwidth"]["value"] = link_metrics.links[0].max_bandwidth;
				json["link_metrics"]["max_bandwidth"]["unit"] = "Gb/s";

				json["link_metrics"]["links"] = nlohmann::ordered_json::array();


				nlohmann::ordered_json link_metric_object = {};

				link_metric_object["gpu"] = gpu_index;
				link_metric_object["bdf"] = gpu_bdf;
				link_metric_object["read"]["value"] = "SELF";
				link_metric_object["read"]["unit"] = "SELF";

				link_metric_object["write"]["value"] = "SELF";
				link_metric_object["write"]["unit"] = "SELF";

				json["link_metrics"]["links"].insert(json["link_metrics"]["links"].end(), link_metric_object);


				uint32_t num_links = link_metrics.num_links;
				for (j = 0; j < num_links; j++) {
					std::string link_bdf = convert_bdf_to_string(link_metrics.links[j].bdf.bdf.function_number,
										   link_metrics.links[j].bdf.bdf.device_number,
										   link_metrics.links[j].bdf.bdf.bus_number,
										   link_metrics.links[j].bdf.bdf.domain_number);

					int bdf_index = j;
					auto it = std::find(bdf_vector.begin(), bdf_vector.end(), link_bdf);
					if (it != bdf_vector.end()) {
						bdf_index = it - bdf_vector.begin();
						link_metric_object["gpu"] = bdf_index;
						link_metric_object["bdf"] = link_bdf;
						link_metric_object["read"]["value"] = link_metrics.links[j].read;
						link_metric_object["read"]["unit"] = "KB";

						link_metric_object["write"]["value"] = link_metrics.links[j].write;
						link_metric_object["write"]["unit"] = "KB";
					}

					json["link_metrics"]["links"].insert(json["link_metrics"]["links"].end(), link_metric_object);
				}
			}
		}

		if (std::find(arg.options.begin(), arg.options.end(), "link-status") != arg.options.end() ||
				arg.all_arguments) {

			json["link_status"] = {};

			ret = host_amdsmi_get_link_metrics(processors[gpu_index], &link_metrics);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			}
			std::string param{"link-status"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				json["link_status"] = nlohmann::ordered_json::array();


				nlohmann::ordered_json link_metric_object = {};

				uint32_t num_links = link_metrics.num_links;
				for (j = 0; j < num_links; j++) {
					std::string link_bdf = convert_bdf_to_string(link_metrics.links[j].bdf.bdf.function_number,
										   link_metrics.links[j].bdf.bdf.device_number,
										   link_metrics.links[j].bdf.bdf.bus_number,
										   link_metrics.links[j].bdf.bdf.domain_number);

					int bdf_index = j;
					auto it = std::find(bdf_vector.begin(), bdf_vector.end(), link_bdf);
					if (it != bdf_vector.end()) {
						bdf_index = it - bdf_vector.begin();
						link_metric_object["gpu"] = bdf_index;
						link_metric_object["bdf"] = link_bdf;
						std::string link_status_string;
						format_link_status(link_metrics.links[j].link_status, true, link_status_string);

						link_metric_object["status"]= link_status_string.c_str();
					}

					json["link_status"].insert(json["link_status"].end(), link_metric_object);
				}
			}
		}

		if (std::find(arg.options.begin(), arg.options.end(), "fb-sharing") != arg.options.end() ||
				arg.all_arguments) {
			json["link_metrics"]["fb_sharing"] = nlohmann::ordered_json::array();
			std::vector<unsigned int> column_order;
			build_gpu_display_order(arg, column_order);
			if (column_order.empty()) {
				for (unsigned int k = 0; k < gpu_count; k++) {
					column_order.push_back(k);
				}
			}
			for (auto col_idx : column_order) {
				j = col_idx;
				amdsmi_processor_handle destination = processors[j];
				uint8_t mode1_enabled{0};
				uint8_t mode2_enabled{0};
				uint8_t mode4_enabled{0};
				uint8_t mode8_enabled{0};
				uint8_t mode_custom_enabled{0};

				std::string gpu_bdf = {};
				amdsmi_get_bdf(j, gpu_bdf);

				nlohmann::ordered_json fb_sharing_element = {};
				fb_sharing_element["gpu"] = j;
				fb_sharing_element["bdf"] = gpu_bdf;
				ret = host_amdsmi_get_xgmi_fb_sharing_mode_info(processors[gpu_index],
						processors[j], AMDSMI_XGMI_FB_SHARING_MODE_1, &mode1_enabled);
				if (ret == 0 && xgmi_caps.cap.mode_1_cap) {
					fb_sharing_element["mode_1"] = mode1_enabled == 1 ? "ON" : "OFF";
				}
				ret = host_amdsmi_get_xgmi_fb_sharing_mode_info(processors[gpu_index],
						processors[j], AMDSMI_XGMI_FB_SHARING_MODE_2, &mode2_enabled);
				if (ret == 0 && xgmi_caps.cap.mode_2_cap) {
					fb_sharing_element["mode_2"] = mode2_enabled == 1 ? "ON" : "OFF";
				}
				ret = host_amdsmi_get_xgmi_fb_sharing_mode_info(processors[gpu_index],
						processors[j], AMDSMI_XGMI_FB_SHARING_MODE_4, &mode4_enabled);
				if (ret == 0 && xgmi_caps.cap.mode_4_cap) {
					fb_sharing_element["mode_4"] = mode4_enabled == 1 ? "ON" : "OFF";
				}
				ret = host_amdsmi_get_xgmi_fb_sharing_mode_info(processors[gpu_index],
						processors[j], AMDSMI_XGMI_FB_SHARING_MODE_8, &mode8_enabled);
				if (ret == 0 && xgmi_caps.cap.mode_8_cap) {
					fb_sharing_element["mode_8"] = mode8_enabled == 1 ? "ON" : "OFF";
				}
				ret = host_amdsmi_get_xgmi_fb_sharing_mode_info(processors[gpu_index],
						processors[j], AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM, &mode_custom_enabled);
				if (ret == 0 && xgmi_caps.cap.mode_custom_cap) {
					fb_sharing_element["mode_custom"] = mode_custom_enabled == 1 ? "ON" : "OFF";
				}

				json["link_metrics"]["fb_sharing"].insert(json["link_metrics"]["fb_sharing"].end(),
					fb_sharing_element);
			}
		}

		if (std::find(arg.options.begin(), arg.options.end(), "source-status") != arg.options.end() ||
				arg.all_arguments) {

			ret = host_amdsmi_get_link_metrics(processors[gpu_index], &link_metrics);
			if (ret != AMDSMI_STATUS_SUCCESS) {
				Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
			}
			std::string param{"source-status"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				json["port_status"] = nlohmann::ordered_json::array();
				std::string link_status_string;
				for (int j=0; j < link_metrics.num_links; j++) {
					format_link_status(link_metrics.links[j].link_status, true, link_status_string);
					json["port_status"].insert(json["port_status"].end(), link_status_string.c_str());
				}
			}
		}

		// Add the json object for this GPU to the output array
		output.insert(output.end(), json);
	}

	out = output.dump(4);

	free(processors);
	return 0;
}
