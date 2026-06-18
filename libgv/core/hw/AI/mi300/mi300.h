/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_MI300_H
#define AMDGV_MI300_H

#include <mi300/ATHUB/athub_1_8_0_offset.h>
#include <mi300/ATHUB/athub_1_8_0_sh_mask.h>
#include <mi300/OSSSYS/osssys_4_4_2_offset.h>
#include <mi300/OSSSYS/osssys_4_4_2_sh_mask.h>
#include <mi300/SDMA/sdma_4_4_2_offset.h>
#include <mi300/SDMA/sdma_4_4_2_sh_mask.h>

#include <mi300/MP/mp_13_0_6_offset.h>
#include <mi300/MP/mp_13_0_6_sh_mask.h>

#include "mi300/SMUIO/smuio_13_0_3_offset.h"
#include "mi300/SMUIO/smuio_13_0_3_sh_mask.h"

#include <mi300/HDP/hdp_4_4_2_offset.h>
#include <mi300/HDP/hdp_4_4_2_sh_mask.h>

#include <mi300/UMC/umc_12_0_offset.h>
#include <mi300/UMC/umc_12_0_sh_mask.h>

#include <mi300/GC/gc_9_4_3_offset.h>
#include <mi300/GC/gc_9_4_3_sh_mask.h>

#include <mi300/NBIO/nbio_7_9_0_offset.h>

#include "mi300/mi300_hw_ip.h"

#define AMDGV_MI300_CROSS_AID_ACCESS(x) ((x) ? (((uint64_t)1 << 34) / 4) : 0)

#define RREG32_SOC15_EXT(ip, inst, reg, ext)                                                  \
	RREG32_PCIE_EXT((adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg) |          \
			AMDGV_MI300_CROSS_AID_ACCESS(ext))

#define WREG32_SOC15_EXT(ip, inst, reg, ext, value)                                           \
	WREG32_PCIE_EXT((adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg) |          \
				AMDGV_MI300_CROSS_AID_ACCESS(ext),                            \
			value)

#define mmCC_BIF_BX_STRAP0 0x0E02

#define mmnbif_gpu_VF_REGWR_EN	  0x0E44
#define mmnbif_gpu_VF_DOORBELL_EN 0x0E45
#define mmnbif_gpu_VF_FB_EN	  0x0E46

#define mmnbif_gpu_RCC_DEV0_EPF0_STRAP4 0x0D3F
#define STRAP_FLR_EN_DEV0_F0_MASK	0x00400000L

typedef enum _AMDGV_MI300_DOORBELL_ASSIGNMENT {
	/* Compute: 0~255 */
	/* SDMA: 0x100 ~ 0x19F */
	AMDGV_MI300_DOORBELL_sDMA_ENGINE0 = 0x100,
	AMDGV_MI300_DOORBELL_sDMA_ENGINE1 = 0x10A,
	AMDGV_MI300_DOORBELL_sDMA_ENGINE2 = 0x114,
	AMDGV_MI300_DOORBELL_sDMA_ENGINE3 = 0x11E,
	// ... each SDMA enigne has 0x0A offset
	// 	AMDGV_MI300_DOORBELL_sDMA_ENGINE15 = 0x196,

	/* IH: 0x1A0 ~ 0x1AF */
	AMDGV_MI300_DOORBELL_IH = 0x1A0,
	/* MMSCH: 0x1B0 ~ 0x1C2 */
	AMDGV_MI300_DOORBELL_MMSCH0 = 0x1B0
} AMDGV_MI300_DOORBELL_ASSIGNMENT;

#endif
