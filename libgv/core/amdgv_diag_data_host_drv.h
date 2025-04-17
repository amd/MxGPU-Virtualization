/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_DIAG_DATA_HOST_DRV_H
#define AMDGV_DIAG_DATA_HOST_DRV_H

#include "amdgv_diag_data.h"

/* Total features in trace log block for host driver */
#define AMDGV_DIAG_DATA_TRACE_LOG_FEATURE_TOTAL \
	(AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_COUNT - 1)

#define BUILD_NUM_LEN  6
#define BUILD_DATA_LEN 15

/* Block sizes for the host driver */
#define AMDGV_DIAG_DATA_HOST_TRACE_LOG_BLK_SIZE	(64 * AMDGV_DIAG_DATA_ALIGNMENT)
#define AMDGV_DIAG_DATA_HOST_GENERAL_INFO_BLK_SIZE	(2 * AMDGV_DIAG_DATA_ALIGNMENT)
#define AMDGV_DIAG_DATA_HOST_POST_VBIOS_LOG_BLK_SIZE	(8 * AMDGV_DIAG_DATA_ALIGNMENT)
#define AMDGV_DIAG_DATA_HOST_IH_RING_DUMP_BLK_SIZE	(64 * AMDGV_DIAG_DATA_ALIGNMENT)
#define AMDGV_DIAG_DATA_HOST_ERROR_DUMP_BLK_SIZE	(2 * AMDGV_DIAG_DATA_ALIGNMENT)
#define AMDGV_DIAG_DATA_HOST_INTER_STRUCT_SIZE	(1 * AMDGV_DIAG_DATA_ALIGNMENT)

/* Static check for the host driver memory size
 * Snapshot logs such as AMDGV_DIAG_DATA_HOST_IH_RING_DUMP_BLK_SIZE and
 * AMDGV_DIAG_DATA_HOST_GENERAL_INFO_BLK_SIZE are skip from the total size,
 * as the contents for IH ring and general info block are directly
 * copied to user memory fromm the host driver data and does
 * not require memory to hold
 */
#define HOST_DRIVER_TOTAL_SIZE	(AMDGV_DIAG_DATA_HOST_TRACE_LOG_BLK_SIZE + \
				AMDGV_DIAG_DATA_HOST_POST_VBIOS_LOG_BLK_SIZE + \
				AMDGV_DIAG_DATA_HOST_ERROR_DUMP_BLK_SIZE + \
				AMDGV_DIAG_DATA_HOST_INTER_STRUCT_SIZE)
#if (HOST_DRIVER_TOTAL_SIZE > AMDGV_DIAG_DATA_HOST_DRV_MEM_SIZE)
#error The AMDGV_DIAG_DATA_HOST_DRV_MEM_SIZE is too small to allocate blocks.
#endif

/* Get the inter struct offset */
#define AMDGV_DIAG_DATA_HOST_DRV_INTER_STRUCT_OFFSET \
	(adapt->diag_data.host_drv_buff.vaddr ? \
	(uint8_t *)adapt->diag_data.host_drv_buff.vaddr + \
	(AMDGV_DIAG_DATA_HOST_DRV_MEM_SIZE - \
	AMDGV_DIAG_DATA_HOST_INTER_STRUCT_SIZE) : NULL)

/* Fills in the memory block for the given block */
#define AMDGV_DIAG_DATA_FILL_MEM_BLK(mem_blk, c_addr, BLOCK) { \
	mem_blk.vaddr = (void *)AMDGV_DIAG_DATA_ALIGN(c_addr); \
	mem_blk.size =  AMDGV_DIAG_DATA_ ## BLOCK ## _BLK_SIZE; \
	mem_blk.block_id = AMDGV_DIAG_DATA_BLOCK_ID_ ## BLOCK; \
	mem_blk.used_size = 0; \
	mem_blk.bus_addr = 0; \
	c_addr = (uint64_t)mem_blk.vaddr + mem_blk.size; \
}

/* Keep entries for the ring buffer blocks */
#define AMDGV_DIAG_DATA_ERROR_DUMP_FIRST_KEEP	16
#define AMDGV_DIAG_DATA_VPOST_POST_KEEP_ENTRIES	1

/* Change this version whenever error list changes in amdgv_error */
#define AMDGV_RD_HOST_ERROR_DUMP_MAJOR_VERSION		0x01
#define AMDGV_RD_HOST_ERROR_DUMP_MINOR_VERSION		0x00
#define AMDGV_RD_HOST_ERROR_DUMP_VERSION \
		(AMDGV_RD_HOST_ERROR_DUMP_MINOR_VERSION | \
		AMDGV_RD_HOST_ERROR_DUMP_MAJOR_VERSION << 8)

/* Ring buffer Index calculations */
#define AMDGV_DIAG_DATA_RB_INDEX(wr_count, total_entries) \
	((total_entries & (total_entries - 1)) ? \
		wr_count % total_entries : \
		((wr_count) & (total_entries - 1)))
#define AMDGV_DIAG_DATA_RB_KEEP_INDEX(w_count, total_entries, keep_entries) \
			(((w_count - keep_entries) % \
			(total_entries - keep_entries)) + keep_entries)

/* IH Dump related data */
#define AMDGV_DIAG_DATA_IH_RING_HDR_RESERVED		16
struct amdgv_diag_data_ih_ring_hdr {
	uint32_t ring_size;
	uint32_t ring_size_log2;
	uint32_t rptr;
	uint32_t rptr_offset;
	uint32_t wptr_offset;
	uint32_t doorbell_index;
	uint32_t use_doorbell;
	uint32_t use_bus_addr;
	uint64_t rb_dma_addr;
	uint64_t gpu_addr;
	uint8_t reserved[AMDGV_DIAG_DATA_IH_RING_HDR_RESERVED];
};

/* Trace log related data */
struct amdgv_diag_data_trace_log {
	uint8_t  client;
	uint8_t  feature;
	uint16_t code;
	uint32_t ext_code;
	uint64_t gpu_timestamp;
	uint64_t cpu_timestamp;
};

/* Error Dump related data */
struct amdgv_diag_data_error_dump_entry {
	uint64_t gpu_timestamp;
	uint64_t cpu_timestamp;
	uint64_t error_data;
	uint32_t error_code;
	uint32_t vf_idx;
};

/* VBIOS Post related data */
#define AMDGV_DIAG_DATA_VBIOS_POST_ENTRIES           256
#define AMDGV_DIAG_DATA_VBIOS_OP_LENGTH         24
#define AMDGV_DIAG_DATA_VBIOS_HDR_RESERVED		36
enum AMDGV_DIAG_DATA_VBIOS_OPERAION {
	AMDGV_DIAG_DATA_VBIOS_OP_READ,
	AMDGV_DIAG_DATA_VBIOS_OP_WRITE,

	AMDGV_DIAG_DATA_VBIOS_OP_COUNT
};

struct amdgv_diag_data_vbios_post_header {
	/* Indexes in the table for the specified OP */
	uint32_t cmd_table_idx;
	/* VBIOS post type */
	uint32_t post_type;
	/* PS, WS and base */
	uint32_t ps_size;
	uint32_t ws_size;
	uint32_t len;
	uint32_t base;
	uint32_t code_loc;
	/* To match the size of the entry */
	uint8_t reserved[AMDGV_DIAG_DATA_VBIOS_HDR_RESERVED];
};

struct amdgv_diag_data_vbios_value {
	uint32_t addr;
	uint32_t value;
};

struct amdgv_diag_data_vbios_post_entry {
	/* Index into OP Table */
	uint32_t op;
	/* Location where the op started */
	uint32_t start_loc;
	/* Location where op ended */
	uint32_t end_loc;
	/* Type on which to perform the OP i.e., REG, PCI etc. */
	uint32_t arg;
	/* Register, FB, read/write data */
	struct amdgv_diag_data_vbios_value
		val[AMDGV_DIAG_DATA_VBIOS_OP_COUNT];
	/* Contents of the location i.e., end_loc - start_loc */
	uint8_t data[AMDGV_DIAG_DATA_VBIOS_OP_LENGTH];
	/* time stamp
	 * timestamp should be the last elment as it is not included
	 * in the memory comparison to find the similar entries
	 */
	uint64_t timestamp;
};


struct amdgv_diag_data_host_driver_trace_log {
	uint32_t total_entries;
	uint32_t feature_entries;
	struct amdgv_diag_data_mem_block mem_blk;
	uint32_t w_count[AMDGV_DIAG_DATA_TRACE_LOG_FEATURE_TOTAL];
	uint32_t feature_offset[AMDGV_DIAG_DATA_TRACE_LOG_FEATURE_TOTAL];
	struct amdgv_diag_data_trace_log *entry;
};

struct amdgv_diag_data_error_dump {
	uint32_t size;
	uint32_t error_read_count;
	uint32_t w_count;
	uint32_t total_entries;
	struct amdgv_diag_data_mem_block mem_blk;
	struct amdgv_diag_data_error_dump_entry *entry;
};

struct amdgv_diag_data_vbios_post {
	uint32_t total_entries;
	uint32_t w_count;
	struct amdgv_diag_data_mem_block mem_blk;
	struct amdgv_diag_data_vbios_post_header *hdr;
	struct amdgv_diag_data_vbios_post_entry *entry;
	struct amdgv_diag_data_vbios_post_entry *last_entry;
};

struct amdgv_diag_data_host_drv_blk {
	struct amdgv_diag_data_host_driver_trace_log trace_log;
	struct amdgv_diag_data_error_dump error_dump;
	struct amdgv_diag_data_vbios_post vbios_post_log;
};

/* Internal APIS defined in amdgv_diag_data_host_drv.c */
int amdgv_diag_data_host_driver_collect(
		struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data);
int amdgv_diag_data_host_driver_init(struct amdgv_adapter *adapt);
void amdgv_diag_data_host_driver_fini(struct amdgv_adapter *adapt);

#endif

