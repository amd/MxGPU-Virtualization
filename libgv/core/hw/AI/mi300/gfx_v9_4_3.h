/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __GFX_V9_4_3_H__
#define __GFX_V9_4_3_H__

#include "amdgv.h"
#include "amdgv_device.h"

#define XCC_TO_DIE(_XCC_) (((_XCC_) & 0x1) ? 1 : 0)

#define GFX9_MEC_HPD_SIZE 4096
#define CP_HQD_PERSISTENT_STATE_DEFAULT 0xbe05301

void gfx_v9_4_3_set_funcs(struct amdgv_adapter *adapt);

void gfx_v9_4_2_dirtybit_control(struct amdgv_adapter *adapt, bool enable);

#endif