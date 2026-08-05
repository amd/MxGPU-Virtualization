/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SDMA_V7_1_H__
#define __SDMA_V7_1_H__

#define SDMA1_REG_OFFSET 0x600
#define SDMA0_SDMA_IDX_0_END 0x450
#define SDMA1_HYP_DEC_REG_OFFSET 0x30

#define SDMA_OP_NOP  0
#define SDMA_OP_COPY  1
#define SDMA_OP_WRITE  2
#define SDMA_OP_INDIRECT  4
#define SDMA_OP_FENCE  5
#define SDMA_OP_TRAP  6
#define SDMA_OP_SEM  7
#define SDMA_OP_POLL_REGMEM  8
#define SDMA_OP_COND_EXE  9
#define SDMA_OP_ATOMIC  10
#define SDMA_OP_CONST_FILL  11
#define SDMA_OP_PTEPDE  12
#define SDMA_OP_TIMESTAMP  13
#define SDMA_OP_SRBM_WRITE  14
#define SDMA_OP_PRE_EXE  15
#define SDMA_OP_GPUVM_INV  16
#define SDMA_OP_GCR_REQ  17
#define SDMA_OP_DUMMY_TRAP  32
#define SDMA_SUBOP_TIMESTAMP_SET  0
#define SDMA_SUBOP_TIMESTAMP_GET  1
#define SDMA_SUBOP_TIMESTAMP_GET_GLOBAL  2
#define SDMA_SUBOP_COPY_LINEAR  0
#define SDMA_SUBOP_COPY_LINEAR_SUB_WIND  4
#define SDMA_SUBOP_COPY_TILED  1
#define SDMA_SUBOP_COPY_TILED_SUB_WIND  5
#define SDMA_SUBOP_COPY_T2T_SUB_WIND  6
#define SDMA_SUBOP_COPY_SOA  3
#define SDMA_SUBOP_COPY_DIRTY_PAGE  7
#define SDMA_SUBOP_COPY_LINEAR_PHY  8
#define SDMA_SUBOP_COPY_LINEAR_SUB_WIND_LARGE  36
#define SDMA_SUBOP_COPY_LINEAR_BC  16
#define SDMA_SUBOP_COPY_TILED_BC  17
#define SDMA_SUBOP_COPY_LINEAR_SUB_WIND_BC  20
#define SDMA_SUBOP_COPY_TILED_SUB_WIND_BC  21
#define SDMA_SUBOP_COPY_T2T_SUB_WIND_BC  22
#define SDMA_SUBOP_WRITE_LINEAR  0
#define SDMA_SUBOP_WRITE_TILED  1
#define SDMA_SUBOP_WRITE_TILED_BC  17
#define SDMA_SUBOP_PTEPDE_GEN  0
#define SDMA_SUBOP_PTEPDE_COPY  1
#define SDMA_SUBOP_PTEPDE_RMW  2
#define SDMA_SUBOP_PTEPDE_COPY_BACKWARDS  3
#define SDMA_SUBOP_MEM_INCR  1
#define SDMA_SUBOP_DATA_FILL_MULTI  1
#define SDMA_SUBOP_POLL_REG_WRITE_MEM  1
#define SDMA_SUBOP_POLL_DBIT_WRITE_MEM  2
#define SDMA_SUBOP_POLL_MEM_VERIFY  3
#define SDMA_SUBOP_VM_INVALIDATION  4
#define HEADER_AGENT_DISPATCH  4
#define HEADER_BARRIER  5
#define SDMA_OP_AQL_COPY  0
#define SDMA_OP_AQL_BARRIER_OR  0

/*
** Definitions for SDMA_PKT_NOP packet
*/

/*define for HEADER word*/
/*define for op field*/
#define SDMA_PKT_NOP_HEADER_op_offset 0
#define SDMA_PKT_NOP_HEADER_op_mask   0x000000FF
#define SDMA_PKT_NOP_HEADER_op_shift  0
#define SDMA_PKT_NOP_HEADER_OP(x) (((x) & SDMA_PKT_NOP_HEADER_op_mask) << SDMA_PKT_NOP_HEADER_op_shift)

/*
** Definitions for SDMA_PKT_COPY_LINEAR packet
*/

/*define for HEADER word*/
/*define for op field*/
#define SDMA_PKT_COPY_LINEAR_HEADER_op_offset 0
#define SDMA_PKT_COPY_LINEAR_HEADER_op_mask   0x000000FF
#define SDMA_PKT_COPY_LINEAR_HEADER_op_shift  0
#define SDMA_PKT_COPY_LINEAR_HEADER_OP(x) (((x) & SDMA_PKT_COPY_LINEAR_HEADER_op_mask) << SDMA_PKT_COPY_LINEAR_HEADER_op_shift)

/*define for sub_op field*/
#define SDMA_PKT_COPY_LINEAR_HEADER_sub_op_offset 0
#define SDMA_PKT_COPY_LINEAR_HEADER_sub_op_mask   0x000000FF
#define SDMA_PKT_COPY_LINEAR_HEADER_sub_op_shift  8
#define SDMA_PKT_COPY_LINEAR_HEADER_SUB_OP(x) (((x) & SDMA_PKT_COPY_LINEAR_HEADER_sub_op_mask) << SDMA_PKT_COPY_LINEAR_HEADER_sub_op_shift)

/*
** Definitions for SDMA_PKT_FENCE packet
*/

/*define for HEADER word*/
/*define for mtype field*/
#define SDMA_PKT_FENCE_HEADER_mtype_offset 0
#define SDMA_PKT_FENCE_HEADER_mtype_mask   0x00000007
#define SDMA_PKT_FENCE_HEADER_mtype_shift  16
#define SDMA_PKT_FENCE_HEADER_MTYPE(x) (((x) & SDMA_PKT_FENCE_HEADER_mtype_mask) << SDMA_PKT_FENCE_HEADER_mtype_shift)

/*
** Definitions for SDMA_PKT_WRITE_UNTILED packet
*/

/*define for HEADER word*/
/*define for DW_3 word*/
/*define for count field*/
#define SDMA_PKT_WRITE_UNTILED_DW_3_count_offset 3
#define SDMA_PKT_WRITE_UNTILED_DW_3_count_mask   0x000FFFFF
#define SDMA_PKT_WRITE_UNTILED_DW_3_count_shift  0
#define SDMA_PKT_WRITE_UNTILED_DW_3_COUNT(x) (((x) & SDMA_PKT_WRITE_UNTILED_DW_3_count_mask) << SDMA_PKT_WRITE_UNTILED_DW_3_count_shift)

enum AMDGPU_DOORBELL_ASSIGNMENT {
	AMDGPU_DOORBELL_SDMA_ENGINE_START = 0x100,
	AMDGPU_DOORBELL_SDMA_ENGINE_END	= 0x19F,
};

void sdma_v7_1_set_ras_funcs(struct amdgv_adapter *adapt);
int sdma_v7_1_sdma_query_dirtybit(struct amdgv_adapter *adapt, uint64_t mc_addr,
			struct amdgv_query_dirty_bit_data *data);
int sdma_v7_1_hw_resume(struct amdgv_adapter *adapt, uint32_t xcc_mask);
int sdma_v7_1_hw_suspend(struct amdgv_adapter *adapt, uint32_t xcc_mask);
#endif
