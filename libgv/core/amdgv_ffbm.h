/*
 * Copyright (c) 2014-2025 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
 */

#ifndef AMDGV_FFBM_H
#define AMDGV_FFBM_H

#include <amdgv_device.h>

#define FFBM_LOCK_LIST      (oss_mutex_lock(adapt->ffbm.pt_lock))
#define FFBM_UNLOCK_LIST    (oss_mutex_unlock(adapt->ffbm.pt_lock))

/* pre-allocate memory blocks:
 * 3 block for each PF/VF, PF needs additional one, so it is 17 * 3 + 1 = 52 blocks
 * 30 reserved pages 30 + 52 = 82 blocks
 * if 30 bad page happens, for each bad page, will increase 2 blocks, 82 + 30 *2 = 142 blocks
 * pre-allocate 256 blocks for safe.
*/
#define AMDGV_FFBM_MEMORY_ALLOCATION_COUNT 256

#define AMDGV_FFBM_FB_TMR_OFFSET 2 /* 2 MB*/

/* 2^fragment * 4kb */
#define AMDGV_FFBM_PAGE_SIZE(fragment)  ((uint64_t)1 << ((fragment) + 12))
#define AMDGV_FFBM_PAGE_ALIGN(val, fragment)  ((val) & ~(AMDGV_FFBM_PAGE_SIZE(fragment) - 1))
#define AMDGV_FFBM_PAGE_ALIGN_CEIL(val, fragment)  AMDGV_FFBM_PAGE_ALIGN(((val) + AMDGV_FFBM_PAGE_SIZE(fragment) - 1), (fragment))
#define AMDGV_FFBM_INVALID_ADDR   (~0)
#define AMDGV_FFBM_ADDR_IN_RANGE(range_start, range_size, addr) ((range_start) <= (addr) && ((range_start) + (range_size)) > (addr))
#define AMDGV_FFBM_ADDR_SIZE_IN_RANGE(range_start, range_size, addr, size) \
	(AMDGV_FFBM_ADDR_IN_RANGE((range_start), (range_size), (addr)) && AMDGV_FFBM_ADDR_IN_RANGE((range_start), (range_size), (addr) + (size) - 1))
#define AMDGV_FFBM_VF_MIN_MAP_SIZE ((uint64_t)(MBYTES_TO_BYTES(16)))
#define AMDGV_FFBM_MAP_SIZE_VALID(size, fragment) ((size) & ((1 << ((fragment) + 12)) - 1))
#define AMDGV_FFBM_TMR_ADDR_VALID(gpa, spa, size, tmr_size) ((gpa) == (MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET)) && (spa) == (MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET)) && (size) == (MBYTES_TO_BYTES(tmr_size)))

enum amdgv_ffbm_resp {
	AMDGV_FFBM_SUCCESS           =  0,
	AMDGV_FFBM_ERROR_OVERLAP     = -1,          /* manual mapping has overlap with others */
	AMDGV_FFBM_ERROR_BAD_ADDR    = -2,          /* physical offset of ffbm offset is not exist in the table */
	AMDGV_FFBM_ERROR_NO_MEM      = -3,          /* not enough address space */
	AMDGV_FFBM_BAD_PAGE_IN_RANGE = -4,          /* there is bad page in manual mapping range */
	AMDGV_FFBM_ERROR_INVALD_SIZE = -5,          /* the mapping size is invalid (e.g. not 2 MB aligned)*/
	AMDGV_FFBM_ERROR_GENERIC     = -6,          /* general error */
};

enum amdgv_ffbm_mem_type {
	AMDGV_FFBM_MEM_TYPE_INVALID   = 0,  /* range which is not initialized, not valid to use*/
	AMDGV_FFBM_MEM_TYPE_PF,             /* PF will not be submit to HW, also will not be used in FFBM */
	AMDGV_FFBM_MEM_TYPE_VF,             /* range which is assigned to a VF */
	AMDGV_FFBM_MEM_TYPE_RESERVED,       /* this mem is reserved for bad page or further use */
	AMDGV_FFBM_MEM_TYPE_BAD_PAGE,       /* this mem has bad page so don't map */
	AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED,   /* range which is not assigned to any usage */
	AMDGV_FFBM_MEM_TYPE_TMR,            /* range which is assigned to TMR */
};

enum amdgv_ffbm_permission {
	AMDGV_FFBM_PERM_READ             = (1 << 0),
	AMDGV_FFBM_PERM_WRITE            = (1 << 1),

	AMDGV_FFBM_PERM_RW               = (AMDGV_FFBM_PERM_READ | AMDGV_FFBM_PERM_WRITE),
};

enum amdgv_ffbm_copy {
	AMDGV_FFBM_MM_COPY             = 0,
	AMDGV_FFBM_PF_COPY,
};
struct amdgv_ffbm_pte_block {
	uint64_t gpa;
	uint64_t spa;
	uint64_t size;
	uint8_t fragment;                  /* ordered by 2 */
	uint8_t permission;
	uint32_t vf_idx;
	enum amdgv_ffbm_mem_type type;

	struct amdgv_list_head gpa_list_node;   /* list by gpa address */
	struct amdgv_list_head spa_list_node;    /* list by spa address */

	bool applied;                       /* this PTE block is applied, any change will cause it be unapplied */
};

struct amdgv_ffbm_memory_block {
	struct amdgv_ffbm_pte_block pte_block;
	bool used;
};

struct amdgv_ffbm {
	int (*apply_pteb)(struct amdgv_adapter *adapt, struct amdgv_ffbm_pte_block *pteb, bool valid);
	int (*invalidate_tlb)(struct amdgv_adapter *adapt, struct amdgv_ffbm_pte_block *pteb, bool flush);

	struct amdgv_ffbm_memory_block memory_block[AMDGV_FFBM_MEMORY_ALLOCATION_COUNT];
	mutex_t memory_lock;                     /* avoid multithread allocation */

	struct amdgv_list_head spa_list;         /* list ordered by pf physical address address, asc */
	uint8_t bad_block_count;
	uint8_t reserved_block_count;
	uint64_t total_physical_size;                   /* total physical size managed by ffbm */

	/* mutex */
	mutex_t pt_lock;

	/* core structure */
	bool enabled;

	/* configurations */
	uint8_t default_fragment;
	uint8_t max_reserved_block;

	/* enable flag for sharing tmr */
	bool share_tmr;

};

int amdgv_ffbm_sw_init(struct amdgv_adapter *adapt);
int amdgv_ffbm_sw_fini(struct amdgv_adapter *adapt);
int amdgv_ffbm_page_table_update_by_fcn(struct amdgv_adapter *adapt, uint32_t vf_idx);
int amdgv_ffbm_page_table_init(struct amdgv_adapter *adapt);
uint64_t amdgv_ffbm_gpa_to_spa(struct amdgv_adapter *adapt, uint64_t gpa, uint32_t vf_idx);
uint64_t amdgv_ffbm_spa_to_gpa(struct amdgv_adapter *adapt, uint64_t spa, uint32_t *vf_idx);
void amdgv_ffbm_reserve_all_bad_pages(struct amdgv_adapter *adapt);
int amdgv_ffbm_manual_map(struct amdgv_adapter *adapt, uint32_t vf_idx, uint64_t size, uint64_t gpa, uint64_t spa, uint8_t permission, enum amdgv_ffbm_mem_type type);
void amdgv_ffbm_page_table_destroy(struct amdgv_adapter *adapt);
void amdgv_ffbm_dump_page_table(struct amdgv_adapter *adapt);
uint64_t amdgv_ffbm_copy_to_gpa(struct amdgv_adapter *adapt, uint32_t *data, uint32_t idx_vf, uint64_t gpa, uint64_t size, enum amdgv_ffbm_copy type);
uint64_t amdgv_ffbm_copy_from_gpa(struct amdgv_adapter *adapt, uint32_t *data, uint32_t idx_vf, uint64_t gpa, uint64_t size, enum amdgv_ffbm_copy type);
void amdgv_ffbm_unmap_by_fcn(struct amdgv_adapter *adapt, uint32_t vf_idx, bool reserve);
void amdgv_ffbm_read_page_table(struct amdgv_adapter *adapt, char *page_table_content, int restore_length, int *len);
void amdgv_ffbm_copy_page_table(struct amdgv_adapter *adapt, void *page_table_content, int max_num, int *len);
int amdgv_ffbm_replace_bad_pages(struct amdgv_adapter *adapt, struct eeprom_table_record *bps, int pages);
int amdgv_ffbm_apply_page_table(struct amdgv_adapter *adapt);
enum amdgv_live_info_status amdgv_ffbm_export_spa(struct amdgv_adapter *adapt, struct amdgv_live_info_ffbm *ffbm_info);
enum amdgv_live_info_status amdgv_ffbm_import_spa_and_gpa(struct amdgv_adapter *adapt, struct amdgv_live_info_ffbm *ffbm_info);
int amdgv_get_vf_fb_mapping_list(struct amdgv_adapter *adapt, uint32_t vf_idx, struct amdgv_vf_ffbm_map_list *list, bool include_tmr_block);

#endif
