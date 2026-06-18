/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_sriovmsg.h"

#ifndef AMDGV_VFMGR_XCHG_H
#define AMDGV_VFMGR_XCHG_H

int amdgv_vfmgr_copy_to_vf_xchg_table(struct amdgv_adapter *adapt, uint32_t idx_vf,
				      enum amd_sriov_msg_table_id_enum table_id,
				      uint64_t offset_in_table, void *buf, uint32_t size);
int amdgv_vfmgr_copy_from_vf_xchg_table(struct amdgv_adapter *adapt, uint32_t idx_vf,
					enum amd_sriov_msg_table_id_enum table_id,
					uint64_t offset_in_table, void *buf, uint32_t size);
int amdgv_vfmgr_init_xchg_region(struct amdgv_adapter* adapt, uint32_t idx_vf,
				 uint64_t gpa_base, uint32_t size);
int amdgv_vfmgr_fini_xchg_region(struct amdgv_adapter* adapt, uint32_t idx_vf);

#endif