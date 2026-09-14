/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>

#include "mec_12_1_0_ucode_wrap.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

void mec_12_1_0_ucode_register(struct amdgv_adapter *adapt)
{
	adapt->ucode.get_mec_ucode = mec_12_1_0_ucode_get;
	adapt->ucode.get_mec_ucode_start_addr = mec_12_1_0_ucode_get_start_addr;
}

int mec_12_1_0_ucode_get(struct amdgv_adapter *adapt, enum amdgv_firmware_id fw_id,
			 unsigned char **ptr, uint32_t *size)
{
	/* rev_id == 0 selects A0, any other rev_id selects B0. */
	bool is_a0 = (adapt->rev_id == 0);

	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
		*ptr = is_a0 ? mec_12_1_0_ucode_a0(size) :
			       mec_12_1_0_ucode_b0(size);
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA:
	case AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA:
	case AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA:
	case AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA:
		*ptr = is_a0 ? mec_12_1_0_data_a0(size) :
			       mec_12_1_0_data_b0(size);
		break;
	default:
		AMDGV_ERROR("MEC ucode accessor: unknown firmware ID %d\n", fw_id);
		return AMDGV_FAILURE;
	}

	return 0;
}

void mec_12_1_0_ucode_get_start_addr(struct amdgv_adapter *adapt, uint32_t *lo, uint32_t *hi)
{
	if (adapt->rev_id == 0)
		mec_12_1_0_ucode_get_start_addr_a0(lo, hi);
	else
		mec_12_1_0_ucode_get_start_addr_b0(lo, hi);
}
