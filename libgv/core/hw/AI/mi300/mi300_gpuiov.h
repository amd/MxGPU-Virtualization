/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#ifndef MI300_GPUIOV_H
#define MI300_GPUIOV_H

#include <amdgv_pci_def.h>

enum mi300_hw_sched_block {
	MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH = 0,
	MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH,
	MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH,
	MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH,
	MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH,
	MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH,
	MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH,
	MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH,
	MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH,
	MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH,
	MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH,
	MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH,
	MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV,
	MI300_HW_SCHED_BLOCK_GFX_SCH1_RLCV,
	MI300_HW_SCHED_BLOCK_GFX_SCH2_RLCV,
	MI300_HW_SCHED_BLOCK_GFX_SCH3_RLCV,
	MI300_HW_SCHED_BLOCK_GFX_SCH4_RLCV,
	MI300_HW_SCHED_BLOCK_GFX_SCH5_RLCV,
	MI300_HW_SCHED_BLOCK_GFX_SCH6_RLCV,
	MI300_HW_SCHED_BLOCK_GFX_SCH7_RLCV,
	MI300_HW_SCHED_BLOCK_NUM,
};

#define MI300_XCC_SCHED_MASK                                                                  \
	((1 << MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV) |                                          \
	 (1 << MI300_HW_SCHED_BLOCK_GFX_SCH1_RLCV) |                                          \
	 (1 << MI300_HW_SCHED_BLOCK_GFX_SCH2_RLCV) |                                          \
	 (1 << MI300_HW_SCHED_BLOCK_GFX_SCH3_RLCV) |                                          \
	 (1 << MI300_HW_SCHED_BLOCK_GFX_SCH4_RLCV) |                                          \
	 (1 << MI300_HW_SCHED_BLOCK_GFX_SCH5_RLCV) |                                          \
	 (1 << MI300_HW_SCHED_BLOCK_GFX_SCH6_RLCV) |                                          \
	 (1 << MI300_HW_SCHED_BLOCK_GFX_SCH7_RLCV))

#define CMD_EXECUTE (0x10)

#define PCI_GPUIOV_FUNC_ID(idx_vf)                                                            \
	((idx_vf == AMDGV_PF_IDX || idx_vf >= adapt->max_num_vf) ? 0 :                        \
								   (0x80 | (idx_vf & 0x0F)))

#define PCI_GPUIOV_CAP		       0x02 /* 16bits */
#define PCI_GPUIOV_CAP__VER(x)	       ((x)&0xf)
#define PCI_GPUIOV_NEXT_CAP__OFFSET(x) ((x) >> 4)

#define PCI_GPUIOV_VSEC		     0x04 /* 32bits*/
#define PCI_GPUIOV_VSEC__ID__GPU_IOV 0x02
#define PCI_GPUIOV_VSEC__ID(x)	     ((x)&0xffff)
#define PCI_GPUIOV_VSEC__REV(x)	     (((x) >> 16) & 0x0f)
#define PCI_GPUIOV_VSEC__LENGTH(x)   ((x) >> 20)

#define PCI_GPUIOV_HVM_MAILBOX_EN     0x0c /* 2bits */
#define PCI_GPUIOV_HVM_MAILBOX_STATUS 0x10 /* 2bits */

#define PCI_GPUIOV_RESET_CONTROL 0x14 /* 8bits */

#define PCI_GPUIOV_HVVM_MBOX0 0x18 /* 32bits */
#define PCI_GPUIOV_HVVM_MBOX1 0x1c /* 32bits */
#define PCI_GPUIOV_HVVM_MBOX2 0x20 /* 32bits */

#define PCI_GPUIOV_CNTXT	    0x24 /* 32bits */
#define PCI_GPUIOV_CNTXT__SIZE(x)   ((x)&0x7f)
#define PCI_GPUIOV_CNTXT__LOC(x)    (((x) >> 7) & 0x01)
#define PCI_GPUIOV_CNTXT__OFFSET(x) ((x) >> 10)

#define PCI_GPUIOV_CNTXT__LOC_IN_FB  0
#define PCI_GPUIOV_CNTXT__LOC_IN_SYS 1

#define PCI_GPUIOV_CNTXT__SIZE__PUT(x)	 ((x)&0x7f)
#define PCI_GPUIOV_CNTXT__LOC__PUT(x)	 (((x)&0x01) << 7)
#define PCI_GPUIOV_CNTXT__OFFSET__PUT(x) (((x)&0x3fffff) << 10)

#define PCI_GPUIOV_INTR_ENABLE 0x0c /* 16bits */
#define PCI_GPUIOV_INTR_STATUS 0x10 /* 16bits */

#define PCI_GPUIOV_TOTAL_FB_AVAILABLE 0x28 /* 16bits */
#define PCI_GPUIOV_TOTAL_FB_CONSUMED  0x2a /* 16bits */

#define PCI_GPUIOV_REGION 0x30 /* 32bits */

#define PCI_GPUIOV_P2P_OVER_XGMI_ENABLE 0x34 /* 32bits */

#define PCI_GPUIOV_VF0_FB_SIZE	 0x38 /* 16bits */
#define PCI_GPUIOV_VF0_FB_OFFSET 0x3a /* 16bits */

#define PCI_GPUIOV_VF1_FB_SIZE	 0x3c /* 16bits */
#define PCI_GPUIOV_VF1_FB_OFFSET 0x3e /* 16bits */

#define PCI_GPUIOV_VF2_FB_SIZE	 0x40 /* 16bits */
#define PCI_GPUIOV_VF2_FB_OFFSET 0x42 /* 16bits */

#define PCI_GPUIOV_VF3_FB_SIZE	 0x44 /* 16bits */
#define PCI_GPUIOV_VF3_FB_OFFSET 0x46 /* 16bits */

#define PCI_GPUIOV_VF4_FB_SIZE	 0x48 /* 16bits */
#define PCI_GPUIOV_VF4_FB_OFFSET 0x4a /* 16bits */

#define PCI_GPUIOV_VF5_FB_SIZE	 0x4c /* 16bits */
#define PCI_GPUIOV_VF5_FB_OFFSET 0x4e /* 16bits */

#define PCI_GPUIOV_VF6_FB_SIZE	 0x50 /* 16bits */
#define PCI_GPUIOV_VF6_FB_OFFSET 0x52 /* 16bits */

#define PCI_GPUIOV_VF7_FB_SIZE	 0x54 /* 16bits */
#define PCI_GPUIOV_VF7_FB_OFFSET 0x56 /* 16bits */

#define PCI_GPUIOV_VF9_FB_SIZE	 0x5c /* 16bits */
#define PCI_GPUIOV_VF9_FB_OFFSET 0x5e /* 16bits */

#define PCI_GPUIOV_VF10_FB_SIZE	  0x60 /* 16bits */
#define PCI_GPUIOV_VF10_FB_OFFSET 0x62 /* 16bits */

#define PCI_GPUIOV_VF11_FB_SIZE	  0x64 /* 16bits */
#define PCI_GPUIOV_VF11_FB_OFFSET 0x66 /* 16bits */

#define PCI_GPUIOV_VF12_FB_SIZE	  0x68 /* 16bits */
#define PCI_GPUIOV_VF12_FB_OFFSET 0x6a /* 16bits */

#define PCI_GPUIOV_VF13_FB_SIZE	  0x6c /* 16bits */
#define PCI_GPUIOV_VF13_FB_OFFSET 0x6e /* 16bits */

#define PCI_GPUIOV_VF14_FB_SIZE	  0x70 /* 16bits */
#define PCI_GPUIOV_VF14_FB_OFFSET 0x72 /* 16bits */

#define PCI_GPUIOV_VF15_FB_SIZE	  0x74 /* 16bits */
#define PCI_GPUIOV_VF15_FB_OFFSET 0x76 /* 16bits */

#define PCI_GPUIOV_VCN_SCH0_OFFSET 0xc0 /* 8bits */
#define PCI_GPUIOV_VCN_SCH1_OFFSET 0xc3 /* 8bits */
#define PCI_GPUIOV_VCN_SCH2_OFFSET 0xc6 /* 8bits */
#define PCI_GPUIOV_VCN_SCH3_OFFSET 0xc9 /* 8bits */

#define PCI_GPUIOV_JPEG_SCH0_OFFSET 0xc1 /* 8bits */
#define PCI_GPUIOV_JPEG_SCH1_OFFSET 0xc4 /* 8bits */
#define PCI_GPUIOV_JPEG_SCH2_OFFSET 0xc7 /* 8bits */
#define PCI_GPUIOV_JPEG_SCH3_OFFSET 0xca /* 8bits */

#define PCI_GPUIOV_JPEG1_SCH0_OFFSET 0xc2 /* 8bits */
#define PCI_GPUIOV_JPEG1_SCH1_OFFSET 0xc5 /* 8bits */
#define PCI_GPUIOV_JPEG1_SCH2_OFFSET 0xc8 /* 8bits */
#define PCI_GPUIOV_JPEG1_SCH3_OFFSET 0xcb /* 8bits */

#define PCI_GPUIOV_GFX_SCH0_OFFSET 0xcc /* 8bits */
#define PCI_GPUIOV_GFX_SCH1_OFFSET 0xcd /* 8bits */
#define PCI_GPUIOV_GFX_SCH2_OFFSET 0xce /* 8bits */
#define PCI_GPUIOV_GFX_SCH3_OFFSET 0xcf /* 8bits */
#define PCI_GPUIOV_GFX_SCH4_OFFSET 0xd0 /* 8bits */
#define PCI_GPUIOV_GFX_SCH5_OFFSET 0xd1 /* 8bits */
#define PCI_GPUIOV_GFX_SCH6_OFFSET 0xd2 /* 8bits */
#define PCI_GPUIOV_GFX_SCH7_OFFSET 0xd3 /* 8bits */

/* SCH */
#define PCI_SCH_CMD_CONTROL 0x04 /* 8bits */
#define PCI_SCH_FCN_ID	    0x05 /* 8bits */
#define PCI_SCH_NXT_FCN_ID  0x06 /* 8bits */

#define PCI_SCH_CMD_STATUS 0x08 /* 8bits */

#define PCI_SCH_VM_BUSY_STATUS	 0x0c /* 32bits */
#define PCI_SCH_ACTIVE_FUNCTIONS 0x10 /* 32bits */

#define PCI_SCH_ACTIVE_FUNCTION_ID	  0x14 /* 16bits */
#define PCI_SCH_ACTIVE_FUNCTION_ID_STATUS 0x16 /* 16bits */

#define PCI_SCH_TIME_QUANTA_OPTION 0x18 /* 32bits */

#define PCI_SCH_TIME_QUANTA_PER_VF 0x1c /* 32bits */

#define PCI_SCH_TIME_QUANTA_PF	       0x20		   /* 32bits */
#define PCI_SCH_TIME_QUANTA_PF__GET(x) (((x) >> 30) & 0x3) //[31:30]
#define PCI_SCH_TIME_QUANTA_PF__SET(x) (((x)&0x3) << 30)   //[31:30]

int mi300_gpuiov_rcv_hvvm_mbox_msg(struct amdgv_adapter *adapt, uint8_t *msg_data);
int mi300_gpuiov_trn_hvvm_mbox_data(struct amdgv_adapter *adapt, uint8_t msg_data);
int mi300_gpuiov_set_hvvm_mbox_valid(struct amdgv_adapter *adapt, uint8_t bits);
int mi300_gpuiov_set_hvvm_mbox_ack(struct amdgv_adapter *adapt);

#endif
