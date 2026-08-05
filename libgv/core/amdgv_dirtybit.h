/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_DIRTYBIT_H
#define AMDGV_DIRTYBIT_H

#include "amdgv.h"

struct amdgv_live_info_acc_bits;

#define amdgv_fb_size_to_bitmap_size_align(fb_size, page_size) \
	(roundup((fb_size / page_size / 8) == 0 ? \
		  PAGE_SIZE : (fb_size / page_size / 8), \
		 PAGE_SIZE))

#define amdgv_fb_size_to_byte_size(fb_size, page_size) \
	((fb_size) / (page_size) / 8)

/*accumulated dirty bitmap from VF initialization */
struct acc_bits {
	void *ptr;
	uint32_t size;
	/* Set to true when it is the first Dbit query of the LM,
	 * when it is true, or the acc bits to queried bitmap
	 */
	bool is_first_query;
};

#define AMDGV_FB_HASH_BYTES 32

struct amdgv_fb_hash_digest {
	uint8_t bytes[AMDGV_FB_HASH_BYTES];
};

enum amdgv_fb_hash_mode {
	AMDGV_FB_HASH_MODE_RAPIDHASH = 0,
	AMDGV_FB_HASH_MODE_SHA256 = 1
};

struct amdgv_fb_hash_vf_state {
	uint32_t  prev_idx;
	bool      initialized;
};

struct amdgv_fb_hash_state {
	struct amdgv_memmgr_mem *page_hash_sys[2];
	struct amdgv_fb_hash_vf_state vf[AMDGV_MAX_VF_NUM];
	uint32_t  page_count;
	uint32_t  page_size;
};

struct amdgv_dirtybit {
	const struct amdgv_dirtybit_funcs *funcs;
	uint8_t *query_submission_frame;
	uint32_t dirty_page_size;
	struct amdgv_memmgr_mem *gc_dirty_bitplane;
	struct amdgv_memmgr_mem *mm_dirty_bitplane;
	uint32_t mam_adram_mode;
	void *acc_bits_whole_fb;
	struct acc_bits acc_bits[AMDGV_MAX_VF_NUM];
	struct amdgv_fb_hash_state fb_hash_state;
	bool fb_hash_support;
	enum amdgv_fb_hash_mode fb_hash_mode;
	uint32_t fb_hash_digest_bytes;
};

struct amdgv_dirtybit_funcs {
	int (*control)(struct amdgv_adapter *adapt, bool enable);
	int (*query_data)(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data);
	int (*query_dirty_page_size)(struct amdgv_adapter *adapt, uint32_t *dirty_page_size);
	void (*gcea_sdp_control)(struct amdgv_adapter *adapt, bool gcea_sdp_enable);
};

int amdgv_dirtybit_control(struct amdgv_adapter *adapt, bool enable);
int amdgv_dirtybit_query_and_accumulate(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data);
int amdgv_dirtybit_get_dirty_page_size(struct amdgv_adapter *adapt, uint32_t *dirty_page_size);
int amdgv_dirtybit_clear_fb_dbit(struct amdgv_adapter *adapt,  uint32_t idx_vf);
int amdgv_merge_acc_bits_to_new_bits(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data);
int amdgv_dirtybit_sw_init(struct amdgv_adapter *adapt);
void amdgv_dirtybit_free_acc_bits_whole_fb(struct amdgv_adapter *adapt);
int amdgv_dirtybit_assgin_acc_bits_to_vf(struct amdgv_adapter *adapt);
void amdgv_dirtybit_destroy_vf_acc_bits(struct amdgv_adapter *adapt);
void amdgv_dirtybit_hw_fini(struct amdgv_adapter *adapt);
void amdgv_dirtybit_set_vfs_acc_bits(struct amdgv_adapter *adapt, char pattern);
void amdgv_dirtybit_set_vf_acc_bits(struct amdgv_adapter *adapt, uint32_t idx_vf, char pattern);
int amdgv_dirtybit_query_vf_fb_dbit(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_dirtybit_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_acc_bits *data);
int amdgv_dirtybit_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_acc_bits *data);
void amdgv_dirtybit_reset_vf_hash_state(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_dirtybit_reset_all_hash_state(struct amdgv_adapter *adapt);
void amdgv_dirtybit_gcea_sdp_control(struct amdgv_adapter *adapt, bool gcea_sdp_enable);
#endif