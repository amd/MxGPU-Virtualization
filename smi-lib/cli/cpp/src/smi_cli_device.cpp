/* * Copyright (C) 2023-2025 Advanced Micro Devices. All rights reserved.
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

Device::Device(int gpu, int vf, DeviceIdentifierType device_type, DeviceType type)
	: gpu_index(gpu), vf_index(vf), identifier_type(device_type), type(type)
{
	if (type != DeviceType::GPU)
		//error
		exit(1);
	if (identifier_type != DeviceIdentifierType::VF_INDEX) {
		//error
	} else {
		domain = "--vf";
		// get vf_handle
	}
}

Device::Device(int gpu, DeviceIdentifierType device_type, DeviceType type) : gpu_index(gpu),
	identifier_type(device_type), type(type)
{
	int ret = 0;
	if (identifier_type != DeviceIdentifierType::INDEX) {
		//error
		exit(1);
	} else {
		if (type == DeviceType::GPU) {
			domain = "--gpu";
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bdf_from_gpu_index(bdf, gpu);
		} else if (type == DeviceType::NIC) {
			domain = "--nic";
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bdf_from_nic_index(bdf, gpu);
		} else {
			//error
			exit(1);
		}
	}
}

Device::Device(std::string device, DeviceIdentifierType device_type, std::string domain,
			   DeviceType type)
	: value(device), identifier_type(device_type), domain(domain), type(type)
{
	if (type != DeviceType::GPU)
		//error
		exit(1);
	if ((identifier_type != DeviceIdentifierType::BDF) && (identifier_type != DeviceIdentifierType::UUID)) {
		//error
	}

	int ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bdf_from_uuid_or_bdf(bdf, gpu_index,
			  device, static_cast<int>(identifier_type));
}
