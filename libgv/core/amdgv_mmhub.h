/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
	uint64_t (*get_mc_fb_offset)(struct amdgv_adapter *adapt);
	int (*get_fb_location)(struct amdgv_adapter *adapt, uint64_t *base, uint64_t *top);
	int (*gart_enable)(struct amdgv_adapter *adapt);
	int (*gart_disable)(struct amdgv_adapter *adapt);
	int (*get_xgmi_info)(struct amdgv_adapter *adapt);
};

struct amdgv_mmhub {
	int num_instances;
	uint32_t active_mask;
	struct ras_common_if	*ras_if;
	const struct amdgv_mmhub_funcs	*funcs;
};

uint64_t amdgv_mmhub_get_mc_fb_offset(struct amdgv_adapter *adapt);
int amdgv_mmhub_get_fb_location(struct amdgv_adapter *adapt, uint64_t *base, uint64_t *top);
int amdgv_mmhub_gart_enable(struct amdgv_adapter *adapt);
int amdgv_mmhub_gart_disable(struct amdgv_adapter *adapt);
int amdgv_mmhub_query_ras_error_count(struct amdgv_adapter *adapt, void *err_data);
int amdgv_mmhub_get_xgmi_info(struct amdgv_adapter *adapt);
#endif
