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

/*
 * Copy manifest data between the PF VRAM scratch buffer and the host buffer via
 * amdgv_sched_event_do_fb_copy(): SDMA/LSDMA when the host end is GPU-addressable
 * (gpu_addr != 0), else pass -1 for that end to force the CPU copy.
 * to_host: true = scratch->host (export), false = host->scratch (import).
 */
static int amdgv_migration_move_manifest_data(struct amdgv_adapter *adapt,
		uint32_t idx_vf, struct amdgv_memmgr_mem *mem, void *cpu_addr,
		uint64_t gpu_addr, uint64_t size, bool to_host)
{
	uint64_t scratch_gpu_addr = amdgv_memmgr_get_gpu_addr(mem);
	void *scratch_cpu_addr = amdgv_memmgr_get_cpu_addr(mem);
	uint64_t host_gpu_addr = gpu_addr ? gpu_addr : (uint64_t)-1;

	return to_host ?
		amdgv_sched_event_do_fb_copy(adapt, idx_vf, scratch_gpu_addr, host_gpu_addr,
				size, scratch_cpu_addr, cpu_addr) :
		amdgv_sched_event_do_fb_copy(adapt, idx_vf, host_gpu_addr, scratch_gpu_addr,
				size, cpu_addr, scratch_cpu_addr);
}

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

static int amdgv_migration_export_bad_pages(struct amdgv_adapter *adapt,
					    void *data_addr, uint64_t data_offset)
{
	struct ras_err_handler_data *data;
	struct amdgv_migration_bad_page_section *section;
	uint64_t mc_base_pfn;
	int i;

	section = (struct amdgv_migration_bad_page_section *)((uint8_t *)data_addr + data_offset);
	section->magic_number = AMDGV_MIGRATION_BAD_PAGE_MAGIC;
	if (!adapt->ecc.eh_data) {
		section->sorted_bp_count = 0;
		return 0;
	}
	oss_mutex_lock(adapt->ecc.recovery_lock);

	data = adapt->ecc.eh_data;
	if (!data || !data->sorted_bps) {
		section->sorted_bp_count = 0;
		oss_mutex_unlock(adapt->ecc.recovery_lock);
		return 0;
	}

	if (data->sorted_bp_count > MAX_BAD_PAGE_THRESHOLD) {
		oss_mutex_unlock(adapt->ecc.recovery_lock);
		AMDGV_ERROR("Bad page count %d exceeds threshold %u.\n",
			    data->sorted_bp_count, MAX_BAD_PAGE_THRESHOLD);
		return AMDGV_FAILURE;
	}

	mc_base_pfn = adapt->memmgr_pf.mc_base >> AMDGV_GPU_PAGE_SHIFT;
	section->sorted_bp_count = data->sorted_bp_count;
	for (i = 0; i < data->sorted_bp_count; i++)
		section->sorted_bp_offsets[i] = data->sorted_bps[i] - mc_base_pfn;

	oss_mutex_unlock(adapt->ecc.recovery_lock);
	return 0;
}

static int amdgv_migration_import_bad_pages(struct amdgv_adapter *adapt,
					    uint32_t idx_vf,
					    void *data_addr, uint64_t data_offset)
{
	struct amdgv_migration_bad_page_section *section;
	struct amdgv_vf_migration_state *mig_state;
	struct ras_err_handler_data *data;
	uint64_t mc_base_pfn = adapt->memmgr_pf.mc_base >> AMDGV_GPU_PAGE_SHIFT;
	int src_idx = 0;
	int dst_idx;
	int unique_count = 0;

	mig_state = &adapt->live_migration.mig_state[idx_vf];

	section = (struct amdgv_migration_bad_page_section *)((uint8_t *)data_addr + data_offset);
	if (section->magic_number != AMDGV_MIGRATION_BAD_PAGE_MAGIC) {
		AMDGV_WARN("No valid bad page section (magic number: 0x%llx), skip importing bad pages.\n",
			   section->magic_number);
		return 0;
	}

	if (!adapt->ecc.eh_data)
		return 0;

	if (section->sorted_bp_count > MAX_BAD_PAGE_THRESHOLD) {
		AMDGV_ERROR("Source bad page count %llu exceeds threshold %u.\n",
			    section->sorted_bp_count, MAX_BAD_PAGE_THRESHOLD);
		return AMDGV_FAILURE;
	}

	if (mig_state->dst_unique_bps == NULL) {
		mig_state->dst_unique_bps = oss_zalloc(AMDGV_MIGRATION_BAD_PAGES_DATA_SIZE);
		if (mig_state->dst_unique_bps == NULL) {
			AMDGV_ERROR("Failed to allocate memory for destination unique bad pages.\n");
			return AMDGV_FAILURE;
		}
	} else {
		oss_memset(mig_state->dst_unique_bps, 0, AMDGV_MIGRATION_BAD_PAGES_DATA_SIZE);
	}
	mig_state->dst_unique_bp_count = 0;

	oss_mutex_lock(adapt->ecc.recovery_lock);
	data = adapt->ecc.eh_data;
	if (data && data->sorted_bps) {

		if (data->sorted_bp_count > MAX_BAD_PAGE_THRESHOLD) {
			AMDGV_ERROR("Destination bad page count %d exceeds threshold %u.\n",
				    data->sorted_bp_count, MAX_BAD_PAGE_THRESHOLD);
			oss_mutex_unlock(adapt->ecc.recovery_lock);
			return AMDGV_FAILURE;
		}

		for (dst_idx = 0; dst_idx < data->sorted_bp_count; dst_idx++) {
			while (src_idx < section->sorted_bp_count &&
			       (section->sorted_bp_offsets[src_idx] + mc_base_pfn) <
				       data->sorted_bps[dst_idx])
				src_idx++;

			if (src_idx < section->sorted_bp_count &&
			    (section->sorted_bp_offsets[src_idx] + mc_base_pfn) ==
				    data->sorted_bps[dst_idx])
				continue;

			mig_state->dst_unique_bps[unique_count++] = data->sorted_bps[dst_idx];
		}
	}
	mig_state->dst_unique_bp_count = unique_count;
	oss_mutex_unlock(adapt->ecc.recovery_lock);

	AMDGV_DEBUG("Migration Import: source has %llu bad pages, destination has %d unique bad pages.\n",
		    section->sorted_bp_count, unique_count);
	return 0;
}

static int amdgv_migration_prepare_transfer_vf_data(struct amdgv_adapter *adapt,
						    uint32_t idx_vf)
{
	int ret = 0;

	/*
	 * Only ASICs that require the GFX engine in PF context for RLCV CP DMA
	 * during TRANSFER_VF_DATA (e.g. Navi32) perform this context switch. The
	 * capability is set during init; other ASICs skip it.
	 */
	if (!adapt->live_migration.need_gfx_pf_ctx_switch)
		return 0;

	/* RLCV needs PF context for CP DMA (both export and import). */
	ret = amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);
	if (ret)
		AMDGV_ERROR("Failed to switch to PF on GFX block\n");

	return ret;
}

int amdgv_migration_transfer_manifest_data(struct amdgv_adapter *adapt, struct amdgv_sched_event *event)
{
	int ret = AMDGV_FAILURE;
	uint32_t idx_vf = event->idx_vf;
	enum amdgv_migration_manifest_data_type type = event->data.lm.type;
	void *data_addr = (void *)event->data.lm.addr;
	uint64_t gpu_addr = event->data.lm.gpu_addr;
	uint64_t size = 0;
	struct amdgv_memmgr_mem *mem = NULL;
	struct amdgv_vf_migration_state *mig_state = &adapt->live_migration.mig_state[idx_vf];

	switch (type) {
	case AMDGV_MIGRATION_EXPORT_STATIC_DATA:
		amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_STATIC, 0);
		mem = adapt->live_migration.static_data_mem;

		AMDGV_DEBUG("Migration Export: PSP static import MEC, VCN, SDMA FW\n");
		if (mem == NULL) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_STATIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_psp_transfer_manifest_data(adapt, idx_vf,
					amdgv_memmgr_get_gpu_addr(mem),
					AMDGV_MIGRATION_MAX_PSP_STATIC_DATA_SIZE,
					PSP_MIGRATION_EXPORT_STATIC_DATA);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_STATIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_migration_export_bad_pages(adapt, amdgv_memmgr_get_cpu_addr(mem),
				AMDGV_MIGRATION_MAX_PSP_STATIC_DATA_SIZE);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_STATIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_migration_move_manifest_data(adapt, idx_vf, mem, data_addr,
				gpu_addr, AMDGV_MIGRATION_STATIC_DATA_SIZE, true);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_STATIC_FAILED, 0);
			goto exit;
		}
		break;
	case AMDGV_MIGRATION_EXPORT_DYNAMIC_DATA:
		amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_DYNAMIC, 0);
		mem = adapt->live_migration.dynamic_data_mem;
		AMDGV_DEBUG("Migration Export: Send TRANSFER_VF_DATA to MMSCH and RLCV\n");
		if (mem == NULL) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		if (amdgv_migration_get_psp_data_size(adapt, &size,
					AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA)) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		if (amdgv_migration_prepare_transfer_vf_data(adapt, idx_vf)) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		if (amdgv_gpuiov_transfer_vf_data(adapt, idx_vf, true)) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_psp_transfer_manifest_data(adapt, idx_vf,
					amdgv_memmgr_get_gpu_addr(mem), size, PSP_MIGRATION_EXPORT_DYNAMIC_DATA);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_migration_move_manifest_data(adapt, idx_vf, mem, data_addr,
				gpu_addr, size, true);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_EXPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}
		break;
	case AMDGV_MIGRATION_IMPORT_PREPARE:
		amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_PREPARE, 0);
		adapt->live_migration.mig_state[idx_vf].is_target = true;

		ret = amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_PREPARE_FAILED, 0);
			goto exit;
		}

		ret = amdgv_mmsch_config_vf(adapt, idx_vf);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_PREPARE_FAILED, 0);
			goto exit;
		}

		AMDGV_DEBUG("Migration Import: Init target VF on all blocks\n");
		ret = amdgv_sched_context_init(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_PREPARE_FAILED, 0);
			goto exit;
		}

		AMDGV_DEBUG("Migration Import: Switch to PF on all blocks\n");
		ret = amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_PREPARE_FAILED, 0);
			goto exit;
		}

		AMDGV_DEBUG("Migration Import: Enable fb/mmio/doorbell write access\n");
		ret = amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_ALL, true);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_PREPARE_FAILED, 0);
			goto exit;
		}

		amdgv_sched_handle_req_gpu_init_data(adapt, idx_vf);
		break;
	case AMDGV_MIGRATION_IMPORT_STATIC_DATA:
		amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_STATIC, 0);
		AMDGV_DEBUG("Migration Import: PSP static import MEC, VCN, SDMA FW\n");
		mem = adapt->live_migration.static_data_mem;
		if (mem == NULL) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_STATIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_migration_get_psp_data_size(adapt, &size, AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_STATIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_migration_move_manifest_data(adapt, idx_vf, mem, data_addr,
				gpu_addr, event->data.lm.size ? event->data.lm.size : size, false);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_STATIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_psp_transfer_manifest_data(adapt, idx_vf,
					amdgv_memmgr_get_gpu_addr(mem),
					event->data.lm.size ? event->data.lm.size : size,
					PSP_MIGRATION_IMPORT_STATIC_DATA);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_STATIC_FAILED, 0);
			goto exit;
		}

		if (mig_state->dst_unique_bps) {
			oss_free(mig_state->dst_unique_bps);
			mig_state->dst_unique_bps = NULL;
		}
		mig_state->dst_unique_bp_count = 0;

		if (!event->data.lm.size || event->data.lm.size == size) {
			ret = amdgv_migration_import_bad_pages(adapt, idx_vf, data_addr,
							       AMDGV_MIGRATION_MAX_PSP_STATIC_DATA_SIZE);
			if (ret) {
				amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_STATIC_FAILED, 0);
				goto exit;
			}
		}
		set_to_suspend_vf(idx_vf);
		break;
	case AMDGV_MIGRATION_IMPORT_DYNAMIC_DATA:
		amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_DYNAMIC, 0);
		mem = adapt->live_migration.dynamic_data_mem;
		if (mem == NULL) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		if (amdgv_migration_get_psp_data_size(adapt, &size,
					AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA)) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_migration_move_manifest_data(adapt, idx_vf, mem, data_addr,
				gpu_addr, size, false);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}
		ret = amdgv_psp_transfer_manifest_data(adapt, idx_vf,
					amdgv_memmgr_get_gpu_addr(mem), size, PSP_MIGRATION_IMPORT_DYNAMIC_DATA);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		// Autoload rlc
		ret = amdgv_migration_rlc_autoload(adapt, idx_vf);
		if (ret) {
			AMDGV_ERROR("Failed to do migration psp rlc autoload.\n");
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		ret = amdgv_migration_prepare_transfer_vf_data(adapt, idx_vf);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_DYNAMIC_FAILED, 0);
			goto exit;
		}

		AMDGV_DEBUG("Migration Import: Send TRANSFER_VF_DATA to MMSCH and RLCV\n");
		ret = amdgv_gpuiov_transfer_vf_data(adapt, idx_vf, false);
		if (ret) {
			amdgv_put_log(idx_vf, AMDGV_LOG_IOV_LIVE_MIGRATION_IMPORT_DYNAMIC_FAILED, 0);
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
				amdgv_put_log(AMDGV_PF_IDX,
						AMDGV_LOG_DRIVER_ALLOC_FB_MEM_FAIL,
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
				amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_FB_MEM_FAIL,
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
	int i;

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

	for (i = 0; i < adapt->num_vf; i++) {
		if (adapt->live_migration.mig_state[i].dst_unique_bps) {
			oss_free(adapt->live_migration.mig_state[i].dst_unique_bps);
			adapt->live_migration.mig_state[i].dst_unique_bps = NULL;
			adapt->live_migration.mig_state[i].dst_unique_bp_count = 0;
		}
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