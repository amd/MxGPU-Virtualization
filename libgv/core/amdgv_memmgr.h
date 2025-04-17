/*
 * Copyright (c) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_MEMMGR_H
#define AMDGV_MEMMGR_H

#include "amdgv_live_info.h"

struct amdgv_memmgr;
struct amdgv_memmgr_mem;

#define PSP_CMD_BUF_ID(idx_cmd) \
	(idx_cmd == 0 ? MEM_PSP_CMD_BUF_0 : \
	(idx_cmd == 1 ? MEM_PSP_CMD_BUF_1 : \
	(idx_cmd == 2 ? MEM_PSP_CMD_BUF_2 : \
	(idx_cmd == 3 ? MEM_PSP_CMD_BUF_3 : \
	(idx_cmd == 4 ? MEM_PSP_CMD_BUF_4 : \
	(idx_cmd == 5 ? MEM_PSP_CMD_BUF_5 : \
	(idx_cmd == 6 ? MEM_PSP_CMD_BUF_6 : \
	(idx_cmd == 7 ? MEM_PSP_CMD_BUF_7 : \
	(idx_cmd == 8 ? MEM_PSP_CMD_BUF_8 : \
	(idx_cmd == 9 ? MEM_PSP_CMD_BUF_9 : \
	(idx_cmd == 10 ? MEM_PSP_CMD_BUF_10 : \
	(idx_cmd == 11 ? MEM_PSP_CMD_BUF_11 : \
	(idx_cmd == 12 ? MEM_PSP_CMD_BUF_12 : \
	(idx_cmd == 13 ? MEM_PSP_CMD_BUF_13 : \
	(idx_cmd == 14 ? MEM_PSP_CMD_BUF_14 : \
	(idx_cmd == 15 ? MEM_PSP_CMD_BUF_15 : \
	(MEM_ID_UNKNOWN)))))))))))))))))

#define GET_MEM_LOCATION(mem) \
	(&adapt->memmgr_pf == mem->memmgr ? "PF" : \
	(&adapt->memmgr_gpu == mem->memmgr ? "GPU" : \
	(&adapt->memmgr_sys == mem->memmgr ? "SYS" : \
	("Unknown"))))

#define MEM_ID_COUNT_MAX 256
#define MEM_ID_NOT_AVAILABLE -1
#define MAX_MEM_COUNT 256
#define MAX_BITMAP_COUNT (MAX_MEM_COUNT / 64)

#define MEM_ID_GET_ID(mem_id) (mem_id & 0xFFFF)
#define MEM_ID_GET_INDEX(mem_id) ((mem_id >> 16) & 0xFFFF)

/*
 *
 * The memory manager implements a manager for the physical FB
 * allocation of libgv.
 * The manager internally contains a list of allocations tracked
 * by the end of the allocation (alloc_off). The allocator either
 * grows the heap from the top MC address downwards (down) or from
 * the botto, MC address upwards (up).
 */

/*
 *          +------------+                       +------------+
 *          |            |                       |            |
 *          |            |               +-------+            <--MC_BASE
 *          |            |               |       |            |
 *          |            |               |     | +------------<--OFFSET
 *          |            |               | len | | ALLOCATION |
 *          |            |               |     v +------------<--alloc_off
 *          |            |               |       | ALLOCATION |
 *          |            |               |       +------------+
 *          +------------+          ^    | TOM   | ALLOCATION |
 *          | ALLOCATION +          |    v       +------------+
 *          +------------+          |            |            |
 *          | ALLOCATION |          |            |            |
 *alloc_off +------------+          | TOM        |            |
 *        | | ALLOCATION |          |            |            |
 *  (len) | +------------<--OFFSET  |            |            |
 *          |            |          |            |            |
 *          |            <--MC_BASE +            |            |
 *          |            |                       |            |
 *          |            |                       |            |
 *          |            |                       |            |
 *          |            |                       |            |
 *          +------------+                       +------------+
 *               (up)                                (down)
 */

/* Memory allocation node */
struct amdgv_memmgr_mem {
	struct amdgv_memmgr *memmgr;     /* ptr to memory manager */
	uint64_t alloc_off;              /* offset to the end of the alloc */
	uint64_t len;                    /* length of the allocation */
	uint64_t align;                  /* alignment of the allocation */
	struct oss_dma_mem_info sys_mem; /* system memory*/

	/* node tracker in the memmgr list */
	struct amdgv_list_head node;

	enum amdgv_mem_id id;	/* id to track mem block user */
};

struct amdgv_memmgr {
	struct amdgv_adapter *adapt;
	mutex_t lock;

	/* Base definitions of the memory manager */
	uint64_t offset; /* Offset from mc_base to first allocation */
	uint64_t size;   /* Maximum size of the heap */
	uint64_t align;  /* Minimum alignment in the heap */

	/* Base definitions of the aperture */
	uint64_t mc_base; /* MC base address, can be either top of memory
			     on downwards or bottom of memory on upwards */
	void *cpu_base;   /* If available cpu visible address of MC_offset */

	/* Allocation list */
	struct amdgv_memmgr_mem *allocs;

	/* Tracker for the amount of memory consumed by the heap */
	uint64_t tom;

	/* grows up or down */
	bool down;

	/* is init flag */
	bool is_init;
};

struct amdgv_mem_with_bitmap {
	enum amdgv_mem_id mem_id;
	/* records indices been assigned for the same mem_id
	 * e.g. 0b00000111 means index 0, 1 and 2 are assigned
	 *      0b00001011 means index 0, 1 and 3 are assigned
	 *
	 * currently allow up to 64 memmgr_mem with the same mem_id,
	 * and now memmgr_mem->id will be: [31:16]index[15:0]mem_id
	 */
	uint64_t bitmaps[MAX_BITMAP_COUNT];
};

/* Allocate a memory manager */
int amdgv_memmgr_init(struct amdgv_adapter *adapt, struct amdgv_memmgr *memmgr,
		      uint64_t offset, uint64_t size, uint32_t align, bool down);
/* Destroy memory manager */
int amdgv_memmgr_fini(struct amdgv_adapter *adapt, struct amdgv_memmgr *memmgr);

/* Functions to reserve memory from the manager.
 * They return non-NULL on success
 */
struct amdgv_memmgr_mem *amdgv_memmgr_alloc(struct amdgv_memmgr *memmgr, uint64_t len,
					    enum amdgv_mem_id id);
struct amdgv_memmgr_mem *amdgv_memmgr_alloc_align(struct amdgv_memmgr *memmgr, uint64_t len,
						  uint64_t align, enum amdgv_mem_id id);
struct amdgv_memmgr_mem *amdgv_memmgr_alloc_align_at(struct amdgv_memmgr *memmgr,
						     uint64_t offset, uint64_t len,
						     enum amdgv_mem_id id);
int amdgv_memmgr_fill_reserved_bad_pages_all(struct amdgv_adapter *adapt,
					     struct amdgv_memmgr_mem **bps_mem);
int amdgv_memmgr_export_mem_allocs_all(struct amdgv_adapter *adapt,
				       struct amdgv_live_info_memmgr_mem *mem_allocs,
				       uint32_t *mem_allocs_count);
int amdgv_memmgr_import_mem_allocs_all(struct amdgv_adapter *adapt,
				       struct amdgv_live_info_memmgr_mem *mem_nodes,
				       uint32_t mem_allocs_count);
/* Free a memory reservation */
int amdgv_memmgr_free(struct amdgv_memmgr_mem *mem);
/* Returns the current Top of Memory, that is the used size of the manager */
int amdgv_memmgr_get_tom(struct amdgv_memmgr *memmgr, uint64_t *tom);
int amdgv_memmgr_get_limit(struct amdgv_memmgr *memmgr, uint64_t *limit);

/* Define the mamimum size of the memory allocator */
int amdgv_memmgr_set_size(struct amdgv_memmgr *memmgr, uint64_t limit);
/* Set the physical offset/base of the allocator */
int amdgv_memmgr_set_gpu_base(struct amdgv_memmgr *memmgr, uint64_t base);
int amdgv_memmgr_set_cpu_base(struct amdgv_memmgr *memmgr, void *base);

/* Get the physical base of the allocator */
uint64_t amdgv_memmgr_get_gpu_base(struct amdgv_memmgr_mem *mem);
void *amdgv_memmgr_get_cpu_base(struct amdgv_memmgr_mem *mem);

/* Get the physical address of a memory reservation */
uint64_t amdgv_memmgr_get_gpu_addr(struct amdgv_memmgr_mem *mem);
void *amdgv_memmgr_get_cpu_addr(struct amdgv_memmgr_mem *mem);

/* Get the physical offset of a memory reservation */
uint64_t amdgv_memmgr_get_offset(struct amdgv_memmgr_mem *mem);

/* Get size of the reservation */
uint64_t amdgv_memmgr_get_size(struct amdgv_memmgr_mem *mem);
/* Get alignment of the reservation */
uint64_t amdgv_memmgr_get_align(struct amdgv_memmgr_mem *mem);

enum amdgv_live_info_status amdgv_memmgr_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_memmgr *memmgr_info);
enum amdgv_live_info_status amdgv_memmgr_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_memmgr *memmgr_info);
#endif
