/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE.
 */

#ifndef AMDGV_DIAG_DATA_TRACE_LOG_H
#define AMDGV_DIAG_DATA_TRACE_LOG_H

/* Change this version whenever something changes in tracelog feature/code */
#define AMDGV_RD_HOST_TRACELOG_MAJOR_VERSION		0x01
#define AMDGV_RD_HOST_TRACELOG_MINOR_VERSION		0x00
#define AMDGV_RD_HOST_TRACELOG_VERSION \
		(AMDGV_RD_HOST_TRACELOG_MINOR_VERSION | \
		AMDGV_RD_HOST_TRACELOG_MAJOR_VERSION << 8)

/* Trace Log Client ID */
enum AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_ID {
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_MP1_FW		= 0x50,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_MINI_MP1_FW		= 0x51,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_GFX_IMU		= 0x52,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_MP5_FW		= 0x53,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_DXIO_FW		= 0x60,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_UMC			= 0x70,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_USB			= 0x80,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_SDMA		= 0x81,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_RLCX6		= 0x9D,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_RLCV		= 0x9E,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_RLCG		= 0x9F,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_ACP			= 0xA0,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_BIOS		= 0xB0,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_CP_ME		= 0xC1,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_CP_PFP		= 0xC2,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_CP_MEC		= 0xC3,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_CP_CE		= 0xC4,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_MES_HW_SCH		= 0xC5,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_DRV_KMD		= 0xD1,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_DRV_PPLIB		= 0xD3,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_DRV_DAL		= 0xD4,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_DRV_HOST		= 0xD5,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_APU_PSP_AGESA	= 0xEA,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_PSP_AGESA		= 0xE0,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_APU_PSP_BL1		= 0xED,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_PSP_BL1		= 0xE1,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_APU_PSP_BL2		= 0xEE,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_PSP_BL2		= 0xE2,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_APU_PSP_TOS		= 0xEF,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_PSP_TOS		= 0xE3,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_VCN			= 0xF0,
	AMDGV_DIAG_DATA_TRACE_LOG_CLIENT_DCN			= 0xF1
};

/* Trace Log Feature ID */
enum AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_ID {
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_NOUSED	= 0x00,

	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_ERR,
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_EVENT,
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_WS,
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_GPUIOV,
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_SMU,
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_PSP,
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_CP,
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_DRV,
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_IRQ,
	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_MAILBOX,

	AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_COUNT
};

enum AMDGV_DIAG_DATA_TRACE_MB_DIR {
	AMDGV_DIAG_DATA_TRACE_MB_DIR_PF_TO_VF,
	AMDGV_DIAG_DATA_TRACE_MB_DIR_VF_TO_PF
};

/* <FEATURE START> */
/* <name:Mailbox; type:BITMASK; value:10;> */
/* <name:VF Index; size:5; pos:0;> */
/* <name:Msg ID; size:8; pos:5;> */
/* <IDX MAP> */
	/* <name:Direction> */
	/* <0:AMDGV_DIAG_DATA_TRACE_MB_DIR_PF_TO_VF > */
	/* <1:AMDGV_DIAG_DATA_TRACE_MB_DIR_VF_TO_PF > */
/* <IDX MAP END> */
/* <name:Direction; size:1; pos:13;> */
/* <FEATURE END> */

#define AMDGV_DIAG_DATA_TRACE_MB_CODE(vf, msg, dir) \
	((vf & 0x1F) | ((msg &  0xFF) << 5) | ((dir & 1) << 13))
#define AMDGV_DIAG_DATA_TRACE_LOG_MB(idx_vf, msg_id, dir) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_MAILBOX, \
		AMDGV_DIAG_DATA_TRACE_MB_CODE(idx_vf, msg_id, \
			AMDGV_DIAG_DATA_TRACE_MB_DIR_VF_TO_PF), 0); \
}

/* <FEATURE START> */
/* <name:World Switch; type:BITMASK; value:3;> */
/* <name:VF Index; size:5; pos:0;> */
/* <IDX MAP> */
	/* <name:Sched ID> */
	/* <0:AMDGV_SCHED_BLOCK_GFX > */
	/* <1:AMDGV_SCHED_BLOCK_UVD > */
	/* <2:AMDGV_SCHED_BLOCK_VCE > */
	/* <3:AMDGV_SCHED_BLOCK_UVD1 > */
	/* <4:AMDGV_SCHED_BLOCK_VCN > */
	/* <5:AMDGV_SCHED_BLOCK_VCN1 > */
/* <IDX MAP END> */
/* <name:Sched ID; size:3; pos:5;> */
/* <IDX MAP> */
	/* <name:Current State,Target State> */
	/* <0:Uninitalized > */
	/* <1:AMDGV_IDLE_GPU > */
	/* <2:AMDGV_SAVE_GPU_STATE > */
	/* <3:AMDGV_LOAD_GPU_STATE > */
	/* <4:AMDGV_RUN_GPU > */
	/* <5:AMDGV_CONTEXT_SWITCH > */
	/* <6:AMDGV_ENABLE_AUTO_HW_SWITCH > */
	/* <7:AMDGV_INIT_GPU > */
	/* <8:AMDGV_SAVE_RLCV_STATE > */
	/* <9:AMDGV_LOAD_RLCV_STATE > */
	/* <10:AMDGV_CLEAR_VF_STATE > */
	/* <11:AMDGV_DISABLE_AUTO_HW_SCHED > */
	/* <12:AMDGV_DISABLE_AUTO_HW_SCHED_AND_SWITCH > */
	/* <13:AMDGV_SHUTDOWN_GPU > */
	/* <15:AMDGV_CONFIG_AUTO_HW_SCHED_MODE > */
/* <IDX MAP END> */
/* <name:Current State; size:4; pos:8;> */
/* <name:Target State; size:4; pos:12;> */
/* <FEATURE END> */

#define AMDGV_DIAG_DATA_TRACE_WS_CODE(vf, hw_sched_id, cur_state, target_state) \
	((vf & 0x1F) | ((adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_block &  0x7) << 5) | \
	((cur_state & 0xF) << 8) | ((target_state & 0xF) << 12))

#define AMDGV_DIAG_DATA_ADD_WS_SWITCH_ENTRY(func, hw_sched_id, target_state) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_WS, \
		AMDGV_DIAG_DATA_TRACE_WS_CODE(func, hw_sched_id, \
			adapt->sched.array_vf[func].cur_vf_state[hw_sched_id], \
			target_state), 0); \
}

/* <FEATURE START> */
/* <name:IRQ; type:BITMASK; value:9;> */
/* <name:Client ID; size:6; pos:0;> */
/* <name:Source ID; size:6; pos:6;> */
/* <name:VM ID; size:4; pos:12;> */
/* <name:VF Index; size:5; pos:0; type:extended;> */
/* <IDX MAP> */
	/* <name:Event ID> */
	/* <1:MB_REQ_MSG_REQ_GPU_INIT_ACCESS > */
	/* <2:MB_REQ_MSG_REL_GPU_INIT_ACCESS > */
	/* <3:MB_REQ_MSG_REQ_GPU_FINI_ACCESS > */
	/* <4:MB_REQ_MSG_REL_GPU_FINI_ACCESS > */
	/* <5:MB_REQ_MSG_REQ_GPU_RESET_ACCESS > */
	/* <6:MB_REQ_MSG_REQ_GPU_INIT_DATA > */
	/* <200:MB_REQ_MSG_LOG_VF_ERROR > */
	/* <201:MB_REQ_MSG_READY_TO_RESET > */
/* <IDX MAP END> */
/* <name:Event ID; size:4; pos:5; type:extended;> */
/* <FEATURE END> */
#define AMDGV_DIAG_DATA_TRACE_IRQ_CODE(client_id, src_id, vm_id) \
	((client_id & 0x3F) | ((src_id &  0x3F) << 6) | \
	((vm_id & 0xF) << 12))
#define AMDGV_DIAG_DATA_TRACE_IRQ_EXT_CODE(event_id, vf_id) \
	((vf_id & 0x1F) | ((event_id &  0xFF) << 5))
#define AMDGV_DIAG_DATA_TRACE_LOG_IRQ(client_id, src_id, vm_id, event, idx_vf) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_IRQ, \
		AMDGV_DIAG_DATA_TRACE_IRQ_CODE(client_id, src_id, vm_id), \
				AMDGV_DIAG_DATA_TRACE_IRQ_EXT_CODE(event, idx_vf)); \
}

/* <FEATURE START> */
/* <name:GPUIOV; type:BITMASK; value:4;> */
/* <name:VF Index; size:5; pos:0;> */
/* <IDX MAP> */
	/* <name:Sched ID> */
	/* <0:AMDGV_SCHED_BLOCK_GFX > */
	/* <1:AMDGV_SCHED_BLOCK_UVD > */
	/* <2:AMDGV_SCHED_BLOCK_VCE > */
	/* <3:AMDGV_SCHED_BLOCK_UVD1 > */
	/* <4:AMDGV_SCHED_BLOCK_VCN > */
	/* <5:AMDGV_SCHED_BLOCK_VCN1 > */
/* <IDX MAP END> */
/* <name:Sched ID; size:3; pos:5;> */
/* <IDX MAP> */
	/* <name:Command ID> */
	/* <1:AMDGV_IDLE_GPU > */
	/* <2:AMDGV_SAVE_GPU_STATE > */
	/* <3:AMDGV_LOAD_GPU_STATE > */
	/* <4:AMDGV_RUN_GPU > */
	/* <5:AMDGV_CONTEXT_SWITCH > */
	/* <6:AMDGV_ENABLE_AUTO_HW_SWITCH > */
	/* <7:AMDGV_INIT_GPU > */
	/* <8:AMDGV_SAVE_RLCV_STATE > */
	/* <9:AMDGV_LOAD_RLCV_STATE > */
	/* <10:AMDGV_CLEAR_VF_STATE > */
	/* <11:AMDGV_DISABLE_AUTO_HW_SCHED > */
	/* <12:AMDGV_DISABLE_AUTO_HW_SCHED_AND_SWITCH > */
	/* <13:AMDGV_SHUTDOWN_GPU > */
	/* <15:AMDGV_CONFIG_AUTO_HW_SCHED_MODE > */
/* <IDX MAP END> */
/* <name:Command ID; size:4; pos:8;> */
/* <IDX MAP> */
	/* <name:Command State> */
	/* <1:AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_STATE_START> */
	/* <2:AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_STATE_END> */
/* <IDX MAP END> */
/* <name:Command State; size:4; pos:12;> */
/* <IDX MAP> */
	/* <name:Command Status> */
	/* <0:AMDGV_CMD_STATUS_DONE > */
	/* <1:AMDGV_CMD_STATUS_UNSUPPORTED > */
	/* <2:AMDGV_CMD_STATUS_ABORTED > */
	/* <17:AMDGV_CMD_STATUS_IDLING_ENGINE > */
	/* <18:AMDGV_CMD_STATUS_SAVING_STATE > */
	/* <19:AMDGV_CMD_STATUS_LOADING_STATE > */
	/* <20:AMDGV_CMD_STATUS_ENABLING_ENGINE > */
	/* <21:AMDGV_CMD_STATUS_INITING_ENGINE > */
	/* <255:AMDGV_CMD_STATUS_PENDING_EXECUTE > */
/* <IDX MAP END> */
/* <name:Command Status; size:8; pos:0; type:extended> */
/* <name:Timeout; size:8; pos:8; type:extended> */
/* <FEATURE END> */

#define AMDGV_DIAG_DATA_TRACE_RLC_CODE(vf, hw_sched_id, cmd_id, cmd_state) \
	((vf & 0x1F) | ((adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_block &  0x7) << 5) | \
	((cmd_id & 0xF) << 8) | ((cmd_state & 0xF) << 12))
#define AMDGV_DIAG_DATA_TRACE_RLC_CODE_EXT(cmd_status, timeout) \
	((cmd_status & 0xFF) | ((timeout &  0xFF) << 8))

#define AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_STATE_START	1
#define AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_STATE_END	2
#define AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_END(idx_vf, hw_sched_id, cmd, status, timeout) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_GPUIOV, \
		AMDGV_DIAG_DATA_TRACE_RLC_CODE(idx_vf, hw_sched_id, \
			adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd, \
		AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_STATE_END), \
		AMDGV_DIAG_DATA_TRACE_RLC_CODE_EXT(status, timeout)); \
}

#define AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_START(idx_vf, hw_sched_id, cmd, pci_dword) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_GPUIOV, \
		AMDGV_DIAG_DATA_TRACE_RLC_CODE(idx_vf, hw_sched_id, \
			adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd, \
		AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_STATE_START), \
		pci_dword); \
}

/* <FEATURE START> */
/* <name:Events; type:BITMASK; value:2;> */
/* <name:VF Index; size:5; pos:0;> */
/* <IDX MAP> */
	/* <name:Sched ID> */
	/* <0:AMDGV_SCHED_BLOCK_GFX > */
	/* <1:AMDGV_SCHED_BLOCK_UVD > */
	/* <2:AMDGV_SCHED_BLOCK_VCE > */
	/* <3:AMDGV_SCHED_BLOCK_UVD1 > */
	/* <4:AMDGV_SCHED_BLOCK_VCN > */
	/* <5:AMDGV_SCHED_BLOCK_VCN1 > */
/* <IDX MAP END> */
/* <name:Sched ID; size:3; pos:5;> */
/* <IDX MAP> */
	/* <name:Event ID> */
	/* <0:AMDGV_EVENT_CLR_MSG_BUF > */
	/* <1:AMDGV_EVENT_READY_TO_ACCESS_GPU > */
	/* <2:AMDGV_EVENT_FLR_NOTIFICATION > */
	/* <3:AMDGV_EVENT_FLR_NOTIFICATION_COMPLETION > */
	/* <4:AMDGV_EVENT_IP_DATA_READY > */
	/* <255:AMDGV_EVENT_TEXT_MESSAGE > */
	/* <61184:AMDGV_EVENT_REQ_GPU_INIT > */
	/* <61185:AMDGV_EVENT_REL_GPU_INIT > */
	/* <61186:AMDGV_EVENT_REQ_GPU_FINI > */
	/* <61187:AMDGV_EVENT_REL_GPU_FINI > */
	/* <61188:AMDGV_EVENT_REQ_GPU_RESET > */
	/* <61189:AMDGV_EVENT_LOG_VF_ERROR > */
	/* <61190:AMDGV_EVENT_REQ_GPU_INIT_DATA > */
	/* <61191:AMDGV_EVENT_READY_TO_RESET > */
	/* <65280:AMDGV_EVENT_HYPER_DEFINED_MSG > */
	/* <65281:AMDGV_EVENT_SCHED_FORCE_RESET_VF > */
	/* <65282:AMDGV_EVENT_SCHED_RESET_VF > */
	/* <65283:AMDGV_EVENT_HW_SCHED_RESET_VF > */
	/* <65284:AMDGV_EVENT_SCHED_SUSPEND_VF > */
	/* <65285:AMDGV_EVENT_SCHED_RESUME_VF > */
	/* <65286:AMDGV_EVENT_SCHED_REMOVE_VF > */
	/* <65287:AMDGV_EVENT_SCHED_FORCE_RESET_GPU > */
	/* <65288:AMDGV_EVENT_SCHED_STOP_VF > */
	/* <65289:AMDGV_EVENT_SCHED_INIT_VF_FB > */
	/* <65290:AMDGV_EVENT_SCHED_RAS_UMC > */
	/* <65291:AMDGV_EVENT_SCHED_SUSPEND > */
	/* <65292:AMDGV_EVENT_SCHED_RESUME > */
	/* <65293:AMDGV_EVENT_SCHED_FW_LIVE_UPDATE_DFC > */
	/* <65294:AMDGV_EVENT_SCHED_GPUMON > */
	/* <65295:AMDGV_EVENT_SCHED_SET_VF_ACCESS > */
	/* <65312:AMDGV_EVENT_SCHED_SUSPEND_LIVE > */
	/* <65313:AMDGV_EVENT_SCHED_RESUME_LIVE > */
	/* <65344:AMDGV_EVENT_COLLECT_DIAG_DATA > */
/* <IDX MAP END> */
/* <name:Event ID; size:8; pos:0; type:extended> */
/* <FEATURE END> */
#define AMDGV_DIAG_DATA_TRACE_EVENT_CODE(vf, sched_block) \
	((vf & 0x1F) | ((sched_block &  0x7) << 5))
#define AMDGV_DIAG_DATA_TRACE_LOG_EVENT(idx_vf, sched_block, event_id) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_EVENT, \
		AMDGV_DIAG_DATA_TRACE_EVENT_CODE(idx_vf, sched_block), \
		event->id); \
}

/* <FEATURE START> */
/* <name:SMU; type:BITMASK; value:5;> */
/* <IDX MAP> */
	/* <name:Op> */
	/* <0:AMDGV_DIAG_DATA_SMU_WRITE_MSG_START > */
	/* <1:AMDGV_DIAG_DATA_SMU_WRITE_MSG_END > */
	/* <2:AMDGV_DIAG_DATA_SMU_WRITE_ARG_START > */
	/* <3:AMDGV_DIAG_DATA_SMU_WRITE_ARG_END > */
	/* <4:AMDGV_DIAG_DATA_SMU_READ_MSG > */
	/* <5:AMDGV_DIAG_DATA_SMU_READ_ARG > */
/* <IDX MAP END> */
/* <name:Op; size:2; pos:0;> */
/* <name:Timeout; size:1; pos:2;> */
/* <name:Reg; size:13; pos:3;> */
/* <name:Data; size:32; pos:0; type:extended> */
/* <FEATURE END> */
#define AMDGV_DIAG_DATA_SMU_WRITE_MSG_START	0
#define AMDGV_DIAG_DATA_SMU_WRITE_MSG_END	1
#define AMDGV_DIAG_DATA_SMU_WRITE_ARG_START	2
#define AMDGV_DIAG_DATA_SMU_WRITE_ARG_END	3
#define AMDGV_DIAG_DATA_SMU_READ_RESP	4
#define AMDGV_DIAG_DATA_SMU_READ_ARG		5

#define AMDGV_DIAG_DATA_TRACE_SMU_CODE(op, timeout, reg_offset) \
	((op & 0x3) | ((timeout & 0x1) << 2) | \
	((reg_offset &  0x1FF) << 3))

#define AMDGV_DIAG_DATA_TRACE_LOG_SMU(op, timeout, reg_offset, ext_code) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_SMU, \
		AMDGV_DIAG_DATA_TRACE_SMU_CODE(op, timeout, reg_offset), \
		ext_code); \
}

/* <FEATURE START> */
/* <name:PSP; type:BITMASK; value:6;> */
/* <IDX MAP> */
	/* <name:Command Type> */
	/* <0:PSP_CMD_KM_TYPE__INVALID > */
	/* <1:PSP_CMD_KM_TYPE__LOAD_TA > */
	/* <2:PSP_CMD_KM_TYPE__UNLOAD_TA > */
	/* <3:PSP_CMD_KM_TYPE__INVOKE_CMD > */
	/* <4:PSP_CMD_KM_TYPE__LOAD_ASD > */
	/* <5:PSP_CMD_KM_TYPE__SETUP_TMR > */
	/* <6:PSP_CMD_KM_TYPE__LOAD_IP_FW > */
	/* <7:PSP_CMD_KM_TYPE__DESTROY_TMR > */
	/* <11:PSP_CMD_KM_TYPE__GBR_IH_REG > */
	/* <12:PSP_CMD_KM_TYPE__VFGATE > */
	/* <13:PSP_CMD_KM_TYPE__CLEAR_VF_FW > */
	/* <14:PSP_CMD_KM_TYPE__NUM_ENABLED_VFS > */
	/* <15:PSP_CMD_KM_TYPE__FW_ATTESTATION > */
	/* <16:PSP_CMD_KM_TYPE__DBG_SNAPSHOT_SET_ADDR > */
	/* <17:PSP_CMD_KM_TYPE__DBG_SNAPSHOT_TRIGGER > */
	/* <19:PSP_CMD_KM_TYPE__DUMP_TRACELOG > */
	/* <32:PSP_CMD_KM_TYPE__LOAD_TOC > */
	/* <33:PSP_CMD_KM_TYPE__AUTOLOAD_RLC > */
/* <IDX MAP END> */
/* <name:Command Type; size:8; pos:0;> */
/* <IDX MAP> */
	/* <name:Command State> */
	/* <0:AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_START> */
	/* <1:AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_PARAM> */
	/* <2:AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_WAIT> */
	/* <3:AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_END> */
/* <IDX MAP END> */
/* <name:Command State; size:8; pos:8;> */
/* <FEATURE END> */
#define AMDGV_DIAG_DATA_TRACE_PSP_CODE(cmd, cmd_state) \
	((cmd & 0xFF) | ((cmd_state & 0xFF) << 8))

#define AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_START	0
#define AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_PARAM	1
#define AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_WAIT		2
#define AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_END		3

#define AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_PARAM_LEN		24

#define AMDGV_DIAG_DATA_TRACE_PSP_CODE(cmd, cmd_state) \
	((cmd & 0xFF) | ((cmd_state & 0xFF) << 8))

#define AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_START(cmd) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_PSP, \
		AMDGV_DIAG_DATA_TRACE_PSP_CODE(cmd, \
		AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_START), \
		0); \
}

#define AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_PARAM(cmd, param) { \
	int i = 0; \
	uint32_t *param_ref = (uint32_t *)&param; \
	for (i = 0; i < AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_PARAM_LEN; \
			i += sizeof(*param_ref)) { \
		amdgv_diag_data_add_trace_log(adapt, \
			AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_PSP, \
			AMDGV_DIAG_DATA_TRACE_PSP_CODE(cmd, \
			AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_PARAM), \
			*param_ref++); \
	} \
}

#define AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_WAIT(cmd, fence_count) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_PSP, \
		AMDGV_DIAG_DATA_TRACE_PSP_CODE(cmd, \
		AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_WAIT), \
		fence_count); \
}

#define AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_FINISH(cmd, status, fence_status) { \
	amdgv_diag_data_add_trace_log(adapt, \
		AMDGV_DIAG_DATA_TRACE_LOG_DRV_HOST_FEATURE_PSP, \
		AMDGV_DIAG_DATA_TRACE_PSP_CODE(cmd, \
		AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_STATE_END), \
		((status & 0xFFFF) | ((fence_status & 0xFFFF) << 16))); \
}


#endif
