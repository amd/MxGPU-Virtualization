/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_INIT_TABLE_H
#define AMDGV_INIT_TABLE_H

#include <amdgv.h>
#include <amdgv_device.h>

extern struct amdgv_init_func gmc_v12_1_func;
extern struct amdgv_init_func gfx_v12_1_func;
extern struct amdgv_init_func gfx_v12_1_clockgating_func;
extern struct amdgv_init_func psp_v15_0_8_func;
extern struct amdgv_init_func smu_v15_0_8_func;
extern struct amdgv_init_func smu_v15_0_8_pp_func;
extern struct amdgv_init_func smu_v15_0_8_eeprom_func;
extern struct amdgv_init_func ih_v7_1_func;
extern struct amdgv_init_func gpuiov_v9_0_func;
extern struct amdgv_init_func gpuiov_v9_0_sched_early_func;
extern struct amdgv_init_func gpuiov_v9_0_sched_late_func;
extern struct amdgv_init_func nbio_v6_3_2_func;
extern struct amdgv_init_func nbio_v6_3_2_mcp_func;
extern struct amdgv_init_func nbio_v6_3_2_mailbox_func;
extern struct amdgv_init_func sdma_v7_1_func;
extern struct amdgv_init_func gpuiov_v9_0_reset_func;
extern struct amdgv_init_func lsdma_v7_1_func;
extern struct amdgv_init_func smu_v15_0_8_gpumon_func;
extern struct amdgv_init_func mmhub_v4_2_0_func;
extern struct amdgv_init_func gfxhub_v12_1_0_func;
extern struct amdgv_init_func gfx_v12_1_misc_func;
extern struct amdgv_init_func mes_v12_1_func;
extern struct amdgv_init_func amdgv_ual_func;
#ifdef AMDGV_UNIRAS_SUPPORT
extern struct amdgv_init_func amdgv_ras_mgr_func;
#endif

struct amdgv_init_func *gc_12_1_init_table[] = {
	&nbio_v6_3_2_mcp_func,
	&nbio_v6_3_2_func,
	&gmc_v12_1_func,
	&mmhub_v4_2_0_func,
	&psp_v15_0_8_func,
	&smu_v15_0_8_func,
	&smu_v15_0_8_eeprom_func,
#ifdef AMDGV_UNIRAS_SUPPORT
	&amdgv_ras_mgr_func,
#endif
	&gfxhub_v12_1_0_func,
	&gfx_v12_1_clockgating_func,
	&gpuiov_v9_0_func,
	&ih_v7_1_func,
	&nbio_v6_3_2_mailbox_func,
	&gpuiov_v9_0_reset_func,
	&smu_v15_0_8_pp_func,
	&smu_v15_0_8_gpumon_func,
	&lsdma_v7_1_func,
	&gfx_v12_1_misc_func,
	&amdgv_vfmgr_func,
	&gpuiov_v9_0_sched_early_func,
	&sdma_v7_1_func,
	&gfx_v12_1_func,
	&mes_v12_1_func,
	&gpuiov_v9_0_sched_late_func,
	//&amdgv_ual_func,
	NULL,
};

struct amdgv_reg_range *gc_12_1_mitigation_table[] = {
	NULL,
};

#endif /* AMDGV_INIT_TABLE_H */
