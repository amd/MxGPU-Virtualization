/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AMDGV_DF_H__
#define __AMDGV_DF_H__

struct amdgv_df_funcs {
	bool (*query_ras_poison_mode)(struct amdgv_adapter *adapt);
};

struct amdgv_df {
	const struct amdgv_df_funcs *funcs;
};

#endif
