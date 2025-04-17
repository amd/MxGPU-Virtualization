/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv.h"
#include "amdgv_api.h"
#include "amdgv_device.h"
#include "amdgv_wb_memory.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

/*
 * amdgv_wb_memory_*()
 * Writeback is the method by which the GPU updates special pages in memory
 * with the status of certain GPU events (fences, ring pointers,etc.).
 */

/**
 * amdgv_wb_memory_init - Init Writeback driver info and allocate memory
 *
 * @adapt: amdgv_device pointer
 *
 * Initializes writeback and allocates writeback memory (all asics).
 * Used at driver startup.
 * Returns 0 on success or an -error on failure.
 */
int amdgv_wb_memory_init(struct amdgv_adapter *adapt)
{
	adapt->wb.wb_obj =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf,
						AMDGV_MAX_WB * AMDGV_WB_MEORY_BYTE_SIZE,
						PAGE_SIZE, MEM_GFX_WB);
	if (!adapt->wb.wb_obj) {
		AMDGV_WARN("Create WB bo failed\n");
		return AMDGV_FAILURE;
	}
	adapt->wb.num_wb = AMDGV_MAX_WB;
	oss_memset(adapt->wb.used, 0, AMDGV_WB_QWORD_COUNT * sizeof(uint64_t));
	return 0;
}

/**
 * amdgv_wb_memory_fini - Disable Writeback and free memory
 *
 * @adapt: amdgv_device pointer
 *
 * Disables Writeback and frees the Writeback memory (all asics).
 * Used at driver shutdown.
 */
void amdgv_wb_memory_fini(struct amdgv_adapter *adapt)
{
	if (adapt->wb.wb_obj) {
		amdgv_memmgr_free(adapt->wb.wb_obj);
		adapt->wb.wb_obj = NULL;
	}
}

void amdgv_wb_memory_clear(struct amdgv_adapter *adapt)
{
	/* Clear the whole wb memory */
	oss_memset((void *)adapt->wb.wb, 0, AMDGV_MAX_WB * AMDGV_WB_MEORY_BYTE_SIZE);
}

int amdgv_wb_memory_hw_init_address(struct amdgv_adapter *adapt)
{
	adapt->wb.gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->wb.wb_obj);
	adapt->wb.wb = amdgv_memmgr_get_cpu_addr(adapt->wb.wb_obj);
	return 0;
}

static uint32_t find_wb_memory_tracking_free_bit(struct amdgv_adapter *adapt)
{
	int qword_index = 0, i;
	uint64_t bitmap = 0;

	while (qword_index < AMDGV_WB_QWORD_COUNT) {
		if (0xFFFFFFFFFFFFFFFFULL != adapt->wb.used[qword_index]) {
			break;
		}
		++qword_index;
	}
	if (qword_index < AMDGV_WB_QWORD_COUNT)	{
		bitmap = adapt->wb.used[qword_index];
		for (i = 0; i < 64; i++) {
			if (!(bitmap & (0x1ULL << i)))
				return i + qword_index * 64;
		}
	}
	return 0xFFFFFFFF;
}

static void set_wb_memory_tracking_bit(struct amdgv_adapter *adapt, uint32_t wb_track_bit_index)
{
	uint32_t qword_index = wb_track_bit_index / 64;
	uint32_t bit_index = wb_track_bit_index % 64;

	adapt->wb.used[qword_index] |= (0x1ULL << bit_index);
}

static void clear_wb_memory_tracking_bit(struct amdgv_adapter *adapt, uint32_t wb_track_bit_index)
{
	uint32_t qword_index = wb_track_bit_index / 64;
	uint32_t bit_index = wb_track_bit_index % 64;

	adapt->wb.used[qword_index] &= ~(0x1ULL << bit_index);
}

/**
 * amdgv_wb_memory_get - Allocate a wb entry
 *
 * @adapt: amdgv_device pointer
 * @wb: wb DWORD index
 *
 * Allocate a wb slot for use by the driver (all asics).
 * Returns 0 on success or AMDGV_FAILURE on failure.
 */
int amdgv_wb_memory_get(struct amdgv_adapter *adapt, uint32_t *wb)
{
	int ret = AMDGV_FAILURE;
	uint32_t bit_index = find_wb_memory_tracking_free_bit(adapt);

	if (bit_index < adapt->wb.num_wb) {
		set_wb_memory_tracking_bit(adapt, bit_index);

		/* Convert to DWORD offset from index of 32 bytes mmemory */
		*wb = bit_index << 3;
		ret = 0;
	}
	return ret;
}

/**
 * amdgv_wb_memory_free - Free a wb entry
 *
 * @adapt: amdgv_device pointer
 * @wb: wb DWORD index
 *
 * Free a wb slot allocated for use by the driver (all asics)
 */
void amdgv_wb_memory_free(struct amdgv_adapter *adapt, uint32_t wb)
{
	/* wb - DWORD offset in write back memory
	 *  1 track bit represents a 32 byts memory block(8 DWORDs)
	 */

	/* Convert DWORD index to index of 32 bytes memory block */
	uint32_t wb_track_bit_index = wb >> 3;

	if (wb_track_bit_index < adapt->wb.num_wb) {
		clear_wb_memory_tracking_bit(adapt, wb_track_bit_index);
	}
}
