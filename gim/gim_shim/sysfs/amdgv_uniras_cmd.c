/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/sort.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include "amdgv_uniras_cmd.h"
#include "amdgv_cmd_uni_def.h"
#include "amdgv_asic.h"
#include "amdgv_api.h"
#include "amdgv_gpumon.h"
#include "gim_debug.h"
#include "gim.h"

extern struct list_head gim_device_list;
extern struct mutex gim_device_list_lock;

#define AMDGV_UNI_CMD _IOWR('R', 0, struct amdgv_uni_cmd)

bool amdgv_is_uni_cmd(unsigned int cmd)
{
	if (cmd == AMDGV_UNI_CMD)
		return true;
	else
		return false;
}
long amdgv_uni_cmd_handler(void *arg)
{
	struct amdgv_uni_cmd *amdgv_uni_cmd;

	amdgv_uni_cmd = gim_vmalloc(sizeof(struct amdgv_uni_cmd));
	if (!amdgv_uni_cmd) {
		gim_warn("Cannot allocate memory for uni command\n");
		return -ENOMEM;
	}

	if (copy_from_user(amdgv_uni_cmd, arg, sizeof(struct amdgv_uni_cmd))) {
		gim_vfree(amdgv_uni_cmd);
		return -EFAULT;
	}

	amdgv_uni_cmd->cmd_res = AMDGV_CMD__ERROR_INVALID_INPUT;
	amdgv_uni_cmd->output_size = 0;

	amdgv_uni_cmd->cmd_res = amdgv_handle_uni_cmd(amdgv_uni_cmd);

	if (copy_to_user(arg, amdgv_uni_cmd, sizeof(struct amdgv_uni_cmd))) {
		gim_vfree(amdgv_uni_cmd);
		return -EFAULT;
	}

	// Clear command buffer
	memset(amdgv_uni_cmd, 0, sizeof(struct amdgv_uni_cmd));
	gim_vfree(amdgv_uni_cmd);

	return 0;
}

