/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_CLOCKGATING_H
#define AMDGV_CLOCKGATING_H

#include "amdgv_basetypes.h"

union gc_supported_features {
	struct {
		uint32_t gc_lbpw_enabled : 1;
		uint32_t reserved	 : 31;
	} bitfields, bits;
	uint32_t u32All;
};

union gc_system_flags {
	struct {
		uint32_t gc_sysflag_rlc_safe_mode_entered : 1;
		uint32_t gc_sysflag_smu_loaded		  : 1;
		uint32_t gc_sysflag_is_zfb		  : 1;
		uint32_t reserved			  : 29;
	} bitfields, bits;
	uint32_t u32All;
};

union gc_clock_gating_support {
	struct {
		uint32_t gc_clockgating_support_gfx_mgcg	 : 1;
		uint32_t gc_clockgating_support_gfx_mgls	 : 1;
		uint32_t gc_clockgating_support_gfx_cgcg	 : 1;
		uint32_t gc_clockgating_support_gfx_fgcg	 : 1;
		uint32_t gc_clockgating_support_gfx_cgls	 : 1;
		uint32_t gc_clockgating_support_gfx_cgts	 : 1;
		uint32_t gc_clockgating_support_gfx_cgts_ls	 : 1;
		uint32_t gc_clockgating_support_gfx_cp_ls	 : 1;
		uint32_t gc_clockgating_support_gfx_rlc_ls	 : 1;
		uint32_t gc_clockgating_support_gfx_3d_cgcg	 : 1;
		uint32_t gc_clockgating_support_gfx_3d_cgls	 : 1;
		uint32_t gc_clockgating_support_gfx_mgcg_perfmon : 1;
		uint32_t gc_clockgating_support_gfx_sram_fgcg    : 1;
		uint32_t gc_clockgating_support_gfx_repeater_fgcg: 1;
		uint32_t gc_clockgating_support_gfx_perf_clk     : 1;
		uint32_t reserved				 : 18;
	} bitfields, bits;
	uint32_t u32All;
};

struct gc_context {
	union gc_supported_features supported_features;
	union gc_system_flags system_flags;
	union gc_clock_gating_support clock_gating_flags;
	uint32_t rlc_safe_count;
};

// clock gating context
struct amdgv_cg {
	struct gc_context *gc;
};
#endif
