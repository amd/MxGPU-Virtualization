/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI32_VCN_H
#define NAVI32_VCN_H

enum navi32_vcn_sub_block {
	NAVI32_VCN_VCPU_VCODEC = 0,
	NAVI32_VCN_MMSCH,

	NAVI32_VCN_MAX_SUB_BLOCK,
};

void navi32_vcn_set_ras_funcs(struct amdgv_adapter *adapt);

#endif
