/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"
#include "json/json.h"


class AmdSmiMetricCommand : public AmdSmiCommands
{
public:
	AmdSmiMetricCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();

	void metric_command_json();
	void metric_command_human();
	void metric_command_csv();
	void metric_command_watch();

	void metric_command_json_vf(nlohmann::ordered_json &json_format);
	void metric_command_json_gpu(int gpu_index, uint64_t gpu_bdf, nlohmann::ordered_json &json_format);
	void metric_command_json_nic(int nic_index, uint64_t nic_bdf, nlohmann::ordered_json &json_format);

	void metric_command_human_vf(std::string &out);
	void metric_command_human_gpu(int gpu_index, uint64_t gpu_bdf, std::string &out);
	void metric_command_human_nic(int nic_index, uint64_t nic_bdf, std::string &out);

	void metric_command_csv_vf(std::string &out);
	void metric_command_csv_gpu(int gpu_index, uint64_t gpu_bdf, std::string &headers, std::vector<std::vector<std::string>> &results);

	void metric_command_watch_format(std::vector<std::string> &first_row, std::vector<std::string> &second_row,
		std::vector<std::string> &third_row, std::vector<std::string> &fourth_row, std::vector<std::vector<std::string>> &value_rows, std::string &out);
	void metric_command_watch_vf(std::vector<std::string> &first_row, std::vector<std::string> &second_row,
		std::vector<std::string> &third_row, std::vector<std::string> &fourth_row);
	void metric_command_watch_gpu(int gpu_index, uint64_t gpu_bdf, int device_index, std::vector<std::string> &first_row, std::vector<std::string> &second_row,
					std::vector<std::string> &third_row, std::vector<std::string> &fourth_row);

	int metric_command_usage(uint64_t processor,
							 std::string &formatted_string);
	int metric_command_per_partition(uint64_t processor, uint64_t vf_index,
							 std::string &formatted_string);
	int metric_command_power(uint64_t processor,
							 std::string &formatted_string);
	int metric_command_clock(uint64_t processor,
							 std::string &formatted_string);
	int metric_command_temperature(uint64_t processor,
								   std::string &formatted_string);
	int metric_command_ecc(uint64_t processor,
						   std::string &formatted_string);
	int metric_command_ecc_block(uint64_t processor,
								 std::string &formatted_string);
	int metric_command_pcie(uint64_t processor,
							std::string &formatted_string);
	int metric_vf_command_schedule(std::string vf_handle,
								   std::string &formatted_string);
	int metric_vf_command_guard(std::string vf_handle,
								std::string &formatted_string);
	int metric_vf_command_guest_data(std::string vf_handle,
									 std::string &formatted_string);
	int metric_command_fb_usage(uint64_t processor,
								std::string &formatted_string);
	int metric_command_energy(uint64_t processor,
							  std::string &formatted_string);
	int metric_command_throttle(uint64_t processor,
								std::string &formatted_string);

	int metric_command_gpuboard(uint64_t processor,
							 std::string &formatted_string);
	int metric_command_port_netdev(uint64_t processor,
								std::string &formatted_string);
	int metric_command_rdma_devices(uint64_t processor,
								std::string &formatted_string);

private:
	bool is_vf_schedule = false;
	bool is_vf_guard_info = false;
	bool is_vf_guest_data = false;
	bool is_per_partition = false;

};
