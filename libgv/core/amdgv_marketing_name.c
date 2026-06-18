/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_asic.h"
#include "amdgv_marketing_name.h"

/*
 * DO NOT interpolate or extrapolate entries as
 * all entries must be official approved.
 */
static const struct amdgv_marketing_name_entry amdgv_marketing_name_table[] = {

	/* Mi200 PF */
	{ CHIP_MI200, 0x7408, 0x00, "AMD Instinct MI250X"},
	{ CHIP_MI200, 0x740C, 0x01, "AMD Instinct MI250X/MI250"},
	{ CHIP_MI200, 0x740F, 0x02, "AMD Instinct MI210"},

	/* Mi300 PF */
	{ CHIP_MI300X, 0x74A1, 0x00, "AMD Instinct MI300X"},
	{ CHIP_MI308X, 0x74A2, 0x00, "AMD Instinct MI308X"},
	{ CHIP_MI308X, 0x74A8, 0x00, "AMD Instinct MI308X"},
	{ CHIP_MI300X, 0x74A5, 0x00, "AMD Instinct MI325X"},
	{ CHIP_MI300X, 0x74A9, 0x00, "AMD Instinct MI300X HF"},

	/* Mi300 VF */
	{ CHIP_MI300X, 0x74B5, 0x00, "AMD Instinct MI300X VF"},
	{ CHIP_MI308X, 0x74B6, 0x00, "AMD Instinct MI308X VF"},
	{ CHIP_MI308X, 0x74BC, 0x00, "AMD Instinct MI308X VF"},
	{ CHIP_MI300X, 0x74BD, 0x00, "AMD Instinct MI300X HF"},

	/* Mi350 PF */
	{ CHIP_MI350X, 0x75A0, 0x00, "AMD Instinct MI350X"},
	{ CHIP_MI350X, 0x75A3, 0x00, "AMD Instinct MI355X"},

	/* Mi350 VF */
	{ CHIP_MI350X, 0x75B0, 0x00, "AMD Instinct MI350X VF"},
	{ CHIP_MI350X, 0x75B3, 0x00, "AMD Instinct MI355X VF"},

	/* Navi32 PF */
	{ CHIP_NAVI32, 0x7460, 0x00, "AMD Radeon PRO V710"},

	/* Navi32 VF */
	{ CHIP_NAVI32, 0x7461, 0x00, "AMD Radeon PRO V710"},
};

const char *amdgv_get_marketing_name(uint32_t dev_id, uint32_t rev_id)
{
	int i;
	struct amdgv_marketing_name_entry *entry;

	for (i = 0; i < ARRAY_SIZE(amdgv_marketing_name_table); i++) {
		entry = (struct amdgv_marketing_name_entry *)&amdgv_marketing_name_table[i];

		if (entry->dev_id == dev_id && entry->rev_id == rev_id)
			return entry->marketing_name;
	}

	return "UNKNOWN";
}
