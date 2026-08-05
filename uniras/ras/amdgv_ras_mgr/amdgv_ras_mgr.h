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
#ifndef __AMDGV_RAS_MGR_H__
#define __AMDGV_RAS_MGR_H__

#include "amdgv_ras_event.h"
#include "../rascore/ras.h"
#include "amdgv_ras_process.h"

/* Size of cmd_buf in struct amd_sriov_uniras_shared_mem (AMD_SRIOV_UNIRAS_CMD_MAX_SIZE). */
#define RAS_CMD_MAX_BUF_SIZE	(4096 * 13)

struct ras_ih_info {
	uint32_t block;
	struct amdgv_iv_entry iv_entry;
};

#define ALLOWED_MAX_FAILURE_NUM  30
struct auto_update_cmd {
	oss_list_head node;
	bool disabled;
	bool need_update;
	uint32_t exe_err_cnt;
	uint32_t cmd_id;
	uint64_t addr;
	uint32_t len;
	uint32_t param;
	void *data;
};

struct vf_auto_cmd_mgr {
	struct oss_mutex cmd_lock;
	oss_list_head cmd_list;
	uint32_t list_count;
	bool need_update_all;
};

struct vf_ras_lifespan_data {
	struct ras_cmd_block_ecc ecc_count_baseline[RAS_BLOCK_ID__LAST];
	struct {
		uint64_t start_rptr;
	} cper_ptr_record;
};

struct amdgv_ras_mgr {
	struct amdgv_adapter *adapt;
	struct ras_core_context *ras_core;
	struct ras_event_manager ras_event_mgr;
	struct vf_auto_cmd_mgr vf_auto_cmd_mgr[AMDGV_MAX_VF_NUM];
	struct vf_ras_lifespan_data array_vf[AMDGV_MAX_VF_NUM];
	bool ras_is_ready;
};

struct amdgv_ras_mgr *amdgv_ras_mgr_get_context(struct amdgv_adapter *adapt);
bool amdgv_ras_mgr_is_rma(struct amdgv_adapter *adapt);
uint64_t amdgv_ras_mgr_gen_ras_event_seqno(struct amdgv_adapter *adapt,
			enum ras_seqno_type seqno_type);
int amdgv_ras_handle_vf_cmd(struct amdgv_adapter *adapt,
		uint32_t idx_vf, uint64_t gpa_addr, uint32_t gpa_size);
int amdgv_ras_mgr_handle_ras_cmd(struct amdgv_adapter *adapt,
			uint32_t cmd_id, void *input, uint32_t input_size,
			void *output, uint32_t out_size);
int amdgv_ras_mgr_fetch_and_sort_bps(struct amdgv_adapter *adapt,
			uint64_t **bp_offsets, int *bp_count);
int amdgv_ras_mgr_early_init_service(struct amdgv_adapter *adapt);
void amdgv_ras_mgr_get_ras_caps(struct amdgv_adapter *adapt,
		struct amd_sriov_msg_pf2vf_info *pf2vf_msg);
void amdgv_ras_mgr_vf_ecc_count_init(struct amdgv_adapter *adapt, uint32_t idx_vf);
#endif
