/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv.h"
#include "amdgv_vcn.h"

#include "asic_reg/VCN/vcn_5_0_0_sh_mask.h"
#include "asic_reg/VCN/vcn_5_0_0_offset.h"

#include "vcn_v5_0_2.h"
#include "gpuiov/gpuiov_v9_0.h"

static const uint32_t this_block = AMDGV_MULTIMEDIA_BLOCK;

