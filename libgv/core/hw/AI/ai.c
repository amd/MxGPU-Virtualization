/*
 * Copyright (c) 2017-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_device.h"
#include "amdgv_vfmgr.h"
#include "amdgv_powerplay.h"

extern struct amdgv_init_func mi300_vbios_early_func;
extern struct amdgv_init_func mi300_vbios_late_func;
extern struct amdgv_init_func mi300_ucode_func;
extern struct amdgv_init_func mi300_psp_func;
extern struct amdgv_init_func mi300_ecc_func;
extern struct amdgv_init_func mi300_gpuiov_func;
extern struct amdgv_init_func mi300_reset_func;
extern struct amdgv_init_func mi300_sched_early_func;
extern struct amdgv_init_func mi300_sched_late_func;
extern struct amdgv_init_func mi300_smu_func;
extern struct amdgv_init_func mi300_powerplay_func;
extern struct amdgv_init_func mi300_gpumon_func;
extern struct amdgv_init_func mi300_misc_func;
extern struct amdgv_init_func mi300_mem_func;
extern struct amdgv_init_func mi300_clockgating_func;
extern struct amdgv_init_func mi300_irqmgr_func;
extern struct amdgv_init_func mi300_mailbox_func;
extern struct amdgv_init_func mi300_mcp_func;
extern struct amdgv_init_func mi308_mcp_func;
extern struct amdgv_init_func mi300_ip_discovery_func;
extern struct amdgv_init_func mi300_diag_data_func;
extern struct amdgv_init_func mi300_xgmi_early_func;
extern struct amdgv_init_func mi300_xgmi_late_func;
extern struct amdgv_init_func mi300_doorbell_func;
extern struct amdgv_init_func mi300_sdma_v4_4_2_func;
extern struct amdgv_init_func mi300_gfx_v9_4_3_func;
extern struct amdgv_init_func mi308_ucode_func;

struct amdgv_init_func *mi300x_init_table[] = {
	&mi300_ip_discovery_func,
	&mi300_mcp_func,
	&mi300_xgmi_early_func,
	&mi300_vbios_early_func,
	&mi300_mem_func,
	&mi300_ucode_func,
	&mi300_psp_func,
	&mi300_smu_func,
	&mi300_clockgating_func,
	&mi300_vbios_late_func,
	&mi300_ecc_func,
	&mi300_gpuiov_func,
	&mi300_xgmi_late_func,
	&mi300_doorbell_func,
	&mi300_irqmgr_func,
	&mi300_mailbox_func,
	&mi300_reset_func,
	&mi300_powerplay_func,
	&mi300_gpumon_func,
	&mi300_misc_func,
	&amdgv_vfmgr_func,
	&mi300_sched_early_func,
	&mi300_diag_data_func,
	&mi300_sdma_v4_4_2_func,
	&mi300_gfx_v9_4_3_func,
	&mi300_sched_late_func,
	NULL,
};

struct amdgv_init_func *mi308x_init_table[] = {
	&mi300_ip_discovery_func,
	&mi308_mcp_func,
	&mi300_xgmi_early_func,
	&mi300_vbios_early_func,
	&mi300_mem_func,
	&mi308_ucode_func,
	&mi300_psp_func,
	&mi300_smu_func,
	&mi300_clockgating_func,
	&mi300_vbios_late_func,
	&mi300_ecc_func,
	&mi300_gpuiov_func,
	&mi300_xgmi_late_func,
	&mi300_doorbell_func,
	&mi300_irqmgr_func,
	&mi300_mailbox_func,
	&mi300_reset_func,
	&mi300_powerplay_func,
	&mi300_gpumon_func,
	&mi300_misc_func,
	&amdgv_vfmgr_func,
	&mi300_sched_early_func,
	/* Uncomment when support is complete for MI300
#ifndef EXCLUDE_DIAG_DATA
	&mi300_diag_data_func,
#endif
	*/
	&mi300_sdma_v4_4_2_func,
	&mi300_gfx_v9_4_3_func,
	&mi300_sched_late_func,
	NULL,
};

struct amdgv_reg_range *mi300_mitigation_table[] = {
	NULL,
};
