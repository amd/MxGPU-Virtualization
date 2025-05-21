/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <amdgv_device.h>

#include "mi200.h"
#include "psp_v13_0.h"
#include "mi200_ucode.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static enum psp_status mi200_fw_compatibility_check(
		struct amdgv_adapter *adapt,
		unsigned char *fw_image, uint32_t fw_id)
{
	struct psp_fw_image_header *fw_header;
	uint32_t product, version;

	fw_header = (struct psp_fw_image_header *)fw_image;
	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__CP_MEC1:
	case AMDGV_FIRMWARE_ID__CP_MEC2:
		/* For SCM A0 unsecured board, the IFWI is v63 (sec_version is
		 * 0xd204032, build number is '605015'). There may not have new
		 * IFWI updates for For SCM A0 unsecured board. The version
		 * checking need be passed.
		 */
		if (((fw_header->image_version & 0xFF) >= 0x45) &&
				(oss_strncmp(adapt->vbios.build_num, "605015",
				BUILD_NUM_LENGTH) != 0)) {
			product = adapt->vbios.sec_version & 0x00FF0000;
			version = adapt->vbios.sec_version & 0xFF;

			if (((product == 0x00200000) && (version < 0x35)) ||
					((product == 0x00220000) && (version < 0x05))) {
				AMDGV_ERROR("Version Compatibility Error: "
					"FW 0x%x Ver = 0x%x. Sec Ver = 0x%x\n",
					fw_id, fw_header->image_version,
					adapt->vbios.sec_version);
				return PSP_STATUS__ERROR_GENERIC;
			}
		}
		break;
	}
	return PSP_STATUS__SUCCESS;
}

static int mi200_ucode_load(struct amdgv_adapter *adapt,
		enum amdgv_firmware_id *ucode_id_list, uint32_t ucode_id_count)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	uint32_t i;

	for (i = 0; i < ucode_id_count; i++) {
		switch (ucode_id_list[i]) {
		case AMDGV_FIRMWARE_ID__PSP_KEYDB:
			ret = psp_v13_load_key_db(adapt,
				psp_key_database_bin_mi200,
				sizeof(psp_key_database_bin_mi200));
			break;
		case AMDGV_FIRMWARE_ID__PSP_SYS:
			ret = psp_v13_load_sysdrv(adapt,
				psp_sys_drv_bin_mi200,
				sizeof(psp_sys_drv_bin_mi200));
			break;
		case AMDGV_FIRMWARE_ID__PSP_SOS:
			ret = psp_v13_load_sos(adapt,
				psp_sos_bin_mi200,
				sizeof(psp_sos_bin_mi200));
			break;
		case AMDGV_FIRMWARE_ID__SMU:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)mi200_smc_firmware,
				sizeof(mi200_smc_firmware),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aRLC_Ucode,
				sizeof(aRLC_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_V:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aRLCV_Ucode,
				RLCV_UCODE_SIZE_SIGNED_BYTES,
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__MMSCH:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)a_MMSCH_SIGNED_Ucode,
				sizeof(a_MMSCH_SIGNED_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA0:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aF32_SDMA0_Ucode,
				sizeof(aF32_SDMA0_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA1:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aF32_SDMA0_Ucode,
				sizeof(aF32_SDMA0_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA2:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aF32_SDMA0_Ucode,
				sizeof(aF32_SDMA0_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA3:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aF32_SDMA0_Ucode,
				sizeof(aF32_SDMA0_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA4:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aF32_SDMA0_Ucode,
				sizeof(aF32_SDMA0_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)
					aRLC_RESTORE_LIST_GPM_MEM_SIGNED,
			RLC_RESTORE_LIST_GPM_MEM_SIGNED_SIZE_SIGNED_BYTES,
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)
					aRLC_RESTORE_LIST_SRM_MEM_SIGNED,
			RLC_RESTORE_LIST_SRM_MEM_SIGNED_SIZE_SIGNED_BYTES,
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aRLC_RESTORE_LIST_CNTL_SIGNED,
				RLC_RESTORE_LIST_CNTL_SIGNED_SIZE_SIGNED_BYTES,
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)psp_tos_wl_bin_mi200,
				sizeof(psp_tos_wl_bin_mi200),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__DFC_FW:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aDFC_FW,
				sizeof(aDFC_FW),
				(ucode_id_list[i]));
			break;
		case AMDGV_FIRMWARE_ID__CP_MEC1:
			ret = mi200_fw_compatibility_check(adapt,
					(unsigned char *)aF32_MEC_Ucode,
					AMDGV_FIRMWARE_ID__CP_MEC1);
			if (ret)
				break;
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aF32_MEC_Ucode,
				sizeof(aF32_MEC_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)aF32_MEC_JumpTable,
				sizeof(aF32_MEC_JumpTable),
				ucode_id_list[i]);
			break;
		default:
			AMDGV_INFO("Unsupported FW id 0x%x. Skipping ..\n",
				ucode_id_list[i]);
			break;
		}
		if (ret != PSP_STATUS__SUCCESS)
			return AMDGV_FAILURE;
	}
	return 0;
}

static int mi200_ucode_sw_init(struct amdgv_adapter *adapt)
{
	adapt->ucode.load = mi200_ucode_load;
	return 0;
}

static int mi200_ucode_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->ucode.load = NULL;
	return 0;
}

static int mi200_ucode_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi200_ucode_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi200_ucode_func = {
	.name = "mi200_ucode_func",
	.sw_init = mi200_ucode_sw_init,
	.sw_fini = mi200_ucode_sw_fini,
	.hw_init = mi200_ucode_hw_init,
	.hw_fini = mi200_ucode_hw_fini,
};
