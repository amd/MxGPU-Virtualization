/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
		} else if ((type == DeviceType::NIC) || (type == DeviceType::BRCM_NIC)) {
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
	int ret = 0;
	if (identifier_type == DeviceIdentifierType::UUID) {
		if (type != DeviceType::GPU) {
			//error
			exit(1);
		}
		ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bdf_from_uuid_or_bdf(bdf, gpu_index,
			  device, static_cast<int>(identifier_type));
	}
	else if (identifier_type == DeviceIdentifierType::BDF) {
		if (type == DeviceType::GPU) {
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bdf_from_uuid_or_bdf(bdf, gpu_index,
				device, static_cast<int>(identifier_type));
		} else if ((type == DeviceType::NIC) || (type == DeviceType::BRCM_NIC)) {
			ret = AmdSmiApiBase::CreateAmdSmiApiObject().amdsmi_get_bdf_from_bdf_nic(bdf, gpu_index,
				device);
		} else {
			//error
			exit(1);
		}
	}
}
