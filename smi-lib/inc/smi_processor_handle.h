/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_PROCESSOR_HANDLE_H__
#define __SMI_PROCESSOR_HANDLE_H__

#define AMDSMI_MAX_NODES    32  //!< Maximum number of nodes supported

enum smi_handle_type {
	SMI_HANDLE_TYPE_UNKNOWN = 0,
	SMI_HANDLE_TYPE_AMD_GPU,
	SMI_HANDLE_TYPE_AMD_NIC,
	SMI_HANDLE_TYPE_BRCM_NIC,
	SMI_HANDLE_TYPE_NODE
};

struct smi_gpu_handle {
	enum smi_handle_type type;
	amdsmi_bdf_t bdf;
	uint64_t handle;
	uint64_t dev_id;
};

struct smi_node_handle {
	enum smi_handle_type type;
	uint64_t handle;
};

struct smi_nic_handle {
	enum smi_handle_type type;
	amdsmi_bdf_t bdf;
};

struct smi_gpu_device_handles {
	uint32_t num_gpus;
	struct smi_gpu_handle gpus[AMDSMI_MAX_DEVICES];
};

struct smi_nic_device_handles {
	uint32_t num_nics;
	struct smi_nic_handle nics[AMDSMI_MAX_DEVICES];
};

struct smi_node_handles {
	uint32_t num_nodes;
	struct smi_node_handle nodes[AMDSMI_MAX_NODES];
};

#endif // __SMI_PROCESSOR_HANDLE_H__
