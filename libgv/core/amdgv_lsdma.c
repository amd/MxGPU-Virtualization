/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_lsdma.h"

static const uint32_t this_block = AMDGV_LSDMA_BLOCK;

int amdgv_lsdma_alloc_bitmap_mem(struct amdgv_adapter *adapt, uint64_t bitmap_size)
{
	adapt->lsdma.bitmap_mem = amdgv_memmgr_alloc_sys_align_zero(&adapt->memmgr_sys,
						bitmap_size, PAGE_SIZE, NULL, NULL);
	if (adapt->lsdma.bitmap_mem == NULL) {
		AMDGV_WARN("Failed to allocate lsdma dirty-bit bitmap (%llu bytes)\n",
			   bitmap_size);
		return AMDGV_FAILURE;
	}

	adapt->lsdma.bitmap_size = (uint32_t)bitmap_size;

	return 0;
}

int amdgv_lsdma_free_bitmap_mem(struct amdgv_adapter *adapt)
{
	if (adapt->lsdma.bitmap_mem) {
		amdgv_memmgr_free(adapt->lsdma.bitmap_mem);
		adapt->lsdma.bitmap_mem = NULL;
	}
	adapt->lsdma.bitmap_size = 0;

	return 0;
}
