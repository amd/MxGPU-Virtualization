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
#include <iostream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <map>

#include "smi_cli_set_command.h"
#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_static_command.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_api_base.h"
#include "smi_cli_platform.h"
#include "smi_cli_exception.h"

void AmdSmiSetCommand::set_command()
{
	int ret;

	for (unsigned int i = 0; i < arg.devices.size(); i++) {
		uint64_t gpu_bdf = arg.devices[i]->get_bdf();

		if ((std::find(arg.options.begin(), arg.options.end(),
					   "accelerator-partition") != arg.options.end())) {

			std::string param{"accelerator-partition"};
			if ((AmdSmiPlatform::getInstance().is_mi200())) {
				throw SmiToolParameterNotSupportedException(param);
			}

			if (i == 0) {
				std::cout << "Setting accelerator-partition in progress. This may take a while...\n" <<std::endl;
			}
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_accelerator_partition_command(gpu_bdf,
				  arg);
			if (ret == 1) {
				std::cout << "ACCELERATOR_PARTITION: Can't set accelerator partition to " <<
						  arg.accelerator_partition_setting
						  << ". Check amd-smi partition command for more information." << std::endl;
			}
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::cout << string_format(setSuccessfullyTemplate,
							   arg.devices[i]->get_gpu_index(),
							   "ACCELERATOR_PARTITION",
							   "accelerator partition",
							   string_format("%d", arg.accelerator_partition_setting).c_str());
			}
		}
		if ((std::find(arg.options.begin(), arg.options.end(), "memory-partition") != arg.options.end())) {

			std::string param{"memory-partition"};
			if ((AmdSmiPlatform::getInstance().is_mi200())) {
				throw SmiToolParameterNotSupportedException(param);
			}

			if (i == 0) {
				std::cout << "NOTE: This change could affect the current accelerator partition configuration. Check amd-smi partition command for more information." << std::endl;
				std::cout << "Setting memory-partition in progress. This may take a while...\n" << std::endl;
			}
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_memory_partition_command(gpu_bdf,
				  arg);
			if (ret == 1) {
				std::cout << "MEMORY_PARTITION: Can't set memory partition to " << arg.memory_partition_setting
						  << ". Check amd-smi partition command for more information." << std::endl;
			}
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::cout << string_format(setSuccessfullyTemplate,
							   arg.devices[i]->get_gpu_index(),
							   "MEMORY_PARTITION",
							   "memory partition",
							   (arg.memory_partition_setting).c_str());
			}

		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "xgmi") != arg.options.end())) {
		std::string param{"xgmi fb-sharing-mode"};
		int error;

		if ((!AmdSmiPlatform::getInstance().is_mi300() && !AmdSmiPlatform::getInstance().is_mi200())
				|| !AmdSmiPlatform::getInstance().is_host()) {
			throw SmiToolParameterNotSupportedException(param);
		}

		// custom mode
		for (auto bdf_list: arg.groups) {
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_xgmi_fb_sharing_mode_command(bdf_list, arg);
			error = handle_exceptions(ret, param, arg);
		}

		// auto mode
		if (arg.groups.size() == 0) {
			std::vector<std::size_t> bfd_list{};
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_xgmi_fb_sharing_mode_command(bfd_list, arg);
			error = handle_exceptions(ret, param, arg);
		}

		if (error == 0) {
			std::cout << "XGMI FB_SHARING_MODE: Successfully set mode to " << arg.fb_sharing_mode
				  << " for the given group/s" << std::endl;
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "process-isolation") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "R") != arg.options.end()) ||
			arg.all_arguments) {

		std::string param{"process-isolation"};
		if ((AmdSmiPlatform::getInstance().is_mi200())) {
			throw SmiToolParameterNotSupportedException(param);
		}

		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_process_isolation_command(gpu_bdf,
				  arg);
			std::string param{"process-isolation"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::string process_isolation_set_str = arg.process_isolation_set == "0" ? "Disable" : "Enable";
				std::cout << string_format(setSuccessfullyTemplate,
							   arg.devices[i]->get_gpu_index(),
							   "PROCESS_ISOLATION",
							   "process isolation",
							   process_isolation_set_str.c_str());
			}
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "soc-pstate") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "p") != arg.options.end()) ||
			arg.all_arguments) {

		std::string param{"soc-pstate"};
		if ((AmdSmiPlatform::getInstance().is_mi200())) {
			throw SmiToolParameterNotSupportedException(param);
		}

		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_soc_pstate_command(gpu_bdf,
				  arg);
			std::string param{"soc-pstate"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::cout << string_format(setSuccessfullyTemplate,
							   arg.devices[i]->get_gpu_index(),
							   "SOC_PSTATE",
							   "dpm soc pstate policy",
							   (arg.pstate_set).c_str());
			}
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "power-cap") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "o") != arg.options.end()) ||
			arg.all_arguments) {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_power_cap_command(gpu_bdf,
				  arg);
			std::string param{"power-cap"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::cout << string_format(setSuccessfullyTemplate,
							   arg.devices[i]->get_gpu_index(),
							   "POWER_CAP",
							   "new power cap value",
							   string_format("%d", arg.power_cap_set).c_str());
			}
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "xgmi-plpd") != arg.options.end()) ||
	(std::find(arg.options.begin(), arg.options.end(), "P") != arg.options.end())) {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_plpd_command(gpu_bdf,
				arg);
			std::string param{"xgmi-plpd"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::cout << string_format(setSuccessfullyTemplate,
							   arg.devices[i]->get_gpu_index(),
							   "DPM_POLICY",
							   "xgmi per-link power down policy",
							   (arg.plpd_set).c_str());
			}
		}
	}

	if (std::find(arg.options.begin(), arg.options.end(), "num-vf") != arg.options.end()) {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_set_num_vf_command(gpu_bdf,
				arg);
			std::string param{"num-vf"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::cout << string_format(setSuccessfullyTemplate,
							   arg.devices[i]->get_gpu_index(),
							   "NUM_VF_ENABLED",
							   "enabled VFs",
							   (arg.num_vf).c_str());
			}
		}
	}
}

void AmdSmiSetCommand::execute_command()
{
	if (arg.output == json) {
		throw SmiToolInvalidParameterException("--json");
	}
	if (arg.output == csv) {
		throw SmiToolInvalidParameterException("--csv");
	}

	set_command();
}
