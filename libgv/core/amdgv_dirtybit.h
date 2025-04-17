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

#ifndef AMDGV_DIRTYBIT_H
#define AMDGV_DIRTYBIT_H

#include "amdgv.h"

struct amdgv_dirtybit {
	const struct amdgv_dirtybit_funcs *funcs;
	uint8_t *query_submission_frame;
	struct amdgv_memmgr_mem *gc_dirty_bitplane;
	struct amdgv_memmgr_mem *mm_dirty_bitplane;
};

struct amdgv_dirtybit_funcs {
	int (*control)(struct amdgv_adapter *adapt, bool enable);
	int (*query_data)(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data);
};

int amdgv_dirtybit_control(struct amdgv_adapter *adapt, bool enable);
int amdgv_dirtybit_querydata(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data);

#endif
