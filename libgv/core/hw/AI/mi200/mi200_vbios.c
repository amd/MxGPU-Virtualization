/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv.h>

#include "mi200.h"
#include "atombios/atomfirmware.h"
#include <atombios/atom.h>
#include "atombios/atombios.h"
#include "mmhub_v1_7.h"
#include "gfxhub_v1_0.h"
#include "psp_v13_0.h"

#include "mi200_golden_settings.h"
#include "mi200_reset.h"
#include "mi200_ip_discovery.h"
#include "mi200_powerplay.h"
#include "smuio_v13_0.h"
#include "gfx_v9_0.h"
#include "mi200_xgmi.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static uint32_t mi200_vbios_get_fw_version(struct amdgv_adapter *adapt,
		uint8_t fw_type, uint32_t *version)
{
	uint32_t offset;

	if (version == NULL)
		return AMDGV_FAILURE;

	offset = amdgv_atombios_get_fw_offset(adapt, fw_type);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_INDEX),
			offset + VBIOS_FW_VERSION_OFFSET);
	*version = RREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_DATA));

	return 0;
}

static bool mi200_vbios_need_post(struct amdgv_adapter *adapt)
{
	uint32_t val;

	val = RREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_7));

	AMDGV_INFO("BIOS_SCRATCH_7 = 0x%08x\n", val);

	val &= ATOM_ASIC_INIT_COMPLETE;

	if (val) {
		AMDGV_INFO("ATOM_ASIC_POSTED\n");
		return false;
	}

	AMDGV_INFO("ATOM_ASIC_NEED_POST\n");
	return true;
}

static int mi200_vbios_read_rom_from_reg(struct amdgv_adapter *adapt,
					   uint8_t *bios,
					   uint32_t length_bytes)
{
	uint32_t *vbios_rom;
	unsigned int i, length_in_dword;

	vbios_rom = (uint32_t *)bios;
	length_in_dword = length_bytes / 4;

	AMDGV_DEBUG("Reading VBIOS from ROM\n");

	WREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_INDEX), 0);

	for (i = 0; i < length_in_dword; i++)
		vbios_rom[i] = RREG32(SOC15_REG_OFFSET(SMUIO, 0, regROM_DATA));

	return 0;
}

static void mi200_vbios_program_asic_golden(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* On Mi200, some golden settings registers are designed as PF only,
	 * KMD in VF is not able to write them. Program them here from PF.
	 */
	mi200_sdma_program_golden_settings(adapt);
	mi200_gfx_program_golden_settings(adapt);

	tmp = RREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_CHICKEN_MI200));
	tmp = REG_SET_FIELD(tmp, IH_CHICKEN, MC_SPACE_GPA_ENABLE, 1);
	WREG32(SOC15_REG_OFFSET(OSSSYS, 0, mmIH_CHICKEN_MI200), tmp);

	/* Disable FB Protection by default */
	WREG32(mmnbif_gpu_VF_FB_EN, 0x7fffffff);

	return;
}

static int mi200_enable_pci_atomic_request(struct amdgv_adapter *adapt)
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
	adapt->pcie_atomic_ops_support_flags = val & (PCIE_DEVICE_CAP2__ATOMIC_COMP32 | PCIE_DEVICE_CAP2__ATOMIC_COMP64);

	AMDGV_INFO("Atomic Request Enabled\n");

	return 0;
}

static void mi200_disable_pci_aer(struct amdgv_adapter *adapt)
{
	int pos;

	pos = oss_pci_find_ext_cap(adapt->dev, PCI_EXT_CAP_ID_ERR);
	if (!pos) {
		AMDGV_ERROR("this device does not support ext capability: %x\n", PCI_EXT_CAP_ID_ERR);
		return;
	}
	oss_pci_write_config_dword(adapt->dev, pos + PCI_ERR_UNCOR_MASK,
				0xffffffff);
	AMDGV_INFO("Disable pci aer\n");
}

static void mi200_assign_sdma_doorbell(struct amdgv_adapter *adapt)
{
	uint32_t instance, doorbell_range, reg;

	for (instance = 0; instance < 5; instance++) {
		if (instance < 2)
			reg = instance + SOC15_REG_OFFSET(NBIO, 0,
				mmBIF_SDMA0_DOORBELL_RANGE);
		else if (instance == 4)
			reg = instance + 0x4 + 0x1 +
				SOC15_REG_OFFSET(NBIO, 0,
						 mmBIF_SDMA0_DOORBELL_RANGE);
		else
			reg = instance + 0x4 + SOC15_REG_OFFSET(NBIO, 0,
				mmBIF_SDMA0_DOORBELL_RANGE);

		doorbell_range = RREG32(reg);
		doorbell_range = REG_SET_FIELD(doorbell_range,
			BIF_SDMA0_DOORBELL_RANGE, OFFSET,
			(AMDGV_MI200_DOORBELL_sDMA_ENGINE0 + 0xa * instance)
			<< 1);
		doorbell_range = REG_SET_FIELD(doorbell_range,
			BIF_SDMA0_DOORBELL_RANGE, SIZE, 20);

		AMDGV_DEBUG("mmBIF_SDMA%d_DOORBELL_RANGE = 0x%x\n", instance,
			    doorbell_range);
		WREG32(reg, doorbell_range);
	}
}

static void mi200_assign_mmsch_doorbell(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* mmBIF_MMSCH0_DOORBELL_RANGE */
	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIF_MMSCH0_DOORBELL_RANGE));
	tmp = REG_SET_FIELD(tmp, BIF_MMSCH0_DOORBELL_RANGE, OFFSET,
			    AMDGV_MI200_DOORBELL_MMSCH0 << 1);
	tmp = REG_SET_FIELD(tmp, BIF_MMSCH0_DOORBELL_RANGE, SIZE, 8);
	AMDGV_DEBUG("mmmmBIF_MMSCH0_DOORBELL_RANGE = 0x%x\n", tmp);

	WREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIF_MMSCH0_DOORBELL_RANGE), tmp);
}

static uint32_t mi200_gfx_create_bitmask(uint32_t bit_width)
{
	return (uint32_t)((1ULL << bit_width) - 1);
}

static uint32_t mi200_gfx_get_cu_active_bitmap(struct amdgv_adapter *adapt)
{
	uint32_t data, mask;

	data = RREG32(SOC15_REG_OFFSET(GC, 0, mmCC_GC_SHADER_ARRAY_CONFIG));
	data |= RREG32(SOC15_REG_OFFSET(GC, 0, mmGC_USER_SHADER_ARRAY_CONFIG));

	data &= CC_GC_SHADER_ARRAY_CONFIG__INACTIVE_CUS_MASK;
	data >>= CC_GC_SHADER_ARRAY_CONFIG__INACTIVE_CUS__SHIFT;

	mask = mi200_gfx_create_bitmask(adapt->config.gfx.max_cu_per_sh);

	return (~data) & mask;
}

static uint32_t mi200_gfx_get_cu_count(struct amdgv_adapter *adapt)
{
	uint32_t cu_count = 0;
	uint32_t i, j, k;
	uint32_t mask, bitmap;

	for (i = 0; i < adapt->config.gfx.max_shader_engines; i++) {
		for (j = 0; j < adapt->config.gfx.max_sh_per_se; j++) {
			mask = 1;
			mi200_gfx_select_se_sh(adapt, i, j, ~0);
			bitmap = mi200_gfx_get_cu_active_bitmap(adapt);

			for (k = 0; k < adapt->config.gfx.max_cu_per_sh; k++) {
				if (bitmap & mask)
					cu_count++;
				mask <<= 1;
			}

		}
	}

	mi200_gfx_select_se_sh(adapt, ~0, ~0, ~0);
	return cu_count;
}

static int mi200_init_xgmi_info(struct amdgv_adapter *adapt)
{
	uint32_t xgmi_lfb_cntl = 0, node_seg_size;
	uint32_t max_region = 0;
	struct amdgv_xgmi *xgmi = &adapt->xgmi;

	xgmi_lfb_cntl = RREG32(SOC15_REG_OFFSET(GC, 0,
						mmMC_VM_XGMI_LFB_CNTL_ALDE));
	max_region = REG_GET_FIELD(xgmi_lfb_cntl, MC_VM_XGMI_LFB_CNTL_ALDE,
				   PF_MAX_REGION);
	AMDGV_DEBUG("xgmi_lfb_cntl = 0x%x\n", RREG32(SOC15_REG_OFFSET(GC, 0,
		mmMC_VM_XGMI_LFB_CNTL_ALDE)));

	if (max_region) {

		if (xgmi->fb_sharing_mode > MI200_XGMI_MAX_SUPPORTED_MODE) {
			AMDGV_ERROR("Unsupported XGMI FB Sharing Mode\n");
			return AMDGV_FAILURE;
		}

		xgmi->phy_nodes_num = max_region + 1;

		if (xgmi->phy_nodes_num > 8)
			return -1;

		xgmi->phy_node_id = REG_GET_FIELD(
			xgmi_lfb_cntl, MC_VM_XGMI_LFB_CNTL_ALDE, PF_LFB_REGION);
		if (xgmi->phy_node_id > 7)
			return -1;

		xgmi->socket_id = REG_GET_FIELD(RREG32_SOC15(SMUIO, 0, regSMUIO_MCM_CONFIG),
							SMUIO_MCM_CONFIG, SOCKET_ID);
		node_seg_size =
			RREG32(SOC15_REG_OFFSET(GC, 0,
						mmMC_VM_XGMI_LFB_SIZE_ALDE));
		xgmi->node_segment_size = REG_GET_FIELD(node_seg_size,
			MC_VM_XGMI_LFB_SIZE_ALDE, PF_LFB_SIZE) << 24;

		AMDGV_INFO("xgmi: nodes_num = %d id = %d segment_size = 0x%llx fb_sharing_mode = 0x%x\n",
			xgmi->phy_nodes_num, xgmi->phy_node_id,
			xgmi->node_segment_size,
			xgmi->fb_sharing_mode);
	}

	if (adapt->smuio.funcs)
		xgmi->connected_to_cpu =
			adapt->smuio.funcs->is_host_gpu_xgmi_supported(adapt);

	/* till now, only mi200 should set mb in hive*/
	xgmi->set_mb_in_hive = true;
	return 0;
}

static void mi200_mc_location_setting(struct amdgv_adapter *adapt)
{
	adapt->mc_fb_loc_addr = RREG32(SOC15_REG_OFFSET(GC, 0,
				       mmMC_VM_FB_LOCATION_BASE));
	adapt->mc_fb_loc_addr = REG_GET_FIELD(adapt->mc_fb_loc_addr,
					      MC_VM_FB_LOCATION_BASE, FB_BASE);
	adapt->mc_fb_loc_addr <<= MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;
	AMDGV_INFO("MC base is at 0x%llx\n", adapt->mc_fb_loc_addr);

	adapt->mc_fb_top_addr = RREG32(SOC15_REG_OFFSET(GC, 0, mmMC_VM_FB_LOCATION_TOP));
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

static void mi200_setup_common_timeout(struct amdgv_adapter *adapt)
{
	/* PSP */
	AMDGV_TIMEOUT(TIMEOUT_PSP_REG) = 1000 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_PSP_MEM) = 1000 * 1000 * 2;
	/* SMU */
	AMDGV_TIMEOUT(TIMEOUT_SMU_REG) = 5000 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_SMU_IND_REG) = 200 * 1000;
	/* RESET */
	AMDGV_TIMEOUT(TIMEOUT_RESET) = 100 * 1000;
	/* STATUS */
	AMDGV_TIMEOUT(TIMEOUT_STATUS_REG) = 50 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_GRBM_STATUS) = 100 * 1000;
	/* COMMAND */
	AMDGV_TIMEOUT(TIMEOUT_CMD_RESP) = 200 * 1000;
	/* CP_DMA */
	AMDGV_TIMEOUT(TIMEOUT_CP_DMA) = 20 * 1000;
	/* READ VBIOS */
	AMDGV_TIMEOUT(TIMEOUT_READ_VBIOS) = 5 * 1000 * 1000;
	/* MANUAL SWITCH */
	AMDGV_TIMEOUT(TIMEOUT_MANUAL_SWITCH) = 500 * 1000;
	/* AUTO SWITCH (MMSCH) */
	AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_MM) = 50 * 1000;
	/* GUEST IDH */
	AMDGV_TIMEOUT(TIMEOUT_GUEST_IDH_RESP) = 10 * 1000;
	AMDGV_TIMEOUT(TIMEOUT_GUEST_IDH_RESP_GPU_RESET) = 500 * 1000;
	/* PCI PENDING TRANSACTION */
	AMDGV_TIMEOUT(TIMEOUT_PCI_TRANS) = 400;
	/* BACO Hardware recovery */
	AMDGV_TIMEOUT(TIMEOUT_BACO_HW) = 2 * 1000 * 1000;
}

static void mi200_init_tcp_config(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	bool hash_64k;
	bool hash_2m;
	bool hash_1g;

	// get FB channel number
	tmp = RREG32_SOC15(DF, 0, mmDF_GCM_AON0_DramMegaBaseAddress0);
	tmp &= ALDEBARAN_DF_CS_UMC_AON0_DramBaseAddress0__IntLvNumChan_MASK;
	tmp >>= DF_CS_UMC_AON0_DramBaseAddress0__IntLvNumChan__SHIFT;

	// DF hash is enabled only when the FB channel number is 0x1e
	if (tmp == 0x1e) {
		// get DF hash settings
		tmp = RREG32_SOC15(DF, 0, mmDF_CS_UMC_AON0_DfGlobalCtrl);
		hash_64k = REG_GET_FIELD(tmp, DF_CS_UMC_AON0_DfGlobalCtrl,
					 GlbHashIntlvCtl64K);
		hash_2m = REG_GET_FIELD(tmp, DF_CS_UMC_AON0_DfGlobalCtrl,
					GlbHashIntlvCtl2M);
		hash_1g = REG_GET_FIELD(tmp, DF_CS_UMC_AON0_DfGlobalCtrl,
					GlbHashIntlvCtl1G);
		AMDGV_DEBUG("DF hash settings:64K[%x] 2M[%x] 1G[%x]\n",
			    hash_64k, hash_2m, hash_1g);

		// apply hash settings to TCP to match DF hash settings
		tmp = RREG32_SOC15(GC, 0, mmTCP_ADDR_CONFIG);
		tmp = REG_SET_FIELD(tmp, TCP_ADDR_CONFIG, ENABLE64KHASH,
				    hash_64k);
		tmp = REG_SET_FIELD(tmp, TCP_ADDR_CONFIG, ENABLE2MHASH,
				    hash_2m);
		tmp = REG_SET_FIELD(tmp, TCP_ADDR_CONFIG, ENABLE1GHASH,
				    hash_1g);
		WREG32_SOC15(GC, 0, mmTCP_ADDR_CONFIG, tmp);
	}
}

static int mi200_vbios_early_sw_init(struct amdgv_adapter *adapt)
{
	const char *name = "MI200";

	/* Hardcode ASIC info */
	oss_memcpy(adapt->config.name, name, oss_strlen(name));

	adapt->vbios.is_atom_fw = true;

	adapt->fw_load_type = AMDGV_FW_LOAD_BY_GIM;
	adapt->vbios.read_rom_from_reg = mi200_vbios_read_rom_from_reg;

	mi200_setup_common_timeout(adapt);

	adapt->smuio.funcs = &smuio_v13_0_funcs;

	mi200_ip_discovery_init(adapt);

	/* MM capability should be retrieved from vbios later */
	adapt->max_mm_bandwidth[AMDGV_HEVC_ENGINE] = 2 << 20;
	adapt->max_mm_bandwidth[AMDGV_HEVC1_ENGINE] = 2 << 20;

	adapt->config.caps.supported_fields_flags = 0;

	return amdgv_vbios_atom_sw_init(adapt);
}

static int mi200_vbios_late_sw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi200_vbios_early_sw_fini(struct amdgv_adapter *adapt)
{
	mi200_ip_discovery_fini(adapt);
	amdgv_vbios_atom_sw_fini(adapt);
	return 0;
}

static int mi200_vbios_late_sw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi200_vbios_early_hw_init(struct amdgv_adapter *adapt)
{
	int r;
	uint32_t ver = 0;

	if (!in_whole_gpu_reset()) {
		r = mi200_init_xgmi_info(adapt);
		if (r)
			return r;
	}

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
	if (!mi200_vbios_need_post(adapt)) {
		if (psp_v13_wait_sos_loaded_status(adapt) &&
		    mi200_smu_13_0_get_fw_loaded_status(adapt) &&
			adapt->xgmi.phy_nodes_num < 2) {
			r = amdgv_reset_hw_for_reload(adapt, false);
			if (r)
				goto failed;
		} else
			goto failed;
	}

	r = amdgv_atomfirmware_post(adapt, VBIOS_POST_ASIC_INIT);
	if (r)
		goto failed;

	AMDGV_INFO("VBIOS posted successfully.\n");

	mi200_clear_dummy_mode_after_reset(adapt);

	adapt->flags |= AMDGV_FLAG_GC_REG_RLC_EN;

	mi200_vbios_get_fw_version(adapt,
		PSP_DIR_ENTRY_TYPE_PSP_SEC_POLICY_STAGE2, &ver);

	adapt->vbios.sec_version = ver;

	return 0;

failed:
	amdgv_vbios_atom_hw_fini(adapt);

	return r;
}

static int mi200_vbios_late_hw_init(struct amdgv_adapter *adapt)
{
	int r;
	uint32_t cp_debug; //interrupt_status;

	r = amdgv_atombios_get_gfx_info(adapt);
	if (r) {
		AMDGV_ERROR("get gfx info failed!\n");
		return r;
	}

	adapt->config.gfx.active_cu_count = mi200_gfx_get_cu_count(adapt);

	mi200_mc_location_setting(adapt);

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

	/* cp debug, it prevents page fault. */
	cp_debug = RREG32(mmCP_DEBUG);
	cp_debug = cp_debug | 0x8000;
	WREG32(mmCP_DEBUG, cp_debug);

	mi200_vbios_program_asic_golden(adapt);
	mi200_init_tcp_config(adapt);

	mi200_assign_sdma_doorbell(adapt);
	mi200_assign_mmsch_doorbell(adapt);

	gfxhub_v1_0_gart_enable(adapt);

	mmhub_v1_7_init(adapt);

	r = mi200_enable_pci_atomic_request(adapt);
	if (r)
		return r;

	mi200_disable_pci_aer(adapt);

	return 0;
}

static int mi200_vbios_early_hw_fini(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	int ret = 0;
	amdgv_vbios_atom_hw_fini(adapt);

	if (!in_whole_gpu_reset()) {
		gfxhub_v1_0_gart_fini(adapt);

		mmhub_v1_7_fini(adapt);

		if (adapt->vbios.image) {
			oss_free_memory(adapt->vbios.image);
			adapt->vbios.image = NULL;
		}

		if (adapt->vbios.guest_image) {
			oss_free_memory(adapt->vbios.guest_image);
			adapt->vbios.guest_image = NULL;
		}

		if ((adapt->xgmi.phy_nodes_num > 1) &&
			(!adapt->reset.in_xgmi_chain_reset)) {
			/* stop sched before mode1 reset */
			amdgv_sched_stop_all(adapt);

			ret = mi200_mode1_reset(adapt);

			/* clear VBIOS status */
			tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_7));
			tmp &= ~ATOM_ASIC_INIT_COMPLETE;
			WREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_7), tmp);
		}
	}

	return 0;
}

static int mi200_vbios_late_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi200_vbios_early_func = {
	.name = "mi200_vbios_early_func",
	.sw_init = mi200_vbios_early_sw_init,
	.sw_fini = mi200_vbios_early_sw_fini,
	.hw_init = mi200_vbios_early_hw_init,
	.hw_fini = mi200_vbios_early_hw_fini,
};

const struct amdgv_init_func mi200_vbios_late_func = {
	.name = "mi200_vbios_late_func",
	.sw_init = mi200_vbios_late_sw_init,
	.sw_fini = mi200_vbios_late_sw_fini,
	.hw_init = mi200_vbios_late_hw_init,
	.hw_fini = mi200_vbios_late_hw_fini,
};
