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

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gpuiov.h>

#include "atombios/atomfirmware.h"
#include "gfxhub_v1_2.h"
#include "mi300.h"
#include "mi300_psp.h"
#include "mmhub_v1_8.h"

#include "mi300_gfx.h"
#include "mi300_golden_settings.h"
#include "mi300_nbio.h"
#include "mi300_powerplay.h"

#include "mi300_reset.h"
#include "smuio_v13_0_3.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

#define BUILD_NUM_MAX_LENGTH 8
#define BUILD_DATA_LENGTH    17

static int mi300_vbios_special_version_check(struct amdgv_adapter *adapt, uint8_t *img)
{
	int i, j, k;
	/* Build_num is defined as 8 bytes length. When the build_num is lower than 8
	 * bytes, there will be 0x00 added in the end until 8 bytes is used, which is
	 * not part of the anchor.
	 */
	int build_num_length;
	uint8_t anchor[] = { 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00 };
	uint8_t build_num[8] = { '\0' };
	uint8_t part_info[128] = { '\0' };
	uint8_t build_date[18] = { '\0' };

	/* search for the first 2k vbios space*/
	for (i = 0; i < 2048 - sizeof(anchor); i++) {
		for (j = 0; j < sizeof(anchor) && ((i + j) < 2048); j++) {
			if (anchor[j] != img[i + j])
				break;
		}

		if (j == sizeof(anchor)) {
			AMDGV_INFO("found anchor at %d\n", i);

			for (k = 0; k < 128; k++) {
				if (img[i + j + k] == '\\' || img[i + j + k] == 0x0)
					break;
			}

			for (build_num_length = 0; build_num_length < BUILD_NUM_MAX_LENGTH;
			     build_num_length++) {
				if (img[i - BUILD_NUM_MAX_LENGTH + build_num_length] == 0x0)
					break;
			}

			if (k < 128) {
				oss_memcpy(build_num, img + i - BUILD_NUM_MAX_LENGTH,
					   build_num_length);
				AMDGV_INFO("build num: %s\n", build_num);
				oss_memcpy(part_info, img + i + j, k);
				AMDGV_INFO("part info: %s\n", part_info);
				oss_memcpy(build_date, img + 0x50, BUILD_DATA_LENGTH);
				AMDGV_INFO("build date: %s\n", build_date);
				return 0;
			}
		}
	}

	return AMDGV_FAILURE;
}

static int mi300_vbios_read_rom_from_reg(struct amdgv_adapter *adapt, uint8_t *bios,
					 uint32_t length_bytes)
{
	if (!oss_pci_read_rom(adapt->dev, bios, &adapt->rom_size, length_bytes))
		return AMDGV_FAILURE;
	return 0;
}

static void mi300_vbios_program_asic_golden(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* Some golden settings registers are designed as PF only,
	 * VF is not able to write them. Program them here from PF.
	 */
	mi300_sdma_program_golden_settings(adapt);
	mi300_gfx_program_golden_settings(adapt);

	tmp = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_CHICKEN_MI300));
	tmp = REG_SET_FIELD(tmp, IH_CHICKEN, MC_SPACE_GPA_ENABLE, 1);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_CHICKEN_MI300), tmp);

	return;
}

static int mi300_enable_pci_atomic_request(struct amdgv_adapter *adapt)
{
	uint16_t val;
	int pos;

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID__PCIE);

	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID__PCIE);
		return AMDGV_FAILURE;
	}

	oss_pci_read_config_word(adapt->dev, pos + PCIE_DEVICE_CONTROL2, &val);
	val |= (PCIE_DEVICE_CONTROL2__ATOMICOP_REQ);
	oss_pci_write_config_word(adapt->dev, pos + PCIE_DEVICE_CONTROL2, val);

	oss_pci_read_config_word(adapt->dev, pos + PCIE_DEVICE_CAP2, &val);
	adapt->pcie_atomic_ops_support_flags =
		val & (PCIE_DEVICE_CAP2__ATOMIC_COMP32 | PCIE_DEVICE_CAP2__ATOMIC_COMP64);

	AMDGV_INFO("Atomic Request Enabled\n");

	return 0;
}

static void mi300_disable_pci_atomic_request(struct amdgv_adapter *adapt)
{
	uint16_t val;
	int pos;

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID_EXP);

	oss_pci_read_config_word(adapt->dev, pos + PCI_EXP_DEVCTL2, &val);
	val &= ~(PCI_EXP_DEVCTL2_ATOMICOP_REQ);
	oss_pci_write_config_word(adapt->dev, pos + PCI_EXP_DEVCTL2, val);
	AMDGV_INFO("Atomic Request Disabled\n");
}

static void mi300_mc_location_setting(struct amdgv_adapter *adapt)
{
	adapt->mc_fb_loc_addr =
		RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, 0), regMC_VM_FB_LOCATION_BASE));
	adapt->mc_fb_loc_addr =
		REG_GET_FIELD(adapt->mc_fb_loc_addr, MC_VM_FB_LOCATION_BASE, FB_BASE);
	adapt->mc_fb_loc_addr <<= MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;
	AMDGV_INFO("MC base is at 0x%llx\n", adapt->mc_fb_loc_addr);

	adapt->mc_fb_top_addr =
		RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, 0), regMC_VM_FB_LOCATION_TOP));
	adapt->mc_fb_top_addr =
		REG_GET_FIELD(adapt->mc_fb_top_addr, MC_VM_FB_LOCATION_TOP, FB_TOP);
	adapt->mc_fb_top_addr <<= MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;
	adapt->mc_fb_top_addr |= ((1 << MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT) - 1);

	adapt->mc_agp_loc_addr = adapt->mc_fb_top_addr + 1;
	adapt->mc_agp_top_addr = adapt->mc_agp_loc_addr + AMDGV_AGP_APERTURE_SIZE - 1;
	if (adapt->flags & AMDGV_FLAG_USE_PF) {
		/* No AGP platform */
		adapt->mc_agp_top_addr = adapt->mc_agp_loc_addr = adapt->mc_fb_top_addr;
	}

	adapt->mc_sys_loc_addr = adapt->mc_fb_loc_addr;
	adapt->mc_sys_top_addr = adapt->mc_agp_top_addr;

	AMDGV_DEBUG("FB MC top is at 0x%llx\n", adapt->mc_fb_top_addr);
	AMDGV_DEBUG("SYS MC base is at 0x%llx\n", adapt->mc_sys_loc_addr);
	AMDGV_DEBUG("SYS MC top is at 0x%llx\n", adapt->mc_sys_top_addr);
	AMDGV_DEBUG("AGP MC base is at 0x%llx\n", adapt->mc_agp_loc_addr);
	AMDGV_DEBUG("AGP MC top is at 0x%llx\n", adapt->mc_agp_top_addr);
}

static void mi300_init_tcp_config(struct amdgv_adapter *adapt)
{
}

static void mi300_emu_device_mode_init(struct amdgv_adapter *adapt)
{
	switch (adapt->dev_id) {
	case 0x369:
	case 0x371:
	case 0x14DC:
	case 0x0070:
	case 0x0071:
		adapt->flags |= AMDGV_FLAG_SIM_MODE;
		adapt->flags |= AMDGV_FLAG_EMU_MODE;
		break;
	default:
		break;
	}
}

static int mi300_vbios_early_sw_init(struct amdgv_adapter *adapt)
{
	const char *name;

	switch (adapt->dev_id) {
	case (0x74A9):
		name = "MI300-HF";
		break;
	case (0x74A1):
		name = "MI300X-O";
		break;
	case (0x74A5):
		name = "MI325X";
		break;
	case (0x74A2):
	case (0x74B6):
	case (0x74A8):
	case (0x74BC):
		name = "MI308X";
		break;
	default:
		name = "MI300X";
		break;
	}

	/* Hardcode ASIC info */
	oss_memcpy(adapt->config.name, name, oss_strlen(name));

	adapt->vbios.is_atom_fw = true;

	mi300_emu_device_mode_init(adapt);

	adapt->fw_load_type = AMDGV_FW_LOAD_BY_GIM;
	adapt->vbios.read_rom_from_reg = mi300_vbios_read_rom_from_reg;
	adapt->vbios.special_version_check = mi300_vbios_special_version_check;

	adapt->smuio.funcs = &smuio_v13_0_3_funcs;

	/* MM capability should be retrieved from vbios later */
	adapt->max_mm_bandwidth[AMDGV_HEVC_ENGINE] = 2 << 20;
	adapt->max_mm_bandwidth[AMDGV_HEVC1_ENGINE] = 2 << 20;

	adapt->config.gfx.major = 9;
	adapt->config.gfx.minor = 0;

	adapt->config.caps.supported_fields_flags = 0;

	return amdgv_vbios_atom_sw_init(adapt);
}

static int mi300_vbios_late_sw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_vbios_early_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_vbios_atom_sw_fini(adapt);
	return 0;
}

static int mi300_vbios_late_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_vbios_early_hw_init(struct amdgv_adapter *adapt)
{
	int r;

	if (!in_whole_gpu_reset()) {
		/* read rom */
		r = amdgv_vbios_read_img(adapt);
		if (r)
			return r;
	}

	/* atom parser init */
	r = amdgv_vbios_atom_hw_init(adapt);
	if (r)
		goto failed;

	/* post vbios */
	if (!mi300_nbio_vbios_need_post(adapt)) {
		r = AMDGV_FAILURE;

		if (mi300_psp_wait_sos_loaded_status(adapt))
			if (mi300_smu_get_fw_loaded_status(adapt))
				r = mi300_reset_trigger_whole_gpu_reset(adapt);

		if (r)
			goto failed;
	}

	r = amdgv_atomfirmware_post(adapt, VBIOS_POST_ASIC_INIT);
	if (r)
		goto failed;

	AMDGV_INFO("VBIOS posted successfully.\n");

	mi300_clear_dummy_mode_after_reset(adapt);

	adapt->flags |= AMDGV_FLAG_GC_REG_RLC_EN;
	adapt->flags |= AMDGV_FLAG_IH_REG_PSP_EN;

	if (mi300_smu_get_fw_loaded_status(adapt)) {
		AMDGV_INFO("SMU fw ready and responding\n");
		/* RLCg needs to handshake with MP5(XCD) firmware if SMU is alived */
		if (adapt->flags & AMDGV_FLAG_EMU_MODE)
			mi300_gfx_rlc_smu_handshake_cntl(adapt, false);
		else
			mi300_gfx_rlc_smu_handshake_cntl(adapt, true);
	} else {
		AMDGV_WARN("SMU fw not responding\n");
	}

	mi300_mc_location_setting(adapt);

	return 0;

failed:
	amdgv_vbios_atom_hw_fini(adapt);

	return r;
}

static int mi300_vbios_late_hw_init(struct amdgv_adapter *adapt)
{
	int r;
	int doorbell_index;
	int i;

	r = amdgv_atomfirmware_set_fw_usage_fb_guest(adapt);
	if (r) {
		AMDGV_ERROR("set guest fw usage fb failed!\n");
		return r;
	}

	r = amdgv_atomfirmware_get_vram_info(adapt);
	if (r) {
		AMDGV_ERROR("get vram info failed!\n");
		return r;
	}

	mi300_nbio_get_vram_vendor(adapt);

	mi300_vbios_program_asic_golden(adapt);
	mi300_init_tcp_config(adapt);

	mi300_nbio_enable_doorbell_aperture(adapt, true);
	mi300_nbio_set_doorbell_fence(adapt);
	if (adapt->asic_type == CHIP_MI308X)
		mi308_nbio_assign_sdma_to_vf(adapt);
	else
		mi300_nbio_assign_sdma_to_vf(adapt);
	mi300_nbio_assign_vcn_to_vf(adapt);

	for (i = 0; i < adapt->sdma.num_instances; i++) {
		doorbell_index = AMDGV_MI300_DOORBELL_sDMA_ENGINE0 + i * 10;
		doorbell_index = doorbell_index << 1;
		mi300_nbio_assign_sdma_doorbell(adapt, i, doorbell_index, 20);
	}
	mi300_nbio_assign_mmsch_doorbell(adapt);

	gfxhub_v1_2_gart_enable(adapt);

	mmhub_v1_8_gart_enable(adapt);

	r = mi300_enable_pci_atomic_request(adapt);
	if (r) {
		AMDGV_ERROR("enable pci atomic request failed!\n");
		return r;
	}

	mi300_nbio_enable_vf_access_mmio_over_512k(adapt);

	mi300_nbio_disable_vf_flr(adapt);

	return 0;
}

static int mi300_vbios_early_hw_fini(struct amdgv_adapter *adapt)
{
	amdgv_vbios_atom_hw_fini(adapt);

	if (!in_whole_gpu_reset()) {
		mi300_disable_pci_atomic_request(adapt);

		mi300_nbio_enable_doorbell_aperture(adapt, false);

		gfxhub_v1_2_gart_fini(adapt);

		mmhub_v1_8_fini(adapt);

		if (adapt->vbios.image) {
			oss_free_memory(adapt->vbios.image);
			adapt->vbios.image = NULL;
		}

		if (adapt->vbios.guest_image) {
			oss_free_memory(adapt->vbios.guest_image);
			adapt->vbios.guest_image = NULL;
		}
	}

	return 0;
}

static int mi300_vbios_late_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi300_vbios_early_func = {
	.name = "mi300_vbios_early_func",
	.sw_init = mi300_vbios_early_sw_init,
	.sw_fini = mi300_vbios_early_sw_fini,
	.hw_init = mi300_vbios_early_hw_init,
	.hw_fini = mi300_vbios_early_hw_fini,
};

const struct amdgv_init_func mi300_vbios_late_func = {
	.name = "mi300_vbios_late_func",
	.sw_init = mi300_vbios_late_sw_init,
	.sw_fini = mi300_vbios_late_sw_fini,
	.hw_init = mi300_vbios_late_hw_init,
	.hw_fini = mi300_vbios_late_hw_fini,
};
