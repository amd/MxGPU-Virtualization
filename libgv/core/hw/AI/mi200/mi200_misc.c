/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv_misc.h>
#include "mi200.h"

#include "mi200_gpuiov.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int mi200_misc_get_hdp_nonsurface_base(struct amdgv_adapter *adapt,
					       uint64_t *hdp_mc_addr)
{
	uint32_t hdp_nonsurface_base_lo;
	uint32_t hdp_nonsurface_base_hi;

	hdp_nonsurface_base_lo = RREG32(SOC15_REG_OFFSET(HDP, 0, mmHDP_NONSURFACE_BASE));
	hdp_nonsurface_base_hi = RREG32(SOC15_REG_OFFSET(HDP, 0, mmHDP_NONSURFACE_BASE_HI));
	*hdp_mc_addr = ((uint64_t)hdp_nonsurface_base_hi << 32) | hdp_nonsurface_base_lo;
	return 0;
}

static int mi200_misc_set_hdp_nonsurface_base(struct amdgv_adapter *adapt,
					       uint64_t hdp_mc_addr)
{
	uint32_t hdp_nonsurface_base_lo;
	uint32_t hdp_nonsurface_base_hi;

	hdp_nonsurface_base_lo = (uint32_t)(hdp_mc_addr & 0xFFFFFFFF);
	hdp_nonsurface_base_hi = (uint32_t)((hdp_mc_addr >> 32) & 0xFFFFFFFF);
	WREG32(SOC15_REG_OFFSET(HDP, 0, mmHDP_NONSURFACE_BASE), hdp_nonsurface_base_lo);
	WREG32(SOC15_REG_OFFSET(HDP, 0, mmHDP_NONSURFACE_BASE_HI), hdp_nonsurface_base_hi);
	return 0;
}

static int mi200_wait_for_cp_dma_pio_cb(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	uint32_t dma_cntl, dma_pio_empty, dma_pio_full, dma_pio_count;

	/* wait CP_DMA engine ready:
	*  check PIO_COUNT
	*  => Count of pending PIO (MMIO) initiated DMAs in the FIFO.
	*  => FIFO depth is 2.
	*  => If count is less than 2, then another DMA can be safely
	*       submitted without causing a deadlock in the h/w
	*/

	dma_cntl = RREG32(SOC15_REG_OFFSET(GC, 0, mmCP_DMA_CNTL));
	dma_pio_empty = REG_GET_FIELD(dma_cntl, CP_DMA_CNTL, PIO_FIFO_EMPTY);
	dma_pio_full = REG_GET_FIELD(dma_cntl, CP_DMA_CNTL, PIO_FIFO_FULL);
	if (dma_pio_empty)
		dma_pio_count = 0;
	else
		dma_pio_count = REG_GET_FIELD(dma_cntl, CP_DMA_CNTL, PIO_COUNT);

	return !(dma_pio_count < 2 && !dma_pio_full);
}

#define DMA_COPY_CONTEXT_REGS 1
static struct amdgv_reg_dump_info dma_copy_context_regs[DMA_COPY_CONTEXT_REGS] = {
	{
		.name = "CP_DMA_CNTL",
		.hwip = GC_HWIP,
		.seg = mmCP_DMA_CNTL_BASE_IDX,
		.logical_inst = 0,
		.offset_hwip = mmCP_DMA_CNTL,
		.access_method = AMDGV_REG_DUMP_ACCESS_MMIO,
	},
};

static int mi200_cp_dma_copy(struct amdgv_adapter *adapt, uint32_t idx_vf, bool fill_mode,
			      uint64_t src, uint64_t dst, uint64_t size, uint64_t *size_copied)
{
	uint32_t dma_cntl;
	uint32_t dma_cmd;
	uint32_t dma_size;
	uint32_t dma_temp;
	uint32_t dma_busy_flag;
	uint32_t curr_idx_vf;
	int wait_ret;
	struct amdgv_wait_for_cb_context cb_context = { 0 };

	cb_context.ctx = (void *)adapt;
	cb_context.type = AMDGV_WAIT_FOR_CP_DMA_PIO;
	cb_context.ctx_ext = dma_copy_context_regs;
	cb_context.num_ctx_ext = DMA_COPY_CONTEXT_REGS;

	/*
	 * to use CP_DMA copy, need to make sure GFX is switched to PF
	 */
	*size_copied = 0;
	/* check if PF is the active fcn on GFX */

	/*
	 * Note: MI200 Uses this function too. Because the HW sched enums values match on both ASICs,
	 *  this code still works.
	 */
	amdgv_gpuiov_get_active_vf_idx(adapt, MI200_HW_SCHED_BLOCK_GFX_SCH0_RLCV, &curr_idx_vf);
	if (curr_idx_vf != AMDGV_PF_IDX)
		return AMDGV_FAILURE;

	/* NOTE: for mmCP_DMA_PIO_CONTROL
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

	/* NOTE: for mmCP_DMA_PIO_COMMAND
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
		wait_ret = amdgv_wait_for(adapt, mi200_wait_for_cp_dma_pio_cb, &cb_context, AMDGV_TIMEOUT(TIMEOUT_CP_DMA), 0);
		if (!wait_ret) {

			WREG32(SOC15_REG_OFFSET(GC, 0, mmCP_DMA_PIO_CONTROL), dma_cntl);
			WREG32(SOC15_REG_OFFSET(GC, 0, mmCP_DMA_PIO_SRC_ADDR),
			       (uint32_t)(src & 0xffffffff));
			WREG32(SOC15_REG_OFFSET(GC, 0, mmCP_DMA_PIO_SRC_ADDR_HI),
			       (uint32_t)((src >> 32) & 0xffffffff));
			WREG32(SOC15_REG_OFFSET(GC, 0, mmCP_DMA_PIO_DST_ADDR),
			       (uint32_t)(dst & 0xffffffff));
			WREG32(SOC15_REG_OFFSET(GC, 0, mmCP_DMA_PIO_DST_ADDR_HI),
			       (uint32_t)((dst >> 32) & 0xffffffff));
			/* Make sure we don't go past end of region */
			if (((*size_copied) + dma_size) > size)
				dma_size = size - (*size_copied);
			/*
			 * NOTE: writing CP_DMA_PIO_COMMAND initiates operation,
			 * so write it last!
			 */
			WREG32(SOC15_REG_OFFSET(GC, 0, mmCP_DMA_PIO_COMMAND),
			       dma_cmd | dma_size);

			/* Advance to next block of FB region to fill/copy */
			*size_copied = (*size_copied) + dma_size;
			if (!fill_mode)
				src = src + dma_size;
			dst = dst + dma_size;
		} else {
			return AMDGV_FAILURE;
		}
	}

	/*
	 * use CP_BUSY or DMA_BUSY from CP_STAT register
	 */
	dma_busy_flag = ((uint32_t)0x1 << CP_STAT__CP_BUSY__SHIFT) |
			((uint32_t)0x1 << CP_STAT__DMA_BUSY__SHIFT);
	/* wait_dma_complete */
	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET_NAME(GC, 0, mmCP_STAT),
					   dma_busy_flag, 0, AMDGV_TIMEOUT(TIMEOUT_CP_DMA),
					   AMDGV_WAIT_CHECK_EQ, 0);

	if (!wait_ret)
		return 0;

	dma_temp = RREG32(SOC15_REG_OFFSET(GC, 0, mmCP_STAT));
	AMDGV_WARN("DMA failed to complete after %d usec, "
		   "cp_stat = 0x%08x (CP_BUSY or DMA_BUSY)\n",
		   AMDGV_TIMEOUT(TIMEOUT_CP_DMA), dma_temp);
	return AMDGV_FAILURE;
}

static int mi200_misc_sw_init(struct amdgv_adapter *adapt)
{
	adapt->misc.get_hdp_nonsurface_base = mi200_misc_get_hdp_nonsurface_base;
	adapt->misc.set_hdp_nonsurface_base = mi200_misc_set_hdp_nonsurface_base;
	if (!(adapt->flags & AMDGV_FLAG_DISABLE_CP_DMA)) {
		adapt->misc.dma_copy = mi200_cp_dma_copy;
		adapt->misc.dma_engine = AMDGV_DMA_ENGINE_CP_DMA;
	} else {
		adapt->misc.dma_engine = AMDGV_DMA_ENGINE_NONE;
	}

	return 0;
}

static int mi200_misc_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->misc.get_hdp_nonsurface_base = NULL;
	adapt->misc.set_hdp_nonsurface_base = NULL;
	adapt->misc.dma_copy = NULL;



	return 0;
}

static int mi200_misc_hw_init(struct amdgv_adapter *adapt)
{
	//Disable FED enable bit in HDP_MISC_CNTL.
	//This is to avoid forward of page faults from MMHUB to PCIE bus.
	uint32_t Hdp_Misc_Cntl = 0;
	Hdp_Misc_Cntl = RREG32(SOC15_REG_OFFSET(HDP, 0,
							mmHDP_MISC_CNTL));
	if (Hdp_Misc_Cntl & HDP_MISC_CNTL__FED_ENABLE_MASK) {
		Hdp_Misc_Cntl &= ~(HDP_MISC_CNTL__FED_ENABLE_MASK);
		WREG32(SOC15_REG_OFFSET(HDP, 0,
					mmHDP_MISC_CNTL), Hdp_Misc_Cntl);
	}

	return 0;
}

static int mi200_misc_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_misc_func = {
	.name = "mi200_misc_func",
	.sw_init = mi200_misc_sw_init,
	.sw_fini = mi200_misc_sw_fini,
	.hw_init = mi200_misc_hw_init,
	.hw_fini = mi200_misc_hw_fini,
};
