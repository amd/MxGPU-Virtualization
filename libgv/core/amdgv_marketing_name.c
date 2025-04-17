/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_asic.h"
#include "amdgv_marketing_name.h"

/*
 * DO NOT interpolate or extrapolate entries as
 * all entries must be official approved.
 */
static const struct amdgv_marketing_name_entry amdgv_marketing_name_table[] = {

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
