/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <algorithm>
#include <cstdint>
#include <ctime>
#include <vector>

#include "smi_cli_helpers.h"
#include "smi_cli_default_command.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"
#include "smi_cli_platform.h"

// Column index constants for GPU table (9 columns: | BDF | GPU-Name | Mem-Uti Hotspot Mem-Temp Power |)
static constexpr size_t GPU_COL_DELIM_0 = 0;
static constexpr size_t GPU_COL_DELIM_1 = 4;
static constexpr size_t GPU_COL_DELIM_2 = 8;
// Column index constants for VF table (7 columns: | GPU VF | FB USAGE Driver |)
static constexpr size_t VF_COL_DELIM_0 = 0;
static constexpr size_t VF_COL_DELIM_1 = 3;
static constexpr size_t VF_COL_DELIM_2 = 6;

// Helper function for calculating visual width of UTF-8 string (character count, not bytes)
static size_t utf8_display_width(const std::string& str) {
	size_t width = 0;
	for (size_t i = 0; i < str.size(); ) {
		unsigned char c = str[i];
		if ((c & 0x80) == 0) {
			// ASCII character (1 byte)
			width++;
			i += 1;
		} else if ((c & 0xE0) == 0xC0) {
			// 2-byte UTF-8 character (e.g. °)
			width++;
			i += 2;
		} else if ((c & 0xF0) == 0xE0) {
			// 3-byte UTF-8 character
			width++;
			i += 3;
		} else if ((c & 0xF8) == 0xF0) {
			// 4-byte UTF-8 character
			width++;
			i += 4;
		} else {
			// Unknown/invalid byte, skip
			width++;
			i += 1;
		}
	}
	return width;
}

// Data container for default command: version info, GPU table, VF table, and layout metrics
struct AmdSmiDefaultCommand::DefaultCommandData {
	std::string lib_version{"N/A"};
	std::string tool_version{"N/A"};
	std::string driver_version{"N/A"};
	std::string vbios_version{"N/A"};
	std::string platform_string{"N/A"};
	std::vector<std::vector<std::string>> table;
	std::vector<std::vector<std::string>> vf_table;
	bool has_vf_data{false};
	size_t total_width{0};
	size_t left_dashes{0};
	size_t right_dashes{0};
	size_t vf_total_width{0};
	size_t vf_left_dashes{0};
	size_t vf_right_dashes{0};
	bool is_guest_platform{false};
	std::string process_info_string{};
};

void AmdSmiDefaultCommand::gather_gpu_row_data(size_t i, DefaultCommandData &data)
{
	int ret{0};
	uint64_t proc_bdf = arg.devices[i]->get_bdf();

	std::string bdf_str{"N/A"};
	ret = default_command_bdf(i, arg, bdf_str);

	std::string gpu_name_oam_id{};
	std::string gpu_name{"N/A"};
	std::string oam_id{"N/A"};
	ret = default_command_gpu_name(proc_bdf, gpu_name_oam_id);
	if (ret == 0) {
		std::vector<std::string> usage_data{split_string(gpu_name_oam_id, ',')};
		gpu_name = (usage_data.size() >= 1) ? usage_data[0] : "N/A";
		oam_id = (usage_data.size() >= 2) ? usage_data[1] : "N/A";
	}

	std::string partition_mode{"N/A"};
	ret = default_command_get_partition_mode(proc_bdf, partition_mode);

	std::string utilization{"N/A"};
	std::string mem_uti{"N/A"};
	std::string gfx_uti{"N/A"};
	ret = default_command_utilization(proc_bdf, utilization);
	if (ret == 0) {
		std::vector<std::string> util_data{split_string(utilization, ',')};
		if (util_data.size() >= 2) {
			gfx_uti = util_data[0];
			mem_uti = util_data[1];
		}
	}

	std::string power_or_fb_usage{"N/A"};
	if (AmdSmiPlatform::getInstance().is_guest()) {
		ret = default_command_fb_usage(proc_bdf, power_or_fb_usage);
		if (ret != 0) power_or_fb_usage = "N/A";
	} else {
		ret = default_command_power_usage(proc_bdf, power_or_fb_usage);
		if (ret != 0) power_or_fb_usage = "N/A";
	}

	std::string hotspot_temperature{"N/A"};
	std::string mem_temp{"N/A"};
	if (AmdSmiPlatform::getInstance().is_host() || AmdSmiPlatform::getInstance().is_baremetal()) {
		ret = default_command_temperature(proc_bdf, hotspot_temperature);
		if (ret == 0) {
			std::vector<std::string> temp_data{split_string(hotspot_temperature, ',')};
			if (temp_data.size() >= 2) {
				hotspot_temperature = (temp_data[0] != "N/A") ? (temp_data[0] + " C") : "N/A";
				mem_temp = (temp_data[1] != "N/A") ? (temp_data[1] + " C") : "N/A";
			}
		}
	} else if (AmdSmiPlatform::getInstance().is_guest()) {
		std::string pcie_info_str{"N/A"};
		ret = default_command_pcie_info(proc_bdf, pcie_info_str);
		if (ret == 0) {
			std::vector<std::string> pcie_data{split_string(pcie_info_str, ',')};
			if (pcie_data.size() >= 2) {
				hotspot_temperature = (pcie_data[0] != "N/A") ? (pcie_data[0]) : "N/A";
				mem_temp = (pcie_data[1] != "N/A") ? (pcie_data[1] + " GT/s") : "N/A";
			}
		}
	}

	data.table.push_back({"| ", bdf_str, "", gpu_name, "| ", mem_uti, hotspot_temperature, power_or_fb_usage, " |"});
	data.table.push_back({"| ", std::to_string(i), oam_id, partition_mode, "| ", gfx_uti, mem_temp, "", " |"});
}

void AmdSmiDefaultCommand::gather_default_command_data(DefaultCommandData &data)
{
	std::string version_string{};
	int ret{0};

	uint64_t processor_bdf{arg.devices[0]->get_bdf()};
	ret = default_command_version(processor_bdf, version_string);
	std::string param{"version"};
	int error = handle_exceptions(ret, param, arg);

	if (error == 0) {
		std::vector<std::string> usage_data{split_string(version_string, ',')};
		data.lib_version = (usage_data.size() >= 1) ? usage_data[0] : "N/A";
		data.tool_version = (usage_data.size() >= 2) ? usage_data[1] : "N/A";
		data.driver_version = (usage_data.size() >= 3) ? usage_data[2] : "N/A";
		data.vbios_version = (usage_data.size() >= 4) ? usage_data[3] : "N/A";
	}

	data.platform_string = AmdSmiPlatform::getInstance().get_platform();
	data.is_guest_platform = AmdSmiPlatform::getInstance().is_guest();

	// ===== BUILD GPU TABLE =====
	bool is_guest_platform = data.is_guest_platform;
	std::string col6_hdr = is_guest_platform ? "MAX_PCIE_WIDTH" : "Hotspot-Temp";
	std::string col7_hdr = is_guest_platform ? "MAX_PCIE_SPEED" : "Mem-Temp";
	std::string col8_hdr = is_guest_platform ? "FB-Usage" : "Power-Usage";
	std::vector<std::string> hdr_row1{"| ", "BDF", "", "GPU-Name", "| ", "Mem-Uti", col6_hdr, col8_hdr, " |"};
	std::vector<std::string> hdr_row2{"| ", "GPU", "OAM-ID", "Partition-Mode", "| ", "GFX-Uti", col7_hdr, "", " |"};
	data.table.push_back(hdr_row1);
	data.table.push_back(hdr_row2);

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		gather_gpu_row_data(i, data);
	}

	// ===== BUILD VF TABLE =====
	std::vector<std::string> vf_hdr_row1{"| ", "GPU", "VF", "| ", "FB USAGE/SIZE", "Driver version", " |"};
	std::vector<std::string> vf_hdr_row2{"| ", "BDF", "BDF", "| ", "", "", " |"};
	data.vf_table.push_back(vf_hdr_row1);
	data.vf_table.push_back(vf_hdr_row2);

	for (size_t i = 0; i < arg.devices.size(); i++) {
		std::string gpu_index_str{std::to_string(i)};
		std::string gpu_bdf_str{"N/A"};
		ret = default_command_bdf(i, arg, gpu_bdf_str);

		uint64_t num_vfs{0};
		std::vector<std::vector<std::string>> vf_data;
		ret = default_command_vf_data(i, arg, num_vfs, vf_data);

		for (uint64_t j = 0; j < num_vfs; j++) {
			std::string vf_index_str{std::to_string(j)};
			std::string vf_bdf = (vf_data.size() > j && vf_data[j].size() > 0) ? vf_data[j][0] : "N/A";
			std::string fb_usage = (vf_data.size() > j && vf_data[j].size() > 3) ? vf_data[j][2] + "/" + vf_data[j][3] + " MB" : "N/A";
			std::string driver_ver = (vf_data.size() > j && vf_data[j].size() > 4) ? vf_data[j][4] : "N/A";

			data.vf_table.push_back({"| ", gpu_index_str, vf_index_str, "| ", fb_usage, driver_ver, " |"});
			data.vf_table.push_back({"| ", gpu_bdf_str, vf_bdf, "| ", "", "", " |"});
		}
	}

	data.has_vf_data = (data.vf_table.size() > 2);

	// ===== GATHER PROCESS INFO (GUEST ONLY) =====
	if (data.is_guest_platform) {
		data.process_info_string.clear();
		for (size_t i = 0; i < arg.devices.size(); i++) {
			std::string one_gpu_processes;
			int proc_num = 0;
			uint64_t proc_bdf = arg.devices[i]->get_bdf();
			ret = default_get_process_info(proc_bdf, one_gpu_processes, proc_num, static_cast<int>(i));
			if (ret == 0 && !one_gpu_processes.empty()) {
				data.process_info_string += one_gpu_processes;
			}
		}
	}
}

// Compute column widths for a table; delimiter columns get fixed width 2, others get content width + 2
static std::vector<size_t> compute_column_widths(
	const std::vector<std::vector<std::string>> &table,
	const std::vector<size_t> &delim_cols)
{
	if (table.empty()) {
		return {};
	}
	const size_t num_cols = table[0].size();
	std::vector<size_t> col_widths(num_cols, 0);
	for (size_t col = 0; col < num_cols; col++) {
		for (size_t row = 0; row < table.size(); row++) {
			if (col >= table[row].size()) continue;
			size_t display_width = utf8_display_width(table[row][col]);
			if (display_width > col_widths[col]) col_widths[col] = display_width;
		}
		bool is_delim = std::find(delim_cols.begin(), delim_cols.end(), col) != delim_cols.end();
		if (!is_delim) {
			col_widths[col] += 2;
		}
		else {
			col_widths[col] = 2;
		}
	}
	return col_widths;
}

// Apply padding to table cells based on column widths and alignment
static void apply_table_padding(
	std::vector<std::vector<std::string>> &table,
	const std::vector<size_t> &col_widths,
	const std::vector<size_t> &delim_cols,
	const std::vector<size_t> &left_align_cols,
	const std::vector<size_t> &center_align_cols)
{
	const size_t num_cols = table[0].size();
	for (size_t row = 0; row < table.size(); row++) {
		if (table[row].size() < num_cols) continue;
		for (size_t col = 0; col < num_cols; col++) {
			if (std::find(delim_cols.begin(), delim_cols.end(), col) != delim_cols.end())
				continue;
			std::string &cell = table[row][col];
			size_t current_width = utf8_display_width(cell);
			if (current_width >= col_widths[col]) continue;
			size_t padding = col_widths[col] - current_width;
			bool left = std::find(left_align_cols.begin(), left_align_cols.end(), col) != left_align_cols.end();
			bool center = std::find(center_align_cols.begin(), center_align_cols.end(), col) != center_align_cols.end();
			if (left)
				cell = cell + std::string(padding, ' ');
			else if (center) {
				size_t left_pad = padding / 2;
				cell = std::string(left_pad, ' ') + cell + std::string(padding - left_pad, ' ');
			} else
				cell = std::string(padding, ' ') + cell;
		}
	}
}

void AmdSmiDefaultCommand::align_default_tables(DefaultCommandData &data)
{
	std::vector<std::vector<std::string>> &table = data.table;
	std::vector<std::vector<std::string>> &vf_table = data.vf_table;
	bool has_vf_data = data.has_vf_data;
	const size_t num_cols = table[0].size();
	const size_t vf_num_cols = vf_table[0].size();

	const std::vector<size_t> gpu_delim_cols = {GPU_COL_DELIM_0, GPU_COL_DELIM_1, GPU_COL_DELIM_2};
	const std::vector<size_t> vf_delim_cols = {VF_COL_DELIM_0, VF_COL_DELIM_1, VF_COL_DELIM_2};

	// ===== CALCULATE INITIAL COLUMN WIDTHS =====
	std::vector<size_t> col_widths = compute_column_widths(table, gpu_delim_cols);

	std::vector<size_t> vf_col_widths(vf_num_cols, 0);
	if (has_vf_data) {
		vf_col_widths = compute_column_widths(vf_table, vf_delim_cols);
	}

	// ===== AUTO-ADJUST: ALIGN BOTH TABLES =====
	size_t gpu_left_content = col_widths[1] + col_widths[2] + col_widths[3];
	size_t vf_left_content = has_vf_data ? (vf_col_widths[1] + vf_col_widths[2]) : 0;

	if (has_vf_data) {
		if (vf_left_content < gpu_left_content) {
			size_t left_extra = gpu_left_content - vf_left_content;
			vf_col_widths[1] += left_extra / 2;
			vf_col_widths[2] += left_extra - (left_extra / 2);
		} else if (gpu_left_content < vf_left_content) {
			size_t left_extra = vf_left_content - gpu_left_content;
			col_widths[1] += left_extra / 3;
			col_widths[2] += left_extra / 3;
			col_widths[3] += left_extra - 2 * (left_extra / 3);
		}
	}

	size_t gpu_total_width = 0;
	for (size_t i = 0; i < num_cols; i++) {
		gpu_total_width += col_widths[i];
	}

	size_t vf_total_width = 0;
	if (has_vf_data) {
		for (size_t i = 0; i < vf_num_cols; i++) {
			vf_total_width += vf_col_widths[i];
		}
	}

	if (has_vf_data) {
		if (vf_total_width < gpu_total_width) {
			size_t right_extra = gpu_total_width - vf_total_width;
			vf_col_widths[4] += right_extra / 2;
			vf_col_widths[5] += right_extra - (right_extra / 2);
			vf_total_width = gpu_total_width;
		} else if (gpu_total_width < vf_total_width) {
			size_t right_extra = vf_total_width - gpu_total_width;
			col_widths[5] += right_extra / 3;
			col_widths[6] += right_extra / 3;
			col_widths[7] += right_extra - 2 * (right_extra / 3);
			gpu_total_width = vf_total_width;
		}
	}

	// ===== ENSURE TOTAL WIDTH ACCOMMODATES VERSION BLOCK =====
	// Version block needs space for "Lib Version: X  Tool version: Y  Driver version: Z" etc.
	// Compute minimum width from actual version strings (driver version can be long)
	size_t ver1_len = utf8_display_width("Lib Version: " + data.lib_version) +
		utf8_display_width("Tool version: " + data.tool_version) +
		utf8_display_width("Driver version: " + data.driver_version);
	size_t ver2_len = utf8_display_width("Boot firmware: " + data.vbios_version) +
		utf8_display_width("Platform: " + data.platform_string);
	const size_t version_min_spacing = 4;  // 2 spaces between each part
	size_t version_min_content = (std::max)(ver1_len, ver2_len) + version_min_spacing;
	size_t version_min_total = version_min_content + 4;  // borders: "| " + " |"

	if (version_min_total > gpu_total_width) {
		size_t extra = version_min_total - gpu_total_width;
		// Distribute extra width to right-side content columns (5, 6, 7)
		col_widths[5] += extra / 3;
		col_widths[6] += extra / 3;
		col_widths[7] += extra - 2 * (extra / 3);
		gpu_total_width = version_min_total;
		if (has_vf_data) {
			vf_col_widths[4] += extra / 2;
			vf_col_widths[5] += extra - (extra / 2);
			vf_total_width = gpu_total_width;
		}
	}

	data.total_width = gpu_total_width;
	data.left_dashes = col_widths[1] + col_widths[2] + col_widths[3] + 1;
	data.right_dashes = data.total_width - data.left_dashes - 3;
	data.vf_total_width = vf_total_width;
	data.vf_left_dashes = data.left_dashes;
	data.vf_right_dashes = has_vf_data ? (vf_total_width - data.left_dashes - 3) : 0;

	// ===== APPLY ALIGNMENT TO GPU TABLE =====
	for (size_t row = 0; row < table.size(); row++) {
		table[row][GPU_COL_DELIM_0] = "| ";
		table[row][GPU_COL_DELIM_1] = "| ";
		table[row][GPU_COL_DELIM_2] = " |";
	}
	apply_table_padding(table, col_widths, gpu_delim_cols,
		{1, 5},   /* left-align */
		{2, 6});  /* center-align */

	// ===== APPLY ALIGNMENT TO VF TABLE =====
	if (has_vf_data) {
		for (size_t row = 0; row < vf_table.size(); row++) {
			vf_table[row][VF_COL_DELIM_0] = "| ";
			vf_table[row][VF_COL_DELIM_1] = "| ";
			vf_table[row][VF_COL_DELIM_2] = " |";
		}
		apply_table_padding(vf_table, vf_col_widths, vf_delim_cols,
			{1, 4},   /* left-align */
			{});      /* no center-align */
	}
}

void AmdSmiDefaultCommand::print_version_block(const DefaultCommandData &data, size_t total_width) const
{
	auto pad_text_left = [](const std::string &text, size_t width) {
		size_t display_width = utf8_display_width(text);
		if (display_width >= width) return text;
		return text + std::string(width - display_width, ' ');
	};

	// Content width = total_width minus border: "| " (2) left + " |" (2) right
	size_t ver_content_width = total_width - 4;

	std::cout << "+" << std::string(total_width - 2, '-') << "+" << std::endl;

	std::string ver1_part1 = "Lib Version: " + data.lib_version;
	std::string ver1_part2 = "Tool version: " + data.tool_version;
	std::string ver1_part3 = "Driver version: " + data.driver_version;
	size_t ver1_total_len = utf8_display_width(ver1_part1) + utf8_display_width(ver1_part2) + utf8_display_width(ver1_part3);
	size_t ver1_spaces = (ver_content_width > ver1_total_len) ? (ver_content_width - ver1_total_len) / 2 : 2;
	std::string ver_line1 = ver1_part1 + std::string(ver1_spaces, ' ') + ver1_part2 + std::string(ver1_spaces, ' ') + ver1_part3;
	std::cout << "| " << pad_text_left(ver_line1, ver_content_width) << " |" << std::endl;

	std::string ver2_part1 = "Boot firmware: " + data.vbios_version;
	std::string ver2_part2 = "Platform: " + data.platform_string;
	size_t ver2_total_len = utf8_display_width(ver2_part1) + utf8_display_width(ver2_part2);
	size_t ver2_spaces = (ver_content_width > ver2_total_len) ? (ver_content_width - ver2_total_len) / 2 : 2;
	std::string ver_line2 = ver2_part1 + std::string(ver2_spaces, ' ') + ver2_part2;
	std::cout << "| " << pad_text_left(ver_line2, ver_content_width) << " |" << std::endl;
}

void AmdSmiDefaultCommand::print_gpu_vf_tables(const DefaultCommandData &data) const
{
	auto make_separator = [](char left, char fill, char mid, char right, size_t lw, size_t rw) {
		return std::string(1, left) + std::string(lw, fill) + std::string(1, mid) +
			std::string(rw, fill) + std::string(1, right);
	};

	size_t left_dashes = data.left_dashes;
	size_t right_dashes = data.right_dashes;

	std::cout << make_separator('+', '-', '+', '+', left_dashes, right_dashes) << std::endl;

	const std::vector<std::vector<std::string>> &table = data.table;
	for (size_t row = 0; row < 2; row++) {
		std::string line;
		for (const auto &col : table[row]) line += col;
		std::cout << line << std::endl;
	}
	std::cout << make_separator('+', '=', '+', '+', left_dashes, right_dashes) << std::endl;

	for (size_t row = 2; row < table.size(); row++) {
		std::string line;
		for (const auto &col : table[row]) line += col;
		std::cout << line << std::endl;
		if ((row - 2) % 2 == 1 && row < table.size() - 1)
			std::cout << make_separator('+', '-', '+', '+', left_dashes, right_dashes) << std::endl;
	}
	std::cout << make_separator('+', '-', '+', '+', left_dashes, right_dashes) << std::endl;

	if (data.has_vf_data) {
		size_t vf_left_dashes = data.vf_left_dashes;
		size_t vf_right_dashes = data.vf_right_dashes;
		size_t vf_total_width = data.vf_total_width;

		std::cout << "+" << std::string(vf_total_width - 2, '-') << "+" << std::endl;
		const std::vector<std::vector<std::string>> &vf_table = data.vf_table;
		for (size_t row = 0; row < 2; row++) {
			std::string line;
			for (const auto &col : vf_table[row]) line += col;
			std::cout << line << std::endl;
		}
		std::cout << make_separator('+', '=', '+', '+', vf_left_dashes, vf_right_dashes) << std::endl;

		for (size_t row = 2; row < vf_table.size(); row++) {
			std::string line;
			for (const auto &col : vf_table[row]) line += col;
			std::cout << line << std::endl;
			if ((row - 2) % 2 == 1 && row < vf_table.size() - 1)
				std::cout << make_separator('+', '-', '+', '+', vf_left_dashes, vf_right_dashes) << std::endl;
		}
		std::cout << make_separator('+', '-', '+', '+', vf_left_dashes, vf_right_dashes) << std::endl;
	}
}

void AmdSmiDefaultCommand::print_process_table(const DefaultCommandData &data) const
{
	if (!data.is_guest_platform || data.process_info_string.empty()) {
		return;
	}

	std::vector<std::string> lines = split_string(data.process_info_string, '\n');
	std::vector<std::vector<std::string>> proc_rows;
	for (const std::string &line : lines) {
		if (line.empty()) {
			continue;
		}
		std::vector<std::string> cols = split_string(line, ',');
		if (cols.size() >= 6) {
			proc_rows.push_back({cols[0], cols[1], cols[2], cols[3], cols[4], cols[5]});
		}
	}
	if (proc_rows.empty()) {
		return;
	}

	const std::vector<std::string> hdr = {"GPU", "PID", "Process_Name", "MEM_USAGE", "GFX", "ENC"};
	const size_t num_cols = 6;
	std::vector<size_t> col_w(num_cols, 0);
	for (size_t c = 0; c < num_cols; c++) {
		col_w[c] = utf8_display_width(hdr[c]);
		for (const auto &row : proc_rows) {
			if (c < row.size()) {
				size_t w = utf8_display_width(row[c]);
				if (w > col_w[c]) col_w[c] = w;
			}
		}
		if (c < num_cols - 1) col_w[c] += 2;
	}
	size_t total_w = 2;
	for (size_t c = 0; c < num_cols; c++) total_w += col_w[c];
	total_w += (num_cols - 1) * 2 + 2;

	auto pad_right = [](const std::string &s, size_t w) {
		size_t dw = utf8_display_width(s);
		return dw >= w ? s : s + std::string(w - dw, ' ');
	};

	const std::string process_title_left{"| Process: "};
	const std::string process_title_right{" |"};
	const size_t process_title_fixed_width = process_title_left.size() + process_title_right.size();

	std::cout << "+" << std::string(total_w - 2, '-') << "+" << std::endl;
	std::cout << process_title_left << pad_right("", total_w - process_title_fixed_width) << process_title_right << std::endl;
	std::string hdr_line = "| ";
	for (size_t c = 0; c < num_cols; c++) {
		hdr_line += pad_right(hdr[c], col_w[c]);
		if (c < num_cols - 1) hdr_line += "  ";
	}
	hdr_line += " |";
	std::cout << hdr_line << std::endl;
	std::cout << "+" << std::string(total_w - 2, '=') << "+" << std::endl;
	for (const auto &row : proc_rows) {
		std::string data_line = "| ";
		for (size_t c = 0; c < num_cols; c++) {
			std::string cell = (c < row.size()) ? row[c] : "";
			data_line += pad_right(cell, col_w[c]);
			if (c < num_cols - 1) data_line += "  ";
		}
		data_line += " |";
		std::cout << data_line << std::endl;
	}
	std::cout << "+" << std::string(total_w - 2, '-') << "+" << std::endl;
}

void AmdSmiDefaultCommand::print_default_command_output(const DefaultCommandData &data)
{
	print_version_block(data, data.total_width);
	print_gpu_vf_tables(data);
	print_process_table(data);
}

int AmdSmiDefaultCommand::default_command_version(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_version_command(processor_bdf,
		arg, formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_command_bdf(uint64_t index, Arguments arg, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_bdf_command(index, arg,
		formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_command_vf_data(uint64_t index, Arguments arg, uint64_t &num_vfs,
	std::vector<std::vector<std::string>> &vf_data)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_vf_data_command(index, arg,
		num_vfs, vf_data);
	return ret;
}

int AmdSmiDefaultCommand::default_command_gpu_name(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_gpu_name_oam_id_command(processor_bdf,
		arg, formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_command_get_partition_mode(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_partition_mode_command(processor_bdf,
		arg, formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_command_uec(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_uec_command(processor_bdf,
		arg, formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_command_temperature(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_temperature_command(processor_bdf,
		arg, formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_command_power_usage(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_power_usage_command(processor_bdf,
		arg, formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_command_utilization(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_utilization_command(processor_bdf,
		arg, formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_command_pcie_info(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_pcie_info_command(processor_bdf,
		arg, formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_command_fb_usage(uint64_t processor_bdf, std::string &formatted_string)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_fb_usage_command(processor_bdf,
		arg, formatted_string);
	return ret;
}

int AmdSmiDefaultCommand::default_get_process_info(uint64_t processor_bdf, std::string &formatted_string, int &proc_num, int gpu_id)
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_default_process_info_command(processor_bdf,
		arg, formatted_string, proc_num, gpu_id);
	return ret;
}

void AmdSmiDefaultCommand::execute_command()
{
	if (AmdSmiPlatform::getInstance().is_guest()) {
		std::time_t start_timestamp{std::time(nullptr)};
		while (std::difftime(std::time(nullptr), start_timestamp) < 1.1) {}
	}

	DefaultCommandData data;
	gather_default_command_data(data);
	align_default_tables(data);
	print_default_command_output(data);
}
