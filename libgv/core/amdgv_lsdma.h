/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AMDGV_LSDMA_H__
#define __AMDGV_LSDMA_H__

#include "amdgv_ring.h"

/* max number of rings */
#define AMDGV_MAX_LSDMA_RINGS 2

struct amdgv_lsdma {
	struct amdgv_ring lsdma_ring[AMDGV_MAX_LSDMA_RINGS];
	struct amdgv_memmgr_mem *bitmap_mem;
	uint32_t bitmap_size;
	int (*query_dirtybit)(struct amdgv_ring *ring,
			      uint64_t src_addr, uint32_t page_nr,
			      uint64_t dst_addr, uint32_t idx_vf,
			      bool clear_dbit);
};

int amdgv_lsdma_alloc_bitmap_mem(struct amdgv_adapter *adapt, uint64_t bitmap_size);
int amdgv_lsdma_free_bitmap_mem(struct amdgv_adapter *adapt);

#endif /* __AMDGV_LSDMA_H__ */
