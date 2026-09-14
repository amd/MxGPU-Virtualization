/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "amdgv_psp_gfx_if.h"
#include "amdgv_vfmgr.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

int amdgv_misc_get_hdp_nonsurface_base(struct amdgv_adapter *adapt, uint64_t *hdp_mc_addr)
{
	if (adapt->misc.get_hdp_nonsurface_base)
		return adapt->misc.get_hdp_nonsurface_base(adapt, hdp_mc_addr);
	*hdp_mc_addr = adapt->mc_fb_loc_addr;
	return AMDGV_FAILURE;
}

int amdgv_misc_set_hdp_nonsurface_base(struct amdgv_adapter *adapt, uint64_t hdp_mc_addr)
{
	if (adapt->misc.set_hdp_nonsurface_base)
		return adapt->misc.set_hdp_nonsurface_base(adapt, hdp_mc_addr);
	return AMDGV_FAILURE;
}

#define CLEAR_FB_MEMSET_LIMIT  MBYTES_TO_BYTES(4096)

/*
 * Fill [fb_offset, fb_offset + fb_size) with pattern using the DMA engine.
 * Returns the bytes filled, short of fb_size when there is no engine or it
 * failed, which is what the device reports once an uncorrectable error has put
 * it in a fatal state.
 */
static uint64_t amdgv_misc_dma_fill_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
					  uint64_t fb_offset, uint64_t fb_size, uint8_t pattern)
{
	uint64_t filled_size = 0;
	uint64_t src;
	uint64_t dst;
	uint32_t i;

	if (!adapt->misc.dma_copy)
		return 0;

	src = (uint64_t)pattern;
	for (i = 0; i < 8; i++)
		src = (src << 8) | pattern;
	/*
	 * use MCAddr of the start of the PF Framebuffer
	 * The VF Framebuffer will start at "PF_MCAddr + offset"
	 * in the PF address space.
	 */
	dst = adapt->mc_fb_loc_addr + fb_offset;
	if (adapt->xgmi.phy_nodes_num > 1)
		dst += adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;

	adapt->misc.dma_copy(adapt, idx_vf, true, src, dst, fb_size, &filled_size);

	if (filled_size)
		AMDGV_DEBUG("DMA filled 0x%llx bytes\n", filled_size);

	return filled_size;
}

/*
 * Fill the same range with the CPU, for whatever the DMA engine left behind.
 * Slow enough that the caller decides whether the range is worth clearing at
 * all before calling this.
 */
static int amdgv_misc_cpu_fill_vf_fb(struct amdgv_adapter *adapt, uint64_t fb_offset,
				     uint64_t fb_size, uint8_t pattern)
{
	uint64_t filled_size = 0;

	if (adapt->fb_size <= MBYTES_TO_BYTES(256)) {
		/* has small PF bar */
		uint32_t chunk_size;
		uint64_t hdp_mc_base;

		if (adapt->misc.get_hdp_nonsurface_base == NULL) {
			AMDGV_ERROR("get_hdp_nonsurface_base not set, cannot clear VF FB\n");
			return AMDGV_FAILURE;
		}
		if (adapt->misc.set_hdp_nonsurface_base == NULL) {
			AMDGV_ERROR("set_hdp_nonsurface_base not set, cannot clear VF FB\n");
			return AMDGV_FAILURE;
		}

		/* save HDP_NONSURFACE_BASE */
		amdgv_misc_get_hdp_nonsurface_base(adapt, &hdp_mc_base);

		/*
		 * HDP base register require multiples of 256B
		 * => make sure chunk_size is aligned to 256B
		 * => make sure "fb_offset" is aligned to 256B
		 */
		chunk_size = rounddown(adapt->fb_size, 256);
		fb_offset = roundup(fb_offset, 256);

		while (filled_size < fb_size) {
			/* Make sure we don't go past end of region */
			if ((filled_size + chunk_size) > fb_size)
				chunk_size = fb_size - filled_size;

			/*
			 * Move HDP_NONSURFACE_BASE
			 * The HDP base register is in multiples
			 * of 256B
			 */
			amdgv_misc_set_hdp_nonsurface_base(
				adapt, hdp_mc_base + TO_256BYTES(fb_offset));

			oss_memset((uint8_t *)adapt->fb, pattern, chunk_size);

			fb_offset = fb_offset + chunk_size;
			filled_size = filled_size + chunk_size;

			/*
			 * This is a long operation. Yield to allow
			 * kernel to schedule other tasks
			 */
			oss_yield();
		}

		/* Restore HDP_NONSURFACE_BASE */
		amdgv_misc_set_hdp_nonsurface_base(adapt, hdp_mc_base);
	} else {
		/* has large PF bar */
		uint32_t chunk_size;

		chunk_size = MBYTES_TO_BYTES(256); /* do 256MB chunk */
		while (filled_size < fb_size) {
			/* Make sure we don't go past end of region */
			if ((filled_size + chunk_size) > fb_size)
				chunk_size = fb_size - filled_size;

			oss_memset((uint8_t *)adapt->fb + fb_offset, pattern, chunk_size);

			fb_offset = fb_offset + chunk_size;
			filled_size = filled_size + chunk_size;

			/*
			 * This is a long operation. Yield to allow
			 * kernel to schedule other tasks
			 */
			oss_yield();
		}
	}

	return 0;
}

/*
 * Whether the DMA engine left too much of the range behind for a CPU memset
 * to be worth the time it would take. Warns when giving up.
 */
static bool amdgv_misc_should_give_up_clear(struct amdgv_adapter *adapt, uint32_t idx_vf,
					     uint64_t filled_size, uint64_t expected_size)
{
	if ((expected_size - filled_size) <= CLEAR_FB_MEMSET_LIMIT)
		return false;

	AMDGV_WARN("%s clear_fb gave up: DMA filled 0x%llx of 0x%llx, the remaining 0x%llx bytes are over the CPU memset limit 0x%llx\n",
		   amdgv_idx_to_str(idx_vf), filled_size, expected_size,
		   expected_size - filled_size, (uint64_t)CLEAR_FB_MEMSET_LIMIT);
	return true;
}

/*
 * Clear a VF's FB across its FFBM PTE blocks, trying the DMA engine on each
 * block and falling back to the CPU for whatever it leaves behind.
 *
 * Adds the bytes filled to *filled_size. Returns AMDGV_FAILURE if a block's
 * remaining CPU work would be over CLEAR_FB_MEMSET_LIMIT (see
 * amdgv_misc_should_give_up_clear) or a CPU fill fails, else 0.
 */
static int amdgv_misc_clear_vf_fb_ffbm(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       struct amdgv_list_head *gpa_list, uint8_t pattern,
				       uint64_t *filled_size)
{
	struct amdgv_ffbm_pte_block *pteb;
	uint64_t expected_size = 0;
	int ret = 0;

	FFBM_LOCK_LIST;

	amdgv_list_for_each_entry(pteb, gpa_list, struct amdgv_ffbm_pte_block, gpa_list_node) {
		if (pteb->type != AMDGV_FFBM_MEM_TYPE_TMR)
			expected_size += pteb->size;
	}

	amdgv_list_for_each_entry(pteb, gpa_list, struct amdgv_ffbm_pte_block, gpa_list_node) {
		uint64_t dma_filled;

		if (pteb->type == AMDGV_FFBM_MEM_TYPE_TMR)
			continue;

		dma_filled = amdgv_misc_dma_fill_vf_fb(adapt, idx_vf, pteb->spa, pteb->size,
						       pattern);
		*filled_size += dma_filled;
		if (dma_filled == pteb->size)
			continue;

		if (amdgv_misc_should_give_up_clear(adapt, idx_vf, *filled_size,
						     expected_size)) {
			ret = AMDGV_FAILURE;
			break;
		}

		ret = amdgv_misc_cpu_fill_vf_fb(adapt, pteb->spa + dma_filled,
						pteb->size - dma_filled, pattern);
		if (ret)
			break;

		*filled_size += pteb->size - dma_filled;
	}

	FFBM_UNLOCK_LIST;

	return (ret || *filled_size < expected_size) ? AMDGV_FAILURE : 0;
}

/*
 * Clear a VF's FB when it is one contiguous range (FFBM disabled). Adds the
 * bytes filled to *filled_size. Returns AMDGV_FAILURE if the DMA shortfall
 * is over CLEAR_FB_MEMSET_LIMIT or the CPU fill fails, else 0.
 */
static int amdgv_misc_clear_vf_fb_range(struct amdgv_adapter *adapt, uint32_t idx_vf,
					uint64_t fb_offset, uint64_t fb_size, uint8_t pattern,
					uint64_t *filled_size)
{
	*filled_size = amdgv_misc_dma_fill_vf_fb(adapt, idx_vf, fb_offset, fb_size, pattern);
	if (*filled_size == fb_size)
		return 0;

	if (amdgv_misc_should_give_up_clear(adapt, idx_vf, *filled_size, fb_size))
		return AMDGV_FAILURE;

	if (amdgv_misc_cpu_fill_vf_fb(adapt, fb_offset + *filled_size,
				     fb_size - *filled_size, pattern))
		return AMDGV_FAILURE;

	*filled_size = fb_size;
	return 0;
}

int amdgv_misc_clear_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint8_t pattern)
{
	struct amdgv_vf_device *entry;
	uint64_t fb_offset;
	uint64_t fb_offset_end;
	uint64_t fb_size;
	uint64_t filled_size = 0;
	int ret;

	/* clear FB memory region for VFs (not PF) */
	if (idx_vf == AMDGV_PF_IDX || idx_vf >= adapt->num_vf)
		return 0;

	/* if "clear_vf_fb" is disabled, bypass clearing the VF FB */
	if (!(adapt->flags & AMDGV_FLAG_ENABLE_CLEAR_VF_FB))
		return 0;

	entry = &adapt->array_vf[idx_vf];
	if (!entry->configured) {
		AMDGV_ERROR("Cannot clean non configured VF%d FB", idx_vf);
		return AMDGV_FAILURE;
	}

	entry->gpu_init_data_ready = false;

	fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
	fb_size = MBYTES_TO_BYTES(entry->fb_size);

	if (adapt->umc.is_pmfw_managed_eeprom ||
		GET_VF_TABLE_OFFSET_BY_ID(adapt, idx_vf, IPD) == 0) {
		/* Clear entire VF FB */
		fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
		fb_offset_end = fb_offset + fb_size;
	} else {
		/* do not clear IP Discovery region, to support VM reload */
		fb_offset_end = fb_offset + GET_VF_TABLE_OFFSET_BY_ID(adapt, idx_vf, IPD);
		fb_size = fb_offset_end - fb_offset;
	}
	AMDGV_DEBUG("%s fb_offset=0x%llx fb_offset_end=0x%llx fb_size=0x%llx\n",
		   amdgv_idx_to_str(idx_vf), fb_offset, fb_offset_end, fb_size);

	if (adapt->ffbm.enabled)
		ret = amdgv_misc_clear_vf_fb_ffbm(adapt, idx_vf, &entry->gpa_list, pattern,
						  &filled_size);
	else
		ret = amdgv_misc_clear_vf_fb_range(adapt, idx_vf, fb_offset, fb_size, pattern,
						   &filled_size);

	AMDGV_DEBUG("%s, fb_offset=0x%llx fb_size_cleared=0x%llx pattern[%u]\n",
		   amdgv_idx_to_str(idx_vf), fb_offset, filled_size, pattern);

	return ret;
}

int amdgv_misc_load_dfc(struct amdgv_adapter *adapt)
{
	/*
	 * Note: This function should only be called during req gpu init.
	 * World switch must be stopped prior to the call.
	 */
	enum amdgv_firmware_id fw_id;
	int ret = 0;

	if (oss_detect_fw(adapt->dev, AMDGV_FIRMWARE_ID__DFC_FW, adapt->asic_type)) {
		AMDGV_DEBUG("Patched DFC not found.\n");
	}

	/* switch to PF for GFX block */
	if (amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX) !=
	    0)
		return AMDGV_FAILURE;

	fw_id = AMDGV_FIRMWARE_ID__DFC_FW;

	if (adapt->ucode.load) {
		if (adapt->ucode.load(adapt, &fw_id, 1))
			ret = PSP_STATUS__ERROR_GENERIC;
	} else {
		ret = AMDGV_FAILURE;
	}

	if (amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX) != 0)
		return AMDGV_FAILURE;

	return ret;
}

void amdgv_misc_reprogram_golden_settings(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (adapt->misc.reprogram_golden_settings)
		adapt->misc.reprogram_golden_settings(adapt, (void *)(&idx_vf));
}

uint64_t amdgv_misc_get_memsize(struct amdgv_adapter *adapt)
{
	if (adapt->misc.get_memsize)
		return adapt->misc.get_memsize(adapt);
	else
		return 0;
}

void amdgv_misc_hdp_flush(struct amdgv_adapter *adapt)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->hdp_flush)
		adapt->nbio.funcs->hdp_flush(adapt);
}

int amdgv_misc_get_agp_cpu_base(struct amdgv_adapter *adapt, void **data)
{
	if (adapt->sys_mem_info.va_ptr) {
		*data = adapt->sys_mem_info.va_ptr;
		return 0;
	}
	AMDGV_WARN("Failed to retrieve AGP cpu base address.\n");
	return AMDGV_FAILURE;
}

int amdgv_misc_dma_copy(struct amdgv_adapter *adapt, int idx_vf,
			uint64_t src, uint64_t size, uint64_t dst)
{
	uint64_t size_copied = 0;

	if (!adapt->array_vf[idx_vf].configured)
		return AMDGV_FAILURE;

	if (!adapt->sys_mem_info.handle) {
		AMDGV_ERROR("AGP memory not ready, exit.\n");
		return AMDGV_FAILURE;
	}

	if (adapt->misc.dma_copy) {
		if (adapt->misc.dma_copy(adapt, idx_vf, false, src, dst, size, &size_copied))
			return AMDGV_FAILURE;
		else {
			AMDGV_DEBUG("0x%lx bytes copied with GPU\n", size_copied);
			return 0;
		}
	} else
		return AMDGV_FAILURE;

	return 0;
}
