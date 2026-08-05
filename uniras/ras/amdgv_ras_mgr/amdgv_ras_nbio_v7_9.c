/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_ras_mgr.h"
#include "amdgv_ras_nbio_v7_9.h"

#define regBIF_BX0_BIF_INTR_CNTL                                 0x0101
#define regBIF_BX0_BIF_INTR_CNTL_BASE_IDX                        2

//BIF_BX0_BIF_INTR_CNTL
#define BIF_BX0_BIF_INTR_CNTL__RAS_INTR_VEC_SEL__SHIFT           0x0
#define BIF_BX0_BIF_INTR_CNTL__RAS_INTR_VEC_SEL_MASK             0x00000001L

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int nbio_v7_9_set_ras_err_event_athub_irq_state(struct ras_core_context *ras_core,
							bool state)
{
	/* The ras_controller_irq enablement should be done in psp bl when it
	 * tries to enable ras feature. Driver only need to set the correct interrupt
	 * vector for bare-metal and sriov use case respectively
	 */
	uint32_t bif_intr_cntl;

	bif_intr_cntl = RAS_DEV_RREG32_SOC15(ras_core->dev, NBIO, 0, regBIF_BX0_BIF_INTR_CNTL);

	if (state) {
		/* set interrupt vector select bit to 1 to select vetcor 4 for sriov case */
		bif_intr_cntl = REG_SET_FIELD(bif_intr_cntl,
					      BIF_BX0_BIF_INTR_CNTL,
					      RAS_INTR_VEC_SEL, 1);

		RAS_DEV_WREG32_SOC15(ras_core->dev, NBIO, 0, regBIF_BX0_BIF_INTR_CNTL, bif_intr_cntl);
	}

	return 0;
}

const struct ras_nbio_sys_func amdgv_ras_nbio_sys_func_v7_9 = {
	.set_ras_err_event_athub_irq_state = nbio_v7_9_set_ras_err_event_athub_irq_state,
};
