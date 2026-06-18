/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI32_GC_H
#define NAVI32_GC_H

uint32_t gc_v11_0_3_read_grbm_status(struct amdgv_adapter *adapt);
uint32_t gc_v11_0_3_read_grbm_status2(struct amdgv_adapter *adapt);
uint32_t gc_v11_0_3_get_page_table_start_addr_lo32(struct amdgv_adapter *adapt);
uint32_t gc_v11_0_3_get_page_table_start_addr_hi32(struct amdgv_adapter *adapt);
uint32_t gc_v11_0_3_get_page_table_end_addr_lo32(struct amdgv_adapter *adapt);
uint32_t gc_v11_0_3_get_page_table_end_addr_hi32(struct amdgv_adapter *adapt);
int gc_v11_0_3_wait_grbm_clean(struct amdgv_adapter *adapt);
void gc_v11_0_3_toggle_rlcg_vf_interface(struct amdgv_adapter *adapt, bool enable);


#endif
