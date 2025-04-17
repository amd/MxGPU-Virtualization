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
#include "smi_cli_device.h"
#include "smi_cli_helpers.h"
#include "smi_cli_api_base.h"

Device::Device(int gpu, int vf, DeviceType device_type)
	: gpu_index(gpu), vf_index(vf), type(device_type)
{
	if (type != VF_INDEX) {
		//error
	} else {
		domain = "--vf";
		// get vf_handle
	}
}

Device::Device(int gpu, DeviceType device_type) : gpu_index(gpu), type(device_type)
{
	if (type != GPU_INDEX) {
		//error
		exit(1);
	} else {
		domain = "--gpu";
		int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bdf_from_gpu_index(bdf, gpu);
	}
}

Device::Device(std::string device, DeviceType device_type, std::string domain)
	: value(device), type(device_type), domain(domain)
{
	if ((type != BDF) && (type != UUID)) {
		//error
	}

	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bdf_from_uuid_or_bdf(bdf, gpu_index,
			  device, type);
}
