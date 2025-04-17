/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_device.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_ffbm.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static void amdgv_ffbm_unmap_pteb(struct amdgv_adapter *adapt, struct amdgv_ffbm_pte_block *pteb);

static struct amdgv_ffbm_pte_block *amdgv_ffbm_allocate_block(struct amdgv_adapter *adapt)
{
	uint32_t i;
	oss_mutex_lock(adapt->ffbm.memory_lock);
	for (i = 0; i < AMDGV_FFBM_MEMORY_ALLOCATION_COUNT; i++) {
		if (adapt->ffbm.memory_block[i].used == false) {
			adapt->ffbm.memory_block[i].used = true;
			oss_memset((void *)&adapt->ffbm.memory_block[i].pte_block, 0, sizeof(adapt->ffbm.memory_block[i].pte_block));
			oss_mutex_unlock(adapt->ffbm.memory_lock);
			return (struct amdgv_ffbm_pte_block *)&adapt->ffbm.memory_block[i].pte_block;
		}
	}
	oss_mutex_unlock(adapt->ffbm.memory_lock);
	return NULL;
}

static void amdgv_ffbm_free_block(void *ptr)
{
	((struct amdgv_ffbm_memory_block *)ptr)->used = false;
}

static void amdgv_ffbm_free_all_block(struct amdgv_adapter *adapt)
{
	oss_memset((void *)&adapt->ffbm.memory_block, 0, sizeof(struct amdgv_ffbm_memory_block) * AMDGV_FFBM_MEMORY_ALLOCATION_COUNT);
}

static char *amdgv_ffbm_type_name_str(uint32_t type)
{
	static char *name[] = { [AMDGV_FFBM_MEM_TYPE_INVALID] = "INVALID",
				[AMDGV_FFBM_MEM_TYPE_PF] = "PF",
				[AMDGV_FFBM_MEM_TYPE_VF] = "VF",
				[AMDGV_FFBM_MEM_TYPE_RESERVED] = "RESERVED",
				[AMDGV_FFBM_MEM_TYPE_BAD_PAGE] = "BAD PAGE",
				[AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED] = "NOT ASSIGNED",
				[AMDGV_FFBM_MEM_TYPE_TMR] = "TMR" };

	return name[type];
}

static void amdgv_ffbm_pteb_update(struct amdgv_ffbm_pte_block *pteb, uint64_t gpa,
				   uint64_t spa, uint64_t size, uint8_t frag, uint32_t vf_idx,
				   uint8_t permission, enum amdgv_ffbm_mem_type type, bool applied)
{
	pteb->gpa = gpa;
	pteb->spa = spa;
	pteb->size = size;
	pteb->type = type;
	pteb->fragment = frag;
	pteb->vf_idx = vf_idx;
	pteb->applied = applied;
	pteb->permission = permission;
}

static void amdgv_ffbm_pteb_init(struct amdgv_ffbm_pte_block *pteb, uint64_t gpa, uint64_t spa,
				 uint64_t size, uint8_t frag, uint32_t vf_idx,
				 uint8_t permission, enum amdgv_ffbm_mem_type type, bool applied)
{
	amdgv_ffbm_pteb_update(pteb, gpa, spa, size, frag, vf_idx, permission, type, applied);
	AMDGV_INIT_LIST_HEAD(&pteb->gpa_list_node);
	AMDGV_INIT_LIST_HEAD(&pteb->spa_list_node);
}

int amdgv_ffbm_apply_page_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ffbm_pte_block *pteb;
	int ret = 0;
	uint32_t vf_idx;

	if (!adapt->ffbm.enabled)
		return 0;

	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->ffbm.spa_list, struct amdgv_ffbm_pte_block,
				   spa_list_node) {
		if ((pteb->type == AMDGV_FFBM_MEM_TYPE_VF ||
		     pteb->type == AMDGV_FFBM_MEM_TYPE_TMR) &&
		    pteb->applied == false)
			ret = adapt->ffbm.apply_pteb(adapt, pteb, true);
	}

	for (vf_idx = 0; vf_idx < adapt->num_vf; vf_idx++) {
		amdgv_list_for_each_entry(pteb, &adapt->array_vf[vf_idx].gpa_list,
					   struct amdgv_ffbm_pte_block, gpa_list_node) {
			if ((pteb->type == AMDGV_FFBM_MEM_TYPE_VF ||
			     pteb->type == AMDGV_FFBM_MEM_TYPE_TMR) &&
			    pteb->applied == false)
				ret = adapt->ffbm.apply_pteb(adapt, pteb, true);
		}
	}
	FFBM_UNLOCK_LIST;
	return ret;
}

static struct amdgv_ffbm_pte_block *amdgv_ffbm_find_pteb_by_phy(struct amdgv_adapter *adapt,
								uint64_t spa)
{
	struct amdgv_ffbm_pte_block *pteb, *ret = NULL;

	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->ffbm.spa_list, struct amdgv_ffbm_pte_block,
				   spa_list_node) {
		if (AMDGV_FFBM_ADDR_IN_RANGE(pteb->spa, pteb->size, spa)) {
			ret = pteb;
			break;
		}
	}
	FFBM_UNLOCK_LIST;

	return ret;
}

static struct amdgv_ffbm_pte_block *amdgv_ffbm_find_pteb_by_ffbm(struct amdgv_adapter *adapt,
								 uint64_t gpa, uint32_t vf_idx)
{
	struct amdgv_ffbm_pte_block *pteb, *ret = NULL;

	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->array_vf[vf_idx].gpa_list,
				   struct amdgv_ffbm_pte_block, gpa_list_node) {
		if (AMDGV_FFBM_ADDR_IN_RANGE(pteb->gpa, pteb->size, gpa)) {
			ret = pteb;
			break;
		}
	}
	FFBM_UNLOCK_LIST;

	return ret;
}

static struct amdgv_ffbm_pte_block *amdgv_ffbm_find_empty_pteb(struct amdgv_adapter *adapt,
							       uint64_t min_size)
{
	struct amdgv_ffbm_pte_block *pteb, *ret = NULL;

	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->ffbm.spa_list, struct amdgv_ffbm_pte_block,
				   spa_list_node) {
		if (pteb->type == AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED && pteb->size >= min_size) {
			ret = pteb;
			break;
		}
	}
	FFBM_UNLOCK_LIST;

	return ret;
}

static struct amdgv_ffbm_pte_block *
amdgv_ffbm_pteb_divide_size(struct amdgv_adapter *adapt, struct amdgv_ffbm_pte_block *pteb,
			    uint64_t size)
{
	struct amdgv_ffbm_pte_block *new_pteb;
	uint64_t new_gpa = AMDGV_FFBM_INVALID_ADDR;

	/* check the new size is 2 MB aligned */
	if (AMDGV_FFBM_MAP_SIZE_VALID(size, pteb->fragment) != 0) {
		AMDGV_ERROR(
			"FFBM: cut PTEB size is not 2 MB aligned with spa (0x%llx) and size (0x%llx), please try a new spa\n",
			size + pteb->spa, size);
		return NULL;
	}

	size = AMDGV_FFBM_PAGE_ALIGN_CEIL(size, pteb->fragment);

	/* if pteb doesn't have enough size, then don't need to cut */
	if (pteb->size <= size)
		return pteb;

	new_pteb = amdgv_ffbm_allocate_block(adapt);
	if (!new_pteb) {
		AMDGV_ERROR("FFBM: Failed to alloc memory\n");
		return NULL;
	}

	if (pteb->gpa != AMDGV_FFBM_INVALID_ADDR)
		new_gpa = pteb->gpa + size;
	amdgv_ffbm_pteb_init(new_pteb, new_gpa, pteb->spa + size, pteb->size - size,
			     pteb->fragment, pteb->vf_idx, pteb->permission, pteb->type, pteb->applied);

	/* shink the size */
	pteb->size = size;

	/* insert to the list after pteb */
	FFBM_LOCK_LIST;
	amdgv_list_add(&new_pteb->spa_list_node, &pteb->spa_list_node);
	if (pteb->gpa != AMDGV_FFBM_INVALID_ADDR)
		amdgv_list_add(&new_pteb->gpa_list_node, &pteb->gpa_list_node);
	FFBM_UNLOCK_LIST;

	return new_pteb;
}

static struct amdgv_ffbm_pte_block *
amdgv_ffbm_create_new_pteb(struct amdgv_adapter *adapt, struct amdgv_ffbm_pte_block *pteb,
			   uint64_t gpa, uint64_t spa, uint64_t size, uint32_t vf_idx,
			   uint8_t permission, enum amdgv_ffbm_mem_type type)
{
	struct amdgv_ffbm_pte_block *new_pteb;

	new_pteb = amdgv_ffbm_allocate_block(adapt);
	if (!new_pteb) {
		AMDGV_ERROR("FFBM: Failed to alloc memory\n");
		return NULL;
	}

	amdgv_ffbm_pteb_init(new_pteb, gpa, spa, size, pteb->fragment, vf_idx, permission,
				type, false);

	return new_pteb;
}

static struct amdgv_ffbm_pte_block *
amdgv_ffbm_pteb_hole_by_phy(struct amdgv_adapter *adapt, struct amdgv_ffbm_pte_block *pteb,
			    uint64_t spa, uint64_t size)
{
	size = AMDGV_FFBM_PAGE_ALIGN_CEIL(size, pteb->fragment);

	if (!AMDGV_FFBM_ADDR_SIZE_IN_RANGE(pteb->spa, pteb->size, spa, size)) {
		/* spa not in the range */
		AMDGV_ERROR("FFBM: physical address 0x%llx, size 0x%llx not in the pteb\n",
			    spa, size);
		return NULL;
	}

	/* divide left one, change to the middle one */
	if (pteb->spa < spa)
		pteb = amdgv_ffbm_pteb_divide_size(adapt, pteb, spa - pteb->spa);

	/* divide right one */
	if (pteb && pteb->size > size)
		/* we don't need the new one */
		amdgv_ffbm_pteb_divide_size(adapt, pteb, size);

	return pteb;
}

static bool is_gpaess_valid(struct amdgv_adapter *adapt, uint64_t size, uint64_t gpa,
			    uint32_t vf_idx)
{
	uint64_t previous_end = 0, current_start = 0;
	struct amdgv_ffbm_pte_block *pteb;
	bool ret = false;

	if ((gpa >= MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET) &&
		gpa < MBYTES_TO_BYTES(adapt->tmr_size + AMDGV_FFBM_FB_TMR_OFFSET)) ||
		(gpa + size > MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET) &&
		gpa + size <= MBYTES_TO_BYTES(adapt->tmr_size + AMDGV_FFBM_FB_TMR_OFFSET))) {
		AMDGV_ERROR("FFBM: gpa is in TMR range, not valid!");
		return ret;
	}

	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->array_vf[vf_idx].gpa_list,
				   struct amdgv_ffbm_pte_block, gpa_list_node) {
		current_start = pteb->gpa;
		if (AMDGV_FFBM_ADDR_SIZE_IN_RANGE(previous_end, current_start - previous_end,
						  gpa, size)) {
			ret = true;
			goto exit;
		}
		previous_end = pteb->gpa + pteb->size;
		if (gpa < previous_end && gpa >= current_start) {
			ret = false;
			goto exit;
		}
	}

	/* empty slot between last mapped pteb and the fb end */
	current_start = adapt->ffbm.total_physical_size;
	/* range after the last pte block */
	if (AMDGV_FFBM_ADDR_SIZE_IN_RANGE(previous_end, current_start - previous_end, gpa,
					  size))
		ret = true;
exit:
	FFBM_UNLOCK_LIST;
	return ret;
}

static void amdgv_ffbm_insert_new_ffbm_node(struct amdgv_adapter *adapt,
					    struct amdgv_ffbm_pte_block *pteb)
{
	struct amdgv_ffbm_pte_block *tmp;
	/* only VF memory should be put into this list */
	AMDGV_ASSERT(pteb->type == AMDGV_FFBM_MEM_TYPE_VF ||
		     pteb->type == AMDGV_FFBM_MEM_TYPE_TMR);

	FFBM_LOCK_LIST;
	/* insert to ffbm mapped list */
	amdgv_list_for_each_entry(tmp, &adapt->array_vf[pteb->vf_idx].gpa_list,
				   struct amdgv_ffbm_pte_block, gpa_list_node) {
		if (tmp->gpa > pteb->gpa)
			break;
	}
	amdgv_list_add(&pteb->gpa_list_node, &tmp->gpa_list_node);
	FFBM_UNLOCK_LIST;
}

static int amdgv_ffbm_replace_page(struct amdgv_adapter *adapt, uint64_t spa)
{
	struct amdgv_ffbm_pte_block *pteb, *new_pteb, *reserved_pteb;
	uint64_t aligned_spa, page_size = 0;
	uint64_t bad_spa = 0;
	uint64_t bad_spa_end = 0;
	uint64_t new_spa = 0;

	/* find pteb */
	pteb = amdgv_ffbm_find_pteb_by_phy(adapt, spa);
	if (!pteb) {
		AMDGV_ERROR("FFBM: failed to find the broken page");
		return AMDGV_FFBM_ERROR_BAD_ADDR;
	}

	/* FFBM only replaces VF type memory */
	if (pteb->type != AMDGV_FFBM_MEM_TYPE_VF) {
		AMDGV_WARN("FFBM: Not support replacement %s type memory!\n", amdgv_ffbm_type_name_str(pteb->type));
		return 0;
	}

	/* ffbm page size = 2^fragment * 4KB */
	page_size = AMDGV_FFBM_PAGE_SIZE(pteb->fragment);
	aligned_spa = AMDGV_FFBM_PAGE_ALIGN(spa, pteb->fragment);

	pteb = amdgv_ffbm_pteb_hole_by_phy(adapt, pteb, aligned_spa, page_size);
	if (!pteb) {
		AMDGV_ERROR("FFBM: mark bad page failed\n");
		return AMDGV_FFBM_ERROR_NO_MEM;
	}

	/* find a reserved PTEB */
	new_pteb = NULL;
	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(reserved_pteb, &adapt->ffbm.spa_list,
				   struct amdgv_ffbm_pte_block, spa_list_node) {
		if (reserved_pteb->type == AMDGV_FFBM_MEM_TYPE_RESERVED) {
			new_pteb = reserved_pteb;
			break;
		}
	}
	FFBM_UNLOCK_LIST;
	if (!new_pteb) {
		AMDGV_ERROR("FFBM: Can't find reserved PTE for swapping\n");
		return AMDGV_FFBM_ERROR_NO_MEM;
	}

	/* Copy the data of bad pteb to new pteb */
	bad_spa = pteb->spa;
	bad_spa_end = bad_spa + page_size;
	new_spa = new_pteb->spa;

	AMDGV_DEBUG(
		"FFBM: Copy pteb data: bad_spa:0x%llx, bad_spa_end:0x%llx, new_spa:0x%llx, page_size:0x%llx\n",
		bad_spa, bad_spa_end, new_spa, page_size);

	while (bad_spa < bad_spa_end) {
		/* Skip the bad page */
		if (bad_spa == spa) {
			AMDGV_DEBUG(
				"FFBM: Skip copying bad page, bad_spa:0x%llx, new_spa:0x%llx, size:0x%x\n",
				bad_spa, new_spa, PAGE_SIZE);
			bad_spa += PAGE_SIZE;
			new_spa += PAGE_SIZE;
		} else {
			WRITE_FB32(new_spa, READ_FB32(bad_spa));
			bad_spa += 4;
			new_spa += 4;
		}
	}

	/* this new pteb is a reserved one which is already in the physical list, so just update */
	amdgv_ffbm_pteb_update(new_pteb, pteb->gpa, new_pteb->spa, page_size, pteb->fragment,
			       pteb->vf_idx, pteb->permission, pteb->type, false);

	adapt->ffbm.reserved_block_count--;

	/* not assigned block doesn't required to be removed from mapped list */
	/* insert before pteb */
	FFBM_LOCK_LIST;
	amdgv_list_add_tail(&new_pteb->gpa_list_node, &pteb->gpa_list_node);
	/* remove the bad one from the mapped ffbm list */
	FFBM_UNLOCK_LIST;

	amdgv_ffbm_unmap_pteb(adapt, pteb);

	/* update bad pteb */
	amdgv_ffbm_pteb_update(pteb, AMDGV_FFBM_INVALID_ADDR, pteb->spa, page_size,
			       pteb->fragment, AMDGV_INVALID_IDX_VF, 0,
			       AMDGV_FFBM_MEM_TYPE_BAD_PAGE, pteb->applied);

	adapt->ffbm.bad_block_count++;

	return 0;
}

int amdgv_ffbm_replace_bad_pages(struct amdgv_adapter *adapt, struct eeprom_table_record *bps,
				 int pages)
{
	int i = 0;
	int ret = 0;
	uint64_t spa = 0;

	for (i = 0; i < pages; i++) {
		spa = bps[i].retired_page << AMDGV_GPU_PAGE_SHIFT;
		ret = amdgv_ffbm_replace_page(adapt, spa);
		if (ret) {
			AMDGV_ERROR("FFBM failed to replace bad page\n");
			break;
		}
	}

	if (!ret) {
		/* Apply replaced page table */
		ret = amdgv_ffbm_apply_page_table(adapt);
		if (ret) {
			AMDGV_ERROR("FFBM failed to apply page table\n");
			return ret;
		}
	}

	return ret;
}

int amdgv_ffbm_manual_map(struct amdgv_adapter *adapt, uint32_t vf_idx, uint64_t size,
			  uint64_t gpa, uint64_t spa, uint8_t permission,
			  enum amdgv_ffbm_mem_type type)
{
	struct amdgv_ffbm_pte_block *pteb;

	AMDGV_DEBUG(
		"FFBM: Manual mapping physical_addr 0x%llx gpa 0x%llx, size 0x%llx, vf_idx %d, type %s\n",
		spa, gpa, size, vf_idx, amdgv_ffbm_type_name_str(type));

	/* check if the size is 2 MB aligned */
	if (AMDGV_FFBM_MAP_SIZE_VALID(size, adapt->ffbm.default_fragment) != 0) {
		AMDGV_ERROR(
			"FFBM: map size: (0x%llx) is not 2 MB aligned, please try a suitable size\n",
			size);
		return AMDGV_FFBM_ERROR_INVALD_SIZE;
	}

	/* check if there is overlap when mapping to VF*/
	if (type == AMDGV_FFBM_MEM_TYPE_VF && !is_gpaess_valid(adapt, size, gpa, vf_idx)) {
		AMDGV_ERROR(
			"FFBM: can't find suitable slot for ffbm address 0x%llx, size 0x%llx\n",
			gpa, size);
		return AMDGV_FFBM_ERROR_BAD_ADDR;
	}

	pteb = amdgv_ffbm_find_pteb_by_phy(adapt, spa);
	if (pteb)
		AMDGV_DEBUG("FFBM: target pteb->spa=0x%llx, size=0x%llx\n", pteb->spa,
			    pteb->size);
	else
		AMDGV_ERROR("NO PTEB\n");
	if (!pteb ||
	    (pteb->type != AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED &&
	     type != AMDGV_FFBM_MEM_TYPE_TMR) ||
	    !AMDGV_FFBM_ADDR_SIZE_IN_RANGE(pteb->spa, pteb->size, spa, size)) {
		AMDGV_ERROR(
			"FFBM: can't find suitable slot for physical address 0x%llx, size 0x%llx\n",
			spa, size);
		return AMDGV_FFBM_ERROR_BAD_ADDR;
	}

	if (pteb->type == AMDGV_FFBM_MEM_TYPE_TMR &&
	    !AMDGV_FFBM_TMR_ADDR_VALID(gpa, spa, size, adapt->tmr_size)) {
		AMDGV_ERROR(
			"FFBM: TMR mapping failed for ffbm address 0x%llx, physical address 0x%llx, size 0x%llx\n",
			gpa, spa, size);
		return AMDGV_FFBM_ERROR_BAD_ADDR;
	}

	/* tmr pteb is already exited, create new tmr pteb for vf */
	if (pteb->type == AMDGV_FFBM_MEM_TYPE_TMR) {
		pteb = amdgv_ffbm_create_new_pteb(adapt, pteb,
						  MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET),
						  MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET),
						  MBYTES_TO_BYTES(adapt->tmr_size), vf_idx,
						  permission, AMDGV_FFBM_MEM_TYPE_TMR);
	} else {
		pteb = amdgv_ffbm_pteb_hole_by_phy(adapt, pteb, spa, size);
		if (!pteb) {
			AMDGV_ERROR("FFBM: map range failed\n");
			return AMDGV_FFBM_ERROR_NO_MEM;
		}
		amdgv_ffbm_pteb_update(pteb, gpa, spa, size, adapt->ffbm.default_fragment,
				       vf_idx, permission, type, false);
	}

	if (type == AMDGV_FFBM_MEM_TYPE_VF ||
	    (pteb->type == AMDGV_FFBM_MEM_TYPE_TMR && vf_idx != AMDGV_PF_IDX))
		amdgv_ffbm_insert_new_ffbm_node(adapt, pteb);

	return 0;
}

static int amdgv_ffbm_map(struct amdgv_adapter *adapt, uint32_t vf_idx, uint64_t gpa,
			  uint64_t size, uint8_t permission, enum amdgv_ffbm_mem_type type)
{
	struct amdgv_ffbm_pte_block *pteb;
	int64_t size_left = size;
	uint64_t gpa_start = gpa;
	uint64_t min_page_size = AMDGV_FFBM_PAGE_SIZE(adapt->ffbm.default_fragment);

	AMDGV_DEBUG("FFBM: Auto mapping ffbm size 0x%llx, vf_idx %d, type %s\n", size, vf_idx,
		    amdgv_ffbm_type_name_str(type));

	if (size < min_page_size) {
		/* check if vf map minsize */
		AMDGV_WARN("FFBM: too small size to map FFBM\n");
		return AMDGV_FFBM_ERROR_BAD_ADDR;
	}

	if (type == AMDGV_FFBM_MEM_TYPE_VF && size < AMDGV_FFBM_VF_MIN_MAP_SIZE && gpa != 0) {
		/* check if vf map minsize */
		AMDGV_WARN("FFBM: too small size to map FFBM for VF\n");
		return AMDGV_FFBM_ERROR_BAD_ADDR;
	}

	/* check if there is overlap */
	if (type != AMDGV_FFBM_MEM_TYPE_VF) {
		AMDGV_ERROR("FFBM: mapping FFBM for non VF\n");
		return AMDGV_FFBM_ERROR_BAD_ADDR;
	}

	while (size_left > 0) {
		pteb = amdgv_ffbm_find_empty_pteb(adapt, 0);
		if (!pteb) {
			AMDGV_ERROR("FFBM: can't find empty pteb\n");
			return AMDGV_FFBM_ERROR_NO_MEM;
		}
		if (pteb->size > size_left)
			amdgv_ffbm_pteb_divide_size(adapt, pteb, size_left);

		amdgv_ffbm_pteb_update(pteb, gpa_start, pteb->spa, pteb->size, pteb->fragment,
				       vf_idx, permission, type, false);

		if (type == AMDGV_FFBM_MEM_TYPE_VF) {
			gpa_start += pteb->size;
			amdgv_ffbm_insert_new_ffbm_node(adapt, pteb);
		}

		size_left -= pteb->size;
	}

	return 0;
}

static void amdgv_ffbm_invalidate_and_unapply_pteb(struct amdgv_adapter *adapt,
				  struct amdgv_ffbm_pte_block *pteb)
{
	if (pteb == NULL)
		return;

	if (pteb->type == AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED)
		return;

	if (!amdgv_list_empty(&pteb->gpa_list_node)) {
		if (!pteb->applied)
			return;

		if (adapt->status != AMDGV_STATUS_SW_INIT && pteb->type == AMDGV_FFBM_MEM_TYPE_VF)
			adapt->ffbm.invalidate_tlb(adapt, pteb, true);

		/* TMR mapping for VF also needs to be unapplied. */
		if (pteb->type == AMDGV_FFBM_MEM_TYPE_VF ||
			pteb->type == AMDGV_FFBM_MEM_TYPE_TMR)
			adapt->ffbm.apply_pteb(adapt, pteb, false);
	}
}

static void amdgv_ffbm_unmap_pteb(struct amdgv_adapter *adapt,
				  struct amdgv_ffbm_pte_block *pteb)
{
	struct amdgv_ffbm_pte_block *tmp_pteb;

	if (pteb == NULL)
		return;

	if (pteb->type == AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED)
		return;

	/* 1. remove from list */
	amdgv_ffbm_invalidate_and_unapply_pteb(adapt, pteb);
	if (!amdgv_list_empty(&pteb->gpa_list_node)) {
		FFBM_LOCK_LIST;
		amdgv_list_del_init(&pteb->gpa_list_node);
		FFBM_UNLOCK_LIST;
	}

	/* 2. Remove TMR type pteb or return back VF type pteb */
	if (pteb->type == AMDGV_FFBM_MEM_TYPE_TMR) {
		/* TMR on VF is mapped from pf and cannot be changed on pf
		 * TMR pteb on VF should be freed once the pteb is removed from list. */
		amdgv_ffbm_free_block(pteb);
	} else if (pteb->type == AMDGV_FFBM_MEM_TYPE_VF) {
		/*2.1 return back VF FB to unassigned */
		amdgv_ffbm_pteb_update(pteb, AMDGV_FFBM_INVALID_ADDR, pteb->spa, pteb->size,
				       adapt->ffbm.default_fragment, AMDGV_INVALID_IDX_VF, 0,
				       AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED, pteb->applied);

		/* 2.2 try merge */
		/* front */
		tmp_pteb = amdgv_list_entry(pteb->spa_list_node.prev,
					    struct amdgv_ffbm_pte_block, spa_list_node);
		if (tmp_pteb->type == AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED) {
			tmp_pteb->size += pteb->size;
			FFBM_LOCK_LIST;
			amdgv_list_del(&pteb->spa_list_node);
			FFBM_UNLOCK_LIST;
			amdgv_ffbm_free_block(pteb);
			pteb = tmp_pteb;
		}

		/* back */
		tmp_pteb = amdgv_list_entry(pteb->spa_list_node.next,
					    struct amdgv_ffbm_pte_block, spa_list_node);
		if (tmp_pteb->type == AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED) {
			pteb->size += tmp_pteb->size;
			FFBM_LOCK_LIST;
			amdgv_list_del(&tmp_pteb->spa_list_node);
			FFBM_UNLOCK_LIST;
			amdgv_ffbm_free_block(tmp_pteb);
		}
	}
}

static void amdgv_ffbm_unmap_by_spa(struct amdgv_adapter *adapt, uint64_t spa)
{
	struct amdgv_ffbm_pte_block *pteb;
	pteb = amdgv_ffbm_find_pteb_by_phy(adapt, spa);
	if (pteb)
		amdgv_ffbm_unmap_pteb(adapt, pteb);
}

void amdgv_ffbm_unmap_by_fcn(struct amdgv_adapter *adapt, uint32_t vf_idx, bool reserve)
{
	struct amdgv_ffbm_pte_block *pteb;
	bool found = false;
	struct amdgv_vf_device *entry = &adapt->array_vf[vf_idx];

	if (!adapt->ffbm.enabled)
		return;

	AMDGV_ASSERT(vf_idx !=
		     AMDGV_INVALID_IDX_VF); /* the invalid part shouldn't be unmapped */
	do {
		found = false;
		FFBM_LOCK_LIST;
		amdgv_list_for_each_entry(pteb, &entry->gpa_list, struct amdgv_ffbm_pte_block,
					   gpa_list_node) {
			if (pteb->vf_idx == vf_idx && pteb->applied) {
				found = true;
				break;
			}
		}
		FFBM_UNLOCK_LIST;
		if (found) {
			if (reserve)
				/* Invalidate ffbm mapping but reserve the pteb in the list */
				amdgv_ffbm_invalidate_and_unapply_pteb(adapt, pteb);
			else
				amdgv_ffbm_unmap_pteb(adapt, pteb);
		}
	} while (found);
}

/* copy content to gpa with FFBM enabled */
uint64_t amdgv_ffbm_copy_to_gpa(struct amdgv_adapter *adapt, uint32_t *data, uint32_t idx_vf,
				uint64_t gpa, uint64_t size, enum amdgv_ffbm_copy type)
{
	struct amdgv_ffbm_pte_block *pteb;
	uint64_t size_left = size;
	uint64_t size_copy = 0;
	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->array_vf[idx_vf].gpa_list,
				   struct amdgv_ffbm_pte_block, gpa_list_node) {
		if (AMDGV_FFBM_ADDR_IN_RANGE(pteb->gpa, pteb->size, gpa)) {
			size_copy = min(pteb->size - (gpa - pteb->gpa), size_left);
			/* copy using MM_INDEX/DATA regs */
			if (type == AMDGV_FFBM_MM_COPY)
				amdgv_mm_copy_to_fb(adapt, pteb->spa + (gpa - pteb->gpa),
						(uint64_t)&data[((size - size_left) / 4)], size_copy);
			/* copy using PF bar */
			else if (type == AMDGV_FFBM_PF_COPY)
				oss_memcpy((void *)((uint64_t)adapt->fb + pteb->spa + (gpa - pteb->gpa)),
						&data[((size - size_left) / 4)], size_copy);
			else
				break;
			size_left -= size_copy;
			gpa += size_copy;
		}
		if (size_left == 0)
			break;
	}
	FFBM_UNLOCK_LIST;
	return size_left;
}

/* copy content from gpa with FFBM enabled */
uint64_t amdgv_ffbm_copy_from_gpa(struct amdgv_adapter *adapt, uint32_t *data, uint32_t idx_vf,
				uint64_t gpa, uint64_t size, enum amdgv_ffbm_copy type)
{
	struct amdgv_ffbm_pte_block *pteb;
	uint64_t size_left = size;
	uint64_t size_copy = 0;
	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->array_vf[idx_vf].gpa_list,
				   struct amdgv_ffbm_pte_block, gpa_list_node) {
		if (AMDGV_FFBM_ADDR_IN_RANGE(pteb->gpa, pteb->size, gpa)) {
			size_copy = min(pteb->size - (gpa - pteb->gpa), size_left);
			/* copy using MM_INDEX/DATA regs */
			if (type == AMDGV_FFBM_MM_COPY)
				amdgv_mm_copy_from_fb(adapt, (uint64_t)&data[((size - size_left) / 4)],
						pteb->spa + (gpa - pteb->gpa), size_copy);
			/* copy using PF bar */
			else if (type == AMDGV_FFBM_PF_COPY)
				oss_memcpy(&data[((size - size_left) / 4)],
						(void *)((uint64_t)adapt->fb + pteb->spa + (gpa - pteb->gpa)), size_copy);
			else
				break;
			size_left -= size_copy;
			gpa += size_copy;
		}
		if (size_left == 0)
			break;
	}
	FFBM_UNLOCK_LIST;
	return size_left;
}

uint64_t amdgv_ffbm_gpa_to_spa(struct amdgv_adapter *adapt, uint64_t gpa, uint32_t vf_idx)
{
	struct amdgv_ffbm_pte_block *pteb;
	pteb = amdgv_ffbm_find_pteb_by_ffbm(adapt, gpa, vf_idx);
	if (pteb)
		return pteb->spa + (gpa - pteb->gpa);
	return AMDGV_FFBM_INVALID_ADDR;
}

uint64_t amdgv_ffbm_spa_to_gpa(struct amdgv_adapter *adapt, uint64_t spa, uint32_t *vf_idx)
{
	struct amdgv_ffbm_pte_block *pteb;

	pteb = amdgv_ffbm_find_pteb_by_phy(adapt, spa);
	if (pteb) {
		*vf_idx = pteb->vf_idx;
		return (pteb->gpa + (spa - pteb->spa));
	}
	return AMDGV_FFBM_INVALID_ADDR;
}

int amdgv_ffbm_sw_init(struct amdgv_adapter *adapt)
{
	int i;
	/* sw init */
	adapt->ffbm.memory_lock = oss_mutex_init();
	adapt->ffbm.pt_lock = oss_mutex_init();
	for (i = 0; i < AMDGV_MAX_VF_SLOT; i++) {
		AMDGV_INIT_LIST_HEAD(&adapt->array_vf[i].gpa_list);
	}
	AMDGV_INIT_LIST_HEAD(&adapt->ffbm.spa_list);
	adapt->ffbm.bad_block_count = 0;
	adapt->ffbm.enabled = true;
	return 0;
}

int amdgv_ffbm_sw_fini(struct amdgv_adapter *adapt)
{
	/* sw fini */
	if (adapt->ffbm.pt_lock)
		oss_mutex_fini(adapt->ffbm.pt_lock);
	if (adapt->ffbm.memory_lock)
		oss_mutex_fini(adapt->ffbm.memory_lock);
	return 0;
}

/* update FFBM page table according to PF/VF FB settings */
int amdgv_ffbm_page_table_update_by_fcn(struct amdgv_adapter *adapt, uint32_t vf_idx)
{
	int ret = 0;
	struct amdgv_vf_device *entry = &adapt->array_vf[vf_idx];
	uint32_t fb_size = 0;

	if (!adapt->ffbm.enabled)
		return 0;

	if (vf_idx == AMDGV_PF_IDX) {
		/* can't use fcn because there are some PF reserved mem in the end of the FB */
		amdgv_ffbm_unmap_by_spa(adapt,
					MBYTES_TO_BYTES(adapt->array_vf[vf_idx].fb_offset));
		amdgv_ffbm_unmap_by_spa(
			adapt, MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET + adapt->tmr_size));

		/* pf needs manual map to the front */
		ret = amdgv_ffbm_manual_map(adapt, vf_idx,
					    MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET -
							    adapt->array_vf[vf_idx].fb_offset),
					    MBYTES_TO_BYTES(adapt->array_vf[vf_idx].fb_offset),
					    MBYTES_TO_BYTES(adapt->array_vf[vf_idx].fb_offset),
					    0, AMDGV_FFBM_MEM_TYPE_PF);
		/* TMR region is alredy mappped in hw init*/
		AMDGV_INFO("FFBM tmr size: %d MB, left pf size %d MB\n", adapt->tmr_size,
			   adapt->array_vf[vf_idx].fb_size - AMDGV_FFBM_FB_TMR_OFFSET -
				   adapt->tmr_size + adapt->array_vf[vf_idx].fb_offset);
		ret = amdgv_ffbm_manual_map(
			adapt, vf_idx,
			MBYTES_TO_BYTES(adapt->array_vf[vf_idx].fb_size - adapt->tmr_size -
					AMDGV_FFBM_FB_TMR_OFFSET +
					adapt->array_vf[vf_idx].fb_offset),
			MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET + adapt->tmr_size),
			MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET + adapt->tmr_size), 0,
			AMDGV_FFBM_MEM_TYPE_PF);
	} else {
		/* VF FFBM should be already unmapped */
		/* don't map ffbm if size is 0 */
		fb_size = (adapt->ffbm.share_tmr) ? entry->fb_size_tmr : entry->fb_size;
		if (fb_size > 0) {
			/* VF gap [0x200000, 0x200000+TMR size] map to PF TMR [0x200000, 0x200000+TMR size] -- manual map */
			ret = amdgv_ffbm_manual_map(adapt, vf_idx,
						    MBYTES_TO_BYTES(adapt->tmr_size),
						    MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET),
						    MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET),
						    AMDGV_FFBM_PERM_RW, AMDGV_FFBM_MEM_TYPE_TMR);

			ret = amdgv_ffbm_map(adapt, vf_idx, 0,
					     MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET),
					     AMDGV_FFBM_PERM_RW, AMDGV_FFBM_MEM_TYPE_VF);

			ret = amdgv_ffbm_map(
				adapt, vf_idx,
				MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET + adapt->tmr_size),
				MBYTES_TO_BYTES(adapt->array_vf[vf_idx].fb_size -
						AMDGV_FFBM_FB_TMR_OFFSET),
				AMDGV_FFBM_PERM_RW, AMDGV_FFBM_MEM_TYPE_VF);
		}
		amdgv_ffbm_apply_page_table(adapt);
	}

	return ret;
}

int amdgv_ffbm_page_table_init(struct amdgv_adapter *adapt)
{
	struct amdgv_ffbm_pte_block *pteb;
	int i;
	uint64_t avail_size;
	uint64_t aligned_size, top_size, page_size, resv_top_offset; /* temp values */

	/* destroy previous page table if exists */
	amdgv_ffbm_page_table_destroy(adapt);
	amdgv_ffbm_free_all_block(adapt);

	/* basic setup */
	/* 1. Unused assignment */
	page_size = AMDGV_FFBM_PAGE_SIZE(adapt->ffbm.default_fragment);

	avail_size = MBYTES_TO_BYTES(adapt->gpuiov.total_fb_avail);
	aligned_size = AMDGV_FFBM_PAGE_ALIGN(avail_size, adapt->ffbm.default_fragment);
	AMDGV_ASSERT(
		aligned_size ==
		avail_size); /* if not equal, then there is a risk that tom is higher than aligned size in step 3 */

	pteb = amdgv_ffbm_allocate_block(adapt);
	if (!pteb) {
		AMDGV_ERROR("FFBM: no memory, failed to setup FFBM table");
		return AMDGV_FFBM_ERROR_NO_MEM;
	}
	adapt->ffbm.total_physical_size = aligned_size;
	amdgv_ffbm_pteb_init(pteb, 0, 0, aligned_size, adapt->ffbm.default_fragment,
			     AMDGV_INVALID_IDX_VF, 0, AMDGV_FFBM_MEM_TYPE_NOT_ASSIGNED, false);

	FFBM_LOCK_LIST;
	amdgv_list_add(&pteb->spa_list_node, &adapt->ffbm.spa_list);
	FFBM_UNLOCK_LIST;

	/* 2. top fb, which reserved for gpu functionalities*/
	top_size = aligned_size - MBYTES_TO_BYTES(adapt->gpuiov.total_fb_usable);
	amdgv_ffbm_manual_map(adapt, AMDGV_INVALID_IDX_VF, top_size, AMDGV_FFBM_INVALID_ADDR,
			      aligned_size - top_size, 0, AMDGV_FFBM_MEM_TYPE_PF);

	/* 3. reserved fb for bad page*/
	for (i = 0; i < adapt->ffbm.max_reserved_block; i++) {
		resv_top_offset = top_size + page_size * (i + 1);
		amdgv_ffbm_manual_map(adapt, AMDGV_INVALID_IDX_VF, page_size,
				      AMDGV_FFBM_INVALID_ADDR, aligned_size - resv_top_offset,
				      0, AMDGV_FFBM_MEM_TYPE_RESERVED);
		/* reserve fb for bad page */
		adapt->gpuiov.total_fb_usable -= (page_size >> 20);
	}

	/* 4. map PF FB TMR region */
	amdgv_ffbm_manual_map(adapt, AMDGV_PF_IDX, adapt->psp.allocated_tmr_size,
			      MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET),
			      MBYTES_TO_BYTES(AMDGV_FFBM_FB_TMR_OFFSET), 0,
			      AMDGV_FFBM_MEM_TYPE_TMR);

	return 0;
}

void amdgv_ffbm_reserve_all_bad_pages(struct amdgv_adapter *adapt)
{
	int ret, i;
	uint32_t bp_cnt;
	struct eeprom_table_record *record;

	record = oss_zalloc(sizeof(*record));
	if (!record) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct eeprom_table_record));
		return;
	}

	amdgv_umc_badpages_count_read(adapt, &bp_cnt);

	for (i = 0; i < bp_cnt; i++) {
		ret = amdgv_umc_get_badpages_record(adapt, i, (void *)record);
		if (ret) {
			AMDGV_WARN("FFBM: failed to query bad page info\n");
			continue;
		}

		ret = amdgv_ffbm_replace_bad_pages(adapt, record, 1);
		if (ret) {
			AMDGV_ERROR("FFBM: failed to reserve bad page\n");
			break;
		}
	}

	oss_free(record);
}

void amdgv_ffbm_page_table_destroy(struct amdgv_adapter *adapt)
{
	struct amdgv_ffbm_pte_block *pteb, *tmp;
	uint32_t vf_idx;
	struct amdgv_list_head *head;

	FFBM_LOCK_LIST;

	for (vf_idx = 0; vf_idx < adapt->num_vf; vf_idx++) {
		head = &adapt->array_vf[vf_idx].gpa_list;
		if (!amdgv_list_empty(head)) {
			amdgv_list_for_each_entry_safe(
				pteb, tmp, head, struct amdgv_ffbm_pte_block, gpa_list_node) {
				amdgv_list_del(&pteb->gpa_list_node);
				amdgv_list_del(&pteb->spa_list_node);
				amdgv_ffbm_free_block(pteb);
			}
		}
		AMDGV_INIT_LIST_HEAD(&adapt->array_vf[vf_idx].gpa_list);
	}

	head = &adapt->ffbm.spa_list;
	if (!amdgv_list_empty(head)) {
		amdgv_list_for_each_entry_safe(pteb, tmp, head, struct amdgv_ffbm_pte_block,
						spa_list_node) {
			amdgv_list_del(&pteb->spa_list_node);
			amdgv_ffbm_free_block(pteb);
		}
	}

	AMDGV_INIT_LIST_HEAD(&adapt->ffbm.spa_list);
	FFBM_UNLOCK_LIST;
}

void amdgv_ffbm_dump_page_table(struct amdgv_adapter *adapt)
{
	struct amdgv_ffbm_pte_block *pteb;
	AMDGV_ERROR("FFBM list dump start\n");
	AMDGV_INFO("Physical List:");
	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->ffbm.spa_list, struct amdgv_ffbm_pte_block,
				   spa_list_node) {
		AMDGV_INFO(
			"\tEntry:\n\t\tphysical: 0x%llx,\n\t\tgpa:0x%llx,\n\t\tsize: 0x%llx,\n\t\tvf:%d,\n\t\ttype:%s",
			pteb->spa, pteb->gpa, pteb->size, pteb->vf_idx,
			amdgv_ffbm_type_name_str(pteb->type));
	}
	FFBM_UNLOCK_LIST;
}

void amdgv_ffbm_read_page_table(struct amdgv_adapter *adapt, char *page_table_content,
				int restore_length, int *len)
{
	struct amdgv_ffbm_pte_block *pteb;
	int length, ret;
	bool dump_table = false;
	length = 0;
	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->ffbm.spa_list, struct amdgv_ffbm_pte_block,
				   spa_list_node) {
		if (length >= restore_length) {
			AMDGV_INFO("FFBM list exceeds 4 KB\n");
			dump_table = true;
			break;
		}

		ret = oss_vsnprintf(
			page_table_content + length, restore_length - length,
			"Entry:\n\tphysical: 0x%llx,\n\tgpa:0x%llx,\n\tsize: 0x%llx,\n\tvf:%d,\n\ttype:%s\n",
			pteb->spa, pteb->gpa, pteb->size, pteb->vf_idx,
			amdgv_ffbm_type_name_str(pteb->type));
		length += ret;
	}
	*len = length;
	FFBM_UNLOCK_LIST;
	if (dump_table)
		amdgv_ffbm_dump_page_table(adapt);
	return;
}

void amdgv_ffbm_copy_page_table(struct amdgv_adapter *adapt, void *page_table_content, int max_num, int *len)
{
	struct amdgv_ffbm_pte_block *pteb;
	char *pteb_buff = page_table_content;
	int index = 0;
	int pte_size = sizeof(struct amdgv_ffbm_pte_block);

	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->ffbm.spa_list, struct amdgv_ffbm_pte_block,
				   spa_list_node) {
		oss_memcpy(pteb_buff, pteb, pte_size);
		pteb_buff += pte_size;

		if (++index == max_num)
			break;
	}
	*len = index * pte_size;
	FFBM_UNLOCK_LIST;
}

enum amdgv_live_info_status amdgv_ffbm_export_spa(struct amdgv_adapter *adapt, struct amdgv_live_info_ffbm *ffbm_info)
{
	struct amdgv_ffbm_pte_block *pteb = NULL;
	uint32_t block_idx = 0;

	FFBM_LOCK_LIST;
	amdgv_list_for_each_entry(pteb, &adapt->ffbm.spa_list, struct amdgv_ffbm_pte_block, spa_list_node) {
		ffbm_info->blocks[block_idx].gpa = pteb->gpa;
		ffbm_info->blocks[block_idx].spa = pteb->spa;
		ffbm_info->blocks[block_idx].size = pteb->size;
		ffbm_info->blocks[block_idx].fragment = pteb->fragment;
		ffbm_info->blocks[block_idx].permission = pteb->permission;
		ffbm_info->blocks[block_idx].vf_idx = pteb->vf_idx;
		ffbm_info->blocks[block_idx].type = pteb->type;
		ffbm_info->blocks[block_idx].applied = pteb->applied;
		block_idx += 1;
	}
	FFBM_UNLOCK_LIST;

	ffbm_info->used_blocks = block_idx;
	ffbm_info->bad_block_count = adapt->ffbm.bad_block_count;
	ffbm_info->reserved_block_count = adapt->ffbm.reserved_block_count;
	ffbm_info->default_fragment = adapt->ffbm.default_fragment;
	ffbm_info->max_reserved_block = adapt->ffbm.max_reserved_block;
	ffbm_info->share_tmr = adapt->ffbm.share_tmr;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_ffbm_import_spa_and_gpa(struct amdgv_adapter *adapt, struct amdgv_live_info_ffbm *ffbm_info)
{
	struct amdgv_ffbm_pte_block *new_pteb = NULL;
	struct amdgv_ffbm_pte_block *cur_pteb = NULL;
	uint32_t block_idx;
	uint32_t num_active_vfs = 0;

	struct amdgv_ffbm_pte_block *pf_tmr_pteb = NULL;

	FFBM_LOCK_LIST;
	for (block_idx = 0; block_idx < ffbm_info->used_blocks; block_idx++) {
		new_pteb = amdgv_ffbm_allocate_block(adapt);
		if (!new_pteb) {
			AMDGV_ERROR("FFBM: Failed to alloc memory\n");
			FFBM_UNLOCK_LIST;
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}

		amdgv_ffbm_pteb_init(new_pteb, ffbm_info->blocks[block_idx].gpa, ffbm_info->blocks[block_idx].spa,
					ffbm_info->blocks[block_idx].size, ffbm_info->blocks[block_idx].fragment, ffbm_info->blocks[block_idx].vf_idx,
					ffbm_info->blocks[block_idx].permission, ffbm_info->blocks[block_idx].type, ffbm_info->blocks[block_idx].applied);

		// GPA FFBM page table addresses are ordered monotonic decreasing
		if (new_pteb->vf_idx < adapt->num_vf) {
			cur_pteb = NULL;
			amdgv_list_for_each_entry(cur_pteb, &adapt->array_vf[new_pteb->vf_idx].gpa_list, struct amdgv_ffbm_pte_block, gpa_list_node) {
				if (cur_pteb->gpa > new_pteb->gpa)
					break;
			}
			amdgv_list_add(&new_pteb->gpa_list_node, &cur_pteb->gpa_list_node);
		}

		// SPA FFBM page table addresses are ordered monotonic increasing
		cur_pteb = NULL;
		amdgv_list_for_each_entry(cur_pteb, &adapt->ffbm.spa_list, struct amdgv_ffbm_pte_block, spa_list_node) {}
		amdgv_list_add_tail(&new_pteb->spa_list_node, &cur_pteb->spa_list_node);

		if (ffbm_info->blocks[block_idx].type == AMDGV_FFBM_MEM_TYPE_TMR) {
			adapt->tmr_size = TO_MBYTES(ffbm_info->blocks[block_idx].size);
			pf_tmr_pteb = new_pteb;
		}

		// Update number of active VFs, so that we know which VFs to add shared TMR
		if (ffbm_info->blocks[block_idx].vf_idx < adapt->num_vf)
			num_active_vfs = (num_active_vfs) > (ffbm_info->blocks[block_idx].vf_idx + 1) ? (num_active_vfs) : (ffbm_info->blocks[block_idx].vf_idx + 1);
	}

	// Add shared TMR to each active VF GPA page table
	if (ffbm_info->share_tmr) {
		if (pf_tmr_pteb) {
			uint32_t vf_idx;
			struct amdgv_ffbm_pte_block *tmr_pteb;

			for (vf_idx = 0; vf_idx < num_active_vfs; vf_idx++) {
				tmr_pteb = amdgv_ffbm_allocate_block(adapt);
				if (!tmr_pteb) {
					AMDGV_ERROR("FFBM: Failed to alloc memory\n");
					FFBM_UNLOCK_LIST;
					return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
				}

				oss_memcpy(tmr_pteb, pf_tmr_pteb, sizeof(struct amdgv_ffbm_pte_block));
				tmr_pteb->vf_idx = vf_idx;

				cur_pteb = NULL;
				amdgv_list_for_each_entry(cur_pteb, &adapt->array_vf[vf_idx].gpa_list, struct amdgv_ffbm_pte_block, gpa_list_node) {
					if (cur_pteb->gpa > tmr_pteb->gpa)
						break;
				}
				amdgv_list_add(&tmr_pteb->gpa_list_node, &cur_pteb->gpa_list_node);
			}
		} else {
			AMDGV_ERROR("shared TMR enabled, yet PF TMR not found during live update import\n");
			FFBM_UNLOCK_LIST;
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}
	}
	FFBM_UNLOCK_LIST;

	adapt->ffbm.bad_block_count = ffbm_info->bad_block_count;
	adapt->ffbm.reserved_block_count = ffbm_info->reserved_block_count;
	adapt->ffbm.default_fragment = ffbm_info->default_fragment;
	adapt->ffbm.max_reserved_block = ffbm_info->max_reserved_block;
	adapt->ffbm.share_tmr = ffbm_info->share_tmr;

	adapt->ffbm.total_physical_size = AMDGV_FFBM_PAGE_ALIGN(MBYTES_TO_BYTES(adapt->gpuiov.total_fb_avail), adapt->ffbm.default_fragment);
	adapt->gpuiov.total_fb_usable -= (AMDGV_FFBM_PAGE_SIZE(adapt->ffbm.default_fragment) >> 20) * adapt->ffbm.max_reserved_block;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

int amdgv_get_vf_fb_mapping_list(struct amdgv_adapter *adapt,
			uint32_t vf_idx, struct amdgv_vf_ffbm_map_list *list, bool include_tmr_block)
{
	struct amdgv_ffbm_pte_block *pteb;
	int ret = 0;
	uint32_t i, j, index = 0;
	bool swapped = true;
	struct ffbm_map_entry temp_entry;

	list->count = 0;
	list->ffbm_enabled = adapt->ffbm.enabled ? 1 : 0;
	if (list->ffbm_enabled && vf_idx < AMDGV_PF_IDX) {
		FFBM_LOCK_LIST;
		amdgv_list_for_each_entry(pteb, &adapt->array_vf[vf_idx].gpa_list,
					struct amdgv_ffbm_pte_block, gpa_list_node) {
			if (pteb->type != AMDGV_FFBM_MEM_TYPE_TMR || include_tmr_block) {
				if (index < FFBM_MAP_ENTRY_MAX_COUNT) {
					list->entry[index].gpa = pteb->gpa;
					list->entry[index].spa = pteb->spa;
					list->entry[index].size = pteb->size;
					++index;
				} else {
					AMDGV_ERROR("FFBM VF teb list count exceeds %d\n",
								FFBM_MAP_ENTRY_MAX_COUNT);
					break;
				}
			}
		}
		FFBM_UNLOCK_LIST;

		list->count = index;
		// Sort the array by ascending "gpa" using Bubble Sort
		if (list->count > 1) {
			for (i = 0;  i < list->count - 1 && swapped; i++) {
				swapped = false;
				for (j = 0; j < list->count - i - 1; j++) {
					if (list->entry[j].gpa > list->entry[j + 1].gpa) {
						temp_entry = list->entry[j + 1];
						list->entry[j + 1] = list->entry[j];
						list->entry[j] = temp_entry;
						swapped = true;
					}
				}
			}
		}
	} else {
		ret = AMDGV_FAILURE;
	}
	return ret;
}
