/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>

#include "rlc_12_1_0_ucode_wrap.h"

/*
 * A0 RLC ucode translation unit: boundary-renames each a0/ blob symbol and
 * re-exports it via the getters declared in rlc_12_1_0_ucode_wrap.h.
 */

#define aRLC_Ucode			rlc_12_1_0_a0_aRLC_Ucode
#include "a0/f32_gpm_ucode_signed.h"
#undef aRLC_Ucode

#define RLC_TOC_DATA			rlc_12_1_0_a0_RLC_TOC_DATA
#include "a0/rlc_toc_data_signed.h"
#undef RLC_TOC_DATA

#define aRLC_RESTORE_LIST_GPM_MEM	rlc_12_1_0_a0_aRLC_RESTORE_LIST_GPM_MEM
#include "a0/rlc_restore_list_gpm_mem_signed.h"
#undef aRLC_RESTORE_LIST_GPM_MEM

#define aRLC_RESTORE_LIST_SRM_MEM	rlc_12_1_0_a0_aRLC_RESTORE_LIST_SRM_MEM
#include "a0/rlc_restore_list_srm_mem_signed.h"
#undef aRLC_RESTORE_LIST_SRM_MEM

#define aLX6_IRAM_UCODE			rlc_12_1_0_a0_aLX6_IRAM_UCODE
#include "a0/rlc_lx6_iram_ucode_signed.h"
#undef aLX6_IRAM_UCODE

#define aLX6_DRAM_UCODE			rlc_12_1_0_a0_aLX6_DRAM_UCODE
#include "a0/rlc_lx6_dram_ucode_signed.h"
#undef aLX6_DRAM_UCODE

#define aLX6_CORE1_IRAM_UCODE		rlc_12_1_0_a0_aLX6_CORE1_IRAM_UCODE
#include "a0/rlc_lx6_1_iram_ucode_signed.h"
#undef aLX6_CORE1_IRAM_UCODE

#define aLX6_CORE1_DRAM_UCODE		rlc_12_1_0_a0_aLX6_CORE1_DRAM_UCODE
#include "a0/rlc_lx6_1_dram_ucode_signed.h"
#undef aLX6_CORE1_DRAM_UCODE

unsigned char *rlc_12_1_0_gpm_ucode_a0(uint32_t *size)
{
	*size = sizeof(rlc_12_1_0_a0_aRLC_Ucode);
	return (unsigned char *)rlc_12_1_0_a0_aRLC_Ucode;
}

unsigned char *rlc_12_1_0_toc_data_a0(uint32_t *size)
{
	*size = sizeof(rlc_12_1_0_a0_RLC_TOC_DATA);
	return (unsigned char *)rlc_12_1_0_a0_RLC_TOC_DATA;
}

unsigned char *rlc_12_1_0_restore_list_gpm_mem_a0(uint32_t *size)
{
	*size = sizeof(rlc_12_1_0_a0_aRLC_RESTORE_LIST_GPM_MEM);
	return (unsigned char *)rlc_12_1_0_a0_aRLC_RESTORE_LIST_GPM_MEM;
}

unsigned char *rlc_12_1_0_restore_list_srm_mem_a0(uint32_t *size)
{
	*size = sizeof(rlc_12_1_0_a0_aRLC_RESTORE_LIST_SRM_MEM);
	return (unsigned char *)rlc_12_1_0_a0_aRLC_RESTORE_LIST_SRM_MEM;
}

unsigned char *rlc_12_1_0_lx6_iram_ucode_a0(uint32_t *size)
{
	*size = sizeof(rlc_12_1_0_a0_aLX6_IRAM_UCODE);
	return (unsigned char *)rlc_12_1_0_a0_aLX6_IRAM_UCODE;
}

unsigned char *rlc_12_1_0_lx6_dram_ucode_a0(uint32_t *size)
{
	*size = sizeof(rlc_12_1_0_a0_aLX6_DRAM_UCODE);
	return (unsigned char *)rlc_12_1_0_a0_aLX6_DRAM_UCODE;
}

unsigned char *rlc_12_1_0_lx6_core1_iram_ucode_a0(uint32_t *size)
{
	*size = sizeof(rlc_12_1_0_a0_aLX6_CORE1_IRAM_UCODE);
	return (unsigned char *)rlc_12_1_0_a0_aLX6_CORE1_IRAM_UCODE;
}

unsigned char *rlc_12_1_0_lx6_core1_dram_ucode_a0(uint32_t *size)
{
	*size = sizeof(rlc_12_1_0_a0_aLX6_CORE1_DRAM_UCODE);
	return (unsigned char *)rlc_12_1_0_a0_aLX6_CORE1_DRAM_UCODE;
}
