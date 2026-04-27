/*
 * Copyright 2022 Advanced Micro Devices, Inc.
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

#ifndef __AMDGV_SDMA_H__
#define __AMDGV_SDMA_H__

#include "amdgv_ring.h"

#define SDMA_MAX_TIMEOUT (500000)

enum amdgv_sdma_ras_memory_id {
	AMDGV_SDMA_MBANK_DATA_BUF0 = 1,
	AMDGV_SDMA_MBANK_DATA_BUF1 = 2,
	AMDGV_SDMA_MBANK_DATA_BUF2 = 3,
	AMDGV_SDMA_MBANK_DATA_BUF3 = 4,
	AMDGV_SDMA_MBANK_DATA_BUF4 = 5,
	AMDGV_SDMA_MBANK_DATA_BUF5 = 6,
	AMDGV_SDMA_MBANK_DATA_BUF6 = 7,
	AMDGV_SDMA_MBANK_DATA_BUF7 = 8,
	AMDGV_SDMA_MBANK_DATA_BUF8 = 9,
	AMDGV_SDMA_MBANK_DATA_BUF9 = 10,
	AMDGV_SDMA_MBANK_DATA_BUF10 = 11,
	AMDGV_SDMA_MBANK_DATA_BUF11 = 12,
	AMDGV_SDMA_MBANK_DATA_BUF12 = 13,
	AMDGV_SDMA_MBANK_DATA_BUF13 = 14,
	AMDGV_SDMA_MBANK_DATA_BUF14 = 15,
	AMDGV_SDMA_MBANK_DATA_BUF15 = 16,
	AMDGV_SDMA_UCODE_BUF = 17,
	AMDGV_SDMA_RB_CMD_BUF = 18,
	AMDGV_SDMA_IB_CMD_BUF = 19,
	AMDGV_SDMA_UTCL1_RD_FIFO = 20,
	AMDGV_SDMA_UTCL1_RDBST_FIFO = 21,
	AMDGV_SDMA_UTCL1_WR_FIFO = 22,
	AMDGV_SDMA_DATA_LUT_FIFO = 23,
	AMDGV_SDMA_SPLIT_DAT_BUF = 24,
	AMDGV_SDMA_MEMORY_BLOCK_LAST,
};

struct amdgv_sdma_ras_funcs {
	void (*err_cnt_init)(struct amdgv_adapter *adapt);
	int (*ras_late_init)(struct amdgv_adapter *adapt);
	void (*query_ras_error_count)(struct amdgv_adapter *adapt, void *ras_error_status);
	void (*reset_ras_error_count)(struct amdgv_adapter *adapt);
};

struct amdgv_sdma {
	int num_instances;
	uint32_t harvest_instances;
	uint32_t num_pf_dedicated_inst;
	uint32_t sdma_mask;
	uint32_t harvest_sdma_mask;
	int num_sdma_rings;
	uint32_t page_size_config;
	struct amdgv_ring sdma_ring[AMDGV_MAX_SDMA_RINGS];
	int num_inst_per_aid;
	struct ras_common_if	*ras_if;
	const struct amdgv_sdma_ras_funcs	*funcs;
	int (*sdma_copy)(struct amdgv_ring *ring, uint64_t src, uint64_t size, uint64_t dest);
	struct amdgv_memmgr_mem *bitmap_mem;
	uint32_t bitmap_size;
	void (*query_dirtybit)(struct amdgv_ring *ring,
			uint64_t src_addr, uint32_t page_nr,
			uint64_t dst_addr,
			uint32_t aid_inst, uint32_t ea_inst, uint32_t dagb,
			bool clear_dbit);
};

#define IS_DEDICATED_SDMA_RING_AVAILABLE(adapt) ((adapt)->sdma.num_pf_dedicated_inst != 0)
#define AMDGV_SDMA_PF_DECIDATED_RING_START_INDEX(adapt) ((adapt)->sdma.num_instances)
#define AMDGV_SDMA_PF_DECIDATED_RING_END_INDEX(adapt) \
		(AMDGV_SDMA_PF_DECIDATED_RING_START_INDEX(adapt) + (adapt)->sdma.num_pf_dedicated_inst)

#define for_each_pf_dedicated_sdma_inst(inst, adapt) \
	for (inst = AMDGV_SDMA_PF_DECIDATED_RING_START_INDEX(adapt); \
		inst < AMDGV_SDMA_PF_DECIDATED_RING_END_INDEX(adapt); \
		inst++)
#define for_each_pfvf_shared_sdma_inst(inst, adapt) \
	for (inst = 0; inst < (adapt)->sdma.num_sdma_rings; inst++)

#define amdgv_sdma_ring_query_dirtybit(r, mc_addr, nr_pages, dst, aid, ea, dagb, clear_dbit) \
		((adapt)->sdma.query_dirtybit != NULL ? \
		 (adapt)->sdma.query_dirtybit((r), (mc_addr), (nr_pages), (dst), (aid), (ea), (dagb), (clear_dbit)) : \
		 AMDGV_FAILURE)

struct amdgv_ring *amdgv_sdma_get_available_ring(struct amdgv_adapter *adapt, enum amdgv_ring_shared_type shared_type);
int amdgv_sdma_ring_copy(struct amdgv_ring *ring, uint64_t src, uint64_t size, uint64_t dst);
int amdgv_sdma_alloc_bitmap_mem(struct amdgv_adapter *adapt, uint64_t bitmap_size);
int amdgv_sdma_free_bitmap_mem(struct amdgv_adapter *adapt);
struct amdgv_ring *amdgv_sdma_get_pf_dedicated_ring(struct amdgv_adapter *adapt, int aid, int index);
struct amdgv_ring *amdgv_sdma_get_pfvf_shared_ring(struct amdgv_adapter *adapt, int aid, int index);
#endif
