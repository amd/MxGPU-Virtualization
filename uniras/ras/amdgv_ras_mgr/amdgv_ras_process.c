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
 * THE SOFTWARE.
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_oss_wrapper.h"
#include "ras_sys.h"
#include "ras_process.h"
#include "amdgv_ras_mgr.h"
#include "amdgv_ras_process.h"

#define RAS_MGR_RETIRE_PAGE_INTERVAL  100

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

/* Deferred reset gpu can allow driver to read the ecc
 * information first.
 */
static int amdgv_ras_process_defer_reset_gpu(struct amdgv_adapter *adapt,
				uint32_t idx_vf, uint32_t block)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_event_req req = {
		.idx_vf = idx_vf,
		.block = block,
		.reset = GPU_RESET_CAUSE_POISON,
	};

	return ras_process_add_interrupt_req(ras_mgr->ras_core, &req, false);
}

int amdgv_ras_process_handle_umc_interrupt(struct amdgv_adapter *adapt, uint32_t idx_vf, void *data)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);

	if (!ras_mgr->ras_core)
		return -RAS_CORE_EINVAL;

	return ras_process_add_interrupt_req(ras_mgr->ras_core, NULL, true);
}

int amdgv_ras_process_handle_consumption_interrupt(struct amdgv_adapter *adapt,
		uint32_t idx_vf, void *data)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	uint32_t ras_block;

	if (!data)
		return -RAS_CORE_EINVAL;

	ras_block = *(uint32_t *)data;

	if (ras_core_poison_supported(ras_mgr->ras_core) && (idx_vf != AMDGV_PF_IDX)) {
		switch (ras_block) {
		case RAS_BLOCK_ID__GFX:
		case RAS_BLOCK_ID__SDMA:
		case RAS_BLOCK_ID__VCN:
		case RAS_BLOCK_ID__JPEG:
			if (adapt->flags & AMDGV_FLAG_VF_HANG_GPU_RESET) {
				AMDGV_INFO("Reset mode is set to MODE1. Queue WGR\n");
				amdgv_ras_process_defer_reset_gpu(adapt, AMDGV_PF_IDX, ras_block);
			} else {
				amdgv_sched_queue_event(adapt, idx_vf,
					AMDGV_EVENT_SCHED_FORCE_RESET_VF, AMDGV_SCHED_BLOCK_ALL);
			}
			break;
		default:
			break;
		}
	} else {
		adapt->reset.reset_mode = adapt->umc.reset_mode;
		if (adapt->xgmi.master_adapt) {
			AMDGV_INFO("Forwarding reset event to master adapter:0x%x\n",
				   adapt->xgmi.master_adapt->bdf);
			adapt = adapt->xgmi.master_adapt;
		}

		amdgv_sched_queue_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_FORCE_RESET_GPU, 0);
	}

	return 0;
}

int amdgv_ras_process_ras_event_dispatch(struct amdgv_adapter *adapt,
		uint32_t idx_vf)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);

	if (!ras_mgr || !ras_mgr->ras_is_ready)
		return AMDGV_FAILURE;

	return ras_process_handle_ras_event(ras_mgr->ras_core);
}

