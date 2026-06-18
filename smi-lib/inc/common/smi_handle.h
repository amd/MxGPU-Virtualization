/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

#ifdef AMD_SMI_NIC_SUPPORT
#include "smi_nic_interface.h"
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
#ifdef AMD_SMI_NIC_SUPPORT
	smi_nic_ctx_t nic_ctx;
	bool nic_init;
#endif
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
