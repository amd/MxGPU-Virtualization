/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_cli_argument.h"
#include <sstream>
#include <iomanip>

SmiCliArgument::SmiCliArgument(const std::string& short_name,
							   const std::string& long_name,
							   const std::string& description,
							   const std::string& value_placeholder,
							   bool specific)
	: short_name_(short_name)
	, long_name_(long_name)
	, description_(description)
	, value_placeholder_(value_placeholder)
	, specific_(specific)
{
}

std::string SmiCliArgument::get_usage() const
{
	std::stringstream ss;
	ss << "[";

	if (!short_name_.empty()) {
		ss << short_name_;
	}

	if (!short_name_.empty() && !long_name_.empty()) {
		ss << " | ";
	}

	if (!long_name_.empty()) {
		ss << long_name_;
		if (!value_placeholder_.empty()) {
			ss << "=" << value_placeholder_;
		}
	}

	ss << "]";
	return ss.str();
}

std::string SmiCliArgument::get_arguments() const
{
	std::stringstream ss;

	if (specific_) {
		ss << std::string(SPECIFIC_ARG_INDENT, ' ');
	} else {
		ss << std::string(REGULAR_ARG_INDENT, ' ');
	}

	if (!short_name_.empty()) {
		ss << short_name_;
	}

	if (!short_name_.empty() && !long_name_.empty()) {
		ss << ", ";
	}

	if (!long_name_.empty()) {
		ss << long_name_;
		if (!value_placeholder_.empty()) {
			ss << "=" << value_placeholder_;
		}
	}

	std::string prefix = ss.str();
	int padding = DESCRIPTION_COLUMN_WIDTH - static_cast<int>(prefix.length());
	if (padding < 1) padding = 1;

	ss << std::string(padding, ' ') << description_ << "\n";

	return ss.str();
}

std::string SmiCliArgument::get_description_continuation_indent()
{
	return "\n" + std::string(DESCRIPTION_COLUMN_WIDTH, ' ');
}

std::string SmiCliArgument::get_usage_line_break()
{
	return "\n" + std::string(USAGE_LINE_BREAK_INDENT, ' ');
}

SmiCliHelpCommand::SmiCliHelpCommand(const std::string& command_name,
									 const std::string& description)
	: command_name_(command_name), description_(description)
{
}

std::string SmiCliHelpCommand::get_help_line() const
{
	std::stringstream ss;
	ss << std::string(SmiCliArgument::REGULAR_ARG_INDENT, ' ') << command_name_;

	int padding = COMMAND_COLUMN_WIDTH - static_cast<int>(command_name_.length());
	if (padding < 1) padding = 1;

	ss << std::string(padding, ' ') << description_ << "\n";
	return ss.str();
}

SmiCliArgumentFactory& SmiCliArgumentFactory::getInstance()
{
	static SmiCliArgumentFactory instance;
	return instance;
}

void SmiCliArgumentFactory::initialize(std::vector<std::string> options)
{
	for (const auto& option : options) {
		if (option == "static") {
			initialize_static_arguments();
			break;
		} else if (option == "metric") {
			initialize_metric_arguments();
			break;
		} else if (option == "topology") {
			initialize_topology_arguments();
			break;
		}
	}
	initialize_device_arguments();
	initialize_help_commands();
}

const SmiCliArgument* SmiCliArgumentFactory::get_argument(const std::string& key) const
{
	auto it = arguments_.find(key);
	return (it != arguments_.end()) ? &it->second : nullptr;
}

std::string SmiCliArgumentFactory::get_usage_string(const std::vector<std::string>& keys) const
{
	return get_usage_string_with_formatting(keys, 0);
}

std::string SmiCliArgumentFactory::get_usage_string_with_formatting(const std::vector<std::string>&
		keys, int starting_row_count) const
{
	std::stringstream ss;
	constexpr int USAGE_COLUMNS_PER_ROW_SPECIFIC = 4;
	const std::string USAGE_LINE_BREAK = SmiCliArgument::get_usage_line_break();

	int current_count = starting_row_count;

	for (size_t i = 0; i < keys.size(); ++i) {
		const auto* arg = get_argument(keys[i]);
		if (arg) {
			if (current_count > 0 && current_count % USAGE_COLUMNS_PER_ROW_SPECIFIC == 0) {
				ss << USAGE_LINE_BREAK;
			}

			ss << " " << arg->get_usage();
			current_count++;
		}
	}
	return ss.str();
}

std::string SmiCliArgumentFactory::get_arguments_string(const std::vector<std::string>& keys) const
{
	std::stringstream ss;
	for (const auto& key : keys) {
		const auto* arg = get_argument(key);
		if (arg) {
			ss << arg->get_arguments();
		}
	}
	return ss.str();
}

const SmiCliHelpCommand* SmiCliArgumentFactory::get_help_command(const std::string& command_name)
const
{
	auto it = help_commands_.find(command_name);
	return (it != help_commands_.end()) ? &it->second : nullptr;
}

std::string SmiCliArgumentFactory::get_help_commands_string(const std::vector<std::string>&
		command_names) const
{
	std::stringstream ss;
	for (const auto& command_name : command_names) {
		const auto* cmd = get_help_command(command_name);
		if (cmd) {
			ss << cmd->get_help_line();
		}
	}
	return ss.str();
}

std::string SmiCliArgumentFactory::get_format_usage(const std::vector<std::string>& format_keys)
const
{
	std::vector<std::string> formats;
	for (const auto& key : format_keys) {
		const auto* arg = get_argument(key);
		if (arg) {
			formats.push_back(arg->get_long_name());
		}
	}

	if (formats.empty()) {
		return "";
	} else if (formats.size() == 1) {
		return " [" + formats[0] + "]";
	} else {
		std::string result = " [";
		for (size_t i = 0; i < formats.size(); ++i) {
			if (i > 0) result += " | ";
			result += formats[i];
		}
		result += "]";
		return result;
	}
}

std::string SmiCliArgumentFactory::get_format_arguments(const std::vector<std::string>& format_keys)
const
{
	std::string result;
	for (const auto& key : format_keys) {
		const auto* arg = get_argument(key);
		if (arg) {
			result += arg->get_arguments();
		}
	}
	return result;
}

void SmiCliArgumentFactory::initialize_static_arguments()
{
	arguments_.emplace("asic", SmiCliArgument("-a", "--asic", "All asic information"));
	arguments_.emplace("bus", SmiCliArgument("-b", "--bus", "All bus information"));
	arguments_.emplace("driver", SmiCliArgument("-d", "--driver", "Displays driver version"));
	arguments_.emplace("numa", SmiCliArgument("-u", "--numa", "All numa information"));
	arguments_.emplace("ifwi", SmiCliArgument("-I", "--ifwi", "All video bios/IFWI information"));
	arguments_.emplace("board", SmiCliArgument("-B", "--board", "All board information"));
	arguments_.emplace("limit", SmiCliArgument("-l", "--limit",
					   "All limit metric values (i.e. power and thermal limits)"));
	arguments_.emplace("ras", SmiCliArgument("-r", "--ras", "Displays ras features information"));
	arguments_.emplace("dfc-ucode", SmiCliArgument("-D", "--dfc-ucode",
					   "All dfc ucode table information"));
	arguments_.emplace("fb-info", SmiCliArgument("-f", "--fb-info", "All fb information (GPU)"));
	arguments_.emplace("hbm-info", SmiCliArgument("-hbm", "--hbm-info", "All hbm information"));
	arguments_.emplace("vf-fb-info", SmiCliArgument("-f", "--fb-info", "All fb information (VF)"));
	arguments_.emplace("num-vf", SmiCliArgument("-nv", "--num-vf",
				   "Displays number of supported and enabled VFs"));
	arguments_.emplace("vram", SmiCliArgument("-v", "--vram", "All vram information"));
	arguments_.emplace("cache", SmiCliArgument("-c", "--cache", "All cache info"));
	arguments_.emplace("partition", SmiCliArgument("-p", "--partition",
					   "Gets current memory and accelerator partition information"));
	arguments_.emplace("process-isolation", SmiCliArgument("-R", "--process-isolation",
					   "The process isolation status"));
	arguments_.emplace("virtualization-mode", SmiCliArgument("-m", "--virtualization-mode",
					   "All virtualization mode info"));
	arguments_.emplace("xgmi-plpd", SmiCliArgument("-pd", "--xgmi-plpd", "Gets current xgmi plpd information"));
	arguments_.emplace("soc-pstate", SmiCliArgument("-ps", "--soc-pstate", "Gets current soc pstate information"));
	arguments_.emplace("port", SmiCliArgument("-po", "--port", "All port information"));
	arguments_.emplace("rdma-devices", SmiCliArgument("-rd", "--rdma-devices",
					   "All RDMA devices information"));
	arguments_.emplace("vf", SmiCliArgument("", "--vf",
				   "Gets general information about the specified VF" +
				   SmiCliArgument::get_description_continuation_indent() +
				   "<gpu_index:vf_index | vf_bdf | vf_uuid>"));
}

void SmiCliArgumentFactory::initialize_metric_arguments()
{
	arguments_.emplace("usage", SmiCliArgument("-u", "--usage", "All usage information"));
	arguments_.emplace("power", SmiCliArgument("-p", "--power", "All power readings information"));
	arguments_.emplace("clock", SmiCliArgument("-c", "--clock", "All frequency sensor readings"));
	arguments_.emplace("temperature", SmiCliArgument("-t", "--temperature",
					   "All thermal sensor readings"));
	arguments_.emplace("ecc", SmiCliArgument("-e", "--ecc", "All ecc information"));
	arguments_.emplace("ecc-block", SmiCliArgument("-k", "--ecc-block",
					   "Number of ECC errors per block"));
	arguments_.emplace("pcie", SmiCliArgument("-P", "--pcie", "Current pcie information"));
	arguments_.emplace("fb-usage", SmiCliArgument("-fb", "--fb-usage", "Total and used framebuffer"));
	arguments_.emplace("energy", SmiCliArgument("-E", "--energy", "Amount of energy consumed"));
	arguments_.emplace("throttle", SmiCliArgument("-th", "--throttle", "Displays throttle accumulators"));
	arguments_.emplace("port", SmiCliArgument("-po", "--port",
					   "All port information (NIC vendor statistics are limited by default; "
					   "use --extended / -ex for the extended set)"));
	arguments_.emplace("rdma-devices", SmiCliArgument("-rd", "--rdma-devices",
					   "All RDMA devices information"));
	arguments_.emplace("extended", SmiCliArgument("-ex", "--extended",
					   "Show the extended NIC vendor statistics set "
					   "(default output is limited to a curated subset)"));
	arguments_.emplace("vf", SmiCliArgument("", "--vf",
					   "Gets metric information about the specified VF" +
					   SmiCliArgument::get_description_continuation_indent() +
					   "If no metric information argument is provided all metric information will be displayed",
					   "<gpu_index:vf_index | vf_bdf | vf_uuid>"));

	arguments_.emplace("schedule", SmiCliArgument("-s", "--schedule", "All scheduling info", "", true));
	arguments_.emplace("guard", SmiCliArgument("-G", "--guard", "All guard information", "", true));
	arguments_.emplace("guest-data", SmiCliArgument("-u", "--guest-data", "All guest data information",
					   "", true));
	arguments_.emplace("per-partition", SmiCliArgument("-pp", "--per-partition",
					   "All metric per partition information", "", true));

	arguments_.emplace("watch_time", SmiCliArgument("-W", "--watch_time",
					   "The total TIME to watch the given command" + SmiCliArgument::get_description_continuation_indent()
					   +
					   "Looping stops by entering 'CTRL' + 'C'" + SmiCliArgument::get_description_continuation_indent() +
					   "If not specified the program will run indefinitely",
					   "TIME", true));
	arguments_.emplace("iterations", SmiCliArgument("-i", "--iterations",
					   "Total number of ITERATIONS to loop on the given command" +
					   SmiCliArgument::get_description_continuation_indent() +
					   "Looping stops by entering 'CTRL' + 'C'" + SmiCliArgument::get_description_continuation_indent() +
					   "If not specified the program will run indefinitely",
					   "ITERATIONS", true));
}

void SmiCliArgumentFactory::initialize_topology_arguments()
{
	arguments_.emplace("weight", SmiCliArgument("", "--weight", "Current weight information"));
	arguments_.emplace("hops", SmiCliArgument("", "--hops", "Current hops information"));
	arguments_.emplace("fb-sharing", SmiCliArgument("", "--fb-sharing",
					   "Current framebuffer sharing information"));
	arguments_.emplace("link-type", SmiCliArgument("", "--link-type", "Link type information"));
	arguments_.emplace("coherent", SmiCliArgument("", "--coherent", "Cache coherent information"));
	arguments_.emplace("atomics", SmiCliArgument("", "--atomics",
					   "32 and 64-bit atomic link capability information"));
	arguments_.emplace("bi-dir", SmiCliArgument("", "--bi-dir",
					   "bi-directional link capability information"));
	arguments_.emplace("dma", SmiCliArgument("", "--dma", "dma link capability information"));
	arguments_.emplace("numa", SmiCliArgument("", "--numa",
					   "NUMA node information"));
	arguments_.emplace("sort", SmiCliArgument("", "--sort",
					   "Sort GPUs in the output by physical id or PCIe BDF (default)",
					   "phy_id|bdf"));
}

void SmiCliArgumentFactory::initialize_device_arguments()
{
	arguments_.emplace("gpu_device", SmiCliArgument("-g", "--gpu",
					   "Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs",
					   "<gpu_index | gpu_bdf | gpu_uuid>"));
	arguments_.emplace("nic_device", SmiCliArgument("-n", "--nic",
					   "Select a NIC ID or BDF, if not selected it will return for all NICs", "<nic_index | nic_bdf>"));
	arguments_.emplace("vf_device", SmiCliArgument("-v", "--vf",
					   "Gets general information about the specified VF (e.g. timeslice, fb info)",
					   "<gpu_index:vf_index | vf_bdf | vf_uuid>"));
	arguments_.emplace("watch_device", SmiCliArgument("-w", "--watch",
					   "Reprint the command in a loop of INTERVAL seconds" +
					   SmiCliArgument::get_description_continuation_indent() +
					   "Looping stops by entering 'CTRL' + 'C'" + SmiCliArgument::get_description_continuation_indent() +
					   "JSON and CSV formats cannot be printed in stdout", "INTERVAL"));
	arguments_.emplace("help", SmiCliArgument("-h", "--help", "Show this help message and exit", ""));
	arguments_.emplace("file", SmiCliArgument("", "--file",
					   "Saves output into a file on the provided path (stdout by default)", "FILE"));
	arguments_.emplace("json", SmiCliArgument("", "--json",
					   "Displays output in JSON format (human readable by default)", ""));
	arguments_.emplace("csv", SmiCliArgument("", "--csv",
					   "Displays output in CSV format (human readable by default)", ""));
}

void SmiCliArgumentFactory::initialize_help_commands()
{
	help_commands_.emplace("version", SmiCliHelpCommand("version",
						   "Display version information (GPU only)"));
	help_commands_.emplace("list", SmiCliHelpCommand("list", "List device information"));
	help_commands_.emplace("static", SmiCliHelpCommand("static",
						   "Gets static information about the specified device"));
	help_commands_.emplace("metric", SmiCliHelpCommand("metric",
						   "Gets metric information about the specified device"));
	help_commands_.emplace("monitor", SmiCliHelpCommand("monitor",
						   "Monitor metrics for target devices (GPU only)"));
	help_commands_.emplace("bad-pages", SmiCliHelpCommand("bad-pages",
						   "Gets bad page information about the specified device (GPU only)"));
	help_commands_.emplace("event", SmiCliHelpCommand("event",
						   "Displays event information for the given device (GPU only)"));
	help_commands_.emplace("firmware", SmiCliHelpCommand("firmware",
						   "Gets firmware information about the specified device (GPU only)"));
	help_commands_.emplace("profile", SmiCliHelpCommand("profile",
						   "Displays information about all profiles and current profile (GPU only)"));
	help_commands_.emplace("process", SmiCliHelpCommand("process",
						   "Lists general process information running on the specified device (GPU only)"));
	help_commands_.emplace("set", SmiCliHelpCommand("set", "Set options for devices (GPU only)"));
	help_commands_.emplace("reset", SmiCliHelpCommand("reset", "Reset options for devices (GPU only)"));
	help_commands_.emplace("xgmi", SmiCliHelpCommand("xgmi",
						   "Displays xgmi information of the devices (GPU only)"));
	help_commands_.emplace("topology", SmiCliHelpCommand("topology",
					   "Displays topology information of the devices"));
	help_commands_.emplace("partition", SmiCliHelpCommand("partition",
						   "Displays partition information of the devices (GPU only)"));
	help_commands_.emplace("fabric", SmiCliHelpCommand("fabric",
						   "Displays fabric information of the devices (GPU only)"));
	help_commands_.emplace("ras", SmiCliHelpCommand("ras",
						   "Displays ras information of the devices (GPU only)"));
	help_commands_.emplace("node", SmiCliHelpCommand("node", "Displays node information of the devices (GPU only)"));
}

// Updates the description of the argument with the given key.
bool SmiCliArgumentFactory::update_argument_description(const std::string& key,
		const std::string& new_description)
{
	auto it = arguments_.find(key);
	if (it != arguments_.end()) {
		it->second.update_description(new_description);
		return true;
	}
	return false;
}
