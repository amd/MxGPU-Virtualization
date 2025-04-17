/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef MI300_DIAG_DATA_H
#define MI300_DIAG_DATA_H

#define MI300_HOST_REGISTERS_DUMP_BLK_SIZE	(4 * AMDGV_DIAG_DATA_ALIGNMENT)
/* PSP block division */
#define MI300_PSP_SNAPSHOT_DUMP_BLK_SIZE	(128 * AMDGV_DIAG_DATA_ALIGNMENT)
#define MI300_PSP_TRACE_LOG_DUMP_BLK_SIZE	(4 * AMDGV_DIAG_DATA_ALIGNMENT)
#define MI300_INTER_STRUCT_SIZE			(1 * AMDGV_DIAG_DATA_ALIGNMENT)
#define MI300_NUM_XCC_PER_AID			2
#define MI300_NUM_SDMA_PER_XCC			2

/* Static check for the host driver memory size
 * Snapshot logs such as MI300_HOST_REGISTERS_DUMP_BLK_SIZE
 * are skip from the total size, as the register list is directly
 * read and copied to the user memory, it does not require additional
 * memory to hold data
 */
#define MI300_TOTAL_SIZE                                                                      \
	(MI300_PSP_SNAPSHOT_DUMP_BLK_SIZE + MI300_PSP_TRACE_LOG_DUMP_BLK_SIZE +               \
	 MI300_INTER_STRUCT_SIZE)
#if (MI300_TOTAL_SIZE > AMDGV_DIAG_DATA_ASIC_MEM_SIZE)
#error The AMDGV_DIAG_DATA_ASIC_MEM_SIZE is too small to allocate all blocks.
#endif

/* Get the inter struct offset */
#define MI300_INTER_STRUCT_OFFSET                                                             \
	(adapt->diag_data.asic_buff.vaddr ?                                                \
		 (uint8_t *)adapt->diag_data.asic_buff.vaddr +                             \
			 (AMDGV_DIAG_DATA_ASIC_MEM_SIZE - MI300_INTER_STRUCT_SIZE) :       \
		 NULL)

/* Fills in the memory block for the given block */
#define MI300_DIAG_DATA_FILL_MEM_BLK(mem_blk, c_addr, BLOCK)                                       \
	{                                                                                     \
		mem_blk.vaddr = (void *)AMDGV_DIAG_DATA_ALIGN(c_addr);                             \
		mem_blk.size = MI300_##BLOCK##_BLK_SIZE;                                      \
		mem_blk.block_id = AMDGV_DIAG_DATA_BLOCK_ID_##BLOCK;                               \
		mem_blk.used_size = 0;                                                        \
		mem_blk.bus_addr = adapt->diag_data.asic_buff.bus_addr +                   \
				   ((uint64_t)mem_blk.vaddr -                                 \
				    (uint64_t)adapt->diag_data.asic_buff.vaddr);           \
		c_addr = (uint64_t)mem_blk.vaddr + mem_blk.size;                              \
	}

struct amdgv_diag_data_asic_blk {
	struct amdgv_diag_data_mem_block psp_snapshot_mem_blk;
	struct amdgv_diag_data_mem_block psp_tracelog_mem_blk;
};

/* List for diagnosis data registers for host driver */
extern struct amdgv_diag_data_dump_reg mi300_diag_data_regs[];
extern uint32_t mi300_diag_data_regs_count;

#endif
