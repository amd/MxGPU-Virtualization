/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __GIM_MEMORY_SENTINEL_H__
#define __GIM_MEMORY_SENTINEL_H__

#include <linux/spinlock.h>
#include <drm/spsc_queue.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include "gim.h"

#define GIM_MEMORY_SENTINEL_BITMAP_ORDER                6
#define GIM_MEMORY_SENTINEL_TABLE_COL                   (1 << GIM_MEMORY_SENTINEL_BITMAP_ORDER)
#define GIM_MEMORY_SENTINEL_TABLE_ROW                   (1 << GIM_MEMORY_SENTINEL_BITMAP_ORDER)
#define GIM_MEMORY_SENTINEL_TABLE_MAX                   (1 << GIM_MEMORY_SENTINEL_BITMAP_ORDER)
#define GIM_MEMORY_SENTINEL_TABLE_ENTRY_INDEX_MAX       (1 << (GIM_MEMORY_SENTINEL_BITMAP_ORDER * 3))

#define GIM_MEMORY_SENTINEL_BITMAP_INIT                 0xffffffffffffffff
#define GIM_MEMORY_SENTINEL_INVALID_INDEX               0xffffffff
#define GIM_MEMORY_SENTINEL_TABLE_LIST_MASK             0x3f
#define GIM_MEMORY_SENTINEL_TABLE_ROW_MASK              0x3f

#define GIM_MEMORY_SENTINEL_ENTRY_HEADER_BROKEN         0x1
#define GIM_MEMORY_SENTINEL_ENTRY_REAR_BROKEN           0x2
#define GIM_MEMORY_SENTINEL_ENTRY_DUMMY_FREED           0x4

#define GIM_MEMORY_SENTINEL_ALLOC_TYPE_INVALID          -1
#define GIM_MEMORY_SENTINEL_ALLOC_TYPE_NORMAL           0
#define GIM_MEMORY_SENTINEL_ALLOC_TYPE_PAGE_ALIGN       1
#define GIM_MEMORY_SENTINEL_ALLOC_TYPE_DMA              2

#define GIM_MEMORY_SENTINEL_ERROR_INVALID_INPUT			-1
#define GIM_MEMORY_SENTINEL_ERROR_OVERFLOW				-2
#define GIM_MEMORY_SENTINEL_ERROR_MEMLEAK				-3
#define GIM_MEMORY_SENTINEL_ERROR_NO_MEM				-4

#ifndef MAX_PAGE_ORDER
#define GIM_MEMORY_SENTINEL_ALLOC_MAX_ORDER             11
#else
#define GIM_MEMORY_SENTINEL_ALLOC_MAX_ORDER             MAX_PAGE_ORDER
#endif

void gim_memory_sentinel_init(void);
void gim_memory_sentinel_fini(void);

/*
* |<--------------  total size  -------------->|
*          |<---- user needed size ---->|
* ----------------------------------------------
* | header |                            | rear |
* ----------------------------------------------
*          ^ return to user
*/
struct gim_memory_sentinel_malloc_header {
	union {
		struct {
			uint32_t size;
			uint32_t index;
		};
		uint64_t header;
	};
	// ……
	// void *rear;
};

uint32_t gim_sentinel_check_memory_overflow(void);
bool gim_sentinel_is_enabled(void);

#endif // __GIM_MEMORY_SENTINEL_H__
