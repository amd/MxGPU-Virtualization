/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_WB_MEMORY_H
#define AMDGV_WB_MEMORY_H

#include "amdgv_basetypes.h"
#include "amdgv_api.h"

/* Reserve slots for amdgv-owned rings. */
#define AMDGV_WB_QWORD_COUNT 16
#define AMDGV_MAX_WB (AMDGV_WB_QWORD_COUNT * 64)

// Each write back memory is 32 (0x20) bytes memory block
#define AMDGV_WB_MEMORY_BYTE_SIZE (8 * sizeof(uint32_t))

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
