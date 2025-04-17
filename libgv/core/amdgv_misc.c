/*
 * Copyright (c) 2018-2023 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amdgv_sched_internal.h"
#include "amdgv_psp_gfx_if.h"

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

static uint64_t amdgv_misc_do_clear_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
					  uint64_t fb_offset, uint64_t fb_size, uint8_t pattern)
{
	uint64_t filled_size;
	/* attempt to use CP_DMA fill_mode to clear the entire VF FB region
	 *  NOTE: if CP_DMA fails or "cp_dma_copy" function not provided,
	 *        will fall-back to use CPU memset to clear VF FB
	 *     => regardless, this guarantees that VF FB is always cleared!
	 */
	filled_size = 0;

	if (adapt->misc.dma_copy) {
		uint64_t src;
		uint64_t dst;
		uint32_t i;

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
		fb_offset = fb_offset + filled_size;
	}

	if (filled_size)
		AMDGV_DEBUG("DMA filled 0x%llx bytes\n", filled_size);

	if (filled_size < fb_size) {
		if (adapt->fb_size <= MBYTES_TO_BYTES(256)) {
			/* has small PF bar */
			uint32_t chunk_size;
			uint64_t hdp_mc_base;

			if (adapt->misc.get_hdp_nonsurface_base == NULL)
				return AMDGV_FAILURE;
			if (adapt->misc.set_hdp_nonsurface_base == NULL)
				return AMDGV_FAILURE;

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

				oss_memset((uint8_t *)adapt->fb + fb_offset, pattern,
					   chunk_size);

				fb_offset = fb_offset + chunk_size;
				filled_size = filled_size + chunk_size;

				/*
				 * This is a long operation. Yield to allow
				 * kernel to schedule other tasks
				 */
				oss_yield();
			}
		}
	}
	return filled_size;
}

int amdgv_misc_clear_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint8_t pattern)
{
	struct amdgv_vf_device *entry;
	uint64_t fb_offset;
	uint64_t fb_offset_end;
	uint64_t fb_size;
	struct amdgv_ffbm_pte_block *pteb;
	uint64_t filled_size = 0;

	/* clear FB memory region for VFs (not PF) */
	if (idx_vf == AMDGV_PF_IDX || idx_vf >= adapt->num_vf)
		return 0;

	/* if "clear_vf_fb" is disabled, bypass clearing the VF FB */
	if (!(adapt->flags & AMDGV_FLAG_ENABLE_CLEAR_VF_FB))
		return 0;

	entry = &adapt->array_vf[idx_vf];
	if (!entry->configured)
		return AMDGV_FAILURE;

	fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
	fb_size = MBYTES_TO_BYTES(entry->fb_size);
	/* do not clear IP Discovery region, to support VM reload */
	fb_offset_end = (fb_offset + fb_size) - AMDGV_IP_DISCOVERY_OFFSET;
	fb_size = fb_offset_end - fb_offset;
	AMDGV_DEBUG("%s fb_offset=0x%llx fb_offset_end=0x%llx fb_size=0x%llx\n",
		   amdgv_idx_to_str(idx_vf), fb_offset, fb_offset_end, fb_size);

	if (adapt->ffbm.enabled) {
		FFBM_LOCK_LIST;
		amdgv_list_for_each_entry(pteb, &entry->gpa_list, struct amdgv_ffbm_pte_block,
					   gpa_list_node) {
			if (pteb->type != AMDGV_FFBM_MEM_TYPE_TMR)
				filled_size += amdgv_misc_do_clear_vf_fb(adapt, idx_vf, pteb->spa,
									 pteb->size, pattern);
		}
		FFBM_UNLOCK_LIST;
	} else
		filled_size = amdgv_misc_do_clear_vf_fb(adapt, idx_vf, fb_offset, fb_size, pattern);

	entry->gpu_init_data_ready = false;

	AMDGV_INFO("%s, fb_offset=0x%llx fb_size_cleared=0x%llx pattern[%u]\n",
		   amdgv_idx_to_str(idx_vf), fb_offset, filled_size, pattern);
	return 0;
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
		AMDGV_INFO("Patched DFC not found.\n");
	}

	/* switch to PF for GFX block */
	if (amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX) !=
	    0) {
		AMDGV_ERROR("switch to pf failed\n");
		return AMDGV_FAILURE;
	}

	fw_id = AMDGV_FIRMWARE_ID__DFC_FW;

	if (adapt->ucode.load) {
		if (adapt->ucode.load(adapt, &fw_id, 1))
			ret = PSP_STATUS__ERROR_GENERIC;
	} else if (adapt->misc.load_dfc) {
		ret = adapt->misc.load_dfc(adapt);
	}

	if (amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX) != 0) {
		AMDGV_ERROR("save pf failed\n");
		return AMDGV_FAILURE;
	}

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

int amdgv_misc_migrate_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t idx_fb_block,
			  void *data, bool to_fb)
{
	uint64_t filled_size;
	uint64_t block_size = AMDGV_MIGRATION_VF_FB_COPY_BLOCK_SIZE;
	uint64_t vf_fb_offset;
	uint64_t vf_fb_size;
	struct amdgv_vf_device *entry;

	entry = &adapt->array_vf[idx_vf];
	if (!entry->configured)
		return AMDGV_FAILURE;

	if (!adapt->sys_mem_info.handle) {
		AMDGV_ERROR("AGP memory not ready, exit.\n");
		return AMDGV_FAILURE;
	}

	vf_fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
	vf_fb_size = MBYTES_TO_BYTES(entry->fb_size);

	AMDGV_DEBUG("%s fb_offset=0x%llx fb_size=0x%llx\n", amdgv_idx_to_str(idx_vf),
		    vf_fb_offset, vf_fb_size);

	if (adapt->misc.dma_copy) {
		uint64_t src;
		uint64_t dst;
		uint64_t fb_offset;

		if (to_fb) {
			/* Check if pass-in data is pointing to agp memory */
			if (data != adapt->sys_mem_info.va_ptr) {
				AMDGV_DEBUG("oss_memcpy needed.\n");
				oss_memcpy(adapt->sys_mem_info.va_ptr, data, block_size);
			}

			src = adapt->mc_agp_loc_addr;
			dst = adapt->mc_fb_loc_addr + vf_fb_offset + idx_fb_block * block_size;
			if (adapt->xgmi.phy_nodes_num > 1)
				dst += adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;

			fb_offset = vf_fb_offset + idx_fb_block * block_size;
			AMDGV_DEBUG("src in mc addr = 0x%llx, dst in mc addr = 0x%llx\n", src,
				    dst);

			filled_size = idx_fb_block * block_size;
			if (block_size >= (vf_fb_size - filled_size)) {
				block_size = vf_fb_size - filled_size;
			}
			AMDGV_DEBUG("block_size = 0x%lx\n", block_size);

			filled_size = 0;
			adapt->misc.dma_copy(adapt, idx_vf, false, src, dst, block_size, &filled_size);
			if (filled_size)
				AMDGV_DEBUG("0x%lx bytes copied with GPU\n", filled_size);

		} else {
			src = adapt->mc_fb_loc_addr + vf_fb_offset + idx_fb_block * block_size;
			if (adapt->xgmi.phy_nodes_num > 1)
				src += adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;
			dst = adapt->mc_agp_loc_addr;

			fb_offset = vf_fb_offset + idx_fb_block * block_size;
			AMDGV_DEBUG("src in mc addr = 0x%llx, dst in mc addr = 0x%llx\n", src,
				    dst);

			filled_size = idx_fb_block * block_size;
			if (block_size >= (vf_fb_size - filled_size)) {
				block_size = vf_fb_size - filled_size;
			}
			AMDGV_DEBUG("block_size = 0x%lx\n", block_size);

			filled_size = 0;
			adapt->misc.dma_copy(adapt, idx_vf, false, src, dst, block_size, &filled_size);
			if (filled_size)
				AMDGV_DEBUG("0x%lx bytes copied with GPU\n", filled_size);

			/* Check if pass-in data is pointing to agp memory */
			if (data != adapt->sys_mem_info.va_ptr) {
				AMDGV_DEBUG("oss_memcpy needed.\n");
				oss_memcpy(data, adapt->sys_mem_info.va_ptr, block_size);
			}
		}
	} else {
		AMDGV_ERROR("dma copy not supported.\n");
		return AMDGV_FAILURE;
	}

	return 0;
}
