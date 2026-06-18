/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

static int amdgv_migration_rlc_autoload(struct amdgv_adapter *adapt, uint32_t vf_idx)
{
       return (adapt->psp.migration_rlc_autoload) ?
               adapt->psp.migration_rlc_autoload(adapt, vf_idx) : 0;
}

void amdgv_live_migration_set_abort_all(struct amdgv_adapter *adapt)
{
	int i;

	for (i = 0; i < adapt->num_vf; i++) {
		if (adapt->live_migration.mig_state[i].state == AMDGV_MIGRATION_VF_STATE_PRE_COPY ||
		    adapt->live_migration.mig_state[i].state == AMDGV_MIGRATION_VF_STATE_STOP_COPY) {
			AMDGV_DEBUG("VF[%d] migration aborted.\n", i);
			AMDGV_MIGRATION_SET_ABORT(adapt, i);
			adapt->live_migration.mig_state[i].is_target = false;
		}
	}
}

int amdgv_live_migration_set_vf_mig_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   enum amdgv_migration_vf_state state)
{
	if (idx_vf == AMDGV_PF_IDX)
		return 0;

	AMDGV_ASSERT(idx_vf < AMDGV_MAX_VF_NUM);

	switch (state) {
	case AMDGV_MIGRATION_VF_STATE_DEFAULT:
		AMDGV_MIGRATION_CLEAR_ABORT(adapt, idx_vf);
		adapt->dirtybit.acc_bits[idx_vf].is_first_query = false;
		break;
	case AMDGV_MIGRATION_VF_STATE_PRE_COPY:
		/* Always have pre_copy */
		adapt->dirtybit.acc_bits[idx_vf].is_first_query = true;
		break;
	case AMDGV_MIGRATION_VF_STATE_STOP_COPY:
		if (adapt->live_migration.mig_state[idx_vf].state != AMDGV_MIGRATION_VF_STATE_PRE_COPY)
			adapt->dirtybit.acc_bits[idx_vf].is_first_query = true;
		break;
	default:
		break;
	}

	adapt->live_migration.mig_state[idx_vf].state = state;

	return 0;
}

void amdgv_live_migration_abort_check(struct amdgv_adapter *adapt, uint32_t idx_vf, enum amdgv_sched_event_id event_id)
{
	int i;

	if (idx_vf == AMDGV_PF_IDX)
		return;

	AMDGV_ASSERT(idx_vf < AMDGV_MAX_VF_NUM);

	switch (event_id) {
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU:
	case AMDGV_EVENT_SCHED_FORCE_RESET_GPU_INTERNAL:
	case AMDGV_EVENT_SCHED_RMA:
	case AMDGV_EVENT_SCHED_RAS_FED:
	case AMDGV_EVENT_SCHED_RAS_POISON_CREATION:
		for (i = 0; i < adapt->num_vf; i++) {
			if (adapt->live_migration.mig_state[i].state == AMDGV_MIGRATION_VF_STATE_PRE_COPY ||
				adapt->live_migration.mig_state[i].state == AMDGV_MIGRATION_VF_STATE_STOP_COPY) {
				AMDGV_DEBUG("VF[%d] migration aborted by event %d\n", i, event_id);
				AMDGV_MIGRATION_SET_ABORT(adapt, i);
				adapt->live_migration.mig_state[i].is_target = false;
			}
		}
		break;
	case AMDGV_EVENT_REQ_GPU_INIT:
	case AMDGV_EVENT_REL_GPU_INIT:
	case AMDGV_EVENT_REQ_GPU_FINI:
	case AMDGV_EVENT_REL_GPU_FINI:
	case AMDGV_EVENT_REQ_GPU_RESET:
	case AMDGV_EVENT_REQ_GPU_INIT_DATA:
	case AMDGV_EVENT_SCHED_FORCE_RESET_VF:
	case AMDGV_EVENT_SCHED_RESET_VF:
	case AMDGV_EVENT_HW_SCHED_RESET_VF:
	case AMDGV_EVENT_SCHED_REMOVE_VF:
	case AMDGV_EVENT_SCHED_STOP_VF:
	case AMDGV_EVENT_SCHED_RAS_UMC:
	case AMDGV_EVENT_SCHED_SUSPEND_LIVE:
	case AMDGV_EVENT_SCHED_RESUME_LIVE:
	case AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC:
	case AMDGV_EVENT_HANDLE_CRASH:
	case AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION:
	case AMDGV_EVENT_SCHED_VF_REQ_GPU_INIT_XCHG_REGION:
		if (adapt->live_migration.mig_state[idx_vf].state == AMDGV_MIGRATION_VF_STATE_PRE_COPY ||
			adapt->live_migration.mig_state[idx_vf].state == AMDGV_MIGRATION_VF_STATE_STOP_COPY) {
			AMDGV_DEBUG("VF[%d] migration aborted by event %d\n", idx_vf, event_id);
			AMDGV_MIGRATION_SET_ABORT(adapt, idx_vf);
			adapt->live_migration.mig_state[idx_vf].is_target = false;
		}
		break;
	default:
		break;
	}
}

static int amdgv_migration_get_migration_info(struct amdgv_adapter *adapt)
{
	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->migration_smu_is_supported) {
		/* Check if PMFW support live migration. */
		if (!adapt->pp.pp_funcs->migration_smu_is_supported(adapt)){
			AMDGV_ERROR("Migration SMU is not supported.\n");
			return PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
		}
	}

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

	if (type == AMDGV_MIGRATION_EXPORT_STATIC_DATA || type == AMDGV_MIGRATION_IMPORT_STATIC_DATA)
		oss_memset(amdgv_memmgr_get_cpu_addr(adapt->live_migration.static_data_mem), 0,
				amdgv_memmgr_get_size(adapt->live_migration.static_data_mem));
	else
		oss_memset(amdgv_memmgr_get_cpu_addr(adapt->live_migration.dynamic_data_mem), 0,
				amdgv_memmgr_get_size(adapt->live_migration.dynamic_data_mem));

	switch (type) {
	case AMDGV_MIGRATION_EXPORT_STATIC_DATA:
		mem = adapt->live_migration.static_data_mem;
		AMDGV_DEBUG("Migration Export: PSP static import MEC, VCN, SDMA FW\n");
		if (mem == NULL)
			goto exit;

		if (amdgv_migration_get_psp_data_size(adapt, &size,
					AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA))
			goto exit;

		ret = amdgv_psp_transfer_manifest_data(adapt, idx_vf,
					amdgv_memmgr_get_gpu_addr(mem), size, PSP_MIGRATION_EXPORT_STATIC_DATA);
		if (ret)
			goto exit;

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
		if (ret)
			goto exit;

		oss_memcpy(data_addr, amdgv_memmgr_get_cpu_addr(mem), size);
		break;
	case AMDGV_MIGRATION_IMPORT_PREPARE:
		ret = amdgv_misc_clear_vf_fb(adapt, idx_vf, 0x00);
		if (ret)
			return ret;

		ret = amdgv_dirtybit_clear_fb_dbit(adapt, idx_vf);
		if (ret)
			return ret;

		/* Save current PF, init VF on all blocks */
		AMDGV_DEBUG("Migration Import: Init target VF on all blocks\n");
		adapt->live_migration.mig_state[idx_vf].is_target = true;
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
		amdgv_sched_handle_req_gpu_init_data(adapt, idx_vf);
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
		if (ret)
			goto exit;
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
		if (ret)
			goto exit;

		AMDGV_DEBUG("Migration Import: Send TRANSFER_VF_DATA to MMSCH and RLCV\n");
		if (amdgv_gpuiov_transfer_vf_data(adapt, idx_vf, false)) {
			ret = AMDGV_FAILURE;
			goto exit;
		}

		// Autoload rlc
		ret = amdgv_migration_rlc_autoload(adapt, idx_vf);
		if (ret) {
			AMDGV_ERROR("Failed to do migration psp rlc autoload.\n");
			goto exit;
		}

		break;
	default:
		ret = AMDGV_FAILURE;
		break;
	}
exit:
	if (mem)
		oss_memset(amdgv_memmgr_get_cpu_addr(mem), 0, size);

	return ret;
}

static int amdgv_migration_sw_init(struct amdgv_adapter *adapt)
{
	if (adapt->asic_type == CHIP_NAVI32 || adapt->asic_type == CHIP_MI350X)
		adapt->live_migration.mig_data_size_cap = true;
	else
		adapt->live_migration.mig_data_size_cap = false;

	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	if (adapt->memmgr_pf.is_init) {
		if (adapt->live_migration.static_data_mem == NULL) {
			adapt->live_migration.static_data_mem =
				amdgv_memmgr_alloc(&adapt->memmgr_pf,
						adapt->live_migration.static_data_size,
						MEM_MIGRATION_PSP_STATIC_DATA);
			if (!adapt->live_migration.static_data_mem) {
				amdgv_put_error(AMDGV_PF_IDX,
						AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL,
						adapt->live_migration.static_data_size);
				return AMDGV_FAILURE;
			}
		}

		if (adapt->live_migration.dynamic_data_mem == NULL) {
			adapt->live_migration.dynamic_data_mem =
				amdgv_memmgr_alloc(&adapt->memmgr_pf,
						adapt->live_migration.dynamic_data_size,
						MEM_MIGRATION_PSP_DYNAMIC_DATA);
			if (!adapt->live_migration.dynamic_data_mem) {
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL,
						adapt->live_migration.dynamic_data_size);
				return AMDGV_FAILURE;
			}
		}

		AMDGV_DEBUG("Migration mem init: static_size=%lu, dynamic_size=%lu\n",
				adapt->live_migration.static_data_size,
				adapt->live_migration.dynamic_data_size);
	} else {
		AMDGV_WARN("memmgr_pf is not initialized, skipped live migration.\n");
		return AMDGV_FAILURE;
	}

	if (!adapt->memmgr_sys.is_init) {
		if (!(adapt->flags & AMDGV_FLAG_USE_PF) &&
			!(adapt->flags & AMDGV_FLAG_DISABLE_SYS_APERTURE)) {
			if (!adapt->sys_mem_info.va_ptr) {
				if (oss_alloc_dma_mem(adapt->dev, AMDGV_AGP_APERTURE_SIZE,
						OSS_DMA_MEM_CACHEABLE, &adapt->sys_mem_info)) {
					AMDGV_WARN("Failed to allocate %dMB continuous dma memory\n",
					AMDGV_AGP_APERTURE_SIZE >> 20);
					return AMDGV_FAILURE;
				} else {
					AMDGV_DEBUG("ma = 0x%llx, va = %p\n",
						adapt->sys_mem_info.bus_addr, adapt->sys_mem_info.va_ptr);
				}
			}
		}
	}

	adapt->live_migration.migration_version = AMDGV_MIGRATION_VERSION_UNINITIALIZED;
	return 0;
}

static int amdgv_migration_sw_fini(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

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

	return 0;
}

static int amdgv_migration_hw_init(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	if (amdgv_ual_is_supported(adapt)) {
		AMDGV_ERROR("Live migration is not supported with UALoE / UALINK enabled.\n");
		return AMDGV_FAILURE;
	}

	amdgv_migration_get_migration_info(adapt);
	return 0;
}

static int amdgv_migration_hw_fini(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	return 0;
}

struct amdgv_init_func amdgv_migration_func = {
	.name = "amdgv_migration_func",
	.sw_init = amdgv_migration_sw_init,
	.sw_fini = amdgv_migration_sw_fini,
	.hw_init = amdgv_migration_hw_init,
	.hw_fini = amdgv_migration_hw_fini,
	.hw_live_init = amdgv_migration_hw_init,
};