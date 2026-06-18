/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include "navi32_reg_inc.h"
#include "navi32_diag_data.h"
#include "navi32_powerplay.h"
#include "navi32_psp.h"
#include "amdgv_sched_internal.h"
#include "navi3/GC/gc_11_0_3_offset.h"

static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

static int navi32_diag_data_psp_collect_snapshot_dump(
		struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data)
{
	struct amdgv_diag_data_mem_block *psp_mem_blk;
	uint32_t used_size = 0;
	uint32_t idx = 0;
	uint32_t target_vfs = 0;
	uint32_t sections = 0;
	enum psp_status psp_ret;

	struct amdgv_diag_data_asic_blk *navi32_blk =
		(struct amdgv_diag_data_asic_blk *)NAVI32_INTER_STRUCT_OFFSET;
	if (!navi32_blk) {
		AMDGV_WARN("Asic specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	psp_mem_blk = &navi32_blk->psp_snapshot_mem_blk;

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

	AMDGV_DEBUG("Snapshot Buffer address: %llx size: %x\n",
				psp_mem_blk->bus_addr, psp_mem_blk->size);

	AMDGV_DEBUG("Snapshot Sections %x target_vfs %x\n",
				sections, target_vfs);

	psp_ret = navi32_psp_set_snapshot_addr(adapt, psp_mem_blk->bus_addr,
				psp_mem_blk->size);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		AMDGV_WARN("Fails to set PSP snapshot_addr\n");
		return AMDGV_FAILURE;
	}

	psp_ret = navi32_psp_trigger_snapshot(adapt, target_vfs, sections,
				&used_size);

	if (psp_ret != PSP_STATUS__SUCCESS) {
		if (psp_ret == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE) {
			AMDGV_WARN("Fails to trigger PSP snapshot - Unsupported feature\n");
			return AMDGV_FAILURE;
		} else {
			/* assume PSP may have partially dumped snapshot.
			 * Copy default size.
			 */
			used_size = psp_mem_blk->size;
		}
	}

	AMDGV_DEBUG("psp snapshot dump of size:%d\n", used_size);

	/* Add debug data to memory */
	if (amdgv_diag_data_add_blk(adapt, psp_mem_blk, file_data,
			used_size, 1) != 0) {
		AMDGV_WARN("Unable to copy psp snapshot dump to memory\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_diag_data_psp_collect_trace_log(
		struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data)
{
	struct amdgv_diag_data_mem_block *psp_mem_blk;
	uint32_t used_size = 0;
	int ret = 0;
	enum psp_status psp_ret;
	struct amdgv_diag_data_asic_blk *navi32_blk =
		(struct amdgv_diag_data_asic_blk *)NAVI32_INTER_STRUCT_OFFSET;
	if (!navi32_blk) {
		AMDGV_WARN("Asic specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	psp_mem_blk = &navi32_blk->psp_tracelog_mem_blk;
	if (!psp_mem_blk->vaddr) {
		AMDGV_WARN("Can't get memory for psp tracelog dump\n");
		return AMDGV_FAILURE;
	}

	if (!psp_mem_blk->bus_addr) {
		AMDGV_WARN("Can't get bus address for psp tracelog dump\n");
		ret = AMDGV_FAILURE;
		goto psp_tracelog_dump_return;
	}

	AMDGV_DEBUG("Tracelog Buffer address: %llx size: %x\n",
				psp_mem_blk->bus_addr, psp_mem_blk->size);
	psp_ret = navi32_psp_dump_tracelog(adapt, psp_mem_blk->bus_addr,
				psp_mem_blk->size, &used_size);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		goto psp_tracelog_dump_return;
	}

	AMDGV_DEBUG("psp tracelog dump of size:%d\n", used_size);

	/* Add debug data to memory */
	if (amdgv_diag_data_add_blk(adapt, psp_mem_blk, file_data,
			used_size, 1) != 0) {
		AMDGV_WARN("Unable to copy psp tracelog dump to memory\n");
		ret = AMDGV_FAILURE;
	}

psp_tracelog_dump_return:
	return ret;
}

static int navi32_diag_data_psp_collect(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data)
{

	if (navi32_diag_data_psp_collect_snapshot_dump(adapt, file_data) != 0)
		AMDGV_WARN("PSP Collect Snapshot dump failed\n");

	if (navi32_diag_data_psp_collect_trace_log(adapt, file_data) != 0)
		AMDGV_WARN("PSP Collect trace log failed\n");

	return 0;
}

static int navi32_diag_data_collect(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data)
{
	if (amdgv_diag_data_host_collect_reg_dump(adapt,
			navi32_diag_data_regs,
			navi32_diag_data_regs_count,
			file_data) != 0)
		AMDGV_WARN("Collect Registers dump failed\n");

	/* GPU initialization failed, Cannot collect ASIC relative Debug Data */
	if (file_data->collect_type ==
		AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_INIT_FAIL) {
		AMDGV_ERROR("GPU initialization failed, Cannot collect ASIC relative Debug Data\n");
		return 0;
	}

	if (navi32_diag_data_psp_collect(adapt, file_data) != 0)
		AMDGV_ERROR("PSP Collect Data failed\n");

	return 0;
}

static uint64_t navi32_diag_data_get_gpu_ref_timestamp(struct amdgv_adapter *adapt)
{
	uint64_t gpu_timestamp = 0;
	uint32_t refclock_msb = 0;
	uint32_t refclock_msb_check = 0;
	uint32_t refclock_lsb = 0;

	refclock_msb = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_REFCLOCK_TIMESTAMP_MSB));
	refclock_lsb = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_REFCLOCK_TIMESTAMP_LSB));

	/* Read high 32-bit register MSB again to check for low 32-bit register LSB overflow */
	refclock_msb_check = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_REFCLOCK_TIMESTAMP_MSB));
	if (refclock_msb != refclock_msb_check) {
		refclock_msb = refclock_msb_check;
		refclock_lsb = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_REFCLOCK_TIMESTAMP_LSB));
	}

	gpu_timestamp = ((uint64_t) refclock_msb) << 32 | refclock_lsb;

	return gpu_timestamp;
}

static int navi32_diag_data_sw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_diag_data_asic_blk *navi32_blk;
	uint64_t asic_addr;

	/* Asic buf is the second part of the memory buffer */
	asic_addr = (uint64_t)adapt->diag_data.asic_buff.vaddr;

	if (asic_addr > asic_addr + AMDGV_DIAG_DATA_ASIC_MEM_SIZE) {
		AMDGV_WARN("ASIC specific block memory overflow\n");
		return AMDGV_FAILURE;
	}

	navi32_blk = (struct amdgv_diag_data_asic_blk *)
		NAVI32_INTER_STRUCT_OFFSET;

	if (!navi32_blk) {
		AMDGV_WARN("Host specific block not initialized\n");
		return AMDGV_FAILURE;
	}

	/* Zero out the memory */
	oss_memset(adapt->diag_data.asic_buff.vaddr,
			0, AMDGV_DIAG_DATA_ASIC_MEM_SIZE);

	/* Init the PSP snapshot/tracelog buffer */
	NAVI32_DIAG_DATA_FILL_MEM_BLK(navi32_blk->psp_snapshot_mem_blk,
			asic_addr, PSP_SNAPSHOT_DUMP);
	NAVI32_DIAG_DATA_FILL_MEM_BLK(navi32_blk->psp_tracelog_mem_blk,
			asic_addr, PSP_TRACE_LOG_DUMP);

	adapt->diag_data.collect_data = navi32_diag_data_collect;
	adapt->diag_data.get_gpu_ref_timestamp = navi32_diag_data_get_gpu_ref_timestamp;

	return 0;
}

static int navi32_diag_data_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_diag_data_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_diag_data_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func navi32_diag_data_func = {
	.name = "navi32_diag_data_func",
	.sw_init = navi32_diag_data_sw_init,
	.sw_fini = navi32_diag_data_sw_fini,
	.hw_init = navi32_diag_data_hw_init,
	.hw_fini = navi32_diag_data_hw_fini,
};

