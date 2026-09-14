// SPDX-License-Identifier: MIT
/*
 * Copyright 2026 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "ras.h"
#include "amdgv_ras_cper.h"
#include "amdgv_ras_mgr.h"
int amdgv_ras_cper_sw_init(struct amdgv_adapter *adapt)
{

	return 0;
}

int amdgv_ras_cper_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

int amdgv_ras_mgr_add_driver_event(struct ras_core_context *ras_core,
		uint64_t event_u64, const void *ctx, uint32_t ctx_size)
{
	struct drv_event_context_wrapper wrapper;

	if (ctx_size > sizeof(wrapper.ctx)) {
		RAS_DEV_ERR(ras_core->dev,
			"driver event ctx size %u exceeds wrapper capacity %zu\n",
			ctx_size, sizeof(wrapper.ctx));
		return -RAS_CORE_EINVAL;
	}

	oss_memset(&wrapper, 0, sizeof(wrapper));
	wrapper.event_u64 = event_u64;
	if (ctx && ctx_size)
		oss_memcpy(&wrapper.ctx, ctx, ctx_size);

	return ras_core_add_log_event(ras_core, RAS_LOG_EVENT_DRIVER_EVENT,
				   &wrapper, sizeof(wrapper));
}
