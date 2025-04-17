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

#include "amdgv_device.h"
#include "amdgv_gpuiov.h"
#include "amdgv_sched.h"
#include "amdgv_sched_internal.h"
#include "amdgv_guard.h"
#include "amdgv_sriovmsg.h"
#include "amdgv_vfmgr.h"
#include "amdgv_misc.h"
#include "amdgv_mmsch.h"
#include "amdgv_psp_gfx_if.h"

#include "ai.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

/* default mm engine FPS */
#define DEFAULT_MM_ENGINE_FPS 30

/* default mm engine bandwidth is 1920x1080p@30fps */
#define DEFAULT_MM_ENGINE_BANDWIDTH                                                           \
	(((1920 + 15) / 16) * ((1080 + 15) / 16) * DEFAULT_MM_ENGINE_FPS)

#define GPU_CAPACITY_MAX 65535

static void amdgv_vfmgr_parse_mm_bandwidth(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_vf_device *entry;

	entry = &adapt->array_vf[idx_vf];

	entry->time_slice[AMDGV_SCHED_BLOCK_UVD] = amdgv_sched_bandwidth_to_time_slice(
		adapt, AMDGV_HEVC_ENGINE, entry->mm_bandwidth[AMDGV_HEVC_ENGINE]);
	entry->time_slice[AMDGV_SCHED_BLOCK_VCE] = amdgv_sched_bandwidth_to_time_slice(
		adapt, AMDGV_VCE_ENGINE, entry->mm_bandwidth[AMDGV_VCE_ENGINE]);
	entry->time_slice[AMDGV_SCHED_BLOCK_UVD1] = amdgv_sched_bandwidth_to_time_slice(
		adapt, AMDGV_HEVC1_ENGINE, entry->mm_bandwidth[AMDGV_HEVC1_ENGINE]);
}

int amdgv_vfmgr_configured_vf_num(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf, config_vf_num = 0;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++)
		if (adapt->array_vf[idx_vf].configured)
			config_vf_num++;
	if (adapt->array_vf[AMDGV_PF_IDX].configured)
		config_vf_num++;

	return config_vf_num;
}

int amdgv_vfmgr_copy_ip_data_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   bool mbox_resp)
{
	int ret;
	uint32_t msg[MAILBOX_DATA_LEN_1];

	if (!adapt->ip_discovery.copy_to_vf)
		return AMDGV_FAILURE;

	ret = adapt->ip_discovery.copy_to_vf(adapt, idx_vf);
	if (ret)
		return AMDGV_FAILURE;

	if (mbox_resp) {
		msg[0] = AMDGV_EVENT_IP_DATA_READY;
		amdgv_mailbox_send_msg(adapt, idx_vf, msg, MAILBOX_DATA_LEN_1, true);
	}
	return 0;
}

int amdgv_vfmgr_init_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, bool mbox_resp,
			   uint8_t pattern_data, uint8_t flag)
{
	int tmp_ret, ret = 0;
	uint8_t pattern = pattern_data;

	/*
	 * Program MC_VM_FB_LOCATION_* registers and HDP for the VF
	 * using PSP interface so that VF can access the end of FB
	 * as soon as it boots up.
	 */
	if (!flag && adapt->psp.psp_program_guest_mc_settings) {
		/* program vf mc settings */
		AMDGV_DEBUG("program %s mc settings\n", amdgv_idx_to_str(idx_vf));
		tmp_ret = adapt->psp.psp_program_guest_mc_settings(adapt, idx_vf);
		if (tmp_ret)
			ret = tmp_ret;
	}

	/* Copy VF's GPUVM settings to master AID */
	amdgv_psp_copy_vf_chiplet_regs(adapt, idx_vf);

	/* FB content is already cleared during whole GPU reset (BACO/MODE1). */
	/* WA: There is an issue with MI300/MI325/MI308 mode 1 reset, need to clear vf fb*/
	if ((!adapt->reset.reset_state) && ((adapt->asic_type != CHIP_MI300X) && (adapt->asic_type == CHIP_MI308X))) {
		/* clear VF FB at VM allocation before copy_ip_data */
		tmp_ret = amdgv_misc_clear_vf_fb(adapt, idx_vf, pattern);
		if (tmp_ret)
			ret = tmp_ret;
	}
	return ret;
}

void amdgv_vfmgr_set_pf_fb(struct amdgv_adapter *adapt)
{
	uint32_t total_avail_fb, pf_fb_size;

	pf_fb_size = adapt->array_vf[AMDGV_PF_IDX].fb_size;

	amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_avail_fb);

	amdgv_gpuiov_set_total_fb_consumed(adapt, (total_avail_fb - pf_fb_size));

	amdgv_ffbm_page_table_update_by_fcn(adapt, AMDGV_PF_IDX);

	AMDGV_INFO("PF FB size set to %d MB\n", pf_fb_size);
}

void amdgv_vfmgr_import_pf_fb(struct amdgv_adapter *adapt)
{
	uint32_t total_avail_fb;
	uint16_t total_fb_consumed;

	amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_avail_fb);

	amdgv_gpuiov_get_total_fb_consumed(adapt, &total_fb_consumed);

	adapt->array_vf[AMDGV_PF_IDX].fb_size = total_avail_fb - total_fb_consumed;
}

static void amdgv_vfmgr_set_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];

	if (!entry->configured)
		return;

	if (idx_vf != AMDGV_PF_IDX)
		amdgv_ffbm_unmap_by_fcn(adapt, idx_vf, false);

	if (adapt->ffbm.enabled && adapt->ffbm.share_tmr)
		amdgv_gpuiov_set_vf_fb(adapt, idx_vf, entry->fb_offset_tmr,
				entry->fb_size_tmr);
	else
		amdgv_gpuiov_set_vf_fb(adapt, idx_vf, entry->fb_offset,
				entry->fb_size);

	amdgv_ffbm_page_table_update_by_fcn(adapt, idx_vf);

	if (amdgv_vfmgr_init_vf_fb(adapt, idx_vf, false, 0x00, 0))
		AMDGV_WARN("Failed to init vf fb\n");
}

void amdgv_vfmgr_import_vfs_fb(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	uint32_t vf_fb_offset_tmr = 0, vf_fb_size_tmr = 0;
	uint32_t pf_fb_size, vf_fb_offset, vf_fb_size;
	uint32_t total_usable_fb, total_vf_fb_size;

	if (adapt->ffbm.enabled && adapt->ffbm.share_tmr) {
		amdgv_gpuiov_get_usable_fb_size(adapt, &total_usable_fb);

		pf_fb_size = adapt->array_vf[AMDGV_PF_IDX].fb_size;
		total_vf_fb_size = total_usable_fb - pf_fb_size;

		vf_fb_size = total_vf_fb_size / adapt->num_vf;

		vf_fb_offset = roundup(pf_fb_size, AMDGV_FUNCTION_FB_ALIGNMENT);
		vf_fb_size = rounddown(vf_fb_size, AMDGV_FUNCTION_FB_ALIGNMENT);

		vf_fb_size_tmr = roundup(vf_fb_size + TO_MBYTES(adapt->psp.tmr_context.mem->len), AMDGV_FUNCTION_FB_ALIGNMENT);
		vf_fb_offset_tmr = vf_fb_offset;

		AMDGV_ASSERT(adapt->tmr_size == vf_fb_size_tmr - vf_fb_size);
	}

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		/* get vf fb settings from GPU */
		if (adapt->ffbm.enabled && adapt->ffbm.share_tmr) {
			/* FFBM WA which includes TMR in VF FB*/
			amdgv_gpuiov_get_vf_fb(adapt, idx_vf,
					       &(adapt->array_vf[idx_vf].fb_offset_tmr),
					       &(adapt->array_vf[idx_vf].fb_size_tmr),
					       &(adapt->array_vf[idx_vf].real_fb_size));

			// For non-active VFs
			if (adapt->array_vf[idx_vf].fb_size_tmr == 0) {
				adapt->array_vf[idx_vf].fb_size_tmr = vf_fb_size_tmr;
				adapt->array_vf[idx_vf].fb_offset_tmr = vf_fb_offset_tmr + idx_vf * vf_fb_size_tmr;
			}
		} else {
			amdgv_gpuiov_get_vf_fb(adapt, idx_vf,
					       &(adapt->array_vf[idx_vf].fb_offset),
					       &(adapt->array_vf[idx_vf].fb_size),
					       &(adapt->array_vf[idx_vf].real_fb_size));
		}
	}
}

static bool amdgv_vfmgr_is_uuid_info_empty(struct amd_sriov_msg_uuid_info *uuid_info)
{
	return (uuid_info->time_low == 0 &&
			uuid_info->time_mid == 0 &&
			uuid_info->time_high == 0 &&
			uuid_info->version == 0 &&
			uuid_info->clk_seq_hi == 0 &&
			uuid_info->variant == 0 &&
			uuid_info->clk_seq_low == 0 &&
			uuid_info->asic_4 == 0 &&
			uuid_info->asic_0 == 0);
}

static void amdgv_vfmgr_gen_uuid_info(struct amd_sriov_msg_uuid_info *uuid_info,
				      uint64_t serial, uint16_t did, uint8_t idx)
{
	uint16_t clk_seq = 0;

	if (amdgv_vfmgr_is_uuid_info_empty(uuid_info)) {
		/* Step1: insert clk_seq */
		uuid_info->clk_seq_low = (uint8_t)clk_seq;
		uuid_info->clk_seq_hi = (uint8_t)(clk_seq >> 8) & 0x3F;

		/* Step2: insert did */
		uuid_info->did = did;

		/* Step3: insert vf idx */
		uuid_info->fcn = idx;

		/* Step4: insert serial */
		uuid_info->asic_0 = (uint32_t)serial;
		uuid_info->asic_4 = (uint16_t)(serial >> 4 * 8) & 0xFFFF;
		uuid_info->asic_6 = (uint8_t)(serial >> 6 * 8) & 0xFF;
		uuid_info->asic_7 = (uint8_t)(serial >> 7 * 8) & 0xFF;

		/* Step5: insert version */
		uuid_info->version = 1;
		/* Step6: insert variant */
		uuid_info->variant = 2;
	}
}

void amdgv_vfmgr_init_pf_config(struct amdgv_adapter *adapt)
{
	int j;
	struct amdgv_vf_option *pf_option;
	struct amdgv_vf_device *pf_entry = &adapt->array_vf[AMDGV_PF_IDX];
	uint64_t pf_fb_min_size = AMDGV_MIN_PF_SIZE;
	uint64_t fb_alignment_byte = MBYTES_TO_BYTES(AMDGV_FUNCTION_FB_ALIGNMENT);
	uint32_t pf_fb_size;

	pf_entry->idx_vf = AMDGV_PF_IDX;
	pf_entry->func_id = 0;
	pf_entry->fb_offset = 0;

	/* if the memory manager is enabled query the PF usage */
	if (adapt->memmgr_pf.is_init) {
		amdgv_memmgr_get_tom(&adapt->memmgr_pf, &pf_fb_min_size);
		pf_fb_min_size = roundup(pf_fb_min_size, fb_alignment_byte);
		pf_fb_min_size = TO_MBYTES(pf_fb_min_size);
	}

	if (adapt->flags & AMDGV_FLAG_USE_PF) {
		if (adapt->opt.pf_option) {
			/* copy user specified pf configuration */
			pf_option = adapt->opt.pf_option;

			/* if pf frame buffer size is less then min pf, use
			 * min pf, otherwise round up to 16MB
			 */
			pf_entry->fb_size = (pf_fb_min_size > pf_option->fb_size) ?
						    pf_fb_min_size :
						    roundup(pf_option->fb_size,
							    AMDGV_FUNCTION_FB_ALIGNMENT);

			pf_entry->time_slice[AMDGV_SCHED_BLOCK_GFX] =
				(pf_option->gfx_time_slice) ?
					pf_option->gfx_time_slice :
					GET_GFX_TIME_SLICE(adapt,
							   adapt->sched.num_vf_per_gfx_sched);

			for (j = 0; j < AMDGV_MAX_MM_ENGINE; j++)
				pf_entry->mm_bandwidth[j] = pf_option->mm_bandwidth[j];
		} else {
			/* set pf configuration to default values */
			pf_entry->fb_size = (pf_fb_min_size > AMDGV_DEFAULT_USE_PF_SIZE) ?
						    pf_fb_min_size :
						    AMDGV_DEFAULT_USE_PF_SIZE;

			pf_entry->time_slice[AMDGV_SCHED_BLOCK_GFX] =
				amdgv_sched_default_gfx_time_slice(
					adapt, adapt->sched.num_vf_per_gfx_sched);

			for (j = 0; j < AMDGV_MAX_MM_ENGINE; j++)
				pf_entry->mm_bandwidth[j] = DEFAULT_MM_ENGINE_BANDWIDTH;
		}

		amdgv_vfmgr_parse_mm_bandwidth(adapt, AMDGV_PF_IDX);
	} else {
		/* if PF isn't used, just assign framebuffer */
		pf_entry->fb_size = pf_fb_min_size;
	}

	/* Update memmger_pf size with real pf size */
	if (adapt->memmgr_pf.is_init)
		amdgv_memmgr_set_size(&adapt->memmgr_pf, MBYTES_TO_BYTES(pf_entry->fb_size));

	amdgv_vfmgr_gen_uuid_info(&pf_entry->uuid_info, adapt->serial, (uint16_t)adapt->dev_id,
				  0xff);

	pf_entry->configured = true;

	/* print LARGE-BAR setting */
	pf_fb_size = TO_MBYTES(adapt->fb_size);
	AMDGV_INFO("LARGE_BAR=%s: FB Aperture Size=0x%llx (%d MB)\n",
		   ((pf_fb_size > 256) ? "true" : "false"), adapt->fb_size, pf_fb_size);
}

void amdgv_vfmgr_init_pf_gfx_time_slice(struct amdgv_adapter *adapt)
{
	struct amdgv_vf_device *pf_entry = &adapt->array_vf[AMDGV_PF_IDX];

	pf_entry->idx_vf = AMDGV_PF_IDX;

	pf_entry->time_slice[AMDGV_SCHED_BLOCK_GFX] =
		amdgv_sched_default_gfx_time_slice(
			adapt, adapt->sched.num_vf_per_gfx_sched);

	AMDGV_INFO("Updated PF timeslice for gfx engine\n");

}

struct amdgv_vf_fb_block *amdgv_vfmgr_find_fb_block_by_fcn(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_vf_fb_block *entry;
	amdgv_list_for_each_entry(entry, &adapt->vf_fb_block_list, struct amdgv_vf_fb_block, vf_fb_block_node) {
		if (entry->allocated && entry->idx_vf == idx_vf)
			return entry;
	}

	return NULL;
}

static void amdgv_vfmgr_insert_fb_block(struct amdgv_adapter *adapt, struct amdgv_vf_fb_block *fb_block)
{
	struct amdgv_vf_fb_block *tmp;
	amdgv_list_for_each_entry(tmp, &adapt->vf_fb_block_list, struct amdgv_vf_fb_block, vf_fb_block_node) {
		if (tmp->fb_offset_tlb >= fb_block->fb_offset_tlb)
			break;
	}
	amdgv_list_add(&fb_block->vf_fb_block_node, tmp->vf_fb_block_node.prev);
}

uint32_t amdgv_vfmgr_calculate_fb_size_tlb(struct amdgv_adapter *adapt, uint32_t fb_size)
{
	uint32_t fb_size_tlb;

	if (adapt->ffbm.enabled && adapt->ffbm.share_tmr)
		fb_size_tlb =
			roundup(fb_size +
				TO_MBYTES(adapt->psp.tmr_context.mem->len) * (fb_size / AMDGV_MIN_FB_SIZE_MB) * 11 / 10,
				AMDGV_FUNCTION_FB_ALIGNMENT);
	else
		fb_size_tlb = fb_size;

	return fb_size_tlb;
}

struct amdgv_vf_fb_block *amdgv_vfmgr_create_fb_block(struct amdgv_adapter *adapt, uint32_t idx_vf,
				uint32_t fb_offset_tlb, uint32_t fb_size, bool free)
{
	struct amdgv_vf_fb_block *entry = NULL;
	uint32_t i;
	uint32_t fb_size_tlb;

	if (!fb_size) {
		AMDGV_ERROR("Should not create fb block with size 0\n");
		return NULL;
	}

	for (i = 0; i < AMDGV_MAX_FB_BLOCK_NUM; i++)
		if (!adapt->vf_fb_block[i].used) {
			oss_memset(&adapt->vf_fb_block[i], 0, sizeof(struct amdgv_vf_fb_block));
			adapt->vf_fb_block[i].used = true;
			entry = &adapt->vf_fb_block[i];
			break;
		}

	if (!entry) {
		AMDGV_ERROR("No empty block found.\n");
		return NULL;
	}

	fb_size_tlb = amdgv_vfmgr_calculate_fb_size_tlb(adapt, fb_size);

	entry->allocated = !free;
	entry->idx_vf = idx_vf;
	entry->fb_size = fb_size;
	entry->fb_size_tlb = fb_size_tlb;
	entry->fb_offset_tlb = fb_offset_tlb;

	AMDGV_INIT_LIST_HEAD(&entry->vf_fb_block_node);
	amdgv_vfmgr_insert_fb_block(adapt, entry);

	return entry;
}

static void amdgv_vfmgr_remove_fb_block(struct amdgv_adapter *adapt, struct amdgv_vf_fb_block *fb_block)
{
	if (!fb_block)
		return;

	amdgv_list_del(&fb_block->vf_fb_block_node);
	fb_block->used = false;
}

static void amdgv_vfmgr_merge_free_blocks(struct amdgv_adapter *adapt, struct amdgv_vf_fb_block *fb_block)
{
	struct amdgv_vf_fb_block *tmp_block;
	tmp_block = amdgv_list_entry(fb_block->vf_fb_block_node.prev,
					struct amdgv_vf_fb_block, vf_fb_block_node);
	if (tmp_block->used && !tmp_block->allocated) {
		tmp_block->fb_size += fb_block->fb_size;
		tmp_block->fb_size_tlb += fb_block->fb_size_tlb;
		amdgv_vfmgr_remove_fb_block(adapt, fb_block);
		fb_block = tmp_block;
	}

	tmp_block = amdgv_list_entry(fb_block->vf_fb_block_node.next,
					struct amdgv_vf_fb_block, vf_fb_block_node);
	if (tmp_block->used && !tmp_block->allocated) {
		fb_block->fb_size += tmp_block->fb_size;
		fb_block->fb_size_tlb += tmp_block->fb_size_tlb;
		amdgv_vfmgr_remove_fb_block(adapt, tmp_block);
	}
}

void amdgv_vfmgr_free_fb_block(struct amdgv_adapter *adapt, struct amdgv_vf_fb_block *fb_block)
{
	if (!fb_block)
		return;

	fb_block->allocated = false;
	amdgv_vfmgr_merge_free_blocks(adapt, fb_block);
}

int amdgv_vfmgr_divide_fb_block(struct amdgv_adapter *adapt, struct amdgv_vf_fb_block *fb_block,
				uint32_t fb_size)
{
	struct amdgv_vf_fb_block *new_block;
	uint32_t fb_size_left;

	fb_size_left = fb_block->fb_size - fb_size;
	/* No need to divide if current node size equals. */
	if (!fb_size || !fb_size_left)
		return 0;

	fb_block->fb_size = fb_size;
	fb_block->fb_size_tlb = amdgv_vfmgr_calculate_fb_size_tlb(adapt, fb_size);
	new_block = amdgv_vfmgr_create_fb_block(adapt, AMDGV_INVALID_IDX_VF, fb_block->fb_offset_tlb + fb_block->fb_size_tlb,
						fb_size_left, true);
	if (!new_block) {
		AMDGV_ERROR("Could not create new block\n");
		return AMDGV_FAILURE;
	}

	amdgv_vfmgr_merge_free_blocks(adapt, new_block);

	return 0;
}

struct amdgv_vf_fb_block *amdgv_vfmgr_find_usable_free_block(struct amdgv_adapter *adapt, uint32_t fb_size)
{
	struct amdgv_vf_fb_block *entry, *ret = NULL;
	amdgv_list_for_each_entry(entry, &adapt->vf_fb_block_list, struct amdgv_vf_fb_block, vf_fb_block_node) {
		if (!entry->allocated && entry->fb_size >= fb_size) {
			ret = entry;
			break;
		}
	}

	return ret;
}

bool amdgv_vfmgr_check_fb_assignable(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t fb_size)
{
	struct amdgv_vf_fb_block *tmp_block, *fb_block = NULL;
	uint32_t free_size;

	if (amdgv_vfmgr_find_usable_free_block(adapt, fb_size))
		return true;

	fb_block = amdgv_vfmgr_find_fb_block_by_fcn(adapt, idx_vf);
	if (fb_block) {
		free_size = fb_block->fb_size;
		tmp_block = amdgv_list_entry(fb_block->vf_fb_block_node.prev,
						struct amdgv_vf_fb_block, vf_fb_block_node);
		if (tmp_block->used && !tmp_block->allocated)
			free_size += tmp_block->fb_size;

		tmp_block = amdgv_list_entry(fb_block->vf_fb_block_node.next,
						struct amdgv_vf_fb_block, vf_fb_block_node);
		if (tmp_block->used && !tmp_block->allocated)
			free_size += tmp_block->fb_size;

		if (free_size >= fb_size)
			return true;
	}

	return false;
}

static int amdgv_vfmgr_asymmetric_fb_reconfig(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t fb_size)
{
	if (idx_vf == AMDGV_PF_IDX) {
		AMDGV_ERROR("Cannot reconfig PF's fb\n");
		return AMDGV_FAILURE;
	}

	if (fb_size < AMDGV_MIN_FB_SIZE_MB) {
		AMDGV_ERROR("Could not set VF fb size less than %d MB\n", AMDGV_MIN_FB_SIZE_MB);
		return AMDGV_FAILURE;
	} else if (fb_size & (AMDGV_FUNCTION_FB_ALIGNMENT - 1)) {
		fb_size = rounddown(fb_size, AMDGV_FUNCTION_FB_ALIGNMENT);
		AMDGV_WARN("fb size not aligned to %dMB, round down to %dMB\n", AMDGV_FUNCTION_FB_ALIGNMENT, fb_size);
	}

	return adapt->sched.asymmetric_fb_reconfig(adapt, idx_vf, fb_size);
}

int amdgv_vfmgr_vf_fb_resize(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t fb_size)
{
	int ret = 0;
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];

	if (!adapt->asymmetric_fb_enabled || !adapt->sched.asymmetric_fb_reconfig) {
		AMDGV_ERROR("Asymmetric FB is not supported on current asic.\n");
		return AMDGV_FAILURE;
	}

	if (is_active_vf(idx_vf)) {
		AMDGV_ERROR("Could not change active VF fb size.\n");
		return AMDGV_FAILURE;
	}

	if (!fb_size)
		return amdgv_vfmgr_free_vf(adapt, idx_vf);

	/* Reconfig vf fb layout. Apply to HW if no error. */
	ret = amdgv_vfmgr_asymmetric_fb_reconfig(adapt, idx_vf, fb_size);
	if (!ret) {
		amdgv_vfmgr_set_vf_fb(adapt, idx_vf);
		/* check whether need to map vf device resources */
		if ((entry->dev != AMDGV_INVALID_HANDLE) && (!entry->res_mapped))
			entry->res_mapped = !oss_map_vf_dev_res(entry->dev, &entry->res);
	}

	return ret;
}

void amdgv_vfmgr_read_asymmetric_fb_layout(struct amdgv_adapter *adapt, char *layout_buf, int *len, uint32_t resv_size)
{
	struct amdgv_vf_fb_block *fb_block;
	int length;

	length = oss_vsnprintf(layout_buf, resv_size, "Current Available VF Number: %d\nFB Layout:\n", adapt->num_vf);

	amdgv_list_for_each_entry(fb_block, &adapt->vf_fb_block_list, struct amdgv_vf_fb_block, vf_fb_block_node) {
		length += oss_vsnprintf(layout_buf + length, resv_size - length,
						"%s - %s - %dMB\n",
						fb_block->allocated ? amdgv_idx_to_str(fb_block->idx_vf) : "Free",
						fb_block->allocated ? (is_active_vf(fb_block->idx_vf) ? "active" : "inactive") : "None",
						fb_block->fb_size);
	}

	*len = length;
}

int amdgv_vfmgr_vf_fb_defragment(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct amdgv_vf_fb_block *fb_block;
	struct amdgv_vf_device *entry;
	uint32_t idx_vf;

	if (!adapt->asymmetric_fb_enabled || !adapt->sched.asymmetric_fb_reconfig) {
		AMDGV_ERROR("Asymmetric FB is not supported on current asic.\n");
		return AMDGV_FAILURE;
	}

	amdgv_sched_stop_all(adapt);

	/* Free all blocks including active VF.
	 * Defragment will re-apply FFBM mapping instead of changing FB position,
	 * so active VF will not be affected */
	amdgv_list_for_each_entry(fb_block, &adapt->vf_fb_block_list, struct amdgv_vf_fb_block, vf_fb_block_node) {
		if (fb_block->allocated) {
			amdgv_vfmgr_free_fb_block(adapt, fb_block);
			/* Invalidate FFBM before reordering VF mapping, or else the mapping will be incorrect.
			 * Reserve the pteb so that could be re-applied later.
			 */
			amdgv_ffbm_unmap_by_fcn(adapt, fb_block->idx_vf, true);
		}
	}
	/* Create fb_block for each vf.
	 * No need to remap FFBM, just re-apply pteb during defragment.
	 */
	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];
		if (!entry->configured || !entry->fb_size)
			continue;
		if (!amdgv_vfmgr_asymmetric_fb_reconfig(adapt, idx_vf, entry->fb_size)) {
			if (adapt->ffbm.enabled && adapt->ffbm.share_tmr)
				amdgv_gpuiov_set_vf_fb(adapt, idx_vf, entry->fb_offset_tmr,
						entry->fb_size_tmr);
			else
				amdgv_gpuiov_set_vf_fb(adapt, idx_vf, entry->fb_offset,
						entry->fb_size);
		} else {
			AMDGV_WARN("Fail to reconfig vf %d FB.\n", idx_vf);
		}
	}

	amdgv_ffbm_apply_page_table(adapt);
	amdgv_sched_start_all(adapt);

	AMDGV_INFO("Defragment completed\n");

	return ret;
}

static void amdgv_vfmgr_init_vfs_fb_config_tmr(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	uint32_t vf_fb_offset_tmr, vf_fb_size_tmr, pf_fb_size, vf_fb_offset, vf_fb_size;
	uint32_t total_usable_fb, total_vf_fb_size;
	struct amdgv_vf_device *entry;
	struct amdgv_vf_fb_block *fb_block;

	amdgv_gpuiov_get_usable_fb_size(adapt, &total_usable_fb);

	pf_fb_size = adapt->array_vf[AMDGV_PF_IDX].fb_size;
	total_vf_fb_size = total_usable_fb - pf_fb_size;

	/* total vf framebuffer equally split to each enabled vf */
	vf_fb_size = total_vf_fb_size / adapt->num_vf;

	vf_fb_offset = roundup(pf_fb_size, AMDGV_FUNCTION_FB_ALIGNMENT);
	vf_fb_size = rounddown(vf_fb_size, AMDGV_FUNCTION_FB_ALIGNMENT);

	vf_fb_size_tmr =
		roundup(vf_fb_size + TO_MBYTES(adapt->psp.tmr_context.mem->len),
			AMDGV_FUNCTION_FB_ALIGNMENT);
	vf_fb_offset_tmr = vf_fb_offset;

	adapt->tmr_size = vf_fb_size_tmr - vf_fb_size;

	if (adapt->asymmetric_fb_enabled)
		/* Adjust vf_fb_size_tmr in asymmetric mode */
		vf_fb_size_tmr =
			roundup(vf_fb_size +
				TO_MBYTES(adapt->psp.tmr_context.mem->len) * (vf_fb_size / AMDGV_MIN_FB_SIZE_MB) * 11 / 10,
				AMDGV_FUNCTION_FB_ALIGNMENT);

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];
		entry->fb_offset = vf_fb_offset + idx_vf * vf_fb_size;
		entry->fb_size = vf_fb_size;

		entry->fb_size_tmr = vf_fb_size + adapt->tmr_size;	// Additional TMR buffer should not affect ffbm or gpuiov setting
		entry->fb_offset_tmr = vf_fb_offset_tmr + idx_vf * vf_fb_size_tmr;

		if (adapt->asymmetric_fb_enabled) {
			amdgv_vfmgr_remove_fb_block(adapt, amdgv_vfmgr_find_fb_block_by_fcn(adapt, idx_vf));
			fb_block = amdgv_vfmgr_create_fb_block(adapt, idx_vf, entry->fb_offset_tmr, entry->fb_size, false);
			if (!fb_block)
				AMDGV_ERROR("Could not create fb block for vf %d\n", idx_vf);
		}

		AMDGV_INFO(
			"FFBM vf idx: %d fb tmr starts at: %d MB in TLB, vf fb size tmr: %d MB, tmr size: %d\n",
			idx_vf, entry->fb_offset_tmr, entry->fb_size_tmr,
			adapt->tmr_size);
	}
}

void amdgv_vfmgr_init_vfs_config_tmr(struct amdgv_adapter *adapt, bool vf_configured)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;
	int i, j;

	if (!adapt->ffbm.enabled) {
		return;
	}

	if (adapt->customized_vf_config_mode) {
		struct amdgv_vf_option *vf_option;

		/* copy user specified VF configuration */
		for (i = 0; i < adapt->opt.vf_opts_num; i++) {
			vf_option = &adapt->opt.vf_options[i];
			idx_vf = vf_option->idx_vf;
			entry = &adapt->array_vf[idx_vf];

			entry->fb_offset = vf_option->fb_offset;
			entry->fb_size = vf_option->fb_size;

			entry->time_slice[AMDGV_SCHED_BLOCK_GFX] = vf_option->gfx_time_slice;

			for (j = 0; j < AMDGV_MAX_MM_ENGINE; j++)
				entry->mm_bandwidth[j] = vf_option->mm_bandwidth[j];

			/* convert mm bandwidth to time slice */
			amdgv_vfmgr_parse_mm_bandwidth(adapt, idx_vf);

			amdgv_vfmgr_gen_uuid_info(&entry->uuid_info, adapt->serial,
						  (uint16_t)adapt->dev_id, (uint8_t)idx_vf);

			entry->configured = vf_configured;
		}
	} else {
		amdgv_vfmgr_init_vfs_fb_config_tmr(adapt);

		for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
			entry = &adapt->array_vf[idx_vf];
			/* assign default time slice for gfx engine */
			entry->time_slice[AMDGV_SCHED_BLOCK_GFX] =
				amdgv_sched_default_gfx_time_slice(adapt, adapt->num_vf);

			/* assign default bandwidth for each MM engine */
			for (i = 0; i < AMDGV_MAX_MM_ENGINE; i++)
				entry->mm_bandwidth[i] = DEFAULT_MM_ENGINE_BANDWIDTH;

			/* convert mm bandwidth to time slice */
			amdgv_vfmgr_parse_mm_bandwidth(adapt, idx_vf);

			amdgv_vfmgr_gen_uuid_info(&entry->uuid_info, adapt->serial,
						  (uint16_t)adapt->dev_id, (uint8_t)idx_vf);
			entry->configured = vf_configured;
		}
	}
}

void amdgv_vfmgr_init_vfs_config(struct amdgv_adapter *adapt)
{
	int i, j;
	uint32_t idx_vf, pf_fb_size, vf_fb_offset = 0;
	uint32_t numa_node_size, numa_fb_size = 0, tmr_ip_data_size = 0, vf_fb_size = 0;
	uint32_t total_usable_fb, total_vf_fb_size = 0;
	struct amdgv_vf_fb_block *fb_block;
	struct amdgv_vf_device *entry;

	if (adapt->customized_vf_config_mode) {
		struct amdgv_vf_option *vf_option;

		/* copy user specified VF configuration */
		for (i = 0; i < adapt->opt.vf_opts_num; i++) {
			vf_option = &adapt->opt.vf_options[i];
			idx_vf = vf_option->idx_vf;
			entry = &adapt->array_vf[idx_vf];

			entry->fb_offset = vf_option->fb_offset;
			entry->fb_size = vf_option->fb_size;

			entry->time_slice[AMDGV_SCHED_BLOCK_GFX] = vf_option->gfx_time_slice;

			for (j = 0; j < AMDGV_MAX_MM_ENGINE; j++)
				entry->mm_bandwidth[j] = vf_option->mm_bandwidth[j];

			/* convert mm bandwidth to time slice */
			amdgv_vfmgr_parse_mm_bandwidth(adapt, idx_vf);

			amdgv_vfmgr_gen_uuid_info(&entry->uuid_info, adapt->serial,
						  (uint16_t)adapt->dev_id, (uint8_t)idx_vf);

			entry->configured = true;
		}
	} else {
		amdgv_gpuiov_get_usable_fb_size(adapt, &total_usable_fb);

		pf_fb_size = adapt->array_vf[AMDGV_PF_IDX].fb_size;

		if (adapt->mcp.numa_count > 0) {
			numa_node_size = (adapt->mcp.numa_range[0].end - adapt->mcp.numa_range[0].start + 1) >> 20;
			numa_fb_size = numa_node_size * adapt->mcp.numa_count;
			tmr_ip_data_size = numa_fb_size < total_usable_fb ? 0 : numa_fb_size - total_usable_fb;
			AMDGV_DEBUG("numa fb size: %d MB, total usable fb size: %d MB, tmr ip data size: %d MB, pf fb size: %d MB\n",
				   numa_fb_size, total_usable_fb, tmr_ip_data_size, pf_fb_size);
		}

		if (adapt->mcp.numa_count > 1 && adapt->num_vf > 1) {
			if (numa_fb_size < total_usable_fb) {
				/* For handling wrong IP discovery data */
				AMDGV_WARN("Possibly wrong IP discovery data: total usable fb size %d MB > numa fb size %d MB\n",
					   total_usable_fb, numa_fb_size);
				vf_fb_offset = 0;
			} else {
				/* Normal use case handling (MI300/MI308 4VF example):
				 *
				 * |       NUMA0       |      NUMA1       |      NUMA2       |       NUMA3       |
				 * <- numa_node_size  -> (identical for each NUMA node)
				 *
				 * |PF|RSV0|    VF0    |    VF1    | RSV1 |    VF2    | RSV2 |    VF3    ||TMR|IP|
				 *        ->vf_fb_size <- (identical for each VF)
				 *
				 * 1. Each VF FB offset should align with the numa range
				 *    - Generally, VF0 offset starts at pf_fb_size
				 *    - Each VF offset afterwards should start at next numa base
				 * 2. Each VF should have identical FB size
				 * 3. PF can make additional reservation for VF0, VF1 and VF2 so that each VF will have same FB size
				 *    (In this case, tmr_ip_data_size is greater than pf_fb_size)
				 */
				vf_fb_offset = pf_fb_size > tmr_ip_data_size ? pf_fb_size : tmr_ip_data_size;
				vf_fb_offset = roundup(vf_fb_offset, AMDGV_FUNCTION_FB_ALIGNMENT);
				vf_fb_size = rounddown(numa_fb_size / adapt->num_vf - vf_fb_offset, AMDGV_FUNCTION_FB_ALIGNMENT);
				total_vf_fb_size = vf_fb_size * adapt->num_vf;
			}
		}

		if (!vf_fb_offset) {
			total_vf_fb_size = total_usable_fb - pf_fb_size;

			/* total vf framebuffer equally split to each enabled vf */
			vf_fb_size = total_vf_fb_size / adapt->num_vf;

			vf_fb_offset = roundup(pf_fb_size, AMDGV_FUNCTION_FB_ALIGNMENT);
			vf_fb_size = rounddown(vf_fb_size, AMDGV_FUNCTION_FB_ALIGNMENT);
		}
		AMDGV_INFO("total vf fb size: %d MB\n", total_vf_fb_size);
		AMDGV_INFO("vf fb starts at: %d MB, vf fb size: %d MB\n", vf_fb_offset,
			   vf_fb_size);

		/* all enabled vfs configure to default settings */
		for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
			entry = &adapt->array_vf[idx_vf];

			entry->fb_offset = vf_fb_offset;
			entry->fb_size = vf_fb_size;

			AMDGV_DEBUG("vf idx: %d, vf fb offset: %d MB, vf fb size: %d MB\n",
				   idx_vf, entry->fb_offset, entry->fb_size);

			if (adapt->asymmetric_fb_enabled) {
				amdgv_vfmgr_remove_fb_block(adapt, amdgv_vfmgr_find_fb_block_by_fcn(adapt, idx_vf));
				fb_block = amdgv_vfmgr_create_fb_block(adapt, idx_vf, entry->fb_offset, entry->fb_size, false);
				if (!fb_block)
					AMDGV_ERROR("Could not create fb block for vf %d\n", idx_vf);
			}

			/* assign default time slice for gfx engine */
			entry->time_slice[AMDGV_SCHED_BLOCK_GFX] =
				amdgv_sched_default_gfx_time_slice(
					adapt, adapt->sched.num_vf_per_gfx_sched);

			/* assign default bandwidth for each MM engine */
			for (j = 0; j < AMDGV_MAX_MM_ENGINE; j++)
				entry->mm_bandwidth[j] = DEFAULT_MM_ENGINE_BANDWIDTH;

			/* convert mm bandwidth to time slice */
			amdgv_vfmgr_parse_mm_bandwidth(adapt, idx_vf);

			amdgv_vfmgr_gen_uuid_info(&entry->uuid_info, adapt->serial,
						  (uint16_t)adapt->dev_id, (uint8_t)idx_vf);

			entry->configured = true;
			if (adapt->mcp.numa_count > 1 && adapt->num_vf > 1)
				/* Each VF offset after VF0 should start at next numa base */
				vf_fb_offset = numa_fb_size / adapt->num_vf * (idx_vf + 1);
			else
				vf_fb_offset += vf_fb_size;
		}
	}
}

static void amdgv_vfmgr_reset_vfs_to_default_config(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;

	/* vfs config set to default vf config */
	if (adapt->ffbm.enabled && adapt->ffbm.share_tmr)
		amdgv_vfmgr_init_vfs_config_tmr(adapt, true);
	else
		amdgv_vfmgr_init_vfs_config(adapt);

	/* apply vfs settings to hardware, need to swtich to PF */
	amdgv_sched_lock(adapt);

	/* apply all vf settings before vf resource mapping */
	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++)
		amdgv_vfmgr_set_vf_fb(adapt, idx_vf);

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];
		if (!entry->configured)
			continue;
		/* map vf device resources */
		if ((entry->dev != AMDGV_INVALID_HANDLE) && (!entry->res_mapped))
			entry->res_mapped = !oss_map_vf_dev_res(entry->dev, &entry->res);

		/* add configured vf to the scheduler */
		amdgv_sched_update_time_slice(adapt, AMDGV_SCHED_BLOCK_ALL, idx_vf);
		set_to_avail_vf(idx_vf);
	}

	amdgv_sched_queue_resume(adapt);
}

/* this is to be called only from an event manager */
static void amdgv_vfmgr_remove_inactive_vfs(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;
	struct amdgv_vf_fb_block *fb_block;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];

		if (!entry->configured)
			continue;

		if (!is_unavail_vf(idx_vf) && !is_avail_vf(idx_vf)) {
			AMDGV_WARN("Active VFs detected. Report bug\n");
			continue;
		}

		/* unmap vf device resources */
		if (entry->res_mapped) {
			entry->res_mapped = false;
			oss_unmap_vf_dev_res(entry->dev, &entry->res);
		}
		entry->fb_offset_os = 0;
		entry->fb_size_os = 0;
		entry->fb_offset = 0;
		entry->fb_size = 0;
		entry->fb_offset_tmr = 0;
		entry->fb_size_tmr = 0;
		entry->configured = false;
		entry->retired_page = 0;
		entry->bp_block_size = 0;

		if (adapt->asymmetric_fb_enabled) {
			fb_block = amdgv_vfmgr_find_fb_block_by_fcn(adapt, idx_vf);
			amdgv_vfmgr_free_fb_block(adapt, fb_block);
			amdgv_vfmgr_remove_fb_block(adapt, fb_block);
		}

		amdgv_ffbm_unmap_by_fcn(adapt, idx_vf, false);

		/* set its framebuffer to 0 */
		amdgv_gpuiov_set_vf_fb(adapt, idx_vf, 0, 0);

		/* Set the entry to unavail */
		set_to_unavail_vf(idx_vf);
	}
}

static void amdgv_vfmgr_remove_configured_vfs(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;
	struct amdgv_vf_fb_block *fb_block;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];

		if (!entry->configured)
			continue;

		/* do not reset previous active VF */
		if (adapt->sched.array_vf[idx_vf].state != AMDGV_SCHED_ACTIVE ||
		    adapt->live_update_state == AMDGV_LIVE_UPDATE_NONE) {
			/* flr vf */
			amdgv_sched_queue_flr_vf(adapt, idx_vf);

			/* remove vf from the scheduler */
			amdgv_sched_queue_remove_vf(adapt, idx_vf);
		}

		/* unmap vf device resources */
		if (entry->res_mapped) {
			entry->res_mapped = false;
			oss_unmap_vf_dev_res(entry->dev, &entry->res);
		}

		oss_put_dev(entry->dev);
		entry->dev = AMDGV_INVALID_HANDLE;
		entry->fb_offset_os = 0;
		entry->fb_size_os = 0;
		entry->fb_offset = 0;
		entry->fb_size = 0;
		entry->fb_offset_tmr = 0;
		entry->fb_size_tmr = 0;
		entry->configured = false;
		entry->retired_page = 0;
		entry->bp_block_size = 0;
		if (adapt->asymmetric_fb_enabled) {
			fb_block = amdgv_vfmgr_find_fb_block_by_fcn(adapt, idx_vf);
			amdgv_vfmgr_free_fb_block(adapt, fb_block);
			amdgv_vfmgr_remove_fb_block(adapt, fb_block);
		}

		amdgv_ffbm_unmap_by_fcn(adapt, idx_vf, false);

		/* set its framebuffer to 0 */
		amdgv_gpuiov_set_vf_fb(adapt, idx_vf, 0, 0);
	}
}

void amdgv_vfmgr_clean_bp_block_size(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];
		entry->bp_block_size = 0;
		entry->retired_page = 0;
	}
}

void amdgv_vfmgr_clean_vf_bp_block_size(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_vf_device *entry;

	if (idx_vf == AMDGV_PF_IDX)
		return;

	entry = &adapt->array_vf[idx_vf];
	entry->bp_block_size = 0;
	entry->retired_page = 0;

	return;
}

int amdgv_vfmgr_copy_and_calc_checksum_to_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
	uint64_t offset, void *buf, uint32_t size,
	uint32_t *checksum)
{
	*checksum += amd_sriov_msg_checksum(buf, size, 0, 0);
	return amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf, offset, buf, size);
}

static inline enum amd_sriov_ras_telemetry_gpu_block
amdgv_ras_block_to_sriov(enum amdgv_ras_block block)
{
	switch (block) {
	case (AMDGV_RAS_BLOCK__UMC):
		return RAS_TELEMETRY_GPU_BLOCK_UMC;
	case (AMDGV_RAS_BLOCK__SDMA):
		return RAS_TELEMETRY_GPU_BLOCK_SDMA;
	case (AMDGV_RAS_BLOCK__GFX):
		return RAS_TELEMETRY_GPU_BLOCK_GFX;
	case (AMDGV_RAS_BLOCK__MMHUB):
		return RAS_TELEMETRY_GPU_BLOCK_MMHUB;
	case (AMDGV_RAS_BLOCK__ATHUB):
		return RAS_TELEMETRY_GPU_BLOCK_ATHUB;
	case (AMDGV_RAS_BLOCK__PCIE_BIF):
		return RAS_TELEMETRY_GPU_BLOCK_PCIE_BIF;
	case (AMDGV_RAS_BLOCK__HDP):
		return RAS_TELEMETRY_GPU_BLOCK_HDP;
	case (AMDGV_RAS_BLOCK__XGMI_WAFL):
		return RAS_TELEMETRY_GPU_BLOCK_XGMI_WAFL;
	case (AMDGV_RAS_BLOCK__DF):
		return RAS_TELEMETRY_GPU_BLOCK_DF;
	case (AMDGV_RAS_BLOCK__SMN):
		return RAS_TELEMETRY_GPU_BLOCK_SMN;
	case (AMDGV_RAS_BLOCK__SEM):
		return RAS_TELEMETRY_GPU_BLOCK_SEM;
	case (AMDGV_RAS_BLOCK__MP0):
		return RAS_TELEMETRY_GPU_BLOCK_MP0;
	case (AMDGV_RAS_BLOCK__MP1):
		return RAS_TELEMETRY_GPU_BLOCK_MP1;
	case (AMDGV_RAS_BLOCK__FUSE):
		return RAS_TELEMETRY_GPU_BLOCK_FUSE;
	case (AMDGV_RAS_BLOCK__MCA):
		return RAS_TELEMETRY_GPU_BLOCK_MCA;
	case (AMDGV_RAS_BLOCK__VCN):
		return RAS_TELEMETRY_GPU_BLOCK_VCN;
	case (AMDGV_RAS_BLOCK__JPEG):
		return RAS_TELEMETRY_GPU_BLOCK_JPEG;
	case (AMDGV_RAS_BLOCK__IH):
		return RAS_TELEMETRY_GPU_BLOCK_IH;
	case (AMDGV_RAS_BLOCK__MPIO):
		return RAS_TELEMETRY_GPU_BLOCK_MPIO;
	default:
		return RAS_TELEMETRY_GPU_BLOCK_COUNT;
	}
}

int amdgv_vfmgr_dump_ras_error_counts(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret;
	struct amdsriov_ras_telemetry *telemetry = NULL;
	uint32_t ce_count, ue_count, de_count;
	uint32_t ce_overflow_count, ue_overflow_count, de_overflow_count;
	enum amd_sriov_ras_telemetry_gpu_block sriov_blk;
	enum amdgv_ras_block ras_blk;

	/* Do not support multi VF until required. */
	if (adapt->num_vf != 1)
		return AMDGV_FAILURE;

	telemetry = oss_zalloc(sizeof(*telemetry));
	if (!telemetry) {
		amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(*telemetry));
		return AMDGV_FAILURE;
	}

	for (ras_blk = 0; ras_blk < AMDGV_RAS_BLOCK__LAST; ras_blk++) {
		if (!(adapt->ecc.ras_cap & BIT(ras_blk)))
			continue;

		sriov_blk = amdgv_ras_block_to_sriov(ras_blk);
		if (sriov_blk >= RAS_TELEMETRY_GPU_BLOCK_COUNT)
			continue;

		ret = amdgv_mca_count_cache_client_get(adapt, &ce_count, &ue_count, &de_count,
						     &ce_overflow_count,
						     &ue_overflow_count,
						     &de_overflow_count,
						     ras_blk, idx_vf);
		if (ret)
			goto fail;

		telemetry->body.error_count.block[sriov_blk].ce_count = ce_count;
		telemetry->body.error_count.block[sriov_blk].ue_count = ue_count;
		telemetry->body.error_count.block[sriov_blk].de_count = de_count;
		telemetry->body.error_count.block[sriov_blk].ce_overflow_count = ce_overflow_count;
		telemetry->body.error_count.block[sriov_blk].ue_overflow_count = ue_overflow_count;
		telemetry->body.error_count.block[sriov_blk].de_overflow_count = de_overflow_count;
	}

	telemetry->header.used_size = sizeof(struct amd_sriov_ras_telemetry_error_count);
	telemetry->header.checksum = amd_sriov_msg_checksum(
					&telemetry->body,
					sizeof(struct amd_sriov_ras_telemetry_error_count),
					0, 0);

	ret = amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf,
					KBYTES_TO_BYTES(AMD_SRIOV_MSG_RAS_TELEMETRY_OFFSET_KB),
					telemetry, sizeof(*telemetry));

fail:
	oss_free(telemetry);

	return ret;
}

static bool amdgv_vfmgr_cper_is_section_allowed(struct amdgv_adapter *adapt, uint32_t idx_vf,
						struct cper_hdr *hdr, uint32_t section_idx)
{
	struct mca_bank_entry bank = { 0 };
	enum amdgv_ras_block ras_block;
	enum amd_sriov_ras_telemetry_gpu_block sriov_blk;

	switch (amdgv_cper_get_type(adapt, hdr, section_idx)) {
	case AMDGV_CPER_TYPE_FATAL:
		amdgv_mca_get_fatal_bank_from_cper_section(adapt, hdr, section_idx, &bank);
		break;
	case AMDGV_CPER_TYPE_RUNTIME:
		amdgv_mca_get_runtime_bank_from_cper_section(adapt, hdr, section_idx, &bank);
		break;
	default:
		return false;
	}

	if (amdgv_mca_decode_block(adapt, &bank, &ras_block))
		return false;

	sriov_blk = amdgv_ras_block_to_sriov(ras_block);
	if (sriov_blk >= RAS_TELEMETRY_GPU_BLOCK_COUNT)
		return false;

	return (adapt->array_vf[idx_vf].ras.caps.all) & BIT(sriov_blk);
}

static void amdgv_vfmgr_cper_get_allowed_list(struct amdgv_adapter *adapt, uint32_t idx_vf,
					      struct cper_hdr *hdr, bool *allowed_sec,
					      uint32_t *allowed_sec_count)
{
	uint32_t i = 0;

	for (i = 0; i < hdr->sec_cnt; i++) {
		if (amdgv_vfmgr_cper_is_section_allowed(adapt, idx_vf, hdr, i)) {
			(*allowed_sec_count)++;
			allowed_sec[i] = true;
		}
	}
}

static int amdgv_vfmgr_cper_copy_to_vf(struct amdgv_adapter *adapt,
				       uint32_t idx_vf, struct cper_hdr *hdr,
				       uint64_t *fb_offset, uint32_t *checksum)
{
	int ret = AMDGV_FAILURE;
	/* Build VF Allowed CPER list dynamically */
	bool *allowed_sec = NULL;
	uint32_t allowed_sec_count = 0;

	allowed_sec_count = 0;
	allowed_sec = oss_zalloc(sizeof(bool) * hdr->sec_cnt);
	if (!allowed_sec) {
		amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(bool) * hdr->sec_cnt);
		return ret;
	}

	amdgv_vfmgr_cper_get_allowed_list(adapt, idx_vf, hdr, allowed_sec,
					  &allowed_sec_count);

	ret = amdgv_cper_patch_to_vf(adapt, idx_vf, hdr, allowed_sec,
				     allowed_sec_count,
				     fb_offset, checksum);
	oss_free(allowed_sec);

	return ret;
}

int amdgv_vfmgr_dump_cpers(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t vf_rptr, bool *allow_again)
{
	int ret;
	struct amd_sriov_ras_telemetry_header hdr = { 0 };
	struct amd_sriov_ras_cper_dump cper_dump = { 0 };
	struct cper_hdr *cper_hdr = NULL;
	struct amdgv_vf_ras *vf_ras = &adapt->array_vf[idx_vf].ras;
	uint64_t rptr = vf_ras->cper.start_rptr + vf_rptr;
	uint64_t wptr = adapt->cper.wptr;
	uint64_t buf_offset =  offsetof(struct amdsriov_ras_telemetry, body.cper_dump) +
			       offsetof(struct amd_sriov_ras_cper_dump, buf);
	uint64_t fb_offset = KBYTES_TO_BYTES(AMD_SRIOV_MSG_RAS_TELEMETRY_OFFSET_KB) +
			     buf_offset;


	if (adapt->mca.vf_policy == AMDGV_RAS_VF_TELEMETRY_DISABLE)
		return AMDGV_FAILURE;

	/* sanitize VF rptr & calculate overflow */
	if (rptr > wptr) {
		rptr = wptr;
		cper_dump.overflow_count = 0;
	} else {
		cper_dump.overflow_count = CPER_MOVE_TO_FIRST_VALID(rptr) - rptr;
		rptr = CPER_MOVE_TO_FIRST_VALID(rptr);
	}

	for (; rptr < wptr; rptr++) {
		cper_hdr = amdgv_cper_get_ring_entry(adapt, rptr);
		if (!cper_hdr)
			goto fail;

		if (cper_hdr->record_length >
		    KBYTES_TO_BYTES(AMD_SRIOV_RAS_TELEMETRY_SIZE_KB) - buf_offset) {
			AMDGV_ERROR("CPER%d Cannot fit in RAS Telemetry region!\n", rptr);
			continue;
		}

		/* Next entry doesn't fit */
		if ((fb_offset - KBYTES_TO_BYTES(AMD_SRIOV_MSG_RAS_TELEMETRY_OFFSET_KB) +
		    cper_hdr->record_length) > KBYTES_TO_BYTES(AMD_SRIOV_RAS_TELEMETRY_SIZE_KB))
			break;

		if (amdgv_vfmgr_cper_copy_to_vf(adapt, idx_vf,
						cper_hdr, &fb_offset,
						&hdr.checksum))
			goto fail;

		cper_dump.count++;
	}

	if (rptr != wptr) {
		cper_dump.more = true;
	}

	cper_dump.wptr = rptr - vf_ras->cper.start_rptr;

	hdr.checksum  += amd_sriov_msg_checksum(&cper_dump,
					       offsetof(struct amd_sriov_ras_cper_dump, buf),
					       0, 0);
	hdr.used_size =  fb_offset - sizeof(struct amd_sriov_ras_telemetry_header) -
			KBYTES_TO_BYTES(AMD_SRIOV_MSG_RAS_TELEMETRY_OFFSET_KB);

	/* Copy static sized cper_dump fields */
	ret = amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf,
					KBYTES_TO_BYTES(AMD_SRIOV_MSG_RAS_TELEMETRY_OFFSET_KB) +
					sizeof(struct amd_sriov_ras_telemetry_header),
					&cper_dump,
					offsetof(struct amd_sriov_ras_cper_dump, buf));

	/* Copy header */
	if (!ret)
		ret = amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf,
					KBYTES_TO_BYTES(AMD_SRIOV_MSG_RAS_TELEMETRY_OFFSET_KB),
					&hdr,
					sizeof(struct amd_sriov_ras_telemetry_header));

	/* Guest should be consuming records up to wptr. If not, then do not decrement
	 * the event guard, even if more CPERs are pending. */
	if (cper_dump.more && vf_rptr == vf_ras->cper.prev_host_wptr)
		*allow_again = true;
	else
		*allow_again = false;

	vf_ras->cper.prev_host_wptr = cper_dump.wptr;

	return ret;
fail:
	return AMDGV_FAILURE;
}

static void amdgv_vfmgr_cper_rptr_init(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	adapt->array_vf[idx_vf].ras.cper.start_rptr = adapt->cper.wptr;
	adapt->array_vf[idx_vf].ras.cper.prev_host_wptr = 0;
}

int amdgv_vfmgr_cper_notify_event(struct amdgv_adapter *adapt, enum amdgv_vfmgr_cper_event event,
				  uint32_t idx_vf)
{
	switch (event) {
	case VFMGR_CPER_EVENT_GUEST_LOAD:
		if (adapt->opt.ras_vf_telemetry_policy == AMDGV_RAS_VF_TELEMETRY_LOG_ON_GUEST_LOAD)
			amdgv_vfmgr_cper_rptr_init(adapt, idx_vf);
		break;
	case VFMGR_CPER_EVENT_NUM_VF_CHANGE:	/* Fallthrough */
	case VFMGR_CPER_EVENT_DISABLE:		/* Fallthrough */
	case VFMGR_CPER_EVENT_ENABLE:		/* Fallthrough */
	default:
		AMDGV_ERROR("Unsupported vfmgr CPER Event %d\n", event);
		break;
	}

	return 0;
}

int amdgv_vfmgr_sw_init(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;

	adapt->pf2vf_msg = (struct amd_sriov_msg_pf2vf_info *)oss_zalloc(
		sizeof(struct amd_sriov_msg_pf2vf_info));

	if (adapt->pf2vf_msg == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct amd_sriov_msg_pf2vf_info));
		return AMDGV_FAILURE;
	}

	if (adapt->sriov_vf_stride != 1) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_IOV_SRIOV_STRIDE_ERROR, 0);
		return AMDGV_FAILURE;
	}

	/* init all supported vf's slot */
	for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];
		entry->idx_vf = idx_vf;
		entry->func_id = idx_vf;
		entry->bdf = adapt->bdf + adapt->sriov_vf_offset + idx_vf;

		amdgv_guard_vf_init(adapt, idx_vf);
	}

	AMDGV_INIT_LIST_HEAD(&adapt->vf_fb_block_list);

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++)
		amdgv_vfmgr_cper_rptr_init(adapt, idx_vf);

	return 0;
}

int amdgv_vfmgr_sw_fini(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;

	for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];

		amdgv_guard_vf_remove(adapt, idx_vf);
		entry->configured = false;
	}

	oss_free(adapt->pf2vf_msg);
	adapt->pf2vf_msg = NULL;

	return 0;
}

int amdgv_vfmgr_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;

	if (!in_whole_gpu_reset()) {
		/* init pf's sw config */
		amdgv_vfmgr_init_pf_config(adapt);

		/* init vf's sw config */
		/* ffbm wa enabled, tmr is allocated at 2MB with size about 200+ MB */
		if (adapt->ffbm.enabled && adapt->ffbm.share_tmr)
			amdgv_vfmgr_init_vfs_config_tmr(adapt, true);
	}

	if (!(adapt->ffbm.enabled && adapt->ffbm.share_tmr))
			amdgv_vfmgr_init_vfs_config(adapt);

	/* write pf fb settings to hardware */
	amdgv_vfmgr_set_pf_fb(adapt);

	/* write all vf fb settings to hardware */
	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		if (!adapt->array_vf[idx_vf].configured)
			continue;
		amdgv_vfmgr_set_vf_fb(adapt, idx_vf);
	}

	if (!in_whole_gpu_reset())
		amdgv_umc_retrieve_bad_pages(adapt);

	if (adapt->ffbm.enabled)
		amdgv_ffbm_reserve_all_bad_pages(adapt);

	if (in_whole_gpu_reset()) {
		return 0;
	}

	/* if shim drv indicate to use pf, add pf to scheduler */
	if (adapt->flags & AMDGV_FLAG_USE_PF) {
		adapt->array_vf[AMDGV_PF_IDX].dev = adapt->dev;

		amdgv_sched_update_time_slice(adapt, AMDGV_SCHED_BLOCK_ALL, AMDGV_PF_IDX);
		set_to_avail_vf(AMDGV_PF_IDX);
	}

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];

		if (!entry->configured)
			continue;

		/* get vf device handle from its bdf */
		entry->dev = oss_get_dev_from_bdf(entry->bdf);

		/* map vf device resources */
		if ((entry->dev != AMDGV_INVALID_HANDLE) && (!entry->res_mapped))
			entry->res_mapped = !oss_map_vf_dev_res(entry->dev, &entry->res);

		/* add configured vf to the scheduler */
		amdgv_sched_update_time_slice(adapt, AMDGV_SCHED_BLOCK_ALL, idx_vf);
		set_to_avail_vf(idx_vf);
	}

	return 0;
}

int amdgv_vfmgr_hw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->umc.supports_ras_eeprom) {
		/* clean bad page block size */
		amdgv_vfmgr_clean_bp_block_size(adapt);
	}

	if (in_whole_gpu_reset() ||
	    ((adapt->flags & AMDGV_FLAG_USE_PF) && adapt->lock_world_switch))
		return 0;

	/* remove all configured vfs */
	amdgv_vfmgr_remove_configured_vfs(adapt);

	/* if PF is enabled, remove pf from scheduler */
	if (adapt->flags & AMDGV_FLAG_USE_PF)
		amdgv_sched_queue_remove_vf(adapt, AMDGV_PF_IDX);

	return 0;
}

struct amdgv_init_func amdgv_vfmgr_func = {
	.name = "amdgv_vfmgr_func",
	.sw_init = amdgv_vfmgr_sw_init,
	.sw_fini = amdgv_vfmgr_sw_fini,
	.hw_init = amdgv_vfmgr_hw_init,
	.hw_fini = amdgv_vfmgr_hw_fini,
};

void amdgv_vfmgr_enter_customized_vf_mode(struct amdgv_adapter *adapt, int clear_vf)
{
	// Allow re-enter to wipe all VFs
	AMDGV_INFO("Enter active VF management mode\n");

	if (clear_vf) {
		AMDGV_INFO("Clear current VF settings\n");
		amdgv_vfmgr_remove_inactive_vfs(adapt);
	}

	adapt->customized_vf_config_mode = true;
}

void amdgv_vfmgr_exit_customized_vf_mode(struct amdgv_adapter *adapt, int default_vf)
{
	// Allow re-enter to reset to default configs
	AMDGV_INFO("Exit active VF management mode\n");

	if (default_vf) {
		AMDGV_INFO("Apply default VF settings\n");
		amdgv_vfmgr_reset_vfs_to_default_config(adapt);
	}

	adapt->customized_vf_config_mode = false;
}

void amdgv_vfmgr_set_all_vf(struct amdgv_adapter *adapt)
{
	AMDGV_INFO("Apply default VF settings\n");
	amdgv_vfmgr_reset_vfs_to_default_config(adapt);
}

static void amdgv_vfmgr_set_entry(struct amdgv_adapter *adapt, enum amdgv_set_vf_opt_type type,
				  struct amdgv_vf_option *opt)
{
	uint32_t j, idx_vf;
	struct amdgv_vf_device *entry;

	idx_vf = opt->idx_vf;
	entry = &adapt->array_vf[idx_vf];

	/* copy the specified vf config to the slot */
	if (type & AMDGV_SET_VF_FB) {
		/* Save opt->fb_offset as fb_offset_os for vf option validation */
		entry->fb_offset_os = opt->fb_offset;
		entry->fb_size_os = opt->fb_size;

		/* If entry was previously freed, need to reinit tmr config to calculate fb_offset_tmr and fb_size_tmr*/
		if (adapt->tmr_size > 0 && !entry->fb_size_tmr)
			amdgv_vfmgr_init_vfs_fb_config_tmr(adapt);

		if (!(adapt->ffbm.enabled && adapt->ffbm.share_tmr)) {
			/* opt->fb_offset was never used when ffbm enabled. */
			entry->fb_offset = opt->fb_offset;
			entry->fb_size = opt->fb_size;
		}

		if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) || (adapt->live_update_state != AMDGV_LIVE_UPDATE_RESTORE)) {
			if (adapt->asymmetric_fb_enabled)
				amdgv_vfmgr_vf_fb_resize(adapt, idx_vf, entry->fb_size);
			else
				amdgv_vfmgr_set_vf_fb(adapt, idx_vf);
		}
	}

	if ((type & (AMDGV_SET_VF_TIME | AMDGV_SET_VF_GFX_PART)) &&
		(!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) || (adapt->live_update_state != AMDGV_LIVE_UPDATE_RESTORE))) {
		/* if gfx_time_slice is 0, use default value */
		entry->time_slice[AMDGV_SCHED_BLOCK_GFX] =
			(opt->gfx_time_slice) ?
				opt->gfx_time_slice :
				amdgv_sched_default_gfx_time_slice(
					adapt, adapt->sched.num_vf_per_gfx_sched);

		for (j = 0; j < AMDGV_MAX_MM_ENGINE; j++)
			entry->mm_bandwidth[j] = opt->mm_bandwidth[j];

		amdgv_vfmgr_parse_mm_bandwidth(adapt, idx_vf);
	}
}

void amdgv_vfmgr_set_default_fb_size(struct amdgv_adapter *adapt, struct amdgv_vf_option *opt)
{
	uint32_t size;
	uint32_t total_usable_fb;

	amdgv_gpuiov_get_usable_fb_size(adapt, &total_usable_fb);

	/* total vf framebuffer equally split to each enabled vf */
	size = total_usable_fb - adapt->array_vf[AMDGV_PF_IDX].fb_size;
	size = rounddown(size / adapt->num_vf, AMDGV_FUNCTION_FB_ALIGNMENT);

	opt->fb_size = size;
}

int amdgv_vfmgr_alloc_vf(struct amdgv_adapter *adapt, struct amdgv_vf_option *opt)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;

	idx_vf = opt->idx_vf;
	entry = &adapt->array_vf[idx_vf];

	entry->configured = true;
	amdgv_vfmgr_set_entry(adapt, AMDGV_SET_VF_SMI_FULL_OPT, opt);

	amdgv_time_log_clear_vf(adapt, opt->idx_vf);

	/* map the vf's device resources */
	if ((entry->dev != AMDGV_INVALID_HANDLE) && (!entry->res_mapped))
		entry->res_mapped = !oss_map_vf_dev_res(entry->dev, &entry->res);

	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) || (adapt->live_update_state != AMDGV_LIVE_UPDATE_RESTORE)) {
		if (amdgv_vfmgr_init_vf_fb(adapt, idx_vf, false, 0, 0))
			AMDGV_WARN("Failed to init vf fb\n");
	}

	/* make the vf available to scheduler */
	if (adapt->live_update_state != AMDGV_LIVE_UPDATE_RESTORE) {
		amdgv_sched_update_time_slice(adapt, AMDGV_SCHED_BLOCK_ALL, idx_vf);
		set_to_avail_vf(idx_vf);
	}

	return 0;
}

int amdgv_vfmgr_free_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = 0;
	struct amdgv_vf_device *entry;

	/* clear VF FB on VM shutdown/Force-Off */
	ret = amdgv_misc_clear_vf_fb(adapt, idx_vf, 0x00);
	if (ret)
		return ret;

	entry = &adapt->array_vf[idx_vf];

	if (is_active_vf(idx_vf)) {
		amdgv_sched_remove_vf(adapt, idx_vf);
	}

	set_to_unavail_vf(idx_vf);

	if (entry->res_mapped) {
		entry->res_mapped = false;
		oss_unmap_vf_dev_res(entry->dev, &entry->res);
	}
	entry->fb_offset_os = 0;
	entry->fb_size_os = 0;
	entry->fb_offset = 0;
	entry->fb_size = 0;
	entry->fb_offset_tmr = 0;
	entry->fb_size_tmr = 0;
	entry->configured = false;
	entry->retired_page = 0;
	entry->bp_block_size = 0;

	if (adapt->asymmetric_fb_enabled)
		amdgv_vfmgr_free_fb_block(adapt, amdgv_vfmgr_find_fb_block_by_fcn(adapt, idx_vf));

	if (adapt->ffbm.enabled && idx_vf != AMDGV_PF_IDX)
		amdgv_ffbm_unmap_by_fcn(adapt, idx_vf, false);

	amdgv_gpuiov_set_vf_fb(adapt, idx_vf, 0, 0);

	return 0;
}

int amdgv_vfmgr_set_vf_num(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	uint32_t idx_vf, cur_num_vf;
	struct amdgv_vf_device *entry;
	int ret;

	if (num_vf > adapt->max_num_vf)
		return AMDGV_FAILURE;

	AMDGV_INFO("Set enabled VF number to %d\n", num_vf);

	/* remove vf require scheduler not paused */
	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];
		if (!entry->configured)
			continue;

		/* unmap vf device resources */
		if (entry->res_mapped) {
			entry->res_mapped = false;
			oss_unmap_vf_dev_res(entry->dev, &entry->res);
		}

		oss_put_dev(entry->dev);
		entry->dev = AMDGV_INVALID_HANDLE;

		/* if the VF is active VF, move VF out of world switching. */
		if (is_active_vf(idx_vf)) {
			amdgv_sched_remove_vf(adapt, idx_vf);
		}
		entry->fb_offset_os = 0;
		entry->fb_size_os = 0;
		entry->fb_offset = 0;
		entry->fb_size = 0;
		entry->fb_offset_tmr = 0;
		entry->fb_size_tmr = 0;
		entry->configured = false;

		amdgv_ffbm_unmap_by_fcn(adapt, idx_vf, false);
		amdgv_sched_start_all(adapt);

		/* set its framebuffer to 0 */
		amdgv_gpuiov_set_vf_fb(adapt, idx_vf, 0, 0);

		entry->retired_page = 0;
		entry->bp_block_size = 0;
		set_to_unavail_vf(idx_vf);
	}

	/* pause the scheduler and don't switch to PF */
	amdgv_sched_stop_all(adapt);

	/* save current num_vf */
	cur_num_vf = adapt->num_vf;

	/* write the number of vf to pci sriov */
	ret = amdgv_gpuiov_set_sriov_vf_num(adapt, num_vf);

	/* init all enabled vf's slots */
	for (idx_vf = 0; idx_vf < num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];
		entry->idx_vf = idx_vf;
		entry->func_id = idx_vf;
		entry->bdf = adapt->bdf + adapt->sriov_vf_offset + idx_vf;
		entry->dev = oss_get_dev_from_bdf(entry->bdf);

		amdgv_guard_vf_reset(adapt, idx_vf);
	}

	/* notify the scheduler the number of vf has changed */
	amdgv_sched_set_vf_num(adapt, num_vf);

	/* if tmr_size not 0, that means we need apply FFBM WA */
	if (adapt->tmr_size > 0) {
		/* config has been changed, need disable customized_vf_config_mode */
		if (cur_num_vf != num_vf)
			adapt->customized_vf_config_mode = false;
		/* re-init vf's sw config and FFBM WA */
		amdgv_vfmgr_init_vfs_config_tmr(adapt, false);
	}

	if (cur_num_vf != num_vf) {
		/* Update PF gfx timeslice according to new vf num */
		amdgv_vfmgr_init_pf_gfx_time_slice(adapt);
		amdgv_sched_update_time_slice(adapt, AMDGV_SCHED_BLOCK_GFX, AMDGV_PF_IDX);
	}

	/* get new bandwidth config according to new num_vf
	 * TODO: it is better to use profile instead of individual data
	 */
	amdgv_mmsch_get_default_bandwidth_config(adapt, &adapt->mmsch.bandwidth_config);

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->set_workload_profile)
		adapt->pp.pp_funcs->set_workload_profile(adapt);
	return ret;
}

int amdgv_vfmgr_set_vf(struct amdgv_adapter *adapt, enum amdgv_set_vf_opt_type type,
		       struct amdgv_vf_option *vf_option)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;

	idx_vf = vf_option->idx_vf;
	entry = &adapt->array_vf[idx_vf];

	if (type & AMDGV_SET_VF_FB)
		if (!is_avail_vf(idx_vf))
			return AMDGV_FAILURE;

	/* pause the scheduler */
	amdgv_sched_stop_all(adapt);

	/* If a reset happens during WS, fail the API */
	if (!amdgv_sched_world_context_all_states_ok(adapt)) {
		amdgv_sched_reset_vf_auto(adapt);
		return AMDGV_FAILURE;
	}

	amdgv_vfmgr_set_entry(adapt, type, vf_option);

	/* update the time slice */
	if (type & (AMDGV_SET_VF_TIME | AMDGV_SET_VF_GFX_PART))
		amdgv_sched_update_time_slice(adapt, AMDGV_SCHED_BLOCK_ALL, idx_vf);

	return 0;
}

int amdgv_vfmgr_get_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *fb_offset,
			  uint32_t *fb_size)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];

	/* ffbm wa enabled, tmr is allocated at 2MB with size about 200+ MB */
	if (entry->fb_size_tmr > 0)
		*fb_offset = entry->fb_offset_tmr;
	else
		*fb_offset = entry->fb_offset;
	*fb_size = entry->real_fb_size;

	return 0;
}

int amdgv_vfmgr_get_vf_bdf(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *bdf)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];

	*bdf = entry->bdf;

	return 0;
}

int amdgv_vfmgr_copy_to_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t offset,
			      void *buf, uint32_t size)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	uint64_t vf_fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
	int ret = 0;

	if (!entry->configured)
		return AMDGV_FAILURE;

	if (!entry->res_mapped) {
		if (entry->dev != AMDGV_INVALID_HANDLE)
			entry->res_mapped = !oss_map_vf_dev_res(entry->dev, &entry->res);
	}

	if ((adapt->flags & AMDGV_FLAG_USE_PF) && (offset >= MAX_OS_FB_MAPPING_SIZE)) {
		if (adapt->ffbm.enabled)
			ret = amdgv_ffbm_copy_to_gpa(adapt, buf, idx_vf, offset, size, AMDGV_FFBM_MM_COPY);
		else
			amdgv_mm_copy_to_fb(adapt, ((uint64_t)entry->fb_offset << 20) + offset,
								(uint64_t)buf, size);
	} else if (entry->res_mapped && (entry->res.fb_size > offset + size) && amdgv_gpuiov_get_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_FB)) {
		oss_memcpy((uint8_t *)entry->res.fb + offset, buf, size);
	} else if (adapt->ffbm.enabled) {
		if (adapt->fb_size >= vf_fb_offset + offset + size)
			ret = amdgv_ffbm_copy_to_gpa(adapt, buf, idx_vf, offset, size, AMDGV_FFBM_PF_COPY);
		else
			ret = amdgv_ffbm_copy_to_gpa(adapt, buf, idx_vf, offset, size, AMDGV_FFBM_MM_COPY);
	} else {
		if (adapt->fb_size >= vf_fb_offset + offset + size)
			oss_memcpy((uint8_t *)adapt->fb + vf_fb_offset + offset, buf, size);
		else
			amdgv_mm_copy_to_fb(adapt, ((uint64_t)entry->fb_offset << 20) + offset,
								(uint64_t)buf, size);
	}

	return ret;
}

int amdgv_vfmgr_copy_from_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t offset,
				void *buf, uint32_t size)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	uint64_t vf_fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
	int ret = 0;

	if (!entry->configured)
		return AMDGV_FAILURE;

	if (!entry->res_mapped) {
		if (entry->dev != AMDGV_INVALID_HANDLE)
			entry->res_mapped = !oss_map_vf_dev_res(entry->dev, &entry->res);
		else
			return AMDGV_FAILURE;
	}

	if ((adapt->flags & AMDGV_FLAG_USE_PF) && (offset >= MAX_OS_FB_MAPPING_SIZE)) {
		if (adapt->ffbm.enabled)
			ret = amdgv_ffbm_copy_from_gpa(adapt, buf, idx_vf, offset, size, AMDGV_FFBM_MM_COPY);
		else
			amdgv_mm_copy_from_fb(adapt, (uint64_t)buf,
								  ((uint64_t)entry->fb_offset << 20) + offset, size);
	} else if (entry->res_mapped && (entry->res.fb_size > offset + size)  && amdgv_gpuiov_get_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_FB)) {
		oss_memcpy(buf, ((uint8_t *)entry->res.fb) + offset, size);
	} else if (adapt->ffbm.enabled) {
		if (adapt->fb_size >= vf_fb_offset + offset + size)
			ret = amdgv_ffbm_copy_from_gpa(adapt, buf, idx_vf, offset, size, AMDGV_FFBM_PF_COPY);
		else
			ret = amdgv_ffbm_copy_from_gpa(adapt, buf, idx_vf, offset, size, AMDGV_FFBM_MM_COPY);
	} else {
		if (adapt->fb_size >= vf_fb_offset + offset + size)
			oss_memcpy(buf, (uint8_t *)adapt->fb + vf_fb_offset + offset, size);
		else
			amdgv_mm_copy_from_fb(adapt, (uint64_t)buf,
								  ((uint64_t)entry->fb_offset << 20) + offset, size);
	}

	return ret;
}

int amdgv_vfmgr_update_bp_message(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = 0;
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];

	if (!entry->configured)
		return AMDGV_FAILURE;

	if (entry->retired_page != 0) {
		/* translate retired page in VF view */
		entry->retired_page =
			entry->retired_page -
			(MBYTES_TO_BYTES(entry->fb_offset) >> AMDGV_GPU_PAGE_SHIFT);

		ret = amdgv_vfmgr_copy_to_vf_fb(
			adapt, idx_vf,
			KBYTES_TO_BYTES(AMD_SRIOV_MSG_BAD_PAGE_OFFSET_KB) +
				entry->bp_block_size,
			&entry->retired_page, sizeof(uint64_t));
		if (ret != 0)
			AMDGV_WARN(
				"Copy retired page %d to VF%d data exchange region failed\n",
				entry->retired_page + (MBYTES_TO_BYTES(entry->fb_offset) >>
						       AMDGV_GPU_PAGE_SHIFT),
				idx_vf);
		else
			entry->bp_block_size += sizeof(uint64_t);

		/* prepare for next update pf2vf */
		entry->retired_page = 0;
	}

	return ret;
}

static int amdgv_vfmgr_update_vf2vf_ras_caps(struct amdgv_adapter *adapt,
					     struct amd_sriov_msg_pf2vf_info *pf2vf_msg,
					     uint32_t idx_vf)
{
	/* Only allow RAS telemetry on MCA enabled GPUs */
	pf2vf_msg->feature_flags.flags.ras_caps = 1;
	pf2vf_msg->ras_en_caps.all = 0;

	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__UMC))
		pf2vf_msg->ras_en_caps.bits.block_umc = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__SDMA))
		pf2vf_msg->ras_en_caps.bits.block_sdma = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__GFX))
		pf2vf_msg->ras_en_caps.bits.block_gfx = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__MMHUB))
		pf2vf_msg->ras_en_caps.bits.block_mmhub = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__ATHUB))
		pf2vf_msg->ras_en_caps.bits.block_athub = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__PCIE_BIF))
		pf2vf_msg->ras_en_caps.bits.block_pcie_bif = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__HDP))
		pf2vf_msg->ras_en_caps.bits.block_hdp = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__XGMI_WAFL))
		pf2vf_msg->ras_en_caps.bits.block_xgmi_wafl = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__DF))
		pf2vf_msg->ras_en_caps.bits.block_df = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__SMN))
		pf2vf_msg->ras_en_caps.bits.block_smn = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__SEM))
		pf2vf_msg->ras_en_caps.bits.block_sem = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__MP0))
		pf2vf_msg->ras_en_caps.bits.block_mp0 = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__MP1))
		pf2vf_msg->ras_en_caps.bits.block_mp1 = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__FUSE))
		pf2vf_msg->ras_en_caps.bits.block_fuse = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__MCA))
		pf2vf_msg->ras_en_caps.bits.block_mca = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__VCN))
		pf2vf_msg->ras_en_caps.bits.block_vcn = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__JPEG))
		pf2vf_msg->ras_en_caps.bits.block_jpeg = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__IH))
		pf2vf_msg->ras_en_caps.bits.block_ih = 1;
	if (adapt->ecc.ras_cap & BIT(AMDGV_RAS_BLOCK__MPIO))
		pf2vf_msg->ras_en_caps.bits.block_mpio = 1;

	if (adapt->ecc.supported & BIT(AMDGV_RAS_POISON_ECC_SUPPORT))
		pf2vf_msg->ras_en_caps.bits.poison_propogation_mode = 1;

	if (!adapt->mca.enabled)
		return 0;

	/* Support only 1 VF for now. */
	if (adapt->num_vf != 1)
		return 0;

	if (adapt->mca.vf_policy == AMDGV_RAS_VF_TELEMETRY_DISABLE)
		return 0;

	AMDGV_INFO("Host Supports VF RAS Telemetry\n");

	pf2vf_msg->feature_flags.flags.ras_telemetry = 1;
	pf2vf_msg->ras_telemetry_en_caps.all = 0;

	if (adapt->cper.enabled)
		pf2vf_msg->feature_flags.flags.ras_cper = 1;

	if (amdgv_xgmi_all_nodes_fb_sharing(adapt)) {
		if (pf2vf_msg->ras_en_caps.bits.block_xgmi_wafl)
			pf2vf_msg->ras_telemetry_en_caps.bits.block_xgmi_wafl = 1;
		if (pf2vf_msg->ras_en_caps.bits.block_df)
			pf2vf_msg->ras_telemetry_en_caps.bits.block_df = 1;
	}

	if (pf2vf_msg->ras_en_caps.bits.block_gfx)
		pf2vf_msg->ras_telemetry_en_caps.bits.block_gfx = 1;
	if (pf2vf_msg->ras_en_caps.bits.block_umc)
		pf2vf_msg->ras_telemetry_en_caps.bits.block_umc = 1;
	if (pf2vf_msg->ras_en_caps.bits.block_pcie_bif)
		pf2vf_msg->ras_telemetry_en_caps.bits.block_pcie_bif = 1;
	if (pf2vf_msg->ras_en_caps.bits.block_mmhub)
		pf2vf_msg->ras_telemetry_en_caps.bits.block_mmhub = 1;

	if (pf2vf_msg->ras_en_caps.bits.block_sdma)
		pf2vf_msg->ras_telemetry_en_caps.bits.block_sdma = 1;

	adapt->array_vf[idx_vf].ras.caps = pf2vf_msg->ras_telemetry_en_caps;

	return 0;
}

int amdgv_vfmgr_update_pf2vf_message(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret;
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	struct amd_sriov_msg_pf2vf_info *pf2vf_msg = adapt->pf2vf_msg;

	if (!entry->configured)
		return AMDGV_FAILURE;

	if (pf2vf_msg == NULL)
		return AMDGV_FAILURE;

	/* FFBM replace bp on host side */
	if (adapt->umc.supports_ras_eeprom && !adapt->ffbm.enabled) {
		/* handle pf2vf message for bad page info */
		amdgv_umc_notify_vf_bp_records(adapt, idx_vf);
	}

	pf2vf_msg->header.size = sizeof(struct amd_sriov_msg_pf2vf_info);
	pf2vf_msg->header.version = AMD_SRIOV_MSG_FW_VRAM_PF2VF_VER;
	/* todo, will enable the flag later. */
	pf2vf_msg->feature_flags.all = 0;
	if (adapt->fw_load_type == AMDGV_FW_LOAD_BY_GIM_PHASE_2)
		pf2vf_msg->feature_flags.flags.host_load_ucodes = 1;

	pf2vf_msg->feature_flags.flags.host_flr_vramlost =
		adapt->array_vf[idx_vf].vram_lost ? 1 : 0;

	pf2vf_msg->feature_flags.flags.error_log_collect = 1;

	if ((adapt->num_vf == 1) && (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH) &&
	    !(adapt->flags & AMDGV_FLAG_SMU_NO_SUPPORT_PP_ONE_VF))
		pf2vf_msg->feature_flags.flags.pp_one_vf_mode = 1;
	AMDGV_DEBUG("One vf mode is %s\n",
		   pf2vf_msg->feature_flags.flags.pp_one_vf_mode ? "ON" : "OFF");

	if (adapt->flags & AMDGV_FLAG_GC_REG_RLC_EN ||
	    adapt->flags & AMDGV_FLAG_MMHUB_REG_RLC_EN ||
	    adapt->flags & AMDGV_FLAG_IH_REG_PSP_EN)
		pf2vf_msg->feature_flags.flags.reg_indirect_acc = 1;
	else
		pf2vf_msg->feature_flags.flags.reg_indirect_acc = 0;

	AMDGV_DEBUG("reg indirectly access %s\n",
		   pf2vf_msg->feature_flags.flags.reg_indirect_acc ? "on" : "off");

	if (adapt->flags & AMDGV_FLAG_GC_REG_RLC_EN)
		pf2vf_msg->reg_access_flags.flags.vf_reg_access_gc = 1;
	else
		pf2vf_msg->reg_access_flags.flags.vf_reg_access_gc = 0;

	if (adapt->flags & AMDGV_FLAG_MMHUB_REG_RLC_EN)
		pf2vf_msg->reg_access_flags.flags.vf_reg_access_mmhub = 1;
	else
		pf2vf_msg->reg_access_flags.flags.vf_reg_access_mmhub = 0;

	if (adapt->flags & AMDGV_FLAG_IH_REG_PSP_EN)
		pf2vf_msg->reg_access_flags.flags.vf_reg_access_ih = 1;
	else
		pf2vf_msg->reg_access_flags.flags.vf_reg_access_ih = 0;

	if (adapt->flags & AMDGV_FLAG_L1_TLB_CNTL_REG_PSP_EN)
		pf2vf_msg->reg_access_flags.flags.vf_reg_access_l1_tlb_cntl = 1;
	else
		pf2vf_msg->reg_access_flags.flags.vf_reg_access_l1_tlb_cntl = 0;

	AMDGV_DEBUG("vf_reg_access_gc is %s "
		    "vf_reg_access_mmhub is %s "
		    "vf_reg_access_ih is %s "
		    "vf_reg_access_l1_tlb_cntl is %s\n",
		    pf2vf_msg->reg_access_flags.flags.vf_reg_access_gc ? "on" : "off",
		    pf2vf_msg->reg_access_flags.flags.vf_reg_access_mmhub ? "on" : "off",
		    pf2vf_msg->reg_access_flags.flags.vf_reg_access_ih ? "on" : "off",
		    pf2vf_msg->reg_access_flags.flags.vf_reg_access_l1_tlb_cntl ? "on" : "off");

	pf2vf_msg->hevc_enc_max_mb_per_second = entry->mm_bandwidth[AMDGV_HEVC_ENGINE];
	pf2vf_msg->hevc_enc_max_mb_per_frame =
		entry->mm_bandwidth[AMDGV_HEVC_ENGINE] / DEFAULT_MM_ENGINE_FPS;
	pf2vf_msg->avc_enc_max_mb_per_second = entry->mm_bandwidth[AMDGV_VCE_ENGINE];
	pf2vf_msg->avc_enc_max_mb_per_frame =
		entry->mm_bandwidth[AMDGV_VCE_ENGINE] / DEFAULT_MM_ENGINE_FPS;
	pf2vf_msg->vf2pf_update_interval_ms = 2000;

	if (!adapt->ffbm.enabled) {
		pf2vf_msg->bp_block_offset_low =
			KBYTES_TO_BYTES(AMD_SRIOV_MSG_BAD_PAGE_OFFSET_KB) & 0xFFFFFFFF;
		pf2vf_msg->bp_block_offset_high =
			(KBYTES_TO_BYTES(AMD_SRIOV_MSG_BAD_PAGE_OFFSET_KB) >> 32) & 0xFFFFFFFF;
		pf2vf_msg->bp_block_size = entry->bp_block_size;
	}
	pf2vf_msg->pcie_atomic_ops_support_flags = adapt->pcie_atomic_ops_support_flags;

	//100% is 65535
	if (adapt->num_vf == 1)
		pf2vf_msg->gpu_capacity = GPU_CAPACITY_MAX;
	else if (adapt->num_vf > 1) {
		if (adapt->opt.pf_option == NULL)
			pf2vf_msg->gpu_capacity = ((adapt->array_vf[idx_vf].fb_size * GPU_CAPACITY_MAX) / (adapt->gpuiov.total_fb_usable - AMDGV_MIN_PF_SIZE));
		else
			pf2vf_msg->gpu_capacity = ((adapt->array_vf[idx_vf].fb_size * GPU_CAPACITY_MAX) / (adapt->gpuiov.total_fb_usable - adapt->opt.pf_option->fb_size));
	}

	if (adapt->flags & AMDGV_FLAG_MES_INFO_DUMP_ENABLE) {
		pf2vf_msg->feature_flags.flags.mes_info_dump_enable = 1;
		adapt->array_vf[idx_vf].mes_info_dump_enabled = true;
	} else {
		pf2vf_msg->feature_flags.flags.mes_info_dump_enable = 0;
		adapt->array_vf[idx_vf].mes_info_dump_enabled = false;
	}

	amdgv_vfmgr_get_adapt_uuid(adapt, &pf2vf_msg->uuid);

	oss_memcpy(&pf2vf_msg->uuid_info, &entry->uuid_info,
		   sizeof(struct amd_sriov_msg_uuid_info));

	amdgv_mmsch_update_pf2vf_msg(adapt, idx_vf);
	amdgv_vfmgr_update_vf2vf_ras_caps(adapt, pf2vf_msg, idx_vf);

	pf2vf_msg->fcn_idx = idx_vf;
	pf2vf_msg->bdf_on_host = adapt->array_vf[idx_vf].bdf;

	pf2vf_msg->checksum = amd_sriov_msg_checksum(
		pf2vf_msg, sizeof(struct amd_sriov_msg_pf2vf_info), 0, 0);

	ret = amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf,
					KBYTES_TO_BYTES(AMD_SRIOV_MSG_PF2VF_OFFSET_KB),
					pf2vf_msg, sizeof(struct amd_sriov_msg_pf2vf_info));

	if (ret == 0) {
		AMDGV_DEBUG("PF2VF message succeed with feature flags(0x%x).\n",
			   pf2vf_msg->feature_flags.all);
		adapt->array_vf[idx_vf].vram_lost = false;
	}

	oss_memset(adapt->pf2vf_msg, 0, sizeof(struct amd_sriov_msg_pf2vf_info));

	return ret;
}

int amdgv_vfmgr_retrieve_vf2pf_message(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       struct amd_sriov_msg_vf2pf_info *output)
{
	int ret;
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	struct amd_sriov_msg_vf2pf_info *vf2pf_msg = NULL;

	char *vf2pf_drv_ver = 0;
	uint32_t vf2pf_size;
	uint32_t vf2pf_version;
	uint32_t checksum;

	/* Retry copying vf2pf_msg in case checksum has not been set by VF yet */
	int retry_cnt = 5;

	uint32_t msg_key = 0; // TODO: should read from register

	if (!entry->configured)
		return AMDGV_FAILURE;

	if (output == NULL)
		return AMDGV_FAILURE;
	else
		vf2pf_msg = output;

retry:
	ret = amdgv_vfmgr_copy_from_vf_fb(adapt, idx_vf,
			KBYTES_TO_BYTES(AMD_SRIOV_MSG_VF2PF_OFFSET_KB),
			vf2pf_msg, sizeof(struct amd_sriov_msg_vf2pf_info));

	if (ret == 0) {
		vf2pf_version = vf2pf_msg->header.version;
		vf2pf_size = vf2pf_msg->header.size;
		vf2pf_drv_ver = vf2pf_msg->driver_version;
		vf2pf_drv_ver[63] = '\0';
		checksum = vf2pf_msg->checksum;

		if (retry_cnt > 0 &&
		    amd_sriov_msg_checksum(vf2pf_msg, vf2pf_size, msg_key, checksum) != checksum) {
			AMDGV_WARN("VF2PF message checksum mismatch, retry remaining %d\n", retry_cnt);
			retry_cnt--;
			goto retry;
		}

		if (vf2pf_size > KBYTES_TO_BYTES(AMD_SRIOV_MSG_SIZE_KB) ||
		    amd_sriov_msg_checksum(vf2pf_msg, vf2pf_size, msg_key, checksum) != checksum) {
			AMDGV_ERROR("Untrustworthy guest information in VF2PF msg. Drop request\n");
			return AMDGV_FAILURE;
		}
	}

	return ret;
}

int amdgv_vfmgr_print_vf2pf_message(struct amdgv_adapter *adapt, uint32_t idx_vf,
				struct amd_sriov_msg_vf2pf_info *vf2pf_msg)
{
	AMDGV_INFO("VF%d Guest Driver Message Header, Version=%d, Size=%d\n",
		idx_vf, vf2pf_msg->header.version, vf2pf_msg->header.size);
	AMDGV_INFO("Guest Driver Version=%s\n", vf2pf_msg->driver_version);

	return 0;
}

/* checksum function shared between all host and guest */
unsigned int amd_sriov_msg_checksum(void *obj, unsigned long obj_size, unsigned int key,
				    unsigned int checksum)
{
	uint32_t ret = key;
	uint32_t i = 0;
	uint8_t *pos;

	pos = (char *)obj;

	/* calculate checksum */
	for (i = 0; i < obj_size; ++i)
		ret += *(pos + i);
	/* minus the checksum itself */
	pos = (char *)&checksum;
	for (i = 0; i < sizeof(checksum); ++i)
		ret -= *(pos + i);

	return ret;
}

int amdgv_vfmgr_get_fb_layout(struct amdgv_adapter *adapt, struct amdgv_fb_layout *layout)
{
	uint32_t pf_fb_size;
	uint32_t total_fb_avail, total_usable_fb;

	if (layout == NULL)
		return AMDGV_FAILURE;

	amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_fb_avail);
	layout->total_fb_avail = total_fb_avail;

	pf_fb_size = adapt->array_vf[AMDGV_PF_IDX].fb_size;
	layout->vf_usable_fb_offset = pf_fb_size;

	amdgv_gpuiov_get_usable_fb_size(adapt, &total_usable_fb);
	layout->total_vf_usable_fb = total_usable_fb - pf_fb_size;

	layout->vres_offset = total_usable_fb;
	layout->vres_size = total_fb_avail - total_usable_fb;

	layout->system_fb_mc_address = adapt->mc_fb_loc_addr;

	layout->fb_mc_address = layout->system_fb_mc_address;
	if (adapt->xgmi.phy_nodes_num > 1)
		layout->fb_mc_address += adapt->xgmi.phy_node_id *
							adapt->xgmi.node_segment_size;
	layout->fb_physical_address = adapt->fb_pa;

	return 0;
}

int amdgv_vfmgr_get_adapt_uuid(struct amdgv_adapter *adapt, uint64_t *uuid)
{
	if (adapt->serial && uuid) {
		*uuid = oss_create_hash_64(adapt->serial ^ adapt->bdf, 64);
		if (*uuid == -1)
			*uuid = adapt->serial;
		return 0;
	}

	return AMDGV_FAILURE;
}

int amdgv_host_upload_fw_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret;

	ret = amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf, adapt->mecfw_offset, adapt->mecfw_image,
					adapt->mecfw_size);
	if (ret) {
		AMDGV_INFO("Upload mec failed\n");
		return AMDGV_FAILURE;
	}
	ret = amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf, adapt->uvdfw_offset, adapt->uvdfw_image,
					adapt->uvdfw_size);
	if (ret) {
		AMDGV_INFO("Upload uvd failed\n");
		return AMDGV_FAILURE;
	}
	ret = amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf, adapt->vcefw_offset, adapt->vcefw_image,
					adapt->vcefw_size);
	if (ret) {
		AMDGV_INFO("Upload vce failed\n");
		return AMDGV_FAILURE;
	}
	return ret;
}

/*
	Helper for MMIO read for the requests that are intercepted and sent to amdgpuv
*/

int amdgv_vfmgr_read_mmio(struct amdgv_adapter *adapt, uint32_t idx_vf, void *buffer,
			  uint32_t offset, uint32_t length)
{
	int ret = 0;
	struct amdgv_vf_device *vf = &adapt->array_vf[idx_vf];

	if (!vf->configured) {
		AMDGV_ERROR("VF:%d not configured. Drop request\n", idx_vf);
		return ret;
	}

	if ((!vf->res_mapped) || (vf->res.mmio == NULL)) {
		AMDGV_ERROR("VF:%d Resource not mapped. Drop request\n", idx_vf);
		return ret;
	}

	switch (length) {
	case 1:
		*((uint8_t *)buffer) = oss_mm_read8(((uint8_t *)(vf->res.mmio)) + offset);
		AMDGV_DEBUG("[VF:%d] Offset:0x%02x Length:%d value:0x%x\n", idx_vf, offset,
			    length, *((uint8_t *)buffer));
		ret = length;
		break;

	case 2:
		*((uint16_t *)buffer) = oss_mm_read16(((uint8_t *)(vf->res.mmio)) + offset);
		AMDGV_DEBUG("[VF:%d] Offset:0x%02x Length:%d value:0x%x\n", idx_vf, offset,
			    length, *((uint16_t *)buffer));
		ret = length;
		break;

	case 4:
		*((uint32_t *)buffer) = oss_mm_read32(((uint8_t *)(vf->res.mmio)) + offset);
		AMDGV_DEBUG("[VF:%d] Offset:0x%02x Length:%d value:0x%x\n", idx_vf, offset,
			    length, *((uint32_t *)buffer));
		ret = length;
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}

/*
	Helper for MMIO write for the requests that are intercepted and sent to amdgpuv.
*/

int amdgv_vfmgr_write_mmio(struct amdgv_adapter *adapt, uint32_t idx_vf, void *buffer,
			   uint32_t offset, uint32_t length)
{
	int ret = 0;
	struct amdgv_vf_device *vf = &adapt->array_vf[idx_vf];
	int val;

	if (!vf->configured) {
		AMDGV_ERROR("VF:%d not configured. Drop request\n", idx_vf);
		return ret;
	}

	if ((!vf->res_mapped) || (vf->res.mmio == NULL)) {
		AMDGV_ERROR("VF:%d Resource not mapped. Drop request\n", idx_vf);
		return ret;
	}

	if (offset == mmMMHUB_VM_FB_LOCATION_TOP) {
		val = *((uint32_t *)buffer);
		if (is_full_access_vf(idx_vf) &&
		    (val == 0x7B || val == 0xF9 || val == 0x1F4 || val == 0x3EB)) {
			AMDGV_ERROR("Found Linux Driver load, rel Full access\n");
			amdgv_sched_queue_remove_vf(adapt, idx_vf);
		}
		ret = length;
		return ret;
	}

	switch (length) {
	case 1:
		oss_mm_write8(((uint8_t *)(vf->res.mmio)) + offset, (*(uint8_t *)buffer));
		AMDGV_DEBUG("[VF:%d] Offset:0x%02x Length:%d value:0x%x\n", idx_vf, offset,
			    length, *((uint8_t *)buffer));
		ret = length;
		break;

	case 2:
		oss_mm_write16(((uint8_t *)(vf->res.mmio)) + offset, (*(uint16_t *)buffer));
		AMDGV_DEBUG("[VF:%d] Offset:0x%02x Length:%d value:0x%x\n", idx_vf, offset,
			    length, *((uint16_t *)buffer));
		ret = length;
		break;

	case 4:
		oss_mm_write32(((uint8_t *)(vf->res.mmio)) + offset, (*(uint32_t *)buffer));
		AMDGV_DEBUG("[VF:%d] Offset:0x%02x Length:%d value:0x%x\n", idx_vf, offset,
			    length, *((uint32_t *)buffer));
		ret = length;
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}

void amdgv_vfmgr_get_xgmi_info(struct amdgv_adapter *adapt, union amdgv_dev_info *info)
{
	struct amdgv_hive_info *hive;

	if (adapt->xgmi.phy_nodes_num > 1) {
		hive = amdgv_get_xgmi_hive(adapt);
		if (!hive) {
			AMDGV_ERROR("XGMI: node 0x%llx, can not match hive "
				    "0x%llx in the hive list.\n",
				    adapt->xgmi.node_id, adapt->xgmi.hive_id);
			return;
		}



		info->xgmi_info.node_id = adapt->xgmi.node_id;
		info->xgmi_info.hive_id = hive->hive_id;
		info->xgmi_info.hive_index = hive->hive_index;
		info->xgmi_info.number_adapters = hive->number_adapters;
		info->xgmi_info.phy_node_id = adapt->xgmi.phy_node_id;
	}
}

enum amdgv_live_info_status amdgv_vfmgr_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_vf *vf_info)
{
	uint32_t idx_live_data, idx_vf;

	for (idx_live_data = 0; idx_live_data < adapt->num_vf + 1; idx_live_data++) {
		if (idx_live_data >= AMDGV_MAX_VF_LIVE) {
			AMDGV_ERROR("VF MGR export live data error, slot# %u, %u live update slots\n", idx_live_data, AMDGV_MAX_VF_LIVE);
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}

		idx_vf = idx_live_data;
		if (idx_live_data == adapt->num_vf)
			idx_vf = AMDGV_PF_IDX;

		vf_info[idx_live_data].configured = adapt->array_vf[idx_vf].configured;
		vf_info[idx_live_data].mm_bandwidth_hevc =
			adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_HEVC_ENGINE];
		vf_info[idx_live_data].mm_bandwidth_vce =
			adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_VCE_ENGINE];
		vf_info[idx_live_data].mm_bandwidth_hevc1 =
			adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_HEVC1_ENGINE];
		vf_info[idx_live_data].mm_bandwidth_vcn =
			adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_VCN_ENGINE];
		vf_info[idx_live_data].mm_bandwidth_jpeg =
			adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_JPEG_ENGINE];
		vf_info[idx_live_data].time_slice_gfx =
			adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_GFX];
		vf_info[idx_live_data].time_slice_uvd =
			adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_UVD];
		vf_info[idx_live_data].time_slice_vce =
			adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_VCE];
		vf_info[idx_live_data].time_slice_uvd1 =
			adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_UVD1];
		vf_info[idx_live_data].time_slice_vcn =
			adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_VCN];
		vf_info[idx_live_data].time_slice_vcn1 =
			adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_VCN1];
		vf_info[idx_live_data].waiting_ack = false;
		vf_info[idx_live_data].ready_to_reset = adapt->array_vf[idx_vf].ready_to_reset;
		vf_info[idx_live_data].vf_status = adapt->array_vf[idx_vf].vf_status;
		vf_info[idx_live_data].gpu_init_data_ready =
			adapt->array_vf[idx_vf].gpu_init_data_ready;

		/* We now need to save each hw_sched info individually as some asics
		 * may have multiple hw schedulers of same type
		 */
		oss_memcpy(&vf_info[idx_live_data].cur_vf_state, &adapt->sched.array_vf[idx_vf].cur_vf_state, sizeof(vf_info[idx_live_data].cur_vf_state));

		vf_info[idx_live_data].retired_page = adapt->array_vf[idx_vf].retired_page;
		vf_info[idx_live_data].bp_block_size = adapt->array_vf[idx_vf].bp_block_size;

		vf_info[idx_live_data].vram_lost = adapt->array_vf[idx_vf].vram_lost;
		vf_info[idx_live_data].auto_run = adapt->array_vf[idx_vf].auto_run;

		vf_info[idx_live_data].state = adapt->sched.array_vf[idx_vf].state;
		vf_info[idx_live_data].reset_notify_vf_pending =
			adapt->array_vf[idx_vf].reset_notify_vf_pending;

		if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE)) {
			if (idx_vf != AMDGV_PF_IDX && adapt->array_vf[idx_vf].res_mapped)
				oss_unmap_vf_dev_res(adapt->array_vf[idx_vf].dev, &adapt->array_vf[idx_vf].res);
		}

		if (idx_vf != AMDGV_PF_IDX) {
			oss_put_dev(adapt->array_vf[idx_vf].dev);
			adapt->array_vf[idx_vf].dev = AMDGV_INVALID_HANDLE;
		}
	}

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_vfmgr_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_vf *vf_info)
{
	uint32_t idx_vf, idx_live_data;
	struct amdgv_vf_device *entry;

	for (idx_live_data = 0; idx_live_data < adapt->num_vf + 1; idx_live_data++) {
		if (idx_live_data >= AMDGV_MAX_VF_LIVE) {
			AMDGV_ERROR("VF MGR import live data error, slot# %u, %u live update slots\n", idx_live_data, AMDGV_MAX_VF_LIVE);
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}

		idx_vf = idx_live_data;
		if (idx_live_data == adapt->num_vf)
			idx_vf = AMDGV_PF_IDX;
		if (idx_vf == AMDGV_PF_IDX) {
			adapt->array_vf[idx_vf].dev = adapt->dev;
			adapt->array_vf[idx_vf].idx_vf = AMDGV_PF_IDX;
		} else
			adapt->array_vf[idx_vf].dev = oss_get_dev_from_bdf(adapt->array_vf[idx_vf].bdf);

		adapt->array_vf[idx_vf].configured = vf_info[idx_live_data].configured;
		adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_HEVC_ENGINE] =
			vf_info[idx_live_data].mm_bandwidth_hevc;
		adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_VCE_ENGINE] =
			vf_info[idx_live_data].mm_bandwidth_vce;
		adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_HEVC1_ENGINE] =
			vf_info[idx_live_data].mm_bandwidth_hevc1;
		adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_VCN_ENGINE] =
			vf_info[idx_live_data].mm_bandwidth_vcn;
		adapt->array_vf[idx_vf].mm_bandwidth[AMDGV_JPEG_ENGINE] =
			vf_info[idx_live_data].mm_bandwidth_jpeg;
		adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_GFX] =
			vf_info[idx_live_data].time_slice_gfx;
		adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_UVD] =
			vf_info[idx_live_data].time_slice_uvd;
		adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_VCE] =
			vf_info[idx_live_data].time_slice_vce;
		adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_UVD1] =
			vf_info[idx_live_data].time_slice_uvd1;
		adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_VCN] =
			vf_info[idx_live_data].time_slice_vcn;
		adapt->array_vf[idx_vf].time_slice[AMDGV_SCHED_BLOCK_VCN1] =
			vf_info[idx_live_data].time_slice_vcn1;
		adapt->array_vf[idx_vf].ready_to_reset = vf_info[idx_live_data].ready_to_reset;
		adapt->array_vf[idx_vf].vf_status = vf_info[idx_live_data].vf_status;
		adapt->array_vf[idx_vf].gpu_init_data_ready =
			vf_info[idx_live_data].gpu_init_data_ready;

		/* We now need to save each hw_sched info individually as some asics
		* may have multiple hw schedulers of same type
		*/
		oss_memcpy(&adapt->sched.array_vf[idx_vf].cur_vf_state, &vf_info[idx_live_data].cur_vf_state, sizeof(adapt->sched.array_vf[idx_vf].cur_vf_state));

		adapt->array_vf[idx_vf].retired_page = vf_info[idx_live_data].retired_page;
		adapt->array_vf[idx_vf].bp_block_size = vf_info[idx_live_data].bp_block_size;

		adapt->array_vf[idx_vf].vram_lost = vf_info[idx_live_data].vram_lost;
		adapt->array_vf[idx_vf].auto_run = vf_info[idx_live_data].auto_run;

		adapt->sched.array_vf[idx_vf].state = vf_info[idx_live_data].state;
		adapt->array_vf[idx_vf].reset_notify_vf_pending =
			vf_info[idx_live_data].reset_notify_vf_pending;

		amdgv_sched_update_time_slice(adapt, AMDGV_SCHED_BLOCK_ALL, idx_vf);

		if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE)) {
			if (idx_vf != AMDGV_PF_IDX) {
				entry = &adapt->array_vf[idx_vf];
				entry->res_mapped = !oss_map_vf_dev_res(entry->dev, &entry->res);
			}
		}

	}

	amdgv_vfmgr_init_pf_config(adapt);
	amdgv_vfmgr_import_vfs_fb(adapt);

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_vfmgr_export_live_data_extend(struct amdgv_adapter *adapt, struct amdgv_live_info_vf_extend *vf_extend)
{
	uint32_t idx_live_data, idx_vf;

	for (idx_live_data = 0; idx_live_data < adapt->num_vf + 1; idx_live_data++) {
		if (idx_live_data >= AMDGV_MAX_VF_LIVE) {
			AMDGV_ERROR("VF MGR export live data error, slot# %u, %u live update slots\n", idx_live_data, AMDGV_MAX_VF_LIVE);
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}

		idx_vf = idx_live_data;
		if (idx_live_data == adapt->num_vf)
			idx_vf = AMDGV_PF_IDX;

		vf_extend[idx_live_data].in_full_access =
					adapt->sched.array_vf[idx_vf].in_full_access;
		vf_extend[idx_live_data].used_time_full_access =
					adapt->sched.array_vf[idx_vf].used_time_full_access;
		vf_extend[idx_live_data].event_id_full_access =
					adapt->sched.array_vf[idx_vf].event_id_full_access;
	}

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_vfmgr_import_live_data_extend(struct amdgv_adapter *adapt, struct amdgv_live_info_vf_extend *vf_extend)
{
	uint32_t idx_live_data, idx_vf;

	for (idx_live_data = 0; idx_live_data < adapt->num_vf + 1; idx_live_data++) {
		if (idx_live_data >= AMDGV_MAX_VF_LIVE) {
			AMDGV_ERROR("VF MGR import live data error, slot# %u, %u live update slots\n", idx_live_data, AMDGV_MAX_VF_LIVE);
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}

		idx_vf = idx_live_data;
		if (idx_live_data == adapt->num_vf)
			idx_vf = AMDGV_PF_IDX;

		adapt->sched.array_vf[idx_vf].in_full_access =
					vf_extend[idx_live_data].in_full_access;
		adapt->sched.array_vf[idx_vf].used_time_full_access =
					vf_extend[idx_live_data].used_time_full_access;
		adapt->sched.array_vf[idx_vf].event_id_full_access =
					vf_extend[idx_live_data].event_id_full_access;

		if (adapt->sched.array_vf[idx_vf].in_full_access)
			adapt->sched.skip_full_access_timeout_check = true;
	}

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

