/*
 * Copyright (c) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amdgv_oss_wrapper.h"
#include "amdgv_list.h"
#include "amdgv_device.h"
#include "amdgv_memmgr.h"
#include "amdgv_wb_memory.h"
#include "amdgv_gart.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

#define DEFAULT_ALIGN_SHIFT		12
#define DEFAULT_SIZE			(0x1ULL << 36)
#define AMDGV_MEMMGR_ALIGN(addr, align) (((addr) + (align)-1) & ~(align - 1))

static const char *amdgv_mem_id_name(uint32_t id)
{
	static char buf[255] = { 0 };

	switch (MEM_ID_GET_ID(id)) {
	case MEM_PSP_CMD_BUF_0:
	case MEM_PSP_CMD_BUF_1:
	case MEM_PSP_CMD_BUF_2:
	case MEM_PSP_CMD_BUF_3:
	case MEM_PSP_CMD_BUF_4:
	case MEM_PSP_CMD_BUF_5:
	case MEM_PSP_CMD_BUF_6:
	case MEM_PSP_CMD_BUF_7:
	case MEM_PSP_CMD_BUF_8:
	case MEM_PSP_CMD_BUF_9:
	case MEM_PSP_CMD_BUF_10:
	case MEM_PSP_CMD_BUF_11:
	case MEM_PSP_CMD_BUF_12:
	case MEM_PSP_CMD_BUF_13:
	case MEM_PSP_CMD_BUF_14:
	case MEM_PSP_CMD_BUF_15:
		oss_vsnprintf(buf, sizeof(buf), "PSP_CMD%d", (MEM_ID_GET_ID(id) - MEM_PSP_CMD_BUF_0));
		return buf;
	case MEM_PSP_RING:
		return "PSP_RING";
	case MEM_PSP_FENCE:
		return "PSP_FENCE";
	case MEM_PSP_PRIVATE:
		return "PSP_PRIVATE";
	case MEM_PSP_RAS:
		return "PSP_RAS";
	case MEM_PSP_TMR:
		return "PSP_TMR";
	case MEM_PSP_XGMI:
		return "PSP_XGMI";
	case MEM_PSP_VBFLASH:
		return "PSP_VBFLASH";
	case MEM_PSP_DUMMY:
		return "PSP_DUMMY";
	/* SMU */
	case MEM_SMC_PPTABLE:
		return "SMC_PPTABLE";
	case MEM_SMC_WATERMARK_TABLE:
		return "SMC_WATERMARK_TABLE";
	case MEM_SMC_METRICS_TABLE:
		return "SMC_METRICS_TABLE";
	case MEM_SMU_CONFIG_TABLE:
		return "SMU_CONFIG_TABLE";
	case MEM_SMC_OVERRIDE_TABLE:
		return "SMC_OVERRIDE_TABLE";
	case MEM_SMC_I2C_TABLE:
		return "SMC_I2C_TABLE";
	case MEM_SMC_ACTIVITY_TABLE:
		return "SMC_ACTIVITY_TABLE";
	case MEM_SMC_PM_STATUS_TABLE:
		return "SMC_PM_STATUS_TABLE";
	/* GPUIOV */
	case MEM_GPUIOV_CSA:
		return "GPUIOV_CSA";
	case MEM_GPUIOV_SCHED_LOG:
		return "GPUIOV_SCHED_LOG";
	case MEM_GPUIOV_SCHED_CFG_DESC:
		return "GPUIOV_SCHED_CFG_DESC";
	/* IRGMGR */
	case MEM_IRQMGR_IH_RING:
		return "IRQMGR_IH_RING";
	/* ECC */
	case MEM_ECC_BAD_PAGE:
		return "ECC_BAD_PAGE";
	case MEM_ECC_BAD_PAGE_NPS_CANDIDATE:
		return "MEM_ECC_BAD_PAGE_NPS_CANDIDATE";
	/* MMSCH */
	case MEM_MMSCH_CMD_BUFFER:
		return "MMSCH_CMD_BUFFER";
	case MEM_MMSCH_BW_CFG:
		return "MEM_MMSCH_BW_CFG";
	case MEM_MMSCH_SRAM_DUMP:
		return "MEM_MMSCH_SRAM_DUMP";
	/* GFX */
	case MEM_GFX_WB:
		return "GFX_WB";
	case MEM_GFX_EOP:
		return "GFX_EOP";
	case MEM_GFX_MQD:
		return "GFX_MQD";
	case MEM_GFX_IB:
		return "GFX_IB";
	case MEM_KIQ_RING:
		return "MEM_KIQ_RING";
	case MEM_KIQ_MQD:
		return "MEM_KIQ_MQD";
	case MEM_COMPUTE0_RING:
		return "MEM_COMPUTE0_RING";
	case MEM_COMPUTE1_RING:
		return "MEM_COMPUTE1_RING";
	case MEM_COMPUTE2_RING:
		return "MEM_COMPUTE2_RING";
	case MEM_COMPUTE3_RING:
		return "MEM_COMPUTE3_RING";
	case MEM_COMPUTE0_MQD:
		return "MEM_COMPUTE0_MQD";
	case MEM_COMPUTE1_MQD:
		return "MEM_COMPUTE1_MQD";
	case MEM_COMPUTE2_MQD:
		return "MEM_COMPUTE2_MQD";
	case MEM_COMPUTE3_MQD:
		return "MEM_COMPUTE3_MQD";
	case MEM_SDMA0_RING:
		return "MEM_SDMA0_RING";
	case MEM_SDMA1_RING:
		return "MEM_SDMA1_RING";
	case MEM_SDMA0_MQD:
		return "MEM_SDMA0_MQD";
	case MEM_SDMA1_MQD:
		return "MEM_SDMA1_MQD";

	/* Live migration */
	case MEM_MIGRATION_PSP_STATIC_DATA:
		return "MEM_MIGRATION_PSP_STATIC_DATA";
	case MEM_MIGRATION_PSP_DYNAMIC_DATA:
		return "MEM_MIGRATION_PSP_DYNAMIC_DATA";

	case MEM_VF_INITD_H_TABLE:
		return "MEM_VF_INITD_H_TABLE";
	case MEM_VF_IPD_TABLE:
		return "MEM_VF_IPD_TABLE";
	case MEM_VF_VBIOS_IMG:
		return "MEM_VF_VBIOS_IMG";
	case MEM_VF_RAS_TELEMETRY_TABLE:
		return "MEM_VF_RAS_TELEMETRY_TABLE";
	case MEM_VF_DATAEXCHANGE_TABLE:
		return "MEM_VF_DATAEXCHANGE_TABLE";
	case MEM_VF_BAD_PAGE_INFO_TABLE:
		return "MEM_VF_BAD_PAGE_INFO_TABLE";

	default:
		return "UNKNOWN";
	}
}

int amdgv_memmgr_init(struct amdgv_adapter *adapt, struct amdgv_memmgr *memmgr,
		      uint64_t offset, uint64_t size, uint32_t align, bool down)
{
	if (!memmgr) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_NO_FB_MANAGER,
				sizeof(struct amdgv_memmgr_mem));
		return AMDGV_FAILURE;
	}

	memmgr->offset = offset;
	memmgr->size = (size == 0) ? DEFAULT_SIZE : size;
	memmgr->align = (align == 0) ? DEFAULT_ALIGN_SHIFT : align;
	memmgr->down = down;

	memmgr->lock = oss_mutex_init();
	if (memmgr->lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		return AMDGV_FAILURE;
	}

	memmgr->allocs = oss_malloc(sizeof(struct amdgv_memmgr_mem));
	if (!memmgr->allocs) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct amdgv_memmgr_mem));
		oss_mutex_fini(memmgr->lock);
		memmgr->lock = OSS_INVALID_HANDLE;
		return AMDGV_FAILURE;
	}

	AMDGV_INIT_LIST_HEAD(&memmgr->allocs->node);
	memmgr->allocs->alloc_off = offset;
	memmgr->allocs->len = 0;
	memmgr->allocs->memmgr = memmgr;

	memmgr->reserves = oss_malloc(sizeof(struct amdgv_memmgr_mem));
	if (!memmgr->reserves) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_RESERVE_SYSTEM_MEM_FAIL,
				sizeof(struct amdgv_memmgr_mem));
		oss_mutex_fini(memmgr->lock);
		memmgr->lock = OSS_INVALID_HANDLE;
		return AMDGV_FAILURE;
	}

	AMDGV_INIT_LIST_HEAD(&memmgr->reserves->node);
	memmgr->reserves->alloc_off = offset;
	memmgr->reserves->len = 0;
	memmgr->reserves->memmgr = memmgr;
	memmgr->reserve_count = 0;

	memmgr->tom = offset;

	memmgr->adapt = adapt;

	memmgr->is_init = true;

	AMDGV_DEBUG("Init memmgr base: 0x%09llx size: 0x%09llx\n", memmgr->offset,
		    memmgr->size);

	return 0;
}

static struct amdgv_memmgr_mem *amdgv_memmgr_find_size(struct amdgv_memmgr *memmgr,
						       uint64_t size, uint64_t align)
{
	struct amdgv_list_head *head;
	struct amdgv_memmgr_mem *alloc;

	uint64_t tom = AMDGV_MEMMGR_ALIGN(memmgr->offset, align);

	head = &memmgr->allocs->node;

	if (amdgv_list_empty(head))
		return memmgr->allocs;

	amdgv_list_for_each_entry (alloc, head, struct amdgv_memmgr_mem, node) {
		if (tom > alloc->alloc_off)
			continue;
		if ((alloc->alloc_off - alloc->len) > (tom + size))
			return amdgv_list_last_entry(&alloc->node, struct amdgv_memmgr_mem,
						     node);
		tom = AMDGV_MEMMGR_ALIGN(alloc->alloc_off, align);
	}
	return amdgv_list_last_entry(&alloc->node, struct amdgv_memmgr_mem, node);
}

/* Find memory with specific mem_id */
static struct amdgv_memmgr_mem *amdgv_memmgr_find_mem_id(struct amdgv_memmgr_mem *allocs,
						     enum amdgv_mem_id id)
{
	struct amdgv_list_head *head;
	struct amdgv_memmgr_mem *alloc;

	if (allocs == NULL)
		return NULL;

	head = &allocs->node;

	if (amdgv_list_empty(head))
		return NULL;

	amdgv_list_for_each_entry(alloc, head, struct amdgv_memmgr_mem, node) {
		if (alloc->id == id)
			return alloc;
	}

	/* Did not find same id mem in the list */
	return NULL;
}

/* Find memory with same id */
static struct amdgv_memmgr_mem *amdgv_memmgr_find_id(struct amdgv_memmgr_mem *allocs,
						     enum amdgv_mem_id id)
{
	struct amdgv_list_head *head;
	struct amdgv_memmgr_mem *alloc;

	if (allocs == NULL)
		return NULL;

	head = &allocs->node;

	if (amdgv_list_empty(head))
		return NULL;

	amdgv_list_for_each_entry(alloc, head, struct amdgv_memmgr_mem, node) {
		if (MEM_ID_GET_ID(alloc->id) == id)
			return alloc;
	}

	/* Did not find same id mem in the list */
	return NULL;
}

static int amdgv_memmgr_export_mem_allocs(struct amdgv_memmgr *memmgr,
					  struct amdgv_live_info_memmgr_mem *mem_allocs,
					  uint32_t *idx, enum amdgv_mem_owner owner)
{
	struct amdgv_adapter *adapt;
	struct amdgv_list_head *head;
	struct amdgv_memmgr_mem *alloc;
	uint32_t i;

	if (memmgr == NULL || mem_allocs == NULL || idx == NULL) {
		return AMDGV_FAILURE;
	}

	adapt = memmgr->adapt;
	i = *idx;

	if (memmgr->allocs == NULL) {
		AMDGV_ERROR("Mem allocation list is NULL, memmgr is not initialized\n");
		return AMDGV_FAILURE;
	}

	head = &memmgr->allocs->node;

	if (amdgv_list_empty(head)) {
		AMDGV_INFO("No memmgr allocs to export\n");
		return 0;
	}

	oss_mutex_lock(memmgr->lock);

	amdgv_list_for_each_entry(alloc, head, struct amdgv_memmgr_mem, node) {
		if (i >= MEM_ALLOCS_MAX) {
			AMDGV_ERROR("No room left for exporting mem\n");
			goto fail;
		}

		/* skip exporting bad page mem alloc */
		if (MEM_ID_GET_ID(alloc->id) == MEM_ECC_BAD_PAGE)
			continue;

		mem_allocs[i].id = alloc->id;
		mem_allocs[i].owner = owner;
		mem_allocs[i].alloc_off = alloc->alloc_off;
		mem_allocs[i].len = alloc->len;
		mem_allocs[i].align = alloc->align;
		AMDGV_DEBUG("Exported mem_id: %s, index: %x\n", amdgv_mem_id_name(alloc->id), (MEM_ID_GET_INDEX(alloc->id)));
		i++;
	}

	*idx = i;

	oss_mutex_unlock(memmgr->lock);

	return 0;

fail:
	oss_mutex_unlock(memmgr->lock);
	return AMDGV_FAILURE;
}

int amdgv_memmgr_export_mem_allocs_all(struct amdgv_adapter *adapt,
				       struct amdgv_live_info_memmgr_mem *mem_allocs,
				       uint32_t *mem_allocs_count)
{
	uint32_t idx = 0;
	int ret = 0;

	if (adapt == NULL || mem_allocs == NULL || mem_allocs_count == NULL) {
		return AMDGV_FAILURE;
	}

	ret = amdgv_memmgr_export_mem_allocs(&adapt->memmgr_pf, mem_allocs, &idx,
					     MEM_OWNER_PF);

	if (adapt->memmgr_gpu.is_init) {
		if (!ret)
			ret = amdgv_memmgr_export_mem_allocs(&adapt->memmgr_gpu, mem_allocs, &idx,
								MEM_OWNER_GPU);
	}

	if (adapt->memmgr_sys.is_init) {
		if (!ret)
			ret = amdgv_memmgr_export_mem_allocs(&adapt->memmgr_sys, mem_allocs,
							     &idx, MEM_OWNER_SYS);
	}

	*mem_allocs_count = idx;

	return ret;
}

static int amdgv_memmgr_import_mem_allocs(struct amdgv_memmgr *memmgr,
					  struct amdgv_live_info_memmgr_mem *mem_allocs,
					  uint32_t mem_allocs_count,
					  enum amdgv_mem_owner owner)
{
	struct amdgv_adapter *adapt;
	struct amdgv_memmgr_mem *new;
	uint32_t i;
	struct amdgv_memmgr_mem *same_id_mem;

	if (memmgr == NULL || mem_allocs == NULL) {
		return AMDGV_FAILURE;
	}

	adapt = memmgr->adapt;

	if (memmgr->allocs == NULL) {
		AMDGV_ERROR("Mem allocation list is NULL, memmgr is not initialized\n");
		return AMDGV_FAILURE;
	}

	oss_mutex_lock(memmgr->lock);

	for (i = 0; i < mem_allocs_count; ++i) {
		if (mem_allocs[i].owner == owner) {

			/* skip importing bad page mem alloc */
			if (MEM_ID_GET_ID(mem_allocs[i].id) == MEM_ECC_BAD_PAGE)
				continue;

			same_id_mem = amdgv_memmgr_find_mem_id(memmgr->allocs, mem_allocs[i].id);
			if (same_id_mem) {
				same_id_mem->len = mem_allocs[i].len;
				same_id_mem->alloc_off = mem_allocs[i].alloc_off;
				same_id_mem->align = mem_allocs[i].align;
				same_id_mem->id = mem_allocs[i].id;
				continue;
			}

			new = oss_malloc(sizeof(struct amdgv_memmgr_mem));
			if (!new) {
				amdgv_put_error(AMDGV_PF_IDX,
						AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
						sizeof(struct amdgv_memmgr_mem));
				goto fail;
			}
			new->len = mem_allocs[i].len;
			new->alloc_off = mem_allocs[i].alloc_off;
			new->align = mem_allocs[i].align;
			new->id = mem_allocs[i].id;
			new->memmgr = memmgr;
			AMDGV_DEBUG("Imported mem_id %d(%d): %s, index: %x, alloc_off: 0x%09llx, len: 0x%09llx\n",
				new->id, MEM_ID_GET_ID(new->id), amdgv_mem_id_name(new->id), MEM_ID_GET_INDEX(new->id), new->alloc_off, new->len);
			/* TODO: APU needs to take care of the oss_dma_mem for live update
			 * as this attribute doesn't exist for dGPU. Forcing it to null to avoid
			 * possible page fault error in memmgr_free()
			 */
			if (!adapt->gart_size)
				new->sys_mem.handle = NULL;

			amdgv_list_add_tail(&new->node, &memmgr->allocs->node);
		}
	}

	oss_mutex_unlock(memmgr->lock);
	return 0;

fail:
	oss_mutex_unlock(memmgr->lock);
	return AMDGV_FAILURE;
}

int amdgv_memmgr_import_mem_allocs_all(struct amdgv_adapter *adapt,
				       struct amdgv_live_info_memmgr_mem *mem_nodes,
				       uint32_t mem_allocs_count)
{
	int ret = 0;

	if (adapt == NULL || mem_nodes == NULL) {
		return AMDGV_FAILURE;
	}

	ret = amdgv_memmgr_import_mem_allocs(&adapt->memmgr_pf, mem_nodes, mem_allocs_count,
					     MEM_OWNER_PF);

	if (adapt->memmgr_gpu.is_init) {
		if (!ret)
			ret = amdgv_memmgr_import_mem_allocs(&adapt->memmgr_gpu, mem_nodes,
								mem_allocs_count, MEM_OWNER_GPU);
	}

	if (adapt->memmgr_sys.is_init) {
		if (!ret)
			ret = amdgv_memmgr_import_mem_allocs(&adapt->memmgr_sys, mem_nodes,
							     mem_allocs_count, MEM_OWNER_SYS);
	}

	return ret;
}

int amdgv_memmgr_fini(struct amdgv_adapter *adapt, struct amdgv_memmgr *memmgr)
{
	struct amdgv_list_head *head;
	struct amdgv_list_head *rhead;
	struct amdgv_memmgr_mem *alloc, *tmp;
	struct amdgv_memmgr_mem *reserve, *rtmp;

	if (!memmgr)
		return 0;

	if (memmgr->lock)
		oss_mutex_lock(memmgr->lock);

	if (memmgr->reserves) {
		rhead = &memmgr->reserves->node;
		if (!amdgv_list_empty(rhead)) {
			amdgv_list_for_each_entry_safe(reserve, rtmp, rhead, struct amdgv_memmgr_mem,
							node) {
				amdgv_list_del(&reserve->node);
				oss_free(reserve);
			}
		}
		oss_free(memmgr->reserves);
		memmgr->reserves = OSS_INVALID_HANDLE;
	}

	if (!memmgr->allocs)
		goto fini;

	head = &memmgr->allocs->node;
	if (!amdgv_list_empty(head)) {
		amdgv_list_for_each_entry_safe (alloc, tmp, head, struct amdgv_memmgr_mem,
						node) {
			amdgv_list_del(&alloc->node);
			oss_free(alloc);
		}
	}
	oss_free(memmgr->allocs);
	memmgr->allocs = OSS_INVALID_HANDLE;

fini:
	memmgr->offset = 0;
	memmgr->size = 0;
	memmgr->tom = 0;
	memmgr->is_init = false;

	if (memmgr->lock) {
		oss_mutex_unlock(memmgr->lock);
		oss_mutex_fini(memmgr->lock);
		memmgr->lock = OSS_INVALID_HANDLE;
	}

	AMDGV_DEBUG("Free memory manager\n");

	return 0;
}
static int amdgv_memmgr_always_alloc_new(enum amdgv_mem_id id)
{
	switch (id) {
	case MEM_ECC_BAD_PAGE:
	case MEM_ECC_BAD_PAGE_NPS_CANDIDATE:
		return 1;
	default:
		return 0;
	}
}

static int amdgv_memmgr_mem_id_list_add(struct amdgv_adapter *adapt,
				enum amdgv_mem_id id)
{
	struct amdgv_mem_with_bitmap *mem;

	if (adapt->mems.valid_mem_id_count < MEM_ID_COUNT_MAX) {
		mem = &adapt->mems.mem_id_list[adapt->mems.valid_mem_id_count++];
		oss_memset(mem->bitmaps, 0, sizeof(mem->bitmaps));
		mem->mem_id = id;
		return 0;
	}

	return MEM_ID_NOT_AVAILABLE;
}

static uint32_t amdgv_memmgr_mem_id_add(struct amdgv_adapter *adapt,
				enum amdgv_mem_id id)
{
	struct amdgv_mem_with_bitmap *mem;
	uint64_t bitmap;
	uint32_t mem_id;
	int i, ret, index;

	for (i = 0; i < adapt->mems.valid_mem_id_count; i++) {
		if (id == adapt->mems.mem_id_list[i].mem_id)
			break;
	}

	if (i == adapt->mems.valid_mem_id_count) {
		/* First time allocating with this mem_id, add to list*/
		ret = amdgv_memmgr_mem_id_list_add(adapt, id);
		if (ret)
			return ret;
	}

	mem = &adapt->mems.mem_id_list[i];

	i = 0;
	while (i < MAX_BITMAP_COUNT && mem->bitmaps[i] == (~0x0ULL))
		i++;

	if (i == MAX_BITMAP_COUNT) {
		AMDGV_ERROR("No available index for %s\n", amdgv_mem_id_name(id));
		return MEM_ID_NOT_AVAILABLE;
	}

	index = i * 64;
	bitmap = mem->bitmaps[i];
	/* Find the next available index */

	while (bitmap & 0x1) {
		bitmap >>= 1;
		index++;
	}

	mem->bitmaps[i] |= 0x1ULL << (index % 64);
	mem_id = (id & 0xFFFF) | ((index & 0xFFFF) << 16);
	return mem_id;
}

static int amdgv_memmgr_mem_id_remove(struct amdgv_adapter *adapt,
				uint32_t mem_id)
{
	struct amdgv_mem_with_bitmap *mem;
	int i, index, id;

	id = MEM_ID_GET_ID(mem_id);
	index = MEM_ID_GET_INDEX(mem_id);

	if (index >= MAX_MEM_COUNT) {
		AMDGV_ERROR("Invalid index %d for %s\n", index, amdgv_mem_id_name(id));
		return MEM_ID_NOT_AVAILABLE;
	}

	mem = NULL;
	for (i = 0; i < adapt->mems.valid_mem_id_count; i++) {
		if (id == adapt->mems.mem_id_list[i].mem_id) {
			mem = &adapt->mems.mem_id_list[i];
			break;
		}
	}

	if (!mem) {
		AMDGV_ERROR("Fail to find %s in the mem_id list\n", amdgv_mem_id_name(id));
		return MEM_ID_NOT_AVAILABLE;
	}

	mem->bitmaps[index / 64] &= ~(0x1ULL << (index % 64));
	return 0;
}

struct amdgv_memmgr_mem *amdgv_memmgr_alloc(struct amdgv_memmgr *memmgr, uint64_t len,
					    enum amdgv_mem_id id)
{
	return amdgv_memmgr_alloc_align(memmgr, len, 0x1ULL << memmgr->align, id);
}

static int amdgv_map_mem_gart(struct amdgv_memmgr_mem *mem, enum amdgv_map_op map_op)
{
	int pages, offset, ret = 0;
	uint64_t i;

	pages = mem->len >> AMDGV_GPU_PAGE_SHIFT;
	offset = amdgv_memmgr_get_offset(mem);
	for (i = 0; i < pages; i++)
		amdgv_gart_map(mem->memmgr->adapt, offset + (i << AMDGV_GPU_PAGE_SHIFT), 1,
			map_op == AMDGV_UNMAP ? 0 : oss_sg_dma_address(mem->sys_mem.handle, i));

	return ret;
}

static struct amdgv_memmgr_mem *amdgv_memmgr_reserve_attrs(struct amdgv_memmgr *memmgr,
					uint64_t len, uint64_t align, enum amdgv_mem_id id)
{
	struct amdgv_adapter *adapt = memmgr->adapt;
	struct amdgv_memmgr_mem *new, *iter;
	uint32_t mem_id;

	if (!memmgr->is_init)
		return NULL;

	mem_id = amdgv_memmgr_mem_id_add(adapt, id);
	if (mem_id == MEM_ID_NOT_AVAILABLE) {
		AMDGV_ERROR("Fail to add %s to the mem_id list\n", amdgv_mem_id_name(id));
		return NULL;
	}

	new = oss_malloc(sizeof(struct amdgv_memmgr_mem));
	if (!new) {
		amdgv_put_error(AMDGV_PF_IDX,
						AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
						sizeof(struct amdgv_memmgr_mem));
		amdgv_memmgr_mem_id_remove(adapt, mem_id);
		return NULL;
	}

	/* Fill attributes only */
	new->len = len;
	new->align = align;
	new->id = mem_id;
	new->memmgr = memmgr;
	new->alloc_off = 0; /* Deferred location assignment */
	new->sys_mem.handle = NULL;

	oss_mutex_lock(memmgr->lock);

	if (amdgv_list_empty(&memmgr->reserves->node)) {
		amdgv_list_add_tail(&new->node, &memmgr->reserves->node);
		goto out;
	}

	/* If list is not empty, walk from head to tail:
	 * Insert BEFORE the first item that's smaller per len ordering */
	amdgv_list_for_each_entry(iter, &memmgr->reserves->node, struct amdgv_memmgr_mem, node) {
		if (new->len >= iter->len) {
			amdgv_list_add(&new->node, iter->node.prev);
			goto out;
		}
	}
	/* New smallest append to the tail */
	amdgv_list_add_tail(&new->node, &memmgr->reserves->node);

out:
	memmgr->reserve_count++;
	oss_mutex_unlock(memmgr->lock);
	return new;
}

static struct amdgv_memmgr_mem *amdgv_memmgr_alloc_unify_align(struct amdgv_memmgr *memmgr, uint64_t len,
						  uint64_t align, enum amdgv_mem_id id,
						  uint64_t *gpu_addr, void *va_ptr)
{
	struct amdgv_adapter *adapt = memmgr->adapt;
	struct amdgv_memmgr_mem *prev, *new;
	uint64_t addr;
	uint32_t mem_id;
	struct amdgv_memmgr_mem *same_id_mem;

	if (!memmgr->is_init)
		return NULL;

	mem_id = id;
	if (!memmgr->is_sys) {
		mem_id = amdgv_memmgr_mem_id_add(adapt, id);
		if (mem_id == MEM_ID_NOT_AVAILABLE) {
			AMDGV_ERROR("Fail to add %s to the mem_id list\n", amdgv_mem_id_name(id));
			return NULL;
		}
		/* For live update, find if same id mem has already been allocated,
		 * if so, memmgr->allocs has already contained the copied mem info,
		 * just find it and return the mem
		 */
		if (adapt->opt.skip_hw_init) {
			/* Some mem have the same mem_id such as MEM_ECC_BAD_PAGE.
			 * Always alloc new mem for them.
			 */
			if (amdgv_memmgr_always_alloc_new(id))
				goto alloc_new;
			same_id_mem = amdgv_memmgr_find_mem_id(memmgr->allocs, mem_id);
			if (same_id_mem) {
				AMDGV_DEBUG(
					"Same ID mem: %s, index: %x, location: %s, alloc_off: 0x%09llx, len: 0x%09llx, align: 0x%09llx\n",
					amdgv_mem_id_name(mem_id), MEM_ID_GET_INDEX(mem_id), GET_MEM_LOCATION(same_id_mem),
					same_id_mem->alloc_off, same_id_mem->len, same_id_mem->align);

				return same_id_mem;
			}

			goto alloc_new;
		} else
			goto alloc_new;
	}
alloc_new:
	new = oss_malloc(sizeof(struct amdgv_memmgr_mem));
	if (!new) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct amdgv_memmgr_mem));
		return NULL;
	}

	oss_mutex_lock(memmgr->lock);

	/* Find where to place an aligned memory block of the req size */
	prev = amdgv_memmgr_find_size(memmgr, len, align);

	/* Allocations fail if not enough space is available from the TOM
	 * to the end of allocable space
	 */
	if (prev->alloc_off == memmgr->tom) {
		if (AMDGV_MEMMGR_ALIGN(memmgr->tom, align) + len >
		    (memmgr->offset + memmgr->size)) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL,
					len);
			goto alloc_fail;
		}
	}

	if (memmgr->down)
		addr = AMDGV_MEMMGR_ALIGN(prev->alloc_off + len, align) - len;
	else
		addr = AMDGV_MEMMGR_ALIGN(prev->alloc_off, align);
	new->len = len;
	new->alloc_off = addr + len;
	new->align = align;
	new->id = mem_id;
	new->memmgr = memmgr;
	new->sys_mem.handle = NULL;
	new->sys_mem.va_ptr = va_ptr;

	if (memmgr->is_sys) {
		/* AMDGV_GPU_PAGE_SIZE is the minimal size GART can manage.
		 * roundup the length and alignment to AMDGV_GPU_PAGE_SIZE.
		 */
		len = roundup(len, AMDGV_GPU_PAGE_SIZE);
		align = roundup(align, AMDGV_GPU_PAGE_SIZE);
		if (oss_alloc_dma_mem(adapt->dev, len, OSS_DMA_ALLOW_DMA_NOT_CONTIGUOUS,
			&new->sys_mem) != 0) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_DMA_MEM_FAIL, len);
			goto alloc_fail;
		}
		if (gpu_addr)
			*gpu_addr = amdgv_memmgr_get_gpu_addr(new);

		if (adapt->gart_ready) {
			if (amdgv_map_mem_gart(new, AMDGV_MAP)) {
				amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_MAP_DMA_MEM_FAIL, len);
				goto map_fail;
			}
		}
	}
	amdgv_list_add(&new->node, &prev->node);

	/* Adding a new allocation beyond current TOP of memory */
	if (prev->alloc_off == memmgr->tom)
		memmgr->tom = new->alloc_off;

	AMDGV_DEBUG("Alloc addr: 0x%09llx len:0x%09llx\n", addr, len);

	AMDGV_DEBUG(
		"New mem: %s, index: %x, location: %s, alloc_off: 0x%09llx, len: 0x%09llx, align: 0x%09llx\n",
		amdgv_mem_id_name(mem_id), MEM_ID_GET_INDEX(mem_id), GET_MEM_LOCATION(new), new->alloc_off, new->len,
		new->align);

	oss_mutex_unlock(memmgr->lock);

	return new;

map_fail:
	oss_free_dma_mem(new->sys_mem.handle);
alloc_fail:
	oss_mutex_unlock(memmgr->lock);
	oss_free(new);
	return NULL;
}

struct amdgv_memmgr_mem *amdgv_memmgr_alloc_sys_align(struct amdgv_memmgr *memmgr, uint64_t len,
						  uint64_t align, uint64_t *gpu_addr, void *va_ptr)
{
	return amdgv_memmgr_alloc_unify_align(memmgr, len, align, 0, gpu_addr, va_ptr);
}

struct amdgv_memmgr_mem *amdgv_memmgr_alloc_align(struct amdgv_memmgr *memmgr, uint64_t len,
						  uint64_t align, enum amdgv_mem_id id)
{
	struct amdgv_adapter *adapt = memmgr->adapt;

	switch (adapt->asic_type) {
	case CHIP_MI350X:
		if (adapt->status == AMDGV_STATUS_INVALID && !adapt->opt.skip_hw_init)
			return amdgv_memmgr_reserve_attrs(memmgr, len, align, id);
	}

	return amdgv_memmgr_alloc_unify_align(memmgr, len, align, id, NULL, NULL);
}

static struct amdgv_memmgr_mem *amdgv_memmgr_find_size_at(struct amdgv_memmgr *memmgr,
							  uint64_t offset, uint64_t size,
							  uint64_t align)
{
	struct amdgv_list_head *head;
	struct amdgv_memmgr_mem *alloc;

	uint64_t tom = AMDGV_MEMMGR_ALIGN(offset, align);

	head = &memmgr->allocs->node;

	if (amdgv_list_empty(head))
		return memmgr->allocs;

	amdgv_list_for_each_entry (alloc, head, struct amdgv_memmgr_mem, node) {
		if (tom > alloc->alloc_off)
			continue;
		/* overlap with alloced section, alloc fail */
		if ((alloc->alloc_off > tom) &&
		    ((alloc->alloc_off - alloc->len) < (tom + size)))
			return NULL;
		if ((alloc->alloc_off - alloc->len) >= (tom + size))
			return amdgv_list_last_entry(&alloc->node, struct amdgv_memmgr_mem,
						     node);
	}
	return amdgv_list_last_entry(&alloc->node, struct amdgv_memmgr_mem, node);
}

struct amdgv_memmgr_mem *amdgv_memmgr_find_mem_at_offset(struct amdgv_memmgr *memmgr,
							  uint64_t offset,
							  uint64_t size)
{
	struct amdgv_list_head *head;
	struct amdgv_memmgr_mem *alloc;

	head = &memmgr->allocs->node;

	if (amdgv_list_empty(head))
		return NULL;
	if (!size)
		return NULL;

	amdgv_list_for_each_entry(alloc, head, struct amdgv_memmgr_mem, node) {
		if (alloc->len == 0)
			continue;
		/* check overlap */
		if (offset < alloc->alloc_off && (alloc->alloc_off - alloc->len) < (offset + size))
			return alloc;
	}
	return NULL;
}

struct amdgv_memmgr_mem *amdgv_memmgr_alloc_align_at(struct amdgv_memmgr *memmgr,
						     uint64_t offset, uint64_t len,
						     enum amdgv_mem_id id)
{
	struct amdgv_adapter *adapt = memmgr->adapt;
	struct amdgv_memmgr_mem *prev, *new;
	uint32_t align = 0x1ULL << memmgr->align;
	uint32_t mem_id;
	uint64_t addr;

	new = oss_zalloc(sizeof(struct amdgv_memmgr_mem));
	if (!new) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct amdgv_memmgr_mem));
		return NULL;
	}
	oss_mutex_lock(memmgr->lock);

	mem_id = amdgv_memmgr_mem_id_add(adapt, id);
	if (mem_id == MEM_ID_NOT_AVAILABLE) {
		AMDGV_ERROR("Fail to add %s to the mem_id list\n", amdgv_mem_id_name(id));
		return NULL;
	}
	/* Allocations fail if not enough space is available from the TOM
	 * to the end of allocable space
	 */
	if (AMDGV_MEMMGR_ALIGN(offset, align) + len > (memmgr->offset + memmgr->size)) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL, len);
		goto alloc_fail;
	}
	/* Find where to place an aligned memory block of the req size */
	prev = amdgv_memmgr_find_size_at(memmgr, offset, len, align);
	if (!prev)
		goto alloc_fail;

	if (memmgr->down)
		addr = AMDGV_MEMMGR_ALIGN(offset + len, align) - len;
	else
		addr = AMDGV_MEMMGR_ALIGN(offset, align);
	new->len = len;
	new->alloc_off = addr + len;
	new->align = align;
	new->id = mem_id;
	new->memmgr = memmgr;
	amdgv_list_add(&new->node, &prev->node);

	/* update tom */
	if (prev->alloc_off == memmgr->tom)
		memmgr->tom = new->alloc_off;

	oss_mutex_unlock(memmgr->lock);

	AMDGV_DEBUG("Alloc addr: 0x%09llx len:0x%09llx\n", addr, len);

	return new;

alloc_fail:
	oss_mutex_unlock(memmgr->lock);
	oss_free(new);
	return NULL;
}

static int amdgv_memmgr_fill_reserved_bad_pages(struct amdgv_memmgr *memmgr,
						struct amdgv_memmgr_mem **bps_mem,
						uint32_t *idx)
{
	struct amdgv_adapter *adapt;
	struct amdgv_list_head *head;
	struct amdgv_memmgr_mem *alloc;
	uint32_t i, mem_id;

	if (memmgr == NULL || bps_mem == NULL || idx == NULL) {
		return AMDGV_FAILURE;
	}

	adapt = memmgr->adapt;
	i = *idx;

	if (memmgr->allocs == NULL) {
		AMDGV_ERROR("Mem allocation list is NULL, memmgr is not initialized\n");
		return AMDGV_FAILURE;
	}

	head = &memmgr->allocs->node;

	if (amdgv_list_empty(head)) {
		AMDGV_INFO("No memmgr allocs\n");
		return 0;
	}

	oss_mutex_lock(memmgr->lock);

	amdgv_list_for_each_entry(alloc, head, struct amdgv_memmgr_mem, node) {
		if (i >= BAD_PAGE_RECORD_THRESHOLD) {
			AMDGV_ERROR("No room left for recording reserved bad pages\n");
			goto fail;
		}
		if (MEM_ID_GET_ID(alloc->id) == MEM_ECC_BAD_PAGE) {
			/* In live update import, bad pages are not allocated by memmgr_alloc,
			 * so need to add the mem_id to the list here and re-assign the
			 * indices to the bad page mem. If fail to find a new index, set
			 * the mem_id to MEM_ECC_BAD_PAGE as default value.
			 */
			mem_id = amdgv_memmgr_mem_id_add(adapt, MEM_ECC_BAD_PAGE);
			alloc->id = mem_id == MEM_ID_NOT_AVAILABLE ? MEM_ECC_BAD_PAGE : mem_id;
			bps_mem[i] = alloc;
			i++;
		}
	}

	*idx = i;

	oss_mutex_unlock(memmgr->lock);

	return 0;

fail:
	oss_mutex_unlock(memmgr->lock);
	return AMDGV_FAILURE;
}

int amdgv_memmgr_fill_reserved_bad_pages_all(struct amdgv_adapter *adapt,
					     struct amdgv_memmgr_mem **bps_mem)
{
	uint32_t idx = 0;
	int ret = 0;

	if (adapt == NULL || bps_mem == NULL) {
		return AMDGV_FAILURE;
	}

	ret = amdgv_memmgr_fill_reserved_bad_pages(&adapt->memmgr_pf, bps_mem, &idx);
	if (!ret)
		ret = amdgv_memmgr_fill_reserved_bad_pages(&adapt->memmgr_gpu, bps_mem, &idx);

	return ret;
}

/**
 * allocate a new mem
 * replace old mem's offset/mapping with new mem's offset/mapping, does not change type
 * assign replace type, old offset/mapping to new mem
 *
 **/
struct amdgv_memmgr_mem *amdgv_memmgr_alloc_and_replace(struct amdgv_adapter *adapt,
					     struct amdgv_memmgr *memmgr,
						 struct amdgv_memmgr_mem *mem_old,
						 enum amdgv_mem_id replace_type)
{
	struct amdgv_memmgr_mem *mem_new = NULL;
	struct oss_dma_mem_info sys_mem_tmp = {0};
	uint64_t addr_tmp;

	if (mem_old) {
		mem_new = amdgv_memmgr_alloc_align(memmgr, mem_old->len, mem_old->align, replace_type);
		if (mem_new) {
			oss_mutex_lock(memmgr->lock);

			addr_tmp = mem_old->alloc_off;
			sys_mem_tmp = mem_old->sys_mem;
			mem_old->alloc_off = mem_new->alloc_off;
			mem_old->sys_mem = mem_new->sys_mem;
			mem_new->alloc_off = addr_tmp;
			mem_new->sys_mem = sys_mem_tmp;

			/* swap old mem to new's position */
			amdgv_list_swap_entries(&mem_new->node, &mem_old->node);

			oss_mutex_unlock(memmgr->lock);

		}
	}

	return mem_new;
}

int amdgv_memmgr_free_by_id(struct amdgv_memmgr *memmgr, enum amdgv_mem_id id)
{
	struct amdgv_adapter *adapt = memmgr->adapt;
	struct amdgv_memmgr_mem *same_id_mem;
	uint32_t mem_id;
	bool more = false;

	if (!memmgr->is_init)
		return AMDGV_FAILURE;

	mem_id = id;
	do {
		same_id_mem = amdgv_memmgr_find_id(memmgr->allocs, mem_id);
		if (same_id_mem) {
			more = true;
			AMDGV_DEBUG(
				"Found same ID mem: %s, index: %x, location: %s, alloc_off: 0x%09llx, len: 0x%09llx, align: 0x%09llx\n",
				amdgv_mem_id_name(mem_id), MEM_ID_GET_INDEX(mem_id), GET_MEM_LOCATION(same_id_mem),
				same_id_mem->alloc_off, same_id_mem->len, same_id_mem->align);

			amdgv_memmgr_free(same_id_mem);
		} else {
			more = false;
		}
	} while (more);

	return 0;
}

int amdgv_memmgr_free(struct amdgv_memmgr_mem *mem)
{
	struct amdgv_memmgr *memmgr;
	struct amdgv_adapter *adapt;
	struct amdgv_memmgr_mem *prev;
	struct amdgv_list_head *alloc_list;
	uint64_t addr;

	if (!mem)
		return 0;

	memmgr = mem->memmgr;
	adapt = memmgr->adapt;
	addr = amdgv_memmgr_get_offset(mem);

	oss_mutex_lock(memmgr->lock);

	alloc_list = &mem->node;

	amdgv_list_del(alloc_list);

	if (!mem->alloc_off && memmgr->reserve_count) {
		// MEM is in reserve list
		memmgr->reserve_count--;
	}

	if (mem->alloc_off >= memmgr->tom) {
		prev = amdgv_list_last_entry(alloc_list, struct amdgv_memmgr_mem, node);
		memmgr->tom = prev->alloc_off;
	}

	if (!memmgr->is_sys)
		amdgv_memmgr_mem_id_remove(adapt, mem->id);

	if (memmgr->is_sys) {
		oss_free_dma_mem(mem->sys_mem.handle);

		if (adapt->gart_ready)
			amdgv_map_mem_gart(mem, AMDGV_UNMAP);
		mem->sys_mem.handle = NULL;
	}
	oss_mutex_unlock(memmgr->lock);
	oss_free(mem);

	AMDGV_DEBUG("Free addr: 0x%09llx\n", addr);

	return 0;
}

int amdgv_memmgr_get_limit(struct amdgv_memmgr *memmgr, uint64_t *limit)
{
	struct amdgv_adapter *adapt = memmgr->adapt;

	oss_mutex_lock(memmgr->lock);
	*limit = memmgr->size;
	oss_mutex_unlock(memmgr->lock);

	AMDGV_DEBUG("Limit: 0x%09llx\n", *limit);
	return 0;
}

int amdgv_memmgr_get_tom(struct amdgv_memmgr *memmgr, uint64_t *tom)
{
	struct amdgv_adapter *adapt = memmgr->adapt;

	oss_mutex_lock(memmgr->lock);
	*tom = memmgr->tom;
	oss_mutex_unlock(memmgr->lock);

	AMDGV_DEBUG("Top of Mem @addr: 0x%09llx\n", *tom);
	return 0;
}

int amdgv_memmgr_set_size(struct amdgv_memmgr *memmgr, uint64_t limit)
{
	struct amdgv_adapter *adapt = memmgr->adapt;

	oss_mutex_lock(memmgr->lock);

	if (limit < memmgr->tom) {
		oss_mutex_unlock(memmgr->lock);
		AMDGV_DEBUG("Can't set MemMgr limit smaller than TOM");
		return AMDGV_FAILURE;
	}

	memmgr->size = limit;

	oss_mutex_unlock(memmgr->lock);
	return 0;
}

int amdgv_memmgr_set_gpu_base(struct amdgv_memmgr *memmgr, uint64_t base)
{
	oss_mutex_lock(memmgr->lock);
	memmgr->mc_base = base;
	oss_mutex_unlock(memmgr->lock);
	return 0;
}

int amdgv_memmgr_set_cpu_base(struct amdgv_memmgr *memmgr, void *base)
{
	oss_mutex_lock(memmgr->lock);
	memmgr->cpu_base = base;
	oss_mutex_unlock(memmgr->lock);
	return 0;
}

uint64_t amdgv_memmgr_get_gpu_base(struct amdgv_memmgr_mem *mem)
{
	struct amdgv_memmgr *memmgr;
	uint64_t mc_base = 0;

	if (!mem)
		return ~0;
	memmgr = mem->memmgr;

	mc_base = memmgr->mc_base;

	return mc_base;
}

void *amdgv_memmgr_get_cpu_base(struct amdgv_memmgr_mem *mem)
{
	struct amdgv_memmgr *memmgr;
	void *cpu_base = 0;

	if (!mem)
		return NULL;
	memmgr = mem->memmgr;

	cpu_base = memmgr->cpu_base;

	return cpu_base;
}

uint64_t amdgv_memmgr_get_gpu_addr(struct amdgv_memmgr_mem *mem)
{
	struct amdgv_memmgr *memmgr;

	if (!mem)
		return ~0;
	memmgr = mem->memmgr;

	if (memmgr->down)
		return memmgr->mc_base - mem->alloc_off;

	return memmgr->mc_base + mem->alloc_off - mem->len;
}

/**
 *apu - vram buffer's system physical address.
 *dgpu - vram buffer offset from FB location base.
 */
uint64_t amdgv_memmgr_get_gpu_pa(struct amdgv_memmgr_mem *mem)
{
	return amdgv_memmgr_get_gpu_addr(mem) - mem->memmgr->adapt->mc_fb_loc_addr
		+ mem->memmgr->adapt->mc_fb_offset;
}

void *amdgv_memmgr_get_cpu_addr(struct amdgv_memmgr_mem *mem)
{
	struct amdgv_adapter *adapt;
	struct amdgv_memmgr *memmgr;

	if (!mem)
		return NULL;
	memmgr = mem->memmgr;

	adapt = memmgr->adapt;

	if (memmgr->is_sys)
		return mem->sys_mem.va_ptr;

	if (!memmgr->cpu_base)
		return NULL;

	if (memmgr->down)
		return (void *)((uint32_t *)memmgr->cpu_base - (mem->alloc_off >> 2));

	return (void *)((uint32_t *)memmgr->cpu_base + ((mem->alloc_off - mem->len) >> 2));
}

uint64_t amdgv_memmgr_get_offset(struct amdgv_memmgr_mem *mem)
{
	struct amdgv_memmgr *memmgr;

	if (!mem)
		return ~0;
	memmgr = mem->memmgr;

	if (memmgr->down)
		return mem->alloc_off;
	return mem->alloc_off - mem->len;
}

uint64_t amdgv_memmgr_get_size(struct amdgv_memmgr_mem *mem)
{
	if (!mem)
		return 0;

	return mem->len;
}

uint64_t amdgv_memmgr_get_align(struct amdgv_memmgr_mem *mem)
{
	if (!mem)
		return 0;

	return mem->align;
}

enum amdgv_live_info_status amdgv_memmgr_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_memmgr *memmgr_info)
{
	uint32_t ret = 0;
	uint64_t fb_alignment_byte = MBYTES_TO_BYTES(AMDGV_FUNCTION_FB_ALIGNMENT);

	memmgr_info->mc_fb_loc_addr = adapt->mc_fb_loc_addr;
	memmgr_info->mc_fb_top_addr = adapt->mc_fb_top_addr;
	memmgr_info->mc_sys_loc_addr = adapt->mc_sys_loc_addr;
	memmgr_info->mc_sys_top_addr = adapt->mc_sys_top_addr;
	memmgr_info->mc_agp_loc_addr = adapt->mc_agp_loc_addr;
	memmgr_info->mc_agp_top_addr = adapt->mc_agp_top_addr;

	memmgr_info->memmgr_pf_mc_base = adapt->memmgr_pf.mc_base;
	memmgr_info->memmgr_pf_offset = adapt->memmgr_pf.offset;
	memmgr_info->memmgr_pf_size = roundup(adapt->memmgr_pf.tom, fb_alignment_byte);
	memmgr_info->memmgr_pf_align = adapt->memmgr_pf.align;
	memmgr_info->memmgr_pf_tom = adapt->memmgr_pf.tom;
	memmgr_info->memmgr_pf_down = adapt->memmgr_pf.down;

	if (adapt->memmgr_gpu.is_init) {
		memmgr_info->memmgr_gpu_mc_base = adapt->memmgr_gpu.mc_base;
		memmgr_info->memmgr_gpu_offset = adapt->memmgr_gpu.offset;
		memmgr_info->memmgr_gpu_size =
			roundup(adapt->memmgr_gpu.tom, fb_alignment_byte);
		memmgr_info->memmgr_gpu_align = adapt->memmgr_gpu.align;
		memmgr_info->memmgr_gpu_tom = adapt->memmgr_gpu.tom;
		memmgr_info->memmgr_gpu_down = adapt->memmgr_gpu.down;
	}

	ret = amdgv_memmgr_export_mem_allocs_all(adapt, memmgr_info->mem_allocs,
							&memmgr_info->mem_allocs_count);

	if (ret)
		return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_memmgr_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_memmgr *memmgr_info)
{
	uint32_t ret = 0;
	uint64_t addr;

	adapt->mc_fb_loc_addr = memmgr_info->mc_fb_loc_addr;
	adapt->mc_fb_top_addr = memmgr_info->mc_fb_top_addr;
	adapt->mc_sys_loc_addr = memmgr_info->mc_sys_loc_addr;
	adapt->mc_sys_top_addr = memmgr_info->mc_sys_top_addr;
	adapt->mc_agp_loc_addr = memmgr_info->mc_agp_loc_addr;
	adapt->mc_agp_top_addr = memmgr_info->mc_agp_top_addr;

	adapt->memmgr_pf.mc_base = memmgr_info->memmgr_pf_mc_base;
	amdgv_memmgr_set_cpu_base(&adapt->memmgr_pf, adapt->fb);
	adapt->memmgr_pf.offset = memmgr_info->memmgr_pf_offset;
	adapt->memmgr_pf.size = memmgr_info->memmgr_pf_size;
	adapt->memmgr_pf.align = memmgr_info->memmgr_pf_align;
	adapt->memmgr_pf.tom = memmgr_info->memmgr_pf_tom;
	adapt->memmgr_pf.down = memmgr_info->memmgr_pf_down;

	if (adapt->memmgr_gpu.is_init) {
		adapt->memmgr_gpu.mc_base = memmgr_info->memmgr_gpu_mc_base;
		addr = memmgr_info->memmgr_gpu_mc_base - memmgr_info->memmgr_pf_mc_base;
		amdgv_memmgr_set_cpu_base(&adapt->memmgr_gpu,
						(void *)(((uint32_t *)adapt->fb) + (addr >> 2)));
		adapt->memmgr_gpu.offset = memmgr_info->memmgr_gpu_offset;
		adapt->memmgr_gpu.size = memmgr_info->memmgr_gpu_size;
		adapt->memmgr_gpu.align = memmgr_info->memmgr_gpu_align;
		adapt->memmgr_gpu.tom = memmgr_info->memmgr_gpu_tom;
		adapt->memmgr_gpu.down = memmgr_info->memmgr_gpu_down;
	}

	ret = amdgv_memmgr_import_mem_allocs_all(adapt, memmgr_info->mem_allocs,
							memmgr_info->mem_allocs_count);

	if (ret)
		return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;

	amdgv_wb_memory_hw_init_address(adapt);
	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

void amdgv_gmc_flush_gpu_tlb(struct amdgv_adapter *adapt, uint32_t vmid,
					uint32_t vmhub, uint32_t flush_type)
{
	if (adapt->gmc.funcs && adapt->gmc.funcs->flush_gpu_tlb)
		adapt->gmc.funcs->flush_gpu_tlb(adapt, vmid, vmhub, flush_type);
}

int amdgv_map_sys_mem_allocs(struct amdgv_memmgr *memmgr, enum amdgv_map_op map_op)
{
	struct amdgv_list_head *head;
	struct amdgv_memmgr_mem *alloc;
	int ret = 0;

	if ((!memmgr->is_init) || (!memmgr->is_sys))
		return AMDGV_FAILURE;

	oss_mutex_lock(memmgr->lock);

	head = &memmgr->allocs->node;

	if (amdgv_list_empty(head)) {
		oss_mutex_unlock(memmgr->lock);
		return ret;
	}

	head = &memmgr->allocs->node;

	amdgv_list_for_each_entry(alloc, head, struct amdgv_memmgr_mem, node) {
		if (alloc->sys_mem.handle)
			ret = amdgv_map_mem_gart(alloc, map_op);
	}
	oss_mutex_unlock(memmgr->lock);

	return ret;
}

int amdgv_memmgr_assign_reserved_region(struct amdgv_memmgr_mem *reserved)
{
	struct amdgv_memmgr *memmgr;
	struct amdgv_adapter *adapt;
	uint64_t len = 0, addr = 0;
	uint32_t align = 0;
	struct amdgv_memmgr_mem *prev;

	if (!reserved || !reserved->memmgr)
		return AMDGV_FAILURE;

	memmgr = reserved->memmgr;
	adapt = memmgr->adapt;
	if (!adapt)
		return AMDGV_FAILURE;

	len = reserved->len;
	align = reserved->align;

	oss_mutex_lock(memmgr->lock);

	/* Find where to place an aligned memory block of the req size */
	prev = amdgv_memmgr_find_size(memmgr, len, align);

	/* Allocations fail if not enough space is available from the TOM
	 * to the end of allocable space
	 */
	if (prev->alloc_off == memmgr->tom) {
		if (AMDGV_MEMMGR_ALIGN(memmgr->tom, align) + len >
		    (memmgr->offset + memmgr->size)) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL,
					len);
			goto alloc_fail;
		}
	}

	if (memmgr->down)
		addr = AMDGV_MEMMGR_ALIGN(prev->alloc_off + len, align) - len;
	else
		addr = AMDGV_MEMMGR_ALIGN(prev->alloc_off, align);

	/* Fill allocation placement */
	reserved->alloc_off = addr + len;

	/* Move list node from reserves -> placed after prev */
	amdgv_list_del(&reserved->node);
	amdgv_list_add(&reserved->node, &prev->node);

	if (prev->alloc_off == memmgr->tom)
		memmgr->tom = reserved->alloc_off;

	oss_mutex_unlock(memmgr->lock);
	return 0;

alloc_fail:
	oss_mutex_unlock(memmgr->lock);
	return AMDGV_FAILURE;;
}

int amdgv_memmgr_alloc_deferred_region(struct amdgv_memmgr *memmgr)
{
	struct amdgv_memmgr_mem *first;
	struct amdgv_adapter *adapt;

	if (!memmgr)
		return AMDGV_FAILURE;

	adapt = memmgr->adapt;
	if (!adapt)
		return AMDGV_FAILURE;

	for (;;) {
		oss_mutex_lock(memmgr->lock);

		if (amdgv_list_empty(&memmgr->reserves->node)) {
			oss_mutex_unlock(memmgr->lock);
			break;
		}

		first = amdgv_list_first_entry(&memmgr->reserves->node, struct amdgv_memmgr_mem, node);

		oss_mutex_unlock(memmgr->lock);

		if (amdgv_memmgr_assign_reserved_region(first)) {
			AMDGV_ERROR("Failed to assign allocation for deferred region: %d.\n", first->id);
			return AMDGV_FAILURE;
		}
		memmgr->reserve_count--;
	}

	if (memmgr->reserve_count)
		return AMDGV_FAILURE;

	return 0;
}
