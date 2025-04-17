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
#include "amdgv.h"
#include "amdgv_device.h"
#include "mi300/GC/gc_9_4_3_offset.h"
#include "mi300/GC/gc_9_4_3_sh_mask.h"
#include "mi_gfx.h"
#include "gfx_v9_4_3.h"
#include <amdgv_powerplay.h>

static const uint32_t this_block = AMDGV_GFX_BLOCK;

static void gfx_v9_4_3_query_ras_error_count(struct amdgv_adapter *adapt,
					void *ras_error_status)
{
	adapt->mca.funcs->pop_block_error_count(adapt, AMDGV_RAS_BLOCK__GFX, ras_error_status);
}

struct amdgv_gfx_funcs gfx_v9_4_3_mi300_funcs = {
	.err_cnt_init = NULL,
	.reset_ras_error_count = NULL,
	.query_ras_error_status = NULL,
	.reset_ras_error_status = NULL,
	.query_ras_error_count = gfx_v9_4_3_query_ras_error_count,
};

void gfx_v9_4_3_set_funcs(struct amdgv_adapter *adapt)
{
	adapt->gfx.funcs = &gfx_v9_4_3_mi300_funcs;
}

/* Enable KIQ and set CP_RLC_SCHEDULER on PF */
static unsigned int order_base_2(unsigned int size_of_dwords)
{
	unsigned int i, size_of_log2 = 0;

	for (i = 0; i < 32; i++) {
		if (size_of_dwords == (1U << i)) {
			size_of_log2 = i;
			break;
		}
	}

	return size_of_log2;
}

static void gfx_v9_4_3_kiq_set_resources(struct amdgv_ring *kiq_ring, uint64_t queue_mask)
{
	amdgv_ring_write(kiq_ring, PACKET3(PACKET3_SET_RESOURCES, 6));
	amdgv_ring_write(kiq_ring, PACKET3_SET_RESOURCES_VMID_MASK(0) |
			/* vmid_mask:0* queue_type:0 (KIQ) */
			PACKET3_SET_RESOURCES_QUEUE_TYPE(0));
	amdgv_ring_write(kiq_ring, lower_32_bits(queue_mask));	/* queue mask lo */
	amdgv_ring_write(kiq_ring, upper_32_bits(queue_mask));	/* queue mask hi */
	amdgv_ring_write(kiq_ring, 0);	/* gws mask lo */
	amdgv_ring_write(kiq_ring, 0);	/* gws mask hi */
	amdgv_ring_write(kiq_ring, 0);	/* oac mask */
	amdgv_ring_write(kiq_ring, 0);	/* gds heap base:0, gds heap size:0 */
}

static void gfx_v9_4_3_kiq_map_queues(struct amdgv_ring *kiq_ring, struct amdgv_ring *ring)
{
	uint64_t mqd_addr = ring->mqd_gpu_addr;
	uint64_t wptr_addr = ring->wptr_gpu_addr;
	uint32_t eng_sel = ring->funcs->type == AMDGV_RING_TYPE_GFX ? 4 : 0;

	amdgv_ring_write(kiq_ring, PACKET3(PACKET3_MAP_QUEUES, 5));
	/* Q_sel:0, vmid:0, vidmem: 1, engine:0, num_Q:1*/
	amdgv_ring_write(kiq_ring, /* Q_sel: 0, vmid: 0, engine: 0, num_Q: 1 */
			 PACKET3_MAP_QUEUES_QUEUE_SEL(0) | /* Queue_Sel */
			 PACKET3_MAP_QUEUES_VMID(0) | /* VMID */
			 PACKET3_MAP_QUEUES_QUEUE(ring->queue) |
			 PACKET3_MAP_QUEUES_PIPE(ring->pipe) |
			 PACKET3_MAP_QUEUES_ME((ring->me == 1 ? 0 : 1)) |
			 /*queue_type: normal compute queue */
			 PACKET3_MAP_QUEUES_QUEUE_TYPE(0) |
			 /* alloc format: all_on_one_pipe */
			 PACKET3_MAP_QUEUES_ALLOC_FORMAT(0) |
			 PACKET3_MAP_QUEUES_ENGINE_SEL(eng_sel) |
			 /* num_queues: must be 1 */
			 PACKET3_MAP_QUEUES_NUM_QUEUES(1));
	amdgv_ring_write(kiq_ring,
		PACKET3_MAP_QUEUES_DOORBELL_OFFSET(ring->doorbell_index));
	amdgv_ring_write(kiq_ring, lower_32_bits(mqd_addr));
	amdgv_ring_write(kiq_ring, upper_32_bits(mqd_addr));
	amdgv_ring_write(kiq_ring, lower_32_bits(wptr_addr));
	amdgv_ring_write(kiq_ring, upper_32_bits(wptr_addr));
}

static void gfx_v9_4_3_kiq_unmap_queues(struct amdgv_ring *kiq_ring,
				   struct amdgv_ring *ring,
				   enum amdgv_unmap_queues_action action,
				   uint64_t gpu_addr, uint64_t seq)
{
	uint32_t eng_sel = ring->funcs->type == AMDGV_RING_TYPE_GFX ? 4 : 0;

	amdgv_ring_write(kiq_ring, PACKET3(PACKET3_UNMAP_QUEUES, 4));
	amdgv_ring_write(kiq_ring, /* Q_sel: 0, vmid: 0, engine: 0, num_Q: 1 */
			  PACKET3_UNMAP_QUEUES_ACTION(action) |
			  PACKET3_UNMAP_QUEUES_QUEUE_SEL(0) |
			  PACKET3_UNMAP_QUEUES_ENGINE_SEL(eng_sel) |
			  PACKET3_UNMAP_QUEUES_NUM_QUEUES(1));
	amdgv_ring_write(kiq_ring,
		PACKET3_UNMAP_QUEUES_DOORBELL_OFFSET0(ring->doorbell_index));

	if (action == PREEMPT_QUEUES_NO_UNMAP) {
		amdgv_ring_write(kiq_ring, lower_32_bits(gpu_addr));
		amdgv_ring_write(kiq_ring, upper_32_bits(gpu_addr));
		amdgv_ring_write(kiq_ring, seq);
	} else {
		amdgv_ring_write(kiq_ring, 0);
		amdgv_ring_write(kiq_ring, 0);
		amdgv_ring_write(kiq_ring, 0);
	}
}

static const struct kiq_pm4_funcs gfx_v9_4_3_kiq_pm4_funcs = {
	.kiq_set_resources = gfx_v9_4_3_kiq_set_resources,
	.kiq_map_queues = gfx_v9_4_3_kiq_map_queues,
	.kiq_unmap_queues = gfx_v9_4_3_kiq_unmap_queues,
	.set_resources_size = 8,
	.map_queues_size = 7,
	.unmap_queues_size = 6,
	.query_status_size = 7,
	.invalidate_tlbs_size = 2,
};

static void gfx_v9_4_3_set_kiq_pm4_funcs(struct amdgv_adapter *adapt)
{
	int i, num_xcc;

	num_xcc = adapt->mcp.gfx.num_xcc;
	for (i = 0; i < num_xcc; i++)
		adapt->gfx.kiq[i].pmf = &gfx_v9_4_3_kiq_pm4_funcs;
}

static void soc15_grbm_select(struct amdgv_adapter *adapt, uint32_t me,
			uint32_t pipe, uint32_t queue, uint32_t vmid, int xcc_id)
{
	uint32_t grbm_gfx_cntl = 0;

	grbm_gfx_cntl =
		REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, PIPEID, pipe);
	grbm_gfx_cntl =
		REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, MEID, me);
	grbm_gfx_cntl =
		REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, VMID, vmid);
	grbm_gfx_cntl =
		REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, QUEUEID, queue);

	WREG32_SOC15_RLC_SHADOW(GC, GET_INST(GC, xcc_id), regGRBM_GFX_CNTL, grbm_gfx_cntl);
}

#define DEFAULT_SH_MEM_BASES (0x6000)
static void gfx_v9_4_3_xcc_init_compute_vmid(struct amdgv_adapter *adapt, int xcc_id)
{
	int i;
	uint32_t sh_mem_config;
	uint32_t sh_mem_bases;
	uint32_t data;

	/*
	 * Configure apertures:
	 * LDS:         0x60000000'00000000 - 0x60000001'00000000 (4GB)
	 * Scratch:     0x60000001'00000000 - 0x60000002'00000000 (4GB)
	 * GPUVM:       0x60010000'00000000 - 0x60020000'00000000 (1TB)
	 */
	sh_mem_bases = DEFAULT_SH_MEM_BASES | (DEFAULT_SH_MEM_BASES << 16);

	sh_mem_config = SH_MEM_ADDRESS_MODE_64 |
			SH_MEM_ALIGNMENT_MODE_UNALIGNED <<
			SH_MEM_CONFIG__ALIGNMENT_MODE__SHIFT;

	oss_mutex_lock(adapt->srbm_mutex);
	for (i = FIRST_KFD_VMID; i < AMDGV_NUM_VMID; i++) {
		soc15_grbm_select(adapt, 0, 0, 0, i, xcc_id);
		/* CP and shaders */
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regSH_MEM_CONFIG, sh_mem_config);
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regSH_MEM_BASES, sh_mem_bases);

		/* Enable trap for each kfd vmid. */
		data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regSPI_GDBG_PER_VMID_CNTL);
		data = REG_SET_FIELD(data, SPI_GDBG_PER_VMID_CNTL, TRAP_EN, 1);
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regSPI_GDBG_PER_VMID_CNTL, data);
	}
	soc15_grbm_select(adapt, 0, 0, 0, 0, xcc_id);
	oss_mutex_unlock(adapt->srbm_mutex);

	/* Initialize all compute VMIDs to have no GDS, GWS, or OA
	   acccess. These should be enabled by FW for target VMIDs. */
	for (i = FIRST_KFD_VMID; i < AMDGV_NUM_VMID; i++) {
		WREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc_id), regGDS_VMID0_BASE, 2 * i, 0);
		WREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc_id), regGDS_VMID0_SIZE, 2 * i, 0);
		WREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc_id), regGDS_GWS_VMID0, i, 0);
		WREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc_id), regGDS_OA_VMID0, i, 0);
	}
}

static void gfx_v9_4_3_xcc_init_gds_vmid(struct amdgv_adapter *adapt, int xcc_id)
{
	int vmid;

	/*
	 * Initialize all compute and user-gfx VMIDs to have no GDS, GWS, or OA
	 * access. Compute VMIDs should be enabled by FW for target VMIDs,
	 * the driver can enable them for graphics. VMID0 should maintain
	 * access so that HWS firmware can save/restore entries.
	 */
	for (vmid = 1; vmid < AMDGV_NUM_VMID; vmid++) {
		WREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc_id), regGDS_VMID0_BASE, 2 * vmid, 0);
		WREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc_id), regGDS_VMID0_SIZE, 2 * vmid, 0);
		WREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc_id), regGDS_GWS_VMID0, vmid, 0);
		WREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc_id), regGDS_OA_VMID0, vmid, 0);
	}
}

static void gfx_v9_4_3_xcc_constants_init(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t tmp;
	int i;

	oss_mutex_lock(adapt->srbm_mutex);
	for (i = 0; i < FIRST_KFD_VMID; i++) {
		soc15_grbm_select(adapt, 0, 0, 0, i, xcc_id);
		/* CP and shaders */
		if (i == 0) {
			tmp = REG_SET_FIELD(0, SH_MEM_CONFIG, ALIGNMENT_MODE,
					    SH_MEM_ALIGNMENT_MODE_UNALIGNED);
			tmp = REG_SET_FIELD(tmp, SH_MEM_CONFIG, RETRY_DISABLE, 1);
			WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id),
					 regSH_MEM_CONFIG, tmp);
			WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id),
					 regSH_MEM_BASES, 0);
		} else {
			tmp = REG_SET_FIELD(0, SH_MEM_CONFIG, ALIGNMENT_MODE,
					    SH_MEM_ALIGNMENT_MODE_UNALIGNED);
			tmp = REG_SET_FIELD(tmp, SH_MEM_CONFIG, RETRY_DISABLE, 1);
			WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id),
					 regSH_MEM_CONFIG, tmp);
			tmp = REG_SET_FIELD(0, SH_MEM_BASES, PRIVATE_BASE,
				(0x1000000000000000ULL >> 48));
			tmp = REG_SET_FIELD(tmp, SH_MEM_BASES, SHARED_BASE,
				(0x2000000000000000ULL >> 48));
			WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id),
					 regSH_MEM_BASES, tmp);
		}
	}
	soc15_grbm_select(adapt, 0, 0, 0, 0, 0);

	oss_mutex_unlock(adapt->srbm_mutex);

	gfx_v9_4_3_xcc_init_compute_vmid(adapt, xcc_id);
	gfx_v9_4_3_xcc_init_gds_vmid(adapt, xcc_id);
}

static void gfx_v9_4_3_constants_init(struct amdgv_adapter *adapt)
{
	int i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++)
		gfx_v9_4_3_xcc_constants_init(adapt, i);
}

static int gfx_v9_4_3_gpu_early_init(struct amdgv_adapter *adapt)
{
	adapt->gfx.config.max_hw_contexts = 8;
	adapt->gfx.config.sc_prim_fifo_size_frontend = 0x20;
	adapt->gfx.config.sc_prim_fifo_size_backend = 0x100;
	adapt->gfx.config.sc_hiz_tile_fifo_size = 0x30;
	adapt->gfx.config.sc_earlyz_tile_fifo_size = 0x4C0;
	return 0;
}

static bool is_gfx_v9_4_3_paging_compute_queue(struct amdgv_adapter *adapt, uint32_t xcc_id,
		uint32_t me, uint32_t pipe, uint32_t queue)
{
	bool result = false;

	if (!xcc_id &&
		adapt->gfx.mec.paging_me == me &&
		adapt->gfx.mec.paging_pipe == pipe  &&
		adapt->gfx.mec.paging_queue == queue) {
		result = true;
	}
	return result;
}

static int gfx_v9_4_3_compute_ring_init(struct amdgv_adapter *adapt, int ring_id,
				      int xcc_id, int mec, int pipe, int queue)
{
	struct amdgv_ring *ring;
	unsigned int hw_prio;
	uint32_t xcc_doorbell_start;
	bool is_paging_queue = false;
	uint32_t frame_dword_size = 1024;
	uint32_t frame_number = 2;

	ring = &adapt->gfx.compute_ring[xcc_id * adapt->gfx.num_compute_rings + ring_id];

	/* mec0 is me1 */
	ring->xcc_id = xcc_id;
	ring->me = mec + 1;
	ring->pipe = pipe;
	ring->queue = queue;
	if (is_gfx_v9_4_3_paging_compute_queue(adapt, ring->xcc_id, ring->me, ring->pipe, ring->queue)) {
		is_paging_queue = true;
		adapt->gfx.compute_paging_queue_id = ring_id;
	}

	ring->ring_obj = NULL;
	ring->use_doorbell = true;
	xcc_doorbell_start = adapt->doorbell_index.mec_ring0 +
					xcc_id * adapt->doorbell_index.xcc_doorbell_range;
	ring->doorbell_index = (xcc_doorbell_start + ring_id) << 1;
	oss_vsnprintf(ring->name, 14, "comp_%d.%d.%d.%d",
					ring->xcc_id, ring->me, ring->pipe, ring->queue);

	hw_prio = amdgv_gfx_is_high_priority_compute_queue(adapt, ring) ?
			AMDGV_GFX_PIPE_PRIO_HIGH : AMDGV_GFX_PIPE_PRIO_NORMAL;
	/* type-2 packets are deprecated on MEC, use type-3 instead */
	if (is_paging_queue) {
		frame_dword_size = adapt->opt.paging_queue_frame_bytes_size / sizeof(uint32_t);
		frame_number = adapt->opt.paging_queue_frame_number;
	}
	return amdgv_ring_init(adapt, ring, frame_dword_size, frame_number, hw_prio, NULL, MEM_COMPUTE0_RING + ring_id);
}

static void gfx_v9_4_3_mec_fini(struct amdgv_adapter *adapt)
{
	amdgv_memmgr_free(adapt->gfx.mec.hpd_eop_obj);
}

static int gfx_v9_4_3_mec_init(struct amdgv_adapter *adapt)
{
	int i, num_xcc;
	uint32_t mec_hpd_size;

	num_xcc = adapt->mcp.gfx.num_xcc;
	for (i = 0; i < num_xcc; i++)
		adapt->gfx.mec_queue_bitmap[i] = 0;

	/* take ownership of the relevant compute queues */
	amdgv_gfx_compute_queue_acquire(adapt);
	mec_hpd_size = adapt->gfx.num_compute_rings * num_xcc * GFX9_MEC_HPD_SIZE;
	if (mec_hpd_size) {
		adapt->gfx.mec.hpd_eop_obj =
			amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					 mec_hpd_size, PAGE_SIZE, MEM_GFX_EOP);
		if (!adapt->gfx.mec.hpd_eop_obj) {
			AMDGV_WARN("create HDP EOP bo failed\n");
			gfx_v9_4_3_mec_fini(adapt);
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

static int gfx_v9_4_3_mec_init_set(struct amdgv_adapter *adapt)
{
	uint32_t i, *hpd;
	uint32_t mec_hpd_size = adapt->gfx.num_compute_rings *
					adapt->mcp.gfx.num_xcc * GFX9_MEC_HPD_SIZE;

	if (mec_hpd_size) {
		adapt->gfx.mec.hpd_eop_gpu_addr =
			amdgv_memmgr_get_gpu_addr(adapt->gfx.mec.hpd_eop_obj);
		hpd =
			amdgv_memmgr_get_cpu_addr(adapt->gfx.mec.hpd_eop_obj);

		oss_memset(hpd, 0, mec_hpd_size);
		for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
			adapt->gfx.compute_ring[i].eop_gpu_addr =
				adapt->gfx.mec.hpd_eop_gpu_addr + i * GFX9_MEC_HPD_SIZE;
		}
	}

	return 0;
}

static int gfx_v9_4_3_early_init(struct amdgv_adapter *adapt);

static int gfx_v9_4_3_sw_init_internal(struct amdgv_adapter *adapt)
{
	int i, j, k, r, ring_id, xcc_id, num_xcc;
	struct amdgv_kiq *kiq;

	if (in_whole_gpu_reset())
		return 0;

	adapt->gfx.mec.num_mec = 2;
	adapt->gfx.mec.num_pipe_per_mec = 4;
	adapt->gfx.mec.num_queue_per_pipe = 8;
	adapt->gfx.mec.mec_hpd_size = GFX9_MEC_HPD_SIZE;
	num_xcc = adapt->mcp.gfx.num_xcc;

	adapt->gfx.mec.paging_me = 1;
	adapt->gfx.mec.paging_pipe = 1;
	adapt->gfx.mec.paging_queue = 0;

	gfx_v9_4_3_early_init(adapt);

	adapt->gfx.gfx_current_status = AMDGV_GFX_NORMAL_MODE;

	r = gfx_v9_4_3_mec_init(adapt);
	if (r) {
		AMDGV_ERROR("Failed to init MEC BOs!\n");
		return r;
	}

	/* set up the compute queues - allocate horizontally across pipes */
	for (xcc_id = 0; xcc_id < num_xcc; xcc_id++) {
		ring_id = 0;
		for (i = 0; i < adapt->gfx.mec.num_mec; ++i) {
			for (j = 0; j < adapt->gfx.mec.num_queue_per_pipe; j++) {
				for (k = 0; k < adapt->gfx.mec.num_pipe_per_mec; k++) {
					if (!amdgv_gfx_is_mec_queue_enabled(adapt, xcc_id, i, k, j))
						continue;

					r = gfx_v9_4_3_compute_ring_init(adapt, ring_id, xcc_id, i, k, j);
					if (r)
						return r;

					ring_id++;
				}
			}
		}

		r = amdgv_gfx_kiq_init(adapt, GFX9_MEC_HPD_SIZE, xcc_id);
		if (r) {
			AMDGV_ERROR("Failed to init KIQ BOs!\n");
			return r;
		}

		kiq = &adapt->gfx.kiq[xcc_id];
		kiq->ring.me = 2;
		kiq->ring.pipe = 1;
		kiq->ring.queue = 0;
		r = amdgv_gfx_kiq_init_ring(adapt, &kiq->ring, xcc_id);
		if (r)
			return r;

		/* create MQD for all compute queues as wel as KIQ for SRIOV case */
		r = amdgv_gfx_mqd_sw_init(adapt, sizeof(struct v9_mqd_allocation), xcc_id);
		if (r)
			return r;
	}

	r = gfx_v9_4_3_gpu_early_init(adapt);
	if (r)
		return r;

	return 0;
}

static int gfx_v9_4_3_hw_init_internal_set(struct amdgv_adapter *adapt)
{
	int i, j, k, ring_id, r, xcc_id, num_xcc;
	struct amdgv_ring *ring;

	r = gfx_v9_4_3_mec_init_set(adapt);
	if (r) {
		AMDGV_ERROR("Failed to init MEC set!\n");
		return r;
	}

	num_xcc = adapt->mcp.gfx.num_xcc;

	for (xcc_id = 0; xcc_id < num_xcc; xcc_id++) {
		ring_id = 0;
		for (i = 0; i < adapt->gfx.mec.num_mec; ++i) {
			for (j = 0; j < adapt->gfx.mec.num_queue_per_pipe; j++) {
				for (k = 0; k < adapt->gfx.mec.num_pipe_per_mec; k++) {
					if (!amdgv_gfx_is_mec_queue_enabled(adapt, xcc_id, i, k, j))
						continue;

					ring = &adapt->gfx.compute_ring[xcc_id * adapt->gfx.num_compute_rings + ring_id];
					r = amdgv_ring_init_set(adapt, ring);
					if (r)
						return r;

					ring_id++;
				}
			}
		}

		r = amdgv_gfx_kiq_init_set(adapt, GFX9_MEC_HPD_SIZE, xcc_id);
		if (r) {
			AMDGV_ERROR("Failed to init KIQ BOs!\n");
			return r;
		}

		ring = &adapt->gfx.kiq[xcc_id].ring;
		r = amdgv_ring_init_set(adapt, ring);
		if (r)
			return r;

		amdgv_gfx_mqd_init_set(adapt, xcc_id);
	}

	return 0;
}

static int gfx_v9_4_3_sw_fini_internal(struct amdgv_adapter *adapt)
{
	int i, num_xcc;

	num_xcc = adapt->mcp.gfx.num_xcc;
	for (i = 0; i < adapt->gfx.num_compute_rings * num_xcc; i++)
		amdgv_ring_fini(&adapt->gfx.compute_ring[i]);

	for (i = 0; i < num_xcc; i++) {
		amdgv_gfx_mqd_sw_fini(adapt, i);
		amdgv_gfx_kiq_free_ring(&adapt->gfx.kiq[i].ring);
		amdgv_gfx_kiq_fini(adapt, i);
	}

	gfx_v9_4_3_mec_fini(adapt);

	return 0;
}

static int gfx_v9_4_3_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)) {
		ret = gfx_v9_4_3_sw_init_internal(adapt);
	}
	return ret;
}

static int gfx_v9_4_3_sw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)) {
		ret = gfx_v9_4_3_sw_fini_internal(adapt);
	}
	return ret;
}

static void gfx_v9_4_3_xcc_enable_save_restore_machine(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t reg;
	reg = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SRM_CNTL);
	reg = REG_SET_FIELD(reg, RLC_SRM_CNTL, SRM_ENABLE, 1);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SRM_CNTL, reg);
}

static void gfx_v9_4_3_xcc_init_pg(struct amdgv_adapter *adapt, int xcc_id)
{
	gfx_v9_4_3_xcc_enable_save_restore_machine(adapt, xcc_id);
}

static void gfx_v9_4_3_xcc_enable_interrupt(struct amdgv_adapter *adapt,
							bool enable, int xcc_id)
{
	uint32_t tmp;
	struct amdgv_ring *ring;

	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, CNTX_BUSY_INT_ENABLE, enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, CNTX_EMPTY_INT_ENABLE, enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0, CMP_BUSY_INT_ENABLE, enable ? 1 : 0);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0, tmp);

	if (!xcc_id) {
		ring = &adapt->gfx.compute_ring[adapt->gfx.compute_paging_queue_id];
		soc15_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0, 0);
		tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCPC_INT_CNTL);
		tmp = REG_SET_FIELD(tmp, CPC_INT_CNTL, TIME_STAMP_INT_ENABLE, enable ? 1 : 0);
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCPC_INT_CNTL, tmp);
		soc15_grbm_select(adapt, 0, 0, 0, 0, 0);
	}
}

static int gfx_v9_4_3_xcc_rlc_resume(struct amdgv_adapter *adapt, int xcc_id)
{
	amdgv_gfx_rlc_enter_safe_mode(adapt, xcc_id);
	gfx_v9_4_3_xcc_init_pg(adapt, xcc_id);
	amdgv_gfx_rlc_exit_safe_mode(adapt, xcc_id);

	return 0;
}

static int gfx_v9_4_3_rlc_resume(struct amdgv_adapter *adapt)
{
	int i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++)
		gfx_v9_4_3_xcc_rlc_resume(adapt, i);

	return 0;
}

static void gfx_v9_4_3_xcc_set_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t data;
	unsigned i;

	data = RLC_SAFE_MODE__CMD_MASK;
	data |= (1 << RLC_SAFE_MODE__MESSAGE__SHIFT);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE, data);

	/* wait for RLC_SAFE_MODE */
	for (i = 0; i < AMDGV_GFX_MAX_USEC_TIMEOUT; i++) {
		if (!REG_GET_FIELD(RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE), RLC_SAFE_MODE, CMD))
			break;
		oss_udelay(1);
	}
}

static void gfx_v9_4_3_xcc_unset_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t data;

	data = RLC_SAFE_MODE__CMD_MASK;
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE, data);
}

static bool gfx_v9_4_3_is_rlc_enabled(struct amdgv_adapter *adapt)
{
	uint32_t rlc_setting;

	/* if RLC is not enabled, do nothing */
	rlc_setting = RREG32_SOC15(GC, GET_INST(GC, 0), regRLC_CNTL);
	if (!(rlc_setting & RLC_CNTL__RLC_ENABLE_F32_MASK))
		return false;

	return true;
}

static const struct amdgv_rlc_funcs gfx_v9_4_3_rlc_funcs = {
	.is_rlc_enabled = gfx_v9_4_3_is_rlc_enabled,
	.resume = gfx_v9_4_3_rlc_resume,
	.set_safe_mode = gfx_v9_4_3_xcc_set_safe_mode,
	.unset_safe_mode = gfx_v9_4_3_xcc_unset_safe_mode,
};

static void gfx_v9_4_3_set_rlc_funcs(struct amdgv_adapter *adapt)
{
	adapt->gfx.rlc.funcs = &gfx_v9_4_3_rlc_funcs;
}

/* KIQ functions */
static void gfx_v9_4_3_xcc_kiq_setting(struct amdgv_ring *ring, int xcc_id)
{
	uint32_t tmp;
	struct amdgv_adapter *adapt = ring->adapt;

	/* tell RLC which is KIQ queue */
	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS);
	tmp &= 0xffffff00;
	tmp |= (ring->me << 5) | (ring->pipe << 3) | (ring->queue);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS, tmp);
	tmp |= 0x80;
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS, tmp);
}

static void gfx_v9_4_3_xcc_cp_compute_enable(struct amdgv_adapter *adapt, bool enable, int xcc_id)
{
	if (enable) {
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_MEC_CNTL, 0);
	} else {
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_MEC_CNTL,
			(CP_MEC_CNTL__MEC_ME1_HALT_MASK | CP_MEC_CNTL__MEC_ME2_HALT_MASK));
	}
	oss_udelay(50);
}

static void gfx_v9_4_3_mqd_set_priority(struct amdgv_ring *ring, struct v9_mqd *mqd)
{
	struct amdgv_adapter *adapt = ring->adapt;

	if (ring->funcs->type == AMDGV_RING_TYPE_COMPUTE) {
		if (amdgv_gfx_is_high_priority_compute_queue(adapt, ring)) {
			mqd->cp_hqd_pipe_priority = AMDGV_GFX_PIPE_PRIO_HIGH;
			mqd->cp_hqd_queue_priority =
				AMDGV_GFX_QUEUE_PRIORITY_MAXIMUM;
		}
	}
}

static void gfx_v9_4_3_ring_submit_frame(struct amdgv_ring *ring, uint8_t *frame_data)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint64_t dword_wptr = ring->wptr % ring->ring_size;
	uint32_t *data32 = (uint32_t *)frame_data;
	uint32_t i;

	for (i = 0; i < ring->max_dw; i++) {
		ring->ring[dword_wptr++] = data32[i];
	}
	ring->wptr += ring->max_dw;
	// Both ring->wptr and CP WPTR are DWORD index
	dword_wptr = ring->wptr;
	*((volatile uint64_t *)(ring->wptr_cpu_addr)) = dword_wptr;

	if (ring->use_doorbell) {
		WDOORBELL64(ring->doorbell_index, dword_wptr);
	} else {
		oss_mutex_lock(adapt->srbm_mutex);
		soc15_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0, 0);
		WREG32_SOC15(GC, GET_INST(GC, ring->xcc_id), regCP_HQD_PQ_WPTR_LO, (uint32_t)(dword_wptr));
		WREG32_SOC15(GC, GET_INST(GC, ring->xcc_id), regCP_HQD_PQ_WPTR_HI, (uint32_t)(dword_wptr >> 32));
		soc15_grbm_select(adapt, 0, 0, 0, 0, 0);
		oss_mutex_unlock(adapt->srbm_mutex);
	}
}

static int gfx_v9_4_3_xcc_mqd_init(struct amdgv_ring *ring, int xcc_id)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v9_mqd_allocation *mqd_alloc = (struct v9_mqd_allocation *)ring->mqd_ptr;
	struct v9_mqd *mqd = &mqd_alloc->mqd;
	uint64_t hqd_gpu_addr, wb_gpu_addr, eop_base_addr;
	uint32_t tmp;

	mqd_alloc->dynamic_cu_mask = 0xFFFFFFFF;
	mqd_alloc->dynamic_rb_mask = 0xFFFFFFFF;

	// TODO: map MEC queue in aql mode to dump LDS data.
	mqd->header = 0xC0310800;
	mqd->compute_pipelinestat_enable = 0x00000001;
	mqd->compute_static_thread_mgmt_se0 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se1 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se2 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se3 = 0xffffffff;
	mqd->compute_misc_reserved = 0x00000003;

	mqd->dynamic_cu_mask_addr_lo =
		lower_32_bits(ring->mqd_gpu_addr
			      + offsetof(struct v9_mqd_allocation, dynamic_cu_mask));
	mqd->dynamic_cu_mask_addr_hi =
		upper_32_bits(ring->mqd_gpu_addr
			      + offsetof(struct v9_mqd_allocation, dynamic_cu_mask));

	eop_base_addr = ring->eop_gpu_addr >> 8;
	mqd->cp_hqd_eop_base_addr_lo = eop_base_addr;
	mqd->cp_hqd_eop_base_addr_hi = upper_32_bits(eop_base_addr);

	/* set the EOP size, register value is 2^(EOP_SIZE+1) dwords */
	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_EOP_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_EOP_CONTROL, EOP_SIZE,
			(order_base_2(GFX9_MEC_HPD_SIZE / 4) - 1));

	mqd->cp_hqd_eop_control = tmp;

	/* enable doorbell? */
	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_DOORBELL_CONTROL);

	if (ring->use_doorbell) {
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
				    DOORBELL_OFFSET, ring->doorbell_index);
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
				    DOORBELL_EN, 1);
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
				    DOORBELL_SOURCE, 0);
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
				    DOORBELL_HIT, 0);
	} else {
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
					 DOORBELL_EN, 0);
	}

	mqd->cp_hqd_pq_doorbell_control = tmp;

	/* disable the queue if it's active */
	mqd->cp_hqd_dequeue_request = 0;
	mqd->cp_hqd_pq_rptr = 0;
	mqd->cp_hqd_pq_wptr_lo = 0;
	mqd->cp_hqd_pq_wptr_hi = 0;

	/* set the pointer to the MQD */
	mqd->cp_mqd_base_addr_lo = ring->mqd_gpu_addr & 0xfffffffc;
	mqd->cp_mqd_base_addr_hi = upper_32_bits(ring->mqd_gpu_addr);

	/* set MQD vmid to 0 */
	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MQD_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_MQD_CONTROL, VMID, 0);
	mqd->cp_mqd_control = tmp;

	/* set the pointer to the HQD, this is similar CP_RB0_BASE/_HI */
	hqd_gpu_addr = ring->gpu_addr >> 8;
	mqd->cp_hqd_pq_base_lo = hqd_gpu_addr;
	mqd->cp_hqd_pq_base_hi = upper_32_bits(hqd_gpu_addr);

	/* set up the HQD, this is similar to CP_RB0_CNTL */
	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, QUEUE_SIZE,
				ring->log2_ring_size - 1);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, RPTR_BLOCK_SIZE,
			((order_base_2(AMDGV_GPU_PAGE_SIZE / 4) - 1) << 8));
#ifdef __BIG_ENDIAN
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, ENDIAN_SWAP, 1);
#endif
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, UNORD_DISPATCH, 0);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, ROQ_PQ_IB_FLIP, 0);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, PRIV_STATE, 1);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, KMD_QUEUE, 1);
	mqd->cp_hqd_pq_control = tmp;

	/* set the wb address whether it's enabled or not */
	wb_gpu_addr = ring->rptr_gpu_addr;
	mqd->cp_hqd_pq_rptr_report_addr_lo = wb_gpu_addr & 0xfffffffc;
	mqd->cp_hqd_pq_rptr_report_addr_hi =
		upper_32_bits(wb_gpu_addr) & 0xffff;

	/* only used if CP_PQ_WPTR_POLL_CNTL.CP_PQ_WPTR_POLL_CNTL__EN_MASK=1 */
	wb_gpu_addr = ring->wptr_gpu_addr;
	mqd->cp_hqd_pq_wptr_poll_addr_lo = wb_gpu_addr & 0xfffffffc;
	mqd->cp_hqd_pq_wptr_poll_addr_hi = upper_32_bits(wb_gpu_addr) & 0xffff;

	/* reset read and write pointers, similar to CP_RB0_WPTR/_RPTR */
	mqd->cp_hqd_pq_rptr = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_RPTR);

	/* set the vmid for the queue */
	mqd->cp_hqd_vmid = 0;

	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PERSISTENT_STATE);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PERSISTENT_STATE, PRELOAD_SIZE, 0x53);
	mqd->cp_hqd_persistent_state = tmp;

	/* set MIN_IB_AVAIL_SIZE */
	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_IB_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_IB_CONTROL, MIN_IB_AVAIL_SIZE, 3);
	mqd->cp_hqd_ib_control = tmp;

	/* set static priority for a queue/ring */
	gfx_v9_4_3_mqd_set_priority(ring, mqd);
	mqd->cp_hqd_quantum = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_QUANTUM);

	/* map_queues packet doesn't need activate the queue,
	 * so only kiq need set this field.
	 */
	if (ring->funcs->type == AMDGV_RING_TYPE_KIQ)
		mqd->cp_hqd_active = 1;

	return 0;
}

static int gfx_v9_4_3_xcc_kiq_init_register(struct amdgv_ring *ring, int xcc_id)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v9_mqd *mqd = (struct v9_mqd *)ring->mqd_ptr;
	int j;
	uint32_t reg;

	/* disable wptr polling */
	reg = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_PQ_WPTR_POLL_CNTL);
	reg = REG_SET_FIELD(reg, CP_PQ_WPTR_POLL_CNTL, EN, 0);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_PQ_WPTR_POLL_CNTL, reg);

	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_EOP_BASE_ADDR,
	       mqd->cp_hqd_eop_base_addr_lo);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_EOP_BASE_ADDR_HI,
	       mqd->cp_hqd_eop_base_addr_hi);

	/* set the EOP size, register value is 2^(EOP_SIZE+1) dwords */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_EOP_CONTROL,
	       mqd->cp_hqd_eop_control);

	/* enable doorbell? */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_DOORBELL_CONTROL,
	       mqd->cp_hqd_pq_doorbell_control);

	/* disable the queue if it's active */
	if (RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_ACTIVE) & 1) {
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_DEQUEUE_REQUEST, 1);
		for (j = 0; j < AMDGV_GFX_MAX_USEC_TIMEOUT; j++) {
			if (!(RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_ACTIVE) & 1))
				break;
			oss_udelay(1);
		}
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_DEQUEUE_REQUEST,
		       mqd->cp_hqd_dequeue_request);
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_RPTR,
		       mqd->cp_hqd_pq_rptr);
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_LO,
		       mqd->cp_hqd_pq_wptr_lo);
		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_HI,
		       mqd->cp_hqd_pq_wptr_hi);
	}

	/* set the pointer to the MQD */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_MQD_BASE_ADDR,
	       mqd->cp_mqd_base_addr_lo);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_MQD_BASE_ADDR_HI,
	       mqd->cp_mqd_base_addr_hi);

	/* set MQD vmid to 0 */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_MQD_CONTROL,
	       mqd->cp_mqd_control);

	/* set the pointer to the HQD, this is similar CP_RB0_BASE/_HI */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_BASE,
	       mqd->cp_hqd_pq_base_lo);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_BASE_HI,
	       mqd->cp_hqd_pq_base_hi);

	/* set up the HQD, this is similar to CP_RB0_CNTL */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_CONTROL,
	       mqd->cp_hqd_pq_control);

	/* set the wb address whether it's enabled or not */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_RPTR_REPORT_ADDR,
				mqd->cp_hqd_pq_rptr_report_addr_lo);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_RPTR_REPORT_ADDR_HI,
				mqd->cp_hqd_pq_rptr_report_addr_hi);

	/* only used if CP_PQ_WPTR_POLL_CNTL.CP_PQ_WPTR_POLL_CNTL__EN_MASK=1 */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_POLL_ADDR,
	       mqd->cp_hqd_pq_wptr_poll_addr_lo);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_POLL_ADDR_HI,
	       mqd->cp_hqd_pq_wptr_poll_addr_hi);

	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MEC_DOORBELL_RANGE_LOWER,
			((adapt->doorbell_index.kiq +
			  xcc_id * adapt->doorbell_index.xcc_doorbell_range) *
			 2) << 2);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MEC_DOORBELL_RANGE_UPPER,
			((adapt->doorbell_index.userqueue_end +
			  xcc_id * adapt->doorbell_index.xcc_doorbell_range) *
			 2) << 2);

	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_DOORBELL_CONTROL,
	       mqd->cp_hqd_pq_doorbell_control);

	/* reset read and write pointers, similar to CP_RB0_WPTR/_RPTR */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_LO,
	       mqd->cp_hqd_pq_wptr_lo);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_HI,
	       mqd->cp_hqd_pq_wptr_hi);

	/* set the vmid for the queue */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_VMID, mqd->cp_hqd_vmid);

	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PERSISTENT_STATE,
	       mqd->cp_hqd_persistent_state);

	/* activate the queue */
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_ACTIVE,
	       mqd->cp_hqd_active);

	reg = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_PQ_STATUS);
	reg = REG_SET_FIELD(reg, CP_PQ_STATUS, DOORBELL_ENABLE, 1);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_PQ_STATUS, reg);
	return 0;
}

static int gfx_v9_4_3_xcc_q_fini_register(struct amdgv_ring *ring, int xcc_id)
{
	struct amdgv_adapter *adapt = ring->adapt;
	int i;

	/* disable the queue if it's active */
	if (RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_ACTIVE) & 1) {

		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_DEQUEUE_REQUEST, 1);

		for (i = 0; i < AMDGV_GFX_MAX_USEC_TIMEOUT; i++) {
			if (!(RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_ACTIVE) & 1))
				break;
			oss_udelay(1);
		}

		if (i == AMDGV_GFX_MAX_USEC_TIMEOUT) {
			AMDGV_ERROR("KIQ dequeue request failed.\n");

			/* Manual disable if dequeue request times out */
			WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_ACTIVE, 0);
		}

		WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_DEQUEUE_REQUEST, 0);
	}

	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_IQ_TIMER, 0);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_IB_CONTROL, 0);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PERSISTENT_STATE, CP_HQD_PERSISTENT_STATE_DEFAULT);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_DOORBELL_CONTROL, 0x40000000);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_DOORBELL_CONTROL, 0);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_RPTR, 0);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_HI, 0);
	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_LO, 0);

	return 0;
}

static int gfx_v9_4_3_xcc_kcq_fini_register(struct amdgv_adapter *adapt, int xcc_id)
{
	struct amdgv_ring *ring;
	int i;

	if (in_whole_gpu_reset())
		return 0;

	for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
		ring = &adapt->gfx.compute_ring[i +  xcc_id * adapt->gfx.num_compute_rings];
		oss_mutex_lock(adapt->srbm_mutex);
		soc15_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0, xcc_id);
		gfx_v9_4_3_xcc_q_fini_register(ring, xcc_id);
		soc15_grbm_select(adapt, 0, 0, 0, 0, xcc_id);
		oss_mutex_unlock(adapt->srbm_mutex);
	}

	return 0;
}

static int gfx_v9_4_3_xcc_kiq_init_queue(struct amdgv_ring *ring, int xcc_id)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v9_mqd_allocation *mqd_alloc = (struct v9_mqd_allocation *)ring->mqd_ptr;
	struct v9_mqd_allocation *init_mqd_alloc =
		(struct v9_mqd_allocation *)adapt->gfx.kiq[xcc_id].mqd_backup;

	oss_mutex_lock(adapt->srbm_mutex);
	soc15_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0, xcc_id);
	if (init_mqd_alloc && init_mqd_alloc->mqd.cp_hqd_pq_control) {
		oss_memcpy(mqd_alloc, init_mqd_alloc, sizeof(struct v9_mqd_allocation));
	} else {
		gfx_v9_4_3_xcc_mqd_init(ring, xcc_id);
		if (init_mqd_alloc)
			oss_memcpy(init_mqd_alloc, mqd_alloc, sizeof(struct v9_mqd_allocation));
	}
	gfx_v9_4_3_xcc_kiq_init_register(ring, xcc_id);
	soc15_grbm_select(adapt, 0, 0, 0, 0, xcc_id);
	oss_mutex_unlock(adapt->srbm_mutex);

	// set ring buffer WRITE pointer to the same value from MQD
	ring->wptr = ((uint64_t)mqd_alloc->mqd.cp_hqd_pq_wptr_hi << 32) + mqd_alloc->mqd.cp_hqd_pq_wptr_lo;
	*((volatile uint64_t *)(ring->wptr_cpu_addr)) = ring->wptr;

	return 0;
}

static int gfx_v9_4_3_xcc_kcq_init_queue(struct amdgv_ring *ring, int xcc_id)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v9_mqd_allocation *mqd_alloc = (struct v9_mqd_allocation *)ring->mqd_ptr;
	int mqd_idx = ring - &adapt->gfx.compute_ring[0];
	struct v9_mqd_allocation *init_mqd_alloc =
		(struct v9_mqd_allocation *)adapt->gfx.mec.mqd_backup[mqd_idx];

	if (init_mqd_alloc && init_mqd_alloc->mqd.cp_hqd_pq_control) {
		oss_memcpy(mqd_alloc, init_mqd_alloc, sizeof(struct v9_mqd_allocation));
	} else {
		oss_mutex_lock(adapt->srbm_mutex);
		soc15_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0, xcc_id);
		gfx_v9_4_3_xcc_mqd_init(ring, xcc_id);
		soc15_grbm_select(adapt, 0, 0, 0, 0, xcc_id);
		oss_mutex_unlock(adapt->srbm_mutex);
		if (init_mqd_alloc)
			oss_memcpy(init_mqd_alloc, mqd_alloc, sizeof(struct v9_mqd_allocation));
	}

	// set ring buffer WRITE pointer to the same value from MQD
	ring->wptr = ((uint64_t)mqd_alloc->mqd.cp_hqd_pq_wptr_hi << 32) + mqd_alloc->mqd.cp_hqd_pq_wptr_lo;
	*((volatile uint64_t *)(ring->wptr_cpu_addr)) = ring->wptr;

	return 0;
}

static int gfx_v9_4_3_xcc_kiq_resume(struct amdgv_adapter *adapt, int xcc_id)
{
	struct amdgv_ring *ring;

	ring = &adapt->gfx.kiq[xcc_id].ring;
	gfx_v9_4_3_xcc_kiq_setting(ring, xcc_id);
	gfx_v9_4_3_xcc_kiq_init_queue(ring, xcc_id);

	return amdgv_gfx_kiq_set_resources(adapt, xcc_id);
}

static int gfx_v9_4_3_xcc_kcq_resume(struct amdgv_adapter *adapt, int xcc_id)
{
	struct amdgv_ring *ring = NULL;
	int r = 0, i;

	gfx_v9_4_3_xcc_cp_compute_enable(adapt, true, xcc_id);

	for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
		ring = &adapt->gfx.compute_ring[i + xcc_id * adapt->gfx.num_compute_rings];
		r = gfx_v9_4_3_xcc_kcq_init_queue(ring, xcc_id);
	}

	// enable KIQ on xcc_id = 0 for SPX. To do for other configurations.
	if (!xcc_id && (adapt->flags & AMDGV_FLAG_ENABLE_COMPUTE_PAGING)) {
		r = amdgv_gfx_map_kcq(adapt, xcc_id, XCC_QUEUE_INDEX__PAGING);
	}
	return r;
}

static int gfx_v9_4_3_xcc_cp_resume(struct amdgv_adapter *adapt, int xcc_id)
{
	int r;

	gfx_v9_4_3_xcc_enable_interrupt(adapt, false, xcc_id);

	r = gfx_v9_4_3_xcc_kiq_resume(adapt, xcc_id);
	if (r)
		return r;

	r = gfx_v9_4_3_xcc_kcq_resume(adapt, xcc_id);
	if (r)
		return r;

	gfx_v9_4_3_xcc_enable_interrupt(adapt, true, xcc_id);

	return 0;
}

static int gfx_v9_4_3_cp_resume(struct amdgv_adapter *adapt)
{
	int r = 0, i, num_xcc;

	num_xcc = adapt->mcp.gfx.num_xcc;
	for (i = 0; i < num_xcc; i++) {
		r = gfx_v9_4_3_xcc_cp_resume(adapt, i);
		if (r)
			return r;
	}

	return 0;
}

static int gfx_v9_4_3_hw_init(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)) {
		gfx_v9_4_3_hw_init_internal_set(adapt);
		gfx_v9_4_3_constants_init(adapt);
		gfx_v9_4_3_rlc_resume(adapt);
		gfx_v9_4_3_cp_resume(adapt);
	}

	/* Don't stop libgv init even gfx init fails */
	return 0;
}

static void gfx_v9_4_3_wait_reg_mem(struct amdgv_ring *ring, int eng_sel,
				  int mem_space, int opt, uint32_t addr0,
				  uint32_t addr1, uint32_t ref, uint32_t mask,
				  uint32_t inv)
{
	amdgv_ring_write(ring, PACKET3(PACKET3_WAIT_REG_MEM, 5));
	amdgv_ring_write(ring,
				 /* memory (1) or register (0) */
				 (WAIT_REG_MEM_MEM_SPACE(mem_space) |
				 WAIT_REG_MEM_OPERATION(opt) | /* wait */
				 WAIT_REG_MEM_FUNCTION(3) |  /* equal */
				 WAIT_REG_MEM_ENGINE(eng_sel)));

	amdgv_ring_write(ring, addr0);
	amdgv_ring_write(ring, addr1);
	amdgv_ring_write(ring, ref);
	amdgv_ring_write(ring, mask);
	amdgv_ring_write(ring, inv); /* poll interval */
}

static void gfx_v9_4_3_ring_emit_hdp_flush(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t ref_and_mask, reg_mem_engine;
	const struct nbio_hdp_flush_reg *nbio_hf_reg = adapt->nbio.hdp_flush_reg;

	if (ring->funcs->type == AMDGV_RING_TYPE_COMPUTE) {
		switch (ring->me) {
		case 1:
			ref_and_mask = nbio_hf_reg->ref_and_mask_cp2 << ring->pipe;
			break;
		case 2:
			ref_and_mask = nbio_hf_reg->ref_and_mask_cp6 << ring->pipe;
			break;
		default:
			return;
		}
		reg_mem_engine = 0;
	} else {
		ref_and_mask = nbio_hf_reg->ref_and_mask_cp0;
		reg_mem_engine = 1; /* pfp */
	}

	gfx_v9_4_3_wait_reg_mem(ring, reg_mem_engine, 0, 1,
			      adapt->nbio.funcs->get_hdp_flush_req_offset(adapt),
			      adapt->nbio.funcs->get_hdp_flush_done_offset(adapt),
			      ref_and_mask, ref_and_mask, 0x20);
}

static void gfx_v9_4_3_ring_set_wptr_compute(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;

	/* XXX check if swapping is necessary on BE */
	if (ring->use_doorbell) {
		// TODO: setting wptr should use atomic set
		// Atomic operation on cacheable memory will cause NMI Hardware Failure (0x80)
		*ring->wptr_cpu_addr = ring->wptr;
		WDOORBELL32(ring->doorbell_index, ring->wptr);
	}
}

static void gfx_v9_4_3_ring_emit_ib_compute(struct amdgv_ring *ring, struct amdgv_ib *ib, uint32_t flags)
{
	unsigned int vmid = 0;
	uint32_t control = INDIRECT_BUFFER_VALID | ib->length_dw | (vmid << 24);

	amdgv_ring_write(ring, PACKET3(PACKET3_INDIRECT_BUFFER, 2));
	amdgv_ring_write(ring,
#ifdef __BIG_ENDIAN
				(2 << 0) |
#endif
				lower_32_bits(ib->gpu_addr));
	amdgv_ring_write(ring, upper_32_bits(ib->gpu_addr));
	amdgv_ring_write(ring, control);
}

static void gfx_v9_4_3_ring_emit_fence(struct amdgv_ring *ring, uint64_t addr,
				     uint64_t seq, unsigned int flags)
{
	bool write64bit = flags & AMDGV_FENCE_FLAG_64BIT;
	bool int_sel = flags & AMDGV_FENCE_FLAG_INT;
	bool writeback = flags & AMDGV_FENCE_FLAG_TC_WB_ONLY;

	/* RELEASE_MEM - flush caches, send int */
	amdgv_ring_write(ring, PACKET3(PACKET3_RELEASE_MEM, 6));
	amdgv_ring_write(ring, ((writeback ? (EOP_TC_WB_ACTION_EN |
					       EOP_TC_NC_ACTION_EN) :
					      (EOP_TCL1_ACTION_EN |
					       EOP_TC_ACTION_EN |
					       EOP_TC_WB_ACTION_EN |
					       EOP_TC_MD_ACTION_EN)) |
				 EVENT_TYPE(CACHE_FLUSH_AND_INV_TS_EVENT) |
				 EVENT_INDEX(5)));
	amdgv_ring_write(ring, DATA_SEL(write64bit ? 2 : 1) |
				INT_SEL(int_sel ? 2 : 0));

	/*
	 * the address should be Qword aligned if 64bit write, Dword
	 * aligned if only send 32bit data low (discard data high)
	 */
	amdgv_ring_write(ring, lower_32_bits(addr));
	amdgv_ring_write(ring, upper_32_bits(addr));
	amdgv_ring_write(ring, lower_32_bits(seq));
	amdgv_ring_write(ring, upper_32_bits(seq));
	amdgv_ring_write(ring, 0);
}

static void gfx_v9_4_3_ring_emit_wreg(struct amdgv_ring *ring, uint32_t reg, uint32_t val)
{
	uint32_t cmd = 0;

	switch (ring->funcs->type) {
	case AMDGV_RING_TYPE_GFX:
		cmd = WRITE_DATA_ENGINE_SEL(1) | WR_CONFIRM;
		break;
	case AMDGV_RING_TYPE_KIQ:
		cmd = (1 << 16); /* no inc addr */
		break;
	default:
		cmd = WR_CONFIRM;
		break;
	}
	amdgv_ring_write(ring, PACKET3(PACKET3_WRITE_DATA, 3));
	amdgv_ring_write(ring, cmd);
	amdgv_ring_write(ring, reg);
	amdgv_ring_write(ring, 0);
	amdgv_ring_write(ring, val);
}

static int gfx_v9_4_3_ring_test_ring(struct amdgv_ring *ring)
{
	uint32_t scratch_reg0_offset, xcc_offset;
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t tmp = 0;
	unsigned i;
	int r;

	/* Use register offset which is local to XCC in the packet */
	xcc_offset = SOC15_REG_OFFSET(GC, 0, regSCRATCH_REG0);
	scratch_reg0_offset = SOC15_REG_OFFSET(GC, GET_INST(GC, ring->xcc_id), regSCRATCH_REG0);
	WREG32(scratch_reg0_offset, 0xCAFEDEAD);
	tmp = RREG32(scratch_reg0_offset);

	r = amdgv_ring_alloc(ring, 3);
	if (r)
		return r;

	amdgv_ring_write(ring, PACKET3(PACKET3_SET_UCONFIG_REG, 1));
	amdgv_ring_write(ring, xcc_offset - PACKET3_SET_UCONFIG_REG_START);
	amdgv_ring_write(ring, 0xDEADBEEF);
	amdgv_ring_commit(ring);

	for (i = 0; i < AMDGV_GFX_MAX_USEC_TIMEOUT; i++) {
		tmp = RREG32(scratch_reg0_offset);
		if (tmp == 0xDEADBEEF)
			break;
		oss_udelay(1);
	}

	if (i >= AMDGV_GFX_MAX_USEC_TIMEOUT) {
		AMDGV_ERROR("amdgpu: ring(%s) failed in self-test with WPTR update\n", ring->name);
	} else {
		AMDGV_INFO("amdgpu: ring(%s) succeeded in self-test with WPTR update\n", ring->name);
	}

	if (i >= AMDGV_GFX_MAX_USEC_TIMEOUT)
		r = AMDGV_FAILURE;
	return r;
}

static const struct amdgv_ring_funcs gfx_v9_4_3_ring_funcs_compute = {
	.type = AMDGV_RING_TYPE_COMPUTE,
	.align_mask = 0xff,
	.nop = PACKET3(PACKET3_NOP, 0x3FFF),
	.support_64bit_ptrs = true,
	.set_wptr = gfx_v9_4_3_ring_set_wptr_compute,
	.submit_frame = gfx_v9_4_3_ring_submit_frame,
	.emit_frame_size =
		20 + /* gfx_v9_4_3_ring_emit_gds_switch */
		7 + /* gfx_v9_4_3_ring_emit_hdp_flush */
		5 + /* hdp invalidate */
		7 + /* gfx_v9_4_3_ring_emit_pipeline_sync */
		SOC15_FLUSH_GPU_TLB_NUM_WREG * 5 +
		SOC15_FLUSH_GPU_TLB_NUM_REG_WAIT * 7 +
		2 + /* gfx_v9_4_3_ring_emit_vm_flush */
		8 + 8 + 8 + /* gfx_v9_4_3_ring_emit_fence x3 for user fence, vm fence */
		7 + /* gfx_v9_4_3_emit_mem_sync */
		5 + /* gfx_v9_4_3_emit_wave_limit for updating regSPI_WCL_PIPE_PERCENT_GFX register */
		15, /* for updating 3 regSPI_WCL_PIPE_PERCENT_CS registers */
	.emit_ib_size =	7, /* gfx_v9_4_3_ring_emit_ib_compute */
	.emit_ib = gfx_v9_4_3_ring_emit_ib_compute,
	.emit_fence = gfx_v9_4_3_ring_emit_fence,
	.emit_hdp_flush = gfx_v9_4_3_ring_emit_hdp_flush,
	.insert_nop = amdgv_ring_insert_nop,
	.emit_wreg = gfx_v9_4_3_ring_emit_wreg,
};

static const struct amdgv_ring_funcs gfx_v9_4_3_ring_funcs_kiq = {
	.type = AMDGV_RING_TYPE_KIQ,
	.align_mask = 0xff,
	.nop = PACKET3(PACKET3_NOP, 0x3FFF),
	.support_64bit_ptrs = true,
	.vmhub = 0,
	.set_wptr = gfx_v9_4_3_ring_set_wptr_compute,
	.emit_frame_size =
		20 + /* gfx_v9_0_ring_emit_gds_switch */
		7 + /* gfx_v9_0_ring_emit_hdp_flush */
		5 + /* hdp invalidate */
		7 + /* gfx_v9_0_ring_emit_pipeline_sync */
		SOC15_FLUSH_GPU_TLB_NUM_WREG * 5 +
		SOC15_FLUSH_GPU_TLB_NUM_REG_WAIT * 7 +
		2 + /* gfx_v9_0_ring_emit_vm_flush */
		/* gfx_v9_0_ring_emit_fence_kiq x3 for user fence,
		 * vm fence
		 */
		8 + 8 + 8,
	.emit_ib_size =	7, /* gfx_v9_0_ring_emit_ib_compute */
	.test_ring = gfx_v9_4_3_ring_test_ring,
	.insert_nop = amdgv_ring_insert_nop,
	.emit_wreg = gfx_v9_4_3_ring_emit_wreg,
};

static void gfx_v9_4_3_set_ring_funcs(struct amdgv_adapter *adapt)
{
	int i, j, num_xcc;
	num_xcc = adapt->mcp.gfx.num_xcc;
	for (i = 0; i < num_xcc; i++) {
		adapt->gfx.kiq[i].ring.funcs = &gfx_v9_4_3_ring_funcs_kiq;

		for (j = 0; j < adapt->gfx.num_compute_rings; j++)
			adapt->gfx.compute_ring[j + i * adapt->gfx.num_compute_rings].funcs
					= &gfx_v9_4_3_ring_funcs_compute;
	}
}

static int gfx_v9_4_3_early_init(struct amdgv_adapter *adapt)
{
	adapt->gfx.num_gfx_rings = 0;
	adapt->gfx.num_compute_rings = AMDGV_MAX_COMPUTE_RINGS;

	gfx_v9_4_3_set_ring_funcs(adapt);
	gfx_v9_4_3_set_kiq_pm4_funcs(adapt);
	gfx_v9_4_3_set_rlc_funcs(adapt);

	return 0;
}

static void gfx_v9_4_3_xcc_fini(struct amdgv_adapter *adapt, int xcc_id)
{
	if (!xcc_id && (adapt->flags & AMDGV_FLAG_ENABLE_COMPUTE_PAGING)) {
		amdgv_gfx_unmap_kcq(adapt, xcc_id, XCC_QUEUE_INDEX__PAGING);
	}
	if (!in_whole_gpu_reset()) {
		oss_mutex_lock(adapt->srbm_mutex);
		soc15_grbm_select(adapt, adapt->gfx.kiq[xcc_id].ring.me,
					adapt->gfx.kiq[xcc_id].ring.pipe,
					adapt->gfx.kiq[xcc_id].ring.queue, 0, xcc_id);
		gfx_v9_4_3_xcc_q_fini_register(&adapt->gfx.kiq[xcc_id].ring, xcc_id);
		soc15_grbm_select(adapt, 0, 0, 0, 0, xcc_id);
		oss_mutex_unlock(adapt->srbm_mutex);
	}

	gfx_v9_4_3_xcc_kcq_fini_register(adapt, xcc_id);
	gfx_v9_4_3_xcc_cp_compute_enable(adapt, false, xcc_id);

	WREG32_SOC15_RLC(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS, 0);
}

static int gfx_v9_4_3_hw_fini(struct amdgv_adapter *adapt)
{
	int i;

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)) {
		for (i = 0; i < adapt->mcp.gfx.num_xcc; i++)
		gfx_v9_4_3_xcc_fini(adapt, i);
	}

	return 0;
}

struct amdgv_init_func mi300_gfx_v9_4_3_func = {
	.name = "mi300_gfx_func",
	.sw_init = gfx_v9_4_3_sw_init,
	.sw_fini = gfx_v9_4_3_sw_fini,
	.hw_engine_init = gfx_v9_4_3_hw_init,
	.hw_engine_fini = gfx_v9_4_3_hw_fini,
};
