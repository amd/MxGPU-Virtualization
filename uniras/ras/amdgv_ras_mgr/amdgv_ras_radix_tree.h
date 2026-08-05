/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _AMDGV_RAS_RADIX_TREE_H
#define _AMDGV_RAS_RADIX_TREE_H

#include "ras_sys.h"

/* Platform-specific definitions */
#ifndef BITS_PER_LONG
#if defined(_MSC_VER)
	#define BITS_PER_LONG  64
#elif defined(__GNUC__)
	#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__) || defined(__aarch64__)
		#define BITS_PER_LONG  64
	#else
		#define BITS_PER_LONG  32
	#endif
#else
	#define BITS_PER_LONG  64
#endif
#endif

/* Maximum number of tags supported per entry */
#define RADIX_TREE_MAX_TAGS 3

/* Number of bits used per level of the tree */
#ifndef RADIX_TREE_MAP_SHIFT
#define RADIX_TREE_MAP_SHIFT   6
#endif

/* Number of slots per node (2^RADIX_TREE_MAP_SHIFT) */
#define RADIX_TREE_MAP_SIZE    (1UL << RADIX_TREE_MAP_SHIFT)
#define RADIX_TREE_MAP_MASK    (RADIX_TREE_MAP_SIZE - 1)

/* Number of u64 words needed for tag bitmaps */
#define RADIX_TREE_TAG_LONGS   \
	((RADIX_TREE_MAP_SIZE + BITS_PER_LONG - 1) / BITS_PER_LONG)

/**
 * struct radix_tree_node - Internal node of the radix tree
 * @shift: Number of bits remaining in index for children
 * @offset: This node's slot offset in parent
 * @count: Number of non-NULL children plus internal node markers
 * @parent: Pointer to parent node (NULL for root's child)
 * @private_data: User data associated with this node
 * @private_list: Linked list for user purposes
 * @slots: Array of child pointers (nodes or data items)
 * @tags: Bitmaps for each tag type
 *
 * Note: The layout uses a union for parent/private_data to maintain
 * compatibility with existing code that may access these fields.
 */
struct radix_tree_node {
	u8     shift;
	u8     offset;
	u32    count;
	union {
		struct {
			struct radix_tree_node *parent;
			void *private_data;
		};
	};
	oss_list_head private_list;
	void  *slots[RADIX_TREE_MAP_SIZE];
	u64  tags[RADIX_TREE_MAX_TAGS][RADIX_TREE_TAG_LONGS];
};

/**
 * struct radix_tree_root - Root of a radix tree
 * @gfp_mask: Allocation flags in lower bits, root tags in upper bits
 * @rnode: Pointer to root node (or direct data for single-item tree)
 *
 * Note: This structure layout matches the original for binary compatibility.
 * Tags are stored in the upper bits of gfp_mask (above bit 28).
 */
struct radix_tree_root {
	u32    gfp_mask;
	struct radix_tree_node *rnode;
};

/**
 * struct radix_tree_iter - Iterator state for traversing the tree
 * @index: Current index being iterated
 * @next_index: Next index to check after current chunk
 * @tags: Tag bitmap for tagged iteration
 */
struct radix_tree_iter {
	u64    index;
	u64    next_index;
	u64    tags;
};

/* Iterator flags */
#define RADIX_TREE_ITER_TAG_MASK   0x00FF
#define RADIX_TREE_ITER_TAGGED     0x0100
#define RADIX_TREE_ITER_CONTIG     0x0200

/**
 * amdgv_radix_tree_for_each_slot - Iterate over all slots
 * @slot: void** variable for the current slot pointer
 * @root: Pointer to radix_tree_root
 * @iter: Pointer to radix_tree_iter
 * @start: Starting index
 */
#define amdgv_radix_tree_for_each_slot(slot, root, iter, start)  \
	for (slot = amdgv_radix_tree_iter_init(iter, start);     \
	     slot || (slot = amdgv_radix_tree_next_chunk(root, iter, 0)); \
	     slot = amdgv_radix_tree_next_slot(slot, iter, 0))

/* Public API functions */
void amdgv_radix_tree_init(void *root);
void amdgv_radix_tree_fini(void *root);
int amdgv_radix_tree_insert(void *root, u64 index, void *item);
void *amdgv_radix_tree_delete(void *root, u64 index);
void *amdgv_radix_tree_lookup(void *root, u64 index);
u32 amdgv_radix_tree_gang_lookup_tag(void *root, void **results,
		u64 first_index, u32 max_items, u32 tag);
void *amdgv_radix_tree_tag_set(void *root, u64 index, u32 tag);
void *amdgv_radix_tree_tag_clear(void *root, u64 index, u32 tag);
void **amdgv_radix_tree_iter_init(void *iter, u64 start);
void **amdgv_radix_tree_next_chunk(void *root, void *iter, unsigned int flags);
void **amdgv_radix_tree_next_slot(void **slot, void *iter, unsigned int flags);
void *amdgv_radix_tree_deref_slot(void **slot);
void *amdgv_radix_tree_delete_iter(void *root, void *iter);

#endif /* _AMDGV_RAS_RADIX_TREE_H */
