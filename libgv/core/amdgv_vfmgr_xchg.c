/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;


/* On A+A platforms, GPA<->SPA translations may not be continuous. And the GPA->SPA translation is
 * not visible to libgv.
 *
 * We must rely on OSS layer to perform translations. By default, only PF/VF data exchange is
 * available for read/write. */


static int amdgv_vfmgr_write_vf_sysmem_xchg(struct amdgv_adapter *adapt, uint32_t idx_vf,
					 void *buf, uint64_t offset, uint32_t size)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];

	if (!entry->xchg.gpa_ctx) {
		amdgv_put_log(idx_vf, AMDGV_LOG_VF_XCHG_REGION_NOT_INITIALIZED, 0);
		return AMDGV_FAILURE;
	}

	return oss_write_vf_sysmem_xchg(adapt->dev, entry->xchg.gpa_ctx, buf, offset, size);
}

static int amdgv_vfmgr_read_vf_sysmem_xchg(struct amdgv_adapter *adapt, uint32_t idx_vf,
					 void *buf, uint64_t offset, uint32_t size)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];

	if (!entry->xchg.gpa_ctx) {
		amdgv_put_log(idx_vf, AMDGV_LOG_VF_XCHG_REGION_NOT_INITIALIZED, 0);
		return AMDGV_FAILURE;
	}

	return oss_read_vf_sysmem_xchg(adapt->dev, entry->xchg.gpa_ctx, buf, offset, size);
}

int amdgv_vfmgr_copy_to_vf_xchg_table(struct amdgv_adapter *adapt, uint32_t idx_vf,
				      enum amd_sriov_msg_table_id_enum table_id,
				      uint64_t offset_in_table, void *buf, uint32_t size)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	uint64_t vf_fb_base = MBYTES_TO_BYTES(entry->fb_offset);
	uint64_t offset;
	int ret = 0;

	offset = (table_id == AMD_SRIOV_MSG_MAX_TABLE_ID) ? offset_in_table :
		 entry->xchg.vf_table_offsets[table_id] + offset_in_table;

	if (adapt->use_vf_sysmem_xchg)
		return amdgv_vfmgr_write_vf_sysmem_xchg(adapt, idx_vf, buf, offset, size);

	if (!entry->configured ||
	    (!(adapt->flags & AMDGV_FLAG_ENABLE_SVM) && entry->dev == AMDGV_INVALID_HANDLE)) {
		amdgv_put_log(idx_vf, AMDGV_LOG_VF_XCHG_TABLE_TARGET_INVALID, 0);
		return AMDGV_FAILURE;
	}

	if (!(adapt->flags & AMDGV_FLAG_ENABLE_SVM))
		amdgv_vfmgr_map_vf_dev_res(adapt, idx_vf);

	if (entry->res_mapped && (entry->mapped_fb_size >= offset + size) &&
	    amdgv_gpuiov_get_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_FB)) {
		oss_memcpy((uint8_t *)entry->res.fb + offset, buf, size);
	} else if (adapt->ffbm.enabled) {
		ret = amdgv_ffbm_copy_to_gpa(adapt, buf, idx_vf, offset, size);
	} else {
		if (adapt->mapped_fb_size >= vf_fb_base + offset + size)
			oss_memcpy((uint8_t *)adapt->fb + vf_fb_base + offset, buf, size);
		else
			amdgv_mm_copy_to_fb(adapt, vf_fb_base + offset, (uint64_t)buf, size);
	}

	return ret;
}

int amdgv_vfmgr_copy_from_vf_xchg_table(struct amdgv_adapter *adapt, uint32_t idx_vf,
					enum amd_sriov_msg_table_id_enum table_id,
					uint64_t offset_in_table, void *buf, uint32_t size)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	uint64_t vf_fb_base = MBYTES_TO_BYTES(entry->fb_offset);
	uint64_t offset;
	int ret = 0;

	offset = (table_id == AMD_SRIOV_MSG_MAX_TABLE_ID) ? offset_in_table :
		 entry->xchg.vf_table_offsets[table_id] + offset_in_table;

	if (adapt->use_vf_sysmem_xchg)
		return amdgv_vfmgr_read_vf_sysmem_xchg(adapt, idx_vf, buf, offset, size);

	if (!entry->configured ||
		(!(adapt->flags & AMDGV_FLAG_ENABLE_SVM) && entry->dev == AMDGV_INVALID_HANDLE)) {
		amdgv_put_log(idx_vf, AMDGV_LOG_VF_XCHG_TABLE_TARGET_INVALID, 0);
		return AMDGV_FAILURE;
	}

	if (!(adapt->flags & AMDGV_FLAG_ENABLE_SVM))
		amdgv_vfmgr_map_vf_dev_res(adapt, idx_vf);

	if (entry->res_mapped && (entry->mapped_fb_size >= offset + size)
			 && amdgv_gpuiov_get_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_FB)) {
		oss_memcpy(buf, ((uint8_t *)entry->res.fb) + offset, size);
	} else if (adapt->ffbm.enabled) {
		ret = amdgv_ffbm_copy_from_gpa(adapt, buf, idx_vf, offset, size);
	} else {
		if (adapt->mapped_fb_size >= vf_fb_base + offset + size)
			oss_memcpy(buf, (uint8_t *)adapt->fb + vf_fb_base + offset, size);
		else
			amdgv_mm_copy_from_fb(adapt, (uint64_t)buf, vf_fb_base + offset, size);
	}

	return ret;
}

int amdgv_vfmgr_init_xchg_region(struct amdgv_adapter* adapt, uint32_t idx_vf,
				 uint64_t gpa_base, uint32_t size)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	uint32_t max_size;

	if (!adapt->use_vf_sysmem_xchg)
		return AMDGV_FAILURE;

	max_size = amdgv_vfmgr_calc_crit_region_pool_size(adapt, idx_vf);
	if (!size || size > max_size) {
		amdgv_put_log(idx_vf, AMDGV_LOG_VF_XCHG_REGION_INVALID_SIZE,
			      AMDGV_LOG_DATA_32_32(size, max_size));
		return AMDGV_FAILURE;
	}

	if (entry->xchg.gpa_ctx)
		oss_fini_vf_sysmem_xchg(adapt->dev, entry->xchg.gpa_ctx);

	entry->xchg.gpa_ctx = oss_init_vf_sysmem_xchg(adapt->dev, idx_vf, gpa_base, size);

	if (!entry->xchg.gpa_ctx)
		return AMDGV_FAILURE;

	return 0;
}

int amdgv_vfmgr_fini_xchg_region(struct amdgv_adapter* adapt, uint32_t idx_vf)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];

	if (entry->xchg.gpa_ctx)
		oss_fini_vf_sysmem_xchg(adapt->dev, entry->xchg.gpa_ctx);

	entry->xchg.gpa_ctx = NULL;

	return 0;
}