/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_vbios.h>
#include <amdgv_pci_def.h>
#include <amdgv_psp.h>
#include <amdgv_gpumon.h>
#include <amdgv_sched.h>
#include <amdgv_nps.h>

#include "psp_v15_0_8.h"
#include "psp_v15_0_8_ras_fw.h"
#include "asic_reg/MP/mp_15_0_8_offset.h"
#include "asic_reg/MP/mp_15_0_8_sh_mask.h"

#include "ucode/mes/12_1_0/rs64_mes_p0_ucode_signed.h"
#include "ucode/mes/12_1_0/rs64_mes_p0_data_signed.h"
#include "ucode/cp/12_1_0/rs64_mec_ucode_signed.h"
#include "ucode/cp/12_1_0/rs64_mec_data_signed.h"
#include "ucode/sdma/7_1_0/sdma_ucode_signed.h"
#include "ucode/rlc/12_1_0/f32_gpm_ucode_signed.h"
#include "ucode/rlc/12_1_0/rlc_toc_data_signed.h"
#include "ucode/rlc/12_1_0/rlc_restore_list_gpm_mem_signed.h"
#include "ucode/rlc/12_1_0/rlc_restore_list_srm_mem_signed.h"
#include "ucode/rlc/12_1_0/rlc_lx6_1_dram_ucode_signed.h"
#include "ucode/rlc/12_1_0/rlc_lx6_1_iram_ucode_signed.h"
#include "ucode/rlc/12_1_0/rlc_lx6_dram_ucode_signed.h"
#include "ucode/rlc/12_1_0/rlc_lx6_iram_ucode_signed.h"
#include "ucode/dfc/15_0_8/dfc_fw_signed.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

int psp_v15_0_8_ring_destroy(struct amdgv_adapter *adapt)
{
	uint32_t flag = 0x80000000;
	uint32_t mask = 0x8000FFFF;
	struct psp_context *psp = &adapt->psp;
	int wait_ret;

	/* Write the ring destroy command to C2PMSG_64 */
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMPASP_SMN_C2PMSG_64),
	       GFX_CTRL_CMD_ID_DESTROY_RINGS);

	/* Wait for response flag (bit 31) in C2PMSG_64 */
	wait_ret = amdgv_wait_for_register(
		adapt, SOC15_REG_OFFSET_NAME(MP0, psp->idx, regMPASP_SMN_C2PMSG_64), mask, flag,
		AMDGV_TIMEOUT(TIMEOUT_PSP_REG), AMDGV_WAIT_CHECK_EQ, 0);

	if (wait_ret)
		return AMDGV_FAILURE;
	else
		return 0;
}

enum psp_status psp_v15_0_8_ring_start(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_ring *ring;
	struct psp_local_memory *local_mem = NULL;
	uint32_t psp_ring_reg = 0; // MPASP_SMN_C2PMSG_64 - 71
	struct psp_context *psp = &adapt->psp;
	int i;

	ring = &psp->km_ring[psp->idx];

	local_mem = &ring->ring_mem;

	/* Wait for TOS ready for ring creation */
	for (i = 0; i < adapt->mcp.num_aid; i++) {
		ret = amdgv_psp_wait_for_register(
			adapt, SOC15_REG_OFFSET_NAME(MP0, i, regMPASP_SMN_C2PMSG_64), 0x80000000,
			0x80000000, false, AMDGV_WAIT_FLAG_FORCE_YIELD);

		if (ret != PSP_STATUS__SUCCESS) {
			AMDGV_ERROR(
				"PSP-%d: Waiting for TOS ready for ring creation failed.\n",
				i);
			return ret;
		}
	}

	/* Write low address of the ring to C2PMSG_69 */
	psp_ring_reg = lower_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMPASP_SMN_C2PMSG_69), psp_ring_reg);

	/* Write high address of the ring to C2PMSG_70 */
	psp_ring_reg = upper_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMPASP_SMN_C2PMSG_70), psp_ring_reg);

	/* Write size of ring to C2PMSG_71 */
	psp_ring_reg = local_mem->size;
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMPASP_SMN_C2PMSG_71), psp_ring_reg);

	/* Write the ring initialization command to C2PMSG_64 */
	psp_ring_reg = ring->ring_type;
	psp_ring_reg = psp_ring_reg << 16;
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMPASP_SMN_C2PMSG_64), psp_ring_reg);

	/* Wait for response flag (bit 31) in C2PMSG_64 */
	ret = amdgv_psp_wait_for_register(
		adapt, SOC15_REG_OFFSET_NAME(MP0, psp->idx, regMPASP_SMN_C2PMSG_64), 0x80000000,
		0x8000FFFF, false, AMDGV_WAIT_FLAG_FORCE_YIELD);

	if (ret != PSP_STATUS__SUCCESS)
		AMDGV_ERROR("PSP: Failed to start ring.\n");

	return ret;
}

static enum psp_status psp_v15_0_8_program_register(struct amdgv_adapter *adapt, uint32_t idx_vf,
						    uint32_t reg_value, uint32_t reg_value_hi,
						    enum psp_ih_reg reg_id)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *program_reg_cmd = adapt->psp.psp_cmd_km_mem;

	if (!program_reg_cmd) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	AMDGV_DEBUG("Program Register 0x%x value 0x%x value_hi 0x%x\n", reg_id, reg_value,
		    reg_value_hi);

	program_reg_cmd->cmd_id = PSP_CMD_KM_TYPE__GBR_IH_REG;
	program_reg_cmd->cmd.program_reg.reg_value = reg_value;
	program_reg_cmd->cmd.program_reg.reg_value_hi = reg_value_hi;
	program_reg_cmd->cmd.program_reg.reg_id = reg_id;
	program_reg_cmd->cmd.program_reg.target_vfid = idx_vf;

	ret = amdgv_psp_cmd_km_submit(adapt, program_reg_cmd, NULL);

	/* Clear system memory used for Program Reg CMD */
	oss_memset(program_reg_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

enum psp_status psp_v15_0_8_program_guest_mc_settings(struct amdgv_adapter *adapt,
						      uint32_t idx_vf)
{
	struct amdgv_vf_device *vf;
	uint64_t fb_base, fb_top;
	uint64_t fb_location_base, fb_location_top;
	uint32_t sys_aper_lo, sys_aper_hi;

	if (adapt->xgmi.connected_to_cpu) {
		fb_location_base = 0x1FFFFFFFF;
		fb_location_top = 0x0;
		sys_aper_lo = 0xFFFFFFFF;
		sys_aper_hi = 0x7F;
	} else {
		vf = &adapt->array_vf[idx_vf];

		/* copy pf fb base to vf*/
		fb_base = adapt->mc_fb_loc_addr;

		if (adapt->xgmi.phy_nodes_num > 1) {
			fb_top = fb_base +
				MBYTES_TO_BYTES(vf->real_fb_size * adapt->xgmi.phy_nodes_num) - 1;
		} else
			fb_top = fb_base + MBYTES_TO_BYTES(vf->real_fb_size) - 1;

		fb_location_base = MB_TO_16MB(TO_MBYTES(fb_base));
		fb_location_top = MB_TO_16MB(TO_MBYTES(fb_top));

		sys_aper_lo = TO_256KBYTES(fb_base);
		sys_aper_hi = TO_256KBYTES(fb_top);
	}

	AMDGV_INFO("[VF%d] fb_location_base = 0x%08x\n", idx_vf, fb_location_base);
	AMDGV_INFO("[VF%d] fb_location_top  = 0x%08x\n", idx_vf, fb_location_top);
	AMDGV_INFO("[VF%d] sys_aper_lo      = 0x%08x\n", idx_vf, sys_aper_lo);
	AMDGV_INFO("[VF%d] sys_aper_hi      = 0x%08x\n", idx_vf, sys_aper_hi);

	// GCHUB
	if (psp_v15_0_8_program_register(adapt, idx_vf, lower_32_bits(fb_location_base), upper_32_bits(fb_location_base),
					   GC_MC_VM_FB_LOCATION_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v15_0_8_program_register(adapt, idx_vf, lower_32_bits(fb_location_top), upper_32_bits(fb_location_top),
					   GC_MC_VM_FB_LOCATION_TOP))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v15_0_8_program_register(adapt, idx_vf, lower_32_bits(sys_aper_lo), upper_32_bits(sys_aper_lo),
					   GC_MC_SYSTEM_APERTURE_LO))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v15_0_8_program_register(adapt, idx_vf, lower_32_bits(sys_aper_hi), upper_32_bits(sys_aper_hi),
					   GC_MC_SYSTEM_APERTURE_HI))
		return PSP_STATUS__ERROR_GENERIC;

	// MMHUB
	if (psp_v15_0_8_program_register(adapt, idx_vf, lower_32_bits(fb_location_base), upper_32_bits(fb_location_base),
					   MM_MC_VM_FB_LOCATION_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v15_0_8_program_register(adapt, idx_vf, lower_32_bits(fb_location_top), upper_32_bits(fb_location_top),
					   MM_MC_VM_FB_LOCATION_TOP))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v15_0_8_program_register(adapt, idx_vf, lower_32_bits(sys_aper_lo), upper_32_bits(sys_aper_lo),
					   MM_MC_SYSTEM_APERTURE_LO))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v15_0_8_program_register(adapt, idx_vf, lower_32_bits(sys_aper_hi), upper_32_bits(sys_aper_hi),
					   MM_MC_SYSTEM_APERTURE_HI))
		return PSP_STATUS__ERROR_GENERIC;

	// MEMSIZE
	if (psp_v15_0_8_program_register(adapt, idx_vf, vf->real_fb_size, 0, RCC_CONFIG_MEMSIZE))
		return PSP_STATUS__ERROR_GENERIC;

	return PSP_STATUS__SUCCESS;
}

static enum psp_status psp_v15_0_8_vfgate_support(struct amdgv_adapter *adapt)
{
	return PSP_STATUS__SUCCESS;
}

static enum psp_status psp_v15_0_8_set_mb_int(struct amdgv_adapter *adapt, uint32_t idx_vf,
					      bool enable)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *vfgate_cmd = adapt->psp.psp_cmd_km_mem;
	bool resp_enable = false;
	struct psp_gfx_resp psp_resp = { 0 };

	ret = psp_v15_0_8_vfgate_support(adapt);
	if (ret == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE)
		return ret;

	if (!vfgate_cmd) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	vfgate_cmd->cmd_id = PSP_CMD_KM_TYPE__VFGATE;
	vfgate_cmd->cmd.vfgate.target_vfid = idx_vf;

	if (enable)
		vfgate_cmd->cmd.vfgate.action = GFX_SCMD_VFGATE_ENABLE;
	else
		vfgate_cmd->cmd.vfgate.action = GFX_SCMD_VFGATE_DISABLE;

	ret = amdgv_psp_cmd_km_submit(adapt, vfgate_cmd, &psp_resp);

	if (ret == PSP_STATUS__SUCCESS) {
		resp_enable = psp_resp.sriov_mbstatus & SRIOV_MBSTATUS_ISENABLED_MASK;

		if (resp_enable == enable)
			AMDGV_DEBUG("psp mailbox %s for VF%d\n",
				   enable ? "enabled" : "disabled", idx_vf);
		else {
			AMDGV_ERROR("psp mailbox failed to %s for VF%d\n",
				   enable ? "enable" : "disable", idx_vf);
			ret = PSP_STATUS__ERROR_GENERIC;
		}
	}

	/* Clear system memory used for VFGATE CMD */
	oss_memset(vfgate_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

static enum psp_status psp_v15_0_8_get_mb_int_status(struct amdgv_adapter *adapt,
						     uint32_t idx_vf,
						     struct psp_mb_status *mb_status)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *vfgate_cmd = adapt->psp.psp_cmd_km_mem;
	struct psp_gfx_resp psp_resp = { 0 };

	ret = psp_v15_0_8_vfgate_support(adapt);

	if (ret == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE)
		return ret;

	if (!vfgate_cmd) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	vfgate_cmd->cmd_id = PSP_CMD_KM_TYPE__VFGATE;
	vfgate_cmd->cmd.vfgate.target_vfid = idx_vf;

	vfgate_cmd->cmd.vfgate.action = GFX_SCMD_VFGATE_STATUS;

	ret = amdgv_psp_cmd_km_submit(adapt, vfgate_cmd, &psp_resp);

	if (psp_resp.sriov_mbstatus & SRIOV_MBSTATUS_ISENABLED_MASK)
		mb_status->vf_gate_enabled = true;
	else
		mb_status->vf_gate_enabled = false;

	/* Clear system memory used for VFGATE CMD */
	oss_memset(vfgate_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

static enum psp_status psp_v15_0_8_clear_vf_fw(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__CLEAR_VF_FW;
	psp_cmd.cmd.clear_vf_fw.target_vf = idx_vf;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);

	return ret;
}

static uint32_t psp_v15_0_8_ring_get_wptr(struct amdgv_adapter *adapt)
{
	return RREG32(SOC15_REG_OFFSET(MP0, adapt->psp.idx, regMPASP_SMN_C2PMSG_67));
}

static void psp_v15_0_8_ring_set_wptr(struct amdgv_adapter *adapt, uint32_t value)
{
	WREG32(SOC15_REG_OFFSET(MP0, adapt->psp.idx, regMPASP_SMN_C2PMSG_67), value);
}

static enum psp_status psp_v15_0_8_set_num_vfs(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__NUM_ENABLED_VFS;
	psp_cmd.cmd.num_vfs.number_of_vfs = num_vf;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);

	return ret;
}

enum psp_status psp_v15_0_8_fw_attestation_support(struct amdgv_adapter *adapt)
{
	// NOT SUPPORTED NOW
	return PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
}

static enum psp_status
psp_v15_0_8_copy_vf_chiplet_regs(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	return ret;
}

uint32_t psp_v15_0_8_get_sos_loaded_status(struct amdgv_adapter *adapt)
{
	return RREG32(SOC15_REG_OFFSET(MP0, adapt->psp.idx, regMPASP_SMN_C2PMSG_81));
}

static int psp_v15_0_8_wait_sos_loaded_status_cb(void *context)
{
	void **context_array = (void **)context;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context_array[0];
	uint32_t *value = (uint32_t *)context_array[1];
	uint32_t reg_val = psp_v15_0_8_get_sos_loaded_status(adapt);

	return !((reg_val != 0 && reg_val != PSP_REGISTER_VALUE_INVALID && reg_val != *value));
}

bool psp_v15_0_8_wait_sos_loaded_status(struct amdgv_adapter *adapt)
{
	uint32_t value;
	void *context_array[2] = { (void *)adapt, 0 }; /* param for amdgv_wait_for */
	int wait_ret;
	struct amdgv_wait_for_cb_context cb_context = { 0 };

	/* If PSP TOS is loaded and alive, C2P 81 will be incrementing
	 *  (PSP Sign of Life / increments once per 100ms)
	 */
	value = psp_v15_0_8_get_sos_loaded_status(adapt);
	context_array[1] = (void *)&value;

	oss_msleep(100); /* this sleep is REQUIRED since TOS may just starting */

	cb_context.ctx = (void *)context_array;
	cb_context.type = AMDGV_WAIT_FOR_PSP_TOS_LOADED_STATUS;
	wait_ret = amdgv_wait_for(adapt, psp_v15_0_8_wait_sos_loaded_status_cb,
				  &cb_context, AMDGV_TIMEOUT(TIMEOUT_PSP_REG), 0);
	if (!wait_ret)
		return true;
	else
		return false;
}

static bool psp_v15_0_8_need_switch_to_pf(struct amdgv_adapter *adapt)
{
	return false;
}

#define PSP_WAIT_BOOTLOADER_RETRY	30
static enum psp_status psp_v15_0_8_wait_for_bootloader(struct amdgv_adapter *adapt)
{
	int retry_loop, ret = 0;

	/* wait for PSP to indicate BL completion */
	for (retry_loop = 0; retry_loop < PSP_WAIT_BOOTLOADER_RETRY; retry_loop++) {
		ret = amdgv_psp_wait_for_register(
			adapt, regMPASP_SMN_C2PMSG_33, "regMPASP_SMN_C2PMSG_33",
			0x80000000, 0xFFFFFFFF, false, AMDGV_WAIT_FLAG_FORCE_YIELD);
		if (ret == PSP_STATUS__SUCCESS)
			break;
	}

	return ret;
}

enum psp_status psp_v15_0_8_wait_for_bootloader_steady(struct amdgv_adapter *adapt)
{
	int retry_loop, ret = 0;

	ret = psp_v15_0_8_wait_for_bootloader(adapt);
	if (ret) {
		AMDGV_ERROR("PSP BL not ready\n");
		return ret;
	}

	/* wait for BL steady state to accept cmd from driver */
	for (retry_loop = 0; retry_loop < PSP_WAIT_BOOTLOADER_RETRY; retry_loop++) {
		ret = amdgv_psp_wait_for_register(
			adapt, regMPASP_SMN_C2PMSG_35, "regMPASP_SMN_C2PMSG_35",
			0x80000000, 0x80000000, false, AMDGV_WAIT_FLAG_FORCE_YIELD);
		if (ret == PSP_STATUS__SUCCESS)
			break;
	}

	return ret;
}

uint32_t psp_v15_0_8_get_bootloader_version(struct amdgv_adapter *adapt)
{
	uint32_t fw_ver = 0;
	struct psp_context *psp = &adapt->psp;

	fw_ver = RREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMPASP_SMN_C2PMSG_59));

	// adapt->psp.fw_info is not allocated for AMDGV_FW_LOAD_DIRECT
	if (adapt->fw_load_type != AMDGV_FW_LOAD_DIRECT)
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_BL] = fw_ver;

	AMDGV_INFO("PSP BL version (psp_C2PMSG_59): %X.%X.%X.%X\n",
		   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
		   (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);

	return fw_ver;
}

static int psp_v15_0_8_load_toc(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum amdgv_firmware_id ucode_id;

	ucode_id = AMDGV_FIRMWARE_ID__PSP_TOC;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret)
		return AMDGV_FAILURE;

	AMDGV_INFO("PSP: RLC TOC is loaded.\n");

	return 0;
}


static int psp_v15_0_8_ucode_load(struct amdgv_adapter *adapt, uint32_t *ucode_id_list,
				  uint32_t ucode_id_count)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	uint32_t i;

	for (i = 0; i < ucode_id_count; i++) {
		switch (ucode_id_list[i]) {
		case AMDGV_FIRMWARE_ID__PSP_TOC:
			ret = amdgv_psp_load_fw(adapt, (unsigned char *)RLC_TOC_DATA,
							sizeof(RLC_TOC_DATA), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aRLC_Ucode,
							sizeof(aRLC_Ucode), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
			ret = amdgv_psp_load_np_fw(adapt,
							(unsigned char *)aRLC_RESTORE_LIST_GPM_MEM,
							sizeof(aRLC_RESTORE_LIST_GPM_MEM), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
			ret = amdgv_psp_load_np_fw(adapt,
							(unsigned char *)aRLC_RESTORE_LIST_SRM_MEM,
							sizeof(aRLC_RESTORE_LIST_SRM_MEM), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLX6:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aLX6_IRAM_UCODE,
							sizeof(aLX6_IRAM_UCODE), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aLX6_DRAM_UCODE,
							sizeof(aLX6_DRAM_UCODE), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aLX6_CORE1_IRAM_UCODE,
							sizeof(aLX6_CORE1_IRAM_UCODE), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aLX6_CORE1_DRAM_UCODE,
							sizeof(aLX6_CORE1_DRAM_UCODE), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__SDMA0:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aSDMA_Ucode,
							sizeof(aSDMA_Ucode), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aRS64_MEC_PRODUCTION_UCODE,
							sizeof(aRS64_MEC_PRODUCTION_UCODE), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA:
		case AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA:
		case AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA:
		case AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA:
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aRS64_MEC_PRODUCTION_DATA,
							sizeof(aRS64_MEC_PRODUCTION_DATA), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RS64_MES:
		case AMDGV_FIRMWARE_ID__RS64_KIQ:
			// Unified MES
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aRS64_MES_P0_UCODE,
							sizeof(aRS64_MES_P0_UCODE), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__RS64_MES_STACK:
		case AMDGV_FIRMWARE_ID__RS64_KIQ_STACK:
			// Unified MES
			ret = amdgv_psp_load_np_fw(adapt, (unsigned char *)aRS64_MES_P0_DATA,
							sizeof(aRS64_MES_P0_DATA), ucode_id_list[i]);
			break;
		case AMDGV_FIRMWARE_ID__DFC_FW:
			ret = amdgv_psp_load_fw(adapt,
				(unsigned char *)aDFC_FW,
				sizeof(aDFC_FW),
				ucode_id_list[i]);
			break;
		default:
			AMDGV_INFO("Unsupported FW id 0x%x. Skipping ..\n", ucode_id_list[i]);
			break;
		}
		if (ret != PSP_STATUS__SUCCESS)
			return AMDGV_FAILURE;
	}
	return 0;
}

static int psp_v15_0_8_ucode_get_start_addr(struct amdgv_adapter *adapt, uint32_t ucode_id,
					    uint64_t *uc_start_addr)
{
	switch (ucode_id) {
	case AMDGV_FIRMWARE_ID__CP_MES:
	case AMDGV_FIRMWARE_ID__MES_THREAD1:
		*uc_start_addr = RS64_MES_P0_UC_START_ADDR_LO >> 2 |
			((uint64_t)(RS64_MES_P0_UC_START_ADDR_HI) << 30);
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
		*uc_start_addr = RS64_MEC_PRODUCTION_UC_START_ADDR_LO >> 2 |
			((uint64_t)(RS64_MEC_PRODUCTION_UC_START_ADDR_HI) << 30);
		break;
	default:
		AMDGV_ERROR("No ucode start address needed for firmware ID %d\n", ucode_id);
		return AMDGV_FAILURE;
	}
	return 0;
}

static int save_accelerator_partition_mode(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;

	oss_save_accelerator_partition_mode(adapt->dev,
					    adapt->mcp.accelerator_partition_mode);

	return 0;
}

static const char *psp_v15_0_8_accel_mode_desc(
	struct amdgv_adapter *adapt, enum amdgv_accelerator_partition_mode accelerator_partition_mode)
{
	switch (accelerator_partition_mode) {
	case 1:
		return "SPX";
	case 2:
		return "DPX";
	case 4:
		return (adapt->max_num_vf == 4) ? "CPX-4" : "QPX";
	case 8:
		return "CPX";
	default:
		return "UNKNOWN";
	}
}

static uint32_t psp_v15_0_8_accelerator_partition_mode_to_psp_cmd(
	enum amdgv_accelerator_partition_mode accelerator_partition_mode)
{
	switch (accelerator_partition_mode) {
	case AMDGV_ACCELERATOR_PARTITION_MODE_SPX:
		return SP_MODE_SPX;
	case AMDGV_ACCELERATOR_PARTITION_MODE_DPX:
		return SP_MODE_DPX;
	case AMDGV_ACCELERATOR_PARTITION_MODE_QPX:
		return SP_MODE_QPX;
	case AMDGV_ACCELERATOR_PARTITION_MODE_CPX:
		return SP_MODE_CPX;
	default:
		return 0;
	}
}

enum psp_status psp_v15_0_8_set_accelerator_partition_mode(struct amdgv_adapter *adapt,
						       enum amdgv_accelerator_partition_mode
						       accelerator_partition_mode)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__SRIOV_SPATIAL_PART;

	/* Interpreted as number of partitions desired */
	psp_cmd.cmd.sriov_spatial_part.mode = psp_v15_0_8_accelerator_partition_mode_to_psp_cmd(accelerator_partition_mode);
	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	/* save accelerator partition mode setting */
	adapt->mcp.accelerator_partition_mode = accelerator_partition_mode;
	oss_schedule_work(adapt->dev, save_accelerator_partition_mode,
			  (void *)adapt);

	AMDGV_INFO("accelerator_partition_mode=%d, num_vf=%u\n",
		accelerator_partition_mode, adapt->num_vf);

	return ret;
}

enum psp_status psp_v15_0_8_get_cc_mode(struct amdgv_adapter *adapt,
		enum amdgv_cc_mode *cc_mode)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__GET_CC_MODE;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	*cc_mode = psp_resp.uresp.uresp_cc_mode.cc_mode;

	AMDGV_DEBUG("PSP: PSP_CMD_KM_TYPE__GET_CC_MODE returned %u\n", *cc_mode);

	return ret;
}

enum psp_status psp_v15_0_8_set_cc_mode(struct amdgv_adapter *adapt,
		enum amdgv_cc_mode cc_mode)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__SET_CC_MODE;
	psp_cmd.cmd.set_cc_mode.cc_mode = cc_mode;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	AMDGV_INFO("PSP: PSP_CMD_KM_TYPE__SET_CC_MODE CC mode %u\n", cc_mode);

	return ret;
}

static enum psp_status psp_v15_0_8_check_cc_mode(struct amdgv_adapter *adapt)
{
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;
	enum amdgv_cc_mode curr_cc_mode;

	/* Get the boot CC mode from PSP */
	psp_ret = psp_v15_0_8_get_cc_mode(adapt, &curr_cc_mode);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("failed to get current CC mode\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* If there is no saved configuration, default to the current boot CC mode */
	if (adapt->mcp.cc_mode == AMDGV_CC_MODE_MAX) {
		adapt->mcp.cc_mode = curr_cc_mode;
		AMDGV_INFO("No saved CC mode config, defaulting to boot mode: %u\n", curr_cc_mode);
	}

	AMDGV_INFO("Boot CC mode=%u, requested CC mode=%u\n",
		curr_cc_mode, adapt->mcp.cc_mode);

	return PSP_STATUS__SUCCESS;
}

static struct amdgv_memmgr *psp_v15_0_8_get_memmgr_for_umf(struct amdgv_adapter *adapt,
                                    uint64_t umf_start,
                                    uint64_t umf_size,
                                    uint64_t *umf_offset)
{
	struct amdgv_memmgr *memmgr = NULL;
	uint64_t config_memsize;

	config_memsize = MBYTES_TO_BYTES(amdgv_nbio_get_memsize(adapt));

	if (amdgv_memmgr_addr_in_range(adapt, &adapt->memmgr_pf, umf_start)) {
		memmgr = &adapt->memmgr_pf;
		*umf_offset = umf_start;
	} else if (amdgv_memmgr_addr_in_range(adapt, &adapt->memmgr_gpu, umf_start)) {
		memmgr = &adapt->memmgr_gpu;
		*umf_offset = config_memsize - umf_start - umf_size;
	}

	if (!memmgr)
		return NULL;

	return memmgr;
}

static enum psp_status psp_v15_0_8_reserve_umf_region(struct amdgv_adapter *adapt,
                                    uint64_t umf_start,
                                    uint64_t umf_size)
{
	struct amdgv_memmgr *memmgr;
	struct amdgv_memmgr_mem *mem;
	uint64_t umf_offset;

	memmgr = psp_v15_0_8_get_memmgr_for_umf(adapt, umf_start, umf_size, &umf_offset);
	if (!memmgr) {
		AMDGV_ERROR("UMF addr 0x%llx not in any memmgr range\n", umf_start);
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Reserve UMF at the exact offset */
	mem = amdgv_memmgr_alloc_align_at(memmgr, umf_offset, umf_size, MEM_PSP_TMR);
	if (!mem) {
		AMDGV_ERROR("Failed to reserve UMF at offset 0x%llx\n", umf_offset);
		return PSP_STATUS__ERROR_GENERIC;
	}

	AMDGV_INFO("UMF reserved in %s at offset 0x%llx, size 0x%llx\n",
				(memmgr == &adapt->memmgr_gpu) ? "memmgr_gpu" : "memmgr_pf",
				umf_offset, umf_size);

	return PSP_STATUS__SUCCESS;
}

enum psp_status psp_v15_0_8_ual_get_interface_version(struct amdgv_adapter *adapt, uint32_t *version)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	if (version == NULL) {
		AMDGV_ERROR("PSP: version pointer is NULL\n");
		return PSP_STATUS__ERROR_INVALID_PARAMS;
	}

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__UAL_GET_INTERFACE_VER;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	*version = psp_resp.uresp.get_intf_ver_ual.intf_ver;

	AMDGV_INFO("PSP: PSP_CMD_KM_TYPE__UAL_GET_INTERFACE_VER returned version 0x%08x\n", *version);

	return ret;
}

enum psp_status psp_v15_0_8_ual_get_config(struct amdgv_adapter *adapt,
	uint64_t data_addr, uint32_t size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__UAL_GET_CONFIG;
	psp_cmd.cmd.get_config_ual.ual_cfg_addr_hi = upper_32_bits(data_addr);
	psp_cmd.cmd.get_config_ual.ual_cfg_addr_lo = lower_32_bits(data_addr);
	psp_cmd.cmd.get_config_ual.ual_cfg_size = size;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	return ret;
}

enum psp_status psp_v15_0_8_ual_set_ppod_config(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_set_ppod_config_req_ual_v1 *config)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	if (config == NULL) {
		AMDGV_ERROR("PSP: config pointer is NULL\n");
		return PSP_STATUS__ERROR_INVALID_PARAMS;
	}

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__UAL_SET_PPOD_CONFIG;
	psp_cmd.cmd.set_ppod_config_ual.accelerator_id = config->accelerator_id;
	oss_memcpy(psp_cmd.cmd.set_ppod_config_ual.ppod_id, config->ppod_id, sizeof(config->ppod_id));
	psp_cmd.cmd.set_ppod_config_ual.ppod_size = config->ppod_size;
	psp_cmd.cmd.set_ppod_config_ual.bandwidth = config->bandwidth;
	psp_cmd.cmd.set_ppod_config_ual.latency = config->latency;
	oss_memcpy(psp_cmd.cmd.set_ppod_config_ual.local_accelerators,
		config->local_accelerators,
		sizeof(psp_cmd.cmd.set_ppod_config_ual.local_accelerators));

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	AMDGV_DEBUG("PSP: PSP_CMD_KM_TYPE__UAL_SET_PPOD_CONFIG successful - "
			"accelerator_id=%u ppod_id=%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x ppod_size=%u bandwidth=%u latency=%u\n",
			config->accelerator_id,
			config->ppod_id[0], config->ppod_id[1], config->ppod_id[2], config->ppod_id[3],
			config->ppod_id[4], config->ppod_id[5], config->ppod_id[6], config->ppod_id[7],
			config->ppod_id[8], config->ppod_id[9], config->ppod_id[10], config->ppod_id[11],
			config->ppod_id[12], config->ppod_id[13], config->ppod_id[14], config->ppod_id[15],
			config->ppod_size,
			config->bandwidth,
			config->latency);

	return ret;
}

enum psp_status psp_v15_0_8_ual_set_vpod_config(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_set_vpod_config_req_ual_v1 *config)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	if (config == NULL) {
		AMDGV_ERROR("PSP: config pointer is NULL\n");
		return PSP_STATUS__ERROR_INVALID_PARAMS;
	}

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__UAL_SET_VPOD_CONFIG;
	psp_cmd.cmd.set_vpod_config_ual.vpod_id = config->vpod_id;
	psp_cmd.cmd.set_vpod_config_ual.vpod_size = config->vpod_size;
	oss_memcpy(psp_cmd.cmd.set_vpod_config_ual.vpod_active_accelerators,
		config->vpod_active_accelerators,
		sizeof(psp_cmd.cmd.set_vpod_config_ual.vpod_active_accelerators));
	psp_cmd.cmd.set_vpod_config_ual.addr_mode = config->addr_mode;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	AMDGV_DEBUG("PSP: PSP_CMD_KM_TYPE__UAL_SET_VPOD_CONFIG successful - "
			"vpod_id=%u vpod_size=%u\n",
			config->vpod_id, config->vpod_size);

	return ret;
}

enum psp_status psp_v15_0_8_ual_set_station_config(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_set_station_config_req_ual_v1 *config)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	if (config == NULL) {
		AMDGV_ERROR("PSP: config pointer is NULL\n");
		return PSP_STATUS__ERROR_INVALID_PARAMS;
	}

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__UAL_SET_STATION_CONFIG;
	psp_cmd.cmd.set_station_config_ual.num_stations = config->num_stations;
	psp_cmd.cmd.set_station_config_ual.station_flag = config->station_flag;
	oss_memcpy(psp_cmd.cmd.set_station_config_ual.lane_en_bitmap,
		config->lane_en_bitmap,
		sizeof(psp_cmd.cmd.set_station_config_ual.lane_en_bitmap));

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	AMDGV_DEBUG("PSP: PSP_CMD_KM_TYPE__UAL_SET_STATION_CONFIG successful - "
			"num_stations=%u station_flag=0x%02x\n",
			config->num_stations, config->station_flag);

	return ret;
}

enum psp_status psp_v15_0_8_ual_send_completion(struct amdgv_adapter *adapt,
		uint32_t cmd_id, uint32_t status)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__UAL_SEND_COMPLETION;
	psp_cmd.cmd.send_completion_ual.cmd_id = cmd_id;
	psp_cmd.cmd.send_completion_ual.status = status;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	AMDGV_DEBUG("PSP: PSP_CMD_KM_TYPE__UAL_SEND_COMPLETION successful - "
			"cmd_id=0x%08x status=0x%08x\n", cmd_id, status);

	return ret;
}

/*
 * psp_v15_0_8_enable_interrupt - Enable/disable MPASP interrupts
 * @adapt: GPU adapter structure
 * @enable: true to enable, false to disable
 *
 * Controls MPASP interrupt enablement. MPASP writes Client_ID and
 * Source_ID into IH_COOKIE and sends interrupt through MPASP_IH_SW_INT.
 */
static int psp_v15_0_8_enable_interrupt(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t val;

	if (enable) {
		AMDGV_INFO("MPASP: Enabling software interrupt\n");

		/* Configure the interrupt ID */
		val = RREG32(SOC15_REG_OFFSET(MP0, 0, regMPASP_SMN_IH_SW_INT));
		val = REG_SET_FIELD(val, MPASP_SMN_IH_SW_INT, ID, IH_IV_SRCID_MP0_IH_SW_INT);
		val = REG_SET_FIELD(val, MPASP_SMN_IH_SW_INT, VALID, 0); /* Clear valid bit */
		WREG32(SOC15_REG_OFFSET(MP0, 0, regMPASP_SMN_IH_SW_INT), val);

		/* Enable interrupt (INT_MASK = 0 means enabled) */
		val = RREG32(SOC15_REG_OFFSET(MP0, 0, regMPASP_SMN_IH_SW_INT_CTRL));
		val = REG_SET_FIELD(val, MPASP_SMN_IH_SW_INT_CTRL, INT_MASK, 0);
		WREG32(SOC15_REG_OFFSET(MP0, 0, regMPASP_SMN_IH_SW_INT_CTRL), val);
	} else {
	AMDGV_INFO("MPASP: Disabling software interrupt\n");

		/* Disable interrupt (INT_MASK = 1 means disabled) */
		val = RREG32(SOC15_REG_OFFSET(MP0, 0, regMPASP_SMN_IH_SW_INT_CTRL));
		val = REG_SET_FIELD(val, MPASP_SMN_IH_SW_INT_CTRL, INT_MASK, 1);
		WREG32(SOC15_REG_OFFSET(MP0, 0, regMPASP_SMN_IH_SW_INT_CTRL), val);
	}

	return 0;
}

static int psp_v15_0_8_handle_irq(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	int ret = 0;
	uint32_t ctx_id = 0;
	uint32_t val = 0;

	if (entry->client_id != IH_IV_CLIENTID_MP0 ||
	    entry->src_id != IH_IV_SRCID_MP0_IH_SW_INT)
		return 0;

	AMDGV_INFO("PSP: Handling ASP interrupt\n");

	/* ack irq first */
	val = RREG32(SOC15_REG_OFFSET(MP0, 0, regMPASP_SMN_IH_SW_INT_CTRL));
	val = REG_SET_FIELD(val, MPASP_SMN_IH_SW_INT_CTRL, INT_ACK, 1);
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMPASP_SMN_IH_SW_INT_CTRL), val);

	ctx_id = entry->src_data[0];
	switch (ctx_id) {
	case PSP_GFX_INT_CTXT_UAL_CMD_PAUSE:
		AMDGV_INFO("ASP: Handling UAL pause req interrupt\n");
		ret = amdgv_sched_queue_event(adapt, AMDGV_PF_IDX,
					      AMDGV_EVENT_SCHED_UAL_PAUSE_REQ,
					      AMDGV_SCHED_BLOCK_ALL);
		break;
	case PSP_GFX_INT_CTXT_UAL_CMD_RESUME:
		AMDGV_INFO("ASP: Handling UAL resume req interrupt\n");
		ret = amdgv_sched_queue_event(adapt, AMDGV_PF_IDX,
					      AMDGV_EVENT_SCHED_UAL_RESUME_REQ,
					      AMDGV_SCHED_BLOCK_ALL);
		break;
	default:
		AMDGV_ERROR("Unsupported ASP interrupt context id %d\n", ctx_id);
		break;
	}

	return ret;
}

static int save_memory_partition_mode(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;

	oss_save_memory_partition_mode(adapt->dev,
				       adapt->mcp.memory_partition_mode);

	return 0;
}

enum psp_status psp_v15_0_8_set_memory_partition_mode(struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__NPS_MODE;
	psp_cmd.cmd.sriov_memory_part.num_parts = memory_partition_mode;
	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);
	if (ret)
		return ret;

	adapt->mcp.memory_partition_mode = memory_partition_mode;
	adapt->mcp.mem_mode_switch_requested = true;
	oss_schedule_work(adapt->dev, save_memory_partition_mode, (void *)adapt);

	AMDGV_DEBUG("PSP: GFX_CMD_ID_NPS_MODE NPS%u mode\n", memory_partition_mode);

	return ret;
}

static enum psp_status psp_v15_0_8_check_memory_partition_mode(struct amdgv_adapter *adapt)
{
	int ret, i;
	enum amdgv_memory_partition_mode curr_memory_partition_mode;
	const struct amdgv_nps_compute_combination *supported_combinations_for_num_vf;
	enum amdgv_accelerator_partition_mode default_compute_mode;

	ret = amdgv_nbio_get_nps_mode(adapt, &curr_memory_partition_mode);
	if (ret) {
		AMDGV_ERROR("failed to get current NPS mode\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* if there is no saved configuration, default to the current memory partition mode */
	if (adapt->mcp.memory_partition_mode == AMDGV_MEMORY_PARTITION_MODE_UNKNOWN ||
		adapt->mcp.memory_partition_mode == AMDGV_MEMORY_PARTITION_MODE_MAX) {
		adapt->mcp.memory_partition_mode = curr_memory_partition_mode;
		/* if there is no config file, first try to keep current memory partition mode.
		 * set to the default compute mode for the vf_num and current memory mode.
		 */
		if (adapt->mcp.accelerator_partition_mode == AMDGV_ACCELERATOR_PARTITION_MODE_MAX) {
			adapt->mcp.accelerator_partition_mode =
				amdgv_nbio_get_default_accel_partition_mode(adapt,
					curr_memory_partition_mode);
			if (adapt->mcp.accelerator_partition_mode ==
				AMDGV_ACCELERATOR_PARTITION_MODE_UNKNOWN) {
				return PSP_STATUS__ERROR_GENERIC;
			}
		}
	}

	if (in_whole_gpu_reset()) {
		if (adapt->mcp.memory_partition_mode != curr_memory_partition_mode) {
			AMDGV_ERROR("memory partition mode change to NPS%u failed. Reverted to NPS%u\n",
				adapt->mcp.memory_partition_mode, curr_memory_partition_mode);
			/* reverse the saved memory partition mode configuration */
			adapt->mcp.memory_partition_mode = curr_memory_partition_mode;
		} else {
			/* re-discover ip for updating NUMA range */
			amdgv_discover_ip(adapt);
		}
	}

	/* If the requested partition mode is not a valid combination for the current number of
	 * VFs, set it to the first supported memory partition mode for the requested accelerator
	 * partition mode. */
	if (amdgv_nbio_is_partition_mode_supported(adapt,
			adapt->mcp.memory_partition_mode,
			adapt->mcp.accelerator_partition_mode) == false) {

		/* Check if the combination is supported in the capability table */
		if (amdgv_nbio_get_asic_nps_caps(adapt, &supported_combinations_for_num_vf))
			return PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;

		adapt->mcp.memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_UNKNOWN;
		for (i = 0; i < AMDGV_NPS_COMPUTE_MAX_COMBINATIONS; i++) {
			if (supported_combinations_for_num_vf[i].compute_mode ==
				adapt->mcp.accelerator_partition_mode) {
				adapt->mcp.memory_partition_mode =
					supported_combinations_for_num_vf[i].nps_mode;
				break;
			}
		}

		/* There are valid accelerator and NPS mode combinations, but no valid memory
		 * partition is found for the current accelerator partition. Fallback to the
		 * first NPS mode in the caps table and choose its corresponding default accel
		 * partition mode */
		if (adapt->mcp.memory_partition_mode == AMDGV_MEMORY_PARTITION_MODE_UNKNOWN) {
			default_compute_mode =
				amdgv_nbio_get_default_accel_partition_mode(adapt,
					supported_combinations_for_num_vf[0].nps_mode);
			AMDGV_WARN("memory partition mode=NPS%u and accelerator partition "
				"mode=%s is not valid for %uVF. Fallback to accelerator "
				"partition mode=%s, memory partition mode=NPS%u\n",
				adapt->mcp.memory_partition_mode,
				psp_v15_0_8_accel_mode_desc(adapt,
					adapt->mcp.accelerator_partition_mode),
				adapt->num_vf,
				psp_v15_0_8_accel_mode_desc(adapt, default_compute_mode),
				supported_combinations_for_num_vf[0].nps_mode);
			adapt->mcp.accelerator_partition_mode = default_compute_mode;
			adapt->mcp.memory_partition_mode =
				supported_combinations_for_num_vf[0].nps_mode;
		}
	}

	/* If mem_mode_switch_requested is set to true, compare the current mode
	 * with saved mode, and clear the flag if the modes match. */
	if (adapt->mcp.mem_mode_switch_requested) {
		if (adapt->mcp.memory_partition_mode == curr_memory_partition_mode) {
			adapt->mcp.mem_mode_switch_requested = false;
		} else {
			// TODO: check if additional handling is needed for mismatch
			AMDGV_WARN("Requested memory partition mode change to NPS%u but current mode is NPS%u\n",
				adapt->mcp.memory_partition_mode, curr_memory_partition_mode);
		}
	}

	oss_schedule_work(adapt->dev, save_memory_partition_mode, (void *)adapt);

	AMDGV_INFO("memory_partition_mode=NPS%u\n", curr_memory_partition_mode);

	return PSP_STATUS__SUCCESS;
}

static int psp_v15_0_8_hw_init(struct amdgv_adapter *adapt)
{
	int r = 0;
	int i;
	int pos;
	uint16_t ctrl;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;
	enum amdgv_firmware_id ucode_np_seq[] = {
		AMDGV_FIRMWARE_ID__RLC,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM,
		AMDGV_FIRMWARE_ID__RLX6,
		AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT,
		AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1,
		AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1,
		AMDGV_FIRMWARE_ID__SDMA0,
		AMDGV_FIRMWARE_ID__RS64_MEC_UCODE,
		AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA,
		AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA,
		AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA,
		AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA,
		AMDGV_FIRMWARE_ID__RS64_MES,
		AMDGV_FIRMWARE_ID__RS64_MES_STACK,
		AMDGV_FIRMWARE_ID__RS64_KIQ,
		AMDGV_FIRMWARE_ID__RS64_KIQ_STACK,
		AMDGV_FIRMWARE_ID__DFC_FW,
	};

	if (adapt->fw_load_type == AMDGV_FW_LOAD_DIRECT)
		return 0;

	adapt->psp.idx = 0;

	// Driver does not need to load PSP FWs in PSP_14
	psp_v15_0_8_get_bootloader_version(adapt);

	psp_ret = psp_v15_0_8_ring_start(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_FW_START_RING_FAIL, 0);
		return AMDGV_FAILURE;
	}

	psp_ret = amdgv_psp_cmd_km_start(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	/* PSP needs this to know that we are SRIOV */
	psp_ret = psp_v15_0_8_set_num_vfs(adapt, adapt->num_vf);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	/* Check and configure CC mode if needed */
	psp_ret = psp_v15_0_8_check_cc_mode(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	psp_ret = psp_v15_0_8_check_memory_partition_mode(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	psp_ret = psp_v15_0_8_set_accelerator_partition_mode(adapt, adapt->mcp.accelerator_partition_mode);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}
	r = psp_v15_0_8_load_toc(adapt);
	if (r)
		goto init_fail;

	/* Disable psp mailbox interrupts for all vfs */
	for (i = 0; i < adapt->num_vf; i++)
		psp_v15_0_8_set_mb_int(adapt, i, false);

	/* Load ucode for non-PSP components */
	r = adapt->ucode.load(adapt, ucode_np_seq, ARRAY_SIZE(ucode_np_seq));
	if (r)
		return r;

	/* start rlc autoload after psp recieved all gfx firmware */
	psp_ret = amdgv_psp_start_rlc_autoload(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	// Continue on AID0
	adapt->psp.idx = 0;
	adapt->psp.tee_version =
		(RREG32(SOC15_REG_OFFSET(MP0, adapt->psp.idx, regMPASP_SMN_C2PMSG_64)) &
		 GFX_CMD_TEE_VERSION_MASK) >>
		GFX_CMD_TEE_VERSION_SHIFT;
	AMDGV_DEBUG("PSP: tee_version=%x\n", adapt->psp.tee_version);

	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	psp_ret = amdgv_iovm_drv_hw_init(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	/* Check if ATS is enabled before programming register */
	pos = oss_pci_find_ext_cap(adapt->dev, PCIE_EXT_CAP_ID__ATS);
	if (pos) {
		oss_pci_read_config_word(adapt->dev, pos + PCI_ATS_CTRL, &ctrl);
		if (ctrl & PCI_ATS_CTRL_ENABLE)
			psp_v15_0_8_program_register(adapt, 0, 0, 0, VM_IOMMU_CONTROL_WA);
	}

init_fail:
	if (r)
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_FW_INIT_FAIL, 0);
	return r;
}

static int psp_v15_0_8_hw_fini(struct amdgv_adapter *adapt)
{
	int r = 0;
	enum psp_status ret;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *local_mem = &(psp->tmr_context);
	struct psp_cmd_km tmr_km_cmd = { 0 };
	struct psp_ras_context *ras_context = &psp->ras_context;

	if (adapt->fw_load_type == AMDGV_FW_LOAD_DIRECT)
		return 0;

	if (ras_context->ras_initialized) {
		if (adapt->reset.reset_state)
			/*For mode1 reset, directly set .ras_initialized to false.*/
			ras_context->ras_initialized = false;
		else
			amdgv_psp_ras_terminate(adapt);
	}

	ras_context->ta_version = 0x0;

	if (!oss_atomic_read(&adapt->in_ecc_recovery)) {
		/* destroy TMR
		* TMR Destroy will destroy all the ucode resident inside the TMR
		* No need to destroy TMR in reset process
		*/
		if (local_mem->mem) {
			/* Prepare CMD to destroy TMR */
			tmr_km_cmd.cmd_id = PSP_CMD_KM_TYPE__DESTROY_TMR;
			tmr_km_cmd.cmd.setup_tmr.tmr_size = local_mem->size;
			tmr_km_cmd.cmd.setup_tmr.tmr_buf_addr_lo =
				lower_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
			tmr_km_cmd.cmd.setup_tmr.tmr_buf_addr_hi =
				upper_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));

			/* Submit CMD buffer to destroy TMR */
			ret = amdgv_psp_cmd_km_submit(adapt, &tmr_km_cmd, NULL);
			if (ret != PSP_STATUS__SUCCESS)
				r = AMDGV_FAILURE;
		}

		if (psp_v15_0_8_ring_destroy(adapt)) {
			r = AMDGV_FAILURE;
			AMDGV_ERROR("PSP: Failed to DESTROY_RINGS.\n");
		}
	}
	return r;
}

static int psp_v15_0_8_sw_fini(struct amdgv_adapter *adapt)
{
	int r = 0;

	adapt->ucode.load = NULL;
	adapt->ucode.get_ucode_start_addr = NULL;

	r = amdgv_iovm_drv_sw_fini(adapt);

	if (adapt->fw_load_type == AMDGV_FW_LOAD_DIRECT)
		return 0;

	if (amdgv_psp_sw_fini(adapt) != PSP_STATUS__SUCCESS)
		r = AMDGV_FAILURE;

	return r;
}

static enum psp_status psp_v15_0_8_tmr_init(struct amdgv_adapter *adapt, uint32_t tmr_size)
{
	if (!MEM_RSV_REGION_FILLED(adapt, REGION_ID__UMF)) {
		AMDGV_INFO("PSP: UMF not supported, using common tmr\n");
		return amdgv_psp_tmr_init(adapt, adapt->psp.allocated_tmr_size);
	} else {
		/* Need to extract build num to check if the mem_rsv_info table need flipped */
		if (adapt->vbios.special_version_check) {
			AMDGV_INFO("PSP: Extracting build_num from IP Discovery ATOMBIOS table\n");
			adapt->vbios.special_version_check(adapt, adapt->vbios.ip_discovery_image);
		}

		AMDGV_INFO("PSP: UMF supported, using UMF\n");
		return psp_v15_0_8_reserve_umf_region(adapt, adapt->mem_rsv_info.entries[REGION_ID__UMF].start_addr, adapt->mem_rsv_info.entries[REGION_ID__UMF].size);

	}
}

/*
 * Route the embedded RAS TA/RL firmware accessors by detected PSP (MP0) IP
 * version. Add a row when a new PSP revision in this family ships embedded RAS
 * firmware; versions absent from the table leave the accessors NULL, so
 * amdgv_psp_get_ras_{ta,rl}_fw returns AMDGV_FAILURE (no embedded firmware).
 */
struct psp_ras_fw_route {
	uint32_t ip_version;
	int (*get_ras_ta_fw)(struct amdgv_adapter *adapt,
			uint8_t **bin_addr, uint32_t *bin_size,
			uint32_t *fw_version, uint32_t *feature_version);
	int (*get_ras_rl_fw)(struct amdgv_adapter *adapt,
			uint8_t **bin_addr, uint32_t *bin_size,
			uint32_t *fw_version, uint32_t *feature_version);
};

static const struct psp_ras_fw_route psp_ras_fw_routes[] = {
	{ IP_VERSION(15, 0, 8), amdgv_psp_v15_0_8_get_ras_ta_fw,
	  amdgv_psp_v15_0_8_get_ras_rl_fw },
};

void psp_set_ras_fw_accessors(struct amdgv_adapter *adapt)
{
	uint32_t psp_ip_version = adapt->ip_versions[MP0_HWIP][GET_INST(MP0, 0)];
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(psp_ras_fw_routes); i++) {
		if (psp_ras_fw_routes[i].ip_version != psp_ip_version)
			continue;

		adapt->psp.get_ras_ta_fw = psp_ras_fw_routes[i].get_ras_ta_fw;
		adapt->psp.get_ras_rl_fw = psp_ras_fw_routes[i].get_ras_rl_fw;
		return;
	}

	AMDGV_WARN("PSP: no embedded RAS TA/RL firmware accessors for MP0 IP version 0x%08x\n",
		   psp_ip_version);
}

static int psp_v15_0_8_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;

	if (adapt->fw_load_type == AMDGV_FW_LOAD_DIRECT)
		return 0;

	oss_memset(&adapt->psp, 0, sizeof(struct psp_context));
	if (MEM_RSV_REGION_FILLED(adapt, REGION_ID__UMF)) {
		adapt->psp.allocated_tmr_size = adapt->mem_rsv_info.entries[REGION_ID__UMF].size;
		AMDGV_INFO("PSP: UMF size from IP Discovery: 0x%llx\n",
			   adapt->mem_rsv_info.entries[REGION_ID__UMF].size);
	} else {
		adapt->psp.allocated_tmr_size = 0x8400000;
		AMDGV_WARN("PSP: UMF size not found in IP Discovery, using default TMR 0x%llx\n",
			0x8400000);
	}
	adapt->psp.tmr_context.size = adapt->psp.allocated_tmr_size;
	adapt->psp.tmr_context.alignment = PSP_TMR_ALIGNMENT;
	adapt->psp.fw_id_support = amdgv_psp_fw_id_support;
	adapt->psp.program_register = psp_v15_0_8_program_register;
	adapt->psp.get_wptr = psp_v15_0_8_ring_get_wptr;
	adapt->psp.set_wptr = psp_v15_0_8_ring_set_wptr;
	adapt->psp.set_mb_int = psp_v15_0_8_set_mb_int;
	adapt->psp.get_mb_int_status = psp_v15_0_8_get_mb_int_status;
	adapt->psp.vfgate_support = psp_v15_0_8_vfgate_support;
	adapt->psp.clear_vf_fw = psp_v15_0_8_clear_vf_fw;
	adapt->psp.psp_program_guest_mc_settings = psp_v15_0_8_program_guest_mc_settings;
	adapt->psp.need_switch_to_pf = psp_v15_0_8_need_switch_to_pf;
	/* false on PSP TEE 3.0 */
	adapt->psp.ras_need_switch_to_pf = psp_v15_0_8_need_switch_to_pf;
	adapt->psp.copy_vf_chiplet_regs = psp_v15_0_8_copy_vf_chiplet_regs;
	adapt->psp.fw_attestation_support = psp_v15_0_8_fw_attestation_support;
	psp_set_ras_fw_accessors(adapt);
	adapt->ucode.load = psp_v15_0_8_ucode_load;
	adapt->ucode.get_ucode_start_addr = psp_v15_0_8_ucode_get_start_addr;
	adapt->psp.tmr_init = psp_v15_0_8_tmr_init;
	adapt->psp.enable_interrupt = psp_v15_0_8_enable_interrupt;
	adapt->psp.handle_irq = psp_v15_0_8_handle_irq;

	psp_ret = amdgv_psp_sw_init(adapt);
	adapt->psp.ras_context.set_init_flag = true;

	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_FW_INIT_FAIL, 0);
		psp_v15_0_8_sw_fini(adapt);
		ret = AMDGV_FAILURE;
	}

	if (ret == PSP_STATUS__SUCCESS)
		ret = amdgv_iovm_drv_sw_init(adapt);

	return ret;
}

const struct amdgv_init_func psp_v15_0_8_func = {
	.name = "psp_v15_0_8_func",
	.sw_init = psp_v15_0_8_sw_init,
	.sw_fini = psp_v15_0_8_sw_fini,
	.hw_init = psp_v15_0_8_hw_init,
	.hw_fini = psp_v15_0_8_hw_fini,
};
