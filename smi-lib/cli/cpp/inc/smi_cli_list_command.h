/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"

class AmdSmiListCommand : public AmdSmiCommands
{
public:
	AmdSmiListCommand(Arguments args) : AmdSmiCommands(args) {};
	void execute_command();
};