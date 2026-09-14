/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>

#include "mec_12_1_0_ucode_wrap.h"

/*
 * B0 MEC ucode translation unit: boundary-renames each b0/ blob symbol and
 * re-exports it, plus the START_ADDR scalars, via the getters declared in
 * mec_12_1_0_ucode_wrap.h.
 */

#define aRS64_MEC_PRODUCTION_UCODE	mec_12_1_0_b0_aRS64_MEC_PRODUCTION_UCODE
#include "b0/rs64_mec_ucode_signed.h"
#undef aRS64_MEC_PRODUCTION_UCODE

#define aRS64_MEC_PRODUCTION_DATA	mec_12_1_0_b0_aRS64_MEC_PRODUCTION_DATA
#include "b0/rs64_mec_data_signed.h"
#undef aRS64_MEC_PRODUCTION_DATA

unsigned char *mec_12_1_0_ucode_b0(uint32_t *size)
{
	*size = sizeof(mec_12_1_0_b0_aRS64_MEC_PRODUCTION_UCODE);
	return (unsigned char *)mec_12_1_0_b0_aRS64_MEC_PRODUCTION_UCODE;
}

unsigned char *mec_12_1_0_data_b0(uint32_t *size)
{
	*size = sizeof(mec_12_1_0_b0_aRS64_MEC_PRODUCTION_DATA);
	return (unsigned char *)mec_12_1_0_b0_aRS64_MEC_PRODUCTION_DATA;
}

void mec_12_1_0_ucode_get_start_addr_b0(uint32_t *lo, uint32_t *hi)
{
	*lo = RS64_MEC_PRODUCTION_UC_START_ADDR_LO;
	*hi = RS64_MEC_PRODUCTION_UC_START_ADDR_HI;
}
