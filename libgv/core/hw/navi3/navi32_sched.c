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
#include <amdgv_sched.h>
#include <amdgv_sched_internal.h>
#include <amdgv_vfmgr.h>

#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include <navi3/MMHUB/mmhub_3_0_0_offset.h>
#include <navi3/MMHUB/mmhub_3_0_0_sh_mask.h>
#include "navi32_gpuiov.h"
#include "navi32_psp.h"
#include "navi32_gfx.h"
#include "navi32_sched.h"
#include "navi32_mmsch.h"

/* navi3 has different default time slice value */
/* 1,2,3,4,6 supports 60fps CG, >6vf supprots 30fps VDI */
#define NAVI32_GFX_TIME_SLICE                       (4000)
#define NAVI32_GFX_TIME_SLICE_2VF                   (6000)
#define NAVI32_GFX_TIME_SLICE_3VF                   (5500)
#define NAVI32_GFX_TIME_SLICE_4VF                   (4000)
#define NAVI32_GFX_TIME_SLICE_6VF                   (2500)
#define NAVI32_GFX_TIME_SLICE_LARGER_6VF            (2750)

static const uint32_t this_block = AMDGV_SCHEDULER_BLOCK;

enum navi32_sched_partition_mode {
	NAVI32_SCHED_PARTITION_MODE_MULTI_VF,
	NAVI32_SCHED_PARTITION_MODE_NUM
};

static struct amdgv_sched_spatial_part navi32_sched_config_tbl[NAVI32_SCHED_PARTITION_MODE_NUM][AMDGV_SCHED_SPATIAL_PARTITION_MAX] = {
	{
		{
			.idx_vf_mask = NAVI3_MMSCH_VCN_BLOCK_ALLOWED_VF_ASSIGNMENT,
			.hw_sched_mask = ((1 << NAVI32_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) |
								(1 << NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV) |
								(1 << NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH))
		},
		{
			.idx_vf_mask = NAVI3_MMSCH_VCN1_BLOCK_ALLOWED_VF_ASSIGNMENT,
			.hw_sched_mask = ((1 << NAVI32_HW_SCHED_BLOCK_VCN1_SCH1_MMSCH) |
								(1 << NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV) |
								(1 << NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH))
		},
		/***********************************
		 * Absolutely No PF on MM Blocks ! *
		 ***********************************/
		{
			.idx_vf_mask = (1 << AMDGV_PF_IDX),
			.hw_sched_mask = (1 << NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV)
		},
	},
};

static void navi32_sched_dump_gpu_state(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	/* RLCV */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_CFG_REG1));
	AMDGV_INFO("RLC_GPU_IOV_CFG_REG1=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_CFG_REG2));
	AMDGV_INFO("RLC_GPU_IOV_CFG_REG2=0x%08x\n", tmp);

	/* GRBM */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS));
	AMDGV_INFO("GRBM_STATUS=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS2));
	AMDGV_INFO("GRBM_STATUS2=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS_SE0));
	AMDGV_INFO("GRBM_STATUS_SE0=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS_SE1));
	AMDGV_INFO("GRBM_STATUS_SE1=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_STATUS_SE2));
	AMDGV_INFO("GRBM_STATUS_SE2=0x%08x\n", tmp);

	/* CP Overall */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_CNTL));
	AMDGV_INFO("CP_MES_CNTL=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_CNTL));
	AMDGV_INFO("CP_MEC_RS64_CNTL=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL));
	AMDGV_INFO("CP_ME_CNTL=0x%08x\n", tmp);

	/* CPC */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_STATUS));
	AMDGV_INFO("CP_CPC_STATUS=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_BUSY_STAT));
	AMDGV_INFO("CP_CPC_BUSY_STAT=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_BUSY_STAT2));
	AMDGV_INFO("CP_CPC_BUSY_STAT2=0x%08x\n", tmp);

	/* CPF */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPF_STATUS));
	AMDGV_INFO("CP_CPF_STATUS=0x%08x\n", tmp);

	/* Page Fault */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regGCVM_L2_PROTECTION_FAULT_STATUS));
	AMDGV_INFO("GCVM_L2_PROTECTION_FAULT_STATUS=0x%08x\n", tmp);
	tmp = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_STATUS));
	AMDGV_INFO("MMVM_L2_PROTECTION_FAULT_STATUS=0x%08x\n", tmp);
}


static uint32_t navi32_cp_sched_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t ret = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_CP_SCHEDULERS));
	return ret;
}

static uint32_t navi32_sched_get_asic_time_slice_gfx(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	/* default value */
	uint32_t ret = DEFAULT_GFX_TIME_SLICE_1VF_SELF_SW;
	if (num_vf > 1 || is_active_vf(AMDGV_PF_IDX)) {
		if (num_vf <= 2) {
			ret = NAVI32_GFX_TIME_SLICE_2VF;
		} else if (num_vf <= 3) {
			ret = NAVI32_GFX_TIME_SLICE_3VF;
		} else if (num_vf <= 4) {
			ret = NAVI32_GFX_TIME_SLICE_4VF;
		} else if (num_vf <= 6) {
			ret = NAVI32_GFX_TIME_SLICE_6VF;
		} else {
			ret = NAVI32_GFX_TIME_SLICE_LARGER_6VF;
		}
	} else if (num_vf == 1) {
		if (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH) {
			ret = DEFAULT_GFX_TIME_SLICE_1VF;
		} else {
			ret = DEFAULT_GFX_TIME_SLICE_1VF_SELF_SW;
		}
	}
	return ret;
}

static uint32_t navi32_sched_get_asic_time_slice_mm(struct amdgv_adapter *adapt)
{
	return DEFAULT_MM_TIME_SLICE;
}

static uint32_t navi32_sched_get_asic_time_slice(struct amdgv_adapter *adapt, enum amdgv_sched_block sched_block, uint32_t num_vf)
{
	if (sched_block == AMDGV_SCHED_BLOCK_GFX)
		return navi32_sched_get_asic_time_slice_gfx(adapt, num_vf);
	else
		return navi32_sched_get_asic_time_slice_mm(adapt);
}

static int navi32_sched_copy_static_spatial_part_table(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	struct amdgv_sched_spatial_part *table;
	uint32_t i;

	oss_memset(adapt->sched.spatial_part, 0, sizeof(struct amdgv_sched_spatial_part));

	adapt->sched.num_vf_per_gfx_sched = num_vf;

	table = navi32_sched_config_tbl[NAVI32_SCHED_PARTITION_MODE_MULTI_VF];
	adapt->sched.num_spatial_partitions = 3;

	if (adapt->sched.num_spatial_partitions > AMDGV_SCHED_SPATIAL_PARTITION_MAX) {
		AMDGV_ERROR("Cannot support %d partition count", adapt->sched.num_spatial_partitions);
		return AMDGV_FAILURE;
	}

	for (i = 0; i < adapt->sched.num_spatial_partitions; i++) {
		adapt->sched.spatial_part[i].hw_sched_mask = table[i].hw_sched_mask;
		/* Remove all invalid VF bits */
		adapt->sched.spatial_part[i].idx_vf_mask =
			(table[i].idx_vf_mask & (((1 << num_vf) - 1) | (1 << AMDGV_PF_IDX)));
	}

	return 0;
}

static int navi32_sched_reconfig_mapping_tables(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	if (navi32_sched_copy_static_spatial_part_table(adapt, num_vf))
		return AMDGV_FAILURE;

	if (amdgv_sched_world_switch_remap_vf_assignment(adapt))
		return AMDGV_FAILURE;

	if (amdgv_sched_part_mapping_init(adapt))
		return AMDGV_FAILURE;

	return 0;
}

static int navi32_setup_vf_timeslice(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t timeslice, enum amdgv_sched_block sched_block)
{
	int ret = 0;

	if (sched_block != AMDGV_SCHED_BLOCK_GFX)
		return 0;

	adapt->array_vf[idx_vf].time_slice[sched_block] = timeslice;
	adapt->gpuiov.sched_cfg.auto_config.vf_time_quanta[idx_vf] = timeslice;

	return ret;
}

static int navi32_setup_default_vfs_timeslice(struct amdgv_adapter *adapt)
{
	uint32_t i;
	uint32_t gfx_tmp;
	int ret = 0;
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	gfx_tmp = navi32_sched_get_asic_time_slice_gfx(adapt, adapt->sched.num_vf_per_gfx_sched);
	for (i = 0; i < adapt->num_vf; i++)
		ret = amdgv_sched_setup_vf_timeslice(adapt, i, gfx_tmp, AMDGV_SCHED_BLOCK_GFX);

	if ((adapt->flags & AMDGV_FLAG_USE_PF))
		ret = amdgv_sched_setup_vf_timeslice(adapt, AMDGV_PF_IDX, gfx_tmp, AMDGV_SCHED_BLOCK_GFX);

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->sched_mode > AMDGV_SCHED_MAX_HW_SCHED_MODE)
			continue;
		ret = amdgv_sched_world_switch_config_auto_sched_mode(adapt, world_switch);
		if (ret)
			return ret;
	}

	return ret;
}

static int navi32_asymmetric_fb_block_setup(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t fb_size,
				struct amdgv_vf_fb_block *fb_block)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];

	if (fb_block == NULL) {
		fb_block = amdgv_vfmgr_find_usable_free_block(adapt, fb_size);
		if (!fb_block) {
			/* This should never happen as we have checked fb assignable before.
			 * For robustness, handle this case by freeing VF on hw side to avoid misalignment.
			 */
			amdgv_vfmgr_free_vf(adapt, idx_vf);
			return AMDGV_FAILURE;
		}
		fb_block->allocated = true;
		fb_block->idx_vf = idx_vf;
		amdgv_vfmgr_divide_fb_block(adapt, fb_block, fb_size);
	}

	/* If share_tmr, we never use entry->fb_offset;
	 * If no ffbm WA, fb_block->fb_offset_tlb equals fb_offset.
	 */
	entry->fb_offset = fb_block->fb_offset_tlb;
	entry->fb_offset_tmr = fb_block->fb_offset_tlb;
	entry->fb_size = fb_size;
	entry->fb_size_tmr = fb_size + adapt->tmr_size;

	return 0;
}

static int navi32_asymmetric_fb_reconfig(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t fb_size)
{
	struct amdgv_vf_fb_block *fb_block;
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	uint64_t fb_size_tlb;

	fb_size_tlb = amdgv_vfmgr_calculate_fb_size_tlb(adapt, fb_size);

	if (!amdgv_vfmgr_check_fb_assignable(adapt, idx_vf, fb_size)) {
		AMDGV_ERROR("Cannot find qualified free block for VF %d, please free up some FB size or perform defragment\n", idx_vf);
		return AMDGV_FAILURE;
	}

	fb_block = amdgv_vfmgr_find_fb_block_by_fcn(adapt, idx_vf);
	if (!fb_block) {
		/* Previously freed. Directly goto setup. */
		if (is_unavail_vf(idx_vf)) {
			set_to_avail_vf(idx_vf);
			entry->configured = true;
		}
	} else {
		if (fb_size <= fb_block->fb_size) {
			/* To reduce existing vf_fb, just divide the node and update vf device */
			amdgv_vfmgr_divide_fb_block(adapt, fb_block, fb_size);
		} else {
			/* To enlarge existing vf_fb, free current layout and realloc suitable block for vf */
			amdgv_vfmgr_free_fb_block(adapt, fb_block);
			fb_block = NULL;
		}
	}

	return navi32_asymmetric_fb_block_setup(adapt, idx_vf, fb_size, fb_block);
}

static int navi32_sched_sw_init_early(struct amdgv_adapter *adapt)
{
	adapt->sched.dump_gpu_state = navi32_sched_dump_gpu_state;
	adapt->sched.get_asic_time_slice = navi32_sched_get_asic_time_slice;
	adapt->sched.reconfig_mapping_tables = navi32_sched_reconfig_mapping_tables;
	adapt->sched.cp_sched_state = navi32_cp_sched_state;
	adapt->sched.setup_vf_timeslice = navi32_setup_vf_timeslice;
	adapt->sched.setup_default_vfs_timeslice = navi32_setup_default_vfs_timeslice;
	adapt->sched.asymmetric_fb_reconfig = navi32_asymmetric_fb_reconfig;

	/* enable clearing vf fb region (by default) */
	if (!(adapt->flags & AMDGV_FLAG_USE_PF))
		adapt->flags |= AMDGV_FLAG_ENABLE_CLEAR_VF_FB;

	/* enable bar protection scheme */
	adapt->flags |= AMDGV_FLAG_VF_FB_PROTECTION;

	if (navi32_sched_copy_static_spatial_part_table(adapt, adapt->num_vf))
		return AMDGV_FAILURE;

	if (amdgv_sched_init(adapt))
		return AMDGV_FAILURE;

	if (adapt->opt.allow_time_full_access == 0 && adapt->sched.num_vf_per_gfx_sched > 1) {
		adapt->sched.allow_time_full_access = 3000 * 1000;
		if (adapt->sched.num_vf_per_gfx_sched >= 6) {
			adapt->sched.allow_time_full_access = 4500 * 1000;
		}
		AMDGV_INFO("allowed time for full access has been changed to %dms for NAVI32 under multiple VF\n",
					adapt->sched.allow_time_full_access / 1000);
	}

	return 0;
}

static int navi32_sched_sw_fini_early(struct amdgv_adapter *adapt)
{
	amdgv_sched_fini(adapt);

	return 0;
}

static int navi32_sched_hw_init_early(struct amdgv_adapter *adapt)
{
	int ret;

	ret = amdgv_sched_init_pf_state_early(adapt);

	return ret;
}

static int navi32_sched_hw_fini_early(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_sched_hw_init_late(struct amdgv_adapter *adapt)
{
	int ret;
	ret = amdgv_sched_init_pf_state_late(adapt);
	if (ret)
		return ret;

	return navi32_setup_default_vfs_timeslice(adapt);;
}

static int navi32_sched_sw_init_late(struct amdgv_adapter *adapt) { return 0; }
static int navi32_sched_sw_fini_late(struct amdgv_adapter *adapt) { return 0; }
static int navi32_sched_hw_fini_late(struct amdgv_adapter *adapt) { return 0; }

struct amdgv_init_func navi32_sched_early_func = {
	.name = "navi32_sched_early_func",
	.sw_init = navi32_sched_sw_init_early,
	.sw_fini = navi32_sched_sw_fini_early,
	.hw_init = navi32_sched_hw_init_early,
	.hw_fini = navi32_sched_hw_fini_early,
};

struct amdgv_init_func navi32_sched_late_func = {
	.name = "navi32_sched_late_func",
	.sw_init = navi32_sched_sw_init_late,
	.sw_fini = navi32_sched_sw_fini_late,
	.hw_init = navi32_sched_hw_init_late,
	.hw_fini = navi32_sched_hw_fini_late,
};
