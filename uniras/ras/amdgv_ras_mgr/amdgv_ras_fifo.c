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
 * @file amdgv_ras_fifo.c
 * @brief Circular buffer (FIFO) implementation for RAS subsystem.
 *
 * This is a clean-room implementation of a circular buffer using standard
 * ring buffer algorithms. It provides both lock-free single-producer
 * single-consumer access and spinlock-protected variants.
 */

#if !defined(SHIM_LAYER_OSS_KFIFO)
#define AMDGV_RAS_PORTABLE_OSS_KFIFO_IMPL 1
#endif

#include "ras_core_status.h"
#include "amdgv_ras_fifo.h"

#ifdef AMDGV_RAS_PORTABLE_OSS_KFIFO_IMPL

/*
 * ============================================================================
 * Internal Helper Functions
 * ============================================================================
 */

/**
 * is_power_of_two - Check if a value is a power of 2
 * @val: value to check
 *
 * Return: true if val is a power of 2, false otherwise
 */
static inline int is_power_of_two(unsigned int val)
{
	return (val != 0) && ((val & (val - 1)) == 0);
}

/**
 * count_leading_zeros - Count leading zeros in a 32-bit value
 * @val: value to analyze
 *
 * Return: number of leading zero bits (0-32)
 */
static inline unsigned int count_leading_zeros(unsigned int val)
{
	unsigned int count = 0;
	unsigned int mask = 0x80000000U;

	if (val == 0)
		return 32;

	while ((val & mask) == 0) {
		count++;
		mask >>= 1;
	}

	return count;
}

/**
 * round_up_to_power_of_two - Round up to the next power of 2
 * @val: value to round up
 *
 * Return: smallest power of 2 >= val, or 1 if val is 0
 */
static inline unsigned int round_up_to_power_of_two(unsigned int val)
{
	unsigned int shift;

	if (val == 0)
		return 1;

	if (is_power_of_two(val))
		return val;

	/* Find position of highest set bit and round up */
	shift = 32 - count_leading_zeros(val);

	/* Prevent overflow for very large values */
	if (shift >= 32)
		return 0x80000000U;

	return 1U << shift;
}

/**
 * compute_buffer_index - Compute actual buffer index from logical index
 * @fifo: the circular buffer
 * @logical_index: the logical (possibly wrapped) index
 *
 * Uses bitwise AND with (size - 1) for efficient modulo operation.
 * This works because size is guaranteed to be a power of 2.
 *
 * Return: actual offset into the buffer
 */
static inline unsigned int compute_buffer_index(struct amdgv_fifo *fifo,
						unsigned int logical_index)
{
	return logical_index & (fifo->size - 1);
}

/**
 * get_used_space - Get number of bytes currently stored in the buffer
 * @fifo: the circular buffer
 *
 * Return: number of bytes available for reading
 */
static inline unsigned int get_used_space(struct amdgv_fifo *fifo)
{
	return fifo->in - fifo->out;
}

/**
 * get_free_space - Get number of bytes available for writing
 * @fifo: the circular buffer
 *
 * Return: number of bytes available for writing
 */
static inline unsigned int get_free_space(struct amdgv_fifo *fifo)
{
	return fifo->size - get_used_space(fifo);
}

/**
 * copy_to_buffer - Copy data into the circular buffer
 * @fifo: the circular buffer
 * @src: source data pointer
 * @len: number of bytes to copy
 * @write_offset: additional offset from current write position
 *
 * Handles wraparound by splitting the copy into two parts if necessary:
 * first from current position to end of buffer, then from start of buffer.
 */
static void copy_to_buffer(struct amdgv_fifo *fifo, const void *src,
			   unsigned int len, unsigned int write_offset)
{
	unsigned int buffer_offset;
	unsigned int bytes_to_end;
	unsigned int first_chunk;
	const unsigned char *src_ptr = (const unsigned char *)src;

	buffer_offset = compute_buffer_index(fifo, fifo->in + write_offset);

	/* Calculate bytes from current position to end of buffer */
	bytes_to_end = fifo->size - buffer_offset;

	/* Determine size of first chunk (up to end of buffer) */
	if (len <= bytes_to_end) {
		first_chunk = len;
	} else {
		first_chunk = bytes_to_end;
	}

	/* Copy first chunk: from current position toward end of buffer */
	oss_memcpy(fifo->buffer + buffer_offset, src_ptr, first_chunk);

	/* Copy second chunk: wraparound to beginning of buffer */
	if (len > first_chunk) {
		oss_memcpy(fifo->buffer, src_ptr + first_chunk, len - first_chunk);
	}
}

/**
 * copy_from_buffer - Copy data from the circular buffer
 * @fifo: the circular buffer
 * @dst: destination data pointer
 * @len: number of bytes to copy
 * @read_offset: additional offset from current read position
 *
 * Handles wraparound by splitting the copy into two parts if necessary:
 * first from current position to end of buffer, then from start of buffer.
 */
static void copy_from_buffer(struct amdgv_fifo *fifo, void *dst,
			     unsigned int len, unsigned int read_offset)
{
	unsigned int buffer_offset;
	unsigned int bytes_to_end;
	unsigned int first_chunk;
	unsigned char *dst_ptr = (unsigned char *)dst;

	buffer_offset = compute_buffer_index(fifo, fifo->out + read_offset);

	/* Calculate bytes from current position to end of buffer */
	bytes_to_end = fifo->size - buffer_offset;

	/* Determine size of first chunk (up to end of buffer) */
	if (len <= bytes_to_end) {
		first_chunk = len;
	} else {
		first_chunk = bytes_to_end;
	}

	/* Copy first chunk: from current position toward end of buffer */
	oss_memcpy(dst_ptr, fifo->buffer + buffer_offset, first_chunk);

	/* Copy second chunk: wraparound from beginning of buffer */
	if (len > first_chunk) {
		oss_memcpy(dst_ptr + first_chunk, fifo->buffer, len - first_chunk);
	}
}

/*
 * ============================================================================
 * Core FIFO Operations
 * ============================================================================
 */

/**
 * fifo_write - Write data into the circular buffer
 * @fifo: the circular buffer
 * @src: source data to write
 * @len: number of bytes to write
 *
 * Writes up to @len bytes into the buffer. If there is not enough space,
 * only the available space is used.
 *
 * Return: number of bytes actually written
 */
static unsigned int fifo_write(struct amdgv_fifo *fifo, const void *src,
			       unsigned int len)
{
	unsigned int available;

	/* Limit write to available space */
	available = get_free_space(fifo);
	if (len > available)
		len = available;

	if (len == 0)
		return 0;

	copy_to_buffer(fifo, src, len, 0);
	fifo->in += len;

	return len;
}

/**
 * fifo_read - Read and remove data from the circular buffer
 * @fifo: the circular buffer
 * @dst: destination buffer for the data
 * @len: maximum number of bytes to read
 *
 * Reads up to @len bytes from the buffer and removes them.
 *
 * Return: number of bytes actually read
 */
static unsigned int fifo_read(struct amdgv_fifo *fifo, void *dst, unsigned int len)
{
	unsigned int stored;

	/* Limit read to stored data */
	stored = get_used_space(fifo);
	if (len > stored)
		len = stored;

	if (len == 0)
		return 0;

	copy_from_buffer(fifo, dst, len, 0);
	fifo->out += len;

	return len;
}

/**
 * fifo_peek - Read data from buffer without removing it
 * @fifo: the circular buffer
 * @dst: destination buffer for the data
 * @len: maximum number of bytes to read
 * @offset: offset from read position to start reading
 *
 * Reads up to @len bytes starting at @offset from the read position.
 * The data remains in the buffer.
 *
 * Return: number of bytes actually read
 */
static unsigned int fifo_peek(struct amdgv_fifo *fifo, void *dst,
			      unsigned int len, unsigned int offset)
{
	unsigned int stored;
	unsigned int available_from_offset;

	stored = get_used_space(fifo);

	/* Check if offset is within stored data */
	if (offset >= stored)
		return 0;

	/* Calculate available bytes from the offset position */
	available_from_offset = stored - offset;
	if (len > available_from_offset)
		len = available_from_offset;

	if (len == 0)
		return 0;

	copy_from_buffer(fifo, dst, len, offset);

	return len;
}

/*
 * ============================================================================
 * FIFO Lifecycle Management
 * ============================================================================
 */

/**
 * fifo_init - Initialize a circular buffer with an existing memory region
 * @fifo: the circular buffer structure to initialize
 * @buffer: pre-allocated memory region (or NULL to mark as uninitialized)
 * @size: size of the buffer in bytes
 */
static void fifo_init(struct amdgv_fifo *fifo, void *buffer, unsigned int size)
{
	fifo->buffer = buffer;
	fifo->size = size;
	fifo->in = 0;
	fifo->out = 0;
}

/**
 * fifo_alloc - Allocate and initialize a new circular buffer
 * @fifo: the circular buffer structure to initialize
 * @size: requested size (will be rounded up to power of 2)
 *
 * Allocates memory for the buffer and initializes the structure.
 * The size is rounded up to the next power of 2 for efficient
 * index calculations.
 *
 * Return: 0 on success, negative error code on failure
 */
static int fifo_alloc(struct amdgv_fifo *fifo, unsigned int size)
{
	unsigned char *buffer;
	unsigned int actual_size;

	if (size == 0) {
		fifo_init(fifo, NULL, 0);
		return -RAS_CORE_EINVAL;
	}

	/* Round up to power of 2 for efficient modulo operations */
	if (!is_power_of_two(size))
		actual_size = round_up_to_power_of_two(size);
	else
		actual_size = size;

	/* Allocate zero-initialized buffer */
	buffer = oss_zalloc(actual_size);
	if (buffer == NULL) {
		fifo_init(fifo, NULL, 0);
		return -RAS_CORE_ENOMEM;
	}

	fifo_init(fifo, buffer, actual_size);

	return 0;
}

/**
 * fifo_free - Free a circular buffer's memory
 * @fifo: the circular buffer to free
 *
 * Releases the buffer memory and resets the structure.
 */
static void fifo_free(struct amdgv_fifo *fifo)
{
	if (fifo->buffer != NULL) {
		oss_free(fifo->buffer);
	}
	fifo_init(fifo, NULL, 0);
}

/*
 * ============================================================================
 * Spinlock-Protected Operations
 * ============================================================================
 */

/**
 * fifo_write_locked - Write data with spinlock protection
 * @fifo: the circular buffer
 * @src: source data to write
 * @len: number of bytes to write
 * @lock: spinlock for synchronization
 *
 * Thread-safe variant of fifo_write using a spinlock.
 *
 * Return: number of bytes actually written
 */
static unsigned int fifo_write_locked(struct amdgv_fifo *fifo, const void *src,
				      unsigned int len, oss_spinlock_t *lock)
{
	unsigned long flags = 0;
	unsigned int written;

	oss_spin_lock_irqsave_raw(lock, flags);
	written = fifo_write(fifo, src, len);
	oss_spin_unlock_irqrestore_raw(lock, flags);

	return written;
}

/**
 * fifo_read_locked - Read data with spinlock protection
 * @fifo: the circular buffer
 * @dst: destination buffer for the data
 * @len: maximum number of bytes to read
 * @lock: spinlock for synchronization
 *
 * Thread-safe variant of fifo_read using a spinlock.
 *
 * Return: number of bytes actually read
 */
static unsigned int fifo_read_locked(struct amdgv_fifo *fifo, void *dst,
				     unsigned int len, oss_spinlock_t *lock)
{
	unsigned long flags = 0;
	unsigned int bytes_read;

	oss_spin_lock_irqsave_raw(lock, flags);
	bytes_read = fifo_read(fifo, dst, len);
	oss_spin_unlock_irqrestore_raw(lock, flags);

	return bytes_read;
}

/*
 * ============================================================================
 * Public API - AMDGV_FIFO Functions
 * ============================================================================
 */

/**
 * amdgv_fifo_alloc - Allocate a new circular buffer
 * @fifo: pointer to AMDGV_FIFO structure
 * @size: requested size in bytes
 *
 * Return: 0 on success, negative error code on failure
 */
int amdgv_fifo_alloc(void *fifo, uint32_t size)
{
	return fifo_alloc((struct amdgv_fifo *)fifo, size);
}

/**
 * amdgv_fifo_in_spinlocked_raw - Write data with spinlock protection
 * @fifo: pointer to AMDGV_FIFO structure
 * @buf: source data buffer
 * @size: number of bytes to write
 * @lock: spinlock for synchronization
 *
 * Return: number of bytes written
 */
int amdgv_fifo_in_spinlocked_raw(void *fifo, void *buf, uint32_t size, void *lock)
{
	return (int)fifo_write_locked((struct amdgv_fifo *)fifo, buf, size,
				      (oss_spinlock_t *)lock);
}

/**
 * amdgv_fifo_out_spinlocked_raw - Read data with spinlock protection
 * @fifo: pointer to AMDGV_FIFO structure
 * @buf: destination data buffer
 * @size: maximum number of bytes to read
 * @lock: spinlock for synchronization
 *
 * Return: number of bytes read
 */
int amdgv_fifo_out_spinlocked_raw(void *fifo, void *buf, uint32_t size, void *lock)
{
	return (int)fifo_read_locked((struct amdgv_fifo *)fifo, buf, size,
				     (oss_spinlock_t *)lock);
}

/**
 * amdgv_fifo_out_peek - Peek data without removing it
 * @fifo: pointer to AMDGV_FIFO structure
 * @buf: destination data buffer
 * @n: maximum number of bytes to peek
 *
 * Return: number of bytes copied to buf
 */
unsigned int amdgv_fifo_out_peek(void *fifo, void *buf, unsigned int n)
{
	return fifo_peek((struct amdgv_fifo *)fifo, buf, n, 0);
}

/**
 * amdgv_fifo_len - Get number of bytes stored in buffer
 * @fifo: pointer to AMDGV_FIFO structure
 *
 * Return: number of bytes currently stored
 */
unsigned int amdgv_fifo_len(void *fifo)
{
	return get_used_space((struct amdgv_fifo *)fifo);
}

/**
 * amdgv_fifo_free - Free a circular buffer
 * @fifo: pointer to AMDGV_FIFO structure
 */
void amdgv_fifo_free(void *fifo)
{
	fifo_free((struct amdgv_fifo *)fifo);
}

#else /* AMDGV_RAS_PORTABLE_OSS_KFIFO_IMPL */

/* Host OSS vtable + amdgv_oss_wrapper.h supply oss_kfifo_*. */

#endif /* AMDGV_RAS_PORTABLE_OSS_KFIFO_IMPL */
