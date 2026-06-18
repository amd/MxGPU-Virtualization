/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <map>

#include "json/json.h"

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_reset_command.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"
#include "smi_cli_platform.h"

void AmdSmiResetCommand::reset_command()
{
	int ret;

	if ((std::find(arg.options.begin(), arg.options.end(), "vf-fb") != arg.options.end()) ||
			arg.all_arguments) {
		std::string vf_bdf;
		std::tuple<std::string, std::string, std::string> indexes =
			getGpuVfIndexFromVfId(arg.vf_id);
		vf_bdf = std::get<2>(indexes).c_str();
		ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_reset_command(vf_bdf,
			  arg);
		std::string param{"vf-fb"};
		int error = handle_exceptions(ret, param, arg);
		if (error == 0) {
			std::cout << "Successfully reset vf fb for vf with id: " << arg.vf_id << std::endl;
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "clean-local-data") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "l") != arg.options.end()) ||
			arg.all_arguments) {
		for (unsigned int i = 0; i < arg.devices.size(); i++) {
			uint64_t gpu_bdf = arg.devices[i]->get_bdf();
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_reset_local_data_command(gpu_bdf,
				  arg);
			std::string param{"clean-local-data"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				std::cout << "GPU: " << arg.devices[i]->get_gpu_index() << std::endl;
				std::cout << "    CLEAN_LOCAL_DATA: Successfully clean GPU local data" << std::endl;
			}
		}
	}

	if ((std::find(arg.options.begin(), arg.options.end(), "gpureset") != arg.options.end()) ||
			(std::find(arg.options.begin(), arg.options.end(), "G") != arg.options.end()) ||
			arg.all_arguments) {
		unsigned int gpu_count;
		if (AmdSmiPlatform::getInstance().is_nv() == true) {
			for (unsigned int i = 0; i < arg.devices.size(); i++) {
				uint64_t gpu_bdf = arg.devices[i]->get_bdf();
				ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_reset_gpu_command(gpu_bdf, arg);
				std::string param{"gpureset"};
				int error = handle_exceptions(ret, param, arg);
				if (error == 0) {
					std::cout << "GPU: " << arg.devices[i]->get_gpu_index() << std::endl;
					std::cout << "    GPU_RESET: Successfully reset GPU" << std::endl;
				}
			}
		} else {
			AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_device_count(gpu_count, static_cast<int>(DeviceType::GPU));
			uint64_t gpu_bdf = arg.devices[0]->get_bdf();
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_reset_gpu_command(gpu_bdf, arg);
			std::string param{"gpureset"};
			int error = handle_exceptions(ret, param, arg);
			if (error == 0) {
				for (unsigned int i = 0; i < gpu_count; i++) {
					std::cout << "GPU: " << i << std::endl;
					std::cout << "    GPU_RESET: Successfully reset GPU" << std::endl;
				}
			}
		}
	}
}

void AmdSmiResetCommand::execute_command()
{
	reset_command();
}
