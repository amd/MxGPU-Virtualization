/*
 * Copyright (c) 2018-2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_device.h"
#include "amdgv_ring.h"
#include "amdgv_gfx.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

#define AMDGV_IB_TEST_TIMEOUT	       msecs_to_jiffies(1000)
#define AMDGV_IB_TEST_GFX_XGMI_TIMEOUT msecs_to_jiffies(2000)

/*
 * IB
 * IBs (Indirect Buffers) and areas of GPU accessible memory where
 * commands are stored.  You can put a pointer to the IB in the
 * command ring and the hw will fetch the commands from the IB
 * and execute them.  Generally userspace acceleration drivers
 * produce command buffers which are send to the kernel and
 * put in IBs for execution by the requested ring.
 */

/**
 * amdgv_ib_get - request an IB (Indirect Buffer)
 *
 * @adapt: amdgv_device pointer
 * @vm: amdgv_vm pointer
 * @size: requested IB size
 * @pool_type: IB pool type (delayed, immediate, direct)
 * @ib: IB object returned
 *
 * Request an IB (all asics).  IBs are allocated using the
 * suballocator.
 * Returns 0 on success, error on failure.
 */
int amdgv_ib_get(struct amdgv_adapter *adapt, unsigned int size,
		 enum amdgv_ib_pool_type pool_type, struct amdgv_ib *ib)
{
	if (size) {
		ib->sa_bo = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, size, 256, MEM_GFX_IB);
		if (!ib->sa_bo) {
			AMDGV_ERROR("failed to get a new IB\n");
			return AMDGV_FAILURE;
		}

		ib->ptr = amdgv_memmgr_get_cpu_addr(ib->sa_bo);
		/* flush the cache before commit the IB */
		ib->flags = AMDGV_IB_FLAG_EMIT_MEM_SYNC;
		ib->gpu_addr = amdgv_memmgr_get_gpu_addr(ib->sa_bo);
	}

	return 0;
}

/**
 * amdgv_ib_free - free an IB (Indirect Buffer)
 *
 * @adapt: amdgv_device pointer
 * @ib: IB object to free
 *
 * Free an IB (all asics).
 */
void amdgv_ib_free(struct amdgv_adapter *adapt, struct amdgv_ib *ib)
{
	amdgv_memmgr_free(ib->sa_bo);
}

/**
 * amdgv_ib_schedule - schedule an IB (Indirect Buffer) on the ring
 *
 * @ring: ring index the IB is associated with
 * @num_ibs: number of IBs to schedule
 * @ibs: IB objects to schedule
 * @job: job to schedule
 * @f: fence created during this submission
 *
 * Schedule an IB on the associated ring (all asics).
 * Returns 0 on success, error on failure.
 *
 * On SI, there are two parallel engines fed from the primary ring,
 * the CE (Constant Engine) and the DE (Drawing Engine).  Since
 * resource descriptors have moved to memory, the CE allows you to
 * prime the caches while the DE is updating register state so that
 * the resource descriptors will be already in cache when the draw is
 * processed.  To accomplish this, the userspace driver submits two
 * IBs, one for the CE and one for the DE.  If there is a CE IB (called
 * a CONST_IB), it will be put on the ring prior to the DE IB.  Prior
 * to SI there was just a DE IB.
 */
int amdgv_ib_schedule(struct amdgv_ring *ring, unsigned int num_ibs, struct amdgv_ib *ibs)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct amdgv_ib *ib = &ibs[0];
	bool need_ctx_switch;
	uint64_t fence_ctx;
	uint32_t status = 0, alloc_size, seq;
	unsigned int fence_flags = 0;
	unsigned int i, cnt = 0;
	int r = 0;

	if (num_ibs == 0)
		return AMDGV_FAILURE;

	/* ring tests don't use a job */
	fence_ctx = 0;

	alloc_size = ring->funcs->emit_frame_size + num_ibs * ring->funcs->emit_ib_size;

	r = amdgv_ring_alloc(ring, alloc_size);
	if (r) {
		AMDGV_ERROR("scheduling IB failed (%d).\n", r);
		return r;
	}

	need_ctx_switch = ring->current_ctx != fence_ctx;

	if (ring->funcs->emit_wave_limit && ring->hw_prio == AMDGV_GFX_PIPE_PRIO_HIGH)
		ring->funcs->emit_wave_limit(ring, true);

	if (ring->funcs->insert_start)
		ring->funcs->insert_start(ring);

	amdgv_ring_emit_hdp_flush(ring);

	if (need_ctx_switch)
		status |= AMDGV_HAVE_CTX_SWITCH;

	for (i = 0; i < num_ibs; ++i) {
		ib = &ibs[i];
		if (ib->sa_bo == NULL) {
			AMDGV_ERROR("IB %u not initialized.\n", i);
			amdgv_ring_undo(ring);
			return AMDGV_FAILURE;
		}
		amdgv_ring_emit_ib(ring, ib, status);
		status &= ~AMDGV_HAVE_CTX_SWITCH;
	}

	if (ib->flags & AMDGV_IB_FLAG_TC_WB_NOT_INVALIDATE)
		fence_flags |= AMDGV_FENCE_FLAG_TC_WB_ONLY;

	r = amdgv_fence_emit_polling(ring, &seq, MAX_KIQ_REG_WAIT);
	if (r) {
		AMDGV_ERROR("failed to emit fence (%d)\n", r);
		amdgv_ring_undo(ring);
		return r;
	}

	if (ring->funcs->insert_end)
		ring->funcs->insert_end(ring);

	ring->current_ctx = fence_ctx;

	if (ring->funcs->emit_wave_limit && ring->hw_prio == AMDGV_GFX_PIPE_PRIO_HIGH)
		ring->funcs->emit_wave_limit(ring, false);

	amdgv_ring_commit(ring);

	while (r < 1 && cnt++ < MAX_KIQ_REG_TRY) {
		oss_usleep(AMDGV_GFX_MAX_USEC_TIMEOUT);
		r = amdgv_fence_wait_polling(ring, seq, MAX_KIQ_REG_WAIT);
	}

	if (cnt > MAX_KIQ_REG_TRY)
		return AMDGV_FAILURE;

	return 0;
}
