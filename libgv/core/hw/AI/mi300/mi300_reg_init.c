/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <ai.h>
#include "mi300.h"

void mi300_reg_base_init(struct amdgv_adapter *adapt)
{
	/* Removed hard-codeing of IP Base Addresses (which are now obtained via IP
	 * Discovery instead) */
}

static int mi300_doorbell_index_init(struct amdgv_adapter *adapt)
{
	uint32_t sdma_id;
	adapt->doorbell_index.kiq = 0;
	adapt->doorbell_index.mec_ring0 = 8;
	adapt->doorbell_index.userqueue_start = 16;
	adapt->doorbell_index.userqueue_end = 31;
	adapt->doorbell_index.xcc_doorbell_range = 32;
	adapt->doorbell_index.first_non_cp = 256;
	adapt->doorbell_index.last_non_cp = 488;
	adapt->doorbell_index.max_assignment = 488 << 1;
	adapt->doorbell_index.sdma_doorbell_range = 20;

	for (sdma_id = 0; sdma_id < 16; sdma_id++)
		adapt->doorbell_index.sdma_engine[sdma_id] =
				AMDGV_MI300_DOORBELL_sDMA_ENGINE0 + sdma_id * 10;

	adapt->doorbell_index.ih = AMDGV_MI300_DOORBELL_IH;

	return 0;
}

/* do nothing in these interfaces */
static int mi300_doorbell_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_doorbell_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_doorbell_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi300_doorbell_func = {
	.name = "mi300_doorbell_func",
	.sw_init = mi300_doorbell_index_init,
	.sw_fini = mi300_doorbell_sw_fini,
	.hw_init = mi300_doorbell_hw_init,
	.hw_fini = mi300_doorbell_hw_fini,
};
