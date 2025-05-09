/*
 * Copyright (C) 2021  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MI300_RESET_H
#define MI300_RESET_H

struct engine_reg_info {
	uint32_t hwip;
	uint32_t offset;
	uint32_t seg;
	uint32_t reg_shift;
	uint32_t reg_mask;
	uint32_t clean_cond;
};

enum engine_reg {
	RLC_STAT,
	SDMA_STATUS_REG,
	SDMA_STATUS4_REG,
	GRBM_STATUS,
	NUM_ENGINES
};

int mi300_reset_trigger_whole_gpu_reset(struct amdgv_adapter *adapt);
void mi300_clear_dummy_mode_after_reset(struct amdgv_adapter *adapt);

#endif
