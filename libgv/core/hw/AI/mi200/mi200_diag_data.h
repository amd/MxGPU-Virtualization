/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI200_DIAG_DATA_H
#define MI200_DIAG_DATA_H

#define MI200_HOST_REGISTERS_DUMP_BLK_SIZE	(4 * AMDGV_DIAG_DATA_ALIGNMENT)
/* PSP block division */
#define MI200_PSP_SNAPSHOT_DUMP_BLK_SIZE	(128 * AMDGV_DIAG_DATA_ALIGNMENT)
#define MI200_PSP_TRACE_LOG_DUMP_BLK_SIZE	(4 * AMDGV_DIAG_DATA_ALIGNMENT)
#define MI200_INTER_STRUCT_SIZE		(1 * AMDGV_DIAG_DATA_ALIGNMENT)

/* Static check for the host driver memory size
 * Snapshot logs such as MI200_HOST_REGISTERS_DUMP_BLK_SIZE
 * are skip from the total size, as the register list is directly
 * read and copied to the user memory, it does not require additional
 * memory to hold data
 */
#define MI200_TOTAL_SIZE	(MI200_PSP_SNAPSHOT_DUMP_BLK_SIZE + \
				MI200_PSP_TRACE_LOG_DUMP_BLK_SIZE + \
				MI200_INTER_STRUCT_SIZE)
#if (MI200_TOTAL_SIZE > AMDGV_DIAG_DATA_ASIC_MEM_SIZE)
#error The AMDGV_DIAG_DATA_ASIC_MEM_SIZE is too small to allocate all blocks.
#endif

/* Get the inter struct offset */
#define MI200_INTER_STRUCT_OFFSET \
	(adapt->diag_data.asic_buff.vaddr ? \
	(uint8_t *)adapt->diag_data.asic_buff.vaddr + \
	(AMDGV_DIAG_DATA_ASIC_MEM_SIZE - \
	MI200_INTER_STRUCT_SIZE) : NULL)

/* Fills in the memory block for the given block */
#define MI200_DIAG_DATA_FILL_MEM_BLK(mem_blk, c_addr, BLOCK) { \
	mem_blk.vaddr = (void *)AMDGV_DIAG_DATA_ALIGN(c_addr); \
	mem_blk.size =  MI200_ ## BLOCK ## _BLK_SIZE; \
	mem_blk.block_id = AMDGV_DIAG_DATA_BLOCK_ID_ ## BLOCK; \
	mem_blk.used_size = 0; \
	mem_blk.bus_addr = adapt->diag_data.asic_buff.bus_addr + \
		((uint64_t)mem_blk.vaddr - \
		(uint64_t)adapt->diag_data.asic_buff.vaddr); \
	c_addr = (uint64_t)mem_blk.vaddr + mem_blk.size; \
}

struct amdgv_diag_data_asic_blk {
	struct amdgv_diag_data_mem_block psp_snapshot_mem_blk;
	struct amdgv_diag_data_mem_block psp_tracelog_mem_blk;
};

/* List for diagnosis data registers for host driver */
extern struct amdgv_diag_data_dump_reg mi200_diag_data_regs[];
extern uint32_t mi200_diag_data_regs_count;

#endif

