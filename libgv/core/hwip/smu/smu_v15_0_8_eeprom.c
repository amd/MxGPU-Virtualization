/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include "smu_v15_0_8_internal.h"

static const uint32_t this_block = AMDGV_POWER_BLOCK;

static const struct amdgv_pmme_funcs smu_v15_0_8_pmme_funcs = {
	.is_pmfw_managed_eeprom = NULL,
	.is_pmme_ready = NULL,
	.get_ras_table_version = NULL,
	.get_rma_status = NULL,
	.get_bad_page_count = NULL,
	.get_bad_page_address = NULL,
	.get_bad_page_info_details = NULL,
	.set_eeprom_timestamp = NULL,
	.get_ras_policy_details = NULL,
	.erase_ras_table = NULL,
};

static int smu_v15_0_8_eeprom_sw_init(struct amdgv_adapter *adapt)
{
	adapt->pp.pmme_funcs = &smu_v15_0_8_pmme_funcs;

	return 0;
}

static int smu_v15_0_8_eeprom_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int smu_v15_0_8_eeprom_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int smu_v15_0_8_eeprom_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func smu_v15_0_8_eeprom_func = {
	.name = "smu_v15_0_8_eeprom_func",
	.sw_init = smu_v15_0_8_eeprom_sw_init,
	.sw_fini = smu_v15_0_8_eeprom_sw_fini,
	.hw_init = smu_v15_0_8_eeprom_hw_init,
	.hw_fini = smu_v15_0_8_eeprom_hw_fini,
};