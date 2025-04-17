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

#include "smi_cli_api_base.h"

class AmdSmiApiGuest : public AmdSmiApiBase
{
public:
	AmdSmiApiGuest ();
	virtual ~AmdSmiApiGuest (void);
	void* amdSmiLibHandle;

	virtual int amdsmi_get_bdf_from_gpu_index(uint64_t &processor_bdf, int index) override;
	virtual int amdsmi_get_bdf_from_uuid_or_bdf(uint64_t &processor_bdf, int &gpu_index,
			std::string device, int type) override;

	virtual int amdsmi_get_error_message(int error_code, std::string& out) override;
	virtual int get_string_from_enum_fw_block(int fw_block, std::string& out) override;

	int list_command(int format, unsigned int gpu_index, std::string bdf, char *uuid,
					 std::string &formatted_string);
	virtual int amdsmi_get_gpu_count(unsigned int &gpu_count) override;

	virtual int amdsmi_get_list_command(Arguments arg, std::string& out) override;
	virtual int csv_recursion(std::string& main_buffer,
							  const std::vector<std::vector<std::string>> &results) override;
	//static
	virtual int amdsmi_get_asic_info_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;
	virtual int amdsmi_get_vbios_info_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_driver_info_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_bus_info_command(uint64_t processor_bdf, Arguments arg,
											std::string &formatted_string) override;
	virtual int amdsmi_get_ras_info_command(uint64_t processor_bdf, Arguments arg,
											std::string &formatted_string) override;
	virtual int amdsmi_get_limit_info_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_process_isolation(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	//metric
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
	virtual int amdsmi_get_pcie_metric_command(uint64_t processor_bdf, Arguments arg,
			std::string& out) override;

	virtual int amdsmi_get_fb_usage_command(uint64_t processor_bdf, Arguments arg,
											std::string& out) override;

	virtual int amdsmi_firmware_fw_list_command(uint64_t processor_bdf, Arguments arg,
			std::string &out_string, std::string *gpu_id) override;

	//process
	virtual int amdsmi_get_all_arguments_process_command(uint64_t processor, Arguments arg,
			std::string &out_string, int &proc_num, int gpu_id) override;
	virtual int amdsmi_get_general_process_command(uint64_t processor, Arguments arg,
			std::string &out_string, int &proc_num, int gpu_id) override;
	virtual int amdsmi_get_engine_process_command(uint64_t processor, Arguments arg,
			std::string &out_string, int &proc_num, int gpu_id) override;

	virtual int amdsmi_get_version_command(Arguments arg, std::string &out_string) override;

	virtual int amdsmi_set_process_isolation_command(uint64_t processor_bdf, Arguments arg) override;
	virtual int amdsmi_reset_local_data_command(uint64_t processor_bdf, Arguments arg) override;

	// monitor
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
	virtual int amdsmi_get_ecc_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_vram_usage_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
	virtual int amdsmi_get_pcie_info_monitor_command(uint64_t processor_bdf, Arguments arg,
			std::string &formatted_string) override;
};
