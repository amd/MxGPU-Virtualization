/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "mi300/GC/gc_9_4_3_offset.h"
#include "mi300/GC/gc_9_4_3_sh_mask.h"
#include "mi_gfx.h"
#include "gfx_v9_4_3.h"
#include <amdgv_powerplay.h>
#include <amdgv_gpumon.h>
#include "mi300_gpumon.h"
#include "mi300_fb_hash_shader.h"

#define MI300_CP_HQD_SAVE_REGS_NUM (regCP_HQD_PQ_WPTR_HI - regCP_MQD_BASE_ADDR + 1)

static const uint32_t this_block = AMDGV_GFX_BLOCK;

static void gfx_v9_4_3_xcc_select_se_sh(struct amdgv_adapter *adapt, uint32_t se_num,
					uint32_t sh_num, uint32_t instance, int xcc_id)
{
	uint32_t data = 0;

	if (instance == 0xffffffff)
		data = REG_SET_FIELD(0, GRBM_GFX_INDEX,
				     INSTANCE_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(0, GRBM_GFX_INDEX,
				     INSTANCE_INDEX, instance);

	if (se_num == 0xffffffff)
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX,
				     SE_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SE_INDEX, se_num);

	if (sh_num == 0xffffffff)
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX,
				     SH_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SH_INDEX, sh_num);

	WREG32_SOC15_RLC_SHADOW_EX(reg, GC, GET_INST(GC, xcc_id), regGRBM_GFX_INDEX, data);
}

static void gfx_v9_4_3_query_ras_error_count(struct amdgv_adapter *adapt,
					void *ras_error_status)
{
	adapt->mca.funcs->pop_block_error_count(adapt, AMDGV_RAS_BLOCK__GFX, ras_error_status);
}

void gfx_v9_4_3_dirtybit_control(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t gc_value;
	int xcc_id;

	for (xcc_id = 0; xcc_id < adapt->mcp.gfx.num_xcc; xcc_id++) {
		/* There are 16 instances in one XCC, if we want to read MAM_CTRL in EA#N,
		 * we should set GRBM_GFX_INDEX.INSTANCE_INDEX to #N firstly.
		 *
		 * Broadcast the value to all the instances.
		 */
		gfx_v9_4_3_xcc_select_se_sh(adapt, 0xffffffff, 0xffffffff, 0xffffffff, xcc_id);
		gc_value = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regGCEA_MAM_CTRL));
		gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_CTRL, MAM_DISABLE, !enable);
		gc_value = REG_SET_FIELD(gc_value, GCEA_MAM_CTRL, ADRAM_MODE, adapt->dirtybit.mam_adram_mode);
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regGCEA_MAM_CTRL), gc_value);

	}
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
	int r, ring_id, xcc_id, num_xcc;
	struct amdgv_kiq *kiq;
	uint32_t i, j, k;

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
	int ring_id, r, xcc_id, num_xcc;
	struct amdgv_ring *ring;
	uint32_t i, j, k;

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
					if (!xcc_id && (adapt->flags & AMDGV_FLAG_ENABLE_COMPUTE_PAGING)) {
						ring->aql_enable = (ring_id == adapt->gfx.compute_paging_queue_id) ? false : true;
					} else {
						ring->aql_enable = true;
					}
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
	for (i = 0; i < (int)(adapt->gfx.num_compute_rings) * num_xcc; i++)
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

	gfx_v9_4_3_set_funcs(adapt);

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
	int ret;

	ret = amdgv_gfx_rlc_enter_safe_mode(adapt, xcc_id);
	if (ret)
		return ret;

	gfx_v9_4_3_xcc_init_pg(adapt, xcc_id);
	ret = amdgv_gfx_rlc_exit_safe_mode(adapt, xcc_id);

	return 0;
}

static int gfx_v9_4_3_rlc_resume(struct amdgv_adapter *adapt)
{
	int i, ret;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		ret = gfx_v9_4_3_xcc_rlc_resume(adapt, i);
		if (ret)
			return ret;
	}

	return 0;
}

static int gfx_v9_4_3_xcc_set_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t data;
	int ret;

	data = RLC_SAFE_MODE__CMD_MASK;
	data |= (1 << RLC_SAFE_MODE__MESSAGE__SHIFT);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE, data);

	ret = amdgv_wait_for_register(
		adapt, SOC15_REG_OFFSET_NAME(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE),
		RLC_SAFE_MODE__CMD_MASK, 0, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG),
		AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_YIELD);

	if (ret) {
		return AMDGV_FAILURE;
	}

	return 0;
}

static int gfx_v9_4_3_xcc_unset_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t data;

	data = RLC_SAFE_MODE__CMD_MASK;
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE, data);

	return 0;
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

	amdgv_misc_hdp_flush(adapt);
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

	if (!mqd_alloc)
		return AMDGV_FAILURE;


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
	if (ring->aql_enable)
		mqd->cp_hqd_aql_control = 1 << CP_HQD_AQL_CONTROL__CONTROL0__SHIFT;

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
	uint32_t i;

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
	int r = 0;
	uint32_t i;

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

int gfx_v9_4_3_aql_queue_init(struct amdgv_adapter *adapt,
				     struct gfx_v9_4_3_aql_queue *aq, uint32_t num_xcc)
{
	uint32_t ring_size = AQL_QUEUE_RING_DWORDS * sizeof(uint32_t);
	uint32_t mqd_stride_size = sizeof(struct v9_mqd_allocation);
	uint64_t mqd_base_gpu;
	uint8_t *mqd_base_cpu;
	uint32_t i;

	oss_memset(aq, 0, sizeof(*aq));
	if (num_xcc == 0 || num_xcc > AMDGV_MAX_GC_INSTANCES)
		return AMDGV_FAILURE;
	aq->num_xcc = num_xcc;

	aq->ring_buf = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, ring_size, PAGE_SIZE,
						     MEM_GFX_IB);
	if (!aq->ring_buf)
		goto fail_ring_buf;
	aq->ring_buf_gpu = amdgv_memmgr_get_gpu_addr(aq->ring_buf);
	aq->ring_buf_cpu = (volatile uint32_t *)amdgv_memmgr_get_cpu_addr(aq->ring_buf);

	aq->eop_obj = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, GFX9_MEC_HPD_SIZE,
						    PAGE_SIZE, MEM_GFX_IB);
	if (!aq->eop_obj)
		goto fail_eop;
	aq->eop_gpu = amdgv_memmgr_get_gpu_addr(aq->eop_obj);

	aq->wb_obj = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, 64, 64, MEM_GFX_IB);
	if (!aq->wb_obj)
		goto fail_wb;
	aq->wptr_gpu = amdgv_memmgr_get_gpu_addr(aq->wb_obj);
	aq->wptr_cpu = (volatile uint64_t *)amdgv_memmgr_get_cpu_addr(aq->wb_obj);
	aq->rptr_gpu = aq->wptr_gpu + sizeof(uint64_t);
	aq->rptr_cpu = (volatile uint32_t *)((uint8_t *)amdgv_memmgr_get_cpu_addr(aq->wb_obj) +
					     sizeof(uint64_t));

	aq->mqd_obj = amdgv_memmgr_alloc_align_zero(
		&adapt->memmgr_pf, (uint64_t)num_xcc * mqd_stride_size, PAGE_SIZE, MEM_GFX_IB);
	if (!aq->mqd_obj)
		goto fail_mqd;
	mqd_base_gpu = amdgv_memmgr_get_gpu_addr(aq->mqd_obj);
	mqd_base_cpu = (uint8_t *)amdgv_memmgr_get_cpu_addr(aq->mqd_obj);

	aq->hqd_save = (uint32_t *)oss_zalloc((uint64_t)num_xcc * MI300_CP_HQD_SAVE_REGS_NUM *
					      sizeof(uint32_t));
	if (!aq->hqd_save)
		goto fail_hqd_save;

	for (i = 0; i < num_xcc; i++) {
		aq->mqd_gpu[i] = mqd_base_gpu + (uint64_t)i * mqd_stride_size;
		aq->mqd_cpu[i] = mqd_base_cpu + (uint64_t)i * mqd_stride_size;
	}

	for (i = 0; i < num_xcc; i++) {
		aq->doorbell_index[i] = (adapt->doorbell_index.mec_ring0 +
					 i * adapt->doorbell_index.xcc_doorbell_range)
					<< 1;
	}

	for (i = 0; i < AQL_QUEUE_RING_DWORDS; i++) {
		aq->ring_buf_cpu[i] = (i % 16 == 0) ? AMDGV_AQL_INVALID_PACKET_HEADER :
						      AMDGV_AQL_INVALID_PACKET_DATA;
	}
	*aq->wptr_cpu = 0;
	*aq->rptr_cpu = 0;

	return 0;

fail_hqd_save:
	amdgv_memmgr_free(aq->mqd_obj);
fail_mqd:
	amdgv_memmgr_free(aq->wb_obj);
fail_wb:
	amdgv_memmgr_free(aq->eop_obj);
fail_eop:
	amdgv_memmgr_free(aq->ring_buf);
fail_ring_buf:
	oss_memset(aq, 0, sizeof(*aq));
	return AMDGV_FAILURE;
}

void gfx_v9_4_3_aql_queue_fini(struct gfx_v9_4_3_aql_queue *aq)
{
	if (aq->hqd_save)
		oss_free(aq->hqd_save);
	if (aq->mqd_obj)
		amdgv_memmgr_free(aq->mqd_obj);
	if (aq->wb_obj)
		amdgv_memmgr_free(aq->wb_obj);
	if (aq->eop_obj)
		amdgv_memmgr_free(aq->eop_obj);
	if (aq->ring_buf)
		amdgv_memmgr_free(aq->ring_buf);
	oss_memset(aq, 0, sizeof(*aq));
}

int gfx_v9_4_3_aql_queue_build_mqd(struct amdgv_adapter *adapt,
				     struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc)
{
	struct amdgv_ring tmp_ring;
	struct v9_mqd_allocation *mqd_alloc;
	struct v9_mqd *mqd;
	struct amdgv_ring *template_ring;
	uint32_t template_idx;
	int r;

	template_idx = xcc * adapt->gfx.num_compute_rings + XCC_QUEUE_INDEX__AQL;
	template_ring = &adapt->gfx.compute_ring[template_idx];

	oss_memset(&tmp_ring, 0, sizeof(tmp_ring));
	tmp_ring.adapt = adapt;
	tmp_ring.funcs = template_ring->funcs;
	tmp_ring.xcc_id = xcc;
	tmp_ring.me = template_ring->me;
	tmp_ring.pipe = template_ring->pipe;
	tmp_ring.queue = template_ring->queue;
	tmp_ring.aql_enable = true;
	tmp_ring.use_doorbell = true;
	tmp_ring.doorbell_index = aq->doorbell_index[xcc];
	tmp_ring.gpu_addr = aq->ring_buf_gpu;
	tmp_ring.ring = (volatile uint32_t *)aq->ring_buf_cpu;
	tmp_ring.ring_size = AQL_QUEUE_RING_DWORDS;
	tmp_ring.log2_ring_size = AQL_QUEUE_RING_LOG2;
	tmp_ring.buf_mask = AQL_QUEUE_RING_DWORDS - 1;
	tmp_ring.ptr_mask = (tmp_ring.funcs && tmp_ring.funcs->support_64bit_ptrs) ?
				    0xffffffffffffffffULL :
				    tmp_ring.buf_mask;
	tmp_ring.eop_gpu_addr = aq->eop_gpu;
	tmp_ring.mqd_obj = aq->mqd_obj;
	tmp_ring.mqd_gpu_addr = aq->mqd_gpu[xcc];
	tmp_ring.mqd_ptr = aq->mqd_cpu[xcc];
	tmp_ring.wptr_gpu_addr = aq->wptr_gpu;
	tmp_ring.wptr_cpu_addr = (volatile uint32_t *)aq->wptr_cpu;
	tmp_ring.rptr_gpu_addr = aq->rptr_gpu;
	tmp_ring.rptr_cpu_addr = (volatile uint32_t *)aq->rptr_cpu;

	mqd_alloc = (struct v9_mqd_allocation *)aq->mqd_cpu[xcc];
	oss_memset(mqd_alloc, 0, sizeof(*mqd_alloc));

	oss_mutex_lock(adapt->srbm_mutex);
	soc15_grbm_select(adapt, tmp_ring.me, tmp_ring.pipe, tmp_ring.queue, 0, (int)xcc);
	r = gfx_v9_4_3_xcc_mqd_init(&tmp_ring, (int)xcc);
	soc15_grbm_select(adapt, 0, 0, 0, 0, (int)xcc);
	oss_mutex_unlock(adapt->srbm_mutex);
	if (r)
		return r;

	mqd = &mqd_alloc->mqd;

	mqd->cp_hqd_pq_control |= CP_HQD_PQ_CONTROL__NO_UPDATE_RPTR_MASK |
				  (2 << CP_HQD_PQ_CONTROL__SLOT_BASED_WPTR__SHIFT) |
				  1 << CP_HQD_PQ_CONTROL__QUEUE_FULL_EN__SHIFT |
				  1 << CP_HQD_PQ_CONTROL__WPP_CLAMP_EN__SHIFT;

	mqd->cp_hqd_pq_doorbell_control |= CP_HQD_PQ_DOORBELL_CONTROL__DOORBELL_BIF_DROP_MASK |
					   CP_HQD_PQ_DOORBELL_CONTROL__DOORBELL_MODE_MASK;

	mqd->compute_tg_chunk_size = 1;
	mqd->compute_current_logic_xcc_id = xcc;

	mqd_alloc->mqd.cp_mqd_stride_size = sizeof(struct v9_mqd_allocation);

	if (xcc == 0) {
		mqd->cp_hqd_pq_control &= ~CP_HQD_PQ_CONTROL__NO_UPDATE_RPTR_MASK;
	}

	return 0;
}

int gfx_v9_4_3_aql_queue_kiq_map_xcc(struct amdgv_adapter *adapt,
				       struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc)
{
	struct amdgv_kiq *kiq = &adapt->gfx.kiq[xcc];
	struct amdgv_ring *kiq_ring = &kiq->ring;
	struct amdgv_ring temp_kcq;
	struct amdgv_ring *src_ring;
	uint32_t src_idx;

	if (!kiq->pmf || !kiq->pmf->kiq_map_queues)
		return AMDGV_FAILURE;

	src_idx = xcc * adapt->gfx.num_compute_rings + XCC_QUEUE_INDEX__AQL;
	src_ring = &adapt->gfx.compute_ring[src_idx];

	oss_memset(&temp_kcq, 0, sizeof(temp_kcq));
	temp_kcq.funcs = src_ring->funcs;
	temp_kcq.me = src_ring->me;
	temp_kcq.pipe = src_ring->pipe;
	temp_kcq.queue = src_ring->queue;
	temp_kcq.xcc_id = xcc;
	temp_kcq.doorbell_index = aq->doorbell_index[xcc];
	temp_kcq.mqd_gpu_addr = aq->mqd_gpu[xcc];
	temp_kcq.wptr_gpu_addr = aq->wptr_gpu;

	oss_spin_lock(kiq->ring_lock);
	if (amdgv_ring_alloc(kiq_ring, kiq->pmf->map_queues_size)) {
		oss_spin_unlock(kiq->ring_lock);
		return AMDGV_FAILURE;
	}
	kiq->pmf->kiq_map_queues(kiq_ring, &temp_kcq);
	amdgv_ring_commit(kiq_ring);
	oss_spin_unlock(kiq->ring_lock);

	aq->mapped[xcc] = true;
	return 0;
}

int gfx_v9_4_3_aql_queue_kiq_unmap_xcc(struct amdgv_adapter *adapt,
					 struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc)
{
	struct amdgv_kiq *kiq = &adapt->gfx.kiq[xcc];
	struct amdgv_ring *kiq_ring = &kiq->ring;
	struct amdgv_ring temp_kcq;
	struct amdgv_ring *src_ring;
	uint32_t src_idx;

	if (!aq->mapped[xcc])
		return 0;

	if (!kiq->pmf || !kiq->pmf->kiq_unmap_queues)
		return AMDGV_FAILURE;

	src_idx = xcc * adapt->gfx.num_compute_rings + XCC_QUEUE_INDEX__AQL;
	src_ring = &adapt->gfx.compute_ring[src_idx];

	oss_memset(&temp_kcq, 0, sizeof(temp_kcq));
	temp_kcq.funcs = src_ring->funcs;
	temp_kcq.me = src_ring->me;
	temp_kcq.pipe = src_ring->pipe;
	temp_kcq.queue = src_ring->queue;
	temp_kcq.xcc_id = xcc;
	temp_kcq.doorbell_index = aq->doorbell_index[xcc];
	temp_kcq.mqd_gpu_addr = aq->mqd_gpu[xcc];
	temp_kcq.wptr_gpu_addr = aq->wptr_gpu;

	oss_spin_lock(kiq->ring_lock);
	if (amdgv_ring_alloc(kiq_ring, kiq->pmf->unmap_queues_size)) {
		oss_spin_unlock(kiq->ring_lock);
		return AMDGV_FAILURE;
	}
	kiq->pmf->kiq_unmap_queues(kiq_ring, &temp_kcq, RESET_QUEUES, 0, 0);
	amdgv_ring_commit(kiq_ring);
	oss_spin_unlock(kiq->ring_lock);

	aq->mapped[xcc] = false;
	return 0;
}

void gfx_v9_4_3_aql_queue_save_hqd(struct amdgv_adapter *adapt,
				   struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc)
{
	struct amdgv_ring *src_ring;
	uint32_t *save;
	uint32_t src_idx;
	uint32_t i;

	src_idx = xcc * adapt->gfx.num_compute_rings + XCC_QUEUE_INDEX__AQL;
	src_ring = &adapt->gfx.compute_ring[src_idx];
	save = aq->hqd_save + (uint64_t)xcc * MI300_CP_HQD_SAVE_REGS_NUM;

	oss_mutex_lock(adapt->srbm_mutex);
	soc15_grbm_select(adapt, src_ring->me, src_ring->pipe, src_ring->queue, 0, (int)xcc);

	for (i = 0; i < MI300_CP_HQD_SAVE_REGS_NUM; i++)
		save[i] = RREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc), regCP_MQD_BASE_ADDR, i);

	soc15_grbm_select(adapt, 0, 0, 0, 0, (int)xcc);
	oss_mutex_unlock(adapt->srbm_mutex);
}

void gfx_v9_4_3_aql_queue_restore_hqd(struct amdgv_adapter *adapt,
				      struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc)
{
	struct amdgv_ring *src_ring;
	const uint32_t *save;
	uint32_t src_idx;
	uint32_t i;

	src_idx = xcc * adapt->gfx.num_compute_rings + XCC_QUEUE_INDEX__AQL;
	src_ring = &adapt->gfx.compute_ring[src_idx];
	save = aq->hqd_save + (uint64_t)xcc * MI300_CP_HQD_SAVE_REGS_NUM;

	oss_mutex_lock(adapt->srbm_mutex);
	soc15_grbm_select(adapt, src_ring->me, src_ring->pipe, src_ring->queue, 0, (int)xcc);

	for (i = 0; i < MI300_CP_HQD_SAVE_REGS_NUM; i++)
		WREG32_SOC15_OFFSET(GC, GET_INST(GC, xcc), regCP_MQD_BASE_ADDR, i, save[i]);

	RREG32_SOC15(GC, GET_INST(GC, xcc), regCP_HQD_PQ_CONTROL);

	soc15_grbm_select(adapt, 0, 0, 0, 0, (int)xcc);
	oss_mutex_unlock(adapt->srbm_mutex);
}

int gfx_v9_4_3_aql_queue_submit_packet_data(struct amdgv_adapter *adapt,
					    struct gfx_v9_4_3_aql_queue *aq,
					    const uint32_t *pkt_data)
{
	const uint32_t pkt_dwords = sizeof(hsa_kernel_dispatch_packet_t) / sizeof(uint32_t);
	const uint32_t ring_slots = AQL_QUEUE_RING_DWORDS / 16;
	const uint64_t full_timeout_us = 1000 * 1000; /* 1s */
	uint64_t wptr_old, wptr_new, start, slot_idx;
	uint32_t *queue_slot;
	uint32_t i;

	if (pkt_dwords > AQL_QUEUE_RING_DWORDS)
		return AMDGV_FAILURE;

	wptr_old = *aq->wptr_cpu;
	wptr_new = wptr_old + 1;
	slot_idx = wptr_old % ring_slots;
	queue_slot =
		(uint32_t *)&aq->ring_buf_cpu[slot_idx * pkt_dwords];

	start = oss_get_time_stamp();
	while ((wptr_old - (uint64_t)*aq->rptr_cpu) >= ring_slots) {
		if (oss_get_time_stamp() - start > full_timeout_us) {
			AMDGV_WARN("AQL: ring full (wptr=%llu rptr=%llu)\n", wptr_old,
				   (uint64_t)*aq->rptr_cpu);
			return AMDGV_FAILURE;
		}
		oss_udelay(1);
	}

	queue_slot[0] = AMDGV_AQL_INVALID_PACKET_HEADER;
	oss_mb();
	*aq->wptr_cpu = wptr_new;
	oss_mb();
	for (i = 1; i < pkt_dwords; i++)
		queue_slot[i] = pkt_data[i];
	oss_mb();
	queue_slot[0] = pkt_data[0];
	oss_mb();
	amdgv_misc_hdp_flush(adapt);

	for (i = 0; i < aq->num_xcc; i++) {
		if (!aq->mapped[i])
			continue;
		WDOORBELL32(aq->doorbell_index[i], (uint32_t)wptr_new);
	}

	return 0;
}

struct gfx_v9_4_3_fb_hash_resources {
	struct amdgv_memmgr_mem *kernelobj;
	struct amdgv_memmgr_mem *kernarg;
	struct amdgv_memmgr_mem *signal;
	struct amdgv_memmgr_mem *packet;
	struct amdgv_memmgr_mem *done_counter;
	struct amdgv_memmgr_mem *block_counter;
};

static void gfx_v9_4_3_fb_hash_free_resources(struct gfx_v9_4_3_fb_hash_resources *res)
{
	if (res->packet) {
		amdgv_memmgr_free(res->packet);
		res->packet = NULL;
	}
	if (res->signal) {
		amdgv_memmgr_free(res->signal);
		res->signal = NULL;
	}
	if (res->done_counter) {
		amdgv_memmgr_free(res->done_counter);
		res->done_counter = NULL;
	}
	if (res->block_counter) {
		amdgv_memmgr_free(res->block_counter);
		res->block_counter = NULL;
	}
	if (res->kernarg) {
		amdgv_memmgr_free(res->kernarg);
		res->kernarg = NULL;
	}
	if (res->kernelobj) {
		amdgv_memmgr_free(res->kernelobj);
		res->kernelobj = NULL;
	}
}

static int gfx_v9_4_3_fb_hash_alloc_resources(struct amdgv_adapter *adapt,
				       struct gfx_v9_4_3_fb_hash_resources *res,
				       uint64_t kernelobj_size, uint32_t kernarg_bytes)
{
	res->kernelobj = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf,
				kernelobj_size, PAGE_SIZE,
				MEM_GFX_IB);
	if (!res->kernelobj)
		goto fail;

	res->kernarg = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf,
				kernarg_bytes, 256,
				MEM_GFX_IB);
	if (!res->kernarg)
		goto fail;

	res->signal = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf,
				256, 256, MEM_GFX_IB);
	if (!res->signal)
		goto fail;

	/* done_counter / block_counter back the shader-side last-block
	 * completion handshake. Both must be zeroed before each launch.
	 */
	res->done_counter = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf,
				256, 256, MEM_GFX_IB);
	if (!res->done_counter)
		goto fail;

	res->block_counter = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf,
				256, 256, MEM_GFX_IB);
	if (!res->block_counter)
		goto fail;

	res->packet = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf,
				sizeof(hsa_kernel_dispatch_packet_t),
				256, MEM_GFX_IB);
	if (!res->packet)
		goto fail;

	return 0;
fail:
	gfx_v9_4_3_fb_hash_free_resources(res);
	return AMDGV_FAILURE;
}

static void gfx_v9_4_3_fb_hash_build_kernarg_sha256(uint8_t *kernarg_cpu,
					     uint64_t page_base_gpua,
					     uint64_t page_size, uint64_t num_pages,
					     uint64_t page_hash_gpua,
					     uint64_t done_counter_gpua,
					     uint64_t block_counter_gpua, uint32_t num_blocks,
					     uint32_t workgroup_x)
{
	oss_memset(kernarg_cpu, 0, SHA256_KERNARG_BYTES);

	*(uint64_t *)(kernarg_cpu + 0x00) = page_base_gpua;
	*(uint64_t *)(kernarg_cpu + 0x08) = page_size;
	*(uint64_t *)(kernarg_cpu + 0x10) = num_pages;
	*(uint64_t *)(kernarg_cpu + 0x18) = page_hash_gpua;
	*(uint64_t *)(kernarg_cpu + 0x20) = done_counter_gpua;
	*(uint64_t *)(kernarg_cpu + 0x28) = block_counter_gpua;

	*(uint32_t *)(kernarg_cpu + 0x30) = num_blocks;
	*(uint32_t *)(kernarg_cpu + 0x34) = 1;
	*(uint32_t *)(kernarg_cpu + 0x38) = 1;
	*(uint16_t *)(kernarg_cpu + 0x3c) = (uint16_t)workgroup_x;
	*(uint16_t *)(kernarg_cpu + 0x3e) = 1;
	*(uint16_t *)(kernarg_cpu + 0x40) = 1;
}

static void gfx_v9_4_3_fb_hash_build_kernarg_rapidhash(uint8_t *kernarg_cpu,
					     uint64_t page_base_gpua,
					     uint64_t page_size, uint64_t per_thread_bytes,
					     uint64_t seed, uint64_t page_hash_gpua,
					     uint64_t done_counter_gpua,
					     uint64_t block_counter_gpua, uint32_t page_count,
					     uint32_t workgroup_x)
{
	oss_memset(kernarg_cpu, 0, RAPIDHASH_KERNARG_BYTES);

	*(uint64_t *)(kernarg_cpu + 0x00) = page_base_gpua;
	*(uint64_t *)(kernarg_cpu + 0x08) = page_size;
	*(uint64_t *)(kernarg_cpu + 0x10) = per_thread_bytes;
	*(uint64_t *)(kernarg_cpu + 0x18) = seed;
	*(uint64_t *)(kernarg_cpu + 0x20) = page_hash_gpua;
	*(uint64_t *)(kernarg_cpu + 0x28) = done_counter_gpua;
	*(uint64_t *)(kernarg_cpu + 0x30) = block_counter_gpua;

	*(uint32_t *)(kernarg_cpu + 0x38) = page_count;
	*(uint32_t *)(kernarg_cpu + 0x3c) = 1;
	*(uint32_t *)(kernarg_cpu + 0x40) = 1;
	*(uint16_t *)(kernarg_cpu + 0x44) = (uint16_t)workgroup_x;
	*(uint16_t *)(kernarg_cpu + 0x46) = 1;
	*(uint16_t *)(kernarg_cpu + 0x48) = 1;
}

static void gfx_v9_4_3_fb_hash_build_packet(hsa_kernel_dispatch_packet_t *pkt, uint64_t kd_gpua,
					    uint64_t kernarg_gpua, uint64_t signal_gpua,
					    uint32_t num_blocks, uint32_t workgroup_x,
					    uint32_t lds_bytes)
{
	oss_memset(pkt, 0, sizeof(*pkt));

	pkt->header = (uint16_t)(HSA_PACKET_TYPE_KERNEL_DISPATCH << HSA_PACKET_HEADER_TYPE);
	pkt->header |=
		(uint16_t)(HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE);
	pkt->header |=
		(uint16_t)(HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE);
	pkt->setup = 1u << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;

	pkt->workgroup_size_x = (uint16_t)workgroup_x;
	pkt->workgroup_size_y = 1;
	pkt->workgroup_size_z = 1;
	pkt->grid_size_x = (uint32_t)num_blocks * workgroup_x;
	pkt->grid_size_y = 1;
	pkt->grid_size_z = 1;

	pkt->private_segment_size = 0;
	pkt->group_segment_size = lds_bytes; /* dynamic LDS */
	pkt->kernel_object = kd_gpua;
	pkt->kernarg_address = (void *)(unsigned long)kernarg_gpua;
	pkt->completion_signal.handle = signal_gpua;
}

static int fb_hash_done_cb(void *ctx)
{
	volatile uint64_t *done_cpu = ctx;
	return *done_cpu ? 0 : 1;
}


static int gfx_v9_4_3_fb_hash_dispatch_internal(struct amdgv_adapter *adapt,
						uint64_t page_bytes,
						uint64_t page_base_gpua, uint32_t page_count,
						struct amdgv_memmgr_mem *page_hash,
						uint64_t page_hash_byte_offset)
{
	struct gfx_v9_4_3_fb_hash_resources res;
	struct gfx_v9_4_3_aql_queue aq;
	uint64_t kernelobj_size;
	uint64_t kernarg_gpua, page_hash_gpua, signal_gpua;
	uint64_t done_counter_gpua, block_counter_gpua, kd_gpua;
	uint8_t *kernarg_cpua;
	hsa_kernel_dispatch_packet_t *pkt_cpua;
	uint32_t num_xcc, lds_bytes, xcc, num_workgroups;
	const uint32_t *fb_hash_shader;
	uint32_t fb_hash_shader_size;
	const kernel_descriptor_t *fb_hash_kd;
	uint32_t kernarg_bytes;
	enum amdgv_fb_hash_mode mode = adapt->dirtybit.fb_hash_mode;
	uint8_t *kobj_cpua;
	volatile int64_t *signal_cpu;
	volatile uint64_t *done_cpu;
	volatile uint32_t *block_cpu;
	struct amdgv_wait_for_cb_context cb_context = { 0 };
	int ret;

	oss_memset(&res, 0, sizeof(res));
	oss_memset(&aq, 0, sizeof(aq));

	num_xcc = adapt->mcp.gfx.num_xcc ? adapt->mcp.gfx.num_xcc : 1;

	if (mode == AMDGV_FB_HASH_MODE_RAPIDHASH) {
		fb_hash_kd = &rapidhash_kd;
		kernarg_bytes = RAPIDHASH_KERNARG_BYTES;
		switch (adapt->asic_type) {
		case CHIP_MI350X:
			fb_hash_shader = mi350_rapidhash_shader;
			fb_hash_shader_size = sizeof(mi350_rapidhash_shader);
			break;
		case CHIP_MI308X:
			fb_hash_shader = mi308_rapidhash_shader;
			fb_hash_shader_size = sizeof(mi308_rapidhash_shader);
			break;
		default:
			AMDGV_WARN("fb_hash: no shader for asic_type=%d\n", adapt->asic_type);
			return AMDGV_FAILURE;
		}
	} else {
		fb_hash_kd = &sha256_kd;
		kernarg_bytes = SHA256_KERNARG_BYTES;
		switch (adapt->asic_type) {
		case CHIP_MI350X:
			fb_hash_shader = mi350_sha256_shader;
			fb_hash_shader_size = sizeof(mi350_sha256_shader);
			break;
		case CHIP_MI308X:
			fb_hash_shader = mi308_sha256_shader;
			fb_hash_shader_size = sizeof(mi308_sha256_shader);
			break;
		default:
			AMDGV_WARN("fb_hash: no shader for asic_type=%d\n", adapt->asic_type);
			return AMDGV_FAILURE;
		}
	}

	kernelobj_size = (FB_HASH_CODE_ENTRY_OFFSET + fb_hash_shader_size + PAGE_SIZE - 1) &
			 ~(PAGE_SIZE - 1);
	ret = gfx_v9_4_3_fb_hash_alloc_resources(adapt, &res, kernelobj_size, kernarg_bytes);
	if (ret) {
		AMDGV_ERROR("fb_hash: failed to allocate resources\n");
		return ret;
	}

	kobj_cpua = (uint8_t *)amdgv_memmgr_get_cpu_addr(res.kernelobj);
	oss_memcpy(kobj_cpua, fb_hash_kd, sizeof(*fb_hash_kd));
	((kernel_descriptor_t *)kobj_cpua)->kernel_code_entry_byte_offset =
		FB_HASH_CODE_ENTRY_OFFSET;
	oss_memcpy(kobj_cpua + FB_HASH_CODE_ENTRY_OFFSET, fb_hash_shader, fb_hash_shader_size);

	kd_gpua = amdgv_memmgr_get_gpu_addr(res.kernelobj);
	kernarg_gpua = amdgv_memmgr_get_gpu_addr(res.kernarg);
	page_hash_gpua = amdgv_memmgr_get_gpu_addr(page_hash) + page_hash_byte_offset;
	signal_gpua = amdgv_memmgr_get_gpu_addr(res.signal);
	done_counter_gpua = amdgv_memmgr_get_gpu_addr(res.done_counter);
	block_counter_gpua = amdgv_memmgr_get_gpu_addr(res.block_counter);

	kernarg_cpua = (uint8_t *)amdgv_memmgr_get_cpu_addr(res.kernarg);
	if (mode == AMDGV_FB_HASH_MODE_RAPIDHASH) {
		num_workgroups = page_count;
		lds_bytes = FB_HASH_WORKGROUP_X * (uint32_t)sizeof(uint64_t);
		gfx_v9_4_3_fb_hash_build_kernarg_rapidhash(kernarg_cpua, page_base_gpua,
					page_bytes, page_bytes / FB_HASH_WORKGROUP_X,
					RAPIDHASH_SEED, page_hash_gpua, done_counter_gpua,
					block_counter_gpua, page_count, FB_HASH_WORKGROUP_X);
	} else {
		num_workgroups = DIV_ROUND_UP(page_count, FB_HASH_WORKGROUP_X);
		lds_bytes = 0;
		gfx_v9_4_3_fb_hash_build_kernarg_sha256(kernarg_cpua, page_base_gpua, page_bytes,
					page_count, page_hash_gpua, done_counter_gpua,
					block_counter_gpua, num_workgroups, FB_HASH_WORKGROUP_X);
	}

	pkt_cpua = (hsa_kernel_dispatch_packet_t *)amdgv_memmgr_get_cpu_addr(res.packet);

	gfx_v9_4_3_fb_hash_build_packet(pkt_cpua, kd_gpua, kernarg_gpua, signal_gpua,
					num_workgroups, FB_HASH_WORKGROUP_X, lds_bytes);

	ret = gfx_v9_4_3_aql_queue_init(adapt, &aq, num_xcc);
	if (ret) {
		AMDGV_ERROR("fb_hash: failed to init AQL queue\n");
		goto err_aql_queue_init;
	}

	for (xcc = 0; xcc < num_xcc; xcc++)
		gfx_v9_4_3_aql_queue_save_hqd(adapt, &aq, xcc);

	for (xcc = 0; xcc < num_xcc; xcc++) {
		ret = gfx_v9_4_3_aql_queue_build_mqd(adapt, &aq, xcc);
		if (ret) {
			AMDGV_WARN("fb_hash: failed to build mqd for xcc %u ret=%d\n", xcc,
				   ret);
			goto err_aql_restore_hqd;
		}
	}
	for (xcc = 0; xcc < num_xcc; xcc++) {
		ret = gfx_v9_4_3_aql_queue_kiq_map_xcc(adapt, &aq, xcc);
		if (ret) {
			AMDGV_WARN("fb_hash: failed to map kiq for xcc %u ret=%d\n", xcc, ret);
			goto err_aql_map_xcc;
		}
	}

	signal_cpu = (volatile int64_t *)amdgv_memmgr_get_cpu_addr(res.signal);
	done_cpu = (volatile uint64_t *)amdgv_memmgr_get_cpu_addr(res.done_counter);
	block_cpu = (volatile uint32_t *)amdgv_memmgr_get_cpu_addr(res.block_counter);
	*signal_cpu = 0;
	*done_cpu = 0;
	*block_cpu = 0;
	oss_mb();
	amdgv_misc_hdp_flush(adapt);
	oss_mb();

	ret = gfx_v9_4_3_aql_queue_submit_packet_data(adapt, &aq, (const uint32_t *)pkt_cpua);
	if (ret) {
		AMDGV_WARN("fb_hash: failed to submit packet data\n");
		goto err_aql_submit;
	}

	cb_context.ctx = (void *)done_cpu;
	cb_context.type = AMDGV_WAIT_FOR_FB_HASH_DONE;
	ret = amdgv_wait_for(adapt, fb_hash_done_cb, &cb_context, FB_HASH_TIMEOUT_US, 0);
	if (ret) {
		AMDGV_WARN("fb_hash: wait for done failed ret=%d\n", ret);
		goto err_aql_submit;
	}
	oss_mb();
	amdgv_misc_hdp_flush(adapt);
	oss_mb();

err_aql_submit:
err_aql_map_xcc:
	for (xcc = 0; xcc < num_xcc; xcc++) {
		if (aq.mapped[xcc])
			gfx_v9_4_3_aql_queue_kiq_unmap_xcc(adapt, &aq, xcc);
	}
err_aql_restore_hqd:
	for (xcc = 0; xcc < num_xcc; xcc++)
		gfx_v9_4_3_aql_queue_restore_hqd(adapt, &aq, xcc);
	gfx_v9_4_3_aql_queue_fini(&aq);
err_aql_queue_init:
	gfx_v9_4_3_fb_hash_free_resources(&res);
	return ret;
}

int gfx_v9_4_3_fb_hash_compute_page_hash(struct amdgv_adapter *adapt, uint32_t idx_vf,
					 uint64_t page_size,
					 struct amdgv_memmgr_mem *fb_hash_buf)
{
	struct amdgv_vf_device *vf;
	uint64_t vf_fb_size_bytes;
	uint64_t page_base_gpua;
	uint64_t needed_bytes;
	uint64_t vf_start_page;
	uint64_t page_hash_byte_offset;
	uint32_t page_count;

	if (adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE ||
	    adapt->flags & AMDGV_FLAG_ENABLE_COMPUTE_PAGING) {
		AMDGV_ERROR("Compute engine disabled or compute paging on\n");
		return AMDGV_FAILURE;
	}

	vf = &adapt->array_vf[idx_vf];
	if (!vf->configured) {
		AMDGV_WARN("vf%u not configured\n", idx_vf);
		return AMDGV_FAILURE;
	}

	vf_fb_size_bytes = MBYTES_TO_BYTES(vf->fb_size);
	page_count = (uint32_t)(vf_fb_size_bytes / page_size);
	if (page_count == 0) {
		AMDGV_WARN("fb_hash_compute_page_hash: page_count=0 (fb=0x%llx page_size=0x%llx)\n",
			   (unsigned long long)vf_fb_size_bytes,
			   (unsigned long long)page_size);
		return AMDGV_FAILURE;
	}

	vf_start_page = MBYTES_TO_BYTES(vf->fb_offset) / page_size;
	page_hash_byte_offset = vf_start_page * adapt->dirtybit.fb_hash_digest_bytes;
	needed_bytes = (vf_start_page + page_count) * adapt->dirtybit.fb_hash_digest_bytes;
	if (amdgv_memmgr_get_size(fb_hash_buf) < needed_bytes) {
		AMDGV_WARN(
			"fb_hash_buf size 0x%llx < needed 0x%llx\n",
			(unsigned long long)amdgv_memmgr_get_size(fb_hash_buf),
			(unsigned long long)needed_bytes);
		return AMDGV_FAILURE;
	}

	page_base_gpua = adapt->memmgr_pf.mc_base + MBYTES_TO_BYTES(vf->fb_offset);

	return gfx_v9_4_3_fb_hash_dispatch_internal(adapt, page_size, page_base_gpua,
						    page_count, fb_hash_buf, page_hash_byte_offset);
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
	.vmhub = VM_GFXHUB,
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
	uint32_t i, j, num_xcc;
	num_xcc = adapt->mcp.gfx.num_xcc;
	for (i = 0; i < num_xcc; i++) {
		adapt->gfx.kiq[i].ring.funcs = &gfx_v9_4_3_ring_funcs_kiq;

		for (j = 0; j < adapt->gfx.num_compute_rings; j++)
			adapt->gfx.compute_ring[j + i * adapt->gfx.num_compute_rings].funcs
					= &gfx_v9_4_3_ring_funcs_compute;
	}
}

static int
gfx_v9_4_3_alloc_dump_cu_resource_memory(struct amdgv_adapter *adapt,
					 struct amdgv_dump_cu_resource_size *resource_size,
					 struct amdgv_dump_cu_resource_memory *resource_mem)
{
	int ret = 0;
	struct amdgv_memmgr_mem *extra_kernelarg, *extra_kernelobj, *extra_packet,
		*extra_signal_obj, *sync_signal_obj, *dev_data;
	uint64_t *kernelarg_cpua, *extra_kernelarg_cpua;
	hsa_signal_t extra_signal;
	hsa_kernel_dispatch_packet_t *packet0, *packet1;
	int num_xcc = 0;
	int cu_count_per_xcc = 0;
	num_xcc = adapt->mcp.gfx.num_xcc ? adapt->mcp.gfx.num_xcc : 1;
	cu_count_per_xcc = adapt->config.gfx.active_cu_count / num_xcc;

	if (adapt->gfx.cu_dump_data_info.cu_dump_type == AMDGV_CU_DATA_TYPE__SGPRs &&
	    !adapt->gfx.cu_dump_data_info.use_extra_ring) {
		AMDGV_ERROR("Dumping SGPRs for GFX9 without extra ring is not supported\n");
		return AMDGV_FAILURE;
	}

	ret = amdgv_gfx_alloc_dump_cu_resource_memory(adapt, resource_size, resource_mem);
	if (ret)
		return ret;

	if (adapt->gfx.cu_dump_data_info.use_extra_ring) {
		extra_kernelarg = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, 256, 256, MEM_GFX_IB);
		if (!extra_kernelarg) {
			AMDGV_WARN("failed to create extra_kernelarg.\n");
			goto free_resource_memory;
		}
		extra_kernelobj = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, resource_size->extra_kernelobj_size, 256, MEM_GFX_IB);
		if (!extra_kernelobj) {
			AMDGV_WARN("failed to create extra_kernelobj.\n");
			goto free_extra_kernelarg;
		}
		extra_signal_obj = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, 256, 256, MEM_GFX_IB);
		if (!extra_signal_obj) {
			AMDGV_WARN("failed to create extra_signal_obj.\n");
			goto free_extra_kernelobj;
		}
		extra_packet = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, sizeof(hsa_kernel_dispatch_packet_t), 256, MEM_GFX_IB);
		if (!extra_packet) {
			AMDGV_WARN("failed to create extra_packet.\n");
			goto free_extra_signal_obj;
		}
		sync_signal_obj = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, 256, 256, MEM_GFX_IB);
		if (!sync_signal_obj) {
			AMDGV_WARN("failed to create sync_signal_obj.\n");
			goto free_extra_packet;
		}
		dev_data = amdgv_memmgr_alloc_align_zero(&adapt->memmgr_pf, 4096, 256, MEM_GFX_IB);
		if (!dev_data) {
			AMDGV_WARN("failed to create device data.\n");
			goto free_sync_signal_obj;
		}

		resource_mem->extra_kernelobj_addr = (uint32_t *)amdgv_memmgr_get_cpu_addr(extra_kernelobj);

		extra_signal.handle = amdgv_memmgr_get_gpu_addr(extra_signal_obj);

		adapt->gfx.dump_cu_packets[1] = (hsa_kernel_dispatch_packet_t *)amdgv_memmgr_get_cpu_addr(
				extra_packet);
		packet0 = adapt->gfx.dump_cu_packets[0];
		packet1 = adapt->gfx.dump_cu_packets[1];

		amdgv_gfx_init_dump_cu_packet(adapt, packet1, resource_size, extra_signal,
					      extra_kernelobj, extra_kernelarg);

		adapt->gfx.dump_cu_memmgr_mem_group->extra_packet = extra_packet;
		adapt->gfx.dump_cu_memmgr_mem_group->extra_kernelobj = extra_kernelobj;
		adapt->gfx.dump_cu_memmgr_mem_group->extra_kernelarg = extra_kernelarg;
		adapt->gfx.dump_cu_memmgr_mem_group->extra_signal_obj = extra_signal_obj;
		adapt->gfx.dump_cu_memmgr_mem_group->sync_signal_obj = sync_signal_obj;
		adapt->gfx.dump_cu_memmgr_mem_group->dev_data = dev_data;

		kernelarg_cpua = amdgv_memmgr_get_cpu_addr(
			adapt->gfx.dump_cu_memmgr_mem_group->kernelarg);
		extra_kernelarg_cpua = amdgv_memmgr_get_cpu_addr(extra_kernelarg);
		oss_memset(kernelarg_cpua, 0, sizeof(uint64_t) * 6);

		kernelarg_cpua[0] = amdgv_memmgr_get_gpu_addr(dev_data);
		kernelarg_cpua[1] = cu_count_per_xcc;
		kernelarg_cpua[2] = amdgv_memmgr_get_gpu_addr(sync_signal_obj);
		kernelarg_cpua[3] = cu_count_per_xcc * 3;
		kernelarg_cpua[4] = amdgv_memmgr_get_gpu_addr(adapt->gfx.dump_cu_memmgr_mem_group->out_data);
		kernelarg_cpua[5] = amdgv_memmgr_get_gpu_addr(adapt->gfx.dump_cu_memmgr_mem_group->out_flag);
		oss_memcpy(extra_kernelarg_cpua, kernelarg_cpua, sizeof(uint64_t) * 6);

		packet0->workgroup_size_x = 64 * 8;
		packet0->grid_size_x = cu_count_per_xcc * packet0->workgroup_size_x;

		packet1->workgroup_size_x = 64 * 12;
		packet1->grid_size_x = 2 * cu_count_per_xcc * packet1->workgroup_size_x;
	} else {
		adapt->gfx.dump_cu_packets[0]->grid_size_x =
			cu_count_per_xcc * adapt->gfx.dump_cu_packets[0]->workgroup_size_x;
	}
	return 0;

free_sync_signal_obj:
	amdgv_memmgr_free(sync_signal_obj);
free_extra_packet:
	amdgv_memmgr_free(extra_packet);
free_extra_signal_obj:
	amdgv_memmgr_free(extra_signal_obj);
free_extra_kernelobj:
	amdgv_memmgr_free(extra_kernelobj);
free_extra_kernelarg:
	amdgv_memmgr_free(extra_kernelarg);
free_resource_memory:
	amdgv_gfx_free_dump_cu_resource_memory(adapt);
	return AMDGV_FAILURE;
}

static void gfx_v9_4_3_free_dump_cu_resource_memory(struct amdgv_adapter *adapt)
{
	struct amdgv_dump_cu_memmgr_mem_group *mem_group = adapt->gfx.dump_cu_memmgr_mem_group;
	if (!mem_group)
		return;

	if (adapt->gfx.cu_dump_data_info.use_extra_ring) {
		adapt->gfx.dump_cu_packets[1] = NULL;
		amdgv_memmgr_free(mem_group->dev_data);
		amdgv_memmgr_free(mem_group->sync_signal_obj);
		amdgv_memmgr_free(mem_group->extra_packet);
		amdgv_memmgr_free(mem_group->extra_signal_obj);
		amdgv_memmgr_free(mem_group->extra_kernelobj);
		amdgv_memmgr_free(mem_group->extra_kernelarg);
		adapt->gfx.cu_dump_data_info.use_extra_ring = false;
	}
	amdgv_gfx_free_dump_cu_resource_memory(adapt);
}

static int gfx_v9_4_3_dump_cu_data(struct amdgv_adapter *adapt)
{
	int i;
	int r = 0;
	struct amdgv_ring *mec_ring = NULL;
	struct amdgv_ring *extra_mec_ring = NULL;
	int xcc_id = adapt->gfx.cu_dump_data_info.xcc_id;
	int ring_idx = xcc_id * adapt->gfx.num_compute_rings + XCC_QUEUE_INDEX__AQL;
	int extra_ring_idx = ring_idx + 1;
	bool use_extra_ring = adapt->gfx.cu_dump_data_info.use_extra_ring;
	uint64_t *sync_signal_addr;
	AMDGV_DEBUG("Dumping CU data on XCC %d, ring %d%s.\n", xcc_id, ring_idx,
		   use_extra_ring ? " with extra ring" : "");
	if (adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE ||
	    adapt->flags & AMDGV_FLAG_ENABLE_COMPUTE_PAGING) {
		AMDGV_WARN("Mi3XX cannot dump CU data when compute engine is disabled.\n");
		return AMDGV_FAILURE;
	}

	r = amdgv_gfx_map_kcq(adapt, xcc_id, XCC_QUEUE_INDEX__AQL);
	if (r) {
		AMDGV_WARN("failed to map KCQ XCC %d, ring %d.\n", xcc_id, XCC_QUEUE_INDEX__AQL);
		return AMDGV_FAILURE;
	}

	mec_ring = &(adapt->gfx.compute_ring[ring_idx]);

	if (use_extra_ring) {
		r = amdgv_gfx_map_kcq(adapt, xcc_id, XCC_QUEUE_INDEX__AQL + 1);
		if (r) {
			AMDGV_WARN("failed to map KCQ XCC %d, ring %d.\n", xcc_id, XCC_QUEUE_INDEX__AQL + 1);
			r = AMDGV_FAILURE;
			goto unmap_ring;
		}
		extra_mec_ring = &(adapt->gfx.compute_ring[extra_ring_idx]);

		sync_signal_addr = (uint64_t *)amdgv_memmgr_get_cpu_addr(
			adapt->gfx.dump_cu_memmgr_mem_group->sync_signal_obj);
		sync_signal_addr[1] = 1;
	}

	if (amdgv_ring_alloc(mec_ring,
			     sizeof(hsa_kernel_dispatch_packet_t) / sizeof(uint32_t))) {
		AMDGV_WARN("failed to allocate ring.\n");
		goto unmap_extra_ring;
	}

	if (use_extra_ring) {
		if (amdgv_ring_alloc(extra_mec_ring, sizeof(hsa_kernel_dispatch_packet_t) /
							     sizeof(uint32_t))) {
			AMDGV_WARN("failed to allocate extra ring.\n");
			r = AMDGV_FAILURE;
			goto unmap_extra_ring;
		}
	}

	for (i = 0; i < sizeof(hsa_kernel_dispatch_packet_t) / sizeof(uint32_t); i++) {
		amdgv_ring_write(mec_ring, ((uint32_t *)adapt->gfx.dump_cu_packets[0])[i]);
	}
	oss_mb();
	amdgv_ring_commit(mec_ring);
	oss_msleep(200);

	if (use_extra_ring) {
		for (i = 0; i < sizeof(hsa_kernel_dispatch_packet_t) / sizeof(uint32_t); i++) {
			amdgv_ring_write(extra_mec_ring,
					 ((uint32_t *)adapt->gfx.dump_cu_packets[1])[i]);
		}
		amdgv_ring_commit(extra_mec_ring);
		oss_msleep(200);
	}
unmap_extra_ring:
	if (use_extra_ring) {
		amdgv_gfx_unmap_kcq(adapt, xcc_id, XCC_QUEUE_INDEX__AQL + 1);
	}
unmap_ring:
	amdgv_gfx_unmap_kcq(adapt, xcc_id, XCC_QUEUE_INDEX__AQL);
	return r;
}

static int gfx_v9_4_3_early_init(struct amdgv_adapter *adapt)
{
	adapt->gfx.num_gfx_rings = 0;
	adapt->gfx.num_compute_rings = AMDGV_MAX_COMPUTE_RINGS;

	gfx_v9_4_3_set_ring_funcs(adapt);
	gfx_v9_4_3_set_kiq_pm4_funcs(adapt);
	gfx_v9_4_3_set_rlc_funcs(adapt);

	adapt->gfx.funcs->dump_cu_data = gfx_v9_4_3_dump_cu_data;
	adapt->gfx.funcs->alloc_dump_cu_resource_memory = gfx_v9_4_3_alloc_dump_cu_resource_memory;
	adapt->gfx.funcs->free_dump_cu_resource_memory = gfx_v9_4_3_free_dump_cu_resource_memory;

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
	.is_engine = true,
	.sw_init = gfx_v9_4_3_sw_init,
	.sw_fini = gfx_v9_4_3_sw_fini,
	.hw_init = gfx_v9_4_3_hw_init,
	.hw_fini = gfx_v9_4_3_hw_fini,
};
