/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __VCN_V2_6_H__
#define __VCN_V2_6_H__

enum amdgv_vcn_v2_6_sub_block {
	AMDGV_VCN_V2_6_VCPU_VCODEC = 0,
	AMDGV_VCN_V2_6_MMSCH,

	AMDGV_VCN_V2_6_MAX_SUB_BLOCK,
};

void vcn_v2_6_set_ras_funcs(struct amdgv_adapter *adapt);

#endif
