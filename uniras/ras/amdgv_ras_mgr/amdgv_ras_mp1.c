/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amdgv_ras_mp1.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static const enum pp_smu_ras_msg ras_smu_ppm_msg_maps[RAS_MP1_MSG_MAX] = {
	[RAS_MP1_MSG_GetRasTableVersion] = PP_SMU_RAS_MSG_GetRasTableVersion,
	[RAS_MP1_MSG_GetRmaStatus] = PP_SMU_RAS_MSG_GetRmaStatus,
	[RAS_MP1_MSG_GetBadPageCount] = PP_SMU_RAS_MSG_GetBadPageCount,
	[RAS_MP1_MSG_GetBadPageMcaAddr] = PP_SMU_RAS_MSG_GetBadPageMcaAddr,
	[RAS_MP1_MSG_GetBadPagePaAddr] = PP_SMU_RAS_MSG_GetBadPagePaAddr,
	[RAS_MP1_MSG_SetTimestamp] = PP_SMU_RAS_MSG_SetTimestamp,
	[RAS_MP1_MSG_GetTimestamp] = PP_SMU_RAS_MSG_GetTimestamp,
	[RAS_MP1_MSG_GetRasPolicy] = PP_SMU_RAS_MSG_GetRasPolicy,
	[RAS_MP1_MSG_GetBadPageIpId] = PP_SMU_RAS_MSG_GetBadPageIpId,
	[RAS_MP1_MSG_EraseRasTable] = PP_SMU_RAS_MSG_EraseRasTable,
};

static enum pp_smu_ras_msg
	__get_ras_smu_ppm_msg(struct ras_core_context *ras_core, u32 msg_id)
{
	if (msg_id >= RAS_MP1_MSG_MAX)
		return 0;

	return ras_smu_ppm_msg_maps[msg_id];
}

static int amdgv_ras_send_mp1_msg(struct ras_core_context *ras_core, u32 msg_id,
		u32 *params, u32 num_params, u32 *read_args, u32 num_read_args)
{
	struct amdgv_adapter *adapt = ras_core->dev;
	enum pp_smu_ras_msg smu_msg;

	smu_msg = __get_ras_smu_ppm_msg(ras_core, msg_id);
	if (!smu_msg)
		return -RAS_CORE_EOPNOTSUPP;

	return amdgv_smu_send_ras_msg(adapt, smu_msg,
			params, num_params, read_args, num_read_args);
}

const struct ras_mp1_sys_func amdgv_ras_mp1_sys_func = {
	.mp1_send_ras_msg = amdgv_ras_send_mp1_msg,
};

