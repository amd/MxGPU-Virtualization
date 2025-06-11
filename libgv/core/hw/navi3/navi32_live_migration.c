/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "amdgv_live_migration.h"
#include "navi32_gpuiov.h"

static const uint32_t this_block = AMDGV_LIVE_MIGRATION_BLOCK;

static int navi32_migration_send_transfer_cmd(struct amdgv_adapter *adapt,
	uint32_t idx_vf, bool export)
{
	int ret = 0;

	if (amdgv_gpuiov_transfer_vf_data(adapt, idx_vf,
			NAVI32_HW_SCHED_BLOCK_VCN_SCH0_MMSCH, export) ||
			amdgv_gpuiov_transfer_vf_data(adapt, idx_vf,
			NAVI32_HW_SCHED_BLOCK_VCN1_SCH1_MMSCH, export) ||
			amdgv_gpuiov_transfer_vf_data(adapt, idx_vf,
			NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH, export) ||
			amdgv_gpuiov_transfer_vf_data(adapt, idx_vf,
			NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV, export)) {
		AMDGV_WARN("Failed to export GPU state for VF%d\n", idx_vf);
		ret = AMDGV_FAILURE;
	}

	return ret;
}


static int navi32_migration_psp_export(struct amdgv_adapter *adapt,
	uint32_t idx_vf, void *data_dst, enum amdgv_migration_export_phase phase)
{
	int ret = 0;
	return ret;
}

static const struct amdgv_lm_funcs navi32_lm_funcs = {
	.send_transfer_cmd = navi32_migration_send_transfer_cmd,
	.psp_export = navi32_migration_psp_export,
};

static int navi32_migration_sw_init(struct amdgv_adapter *adapt)
{
	adapt->live_migration.lm_lock = oss_mutex_init();
	if (adapt->live_migration.lm_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	adapt->live_migration.lm_funcs = &navi32_lm_funcs;

	return 0;

fail:
	return 0;
}

static int navi32_migration_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->live_migration.lm_funcs = NULL;

	if (adapt->live_migration.lm_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->live_migration.lm_lock);
		adapt->live_migration.lm_lock = OSS_INVALID_HANDLE;
	}

	return 0;
}

static int navi32_migration_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_migration_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func navi32_live_migration_func = {
	.name = "navi32_live_migration_func",
	.sw_init = navi32_migration_sw_init,
	.sw_fini = navi32_migration_sw_fini,
	.hw_init = navi32_migration_hw_init,
	.hw_fini = navi32_migration_hw_fini,
};

