/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_AI_H
#define AMDGV_AI_H

#define mmMMHUB_VM_FB_LOCATION_TOP     0x6A0B4

extern struct amdgv_init_func *mi200_init_table[];
extern struct amdgv_reg_range *mi200_mitigation_table[];

extern struct amdgv_init_func *mi300x_init_table[];
extern struct amdgv_init_func *mi308x_init_table[];
extern struct amdgv_reg_range *mi300_mitigation_table[];
extern struct amdgv_init_func *mi350x_init_table[];
extern struct amdgv_reg_range *mi350_mitigation_table[];

extern void mi200_reg_base_init(struct amdgv_adapter *adapt);
extern void mi300_reg_base_init(struct amdgv_adapter *adapt);

#endif
