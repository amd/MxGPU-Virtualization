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
#ifndef __SDMA_V4_4_2_H__
#define __SDMA_V4_4_2_H__

#define SDMA_INST_NUM_PER_AID   4

#define SDMA_INST_TO_AID(_SDMA_INST_) ((_SDMA_INST_)/adapt->sdma.num_inst_per_aid)

enum SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG {
	SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG_256KB = 0,
	SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG_512KB = 1,
	SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG_1MB = 2,
	SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG_2MB = 3,
	SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG_MAX,
};

/*define for HEADER word*/
/*define for op field*/
#define SDMA_PKT_NOP_HEADER_op_offset 0
#define SDMA_PKT_NOP_HEADER_op_mask   0x000000FF
#define SDMA_PKT_NOP_HEADER_op_shift  0
#define SDMA_PKT_NOP_HEADER_OP(x) (((x) & SDMA_PKT_NOP_HEADER_op_mask) << SDMA_PKT_NOP_HEADER_op_shift)

/*define for op field*/
#define SDMA_PKT_HEADER_op_offset 0
#define SDMA_PKT_HEADER_op_mask   0x000000FF
#define SDMA_PKT_HEADER_op_shift  0
#define SDMA_PKT_HEADER_OP(x) (((x) & SDMA_PKT_HEADER_op_mask) << SDMA_PKT_HEADER_op_shift)

/*define for sub_op field*/
#define SDMA_PKT_HEADER_sub_op_offset 0
#define SDMA_PKT_HEADER_sub_op_mask   0x000000FF
#define SDMA_PKT_HEADER_sub_op_shift  8
#define SDMA_PKT_HEADER_SUB_OP(x) (((x) & SDMA_PKT_HEADER_sub_op_mask) << SDMA_PKT_HEADER_sub_op_shift)

/*define for mtype field */
#define SDMA_PKT_FENCE_HEADER_mtype_offset 0
#define SDMA_PKT_FENCE_HEADER_mtype_mask   0x00000007
#define SDMA_PKT_FENCE_HEADER_mtype_shift  16
#define SDMA_PKT_FENCE_HEADER_MTYPE(x) (((x) & SDMA_PKT_FENCE_HEADER_mtype_mask) << SDMA_PKT_FENCE_HEADER_mtype_shift)

/*define for DW_3 word*/
/*define for count field*/
#define SDMA_PKT_WRITE_UNTILED_DW_3_count_offset 3
#define SDMA_PKT_WRITE_UNTILED_DW_3_count_mask   0x000FFFFF
#define SDMA_PKT_WRITE_UNTILED_DW_3_count_shift  0
#define SDMA_PKT_WRITE_UNTILED_DW_3_COUNT(x) (((x) & SDMA_PKT_WRITE_UNTILED_DW_3_count_mask) << SDMA_PKT_WRITE_UNTILED_DW_3_count_shift)

/*define for ea inst field*/
#define SDMA_PKT_HEADER_ea_offset 0
#define SDMA_PKT_HEADER_ea_mask   0x0000000F
#define SDMA_PKT_HEADER_ea_shift  16
#define SDMA_PKT_HEADER_EA(x) (((x) & SDMA_PKT_HEADER_ea_mask) << SDMA_PKT_HEADER_ea_shift)

/*define for aid inst field*/
#define SDMA_PKT_HEADER_aid_offset 0
#define SDMA_PKT_HEADER_aid_mask   0x0000000F
#define SDMA_PKT_HEADER_aid_shift  28
#define SDMA_PKT_HEADER_AID(x) (((x) & SDMA_PKT_HEADER_aid_mask) << SDMA_PKT_HEADER_aid_shift)

/*define for dagb field*/
#define SDMA_PKT_HEADER_dagb_offset 0
#define SDMA_PKT_HEADER_dagb_mask   0x0000000F
#define SDMA_PKT_HEADER_dagb_shift  20
#define SDMA_PKT_HEADER_DAGB(x) (((x) & SDMA_PKT_HEADER_dagb_mask) << SDMA_PKT_HEADER_dagb_shift)

/*define for dbit preserve*/
#define SDMA_PKT_HEADER_dbit_p_offset 0
#define SDMA_PKT_HEADER_dbit_p_mask   0x00000001
#define SDMA_PKT_HEADER_dbit_p_shift  31
#define SDMA_PKT_HEADER_DBIT_P(x) (((x) & SDMA_PKT_HEADER_dbit_p_mask) << SDMA_PKT_HEADER_dbit_p_shift)

#define SDMA_PKT_DBIT_FB_ADDRESS(x) (((x) >> 16) << 4)

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
#define SDMA_SUBOP_DATA_FILL_MULTI  1
#define SDMA_SUBOP_POLL_REG_WRITE_MEM  1
#define SDMA_SUBOP_POLL_DBIT_WRITE_MEM  2
#define SDMA_SUBOP_POLL_MEM_VERIFY  3
#define HEADER_AGENT_DISPATCH  4
#define HEADER_BARRIER  5
#define SDMA_OP_AQL_COPY  0
#define SDMA_OP_AQL_BARRIER_OR  0

void sdma_v4_4_2_set_ras_funcs(struct amdgv_adapter *adapt);
#endif
