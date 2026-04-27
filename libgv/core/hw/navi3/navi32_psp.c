/*
 * Copyright (C) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include <navi3/MP/mp_13_0_0_offset.h>
#include <navi3/MP/mp_13_0_0_sh_mask.h>
#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include <navi3/MMHUB/mmhub_3_0_0_offset.h>
#include <navi3/MMHUB/mmhub_3_0_0_sh_mask.h>
#include <navi3/HDP/hdp_6_0_0_offset.h>
#include <navi3/HDP/hdp_6_0_0_sh_mask.h>
#include <navi3/NBIO/nbio_4_3_0_offset.h>

#include "navi32_gpuiov.h"
#include "navi32_psp.h"
#include "navi32_powerplay.h"
#include "navi32_ucode.h"
#include "amdgv_sched_internal.h"

#include "../common/ucode/psp_asd_bin.h"
#include "../common/ucode/psp_asd_tee3_bin.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

enum { PSP_START_OFFSET = 0x800000 }; /* 8MB reserved for MP0/MP1 */
/* need to confirm */

enum psp_status navi32_psp_ring_start(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_ring *ring;
	struct psp_local_memory *local_mem = NULL;
	uint32_t psp_ring_reg = 0; // MP0_SMN_C2PMSG_64 - 71
	struct psp_context *psp = &adapt->psp;

	ring = &psp->km_ring[psp->idx];

	local_mem = &ring->ring_mem;
	if (!local_mem->mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

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
	ret = amdgv_psp_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64),
					  0x80000000, 0x8000FFFF, false, AMDGV_WAIT_FLAG_FORCE_YIELD);

	psp->tee_version = (RREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64)) & GFX_CMD_TEE_VERSION_MASK) >> GFX_CMD_TEE_VERSION_SHIFT;

	if (ret != PSP_STATUS__SUCCESS)
		AMDGV_ERROR("PSP: Failed to start ring.\n");

	return ret;
}

uint32_t navi32_psp_ring_get_wptr(struct amdgv_adapter *adapt)
{
	return RREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_67));
}

void navi32_psp_ring_set_wptr(struct amdgv_adapter *adapt, uint32_t value)
{
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_67), value);
}

enum psp_status navi32_psp_program_guest_mc_settings(struct amdgv_adapter *adapt,
						    uint32_t idx_vf)
{
	struct amdgv_vf_device *vf;
	uint64_t fb_base, fb_top;
	uint32_t fb_location_base, fb_location_top;
	uint32_t sys_aper_lo, sys_aper_hi;
	uint64_t hdp_nonsurface;
	uint32_t hdp_nonsurface_lo, hdp_nonsurface_hi;

	vf = &adapt->array_vf[idx_vf];

	/* copy pf fb base to vf*/
	fb_base = adapt->mc_fb_loc_addr;
	if (idx_vf == AMDGV_PF_IDX)
		fb_top = fb_base + adapt->fb_size - 1;
	else if (adapt->ffbm.share_tmr)
		fb_top = fb_base + MBYTES_TO_BYTES(vf->fb_size_tmr) - 1;
	else
		fb_top = fb_base + MBYTES_TO_BYTES(vf->fb_size) - 1;

	fb_location_base = (uint32_t)(fb_base >> 24);
	fb_location_top = (uint32_t)(fb_top >> 24);

	sys_aper_lo = (uint32_t)(fb_base >> 18);
	sys_aper_hi = (uint32_t)(fb_top >> 18);

	hdp_nonsurface = (uint64_t)(fb_base >> 8);
	hdp_nonsurface_lo = lower_32_bits(hdp_nonsurface);
	hdp_nonsurface_hi = upper_32_bits(hdp_nonsurface);

	AMDGV_DEBUG("fb_location_base  = 0x%08x\n", fb_location_base);
	AMDGV_DEBUG("fb_location_top   = 0x%08x\n", fb_location_top);
	AMDGV_DEBUG("sys_aper_lo       = 0x%08x\n", sys_aper_lo);
	AMDGV_DEBUG("sys_aper_hi       = 0x%08x\n", sys_aper_hi);
	AMDGV_DEBUG("hdp_nonsurface_lo = 0x%08x\n", hdp_nonsurface_lo);
	AMDGV_DEBUG("hdp_nonsurface_hi = 0x%08x\n", hdp_nonsurface_hi);

	if (idx_vf == AMDGV_PF_IDX) {
		WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_FB_LOCATION_BASE), fb_location_base);
		WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_FB_LOCATION_BASE), fb_location_base);

		WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_FB_LOCATION_TOP), fb_location_top);
		WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_FB_LOCATION_TOP), fb_location_top);

		WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_SYSTEM_APERTURE_LOW_ADDR), sys_aper_lo);
		WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_SYSTEM_APERTURE_LOW_ADDR), sys_aper_lo);

		WREG32(SOC15_REG_OFFSET(GC, 0, regGCMC_VM_SYSTEM_APERTURE_HIGH_ADDR), sys_aper_hi);
		WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_SYSTEM_APERTURE_HIGH_ADDR), sys_aper_hi);

		WREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE), hdp_nonsurface_lo);
		WREG32(SOC15_REG_OFFSET(HDP, 0, regHDP_NONSURFACE_BASE_HI), hdp_nonsurface_hi);

		return PSP_STATUS__SUCCESS;
	}

	if (navi32_psp_program_register(adapt, idx_vf, fb_location_base, 0,
				       GC_MC_VM_FB_LOCATION_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (navi32_psp_program_register(adapt, idx_vf, fb_location_base, 0,
				       MM_MC_VM_FB_LOCATION_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (navi32_psp_program_register(adapt, idx_vf, fb_location_top, 0,
				       GC_MC_VM_FB_LOCATION_TOP))
		return PSP_STATUS__ERROR_GENERIC;
	if (navi32_psp_program_register(adapt, idx_vf, fb_location_top, 0,
				       MM_MC_VM_FB_LOCATION_TOP))
		return PSP_STATUS__ERROR_GENERIC;
	if (navi32_psp_program_register(adapt, idx_vf, sys_aper_lo, 0,
				       GC_MC_SYSTEM_APERTURE_LO))
		return PSP_STATUS__ERROR_GENERIC;
	if (navi32_psp_program_register(adapt, idx_vf, sys_aper_lo, 0,
				       MM_MC_SYSTEM_APERTURE_LO))
		return PSP_STATUS__ERROR_GENERIC;
	if (navi32_psp_program_register(adapt, idx_vf, sys_aper_hi, 0,
				       GC_MC_SYSTEM_APERTURE_HI))
		return PSP_STATUS__ERROR_GENERIC;
	if (navi32_psp_program_register(adapt, idx_vf, sys_aper_hi, 0,
				       MM_MC_SYSTEM_APERTURE_HI))
		return PSP_STATUS__ERROR_GENERIC;
	if (navi32_psp_program_register(adapt, idx_vf, hdp_nonsurface_lo, 0,
				       HDP_NONSURFACE_BASE))
		return PSP_STATUS__ERROR_GENERIC;
	if (navi32_psp_program_register(adapt, idx_vf, hdp_nonsurface_hi, 0,
				       HDP_NONSURFACE_BASE_HI))
		return PSP_STATUS__ERROR_GENERIC;

	return PSP_STATUS__SUCCESS;
}

enum psp_status navi32_psp_load_keydb(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				     uint32_t fw_image_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_kdb_load_mem = psp->private_fw_memory;
	uint32_t fw_ver;

	if (!psp_kdb_load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy keydb binary to PSP memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_kdb_load_mem.mem), fw_image, fw_image_size);

	/* Provide the kdb driver to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(amdgv_memmgr_get_gpu_addr(psp_kdb_load_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35), PSP_BL__LOAD_KEY_DATABASE);

	/* wait for C2P[35] != PSP_BL__LOAD_KEY_DATABASE */
	if (amdgv_psp_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
					0x80000000, 0x80000000,
					false, AMDGV_WAIT_FLAG_FORCE_YIELD) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("PSP: Failed to load KEYDB.\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;
		AMDGV_INFO("PSP: KEYDB(version:%X.%X.%X.%X) is loaded\n",
			   (fw_ver >> 24) & 0xFF,
			   (fw_ver >> 16) & 0xFF,
			   (fw_ver >> 8) & 0xFF,
			   fw_ver & 0xFF);
	}

	return ret;
}

enum psp_status navi32_psp_load_spl(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				   uint32_t fw_image_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_spl_mem = psp->private_fw_memory;
	uint32_t fw_ver;

	if (!psp_spl_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy SPL Table binary to PSP memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_spl_mem.mem), fw_image, fw_image_size);

	/* Provide SPL Table location in memory to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(amdgv_memmgr_get_gpu_addr(psp_spl_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35), PSP_BL__LOAD_TOS_SPL_TABLE);

	/* wait for C2P[35] != PSP_BL__LOAD_TOS_SPL_TABLE */
	if (amdgv_psp_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
					0x80000000, 0x80000000,
					false, AMDGV_WAIT_FLAG_FORCE_YIELD) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("PSP: Failed to load SPL_TABLE.\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;
		AMDGV_INFO("PSP: SPL_TABLE(version:%X.%X.%X.%X) is loaded\n",
			   (fw_ver >> 24) & 0xFF,
			   (fw_ver >> 16) & 0xFF,
			   (fw_ver >> 8) & 0xFF,
			   fw_ver & 0xFF);
	}
	return ret;
}

enum psp_status navi32_psp_load_sysdrv(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_sysdrv_load_mem = psp->private_fw_memory;
	uint32_t fw_ver;

	if (!psp_sysdrv_load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy PSP System Driver binary to memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_sysdrv_load_mem.mem), fw_image,
		   fw_image_size);

	/* Provide the sys driver to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(amdgv_memmgr_get_gpu_addr(psp_sysdrv_load_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35), PSP_BL__LOAD_SYSDRV);

	/* wait for C2P[35] != PSP_BL__LOAD_SYSDRV */
	if (amdgv_psp_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
					0x80000000, 0x80000000,
					false, AMDGV_WAIT_FLAG_FORCE_YIELD) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("PSP: Failed to load sysdrv.\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP: SYS(version:%X.%X.%X.%X) is loaded.\n", (fw_ver >> 24) & 0xFF,
			   (fw_ver >> 16) & 0xFF, (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SYS] = fw_ver;
	}

	return ret;
}

static enum psp_status navi32_psp_load_rasdrv(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				      uint32_t fw_image_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_rasdrv_load_mem = psp->private_fw_memory;
	uint32_t fw_ver;

	if (!psp_rasdrv_load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy PSP Ras Driver binary to memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_rasdrv_load_mem.mem), fw_image,
		   fw_image_size);

	/* Provide the sys driver to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(amdgv_memmgr_get_gpu_addr(psp_rasdrv_load_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35), PSP_BL__LOAD_RASDRV);

	/* wait for C2P[35] != PSP_BL__LOAD_RASDRV */
	if (amdgv_psp_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
					0x80000000, 0x80000000,
					false, AMDGV_WAIT_FLAG_FORCE_YIELD) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("PSP: Failed to load rasdrv.\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP: RASDRV(version:%X.%X.%X.%X) is loaded.\n", (fw_ver >> 24) & 0xFF,
			   (fw_ver >> 16) & 0xFF, (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_RAS] = fw_ver;
	}

	return ret;
}

enum psp_status navi32_psp_load_sos(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				   uint32_t fw_image_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_sos_load_mem = psp->private_fw_memory;
	uint32_t fw_ver;

	if (!psp_sos_load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy Secure OS binary to PSP memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_sos_load_mem.mem), fw_image, fw_image_size);

	/* Provide the sos driver to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(amdgv_memmgr_get_gpu_addr(psp_sos_load_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35), PSP_BL__LOAD_SOSDRV);

	/* Wait for C2P[35] != PSP_BL__LOAD_SOSDRV */
	// if (amdgv_psp_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
	// 				0x80000000, 0x80000000,
	// 				false, AMDGV_WAIT_FLAG_FORCE_YIELD) != PSP_STATUS__SUCCESS) {
	// 	AMDGV_ERROR("PSP: Failed to load sos.\n");
	// 	return PSP_STATUS__ERROR_GENERIC;
	// }

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP: OS(version:%X.%X.%X.%X) is loaded.\n", (fw_ver >> 24) & 0xFF,
			   (fw_ver >> 16) & 0xFF, (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SOS] = fw_ver;
	}

	return ret;
}

enum psp_status navi32_psp_load_psp_ucode(struct amdgv_adapter *adapt, const unsigned char *fw_image,
				   uint32_t fw_image_size, uint32_t fw_id)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_ucode_load_mem = psp->private_fw_memory;
	uint32_t fw_ver;
	uint32_t bl_cmd;

	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__PSP_SOC:
		bl_cmd = PSP_BL__LOAD_SOCDRV;
		break;
	case AMDGV_FIRMWARE_ID__PSP_DBG:
		bl_cmd = PSP_BL__LOAD_DBGDRV;
		break;
	case AMDGV_FIRMWARE_ID__PSP_INTF:
		bl_cmd = PSP_BL__LOAD_INTFDRV;
		break;
	default:
		return PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
	}

	if (!psp_ucode_load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Copy Secure OS binary to PSP memory */
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_ucode_load_mem.mem), fw_image, fw_image_size);

	/* Provide the ucode to bootrom */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_36),
	       lower_32_bits(amdgv_memmgr_get_gpu_addr(psp_ucode_load_mem.mem) >> 20));
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35), bl_cmd);

	/* Wait for C2P[35] != bl_cmd */
	if (amdgv_psp_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35),
					0x80000000, 0x80000000,
					false, AMDGV_WAIT_FLAG_FORCE_YIELD) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("PSP: Failed to load ucode 0x%x.\n", fw_id);
		return PSP_STATUS__ERROR_GENERIC;
	}

	if (ret == PSP_STATUS__SUCCESS) {
		fw_ver = ((struct psp_fw_image_header *)(fw_image))->image_version;

		AMDGV_INFO("PSP: ucode 0x%x (version:%X.%X.%X.%X) is loaded.\n", fw_id, (fw_ver >> 24) & 0xFF,
			   (fw_ver >> 16) & 0xFF, (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
		adapt->psp.fw_info[fw_id] = fw_ver;
	}

	return ret;
}

bool navi32_psp_wait_boot_complete(struct amdgv_adapter *adapt, uint32_t reg_index)
{

	if (amdgv_wait_for_register(adapt, reg_index, 0, PSP_C2P_MAILBOX_CLOSED,
				    AMDGV_TIMEOUT(TIMEOUT_PSP_REG), AMDGV_WAIT_CHECK_NE, 0) != 0)
		return false;

	if (amdgv_psp_wait_for_register(adapt, reg_index, 0x80000000, 0x80000000,
			false, AMDGV_WAIT_FLAG_FORCE_YIELD) !=
	    PSP_STATUS__SUCCESS)
		return false;

	return true;
}

static int navi32_psp_wait_sos_loaded_status_cb(void *context)
{
	void **context_array = (void **)context;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context_array[0];
	uint32_t *value = (uint32_t *)context_array[1];
	uint32_t reg_val = RREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_81));

	return !((reg_val != 0 && reg_val != PSP_REGISTER_VALUE_INVALID && reg_val != *value));
}

bool navi32_psp_wait_sos_loaded_status(struct amdgv_adapter *adapt)
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

	wait_ret = amdgv_wait_for(adapt, navi32_psp_wait_sos_loaded_status_cb,
				  (void *)context_array, AMDGV_TIMEOUT(TIMEOUT_PSP_REG), 0);
	if (!wait_ret)
		return true;
	else
		return false;
}

enum psp_status navi32_psp_program_register(struct amdgv_adapter *adapt, uint32_t idx_vf,
					   uint32_t reg_value, uint32_t reg_value_hi,
					   enum psp_ih_reg reg_id)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km program_reg_cmd = { 0 };

	AMDGV_DEBUG4("Program Register 0x%x value 0x%x value_hi 0x%x\n", reg_id, reg_value,
		     reg_value_hi);

	program_reg_cmd.cmd_id = PSP_CMD_KM_TYPE__GBR_IH_REG;
	program_reg_cmd.cmd.program_reg.reg_value = reg_value;
	program_reg_cmd.cmd.program_reg.reg_id = reg_id;
	program_reg_cmd.cmd.program_reg.reg_value_hi = reg_value_hi;
	program_reg_cmd.cmd.program_reg.target_vfid = idx_vf;

	ret = amdgv_psp_cmd_km_submit(adapt, &program_reg_cmd, NULL);

	if (ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("PSP: Failed to Write Register %d\n", reg_id);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

static enum psp_status navi32_psp_v11_vfgate_support(struct amdgv_adapter *adapt)
{
	return PSP_STATUS__SUCCESS;
}

static enum psp_status navi32_psp_v11_set_mb_int(struct amdgv_adapter *adapt, uint32_t idx_vf,
						 bool enable)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *vfgate_cmd = adapt->psp.psp_cmd_km_mem;
	bool resp_enable = false;
	struct psp_gfx_resp psp_resp = { 0 };

	ret = navi32_psp_v11_vfgate_support(adapt);

	if (ret == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE)
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
			AMDGV_DEBUG("psp mailbox %s for VF%d\n",
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

static enum psp_status navi32_psp_v11_get_mb_int_status(struct amdgv_adapter *adapt,
							uint32_t idx_vf, struct psp_mb_status *mb_status)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *vfgate_cmd = adapt->psp.psp_cmd_km_mem;
	struct psp_gfx_resp psp_resp = { 0 };
	uint32_t gfx_fw_type = 0;
	enum amdgv_firmware_id psp_fw_id = (enum amdgv_firmware_id)AMDGV_FIRMWARE_ID__MAX;

	ret = navi32_psp_v11_vfgate_support(adapt);

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
static enum psp_status navi32_psp_set_num_vfs(struct amdgv_adapter *adapt)
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

enum psp_status navi32_psp_fw_attestation_support(struct amdgv_adapter *adapt)
{
	return PSP_STATUS__SUCCESS;
}

void navi32_psp_get_fw_version(enum amdgv_firmware_id firmware_id,
			uint32_t image_version, char *fw_version, uint32_t size)
{
	switch (firmware_id) {
	case AMDGV_FIRMWARE_ID__SDMA0:
	case AMDGV_FIRMWARE_ID__SDMA1:
	case AMDGV_FIRMWARE_ID__RLC:
	case AMDGV_FIRMWARE_ID__RLC_V:
	case AMDGV_FIRMWARE_ID__RLC_P:
	case AMDGV_FIRMWARE_ID__CP_CE:
	case AMDGV_FIRMWARE_ID__CP_PFP:
	case AMDGV_FIRMWARE_ID__CP_ME:
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0:
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1:
	case AMDGV_FIRMWARE_ID__RS64_ME_UCODE:
	case AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA:
	case AMDGV_FIRMWARE_ID__RS64_ME_P1_DATA:
	case AMDGV_FIRMWARE_ID__RS64_PFP_UCODE:
	case AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA:
	case AMDGV_FIRMWARE_ID__RS64_PFP_P1_DATA:
	case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
		oss_vsnprintf(fw_version, size, "%d", image_version);
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
	case AMDGV_FIRMWARE_ID__CP_MEC_JT2:
	case AMDGV_FIRMWARE_ID__CP_MEC1:
	case AMDGV_FIRMWARE_ID__CP_MEC2:
		if (image_version & 0x8000)
			oss_vsnprintf(fw_version, size, "%d(Two-level)",
				image_version & 0x7FFF);
		else
			oss_vsnprintf(fw_version, size, "%d",
				image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL:
		oss_vsnprintf(fw_version, size, "%d",
				image_version & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__MMSCH:
		oss_vsnprintf(fw_version, size, "%d.%d.%d",
				(image_version >> 24) & 0xFF,
				(image_version >> 16) & 0xFF,
				image_version & 0xFFFF);
		break;
	case AMDGV_FIRMWARE_ID__SMU:
		oss_vsnprintf(fw_version, size, "%d.%d.%d",
				(image_version >> 8) & 0xFF,
				(image_version >> 16) & 0xFF,
				(image_version >> 24) & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__UVD:
		oss_vsnprintf(fw_version, size, "%d.%d.%d.%d",
				(image_version >> 30) & 0x3,
				(image_version >> 24) & 0x3F,
				(image_version >> 8) & 0xFF,
				image_version & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__VCE:
		oss_vsnprintf(fw_version, size, "%d.%d.%d",
				(image_version >> 20) & 0xFFF,
				(image_version >> 8) & 0xFFF,
				image_version & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__DFC_FW:
		oss_vsnprintf(fw_version, size, "%x.%x.%x.%x",
				(image_version >> 24) & 0xFF,
				(image_version >> 16) & 0xFF,
				(image_version >> 8) & 0xFF,
				image_version & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__VCN:
		oss_vsnprintf(fw_version, size, "%d.%d.%d.%d",
				(image_version >> 24) & 0xF,
				(image_version >> 20) & 0xF,
				(image_version >> 12) & 0xFF,
				image_version & 0xFFF);
		break;
	case AMDGV_FIRMWARE_ID__IMU_DRAM:
		oss_vsnprintf(fw_version, size, "%d.%d.%d.%d",
				(image_version >> 24) & 0xF,
				(image_version >> 20) & 0xF,
				(image_version >> 12) & 0xFF,
				image_version & 0xFFF);
		break;
	case AMDGV_FIRMWARE_ID__IMU_IRAM:
		oss_vsnprintf(fw_version, size, "%d.%d.%d.%d",
				(image_version >> 24) & 0xF,
				(image_version >> 20) & 0xF,
				(image_version >> 12) & 0xFF,
				image_version & 0xFFF);
		break;
	case AMDGV_FIRMWARE_ID__CP_MES:
	case AMDGV_FIRMWARE_ID__MES_STACK:
	case AMDGV_FIRMWARE_ID__MES_THREAD1:
	case AMDGV_FIRMWARE_ID__MES_THREAD1_STACK:
		oss_vsnprintf(fw_version, size, "0x%08x", image_version);
		break;
	default:
		oss_vsnprintf(fw_version, size, "unknown");
		break;
	}
}

enum psp_status navi32_psp_fb_addr_bound_check(struct amdgv_adapter *adapt, uint64_t fb_addr, uint64_t size)
{
	uint64_t upper_bound = 0, lower_bound = 0;

	if ((void *)fb_addr == NULL || size <= 0) {
		AMDGV_ERROR("Invalid fb address information.\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	// 3Mb (gpu memmgr offset) from top of GPU fb is reserved
	// Reserved area is to save PSP and vbios info
	if ((adapt->memmgr_gpu).down) {
		upper_bound = (adapt->memmgr_gpu).mc_base - size;
		lower_bound = (adapt->memmgr_gpu).mc_base - (adapt->memmgr_gpu).offset;
	} else {
		lower_bound = (adapt->memmgr_gpu).mc_base;
		upper_bound = (adapt->memmgr_gpu).mc_base + (adapt->memmgr_gpu).offset - size;
	}

	if ((fb_addr < lower_bound) || (fb_addr > upper_bound)) {
		AMDGV_ERROR("Fb address 0x%llx out of bound, psp reserved fb is 0x%llx - 0x%llx.\n",
				fb_addr, lower_bound, upper_bound);
		return PSP_STATUS__ERROR_GENERIC;
	}

	return PSP_STATUS__SUCCESS;
}

enum psp_status navi32_psp_get_fw_attestation_database_addr(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	void *cpu_addr;

	if (!amdgv_psp_fw_attestation_support(adapt)) {
		AMDGV_WARN("FW attestation feature is currently not supported.\n");
		return ret;
	}

	if (!amdgv_in_live_update_seq()) {
		// Get FW attestation GPU virtual address
		ret = amdgv_psp_get_fw_attestation_db_add(adapt);

		if (ret != PSP_STATUS__SUCCESS) {
			AMDGV_ERROR("Failed to get fw attestation address.\n");
			return PSP_STATUS__ERROR_GENERIC;
		}
	}

	// FW attestation mc_address bound check
	ret = navi32_psp_fb_addr_bound_check(adapt,
			(adapt->psp).attestation_db_gpu_addr,
			sizeof(struct ATTESTATION_DB_HEADER));

	if (ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("Attestation mc_address locates at an invalid region.\n");
		(adapt->psp).attestation_db_gpu_addr = 0;
		return PSP_STATUS__ERROR_GENERIC;
	}

	// Get FW attestation CPU virtual address
	if (amdgv_acquire_fb_virtual_addr(adapt,
			(adapt->psp).attestation_db_gpu_addr,
			sizeof(struct ATTESTATION_DB_HEADER), (uint64_t *)&cpu_addr) == 0) {
		(adapt->psp).attestation_db_cpu_addr = cpu_addr;
	} else {
		AMDGV_ERROR("Acquire frame buffer cpu virtual address failed, cpu virtual addr is NULL.\n");
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

static enum psp_status navi32_psp_parse_psp_info(struct amdgv_adapter *adapt)
{
	if ((adapt->psp).attestation_db_gpu_addr != 0)
		return navi32_psp_get_fw_attestation_database_addr(adapt);

	return PSP_STATUS__SUCCESS;
}

enum psp_status navi32_psp_clear_fw_attestation_database_addr(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	uint64_t mc_base, gpu_addr, offset;

	if (!amdgv_psp_fw_attestation_support(adapt)) {
		AMDGV_WARN("FW attestation feature is currently not supported.\n");
		return ret;
	}

	mc_base = adapt->mc_fb_loc_addr;
	gpu_addr = (adapt->psp).attestation_db_gpu_addr;
	if (!gpu_addr) {
		AMDGV_WARN("FW attestation address is NULL.\n");
		return ret;
	}

	offset = (gpu_addr > mc_base) ?
				(gpu_addr - mc_base) : (mc_base - gpu_addr);

	// Unmap FW attestation CPU virtual address
	if (amdgv_release_fb_virtual_addr(adapt,
		(adapt->psp).attestation_db_cpu_addr, offset) == 0)
		(adapt->psp).attestation_db_cpu_addr = NULL;
	else {
		AMDGV_ERROR("Unmap and clear Fw Attestation frame buffer address failed.\n");
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

enum psp_status navi32_psp_get_fw_attestation_info(struct amdgv_adapter *adapt,
						uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct FWMAN_DB_HEADER *pDb =
		(struct FWMAN_DB_HEADER *)psp->attestation_db_cpu_addr;
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

	oss_memset(vf->fw_info, 0, AMDGV_FIRMWARE_ID__MAX * sizeof(struct amdgv_firmware_info));

	for (i = 0; i < MAX_VF_DB_SIZE; ++i) {
		struct FWMAN_RECORD *vf_entry =
			(struct FWMAN_RECORD *) &(pDb->vf_table[i]);

		if (vf_entry->FwManId.FwEntry.VfId == idx_vf
				&& vf_entry->record_info.bit0.record_valid) {

			fw_id = amdgv_psp_fw_id_map(vf_entry->FwManId.FwEntry.FwId);

			if (fw_id != AMDGV_FIRMWARE_ID__MAX &&
				fw_info_index != AMDGV_FIRMWARE_ID__MAX) {
				vf->fw_info[fw_info_index].id = fw_id;
				vf->fw_info[fw_info_index].version = vf_entry->FwVer;

				navi32_psp_get_fw_version(fw_id,
						vf->fw_info[fw_info_index].version,
						fw_version, sizeof(fw_version));

				AMDGV_DEBUG("PSP: id:%d(version:%s) is loaded on vf%d.\n",
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

static enum psp_status navi32_exec_spi_cmd(struct amdgv_adapter *adapt, int spi_cmd)
{
	uint32_t reg_status = 0;
	uint32_t reg_val = 0;

	/* clear MBX ready (MBOX_READY_MASK bit is 0) and set update command */
	reg_val |= (spi_cmd << 16);
	WREG32_SOC15(MP0, 0, regMP0_SMN_C2PMSG_115,  reg_val);

	/* Ring the doorbell */
	WREG32_SOC15(MP0, 0, regMP0_SMN_C2PMSG_73, 1);

	if (spi_cmd == C2PMSG_CMD_SPI_UPDATE_FLASH_IMAGE)
		return 0;

	if (amdgv_psp_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_115),
				0x80000000, 0x80000000, false, AMDGV_WAIT_FLAG_FORCE_YIELD) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("SPI cmd %x timed out\n", spi_cmd);
		return PSP_STATUS__ERROR_GENERIC;
	}

	reg_status = RREG32_SOC15(MP0, 0, regMP0_SMN_C2PMSG_115);
	if ((reg_status & 0xFFFF) != 0) {
		AMDGV_ERROR("SPI cmd %x failed, fail status = %04x\n",
				spi_cmd, reg_status & 0xFFFF);
		return PSP_STATUS__ERROR_GENERIC;
	}

	return PSP_STATUS__SUCCESS;
}

static enum psp_status navi32_psp_update_spirom(struct amdgv_adapter *adapt)
{
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *local_mem = &(psp->vbflash_context.shared_buffer);

	/* Confirm PSP is ready to start */
	if (amdgv_psp_wait_for_register(adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_115),
					0x80000000, 0x80000000, false, AMDGV_WAIT_FLAG_FORCE_YIELD) != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("PSP: Not ready to start updating spirom, 115 value is 0x%x\n", RREG32_SOC15(MP0, 0, regMP0_SMN_C2PMSG_115));
		return PSP_STATUS__ERROR_GENERIC;
	}

	WREG32_SOC15(MP0, 0, regMP0_SMN_C2PMSG_116, lower_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem)));

	if (navi32_exec_spi_cmd(adapt, C2PMSG_CMD_SPI_UPDATE_ROM_IMAGE_ADDR_LO) != PSP_STATUS__SUCCESS)
		return PSP_STATUS__ERROR_GENERIC;

	WREG32_SOC15(MP0, 0, regMP0_SMN_C2PMSG_116, upper_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem)));

	if (navi32_exec_spi_cmd(adapt, C2PMSG_CMD_SPI_UPDATE_ROM_IMAGE_ADDR_HI) != PSP_STATUS__SUCCESS)
		return PSP_STATUS__ERROR_GENERIC;
	psp->vbflash_context.vbflash_done = true;

	if (navi32_exec_spi_cmd(adapt, C2PMSG_CMD_SPI_UPDATE_FLASH_IMAGE) != PSP_STATUS__SUCCESS)
		return PSP_STATUS__ERROR_GENERIC;

	return PSP_STATUS__SUCCESS;
}

static enum psp_status navi32_psp_vbflash_status(struct amdgv_adapter *adapt, uint32_t *status)
{
	*status = RREG32_SOC15(MP0, 0, regMP0_SMN_C2PMSG_115);
	return PSP_STATUS__SUCCESS;
}

enum psp_status navi32_psp_vf_cmd_relay(struct amdgv_adapter *adapt, uint32_t vf_id)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *vf_relay_cmd = adapt->psp.psp_cmd_km_mem;
	struct psp_gfx_resp psp_resp = { 0 };

	vf_relay_cmd->cmd_id = PSP_CMD_KM_TYPE__VF_RELAY;
	vf_relay_cmd->cmd.vf_relay.target_vf = vf_id;
	vf_relay_cmd->cmd.vf_relay.vf_relay_wtr_ptr = adapt->psp.vf_relay_wtr_ptr[vf_id];

	ret = amdgv_psp_cmd_km_submit(adapt, vf_relay_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS || psp_resp.status != 0) {
		AMDGV_INFO("PSP: failed to submit PSP_CMD_KM_TYPE__VF_RELAY "
			"(gfx_cmd_resp=0x%08x)\n", psp_resp.status);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

enum psp_status navi32_psp_load_asd_fw_to_mem(struct amdgv_adapter *adapt,
						struct psp_local_memory *asd_bin_mem, uint32_t *size)
{
	const unsigned char *fw_image;
	uint32_t fw_image_size;

	if (!asd_bin_mem || !size) {
		AMDGV_INFO("PSP: failed to load asd fw, local memory input is NULL\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	switch (adapt->psp.tee_version) {
	case GFX_TEE_VERSION_3:
		fw_image = psp_asd_tee3_bin;
		fw_image_size = sizeof(psp_asd_tee3_bin);
		break;
	default:
		fw_image = psp_asd_bin;
		fw_image_size = sizeof(psp_asd_bin);
		break;
	}

	*size = fw_image_size;
	oss_memset(amdgv_memmgr_get_cpu_addr(asd_bin_mem->mem), 0, PSP_PRIVATE_IMAGE_SIZE);
	oss_memcpy(amdgv_memmgr_get_cpu_addr(asd_bin_mem->mem), fw_image, fw_image_size);

	return PSP_STATUS__SUCCESS;
}

static enum psp_status navi32_psp_get_migration_version(struct amdgv_adapter *adapt,
	uint32_t *migration_version)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *migration_get_psp_info_cmd = NULL;
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
		} else {
			amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_FW_MIGRATION_GET_PSP_INFO_FAIL,
				psp_resp.status);
		}
	} else {
		*migration_version =
			psp_resp.uresp.migration_info.migration_version;
		AMDGV_DEBUG("Live Migration Version: 0x%x\n", *migration_version);
	}

	return ret;
}

static enum psp_status navi32_psp_get_migration_info(struct amdgv_adapter *adapt)
{
	int ret = 0;

	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;

	ret = navi32_psp_get_migration_version(adapt, &adapt->live_migration.migration_version);
	if (ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("Failed to get PSP migration version.\n");
		return ret;
	}

	return ret;
}

static int navi32_psp_migration_cmd_init(struct psp_cmd_km *migration_cmd,
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

static enum psp_status navi32_psp_transfer_manifest_data(struct amdgv_adapter *adapt,
				uint32_t idx_vf, uint64_t data_addr, uint32_t size,
				enum psp_migration_manifest_data_type type)
{
	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;
	struct psp_cmd_km *migration_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };

	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;

	migration_cmd = adapt->psp.psp_cmd_km_mem;
	if (!migration_cmd) {
		amdgv_put_error(AMDGV_PF_IDX,
		AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
		sizeof(struct psp_cmd_km));
		return ret;
	}

	if (navi32_psp_migration_cmd_init(migration_cmd, idx_vf, data_addr, size, type)) {
		amdgv_put_error(AMDGV_PF_IDX,
		AMDGV_ERROR_FW_MIGRATION_EXPORT_FAIL,
		psp_resp.status);
		return ret;
	}

	ret = amdgv_psp_cmd_km_submit(adapt, migration_cmd, &psp_resp);
	if (ret != PSP_STATUS__SUCCESS) {
		if (psp_resp.status == PSP_KM_TEE_ERROR_NOT_SUPPORTED) {
			amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_FW_NOT_SUPPORTED_FEATURE, 0);
			ret = PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;
		} else {
			amdgv_put_error(AMDGV_PF_IDX,
				AMDGV_ERROR_FW_MIGRATION_IMPORT_FAIL,
				psp_resp.status);
		}
	}

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

	// TODO: implement psp_resp.status to give specific error
	if ((type == PSP_MIGRATION_EXPORT_STATIC_DATA) ||
		(type == PSP_MIGRATION_EXPORT_DYNAMIC_DATA)) {
		uint32_t pkg_size = psp_resp.uresp.migration_export.size_written;

		if (pkg_size <= 0 || pkg_size > migration_cmd->cmd.migration_export.pkg_size_allocated) {
			AMDGV_ERROR("PSP export data is empty or oversized.\n");
			ret = AMDGV_FAILURE;
		}
	}

	return ret;
}

static enum psp_status navi32_psp_clear_vf_fw(struct amdgv_adapter *adapt, uint32_t idx_vf)
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

static int navi32_psp_sw_fini(struct amdgv_adapter *adapt)
{
	if (amdgv_psp_sw_fini(adapt) != PSP_STATUS__SUCCESS)
		return AMDGV_FAILURE;

	return 0;
}

static enum psp_status navi32_psp_tmr_init(struct amdgv_adapter *adapt, uint32_t tmr_size)
{
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *local_mem = &(psp->tmr_context);

	AMDGV_DEBUG("allocate TMR\n");
    /* TMR address alignment is 2MB to force its start from 2MB boundary, so */
    /* there is no need to allocate DUMMY memory and afterwards free it      */

	/* Allocate TMR in bottom of FB */
	local_mem->mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, tmr_size,
						  local_mem->alignment, MEM_PSP_TMR);

	if (!local_mem->mem) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL,
				tmr_size);
		return PSP_STATUS__ERROR_GENERIC;
	}

	AMDGV_INFO("TMR: GPU_ADDR=0x%llx MEM_ADDR=0x%llx MEM_SIZE=0x%llx\n",
			amdgv_memmgr_get_gpu_addr(local_mem->mem),
			amdgv_memmgr_get_offset(local_mem->mem),
			amdgv_memmgr_get_size(local_mem->mem));

	return PSP_STATUS__SUCCESS;
}

static bool navi32_psp_need_switch_to_pf(struct amdgv_adapter *adapt)
{
	return false;
}

static int navi32_psp_sw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;

	oss_memset(&adapt->psp, 0, sizeof(struct psp_context));

	adapt->psp.allocated_tmr_size = roundup(PSP_TMR_SIZE(adapt->max_num_vf), PSP_TMR_SIZE_ALIGNMENT);
	adapt->psp.tmr_context.alignment = NAVI32_PSP_TMR_ALIGNMENT;
	adapt->psp.get_fw_attestation_info = navi32_psp_get_fw_attestation_info;
	adapt->psp.fw_attestation_support = navi32_psp_fw_attestation_support;
	adapt->psp.fw_id_support = amdgv_psp_fw_id_support;
	adapt->psp.program_register = navi32_psp_program_register;
	adapt->psp.get_wptr = navi32_psp_ring_get_wptr;
	adapt->psp.set_wptr = navi32_psp_ring_set_wptr;
	adapt->psp.load_keydb = navi32_psp_load_keydb;
	adapt->psp.load_spl = navi32_psp_load_spl;
	adapt->psp.load_sysdrv = navi32_psp_load_sysdrv;
	adapt->psp.load_rasdrv = navi32_psp_load_rasdrv;
	adapt->psp.load_sosdrv = navi32_psp_load_sos;
	adapt->psp.set_mb_int = navi32_psp_v11_set_mb_int;
	adapt->psp.get_mb_int_status = navi32_psp_v11_get_mb_int_status;
	adapt->psp.vfgate_support = navi32_psp_v11_vfgate_support;
	adapt->psp.load_psp_ucode = navi32_psp_load_psp_ucode;
	adapt->psp.update_spirom = navi32_psp_update_spirom;
	adapt->psp.vbflash_status = navi32_psp_vbflash_status;
	adapt->psp.psp_program_guest_mc_settings = navi32_psp_program_guest_mc_settings;
	adapt->psp.need_switch_to_pf = navi32_psp_need_switch_to_pf;
	/* false on PSP TEE 3.0 */
	adapt->psp.ras_need_switch_to_pf = navi32_psp_need_switch_to_pf;
	adapt->psp.vf_relay = navi32_psp_vf_cmd_relay;
	adapt->psp.load_asd_fw_to_mem = navi32_psp_load_asd_fw_to_mem;
	adapt->psp.parse_psp_info = navi32_psp_parse_psp_info;
	adapt->psp.tmr_init = navi32_psp_tmr_init;
	adapt->psp.transfer_manifest_data = navi32_psp_transfer_manifest_data;
	adapt->psp.get_migration_info = navi32_psp_get_migration_info;
	adapt->psp.clear_vf_fw = navi32_psp_clear_vf_fw;

	psp_ret = amdgv_psp_sw_init(adapt);
	adapt->psp.ras_context.set_init_flag = true;

	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_INIT_FAIL, 0);
		navi32_psp_sw_fini(adapt);
		ret = AMDGV_FAILURE;
	}

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION) {
		adapt->live_migration.static_data_size = NAVI32_MIGRATION_PSP_STATIC_DATA_SIZE;
		adapt->live_migration.dynamic_data_size = NAVI32_MIGRATION_PSP_DYNAMIC_DATA_SIZE;
	}

	return ret;
}

static int navi32_psp_load_toc_tmr(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum amdgv_firmware_id ucode_id;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;

	psp_ret = navi32_psp_set_num_vfs(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS)
		return AMDGV_FAILURE;

	ucode_id = AMDGV_FIRMWARE_ID__PSP_TOC;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL, ucode_id);
		return AMDGV_FAILURE;
	}

	navi32_log_toc_version(adapt);

	/* compare allocated tmr size with psp calculated tmr size */
	if (adapt->psp.allocated_tmr_size < adapt->psp.tmr_context.size) {
		AMDGV_ERROR("Not enough memory, allocated tmr size 0x%09llx is smaller than "
			"psp calculated size 0x%09llx\n", adapt->psp.allocated_tmr_size,
			adapt->psp.tmr_context.size);
		return AMDGV_FAILURE;
	}

	psp_ret = amdgv_psp_tmr_load(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_TMR_LOAD_FAIL, 0);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_psp_load_smu(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum amdgv_firmware_id ucode_id;

	ucode_id = AMDGV_FIRMWARE_ID__SMU;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL, ucode_id);
		return AMDGV_FAILURE;
	}

	/* wait for SMU to report ready */
	if (!adapt->pp.pp_funcs->get_smu_fw_loaded_status(adapt)) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL, ucode_id);
		return AMDGV_FAILURE;
	}

	return ret;
}

static int navi32_psp_load_psp_fw(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum amdgv_firmware_id ucode_id;

	int i = 600;
	ucode_id = AMDGV_FIRMWARE_ID__PSP_KEYDB;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return ret;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_SYS;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return ret;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_RAS;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return ret;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_SOC;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return ret;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_DBG;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return ret;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_INTF;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return ret;
	}

	ucode_id = AMDGV_FIRMWARE_ID__PSP_SOS;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return ret;
	}


	 if (adapt->emu_type != FULL_EMU) {
		/* Check sOS sign of life register to confirm that
		* PSP drv_sys and sOS are loaded, alive and ready to respond
		* to GFX mailbox (commands from host and VMs)
		*/
		if (navi32_psp_wait_sos_loaded_status(adapt) == false) {
			AMDGV_ERROR("TIMEOUT waiting for PSP tOS sign-of-life\n");
			return AMDGV_FAILURE;
		}
	} else {
		while (i) {
			oss_msleep(1000);
			i--;
		}
	}




	return ret;
}

static int navi32_psp_load_imu(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum amdgv_firmware_id ucode_id;

	ucode_id = AMDGV_FIRMWARE_ID__IMU_IRAM;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return AMDGV_FAILURE;
	}

	ucode_id = AMDGV_FIRMWARE_ID__IMU_DRAM;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return AMDGV_FAILURE;
	}

	return ret;
}

static int navi32_psp_load_np_fw(struct amdgv_adapter *adapt)
{
	int ret = 0;

	enum amdgv_firmware_id ucode_np_seq[] = {
		AMDGV_FIRMWARE_ID__RS64_PFP_UCODE,
		AMDGV_FIRMWARE_ID__RS64_ME_UCODE,
		AMDGV_FIRMWARE_ID__RS64_MEC_UCODE,
		AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA,
		AMDGV_FIRMWARE_ID__RS64_ME_P1_DATA,
		AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA,
		AMDGV_FIRMWARE_ID__RS64_PFP_P1_DATA,
		AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA,
		AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA,
		AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA,
		AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA,
		AMDGV_FIRMWARE_ID__RLC,
		AMDGV_FIRMWARE_ID__RLC_V,	//Side-load F32 RLCV as part of FW WA of a world switch.
		AMDGV_FIRMWARE_ID__RLC_P,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM,
		AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM,
		AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0,
		AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1,
		AMDGV_FIRMWARE_ID__CP_MES,
		AMDGV_FIRMWARE_ID__MES_STACK,
		AMDGV_FIRMWARE_ID__MES_THREAD1,
		AMDGV_FIRMWARE_ID__MES_THREAD1_STACK,
		AMDGV_FIRMWARE_ID__RLX6,
		AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT,
		AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1,
		AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1,
		AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST,
		AMDGV_FIRMWARE_ID__DFC_FW,
	};

	ret = adapt->ucode.load(adapt, ucode_np_seq, ARRAY_SIZE(ucode_np_seq));
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL, 0);
		ret = AMDGV_FAILURE;
		goto load_failed;
	}

load_failed:
	return ret;
}

#define smnMP1_EXT_SCRATCH0             0x3b10d00

void navi32_grbm_select(struct amdgv_adapter *adapt,
		     uint32_t me, uint32_t pipe, uint32_t queue, uint32_t vmid)
{
	uint32_t grbm_gfx_cntl = 0;
	grbm_gfx_cntl = REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, PIPEID, pipe);
	grbm_gfx_cntl = REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, MEID, me);
	grbm_gfx_cntl = REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, VMID, vmid);
	grbm_gfx_cntl = REG_SET_FIELD(grbm_gfx_cntl, GRBM_GFX_CNTL, QUEUEID, queue);

	WREG32(SOC15_REG_OFFSET(GC, 0, regGRBM_GFX_CNTL), grbm_gfx_cntl);
}

static int navi32_psp_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t i;
	struct psp_runtime_scpm_entry scpm_entry = { 0 };
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;

	/* SCPM database */
	if (amdgv_psp_get_runtime_db_entry(adapt, PSP_RUNTIME_ENTRY_TYPE_PPTABLE_ERR_STATUS, &scpm_entry)) {
		adapt->scpm_enabled = false;
		adapt->psp.scpm_status = SCPM_DISABLE;
		ret = AMDGV_FAILURE;
		AMDGV_ERROR("Error retrieving SCPM status!\n");
		goto init_fail;
	}

	adapt->psp.scpm_status = scpm_entry.scpm_enable_bits;

	AMDGV_INFO("SCPM Status: 0x%x\n", adapt->psp.scpm_status);

	if (SCPM_ENABLE == scpm_entry.scpm_enable_bits) {
		adapt->scpm_enabled = true;
		AMDGV_INFO("SCPM is enabled\n");
	} else if (SCPM_DISABLE == scpm_entry.scpm_enable_bits) {
		adapt->scpm_enabled = false;
		AMDGV_WARN("SCPM is disabled\n");
	} else {
		adapt->scpm_enabled = false;
		AMDGV_ERROR("SCPM error!\n");
		ret = AMDGV_FAILURE;
		goto init_fail;
	}

	/* Check if PSP BL is ready to load psp fw */
	if (navi32_psp_wait_boot_complete(
		    adapt, SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_35)) == false) {
		AMDGV_ERROR("TIMEOUT waiting for GFX mailbox to open\n");
		return AMDGV_FAILURE;
	}

	/* load psp keydb sos, sysdrv, spl */
	ret = navi32_psp_load_psp_fw(adapt);
	if (ret)
		goto init_fail;

	psp_ret = navi32_psp_ring_start(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_START_RING_FAIL, 0);
		return AMDGV_FAILURE;
	}

	psp_ret = amdgv_psp_cmd_km_start(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		goto init_fail;
	}

	ret = navi32_psp_load_smu(adapt);
	if (ret) {
		goto init_fail;
	}
	ret = navi32_psp_load_toc_tmr(adapt);
	if (ret)
		goto init_fail;

	/* Disable psp mailbox interrupts for all vfs */
	for (i = 0; i < adapt->num_vf; i++)
		navi32_psp_v11_set_mb_int(adapt, i, false);

	psp_ret = amdgv_psp_asd_load(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_ASD_LOAD_FAIL, 0);
		ret = AMDGV_FAILURE;
		goto init_fail;
	}

	// psp_ret = amdgv_psp_ras_initialize(adapt);
	// if (psp_ret != PSP_STATUS__SUCCESS) {
	// 	ret = AMDGV_FAILURE;
	// 	goto init_fail;
	// }

	ret = navi32_psp_load_imu(adapt);
	if (ret)
		goto init_fail;

	ret = navi32_psp_load_np_fw(adapt);
	if (ret)
		goto init_fail;

	/* start rlc autoload after psp recieved all gfx firmware */
	psp_ret = amdgv_psp_start_rlc_autoload(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		goto init_fail;
	}

	psp_ret = navi32_psp_get_fw_attestation_database_addr(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		AMDGV_INFO("Fail Getting FW Attestation Database Addr.");
		goto init_fail;
	}


init_fail:
	if (ret)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_INIT_FAIL, 0);

	return ret;
}

static int navi32_psp_hw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum psp_status psp_ret;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *local_mem = &(psp->tmr_context);
	struct psp_cmd_km tmr_km_cmd = { 0 };
	struct psp_ras_context *ras_context = &psp->ras_context;

	oss_mutex_lock(adapt->psp_lock);

	psp_ret = navi32_psp_clear_fw_attestation_database_addr(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		AMDGV_ERROR("PSP: Failed to unmap fw attestation database addr.\n");
	}

	if (ras_context->ras_initialized)
		amdgv_psp_ras_terminate(adapt);

	amdgv_psp_asd_unload(adapt);

	/* destory TMR
	 * TMR Destroy will destroy all the ucode resident inside the TMR
	 */
	// No need to destroy TMR in reset process
	if (!adapt->reset.reset_state && local_mem->mem) {
		/* Prepare CMD to destroy TMR */
		tmr_km_cmd.cmd_id = PSP_CMD_KM_TYPE__DESTROY_TMR;
		tmr_km_cmd.cmd.setup_tmr.tmr_size = local_mem->size;
		tmr_km_cmd.cmd.setup_tmr.tmr_buf_addr_lo =
			lower_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
		tmr_km_cmd.cmd.setup_tmr.tmr_buf_addr_hi =
			upper_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));

		/* Submit CMD buffer to destroy TMR */
		psp_ret = amdgv_psp_cmd_km_submit(adapt, &tmr_km_cmd, NULL);

		if (psp_ret != PSP_STATUS__SUCCESS) {
			ret = AMDGV_FAILURE;
			AMDGV_ERROR("PSP: Failed to destroy TMR.\n");
		}
	}

	/* destory PSP RBI_RING */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64),
	       GFX_CTRL_CMD_ID_DESTROY_RBI_RING);
	/* Wait for response flag (bit 31) in C2PMSG_64 */
	psp_ret = amdgv_psp_wait_for_register(adapt,
					      SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64),
					      0x80000000, 0x80000000, false, AMDGV_WAIT_FLAG_FORCE_YIELD);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		AMDGV_ERROR("PSP: Failed to DESTROY_RBI_RING.\n");
	}

	/* destory PSP GPCOM_RING */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64),
	       GFX_CTRL_CMD_ID_DESTROY_GPCOM_RING);
	/* Wait for response flag (bit 31) in C2PMSG_64 */
	psp_ret = amdgv_psp_wait_for_register(adapt,
					      SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64),
					      0x80000000, 0x80000000, false, AMDGV_WAIT_FLAG_FORCE_YIELD);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		AMDGV_ERROR("PSP: Failed to DESTROY_GPCOM_RING.\n");
	}

	/* destory PSP rings */
	WREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64), GFX_CTRL_CMD_ID_DESTROY_RINGS);
	/* Wait for response flag (bit 31) in C2PMSG_64 */
	psp_ret = amdgv_psp_wait_for_register(adapt,
					      SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_64),
					      0x80000000, 0x80000000, false, AMDGV_WAIT_FLAG_FORCE_YIELD);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		AMDGV_ERROR("PSP: Failed to DESTROY_RINGS.\n");
	}

	oss_mutex_unlock(adapt->psp_lock);

	return ret;
}

static int navi32_psp_post_reset(struct amdgv_adapter *adapt)
{
	int ret = 0;
	enum psp_status psp_ret;

	adapt->psp.ras_context.ras_initialized = false;
	oss_mutex_lock(adapt->psp_lock);
	psp_ret = navi32_psp_clear_fw_attestation_database_addr(adapt);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		ret = AMDGV_FAILURE;
		AMDGV_ERROR("PSP: Failed to unmap fw attestation database addr.\n");
	}
	oss_mutex_unlock(adapt->psp_lock);
	return ret;
}

enum psp_status navi32_psp_dump_tracelog(struct amdgv_adapter *adapt,
    uint64_t buf_bus_addr, uint32_t buf_size, uint32_t *buf_used_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km *dump_tracelog_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };

	if (!buf_used_size) {
		AMDGV_ERROR("Failed to dump tracelog: buffer size invalid");
		return ret;
	}
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

enum psp_status navi32_psp_set_snapshot_addr(struct amdgv_adapter *adapt,
    uint64_t buf_bus_addr, uint32_t buf_size)
{
	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;
	struct psp_cmd_km *snapshot_set_addr_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };

	if (buf_size == 0) {
		AMDGV_ERROR("Failed to set snapshot dump address: buffer size invalid\n");
		return ret;
	}

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

enum psp_status navi32_psp_trigger_snapshot(struct amdgv_adapter *adapt,
    uint32_t target_vfid, uint32_t sections, uint32_t *buf_used_size)
{
	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;
	struct psp_cmd_km *snapshot_trigger_cmd = NULL;
	struct psp_gfx_resp psp_resp = { 0 };

	if (!buf_used_size) {
		AMDGV_ERROR("Failed to trigger snapshot: buffer size invalid");
		return ret;
	}

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

const struct amdgv_init_func navi32_psp_func = {
	.name = "navi32_psp_func",
	.sw_init = navi32_psp_sw_init,
	.sw_fini = navi32_psp_sw_fini,
	.hw_init = navi32_psp_hw_init,
	.hw_fini = navi32_psp_hw_fini,
	.post_reset = navi32_psp_post_reset,
};
