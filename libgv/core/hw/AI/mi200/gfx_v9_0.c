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
#include "amdgv_misc.h"
#include "amdgv_nbio.h"
#include "amdgv_gfx.h"
#include "mi200/GC/gc_9_0_default.h"
#include "mi200/GC/gc_9_0_offset.h"
#include "mi200/GC/gc_9_0_sh_mask.h"
#include "gfx_v9_0.h"
#include "mi_gfx.h"
#include "gfx_v9_4_2.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

#define GFX9_MEC_HPD_SIZE 4096

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

static void gfx_v9_0_kiq_set_resources(struct amdgv_ring *kiq_ring,
				uint64_t queue_mask)
{
	amdgv_ring_write(kiq_ring, PACKET3(PACKET3_SET_RESOURCES, 6));
	amdgv_ring_write(kiq_ring,
		PACKET3_SET_RESOURCES_VMID_MASK(0) |
		/* vmid_mask:0* queue_type:0 (KIQ) */
		PACKET3_SET_RESOURCES_QUEUE_TYPE(0));
	amdgv_ring_write(kiq_ring,
			lower_32_bits(queue_mask));	/* queue mask lo */
	amdgv_ring_write(kiq_ring,
			upper_32_bits(queue_mask));	/* queue mask hi */
	amdgv_ring_write(kiq_ring, 0);	/* gws mask lo */
	amdgv_ring_write(kiq_ring, 0);	/* gws mask hi */
	amdgv_ring_write(kiq_ring, 0);	/* oac mask */
	amdgv_ring_write(kiq_ring, 0);	/* gds heap base:0, gds heap size:0 */
}

static void gfx_v9_0_kiq_map_queues(struct amdgv_ring *kiq_ring,
				 struct amdgv_ring *ring)
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

static void gfx_v9_0_kiq_unmap_queues(struct amdgv_ring *kiq_ring,
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

static const struct kiq_pm4_funcs gfx_v9_0_kiq_pm4_funcs = {
	.kiq_set_resources = gfx_v9_0_kiq_set_resources,
	.kiq_map_queues = gfx_v9_0_kiq_map_queues,
	.kiq_unmap_queues = gfx_v9_0_kiq_unmap_queues,
	.set_resources_size = 8,
	.map_queues_size = 7,
	.unmap_queues_size = 6,
	.query_status_size = 7,
	.invalidate_tlbs_size = 2,
};

static void gfx_v9_0_set_kiq_pm4_funcs(struct amdgv_adapter *adapt)
{
	adapt->gfx.kiq[0].pmf = &gfx_v9_0_kiq_pm4_funcs;
}

static void gfx_v9_0_init_rlcg_reg_access_ctrl(struct amdgv_adapter *adapt)
{
	struct amdgv_rlcg_reg_access_ctrl *reg_access_ctrl;

	reg_access_ctrl = &adapt->gfx.rlc.reg_access_ctrl[0];
	reg_access_ctrl->scratch_reg0 = SOC15_REG_OFFSET(GC, 0, mmSCRATCH_REG0);
	reg_access_ctrl->scratch_reg1 = SOC15_REG_OFFSET(GC, 0, mmSCRATCH_REG1);
	reg_access_ctrl->scratch_reg2 = SOC15_REG_OFFSET(GC, 0, mmSCRATCH_REG2);
	reg_access_ctrl->scratch_reg3 = SOC15_REG_OFFSET(GC, 0, mmSCRATCH_REG3);
	reg_access_ctrl->grbm_cntl = SOC15_REG_OFFSET(GC, 0, mmGRBM_GFX_CNTL);
	reg_access_ctrl->grbm_idx = SOC15_REG_OFFSET(GC, 0, mmGRBM_GFX_INDEX);
	reg_access_ctrl->spare_int = SOC15_REG_OFFSET(GC, 0, mmRLC_SPARE_INT);
	adapt->gfx.rlc.rlcg_reg_access_supported = true;
}

static void soc15_grbm_select(struct amdgv_adapter *adapt,
		     uint32_t me, uint32_t pipe, uint32_t queue, uint32_t vmid)
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

	WREG32_SOC15_RLC_SHADOW(GC, 0, mmGRBM_GFX_CNTL, grbm_gfx_cntl);
}

#define DEFAULT_SH_MEM_BASES	(0x6000)
static void gfx_v9_0_init_compute_vmid(struct amdgv_adapter *adapt)
{
	int i;
	uint32_t sh_mem_config;
	uint32_t sh_mem_bases;

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
		soc15_grbm_select(adapt, 0, 0, 0, i);
		/* CP and shaders */
		WREG32_SOC15_RLC(GC, 0, mmSH_MEM_CONFIG, sh_mem_config);
		WREG32_SOC15_RLC(GC, 0, mmSH_MEM_BASES, sh_mem_bases);
	}
	soc15_grbm_select(adapt, 0, 0, 0, 0);
	oss_mutex_unlock(adapt->srbm_mutex);

	/* Initialize all compute VMIDs to have no GDS, GWS, or OA
	 * access. These should be enabled by FW for target VMIDs.
	 */
	for (i = FIRST_KFD_VMID; i < AMDGV_NUM_VMID; i++) {
		WREG32_SOC15_OFFSET(GC, 0, mmGDS_VMID0_BASE, 2 * i, 0);
		WREG32_SOC15_OFFSET(GC, 0, mmGDS_VMID0_SIZE, 2 * i, 0);
		WREG32_SOC15_OFFSET(GC, 0, mmGDS_GWS_VMID0, i, 0);
		WREG32_SOC15_OFFSET(GC, 0, mmGDS_OA_VMID0, i, 0);
	}
}

static void gfx_v9_0_init_gds_vmid(struct amdgv_adapter *adapt)
{
	int vmid;

	/*
	 * Initialize all compute and user-gfx VMIDs to have no GDS, GWS, or OA
	 * access. Compute VMIDs should be enabled by FW for target VMIDs,
	 * the driver can enable them for graphics. VMID0 should maintain
	 * access so that HWS firmware can save/restore entries.
	 */
	for (vmid = 1; vmid < AMDGV_NUM_VMID; vmid++) {
		WREG32_SOC15_OFFSET(GC, 0, mmGDS_VMID0_BASE, 2 * vmid, 0);
		WREG32_SOC15_OFFSET(GC, 0, mmGDS_VMID0_SIZE, 2 * vmid, 0);
		WREG32_SOC15_OFFSET(GC, 0, mmGDS_GWS_VMID0, vmid, 0);
		WREG32_SOC15_OFFSET(GC, 0, mmGDS_OA_VMID0, vmid, 0);
	}
}

static void gfx_v9_0_constants_init(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	int i;

	WREG32_FIELD15_RLC(GC, 0, GRBM_CNTL, READ_TIMEOUT, 0xff);

	/* XXX SH_MEM regs */
	/* where to put LDS, scratch, GPUVM in FSA64 space */
	oss_mutex_lock(adapt->srbm_mutex);
	for (i = 0; i < NUM_IDS; i++) {
		soc15_grbm_select(adapt, 0, 0, 0, i);
		/* CP and shaders */
		if (i == 0) {
			tmp = REG_SET_FIELD(0, SH_MEM_CONFIG, ALIGNMENT_MODE,
					    SH_MEM_ALIGNMENT_MODE_UNALIGNED);
			tmp = REG_SET_FIELD(tmp, SH_MEM_CONFIG,
					RETRY_DISABLE, 1);
			WREG32_SOC15_RLC(GC, 0, mmSH_MEM_CONFIG, tmp);
			WREG32_SOC15_RLC(GC, 0, mmSH_MEM_BASES, 0);
		} else {
			tmp = REG_SET_FIELD(0, SH_MEM_CONFIG, ALIGNMENT_MODE,
					    SH_MEM_ALIGNMENT_MODE_UNALIGNED);
			tmp = REG_SET_FIELD(tmp, SH_MEM_CONFIG,
					RETRY_DISABLE, 1);
			WREG32_SOC15_RLC(GC, 0, mmSH_MEM_CONFIG, tmp);
			tmp = REG_SET_FIELD(0, SH_MEM_BASES, PRIVATE_BASE,
				(0x1000000000000000ULL >> 48));
			tmp = REG_SET_FIELD(tmp, SH_MEM_BASES, SHARED_BASE,
				(0x2000000000000000ULL >> 48));
			WREG32_SOC15_RLC(GC, 0, mmSH_MEM_BASES, tmp);
		}
	}
	soc15_grbm_select(adapt, 0, 0, 0, 0);

	oss_mutex_unlock(adapt->srbm_mutex);

	gfx_v9_0_init_compute_vmid(adapt);
	gfx_v9_0_init_gds_vmid(adapt);
}

static void gfx_v9_0_init_tcp_config(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	tmp = RREG32_SOC15(GC, 0, mmTCP_ADDR_CONFIG);
	tmp = REG_SET_FIELD(tmp, TCP_ADDR_CONFIG, ENABLE64KHASH, 1);
	tmp = REG_SET_FIELD(tmp, TCP_ADDR_CONFIG, ENABLE2MHASH, 1);
	tmp = REG_SET_FIELD(tmp, TCP_ADDR_CONFIG, ENABLE1GHASH, 1);
	WREG32_SOC15(GC, 0, mmTCP_ADDR_CONFIG, tmp);
}

static int gfx_v9_0_gpu_early_init(struct amdgv_adapter *adapt)
{
	adapt->gfx.config.max_hw_contexts = 8;
	adapt->gfx.config.sc_prim_fifo_size_frontend = 0x20;
	adapt->gfx.config.sc_prim_fifo_size_backend = 0x100;
	adapt->gfx.config.sc_hiz_tile_fifo_size = 0x30;
	adapt->gfx.config.sc_earlyz_tile_fifo_size = 0x4C0;

	return 0;
}

static int gfx_v9_0_gpu_early_init_set(struct amdgv_adapter *adapt)
{
	uint32_t gb_addr_config;
	gb_addr_config = RREG32_SOC15(GC, 0, mmGB_ADDR_CONFIG);
	gb_addr_config &= ~0xf3e777ff;
	gb_addr_config |= 0x22014042;

	adapt->gfx.config.gb_addr_config = gb_addr_config;

	adapt->gfx.config.gb_addr_config_fields.num_pipes = 1 <<
		REG_GET_FIELD(
				adapt->gfx.config.gb_addr_config,
				GB_ADDR_CONFIG,
				NUM_PIPES);

	adapt->gfx.config.max_tile_pipes =
		adapt->gfx.config.gb_addr_config_fields.num_pipes;

	adapt->gfx.config.gb_addr_config_fields.num_banks = 1 <<
		REG_GET_FIELD(
				adapt->gfx.config.gb_addr_config,
				GB_ADDR_CONFIG,
				NUM_BANKS);
	adapt->gfx.config.gb_addr_config_fields.max_compress_frags = 1 <<
		REG_GET_FIELD(
				adapt->gfx.config.gb_addr_config,
				GB_ADDR_CONFIG,
				MAX_COMPRESSED_FRAGS);
	adapt->gfx.config.gb_addr_config_fields.num_rb_per_se = 1 <<
		REG_GET_FIELD(
				adapt->gfx.config.gb_addr_config,
				GB_ADDR_CONFIG,
				NUM_RB_PER_SE);
	adapt->gfx.config.gb_addr_config_fields.num_se = 1 <<
		REG_GET_FIELD(
				adapt->gfx.config.gb_addr_config,
				GB_ADDR_CONFIG,
				NUM_SHADER_ENGINES);
	adapt->gfx.config.gb_addr_config_fields.pipe_interleave_size = 1 << (8 +
			REG_GET_FIELD(
				adapt->gfx.config.gb_addr_config,
				GB_ADDR_CONFIG,
				PIPE_INTERLEAVE_SIZE));

	return 0;
}

static int gfx_v9_0_compute_ring_init(struct amdgv_adapter *adapt, int ring_id,
				      int mec, int pipe, int queue)
{
	struct amdgv_ring *ring = &adapt->gfx.compute_ring[ring_id];
	unsigned int hw_prio;
	uint32_t frame_dword_size = 1024;
	uint32_t frame_number = 2;

	ring = &adapt->gfx.compute_ring[ring_id];

	/* mec0 is me1 */
	ring->me = mec + 1;
	ring->pipe = pipe;
	ring->queue = queue;
	ring->xcc_id = 0;

	ring->ring_obj = NULL;
	ring->use_doorbell = true;
	ring->doorbell_index = (adapt->doorbell_index.mec_ring0 + ring_id) << 1;
	ring->eop_gpu_addr = adapt->gfx.mec.hpd_eop_gpu_addr
				+ (ring_id * GFX9_MEC_HPD_SIZE);
	oss_vsnprintf(ring->name, 12, "comp_%d.%d.%d", ring->me,
			ring->pipe, ring->queue);

	hw_prio = amdgv_gfx_is_high_priority_compute_queue(adapt, ring) ?
			AMDGV_RING_PRIO_2 : AMDGV_RING_PRIO_DEFAULT;
	/* type-2 packets are deprecated on MEC, use type-3 instead */
	return amdgv_ring_init(adapt, ring, frame_dword_size, frame_number, hw_prio, NULL, MEM_COMPUTE0_RING + ring_id);
}

static void gfx_v9_0_mec_fini(struct amdgv_adapter *adapt)
{
	amdgv_memmgr_free(adapt->gfx.mec.hpd_eop_obj);
}

static int gfx_v9_0_mec_init(struct amdgv_adapter *adapt)
{
	uint32_t mec_hpd_size;

	adapt->gfx.mec_queue_bitmap[0] = 0;

	/* take ownership of the relevant compute queues */
	amdgv_gfx_compute_queue_acquire(adapt);
	mec_hpd_size = adapt->gfx.num_compute_rings * GFX9_MEC_HPD_SIZE;
	if (mec_hpd_size) {
		adapt->gfx.mec.hpd_eop_obj =
			amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
					 mec_hpd_size, PAGE_SIZE, MEM_GFX_EOP);
		if (!adapt->gfx.mec.hpd_eop_obj) {
			AMDGV_WARN("create HDP EOP bo failed\n");
			gfx_v9_0_mec_fini(adapt);
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

static int gfx_v9_0_mec_init_set(struct amdgv_adapter *adapt)
{
	uint32_t i, *hpd;
	uint32_t mec_hpd_size = adapt->gfx.num_compute_rings * GFX9_MEC_HPD_SIZE;

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

static int gfx_v9_0_early_init(struct amdgv_adapter *adapt);

static int gfx_v9_0_sw_init_internal(struct amdgv_adapter *adapt)
{
	int r, ring_id;
	uint32_t i, j, k;
	struct amdgv_kiq *kiq;

	adapt->gfx.mec.num_mec = 2;
	adapt->gfx.mec.num_pipe_per_mec = 4;
	adapt->gfx.mec.num_queue_per_pipe = 8;

	gfx_v9_0_early_init(adapt);

	adapt->gfx.gfx_current_status = AMDGV_GFX_NORMAL_MODE;

	r = gfx_v9_0_mec_init(adapt);
	if (r) {
		AMDGV_ERROR("Failed to init MEC BOs!\n");
		return r;
	}

	/* set up the compute queues - allocate horizontally across pipes */
	ring_id = 0;
	for (i = 0; i < adapt->gfx.mec.num_mec; ++i) {
		for (j = 0; j < adapt->gfx.mec.num_queue_per_pipe; j++) {
			for (k = 0; k < adapt->gfx.mec.num_pipe_per_mec; k++) {
				if (!amdgv_gfx_is_mec_queue_enabled(adapt, 0, i, k, j))
					continue;

				r = gfx_v9_0_compute_ring_init(adapt, ring_id, i, k, j);
				if (r)
					return r;

				ring_id++;
			}
		}
	}

	r = amdgv_gfx_kiq_init(adapt, GFX9_MEC_HPD_SIZE, 0);
	if (r) {
		AMDGV_ERROR("Failed to init KIQ BOs!\n");
		return r;
	}

	kiq = &adapt->gfx.kiq[0];
	kiq->ring.me = 2;
	kiq->ring.pipe = 1;
	kiq->ring.queue = 0;
	r = amdgv_gfx_kiq_init_ring(adapt, &kiq->ring, 0);
	if (r)
		return r;

	/* create MQD for all compute queues as wel as KIQ for SRIOV case */
	r = amdgv_gfx_mqd_sw_init(adapt, sizeof(struct v9_mqd_allocation), 0);
	if (r)
		return r;

	adapt->gfx.ce_ram_size = 0x8000;

	r = gfx_v9_0_gpu_early_init(adapt);
	if (r)
		return r;

	return 0;
}

static int gfx_v9_0_hw_init_internal_set(struct amdgv_adapter *adapt)
{
	int ring_id, r;
	uint32_t i, j, k;
	struct amdgv_ring *ring;

	r = gfx_v9_0_mec_init_set(adapt);
	if (r) {
		AMDGV_ERROR("Failed to init MEC set!\n");
		return r;
	}

	ring_id = 0;
	for (i = 0; i < adapt->gfx.mec.num_mec; ++i) {
		for (j = 0; j < adapt->gfx.mec.num_queue_per_pipe; j++) {
			for (k = 0; k < adapt->gfx.mec.num_pipe_per_mec; k++) {
				if (!amdgv_gfx_is_mec_queue_enabled(adapt, 0, i, k, j))
					continue;

				ring = &adapt->gfx.compute_ring[ring_id];
				r = amdgv_ring_init_set(adapt, ring);
				if (r)
					return r;

				ring_id++;
			}
		}
	}
	r = amdgv_gfx_kiq_init_set(adapt, GFX9_MEC_HPD_SIZE, 0);
	if (r) {
		AMDGV_ERROR("Failed to init KIQ BOs!\n");
		return r;
	}

	ring = &adapt->gfx.kiq[0].ring;
	r = amdgv_ring_init_set(adapt, ring);
	if (r)
		return r;

	amdgv_gfx_mqd_init_set(adapt, 0);

	gfx_v9_0_gpu_early_init_set(adapt);

	return 0;
}

static int gfx_v9_0_sw_fini_internal(struct amdgv_adapter *adapt)
{
	uint32_t i;

	for (i = 0; i < adapt->gfx.num_compute_rings; i++)
		amdgv_ring_fini(&adapt->gfx.compute_ring[i]);

	amdgv_gfx_mqd_sw_fini(adapt, 0);
	amdgv_gfx_kiq_free_ring(&adapt->gfx.kiq[0].ring);
	amdgv_gfx_kiq_fini(adapt, 0);

	gfx_v9_0_mec_fini(adapt);

	return 0;
}

static int gfx_v9_0_sw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int gfx_v9_0_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static void gfx_v9_0_enable_save_restore_machine(struct amdgv_adapter *adapt)
{
	WREG32_FIELD15(GC, 0, RLC_SRM_CNTL, SRM_ENABLE, 1);
}

/* shared by other blocks */
void mi200_gfx_select_se_sh(struct amdgv_adapter *adapt,
		uint32_t se, uint32_t sh, uint32_t instance)
{
	uint32_t data;

	if (instance == 0xffffffff)
		data = REG_SET_FIELD(0, GRBM_GFX_INDEX,
				INSTANCE_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(0, GRBM_GFX_INDEX,
				INSTANCE_INDEX, instance);

	if (se == 0xffffffff)
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX,
				SE_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SE_INDEX, se);

	if (sh == 0xffffffff)
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX,
				SH_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SH_INDEX, sh);

	WREG32(SOC15_REG_OFFSET(GC, 0, mmGRBM_GFX_INDEX), data);
	AMDGV_DEBUG("GRBM_GFX_INDEX=0x%x\n", data);
}

static void gfx_v9_0_init_pg(struct amdgv_adapter *adapt)
{
	/*
	 * Rlc save restore list is workable since v2_1.
	 * And it's needed by gfxoff feature.
	 */
	gfx_v9_0_enable_save_restore_machine(adapt);
}

static void gfx_v9_0_enable_gui_idle_interrupt(struct amdgv_adapter *adapt,
					       bool enable)
{
	uint32_t tmp;

	/* These interrupts should be enabled to drive DS clock */
	tmp = RREG32_SOC15(GC, 0, mmCP_INT_CNTL_RING0);

	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0,
			CNTX_BUSY_INT_ENABLE, enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0,
			CNTX_EMPTY_INT_ENABLE, enable ? 1 : 0);
	tmp = REG_SET_FIELD(tmp, CP_INT_CNTL_RING0,
			CMP_BUSY_INT_ENABLE, enable ? 1 : 0);

	WREG32_SOC15(GC, 0, mmCP_INT_CNTL_RING0, tmp);
}

static void gfx_v9_0_wait_for_rlc_serdes(struct amdgv_adapter *adapt)
{
	uint32_t i, j, k;
	uint32_t mask;

	oss_mutex_lock(adapt->grbm_idx_mutex);
	for (i = 0; i < adapt->config.gfx.max_shader_engines; i++) {
		for (j = 0; j < adapt->config.gfx.max_sh_per_se; j++) {
			mi200_gfx_select_se_sh(adapt, i, j, 0xffffffff);
			for (k = 0; k < AMDGV_GFX_MAX_USEC_TIMEOUT; k++) {
				if (RREG32_SOC15(GC, 0,
				    mmRLC_SERDES_CU_MASTER_BUSY) == 0)
					break;
				oss_udelay(1);
			}
			if (k == AMDGV_GFX_MAX_USEC_TIMEOUT) {
				mi200_gfx_select_se_sh(adapt, 0xffffffff,
						      0xffffffff, 0xffffffff);
				oss_mutex_unlock(adapt->grbm_idx_mutex);
				AMDGV_INFO(
					"Timeout wait for RLC serdes %u,%u\n",
					 i, j);
				return;
			}
		}
	}
	mi200_gfx_select_se_sh(adapt, 0xffffffff, 0xffffffff, 0xffffffff);
	oss_mutex_unlock(adapt->grbm_idx_mutex);

	mask = RLC_SERDES_NONCU_MASTER_BUSY__SE_MASTER_BUSY_MASK |
		RLC_SERDES_NONCU_MASTER_BUSY__GC_MASTER_BUSY_MASK |
		RLC_SERDES_NONCU_MASTER_BUSY__TC0_MASTER_BUSY_MASK |
		RLC_SERDES_NONCU_MASTER_BUSY__TC1_MASTER_BUSY_MASK;
	for (k = 0; k < AMDGV_GFX_MAX_USEC_TIMEOUT; k++) {
		if ((RREG32_SOC15(GC, 0,
		    mmRLC_SERDES_NONCU_MASTER_BUSY) & mask) == 0)
			break;
		oss_udelay(1);
	}
}

static void gfx_v9_0_rlc_stop(struct amdgv_adapter *adapt)
{
	WREG32_FIELD15(GC, 0, RLC_CNTL, RLC_ENABLE_F32, 0);
	gfx_v9_0_enable_gui_idle_interrupt(adapt, false);
	gfx_v9_0_wait_for_rlc_serdes(adapt);
}

static void gfx_v9_0_rlc_start(struct amdgv_adapter *adapt)
{
#ifdef AMDGV_RLC_DEBUG_RETRY
	uint32_t rlc_ucode_ver;
#endif

	WREG32_FIELD15(GC, 0, RLC_CNTL, RLC_ENABLE_F32, 1);
	oss_udelay(50);

	gfx_v9_0_enable_gui_idle_interrupt(adapt, true);
	oss_udelay(50);

#ifdef AMDGV_RLC_DEBUG_RETRY
	/* RLC_GPM_GENERAL_6 : RLC Ucode version */
	rlc_ucode_ver = RREG32_SOC15(GC, 0, mmRLC_GPM_GENERAL_6);
	if (rlc_ucode_ver == 0x108) {
		AMDGV_INFO(
			"Using rlc debug ucode. mmRLC_GPM_GENERAL_6 ==0x08%x / fw_ver == %i \n",
			rlc_ucode_ver, adapt->gfx.rlc_fw_version);
		/* RLC_GPM_TIMER_INT_3 : Timer interval in RefCLK cycles,
		 * default is 0x9C4 to create a 100us interval
		 */
		WREG32_SOC15(GC, 0, mmRLC_GPM_TIMER_INT_3, 0x9C4);
		/* RLC_GPM_GENERAL_12 : Minimum gap between wptr and rptr
		 * to disable the page fault retry interrupts, default is
		 * 0x100 (256)
		 */
		WREG32_SOC15(GC, 0, mmRLC_GPM_GENERAL_12, 0x100);
	}
#endif
}

static int gfx_v9_0_rlc_resume(struct amdgv_adapter *adapt)
{
	gfx_v9_0_rlc_stop(adapt);

	/* disable CG */
	WREG32_SOC15(GC, 0, mmRLC_CGCG_CGLS_CTRL, 0);

	gfx_v9_0_init_pg(adapt);

	gfx_v9_0_rlc_start(adapt);

	return 0;
}

static const struct amdgv_rlc_funcs gfx_v9_0_rlc_funcs = {
	.resume = gfx_v9_0_rlc_resume,
	.stop = gfx_v9_0_rlc_stop,
	.start = gfx_v9_0_rlc_start,
};

static void gfx_v9_0_set_rlc_funcs(struct amdgv_adapter *adapt)
{
	adapt->gfx.rlc.funcs = &gfx_v9_0_rlc_funcs;
}

/* KIQ functions */
static void gfx_v9_0_kiq_setting(struct amdgv_ring *ring)
{
	uint32_t tmp;
	struct amdgv_adapter *adapt = ring->adapt;

	/* tell RLC which is KIQ queue */
	tmp = RREG32_SOC15(GC, 0, mmRLC_CP_SCHEDULERS);
	tmp &= 0xffffff00;
	tmp |= (ring->me << 5) | (ring->pipe << 3) | (ring->queue);
	WREG32_SOC15_RLC(GC, 0, mmRLC_CP_SCHEDULERS, tmp);
	tmp |= 0x80;
	WREG32_SOC15_RLC(GC, 0, mmRLC_CP_SCHEDULERS, tmp);
}

static void gfx_v9_0_cp_compute_enable(struct amdgv_adapter *adapt, bool enable)
{
	if (enable) {
		WREG32_SOC15_RLC(GC, 0, mmCP_MEC_CNTL, 0);
	} else {
		WREG32_SOC15_RLC(GC, 0, mmCP_MEC_CNTL,
			(CP_MEC_CNTL__MEC_ME1_HALT_MASK |
			CP_MEC_CNTL__MEC_ME2_HALT_MASK));
	}
	oss_udelay(50);
}

static void gfx_v9_0_mqd_set_priority(struct amdgv_ring *ring,
				struct v9_mqd *mqd)
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

static int gfx_v9_0_mqd_init(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v9_mqd *mqd = (struct v9_mqd *)ring->mqd_ptr;
	uint64_t hqd_gpu_addr, wb_gpu_addr, eop_base_addr;
	uint32_t tmp;

	mqd->header = 0xC0310800;
	mqd->compute_pipelinestat_enable = 0x00000001;
	mqd->compute_static_thread_mgmt_se0 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se1 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se2 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se3 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se4 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se5 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se6 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se7 = 0xffffffff;
	mqd->compute_misc_reserved = 0x00000003;

	mqd->dynamic_cu_mask_addr_lo =
		lower_32_bits(ring->mqd_gpu_addr
			      + offsetof(struct v9_mqd_allocation,
			      dynamic_cu_mask));
	mqd->dynamic_cu_mask_addr_hi =
		upper_32_bits(ring->mqd_gpu_addr
			      + offsetof(struct v9_mqd_allocation,
			      dynamic_cu_mask));

	eop_base_addr = ring->eop_gpu_addr >> 8;
	mqd->cp_hqd_eop_base_addr_lo = eop_base_addr;
	mqd->cp_hqd_eop_base_addr_hi = upper_32_bits(eop_base_addr);

	/* set the EOP size, register value is 2^(EOP_SIZE+1) dwords */
	tmp = RREG32_SOC15(GC, 0, mmCP_HQD_EOP_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_EOP_CONTROL, EOP_SIZE,
			(order_base_2(GFX9_MEC_HPD_SIZE / 4) - 1));

	mqd->cp_hqd_eop_control = tmp;

	/* enable doorbell? */
	tmp = RREG32_SOC15(GC, 0, mmCP_HQD_PQ_DOORBELL_CONTROL);

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
	ring->wptr = 0;
	mqd->cp_hqd_dequeue_request = 0;
	mqd->cp_hqd_pq_rptr = 0;
	mqd->cp_hqd_pq_wptr_lo = 0;
	mqd->cp_hqd_pq_wptr_hi = 0;

	/* set the pointer to the MQD */
	mqd->cp_mqd_base_addr_lo = ring->mqd_gpu_addr & 0xfffffffc;
	mqd->cp_mqd_base_addr_hi = upper_32_bits(ring->mqd_gpu_addr);

	/* set MQD vmid to 0 */
	tmp = RREG32_SOC15(GC, 0, mmCP_MQD_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_MQD_CONTROL, VMID, 0);
	mqd->cp_mqd_control = tmp;

	/* set the pointer to the HQD, this is similar CP_RB0_BASE/_HI */
	hqd_gpu_addr = ring->gpu_addr >> 8;
	mqd->cp_hqd_pq_base_lo = hqd_gpu_addr;
	mqd->cp_hqd_pq_base_hi = upper_32_bits(hqd_gpu_addr);

	/* set up the HQD, this is similar to CP_RB0_CNTL */
	tmp = RREG32_SOC15(GC, 0, mmCP_HQD_PQ_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, QUEUE_SIZE,
			    (order_base_2(ring->ring_size) - 1));
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, RPTR_BLOCK_SIZE,
			(order_base_2(AMDGV_GPU_PAGE_SIZE / 4) - 1));
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

	tmp = 0;
	/* enable the doorbell if requested */
	if (ring->use_doorbell) {
		tmp = RREG32_SOC15(GC, 0, mmCP_HQD_PQ_DOORBELL_CONTROL);
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
				DOORBELL_OFFSET, ring->doorbell_index);

		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
					 DOORBELL_EN, 1);
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
					 DOORBELL_SOURCE, 0);
		tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
					 DOORBELL_HIT, 0);
	}

	mqd->cp_hqd_pq_doorbell_control = tmp;

	/* reset read and write pointers, similar to CP_RB0_WPTR/_RPTR */
	ring->wptr = 0;
	mqd->cp_hqd_pq_rptr = RREG32_SOC15(GC, 0, mmCP_HQD_PQ_RPTR);

	/* set the vmid for the queue */
	mqd->cp_hqd_vmid = 0;

	tmp = RREG32_SOC15(GC, 0, mmCP_HQD_PERSISTENT_STATE);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PERSISTENT_STATE, PRELOAD_SIZE, 0x53);
	mqd->cp_hqd_persistent_state = tmp;

	/* set MIN_IB_AVAIL_SIZE */
	tmp = RREG32_SOC15(GC, 0, mmCP_HQD_IB_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_IB_CONTROL, MIN_IB_AVAIL_SIZE, 3);
	mqd->cp_hqd_ib_control = tmp;

	/* set static priority for a queue/ring */
	gfx_v9_0_mqd_set_priority(ring, mqd);
	mqd->cp_hqd_quantum = RREG32_SOC15(GC, 0, mmCP_HQD_QUANTUM);

	/* map_queues packet doesn't need activate the queue,
	 * so only kiq need set this field.
	 */
	if (ring->funcs->type == AMDGV_RING_TYPE_KIQ)
		mqd->cp_hqd_active = 1;

	return 0;
}

static int gfx_v9_0_kiq_init_register(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v9_mqd *mqd = (struct v9_mqd *)ring->mqd_ptr;
	int j;

	/* disable wptr polling */
	WREG32_FIELD15(GC, 0, CP_PQ_WPTR_POLL_CNTL, EN, 0);

	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_EOP_BASE_ADDR,
	       mqd->cp_hqd_eop_base_addr_lo);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_EOP_BASE_ADDR_HI,
	       mqd->cp_hqd_eop_base_addr_hi);

	/* set the EOP size, register value is 2^(EOP_SIZE+1) dwords */
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_EOP_CONTROL,
	       mqd->cp_hqd_eop_control);

	/* enable doorbell? */
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_DOORBELL_CONTROL,
	       mqd->cp_hqd_pq_doorbell_control);

	/* disable the queue if it's active */
	if (RREG32_SOC15(GC, 0, mmCP_HQD_ACTIVE) & 1) {
		WREG32_SOC15_RLC(GC, 0, mmCP_HQD_DEQUEUE_REQUEST, 1);
		for (j = 0; j < AMDGV_GFX_MAX_USEC_TIMEOUT; j++) {
			if (!(RREG32_SOC15(GC, 0, mmCP_HQD_ACTIVE) & 1))
				break;
			oss_udelay(1);
		}
		WREG32_SOC15_RLC(GC, 0, mmCP_HQD_DEQUEUE_REQUEST,
		       mqd->cp_hqd_dequeue_request);
		WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_RPTR,
		       mqd->cp_hqd_pq_rptr);
		WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_WPTR_LO,
		       mqd->cp_hqd_pq_wptr_lo);
		WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_WPTR_HI,
		       mqd->cp_hqd_pq_wptr_hi);
	}

	/* set the pointer to the MQD */
	WREG32_SOC15_RLC(GC, 0, mmCP_MQD_BASE_ADDR,
	       mqd->cp_mqd_base_addr_lo);
	WREG32_SOC15_RLC(GC, 0, mmCP_MQD_BASE_ADDR_HI,
	       mqd->cp_mqd_base_addr_hi);

	/* set MQD vmid to 0 */
	WREG32_SOC15_RLC(GC, 0, mmCP_MQD_CONTROL,
	       mqd->cp_mqd_control);

	/* set the pointer to the HQD, this is similar CP_RB0_BASE/_HI */
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_BASE,
	       mqd->cp_hqd_pq_base_lo);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_BASE_HI,
	       mqd->cp_hqd_pq_base_hi);

	/* set up the HQD, this is similar to CP_RB0_CNTL */
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_CONTROL,
	       mqd->cp_hqd_pq_control);

	/* set the wb address whether it's enabled or not */
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_RPTR_REPORT_ADDR,
				mqd->cp_hqd_pq_rptr_report_addr_lo);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_RPTR_REPORT_ADDR_HI,
				mqd->cp_hqd_pq_rptr_report_addr_hi);

	/* only used if CP_PQ_WPTR_POLL_CNTL.CP_PQ_WPTR_POLL_CNTL__EN_MASK=1 */
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_WPTR_POLL_ADDR,
	       mqd->cp_hqd_pq_wptr_poll_addr_lo);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_WPTR_POLL_ADDR_HI,
	       mqd->cp_hqd_pq_wptr_poll_addr_hi);

	/* enable the doorbell if requested */
	if (ring->use_doorbell) {
		WREG32_SOC15(GC, 0, mmCP_MEC_DOORBELL_RANGE_LOWER,
					(adapt->doorbell_index.kiq * 2) << 2);
		/* If GC has entered CGPG, ringing doorbell > first page
		 * doesn't wakeup GC. Enlarge CP_MEC_DOORBELL_RANGE_UPPER to
		 * WA this issue. And this change has to align with
		 * firmware update.
		 */
		WREG32_SOC15(GC, 0, mmCP_MEC_DOORBELL_RANGE_UPPER,
			(adapt->doorbell_index.userqueue_end * 2) << 2);
	}

	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_DOORBELL_CONTROL,
	       mqd->cp_hqd_pq_doorbell_control);

	/* reset read and write pointers, similar to CP_RB0_WPTR/_RPTR */
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_WPTR_LO,
	       mqd->cp_hqd_pq_wptr_lo);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_WPTR_HI,
	       mqd->cp_hqd_pq_wptr_hi);

	/* set the vmid for the queue */
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_VMID, mqd->cp_hqd_vmid);

	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PERSISTENT_STATE,
	       mqd->cp_hqd_persistent_state);

	/* activate the queue */
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_ACTIVE,
	       mqd->cp_hqd_active);

	if (ring->use_doorbell)
		WREG32_FIELD15(GC, 0, CP_PQ_STATUS, DOORBELL_ENABLE, 1);

	return 0;
}

static int gfx_v9_0_kiq_fini_register(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	int j;

	/* disable the queue if it's active */
	if (RREG32_SOC15(GC, 0, mmCP_HQD_ACTIVE) & 1) {

		WREG32_SOC15_RLC(GC, 0, mmCP_HQD_DEQUEUE_REQUEST, 1);

		for (j = 0; j < AMDGV_GFX_MAX_USEC_TIMEOUT; j++) {
			if (!(RREG32_SOC15(GC, 0, mmCP_HQD_ACTIVE) & 1))
				break;
			oss_udelay(1);
		}

		if (j == AMDGV_GFX_MAX_USEC_TIMEOUT) {
			AMDGV_DEBUG("KIQ dequeue request failed.\n");

			/* Manual disable if dequeue request times out */
			WREG32_SOC15_RLC(GC, 0, mmCP_HQD_ACTIVE, 0);
		}

		WREG32_SOC15_RLC(GC, 0, mmCP_HQD_DEQUEUE_REQUEST, 0);
	}

	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_IQ_TIMER, 0);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_IB_CONTROL, 0);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PERSISTENT_STATE, 0);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_DOORBELL_CONTROL, 0x40000000);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_DOORBELL_CONTROL, 0);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_RPTR, 0);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_WPTR_HI, 0);
	WREG32_SOC15_RLC(GC, 0, mmCP_HQD_PQ_WPTR_LO, 0);

	return 0;
}

static int gfx_v9_0_kiq_init_queue(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v9_mqd *mqd = ring->mqd_ptr;
	struct v9_mqd *tmp_mqd;

	gfx_v9_0_kiq_setting(ring);

	/* GPU could be in bad state during probe, driver trigger the reset
	 * after load the SMU, in this case , the mqd is not be initialized.
	 * driver need to re-init the mqd.
	 * check mqd->cp_hqd_pq_control since this value should not be 0
	 */
	tmp_mqd = (struct v9_mqd *)adapt->gfx.kiq[ring->xcc_id].mqd_backup;

	oss_memset((void *)mqd, 0, sizeof(struct v9_mqd_allocation));
	((struct v9_mqd_allocation *)mqd)->dynamic_cu_mask = 0xFFFFFFFF;
	((struct v9_mqd_allocation *)mqd)->dynamic_rb_mask = 0xFFFFFFFF;
	oss_mutex_lock(adapt->srbm_mutex);
	soc15_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0);
	gfx_v9_0_mqd_init(ring);
	gfx_v9_0_kiq_init_register(ring);
	soc15_grbm_select(adapt, 0, 0, 0, 0);
	oss_mutex_unlock(adapt->srbm_mutex);

	if (tmp_mqd)
		oss_memcpy(tmp_mqd,
				mqd, sizeof(struct v9_mqd_allocation));

	return 0;
}

static int gfx_v9_0_kcq_init_queue(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v9_mqd *mqd = ring->mqd_ptr;
	int mqd_idx = ring - &adapt->gfx.compute_ring[0];
	struct v9_mqd *tmp_mqd;

	/* Same as above kiq init, driver need to re-init the mqd
	 * if mqd->cp_hqd_pq_control is not be initialized before
	 */
	tmp_mqd = (struct v9_mqd *)adapt->gfx.mec.mqd_backup[mqd_idx];

	if (tmp_mqd && tmp_mqd->cp_hqd_pq_control) {
		oss_memcpy(mqd, tmp_mqd,
				sizeof(struct v9_mqd_allocation));
		amdgv_ring_clear_ring(ring);
	} else {
		oss_memset((void *)mqd, 0, sizeof(struct v9_mqd_allocation));
		((struct v9_mqd_allocation *)mqd)->dynamic_cu_mask = 0xFFFFFFFF;
		((struct v9_mqd_allocation *)mqd)->dynamic_rb_mask = 0xFFFFFFFF;
		oss_mutex_lock(adapt->srbm_mutex);
		soc15_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0);
		gfx_v9_0_mqd_init(ring);
		soc15_grbm_select(adapt, 0, 0, 0, 0);
		oss_mutex_unlock(adapt->srbm_mutex);

		if (tmp_mqd)
			oss_memcpy(tmp_mqd, mqd,
					sizeof(struct v9_mqd_allocation));
	}

	return 0;
}

static int gfx_v9_0_kiq_resume(struct amdgv_adapter *adapt)
{
	struct amdgv_ring *ring;

	ring = &adapt->gfx.kiq[0].ring;
	gfx_v9_0_kiq_init_queue(ring);
	ring->mqd_ptr = NULL;

	return amdgv_gfx_kiq_set_resources(adapt, 0);
}

static int gfx_v9_0_kcq_resume(struct amdgv_adapter *adapt)
{
	struct amdgv_ring *ring = NULL;
	int r = 0;
	uint32_t i;

	gfx_v9_0_cp_compute_enable(adapt, true);

	for (i = 0; i < adapt->gfx.num_compute_rings; i++) {
		ring = &adapt->gfx.compute_ring[i];
		r = gfx_v9_0_kcq_init_queue(ring);
		if (r)
			break;
		r = amdgv_gfx_map_kcq(adapt, 0, i);
		if (r)
			break;
	}
	return r;
}

static int gfx_v9_0_cp_resume(struct amdgv_adapter *adapt)
{
	int r;

	gfx_v9_0_enable_gui_idle_interrupt(adapt, false);

	r = gfx_v9_0_kiq_resume(adapt);
	if (r)
		return r;

	r = gfx_v9_0_kcq_resume(adapt);
	if (r)
		return r;

	gfx_v9_0_enable_gui_idle_interrupt(adapt, true);

	return 0;
}

static int gfx_v9_0_hw_fini_internal(struct amdgv_adapter *adapt);

static int gfx_v9_0_hw_init(struct amdgv_adapter *adapt)
{
	/* do nothing in gpu reset */
	if (in_whole_gpu_reset())
		return 0;


	gfx_v9_0_sw_init_internal(adapt);

	gfx_v9_0_hw_init_internal_set(adapt);

	gfx_v9_0_constants_init(adapt);

	gfx_v9_0_init_tcp_config(adapt);

	gfx_v9_0_rlc_resume(adapt);
	gfx_v9_0_cp_resume(adapt);

	gfx_v9_4_2_set_power_brake_sequence(adapt);
	if (!gfx_v9_4_2_do_edc_gpr_wa(adapt)) {
		amdgv_ecc_enable_ras_feature(adapt);
	}

	/* destroy GFX framework positively to avoid confliction with guest */
	gfx_v9_0_hw_fini_internal(adapt);
	gfx_v9_0_sw_fini_internal(adapt);


	/* Don't stop libgv init even gfx init fails */
	return 0;
}

static void gfx_v9_0_wait_reg_mem(struct amdgv_ring *ring, int eng_sel,
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

static void gfx_v9_0_ring_emit_hdp_flush(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t ref_and_mask, reg_mem_engine;
	const struct nbio_hdp_flush_reg *nbio_hf_reg =
				adapt->nbio.hdp_flush_reg;

	if (ring->funcs->type == AMDGV_RING_TYPE_COMPUTE) {
		switch (ring->me) {
		case 1:
			ref_and_mask =
				nbio_hf_reg->ref_and_mask_cp2 << ring->pipe;
			break;
		case 2:
			ref_and_mask =
				nbio_hf_reg->ref_and_mask_cp6 << ring->pipe;
			break;
		default:
			return;
		}
		reg_mem_engine = 0;
	} else {
		ref_and_mask = nbio_hf_reg->ref_and_mask_cp0;
		reg_mem_engine = 1; /* pfp */
	}

	gfx_v9_0_wait_reg_mem(ring, reg_mem_engine, 0, 1,
		adapt->nbio.funcs->get_hdp_flush_req_offset(adapt),
		adapt->nbio.funcs->get_hdp_flush_done_offset(adapt),
		ref_and_mask, ref_and_mask, 0x20);
}

static void gfx_v9_0_ring_set_wptr_compute(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;

	/* XXX check if swapping is necessary on BE */
	if (ring->use_doorbell) {
		*((volatile uint64_t *)(ring->wptr_cpu_addr)) = ring->wptr;
		WDOORBELL32(ring->doorbell_index, ring->wptr);
	} else {
		/* only DOORBELL method supported on gfx9 now */
	}
}

static void gfx_v9_0_ring_emit_ib_compute(struct amdgv_ring *ring,
					  struct amdgv_ib *ib,
					  uint32_t flags)
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

static void gfx_v9_0_ring_emit_fence(struct amdgv_ring *ring, uint64_t addr,
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

static void gfx_v9_0_ring_emit_wreg(struct amdgv_ring *ring, uint32_t reg,
				    uint32_t val)
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

static int gfx_v9_0_ring_test_ring(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t tmp = 0;
	unsigned int i;
	int r;

	WREG32_SOC15(GC, 0, mmSCRATCH_REG0, 0xCAFEDEAD);
	r = amdgv_ring_alloc(ring, 3);
	if (r)
		return r;

	amdgv_ring_write(ring, PACKET3(PACKET3_SET_UCONFIG_REG, 1));
	amdgv_ring_write(ring, SOC15_REG_OFFSET(GC, 0, mmSCRATCH_REG0) -
			  PACKET3_SET_UCONFIG_REG_START);
	amdgv_ring_write(ring, 0xDEADBEEF);
	amdgv_ring_commit(ring);

	for (i = 0; i < AMDGV_GFX_MAX_USEC_TIMEOUT; i++) {
		tmp = RREG32_SOC15(GC, 0, mmSCRATCH_REG0);
		if (tmp == 0xDEADBEEF)
			break;
		oss_udelay(1);
	}

	if (i >= AMDGV_GFX_MAX_USEC_TIMEOUT)
		r = AMDGV_FAILURE;
	return r;
}

static void gfx_v9_0_emit_wave_limit_cs(struct amdgv_ring *ring,
					uint32_t pipe, bool enable)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t val;
	uint32_t wcl_cs_reg;

	/* mmSPI_WCL_PIPE_PERCENT_CS[0-7]_DEFAULT values are same */
	val = enable ? 0x1 : mmSPI_WCL_PIPE_PERCENT_CS0_DEFAULT;

	switch (pipe) {
	case 0:
		wcl_cs_reg = SOC15_REG_OFFSET(GC, 0,
					mmSPI_WCL_PIPE_PERCENT_CS0);
		break;
	case 1:
		wcl_cs_reg = SOC15_REG_OFFSET(GC, 0,
					mmSPI_WCL_PIPE_PERCENT_CS1);
		break;
	case 2:
		wcl_cs_reg = SOC15_REG_OFFSET(GC, 0,
					mmSPI_WCL_PIPE_PERCENT_CS2);
		break;
	case 3:
		wcl_cs_reg = SOC15_REG_OFFSET(GC, 0,
					mmSPI_WCL_PIPE_PERCENT_CS3);
		break;
	default:
		AMDGV_DEBUG("invalid pipe %d\n", pipe);
		return;
	}

	amdgv_ring_emit_wreg(ring, wcl_cs_reg, val);
}

static void gfx_v9_0_emit_wave_limit(struct amdgv_ring *ring, bool enable)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t val;
	uint32_t i;


	/* mmSPI_WCL_PIPE_PERCENT_GFX is 7 bit multiplier register to limit
	 * number of gfx waves. Setting 5 bit will make sure gfx only gets
	 * around 25% of gpu resources.
	 */
	val = enable ? 0x1f : mmSPI_WCL_PIPE_PERCENT_GFX_DEFAULT;
	amdgv_ring_emit_wreg(ring,
		SOC15_REG_OFFSET(GC, 0, mmSPI_WCL_PIPE_PERCENT_GFX),
		val);

	/* Restrict waves for normal/low priority compute queues as well
	 * to get best QoS for high priority compute jobs.
	 *
	 * amdgv controls only 1st ME(0-3 CS pipes).
	 */
	for (i = 0; i < adapt->gfx.mec.num_pipe_per_mec; i++) {
		if (i != ring->pipe)
			gfx_v9_0_emit_wave_limit_cs(ring, i, enable);

	}
}

static const struct amdgv_ring_funcs gfx_v9_0_ring_funcs_compute = {
	.type = AMDGV_RING_TYPE_COMPUTE,
	.align_mask = 0xff,
	.nop = PACKET3(PACKET3_NOP, 0x3FFF),
	.support_64bit_ptrs = true,
	.vmhub = 0,
	.set_wptr = gfx_v9_0_ring_set_wptr_compute,
	.emit_frame_size =
		20 + /* gfx_v9_0_ring_emit_gds_switch */
		7 + /* gfx_v9_0_ring_emit_hdp_flush */
		5 + /* hdp invalidate */
		7 + /* gfx_v9_0_ring_emit_pipeline_sync */
		SOC15_FLUSH_GPU_TLB_NUM_WREG * 5 +
		SOC15_FLUSH_GPU_TLB_NUM_REG_WAIT * 7 +
		2 + /* gfx_v9_0_ring_emit_vm_flush */
		/* gfx_v9_0_ring_emit_fence x3 for user fence,
		 * vm fence
		 */
		8 + 8 + 8 +
		7 + /* gfx_v9_0_emit_mem_sync */
		/* gfx_v9_0_emit_wave_limit for updating
		 * mmSPI_WCL_PIPE_PERCENT_GFX register
		 */
		5 +
		15, /* for updating 3 mmSPI_WCL_PIPE_PERCENT_CS registers */
	.emit_ib_size =	7, /* gfx_v9_0_ring_emit_ib_compute */
	.emit_ib = gfx_v9_0_ring_emit_ib_compute,
	.emit_fence = gfx_v9_0_ring_emit_fence,
	.emit_hdp_flush = gfx_v9_0_ring_emit_hdp_flush,
	.insert_nop = amdgv_ring_insert_nop,
	.emit_wreg = gfx_v9_0_ring_emit_wreg,
	.emit_wave_limit = gfx_v9_0_emit_wave_limit,
};

static const struct amdgv_ring_funcs gfx_v9_0_ring_funcs_kiq = {
	.type = AMDGV_RING_TYPE_KIQ,
	.align_mask = 0xff,
	.nop = PACKET3(PACKET3_NOP, 0x3FFF),
	.support_64bit_ptrs = true,
	.vmhub = 0,
	.set_wptr = gfx_v9_0_ring_set_wptr_compute,
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
	.test_ring = gfx_v9_0_ring_test_ring,
	.insert_nop = amdgv_ring_insert_nop,
	.emit_wreg = gfx_v9_0_ring_emit_wreg,
};

static void gfx_v9_0_set_ring_funcs(struct amdgv_adapter *adapt)
{
	uint32_t i;

	adapt->gfx.kiq[0].ring.funcs = &gfx_v9_0_ring_funcs_kiq;

	for (i = 0; i < adapt->gfx.num_compute_rings; i++)
		adapt->gfx.compute_ring[i].funcs = &gfx_v9_0_ring_funcs_compute;
}

static int gfx_v9_0_early_init(struct amdgv_adapter *adapt)
{
	adapt->gfx.num_gfx_rings = 0;
	adapt->gfx.num_compute_rings = AMDGV_MAX_COMPUTE_RINGS;

	gfx_v9_0_set_ring_funcs(adapt);
	gfx_v9_0_set_kiq_pm4_funcs(adapt);
	gfx_v9_0_set_rlc_funcs(adapt);

	/* init rlcg reg access ctrl */
	gfx_v9_0_init_rlcg_reg_access_ctrl(adapt);

	return 0;
}

static int gfx_v9_0_hw_fini_internal(struct amdgv_adapter *adapt)
{
	/* DF freeze and kcq disable will fail */

	/* Use deinitialize sequence from CAIL when unbinding device
	 * from driver, otherwise KIQ is hanging when binding back
	 */
	oss_mutex_lock(adapt->srbm_mutex);
	soc15_grbm_select(adapt, adapt->gfx.kiq[0].ring.me,
			adapt->gfx.kiq[0].ring.pipe,
			adapt->gfx.kiq[0].ring.queue, 0);
	gfx_v9_0_kiq_fini_register(&adapt->gfx.kiq[0].ring);
	soc15_grbm_select(adapt, 0, 0, 0, 0);
	oss_mutex_unlock(adapt->srbm_mutex);

	gfx_v9_0_cp_compute_enable(adapt, false);

	AMDGV_DEBUG("Skipping RLC halt\n");

	return 0;
}

/* we call gfx_v9_0_hw_fini_internal in hw_init diretcly */
static int gfx_v9_0_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_gfx_v9_0_func = {
	.name = "mi200_gfx_func",
	.is_engine = true,
	.sw_init = gfx_v9_0_sw_init,
	.sw_fini = gfx_v9_0_sw_fini,
	.hw_init = gfx_v9_0_hw_init,
	.hw_fini = gfx_v9_0_hw_fini,
};
