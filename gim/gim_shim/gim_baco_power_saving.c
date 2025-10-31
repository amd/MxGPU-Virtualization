/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/completion.h>

#include "amdgv_oss.h"
#include "gim_debug.h"
#include "gim.h"
#include "gim_config.h"
#include "gim_baco_power_saving.h"

int gim_enter_baco(amdgv_dev_t dev)
{
	int i;
	uint32_t vf_num;
	unsigned int domain;
	unsigned int bus;
	unsigned int devfn;
	struct pci_dev *pdev;
	union amdgv_dev_info dev_info;
	struct gim_dev_data *data;

	if (power_saving_mode != AMDGV_IPS_POWER_SAVING_MANUAL) {
		gim_warn("Failed to enter baco, power saving mode not set to manual\n");
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	list_for_each_entry(data, &gim_device_list, list) {
		if (data->adev == dev)
			break;
	}

	if (!data || data->adev != dev)
		return -1;

	memset(&dev_info, 0, sizeof(dev_info));
	if (amdgv_get_dev_info(dev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		vf_num = 0;
	else
		vf_num = dev_info.vf.num_enabled_vf;

	memset(&data->vf_info, 0, sizeof(data->vf_info));
	for (i = 0; i < vf_num; i++) {
		if (amdgv_get_vf_info(dev, i, AMDGV_GET_VF_BDF, &data->vf_info))
			continue;

		domain = data->vf_info.id.bdf >> 16;
		bus = data->vf_info.id.bdf >> 8 & 0xff;
		devfn = data->vf_info.id.bdf & 0xff;
		pdev = pci_get_domain_bus_and_slot(domain, bus, devfn);
		if (pdev && pdev->current_state != PCI_UNKNOWN) {
			gim_warn("Failed to enter baco power saving, there is VM running\n");
			return AMDGV_ERROR_GPUMON_VF_BUSY;
		}
	}

	return amdgv_toggle_power_saving(dev, 1);
}

int gim_exit_baco(amdgv_dev_t dev)
{
	if (power_saving_mode != AMDGV_IPS_POWER_SAVING_MANUAL) {
		gim_warn("Failed to exit baco, power saving mode not set to manual\n");
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	return amdgv_toggle_power_saving(dev, 0);
}

int gim_get_baco_status(amdgv_dev_t dev, uint32_t *status)
{
	if (power_saving_mode != AMDGV_IPS_POWER_SAVING_MANUAL) {
		gim_warn("Failed get baco status, power saving mode not set to manual\n");
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	return amdgv_query_power_saving_status(dev, status);
}

int gim_get_power_saving_mode(void)
{
	return power_saving_mode;
}
