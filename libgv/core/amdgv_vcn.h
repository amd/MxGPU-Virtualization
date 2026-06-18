/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AMDGV_VCN_H__
#define __AMDGV_VCN_H__

struct amdgv_vcn_ras_funcs {
	void (*query_poison_status)(struct amdgv_adapter *adapt);
};

struct amdgv_vcn {
	int num_instances;
	struct ras_common_if	*ras_if;
	const struct amdgv_vcn_ras_funcs	*funcs;
};

#endif
