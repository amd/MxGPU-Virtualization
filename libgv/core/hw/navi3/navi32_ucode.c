/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_mes.h>
#include "navi32_psp.h"
#include "navi32_ucode.h"


#define FRONTDOOR_LOAD


#ifdef FRONTDOOR_LOAD
#include "ucode/navi32/navi32_smu_ucode_signed.h"
#include "ucode/navi32/rs64_mes_p0_data_signed.h"
#include "ucode/navi32/rs64_mes_p0_ucode_signed.h"
#include "ucode/navi32/rs64_mes_p1_data_signed.h"
#include "ucode/navi32/rs64_mes_p1_ucode_signed.h"
#include "ucode/navi32/f32_gpm_ucode_signed.h"
#include "ucode/navi32/f32_rlcp_ucode_signed.h"
#include "ucode/navi32/f32_rlcv_ucode_signed.h"
#include "ucode/navi32/f32mt_sdma_ctl_ucode_signed.h"
#include "ucode/navi32/f32mt_sdma_ctx_ucode_signed.h"
#include "ucode/navi32/rlc_restore_list_srm_mem_signed.h"
#include "ucode/navi32/rlc_restore_list_gpm_mem_signed.h"
#include "ucode/navi32/psp_key_database_bin_navi32.h"
#include "ucode/navi32/psp_sys_drv_bin_navi32.h"
#include "ucode/navi32/psp_sos_bin_navi32.h"
#include "ucode/navi32/psp_drv_soc_navi32.h"
#include "ucode/navi32/psp_drv_intf_navi32.h"
#include "ucode/navi32/psp_drv_dbg_navi32.h"
#include "ucode/navi32/psp_toc_header_signed.h"
#include "ucode/navi32/rs64_pfp_ucode_signed.h"
#include "ucode/navi32/rs64_pfp_data_signed.h"
#include "ucode/navi32/rs64_me_ucode_signed.h"
#include "ucode/navi32/rs64_me_data_signed.h"
#include "ucode/navi32/rs64_mec_ucode_signed.h"
#include "ucode/navi32/rs64_mec_data_signed.h"
#include "ucode/navi32/imu_lx7_iram_ucode_signed.h"
#include "ucode/navi32/imu_lx7_dram_ucode_signed.h"
#include "ucode/navi32/rlcv_lx7_dram_ucode_signed.h"
#include "ucode/navi32/rlcv_lx7_iram_ucode_signed.h"
#include "ucode/navi32/rlc_lx6_iram_ucode_signed.h"
#include "ucode/navi32/rlc_lx6_dram_ucode_signed.h"
#include "ucode/navi32/psp_tos_wl_bin_navi32.h"
#include "ucode/navi32/psp_drv_ras_navi32.h"

#include "ucode/navi32/psp_sys_drv_bin_navi32_dc.h"
#include "ucode/navi32/psp_sos_bin_navi32_dc.h"
#include "ucode/navi32/psp_drv_soc_navi32_dc.h"
#include "ucode/navi32/psp_drv_intf_navi32_dc.h"
#include "ucode/navi32/psp_drv_dbg_navi32_dc.h"
#include "ucode/navi32/psp_drv_ras_navi32_dc.h"
#include "ucode/navi32/psp_tos_wl_bin_navi32_dc.h"
#else

#include "ucode/navi32/navi32_smu_ucode_unsigned.h"
#include "ucode/navi32/f32_pfp_ucode.h"
#include "ucode/navi32/f32_me_ucode.h"
#include "ucode/navi32/f32_mec_ucode.h"
#include "ucode/navi32/f32_gpm_ucode.h"
#include "ucode/navi32/rs64_mes_p0_data.h"
#include "ucode/navi32/rs64_mes_p0_ucode.h"
#include "ucode/navi32/rs64_mes_p1_data.h"
#include "ucode/navi32/rs64_mes_p1_ucode.h"
#include "ucode/navi32/f32_rlcp_ucode.h"
#include "ucode/navi32/f32_rlcv_ucode.h"
#include "ucode/navi32/f32mt_sdma_ctl_ucode.h"
#include "ucode/navi32/f32mt_sdma_ctx_ucode.h"
#include "ucode/navi32/rlc_restore_list_srm_mem.h"
#include "ucode/navi32/rlc_restore_list_gpm_mem.h"
#include "ucode/navi32/rlc_xt1_iram0.h"
#include "ucode/navi32/rlc_xt1_dram0.h"
#include "ucode/navi32/imu_lx7_iram_ucode.h"
#include "ucode/navi32/imu_lx7_dram_ucode.h"
#include "ucode/navi32/rlc_lx6_iram_ucode.h"
#include "ucode/navi32/rlc_lx6_dram_ucode.h"
#include "ucode/navi32/rs64_pfp_ucode.h"
#include "ucode/navi32/rs64_pfp_data.h"
#include "ucode/navi32/rs64_me_ucode.h"
#include "ucode/navi32/rs64_me_data.h"
#include "ucode/navi32/rs64_mec_ucode.h"
#include "ucode/navi32/rs64_mec_data.h"

#endif

#include "ucode/navi32/mmschfw_signed.h"
#include "ucode/navi32/dfc_fw_signed.h"

#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include <navi3/NBIO/nbio_4_3_0_offset.h>

/* MP Apertures */
#define MP0_Public			0x03800000
#define MP0_SRAM			0x03900000
#define MP1_Private			0x03a00000
#define MP1_Public			0x03b00000
#define MP1_SRAM			0x03c00004

/* address block */
#define smnMP1_FIRMWARE_FLAGS		0x3010024
#define smnMP0_FW_INTF			0x30101c0
#define smnMP1_PUB_CTRL			0x3010b14
#define MP1_SMN_PUB_CTRL__RESET_MASK 0x00000001L

#define smnMP1_SEC_SCRATCH0             0x32000f4
#define smnMP1_SOFT_RESET_CTRL          0x32000c4
#define smnMP1_LX3_PDEBUGPC             0x3b10028
#define smnMP1_PUB_SCRATCH0             0x3b10088
#define smnMP1_EXT_SCRATCH0             0x3b10d00

#define RLCG_UCODE_LOADING_START_ADDRESS	0x00002000L

#define LM_PSP_BL_VERSION 0xDC

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

void navi32_log_toc_version(struct amdgv_adapter *adapt)
{
	AMDGV_INFO("PSP: RLC TOC(version:%d) is loaded.\n", RLC_TOC_FORMAT_VERSION_USED);
}

static int navi32_ucode_load(struct amdgv_adapter *adapt,
		enum amdgv_firmware_id *ucode_id_list, uint32_t ucode_id_count)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
#ifdef FRONTDOOR_LOAD
	uint32_t i;

	for (i = 0; i < ucode_id_count; i++) {
		switch (ucode_id_list[i]) {
		case AMDGV_FIRMWARE_ID__PSP_KEYDB:
			ret = amdgv_psp_load_fw(adapt,
				psp_key_database_bin_navi32,
				sizeof(psp_key_database_bin_navi32),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__PSP_SYS:
			if ((adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_BL] & 0xFF) == LM_PSP_BL_VERSION) {
				ret = amdgv_psp_load_fw(adapt,
							psp_sys_drv_bin_navi32_dc,
							sizeof(psp_sys_drv_bin_navi32_dc),
							ucode_id_list[i]);
			} else {
				ret = amdgv_psp_load_fw(adapt,
							psp_sys_drv_bin_navi32,
							sizeof(psp_sys_drv_bin_navi32),
							ucode_id_list[i]);
			}
			break;
		case AMDGV_FIRMWARE_ID__PSP_RAS:
			if ((adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_BL] & 0xFF) == LM_PSP_BL_VERSION) {
				ret = amdgv_psp_load_fw(adapt,
							psp_drv_ras_navi32_dc,
							sizeof(psp_drv_ras_navi32_dc),
							ucode_id_list[i]);
			} else {
				ret = amdgv_psp_load_fw(adapt,
							psp_drv_ras_navi32,
							sizeof(psp_drv_ras_navi32),
							ucode_id_list[i]);
			}
			break;
		case AMDGV_FIRMWARE_ID__PSP_SOS:
			if ((adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_BL] & 0xFF) == LM_PSP_BL_VERSION) {
				ret = amdgv_psp_load_fw(adapt,
							psp_sos_bin_navi32_dc,
							sizeof(psp_sos_bin_navi32_dc),
							ucode_id_list[i]);
			} else {
				ret = amdgv_psp_load_fw(adapt,
							psp_sos_bin_navi32,
							sizeof(psp_sos_bin_navi32),
							ucode_id_list[i]);
			}
			break;
		case AMDGV_FIRMWARE_ID__PSP_SOC:
			if ((adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_BL] & 0xFF) == LM_PSP_BL_VERSION) {
				ret = amdgv_psp_load_fw(adapt,
							psp_drv_soc_navi32_dc,
							sizeof(psp_drv_soc_navi32_dc),
							ucode_id_list[i]);
			} else {
				ret = amdgv_psp_load_fw(adapt,
							psp_drv_soc_navi32,
							sizeof(psp_drv_soc_navi32),
							ucode_id_list[i]);
			}
			break;
		case AMDGV_FIRMWARE_ID__PSP_INTF:
			if ((adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_BL] & 0xFF) == LM_PSP_BL_VERSION) {
				ret = amdgv_psp_load_fw(adapt,
							psp_drv_intf_navi32_dc,
							sizeof(psp_drv_intf_navi32_dc),
							ucode_id_list[i]);
			} else {
				ret = amdgv_psp_load_fw(adapt,
							psp_drv_intf_navi32,
							sizeof(psp_drv_intf_navi32),
							ucode_id_list[i]);
			}
			break;
		case AMDGV_FIRMWARE_ID__PSP_DBG:
			if ((adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_BL] & 0xFF) == LM_PSP_BL_VERSION) {
				ret = amdgv_psp_load_fw(adapt,
							psp_drv_dbg_navi32_dc,
							sizeof(psp_drv_dbg_navi32_dc),
							ucode_id_list[i]);
			} else {
				ret = amdgv_psp_load_fw(adapt,
							psp_drv_dbg_navi32,
							sizeof(psp_drv_dbg_navi32),
							ucode_id_list[i]);
			}
			break;
		case AMDGV_FIRMWARE_ID__SMU:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)navi32_smc_firmware,
				sizeof(navi32_smc_firmware),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__PSP_TOC:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *) TOC_TABLE,
				sizeof(TOC_TABLE),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__IMU_DRAM:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aIMU_LX7_DRAM_UCODE,
				sizeof(aIMU_LX7_DRAM_UCODE),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__IMU_IRAM:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aIMU_LX7_IRAM_UCODE,
				sizeof(aIMU_LX7_IRAM_UCODE),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRLC_Ucode,
				sizeof(aRLC_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_V:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRLCV_Ucode,
				sizeof(aRLCV_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_P:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRLCP_Ucode,
				sizeof(aRLCP_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__CP_MES:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_MES_P0_UCODE,
				sizeof(aRS64_MES_P0_UCODE),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__MES_STACK:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_MES_P0_DATA,
				sizeof(aRS64_MES_P0_DATA),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__MES_THREAD1:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_KIQ_API_P1_UCODE,
				sizeof(aRS64_KIQ_API_P1_UCODE),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__MES_THREAD1_STACK:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_KIQ_API_P1_DATA,
				sizeof(aRS64_KIQ_API_P1_DATA),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aF32MT_SDMA0_CTX_Ucode,
				sizeof(aF32MT_SDMA0_CTX_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aF32MT_SDMA0_CTL_Ucode,
				sizeof(aF32MT_SDMA0_CTL_Ucode),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLX6:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aLX6_IRAM_UCODE,
				sizeof(aLX6_IRAM_UCODE),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aLX6_DRAM_UCODE,
				sizeof(aLX6_DRAM_UCODE),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
			ret = amdgv_psp_load_fw(adapt,
			(unsigned char *)aRLC_RESTORE_LIST_GPM_MEM_SIGNED,
				sizeof(aRLC_RESTORE_LIST_GPM_MEM_SIGNED),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
			ret = amdgv_psp_load_fw(adapt,
			(unsigned char *)aRLC_RESTORE_LIST_SRM_MEM_SIGNED,
				sizeof(aRLC_RESTORE_LIST_SRM_MEM_SIGNED),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RS64_ME_UCODE:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_GFX_ME_PRODUCTION_UCODE,
				sizeof(aRS64_GFX_ME_PRODUCTION_UCODE),
				ucode_id_list[i]);
			break;
		/* all pipes use same data */
		case AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA:
		case AMDGV_FIRMWARE_ID__RS64_ME_P1_DATA:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_GFX_ME_PRODUCTION_DATA,
				sizeof(aRS64_GFX_ME_PRODUCTION_DATA),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RS64_PFP_UCODE:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_GFX_PFP_PRODUCTION_UCODE,
				sizeof(aRS64_GFX_PFP_PRODUCTION_UCODE),
				ucode_id_list[i]);
			break;
		/* all pipes use same data */
		case AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA:
		case AMDGV_FIRMWARE_ID__RS64_PFP_P1_DATA:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_GFX_PFP_PRODUCTION_DATA,
				sizeof(aRS64_GFX_PFP_PRODUCTION_DATA),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_MEC_PRODUCTION_UCODE,
				sizeof(aRS64_MEC_PRODUCTION_UCODE),
				ucode_id_list[i]);
			break;
		/* all pipes use same data */
		case AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA:
		case AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA:
		case AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA:
		case AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRS64_MEC_PRODUCTION_DATA,
				sizeof(aRS64_MEC_PRODUCTION_DATA),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRLCV_IRAM_UCODE,
				sizeof(aRLCV_IRAM_UCODE),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aRLCV_DRAM_UCODE,
				sizeof(aRLCV_DRAM_UCODE),
				ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__MMSCH:
			ret = amdgv_psp_load_fw(adapt,
					(unsigned char *)a_MMSCH_SIGNED_Ucode,
					sizeof(a_MMSCH_SIGNED_Ucode),
					(ucode_id_list[i]));
			break;
		case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
			if ((adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_BL] & 0xFF) == LM_PSP_BL_VERSION) {
				ret = amdgv_psp_load_fw(adapt,
							psp_tos_wl_bin_navi32_dc,
							sizeof(psp_tos_wl_bin_navi32_dc),
							ucode_id_list[i]);
			} else {
				ret = amdgv_psp_load_fw(adapt,
							(unsigned char *)psp_tos_wl_bin_navi32,
							sizeof(psp_tos_wl_bin_navi32),
							ucode_id_list[i]);
			}
			break;
		case AMDGV_FIRMWARE_ID__DFC_FW:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aDFC_FW,
				sizeof(aDFC_FW),
				(ucode_id_list[i]));
			break;
		default:
			AMDGV_WARN("Unsupported FW id 0x%x. Skipping...\n",
				ucode_id_list[i]);
			break;
		}
		if (ret != PSP_STATUS__SUCCESS)
			return AMDGV_FAILURE;
	}
#endif
	return ret;
}

#ifndef FRONTDOOR_LOAD
static int navi32_cp_pfp_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t i, tmp;
	uint32_t fw_ucode_gpu_addr;
	void *fw_ucode_cpu_addr;
	uint32_t *fw_data = (uint32_t *)aF32_PFP_Ucode;
	uint32_t fw_size = sizeof(aF32_PFP_Ucode);
	uint32_t usec_timeout = 50000;  /* wait for 50ms */
	uint32_t jt_offset = F32_PFP_UCODE_JT_OFFSET;
	uint32_t jt_size = F32_PFP_UCODE_JT_SIZE;

	fw_ucode_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_pfp_fw);
	fw_ucode_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_pfp_fw);

	oss_memcpy(fw_ucode_cpu_addr, (void *)fw_data, fw_size);

	AMDGV_INFO("PFP fw in GPU address is 0x%x\n", *(uint32_t *)(fw_ucode_cpu_addr));

	/* Trigger an invalidation of the L1 instruction caches */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_OP_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_PFP_IC_OP_CNTL, INVALIDATE_CACHE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_OP_CNTL), tmp);
	AMDGV_INFO("[CZ] navi32_cp_load_pfp_microcode 1");
	/* Wait for invalidation complete */
	for (i = 0; i < usec_timeout; i++) {
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_OP_CNTL));
		if (1 == REG_GET_FIELD(tmp, CP_PFP_IC_OP_CNTL,
			INVALIDATE_CACHE_COMPLETE))
			break;
		oss_udelay(1);
	}
	AMDGV_INFO("[CZ] navi32_cp_load_pfp_microcode 2");
	if (i >= usec_timeout) {
		AMDGV_ERROR("failed to invalidate instruction cache\n");
		return AMDGV_FAILURE;
	}
	AMDGV_INFO("[CZ] navi32_cp_load_pfp_microcode 3");

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_BASE_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_PFP_IC_BASE_CNTL, VMID, 0);
	tmp = REG_SET_FIELD(tmp, CP_PFP_IC_BASE_CNTL, CACHE_POLICY, 0);
	tmp = REG_SET_FIELD(tmp, CP_PFP_IC_BASE_CNTL, EXE_DISABLE, 0);
	tmp = REG_SET_FIELD(tmp, CP_PFP_IC_BASE_CNTL, ADDRESS_CLAMP, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_BASE_CNTL), tmp);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_BASE_LO),
		fw_ucode_gpu_addr & 0xFFFFF000);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_BASE_HI), upper_32_bits(fw_ucode_gpu_addr));

	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_HYP_PFP_UCODE_ADDR), 0);

	for (i = 0; i < jt_size; i++)
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_HYP_PFP_UCODE_DATA), *(fw_data + jt_offset + i));

	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_HYP_PFP_UCODE_ADDR), 28);
	AMDGV_INFO("[CZ]] navi32_cp_load_pfp_microcode 4");

	return 0;
}

static int navi32_cp_me_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t i, tmp;
	uint32_t fw_ucode_gpu_addr;
	void  *fw_ucode_cpu_addr;
	uint32_t *fw_data = (uint32_t *)aF32_ME_Ucode;
	uint32_t fw_size = sizeof(aF32_ME_Ucode);
	uint32_t usec_timeout = 50000;  /* wait for 50ms */
	uint32_t jt_offset = F32_ME_UCODE_JT_OFFSET;
	uint32_t jt_size = F32_ME_UCODE_JT_SIZE;

	fw_ucode_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_me_fw);
	fw_ucode_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_me_fw);

	oss_memcpy(fw_ucode_cpu_addr, (void *)fw_data, fw_size);

	AMDGV_INFO("ME fw in GPU address is 0x%x\n", *(uint32_t *)(fw_ucode_cpu_addr));

	/* Trigger an invalidation of the L1 instruction caches */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_OP_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_ME_IC_OP_CNTL, INVALIDATE_CACHE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_OP_CNTL), tmp);
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_me_microcode 1");
	/* Wait for invalidation complete */
	for (i = 0; i < usec_timeout; i++) {
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_OP_CNTL));
		if (1 == REG_GET_FIELD(tmp, CP_ME_IC_OP_CNTL,
			INVALIDATE_CACHE_COMPLETE))
			break;
		oss_udelay(1);
	}
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_me_microcode 2");
	if (i >= usec_timeout) {
		AMDGV_ERROR("failed to invalidate instruction cache\n");
		return AMDGV_FAILURE;
	}

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_BASE_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_ME_IC_BASE_CNTL, VMID, 0);
	tmp = REG_SET_FIELD(tmp, CP_ME_IC_BASE_CNTL, CACHE_POLICY, 0);
	tmp = REG_SET_FIELD(tmp, CP_ME_IC_BASE_CNTL, EXE_DISABLE, 0);
	tmp = REG_SET_FIELD(tmp, CP_ME_IC_BASE_CNTL, ADDRESS_CLAMP, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_BASE_LO),
		fw_ucode_gpu_addr & 0xFFFFF000);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_BASE_HI), upper_32_bits(fw_ucode_gpu_addr));

	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_HYP_ME_UCODE_ADDR), 0);
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_me_microcode 3");
	for (i = 0; i < jt_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_HYP_ME_UCODE_DATA),
			     *(fw_data + jt_offset + i));
	}
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_me_microcode 4");
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_HYP_ME_UCODE_ADDR), 29);
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_me_microcode 5");
	return 0;
}

static int navi32_cp_mec1_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t i, tmp;
	uint32_t fw_ucode_gpu_addr;
	void *fw_ucode_cpu_addr;
	uint32_t *fw_data = (uint32_t *)aF32_MEC_Ucode;
	uint32_t fw_size = sizeof(aF32_MEC_Ucode);
	uint32_t usec_timeout = 50000;  /* wait for 50ms */
	uint32_t jt_offset = F32_MEC_UCODE_JT_OFFSET;
	uint32_t jt_size = F32_MEC_UCODE_JT_SIZE;

	fw_ucode_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_mec1_fw);
	fw_ucode_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_mec1_fw);

	oss_memcpy(fw_ucode_cpu_addr, (void *)fw_data, fw_size);

	AMDGV_INFO("MEC1 fw in GPU address is 0x%x\n", *(uint32_t *)(fw_ucode_cpu_addr));

	/* Trigger an invalidation of the L1 instruction caches */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_OP_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_CPC_IC_OP_CNTL, INVALIDATE_CACHE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_OP_CNTL), tmp);
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_mec1_microcode 1");

	/* Wait for invalidation complete */
	for (i = 0; i < usec_timeout; i++) {
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_OP_CNTL));
		if (1 == REG_GET_FIELD(tmp, CP_CPC_IC_OP_CNTL,
				       INVALIDATE_CACHE_COMPLETE))
			break;
		oss_udelay(1);
	}
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_mec1_microcode 2");
	if (i >= usec_timeout) {
		AMDGV_ERROR("failed to invalidate instruction cache\n");
		return AMDGV_FAILURE;
	}

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_BASE_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_CPC_IC_BASE_CNTL, CACHE_POLICY, 0);
	tmp = REG_SET_FIELD(tmp, CP_CPC_IC_BASE_CNTL, EXE_DISABLE, 0);
	tmp = REG_SET_FIELD(tmp, CP_CPC_IC_BASE_CNTL, ADDRESS_CLAMP, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_BASE_CNTL), tmp);

	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_BASE_LO), fw_ucode_gpu_addr &
		     0xFFFFF000);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_BASE_HI),
		     upper_32_bits(fw_ucode_gpu_addr));

	/* MEC1 */
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_ME1_UCODE_ADDR), 0);
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_mec1_microcode 3");

	for (i = 0; i < jt_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_ME1_UCODE_DATA),
			     *(fw_data + jt_offset + i));
	}
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_mec1_microcode 4");
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_ME1_UCODE_ADDR), 24);
	AMDGV_INFO("[CZ] navi32_cp_gfx_load_mec1_microcode 5");

	return 0;
}

static int navi32_cp_pfp_load_ucode_rs64(struct amdgv_adapter *adapt)
{
	uint32_t *fw_ucode, *fw_data;
	uint32_t i, fw_ucode_size, fw_data_size;
	uint32_t pipe_id = 0;
	uint32_t tmp;
	uint32_t usec_timeout = 50000;  /* wait for 50ms */
	uint32_t pfp_ucode_gpu_addr;
	void *pfp_ucode_cpu_addr;
	uint32_t pfp_data_gpu_addr[2];
	void *pfp_data_cpu_addr[2];
	uint64_t pfp_uc_start_addr = RS64_GFX_PFP_PRODUCTION_UC_START_ADDR_LO |
		((uint64_t)(RS64_GFX_PFP_PRODUCTION_UC_START_ADDR_HI) << 32);

	/* instruction */
	fw_ucode = (uint32_t *)aRS64_GFX_PFP_PRODUCTION_UCODE;
	fw_ucode_size = sizeof(aRS64_GFX_PFP_PRODUCTION_UCODE);
	/* data */
	fw_data = (uint32_t *)aRS64_GFX_PFP_PRODUCTION_DATA;
	fw_data_size = sizeof(aRS64_GFX_PFP_PRODUCTION_DATA);

	pfp_ucode_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_rs64_pfp_ucode);
	pfp_ucode_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_rs64_pfp_ucode);

	for (i = 0; i < 2; ++i) {
		pfp_data_gpu_addr[i] = amdgv_memmgr_get_gpu_addr(adapt->mem_rs64_pfp_data[i]);
		pfp_data_cpu_addr[i] = amdgv_memmgr_get_cpu_addr(adapt->mem_rs64_pfp_data[i]);
	}

	oss_memcpy(pfp_ucode_cpu_addr, (void *)fw_ucode, fw_ucode_size);

	for (i = 0; i < 2; ++i) {
		oss_memcpy(pfp_data_cpu_addr[i], (void *)fw_data, fw_data_size);
	}

	AMDGV_INFO("PFP rs64 ucode in GPU address is 0x%x\n", *(uint32_t *)(pfp_ucode_cpu_addr));

	for (i = 0; i < 2; ++i) {
		AMDGV_INFO("PFP rs64 data %d in GPU address is 0x%x\n", i, *((uint32_t *)(pfp_data_cpu_addr[i]) + 22));
	}

	//select RS64 processors
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_GFX_CNTL, ENGINE_SEL, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_CNTL), tmp);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, PFP_INVALIDATE_ICACHE, 1);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, ME_INVALIDATE_ICACHE, 1);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, PFP_PIPE0_RESET, 1);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, PFP_PIPE1_RESET, 1);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, ME_PIPE0_RESET, 1);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, ME_PIPE1_RESET, 1);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, PFP_HALT, 1);
	tmp = REG_SET_FIELD(tmp, CP_ME_CNTL, ME_HALT, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL), tmp);

	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_BASE_LO), lower_32_bits(pfp_ucode_gpu_addr));
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_BASE_HI), upper_32_bits(pfp_ucode_gpu_addr));

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_BASE_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_PFP_IC_BASE_CNTL, VMID, 0);
	tmp = REG_SET_FIELD(tmp, CP_PFP_IC_BASE_CNTL, CACHE_POLICY, 0);
	tmp = REG_SET_FIELD(tmp, CP_PFP_IC_BASE_CNTL, EXE_DISABLE, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_BASE_CNTL), tmp);

	AMDGV_INFO("[CZ] navi32_cp_pfp_load_ucode_rs64 1");
	/*
	 * Programming any of the CP_PFP_IC_BASE registers
	 * forces invalidation of the ME L1 I$. Wait for the
	 * invalidation complete
	 */
	for (i = 0; i < usec_timeout; i++) {
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_OP_CNTL));
		if (1 == REG_GET_FIELD(tmp, CP_PFP_IC_OP_CNTL,
			INVALIDATE_CACHE_COMPLETE))
			break;
		oss_udelay(1);
	}

	if (i >= usec_timeout) {
		AMDGV_ERROR("failed to invalidate instruction cache\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("[CZ] navi32_cp_pfp_load_ucode_rs64 2");
	/* Prime the L1 instruction caches */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_OP_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_PFP_IC_OP_CNTL, PRIME_ICACHE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_OP_CNTL), tmp);
	/* Waiting for cache primed*/
	for (i = 0; i < usec_timeout; i++) {
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_IC_OP_CNTL));
		if (1 == REG_GET_FIELD(tmp, CP_PFP_IC_OP_CNTL,
			ICACHE_PRIMED))
			break;
		oss_udelay(1);
	}

	if (i >= usec_timeout) {
		AMDGV_ERROR("failed to prime instruction cache\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("[CZ] navi32_cp_pfp_load_ucode_rs64 3");

	AMDGV_INFO("[CZ] pfp_uc_start_addr = 0x%llx\n", pfp_uc_start_addr);
	pfp_uc_start_addr = pfp_uc_start_addr >> 2;

	for (pipe_id = 0; pipe_id < 2; ++pipe_id) {
		navi32_grbm_select(adapt, 0, pipe_id, 0, 0);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_PRGRM_CNTR_START), lower_32_bits(pfp_uc_start_addr));
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_PRGRM_CNTR_START_HI), upper_32_bits(pfp_uc_start_addr));

		AMDGV_INFO("[CZ] navi32_cp_pfp_load_ucode_rs64 4");
		/*
			* Program CP_ME_CNTL to reset given PIPE to take
			* effect of CP_PFP_PRGRM_CNTR_START.
			*/
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL));
		if (pipe_id == 0)
			tmp = REG_SET_FIELD(tmp, CP_ME_CNTL,
					PFP_PIPE0_RESET, 1);
		else
			tmp = REG_SET_FIELD(tmp, CP_ME_CNTL,
					PFP_PIPE1_RESET, 1);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL), tmp);

		/* Clear pfp pipe0 reset bit. */
		if (pipe_id == 0)
			tmp = REG_SET_FIELD(tmp, CP_ME_CNTL,
					PFP_PIPE0_RESET, 0);
		else
			tmp = REG_SET_FIELD(tmp, CP_ME_CNTL,
					PFP_PIPE1_RESET, 0);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL), tmp);

		AMDGV_INFO("[CZ] navi32_cp_pfp_load_ucode_rs64 5");

		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_BASE0_LO), lower_32_bits(pfp_data_gpu_addr[pipe_id]));
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_BASE0_HI), upper_32_bits(pfp_data_gpu_addr[pipe_id]));

		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_BASE_CNTL));
		tmp = REG_SET_FIELD(tmp, CP_GFX_RS64_DC_BASE_CNTL, VMID, 0);
		tmp = REG_SET_FIELD(tmp, CP_GFX_RS64_DC_BASE_CNTL, CACHE_POLICY, 0);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_BASE_CNTL), tmp);

		AMDGV_INFO("[CZ] navi32_cp_pfp_load_ucode_rs64 6");

		/* Invalidate the data caches */
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_OP_CNTL));
		tmp = REG_SET_FIELD(tmp, CP_GFX_RS64_DC_OP_CNTL, INVALIDATE_DCACHE, 1);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_OP_CNTL), tmp);

		for (i = 0; i < usec_timeout; i++) {
			tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_OP_CNTL));
			if (1 == REG_GET_FIELD(tmp, CP_GFX_RS64_DC_OP_CNTL,
				INVALIDATE_DCACHE_COMPLETE))
				break;
			oss_udelay(1);
		}

		if (i >= usec_timeout) {
			AMDGV_ERROR("failed to invalidate RS64 data cache\n");
			return AMDGV_FAILURE;
		}
	}
	navi32_grbm_select(adapt, 0, 0, 0, 0);


	AMDGV_INFO("[CZ] navi32_cp_pfp_load_ucode_rs64 7");

	return 0;
}

static int navi32_cp_me_load_ucode_rs64(struct amdgv_adapter *adapt)
{
	uint32_t *fw_ucode, *fw_data;
	uint32_t i, fw_ucode_size, fw_data_size;
	uint32_t pipe_id = 0;
	uint32_t tmp;
	uint32_t usec_timeout = 50000;  /* wait for 50ms */
	uint32_t me_ucode_gpu_addr;
	void *me_ucode_cpu_addr;
	uint32_t me_data_gpu_addr[2];
	void *me_data_cpu_addr[2];
	uint64_t me_uc_start_addr = RS64_GFX_ME_PRODUCTION_UC_START_ADDR_LO |
		((uint64_t)(RS64_GFX_ME_PRODUCTION_UC_START_ADDR_HI) << 32);

	/* instruction */
	fw_ucode = (uint32_t *)aRS64_GFX_ME_PRODUCTION_UCODE;
	fw_ucode_size = sizeof(aRS64_GFX_ME_PRODUCTION_UCODE);
	/* data */
	fw_data = (uint32_t *)aRS64_GFX_ME_PRODUCTION_DATA;
	fw_data_size = sizeof(aRS64_GFX_ME_PRODUCTION_DATA);

	me_ucode_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_rs64_me_ucode);
	me_ucode_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_rs64_me_ucode);

	for (i = 0; i < 2; ++i) {
		me_data_gpu_addr[i] = amdgv_memmgr_get_gpu_addr(adapt->mem_rs64_me_data[i]);
		me_data_cpu_addr[i] = amdgv_memmgr_get_cpu_addr(adapt->mem_rs64_me_data[i]);
	}

	oss_memcpy(me_ucode_cpu_addr, (void *)fw_ucode, fw_ucode_size);

	for (i = 0; i < 2; ++i) {
		oss_memcpy(me_data_cpu_addr[i], (void *)fw_data, fw_data_size);
	}

	AMDGV_INFO("ME rs64 ucode in GPU address is 0x%x\n", *(uint32_t *)(me_ucode_cpu_addr));

	for (i = 0; i < 2; ++i) {
		AMDGV_INFO("ME rs64 data in GPU address is 0x%x\n", *((uint32_t *)(me_data_cpu_addr[i]) + 22));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_BASE_LO), lower_32_bits(me_ucode_gpu_addr));
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_BASE_HI), upper_32_bits(me_ucode_gpu_addr));

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_BASE_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_ME_IC_BASE_CNTL, VMID, 0);
	tmp = REG_SET_FIELD(tmp, CP_ME_IC_BASE_CNTL, CACHE_POLICY, 0);
	tmp = REG_SET_FIELD(tmp, CP_ME_IC_BASE_CNTL, EXE_DISABLE, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_BASE_CNTL), tmp);

	AMDGV_INFO("[CZ] navi32_cp_me_load_ucode_rs64 1");

	/*
	 * Programming any of the CP_ME_IC_BASE registers
	 * forces invalidation of the ME L1 I$. Wait for the
	 * invalidation complete
	 */
	for (i = 0; i < usec_timeout; i++) {
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_OP_CNTL));
		if (1 == REG_GET_FIELD(tmp, CP_ME_IC_OP_CNTL,
			INVALIDATE_CACHE_COMPLETE))
			break;
		oss_udelay(1);
	}

	if (i >= usec_timeout) {
		AMDGV_ERROR("failed to invalidate instruction cache\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("[CZ] navi32_cp_me_load_ucode_rs64 2");

	/* Prime the instruction caches */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_OP_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_ME_IC_OP_CNTL, PRIME_ICACHE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_OP_CNTL), tmp);

	/* Waiting for instruction cache primed*/
	for (i = 0; i < usec_timeout; i++) {
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_IC_OP_CNTL));
		if (1 == REG_GET_FIELD(tmp, CP_ME_IC_OP_CNTL,
			ICACHE_PRIMED))
			break;
		oss_udelay(1);
	}

	if (i >= usec_timeout) {
		AMDGV_ERROR("failed to prime instruction cache\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("[CZ] navi32_cp_me_load_ucode_rs64 3");

	AMDGV_INFO("[CZ] me_uc_start_addr = 0x%llx\n", me_uc_start_addr);
	me_uc_start_addr = me_uc_start_addr >> 2;

	for (pipe_id = 0; pipe_id < 2; ++pipe_id) {
		navi32_grbm_select(adapt, 0, pipe_id, 0, 0);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_PRGRM_CNTR_START), lower_32_bits(me_uc_start_addr));
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_PRGRM_CNTR_START_HI), upper_32_bits(me_uc_start_addr));

		AMDGV_INFO("[CZ] navi32_cp_me_load_ucode_rs64 4");

		/*
			* Program CP_ME_CNTL to reset given PIPE to take
			* effect of CP_PFP_PRGRM_CNTR_START.
			*/
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL));
		if (pipe_id == 0)
			tmp = REG_SET_FIELD(tmp, CP_ME_CNTL,
					ME_PIPE0_RESET, 1);
		else
			tmp = REG_SET_FIELD(tmp, CP_ME_CNTL,
					ME_PIPE1_RESET, 1);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL), tmp);

		/* Clear pfp pipe0 reset bit. */
		if (pipe_id == 0)
			tmp = REG_SET_FIELD(tmp, CP_ME_CNTL,
					ME_PIPE0_RESET, 0);
		else
			tmp = REG_SET_FIELD(tmp, CP_ME_CNTL,
					ME_PIPE1_RESET, 0);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_CNTL), tmp);

		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_BASE1_LO), lower_32_bits(me_data_gpu_addr[pipe_id]));
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_BASE1_HI), upper_32_bits(me_data_gpu_addr[pipe_id]));

		AMDGV_INFO("[CZ] navi32_cp_me_load_ucode_rs64 5");

		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_BASE_CNTL));
		tmp = REG_SET_FIELD(tmp, CP_GFX_RS64_DC_BASE_CNTL, VMID, 0);
		tmp = REG_SET_FIELD(tmp, CP_GFX_RS64_DC_BASE_CNTL, CACHE_POLICY, 0);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_BASE_CNTL), tmp);

		AMDGV_INFO("[CZ] navi32_cp_me_load_ucode_rs64 6");
		/* Invalidate the data caches */
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_OP_CNTL));
		tmp = REG_SET_FIELD(tmp, CP_GFX_RS64_DC_OP_CNTL, INVALIDATE_DCACHE, 1);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_OP_CNTL), tmp);

		AMDGV_INFO("[CZ] navi32_cp_me_load_ucode_rs64 7");
		for (i = 0; i < usec_timeout; i++) {
			tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_GFX_RS64_DC_OP_CNTL));
			if (1 == REG_GET_FIELD(tmp, CP_GFX_RS64_DC_OP_CNTL,
				INVALIDATE_DCACHE_COMPLETE))
				break;
			oss_udelay(1);
		}

		if (i >= usec_timeout) {
			AMDGV_ERROR("failed to invalidate RS64 data cache\n");
			return AMDGV_FAILURE;
		}

		navi32_grbm_select(adapt, 0, 0, 0, 0);
	}

	AMDGV_INFO("[CZ] navi32_cp_me_load_ucode_rs64 8");
	return 0;
}

static int navi32_cp_mec1_load_ucode_rs64(struct amdgv_adapter *adapt)
{
	uint32_t *fw_ucode, *fw_data;
	uint32_t tmp, fw_ucode_size, fw_data_size;
	uint32_t i, usec_timeout = 50000; /* Wait for 50 ms */
	uint32_t mec_ucode_gpu_addr;
	void *mec_ucode_cpu_addr;
	uint32_t mec_data_gpu_addr[4];
	void *mec_data_cpu_addr[4];
	uint64_t mec_uc_start_addr = RS64_MEC_PRODUCTION_UC_START_ADDR_LO |
		((uint64_t)(RS64_MEC_PRODUCTION_UC_START_ADDR_HI) << 32);

	/* instruction */
	fw_ucode = (uint32_t *)aRS64_MEC_PRODUCTION_UCODE;
	fw_ucode_size = sizeof(aRS64_MEC_PRODUCTION_UCODE);
	/* data */
	fw_data = (uint32_t *)aRS64_MEC_PRODUCTION_DATA;
	fw_data_size = sizeof(aRS64_MEC_PRODUCTION_DATA);

	mec_ucode_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_rs64_mec1_ucode);
	mec_ucode_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_rs64_mec1_ucode);

	for (i = 0; i < 4; ++i) {
		mec_data_gpu_addr[i] = amdgv_memmgr_get_gpu_addr(adapt->mem_rs64_mec1_data[i]);
		mec_data_cpu_addr[i] = amdgv_memmgr_get_cpu_addr(adapt->mem_rs64_mec1_data[i]);
	}

	oss_memcpy(mec_ucode_cpu_addr, (void *)fw_ucode, fw_ucode_size);

	for (i = 0; i < 4; ++i) {
		oss_memcpy(mec_data_cpu_addr[i], (void *)fw_data, fw_data_size);
	}

	AMDGV_INFO("MEC rs64 ucode in GPU address is 0x%x\n", *(uint32_t *)(mec_ucode_cpu_addr));
	for (i = 0; i < 4; ++i) {
		AMDGV_INFO("MEC rs64 data in GPU address is 0x%x\n", *((uint32_t *)(mec_data_cpu_addr[i]) + 18));
	}

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_ISA_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_MEC_ISA_CNTL, ISA_MODE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_ISA_CNTL), tmp);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_INVALIDATE_ICACHE, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE0_RESET, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE1_RESET, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE2_RESET, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE3_RESET, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE0_ACTIVE, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE1_ACTIVE, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE2_ACTIVE, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE3_ACTIVE, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_HALT, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_CNTL), tmp);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_BASE_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_CPC_IC_BASE_CNTL, VMID, 0);
	tmp = REG_SET_FIELD(tmp, CP_CPC_IC_BASE_CNTL, EXE_DISABLE, 0);
	tmp = REG_SET_FIELD(tmp, CP_CPC_IC_BASE_CNTL, CACHE_POLICY, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_BASE_CNTL), tmp);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_DC_BASE_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_MEC_DC_BASE_CNTL, VMID, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_DC_BASE_CNTL, CACHE_POLICY, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_DC_BASE_CNTL), tmp);

	AMDGV_INFO("[CZ] navi32_cp_mec1_load_ucode_rs64 1");

	AMDGV_INFO("[CZ] mec_uc_start_addr = 0x%llx\n", mec_uc_start_addr);
	mec_uc_start_addr = mec_uc_start_addr >> 2;
	for (i = 0; i < 4; i++) {
		navi32_grbm_select(adapt, 1, i, 0, 0);

		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_MDBASE_LO), lower_32_bits(mec_data_gpu_addr[i]));
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_MDBASE_HI), upper_32_bits(mec_data_gpu_addr[i]));

		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_PRGRM_CNTR_START), lower_32_bits(mec_uc_start_addr));
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_PRGRM_CNTR_START_HI), upper_32_bits(mec_uc_start_addr));

		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_BASE_LO), lower_32_bits(mec_ucode_gpu_addr));
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_BASE_HI), upper_32_bits(mec_ucode_gpu_addr));
	}

	AMDGV_INFO("[CZ] navi32_cp_mec1_load_ucode_rs64 2");

	/* Trigger an invalidation of the L1 instruction caches */
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_OP_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_CPC_IC_OP_CNTL, INVALIDATE_CACHE, 1);
	tmp = REG_SET_FIELD(tmp, CP_CPC_IC_OP_CNTL, PRIME_ICACHE, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_OP_CNTL), tmp);

	/* Wait for invalidation complete */
	for (i = 0; i < usec_timeout; i++) {
		tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_CPC_IC_OP_CNTL));
		if (1 == REG_GET_FIELD(tmp, CP_CPC_IC_OP_CNTL,
				       INVALIDATE_CACHE_COMPLETE))
			break;
		oss_udelay(1);
	}

	if (i >= usec_timeout) {
		AMDGV_ERROR("failed to invalidate instruction cache\n");
		return AMDGV_FAILURE;
	}
	navi32_grbm_select(adapt, 0, 0, 0, 0);

	AMDGV_INFO("[CZ] navi32_cp_mec1_load_ucode_rs64 3");

	// /* Trigger an invalidation of the L1 instruction caches */
	// tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_DC_OP_CNTL));
	// tmp = REG_SET_FIELD(tmp, CP_MEC_DC_OP_CNTL, INVALIDATE_DCACHE, 1);
	// WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_DC_OP_CNTL), tmp);

	// /* Wait for invalidation complete */
	// for (i = 0; i < usec_timeout; i++) {
	// 	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_DC_OP_CNTL));
	// 	if (1 == REG_GET_FIELD(tmp, CP_MEC_DC_OP_CNTL,
	// 			       INVALIDATE_DCACHE_COMPLETE))
	// 		break;
	// 	oss_udelay(1);
	// }

	// if (i >= usec_timeout) {
	// 	AMDGV_ERROR("failed to invalidate instruction cache\n");
	// 	return AMDGV_FAILURE;
	// }

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_CNTL));
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_INVALIDATE_ICACHE, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE0_RESET, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE1_RESET, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE2_RESET, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE3_RESET, 0);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE0_ACTIVE, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE1_ACTIVE, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE2_ACTIVE, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_PIPE3_ACTIVE, 1);
	tmp = REG_SET_FIELD(tmp, CP_MEC_RS64_CNTL, MEC_HALT, 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_CNTL), tmp);

	AMDGV_INFO("[CZ] navi32_cp_mec1_load_ucode_rs64 4");
	return 0;
}

static int navi32_rlc_srlist_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t *fw_data;
	uint32_t fw_size;
	uint32_t tmp, i;

	/* load rlc srlist srm */
	fw_data = aRLC_RESTORE_LIST_SRM_MEM;
	fw_size = sizeof(aRLC_RESTORE_LIST_SRM_MEM)/4;

	AMDGV_INFO("navi32 load srlist 1\n");
	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_SRM_CNTL));
	tmp |= RLC_SRM_CNTL__AUTO_INCR_ADDR_MASK | RLC_SRM_CNTL__SRM_ENABLE_MASK;
	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_SRM_CNTL), tmp);

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_SRM_ARAM_ADDR), 0);
	AMDGV_INFO("navi32 load srlist 2\n");

	for (i = 0; i < fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_SRM_ARAM_DATA), *(fw_data++));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_SRM_ARAM_ADDR), 0xEF);
	AMDGV_INFO("navi32 load srlist 3\n");

	/* load rlc srlist gpm */
	fw_data = aRLC_RESTORE_LIST_GPM_MEM;
	fw_size = sizeof(aRLC_RESTORE_LIST_GPM_MEM)/4;

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_SCRATCH_ADDR), 0);
	AMDGV_INFO("navi32 load srlist 4\n");

	for (i = 0; i < fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_SCRATCH_DATA), *(fw_data++));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_SCRATCH_ADDR), 0xEF);
	AMDGV_INFO("navi32 load srlist 5\n");

	return 0;
}

static int navi32_rlcg_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t *fw_data;
	unsigned i, fw_size;

	fw_data = aF32_RLC_Ucode;
	fw_size = sizeof(aF32_RLC_Ucode)/4 ;

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_UCODE_ADDR), RLCG_UCODE_LOADING_START_ADDRESS);

	for (i = 0; i < fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_UCODE_DATA),
			     *(fw_data++));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_UCODE_ADDR), 208);

	return 0;
}

static int navi32_rlcp_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t *fw_data;
	uint32_t i, fw_size;
	uint32_t tmp;

	fw_data = aRLCP_Ucode;
	fw_size = sizeof(aRLCP_Ucode) / 4;

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_PACE_UCODE_ADDR), 0);

	for (i = 0; i < fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_PACE_UCODE_DATA),
				*(fw_data++));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_PACE_UCODE_ADDR), 19);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_THREAD_ENABLE));
	tmp = REG_SET_FIELD(tmp, RLC_GPM_THREAD_ENABLE, THREAD1_ENABLE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_THREAD_ENABLE), tmp);
	return 0;
}

static int navi32_rlcv_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t *fw_data;
	uint32_t i, fw_size;
	uint32_t tmp;
	fw_data = aRLCV_Ucode;
	fw_size = sizeof(aRLCV_Ucode) / 4;

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_UCODE_ADDR), 0);

	for (i = 0; i < fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_UCODE_DATA),
				*(fw_data++));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_UCODE_ADDR), 21);

	tmp = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_F32_CNTL));
	tmp = REG_SET_FIELD(tmp, RLC_GPU_IOV_F32_CNTL, ENABLE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_F32_CNTL), tmp);
	return 0;
}

static int navi32_rlcv_lx7_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t *iram_fw_data, *dram_fw_data;
	uint32_t i, iram_fw_size, dram_fw_size;

	iram_fw_data = aRLCV_IRAM_UCODE;
	iram_fw_size = sizeof(aRLCV_IRAM_UCODE) / 4;

	dram_fw_data = aRLCV_DRAM_UCODE;
	dram_fw_size = sizeof(aRLCV_DRAM_UCODE) / 4;

	AMDGV_INFO("[CZ] load RLCV LX7 1\n");

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_LX6_CORE1_IRAM_ADDR), 0);

	for (i = 0; i < iram_fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_LX6_CORE1_IRAM_DATA),
				*(iram_fw_data++));
	}

	AMDGV_INFO("[CZ] load RLCV LX7 2\n");

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_LX6_CORE1_DRAM_ADDR), 0);

	for (i = 0; i < dram_fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_LX6_CORE1_DRAM_DATA),
				*(dram_fw_data++));
	}

	AMDGV_INFO("[CZ] load RLCV LX7 3\n");

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_LX6_CORE1_CNTL), 0x5);
	oss_msleep(10);
	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_LX6_CORE1_CNTL), 0x4);

	AMDGV_INFO("[CZ] load RLCV LX7 4\n");

	return 0;
}

static int navi32_mes_load_ucode(struct amdgv_adapter *adapt, enum amdgv_mes_pipe pipe)
{
	uint32_t data;
	uint64_t mes_uc_addr;

	uint32_t *data_fw;
	uint32_t data_fw_size;

	uint32_t data_fw_gpu_addr;
	void *data_fw_cpu_addr;

	uint32_t *ucode_fw;
	uint32_t ucode_fw_size;

	uint32_t ucode_fw_gpu_addr;
	void *ucode_fw_cpu_addr;

	if (pipe == AMDGV_MES_SCHED_PIPE) {
		data_fw = aRS64_MES_P0_DATA_NV32;
		data_fw_size = sizeof(aRS64_MES_P0_DATA_NV32);
		data_fw_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_mes_p0_data_fw);
		data_fw_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_mes_p0_data_fw);

		oss_memcpy(data_fw_cpu_addr, (void *)data_fw, data_fw_size);

		AMDGV_INFO("MES P0 data fw in GPU address is 0x%x\n", *(uint32_t *)(data_fw_cpu_addr));

		ucode_fw = aRS64_MES_P0_UCODE;
		ucode_fw_size = sizeof(aRS64_MES_P0_UCODE);
		ucode_fw_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_mes_p0_ucode_fw);
		ucode_fw_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_mes_p0_ucode_fw);

		oss_memcpy(ucode_fw_cpu_addr, (void *)ucode_fw, ucode_fw_size);

		AMDGV_INFO("MES P0 ucode fw in GPU address is 0x%x\n", *(uint32_t *)(ucode_fw_cpu_addr));
	} else if (pipe == AMDGV_MES_KIQ_PIPE) {
		data_fw = aRS64_KIQ_API_P1_DATA;
		data_fw_size = sizeof(aRS64_KIQ_API_P1_DATA);
		data_fw_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_mes_p1_data_fw);
		data_fw_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_mes_p1_data_fw);

		oss_memcpy(data_fw_cpu_addr, (void *)data_fw, data_fw_size);

		AMDGV_INFO("MES P1 data fw in GPU address is 0x%x\n", *(uint32_t *)(data_fw_cpu_addr));

		ucode_fw = aRS64_KIQ_API_P1_UCODE;
		ucode_fw_size = sizeof(aRS64_KIQ_API_P1_UCODE);
		ucode_fw_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->mem_mes_p1_ucode_fw);
		ucode_fw_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->mem_mes_p1_ucode_fw);
		oss_memcpy(ucode_fw_cpu_addr, (void *)ucode_fw, ucode_fw_size);

		AMDGV_INFO("MES P1 ucode fw in GPU address is 0x%x\n", *(uint32_t *)(ucode_fw_cpu_addr));
	}

	/* me=3, pipe=0, queue=0 */
	navi32_grbm_select(adapt, 3, pipe, 0, 0);

	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_BASE_CNTL), 0);
	AMDGV_INFO("[CZ] navi32_mes_load_ucode 1\n");

	/* set ucode start address */
	mes_uc_addr = adapt->mes_uc_start_addr[pipe] >> 2;
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_PRGRM_CNTR_START),
		     lower_32_bits(mes_uc_addr));
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_PRGRM_CNTR_START_HI),
		     upper_32_bits(mes_uc_addr));
	AMDGV_INFO("[CZ] navi32_mes_load_ucode 2\n");

	/* set ucode fimrware address */
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_BASE_LO),
		     lower_32_bits(ucode_fw_gpu_addr));
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_BASE_HI),
		     upper_32_bits(ucode_fw_gpu_addr));

	/* set ucode instruction cache boundary to 2M-1 */
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_MIBOUND_LO), 0x1FFFFF);
	AMDGV_INFO("[CZ] navi32_mes_load_ucode 3\n");

	/* set ucode data firmware address */
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_MDBASE_LO),
		     lower_32_bits(data_fw_gpu_addr));
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_MDBASE_HI),
		     upper_32_bits(data_fw_gpu_addr));

	/* Set 0x3FFFF (256K-1) to CP_MES_MDBOUND_LO */
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_MDBOUND_LO), 0x3FFFF);
	AMDGV_INFO("[CZ] navi32_mes_load_ucode 4\n");

	/* invalidate ICACHE */
	data = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_OP_CNTL));
	data = REG_SET_FIELD(data, CP_MES_IC_OP_CNTL, PRIME_ICACHE, 0);
	data = REG_SET_FIELD(data, CP_MES_IC_OP_CNTL, INVALIDATE_CACHE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_OP_CNTL), data);
	AMDGV_INFO("[CZ] navi32_mes_load_ucode 5\n");

	/* prime the ICACHE. */
	data = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_OP_CNTL));
	data = REG_SET_FIELD(data, CP_MES_IC_OP_CNTL, PRIME_ICACHE, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_OP_CNTL), data);
	AMDGV_INFO("[CZ] navi32_mes_load_ucode 6\n");

	navi32_grbm_select(adapt, 0, 0, 0, 0);
	AMDGV_INFO("[CZ] navi32_mes_load_ucode 7\n");

	return 0;
}

static int navi32_sdma0_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t *fw_data;
	uint32_t fw_size;
	int i;

	/* load Context Switch microcode */
	fw_size = sizeof(aF32MT_SDMA0_CTX_Ucode)/4;
	fw_data = aF32MT_SDMA0_CTX_Ucode;

	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_UCODE_ADDR), 0);

	for (i = 0; i < fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_UCODE_DATA), *(fw_data++));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_UCODE_ADDR), 64);
	AMDGV_INFO("navi32 load sdma0 ucode 1\n");

	/* load Control Thread microcode */
	fw_size = sizeof(aF32MT_SDMA0_CTL_Ucode)/4;
	fw_data = aF32MT_SDMA0_CTL_Ucode;

	AMDGV_INFO("navi32 load sdma0 ucode 2\n");
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_UCODE_ADDR), 0x8000);

	for (i = 0; i < fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_UCODE_DATA), *(fw_data++));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA0_UCODE_ADDR), 64);
	AMDGV_INFO("navi32 load sdma0 ucode 3\n");

	return 0;
}

static int navi32_sdma1_load_ucode(struct amdgv_adapter *adapt)
{
	uint32_t *fw_data;
	uint32_t fw_size;
	int i;

	/* load Context Switch microcode */
	fw_size = sizeof(aF32MT_SDMA1_CTX_Ucode)/4;
	fw_data = aF32MT_SDMA1_CTX_Ucode;

	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_UCODE_ADDR), 0);

	for (i = 0; i < fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_UCODE_DATA), *(fw_data++));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_UCODE_ADDR), 64);
	AMDGV_INFO("navi32 load sdma1 ucode 1\n");

	/* load Control Thread microcode */
	fw_size = sizeof(aF32MT_SDMA1_CTL_Ucode)/4;
	fw_data = aF32MT_SDMA1_CTL_Ucode;

	AMDGV_INFO("navi32 load sdma1 ucode 2\n");
	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_UCODE_ADDR), 0x8000);

	for (i = 0; i < fw_size; i++) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_UCODE_DATA), *(fw_data++));
	}

	WREG32(SOC15_REG_OFFSET(GC, 0, regSDMA1_UCODE_ADDR), 64);
	AMDGV_INFO("navi32 load sdma1 ucode 3\n");

	return 0;
}

static int navi32_smu_load_ucode(struct amdgv_adapter *adapt)
{
	const uint32_t *src;
	uint32_t addr_start = MP1_SRAM;
	uint32_t i;
	uint32_t smc_fw_size;
	uint32_t mp1_fw_flags;

	WREG32_PCIE2(MP1_Public | (smnMP1_PUB_CTRL & 0xffffffff),
		    1 & MP1_SMN_PUB_CTRL__RESET_MASK);

	src = (const uint32_t *)navi32_smc_firmware;
	smc_fw_size = sizeof(navi32_smc_firmware);

	AMDGV_INFO("navi32 load SMU ucode 1\n");

	for (i = 1; i < smc_fw_size/4 - 1; i++) {
		WREG32_PCIE2(addr_start, src[i]);
		addr_start += 4;
	}

	AMDGV_INFO("navi32 load SMU ucode 2\n");

	WREG32_PCIE2((smnMP1_EXT_SCRATCH0 & 0xffffffff), 0xffffffef);
	WREG32_PCIE2(MP1_Private | (smnMP1_SOFT_RESET_CTRL & 0xffffffff),
		    1 & ~MP1_SMN_PUB_CTRL__RESET_MASK);
	WREG32_PCIE2(MP1_Public | (smnMP1_PUB_CTRL & 0xffffffff),
		    1 & ~MP1_SMN_PUB_CTRL__RESET_MASK);

	AMDGV_INFO("navi32 load SMU ucode 3\n");

	for (i = 0; i < MAX_USEC_TIMEOUT; i++) {
		mp1_fw_flags = RREG32_PCIE2(MP1_Public |
					   (smnMP1_FIRMWARE_FLAGS & 0xffffffff));
		if ((mp1_fw_flags & MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED_MASK) >>
		    MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED__SHIFT)
			break;
		oss_udelay(1);
	}
	AMDGV_INFO("navi32 load SMU ucode 4\n");

	if (i >= MAX_USEC_TIMEOUT)
		return AMDGV_FAILURE;
	AMDGV_INFO("navi32 load SMU ucode 5\n");

	return 0;
}
#endif

static int navi32_ucode_load_direct(struct amdgv_adapter *adapt,
		enum amdgv_firmware_id *ucode_id_list, uint32_t ucode_id_count)
{
	int ret = 0;
#ifndef FRONTDOOR_LOAD
	uint32_t i = 0;

	for (i = 0; i < ucode_id_count; i++) {
		switch (ucode_id_list[i]) {
		case AMDGV_FIRMWARE_ID__RLC:
			ret = navi32_rlcg_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__RLC_P:
			ret = navi32_rlcp_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__RLC_V:
			ret = navi32_rlcv_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__SMU:
			ret = navi32_smu_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__CP_PFP:
			ret = navi32_cp_pfp_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__CP_ME:
			ret = navi32_cp_me_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__CP_MEC1:
			ret = navi32_cp_mec1_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__CP_MES:
			ret = navi32_mes_load_ucode(adapt, AMDGV_MES_SCHED_PIPE);
			break;
		case AMDGV_FIRMWARE_ID__MES_THREAD1:
			ret = navi32_mes_load_ucode(adapt, AMDGV_MES_KIQ_PIPE);
			break;
		case AMDGV_FIRMWARE_ID__SDMA0:
			ret = navi32_sdma0_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__SDMA1:
			ret = navi32_sdma1_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__RLC_SAVE_RESTROE_LIST:
			ret = navi32_rlc_srlist_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__RLCV_LX7:
			ret = navi32_rlcv_lx7_load_ucode(adapt);
			break;
		case AMDGV_FIRMWARE_ID__RS64_PFP_UCODE:
			ret = navi32_cp_pfp_load_ucode_rs64(adapt);
			break;
		case AMDGV_FIRMWARE_ID__RS64_ME_UCODE:
			ret = navi32_cp_me_load_ucode_rs64(adapt);
			break;
		case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
			ret = navi32_cp_mec1_load_ucode_rs64(adapt);
			break;
		default:
			AMDGV_WARN("Unsupported FW id 0x%x. Skipping...\n",
				ucode_id_list[i]);
			break;
		}
	}
#endif
	return ret;
}

static int navi32_ucode_prepare_engine(struct amdgv_adapter *adapt,
		uint32_t ucode_id)
{
	uint32_t grbm_val;
	switch (ucode_id) {
	case AMDGV_FIRMWARE_ID__MES_THREAD1:

		WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), 0xc);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_PRGRM_CNTR_START),
		(RS64_MES_P0_UC_START_ADDR_LO >> 2) |
		(uint32_t)(RS64_MES_P0_UC_START_ADDR_HI << 30));
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_PRGRM_CNTR_START_HI),
		(RS64_MES_P0_UC_START_ADDR_HI >> 2));

		WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), 0xd);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_PRGRM_CNTR_START),
		(RS64_KIQ_API_P1_UC_START_ADDR_LO >> 2) |
		(uint32_t)(RS64_KIQ_API_P1_UC_START_ADDR_HI << 30));
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_PRGRM_CNTR_START_HI),
		(RS64_KIQ_API_P1_UC_START_ADDR_HI >> 2));

		grbm_val = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_OP_CNTL));
		grbm_val = REG_SET_FIELD(grbm_val, CP_MES_IC_OP_CNTL, INVALIDATE_CACHE, 1);
		grbm_val = REG_SET_FIELD(grbm_val, CP_MES_IC_OP_CNTL, PRIME_ICACHE, 0);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_OP_CNTL), grbm_val);
		grbm_val = RREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_OP_CNTL));
		grbm_val = REG_SET_FIELD(grbm_val, CP_MES_IC_OP_CNTL, PRIME_ICACHE, 1);
		WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MES_IC_OP_CNTL), grbm_val);

		WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), 0x0);
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
		/* MEC SET FOR 4 pipes */
		for (grbm_val = 0x4; grbm_val <= 0x7; grbm_val++) {
			WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), grbm_val);
			WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_PRGRM_CNTR_START),
					(RS64_MEC_PRODUCTION_UC_START_ADDR_LO >> 2) |
					(uint32_t)((RS64_MEC_PRODUCTION_UC_START_ADDR_HI & 3) << 30));
			WREG32(SOC15_REG_OFFSET(GC, 0, regCP_MEC_RS64_PRGRM_CNTR_START_HI),
					(RS64_MEC_PRODUCTION_UC_START_ADDR_HI >> 2));
		}
		WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), 0x0);
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_UCODE:
		for (grbm_val = 0x0; grbm_val <= 0x1; grbm_val++) {
			WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), grbm_val);
			WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_PRGRM_CNTR_START),
					(RS64_GFX_ME_PRODUCTION_UC_START_ADDR_LO >> 2) |
					(uint32_t)((RS64_GFX_ME_PRODUCTION_UC_START_ADDR_HI & 3) << 30));
			WREG32(SOC15_REG_OFFSET(GC, 0, regCP_ME_PRGRM_CNTR_START_HI),
					(RS64_GFX_ME_PRODUCTION_UC_START_ADDR_HI >> 2));
		}
		WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), 0x0);
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_UCODE:
		for (grbm_val = 0x0; grbm_val <= 0x1; grbm_val++) {
			WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), grbm_val);
			WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_PRGRM_CNTR_START),
					(RS64_GFX_PFP_PRODUCTION_UC_START_ADDR_LO >> 2) |
					(uint32_t)((RS64_GFX_PFP_PRODUCTION_UC_START_ADDR_HI & 3) << 30));
			WREG32(SOC15_REG_OFFSET(GC, 0, regCP_PFP_PRGRM_CNTR_START_HI),
					(RS64_GFX_PFP_PRODUCTION_UC_START_ADDR_HI >> 2));
		}
		WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), 0x0);
		break;
	default:
		AMDGV_INFO("Engine with ucode ID %d doesn't needs to prepare\n",
			ucode_id);
		break;
	}
	return 0;
}

static int navi32_ucode_sw_init(struct amdgv_adapter *adapt)
{
	if (adapt->fw_load_engine == AMDGV_FW_LOAD_DIRECT)
		adapt->ucode.load = navi32_ucode_load_direct;
	else
		adapt->ucode.load = navi32_ucode_load;

	adapt->ucode.prepare_ucode_engine = navi32_ucode_prepare_engine;
	return 0;
}

static int navi32_ucode_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->ucode.load = NULL;
	return 0;
}

static int navi32_ucode_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_ucode_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func navi32_ucode_func = {
	.name = "navi32_ucode_func",
	.sw_init = navi32_ucode_sw_init,
	.sw_fini = navi32_ucode_sw_fini,
	.hw_init = navi32_ucode_hw_init,
	.hw_fini = navi32_ucode_hw_fini,
};

