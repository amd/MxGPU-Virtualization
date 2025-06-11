/*
 * Copyright (C) 2022 Advanced Micro Devices, Inc. All rights reserved.
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
#include <amdgv_misc.h>

#include "navi32_psp.h"
#include "navi32_sdma.h"
#include "navi32_gfx.h"

#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include <navi3/GC/gc_11_0_0_default.h>
#include <navi3/LSDMA/lsdma_6_0_0_offset.h>
#include <navi3/LSDMA/lsdma_6_0_0_sh_mask.h>
#include <navi3/HDP/hdp_6_0_0_offset.h>
#include <navi3/HDP/hdp_6_0_0_sh_mask.h>

#include <navi3/NBIO/nbio_4_3_0_offset.h>

#include <navi3/MMHUB/mmhub_3_0_0_offset.h>
#include <navi3/MMHUB/mmhub_3_0_0_sh_mask.h>

#define LSDMA_PIO_DMA_MAX_SIZE	0x3FFFFFFL		/* 64MB - 1*/

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int navi32_misc_get_hdp_nonsurface_base(struct amdgv_adapter *adapt,
					      uint64_t *hdp_mc_addr)
{
	uint32_t hdp_nonsurface_base_lo;
	uint32_t hdp_nonsurface_base_hi;

	hdp_nonsurface_base_lo = RREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE));
	hdp_nonsurface_base_hi = RREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE_HI));
	*hdp_mc_addr = ((uint64_t)hdp_nonsurface_base_hi << 32) | hdp_nonsurface_base_lo;

	return 0;
}

static int navi32_misc_set_hdp_nonsurface_base(struct amdgv_adapter *adapt,
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

static int navi32_wait_for_lsdma_pio_cb(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	uint32_t dma_status, fifo_full;

	dma_status = RREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_STATUS));
	fifo_full = REG_GET_FIELD(dma_status, LSDMA_PIO_STATUS, PIO_FIFO_FULL);

	return fifo_full;
}

static int navi32_lsdma_copy(struct amdgv_adapter *adapt, uint32_t idx_vf, bool fill_mode,
			     uint64_t src, uint64_t dst, uint64_t size, uint64_t *size_copied)
{
	uint32_t dma_cmd = 0;
	uint32_t dma_size, dma_temp;
	int wait_ret;

	dma_size = (size < LSDMA_PIO_DMA_MAX_SIZE) ? size : LSDMA_PIO_DMA_MAX_SIZE;

	*size_copied = 0;
	while ((*size_copied) < size) {

		wait_ret = amdgv_wait_for(adapt, navi32_wait_for_lsdma_pio_cb, (void *)adapt,
				AMDGV_TIMEOUT(TIMEOUT_LSDMA), 0);

		if (!wait_ret) {
			if (fill_mode) {
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_SRC_ADDR_LO), 0);
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_SRC_ADDR_HI), 0);
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_CONSTFILL_DATA),
						(uint32_t)(src & 0xffffffff));
			} else {
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_SRC_ADDR_LO),
						(uint32_t)(src & 0xffffffff));
				WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_SRC_ADDR_HI),
						(uint32_t)((src >> 32) & 0xffffffff));
			}

			WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_DST_ADDR_LO),
			       (uint32_t)(dst & 0xffffffff));
			WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_DST_ADDR_HI),
			       (uint32_t)((dst >> 32) & 0xffffffff));

			/* Make sure we don't go past end of region */
			if (((*size_copied) + dma_size) > size)
				dma_size = size - (*size_copied);

			dma_cmd = dma_size << LSDMA_PIO_COMMAND__BYTE_COUNT__SHIFT;

			if (fill_mode)
				dma_cmd = dma_cmd | (1 << LSDMA_PIO_COMMAND__CONSTANT_FILL__SHIFT);
			/*
			 * NOTE: writing LSDMA_PIO_COMMAND initiates operation,
			 * so write it last!
			 */
			WREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_COMMAND), dma_cmd);

			AMDGV_DEBUG4("dma_cmd=0x%x "
				     "dma_size=0x%x src=0x%llx dst=0x%llx\n",
				     dma_cmd, dma_size, src, dst);

			/* Advance to next block of FB region to fill/copy */
			*size_copied = (*size_copied) + dma_size;
			if (!fill_mode)
				src = src + dma_size;
			dst = dst + dma_size;
		} else {
			dma_temp = RREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_STATUS));
			AMDGV_WARN("DMA failed! FIFO full! "
				   "DMA not ready (at pf_mc_addr=0x%llx) after "
				   "%d usec, dma_status = 0x%x)\n",
				   dst, AMDGV_TIMEOUT(TIMEOUT_LSDMA), dma_temp);
			return AMDGV_FAILURE;
		}

	}

	/* wait_dma_pio_idle */
	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_STATUS),
					   LSDMA_PIO_STATUS__PIO_IDLE_MASK, 0,
					   AMDGV_TIMEOUT(TIMEOUT_LSDMA), AMDGV_WAIT_CHECK_NE, 0);

	if (wait_ret) {
		dma_temp = RREG32(SOC15_REG_OFFSET(LSDMA, 0, regLSDMA_PIO_STATUS));

		AMDGV_WARN("DMA failed! PIO status does not become idle! "
			   "DMA not ready (at pf_mc_addr=0x%llx) after "
			   "%d usec, dma_status = 0x%x)\n",
			   dst, AMDGV_TIMEOUT(TIMEOUT_LSDMA), dma_temp);
		return AMDGV_FAILURE;
	}

	return 0;

}

static void navi32_setup_common_timeout(struct amdgv_adapter *adapt)
{
	/* PSP */
	AMDGV_TIMEOUT(TIMEOUT_PSP_REG) = 1000 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_PSP_MEM) = 1000 * 1000;

	/* SMU */
	AMDGV_TIMEOUT(TIMEOUT_SMU_REG) = 200 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_SMU_IND_REG) = 200 * 1000;

	/* RESET */
	AMDGV_TIMEOUT(TIMEOUT_RESET) = 100 * 1000;
	/* STATUS */
	AMDGV_TIMEOUT(TIMEOUT_GRBM_STATUS) = 100 * 1000;

	AMDGV_TIMEOUT(TIMEOUT_STATUS_REG) = 50 * 1000;

	/* COMMAND */
	AMDGV_TIMEOUT(TIMEOUT_CMD_RESP) = 200 * 1000;

	/* LSDMA */
	AMDGV_TIMEOUT(TIMEOUT_LSDMA) = 100 * 1000;

	/* READ VBIOS */
	AMDGV_TIMEOUT(TIMEOUT_READ_VBIOS) = 5 * 1000 * 1000;

	/* MANUAL SWITCH */
	AMDGV_TIMEOUT(TIMEOUT_MANUAL_SWITCH) = 500 * 1000;
	/* AUTO SWITCH (MMSCH) */
	AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_MM) = 50 * 1000;
	/* AUTO SWITCH(GFX_SCH0_RLCV) */
	AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_GFX) = 500 * 1000;
	/* GUEST IDH */
	AMDGV_TIMEOUT(TIMEOUT_GUEST_IDH_RESP) = 10 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_GUEST_IDH_RESP_GPU_RESET) = 500 * 1000;
	/* PCI PENDING TRANSACTION */
	AMDGV_TIMEOUT(TIMEOUT_PCI_TRANS) = 400;
	/* BACO Hardware recovery */
	AMDGV_TIMEOUT(TIMEOUT_BACO_HW) = 2 * 1000 * 1000;
	/* DUMP CU DATA */
	AMDGV_TIMEOUT(TIMEOUT_DUMP_CU_DATA) = 70 * 1000 * 1000;
}

static void navi32_enable_gfxhub_gart(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint32_t crash_on_fault = 0;

	uint32_t hdp_nonsurface_base_lo;
	uint32_t hdp_nonsurface_base_hi;

	WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_AGP_BASE),
	       adapt->sys_mem_info.bus_addr >> 24);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_AGP_BOT), adapt->mc_agp_loc_addr >> 24);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_AGP_TOP), adapt->mc_agp_top_addr >> 24);

	WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_SYSTEM_APERTURE_LOW_ADDR),
	       adapt->mc_sys_loc_addr >> 18);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_SYSTEM_APERTURE_HIGH_ADDR),
	       adapt->mc_sys_top_addr >> 18);

	/* config context0 to trap all kinds of page fault */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT0_CNTL));
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, ENABLE_CONTEXT, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, PAGE_TABLE_DEPTH, 0);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, RETRY_PERMISSION_OR_INVALID_PAGE_FAULT,
			    0);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, RANGE_PROTECTION_FAULT_ENABLE_INTERRUPT,
			    1);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL,
			    DUMMY_PAGE_PROTECTION_FAULT_ENABLE_INTERRUPT, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, PDE0_PROTECTION_FAULT_ENABLE_INTERRUPT,
			    1);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, VALID_PROTECTION_FAULT_ENABLE_INTERRUPT,
			    1);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, READ_PROTECTION_FAULT_ENABLE_INTERRUPT,
			    1);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, WRITE_PROTECTION_FAULT_ENABLE_INTERRUPT,
			    1);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, EXECUTE_PROTECTION_FAULT_ENABLE_INTERRUPT,
			    1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT0_CNTL), tmp);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32), ~0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32), ~0);
	/* set GART logic space range from ~0 to 0
	 * thus force all GART range page fault
	 */
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT0_PAGE_TABLE_START_ADDR_LO32), ~0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT0_PAGE_TABLE_START_ADDR_HI32), ~0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT0_PAGE_TABLE_END_ADDR_LO32), 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT0_PAGE_TABLE_END_ADDR_HI32), 0);

	/* Setup TLB control */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_MX_L1_TLB_CNTL));
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 1);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, SYSTEM_ACCESS_MODE, 3);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL, 1);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, SYSTEM_APERTURE_UNMAPPED_ACCESS, 0);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, ECO_BITS, 0xA);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, MTYPE, 3); /* XXX for emulation. */
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_MX_L1_TLB_CNTL), tmp);

	/* Set GCMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR to HDP_NONSURFACE_BASE(FB start) */
	hdp_nonsurface_base_lo = RREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE));
	hdp_nonsurface_base_hi = RREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE_HI));
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_LSB), hdp_nonsurface_base_lo);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_MSB), hdp_nonsurface_base_hi);

	/* mmGCVM_L2_PROTECTION_FAULT_DEFAULT_ADDR */
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_LO32), 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_HI32), 0);

	/* regGCVM_L2_PROTECTION_FAULT_CNTL2 */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_CNTL2));
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL2,
			    ACTIVE_PAGE_MIGRATION_PTE_READ_RETRY, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_CNTL2), tmp);

	/* regGCVM_L2_CNTL */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL));
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, ENABLE_L2_CACHE, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, ENABLE_L2_FRAGMENT_PROCESSING, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, ENABLE_DEFAULT_PAGE_OUT_TO_SYSTEM_MEMORY, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, L2_PDE0_CACHE_TAG_GENERATION_MODE, 0);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, PDE_FAULT_CLASSIFICATION, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, CONTEXT1_IDENTITY_ACCESS_MODE, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, IDENTITY_MODE_FRAGMENT_SIZE, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL), tmp);

	/* regGCVM_L2_CNTL2 */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL2));
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL2, INVALIDATE_ALL_L1_TLBS, 0);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL2, INVALIDATE_L2_CACHE, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL2), tmp);

	/* regGCVM_L2_CNTL3 */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL3));
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL3, BANK_SELECT, 0x9);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL3, L2_CACHE_BIGK_FRAGMENT_SIZE, 0x6);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL3), tmp);

	/* mmVM_L2_CNTL4 */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL4));
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL4, VMC_TAP_PDE_REQUEST_PHYSICAL, 0);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL4, VMC_TAP_PTE_REQUEST_PHYSICAL, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL4), tmp);

	/* mmVM_L2_CNTL5 */
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL5), regGCVM_L2_CNTL5_DEFAULT);

	/* mmVM_L2_GCR_CNTL */
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_GCR_CNTL), regGCVM_L2_GCR_CNTL_DEFAULT);


	/* Disable identity aperture.*/
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_LO32),
	       0XFFFFFFFF);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_HI32),
	       0x0000000F);

	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_LO32),
	       0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_HI32),
	       0);

	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_LO32), 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_HI32), 0);

	/* regGCVM_L2_PROTECTION_FAULT_CNTL */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_CNTL));
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    RANGE_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    PDE0_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    PDE1_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    PDE2_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    TRANSLATE_FURTHER_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    NACK_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    DUMMY_PAGE_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    VALID_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    READ_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    WRITE_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    EXECUTE_PROTECTION_FAULT_ENABLE_DEFAULT, !crash_on_fault);

	/* CRASH_ON_NO_RETRY */
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL, CRASH_ON_NO_RETRY_FAULT,
			    crash_on_fault);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL, CRASH_ON_RETRY_FAULT,
			    crash_on_fault);

	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_CNTL), tmp);
}

static void navi32_disable_gfxhub_gart(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* regGCVM_L2_CNTL */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL));
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, ENABLE_L2_CACHE, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL), tmp);

	/* regGCVM_L2_CNTL3 */
	WREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_CNTL3), 0);
}


static void navi32_enable_mmutcl1_system_aperture_fault(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* regMMMC_VM_MX_L1_TLB4_DEBUG */
	tmp = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_MX_L1_TLB4_DEBUG));
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB4_DEBUG, ENABLE_APERTURE_FAULTS_ON_L2_RETURN, 1);
	WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_MX_L1_TLB4_DEBUG), tmp);

	/* regMMMC_VM_MX_L1_TLB5_DEBUG */
	tmp = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_MX_L1_TLB5_DEBUG));
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB5_DEBUG, ENABLE_APERTURE_FAULTS_ON_L2_RETURN, 1);
	WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_MX_L1_TLB5_DEBUG), tmp);
}

static uint64_t navi32_get_memsize(struct amdgv_adapter *adapt)
{
    return (uint64_t)RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_RCC_CONFIG_MEMSIZE));
}

static void navi32_mmhub_program_golden_settings(struct amdgv_adapter *adapt, void *idx_vf)
{
	uint32_t tmp;

	tmp = 0;
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 1);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, SYSTEM_ACCESS_MODE, 3);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL, 1);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, SYSTEM_APERTURE_UNMAPPED_ACCESS, 0);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, ECO_BITS, 0xA);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, MTYPE, 3);

	if (*(uint32_t *)idx_vf == AMDGV_PF_IDX) {
		WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_MX_L1_TLB_CNTL), tmp);
	} else {
		if (navi32_psp_program_register(adapt, *(uint32_t *)idx_vf, tmp, 0,
					MM_MC_VM_MX_L1_TLB_CNTL))
			AMDGV_ERROR("PSP: Failed to reprogram golden setting registers\n");
	}

	/* regMMVM_L2_CNTL2 */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regMMVM_L2_CNTL2));
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL2, INVALIDATE_ALL_L1_TLBS, 0);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL2, INVALIDATE_L2_CACHE, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regMMVM_L2_CNTL2), tmp);
}

static int navi32_misc_sw_init(struct amdgv_adapter *adapt)
{
	adapt->misc.get_hdp_nonsurface_base = navi32_misc_get_hdp_nonsurface_base;
	adapt->misc.set_hdp_nonsurface_base = navi32_misc_set_hdp_nonsurface_base;
	adapt->misc.get_memsize = navi32_get_memsize;

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_CP_DMA)) {
		adapt->misc.dma_copy = navi32_lsdma_copy;
		adapt->misc.dma_engine = AMDGV_DMA_ENGINE_LSDMA;
	} else {
		adapt->misc.dma_engine = AMDGV_DMA_ENGINE_NONE;
	}

	adapt->misc.reprogram_golden_settings = navi32_mmhub_program_golden_settings;
	navi32_setup_common_timeout(adapt);

	return 0;
}

static int navi32_misc_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->misc.get_hdp_nonsurface_base = NULL;
	adapt->misc.set_hdp_nonsurface_base = NULL;
	adapt->misc.dma_copy = NULL;
	adapt->misc.reprogram_golden_settings = NULL;

	return 0;
}

static int navi32_misc_hw_init(struct amdgv_adapter *adapt)
{
	/*move gart enablement here because we can't program any GC registers
	 *before RLCG autoload finished
	 */
	navi32_enable_gfxhub_gart(adapt);

	/* GC/SDMA golden registers are designed as PF only,
	* KMD in VF is not able to write them. Program them here from PF.
	* so that as PF, write golden settings that guest VM want to write
	*/
	navi32_sdma_program_golden_settings(adapt);

	navi32_gfx_program_golden_settings(adapt);

	navi32_enable_mmutcl1_system_aperture_fault(adapt);

	return 0;
}

static int navi32_misc_hw_fini(struct amdgv_adapter *adapt)
{
	if (!adapt->reset.reset_state) {
		navi32_disable_gfxhub_gart(adapt);
	}

	return 0;
}

struct amdgv_init_func navi32_misc_func = {
	.name = "navi32_misc_func",
	.sw_init = navi32_misc_sw_init,
	.sw_fini = navi32_misc_sw_fini,
	.hw_init = navi32_misc_hw_init,
	.hw_fini = navi32_misc_hw_fini,
};
