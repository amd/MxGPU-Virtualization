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
#include "amdgv_dirtybit.h"
#include "amdgv_api.h"

static const uint32_t this_block = AMDGV_LIVE_MIGRATION_BLOCK;

int amdgv_dirtybit_get_dirty_page_size(struct amdgv_adapter *adapt, uint32_t *dirty_page_size)
{
	int ret = 0;

	if (adapt->dirtybit.funcs &&
		adapt->dirtybit.funcs->query_dirty_page_size) {
		ret = adapt->dirtybit.funcs->query_dirty_page_size(adapt, dirty_page_size);
	} else {
		AMDGV_ERROR("query_dirty_page_size is not properly defined.");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_dirtybit_control(struct amdgv_adapter *adapt, bool enable)
{
	int ret = 0;

	if (adapt->dirtybit.funcs &&
		adapt->dirtybit.funcs->control) {
		ret = adapt->dirtybit.funcs->control(adapt, enable);
	} else {
		AMDGV_ERROR("dirtybit_control is not properly defined.");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_dirtybit_querydata(struct amdgv_adapter *adapt,
				struct amdgv_query_dirty_bit_data *data)
{
	int ret = 0;

	if (data->query_size == 0) {
		AMDGV_ERROR("query_size is 0\n");
		return AMDGV_FAILURE;
	}

	if (data->query_fb_offset > adapt->mc_fb_top_addr) {
		AMDGV_ERROR("query_fb_offset is out of range\n");
		return AMDGV_FAILURE;
	}

	if (data->dbit_plane_data_buffer == NULL) {
		AMDGV_ERROR("dbit_plane_data_buffer is NULL\n");
		return AMDGV_FAILURE;
	}

	if (adapt->dirtybit.funcs &&
		adapt->dirtybit.funcs->query_data) {
		ret = adapt->dirtybit.funcs->query_data(adapt, data);
	} else {
		AMDGV_ERROR("query_data is not properly defined.");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_dirtybit_clear_fb_dbit(struct amdgv_adapter *adapt,  uint32_t idx_vf)
{
	struct amdgv_query_dirty_bit_data query_info;
	struct amdgv_vf_device *entry = &adapt->array_vf[idx_vf];
	uint64_t fb_size = MBYTES_TO_BYTES(entry->fb_size);
	uint32_t dirty_page_size;
	uint32_t bitmap_size;
	uint32_t *bitmap_buf;

	if (idx_vf == AMDGV_PF_IDX)
		return 0;

	if (amdgv_dirtybit_get_dirty_page_size(adapt, &dirty_page_size)) {
		AMDGV_ERROR("Failed to get dirty page size.\n");
		return AMDGV_FAILURE;
	}
	bitmap_size = amdgv_fb_size_to_bitmap_size(fb_size, dirty_page_size);
	bitmap_buf = (uint32_t *)oss_zalloc(bitmap_size);
	if (bitmap_buf == NULL) {
		AMDGV_ERROR("Failed to allocate memory for dirty bit\n");
		return AMDGV_FAILURE;
	}

	query_info.query_fb_offset = 0;
	query_info.query_size = fb_size;
	query_info.dbit_plane_data_buffer = bitmap_buf;
	query_info.dbit_plane_data_size = bitmap_size;
	query_info.dbit_preserve = 0;// clear the Dbit
	query_info.idx_vf = idx_vf;

	/* Clear the Dbit by querying the whole VF FB,
	 * bm_info.dbit_preserve = 0 will clear the Dbit when querying
	 */
	if (amdgv_dirtybit_querydata(adapt, &query_info)) {
		AMDGV_WARN("Failed to clear VF[%d] FB dbit\n", idx_vf);
		oss_free(bitmap_buf);
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("VF[%d] FB dbit cleared\n", idx_vf);
	oss_free(bitmap_buf);
	return 0;
}
