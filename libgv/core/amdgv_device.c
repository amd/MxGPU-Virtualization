/*
 * Copyright (c) 2017-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_basetypes.h"
#include "amdgv_api.h"
#include "amdgv_device.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_error.h"
#include "amdgv_powerplay_swsmu.h"
#include "amdgv_powerplay.h"
#include "amdgv_guard.h"
#include "amdgv_vfmgr.h"
#include "amdgv_psp_gfx_if.h"
#include "amdgv_ras.h"
#include "amdgv_xgmi.h"

#include "hw/AI/ai.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

/* common register offset for all asics*/
#define mmMM_INDEX	 0x0000
#define mmMM_DATA	 0x0001
#define mmMM_INDEX_HI	 0x0006
#define mmPCIE_INDEX2	 0x000e
#define mmPCIE_INDEX2_HI 0x0011
#define mmPCIE_DATA2	 0x000f

#define MI300_CAPS (AMDGV_CAP_MANUAL_SCHED | AMDGV_CAP_HUNG_HW_DETECT)

static const struct amdgv_asic_entry amdgv_sriov_device_table[] = {
/* dev_id, sub_dev_id, rev_id, caps,
	 * init function table, mitigation table,
	 * reg base init
	 */

	/* Mi300X */
	{
		CHIP_MI300X,
		0x74A1,
		ANY_ID,
		ANY_ID,
		MI300_CAPS,
		&mi300x_init_table,
		NULL,
		&mi300_mitigation_table,
		mi300_reg_base_init,
	},
	/* Mi325X */
	{
		CHIP_MI300X,
		0x74A5,
		ANY_ID,
		ANY_ID,
		MI300_CAPS,
		&mi300x_init_table,
		NULL,
		&mi300_mitigation_table,
		mi300_reg_base_init,
	},
	/* Mi308X */
	{
		CHIP_MI308X,
		0x74A2,
		ANY_ID,
		ANY_ID,
		MI300_CAPS,
		&mi308x_init_table,
		NULL,
		&mi300_mitigation_table,
		mi300_reg_base_init,
	},
	/* MI308X */
	{
		CHIP_MI308X,
		0x74A8,
		ANY_ID,
		ANY_ID,
		MI300_CAPS,
		&mi308x_init_table,
		NULL,
		&mi300_mitigation_table,
		mi300_reg_base_init,
	},
	/* Mi300X_HC*/
	{
		CHIP_MI300X,
		0x74A9,
		ANY_ID,
		ANY_ID,
		MI300_CAPS,
		&mi300x_init_table,
		NULL,
		&mi300_mitigation_table,
		mi300_reg_base_init,
	},

};

uint32_t amdgv_mm_rreg(struct amdgv_adapter *adapt, uint32_t reg, bool always_indirect)
{
	uint32_t ret;

	if ((reg * 4) < adapt->mmio_size && !always_indirect)
		ret = oss_mm_read32((uint8_t *)adapt->mmio + (reg * 4));
	else {
		oss_spin_lock_irq(adapt->mmio_idx_lock);
		oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4));
		ret = oss_mm_read32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4));
		oss_spin_unlock_irq(adapt->mmio_idx_lock);
	}

	AMDGV_DEBUG4("reg = 0x%x, ret = 0x%x\n", reg, ret);
	return ret;
}

void amdgv_mm_wreg(struct amdgv_adapter *adapt, uint32_t reg, uint32_t val,
		   bool always_indirect)
{
	AMDGV_DEBUG4("reg = 0x%x, value = 0x%x\n", reg, val);

	if ((reg * 4) < adapt->mmio_size && !always_indirect)
		oss_mm_write32(((uint8_t *)adapt->mmio) + (reg * 4), val);
	else {
		oss_spin_lock_irq(adapt->mmio_idx_lock);
		oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4));
		oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), val);
		oss_spin_unlock_irq(adapt->mmio_idx_lock);
	}
}

void amdgv_smn_wreg8(struct amdgv_adapter *adapt, uint32_t reg, uint8_t val)
{
	AMDGV_DEBUG4("reg = 0x%x, value = 0x%x\n", reg, val);
	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), reg);
	oss_mm_write8(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), val);
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
}

uint8_t amdgv_smn_rreg8(struct amdgv_adapter *adapt, uint32_t reg)
{
	uint8_t ret;

	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), reg);
	ret = oss_mm_read8(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4));
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
	AMDGV_DEBUG4("reg = 0x%x, ret = 0x%x\n", reg, ret);

	return ret;
}

void amdgv_smn_wreg16(struct amdgv_adapter *adapt, uint32_t reg, uint16_t val)
{
	AMDGV_DEBUG4("reg = 0x%x, value = 0x%x\n", reg, val);
	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), reg);
	oss_mm_write16(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), val);
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
}

uint16_t amdgv_smn_rreg16(struct amdgv_adapter *adapt, uint32_t reg)
{
	uint16_t ret;

	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), reg);
	ret = oss_mm_read16(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4));
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
	AMDGV_DEBUG4("reg = 0x%x, ret = 0x%x\n", reg, ret);

	return ret;
}

void amdgv_smn_wreg32(struct amdgv_adapter *adapt, uint32_t reg, uint32_t val)
{
	AMDGV_DEBUG4("reg = 0x%x, value = 0x%x\n", reg, val);
	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), reg);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), val);
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
}

uint32_t amdgv_smn_rreg32(struct amdgv_adapter *adapt, uint32_t reg)
{
	uint32_t ret;

	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), reg);
	ret = oss_mm_read32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4));
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
	AMDGV_DEBUG4("reg = 0x%x, ret = 0x%x\n", reg, ret);

	return ret;
}

uint32_t amdgv_pcie_rreg(struct amdgv_adapter *adapt, uint32_t reg)
{
	uint32_t ret;

	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4));
	ret = oss_mm_read32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4));
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
	AMDGV_DEBUG4("reg = 0x%x, ret = 0x%x\n", reg, ret);
	return ret;
}

void amdgv_pcie_wreg(struct amdgv_adapter *adapt, uint32_t reg, uint32_t val)
{
	AMDGV_DEBUG4("reg = 0x%x, value = 0x%x\n", reg, val);
	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4));
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), val);
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
}

uint32_t amdgv_pcie_rreg_ext(struct amdgv_adapter *adapt, uint64_t reg)
{
	uint32_t ret;

	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4) & 0xFFFFFFFF);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2_HI * 4), (reg * 4) >> 32);
	ret = oss_mm_read32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4));
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2_HI * 4), 0);
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
	AMDGV_DEBUG4("reg = 0x%llx, ret = 0x%x\n", reg, ret);
	return ret;
}

void amdgv_pcie_wreg_ext(struct amdgv_adapter *adapt, uint64_t reg, uint32_t val)
{
	AMDGV_DEBUG4("reg = 0x%llx, value = 0x%x\n", reg, val);
	oss_spin_lock_irq(adapt->pcie_idx_lock);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4) & 0xFFFFFFFF);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2_HI * 4), (reg * 4) >> 32);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), val);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2_HI * 4), 0);
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
}

uint64_t amdgv_pcie_rreg64(struct amdgv_adapter *adapt, uint32_t reg)
{
	uint64_t ret;

	oss_spin_lock_irq(adapt->pcie_idx_lock);
	/* read low 32 bit*/
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4));
	ret = oss_mm_read32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4));

	/* read high 32 bit*/
	reg++;
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4));
	ret |= (((uint64_t)oss_mm_read32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4)))
		<< 32);
	oss_spin_unlock_irq(adapt->pcie_idx_lock);

	AMDGV_DEBUG4("reg = 0x%x, ret = 0x%llx\n", reg, ret);
	return ret;
}

void amdgv_pcie_wreg64(struct amdgv_adapter *adapt, uint32_t reg, uint64_t val)
{
	AMDGV_DEBUG4("reg = 0x%x, value = 0x%llx\n", reg, val);
	oss_spin_lock_irq(adapt->pcie_idx_lock);
	/* write low 32 bit */
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4));
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), val);

	/* write high 32 bit */
	reg++;
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4));
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), (uint32_t)(val >> 32));
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
}


uint64_t amdgv_pcie_rreg64_ext(struct amdgv_adapter *adapt, uint64_t reg)
{
	uint64_t ret;

	oss_spin_lock_irq(adapt->pcie_idx_lock);
	/* read low 32 bit*/
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4) & 0xFFFFFFFF);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2_HI * 4), (reg * 4) >> 32);
	ret = oss_mm_read32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4));

	/* read high 32 bit*/
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4 + 4) & 0xFFFFFFFF);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2_HI * 4), (reg * 4) >> 32);
	ret |= (((uint64_t)oss_mm_read32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4)))
		<< 32);
	oss_spin_unlock_irq(adapt->pcie_idx_lock);

	AMDGV_DEBUG4("reg = 0x%x, ret = 0x%llx\n", reg, ret);
	return ret;
}

void amdgv_pcie_wreg64_ext(struct amdgv_adapter *adapt, uint64_t reg, uint64_t val)
{
	AMDGV_DEBUG4("reg = 0x%x, value = 0x%llx\n", reg, val);
	oss_spin_lock_irq(adapt->pcie_idx_lock);
	/* write low 32 bit */
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4) & 0xFFFFFFFF);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2_HI * 4), (reg * 4) >> 32);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), val);

	/* write high 32 bit */
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2 * 4), (reg * 4 + 4) & 0xFFFFFFFF);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_INDEX2_HI * 4), (reg * 4) >> 32);
	oss_mm_write32(((uint8_t *)adapt->mmio) + (mmPCIE_DATA2 * 4), (uint32_t)(val >> 32));
	oss_spin_unlock_irq(adapt->pcie_idx_lock);
}

uint32_t amdgv_io_rreg(struct amdgv_adapter *adapt, uint32_t reg)
{
	uint32_t ret;

	if ((reg * 4) < adapt->io_mem_size)
		ret = oss_io_read32((uint8_t *)adapt->io_mem + (reg * 4));
	else {
		oss_io_write32((uint8_t *)adapt->io_mem + (mmMM_INDEX * 4), (reg * 4));
		ret = oss_io_read32((uint8_t *)adapt->io_mem + (mmMM_DATA * 4));
	}

	AMDGV_DEBUG4("reg = 0x%x, ret = 0x%x\n", reg, ret);

	return ret;
}

void amdgv_io_wreg(struct amdgv_adapter *adapt, uint32_t reg, uint32_t val)
{
	AMDGV_DEBUG4("reg = 0x%x, value = 0x%x\n", reg, val);

	if ((reg * 4) < adapt->io_mem_size)
		oss_io_write32((uint8_t *)adapt->io_mem + (reg * 4), val);
	else {
		oss_io_write32((uint8_t *)adapt->io_mem + (mmMM_INDEX * 4), (reg * 4));
		oss_io_write32((uint8_t *)adapt->io_mem + (mmMM_DATA * 4), val);
	}
}

uint32_t amdgv_mm_read_fb(struct amdgv_adapter *adapt, uint64_t addr)
{
	uint32_t ret;

	oss_spin_lock_irq(adapt->mmio_idx_lock);
	WREG32(mmMM_INDEX_HI, (uint32_t)(addr >> 31));
	WREG32(mmMM_INDEX, (uint32_t)(0x80000000 | (addr & 0x7ffffffc)));
	ret = RREG32(mmMM_DATA);
	oss_spin_unlock_irq(adapt->mmio_idx_lock);

	AMDGV_DEBUG4("addr = 0x%llx, ret = 0x%x\n", addr, ret);
	return ret;
}

void amdgv_mm_write_fb(struct amdgv_adapter *adapt, uint64_t addr, uint32_t val)
{
	AMDGV_DEBUG4("addr = 0x%llx, val = 0x%x\n", addr, val);

	oss_spin_lock_irq(adapt->mmio_idx_lock);
	WREG32(mmMM_INDEX_HI, (uint32_t)(addr >> 31));
	WREG32(mmMM_INDEX, (uint32_t)(0x80000000 | (addr & 0x7ffffffc)));
	WREG32(mmMM_DATA, val);
	oss_spin_unlock_irq(adapt->mmio_idx_lock);
}

void amdgv_mm_copy_to_fb(struct amdgv_adapter *adapt, uint64_t dst, uint64_t src,
			 uint64_t size)
{
	uint32_t i;
	uint32_t loop = (size + sizeof(uint32_t) - 1) / sizeof(uint32_t);

	for (i = 0; i < loop; i++) {
		WRITE_FB32(dst + (i * sizeof(uint32_t)), *(((uint32_t *)src) + i));
	}
}

void amdgv_mm_copy_from_fb(struct amdgv_adapter *adapt, uint64_t dst, uint64_t src,
			   uint64_t size)
{
	uint32_t i;
	uint32_t loop = (size + sizeof(uint32_t) - 1) / sizeof(uint32_t);

	for (i = 0; i < loop; i++) {
		*(((uint32_t *)dst) + i) = READ_FB32(src + (i * sizeof(uint32_t)));
	}
}

/* function to acquire frame buffer cpu virtual address from gpu virtual address
   Input:
   adapt: adapter structure
   gpu_vir: frame buffer gpu virtual address
   size: size of the structure locates at gpu_vir on fb
   cpu_vir: mapped cpu virtual address
   Output:
   return 0 as acquire virtual address successed
   return AMDGV_FAILURE as failed

*/
int amdgv_acquire_fb_virtual_addr(struct amdgv_adapter *adapt, uint64_t gpu_vir, uint64_t size,
				  uint64_t *cpu_vir)
{
	int ret = 0;
	uint64_t gpu_phy, mc_base, mapped_addr, offset;
	int map_gpu_phy_status = 0;

	if ((void *)gpu_vir == NULL) {
		AMDGV_ERROR("Invalid frame buffer gpu virtual address.\n");
		return AMDGV_FAILURE;
	}

	if (size <= 0) {
		AMDGV_ERROR("Mapped structure size is out of valid scope.\n");
		return AMDGV_FAILURE;
	}

	if (adapt->xgmi.phy_nodes_num > 1)
		mc_base = (adapt->memmgr_pf).mc_base;
	else
		mc_base = adapt->mc_fb_loc_addr;

	offset = (gpu_vir > mc_base) ? (gpu_vir - mc_base) : (mc_base - gpu_vir);

	// Map GPU physical address to CPU virtual address
	if ((adapt->flags & AMDGV_FLAG_USE_PF) && (offset >= MAX_OS_FB_MAPPING_SIZE)) {
		// OS does not support fb address mapping exceeds 4Gb
		gpu_phy = (gpu_vir > mc_base) ? (adapt->fb_pa + offset) :
						(adapt->fb_pa - offset);

		map_gpu_phy_status =
			oss_map_framebuffer(adapt->dev, gpu_phy, size, (void **)&mapped_addr);

		if (map_gpu_phy_status == 0) {
			*cpu_vir = mapped_addr;
		} else {
			AMDGV_ERROR("Frame buffer address mapping failed.\n");
			*cpu_vir = 0;
			ret = AMDGV_FAILURE;
		}
	} else {
		// OS support frame buffer address mapping > 4Gb
		// or frame buffer address offset < 4Gb
		*cpu_vir = (gpu_vir > mc_base) ? ((uint64_t)adapt->fb + offset) :
						 ((uint64_t)adapt->fb - offset);
	}

	return ret;
}

int amdgv_release_fb_virtual_addr(struct amdgv_adapter *adapt, void *addr, uint64_t offset)
{
	int ret = 0;

	if (!addr) {
		AMDGV_ERROR("Invalid frame buffer virtual addr.\n");
		return AMDGV_FAILURE;
	}

	if ((adapt->flags & AMDGV_FLAG_USE_PF) && (offset >= MAX_OS_FB_MAPPING_SIZE)) {
		if (oss_unmap_framebuffer(adapt->dev, addr) < 0) {
			AMDGV_ERROR("Unmap frame buffer address failed.\n");
			ret = AMDGV_FAILURE;
		}
	}

	return ret;
}

uint32_t amdgv_mm_rdoorbell(struct amdgv_adapter *adapt, uint32_t index)
{
	return oss_mm_read32(((uint32_t *)adapt->doorbell) + index);
}

void amdgv_mm_wdoorbell(struct amdgv_adapter *adapt, uint32_t index, uint32_t val)
{
	oss_mm_write32(((uint32_t *)adapt->doorbell) + index, val);
}

void amdgv_mm_wdoorbell64(struct amdgv_adapter *adapt, uint32_t index, uint64_t val)
{
	oss_mm_write64(((uint32_t *)adapt->doorbell) + index, val);
}

/* return the time stamp when should timeout the wait in 2 timeouts */
static uint64_t amdgv_wait_get_timeout(uint64_t start, uint64_t timeout_all,
					   uint64_t timeout_phase)
{
	uint64_t time_left;
	uint64_t now = oss_get_time_stamp();

	time_left = (now - start > timeout_all) ? 0 : (timeout_all - (now - start));
	return now + ((timeout_phase < time_left) ? timeout_phase : time_left);
}

/*
	amdgv_wait_for : wait for cb_func to return 0 or timeout.
	input:
		adapt:adapter structure
		cb_func: call back function which will be check in every wait loop. If function matched return 0, otherwise return a non-zero value
		cb_context: parameter which will be sent to cb_func
		timeout_us: timeout value in us
		wait_flag: config flag in waiting. Currently used to forcibly set wait function to udelay.
	output:
		return: 0 as succeeded, AMDGV_WAIT_RET_TIMED_OUT as timed out, AMDGV_WAIT_RET_INVALID as invalid parameter.
*/
int amdgv_wait_for(struct amdgv_adapter *adapt, amdgv_wait_cb_t cb_func, void *cb_context,
		   uint64_t timeout_us, uint32_t wait_flag)
{
	int interval;
	uint64_t start, now, phase_timeout, sub_timeout, time_left;
	start = now = oss_get_time_stamp();

	/* 2 special timeout values */
	if (timeout_us == 0) {
		if (adapt)
			AMDGV_ERROR("timeout argument should not be 0\n");
		else
			AMDGV_PRINT("[Wait Error]timeout argument should not be 0\n");
		return AMDGV_WAIT_RET_INVALID;
	} else if (timeout_us == 1) {
		if (cb_func(cb_context) == 0)
			goto success;
		else
			return AMDGV_WAIT_RET_TIMED_OUT;
	}

	/* phase 1, delay in 0~AMDGV_WAIT_PHASE1_THRESHOLD_US */
	interval = 2;
	/* select phase 1 time out */
	phase_timeout =
		amdgv_wait_get_timeout(start, timeout_us, AMDGV_WAIT_PHASE1_THRESHOLD_US);
	while (now < phase_timeout) {
		/* select sub phase time out */
		sub_timeout = amdgv_wait_get_timeout(start, timeout_us,
							 AMDGV_WAIT_PHASE1_LOOP_TIMEOUT(interval));

		while (now < sub_timeout && now < phase_timeout) {
			if (cb_func(cb_context) == 0)
				goto success;
			if ((interval > 10) && (wait_flag & AMDGV_WAIT_FLAG_USLEEP))
				oss_usleep(interval);
			else
				oss_udelay(interval);
			if (wait_flag & AMDGV_WAIT_FLAG_FORCE_YIELD) {
				oss_yield();
			}
			now = oss_get_time_stamp();
		}

		now = oss_get_time_stamp();
		/* phase 1 max interval is AMDGV_WAIT_PHASE1_MAX_DELAY_US,
		   so timeout is 2,4,8,16,32,50,50...... */
		interval = (interval * 2 < AMDGV_WAIT_MAX_DELAY_US) ? interval * 2 :
									  AMDGV_WAIT_MAX_DELAY_US;
	}

	/* phase 2, AMDGV_WAIT_PHASE1_THRESHOLD ~ timeout_us */
	phase_timeout = start + timeout_us;
	now = oss_get_time_stamp();
	time_left = (phase_timeout > now) ? phase_timeout - now : 0;
	/* minimum interval is 50us, otherwise loop for 100 cycle in maximum */
	interval = (AMDGV_WAIT_MIN_SLEEP_US < time_left / AMDGV_WAIT_CYCLE) ?
			   time_left / AMDGV_WAIT_CYCLE :
			   AMDGV_WAIT_MIN_SLEEP_US;
	while (now < phase_timeout) {
		if (cb_func(cb_context) == 0)
			goto success;

		if (wait_flag & AMDGV_WAIT_FLAG_FORCE_DELAY) {
			oss_udelay(AMDGV_WAIT_MAX_DELAY_US);
			if (wait_flag & AMDGV_WAIT_FLAG_FORCE_YIELD) {
				oss_yield();
			}
		} else if (wait_flag & AMDGV_WAIT_FLAG_USLEEP)
			oss_udelay(AMDGV_WAIT_MAX_DELAY_US);
		else if (interval > AMDGV_WAIT_PHASE2_MSLEEP_THRESHOLD_US)
			oss_msleep(interval / 1000);
		else
			oss_usleep(interval);

		now = oss_get_time_stamp();
	}
	/* check again if last sleep successes */
	if (cb_func(cb_context) == 0)
		goto success;
	if (!(wait_flag & AMDGV_WAIT_FLAG_NO_WARNING)) {
		if (adapt)
			AMDGV_WARN("wait timed out after %ld us, timeout=%ld us\n", now - start,
				   timeout_us);
		else
			AMDGV_PRINT("[Wait Warning]wait timed out after %ld us, timeout=%ld us\n",
					now - start, timeout_us);
	}
	return AMDGV_WAIT_RET_TIMED_OUT;
success:
	time_left = oss_get_time_stamp() - start;
	if (adapt)
		AMDGV_DEBUG("wait passed after %d us in %d us' timeout\n", time_left,
				timeout_us);
	return 0;
}

/* callback to check register values */
static int amdgv_wait_for_register_cb(void *context)
{
	struct amdgv_wait_for_register_context *reg_context =
		(struct amdgv_wait_for_register_context *)context;
	struct amdgv_adapter *adapt = reg_context->adapt;
	uint32_t offset = reg_context->offset;
	uint32_t mask = reg_context->mask;
	uint32_t value = reg_context->value;
	uint32_t check_flag = reg_context->check_flag;

	/* if mask is 0, it means no mask */
	if (mask == 0)
		mask = ~mask;

	/* we need to return 0 if hit, otherwise non 0 */
	switch (check_flag) {
	case AMDGV_WAIT_CHECK_EQ:
		return !((RREG32(offset) & mask) == value);
	case AMDGV_WAIT_CHECK_NE:
		return !((RREG32(offset) & mask) != value);
	default:
		AMDGV_ERROR("Wrong check flag\n");
		return AMDGV_FAILURE;
	}
}

/* callback to check if irq handler is processing */
int amdgv_wait_for_irq_handler(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	return adapt->irqmgr.ih.irq_processing;
}

/* function to wait for a register value */
int amdgv_wait_for_register(struct amdgv_adapter *adapt, uint32_t offset, uint32_t mask,
				uint32_t value, uint64_t timeout_us, uint32_t check_flag,
				uint32_t wait_flag)
{
	struct amdgv_wait_for_register_context reg_context;

	reg_context.adapt = adapt;
	reg_context.offset = offset;
	reg_context.mask = mask;
	reg_context.value = value;
	reg_context.check_flag = check_flag;
	if (adapt)
		AMDGV_DEBUG("start wait for register 0x%x in timeout %ld us\n", offset,
				timeout_us);
	return amdgv_wait_for(adapt, amdgv_wait_for_register_cb, (void *)&reg_context,
				  timeout_us, wait_flag);
}

/* callback to check register values */
static int amdgv_wait_for_memory_cb(void *context)
{
	struct amdgv_wait_for_memory_context *mm_context =
		(struct amdgv_wait_for_memory_context *)context;

	return !(*mm_context->address == mm_context->value);
}

int amdgv_wait_for_memory(struct amdgv_adapter *adapt, uint32_t *addr, uint32_t value,
			  uint64_t timeout_us)
{
	struct amdgv_wait_for_memory_context mm_context;

	mm_context.address = (volatile uint32_t *)addr;
	mm_context.value = value;
	if (adapt)
		AMDGV_DEBUG("start wait for memory %p in timeout %ld us\n", addr, timeout_us);
	return amdgv_wait_for(adapt, amdgv_wait_for_memory_cb, (void *)&mm_context, timeout_us,
				  0);
}

static int amdgv_wait_for_pci_cfg_cb(void *context)
{
	struct amdgv_wait_for_pci_cfg_context *cfg_context =
		(struct amdgv_wait_for_pci_cfg_context *)context;
	struct amdgv_adapter *adapt = cfg_context->adapt;
	uint32_t offset = cfg_context->offset;
	uint32_t mask = cfg_context->mask;
	uint32_t value = cfg_context->value;
	uint32_t check_flag = cfg_context->check_flag;
	uint8_t byte_len = cfg_context->byte_len;
	oss_dev_t dev = cfg_context->dev;
	uint32_t rd = 0;

	/* if mask is 0, it means no mask */
	if (mask == 0)
		mask = ~mask;

	if (dev) {
		switch (byte_len) {
		case 1:
			oss_pci_read_config_byte(dev, offset, (uint8_t *)&rd);
			break;
		case 2:
			oss_pci_read_config_word(dev, offset, (uint16_t *)&rd);
			break;
		case 4:
			oss_pci_read_config_dword(dev, offset, (uint32_t *)&rd);
			break;
		default:
			return AMDGV_FAILURE;
		}
	} else {
		switch (byte_len) {
		case 1:
			rd = RREG8_SMN(offset);
			break;
		case 2:
			rd = RREG16_SMN(offset);
			break;
		case 4:
			rd = RREG32_SMN(offset);
			break;
		default:
			return AMDGV_FAILURE;
		}
	}

	/* we need to return 0 if hit, otherwise non 0 */
	switch (check_flag) {
	case AMDGV_WAIT_CHECK_EQ:
		return !((rd & mask) == value);
	case AMDGV_WAIT_CHECK_NE:
		return !((rd & mask) != value);
	default:
		AMDGV_ERROR("Wrong check flag\n");
		return AMDGV_FAILURE;
	}
}

int amdgv_wait_for_pci_cfg(struct amdgv_adapter *adapt, oss_dev_t dev, uint32_t offset,
			   uint32_t mask, uint32_t value, uint8_t byte_len,
			   uint64_t timeout_us, uint32_t check_flag, uint32_t wait_flag)
{
	struct amdgv_wait_for_pci_cfg_context cfg_context;

	cfg_context.adapt = adapt;
	cfg_context.offset = offset;
	cfg_context.dev = dev;
	cfg_context.mask = mask;
	cfg_context.value = value;
	cfg_context.byte_len = byte_len;
	cfg_context.check_flag = check_flag;
	if (adapt)
		AMDGV_DEBUG("start wait for pci cfg %p in timeout %ld us\n", offset,
				timeout_us);
	return amdgv_wait_for(adapt, amdgv_wait_for_pci_cfg_cb, (void *)&cfg_context,
				  timeout_us, wait_flag);
}

static struct amdgv_asic_entry *amdgv_match_asic_table(uint32_t dev_id, uint32_t rev_id)

{
	int i;
	struct amdgv_asic_entry *asic_entry;

	for (i = 0; i < ARRAY_SIZE(amdgv_sriov_device_table); i++) {
		asic_entry = (struct amdgv_asic_entry *)&amdgv_sriov_device_table[i];

		if (asic_entry->dev_id == dev_id) {
			/* found a valid asic entry in device support table */
			if (asic_entry->rev_id == ANY_ID || asic_entry->rev_id == rev_id)
				return asic_entry;
		}
	}

	return NULL;
}

uint32_t amdgv_internal_get_asic_type(uint32_t device_id, uint32_t revision_id)
{
	struct amdgv_asic_entry *asic_entry = amdgv_match_asic_table(device_id, revision_id);
	if (asic_entry == NULL) {
		return CHIP_NOT_SUPPORTED;
	}
	return asic_entry->asic_type;
}

static void amdgv_fill_device_info(struct amdgv_adapter *adapt,
				   struct amdgv_init_data *init_data)
{
	adapt->dev = init_data->info.dev;
	adapt->bdf = init_data->info.bdf;
	adapt->vendor_id = init_data->info.vendor_id;
	adapt->dev_id = init_data->info.dev_id;
	adapt->rev_id = init_data->info.rev_id;
	adapt->sub_sys_id = init_data->info.sub_sys_id;
	adapt->sub_vnd_id = init_data->info.sub_vnd_id;
	adapt->fb = init_data->info.fb;
	adapt->mmio = init_data->info.mmio;
	adapt->io_mem = init_data->info.io_mem;
	adapt->doorbell = init_data->info.doorbell;
	adapt->fb_pa = init_data->info.fb_pa;
	adapt->fb_size = init_data->info.fb_size;
	adapt->mmio_pa = init_data->info.mmio_pa;
	adapt->mmio_size = init_data->info.mmio_size;
	adapt->io_mem_pa = init_data->info.io_mem_pa;
	adapt->io_mem_size = init_data->info.io_mem_size;
	adapt->doorbell_pa = init_data->info.doorbell_pa;
	adapt->doorbell_size = init_data->info.doorbell_size;
	adapt->rom = init_data->info.rom;           /* deprecated */
	adapt->rom_size = init_data->info.rom_size; /* deprecated */
	adapt->sriov_cap_pos = init_data->info.sriov_cap_pos;
	adapt->sriov_vf_offset = init_data->info.sriov_vf_offset;
	adapt->sriov_vf_stride = init_data->info.sriov_vf_stride;
	adapt->sriov_vf_devid = init_data->info.sriov_vf_devid;
	adapt->hive_lock = init_data->info.hive_lock;
	adapt->gpumon_hive_lock = init_data->info.gpumon_hive_lock;
	adapt->sys_mem_info.bus_addr = init_data->sys_mem_info.bus_addr;
	adapt->sys_mem_info.va_ptr = init_data->sys_mem_info.va_ptr;
	adapt->sys_mem_info.phys_addr = init_data->sys_mem_info.phys_addr;
	adapt->sys_mem_info.handle = NULL;
}

static void amdgv_free_config_opt(struct amdgv_adapter *adapt)
{
	if (adapt->opt.vf_options) {
		oss_free(adapt->opt.vf_options);
		adapt->opt.vf_options = NULL;
	}

	if (adapt->opt.pf_option) {
		oss_free(adapt->opt.pf_option);
		adapt->opt.pf_option = NULL;
	}

	if (adapt->opt.dfc_fw) {
		oss_free(adapt->opt.dfc_fw);
		adapt->opt.dfc_fw = NULL;
	}
}

static int amdgv_copy_config_opt(struct amdgv_adapter *adapt,
				 struct amdgv_init_data *init_data)
{
	oss_memcpy(&adapt->opt, &init_data->opt, sizeof(struct amdgv_init_config_opt));

	if (init_data->opt.vf_opts_num == 0) {
		adapt->opt.vf_options = NULL;
	} else {
		int size = init_data->opt.vf_opts_num * sizeof(struct amdgv_vf_option);

		adapt->opt.vf_options = oss_malloc(size);
		if (adapt->opt.vf_options == NULL)
			return AMDGV_FAILURE;

		oss_memcpy(adapt->opt.vf_options, init_data->opt.vf_options, size);
	}

	if (init_data->opt.pf_option) {
		adapt->opt.pf_option = oss_malloc(sizeof(struct amdgv_vf_option));

		if (adapt->opt.pf_option == NULL) {
			amdgv_free_config_opt(adapt);
			return AMDGV_FAILURE;
		}

		oss_memcpy(adapt->opt.pf_option, init_data->opt.pf_option,
			   sizeof(struct amdgv_vf_option));
	}

	if (init_data->opt.dfc_fw) {
		adapt->opt.dfc_fw = oss_malloc(AMDGV_MAX_DFC_FW_SIZE);

		if (adapt->opt.dfc_fw == NULL) {
			amdgv_free_config_opt(adapt);
			return AMDGV_FAILURE;
		}

		oss_memcpy(adapt->opt.dfc_fw, init_data->opt.dfc_fw,
			   AMDGV_MAX_DFC_FW_SIZE);
	}

	return 0;
}

static int amdgv_parse_config_opt(struct amdgv_adapter *adapt)
{
	int total_vf_num;
	int i;
	uint32_t gfx_tmp;
	uint32_t gfx_quanta_option;
	uint32_t mm_tmp;
	uint32_t mm_quanta_option;

	adapt->flags = adapt->opt.flags;

	amdgv_debug_set_mode(adapt, adapt->opt.debug_mode);

	/* check max VF */
	if (adapt->opt.total_vf_num > adapt->max_num_vf) {
		AMDGV_ERROR("VF number %u to be enabled exceeds the vf limit %u.\n",
			   adapt->opt.total_vf_num, adapt->max_num_vf);
		oss_clear_conf_file(adapt->dev);
		return AMDGV_FAILURE;
	} else {
		total_vf_num = adapt->opt.total_vf_num;
	}

	if (adapt->opt.vf_opts_num == 0) {
		if (total_vf_num == 0)
			/* if num vf not specified, enable 4 VFs by default */
			adapt->num_vf = AMDGV_VF_NUM_DEFAULT;
		else
			adapt->num_vf = total_vf_num;

		adapt->customized_vf_config_mode = false;

	} else {
		/* Invalid VF configurations */
		if (total_vf_num == 0 || total_vf_num != adapt->opt.vf_opts_num) {
			return AMDGV_FAILURE;
		}

		adapt->num_vf = total_vf_num;

		adapt->customized_vf_config_mode = true;
	}

	if (adapt->opt.ip_discovery_load_type > AMDGV_IP_DISCOVERY_LOAD_TYPE_BEGIN &&
		adapt->opt.ip_discovery_load_type < AMDGV_IP_DISCOVERY_LOAD_TYPE_END) {
		adapt->ip_discovery_load_type = adapt->opt.ip_discovery_load_type;
	} else
		adapt->ip_discovery_load_type = AMDGV_IP_DISCOVERY_LOAD_VIA_PSP;

	if (adapt->opt.fw_load_type > AMDGV_FW_LOAD_TYPE_BEGIN &&
		adapt->opt.fw_load_type < AMDGV_FW_LOAD_TYPE_END)
		adapt->fw_load_type = adapt->opt.fw_load_type;
	else
		adapt->fw_load_type = AMDGV_FW_LOAD_BY_GIM;

	if (adapt->opt.mm_policy >= AMDGV_MM_ALL_ASSIGNMENT_FOR_ONE_VF &&
		adapt->opt.mm_policy < AMDGV_MM_MAX_ASSIGNMENT_FOR_ONE_VF)
		adapt->mm_policy = adapt->opt.mm_policy;
	else
		adapt->mm_policy = AMDGV_MM_ALL_ASSIGNMENT_FOR_ONE_VF;

	if (adapt->opt.fb_sharing_mode < AMDGV_XGMI_FB_SHARING_MODE_UNKNOWN) {
		adapt->xgmi.fb_sharing_mode = adapt->opt.fb_sharing_mode;
	} else {
		adapt->xgmi.fb_sharing_mode =
			AMDGV_XGMI_FB_SHARING_MODE_DEFAULT;
	}

	if (adapt->opt.ras_vf_telemetry_policy < AMDGV_RAS_VF_TELEMETRY_POLICY_COUNT)
		adapt->mca.vf_policy = adapt->opt.ras_vf_telemetry_policy;

	adapt->mcp.memory_partition_mode = adapt->opt.memory_partition_mode;
	adapt->mcp.accelerator_partition_mode = adapt->opt.accelerator_partition_mode;

	if (adapt->opt.hang_detection_mode) {
		adapt->flags |= AMDGV_FLAG_ENABLE_HANG_DETECTION;
	}
	if (!(adapt->flags & AMDGV_FLAG_USE_PF)) {
		adapt->flags |= AMDGV_FLAG_DISABLE_SDMA_ENGINE;
	}
	if (adapt->opt.partition_full_access_enable) {
		adapt->flags |= AMDGV_FLAG_ENABLE_PARTITION_FULL_ACCESS;
	}

	if (adapt->opt.debug_dump_reserve_size > AMDGV_AUTO_SCHED_DEBUG_DUMP_MAX_SIZE) {
		AMDGV_WARN("Debug dump size exceeds max size, setting to 256MB.\n");
		adapt->opt.debug_dump_reserve_size = AMDGV_AUTO_SCHED_DEBUG_DUMP_MAX_SIZE;
	}

	if (adapt->opt.asymmetric_fb_mode)
		adapt->asymmetric_fb_enabled = true;

	/* TODO: add sysfs interface to provide time_quanta for each sched_id
	 *  for now, hardcode all the time_quanta_option[4]
	 *   (note: there are 4 entries, each is 8-bit)
	 *   o for GFX to "DEFAULT_GFX_TIME_SLICE" (in ms)
	 *   o for MM to "DEFAULT_MM_TIME_SLICE" (in ms)
	 */
	gfx_tmp = (uint8_t)(DEFAULT_GFX_TIME_SLICE / 1000);
	mm_tmp = (uint8_t)(DEFAULT_MM_TIME_SLICE / 1000);
	gfx_quanta_option = gfx_tmp;
	mm_quanta_option = mm_tmp;
	for (i = 0; i < 4; i++) {
		gfx_quanta_option = (gfx_quanta_option << 8) | gfx_tmp;
		mm_quanta_option = (mm_quanta_option << 8) | mm_tmp;
	}
	for (i = 0; i < AMDGV_SCHED_BLOCK_MAX; i++) {
		if (i == AMDGV_SCHED_BLOCK_GFX)
			adapt->time_quanta_option[i] = gfx_quanta_option;
		else
			adapt->time_quanta_option[i] = mm_quanta_option;
	}

	if (adapt->flags & AMDGV_FLAG_DYNAMIC_TIME_SLICE)
		AMDGV_INFO("Dynamic time slice enabled.\n");
	else
		AMDGV_INFO("Dynamic time slice disabled.\n");

	AMDGV_INFO("%d VF will be enabled.\n", adapt->num_vf);
	return 0;
}

void amdgv_print_failed_init_name(struct amdgv_adapter *adapt, bool is_sw,
				  const char *func_name)
{
	const char *type = is_sw ? "sw" : "hw";

	AMDGV_PRINT("failed to %s_init of %s\n", type, func_name);
}

static const uint32_t amdgv_ranges[] = {
	22,   27,   33,	  41,	51,   63,   77,	  96,	 118,	147,   181,
	224,  278,  344,  426,	527,  653,  808,  1001,	 1239,	1535,  1901,
	2354, 2915, 3609, 4470, 5535, 6855, 8489, 10513, 13019, 16122,
};

static void amdgv_init_histogram_range(struct amdgv_histogram *hist)
{
	int i;

	/* log16 */
	if (hist->interval) {
		for (i = 0; i < AMDGV_HISTOGRAM_SIZE; i++) {
			hist->range[i] = hist->start + i * hist->interval;
			hist->count[i] = 0;
		}
	} else {
		hist->start = 0;

		for (i = 0; i < AMDGV_HISTOGRAM_SIZE; i++) {
			hist->range[i] = amdgv_ranges[hist->start + i];
			hist->count[i] = 0;
		}
	}
	hist->total = 0;
}

void amdgv_time_log_increase_vf_reset_cnt(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		adapt->array_vf[idx_vf].time_log[hw_sched_id].reset_count++;
	}
}

void amdgv_time_log_clear_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		/* clear VF's time log and FLR count */
		oss_memset(&adapt->array_vf[idx_vf].time_log[hw_sched_id], 0,
			   sizeof(struct amdgv_time_log));
	}
}

void amdgv_time_log_note_vf_init_start(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		/* note the timestamp for gpu monitoring */
		adapt->array_vf[idx_vf].time_log[hw_sched_id].init_start =
			oss_get_time_stamp();
	}
}

void amdgv_time_log_note_vf_init_end(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		/* note the timestamp for gpu monitoring */
		adapt->array_vf[idx_vf].time_log[hw_sched_id].init_end = oss_get_time_stamp();
	}
}

void amdgv_time_log_note_vf_fini_start(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		/* note the timestamp for gpu monitoring */
		adapt->array_vf[idx_vf].time_log[hw_sched_id].finish_start =
			oss_get_time_stamp();
	}
}

void amdgv_time_log_note_vf_fini_end(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		/* note the timestamp for gpu monitoring */
		adapt->array_vf[idx_vf].time_log[hw_sched_id].finish_end =
			oss_get_time_stamp();
	}
}

void amdgv_time_log_update_vf_cumulative_running_time(struct amdgv_adapter *adapt,
							  uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		adapt->array_vf[idx_vf].time_log[hw_sched_id].cumulative_running_time +=
			adapt->array_vf[idx_vf].time_log[hw_sched_id].finish_end -
			adapt->array_vf[idx_vf].time_log[hw_sched_id].init_start;
	}
}

void amdgv_time_log_update_vf_cumulative_running_time_after_flr(struct amdgv_adapter *adapt,
							  uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		adapt->array_vf[idx_vf].time_log[hw_sched_id].cumulative_running_time +=
			oss_get_time_stamp() - adapt->array_vf[idx_vf].time_log[hw_sched_id].init_start;
	}
}

void amdgv_time_log_note_vf_reset_start(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		/* note the timestamp for gpu monitoring */
		adapt->array_vf[idx_vf].time_log[hw_sched_id].reset_start =
			oss_get_time_stamp();
	}
}

void amdgv_time_log_note_vf_reset_end(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t hw_sched_id;

	for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		/* note the timestamp for gpu monitoring */
		adapt->array_vf[idx_vf].time_log[hw_sched_id].reset_end = oss_get_time_stamp();
	}
}

static void amdgv_init_histogram(struct amdgv_adapter *adapt)
{
	int idx_vf;
	uint32_t hw_sched_id;

	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++) {
		for (hw_sched_id = 0; hw_sched_id < AMDGV_MAX_NUM_HW_SCHED; hw_sched_id++) {
			adapt->array_vf[idx_vf]
				.time_log[hw_sched_id]
				.gfx_load_latency_us.interval = 0;
			adapt->array_vf[idx_vf]
				.time_log[hw_sched_id]
				.gfx_idle_latency_us.interval = 0;
			adapt->array_vf[idx_vf]
				.time_log[hw_sched_id]
				.gfx_save_latency_us.interval = 0;
			adapt->array_vf[idx_vf]
				.time_log[hw_sched_id]
				.gfx_run_latency_us.interval = 0;
			adapt->array_vf[idx_vf]
				.time_log[hw_sched_id]
				.gfx_run_summation_us.start = 3000;
			adapt->array_vf[idx_vf]
				.time_log[hw_sched_id]
				.gfx_run_summation_us.interval = 250;

			amdgv_init_histogram_range(&adapt->array_vf[idx_vf]
								.time_log[hw_sched_id]
								.gfx_load_latency_us);
			amdgv_init_histogram_range(&adapt->array_vf[idx_vf]
								.time_log[hw_sched_id]
								.gfx_idle_latency_us);
			amdgv_init_histogram_range(&adapt->array_vf[idx_vf]
								.time_log[hw_sched_id]
								.gfx_save_latency_us);
			amdgv_init_histogram_range(&adapt->array_vf[idx_vf]
								.time_log[hw_sched_id]
								.gfx_run_latency_us);
			amdgv_init_histogram_range(&adapt->array_vf[idx_vf]
								.time_log[hw_sched_id]
								.gfx_run_summation_us);
		}
	}
}

#define MAX_INIT_FUNCS 50
static int amdgv_device_func_sw_init(struct amdgv_adapter *adapt)
{
	int i, j;
	struct amdgv_init_func *init_func;
	bool imported_memmgr = false;
	bool sw_init_complete[MAX_INIT_FUNCS] = { 0 };
	bool hw_init_complete[MAX_INIT_FUNCS] = { 0 };

	if (amdgv_error_init(adapt) == AMDGV_FAILURE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ERROR_LOGGING_FAILED, 0);
		return AMDGV_FAILURE;
	}

	if (adapt->num_funcs > MAX_INIT_FUNCS) {
		AMDGV_ERROR("There are more init functions than supported! (%d > %d)\n",
				adapt->num_funcs, MAX_INIT_FUNCS);
		return AMDGV_FAILURE;
	}

	/* sw init */
	for (i = 0; i < adapt->num_funcs; i++) {
		init_func = adapt->init_funcs[i];
		AMDGV_INFO("start sw_init of %s\n", init_func->name);
		if (init_func->sw_init && init_func->sw_init(adapt) < 0) {
			amdgv_print_failed_init_name(adapt, true, init_func->name);
			goto init_fail;
		}
		sw_init_complete[i] = true;

		if (adapt->opt.skip_hw_init && adapt->memmgr_pf.is_init && !imported_memmgr) {
			if (amdgv_import_data_by_op(adapt, AMDGV_LIVE_INFO_DATA__MEMMGR)) {
				init_func->sw_fini(adapt);
				init_func->sw_init(adapt);
			}
			imported_memmgr = true;
		}

		if (init_func->hw_priority) {
			AMDGV_INFO("start hw_init of %s\n", init_func->name);
			if (init_func->hw_init && init_func->hw_init(adapt) < 0) {
				amdgv_print_failed_init_name(adapt, false, init_func->name);
				goto init_fail;
			}
			hw_init_complete[i] = true;
		}
	}

	/* histogram init*/
	amdgv_init_histogram(adapt);

	return 0;

init_fail:
	for (j = i; j >= 0; j--) {
		/* hw fini */
		if (adapt->init_funcs[j]->hw_priority && hw_init_complete[j]) {
			if (adapt->init_funcs[j]->hw_fini) {
				AMDGV_INFO("start hw_fini of %s\n",
					   adapt->init_funcs[j]->name);
				adapt->init_funcs[j]->hw_fini(adapt);
				hw_init_complete[j] = false;
			}
		}
		/* sw fini */
		if (adapt->init_funcs[j]->sw_fini && sw_init_complete[j]) {
			AMDGV_INFO("start sw_fini of %s\n", adapt->init_funcs[j]->name);
			adapt->init_funcs[j]->sw_fini(adapt);
			sw_init_complete[j] = false;
		}
	}

	amdgv_error_fini(adapt);

	return AMDGV_FAILURE;
}

static int amdgv_device_func_hw_init(struct amdgv_adapter *adapt)
{
	int j;
	struct amdgv_init_func *init_func;

	if (adapt->opt.skip_hw_init) {
		AMDGV_INFO("Skip hw init for live update.\n");

		if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) {
			int ret;
			uint32_t world_switch_id;
			uint32_t hw_sched_id;
			struct amdgv_sched_world_switch *world_switch;

			AMDGV_INFO("Enable SRIOV for GPUV.\n");
			ret = oss_pci_enable_sriov(adapt->dev, adapt->num_vf);
			if (ret)
				AMDGV_ERROR("Enable SRIOV for GPUV failed\n");

			if ((adapt->flags & AMDGV_FLAG_DISABLE_SDMA_ENGINE) &&
				(adapt->flags & AMDGV_FLAG_DISABLE_COMPUTE_ENGINE)) {
				// Prep PF Shutdown: idle, save
				for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, AMDGV_PF_IDX)) {
					world_switch = &adapt->sched.world_switch[world_switch_id];
					for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
						adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd = AMDGV_RUN_GPU;
					}
				}
				ret = amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL);

				ret = amdgv_sched_shutdown_vf(adapt, AMDGV_PF_IDX);
				ret = amdgv_sched_init_pf_state(adapt);

				if (ret)
					AMDGV_ERROR("Init PF state failed after GPUV live update, status: 0x%x\n", ret);
			} else {
				amdgv_device_func_hw_live_init(adapt);
			}
		}

		return 0;
	}

	/* hw init */
	for (j = 0; j < adapt->num_funcs; j++) {
		init_func = adapt->init_funcs[j];

		if (init_func->hw_priority)
			continue;

		if (init_func->hw_init) {
			AMDGV_INFO("start hw_init of %s\n", init_func->name);
			if (init_func->hw_init(adapt) < 0) {
				amdgv_print_failed_init_name(adapt, false, init_func->name);
				goto hw_init_fail;
			}
		} else if (init_func->hw_engine_init) {
			AMDGV_INFO("start hw_engine_init of %s\n", init_func->name);
			if (init_func->hw_engine_init(adapt) < 0) {
				amdgv_print_failed_init_name(adapt, false, init_func->name);
				goto hw_init_fail;
			}
		}
	}

	if (adapt->continue_init_other_block_hw == true)
		goto hw_init_fail;

	return 0;

hw_init_fail:
	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_HW_INIT_FAIL, 0);
	/* Collect the diagnosis data logs */
	if (amdgv_diag_data_cache_dump(adapt, AMDGV_PF_IDX,
						AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_INIT_FAIL))
		AMDGV_WARN("Can't collect HW init fail debug log\n");
	for (j = j - 1; j >= 0; j--) {
		if (adapt->init_funcs[j]->hw_fini) {
			AMDGV_INFO("start hw_fini of %s\n", adapt->init_funcs[j]->name);
			adapt->init_funcs[j]->hw_fini(adapt);
		} else if (adapt->init_funcs[j]->hw_engine_fini) {
			AMDGV_INFO("start hw_engine_fini of %s\n", adapt->init_funcs[j]->name);
			adapt->init_funcs[j]->hw_engine_fini(adapt);
		}
	}

	return AMDGV_FAILURE;
}

static void amdgv_device_func_sw_fini(struct amdgv_adapter *adapt)
{
	int i;

	/* sw fini */
	for (i = adapt->num_funcs - 1; i >= 0; i--) {
		if (adapt->init_funcs[i]->sw_fini) {
			AMDGV_INFO("start sw_fini of %s\n", adapt->init_funcs[i]->name);
			adapt->init_funcs[i]->sw_fini(adapt);
		}
	}

	amdgv_error_fini(adapt);
}

static void amdgv_device_func_hw_fini(struct amdgv_adapter *adapt)
{
	int i;

	/* hw fini */
	for (i = adapt->num_funcs - 1; i >= 0; i--) {
		if (adapt->init_funcs[i]->hw_fini) {
			AMDGV_INFO("start hw_fini of %s\n", adapt->init_funcs[i]->name);
			adapt->init_funcs[i]->hw_fini(adapt);
		}
	}
}

int amdgv_device_func_hw_engine_init(struct amdgv_adapter *adapt)
{
	struct amdgv_init_func *init_func;
	int ret = 0, i;

	/* hw engine init */
	for (i = 0; i < adapt->num_funcs; i++) {
		init_func = adapt->init_funcs[i];
		if (init_func->hw_engine_init) {
			AMDGV_INFO("start hw_engine_init of %s\n", init_func->name);
			ret = init_func->hw_engine_init(adapt);
			if (ret < 0) {
				amdgv_print_failed_init_name(adapt, false, init_func->name);
				break;
			}
		}
	}
	return ret;
}

void amdgv_device_func_hw_engine_fini(struct amdgv_adapter *adapt)
{
	int i;

	/* hw engine fini */
	for (i = adapt->num_funcs - 1; i >= 0; i--) {
		if (adapt->init_funcs[i]->hw_engine_fini) {
			AMDGV_INFO("start hw_engine_fini of %s\n", adapt->init_funcs[i]->name);
			adapt->init_funcs[i]->hw_engine_fini(adapt);
		}
	}
}

int amdgv_device_func_hw_live_init(struct amdgv_adapter *adapt)
{
	struct amdgv_init_func *init_func;
	int ret = 0, i;

	/* hw live init */
	for (i = 0; i < adapt->num_funcs; i++) {
		init_func = adapt->init_funcs[i];
		if (init_func->hw_live_init) {
			AMDGV_INFO("start hw_live_init of %s\n", init_func->name);
			ret = init_func->hw_live_init(adapt);
			if (ret < 0) {
				amdgv_print_failed_init_name(adapt, false, init_func->name);
				break;
			}
		}
	}
	return ret;
}

static void amdgv_device_cleanup(struct amdgv_adapter *adapt)
{
	int i;
	struct amdgv_hive_info *hive;
	hive = NULL;

	amdgv_free_config_opt(adapt);

	if (adapt->mmio_idx_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->mmio_idx_lock);
		adapt->mmio_idx_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->pcie_idx_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->pcie_idx_lock);
		adapt->pcie_idx_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->smu_msg_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->smu_msg_lock);
		adapt->smu_msg_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->api_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->api_lock);
		adapt->api_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->set_vf_access_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->set_vf_access_lock);
		adapt->set_vf_access_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->psp_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->psp_lock);
		adapt->psp_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->smu_i2c_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->smu_i2c_lock);
		adapt->smu_i2c_lock = OSS_INVALID_HANDLE;
	}

	if (adapt->srbm_mutex != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->srbm_mutex);
		adapt->srbm_mutex = OSS_INVALID_HANDLE;
	}

	if (adapt->grbm_idx_mutex != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->grbm_idx_mutex);
		adapt->grbm_idx_mutex = OSS_INVALID_HANDLE;
	}

	if (adapt->bp_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(adapt->bp_lock);
		adapt->bp_lock = OSS_INVALID_HANDLE;
	}

	for (i = 0; i < AMDGV_MAX_NUM_HW_SCHED; ++i) {
		if (adapt->sched.hw_state_machine[i].ws_lock != OSS_INVALID_HANDLE) {
			oss_rwsema_fini(adapt->sched.hw_state_machine[i].ws_lock);
			adapt->sched.hw_state_machine[i].ws_lock = OSS_INVALID_HANDLE;
		}
	}

	/* Clean up mcm_xgmi_lock only if current adapter is last one being removed */
	hive = amdgv_get_xgmi_hive(adapt);
	if (hive && hive->number_adapters == 0 && hive->mcm_hive_lock != OSS_INVALID_HANDLE) {
		oss_mutex_fini(hive->mcm_hive_lock);
		hive->mcm_hive_lock = OSS_INVALID_HANDLE;
	}
#ifdef WS_RECORD
	oss_free(adapt->record_buf);
	oss_free(adapt->auto_ws_record_buf);
#endif
	oss_free(adapt);
}

struct amdgv_adapter *amdgv_device_internal_init(struct amdgv_init_data *init_data)
{
	struct amdgv_adapter *adapt = NULL;
	struct amdgv_asic_entry *asic_entry;
	uint16_t vbios_support_vf_num;
	uint32_t offset;
	int i;
	int ret;
	struct amdgv_lock_sched_opt opt;
	opt.is_live_update = true;

	/* allocate adapter storage for the device */
	adapt = (struct amdgv_adapter *)oss_zalloc(sizeof(struct amdgv_adapter));
	if (adapt == NULL) {
		AMDGV_PRINT(
			"Cannot allocate memory for adapter during device initialization\n");
		return NULL;
	}

#ifdef WS_RECORD
	adapt->record_buf = (char *)oss_zalloc(MAX_RECORD_LENGTH);
	if (adapt->record_buf == NULL) {
		AMDGV_PRINT("Cannot allocate memory for ws_record buffer during device initialization\n");
		return NULL;
	}
	adapt->auto_ws_record_buf = (char *)oss_zalloc(MAX_RECORD_LENGTH);
	if (adapt->auto_ws_record_buf == NULL) {
		AMDGV_PRINT("Cannot allocate memory for ws_record buffer during device initialization\n");
		return NULL;
	}
#endif

	/* parse init data, fill in device info */
	amdgv_fill_device_info(adapt, init_data);
	adapt->error_dump_stack_max = 0;
	adapt->error_dump_stack_count = 0;
	amdgv_initialize_default_filters(adapt);

	/* check if the device is supported by libgv */
	asic_entry = amdgv_match_asic_table(init_data->info.dev_id, init_data->info.rev_id);
	if (asic_entry == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPU_NOT_SUPPORTED,
				init_data->info.dev_id);
		goto fail;
	}

	/* check GPU sriov capability */
	if (init_data->info.sriov_cap_pos == 0) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_IOV_ASIC_NO_SRIOV_SUPPORT, 0);
		goto fail;
	}

	if (init_data->info.sriov_vf_stride != 1) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_IOV_SRIOV_STRIDE_ERROR, 0);
		goto fail;
	}

	if (init_data->opt.log_level < 0) {
		/* set the default log level and mask */
		adapt->log_level = AMDGV_INFO_LEVEL;
		adapt->log_mask = AMDGV_ALL_BLOCK;
	} else {
		adapt->log_level = init_data->opt.log_level;
		adapt->log_mask = init_data->opt.log_mask;
	}

	adapt->force_switch_vf_idx = AMDGV_MAX_VF_SLOT;

	adapt->mmio_idx_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
	if (adapt->mmio_idx_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		goto fail;
	}

	adapt->pcie_idx_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
	if (adapt->pcie_idx_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		goto fail;
	}

	adapt->smu_msg_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_HIGHEST_RANK);
	if (adapt->smu_msg_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		goto fail;
	}

	adapt->api_lock = oss_mutex_init();
	if (adapt->api_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	adapt->set_vf_access_lock = oss_mutex_init();
	if (adapt->set_vf_access_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	adapt->psp_lock = oss_mutex_init();
	if (adapt->psp_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	adapt->smu_i2c_lock = oss_mutex_init();
	if (adapt->smu_i2c_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	adapt->srbm_mutex = oss_mutex_init();
	if (adapt->srbm_mutex == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	adapt->grbm_idx_mutex = oss_mutex_init();
	if (adapt->grbm_idx_mutex == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	adapt->bp_lock = oss_mutex_init();
	if (adapt->bp_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	for (i = 0; i < AMDGV_MAX_NUM_HW_SCHED; ++i) {
		adapt->sched.hw_state_machine[i].ws_lock = oss_rwsema_init();
		if (adapt->sched.hw_state_machine[i].ws_lock == OSS_INVALID_HANDLE) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_RWSEMA_FAIL,
					0);
			goto fail;
		}
	}

	adapt->init_funcs = *(asic_entry->init_funcs);
	adapt->num_funcs = amdgv_count_array(adapt->init_funcs);

	adapt->live_info_funcs = *(asic_entry->live_info_funcs);

	adapt->miti_table = *(asic_entry->miti_table);
	adapt->num_miti = amdgv_count_array(adapt->miti_table);

	adapt->reg_base_init = asic_entry->reg_base_init;
	adapt->asic_type = asic_entry->asic_type;

	adapt->ip_discovery.size = DEFAULT_IP_DISCOVERY_SIZE;

	adapt->in_sync_flood = &adapt->sync_flood;
	adapt->in_ecc_recovery = &adapt->ecc_recovery;
	if ((adapt->asic_type == CHIP_MI300X) || (adapt->asic_type == CHIP_MI308X)) {
		adapt->ip_discovery.size = MI300_IP_DISCOVERY_SIZE;
	}
	AMDGV_INFO("IP Discovery Size being used: %d\n", adapt->ip_discovery.size);

	offset = adapt->sriov_cap_pos + PCIE_EXT_SRIOV_TOTALVF;
	oss_pci_read_config_word(adapt->dev, offset, &vbios_support_vf_num);
	adapt->max_num_vf = vbios_support_vf_num;
	amdgv_device_set_status(adapt, AMDGV_STATUS_INVALID);

	/* copy config options */
	if (amdgv_copy_config_opt(adapt, init_data) < 0)
		goto fail;

	/* parse config option */
	if (amdgv_parse_config_opt(adapt) < 0) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_INIT_CONFIG_ERROR, 0);
		goto fail;
	}
	adapt->hash_addr = LIVE_INFO_HASH_ADDR;
	/* clear the hash_addr data */
	oss_pci_write_config_dword(adapt->dev, adapt->hash_addr, 0);
	/* reg base init first */
	if (adapt->reg_base_init != NULL)
		adapt->reg_base_init(adapt);
	else {
		AMDGV_WARN("please add reg_base_init() for your ASIC\n");
		goto fail;
	}

	/* diagnosis data initialization */
	if (amdgv_diag_data_init(adapt) < 0)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_DIAG_DATA_INIT_FAIL, 0);

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) {
		if (adapt->opt.skip_hw_init)
			adapt->live_update_state = AMDGV_LIVE_UPDATE_RESTORE;
	} else {
		adapt->in_live_update = adapt->opt.skip_hw_init ? true : false;
	}

	amdgv_import_data_by_op(adapt, AMDGV_LIVE_INFO_DATA__MODULE_PARAM_PRE);

	/* device function initialization */
	if (amdgv_device_func_sw_init(adapt) < 0) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_SW_INIT_FAIL, 0);
		goto fail;
	}
	amdgv_device_set_status(adapt, AMDGV_STATUS_SW_INIT);

	ret = amdgv_import_data(adapt);
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE)) {
		if (ret != AMDGV_LIVE_INFO_STATUS_SUCCESS)
			goto fail;
	}

	/* device function initialization */
	if (amdgv_device_func_hw_init(adapt) == 0) {
		amdgv_device_set_status(adapt, AMDGV_STATUS_HW_INIT);
		oss_signal_event(adapt->sched.event);
	}

	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE)) {
		if (adapt->opt.skip_hw_init) {
			AMDGV_DEBUG("Resume sched.\n");
			amdgv_unlock_sched_ex(adapt, opt);
			amdgv_import_data_by_op(adapt, AMDGV_LIVE_INFO_DATA__UNPROCESSED_EVENT);
			AMDGV_DEBUG("Enable interrupt.\n");
			amdgv_toggle_interrupt(adapt, true);
			adapt->in_live_update = false;
			adapt->flags |= AMDGV_FLAG_MIDDLE_OF_LIVE_UPDATE;
		}
	}
	return adapt;

fail:
	/* Collect the diagnosis data logs */
	if (amdgv_diag_data_cache_dump(adapt, AMDGV_PF_IDX,
						AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_INIT_FAIL))
		AMDGV_WARN("Unable to collect diagnosis data log on VF FLR\n");
	/* Check within the API if the diagnosis data has initialized */
	amdgv_diag_data_fini(adapt);
	/* Check within the API if the diagnosis data has initialized */
	amdgv_device_cleanup(adapt);

	return NULL;
}

static void amdgv_device_live_update_pre_fini(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *entry;
	bool all_gpu_idle;

	if (adapt->in_chain_live_update)
		return;

	hive = amdgv_get_xgmi_hive(adapt);
	if ((adapt->xgmi.phy_nodes_num > 1) && hive) {
		/* If all the adapters are in the same hive, we need to
		 * disable all the adapters' interrupt, and make sure event
		 * queue is empty before executing sw_fini. Otherwise, if a
		 * latter adapter triggers a chained whole gpu reset, the first
		 * few adapters may not respond because they might be already
		 * finished sw_fini.
		 */
		amdgv_list_for_each_entry(entry, &hive->adapt_list,
					struct amdgv_adapter, xgmi.head) {
			amdgv_toggle_interrupt(entry, false);
			entry->in_chain_live_update = true;
		}

		while (1) {
			all_gpu_idle = 1;
			amdgv_list_for_each_entry(entry, &hive->adapt_list,
				struct amdgv_adapter, xgmi.head) {
					if (entry->event_thread_status != AMDGV_EVENT_THREAD_IDLE) {
						all_gpu_idle = 0;
						/* Skip waiting for full access exit. */
						if (entry->event_thread_status == AMDGV_EVENT_THREAD_WAITING)
							oss_signal_event(entry->sched.event);

						break;
					}
			}

			if (all_gpu_idle)
				break;

			oss_msleep(5);
		}
	} else {
		amdgv_toggle_interrupt(adapt, false);
	}
}

void amdgv_device_internal_fini(struct amdgv_adapter *adapt,
				struct amdgv_fini_config_opt *fini_opt)
{
	struct amdgv_lock_sched_opt opt;
	/* disable hang debug forcibly on unload */
	amdgv_debug_set_mode(adapt, AMDGV_DEBUG_MODE_DEFAULT);

	/* disable bp mode */
	adapt->bp_mode = 0;

	oss_memcpy(&adapt->fini_opt, fini_opt, sizeof(struct amdgv_fini_config_opt));

	// Currently does not apply for GPUV
	opt.is_live_update = true;

	if (adapt->fini_opt.skip_hw_fini) {
		amdgv_device_live_update_pre_fini(adapt);
		amdgv_lock_sched_ex(adapt, opt);
		amdgv_live_info_init_metadata(adapt);
		amdgv_export_data(adapt);
	}

	if (adapt->status != AMDGV_STATUS_SW_INIT) {
		if (!adapt->fini_opt.skip_hw_fini) {
			/* we are going to hw_fini, set this
			 * to notify all other threads stop touching hardware */
			adapt->status = AMDGV_STATUS_HW_FINI;
			amdgv_device_func_hw_fini(adapt);
			amdgv_device_func_hw_engine_fini(adapt);
		} else
			AMDGV_INFO("Skip HW fini.\n");
	}

	adapt->status = AMDGV_STATUS_SW_FINI;
	amdgv_device_func_sw_fini(adapt);

	amdgv_diag_data_fini(adapt);

	oss_memcpy(fini_opt, &adapt->fini_opt, sizeof(struct amdgv_fini_config_opt));

	amdgv_device_cleanup(adapt);
}

int amdgv_get_supported_dev_ids(struct amdgv_device_ids *dev_ids, int length)
{
	int i, j;
	struct amdgv_asic_entry *asic_entry;
	struct amdgv_device_ids *dev_entry;

	if (ARRAY_SIZE(amdgv_sriov_device_table) > length)
		return AMDGV_FAILURE;

	for (i = 0; i < ARRAY_SIZE(amdgv_sriov_device_table); i++) {
		asic_entry = (struct amdgv_asic_entry *)&amdgv_sriov_device_table[i];
		dev_entry = (struct amdgv_device_ids *)&dev_ids[i];

		dev_entry->vendor_id = PCI_VENDOR_ID_ATI;
		dev_entry->dev_id = asic_entry->dev_id;
		dev_entry->sub_vendor_id = ANY_ID;
		dev_entry->sub_dev_id = asic_entry->sub_dev_id;
		dev_entry->rev_id = asic_entry->rev_id;
	}

	for (j = i; j < length; j++) {
		dev_entry = (struct amdgv_device_ids *)&dev_ids[j];

		dev_entry->vendor_id = 0;
		dev_entry->dev_id = 0;
		dev_entry->sub_vendor_id = 0;
		dev_entry->sub_dev_id = 0;
		dev_entry->rev_id = 0;
	}
	return 0;
}

void amdgv_program_register_sequence(struct amdgv_adapter *adapt,
					 const struct amdgv_reg_golden *regs,
					 const uint32_t array_size)
{
	uint32_t i;
	uint32_t tmp, reg;
	const struct amdgv_reg_golden *entry;

	for (i = 0; i < array_size; ++i) {
		entry = &regs[i];
		reg = adapt->reg_offset[entry->hwip][entry->instance][entry->segment] +
			  entry->reg;

		if (entry->and_mask == 0xffffffff) {
			tmp = entry->or_mask;
		} else {
			tmp = RREG32(reg);
			tmp &= ~(entry->and_mask);
			tmp |= entry->and_mask & entry->or_mask;
		}

		WREG32(reg, tmp);
	}
}

static void amdgv_device_dispatch_rma(struct amdgv_adapter *adapt)
{
	if (adapt->status == AMDGV_STATUS_SW_INIT) {
		AMDGV_ERROR("RMA detected during init\n");
		amdgv_sched_queue_event_no_signal(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_RMA, 0);
	} else if (adapt->status == AMDGV_STATUS_HW_INIT) {
		AMDGV_ERROR("RMA detected at runtime\n");
		amdgv_sched_queue_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_RMA, 0);
	}
}

void amdgv_device_handle_bad_gpu(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *entry, *tmp;

	hive = amdgv_get_xgmi_hive(adapt);
	if (hive) {
		amdgv_xgmi_mark_bad_gpu_in_hive(adapt);
		amdgv_list_for_each_entry_safe(entry, tmp, &hive->adapt_list,
						struct amdgv_adapter, xgmi.head) {
				if (amdgv_xgmi_is_fb_sharing_allowed(adapt,
						adapt->xgmi.phy_node_id,
						entry->xgmi.phy_node_id,
						adapt->xgmi.fb_sharing_mode))
							amdgv_device_dispatch_rma(entry);
		}
	} else {
		amdgv_device_dispatch_rma(adapt);
	}
}

/* Check if any GPU is in RMA state, and if so,
 * dispatch RMA events to all FB shared devices */
void amdgv_device_handle_bad_hive(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *entry, *tmp;
	int fb_sharing_gpus[AMDGV_MAX_XGMI_HIVE] = {0};
	int i;

	hive = amdgv_get_xgmi_hive(adapt);
	if (hive) {
		amdgv_list_for_each_entry_safe(entry, tmp, &hive->adapt_list,
					       struct amdgv_adapter, xgmi.head) {
			if (amdgv_ras_eeprom_is_gpu_bad(entry)) {
				fb_sharing_gpus[entry->xgmi.phy_node_id]++;
				for (i = 0; i < AMDGV_MAX_XGMI_HIVE; i++) {
					if (amdgv_xgmi_is_fb_sharing_allowed(adapt,
							entry->xgmi.phy_node_id,
							i,
							entry->xgmi.fb_sharing_mode))
						fb_sharing_gpus[i]++;
				}
			}
		}
		amdgv_list_for_each_entry_safe(entry, tmp, &hive->adapt_list,
					struct amdgv_adapter, xgmi.head) {
			if (fb_sharing_gpus[entry->xgmi.phy_node_id] > 0) {
				amdgv_device_dispatch_rma(entry);
			}
		}
	}
}

int amdgv_device_generate_rma_cper(struct amdgv_adapter *adapt)
{
	struct cper_hdr *bad_page_thr = NULL;

	bad_page_thr = amdgv_cper_alloc_entry(adapt, AMDGV_CPER_TYPE_BP_THR, 1);
	if (!bad_page_thr)
		return AMDGV_FAILURE;

	amdgv_cper_entry_fill_hdr(adapt, bad_page_thr, AMDGV_CPER_TYPE_BP_THR, CPER_SEV_FATAL);
	amdgv_cper_entry_fill_bad_page_thr_section(adapt, bad_page_thr, 0);

	amdgv_cper_commit_entry(adapt, bad_page_thr);

	return 0;
}

/* Different program PMFWs have different interfaces */
void amdgv_device_report_rma_to_fw(struct amdgv_adapter *adapt)
{
	if (amdgv_ras_eeprom_is_gpu_bad(adapt)) {
		if (adapt->pp.pp_funcs->send_rma_reason)
			adapt->pp.pp_funcs->send_rma_reason(adapt, PP_RMA_BAD_PAGE_THRESHOLD);
		if (adapt->gpumon.funcs->ras_report)
			adapt->gpumon.funcs->ras_report(adapt, (int)PP_RAS_TYPE__RMA);
	}
}

static void amdgv_device_set_gpu_rma(struct amdgv_adapter *adapt)
{
	if (adapt->status != AMDGV_STATUS_HW_RMA)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPU_RMA, 0);

	adapt->status = AMDGV_STATUS_HW_RMA;
	amdgv_sched_set_unrecov_err(adapt);
}

static void amdgv_device_set_hive_rma(struct amdgv_adapter *adapt)
{
	if (adapt->status != AMDGV_STATUS_HW_HIVE_RMA)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPU_HIVE_RMA, 0);

	adapt->status = AMDGV_STATUS_HW_HIVE_RMA;
	amdgv_sched_set_unrecov_err(adapt);
}

static void amdgv_device_set_gpu_lost(struct amdgv_adapter *adapt)
{
	if (adapt->status != AMDGV_STATUS_HW_LOST)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPU_DEVICE_LOST, 0);

	adapt->status = AMDGV_STATUS_HW_LOST;
	amdgv_sched_set_unrecov_err(adapt);
}

void amdgv_device_set_status(struct amdgv_adapter *adapt, enum amdgv_dev_status status)
{
	switch (status) {
	case AMDGV_STATUS_INVALID:
	case AMDGV_STATUS_SW_INIT: /* Fallthrough */
	case AMDGV_STATUS_HW_INIT: /* Fallthrough */
	case AMDGV_STATUS_HW_FINI: /* Fallthrough */
	case AMDGV_STATUS_SW_FINI: /* Fallthrough */
		adapt->status = status;
		amdgv_sched_clear_unrecov_err(adapt);
		break;
	case AMDGV_STATUS_HW_LOST:
		amdgv_device_set_gpu_lost(adapt);
		break;
	case AMDGV_STATUS_HW_RMA:
		amdgv_device_set_gpu_rma(adapt);
		break;
	case AMDGV_STATUS_HW_HIVE_RMA:
		amdgv_device_set_hive_rma(adapt);
		break;
	default:
		AMDGV_WARN("Unknown Device Status: %d\n", status);
		break;
	}
}

bool amdgv_device_is_gpu_lost(struct amdgv_adapter *adapt)
{
	return (adapt->status == AMDGV_STATUS_HW_LOST);
}
