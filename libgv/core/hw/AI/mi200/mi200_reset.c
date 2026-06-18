/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_pci_def.h>
#include <amdgv_gpuiov.h>
#include <amdgv_irqmgr.h>
#include <amdgv_psp_gfx_if.h>
#include <atombios/atomfirmware.h>
#include <atombios/atom.h>
#include <atombios/atombios.h>

#include "mi200.h"
#include "mi200_gpuiov.h"
#include "mi200_ppsmc.h"
#include "mi200_irqmgr.h"
#include "mi200_golden_settings.h"
#include "mi200_reset.h"
#include "mi200_powerplay.h"

#define MI200_MAX_VF_NUM 16

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

struct mi200_whole_gpu_reset_state {
	uint32_t bif_bx_strap0;

	uint32_t pf_pm_ctl;
	uint16_t pf_misx_ctl;

	uint32_t pf_pci_cfg[PCI_CONFIG_SIZE];
	uint32_t pf_msix_tab[12];

	/* the max vf num of mi200 is 16*/
	uint32_t vf_pci_cfg[MI200_MAX_VF_NUM][PCI_CONFIG_SIZE];
	uint32_t vf_msix_tab[MI200_MAX_VF_NUM][12];

	struct mi200_irqmgr_reset_state irqmgr_reset_state;
};

struct mi200_vf_flr_state {
	uint32_t idx_vf;
	uint32_t tcp_addr_config;

	uint32_t active_vfs[AMDGV_MAX_NUM_HW_SCHED];
	uint32_t quanta_option[AMDGV_MAX_NUM_HW_SCHED];
	uint32_t quanta_index[AMDGV_MAX_NUM_HW_SCHED];

	uint32_t csa;

	uint32_t *pci_cfg;
	uint32_t msix_tab[12];
};

/* list all writeable cfgs need to be restored and in a correct sequence */
static struct pf_pcie_restore restore_tbl[] = {
	{0x0004, 2, "Command_Epf"},
	{0x0006, 2, "Status"},
	{0x000c, 1, "Cache Line"},
	{0x000d, 1, "Latency"},
	{0x000e, 1, "Header"},
	{0x000f, 1, "BIST"},
	{0x0010, 4, "BAR1"},
	{0x0014, 4, "BAR2"},
	{0x0018, 4, "BAR3"},
	{0x001c, 4, "BAR4"},
	{0x0020, 4, "BAR5"},
	{0x0024, 4, "BAR6"},

	{0x0030, 4, "ROM Base Address"},
	{0x0034, 4, "Cap Pointer"},

	{0x003c, 1, "Int Line"},
	{0x003d, 1, "Int Pin"},
	{0x003e, 1, "Min Grant"},
	{0x003f, 1, "Max Latency"},


	{0x0048, 4, "Vendor Cap List"},
	{0x004c, 4, "Adapter Id Master-write"},
	{0x0052, 2, "PMI Cap"},
	{0x0054, 4, "PMI Status Control"},

	{0x006C, 2, "Device Cntl"},
	{0x0074, 2, "Link Cntl"},

	{0x008C, 2, "Device Cntl2"},
	{0x0094, 2, "Link Cntl2"},
	{0x009C, 2, "Slot Cntl2"},

	/* enable interrupt */
	{0x00c2, 2, "MSIX Msg Cntl"},

#define CAP_SRIOV_OFFSET 0x330
	/* CAP SRIOV */
	{0x334, 4, "SR-IOV Capabilities"},
	{0x33a, 2, "SR-IOV Status"},
	{0x33c, 2, "SR-IOV Initial VFs"},
	{0x33e, 2, "SR-IOV Total VFs"},
	{0x340, 2, "SR-IOV Num VFs"},
	{0x342, 2, "SR-IOV Func Dep Link"},
	{0x344, 2, "SR-IOV First Offset"},
	{0x346, 2, "SR-IOV Stride"},
	{0x34a, 2, "SR-IOV VF Device ID"},
	{0x34c, 4, "SR-IOV Sup PageSize"},
	{0x350, 4, "SR-IOV Sys PageSize"},
	{0x354, 4, "SR-IOV VF BAR0"},
	{0x358, 4, "SR-IOV VF BAR1"},
	{0x35c, 4, "SR-IOV VF BAR2"},
	{0x360, 4, "SR-IOV VF BAR3"},
	{0x364, 4, "SR-IOV VF BAR4"},
	{0x368, 4, "SR-IOV VF BAR5"},
	{0x36c, 4, "SR-IOV VF Migration State Array Offset"},

	/* enable bit is set AFTER setting up SRIOV capabilities. */
	{0x338, 2, "SR-IOV Control"},

};

struct mi200_reset_access_info {
	uint32_t fb_access_info;
	uint32_t doorbell_access_info;
	uint32_t register_write_access_info;
};

static void restore_reg(struct amdgv_adapter *adapt,
		struct pf_pcie_restore *entry,
		struct mi200_whole_gpu_reset_state *reset_state)
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
		oss_pci_write_config_word(adapt->dev, offset,
				*(uint16_t *)val);
		break;

	case 4:
		oss_pci_write_config_dword(adapt->dev, offset,
				*(uint32_t *)val);
		break;
	}
}

/* a dummy funciton */
static int mi200_reset_save_vddgfx_state(struct amdgv_adapter *adapt,
		uint32_t idx_vf)
{
	return 0;
}

static void mi200_reset_save_active_vf_idx(struct amdgv_adapter *adapt,
					   struct mi200_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->enabled) {
			for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
					amdgv_gpuiov_get_active_vfs(adapt, hw_sched_id,
						&vf_state->active_vfs[world_switch->sched_block]);
			}
		}
	}
}

static void mi200_reset_restore_active_vf_idx(struct amdgv_adapter *adapt,
					      struct mi200_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->enabled) {
			for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
					amdgv_gpuiov_set_active_vfs(adapt, hw_sched_id,
						vf_state->active_vfs[world_switch->sched_block]);
			}
		}
	}
}

static void mi200_reset_save_time_quanta_info(struct amdgv_adapter *adapt,
					      struct mi200_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->enabled) {
			for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
				amdgv_gpuiov_get_time_quanta_option(adapt, hw_sched_id,
									&vf_state->quanta_option[world_switch->sched_block]);
				amdgv_gpuiov_get_time_quanta_index(adapt, vf_state->idx_vf, hw_sched_id,
								&vf_state->quanta_index[world_switch->sched_block]);
			}
		}
	}
}

static void mi200_reset_restore_time_quanta_info(struct amdgv_adapter *adapt,
						 struct mi200_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->enabled) {
			for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
				amdgv_gpuiov_set_time_quanta_option(adapt, hw_sched_id,
									vf_state->quanta_option[world_switch->sched_block]);
				amdgv_gpuiov_set_time_quanta_index(adapt, vf_state->idx_vf, hw_sched_id,
								vf_state->quanta_index[world_switch->sched_block]);
			}
		}
	}
}

static int mi200_reset_save_csa_config(struct amdgv_adapter *adapt,
					uint32_t *csa)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_CNTXT;
	oss_pci_read_config_dword(adapt->dev, offset, csa);

	return 0;
}

static int mi200_reset_restore_csa_config(struct amdgv_adapter *adapt,
					uint32_t *csa)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_CNTXT;
	oss_pci_write_config_dword(adapt->dev, offset, *csa);

	return 0;
}

static void mi200_reset_save_access_info(struct amdgv_adapter *adapt,
					   struct mi200_reset_access_info *access_info)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	access_info->fb_access_info = RREG32(mmnbif_gpu_VF_FB_EN);
	access_info->doorbell_access_info = RREG32(mmnbif_gpu_VF_DOORBELL_EN);
	access_info->register_write_access_info = RREG32(mmnbif_gpu_VF_REGWR_EN);
}

static void mi200_reset_restore_access_info(struct amdgv_adapter *adapt,
						struct mi200_reset_access_info *access_info)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	WREG32(mmnbif_gpu_VF_FB_EN, access_info->fb_access_info);
	WREG32(mmnbif_gpu_VF_DOORBELL_EN, access_info->doorbell_access_info);
	WREG32(mmnbif_gpu_VF_REGWR_EN, access_info->register_write_access_info);
}

static void mi200_reset_enable_mmio_protection(struct amdgv_adapter *adapt)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	WREG32(mmnbif_gpu_VF_FB_EN, 0);
	WREG32(mmnbif_gpu_VF_DOORBELL_EN, 0);
	WREG32(mmnbif_gpu_VF_REGWR_EN, 0);
}

static void mi200_reset_vf_save_and_disable(struct amdgv_adapter *adapt,
		uint32_t idx_vf,
		uint32_t *pci_cfg,
		uint32_t *msix_tab)
{
	uint32_t idx = 0;
	struct amdgv_vf_device *vf;
#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	int entry;
	uint32_t *tab;
#endif

	vf = &adapt->array_vf[idx_vf];

	/* Save the vf pci cfg space */
	for (idx = 0; idx < PCI_CONFIG_SIZE; idx += 4) {
		oss_pci_read_config_dword(vf->dev, idx, pci_cfg);
		pci_cfg++;
	}

#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	/* save msix table
	 * based on mi200 hw, the table is located in BAR5 res
	 * and max vector number is 3 */
	if (vf->res_mapped && vf->res.mmio) {
		for (entry = 0; entry < 3; entry++) {
			/* after decoding the MSI-X capability,
			 * the PBA is 5 and table offest is 0x42000 */
			tab = (uint32_t *)vf->res.mmio + 4 * entry
				+ SOC15_REG_OFFSET(NBIO, 0,
						mmGFXMSIX_VECT0_ADDR_LO);
			msix_tab[entry * 4] = oss_mm_read32(tab);
			msix_tab[entry * 4 + 1] = oss_mm_read32(tab + 1);
			msix_tab[entry * 4 + 2] = oss_mm_read32(tab + 2);
			msix_tab[entry * 4 + 3] = oss_mm_read32(tab + 3);
		}
	}
#endif

	/* disable the vf by clearing the command */
	oss_pci_write_config_word(vf->dev, PCI_COMMAND,
				PCI_COMMAND_INTX_DISABLE);
}

static void mi200_reset_vf_restore(struct amdgv_adapter *adapt,
		uint32_t idx_vf,
		uint32_t *pci_cfg,
		uint32_t *msix_tab)

{
#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	int entry;
	uint32_t *val, *tab;
#endif
	uint32_t idx;
	struct amdgv_vf_device *vf;

	vf = &adapt->array_vf[idx_vf];

	/* Restore the vf pci cfg space */
	for (idx = 0; idx < PCI_CONFIG_SIZE; idx += 4) {
		oss_pci_write_config_dword(vf->dev, idx, *pci_cfg);
		pci_cfg++;
	}

#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	/* Restore the vf msix table */
	if (vf->res_mapped && vf->res.mmio) {
		/* disable mmio reg write protection */
		amdgv_gpuiov_set_vf_access(
		    adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, true);

		for (entry = 0; entry < 3; entry++) {
			tab = (uint32_t *)vf->res.mmio + 4 * entry
				+ SOC15_REG_OFFSET(NBIO, 0,
						mmGFXMSIX_VECT0_ADDR_LO);
			val = &msix_tab[entry * 4];

			oss_mm_write32(tab, val[0]);
			oss_mm_write32(tab + 1, val[1]);
			oss_mm_write32(tab + 2, val[2]);
			oss_mm_write32(tab + 3, val[3]);
		}
	}
#endif
}

static int mi200_wait_rlc_idle(struct amdgv_adapter *adapt)
{
	int wait_ret;

	wait_ret = amdgv_wait_for_register(
	    adapt, SOC15_REG_OFFSET_NAME(GC, 0, mmRLC_STAT), 0, 0,
	    AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
	    AMDGV_WAIT_FLAG_FORCE_YIELD);

	if (wait_ret)
		return AMDGV_FAILURE;
	else
		return 0;
}

static int mi200_reset_enable_rlc(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t rlc_cntl;

	WREG32(SOC15_REG_OFFSET(GC, 0, mmRLC_GPM_THREAD_ENABLE), 0x3);

	/* Re-enable RLC */
	rlc_cntl = RREG32(SOC15_REG_OFFSET(GC, 0, mmRLC_CNTL));
	rlc_cntl = REG_SET_FIELD(rlc_cntl, RLC_CNTL,
				RLC_ENABLE_F32, 1);
	WREG32(SOC15_REG_OFFSET(GC, 0, mmRLC_CNTL), rlc_cntl);

	WREG32(SOC15_REG_OFFSET(GC, 0, mmRLC_SRM_CNTL), 3);

	if (mi200_wait_rlc_idle(adapt)) {
		AMDGV_ERROR("Failed to idle rlc state after %s FLR\n",
				amdgv_idx_to_str(idx_vf));
		return AMDGV_FAILURE;
	}

	return 0;
}

static void mi200_reset_save_tcp_addr_config(struct amdgv_adapter *adapt,
		struct mi200_vf_flr_state *vf_state)
{
	vf_state->tcp_addr_config = RREG32_SOC15(GC, 0, mmTCP_ADDR_CONFIG);
}

static void mi200_reset_restore_tcp_addr_config(struct amdgv_adapter *adapt,
		struct mi200_vf_flr_state *vf_state)
{
	WREG32_SOC15(GC, 0, mmTCP_ADDR_CONFIG, vf_state->tcp_addr_config);
}

static int mi200_reset_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int pos;
	uint16_t val;
	struct amdgv_vf_device *vf;
	uint32_t strap4;
	int wait_ret;
	int ret = 0;

	vf = &adapt->array_vf[idx_vf];
	pos = oss_pci_find_capability(vf->dev, PCI_CAP_ID_EXP);

	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID_EXP);
		return AMDGV_FAILURE;
	}

	/* disable device bus mastering */
	oss_pci_read_config_word(vf->dev, PCI_COMMAND, &val);
	val &= (~PCI_COMMAND_MASTER);
	oss_pci_write_config_word(vf->dev, PCI_COMMAND, val);

	/* wait for transaction done */
	wait_ret = amdgv_wait_for_pci_cfg(adapt, vf->dev, pos + PCIE_DEVICE_STATUS, PCIE_DEVICE_STATUS__TRANS_PEND, 0, 2, AMDGV_TIMEOUT(TIMEOUT_PCI_TRANS), AMDGV_WAIT_CHECK_EQ, 0);

	if (wait_ret)
		AMDGV_WARN("Abort data transaction on %s for FLR\n", amdgv_idx_to_str(idx_vf));

	/* enable FLR */
	if (!(adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY)) {
		strap4 = RREG32(mmnbif_gpu_RCC_DEV0_EPF0_STRAP4);
		strap4 |= STRAP_FLR_EN_DEV0_F0_MASK;
		WREG32(mmnbif_gpu_RCC_DEV0_EPF0_STRAP4, strap4);
	}

	/* use SMU msg to trigger FLR instead of PCIe control bit*/
	if (adapt->pp.pp_funcs->trigger_vf_flr) {
		ret = adapt->pp.pp_funcs->trigger_vf_flr(adapt, 1 << idx_vf);
		if (ret) {
			AMDGV_ERROR("Send Trigger VF FLR msg failed\n");
			ret = AMDGV_FAILURE;
		}
	} else {
		AMDGV_WARN("trigger_vf_flr is not implemented.\n");
		ret = AMDGV_FAILURE;
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

static int mi200_reset_wait_for_grbm(struct amdgv_adapter *adapt)
{
	uint32_t grbm_status = 0;
	uint32_t grbm_status2 = 0;
	int wait_ret;

	/* wait for a clean state */
	wait_ret = amdgv_wait_for_register(
	    adapt, SOC15_REG_OFFSET_NAME(GC, 0, mmGRBM_STATUS2),
	    GRBM_STATUS2__EA_BUSY_MASK | GRBM_STATUS2__EA_LINK_BUSY_MASK |
		GRBM_STATUS2__RLC_BUSY_MASK,
	    0, AMDGV_TIMEOUT(TIMEOUT_GRBM_STATUS), AMDGV_WAIT_CHECK_EQ,
	    AMDGV_WAIT_FLAG_FORCE_YIELD);

	grbm_status = RREG32(SOC15_REG_OFFSET(GC, 0, mmGRBM_STATUS));
	grbm_status2 = RREG32(SOC15_REG_OFFSET(GC, 0, mmGRBM_STATUS2));
	AMDGV_DEBUG("GRBM_STATUS = 0x%x GRBM_STATUS2 = 0x%x\n", grbm_status,
		    grbm_status2);

	if (wait_ret)
		return AMDGV_FAILURE;
	else
		return 0;
}

static int mi200_reset_init_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int entry;
	uint32_t vm_l2_protection_fault_cntl;
	uint32_t *tab_reg;
	struct amdgv_vf_device *vf;

	vf = &adapt->array_vf[idx_vf];

	/* stop gfx hub error state */
	vm_l2_protection_fault_cntl = RREG32(SOC15_REG_OFFSET(GC, 0,
				mmVM_L2_PROTECTION_FAULT_CNTL));

	/* clear bit 2 3 4 5 6 7 8 9 10 11 12 */
	vm_l2_protection_fault_cntl &= ~0x1ffb;

	/* set bit 30 31 */
	vm_l2_protection_fault_cntl |= 0x3 << 30;

	WREG32(SOC15_REG_OFFSET(GC, 0, mmVM_L2_PROTECTION_FAULT_CNTL),
			vm_l2_protection_fault_cntl);

	/* disable user context vm by setting START ADDR bigger
	 * than END ADDR to make all vm address invalid
	 */
	if (vf->res_mapped && vf->res.mmio) {
		for (entry = 0; entry < 15; ++entry) {
			tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
				SOC15_REG_OFFSET(GC, 0,
				mmVM_CONTEXT1_PAGE_TABLE_START_ADDR_LO32);

			oss_mm_write32(tab_reg, ~0);

			tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
				SOC15_REG_OFFSET(GC, 0,
				mmVM_CONTEXT1_PAGE_TABLE_START_ADDR_HI32);
			oss_mm_write32(tab_reg, ~0);

			tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
				SOC15_REG_OFFSET(GC, 0,
				mmVM_CONTEXT1_PAGE_TABLE_END_ADDR_LO32);
			oss_mm_write32(tab_reg, ~0);

			tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
				SOC15_REG_OFFSET(GC, 0,
				mmVM_CONTEXT1_PAGE_TABLE_END_ADDR_HI32);
			oss_mm_write32(tab_reg, ~0);
		}
	}

	if (mi200_reset_wait_for_grbm(adapt))
		AMDGV_WARN("GRBM_STATUS2 is not clean for FLR\n");

	return 0;
}

static int mi200_reset_fini_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t vm_l2_protection_fault_cntl;

	/* enable gfx hub error state */
	vm_l2_protection_fault_cntl =
	    RREG32(SOC15_REG_OFFSET(GC, 0, mmVM_L2_PROTECTION_FAULT_CNTL));

	/* set bit 2 3 4 5 6 7 8 9 10 11 12 */
	vm_l2_protection_fault_cntl |= 0x1ffb;

	/* clear bit 30 31 */
	vm_l2_protection_fault_cntl &= ~(0x3 << 30);

	WREG32(SOC15_REG_OFFSET(GC, 0, mmVM_L2_PROTECTION_FAULT_CNTL),
	       vm_l2_protection_fault_cntl);

	return 0;
}

static int mi200_reset_verify_flr(struct amdgv_adapter *adapt)
{
	if (mi200_reset_wait_for_grbm(adapt)) {
		AMDGV_ERROR("GRBM_STATUS2 is not clean after FLR.\n");
		return AMDGV_FAILURE;
	}
	return 0;
}

static int mi200_reset_trigger_vf_flr(struct amdgv_adapter *adapt,
		uint32_t idx_vf)
{
	int ret = 0;
	struct mi200_vf_flr_state vf_state;
	struct mi200_reset_access_info access_info;

	if (idx_vf == AMDGV_PF_IDX) {
		AMDGV_ERROR("PF FLR is not supported!\n");
		return AMDGV_FAILURE;
	} else
		AMDGV_INFO("start %s FLR\n", amdgv_idx_to_str(idx_vf));

	/* need SMU FW loaded and responding to do VF_FLR */
	if (mi200_smu_13_0_get_fw_loaded_status(adapt) == 0) {
		AMDGV_ERROR("SMU FW not responding. Unable to do FLR\n");
		return AMDGV_FAILURE;
	}

	vf_state.pci_cfg = oss_zalloc(PCI_CONFIG_SIZE);
	if (!vf_state.pci_cfg) {
		AMDGV_ERROR("Failed to alloc PCI config space\n");
		return AMDGV_FAILURE;
	}

	vf_state.idx_vf = idx_vf;
	mi200_reset_init_flr(adapt, idx_vf);
	mi200_reset_save_tcp_addr_config(adapt, &vf_state);

	mi200_reset_save_active_vf_idx(adapt, &vf_state);
	mi200_reset_save_time_quanta_info(adapt, &vf_state);
	mi200_reset_save_csa_config(adapt, &vf_state.csa);
	mi200_reset_vf_save_and_disable(adapt, idx_vf,
					vf_state.pci_cfg,
					vf_state.msix_tab);

	/* save mmio protection info before flr */
	mi200_reset_save_access_info(adapt, &access_info);

	/* enable all protection before flr */
	amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_ALL, false);

	/* do the FLR */
	ret = mi200_reset_vf_flr(adapt, idx_vf);

	if (adapt->psp.psp_program_guest_mc_settings) {
		/* program vf mc settings */
		AMDGV_DEBUG("program %s mc settings\n",
			    amdgv_idx_to_str(idx_vf));
		if (adapt->psp.psp_program_guest_mc_settings(adapt, idx_vf)) {
			ret = AMDGV_FAILURE;
		}
	}

	/* restore SDMA golden register settings */
	mi200_sdma_program_golden_settings(adapt);
	mi200_gfx_program_golden_settings(adapt);

	mi200_reset_restore_tcp_addr_config(adapt, &vf_state);

	mi200_reset_vf_restore(adapt, idx_vf,
				vf_state.pci_cfg, vf_state.msix_tab);

	/* init CSA */
	mi200_reset_restore_csa_config(adapt, &vf_state.csa);

	if (mi200_reset_enable_rlc(adapt, idx_vf) ||
	    mi200_reset_verify_flr(adapt))
		ret = AMDGV_FAILURE;

	mi200_reset_restore_active_vf_idx(adapt, &vf_state);
	mi200_reset_restore_time_quanta_info(adapt, &vf_state);

	mi200_reset_fini_flr(adapt, idx_vf);

	/* restore mmio protection info after flr */
	mi200_reset_restore_access_info(adapt, &access_info);

	oss_free(vf_state.pci_cfg);

	if (ret == 0) {
		ret = amdgv_sched_reset(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);
	}

	if (ret == 0)
		AMDGV_INFO("completed %s FLR\n", amdgv_idx_to_str(idx_vf));

	return ret;
}

static void mi200_reset_clear_all_pci_errors(struct amdgv_adapter *adapt)
{
	uint16_t data_16;
	uint32_t data_32;
	int      pos;

	/* Clear any pending status in PCI_STATUS (offset 0x06) */
	oss_pci_read_config_word(adapt->dev, PCI_STATUS, &data_16);
	data_16 &= 0xF900;  /* Isolate resettable error bits */
	if (data_16)
		oss_pci_write_config_word(adapt->dev,
				PCI_STATUS, data_16);

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID_EXP);
	/* Clear DevStatus (offset 0x62) */
	oss_pci_read_config_word(adapt->dev,
			pos + PCI_EXP_DEVSTA,
			&data_16);
	data_16 &= 0x000F;  /* Isolate resettable error bits */
	if (data_16)
		oss_pci_write_config_word(adapt->dev,
				pos + PCI_EXP_DEVSTA,
				data_16);

	pos = oss_pci_find_ext_cap(adapt->dev, PCI_EXT_CAP_ID_ERR);

	/* Clear uncorrectable error status (offset 0x154) */
	oss_pci_read_config_dword(adapt->dev,
			pos + PCI_ERR_UNCOR_STATUS,
			&data_32);
	if (data_32)
		oss_pci_write_config_dword(adapt->dev,
				pos + PCI_ERR_UNCOR_STATUS,
				data_32);

	/* Clear the correctable error status(offset 0x160) */
	oss_pci_read_config_dword(adapt->dev,
			pos + PCI_ERR_COR_STATUS,
			&data_32);
	if (data_32)
		oss_pci_write_config_dword(adapt->dev,
				pos + PCI_ERR_COR_STATUS,
				data_32);
}

static void mi200_reset_save_vfs(struct amdgv_adapter *adapt,
				struct mi200_whole_gpu_reset_state *reset_state)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		vf = &adapt->array_vf[idx_vf];

		if (!vf->configured)
			continue;

		/* save VF */
		mi200_reset_vf_save_and_disable(adapt, idx_vf,
						reset_state->vf_pci_cfg[idx_vf],
						reset_state->vf_msix_tab[idx_vf]);

		amdgv_mailbox_save_state(adapt, idx_vf);
	}
}

static void mi200_reset_restore_vfs(struct amdgv_adapter *adapt,
				struct mi200_whole_gpu_reset_state *reset_state)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		vf = &adapt->array_vf[idx_vf];
		if (!vf->configured)
			continue;

		/* enable MMIO register write VF access */
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, true);

		/* restore VF */
		mi200_reset_vf_restore(adapt, idx_vf,
				reset_state->vf_pci_cfg[idx_vf],
				reset_state->vf_msix_tab[idx_vf]);

		/* disable MMIO register write VF access */
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, false);

		amdgv_mailbox_restore_state(adapt, idx_vf);
	}
}

static void mi200_reset_save_pf_misc_reg(struct amdgv_adapter *adapt,
			struct mi200_whole_gpu_reset_state *reset_state)
{
	int entry;
	uint32_t *msix_tab;
	uint32_t *tab;

	/* save bif_bx strap0 */
	reset_state->bif_bx_strap0 = RREG32(mmCC_BIF_BX_STRAP0);

	msix_tab = reset_state->pf_msix_tab;
	/* save msix table
	 * based on mi200 hw, the table is located in BAR5 res
	 * and max vector number is 3 */
	for (entry = 0; entry < 3; entry++) {
		/* after decoding the MSI-X capability,
		 * the PBA is 5 and table offest is 0x42000 */
		tab = (uint32_t *)adapt->mmio + 4 * entry
				+ SOC15_REG_OFFSET(NBIO, 0,
						mmGFXMSIX_VECT0_ADDR_LO);
		msix_tab[entry * 4] = oss_mm_read32(tab);
		msix_tab[entry * 4 + 1] = oss_mm_read32(tab + 1);
		msix_tab[entry * 4 + 2] = oss_mm_read32(tab + 2);
		msix_tab[entry * 4 + 3] = oss_mm_read32(tab + 3);
	}
}

static void mi200_reset_save_pf(struct amdgv_adapter *adapt,
			struct mi200_whole_gpu_reset_state *reset_state)
{
	int idx;
	uint32_t *pci_cfg;

	pci_cfg = reset_state->pf_pci_cfg;

	/* Save the pf pci cfg space */
	for (idx = 0; idx < PCI_CONFIG_SIZE; idx += 4) {
		oss_pci_read_config_dword(adapt->dev, idx, pci_cfg);
		pci_cfg++;
	}

	mi200_reset_save_pf_misc_reg(adapt, reset_state);
}


static void mi200_reset_restore_pf_misc_reg(struct amdgv_adapter *adapt,
			struct mi200_whole_gpu_reset_state *reset_state)
{
	int entry;
	uint32_t *tab;
	uint32_t *val;

	/* resotre bif_bx strap0 */
	WREG32(mmCC_BIF_BX_STRAP0, reset_state->bif_bx_strap0);

	/* Resore the PF msix table */
	for (entry = 0; entry < 3; entry++) {
		tab = (uint32_t *)adapt->mmio + 4 * entry
				+ SOC15_REG_OFFSET(NBIO, 0,
						mmGFXMSIX_VECT0_ADDR_LO);
		val = &reset_state->pf_msix_tab[entry * 4];

		oss_mm_write32(tab, val[0]);
		oss_mm_write32(tab + 1, val[1]);
		oss_mm_write32(tab + 2, val[2]);
		oss_mm_write32(tab + 3, val[3]);
	}
}

static void mi200_reset_restore_pf(struct amdgv_adapter *adapt,
		struct mi200_whole_gpu_reset_state *reset_state)
{
	int idx;

	for (idx = 0; idx < ARRAY_SIZE(restore_tbl) ; ++idx) {
		if (restore_tbl[idx].offset >= CAP_SRIOV_OFFSET)
			break;

		restore_reg(adapt, &restore_tbl[idx], reset_state);
	}

	mi200_reset_restore_pf_misc_reg(adapt, reset_state);
}

int mi200_reset_trigger_whole_gpu_reset(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t tmp;
	uint32_t bit_s3_int;
	struct mi200_reset_access_info access_info;
	struct mi200_whole_gpu_reset_state *reset_state;

	/* need SMU FW loaded and responding to do WHOLE_GPU_RESET */
	if (mi200_smu_13_0_get_fw_loaded_status(adapt) == 0) {
		AMDGV_ERROR("SMU FW not responding. Unable to do GPU_RESET\n");
		return AMDGV_FAILURE;
	}

	/* alloc memory for whole GPU reset */
	reset_state = oss_zalloc(
			sizeof(struct mi200_whole_gpu_reset_state));
	if (reset_state == NULL) {
		AMDGV_ERROR("Failed to alloc memory for whole GPU reset\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("start whole gpu reset\n");

	/* save mmio protection info before WHOLE_GPU_RESET */
	mi200_reset_save_access_info(adapt, &access_info);

	/* enable all protection before WHOLE_GPU_RESET */
	mi200_reset_enable_mmio_protection(adapt);

	mi200_reset_save_pf(adapt, reset_state);
	amdgv_reset_save_sriov(adapt);

	/* force S3 engine hung */
	bit_s3_int = RREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_3));
	tmp = bit_s3_int | ATOM_S3_ASIC_GUI_ENGINE_HUNG;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_3), tmp);

	ret = mi200_mode1_reset(adapt);
	if (ret) {
		goto exit;
	}

	mi200_reset_restore_pf(adapt, reset_state);

	/*check mode1 reset finish after restore_pci_config*/
	ret = mi200_wait_mode1_reset_completion(adapt);
	if (ret)
		goto exit;

	/* disable S3 engine hung state */
	tmp = bit_s3_int & ~ATOM_S3_ASIC_GUI_ENGINE_HUNG;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_3), tmp);

	/* clear VBIOS status */
	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_7));
	tmp &= ~ATOM_ASIC_INIT_COMPLETE;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_7), tmp);

	amdgv_reset_restore_sriov(adapt);

	/* restore mmio protection info after PF_FLR or WHOLE_GPU_RESET */
	mi200_reset_restore_access_info(adapt, &access_info);

exit:
	oss_free(reset_state);

	if (ret)
		amdgv_put_error(
				AMDGV_PF_IDX,
				AMDGV_ERROR_RESET_GPU_FAILED,
				0
		);
	else
		AMDGV_INFO("complete whole gpu reset\n");

	return ret;
}

void mi200_clear_dummy_mode_after_reset(struct amdgv_adapter *adapt)
{
	uint32_t baco_cntl;

	baco_cntl = RREG32(SOC15_REG_OFFSET(NBIO, 0, mmBACO_CNTL));

	if (baco_cntl &
		(BACO_CNTL__BACO_DUMMY_EN_MASK | BACO_CNTL__BACO_EN_MASK)) {
		baco_cntl &= ~(BACO_CNTL__BACO_DUMMY_EN_MASK |
				BACO_CNTL__BACO_EN_MASK);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, mmBACO_CNTL), baco_cntl);
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
static int mi200_reset_whole_gpu_reset(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int idx;
	uint32_t tmp;
	uint32_t bit_s3_int;
	struct mi200_reset_access_info access_info;
	struct mi200_whole_gpu_reset_state *reset_state;
	struct amdgv_hive_info *hive;

	/* need SMU FW loaded and responding to do WHOLE_GPU_RESET */
	if (mi200_smu_13_0_get_fw_loaded_status(adapt) == 0) {
		AMDGV_ERROR("SMU FW not responding. Unable to do GPU_RESET\n");
		return AMDGV_FAILURE;
	}

	/* alloc memory for whole GPU reset */
	reset_state = oss_zalloc(
			sizeof(struct mi200_whole_gpu_reset_state));
	if (reset_state == NULL) {
		AMDGV_ERROR("Failed to alloc memory for whole GPU reset\n");
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("start whole gpu reset\n");

	mi200_reset_clear_all_pci_errors(adapt);

	/* save mmio protection info before WHOLE_GPU_RESET */
	mi200_reset_save_access_info(adapt, &access_info);

	/* enable all protection before WHOLE_GPU_RESET */
	mi200_reset_enable_mmio_protection(adapt);

	mi200_reset_save_vfs(adapt, reset_state);

	mi200_irqmgr_save_and_fini(adapt, &reset_state->irqmgr_reset_state);

	mi200_reset_save_pf(adapt, reset_state);

	amdgv_reset_save_sriov(adapt);

	/* fini HW */
	for (idx = adapt->num_funcs - 1; idx >= 0; idx--)
		if (adapt->init_funcs[idx]->hw_fini)
			adapt->init_funcs[idx]->hw_fini(adapt);

	/* force S3 engine hung */
	bit_s3_int = RREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_3));
	tmp = bit_s3_int | ATOM_S3_ASIC_GUI_ENGINE_HUNG;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_3), tmp);

	if (adapt->reset.in_xgmi_chain_reset) {
		hive = amdgv_get_xgmi_hive(adapt);
		if (!hive)
			goto exit;
		task_barrier_full(&hive->tb_chain_reset, hive->number_adapters);
		ret = mi200_mode1_reset(adapt);
		if (ret) {
			goto exit;
		}
	} else {
		ret = mi200_mode1_reset(adapt);
		if (ret) {
			goto exit;
		}
	}

	mi200_reset_restore_pf(adapt, reset_state);

	/*check mode1 reset finish after restore_pci_config*/
	ret = mi200_wait_mode1_reset_completion(adapt);
	if (ret) {
		goto exit;
	}

	/* disable S3 engine hung state */
	tmp = bit_s3_int & ~ATOM_S3_ASIC_GUI_ENGINE_HUNG;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_3), tmp);

	/* clear VBIOS status */
	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_7));
	tmp &= ~ATOM_ASIC_INIT_COMPLETE;
	WREG32(SOC15_REG_OFFSET(NBIO, 0, mmBIOS_SCRATCH_7), tmp);

	/* re-init HW */
	for (idx = 0; idx < adapt->num_funcs; idx++) {
		if (adapt->init_funcs[idx]->hw_init) {
			ret = adapt->init_funcs[idx]->hw_init(adapt);
			if (ret) {
				amdgv_print_failed_init_name(adapt, false, adapt->init_funcs[idx]->name);
				amdgv_put_error(
						AMDGV_PF_IDX,
						AMDGV_ERROR_DRIVER_HW_INIT_FAIL,
						0
				);

				goto exit;
			}
		}
	}

	mi200_irqmgr_restore_and_init(adapt, &reset_state->irqmgr_reset_state);

	mi200_reset_restore_vfs(adapt, reset_state);

	/* restore mmio protection info after PF_FLR or WHOLE_GPU_RESET */
	mi200_reset_restore_access_info(adapt, &access_info);

exit:
	oss_free(reset_state);

	if (ret)
		amdgv_put_error(
				AMDGV_PF_IDX,
				AMDGV_ERROR_RESET_GPU_FAILED,
				0
		);
	else
		AMDGV_INFO("complete whole gpu reset\n");

	return ret;
}

struct amdgv_gpu_reset_funcs mi200_reset_funcs = {
	.save_vddgfx_state = mi200_reset_save_vddgfx_state,
	.trigger_vf_flr = mi200_reset_trigger_vf_flr,
	.gpu_reset_and_reinit = mi200_reset_whole_gpu_reset,
};

static int mi200_reset_sw_init(struct amdgv_adapter *adapt)
{
	adapt->reset.reset_num = 0;
	adapt->reset.reset_state = false;
	adapt->reset.in_xgmi_chain_reset = false;
	adapt->reset.funcs = &mi200_reset_funcs;

	return 0;
}

static int mi200_reset_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->reset.funcs = NULL;

	return 0;
}

static int mi200_reset_hw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *entry;

	if (adapt->xgmi.phy_nodes_num > 1) {

		hive = amdgv_get_xgmi_hive(adapt);
		if (!hive)
			return AMDGV_FAILURE;

		if (!adapt->reset.in_xgmi_chain_reset) {
			/* Sync counter to get the last adapt that passed */
			if (oss_atomic_inc_return(&hive->tb_drv_init.thread_count) == adapt->xgmi.phy_nodes_num) {
				/* reset counter after the last adapt is identified */
				while (oss_atomic_dec_return(&hive->tb_drv_init.thread_count) > 0)
					;
				AMDGV_INFO("Last initialized device in hive initiating XGMI Topology update.\n");
				amdgv_list_for_each_entry(entry, &hive->adapt_list,
					struct amdgv_adapter, xgmi.head) {
						if (entry->status == AMDGV_STATUS_HW_INIT)
							amdgv_sched_queue_event(entry, AMDGV_PF_IDX,
								AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY, AMDGV_SCHED_BLOCK_ALL);
						else
							amdgv_sched_queue_event_no_signal(entry, AMDGV_PF_IDX,
								AMDGV_EVENT_SCHED_UPDATE_TOPOLOGY, AMDGV_SCHED_BLOCK_ALL);
				}
			}
		} else {
			amdgv_psp_xgmi_set_topology_info(adapt, hive);
		}
	}

	return 0;
}

static int mi200_reset_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_reset_func = {
	.name = "mi200_reset_func",
	.sw_init = mi200_reset_sw_init,
	.sw_fini = mi200_reset_sw_fini,
	.hw_init = mi200_reset_hw_init,
	.hw_fini = mi200_reset_hw_fini,
};
