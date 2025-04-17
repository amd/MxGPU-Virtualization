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

#ifndef AMDGV_WB_MEMORY_H
#define AMDGV_WB_MEMORY_H

#include "amdgv_basetypes.h"
#include "amdgv_api.h"

/* Reserve slots for amdgv-owned rings. */
#define AMDGV_WB_QWORD_COUNT 16
#define AMDGV_MAX_WB (AMDGV_WB_QWORD_COUNT * 64)

// Each write back memory is 32 (0x20) bytes memory block
#define AMDGV_WB_MEORY_BYTE_SIZE (8 * sizeof(uint32_t))

struct amdgv_wb {
	struct amdgv_memmgr_mem	*wb_obj;

	volatile uint32_t *wb;
	uint64_t gpu_addr;

	/* Number of wb slots actually reserved for amdgv. */
	uint32_t num_wb;
	uint64_t used[AMDGV_WB_QWORD_COUNT];
};

int  amdgv_wb_memory_init(struct amdgv_adapter *adapt);
void amdgv_wb_memory_fini(struct amdgv_adapter *adapt);

void amdgv_wb_memory_clear(struct amdgv_adapter *adapt);
int  amdgv_wb_memory_hw_init_address(struct amdgv_adapter *adapt);

int  amdgv_wb_memory_get(struct amdgv_adapter *adapt, uint32_t *wb);
void amdgv_wb_memory_free(struct amdgv_adapter *adapt, uint32_t wb);

#endif // AMDGV_WB_MEMORY_H
