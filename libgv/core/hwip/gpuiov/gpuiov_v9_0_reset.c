/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv_irqmgr.h>
#include <amdgv_gart.h>
#include <amdgv_sched_internal.h>

#include "gpuiov_v9_0_reset.h"
#include "hwip/gc/gfx_v12_1.h"
#include "hwip/sdma/sdma_v7_1.h"

#include "asic_reg/NBIO/nbio_6_3_2_offset.h"
#include "asic_reg/NBIO/nbio_6_3_2_sh_mask.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

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
	{ 0x00c2, 2, "MSIX Msg Cntl" }
};

static int gpuiov_v9_0_reset_pf_allowed(struct amdgv_adapter *adapt, uint32_t active_vf_mask)
{
	uint32_t pf_xcc_mask = amdgv_sched_get_xcc_mask_by_vf(adapt, AMDGV_PF_IDX);
	uint32_t idx_vf = 0;

	for_each_id(idx_vf, active_vf_mask) {
		if (pf_xcc_mask & amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
			return false;
		}
	}

	return true;
}

static void gpuiov_v9_0_reset_save_and_disable_vf_access(struct amdgv_adapter *adapt,
					    struct gpuiov_v9_0_reset_access_info *access_info)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	access_info->fb_access_info = RREG32_SOC15(NBIO, 0, regBIF_BX0_VF_FB_EN);
	access_info->doorbell_access_info = RREG32_SOC15(NBIO, 0, regBIF_BX0_VF_DOORBELL_EN);
	access_info->register_write_access_info = RREG32_SOC15(NBIO, 0, regBIF_BX0_VF_REGWR_EN);

	WREG32_SOC15(NBIO, 0, regBIF_BX0_VF_FB_EN, 0);
	WREG32_SOC15(NBIO, 0, regBIF_BX0_VF_DOORBELL_EN, 0);
	WREG32_SOC15(NBIO, 0, regBIF_BX0_VF_REGWR_EN, 0);
}

static void gpuiov_v9_0_restore_vf_access(struct amdgv_adapter *adapt,
					    struct gpuiov_v9_0_reset_access_info *access_info)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	WREG32_SOC15(NBIO, 0, regBIF_BX0_VF_FB_EN, access_info->fb_access_info);
	WREG32_SOC15(NBIO, 0, regBIF_BX0_VF_DOORBELL_EN, access_info->doorbell_access_info);
	WREG32_SOC15(NBIO, 0, regBIF_BX0_VF_REGWR_EN, access_info->register_write_access_info);
}

static void gpuiov_v9_0_reset_vf_save_and_disable(struct amdgv_adapter *adapt, uint32_t idx_vf,
					    uint32_t *pci_cfg, uint32_t *msix_tab)
{
	uint32_t i = 0;
	struct amdgv_vf_device *vf;
	int entry;
	uint32_t *tab;

	if (adapt->flags & AMDGV_FLAG_ENABLE_SVM) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_NO_ACCESS_PCI_REGION, 0);
		return;
	}

	/* Save the vf pci cfg space */
	for (i = 0; i < PCI_CONFIG_SIZE; i += 4) {
		vf = &adapt->array_vf[idx_vf];
		oss_pci_read_config_dword(vf->dev, i, pci_cfg);
		pci_cfg++;
	}

	if (vf->res_mapped && vf->res.mmio) {
		for (entry = 0; entry < 3; entry++) {
			tab = (uint32_t *)vf->res.mmio + 4 * entry +
			      SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_GFXMSIX_VECT0_ADDR_LO);
			msix_tab[entry * 4] = oss_mm_read32(tab);
			msix_tab[entry * 4 + 1] = oss_mm_read32(tab + 1);
			msix_tab[entry * 4 + 2] = oss_mm_read32(tab + 2);
			msix_tab[entry * 4 + 3] = oss_mm_read32(tab + 3);
		}
	}

	if (idx_vf != AMDGV_PF_IDX)
		oss_pci_write_config_word(vf->dev, PCI_COMMAND, PCI_COMMAND_INTX_DISABLE);
}

static void gpuiov_v9_0_reset_save_vfs(struct amdgv_adapter *adapt,
				 struct gpuiov_v9_0_whole_gpu_reset_state *reset_state)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *vf;

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		vf = &adapt->array_vf[idx_vf];
		if (!vf->configured)
			continue;

		gpuiov_v9_0_reset_vf_save_and_disable(adapt,
						      idx_vf, reset_state->vf_pci_cfg[idx_vf],
						      reset_state->vf_msix_tab[idx_vf]);
		amdgv_mailbox_save_state(adapt, idx_vf);
	}
}

static void gpuiov_v9_0_reset_save_pf(struct amdgv_adapter *adapt,
				struct gpuiov_v9_0_whole_gpu_reset_state *reset_state)
{
	int i;
	int entry;
	uint32_t *tab;
	uint32_t *pci_cfg;
	uint32_t *msix_tab;

	pci_cfg = reset_state->pf_pci_cfg;
	msix_tab = reset_state->pf_msix_tab;

	/* Save the pf pci cfg space */
	for (i = 0; i < PCI_CONFIG_SIZE; i += 4) {
		oss_pci_read_config_dword(adapt->dev, i, pci_cfg);
		pci_cfg++;
	}

	for (entry = 0; entry < 3; entry++) {
		tab = (uint32_t *)adapt->mmio + 4 * entry +
		      SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_GFXMSIX_VECT0_ADDR_LO);
		msix_tab[entry * 4] = oss_mm_read32(tab);
		msix_tab[entry * 4 + 1] = oss_mm_read32(tab + 1);
		msix_tab[entry * 4 + 2] = oss_mm_read32(tab + 2);
		msix_tab[entry * 4 + 3] = oss_mm_read32(tab + 3);
	}
}

static void gpuiov_v9_0_reset_clear_all_pci_errors(struct amdgv_adapter *adapt)
{
	uint16_t data_16;
	uint32_t data_32;
	int pos;

	oss_pci_read_config_word(adapt->dev, PCI_STATUS, &data_16);
	data_16 &= 0xF900; /* Isolate resettable error bits */
	if (data_16)
		oss_pci_write_config_word(adapt->dev, PCI_STATUS, data_16);

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID_EXP);
	oss_pci_read_config_word(adapt->dev, pos + PCI_EXP_DEVSTA, &data_16);
	data_16 &= 0x000F; /* Isolate resettable error bits */
	if (data_16)
		oss_pci_write_config_word(adapt->dev, pos + PCI_EXP_DEVSTA, data_16);

	pos = oss_pci_find_ext_cap(adapt->dev, PCI_EXT_CAP_ID_ERR);
	oss_pci_read_config_dword(adapt->dev, pos + PCI_ERR_UNCOR_STATUS, &data_32);
	if (data_32)
		oss_pci_write_config_dword(adapt->dev, pos + PCI_ERR_UNCOR_STATUS, data_32);

	oss_pci_read_config_dword(adapt->dev, pos + PCI_ERR_COR_STATUS, &data_32);
	if (data_32)
		oss_pci_write_config_dword(adapt->dev, pos + PCI_ERR_COR_STATUS, data_32);
}

static void gpuiov_v9_0_reset_vf_restore(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   uint32_t *pci_cfg, uint32_t *msix_tab)

{
	struct amdgv_vf_device *vf;
	int entry;
	uint32_t *val, *tab;
	uint32_t idx;

	if (adapt->flags & AMDGV_FLAG_ENABLE_SVM) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_NO_ACCESS_PCI_REGION, 0);
		return;
	}

	/* Restore the vf pci cfg space */
	for (idx = 0; idx < PCI_CONFIG_SIZE; idx += 4) {
		vf = &adapt->array_vf[idx_vf];
		oss_pci_write_config_dword(vf->dev, idx, *pci_cfg);
		pci_cfg++;
	}

	if (vf->res_mapped && vf->res.mmio) {
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, true);
		for (entry = 0; entry < 3; entry++) {
			tab = (uint32_t *)vf->res.mmio + 4 * entry +
			SOC15_REG_OFFSET(NBIO, 0, regRCC_DEV0_EPF0_GFXMSIX_VECT0_ADDR_LO);
			val = &msix_tab[entry * 4];

			oss_mm_write32(tab, val[0]);
			oss_mm_write32(tab + 1, val[1]);
			oss_mm_write32(tab + 2, val[2]);
			oss_mm_write32(tab + 3, val[3]);
		}
	}
}

static void gpuiov_v9_0_reset_restore_vfs(struct amdgv_adapter *adapt,
				    struct gpuiov_v9_0_whole_gpu_reset_state *reset_state)
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
		gpuiov_v9_0_reset_vf_restore(adapt, idx_vf, reset_state->vf_pci_cfg[idx_vf],
				       reset_state->vf_msix_tab[idx_vf]);

		/* disable MMIO register write VF access */
		amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE,
					   false);

		amdgv_mailbox_restore_state(adapt, idx_vf);
	}
}

static void __restore_reg(struct amdgv_adapter *adapt, struct pf_pcie_restore *entry,
			struct gpuiov_v9_0_whole_gpu_reset_state *reset_state)
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


static void gpuiov_v9_0_reset_restore_pf(struct amdgv_adapter *adapt,
				   struct gpuiov_v9_0_whole_gpu_reset_state *reset_state)
{
	int i, entry;
	uint32_t *tab;
	uint32_t *val;

	for (i = 0; i < ARRAY_SIZE(restore_tbl); ++i) {
		__restore_reg(adapt, &restore_tbl[i], reset_state);
	}

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

static int gpuiov_v9_0_reset_pf_flr(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint16_t val;
	int pos = 0;

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID_EXP);
	if (!pos)
		return AMDGV_FAILURE;

	oss_pci_write_config_dword(adapt->dev, cfgBIF_CFG_DEV0_EPF0_PCIE_VENDOR_SPECIFIC1, 0);

	oss_pci_read_config_word(adapt->dev, PCI_COMMAND, &val);
	val &= (~PCI_COMMAND_MASTER);
	oss_pci_write_config_word(adapt->dev, PCI_COMMAND, val);

	amdgv_wait_for_pci_cfg(adapt, adapt->dev, pos + PCIE_DEVICE_STATUS,
			       PCIE_DEVICE_STATUS__TRANS_PEND, 0, 2,
			       AMDGV_TIMEOUT(TIMEOUT_PCI_TRANS),
			       AMDGV_WAIT_CHECK_EQ, 0);

	oss_pci_read_config_word(adapt->dev, pos + PCI_EXP_DEVCTL, &val);
	val |= PCI_EXP_DEVCTL_BCR_FLR;
	oss_pci_write_config_word(adapt->dev, pos + PCI_EXP_DEVCTL, val);

	/* Wait for FLR Complete indication. */
	ret = amdgv_wait_for_pci_cfg(adapt, adapt->dev,
				     cfgBIF_CFG_DEV0_EPF0_PCIE_VENDOR_SPECIFIC1,
				     0xffffffff, 1, 4,
				     AMDGV_TIMEOUT(TIMEOUT_SMU_REG),
				     AMDGV_WAIT_CHECK_EQ, 0);
	if (ret)
		return ret;

	oss_pci_read_config_word(adapt->dev, PCI_COMMAND, &val);
	val |= PCI_COMMAND_MASTER;
	oss_pci_write_config_word(adapt->dev, PCI_COMMAND, val);

	return ret;
}

static int gpuiov_v9_0_reset_hw_for_reload(struct amdgv_adapter *adapt, bool is_unload)
{
	int ret = 0;
	struct gpuiov_v9_0_whole_gpu_reset_state *reset_state;

	reset_state = oss_zalloc(sizeof(struct gpuiov_v9_0_whole_gpu_reset_state));
	if (reset_state == NULL) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct gpuiov_v9_0_whole_gpu_reset_state));
		return AMDGV_FAILURE;
	}

	gpuiov_v9_0_reset_clear_all_pci_errors(adapt);
	amdgv_reset_save_sriov(adapt);
	gpuiov_v9_0_reset_save_pf(adapt, reset_state);

	if (adapt->xgmi.connected_to_cpu)
		ret = gpuiov_v9_0_reset_pf_flr(adapt);
	else
		ret = amdgv_powerplay_mode0_reset(adapt);

	if (ret)
		goto exit;

	gpuiov_v9_0_reset_restore_pf(adapt, reset_state);
	amdgv_reset_restore_sriov(adapt);

exit:
	oss_free(reset_state);

	if (ret) {
		if (adapt->xgmi.connected_to_cpu)
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_FLR_FAILED, AMDGV_PF_IDX);
		else
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_GPU_FAILED, 0);
	}

	return ret;
}

static int gpuiov_v9_0_gpu_reset_and_reinit(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int i;
	struct gpuiov_v9_0_reset_access_info access_info;
	struct gpuiov_v9_0_whole_gpu_reset_state *reset_state;

	reset_state = oss_zalloc(sizeof(struct gpuiov_v9_0_whole_gpu_reset_state));
	if (reset_state == NULL) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct gpuiov_v9_0_whole_gpu_reset_state));
		return AMDGV_FAILURE;
	}

	gpuiov_v9_0_reset_clear_all_pci_errors(adapt);
	gpuiov_v9_0_reset_save_and_disable_vf_access(adapt, &access_info);
	gpuiov_v9_0_reset_save_vfs(adapt, reset_state);
	gpuiov_v9_0_reset_save_pf(adapt, reset_state);
	amdgv_reset_save_sriov(adapt);

	for (i = adapt->num_funcs - 1; i >= 0; i--)
		if (adapt->init_funcs[i]->hw_fini)
			adapt->init_funcs[i]->hw_fini(adapt);

	if (adapt->xgmi.connected_to_cpu)
		ret = amdgv_powerplay_mode2_reset(adapt);
	else
		ret = amdgv_powerplay_mode0_reset(adapt);

	if (ret)
		goto exit;

	gpuiov_v9_0_reset_restore_pf(adapt, reset_state);

	if (amdgv_ras_intr_triggered())
		amdgv_ras_intr_cleared();

	//@TODO: re-enable when RAS is supported
	// if (!adapt->umc.is_pmfw_managed_eeprom) {
	// 	ret = amdgv_umc_replace_bad_pages(adapt);
	// 	if (ret)
	// 	goto exit;
	// }

	/* re-init HW */
	for (i = 0; i < adapt->num_funcs; i++) {
		if (adapt->init_funcs[i]->hw_init) {
			ret = adapt->init_funcs[i]->hw_init(adapt);
			if (ret) {
				amdgv_put_log(i, AMDGV_LOG_DRIVER_HW_INIT_FAIL, 0);
				goto exit;
			}
		}
	}

	gpuiov_v9_0_reset_restore_vfs(adapt, reset_state);
	gpuiov_v9_0_restore_vf_access(adapt, &access_info);

exit:
	oss_free(reset_state);

	if (ret)
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_GPU_FAILED, 0);

	return ret;
}

static void gpuiov_v9_0_reset_save_active_vf_idx(struct amdgv_adapter *adapt,
						 struct gpuiov_v9_0_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id (world_switch_id, amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (!world_switch->enabled)
			continue;

		for_each_id (hw_sched_id, world_switch->hw_sched_mask) {
			amdgv_gpuiov_get_active_vfs(adapt, hw_sched_id,
				&vf_state->active_vfs[world_switch->sched_block]);
		}
	}
}

static void gpuiov_v9_0_reset_save_time_quanta_info(struct amdgv_adapter *adapt,
						    struct gpuiov_v9_0_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id (world_switch_id, amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (!world_switch->enabled)
			continue;
		for_each_id (hw_sched_id, world_switch->hw_sched_mask) {
			amdgv_gpuiov_get_time_quanta_option(adapt, hw_sched_id,
				&vf_state->quanta_option[world_switch->sched_block]);
			amdgv_gpuiov_get_time_quanta_index(adapt, vf_state->idx_vf, hw_sched_id,
				&vf_state->quanta_index[world_switch->sched_block]);
		}
	}
}

static void gpuiov_v9_0_reset_restore_active_vf_idx(struct amdgv_adapter *adapt,
						    struct gpuiov_v9_0_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id (world_switch_id, amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (!world_switch->enabled)
			continue;
		for_each_id (hw_sched_id, world_switch->hw_sched_mask) {
			amdgv_gpuiov_set_active_vfs(adapt, hw_sched_id,
				vf_state->active_vfs[world_switch->sched_block]);
		}
	}
}

static void gpuiov_v9_0_reset_restore_time_quanta_info(struct amdgv_adapter *adapt,
						       struct gpuiov_v9_0_vf_flr_state *vf_state)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t hw_sched_id = 0;
	uint32_t world_switch_id = 0;

	for_each_id (world_switch_id, amdgv_sched_get_world_switch_mask(adapt, vf_state->idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (!world_switch->enabled)
			continue;
		for_each_id (hw_sched_id, world_switch->hw_sched_mask) {
			amdgv_gpuiov_set_time_quanta_option(adapt, hw_sched_id,
				vf_state->quanta_option[world_switch->sched_block]);
			amdgv_gpuiov_set_time_quanta_index(adapt, vf_state->idx_vf, hw_sched_id,
				vf_state->quanta_index[world_switch->sched_block]);
		}
	}
}

static int gpuiov_v9_0_reset_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = 0;
	uint16_t val;
	struct amdgv_vf_device *vf;
	int pos = 0;

	vf = &adapt->array_vf[idx_vf];
	pos = oss_pci_find_capability(vf->dev, PCI_CAP_ID_EXP);
	if (!pos)
		return AMDGV_FAILURE;

	oss_pci_read_config_word(vf->dev, PCI_COMMAND, &val);
	val &= (~PCI_COMMAND_MASTER);
	oss_pci_write_config_word(vf->dev, PCI_COMMAND, val);

	amdgv_wait_for_pci_cfg(adapt, vf->dev, pos + PCIE_DEVICE_STATUS,
			       PCIE_DEVICE_STATUS__TRANS_PEND, 0, 2,
			       AMDGV_TIMEOUT(TIMEOUT_PCI_TRANS),
			       AMDGV_WAIT_CHECK_EQ, 0);

	ret = amdgv_powerplay_flr_reset(adapt, idx_vf);

	oss_pci_read_config_word(vf->dev, PCI_COMMAND, &val);
	val |= PCI_COMMAND_MASTER;
	oss_pci_write_config_word(vf->dev, PCI_COMMAND, val);

	return ret;
}

static int gpuiov_v9_0_reset_trigger_soft_pf_flr(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct gpuiov_v9_0_reset_access_info access_info;
	struct gpuiov_v9_0_whole_gpu_reset_state *reset_state;

	reset_state = oss_zalloc(sizeof(struct gpuiov_v9_0_whole_gpu_reset_state));
	if (!reset_state) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			sizeof(struct gpuiov_v9_0_whole_gpu_reset_state));
		return AMDGV_FAILURE;
	}

	gpuiov_v9_0_reset_save_vfs(adapt, reset_state);
	gpuiov_v9_0_reset_save_pf(adapt, reset_state);
	amdgv_reset_save_sriov(adapt);

	gpuiov_v9_0_reset_save_and_disable_vf_access(adapt, &access_info);

	ret = gpuiov_v9_0_reset_vf_flr(adapt, AMDGV_PF_IDX);
	if (ret)
		goto failed;

	ret = amdgv_gfx_check_rlc_autoload_complete(adapt);
	if (ret)
		goto failed;

	if (adapt->psp.psp_program_guest_mc_settings) {
		ret = adapt->psp.psp_program_guest_mc_settings(adapt, AMDGV_PF_IDX);
		if (ret)
			goto failed;
	}

	amdgv_mmhub_gart_enable(adapt);
	amdgv_gfxhub_gart_enable(adapt);

	amdgv_reset_restore_sriov(adapt);
	gpuiov_v9_0_reset_restore_pf(adapt, reset_state);
	gpuiov_v9_0_reset_restore_vfs(adapt, reset_state);

	gpuiov_v9_0_restore_vf_access(adapt, &access_info);

	ret = sdma_v7_1_hw_resume(adapt, amdgv_sched_get_xcc_mask_by_vf(adapt, AMDGV_PF_IDX));
	if (ret)
		goto failed;
	ret = gfx_v12_1_hw_resume(adapt, amdgv_sched_get_xcc_mask_by_vf(adapt, AMDGV_PF_IDX));
	if (ret)
		goto failed;

	amdgv_gfx_set_clockgating_state(adapt, true);

	ret = amdgv_sched_reset(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL);
	if (ret)
		goto failed;

	amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_PF_SOFT_FLR_DONE, 0);

failed:
	oss_free(reset_state);
	return ret;
}

static int gpuiov_v9_0_reset_trigger_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = 0;
	struct gpuiov_v9_0_vf_flr_state vf_state;
	struct gpuiov_v9_0_reset_access_info access_info;

	if (idx_vf == AMDGV_PF_IDX)
		return gpuiov_v9_0_reset_trigger_soft_pf_flr(adapt);

	vf_state.pci_cfg = oss_zalloc(PCI_CONFIG_SIZE);
	if (!vf_state.pci_cfg) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			PCI_CONFIG_SIZE);
		return AMDGV_FAILURE;
	}

	vf_state.idx_vf = idx_vf;

	gpuiov_v9_0_reset_save_active_vf_idx(adapt, &vf_state);
	gpuiov_v9_0_reset_save_time_quanta_info(adapt, &vf_state);

	gpuiov_v9_0_reset_vf_save_and_disable(adapt, idx_vf, vf_state.pci_cfg, vf_state.msix_tab);
	gpuiov_v9_0_reset_save_and_disable_vf_access(adapt, &access_info);

	ret = gpuiov_v9_0_reset_vf_flr(adapt, idx_vf);
	if (ret)
		goto failed;

	ret = amdgv_gfx_check_rlc_autoload_complete(adapt);
	if (ret)
		goto failed;

	amdgv_mmhub_gart_enable(adapt);
	amdgv_gfxhub_gart_enable(adapt);

	gpuiov_v9_0_reset_vf_restore(adapt, idx_vf, vf_state.pci_cfg, vf_state.msix_tab);

	gpuiov_v9_0_reset_restore_active_vf_idx(adapt, &vf_state);
	gpuiov_v9_0_reset_restore_time_quanta_info(adapt, &vf_state);

	if (adapt->psp.psp_program_guest_mc_settings) {
		ret = adapt->psp.psp_program_guest_mc_settings(adapt, idx_vf);
		if (ret)
			goto failed;
	}
	amdgv_gpuiov_set_vf_fb(adapt, idx_vf, adapt->array_vf[idx_vf].fb_offset,
				  adapt->array_vf[idx_vf].fb_size);

	gpuiov_v9_0_restore_vf_access(adapt, &access_info);

	ret = sdma_v7_1_hw_resume(adapt, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf));
	if (ret)
		goto failed;

	ret = gfx_v12_1_hw_resume(adapt, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf));
	if (ret)
		goto failed;

	ret = amdgv_sched_reset(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);
	if (ret)
		goto failed;

failed:
	oss_free(vf_state.pci_cfg);
	return ret;
}

struct amdgv_gpu_reset_funcs gpuiov_v9_0_reset_funcs = {
	.save_vddgfx_state = NULL,
	.notify_engine_status = NULL,
	.reset_pf_allowed = gpuiov_v9_0_reset_pf_allowed,
	.trigger_vf_flr = gpuiov_v9_0_reset_trigger_vf_flr,
	.reset_hw_for_reload = gpuiov_v9_0_reset_hw_for_reload,
	.gpu_reset_and_reinit = gpuiov_v9_0_gpu_reset_and_reinit,
};

static int gpuiov_v9_0_reset_sw_init(struct amdgv_adapter *adapt)
{
	adapt->reset.reset_num = 0;
	adapt->reset.reset_state = false;
	adapt->reset.in_xgmi_chain_reset = false;
	adapt->reset.funcs = &gpuiov_v9_0_reset_funcs;

	return 0;
}

static int gpuiov_v9_0_reset_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->reset.funcs = NULL;

	return 0;
}

static int gpuiov_v9_0_reset_hw_init(struct amdgv_adapter *adapt)
{
	return 0;
}

static int gpuiov_v9_0_reset_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func gpuiov_v9_0_reset_func = {
	.name = "gpuiov_v9_0_reset_func",
	.sw_init = gpuiov_v9_0_reset_sw_init,
	.sw_fini = gpuiov_v9_0_reset_sw_fini,
	.hw_init = gpuiov_v9_0_reset_hw_init,
	.hw_fini = gpuiov_v9_0_reset_hw_fini,
};
