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
