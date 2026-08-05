/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __GFX_V9_4_3_H__
#define __GFX_V9_4_3_H__

#include "amdgv.h"
#include "amdgv_device.h"

#define XCC_TO_DIE(_XCC_) (((_XCC_) & 0x1) ? 1 : 0)

#define GFX9_MEC_HPD_SIZE 4096
#define CP_HQD_PERSISTENT_STATE_DEFAULT 0xbe05301

#define AQL_QUEUE_RING_DWORDS	256
#define AQL_QUEUE_RING_LOG2	8

struct gfx_v9_4_3_aql_queue {
	struct amdgv_memmgr_mem	*ring_buf;
	struct amdgv_memmgr_mem	*eop_obj;
	struct amdgv_memmgr_mem	*wb_obj;
	struct amdgv_memmgr_mem	*mqd_obj;

	uint64_t		ring_buf_gpu;
	volatile uint32_t	*ring_buf_cpu;

	uint64_t		eop_gpu;

	uint64_t		wptr_gpu;
	volatile uint64_t	*wptr_cpu;
	uint64_t		rptr_gpu;
	volatile uint32_t	*rptr_cpu;

	uint64_t		mqd_gpu[AMDGV_MAX_GC_INSTANCES];
	void			*mqd_cpu[AMDGV_MAX_GC_INSTANCES];
	uint32_t		doorbell_index[AMDGV_MAX_GC_INSTANCES];
	bool			mapped[AMDGV_MAX_GC_INSTANCES];

	uint32_t		*hqd_save;

	uint32_t		num_xcc;
};

void gfx_v9_4_3_set_funcs(struct amdgv_adapter *adapt);

void gfx_v9_4_3_dirtybit_control(struct amdgv_adapter *adapt, bool enable);

int gfx_v9_4_3_aql_queue_init(struct amdgv_adapter *adapt,
	struct gfx_v9_4_3_aql_queue *aq, uint32_t num_xcc);
void gfx_v9_4_3_aql_queue_fini(struct gfx_v9_4_3_aql_queue *aq);
int gfx_v9_4_3_aql_queue_build_mqd(struct amdgv_adapter *adapt,
	struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc);
int gfx_v9_4_3_aql_queue_kiq_map_xcc(struct amdgv_adapter *adapt,
		struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc);
int gfx_v9_4_3_aql_queue_kiq_unmap_xcc(struct amdgv_adapter *adapt,
			struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc);
void gfx_v9_4_3_aql_queue_save_hqd(struct amdgv_adapter *adapt,
			struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc);
void gfx_v9_4_3_aql_queue_restore_hqd(struct amdgv_adapter *adapt,
			struct gfx_v9_4_3_aql_queue *aq, uint32_t xcc);
int gfx_v9_4_3_aql_queue_submit_packet_data(struct amdgv_adapter *adapt,
					    struct gfx_v9_4_3_aql_queue *aq,
					    const uint32_t *pkt_data);
int gfx_v9_4_3_fb_hash_compute_page_hash(struct amdgv_adapter *adapt, uint32_t idx_vf,
					 uint64_t page_size,
					 struct amdgv_memmgr_mem *fb_hash_buf);
#endif
