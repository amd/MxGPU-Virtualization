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
#include "mi300_gfx.h"
#include <amdgv_sched_internal.h>
#include "mi300/GC/gc_9_4_3_offset.h"
#include "mi300/GC/gc_9_4_3_sh_mask.h"
#include "gfxhub_v1_2.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static void mi300_gfx_select_xcc_se_sh(struct amdgv_adapter *adapt, uint32_t se, uint32_t sh,
				   uint32_t instance, uint32_t xcc_id)
{
	uint32_t data;

	if (instance == ~0)
		data = REG_SET_FIELD(0, GRBM_GFX_INDEX, INSTANCE_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(0, GRBM_GFX_INDEX, INSTANCE_INDEX, instance);

	if (se == ~0)
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SE_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SE_INDEX, se);

	if (sh == ~0)
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SH_BROADCAST_WRITES, 1);
	else
		data = REG_SET_FIELD(data, GRBM_GFX_INDEX, SH_INDEX, sh);

	WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regGRBM_GFX_INDEX), data);
	AMDGV_DEBUG("GRBM_GFX_INDEX=0x%x\n", data);
}

static uint32_t mi300_gfx_create_bitmask(uint32_t bit_width)
{
	return (uint32_t)((1ULL << bit_width) - 1);
}

static uint32_t mi300_gfx_get_xcc_cu_active_bitmap(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	uint32_t data, mask;

	data = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regCC_GC_SHADER_ARRAY_CONFIG));
	data |= RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regGC_USER_SHADER_ARRAY_CONFIG));

	data &= CC_GC_SHADER_ARRAY_CONFIG__INACTIVE_CUS_MASK;
	data >>= CC_GC_SHADER_ARRAY_CONFIG__INACTIVE_CUS__SHIFT;

	mask = mi300_gfx_create_bitmask(adapt->config.gfx.max_cu_per_sh);

	return (~data) & mask;
}

bool mi300_gfx_is_symetric_cu(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	int cu_per_se = 0;
	int pre_cu = 0;
	int i, j, k;
	uint32_t mask, bitmap;
	bool is_symmetric = true;

	for (i = 0; i < adapt->config.gfx.max_shader_engines; i++) {
		cu_per_se = 0;
		for (j = 0; j < adapt->config.gfx.max_sh_per_se; j++) {
			mask = 1;
			mi300_gfx_select_xcc_se_sh(adapt, i, j, ~0, xcc_id);
			bitmap = mi300_gfx_get_xcc_cu_active_bitmap(adapt, xcc_id);
			for (k = 0; k < adapt->config.gfx.max_cu_per_sh; k++) {
				if (bitmap & mask)
					cu_per_se++;
				mask <<= 1;
			}
		}
		if ((i && is_symmetric) && pre_cu != cu_per_se) {
			is_symmetric = false;
			break;
		}
		pre_cu = cu_per_se;
	}

	mi300_gfx_select_xcc_se_sh(adapt, ~0, ~0, ~0, xcc_id);
	return is_symmetric;
}

uint32_t mi300_gfx_get_xcc_cu_count(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	uint32_t cu_count = 0;
	int i, j, k;
	uint32_t mask, bitmap;

	for (i = 0; i < adapt->config.gfx.max_shader_engines; i++) {
		for (j = 0; j < adapt->config.gfx.max_sh_per_se; j++) {
			mask = 1;
			mi300_gfx_select_xcc_se_sh(adapt, i, j, ~0, xcc_id);
			bitmap = mi300_gfx_get_xcc_cu_active_bitmap(adapt, xcc_id);

			for (k = 0; k < adapt->config.gfx.max_cu_per_sh; k++) {
				if (bitmap & mask)
					cu_count++;
				mask <<= 1;
			}
		}
	}

	mi300_gfx_select_xcc_se_sh(adapt, ~0, ~0, ~0, xcc_id);
	return cu_count;
}

int mi300_gfx_enable_rlc(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t rlc_cntl;
	uint32_t xcc_id = 0;

	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regRLC_GPM_THREAD_ENABLE), 0x3);

		/* Re-enable RLC */
		rlc_cntl = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regRLC_CNTL));
		rlc_cntl = REG_SET_FIELD(rlc_cntl, RLC_CNTL, RLC_ENABLE_F32, 1);
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regRLC_CNTL), rlc_cntl);

		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regRLC_SRM_CNTL), 3);

		if (mi300_gfx_wait_rlc_idle(adapt, GET_INST(GC, xcc_id))) {
			AMDGV_ERROR("RLC Enable failed\n");
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

// TODO Need to save multiple?
void mi300_gfx_save_tcp_addr_config(struct amdgv_adapter *adapt,
				    struct mi300_vf_flr_state *vf_state)
{
	uint32_t xcc_id = 0;

	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, vf_state->idx_vf)) {
		vf_state->tcp_addr_config = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regTCP_ADDR_CONFIG);
	}
}

void mi300_gfx_restore_tcp_addr_config(struct amdgv_adapter *adapt,
				       struct mi300_vf_flr_state *vf_state)
{
	uint32_t xcc_id = 0;

	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, vf_state->idx_vf)) {
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regTCP_ADDR_CONFIG, vf_state->tcp_addr_config);
	}
}

int mi300_gfx_wait_for_grbm(struct amdgv_adapter *adapt, uint16_t idx_vf)
{
	uint32_t grbm_status = 0;
	uint32_t grbm_status2 = 0;
	int wait_ret;
	uint32_t xcc_id = 0;

	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
		/* wait for a clean state */
		wait_ret = amdgv_wait_for_register(
			adapt, SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regGRBM_STATUS2),
			GRBM_STATUS2__EA_BUSY_MASK | GRBM_STATUS2__EA_LINK_BUSY_MASK |
				GRBM_STATUS2__RLC_BUSY_MASK,
			0, AMDGV_TIMEOUT(TIMEOUT_GRBM_STATUS), AMDGV_WAIT_CHECK_EQ,
			AMDGV_WAIT_FLAG_FORCE_YIELD);

		grbm_status = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regGRBM_STATUS));
		grbm_status2 = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regGRBM_STATUS2));
		AMDGV_DEBUG("GRBM_STATUS = 0x%x GRBM_STATUS2 = 0x%x\n", grbm_status,
			    grbm_status2);

		if (wait_ret)
			return AMDGV_FAILURE;
	}

	return 0;
}

int mi300_gfx_init_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int entry;
	uint32_t vm_l2_protection_fault_cntl;
	uint32_t *tab_reg;
	struct amdgv_vf_device *vf;
	uint32_t xcc_id = 0;

	vf = &adapt->array_vf[idx_vf];

	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
		/* stop gfx hub error state */
		vm_l2_protection_fault_cntl =
			RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regVM_L2_PROTECTION_FAULT_CNTL));

		/* clear bit 2 3 4 5 6 7 8 9 10 11 12 */
		vm_l2_protection_fault_cntl &= ~0x1ffb;

		/* set bit 30 31 */
		vm_l2_protection_fault_cntl |= 0x3 << 30;

		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regVM_L2_PROTECTION_FAULT_CNTL),
		       vm_l2_protection_fault_cntl);

		/* disable user context vm by setting START ADDR bigger
		 * than END ADDR to make all vm address invalid
		 */
		if (vf->res_mapped && vf->res.mmio) {
			for (entry = 0; entry < 15; ++entry) {
				tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
					  SOC15_REG_OFFSET(
						  GC, GET_INST(GC, xcc_id),
						  regVM_CONTEXT1_PAGE_TABLE_START_ADDR_LO32);

				oss_mm_write32(tab_reg, ~0);

				tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
					  SOC15_REG_OFFSET(
						  GC, GET_INST(GC, xcc_id),
						  regVM_CONTEXT1_PAGE_TABLE_START_ADDR_HI32);
				oss_mm_write32(tab_reg, ~0);

				tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
					  SOC15_REG_OFFSET(
						  GC, GET_INST(GC, xcc_id),
						  regVM_CONTEXT1_PAGE_TABLE_END_ADDR_LO32);
				oss_mm_write32(tab_reg, ~0);

				tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
					  SOC15_REG_OFFSET(
						  GC, GET_INST(GC, xcc_id),
						  regVM_CONTEXT1_PAGE_TABLE_END_ADDR_HI32);
				oss_mm_write32(tab_reg, ~0);
			}
		}
	}

	if (mi300_gfx_wait_for_grbm(adapt, idx_vf))
		AMDGV_WARN("GRBM_STATUS2 is not clean for FLR\n");

	return 0;
}

int mi300_gfx_wait_rlc_idle(struct amdgv_adapter *adapt, uint16_t phys_xcc_id)
{
	int wait_ret;

	wait_ret =
		amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, phys_xcc_id, regRLC_STAT),
					0, 0, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG),
					AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_YIELD);

	if (wait_ret) {
		AMDGV_ERROR("Wait for RLC idle failed\n");
		return AMDGV_FAILURE;
	} else {
		return 0;
	}
}

void mi300_gfx_rlc_smu_handshake_cntl(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t rlc_pg_cntl;
	uint32_t i = 0;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		rlc_pg_cntl = RREG32_SOC15(GC, GET_INST(GC, i), regRLC_PG_CNTL);

		/* RLC_PG_CNTL[23] = 0 (default)
		 * RLC will wait for handshake acks with SMU
		 * GFXOFF will be enabled
		 * RLC_PG_CNTL[23] = 1
		 * RLC will not issue any message to SMU
		 * hence no handshake between SMU & RLC
		 * GFXOFF will be disabled
		 */
		if (!enable)
			rlc_pg_cntl |= 0x800000;
		else
			rlc_pg_cntl &= ~0x800000;

		WREG32_SOC15(GC, GET_INST(GC, i), regRLC_PG_CNTL, rlc_pg_cntl);
	}
}
