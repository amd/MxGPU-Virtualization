/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_gfx.h"
#include "amdgv_ras.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static const uint32_t this_block = AMDGV_GFX_BLOCK;

static const hsa_signal_t signal = { 0 };

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
	uint32_t i, j;

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

		/* MQD backup disabled */
		kiq->mqd_backup = NULL;

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

			/* MQD backup disabled */
			adapt->gfx.mec.mqd_backup[j] = NULL;
		}
	}

	return 0;
}

int amdgv_gfx_mqd_init_set(struct amdgv_adapter *adapt, int xcc_id)
{
	struct amdgv_ring *ring = NULL;
	uint32_t i, j;

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
	uint32_t i, j;

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
	AMDGV_DEBUG("COMPUTE CAP: %08x\n", *compute_cap);

	return ret;
}

int amdgv_gfx_rlc_enter_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	int ret;

	if (!adapt->gfx.rlc.funcs ||
	    !adapt->gfx.rlc.funcs->is_rlc_enabled ||
	    !adapt->gfx.rlc.funcs->set_safe_mode)
		return 0;

	if (!adapt->gfx.rlc.funcs->is_rlc_enabled(adapt))
		return AMDGV_FAILURE;

	if (adapt->gfx.rlc.safe_mode_count[xcc_id]) {
		adapt->gfx.rlc.safe_mode_count[xcc_id]++;
		return 0;
	}

	ret = adapt->gfx.rlc.funcs->set_safe_mode(adapt, xcc_id);
	if (ret)
		return ret;

	adapt->gfx.rlc.safe_mode_count[xcc_id] = 1;
	return 0;
}

int amdgv_gfx_rlc_exit_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	int ret;

	if (!adapt->gfx.rlc.funcs ||
	    !adapt->gfx.rlc.funcs->is_rlc_enabled ||
	    !adapt->gfx.rlc.funcs->unset_safe_mode)
		return 0;

	if (!adapt->gfx.rlc.funcs->is_rlc_enabled(adapt))
		return AMDGV_FAILURE;

	if (!adapt->gfx.rlc.safe_mode_count[xcc_id])
		return 0;

	adapt->gfx.rlc.safe_mode_count[xcc_id]--;
	if (adapt->gfx.rlc.safe_mode_count[xcc_id])
		return 0;

	ret = adapt->gfx.rlc.funcs->unset_safe_mode(adapt, xcc_id);
	if (ret) {
		adapt->gfx.rlc.safe_mode_count[xcc_id] = 1;
		return ret;
	}

	return 0;
}

int amdgv_gfx_rlc_safe_mode(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t xcc_id, num_xcc;
	int ret;

	num_xcc = adapt->mcp.gfx.num_xcc ? adapt->mcp.gfx.num_xcc : 1;

	for (xcc_id = 0; xcc_id < num_xcc; xcc_id++) {
		if (enable)
			ret = amdgv_gfx_rlc_enter_safe_mode(adapt, xcc_id);
		else
			ret = amdgv_gfx_rlc_exit_safe_mode(adapt, xcc_id);

		if (ret) {
			AMDGV_WARN("failed to %s RLC safe mode on XCC%d\n",
				   enable ? "enter" : "exit", xcc_id);
			return ret;
		}
	}

	return 0;
}

static void amdgv_gfx_set_aql_comp_ring_info(struct amdgv_adapter *adapt,
		struct amdgv_ring *aql_compute_ring, struct oss_aql_comp_rb_info *info)
{
	aql_compute_ring->adapt = adapt;

	aql_compute_ring->aql_enable = true;
	aql_compute_ring->use_doorbell = true;
	aql_compute_ring->wptr = *(info->wptr_poll_memory);
	aql_compute_ring->buf_mask = info->ring_dw_size - 1;
	aql_compute_ring->ring = info->ring_base;
	aql_compute_ring->ptr_mask =
			aql_compute_ring->funcs->support_64bit_ptrs ? 0xffffffffffffffff : aql_compute_ring->buf_mask;
	aql_compute_ring->wptr_cpu_addr = (volatile uint32_t *)(info->wptr_poll_memory);
	aql_compute_ring->doorbell_index = info->doorbell_offset_in_dword;
	aql_compute_ring->max_dw = AQL_COMP_RING_MAX_DWORD;
}

void amdgv_gfx_init_dump_cu_packet(struct amdgv_adapter *adapt,
				   hsa_kernel_dispatch_packet_t *packet,
				   struct amdgv_dump_cu_resource_size *resource_size,
				   hsa_signal_t signal, struct amdgv_memmgr_mem *kernelobj,
				   struct amdgv_memmgr_mem *kernelarg)
{
	packet->header |= HSA_PACKET_TYPE_KERNEL_DISPATCH << HSA_PACKET_HEADER_TYPE;
	packet->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
	packet->header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;
	packet->setup = 1 << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
	packet->workgroup_size_x = resource_size->workgroup_size_x;
	packet->workgroup_size_y = resource_size->workgroup_size_y;
	packet->workgroup_size_z = resource_size->workgroup_size_z;
	packet->grid_size_x = resource_size->grid_size_x;
	packet->grid_size_y = resource_size->grid_size_y;
	packet->grid_size_z = resource_size->grid_size_z;
	packet->private_segment_size = resource_size->private_segment_size;
	packet->group_segment_size = resource_size->group_segment_size;
	packet->kernel_object = amdgv_memmgr_get_gpu_addr(kernelobj);
	packet->kernarg_address = (void *)(amdgv_memmgr_get_gpu_addr(kernelarg));
	packet->completion_signal = signal;
}

int amdgv_gfx_alloc_dump_cu_resource_memory(struct amdgv_adapter *adapt, struct amdgv_dump_cu_resource_size *resource_size,
											struct amdgv_dump_cu_resource_memory *resource_mem)
{
	struct amdgv_memmgr_mem *kernelobj, *kernelarg, *out_data, *out_flag, *packet, *signal_obj;
	uint32_t out_data_size, out_flag_size;
	hsa_signal_t signal;

	uint64_t *kernarg_cpua, *signal_obj_cpua;
	void *out_data_cpua = NULL;
	void *out_flag_cpua = NULL;
	uint64_t out_data_gpua, out_flag_gpua;

	out_data_size = resource_size->out_data_size;
	out_flag_size = resource_size->out_flag_size;

	// Allocate the memory:
	// kernelarg: hold address of out_data and out_flag
	// kernelobj: hsa kernel obj and shader
	// out_data: hold dump data
	// out_flag: dump flag which indicates the valid data position
	// signal_obj: completion signal
	// packet: aql packet
	kernelarg = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, 256, 256, MEM_GFX_IB);
	if (!kernelarg) {
		AMDGV_WARN("failed to create kernelarg.\n");
		return AMDGV_FAILURE;
	}
	out_data = amdgv_memmgr_alloc_sys_align_zero(&adapt->memmgr_sys, out_data_size, PAGE_SIZE, &out_data_gpua, out_data_cpua);
	if (!out_data) {
		AMDGV_WARN("failed to create out_data.\n");
		goto free_kernelarg;
	}
	out_flag = amdgv_memmgr_alloc_sys_align_zero(&adapt->memmgr_sys, out_flag_size, PAGE_SIZE, &out_flag_gpua, out_flag_cpua);
	if (!out_flag) {
		AMDGV_WARN("failed to create out_flag.\n");
		goto free_out_data;
	}
	kernelobj = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, resource_size->kernelobj_size, 256, MEM_GFX_IB);
	if (!kernelobj) {
		AMDGV_WARN("failed to create kernelobj.\n");
		goto free_out_flag;
	}
	signal_obj = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, 256, 256, MEM_GFX_IB);
	if (!signal_obj) {
		AMDGV_WARN("failed to create signal_obj.\n");
		goto free_kernelobj;
	}
	packet = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, sizeof(hsa_kernel_dispatch_packet_t), 256, MEM_GFX_IB);
	if (!packet) {
		AMDGV_WARN("failed to create packet.\n");
		goto free_signalobj;
	}

	if (out_data->sys_mem.va_ptr)
		out_data_cpua = out_data->sys_mem.va_ptr;
	if (out_flag->sys_mem.va_ptr)
		out_flag_cpua = out_flag->sys_mem.va_ptr;

	resource_mem->kernelobj_addr = (uint32_t *)amdgv_memmgr_get_cpu_addr(kernelobj);
	resource_mem->out_data_addr = (uint32_t *)out_data_cpua;
	resource_mem->out_flag_addr = (uint32_t *)out_flag_cpua;

	signal.handle = amdgv_memmgr_get_gpu_addr(signal_obj);
	signal_obj_cpua = (uint64_t *)amdgv_memmgr_get_cpu_addr(signal_obj);
	oss_memset((uint64_t *)out_data_cpua, 2, out_data_size);
	kernarg_cpua = (uint64_t *)amdgv_memmgr_get_cpu_addr(kernelarg);

	kernarg_cpua[0] = out_data_gpua;
	kernarg_cpua[1] = out_flag_gpua;

	adapt->gfx.dump_cu_packets[0] =
		(hsa_kernel_dispatch_packet_t *)amdgv_memmgr_get_cpu_addr(packet);
	amdgv_gfx_init_dump_cu_packet(adapt, adapt->gfx.dump_cu_packets[0], resource_size,
				      signal, kernelobj, kernelarg);

	adapt->gfx.dump_cu_memmgr_mem_group =
			(struct amdgv_dump_cu_memmgr_mem_group *)(oss_zalloc(sizeof(struct amdgv_dump_cu_memmgr_mem_group)));
	if (!adapt->gfx.dump_cu_memmgr_mem_group) {
		goto free_packet;
	}

	adapt->gfx.dump_cu_memmgr_mem_group->kernelobj = kernelobj;
	adapt->gfx.dump_cu_memmgr_mem_group->kernelarg = kernelarg;
	adapt->gfx.dump_cu_memmgr_mem_group->out_data = out_data;
	adapt->gfx.dump_cu_memmgr_mem_group->out_flag = out_flag;
	adapt->gfx.dump_cu_memmgr_mem_group->signal_obj = signal_obj;
	adapt->gfx.dump_cu_memmgr_mem_group->packet = packet;

	return 0;

free_packet:
	adapt->gfx.dump_cu_packets[0] = NULL;
	amdgv_memmgr_free(packet);
free_signalobj:
	amdgv_memmgr_free(signal_obj);
free_kernelobj:
	amdgv_memmgr_free(kernelobj);
free_out_flag:
	amdgv_memmgr_free(out_flag);
free_out_data:
	amdgv_memmgr_free(out_data);
free_kernelarg:
	amdgv_memmgr_free(kernelarg);

	return AMDGV_FAILURE;
}

void amdgv_gfx_free_dump_cu_resource_memory(struct amdgv_adapter *adapt)
{
	struct amdgv_dump_cu_memmgr_mem_group *mem_group = adapt->gfx.dump_cu_memmgr_mem_group;

	if (!mem_group)
		return;

	amdgv_memmgr_free(mem_group->kernelobj);
	amdgv_memmgr_free(mem_group->kernelarg);
	amdgv_memmgr_free(mem_group->out_data);
	amdgv_memmgr_free(mem_group->out_flag);
	amdgv_memmgr_free(mem_group->signal_obj);
	amdgv_memmgr_free(mem_group->packet);

	oss_free(mem_group);
	adapt->gfx.dump_cu_memmgr_mem_group = NULL;
	adapt->gfx.dump_cu_packets[0] = NULL;
	adapt->gfx.dump_cu_packets[1] = NULL;
}

int amdgv_gfx_dump_cu_data(struct amdgv_adapter *adapt)
{
	int i;
	int r = 0;
	struct amdgv_ring *mec_ring = NULL;
	struct oss_aql_comp_rb_info *aql_comp_rb_info = NULL;

	if (adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE) {
		aql_comp_rb_info = (struct oss_aql_comp_rb_info *)(oss_alloc_memory(sizeof(struct oss_aql_comp_rb_info)));
		if (aql_comp_rb_info == NULL)
			return AMDGV_FAILURE;

		if (oss_map_queue(adapt->dev, true, OSS_COMPUTE_AQL_QUEUE, aql_comp_rb_info)) {
			amdgv_gfx_set_aql_comp_ring_info(adapt, &adapt->aql_compute_ring, aql_comp_rb_info);
			mec_ring = &(adapt->aql_compute_ring);
		} else {
			AMDGV_ERROR("Map queue failed.\n");
			r = AMDGV_FAILURE;
			goto clean;
		}
	} else {
		amdgv_gfx_map_kcq(adapt, 0, XCC_QUEUE_INDEX__AQL);
		mec_ring = &(adapt->gfx.compute_ring[XCC_QUEUE_INDEX__AQL]);
	}

	if (amdgv_ring_alloc(mec_ring, sizeof(hsa_kernel_dispatch_packet_t)/sizeof(uint32_t))) {
		AMDGV_WARN("failed to allocate ring.\n");
		r = AMDGV_FAILURE;
		goto unmap;
	}

	for (i = 0; i < sizeof(hsa_kernel_dispatch_packet_t)/sizeof(uint32_t); i++) {
		amdgv_ring_write(mec_ring, ((uint32_t *)adapt->gfx.dump_cu_packets[0])[i]);
	}

	amdgv_ring_commit(mec_ring);
	oss_msleep(100);

unmap:
	if (adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)
		oss_map_queue(adapt->dev, false, OSS_COMPUTE_AQL_QUEUE, NULL);
	else
		amdgv_gfx_unmap_kcq(adapt, 0, XCC_QUEUE_INDEX__AQL);
clean:
	if (adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)
		oss_free_memory(aql_comp_rb_info);

	return r;
}

bool amdgv_gfx_is_gfx_off(struct amdgv_adapter *adapt)
{
	if (adapt->gfx.funcs && adapt->gfx.funcs->is_gfx_off) {
		return adapt->gfx.funcs->is_gfx_off(adapt);
	} else {
		AMDGV_ERROR("is_gfx_off callback not implemented!\n");
		return true;
	}
}

int amdgv_gfx_check_rlc_autoload_complete(struct amdgv_adapter *adapt)
{
	if (adapt->gfx.funcs && adapt->gfx.funcs->check_rlc_autoload_complete)
		return adapt->gfx.funcs->check_rlc_autoload_complete(adapt);
	else {
		AMDGV_ERROR("check_rlc_autoload_complete callback not implemented!\n");
		return AMDGV_FAILURE;
	}
}

int amdgv_gfx_set_clockgating_state(struct amdgv_adapter *adapt, bool enable)
{
	if (adapt->gfx.funcs && adapt->gfx.funcs->set_clockgating_state)
		return adapt->gfx.funcs->set_clockgating_state(adapt, enable);
	else {
		AMDGV_ERROR("set_clockgating_state callback not implemented!\n");
		return AMDGV_FAILURE;
	}
}
