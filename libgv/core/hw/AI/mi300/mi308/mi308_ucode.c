/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>

#include "mi300.h"
#include "mi300_psp.h"
#include "mi308_ucode.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static int mi308_ucode_load(struct amdgv_adapter *adapt, enum amdgv_firmware_id *ucode_id_list,
			    uint32_t ucode_id_count)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	uint32_t i;

	for (i = 0; i < ucode_id_count; i++) {
		switch (ucode_id_list[i]) {
		case AMDGV_FIRMWARE_ID__PSP_KEYDB:
			ret = mi300_psp_load_key_db(adapt, psp_key_database_bin_mi308,
						    sizeof(psp_key_database_bin_mi308));
			break;
		case AMDGV_FIRMWARE_ID__PSP_SYS:
			ret = mi300_psp_load_sys_drv(adapt, psp_sys_drv_bin_mi308,
						     sizeof(psp_sys_drv_bin_mi308));
			break;
		case AMDGV_FIRMWARE_ID__PSP_SOC:
			ret = mi300_psp_load_soc_drv(adapt, psp_drv_soc_mi308,
						     sizeof(psp_drv_soc_mi308));
			break;
		case AMDGV_FIRMWARE_ID__PSP_INTF:
			ret = mi300_psp_load_intf_drv(adapt, psp_drv_intf_mi308,
						      sizeof(psp_drv_intf_mi308));
			break;
		case AMDGV_FIRMWARE_ID__PSP_DBG:
			ret = mi300_psp_load_dbg_drv(adapt, psp_drv_had_mi308,
						     sizeof(psp_drv_had_mi308));
			break;
		case AMDGV_FIRMWARE_ID__PSP_SOS:
			ret = mi300_psp_load_sos(adapt, psp_sos_bin_mi308,
						 sizeof(psp_sos_bin_mi308));
			break;
		case AMDGV_FIRMWARE_ID__RLC:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aRLC_MI308_Ucode,
						   sizeof(aRLC_MI308_Ucode), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_V:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aRLCV_MI308_Ucode,
						   RLCV_MI308_UCODE_SIZE_SIGNED_BYTES,
						   ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__MMSCH:
			ret = amdgv_psp_load_np_fw(adapt,
						   (unsigned char *)a_MMSCH_MI308_SIGNED_Ucode,
						   sizeof(a_MMSCH_MI308_SIGNED_Ucode),
						   ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA0:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode_mi308,
						   sizeof(aF32_SDMA0_Ucode_mi308), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA1:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode_mi308,
						   sizeof(aF32_SDMA0_Ucode_mi308), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA2:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode_mi308,
						   sizeof(aF32_SDMA0_Ucode_mi308), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA3:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode_mi308,
						   sizeof(aF32_SDMA0_Ucode_mi308), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA4:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_SDMA0_Ucode_mi308,
						   sizeof(aF32_SDMA0_Ucode_mi308), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
			ret = amdgv_psp_load_np_fw(
				adapt, (unsigned char *)aRLC_RESTORE_LIST_GPM_MEM_MI308_SIGNED,
				RLC_RESTORE_LIST_GPM_MEM_MI308_SIGNED_SIZE_SIGNED_BYTES,
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
			ret = amdgv_psp_load_np_fw(
				adapt, (unsigned char *)aRLC_RESTORE_LIST_SRM_MEM_MI308_SIGNED,
				RLC_RESTORE_LIST_SRM_MEM_MI308_SIGNED_SIZE_SIGNED_BYTES,
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL:
			ret = amdgv_psp_load_np_fw(
				adapt, (unsigned char *)aRLC_RESTORE_LIST_CNTL_MI308_SIGNED,
				RLC_RESTORE_LIST_CNTL_MI308_SIGNED_SIZE_SIGNED_BYTES,
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__CP_MEC1:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_MEC_MI308_Ucode,
						   F32_MEC_MI308_UCODE_SIZE_SIGNED_BYTES,
						   ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aF32_MEC_MI308_JumpTable,
						   F32_MEC_MI308_UCODE_JT_SIZE_SIGNED_BYTES,
						   ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__P2S_TABLE:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)mi308x_p2s_table,
				MI308X_P2S_TABLE_SIZE,
				(ucode_id_list[i]));
			break;
		case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)psp_tos_wl_bin_mi308,
				sizeof(psp_tos_wl_bin_mi308),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__PSP_RAS:
			ret = mi300_psp_load_ras_drv(adapt,
				psp_drv_ras_mi308,
				sizeof(psp_drv_ras_mi308));
			break;
		default:
			AMDGV_WARN("Unsupported FW id 0x%x. Skipping...\n", ucode_id_list[i]);
			break;
		}
		if (ret != PSP_STATUS__SUCCESS)
			return AMDGV_FAILURE;
	}
	return 0;
}

static int mi308_ucode_sw_init(struct amdgv_adapter *adapt)
{
	adapt->ucode.load = mi308_ucode_load;
	return 0;
}

static int mi308_ucode_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->ucode.load = NULL;
	return 0;
}

static int mi308_ucode_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi308_ucode_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi308_ucode_func = {
	.name = "mi308_ucode_func",
	.sw_init = mi308_ucode_sw_init,
	.sw_fini = mi308_ucode_sw_fini,
	.hw_init = mi308_ucode_hw_init,
	.hw_fini = mi308_ucode_hw_fini,
};
