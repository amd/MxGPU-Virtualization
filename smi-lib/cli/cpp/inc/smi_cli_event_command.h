/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <thread>
#include <vector>

#include "smi_cli_parser.h"
#include "smi_cli_commands.h"

auto constexpr event_csv_header {"gpu, message, category, date"};

const char *const EVENT_CATEGORY_STR[] = {
	"NULL",
	"Driver",
	"Reset",
	"Scheduler",
	"VBIOS",
	"ECC",
	"Powerplay",
	"SRIOV",
	"VF",
	"Ucode",
	"GPU device",
	"Event guard",
	"GPU monitor",
};
class AmdSmiEventCommand : public AmdSmiCommands
{
public:
	AmdSmiEventCommand(Arguments args);
	void execute_command();
	void event_command_json();
	void event_command_human();
	void event_command_csv();

private:
	char stop;
	std::vector<std::thread> threads;
};