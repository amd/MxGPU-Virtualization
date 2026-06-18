/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "atom.h"
#include "atom-bits.h"
#include "atombios.h"
#include "amdgv_device.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

uint32_t amdgv_atombios_get_fw_offset(struct amdgv_adapter *adapt, uint8_t fw_type)
{
	uint32_t i, fw_offset = 0, total_entries;
	uint16_t offset;
	int index;
	bool ret;
	uint8_t *vbios = adapt->vbios.image;
	struct atom_context *ctx = adapt->vbios.atom_context;
	PSP_DIRECTORY_TABLE_V2_5 *psptable_2_5;
	PSP_DIRECTORY_TABLE_V2_1 *psptable_2_1;
	PSP_DIRECTORY *psp_dir;

	if (!((fw_type == PSP_DIR_ENTRY_TYPE_PSP_FW_BOOT_LOADER) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_PSP_FW_TRUSTED_OS) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_SMU_OFF_CHIP_FW) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_SDMA0_FW) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_SDMA1_FW) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_RLCG_FW) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_RLCV_FW) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_MMSCHEDULER_FW) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_SECURITY_GASKET) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_PSP_SEC_POLICY_STAGE2) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_PSP_FW_SYSDRV) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_GPM_MEM) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_SRM_MEM) ||
	      (fw_type == PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_CNTL))) {
		return AMDGV_FAILURE;
	}

	index = GetIndexIntoMasterTable(DATA, PspDirectory);
	ret = amdgv_atom_parse_data_header(ctx, index, NULL, NULL, NULL, &offset);

	if (!ret)
		return AMDGV_FAILURE;

	psptable_2_5 = (PSP_DIRECTORY_TABLE_V2_5 *)(vbios + offset);
	psptable_2_1 = (PSP_DIRECTORY_TABLE_V2_1 *)(vbios + offset);

	if (psptable_2_5->table_header.ucTableFormatRevision == 1) {
		return AMDGV_FAILURE;
	}

	if (psptable_2_5->table_header.ucTableFormatRevision >= 2) {
		psp_dir = (PSP_DIRECTORY *)&psptable_2_5->psp_directory;
	} else {
		psp_dir = (PSP_DIRECTORY *)&psptable_2_1->psp_directory;
	}

	/* TotalEntries comes from the VBIOS image; clamp it to the fixed
	 * pspEntry[] capacity to avoid an out-of-bounds read (CWE-125).
	 */
	total_entries = min(psp_dir->Header.TotalEntries,
			    (uint32_t)PSP_DIRECTORY_MAX_ENTRIES);

	for (i = 0; i < total_entries; i++) {
		if (psp_dir->pspEntry[i].u32Type == fw_type) {
			fw_offset = psp_dir->pspEntry[i].Location;
		}
	}

	return fw_offset;
}

void amdgv_atombios_print_fw_version(struct amdgv_adapter *adapt, uint8_t fw_type,
				     uint32_t version)
{
	char *name;
	enum amdgv_firmware_id fw_id;

	switch (fw_type) {
	case PSP_DIR_ENTRY_TYPE_PSP_FW_BOOT_LOADER:
		fw_id = AMDGV_FIRMWARE_ID__PSP_BL;
		name = "PSP BL";
		break;
	case PSP_DIR_ENTRY_TYPE_PSP_FW_TRUSTED_OS:
		fw_id = AMDGV_FIRMWARE_ID__PSP_SOS;
		name = "PSP SOS";
		break;
	case PSP_DIR_ENTRY_TYPE_SMU_OFF_CHIP_FW:
		fw_id = AMDGV_FIRMWARE_ID__SMU;
		name = "SMU";
		break;
	case PSP_DIR_ENTRY_TYPE_SDMA0_FW:
		fw_id = AMDGV_FIRMWARE_ID__SDMA0;
		name = "SDMA0";
		break;
	case PSP_DIR_ENTRY_TYPE_SDMA1_FW:
		fw_id = AMDGV_FIRMWARE_ID__SDMA1;
		name = "SDMA1";
		break;
	case PSP_DIR_ENTRY_TYPE_RLCG_FW:
		fw_id = AMDGV_FIRMWARE_ID__RLC;
		name = "RLCG";
		break;
	case PSP_DIR_ENTRY_TYPE_RLCV_FW:
		fw_id = AMDGV_FIRMWARE_ID__RLC_V;
		name = "RLCV";
		break;
	case PSP_DIR_ENTRY_TYPE_MMSCHEDULER_FW:
		fw_id = AMDGV_FIRMWARE_ID__MMSCH;
		name = "MM_SCHEDULER";
		break;
	case PSP_DIR_ENTRY_TYPE_SECURITY_GASKET:
		fw_id = AMDGV_FIRMWARE_ID__MAX; // not exposed
		name = "PSP SECURITY GASKET";
		break;
	case PSP_DIR_ENTRY_TYPE_PSP_SEC_POLICY_STAGE2:
		fw_id = AMDGV_FIRMWARE_ID__SEC_POLICY_STAGE2;
		name = "PSP SECURITY POLICY";
		break;
	case PSP_DIR_ENTRY_TYPE_PSP_FW_SYSDRV:
		fw_id = AMDGV_FIRMWARE_ID__PSP_SYS;
		name = "PSP SYSDRV";
		break;
	case PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_GPM_MEM:
		fw_id = AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM;
		name = "RLC GPM MEM";
		break;
	case PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_SRM_MEM:
		fw_id = AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM;
		name = "RLC SRM MEM";
		break;
	case PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_CNTL:
		fw_id = AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL;
		name = "RLC CNTL";
		break;
	default:
		fw_id = AMDGV_FIRMWARE_ID__MAX;
		name = "UNKNOWN";
		break;
	}

	if ((fw_type == PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_GPM_MEM) ||
	    (fw_type == PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_SRM_MEM) ||
	    (fw_type == PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_CNTL))
		AMDGV_INFO("%s firmware (version %d) loaded\n", name, version & 0xFF);

	else if (fw_type == PSP_DIR_ENTRY_TYPE_MMSCHEDULER_FW)
		AMDGV_INFO("%s firmware (version %d.%d.%d) loaded\n", name,
			   (version >> 24) & 0xFF,
			   (version >> 16) & 0xFF,
			   version & 0xFFFF);
	else if (fw_type == PSP_DIR_ENTRY_TYPE_SMU_OFF_CHIP_FW)
		AMDGV_INFO("%s firmware (version %d.%d.%d) loaded\n", name,
			   (version >> 8) & 0xFF,
			   (version >> 16) & 0xFF,
			   (version >> 24) & 0xFF);
	else if ((fw_type == PSP_DIR_ENTRY_TYPE_PSP_FW_TRUSTED_OS) ||
		 (fw_type == PSP_DIR_ENTRY_TYPE_PSP_FW_SYSDRV) ||
		 (fw_type == PSP_DIR_ENTRY_TYPE_SECURITY_GASKET) ||
		 (fw_type == PSP_DIR_ENTRY_TYPE_PSP_SEC_POLICY_STAGE2))
		AMDGV_INFO("%s firmware (version %X.%X.%X.%X) loaded\n", name,
			   (version >> 24) & 0xFF,
			   (version >> 16) & 0xFF,
			   (version >> 8) & 0xFF,
			   version & 0xFF);
	else if (fw_type == PSP_DIR_ENTRY_TYPE_PSP_FW_BOOT_LOADER)
		AMDGV_INFO("%s firmware (version %d.%d.%d.%d) loaded\n", name, version & 0xFF,
			   (version >> 8) & 0xFF,
			   (version >> 16) & 0xFF,
			   (version >> 24) & 0xFF);
	else
		AMDGV_INFO("%s firmware (version %d) loaded\n", name, version);

	/* update FW version to adapt */
	if (fw_id != AMDGV_FIRMWARE_ID__MAX)
		adapt->psp.fw_info[fw_id] = version;
}

int amdgv_atombios_get_fw_usage_fb(struct amdgv_adapter *adapt)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index = GetIndexIntoMasterTable(DATA, VRAM_UsageByFirmware);
	uint16_t data_offset;
	struct _ATOM_VRAM_USAGE_BY_FIRMWARE *fw_usage;
	struct _ATOM_FIRMWARE_VRAM_RESERVE_INFO *resv_info;

	if (amdgv_atom_parse_data_header(ctx, index, NULL, NULL, NULL, &data_offset)) {
		fw_usage = (struct _ATOM_VRAM_USAGE_BY_FIRMWARE *)((uint8_t *)ctx->bios +
								   data_offset);

		resv_info = &fw_usage->asFirmwareVramReserveInfo[0];

		AMDGV_DEBUG("atom firmware requested %08x %dkb\n",
			   resv_info->ulStartAddrUsedByFirmware, resv_info->usFirmwareUseInKb);

		adapt->vbios.vram_usage_start_addr =
			(uint64_t)resv_info->ulStartAddrUsedByFirmware;

		AMDGV_DEBUG("vram used by fw start addr=0x%llx\n",
			   adapt->vbios.vram_usage_start_addr);
	}

	return 0;
}

int amdgv_atombios_get_gfx_info(struct amdgv_adapter *adapt)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index = GetIndexIntoMasterTable(DATA, GFX_Info);
	uint8_t frev, crev;
	uint16_t data_offset;
	int ret = AMDGV_FAILURE;

	if (amdgv_atom_parse_data_header(ctx, index, NULL, &frev, &crev, &data_offset)) {
		ATOM_GFX_INFO_V2_1 *gfx_info =
			(ATOM_GFX_INFO_V2_1 *)((uint8_t *)ctx->bios + data_offset);

		adapt->config.gfx.max_shader_engines = gfx_info->max_shader_engines;
		adapt->config.gfx.max_cu_per_sh = gfx_info->max_cu_per_sh;
		adapt->config.gfx.max_sh_per_se = gfx_info->max_sh_per_se;

		ret = 0;
	}
	return ret;
}
