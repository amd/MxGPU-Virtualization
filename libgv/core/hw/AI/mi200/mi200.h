/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_MI200_H
#define AMDGV_MI200_H

#include <mi200/OSSSYS/osssys_4_2_0_offset.h>
#include <mi200/OSSSYS/osssys_4_2_0_sh_mask.h>

#include <mi200/NBIO/nbio_7_4_offset.h>
#include <mi200/NBIO/nbio_7_4_sh_mask.h>

#include <mi200/SDMA0/sdma0_4_2_offset.h>
#include <mi200/SDMA0/sdma0_4_2_sh_mask.h>
#include <mi200/SDMA1/sdma1_4_2_offset.h>
#include <mi200/SDMA1/sdma1_4_2_sh_mask.h>
#include <mi200/SDMA2/sdma2_4_2_2_offset.h>
#include <mi200/SDMA2/sdma2_4_2_2_sh_mask.h>
#include <mi200/SDMA3/sdma3_4_2_2_offset.h>
#include <mi200/SDMA3/sdma3_4_2_2_sh_mask.h>
#include <mi200/SDMA4/sdma4_4_2_2_offset.h>
#include <mi200/SDMA4/sdma4_4_2_2_sh_mask.h>

#include "mi200/GC/gc_9_4_offset.h"
#include "mi200/GC/gc_9_4_sh_mask.h"

#include <mi200/MP/mp_13_0_2_offset.h>
#include <mi200/MP/mp_13_0_2_sh_mask.h>

#include "mi200/SMUIO/smuio_13_0_2_offset.h"
#include "mi200/SMUIO/smuio_13_0_2_sh_mask.h"

#include <mi200/HDP/hdp_4_0_offset.h>
#include <mi200/HDP/hdp_4_0_sh_mask.h>

#include "mi200/UMC/umc_6_7_0_sh_mask.h"
#include "mi200/UMC/umc_6_7_0_offset.h"

#include <mi200/DF/df_3_6_offset.h>
#include <mi200/DF/df_3_6_sh_mask.h>

#include <mi200/ATHUB/athub_1_0_offset.h>
#include <mi200/ATHUB/athub_1_0_sh_mask.h>

#include <mi200/MMHUB/mmhub_1_7_offset.h>

#include "mi200/mi200_ip_offset.h"


/* On Mi200, some golden settings registers are designed as PF only,
 * KMD in VF is not able to write them. Program them here from PF.
 */

#define mmMC_VM_XGMI_LFB_CNTL_ALDE                      0x0978
#define mmMC_VM_XGMI_LFB_CNTL_ALDE_BASE_IDX             0
#define mmMC_VM_XGMI_LFB_SIZE_ALDE                      0x0979
#define mmMC_VM_XGMI_LFB_SIZE_ALDE_BASE_IDX             0
//MC_VM_XGMI_LFB_CNTL
#define MC_VM_XGMI_LFB_CNTL_ALDE__PF_LFB_REGION__SHIFT  0x0
#define MC_VM_XGMI_LFB_CNTL_ALDE__PF_MAX_REGION__SHIFT  0x4
#define MC_VM_XGMI_LFB_CNTL_ALDE__PF_LFB_REGION_MASK    0x0000000FL
#define MC_VM_XGMI_LFB_CNTL_ALDE__PF_MAX_REGION_MASK    0x000000F0L
// MC_VM_XGMI_LFB_SIZE
#define MC_VM_XGMI_LFB_SIZE_ALDE__PF_LFB_SIZE__SHIFT    0x0
#define MC_VM_XGMI_LFB_SIZE_ALDE__PF_LFB_SIZE_MASK      0x0001FFFFL

#define mmnbif_gpu_VF_REGWR_EN                          0x0E44
#define mmnbif_gpu_VF_DOORBELL_EN                       0x0E45
#define mmnbif_gpu_VF_FB_EN                             0x0E46

//FLR
#define mmnbif_gpu_RCC_DEV0_EPF0_STRAP4                 0x0D3F
#define STRAP_FLR_EN_DEV0_F0_MASK                       0x00400000L

#define mmIH_CHICKEN_MI200                              0x18d
#define mmIH_CHICKEN_MI200_BASE_IDX                     0

extern struct amdgv_init_func *mi200_init_table[];
extern struct amdgv_reg_range *mi200_mitigation_table[];

#endif
