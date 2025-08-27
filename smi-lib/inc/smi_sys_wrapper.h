/*
 * Copyright (c) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __SMI_WRAPPER_H__
#define __SMI_WRAPPER_H__

#include <stddef.h>
#include <stdio.h>

#include "amdsmi.h"
#include "smi_defines.h"
#include "smi_os_defines.h"
#include "common/smi_cmd.h"

typedef struct {
	void *(*smi_malloc)(size_t);
	void* (*smi_calloc) (size_t num, size_t size);
	void (*smi_free)(void *);
	int (*smi_ioctl)(smi_file_handle, smi_ioctl_cmd *);
	smi_file_handle (*smi_open)(enum smi_file_access_mode);
	int (*smi_access)(void);
	int (*smi_close)(smi_file_handle);
	int (*smi_poll)(struct smi_event_set_s *, amdsmi_event_entry_t *, int64_t);
	void *(*smi_poll_alloc)(smi_event_handle_t *, uint32_t);
	bool (*smi_is_user_mode)(void);
	void* (*smi_aligned_alloc)(void **mem, size_t alignment, size_t size);
	void (*smi_aligned_free)(void *);
	int (*smi_strncpy)(char *dest, size_t destsz, const char *src, size_t count);
	long (*smi_sysconf)(int);
	FILE* (*fopen)(const char *, const char *);
	char* (*fgets)(char *, int, FILE *);
} system_wrapper;

extern system_wrapper *get_system_wrapper(void);

#endif // __SMI_WRAPPER_H__
