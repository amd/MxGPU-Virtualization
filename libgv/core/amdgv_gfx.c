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
#include "amdgv_gfx.h"
#include "amdgv_ras.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static const uint32_t this_block = AMDGV_GFX_BLOCK;

/*
 * GPU GFX IP block helpers function, bitmap is no more than 64 bits.
 */
bool amdgv_gfx_test_bit(int nr, uint64_t bitmap)
{
	return bitmap & (0x1ULL << nr);
}

void amdgv_gfx_set_bit(int nr, uint64_t *bitmap)
{
	*bitmap |= (0x1ULL << nr);
}

void amdgv_gfx_clear_bit(int nr, uint64_t *bitmap)
{
	*bitmap &= ~(0x1ULL << nr);
}

unsigned long amdgv_gfx_find_first_zero_bit(uint64_t bitmap)
{
	int i;

	for (i = 0; i < 64; i++) {
		if (!(bitmap & (0x1ULL << i)))
			return i;
	}

	return 0xffffffff;
}

int amdgv_gfx_mec_queue_to_bit(struct amdgv_adapter *adapt, int mec, int pipe, int queue)
{
	int bit = 0;

	bit += mec * adapt->gfx.mec.num_pipe_per_mec * adapt->gfx.mec.num_queue_per_pipe;
	bit += pipe * adapt->gfx.mec.num_queue_per_pipe;
	bit += queue;

	return bit;
}

void amdgv_queue_mask_bit_to_mec_queue(struct amdgv_adapter *adapt, int bit, int *mec,
				       int *pipe, int *queue)
{
	*queue = bit % adapt->gfx.mec.num_queue_per_pipe;
	*pipe = (bit / adapt->gfx.mec.num_queue_per_pipe) % adapt->gfx.mec.num_pipe_per_mec;
	*mec = (bit / adapt->gfx.mec.num_queue_per_pipe) / adapt->gfx.mec.num_pipe_per_mec;
}

bool amdgv_gfx_is_mec_queue_enabled(struct amdgv_adapter *adapt, int xcc_id, int mec,
									int pipe, int queue)
{
	return amdgv_gfx_test_bit(amdgv_gfx_mec_queue_to_bit(adapt, mec, pipe, queue),
			adapt->gfx.mec_queue_bitmap[xcc_id]);
}

bool amdgv_gfx_is_high_priority_compute_queue(struct amdgv_adapter *adapt,
					       struct amdgv_ring *ring)
{
	/* Policy: use 1st queue as high priority compute queue if we
	 * have more than one compute queue.
	 */
	if (adapt->gfx.num_compute_rings > 1 && ring == &adapt->gfx.compute_ring[0])
		return true;

	return false;
}

void amdgv_gfx_compute_queue_acquire(struct amdgv_adapter *adapt)
{
	int i, j, queue, pipe, num_xcc;
	int max_queues_per_mec =
		min(adapt->gfx.mec.num_pipe_per_mec * adapt->gfx.mec.num_queue_per_pipe,
				     adapt->gfx.num_compute_rings);
	num_xcc = adapt->mcp.gfx.num_xcc ? adapt->mcp.gfx.num_xcc : 1;

	/* policy: make compute queues evenly cross all pipes on MEC1 only */
	for (j = 0; j < num_xcc; j++) {
		for (i = 0; i < max_queues_per_mec; i++) {
			pipe = i % adapt->gfx.mec.num_pipe_per_mec;
			queue = (i / adapt->gfx.mec.num_pipe_per_mec) %
				adapt->gfx.mec.num_queue_per_pipe;

			amdgv_gfx_set_bit(
				pipe * adapt->gfx.mec.num_queue_per_pipe +
				queue,
				&(adapt->gfx.mec_queue_bitmap[j]));
		}
	}
}

int amdgv_gfx_kiq_init_ring(struct amdgv_adapter *adapt, struct amdgv_ring *ring, int xcc_id)
{
	struct amdgv_kiq *kiq = &adapt->gfx.kiq[xcc_id];
	int r = 0;
	uint32_t frame_dword_size = 1024;
	uint32_t frame_number = 256;

	kiq->ring_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_MEDIUM_RANK);

	ring->adapt = NULL;
	ring->ring_obj = NULL;
	ring->use_doorbell = true;
	ring->xcc_id = xcc_id;
	ring->doorbell_index = (adapt->doorbell_index.kiq +
		xcc_id * adapt->doorbell_index.xcc_doorbell_range) << 1;

	ring->no_scheduler = true;
	oss_vsnprintf(ring->name, 14, "kiq[%d].%d.%d.%d", xcc_id, ring->me,
			ring->pipe, ring->queue);
	/* Set the hw submission limit higher for KIQ because
	 * it's used for a number of gfx/compute tasks by both
	 * KFD and KGD which may have outstanding fences and
	 * it doesn't really use the gpu scheduler anyway;
	 * KIQ tasks get submitted directly to the ring.
	 */
	r = amdgv_ring_init(adapt, ring, frame_dword_size, frame_number, AMDGV_RING_PRIO_DEFAULT, NULL, MEM_KIQ_RING);
	if (r)
		AMDGV_WARN("(%d) failed to init kiq ring\n", r);

	return r;
}

void amdgv_gfx_kiq_free_ring(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct amdgv_kiq *kiq = NULL;

	if (adapt)
		kiq = &adapt->gfx.kiq[ring->xcc_id];

	amdgv_ring_fini(ring);
	if (kiq && kiq->ring_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(kiq->ring_lock);
		kiq->ring_lock = OSS_INVALID_HANDLE;
	}
}

void amdgv_gfx_kiq_fini(struct amdgv_adapter *adapt, int xcc_id)
{
	struct amdgv_kiq *kiq = &adapt->gfx.kiq[xcc_id];

	if (kiq && kiq->eop_obj)
		amdgv_memmgr_free(kiq->eop_obj);
}

int amdgv_gfx_kiq_init(struct amdgv_adapter *adapt, unsigned int hpd_size, int xcc_id)
{
	struct amdgv_kiq *kiq = &adapt->gfx.kiq[xcc_id];

	kiq->eop_obj =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf, hpd_size, PAGE_SIZE, MEM_KIQ_EOP);
	if (!kiq->eop_obj) {
		AMDGV_WARN("failed to create KIQ bo.\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_gfx_kiq_init_set(struct amdgv_adapter *adapt, unsigned int hpd_size,
				int xcc_id)
{
	uint32_t *hpd;
	struct amdgv_kiq *kiq = &adapt->gfx.kiq[xcc_id];

	kiq->eop_gpu_addr = amdgv_memmgr_get_gpu_addr(kiq->eop_obj);
	hpd = amdgv_memmgr_get_cpu_addr(kiq->eop_obj);
	oss_memset(hpd, 0, hpd_size);

	kiq->ring.eop_gpu_addr = kiq->eop_gpu_addr;

	return 0;
}

/* create MQD for each compute/gfx queue */
int amdgv_gfx_mqd_sw_init(struct amdgv_adapter *adapt,
				unsigned int mqd_size, int xcc_id)
{
	struct amdgv_kiq *kiq = NULL;
	struct amdgv_ring *ring = NULL;
	int i, j;

	/* create MQD for KIQ */
	kiq = &adapt->gfx.kiq[xcc_id];
	ring = &adapt->gfx.kiq[xcc_id].ring;
	if (!ring->mqd_obj) {
		ring->mqd_obj = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, mqd_size,
							 PAGE_SIZE, MEM_KIQ_MQD);
		if (!ring->mqd_obj) {
			AMDGV_WARN("failed to create ring mqd ob");
			return AMDGV_FAILURE;
		}

		/* prepare MQD backup */
		kiq->mqd_backup = oss_zalloc(mqd_size);
		if (!kiq->mqd_backup)
			AMDGV_WARN("no memory to create MQD backup for ring %s\n", ring->name);
	}

	/* create MQD for each KCQ */
	for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
		j = i + xcc_id * adapt->gfx.num_compute_rings;
		ring = &adapt->gfx.compute_ring[j];
		if (!ring->mqd_obj) {
			ring->mqd_obj = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, mqd_size,
								 PAGE_SIZE, MEM_COMPUTE0_MQD + i);
			if (!ring->mqd_obj) {
				AMDGV_WARN("failed to create ring mqd bo");
				return AMDGV_FAILURE;
			}

			/* prepare MQD backup */
			adapt->gfx.mec.mqd_backup[j] = oss_zalloc(mqd_size);
			if (!adapt->gfx.mec.mqd_backup[j])
				AMDGV_WARN("no memory to create MQD backup for ring %s\n",
					   ring->name);
		}
	}

	return 0;
}

int amdgv_gfx_mqd_init_set(struct amdgv_adapter *adapt, int xcc_id)
{
	struct amdgv_ring *ring = NULL;
	int i, j;

	/* get GPU and CPU addresses for KIQ */
	ring = &adapt->gfx.kiq[xcc_id].ring;
	ring->mqd_gpu_addr = amdgv_memmgr_get_gpu_addr(ring->mqd_obj);
	ring->mqd_ptr = amdgv_memmgr_get_cpu_addr(ring->mqd_obj);

	/* get GPU and CPU addresses for each KCQ */
	for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
		j = i + xcc_id * adapt->gfx.num_compute_rings;
		ring = &adapt->gfx.compute_ring[j];
		ring->mqd_gpu_addr = amdgv_memmgr_get_gpu_addr(ring->mqd_obj);
		ring->mqd_ptr = amdgv_memmgr_get_cpu_addr(ring->mqd_obj);
	}

	return 0;
}

void amdgv_gfx_mqd_sw_fini(struct amdgv_adapter *adapt, int xcc_id)
{
	struct amdgv_ring *ring = NULL;
	int i, j;

	for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
		j = i + xcc_id * adapt->gfx.num_compute_rings;
		ring = &adapt->gfx.compute_ring[j];
		oss_free(adapt->gfx.mec.mqd_backup[j]);
		amdgv_memmgr_free(ring->mqd_obj);
		adapt->gfx.mec.mqd_backup[j] = NULL;
		ring->mqd_obj = NULL;
	}

	ring = &adapt->gfx.kiq[xcc_id].ring;
	oss_free(adapt->gfx.kiq[xcc_id].mqd_backup);
	amdgv_memmgr_free(ring->mqd_obj);
	adapt->gfx.kiq[xcc_id].mqd_backup = NULL;
	ring->mqd_obj = NULL;
}

int amdgv_queue_mask_bit_to_set_resource_bit(struct amdgv_adapter *adapt, int queue_bit)
{
	int mec, pipe, queue;
	int set_resource_bit = 0;

	amdgv_queue_mask_bit_to_mec_queue(adapt, queue_bit, &mec, &pipe, &queue);

	set_resource_bit = mec * 4 * 8 + pipe * 8 + queue;

	return set_resource_bit;
}

int amdgv_gfx_kiq_set_resources(struct amdgv_adapter *adapt, int xcc_id)
{
	struct amdgv_kiq *kiq = &adapt->gfx.kiq[xcc_id];
	struct amdgv_ring *kiq_ring = &adapt->gfx.kiq[xcc_id].ring;
	uint64_t queue_mask = 0;
	int r, i;

	if (!kiq->pmf || !kiq->pmf->kiq_map_queues || !kiq->pmf->kiq_set_resources)
		return AMDGV_FAILURE;

	for (i = 0; i < AMDGV_MAX_COMPUTE_QUEUES; ++i) {
		if (!amdgv_gfx_test_bit(i, adapt->gfx.mec_queue_bitmap[xcc_id]))
			continue;

		/* This situation may be hit in the future if a new HW
		 * generation exposes more than 64 queues. If so, the
		 * definition of queue_mask needs updating
		 */
		if (i > (sizeof(queue_mask) * 8)) {
			AMDGV_ERROR("Invalid KCQ enabled: %d\n", i);
			break;
		}

		queue_mask |= (1ull << amdgv_queue_mask_bit_to_set_resource_bit(adapt, i));
	}

	AMDGV_INFO("kiq ring mec %d pipe %d q %d\n", kiq_ring->me, kiq_ring->pipe,
		   kiq_ring->queue);

	oss_spin_lock(adapt->gfx.kiq[xcc_id].ring_lock);
	r = amdgv_ring_alloc(kiq_ring, kiq->pmf->set_resources_size);
	if (r) {
		AMDGV_ERROR("Failed to lock KIQ (%d).\n", r);
		oss_spin_unlock(adapt->gfx.kiq[xcc_id].ring_lock);
		return r;
	}

	kiq->pmf->kiq_set_resources(kiq_ring, queue_mask);
	amdgv_ring_commit(kiq_ring);

	oss_spin_unlock(adapt->gfx.kiq[xcc_id].ring_lock);
	return 0;
}

int amdgv_gfx_map_kcq(struct amdgv_adapter *adapt, int xcc_id,
						enum amdgv_gfx_xcc_queue_index index)
{
	struct amdgv_kiq *kiq = &adapt->gfx.kiq[xcc_id];
	struct amdgv_ring *kiq_ring = &kiq->ring;
	uint32_t ring_index = xcc_id * adapt->gfx.num_compute_rings + index;
	struct amdgv_ring *kcq_ring = &adapt->gfx.compute_ring[ring_index];

	if (!kiq->pmf || !kiq->pmf->kiq_map_queues)
		return AMDGV_FAILURE;

	oss_spin_lock(adapt->gfx.kiq[xcc_id].ring_lock);
	if (amdgv_ring_alloc(kiq_ring, kiq->pmf->map_queues_size)) {
		oss_spin_unlock(adapt->gfx.kiq[xcc_id].ring_lock);
		return AMDGV_FAILURE;
	}
	kiq->pmf->kiq_map_queues(kiq_ring, kcq_ring);
	amdgv_ring_commit(kiq_ring);
	oss_spin_unlock(adapt->gfx.kiq[xcc_id].ring_lock);
	return 0;
}

int amdgv_gfx_unmap_kcq(struct amdgv_adapter *adapt, int xcc_id,
							enum amdgv_gfx_xcc_queue_index index)
{
	struct amdgv_kiq *kiq = &adapt->gfx.kiq[xcc_id];
	struct amdgv_ring *kiq_ring = &kiq->ring;
	uint32_t ring_index = xcc_id * adapt->gfx.num_compute_rings + index;
	struct amdgv_ring *kcq_ring = &adapt->gfx.compute_ring[ring_index];

	if (!kiq->pmf || !kiq->pmf->kiq_unmap_queues)
		return AMDGV_FAILURE;

	oss_spin_lock(adapt->gfx.kiq[xcc_id].ring_lock);
	if (amdgv_ring_alloc(kiq_ring, kiq->pmf->unmap_queues_size)) {
		oss_spin_unlock(adapt->gfx.kiq[xcc_id].ring_lock);
		return AMDGV_FAILURE;
	}
	kiq->pmf->kiq_unmap_queues(kiq_ring, kcq_ring, RESET_QUEUES, 0, 0);
	amdgv_ring_commit(kiq_ring);
	oss_spin_unlock(adapt->gfx.kiq[xcc_id].ring_lock);
	return 0;
}

void amdgv_gfx_ras_error_func(struct amdgv_adapter *adapt,
	void *ras_error_status,
	void (*func)(struct amdgv_adapter *adapt, void *ras_error_status,
	int xcc_id))
{
	int i;
	int num_xcc = adapt->mcp.gfx.xcc_mask ? AMDGV_RAS_NUM_XCC(adapt->mcp.gfx.xcc_mask) : 1;
	uint32_t xcc_mask = AMDGV_RAS_GENMASK(num_xcc - 1, 0);
	struct ras_err_data *err_data = (struct ras_err_data *)ras_error_status;

	if (err_data) {
		err_data->ue_count = 0;
		err_data->ce_count = 0;
	}

	for_each_id(i, xcc_mask)
		func(adapt, ras_error_status, i);
}


/* Return in GFLOPs */
int amdgv_gfx_get_compute_cap(struct amdgv_adapter *adapt, bool min, uint32_t *compute_cap)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk_mhz = 0;
	uint32_t threads_per_cu = 4 * adapt->config.gfx.max_waves_per_simd
								* adapt->config.gfx.wave_size;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		if (min) {
			adapt->pp.pp_funcs->get_clock_limit(adapt,
				PP_CLOCK_TYPE__GFX, PP_CLOCK_LIMIT_TYPE__SOFT_MIN, &clk_mhz);
			ret = 0;
		} else {
			adapt->pp.pp_funcs->get_clock_limit(adapt,
				PP_CLOCK_TYPE__GFX, PP_CLOCK_LIMIT_TYPE__SOFT_MAX, &clk_mhz);
			ret = 0;
		}
	}
	*compute_cap = (adapt->config.gfx.active_cu_count * clk_mhz * threads_per_cu) / 1000;
	AMDGV_INFO("COMPUTE CAP: %08x\n", *compute_cap);

	return ret;
}

void amdgv_gfx_rlc_enter_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	if (adapt->gfx.rlc.in_safe_mode[xcc_id])
		return;

	/* if RLC is not enabled, do nothing */
	if (!adapt->gfx.rlc.funcs->is_rlc_enabled(adapt))
		return;

	adapt->gfx.rlc.funcs->set_safe_mode(adapt, xcc_id);
	adapt->gfx.rlc.in_safe_mode[xcc_id] = true;
}

void amdgv_gfx_rlc_exit_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	if (!(adapt->gfx.rlc.in_safe_mode[xcc_id]))
		return;

	/* if RLC is not enabled, do nothing */
	if (!adapt->gfx.rlc.funcs->is_rlc_enabled(adapt))
		return;

	adapt->gfx.rlc.funcs->unset_safe_mode(adapt, xcc_id);
	adapt->gfx.rlc.in_safe_mode[xcc_id] = false;
}

