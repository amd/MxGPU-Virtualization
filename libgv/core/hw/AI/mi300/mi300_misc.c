/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_gfx.h>
#include <amdgv_gpuiov.h>
#include <amdgv_misc.h>
#include "mi300.h"

#include "mi300_gpuiov.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

struct mi300_wait_for_cp_dma_pio_cb_ctx {
	struct amdgv_adapter *adapt;
	uint32_t xcc_id;
};

static int mi300_misc_get_hdp_nonsurface_base(struct amdgv_adapter *adapt,
					       uint64_t *hdp_mc_addr)
{
	uint32_t hdp_nonsurface_base_lo;
	uint32_t hdp_nonsurface_base_hi;

	hdp_nonsurface_base_lo = RREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE));
	hdp_nonsurface_base_hi = RREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE_HI));
	*hdp_mc_addr = ((uint64_t)hdp_nonsurface_base_hi << 32) | hdp_nonsurface_base_lo;
	return 0;
}

static int mi300_misc_set_hdp_nonsurface_base(struct amdgv_adapter *adapt,
					       uint64_t hdp_mc_addr)
{
	uint32_t hdp_nonsurface_base_lo;
	uint32_t hdp_nonsurface_base_hi;

	hdp_nonsurface_base_lo = (uint32_t)(hdp_mc_addr & 0xFFFFFFFF);
	hdp_nonsurface_base_hi = (uint32_t)((hdp_mc_addr >> 32) & 0xFFFFFFFF);
	WREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE), hdp_nonsurface_base_lo);
	WREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE_HI), hdp_nonsurface_base_hi);
	return 0;
}

static int mi300_wait_for_cp_dma_pio_cb(void *context)
{
	struct mi300_wait_for_cp_dma_pio_cb_ctx *ctx = (struct mi300_wait_for_cp_dma_pio_cb_ctx *)context;
	struct amdgv_adapter *adapt = ctx->adapt;
	uint32_t xcc_id = ctx->xcc_id;
	uint32_t dma_cntl, dma_pio_empty, dma_pio_full, dma_pio_count;

	/* wait CP_DMA engine ready:
	*  check PIO_COUNT
	*  => Count of pending PIO (MMIO) initiated DMAs in the FIFO.
	*  => FIFO depth is 2.
	*  => If count is less than 2, then another DMA can be safely
	*       submitted without causing a deadlock in the h/w
	*/

	dma_cntl = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_DMA_CNTL));
	dma_pio_empty = REG_GET_FIELD(dma_cntl, CP_DMA_CNTL, PIO_FIFO_EMPTY);
	dma_pio_full = REG_GET_FIELD(dma_cntl, CP_DMA_CNTL, PIO_FIFO_FULL);
	if (dma_pio_empty)
		dma_pio_count = 0;
	else
		dma_pio_count = REG_GET_FIELD(dma_cntl, CP_DMA_CNTL, PIO_COUNT);

	return !(dma_pio_count < 2 && !dma_pio_full);
}

/* Prevents CP DMA and FLR from running concurrently. */
static void mi300_cp_dma_enter(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;

	if (adapt->asic_type != CHIP_MI350X)
		return;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive)
		return;

	shared_exclusion_enter(adapt, &hive->flr_cp_dma_lock, SHARED_EXCLUSION_GROUP_CP_DMA);
}

static void mi300_cp_dma_exit(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;

	if (adapt->asic_type != CHIP_MI350X)
		return;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive)
		return;

	shared_exclusion_exit(adapt, &hive->flr_cp_dma_lock, SHARED_EXCLUSION_GROUP_CP_DMA);
}

#define DMA_COPY_CONTEXT_REGS 8
static struct amdgv_reg_dump_info dma_copy_context_regs[DMA_COPY_CONTEXT_REGS] = {
	{
		.name = "CP_DMA_CNTL",
		.hwip = GC_HWIP,
		.seg = regCP_DMA_CNTL_BASE_IDX,
		.logical_inst = 0,
		.offset_hwip = regCP_DMA_CNTL,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
	{
		.name = "CP_DMA_CNTL",
		.hwip = GC_HWIP,
		.seg = regCP_DMA_CNTL_BASE_IDX,
		.logical_inst = 1,
		.offset_hwip = regCP_DMA_CNTL,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
	{
		.name = "CP_DMA_CNTL",
		.hwip = GC_HWIP,
		.seg = regCP_DMA_CNTL_BASE_IDX,
		.logical_inst = 2,
		.offset_hwip = regCP_DMA_CNTL,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
	{
		.name = "CP_DMA_CNTL",
		.hwip = GC_HWIP,
		.seg = regCP_DMA_CNTL_BASE_IDX,
		.logical_inst = 3,
		.offset_hwip = regCP_DMA_CNTL,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
	{
		.name = "CP_DMA_CNTL",
		.hwip = GC_HWIP,
		.seg = regCP_DMA_CNTL_BASE_IDX,
		.logical_inst = 4,
		.offset_hwip = regCP_DMA_CNTL,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
	{
		.name = "CP_DMA_CNTL",
		.hwip = GC_HWIP,
		.seg = regCP_DMA_CNTL_BASE_IDX,
		.logical_inst = 5,
		.offset_hwip = regCP_DMA_CNTL,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
	{
		.name = "CP_DMA_CNTL",
		.hwip = GC_HWIP,
		.seg = regCP_DMA_CNTL_BASE_IDX,
		.logical_inst = 6,
		.offset_hwip = regCP_DMA_CNTL,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
	{
		.name = "CP_DMA_CNTL",
		.hwip = GC_HWIP,
		.seg = regCP_DMA_CNTL_BASE_IDX,
		.logical_inst = 7,
		.offset_hwip = regCP_DMA_CNTL,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
};

static int mi300_cp_dma_copy(struct amdgv_adapter *adapt, uint32_t idx_vf, bool fill_mode,
			      uint64_t src, uint64_t dst, uint64_t size, uint64_t *size_copied)
{
	uint32_t dma_cntl;
	uint32_t dma_cmd;
	uint32_t dma_size;
	uint32_t dma_temp;
	uint32_t dma_busy_flag;
	uint32_t curr_idx_vf;
	int wait_ret;
	uint32_t xcc_id;
	uint32_t hw_sched_id;
	struct mi300_wait_for_cp_dma_pio_cb_ctx ctx = { 0 };
	int ret = 0;
	struct amdgv_wait_for_cb_context cb_context = { 0 };

	ctx.adapt = adapt;
	cb_context.type = AMDGV_WAIT_FOR_CP_DMA_PIO;
	cb_context.ctx_ext = dma_copy_context_regs;
	cb_context.num_ctx_ext = min(DMA_COPY_CONTEXT_REGS, adapt->mcp.gfx.num_xcc);

	/* to use CP_DMA copy, need to make sure GFX is switched to PF */
	*size_copied = 0;

	/* check if PF is the active fcn on GFX HW scheds */
	if (idx_vf == AMDGV_PF_IDX || idx_vf >= adapt->num_vf)
		return AMDGV_FAILURE;
	for_each_id (hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		if (!IS_HW_SCHED_TYPE_GFX(hw_sched_id))
			continue;
		amdgv_gpuiov_get_active_vf_idx(adapt, hw_sched_id, &curr_idx_vf);
		if (curr_idx_vf != AMDGV_PF_IDX)
			return AMDGV_FAILURE;
	}

	/* WORKAROUND */
	/* Remove wrapping CP DMA in RLC safe mode after PMFW provides fix */
	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf))
		amdgv_gfx_rlc_enter_safe_mode(adapt, xcc_id);

	mi300_cp_dma_enter(adapt);

	/* NOTE: for regCP_DMA_PIO_CONTROL
	 * CP_DMA_PIO_CONTROL[30:29] => SRC_SELECT: 0 (use SAS in COMMAND)
	 * CP_DMA_PIO_CONTROL[21:20] => DST_SELECT: 0 (use DAS in COMMAND)
	 *
	 */
	if (fill_mode) {
		/* src = data, dst = das, dst_mtype = 3(uncacheable) */
		dma_cntl = ((uint32_t)0x2 << CP_DMA_PIO_CONTROL__SRC_SELECT__SHIFT);
		/* => that is, SRC_ADDR will contain PATTERN to be filled */
	} else {
		dma_cntl = 0;
	}

	/* NOTE: for regCP_DMA_PIO_COMMAND
	 * CP_DMA_PIO_COMMAND[31]: DIS_WC=1 (Disable Write Confirm)
	 * CP_DMA_PIO_COMMAND[30]: RAW_WAIT=1 (Wait previous write)
	 * CP_DMA_PIO_COMMAND[29]: DAIC=0 (incr internal dest_addr)
	 * CP_DMA_PIO_COMMAND[28]: SAIC=0 (incr internal src_addr)
	 * CP_DMA_PIO_COMMAND[27]: DAS=0 (dest_addr is memory space address)
	 * CP_DMA_PIO_COMMAND[26]: SAS=0 (src_addr is memory space address)
	 * CP_DMA_PIO_COMMAND[25:0] => BYTE_COUNT
	 *   => if (and only if) both the Source and Destination are memory,
	 *	  BYTE_COUNT = N Bytes of Data to move
	 *      For any other cases,
	 *	  BYTE_COUNT must be DWORD-aligned (i.e, multiple of 4 Bytes)
	 *   => if both the Source and Destination are memory,
	 *	  Source and Destination addresses can be Byte-aligned
	 *      For any other cases,
	 *	  Source and Destination addresses must be DWORD-aligned
	 *
	 */
	if (fill_mode) {
		/* DIS_WC=0 RAW_WAIT=1 SAIC=1(not inc) DAS=0(memory) SAS=1(register) */
		dma_cmd = ((uint32_t)0x1 << CP_DMA_PIO_COMMAND__RAW_WAIT__SHIFT) |
			  ((uint32_t)0x1 << CP_DMA_PIO_COMMAND__SAIC__SHIFT) |
			  ((uint32_t)0x1 << CP_DMA_PIO_COMMAND__SAS__SHIFT);
	} else {
		dma_cmd = ((uint32_t)0x1 << CP_DMA_PIO_COMMAND__RAW_WAIT__SHIFT);
	}

	/* maximum BYTE_COUNT (26-bit) supported by CP_DMA => 64MB*/
	dma_size = ((uint32_t)CP_DMA_PIO_COMMAND__BYTE_COUNT_MASK >>
		    CP_DMA_PIO_COMMAND__BYTE_COUNT__SHIFT);
	if ((uint64_t)dma_size > size) {
		dma_size = (uint32_t)size;
		if (fill_mode) {
			/* Make sure "dma_size" is DWORD-aligned */
			dma_size = ((dma_size + 3) / 4) * 4;
		}
	} else {
		if (fill_mode) {
			/* Make sure "dma_size" is 32 Bytes (8 DWORDs)-aligned
			 * since the CP_DMA copy 32 Bytes (8 DWORDs) per clock.
			 */
			dma_size = dma_size & (~(0x1F));
		}
	}

	/*
	 * The CP DMA has the bandwidth of around 10 GBytes per second.
	 * The time consumed is around 100 us for 1 MBytes.
	 * Max data size for 1 DMA is less than 64M
	 * so, max time out is less than 6.25 ms.
	 * For safe margin, use Wait of 20000 usec (to cover 64MB max).
	 */
	AMDGV_DEBUG("CP DMA: fill=%d src=0x%llx dst=0x%llx size=0x%llx\n",
				fill_mode, src, dst, size);

	*size_copied = 0;
	while ((*size_copied) < size) {
		for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
			ctx.xcc_id = xcc_id;
			cb_context.ctx = (void *)&ctx;
			wait_ret = amdgv_wait_for(adapt, mi300_wait_for_cp_dma_pio_cb, &cb_context, AMDGV_TIMEOUT(TIMEOUT_CP_DMA), 0);

			if (!wait_ret) {
				WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_DMA_PIO_CONTROL), dma_cntl);
				WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_DMA_PIO_SRC_ADDR),
					(uint32_t)(src & 0xffffffff));
				WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_DMA_PIO_SRC_ADDR_HI),
					(uint32_t)((src >> 32) & 0xffffffff));
				WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_DMA_PIO_DST_ADDR),
					(uint32_t)(dst & 0xffffffff));
				WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_DMA_PIO_DST_ADDR_HI),
					(uint32_t)((dst >> 32) & 0xffffffff));

				/* Make sure we don't go past end of region */
				if (((*size_copied) + dma_size) > size)
					dma_size = size - (*size_copied);

				/*
				* NOTE: writing CP_DMA_PIO_COMMAND initiates operation,
				* so write it last!
				*/
				WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_DMA_PIO_COMMAND),
			       dma_cmd | dma_size);

				/* Advance to next block of FB region to fill/copy */
				*size_copied = (*size_copied) + dma_size;
				if ((*size_copied) >= size)
					break;

				if (!fill_mode)
					src = src + dma_size;
				dst = dst + dma_size;
			} else {
				ret = AMDGV_FAILURE;
				goto exit_cp_dma;
			}
		}
	}

	/*
	 * use CP_BUSY or DMA_BUSY from CP_STAT register
	 */
	dma_busy_flag = ((uint32_t)0x1 << CP_STAT__CP_BUSY__SHIFT) |
			((uint32_t)0x1 << CP_STAT__DMA_BUSY__SHIFT);
	/* wait_dma_complete for all CP inst used */
	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
		wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET_NAME(GC, GET_INST(GC, xcc_id), regCP_STAT),
						dma_busy_flag, 0, AMDGV_TIMEOUT(TIMEOUT_CP_DMA),
						AMDGV_WAIT_CHECK_EQ, 0);

		if (!wait_ret)
			continue;

		dma_temp = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCP_STAT));
		AMDGV_WARN("DMA on inst %u failed to complete after %d usec, "
			"cp_stat = 0x%08x (CP_BUSY or DMA_BUSY)\n",
			GET_INST(GC, xcc_id), AMDGV_TIMEOUT(TIMEOUT_CP_DMA), dma_temp);
		ret = AMDGV_FAILURE;
		goto exit_cp_dma;
	}

exit_cp_dma:
	mi300_cp_dma_exit(adapt);

	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf))
		amdgv_gfx_rlc_exit_safe_mode(adapt, xcc_id);

	return ret;
}

static int mi300_misc_sw_init(struct amdgv_adapter *adapt)
{
	adapt->misc.get_hdp_nonsurface_base = mi300_misc_get_hdp_nonsurface_base;
	adapt->misc.set_hdp_nonsurface_base = mi300_misc_set_hdp_nonsurface_base;
	if (!(adapt->flags & AMDGV_FLAG_DISABLE_CP_DMA)) {
		adapt->misc.dma_copy = mi300_cp_dma_copy;
		adapt->misc.dma_engine = AMDGV_DMA_ENGINE_CP_DMA;
	} else {
		adapt->misc.dma_engine = AMDGV_DMA_ENGINE_NONE;
	}

	amdgv_migration_set_ctx_version(adapt, AMDGV_MIGRATION_CONTEXT_VERSION_V2);

	return 0;
}

static int mi300_misc_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->misc.get_hdp_nonsurface_base = NULL;
	adapt->misc.set_hdp_nonsurface_base = NULL;
	adapt->misc.dma_copy = NULL;

	return 0;
}

static int mi300_misc_hw_init(struct amdgv_adapter *adapt)
{
	//Disable FED enable bit in HDP_MISC_CNTL.
	//This is to avoid forward of page faults from MMHUB to PCIE bus.
	uint32_t Hdp_Misc_Cntl = 0;
	Hdp_Misc_Cntl = RREG32(SOC15_REG_OFFSET(HDP, 0,
							regHDP_MISC_CNTL));
	if (Hdp_Misc_Cntl & HDP_MISC_CNTL__FED_ENABLE_MASK) {
		Hdp_Misc_Cntl &= ~(HDP_MISC_CNTL__FED_ENABLE_MASK);
		WREG32(SOC15_REG_OFFSET(HDP, 0,
					regHDP_MISC_CNTL), Hdp_Misc_Cntl);
	}

	// MIX00 supports ucode load. Skip DFC load in misc
	return 0;
}

static int mi300_misc_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi300_misc_func = {
	.name = "mi300_misc_func",
	.sw_init = mi300_misc_sw_init,
	.sw_fini = mi300_misc_sw_fini,
	.hw_init = mi300_misc_hw_init,
	.hw_fini = mi300_misc_hw_fini,
};
