/*
 * Copyright (C) 2022  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
