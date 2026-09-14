/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>

#include "rlc_12_1_0_ucode_wrap.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

void rlc_12_1_0_ucode_register(struct amdgv_adapter *adapt)
{
	adapt->ucode.get_rlc_ucode = rlc_12_1_0_ucode_get;
}

int rlc_12_1_0_ucode_get(struct amdgv_adapter *adapt, enum amdgv_firmware_id fw_id,
			 unsigned char **ptr, uint32_t *size)
{
	/* rev_id == 0 selects A0, any other rev_id selects B0. */
	bool is_a0 = (adapt->rev_id == 0);

	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__RLC:
		*ptr = is_a0 ? rlc_12_1_0_gpm_ucode_a0(size) :
			       rlc_12_1_0_gpm_ucode_b0(size);
		break;
	case AMDGV_FIRMWARE_ID__PSP_TOC:
		*ptr = is_a0 ? rlc_12_1_0_toc_data_a0(size) :
			       rlc_12_1_0_toc_data_b0(size);
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
		*ptr = is_a0 ? rlc_12_1_0_restore_list_gpm_mem_a0(size) :
			       rlc_12_1_0_restore_list_gpm_mem_b0(size);
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
		*ptr = is_a0 ? rlc_12_1_0_restore_list_srm_mem_a0(size) :
			       rlc_12_1_0_restore_list_srm_mem_b0(size);
		break;
	case AMDGV_FIRMWARE_ID__RLX6:
		*ptr = is_a0 ? rlc_12_1_0_lx6_iram_ucode_a0(size) :
			       rlc_12_1_0_lx6_iram_ucode_b0(size);
		break;
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT:
		*ptr = is_a0 ? rlc_12_1_0_lx6_dram_ucode_a0(size) :
			       rlc_12_1_0_lx6_dram_ucode_b0(size);
		break;
	case AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1:
		*ptr = is_a0 ? rlc_12_1_0_lx6_core1_iram_ucode_a0(size) :
			       rlc_12_1_0_lx6_core1_iram_ucode_b0(size);
		break;
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1:
		*ptr = is_a0 ? rlc_12_1_0_lx6_core1_dram_ucode_a0(size) :
			       rlc_12_1_0_lx6_core1_dram_ucode_b0(size);
		break;
	default:
		AMDGV_ERROR("RLC ucode accessor: unknown firmware ID %d\n", fw_id);
		return AMDGV_FAILURE;
	}

	return 0;
}
