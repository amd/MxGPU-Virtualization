/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
 */

#include <amdgv.h>
#include <amdgv_device.h>

#include "mi300/GC/gc_9_4_3_offset.h"
#include "mi300/GC/gc_9_4_3_sh_mask.h"
#include "mi300_clockgating.h"

#include "mi300/SDMA/sdma_4_4_2_offset.h"
#include "mi300/SDMA/sdma_4_4_2_sh_mask.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;

#define for_each_sdma_inst(inst, adapt)                                                       \
	for (inst = 0; inst < (adapt)->sdma.num_instances; inst++)

static void mi300_gc_set_clock_gating_feature_flag(struct amdgv_adapter *adapt)
{
	union gc_clock_gating_support flags;
	struct gc_context *gc = adapt->cg.gc;

	oss_memset(&flags, 0, sizeof(flags));

	// set up which CG feature we are going to enable by default
	flags.bits.gc_clockgating_support_gfx_mgcg = 1;
	flags.bits.gc_clockgating_support_gfx_cgcg = 1;
	flags.bits.gc_clockgating_support_gfx_fgcg = 1;
	flags.bits.gc_clockgating_support_gfx_cgls = 1;

	gc->clock_gating_flags.u32All = flags.u32All;
}

static int mi300_gc_enter_rlc_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t data;
	int ret;

	data = RLC_SAFE_MODE__CMD_MASK;
	data = REG_SET_FIELD(data, RLC_SAFE_MODE, MESSAGE, 1);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE, data);

	ret = amdgv_wait_for_register(
		adapt, SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE),
		RLC_SAFE_MODE__CMD_MASK, 0, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG),
		AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_YIELD);

	return ret ? AMDGV_FAILURE : 0;
}

static int mi300_gc_exit_rlc_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t data;

	data = RLC_SAFE_MODE__CMD_MASK;
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_SAFE_MODE, data);

	return 0;
}

static void mi300_clockgating_mgcg_control(struct amdgv_adapter *adapt, int xcc_id,
					   bool enable)
{
	struct gc_context *gc = adapt->cg.gc;
	uint32_t data, def;

	if (enable && gc->clock_gating_flags.bits.gc_clockgating_support_gfx_mgcg) {
		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE);

		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GRBM_CGTT_SCLK_OVERRIDE, 0);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_MGCG_OVERRIDE, 0);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, RLC_CGTT_SCLK_OVERRIDE, 0);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_MGLS_OVERRIDE, 0);

		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE,
				     data);

	} else {
		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE);

		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GRBM_CGTT_SCLK_OVERRIDE, 1);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_MGCG_OVERRIDE, 1);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, RLC_CGTT_SCLK_OVERRIDE, 1);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_MGLS_OVERRIDE, 1);

		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE,
				     data);
	}
}

static void mi300_clockgating_cgcg_control(struct amdgv_adapter *adapt, int xcc_id,
					   bool enable)
{
	struct gc_context *gc = adapt->cg.gc;
	uint32_t def, data;

	if (enable && gc->clock_gating_flags.bits.gc_clockgating_support_gfx_cgcg) {
		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE);

		/* unset CGCG override */
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_CGCG_OVERRIDE, 0);

		if (gc->clock_gating_flags.bits.gc_clockgating_support_gfx_cgls)
			data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_CGLS_OVERRIDE,
					     0);
		else
			data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_CGLS_OVERRIDE,
					     1);

		/* update CGCG and CGLS override bits */
		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE,
				     data);

		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGCG_CGLS_CTRL);

		/* CGCG Hysteresis: 400us */
		data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGCG_GFX_IDLE_THRESHOLD, 0x2710);
		data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGCG_EN, 1);

		if (gc->clock_gating_flags.bits.gc_clockgating_support_gfx_cgls) {
			data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL,
					     CGLS_REP_COMPANSAT_DELAY, 0xf);
			data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGLS_EN, 1);
		}

		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGCG_CGLS_CTRL, data);

		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_RB_WPTR_POLL_CNTL);

		/* set IDLE_POLL_COUNT(0x33450100) */
		data = REG_SET_FIELD(data, CP_RB_WPTR_POLL_CNTL, POLL_FREQUENCY, 0x100);
		data = REG_SET_FIELD(data, CP_RB_WPTR_POLL_CNTL, IDLE_POLL_COUNT, 0x3345);

		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_RB_WPTR_POLL_CNTL, data);
	} else {
		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGCG_CGLS_CTRL);

		/* reset CGCG/CGLS bits */
		data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGCG_EN, 0);
		data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGLS_EN, 0);

		/* disable cgcg and cgls in FSM */
		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGCG_CGLS_CTRL, data);
	}
}

static void mi300_clockgating_cp_int_cntl(struct amdgv_adapter *adapt, int xcc_id, bool enable)
{
	uint32_t data;

	if (enable) {
		// enable cp internal interrupt
		data = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0));
		data |= (CP_INT_CNTL_RING0__CNTX_BUSY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__CNTX_EMPTY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__CMP_BUSY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__GFX_IDLE_INT_ENABLE_MASK);
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0), data);
	} else {
		// disable cp internal interrupt
		data = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0));
		data &= ~(CP_INT_CNTL_RING0__CNTX_BUSY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__CNTX_EMPTY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__CMP_BUSY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__GFX_IDLE_INT_ENABLE_MASK);
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0), data);
	}

}

static void mi300_clockgating_sram_fgcg(struct amdgv_adapter *adapt, int xcc_id, bool enable)
{
	struct gc_context *gc = adapt->cg.gc;
	uint32_t def, data;

	if (!gc->clock_gating_flags.bits.gc_clockgating_support_gfx_fgcg)
		return;

	def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE);

	if (enable)
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_FGCG_OVERRIDE, 0);
	else
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_FGCG_OVERRIDE, 1);

	if (def != data)
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE, data);

	def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CLK_CNTL);

	if (enable)
		data = REG_SET_FIELD(data, RLC_CLK_CNTL, RLC_SRAM_CLK_GATER_OVERRIDE, 0);
	else
		data = REG_SET_FIELD(data, RLC_CLK_CNTL, RLC_SRAM_CLK_GATER_OVERRIDE, 1);

	if (def != data)
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CLK_CNTL, data);
}

static void mi300_clockgating_repeater_fgcg(struct amdgv_adapter *adapt, int xcc_id,
					    bool enable)
{
	struct gc_context *gc = adapt->cg.gc;
	uint32_t def, data;

	if (!gc->clock_gating_flags.bits.gc_clockgating_support_gfx_fgcg)
		return;

	def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE);

	if (enable)
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_REP_FGCG_OVERRIDE, 0);
	else
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_REP_FGCG_OVERRIDE, 1);

	if (def != data)
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE, data);
}

static int mi300_gc_control_power_features(struct amdgv_adapter *adapt, int xcc_id,
					   bool enable)
{
	int ret;

	if (enable)
		mi300_clockgating_cp_int_cntl(adapt, xcc_id, enable);

	ret = mi300_gc_enter_rlc_safe_mode(adapt, xcc_id);
	if (ret)
		return ret;

	if (enable) {
		mi300_clockgating_sram_fgcg(adapt, xcc_id, enable);
		mi300_clockgating_repeater_fgcg(adapt, xcc_id, enable);
		mi300_clockgating_mgcg_control(adapt, xcc_id, enable);
		mi300_clockgating_cgcg_control(adapt, xcc_id, enable);
	} else {
		mi300_clockgating_cgcg_control(adapt, xcc_id, enable);
		mi300_clockgating_mgcg_control(adapt, xcc_id, enable);
		mi300_clockgating_sram_fgcg(adapt, xcc_id, enable);
		mi300_clockgating_repeater_fgcg(adapt, xcc_id, enable);
	}

	ret = mi300_gc_exit_rlc_safe_mode(adapt, xcc_id);
	if (!enable)
		mi300_clockgating_cp_int_cntl(adapt, xcc_id, enable);
	if (ret)
		return ret;

	return ret;
}

static int mi300_clockgating_sw_init(struct amdgv_adapter *adapt)
{
	struct gc_context *gc;

	gc = oss_zalloc(sizeof(struct gc_context));
	if (!gc) {
		AMDGV_ERROR("Failed to alloc memory for gc context\n");
		return AMDGV_FAILURE;
	}

	adapt->cg.gc = gc;

	mi300_gc_set_clock_gating_feature_flag(adapt);

	return 0;
}

static int mi300_clockgating_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->cg.gc) {
		oss_free(adapt->cg.gc);
		adapt->cg.gc = NULL;
	}

	return 0;
}

static int mi300_sdma_clockgating_set_mgcg(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t inst, def, val, mask;

	mask = SDMA_CLK_CTRL__SOFT_OVERRIDE0_MASK | SDMA_CLK_CTRL__SOFT_OVERRIDE1_MASK |
	       SDMA_CLK_CTRL__SOFT_OVERRIDE2_MASK | SDMA_CLK_CTRL__SOFT_OVERRIDE3_MASK |
	       SDMA_CLK_CTRL__SOFT_OVERRIDE4_MASK | SDMA_CLK_CTRL__SOFT_OVERRIDE5_MASK;

	if (enable) {
		for_each_sdma_inst (inst, adapt) {
			def = val =
				RREG32_SOC15(SDMA0, GET_INST(SDMA0, inst), regSDMA_CLK_CTRL);
			val &= ~mask;
			if (def != val)
				WREG32_SOC15(SDMA0, GET_INST(SDMA0, inst), regSDMA_CLK_CTRL,
					     val);
		}
	} else {
		for_each_sdma_inst (inst, adapt) {
			def = val =
				RREG32_SOC15(SDMA0, GET_INST(SDMA0, inst), regSDMA_CLK_CTRL);
			val |= mask;
			if (def != val)
				WREG32_SOC15(SDMA0, GET_INST(SDMA0, inst), regSDMA_CLK_CTRL,
					     val);
		}
	}

	return 0;
}

static int mi300_sdma_clockgating_set_mgls(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t inst, def, val;

	if (enable) {
		for_each_sdma_inst (inst, adapt) {
			def = val =
				RREG32_SOC15(SDMA0, GET_INST(SDMA0, inst), regSDMA_POWER_CNTL);
			val = REG_SET_FIELD(val, SDMA_POWER_CNTL, MEM_POWER_OVERRIDE, 1);
			if (def != val)
				WREG32_SOC15(SDMA0, GET_INST(SDMA0, inst), regSDMA_POWER_CNTL,
					     val);
		}
	} else {
		for_each_sdma_inst (inst, adapt) {
			def = val =
				RREG32_SOC15(SDMA0, GET_INST(SDMA0, inst), regSDMA_POWER_CNTL);
			val = REG_SET_FIELD(val, SDMA_POWER_CNTL, MEM_POWER_OVERRIDE, 0);
			if (def != val)
				WREG32_SOC15(SDMA0, GET_INST(SDMA0, inst), regSDMA_POWER_CNTL,
					     val);
		}
	}

	return 0;
}

static int mi300_sdma_clockgating_control(struct amdgv_adapter *adapt, bool enable)
{
	int ret;

	ret = mi300_sdma_clockgating_set_mgcg(adapt, enable);
	if (ret)
		return ret;

	ret = mi300_sdma_clockgating_set_mgls(adapt, enable);
	if (ret)
		return ret;

	return ret;
}

int mi300_clockgating_hw_init(struct amdgv_adapter *adapt)
{
	int ret, i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		ret = mi300_gc_control_power_features(adapt, i, true);
		if (ret)
			return ret;
	}

	ret = mi300_sdma_clockgating_control(adapt, true);
	if (ret)
		return ret;

	return 0;
}

static int mi300_clockgating_hw_fini(struct amdgv_adapter *adapt)
{
	int ret, i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		ret = mi300_gc_control_power_features(adapt, i, false);
		if (ret)
			return ret;
	}

	ret = mi300_sdma_clockgating_control(adapt, false);
	if (ret)
		return ret;

	return 0;
}

struct amdgv_init_func mi300_clockgating_func = {
	.name = "mi300_clockgating_func",
	.sw_init = mi300_clockgating_sw_init,
	.sw_fini = mi300_clockgating_sw_fini,
	.hw_init = mi300_clockgating_hw_init,
	.hw_fini = mi300_clockgating_hw_fini,
};
