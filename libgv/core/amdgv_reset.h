/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

	/* trigger whole gpu reset */
	int (*trigger_gpu_reset)(struct amdgv_adapter *adapt);

	int (*notify_engine_status)(struct amdgv_adapter *adapt, uint32_t idx_vf);
};

struct amdgv_reset {
	bool reset_state;
	bool in_xgmi_chain_reset;

	/*
	 * Flag to know whether it is safe to call load rlcv state
	 * Should be reset on driver re-init and whole gpu reset
	 */
	bool saved_rlcv_state;
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
int amdgv_reset_gpu(struct amdgv_adapter *adapt);
int amdgv_reset_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_reset_save_sriov(struct amdgv_adapter *adapt);
void amdgv_reset_restore_sriov(struct amdgv_adapter *adapt);
void amdgv_reset_restore_interrupt(struct amdgv_adapter *adapt);
int amdgv_reset_notify_engine_status(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_reset_notify_gpu_rma(struct amdgv_adapter *adapt, uint32_t idx_vf);

#endif
