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

/**
 * @file amdgv_ras_mempool.c
 * @brief Fixed-size memory pool implementation.
 *
 * Provides a thread-safe pool of pre-allocated memory blocks. All blocks
 * are allocated at pool creation time to guarantee availability during
 * runtime operations. The pool uses a stack-based free list for O(1)
 * allocation and deallocation.
 */

#if !defined(SHIM_LAYER_OSS_MEMPOOL)
#define AMDGV_RAS_PORTABLE_OSS_MEMPOOL_IMPL 1
#endif

#include "ras_core_status.h"
#include "amdgv_ras_mempool.h"

#ifdef AMDGV_RAS_PORTABLE_OSS_MEMPOOL_IMPL

/* Signature written to freed elements to detect double-free errors */
#define MEMPOOL_FREED_SIGNATURE  0xFEEDDEADU

/**
 * ras_mempool_assert_cond - Log and return 0 if condition is false (MSVC-safe;
 * avoids GCC statement-expression ({ ... }) used in older mempool_assert).
 */
static inline int ras_mempool_assert_cond(int cond_ok, const char *file, int line,
					  const char *fn, const char *expr)
{
	if (!cond_ok) {
		RAS_INFO("%s(%d): %s assertion failed: (%s)\n", file, line, fn, expr);
		oss_dump_stack();
		return 0;
	}
	return 1;
}

#define mempool_assert(cond) \
	ras_mempool_assert_cond((int)!!(cond), __FILE__, __LINE__, __func__, #cond)

/**
 * mempool_push - Add an element to the free stack
 * @pool: Memory pool
 * @elem: Element to return to the pool
 *
 * Caller must hold pool->lock. Asserts if pool is already full.
 */
static inline void mempool_push(struct amdgv_mempool *pool, void *elem)
{
	if (!mempool_assert(pool->count < pool->capacity))
		return;
	pool->free_stack[pool->count] = elem;
	pool->count++;
}

/**
 * mempool_pop - Remove an element from the free stack
 * @pool: Memory pool
 *
 * Caller must hold pool->lock. Asserts if pool is empty.
 *
 * Return: Pointer to an available element
 */
static inline void *mempool_pop(struct amdgv_mempool *pool)
{
	if (!mempool_assert(pool->count > 0))
		return NULL;
	pool->count--;
	return pool->free_stack[pool->count];
}

/**
 * mempool_release_all - Free all elements and the free stack
 * @pool: Memory pool to clean up
 *
 * Frees the bulk allocation and free_stack array.
 * Does not free the pool structure itself.
 */
static void mempool_release_all(struct amdgv_mempool *pool)
{
	pool->count = 0;

	if (pool->bulk_allocation) {
		oss_free_memory(pool->bulk_allocation);
		pool->bulk_allocation = NULL;
	}

	if (pool->free_stack) {
		oss_free(pool->free_stack);
		pool->free_stack = NULL;
	}
}

/**
 * mempool_init - Initialize and populate a memory pool
 * @pool:      Pool structure to initialize
 * @num_elems: Number of elements to pre-allocate
 * @elem_size: Size of each element in bytes
 *
 * Allocates the free stack and all elements in a single bulk allocation.
 * On failure, any partially allocated resources are cleaned up.
 *
 * Return: 0 on success, -1 on allocation failure
 */
static int mempool_init(struct amdgv_mempool *pool, int num_elems,
			unsigned long elem_size)
{
	int i;
	char *bulk_mem;
	unsigned long total_size;

	pool->capacity = num_elems;
	pool->elem_size = elem_size;
	pool->count = 0;
	pool->free_stack = NULL;
	pool->bulk_allocation = NULL;

	if (num_elems <= 0 || elem_size == 0)
		return -1;

	if (elem_size > (~0UL) / sizeof(void *))
		return -1;

	if ((unsigned long)num_elems > (~0UL) / elem_size)
		return -1;

	oss_spin_lock_init_raw(&pool->lock);

	/* Allocate the free stack (array of pointers) */
	pool->free_stack = oss_zalloc(num_elems * sizeof(void *));
	if (!pool->free_stack)
		return -1;

	/* Allocate all elements in a single bulk allocation for efficiency */
	total_size = (unsigned long)num_elems * elem_size;
	bulk_mem = oss_alloc_memory(total_size);
	if (!bulk_mem) {
		oss_free(pool->free_stack);
		pool->free_stack = NULL;
		return -1;
	}
	oss_memset(bulk_mem, 0, total_size);
	pool->bulk_allocation = bulk_mem;

	/* Push each element from the bulk allocation onto the free stack */
	for (i = 0; i < num_elems; i++) {
		mempool_push(pool, bulk_mem + (i * elem_size));
	}

	return 0;
}

/**
 * mempool_create - Create a new memory pool
 * @num_elems: Number of elements to pre-allocate
 * @elem_size: Size of each element in bytes
 *
 * Allocates and initializes a new memory pool with the specified number
 * of pre-allocated elements.
 *
 * Return: Pointer to the new pool, or NULL on failure
 */
static struct amdgv_mempool *mempool_create(int num_elems,
					    unsigned long elem_size)
{
	struct amdgv_mempool *pool;

	pool = oss_zalloc(sizeof(*pool));
	if (!pool)
		return NULL;

	if (mempool_init(pool, num_elems, elem_size) != 0) {
		oss_free(pool);
		return NULL;
	}

	return pool;
}

/**
 * mempool_destroy - Destroy a memory pool and free all resources
 * @pool: Pool to destroy (may be NULL)
 *
 * Frees all pooled elements, the free stack, and the pool structure.
 */
static void mempool_destroy(struct amdgv_mempool *pool)
{
	if (!pool)
		return;

	mempool_release_all(pool);
	oss_free(pool);
}

/**
 * mempool_alloc - Allocate an element from the pool
 * @pool: Memory pool
 *
 * Returns a pre-allocated element from the pool. The element is
 * zero-initialized before being returned. This function is thread-safe.
 *
 * Return: Pointer to an element, or NULL if pool is empty or invalid
 */
static void *mempool_alloc(struct amdgv_mempool *pool)
{
	void *elem;
	unsigned long flags = 0;

	if (!pool)
		return NULL;

	oss_spin_lock_irqsave_raw(&pool->lock, flags);

	if (pool->count == 0) {
		oss_spin_unlock_irqrestore_raw(&pool->lock, flags);
		return NULL;
	}

	elem = mempool_pop(pool);
	oss_memset(elem, 0, pool->elem_size);

	oss_spin_unlock_irqrestore_raw(&pool->lock, flags);

	return elem;
}

/**
 * mempool_free_elem - Return an element to the pool
 * @elem: Element to return (may be NULL)
 * @pool: Memory pool (may be NULL)
 *
 * Returns a previously allocated element back to the pool. The element
 * is marked with a signature to detect double-free errors. This function
 * is thread-safe.
 */
static void mempool_free_elem(void *elem, struct amdgv_mempool *pool)
{
	unsigned long flags = 0;
	unsigned int *signature;

	if (!elem || !pool)
		return;

	signature = (unsigned int *)elem;

	oss_spin_lock_irqsave_raw(&pool->lock, flags);

	/* Check for double-free: if signature matches, element was already freed */
	if (*signature == MEMPOOL_FREED_SIGNATURE) {
		oss_spin_unlock_irqrestore_raw(&pool->lock, flags);
		return;
	}

	if (!mempool_assert(pool->count < pool->capacity)) {
		oss_spin_unlock_irqrestore_raw(&pool->lock, flags);
		return;
	}

	/* Mark element as freed */
	*signature = MEMPOOL_FREED_SIGNATURE;
	mempool_push(pool, elem);

	oss_spin_unlock_irqrestore_raw(&pool->lock, flags);
}

/*
 * Public API - AMDGV mempool implementations; ras_sys.h maps oss_mempool_*
 * to these when SHIM_LAYER_OSS_MEMPOOL is not set (portable path).
 */

/**
 * amdgv_mempool_create_kmalloc_pool - Create a memory pool
 * @element_nr:   Number of elements to pre-allocate
 * @element_size: Size of each element in bytes
 *
 * Return: Opaque handle to the pool, or NULL on failure
 */
void *amdgv_mempool_create_kmalloc_pool(int element_nr,
					unsigned long element_size)
{
	return mempool_create(element_nr, element_size);
}

/**
 * amdgv_mempool_destroy - Destroy a memory pool
 * @pool: Pool handle returned by amdgv_mempool_create_kmalloc_pool()
 */
void amdgv_mempool_destroy(void *pool)
{
	mempool_destroy((struct amdgv_mempool *)pool);
}

/**
 * amdgv_mempool_alloc_preallocated - Allocate from pool
 * @pool: Pool handle
 *
 * Return: Pointer to a zero-initialized element, or NULL if unavailable
 */
void *amdgv_mempool_alloc_preallocated(void *pool)
{
	return mempool_alloc((struct amdgv_mempool *)pool);
}

/**
 * amdgv_mempool_free - Return an element to the pool
 * @element: Element to free
 * @pool:    Pool handle
 */
void amdgv_mempool_free(void *element, void *pool)
{
	mempool_free_elem(element, (struct amdgv_mempool *)pool);
}

#else /* AMDGV_RAS_PORTABLE_OSS_MEMPOOL_IMPL */

/* Host OSS vtable + amdgv_oss_wrapper.h supply oss_mempool_*. */

#endif /* AMDGV_RAS_PORTABLE_OSS_MEMPOOL_IMPL */
