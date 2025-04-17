/*
 * Copyright (c) 2016-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include "atomfirmware.h"
#include "atom.h"
#include "atombios.h"
#include "amdgv_device.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static int amdgv_atomfirmware_update_checksum(struct amdgv_adapter *adapt, uint8_t *image,
					      unsigned int image_size)
{
	int i, sum = 0;

	VBIOS_ROM_HEADER *rom_header = (VBIOS_ROM_HEADER *)image;

	oss_memset(rom_header->CheckSum, 0, sizeof(rom_header->CheckSum));

	for (i = 0; i < image_size; i++)
		sum += image[i];

	rom_header->CheckSum[0] = 0x100 - (uint8_t)sum;

	AMDGV_INFO("update guest vbios checksum to 0x%02x \n", rom_header->CheckSum[0]);

	return 0;
}

int amdgv_atomfirmware_get_fw_usage_fb(struct amdgv_adapter *adapt)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
						vram_usagebyfirmware);
	uint16_t data_offset;

	if (amdgv_atom_parse_data_header(ctx, index, NULL, NULL, NULL, &data_offset)) {
		struct vram_usagebyfirmware_v2_1 *firmware_usage =
			(struct vram_usagebyfirmware_v2_1 *)((uint8_t *)ctx->bios +
							     data_offset);

		AMDGV_DEBUG("atom firmware requested %08x %dkb fw %dkb drv\n",
			   firmware_usage->start_address_in_kb,
			   firmware_usage->used_by_firmware_in_kb,
			   firmware_usage->used_by_driver_in_kb);

		adapt->vbios.vram_usage_start_addr =
			(uint64_t)firmware_usage->start_address_in_kb * 1024;

		AMDGV_DEBUG("vram used by fw start addr=0x%llx\n",
			   adapt->vbios.vram_usage_start_addr);
	}

	return 0;
}

int amdgv_atomfirmware_set_fw_usage_fb_guest(struct amdgv_adapter *adapt)
{
	uint16_t data_offset = 0;
	uint16_t size = 0;
	uint8_t frev = 0;
	uint8_t crev = 0;
	struct vram_usagebyfirmware_v2_1 *firmware_usage_v2_1 = NULL;
	struct vram_usagebyfirmware_v2_2 *firmware_usage_v2_2 = NULL;
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
						vram_usagebyfirmware);

	if (amdgv_atom_parse_data_header(ctx, index, &size, &frev, &crev, &data_offset)) {
		if (frev == 2 && crev == 1) {
			if (adapt->ffbm.share_tmr) {
				AMDGV_ERROR("FFBM share tmr enabled, uncompatible format_revision (%d) and content_revision (%d) combination", frev, crev);
				return AMDGV_FAILURE;
			}

			firmware_usage_v2_1 = (struct vram_usagebyfirmware_v2_1 *)(adapt->vbios.guest_image + data_offset);

			AMDGV_DEBUG("original atom firmware usage %08x %dkb fw %dkb drv\n",
				firmware_usage_v2_1->start_address_in_kb,
				firmware_usage_v2_1->used_by_firmware_in_kb,
				firmware_usage_v2_1->used_by_driver_in_kb);

			firmware_usage_v2_1->start_address_in_kb =
				AMDGV_RESERVE_FB_OFFSET_KB |
				(ATOM_VRAM_BLOCK_SRIOV_MSG_SHARE_RESERVATION
				<< ATOM_VRAM_OPERATION_FLAGS_SHIFT);

			if (adapt->fw_load_type == AMDGV_FW_LOAD_BY_GIM_PHASE_2) {
				firmware_usage_v2_1->used_by_firmware_in_kb =
					AMDGV_RESERVE_FB_SIZE_KB_UCODE;
			} else {
				firmware_usage_v2_1->used_by_firmware_in_kb = AMDGV_RESERVE_FB_SIZE_KB;
			}

			AMDGV_DEBUG("set atom firmware usage %08x %dkb fw %dkb drv\n",
				firmware_usage_v2_1->start_address_in_kb,
				firmware_usage_v2_1->used_by_firmware_in_kb,
				firmware_usage_v2_1->used_by_driver_in_kb);
		} else if (frev >= 2 && crev >= 2) {
			firmware_usage_v2_2 = (struct vram_usagebyfirmware_v2_2 *)(adapt->vbios.guest_image + data_offset);

			AMDGV_DEBUG("original atom firmware usage v2_2 start at %08x %dkb fw and start at %08x %dkb drv region0\n",
				firmware_usage_v2_2->fw_region_start_address_in_kb,
				firmware_usage_v2_2->used_by_firmware_in_kb,
				firmware_usage_v2_2->driver_region0_start_address_in_kb,
				firmware_usage_v2_2->used_by_driver_region0_in_kb);

			firmware_usage_v2_2->fw_region_start_address_in_kb = ATOM_VRAM_BLOCK_NEEDS_NO_RESERVATION << 30;

			firmware_usage_v2_2->used_by_firmware_in_kb = 0;
			firmware_usage_v2_2->driver_region0_start_address_in_kb = 0;

			if (adapt->ffbm.share_tmr) {
				AMDGV_DEBUG("FFBM TMR share enabled: atom tmr offset kb: %d, allocated tmr size kb: %d", AMD_SRIOV_MSG_TMR_OFFSET_KB, adapt->psp.allocated_tmr_size / 1024);
				firmware_usage_v2_2->used_by_driver_region0_in_kb = (uint32_t)(AMD_SRIOV_MSG_TMR_OFFSET_KB + adapt->psp.allocated_tmr_size / 1024);
			} else {
				firmware_usage_v2_2->used_by_driver_region0_in_kb = AMDGV_RESERVE_FB_SIZE_KB;
			}

			AMDGV_DEBUG("set atom firmware usage v2_2 start at %08x %dkb fw and start at %08x %dkb drv region0\n",
				firmware_usage_v2_2->fw_region_start_address_in_kb,
				firmware_usage_v2_2->used_by_firmware_in_kb,
				firmware_usage_v2_2->driver_region0_start_address_in_kb,
				firmware_usage_v2_2->used_by_driver_region0_in_kb);
		} else {
			AMDGV_ERROR("Uncompatible format_revision (%d) and content_revision (%d) combination", frev, crev);
			return AMDGV_FAILURE;
		}

		/*Update guest vbios checksum to pass windows driver checksum check*/
		amdgv_atomfirmware_update_checksum(adapt, adapt->vbios.guest_image,
							adapt->vbios.image_size);
		return 0;
	}

	return AMDGV_FAILURE;
}

int amdgv_atomfirmware_post(struct amdgv_adapter *adapt, int post_type)
{
	int index;
	uint16_t data_offset;
	uint8_t frev, crev;
	struct atom_context *ctx = adapt->vbios.atom_context;
	struct atom_firmware_info_v3_1 *fw_info;
	struct asic_init_ps_allocation_v2_1 asic_init_ps_v2_1;

	/* get firmwareinfo table */
	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					    firmwareinfo);
	if (!amdgv_atom_parse_data_header(ctx, index, NULL, NULL, NULL, &data_offset))
		return AMDGV_FAILURE;
	else
		fw_info =
			(struct atom_firmware_info_v3_1 *)((uint8_t *)ctx->bios + data_offset);

	/* execute asic_init table */
	index = get_index_into_master_table(atom_master_list_of_command_functions_v2_1,
					    asic_init);
	if (!amdgv_atom_parse_cmd_header(ctx, index, &frev, &crev, NULL))
		return AMDGV_FAILURE;

	if (frev == 2 && crev >= 1) {
		oss_memset((char *)&asic_init_ps_v2_1, 0,
			   sizeof(struct asic_init_ps_allocation_v2_1));

		asic_init_ps_v2_1.param.engineparam.sclkfreqin10khz =
			fw_info->bootup_sclk_in10khz;
		asic_init_ps_v2_1.param.memparam.mclkfreqin10khz =
			fw_info->bootup_mclk_in10khz;

		/* 0     - normal boot
		 * 0x20  - boot after self-refresh
		 */
		asic_init_ps_v2_1.param.memparam.memflag = 0;

		if (post_type == VBIOS_POST_FULL_POST) {
			AMDGV_INFO("Do a full post\n");
			asic_init_ps_v2_1.param.engineparam.engineflag = b3SRIOV_LOAD_UCODE;
		} else if (post_type == VBIOS_POST_ASIC_INIT) {
			AMDGV_INFO("Do a asic init\n");
			asic_init_ps_v2_1.param.engineparam.engineflag = b3NORMAL_ENGINE_INIT;
		} else if (post_type == VBIOS_POST_LOAD_UCODE) {
			AMDGV_INFO("just load uCode\n");
			asic_init_ps_v2_1.param.engineparam.engineflag =
				b3SRIOV_LOAD_UCODE | b3SRIOV_SKIP_ASIC_INIT;
		} else if (post_type == VBIOS_POST_REPOST) {
			AMDGV_INFO("Do a re-post\n");
			asic_init_ps_v2_1.param.engineparam.engineflag = b3SRIOV_LOAD_UCODE;
		}

		return amdgv_atom_execute_table(ctx, index, (uint32_t *)&asic_init_ps_v2_1);
	}

	return AMDGV_FAILURE;
}

union umc_info {
	struct atom_umc_info_v3_1 v31;
	struct atom_umc_info_v3_2 v32;
	struct atom_umc_info_v3_3 v33;
	struct atom_umc_info_v4_0 v40;
};

bool amdgv_atomfirmware_mem_ecc_support(struct amdgv_adapter *adapt)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index;
	union umc_info *umc_info;
	uint8_t frev, crev;
	uint16_t size = 0;
	uint16_t data_offset = 0;
	uint8_t umc_config;
	uint32_t umc_config1;
	bool ecc_default_enabled = false;

	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
			umc_info);

	if (amdgv_atom_parse_data_header(ctx, index, &size, &frev, &crev, &data_offset)) {
		if (frev == 3) {
			umc_info = (union umc_info *)
				((uint8_t *)ctx->bios + data_offset);
			switch (crev) {
			case 1:
				umc_config = le32_to_cpu(umc_info->v31.umc_config);
				ecc_default_enabled =
					(umc_config & UMC_CONFIG__DEFAULT_MEM_ECC_ENABLE) ? true : false;
				break;
			case 2:
				umc_config = le32_to_cpu(umc_info->v32.umc_config);
				ecc_default_enabled =
					(umc_config & UMC_CONFIG__DEFAULT_MEM_ECC_ENABLE) ? true : false;
				break;
			case 3:
				umc_config = le32_to_cpu(umc_info->v33.umc_config);
				umc_config1 = le32_to_cpu(umc_info->v33.umc_config1);
				ecc_default_enabled = ((umc_config & UMC_CONFIG__DEFAULT_MEM_ECC_ENABLE) ||
					(umc_config1 & UMC_CONFIG1__ENABLE_ECC_CAPABLE)) ? true : false;
				break;
			default:
				/* unsupported crev */
				return false;
			}
		} else if (frev == 4) {
			umc_info = (union umc_info *)((uint8_t *)ctx->bios + data_offset);
			switch (crev) {
			case 0:
				umc_config1 = le32_to_cpu(umc_info->v40.umc_config1);
				ecc_default_enabled =
					(umc_config1 & UMC_CONFIG1__ENABLE_ECC_CAPABLE) ? true : false;
				break;
			default:
				/* unsupported crev */
				return false;
			}
		} else {
			/* unsupported frev */
			return false;
		}
	}

	return ecc_default_enabled;
}

bool amdgv_atomfirmware_sram_ecc_support(struct amdgv_adapter *adapt)
{
	uint32_t fw_cap;

	fw_cap = adapt->firmware_flags;

	return (fw_cap & ATOM_FIRMWARE_CAP_SRAM_ECC) ? true : false;
}

int amdgv_atomfirmware_ras_rom_addr(struct amdgv_adapter *adapt,
				      uint8_t *i2c_address)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index;
	uint16_t data_offset;
	uint8_t frev, crev;

	/* get firmwareinfo table */
	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					    firmwareinfo);

	if (amdgv_atom_parse_data_header(ctx, index, NULL, &frev, &crev, &data_offset)) {
		/* support RAS I2C 8 bit address in firmware_info 3.4 */
		if (frev == 3 && crev == 4) {
			struct atom_firmware_info_v3_4 *fw_info;
			fw_info = (struct atom_firmware_info_v3_4 *)
				((uint8_t *)ctx->bios + data_offset);

			if (i2c_address && fw_info->ras_rom_i2c_slave_addr)
				*i2c_address = fw_info->ras_rom_i2c_slave_addr;
			return 0;
		} else {
			AMDGV_WARN("Firmware Info Table does not support RAS I2C address info");
		}
	}

	return AMDGV_FAILURE;
}

int amdgv_atomfirmware_get_vram_info(struct amdgv_adapter *adapt)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index;
	uint16_t data_offset;
	uint8_t frev, crev;
	union umc_info *umc_info;

	/* get firmwareinfo table */
	index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1, umc_info);

	if (amdgv_atom_parse_data_header(ctx, index, NULL, &frev, &crev, &data_offset)) {
		umc_info = (union umc_info *)((uint8_t *)ctx->bios + data_offset);

		/* No translation required for this version of atomfirmware */
		if (frev == 3 && crev == 3) {
			adapt->vram_info.vram_type = umc_info->v33.vram_type;
			adapt->vram_info.vram_bit_width = 0xffffffff; // channel number and channel width are not available in this version
			return 0;
		} else if (frev == 4 && crev == 0) {
			adapt->vram_info.vram_type = umc_info->v40.vram_type;
			adapt->vram_info.vram_bit_width = umc_info->v40.channel_num * (1 << umc_info->v40.channel_width) * adapt->mcp.num_aid;
			return 0;
		} else {
			AMDGV_WARN("LibGV does not support umc info table %d.%d\n", frev, crev);
		}
	}

	return AMDGV_FAILURE;
}
