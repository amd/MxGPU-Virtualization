/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE.
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
