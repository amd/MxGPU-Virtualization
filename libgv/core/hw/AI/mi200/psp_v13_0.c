/*
 * Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include <amdgv.h>
#include <amdgv_device.h>

#include "mi200.h"
#include "mi200_gpuiov.h"
#include "mi200_powerplay.h"
#include "psp_v13_0.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;
enum { PSP_TMR_SIZE = 0x8000000 }; /* 128M */

int psp_v13_ring_destroy(struct amdgv_adapter *adapt)
{
	uint32_t flag = 0x80000000;
	uint32_t mask = 0x8000FFFF;
	int wait_ret;

	/* Write the ring destroy command to C2PMSG_64 */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64), GFX_CTRL_CMD_ID_DESTROY_RINGS);

	/* Wait for response flag (bit 31) in C2PMSG_64 */
	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64),
					   mask, flag, AMDGV_TIMEOUT(TIMEOUT_PSP_REG),
					   AMDGV_WAIT_CHECK_EQ, 0);

	if (wait_ret)
		return AMDGV_FAILURE;
	else
		return 0;
}

enum psp_status psp_v13_ring_start(struct amdgv_adapter *adapt)
{
	enum psp_status            ret            = PSP_STATUS__SUCCESS;
	struct psp_ring           *ring;
	struct psp_local_memory   *local_mem      = NULL;
	uint32_t                   psp_ring_reg   = 0; //MP0_SMN_C2PMSG_64 - 71
	struct psp_context        *psp            = &adapt->psp;

	ring = &psp->km_ring[psp->idx];

	local_mem = &ring->ring_mem;

	/* Write low address of the ring to C2PMSG_69 */
	psp_ring_reg = lower_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_69), psp_ring_reg);

	/* Write high address of the ring to C2PMSG_70 */
	psp_ring_reg = upper_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_70), psp_ring_reg);

	/* Write size of ring to C2PMSG_71 */
	psp_ring_reg = local_mem->size;
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_71), psp_ring_reg);

	/* Write the ring initialization command to C2PMSG_64 */
	psp_ring_reg = ring->ring_type;
	psp_ring_reg = psp_ring_reg << 16;
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64), psp_ring_reg);

	/* Wait for response flag (bit 31) in C2PMSG_64 */
	ret = amdgv_psp_wait_for_register(
	    adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64), 0x80000000,
	    0x8000FFFF, false);

	if (ret != PSP_STATUS__SUCCESS)
		AMDGV_ERROR("PSP: Failed to start ring.\n");

	return ret;
}

enum psp_status psp_v13_load_key_db(struct amdgv_adapter *adapt,
		unsigned char *fw_image, uint32_t fw_image_size)
{
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_key_db_load_mem = psp->private_fw_memory;
	uint32_t fw_ver;

	if (!psp_key_db_load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy PSP Key DB binary to memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_key_db_load_mem.mem),
			fw_image, fw_image_size);

	/* Provide the Key DB to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(
		   amdgv_memmgr_get_gpu_addr(psp_key_db_load_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
	       PSP_BL__LOAD_KEY_DATABASE);

	/* wait for C2P[35] != PSP_BL__LOAD_KEY_DATABASE */
	if (amdgv_psp_wait_for_register(
		adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
		0x80000000, 0x80000000, false) != PSP_STATUS__SUCCESS) {
		return PSP_STATUS__ERROR_GENERIC;
	}

	fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

	AMDGV_INFO("PSP: Key DB(version:%X.%X.%X.%X) is loaded.\n",
		   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
		   (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);

	return PSP_STATUS__SUCCESS;
}

enum psp_status psp_v13_load_sysdrv(struct amdgv_adapter *adapt,
				unsigned char *fw_image, uint32_t fw_image_size)
{
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_sysdrv_load_mem = psp->private_fw_memory;
	uint32_t fw_ver;

	if (!psp_sysdrv_load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy PSP System Driver binary to memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_sysdrv_load_mem.mem),
			fw_image, fw_image_size);

	/* Provide the sys driver to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(
		   amdgv_memmgr_get_gpu_addr(psp_sysdrv_load_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
	       PSP_BL__LOAD_SYSDRV);

	/* wait for C2P[35] != PSP_BL__LOAD_SYSDRV */
	if (amdgv_psp_wait_for_register(
		adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
		0x80000000, 0x80000000, false) != PSP_STATUS__SUCCESS) {
		return PSP_STATUS__ERROR_GENERIC;
	}

	fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

	AMDGV_INFO("PSP: SYS(version:%X.%X.%X.%X) is loaded.\n",
		   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
		   (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SYS] = fw_ver;

	return PSP_STATUS__SUCCESS;
}

enum psp_status psp_v13_load_sos(struct amdgv_adapter *adapt,
				unsigned char *fw_image, uint32_t fw_image_size)
{
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_sos_load_mem = psp->private_fw_memory;
	uint32_t fw_ver;

	if (!psp_sos_load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy Secure OS binary to PSP memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_sos_load_mem.mem),
			fw_image, fw_image_size);

	/* Provide the sos driver to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(amdgv_memmgr_get_gpu_addr(psp_sos_load_mem.mem) >>
			     20));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
	       PSP_BL__LOAD_SOSDRV);

	/* Check sOS sign of life register to confirm that
	 * PSP drv_sys and sOS are loaded, alive and ready to respond
	 * to GFX mailbox (commands from host and VMs)
	 */
	if (psp_v13_wait_sos_loaded_status(adapt) == false) {
		AMDGV_ERROR("TIMEOUT waiting for PSP tOS sign-of-life\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

	AMDGV_INFO("PSP: OS(version:%X.%X.%X.%X) is loaded.\n",
		   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
		   (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SOS] = fw_ver;

	return PSP_STATUS__SUCCESS;
}

static enum psp_status psp_v13_program_register(struct amdgv_adapter *adapt,
		uint32_t idx_vf, uint32_t reg_value, uint32_t reg_value_hi,
		enum psp_ih_reg reg_id)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *program_reg_cmd = adapt->psp.psp_cmd_km_mem;

	if (!program_reg_cmd) {
		amdgv_put_error(
			AMDGV_PF_IDX,
			AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct psp_cmd_km)
		);

		return PSP_STATUS__ERROR_GENERIC;
	}

	AMDGV_DEBUG("Program Register 0x%x value 0x%x\n", reg_id, reg_value);

	program_reg_cmd->cmd_id = PSP_CMD_KM_TYPE__GBR_IH_REG;
	program_reg_cmd->cmd.program_reg.reg_value = reg_value;
	program_reg_cmd->cmd.program_reg.reg_value_hi = reg_value_hi;
	program_reg_cmd->cmd.program_reg.reg_id = reg_id;

	WREG32(SOC15_REG_OFFSET(GC, 0, mmSCRATCH_REG0), idx_vf);

	ret = amdgv_psp_cmd_km_submit(adapt, program_reg_cmd, NULL);

	if (ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("PSP: Failed to Write Register %d\n", reg_id);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	/* Clear system memory used for Program Reg CMD */
	oss_memset(program_reg_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

enum psp_status psp_v13_program_guest_mc_settings(struct amdgv_adapter *adapt,
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
			 MBYTES_TO_BYTES(vf->real_fb_size *
					 adapt->xgmi.phy_nodes_num) - 1;
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
		    fb_base +
		    MBYTES_TO_BYTES(vf->real_fb_size * adapt->xgmi.phy_node_id);
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

	if (psp_v13_program_register(adapt, idx_vf,
			fb_location_base, 0, GC_MC_VM_FB_LOCATION_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v13_program_register(adapt, idx_vf,
			fb_location_base, 0, MM_MC_VM_FB_LOCATION_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v13_program_register(adapt, idx_vf,
			fb_location_top, 0, GC_MC_VM_FB_LOCATION_TOP))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v13_program_register(adapt, idx_vf,
			fb_location_top, 0, MM_MC_VM_FB_LOCATION_TOP))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v13_program_register(adapt, idx_vf,
			sys_aper_lo, 0, GC_MC_SYSTEM_APERTURE_LO))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v13_program_register(adapt, idx_vf,
			sys_aper_lo, 0, MM_MC_SYSTEM_APERTURE_LO))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v13_program_register(adapt, idx_vf,
			sys_aper_hi, 0, GC_MC_SYSTEM_APERTURE_HI))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v13_program_register(adapt, idx_vf,
			sys_aper_hi, 0, MM_MC_SYSTEM_APERTURE_HI))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v13_program_register(adapt, idx_vf,
			nonsurface_mc_lo, 0, HDP_NONSURFACE_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (psp_v13_program_register(adapt, idx_vf,
			nonsurface_mc_hi, 0, HDP_NONSURFACE_BASE_HI))
		return PSP_STATUS__ERROR_GENERIC;

	return PSP_STATUS__SUCCESS;
}

static enum psp_status psp_v13_vfgate_support(struct amdgv_adapter *adapt)
{
	if (adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SYS] < 0x00180043 ||
	    adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SOS] < 0x00180043) {
		return PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
	}

	return PSP_STATUS__SUCCESS;
}

static enum psp_status psp_v13_set_mb_int(struct amdgv_adapter *adapt, uint32_t idx_vf,
						 bool enable)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *vfgate_cmd = adapt->psp.psp_cmd_km_mem;
	bool resp_enable = false;
	struct psp_gfx_resp psp_resp = { 0 };

	ret = psp_v13_vfgate_support(adapt);

	if (ret == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE ||
	    adapt->flags & AMDGV_FLAG_DISABLE_PSP_VF_GATE)
		return ret;

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
			AMDGV_INFO("psp mailbox %s for VF%d\n",
				   enable ? "enabled" : "disabled", idx_vf);
		else {
			AMDGV_INFO("psp mailbox failed to %s for VF%d\n",
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

static enum psp_status psp_v13_get_mb_int_status(struct amdgv_adapter *adapt,
							uint32_t idx_vf, struct psp_mb_status *mb_status)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *vfgate_cmd = adapt->psp.psp_cmd_km_mem;
	struct psp_gfx_resp psp_resp = { 0 };
	uint32_t gfx_fw_type = 0;
	enum amdgv_firmware_id psp_fw_id = (enum amdgv_firmware_id)AMDGV_FIRMWARE_ID__MAX;

	ret = psp_v13_vfgate_support(adapt);

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

static enum psp_status psp_v13_clear_vf_fw(struct amdgv_adapter *adapt,
					uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__CLEAR_VF_FW;
	psp_cmd.cmd.clear_vf_fw.target_vf = idx_vf;

	psp_resp.status = 0xdeadbeef;
	ret = amdgv_psp_cmd_km_submit(adapt, &psp_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS || psp_resp.status != 0) {
		  AMDGV_INFO("PSP: failed to submit GFX_CMD_ID_CLEAR_VF_FW "\
				  "(gfx_cmd_resp=0x%08x)\n", psp_resp.status);
		  ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

static uint32_t psp_v13_ring_get_wptr(struct amdgv_adapter *adapt)
{
	return RREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_67));
}

static void psp_v13_ring_set_wptr(struct amdgv_adapter *adapt, uint32_t value)
{
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_67), value);
}

static enum psp_status psp_v13_set_num_vfs(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km psp_cmd = { 0 };
	struct psp_gfx_resp psp_resp = { 0 };

	psp_cmd.cmd_id = PSP_CMD_KM_TYPE__NUM_ENABLED_VFS;
	psp_cmd.cmd.num_vfs.number_of_vfs = adapt->max_num_vf;

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

static int psp_v13_wait_sos_loaded_status_cb(void *context)
{
	void **context_array = (void **)context;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context_array[0];
	uint32_t *value = (uint32_t *)context_array[1];
	uint32_t reg_val = RREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_81));

	return !((reg_val != 0 && reg_val != PSP_REGISTER_VALUE_INVALID && reg_val != *value));
}

bool psp_v13_wait_sos_loaded_status(struct amdgv_adapter *adapt)
{
	uint32_t value;
	uint32_t reg_index = SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_81);
	void *context_array[2] = {(void *)adapt, 0};	/* param for amdgv_wait_for */
	int wait_ret;

	/* If PSP TOS is loaded and alive, C2P 81 will be incrementing
	 *  (PSP Sign of Life / increments once per 100ms)
	 */
	value = RREG32(reg_index);
	context_array[1] = (void *)&value;

	oss_msleep(100); /* this sleep is REQUIRED since TOS may just starting */

	wait_ret = amdgv_wait_for(adapt, psp_v13_wait_sos_loaded_status_cb,
				  (void *)context_array, AMDGV_TIMEOUT(TIMEOUT_PSP_REG), 0);
	if (!wait_ret)
		return true;
	else
		return false;
}

enum psp_status psp_v13_dump_tracelog(struct amdgv_adapter *adapt,
	uint64_t buf_bus_addr, uint32_t buf_size, uint32_t *buf_used_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *dump_tracelog_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };
	*buf_used_size = 0;

	dump_tracelog_cmd = adapt->psp.psp_cmd_km_mem;
	if (!dump_tracelog_cmd) {
		amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	dump_tracelog_cmd->cmd_id = PSP_CMD_KM_TYPE__DUMP_TRACELOG;
	dump_tracelog_cmd->cmd.dump_tracelog.addr_lo =
						lower_32_bits(buf_bus_addr);
	dump_tracelog_cmd->cmd.dump_tracelog.addr_hi =
						upper_32_bits(buf_bus_addr);
	dump_tracelog_cmd->cmd.dump_tracelog.size = buf_size;

	ret = amdgv_psp_cmd_km_submit(adapt, dump_tracelog_cmd, &psp_resp);

	if (ret == PSP_STATUS__SUCCESS)
		*buf_used_size = psp_resp.info;
	else {
		*buf_used_size = 0;
		if (psp_resp.status == PSP_KM_TEE_ERROR_NOT_SUPPORTED)
			amdgv_put_error(AMDGV_PF_IDX,
					AMDGV_ERROR_FW_NOT_SUPPORTED_FEATURE, 0);
		else
			amdgv_put_error(AMDGV_PF_IDX,
					AMDGV_ERROR_FW_GET_PSP_TRACELOG_FAIL,
					psp_resp.status);
	}
	AMDGV_INFO("Addr: 0x%08lx%08lx size=0x%x rsp.info=0x%x rsp.st=0x%x\n",
		       dump_tracelog_cmd->cmd.dump_tracelog.addr_hi,
		       dump_tracelog_cmd->cmd.dump_tracelog.addr_lo,
		       dump_tracelog_cmd->cmd.dump_tracelog.size,
		       psp_resp.info,
		       psp_resp.status);

	return ret;
}


enum psp_status psp_v13_set_snapshot_addr(struct amdgv_adapter *adapt,
	uint64_t buf_bus_addr, uint32_t buf_size)
{

	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;
	struct psp_cmd_km *snapshot_set_addr_cmd = NULL;
	struct psp_gfx_resp psp_resp = {0};

	snapshot_set_addr_cmd = adapt->psp.psp_cmd_km_mem;
	if (!snapshot_set_addr_cmd) {
		amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));
		return ret;
	}

	snapshot_set_addr_cmd->cmd_id = PSP_CMD_KM_TYPE__DBG_SNAPSHOT_SET_ADDR;
	snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.addr_lo =
		lower_32_bits(buf_bus_addr);
	snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.addr_hi =
		upper_32_bits(buf_bus_addr);
	snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.size = buf_size;

	ret = amdgv_psp_cmd_km_submit(adapt, snapshot_set_addr_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS) {
		if (psp_resp.status == PSP_KM_TEE_ERROR_NOT_SUPPORTED) {
			amdgv_put_error(AMDGV_PF_IDX,
					AMDGV_ERROR_FW_NOT_SUPPORTED_FEATURE, 0);
			ret = PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
		} else {
			amdgv_put_error(AMDGV_PF_IDX,
					AMDGV_ERROR_FW_SET_SNAPSHOT_ADDR_FAIL,
					psp_resp.status);
		}
	}

	AMDGV_INFO("Addr: 0x%08lx%08lx size=0x%x rsp.info=0x%x rsp.st=0x%x\n",
			   snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.addr_hi,
			   snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.addr_lo,
			   snapshot_set_addr_cmd->cmd.dbg_snapshot_setaddr.size,
			   psp_resp.info,
			   psp_resp.status);

	return ret;
}


enum psp_status psp_v13_trigger_snapshot(struct amdgv_adapter *adapt,
	uint32_t target_vfid, uint32_t sections, uint32_t *buf_used_size)
{

	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;
	struct psp_cmd_km *snapshot_trigger_cmd = NULL;
	struct psp_gfx_resp psp_resp = {0};
	*buf_used_size = 0;

	snapshot_trigger_cmd = adapt->psp.psp_cmd_km_mem;
	if (!snapshot_trigger_cmd) {
		amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));
		return ret;
	}

	snapshot_trigger_cmd->cmd_id = PSP_CMD_KM_TYPE__DBG_SNAPSHOT_TRIGGER;
	snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.target_vfid = target_vfid;
	snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.sections = sections;

	ret = amdgv_psp_cmd_km_submit(adapt, snapshot_trigger_cmd, &psp_resp);

	if (ret == PSP_STATUS__SUCCESS) {
		*buf_used_size = psp_resp.info;
	} else {
		*buf_used_size = 0;
		if (psp_resp.status == PSP_KM_TEE_ERROR_NOT_SUPPORTED) {
			amdgv_put_error(AMDGV_PF_IDX,
					AMDGV_ERROR_FW_NOT_SUPPORTED_FEATURE, 0);
			ret = PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
		} else {
			amdgv_put_error(AMDGV_PF_IDX,
					AMDGV_ERROR_FW_SNAPSHOT_TRIGGER_FAIL,
					psp_resp.status);
		}
	}

	AMDGV_INFO("sections: 0x%x target_vfid=0x%x rsp.info=0x%x rsp.st=0x%x\n",
			   snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.sections,
			   snapshot_trigger_cmd->cmd.dbg_snapshot_trigger.target_vfid,
			   psp_resp.info,
			   psp_resp.status);

	return ret;
}

static enum psp_status psp_v13_get_migration_version(struct amdgv_adapter *adapt,
	uint32_t *migration_version)
{

	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;
	struct psp_cmd_km* migration_get_psp_info_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };

	migration_get_psp_info_cmd = adapt->psp.psp_cmd_km_mem;
	if (!migration_get_psp_info_cmd) {
		amdgv_put_error(AMDGV_PF_IDX,
			AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct psp_cmd_km));
		return ret;
	}
	migration_get_psp_info_cmd->cmd_id = PSP_CMD_KM_TYPE__MIGRATION_GET_PSP_INFO;
	migration_get_psp_info_cmd->cmd.migration_get_psp_info.migration_version = 0;

	ret = amdgv_psp_cmd_km_submit(adapt, migration_get_psp_info_cmd, &psp_resp);
	if (ret != PSP_STATUS__SUCCESS) {
		if (psp_resp.status == PSP_KM_TEE_ERROR_NOT_SUPPORTED) {
			amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_FW_NOT_SUPPORTED_FEATURE, 0);
			ret = PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
		}
		else {
			amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_FW_MIGRATION_GET_PSP_INFO_FAIL,
				psp_resp.status);
		}
	}

	*migration_version =
		psp_resp.uresp.migration_info.migration_version;
	AMDGV_DEBUG("Live Migration Version: 0x%x\n", *migration_version);

	return ret;
}

static enum psp_status psp_v13_migration_get_psp_info(struct amdgv_adapter *adapt)
{

	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;

	ret = psp_v13_get_migration_version(adapt, &adapt->live_migration.migration_version);
	if (ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("Failed to get PSP migration version.\n");
		return ret;
	}

	adapt->live_migration.static_data_size = MI200_MIGRATION_PSP_STATIC_DATA_SIZE;
	adapt->live_migration.dynamic_data_size = MI200_MIGRATION_PSP_DYNAMIC_DATA_SIZE;

	return ret;
}

static int psp_v13_migration_cmd_init(struct psp_cmd_km *migration_cmd,
	uint32_t vfid, uint64_t data_addr, uint32_t size,
	enum psp_migration_manifest_data_type type)
{
	switch (type) {
	case PSP_MIGRATION_EXPORT_STATIC_DATA:
		migration_cmd->cmd_id = PSP_CMD_KM_TYPE__MIGRATION_EXPORT;
		migration_cmd->cmd.migration_export.pkg_addr_lo =
			lower_32_bits(data_addr);
		migration_cmd->cmd.migration_export.pkg_addr_hi =
			upper_32_bits(data_addr);
		migration_cmd->cmd.migration_export.pkg_size_allocated = size;
		migration_cmd->cmd.migration_export.target_vfid = vfid;
		migration_cmd->cmd.migration_export.flags = MIGRATION_FLAG_STATIC;
		break;
	case PSP_MIGRATION_EXPORT_DYNAMIC_DATA:
		migration_cmd->cmd_id = PSP_CMD_KM_TYPE__MIGRATION_EXPORT;
		migration_cmd->cmd.migration_export.pkg_addr_lo =
			lower_32_bits(data_addr);
		migration_cmd->cmd.migration_export.pkg_addr_hi =
			upper_32_bits(data_addr);
		migration_cmd->cmd.migration_export.pkg_size_allocated = size;
		migration_cmd->cmd.migration_export.target_vfid = vfid;
		migration_cmd->cmd.migration_export.flags = MIGRATION_FLAG_DYNAMIC;
		break;
	case PSP_MIGRATION_IMPORT_DYNAMIC_DATA:
		migration_cmd->cmd_id = PSP_CMD_KM_TYPE__MIGRATION_IMPORT;
		migration_cmd->cmd.migration_import.pkg_addr_lo =
			lower_32_bits(data_addr);
		migration_cmd->cmd.migration_import.pkg_addr_hi =
			upper_32_bits(data_addr);
		migration_cmd->cmd.migration_import.pkg_size = size;
		migration_cmd->cmd.migration_import.target_vfid = vfid;
		break;
	case PSP_MIGRATION_IMPORT_STATIC_DATA:
		migration_cmd->cmd_id = PSP_CMD_KM_TYPE__MIGRATION_IMPORT;
		migration_cmd->cmd.migration_import.pkg_addr_lo =
			lower_32_bits(data_addr);
		migration_cmd->cmd.migration_import.pkg_addr_hi =
			upper_32_bits(data_addr);
		migration_cmd->cmd.migration_import.pkg_size = size;
		migration_cmd->cmd.migration_import.target_vfid = vfid;
		break;
	default:
		//AMDGV_ERROR("Invalid migration data type: %d\n", type);
		return -1;
	}

	return 0;
}

static enum psp_status psp_v13_transfer_manifest_data(struct amdgv_adapter *adapt,
	uint32_t idx_vf, uint64_t data_addr, uint32_t size,
	enum psp_migration_manifest_data_type type)
{
	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;
	struct psp_cmd_km *migration_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };

	migration_cmd = adapt->psp.psp_cmd_km_mem;
	if (!migration_cmd) {
		amdgv_put_error(AMDGV_PF_IDX,
		AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
		sizeof(struct psp_cmd_km));
		return ret;
	}


	if (psp_v13_migration_cmd_init(migration_cmd, idx_vf, data_addr, size, type)) {
		amdgv_put_error(AMDGV_PF_IDX,
		AMDGV_ERROR_FW_MIGRATION_EXPORT_FAIL,
		psp_resp.status);
		return ret;
	}

	ret = amdgv_psp_cmd_km_submit(adapt, migration_cmd, &psp_resp);

	if (type == PSP_MIGRATION_EXPORT_STATIC_DATA ||
		type == PSP_MIGRATION_EXPORT_DYNAMIC_DATA)
		AMDGV_INFO("Package addr: 0x%08lx%08lx target_vfid=0x%x size=0x%x flags=0x%x rsp.info=0x%x"
			" rsp.st=0x%x\n",
			migration_cmd->cmd.migration_export.pkg_addr_hi,
			migration_cmd->cmd.migration_export.pkg_addr_lo,
			migration_cmd->cmd.migration_export.target_vfid,
			migration_cmd->cmd.migration_export.pkg_size_allocated,
			migration_cmd->cmd.migration_export.flags,
			psp_resp.info,
			psp_resp.status);

	if (type == PSP_MIGRATION_IMPORT_DYNAMIC_DATA ||
		type == PSP_MIGRATION_IMPORT_STATIC_DATA)
		AMDGV_INFO("Package addr: 0x%08lx%08lx target_vfid=0x%x size=0x%x rsp.info=0x%x"
			" rsp.st=0x%x\n",
			migration_cmd->cmd.migration_import.pkg_addr_hi,
			migration_cmd->cmd.migration_import.pkg_addr_lo,
			migration_cmd->cmd.migration_import.target_vfid,
			migration_cmd->cmd.migration_import.pkg_size,
			psp_resp.info,
			psp_resp.status);

	if (ret)
		return ret;

	if ((type == PSP_MIGRATION_EXPORT_STATIC_DATA) ||
		(type == PSP_MIGRATION_EXPORT_DYNAMIC_DATA)) {
		uint32_t pkg_size = psp_resp.uresp.migration_export.size_written;

		if (pkg_size <= 0 || pkg_size > migration_cmd->cmd.migration_export.pkg_size_allocated) {
			AMDGV_ERROR("PSP static export data is empty or oversized.\n");
			ret = AMDGV_FAILURE;
		}
	}
	return ret;
}

static int psp_v13_sw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;

	if (amdgv_psp_sw_fini(adapt) != PSP_STATUS__SUCCESS)
		ret = AMDGV_FAILURE;

	return ret;
}

static int psp_v13_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;

	oss_memset(&adapt->psp, 0, sizeof(struct psp_context));

	adapt->psp.allocated_tmr_size = PSP_TMR_SIZE;
	adapt->psp.tmr_context.size = PSP_TMR_SIZE;
	adapt->psp.tmr_context.alignment = PSP_TMR_ALIGNMENT;
	adapt->psp.fw_id_support = amdgv_psp_fw_id_support;
	adapt->psp.program_register = psp_v13_program_register;
	adapt->psp.get_wptr = psp_v13_ring_get_wptr;
	adapt->psp.set_wptr = psp_v13_ring_set_wptr;
	adapt->psp.load_keydb = psp_v13_load_key_db;
	adapt->psp.load_sysdrv = psp_v13_load_sysdrv;
	adapt->psp.load_sosdrv = psp_v13_load_sos;
	adapt->psp.set_mb_int = psp_v13_set_mb_int;
	adapt->psp.get_mb_int_status = psp_v13_get_mb_int_status;
	adapt->psp.vfgate_support = psp_v13_vfgate_support;
	adapt->psp.clear_vf_fw = psp_v13_clear_vf_fw;
	adapt->psp.psp_program_guest_mc_settings =
	    psp_v13_program_guest_mc_settings;
	adapt->psp.transfer_manifest_data = psp_v13_transfer_manifest_data;
	adapt->psp.get_migration_info = psp_v13_migration_get_psp_info;
	psp_ret = amdgv_psp_sw_init(adapt);
	adapt->psp.ras_context.set_init_flag = true;

	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_INIT_FAIL, 0);
		psp_v13_sw_fini(adapt);
		ret = AMDGV_FAILURE;
	}

	return ret;
}

static int psp_v13_hw_init(struct amdgv_adapter *adapt)
{
	int r = 0;
	uint32_t i;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;
	enum amdgv_firmware_id ucode_id;
	enum amdgv_firmware_id ucode_np_seq[] = {
		AMDGV_FIRMWARE_ID__SDMA0,
		AMDGV_FIRMWARE_ID__SDMA1,
		AMDGV_FIRMWARE_ID__SDMA2,
		AMDGV_FIRMWARE_ID__SDMA3,
		AMDGV_FIRMWARE_ID__SDMA4,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL,
		AMDGV_FIRMWARE_ID__RLC,
		AMDGV_FIRMWARE_ID__RLC_V,
		AMDGV_FIRMWARE_ID__MMSCH,
		AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST,
		AMDGV_FIRMWARE_ID__DFC_FW,
		AMDGV_FIRMWARE_ID__CP_MEC1,
		AMDGV_FIRMWARE_ID__CP_MEC_JT1,
	};

	/* need PSP BL to load KEYDB, PSP drv_sys and PSP sos
	 * Therefore, make sure PSP bootloader is ready
	 *    PSP BL will set bit[31] of C2PMSG_35 to 1 (GFX Mailbox)
	 *    (to indicate boot process complete)
	 */
	if (amdgv_psp_wait_for_register(
		adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
		0x80000000, 0x80000000, false) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("TIMEOUT waiting for GFX mailbox to open\n");
		return AMDGV_FAILURE;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_KEYDB;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(
			AMDGV_PF_IDX,
			AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
			amdgv_psp_cmd_km_fw_id_map(ucode_id)
		);
		return r;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_SYS;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(
			AMDGV_PF_IDX,
			AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
			amdgv_psp_cmd_km_fw_id_map(ucode_id)
		);
		return r;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_SOS;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(
			AMDGV_PF_IDX,
			AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
			amdgv_psp_cmd_km_fw_id_map(ucode_id)
		);
		return r;
	}

	psp_ret = psp_v13_ring_start(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(
			AMDGV_PF_IDX,
			AMDGV_ERROR_FW_START_RING_FAIL,
			0
		);
		return AMDGV_FAILURE;
	}

	psp_ret = amdgv_psp_cmd_km_start(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	ucode_id = AMDGV_FIRMWARE_ID__SMU;
	r = adapt->ucode.load(adapt, &ucode_id, 1);
	if (r) {
		amdgv_put_error(
			AMDGV_PF_IDX,
			AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
			amdgv_psp_cmd_km_fw_id_map(ucode_id)
		);
		return r;
	}
	/* wait for SMU to report ready */
	if (!mi200_smu_13_0_get_fw_loaded_status(adapt)) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				(ucode_id));
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	/* before tmr_load, tell PSP num_vf for TMR_SIZE calculation */
	psp_ret = psp_v13_set_num_vfs(adapt);
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
		psp_v13_set_mb_int(adapt, i, false);

	r = adapt->ucode.load(adapt, ucode_np_seq, ARRAY_SIZE(ucode_np_seq));
	if (r) {
		amdgv_put_error(
			AMDGV_PF_IDX,
			AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
			0
		);
		return r;
	}

	psp_ret = amdgv_psp_xgmi_mem_init(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	psp_ret = amdgv_psp_xgmi_initialize(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		r = AMDGV_FAILURE;
		goto init_fail;
	}

	if (adapt->xgmi.phy_nodes_num > 1 && !adapt->reset.in_xgmi_chain_reset) {
		amdgv_psp_xgmi_get_node_id(adapt);
		amdgv_psp_xgmi_get_hive_id(adapt);
		r = amdgv_xgmi_init_hive(adapt);
		if (r)
			goto init_fail;
		amdgv_xgmi_add_to_hive(adapt);
	}

	adapt->psp.xgmi_context.supports_extended_data =
		!adapt->xgmi.connected_to_cpu;

init_fail:
	if (r)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_INIT_FAIL, 0);
	return r;
}

static int psp_v13_hw_fini(struct amdgv_adapter *adapt)
{
	int r = 0;
	enum psp_status ret;
	struct psp_context      *psp = &adapt->psp;
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

	if (ras_context->ras_initialized && !oss_atomic_read(adapt->in_ecc_recovery))
		amdgv_psp_ras_terminate(adapt);

	amdgv_psp_xgmi_mem_fini(adapt);

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

		if (psp_v13_ring_destroy(adapt)) {
			r = AMDGV_FAILURE;
			AMDGV_ERROR("PSP: Failed to DESTROY_RINGS.\n");
		}
	}

	return r;
}

const struct amdgv_init_func mi200_psp_func = {
	.name = "mi200_psp_func",
	.sw_init = psp_v13_sw_init,
	.sw_fini = psp_v13_sw_fini,
	.hw_init = psp_v13_hw_init,
	.hw_fini = psp_v13_hw_fini,
};
