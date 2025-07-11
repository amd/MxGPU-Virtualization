/*
 * Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <amdgv_device.h>
#include <amdgv_sched_internal.h>
#include "navi32_ffbm.h"
#include <amdgv_ffbm.h>

#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include <navi3/MMHUB/mmhub_3_0_0_offset.h>
#include <navi3/MMHUB/mmhub_3_0_0_sh_mask.h>

#include "navi3/NBIO/nbio_4_3_0_offset.h"
#include "navi3/NBIO/nbio_4_3_0_sh_mask.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

#define TO_2M_ALIGN(val) ((uint32_t)(val >> 21))
#define HAS_READ_PERMISSION(val) ((val & AMDGV_FFBM_PERM_READ) ? 1 : 0)
#define HAS_WRITE_PERMISSION(val) ((val & AMDGV_FFBM_PERM_WRITE) ? 1 : 0)

static int navi32_ffbm_request_access(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t check_mask, val, world_switch_id;
	struct amdgv_sched_world_switch *world_switch;
	int wait_ret = 0;
	if (enable) {
		check_mask = AMDGV_WAIT_CHECK_EQ;
		val = 1;
	} else {
		check_mask = AMDGV_WAIT_CHECK_NE;
		val = 0;
	}

	FFBM_SET_REG(FFBM_ACCESS_CNTL, val);
	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, 0, regGCUTCL2_FFBM_ACCESS_CNTL), GCUTCL2_FFBM_ACCESS_CNTL__TLB_ACCESS_GRANT_MASK,
		GCUTCL2_FFBM_ACCESS_CNTL__TLB_ACCESS_GRANT_MASK, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), check_mask, AMDGV_WAIT_FLAG_FORCE_DELAY);
	if (!wait_ret)
		wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(MMHUB, 0, regMMUTCL2_FFBM_ACCESS_CNTL), MMUTCL2_FFBM_ACCESS_CNTL__TLB_ACCESS_GRANT_MASK,
			MMUTCL2_FFBM_ACCESS_CNTL__TLB_ACCESS_GRANT_MASK, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), check_mask, AMDGV_WAIT_FLAG_FORCE_DELAY);

	if (wait_ret) {
		AMDGV_ERROR("FFBM: Failed to %s access\n", enable ? "enable" : "disable");
		goto failed;
	}

	/* To lock common GFX world switch when configuring FFBM */
	for (world_switch_id = 0; world_switch_id < adapt->sched.num_world_switch;
		 world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX && world_switch->enabled
			&& world_switch->manual.switching_lock) {
			if (enable) {
				oss_mutex_lock(world_switch->manual.switching_lock);
			} else {
				oss_mutex_unlock(world_switch->manual.switching_lock);
			}
		}
	}

failed:
	return wait_ret;
}

static int navi32_ffbm_clear_all(struct amdgv_adapter *adapt)
{
	uint64_t page_size = AMDGV_FFBM_PAGE_SIZE(adapt->ffbm.default_fragment);
	uint64_t current_spa;
	uint32_t total_fb;

	if (navi32_ffbm_request_access(adapt, true))
		return AMDGV_FAILURE;

	FFBM_SET_REG(FFBM_ADDRESS, 0);

	amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_fb);

	for (current_spa = 0;
			current_spa < MBYTES_TO_BYTES(total_fb);
			current_spa += page_size) {

		/* do dummy read before read register regGCUTCL2_FFBM_DATA */
		RREG32(SOC15_REG_OFFSET(GC, 0, regGCUTCL2_FFBM_DATA));
		FFBM_SET_REG(FFBM_DATA, 0);
	}

	navi32_ffbm_request_access(adapt, false);

	return 0;
}

static int navi32_ffbm_apply_pteb(struct amdgv_adapter *adapt, struct amdgv_ffbm_pte_block *pteb, bool valid)
{
	uint64_t page_size = AMDGV_FFBM_PAGE_SIZE(adapt->ffbm.default_fragment);
	uint64_t current_gpa = pteb->gpa;
	uint64_t current_spa = pteb->spa;
	uint32_t gpa;
	uint32_t ffbm_data;

	AMDGV_DEBUG("FFBM apply pteb: gpa: 0x%llx spa: 0x%llx, size: 0x%llx, type: %d, applied: %d, permission: %d",
				current_gpa,
				current_spa,
				pteb->size,
				pteb->type,
				pteb->applied,
				pteb->permission);

	AMDGV_ASSERT(pteb->type == AMDGV_FFBM_MEM_TYPE_VF || pteb->type == AMDGV_FFBM_MEM_TYPE_TMR);

	if (navi32_ffbm_request_access(adapt, true))
		return AMDGV_FAILURE;

	gpa = 0;
	gpa = REG_SET_FIELD(gpa, GCUTCL2_FFBM_ADDRESS, VFID, pteb->vf_idx);
	gpa = REG_SET_FIELD(gpa, GCUTCL2_FFBM_ADDRESS, ADDRESS, TO_2M_ALIGN(current_gpa));
	FFBM_SET_REG(FFBM_ADDRESS, gpa);

	for (current_spa = pteb->spa;
			current_spa < pteb->spa + pteb->size;
			current_spa += page_size) {

		/* do dummy read before read register regGCUTCL2_FFBM_DATA */
		RREG32(SOC15_REG_OFFSET(GC, 0, regGCUTCL2_FFBM_DATA));
		ffbm_data = 0;
		ffbm_data = REG_SET_FIELD(ffbm_data, GCUTCL2_FFBM_DATA, VALID, valid ? 1 : 0);
		ffbm_data = REG_SET_FIELD(ffbm_data, GCUTCL2_FFBM_DATA, READ_PERMISSION, (pteb->permission & AMDGV_FFBM_PERM_READ) ? 1 : 0);
		ffbm_data = REG_SET_FIELD(ffbm_data, GCUTCL2_FFBM_DATA, WRITE_PERMISSION, (pteb->permission & AMDGV_FFBM_PERM_WRITE) ? 1 : 0);
		ffbm_data = REG_SET_FIELD(ffbm_data, GCUTCL2_FFBM_DATA, FRAGMENT, pteb->fragment);
		ffbm_data = REG_SET_FIELD(ffbm_data, GCUTCL2_FFBM_DATA, FB_SPA, TO_2M_ALIGN(current_spa));

		FFBM_SET_REG(FFBM_DATA, ffbm_data);
	}

	navi32_ffbm_request_access(adapt, false);

	pteb->applied = valid;

	return 0;
}

static int navi32_ffbm_invalidate_tlb(struct amdgv_adapter *adapt, struct amdgv_ffbm_pte_block *pteb, bool flush)
{
	uint64_t page_size = AMDGV_FFBM_PAGE_SIZE(pteb->fragment);      /* invalidate use real fragment */
	uint64_t current_gpa = pteb->gpa;
	uint32_t data;
	int wait_ret = 0;
	AMDGV_ASSERT(pteb->type == AMDGV_FFBM_MEM_TYPE_VF);

	if (navi32_ffbm_request_access(adapt, true))
		return AMDGV_FAILURE;

	amdgv_misc_hdp_flush(adapt);

	for (current_gpa = pteb->gpa;
			current_gpa < pteb->gpa + pteb->size;
			current_gpa += page_size) {

		data = 0;
		data = REG_SET_FIELD(data, GCUTCL2_FFBM_INVALIDATE_REQUEST, REQ, 1);
		data = REG_SET_FIELD(data, GCUTCL2_FFBM_INVALIDATE_REQUEST, VFID, pteb->vf_idx);
		data = REG_SET_FIELD(data, GCUTCL2_FFBM_INVALIDATE_REQUEST, FLUSHTYPE, flush ? 1 : 0);
		data = REG_SET_FIELD(data, GCUTCL2_FFBM_INVALIDATE_REQUEST, SIZE, (pteb->fragment == 9) ? 0 : 1);
		data = REG_SET_FIELD(data, GCUTCL2_FFBM_INVALIDATE_REQUEST, ADDRESS, TO_2M_ALIGN(current_gpa));

		FFBM_SET_REG(FFBM_INVALIDATE_REQUEST, data);

		/* do dummy read regGCUTCL2_FFBM_INVALIDATE_REQUEST before read register regGCUTCL2_FFBM_INVALIDATE_RESPONSE */
		RREG32(SOC15_REG_OFFSET(GC, 0, regGCUTCL2_FFBM_INVALIDATE_REQUEST));

		if (flush) {
			wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, 0, regGCUTCL2_FFBM_INVALIDATE_RESPONSE), GCUTCL2_FFBM_INVALIDATE_RESPONSE__FLUSHTYPE_INVALIDATE_ACK_MASK,
						GCUTCL2_FFBM_INVALIDATE_RESPONSE__FLUSHTYPE_INVALIDATE_ACK_MASK, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_DELAY);
			if (!wait_ret)
				wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(MMHUB, 0, regMMUTCL2_FFBM_INVALIDATE_RESPONSE), MMUTCL2_FFBM_INVALIDATE_RESPONSE__FLUSHTYPE_INVALIDATE_ACK_MASK,
							MMUTCL2_FFBM_INVALIDATE_RESPONSE__FLUSHTYPE_INVALIDATE_ACK_MASK, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_DELAY);
		} else {
			wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, 0, regGCUTCL2_FFBM_INVALIDATE_RESPONSE), GCUTCL2_FFBM_INVALIDATE_RESPONSE__NON_FLUSHTYPE_INVALIDATE_ACK_MASK,
						GCUTCL2_FFBM_INVALIDATE_RESPONSE__NON_FLUSHTYPE_INVALIDATE_ACK_MASK, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_DELAY);
			if (!wait_ret)
				wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(MMHUB, 0, regMMUTCL2_FFBM_INVALIDATE_RESPONSE), MMUTCL2_FFBM_INVALIDATE_RESPONSE__NON_FLUSHTYPE_INVALIDATE_ACK_MASK,
							MMUTCL2_FFBM_INVALIDATE_RESPONSE__NON_FLUSHTYPE_INVALIDATE_ACK_MASK, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ, AMDGV_WAIT_FLAG_FORCE_DELAY);
		}

		if (wait_ret) {
			AMDGV_WARN("FFBM timeout waiting for FFBM invalidate response, break invalidate tlb\n");
			break;
		}
	}

	navi32_ffbm_request_access(adapt, false);

	return 0;
}

static int navi32_ffbm_sw_init(struct amdgv_adapter *adapt)
{
	amdgv_ffbm_sw_init(adapt);
	adapt->ffbm.apply_pteb = navi32_ffbm_apply_pteb;
	adapt->ffbm.invalidate_tlb = navi32_ffbm_invalidate_tlb;

	adapt->ffbm.default_fragment = 9;      /* 2M */

	adapt->ffbm.share_tmr = true;

	return 0;
}

static int navi32_ffbm_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_ffbm_sw_fini(adapt);
	return 0;
}

static int navi32_ffbm_hw_init(struct amdgv_adapter *adapt)
{
	/* re-init max reserved block after ecc hw_init, since ffbm is navi32+ feature, this field should have value */
	adapt->ffbm.max_reserved_block = adapt->ecc.bad_page_record_threshold;
	navi32_ffbm_clear_all(adapt);
	amdgv_ffbm_page_table_init(adapt);
	return 0;
}

static int navi32_ffbm_hw_fini(struct amdgv_adapter *adapt)
{
	amdgv_ffbm_page_table_destroy(adapt);
	return 0;
}


struct amdgv_init_func navi32_ffbm_func = {
	.name = "navi32_ffbm_func",
	.sw_init = navi32_ffbm_sw_init,
	.sw_fini = navi32_ffbm_sw_fini,
	.hw_init = navi32_ffbm_hw_init,
	.hw_fini = navi32_ffbm_hw_fini,
};

