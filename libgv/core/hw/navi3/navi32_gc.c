/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv.h>

#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include "navi32_gc.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

/* RLCG indirect register access interface fields. */
#define GC_V11_0_3_RLCG_GC_READ        (0x1u << 28)
#define GC_V11_0_3_RLCG_ADDRESS_MASK   0xFFFFF
#define GC_V11_0_3_RLCG_ERROR_MASK     0xF000000

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
    ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET_NAME(GC, 0, regGRBM_STATUS2),
		GRBM_STATUS2__EA_BUSY_MASK | GRBM_STATUS2__EA_LINK_BUSY_MASK | GRBM_STATUS2__RLC_BUSY_MASK,
		0, AMDGV_TIMEOUT(TIMEOUT_GRBM_STATUS), AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_YIELD);

    return ret;
}

void gc_v11_0_3_toggle_rlcg_vf_interface(struct amdgv_adapter *adapt, bool enable)
{
    WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_GENERAL_14), enable ? 1 : 0);
}

uint32_t gc_v11_0_3_rlcg_rreg(struct amdgv_adapter *adapt, uint32_t off_dw)
{
	uint32_t s0 = SOC15_REG_OFFSET(GC, 0, regSCRATCH_REG0);
	uint32_t s1 = SOC15_REG_OFFSET(GC, 0, regSCRATCH_REG1);
	uint32_t si = SOC15_REG_OFFSET(GC, 0, regRLC_SPARE_INT_0);
	uint32_t err;
	int ret;

	WREG32(s0, 0);
	WREG32(s1, off_dw | GC_V11_0_3_RLCG_GC_READ);
	WREG32(si, 0x1);

	ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET_NAME(GC, 0, regSCRATCH_REG1),
								  GC_V11_0_3_RLCG_ADDRESS_MASK, 0,
								  AMDGV_TIMEOUT(TIMEOUT_GRBM_STATUS),
								  AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_YIELD);

	if (ret) {
		AMDGV_WARN("RLCG read off_dw=0x%05x timed out, RLC did not respond\n", off_dw);
	} else {
		err = RREG32(s1) & GC_V11_0_3_RLCG_ERROR_MASK;
		if (err)
			AMDGV_WARN("RLCG read off_dw=0x%05x error, status=0x%08x\n", off_dw, err);
	}

	return RREG32(s0);
}

