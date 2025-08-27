/*
 * Copyright 2022-2023 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "amdgv_device.h"
#include "amdgv_live_migration.h"
#include "amdgv_sriovmsg.h"
#include "amdgv.h"
#include "amdgv_api.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_list.h"
#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "amdgv_guard.h"

static const uint32_t this_block = AMDGV_LIVE_MIGRATION_BLOCK;


static int amdgv_migration_get_migration_info(struct amdgv_adapter *adapt)
{
	return (adapt->psp.get_migration_info) ?
		adapt->psp.get_migration_info(adapt) :
		PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
}

void amdgv_migration_set_ctx_version(struct amdgv_adapter *adapt,
				     enum amdgv_migration_context_version version)
{
	adapt->live_migration.context_version = version;
}

int amdgv_migration_get_migration_version(struct amdgv_adapter *adapt,
					  uint32_t *migration_version)
{
	*migration_version = adapt->live_migration.migration_version;

	return 0;
}

int amdgv_migration_get_psp_data_size(struct amdgv_adapter *adapt, uint64_t *size,
				      enum amdgv_migration_data_section section)
{
	switch (section) {
	case AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA:
		*size = adapt->live_migration.static_data_size;
		break;
	case AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA:
		*size = adapt->live_migration.dynamic_data_size;
		break;
	default:
		AMDGV_ERROR("Invalid section.\n");
		return AMDGV_FAILURE;
	}
	return 0;
}

int amdgv_migration_transfer_manifest_data(struct amdgv_adapter *adapt, struct amdgv_sched_event *event)
{
	int ret = AMDGV_FAILURE;
	uint32_t idx_vf = event->idx_vf;
	enum amdgv_migration_manifest_data_type type = event->data.lm.type;
	void *data_addr = (void *)event->data.lm.addr;
	uint64_t size = 0;
	struct amdgv_memmgr_mem *mem = NULL;

	switch (type) {
	case AMDGV_MIGRATION_EXPORT_STATIC_DATA:
		mem = adapt->live_migration.static_data_mem;
		AMDGV_DEBUG("Migration Export: PSP static import MEC, VCN, SDMA FW\n");
		if (mem == NULL)
			goto exit;

		if (amdgv_migration_get_psp_data_size(adapt, &size,
					AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA))
			goto exit;

#ifdef AMDGV_MIGRATION_DEBUG
		/* Output RWL values in dynamic export pkg for debugging purpose */
		if ((adapt->psp.print_rwl == NULL) ||
			(adapt->psp.print_rwl(adapt, idx_vf))) {
			ret = AMDGV_FAILURE;
			goto exit;
		}
#endif
		ret = amdgv_psp_transfer_manifest_data(adapt, idx_vf,
					amdgv_memmgr_get_gpu_addr(mem), size, PSP_MIGRATION_EXPORT_STATIC_DATA);
		if (ret) {
			AMDGV_ERROR("Failed to do migration psp static export.\n");
			goto exit;
		}

		oss_memcpy(data_addr, amdgv_memmgr_get_cpu_addr(mem), size);
		break;
	case AMDGV_MIGRATION_EXPORT_DYNAMIC_DATA:
		mem = adapt->live_migration.dynamic_data_mem;
		AMDGV_DEBUG("Migration Export: Send TRANSFER_VF_DATA to MMSCH and RLCV\n");
		if (mem == NULL)
			goto exit;

		if (amdgv_migration_get_psp_data_size(adapt, &size,
					AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA))
			goto exit;

		if (amdgv_gpuiov_transfer_vf_data(adapt, idx_vf, true))
			goto exit;

		ret = amdgv_psp_transfer_manifest_data(adapt, idx_vf,
					amdgv_memmgr_get_gpu_addr(mem), size, PSP_MIGRATION_EXPORT_DYNAMIC_DATA);
		if (ret) {
			AMDGV_ERROR("Failed to do migration psp static export.\n");
			goto exit;
		}

		oss_memcpy(data_addr, amdgv_memmgr_get_cpu_addr(mem), size);
		break;
	case AMDGV_MIGRATION_IMPORT_PREPARE:
		/* Save current PF, init VF on all blocks */
		AMDGV_DEBUG("Migration Import: Init target VF on all blocks\n");
		ret = 0;
		if (amdgv_sched_context_init(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL)) {
			AMDGV_DEBUG("Failed to init VF%d WS context.\n", idx_vf);
			ret = AMDGV_FAILURE;
		}
		/* Switch to PF on all blocks */
		AMDGV_DEBUG("Migration Import: Switch to PF on all blocks\n");
		if (amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX,
						     AMDGV_SCHED_BLOCK_ALL)) {
			AMDGV_DEBUG("Failed to switch to PF on all blocks.\n");
			ret = AMDGV_FAILURE;
		}
		AMDGV_DEBUG("Migration Import: Enable fb/mmio/doorbell write access\n");
		if (amdgv_gpuiov_set_vf_access(
			    adapt, idx_vf,
			    AMDGV_VF_ACCESS_ALL, true)) {
			AMDGV_DEBUG("Failed to enable mmio/doorbell write access.\n");
			ret = AMDGV_FAILURE;
		}
		break;
	case AMDGV_MIGRATION_IMPORT_STATIC_DATA:
		AMDGV_DEBUG("Migration Import: PSP static import MEC, VCN, SDMA FW\n");
		mem = adapt->live_migration.static_data_mem;
		if (mem == NULL)
			goto exit;

		if (amdgv_migration_get_psp_data_size(adapt, &size,
					AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA))
			goto exit;

		oss_memcpy(amdgv_memmgr_get_cpu_addr(mem), data_addr, size);
		ret = amdgv_psp_transfer_manifest_data(adapt, idx_vf,
					amdgv_memmgr_get_gpu_addr(mem), size, PSP_MIGRATION_IMPORT_STATIC_DATA);
		if (ret) {
			AMDGV_ERROR("Failed to do migration psp dynamic import.\n");
			goto exit;
		}
		set_to_suspend_vf(idx_vf);
		break;
	case AMDGV_MIGRATION_IMPORT_DYNAMIC_DATA:
		mem = adapt->live_migration.dynamic_data_mem;
		if (mem == NULL)
			goto exit;

		if (amdgv_migration_get_psp_data_size(adapt, &size,
					AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA))
			goto exit;

		oss_memcpy(amdgv_memmgr_get_cpu_addr(mem), data_addr, size);
		ret = amdgv_psp_transfer_manifest_data(adapt, idx_vf,
					amdgv_memmgr_get_gpu_addr(mem), size, PSP_MIGRATION_IMPORT_DYNAMIC_DATA);

		if (amdgv_gpuiov_transfer_vf_data(adapt, idx_vf, false))
			goto exit;

		if (ret) {
			AMDGV_ERROR("Failed to do migration psp dynamic import.\n");
			goto exit;
		}

		AMDGV_DEBUG("Migration Import: Idle/save PF\n");
		if (amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL)) {
			AMDGV_DEBUG("Failed to idle/save PF on all blocks.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}

		AMDGV_DEBUG("Migration Import: Send TRANSFER_VF_DATA to MMSCH and RLCV\n");
		if (amdgv_gpuiov_transfer_vf_data(adapt, idx_vf, false)) {
			ret = AMDGV_FAILURE;
			goto exit;
		}

#ifdef AMDGV_MIGRATION_DEBUG
		/* Output RWL values in dynamic import pkg for debugging purpose */
		if ((adapt->psp.print_rwl == NULL) ||
			(adapt->psp.print_rwl(adapt, idx_vf))) {
			ret = AMDGV_FAILURE;
			goto exit;
		}
#endif
		break;
	default:
		ret = AMDGV_FAILURE;
		break;
	}
exit:
	if (mem)
		oss_memset(amdgv_memmgr_get_cpu_addr(mem), 0, size);

	adapt->live_migration.migration_status = ret;
	return ret;
}

int amdgv_migration_init(struct amdgv_adapter *adapt)
{
	uint64_t static_size = 0;
	uint64_t dynamic_size = 0;

	if (amdgv_migration_get_migration_info(adapt))
		return AMDGV_FAILURE;

	if (amdgv_migration_get_psp_data_size(adapt, &static_size, AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA) ||
		amdgv_migration_get_psp_data_size(adapt, &dynamic_size, AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA))
		return AMDGV_FAILURE;

	/* allocate static/dynamic data pkg inside visible framebuffer
	* if the memory manager is enabled.
	* Skip alloc for reset to avoid double allocation
	*/
	if (adapt->memmgr_pf.is_init) {
		//TODO: use one memory for static and dynamic data.
		if (adapt->live_migration.static_data_mem == NULL) {
			adapt->live_migration.static_data_mem =
				amdgv_memmgr_alloc(&adapt->memmgr_pf, static_size,
						MEM_MIGRATION_PSP_STATIC_DATA);
			if (!adapt->live_migration.static_data_mem) {
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL,
						static_size);
				return AMDGV_FAILURE;
			}
		}

		if (adapt->live_migration.dynamic_data_mem == NULL) {
			adapt->live_migration.dynamic_data_mem =
				amdgv_memmgr_alloc(&adapt->memmgr_pf, dynamic_size,
						MEM_MIGRATION_PSP_DYNAMIC_DATA);
			if (!adapt->live_migration.dynamic_data_mem) {
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL,
						dynamic_size);
				return AMDGV_FAILURE;
			}
		}
		adapt->live_migration.migration_status = 0;
		AMDGV_INFO("Migration mem init: static_size=%lu, dynamic_size=%lu\n",
		static_size, dynamic_size);
	} else {
		AMDGV_WARN("memmgr_pf is not initialized, skipped live migration.\n");
		return AMDGV_FAILURE;
	}


	oss_memset(amdgv_memmgr_get_cpu_addr(adapt->live_migration.static_data_mem), 0, static_size);
	oss_memset(amdgv_memmgr_get_cpu_addr(adapt->live_migration.dynamic_data_mem), 0, dynamic_size);
	return 0;
}

void amdgv_migration_fini(struct amdgv_adapter *adapt)
{
	if (adapt->live_migration.static_data_mem) {
		amdgv_memmgr_free(adapt->live_migration.static_data_mem);
		adapt->live_migration.static_data_mem = NULL;
	}
	if (adapt->live_migration.dynamic_data_mem) {
		amdgv_memmgr_free(adapt->live_migration.dynamic_data_mem);
		adapt->live_migration.dynamic_data_mem = NULL;
	}
}
