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

int amdgv_migration_get_migration_version(struct amdgv_adapter *adapt,
					  uint32_t *migration_version)
{
	int ret = 0;

	if (adapt->live_migration.lm_funcs &&
	    adapt->live_migration.lm_funcs->get_migration_version) {
		ret = adapt->live_migration.lm_funcs->get_migration_version(adapt,
									    migration_version);
	} else {
		AMDGV_ERROR("get_migration_version is not properly defined.");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_migration_get_psp_data_size(struct amdgv_adapter *adapt, uint64_t *size,
				      enum amdgv_migration_data_section section)
{
	int ret = 0;

	if (adapt->live_migration.lm_funcs &&
	    adapt->live_migration.lm_funcs->get_manifest_size) {
		ret = adapt->live_migration.lm_funcs->get_manifest_size(adapt, size, section);
	} else {
		AMDGV_ERROR("get_manifest_size is not properly defined.");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_migration_send_transfer_cmd(struct amdgv_adapter *adapt, uint32_t idx_vf,
				      bool to_export)
{
	int ret = 0;

	if (adapt->live_migration.lm_funcs &&
	    adapt->live_migration.lm_funcs->send_transfer_cmd) {
		ret = adapt->live_migration.lm_funcs->send_transfer_cmd(adapt, idx_vf,
									to_export);
	} else {
		AMDGV_ERROR("send_transfer_cmd is not properly defined.");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_migration_export_vf(struct amdgv_adapter *adapt, uint32_t idx_vf, void *data_dst,
			      enum amdgv_migration_export_phase phase)
{
	int ret = 0;
	bool vf_needs_restored = false;

	oss_mutex_lock(adapt->live_migration.lm_lock);

	/* If target VF is not suspended, suspend it first.
	 * if it's in phase 1 export, target VF will be restored to the state
	 * before enter this function. */
	if (!is_suspend_vf(idx_vf)) {
		AMDGV_DEBUG("Target VF%d is not suspended yet, suspend it now.\n", idx_vf);
		if (amdgv_sched_queue_event_and_wait(adapt, idx_vf,
						     AMDGV_EVENT_SCHED_SUSPEND_VF,
						     AMDGV_SCHED_BLOCK_ALL)) {
			AMDGV_ERROR("Failed to schedule suspend_VF.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}
		if (!is_suspend_vf(idx_vf)) {
			AMDGV_ERROR("Failed to suspend the target VF.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}
		vf_needs_restored =
			(phase == AMDGV_MIGRATION_EXPORT_PHASE1_STATIC_DATA) ? true : false;
	}

	AMDGV_DEBUG("Migration Export: Switch WS to PF on all blocks\n");
	if (amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL)) {
		AMDGV_ERROR("Failed to switch to PF on all blocks.\n");
		ret = AMDGV_FAILURE;
		goto exit;
	}

	if (phase == AMDGV_MIGRATION_EXPORT_PHASE2_DYNAMIC_DATA) {
		AMDGV_DEBUG("Migration Export: Send TRANSFER_VF_DATA to MMSCH and RLCV\n");
		if (amdgv_migration_send_transfer_cmd(adapt, idx_vf, true)) {
			ret = AMDGV_FAILURE;
			goto exit;
		}
	}

	AMDGV_DEBUG("Migration Export: PSP export phase%d\n", phase);
	if (adapt->live_migration.lm_funcs && adapt->live_migration.lm_funcs->psp_export) {
		ret = adapt->live_migration.lm_funcs->psp_export(adapt, idx_vf, data_dst,
								 phase);
	} else {
		AMDGV_ERROR("psp_export is not properly defined.");
		ret = AMDGV_FAILURE;
	}

	if (vf_needs_restored) {
		if (amdgv_sched_queue_event_and_wait(adapt, idx_vf,
						     AMDGV_EVENT_SCHED_RESUME_VF,
						     AMDGV_SCHED_BLOCK_ALL)) {
			AMDGV_ERROR("Failed to schedule resume_vf.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}
		if (!is_active_vf(idx_vf)) {
			AMDGV_ERROR("Failed to resume target VF.\n");
			ret = AMDGV_FAILURE;
		}
	}

exit:
	oss_mutex_unlock(adapt->live_migration.lm_lock);
	return ret;
}

int amdgv_migration_import_vf(struct amdgv_adapter *adapt, uint32_t idx_vf, void *data_src,
			      enum amdgv_migration_import_phase phase)
{
	int ret = 0;

	oss_mutex_lock(adapt->live_migration.lm_lock);

	switch (phase) {
	case AMDGV_MIGRATION_IMPORT_PHASE1_PREPARE:
		/* Save current PF, init VF on all blocks */
		AMDGV_DEBUG("Migration Import: Init target VF on all blocks\n");
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

		AMDGV_DEBUG("Migration Import: Enable mmio/doorbell write access\n");
		if (amdgv_gpuiov_set_vf_access(
			    adapt, idx_vf,
			    AMDGV_VF_ACCESS_MMIO_REG_WRITE | AMDGV_VF_ACCESS_DOORBELL, true)) {
			AMDGV_DEBUG("Failed to enable mmio/doorbell write access.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}
		break;

	case AMDGV_MIGRATION_IMPORT_PHASE2_STATIC_DATA:
		AMDGV_DEBUG("Migration Import: PSP static import MEC, VCN, SDMA FW\n");
		if (adapt->live_migration.lm_funcs &&
		    adapt->live_migration.lm_funcs->psp_import) {
			ret = adapt->live_migration.lm_funcs->psp_import(adapt, idx_vf, phase);
			AMDGV_DEBUG("Set VF to suspended\n");
			/* Once the VF is imported, set it to suspended so it can be resumed. */
			set_to_suspend_vf(idx_vf);
		} else {
			AMDGV_ERROR("psp_import is not properly defined.");
			ret = AMDGV_FAILURE;
		}
		break;

	case AMDGV_MIGRATION_IMPORT_PHASE3_DYNAMIC_DATA:
		AMDGV_DEBUG("Migration Import: PSP dynamic import MMSCH_CTX, SMU_CTX, "
			    "VCN0/1_RAM, PSP_CTX, SDMA_CTX, SRL, RWL\n");
		if (adapt->live_migration.lm_funcs &&
		    adapt->live_migration.lm_funcs->psp_import) {
			if (adapt->live_migration.lm_funcs->psp_import(adapt, idx_vf, phase)) {
				AMDGV_ERROR("Phase 3 PSP import failed.\n");
				ret = AMDGV_FAILURE;
				goto exit;
			}
		} else {
			AMDGV_ERROR("psp_import is not properly defined.");
			ret = AMDGV_FAILURE;
			goto exit;
		}

		AMDGV_DEBUG("Migration Import: Idle/save PF\n");
		if (amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL)) {
			AMDGV_DEBUG("Failed to idle/save PF on all blocks.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}

		AMDGV_DEBUG("Migration Import: Send TRANSFER_VF_DATA to MMSCH and RLCV\n");
		if (amdgv_migration_send_transfer_cmd(adapt, idx_vf, false)) {
			ret = AMDGV_FAILURE;
			goto exit;
		}

		AMDGV_DEBUG("Migration Import: Add VF back to WS\n");
		if (amdgv_sched_queue_event_and_wait(adapt, idx_vf,
						     AMDGV_EVENT_SCHED_RESUME_VF,
						     AMDGV_SCHED_BLOCK_ALL)) {
			AMDGV_ERROR("Failed to schedule resume_vf.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}
		if (!is_active_vf(idx_vf)) {
			AMDGV_ERROR("Failed to resume target VF.\n");
			ret = AMDGV_FAILURE;
		}

		AMDGV_DEBUG("Migration Import: Disable MMIO write access.\n");
		if (amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE,
					       false)) {
			AMDGV_ERROR("Failed to enable mmio protection.\n");
			ret = AMDGV_FAILURE;
			goto exit;
		}
		break;

	default:
		ret = AMDGV_FAILURE;
		break;
	}

exit:
	oss_mutex_unlock(adapt->live_migration.lm_lock);
	return ret;
}

