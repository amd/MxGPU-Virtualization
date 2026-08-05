/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_misc.h"
#include "amdgv_nbio.h"
#include "amdgv_gfx.h"
#include "amdgv_ras.h"
#include "amdgv_sdma.h"

#include "asic_reg/GC/gc_12_1_0_offset.h"
#include "asic_reg/GC/gc_12_1_0_sh_mask.h"

#include "sdma_v7_1.h"

#define for_each_sdma_inst(inst, adapt) \
	for (inst = 0; inst < (adapt)->sdma.num_sdma_rings; inst++)

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static uint32_t sdma_v7_1_get_reg_offset(struct amdgv_adapter *adapt,
					uint32_t instance, uint32_t internal_offset)
{
	uint32_t base;
	uint32_t dev_inst = GET_INST(SDMA0, instance);
	int xcc_id = adapt->sdma.sdma_ring[instance].xcc_id;
	int xcc_inst = dev_inst % (adapt->sdma.num_instances / adapt->mcp.gfx.num_xcc);

	if (internal_offset >= SDMA0_SDMA_IDX_0_END) {
		base = adapt->reg_offset[GC_HWIP][xcc_id][1];
		if (xcc_inst != 0)
			internal_offset += SDMA1_HYP_DEC_REG_OFFSET * xcc_inst;
	} else {
		base = adapt->reg_offset[GC_HWIP][xcc_id][0];
		if (xcc_inst != 0)
			internal_offset += SDMA1_REG_OFFSET * xcc_inst;
	}

	return base + internal_offset;
}

static void sdma_v7_1_program_golden_settings(struct amdgv_adapter *adapt)
{
	int i;
	uint32_t sdma_cntl;
	uint32_t rb_cntl;

	for (i = 0; i < adapt->sdma.num_instances; i++) {
		/* Read-modify-write to preserve HW default bits (aligned with upstream) */
		sdma_cntl = RREG32(sdma_v7_1_get_reg_offset(adapt, i, regSDMA0_SDMA_CNTL));
		sdma_cntl = REG_SET_FIELD(sdma_cntl, SDMA0_SDMA_CNTL, TRAP_ENABLE, 1);
		sdma_cntl = REG_SET_FIELD(sdma_cntl, SDMA0_SDMA_CNTL, CTXEMPTY_INT_ENABLE, 1);
		WREG32(sdma_v7_1_get_reg_offset(adapt, i, regSDMA0_SDMA_CNTL), sdma_cntl);

		rb_cntl = RREG32(sdma_v7_1_get_reg_offset(adapt, i, regSDMA0_SDMA_QUEUE0_RB_CNTL));
		rb_cntl = REG_SET_FIELD(rb_cntl, SDMA0_SDMA_QUEUE0_RB_CNTL, RB_ENABLE, 1);
		rb_cntl = REG_SET_FIELD(rb_cntl, SDMA0_SDMA_QUEUE0_RB_CNTL, RB_SIZE, 11);
		rb_cntl = REG_SET_FIELD(rb_cntl, SDMA0_SDMA_QUEUE0_RB_CNTL, RPTR_WRITEBACK_ENABLE, 1);
		rb_cntl = REG_SET_FIELD(rb_cntl, SDMA0_SDMA_QUEUE0_RB_CNTL, RPTR_WRITEBACK_TIMER, 4);
		WREG32(sdma_v7_1_get_reg_offset(adapt, i, regSDMA0_SDMA_QUEUE0_RB_CNTL), rb_cntl);

		WREG32(sdma_v7_1_get_reg_offset(adapt, i, regSDMA0_SDMA_UTCL1_TIMEOUT), 0x80);
	}
}


const struct amdgv_sdma_ras_funcs sdma_v7_1_ras_funcs = {
	.err_cnt_init = NULL,
	.query_ras_error_count = NULL,
	.reset_ras_error_count = NULL,
};

void sdma_v7_1_set_ras_funcs(struct amdgv_adapter *adapt)
{
	adapt->sdma.num_inst_per_aid = adapt->sdma.num_instances / adapt->mcp.num_aid;
	adapt->sdma.funcs = &sdma_v7_1_ras_funcs;
}

static int sdma_v7_1_sw_init(struct amdgv_adapter *adapt)
{
	int i;

	sdma_v7_1_set_ras_funcs(adapt);

	adapt->doorbell_index.sdma_doorbell_range = 20;
	for (i = 0; i < adapt->sdma.num_instances; i++) {
		adapt->doorbell_index.sdma_engine[i] =
			AMDGPU_DOORBELL_SDMA_ENGINE_START +
			i * (adapt->doorbell_index.sdma_doorbell_range >> 1);
	}

	adapt->sdma.num_sdma_rings = 0;

	return 0;
}

static int sdma_v7_1_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int sdma_v7_1_pf_sdma_hw_init(struct amdgv_adapter *adapt)
{
	if (adapt->flags & AMDGV_FLAG_USE_PF)
		amdgv_sched_context_load(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);

	if (adapt->flags & AMDGV_FLAG_USE_PF)
		amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);

	return 0;
}

static int sdma_v7_1_hw_init(struct amdgv_adapter *adapt)
{
	/* sdma doorbell should be programmed from host on SOC */
	amdgv_nbio_sdma_doorbell_range(adapt, 0, true,
		adapt->doorbell_index.sdma_engine[0] << 1,
		adapt->doorbell_index.sdma_doorbell_range * adapt->sdma.num_instances);

	sdma_v7_1_program_golden_settings(adapt);

	if (sdma_v7_1_pf_sdma_hw_init(adapt)) {
		AMDGV_ERROR("PF SDMA hw init failed\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int sdma_v7_1_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func sdma_v7_1_func = {
	.name = "sdma_v7_1_func",
	.is_engine = true,
	.sw_init = sdma_v7_1_sw_init,
	.sw_fini = sdma_v7_1_sw_fini,
	.hw_init = sdma_v7_1_hw_init,
	.hw_fini = sdma_v7_1_hw_fini,
	.hw_live_init = NULL,
};

int sdma_v7_1_hw_resume(struct amdgv_adapter *adapt, uint32_t xcc_mask)
{
	if (adapt->num_vf != 1) {
		/* sdma v7_1 is not fully implemented. The ring <-> xcc_id mapping is missing */
		AMDGV_ERROR("MultiVF PF SDMA resume not implemented!\n");
		return AMDGV_FAILURE;
	}

	return sdma_v7_1_hw_init(adapt);
}

int sdma_v7_1_hw_suspend(struct amdgv_adapter *adapt, uint32_t xcc_mask)
{
	if (adapt->num_vf != 1) {
		/* sdma v7_1 is not fully implemented. The ring <-> xcc_id mapping is missing */
		AMDGV_ERROR("MultiVF PF SDMA suspend not implemented!\n");
		return AMDGV_FAILURE;
	}

	/* SDMA goes into hard reset, same as regular HW fini*/
	return sdma_v7_1_hw_fini(adapt);
}