/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_vbios.h>
#include <amdgv_pci_def.h>

#include "mi300.h"
#include "mi300_gfx.h"
#include "mi300_gpuiov.h"
#include "mi300_nbio.h"
#include "mi300_psp.h"
#include "mi300_nbio.h"
#include "mi300_gfx.h"
#include "mi300_ecc.h"
#include "mi300_ip_discovery.h"

#define PSP_BL_VERSION_STATUS_SUPPORT 0x00A10109

/* these registers are used before ip discovery, so we cannot calculate the offset */
#define reg_mmMP0_SMN_C2PMSG_33                             0x16061
#define reg_mmMP0_SMN_C2PMSG_35                             0x16063
#define reg_mmMP0_SMN_C2PMSG_81                             0x16091

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;
/* PSP FW reserves the TMR area. Host driver need not reserve it. */
enum { PSP_TMR_SIZE = 0x0 };

int mi300_psp_ring_destroy(struct amdgv_adapter *adapt)
{
	uint32_t flag = 0x80000000;
	uint32_t mask = 0x8000FFFF;
	struct psp_context *psp = &adapt->psp;
	int wait_ret;

	/* Write the ring destroy command to C2PMSG_64 */
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_64),
	       GFX_CTRL_CMD_ID_DESTROY_RINGS);

	/* Wait for response flag (bit 31) in C2PMSG_64 */
	wait_ret = amdgv_wait_for_register(
		adapt, SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_64), mask, flag,
		AMDGV_TIMEOUT(TIMEOUT_PSP_REG), AMDGV_WAIT_CHECK_EQ, 0);

	if (wait_ret)
		return AMDGV_FAILURE;
	else
		return 0;
}

enum psp_status mi300_psp_ring_start(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_ring *ring;
	struct psp_local_memory *local_mem = NULL;
	uint32_t psp_ring_reg = 0; // MP0_SMN_C2PMSG_64 - 71
	struct psp_context *psp = &adapt->psp;
	int i;

	ring = &psp->km_ring[psp->idx];

	local_mem = &ring->ring_mem;

	/* Wait for TOS ready for ring creation */
	for (i = 0; i < adapt->mcp.num_aid; i++) {
		ret = amdgv_psp_wait_for_register(
			adapt, SOC15_REG_OFFSET(MP0, i, regMP0_SMN_C2PMSG_64), 0x80000000,
			0x80000000, false);

		if (ret != PSP_STATUS__SUCCESS) {
			AMDGV_ERROR(
				"PSP-%d: Waiting for TOS ready for ring creation failed.\n",
				i);
			return ret;
		}
	}

	/* Write low address of the ring to C2PMSG_69 */
	psp_ring_reg = lower_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_69), psp_ring_reg);

	/* Write high address of the ring to C2PMSG_70 */
	psp_ring_reg = upper_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_70), psp_ring_reg);

	/* Write size of ring to C2PMSG_71 */
	psp_ring_reg = local_mem->size;
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_71), psp_ring_reg);

	/* Write the ring initialization command to C2PMSG_64 */
	psp_ring_reg = ring->ring_type;
	psp_ring_reg = psp_ring_reg << 16;
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_64), psp_ring_reg);

	/* Wait for response flag (bit 31) in C2PMSG_64 */
	ret = amdgv_psp_wait_for_register(
		adapt, SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_64), 0x80000000,
		0x8000FFFF, false);

	if (ret != PSP_STATUS__SUCCESS)
		AMDGV_ERROR("PSP: Failed to start ring.\n");

	return ret;
}

static enum psp_status
mi300_psp_bootloader_load_component(struct amdgv_adapter *adapt, unsigned char *fw_image,
				    uint32_t fw_image_size,
				    enum psp_bootloader_command_list bl_cmd)
{
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory load_mem = psp->private_fw_memory;

	if (!load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy PSP component binary to memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(load_mem.mem), fw_image, fw_image_size);

	/* Provide the component to bootloader */
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(amdgv_memmgr_get_gpu_addr(load_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_35), bl_cmd);

	/* wait for loading completed */
	if (amdgv_psp_wait_for_register(
		    adapt, SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_35), 0x80000000,
		    0x80000000, false) != PSP_STATUS__SUCCESS) {
		return PSP_STATUS__ERROR_GENERIC;
	}

	return PSP_STATUS__SUCCESS;
}

enum psp_status mi300_psp_load_key_db(struct amdgv_adapter *adapt, unsigned char *fw_image,
				      uint32_t fw_image_size)
{
	enum psp_status ret;
	uint32_t fw_ver;

	ret = mi300_psp_bootloader_load_component(adapt, fw_image, fw_image_size,
						  PSP_BL__LOAD_KEY_DATABASE);

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP-%d: Key DB(version:%X.%X.%X.%X) is loaded.\n", adapt->psp.idx,
			   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF, (fw_ver >> 8) & 0xFF,
			   fw_ver & 0xFF);
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_KEYDB] = fw_ver;
	}

	return ret;
}

enum psp_status mi300_psp_load_sys_drv(struct amdgv_adapter *adapt, unsigned char *fw_image,
				       uint32_t fw_image_size)
{
	enum psp_status ret;
	uint32_t fw_ver;

	ret = mi300_psp_bootloader_load_component(adapt, fw_image, fw_image_size,
						  PSP_BL__LOAD_SYSDRV);

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP-%d: SYS DRV(version:%X.%X.%X.%X) is loaded.\n", adapt->psp.idx,
			   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF, (fw_ver >> 8) & 0xFF,
			   fw_ver & 0xFF);
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SYS] = fw_ver;
	}

	return ret;
}

enum psp_status mi300_psp_load_ras_drv(struct amdgv_adapter *adapt,
			unsigned char *fw_image, uint32_t fw_image_size)
{
	enum psp_status ret;
	uint32_t fw_ver;

	ret = mi300_psp_bootloader_load_component(
		adapt, fw_image, fw_image_size, PSP_BL__LOAD_RASDRV);

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver =
			((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP-%d: RAS DRV(version:%X.%X.%X.%X) is loaded.\n",
			adapt->psp.idx,
			(fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
			(fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_RAS] = fw_ver;
	}

	return ret;
}

enum psp_status mi300_psp_load_soc_drv(struct amdgv_adapter *adapt, unsigned char *fw_image,
				       uint32_t fw_image_size)
{
	enum psp_status ret;
	uint32_t fw_ver;

	ret = mi300_psp_bootloader_load_component(adapt, fw_image, fw_image_size,
						  PSP_BL__LOAD_SOCDRV);

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP-%d: SOC DRV(version:%X.%X.%X.%X) is loaded.\n", adapt->psp.idx,
			   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF, (fw_ver >> 8) & 0xFF,
			   fw_ver & 0xFF);
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SOC] = fw_ver;
	}

	return ret;
}

enum psp_status mi300_psp_load_intf_drv(struct amdgv_adapter *adapt, unsigned char *fw_image,
					uint32_t fw_image_size)
{
	enum psp_status ret;
	uint32_t fw_ver;

	ret = mi300_psp_bootloader_load_component(adapt, fw_image, fw_image_size,
						  PSP_BL__LOAD_INTFDRV);

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP-%d: INTF DRV(version:%X.%X.%X.%X) is loaded.\n",
			   adapt->psp.idx, (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
			   (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_INTF] = fw_ver;
	}

	return ret;
}

enum psp_status mi300_psp_load_dbg_drv(struct amdgv_adapter *adapt, unsigned char *fw_image,
				       uint32_t fw_image_size)
{
	enum psp_status ret;
	uint32_t fw_ver;

	ret = mi300_psp_bootloader_load_component(adapt, fw_image, fw_image_size,
						  PSP_BL__LOAD_DBGDRV);

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP-%d: DBG DRV(version:%X.%X.%X.%X) is loaded.\n", adapt->psp.idx,
			   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF, (fw_ver >> 8) & 0xFF,
			   fw_ver & 0xFF);
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_DBG] = fw_ver;
	}

	return ret;
}

enum psp_status mi300_psp_load_sos(struct amdgv_adapter *adapt, unsigned char *fw_image,
				   uint32_t fw_image_size)
{
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_sos_load_mem = psp->private_fw_memory;
	uint32_t fw_ver;

	if (!psp_sos_load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy Secure OS binary to PSP memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_sos_load_mem.mem), fw_image, fw_image_size);

	/* Provide the sos driver to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(amdgv_memmgr_get_gpu_addr(psp_sos_load_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_35), PSP_BL__LOAD_SOSDRV);

	/* Check sOS sign of life register to confirm that
	 * PSP drv_sys and sOS are loaded, alive and ready to respond
	 * to GFX mailbox (commands from host and VMs)
	 */
	if (mi300_psp_wait_sos_loaded_status(adapt) == false) {
		AMDGV_ERROR("TIMEOUT waiting for PSP tOS sign-of-life\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

	AMDGV_INFO("PSP-%d: OS(version:%X.%X.%X.%X) is loaded.\n", adapt->psp.idx,
		   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF, (fw_ver >> 8) & 0xFF,
		   fw_ver & 0xFF);
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SOS] = fw_ver;

	return PSP_STATUS__SUCCESS;
}

static enum psp_status mi300_psp_program_register(struct amdgv_adapter *adapt, uint32_t idx_vf,
						  uint32_t reg_value, uint32_t reg_value_hi,
						  enum psp_ih_reg reg_id)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *program_reg_cmd = adapt->psp.psp_cmd_km_mem;

	if (!program_reg_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
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

	if (ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("PSP: Failed to Write Register %d\n", reg_id);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	/* Clear system memory used for Program Reg CMD */
	oss_memset(program_reg_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

enum psp_status mi300_psp_program_guest_mc_settings(struct amdgv_adapter *adapt,
						    uint32_t idx_vf)
{
	struct amdgv_vf_device *vf;
	uint64_t fb_base, fb_top, nonsurface_mc;
	uint32_t fb_location_base, fb_location_top;
	uint32_t sys_aper_lo, sys_aper_hi;
	uint32_t nonsurface_mc_lo, nonsurface_mc_hi;

	vf = &adapt->array_vf[idx_vf];

	/* copy pf fb base to vf*/
	fb_base = adapt->mc_fb_loc_addr;

	if (adapt->xgmi.phy_nodes_num > 1) {
		fb_top = fb_base +
			 MBYTES_TO_BYTES(vf->real_fb_size * adapt->xgmi.phy_nodes_num) - 1;
	} else
		fb_top = fb_base + MBYTES_TO_BYTES(vf->real_fb_size) - 1;

	AMDGV_DEBUG("fb_base = 0x%llx\n", fb_base);
	AMDGV_DEBUG("fb_top  = 0x%llx\n", fb_top);

	fb_location_base = MB_TO_16MB(TO_MBYTES(fb_base));
	fb_location_top = MB_TO_16MB(TO_MBYTES(fb_top));

	sys_aper_lo = TO_256KBYTES(fb_base);
	sys_aper_hi = TO_256KBYTES(fb_top);

	if (adapt->xgmi.phy_nodes_num > 1) {
		nonsurface_mc =
			fb_base + MBYTES_TO_BYTES(vf->real_fb_size * adapt->xgmi.phy_node_id);
	} else
		nonsurface_mc = fb_base;

	nonsurface_mc_lo = lower_32_bits(TO_256BYTES(nonsurface_mc));
	nonsurface_mc_hi = upper_32_bits(TO_256BYTES(nonsurface_mc));

	AMDGV_DEBUG("fb_location_base = 0x%08x\n", fb_location_base);
	AMDGV_DEBUG("fb_location_top  = 0x%08x\n", fb_location_top);
	AMDGV_DEBUG("sys_aper_lo      = 0x%08x\n", sys_aper_lo);
	AMDGV_DEBUG("sys_aper_hi      = 0x%08x\n", sys_aper_hi);
	AMDGV_DEBUG("nonsurface_mc_lo = 0x%08x\n", nonsurface_mc_lo);
	AMDGV_DEBUG("nonsurface_mc_hi = 0x%08x\n", nonsurface_mc_hi);

	if (mi300_psp_program_register(adapt, idx_vf, fb_location_base, 0,
				       GC_MC_VM_FB_LOCATION_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (mi300_psp_program_register(adapt, idx_vf, fb_location_base, 0,
				       MM_MC_VM_FB_LOCATION_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (mi300_psp_program_register(adapt, idx_vf, fb_location_top, 0,
				       GC_MC_VM_FB_LOCATION_TOP))
		return PSP_STATUS__ERROR_GENERIC;
	if (mi300_psp_program_register(adapt, idx_vf, fb_location_top, 0,
				       MM_MC_VM_FB_LOCATION_TOP))
		return PSP_STATUS__ERROR_GENERIC;
	if (mi300_psp_program_register(adapt, idx_vf, sys_aper_lo, 0,
				       GC_MC_SYSTEM_APERTURE_LO))
		return PSP_STATUS__ERROR_GENERIC;
	if (mi300_psp_program_register(adapt, idx_vf, sys_aper_lo, 0,
				       MM_MC_SYSTEM_APERTURE_LO))
		return PSP_STATUS__ERROR_GENERIC;
	if (mi300_psp_program_register(adapt, idx_vf, sys_aper_hi, 0,
				       GC_MC_SYSTEM_APERTURE_HI))
		return PSP_STATUS__ERROR_GENERIC;
	if (mi300_psp_program_register(adapt, idx_vf, sys_aper_hi, 0,
				       MM_MC_SYSTEM_APERTURE_HI))
		return PSP_STATUS__ERROR_GENERIC;
	if (mi300_psp_program_register(adapt, idx_vf, nonsurface_mc_lo, 0,
				       HDP_NONSURFACE_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (mi300_psp_program_register(adapt, idx_vf, nonsurface_mc_hi, 0,
				       HDP_NONSURFACE_BASE_HI))
		return PSP_STATUS__ERROR_GENERIC;

	return PSP_STATUS__SUCCESS;
}

static enum psp_status mi300_psp_set_mb_int(struct amdgv_adapter *adapt, uint32_t idx_vf,
					    bool enable)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *vfgate_cmd = adapt->psp.psp_cmd_km_mem;
	bool resp_enable = false;
	struct psp_gfx_resp psp_resp = { 0 };

	if (!vfgate_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
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

	if (ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_VFGATE_FAIL, 0);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	/* Clear system memory used for VFGATE CMD */
	oss_memset(vfgate_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

static enum psp_status mi300_psp_vfgate_support(struct amdgv_adapter *adapt)
{
	return PSP_STATUS__SUCCESS;
}

static enum psp_status mi300_psp_get_mb_int_status(struct amdgv_adapter *adapt,
						   uint32_t idx_vf,
						   struct psp_mb_status *mb_status)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *vfgate_cmd = adapt->psp.psp_cmd_km_mem;
	struct psp_gfx_resp psp_resp = { 0 };
	uint32_t gfx_fw_type = 0;
	enum amdgv_firmware_id psp_fw_id = (enum amdgv_firmware_id)AMDGV_FIRMWARE_ID__MAX;

	ret = mi300_psp_vfgate_support(adapt);

	if (ret == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE)
		return ret;

	if (!vfgate_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	vfgate_cmd->cmd_id = PSP_CMD_KM_TYPE__VFGATE;
	vfgate_cmd->cmd.vfgate.target_vfid = idx_vf;

	vfgate_cmd->cmd.vfgate.action = GFX_SCMD_VFGATE_STATUS;

	ret = amdgv_psp_cmd_km_submit(adapt, vfgate_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_VFGATE_FAIL, 0);
		ret = PSP_STATUS__ERROR_GENERIC;
	} else {
		if (psp_resp.sriov_mbstatus & SRIOV_MBSTATUS_ISENABLED_MASK)
				mb_status->vf_gate_enabled = true;
		else
				mb_status->vf_gate_enabled = false;

		gfx_fw_type = (psp_resp.sriov_mbstatus & SRIOV_MBSTATUS_ERR_FWTYPE_MASK) >> 8;
		psp_fw_id = (enum amdgv_firmware_id)amdgv_psp_gfx_fw_id_map(gfx_fw_type);

		mb_status->drv_version = psp_resp.uresp.vfgate.drv_version;
		mb_status->fw_id = (uint32_t)psp_fw_id;
		mb_status->status = psp_resp.sriov_mbstatus & SRIOV_MBSTATUS_ERRSTATUS_MASK;

		AMDGV_DEBUG("PSP mailbox status drv_version = 0x%x\n", mb_status->drv_version);
		AMDGV_DEBUG("PSP mailbox status errStatus = 0x%x\n", mb_status->status);
		AMDGV_DEBUG("PSP mailbox status psp fw id = 0x%x\n", (uint32_t)psp_fw_id);
	}

	/* Clear system memory used for VFGATE CMD */
	oss_memset(vfgate_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

static enum psp_status mi300_psp_clear_vf_fw(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__CLEAR_VF_FW;
	psp_cmd.cmd.clear_vf_fw.target_vf = idx_vf;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS || psp_resp.status != 0) {
		AMDGV_INFO("PSP: failed to submit GFX_CMD_ID_CLEAR_VF_FW "
			   "(gfx_cmd_resp=0x%08x)\n",
			   psp_resp.status);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

static uint32_t mi300_psp_ring_get_wptr(struct amdgv_adapter *adapt)
{
	return RREG32(SOC15_REG_OFFSET(MP0, adapt->psp.idx, regMP0_SMN_C2PMSG_67));
}

static void mi300_psp_ring_set_wptr(struct amdgv_adapter *adapt, uint32_t value)
{
	WREG32(SOC15_REG_OFFSET(MP0, adapt->psp.idx, regMP0_SMN_C2PMSG_67), value);
}

static enum psp_status mi300_psp_set_num_vfs(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__NUM_ENABLED_VFS;
	psp_cmd.cmd.num_vfs.number_of_vfs = num_vf;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS || psp_resp.status != 0) {
		AMDGV_INFO("PSP: failed to submit GFX_CMD_ID_NUM_ENABLED_VFS "
			   "(gfx_cmd_resp=0x%08x)\n",
			   psp_resp.status);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

enum psp_status mi300_psp_fw_attestation_support(struct amdgv_adapter *adapt)
{
	return PSP_STATUS__SUCCESS;
}

void mi300_psp_get_fw_version(enum amdgv_firmware_id firmware_id, uint32_t image_version,
			      char *fw_version, uint32_t size)
{
	switch (firmware_id) {
	case AMDGV_FIRMWARE_ID__SDMA0:
	case AMDGV_FIRMWARE_ID__SDMA1:
	case AMDGV_FIRMWARE_ID__SDMA2:
	case AMDGV_FIRMWARE_ID__SDMA3:
	case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
	case AMDGV_FIRMWARE_ID__CP_MEC1:
		if (image_version & 0x8000)
			oss_vsnprintf(fw_version, size, "%d(Two-level)",
				      image_version & 0x7FFF);
		else
			oss_vsnprintf(fw_version, size, "%d", image_version);
		break;
	case AMDGV_FIRMWARE_ID__VCN:
		oss_vsnprintf(fw_version, size, "%d.%d.%d.%d", (image_version >> 24) & 0xF,
			      (image_version >> 20) & 0xF, (image_version >> 12) & 0xFF,
			      image_version & 0xFFF);
		break;
	default:
		oss_vsnprintf(fw_version, size, "unknown");
		break;
	}
}

enum psp_status mi300_psp_fb_addr_bound_check(struct amdgv_adapter *adapt, uint64_t fb_addr,
					      uint64_t size)
{
	uint64_t upper_bound = 0, lower_bound = 0;
	uint64_t config_memsize = 0;

	if ((void *)fb_addr == NULL || size <= 0) {
		AMDGV_ERROR("Invalid fb address information.\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	// Get reserved vbios size from firmware info table
	config_memsize = mi300_nbio_get_config_memsize(adapt);

	upper_bound = (adapt->memmgr_pf).mc_base + (config_memsize << 20);
	lower_bound = (adapt->memmgr_pf).mc_base +
		      ((config_memsize - amdgv_vbios_get_fw_reserved_size(adapt)) << 20);

	AMDGV_INFO("Calclated psp fb upper bound is %llx, Lower bound is %llx\n", upper_bound,
		    lower_bound);

	if ((fb_addr < lower_bound) || (fb_addr > upper_bound)) {
		AMDGV_ERROR("Fb address 0x%llx out of bound, psp reserved fb is 0x%llx - 0x%llx.\n",
			fb_addr, lower_bound, upper_bound);
	}

	return PSP_STATUS__SUCCESS;
}

enum psp_status mi300_psp_get_fw_attestation_database_addr(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	void *cpu_addr;

	if (!amdgv_psp_fw_attestation_support(adapt)) {
		AMDGV_WARN("FW attestation feature is currently not supported.\n");
		return ret;
	}

	if (!adapt->opt.skip_hw_init) {
		// Get FW attestation GPU virtual address
		ret = amdgv_psp_get_fw_attestation_db_add(adapt);

		(adapt->psp).attestation_db_gpu_addr += adapt->mc_fb_loc_addr;

		if (ret != PSP_STATUS__SUCCESS) {
			AMDGV_ERROR("Failed to get fw attestation address.\n");
			return PSP_STATUS__ERROR_GENERIC;
		}
	}
	// FW attestation mc_address bound check
	ret = mi300_psp_fb_addr_bound_check(adapt, (adapt->psp).attestation_db_gpu_addr,
					    sizeof(struct ATTESTATION_DB_HEADER));

	if (ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("Attestation mc_address locates at an invalid region.\n");
		(adapt->psp).attestation_db_gpu_addr = 0;
		return PSP_STATUS__ERROR_GENERIC;
	}

	// Get FW attestation CPU virtual address
	if (amdgv_acquire_fb_virtual_addr(adapt, (adapt->psp).attestation_db_gpu_addr,
					  sizeof(struct ATTESTATION_DB_HEADER),
					  (uint64_t *)&cpu_addr) == 0) {
		(adapt->psp).attestation_db_cpu_addr = cpu_addr;
		AMDGV_INFO("cpu addr is %x.\n", cpu_addr);

	} else {
		AMDGV_INFO("Acquire frame buffer cpu virtual address failed, cpu virtual "
			    "addr is NULL.\n");
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

enum psp_status mi300_psp_clear_fw_attestation_database_addr(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	uint64_t mc_base, gpu_addr, offset;

	if (!amdgv_psp_fw_attestation_support(adapt)) {
		AMDGV_WARN("FW attestation feature is currently not supported.\n");
		return ret;
	}

	mc_base = (adapt->memmgr_pf).mc_base;
	gpu_addr = (adapt->psp).attestation_db_gpu_addr;
	if (!gpu_addr) {
		AMDGV_WARN("FW attestation address is NULL.\n");
		return ret;
	}

	offset = (gpu_addr > mc_base) ? (gpu_addr - mc_base) : (mc_base - gpu_addr);

	// Unmap FW attestation CPU virtual address
	if (amdgv_release_fb_virtual_addr(adapt, (adapt->psp).attestation_db_cpu_addr,
					  offset) == 0)
		(adapt->psp).attestation_db_cpu_addr = NULL;
	else {
		AMDGV_ERROR("Unmap and clear Fw Attestation frame buffer address failed.\n");
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

enum psp_status mi300_psp_get_fw_attestation_info(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct FWMAN_DB_HEADER *pDb = (struct FWMAN_DB_HEADER *)psp->attestation_db_cpu_addr;
	struct amdgv_vf_device *vf = &(adapt->array_vf[idx_vf]);

	uint32_t i = 0;
	uint32_t fw_info_index = 0;
	bool found = false;
	enum amdgv_firmware_id fw_id = AMDGV_FIRMWARE_ID__MAX;
	char fw_version[16] = { 0 };

	if (!amdgv_psp_fw_attestation_support(adapt)) {
		AMDGV_WARN("FW attestation feature is currently not supported.\n");
		return ret;
	}

	// Skip pf, information for pf is saved in adapt->psp.fw_info during load fw
	if (idx_vf == AMDGV_PF_IDX) {
		AMDGV_INFO("Skip get FW info for PF, PF info saved during load FW.\n");
		return ret;
	}

	// Check if fw attestation database address is available
	if (!pDb) {
		AMDGV_ERROR("Attestation table address not available.\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	// Check cookie
	if (pDb->attestation_db_cookie != ATTESTATION_TABLE_COOKIE) {
		AMDGV_ERROR("Attestation table cookie does not match.\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	oss_memset(vf->fw_info, 0,
		   AMDGV_FIRMWARE_ID__MAX * sizeof(struct amdgv_firmware_info));

	for (i = 0; i < MAX_VF_DB_SIZE; ++i) {
		struct FWMAN_RECORD *vf_entry = (struct FWMAN_RECORD *)&(pDb->vf_table[i]);

		if (vf_entry->FwManId.FwEntry.VfId == idx_vf &&
		    vf_entry->record_info.bit0.record_valid) {
			fw_id = amdgv_psp_fw_id_map(vf_entry->FwManId.FwEntry.FwId);

			if (fw_id != AMDGV_FIRMWARE_ID__MAX &&
			    fw_info_index != AMDGV_FIRMWARE_ID__MAX) {
				vf->fw_info[fw_info_index].id = fw_id;
				vf->fw_info[fw_info_index].version = vf_entry->FwVer;
				mi300_psp_get_fw_version(fw_id,
							 vf->fw_info[fw_info_index].version,
							 fw_version, sizeof(fw_version));

				AMDGV_INFO("PSP: id:%d(version:%s) is loaded on vf%d.\n",
					    fw_id, fw_version, idx_vf);

				found = true;
				fw_info_index++;
			}
		}
	}

	if (!found)
		AMDGV_WARN("No attestation record found for vf%d.\n", idx_vf);

	return ret;
}

static const char *mi300_get_accelerator_partition_mode_desc(
	struct amdgv_adapter *adapt, uint32_t accelerator_partition_mode)
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

static int save_accelerator_partition_mode(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;

	oss_save_accelerator_partition_mode(adapt->dev,
					    adapt->mcp.accelerator_partition_mode);

	return 0;
}

enum psp_status
mi300_psp_set_accelerator_partition_mode(struct amdgv_adapter *adapt,
					uint32_t accelerator_partition_mode)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__SRIOV_SPATIAL_PART;

	/* if requested accelerator_partition_mode is not supported
	 * set accelerator_partition_mode to default as follows:
	 * 			NPS1		NPS4
	 * 1VF		SPX			CPX
	 * 2VF		DPX			X
	 * 4VF		CPX-4/QPX	CPX-4/QPX
	 * 8VF		CPX			CPX
	 */
	if (adapt->mcp.accelerator_partition_mode == 0 ||
		mi300_nbio_is_accelerator_partition_mode_supported(
			adapt, adapt->mcp.memory_partition_mode,
			accelerator_partition_mode) == false) {
		AMDGV_WARN("accelerator_partition_mode=%u is not supported. "
			"setting to default mode for %uVF and NPS%u mode\n",
			accelerator_partition_mode, adapt->num_vf,
			adapt->mcp.memory_partition_mode);
		accelerator_partition_mode =
			mi300_nbio_get_accelerator_partition_mode_default_setting(adapt,
				adapt->mcp.memory_partition_mode);
		if (!accelerator_partition_mode) {
			AMDGV_ERROR(
				"failed to set default accelerator partition mode for %uVF\n",
				adapt->num_vf);
			return PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
		}
	}

	/* Interpreted as number of partitions desired */
	psp_cmd.cmd.sriov_spatial_part.mode = accelerator_partition_mode;
	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS || psp_resp.status != 0) {
		AMDGV_ERROR("PSP: failed to submit GFX_CMD_ID_SRIOV_SPATIAL_PART "
				"%uVF, number_of_xcps=%u "
				"(gfx_cmd_resp=0x%08x)\n",
				adapt->num_vf, psp_cmd.cmd.sriov_spatial_part.mode,
				psp_resp.status);
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* save accelerator partition mode setting */
	adapt->mcp.accelerator_partition_mode = accelerator_partition_mode;
	oss_schedule_work(adapt->dev, save_accelerator_partition_mode,
			  (void *)adapt);

	AMDGV_INFO("accelerator_partition_mode=%s, num_vf=%u\n",
		mi300_get_accelerator_partition_mode_desc(
			adapt, psp_cmd.cmd.sriov_spatial_part.mode),
		adapt->num_vf);

	return ret;
}

static int save_memory_partition_mode(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;

	oss_save_memory_partition_mode(adapt->dev,
				       adapt->mcp.memory_partition_mode);

	return 0;
}

enum psp_status mi300_psp_set_memory_partition_mode(struct amdgv_adapter *adapt,
		enum amdgv_memory_partition_mode memory_partition_mode)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__NPS_MODE;
	psp_cmd.cmd.sriov_memory_part.num_parts = memory_partition_mode;
	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS || psp_resp.status != 0) {
		AMDGV_ERROR("PSP: failed to submit GFX_CMD_ID_NPS_MODE "
				"NPS%u mode "
				"(gfx_cmd_resp=0x%08x)\n",
				psp_cmd.cmd.sriov_memory_part.num_parts,
				psp_resp.status);
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* save the memory partition mode setting */
	adapt->mcp.memory_partition_mode = memory_partition_mode;
	adapt->mcp.mem_mode_switch_requested = true;
	oss_schedule_work(adapt->dev, save_memory_partition_mode,
			(void *)adapt);

	AMDGV_INFO(
		"PSP: GFX_CMD_ID_NPS_MODE NPS%u mode\n",
		memory_partition_mode);

	return ret;
}

static enum psp_status mi300_psp_check_memory_partition_mode(
	struct amdgv_adapter *adapt)
{
	int ret;
	enum amdgv_memory_partition_mode curr_memory_partition_mode;

	ret = mi300_nbio_get_curr_memory_partition_mode(
		adapt, &curr_memory_partition_mode);
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
				mi300_nbio_get_accelerator_partition_mode_default_setting(
					adapt, curr_memory_partition_mode);
			if (!adapt->mcp.accelerator_partition_mode) {
				AMDGV_ERROR(
					"failed to set default accelerator partition mode for %uVF\n",
					adapt->num_vf);
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
			(void)mi300_discover_ip_nps_table(adapt);
		}
	}

	/* If requested partition modes is not a valid combination for the current
	 * number of VFs, default memory partition mode to NPS1.
	 * Accelerator partition mode will be set to default based on the vf_num
	 * and NPS1 mode subsequently.
	 */
	if (mi300_nbio_is_partition_mode_combination_supported(
		    adapt, adapt->mcp.memory_partition_mode,
			adapt->mcp.accelerator_partition_mode) == false) {
		AMDGV_WARN("memory_partition_mode=NPS%u and accelerator partition mode=%s "
			"is not valid for %uVF. default to NPS1\n",
			adapt->mcp.memory_partition_mode,
			mi300_get_accelerator_partition_mode_desc(
				adapt, adapt->mcp.accelerator_partition_mode),
			adapt->num_vf);
		adapt->mcp.memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS1;
	}

	/* If mem_mode_switch_requested is set to true, compare the current mode
	 * with saved mode, and clear the flag if the modes match.
	 */
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

static enum psp_status
mi300_psp_copy_vf_chiplet_regs(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KMD_TYPE_SRIOV_COPY_VF_CHIPLET_REGS;
	psp_cmd.cmd.sriov_copy_vf_chiplet_regs.source_vfid = idx_vf;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS || psp_resp.status != 0) {
		AMDGV_INFO("PSP: failed to submit PSP_CMD_KMD_TYPE_SRIOV_COPY_VF_CHIPLET_REGS "
			   "(gfx_cmd_resp=0x%08x)\n",
			   psp_resp.status);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

static enum psp_status mi300_psp_dfc_check_guest_version(struct amdgv_adapter *adapt, char *driver_version)
{
	int i;
	uint32_t driver_ver = 0;
	struct dfc_fw_data *driver_version_data = NULL;
	bool in_white_list = false;
	bool in_black_list = false;
	uint32_t total_entries;

	/* Check guest version only when Driver Version exists in DFC AND Verification is Disabled */
	/* GFX_FW_TYPE_DRIVER_VERSION 63 */
	if (adapt->psp.dfc_fw != NULL) {
		total_entries = adapt->psp.dfc_fw->header.dfc_fw_total_entries;
		for (i = 0; i < total_entries; i++) {
			if (adapt->psp.dfc_fw->data[i].dfc_fw_type == 63 &&
				adapt->psp.dfc_fw->data[i].verification_enabled == 0) {
				driver_version_data = &adapt->psp.dfc_fw->data[i];
				break;
			}
		}
	}

	if (driver_version_data == NULL) {
		return 0;
	}

	/* driver_version is uint_8[64] */
	/* driver_version's format is x.y.z*/
	/* It's safe to check the first 8 characters only */
	for (i = 0; i < 8; i++) {
		if (driver_version[i] == 0)
			break;

		if (driver_version[i] >= '0' && driver_version[i] <= '9') {
			driver_ver *= 10;
			driver_ver += driver_version[i] - '0';
		}
	}

	/* check if driver_version is in whitelist */
	for (i = 0; i < 16; i++) {
		if (driver_ver >= driver_version_data->white_list[i].oldest &&
			driver_ver <= driver_version_data->white_list[i].latest) {
			in_white_list = true;
			break;
		}
	}

	/* check if driver_version is in blacklist */
	if (in_white_list) {
		for (i = 0; i < 64; i++) {
			if (driver_ver == driver_version_data->black_list[i]) {
				in_black_list = true;
				break;
			}
		}
	}

	if (in_white_list && !in_black_list)
		return 0;

	return PSP_STATUS__ERROR_GENERIC;
}

uint32_t mi300_psp_get_sos_loaded_status(struct amdgv_adapter *adapt)
{
	if (adapt->psp.idx)
		return RREG32_PCIE_EXT(reg_mmMP0_SMN_C2PMSG_81 | AMDGV_MI300_CROSS_AID_ACCESS(adapt->psp.idx));
	return RREG32(reg_mmMP0_SMN_C2PMSG_81);
}

static int mi300_psp_wait_sos_loaded_status_cb(void *context)
{
	void **context_array = (void **)context;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context_array[0];
	uint32_t *value = (uint32_t *)context_array[1];
	uint32_t reg_val = mi300_psp_get_sos_loaded_status(adapt);

	return !((reg_val != 0 && reg_val != PSP_REGISTER_VALUE_INVALID && reg_val != *value));
}

bool mi300_psp_wait_sos_loaded_status(struct amdgv_adapter *adapt)
{
	uint32_t value;
	void *context_array[2] = { (void *)adapt, 0 }; /* param for amdgv_wait_for */
	int wait_ret;

	/* If PSP TOS is loaded and alive, C2P 81 will be incrementing
	 *  (PSP Sign of Life / increments once per 100ms)
	 */
	value = mi300_psp_get_sos_loaded_status(adapt);
	context_array[1] = (void *)&value;

	oss_msleep(100); /* this sleep is REQUIRED since TOS may just starting */

	wait_ret = amdgv_wait_for(adapt, mi300_psp_wait_sos_loaded_status_cb,
				  (void *)context_array, AMDGV_TIMEOUT(TIMEOUT_PSP_REG), 0);
	if (!wait_ret)
		return true;
	else
		return false;
}

enum psp_status mi300_psp_dump_tracelog(struct amdgv_adapter *adapt, uint64_t buf_bus_addr,
					uint32_t buf_size, uint32_t *buf_used_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *dump_tracelog_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };
	*buf_used_size = 0;

	dump_tracelog_cmd = adapt->psp.psp_cmd_km_mem;
	if (!dump_tracelog_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	dump_tracelog_cmd->cmd_id = PSP_CMD_KM_TYPE__DUMP_TRACELOG;
	dump_tracelog_cmd->cmd.dump_tracelog.addr_lo = lower_32_bits(buf_bus_addr);
	dump_tracelog_cmd->cmd.dump_tracelog.addr_hi = upper_32_bits(buf_bus_addr);
	dump_tracelog_cmd->cmd.dump_tracelog.size = buf_size;

	ret = amdgv_psp_cmd_km_submit(adapt, dump_tracelog_cmd, &psp_resp);

	if (ret == PSP_STATUS__SUCCESS)
		*buf_used_size = psp_resp.info;
	else {
		*buf_used_size = 0;
		if (psp_resp.status == PSP_KM_TEE_ERROR_NOT_SUPPORTED)
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_NOT_SUPPORTED_FEATURE, 0);
		else
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_GET_PSP_TRACELOG_FAIL,
					psp_resp.status);
	}
	AMDGV_INFO("Addr: 0x%08lx%08lx size=0x%x rsp.info=0x%x rsp.st=0x%x\n",
		   dump_tracelog_cmd->cmd.dump_tracelog.addr_hi,
		   dump_tracelog_cmd->cmd.dump_tracelog.addr_lo,
		   dump_tracelog_cmd->cmd.dump_tracelog.size, psp_resp.info, psp_resp.status);

	return ret;
}

enum psp_status mi300_psp_set_snapshot_addr(struct amdgv_adapter *adapt, uint64_t buf_bus_addr,
					    uint32_t buf_size)
{
	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;
	struct psp_cmd_km *snapshot_set_addr_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };

	snapshot_set_addr_cmd = adapt->psp.psp_cmd_km_mem;
	if (!snapshot_set_addr_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));
		return ret;
	}

	snapshot_set_addr_cmd->cmd_id = PSP_CMD_KM_TYPE__DBG_SNAPSHOT_SET_ADDR;
	snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.addr_lo = lower_32_bits(buf_bus_addr);
	snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.addr_hi = upper_32_bits(buf_bus_addr);
	snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.size = buf_size;

	ret = amdgv_psp_cmd_km_submit(adapt, snapshot_set_addr_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS) {
		if (psp_resp.status == PSP_KM_TEE_ERROR_NOT_SUPPORTED) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_NOT_SUPPORTED_FEATURE, 0);
			ret = PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
		} else {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_SET_SNAPSHOT_ADDR_FAIL,
					psp_resp.status);
		}
	}

	AMDGV_INFO("Addr: 0x%08lx%08lx size=0x%x rsp.info=0x%x rsp.st=0x%x\n",
		   snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.addr_hi,
		   snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.addr_lo,
		   snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.size, psp_resp.info,
		   psp_resp.status);

	return ret;
}

enum psp_status mi300_psp_trigger_snapshot(struct amdgv_adapter *adapt, uint32_t target_vfid,
					   uint32_t sections, uint32_t aid_mask,
					   uint32_t xcc_mask, uint32_t *buf_used_size)
{
	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;
	struct psp_cmd_km *snapshot_trigger_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };
	*buf_used_size = 0;

	snapshot_trigger_cmd = adapt->psp.psp_cmd_km_mem;
	if (!snapshot_trigger_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));
		return ret;
	}

	snapshot_trigger_cmd->cmd_id = PSP_CMD_KM_TYPE__DBG_SNAPSHOT_TRIGGER;
	snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.target_vfid = target_vfid;
	snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.sections = sections;
	snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.aid_mask = aid_mask;
	snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.xcc_mask = xcc_mask;

	ret = amdgv_psp_cmd_km_submit(adapt, snapshot_trigger_cmd, &psp_resp);

	if (ret == PSP_STATUS__SUCCESS) {
		*buf_used_size = psp_resp.info;
	} else {
		*buf_used_size = 0;
		if (psp_resp.status == PSP_KM_TEE_ERROR_NOT_SUPPORTED) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_NOT_SUPPORTED_FEATURE, 0);
			ret = PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
		} else {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_SNAPSHOT_TRIGGER_FAIL,
					psp_resp.status);
		}
	}

	AMDGV_INFO("sections: 0x%x target_vfid=0x%x iod_mask=0x%x inst_mask=0x%x rsp.info=0x%x rsp.st=0x%x\n",
			snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.sections,
			snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.target_vfid,
			snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.aid_mask,
			snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.xcc_mask,
			psp_resp.info, psp_resp.status);

	return ret;
}

static int mi300_psp_sw_fini(struct amdgv_adapter *adapt)
{
	int r = 0;

	if (amdgv_psp_sw_fini(adapt) != PSP_STATUS__SUCCESS)
		r = AMDGV_FAILURE;

	if (amdgv_psp_xgmi_mem_fini(adapt) != PSP_STATUS__SUCCESS)
		r = AMDGV_FAILURE;

	return r;
}

static bool mi300_psp_need_switch_to_pf(struct amdgv_adapter *adapt)
{
	return false;
}

static enum psp_status mi300_psp_parse_psp_info(struct amdgv_adapter *adapt)
{
	if ((adapt->psp).attestation_db_gpu_addr != 0)
		return mi300_psp_get_fw_attestation_database_addr(adapt);

	return PSP_STATUS__SUCCESS;
}

static int mi300_psp_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;

	oss_memset(&adapt->psp, 0, sizeof(struct psp_context));

	adapt->psp.tmr_context.size = PSP_TMR_SIZE;
	adapt->psp.tmr_context.alignment = PSP_TMR_ALIGNMENT;
	adapt->psp.fw_id_support = amdgv_psp_fw_id_support;
	adapt->psp.program_register = mi300_psp_program_register;
	adapt->psp.get_wptr = mi300_psp_ring_get_wptr;
	adapt->psp.set_wptr = mi300_psp_ring_set_wptr;
	adapt->psp.set_mb_int = mi300_psp_set_mb_int;
	adapt->psp.get_mb_int_status = mi300_psp_get_mb_int_status;
	adapt->psp.vfgate_support = mi300_psp_vfgate_support;
	adapt->psp.clear_vf_fw = mi300_psp_clear_vf_fw;
	adapt->psp.psp_program_guest_mc_settings = mi300_psp_program_guest_mc_settings;
	adapt->psp.need_switch_to_pf = mi300_psp_need_switch_to_pf;
	/* false on PSP TEE 3.0 */
	adapt->psp.ras_need_switch_to_pf = mi300_psp_need_switch_to_pf;
	adapt->psp.copy_vf_chiplet_regs = mi300_psp_copy_vf_chiplet_regs;
	adapt->psp.dfc_check_guest_version = mi300_psp_dfc_check_guest_version;
	adapt->psp.parse_psp_info = mi300_psp_parse_psp_info;
	adapt->psp.get_fw_attestation_info = mi300_psp_get_fw_attestation_info;
	adapt->psp.fw_attestation_support = mi300_psp_fw_attestation_support;

	psp_ret = amdgv_psp_sw_init(adapt);
	adapt->psp.ras_context.set_init_flag = true;

	if (psp_ret == PSP_STATUS__SUCCESS)
		psp_ret = amdgv_psp_xgmi_mem_init(adapt);

	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_INIT_FAIL, 0);
		mi300_psp_sw_fini(adapt);
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static int mi300_psp_load_psp_fw(struct amdgv_adapter *adapt)
{
	int r = 0;
	enum amdgv_firmware_id ucode_id;

	/* need PSP BL to load KEYDB, PSP drv_sys and PSP sos
	 * Therefore, make sure PSP bootloader is ready
	 *    PSP BL will set bit[31] of C2PMSG_35 to 1 (GFX Mailbox)
	 *    (to indicate boot process complete)
	 */
	if (amdgv_psp_wait_for_register(
		    adapt, SOC15_REG_OFFSET(MP0, adapt->psp.idx, regMP0_SMN_C2PMSG_35),
		    0x80000000, 0x80000000, false) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("TIMEOUT waiting for GFX mailbox to open\n");
		return AMDGV_FAILURE;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_KEYDB;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				amdgv_psp_bl_command_map(ucode_id));
		return r;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_SYS;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				amdgv_psp_bl_command_map(ucode_id));
		return r;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_RAS;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(
			AMDGV_PF_IDX,
			AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
			amdgv_psp_cmd_km_fw_id_map(ucode_id)
		);
		return r;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_SOC;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				amdgv_psp_bl_command_map(ucode_id));
		return r;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_INTF;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				amdgv_psp_bl_command_map(ucode_id));
		return r;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_DBG;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				amdgv_psp_bl_command_map(ucode_id));
		return r;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_SOS;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				amdgv_psp_bl_command_map(ucode_id));
		return r;
	}

	return r;
}

#define PSP_WAIT_BOOTLOADER_RETRY	30

static enum psp_status mi300_psp_wait_for_bootloader(struct amdgv_adapter *adapt)
{
	int retry_loop, ret = 0;

	/* wait for PSP to indicate BL completion */
	for (retry_loop = 0; retry_loop < PSP_WAIT_BOOTLOADER_RETRY; retry_loop++) {
		ret = amdgv_psp_wait_for_register(adapt, reg_mmMP0_SMN_C2PMSG_33,
						  0x80000000, 0xFFFFFFFF, false);
		if (ret == PSP_STATUS__SUCCESS)
			break;
	}

	return ret;
}

enum psp_status mi300_psp_wait_for_bootloader_steady(struct amdgv_adapter *adapt)
{
	int retry_loop, ret = 0;

	ret = mi300_psp_wait_for_bootloader(adapt);
	if (ret)
		goto fail;

	/* wait for BL steady state to accept cmd from driver */
	for (retry_loop = 0; retry_loop < PSP_WAIT_BOOTLOADER_RETRY; retry_loop++) {
		ret = amdgv_psp_wait_for_register(
			adapt, reg_mmMP0_SMN_C2PMSG_35,
			0x80000000, 0xFFFFFFFF, false);
		if (!ret)
			break;
	}

fail:
	if (ret)
		mi300_ras_query_boot_status(adapt, 4);

	return ret;
}

uint32_t mi300_psp_get_bootloader_version(struct amdgv_adapter *adapt)
{
	uint32_t fw_ver = 0;
	struct psp_context *psp = &adapt->psp;

	fw_ver = RREG32(SOC15_REG_OFFSET(MP0, psp->idx, regMP0_SMN_C2PMSG_59));

	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_BL] = fw_ver;

	AMDGV_INFO("PSP BL version (MP0_C2PMSG_59): %X.%X.%X.%X\n",
		   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
		   (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);

	return fw_ver;
}

static int mi300_psp_hw_init(struct amdgv_adapter *adapt)
{
	int r = 0;
	int i;
	int pos;
	uint16_t ctrl;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;
	enum amdgv_firmware_id ucode_np_seq[] = {
		AMDGV_FIRMWARE_ID__P2S_TABLE,
		AMDGV_FIRMWARE_ID__SDMA0,
		AMDGV_FIRMWARE_ID__SDMA1,
		AMDGV_FIRMWARE_ID__SDMA2,
		AMDGV_FIRMWARE_ID__SDMA3,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL,
		AMDGV_FIRMWARE_ID__CP_MEC1,
		AMDGV_FIRMWARE_ID__CP_MEC_JT1,
		AMDGV_FIRMWARE_ID__RLC,
		AMDGV_FIRMWARE_ID__RLC_V,
		AMDGV_FIRMWARE_ID__MMSCH,
		AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST,
		AMDGV_FIRMWARE_ID__DFC_FW,
	};

	adapt->psp.idx = 0;

	if ((adapt->asic_type == CHIP_MI300X) || (adapt->asic_type == CHIP_MI308X)) {
		r = mi300_psp_load_psp_fw(adapt);
		if (r)
			goto init_fail;
	} else {
		if (mi300_psp_wait_sos_loaded_status(adapt) == false) {
			AMDGV_ERROR("AID0: TIMEOUT waiting for PSP tOS sign-of-life\n");
			r = AMDGV_FAILURE;
			goto init_fail;
		}
	}

	mi300_psp_get_bootloader_version(adapt);

	psp_ret = mi300_psp_ring_start(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_START_RING_FAIL, 0);
		return AMDGV_FAILURE;
	}

	psp_ret = amdgv_psp_cmd_km_start(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	/* PSP needs this to know that we are SRIOV */
	psp_ret = mi300_psp_set_num_vfs(adapt, adapt->num_vf);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	psp_ret = mi300_psp_check_memory_partition_mode(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	psp_ret = mi300_psp_set_accelerator_partition_mode(adapt,
		adapt->mcp.accelerator_partition_mode);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	psp_ret = amdgv_psp_tmr_load(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	/* Disable psp mailbox interrupts for all vfs */
	for (i = 0; i < adapt->num_vf; i++)
		mi300_psp_set_mb_int(adapt, i, false);

	r = adapt->ucode.load(adapt, ucode_np_seq, ARRAY_SIZE(ucode_np_seq));
	if (r) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL, 0);
		return r;
	}

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		r = mi300_gfx_wait_rlc_idle(adapt, GET_INST(GC, i));
		if (r)
			goto init_fail;
	}

	psp_ret = mi300_psp_get_fw_attestation_database_addr(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		AMDGV_INFO("Fail Getting FW Attestation Database Addr.");
		goto init_fail;
	}

	// Continue on AID0
	adapt->psp.idx = 0;
	adapt->psp.tee_version =
		(RREG32(SOC15_REG_OFFSET(MP0, adapt->psp.idx, regMP0_SMN_C2PMSG_64)) &
		 GFX_CMD_TEE_VERSION_MASK) >>
		GFX_CMD_TEE_VERSION_SHIFT;
	AMDGV_DEBUG("PSP: tee_version=%x\n", adapt->psp.tee_version);

	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	psp_ret = amdgv_psp_xgmi_initialize(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	/* This information does not change across resets. */
	if (adapt->xgmi.phy_nodes_num > 1 && !adapt->reset.in_xgmi_chain_reset) {
		amdgv_psp_xgmi_get_node_id(adapt);
		amdgv_psp_xgmi_get_hive_id(adapt);
		r = amdgv_xgmi_init_hive(adapt);
		if (r)
			goto init_fail;
		amdgv_xgmi_add_to_hive(adapt);
	}

	/* Check if ATS is enabled before programming register */
	pos = oss_pci_find_ext_cap(adapt->dev, PCIE_EXT_CAP_ID__ATS);
	if (pos) {
		oss_pci_read_config_word(adapt->dev, pos + PCI_ATS_CTRL, &ctrl);
		if ((ctrl & PCI_ATS_CTRL_ENABLE) &&
			mi300_psp_program_register(adapt, 0, 0, 0, VM_IOMMU_CONTROL_WA))
			AMDGV_WARN("Failed to program IOMMUEN via PSP");
	}

init_fail:
	if (r)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_INIT_FAIL, 0);
	return r;
}

static int mi300_psp_hw_fini(struct amdgv_adapter *adapt)
{
	int r = 0;
	enum psp_status ret;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *local_mem = &(psp->tmr_context);
	struct psp_cmd_km tmr_km_cmd = { 0 };
	struct psp_ras_context *ras_context = &psp->ras_context;
	struct amdgv_hive_info *hive;

	if ((adapt->xgmi.phy_nodes_num > 1) &&
	    (!adapt->reset.in_xgmi_chain_reset)) {
		hive = amdgv_get_xgmi_hive(adapt);
		if (!hive) {
			AMDGV_ERROR("XGMI: node 0x%llx, can not match hive "
				    "0x%llx in the hive list.\n",
				    adapt->xgmi.node_id, adapt->xgmi.hive_id);
		}
		amdgv_xgmi_remove_from_hive(adapt);
	}

	if (ras_context->ras_initialized) {
		if (adapt->reset.reset_state)
			/*For mode1 reset, directly set .ras_initialized to false.*/
			ras_context->ras_initialized = false;
		else
			amdgv_psp_ras_terminate(adapt);
	}

	ras_context->ta_version = 0x0;


	if (!oss_atomic_read(adapt->in_ecc_recovery)) {
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

			if (ret != PSP_STATUS__SUCCESS) {
				r = AMDGV_FAILURE;
				AMDGV_ERROR("PSP: Failed to destroy TMR.\n");
			}
		}

		if (mi300_psp_ring_destroy(adapt)) {
			r = AMDGV_FAILURE;
			AMDGV_ERROR("PSP: Failed to DESTROY_RINGS.\n");
		}
	}
	return r;
}

const struct amdgv_init_func mi300_psp_func = {
	.name = "mi300_psp_func",
	.sw_init = mi300_psp_sw_init,
	.sw_fini = mi300_psp_sw_fini,
	.hw_init = mi300_psp_hw_init,
	.hw_fini = mi300_psp_hw_fini,
};
