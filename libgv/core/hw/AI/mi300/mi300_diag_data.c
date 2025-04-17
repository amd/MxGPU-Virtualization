/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE.
 */

#include <amdgv_device.h>
#include "mi300.h"
#include "mi300_diag_data.h"
#include "mi300_powerplay.h"
#include "mi300_psp.h"
#include "amdgv_sched_internal.h"

static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

static int mi300_diag_data_psp_collect_snapshot_dump(
	struct amdgv_adapter *adapt, struct amdgv_diag_data_file_info *file_data)
{
	struct amdgv_diag_data_mem_block *psp_mem_blk;
	uint32_t used_size = 0;
	uint32_t idx = 0;
	uint32_t target_vfs = 0;
	uint32_t sections = 0;
	uint32_t aid_sections = 0;
	uint32_t idx_vf;
	enum psp_status psp_ret;
	uint32_t xcc_mask;
	uint32_t xcc_per_aid = MI300_NUM_XCC_PER_AID;
	uint32_t sdma_per_xcc = MI300_NUM_SDMA_PER_XCC;
	uint32_t xcc_per_aid_mask;
	uint32_t sdma_per_xcc_mask;
	uint32_t sdma_section_base;
	uint32_t xcc_local_mask;
	uint32_t i;
	bool gen_hdr;
	uint32_t total_size;

	struct amdgv_diag_data_asic_blk *mi300_blk =
		(struct amdgv_diag_data_asic_blk *)MI300_INTER_STRUCT_OFFSET;

	if (!mi300_blk) {
		AMDGV_WARN("Asic specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	psp_mem_blk = &mi300_blk->psp_snapshot_mem_blk;

	if (!psp_mem_blk->vaddr) {
		AMDGV_WARN("Can't get memory for psp snapshot dump\n");
		return AMDGV_FAILURE;
	}

	if (!psp_mem_blk->bus_addr) {
		AMDGV_WARN("Can't get bus address for psp snapshot dump\n");
		return AMDGV_FAILURE;
	}

	/* Set the mask for diagnosis data sections */
	sections = ((1 << (RWL_V01_SECTION_DIAG_DATA_END + 1)) - 1);
	if (RWL_V01_SECTION_DIAG_DATA_START)
		sections ^= ((1 << (RWL_V01_SECTION_DIAG_DATA_START)) - 1);

	idx_vf = file_data->idx_vf;
	if (idx_vf != AMDGV_PF_IDX) {
		xcc_mask = amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf);
		if (sections & RWL_V01_SECTION_MASK_VF_REG)
			target_vfs |= (1 << idx_vf);
	} else {
		xcc_mask = ((1 << adapt->mcp.gfx.num_xcc) - 1);
		if (sections & RWL_V01_SECTION_MASK_VF_REG) {
			for (idx = 0; idx < AMDGV_MAX_VF_NUM; idx++) {
				if (is_active_vf(idx))
					target_vfs |= (1 << idx);
			}

			/* remove the VF section if no VFs are active */
			if (!target_vfs)
				sections &= ~(RWL_V01_SECTION_MASK_VF_REG);
		}
	}

	AMDGV_INFO("Snapshot Buffer address: %llx size: %x\n", psp_mem_blk->bus_addr,
		   psp_mem_blk->size);

	AMDGV_INFO("Snapshot Sections %x target_vfs %x\n", sections, target_vfs);

	psp_ret = mi300_psp_set_snapshot_addr(adapt, psp_mem_blk->bus_addr, psp_mem_blk->size);
	if (psp_ret != PSP_STATUS__SUCCESS)
		return AMDGV_FAILURE;

	gen_hdr = true;
	total_size = 0;
	xcc_per_aid_mask = ((1 << xcc_per_aid) - 1);
	sdma_per_xcc_mask = ((1 << sdma_per_xcc) - 1);
	for (idx = 0; idx <  adapt->mcp.num_aid; idx++) {
		used_size = 0;
		if (xcc_mask & xcc_per_aid_mask) {
			/* Zero out the memory */
			oss_memset(psp_mem_blk->vaddr, 0, psp_mem_blk->size);

			/* Calculate SDMA mask */
			aid_sections = sections;
			sdma_section_base = RWL_V01_SECTION_ID_SRAM_SDMA0;
			if ((xcc_mask & xcc_per_aid_mask) != xcc_per_aid_mask) {
				xcc_local_mask = (xcc_mask & xcc_per_aid_mask) >> (idx * xcc_per_aid);
				for (i = 0; i < xcc_per_aid; i++) {
					if ((xcc_local_mask & 1) == 0)
						aid_sections &= ~(sdma_per_xcc_mask << sdma_section_base);
					xcc_local_mask >>= 1;
					sdma_section_base += sdma_per_xcc;
				}
			}

			psp_ret = mi300_psp_trigger_snapshot(adapt, target_vfs, aid_sections, 1 << idx,
					xcc_mask & xcc_per_aid_mask, &used_size);
			if (psp_ret != PSP_STATUS__SUCCESS) {
				AMDGV_ERROR("Unable to trigger snapshot data for AID:%d\n", idx);
				if (psp_ret == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE)
					break;

				/* Still copy the buffer out in case of partial data */
				used_size = psp_mem_blk->size;
			}

			/* Add data to file */
			if (used_size) {
				if (gen_hdr) {
					gen_hdr = false;
					if (amdgv_diag_data_add_blk(adapt, psp_mem_blk, file_data,
							used_size,
							AMDGV_DIAG_DATA_PSP_RWL_SECTION_FLAG(0x01)) != 0) {
						AMDGV_WARN("Unable to copy psp snapshot dump to memory\n");
						break;
					}
				} else {
					if (amdgv_diag_data_add_blk_without_hdr(adapt, psp_mem_blk,
							file_data, used_size, true) != 0)
						AMDGV_WARN("Unable to copy psp snapshot to memory for AID:%d\n", idx);
				}
			}
		}
		xcc_per_aid_mask <<= xcc_per_aid;
		total_size += used_size;
	}

	AMDGV_INFO("psp snapshot dump of size:%d\n", total_size);
	return 0;
}

static int
mi300_diag_data_psp_collect_trace_log(struct amdgv_adapter *adapt,
					 struct amdgv_diag_data_file_info *file_data)
{
	struct amdgv_diag_data_mem_block *psp_mem_blk;
	uint32_t used_size = 0;
	int ret = 0;
	enum psp_status psp_ret;
	struct amdgv_diag_data_asic_blk *mi300_blk =
		(struct amdgv_diag_data_asic_blk *)MI300_INTER_STRUCT_OFFSET;
	if (!mi300_blk) {
		AMDGV_WARN("Asic specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	psp_mem_blk = &mi300_blk->psp_tracelog_mem_blk;
	if (!psp_mem_blk->vaddr) {
		AMDGV_WARN("Can't get memory for psp tracelog dump\n");
		return AMDGV_FAILURE;
	}

	if (!psp_mem_blk->bus_addr) {
		AMDGV_WARN("Can't get bus address for psp tracelog dump\n");
		ret = AMDGV_FAILURE;
		goto psp_tracelog_dump_return;
	}

	AMDGV_INFO("Tracelog Buffer address: %llx size: %x\n", psp_mem_blk->bus_addr,
		   psp_mem_blk->size);
	psp_ret = mi300_psp_dump_tracelog(adapt, psp_mem_blk->bus_addr, psp_mem_blk->size,
					  &used_size);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		goto psp_tracelog_dump_return;
	}

	AMDGV_INFO("psp tracelog dump of size:%d\n", used_size);

	/* Add debug data to memory */
	if (amdgv_diag_data_add_blk(adapt, psp_mem_blk, file_data, used_size, 1) !=
	    0) {
		AMDGV_WARN("Unable to copy psp tracelog dump to memory\n");
		ret = AMDGV_FAILURE;
	}

psp_tracelog_dump_return:
	return ret;
}

static int
mi300_diag_data_psp_collect(struct amdgv_adapter *adapt,
				    struct amdgv_diag_data_file_info *file_data)
{
	if (adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SYS] < 0x0018005D) {
		AMDGV_WARN("PSP does not support Snapshot/Tracelog Dump\n");
		return 0;
	}

	if (mi300_diag_data_psp_collect_snapshot_dump(adapt, file_data) != 0)
		AMDGV_WARN("PSP Collect Snapshot dump failed\n");

	if (mi300_diag_data_psp_collect_trace_log(adapt, file_data) != 0)
		AMDGV_WARN("PSP Collect trace log failed\n");

	return 0;
}

static int mi300_diag_data_collect(struct amdgv_adapter *adapt,
					   struct amdgv_diag_data_file_info *file_data)
{
	if (amdgv_diag_data_host_collect_reg_dump(adapt, mi300_diag_data_regs,
						     mi300_diag_data_regs_count,
						     file_data) != 0)
		AMDGV_WARN("Collect Registers dump failed\n");

	/* GPU initialization failed, Cannot collect ASIC relative Debug Data */
	if (file_data->collect_type == AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_INIT_FAIL)
		return 0;

	if (mi300_diag_data_psp_collect(adapt, file_data) != 0)
		AMDGV_WARN("PSP Collect Data failed\n");

	return 0;
}

static uint32_t mi300_read_cross_aid_reg(struct amdgv_adapter *adapt,
		uint64_t reg, uint32_t aid)
{
	return RREG32_PCIE_EXT(reg | AMDGV_MI300_CROSS_AID_ACCESS(aid));
}

static int mi300_diag_data_sw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_diag_data_asic_blk *mi300_blk;
	uint64_t asic_addr;

	/* Asic buf is the second part of the memory buffer */
	asic_addr = (uint64_t)adapt->diag_data.asic_buff.vaddr;

	if (asic_addr > asic_addr + AMDGV_DIAG_DATA_ASIC_MEM_SIZE) {
		AMDGV_WARN("ASIC specific block memory overflow\n");
		return AMDGV_FAILURE;
	}

	mi300_blk = (struct amdgv_diag_data_asic_blk *)MI300_INTER_STRUCT_OFFSET;

	if (!mi300_blk) {
		AMDGV_WARN("Host specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	/* Zero out the memory */
	oss_memset(adapt->diag_data.asic_buff.vaddr, 0, AMDGV_DIAG_DATA_ASIC_MEM_SIZE);

	/* Init the PSP snapshot/tracelog buffer */
	MI300_DIAG_DATA_FILL_MEM_BLK(mi300_blk->psp_snapshot_mem_blk, asic_addr, PSP_SNAPSHOT_DUMP);
	MI300_DIAG_DATA_FILL_MEM_BLK(mi300_blk->psp_tracelog_mem_blk, asic_addr,
				PSP_TRACE_LOG_DUMP);

	adapt->diag_data.collect_data = mi300_diag_data_collect;
	adapt->diag_data.read_cross_aid_reg = mi300_read_cross_aid_reg;

	return 0;
}

static int mi300_diag_data_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_diag_data_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_diag_data_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi300_diag_data_func = {
	.name = "mi300_diag_data_func",
	.sw_init = mi300_diag_data_sw_init,
	.sw_fini = mi300_diag_data_sw_fini,
	.hw_init = mi300_diag_data_hw_init,
	.hw_fini = mi300_diag_data_hw_fini,
};

