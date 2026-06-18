/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include "smi_cli_parser.h"
#include "smi_cli_platform.h"

class AmdSmiHelpInfo
{
public:
	std::string help_specific{};
	std::string list_specific{};
	std::string version_specific{};
	std::string static_specific{};
	std::string bad_pages_specific{};
	std::string firmware_specific{};
	std::string metric_specific{};
	std::string process_specific{};
	std::string profile_specific{};
	std::string event_specific{};
	std::string xgmi_specific{};
	std::string topology_specific{};
	std::string fabric_specific{};
	std::string partition_specific{};
	std::string set_specific{};
	std::string reset_specific{};
	std::string monitor_specific{};
	std::string ras_specific{};
	std::string node_specific{};
	std::string confidential_compute_specific{};

	std::string usage_list_specific{};
	std::string usage_version_specific{};
	std::string usage_static_specific{};
	std::string usage_bad_pages_specific{};
	std::string usage_firmware_specific{};
	std::string usage_metric_specific{};
	std::string usage_process_specific{};
	std::string usage_profile_specific{};
	std::string usage_event_specific{};
	std::string usage_xgmi_specific{};
	std::string usage_topology_specific{};
	std::string usage_fabric_specific{};
	std::string usage_partition_specific{};
	std::string usage_set_specific{};
	std::string usage_reset_specific{};
	std::string usage_monitor_specific{};
	std::string usage_ras_specific{};
	std::string usage_node_specific{};
	std::string usage_confidential_compute_specific{};
	AmdSmiHelpInfo(Arguments arg);
	std::string get_help_message();
	bool is_command_supported(std::string command, bool modifiers, std::string common,
							  std::string specific);

private:
	// Helper methods for constructor
	void configure_device_specific_settings(const Arguments& arg);

	// Platform-specific initialization methods
	void initialize_windows_platform(const Arguments& arg);
	void initialize_linux_platform(const Arguments& arg);

	// Windows platform sub-methods
	void configure_windows_host_mixxx(const Arguments& arg);
	void configure_windows_host_mi3xx(const Arguments& arg);
	void configure_windows_host_standard(const Arguments& arg);
	void configure_windows_baremetal(const Arguments& arg);
	void configure_windows_guest(const Arguments& arg);

	// Common configuration helpers
	void set_common_windows_host_settings();
	void set_empty_settings();

	// Device type helper function
	std::string append_device_usage_by_type(const Arguments& arg);

	// Linux platform sub-methods
	void configure_linux_host_mixxx(const Arguments& arg);
	void configure_linux_host_mi300(const Arguments& arg);
	void configure_linux_host_mi350(const Arguments& arg);
	void configure_linux_host_mi200(const Arguments& arg);
	void configure_linux_host_standard(const Arguments& arg);
	void set_common_linux_host_settings();

public:
	std::string get_list_help_message(const Arguments& arg, bool modifiers);
	std::string get_static_help_message(const Arguments& arg, bool modifiers);
	std::string get_bad_page_help_message(bool modifiers);
	std::string get_firmware_help_message(bool modifiers);
	std::string get_metric_help_message(const Arguments& arg, bool modifiers);
	std::string get_process_help_message(bool modifiers);
	std::string get_profile_help_message(bool modifiers);
	std::string get_version_help_message(bool modifiers);
	std::string get_event_help_message(bool modifiers);
	std::string get_xgmi_help_message(bool modifiers);
	std::string get_fabric_help_message(bool modifiers);
	std::string get_topology_help_message(const Arguments& arg, bool modifiers);
	std::string get_partition_help_message(bool modifiers);
	std::string get_set_help_message(bool modifiers);
	std::string get_reset_help_message(bool modifiers);
	std::string get_monitor_help_message(bool modifiers);
	std::string get_ras_help_message(bool modifiers);
	std::string get_node_help_message(bool modifiers);
	std::string get_confidential_compute_help_message(bool modifiers);
};
