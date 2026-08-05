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
 * @file amdgv_ras_mempool.h
 * @brief Fixed-size memory pool for pre-allocated buffer management.
 *
 * This module provides a thread-safe memory pool implementation that
 * pre-allocates a fixed number of equally-sized memory blocks. This is
 * useful for scenarios requiring guaranteed memory availability without
 * runtime allocation failures.
 */

#ifndef _AMDGV_RAS_MEMPOOL_H_
#define _AMDGV_RAS_MEMPOOL_H_

#include "ras_sys.h"

/**
 * struct amdgv_mempool - Fixed-size memory pool descriptor
 * @lock:             Spinlock protecting pool operations
 * @capacity:         Total number of element slots in the pool
 * @count:            Current number of available elements
 * @elem_size:        Size in bytes of each element
 * @free_stack:       Array of pointers to available elements (used as a stack)
 * @bulk_allocation:  Single bulk memory allocation backing all elements
 */
struct amdgv_mempool {
	oss_spinlock_t lock;
	int capacity;
	int count;
	unsigned long elem_size;
	void **free_stack;
	void *bulk_allocation;
};

typedef struct amdgv_mempool mempool_t;

void *amdgv_mempool_create_kmalloc_pool(int element_nr,
		unsigned long element_size);
void amdgv_mempool_destroy(void *pool);
void *amdgv_mempool_alloc_preallocated(void *pool);
void amdgv_mempool_free(void *element, void *pool);

#endif /* _AMDGV_RAS_MEMPOOL_H_ */
