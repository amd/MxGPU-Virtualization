/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef MI300_GFX_H
#define MI300_GFX_H

struct mi300_vf_flr_state {
	uint32_t idx_vf;
	uint32_t tcp_addr_config;

	uint32_t active_vfs[AMDGV_MAX_NUM_HW_SCHED];
	uint32_t quanta_option[AMDGV_MAX_NUM_HW_SCHED];
	uint32_t quanta_index[AMDGV_MAX_NUM_HW_SCHED];

	uint32_t csa;

	uint32_t *pci_cfg;
	uint32_t msix_tab[12];
};

uint32_t mi300_gfx_get_xcc_cu_count(struct amdgv_adapter *adapt, uint32_t xcc_id);
bool mi300_gfx_is_symetric_cu(struct amdgv_adapter *adapt, uint32_t xcc_id);
int mi300_gfx_enable_rlc(struct amdgv_adapter *adapt, uint32_t idx_vf);
void mi300_gfx_save_tcp_addr_config(struct amdgv_adapter *adapt,
				    struct mi300_vf_flr_state *vf_state);
void mi300_gfx_restore_tcp_addr_config(struct amdgv_adapter *adapt,
				       struct mi300_vf_flr_state *vf_state);
int mi300_gfx_wait_for_grbm(struct amdgv_adapter *adapt, uint16_t idx_vf);
int mi300_gfx_init_flr(struct amdgv_adapter *adapt, uint32_t idx_vf);
int mi300_gfx_wait_rlc_idle(struct amdgv_adapter *adapt, uint16_t phys_xcc_id);
void mi300_gfx_rlc_smu_handshake_cntl(struct amdgv_adapter *adapt, bool enable);
#endif
