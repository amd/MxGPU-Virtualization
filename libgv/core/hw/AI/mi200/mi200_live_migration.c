/*
 * Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include <amdgv_device.h>
#include "mi200.h"
#include "mi200_live_migration.h"
#include "mi200_ppsmc.h"
#include "mi200_powerplay.h"
#include "psp_v13_0.h"
#include "amdgv_sched_internal.h"
#include "amdgv_live_migration.h"
#include "mi200_gpuiov.h"

static const uint32_t this_block = AMDGV_LIVE_MIGRATION_BLOCK;

/*
 *			+---------------+					^
 *          |  			 	|					|
 *          |		(4M)	|				dynamic_data_mem
 *          |	ALLOCATION	|					|
 * 			+---------------+<--PSP DYNAMIC PKG	-
 *          |  			 	|					|
 *          |		(4M)	|				static_data_mem
 *          |	ALLOCATION	|					|
 *          +---------------+<--PSP STATIC PKG	v
 */
static int mi200_migration_mem_init(struct amdgv_adapter *adapt)
{
	/* allocate static/dynamic data pkg inside visible framebuffer
	 * if the memory manager is enabled */
	if (adapt->memmgr_pf.is_init) {
		adapt->live_migration.static_data_mem =
			amdgv_memmgr_alloc(&adapt->memmgr_pf,
				MI200_MIGRATION_PSP_STATIC_DATA_SIZE,
				MEM_MIGRATION_PSP_STATIC_DATA);
		if (!adapt->live_migration.static_data_mem) {
			amdgv_put_error(AMDGV_PF_IDX,
					AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL,
					MI200_MIGRATION_PSP_STATIC_DATA_SIZE);
			return AMDGV_FAILURE;
		}

		adapt->live_migration.dynamic_data_mem =
			amdgv_memmgr_alloc(&adapt->memmgr_pf,
				MI200_MIGRATION_PSP_DYNAMIC_DATA_SIZE,
				MEM_MIGRATION_PSP_DYNAMIC_DATA);
		if (!adapt->live_migration.dynamic_data_mem) {
			amdgv_put_error(AMDGV_PF_IDX,
					AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL,
					MI200_MIGRATION_PSP_DYNAMIC_DATA_SIZE);
			return AMDGV_FAILURE;
		}
	}
	else {
		AMDGV_WARN("memmgr_pf is not initialized, skipped live migration.\n");
		return AMDGV_FAILURE;
	}

	/* Allocate sysmem buffer for storing VF FB during live migration*/
	/* TODO: will need a better way to move the allocation to be during runtime */
	if (!(adapt->flags & AMDGV_FLAG_USE_PF) &&
	    !(adapt->flags & AMDGV_FLAG_DISABLE_SYS_APERTURE)) {
		if (!adapt->sys_mem_info.va_ptr) {
			if (oss_alloc_dma_mem(adapt->dev, AMDGV_AGP_APERTURE_SIZE,
				      OSS_DMA_MEM_CACHEABLE, &adapt->sys_mem_info)) {
				AMDGV_WARN("Failed to allocate %dMB continuous dma memory\n",
				   AMDGV_AGP_APERTURE_SIZE >> 20);
				return AMDGV_FAILURE;
			}
			else {
				AMDGV_DEBUG("ma = 0x%llx, va = %p\n",
					adapt->sys_mem_info.bus_addr, adapt->sys_mem_info.va_ptr);
			}
		}
	}

	return 0;
}

static void mi200_migration_mem_fini(struct amdgv_adapter *adapt)
{
	if (adapt->live_migration.static_data_mem) {
		amdgv_memmgr_free(adapt->live_migration.static_data_mem);
		adapt->live_migration.static_data_mem = NULL;
	}
	if (adapt->live_migration.dynamic_data_mem) {
		amdgv_memmgr_free(adapt->live_migration.dynamic_data_mem);
		adapt->live_migration.dynamic_data_mem = NULL;
	}
	if (adapt->sys_mem_info.handle) {
		oss_free_dma_mem(adapt->sys_mem_info.handle);
		adapt->sys_mem_info.handle = NULL;
	}
}

static int mi200_migration_get_migration_version(struct amdgv_adapter *adapt,
	uint32_t *migration_version)
{
	int ret = 0;
	enum psp_status psp_ret;

	psp_ret = psp_v13_migration_get_psp_info(adapt, migration_version);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("Failed to get PSP migration version.\n");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static int mi200_migration_get_manifest_size(struct amdgv_adapter *adapt,
	uint64_t* size, enum amdgv_migration_data_section section)
{
	int ret = 0;

	switch (section) {
	case AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA:
		*size = MI200_MIGRATION_PSP_STATIC_DATA_SIZE;
		break;

	case AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA:
		*size = MI200_MIGRATION_PSP_DYNAMIC_DATA_SIZE;
		break;

	default:
		*size = 0;
		ret = AMDGV_FAILURE;
	}
	return ret;
}

static int mi200_migration_send_transfer_cmd(struct amdgv_adapter *adapt,
	uint32_t idx_vf, bool export)
{
	int ret = 0;

	if (amdgv_gpuiov_transfer_vf_data(adapt, idx_vf,
			MI200_HW_SCHED_BLOCK_UVD_SCH0_MMSCH, export) ||
			amdgv_gpuiov_transfer_vf_data(adapt, idx_vf,
			MI200_HW_SCHED_BLOCK_UVD_SCH1_MMSCH, export) ||
			amdgv_gpuiov_transfer_vf_data(adapt, idx_vf,
			MI200_HW_SCHED_BLOCK_GFX_SCH0_RLCV, export)) {
		AMDGV_WARN("Failed to export GPU state for VF%d\n", idx_vf);
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static int mi200_migration_psp_export(struct amdgv_adapter *adapt,
	uint32_t idx_vf, void* data_dst, enum amdgv_migration_export_phase phase)
{
	int ret = 0;
	enum psp_status psp_ret;
	uint32_t pkg_size 				= 0;
	uint64_t psp_data_gpu_addr		= 0;

	void *static_data_cpu_addr 	= NULL;
	void *dynamic_data_cpu_addr = NULL;

	switch (phase) {
	case AMDGV_MIGRATION_EXPORT_PHASE1_STATIC_DATA:
		if (!adapt->live_migration.static_data_mem) {
			AMDGV_ERROR("Migration static data memory not ready, exit.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}

		static_data_cpu_addr =
			amdgv_memmgr_get_cpu_addr(adapt->live_migration.static_data_mem);
		psp_data_gpu_addr =
			amdgv_memmgr_get_gpu_addr(adapt->live_migration.static_data_mem);

		psp_ret = psp_v13_migration_export(adapt, idx_vf, psp_data_gpu_addr,
			MI200_MIGRATION_PSP_STATIC_DATA_SIZE, &pkg_size, true);
		if (psp_ret != PSP_STATUS__SUCCESS) {
			AMDGV_ERROR("Failed to do migration psp static export.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}

		if (pkg_size > 0 && pkg_size <= MI200_MIGRATION_PSP_STATIC_DATA_SIZE) {
			oss_memcpy(data_dst, static_data_cpu_addr,
				pkg_size);
		}
		else {
			AMDGV_ERROR("PSP static export data is empty or oversized.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}
		break;

	case AMDGV_MIGRATION_EXPORT_PHASE2_DYNAMIC_DATA:
		if (!adapt->live_migration.dynamic_data_mem) {
			AMDGV_ERROR("Migration dynamic data memory not ready, exit.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}

		dynamic_data_cpu_addr =
			amdgv_memmgr_get_cpu_addr(adapt->live_migration.dynamic_data_mem);

		psp_data_gpu_addr =
			amdgv_memmgr_get_gpu_addr(adapt->live_migration.dynamic_data_mem);

		psp_ret = psp_v13_migration_export(adapt, idx_vf, psp_data_gpu_addr,
			MI200_MIGRATION_PSP_DYNAMIC_DATA_SIZE, &pkg_size, false);
		if (psp_ret != PSP_STATUS__SUCCESS) {
			AMDGV_ERROR("Failed to do migration psp dynamic export.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}

		if (pkg_size > 0 && pkg_size <= MI200_MIGRATION_PSP_DYNAMIC_DATA_SIZE) {
			oss_memcpy(data_dst, dynamic_data_cpu_addr,
				pkg_size);
		}
		else {
			AMDGV_ERROR("PSP dynamic export data is empty or oversized.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}
		break;

	default:
		ret = AMDGV_FAILURE;
		break;
	}

exit:
	return ret;
}

static int mi200_migration_psp_import(struct amdgv_adapter *adapt,
	uint32_t idx_vf, enum amdgv_migration_import_phase phase)
{
	int ret = 0;
	enum psp_status psp_ret;
	uint64_t psp_data_gpu_addr		= 0;

	switch (phase) {
	case AMDGV_MIGRATION_IMPORT_PHASE2_STATIC_DATA:
		psp_data_gpu_addr =
			amdgv_memmgr_get_gpu_addr(adapt->live_migration.static_data_mem);

		psp_ret = psp_v13_migration_import(adapt, idx_vf, psp_data_gpu_addr,
			MI200_MIGRATION_PSP_STATIC_DATA_SIZE);
		if (psp_ret != PSP_STATUS__SUCCESS) {
			AMDGV_ERROR("Failed to do migration psp static import.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}

		break;

	case AMDGV_MIGRATION_IMPORT_PHASE3_DYNAMIC_DATA:
		psp_data_gpu_addr =
			amdgv_memmgr_get_gpu_addr(adapt->live_migration.dynamic_data_mem);

		psp_ret = psp_v13_migration_import(adapt, idx_vf, psp_data_gpu_addr,
			MI200_MIGRATION_PSP_DYNAMIC_DATA_SIZE);
		if (psp_ret != PSP_STATUS__SUCCESS) {
			AMDGV_ERROR("Failed to do migration psp dynamic import.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}

		break;

	default:
		ret = AMDGV_FAILURE;
		break;
	}

exit:
	return ret;
}

static const struct amdgv_lm_funcs mi200_lm_funcs = {
	.get_migration_version = mi200_migration_get_migration_version,
	.get_manifest_size = mi200_migration_get_manifest_size,
	.send_transfer_cmd = mi200_migration_send_transfer_cmd,
	.psp_export = mi200_migration_psp_export,
	.psp_import = mi200_migration_psp_import,
};

static int mi200_migration_sw_init(struct amdgv_adapter *adapt)
{
	/* Init memory for future migration data in FB */
	if (mi200_migration_mem_init(adapt) != 0) {
		AMDGV_ERROR("Failed to initialize migration data memory.\n");
		goto fail;
	}

	adapt->live_migration.lm_lock = oss_mutex_init();
	if (adapt->live_migration.lm_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	/* Add reference for psp frameworks */
	adapt->live_migration.lm_funcs = &mi200_lm_funcs;

	return 0;

fail:
	mi200_migration_mem_fini(adapt);

	return 0;
}

static int mi200_migration_sw_fini(struct amdgv_adapter *adapt)
{
	mi200_migration_mem_fini(adapt);

	adapt->live_migration.lm_funcs = NULL;

	if (adapt->live_migration.lm_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->live_migration.lm_lock);
		adapt->live_migration.lm_lock = OSS_INVALID_HANDLE;
	}

	return 0;
}

static int mi200_migration_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi200_migration_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_live_migration_func = {
	.name = "mi200_live_migration_func",
	.sw_init = mi200_migration_sw_init,
	.sw_fini = mi200_migration_sw_fini,
	.hw_init = mi200_migration_hw_init,
	.hw_fini = mi200_migration_hw_fini,
};

