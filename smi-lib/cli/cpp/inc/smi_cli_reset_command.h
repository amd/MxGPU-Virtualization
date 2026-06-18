/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_commands.h"
#include "smi_cli_parser.h"

class AmdSmiResetCommand : public AmdSmiCommands
{
public:
	AmdSmiResetCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();
	void reset_command();
};