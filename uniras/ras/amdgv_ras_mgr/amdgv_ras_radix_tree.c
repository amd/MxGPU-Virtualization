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
 *
 * This is a clean-room implementation of a radix tree data structure.
 */

#include "ras_core_status.h"
#include "amdgv_ras_radix_tree.h"

/* Number of bits in a 64-bit index */
#define RADIX_TREE_INDEX_BITS  64

/* Maximum tree height needed for 64-bit indices */
#define RADIX_TREE_MAX_HEIGHT  \
	((RADIX_TREE_INDEX_BITS + RADIX_TREE_MAP_SHIFT - 1) / RADIX_TREE_MAP_SHIFT)

/* Bit manipulation helpers */
#define RT_BIT_MASK(nr)    (1ULL << ((nr) % BITS_PER_LONG))
#define RT_BIT_WORD(nr)    ((nr) / BITS_PER_LONG)

/*
 * Internal node marker: The lowest bit of a pointer indicates if it points
 * to an internal node (1) or user data (0). This allows storing data directly
 * in slots at leaf level while being able to distinguish from internal nodes.
 */
#define RADIX_TREE_INTERNAL_NODE  1UL
#define RADIX_TREE_ENTRY_MASK     1UL

/* GFP mask bits - tags are stored in upper bits */
#define __GFP_BITS_SHIFT   28
#define __GFP_BITS_MASK    ((1U << __GFP_BITS_SHIFT) - 1)
#define RT_GFP_KERNEL      0xCC0U

/*
 * Check if a pointer is an internal node
 */
static inline int is_internal_node(void *ptr)
{
	return ((u64)ptr & RADIX_TREE_ENTRY_MASK) == RADIX_TREE_INTERNAL_NODE;
}

/*
 * Convert internal node pointer to actual node pointer
 */
static inline struct radix_tree_node *entry_to_node(void *ptr)
{
	return (struct radix_tree_node *)((u64)ptr & ~RADIX_TREE_INTERNAL_NODE);
}

/*
 * Convert node pointer to internal entry (with marker bit set)
 */
static inline void *node_to_entry(struct radix_tree_node *node)
{
	return (void *)((u64)node | RADIX_TREE_INTERNAL_NODE);
}

/*
 * Allocate and initialize a new tree node
 */
static struct radix_tree_node *radix_tree_node_alloc(void)
{
	struct radix_tree_node *node;

	node = oss_zalloc(sizeof(*node));
	if (node) {
		OSS_INIT_LIST_HEAD(&node->private_list);
	}
	return node;
}

/*
 * Free a tree node
 */
static void radix_tree_node_free(struct radix_tree_node *node)
{
	if (node)
		oss_free(node);
}

/*
 * Set a tag bit for a slot in a node
 */
static void tag_set(struct radix_tree_node *node, u32 tag, u32 offset)
{
	u64 *addr = &node->tags[tag][RT_BIT_WORD(offset)];
	*addr |= RT_BIT_MASK(offset);
}

/*
 * Clear a tag bit for a slot in a node
 */
static void tag_clear(struct radix_tree_node *node, u32 tag, u32 offset)
{
	u64 *addr = &node->tags[tag][RT_BIT_WORD(offset)];
	*addr &= ~RT_BIT_MASK(offset);
}

/*
 * Check if a tag bit is set for a slot
 */
static int tag_get(struct radix_tree_node *node, u32 tag, u32 offset)
{
	return (node->tags[tag][RT_BIT_WORD(offset)] & RT_BIT_MASK(offset)) != 0;
}

/*
 * Check if any slot in a node has a specific tag set
 */
static int any_tag_set(struct radix_tree_node *node, u32 tag)
{
	u32 i;

	for (i = 0; i < RADIX_TREE_TAG_LONGS; i++) {
		if (node->tags[tag][i])
			return 1;
	}
	return 0;
}

/*
 * Set a tag at the root level (stored in upper bits of gfp_mask)
 */
static void root_tag_set(struct radix_tree_root *root, u32 tag)
{
	root->gfp_mask |= (1U << (tag + __GFP_BITS_SHIFT));
}

/*
 * Clear a tag at the root level
 */
static void root_tag_clear(struct radix_tree_root *root, u32 tag)
{
	root->gfp_mask &= ~(1U << (tag + __GFP_BITS_SHIFT));
}

/*
 * Clear all tags at the root level
 */
static void root_tag_clear_all(struct radix_tree_root *root)
{
	root->gfp_mask &= __GFP_BITS_MASK;
}

/*
 * Check if a tag is set at the root level
 */
static int root_tag_get(struct radix_tree_root *root, u32 tag)
{
	return (root->gfp_mask & (1U << (tag + __GFP_BITS_SHIFT))) != 0;
}

/*
 * Calculate the maximum index that can be stored at a given shift level
 */
static u64 shift_maxindex(u32 shift)
{
	return (RADIX_TREE_MAP_SIZE << shift) - 1;
}

/*
 * Get the maximum index for a node
 */
static u64 node_maxindex(struct radix_tree_node *node)
{
	return shift_maxindex(node->shift);
}

/*
 * Load root node and calculate max index
 */
static u32 radix_tree_load_root(struct radix_tree_root *root,
				struct radix_tree_node **nodep, u64 *maxindex)
{
	struct radix_tree_node *node = root->rnode;

	*nodep = node;

	if (is_internal_node(node)) {
		node = entry_to_node(node);
		*maxindex = node_maxindex(node);
		return node->shift + RADIX_TREE_MAP_SHIFT;
	}

	*maxindex = 0;
	return 0;
}

/*
 * Descend one level in the tree
 */
static u32 radix_tree_descend(struct radix_tree_node *parent,
			      struct radix_tree_node **nodep, u64 index)
{
	u32 offset = (index >> parent->shift) & RADIX_TREE_MAP_MASK;
	void *entry = parent->slots[offset];

	*nodep = entry;
	return offset;
}

/*
 * Extend tree height to accommodate a larger index
 */
static int radix_tree_extend(struct radix_tree_root *root, u64 index, u32 shift)
{
	struct radix_tree_node *slot;
	struct radix_tree_node *node;
	u32 maxshift;
	u32 tag;

	/* Figure out what the shift should be */
	maxshift = shift;
	while (index > shift_maxindex(maxshift))
		maxshift += RADIX_TREE_MAP_SHIFT;

	slot = root->rnode;
	if (!slot)
		goto out;

	do {
		node = radix_tree_node_alloc();
		if (!node)
			return -RAS_CORE_ENOMEM;

		/* Propagate the aggregated tag info into the new root */
		for (tag = 0; tag < RADIX_TREE_MAX_TAGS; tag++) {
			if (root_tag_get(root, tag))
				tag_set(node, tag, 0);
		}

		node->shift = shift;
		node->offset = 0;
		node->count = 1;
		node->parent = NULL;

		if (is_internal_node(slot))
			entry_to_node(slot)->parent = node;

		node->slots[0] = slot;
		slot = node_to_entry(node);
		root->rnode = (struct radix_tree_node *)slot;
		shift += RADIX_TREE_MAP_SHIFT;
	} while (shift <= maxshift);

out:
	return maxshift + RADIX_TREE_MAP_SHIFT;
}

/*
 * Create path to a slot in the radix tree
 */
static int radix_tree_create(struct radix_tree_root *root, u64 index,
			     struct radix_tree_node **nodep, void ***slotp)
{
	struct radix_tree_node *node = NULL;
	struct radix_tree_node *child;
	void **slot = (void **)&root->rnode;
	u64 maxindex;
	u32 shift;
	u32 offset = 0;

	shift = radix_tree_load_root(root, &child, &maxindex);

	/* Make sure the tree is high enough */
	if (index > maxindex) {
		int error = radix_tree_extend(root, index, shift);
		if (error < 0)
			return error;
		shift = error;
		child = root->rnode;
	}

	while (shift > 0) {
		shift -= RADIX_TREE_MAP_SHIFT;
		if (child == NULL) {
			/* Have to add a child node */
			child = radix_tree_node_alloc();
			if (!child)
				return -RAS_CORE_ENOMEM;
			child->shift = shift;
			child->offset = offset;
			child->parent = node;
			*slot = node_to_entry(child);
			if (node)
				node->count++;
		} else if (!is_internal_node(child))
			break;

		/* Go a level down */
		node = entry_to_node(child);
		offset = radix_tree_descend(node, &child, index);
		slot = &node->slots[offset];
	}

	if (nodep)
		*nodep = node;
	if (slotp)
		*slotp = slot;
	return 0;
}

/*
 * Shrink tree height if possible
 */
static int radix_tree_shrink(struct radix_tree_root *root)
{
	int shrunk = 0;

	for (;;) {
		struct radix_tree_node *node = root->rnode;
		struct radix_tree_node *child;

		if (!is_internal_node(node))
			break;
		node = entry_to_node(node);

		/* The candidate node has more than one child, or its child
		 * is not at the leftmost slot, we cannot shrink.
		 */
		if (node->count != 1)
			break;
		child = node->slots[0];
		if (!child)
			break;
		if (!is_internal_node(child) && node->shift)
			break;

		if (is_internal_node(child))
			entry_to_node(child)->parent = NULL;

		root->rnode = child;
		radix_tree_node_free(node);
		shrunk = 1;
	}

	return shrunk;
}

/*
 * Try to free node after clearing a slot
 */
static int radix_tree_delete_node(struct radix_tree_root *root,
				  struct radix_tree_node *node)
{
	int deleted = 0;

	do {
		struct radix_tree_node *parent;

		if (node->count) {
			if (node == entry_to_node(root->rnode))
				deleted |= radix_tree_shrink(root);
			return deleted;
		}

		parent = node->parent;
		if (parent) {
			parent->slots[node->offset] = NULL;
			parent->count--;
		} else {
			root_tag_clear_all(root);
			root->rnode = NULL;
		}

		radix_tree_node_free(node);
		deleted = 1;

		node = parent;
	} while (node);

	return deleted;
}

/*
 * Clear a tag going up the tree
 */
static void node_tag_clear(struct radix_tree_root *root,
			   struct radix_tree_node *node,
			   u32 tag, u32 offset)
{
	while (node) {
		if (!tag_get(node, tag, offset))
			return;
		tag_clear(node, tag, offset);
		if (any_tag_set(node, tag))
			return;

		offset = node->offset;
		node = node->parent;
	}

	/* Clear the root's tag bit */
	if (root_tag_get(root, tag))
		root_tag_clear(root, tag);
}

/*
 * Find the first set bit in a value
 */
static u32 find_first_set_bit(u64 val)
{
	u32 pos = 0;

	if (!val)
		return 64;

	if (!(val & 0xFFFFFFFFULL)) {
		pos += 32;
		val >>= 32;
	}
	if (!(val & 0xFFFFULL)) {
		pos += 16;
		val >>= 16;
	}
	if (!(val & 0xFFULL)) {
		pos += 8;
		val >>= 8;
	}
	if (!(val & 0xFULL)) {
		pos += 4;
		val >>= 4;
	}
	if (!(val & 0x3ULL)) {
		pos += 2;
		val >>= 2;
	}
	if (!(val & 0x1ULL))
		pos += 1;

	return pos;
}

/*
 * Find next set bit in the tag bitmap
 */
static u64 radix_tree_find_next_bit(const u64 *addr, u64 size, u64 offset)
{
	u64 tmp;

	if (offset < size) {
		addr += offset / BITS_PER_LONG;
		tmp = *addr >> (offset % BITS_PER_LONG);
		if (tmp)
			return find_first_set_bit(tmp) + offset;
		offset = (offset + BITS_PER_LONG) & ~((u64)BITS_PER_LONG - 1);
		while (offset < size) {
			tmp = *++addr;
			if (tmp)
				return find_first_set_bit(tmp) + offset;
			offset += BITS_PER_LONG;
		}
	}
	return size;
}

/*
 * Recursively free all nodes in a subtree
 */
static void delete_node_recursive(struct radix_tree_node *node)
{
	u32 i;

	if (!node)
		return;

	for (i = 0; i < RADIX_TREE_MAP_SIZE; i++) {
		void *child = node->slots[i];
		if (child && is_internal_node(child))
			delete_node_recursive(entry_to_node(child));
	}

	radix_tree_node_free(node);
}

/* ===== Public API Implementation ===== */

/**
 * amdgv_radix_tree_init - Initialize a radix tree
 * @root: Pointer to radix_tree_root structure
 */
void amdgv_radix_tree_init(void *root)
{
	struct radix_tree_root *r = (struct radix_tree_root *)root;

	r->gfp_mask = RT_GFP_KERNEL;
	r->rnode = NULL;
}

/**
 * amdgv_radix_tree_fini - Free all resources used by a radix tree
 * @root: Pointer to radix_tree_root structure
 *
 * Note: This will only free the internal nodes in the tree. The root
 * structure should be freed by the caller if it was dynamically allocated.
 */
void amdgv_radix_tree_fini(void *root)
{
	struct radix_tree_root *r = (struct radix_tree_root *)root;

	if (r->rnode && is_internal_node(r->rnode))
		delete_node_recursive(entry_to_node(r->rnode));
}

/**
 * amdgv_radix_tree_insert - Insert an item into the tree
 * @root: Pointer to radix_tree_root
 * @index: Index at which to insert
 * @item: Item to insert (must not be NULL)
 *
 * Returns: 0 on success, -RAS_CORE_ENOMEM if out of memory,
 *          -RAS_CORE_EEXIST if index already has an item
 */
int amdgv_radix_tree_insert(void *root, u64 index, void *item)
{
	struct radix_tree_root *r = (struct radix_tree_root *)root;
	struct radix_tree_node *node;
	void **slot;
	int error;

	/* Item must not look like an internal node */
	if (!item || is_internal_node(item))
		return -RAS_CORE_EINVAL;

	error = radix_tree_create(r, index, &node, &slot);
	if (error)
		return error;

	if (*slot != NULL)
		return -RAS_CORE_EEXIST;

	*slot = item;

	if (node) {
		u32 offset = slot - node->slots;
		node->count++;
		/* Verify no tags are set on new slot */
		(void)offset;  /* Used for assertion in debug builds */
	}

	return 0;
}

/**
 * amdgv_radix_tree_lookup - Look up an item by index
 * @root: Pointer to radix_tree_root
 * @index: Index to look up
 *
 * Returns: The item at the given index, or NULL if not found
 */
void *amdgv_radix_tree_lookup(void *root, u64 index)
{
	struct radix_tree_root *r = (struct radix_tree_root *)root;
	struct radix_tree_node *node;
	struct radix_tree_node *parent;
	u64 maxindex;
	void **slot;

	parent = NULL;
	slot = (void **)&r->rnode;
	radix_tree_load_root(r, &node, &maxindex);
	if (index > maxindex)
		return NULL;

	while (is_internal_node(node)) {
		u32 offset;

		parent = entry_to_node(node);
		offset = radix_tree_descend(parent, &node, index);
		slot = parent->slots + offset;
	}

	return node;
}

/**
 * amdgv_radix_tree_delete - Delete an item from the tree
 * @root: Pointer to radix_tree_root
 * @index: Index of item to delete
 *
 * Returns: The deleted item, or NULL if not found
 */
void *amdgv_radix_tree_delete(void *root, u64 index)
{
	struct radix_tree_root *r = (struct radix_tree_root *)root;
	struct radix_tree_node *node;
	struct radix_tree_node *parent;
	u64 maxindex;
	void **slot;
	void *entry;
	u32 offset;
	u32 tag;

	parent = NULL;
	slot = (void **)&r->rnode;
	radix_tree_load_root(r, &node, &maxindex);
	if (index > maxindex)
		return NULL;

	while (is_internal_node(node)) {
		parent = entry_to_node(node);
		offset = radix_tree_descend(parent, &node, index);
		slot = parent->slots + offset;
	}

	if (!node)
		return NULL;

	entry = node;

	if (!parent) {
		root_tag_clear_all(r);
		r->rnode = NULL;
		return entry;
	}

	offset = slot - parent->slots;

	/* Clear all tags associated with the item to be deleted */
	for (tag = 0; tag < RADIX_TREE_MAX_TAGS; tag++)
		node_tag_clear(r, parent, tag, offset);

	parent->slots[offset] = NULL;
	parent->count--;

	radix_tree_delete_node(r, parent);

	return entry;
}

/**
 * amdgv_radix_tree_tag_set - Set a tag on an item
 * @root: Pointer to radix_tree_root
 * @index: Index of item
 * @tag: Tag index to set
 *
 * Returns: The item at the index, or NULL if not found
 */
void *amdgv_radix_tree_tag_set(void *root, u64 index, u32 tag)
{
	struct radix_tree_root *r = (struct radix_tree_root *)root;
	struct radix_tree_node *node;
	struct radix_tree_node *parent;
	u64 maxindex;

	if (tag >= RADIX_TREE_MAX_TAGS)
		return NULL;

	radix_tree_load_root(r, &node, &maxindex);
	if (index > maxindex)
		return NULL;

	while (is_internal_node(node)) {
		u32 offset;

		parent = entry_to_node(node);
		offset = radix_tree_descend(parent, &node, index);

		if (!tag_get(parent, tag, offset))
			tag_set(parent, tag, offset);
	}

	/* Set the root's tag bit */
	if (!root_tag_get(r, tag))
		root_tag_set(r, tag);

	return node;
}

/**
 * amdgv_radix_tree_tag_clear - Clear a tag on an item
 * @root: Pointer to radix_tree_root
 * @index: Index of item
 * @tag: Tag index to clear
 *
 * Returns: The item at the index, or NULL if not found
 */
void *amdgv_radix_tree_tag_clear(void *root, u64 index, u32 tag)
{
	struct radix_tree_root *r = (struct radix_tree_root *)root;
	struct radix_tree_node *node;
	struct radix_tree_node *parent;
	u64 maxindex;
	int offset = 0;

	if (tag >= RADIX_TREE_MAX_TAGS)
		return NULL;

	radix_tree_load_root(r, &node, &maxindex);
	if (index > maxindex)
		return NULL;

	parent = NULL;

	while (is_internal_node(node)) {
		parent = entry_to_node(node);
		offset = radix_tree_descend(parent, &node, index);
	}

	if (node)
		node_tag_clear(r, parent, tag, offset);

	return node;
}

/**
 * amdgv_radix_tree_gang_lookup_tag - Look up multiple tagged items
 * @root: Pointer to radix_tree_root
 * @results: Array to store found items
 * @first_index: Starting index
 * @max_items: Maximum number of items to return
 * @tag: Tag to search for
 *
 * Returns: Number of items found
 */
u32 amdgv_radix_tree_gang_lookup_tag(void *root, void **results,
		u64 first_index, u32 max_items, u32 tag)
{
	struct radix_tree_root *r = (struct radix_tree_root *)root;
	struct radix_tree_iter iter;
	void **slot;
	u32 count = 0;

	if (!max_items || tag >= RADIX_TREE_MAX_TAGS)
		return 0;

	if (!root_tag_get(r, tag))
		return 0;

	slot = amdgv_radix_tree_iter_init(&iter, first_index);

	while (count < max_items) {
		slot = amdgv_radix_tree_next_chunk(r, &iter,
				RADIX_TREE_ITER_TAGGED | tag);
		if (!slot)
			break;

		do {
			void *item = *slot;
			if (item && !is_internal_node(item)) {
				results[count++] = item;
				if (count >= max_items)
					break;
			}
			slot = amdgv_radix_tree_next_slot(slot, &iter,
					RADIX_TREE_ITER_TAGGED);
		} while (slot);
	}

	return count;
}

/**
 * amdgv_radix_tree_iter_init - Initialize an iterator
 * @iter: Pointer to radix_tree_iter
 * @start: Starting index
 *
 * Returns: NULL (iteration must call next_chunk first)
 */
void **amdgv_radix_tree_iter_init(void *iter, u64 start)
{
	struct radix_tree_iter *it = (struct radix_tree_iter *)iter;

	it->index = 0;
	it->next_index = start;
	it->tags = 0;

	return NULL;
}

/**
 * amdgv_radix_tree_next_chunk - Find next chunk of slots
 * @root: Pointer to radix_tree_root
 * @iter: Pointer to radix_tree_iter
 * @flags: Iteration flags
 *
 * Returns: Pointer to first slot in chunk, or NULL if no more
 */
void **amdgv_radix_tree_next_chunk(void *root, void *iter, unsigned int flags)
{
	struct radix_tree_root *r = (struct radix_tree_root *)root;
	struct radix_tree_iter *it = (struct radix_tree_iter *)iter;
	struct radix_tree_node *node;
	struct radix_tree_node *child;
	u64 index, maxindex;
	u32 offset;
	u32 tag = flags & RADIX_TREE_ITER_TAG_MASK;
	int tagged = (flags & RADIX_TREE_ITER_TAGGED) != 0;
	int check_sibling_slots = 0;

	if (tagged && !root_tag_get(r, tag))
		return NULL;

	/*
	 * Catch next_index overflow after ~0UL. iter->index never overflows
	 * during iterating; it can be zero only at the beginning.
	 */
	index = it->next_index;
	if (!index && it->index)
		return NULL;

	for (;;) {
		radix_tree_load_root(r, &child, &maxindex);
		if (index > maxindex)
			return NULL;
		if (!child)
			return NULL;

		if (!is_internal_node(child)) {
			/* Single-slot tree */
			it->index = index;
			it->next_index = maxindex + 1;
			it->tags = 1;
			return (void **)&r->rnode;
		}

		do {
			node = entry_to_node(child);
			offset = radix_tree_descend(node, &child, index);

			if ((tagged ? !tag_get(node, tag, offset) : !child)) {
				/* Hole detected */
				if (flags & RADIX_TREE_ITER_CONTIG)
					return NULL;

				if (tagged)
					offset = (u32)radix_tree_find_next_bit(
							node->tags[tag],
							RADIX_TREE_MAP_SIZE,
							offset + 1);
				else
					while (++offset < RADIX_TREE_MAP_SIZE) {
						void *slot = node->slots[offset];
						if (slot)
							break;
					}

				index &= ~node_maxindex(node);
				index += (u64)offset << node->shift;
				/* Overflow after ~0UL */
				if (!index)
					return NULL;
				if (offset == RADIX_TREE_MAP_SIZE) {
					check_sibling_slots = 1;
					break;
				}
				child = node->slots[offset];
			}

			if (child == NULL) {
				check_sibling_slots = 1;
				break;
			}
		} while (is_internal_node(child));

		if (!check_sibling_slots) {
			break;
		} else {
			check_sibling_slots = 0;
		}
	};

	/* Update the iterator state */
	it->index = (index & ~node_maxindex(node)) | ((u64)offset << node->shift);
	it->next_index = (index | node_maxindex(node)) + 1;

	/* Construct iter->tags bit-mask from node->tags[tag] array */
	if (tagged) {
		u32 tag_long, tag_bit;

		tag_long = offset / BITS_PER_LONG;
		tag_bit = offset % BITS_PER_LONG;
		it->tags = node->tags[tag][tag_long] >> tag_bit;
		/* This never happens if RADIX_TREE_TAG_LONGS == 1 */
		if (tag_long < RADIX_TREE_TAG_LONGS - 1) {
			/* Pick tags from next element */
			if (tag_bit)
				it->tags |= node->tags[tag][tag_long + 1] <<
						(BITS_PER_LONG - tag_bit);
			/* Clip chunk size, here only BITS_PER_LONG tags */
			it->next_index = index + BITS_PER_LONG;
		}
	}

	return node->slots + offset;
}

/**
 * amdgv_radix_tree_next_slot - Advance to next slot in current chunk
 * @slot: Current slot pointer
 * @iter: Pointer to radix_tree_iter
 * @flags: Iteration flags
 *
 * Returns: Pointer to next slot, or NULL if chunk is exhausted
 */
void **amdgv_radix_tree_next_slot(void **slot, void *iter, unsigned int flags)
{
	struct radix_tree_iter *it = (struct radix_tree_iter *)iter;
	int tagged = (flags & RADIX_TREE_ITER_TAGGED) != 0;

	if (!slot)
		return NULL;

	if (tagged) {
		it->tags >>= 1;
		if (!it->tags)
			return NULL;

		if (it->tags & 1ul) {
			it->index++;
			return slot + 1;
		}

		if (!(flags & RADIX_TREE_ITER_CONTIG)) {
			u32 offset = find_first_set_bit(it->tags);

			it->tags >>= offset;
			it->index += offset + 1;
			return slot + offset + 1;
		}
	} else {
		u64 count = it->next_index - it->index;

		while (--count > 0) {
			slot++;
			it->index++;

			if (*slot)
				return slot;
			if (flags & RADIX_TREE_ITER_CONTIG) {
				/* Forbid switching to the next chunk */
				it->next_index = 0;
				break;
			}
		}
	}

	return NULL;
}

/**
 * amdgv_radix_tree_deref_slot - Dereference a slot pointer
 * @slot: Slot pointer from iteration
 *
 * Returns: The item stored in the slot
 */
void *amdgv_radix_tree_deref_slot(void **slot)
{
	if (!slot)
		return NULL;
	return *slot;
}

/**
 * amdgv_radix_tree_delete_iter - Delete item at current iterator position
 * @root: Pointer to radix_tree_root
 * @iter: Pointer to radix_tree_iter
 *
 * Returns: The deleted item, or NULL if not found
 */
void *amdgv_radix_tree_delete_iter(void *root, void *iter)
{
	struct radix_tree_iter *it = (struct radix_tree_iter *)iter;

	return amdgv_radix_tree_delete(root, it->index);
}
