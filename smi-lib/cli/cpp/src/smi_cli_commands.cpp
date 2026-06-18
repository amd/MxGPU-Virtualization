/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_cli_commands.h"
#include "smi_cli_helpers.h"
#include "smi_cli_templates.h"

AmdSmiCommands::AmdSmiCommands(Arguments args)
{
	arg = args;
}

void AmdSmiCommands::execute_command() {};
