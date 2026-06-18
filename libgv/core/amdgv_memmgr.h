/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_MEMMGR_H
#define AMDGV_MEMMGR_H

#include "amdgv_live_info.h"
#include "amdgv_ring.h"

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

enum amdgv_map_op {
	AMDGV_UNMAP = 0,
	AMDGV_MAP = 1,
};

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
	struct amdgv_memmgr_mem *reserves;
	int reserve_count;

	/* Tracker for the amount of memory consumed by the heap */
	uint64_t tom;

	/* grows up or down */
	bool down;

	/* is init flag */
	bool is_init;

	/* is system memmory manager or not*/
	bool is_sys;

	struct amdgv_list_head reservations_pending;
	struct amdgv_list_head reserved_pages;
	mutex_t rsv_lock;
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

/*
 * GPU MC structures, functions & helpers
 */
struct amdgv_gmc_funcs {
	/* flush the vm tlb via mmio */
	int (*flush_gpu_tlb)(struct amdgv_adapter *adapter, uint32_t vmid,
						uint32_t vmhub, uint32_t flush_type);
	uint64_t (*get_gart_map_flags)(struct amdgv_adapter *adapt);
	/* flush the vm tlb via ring */
	uint64_t (*emit_flush_gpu_tlb)(struct amdgv_ring *ring, unsigned vmid,
								uint64_t pd_addr);
};

struct amdgv_gmc {
	const struct amdgv_gmc_funcs *funcs;
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
struct amdgv_memmgr_mem *amdgv_memmgr_alloc_sys_align_with_attr(struct amdgv_memmgr *memmgr,
							uint64_t len, uint64_t align,
							enum oss_page_attr page_attr,
							uint64_t *gpu_addr, void *va_ptr);
struct amdgv_memmgr_mem *amdgv_memmgr_alloc_align_zero(struct amdgv_memmgr *memmgr, uint64_t len,
							uint64_t align, enum amdgv_mem_id id);
struct amdgv_memmgr_mem *amdgv_memmgr_alloc_sys_align_zero(struct amdgv_memmgr *memmgr, uint64_t len,
							uint64_t align, uint64_t *gpu_addr, void *va_ptr);
struct amdgv_memmgr_mem *amdgv_memmgr_alloc_align_at(struct amdgv_memmgr *memmgr,
						     uint64_t offset, uint64_t len,
						     enum amdgv_mem_id id);
int amdgv_memmgr_fill_reserved_bad_pages_all(struct amdgv_adapter *adapt,
					     struct amdgv_memmgr_mem **bps_mem);
struct amdgv_memmgr_mem *amdgv_memmgr_find_mem_at_offset(struct amdgv_memmgr *memmgr,
							  uint64_t offset,
							  uint64_t size);
struct amdgv_memmgr_mem *amdgv_memmgr_alloc_and_replace(struct amdgv_adapter *adapt,
					     struct amdgv_memmgr *memmgr,
						 struct amdgv_memmgr_mem *mem_old,
						 enum amdgv_mem_id replace_type);

int amdgv_memmgr_export_mem_allocs_all(struct amdgv_adapter *adapt,
				       struct amdgv_live_info_memmgr_mem *mem_allocs,
				       uint32_t *mem_allocs_count);
int amdgv_memmgr_import_mem_allocs_all(struct amdgv_adapter *adapt,
				       struct amdgv_live_info_memmgr_mem *mem_nodes,
				       uint32_t mem_allocs_count);
/* Free a memory reservation */
int amdgv_memmgr_free_by_id(struct amdgv_memmgr *memmgr, enum amdgv_mem_id id);
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
uint64_t amdgv_memmgr_get_gpu_pa(struct amdgv_memmgr_mem *mem);
void *amdgv_memmgr_get_cpu_addr(struct amdgv_memmgr_mem *mem);

/* Get the physical offset of a memory reservation */
uint64_t amdgv_memmgr_get_offset(struct amdgv_memmgr_mem *mem);

/* Get size of the reservation */
uint64_t amdgv_memmgr_get_size(struct amdgv_memmgr_mem *mem);
/* Get alignment of the reservation */
uint64_t amdgv_memmgr_get_align(struct amdgv_memmgr_mem *mem);

enum amdgv_live_info_status amdgv_memmgr_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_memmgr *memmgr_info);
enum amdgv_live_info_status amdgv_memmgr_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_memmgr *memmgr_info);
void amdgv_gmc_flush_gpu_tlb(struct amdgv_adapter *adapt, uint32_t vmid,
					uint32_t vmhub, uint32_t flush_type);
int amdgv_map_sys_mem_allocs(struct amdgv_memmgr *memmgr, enum amdgv_map_op map_op);

int amdgv_memmgr_pf_init(struct amdgv_adapter *adapt);
int amdgv_memmgr_pf_fini(struct amdgv_adapter *adapt);
int amdgv_memmgr_reserve_page(struct amdgv_adapter *adapt,
		struct amdgv_memmgr *mgr, uint64_t pfn);
int amdgv_memmgr_query_page_reserve_status(struct amdgv_adapter *adapt,
		struct amdgv_memmgr *mgr, uint64_t start);
bool amdgv_memmgr_check_critical_address(struct amdgv_adapter *adapt,
		uint64_t address);

int amdgv_memmgr_assign_reserved_region(struct amdgv_memmgr_mem *reserved);
int amdgv_memmgr_alloc_deferred_region(struct amdgv_memmgr *memmgr);

bool amdgv_memmgr_addr_in_range(struct amdgv_adapter *adapt, struct amdgv_memmgr *memmgr, uint64_t addr);

#endif
