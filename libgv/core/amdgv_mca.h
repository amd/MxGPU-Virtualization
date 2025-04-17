/*
 * Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef AMDGV_MCA_H
#define AMDGV_MCA_H

#include "amdgv_ras.h"
#include "amdgv_list.h"
#include "amd_cper.h"

enum amdgv_mca_ip {
	AMDGV_MCA_IP_UNKNOW = -1,
	AMDGV_MCA_IP_PSP = 0,
	AMDGV_MCA_IP_SDMA,
	AMDGV_MCA_IP_GC,
	AMDGV_MCA_IP_SMU,
	AMDGV_MCA_IP_MP5,
	AMDGV_MCA_IP_UMC,
	AMDGV_MCA_IP_PCS_XGMI,
	AMDGV_MCA_IP_MMHUB,
	AMDGV_MCA_IP_COUNT,
};

enum amdgv_mca_error_type {
	AMDGV_MCA_ERROR_TYPE_UE = 0,
	AMDGV_MCA_ERROR_TYPE_CE,
	AMDGV_MCA_ERROR_TYPE_DE,
	AMDGV_MCA_ERROR_TYPE_LAST
};

enum amdgv_mca_cache_event {
	MCA_CACHE_EVENT_GUEST_LOAD,
	MCA_CACHE_EVENT_HOST_LOAD,
	MCA_CACHE_EVENT_NUM_VF_CHANGE,
	MCA_CACHE_EVENT_DISABLE_CACHE, /* For dynamic inf */
	MCA_CACHE_EVENT_ENABLE_CACHE, /* For dynamic inf */
};

enum mca_reg_idx {
	MCA_REG_IDX_CTL			= 0,
	MCA_REG_IDX_STATUS		= 1,
	MCA_REG_IDX_ADDR		= 2,
	MCA_REG_IDX_MISC0		= 3,
	MCA_REG_IDX_CONFIG		= 4,
	MCA_REG_IDX_IPID		= 5,
	MCA_REG_IDX_SYND		= 6,
	MCA_REG_IDX_COUNT		= 16,
};

struct mca_err_addr {
	struct amdgv_list_head node;
	uint64_t mca_status;
	uint64_t mca_ipid;
	uint64_t mca_addr;
};

struct mca_ecc {
	uint32_t new_ce_count;
	uint32_t total_ce_count;
	uint32_t new_ue_count;
	uint32_t total_ue_count;
	uint32_t new_de_count;
	uint32_t total_de_count;
	struct amdgv_list_head err_addr_list;
};

struct mca_bank_info {
	int socket_id;
	int aid;
	int hwid;
	int mcatype;
};

struct mca_bank_entry {
  int idx;
  enum amdgv_mca_error_type type;
  enum amdgv_mca_ip ip;
  struct mca_bank_info info;
  uint64_t regs[MCA_REG_IDX_COUNT];
};

struct block_ecc{
  struct mca_ecc *mca_ecc;
};

struct amdgv_mca_error_count_cache {
	uint32_t ce_count;
	uint32_t ue_count;
	uint32_t de_count;

	uint32_t ce_overflow_count;
	uint32_t ue_overflow_count;
	uint32_t de_overflow_count;
};

struct amdgv_mca_error_count_cache_client {
	bool enabled;
	struct amdgv_mca_error_count_cache cache[AMDGV_RAS_BLOCK__LAST];
};

struct amdgv_mca_error_count_cache_mgr {
	struct amdgv_mca_error_count_cache_client client[AMDGV_MAX_VF_SLOT];
};

struct amdgv_funcs {
	int (*set_debug_mode)(struct amdgv_adapter *adapt, bool enable);
	int (*get_new_banks)(struct amdgv_adapter *adapt, enum amdgv_mca_error_type type);
	int (*get_block_raw_ecc)(struct amdgv_adapter *adapt, enum amdgv_ras_block block,
				 struct block_ecc **blk_ecc);
	int (*pop_block_error_count)(struct amdgv_adapter *adapt, enum amdgv_ras_block block,
				     struct ras_err_data *err_data);
	int (*reset_block_error_count)(struct amdgv_adapter *adapt,
				       enum amdgv_ras_block block);
	int (*decode_block)(struct amdgv_adapter *adapt, struct mca_bank_entry *bank,
			    enum amdgv_ras_block *block);
};

struct amdgv_mca {

	bool enabled;
	uint32_t max_aid_xcd_num;

	const struct amdgv_funcs *funcs;
	struct block_ecc  ecc_block[AMDGV_RAS_BLOCK__LAST];
	/* Driver cannot decode unknown MCA bank. */
	uint32_t num_unknown_mca_entries;

	enum amdgv_ras_vf_telemetry_policy vf_policy;
	struct amdgv_mca_error_count_cache_mgr count_cache;
};

int amdgv_mca_get_new_banks(struct amdgv_adapter *adapt, enum amdgv_mca_error_type type);
int amdgv_mca_reset_block_error_count(struct amdgv_adapter *adapt, enum amdgv_ras_block block);
int amdgv_mca_decode_block(struct amdgv_adapter *adapt, struct mca_bank_entry *bank,
			   enum amdgv_ras_block *block);

int amdgv_mca_count_cache_reset(struct amdgv_adapter *adapt);
int amdgv_mca_count_cache_init(struct amdgv_adapter *adapt);
int amdgv_mca_count_cache_fini(struct amdgv_adapter *adapt);

int amdgv_mca_count_cache_put(struct amdgv_adapter *adapt,
			       uint32_t ce_count,
			       uint32_t ue_count,
			       uint32_t de_count,
			       enum amdgv_ras_block block);
int amdgv_mca_count_cache_client_get(struct amdgv_adapter *adapt,
				     uint32_t *ce_count,
				     uint32_t *ue_count,
				     uint32_t *de_count,
				     uint32_t *ce_overflow_count,
				     uint32_t *ue_overflow_count,
				     uint32_t *de_overflow_count,
				     enum amdgv_ras_block block,
				     uint32_t idx_vf);
int amdgv_mca_cache_notify_event(struct amdgv_adapter *adapt,
				 enum amdgv_mca_cache_event event,
				 uint32_t idx_vf,
				  uint32_t param);

int amdgv_mca_generate_cpers(struct amdgv_adapter *adapt, enum amdgv_mca_error_type type,
			     struct mca_bank_entry *banks, uint16_t bank_count);
int amdgv_mca_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_mca *mca_data);
int amdgv_mca_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_mca *mca_data);

int amdgv_mca_get_fatal_bank_from_cper_section(struct amdgv_adapter *adapt, struct cper_hdr *hdr,
					       uint32_t idx,
					       struct mca_bank_entry *bank);
int amdgv_mca_get_runtime_bank_from_cper_section(struct amdgv_adapter *adapt, struct cper_hdr *hdr,
						 uint32_t idx,
						 struct mca_bank_entry *bank);

#endif