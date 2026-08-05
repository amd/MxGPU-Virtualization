/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv.h"
#include "amdgv_device.h"
#include "mi300_powerplay.h"
#include "amdgv_ras_mp1_v13_0.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int mp1_v13_0_get_valid_bank_count(struct ras_core_context *ras_core,
					  u32 msg, u32 *count)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	int ret;

	if (!count)
		return AMDGV_FAILURE;

	ret = mi300_smu_send_msg(adapt, msg, count);
	if (ret) {
		*count = 0;
		return ret;
	}

	return 0;
}

static int mp1_v13_0_dump_valid_bank(struct ras_core_context *ras_core,
				     u32 msg, u32 idx, u32 reg_idx, u64 *val)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	uint32_t data[2] = {0, 0};
	uint32_t param;
	int ret;
	int i, offset;

	offset = reg_idx * 8;
	for (i = 0; i < ARRAY_SIZE(data); i++) {
		param = ((idx & 0xffff) << 16) | ((offset + (i << 2)) & 0xfffc);
		ret = mi300_smu_send_msg_with_param(adapt, msg, param, &data[i]);
		if (ret) {
			AMDGV_ERROR("ACA failed to read register[%d], offset:0x%x\n",
				reg_idx, offset);
			return ret;
		}
	}

	*val = (uint64_t)data[1] << 32 | data[0];

	return 0;
}

const struct ras_mp1_sys_func amdgv_ras_mp1_sys_func_v13_0 = {
	.mp1_get_valid_bank_count = mp1_v13_0_get_valid_bank_count,
	.mp1_dump_valid_bank = mp1_v13_0_dump_valid_bank,
};


