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
 * THE SOFTWARE.
 */

#include <amdgv.h>
#include <amdgv_device.h>

#include <mi200/SDMA0/sdma0_4_2_offset.h>
#include <mi200/SDMA0/sdma0_4_2_sh_mask.h>
#include <mi200/SDMA1/sdma1_4_2_offset.h>
#include <mi200/SDMA1/sdma1_4_2_sh_mask.h>
#include <mi200/SDMA2/sdma2_4_2_2_offset.h>
#include <mi200/SDMA2/sdma2_4_2_2_sh_mask.h>
#include <mi200/SDMA3/sdma3_4_2_2_offset.h>
#include <mi200/SDMA3/sdma3_4_2_2_sh_mask.h>
#include <mi200/SDMA4/sdma4_4_2_2_offset.h>
#include <mi200/SDMA4/sdma4_4_2_2_sh_mask.h>

#include "mi200/GC/gc_9_4_2_offset.h"
#include "mi200/GC/gc_9_4_2_sh_mask.h"

#include "mi200/SMUIO/smuio_13_0_2_offset.h"
#include "mi200/SMUIO/smuio_13_0_2_sh_mask.h"

#include "mi200_golden_settings.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

static const struct amdgv_reg_golden golden_settings_sdma_aldebaran[] = {
	SOC15_REG_GOLDEN_VALUE(SDMA0, 0, mmSDMA0_GB_ADDR_CONFIG, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA0, 0, mmSDMA0_GB_ADDR_CONFIG_READ, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA0, 0, mmSDMA0_UTCL1_TIMEOUT, 0xffffffff, 0x00010001),
	SOC15_REG_GOLDEN_VALUE(SDMA0, 0, mmSDMA0_CNTL, 0xffffffff, 0x40003),
	SOC15_REG_GOLDEN_VALUE(SDMA1, 0, mmSDMA1_GB_ADDR_CONFIG, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA1, 0, mmSDMA1_GB_ADDR_CONFIG_READ, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA1, 0, mmSDMA1_UTCL1_TIMEOUT, 0xffffffff, 0x00010001),
	SOC15_REG_GOLDEN_VALUE(SDMA1, 0, mmSDMA1_CNTL, 0xffffffff, 0x40003),
	SOC15_REG_GOLDEN_VALUE(SDMA2, 0, mmSDMA2_GB_ADDR_CONFIG, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA2, 0, mmSDMA2_GB_ADDR_CONFIG_READ, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA2, 0, mmSDMA2_UTCL1_TIMEOUT, 0xffffffff, 0x00010001),
	SOC15_REG_GOLDEN_VALUE(SDMA2, 0, mmSDMA2_CNTL, 0xffffffff, 0x40003),
	SOC15_REG_GOLDEN_VALUE(SDMA3, 0, mmSDMA3_GB_ADDR_CONFIG, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA3, 0, mmSDMA3_GB_ADDR_CONFIG_READ, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA3, 0, mmSDMA3_UTCL1_TIMEOUT, 0xffffffff, 0x00010001),
	SOC15_REG_GOLDEN_VALUE(SDMA3, 0, mmSDMA3_CNTL, 0xffffffff, 0x40003),
	SOC15_REG_GOLDEN_VALUE(SDMA4, 0, mmSDMA4_GB_ADDR_CONFIG, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA4, 0, mmSDMA4_GB_ADDR_CONFIG_READ, 0x0018773f, 0x00104002),
	SOC15_REG_GOLDEN_VALUE(SDMA4, 0, mmSDMA4_UTCL1_TIMEOUT, 0xffffffff, 0x00010001),
	SOC15_REG_GOLDEN_VALUE(SDMA4, 0, mmSDMA4_CNTL, 0xffffffff, 0x40003),
};

static const struct amdgv_reg_golden golden_settings_gc_9_4_2_alde_die_0[] = {
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_0, 0x3fffffff, 0x141dc920),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_1, 0x3fffffff, 0x3b458b93),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_2, 0x3fffffff, 0x1a4f5583),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_3, 0x3fffffff, 0x317717f6),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_4, 0x3fffffff, 0x107cc1e6),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_5, 0x3ff, 0x351),
};

static const struct amdgv_reg_golden golden_settings_gc_9_4_2_alde_die_1[] = {
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_0, 0x3fffffff, 0x2591aa38),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_1, 0x3fffffff, 0xac9e88b),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_2, 0x3fffffff, 0x2bc3369b),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_3, 0x3fffffff, 0xfb74ee),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_4, 0x3fffffff, 0x21f0a2fe),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_CHAN_STEER_5, 0x3ff, 0x49),
};

static const struct amdgv_reg_golden golden_settings_gc_9_4_2_alde[] = {
	SOC15_REG_GOLDEN_VALUE(GC, 0, regGB_ADDR_CONFIG, 0xffff77ff, 0x2a114042),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTA_CNTL_AUX, 0xfffffeef, 0x10b0000),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCP_UTCL1_CNTL1, 0xffffffff, 0x30800400),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regTCI_CNTL_3, 0xff, 0x20),
};

void mi200_sdma_program_golden_settings(struct amdgv_adapter *adapt)
{
	amdgv_program_register_sequence(adapt, golden_settings_sdma_aldebaran,
		ARRAY_SIZE(golden_settings_sdma_aldebaran));
}

void mi200_gfx_program_golden_settings(struct amdgv_adapter *adapt)
{
	uint32_t data, die_id;

	amdgv_program_register_sequence(adapt, golden_settings_gc_9_4_2_alde,
		ARRAY_SIZE(golden_settings_gc_9_4_2_alde));

	data = RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG);
	die_id = REG_GET_FIELD(data, SMUIO_MCM_CONFIG, DIE_ID);

	/* apply golden settings per die */
	switch (die_id) {
	case 0:
		amdgv_program_register_sequence(adapt,
				golden_settings_gc_9_4_2_alde_die_0,
				ARRAY_SIZE(golden_settings_gc_9_4_2_alde_die_0));
		break;
	case 1:
		amdgv_program_register_sequence(adapt,
				golden_settings_gc_9_4_2_alde_die_1,
				ARRAY_SIZE(golden_settings_gc_9_4_2_alde_die_1));
		break;
	default:
		AMDGV_WARN("invalid die id %d, ignore channel fabricid remap settings\n",
			    die_id);
		break;
	}
}