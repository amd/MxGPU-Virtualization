/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI3_UCODE_H
#define NAVI3_UCODE_H

#include <amdgv_device.h>
#include <amdgv_psp.h>

void navi32_log_toc_version(struct amdgv_adapter *adapt);

struct navi32_ucode_context {
	struct psp_local_memory cp_pfp_ucode;
	struct psp_local_memory cp_ce_ucode;
	struct psp_local_memory cp_me_ucode;
	struct psp_local_memory cp_mec_ucode;
};

#endif
