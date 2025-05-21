/*
 * Copyright (C) 2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include <amdgv_device.h>
#include <amdgv.h>
#include <navi32_device.h>
#include "navi32_reg_inc.h"

void navi32_reg_base_init(struct amdgv_adapter *adapt)
{
	uint32_t i;

	/*
	 * HW has more IP blocks,  only NBIO and SMUIO are init
	 * here, the rest will be discovered by VBIOS parsing the
	 * harvesting tables
	 *
	 * HOWEVER, need MP0 and MP1 because they are not available
	 * before VBIOS post (IP Discovery).
	 * Need MP0 and MP1 for GIM reload to do MODEx RESET and BACO-IN/OUT
	 */
	for (i = 0; i < MAX_INSTANCE; ++i) {
		adapt->reg_offset[NBIO_HWIP][i] = (uint32_t *)(&(NBIO_BASE.instance[i]));
		adapt->reg_offset[SMUIO_HWIP][i] = (uint32_t *)(&(SMUIO_BASE.instance[i]));
		adapt->reg_offset[MP0_HWIP][i] = (uint32_t *)(&(MP0_BASE.instance[i]));
		adapt->reg_offset[MP1_HWIP][i] = (uint32_t *)(&(MP1_BASE.instance[i]));
		adapt->reg_offset[GC_HWIP][i] = (uint32_t *)(&(GC_BASE.instance[i]));
		adapt->reg_offset[MMHUB_HWIP][i] = (uint32_t *)(&(MMHUB_BASE.instance[i]));
		adapt->reg_offset[ATHUB_HWIP][i] = (uint32_t *)(&(ATHUB_BASE.instance[i]));
		adapt->reg_offset[OSSSYS_HWIP][i] = (uint32_t *)(&(OSSSYS_BASE.instance[i]));
		adapt->reg_offset[HDP_HWIP][i] = (uint32_t *)(&(HDP_BASE.instance[i]));
		adapt->reg_offset[LSDMA_HWIP][i] = (uint32_t *)(&(LSDMA_BASE.instance[i]));
	}
}

// Each offset means 8-bytes from pcie doorbell base, total 0x200 items, 0x200*8=4KB
// Source:  ttl\src\dev\doorbell\ttl_doorbell_range_nv3.h
enum DOORBELL_RANGE_ASSIGNMENT_NV3 {
    COMPUTE_START_OFFSET           = 0,
    COMPUTE_NUM_ENTRIES            = 128,

    // Compute range is getting shared by swGC and swMES.
    // TTL needs to segment this internally so the two SWIPs' doorbell usage won't overlap
	// [0x000 .. 0x001)
    COMPUTE_KIQ_START_OFFSET       = COMPUTE_START_OFFSET,
    COMPUTE_KIQ_NUM_ENTRIES        = 1,

	// [0x001 .. 0x080)
    COMPUTE_MES_SCH_START_OFFSET   = COMPUTE_KIQ_START_OFFSET + COMPUTE_KIQ_NUM_ENTRIES,
    COMPUTE_MES_SCH_NUM_ENTRIES    = COMPUTE_NUM_ENTRIES - COMPUTE_KIQ_NUM_ENTRIES,

	// [0x080 .. 0x100)
    GFX_START_OFFSET               = COMPUTE_MES_SCH_START_OFFSET + COMPUTE_MES_SCH_NUM_ENTRIES,
    GFX_NUM_ENTRIES                = 128,

	// [0x100 .. 0x110)
    SDMA_START_OFFSET              = GFX_START_OFFSET + GFX_NUM_ENTRIES,
    // should have 16 slots, 1 slots for 1 queue. however due to the following known issue,
	// we temparorily use 8, will correct it after hw issue fixed.
    // We have 2 SDMA instances inSDMA6.0, each SDMA instance has 8 queues, so we need 16
	// doorbell slots totally,as the field SIZE of BIF_CSDMA_DOORBELL_RANGE is in unit of DWORD,
    // but our doorbell is in 64bit doorbell mode, so maxsize should be 32, which exceed the
	// current SIZE bits(5bits),Software side and hardware side has decideto enlarge the SIZE
    // field to 8bits. Before that, we will use a small SIZE tobring up the model.
    SDMA_NUM_ENTRIES               = 16,

	// [0x110 .. 0x178)
    RESERVED0_START_OFFSET         = SDMA_START_OFFSET + SDMA_NUM_ENTRIES,
    RESERVED0_NUM_ENTRIES          = 104,

	// [0x178 .. 0x188)
    IH_START_OFFSET                = RESERVED0_START_OFFSET + RESERVED0_NUM_ENTRIES,
    IH_NUM_ENTRIES                 = 16,

	// [0x188 .. 0x198)
    VCN_INST0_START_OFFSET         = IH_START_OFFSET + IH_NUM_ENTRIES,
    VCN_INST0_NUM_ENTRIES          = 16,

	// [0x198 .. 0x1A8)
    VCN_INST1_START_OFFSET         = VCN_INST0_START_OFFSET + VCN_INST0_NUM_ENTRIES,
    VCN_INST1_NUM_ENTRIES          = 16,

    // JPEG is within the range of VCN0/1, which programs mmBIF_MMSCH0_DOORBELL_RANGE
	// with VCN_INST0_NUM_ENTRIES and VCN_INST1_NUM_ENTRIES
	// [0x188 .. 0x198)
    JPEG_START_OFFSET              = VCN_INST0_START_OFFSET,
    JPEG_NUM_ENTRIES               = VCN_INST0_NUM_ENTRIES,

	// [0x188 .. 0x198)
    UMS_START_OFFSET               = VCN_INST0_START_OFFSET,
    UMS_NUM_ENTRIES                = VCN_INST0_NUM_ENTRIES,

	// [0x198 .. 0x1F0)
    RESERVED1_START_OFFSET         = UMS_START_OFFSET + UMS_NUM_ENTRIES,
    RESERVED1_NUM_ENTRIES          = 88,
};
static int navi32_doorbell_index_init(struct amdgv_adapter *adapt)
{
	adapt->doorbell_index.kiq = COMPUTE_KIQ_START_OFFSET;
	adapt->doorbell_index.mec_ring0 = COMPUTE_START_OFFSET + 0x10;
	adapt->doorbell_index.mec_ring1 = COMPUTE_START_OFFSET + 0x11;
	adapt->doorbell_index.mec_ring2 = COMPUTE_START_OFFSET + 0x12;
	adapt->doorbell_index.mec_ring3 = COMPUTE_START_OFFSET + 0x13;
	adapt->doorbell_index.mec_ring4 = COMPUTE_START_OFFSET + 0x14;
	adapt->doorbell_index.mec_ring5 = COMPUTE_START_OFFSET + 0x15;
	adapt->doorbell_index.mec_ring6 = COMPUTE_START_OFFSET + 0x16;
	adapt->doorbell_index.mec_ring7 = COMPUTE_START_OFFSET + 0x17;
	adapt->doorbell_index.userqueue_start =
			COMPUTE_START_OFFSET + 0x20;
	adapt->doorbell_index.userqueue_end =
			COMPUTE_START_OFFSET + 70;
	adapt->doorbell_index.gfx_ring0 = GFX_START_OFFSET;
	adapt->doorbell_index.sdma_engine[0] =
			SDMA_START_OFFSET + 0x00;
	adapt->doorbell_index.sdma_engine[1] =
			SDMA_START_OFFSET + 0x01;
	adapt->doorbell_index.sdma_engine[2] =
			SDMA_START_OFFSET + 0x02;
	adapt->doorbell_index.sdma_engine[3] =
			SDMA_START_OFFSET + 0x03;
	adapt->doorbell_index.sdma_engine[4] =
			SDMA_START_OFFSET + 0x04;
	adapt->doorbell_index.sdma_engine[5] =
			SDMA_START_OFFSET + 0x05;
	adapt->doorbell_index.sdma_engine[6] =
			SDMA_START_OFFSET + 0x06;
	adapt->doorbell_index.sdma_engine[7] =
			SDMA_START_OFFSET + 0x07;
	adapt->doorbell_index.ih = IH_START_OFFSET;
	adapt->doorbell_index.uvd_vce.uvd_ring0_1 =
			VCN_INST0_START_OFFSET + 0x00;
	adapt->doorbell_index.uvd_vce.uvd_ring2_3 =
			VCN_INST0_START_OFFSET + 0x01;
	adapt->doorbell_index.uvd_vce.uvd_ring4_5 =
			VCN_INST0_START_OFFSET + 0x02;
	adapt->doorbell_index.uvd_vce.uvd_ring6_7 =
			VCN_INST0_START_OFFSET + 0x03;
	adapt->doorbell_index.uvd_vce.vce_ring0_1 =
			VCN_INST0_START_OFFSET + 0x04;
	adapt->doorbell_index.uvd_vce.vce_ring2_3 =
			VCN_INST0_START_OFFSET + 0x05;
	adapt->doorbell_index.uvd_vce.vce_ring4_5 =
			VCN_INST0_START_OFFSET + 0x06;
	adapt->doorbell_index.uvd_vce.vce_ring6_7 =
			VCN_INST0_START_OFFSET + 0x07;
	adapt->doorbell_index.vcn.vcn_ring0_1 = VCN_INST1_START_OFFSET + 0x00;
	adapt->doorbell_index.vcn.vcn_ring2_3 = VCN_INST1_START_OFFSET + 0x01;
	adapt->doorbell_index.vcn.vcn_ring4_5 = VCN_INST1_START_OFFSET + 0x02;
	adapt->doorbell_index.vcn.vcn_ring6_7 = VCN_INST1_START_OFFSET + 0x03;

	adapt->doorbell_index.first_non_cp =
			SDMA_START_OFFSET;
	adapt->doorbell_index.last_non_cp =
			VCN_INST1_START_OFFSET + VCN_INST1_NUM_ENTRIES;

	adapt->doorbell_index.max_assignment =
			VCN_INST1_START_OFFSET + VCN_INST1_NUM_ENTRIES;
	adapt->doorbell_index.sdma_doorbell_range = SDMA_NUM_ENTRIES;

	return 0;
}

/* do nothing in these interfaces */
static int navi32_doorbell_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_doorbell_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_doorbell_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func navi32_doorbell_func = {
	.name = "navi32_doorbell_func",
	.sw_init = navi32_doorbell_index_init,
	.sw_fini = navi32_doorbell_sw_fini,
	.hw_init = navi32_doorbell_hw_init,
	.hw_fini = navi32_doorbell_hw_fini,
};
