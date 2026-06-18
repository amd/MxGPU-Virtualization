/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_cli_event_command.h"
#include "smi_cli_api_base.h"
#include "smi_cli_exception.h"

AmdSmiEventCommand::AmdSmiEventCommand(Arguments args) : AmdSmiCommands(args), stop('\0')
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().initEvent();
	if (ret != 0) {
		std::string command{"event"};
		throw SmiToolCommandNotSupportedException(command);
	}
};

void AmdSmiEventCommand::execute_command()
{
	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_event_command(arg, stop, threads);
	if (ret != 0) {
		std::string command{"event"};
		throw SmiToolCommandNotSupportedException(command);
	}
}
