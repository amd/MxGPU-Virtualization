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
 * @file amdgv_ras_fifo.h
 * @brief A circular buffer (FIFO) implementation for RAS subsystem.
 *
 * This implementation provides a thread-safe producer-consumer
 * circular buffer using spinlock, power-of-2 sizing and index wrapping techniques.
 */

#ifndef _AMDGV_FIFO_H_
#define _AMDGV_FIFO_H_

#include "ras_sys.h"

/**
 * struct amdgv_fifo - circular buffer structure
 * @buffer: pointer to the data storage area
 * @size: total size of the buffer in bytes (must be power of 2)
 * @in: write index (data is added at buffer[in % size])
 * @out: read index (data is extracted from buffer[out % size])
 *
 * The buffer uses modular arithmetic on indices to avoid explicit
 * wraparound handling. The size must be a power of 2 so that
 * (index & (size - 1)) gives the actual buffer offset.
 */
struct amdgv_fifo {
	unsigned char *buffer;
	unsigned int size;
	unsigned int in;
	unsigned int out;
};

int amdgv_fifo_alloc(void *fifo, uint32_t size);
int amdgv_fifo_in_spinlocked_raw(void *fifo, void *buf, uint32_t size, void *lock);
int amdgv_fifo_out_spinlocked_raw(void *fifo, void *buf, uint32_t size, void *lock);
unsigned int amdgv_fifo_out_peek(void *fifo, void *buf, unsigned int n);
unsigned int amdgv_fifo_len(void *fifo);
void amdgv_fifo_free(void *fifo);

#endif /* _AMDGV_FIFO_H_ */
