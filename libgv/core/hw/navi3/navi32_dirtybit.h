/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI32_DIRTY_BIT_H
#define NAVI32_DIRTY_BIT_H

#include "amdgv.h"

#define MAX_SEGMENT_INDEX 0x8000

#define MAX_MAM_INSTANCES_NAVI32 16
#define INVALID_MAM_INSTANCE 0xFF
#define SHIFT_64K 16
#define SHIFT_256K 18
#define SHIFT_512K 19
#define SHIFT_1M 20
#define SEGMENT_SIZE_1M 0x100000
#define SEGMENT_SIZE_1M_ALIGN(val)  ((val) & ~(SEGMENT_SIZE_1M - 1))

enum NAVI32_DBIT_TRACK_SEGMENT {
	NAVI32_DBIT_TRACK_SEGMENT_256kB = 0,    //256kB segment/8GB of FB
	NAVI32_DBIT_TRACK_SEGMENT_512kB = 1,    //512kB segment/16GB of FB
	NAVI32_DBIT_TRACK_SEGMENT_1MB = 2,      //1MB segment/32GB of FB
	NAVI32_DBIT_TRACK_SEGMENT_2MB = 3,      //2MB segment/64GB of FB
	NAVI32_DBIT_TRACK_SEGMENT_4MB = 4,      //4MB segment/128GB of FB
	NAVI32_DBIT_TRACK_SEGMENT_MAX
};

struct nv32_poll_dbit_write_mem {
	uint64_t query_addr;
	uint64_t gc_destination_addr;
	uint64_t mm_destination_addr;
	uint32_t number_of_pages;
	bool clear_dbit;
};

enum NV32_DBIT_QUERY{
	NV32_DBIT_QUERY_GC_MM = 0,
	NV32_DBIT_QUERY_GC    = 1,
	NV32_DBIT_QUERY_MM    = 2,
};

void navi32_select_mam_instance(struct amdgv_adapter *adapt, uint8_t mam_instance);
void navi32_dirtybit_gcea_sdp_control(struct amdgv_adapter *adapt,
						 bool gcea_sdp_enable);
int navi32_dirtybit_control(struct amdgv_adapter *adapt, bool enable);
void navi32_dirtybit_setup_sdma_hbm_page_size(struct amdgv_adapter *adapt);
int navi32_is_segment_dirty(struct amdgv_adapter *adapt, uint64_t segment, bool dbit_preserve,
				enum NV32_DBIT_QUERY query_type, bool *is_dirty);
int navi32_dirtybit_query_data(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data);
int navi32_dirtybit_sw_init(struct amdgv_adapter *adapt);
int navi32_dirtybit_sw_fini(struct amdgv_adapter *adapt);
int navi32_dirtybit_hw_init(struct amdgv_adapter *adapt);
int navi32_dirtybit_hw_fini(struct amdgv_adapter *adapt);

#endif
