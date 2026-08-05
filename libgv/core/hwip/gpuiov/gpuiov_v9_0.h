/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GPUIOV_V9_0_H
#define GPUIOV_V9_0_H

#include <amdgv_pci_def.h>
#include <amdgv_sched.h>

enum GPUIOV_V9_0_HW_SCHED_BLOCK{
	GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH0_MMSCH = 0,
	GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH1_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH2_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH3_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH,
	GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH0_RLCV,
	GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH1_RLCV,
	GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH2_RLCV,
	GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH3_RLCV,
	GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH4_RLCV,
	GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH5_RLCV,
	GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH6_RLCV,
	GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH7_RLCV,
	GPUIOV_V9_0_HW_SCHED_BLOCK_NUM,
};

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

#define PCI_GPUIOV_VCN_SCH0_OFFSET	0x58 /* 8bits */
#define PCI_GPUIOV_JPEG_SCH0_OFFSET	0x59 /* 8bits */
#define PCI_GPUIOV_JPEG1_SCH0_OFFSET	0x5a /* 8bits */

#define PCI_GPUIOV_VCN_SCH1_OFFSET	0x5b /* 8bits */
#define PCI_GPUIOV_JPEG_SCH1_OFFSET	0x5c /* 8bits */
#define PCI_GPUIOV_JPEG1_SCH1_OFFSET	0x5d /* 8bits */

#define PCI_GPUIOV_VCN_SCH2_OFFSET	0x5e /* 8bits */
#define PCI_GPUIOV_JPEG_SCH2_OFFSET	0x5f /* 8bits */
#define PCI_GPUIOV_JPEG1_SCH2_OFFSET	0x60 /* 8bits */

#define PCI_GPUIOV_VCN_SCH3_OFFSET	0x61 /* 8bits */
#define PCI_GPUIOV_JPEG_SCH3_OFFSET	0x62 /* 8bits */
#define PCI_GPUIOV_JPEG1_SCH3_OFFSET	0x63 /* 8bits */

#define PCI_GPUIOV_GFX_SCH0_OFFSET	0x64 /* 8bits */
#define PCI_GPUIOV_GFX_SCH1_OFFSET	0x65 /* 8bits */
#define PCI_GPUIOV_GFX_SCH2_OFFSET	0x66 /* 8bits */
#define PCI_GPUIOV_GFX_SCH3_OFFSET	0x67 /* 8bits */
#define PCI_GPUIOV_GFX_SCH4_OFFSET	0x68 /* 8bits */
#define PCI_GPUIOV_GFX_SCH5_OFFSET	0x69 /* 8bits */
#define PCI_GPUIOV_GFX_SCH6_OFFSET	0x6a /* 8bits */
#define PCI_GPUIOV_GFX_SCH7_OFFSET	0x6b /* 8bits */

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

enum gpuiov_v9_0_cmd {
	GPUIOV_V9_0_NO_CMD                      = 0x00,
	GPUIOV_V9_0_IDLE_GPU                    = 0x01,
	GPUIOV_V9_0_SAVE_GPU_STATE              = 0x02,
	GPUIOV_V9_0_LOAD_GPU_STATE              = 0x03,
	GPUIOV_V9_0_RUN_GPU                     = 0x04,
	GPUIOV_V9_0_CONTEXT_SWITCH              = 0x05,
	GPUIOV_V9_0_ENABLE_AUTO_SCHEDULING      = 0x06,
	GPUIOV_V9_0_INIT_GPU	                = 0x07,

	GPUIOV_V9_0_EVENT_NOTIFICATION          = 0x09,
	GPUIOV_V9_0_TRANSFER_VF_DATA            = 0x0A,
	GPUIOV_V9_0_DISABLE_AUTO_SCHEDULING     = 0x0B,

	GPUIOV_V9_0_SHUTDOWN_GPU                = 0x0D,

	GPUIOV_V9_0_CONFIG_AUTO_SCHEDULING      = 0x0F,
	GPUIOV_V9_0_INVALID_COMMAND             = 0x10,
};

enum gpuiov_v9_0_gpuiov_cmd_status {
	GPUIOV_V9_0_IDLE_GPU_STATUS                  = 0x01,
	GPUIOV_V9_0_SAVE_GPU_STATE_STATUS            = 0x02,
	GPUIOV_V9_0_LOAD_GPU_STATE_STATUS            = 0x03,
	GPUIOV_V9_0_RUN_GPU_STATUS                   = 0x04,
	GPUIOV_V9_0_CONTEXT_SWITCH_STATUS            = 0x05,
	GPUIOV_V9_0_ENABLE_AUTO_SCHEDULING_STATUS    = 0x06,
	GPUIOV_V9_0_INIT_GPU_STATUS                  = 0x07,

	GPUIOV_V9_0_EVENT_NOTIFICATION_STATUS        = 0x09,
	GPUIOV_V9_0_TRANSFER_VF_DATA_STATUS          = 0x0A,
	GPUIOV_V9_0_DISABLE_AUTO_SCHEDULING_STATUS   = 0x0B,

	GPUIOV_V9_0_SHUTDOWN_GPU_STATUS              = 0x0D,

	GPUIOV_V9_0_CONFIG_AUTO_SCHEDULING_STATUS    = 0x0F,

	GPUIOV_V9_0_CP_ENGINE_STATUS                 = 0x0100,
	GPUIOV_V9_0_RLCG_ENGINE_STATUS               = 0x0200,
	GPUIOV_V9_0_SDMA_ENGINE_STATUS               = 0x0300,
	GPUIOV_V9_0_HV_CLIENT_STATUS                 = 0x0400,
	GPUIOV_V9_0_CPDMA_ENGINE_STATUS              = 0x0500,
	GPUIOV_V9_0_GENERIC_STATUS                   = 0x0600,

	GPUIOV_V9_0_HANG_DETECT_STATUS               = 0x1000,
	GPUIOV_V9_0_HV_ENGINE_HALT_ERROR_STATUS      = 0x2000,
	GPUIOV_V9_0_INVALID_HV_COMMAND_STATUS        = 0x3000,
	GPUIOV_V9_0_MALFORMED_FN_STATUS              = 0x4000,
	GPUIOV_V9_0_HV_ENGINE_TIMEOUT_STATUS         = 0x5000,
	GPUIOV_V9_0_RLCV_FED_ERROR_STATUS            = 0x8000,
};


/* the size is in units of 256K */
#define CSA_SIZE_PER_VF 1
#define UNIT_256KB	BIT(18)
#define GPUIOV_V9_0_MAX_XCD_NUM	8
#define GPUIOV_V9_0_MAX_VCN_NUM	4
#define GPUIOV_V9_0_SUPPORTED_GFX_SCHED_MODE (BIT(AMDGV_SCHED_FAIRNESS) | BIT(AMDGV_SCHED_ROUND_ROBIN))

struct gpuiov_v9_0_cmd_id {
	const char *name;
	uint32_t    cmd;
	uint32_t    gpuiov_v9_0_cmd;
};

int gpuiov_v9_0_sched_compute_spatial_part_table(struct amdgv_sched_spatial_part *table,
						 uint32_t xcc_mask, uint32_t num_vf);

#endif
