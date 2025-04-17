/*
 * Copyright 2022 Advanced Micro Devices, Inc.
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
#ifndef __AMDGV_RING_H__
#define __AMDGV_RING_H__

#include "amdgv.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_wb_memory.h"

struct amdgv_adapter;
struct amdgv_ring;
struct amdgv_ib;
struct amdgv_live_info_ring;

/* max number of rings */
#define AMDGV_MAX_RINGS		28
#define AMDGV_MAX_HWIP_RINGS	8
#define AMDGV_MAX_GFX_RINGS		1
#define AMDGV_MAX_COMPUTE_RINGS	2
#define AMDGV_MAX_VCE_RINGS		3
#define AMDGV_MAX_UVD_ENC_RINGS	2
#define AMDGV_MAX_SDMA_RINGS 8

enum amdgv_ring_priority_level {
	AMDGV_RING_PRIO_0,
	AMDGV_RING_PRIO_1,
	AMDGV_RING_PRIO_DEFAULT = 1,
	AMDGV_RING_PRIO_2,
	AMDGV_RING_PRIO_MAX
};

/* AQL invalid packet header and data for ring initialization */
#define AMDGV_AQL_INVALID_PACKET_HEADER 0x1
#define AMDGV_AQL_INVALID_PACKET_DATA   0x0

/* Specify flags to be used for IB */

/* This IB should be submitted to CE */
#define AMDGV_IB_FLAG_CE	(1<<0)

/* Preamble flag, which means the IB could be dropped if no context switch */
#define AMDGV_IB_FLAG_PREAMBLE (1<<1)

/* Preempt flag, IB should set Pre_enb bit if PREEMPT flag detected */
#define AMDGV_IB_FLAG_PREEMPT (1<<2)

/* The IB fence should do the L2 writeback but not invalidate any shader
 * caches (L2/vL1/sL1/I$).
 */
#define AMDGV_IB_FLAG_TC_WB_NOT_INVALIDATE (1 << 3)

/* Set GDS_COMPUTE_MAX_WAVE_ID = DEFAULT before PACKET3_INDIRECT_BUFFER.
 * This will reset wave ID counters for the IB.
 */
#define AMDGV_IB_FLAG_RESET_GDS_MAX_WAVE_ID (1 << 4)

/* Flag the IB as secure (TMZ)
 */
#define AMDGV_IB_FLAGS_SECURE  (1 << 5)

/* Tell KMD to flush and invalidate caches
 */
#define AMDGV_IB_FLAG_EMIT_MEM_SYNC  (1 << 6)

/* bit set means command submit involves a preamble IB */
#define AMDGV_PREAMBLE_IB_PRESENT          (1 << 0)
/* bit set means preamble IB is first presented in belonging context */
#define AMDGV_PREAMBLE_IB_PRESENT_FIRST    (1 << 1)
/* bit set means context switch occurred */
#define AMDGV_HAVE_CTX_SWITCH              (1 << 2)
/* bit set means IB is preempted */
#define AMDGV_IB_PREEMPTED                 (1 << 3)

/* some special values for the owner field */
#define AMDGV_FENCE_OWNER_UNDEFINED	((void *)0ul)
#define AMDGV_FENCE_OWNER_VM		((void *)1ul)
#define AMDGV_FENCE_OWNER_KFD		((void *)2ul)

#define AMDGV_FENCE_FLAG_64BIT         (1 << 0)
#define AMDGV_FENCE_FLAG_INT           (1 << 1)
#define AMDGV_FENCE_FLAG_TC_WB_ONLY    (1 << 2)

enum amdgv_ring_type {
	AMDGV_RING_TYPE_GFX		= 0,
	AMDGV_RING_TYPE_COMPUTE	= 1,
	AMDGV_RING_TYPE_SDMA	= 2,
	AMDGV_RING_TYPE_KIQ     = 3,
	AMDGV_RING_TYPE_MES     = 4
};

enum amdgv_ib_pool_type {
	/* Normal submissions to the top of the pipeline. */
	AMDGV_IB_POOL_DELAYED,
	/* Immediate submissions to the bottom of the pipeline. */
	AMDGV_IB_POOL_IMMEDIATE,
	/* Direct submission to the ring buffer during init and reset. */
	AMDGV_IB_POOL_DIRECT,

	AMDGV_IB_POOL_MAX
};

struct amdgv_ib {
	struct amdgv_memmgr_mem *sa_bo;
	uint32_t			length_dw;
	uint64_t			gpu_addr;
	uint32_t			*ptr;
	uint32_t			flags;
};

/*
 * Fences.
 */
struct amdgv_fence_driver {
	uint64_t			gpu_addr;
	volatile uint32_t		*cpu_addr;
	/* sync_seq is protected by ring emission lock */
	uint32_t			sync_seq;
	atomic_t			last_seq;
	bool				initialized;
	unsigned int			irq_type;
	unsigned int			num_fences_mask;
	spin_lock_t			lock;
};

int amdgv_fence_emit_polling(struct amdgv_ring *ring, uint32_t *s,
			      uint32_t timeout);
signed long amdgv_fence_wait_polling(struct amdgv_ring *ring,
				      uint32_t wait_seq,
				      signed long timeout);

/*
 * Rings.
 */

/* provided by hw blocks that expose a ring buffer for commands */
struct amdgv_ring_funcs {
	enum amdgv_ring_type	type;
	uint32_t		align_mask;
	uint32_t		aql_align_mask;
	uint32_t		nop;
	uint32_t		aql_nop;
	bool			support_64bit_ptrs;
	bool			no_user_fence;
	bool			secure_submission_supported;
	unsigned int		vmhub;
	unsigned int		extra_dw;

	/* ring read/write ptr handling */
	uint64_t (*get_rptr)(struct amdgv_ring *ring);
	uint64_t (*get_wptr)(struct amdgv_ring *ring);
	void (*set_wptr)(struct amdgv_ring *ring);
	/* constants to calculate how many DW are needed for an emit */
	unsigned int emit_frame_size;
	unsigned int emit_ib_size;
	/* command emit functions */
	void (*emit_ib)(struct amdgv_ring *ring,
			struct amdgv_ib *ib,
			uint32_t flags);
	void (*emit_fence)(struct amdgv_ring *ring, uint64_t addr,
			   uint64_t seq, unsigned int flags);
	void (*emit_pipeline_sync)(struct amdgv_ring *ring);
	void (*emit_vm_flush)(struct amdgv_ring *ring, unsigned int vmid,
			      uint64_t pd_addr);
	void (*emit_hdp_flush)(struct amdgv_ring *ring);
	void (*emit_gds_switch)(struct amdgv_ring *ring, uint32_t vmid,
				uint32_t gds_base, uint32_t gds_size,
				uint32_t gws_base, uint32_t gws_size,
				uint32_t oa_base, uint32_t oa_size);
	/* testing functions */
	int (*test_ring)(struct amdgv_ring *ring);
	int (*test_ib)(struct amdgv_ring *ring, long timeout);
	/* insert NOP packets */
	void (*insert_nop)(struct amdgv_ring *ring, uint32_t count);
	void (*insert_start)(struct amdgv_ring *ring);
	void (*insert_end)(struct amdgv_ring *ring);
	/* pad the indirect buffer to the necessary number of dw */
	void (*pad_ib)(struct amdgv_ring *ring, struct amdgv_ib *ib);
	unsigned int (*init_cond_exec)(struct amdgv_ring *ring);
	void (*patch_cond_exec)(struct amdgv_ring *ring, unsigned int offset);
	/* note usage for clock and power gating */
	void (*begin_use)(struct amdgv_ring *ring);
	void (*end_use)(struct amdgv_ring *ring);
	void (*emit_switch_buffer)(struct amdgv_ring *ring);
	void (*emit_cntxcntl)(struct amdgv_ring *ring, uint32_t flags);
	void (*emit_rreg)(struct amdgv_ring *ring, uint32_t reg,
			  uint32_t reg_val_offs);
	void (*emit_wreg)(struct amdgv_ring *ring, uint32_t reg, uint32_t val);
	void (*emit_reg_wait)(struct amdgv_ring *ring, uint32_t reg,
			      uint32_t val, uint32_t mask);
	void (*emit_reg_write_reg_wait)(struct amdgv_ring *ring,
					uint32_t reg0, uint32_t reg1,
					uint32_t ref, uint32_t mask);
	void (*emit_frame_cntl)(struct amdgv_ring *ring, bool start,
				bool secure);
	/* Try to soft recover the ring to make the fence signal */
	void (*soft_recovery)(struct amdgv_ring *ring, unsigned int vmid);
	int (*preempt_ib)(struct amdgv_ring *ring);
	void (*emit_mem_sync)(struct amdgv_ring *ring);
	void (*emit_wave_limit)(struct amdgv_ring *ring, bool enable);
	void (*submit_frame)(struct amdgv_ring *ring, uint8_t *frame_data);
};

struct amdgv_ring {
	struct amdgv_adapter *adapt;
	const struct amdgv_ring_funcs	*funcs;
	struct amdgv_fence_driver	fence_drv;

	struct amdgv_memmgr_mem	*ring_obj;
	volatile uint32_t	*ring;
	uint32_t			rptr_offs;
	uint64_t			rptr_gpu_addr;
	volatile uint32_t	*rptr_cpu_addr;
	uint64_t			wptr; 		// in DWORD
	uint64_t			wptr_old;	// in DWORD
	uint32_t			ring_size; 	// in DWORD
	uint32_t			log2_ring_size; // in DWORD
	uint32_t			max_dw;
	int					count_dw;
	uint64_t			gpu_addr;
	uint64_t			ptr_mask;
	uint32_t			buf_mask;
	uint32_t			idx;
	uint32_t			xcc_id;
	uint32_t			me;
	uint32_t			pipe;
	uint32_t			queue;
	struct amdgv_memmgr_mem *mqd_obj;
	uint64_t            mqd_gpu_addr;
	void                *mqd_ptr;
	uint64_t            eop_gpu_addr;
	uint32_t			doorbell_index;
	bool				use_doorbell;
	bool				use_pollmem;
	uint32_t			wptr_offs;
	uint64_t			wptr_gpu_addr;
	volatile uint32_t	*wptr_cpu_addr;
	uint32_t			fence_offs;
	uint64_t			fence_gpu_addr;
	volatile uint32_t	*fence_cpu_addr;
	uint64_t			current_ctx;
	char				name[16];
	uint32_t            trail_seq;
	uint32_t			trail_fence_offs;
	uint64_t			trail_fence_gpu_addr;
	volatile uint32_t	*trail_fence_cpu_addr;
	uint32_t			cond_exe_offs;
	uint64_t			cond_exe_gpu_addr;
	volatile uint32_t	*cond_exe_cpu_addr;
	uint32_t			vm_inv_eng;
	struct dma_fence	*vmid_wait;
	bool				has_compute_vm_bug;
	bool				no_scheduler;
	int					hw_prio;
	uint32_t			num_hw_submission;
	atomic_t			*sched_score;
	bool		        aql_enable;
	/* used for mes */
	bool			is_mes_queue;
	uint32_t		hw_queue_id;
};

#define amdgv_ring_parse_cs(r, p, job, ib) \
		((r)->funcs->parse_cs((p), (job), (ib)))
#define amdgv_ring_patch_cs_in_place(r, p, job, ib) \
		((r)->funcs->patch_cs_in_place((p), (job), (ib)))
#define amdgv_ring_test_ring(r) ((r)->funcs->test_ring((r)))
#define amdgv_ring_test_ib(r, t) ((r)->funcs->test_ib((r), (t)))
#define amdgv_ring_get_rptr(r) ((r)->funcs->get_rptr((r)))
#define amdgv_ring_get_wptr(r) ((r)->funcs->get_wptr((r)))
#define amdgv_ring_set_wptr(r) ((r)->funcs->set_wptr((r)))
#define amdgv_ring_emit_ib(r, ib, flags) \
		((r)->funcs->emit_ib((r), (ib), (flags)))
#define amdgv_ring_emit_pipeline_sync(r) ((r)->funcs->emit_pipeline_sync((r)))
#define amdgv_ring_emit_vm_flush(r, vmid, addr) \
		((r)->funcs->emit_vm_flush((r), (vmid), (addr)))
#define amdgv_ring_emit_fence(r, addr, seq, flags) \
		((r)->funcs->emit_fence((r), (addr), (seq), (flags)))
#define amdgv_ring_emit_gds_switch(r, v, db, ds, wb, ws, ab, as) \
		((r)->funcs->emit_gds_switch((r), (v), (db), (ds), (wb), (ws), \
		(ab), (as)))
#define amdgv_ring_emit_hdp_flush(r) ((r)->funcs->emit_hdp_flush((r)))
#define amdgv_ring_emit_switch_buffer(r) ((r)->funcs->emit_switch_buffer((r)))
#define amdgv_ring_emit_cntxcntl(r, d) ((r)->funcs->emit_cntxcntl((r), (d)))
#define amdgv_ring_emit_rreg(r, d, o) ((r)->funcs->emit_rreg((r), (d), (o)))
#define amdgv_ring_emit_wreg(r, d, v) ((r)->funcs->emit_wreg((r), (d), (v)))
#define amdgv_ring_emit_reg_wait(r, d, v, m) \
		((r)->funcs->emit_reg_wait((r), (d), (v), (m)))
#define amdgv_ring_emit_reg_write_reg_wait(r, d0, d1, v, m) \
		((r)->funcs->emit_reg_write_reg_wait((r), (d0), (d1), (v), (m)))
#define amdgv_ring_emit_frame_cntl(r, b, s) \
		((r)->funcs->emit_frame_cntl((r), (b), (s)))
#define amdgv_ring_pad_ib(r, ib) ((r)->funcs->pad_ib((r), (ib)))
#define amdgv_ring_init_cond_exec(r) ((r)->funcs->init_cond_exec((r)))
#define amdgv_ring_patch_cond_exec(r, o) \
		((r)->funcs->patch_cond_exec((r), (o)))
#define amdgv_ring_preempt_ib(r) ((r)->funcs->preempt_ib(r))

int amdgv_ring_alloc(struct amdgv_ring *ring, unsigned int ndw);
void amdgv_ring_insert_nop(struct amdgv_ring *ring, uint32_t count);
void amdgv_ring_commit(struct amdgv_ring *ring);
void amdgv_ring_undo(struct amdgv_ring *ring);
int amdgv_ring_init(struct amdgv_adapter *adapt, struct amdgv_ring *ring,
		uint32_t max_dw, uint32_t frames, uint32_t hw_prio,
		atomic_t *sched_score, uint32_t mem_id);
void amdgv_ring_fini(struct amdgv_ring *ring);
int amdgv_ring_test_helper(struct amdgv_ring *ring);

int amdgv_ring_init_set(struct amdgv_adapter *adapt, struct amdgv_ring *ring);
void amdgv_ring_write(struct amdgv_ring *ring, uint32_t v);
void amdgv_ring_clear_ring(struct amdgv_ring *ring);

int amdgv_ib_get(struct amdgv_adapter *adapt,
		  unsigned int size,
		  enum amdgv_ib_pool_type pool,
		  struct amdgv_ib *ib);
void amdgv_ib_free(struct amdgv_adapter *adapt, struct amdgv_ib *ib);
int amdgv_ib_schedule(struct amdgv_ring *ring, unsigned int num_ibs,
		       struct amdgv_ib *ibs);

enum amdgv_live_info_status amdgv_ring_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ring *ring_info);
enum amdgv_live_info_status amdgv_ring_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_ring *ring_info);
#endif
