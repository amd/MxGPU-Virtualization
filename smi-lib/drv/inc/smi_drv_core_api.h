/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DRV_CORE_API_H__
#define __SMI_DRV_CORE_API_H__

#include <smi_drv_oss.h>

#include "smi_drv_types.h"
#include "smi_cmd_ioctl.h"

struct oss_interface;

struct smi_device_data {
	struct amdgv_init_data init_data;
	amdgv_dev_t adev;
	int64_t parent;
};

int smi_core_init(struct oss_interface *oss_interface,
		struct smi_shim_interface *shim_interface);
int smi_core_fini(void);

int smi_core_open(file_t filp, bool is_privileged);
int smi_core_release(file_t filp);
int smi_core_ioctl_handler(file_t filp, unsigned int cmd, void *arg);

#endif // __SMI_DRV_CORE_API_H__
