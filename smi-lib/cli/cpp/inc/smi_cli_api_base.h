/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include "smi_cli_api_interface.h"

#include "tabulate/tabulate.hpp"

class AmdSmiApiBase : public IAmdSmiApi
{
public:
	AmdSmiApiBase();
	virtual ~AmdSmiApiBase(void);

	virtual int amdsmi_get_bdf_from_gpu_index(uint64_t &processor_bdf, int index) override;
	virtual int amdsmi_get_bdf_from_uuid_or_bdf(uint64_t &processor_bdf, int &gpu_index,
			std::string device, int type) override;
	virtual int amdsmi_get_gpu_count(unsigned int &gpu_count) override;
	virtual int amdsmi_get_vf_tree(std::vector<std::map<std::string, std::string>> &out) override;
	virtual int amdsmi_get_error_message(int error_code, std::string& out) override;
	virtual int format_link_type(const int& link_type, std::string& out) override;
	virtual int format_link_status(const int& link_status, std::string& out) override;
	virtual int transform_ecc_correction_schema(uint32_t flag, std::vector<std::string>& out) override;
	virtual int transform_cache_properties(uint32_t initial_property,
										   std::vector<std::string>& properties) override;
	virtual int get_string_from_enum_fw_block(int fw_block, std::string& out) override;
	virtual int get_string_from_enum_ecc_blocks(int ecc_block, std::string& out) override;
	virtual int get_string_from_enum_vf_sched_state(int vf_state, std::string& out) override;
	virtual int get_string_from_enum_vf_guard_state(int vf_state, std::string& out) override;
	virtual int get_string_from_enum_vf_guard_type(int guard_type, std::string& out) override;
	virtual int get_string_from_enum_vram_type(int vram_type, std::string& out) override;
	virtual int get_string_from_enum_vram_vendor_type(int vram_vendor_type, std::string& out) override;
	virtual int get_string_from_enum_driver_model(int driver_model, std::string& out) override;
	virtual int get_string_from_enum_accelerator_partition_type(int partition_type,
			std::string& out) override;
	virtual int get_string_from_enum_cper_severity_mask(int severity_mask, std::string& out) override;
	virtual int get_string_from_enum_mp_setting(int mp_setting, std::string& out) override;
	virtual int get_string_from_enum_resource_type(int resource_type, std::string& out) override;
	virtual int initTopology(std::vector<std::shared_ptr<Device> > devices,
							 std::vector<std::string>& bdf_vector) override;
	virtual int csv_recursion(std::string& main_buffer,
							  const std::vector<std::vector<std::string>> &results) override;

	//SMI API
	virtual int amdsmi_get_asic_info_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_bus_info_command(uint64_t processor_bdf, Arguments arg,  std::string &out);
	virtual int amdsmi_get_vbios_info_command(uint64_t processor_bdf, Arguments arg,  std::string &out);
	virtual int amdsmi_get_board_info_command(uint64_t processor_bdf, Arguments arg,  std::string &out);
	virtual int amdsmi_get_limit_info_command(uint64_t processor_bdf, Arguments arg,  std::string &out);
	virtual int amdsmi_get_driver_info_command(uint64_t processor_bdf, Arguments arg,
			std::string &out);
	virtual int amdsmi_get_ras_info_command(uint64_t processor_bdf, Arguments arg,  std::string &out);
	virtual int amdsmi_get_dfc_info_command(uint64_t processor_bdf, Arguments arg,  std::string &out);
	virtual int amdsmi_get_fb_info_command(uint64_t processor_bdf, Arguments arg,  std::string &out);
	virtual int amdsmi_get_num_vf_command(uint64_t processor_bdf, Arguments arg,  std::string &out);
	virtual int amdsmi_get_vram_info_command(uint64_t processor_bdf, Arguments arg,  std::string &out);
	virtual int amdsmi_get_cache_info_command(uint64_t processor_bdf, Arguments arg, std::string &out);
	virtual int amdsmi_get_vf_info_static_command(std::string device, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_process_isolation(uint64_t processor_bdf, Arguments arg,
			std::string &out) override;
	virtual int amdsmi_get_soc_pstate(uint64_t processor_bdf, Arguments arg, std::string &out);

	virtual int amdsmi_get_list_command(Arguments arg, std::string& out) override;
	virtual int amdsmi_get_bad_pages_command(uint64_t processor_bdf, Arguments arg, std::string& out,
			std::string* gpu_id=nullptr) override;
	virtual int amdsmi_get_static_partition_command(uint64_t processor_bdf, Arguments arg,
			std::string &out) override;

	//firmware
	virtual int amdsmi_firmware_fw_list_command(uint64_t processor_bdf, Arguments arg,
			std::string &out, std::string *gpu_id);
	virtual int amdsmi_firmware_err_rec_command(uint64_t processor_bdf, Arguments arg,
			std::string &out);
	virtual int amdsmi_firmware_vf_fw_list_command(std::string device, Arguments arg,
			std::string *gpu_id, std::string *vf_id, std::string &out) override;

	virtual int amdsmi_get_usage_metric_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_power_metric_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_clock_metric_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_temperature_metric_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_ecc_metric_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_ecc_block_metric_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_pcie_metric_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_xgmi_metric_command(Arguments arg, std::string& out) override;
	virtual int amdsmi_get_schedule_metric_command(std::string device, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_guard_metric_command(std::string device, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_guest_data_metric_command(std::string device, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_energy_metric_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;

	virtual int amdsmi_get_weight_topology_command(Arguments arg,
			std::vector<std::string> bdf_vector, std::string& out) override;
	virtual int amdsmi_get_hops_topology_command(Arguments arg,
			std::vector<std::string> bdf_vector, std::string& out) override;
	virtual int amdsmi_get_fb_sharing_topology_command(Arguments arg,
			std::vector<std::string> bdf_vector, std::string& out) override;
	virtual int amdsmi_get_link_type_topology_command(Arguments arg,
			std::vector<std::string> bdf_vector, std::string& out) override;
	virtual int amdsmi_get_link_status_topology_command(Arguments arg,
			std::vector<std::string> bdf_vector, std::string& out) override;

	virtual int amdsmi_get_all_topology_command(Arguments arg,
			std::vector<std::string> bdf_vector, std::string& out) override;


	virtual int amdsmi_get_caps_xgmi_command(Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_fb_sharing_xgmi_command(Arguments arg,
			std::string& out) override;
	virtual int amdsmi_set_fb_sharing_xgmi_command(Arguments arg,
			std::string& out) override;

	virtual int amdsmi_get_all_xgmi_command(Arguments arg,
											std::string& out) override;

	virtual int initEvent() override;
	virtual int amdsmi_get_event_command(Arguments arg, char &stop,
										 std::vector<std::thread> &threads) override;

	virtual int amdsmi_get_profile_command(uint64_t processor, Arguments arg, int gpu_index,
										   std::string &out_string) override;

	virtual int amdsmi_get_fb_usage_command(uint64_t processor_bdf, Arguments arg,
											std::string& out) override;

	static IAmdSmiApi& CreateAmdSmiApiObject();

	virtual int amdsmi_get_all_arguments_process_command(uint64_t processor, Arguments arg,
			std::string &out_string, int &proc_num, int gpu_id) override;
	virtual int amdsmi_get_general_process_command(uint64_t processor, Arguments arg,
			std::string &out_string, int &proc_num, int gpu_id) override;
	virtual int amdsmi_get_engine_process_command(uint64_t processor, Arguments arg,
			std::string &out_string, int &proc_num, int gpu_id) override;
	virtual int amdsmi_get_version_command(Arguments arg, std::string& out) override;
	virtual int amdsmi_reset_command(std::string vf_bdf, Arguments arg) override;
	virtual int amdsmi_set_process_isolation_command(uint64_t processor_bdf, Arguments arg) override;
	virtual int amdsmi_reset_local_data_command(uint64_t processor_bdf, Arguments arg) override;
	virtual int amdsmi_set_xgmi_fb_sharing_mode_command(std::vector<uint64_t> bfd_list,
			Arguments arg) override;

	virtual int amdsmi_get_power_usage_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_temperature_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_gfx_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_mem_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_encoder_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_decoder_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_ecc_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_vram_usage_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_pcie_info_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_process_monitor_command(Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_set_memory_partition_command(uint64_t processor_bdf, Arguments arg) override;
	virtual int amdsmi_set_accelerator_partition_command(uint64_t processor_bdf,
			Arguments arg) override;
	virtual int amdsmi_get_accelerator_partition_command(uint64_t processor_bdf, Arguments arg,
			std::vector<tabulate::Table::Row_t> &rows, std::vector<tabulate::Table::Row_t> &resource_rows,
			std::string &gpu_id) override;
	virtual int amdsmi_get_memory_partition_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_current_partition_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_set_soc_pstate_command(uint64_t processor_bdf, Arguments arg) override;
	virtual int amdsmi_set_power_cap_command(uint64_t processor_bdf, Arguments arg) override;

	virtual int amdsmi_get_cper_entries_command(Arguments arg,
			std::string &formatted_string) override;
};
