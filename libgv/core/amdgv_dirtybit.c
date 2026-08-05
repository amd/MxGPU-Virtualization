/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_dirtybit.h"
#include "amdgv_api.h"
#include "amdgv_sched_internal.h"

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

	/* On ASICs where dirty_page_size is only assigned during dirtybit hw_init
	 * (e.g. navi32), it reads back as 0 any time acc_bits are (re)assigned
	 * before hw_init has run, such as during a multi-partition mode switch.
	 * Reject 0 centrally here so callers (which all divide by it via
	 * amdgv_fb_size_to_byte_size / amdgv_fb_size_to_bitmap_size_align) bail
	 * out instead of hitting a divide-by-zero.
	 */
	if (ret == 0 && *dirty_page_size == 0) {
		AMDGV_WARN("dirty_page_size is 0 (dirtybit hw not initialized yet).\n");
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

/* Common sw_init path shared by all ASICs that support live migration
 * dirtybit tracking: allocate the acc_bits whole-FB buffer, later carved
 * per-VF by amdgv_dirtybit_hw_init()/amdgv_dirtybit_assgin_acc_bits_to_vf()
 * once dirty_page_size is known. Callers are expected to only invoke this
 * when AMDGV_FLAG_GPUV_LIVE_MIGRATION is set.
 */
int amdgv_dirtybit_sw_init(struct amdgv_adapter *adapt)
{
	adapt->dirtybit.acc_bits_whole_fb = oss_malloc(AMDGV_DIRTYBIT_BUFFER_SIZE);
	if (adapt->dirtybit.acc_bits_whole_fb == NULL) {
		AMDGV_ERROR("failed to allocate acc_bits_whole_fb\n");
		return AMDGV_FAILURE;
	}
	oss_memset(adapt->dirtybit.acc_bits_whole_fb, 0, AMDGV_DIRTYBIT_BUFFER_SIZE);

	return 0;
}

void amdgv_dirtybit_free_acc_bits_whole_fb(struct amdgv_adapter *adapt)
{
	if (adapt->dirtybit.acc_bits_whole_fb != NULL) {
		oss_free(adapt->dirtybit.acc_bits_whole_fb);
		adapt->dirtybit.acc_bits_whole_fb = NULL;
	}
}

int amdgv_dirtybit_assgin_acc_bits_to_vf(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *entry;
	uint64_t fb_size;
	uint64_t fb_offset;
	uint32_t dirty_page_size;
	uint32_t byte_size;

	if (adapt->dirtybit.acc_bits_whole_fb == NULL)
		return 0;

	if (amdgv_dirtybit_get_dirty_page_size(adapt, &dirty_page_size))
		return AMDGV_FAILURE;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		entry = &adapt->array_vf[idx_vf];
		fb_size = MBYTES_TO_BYTES(entry->fb_size);
		fb_offset = MBYTES_TO_BYTES(entry->fb_offset);

		byte_size = amdgv_fb_size_to_byte_size(fb_offset, dirty_page_size);
		adapt->dirtybit.acc_bits[idx_vf].ptr = (char *)adapt->dirtybit.acc_bits_whole_fb + byte_size;

		byte_size = amdgv_fb_size_to_byte_size(fb_size, dirty_page_size);
		adapt->dirtybit.acc_bits[idx_vf].size = byte_size;

		AMDGV_DEBUG("Set VF[%d] acc_bits, offset=0x%x, size=0x%x\n", idx_vf,
			   amdgv_fb_size_to_byte_size(fb_offset, dirty_page_size),
			   amdgv_fb_size_to_byte_size(fb_size, dirty_page_size));
	}

	return 0;
}

void amdgv_dirtybit_destroy_vf_acc_bits(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		adapt->dirtybit.acc_bits[idx_vf].ptr = NULL;
		adapt->dirtybit.acc_bits[idx_vf].size = 0;
	}
}

/* Common hw_fini path shared by all ASICs that support live migration
 * dirtybit tracking: disable dirtybit tracking hw and tear down the per-VF
 * acc_bits[] pointers/sizes carved out by amdgv_dirtybit_assgin_acc_bits_to_vf().
 * Callers are expected to only invoke this when AMDGV_FLAG_GPUV_LIVE_MIGRATION
 * is set.
 */
void amdgv_dirtybit_hw_fini(struct amdgv_adapter *adapt)
{
	amdgv_dirtybit_control(adapt, false);
	amdgv_dirtybit_destroy_vf_acc_bits(adapt);
}

void amdgv_dirtybit_set_vf_acc_bits(struct amdgv_adapter *adapt, uint32_t idx_vf, char pattern)
{
	if (idx_vf >= AMDGV_MAX_VF_NUM)
		return;

	if (adapt->dirtybit.acc_bits[idx_vf].ptr == NULL)
		return;

	oss_memset(adapt->dirtybit.acc_bits[idx_vf].ptr, pattern, adapt->dirtybit.acc_bits[idx_vf].size);
}

void amdgv_dirtybit_set_vfs_acc_bits(struct amdgv_adapter *adapt, char pattern)
{
	uint32_t idx_vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++)
		amdgv_dirtybit_set_vf_acc_bits(adapt, idx_vf, pattern);
}

void amdgv_dirtybit_reset_vf_hash_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_fb_hash_vf_state *vf_state;

	if (idx_vf >= AMDGV_MAX_VF_NUM)
		return;

	vf_state = &adapt->dirtybit.fb_hash_state.vf[idx_vf];
	vf_state->prev_idx = 0;
	vf_state->initialized = false;
}

void amdgv_dirtybit_reset_all_hash_state(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;

	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_NUM; idx_vf++)
		amdgv_dirtybit_reset_vf_hash_state(adapt, idx_vf);
}

/*
 * Update the bitmap from the acc bits to data->dbit_plane_data_buffer, or vise versa
 * data: the query dirty bit data
 * to_acc_bits: true if update to acc bits, false if update to data->dbit_plane_data_buffer
 */
static int amdgv_dirtybit_merge_bitmap(struct amdgv_adapter *adapt,
				 struct amdgv_query_dirty_bit_data *data,
				 bool to_acc_bits)
{
	uint32_t idx_vf = data->idx_vf;
	void *acc_bits = adapt->dirtybit.acc_bits[idx_vf].ptr;
	void *bitmap = data->dbit_plane_data_buffer;
	uint32_t *src, *dst;
	int i = 0;
	uint32_t dirty_page_size = 0;
	uint32_t byte_offset;
	uint32_t byte_size;

	if (acc_bits == NULL)
		return 0;

	if (acc_bits == bitmap) {
		return 0;
	}

	if (amdgv_dirtybit_get_dirty_page_size(adapt, &dirty_page_size)) {
		AMDGV_ERROR("Failed to get dirty page size.\n");
		return AMDGV_FAILURE;
	}

	byte_size = amdgv_fb_size_to_byte_size(data->query_size, dirty_page_size);
	byte_offset = amdgv_fb_size_to_byte_size(data->query_fb_offset, dirty_page_size);

	acc_bits = (char *)acc_bits + byte_offset;

	if (to_acc_bits) {
		src = bitmap;
		dst = acc_bits;
	} else {
		src = acc_bits;
		dst = bitmap;
	}

	for (i = 0; i < byte_size / sizeof(uint32_t); i++) {
		if (src[i] != 0)
			dst[i] |= src[i];
	}

	src += i;
	dst += i;

	for (i = 0; i < byte_size % sizeof(uint32_t); i++) {
		if (*((char *)src + i) != 0)
			*((char *)dst + i) |= *((char *)src + i);
	}

	return 0;
}

static inline int amdgv_merge_new_bits_to_acc_bits(struct amdgv_adapter *adapt,
				     struct amdgv_query_dirty_bit_data *data)
{
	return amdgv_dirtybit_merge_bitmap(adapt, data, true);
}

int amdgv_merge_acc_bits_to_new_bits(struct amdgv_adapter *adapt,
			     struct amdgv_query_dirty_bit_data *data)
{
	return amdgv_dirtybit_merge_bitmap(adapt, data, false);
}

/*
 * Mark every dirty page in the current query as dirty in dbit_plane_data_buffer.
 * One bit per dirty page; query_size need not be page-aligned (partial tail counts
 * as an extra page). Only the bits for those pages are set; the rest of the buffer
 * is cleared. Query-relative layout (bit 0 = first page in the query window).
 */
static int amdgv_dirtybit_set_bitmap_query_buffer_to_dirty(struct amdgv_adapter *adapt,
						struct amdgv_query_dirty_bit_data *data)
{
	uint32_t dirty_page_size = 0;
	uint64_t num_bits;
	uint64_t full_bytes;
	uint32_t rem_bits;
	uint8_t *buf = (uint8_t *)data->dbit_plane_data_buffer;

	if (amdgv_dirtybit_get_dirty_page_size(adapt, &dirty_page_size)) {
		AMDGV_ERROR("Failed to get dirty page size for forced-dirty bitmap\n");
		return AMDGV_FAILURE;
	}

	num_bits = DIV_ROUND_UP(data->query_size, dirty_page_size);
	full_bytes = num_bits / 8ULL;
	rem_bits = (uint32_t)(num_bits % 8ULL);

	/* Need room for full 0xff bytes plus optional partial byte at buf[full_bytes] */
	if (full_bytes + (rem_bits != 0 ? 1ULL : 0ULL) > data->dbit_plane_data_size) {
		AMDGV_ERROR("dirty bitmap buffer too small: need %llu bytes, have %llu\n",
			    (unsigned long long)(full_bytes + (rem_bits != 0 ? 1ULL : 0ULL)),
			    (unsigned long long)data->dbit_plane_data_size);
		return AMDGV_FAILURE;
	}

	if (full_bytes)
		oss_memset(buf, 0xff, full_bytes);
	if (rem_bits != 0)
		buf[full_bytes] = (uint8_t)((1U << rem_bits) - 1U);

	return 0;
}

static int amdgv_dirtybit_querydata(struct amdgv_adapter *adapt,
				struct amdgv_query_dirty_bit_data *data)
{
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

	if (amdgv_xgmi_node_fb_sharing_allowed(adapt) && !adapt->dirtybit.fb_hash_support) {
		AMDGV_DEBUG("FB sharing mode is enabled, set queried FB range dirty in bitmap\n");
		return amdgv_dirtybit_set_bitmap_query_buffer_to_dirty(adapt, data);
	}

	if (adapt->dirtybit.funcs &&
		adapt->dirtybit.funcs->query_data) {
		return adapt->dirtybit.funcs->query_data(adapt, data);
	}

	AMDGV_ERROR("query_data is not properly defined.");
	return AMDGV_FAILURE;
}

/*
 * Query Dbit and accumulate the result into acc_bits.
 * HW Dbit may be lost due to VF FLR, LM failure, etc.
 * We need to record all the Dbits to ensure the success of incoming LM.
 * On any failure, the VF's acc_bits are set to all-dirty as a safe fallback.
 */
int amdgv_dirtybit_query_and_accumulate(struct amdgv_adapter *adapt,
				struct amdgv_query_dirty_bit_data *data)
{
	int ret;

	ret = amdgv_dirtybit_querydata(adapt, data);
	if (ret)
		goto dbit_fail;

	ret = amdgv_merge_new_bits_to_acc_bits(adapt, data);
	if (ret) {
		AMDGV_ERROR("Failed to merge bitmap to acc bits\n");
		goto dbit_fail;
	}

	return ret;

dbit_fail:
	AMDGV_WARN("Set VF[%d] whole fb to dirty\n", data->idx_vf);
	amdgv_dirtybit_set_vf_acc_bits(adapt, data->idx_vf, 0xff);
	return ret;
}

static inline void amdgv_dirtybit_prepare_query_params(struct amdgv_adapter *adapt,
					     struct amdgv_query_dirty_bit_data *data,
					     uint32_t idx_vf,
					     uint64_t fb_offset,
					     uint64_t fb_size,
					     void *bitmap,
					     uint32_t size,
					     bool preserve)
{
	data->query_fb_offset = fb_offset;
	data->query_size = fb_size;
	data->dbit_plane_data_buffer = bitmap;
	data->dbit_plane_data_size = size;
	data->dbit_preserve = preserve;// clear the Dbit
	data->idx_vf = idx_vf;
}

static int amdgv_dirtybit_query_vf_fb_dbit_common(struct amdgv_adapter *adapt,
						  uint32_t idx_vf, bool to_acc)
{
	uint32_t bitmap_size = 0;
	uint32_t *bitmap = NULL;
	uint64_t fb_size = MBYTES_TO_BYTES(adapt->array_vf[idx_vf].fb_size);
	uint32_t dirty_page_size = 0;
	struct amdgv_query_dirty_bit_data query_params;
	int ret = 0;

	if (amdgv_dirtybit_get_dirty_page_size(adapt, &dirty_page_size)) {
		AMDGV_ERROR("Failed to get dirty page size.\n");
		return AMDGV_FAILURE;
	}

	bitmap_size = amdgv_fb_size_to_bitmap_size_align(fb_size, dirty_page_size);
	bitmap = (uint32_t *)oss_zalloc(bitmap_size);
	if (bitmap == NULL) {
		AMDGV_ERROR("Failed to allocate memory for dirty bit\n");
		return AMDGV_FAILURE;
	}

	amdgv_dirtybit_prepare_query_params(adapt, &query_params, idx_vf,
					    0, fb_size, bitmap, bitmap_size, false);
	if (to_acc)
		ret = amdgv_dirtybit_query_and_accumulate(adapt, &query_params);
	else
		ret = amdgv_dirtybit_querydata(adapt, &query_params);

	oss_free(bitmap);

	return ret;
}

int amdgv_dirtybit_query_vf_fb_dbit(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return amdgv_dirtybit_query_vf_fb_dbit_common(adapt, idx_vf, true);
}

int amdgv_dirtybit_clear_fb_dbit(struct amdgv_adapter *adapt,  uint32_t idx_vf)
{
	int ret = 0;

	if (idx_vf >= AMDGV_MAX_VF_NUM) {
		AMDGV_ERROR("Invalid idx_vf %u for dirty bit clear\n", idx_vf);
		return AMDGV_FAILURE;
	}

	/* Clear the Dbit by querying the whole VF FB with preserve = false,
	 * preserve = false will clear the Dbit when querying
	 */
	if (amdgv_dirtybit_query_vf_fb_dbit_common(adapt, idx_vf, false)) {
		AMDGV_WARN("Failed to clear VF[%d] FB dbit\n", idx_vf);
		ret = AMDGV_FAILURE;
	} else {
		AMDGV_DEBUG("VF[%d] FB dbit cleared\n", idx_vf);
		ret = 0;
	}

	amdgv_dirtybit_set_vf_acc_bits(adapt, idx_vf, 0);

	return ret;
}

int amdgv_dirtybit_export_live_data(struct amdgv_adapter *adapt,
				    struct amdgv_live_info_acc_bits *data)
{
	if (adapt->dirtybit.acc_bits_whole_fb != NULL) {
		oss_memcpy(data->acc_bits_whole_fb, adapt->dirtybit.acc_bits_whole_fb,
			   AMDGV_DIRTYBIT_BUFFER_SIZE);
	}
	return 0;
}

int amdgv_dirtybit_import_live_data(struct amdgv_adapter *adapt,
				    struct amdgv_live_info_acc_bits *data)
{
	if (adapt->dirtybit.acc_bits_whole_fb == NULL)
		return 0;

	oss_memcpy(adapt->dirtybit.acc_bits_whole_fb, data->acc_bits_whole_fb,
		   AMDGV_DIRTYBIT_BUFFER_SIZE);

	if (amdgv_dirtybit_assgin_acc_bits_to_vf(adapt)) {
		AMDGV_ERROR("Failed to assign acc bits to VF\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

void amdgv_dirtybit_gcea_sdp_control(struct amdgv_adapter *adapt,
	bool gcea_sdp_enable)
{
	if (adapt->dirtybit.funcs &&
		adapt->dirtybit.funcs->gcea_sdp_control) {
		adapt->dirtybit.funcs->gcea_sdp_control(adapt, gcea_sdp_enable);
	} else {
		AMDGV_WARN("gcea_sdp_control is not defined.");
	}
}