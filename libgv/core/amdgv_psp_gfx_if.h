/*
 * Copyright (c) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_PSP_GFX_IF_H
#define AMDGV_PSP_GFX_IF_H

#include "ta_ras_if.h"
#include "ta_xgmi_if.h"

/* Max number of descriptors for one shared buffer (in how many different
 *  physical locations one shared buffer can be stored). If buffer is too
 *  fragmented, error will be returned.
 */
#define GFX_BUF_MAX_DESC 64

#define GFX_CMD_TEE_VERSION_MASK 0x03C00000
#define GFX_CMD_TEE_VERSION_SHIFT 22

enum { PSP_TMR_ALIGNMENT = 0x100000 }; /* 1M */

enum psp_cmd_km_type {
	PSP_CMD_KM_TYPE__INVALID	 = 0, /* invalid */
	PSP_CMD_KM_TYPE__LOAD_TA	 = 1, /* load TA */
	PSP_CMD_KM_TYPE__UNLOAD_TA	 = 2, /* unload TA */
	PSP_CMD_KM_TYPE__INVOKE_CMD	 = 3, /* send command to TA */
	PSP_CMD_KM_TYPE__LOAD_ASD	 = 4, /* load ASD Driver */
	PSP_CMD_KM_TYPE__SETUP_TMR	 = 5, /* setup TMR region */
	PSP_CMD_KM_TYPE__LOAD_IP_FW	 = 6, /* load HW IP FW */
	PSP_CMD_KM_TYPE__DESTROY_TMR	 = 7, /* destroy TMR region */
	PSP_CMD_KM_TYPE__GBR_IH_REG	 = 0xB, /* Program Register */
	PSP_CMD_KM_TYPE__VFGATE		 = 0xC, /* VF gate */
	PSP_CMD_KM_TYPE__CLEAR_VF_FW = 0xD, /* Clear VF FW */
	PSP_CMD_KM_TYPE__NUM_ENABLED_VFS = 0xE, /* num_available_vfs */
	PSP_CMD_KM_TYPE__FW_ATTESTATION  = 0xF, /* get fw attestation db address*/
	PSP_CMD_KM_TYPE__DBG_SNAPSHOT_SET_ADDR  = 0x10,
	PSP_CMD_KM_TYPE__DBG_SNAPSHOT_TRIGGER   = 0x11,
	PSP_CMD_KM_TYPE__DUMP_TRACELOG   = 0x13, /* dump tracelog log */
	PSP_CMD_KM_TYPE__LOAD_TOC	 = 0x00000020, /* load TOC */
	PSP_CMD_KM_TYPE__AUTOLOAD_RLC	 = 0x00000021, /* Start Autoload */
	PSP_CMD_KM_TYPE__SRIOV_SPATIAL_PART = 0x00000022,
	PSP_CMD_KMD_TYPE_SRIOV_COPY_VF_CHIPLET_REGS = 0x00000023, /* Copy VF Regs values to master IOD */
	PSP_CMD_KM_TYPE__MIGRATION_GET_PSP_INFO	= 0x25,
	PSP_CMD_KM_TYPE__MIGRATION_EXPORT		= 0x26,
	PSP_CMD_KM_TYPE__MIGRATION_IMPORT		= 0x27,
	PSP_CMD_KM_TYPE__VF_RELAY        = 0x00000045, /* PSP VF Command Replay*/
	PSP_CMD_KM_TYPE__NPS_MODE = 0x00000048,
};

/* TEE Gfx Command IDs for the ring buffer interface. */
enum psp_gfx_cmd_id {
	GFX_CMD_ID_LOAD_TA	    = 0x00000001, /* load TA */
	GFX_CMD_ID_UNLOAD_TA	    = 0x00000002, /* unload TA */
	GFX_CMD_ID_INVOKE_CMD	    = 0x00000003, /* send command to TA */
	GFX_CMD_ID_LOAD_ASD	    = 0x00000004, /* load ASD Driver */
	GFX_CMD_ID_SETUP_TMR	    = 0x00000005, /* setup TMR region */
	GFX_CMD_ID_LOAD_IP_FW	    = 0x00000006, /* load HW IP FW */
	GFX_CMD_ID_DESTROY_TMR	    = 0x00000007, /* destroy TMR region */
	GFX_CMD_ID_SAVE_RESTORE	    = 0x00000008, /* save/restore HW IP FW */
	GFX_CMD_ID_SETUP_VMR	    = 0x00000009, /* setup VMR region */
	GFX_CMD_ID_DESTROY_VMR	    = 0x0000000A, /* destroy VMR region */
	GFX_CMD_ID_GBR_IH_REG	    = 0x0000000B, /* set Gbr IH RB CNTL regs*/
	GFX_CMD_ID_VFGATE	    = 0x0000000C,
	GFX_CMD_ID_CLEAR_VF_FW      = 0x0000000D, /* Clear VF FW */
	GFX_CMD_ID_NUM_ENABLED_VFS  = 0x0000000E, /* set num_available_vfs */
	GFX_CMD_ID_FW_ATTESTATION   = 0x0000000F, /* get fw attestation db address */
	GFX_CMD_ID_DBG_SNAPSHOT_SET_ADDR = 0x00000010, /* set soc snapshot memory dump addr*/
	GFX_CMD_ID_DBG_SNAPSHOT_TRIGGER  = 0x00000011, /* trigger soc snapshot memory dump */
	GFX_CMD_ID_DUMP_TRACELOG    = 0x00000013, /* dump tracelog log */
	// IDs upto 0x1F are reserved for older programs
	// which are build from 10-RV branch in Perforce.
	//
	GFX_CMD_ID_LOAD_TOC	    = 0x00000020, /* Load TOC and obtain TMR size */
	GFX_CMD_ID_AUTOLOAD_RLC     = 0x00000021, /* Indicates all graphics fw loaded, start RLC
						     autoload */
	GFX_CMD_ID_PROC_DRAM_PARAMS = 0x00000022, /* GDDR6 training params control (Navi10/14
						     dGPU specific) */
	GFX_CMD_ID_SETUP_VM_ADDRESS = 0x00000023, /* Set up VM default address (Renoir
						     specific)*/
	GFX_CMD_ID_SRIOV_SPATIAL_PART = 0x00000027, /* Configure for spatial partitioning mode */
	GFX_CMD_ID_SRIOV_COPY_VF_CHIPLET_REGS = 0x00000029, /* Copy VF Regs values to master IOD */
	GFX_CMD_ID_MIGRATION_GET_PSP_INFO	= 0x00000031,
	GFX_CMD_ID_MIGRATION_EXPORT			= 0x00000032,
	GFX_CMD_ID_MIGRATION_IMPORT			= 0x00000033,
	GFX_CMD_ID_VF_RELAY         = 0x00000045, /* Host informs PSP about VF command replay*/
	GFX_CMD_ID_NPS_MODE = 0x00000048, /* Change memory NPS mode on next mode-1 reset */
};

/* TEE Gfx Command IDs for the register interface.
 *  Command ID must be between 0x00010000 and 0x000F0000.
 */
enum psp_gfx_crtl_cmd_id {
	GFX_CTRL_CMD_ID_INIT_RBI_RING       = 0x00010000, /* initialize RBI
								ring */
	GFX_CTRL_CMD_ID_INIT_GPCOM_RING     = 0x00020000, /* initialize
							       GPCOM ring */
	GFX_CTRL_CMD_ID_DESTROY_RINGS       = 0x00030000, /* destroy rings */

	GFX_CTRL_CMD_ID_CAN_INIT_RINGS      = 0x00040000, /* is it allowed to
							initialized the rings */
	GFX_CTRL_CMD_ID_ENABLE_INT          = 0x00050000, /* enable
							PSP-to-Gfx interrupt */
	GFX_CTRL_CMD_ID_DISABLE_INT         = 0x00060000, /* disable
							PSP-to-Gfx interrupt */
	GFX_CTRL_CMD_ID_MODE1_RST           = 0x00070000, /* trigger the
							       Mode 1 reset */
	GFX_CTRL_CMD_ID_GBR_IH_SET          = 0x00080000, /* set Gbr
							IH_RB_CNTL registers */
	GFX_CTRL_CMD_ID_CONSUME_CMD         = 0x00090000, /* send interrupt
				to PSP for SRIOV ring write pointer update */
	GFX_CTRL_CMD_ID_MODE2_RST           = 0x000A0000, /* trigger the
							       Mode 2 reset */
	GFX_CTRL_CMD_ID_DESTROY_RBI_RING    = 0x000B0000, /* destroy RBI
							       ring */
	GFX_CTRL_CMD_ID_DESTROY_GPCOM_RING  = 0x000C0000, /* destroy GPCOM
							       ring */
	GFX_CTRL_CMD_ID_MAX                 = 0x000F0000, /* max command ID */
};

enum psp_gfx_fw_type {
	GFX_FW_TYPE_NONE        = 0,    /* */
	GFX_FW_TYPE_CP_ME       = 1,    /* CP-ME                    VG + RV */
	GFX_FW_TYPE_CP_PFP      = 2,    /* CP-PFP                   VG + RV */
	GFX_FW_TYPE_CP_CE       = 3,    /* CP-CE                    VG + RV */
	GFX_FW_TYPE_CP_MEC      = 4,    /* CP-MEC FW                VG + RV */
	GFX_FW_TYPE_CP_MEC_ME1  = 5,    /* CP-MEC Jump Table 1      VG + RV */
	GFX_FW_TYPE_CP_MEC_ME2  = 6,    /* CP-MEC Jump Table 2      VG      */
	GFX_FW_TYPE_RLC_V       = 7,    /* RLC-V                    VG      */
	GFX_FW_TYPE_RLC_G       = 8,    /* RLC-G                    VG + RV */
	GFX_FW_TYPE_SDMA0       = 9,    /* SDMA0                    VG + RV */
	GFX_FW_TYPE_SDMA1       = 10,   /* SDMA1                    VG      */
	GFX_FW_TYPE_DMCU_ERAM   = 11,   /* DMCU-ERAM                VG + RV */
	GFX_FW_TYPE_DMCU_ISR    = 12,   /* DMCU-ISR                 VG + RV */
	GFX_FW_TYPE_VCN         = 13,   /* VCN                           RV */
	GFX_FW_TYPE_UVD         = 14,   /* UVD                      VG      */
	GFX_FW_TYPE_VCE         = 15,   /* VCE                      VG      */
	GFX_FW_TYPE_ISP         = 16,   /* ISP                           RV */
	GFX_FW_TYPE_ACP         = 17,   /* ACP                           RV */
	GFX_FW_TYPE_SMU         = 18,   /* SMU                      VG      */
	GFX_FW_TYPE_MMSCH       = 19,   /* MMSCH                    VG      */
	GFX_FW_TYPE_RLC_RESTORE_LIST_GPM_MEM        = 20,   /* RLC GPM                  VG + RV */
	GFX_FW_TYPE_RLC_RESTORE_LIST_SRM_MEM        = 21,   /* RLC SRM                  VG + RV */
	GFX_FW_TYPE_RLC_RESTORE_LIST_SRM_CNTL       = 22,   /* RLC CNTL                 VG + RV */
	GFX_FW_TYPE_UVD1        = 23,   /* UVD1                     VG-20   */
	GFX_FW_TYPE_TOC         = 24,   /* TOC                      NV-10   */
	GFX_FW_TYPE_RLC_P                           = 25,   /* RLC P                    NV      */
	GFX_FW_TYPE_RLC_IRAM                        = 26,   /* RLC_IRAM                 NV      */
	GFX_FW_TYPE_GLOBAL_TAP_DELAYS               = 27,   /* GLOBAL TAP DELAYS        NV      */
	GFX_FW_TYPE_SE0_TAP_DELAYS                  = 28,   /* SE0 TAP DELAYS           NV      */
	GFX_FW_TYPE_SE1_TAP_DELAYS                  = 29,   /* SE1 TAP DELAYS           NV      */
	GFX_FW_TYPE_GLOBAL_SE0_SE1_SKEW_DELAYS      = 30,   /* GLOBAL SE0/1 SKEW DELAYS NV      */
	GFX_FW_TYPE_SDMA0_JT                        = 31,   /* SDMA0 JT                 NV      */
	GFX_FW_TYPE_SDMA1_JT                        = 32,   /* SDNA1 JT                 NV      */
	GFX_FW_TYPE_CP_MES                          = 33,   /* CP MES                   NV      */
	GFX_FW_TYPE_MES_STACK                       = 34,   /* MES STACK                NV      */
	GFX_FW_TYPE_RLC_SRM_DRAM_SR                 = 35,   /* RLC SRM DRAM             NV      */
	GFX_FW_TYPE_RLCG_SCRATCH_SR                 = 36,   /* RLCG SCRATCH             NV      */
	GFX_FW_TYPE_RLCP_SCRATCH_SR                 = 37,   /* RLCP SCRATCH             NV      */
	GFX_FW_TYPE_RLCV_SCRATCH_SR                 = 38,   /* RLCV SCRATCH             NV      */
	GFX_FW_TYPE_RLX6_DRAM_SR                    = 39,   /* RLX6 DRAM                NV      */
	GFX_FW_TYPE_SDMA0_PG_CONTEXT                = 40,   /* SDMA0 PG CONTEXT         NV      */
	GFX_FW_TYPE_SDMA1_PG_CONTEXT                = 41,   /* SDMA1 PG CONTEXT         NV      */
	GFX_FW_TYPE_GLOBAL_MUX_SELECT_RAM           = 42,   /* GLOBAL MUX SEL RAM       NV      */
	GFX_FW_TYPE_SE0_MUX_SELECT_RAM              = 43,   /* SE0 MUX SEL RAM          NV      */
	GFX_FW_TYPE_SE1_MUX_SELECT_RAM              = 44,   /* SE1 MUX SEL RAM          NV      */
	GFX_FW_TYPE_ACCUM_CTRL_RAM                  = 45,   /* ACCUM CTRL RAM           NV      */
	GFX_FW_TYPE_RLCP_CAM                        = 46,   /* RLCP CAM                 NV      */
	GFX_FW_TYPE_RLC_SPP_CAM_EXT                 = 47,   /* RLC SPP CAM EXT          NV      */
	GFX_FW_TYPE_RLC_DRAM_BOOT                   = 48,   /* RLC DRAM BOOT            NV      */
	GFX_FW_TYPE_VCN0_RAM                        = 49,   /* VCN_RAM                  NV + RN */
	GFX_FW_TYPE_VCN1_RAM                        = 50,   /* VCN_RAM                  NV + RN */
	GFX_FW_TYPE_DMUB                            = 51,   /* DMUB                          RN */
	GFX_FW_TYPE_SDMA2                           = 52,   /* SDMA2                    MI      */
	GFX_FW_TYPE_SDMA3                           = 53,   /* SDMA3                    MI      */
	GFX_FW_TYPE_SDMA4                           = 54,   /* SDMA4                    MI      */
	GFX_FW_TYPE_SDMA5                           = 55,   /* SDMA5                    MI      */
	GFX_FW_TYPE_SDMA6                           = 56,   /* SDMA6                    MI      */
	GFX_FW_TYPE_SDMA7                           = 57,   /* SDMA7                    MI      */
	GFX_FW_TYPE_VCN1                            = 58,   /* VCN1                     MI      */
	GFX_FW_TYPE_DFC_FW                          = 61,
	GFX_FW_TYPE_DRV_CAP                         = 62,
	GFX_FW_TYPE_REG_ACCESS_WHITELIST            = 67,
	GFX_FW_TYPE_IMU_I                           = 68,   /* IMU Instruction FW       PB      */
	GFX_FW_TYPE_IMU_D                           = 69,   /* IMU Data FW              PB      */
	GFX_FW_TYPE_LSDMA                           = 70,   /* LSDMA FW                 PB      */
	GFX_FW_TYPE_SDMA_UCODE_TH0                  = 71,   /* SDMA Thread 0/CTX        PB      */
	GFX_FW_TYPE_SDMA_UCODE_TH1                  = 72,   /* SDMA Thread 1/CTL        PB      */
	GFX_FW_TYPE_PPTABLE                         = 73,   /* PPTABLE                  PB      */
	GFX_FW_TYPE_DISCRETE_USB4                   = 74,   /* dUSB4 FW                 PB      */
	GFX_FW_TYPE_TA                              = 75,   /* SRIOV TA FW UUID         PB      */
	GFX_FW_TYPE_RS64_MES                        = 76,   /* RS64 MES ucode           PB      */
	GFX_FW_TYPE_RS64_MES_STACK                  = 77,   /* RS64 MES stack ucode     PB      */
	GFX_FW_TYPE_RS64_KIQ                        = 78,   /* RS64 KIQ ucode           PB      */
	GFX_FW_TYPE_RS64_KIQ_STACK                  = 79,   /* RS64 KIQ Heap stack      PB      */
	GFX_FW_TYPE_ISP_DATA                        = 80,   /* ISP DATA                 PB      */
	GFX_FW_TYPE_CP_MES_KIQ                      = 81,   /* MES KIQ ucode            PB      */
	GFX_FW_TYPE_MES_KIQ_STACK                   = 82,   /* MES KIQ stack            PB      */
	GFX_FW_TYPE_UMSCH_DATA                      = 83,   /* User Mode Scheduler Data PB      */
	GFX_FW_TYPE_UMSCH_UCODE                     = 84,   /* User Mode Scheduler Ucode PB     */
	GFX_FW_TYPE_UMSCH_CMD_BUFFER                = 85,   /* User Mode Scheduler Command Buffer PB */
	GFX_FW_TYPE_USB_DP_COMBO_PHY                = 86,   /* USB-Display port Combo   PB + PHY */
	GFX_FW_TYPE_RS64_PFP                        = 87,   /* RS64 PFP                 PB      */
	GFX_FW_TYPE_RS64_ME                         = 88,   /* RS64 ME                  PB      */
	GFX_FW_TYPE_RS64_MEC                        = 89,   /* RS64 MEC                 PB      */
	GFX_FW_TYPE_RS64_PFP_P0_STACK               = 90,   /* RS64 PFP stack P0        PB      */
	GFX_FW_TYPE_RS64_PFP_P1_STACK               = 91,   /* RS64 PFP stack P1        PB      */
	GFX_FW_TYPE_RS64_ME_P0_STACK                = 92,   /* RS64 ME stack P0         PB      */
	GFX_FW_TYPE_RS64_ME_P1_STACK                = 93,   /* RS64 ME stack P1         PB      */
	GFX_FW_TYPE_RS64_MEC_P0_STACK               = 94,   /* RS64 MEC stack P0        PB      */
	GFX_FW_TYPE_RS64_MEC_P1_STACK               = 95,   /* RS64 MEC stack P1        PB      */
	GFX_FW_TYPE_RS64_MEC_P2_STACK               = 96,   /* RS64 MEC stack P2        PB      */
	GFX_FW_TYPE_RS64_MEC_P3_STACK               = 97,   /* RS64 MEC stack P3        PB      */
	GFX_FW_TYPE_RLX6_UCODE_CORE1                = 98,
	GFX_FW_TYPE_RLX6_DRAM_BOOT_CORE1            = 99,
	GFX_FW_TYPE_RLCV_LX7                        = 100,
	GFX_FW_TYPE_RLC_SAVE_RESTROE_LIST           = 101,
	GFX_FW_TYPE_P2S_TABLE                       = 129,
	GFX_FW_TYPE_MAX
};

enum fwman_fw_id {
    FW_ID_NONE                          = 0x0000,
    FW_ID_PSP_BL_STAGE1                 = 0x0001,
    FW_ID_PSP_BL_STAGE2                 = 0x0002,
    FW_ID_PSP_BL_STAGE3                 = 0x0003,
    FW_ID_PSP_TRUSTED_OS                = 0x0009,
    FW_ID_PSP_SYSDRV                    = 0x000A,
    FW_ID_DEBUG_UNLOCK                  = 0x000B,
    FW_ID_PSP_WHITELIST                 = 0x000C,
    FW_ID_FTPM                          = 0x000D,
    FW_ID_ABL_RUNTIME_DRIVER            = 0x000E,
    FW_ID_PSP_ABL_TOC                   = 0x000F,
    FW_ID_PSP_ABL_0                     = 0x0010,
    FW_ID_PSP_ABL_1                     = 0x0011,
    FW_ID_PSP_ABL_2                     = 0x0012,
    FW_ID_PSP_ABL_3                     = 0x0013,
    FW_ID_PSP_ABL_4                     = 0x0014,
    FW_ID_PSP_ABL_5                     = 0x0015,
    FW_ID_PSP_ABL_6                     = 0x0016,
    FW_ID_PSP_ABL_7                     = 0x0017,
    FW_ID_PSP_ABL_8                     = 0x0018,
    FW_ID_PSP_ABL_9                     = 0x0019,
    FW_ID_PSP_ABL_10                    = 0x001A,
    FW_ID_PSP_ABL_11                    = 0x001B,
    FW_ID_PSP_ABL_12                    = 0x001C,
    FW_ID_PSP_ABL_13                    = 0x001D,
    FW_ID_PSP_ABL_14                    = 0x001E,
    FW_ID_PSP_ABL_15                    = 0x001F,
    FW_ID_PSP_ABL_16                    = 0x0020,
    FW_ID_PSP_ABL_17                    = 0x0021,
    FW_ID_PSP_ABL_18                    = 0x0022,
    FW_ID_PSP_ABL_19                    = 0x0023,
    FW_ID_PSP_ABL_20                    = 0x0024,
    FW_ID_PSP_VBL0                      = 0x0040,
    FW_ID_PSP_VBL1                      = 0x0041,
    FW_ID_PSP_VBL2                      = 0x0042,
    FW_ID_PSP_VBL_MGPU                  = 0x0043,
    FW_ID_PSP_TOKEN_UNLOCK_DATA         = 0x004A,
    FW_ID_SEC_GASKET                    = 0x004B,
    FW_ID_GFX_SEC_GASKET                = 0x004C,
    FW_ID_SRIOV_SEC_GASKET              = 0x004D,
    FW_ID_PSP_BL_KDB                    = 0x004E,
    FW_ID_PSP_SOS_KDB                   = 0x004F,
    FW_ID_DIAG_BL                       = 0x0050,
    FW_ID_PSP_KVM                       = 0x0051,
    FW_ID_PSP_BOOT_DRTM                 = 0x0052,
    FW_ID_PSP_XGMI_PHY                  = 0x101A,
    FW_ID_PSP_DP_PHY                    = 0x0054,
    FW_ID_ANTIROLLBACK                  = 0x0055,
    FW_ID_GFX_ANTIROLLBACK              = 0x0056,
    FW_ID_PSP_WAFL                      = 0x0057,
    FW_ID_PCIE_ESM                      = 0x0058,
    FW_ID_PSP_RAS                       = 0x0059,
    FW_ID_PSP_TOPOLOGY                  = 0x005A,
    FW_ID_PSP_PLAT_CONFIG               = 0x005B,
    FW_ID_BCM                           = 0x005C,
    FW_ID_PSP_TOS_NONPROD_KEYDB         = 0x005D,
    FW_ID_SEC_PENETRATION               = 0x005E,
    FW_ID_PSP_DXIO                      = 0x005F,
    FW_ID_VBL_STAGE1                    = 0x0040,
    FW_ID_VBL_STAGE2                    = 0x0041,
    FW_ID_VBL_STAGE3                    = 0x0042,
    FW_ID_VBL_MGPU                      = 0x0043,
    FW_ID_DF_RIB_TOC                    = 0x0131,
    FW_ID_PSP_SEC_DEBUG_UNLOCK          = 0x0132,
    FW_ID_SPIROM_MODEL                  = 0x0133,
    FW_ID_PSP_RAS_MCA_TABLE             = 0x0134,
    FW_ID_AMD_PUB_KEY_TOKEN             = 0x0139,
    FW_ID_PSP_WRAPPED_IKEK              = 0x013B,
    FW_ID_SMU_SRAM                      = 0x1001,
    FW_ID_SMU_DRAM                      = 0x1002,
    FW_ID_DMCUB                         = 0x1005,
    FW_ID_DMCUB_ISR                     = 0x1006,
    FW_ID_IP_DISCOVERY                  = 0x1007,
    FW_ID_MC_STAGE1                     = 0x1009,
    FW_ID_MC_STAGE2                     = 0x100A,
    FW_ID_MP2                           = 0x1010,
    FW_ID_MP2_CONFIG_DATA               = 0x1011,
    FW_ID_PMU1                          = 0x1012,
    FW_ID_PMU1_DATA                     = 0x1013,
    FW_ID_PMU2                          = 0x1014,
    FW_ID_PMU2_DATA                     = 0x1015,
    FW_ID_WLAN                          = 0x1016,
    FW_ID_MSMU0                         = 0x1018,
    FW_ID_MSMU1                         = 0x1019,
    FW_ID_MPIO                          = 0x101B,
    FW_ID_USB_PHY_FW_TOC                = 0x101C,
    FW_ID_PMU3                          = 0x1020,
    FW_ID_PMU3_DATA                     = 0x1021,
    FW_ID_PMU4                          = 0x1022,
    FW_ID_PMU4_DATA                     = 0x1023,
    FW_ID_PMU5                          = 0x1024,
    FW_ID_PMU5_DATA                     = 0x1025,
    FW_ID_PMU6                          = 0x1026,
    FW_ID_PMU6_DATA                     = 0x1027,
    FW_ID_PMU7                          = 0x1028,
    FW_ID_PMU7_DATA                     = 0x1029,
    FW_ID_PMU8                          = 0x102A,
    FW_ID_PMU8_DATA                     = 0x102B,
    FW_ID_PMU9                          = 0x102C,
    FW_ID_PMU9_DATA                     = 0x102D,
    FW_ID_PMU10                         = 0x102E,
    FW_ID_PMU10_DATA                    = 0x102F,
    FW_ID_PMU11                         = 0x1030,
    FW_ID_PMU11_DATA                    = 0x1031,
    FW_ID_PMU12                         = 0x1032,
    FW_ID_PMU12_DATA                    = 0x1033,
    FW_ID_PMU13                         = 0x1034,
    FW_ID_PMU13_DATA                    = 0x1035,
    FW_ID_PMU14                         = 0x1036,
    FW_ID_PMU14_DATA                    = 0x1037,
    FW_ID_PMU15                         = 0x1038,
    FW_ID_PMU15_DATA                    = 0x1039,
    FW_AMF_SRAM                         = 0x1041,
    FW_AMF_DRAM                         = 0x1042,
    FW_AMF_AEPP                         = 0x1043,
    FW_AMF_WLAN                         = 0x1044,
    FW_AMF_MFD                          = 0x1046,
    FW_ID_SDMA0                         = 0x2025,
    FW_ID_SDMA1                         = 0x2026,
    FW_ID_RLC_G                         = 0x2009,
    FW_ID_RLC_V                         = 0x200A,
    FW_ID_MMSCHEDULER                   = 0x2037,
    FW_ID_CP_ME                         = 0x2001,
    FW_ID_CP_PFP                        = 0x2002,
    FW_ID_CP_CE                         = 0x2003,
    FW_ID_CP_MEC                        = 0x2004,
    FW_ID_DMCU_ERAM                     = 0x1003,
    FW_ID_DMCU_ISR                      = 0x1004,
    FW_ID_ISP                           = 0x2031,
    FW_ID_ACP                           = 0x2032,
    FW_ID_RLC_RESTORE_LIST_GPM_MEM      = 0x200B,
    FW_ID_RLC_RESTORE_LIST_SRM_MEM      = 0x200C,
    FW_ID_RLC_RESTORE_LIST_SRM_CNTL     = 0x200D,
    FW_ID_RLC_P                         = 0x200F,
    FW_ID_GLOBAL_TAP_DELAYS             = 0x2019,
    FW_ID_SE0_TAP_DELAYS                = 0x201A,
    FW_ID_SE1_TAP_DELAYS                = 0x201B,
    FW_ID_GLOBAL_SE0_SE1_SKEW_DELAYS    = 0x201E,
    FW_ID_CP_MES                        = 0x2007,
    FW_ID_MES_STACK                     = 0x2008,
    FW_ID_RLC_SRM_DRAM_SR               = 0x2012,
    FW_ID_RLCG_SCRATCH_SR               = 0x2013,
    FW_ID_RLCP_SCRATCH_SR               = 0x2014,
    FW_ID_RLCV_SCRATCH_SR               = 0x2015,
    FW_ID_RLX6_DRAM_SR                  = 0x2018,
    FW_ID_SDMA0_PG_CONTEXT              = 0x202D,
    FW_ID_SDMA1_PG_CONTEXT              = 0x202E,
    FW_ID_GLOBAL_MUX_SELECT_RAM         = 0x201F,
    FW_ID_SE0_MUX_SELECT_RAM            = 0x2020,
    FW_ID_SE1_MUX_SELECT_RAM            = 0x2021,
    FW_ID_ACCUM_CTRL_RAM                = 0x2024,
    FW_ID_RLCP_CAM                      = 0x2010,
    FW_ID_RLC_SPP_CAM_EXT               = 0x2011,
    FW_ID_RLX6_DRAM_BOOT                = 0x2017,
    FW_ID_SDMA2                         = 0x2027,
    FW_ID_SDMA3                         = 0x2028,
    FW_ID_SDMA4                         = 0x2029,
    FW_ID_SDMA5                         = 0x202A,
    FW_ID_SDMA6                         = 0x202B,
    FW_ID_SDMA7                         = 0x202C,
    FW_ID_VCN1                          = 0x2039,
    FW_ID_RESERVED                      = 0x1000,
    FW_ID_USB_PD_FW                     = 0x1045,
    FW_ID_SRIOV_COMPAT_TABLE            = 0x01C0,
    FW_ID_CVIP                          = 0x101F,
    FW_ID_SE2_TAP_DELAYS                = 0x201C,
    FW_ID_SE3_TAP_DELAYS                = 0x201D,
    FW_ID_VCN                           = 0x2038,
    FW_ID_CP_MEC_ME1                    = 0x2005,
    FW_ID_CP_MEC_ME2                    = 0x2006,
    FW_ID_SMU                           = 0x1018,
    FW_ID_TOC                           = 0x200E,
    FW_ID_RLX6                          = 0x2016,
    FW_ID_DMUB                          = 0x1005,
    FW_ID_REG_ACCESS_WHITELIST          = 0x0135,
    FW_ID_IMU_I                         = 0x203B,
    FW_ID_IMU_D                         = 0x203C,
    FW_ID_SDMA_UCODE_TH0                = 0x203E,
    FW_ID_SDMA_UCODE_TH1                = 0x203F,
    FW_ID_PPTABLE                       = 0x0146,
    FW_ID_RS64_KIQ                      = 0x2042,
    FW_ID_RS64_KIQ_STACK                = 0x2043,
    FW_ID_UMSCH_DATA                    = 0x204B,
    FW_ID_UMSCH_UCODE                   = 0x204A,
    FW_ID_USB_DP_COMBO_PHY              = 0x101D,
    FW_ID_RS64_PFP                      = 0x2044,
    FW_ID_RS64_ME                       = 0x2046,
    FW_ID_RS64_MEC                      = 0x2048,
    FW_ID_RS64_PFP_P0_STACK             = 0x2045,
    FW_ID_RS64_ME_P0_STACK              = 0x2047,
    FW_ID_RS64_MEC_P0_STACK             = 0x2049,
    FW_ID_SKU_TOKEN                     = 0x0145,
    FW_ID_ATOM_TABLE                    = 0x0147,
    FW_ID_IGNORE
};

enum psp_bootloader_command_list {
	PSP_BL__LOAD_SYSDRV                         = 0x10000,
	PSP_BL__LOAD_SOSDRV                         = 0x20000,
	PSP_BL__LOAD_KEY_DATABASE                   = 0x80000,
	PSP_BL__APPLY_SECURITY_POLICY               = 0x90000,
	PSP_BL__LOAD_SOCDRV                         = 0xB0000,
	PSP_BL__LOAD_DBGDRV                         = 0xC0000,
	PSP_BL__LOAD_INTFDRV                        = 0xD0000,
	PSP_BL__LOAD_RASDRV                         = 0xE0000,
	PSP_BL__LOAD_TOS_SPL_TABLE                  = 0x10000000,
};

/* Command-specific response for VF gating */
struct psp_gfx_uresp_vfgate {
	uint32_t drv_version; /* driver version of last-attempted */
	/*  CAP load */
};

/* Command-specific response for boot config. */
struct psp_gfx_uresp_bootcfg
{
    uint32_t    boot_cfg;           /* boot config data */
};

/* Sets size of command-specific response.
 * NO OTHER STRUCT IN THE UNION MAY BE BIGGER THAN THIS.
 * There's no (good) way to auto-check this at compile-time. */
struct psp_gfx_uresp_reserved
{
    uint32_t reserved[8];
};

struct psp_gfx_migration_info
{
	uint32_t    migration_version;           /**< Boot config data */
};

struct psp_gfx_migration_export
{
	uint32_t    size_written;           /**< Boot config data */
};

/* Command-specific response for Fw Attestation Db */
struct psp_gfx_uresp_fw_attestation_db_info {
	uint32_t        FwAttestationDbAddrLo;
	uint32_t        FwAttestationDbAddrHi;
};

/* Command-specific responses for GPCOM ring */
union psp_gfx_uresp {
	struct psp_gfx_uresp_vfgate vfgate;
	struct psp_gfx_uresp_fw_attestation_db_info  fw_attestation_db_info;
	struct psp_gfx_uresp_reserved		reserved;
	struct psp_gfx_uresp_bootcfg		bootcfg;
	struct psp_gfx_migration_info		migration_info;
	struct psp_gfx_migration_export		migration_export;
};

/* SRIOV mailbox response fields */
#define SRIOV_MBSTATUS_ISENABLED_MASK (0x80000000u)
/* fw type if last error was related to fw loading */
#define SRIOV_MBSTATUS_ERR_FWTYPE_MASK (0x0000FF00u)
#define SRIOV_MBSTATUS_ERRSTATUS_MASK  (0x000000FFu)

struct psp_gfx_resp {
	uint32_t status; /* +0  status of command execution*/
	uint32_t session_id; /* +4  session ID in response to LoadTa
			      * command
			      */
	uint32_t fw_addr_lo; /* +8  bits [31:0] of FW address within
			      * TMR  (in response to cmd_load_ip_fw
			      * command)
			      */
	uint32_t fw_addr_hi; /* +12 bits [63:32] of FW address
			      * within TMR (in response to
			      * cmd_load_ip_fw command)
			      */
	uint32_t tmr_size; /* +16 Size of the TMR to be reserved
			    * including MM fw and Gfx fw
			    * (in response to cmd_load_toc command)
			    */
	uint32_t timestamp; /* +20 calendar time as number of secs
			     * in resp to psp_gfx_cmd_dram_params
			     * command (not in LibGV)
			     */
	uint8_t sys_id[32]; /* +24 system ID in response to
			     * psp_gfx_cmd_dram_params command
			     * (not used in LibGV)
			     */
	uint32_t sriov_mbstatus; /* +56 status of SRIOV mailbox,
				  * returned by VFGATE command
				  */
	uint32_t info; /* +60 resp to GFX_CMD_ID_QUERY_XX
			* cmds
			*/
	union psp_gfx_uresp uresp; /* cmd-specific resp */
	uint32_t	    reserved[9]; /* +64 */
	/* total 96 bytes */
};

/* Command to setup TMR region. */
struct psp_gfx_cmd_setup_tmr {
	uint32_t buf_phy_addr_lo; /* bits [31:0] of GPU Virtual address
				   * of TMR buffer (must be 4 KB
				   * aligned)
				   */
	uint32_t buf_phy_addr_hi; /* bits [63:32] of GPU Virtual
				   * address of TMR buffer
				   */
	uint32_t buf_size; /* buffer size in bytes (must be
			    * multiple of 4 KB)
			    */
	uint32_t sriov_params; /* bit[31] is set if SR-IOV is enabled, bits[0-4] give the
				  number of VFs if SR-IOV is enabled */
};

struct psp_gfx_cmd_load_ip_fw {
	uint32_t fw_phy_addr_lo; /* bits [31:0] of physical address of
				  * FW location (must be 4 KB aligned)
				  */
	uint32_t fw_phy_addr_hi; /* bits [63:32] of physical address
				  * of FW location
				  */

	uint32_t	     fw_size; /* FW buffer size in bytes */
	enum psp_gfx_fw_type fw_type; /* FW type */
};

/* Command to load Trusted Application binary into PSP OS. */
struct psp_gfx_cmd_load_ta {
	uint32_t app_phy_addr_lo; /* bits [31:0] of the GPU Virtual address of the TA binary
				     (must be 4 KB aligned) */
	uint32_t app_phy_addr_hi; /* bits [63:32] of the GPU Virtual address of the TA binary
				   */
	uint32_t app_len; /* length of the TA binary in bytes */
	uint32_t cmd_buf_phy_addr_lo; /* bits [31:0] of the GPU Virtual address of CMD buffer
					 (must be 4 KB aligned) */
	uint32_t cmd_buf_phy_addr_hi; /* bits [63:32] of the GPU Virtual address of CMD buffer
				       */
	uint32_t cmd_buf_len; /* length of the CMD buffer in bytes; must be multiple of 4 KB */

	/* Note: CmdBufLen can be set to 0. In this case no persistent CMD buffer is provided
	 *       for the TA. Each InvokeCommand can have dinamically mapped CMD buffer instead
	 *       of using global persistent buffer.
	 */
};

/* Command to Unload Trusted Application binary from PSP OS. */
struct psp_gfx_cmd_unload_ta {
	uint32_t session_id; /* Session ID of the loaded TA to be unloaded */
};

/* Shared buffers for InvokeCommand.
 */
struct psp_gfx_buf_desc {
	uint32_t buf_phy_addr_lo; /* bits [31:0] of GPU Virtual address of the buffer (must be
				     4 KB aligned) */
	uint32_t buf_phy_addr_hi; /* bits [63:32] of GPU Virtual address of the buffer */
	uint32_t buf_size; /* buffer size in bytes (must be multiple of 4 KB and no bigger than
			      64 MB) */
};

struct psp_gfx_buf_list {
	uint32_t num_desc; /* number of buffer descriptors in the list */
	uint32_t total_size; /* total size of all buffers in the list in bytes (must be
				multiple of 4 KB) */
	struct psp_gfx_buf_desc buf_desc[GFX_BUF_MAX_DESC]; /* list of buffer descriptors */
	/* total 776 bytes */
};

/* Command to execute InvokeCommand entry point of the TA. */
struct psp_gfx_cmd_invoke_cmd {
	uint32_t		session_id; /* Session ID of the TA to be executed */
	uint32_t		ta_cmd_id; /* Command ID to be sent to TA */
	struct psp_gfx_buf_list buf; /* one indirect buffer (scatter/gather list) */
};

struct psp_gfx_cmd_load_toc {
	uint32_t toc_phy_addr_lo; /* bits [31:0] of physical address of
				   * FW location (must be 4 KB aligned)
				   */
	uint32_t toc_phy_addr_hi; /* bits [63:32] of physical address
				   * of FW location
				   */

	uint32_t toc_size; /* FW buffer size in bytes */
};

struct psp_gfx_cmd_gbr_ih_reg {
	uint32_t                reg_value;        /**< 32-bit value to be set or LOW word of 64-bit. */
	uint32_t                reg_id;           /**< ID of the register */
	uint32_t                reg_value_hi;     /**< Ignored in 32-bit case or HIGH word of 64-bit. */
	uint32_t                target_vfid;      /**< Target vfid to be programmed from PF */
};

enum psp_gfx_vfgate_action {
	GFX_SCMD_VFGATE_STATUS	    = 0x00000000, /* Return current status */
	GFX_SCMD_VFGATE_ENABLE	    = 0x00000001, /* Enable the SRIOV mailboxes */
	GFX_SCMD_VFGATE_DISABLE	    = 0x00000002, /* Disable the SRIOV mailboxes */
	GFX_SCMD_VFGATE_AUTODISABLE = 0x00000003, /* Disable without status
						   *  reset. Used internally by
						   *  PSP only
						   */
};

/* Check for the RWL section version in 3rd byte of flag field of block
 * header of PSP snapshot block. i.e., bit 16 to 23. if it is set to 01
 * then use RWL_V01_SECTION_ID
 * refer to macro AMDGV_DIAG_DATA_PSP_RWL_SECTION_FLAG
 */
enum RWL_V01_SECTION_ID {
	RWL_V01_SECTION_ID_RAS		= 0,

	RWL_V01_SECTION_DIAG_DATA_START	= 1,
	RWL_V01_SECTION_ID_PF_REG		= 1,
	RWL_V01_SECTION_ID_SRAM_RLCV		= 2,
	RWL_V01_SECTION_ID_SRAM_RLCG		= 3,
	RWL_V01_SECTION_ID_SRAM_MMSCH		= 4,
	RWL_V01_SECTION_ID_SRAM_SDMA0		= 5,
	RWL_V01_SECTION_ID_SRAM_SDMA1		= 6,
	RWL_V01_SECTION_ID_SRAM_SDMA2		= 7,
	RWL_V01_SECTION_ID_SRAM_SDMA3		= 8,
	RWL_V01_SECTION_ID_SRAM_SMU		= 9,
	RWL_V01_SECTION_ID_SRAM_ARAM		= 10,
	RWL_V01_SECTION_ID_SRAM_DRAM		= 11,
	RWL_V01_SECTION_ID_SRAM_SMU_MP5		= 12,
	RWL_V01_SECTION_ID_VF_REG		= 13,
	RWL_V01_SECTION_DIAG_DATA_END 	= 13,

	RWL_V01_SECTION_ID_LIVEMIG_VF		= 14,
};

enum RWL_V01_SECTION_MASK {
	RWL_V01_SECTION_MASK_RAS		= 1 << RWL_V01_SECTION_ID_RAS,
	RWL_V01_SECTION_MASK_PF			= 1 << RWL_V01_SECTION_ID_PF_REG,
	RWL_V01_SECTION_MASK_SRAM_RLCV		= 1 << RWL_V01_SECTION_ID_SRAM_RLCV,
	RWL_V01_SECTION_MASK_SRAM_RLCG		= 1 << RWL_V01_SECTION_ID_SRAM_RLCG,
	RWL_V01_SECTION_MASK_SRAM_MMSCH		= 1 << RWL_V01_SECTION_ID_SRAM_MMSCH,
	RWL_V01_SECTION_MASK_SRAM_SDMA0		= 1 << RWL_V01_SECTION_ID_SRAM_SDMA0,
	RWL_V01_SECTION_MASK_SRAM_SDMA1		= 1 << RWL_V01_SECTION_ID_SRAM_SDMA1,
	RWL_V01_SECTION_MASK_SRAM_SDMA2		= 1 << RWL_V01_SECTION_ID_SRAM_SDMA2,
	RWL_V01_SECTION_MASK_SRAM_SDMA3		= 1 << RWL_V01_SECTION_ID_SRAM_SDMA3,
	RWL_V01_SECTION_MASK_SRAM_SMU		= 1 << RWL_V01_SECTION_ID_SRAM_SMU,
	RWL_V01_SECTION_MASK_SRAM_ARAM		= 1 << RWL_V01_SECTION_ID_SRAM_ARAM,
	RWL_V01_SECTION_MASK_SRAM_DRAM		= 1 << RWL_V01_SECTION_ID_SRAM_DRAM,
	RWL_V01_SECTION_MASK_SRAM_SMU_MP5	= 1 << RWL_V01_SECTION_ID_SRAM_SMU_MP5,
	RWL_V01_SECTION_MASK_VF_REG		= 1 << RWL_V01_SECTION_ID_VF_REG,
	RWL_V01_SECTION_MASK_LIVEMIG_VF		= 1 << RWL_V01_SECTION_ID_LIVEMIG_VF,
};

struct psp_gfx_cmd_vfgate {
	enum psp_gfx_vfgate_action action;
	uint32_t		   target_vfid;
};

struct psp_gfx_cmd_num_vfs {
	uint32_t number_of_vfs;
};

struct psp_gfx_cmd_clear_vf_fw {
	uint32_t target_vf;
};

struct psp_gfx_cmd_vf_relay {
	uint32_t target_vf;
	uint32_t vf_relay_wtr_ptr;
	uint32_t reserved[2];
};

struct psp_gfx_cmd_dbg_snapshot_setaddr {
	uint32_t addr_hi;
	uint32_t addr_lo;
	uint32_t size;
};

struct psp_gfx_cmd_dbg_snapshot_trigger {
	uint32_t target_vfid;
	uint32_t sections;
	uint32_t size;
	uint32_t aid_mask;
	uint32_t xcc_mask;
};

struct psp_gfx_cmd_dump_tracelog {
	uint32_t addr_hi; /* bits [63:32] of GPU Virtual addr of the buffer */
	uint32_t addr_lo;	/* bits [31:0] of GPU Virtual address of the
				 * buffer (must be 4 KB aligned)
				 */
	uint32_t size;	/* buffer size in bytes (must be multiple of 4 KB and
			 * no bigger than 64 MB)
			 */
};

struct psp_gfx_cmd_sriov_spatial_part {
	uint32_t mode;	/* Interpreted as number of partitions desired */
	uint32_t override_ips;
	uint32_t override_xcds_avail;
	uint32_t override_this_aid;
};

struct psp_gfx_cmd_sriov_memory_part {
	uint32_t num_parts; /* Interpreted as number of partitions desired */
};

struct psp_gfx_cmd_sriov_copy_vf_chiplet_regs {
	uint32_t source_vfid;
};

struct psp_gfx_cmd_migration_get_psp_info {
	uint32_t migration_version;
};
struct psp_gfx_cmd_migration_export
{
	uint32_t pkg_addr_hi;
	uint32_t pkg_addr_lo;
	/* Input: no-overrun limit provided by driver */
	uint32_t pkg_size_allocated;
	uint32_t target_vfid;
	uint32_t flags;
};
struct psp_gfx_cmd_migration_import
{
	uint32_t pkg_addr_hi;
	uint32_t pkg_addr_lo;
	/* Input: driver provides actual total PKG size. */
	uint32_t pkg_size;
	uint32_t target_vfid;
};

union psp_gfx_commands {
	struct psp_gfx_cmd_load_ta    cmd_load_ta;
	struct psp_gfx_cmd_unload_ta  cmd_unload_ta;
	struct psp_gfx_cmd_invoke_cmd cmd_invoke_cmd;
	struct psp_gfx_cmd_setup_tmr  cmd_setup_tmr;
	struct psp_gfx_cmd_load_ip_fw cmd_load_ip_fw;
	struct psp_gfx_cmd_load_toc   cmd_load_toc;
	struct psp_gfx_cmd_gbr_ih_reg cmd_program_reg;
	struct psp_gfx_cmd_vfgate     cmd_vfgate;
	struct psp_gfx_cmd_num_vfs    cmd_num_vfs;
	struct psp_gfx_cmd_clear_vf_fw cmd_clear_vf_fw;
	struct psp_gfx_cmd_vf_relay    cmd_vf_relay;
	struct psp_gfx_cmd_dbg_snapshot_setaddr cmd_dbg_snapshot_setaddr;
	struct psp_gfx_cmd_dbg_snapshot_trigger cmd_dbg_snapshot_trigger;
	struct psp_gfx_cmd_dump_tracelog	cmd_dump_tracelog;
	struct psp_gfx_cmd_sriov_spatial_part cmd_sriov_spatial_part;
	struct psp_gfx_cmd_sriov_memory_part cmd_sriov_memory_part;
	struct psp_gfx_cmd_sriov_copy_vf_chiplet_regs cmd_sriov_copy_vf_chiplet_regs;
	struct psp_gfx_cmd_migration_get_psp_info	cmd_migration_get_psp_info;
	struct psp_gfx_cmd_migration_export			cmd_migration_export;
	struct psp_gfx_cmd_migration_import			cmd_migration_import;
};

struct psp_gfx_cmd_resp {
	uint32_t buf_size; /* +0 total size of the buffer in bytes */
	uint32_t buf_version; /* +4 version of the buffer structure;
			       * must be PSP_GFX_CMD_BUF_VERSION
			       */
	uint32_t cmd_id; /* +8  command ID */

	/* These fields are used for RBI only.They are all 0 in GPCOM commands */

	uint32_t resp_buf_addr_lo; /* +12 bits [31:0] of physical address
				    * of response buffer (must be 4 KB
				    * aligned)
				    */
	uint32_t resp_buf_addr_hi; /* +16 bits [63:32] of physical
				    * address of response buffer
				    */
	uint32_t resp_offset; /* +20 offset within response buffer*/
	uint32_t resp_buf_size; /* +24 total size of the response
				 * buffer in bytes
				 */

	union psp_gfx_commands cmd; /* +28 command specific structures */

	uint8_t reserved_1[864 - sizeof(union psp_gfx_commands) - 28];

	/* Note: Resp is part of this buffer for GPCOM ring. For RBI ring the
	 *	response is separate buffer pointed by resp_buf_addr_hi and
	 *resp_buf_addr_lo.
	 */
	struct psp_gfx_resp resp; /* +864 response */

	uint8_t reserved_2[1024 - 864 - sizeof(struct psp_gfx_resp)];

	/* total size 1024 bytes */
};

struct psp_cmd_km_load_ip_fw {
	uint32_t	     fw_addr_lo; /* bits [63:32] of address of the buffer */
	uint32_t	     fw_addr_hi; /* bits [31:0] of address of the *buffer */
	uint32_t	     fw_size; /* fw size in bytes */
	enum amdgv_firmware_id fw_type;
};

/* Command to load Trusted Application binary into PSP OS. */
struct psp_cmd_km_load_ta {
	uint32_t app_buf_addr_lo; /* bits [63:32] of the physical address of the TA binary */
	uint32_t app_buf_addr_hi; /* bits [31:0] of the physical address of the TA binary */
	uint32_t app_size; /* size of the TA binary in bytes */
	uint32_t shared_mem_addr_lo; /* bits [63:32] of the physical address of Shared Memory
									buffer */
	uint32_t shared_mem_addr_hi; /* bits [31:0] of the physical address of Shared Memory
									buffer; must be 4 KB
					aligned */
	uint32_t shared_mem_size; /* size of the Shared Memory buffer in bytes; must be
				     multiple of 4 KB */
	/* Note: cmd_buf.buf_size can be set to 0. In this case no persistent CMD buffer is
	 * provided for the TA. Each InvokeCommand can have dynamically mapped CMD buffer
	 * instead of using global persistent buffer.
	 */
};

/* Command to Unload Trusted Application binary from PSP OS. */
struct psp_cmd_km_unload_ta {
	uint32_t session_id; /* Session ID of the loaded TA to be unloaded */
};

struct psp_cmd_km_buf_desc {
	uint32_t addr_lo; /* bits [63:32] of address of the buffer */
	uint32_t addr_hi; /* bits [31:0] of address of the buffer */
	uint32_t size; /* buffer size in bytes */
};

struct psp_cmd_km_ind_buf_list {
	uint32_t num_desc; /* number of buffer descriptors in the list */
	uint32_t total_size; /* total size of all buffers in the list in bytes (must be
				multiple of 4 KB) */
	struct psp_cmd_km_buf_desc buf_desc[GFX_BUF_MAX_DESC]; /* list of buffer descriptors */
	/* total 776 bytes */
};

/* Command to execute InvokeCommand entry point of the TA. */
struct psp_cmd_km_invoke_cmd {
	uint32_t		       session_id; /* Session ID of the TA to be executed */
	uint32_t		       ta_cmd_id; /* Command ID to be sent to TA */
	struct psp_cmd_km_ind_buf_list buf_list; /* one indirect buffer (scatter/gather list)
						  */
};

/* Command to setup TMR region. */
struct psp_cmd_km_setup_tmr {
	uint32_t tmr_buf_addr_lo; /* bits [63:32] of address of the buffer */
	uint32_t tmr_buf_addr_hi; /* bits [31:0] of address of the buffer */
	uint32_t tmr_size; /* buffer size in bytes */
	uint32_t tmr_sriov_params; /* set as 1 for sriov, 0 for bare-metal*/
};

/* Command to setup TOC region. */
struct psp_cmd_km_setup_toc {
	uint32_t toc_addr_lo; /* bits [63:32] of address of the buffer */
	uint32_t toc_addr_hi; /* bits [31:0] of address of the buffer */
	uint32_t toc_size; /* buffer size in bytes */
};

struct psp_cmd_km_program_reg {
	uint32_t                reg_value;        /**< 32-bit value to be set or LOW word of 64-bit. */
	uint32_t                reg_id;           /**< ID of the register */
	uint32_t                reg_value_hi;     /**< Ignored in 32-bit case or HIGH word of 64-bit. */
	uint32_t                target_vfid;      /**< Target vfid to be programmed from PF */
};

struct psp_cmd_km_vfgate {
	enum psp_gfx_vfgate_action action;
	uint32_t		   target_vfid;
};

struct psp_cmd_km_num_vfs {
	uint32_t number_of_vfs; /* number of enabled vfs */
};

struct psp_cmd_km_clear_vf_fw {
	uint32_t target_vf;
};

struct psp_cmd_km_vf_relay {
	uint32_t target_vf;
	uint32_t vf_relay_wtr_ptr;
	uint32_t reserved[2];
};

struct psp_cmd_km_dbg_snapshot_setaddr {
	uint32_t addr_hi;
	uint32_t addr_lo;
	uint32_t size;
};

struct psp_cmd_km_dbg_snapshot_trigger {
	uint32_t target_vfid;
	uint32_t sections;
	uint32_t size;
	uint32_t aid_mask;
	uint32_t xcc_mask;
};

struct psp_cmd_km_dump_tracelog {
	uint32_t addr_hi; /* bits [63:32] of GPU Virtual addr of the buffer */
	uint32_t addr_lo;	/* bits [31:0] of GPU Virtual address of the
				 * buffer (must be 4 KB aligned)
				 */
	uint32_t size;	/* buffer size in bytes (must be multiple of 4 KB and
			 * no bigger than 64 MB)
			 */
};

struct psp_cmd_km_sriov_spatial_part {
	uint32_t mode;	/* Interpreted as number of partitions desired */
	uint32_t override_ips;
	uint32_t override_xcds_avail;
	uint32_t override_this_aid;
};

struct psp_cmd_km_sriov_memory_part {
	uint32_t num_parts; /* Interpreted as number of partitions desired */
};

struct psp_cmd_km_sriov_copy_vf_chiplet_regs {
	uint32_t source_vfid;
};

struct psp_cmd_km_migration_get_psp_info {
	uint32_t migration_version;
};

struct psp_cmd_km_migration_export
{
	uint32_t pkg_addr_hi;
	uint32_t pkg_addr_lo;
	uint32_t pkg_size_allocated; // Input: no-overrun limit provided by driver
	uint32_t target_vfid;
	uint32_t flags;
};

struct psp_cmd_km_migration_import
{
	uint32_t pkg_addr_hi;
	uint32_t pkg_addr_lo;
	uint32_t pkg_size; // Input: driver provides actual total PKG size.
	uint32_t target_vfid;
};

union psp_cmd_km_commands {
	struct psp_cmd_km_load_ta     load_ta;
	struct psp_cmd_km_unload_ta   unload_ta;
	struct psp_cmd_km_invoke_cmd  invoke_ta;
	struct psp_cmd_km_setup_tmr   setup_tmr;
	struct psp_cmd_km_load_ip_fw  load_ip_fw;
	struct psp_cmd_km_setup_toc   load_toc;
	struct psp_cmd_km_program_reg program_reg;
	struct psp_cmd_km_vfgate      vfgate;
	struct psp_cmd_km_num_vfs     num_vfs;
	struct psp_cmd_km_clear_vf_fw clear_vf_fw;
	struct psp_cmd_km_vf_relay    vf_relay;
	struct psp_cmd_km_dbg_snapshot_setaddr	dbg_snapshot_setaddr;
	struct psp_cmd_km_dbg_snapshot_trigger	dbg_snapshot_trigger;
	struct psp_cmd_km_dump_tracelog	dump_tracelog;
	struct psp_cmd_km_sriov_spatial_part	sriov_spatial_part;
	struct psp_cmd_km_sriov_memory_part	sriov_memory_part;
	struct psp_cmd_km_sriov_copy_vf_chiplet_regs sriov_copy_vf_chiplet_regs;
	struct psp_cmd_km_migration_get_psp_info	migration_get_psp_info;
	struct psp_cmd_km_migration_export			migration_export;
	struct psp_cmd_km_migration_import			migration_import;
};

struct psp_cmd_km {
	enum psp_cmd_km_type	  cmd_id;
	union psp_cmd_km_commands cmd;
};

struct psp_cmd_km_handle {
	uint32_t index; /* pointer to cmd buffer */
};

struct psp_gfx_rb_frame {
	uint32_t cmd_buf_addr_lo; /* +0  bits [31:0] of physical address
				   * of command buffer (must be 4 KB
				   * aligned)
				   */
	uint32_t cmd_buf_addr_hi; /* +4  bits [63:32] of physical address
				   * of command buffer
				   */
	uint32_t cmd_buf_size; /* +8  command buffer size in bytes */
	uint32_t fence_addr_lo; /* +12 bits [31:0] of physical address
				 * of Fence for this frame
				 */
	uint32_t fence_addr_hi; /* +16 bits [63:32] of physical address
				 * of Fence for this frame
				 */
	uint32_t fence_value; /* +20 Fence value */
	uint32_t sid_lo; /* +24 bits [31:0] of SID value (used
			  * only for RBI frames)
			  */
	uint32_t sid_hi; /* +28 bits [63:32] of SID value (used
			  * only for RBI frames)
			  */
	uint8_t vmid; /* +32 VMID value used for mapping of
		       * all addresses for this frame
		       */
	uint8_t frame_type; /* +33 1: destroy context frame, 0: all
			     * other frames; used only for RBI
			     * frames
			     */
	uint8_t	 reserved1[2]; /* +34 reserved, must be 0 */
	uint32_t reserved2[7]; /* +36 reserved, must be 0 */
	/* total 64 bytes */
};

struct psp_mb_status {
	uint32_t drv_version;
	uint32_t fw_id;
	uint32_t status;
	bool     vf_gate_enabled;
};

enum psp_status amdgv_psp_cmd_km_init(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_cmd_km_fini(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_cmd_km_start(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_cmd_km_allocate_buf(struct psp_context *psp,
					      struct psp_cmd_km_handle *buf_handle);
enum psp_status amdgv_psp_cmd_km_release_buf(struct psp_context *psp,
					     struct psp_cmd_km_handle *buf_handle);
enum psp_status amdgv_psp_cmd_km_buf_prep(struct psp_context *psp, struct psp_cmd_km *km_cmd,
					  struct psp_cmd_km_handle *km_cmd_handle);
enum psp_status amdgv_psp_wait_for_memory(struct amdgv_adapter *adapt,
					  uint32_t *memory_address, uint32_t memory_value);
enum psp_status amdgv_psp_cmd_km_fence_wait(struct amdgv_adapter *adapt,
					    struct psp_context *psp,
					    struct psp_cmd_km_handle *km_cmd_handle,
					    struct psp_gfx_resp *psp_resp);
enum psp_bootloader_command_list amdgv_psp_bl_command_map(enum amdgv_firmware_id fw_id);
enum psp_gfx_fw_type amdgv_psp_cmd_km_fw_id_map(enum amdgv_firmware_id fw_id);
enum amdgv_firmware_id amdgv_psp_gfx_fw_id_map(enum psp_gfx_fw_type gfx_fw_id);
enum amdgv_firmware_id amdgv_psp_fw_id_map(enum fwman_fw_id man_fw_id);
bool amdgv_psp_fw_id_support(uint32_t ucode_id);
enum psp_status amdgv_psp_tmr_init(struct amdgv_adapter *adapt, uint32_t tmr_size);
enum psp_status amdgv_psp_tmr_fini(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_tmr_load(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_fw_mem_init(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_fw_mem_fini(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_ras_mem_init(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_ras_mem_fini(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_xgmi_mem_init(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_xgmi_mem_fini(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_ring_km_submit(struct amdgv_adapter *adapt, uint64_t cmd_buf_mc_addr,
					 uint64_t fence_mc_addr, uint32_t fence_value);
enum psp_status amdgv_psp_cmd_km_submit(struct amdgv_adapter *adapt,
					struct psp_cmd_km *input_index,
					struct psp_gfx_resp *psp_resp);
enum psp_status amdgv_psp_ring_init(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_ring_fini(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_wait_for_register(struct amdgv_adapter *adapt, uint32_t reg_index,
					    uint32_t reg_value, uint32_t reg_mask,
					    bool check_changed);
void amdgv_psp_get_fw_info(uint32_t image_version, char *info, uint32_t size, uint32_t fw_id);
enum psp_status amdgv_psp_load_np_fw(struct amdgv_adapter *adapt, unsigned char *fw_image,
				     uint32_t fw_image_size, uint32_t fw_id);
enum psp_status amdgv_psp_load_toc(struct amdgv_adapter *adapt, unsigned char *toc_image,
				   uint32_t toc_size);
enum psp_status amdgv_psp_load_fw(struct amdgv_adapter *adapt, unsigned char *fw_image,
				  uint32_t fw_image_size, enum amdgv_firmware_id ucode_id);
enum psp_status amdgv_psp_load_local_fw(struct amdgv_adapter *adapt,
					enum amdgv_firmware_id ucode_id, unsigned char *embedded_fw_image);
enum psp_status amdgv_psp_live_update_fw(struct amdgv_adapter *adapt,
					 enum amdgv_firmware_id fw_id);
enum psp_status amdgv_psp_ras_initialize(struct amdgv_adapter *adapt,
					unsigned char *ras_image, uint32_t ras_size);
enum psp_status amdgv_psp_ras_terminate(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_ras_trigger_error(struct amdgv_adapter *adapt,
					    struct ta_ras_trigger_error_input *info);
enum psp_status amdgv_psp_ras_set_feature(struct amdgv_adapter *adapt,
					union ta_ras_cmd_input *info, bool is_enable);
enum psp_status amdgv_psp_ras_get_ta_version(struct amdgv_adapter *adapt, void *fw_image, uint32_t *ver_ptr);
bool amdgv_psp_vfgate_support(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_set_mb_int(struct amdgv_adapter *adapt, uint32_t idx_vf,
				     bool enable);
enum psp_status amdgv_psp_get_mb_int_status(struct amdgv_adapter *adapt, uint32_t idx_vf,
					struct psp_mb_status *mb_status);
uint32_t amdgv_psp_ta_version(struct amdgv_adapter *adapt, void *fw_image, char *name);

enum psp_status amdgv_psp_xgmi_load(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_xgmi_invoke(struct amdgv_adapter *adapt, uint32_t ta_cmd_id,
				      uint32_t session_id);

enum psp_status amdgv_psp_xgmi_initialize(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_xgmi_get_node_id(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_xgmi_get_hive_id(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_xgmi_set_topology_info(struct amdgv_adapter *adapt,
						struct amdgv_hive_info *hive);
enum psp_status amdgv_psp_xgmi_get_topology_info(struct amdgv_adapter *adapt,
	struct amdgv_hive_info *hive, struct amdgv_xgmi_psp_topology_info *topology_info);
enum psp_status amdgv_psp_xgmi_get_peer_link_info(struct amdgv_adapter *adapt,
	struct amdgv_hive_info *hive, struct amdgv_xgmi_psp_link_info *link_info);

enum psp_status amdgv_psp_load_fw_work(struct amdgv_adapter *adapt, unsigned char *fw_image,
				       uint32_t fw_image_size,
				       enum amdgv_firmware_id ucode_id);

enum psp_status amdgv_psp_clear_vf_fw(struct amdgv_adapter *adapt,
				    uint32_t idx_vf);
enum psp_status amdgv_psp_vf_cmd_relay(struct amdgv_adapter *adapt,
					uint32_t idx_vf);
enum psp_status amdgv_psp_copy_vf_chiplet_regs(struct amdgv_adapter *adapt, uint32_t idx_vf);
bool amdgv_psp_fw_attestation_support(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_get_fw_attestation_db_add(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_get_fw_attestation_info(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_psp_save_mb_error_record(struct amdgv_adapter *adapt, uint32_t idx_vf,
						struct psp_mb_status *mb_status);
void amdgv_psp_record_loaded_fw(struct amdgv_adapter *adapt, unsigned char *fw_image, uint32_t fw_id);
enum psp_status amdgv_psp_load_vbflash_bin(struct amdgv_adapter *adapt,
					char *buffer, uint32_t pos, uint32_t count);
enum psp_status amdgv_psp_update_spirom(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_vbflash_status(struct amdgv_adapter *adapt, uint32_t *status);
enum psp_status amdgv_psp_sw_init(struct amdgv_adapter *adapt);
enum psp_status amdgv_psp_sw_fini(struct amdgv_adapter *adapt);
#endif /* AMDGV_PSP_GFX_IF_H */
