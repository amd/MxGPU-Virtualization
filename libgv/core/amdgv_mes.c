/*
 * Copyright 2026 Advanced Micro Devices, Inc.
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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "amdgv_mcp.h"
#include "amdgv_mes.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

#define AMDGV_MES_RESERVED_QUEUES	2	/* reserved for MES_KIQ and MES_SCHED */

int amdgv_mes_map_legacy_queue(struct amdgv_adapter *adapt,
	struct amdgv_ring *ring, uint32_t xcc_id)
{
	struct mes_map_legacy_queue_input queue_input;
	int r;

	oss_memset(&queue_input, 0, sizeof(queue_input));

	queue_input.xcc_id = xcc_id;
	queue_input.queue_type = ring->funcs->type;
	queue_input.doorbell_offset = ring->doorbell_index;
	queue_input.pipe_id = ring->pipe;
	queue_input.queue_id = ring->queue;
	queue_input.mqd_addr = ring->mqd_gpu_addr;
	queue_input.wptr_addr = ring->wptr_gpu_addr;

	r = adapt->mes.funcs->map_legacy_queue(&adapt->mes, &queue_input);

	if (r)
		AMDGV_ERROR("failed to map legacy queue\n");

	return r;
}

int amdgv_mes_unmap_legacy_queue(struct amdgv_adapter *adapt,
	  struct amdgv_ring *ring,
	  enum amdgv_unmap_queues_action action,
	  uint64_t gpu_addr, uint64_t seq, uint32_t xcc_id)
{
	struct mes_unmap_legacy_queue_input queue_input;
	int r;

	oss_memset(&queue_input, 0, sizeof(queue_input));

	queue_input.xcc_id = xcc_id;
	queue_input.action = action;
	queue_input.queue_type = ring->funcs->type;
	queue_input.doorbell_offset = ring->doorbell_index;
	queue_input.pipe_id = ring->pipe;
	queue_input.queue_id = ring->queue;
	queue_input.trail_fence_addr = gpu_addr;
	queue_input.trail_fence_data = seq;

	r = adapt->mes.funcs->unmap_legacy_queue(&adapt->mes, &queue_input);

	if (r)
		AMDGV_ERROR("failed to unmap legacy queue\n");

	return r;
}

int amdgv_mes_reset_legacy_queue(struct amdgv_adapter *adapt,
	  struct amdgv_ring *ring,
	  uint32_t vmid,
	  bool use_mmio_doorbell,
	  uint32_t xcc_id)
{
	struct mes_reset_queue_input queue_input;
	int r;

	oss_memset(&queue_input, 0, sizeof(queue_input));

	queue_input.xcc_id = xcc_id;
	queue_input.queue_type = ring->funcs->type;
	queue_input.doorbell_offset = ring->doorbell_index;
	queue_input.me_id = ring->me;
	queue_input.pipe_id = ring->pipe;
	queue_input.queue_id = ring->queue;
	queue_input.mqd_addr = ring->mqd_gpu_addr;
	queue_input.wptr_addr = ring->wptr_gpu_addr;
	queue_input.vmid = vmid;
	queue_input.use_mmio = use_mmio_doorbell;
	queue_input.is_kq = true;
	if (ring->funcs->type == AMDGV_RING_TYPE_GFX)
		queue_input.legacy_gfx = true;

	r = adapt->mes.funcs->reset_hw_queue(&adapt->mes, &queue_input);

	if (r)
		AMDGV_ERROR("failed to reset legacy queue\n");

	return r;
}

static int amdgv_mes_doorbell_init(struct amdgv_adapter *adapt)
{

	return 0;
}

static void amdgv_mes_doorbell_fini(struct amdgv_adapter *adapt)
{
}

int amdgv_mes_init(struct amdgv_adapter *adapt)
{
	int i, r, num_pipes;
	uint32_t queue_mask, reserved_queue_mask;

	adapt->mes.adapt = adapt;

	queue_mask = (uint32_t)(1UL << adapt->gfx.mec.num_queue_per_pipe) - 1;
	reserved_queue_mask = (uint32_t)(1UL << AMDGV_MES_RESERVED_QUEUES) - 1;

	adapt->mes.total_max_queue = AMDGV_MAX_MES_INST_PIPES;

	num_pipes = adapt->gfx.mec.num_pipe_per_mec * adapt->gfx.mec.num_mec;

	if (num_pipes > AMDGV_MES_MAX_COMPUTE_PIPES)
		AMDGV_WARN("More compute pipes that supported by MES (%d > %d)", num_pipes, AMDGV_MES_MAX_COMPUTE_PIPES);

	for (i = 0; i < AMDGV_MES_MAX_COMPUTE_PIPES; i++) {
		if (i >= num_pipes)
			break;
		adapt->mes.compute_hqd_mask[i] = queue_mask & ~reserved_queue_mask;
	}

	num_pipes = adapt->sdma.num_instances;
	if (num_pipes > AMDGV_MES_MAX_SDMA_PIPES)
		AMDGV_WARN("More SDMA pipes that supported by MES (%d > %d)", num_pipes, AMDGV_MES_MAX_SDMA_PIPES);
	for (i = 0; i < AMDGV_MES_MAX_SDMA_PIPES; i++) {
		if (i >= num_pipes)
			break;
		adapt->mes.sdma_hqd_mask[i] = 0xfc;
	}

	for (i = 0; i < AMDGV_MAX_MES_INST_PIPES; i++) {
		r = amdgv_wb_memory_get(adapt, &adapt->mes.sch_ctx_offs[i]);
		if (r) {
			AMDGV_WARN ("failed to get sch ctx offs\n");
			goto error;
		}

		adapt->mes.sch_ctx_gpu_addr[i] =
			adapt->wb.gpu_addr + (adapt->mes.sch_ctx_offs[i] * 4);
		adapt->mes.sch_ctx_ptr[i] =
			(uint64_t *)&adapt->wb.wb[adapt->mes.sch_ctx_offs[i]];

		r = amdgv_wb_memory_get(adapt,
				 &adapt->mes.query_status_fence_offs[i]);
		if (r) {
			AMDGV_WARN("query_status_fence_offs wb alloc failed\n");
			goto error;
		}
		adapt->mes.query_status_fence_gpu_addr[i] = adapt->wb.gpu_addr +
			(adapt->mes.query_status_fence_offs[i] * 4);
		adapt->mes.query_status_fence_ptr[i] =
			(uint64_t *)&adapt->wb.wb[adapt->mes.query_status_fence_offs[i]];
	}

	r = amdgv_mes_doorbell_init(adapt);
	if (r) {
		AMDGV_WARN("failed to init mes doorbell\n");
		goto error;
	}

	return 0;
error:
	for (i = 0; i < AMDGV_MAX_MES_INST_PIPES; i++) {
		if (adapt->mes.sch_ctx_ptr[i]) {
			amdgv_wb_memory_free(adapt, adapt->mes.sch_ctx_offs[i]);
			adapt->mes.sch_ctx_ptr[i] = NULL;
		}
		if (adapt->mes.query_status_fence_ptr[i]) {
			amdgv_wb_memory_free(adapt, adapt->mes.query_status_fence_offs[i]);
			adapt->mes.query_status_fence_ptr[i] = NULL;
		}
	}

	return r;
}

int amdgv_mes_fini(struct amdgv_adapter *adapt)
{
	int i;

	for (i = 0; i < AMDGV_MAX_MES_INST_PIPES; i++) {
		if (adapt->mes.sch_ctx_ptr[i]) {
			amdgv_wb_memory_free(adapt, adapt->mes.sch_ctx_offs[i]);
			adapt->mes.sch_ctx_ptr[i] = NULL;
		}
		if (adapt->mes.query_status_fence_ptr[i]) {
			amdgv_wb_memory_free(adapt, adapt->mes.query_status_fence_offs[i]);
			adapt->mes.query_status_fence_ptr[i] = NULL;
		}
	}

	amdgv_mes_doorbell_fini(adapt);

	return 0;
}