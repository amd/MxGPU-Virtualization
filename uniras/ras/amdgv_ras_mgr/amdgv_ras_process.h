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

#ifndef __AMDGV_RAS_PROCESS_H__
#define __AMDGV_RAS_PROCESS_H__
#include "ras_process.h"
#include "amdgv_ras_mgr.h"

enum ras_ih_type {
	RAS_IH_NONE,
	RAS_IH_FROM_BLOCK_CONTROLLER,
	RAS_IH_FROM_CONSUMER_CLIENT,
	RAS_IH_FROM_FATAL_ERROR,
};
int amdgv_ras_process_init(struct amdgv_adapter *adapt);
int amdgv_ras_process_fini(struct amdgv_adapter *adapt);
int amdgv_ras_process_handle_umc_interrupt(struct amdgv_adapter *adapt,
		uint32_t idx_vf, void *data);
int amdgv_ras_process_handle_consumption_interrupt(struct amdgv_adapter *adapt,
		uint32_t idx_vf, void *data);
int amdgv_ras_process_ras_event_dispatch(struct amdgv_adapter *adapt,
		uint32_t idx_vf);
#endif
