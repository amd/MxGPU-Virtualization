/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_gfx.h>

#include "gfx_v12_1.h"
#include "asic_reg/GC/gc_12_1_0_offset.h"
#include "asic_reg/GC/gc_12_1_0_sh_mask.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;

static void gfx_v12_1_gc_set_clock_gating_feature_flag(struct amdgv_adapter *adapt)
{
	struct gc_context *gc = adapt->cg.gc;

	gc->clock_gating_flags.u32All = 0;

	gc->clock_gating_flags.bits.gc_clockgating_support_gfx_mgcg = 1;
	gc->clock_gating_flags.bits.gc_clockgating_support_gfx_cgcg = 1;
	gc->clock_gating_flags.bits.gc_clockgating_support_gfx_fgcg = 1;
	gc->clock_gating_flags.bits.gc_clockgating_support_gfx_cgls = 1;
	gc->clock_gating_flags.bits.gc_clockgating_support_gfx_perf_clk = 1;
}


static void gfx_v12_1_clockgating_mgcg_control(struct amdgv_adapter *adapt, int xcc_id, bool enable)
{
	struct gc_context *gc = adapt->cg.gc;
	uint32_t data, def;

	if (!gc->clock_gating_flags.bits.gc_clockgating_support_gfx_mgcg)
		return;

	def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE);

	if (enable) {
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GRBM_CGTT_SCLK_OVERRIDE, 0);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, RLC_CGTT_SCLK_OVERRIDE, 0);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_MGCG_OVERRIDE, 0);
	} else {
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GRBM_CGTT_SCLK_OVERRIDE, 1);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_MGCG_OVERRIDE, 1);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, RLC_CGTT_SCLK_OVERRIDE, 1);
	}

	if (def != data)
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE, data);
}

static void gfx_v12_1_clockgating_cgcg_control(struct amdgv_adapter *adapt, int xcc_id, bool enable)
{
	struct gc_context *gc = adapt->cg.gc;
	uint32_t def, data;

	if (!gc->clock_gating_flags.bits.gc_clockgating_support_gfx_cgcg)
		return;

	if (enable) {
		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_CGCG_OVERRIDE, 0);

		if (gc->clock_gating_flags.bits.gc_clockgating_support_gfx_cgls)
			data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_CGLS_OVERRIDE, 0);
		else
			data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_CGLS_OVERRIDE, 1);

		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE, data);

		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGCG_CGLS_CTRL);
		data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGCG_GFX_IDLE_THRESHOLD, 0x36);
		data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGCG_EN, 1);

		if (gc->clock_gating_flags.bits.gc_clockgating_support_gfx_cgls) {
			data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL,
					     CGLS_REP_COMPANSAT_DELAY, 0xf);
			data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGLS_EN, 1);
		}

		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGCG_CGLS_CTRL, data);

		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_RB_WPTR_POLL_CNTL);
		data = REG_SET_FIELD(data, CP_RB_WPTR_POLL_CNTL, POLL_FREQUENCY, 0x100);
		data = REG_SET_FIELD(data, CP_RB_WPTR_POLL_CNTL, IDLE_POLL_COUNT, 0x0090);

		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_RB_WPTR_POLL_CNTL, data);
	} else {
		def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGCG_CGLS_CTRL);
		data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGCG_EN, 0);
		data = REG_SET_FIELD(data, RLC_CGCG_CGLS_CTRL, CGLS_EN, 0);

		if (def != data)
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGCG_CGLS_CTRL, data);
	}
}

static void gfx_v12_1_clockgating_cp_int_cntl(struct amdgv_adapter *adapt, int xcc_id, bool enable)
{
	uint32_t data;
	struct gc_context *gc = adapt->cg.gc;

	if (!gc->clock_gating_flags.u32All)
		return;

	data = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0));

	if (enable) {
		data |= (CP_INT_CNTL_RING0__CNTX_BUSY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__CNTX_EMPTY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__CMP_BUSY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__GFX_IDLE_INT_ENABLE_MASK);
	} else {
		data &= ~(CP_INT_CNTL_RING0__CNTX_BUSY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__CNTX_EMPTY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__CMP_BUSY_INT_ENABLE_MASK |
			CP_INT_CNTL_RING0__GFX_IDLE_INT_ENABLE_MASK);
		}

	WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_INT_CNTL_RING0), data);
}

static void gfx_v12_1_clockgating_sram_fgcg(struct amdgv_adapter *adapt, int xcc_id, bool enable)
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
}

static void gfx_v12_1_clockgating_repeater_fgcg(struct amdgv_adapter *adapt, int xcc_id,
					    bool enable)
{
	struct gc_context *gc = adapt->cg.gc;
	uint32_t def, data;

	if (!gc->clock_gating_flags.bits.gc_clockgating_support_gfx_fgcg)
		return;

	def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE);

	if (enable) {
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_REPEATER_FGCG_OVERRIDE, 0);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, RLC_REPEATER_FGCG_OVERRIDE, 0);
	} else {
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, GFXIP_REPEATER_FGCG_OVERRIDE, 1);
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, RLC_REPEATER_FGCG_OVERRIDE, 1);
	}

	if (def != data)
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE, data);
}

static void gfx_v12_1_clockgating_perfmon(struct amdgv_adapter *adapt,  int xcc_id,
					  bool enable)
{
	struct gc_context *gc = adapt->cg.gc;
	uint32_t def, data;

	if (!gc->clock_gating_flags.bits.gc_clockgating_support_gfx_perf_clk)
		return;

	def = data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE);

	if (enable)
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, PERFMON_CLOCK_STATE, 0);
	else
		data = REG_SET_FIELD(data, RLC_CGTT_MGCG_OVERRIDE, PERFMON_CLOCK_STATE, 1);

	if (def != data)
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CGTT_MGCG_OVERRIDE, data);
}

static int gfx_v12_1_gc_control_power_features(struct amdgv_adapter *adapt, int xcc_id,
					   bool enable)
{
	struct gc_context *gc = adapt->cg.gc;
	int ret;

	if (!gc->clock_gating_flags.u32All)
		return 0;

	ret = amdgv_gfx_rlc_enter_safe_mode(adapt, xcc_id);
	if (ret)
		return ret;

	gfx_v12_1_clockgating_cgcg_control(adapt, xcc_id, enable);
	gfx_v12_1_clockgating_mgcg_control(adapt, xcc_id, enable);
	gfx_v12_1_clockgating_repeater_fgcg(adapt, xcc_id, enable);
	gfx_v12_1_clockgating_sram_fgcg(adapt, xcc_id, enable);
	gfx_v12_1_clockgating_perfmon(adapt, xcc_id, enable);
	gfx_v12_1_clockgating_cp_int_cntl(adapt, xcc_id, enable);

	ret = amdgv_gfx_rlc_exit_safe_mode(adapt, xcc_id);
	if (ret)
		AMDGV_WARN("Failed to exit RLC safe mode on xcc %d\n", xcc_id);

	return 0;
}

static int gfx_v12_1_clockgating_sw_init(struct amdgv_adapter *adapt)
{
	struct gc_context *gc;

	gc = oss_zalloc(sizeof(struct gc_context));
	if (!gc) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct gc_context));
		return AMDGV_FAILURE;
	}

	adapt->cg.gc = gc;

	gfx_v12_1_gc_set_clock_gating_feature_flag(adapt);

	return 0;
}

static int gfx_v12_1_clockgating_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->cg.gc) {
		oss_free(adapt->cg.gc);
		adapt->cg.gc = NULL;
	}

	return 0;
}

static int gfx_v12_1_clockgating_hw_init(struct amdgv_adapter *adapt)
{
	int ret;
	uint32_t xcc_id;

	for (xcc_id = 0; xcc_id < adapt->mcp.gfx.num_xcc; xcc_id++) {
		ret = gfx_v12_1_gc_control_power_features(adapt, xcc_id, true);
		if (ret)
			return ret;
	}

	return 0;
}

static int gfx_v12_1_clockgating_hw_fini(struct amdgv_adapter *adapt)
{
	int ret;
	uint32_t xcc_id;

	for (xcc_id = 0; xcc_id < adapt->mcp.gfx.num_xcc; xcc_id++) {
		ret = gfx_v12_1_gc_control_power_features(adapt, xcc_id, false);
		if (ret)
			return ret;
	}

	return 0;
}

struct amdgv_init_func gfx_v12_1_clockgating_func = {
	.name = "gfx_v12_1_clockgating_func",
	.sw_init = gfx_v12_1_clockgating_sw_init,
	.sw_fini = gfx_v12_1_clockgating_sw_fini,
	.hw_init = gfx_v12_1_clockgating_hw_init,
	.hw_fini = gfx_v12_1_clockgating_hw_fini,
};

int gfx_v12_1_set_clockgating_state(struct amdgv_adapter *adapt, bool enable)
{
	int ret;
	uint32_t xcc_id;

	for (xcc_id = 0; xcc_id < adapt->mcp.gfx.num_xcc; xcc_id++) {
		ret = gfx_v12_1_gc_control_power_features(adapt, xcc_id, enable);
		if (ret)
			return ret;
	}

	return 0;
}
