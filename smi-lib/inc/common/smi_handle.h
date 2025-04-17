/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __SMI_HANDLE_H__
#define __SMI_HANDLE_H__

#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#endif
#include "smi_cmd_ioctl.h"

#ifdef _WIN64
#ifndef _KERNEL_MODE
#include <fileapi.h>
#endif
typedef HANDLE smi_file_handle;
#else
typedef int smi_file_handle;
#endif

#ifdef THREAD_SAFE
#include "smi_thread.h"
#endif

typedef struct smi_req_ctx_s smi_req_ctx;

typedef struct {
	smi_file_handle fd;
	int version;
	bool init;
	uint8_t padding[3];
#ifdef THREAD_SAFE
	smi_mutex_t lock;
#endif
} smi_handle_struct;

typedef struct {
	smi_ioctl_cmd ioctl_cmd;
} smi_thread_ctx;

struct smi_req_ctx_s {
	smi_handle_struct *handle;
	smi_thread_ctx    *thread;
};

typedef union {
	smi_file_handle fd;
	void *as_ptr;
} smi_event_handle_t;

#endif // SMI_HANDLE_H
