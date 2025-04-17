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

#include "amdgv_device.h"
#include "amdgv_dirtybit.h"
#include "amdgv_api.h"

static const uint32_t this_block = AMDGV_LIVE_MIGRATION_BLOCK;

int amdgv_dirtybit_control(struct amdgv_adapter *adapt, bool enable)
{
	int ret = 0;

	if (adapt->dirtybit.funcs &&
		adapt->dirtybit.funcs->control) {
		ret = adapt->dirtybit.funcs->control(adapt, enable);
	} else {
		AMDGV_ERROR("dirtybit_control is not properly defined.");
		ret = AMDGV_FAILURE;
	}

	return ret;
}

int amdgv_dirtybit_querydata(struct amdgv_adapter *adapt,
				struct amdgv_query_dirty_bit_data *data)
{
	int ret = 0;

	if (adapt->dirtybit.funcs &&
		adapt->dirtybit.funcs->query_data) {
		ret = adapt->dirtybit.funcs->query_data(adapt, data);
	} else {
		AMDGV_ERROR("query_data is not properly defined.");
		ret = AMDGV_FAILURE;
	}

	return ret;
}
