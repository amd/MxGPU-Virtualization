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

struct amdgv_dirtybit {
	const struct amdgv_dirtybit_funcs *funcs;
	uint8_t *query_submission_frame;
	uint32_t dirty_page_size;
	struct amdgv_memmgr_mem *gc_dirty_bitplane;
	struct amdgv_memmgr_mem *mm_dirty_bitplane;
	uint32_t mam_adram_mode;
	void *acc_bits_whole_fb;
	struct acc_bits acc_bits[AMDGV_MAX_VF_NUM];
};

struct amdgv_dirtybit_funcs {
	int (*control)(struct amdgv_adapter *adapt, bool enable);
	int (*query_data)(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data);
	int (*query_dirty_page_size)(struct amdgv_adapter *adapt, uint32_t *dirty_page_size);
};

int amdgv_dirtybit_control(struct amdgv_adapter *adapt, bool enable);
int amdgv_dirtybit_querydata(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data);
int amdgv_dirtybit_get_dirty_page_size(struct amdgv_adapter *adapt, uint32_t *dirty_page_size);
int amdgv_dirtybit_clear_fb_dbit(struct amdgv_adapter *adapt,  uint32_t idx_vf);
int amdgv_merge_acc_bits_to_new_bits(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data);
int amdgv_dirtybit_assgin_acc_bits_to_vf(struct amdgv_adapter *adapt);
void amdgv_dirtybit_destroy_vf_acc_bits(struct amdgv_adapter *adapt);
void amdgv_dirtybit_set_vfs_acc_bits(struct amdgv_adapter *adapt, char pattern);
int amdgv_dirtybit_query_vf_fb_dbit(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_dirtybit_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_acc_bits *data);
int amdgv_dirtybit_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_acc_bits *data);
#endif