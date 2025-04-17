/* * Copyright (C) 2024 Advanced Micro Devices. All rights reserved.
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

#include "json/json.h"

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_reset_command.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"

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
}

void AmdSmiResetCommand::execute_command()
{
	reset_command();
}
