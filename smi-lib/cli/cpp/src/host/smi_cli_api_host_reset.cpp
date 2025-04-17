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

#include <sstream>
#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_vf_handle_t *);
typedef amdsmi_status_t (*AMDSMI_CLEAR_VF_FB)(amdsmi_vf_handle_t);

extern AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
extern AMDSMI_CLEAR_VF_FB host_amdsmi_clear_vf_fb;


int AmdSmiApiHost::amdsmi_reset_command(std::string vf_bdf_str, Arguments arg)
{
	int ret;
	amdsmi_bdf_t vf_bdf;
	amdsmi_vf_handle_t vf_handle;

	vf_bdf.bdf.domain_number = std::stoi(vf_bdf_str.substr(0, 4), nullptr, 16);
	vf_bdf.bdf.bus_number = std::stoi(vf_bdf_str.substr(5, 2), nullptr, 16);
	vf_bdf.bdf.device_number = std::stoi(vf_bdf_str.substr(8, 2), nullptr, 16);
	vf_bdf.bdf.function_number = std::stoi(vf_bdf_str.substr(11), nullptr, 16);

	ret = host_amdsmi_get_vf_handle_from_bdf(vf_bdf, &vf_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		Logger::getInstance().log(LogLevel::Error, ret, __FUNCTION__, __FILE__, __LINE__);
		return ret;
	}

	ret = host_amdsmi_clear_vf_fb(vf_handle);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		return ret;
	}
	return ret;
}
