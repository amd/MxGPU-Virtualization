/*
 * Copyright (C) 2021  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <amdgv_device.h>

#include "mi300.h"
#include "mi300_psp.h"
#include "mi300_ucode.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static int mi300_ucode_individual_load(struct amdgv_adapter *adapt, uint32_t ucode_id)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;

	switch (ucode_id) {
	case AMDGV_FIRMWARE_ID__PSP_KEYDB:
		ret = mi300_psp_load_key_db(adapt, psp_key_database_bin_mi300,
					sizeof(psp_key_database_bin_mi300));
		break;
	case AMDGV_FIRMWARE_ID__PSP_SYS:
		ret = mi300_psp_load_sys_drv(adapt, psp_drv_sys_bin_mi300,
					sizeof(psp_drv_sys_bin_mi300));
		break;
	case AMDGV_FIRMWARE_ID__PSP_RAS:
		ret = mi300_psp_load_ras_drv(adapt, psp_drv_ras_bin_mi300,
					sizeof(psp_drv_ras_bin_mi300));
		break;
	case AMDGV_FIRMWARE_ID__PSP_SOC:
		ret = mi300_psp_load_soc_drv(adapt, psp_drv_soc_bin_mi300,
					sizeof(psp_drv_soc_bin_mi300));
		break;
	case AMDGV_FIRMWARE_ID__PSP_INTF:
		ret = mi300_psp_load_intf_drv(adapt, psp_drv_intf_bin_mi300,
					sizeof(psp_drv_intf_bin_mi300));
		break;
	case AMDGV_FIRMWARE_ID__PSP_DBG:
		ret = mi300_psp_load_dbg_drv(adapt, psp_drv_had_bin_mi300,
					sizeof(psp_drv_had_bin_mi300));
		break;
	case AMDGV_FIRMWARE_ID__PSP_SOS:
		ret = mi300_psp_load_sos(adapt, psp_sos_bin_mi300,
					sizeof(psp_sos_bin_mi300));
		break;
	case AMDGV_FIRMWARE_ID__RLC:
		ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aRLC_Ucode,
					   sizeof(aRLC_Ucode), ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__RLC_V:
		ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aRLCV_Ucode,
					   RLCV_UCODE_SIZE_SIGNED_BYTES,
					   ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__MMSCH:
		ret = amdgv_psp_load_np_fw(adapt,
					   (unsigned char *)a_MMSCH_SIGNED_Ucode,
					   sizeof(a_MMSCH_SIGNED_Ucode),
					   ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__SDMA0:
		ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode,
					   sizeof(aF32_SDMA0_Ucode), ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__SDMA1:
		ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode,
					   sizeof(aF32_SDMA0_Ucode), ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__SDMA2:
		ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode,
					   sizeof(aF32_SDMA0_Ucode), ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__SDMA3:
		ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode,
					   sizeof(aF32_SDMA0_Ucode), ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__SDMA4:
		ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode,
					   sizeof(aF32_SDMA0_Ucode), ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
		ret = amdgv_psp_load_np_fw(
			adapt, (unsigned char *)aRLC_RESTORE_LIST_GPM_MEM_SIGNED,
			RLC_RESTORE_LIST_GPM_MEM_SIGNED_SIZE_SIGNED_BYTES,
			ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
		ret = amdgv_psp_load_np_fw(
			adapt, (unsigned char *)aRLC_RESTORE_LIST_SRM_MEM_SIGNED,
			RLC_RESTORE_LIST_SRM_MEM_SIGNED_SIZE_SIGNED_BYTES,
			ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL:
		ret = amdgv_psp_load_np_fw(
			adapt, (unsigned char *)aRLC_RESTORE_LIST_CNTL_SIGNED,
			RLC_RESTORE_LIST_CNTL_SIGNED_SIZE_SIGNED_BYTES,
			ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
		ret = amdgv_psp_load_np_fw(adapt,
			(unsigned char *)psp_tos_wl_bin_mi300,
			sizeof(psp_tos_wl_bin_mi300),
			ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC1:
		ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_MEC_Ucode,
					   F32_MEC_UCODE_SIZE_SIGNED_BYTES,
					   ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
		ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_MEC_JumpTable,
					   F32_MEC_UCODE_JT_SIZE_SIGNED_BYTES,
					   ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__P2S_TABLE:
		ret = amdgv_psp_load_np_fw(adapt,
			(unsigned char *)mi300x_p2s_table,
			MI300X_P2S_TABLE_SIZE,
			ucode_id);
		break;
	case AMDGV_FIRMWARE_ID__DFC_FW:
		ret = amdgv_psp_load_fw(adapt,
			(unsigned char *)aDFC_FW,
			sizeof(aDFC_FW),
			ucode_id);
		break;
	default:
		AMDGV_INFO("Unsupported FW id 0x%x. Skipping ..\n", ucode_id);
		break;
	}

	return ret;
}

static int mi300_ucode_load(struct amdgv_adapter *adapt, enum amdgv_firmware_id *ucode_id_list,
			    uint32_t ucode_id_count)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	uint32_t i;

	for (i = 0; i < ucode_id_count; i++) {
		ret = mi300_ucode_individual_load(adapt, ucode_id_list[i]);

		if (ret != PSP_STATUS__SUCCESS)
			return AMDGV_FAILURE;
	}
	return 0;
}

/* Mi325 shares most FWs with Mi300 */
static int mi325_ucode_load(struct amdgv_adapter *adapt, enum amdgv_firmware_id *ucode_id_list,
			    uint32_t ucode_id_count)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	uint32_t i;

	for (i = 0; i < ucode_id_count; i++) {
		switch (ucode_id_list[i]) {
		case AMDGV_FIRMWARE_ID__PSP_KEYDB:
		case AMDGV_FIRMWARE_ID__PSP_SYS:
		case AMDGV_FIRMWARE_ID__PSP_RAS:
		case AMDGV_FIRMWARE_ID__PSP_SOC:
		case AMDGV_FIRMWARE_ID__PSP_INTF:
		case AMDGV_FIRMWARE_ID__PSP_DBG:
		case AMDGV_FIRMWARE_ID__PSP_SOS:
		case AMDGV_FIRMWARE_ID__RLC:
		case AMDGV_FIRMWARE_ID__RLC_V:
		case AMDGV_FIRMWARE_ID__MMSCH:
		case AMDGV_FIRMWARE_ID__SDMA0:
		case AMDGV_FIRMWARE_ID__SDMA1:
		case AMDGV_FIRMWARE_ID__SDMA2:
		case AMDGV_FIRMWARE_ID__SDMA3:
		case AMDGV_FIRMWARE_ID__SDMA4:
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL:
		case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
		case AMDGV_FIRMWARE_ID__CP_MEC1:
		case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
		case AMDGV_FIRMWARE_ID__DFC_FW:
			ret = mi300_ucode_individual_load(adapt, ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__P2S_TABLE:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)mi325_p2s_table,
				MI325_P2S_TABLE_SIZE,
				ucode_id_list[i]);
			break;
		default:
			AMDGV_INFO("Unsupported FW id 0x%x. Skipping ..\n", ucode_id_list[i]);
			break;
		}

		if (ret != PSP_STATUS__SUCCESS)
			return AMDGV_FAILURE;
	}
	return 0;
}

static int mi300_ucode_sw_init(struct amdgv_adapter *adapt)
{
	if (adapt->dev_id == 0x74A5) {
		AMDGV_INFO("set to Mi325 FW loading function\n");
		adapt->ucode.load = mi325_ucode_load;
	} else {
		AMDGV_INFO("set to Mi300 FW loading function\n");
		adapt->ucode.load = mi300_ucode_load;
	}

	return 0;
}

static int mi300_ucode_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->ucode.load = NULL;
	return 0;
}

static int mi300_ucode_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_ucode_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi300_ucode_func = {
	.name = "mi300_ucode_func",
	.sw_init = mi300_ucode_sw_init,
	.sw_fini = mi300_ucode_sw_fini,
	.hw_init = mi300_ucode_hw_init,
	.hw_fini = mi300_ucode_hw_fini,
};
