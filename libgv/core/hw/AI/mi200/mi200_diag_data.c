/*
 * Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
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
#include "mi200.h"
#include "mi200_diag_data.h"
#include "mi200_ppsmc.h"
#include "mi200_powerplay.h"
#include "psp_v13_0.h"
#include "amdgv_sched_internal.h"

static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

static int mi200_diag_data_psp_collect_snapshot_dump(
		struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data)
{
	struct amdgv_diag_data_mem_block *psp_mem_blk;
	uint32_t used_size = 0;
	uint32_t idx = 0;
	uint32_t target_vfs = 0;
	uint32_t sections = 0;
	enum psp_status psp_ret;
	struct amdgv_diag_data_asic_blk *mi200_blk =
		(struct amdgv_diag_data_asic_blk *)MI200_INTER_STRUCT_OFFSET;


	if (!mi200_blk) {
		AMDGV_WARN("Asic specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	psp_mem_blk = &mi200_blk->psp_snapshot_mem_blk;

	if (!psp_mem_blk->vaddr) {
		AMDGV_WARN("Can't get memory for psp snapshot dump\n");
		return AMDGV_FAILURE;
	}

	if (!psp_mem_blk->bus_addr) {
		AMDGV_WARN("Can't get bus address for psp snapshot dump\n");
		return AMDGV_FAILURE;
	}

	/* Zero out the memory */
	oss_memset(psp_mem_blk->vaddr, 0, psp_mem_blk->size);

	for (idx = 0; idx < RWL_SECTION_ID_NUM; idx++)
		sections |= (1 << idx);

	if (sections & RWL_SECTION_MASK_SRIOV) {
		for (idx = 0; idx < AMDGV_MAX_VF_NUM; idx++) {
			if (is_active_vf(idx))
				target_vfs |= (1 << idx);
		}

		/* remove the VF section if no VFs are active */
		if (!target_vfs)
			sections &= ~(RWL_SECTION_MASK_SRIOV);
	}

	AMDGV_INFO("Snapshot Buffer address: %llx size: %x\n",
				psp_mem_blk->bus_addr, psp_mem_blk->size);

	AMDGV_INFO("Snapshot Sections %x target_vfs %x\n",
				sections, target_vfs);

	psp_ret = psp_v13_set_snapshot_addr(adapt, psp_mem_blk->bus_addr,
				psp_mem_blk->size);
	if (psp_ret != PSP_STATUS__SUCCESS)
		return AMDGV_FAILURE;

	psp_ret = psp_v13_trigger_snapshot(adapt, target_vfs, sections,
				&used_size);

	if (psp_ret != PSP_STATUS__SUCCESS) {
		if (psp_ret == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE) {
			return AMDGV_FAILURE;
		} else {
			/* assume PSP may have partially dumped snapshot.
			 * Copy default size.
			 */
			used_size = psp_mem_blk->size;
		}
	}

	AMDGV_INFO("psp snapshot dump of size:%d\n", used_size);

	/* Add debug data to memory */
	if (amdgv_diag_data_add_blk(adapt, psp_mem_blk, file_data,
			used_size, 1) != 0) {
		AMDGV_WARN("Unable to copy psp snapshot dump to memory\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi200_diag_data_psp_collect_trace_log(
		struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data)
{
	struct amdgv_diag_data_mem_block *psp_mem_blk;
	uint32_t used_size = 0;
	int ret = 0;
	enum psp_status psp_ret;
	struct amdgv_diag_data_asic_blk *mi200_blk =
		(struct amdgv_diag_data_asic_blk *)MI200_INTER_STRUCT_OFFSET;
	if (!mi200_blk) {
		AMDGV_WARN("Asic specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	psp_mem_blk = &mi200_blk->psp_tracelog_mem_blk;
	if (!psp_mem_blk->vaddr) {
		AMDGV_WARN("Can't get memory for psp tracelog dump\n");
		return AMDGV_FAILURE;
	}

	if (!psp_mem_blk->bus_addr) {
		AMDGV_WARN("Can't get bus address for psp tracelog dump\n");
		ret = AMDGV_FAILURE;
		goto psp_tracelog_dump_return;
	}

	AMDGV_INFO("Tracelog Buffer address: %llx size: %x\n",
				psp_mem_blk->bus_addr, psp_mem_blk->size);
	psp_ret = psp_v13_dump_tracelog(adapt, psp_mem_blk->bus_addr,
				psp_mem_blk->size, &used_size);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		goto psp_tracelog_dump_return;
	}

	AMDGV_INFO("psp tracelog dump of size:%d\n", used_size);

	/* Add debug data to memory */
	if (amdgv_diag_data_add_blk(adapt, psp_mem_blk, file_data,
			used_size, 1) != 0) {
		AMDGV_WARN("Unable to copy psp tracelog dump to memory\n");
		ret = AMDGV_FAILURE;
	}

psp_tracelog_dump_return:
	return ret;
}

static int mi200_diag_data_psp_collect(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data)
{
	if (adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SYS] < 0x0018005D) {
		AMDGV_WARN("PSP does not support Snapshot/Tracelog Dump\n");
		return 0;
	}

	if (mi200_diag_data_psp_collect_snapshot_dump(adapt, file_data) != 0)
		AMDGV_WARN("PSP Collect Snapshot dump failed\n");

	if (mi200_diag_data_psp_collect_trace_log(adapt, file_data) != 0)
		AMDGV_WARN("PSP Collect trace log failed\n");

	return 0;
}

static int mi200_diag_data_collect(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data)
{
	if (amdgv_diag_data_host_collect_reg_dump(adapt,
			mi200_diag_data_regs,
			mi200_diag_data_regs_count,
			file_data) != 0)
		AMDGV_WARN("Collect Registers dump failed\n");

	/* GPU initialization failed, Cannot collect ASIC relative Debug Data */
	if (file_data->collect_type ==
		AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_INIT_FAIL)
		return 0;

	if (mi200_diag_data_psp_collect(adapt, file_data) != 0)
		AMDGV_WARN("PSP Collect Data failed\n");

	return 0;
}

static int mi200_diag_data_sw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_diag_data_asic_blk *mi200_blk;
	uint64_t asic_addr;

	/* Asic buf is the second part of the memory buffer */
	asic_addr = (uint64_t)adapt->diag_data.asic_buff.vaddr;

	if (asic_addr > asic_addr + AMDGV_DIAG_DATA_ASIC_MEM_SIZE) {
		AMDGV_WARN("ASIC specific block memory overflow\n");
		return AMDGV_FAILURE;
	}

	mi200_blk = (struct amdgv_diag_data_asic_blk *)
		MI200_INTER_STRUCT_OFFSET;

	if (!mi200_blk) {
		AMDGV_WARN("Host specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	/* Zero out the memory */
	oss_memset(adapt->diag_data.asic_buff.vaddr,
			0, AMDGV_DIAG_DATA_ASIC_MEM_SIZE);

	/* Init the PSP snapshot/tracelog buffer */
	MI200_DIAG_DATA_FILL_MEM_BLK(mi200_blk->psp_snapshot_mem_blk,
			asic_addr, PSP_SNAPSHOT_DUMP);
	MI200_DIAG_DATA_FILL_MEM_BLK(mi200_blk->psp_tracelog_mem_blk,
			asic_addr, PSP_TRACE_LOG_DUMP);

	adapt->diag_data.collect_data = mi200_diag_data_collect;

	return 0;
}

static int mi200_diag_data_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi200_diag_data_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi200_diag_data_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_diag_data_func = {
	.name = "mi200_diag_data_func",
	.sw_init = mi200_diag_data_sw_init,
	.sw_fini = mi200_diag_data_sw_fini,
	.hw_init = mi200_diag_data_hw_init,
	.hw_fini = mi200_diag_data_hw_fini,
};

