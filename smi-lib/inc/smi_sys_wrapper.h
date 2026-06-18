/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
	int (*smi_poll)(struct smi_event_set_s *, struct smi_event_entry *, int64_t);
	void *(*smi_poll_alloc)(smi_event_handle_t *, uint32_t);
	bool (*smi_is_user_mode)(void);
	void* (*smi_aligned_alloc)(void **mem, size_t alignment, size_t size);
	void (*smi_aligned_free)(void *);
	int (*smi_strncpy)(char *dest, size_t destsz, const char *src, size_t count);
	long (*smi_sysconf)(int);
	FILE* (*fopen)(const char *, const char *);
	char* (*fgets)(char *, int, FILE *);
	int (*snprintf)(char *, size_t, const char *, ...);
} system_wrapper;

extern system_wrapper *get_system_wrapper(void);

#endif // __SMI_WRAPPER_H__
