/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "mi200/NBIO/nbio_7_4_offset.h"
#include "mi200/NBIO/nbio_7_4_sh_mask.h"
#include "mi200/NBIO/nbio_7_4_0_smn.h"
#include "amdgv_device.h"
#include "amdgv_ras.h"
#include "amdgv_nbio.h"
#include "nbio_v7_4.h"
#include "mi200_nbio.h"

uint32_t mi200_nbio_get_total_vram_size(struct amdgv_adapter *adapt)
{
	return RREG32_SOC15(NBIO, 0, mmRCC_CONFIG_MEMSIZE);
}
