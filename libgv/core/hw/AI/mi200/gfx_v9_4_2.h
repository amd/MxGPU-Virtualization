/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __GFX_V9_4_2_H__
#define __GFX_V9_4_2_H__

#include "amdgv.h"
#include "amdgv_device.h"

void gfx_v9_4_2_set_funcs(struct amdgv_adapter *adapt);
void gfx_v9_4_2_set_power_brake_sequence(struct amdgv_adapter *adapt);
int gfx_v9_4_2_do_edc_gpr_wa(struct amdgv_adapter *adapt);

#endif
