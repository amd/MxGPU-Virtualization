/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_RESET_H
#define AMDGV_RESET_H
#include <amdgv_pci_def.h>

#define in_whole_gpu_reset() (adapt->reset.reset_state == true)
#define AMDGV_RESET_DEFAULT_RESET_INTERVAL (60 * 1000 * 1000)

struct amdgv_gpu_reset_funcs {
	/* save vdd gfx state during init vf */
	int (*save_vddgfx_state)(struct amdgv_adapter *adapt, uint32_t idx_vf);

	/* trigger VF FLR */
	int (*trigger_vf_flr)(struct amdgv_adapter *adapt, uint32_t idx_vf);

	int (*reset_hw_for_reload)(struct amdgv_adapter *adapt, bool is_unload);
	int (*gpu_reset_and_reinit)(struct amdgv_adapter *adapt);

	int (*notify_engine_status)(struct amdgv_adapter *adapt, uint32_t idx_vf);

	int (*reset_pf_allowed)(struct amdgv_adapter *adapt, uint32_t active_vf_mask);
};

struct amdgv_reset {
	bool reset_state;
	bool in_xgmi_chain_reset;

	event_t pf_rel_gpu_init;
	bool reset_notify_vf_pending;
	uint32_t reset_mode;
	uint32_t reset_num;
	uint8_t sriov_cap[PCIE_EXT_SRIOV_SIZE];
	void *whole_gpu_reset_state;
	const struct amdgv_gpu_reset_funcs *funcs;
};

int amdgv_reset_mailbox_notify_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  bool completion);
int amdgv_reset_mailbox_notify_after_pf(struct amdgv_adapter *adapt);
int amdgv_reset_program_vf_mc_settings(struct amdgv_adapter *adapt);
int amdgv_reset_gpu_and_reinit(struct amdgv_adapter *adapt);
int amdgv_reset_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_reset_save_sriov(struct amdgv_adapter *adapt);
void amdgv_reset_restore_sriov(struct amdgv_adapter *adapt);
void amdgv_reset_restore_interrupt(struct amdgv_adapter *adapt);
int amdgv_reset_notify_engine_status(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_reset_notify_gpu_rma(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_reset_hw_for_reload(struct amdgv_adapter *adapt, bool is_unload);

#endif
