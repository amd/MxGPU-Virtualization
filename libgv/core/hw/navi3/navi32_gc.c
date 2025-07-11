/*
 * Copyright (C) 2022  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <amdgv_device.h>
#include <amdgv.h>

#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include "navi32_gc.h"

uint32_t gc_v11_0_3_read_grbm_status(struct amdgv_adapter *adapt)
{
    return RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS));
}

uint32_t gc_v11_0_3_read_grbm_status2(struct amdgv_adapter *adapt)
{
    return RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS2));
}

uint32_t gc_v11_0_3_get_page_table_start_addr_lo32(struct amdgv_adapter *adapt)
{
    return SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT1_PAGE_TABLE_START_ADDR_LO32);
}

uint32_t gc_v11_0_3_get_page_table_start_addr_hi32(struct amdgv_adapter *adapt)
{
    return SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT1_PAGE_TABLE_START_ADDR_HI32);
}

uint32_t gc_v11_0_3_get_page_table_end_addr_lo32(struct amdgv_adapter *adapt)
{
    return SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT1_PAGE_TABLE_END_ADDR_LO32);
}

uint32_t gc_v11_0_3_get_page_table_end_addr_hi32(struct amdgv_adapter *adapt)
{
    return SOC15_REG_OFFSET(GC, 0, regGCVM_CONTEXT1_PAGE_TABLE_END_ADDR_HI32);
}

int gc_v11_0_3_wait_grbm_clean(struct amdgv_adapter *adapt)
{
    int ret;
    ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS2),
		GRBM_STATUS2__EA_BUSY_MASK | GRBM_STATUS2__EA_LINK_BUSY_MASK | GRBM_STATUS2__RLC_BUSY_MASK,
		0, AMDGV_TIMEOUT(TIMEOUT_GRBM_STATUS), AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_YIELD);

    return ret;
}

void gc_v11_0_3_toggle_rlcg_vf_interface(struct amdgv_adapter *adapt, bool enable)
{
    WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_GENERAL_14), enable ? 1 : 0);
}

