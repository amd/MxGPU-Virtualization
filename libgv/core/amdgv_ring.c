/*
 * Copyright 2022-2024 Advanced Micro Devices, Inc.
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

#include "amdgv_live_info.h"
#include "amdgv_ring.h"
#include "amdgv_gfx.h"
#include "amdgv_wb_memory.h"
#include "amdgv_device.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

/*
 * Rings
 * Most engines on the GPU are fed via ring buffers.  Ring
 * buffers are areas of GPU accessible memory that the host
 * writes commands into and the GPU reads commands out of.
 * There is a rptr (read pointer) that determines where the
 * GPU is currently reading, and a wptr (write pointer)
 * which determines where the host has written.  When the
 * pointers are equal, the ring is idle.  When the host
 * writes commands to the ring buffer, it increments the
 * wptr.  The GPU then starts fetching commands and executes
 * them until the pointers are equal again.
 */

inline void amdgv_ring_clear_ring(struct amdgv_ring *ring)
{
	int i = 0;

	while (i <= ring->buf_mask) {
		if (ring->aql_enable) {
			if (!(i % 16))
				ring->ring[i++] = AMDGV_AQL_INVALID_PACKET_HEADER;
			else
				ring->ring[i++] = AMDGV_AQL_INVALID_PACKET_DATA;
		} else {
			ring->ring[i++] = ring->funcs->nop;
		}
	}
}

inline void amdgv_ring_write(struct amdgv_ring *ring, uint32_t v)
{
	struct amdgv_adapter *adapt = ring->adapt;

	if (ring->count_dw <= 0)
		AMDGV_ERROR("amdgv: writing more dwords to the ring than expected!\n");
	ring->ring[ring->wptr++ & ring->buf_mask] = v;
	ring->wptr &= ring->ptr_mask;
	ring->count_dw--;
}

/**
 * amdgv_ring_alloc - allocate space on the ring buffer
 *
 * @ring: amdgv_ring structure holding ring information
 * @ndw: number of dwords to allocate in the ring buffer
 *
 * Allocate @ndw dwords in the ring buffer (all asics).
 * Returns 0 on success, error on failure.
 */
int amdgv_ring_alloc(struct amdgv_ring *ring, unsigned int ndw)
{
	/* Align requested size with padding so unlock_commit can
	 * pad safely
	 */
	if (ring->aql_enable)
		ndw = (ndw + ring->funcs->aql_align_mask) & ~ring->funcs->aql_align_mask;
	else
		ndw = (ndw + ring->funcs->align_mask) & ~ring->funcs->align_mask;

	/* Make sure we aren't trying to allocate more space
	 * than the maximum for one submission
	 */
	if (ndw > ring->max_dw)
		return AMDGV_FAILURE;

	ring->count_dw = ndw;
	ring->wptr_old = ring->wptr;

	if (ring->funcs->begin_use)
		ring->funcs->begin_use(ring);

	return 0;
}

/** amdgv_ring_insert_nop - insert NOP packets
 *
 * @ring: amdgv_ring structure holding ring information
 * @count: the number of NOP packets to insert
 *
 * This is the generic insert_nop function for rings except SDMA
 */
void amdgv_ring_insert_nop(struct amdgv_ring *ring, uint32_t count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (ring->aql_enable)
			amdgv_ring_write(ring, ring->funcs->aql_nop);
		else
			amdgv_ring_write(ring, ring->funcs->nop);
	}
}

/**
 * amdgv_ring_commit - tell the GPU to execute the new
 * commands on the ring buffer
 *
 * @ring: amdgv_ring structure holding ring information
 *
 * Update the wptr (write pointer) to tell the GPU to
 * execute new commands on the ring buffer (all asics).
 */
void amdgv_ring_commit(struct amdgv_ring *ring)
{
	uint32_t count;
	uint32_t align_mask;

	if (ring->aql_enable)
		align_mask = ring->funcs->aql_align_mask;
	else
		align_mask = ring->funcs->align_mask;

	/* We pad to match fetch size */
	count = align_mask + 1 - (ring->wptr & align_mask);
	count %= align_mask + 1;
	ring->funcs->insert_nop(ring, count);

	amdgv_ring_set_wptr(ring);

	if (ring->funcs->end_use)
		ring->funcs->end_use(ring);
}

/**
 * amdgv_ring_undo - reset the wptr
 *
 * @ring: amdgv_ring structure holding ring information
 *
 * Reset the driver's copy of the wptr (all asics).
 */
void amdgv_ring_undo(struct amdgv_ring *ring)
{
	ring->wptr = ring->wptr_old;

	if (ring->funcs->end_use)
		ring->funcs->end_use(ring);
}

#define amdgv_ring_get_gpu_addr(ring, offset) (ring->adapt->wb.gpu_addr + offset * 4)

#define amdgv_ring_get_cpu_addr(ring, offset) (&ring->adapt->wb.wb[offset])

/**
 * amdgv_fence_write - write a fence value
 *
 * @ring: ring the fence is associated with
 * @seq: sequence number to write
 *
 * Writes a fence value to memory (all asics).
 */
static void amdgv_fence_write(struct amdgv_ring *ring, uint32_t seq)
{
	struct amdgv_fence_driver *drv = &ring->fence_drv;

	if (drv->cpu_addr)
		*drv->cpu_addr = cpu_to_le32(seq);
}

/**
 * amdgv_fence_driver_start_ring - make the fence driver
 * ready for use on the requested ring.
 *
 * @ring: ring to start the fence driver on
 * @irq_src: interrupt source to use for this ring
 * @irq_type: interrupt type to use for this ring
 *
 * Make the fence driver ready for processing (all asics).
 * Not all asics have all rings, so each asic will only
 * start the fence driver on the rings it has.
 * Returns 0 for success, errors for failure.
 */
static int amdgv_fence_driver_start_ring(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;

	ring->fence_drv.cpu_addr = ring->fence_cpu_addr;
	ring->fence_drv.gpu_addr = ring->fence_gpu_addr;

	amdgv_fence_write(ring, oss_atomic_read(&ring->fence_drv.last_seq));

	ring->fence_drv.initialized = true;

	AMDGV_DEBUG("fence driver on ring %s use gpu addr 0x%016llx\n", ring->name,
		    ring->fence_drv.gpu_addr);
	return 0;
}

/**
 * amdgv_fence_driver_init_ring - init the fence driver
 * for the requested ring.
 *
 * @ring: ring to init the fence driver on
 *
 * Init the fence driver for the requested ring (all asics).
 * Helper function for adapt_fence_driver_init().
 */
static int amdgv_fence_driver_init_ring(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;

	if (!adapt)
		return AMDGV_FAILURE;

	ring->fence_drv.cpu_addr = NULL;
	ring->fence_drv.gpu_addr = 0;
	ring->fence_drv.sync_seq = 0;
	oss_atomic_set(&ring->fence_drv.last_seq, 0);
	ring->fence_drv.initialized = false;

	ring->fence_drv.num_fences_mask = ring->num_hw_submission * 2 - 1;
	ring->fence_drv.lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_MEDIUM_RANK);

	return 0;
}

/**
 * amdgv_ring_init - init driver ring struct.
 *
 * @adapt: amdgv_device pointer
 * @ring: amdgv_ring structure holding ring information
 * @max_dw: maximum number of dw for ring alloc
 * @hw_prio: ring priority (NORMAL/HIGH)
 * @sched_score: optional score atomic shared with other schedulers
 *
 * Initialize the driver information for the selected ring (all asics).
 * Returns 0 on success, error on failure.
 */
int amdgv_ring_init(struct amdgv_adapter *adapt, struct amdgv_ring *ring, uint32_t max_dw,
			uint32_t frames, uint32_t hw_prio, atomic_t *sched_score, uint32_t mem_id)
{
	int r;
	// assume max ring DWORD size are (0x1000000) (0x1000000 * 4 = 64MB)
	uint32_t ring_buffer_dword_size;
	uint32_t log2_ring_size = 23;
	uint32_t frame_dword_size = (!max_dw) ? 1024 : max_dw;
	uint32_t i, mask = 0x800000;
	int sched_hw_submission = (frames < 2) ? 2 : frames;
	uint32_t extra_dw = ring->funcs ? ring->funcs->extra_dw : 0;

	if (ring->adapt == NULL) {
		if (adapt->num_rings >= AMDGV_MAX_RINGS)
			return AMDGV_FAILURE;

		ring->adapt = adapt;
		ring->num_hw_submission = sched_hw_submission;  // frames
		ring->sched_score = sched_score;

		if (!ring->is_mes_queue) {
			ring->idx = adapt->num_rings++;
			adapt->rings[ring->idx] = ring;
		}

		r = amdgv_fence_driver_init_ring(ring);
		if (r)
			return r;
	}

	r = amdgv_wb_memory_get(adapt, &ring->rptr_offs);
	if (r) {
		AMDGV_ERROR("(%d) ring rptr_offs wb alloc failed\n", r);
		return r;
	}

	r = amdgv_wb_memory_get(adapt, &ring->wptr_offs);
	if (r) {
		AMDGV_ERROR("(%d) ring wptr_offs wb alloc failed\n", r);
		return r;
	}

	r = amdgv_wb_memory_get(adapt, &ring->fence_offs);
	if (r) {
		AMDGV_ERROR("(%d) ring fence_offs wb alloc failed\n", r);
		return r;
	}

	r = amdgv_wb_memory_get(adapt, &ring->trail_fence_offs);
	if (r) {
		AMDGV_ERROR("(%d) ring trail_fence_offs wb alloc failed\n", r);
		return r;
	}

	r = amdgv_wb_memory_get(adapt, &ring->cond_exe_offs);
	if (r) {
		AMDGV_ERROR("(%d) ring cond_exec_polling wb alloc failed\n", r);
		return r;
	}

	ring_buffer_dword_size = frame_dword_size * sched_hw_submission;
	if (ring_buffer_dword_size >= 0x1000000) {
		++log2_ring_size;
	} else {
		if (ring_buffer_dword_size & (ring_buffer_dword_size - 1)) {
			// size is not power of 2
			++log2_ring_size;
		}
		for (i = 0; i < 32; i++) {
			if (ring_buffer_dword_size & mask)
				break;

			mask >>= 1;
			--log2_ring_size;
		}
	}
	ring->ring_size = 1 << log2_ring_size;
	ring->log2_ring_size = log2_ring_size;

	ring->buf_mask = ring->ring_size - 1;
	ring->ptr_mask = ring->buf_mask;
	if (ring->funcs && ring->funcs->support_64bit_ptrs) {
		ring->ptr_mask = 0xffffffffffffffff;
	}

	/* Allocate ring buffer */
	if (ring->ring_obj == NULL) {
		ring->ring_obj =
			amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
						 (ring->ring_size + extra_dw) * sizeof(uint32_t),
						 PAGE_SIZE, mem_id);
		if (!ring->ring_obj) {
			AMDGV_ERROR("ring create failed\n");
			return AMDGV_FAILURE;
		}
	}

	ring->max_dw = frame_dword_size;  // frame dword size
	ring->hw_prio = hw_prio;
	ring->wptr = 0;
	ring->wptr_old = 0;

	return 0;
}

int amdgv_ring_init_set(struct amdgv_adapter *adapt, struct amdgv_ring *ring)
{
	int r;

	ring->fence_gpu_addr = amdgv_ring_get_gpu_addr(ring, ring->fence_offs);
	ring->fence_cpu_addr = amdgv_ring_get_cpu_addr(ring, ring->fence_offs);

	ring->rptr_gpu_addr = amdgv_ring_get_gpu_addr(ring, ring->rptr_offs);
	ring->rptr_cpu_addr = amdgv_ring_get_cpu_addr(ring, ring->rptr_offs);

	ring->wptr_gpu_addr = amdgv_ring_get_gpu_addr(ring, ring->wptr_offs);
	ring->wptr_cpu_addr = amdgv_ring_get_cpu_addr(ring, ring->wptr_offs);

	ring->trail_fence_gpu_addr = amdgv_ring_get_gpu_addr(ring, ring->trail_fence_offs);
	ring->trail_fence_cpu_addr = amdgv_ring_get_cpu_addr(ring, ring->trail_fence_offs);

	ring->cond_exe_gpu_addr = amdgv_ring_get_gpu_addr(ring, ring->cond_exe_offs);
	ring->cond_exe_cpu_addr = amdgv_ring_get_cpu_addr(ring, ring->cond_exe_offs);

	/* always set cond_exec_polling to CONTINUE */
	*ring->cond_exe_cpu_addr = 1;

	r = amdgv_fence_driver_start_ring(ring);
	if (r) {
		AMDGV_ERROR("failed initializing fences (%d).\n", r);
		return r;
	}

	// Retrieve the GPU and CPU addresses of the allocated memory
	ring->gpu_addr = amdgv_memmgr_get_gpu_addr(ring->ring_obj);
	ring->ring = amdgv_memmgr_get_cpu_addr(ring->ring_obj);

	// Clear the ring
	amdgv_ring_clear_ring(ring);

	return 0;
}

/**
 * amdgv_ring_fini - tear down the driver ring struct.
 *
 * @ring: amdgv_ring structure holding ring information
 *
 * Tear down the driver information for the selected ring (all asics).
 */
void amdgv_ring_fini(struct amdgv_ring *ring)
{
	/* Not to finish a ring which is not initialized */
	if (!(ring->adapt) || (!ring->is_mes_queue && !(ring->adapt->rings[ring->idx])))
		return;

	if (!ring->is_mes_queue) {
		amdgv_wb_memory_free(ring->adapt, ring->rptr_offs);
		amdgv_wb_memory_free(ring->adapt, ring->wptr_offs);

		amdgv_wb_memory_free(ring->adapt, ring->cond_exe_offs);
		amdgv_wb_memory_free(ring->adapt, ring->fence_offs);

		amdgv_memmgr_free(ring->ring_obj);
	}

	ring->me = 0;

	if (!ring->is_mes_queue)
		ring->adapt->rings[ring->idx] = NULL;

	if (ring->fence_drv.lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(ring->fence_drv.lock);
		ring->fence_drv.lock = OSS_INVALID_HANDLE;
	}
}

/**
 * amdgv_ring_test_helper - tests ring and set sched readiness status
 *
 * @ring: ring to try the recovery on
 *
 * Tests ring and set sched readiness status
 *
 * Returns 0 on success, error on failure.
 */
int amdgv_ring_test_helper(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	int r;

	r = amdgv_ring_test_ring(ring);
	if (r)
		AMDGV_ERROR("ring %s test failed (%d)\n", ring->name, r);
	else
		AMDGV_INFO("ring test on %s succeeded\n", ring->name);

	return r;
}

/**
 * amdgv_fence_read - read a fence value
 *
 * @ring: ring the fence is associated with
 *
 * Reads a fence value from memory (all asics).
 * Returns the value of the fence read from memory.
 */
static uint32_t amdgv_fence_read(struct amdgv_ring *ring)
{
	struct amdgv_fence_driver *drv = &ring->fence_drv;
	uint32_t seq = 0;

	if (drv->cpu_addr)
		seq = le32_to_cpu(*drv->cpu_addr);
	else
		seq = oss_atomic_read(&drv->last_seq);

	return seq;
}

/**
 * amdgv_fence_wait_polling - busy wait for givn sequence number
 *
 * @ring: ring index the fence is associated with
 * @wait_seq: sequence number to wait
 * @timeout: the timeout for waiting in usecs
 *
 * Wait for all fences on the requested ring to signal (all asics).
 * Returns left time if no timeout, 0 or minus if timeout.
 */
signed long amdgv_fence_wait_polling(struct amdgv_ring *ring, uint32_t wait_seq,
				      signed long timeout)
{
	uint32_t seq;

	do {
		seq = amdgv_fence_read(ring);
		oss_udelay(5);
		timeout -= 5;
	} while ((int32_t)(wait_seq - seq) > 0 && timeout > 0);

	return timeout > 0 ? timeout : 0;
}

/**
 * amdgv_fence_emit_polling - emit a fence on the requeste ring
 *
 * @ring: ring the fence is associated with
 * @s: resulting sequence number
 * @timeout: the timeout for waiting in usecs
 *
 * Emits a fence command on the requested ring (all asics).
 * Used For polling fence.
 * Returns 0 on success, -ENOMEM on failure.
 */
int amdgv_fence_emit_polling(struct amdgv_ring *ring, uint32_t *s, uint32_t timeout)
{
	uint32_t seq;
	signed long r;

	if (!s)
		return AMDGV_FAILURE;

	seq = ++ring->fence_drv.sync_seq;
	r = amdgv_fence_wait_polling(ring, seq - ring->fence_drv.num_fences_mask, timeout);
	if (r < 1)
		return AMDGV_FAILURE;

	amdgv_ring_emit_fence(ring, ring->fence_drv.gpu_addr, seq, 0);

	*s = seq;

	return 0;
}

enum amdgv_live_info_status amdgv_ring_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ring *ring_info)
{
	uint32_t i;
	struct amdgv_ring *ring ;

	if (!adapt->irqmgr.disable_parse_ih) {
		for (i = 0; i < LIVE_INFO_MAX_GC_INSTANCES; i++)
			ring_info->irqmgr[i].rb_rptr = adapt->irqmgr.ih.rptr;
	}

	for (i = 0; i < LIVE_INFO_MAX_SDMA_RINGS; i++) {
		ring = &adapt->sdma.sdma_ring[i];
		if (ring->ring_obj)
			ring_info->sdma[i].rb_wptr = ring->wptr;
	}
	ring = &adapt->gfx.kiq[0].ring;
	if (ring->ring_obj) {
		for (i = 0; i < LIVE_INFO_MAX_GC_INSTANCES; i++)
			ring_info->kiq[i].rb_wptr = ring->wptr;
	}

	for (i = 0; i < (LIVE_INFO_MAX_COMPUTE_RINGS); i++) {
		ring = &adapt->gfx.compute_ring[i];
		if (ring->ring_obj)
			ring_info->compute[i].rb_wptr = ring->wptr;
	}
	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_ring_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ring *ring_info)
{
	uint32_t i;
	struct amdgv_ring *ring ;

	if (!adapt->irqmgr.disable_parse_ih) {
		for (i = 0; i < LIVE_INFO_MAX_GC_INSTANCES; i++)
			adapt->irqmgr.ih.rptr = ring_info->irqmgr[i].rb_rptr;
	}

	for (i = 0; i < LIVE_INFO_MAX_SDMA_RINGS; i++) {
		ring = &adapt->sdma.sdma_ring[i];
		if (ring->ring_obj)
			ring->wptr = ring_info->sdma[i].rb_wptr;
	}
	ring = &adapt->gfx.kiq[0].ring;
	if (ring->ring_obj) {
		for (i = 0; i < LIVE_INFO_MAX_GC_INSTANCES; i++)
			ring->wptr = ring_info->kiq[i].rb_wptr;
	}

	for (i = 0; i < (LIVE_INFO_MAX_COMPUTE_RINGS); i++) {
		ring = &adapt->gfx.compute_ring[i];
		if (ring->ring_obj)
			ring->wptr = ring_info->compute[i].rb_wptr;
	}
	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}
