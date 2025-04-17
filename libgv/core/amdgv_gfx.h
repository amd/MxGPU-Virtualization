/*
 * Copyright (C) 2022 Advanced Micro Devices, Inc.
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
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef __AMDGV_GFX_H__
#define __AMDGV_GFX_H__

#include "amdgv_ras.h"
#include "amdgv_ring.h"

#define MAX_KIQ_REG_WAIT       5000 /* in usecs, 5ms */
#define MAX_KIQ_REG_BAILOUT_INTERVAL   5 /* in msecs, 5ms */
#define MAX_KIQ_REG_TRY 1000

#define AMDGV_GFX_MAX_USEC_TIMEOUT		1000	/* 1 ms */

/* GFX current status */
#define AMDGV_GFX_NORMAL_MODE			0x00000000L
#define AMDGV_GFX_SAFE_MODE				0x00000001L
#define AMDGV_GFX_PG_DISABLED_MODE		0x00000002L
#define AMDGV_GFX_CG_DISABLED_MODE		0x00000004L
#define AMDGV_GFX_LBPW_DISABLED_MODE	0x00000008L

#define KGD_MAX_QUEUES 64
#define AMDGV_MAX_GFX_QUEUES KGD_MAX_QUEUES
#define AMDGV_MAX_COMPUTE_QUEUES KGD_MAX_QUEUES

#define PAGE_SIZE 0x1000
#define AMDGV_GFX_ALIGN(data, align) (((data) + (align)-1) & ~(align - 1))

#define AQL_COMP_RING_MAX_DWORD 1024

/*
 * PM4
 */
#define	PACKET_TYPE0	0
#define	PACKET_TYPE1	1
#define	PACKET_TYPE2	2
#define	PACKET_TYPE3	3

#define CP_PACKET_GET_TYPE(h) (((h) >> 30) & 3)
#define CP_PACKET_GET_COUNT(h) (((h) >> 16) & 0x3FFF)
#define CP_PACKET0_GET_REG(h) ((h) & 0xFFFF)
#define CP_PACKET3_GET_OPCODE(h) (((h) >> 8) & 0xFF)
#define PACKET0(reg, n)	((PACKET_TYPE0 << 30) |				\
			 ((reg) & 0xFFFF) |			\
			 ((n) & 0x3FFF) << 16)
#define CP_PACKET2			0x80000000
#define		PACKET2_PAD_SHIFT		0
#define		PACKET2_PAD_MASK		(0x3fffffff << 0)

#define PACKET2(v)	(CP_PACKET2 | REG_SET(PACKET2_PAD, (v)))

#define PACKET3(op, n)	((PACKET_TYPE3 << 30) |				\
			 (((op) & 0xFF) << 8) |				\
			 ((n) & 0x3FFF) << 16)

#define PACKET3_COMPUTE(op, n) (PACKET3(op, n) | 1 << 1)

/* Packet 3 types */
#define	PACKET3_NOP					0x10
#define	PACKET3_SET_BASE				0x11
#define		PACKET3_BASE_INDEX(x)                  ((x) << 0)
#define			CE_PARTITION_BASE		3
#define	PACKET3_CLEAR_STATE				0x12
#define	PACKET3_INDEX_BUFFER_SIZE			0x13
#define	PACKET3_DISPATCH_DIRECT				0x15
#define	PACKET3_DISPATCH_INDIRECT			0x16
#define	PACKET3_ATOMIC_GDS				0x1D
#define	PACKET3_ATOMIC_MEM				0x1E
#define	PACKET3_OCCLUSION_QUERY				0x1F
#define	PACKET3_SET_PREDICATION				0x20
#define	PACKET3_REG_RMW					0x21
#define	PACKET3_COND_EXEC				0x22
#define	PACKET3_PRED_EXEC				0x23
#define	PACKET3_DRAW_INDIRECT				0x24
#define	PACKET3_DRAW_INDEX_INDIRECT			0x25
#define	PACKET3_INDEX_BASE				0x26
#define	PACKET3_DRAW_INDEX_2				0x27
#define	PACKET3_CONTEXT_CONTROL				0x28
#define	PACKET3_INDEX_TYPE				0x2A
#define	PACKET3_DRAW_INDIRECT_MULTI			0x2C
#define	PACKET3_DRAW_INDEX_AUTO				0x2D
#define	PACKET3_NUM_INSTANCES				0x2F
#define	PACKET3_DRAW_INDEX_MULTI_AUTO			0x30
#define	PACKET3_INDIRECT_BUFFER_CONST			0x33
#define	PACKET3_STRMOUT_BUFFER_UPDATE			0x34
#define	PACKET3_DRAW_INDEX_OFFSET_2			0x35
#define	PACKET3_DRAW_PREAMBLE				0x36
#define	PACKET3_WRITE_DATA				0x37
#define		WRITE_DATA_DST_SEL(x)                   ((x) << 8)
		/* 0 - register
		 * 1 - memory (sync - via GRBM)
		 * 2 - gl2
		 * 3 - gds
		 * 4 - reserved
		 * 5 - memory (async - direct)
		 */
#define		WR_ONE_ADDR                             (1 << 16)
#define		WR_CONFIRM                              (1 << 20)
#define		WRITE_DATA_CACHE_POLICY(x)              ((x) << 25)
		/* 0 - LRU
		 * 1 - Stream
		 */
#define		WRITE_DATA_ENGINE_SEL(x)                ((x) << 30)
		/* 0 - me
		 * 1 - pfp
		 * 2 - ce
		 */
#define	PACKET3_DRAW_INDEX_INDIRECT_MULTI		0x38
#define	PACKET3_MEM_SEMAPHORE				0x39
#define PACKET3_SEM_USE_MAILBOX       (0x1 << 16)
/* 0 = increment, 1 = write 1 */
#define PACKET3_SEM_SEL_SIGNAL_TYPE   (0x1 << 20)
#define PACKET3_SEM_SEL_SIGNAL	    (0x6 << 29)
#define PACKET3_SEM_SEL_WAIT	    (0x7 << 29)
#define	PACKET3_WAIT_REG_MEM				0x3C
#define		WAIT_REG_MEM_FUNCTION(x)                ((x) << 0)
		/* 0 - always
		 * 1 - <
		 * 2 - <=
		 * 3 - ==
		 * 4 - !=
		 * 5 - >=
		 * 6 - >
		 */
#define		WAIT_REG_MEM_MEM_SPACE(x)               ((x) << 4)
		/* 0 - reg
		 * 1 - mem
		 */
#define		WAIT_REG_MEM_OPERATION(x)               ((x) << 6)
		/* 0 - wait_reg_mem
		 * 1 - wr_wait_wr_reg
		 */
#define		WAIT_REG_MEM_ENGINE(x)                  ((x) << 8)
		/* 0 - me
		 * 1 - pfp
		 */
#define	PACKET3_INDIRECT_BUFFER				0x3F
#define		INDIRECT_BUFFER_VALID                   (1 << 23)
#define		INDIRECT_BUFFER_CACHE_POLICY(x)         ((x) << 28)
		/* 0 - LRU
		 * 1 - Stream
		 * 2 - Bypass
		 */
#define     INDIRECT_BUFFER_PRE_ENB(x)		 ((x) << 21)
#define	PACKET3_COPY_DATA				0x40
#define	PACKET3_PFP_SYNC_ME				0x42
#define	PACKET3_COND_WRITE				0x45
#define	PACKET3_EVENT_WRITE				0x46
#define		EVENT_TYPE(x)                           ((x) << 0)
#define		EVENT_INDEX(x)                          ((x) << 8)
		/* 0 - any non-TS event
		 * 1 - ZPASS_DONE, PIXEL_PIPE_STAT_*
		 * 2 - SAMPLE_PIPELINESTAT
		 * 3 - SAMPLE_STREAMOUTSTAT*
		 * 4 - *S_PARTIAL_FLUSH
		 */
#define	PACKET3_RELEASE_MEM				0x49
#define		PACKET3_RELEASE_MEM_EVENT_TYPE(x)	((x) << 0)
#define		PACKET3_RELEASE_MEM_EVENT_INDEX(x)	((x) << 8)
#define		PACKET3_RELEASE_MEM_GCR_GLM_WB		(1 << 12)
#define		PACKET3_RELEASE_MEM_GCR_GLM_INV		(1 << 13)
#define		PACKET3_RELEASE_MEM_GCR_GLV_INV		(1 << 14)
#define		PACKET3_RELEASE_MEM_GCR_GL1_INV		(1 << 15)
#define		PACKET3_RELEASE_MEM_GCR_GL2_US		(1 << 16)
#define		PACKET3_RELEASE_MEM_GCR_GL2_RANGE	(1 << 17)
#define		PACKET3_RELEASE_MEM_GCR_GL2_DISCARD	(1 << 19)
#define		PACKET3_RELEASE_MEM_GCR_GL2_INV		(1 << 20)
#define		PACKET3_RELEASE_MEM_GCR_GL2_WB		(1 << 21)
#define		PACKET3_RELEASE_MEM_GCR_SEQ		(1 << 22)
#define		PACKET3_RELEASE_MEM_CACHE_POLICY(x)	((x) << 25)
		/* 0 - cache_policy__me_release_mem__lru
		 * 1 - cache_policy__me_release_mem__stream
		 * 2 - cache_policy__me_release_mem__noa
		 * 3 - cache_policy__me_release_mem__bypass
		 */
#define		PACKET3_RELEASE_MEM_EXECUTE		(1 << 28)

#define		PACKET3_RELEASE_MEM_DATA_SEL(x)		((x) << 29)
		/* 0 - discard
		 * 1 - send low 32bit data
		 * 2 - send 64bit data
		 * 3 - send 64bit GPU counter value
		 * 4 - send 64bit sys counter value
		 */
#define		PACKET3_RELEASE_MEM_INT_SEL(x)		((x) << 24)
		/* 0 - none
		 * 1 - interrupt only (DATA_SEL = 0)
		 * 2 - interrupt when data write is confirmed
		 */
#define		PACKET3_RELEASE_MEM_DST_SEL(x)		((x) << 16)
		/* 0 - MC
		 * 1 - TC/L2
		 */
#define		EVENT_TYPE(x)                           ((x) << 0)
#define		EVENT_INDEX(x)                          ((x) << 8)
#define		EOP_TCL1_VOL_ACTION_EN                  (1 << 12)
#define		EOP_TC_VOL_ACTION_EN                    (1 << 13) /* L2 */
#define		EOP_TC_WB_ACTION_EN                     (1 << 15) /* L2 */
#define		EOP_TCL1_ACTION_EN                      (1 << 16)
#define		EOP_TC_ACTION_EN                        (1 << 17) /* L2 */
#define		EOP_TC_NC_ACTION_EN	(1 << 19)
#define		EOP_TC_MD_ACTION_EN	(1 << 21) /* L2 metadata */

#define		DATA_SEL(x)                             ((x) << 29)
		/* 0 - discard
		 * 1 - send low 32bit data
		 * 2 - send 64bit data
		 * 3 - send 64bit GPU counter value
		 * 4 - send 64bit sys counter value
		 */
#define		INT_SEL(x)                              ((x) << 24)
		/* 0 - none
		 * 1 - interrupt only (DATA_SEL = 0)
		 * 2 - interrupt when data write is confirmed
		 */
#define		DST_SEL(x)                              ((x) << 16)
		/* 0 - MC
		 * 1 - TC/L2
		 */

#define	PACKET3_PREAMBLE_CNTL				0x4A
#              define PACKET3_PREAMBLE_BEGIN_CLEAR_STATE     (2 << 28)
#              define PACKET3_PREAMBLE_END_CLEAR_STATE       (3 << 28)
#define	PACKET3_DMA_DATA				0x50
/* 1. header
 * 2. CONTROL
 * 3. SRC_ADDR_LO or DATA [31:0]
 * 4. SRC_ADDR_HI [31:0]
 * 5. DST_ADDR_LO [31:0]
 * 6. DST_ADDR_HI [7:0]
 * 7. COMMAND [30:21] | BYTE_COUNT [20:0]
 */
/* CONTROL */
#              define PACKET3_DMA_DATA_ENGINE(x)     ((x) << 0)
		/* 0 - ME
		 * 1 - PFP
		 */
#              define PACKET3_DMA_DATA_SRC_CACHE_POLICY(x) ((x) << 13)
		/* 0 - LRU
		 * 1 - Stream
		 */
#              define PACKET3_DMA_DATA_DST_SEL(x)  ((x) << 20)
		/* 0 - DST_ADDR using DAS
		 * 1 - GDS
		 * 3 - DST_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_DST_CACHE_POLICY(x) ((x) << 25)
		/* 0 - LRU
		 * 1 - Stream
		 */
#              define PACKET3_DMA_DATA_SRC_SEL(x)  ((x) << 29)
		/* 0 - SRC_ADDR using SAS
		 * 1 - GDS
		 * 2 - DATA
		 * 3 - SRC_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_CP_SYNC     (1 << 31)
/* COMMAND */
#              define PACKET3_DMA_DATA_CMD_SAS     (1 << 26)
		/* 0 - memory
		 * 1 - register
		 */
#              define PACKET3_DMA_DATA_CMD_DAS     (1 << 27)
		/* 0 - memory
		 * 1 - register
		 */
#              define PACKET3_DMA_DATA_CMD_SAIC    (1 << 28)
#              define PACKET3_DMA_DATA_CMD_DAIC    (1 << 29)
#              define PACKET3_DMA_DATA_CMD_RAW_WAIT  (1 << 30)
#define	PACKET3_ACQUIRE_MEM				0x58
/* 1.  HEADER
 * 2.  COHER_CNTL [30:0]
 * 2.1 ENGINE_SEL [31:31]
 * 3.  COHER_SIZE [31:0]
 * 4.  COHER_SIZE_HI [7:0]
 * 5.  COHER_BASE_LO [31:0]
 * 6.  COHER_BASE_HI [23:0]
 * 7.  POLL_INTERVAL [15:0]
 */
/* COHER_CNTL fields for CP_COHER_CNTL */
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_TC_NC_ACTION_ENA(x) ((x) << 3)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_TC_WC_ACTION_ENA(x) ((x) << 4)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_TC_INV_METADATA_ACTION_ENA(x) \
				((x) << 5)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_TCL1_VOL_ACTION_ENA(x) ((x) << 15)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_TC_WB_ACTION_ENA(x) ((x) << 18)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_TCL1_ACTION_ENA(x) ((x) << 22)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_TC_ACTION_ENA(x) ((x) << 23)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_CB_ACTION_ENA(x) ((x) << 25)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_DB_ACTION_ENA(x) ((x) << 26)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_SH_KCACHE_ACTION_ENA(x) ((x) << 27)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_SH_KCACHE_VOL_ACTION_ENA(x) \
				((x) << 28)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_SH_ICACHE_ACTION_ENA(x) ((x) << 29)
#define	PACKET3_ACQUIRE_MEM_CP_COHER_CNTL_SH_KCACHE_WB_ACTION_ENA(x) ((x) << 30)
#define	PACKET3_REWIND					0x59
#define	PACKET3_LOAD_UCONFIG_REG			0x5E
#define	PACKET3_LOAD_SH_REG				0x5F
#define	PACKET3_LOAD_CONFIG_REG				0x60
#define	PACKET3_LOAD_CONTEXT_REG			0x61
#define	PACKET3_SET_CONFIG_REG				0x68
#define		PACKET3_SET_CONFIG_REG_START			0x00002000
#define		PACKET3_SET_CONFIG_REG_END			0x00002c00
#define	PACKET3_SET_CONTEXT_REG				0x69
#define		PACKET3_SET_CONTEXT_REG_START			0x0000a000
#define		PACKET3_SET_CONTEXT_REG_END			0x0000a400
#define	PACKET3_SET_CONTEXT_REG_INDIRECT		0x73
#define	PACKET3_SET_SH_REG				0x76
#define		PACKET3_SET_SH_REG_START			0x00002c00
#define		PACKET3_SET_SH_REG_END				0x00003000
#define	PACKET3_SET_SH_REG_OFFSET			0x77
#define	PACKET3_SET_QUEUE_REG				0x78
#define	PACKET3_SET_UCONFIG_REG				0x79
#define		PACKET3_SET_UCONFIG_REG_START			0x0000c000
#define		PACKET3_SET_UCONFIG_REG_END			0x0000c400
#define		PACKET3_SET_UCONFIG_REG_INDEX_TYPE		(2 << 28)
#define	PACKET3_SCRATCH_RAM_WRITE			0x7D
#define	PACKET3_SCRATCH_RAM_READ			0x7E
#define	PACKET3_LOAD_CONST_RAM				0x80
#define	PACKET3_WRITE_CONST_RAM				0x81
#define	PACKET3_DUMP_CONST_RAM				0x83
#define	PACKET3_INCREMENT_CE_COUNTER			0x84
#define	PACKET3_INCREMENT_DE_COUNTER			0x85
#define	PACKET3_WAIT_ON_CE_COUNTER			0x86
#define	PACKET3_WAIT_ON_DE_COUNTER_DIFF			0x88
#define	PACKET3_SWITCH_BUFFER				0x8B
#define PACKET3_FRAME_CONTROL				0x90
#			define FRAME_TMZ	(1 << 0)
#			define FRAME_CMD(x) ((x) << 28)
			/*
			 * x=0: tmz_begin
			 * x=1: tmz_end
			 */

#define	PACKET3_INVALIDATE_TLBS				0x98
#              define PACKET3_INVALIDATE_TLBS_DST_SEL(x)     ((x) << 0)
#              define PACKET3_INVALIDATE_TLBS_ALL_HUB(x)     ((x) << 4)
#              define PACKET3_INVALIDATE_TLBS_PASID(x)       ((x) << 5)
#              define PACKET3_INVALIDATE_TLBS_FLUSH_TYPE(x)  ((x) << 29)
#define PACKET3_SET_RESOURCES				0xA0
/* 1. header
 * 2. CONTROL
 * 3. QUEUE_MASK_LO [31:0]
 * 4. QUEUE_MASK_HI [31:0]
 * 5. GWS_MASK_LO [31:0]
 * 6. GWS_MASK_HI [31:0]
 * 7. OAC_MASK [15:0]
 * 8. GDS_HEAP_SIZE [16:11] | GDS_HEAP_BASE [5:0]
 */
#              define PACKET3_SET_RESOURCES_VMID_MASK(x)     ((x) << 0)
#              define PACKET3_SET_RESOURCES_UNMAP_LATENTY(x) ((x) << 16)
#              define PACKET3_SET_RESOURCES_QUEUE_TYPE(x)    ((x) << 29)
#define PACKET3_MAP_QUEUES				0xA2
/* 1. header
 * 2. CONTROL
 * 3. CONTROL2
 * 4. MQD_ADDR_LO [31:0]
 * 5. MQD_ADDR_HI [31:0]
 * 6. WPTR_ADDR_LO [31:0]
 * 7. WPTR_ADDR_HI [31:0]
 */
/* CONTROL */
#              define PACKET3_MAP_QUEUES_QUEUE_SEL(x)       ((x) << 4)
#              define PACKET3_MAP_QUEUES_VMID(x)            ((x) << 8)
#              define PACKET3_MAP_QUEUES_QUEUE(x)           ((x) << 13)
#              define PACKET3_MAP_QUEUES_PIPE(x)            ((x) << 16)
#              define PACKET3_MAP_QUEUES_ME(x)              ((x) << 18)
#              define PACKET3_MAP_QUEUES_QUEUE_TYPE(x)      ((x) << 21)
#              define PACKET3_MAP_QUEUES_ALLOC_FORMAT(x)    ((x) << 24)
#              define PACKET3_MAP_QUEUES_ENGINE_SEL(x)      ((x) << 26)
#              define PACKET3_MAP_QUEUES_NUM_QUEUES(x)      ((x) << 29)
/* CONTROL2 */
#              define PACKET3_MAP_QUEUES_CHECK_DISABLE(x)   ((x) << 1)
#              define PACKET3_MAP_QUEUES_DOORBELL_OFFSET(x) ((x) << 2)
#define	PACKET3_UNMAP_QUEUES				0xA3
/* 1. header
 * 2. CONTROL
 * 3. CONTROL2
 * 4. CONTROL3
 * 5. CONTROL4
 * 6. CONTROL5
 */
/* CONTROL */
#              define PACKET3_UNMAP_QUEUES_ACTION(x)           ((x) << 0)
		/* 0 - PREEMPT_QUEUES
		 * 1 - RESET_QUEUES
		 * 2 - DISABLE_PROCESS_QUEUES
		 * 3 - PREEMPT_QUEUES_NO_UNMAP
		 */
#              define PACKET3_UNMAP_QUEUES_QUEUE_SEL(x)        ((x) << 4)
#              define PACKET3_UNMAP_QUEUES_ENGINE_SEL(x)       ((x) << 26)
#              define PACKET3_UNMAP_QUEUES_NUM_QUEUES(x)       ((x) << 29)
/* CONTROL2a */
#              define PACKET3_UNMAP_QUEUES_PASID(x)            ((x) << 0)
/* CONTROL2b */
#              define PACKET3_UNMAP_QUEUES_DOORBELL_OFFSET0(x) ((x) << 2)
/* CONTROL3a */
#              define PACKET3_UNMAP_QUEUES_DOORBELL_OFFSET1(x) ((x) << 2)
/* CONTROL3b */
#              define PACKET3_UNMAP_QUEUES_RB_WPTR(x)          ((x) << 0)
/* CONTROL4 */
#              define PACKET3_UNMAP_QUEUES_DOORBELL_OFFSET2(x) ((x) << 2)
/* CONTROL5 */
#              define PACKET3_UNMAP_QUEUES_DOORBELL_OFFSET3(x) ((x) << 2)
#define	PACKET3_QUERY_STATUS				0xA4
/* 1. header
 * 2. CONTROL
 * 3. CONTROL2
 * 4. ADDR_LO [31:0]
 * 5. ADDR_HI [31:0]
 * 6. DATA_LO [31:0]
 * 7. DATA_HI [31:0]
 */
/* CONTROL */
#              define PACKET3_QUERY_STATUS_CONTEXT_ID(x)       ((x) << 0)
#              define PACKET3_QUERY_STATUS_INTERRUPT_SEL(x)    ((x) << 28)
#              define PACKET3_QUERY_STATUS_COMMAND(x)          ((x) << 30)
/* CONTROL2a */
#              define PACKET3_QUERY_STATUS_PASID(x)            ((x) << 0)
/* CONTROL2b */
#              define PACKET3_QUERY_STATUS_DOORBELL_OFFSET(x)  ((x) << 2)
#              define PACKET3_QUERY_STATUS_ENG_SEL(x)          ((x) << 25)

#define MAX_BUF_SIZE_PER_LINE 128

enum amdgv_gfx_pipe_priority {
	AMDGV_GFX_PIPE_PRIO_NORMAL = AMDGV_RING_PRIO_1,
	AMDGV_GFX_PIPE_PRIO_HIGH = AMDGV_RING_PRIO_2
};

#define AMDGV_GFX_QUEUE_PRIORITY_MINIMUM  0
#define AMDGV_GFX_QUEUE_PRIORITY_MAXIMUM  15

#define AMDGV_MAX_GC_INSTANCES 8

struct soc15_reg {
	uint32_t hwip;
	uint32_t inst;
	uint32_t seg;
	uint32_t reg_offset;
};

struct soc15_reg_entry {
	uint32_t hwip;
	uint32_t inst;
	uint32_t seg;
	uint32_t reg_offset;
	uint32_t reg_value;
	uint32_t se_num;
	uint32_t instance;
};

struct soc15_ras_field_entry {
	const char *name;
	uint32_t hwip;
	uint32_t inst;
	uint32_t seg;
	uint32_t reg_offset;
	uint32_t sec_count_mask;
	uint32_t sec_count_shift;
	uint32_t ded_count_mask;
	uint32_t ded_count_shift;
};

enum {
	PERFCOUNTER_TA,
	PERFCOUNTER_TD,
	PERFCOUNTER_SQ0,        //SQ register perfcounter can only use even numbers
	PERFCOUNTER_SQ2,        //SQ register perfcounter can only use even numbers
	PERFCOUNTER_SQ4,        //SQ register perfcounter can only use even numbers
	PERFCOUNTER_SQ6,        //SQ register perfcounter can only use even numbers
	PERFCOUNTER_PER_WGP = PERFCOUNTER_SQ6,    //Above this, need to be read per-WGP
	PERFCOUNTER_CPG,
	PERFCOUNTER_SQG,
	PERFCOUNTER_SDMA0,
	PERFCOUNTER_SDMA1,
	PERFCOUNTER_SDMA2,
	PERFCOUNTER_SDMA3,
	PERFCOUNTER_MAX
};

typedef int (*amdgv_wait_cb_t)(void *cb_context);

struct amdgv_gfx_funcs {
	/* hang detection */
	int (*wait_detect_hang)(struct amdgv_adapter *adapt, amdgv_wait_cb_t cb_func, void *cb_context, uint64_t timeout);

	/* legacy ecc */
	void (*err_cnt_init)(struct amdgv_adapter *adapt);
	int (*ras_late_init)(struct amdgv_adapter *adapt);
	void (*query_ras_error_count)(struct amdgv_adapter *adapt, void *ras_error_status);
	void (*query_ras_error_status)(struct amdgv_adapter *adapt);
	void (*reset_ras_error_count)(struct amdgv_adapter *adapt);
	void (*reset_ras_error_status)(struct amdgv_adapter *adapt);
	int  (*ras_error_inject)(struct amdgv_adapter *adapt,
		void *inject_if, uint32_t instance_mask);
	int  (*hw_init_internal_set)(struct amdgv_adapter *adapt);
};

struct amdgv_mec {
	struct amdgv_memmgr_mem	*hpd_eop_obj;
	uint64_t			hpd_eop_gpu_addr;
	struct amdgv_memmgr_mem	*mec_fw_obj;
	uint64_t			mec_fw_gpu_addr;
	struct amdgv_memmgr_mem	*mec_fw_data_obj;
	uint64_t			mec_fw_data_gpu_addr;

	uint32_t num_mec;
	uint32_t num_pipe_per_mec;
	uint32_t num_queue_per_pipe;
	uint32_t mec_hpd_size;
	void	*mqd_backup[AMDGV_MAX_COMPUTE_RINGS * AMDGV_MAX_GC_INSTANCES];

	/* config which compute will be used as paging queue */
	uint32_t paging_me;
	uint32_t paging_pipe;
	uint32_t paging_queue;
};

enum amdgv_unmap_queues_action {
	PREEMPT_QUEUES = 0,
	RESET_QUEUES,
	DISABLE_PROCESS_QUEUES,
	PREEMPT_QUEUES_NO_UNMAP,
};

struct kiq_pm4_funcs {
	/* Support ASIC-specific kiq pm4 packets*/
	void (*kiq_set_resources)(struct amdgv_ring *kiq_ring,
					uint64_t queue_mask);
	void (*kiq_map_queues)(struct amdgv_ring *kiq_ring,
					struct amdgv_ring *ring);
	void (*kiq_unmap_queues)(struct amdgv_ring *kiq_ring,
				 struct amdgv_ring *ring,
				 enum amdgv_unmap_queues_action action,
				 uint64_t gpu_addr, uint64_t seq);
	void (*kiq_query_status)(struct amdgv_ring *kiq_ring,
					struct amdgv_ring *ring,
					uint64_t addr,
					uint64_t seq);
	void (*kiq_invalidate_tlbs)(struct amdgv_ring *kiq_ring,
				uint16_t pasid, uint32_t flush_type,
				bool all_hub);
	/* Packet sizes */
	int set_resources_size;
	int map_queues_size;
	int unmap_queues_size;
	int query_status_size;
	int invalidate_tlbs_size;
};

struct amdgv_kiq {
	uint64_t			eop_gpu_addr;
	struct amdgv_memmgr_mem	*eop_obj;
	spin_lock_t              ring_lock;
	struct amdgv_ring	ring;
	const struct kiq_pm4_funcs *pmf;
	void *mqd_backup;
};

/*
 * GFX configurations
 */
#define AMDGV_GFX_MAX_SE 4
#define AMDGV_GFX_MAX_SH_PER_SE 2

struct amdgv_rb_config {
	uint32_t rb_backend_disable;
	uint32_t user_rb_backend_disable;
	uint32_t raster_config;
	uint32_t raster_config_1;
};

struct gb_addr_config {
	uint16_t pipe_interleave_size;
	uint8_t num_pipes;
	uint8_t max_compress_frags;
	uint8_t num_banks;
	uint8_t num_se;
	uint8_t num_rb_per_se;
	uint8_t num_pkrs;
};

struct amdgv_gfx_config {
	unsigned int max_shader_engines;
	unsigned int max_tile_pipes;
	unsigned int max_cu_per_sh;
	unsigned int max_sh_per_se;
	unsigned int max_backends_per_se;
	unsigned int max_texture_channel_caches;
	unsigned int max_gprs;
	unsigned int max_gs_threads;
	unsigned int max_hw_contexts;
	unsigned int sc_prim_fifo_size_frontend;
	unsigned int sc_prim_fifo_size_backend;
	unsigned int sc_hiz_tile_fifo_size;
	unsigned int sc_earlyz_tile_fifo_size;

	unsigned int num_tile_pipes;
	unsigned int backend_enable_mask;
	unsigned int mem_max_burst_length_bytes;
	unsigned int mem_row_size_in_kb;
	unsigned int shader_engine_tile_size;
	unsigned int num_gpus;
	unsigned int multi_gpu_tile_size;
	unsigned int mc_arb_ramcfg;
	unsigned int num_banks;
	unsigned int num_ranks;
	unsigned int gb_addr_config;
	unsigned int num_rbs;
	unsigned int gs_vgt_table_depth;
	unsigned int gs_prim_buffer_depth;

	uint32_t tile_mode_array[32];
	uint32_t macrotile_mode_array[16];

	struct gb_addr_config gb_addr_config_fields;
	struct amdgv_rb_config rb_config[AMDGV_GFX_MAX_SE][AMDGV_GFX_MAX_SH_PER_SE];

	/* gfx configure feature */
	uint32_t double_offchip_lds_buf;
	/* cached value of DB_DEBUG2 */
	uint32_t db_debug2;
	/* gfx10 specific config */
	uint32_t num_sc_per_sh;
	uint32_t num_packer_per_sc;
	uint32_t pa_sc_tile_steering_override;
	uint64_t tcc_disabled_mask;
	uint32_t gc_num_tcp_per_sa;
	uint32_t gc_num_sdp_interface;
	uint32_t gc_num_tcps;
	uint32_t gc_num_tcp_per_wpg;
	uint32_t gc_tcp_l1_size;
	uint32_t gc_num_sqc_per_wgp;
	uint32_t gc_l1_instruction_cache_size_per_sqc;
	uint32_t gc_l1_data_cache_size_per_sqc;
	uint32_t gc_gl1c_per_sa;
	uint32_t gc_gl1c_size_per_instance;
	uint32_t gc_gl2c_per_gpu;
};

struct amdgv_cu_info {
	uint32_t simd_per_cu;
	uint32_t max_waves_per_simd;
	uint32_t wave_front_size;
	uint32_t max_scratch_slots_per_cu;
	uint32_t lds_size;
	// General GPU information
	uint32_t num_se;
	uint32_t num_sa_per_se;
	uint32_t num_cus_per_sa;
	// SGPR GPU information
	uint32_t num_sgprs_per_simd;
	uint32_t num_sgprs_per_wave_slot;
	// VGPR GPU information
	uint32_t num_vgprs_per_simd;
	uint32_t num_lanes_per_vgpr;
	// LDS GPU information
	uint64_t num_lds_dwords_per_cu;

	/* total active CU number */
	uint32_t number;
	uint32_t ao_cu_mask;
	uint32_t ao_cu_bitmap[4][4];
	uint32_t bitmap[4][4];
};

typedef struct {
	uint32_t group_segment_fixed_size;
	uint32_t private_segment_fixed_size;
	uint32_t kernarg_size;
	uint32_t reserved1;
	uint64_t kernel_code_entry_byte_offset;
	uint32_t reserved2[5];
	uint32_t compute_pgm_rsrc3;
	uint32_t compute_pgm_rsrc1;
	uint32_t compute_pgm_rsrc2;
	uint16_t enable_sgpr_private_segment_buffer:1;
	uint16_t enable_sgpr_dispatch_ptr:1;
	uint16_t enable_sgpr_queue_ptr:1;
	uint16_t enable_sgpr_kernarg_segment_ptr:1;
	uint16_t enable_sgpr_dispatch_id:1;
	uint16_t enable_sgpr_flat_scratch_init:1;
	uint16_t enable_sgpr_private_segment_size:1;
	uint16_t reserved3:3;
	uint16_t enable_wavefront_size32:1;
	uint16_t uses_dynamic_stack:1;
	uint16_t reserved4:4;
	uint16_t kernarg_preload_spec_length:7;
	uint16_t kernarg_preload_spec_offset:9;
	uint32_t reserved5;
} kernel_descriptor_t;

struct amdgv_rlcg_reg_access_ctrl {
	uint32_t scratch_reg0;
	uint32_t scratch_reg1;
	uint32_t scratch_reg2;
	uint32_t scratch_reg3;
	uint32_t grbm_cntl;
	uint32_t grbm_idx;
	uint32_t spare_int;
};

struct amdgv_rlc_funcs {
	bool (*is_rlc_enabled)(struct amdgv_adapter *adapt);
	void (*set_safe_mode)(struct amdgv_adapter *adapt, int xcc_id);
	void (*unset_safe_mode)(struct amdgv_adapter *adapt, int xcc_id);
	int  (*init)(struct amdgv_adapter *adapt);
	uint32_t  (*get_csb_size)(struct amdgv_adapter *adapt);
	void (*get_csb_buffer)(struct amdgv_adapter *adapt,
				volatile uint32_t *buffer);
	int  (*get_cp_table_num)(struct amdgv_adapter *adapt);
	int  (*resume)(struct amdgv_adapter *adapt);
	void (*stop)(struct amdgv_adapter *adapt);
	void (*reset)(struct amdgv_adapter *adapt);
	void (*start)(struct amdgv_adapter *adapt);
	void (*update_spm_vmid)(struct amdgv_adapter *adapt, unsigned int vmid);
	bool (*is_rlcg_access_range)(struct amdgv_adapter *adapt, uint32_t reg);
};

struct amdgv_rlc {
	/* for power gating */
	struct amdgv_memmgr_mem        *save_restore_obj;

	uint64_t                save_restore_gpu_addr;

	volatile uint32_t       *sr_ptr;
	const uint32_t               *reg_list;
	uint32_t                     reg_list_size;
	/* for clear state */
	struct amdgv_memmgr_mem        *clear_state_obj;
	uint64_t                clear_state_gpu_addr;
	volatile uint32_t       *cs_ptr;
	uint32_t                     clear_state_size;
	/* for cp tables */
	struct amdgv_memmgr_mem        *cp_table_obj;
	uint64_t                cp_table_gpu_addr;
	volatile uint32_t       *cp_table_ptr;
	uint32_t                     cp_table_size;

	/* safe mode for updating CG/PG state */
	bool in_safe_mode[AMDGV_MAX_GC_INSTANCES];
	const struct amdgv_rlc_funcs *funcs;

	/* for firmware data */
	uint32_t save_and_restore_offset;
	uint32_t clear_state_descriptor_offset;
	uint32_t avail_scratch_ram_locations;
	uint32_t reg_restore_list_size;
	uint32_t reg_list_format_start;
	uint32_t reg_list_format_separate_start;
	uint32_t starting_offsets_start;
	uint32_t reg_list_format_size_bytes;
	uint32_t reg_list_size_bytes;
	uint32_t reg_list_format_direct_reg_list_length;
	uint32_t save_restore_list_cntl_size_bytes;
	uint32_t save_restore_list_gpm_size_bytes;
	uint32_t save_restore_list_srm_size_bytes;
	uint32_t rlc_iram_ucode_size_bytes;
	uint32_t rlc_dram_ucode_size_bytes;
	uint32_t rlcp_ucode_size_bytes;
	uint32_t rlcv_ucode_size_bytes;

	uint32_t *register_list_format;
	uint32_t *register_restore;
	uint8_t *save_restore_list_cntl;
	uint8_t *save_restore_list_gpm;
	uint8_t *save_restore_list_srm;
	uint8_t *rlc_iram_ucode;
	uint8_t *rlc_dram_ucode;
	uint8_t *rlcp_ucode;
	uint8_t *rlcv_ucode;

	bool is_rlc_v2_1;

	/* for rlc autoload */
	struct amdgv_memmgr_mem	*rlc_autoload_bo;
	uint64_t			rlc_autoload_gpu_addr;
	void			*rlc_autoload_ptr;

	/* rlc toc buffer */
	struct amdgv_memmgr_mem	*rlc_toc_bo;
	uint64_t		rlc_toc_gpu_addr;
	void			*rlc_toc_buf;

	bool rlcg_reg_access_supported;
	/* registers for rlcg indirect reg access */
	struct amdgv_rlcg_reg_access_ctrl reg_access_ctrl[AMDGV_MAX_GC_INSTANCES];
};

struct amdgv_gfx {
	struct ras_common_if *ras_if;
	struct amdgv_gfx_config	config;
	struct amdgv_rlc		rlc;
	struct amdgv_mec		mec;
	struct amdgv_kiq		kiq[AMDGV_MAX_GC_INSTANCES];
	uint64_t 			mec_queue_bitmap[AMDGV_MAX_GC_INSTANCES];
	bool				rs64_enable; /* firmware format */
	uint32_t			imu_fw_version;
	uint32_t			me_feature_version;
	uint32_t			ce_feature_version;
	uint32_t			pfp_feature_version;
	uint32_t			rlc_feature_version;
	uint32_t			rlc_srlc_fw_version;
	uint32_t			rlc_srlc_feature_version;
	uint32_t			rlc_srlg_fw_version;
	uint32_t			rlc_srlg_feature_version;
	uint32_t			rlc_srls_fw_version;
	uint32_t			rlc_srls_feature_version;
	uint32_t			mec_feature_version;
	uint32_t			mec2_feature_version;
	bool				mec_fw_write_wait;
	bool				me_fw_write_wait;
	bool				cp_fw_write_wait;
	uint32_t			num_gfx_rings;
	struct amdgv_ring	gfx_ring[AMDGV_MAX_GFX_RINGS];
	uint32_t			num_compute_rings;
	uint32_t			compute_paging_queue_id;
	struct amdgv_ring	compute_ring[AMDGV_MAX_GC_INSTANCES * AMDGV_MAX_COMPUTE_RINGS];

	/* gfx status */
	uint32_t			gfx_current_status;
	/* ce ram size*/
	uint32_t			ce_ram_size;

	struct amdgv_gfx_funcs	*funcs;

	/* reset mask */
	uint32_t            grbm_soft_reset;
	uint32_t            srbm_soft_reset;

	/* gfx off */
	bool gfx_off_state; /* true: enabled, false: disabled */
	/* default 1, enable gfx off: dec 1, disable gfx off: add 1 */
	uint32_t            gfx_off_req_count;

	uint64_t pipe_reserve_bitmap;

	bool				is_poweron;

	/* hang detection parameters */
	bool              hang_detection_supported;
	uint32_t          hang_detection_threshold_us;
	uint32_t          hang_detection_duration_us;
};

enum amdgv_gfx_ras_mem_id_type {
	AMDGV_GFX_CP_MEM = 0,
	AMDGV_GFX_GCEA_MEM,
	AMDGV_GFX_GC_CANE_MEM,
	AMDGV_GFX_GCUTCL2_MEM,
	AMDGV_GFX_GDS_MEM,
	AMDGV_GFX_LDS_MEM,
	AMDGV_GFX_RLC_MEM,
	AMDGV_GFX_SP_MEM,
	AMDGV_GFX_SPI_MEM,
	AMDGV_GFX_SQC_MEM,
	AMDGV_GFX_SQ_MEM,
	AMDGV_GFX_TA_MEM,
	AMDGV_GFX_TCC_MEM,
	AMDGV_GFX_TCA_MEM,
	AMDGV_GFX_TCI_MEM,
	AMDGV_GFX_TCP_MEM,
	AMDGV_GFX_TD_MEM,
	AMDGV_GFX_TCX_MEM,
	AMDGV_GFX_ATC_L2_MEM,
	AMDGV_GFX_UTCL2_MEM,
	AMDGV_GFX_VML2_MEM,
	AMDGV_GFX_VML2_WALKER_MEM,
	AMDGV_GFX_MEM_TYPE_NUM
};

enum amdgv_gfx_xcc_queue_index {
	XCC_QUEUE_INDEX__AQL = 0,
	XCC_QUEUE_INDEX__PAGING = 1,
	XCC_QUEUE_INDEX__MAX = 2,
};

struct amdgv_gfx_ras_reg_entry {
	struct amdgv_ras_err_status_reg_entry reg_entry;
	enum amdgv_gfx_ras_mem_id_type mem_id_type;
	uint32_t se_num;
};

struct amdgv_gfx_ras_mem_id_entry {
	const struct amdgv_ras_memory_id_entry *mem_id_ent;
	uint32_t size;
};

#define AMDGV_GFX_MEMID_ENT(x) {(x), ARRAY_SIZE(x)},
#define AMDGV_GFX_GET_NUM_XCC (adapt->mcp.gfx.num_xcc ? adapt->mcp.gfx.num_xcc : 1)

#define amdgv_gfx_get_gpu_clock_counter(adapt) \
		((adapt)->gfx.funcs->get_gpu_clock_counter((adapt)))
#define amdgv_gfx_select_se_sh(adapt, se, sh, instance) \
		((adapt)->gfx.funcs->select_se_sh((adapt), (se), \
		(sh), (instance)))
#define amdgv_gfx_select_me_pipe_q(adapt, me, pipe, q, vmid) \
		((adapt)->gfx.funcs->select_me_pipe_q((adapt), (me), \
		(pipe), (q), (vmid)))
#define amdgv_gfx_init_spm_golden(adapt) \
		((adapt)->gfx.funcs->init_spm_golden((adapt)))

bool amdgv_gfx_test_bit(int nr, uint64_t bitmap);
void amdgv_gfx_set_bit(int nr, uint64_t *bitmap);
void amdgv_gfx_clear_bit(int nr, uint64_t *bitmap);
unsigned long amdgv_gfx_find_first_zero_bit(uint64_t bitmap);

int amdgv_gfx_kiq_init_ring(struct amdgv_adapter *adapt,
				struct amdgv_ring *ring, int xcc_id);
void amdgv_gfx_kiq_free_ring(struct amdgv_ring *ring);

void amdgv_gfx_kiq_fini(struct amdgv_adapter *adapt, int xcc_id);
int amdgv_gfx_kiq_init(struct amdgv_adapter *adapt,
			unsigned int hpd_size, int xcc_id);
int amdgv_gfx_kiq_init_set(struct amdgv_adapter *adapt,
			unsigned int hpd_size, int xcc_id);
int amdgv_gfx_mqd_sw_init(struct amdgv_adapter *adapt,
			   unsigned int mqd_size, int xcc_id);
int amdgv_gfx_mqd_init_set(struct amdgv_adapter *adapt, int xcc_id);
void amdgv_gfx_mqd_sw_fini(struct amdgv_adapter *adapt, int xcc_id);

int amdgv_gfx_kiq_set_resources(struct amdgv_adapter *adapt, int xcc_id);
int amdgv_gfx_map_kcq(struct amdgv_adapter *adapt, int xcc_id,
						enum amdgv_gfx_xcc_queue_index index);
int amdgv_gfx_unmap_kcq(struct amdgv_adapter *adapt, int xcc_id,
						enum amdgv_gfx_xcc_queue_index index);

void amdgv_gfx_compute_queue_acquire(struct amdgv_adapter *adapt);

int amdgv_gfx_mec_queue_to_bit(struct amdgv_adapter *adapt, int mec,
				int pipe, int queue);
void amdgv_queue_mask_bit_to_mec_queue(struct amdgv_adapter *adapt, int bit,
				 int *mec, int *pipe, int *queue);
bool amdgv_gfx_is_mec_queue_enabled(struct amdgv_adapter *adapt, int xcc_id,
					int mec, int pipe, int queue);
bool amdgv_gfx_is_high_priority_compute_queue(struct amdgv_adapter *adapt,
					       struct amdgv_ring *ring);
int amdgv_gfx_me_queue_to_bit(struct amdgv_adapter *adapt, int me,
			       int pipe, int queue);
void amdgv_gfx_bit_to_me_queue(struct amdgv_adapter *adapt, int bit,
				int *me, int *pipe, int *queue);
int amdgv_queue_mask_bit_to_set_resource_bit(struct amdgv_adapter *adapt,
					int queue_bit);
void amdgv_gfx_ras_error_func(struct amdgv_adapter *adapt,
	void *ras_error_status,
	void (*func)(struct amdgv_adapter *adapt, void *ras_error_status,
	int xcc_id));
int amdgv_gfx_get_compute_cap(struct amdgv_adapter *adapt, bool min, uint32_t *compute_cap);
void amdgv_gfx_rlc_enter_safe_mode(struct amdgv_adapter *adapt, int xcc_id);
void amdgv_gfx_rlc_exit_safe_mode(struct amdgv_adapter *adapt, int xcc_id);

#endif
