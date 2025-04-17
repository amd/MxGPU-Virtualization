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

#include <amdgv_device.h>
#include <amdgv_pci_def.h>
#include <amdgv_gpuiov.h>
#include <amdgv_irqmgr.h>
#include <atombios/atomfirmware.h>
#include <atombios/atom.h>
#include <atombios/atombios.h>
#include <amdgv_sched_internal.h>

#include "mi300.h"
#include "mi300_nbio.h"
#include "mi300_gfx.h"
#include "mi300_irqmgr.h"
#include "mi300_gpuiov.h"
#include "mi300_golden_settings.h"
#include "mi300_reset.h"
#include "mi300_powerplay.h"
#include "mi300_clockgating.h"

#include "mi300/NBIO/nbio_7_9_0_offset.h"
#include "mi300/NBIO/nbio_7_9_0_sh_mask.h"
#include "mi300_xgmi.h"
#include "gfxhub_v1_2.h"

#define PCI_CONFIG_SIZE 1024
#define MI300_MAX_VF_NUM 8

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;
static int mi300_reset_trigger_pf_soft_flr(struct amdgv_adapter *adapt);

struct pf_pcie_restore {
	int offset;
	int size;
	char *name;
};

struct mi300_whole_gpu_reset_state {
	uint32_t bif_bx_strap0;

	uint32_t pf_pm_ctl;
	uint16_t pf_misx_ctl;

	uint32_t pf_pci_cfg[PCI_CONFIG_SIZE];
	uint32_t pf_msix_tab[12];

	uint32_t vf_pci_cfg[MI300_MAX_VF_NUM][PCI_CONFIG_SIZE];
	uint32_t vf_msix_tab[MI300_MAX_VF_NUM][12];

	struct mi300_irqmgr_reset_state irqmgr_reset_state;
};

/* list all writeable cfgs need to be restored and in a correct sequence */
static struct pf_pcie_restore restore_tbl[] = {
	{ 0x0004, 2, "Command_Epf" },
	{ 0x0006, 2, "Status" },
	{ 0x000c, 1, "Cache Line" },
	{ 0x000d, 1, "Latency" },
	{ 0x000e, 1, "Header" },
	{ 0x000f, 1, "BIST" },
	{ 0x0010, 4, "BAR1" },
	{ 0x0014, 4, "BAR2" },
	{ 0x0018, 4, "BAR3" },
	{ 0x001c, 4, "BAR4" },
	{ 0x0020, 4, "BAR5" },
	{ 0x0024, 4, "BAR6" },

	{ 0x0030, 4, "ROM Base Address" },
	{ 0x0034, 4, "Cap Pointer" },

	{ 0x003c, 1, "Int Line" },
	{ 0x003d, 1, "Int Pin" },
	{ 0x003e, 1, "Min Grant" },
	{ 0x003f, 1, "Max Latency" },

	{ 0x0048, 4, "Vendor Cap List" },
	{ 0x004c, 4, "Adapter Id Master-write" },
	{ 0x0052, 2, "PMI Cap" },
	{ 0x0054, 4, "PMI Status Control" },

	{ 0x006C, 2, "Device Cntl" },
	{ 0x0074, 2, "Link Cntl" },

	{ 0x008C, 2, "Device Cntl2" },
	{ 0x0094, 2, "Link Cntl2" },
	{ 0x009C, 2, "Slot Cntl2" },

	/* enable interrupt */
	{ 0x00c2, 2, "MSIX Msg Cntl" },

#define CAP_SRIOV_OFFSET 0x330
	/* CAP SRIOV */
	{ 0x334, 4, "SR-IOV Capabilities" },
	{ 0x33a, 2, "SR-IOV Status" },
	{ 0x33c, 2, "SR-IOV Initial VFs" },
	{ 0x33e, 2, "SR-IOV Total VFs" },
	{ 0x340, 2, "SR-IOV Num VFs" },
	{ 0x342, 2, "SR-IOV Func Dep Link" },
	{ 0x344, 2, "SR-IOV First Offset" },
	{ 0x346, 2, "SR-IOV Stride" },
	{ 0x34a, 2, "SR-IOV VF Device ID" },
	{ 0x34c, 4, "SR-IOV Sup PageSize" },
	{ 0x350, 4, "SR-IOV Sys PageSize" },
	{ 0x354, 4, "SR-IOV VF BAR0" },
	{ 0x358, 4, "SR-IOV VF BAR1" },
	{ 0x35c, 4, "SR-IOV VF BAR2" },
	{ 0x360, 4, "SR-IOV VF BAR3" },
	{ 0x364, 4, "SR-IOV VF BAR4" },
	{ 0x368, 4, "SR-IOV VF BAR5" },
	{ 0x36c, 4, "SR-IOV VF Migration State Array Offset" },

	/* enable bit is set AFTER setting up SRIOV capabilities. */
	{ 0x338, 2, "SR-IOV Control" },

};

struct engine_reg_info mi300_engine_reg_info[NUM_ENGINES] = {
	[RLC_STAT] = {
			GC_HWIP, regRLC_STAT, regRLC_STAT_BASE_IDX,
			RLC_STAT__RLC_BUSY__SHIFT, RLC_STAT__RLC_BUSY_MASK,
			0
		},
	[SDMA_STATUS_REG] = {
			SDMA0_HWIP, regSDMA_STATUS_REG, regSDMA_STATUS_REG_BASE_IDX,
			SDMA_STATUS_REG__IDLE__SHIFT, SDMA_STATUS_REG__IDLE_MASK,
			1
		},
	[SDMA_STATUS4_REG] = {
			SDMA0_HWIP, regSDMA_STATUS4_REG, regSDMA_STATUS4_REG_BASE_IDX,
			SDMA_STATUS4_REG__IDLE__SHIFT, SDMA_STATUS4_REG__IDLE_MASK,
			1
		},
	[GRBM_STATUS] = {
			GC_HWIP, regGRBM_STATUS, regGRBM_STATUS_BASE_IDX,
			GRBM_STATUS__GUI_ACTIVE__SHIFT, GRBM_STATUS__GUI_ACTIVE_MASK,
			0
		},
};

struct mi300_reset_access_info {
	uint32_t fb_access_info;
	uint32_t doorbell_access_info;
	uint32_t register_write_access_info;
};

static void restore_reg(struct amdgv_adapter *adapt, struct pf_pcie_restore *entry,
			struct mi300_whole_gpu_reset_state *reset_state)
{
	uint32_t offset;
	uint8_t *val;

	offset = entry->offset;
	val = (uint8_t *)reset_state->pf_pci_cfg;
	val += offset;

	switch (entry->size) {
	case 1:
		oss_pci_write_config_byte(adapt->dev, offset, *val);
		break;

	case 2:
		oss_pci_write_config_word(adapt->dev, offset, *(uint16_t *)val);
		break;

	case 4:
		oss_pci_write_config_dword(adapt->dev, offset, *(uint32_t *)val);
		break;
	}
}

/* a dummy function */
static int mi300_reset_save_vddgfx_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return 0;
}

static void mi300_reset_save_active_vf_idx(struct amdgv_adapter *adapt,
					   struct mi300_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id (world_switch_id,
		     amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->enabled) {
			for_each_id (hw_sched_id, world_switch->hw_sched_mask) {
				amdgv_gpuiov_get_active_vfs(
					adapt, hw_sched_id,
					&vf_state->active_vfs[world_switch->sched_block]);
			}
		}
	}
}

static void mi300_reset_restore_active_vf_idx(struct amdgv_adapter *adapt,
					      struct mi300_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id (world_switch_id,
		     amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->enabled) {
			for_each_id (hw_sched_id, world_switch->hw_sched_mask) {
				amdgv_gpuiov_set_active_vfs(
					adapt, hw_sched_id,
					vf_state->active_vfs[world_switch->sched_block]);
			}
		}
	}
}

static void mi300_reset_save_time_quanta_info(struct amdgv_adapter *adapt,
					      struct mi300_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id (world_switch_id,
		     amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->enabled) {
			for_each_id (hw_sched_id, world_switch->hw_sched_mask) {
				amdgv_gpuiov_get_time_quanta_option(
					adapt, hw_sched_id,
					&vf_state->quanta_option[world_switch->sched_block]);
				amdgv_gpuiov_get_time_quanta_index(
					adapt, vf_state->idx_vf, hw_sched_id,
					&vf_state->quanta_index[world_switch->sched_block]);
			}
		}
	}
}

static void mi300_reset_restore_time_quanta_info(struct amdgv_adapter *adapt,
						 struct mi300_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id (world_switch_id,
		     amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->enabled) {
			for_each_id (hw_sched_id, world_switch->hw_sched_mask) {
				amdgv_gpuiov_set_time_quanta_option(
					adapt, hw_sched_id,
					vf_state->quanta_option[world_switch->sched_block]);
				amdgv_gpuiov_set_time_quanta_index(
					adapt, vf_state->idx_vf, hw_sched_id,
					vf_state->quanta_index[world_switch->sched_block]);
			}
		}
	}
}

static int mi300_reset_save_csa_config(struct amdgv_adapter *adapt, uint32_t *csa)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_CNTXT;
	oss_pci_read_config_dword(adapt->dev, offset, csa);

	return 0;
}

static int mi300_reset_restore_csa_config(struct amdgv_adapter *adapt, uint32_t *csa)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_CNTXT;
	oss_pci_write_config_dword(adapt->dev, offset, *csa);

	return 0;
}

static void mi300_reset_save_access_info(struct amdgv_adapter *adapt,
					 struct mi300_reset_access_info *access_info)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	access_info->fb_access_info = RREG32(mmnbif_gpu_VF_FB_EN);
	access_info->doorbell_access_info = RREG32(mmnbif_gpu_VF_DOORBELL_EN);
	access_info->register_write_access_info = RREG32(mmnbif_gpu_VF_REGWR_EN);
}

static void mi300_reset_restore_access_info(struct amdgv_adapter *adapt,
					    struct mi300_reset_access_info *access_info)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	WREG32(mmnbif_gpu_VF_FB_EN, access_info->fb_access_info);
	WREG32(mmnbif_gpu_VF_DOORBELL_EN, access_info->doorbell_access_info);
	WREG32(mmnbif_gpu_VF_REGWR_EN, access_info->register_write_access_info);
}

static void mi300_reset_enable_mmio_protection(struct amdgv_adapter *adapt)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	WREG32(mmnbif_gpu_VF_FB_EN, 0);
	WREG32(mmnbif_gpu_VF_DOORBELL_EN, 0);
	WREG32(mmnbif_gpu_VF_REGWR_EN, 0);
}

static void mi300_reset_vf_save_and_disable(struct amdgv_adapter *adapt, uint32_t idx_vf,
					    uint32_t *pci_cfg, uint32_t *msix_tab)
{
	uint32_t idx = 0;
	struct amdgv_vf_device *vf = &adapt->array_vf[idx_vf];
	int entry;
	uint32_t *tab;

	/* Save the vf pci cfg space */
	for (idx = 0; idx < PCI_CONFIG_SIZE; idx += 4) {
		oss_pci_read_config_dword(vf->dev, idx, pci_cfg);
		pci_cfg++;
	}

	/* save msix table
	 * based on mi300 hw, the table is located in BAR5 res
	 * and max vector number is 3 */
	if (vf->res_mapped && vf->res.mmio) {
		for (entry = 0; entry < 3; entry++) {
			/* after decoding the MSI-X capability,
			 * the PBA is 5 and table offest is 0x42000 */
			tab = (uint32_t *)vf->res.mmio + 4 * entry +
			      SOC15_REG_OFFSET(NBIO, 0,
					       regRCC_DEV0_EPF0_GFXMSIX_VECT0_ADDR_LO);
			msix_tab[entry * 4] = oss_mm_read32(tab);
			msix_tab[entry * 4 + 1] = oss_mm_read32(tab + 1);
			msix_tab[entry * 4 + 2] = oss_mm_read32(tab + 2);
			msix_tab[entry * 4 + 3] = oss_mm_read32(tab + 3);
		}
	}

	/* disable the vf by clearing the command */
	if (idx_vf != AMDGV_PF_IDX) {
		oss_pci_write_config_word(vf->dev, PCI_COMMAND, PCI_COMMAND_INTX_DISABLE);
	}
}

static void mi300_reset_vf_restore(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint32_t *pci_cfg, uint32_t *msix_tab)

{
	struct amdgv_vf_device *vf = &adapt->array_vf[idx_vf];
	int entry;
	uint32_t *val, *tab;
	uint32_t idx;

	/* Restore the vf pci cfg space */
	for (idx = 0; idx < PCI_CONFIG_SIZE; idx += 4) {
		oss_pci_write_config_dword(vf->dev, idx, *pci_cfg);
		pci_cfg++;
	}

	/* Restore the vf msix table */
	if (vf->res_mapped && vf->res.mmio) {
		/* disable mmio reg write protection */
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE,
					   true);

		for (entry = 0; entry < 3; entry++) {
			tab = (uint32_t *)vf->res.mmio + 4 * entry +
			      SOC15_REG_OFFSET(NBIO, 0,
					       regRCC_DEV0_EPF0_GFXMSIX_VECT0_ADDR_LO);
			val = &msix_tab[entry * 4];

			oss_mm_write32(tab, val[0]);
			oss_mm_write32(tab + 1, val[1]);
			oss_mm_write32(tab + 2, val[2]);
			oss_mm_write32(tab + 3, val[3]);
		}
	}
}

static int mi300_reset_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t strap4;
	uint32_t xcc_mask;
	int wait_ret;
	int ret = 0;
	uint16_t val;

	struct amdgv_vf_device *vf = &adapt->array_vf[idx_vf];
	int pos = oss_pci_find_capability(vf->dev, PCI_CAP_ID_EXP);

	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID_EXP);
		return AMDGV_FAILURE;
	}

	/* disable device bus mastering */
	oss_pci_read_config_word(vf->dev, PCI_COMMAND, &val);
	val &= (~PCI_COMMAND_MASTER);
	oss_pci_write_config_word(vf->dev, PCI_COMMAND, val);

	/* wait for transaction done */
	wait_ret = amdgv_wait_for_pci_cfg(adapt, vf->dev, pos + PCIE_DEVICE_STATUS,
			PCIE_DEVICE_STATUS__TRANS_PEND, 0, 2,
			AMDGV_TIMEOUT(TIMEOUT_PCI_TRANS),
			AMDGV_WAIT_CHECK_EQ, 0);

	if (wait_ret)
		AMDGV_WARN("Abort data transaction on %s for FLR\n", amdgv_idx_to_str(idx_vf));

	/* enable FLR */
	if (!(adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY)) {
		strap4 = RREG32(mmnbif_gpu_RCC_DEV0_EPF0_STRAP4);
		strap4 |= STRAP_FLR_EN_DEV0_F0_MASK;
		WREG32(mmnbif_gpu_RCC_DEV0_EPF0_STRAP4, strap4);
	}

	if (idx_vf == AMDGV_PF_IDX) {
		xcc_mask = amdgv_sched_get_xcc_mask_by_vf(adapt, AMDGV_PF_IDX);
		AMDGV_INFO("Send Mode 3 reset msg with xcc mask = 0x%x on PF", xcc_mask);
		ret = mi300_smu_trigger_mode_3_reset(adapt, xcc_mask);
	} else {
		ret = mi300_smu_trigger_vf_flr(adapt, idx_vf);
	}

	/* disable FLR */
	if (!(adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY)) {
		strap4 = RREG32(mmnbif_gpu_RCC_DEV0_EPF0_STRAP4);
		strap4 &= ~STRAP_FLR_EN_DEV0_F0_MASK;
		WREG32(mmnbif_gpu_RCC_DEV0_EPF0_STRAP4, strap4);
	}

	/* enable bus mastering */
	oss_pci_read_config_word(vf->dev, PCI_COMMAND, &val);
	val |= PCI_COMMAND_MASTER;
	oss_pci_write_config_word(vf->dev, PCI_COMMAND, val);

	return ret;
}

static int mi300_reset_verify_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (mi300_gfx_wait_for_grbm(adapt, idx_vf)) {
		AMDGV_ERROR("GRBM_STATUS2 is not clean after FLR.\n");
		return AMDGV_FAILURE;
	}
	return 0;
}

static int mi300_reset_trigger_vf_flr(struct amdgv_adapter *adapt,
		uint32_t idx_vf)
{
	int ret = 0;
	struct mi300_vf_flr_state vf_state;
	struct mi300_reset_access_info access_info;
	int sdma_id;
	int doorbell_index;

	/* need SMU FW loaded and responding to do FLR */
	if (!mi300_smu_get_fw_loaded_status(adapt)) {
		AMDGV_ERROR("SMU FW not responding. Unable to do FLR\n");
		return AMDGV_FAILURE;
	}

	if (idx_vf == AMDGV_PF_IDX)
		return mi300_reset_trigger_pf_soft_flr(adapt);

	AMDGV_INFO("start %s FLR\n", amdgv_idx_to_str(idx_vf));

	vf_state.pci_cfg = oss_zalloc(PCI_CONFIG_SIZE);
	if (!vf_state.pci_cfg) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			PCI_CONFIG_SIZE);
		return AMDGV_FAILURE;
	}

	vf_state.idx_vf = idx_vf;
	mi300_gfx_init_flr(adapt, idx_vf);
	mi300_gfx_save_tcp_addr_config(adapt, &vf_state);

	mi300_reset_save_active_vf_idx(adapt, &vf_state);
	mi300_reset_save_time_quanta_info(adapt, &vf_state);
	mi300_reset_save_csa_config(adapt, &vf_state.csa);
	mi300_reset_vf_save_and_disable(adapt, idx_vf, vf_state.pci_cfg, vf_state.msix_tab);

	/* save mmio protection info before flr */
	mi300_reset_save_access_info(adapt, &access_info);

	/* enable all protection before flr */
	amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_ALL, false);

	/* do the FLR */
	ret = mi300_reset_vf_flr(adapt, idx_vf);
	if (ret)
		goto failed;

	if (adapt->psp.psp_program_guest_mc_settings) {
		/* program vf mc settings */
		AMDGV_DEBUG("program %s mc settings\n",
				amdgv_idx_to_str(idx_vf));
		ret = adapt->psp.psp_program_guest_mc_settings(adapt, idx_vf);
		if (ret) {
			AMDGV_ERROR("program %s mc settings failed\n", amdgv_idx_to_str(idx_vf));
			goto failed;
		}
	}

	/* restore SDMA golden register settings */
	mi300_sdma_program_golden_settings(adapt);
	mi300_gfx_program_golden_settings(adapt);
	gfxhub_v1_2_gart_enable(adapt);
	mi300_gfx_restore_tcp_addr_config(adapt, &vf_state);
	mi300_reset_vf_restore(adapt, idx_vf, vf_state.pci_cfg, vf_state.msix_tab);

	/* init CSA */
	mi300_reset_restore_csa_config(adapt, &vf_state.csa);
	if (adapt->asic_type == CHIP_MI308X)
		mi308_nbio_assign_sdma_to_vf(adapt);
	else
		mi300_nbio_assign_sdma_to_vf(adapt);

	for (sdma_id = 0; sdma_id < adapt->sdma.num_instances; sdma_id++) {
		doorbell_index = AMDGV_MI300_DOORBELL_sDMA_ENGINE0 + sdma_id * 10;
		doorbell_index = doorbell_index << 1;
		mi300_nbio_assign_sdma_doorbell(adapt, sdma_id, doorbell_index, 20);
	}

	ret = mi300_gfx_enable_rlc(adapt, idx_vf);
	if (ret)
		goto failed;

	ret = mi300_reset_verify_flr(adapt, idx_vf);
	if (ret)
		goto failed;

	/* PMFW unhalt SDMA & RLCV, restore GPUIOV settings */
	ret = mi300_smu_gfx_flr_recovery(adapt, idx_vf);
	if (ret)
		goto failed;

	/* Check that RLC is idle */
	ret = mi300_reset_verify_flr(adapt, idx_vf);
	if (ret)
		goto failed;

	mi300_reset_restore_active_vf_idx(adapt, &vf_state);
	mi300_reset_restore_time_quanta_info(adapt, &vf_state);

	/* restore mmio protection info after flr */
	mi300_reset_restore_access_info(adapt, &access_info);

	amdgv_gpuiov_set_vf_fb(adapt, idx_vf, adapt->array_vf[idx_vf].fb_offset,
					adapt->array_vf[idx_vf].fb_size);

	ret = amdgv_sched_reset(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);

	if (!ret)
		AMDGV_INFO("completed %s FLR succesfully\n", amdgv_idx_to_str(idx_vf));

failed:
	oss_free(vf_state.pci_cfg);
	return ret;
}

static void mi300_reset_clear_all_pci_errors(struct amdgv_adapter *adapt)
{
	uint16_t data_16;
	uint32_t data_32;
	int pos;

	/* Clear any pending status in PCI_STATUS (offset 0x06) */
	oss_pci_read_config_word(adapt->dev, PCI_STATUS, &data_16);
	data_16 &= 0xF900; /* Isolate resettable error bits */
	if (data_16)
		oss_pci_write_config_word(adapt->dev, PCI_STATUS, data_16);

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID_EXP);
	/* Clear DevStatus (offset 0x62) */
	oss_pci_read_config_word(adapt->dev, pos + PCI_EXP_DEVSTA, &data_16);
	data_16 &= 0x000F; /* Isolate resettable error bits */
	if (data_16)
		oss_pci_write_config_word(adapt->dev, pos + PCI_EXP_DEVSTA, data_16);

	pos = oss_pci_find_ext_cap(adapt->dev, PCI_EXT_CAP_ID_ERR);

	/* Clear uncorrectable error status (offset 0x154) */
	oss_pci_read_config_dword(adapt->dev, pos + PCI_ERR_UNCOR_STATUS, &data_32);
	if (data_32)
		oss_pci_write_config_dword(adapt->dev, pos + PCI_ERR_UNCOR_STATUS, data_32);

	/* Clear the correctable error status(offset 0x160) */
	oss_pci_read_config_dword(adapt->dev, pos + PCI_ERR_COR_STATUS, &data_32);
	if (data_32)
		oss_pci_write_config_dword(adapt->dev, pos + PCI_ERR_COR_STATUS, data_32);
}

static void mi300_reset_save_vfs(struct amdgv_adapter *adapt,
				 struct mi300_whole_gpu_reset_state *reset_state)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		vf = &adapt->array_vf[idx_vf];

		if (!vf->configured)
			continue;

		/* save VF */
		mi300_reset_vf_save_and_disable(adapt, idx_vf, reset_state->vf_pci_cfg[idx_vf],
						reset_state->vf_msix_tab[idx_vf]);

		amdgv_mailbox_save_state(adapt, idx_vf);
	}
}

static void mi300_reset_restore_vfs(struct amdgv_adapter *adapt,
				    struct mi300_whole_gpu_reset_state *reset_state)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		vf = &adapt->array_vf[idx_vf];
		if (!vf->configured)
			continue;

		/* enable MMIO register write VF access */
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE,
					   true);

		/* restore VF */
		mi300_reset_vf_restore(adapt, idx_vf, reset_state->vf_pci_cfg[idx_vf],
				       reset_state->vf_msix_tab[idx_vf]);

		/* disable MMIO register write VF access */
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE,
					   false);

		amdgv_mailbox_restore_state(adapt, idx_vf);
	}
}

static void mi300_reset_save_pf_misc_reg(struct amdgv_adapter *adapt,
					 struct mi300_whole_gpu_reset_state *reset_state)
{
	int entry;
	uint32_t *msix_tab;
	uint32_t *tab;

	/* save bif_bx strap0 */
	reset_state->bif_bx_strap0 = RREG32(mmCC_BIF_BX_STRAP0);

	msix_tab = reset_state->pf_msix_tab;
	/* save msix table
	 * based on mi300 hw, the table is located in BAR5 res
	 * and max vector number is 3 */
	for (entry = 0; entry < 3; entry++) {
		/* after decoding the MSI-X capability,
		 * the PBA is 5 and table offest is 0x42000 */
		tab = (uint32_t *)adapt->mmio + 4 * entry +
		      SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_GFXMSIX_VECT0_ADDR_LO);
		msix_tab[entry * 4] = oss_mm_read32(tab);
		msix_tab[entry * 4 + 1] = oss_mm_read32(tab + 1);
		msix_tab[entry * 4 + 2] = oss_mm_read32(tab + 2);
		msix_tab[entry * 4 + 3] = oss_mm_read32(tab + 3);
	}
}

static void mi300_reset_save_pf(struct amdgv_adapter *adapt,
				struct mi300_whole_gpu_reset_state *reset_state)
{
	int idx;
	uint32_t *pci_cfg;

	pci_cfg = reset_state->pf_pci_cfg;

	/* Save the pf pci cfg space */
	for (idx = 0; idx < PCI_CONFIG_SIZE; idx += 4) {
		oss_pci_read_config_dword(adapt->dev, idx, pci_cfg);
		pci_cfg++;
	}

	mi300_reset_save_pf_misc_reg(adapt, reset_state);
}

static void mi300_reset_restore_pf_misc_reg(struct amdgv_adapter *adapt,
					    struct mi300_whole_gpu_reset_state *reset_state)
{
	int entry;
	uint32_t *tab;
	uint32_t *val;

	/* resotre bif_bx strap0 */
	WREG32(mmCC_BIF_BX_STRAP0, reset_state->bif_bx_strap0);

	/* Resore the PF msix table */
	for (entry = 0; entry < 3; entry++) {
		tab = (uint32_t *)adapt->mmio + 4 * entry +
		      SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_GFXMSIX_VECT0_ADDR_LO);
		val = &reset_state->pf_msix_tab[entry * 4];

		oss_mm_write32(tab, val[0]);
		oss_mm_write32(tab + 1, val[1]);
		oss_mm_write32(tab + 2, val[2]);
		oss_mm_write32(tab + 3, val[3]);
	}
}

static void mi300_reset_restore_pf(struct amdgv_adapter *adapt,
				   struct mi300_whole_gpu_reset_state *reset_state)
{
	int idx;

	for (idx = 0; idx < ARRAY_SIZE(restore_tbl); ++idx) {
		if (restore_tbl[idx].offset >= CAP_SRIOV_OFFSET)
			break;

		restore_reg(adapt, &restore_tbl[idx], reset_state);
	}

	mi300_reset_restore_pf_misc_reg(adapt, reset_state);
}

static int mi300_reset_trigger_pf_soft_flr(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct mi300_reset_access_info access_info;
	struct mi300_whole_gpu_reset_state *reset_state;
	uint32_t sdma_id;
	int doorbell_index;
	uint32_t csa_offset = 0;

	AMDGV_INFO("start PF Soft FLR\n");

	reset_state = oss_zalloc(sizeof(struct mi300_whole_gpu_reset_state));
	if (!reset_state) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct mi300_whole_gpu_reset_state));
		return AMDGV_FAILURE;
	}

	//save all vfs
	mi300_reset_save_vfs(adapt, reset_state);
	mi300_reset_save_pf(adapt, reset_state);
	amdgv_reset_save_sriov(adapt);
	mi300_reset_save_csa_config(adapt, &csa_offset);

	/* save mmio protection info before flr */
	mi300_reset_save_access_info(adapt, &access_info);

	/* do the FLR */
	ret = mi300_reset_vf_flr(adapt, AMDGV_PF_IDX);
	if (ret)
		goto failed;

	if (adapt->psp.psp_program_guest_mc_settings) {
		ret = adapt->psp.psp_program_guest_mc_settings(adapt, AMDGV_PF_IDX);
		if (ret) {
			AMDGV_ERROR("program PF MC settings failed\n");
			goto failed;
		}
	}

	/* restore SDMA golden register settings */
	mi300_sdma_program_golden_settings(adapt);
	mi300_gfx_program_golden_settings(adapt);
	gfxhub_v1_2_gart_enable(adapt);

	mi300_reset_restore_vfs(adapt, reset_state);
	mi300_reset_restore_pf(adapt, reset_state);
	mi300_reset_restore_csa_config(adapt, &csa_offset);
	amdgv_reset_restore_sriov(adapt);

	/* restore mmio protection info after flr */
	mi300_reset_restore_access_info(adapt, &access_info);

	if (adapt->asic_type == CHIP_MI308X)
		mi308_nbio_assign_sdma_to_vf(adapt);
	else
		mi300_nbio_assign_sdma_to_vf(adapt);

	for (sdma_id = 0; sdma_id < adapt->sdma.num_instances; sdma_id++) {
		doorbell_index = AMDGV_MI300_DOORBELL_sDMA_ENGINE0 + sdma_id * 10;
		doorbell_index = doorbell_index << 1;
		mi300_nbio_assign_sdma_doorbell(adapt, sdma_id, doorbell_index, 20);
	}

	ret = mi300_gfx_enable_rlc(adapt, AMDGV_PF_IDX);
	if (ret)
		goto failed;

	ret = mi300_reset_verify_flr(adapt, AMDGV_PF_IDX);
	if (ret)
		goto failed;

	/* PMFW unhalt RLCV & SDMA FWs */
	ret = mi300_smu_gfx_mode_3_recovery(adapt,
				amdgv_sched_get_xcc_mask_by_vf(adapt, AMDGV_PF_IDX));
	if (ret)
		goto failed;

	/* Check that RLC is idle */
	ret = mi300_reset_verify_flr(adapt, AMDGV_PF_IDX);
	if (ret)
		goto failed;

	/* re-enable CGCG */
	mi300_clockgating_hw_init(adapt);

	ret = amdgv_sched_reset(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL);

	if (!ret)
		AMDGV_INFO("completed PF Soft FLR succesfully\n");

failed:
	oss_free(reset_state);
	return ret;
}

int mi300_reset_trigger_whole_gpu_reset(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t tmp;
	uint32_t bit_s3_int;
	struct mi300_reset_access_info access_info;
	struct mi300_whole_gpu_reset_state *reset_state;

	/* alloc memory for whole GPU reset */
	reset_state = oss_zalloc(sizeof(struct mi300_whole_gpu_reset_state));
	if (reset_state == NULL) {
		AMDGV_ERROR("Failed to alloc memory for whole GPU reset\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("start whole gpu reset\n");

	/* save mmio protection info before WHOLE_GPU_RESET */
	mi300_reset_save_access_info(adapt, &access_info);

	/* enable all protection before WHOLE_GPU_RESET */
	mi300_reset_enable_mmio_protection(adapt);

	mi300_reset_save_pf(adapt, reset_state);
	amdgv_reset_save_sriov(adapt);

	/* force S3 engine hung */
	bit_s3_int = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_SBIOS_SCRATCH_3));
	tmp = bit_s3_int | ATOM_S3_ASIC_GUI_ENGINE_HUNG;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_SBIOS_SCRATCH_3), tmp);

	ret = mi300_gpu_mode1_reset(adapt);
	if (ret)
		goto exit;

	mi300_reset_restore_pf(adapt, reset_state);

	/*check mode1 reset finish after restore_pci_config*/
	ret = mi300_wait_gpu_reset_completion(adapt);
	if (ret)
		goto exit;

	/* disable S3 engine hung state */
	tmp = bit_s3_int & ~ATOM_S3_ASIC_GUI_ENGINE_HUNG;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_SBIOS_SCRATCH_3), tmp);

	/* clear VBIOS status */
	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7));
	tmp &= ~ATOM_ASIC_INIT_COMPLETE;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7), tmp);

	amdgv_reset_restore_sriov(adapt);

	/* restore mmio protection info after PF_FLR or WHOLE_GPU_RESET */
	mi300_reset_restore_access_info(adapt, &access_info);

exit:
	oss_free(reset_state);

	if (ret)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_RESET_GPU_FAILED, 0);
	else
		AMDGV_INFO("complete whole gpu reset\n");

	return ret;
}

void mi300_clear_dummy_mode_after_reset(struct amdgv_adapter *adapt)
{
	uint32_t baco_cntl;

	baco_cntl = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL));

	if (baco_cntl &
		(BIF_BX0_BACO_CNTL__BACO_DUMMY_EN_MASK | BIF_BX0_BACO_CNTL__BACO_EN_MASK)) {
		baco_cntl &= ~(BIF_BX0_BACO_CNTL__BACO_DUMMY_EN_MASK |
				BIF_BX0_BACO_CNTL__BACO_EN_MASK);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BACO_CNTL), baco_cntl);
	}
}

/* Whole GPU reset only can work with below assumptions:
 *  1) No world switch(SW&HW), No interrupt
 *  2) All active VFs should be notified before start hot link reset
 *  3) All active VFs should be notified after hot link reset done
 *  4) VF & PF pci cfg status (EXCEPT PF GPUIOV SETTING) should be saved
 *     and restored by GIM
 *  5) PF GPUIOV SETTING is supposed to be recovered by hw_init and handshake
 *     with guest driver for reset
 *  6) MSI-X table should be saved and retored for both PF and VF by GIM
 *  7) VF GPU driver should reinit GPU after reset
 */
static int mi300_reset_whole_gpu_reset(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int idx;
	uint32_t tmp;
	uint32_t bit_s3_int;
	struct mi300_reset_access_info access_info;
	struct mi300_whole_gpu_reset_state *reset_state;
	struct amdgv_hive_info *hive;

	/* alloc memory for whole GPU reset */
	reset_state = oss_zalloc(sizeof(struct mi300_whole_gpu_reset_state));
	if (reset_state == NULL) {
		AMDGV_ERROR("Failed to alloc memory for whole GPU reset\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("start whole gpu reset\n");

	mi300_reset_clear_all_pci_errors(adapt);

	/* save mmio protection info before WHOLE_GPU_RESET */
	mi300_reset_save_access_info(adapt, &access_info);

	/* enable all protection before WHOLE_GPU_RESET */
	mi300_reset_enable_mmio_protection(adapt);

	mi300_reset_save_vfs(adapt, reset_state);

	mi300_irqmgr_save_and_fini(adapt, &reset_state->irqmgr_reset_state);

	mi300_reset_save_pf(adapt, reset_state);

	amdgv_reset_save_sriov(adapt);

	/* fini HW */
	for (idx = adapt->num_funcs - 1; idx >= 0; idx--)
		if (adapt->init_funcs[idx]->hw_fini)
			adapt->init_funcs[idx]->hw_fini(adapt);

	/* force S3 engine hung */
	bit_s3_int = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_SBIOS_SCRATCH_3));
	tmp = bit_s3_int | ATOM_S3_ASIC_GUI_ENGINE_HUNG;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_SBIOS_SCRATCH_3), tmp);

	if (adapt->reset.in_xgmi_chain_reset) {
		hive = amdgv_get_xgmi_hive(adapt);
		if (!hive) {
			AMDGV_ERROR("XGMI: node 0x%llx, can not match hive "
				    "0x%llx in the hive list.\n",
				    adapt->xgmi.node_id, adapt->xgmi.hive_id);
			goto exit;
		}

		/* Only 1 GPU should perform XGMI FB sharing sanity check */
		if (adapt->xgmi.phy_node_id == 0)
			mi300_xgmi_sanitize_hive_fb_sharing_config(adapt, hive);

		task_barrier_enter(&hive->tb_chain_reset, hive->number_adapters);

		ret = mi300_gpu_mode1_reset(adapt);

		task_barrier_exit(&hive->tb_chain_reset, hive->number_adapters);
		if (ret)
			goto exit;
		AMDGV_DEBUG("Return from whole gpu reset call on XGMI: node 0x%llx\n",
			    adapt->xgmi.node_id);

	} else {
		ret = mi300_gpu_mode1_reset(adapt);
		if (ret)
			goto exit;
	}

	if (amdgv_ras_intr_triggered())
		amdgv_ras_intr_cleared();

	mi300_reset_restore_pf(adapt, reset_state);

	/*check mode1 reset finish after restore_pci_config*/
	ret = mi300_wait_gpu_reset_completion(adapt);
	if (ret) {
		goto exit;
	}

	/* disable S3 engine hung state */
	tmp = bit_s3_int & ~ATOM_S3_ASIC_GUI_ENGINE_HUNG;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_SBIOS_SCRATCH_3), tmp);

	/* clear VBIOS status */
	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7));
	tmp &= ~ATOM_ASIC_INIT_COMPLETE;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7), tmp);

	/* re-init HW */
	for (idx = 0; idx < adapt->num_funcs; idx++) {
		if (adapt->init_funcs[idx]->hw_init) {
			ret = adapt->init_funcs[idx]->hw_init(adapt);
			if (ret) {
				amdgv_put_error(idx, AMDGV_ERROR_DRIVER_HW_INIT_FAIL, 0);

				goto exit;
			}
		}
	}

	mi300_irqmgr_restore_and_init(adapt, &reset_state->irqmgr_reset_state);

	mi300_reset_restore_vfs(adapt, reset_state);

	/* restore mmio protection info after PF_FLR or WHOLE_GPU_RESET */
	mi300_reset_restore_access_info(adapt, &access_info);

exit:
	oss_free(reset_state);

	if (ret)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_RESET_GPU_FAILED, 0);
	else
		AMDGV_INFO("complete whole gpu reset\n");

	return ret;
}

static bool mi300_reset_is_engine_inst_busy(struct amdgv_adapter *adapt,
		struct engine_reg_info *engine_info, uint32_t inst)
{
	uint32_t status;

	status = RREG32(
		adapt->reg_offset[engine_info->hwip][inst][engine_info->seg] + engine_info->offset);
	if (engine_info->clean_cond != (engine_info->reg_mask & (status >> engine_info->reg_shift)))
		return true;

	return false;
}

static bool mi300_reset_is_engine_busy(struct amdgv_adapter *adapt, uint32_t idx_vf,
		struct engine_reg_info *engine_info)
{
	uint32_t xcc_id;

	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
		switch (engine_info->hwip) {
		case GC_HWIP:
			if (mi300_reset_is_engine_inst_busy(adapt, engine_info,
					GET_INST(GC, xcc_id)))
				return true;
			break;
		case SDMA0_HWIP:
			/* There are 2 SDMA instances per XCC. All instances are under SDMA0 HWIP*/
			if (mi300_reset_is_engine_inst_busy(adapt, engine_info,
					GET_INST(SDMA0, xcc_id * 2)))
				return true;
			if (mi300_reset_is_engine_inst_busy(adapt, engine_info,
					GET_INST(SDMA0, (xcc_id * 2) + 1)))
				return true;
			break;
		default:
			AMDGV_WARN("Unsupported Engine type\n");
			break;
		}
	}

	return false;
}

static bool mi300_reset_is_hw_sched_abnormal(struct amdgv_adapter *adapt, uint32_t idx_vf,
		uint32_t hw_sched_mask)
{
	uint32_t hw_sched_id = 0;
	uint32_t curr_vf_state;

	for_each_id (hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
		if (hw_sched_mask & (1 << hw_sched_id)) {
			amdgv_sched_world_context_get_hw_curr_state(adapt, hw_sched_id, &curr_vf_state);
			if (curr_vf_state == AMDGV_VF_CONTEXT_ABNORMAL)
				return true;
		}
	}

	return false;
}

static int mi300_reset_notify_engine_status(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (mi300_reset_is_engine_busy(adapt, idx_vf, &mi300_engine_reg_info[RLC_STAT]))
		amdgv_put_error(idx_vf, AMDGV_ERROR_GPU_RLC_ENGINE_BUSY, 0);

	if (mi300_reset_is_engine_busy(adapt, idx_vf, &mi300_engine_reg_info[SDMA_STATUS_REG]) ||
		mi300_reset_is_engine_busy(adapt, idx_vf, &mi300_engine_reg_info[SDMA_STATUS4_REG]))
		amdgv_put_error(idx_vf, AMDGV_ERROR_GPU_SDMA_ENGINE_BUSY, 0);

	if (mi300_reset_is_engine_busy(adapt, idx_vf, &mi300_engine_reg_info[GRBM_STATUS]))
		amdgv_put_error(idx_vf, AMDGV_ERROR_GPU_GC_ENGINE_BUSY, 0);

	if (mi300_reset_is_hw_sched_abnormal(adapt, idx_vf, MI300_XCC_SCHED_MASK))
		amdgv_put_error(idx_vf, AMDGV_ERROR_GPU_RLCV_ABNORMAL_STATE, 0);

	if (mi300_reset_is_hw_sched_abnormal(adapt, idx_vf, ~MI300_XCC_SCHED_MASK))
		amdgv_put_error(idx_vf, AMDGV_ERROR_GPU_MMSCH_ABNORMAL_STATE, 0);

	return 0;
}

struct amdgv_gpu_reset_funcs mi300_reset_funcs = {
	.save_vddgfx_state = mi300_reset_save_vddgfx_state,
	.trigger_vf_flr = mi300_reset_trigger_vf_flr,
	.trigger_gpu_reset = mi300_reset_whole_gpu_reset,
	.notify_engine_status = mi300_reset_notify_engine_status
};

static int mi300_reset_sw_init(struct amdgv_adapter *adapt)
{
	adapt->reset.reset_num = 0;
	adapt->reset.reset_state = false;
	adapt->reset.in_xgmi_chain_reset = false;
	adapt->reset.funcs = &mi300_reset_funcs;

	return 0;
}

static int mi300_reset_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->reset.funcs = NULL;

	return 0;
}

static int mi300_reset_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_reset_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi300_reset_func = {
	.name = "mi300_reset_func",
	.sw_init = mi300_reset_sw_init,
	.sw_fini = mi300_reset_sw_fini,
	.hw_init = mi300_reset_hw_init,
	.hw_fini = mi300_reset_hw_fini,
};
