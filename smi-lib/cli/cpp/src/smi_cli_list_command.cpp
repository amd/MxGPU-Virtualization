/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_list_command.h"
#include "smi_cli_templates.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"

void AmdSmiListCommand::execute_command()
{
	std::string out;

	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_list_command(arg, out);
	std::string param{"list"};
	int error = handle_exceptions(ret, param, arg);
	if (error == 0) {
		if (arg.is_file) {
			write_to_file(arg.file_path, out);
		} else {
			std::cout << out.c_str() << std::endl;
		}
	}
};