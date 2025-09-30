/*
 * Copyright 2024 Advanced Micro Devices, Inc.
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
 */

#ifndef __AMDGV_NPS_H__
#define __AMDGV_NPS_H__

#include <amdgv.h>
#include <amdgv_device.h>

#define AMDGV_VF_NPS_MAX_COMBINATIONS		4
#define AMDGV_NPS_COMPUTE_MAX_COMBINATIONS	12

struct amdgv_nps_compute_combination {
	enum amdgv_memory_partition_mode nps_mode;
	enum amdgv_accelerator_partition_mode compute_mode;
};

struct amdgv_vf_nps_combination {
	uint32_t vf_num;
	struct amdgv_nps_compute_combination combinations[AMDGV_NPS_COMPUTE_MAX_COMBINATIONS];
};

#endif
