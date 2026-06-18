/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NAVI32_GPUIOV_H
#define NAVI32_GPUIOV_H

#include <amdgv_pci_def.h>

enum navi32_hw_sched_block {
	NAVI32_HW_SCHED_BLOCK_VCN_SCH0_MMSCH = 0,
	NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV,
	NAVI32_HW_SCHED_BLOCK_VCN1_SCH1_MMSCH,
	NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH,
	NAVI32_HW_SCHED_BLOCK_NUM,
};

#define CMD_EXECUTE (0x10)

#define PCI_GPUIOV_FUNC_ID(idx_vf)                                                            \
	((idx_vf == AMDGV_PF_IDX || idx_vf >= adapt->max_num_vf) ? 0 : (0x80 | (idx_vf & 0x1F)))

#define PCI_GPUIOV_CAP		       0x02 /* 16bits */
#define PCI_GPUIOV_CAP__VER(x)	       ((x)&0xf)
#define PCI_GPUIOV_NEXT_CAP__OFFSET(x) ((x) >> 4)

#define PCI_GPUIOV_VSEC		     0x04 /* 32bits*/
#define PCI_GPUIOV_VSEC__ID__GPU_IOV 0x02
#define PCI_GPUIOV_VSEC__ID(x)	     ((x)&0xffff)
#define PCI_GPUIOV_VSEC__REV(x)	     (((x) >> 16) & 0x0f)
#define PCI_GPUIOV_VSEC__LENGTH(x)   ((x) >> 20)

#define PCI_GPUIOV_CTRL		 0x08 /* 32bits*/
#define PCI_GPUIOV_INTR_ENABLE	 0x0c /* 32bits */
#define PCI_GPUIOV_INTR_STATUS	 0x10 /* 32bits */
#define PCI_GPUIOV_RESET_CONTROL 0x14 /* 8bits */
#define PCI_GPUIOV_HVVM_MBOX0	 0x18 /* 32bits */
#define PCI_GPUIOV_HVVM_MBOX1	 0x1c /* 32bits */
#define PCI_GPUIOV_HVVM_MBOX2	 0x20 /* 32bits */

#define PCI_GPUIOV_ENG_A_INTR_ENABLE	0x200 /* 4bits */
#define PCI_GPUIOV_ENG_B_INTR_ENABLE	0x208 /* 8bits */
#define PCI_GPUIOV_ENG_A_INTR_STATUS	0x210 /* 4bits */
#define PCI_GPUIOV_ENG_B_INTR_STATUS	0x218 /* 8bits */

#define PCI_GPUIOV_CNTXT		0x24 /* 32bits */
#define PCI_GPUIOV_CNTXT__OFFSET__MASK	0x3fffff
#define PCI_GPUIOV_CNTXT__OFFSET__SHIFT 10
#define PCI_GPUIOV_CNTXT__LOC__MASK	0x1
#define PCI_GPUIOV_CNTXT__LOC__SHIFT	7
#define PCI_GPUIOV_CNTXT__SIZE__MASK	0x7f
#define PCI_GPUIOV_CNTXT__SIZE__SHIFT	0

#define PCI_GPUIOV_TOTAL_FB_AVAILABLE 0x28 /* 16bits */
#define PCI_GPUIOV_TOTAL_FB_CONSUMED  0x2a /* 16bits */

#define PCI_GPUIOV_UVD0SCH_OFFSET 0x2c /* 8bits */
#define PCI_GPUIOV_VCESCH_OFFSET  0x2d /* 8bits */
#define PCI_GPUIOV_GFXSCH_OFFSET  0x2e /* 8bits */
#define PCI_GPUIOV_UVD1SCH_OFFSET 0x2f /* 8bits */

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

#define PCI_GPUIOV_VF8_FB_SIZE	 0x58 /* 16bits */
#define PCI_GPUIOV_VF8_FB_OFFSET 0x5a /* 16bits */

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

#define PCI_GPUIOV_VF16_FB_SIZE	  0x78 /* 16bits */
#define PCI_GPUIOV_VF16_FB_OFFSET 0x7a /* 16bits */

#define PCI_GPUIOV_VF17_FB_SIZE	  0x7c /* 16bits */
#define PCI_GPUIOV_VF17_FB_OFFSET 0x7e /* 16bits */

#define PCI_GPUIOV_VF18_FB_SIZE	  0x80 /* 16bits */
#define PCI_GPUIOV_VF18_FB_OFFSET 0x82 /* 16bits */

#define PCI_GPUIOV_VF19_FB_SIZE	  0x84 /* 16bits */
#define PCI_GPUIOV_VF19_FB_OFFSET 0x86 /* 16bits */

#define PCI_GPUIOV_VF20_FB_SIZE	  0x88 /* 16bits */
#define PCI_GPUIOV_VF20_FB_OFFSET 0x8a /* 16bits */

#define PCI_GPUIOV_VF21_FB_SIZE	  0x8c /* 16bits */
#define PCI_GPUIOV_VF21_FB_OFFSET 0x8e /* 16bits */

#define PCI_GPUIOV_VF22_FB_SIZE	  0x90 /* 16bits */
#define PCI_GPUIOV_VF22_FB_OFFSET 0x92 /* 16bits */

#define PCI_GPUIOV_VF23_FB_SIZE	  0x94 /* 16bits */
#define PCI_GPUIOV_VF23_FB_OFFSET 0x96 /* 16bits */

#define PCI_GPUIOV_VF24_FB_SIZE	  0x98 /* 16bits */
#define PCI_GPUIOV_VF24_FB_OFFSET 0x9a /* 16bits */

#define PCI_GPUIOV_VF25_FB_SIZE	  0x9c /* 16bits */
#define PCI_GPUIOV_VF25_FB_OFFSET 0x9e /* 16bits */

#define PCI_GPUIOV_VF26_FB_SIZE	  0xa0 /* 16bits */
#define PCI_GPUIOV_VF26_FB_OFFSET 0xa2 /* 16bits */

#define PCI_GPUIOV_VF27_FB_SIZE	  0xa4 /* 16bits */
#define PCI_GPUIOV_VF27_FB_OFFSET 0xa6 /* 16bits */

#define PCI_GPUIOV_VF28_FB_SIZE	  0xa8 /* 16bits */
#define PCI_GPUIOV_VF28_FB_OFFSET 0xaa /* 16bits */

#define PCI_GPUIOV_VF29_FB_SIZE	  0xac /* 16bits */
#define PCI_GPUIOV_VF29_FB_OFFSET 0xae /* 16bits */

#define PCI_GPUIOV_VF30_FB_SIZE	  0xb0 /* 16bits */
#define PCI_GPUIOV_VF30_FB_OFFSET 0xb2 /* 16bits */

#define REG_GPUIOV_VF_STATUS_MASK 0xf00 /* 32bits */

/* SCH */
#define PCI_SCH_CMD_CONTROL 0x04 /* 8bits */
#define PCI_SCH_FCN_ID	    0x05 /* 8bits */
#define PCI_SCH_NXT_FCN_ID  0x06 /* 8bits */

#define PCI_SCH_CMD_STATUS 0x08 /* 8bits */

#define PCI_SCH_VM_BUSY_STATUS	 0x0c /* 32bits */
#define PCI_SCH_ACTIVE_FUNCTIONS 0x10 /* 32bits */

#define PCI_SCH_ACTIVE_FUNCTION_ID        0x14 /* 8bits for mmsched, 32bit for gfx */
#define PCI_SCH_ACTIVE_FUNCTION_ID_STATUS 0x15 /* 4bits for mmsched */

#define PCI_SCH_TIME_QUANTA_OPTION		0x18 /* 32bits */
#define PCI_SCH_TIME_QUANTA_INDEX(idx_vf)	((idx_vf <= 15) ? 0x1c : 0x20)
#define PCI_SCH_TIME_QUANTA_INDEX_SHIFT(idx_vf) ((idx_vf % 16) * 2)

/* asymmetric mode mask */
#define NAVI32_ASYMMETRIC_TIME_SLICE_MASK  0x3FFFFFFF

#define NAVI32_JPEG_HANG_NEED_FLR_INTR (1 << 8)

enum navi32_gpuiov_cmd_lx7 {
	NAVI32_NO_CMD                      = 0x0,
	NAVI32_IDLE_GPU_LX7                = 0x01,
	NAVI32_SAVE_GPU_STATE_LX7          = 0x02,
	NAVI32_LOAD_GPU_STATE_LX7          = 0x03,
	NAVI32_RUN_GPU_LX7                 = 0x04,
	NAVI32_CONTEXT_SWITCH_LX7          = 0x05,
	NAVI32_ENABLE_AUTO_SCHEDULING      = 0x06,
	NAVI32_INIT_GPU_LX7	               = 0x07,

	NAVI32_EVENT_NOTIFICATION          = 0x09,
	NAVI32_TRANSFER_VF_DATA            = 0x0A,
	NAVI32_DISABLE_HW_AUTO_SCHEDULING  = 0x0B,

	NAVI32_SHUTDOWN_GPU_LX7            = 0x0D,

	NAVI32_CONFIG_SCHEDULER_FEATURE    = 0x0F,
	NAVI32_INVALID_COMMAND             = 0x10,
};

enum navi32_gpuiv_cmd_status_lx7 {
	NAVI32_IDLE_GPU_STATUS_LX7                  = 0x01,
	NAVI32_SAVE_GPU_STATE_STATUS_LX7            = 0x02,
	NAVI32_LOAD_GPU_STATE_STATUS_LX7            = 0x03,
	NAVI32_RUN_GPU_STATUS_LX7                   = 0x04,
	NAVI32_CONTEXT_SWITCH_STATUS_LX7            = 0x05,
	NAVI32_ENABLE_AUTO_SCHEDULING_STATUS        = 0x06,
	NAVI32_INIT_GPU_STATUS_LX7                  = 0x07,

	NAVI32_EVENT_NOTIFICATION_STATUS            = 0x09,
	NAVI32_TRANSFER_VF_DATA_STATUS              = 0x0A,
	NAVI32_DISABLE_HW_AUTO_SCHEDULING_STATUS    = 0x0B,

	NAVI32_SHUTDOWN_GPU_STATUS_LX7              = 0x0D,

	NAVI32_CONFIG_SCHEDULER_FEATURE_STATUS      = 0x0F,
};

struct navi32_cmd_id {
	uint32_t    cmd;
	uint32_t    navi32_cmd;
	const char *name;
};

void navi32_dump_gpuiov_cmd_status(struct amdgv_adapter *adapt,
				  uint32_t hw_sched_id);

int navi32_gpuiov_rcv_hvvm_mbox_msg(struct amdgv_adapter *adapt, uint8_t *msg_data);

int navi32_gpuiov_trn_hvvm_mbox_data(struct amdgv_adapter *adapt, uint8_t msg_data);

int navi32_gpuiov_set_hvvm_mbox_valid(struct amdgv_adapter *adapt, uint8_t bits);

int navi32_gpuiov_set_hvvm_mbox_ack(struct amdgv_adapter *adapt);

#endif
