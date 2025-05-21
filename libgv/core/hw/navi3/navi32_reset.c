/*
 * Copyright (C) 2022  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv_pci_def.h>
#include <amdgv_irqmgr.h>

#include <atombios/atomfirmware.h>
#include <atombios/atom.h>
#include <atombios/atombios.h>

#include "navi32_reg_inc.h"
#include "navi32_nbio_mapper.h"
#include "navi32_nbio.h"
#include "navi32_gpuiov.h"
#include "navi32_irqmgr.h"
#include "navi32_psp.h"
#include "navi32_gfx.h"
#include "navi32_sdma.h"
#include "navi32_clockgating.h"
#include "navi32_powerplay.h"
#include "navi32_reset.h"
#include "navi32_smu_ppsmc_wrapper.h"
#include "navi32_gc.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

struct navi32_reset_pci_info {
	int offset;
	int size;
	char *name;
};

#define PCI_CONFIG_SIZE 1024
#define MSIX_TAB_COUNT	12

#define NAVI32_ME1_MAX_PIPE_PER_ME 4
#define NAVI32_ME1_MAX_QUEUE_PER_PIPE 4
#define NAVI32_ME3_MAX_PIPE_PER_ME 2
#define NAVI32_ME3_MAX_QUEUE_PER_PIPE 1

struct navi32_reset_pci_state {
	uint32_t idx_vf;
	uint32_t *pci_cfg;
#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	uint32_t *msix_tab;
#endif
};

/* list all writeable cfgs need to be restored and in a correct sequence */
static struct navi32_reset_pci_info navi32_reset_pci_info_list[] = {
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

struct navi32_reset_vf_flr_state {
	uint32_t idx_vf;
	uint32_t strap4;

	uint32_t active_vfs[AMDGV_MAX_NUM_HW_SCHED];
	uint32_t quanta_option[AMDGV_MAX_NUM_HW_SCHED];
	uint32_t quanta_index[AMDGV_MAX_NUM_HW_SCHED];

	uint32_t csa;
};

struct navi32_reset_access_info {
	uint32_t fb_access_info;
	uint32_t doorbell_access_info;
	uint32_t register_write_access_info;
};

/* a dummy funciton */
static int navi32_reset_save_vddgfx_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return 0;
}

static void navi32_reset_save_active_vf_idx(struct amdgv_adapter *adapt,
					   struct navi32_reset_vf_flr_state *vf_state)
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

static void navi32_reset_restore_active_vf_idx(struct amdgv_adapter *adapt,
					      struct navi32_reset_vf_flr_state *vf_state)
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

static void navi32_reset_save_time_quanta_info(struct amdgv_adapter *adapt,
					      struct navi32_reset_vf_flr_state *vf_state)
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

static void navi32_reset_restore_time_quanta_info(struct amdgv_adapter *adapt,
						 struct navi32_reset_vf_flr_state *vf_state)
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

static void navi32_reset_save_csa_config(struct amdgv_adapter *adapt,
					struct navi32_reset_vf_flr_state *vf_state)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_CNTXT;
	oss_pci_read_config_dword(adapt->dev, offset, &vf_state->csa);
}

static void navi32_reset_restore_csa_config(struct amdgv_adapter *adapt,
					   struct navi32_reset_vf_flr_state *vf_state)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_CNTXT;
	oss_pci_write_config_dword(adapt->dev, offset, vf_state->csa);
}

static int navi32_reset_alloc_pci_config(struct amdgv_adapter *adapt,
					struct navi32_reset_pci_state *pci_state)
{
	pci_state->pci_cfg = oss_zalloc(PCI_CONFIG_SIZE);
	if (!pci_state->pci_cfg)
		return AMDGV_FAILURE;
#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	pci_state->msix_tab = oss_zalloc(MSIX_TAB_COUNT * sizeof(uint32_t));
	if (!pci_state->msix_tab) {
		oss_free(pci_state->pci_cfg);
		pci_state->pci_cfg = NULL;
		return AMDGV_FAILURE;
	}
#endif
	return 0;
}

static int navi32_reset_free_pci_config(struct amdgv_adapter *adapt,
				       struct navi32_reset_pci_state *pci_state)
{
	if (pci_state->pci_cfg) {
		oss_free(pci_state->pci_cfg);
		pci_state->pci_cfg = NULL;
	}
#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	if (pci_state->msix_tab) {
		oss_free(pci_state->msix_tab);
		pci_state->msix_tab = NULL;
	}
#endif
	return 0;
}

static void navi32_reset_save_pci_config(struct amdgv_adapter *adapt,
					struct navi32_reset_pci_state *pci_state)
{
	uint32_t idx_vf;
	struct amdgv_vf_device *vf;
	uint32_t offset;
	int i, entry;
	uint32_t *tab_reg;

	idx_vf = pci_state->idx_vf;
	vf = &adapt->array_vf[idx_vf];

	if (pci_state->pci_cfg) {
#ifdef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
		offset = SOC15_REG_OFFSET_SMN_NBIO_BLOCK(idx_vf,
							 BIF_CFG,
							 VENDOR_ID);
#endif

		/* Save the vf pci cfg space */
		i = 0;
		for (entry = 0; entry < PCI_CONFIG_SIZE; entry += 4) {
			if (idx_vf == AMDGV_PF_IDX)
				oss_pci_read_config_dword(adapt->dev, entry,
							  &pci_state->pci_cfg[i]);
			else
#ifndef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
				oss_pci_read_config_dword(vf->dev, entry,
							  &pci_state->pci_cfg[i]);
#else
				pci_state->pci_cfg[i] = RREG32_SMN(offset + entry);
#endif
			i++;
		}
	}

#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	/* save msix table
	 * the table is located in BAR5 res and max vector number is 3
	 */
	if (pci_state->msix_tab &&
	    (idx_vf == AMDGV_PF_IDX || (vf->res_mapped && vf->res.mmio))) {
		uint32_t *tab;

		offset = SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, RCC,
						     GFXMSIX_VECT0_ADDR_LO);

		for (i = 0, entry = 0; i < 3; i++, entry += 4) {
			/* after decoding the MSI-X capability,
			 * the PBA is 5 and table offest is 0x42000
			 */
			if (idx_vf == AMDGV_PF_IDX)
				tab = (uint32_t *)adapt->mmio + entry + offset;
			else
				tab = (uint32_t *)vf->res.mmio + entry + offset;

			pci_state->msix_tab[entry + 0] = oss_mm_read32(tab + 0);
			pci_state->msix_tab[entry + 1] = oss_mm_read32(tab + 1);
			pci_state->msix_tab[entry + 2] = oss_mm_read32(tab + 2);
			pci_state->msix_tab[entry + 3] = oss_mm_read32(tab + 3);
		}
	}
#endif

	if (idx_vf != AMDGV_PF_IDX) {
		/* disable user context vm by setting START ADDR bigger
		 * than END ADDR to make all vm address invalid
		 */
		if (vf->res_mapped && vf->res.mmio) {
			for (entry = 0; entry < 15; ++entry) {
				tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
					  gc_v11_0_3_get_page_table_start_addr_lo32(adapt);
				oss_mm_write32(tab_reg, ~0);

				tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
					  gc_v11_0_3_get_page_table_start_addr_hi32(adapt);
				oss_mm_write32(tab_reg, ~0);

				tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
					  gc_v11_0_3_get_page_table_end_addr_lo32(adapt);
				oss_mm_write32(tab_reg, 0);

				tab_reg = (uint32_t *)vf->res.mmio + (2 * entry) +
					  gc_v11_0_3_get_page_table_end_addr_hi32(adapt);
				oss_mm_write32(tab_reg, 0);
			}
		}

		/* disable the vf by clearing the command */
#ifndef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
		oss_pci_write_config_word(vf->dev, PCI_COMMAND, PCI_COMMAND__INT_DIS);
#else
		offset = SOC15_REG_OFFSET_SMN_NBIO_BLOCK(idx_vf,
							 BIF_CFG,
							 COMMAND);
		WREG16_SMN(offset, PCI_COMMAND__INT_DIS);
#endif
	}
}

static void navi32_reset_save_access_info(struct amdgv_adapter *adapt,
					   struct navi32_reset_access_info *access_info)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	access_info->fb_access_info = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_FB_EN));
	access_info->doorbell_access_info = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_DOORBELL_EN));
	access_info->register_write_access_info = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_REGWR_EN));
}

static void navi32_reset_restore_access_info(struct amdgv_adapter *adapt,
						struct navi32_reset_access_info *access_info)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_FB_EN),
			access_info->fb_access_info);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_DOORBELL_EN),
			access_info->doorbell_access_info);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_REGWR_EN),
			access_info->register_write_access_info);
}

static void navi32_reset_enable_mmio_protection(struct amdgv_adapter *adapt)
{
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION)
		return;

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_FB_EN), 0);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_DOORBELL_EN), 0);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_REGWR_EN), 0);
}

static void navi32_reset_restore_pci_config(struct amdgv_adapter *adapt,
					   struct navi32_reset_pci_state *pci_state)
{
	struct amdgv_vf_device *vf;
	uint32_t offset;
#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	int i;
#endif
	int entry, count;
	uint32_t idx_vf;

	idx_vf = pci_state->idx_vf;
	vf = &adapt->array_vf[idx_vf];

	/* enable MMIO register write VF access */
	amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, true);

	if (pci_state->pci_cfg) {
		if (idx_vf == AMDGV_PF_IDX) {
			/* Restore the PF pci cfg space */
			struct navi32_reset_pci_info *pci_info;
			uint8_t *pci_u8;

			count = ARRAY_SIZE(navi32_reset_pci_info_list);
			for (entry = 0; entry < count; entry++) {
				pci_info = &navi32_reset_pci_info_list[entry];
				if (pci_info->offset >= CAP_SRIOV_OFFSET)
					break;

				pci_u8 = (uint8_t *)pci_state->pci_cfg;
				pci_u8 += pci_info->offset;

				switch (pci_info->size) {
				case 1:
					oss_pci_write_config_byte(adapt->dev, pci_info->offset,
								  *pci_u8);
					break;
				case 2:
					oss_pci_write_config_word(adapt->dev, pci_info->offset,
								  *(uint16_t *)pci_u8);
					break;
				case 4:
					oss_pci_write_config_dword(adapt->dev,
								   pci_info->offset,
								   *(uint32_t *)pci_u8);
					break;
				}
			}
		} else {
			/* Restore the vf pci cfg space */
			uint32_t *pci_cfg = pci_state->pci_cfg;

#ifdef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
			offset = SOC15_REG_OFFSET_SMN_NBIO_BLOCK(idx_vf,
								 BIF_CFG,
								 VENDOR_ID);
#endif

			for (entry = 0; entry < PCI_CONFIG_SIZE; entry += 4) {
#ifndef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
				oss_pci_write_config_dword(vf->dev, entry, *pci_cfg);
#else
				WREG32_SMN(offset + entry, *pci_cfg);
#endif
				pci_cfg++;
			}
		}
	}

#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	/* Restore the vf msix table */
	if (pci_state->msix_tab &&
	    (idx_vf == AMDGV_PF_IDX || (vf->res_mapped && vf->res.mmio))) {
		uint32_t *tab;

		offset = SOC15_REG_OFFSET_NBIO_BLOCK(NBIO, 0, idx_vf, RCC,
						     GFXMSIX_VECT0_ADDR_LO);

		for (i = 0, entry = 0; i < 3; i++, entry += 4) {
			if (idx_vf == AMDGV_PF_IDX)
				tab = (uint32_t *)adapt->mmio + entry + offset;
			else
				tab = (uint32_t *)vf->res.mmio + entry + offset;

			oss_mm_write32(tab + 0, pci_state->msix_tab[entry + 0]);
			oss_mm_write32(tab + 1, pci_state->msix_tab[entry + 1]);
			oss_mm_write32(tab + 2, pci_state->msix_tab[entry + 2]);
			oss_mm_write32(tab + 3, pci_state->msix_tab[entry + 3]);
		}
	}

#endif

	/* disable MMIO register write VF access */
	amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_MMIO_REG_WRITE, false);
}

static int navi32_reset_clear_all_pci_errors(struct amdgv_adapter *adapt)
{
	uint16_t data_16;
	uint32_t data_32;
	int pos;

	/* Clear any pending status in PCI_STATUS (offset 0x06) */
	oss_pci_read_config_word(adapt->dev, PCI_STATUS, &data_16);
	data_16 &= 0xF900; /* Isolate resettable error bits */
	if (data_16)
		oss_pci_write_config_word(adapt->dev, PCI_STATUS, data_16);

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID__PCIE);

	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID__PCIE);
		return AMDGV_FAILURE;
	}

	/* Clear DevStatus (offset 0x62) */
	oss_pci_read_config_word(adapt->dev, pos + PCIE_DEVICE_STATUS, &data_16);
	data_16 &= 0x000F; /* Isolate resettable error bits */
	if (data_16)
		oss_pci_write_config_word(adapt->dev, pos + PCIE_DEVICE_STATUS, data_16);

	pos = oss_pci_find_ext_cap(adapt->dev, PCIE_EXT_CAP_ID__AER);
	if (!pos) {
		AMDGV_ERROR("this device does not support ext capability: %x\n", PCIE_EXT_CAP_ID__AER);
		return AMDGV_FAILURE;
	}

	/* Clear uncorrectable error status (offset 0x154) */
	oss_pci_read_config_dword(adapt->dev, pos + PCIE_EXT_AER_UNCOR_STATUS, &data_32);
	if (data_32)
		oss_pci_write_config_dword(adapt->dev, pos + PCIE_EXT_AER_UNCOR_STATUS,
					   data_32);

	/* Clear the correctable error status(offset 0x160) */
	oss_pci_read_config_dword(adapt->dev, pos + PCIE_EXT_AER_COR_ERR_STATUS, &data_32);
	if (data_32)
		oss_pci_write_config_dword(adapt->dev, pos + PCIE_EXT_AER_COR_ERR_STATUS,
					   data_32);

	return 0;
}

static void navi32_reset_clear_smu_flr_status(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DWNP_DEV0_0_PCIEP_HW_DEBUG));
	tmp = REG_SET_FIELD(tmp, PCIEP_HW_DEBUG, HW_08_DEBUG, 0);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_DWNP_DEV0_0_PCIEP_HW_DEBUG), tmp);

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_GFX_RST_CNTL), 0);
}

static int navi32_trigger_vf_flr_by_msg(struct amdgv_adapter *adapt, uint32_t param)
{
	int ret = 0;

	ret = navi32_powerplay_send_msg_with_param(
			adapt,
			SMU_13_0_MSG__TRIGGER_VF_FLR,
			param);

	return ret;
}


static int navi32_reset_wait_smu_flr_complete(struct amdgv_adapter *adapt, int timeout)
{
	int wait_ret;

	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_GFX_RST_CNTL),
					BIF_BX0_GFX_RST_CNTL__GFX_RST_FINISH_INDICATION_MASK, 1, timeout, AMDGV_WAIT_CHECK_EQ, 0);
	if (wait_ret) {
		AMDGV_ERROR("TIMEOUT after %d ms waiting for SMU to complete FLR\n");
		AMDGV_ERROR("GFX_RST_CNTL is %llx", RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_GFX_RST_CNTL)));
		return AMDGV_FAILURE;
	}

	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, 0, regGFX_IMU_RLC_STATUS),
					GFX_IMU_RLC_STATUS__RLC_ALIVE_MASK, 0, timeout, AMDGV_WAIT_CHECK_NE, 0);
	if (wait_ret) {
		AMDGV_ERROR("TIMEOUT after %d ms waiting for SMU to complete FLR\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_reset_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
#ifndef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
	int pos;
	struct amdgv_vf_device *vf;
#else
	uint32_t offset;
#endif
	uint16_t val;
	int wait_ret;
	int ret = 0;
	uint32_t strap4;

#ifndef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
	vf = &adapt->array_vf[idx_vf];
	pos = oss_pci_find_capability(vf->dev, PCI_CAP_ID__PCIE);

	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID__PCIE);
		return AMDGV_FAILURE;
	}
#endif

	/* Before trigger FLR, clear status SMU will set for FLR complete */
	navi32_reset_clear_smu_flr_status(adapt);

	/* disable device bus mastering */
#ifndef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
	oss_pci_read_config_word(vf->dev, PCI_COMMAND, &val);
	val &= (~PCI_COMMAND__BUS_MASTER_EN);
	oss_pci_write_config_word(vf->dev, PCI_COMMAND, val);
#else
	offset = SOC15_REG_OFFSET_SMN_NBIO_BLOCK(idx_vf, BIF_CFG, COMMAND);
	val = RREG16_SMN(offset);
	val &= (~PCI_COMMAND__BUS_MASTER_EN);
	WREG16_SMN(offset, val);
#endif

	/* enable FLR strap */
	if (!(adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY)) {
		strap4 = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4));
		strap4 = REG_SET_FIELD(strap4, RCC_DEV0_EPF0_STRAP4, STRAP_FLR_EN_DEV0_F0, 1);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4), strap4);
	}

	/* wait for transaction done */
#ifndef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
	wait_ret = amdgv_wait_for_pci_cfg(adapt, vf->dev, pos + PCIE_DEVICE_STATUS, PCIE_DEVICE_STATUS__TRANS_PEND, 0, 2, AMDGV_TIMEOUT(TIMEOUT_PCI_TRANS), AMDGV_WAIT_CHECK_EQ, 0);
#else
	offset = SOC15_REG_OFFSET_SMN_NBIO_BLOCK(idx_vf,
						 BIF_CFG,
						 DEVICE_STATUS);
	wait_ret = amdgv_wait_for_pci_cfg(adapt, NULL, offset, PCIE_DEVICE_STATUS__TRANS_PEND, 0, 2, AMDGV_TIMEOUT(TIMEOUT_PCI_TRANS), AMDGV_WAIT_CHECK_EQ, 0);
#endif

	if (wait_ret)
		AMDGV_WARN("Abort data transaction on %s for FLR\n", amdgv_idx_to_str(idx_vf));

	/* For VF_FLR & PFSoft_FLR, SMU read BIF_PF0_VF_FLR_INTR_STS.
	 * If this register is set, then SMU apply VF_FLR sequence.
	 * The “mask” SMU uses for vf_flr sequence comes from reading
	 * this register
	 * therefore, there is no difference in the way SMU handles
	 * vf_flr and Pfsoft_flr
	 */

	/* use SMU msg to trigger FLR instead of PCIe control bit*/
	ret = navi32_trigger_vf_flr_by_msg(adapt, 1 << idx_vf);
	if (ret) {
		AMDGV_ERROR("Send Trigger VF FLR msg failed\n");
		ret = AMDGV_FAILURE;
	}

	/* After an FLR has been initiated by writing a 1b to
	 * the Initiate Function Level Reset bit, the Function
	 * must complete the FLR within 100 ms. PCIe spec 6.6.2.
	 */
	if (ret == 0)
		ret = navi32_reset_wait_smu_flr_complete(adapt, AMDGV_TIMEOUT(TIMEOUT_RESET));

	/* disable FLR strap */
	if (!(adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY)) {
		strap4 = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4));
		strap4 = REG_SET_FIELD(strap4, RCC_DEV0_EPF0_STRAP4, STRAP_FLR_EN_DEV0_F0, 0);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4), strap4);
	}

	/* enable bus mastering */
#ifndef EXCLUDE_VF_DEVICE_PCI_CONFIG_ACCESS
	oss_pci_read_config_word(vf->dev, PCI_COMMAND, &val);
	val |= PCI_COMMAND__BUS_MASTER_EN;
	oss_pci_write_config_word(vf->dev, PCI_COMMAND, val);
#else
	offset = SOC15_REG_OFFSET_SMN_NBIO_BLOCK(idx_vf, BIF_CFG, COMMAND);
	val = RREG16_SMN(offset);
	val |= PCI_COMMAND__BUS_MASTER_EN;
	WREG16_SMN(offset, val);
#endif

	return ret;
}

static int navi32_reset_wait_for_grbm(struct amdgv_adapter *adapt)
{
	uint32_t grbm_status = 0;
	uint32_t grbm_status2 = 0;
	int wait_ret;

	/* wait for a clean state */
	wait_ret = gc_v11_0_3_wait_grbm_clean(adapt);

	grbm_status = gc_v11_0_3_read_grbm_status(adapt);
	grbm_status2 = gc_v11_0_3_read_grbm_status2(adapt);
	AMDGV_DEBUG("GRBM_STATUS = 0x%x GRBM_STATUS2 = 0x%x\n", grbm_status, grbm_status2);

	if (wait_ret)
		return AMDGV_FAILURE;
	else
		return 0;
}

/* this is PF_FLR! This should reset all VFs + PF */
static int navi32_reset_pf_flr(struct amdgv_adapter *adapt)
{
	return 0;
}

static int navi32_reset_trigger_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = 0;
	struct navi32_reset_vf_flr_state vf_state;
	struct navi32_reset_pci_state pci_state;
	struct navi32_reset_access_info access_info;
	enum psp_status psp_ret;

	if (idx_vf == AMDGV_PF_IDX)
		AMDGV_INFO("start SOFT_PF_FLR\n");
	else
		AMDGV_INFO("start %s FLR\n", amdgv_idx_to_str(idx_vf));

	/* need SMU FW loaded and responding to do VF_FLR */
	if (navi32_powerplay_get_fw_loaded_status(adapt) == 0) {
		AMDGV_ERROR("SMU FW not responding. Unable to do FLR\n");
		return AMDGV_FAILURE;
	}

	vf_state.idx_vf = idx_vf;
	pci_state.idx_vf = idx_vf;
	ret = navi32_reset_alloc_pci_config(adapt, &pci_state);
	if (ret != 0) {
		AMDGV_ERROR("failed to allocate heap for save/restore\n");
		return AMDGV_FAILURE;
	}

	navi32_reset_save_active_vf_idx(adapt, &vf_state);
	navi32_reset_save_time_quanta_info(adapt, &vf_state);
	navi32_reset_save_csa_config(adapt, &vf_state);
	navi32_reset_save_pci_config(adapt, &pci_state);
	navi32_gfx_halt_gpu_state(adapt);

	/* save mmio protection info before flr */
	navi32_reset_save_access_info(adapt, &access_info);

	/* enable all protection before flr */
	amdgv_gpuiov_set_vf_access(adapt, idx_vf, AMDGV_VF_ACCESS_ALL, false);

	/* do the FLR */
	ret = navi32_reset_vf_flr(adapt, idx_vf);
	if (ret == 0) {
		/* make sure TOC, RLCG and RLCV are re-enabled (by PSP) */
		psp_ret = navi32_gfx_check_rlc_autoload_complete(adapt);
		if (psp_ret != PSP_STATUS__SUCCESS) {
			navi32_reset_free_pci_config(adapt, &pci_state);
			AMDGV_ERROR("failed at RLC_AUTOLOAD_COMPLETE\n");
			return AMDGV_FAILURE;
		} else if (navi32_reset_wait_for_grbm(adapt)) {
			navi32_reset_free_pci_config(adapt, &pci_state);
			AMDGV_ERROR("failed at GRBM_STATUS2 not clean\n");
			return AMDGV_FAILURE;
		}
	}

	/* restore mmio protection info after flr */
	navi32_reset_restore_access_info(adapt, &access_info);

	if (ret == 0) {
		navi32_gfx_atc_ats_invalidate(adapt);

		/* restore GFX/SDMA golden register settings */
		navi32_gfx_program_golden_settings(adapt);
		navi32_sdma_program_golden_settings(adapt);
	}

	/* restore VF_PCI CONFIG and CSA_CONFIG */
	navi32_reset_restore_pci_config(adapt, &pci_state);
	navi32_reset_restore_csa_config(adapt, &vf_state);
	navi32_gfx_unhalt_gpu_state(adapt);
	navi32_reset_restore_time_quanta_info(adapt, &vf_state);
	navi32_reset_restore_active_vf_idx(adapt, &vf_state);
	navi32_reset_free_pci_config(adapt, &pci_state);

	/* enable all feature */
	navi32_powerplay_enable_smu_features(adapt);

	/* re-send no dal message to enable CLK DPM */
	navi32_powerplay_notify_no_dal(adapt);

	/* re-enable CGCG */
	adapt->sched.cg_control(adapt, false);
	adapt->sched.cg_control(adapt, true);

	if (ret == 0) {
		ret = amdgv_sched_reset(adapt, idx_vf, AMDGV_SCHED_BLOCK_ALL);
	}

	/* NOTE: if (r), failure/error message already printed */
	if (ret == 0) {
		if (idx_vf == AMDGV_PF_IDX)
			AMDGV_INFO("completed SOFT_PF_FLR\n");
		else
			AMDGV_INFO("completed %s FLR\n", amdgv_idx_to_str(idx_vf));
	}

	return ret;
}

/* do BACO or mode1 reset */
static int navi32_reset_hardware_reset(struct amdgv_adapter *adapt, uint32_t mode)
{
	int ret = 0;
	int i;
	uint32_t idx_vf;
	uint32_t tmp;
	uint32_t bif_bx_strap0;
	uint32_t bif_db_int;
	uint32_t bit_s3_int = 0;
	struct amdgv_vf_device *vf;
	struct navi32_reset_pci_state pci_state[AMDGV_MAX_VF_SLOT] = {0};

	/*save config space*/
	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++) {
		if (idx_vf != AMDGV_PF_IDX) {
			vf = &adapt->array_vf[idx_vf];
			if (!vf->configured)
				continue;
		}
		pci_state[idx_vf].idx_vf = idx_vf;
		ret = navi32_reset_alloc_pci_config(adapt, &pci_state[idx_vf]);
		if (ret) {
			AMDGV_ERROR("failed to allocate heap for %s save/restore\n",
				    amdgv_idx_to_str(idx_vf));
			goto exit_whole_gpu_reset;
		}
	}

	ret = navi32_reset_clear_all_pci_errors(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to reset clear all pci errors\n");
		goto exit_whole_gpu_reset;
	}

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		vf = &adapt->array_vf[idx_vf];
		if (!vf->configured)
			continue;

		/* save VF */
		navi32_reset_save_pci_config(adapt, &pci_state[idx_vf]);
		amdgv_mailbox_save_state(adapt, idx_vf);
	}

	/* Save the pf pci cfg space */
	navi32_reset_save_pci_config(adapt, &pci_state[AMDGV_PF_IDX]);

	amdgv_reset_save_sriov(adapt);

	/* save bif_bx strap0 */
	bif_bx_strap0 = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_CC_BIF_BX_STRAP0));

	/* clean up SW */
	for (i = adapt->num_funcs - 1; i >= 0; i--) {
		if (adapt->init_funcs[i]->pre_reset)
			adapt->init_funcs[i]->pre_reset(adapt);
	}

	/* Disable doorbell interrupt before PF_FLR or WHOLE_GPU_RESET */
	bif_db_int = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL));
	tmp = REG_SET_FIELD(bif_db_int, BIF_DOORBELL_INT_CNTL, DOORBELL_INTERRUPT_DISABLE, 1);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL), tmp);

	if (mode == AMDGV_RESET_MODE1) {
		/* force S3 engine hung */
		bit_s3_int = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_3));
		tmp = bit_s3_int | ATOM_S3_ASIC_GUI_ENGINE_HUNG;
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_3), tmp);

		ret = navi32_mode1_reset(adapt);

		for (i = adapt->num_funcs - 1; i >= 0; i--) {
			if (adapt->init_funcs[i]->post_reset)
				adapt->init_funcs[i]->post_reset(adapt);
		}

	}

	/* restore the pci cfg space */
	navi32_reset_restore_pci_config(adapt, &pci_state[AMDGV_PF_IDX]);

	if (mode == AMDGV_RESET_MODE1) {
		/*check mode1 reset finish after restore_pci_config*/
		ret = navi32_wait_mode1_reset_completion(adapt);
		if (ret)
			AMDGV_WARN("No mode 1 completion notice from SMU.\n");

		/* disable S3 engine hung state */
		tmp = bit_s3_int & ~ATOM_S3_ASIC_GUI_ENGINE_HUNG;
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_3), tmp);

		/* clear VBIOS status */
		tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7));
		tmp &= ~ATOM_ASIC_INIT_COMPLETE;
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7), tmp);
	}

	/* restore doorbell interrupt after PF_FLR or WHOLE_GPU_RESET */
	tmp = REG_SET_FIELD(bif_db_int, BIF_DOORBELL_INT_CNTL, DOORBELL_INTERRUPT_DISABLE, 0);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL), tmp);

	/* re-init HW */
	for (i = 0; i < adapt->num_funcs; i++) {
		if (adapt->init_funcs[i]->hw_init) {
			ret = adapt->init_funcs[i]->hw_init(adapt);
			if (ret) {
				ret = AMDGV_FAILURE;
				amdgv_print_failed_init_name(adapt, false, adapt->init_funcs[i]->name);
				amdgv_put_error(i, AMDGV_ERROR_DRIVER_HW_INIT_FAIL, 0);
				goto exit_whole_gpu_reset;
			}
		}
	}

	/* resotre bif_bx strap0 */
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_CC_BIF_BX_STRAP0), bif_bx_strap0);

	for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
		vf = &adapt->array_vf[idx_vf];
		if (!vf->configured)
			continue;

		/* restore VF */
		navi32_reset_restore_pci_config(adapt, &pci_state[idx_vf]);
		amdgv_mailbox_restore_state(adapt, idx_vf);
	}

exit_whole_gpu_reset:
	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++) {
		if (idx_vf != AMDGV_PF_IDX) {
			vf = &adapt->array_vf[idx_vf];
			if (!vf->configured)
				continue;
		}
		navi32_reset_free_pci_config(adapt, &pci_state[idx_vf]);
	}

	return ret;
}

/* do whole gpu reset without context save&restore */
int navi32_reset_whole_gpu_reset(struct amdgv_adapter *adapt, uint32_t mode)
{
	int ret = 0;
	uint32_t i;
	uint32_t reg_data, tmp;
	struct navi32_reset_access_info access_info;

	/* Disable doorbell interrupt before PF_FLR or WHOLE_GPU_RESET */
	reg_data = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL));
	tmp = REG_SET_FIELD(reg_data, BIF_DOORBELL_INT_CNTL, DOORBELL_INTERRUPT_DISABLE, 1);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL), tmp);

	/* save mmio protection info before PF_FLR or WHOLE_GPU_RESET */
	navi32_reset_save_access_info(adapt, &access_info);

	/* enable all protection before PF_FLR or WHOLE_GPU_RESET */
	navi32_reset_enable_mmio_protection(adapt);

	if (mode == AMDGV_RESET_MODE1) {
		uint32_t astate;
		uint32_t bif_bx_strap0;
		struct navi32_reset_pci_state pf_pci_state;

		/*save config space*/
		pf_pci_state.idx_vf = AMDGV_PF_IDX;
		ret = navi32_reset_alloc_pci_config(adapt, &pf_pci_state);
		if (ret) {
			AMDGV_ERROR("failed to allocate heap for PF save/restore\n");
			navi32_reset_free_pci_config(adapt, &pf_pci_state);
			return ret;
		}

		/* Save the pf pci cfg space */
		navi32_reset_save_pci_config(adapt, &pf_pci_state);
		amdgv_reset_save_sriov(adapt);

		/* save bif_bx strap0 */
		bif_bx_strap0 = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_CC_BIF_BX_STRAP0));

		/* force S3 engine hung */
		astate = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_3));
		tmp = astate | ATOM_S3_ASIC_GUI_ENGINE_HUNG;
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_3), tmp);

		ret = navi32_mode1_reset(adapt);

		/* restore the pci cfg space */
		navi32_reset_restore_pci_config(adapt, &pf_pci_state);

		/*check mode1 reset finish after restore_pci_config*/
		ret = navi32_wait_mode1_reset_completion(adapt);
		if (ret) {
			navi32_reset_free_pci_config(adapt, &pf_pci_state);
			return AMDGV_FAILURE;
		}

		/* disable S3 engine hung state */
		tmp = astate & ~ATOM_S3_ASIC_GUI_ENGINE_HUNG;
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_3), tmp);

		/* clear VBIOS status */
		tmp = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7));
		tmp &= ~ATOM_ASIC_INIT_COMPLETE;
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIOS_SCRATCH_7), tmp);

		/* resotre bif_bx strap0 */
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_CC_BIF_BX_STRAP0), bif_bx_strap0);
		amdgv_reset_restore_sriov(adapt);

		navi32_reset_free_pci_config(adapt, &pf_pci_state);
	}

	/* restore mmio protection info after PF_FLR or WHOLE_GPU_RESET */
	navi32_reset_restore_access_info(adapt, &access_info);

	/* restore doorbell interrupt after PF_FLR or WHOLE_GPU_RESET */
	tmp = REG_SET_FIELD(reg_data, BIF_DOORBELL_INT_CNTL, DOORBELL_INTERRUPT_DISABLE, 0);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL), tmp);

	/* disable VF FB access */
	if (adapt->flags & AMDGV_FLAG_VF_FB_PROTECTION) {
		for (i = 0; i < adapt->num_vf; i++) {
			amdgv_gpuiov_set_vf_access(adapt, i, AMDGV_VF_ACCESS_FB, false);
		}
	}

	return ret;
}

/* Whole GPU reset only can work with below assumptions:
 *  1) No world switch(SW&HW), No interrupt
 *  2) All active VFs should be notified before start whole gpu reset
 *  3) All active VFs should be notified after whole gpu reset done
 *  4) VF & PF pci cfg status (EXCEPT PF GPUIOV SETTING) should be saved
 *     and restored by GIM
 *  5) PF GPUIOV SETTING is supposed to be recovered by hw_init and handshake
 *     with guest driver for reset
 *  6) MSI-X table should be saved and retored for both PF and VF by GIM
 *  7) VF GPU driver should reinit GPU after reset
 */
static int navi32_reset_trigger_whole_gpu_reset(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct navi32_reset_access_info access_info;

	if (adapt->log_level < AMDGV_DEBUG_LEVEL)
		AMDGV_INFO("start whole gpu reset\n");
	else if (adapt->reset.reset_mode == AMDGV_RESET_PF_FLR)
		AMDGV_DEBUG("start whole gpu reset(PF_FLR)\n");
	else if (adapt->reset.reset_mode == AMDGV_RESET_MODE1)
		AMDGV_DEBUG("start whole gpu reset(MODE1_RESET)\n");
	else
		AMDGV_DEBUG("start whole gpu reset(BACO-IN/OUT)\n");

	/* save mmio protection info before PF_FLR or WHOLE_GPU_RESET */
	navi32_reset_save_access_info(adapt, &access_info);

	/* enable all protection before PF_FLR or WHOLE_GPU_RESET */
	navi32_reset_enable_mmio_protection(adapt);

	if (adapt->reset.reset_mode == AMDGV_RESET_PF_FLR) {
		/* PF_FLR is same as VF_FLR but
		 * PF_FLR will do FLR for PF as well as all active VF
		 * don't need to do hw_fini/hw_init. Just message SMU
		 */
		ret = navi32_reset_pf_flr(adapt);
	} else
		ret = navi32_reset_hardware_reset(adapt, adapt->reset.reset_mode);

	/* restore mmio protection info after PF_FLR or WHOLE_GPU_RESET */
	navi32_reset_restore_access_info(adapt, &access_info);

	/* NOTE: if (ret), failure/error message already printed */
	if (!ret) {
		AMDGV_INFO("complete whole gpu reset\n");
	} else {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_RESET_GPU_FAILED, 0);
	}
	return ret;
}

int navi32_reset_enter_power_saving(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int i;
	uint32_t tmp;

	/* need SMU FW loaded and responding to enter BACO */
	if (adapt->pp.pp_funcs->get_smu_fw_loaded_status(adapt) == 0) {
		AMDGV_ERROR("SMU FW not responding. Unable to enter power saving\n");
		return AMDGV_FAILURE;
	}

	adapt->reset.reset_state = true;
	adapt->pp.pf_pci_config.idx_vf = AMDGV_PF_IDX;
	adapt->pp.is_in_powersaving = true;
	/*save config space*/
	ret = navi32_reset_alloc_pci_config(adapt,
		(struct navi32_reset_pci_state *)&adapt->pp.pf_pci_config);
	if (ret) {
		AMDGV_ERROR("failed to allocate heap for PF save/restore\n");
		goto exit_powersaving;
	}

	ret = navi32_reset_clear_all_pci_errors(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to reset clear all pci errors\n");
		goto exit_powersaving;
	}

	/* Save the pf pci cfg space */
	navi32_reset_save_pci_config(adapt,
		(struct navi32_reset_pci_state *)&adapt->pp.pf_pci_config);

	amdgv_reset_save_sriov(adapt);

	/* save bif_bx strap0 */
	adapt->pp.pf_bif_bx_strap0 = RREG32(SOC15_REG_OFFSET(NBIO, 0,
		regBIF_BX0_CC_BIF_BX_STRAP0));

	/* fini HW */
	for (i = adapt->num_funcs - 1; i >= 0; i--)
		if (adapt->init_funcs[i]->hw_fini)
			adapt->init_funcs[i]->hw_fini(adapt);

	/* Disable doorbell interrupt before enter baco */
	adapt->pp.pf_bif_db_int = RREG32(SOC15_REG_OFFSET(NBIO, 0,
		regBIF_BX0_BIF_DOORBELL_INT_CNTL));
	tmp = REG_SET_FIELD(adapt->pp.pf_bif_db_int, BIF_DOORBELL_INT_CNTL,
		DOORBELL_INTERRUPT_DISABLE, 1);
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL), tmp);

	/* do BACO-IN/BACO-OUT */
	ret = adapt->pp.pp_funcs->enter_baco(adapt);

	if (ret)
		amdgv_device_set_status(adapt, AMDGV_STATUS_HW_LOST);
	else
		return ret;

exit_powersaving:

	adapt->pp.is_in_powersaving = false;
	adapt->reset.reset_state = false;

#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	if (adapt->pp.pf_pci_config.pci_cfg || adapt->pp.pf_pci_config.msix_tab) {
#else
	if (adapt->pp.pf_pci_config.pci_cfg) {
#endif
		navi32_reset_free_pci_config(adapt,
			(struct navi32_reset_pci_state *)&adapt->pp.pf_pci_config);
	}

	return ret;
}

int navi32_reset_exit_power_saving(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int i;
	uint32_t tmp;

	/* need SMU FW loaded and responding to exit BACO */
	if (adapt->pp.pp_funcs->get_smu_fw_loaded_status(adapt) == 0) {
		AMDGV_ERROR("SMU FW not responding. Unable to exit power saving\n");
		return AMDGV_FAILURE;
	}

	ret = adapt->pp.pp_funcs->exit_baco(adapt);
	if (!ret) {
		/* restore the pci cfg space */
		navi32_reset_restore_pci_config(adapt,
			(struct navi32_reset_pci_state *)&adapt->pp.pf_pci_config);

		/* restore doorbell interrupt after exit baco */
		tmp = REG_SET_FIELD(adapt->pp.pf_bif_db_int, BIF_DOORBELL_INT_CNTL,
			DOORBELL_INTERRUPT_DISABLE, 0);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_BIF_DOORBELL_INT_CNTL), tmp);

		/* re-init HW */
		for (i = 0; i < adapt->num_funcs; i++) {
			if (adapt->init_funcs[i]->hw_init) {
				ret = adapt->init_funcs[i]->hw_init(adapt);
				if (ret) {
					ret = AMDGV_FAILURE;
					amdgv_print_failed_init_name(adapt, false, adapt->init_funcs[i]->name);
					amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_HW_INIT_FAIL, 0);
					break;
				}
			}
		}
	}

	if (ret)
		amdgv_device_set_status(adapt, AMDGV_STATUS_HW_LOST);

	adapt->reset.reset_state = false;
	adapt->pp.is_in_powersaving = false;

	/* restore bif_bx strap0 */
	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_CC_BIF_BX_STRAP0),
		adapt->pp.pf_bif_bx_strap0);

#ifndef CONFIG_AMDGV_FLR_NOT_RESTORE_MSIX
	if (adapt->pp.pf_pci_config.pci_cfg || adapt->pp.pf_pci_config.msix_tab) {
#else
	if (adapt->pp.pf_pci_config.pci_cfg) {
#endif
		navi32_reset_free_pci_config(adapt,
			(struct navi32_reset_pci_state *)&adapt->pp.pf_pci_config);
	}

	return ret;
}

int navi32_reset_grbm_soft_reset_stage_1(struct amdgv_adapter *adapt, bool gl2c_bit)
{
	int ret = 0;
	uint32_t reg_data;
	uint32_t pipe, queue;

	/* 1. Write CP_INT_CNTL[21:18] = 0x0 */
	reg_data = RREG32_SOC15(GC, 0, regCP_INT_CNTL);
	reg_data &= 0xffc3ffff;
	WREG32_SOC15(GC, 0, regCP_INT_CNTL, reg_data);
	/* 2-3. enter safe mode */
	adapt->sched.rlc_safe_mode(adapt, true);
	/* 4. Write GRBM_GFX_CNTL with MEID/PipeID/QueueID for each compute and MES queue
		* Write CP_HQD_DEQUEUE_REQUEST to 0x2
		* Write SPI_COMPUTE_QUEUE_RESET to 0x1, only for compute queues, not MES since MES has no such connection */
	/* MEC, ME=1 */
	for (pipe = 0; pipe < NAVI32_ME1_MAX_PIPE_PER_ME; pipe++) {
		for (queue = 0; queue < NAVI32_ME1_MAX_QUEUE_PER_PIPE; queue++) {
			navi32_grbm_select(adapt, 1, pipe, queue, 0);
			WREG32_SOC15(GC, 0, regCP_HQD_DEQUEUE_REQUEST, 0x2);
			WREG32_SOC15(GC, 0, regSPI_COMPUTE_QUEUE_RESET, 0x1);
		}
	}

	/* MES, ME=3 */
	for (pipe = 0; pipe < NAVI32_ME3_MAX_PIPE_PER_ME; pipe++) {
		for (queue = 0; queue < NAVI32_ME3_MAX_QUEUE_PER_PIPE; queue++) {
			navi32_grbm_select(adapt, 3, pipe, queue, 0);
			WREG32_SOC15(GC, 0, regCP_HQD_DEQUEUE_REQUEST, 0x2);
		}
	}
	navi32_grbm_select(adapt, 0, 0, 0, 0);
	/* 5. Write the CP_VMID_RESET register with VMIDs 15:1 and all queue bits set to 1*/
	WREG32_SOC15(GC, 0, regCP_VMID_RESET, 0xfffffffe);

	/* 6-7. GL2C */
	if (gl2c_bit) {
		AMDGV_WARN("gl2c error detacted.\n");
		reg_data = RREG32_SOC15(GC, 0, regRLC_FED_DRVR_STATUS);
		reg_data = REG_SET_FIELD(reg_data, RLC_FED_DRVR_STATUS, PENDING, 0x2);
		WREG32_SOC15(GC, 0, regRLC_FED_DRVR_STATUS, reg_data);
		ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, 0, regRLC_FED_DRVR_STATUS),
				REG_FIELD_MASK(RLC_FED_DRVR_STATUS, PENDING), 0x1,
				AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ, 0);
		if (ret) {
			adapt->sched.rlc_safe_mode(adapt, false);
			return AMDGV_FAILURE;
		}
	}
	/* continue after page replaced */
	return 0;
}

int navi32_reset_grbm_soft_reset_stage_2(struct amdgv_adapter *adapt)
{
	uint32_t reg_data;

	uint32_t wait_ret, i;
	/* GC continue after bad page replaced */
	/* 8. Read the CP_VMID_RESET register three times*/
	RREG32_SOC15(GC, 0, regCP_VMID_RESET);
	RREG32_SOC15(GC, 0, regCP_VMID_RESET);
	RREG32_SOC15(GC, 0, regCP_VMID_RESET);
	/* 9 Write GRBM_SOFT_RESET(CP=GFX=CPF=CPC=CPG=SDMAx=1)*/
	reg_data = RREG32_SOC15(GC, 0, regGRBM_SOFT_RESET);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CP, 1);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_GFX, 1);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPF, 1);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPC, 1);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPG, 1);
	WREG32_SOC15(GC, 0, regGRBM_SOFT_RESET, reg_data);
	/* 10. read 8 times */
	for (i = 0; i < 8; i++)
		RREG32_SOC15(GC, 0, regGRBM_SOFT_RESET);
	/* 11. Write GRBM_SOFT_RESET(CP=GFX=CPF=CPC=CPG=SDMAx=0)*/
	reg_data = RREG32_SOC15(GC, 0, regGRBM_SOFT_RESET);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CP, 0);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_GFX, 0);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPF, 0);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPC, 0);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPG, 0);
	WREG32_SOC15(GC, 0, regGRBM_SOFT_RESET, reg_data);
	/* 12. Write CP_SOFT_RESET_CNTL.CMP_HQD_REG_RESET = 0x1 */
	reg_data = RREG32_SOC15(GC, 0, regCP_SOFT_RESET_CNTL);
	reg_data = REG_SET_FIELD(reg_data, CP_SOFT_RESET_CNTL,
					CMP_HQD_REG_RESET, 1);
	WREG32_SOC15(GC, 0, regCP_SOFT_RESET_CNTL, reg_data);
	/* 13 Write GRBM_SOFT_RESET(CP=GFX=CPF=CPC=CPG=SDMAx=1)*/
	reg_data = RREG32_SOC15(GC, 0, regGRBM_SOFT_RESET);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CP, 1);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_GFX, 1);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPF, 1);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPC, 1);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPG, 1);
	WREG32_SOC15(GC, 0, regGRBM_SOFT_RESET, reg_data);
	/* 14. read 8 times */
	for (i = 0; i < 8; i++)
		RREG32_SOC15(GC, 0, regGRBM_SOFT_RESET);
	/* 15. Write GRBM_SOFT_RESET (CP=GFX=CPF=CPC=CPG=SDMAx=0)*/
	reg_data = RREG32_SOC15(GC, 0, regGRBM_SOFT_RESET);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CP, 0);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_GFX, 0);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPF, 0);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPC, 0);
	reg_data = REG_SET_FIELD(reg_data, GRBM_SOFT_RESET,
					SOFT_RESET_CPG, 0);
	WREG32_SOC15(GC, 0, regGRBM_SOFT_RESET, reg_data);
	/* 16. Write mmCP_ME_CNTL=0x0 */
	WREG32_SOC15(GC, 0, regCP_ME_CNTL, 0);
	/* 17. Write mmCP_MEC_CNTL=0x0 */
	WREG32_SOC15(GC, 0, regCP_MEC_CNTL, 0);
	/* 18. wait CP_VMID_RESET to be 0 */
	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(GC, 0, regCP_VMID_RESET),
		0, 0, AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ, 0);
	if (wait_ret)
		AMDGV_ERROR("Soft reset failed to wait VMID_RESET to be 0\n");

	/* 19. set rlc drvr status to 0 */
	reg_data = RREG32_SOC15(GC, 0, regRLC_FED_DRVR_STATUS);
	if (REG_GET_FIELD(reg_data, RLC_FED_DRVR_STATUS, PENDING) == 1) {
		reg_data = REG_SET_FIELD(reg_data, RLC_FED_DRVR_STATUS, PENDING, 0x0);
		WREG32_SOC15(GC, 0, regRLC_FED_DRVR_STATUS, reg_data);
	}

	/* 20. Write CP_INT_CNTL[21:18] = 0xF */
	reg_data = RREG32_SOC15(GC, 0, regCP_INT_CNTL);
	reg_data |= 0x3c0000;
	WREG32_SOC15(GC, 0, regCP_INT_CNTL, reg_data);
	/* 21. exit safe mode */
	adapt->sched.rlc_safe_mode(adapt, false);

	return 0;
}

struct amdgv_gpu_reset_funcs navi32_reset_funcs = {
	.save_vddgfx_state = navi32_reset_save_vddgfx_state,
	.trigger_vf_flr = navi32_reset_trigger_vf_flr,
	.trigger_gpu_reset = navi32_reset_trigger_whole_gpu_reset,
};

static int navi32_reset_sw_init(struct amdgv_adapter *adapt)
{
	adapt->reset.reset_num = 0;
	adapt->reset.reset_state = false;
	adapt->reset.reset_mode = AMDGV_RESET_MODE1;
	adapt->reset.funcs = &navi32_reset_funcs;

	return 0;
}

static int navi32_reset_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->reset.funcs = NULL;

	return 0;
}

static int navi32_reset_hw_init(struct amdgv_adapter *adapt)
{
	adapt->reset.saved_rlcv_state = false;
	return 0;
}

static int navi32_reset_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func navi32_reset_func = {
	.name = "navi32_reset_func",
	.sw_init = navi32_reset_sw_init,
	.sw_fini = navi32_reset_sw_fini,
	.hw_init = navi32_reset_hw_init,
	.hw_fini = navi32_reset_hw_fini,
};
