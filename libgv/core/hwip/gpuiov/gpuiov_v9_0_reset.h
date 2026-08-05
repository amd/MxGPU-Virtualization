/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_RESET_H
#define MI300_RESET_H

#include <amdgv_pci_def.h>

#define GPUIOV_V9_0_MAX_VF_NUM 8

struct gpuiov_v9_0_reset_access_info {
	uint32_t fb_access_info;
	uint32_t doorbell_access_info;
	uint32_t register_write_access_info;
};

struct gpuiov_v9_0_vf_flr_state {
	uint32_t idx_vf;
	uint32_t tcp_addr_config;

	uint32_t active_vfs[AMDGV_MAX_NUM_HW_SCHED];
	uint32_t quanta_option[AMDGV_MAX_NUM_HW_SCHED];
	uint32_t quanta_index[AMDGV_MAX_NUM_HW_SCHED];

	uint32_t *pci_cfg;
	uint32_t msix_tab[12];
};

struct gpuiov_v9_0_whole_gpu_reset_state {
	uint32_t bif_bx_strap0;
	uint32_t pf_pm_ctl;
	uint16_t pf_misx_ctl;
	uint32_t pf_pci_cfg[PCI_CONFIG_SIZE];
	uint32_t pf_msix_tab[12];
	uint32_t vf_pci_cfg[GPUIOV_V9_0_MAX_VF_NUM][PCI_CONFIG_SIZE];
	uint32_t vf_msix_tab[GPUIOV_V9_0_MAX_VF_NUM][12];

        //@VICTOR: Delete once verified this is not required.
	// struct irqmgr_reset_state irqmgr_reset_state;
};

#endif