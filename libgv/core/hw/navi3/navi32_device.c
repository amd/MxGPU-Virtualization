/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_vfmgr.h>

extern struct amdgv_init_func navi32_irqmgr_func;
extern struct amdgv_init_func navi32_mailbox_func;
extern struct amdgv_init_func navi32_mem_func;
extern struct amdgv_init_func navi32_misc_func;
extern struct amdgv_init_func navi32_ecc_func;
extern struct amdgv_init_func navi32_gpumon_func;

extern struct amdgv_init_func navi32_vbios_func;
extern struct amdgv_init_func navi32_reset_func;
extern struct amdgv_init_func navi32_gpuiov_func;
extern struct amdgv_init_func navi32_sched_early_func;
extern struct amdgv_init_func navi32_sched_late_func;
extern struct amdgv_init_func navi32_ucode_func;
extern struct amdgv_init_func navi32_clockgating_func;
extern struct amdgv_init_func navi32_psp_func;
extern struct amdgv_init_func navi32_misc_func;
extern struct amdgv_init_func navi32_powerplay_func;
extern struct amdgv_init_func navi32_smu_func;
extern struct amdgv_init_func navi32_ffbm_func;
extern struct amdgv_init_func navi32_gfx_func;
extern struct amdgv_init_func navi32_mmsch_func;
extern struct amdgv_init_func navi32_df_v4_3_func;
extern struct amdgv_init_func navi32_doorbell_func;
extern struct amdgv_init_func gfx_v11_func;
extern struct amdgv_init_func navi32_sdma_func;
extern struct amdgv_init_func navi32_diag_data_func;
extern struct amdgv_init_func navi32_dirtybit_func;
extern struct amdgv_init_func amdgv_migration_func;
extern struct amdgv_live_info_func navi32_ecc_live_info_func;

struct amdgv_init_func *navi32_init_table[] = {
	&navi32_vbios_func,
	&navi32_mem_func,
	&navi32_ucode_func,
	&navi32_psp_func,
	&navi32_smu_func,
	&navi32_df_v4_3_func,
	&navi32_powerplay_func,
	&navi32_gfx_func,
	&navi32_ecc_func,
	&navi32_mmsch_func,
	&navi32_gpuiov_func,
	&navi32_doorbell_func,
	&navi32_irqmgr_func,
	&navi32_mailbox_func,
	&navi32_reset_func,

	&navi32_clockgating_func,
	&navi32_ffbm_func,

	&navi32_gpumon_func,
	&navi32_misc_func,
	&amdgv_vfmgr_func,
	&navi32_sched_early_func,
	&navi32_sdma_func,
	&gfx_v11_func,
	&navi32_sched_late_func,
	&navi32_diag_data_func,
	&amdgv_migration_func,
	&navi32_dirtybit_func,
	NULL,
};

struct amdgv_live_info_func *navi32_live_info_table[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&navi32_ecc_live_info_func,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

struct amdgv_reg_range *navi32_mitigation_table[] = {
	NULL,
};
