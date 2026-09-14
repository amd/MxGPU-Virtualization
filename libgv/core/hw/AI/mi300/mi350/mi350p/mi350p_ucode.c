/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>

#include "mi300.h"
#include "mi300_psp.h"
#include "mi350p_ucode.h"

int mi350_ucode_load(struct amdgv_adapter *adapt,
		     enum amdgv_firmware_id *ucode_id_list,
		     uint32_t ucode_id_count);

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

int mi350p_ucode_load(struct amdgv_adapter *adapt,
		      enum amdgv_firmware_id *ucode_id_list,
		      uint32_t ucode_id_count)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	uint32_t i;

	for (i = 0; i < ucode_id_count; i++) {
		switch (ucode_id_list[i]) {
		case AMDGV_FIRMWARE_ID__PSP_KEYDB:
			ret = mi300_psp_load_key_db(adapt, psp_keydb_mi350,
						    sizeof(psp_keydb_mi350));
			break;
		case AMDGV_FIRMWARE_ID__PSP_SYS:
			ret = mi300_psp_load_sys_drv(adapt, psp_drv_sys_bin_mi350,
						     sizeof(psp_drv_sys_bin_mi350));
			break;
		case AMDGV_FIRMWARE_ID__PSP_RAS:
			ret = mi300_psp_load_ras_drv(adapt,
				psp_drv_ras_bin_mi350,
				sizeof(psp_drv_ras_bin_mi350));
			break;
		case AMDGV_FIRMWARE_ID__PSP_SOC:
			ret = mi300_psp_load_soc_drv(adapt, psp_drv_soc_bin_mi350,
						     sizeof(psp_drv_soc_bin_mi350));
			break;
		case AMDGV_FIRMWARE_ID__PSP_INTF:
			ret = mi300_psp_load_intf_drv(adapt, psp_drv_intf_bin_mi350,
						      sizeof(psp_drv_intf_bin_mi350));
			break;
		case AMDGV_FIRMWARE_ID__PSP_DBG:
			ret = mi300_psp_load_dbg_drv(adapt, psp_drv_had_bin_mi350,
						     sizeof(psp_drv_had_bin_mi350));
			break;
		case AMDGV_FIRMWARE_ID__PSP_SOS:
			ret = mi300_psp_load_sos(adapt, psp_sos_bin_mi350,
						 sizeof(psp_sos_bin_mi350));
			break;
		case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
			ret = amdgv_psp_load_np_fw(adapt,
				(unsigned char *)psp_tos_wl_bin_mi350,
				sizeof(psp_tos_wl_bin_mi350),
				ucode_id_list[i]);
			break;
		default:
			ret = mi350_ucode_load(adapt, &ucode_id_list[i], 1);
			break;
		}
		if (ret != PSP_STATUS__SUCCESS)
			return AMDGV_FAILURE;
	}

	return 0;
}

static int mi350p_ucode_sw_init(struct amdgv_adapter *adapt)
{
	adapt->ucode.load = mi350p_ucode_load;
	return 0;
}

static int mi350p_ucode_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->ucode.load = NULL;
	return 0;
}

static int mi350p_ucode_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi350p_ucode_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi350p_ucode_func = {
	.name = "mi350p_ucode_func",
	.sw_init = mi350p_ucode_sw_init,
	.sw_fini = mi350p_ucode_sw_fini,
	.hw_init = mi350p_ucode_hw_init,
	.hw_fini = mi350p_ucode_hw_fini,
};
