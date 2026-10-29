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

#ifndef _MMSCH_V10_0_H_
#define _MMSCH_V10_0_H_

#define MMSCH_V10_0_VCN_PER_MID 2

void mmsch_v10_0_get_regid_instid(struct amdgv_adapter *adapt, uint32_t hw_sched_id, uint64_t *reg, bool control);
int mmsch_v10_0_set_doorbell_addr_base(struct amdgv_adapter *adapt);

void mmsch_v10_0_get_gpuiov_reg_by_base(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					uint64_t *reg, uint32_t reg0, uint32_t reg1,
					uint32_t reg2, uint32_t base);
void mmsch_v10_0_get_time_quanta_index_reg(struct amdgv_adapter *adapt, uint32_t hw_sched_id, uint32_t idx_vf, uint64_t *reg);

/* Resolve an MMSCH GPUIOV register for the multimedia group owning hw_sched_id. */
#define mmsch_v10_0_get_gpuiov_reg(adapt, hw_sched_id, reg, name) \
	mmsch_v10_0_get_gpuiov_reg_by_base((adapt), (hw_sched_id), (reg), \
		name##_0, name##_1, name##_2, name##_0_BASE_IDX)

#endif // _MMSCH_V10_0_H_
