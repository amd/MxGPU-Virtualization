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

#ifndef AMDGV_DIAG_DATA_H
#define AMDGV_DIAG_DATA_H

#include "amdgv_diag_data_trace_log.h"

/* General Header and Block signatures */
#define AMDGV_DIAG_DATA_HEADER_SIGNATURE		0x42445241
#define AMDGV_DIAG_DATA_BLOCK_SIGNATURE		0x4B434C42

/* Memory buffer size of 1 M each for host and asic */
#define AMDGV_DIAG_DATA_HOST_DRV_MEM_SIZE		(1024 * 1024)
#define AMDGV_DIAG_DATA_ASIC_MEM_SIZE		(1024 * 1024)
#define AMDGV_DIAG_DATA_MEM_SIZE \
	(AMDGV_DIAG_DATA_HOST_DRV_MEM_SIZE + \
	 AMDGV_DIAG_DATA_ASIC_MEM_SIZE)

#define AMDGV_DIAG_DATA_MINOR_VERSION		0x00
#define AMDGV_DIAG_DATA_MAJOR_VERSION		0x01
#define AMDGV_DIAG_DATA_VERSION	(AMDGV_DIAG_DATA_MINOR_VERSION | \
		AMDGV_DIAG_DATA_MAJOR_VERSION << 8)

/* Alignemnt of the host driver block to 4k */
#define AMDGV_DIAG_DATA_ALIGNMENT				4096
#if (AMDGV_DIAG_DATA_ALIGNMENT % 4096) != 0
#error diagnosis data block alignment should be multiple of 4K
#endif
#define AMDGV_DIAG_DATA_ALIGN(addr) \
	(((addr) + (AMDGV_DIAG_DATA_ALIGNMENT)-1) & ~(AMDGV_DIAG_DATA_ALIGNMENT - 1))

/* FTRACE block size */
#define AMDGV_DIAG_DATA_FTRACE_LOG_SIZE		(256 * AMDGV_DIAG_DATA_ALIGNMENT)

/* Critical error log size */
#define AMDGV_DIAG_DATA_CRIT_ERR_LOG_SIZE  \
	(AMDGV_DIAG_DATA_MEM_SIZE + AMDGV_DIAG_DATA_FTRACE_LOG_SIZE)

/* Bit 31:24 of block header indicates the timing of cllecting Debug Data */
#define AMDGV_DIAG_DATA_COLLET_TIMING_FLAG(t)     ((t & 0xFF) << 24)

/* Bit 23:16 of block header indicates RWL section for PSP snapshot block */
#define AMDGV_DIAG_DATA_PSP_RWL_SECTION_FLAG(ver) ((ver & 0xFF) << 16)

/* Block Component ID */
enum AMDGV_DIAG_DATA_BLOCK_COMPONENT_ID {
	AMDGV_DIAG_DATA_BLOCK_COMPONENT_INVALID = 0,

	AMDGV_DIAG_DATA_BLOCK_COMPONENT_HOST,
	AMDGV_DIAG_DATA_BLOCK_COMPONENT_SMU,
	AMDGV_DIAG_DATA_BLOCK_COMPONENT_PSP,
	AMDGV_DIAG_DATA_BLOCK_COMPONENT_RLC,
	AMDGV_DIAG_DATA_BLOCK_COMPONENT_MMSCH,
	AMDGV_DIAG_DATA_BLOCK_COMPONENT_GUEST,

	AMDGV_DIAG_DATA_BLOCK_COMPONENT_INTERN = 0xF0,
};

/* Block ID */
enum AMDGV_DIAG_DATA_BLOCK_ID {

	/* Host Logs */
	AMDGV_DIAG_DATA_BLOCK_ID_HOST_INVALID = AMDGV_DIAG_DATA_BLOCK_COMPONENT_HOST << 8,
	AMDGV_DIAG_DATA_BLOCK_ID_HOST_REGISTERS_DUMP,
	AMDGV_DIAG_DATA_BLOCK_ID_HOST_TRACE_LOG,
	AMDGV_DIAG_DATA_BLOCK_ID_HOST_GENERAL_INFO,
	AMDGV_DIAG_DATA_BLOCK_ID_HOST_ERROR_DUMP,
	AMDGV_DIAG_DATA_BLOCK_ID_HOST_FTRACE_LOG,
	AMDGV_DIAG_DATA_BLOCK_ID_HOST_IH_RING_DUMP,
	AMDGV_DIAG_DATA_BLOCK_ID_HOST_POST_VBIOS_LOG,

	/* SMU Logs */
	AMDGV_DIAG_DATA_BLOCK_ID_SMU_INVALID = AMDGV_DIAG_DATA_BLOCK_COMPONENT_SMU << 8,
	AMDGV_DIAG_DATA_BLOCK_ID_SMU_SRAM_DUMP,

	/* PSP Logs */
	AMDGV_DIAG_DATA_BLOCK_ID_PSP_INVALID = AMDGV_DIAG_DATA_BLOCK_COMPONENT_PSP << 8,
	AMDGV_DIAG_DATA_BLOCK_ID_PSP_SNAPSHOT_DUMP,
	AMDGV_DIAG_DATA_BLOCK_ID_PSP_TRACE_LOG_DUMP,
	AMDGV_DIAG_DATA_BLOCK_ID_PSP_REGISTERS_DUMP,

	/* RLC Logs */
	AMDGV_DIAG_DATA_BLOCK_ID_RLC_INVALID = AMDGV_DIAG_DATA_BLOCK_COMPONENT_RLC << 8,
	AMDGV_DIAG_DATA_BLOCK_ID_RLCV_SRAM_DUMP,
	AMDGV_DIAG_DATA_BLOCK_ID_RLCG_SRAM_DUMP,
	AMDGV_DIAG_DATA_BLOCK_ID_RLC_SRM_ARAM_DUMP,
	AMDGV_DIAG_DATA_BLOCK_ID_RLC_SRM_DRAM_DUMP,

	/* MMSCH Logs */
	AMDGV_DIAG_DATA_BLOCK_ID_MMSCH_INVALID = AMDGV_DIAG_DATA_BLOCK_COMPONENT_MMSCH << 8,
	AMDGV_DIAG_DATA_BLOCK_ID_MMSCH_SRAM_DUMP,

	/* Guest Logs */
	AMDGV_DIAG_DATA_BLOCK_ID_GUEST_INVALID = AMDGV_DIAG_DATA_BLOCK_COMPONENT_GUEST << 8,
};

/* Debug Memory block */
struct amdgv_diag_data_mem_block {
	uint32_t		block_id;
	void			*vaddr;
	uint64_t		bus_addr;
	uint32_t		size;
	uint32_t		used_size;
};

/* This header is saved in the file of diagnosis data */
struct amdgv_diag_data_header {
	uint32_t signature;
	uint32_t version;
	uint64_t datetime;
	uint32_t asic_id;
	uint32_t bus_id;
	uint32_t size;
	uint32_t next_block_offset;
	uint8_t	reserved[32];
};

struct amdgv_diag_data_block_header {
	uint32_t block_signature; /* Always be 0x4B434C42 */
	uint32_t block_id;
	uint32_t block_size;
	uint32_t version;
	uint64_t timestamp;
	uint32_t next_block_offset;
	uint32_t checksum;
	uint64_t cpu_timestamp;
	uint32_t blk_flags;
	uint8_t md5[16];
	uint8_t reserved[4];
};

enum AMDGV_DIAG_DATA_LOG_COLLECT_TYPE {
	AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_START = 0,

	AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_FLR,
	AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_RESET,
	AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_INIT_FAIL,
	AMDGV_DIAG_DATA_LOG_COLLECT_ACTIVITY_BUFFER,

	AMDGV_DIAG_DATA_LOG_COLLECT_CACHE_END,

};

struct amdgv_diag_data_cache_buf {
	uint8_t buf[AMDGV_DIAG_DATA_CRIT_ERR_LOG_SIZE];
	struct amdgv_list_head head;
	uint32_t used_size;
	uint32_t bdf;
	uint32_t dev_id;
	enum AMDGV_DIAG_DATA_LOG_COLLECT_TYPE type;
};

struct amdgv_diag_data_file_info {
	void *buff;
	uint32_t size;
	uint32_t cur_offset;
	uint32_t collect_type;
	uint32_t idx_vf;
	struct amdgv_diag_data_block_header *last_blk_hdr;
};

/* Register Dump related data */
struct amdgv_diag_data_dump_reg {
	struct {
		uint32_t  reg_index;
		uint32_t  reg_index_hi;
		uint32_t  reg_data;
	} dir;
	uint32_t hwid;
	uint32_t max_inst;
	uint32_t seg;
	uint32_t offset;
	uint32_t cross_aid;
};

/*
 * Update AMDGV_DIAG_DATA_REG_DUMP_BLOCK_VERSION when amdgv_diag_data_reg_entry
 * is updated
 */
#define AMDGV_DIAG_DATA_REG_DUMP_BLOCK_VERSION 0x00000101
struct amdgv_diag_data_reg_entry {
	uint32_t reg_addr;
	uint32_t reg_val;
	uint8_t inst;
	uint8_t reserved[3];
};

struct amdgv_diag_data_buff {
	void		*vaddr;
	uint64_t	bus_addr;
};

struct amdgv_diag_data {
	struct oss_dma_mem_info	asic_sys_mem;
	int (*collect_data)(struct amdgv_adapter *adapt,
			struct amdgv_diag_data_file_info *file_data);
	uint64_t (*get_gpu_ref_timestamp)(struct amdgv_adapter *adapt);
	uint32_t (*read_cross_aid_reg)(struct amdgv_adapter *adapt,
			uint64_t reg, uint32_t aid);

	/* Placeholder for Host/Asic specific blocks */
	struct amdgv_diag_data_buff host_drv_buff;
	struct amdgv_diag_data_buff asic_buff;
};

#ifndef EXCLUDE_FTRACE
/* Host driver exclude list for call_trace */
extern char *host_driver_call_trace_exclude_list[];
extern uint32_t host_driver_call_trace_exclude_list_len;

/*
 * amdgv_diag_data_exclude_list - Get the exclude list for call_trace.
 *				It contains all functions that should be excluded from tracing.
 *
 * @call_trace_exclude_list:	buffer to populate with list of functions to exclude.
 * @call_trace_exclude_len:	Length of the call_trace_exclude_list
 */
void amdgv_diag_data_exclude_list(char **call_trace_exclude_list[],
		uint32_t *call_trace_exclude_len);
#endif

/* Diagnosis data collection core APIs */
/*
 * amdgv_diag_data_collect - Request to collect/copy diagnosis data in buffer
 *
 * @adapt:		amdgv device handle, NULL if GPU is not initialized
 * @bdf:		bus address of the GPU
 * @buf:		buffer to copy the diagnosis data blocks
 * @size:		size of the buffer passed
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_diag_data_collect(struct amdgv_adapter *adapt, uint32_t bdf,
		void *buff, uint32_t size);

/*
 * amdgv_diag_data_init - Initialize the diagnosis data section of host driver.
 *
 * @adapt:	amdgv device handle
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_diag_data_init(struct amdgv_adapter *adapt);

/*
 * amdgv_diag_data_fini - De-initialize the diagnosis data section of host driver.
 *
 * @adapt:	amdgv device handle
 *
 */
void amdgv_diag_data_fini(struct amdgv_adapter *adapt);

/*
 * amdgv_diag_data_cache_dump - Dump diagnosis data to the buffer.
 *                     The buffer is allocated by the API.
 *                     The API is invoked in case of error
 *                     such as initialization failed, GPU reset, or FLR,
 *                     Or it can also be used to collect current activity data
 *
 * @adapt:	amdgv device handle
 * @idx_vf:	VF ID for which we want to collect data
 * @type:   	parameter to indicate whether the data is collected
 *		as a result of an FLR, GPU reset, or initialization failed.
 *
 * Returns:
 * Valid address for success, 0 for failure
 *
 */
int amdgv_diag_data_cache_dump(struct amdgv_adapter *adapt, uint32_t idx_vf,
		enum AMDGV_DIAG_DATA_LOG_COLLECT_TYPE type);

/*
 * amdgv_diag_data_add_trace_log -	Add trace log entry to the diagnosis trace
 * log block
 *
 * @adapt:	amdgv device handle
 * @timestamp:	GPU timestamp
 * @feature:	feature for which the trace log entry is added. Refer to
 *		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_ID for valid value
 *		of the field
 * @code:	2 bytes code to add the trace log. The formation of the code
 *		varies among features
 * @ext_code:	4 bytes extended code to add the trace log. The formation of the
 *		extended code varies among features
 *
 * Returns:
 * Valid address for success, 0 for failure
 *
 */
int amdgv_diag_data_add_trace_log(struct amdgv_adapter *adapt,
		uint8_t feature, uint16_t code, uint32_t ext_code);

/*
 * amdgv_diag_data_add_error - Add error entry to diagnosis data error block
 * 				  The API is to log the errors in host driver.
 *
 * @adapt:	amdgv device handle
 *
 * Returns: none
 */
void amdgv_diag_data_add_error(struct amdgv_adapter *adapt);

/*
 * amdgv_diag_data_vbios_post_start - Init VBIOS post log in diagnosis data.
 *					API allocates the resources required for
 *					VBIOS post such as memory block etc.
 *
 * @adapt:	amdgv device handle
 * @post_type	VBIOS post type i.e., full post, asic init etc.
 * @index	index in the command table
 * @ps		parameter space length for the current operation
 * @ws		workspace length for the currect operation
 * @len		length of the current operation
 * @base	base address in command table based on the index
 * @code_loc	location of the code in the table
 *
 * Returns:
 * Valid address for success, 0 for failure
 *
 */
int amdgv_diag_data_vbios_post_start(struct amdgv_adapter *adapt,
		uint32_t post_type, uint32_t index, uint32_t ps,
		uint32_t ws, uint32_t len, uint32_t base, uint32_t code_loc);

/*
 * amdgv_diag_data_vbios_post_end - De-init VBIOS post log in diagnosis data.
 *					API releases all resources allocated for
 *					VBIOS post log using
 *					amdgv_diag_data_vbios_post_init
 *
 * @adapt:	amdgv device handle
 *
 */
void amdgv_diag_data_vbios_post_end(struct amdgv_adapter *adapt);

/*
 * amdgv_diag_data_vbios_post_log - Add VBIOS post log entry in diagnosis data.
 *
 * @adapt:	amdgv device handle
 * @op		operation in atom parse table
 * @arg		arg from parse table for the op.
 * @timestamp	CPU timestamp
 * @start_loc	start location of the op in the table
 * @end_loc	end location of the op. start_loc to end_loc gives the contents
 *		for the current op
 * @write_val	written value as result of op
 * @write_addr	address where the value is written. It can be a register or FB
 * @read_value	read value as result of op
 * @read_addr	address from where the value is read. It can be a register or FB
 *
 * Returns:
 * Valid address for success, 0 for failure
 *
 */
int amdgv_diag_data_add_vbios_post_log(struct amdgv_adapter *adapt,
		uint32_t op, uint32_t arg, uint64_t timestamp,
		uint32_t start_loc, uint32_t end_loc,
		uint32_t write_val, uint32_t write_addr,
		uint64_t read_val, uint32_t read_addr);

/* APIS in amdgv_diag_data.c shared with amdgv_diag_data_host_drv.c */
int amdgv_diag_data_gen_blk_file_hdr(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data,
		uint32_t block_id, void *data,
		uint32_t data_len, uint32_t flag);
/*
 * amdgv_diag_data_add_ring_buffer -	Copy data from the memory block
 *						to the file buffer(file_data). This API
 *						generates the block header and calls
 *						amdgv_diag_data_copy to copy the
 *						contents of the buffer.
 *
 * @adapt:		amdgv device handle
 * @file_data		file buffer (destination) to copy the data.
 * @mem_blk:		memory block that contains the buuffer to copy
 * @entry_size:		size of each entry in the buffer
 * @log_entries:	total number of entries
 * @w_count:		total written entries, this can excced the buffer size
 * @keep_entries:	the number of entries to keep i.e., not overwrriten by overflow.
 *
 * Returns:
 * Valid address for success, 0 for failure
 *
 */
int amdgv_diag_data_add_ring_buffer(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data,
		struct amdgv_diag_data_mem_block *mem_blk,
		uint32_t entry_size, uint32_t log_entries, uint64_t w_count,
		uint32_t keep_entries, uint32_t flag);

/*
 * amdgv_diag_data_add_blk -	Request to collect/copy diagnosis data block
 * 					in buffer
 *
 * @adapt:	amdgv device handle
 * @mem_blk:	the block of the memory to be copied. This block
 *		was obtained by calling @amdgv_diag_data_mem_request
 * @file_data:	parameter contains the buffer to copy the diagnosis data
 *		block. In addition to that it also conatins the size of the
 *		buffer and the current offset.
 * @used_size:	length of the data in the memory block that needs to be copied.
 * @flag:	4 byte flag. Lowest 3 bytes to indicate the block specific flags
 *		such as encrypt/decrypt. Upper byte contians the index of the
 *		cache log in case the data collection is triggered by error(in
 *		current implementation the index will always be 1 as cache
 *		log is configured to contian only 1 log).
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_diag_data_add_blk(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_mem_block *mem_blk,
		struct amdgv_diag_data_file_info *file_data,
		uint32_t used_size, uint32_t flag);

/*
 * amdgv_diag_data_add_blk_without_hdr - Request to collect/copy diagnosis data block
 * 					data in buffer of last block wihout generating header
 *
 * @adapt:	amdgv device handle
 * @mem_blk:	the block of the memory to be copied. This block
 *		was obtained by calling @amdgv_diag_data_mem_request
 * @file_data:	parameter contains the buffer to copy the diagnosis data
 *		block. In addition to that it also conatins the size of the
 *		buffer and the current offset.
 * @used_size:	length of the data in the memory block that needs to be copied.
 * @gen_cksum:	flag to indicate if the cksum should be calculated
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_diag_data_add_blk_without_hdr(struct amdgv_adapter *adapt,
				    struct amdgv_diag_data_mem_block *mem_blk,
				    struct amdgv_diag_data_file_info *file_data,
				    uint32_t used_size, bool gen_cksum);

/*
 * amdgv_diag_data_copy -	Copy data from the buffer (blk_vaddr) to
 * 					the file buffer(file_data). If the enties
 * 					are less than the size than it does as simple
 * 					copy, otherwise the buffer is copied as ring
 * 					buffer.
 *
 * @adapt:		amdgv device handle
 * @file_data		file buffer (destination) to copy the data.
 * @entry_size:		size of each entry in the buffer
 * @log_entries:	total number of entires
 * @w_count:		total written entries, this can excced the buffer size
 * @keep_entries:	the number of entries to keep i.e., not overwrriten bu overflow.
 * @blk_vaddr:		address of the source buffer
 *
 * Returns:
 * Valid address for success, 0 for failure
 *
 */
void amdgv_diag_data_copy(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_file_info *file_data,
		uint32_t entry_size, uint32_t log_entries, uint64_t w_count,
		uint32_t keep_entries, void *blk_vaddr);

/* Collect register dump */
int amdgv_diag_data_host_collect_reg_dump(struct amdgv_adapter *adapt,
		struct amdgv_diag_data_dump_reg *dbg_regs, uint32_t regs_count,
		struct amdgv_diag_data_file_info *file_data);

/*
 * amdgv_diag_data_cache_buf_init - Initialize the diagnosis data Cache
 *         Data Buffer list.
 */
void amdgv_diag_data_cache_buf_init(void);

/*
 * amdgv_diag_data_cache_buf_fini - De-initialize the diagnosis data Cache
 *         Data Buffer list.
 */
void amdgv_diag_data_cache_buf_fini(void);

#endif
