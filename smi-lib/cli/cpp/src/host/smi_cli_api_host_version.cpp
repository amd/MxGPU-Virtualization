/* * Copyright (C) 2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "amdsmi.h"
#include "smi_cli_api_host.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"

#include "json/json.h"

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_LIB_VERSION)(amdsmi_version_t *);

extern AMDSMI_GET_LIB_VERSION host_amdsmi_get_lib_version;

int AmdSmiApiHost::amdsmi_get_version_command(Arguments arg,
		std::string &out_string)
{
	std::string out{};
	amdsmi_version_t version;
	amdsmi_status_t ret;
	nlohmann::ordered_json json_out;
	ret = host_amdsmi_get_lib_version(&version);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}

	std::string amdsmi_lib_ver_str = string_format("%ld.%ld.%ld", version.major,
									 version.minor, version.release);

	if (arg.output == json) {
		json_out = {
			{"tool_name", AMDSMI_TOOL_NAME},
			{"tool_version", AMDSMI_TOOL_VERSION_STRING},
			{"lib_version", amdsmi_lib_ver_str.c_str()}
		};
	} else if (arg.output == csv) {
		out += string_format(
				   "%s,%s,%s", AMDSMI_TOOL_NAME,
				   AMDSMI_TOOL_VERSION_STRING,
				   amdsmi_lib_ver_str.c_str());
	} else {
		out += string_format(
				   versionTemplate, AMDSMI_TOOL_NAME,
				   AMDSMI_TOOL_VERSION_STRING,
				   amdsmi_lib_ver_str.c_str());
	}

	if (arg.output == json) {
		out_string += json_out.dump(4);
	} else {
		out_string += out;
	}

	return AMDSMI_STATUS_SUCCESS;
}
