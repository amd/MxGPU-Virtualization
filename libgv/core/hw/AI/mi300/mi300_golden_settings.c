/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include <mi300/SDMA/sdma_4_4_2_offset.h>
#include <mi300/SDMA/sdma_4_4_2_sh_mask.h>

#include "mi300/GC/gc_9_4_3_offset.h"
#include "mi300/GC/gc_9_4_3_sh_mask.h"

#include "mi300/SMUIO/smuio_13_0_3_offset.h"
#include "mi300/SMUIO/smuio_13_0_3_sh_mask.h"

#include "mi300_gfx.h"
#include "mi300_golden_settings.h"

#define GOLDEN_GB_ADDR_CONFIG 0x2a114042

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

void mi300_sdma_program_golden_settings(struct amdgv_adapter *adapt)
{
	uint32_t val;
	int i;

	for (i = 0; i < adapt->sdma.num_instances; i++) {
		val = RREG32_SOC15(SDMA0, GET_INST(SDMA0, i), regSDMA_GB_ADDR_CONFIG);
		val = REG_SET_FIELD(val, SDMA_GB_ADDR_CONFIG, NUM_BANKS, 4);
		val = REG_SET_FIELD(val, SDMA_GB_ADDR_CONFIG, PIPE_INTERLEAVE_SIZE, 0);
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, i), regSDMA_GB_ADDR_CONFIG, val);

		val = RREG32_SOC15(SDMA0, GET_INST(SDMA0, i), regSDMA_GB_ADDR_CONFIG_READ);
		val = REG_SET_FIELD(val, SDMA_GB_ADDR_CONFIG_READ, NUM_BANKS, 4);
		val = REG_SET_FIELD(val, SDMA_GB_ADDR_CONFIG_READ, PIPE_INTERLEAVE_SIZE, 0);
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, i), regSDMA_GB_ADDR_CONFIG_READ, val);

		WREG32_SOC15(SDMA0, GET_INST(SDMA0, i), regSDMA_CNTL, 0x10044783);
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, i), regSDMA_GFX_RB_CNTL, 0x41017);
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, i), regSDMA_UTCL1_TIMEOUT, 0x00800080);
	}

	return;
}

void mi300_gfx_program_golden_settings(struct amdgv_adapter *adapt)
{
	uint32_t val;
	int i;
	uint32_t cp_debug; /* interrupt_status */
	uint32_t cp_cpc_debug;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		WREG32_SOC15(GC, GET_INST(GC, i), regGB_ADDR_CONFIG, GOLDEN_GB_ADDR_CONFIG);

		if (IP_VERSION_SUBREV(adapt->ip_versions[GC_HWIP][GET_INST(GC, i)]) > 0) {
			val = RREG32_SOC15(GC, GET_INST(GC, i), regTCP_UTCL1_CNTL2);
			val = REG_SET_FIELD(val, TCP_UTCL1_CNTL2, SPARE, 1);
			WREG32_SOC15(GC, GET_INST(GC, i), regTCP_UTCL1_CNTL2, val);
		} else {
			val = RREG32_SOC15(GC, GET_INST(GC, i), regTCP_UTCL1_CNTL1);
			val = REG_SET_FIELD(val, TCP_UTCL1_CNTL1, REDUCE_FIFO_DEPTH_BY_2, 2);
			WREG32_SOC15(GC, GET_INST(GC, i), regTCP_UTCL1_CNTL1, val);
		}

		/* cp debug, it prevents page fault. */
		cp_debug = RREG32_SOC15(GC, GET_INST(GC, i), regCP_DEBUG);
		cp_debug = cp_debug | 0x8000;
		WREG32_SOC15(GC, GET_INST(GC, i), regCP_DEBUG, cp_debug);

		if (mi300_gfx_is_symetric_cu(adapt, i)) {
			cp_cpc_debug = RREG32_SOC15(GC, GET_INST(GC, i), regCP_CPC_DEBUG);
			cp_cpc_debug = REG_SET_FIELD(cp_cpc_debug, CP_CPC_DEBUG, CPC_HARVESTING_RELAUNCH_DISABLE, 1);
			cp_cpc_debug = REG_SET_FIELD(cp_cpc_debug, CP_CPC_DEBUG, CPC_HARVESTING_DISPATCH_DISABLE, 1);
			WREG32_SOC15(GC, GET_INST(GC, i), regCP_CPC_DEBUG, cp_cpc_debug);
		}
	}

	return;
}
