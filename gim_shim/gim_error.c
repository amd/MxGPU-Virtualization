/*
 * Copyright (c) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
 */

#include <linux/slab.h>

#include "gim.h"
#include "gim_error.h"

#define BUFFER_SIZE 100

int gim_error_ring_buffer_init(struct gim_error_ring_buffer **rb)
{
	if (rb == NULL)
		return GIM_FAILURE;

	(*rb) = kzalloc(sizeof(struct gim_error_ring_buffer), GFP_KERNEL);
	if ((*rb) == NULL)
		goto fini;

	(*rb)->read_lock = kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if ((*rb)->read_lock == NULL)
		goto fini;
	mutex_init((*rb)->read_lock);

	(*rb)->write_lock = kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if ((*rb)->write_lock == NULL)
		goto fini;
	mutex_init((*rb)->write_lock);

	return 0;

fini:
	printk(KERN_ERR GIM_ERROR_PRINT_HEADER "%s\n",
		__func__, __LINE__,
		"Cannot init error ring buffer");

	gim_error_ring_buffer_fini(rb);
	return GIM_FAILURE;
}

void gim_error_ring_buffer_fini(struct gim_error_ring_buffer **rb)
{
	if (rb == NULL || (*rb) == NULL)
		return;

	if ((*rb)->read_lock) {
		mutex_destroy((*rb)->read_lock);
		kfree((*rb)->read_lock);
	}

	if ((*rb)->write_lock) {
		mutex_destroy((*rb)->write_lock);
		kfree((*rb)->write_lock);
	}

	kfree(*rb);
	*rb = NULL;
}

/*
 * Beware the rb->write_count overflow problem
 *
 * Since we store the write_count before the lock,
 * if suddenly multiple gim_error_write_buffers are called
 * and they overflow the ring buffer,
 * the read data will be corrupted and we will report overflow
 * in the next read
 *
 * Although this scenario is unlikely to happen as long
 * as we consume the buffer quickly enough within this call
 */
int gim_error_read_buffer(struct gim_error_ring_buffer *rb,
		struct gim_error_entry *entry)
{
	uint32_t index;
	uint32_t wr_diff;
	uint32_t write_count;

	if (rb == NULL || entry == NULL)
		return GIM_FAILURE;

	memset(entry, 0, sizeof(struct gim_error_entry));

	write_count = rb->write_count;

	mutex_lock(rb->read_lock);

	wr_diff = write_count - rb->read_count;
	if (wr_diff == 0)
		goto unlock;

	/* check overflow */
	if (wr_diff > GIM_ERROR_BUF_SIZE) {
		entry->timestamp  = (uint64_t)ktime_to_us(ktime_get());
		entry->error_code = AMDGV_ERROR_DRIVER_BUFFER_OVERFLOW;
		entry->error_data = wr_diff - GIM_ERROR_BUF_SIZE;

		rb->read_count = write_count - GIM_ERROR_BUF_SIZE;
	} else {
		index = GIM_ERROR_BUF_IDX(rb->read_count);
		*entry = rb->buffer[index];
		rb->read_count++;
	}

unlock:
	mutex_unlock(rb->read_lock);

	return 0;
}

int gim_error_write_buffer(struct gim_error_ring_buffer *rb,
		struct gim_error_entry *entry)
{
	uint32_t index;

	if (rb == NULL || entry == NULL)
		return GIM_FAILURE;

	// allow over-written
	mutex_lock(rb->write_lock);

	index = GIM_ERROR_BUF_IDX(rb->write_count);
	rb->buffer[index] = *entry;
	rb->write_count++;

	mutex_unlock(rb->write_lock);

	return 0;
}

int gim_put_error_int(struct gim_error_ring_buffer *rb,
		uint32_t error_code,
		uint64_t error_data,
		const char *func_name,
		uint32_t line_num)
{
	struct gim_error_entry entry = {0};
	char print_buf[BUFFER_SIZE];

	if (rb) {
		entry.timestamp = (uint64_t)ktime_to_us(ktime_get());
		entry.error_code = error_code;
		entry.error_data = error_data;
		gim_error_write_buffer(rb, &entry);
	}

	amdgv_error_get_error_text(error_code, error_data,
				print_buf, BUFFER_SIZE);

	printk(KERN_ERR GIM_ERROR_PRINT_HEADER "%s\n",
		func_name, line_num, print_buf);

	return 0;
}
