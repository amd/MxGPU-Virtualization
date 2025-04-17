/*
 * Copyright (c) 2017-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_VFMGR_H
#define AMDGV_VFMGR_H

#include "amdgv_sriovmsg.h"

extern struct amdgv_init_func amdgv_vfmgr_func;


enum amdgv_vfmgr_cper_event {
	VFMGR_CPER_EVENT_GUEST_LOAD,
	VFMGR_CPER_EVENT_NUM_VF_CHANGE,
	VFMGR_CPER_EVENT_DISABLE,
	VFMGR_CPER_EVENT_ENABLE,
};

void amdgv_vfmgr_enter_customized_vf_mode(struct amdgv_adapter *adapt, int clear_vf);
void amdgv_vfmgr_exit_customized_vf_mode(struct amdgv_adapter *adapt, int default_vf);
void amdgv_vfmgr_set_all_vf(struct amdgv_adapter *adapt);
void amdgv_vfmgr_set_default_fb_size(struct amdgv_adapter *adapt,
			 struct amdgv_vf_option *opt);
int amdgv_vfmgr_alloc_vf(struct amdgv_adapter *adapt, struct amdgv_vf_option *opt);
int amdgv_vfmgr_free_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_vfmgr_set_vf_num(struct amdgv_adapter *adapt, uint32_t num_vf);
int amdgv_vfmgr_set_vf(struct amdgv_adapter *adapt, enum amdgv_set_vf_opt_type type,
		       struct amdgv_vf_option *vf_option);

int amdgv_vfmgr_get_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *fb_offset,
			  uint32_t *fb_size);
int amdgv_vfmgr_get_vf_bdf(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t *bdf);

int amdgv_vfmgr_copy_to_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t offset,
			      void *buf, uint32_t size);
int amdgv_vfmgr_copy_from_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t offset,
				void *buf, uint32_t size);

int amdgv_vfmgr_copy_and_calc_checksum_to_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
	uint64_t offset, void *buf, uint32_t size,
	uint32_t *checksum);


int amdgv_vfmgr_update_bp_message(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_vfmgr_update_pf2vf_message(struct amdgv_adapter *adapt, uint32_t idx_vf);
int amdgv_vfmgr_retrieve_vf2pf_message(struct amdgv_adapter *adapt, uint32_t idx_vf,
					struct amd_sriov_msg_vf2pf_info *output);
int amdgv_vfmgr_print_vf2pf_message(struct amdgv_adapter *adapt, uint32_t idx_vf,
					struct amd_sriov_msg_vf2pf_info *vf2pf_msg);

int amdgv_vfmgr_get_fb_layout(struct amdgv_adapter *adapt, struct amdgv_fb_layout *layout);

int amdgv_vfmgr_get_adapt_uuid(struct amdgv_adapter *adapt, uint64_t *uuid);

int amdgv_host_upload_fw_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_vfmgr_configured_vf_num(struct amdgv_adapter *adapt);

int amdgv_vfmgr_copy_ip_data_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   bool mbox_resp);

/* program_vf_mc_settings, clear_vf_fb and copy_ip_data_to_vf_fb */
int amdgv_vfmgr_init_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf, bool mbox_resp,
			   uint8_t pattern_data, uint8_t flag);

void amdgv_vfmgr_clean_bp_block_size(struct amdgv_adapter *adapt);
void amdgv_vfmgr_clean_vf_bp_block_size(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_msg_checksum(void *obj, uint32_t obj_size, uint32_t key, uint32_t chksum);

void amdgv_vfmgr_import_pf_fb(struct amdgv_adapter *adapt);
void amdgv_vfmgr_import_vfs_fb(struct amdgv_adapter *adapt);
int amdgv_vfmgr_read_mmio(struct amdgv_adapter *adapt, uint32_t idx_vf,
    void *buffer, uint32_t offset, uint32_t length);
int amdgv_vfmgr_write_mmio(struct amdgv_adapter *adapt, uint32_t idx_vf,
    void *buffer, uint32_t offset, uint32_t length);

void amdgv_vfmgr_get_xgmi_info(struct amdgv_adapter *adapt, union amdgv_dev_info *info);

int amdgv_vfmgr_sw_init(struct amdgv_adapter *adapt);
int amdgv_vfmgr_sw_fini(struct amdgv_adapter *adapt);
int amdgv_vfmgr_hw_init(struct amdgv_adapter *adapt);
int amdgv_vfmgr_hw_fini(struct amdgv_adapter *adapt);

void amdgv_vfmgr_init_pf_config(struct amdgv_adapter *adapt);
void amdgv_vfmgr_init_pf_gfx_time_slice(struct amdgv_adapter *adapt);
void amdgv_vfmgr_init_vfs_config(struct amdgv_adapter *adapt);
void amdgv_vfmgr_init_vfs_config_tmr(struct amdgv_adapter *adapt, bool vf_configured);
void amdgv_vfmgr_set_pf_fb(struct amdgv_adapter *adapt);

enum amdgv_live_info_status amdgv_vfmgr_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_vf *vf_info);
enum amdgv_live_info_status amdgv_vfmgr_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_vf *vf_info);
enum amdgv_live_info_status amdgv_vfmgr_export_live_data_extend(struct amdgv_adapter *adapt, struct amdgv_live_info_vf_extend *vf_extend);
enum amdgv_live_info_status amdgv_vfmgr_import_live_data_extend(struct amdgv_adapter *adapt, struct amdgv_live_info_vf_extend *vf_extend);

struct amdgv_vf_fb_block *amdgv_vfmgr_find_fb_block_by_fcn(struct amdgv_adapter *adapt, uint32_t vf_idx);
struct amdgv_vf_fb_block *amdgv_vfmgr_find_usable_free_block(struct amdgv_adapter *adapt, uint32_t fb_size);
struct amdgv_vf_fb_block *amdgv_vfmgr_create_fb_block(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t fb_offset_tlb,
				uint32_t fb_size, bool free);

uint32_t amdgv_vfmgr_calculate_fb_size_tlb(struct amdgv_adapter *adapt, uint32_t fb_size);
int amdgv_vfmgr_divide_fb_block(struct amdgv_adapter *adapt, struct amdgv_vf_fb_block *fb_block, uint32_t fb_size);
void amdgv_vfmgr_free_fb_block(struct amdgv_adapter *adapt, struct amdgv_vf_fb_block *fb_block);
bool amdgv_vfmgr_check_fb_assignable(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t fb_size);

int amdgv_vfmgr_vf_fb_resize(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t fb_size);
int amdgv_vfmgr_vf_fb_defragment(struct amdgv_adapter *adapt);
void amdgv_vfmgr_read_asymmetric_fb_layout(struct amdgv_adapter *adapt, char *layout_buf, int *len, uint32_t resv_size);
int amdgv_vfmgr_dump_ras_error_counts(struct amdgv_adapter *adapt, uint32_t idx_vf);

int amdgv_vfmgr_dump_cpers(struct amdgv_adapter *adapt, uint32_t idx_vf, uint64_t vf_rptr, bool *allow_again);
int amdgv_vfmgr_cper_notify_event(struct amdgv_adapter *adapt, enum amdgv_vfmgr_cper_event event, uint32_t idx_vf);
#endif
