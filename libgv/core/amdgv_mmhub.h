/*
 * Copyright 2022 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __AMDGV_MMHUB_H__
#define __AMDGV_MMHUB_H__


enum amdgv_mmhub_ras_memory_id {
	AMDGV_MMHUB_WGMI_PAGEMEM = 0,
	AMDGV_MMHUB_RGMI_PAGEMEM = 1,
	AMDGV_MMHUB_WDRAM_PAGEMEM = 2,
	AMDGV_MMHUB_RDRAM_PAGEMEM = 3,
	AMDGV_MMHUB_WIO_CMDMEM = 4,
	AMDGV_MMHUB_RIO_CMDMEM = 5,
	AMDGV_MMHUB_WGMI_CMDMEM = 6,
	AMDGV_MMHUB_RGMI_CMDMEM = 7,
	AMDGV_MMHUB_WDRAM_CMDMEM = 8,
	AMDGV_MMHUB_RDRAM_CMDMEM = 9,
	AMDGV_MMHUB_MAM_DMEM0 = 10,
	AMDGV_MMHUB_MAM_DMEM1 = 11,
	AMDGV_MMHUB_MAM_DMEM2 = 12,
	AMDGV_MMHUB_MAM_DMEM3 = 13,
	AMDGV_MMHUB_WRET_TAGMEM = 19,
	AMDGV_MMHUB_RRET_TAGMEM = 20,
	AMDGV_MMHUB_WIO_DATAMEM = 21,
	AMDGV_MMHUB_WGMI_DATAMEM = 22,
	AMDGV_MMHUB_WDRAM_DATAMEM = 23,
	AMDGV_MMHUB_MEMORY_BLOCK_LAST,
};

struct amdgv_mmhub_funcs {
	void (*err_cnt_init)(struct amdgv_adapter *adapt);
	void (*query_ras_error_count)(struct amdgv_adapter *adapt, void *ras_error_status);
	void (*query_ras_error_status)(struct amdgv_adapter *adapt);
	void (*reset_ras_error_count)(struct amdgv_adapter *adapt);
	void (*reset_ras_error_status)(struct amdgv_adapter *adapt);
};

struct amdgv_mmhub {
	int num_instances;
	uint32_t active_mask;
	struct ras_common_if	*ras_if;
	const struct amdgv_mmhub_funcs	*funcs;
};

#endif
