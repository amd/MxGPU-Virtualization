/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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
#include "mi300.h"
#include "amdgv_ras.h"
#include "mi300_nbio.h"
#include "mi300/NBIO/nbio_7_9_0_offset.h"
#include "mi300/NBIO/nbio_7_9_0_sh_mask.h"
#include "atombios/atomfirmware.h"

const uint32_t this_block = AMDGV_MEMORY_BLOCK;

bool mi300_nbio_vbios_need_post(struct amdgv_adapter *adapt)
{
	uint32_t val;
	const uint32_t this_block = AMDGV_SECURITY_BLOCK;

	val = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7));

	AMDGV_INFO("BIOS_SCRATCH_7 = 0x%08x\n", val);

	val &= ATOM_ASIC_INIT_COMPLETE;

	if (val) {
		AMDGV_INFO("ATOM_ASIC_POSTED\n");
		return false;
	}

	AMDGV_INFO("ATOM_ASIC_NEED_POST\n");
	return true;
}

void mi300_nbio_enable_doorbell_aperture(struct amdgv_adapter *adapt, bool enable)
{
	WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_PF, (enable ? 0xfffff : 0));

	switch (adapt->num_vf) {
	case 1:
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF0, (enable ? 0xfffff : 0));
		break;

	case 2:
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF0, (enable ? 0x300ff : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF1, (enable ? 0xcff00 : 0));
		break;

	case 4:
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF0, (enable ? 0x1000f : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF1, (enable ? 0x200f0 : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF2, (enable ? 0x40f00 : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF3, (enable ? 0x8f000 : 0));
		break;

	case 8:
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF0, (enable ? 0x10003 : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF1, (enable ? 0x1000c : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF2, (enable ? 0x20030 : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF3, (enable ? 0x2f0c0 : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF4, (enable ? 0x40300 : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF5, (enable ? 0x40c00 : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF6, (enable ? 0x83000 : 0));
		WREG32_SOC15(NBIO, 0, regBIFC_DOORBELL_ACCESS_EN_VF7, (enable ? 0x8c000 : 0));
		break;

	default:
		AMDGV_WARN("Need to add support for Enabling Doorbell for num_vf=%d\n",
			   adapt->num_vf);
	}
}

void mi300_nbio_assign_sdma_to_vf(struct amdgv_adapter *adapt)
{
	switch (adapt->num_vf) {
	case 1:
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF0, 0, 0x76543210);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF0, 0, 0xfedcba98);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF7, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF7, 0, 0x0);
		break;

	case 2:
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF0, 0, 0x76543210);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF0, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF1, 0, 0xfedcba98);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF7, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF7, 0, 0x0);
		break;

	case 4:
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF0, 0, 0x3210);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF0, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF1, 0, 0x7654);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF2, 0, 0xba98);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF3, 0, 0xfedc);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF7, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF7, 0, 0x0);
		break;

	case 8:
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF0, 0, 0x10);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF0, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF1, 0, 0x32);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF2, 0, 0x54);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF3, 0, 0x76);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF4, 0, 0x98);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF5, 0, 0xba);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF6, 0, 0xdc);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF7, 0, 0xfe);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF7, 0, 0x0);
		break;

	default:
		AMDGV_WARN("Need to config the VF SDMA assignment for num_vf=%d\n",
			   adapt->num_vf);
	}
}

void mi308_nbio_assign_sdma_to_vf(struct amdgv_adapter *adapt)
{
	switch (adapt->num_vf) {
	case 1:
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF0, 0, 0x76543210);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF0, 0, 0xfedcba98);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF7, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF7, 0, 0x0);
		break;

	case 2:
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF0, 0, 0x76543210);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF0, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF1, 0, 0xfedcba98);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF7, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF7, 0, 0x0);
		break;

	case 4:
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF0, 0, 0x10);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF0, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF1, 0, 0x76);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF2, 0, 0x98);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF3, 0, 0xfe);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET0_VF7, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regSDMA_TARGET1_VF7, 0, 0x0);
		break;

	default:
		AMDGV_WARN("Need to config the VF SDMA assignment for num_vf=%d\n",
			   adapt->num_vf);
	}
}

void mi300_nbio_assign_vcn_to_vf(struct amdgv_adapter *adapt)
{
	switch (adapt->num_vf) {
	case 1:
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF0, 0, 0x3210);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF7, 0, 0x0);
		break;
	case 2:
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF0, 0, 0x10);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF1, 0, 0x32);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF2, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF3, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF7, 0, 0x0);
		break;
	case 4:
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF0, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF1, 0, 0x1);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF2, 0, 0x2);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF3, 0, 0x3);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF4, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF5, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF6, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF7, 0, 0x0);
		break;
	case 8:
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF0, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF1, 0, 0x0);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF2, 0, 0x1);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF3, 0, 0x1);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF4, 0, 0x2);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF5, 0, 0x2);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF6, 0, 0x3);
		WREG32_SOC15_EXT(NBIO, 0, regVCN_TARGET_VF7, 0, 0x3);
		break;

	default:
		AMDGV_WARN("Need config the VF VCN assignment for num_vf=%d\n", adapt->num_vf);
	}
}

void mi300_nbio_enable_vf_access_mmio_over_512k(struct amdgv_adapter *adapt)
{
	uint32_t val;
	val = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP9));
	val |= RCC_STRAP0_RCC_DEV0_EPF0_STRAP9__STRAP_NBIF_ROM_BAR_DIS_CHICKEN_DEV0_F0_MASK;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP9), val);
}

void mi300_nbio_set_doorbell_fence(struct amdgv_adapter *adapt)
{
	int aid;
	uint32_t val;

	for (aid = 0; aid < adapt->mcp.num_aid; aid++) {
		val = 0xff & ~(adapt->mcp.gfx.xcc_mask);
		if (aid)
			val |= XCC_DOORBELL_FENCE__SHUB_SLV_MODE_MASK;

		WREG32_SOC15_EXT(NBIO, aid, regXCC_DOORBELL_FENCE, aid, val);
	}
}

void mi300_nbio_assign_sdma_doorbell(struct amdgv_adapter *adapt, int instance,
				int doorbell_index, int doorbell_size)
{
	uint32_t doorbell_range = 0, doorbell_ctrl = 0;
	int aid_id, dev_inst;

	dev_inst = GET_INST(SDMA0, instance);
	aid_id = dev_inst / 4;

	doorbell_range = REG_SET_FIELD(doorbell_range, DOORBELL0_CTRL_ENTRY_0,
					BIF_DOORBELL0_RANGE_OFFSET_ENTRY, doorbell_index);
	doorbell_range = REG_SET_FIELD(doorbell_range, DOORBELL0_CTRL_ENTRY_0,
					BIF_DOORBELL0_RANGE_SIZE_ENTRY, doorbell_size);
	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
					S2A_DOORBELL_PORT1_ENABLE, 1);
	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
					S2A_DOORBELL_PORT1_RANGE_SIZE, doorbell_size);

	switch (dev_inst % 4) {
	case 0:
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regDOORBELL0_CTRL_ENTRY_1) + 4 * aid_id,
		       doorbell_range | 0xe0000);

		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWID, 0xe);
		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_RANGE_OFFSET, 0xe);
		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWADDR_31_28_VALUE, 0x1);
		WREG32_SOC15_EXT(NBIO, aid_id, regS2A_DOORBELL_ENTRY_1_CTRL, aid_id,
				doorbell_ctrl);
		break;
	case 1:
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regDOORBELL0_CTRL_ENTRY_2) + 4 * aid_id,
		       doorbell_range | 0x80000);

		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWID, 0x8);
		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_RANGE_OFFSET, 0x8);
		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWADDR_31_28_VALUE, 0x2);

		WREG32_SOC15_EXT(NBIO, aid_id, regS2A_DOORBELL_ENTRY_2_CTRL, aid_id,
				doorbell_ctrl);
		break;
	case 2:
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regDOORBELL0_CTRL_ENTRY_3) + 4 * aid_id,
		       doorbell_range | 0x90000);

		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWID, 0x9);
		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_RANGE_OFFSET, 0x9);
		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWADDR_31_28_VALUE, 0x8);
		WREG32_SOC15_EXT(NBIO, aid_id, regS2A_DOORBELL_ENTRY_5_CTRL, aid_id,
				doorbell_ctrl);
		break;
	case 3:
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regDOORBELL0_CTRL_ENTRY_4) + 4 * aid_id,
		       doorbell_range | 0xa0000);

		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWID, 0xa);
		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_RANGE_OFFSET, 0xa);
		doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_1_CTRL,
						S2A_DOORBELL_PORT1_AWADDR_31_28_VALUE, 0x9);
		WREG32_SOC15_EXT(NBIO, aid_id, regS2A_DOORBELL_ENTRY_6_CTRL, aid_id,
				doorbell_ctrl);
		break;
	default:
		break;
	};

	return;
}

static void mi300_nbio_assign_mmsch_doorbell_vcn(struct amdgv_adapter *adapt, int vcn_inst,
						int doorbell_index)
{
	uint32_t doorbell_range = 0, doorbell_ctrl = 0;

	doorbell_range = REG_SET_FIELD(doorbell_range, DOORBELL0_CTRL_ENTRY_0,
					BIF_DOORBELL0_RANGE_OFFSET_ENTRY, doorbell_index);
	doorbell_range = REG_SET_FIELD(doorbell_range, DOORBELL0_CTRL_ENTRY_0,
					BIF_DOORBELL0_RANGE_SIZE_ENTRY, 16);

	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_4_CTRL,
					S2A_DOORBELL_PORT4_ENABLE, 1);
	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_4_CTRL,
					S2A_DOORBELL_PORT4_AWID, 0x4);
	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_4_CTRL,
					S2A_DOORBELL_PORT4_RANGE_OFFSET, 0x4);
	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_4_CTRL,
					S2A_DOORBELL_PORT4_AWADDR_31_28_VALUE, 0x4);
	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_4_CTRL,
					S2A_DOORBELL_PORT4_NEED_DEDUCT_RANGE_OFFSET, 0x0);
	doorbell_ctrl = REG_SET_FIELD(doorbell_ctrl, S2A_DOORBELL_ENTRY_4_CTRL,
					S2A_DOORBELL_PORT4_64BIT_SUPPORT_DIS, 0x1);

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regDOORBELL0_CTRL_ENTRY_17) + vcn_inst,
			doorbell_range);

	WREG32_SOC15_EXT(NBIO, vcn_inst, regS2A_DOORBELL_ENTRY_4_CTRL, vcn_inst,
			doorbell_ctrl);
}

void mi300_nbio_assign_mmsch_doorbell(struct amdgv_adapter *adapt)
{
	int i, vcn_inst;
	int doorbell_index;

	for (i = 0; i < adapt->config.mm.count[AMDGV_VCN_ENGINE]; i++) {
		vcn_inst = GET_INST(VCN, i);
		doorbell_index = (AMDGV_MI300_DOORBELL_MMSCH0 << 1) + 32 * vcn_inst;
		mi300_nbio_assign_mmsch_doorbell_vcn(adapt, vcn_inst, doorbell_index);
	}

}

uint32_t mi300_nbio_get_config_memsize(struct amdgv_adapter *adapt)
{
	uint32_t memsize; /* in megabytes */

	memsize = RREG32_SOC15(NBIO, 0, regRCC_DEV0_EPF0_RCC_CONFIG_MEMSIZE);

	return memsize;
}

uint32_t mi300_nbio_get_total_vram_size(struct amdgv_adapter *adapt)
{
	return RREG32_SOC15(NBIO, 0, regRCC_DEV0_EPF0_RCC_CONFIG_MEMSIZE);
}

void mi300_nbio_get_vram_vendor(struct amdgv_adapter *adapt)
{
	uint32_t scratch, scratch_vendor_id_mask;

	scratch_vendor_id_mask = 0xF;

	scratch = RREG32_SOC15(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_4);

	adapt->vram_info.vram_vendor = scratch & scratch_vendor_id_mask;
}

void mi300_nbio_enable_pf_rrmt(struct amdgv_adapter *adapt)
{
	uint32_t dist, aid_pf_base, aid_pf_offset, tmp;
	int i;

	dist = regAID1_PF_BASE_ADDR - regAID0_PF_BASE_ADDR;
	aid_pf_base = 0x8000;
	aid_pf_offset = 0x2000;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regAID0_PF_BASE_ADDR) + dist * i,
		       aid_pf_base + aid_pf_offset * i);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regAID0_XCC0_PF_BASE_ADDR) + dist * i,
		       aid_pf_base + aid_pf_offset * i);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regAID0_XCC1_PF_BASE_ADDR) + dist * i,
		       aid_pf_base + aid_pf_offset * i);
	}

	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regNBIF_RRMT_CNTL));
	tmp = REG_SET_FIELD(tmp, NBIF_RRMT_CNTL, RRMT_ENABLE, 1);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regNBIF_RRMT_CNTL), tmp);
}

void mi300_nbio_disable_vf_flr(struct amdgv_adapter *adapt)
{
	uint32_t strap4;

	strap4 = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4));
	strap4 &= ~RCC_STRAP0_RCC_DEV0_EPF0_STRAP4__STRAP_FLR_EN_DEV0_F0_MASK;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4), strap4);
}

int mi300_nbio_pcie_curr_link_speed(struct amdgv_adapter *adapt)
{
	uint32_t reg;
	int speed = 0;

	reg = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_CFG_DEV0_EPF0_0_LINK_STATUS));
	/* This register is not DWORD aligned. Only 2 upper bytes are valid. */
	reg = (reg >> 16);

	speed = REG_GET_FIELD(reg, BIF_CFG_DEV0_RC0_LINK_STATUS, CURRENT_LINK_SPEED);

	return speed;
}

int mi300_nbio_pcie_curr_link_width(struct amdgv_adapter *adapt)
{
	uint32_t reg;

	reg = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_CFG_DEV0_EPF0_0_LINK_STATUS));
	/* This register is not DWORD aligned. Only 2 upper bytes are valid. */
	reg = (reg >> 16);

	return REG_GET_FIELD(reg, BIF_CFG_DEV0_RC0_LINK_STATUS,
			     NEGOTIATED_LINK_WIDTH);
}

int mi300_nbio_get_curr_memory_partition_mode(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode *memory_partition_mode)
{
	uint32_t mem_status;
	uint32_t mem_mode;

	mem_status = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_PARTITION_MEM_STATUS));

	/* Each bit represents a mode 1-8*/
	mem_mode = REG_GET_FIELD(mem_status, BIF_BX_PF0_PARTITION_MEM_STATUS, NPS_MODE);

	switch (mem_mode) {
	case (1 << 0):
		*memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS1;
		break;
	case (1 << 1):
		*memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS2;
		break;
	case (1 << 3):
		*memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS4;
		break;
	case (1 << 7):
		*memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS8;
		break;
	default:
		AMDGV_ERROR("Unknown NPS mode\n");
		return AMDGV_FAILURE;
		break;
	}

	return 0;
}

uint32_t mi300_nbio_get_accelerator_partition_mode(struct amdgv_adapter *adapt)
{
	uint32_t num_xcc_in_xcp;

	num_xcc_in_xcp = RREG32(SOC15_REG_OFFSET(
		GC, GET_INST(GC, 0), regCP_HYP_XCP_CTL));
	num_xcc_in_xcp = REG_GET_FIELD(num_xcc_in_xcp,
		CP_HYP_XCP_CTL, NUM_XCC_IN_XCP);
	if (num_xcc_in_xcp > adapt->mcp.gfx.num_xcc)
		AMDGV_ERROR("Exceed the limit %d, num_xcc_in_xcp=%u\n", adapt->mcp.gfx.num_xcc, num_xcc_in_xcp);

	switch (num_xcc_in_xcp) {
	case 1:
	case 2:
	case 4:
	case 8:
		return adapt->mcp.gfx.num_xcc / num_xcc_in_xcp;
	default:
		AMDGV_ERROR("Unknown num_xcc_in_xcp=%u\n", num_xcc_in_xcp);
		return 0;
	}
}

/* Accelerator partition mode should be set to the default mode
 * when switching the NPS mode:
 * 			NPS1		NPS4
 * 1VF		SPX			CPX
 * 2VF		DPX			X
 * 4VF		CPX-4/QPX	CPX-4/QPX
 * 8VF		CPX			CPX
 */
uint32_t mi300_nbio_get_accelerator_partition_mode_default_setting(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode)
{
	switch (adapt->num_vf) {
	case 1:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
			return 1;
		case AMDGV_MEMORY_PARTITION_MODE_NPS4:
			return adapt->mcp.gfx.num_xcc;
		default:
			return 0;
		}
	case 2:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
			return 2;
		default:
			return 0;
		}
	case 4:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
			return 4;
		case AMDGV_MEMORY_PARTITION_MODE_NPS4:
			return 4;
		default:
			return 0;
		}
	case 8:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
			return 8;
		case AMDGV_MEMORY_PARTITION_MODE_NPS4:
			return 8;
		default:
			return 0;
		}
	}
	return 0;
}

bool mi300_nbio_is_accelerator_partition_mode_supported(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode,
	uint32_t accelerator_partition_mode)
{
	/* If NPS4 is set, SPX/DPX/TPX should not be allowed
	 * return false for such cases so driver will set to the default
	 * accelerator partition mode for the vf_num and NPS4 mode
	 * i.e., value of accelerator_partition_mode should not be less
	 * than the value of memory_partition_mode
	 */
	if ((accelerator_partition_mode == adapt->num_vf &&
		 memory_partition_mode > AMDGV_MEMORY_PARTITION_MODE_UNKNOWN &&
		 memory_partition_mode < AMDGV_MEMORY_PARTITION_MODE_MAX &&
		 accelerator_partition_mode >= (uint32_t)memory_partition_mode) ||
		accelerator_partition_mode == adapt->mcp.gfx.num_xcc) {
		return true;
	}

	return false;
}

/* For a given vf_num, check if the combination of memory partition mode and
 * accelerator partition mode is supported.
 * 1VF: NPS1 + SPX			NPS1 + CPX			NPS4 + CPX
 * 2VF: NPS1 + DPX			(NPS1 + CPX)
 * 4VF: NPS1 + CPX-4/QPX	NPS4 + CPX-4/QPX
 * 8VF: NPS1 + CPX			NPS4 + CPX
 *
 * If not, driver will reset:
 * 1. memory partition mode to the default mode for the vf_num
 * 2. accelerator partition mode to the default mode for the vf_num and memory
 *    partition mode
 */
bool mi300_nbio_is_partition_mode_combination_supported(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode,
	uint32_t accelerator_partition_mode)
{
	switch (adapt->num_vf) {
	case 1:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
		case AMDGV_MEMORY_PARTITION_MODE_NPS4:
			return mi300_nbio_is_accelerator_partition_mode_supported(
					adapt, memory_partition_mode, accelerator_partition_mode);
		default:
			return false;
		}
	case 2:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
			return mi300_nbio_is_accelerator_partition_mode_supported(
					adapt, memory_partition_mode, accelerator_partition_mode);
		default:
			return false;
		}
	case 4:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
		case AMDGV_MEMORY_PARTITION_MODE_NPS4:
			return mi300_nbio_is_accelerator_partition_mode_supported(
					adapt, memory_partition_mode, accelerator_partition_mode);
		default:
			return false;
		}
	case 8:
		switch (memory_partition_mode) {
		case AMDGV_MEMORY_PARTITION_MODE_NPS1:
		case AMDGV_MEMORY_PARTITION_MODE_NPS4:
			return mi300_nbio_is_accelerator_partition_mode_supported(
					adapt, memory_partition_mode, accelerator_partition_mode);
		default:
			return false;
		}
	}

	return false;
}

#define mmPCIEP_NAK_COUNTER		0x1A340218
#define RX_NUM_NAK_RECEIVED_PORT_SHIFT	0
#define RX_NUM_NAK_RECEIVED_PORT_MASK	0xFFFF
#define RX_NUM_NAK_GENERATED_PORT_SHIFT 16
#define RX_NUM_NAK_GENERATED_PORT_MASK	0xFFFF

int mi300_nbio_get_pcie_replay_count(struct amdgv_adapter *adapt)
{
	uint32_t reg;
	int count = 0;

	reg = RREG32_SMN(mmPCIEP_NAK_COUNTER);

	count = ((reg >> RX_NUM_NAK_RECEIVED_PORT_SHIFT) & RX_NUM_NAK_RECEIVED_PORT_MASK) +
		((reg >> RX_NUM_NAK_GENERATED_PORT_SHIFT) & RX_NUM_NAK_GENERATED_PORT_MASK);

	return count;
}

static void nbio_v7_9_handle_ras_controller_intr_no_bifring(struct amdgv_adapter *adapt)
{
	uint32_t bif_doorbell_intr_cntl = 0;

	bif_doorbell_intr_cntl = RREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL);

	AMDGV_DEBUG("bif_doorbell_int_cntl 0x%x, ras_cntlr_interrupt_status %d\n",
			bif_doorbell_intr_cntl,
			REG_GET_FIELD(bif_doorbell_intr_cntl, BIF_BX0_BIF_DOORBELL_INT_CNTL, RAS_CNTLR_INTERRUPT_STATUS));

	if (REG_GET_FIELD(bif_doorbell_intr_cntl,
		BIF_BX0_BIF_DOORBELL_INT_CNTL, RAS_CNTLR_INTERRUPT_STATUS)) {
		/* driver has to clear the interrupt status when bif ring is disabled */
		bif_doorbell_intr_cntl = REG_SET_FIELD(bif_doorbell_intr_cntl,
						BIF_BX0_BIF_DOORBELL_INT_CNTL,
						RAS_CNTLR_INTERRUPT_CLEAR, 1);

		WREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL, bif_doorbell_intr_cntl);

		/* TODO: handle ras controller interrupt */
	}
}

static void nbio_v7_9_handle_ras_err_event_athub_intr_no_bifring(struct amdgv_adapter *adapt)
{
	uint32_t bif_doorbell_intr_cntl = 0;

	bif_doorbell_intr_cntl = RREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL);

	AMDGV_DEBUG("bif_doorbell_int_cntl 0x%x, ras_athub_err_event_interrupt_status %d\n",
			bif_doorbell_intr_cntl,
			REG_GET_FIELD(bif_doorbell_intr_cntl, BIF_BX0_BIF_DOORBELL_INT_CNTL, RAS_ATHUB_ERR_EVENT_INTERRUPT_STATUS));

	if (REG_GET_FIELD(bif_doorbell_intr_cntl,
		BIF_BX0_BIF_DOORBELL_INT_CNTL, RAS_ATHUB_ERR_EVENT_INTERRUPT_STATUS)) {
		/* driver has to clear the interrupt status when bif ring is disabled */
		bif_doorbell_intr_cntl = REG_SET_FIELD(bif_doorbell_intr_cntl,
						BIF_BX0_BIF_DOORBELL_INT_CNTL,
						RAS_ATHUB_ERR_EVENT_INTERRUPT_CLEAR, 1);

		WREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL, bif_doorbell_intr_cntl);

		AMDGV_INFO("Uncorrectable hardware error(ERREVENT_ATHUB_INTERRUPT) detected!\n");

		/* handle poison consumption check */
		amdgv_ecc_get_poison_stat(adapt);

		oss_atomic_set(adapt->in_sync_flood, 1);
		/* handle global_ras_isr */
		amdgv_ecc_check_global_ras_errors(adapt);
	}
}

static int nbio_v7_9_set_ras_err_event_athub_irq_state(struct amdgv_adapter *adapt,
							bool state)
{
	/* The ras_controller_irq enablement should be done in psp bl when it
	 * tries to enable ras feature. Driver only need to set the correct interrupt
	 * vector for bare-metal and sriov use case respectively
	 */
	uint32_t bif_intr_cntl;

	bif_intr_cntl = RREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_INTR_CNTL);

	if (state == true) {
		/* set interrupt vector select bit to 1 to select
		 * vetcor 4 for sriov case */
		bif_intr_cntl = REG_SET_FIELD(bif_intr_cntl,
					      BIF_BX0_BIF_INTR_CNTL,
					      RAS_INTR_VEC_SEL, 1);

		WREG32_SOC15(NBIO, 0, regBIF_BX0_BIF_INTR_CNTL, bif_intr_cntl);
	}
	return 0;
}

const struct amdgv_nbio_ras nbio_v7_9_ras = {
	.handle_ras_controller_intr_no_bifring = nbio_v7_9_handle_ras_controller_intr_no_bifring,
	.handle_ras_err_event_athub_intr_no_bifring = nbio_v7_9_handle_ras_err_event_athub_intr_no_bifring,
	.set_ras_err_event_athub_irq_state = nbio_v7_9_set_ras_err_event_athub_irq_state,
	.get_curr_memory_partition_mode = mi300_nbio_get_curr_memory_partition_mode,
};

void nbio_v7_9_set_ras_funcs(struct amdgv_adapter *adapt)
{
	adapt->nbio.ras = &nbio_v7_9_ras;
}
