/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GIM_ERROR_H
#define GIM_ERROR_H

#include <linux/mutex.h>
#include "amdgv_error.h"

#define GIM_ERROR_BUF_SIZE   128
#define GIM_ERROR_BUF_IDX(c) ((c) & (GIM_ERROR_BUF_SIZE - 1))
#define GIM_ERROR_PRINT_HEADER "[gim][%s:%d] "

#define GIM_FAILURE -1

struct gim_error_entry {
	uint64_t timestamp;
	uint64_t error_data;
	uint32_t error_code;
};

struct gim_error_ring_buffer {
	uint32_t write_count;
	uint32_t read_count;

	struct mutex *read_lock;
	struct mutex *write_lock;

	struct gim_error_entry buffer[GIM_ERROR_BUF_SIZE];
};

#define gim_put_error(code, data) \
	gim_put_error_int(gim_error_rb, code, (uint64_t)data, \
			__func__, __LINE__)

int gim_put_error_int(struct gim_error_ring_buffer *rb,
		uint32_t error_code, uint64_t error_data,
		const char *func_name, uint32_t line_num);

int gim_error_ring_buffer_init(struct gim_error_ring_buffer **rb);
void gim_error_ring_buffer_fini(struct gim_error_ring_buffer **rb);

int gim_error_read_buffer(struct gim_error_ring_buffer *rb,
			struct gim_error_entry *entry);
int gim_error_write_buffer(struct gim_error_ring_buffer *rb,
			struct gim_error_entry *entry);

#endif
