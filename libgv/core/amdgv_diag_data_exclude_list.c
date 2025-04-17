/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amdgv.h"

#ifndef EXCLUDE_FTRACE
char *host_driver_call_trace_exclude_list[] = {
	"amdgv_pcie_rreg",
	"amdgv_pcie_rreg64",
	"amdgv_mm_read_fb",
	"cail_pll_read",
	"cail_pll_write",
	"cail_fb_write",
	"cail_fb_read",
	"cail_ioreg_write",
	"cail_ioreg_read",
	"cail_reg_write",
	"cail_reg_read",
	"cail_mc_write",
	"cail_mc_read",
	"amdgv_memmgr_free",
	"amdgv_diag_data_trace_callback",
	"oss_atomic_set",
	"oss_atomic_inc_return",
	"amdgv_wait_for",
	"amdgv_wait_for_register_cb",
	"amdgv_vbios_wait_read_cb",
	"amdgv_mm_rreg",
	"amdgv_mm_wreg"
};
uint32_t host_driver_call_trace_exclude_list_len =
	(sizeof(host_driver_call_trace_exclude_list) / sizeof(const char *));
#endif
