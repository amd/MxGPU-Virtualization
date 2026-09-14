/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>

#include "mec_12_1_0_ucode_wrap.h"

/*
 * A0 MEC ucode translation unit: boundary-renames each a0/ blob symbol and
 * re-exports it, plus the START_ADDR scalars, via the getters declared in
 * mec_12_1_0_ucode_wrap.h.
 */

#define aRS64_MEC_PRODUCTION_UCODE	mec_12_1_0_a0_aRS64_MEC_PRODUCTION_UCODE
#include "a0/rs64_mec_ucode_signed.h"
#undef aRS64_MEC_PRODUCTION_UCODE

#define aRS64_MEC_PRODUCTION_DATA	mec_12_1_0_a0_aRS64_MEC_PRODUCTION_DATA
#include "a0/rs64_mec_data_signed.h"
#undef aRS64_MEC_PRODUCTION_DATA

unsigned char *mec_12_1_0_ucode_a0(uint32_t *size)
{
	*size = sizeof(mec_12_1_0_a0_aRS64_MEC_PRODUCTION_UCODE);
	return (unsigned char *)mec_12_1_0_a0_aRS64_MEC_PRODUCTION_UCODE;
}

unsigned char *mec_12_1_0_data_a0(uint32_t *size)
{
	*size = sizeof(mec_12_1_0_a0_aRS64_MEC_PRODUCTION_DATA);
	return (unsigned char *)mec_12_1_0_a0_aRS64_MEC_PRODUCTION_DATA;
}

void mec_12_1_0_ucode_get_start_addr_a0(uint32_t *lo, uint32_t *hi)
{
	*lo = RS64_MEC_PRODUCTION_UC_START_ADDR_LO;
	*hi = RS64_MEC_PRODUCTION_UC_START_ADDR_HI;
}
