/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include "amdgv_clockgating.h"

#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include "navi32_clockgating.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;

static void navi32_gc_set_clock_gating_feature_flag(struct amdgv_adapter *adapt)
{
	union gc_clock_gating_support flags;
	struct gc_context *gc = adapt->cg.gc;

	oss_memset(&flags, 0, sizeof(flags));
	// set up which CG feature we are going to enable by default
	flags.bits.gc_clockgating_support_gfx_mgcg = 1;
	flags.bits.gc_clockgating_support_gfx_cgcg = 1;
	flags.bits.gc_clockgating_support_gfx_cgls = 1;
	flags.bits.gc_clockgating_support_gfx_3d_cgcg = 0;
	flags.bits.gc_clockgating_support_gfx_3d_cgls = 0;
	flags.bits.gc_clockgating_support_gfx_sram_fgcg = 1;
	flags.bits.gc_clockgating_support_gfx_repeater_fgcg = 1;
	flags.bits.gc_clockgating_support_gfx_perf_clk = 1;

	gc->clock_gating_flags.u32All = flags.u32All;
}

static void navi32_gc_set_supported_features(struct amdgv_adapter *adapt)
{
	struct gc_context *gc = adapt->cg.gc;

	gc->supported_features.bitfields.gc_lbpw_enabled = 0;
}

static void navi32_gc_update_coarse_grain_clock_gating(struct amdgv_adapter *adapt,
						      union gc_clock_gating_support flags,
						      bool enable)
{
	uint32_t reg_default = 0;
	uint32_t reg_data = 0;

	if (!enable) {
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL));
		reg_data = reg_default;
		// Reset CGCG/CGLS bits
		if (flags.bits.gc_clockgating_support_gfx_cgcg) {
			reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL, CGCG_EN, 0);
		}
		if (flags.bits.gc_clockgating_support_gfx_cgls) {
			reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL, CGLS_EN, 0);
		}
		// Disable CGCG/CGLS in FSM
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL), reg_data);

		// Disable SDMA CGCG
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_RLC_CGCG_CTRL));
		reg_data = reg_default;
		reg_data =
			REG_SET_FIELD(reg_data, SDMA0_RLC_CGCG_CTRL, CGCG_INT_ENABLE, 0x0);
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_RLC_CGCG_CTRL), reg_data);

		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_RLC_CGCG_CTRL));
		reg_data = reg_default;
		reg_data =
			REG_SET_FIELD(reg_data, SDMA1_RLC_CGCG_CTRL, CGCG_INT_ENABLE, 0x0);
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_RLC_CGCG_CTRL), reg_data);
	} else {
		// Clear CGCG/CGLS OV
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE));
		reg_data = reg_default;
		if (flags.bits.gc_clockgating_support_gfx_cgcg) {
			reg_data = REG_SET_FIELD(reg_data, RLC_CGTT_MGCG_OVERRIDE,
						 GFXIP_CGCG_OVERRIDE, 0);
		}
		if (flags.bits.gc_clockgating_support_gfx_cgls) {
			reg_data = REG_SET_FIELD(reg_data, RLC_CGTT_MGCG_OVERRIDE,
						 GFXIP_CGLS_OVERRIDE, 0);
		}
		// Update CGCG/CGLS OV bits
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE), reg_data);
		// Enable CGCG/CGLS FSM (0x0000363F)
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL));
		reg_data = 0;
		if (flags.bits.gc_clockgating_support_gfx_cgcg) {
			reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL,
						 CGCG_GFX_IDLE_THRESHOLD, 0x36);
			reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL, CGCG_EN, 1);
		}
		if (flags.bits.gc_clockgating_support_gfx_cgls) {
			reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL,
						 CGLS_REP_COMPANSAT_DELAY, 0x000F);
			reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL, CGLS_EN, 1);
		}
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL), reg_data);

		// Set IDLE_POOL_COUNT (0x00900100)
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB_WPTR_POLL_CNTL));
		reg_data = reg_default;
		reg_data =
			REG_SET_FIELD(reg_data, CP_RB_WPTR_POLL_CNTL, POLL_FREQUENCY, 0x0100);
		reg_data =
			REG_SET_FIELD(reg_data, CP_RB_WPTR_POLL_CNTL, IDLE_POLL_COUNT, 0x0090);
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB_WPTR_POLL_CNTL), reg_data);

		// Enable SDMA CGCG
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_RLC_CGCG_CTRL));
		reg_data = reg_default;
		reg_data =
			REG_SET_FIELD(reg_data, SDMA0_RLC_CGCG_CTRL, CGCG_INT_ENABLE, 0x1);
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_RLC_CGCG_CTRL), reg_data);

		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_RLC_CGCG_CTRL));
		reg_data = reg_default;
		reg_data =
			REG_SET_FIELD(reg_data, SDMA1_RLC_CGCG_CTRL, CGCG_INT_ENABLE, 0x1);
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_RLC_CGCG_CTRL), reg_data);
	}
}

static void navi32_gc_update_3d_clock_gating(struct amdgv_adapter *adapt,
					    union gc_clock_gating_support flags, bool enable)
{
	uint32_t reg_default = 0;
	uint32_t reg_data = 0;

	if (!enable) {
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL_3D));
		reg_data = reg_default;
		// Disable 3D cgcg, cgls
		if (flags.bits.gc_clockgating_support_gfx_3d_cgcg) {
			reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL_3D, CGCG_EN, 0);
		}
		if (flags.bits.gc_clockgating_support_gfx_3d_cgls) {
			reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL_3D, CGLS_EN, 0);
		}
		// Disable 3D cgcg and cgls in FSM
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL_3D), reg_data);
	} else {
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE));
		reg_data = reg_default;
		if (flags.bits.gc_clockgating_support_gfx_3d_cgcg) {
			reg_data = REG_SET_FIELD(reg_data, RLC_CGTT_MGCG_OVERRIDE,
						 GFXIP_GFX3D_CG_OVERRIDE, 0);
		}
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE), reg_data);
		// enable 3D cgcg FSM(0x0000363f)
		if (reg_default != reg_data) {
			reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL_3D));
			reg_data = 0;
			// 0x0020003f
			if (flags.bits.gc_clockgating_support_gfx_3d_cgcg) {
				reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL_3D,
							 CGCG_GFX_IDLE_THRESHOLD, 0x36);
				reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL_3D,
							 CGCG_EN, 1);
			}
			if (flags.bits.gc_clockgating_support_gfx_3d_cgls) {
				reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL_3D,
							 CGLS_REP_COMPANSAT_DELAY, 0x000F);
				reg_data = REG_SET_FIELD(reg_data, RLC_CGCG_CGLS_CTRL_3D,
							 CGLS_EN, 1);
			}
			if (reg_default != reg_data)
				WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL_3D),
				       reg_data);
			// Set IDLE_POLL_COUNT(0x00900100)
			reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB_WPTR_POLL_CNTL));
			reg_data = reg_default;
			reg_data = REG_SET_FIELD(reg_data, CP_RB_WPTR_POLL_CNTL,
						 POLL_FREQUENCY, 0x0100);
			reg_data = REG_SET_FIELD(reg_data, CP_RB_WPTR_POLL_CNTL,
						 IDLE_POLL_COUNT, 0x0090);
			if (reg_default != reg_data)
				WREG32(SOC15_REG_OFFSET(GC, 0, regCP_RB_WPTR_POLL_CNTL),
				       reg_data);
		}
	}
}

static void navi32_gc_update_medium_grain_clock_gating(struct amdgv_adapter *adapt,
						      union gc_clock_gating_support flags,
						      bool enable)
{
	uint32_t reg_default = 0;
	uint32_t reg_data = 0;

	if (!enable) {
		// RLC_CGTT_MGCG_OVERRIDE
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE));
		reg_data = reg_default;
		reg_data |= (RLC_CGTT_MGCG_OVERRIDE__RLC_CGTT_SCLK_OVERRIDE_MASK |
			     RLC_CGTT_MGCG_OVERRIDE__GRBM_CGTT_SCLK_OVERRIDE_MASK    |
			     RLC_CGTT_MGCG_OVERRIDE__GFXIP_MGCG_OVERRIDE_MASK);
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE), reg_data);
	} else {
		// RLC_CGTT_MGCG_OVERRIDE
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE));
		reg_data = reg_default;
		reg_data &= ~(RLC_CGTT_MGCG_OVERRIDE__GRBM_CGTT_SCLK_OVERRIDE_MASK |
			      RLC_CGTT_MGCG_OVERRIDE__GFXIP_MGCG_OVERRIDE_MASK         |
			      RLC_CGTT_MGCG_OVERRIDE__RLC_CGTT_SCLK_OVERRIDE_MASK);
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE), reg_data);
	}
}

static void navi32_gc_update_fine_grain_clock_gating(struct amdgv_adapter *adapt,
						      union gc_clock_gating_support flags,
						      bool enable)
{
	uint32_t reg_default = 0;
	uint32_t reg_data = 0;

	if (!enable) {
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE));
		reg_data = reg_default;
		if (flags.bits.gc_clockgating_support_gfx_sram_fgcg)
			reg_data |= RLC_CGTT_MGCG_OVERRIDE__GFXIP_FGCG_OVERRIDE_MASK;
		if (flags.bits.gc_clockgating_support_gfx_repeater_fgcg)
			reg_data |= RLC_CGTT_MGCG_OVERRIDE__GFXIP_REPEATER_FGCG_OVERRIDE_MASK;
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE), reg_data);
	} else {
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE));
		reg_data = reg_default;
		if (flags.bits.gc_clockgating_support_gfx_sram_fgcg)
			reg_data &= ~RLC_CGTT_MGCG_OVERRIDE__GFXIP_FGCG_OVERRIDE_MASK;
		if (flags.bits.gc_clockgating_support_gfx_repeater_fgcg)
			reg_data &= ~RLC_CGTT_MGCG_OVERRIDE__GFXIP_REPEATER_FGCG_OVERRIDE_MASK;
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE), reg_data);
	}
}

static void navi32_gc_update_perfmon_clock_state(struct amdgv_adapter *adapt,
						      union gc_clock_gating_support flags,
						      bool enable)
{
	uint32_t reg_default = 0;
	uint32_t reg_data = 0;

	if (enable) {
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE));
		reg_data = reg_default;
		reg_data |= RLC_CGTT_MGCG_OVERRIDE__PERFMON_CLOCK_STATE_MASK;
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE), reg_data);
	} else {
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE));
		reg_data = reg_default;
		reg_data &= ~RLC_CGTT_MGCG_OVERRIDE__PERFMON_CLOCK_STATE_MASK;
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGTT_MGCG_OVERRIDE), reg_data);
	}
}

static bool navi32_is_rlc_enabled(struct amdgv_adapter *adapt)
{
	uint32_t reg_data;

	reg_data = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CNTL));
	return !!(reg_data & RLC_CNTL__RLC_ENABLE_F32_MASK);
}

static int navi32_xcc_set_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	uint32_t data;
	int ret;

	data = RLC_SAFE_MODE__CMD_MASK;
	data |= (1 << RLC_SAFE_MODE__MESSAGE__SHIFT);

	/* Single-XCC GPU; xcc_id is always 0 and is intentionally unused */
	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_SAFE_MODE), data);

	ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET_NAME(GC, 0, regRLC_SAFE_MODE),
				      RLC_SAFE_MODE__CMD_MASK, 0,
				      AMDGV_TIMEOUT(TIMEOUT_STATUS_REG),
				      AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_YIELD);
	if (ret) {
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_xcc_unset_safe_mode(struct amdgv_adapter *adapt, int xcc_id)
{
	/* Single-XCC GPU; xcc_id is always 0 and is intentionally unused */
	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_SAFE_MODE), RLC_SAFE_MODE__CMD_MASK);

	return 0;
}

static const struct amdgv_rlc_funcs navi32_rlc_funcs = {
	.is_rlc_enabled = navi32_is_rlc_enabled,
	.set_safe_mode = navi32_xcc_set_safe_mode,
	.unset_safe_mode = navi32_xcc_unset_safe_mode,
};

static void navi32_gc_control_clock_gating(struct amdgv_adapter *adapt, bool enable)
{
	union gc_clock_gating_support flags = { 0 };
	uint32_t reg_data = 0;
	uint32_t reg_default = 0;
	struct gc_context *gc = adapt->cg.gc;

	flags.u32All = gc->clock_gating_flags.u32All;

	if (!enable) {
		// disable cp internal interrupt
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_INT_CNTL_RING0));
		reg_data = reg_default;
		reg_data &= ~(CP_INT_CNTL_RING0__CNTX_BUSY_INT_ENABLE_MASK |
			      CP_INT_CNTL_RING0__CNTX_EMPTY_INT_ENABLE_MASK |
			      CP_INT_CNTL_RING0__CMP_BUSY_INT_ENABLE_MASK |
			      CP_INT_CNTL_RING0__GFX_IDLE_INT_ENABLE_MASK |
			      CP_INT_CNTL_RING0__TIME_STAMP_INT_ENABLE_MASK |
			      CP_INT_CNTL_RING0__PRIV_REG_INT_ENABLE_MASK |
			      CP_INT_CNTL_RING0__PRIV_INSTR_INT_ENABLE_MASK);
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regCP_INT_CNTL_RING0), reg_data);

		// CGCG/CGLS should be disabled before MGCG/MGLS.
		// ===  CGCG + CGLS ===
		if (flags.bits.gc_clockgating_support_gfx_cgcg ||
		    flags.bits.gc_clockgating_support_gfx_cgls)
			navi32_gc_update_coarse_grain_clock_gating(adapt, flags, enable);

		// ===  CGCG /CGLS for GFX 3D Only ===
		if (flags.bits.gc_clockgating_support_gfx_3d_cgcg ||
		    flags.bits.gc_clockgating_support_gfx_3d_cgls)
			navi32_gc_update_3d_clock_gating(adapt, flags, enable);

		// ===  MGCG  ===
		if (flags.bits.gc_clockgating_support_gfx_mgcg)
			navi32_gc_update_medium_grain_clock_gating(adapt, flags, enable);

		// ===  SRAM FGCG + REPEATER FGCG  ===
		if (flags.bits.gc_clockgating_support_gfx_sram_fgcg ||
		    flags.bits.gc_clockgating_support_gfx_repeater_fgcg)
			navi32_gc_update_fine_grain_clock_gating(adapt, flags, enable);

		// ===  PERFMON CLOCK STATE  ===
		if (flags.bits.gc_clockgating_support_gfx_perf_clk)
			navi32_gc_update_perfmon_clock_state(adapt, flags, enable);
	} else {
		// ===  PERFMON CLOCK STATE  ===
		if (flags.bits.gc_clockgating_support_gfx_perf_clk)
			navi32_gc_update_perfmon_clock_state(adapt, flags, enable);

		// ===  SRAM FGCG + REPEATER FGCG  ===
		if (flags.bits.gc_clockgating_support_gfx_sram_fgcg ||
		    flags.bits.gc_clockgating_support_gfx_repeater_fgcg)
			navi32_gc_update_fine_grain_clock_gating(adapt, flags, enable);
		// CGCG/CGLS should be enabled after MGCG/MGLS
		// ===  MGCG  ===
		if (flags.bits.gc_clockgating_support_gfx_mgcg)
			navi32_gc_update_medium_grain_clock_gating(adapt, flags, enable);

		// ===  CGCG /CGLS for GFX 3D Only ===
		if (flags.bits.gc_clockgating_support_gfx_3d_cgcg ||
		    flags.bits.gc_clockgating_support_gfx_3d_cgls)
			navi32_gc_update_3d_clock_gating(adapt, flags, enable);

		// ===  CGCG + CGLS ===
		if (flags.bits.gc_clockgating_support_gfx_cgcg ||
		    flags.bits.gc_clockgating_support_gfx_cgls)
			navi32_gc_update_coarse_grain_clock_gating(adapt, flags, enable);

		// enable cp internal interrupt
		reg_default = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_INT_CNTL_RING0));
		reg_data = reg_default;
		reg_data |= (CP_INT_CNTL_RING0__CNTX_BUSY_INT_ENABLE_MASK |
			     CP_INT_CNTL_RING0__CNTX_EMPTY_INT_ENABLE_MASK |
			     CP_INT_CNTL_RING0__CMP_BUSY_INT_ENABLE_MASK |
			     CP_INT_CNTL_RING0__GFX_IDLE_INT_ENABLE_MASK |
			     CP_INT_CNTL_RING0__TIME_STAMP_INT_ENABLE_MASK |
			     CP_INT_CNTL_RING0__PRIV_REG_INT_ENABLE_MASK |
			     CP_INT_CNTL_RING0__PRIV_INSTR_INT_ENABLE_MASK);
		if (reg_default != reg_data)
			WREG32(SOC15_REG_OFFSET(GC, 0, regCP_INT_CNTL_RING0), reg_data);
	}
}

int navi32_gc_control_power_features(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t cgcg_ctrl;
	int ret;

	/* rlcv might fail to enter safe mode if clock gating is enabled
	 * NOTE: no need to restore since navi32_gc_control_clock_gating()
	 *       will configure CGCG enable/disable
	 */
	cgcg_ctrl = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL));
	cgcg_ctrl = REG_SET_FIELD(cgcg_ctrl, RLC_CGCG_CGLS_CTRL, CGCG_EN, 0);
	cgcg_ctrl = REG_SET_FIELD(cgcg_ctrl, RLC_CGCG_CGLS_CTRL, CGLS_EN, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CGCG_CGLS_CTRL), cgcg_ctrl);

	ret = amdgv_gfx_rlc_enter_safe_mode(adapt, 0);
	if (ret)
		return ret;

	navi32_gc_control_clock_gating(adapt, enable);
	ret = amdgv_gfx_rlc_exit_safe_mode(adapt, 0);

	return 0;
}

int navi32_clockgating_sw_init(struct amdgv_adapter *adapt)
{
	struct gc_context *gc = NULL;

	gc = oss_zalloc(sizeof(struct gc_context));
	if (gc == NULL) {
		AMDGV_ERROR("Failed to alloc memory for gc context\n");
		return AMDGV_FAILURE;
	}
	adapt->cg.gc = gc;
	adapt->sched.cg_control = navi32_gc_control_power_features;
	adapt->gfx.rlc.funcs = &navi32_rlc_funcs;

	navi32_gc_set_supported_features(adapt);

	return 0;
}

int navi32_clockgating_hw_init(struct amdgv_adapter *adapt)
{
	navi32_gc_set_clock_gating_feature_flag(adapt);
	if (adapt->sched.cg_control)
		adapt->sched.cg_control(adapt, true);

	return 0;
}

int navi32_clockgating_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->cg.gc != NULL) {
		oss_free(adapt->cg.gc);
		adapt->cg.gc = NULL;
	}

	return 0;
}

int navi32_clockgating_hw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->sched.cg_control)
		adapt->sched.cg_control(adapt, false);

	return 0;
}

struct amdgv_init_func navi32_clockgating_func = {
	.name = "navi32_clockgating_func",
	.sw_init = navi32_clockgating_sw_init,
	.sw_fini = navi32_clockgating_sw_fini,
	.hw_init = navi32_clockgating_hw_init,
	.hw_fini = navi32_clockgating_hw_fini,
};
