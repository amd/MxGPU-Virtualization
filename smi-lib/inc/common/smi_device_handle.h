/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DEVICE_HANDLE_H__
#define __SMI_DEVICE_HANDLE_H__

typedef struct device_handle {
	uint64_t handle;
	uint64_t device_id;
} smi_device_handle_t;

typedef struct node_handle {
	uint64_t handle;
} smi_node_handle_t;

#endif // __SMI_DEVICE_HANDLE_H__
