/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI3_INIT_FUNC_H
#define NAVI3_INIT_FUNC_H

extern struct amdgv_init_func *navi32_init_table[];
extern struct amdgv_live_info_func *navi32_live_info_table[];
extern struct amdgv_reg_range *navi32_mitigation_table[];

extern void navi32_reg_base_init(struct amdgv_adapter *adapt);

#endif
