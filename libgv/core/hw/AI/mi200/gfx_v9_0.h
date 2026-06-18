/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __GFX_V9_0_H__
#define __GFX_V9_0_H__

#define NUM_IDS 64

void mi200_gfx_select_se_sh(struct amdgv_adapter *adapt,
		uint32_t se, uint32_t sh, uint32_t instance);
#endif
