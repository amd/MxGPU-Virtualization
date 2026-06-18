/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
