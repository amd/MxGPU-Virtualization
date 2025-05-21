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

#ifndef NAVI3_MMSCH_H
#define NAVI3_MMSCH_H

/* ASIC specified max VCN engine number */
#define NAVI3_MMSCH_MAX_VCN_ENGINE 2

/* ASIC specified max VF slot number */
#define NAVI3_MMSCH_MAX_VF_SLOT 16

#define NAVI3_MMSCH_VCN_BLOCK_ENCODE_DISABLE_BIT 0x80
#define NAVI3_MMSCH_VCN_BLOCK_DECODE_DISABLE_BIT 0x40

/* in libgv index */
#define NAVI3_MMSCH_VCN_BLOCK_ALLOWED_VF_ASSIGNMENT  0x0FFF
#define NAVI3_MMSCH_VCN1_BLOCK_ALLOWED_VF_ASSIGNMENT 0x0FFF

/* in libgv index */
#define NAVI3_MMSCH_RB_DECOUPLE_ENABLED_VF   0x0FFF
#define NAVI3_MMSCH_RB_DECOUPLE_AV1_DISABLED 0x0000

#pragma pack(push, 1)

struct navi32_mmsch_bandwidth_config {
	// Input
	union {
		struct {
			uint32_t time_partition_enable : 1;
			uint32_t job_limit_enable : 1;
			uint32_t reserved : 30;
		};
		uint32_t allbits;
	} flags;
	uint32_t vcn_enabled_vfs[NAVI3_MMSCH_MAX_VCN_ENGINE]; // bit0: PF, bit1: VF0, …
	struct amdgv_mmsch_vcn_vf_bandwidth vcn_vf_bandwidth[NAVI3_MMSCH_MAX_VCN_ENGINE][NAVI3_MMSCH_MAX_VF_SLOT]; // 0: PF, 1: VF0, …

	// Output
	struct amdgv_mmsch_vcn_vf_status vcn_vf_status[NAVI3_MMSCH_MAX_VCN_ENGINE][NAVI3_MMSCH_MAX_VF_SLOT]; // 0: PF, 1: VF0, …
	uint32_t output_gpu_timestamp_hi;
	uint32_t output_gpu_timestamp_lo;
};

#pragma pack(pop)

int navi32_mmsch_modify_vcn_ip_discovery_revison(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf,
				uint32_t vcn_engine_idx, uint8_t *revision);

#endif
