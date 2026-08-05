/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_cli_platform.h"
#include "smi_cli_help_info.h"
#include "smi_cli_helpers.h"
#include "smi_cli_exception.h"
#include "smi_cli_help_strings.h"
#include "smi_cli_argument.h"

namespace
{
constexpr int USAGE_COLUMNS_PER_ROW_SPECIFIC = 4;
const std::string USAGE_LINE_BREAK = SmiCliArgument::get_usage_line_break();

static bool is_first_group_in_sequence = true;

void reset_group_sequence()
{
	is_first_group_in_sequence = true;
}

std::string get_device_arguments(const std::string& device_key)
{
	const auto* arg = SmiCliArgumentFactory::getInstance().get_argument(device_key + "_device");
	return arg ? arg->get_arguments() : "";
}

std::string get_device_usage(const std::string& device_key)
{
	const auto* arg = SmiCliArgumentFactory::getInstance().get_argument(device_key + "_device");
	return arg ? " " + arg->get_usage() : "";
}

std::string get_file_usage()
{
	const auto* arg = SmiCliArgumentFactory::getInstance().get_argument("file");
	return arg ? " " + arg->get_usage() : "";
}
std::string get_help_arguments()
{
	const auto* arg = SmiCliArgumentFactory::getInstance().get_argument("help");
	return arg ? arg->get_arguments() : "";
}
std::string get_help_usage()
{
	const auto* arg = SmiCliArgumentFactory::getInstance().get_argument("help");
	return arg ? " " + arg->get_usage() : "";
}
std::string get_format_usage(const std::vector<std::string>& format_keys)
{
	return SmiCliArgumentFactory::getInstance().get_format_usage(format_keys);
}
std::string get_format_usage_by_device_type(DevicesType device_type)
{
	std::vector<std::string> format_keys = (device_type == NIC_TYPE) ?
										   std::vector<std::string> {"json"} :
										   std::vector<std::string> {"json", "csv"};
	return get_format_usage(format_keys);
}
std::string get_help_commands_from_factory(const
		std::map<std::string, std::map<std::string, std::vector<std::string>>>& vector_map,
		const std::string& category,
		const std::string& vector_key)
{
	const auto& command_names = vector_map.at(category).at(vector_key);
	return SmiCliArgumentFactory::getInstance().get_help_commands_string(command_names);
}

std::string concat_arguments_from_factory(const
		std::map<std::string, std::map<std::string, std::vector<std::string>>>& vector_map,
		const std::string& category,
		const std::string& vector_key)
{
	const auto& keys = vector_map.at(category).at(vector_key);
	return SmiCliArgumentFactory::getInstance().get_arguments_string(keys);
}

std::string concat_usage_from_factory(const
									  std::map<std::string, std::map<std::string, std::vector<std::string>>>& vector_map,
									  const std::string& category,
									  const std::string& vector_key,
									  int& row_count)
{
	const auto& keys = vector_map.at(category).at(vector_key);

	std::string result;

	if (is_first_group_in_sequence && !keys.empty()) {
		result += USAGE_LINE_BREAK;
	} else if (row_count == 0 && !is_first_group_in_sequence && !keys.empty()) {
		result += USAGE_LINE_BREAK;
	}

	result += SmiCliArgumentFactory::getInstance().get_usage_string_with_formatting(keys, row_count);

	row_count = (row_count + static_cast<int>(keys.size())) % USAGE_COLUMNS_PER_ROW_SPECIFIC;

	is_first_group_in_sequence = false;

	return result;
}

std::string get_vf_usage_smart(int specific_row_count)
{
	std::string result{};
	const auto* vf_arg = SmiCliArgumentFactory::getInstance().get_argument("vf_device");
	if (vf_arg) {
		if (specific_row_count == 0) {
			result.append(USAGE_LINE_BREAK);
			result.append(" ").append(vf_arg->get_usage());
		} else if (specific_row_count <= (USAGE_COLUMNS_PER_ROW_SPECIFIC - 1)) {
			result.append(" ").append(vf_arg->get_usage());
		} else {
			result.append(USAGE_LINE_BREAK);
			result.append(" ").append(vf_arg->get_usage());
		}
	}
	return result;
}

struct UsageConfig {
	bool include_vf = false;
};

std::string build_usage_from_categories(const
										std::map<std::string, std::map<std::string, std::vector<std::string>>>& vector_map,
										const std::vector<std::pair<std::string, std::string>>& categories,
										const UsageConfig& config = {})
{
	reset_group_sequence();
	int row_count = 0;
	std::string result;

	for (const auto& [category, vector_key] : categories) {
		result += concat_usage_from_factory(vector_map, category, vector_key, row_count);
	}

	if (config.include_vf) {
		result += get_vf_usage_smart(row_count);
	}

	return result;
}

std::string build_help_commands_from_categories(const
		std::map<std::string, std::map<std::string, std::vector<std::string>>>& vector_map,
		const std::vector<std::pair<std::string, std::string>>& categories)
{
	std::string result;
	for (const auto& [category, vector_key] : categories) {
		result += get_help_commands_from_factory(vector_map, category, vector_key);
	}
	return result;
}

// Helper function to build arguments strings from multiple categories
std::string build_arguments_from_categories(const
		std::map<std::string, std::map<std::string, std::vector<std::string>>>& vector_map,
		const std::vector<std::pair<std::string, std::string>>& categories)
{
	std::string result;
	for (const auto& [category, vector_key] : categories) {
		result += concat_arguments_from_factory(vector_map, category, vector_key);
	}
	return result;
}

struct CommandConfig {
	bool include_vf = false;
	bool include_gpu_device = false;
	bool include_nic_device = false;
	bool include_watch_device = false;
	std::vector<std::string> format_keys = {"json", "csv"};
	std::string common_prefix = "";
};

void configure_list_settings(std::string& usage_list_specific, std::string& list_specific,
							 const CommandConfig& config = {})
{

	list_specific = config.common_prefix;
	list_specific += get_help_arguments();
	if (config.include_gpu_device) {
		list_specific += get_device_arguments("gpu");
	}
	if (config.include_nic_device) {
		list_specific += get_device_arguments("nic");
	}
}
void configure_static_settings(const
							   std::map<std::string, std::map<std::string, std::vector<std::string>>>& static_map,
							   const std::vector<std::pair<std::string, std::string>>& static_categories,
							   std::string& usage_static_specific,
							   std::string& static_specific,
							   const CommandConfig& config = {})
{

	usage_static_specific = build_usage_from_categories(static_map, static_categories, {.include_vf = config.include_vf});

	static_specific = config.common_prefix;
	static_specific += get_help_arguments();
	if (config.include_gpu_device) {
		static_specific += get_device_arguments("gpu");
	}
	if (config.include_nic_device) {
		static_specific += get_device_arguments("nic");
	}
	static_specific += build_arguments_from_categories(static_map, static_categories);
}

void configure_metric_settings(const
							   std::map<std::string, std::map<std::string, std::vector<std::string>>>& metric_map,
							   const std::vector<std::pair<std::string, std::string>>& metric_categories,
							   const std::vector<std::pair<std::string, std::string>>& vf_categories,
							   std::string& usage_metric_specific,
							   std::string& metric_specific,
							   const CommandConfig& config = {})
{

	std::string usage_result;
	if (config.include_watch_device) {
		usage_result += get_device_usage("watch");
	}
	usage_result += build_usage_from_categories(metric_map, metric_categories, {.include_vf = config.include_vf});
	usage_metric_specific = usage_result;

	metric_specific = config.common_prefix;
	metric_specific += get_help_arguments();
	if (config.include_gpu_device) {
		metric_specific += get_device_arguments("gpu");
	}
	if (config.include_nic_device) {
		metric_specific += get_device_arguments("nic");
	}
	if (config.include_watch_device) {
		metric_specific += get_device_arguments("watch");
	}
	metric_specific += build_arguments_from_categories(metric_map, metric_categories);
	if (config.include_vf) {
		metric_specific += build_arguments_from_categories(metric_map, vf_categories);
	}
}

void configure_topology_settings(const
								std::map<std::string, std::map<std::string, std::vector<std::string>>>& topology_map,
								const std::vector<std::pair<std::string, std::string>>& topology_categories,
								std::string& usage_topology_specific,
								std::string& topology_specific,
								const CommandConfig& config = {})
{
	usage_topology_specific = build_usage_from_categories(topology_map, topology_categories);

	topology_specific = config.common_prefix;
	topology_specific += get_help_arguments();
	if (config.include_gpu_device) {
		topology_specific += get_device_arguments("gpu");
	}
	if (config.include_nic_device) {
		topology_specific += get_device_arguments("nic");
	}
	topology_specific += build_arguments_from_categories(topology_map, topology_categories);
}
}

AmdSmiHelpInfo::AmdSmiHelpInfo(Arguments arg)
{
	SmiCliArgumentFactory::getInstance().initialize(arg.options);

	if (AmdSmiPlatform::getInstance().is_windows()) {
		initialize_windows_platform(arg);
	} else {
		initialize_linux_platform(arg);
	}
}

void AmdSmiHelpInfo::initialize_windows_platform(const Arguments& arg)
{
	if (AmdSmiPlatform::getInstance().is_host()) {
		set_common_windows_host_settings();
		if (AmdSmiPlatform::getInstance().is_mixxx()) {
			configure_windows_host_mixxx(arg);
		} else if (AmdSmiPlatform::getInstance().is_mi300()) {
			configure_windows_host_mi3xx(arg);
		} else {
			configure_windows_host_standard(arg);
		}
	} else if (AmdSmiPlatform::getInstance().is_baremetal()) {
		configure_windows_baremetal(arg);
	} else {
		configure_windows_guest(arg);
	}
}

void AmdSmiHelpInfo::initialize_linux_platform(const Arguments& arg)
{
	if (AmdSmiPlatform::getInstance().is_host()) {
		set_common_linux_host_settings();
		if (AmdSmiPlatform::getInstance().is_mixxx()) {
			configure_linux_host_mixxx(arg);
		} else if (AmdSmiPlatform::getInstance().is_mi300()) {
			configure_linux_host_mi300(arg);
		} else if (AmdSmiPlatform::getInstance().is_mi350()) {
			configure_linux_host_mi350(arg);
		} else if (AmdSmiPlatform::getInstance().is_mi200()) {
			configure_linux_host_mi200(arg);
		} else {
			configure_linux_host_standard(arg);
		}
	}
}

void AmdSmiHelpInfo::configure_linux_host_standard(const Arguments& arg)
{
	configure_device_specific_settings(arg);
	set_specific = set_host;
	usage_set_specific = set_usage_host;
	reset_specific = reset_host_linux;
	usage_reset_specific = reset_usage_linux;
	set_empty_settings();
}

void AmdSmiHelpInfo::configure_device_specific_settings(const Arguments& arg)
{
	switch (arg.devices_type) {
	case GPU_TYPE:
		{
		CommandConfig listConfig = {};
		listConfig.include_gpu_device = true;
		listConfig.common_prefix = list_common;
		CommandConfig staticConfig = {};
		staticConfig.include_gpu_device = true;
		staticConfig.common_prefix = static_common;
		CommandConfig metricConfig = {};
		metricConfig.include_vf = true;
		metricConfig.include_gpu_device = true;
		metricConfig.include_watch_device = true;
		metricConfig.common_prefix = metric_common;
		configure_list_settings(
			usage_list_specific, list_specific,
			listConfig
		);
		help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "linux_host"}, {"gpu", "host_spec"}});

		configure_static_settings(
			static_argument_vectors_map,
		{{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"gpu", "common"}, {"gpu", "host_linux"}, {"gpu", "host_linux_spec"}},
		usage_static_specific, static_specific,
		staticConfig
		);

		configure_metric_settings(
			metric_argument_vectors_map,
		{{"gpu", "common"}, {"gpu", "host"}, {"gpu", "host_linux_spec"}},
		{},
		usage_metric_specific, metric_specific,
		metricConfig
		);
		}
		break;
	case NIC_TYPE:
		{
		CommandConfig listConfig = {};
		listConfig.include_nic_device = true;
		listConfig.format_keys = {"json"};
		listConfig.common_prefix = list_common;
		CommandConfig staticConfig = {};
		staticConfig.include_nic_device = true;
		staticConfig.format_keys = {"json"};
		staticConfig.common_prefix = static_common;
		CommandConfig metricConfig = {};
		metricConfig.include_nic_device = true;
		metricConfig.format_keys = {"json"};
		metricConfig.common_prefix = metric_common;
		configure_list_settings(
			usage_list_specific, list_specific,
		listConfig
		);
		help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}});

		configure_static_settings(
			static_argument_vectors_map,
		{{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"nic", "host_linux"}},
		usage_static_specific, static_specific,
		staticConfig
		);

		configure_metric_settings(
			metric_argument_vectors_map,
		{{"nic", "host_linux"}}, {},
		usage_metric_specific, metric_specific,
		metricConfig
		);
		}
		break;
	case ALL_TYPE:
		{
		CommandConfig listConfig = {};
		listConfig.include_gpu_device = true;
		listConfig.include_nic_device = true;
		listConfig.common_prefix = list_common;
		configure_list_settings(
			usage_list_specific, list_specific,
		listConfig
		);

		help_specific = build_help_commands_from_categories(help_supported_command_map, {
			{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "linux_host"},  {"gpu", "host_spec"}
		});

		usage_static_specific = build_usage_from_categories(static_argument_vectors_map, {
			{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"gpu", "common"},
			{"gpu", "host_linux"}, {"gpu", "host_linux_spec"}, {"nic", "host_linux"}
		});

		static_specific = static_common + get_help_arguments() +
		build_arguments_from_categories(static_argument_vectors_map, {
			{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"},
		}) +
		common_gpu + get_device_arguments("gpu") +
		build_arguments_from_categories(static_argument_vectors_map, {
			{"gpu", "common"},{"gpu", "host_linux"}, {"gpu", "host_linux_spec"}
		}) + common_nic + get_device_arguments("nic") +
		build_arguments_from_categories(static_argument_vectors_map, {{"nic", "host_linux"}});

		usage_metric_specific = get_device_usage("watch") +
		build_usage_from_categories(metric_argument_vectors_map, {
			{"gpu", "common"}, {"gpu", "host"}, {"gpu", "host_linux_spec"}, {"nic", "host_linux"}
		});

		metric_specific = metric_common + get_help_arguments() + common_gpu + get_device_arguments("gpu") +
						  get_device_arguments("watch") +
		build_arguments_from_categories(metric_argument_vectors_map, {
			{"gpu", "common"}, {"gpu", "host"}, {"gpu", "host_linux_spec"}
		}) +
		common_nic + get_device_arguments("nic") +
		build_arguments_from_categories(metric_argument_vectors_map, {{"nic", "host_linux"}});
		}
		break;
	default:
		break;
	}
}

void AmdSmiHelpInfo::configure_windows_host_mi3xx(const Arguments& arg)
{
	CommandConfig staticConfig = {};
	staticConfig.include_gpu_device = true;
	CommandConfig topologyConfig = {};
	topologyConfig.include_gpu_device = true;
	topologyConfig.common_prefix = topology_arguments_header;

	help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "windows_host"}, {"gpu", "host_mi3xx"}});

	configure_static_settings(
		static_argument_vectors_map,
	{{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "host_windows"}, {"gpu", "host_mi3xx"}, {"gpu", "host_vf"}, {"gpu", "host_vf_mixxx"}},
	usage_static_specific, static_specific,
	staticConfig
	);

	configure_topology_settings(
		topology_argument_vectors_map,
	{{"gpu_nic_common", "host"}, {"gpu", "host"}},
	usage_topology_specific, topology_specific,
	topologyConfig
	);

	xgmi_specific = xgmi_host;
	usage_xgmi_specific = xgmi_usage_host;
	set_specific = set_host_mi300;
	usage_set_specific = set_usage_host_mi300;
	partition_specific = partition_host;
	usage_partition_specific = partition_usage_host;
	ras_specific = ras_host;
	usage_ras_specific = usage_ras_host;

	reset_specific = "";
	usage_reset_specific = "";
}

void AmdSmiHelpInfo::configure_windows_host_mixxx(const Arguments& arg)
{

	help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "windows_host"}, {"gpu", "host_mixxx"}});

	configure_static_settings(
		static_argument_vectors_map,
	{{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "host_windows"}, {"gpu", "host_mi3xx"}, {"gpu", "host_vf"}},
	usage_static_specific, static_specific,
	{.include_gpu_device = true}
	);

	xgmi_specific = xgmi_host;
	usage_xgmi_specific = xgmi_usage_host;
	topology_specific = topology_host;
	usage_topology_specific = topology_usage_host;
	fabric_specific = fabric_host;
	usage_fabric_specific = fabric_usage_host;
	set_specific = set_host_mixxx;
	usage_set_specific = set_usage_host_mixxx;
	partition_specific = partition_host;
	usage_partition_specific = partition_usage_host;
	ras_specific = ras_host;
	usage_ras_specific = usage_ras_host;
	confidential_compute_specific = confidential_compute_host;
	usage_confidential_compute_specific = confidential_compute_usage_host;

	reset_specific = "";
	usage_reset_specific = "";
}

void AmdSmiHelpInfo::configure_windows_host_standard(const Arguments& arg)
{
	CommandConfig staticConfig = {};
	staticConfig.include_gpu_device = true;

	help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "windows_host"}});

	configure_static_settings(
		static_argument_vectors_map,
	{{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "host_windows"}},
	usage_static_specific, static_specific,
	staticConfig
	);
	set_specific = "";
	usage_set_specific = "";
	reset_specific = "";
	usage_reset_specific = "";
	set_empty_settings();
}

void AmdSmiHelpInfo::configure_windows_baremetal(const Arguments& arg)
{
	CommandConfig listConfig = {};
	listConfig.include_gpu_device = true;
	listConfig.common_prefix = list_common;
	CommandConfig staticConfig = {};
	staticConfig.include_gpu_device = true;
	CommandConfig metricConfig = {};
	metricConfig.include_gpu_device = true;
	metricConfig.include_watch_device = true;

	configure_list_settings(
		usage_list_specific, list_specific,
	listConfig
	);
	help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "bm"}});

	configure_static_settings(
		static_argument_vectors_map,
	{{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "bm"}},
	usage_static_specific, static_specific,
	staticConfig
	);

	configure_metric_settings(
		metric_argument_vectors_map,
	{{"gpu", "common"}, {"gpu", "bm"}}, {},
	usage_metric_specific, metric_specific,
	metricConfig
	);

	bad_pages_specific = "";
	usage_bad_pages_specific = "";
	firmware_specific = firmware_bm;
	usage_firmware_specific = firmware_usage_bm;
	process_specific = process_bm;
	usage_process_specific = process_usage_bm;
	set_specific = set_bm;
	usage_set_specific = set_usage_bm;
	reset_specific = reset_bm;
	usage_reset_specific = reset_usage_bm;

	profile_specific = "";
	usage_profile_specific = "";
	event_specific = "";
	xgmi_specific = "";
	usage_xgmi_specific = "";
	topology_specific = "";
	usage_topology_specific = "";
	monitor_specific = monitor_bm;
	usage_monitor_specific = monitor_usage_bm;
}

void AmdSmiHelpInfo::configure_windows_guest(const Arguments& arg)
{
	CommandConfig listConfig = {};
	listConfig.include_gpu_device = true;
	listConfig.common_prefix = list_common;
	CommandConfig staticConfig = {};
	staticConfig.include_gpu_device = true;
	CommandConfig metricConfig = {};
	metricConfig.include_gpu_device = true;
	metricConfig.include_watch_device = true;

	configure_list_settings(
		usage_list_specific, list_specific,
	listConfig
	);
	help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "guest"}});

	configure_static_settings(
		static_argument_vectors_map,
	{{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "bm"}},
	usage_static_specific, static_specific,
	staticConfig
	);

	configure_metric_settings(
		metric_argument_vectors_map,
	{{"gpu", "common"}, {"gpu", "guest"}}, {},
	usage_metric_specific, metric_specific,
	metricConfig
	);

	bad_pages_specific = "";
	firmware_specific = "";
	usage_firmware_specific = "";
	process_specific = process_bm;
	usage_process_specific = process_usage_bm;
	set_specific = set_bm;
	usage_set_specific = set_usage_bm;
	reset_specific = reset_bm;
	usage_reset_specific = reset_usage_bm;

	profile_specific = "";
	usage_profile_specific = "";
	event_specific = "";
	usage_event_specific = "";
	xgmi_specific = "";
	usage_xgmi_specific = "";
	topology_specific = "";
	usage_topology_specific = "";
	monitor_specific = monitor_guest;
	usage_monitor_specific = monitor_usage_guest;
}

void AmdSmiHelpInfo::set_common_windows_host_settings()
{
	CommandConfig listConfig = {};
	listConfig.include_gpu_device = true;
	listConfig.common_prefix = list_common;
	CommandConfig metricConfig = {};
	metricConfig.include_vf = true;
	metricConfig.include_gpu_device = true;
	metricConfig.include_watch_device = true;

	configure_list_settings(
		usage_list_specific, list_specific,
	listConfig
	);
	configure_metric_settings(
		metric_argument_vectors_map,
	{{"gpu", "common"}, {"gpu", "host"},  {"gpu", "host_linux_spec"}, {"gpu", "host_vf"}},
	{{"vf", "host_vf"}},
	usage_metric_specific, metric_specific,
	metricConfig
	);

	bad_pages_specific = bad_pages_host;
	firmware_specific = firmware_host;
	usage_firmware_specific = firmware_usage_host;
	profile_specific = profile_host_windows;
	usage_profile_specific = profile_usage_host_windows;
	event_specific = event_host;
	usage_event_specific = event_usage_host;
	monitor_specific = monitor_host;
	usage_monitor_specific = monitor_usage_host;
	process_specific = "";
	usage_process_specific = "";
}

void AmdSmiHelpInfo::set_empty_settings()
{
	xgmi_specific = "";
	usage_xgmi_specific = "";
	topology_specific = "";
	usage_topology_specific = "";
	fabric_specific = "";
	usage_fabric_specific = "";
	partition_specific = "";
	usage_partition_specific = "";
	ras_specific = "";
	usage_ras_specific = "";
}

std::string AmdSmiHelpInfo::append_device_usage_by_type(const Arguments& arg)
{
	std::string out{};
	out.append(get_help_usage());
	switch (arg.devices_type) {
	case GPU_TYPE:
		out.append(get_device_usage("gpu"));
		break;
	case NIC_TYPE:
		out.append(get_device_usage("nic"));
		break;
	case ALL_TYPE:
		out.append(get_device_usage("gpu"));
		out.append(USAGE_LINE_BREAK);
		out.append(get_device_usage("nic"));
		break;
	default:
		break;
	}
	out.append(get_file_usage());
	return out;
}

void AmdSmiHelpInfo::configure_linux_host_mi300(const Arguments& arg)
{
	switch (arg.devices_type) {
	case GPU_TYPE:
		{
		CommandConfig listConfig = {};
		listConfig.include_gpu_device = true;
		listConfig.common_prefix = list_common;
		CommandConfig staticConfig = {};
		staticConfig.include_gpu_device = true;
		staticConfig.common_prefix = static_common;
		CommandConfig metricConfig = {};
		metricConfig.include_vf = true;
		metricConfig.include_gpu_device = true;
		metricConfig.include_watch_device = true;
		metricConfig.common_prefix = metric_common;
		CommandConfig topologyConfig = {};
		topologyConfig.include_gpu_device = true;
		topologyConfig.common_prefix = topology_arguments_header;

		configure_list_settings(
			usage_list_specific, list_specific,
		listConfig
		);
		help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "linux_host"}, {"gpu", "host_mi3xx"}});

		configure_static_settings(
			static_argument_vectors_map,
		{{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"gpu", "common"}, {"gpu", "host_linux"}, {"gpu", "host_linux_spec"}, {"gpu", "host_mi3xx"}, {"gpu", "host_vf"}},
		usage_static_specific, static_specific,
		staticConfig
		);

		configure_metric_settings(
			metric_argument_vectors_map,
		{{"gpu", "common"}, {"gpu", "host"}, {"gpu", "host_linux_spec"}, {"gpu", "host_vf"}},
		{{"vf", "host_vf"}, {"vf", "host_linux_mi3xx_vf"}},
		usage_metric_specific, metric_specific,
		metricConfig
		);

		configure_topology_settings(
			topology_argument_vectors_map,
		{{"gpu_nic_common", "host"}, {"gpu", "host"}},
		usage_topology_specific, topology_specific,
		topologyConfig
		);
		}
		break;

	case NIC_TYPE:
		{
		CommandConfig listConfig = {};
		listConfig.include_nic_device = true;
		listConfig.common_prefix = list_common;
		CommandConfig staticConfig = {};
		staticConfig.include_nic_device = true;
		staticConfig.common_prefix = static_common;
		CommandConfig metricConfig = {};
		metricConfig.include_nic_device = true;
		metricConfig.common_prefix = metric_common;
		CommandConfig topologyConfig = {};
		topologyConfig.include_nic_device = true;
		topologyConfig.common_prefix = topology_arguments_header;
		configure_list_settings(
			usage_list_specific, list_specific,
		listConfig
		);
		help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"nic", "host_mi3xx"}});

		configure_static_settings(
			static_argument_vectors_map,
		{{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"nic", "host_linux"}},
		usage_static_specific, static_specific,
		staticConfig
		);

		configure_metric_settings(
			metric_argument_vectors_map,
		{{"nic", "host_linux"}}, {},
		usage_metric_specific, metric_specific,
		metricConfig
		);

		configure_topology_settings(
			topology_argument_vectors_map,
		{{"gpu_nic_common", "host"}, {"nic", "host_linux"}},
		usage_topology_specific, topology_specific,
		topologyConfig
		);
		}
		break;

	case ALL_TYPE:
		{
		CommandConfig listConfig = {};
		listConfig.include_gpu_device = true;
		listConfig.include_nic_device = true;
		listConfig.common_prefix = list_common;
		configure_list_settings(
			usage_list_specific, list_specific,
		listConfig
		);

		help_specific = build_help_commands_from_categories(help_supported_command_map, {
			{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "linux_host"}, {"gpu", "host_mi3xx"}
		});
	
		usage_static_specific = build_usage_from_categories(static_argument_vectors_map, {
			{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"gpu", "common"},
			{"gpu", "host_linux"}, {"gpu", "host_linux_spec"}, {"gpu", "host_mi3xx"}, {"nic", "host_linux"}, {"gpu", "host_vf"}
		});
	
		static_specific = static_common + get_help_arguments() +
		build_arguments_from_categories(static_argument_vectors_map, {
			{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}
		}) +
		common_gpu + get_device_arguments("gpu") +
		build_arguments_from_categories(static_argument_vectors_map, {
			{"gpu", "common"}, {"gpu", "host_linux"}, {"gpu", "host_linux_spec"}, {"gpu", "host_mi3xx"}, {"gpu", "host_vf"}
		}) + common_nic + get_device_arguments("nic") +
		build_arguments_from_categories(static_argument_vectors_map, {{"nic", "host_linux"}});

		usage_metric_specific = get_device_usage("watch") +
		build_usage_from_categories(metric_argument_vectors_map, {
			{"gpu", "common"}, {"gpu", "host"}, {"gpu", "host_linux_spec"}, {"nic", "host_linux"}, {"gpu", "host_vf"}
		});

		metric_specific = metric_common + get_help_arguments() + common_gpu + get_device_arguments("gpu") +
						  get_device_arguments("watch") +
		build_arguments_from_categories(metric_argument_vectors_map, {
			{"gpu", "common"}, {"gpu", "host"}, {"gpu", "host_linux_spec"}, {"gpu", "host_vf"}
		}) +
		build_arguments_from_categories(metric_argument_vectors_map, {
			{"vf", "host_vf"}, {"vf", "host_linux_mi3xx_vf"}
		}) +
		common_nic + get_device_arguments("nic") +
		build_arguments_from_categories(metric_argument_vectors_map, {{"nic", "host_linux"}});

		usage_topology_specific = build_usage_from_categories(topology_argument_vectors_map, {
			{"gpu_nic_common", "host"}, {"gpu", "host"}, {"nic", "host_linux"}
		});

		topology_specific = topology_arguments_header + get_help_arguments() +
		build_arguments_from_categories(topology_argument_vectors_map, {{"gpu_nic_common", "host"}}) +
		common_gpu + get_device_arguments("gpu") +
		build_arguments_from_categories(topology_argument_vectors_map, {{"gpu", "host"}}) +
		common_nic + get_device_arguments("nic") +
		build_arguments_from_categories(topology_argument_vectors_map, {{"nic", "host_linux"}});
		}
		break;

	default:
		break;
	}

	xgmi_specific = xgmi_host;
	usage_xgmi_specific = xgmi_usage_host;
	set_specific = set_host_mi300;
	usage_set_specific = set_usage_host_mi300;
	if(AmdSmiPlatform::getInstance().is_mi308()) {
		set_specific += set_host_mi308;
		usage_set_specific += set_usage_host_mi308;
	}
	reset_specific = reset_host_linux;
	usage_reset_specific = reset_usage_linux;
	partition_specific = partition_host;
	usage_partition_specific = partition_usage_host;
	ras_specific = ras_host;
	usage_ras_specific = usage_ras_host;
}

void AmdSmiHelpInfo::configure_linux_host_mi350(const Arguments& arg)
{
	configure_linux_host_mi300(arg);
	switch (arg.devices_type) {
	case GPU_TYPE:
	case ALL_TYPE:
		help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "linux_host"}, {"gpu", "host_mi3xx"}, {"gpu", "host_mi350"}});
		break;
	default:
		break;
	}

	if (AmdSmiPlatform::getInstance().is_esxi()) {
		set_specific = set_host_mi300_esxi;
		usage_set_specific = set_usage_host_mi300_esxi;
	}

	usage_node_specific = usage_node_mi350;
	node_specific = node_mi350;
}

void AmdSmiHelpInfo::configure_linux_host_mixxx(const Arguments& arg)
{
switch (arg.devices_type) {
case GPU_TYPE:
	{
	CommandConfig listConfig = {};
	listConfig.include_gpu_device = true;
	listConfig.common_prefix = list_common;
	CommandConfig staticConfig = {};
	staticConfig.include_gpu_device = true;
	staticConfig.common_prefix = static_common;
	CommandConfig metricConfig = {};
	metricConfig.include_vf = true;
	metricConfig.include_gpu_device = true;
	metricConfig.include_watch_device = true;
	metricConfig.common_prefix = metric_common;
	configure_list_settings(
		usage_list_specific, list_specific,
		listConfig
	);
	help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "linux_host"}, {"gpu", "host_mixxx"}});

	configure_static_settings(
		static_argument_vectors_map,
	{{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"gpu", "common"}, {"gpu", "host_linux"}, {"gpu", "host_linux_spec"}, {"gpu", "host_mi3xx"}, {"gpu", "host_vf"}, {"gpu", "host_vf_mixxx"}},
	usage_static_specific, static_specific,
	staticConfig
	);

	configure_metric_settings(
		metric_argument_vectors_map,
	{{"gpu", "common"}, {"gpu", "host"}, {"gpu", "host_linux_spec"}, {"gpu", "host_vf"}},
	{{"vf", "host_vf"}, {"vf", "host_linux_mi3xx_vf"}},
	usage_metric_specific, metric_specific,
	metricConfig
	);
	}
	break;

case NIC_TYPE:
	{
	CommandConfig listConfig = {};
	listConfig.include_nic_device = true;
	listConfig.common_prefix = list_common;
	CommandConfig staticConfig = {};
	staticConfig.include_nic_device = true;
	staticConfig.common_prefix = static_common;
	CommandConfig metricConfig = {};
	metricConfig.include_nic_device = true;
	metricConfig.common_prefix = metric_common;
	configure_list_settings(
		usage_list_specific, list_specific,
	listConfig
	);
	help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}});

	configure_static_settings(
		static_argument_vectors_map,
	{{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"nic", "host_linux"}},
	usage_static_specific, static_specific,
	staticConfig
	);

	configure_metric_settings(
		metric_argument_vectors_map,
	{{"nic", "host_linux"}}, {},
	usage_metric_specific, metric_specific,
	metricConfig
	);
	}
	break;

case ALL_TYPE:
	{
	CommandConfig listConfig = {};
	listConfig.include_gpu_device = true;
	listConfig.include_nic_device = true;
	listConfig.common_prefix = list_common;
	configure_list_settings(
		usage_list_specific, list_specific,
	listConfig
	);

	help_specific = build_help_commands_from_categories(help_supported_command_map, {
		{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "linux_host"}, {"gpu", "host_mixxx"}
	});

	usage_static_specific = build_usage_from_categories(static_argument_vectors_map, {
		{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"gpu", "common"},
		{"gpu", "host_linux"}, {"gpu", "host_linux_spec"}, {"gpu", "host_mi3xx"}, {"nic", "host_linux"}, {"gpu", "host_vf"}, {"gpu", "host_vf_mixxx"}
	});

	static_specific = static_common + get_help_arguments() +
	build_arguments_from_categories(static_argument_vectors_map, {
		{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}
	}) +
	common_gpu + get_device_arguments("gpu") +
	build_arguments_from_categories(static_argument_vectors_map, {
		{"gpu", "common"}, {"gpu", "host_linux"}, {"gpu", "host_linux_spec"}, {"gpu", "host_mi3xx"}, {"gpu", "host_vf"}, {"gpu", "host_vf_mixxx"}
	}) + common_nic + get_device_arguments("nic") +
	build_arguments_from_categories(static_argument_vectors_map, {{"nic", "host_linux"}});

	usage_metric_specific = get_device_usage("watch") +
	build_usage_from_categories(metric_argument_vectors_map, {
		{"gpu", "common"}, {"gpu", "host"}, {"gpu", "host_linux_spec"}, {"nic", "host_linux"}, {"gpu", "host_vf"}
	});

	metric_specific = metric_common + get_help_arguments() + common_gpu + get_device_arguments("gpu") +
					  get_device_arguments("watch") +
	build_arguments_from_categories(metric_argument_vectors_map, {
		{"gpu", "common"}, {"gpu", "host"}, {"gpu", "host_linux_spec"}, {"gpu", "host_vf"}
	}) +
	build_arguments_from_categories(metric_argument_vectors_map, {
		{"vf", "host_vf"}, {"vf", "host_linux_mi3xx_vf"}
	}) +
	common_nic + get_device_arguments("nic") +
	build_arguments_from_categories(metric_argument_vectors_map, {{"nic", "host_linux"}});
	}
	break;

	default:
		break;
	}

	xgmi_specific = xgmi_host;
	usage_xgmi_specific = xgmi_usage_host;
	topology_specific = topology_host;
	usage_topology_specific = topology_usage_host;
	fabric_specific = fabric_host;
	usage_fabric_specific = fabric_usage_host;
	set_specific = set_host_mixxx;
	usage_set_specific = set_usage_host_mixxx;
	reset_specific = reset_host_linux;
	usage_reset_specific = reset_usage_linux;
	partition_specific = partition_host;
	usage_partition_specific = partition_usage_host;
	ras_specific = ras_host;
	usage_ras_specific = usage_ras_host;
	usage_node_specific = usage_node;
	node_specific = node_common;
	confidential_compute_specific = confidential_compute_host;
	usage_confidential_compute_specific = confidential_compute_usage_host;
}

void AmdSmiHelpInfo::configure_linux_host_mi200(const Arguments& arg)
{
	CommandConfig listConfig = {};
	listConfig.include_gpu_device = true;
	listConfig.common_prefix = list_common;
	CommandConfig staticConfig = {};
	staticConfig.include_gpu_device = true;
	staticConfig.common_prefix = static_common;
	CommandConfig metricConfig = {};
	metricConfig.include_vf = true;
	metricConfig.include_gpu_device = true;
	metricConfig.include_watch_device = true;
	metricConfig.common_prefix = metric_common;
	CommandConfig topologyConfig = {};
	topologyConfig.include_gpu_device = true;
	topologyConfig.common_prefix = topology_arguments_header;

	configure_list_settings(
		usage_list_specific, list_specific,
		listConfig
	);
	help_specific = build_help_commands_from_categories(help_supported_command_map, {{"gpu_nic_common", "common"}, {"gpu", "common"}, {"gpu", "linux_host"}, {"gpu", "host_mi200"}});

	configure_static_settings(
		static_argument_vectors_map,
	{{"gpu_nic_common", "common"}, {"gpu_nic_common", "host_linux"}, {"gpu", "host_linux"}},
	usage_static_specific, static_specific,
	staticConfig
	);

	configure_metric_settings(
		metric_argument_vectors_map,
	{{"gpu", "common"}, {"gpu", "host"}},
	{},
	usage_metric_specific, metric_specific,
	metricConfig
	);

	configure_topology_settings(
		topology_argument_vectors_map,
	{{"gpu_nic_common", "host"}, {"gpu", "host"}},
	usage_topology_specific, topology_specific,
	topologyConfig
	);

	xgmi_specific = xgmi_host_mi200;
	usage_xgmi_specific = xgmi_usage_host_mi200;
	set_specific = set_host_mi200;
	usage_set_specific = set_usage_host_mi200;
	reset_specific = reset_host_linux;
	usage_reset_specific = reset_usage_linux;
	partition_specific = "";
	usage_partition_specific = "";
}

void AmdSmiHelpInfo::set_common_linux_host_settings()
{
	bad_pages_specific = bad_pages_host;
	firmware_specific = firmware_host_linux;
	usage_firmware_specific = firmware_usage_host_linux;
	event_specific = event_host;
	usage_event_specific = event_usage_host;
	monitor_specific = monitor_host;
	usage_monitor_specific = monitor_usage_host;

	process_specific = "";
	usage_process_specific = "";
	profile_specific = "";
	usage_profile_specific = "";
}

bool AmdSmiHelpInfo::is_command_supported(std::string command, bool modifiers, std::string common,
		std::string specific)
{
	if (modifiers && common.size() == 0 && specific.size() == 0) {
		throw SmiToolCommandNotSupportedException(command);
	}
	return true;
}
std::string AmdSmiHelpInfo::get_list_help_message(const Arguments& arg, bool modifiers = false)
{
	is_command_supported("list",modifiers,list_common,list_specific);
	return copyright_message + usage_list_common + append_device_usage_by_type(
			   arg) + get_format_usage_by_device_type(arg.devices_type) + usage_list_specific + "\n\n" +
		   list_usage_message
		   + list_specific + "\n" + command_modifiers;
}

std::string AmdSmiHelpInfo::get_static_help_message(const Arguments& arg, bool modifiers = false)
{
	is_command_supported("static",modifiers,static_common,static_specific);
	std::string result = copyright_message + usage_static_common + append_device_usage_by_type(
							 arg) + get_format_usage_by_device_type(arg.devices_type) + usage_static_specific;
	result.append("\n")
	.append(static_usage_message)
	.append(static_specific)
	.append("\n")
	.append(command_modifiers);
	return result;
}

std::string AmdSmiHelpInfo::get_bad_page_help_message(bool modifiers = false)
{
	is_command_supported("bad_paged",modifiers,bad_pages_common,bad_pages_specific);
	return copyright_message + usage_bad_pages_specific + bad_pages_message + bad_pages_common +
		   bad_pages_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_firmware_help_message(bool modifiers = false)
{
	is_command_supported("firmware",modifiers,firmware_common,firmware_specific);
	return copyright_message + firmware_usage_common + usage_firmware_specific + firmware_message +
		   firmware_common + firmware_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_metric_help_message(const Arguments& arg, bool modifiers = false)
{
	is_command_supported("metric",modifiers,metric_common,metric_specific);
	std::string result = copyright_message + usage_metric_common + append_device_usage_by_type(
							 arg) + get_format_usage_by_device_type(arg.devices_type) + usage_metric_specific;
	result.append("\n")
	.append(metric_message)
	.append(metric_specific)
	.append("\n")
	.append(metric_modifiers);
	return result;
}
std::string AmdSmiHelpInfo::get_process_help_message(bool modifiers = false)
{
	is_command_supported("process",modifiers,process_common,process_specific);
	return copyright_message + process_usage_common + usage_process_specific + process_message +
		   process_common + process_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_profile_help_message(bool modifiers = false)
{
	is_command_supported("profile",modifiers,profile_common,profile_specific);
	return copyright_message + profile_usage_common + usage_profile_specific + profile_message +
		   profile_common + profile_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_version_help_message(bool modifiers = false)
{
	is_command_supported("version",modifiers,version_common,version_specific);
	return copyright_message + version_common + version_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_event_help_message(bool modifiers = false)
{
	is_command_supported("event",modifiers,event_common,event_specific);
	return copyright_message + usage_event_specific + event_common + event_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_xgmi_help_message(bool modifiers = false)
{
	is_command_supported("xgmi",modifiers,xgmi_common,xgmi_specific);
	return copyright_message + xgmi_usage_common + usage_xgmi_specific + xgmi_message + xgmi_common +
		   xgmi_specific + xgmi_modifiers;
}
std::string AmdSmiHelpInfo::get_topology_help_message(const Arguments& arg, bool modifiers = false)
{
	is_command_supported("topology",modifiers,topology_common,topology_specific);
	std::string result = copyright_message + topology_usage_common + append_device_usage_by_type(arg);
	if (arg.devices_type != NIC_TYPE) {
		result.append(get_format_usage({"json"}));
	}
	result.append(usage_topology_specific);
	result.append("\n")
	.append(topology_message)
	.append(topology_specific)
	.append("\n")
	.append(xgmi_modifiers);
	return result;
}
std::string AmdSmiHelpInfo::get_fabric_help_message(bool modifiers = false)
{
	is_command_supported("fabric",modifiers,fabric_common,fabric_specific);
	return copyright_message + fabric_usage_common + usage_fabric_specific + fabric_message +
		   fabric_common + fabric_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_partition_help_message(bool modifiers = false)
{
	is_command_supported("partition",modifiers,partition_common,partition_specific);
	return copyright_message + partition_usage_common + usage_partition_specific + partition_message +
		   partition_common + partition_specific + partition_modifiers;
}
std::string AmdSmiHelpInfo::get_set_help_message(bool modifiers = false)
{
	is_command_supported("set",modifiers,set_common,set_specific);
	return copyright_message + set_usage_common + usage_set_specific + set_message + set_common +
		   set_specific;
}
std::string AmdSmiHelpInfo::get_reset_help_message(bool modifiers = false)
{
	is_command_supported("reset",modifiers,reset_common,reset_specific);
	return copyright_message + reset_usage_common + usage_reset_specific + reset_message + reset_common
		   + reset_specific;
}
std::string AmdSmiHelpInfo::get_monitor_help_message(bool modifiers = false)
{
	is_command_supported("monitor",modifiers, monitor_common,monitor_specific);
	return copyright_message + monitor_usage_common + usage_monitor_specific + monitor_message
		   + monitor_common + monitor_specific + command_modifiers;
}
std::string AmdSmiHelpInfo::get_help_message()
{
	std::string version{};
	std::string common{};
	version = string_format("%s version %s", AMDSMI_TOOL_NAME, AMDSMI_TOOL_VERSION_STRING);
	std::string message{string_format(help_common, version.c_str())};
	return copyright_message + message + help_specific + "\n";
}

std::string AmdSmiHelpInfo::get_ras_help_message(bool modifiers = false)
{
	is_command_supported("ras", modifiers, ras_common, ras_specific);
	return  copyright_message + usage_ras_common + usage_ras_specific + ras_usage_message +
			ras_common + ras_specific +  command_modifiers;
}

std::string AmdSmiHelpInfo::get_node_help_message(bool modifiers = false)
{
	is_command_supported("node", modifiers, node_common, node_specific);
	return copyright_message + usage_node_specific + node_specific + command_modifiers;
}

std::string AmdSmiHelpInfo::get_confidential_compute_help_message(bool modifiers = false)
{
	is_command_supported("confidential-compute", modifiers, confidential_compute_common, confidential_compute_specific);
	return copyright_message + usage_confidential_compute_specific + confidential_compute_message + confidential_compute_common + confidential_compute_specific + command_modifiers;
}
