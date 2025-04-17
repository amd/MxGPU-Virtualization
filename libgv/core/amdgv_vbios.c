/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include "atombios/atom.h"
#include "atombios/atomfirmware.h"
#include "atombios/atombios.h"
#include "amdgv_vbios.h"
#include "amdgv_device.h"
#include "amdgv_vfmgr.h"
#include "amdgv_sriovmsg.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

#define BUILD_DATA_LENGTH 15

union firmware_info {
	struct atom_firmware_info_v3_1 v31;
	struct atom_firmware_info_v3_2 v32;
	struct atom_firmware_info_v3_3 v33;
	struct atom_firmware_info_v3_4 v34;
};

static uint32_t cail_pll_read(struct card_info *info, uint32_t reg)
{
	return 0;
}

static void cail_pll_write(struct card_info *info, uint32_t reg, uint32_t val)
{
}

static uint32_t cail_mc_read(struct card_info *info, uint32_t reg)
{
	return 0;
}

static void cail_mc_write(struct card_info *info, uint32_t reg, uint32_t val)
{
}

static void cail_reg_write(struct card_info *info, uint32_t reg, uint32_t val)
{
	struct amdgv_adapter *adapt = info->dev;

	WREG32(reg, val);
}

static uint32_t cail_reg_read(struct card_info *info, uint32_t reg)
{
	struct amdgv_adapter *adapt = info->dev;
	uint32_t r;

	r = RREG32(reg);
	return r;
}

static void cail_ioreg_write(struct card_info *info, uint32_t reg, uint32_t val)
{
	struct amdgv_adapter *adapt = info->dev;

	WREG32_IO(reg, val);
}

static uint32_t cail_ioreg_read(struct card_info *info, uint32_t reg)
{
	struct amdgv_adapter *adapt = info->dev;
	uint32_t r;

	r = RREG32_IO(reg);
	return r;
}

static void cail_fb_write(struct card_info *info, uint64_t addr, uint32_t val)
{
	struct amdgv_adapter *adapt = info->dev;

	addr += adapt->vbios.vram_usage_start_addr;

	WRITE_FB32(addr, val);
}

static uint32_t cail_fb_read(struct card_info *info, uint64_t addr)
{
	struct amdgv_adapter *adapt = info->dev;

	addr += adapt->vbios.vram_usage_start_addr;

	return READ_FB32(addr);
}

static int amdgv_vbios_get_info(struct amdgv_adapter *adapt, uint8_t *img)
{
	int i, j, k;
	uint8_t anchor[] = { 0x20, 0x20, 0x00, 0x20, 0x20, 0x20,
			     0x20, 0x20, 0x20, 0x20, 0x20, 0x00 };
	uint8_t part_info[128] = { '\0' };
	uint8_t build_date[16] = { '\0' };

	if (adapt->vbios.special_version_check)
		return adapt->vbios.special_version_check(adapt, img);

	/* search for the first 2k vbios space*/
	for (i = 0; i < 2048 - sizeof(anchor); i++) {
		for (j = 0; j < sizeof(anchor) && ((i + j) < 2048); j++) {
			if (anchor[j] != img[i + j])
				break;
		}

		if (j == sizeof(anchor)) {
			AMDGV_INFO("found anchor at %d\n", i);

			for (k = 0; k < 128; k++) {
				if (img[i + j + k] == '\\')
					break;
			}

			if (k < 128) {
				oss_memset(adapt->vbios.build_num, 0,
					   sizeof(adapt->vbios.build_num));
				oss_memcpy(adapt->vbios.build_num, img + i - BUILD_NUM_LENGTH,
					   BUILD_NUM_LENGTH);
				AMDGV_INFO("build num: %s\n", adapt->vbios.build_num);

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

static int amdgv_vbios_read_rom_from_reg(struct amdgv_adapter *adapt, uint8_t *bios,
					 uint32_t length_bytes)
{
	if (adapt->vbios.read_rom_from_reg) {
		return adapt->vbios.read_rom_from_reg(adapt, bios, length_bytes);
	}

	return AMDGV_FAILURE;
}

void amdgv_vbios_cache_update(struct amdgv_adapter *adapt)
{
	struct amdgv_vbios_info *vbiosinfo = &adapt->vbios_cache;
	unsigned short offset;
	struct _ATOM_ROM_HEADER atom_rom_header;
	struct _ATOM_MASTER_DATA_TABLE master_table;
	struct _ATOM_FIRMWARE_INFO atom_fw_info;

	amdgv_vbios_read_rom_image(adapt, (unsigned char *)&offset, sizeof(unsigned short),
				   OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER);

	amdgv_vbios_read_rom_image(adapt, (unsigned char *)&atom_rom_header,
				   sizeof(struct _ATOM_ROM_HEADER), offset);

	vbiosinfo->sub_ved_id = atom_rom_header.usSubsystemVendorID;
	vbiosinfo->sub_dev_id = atom_rom_header.usSubsystemID;

	if (atom_rom_header.usMasterDataTableOffset != 0) {
		amdgv_vbios_read_rom_image(adapt, (unsigned char *)&master_table,
					   sizeof(struct _ATOM_MASTER_DATA_TABLE),
					   atom_rom_header.usMasterDataTableOffset);

		if (master_table.ListOfDataTables.FirmwareInfo != 0) {
			amdgv_vbios_read_rom_image(adapt, (unsigned char *)&atom_fw_info,
						   sizeof(struct _ATOM_FIRMWARE_INFO),
						   master_table.ListOfDataTables.FirmwareInfo);

			vbiosinfo->version = atom_fw_info.ulFirmwareRevision;
		}
	}

	amdgv_vbios_get_vbios_pn(adapt, vbiosinfo->vbios_pn);
	amdgv_vbios_get_vbios_date(adapt, vbiosinfo->date);
	amdgv_vbios_get_vbios_name(adapt, vbiosinfo->name);
	amdgv_vbios_get_vbios_version(adapt, vbiosinfo->vbios_version_string);
}

static void amdgv_vbios_free_vbios_images(struct amdgv_adapter *adapt)
{
	struct amdgv_vbios *vbios = &adapt->vbios;
	if (vbios->guest_image) {
		oss_free_memory(vbios->guest_image);
		vbios->guest_image = NULL;
	}

	if (vbios->image) {
		oss_free_memory(vbios->image);
		vbios->image = NULL;
	}

	vbios->image_size = 0;
}

void amdgv_vbios_atom_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->vbios.atom_context) {
		oss_free(adapt->vbios.atom_context->iio);
		if (adapt->vbios.atom_context->mutex) {
			oss_mutex_fini(adapt->vbios.atom_context->mutex);
			adapt->vbios.atom_context->mutex = OSS_INVALID_HANDLE;
		}

		oss_free(adapt->vbios.atom_context);
		adapt->vbios.atom_context = NULL;
	}

	if (adapt->vbios.atom_card_info) {
		oss_free(adapt->vbios.atom_card_info);
		adapt->vbios.atom_card_info = NULL;
	}

	amdgv_vbios_free_vbios_images(adapt);
}

void amdgv_vbios_atom_hw_fini(struct amdgv_adapter *adapt)
{
	/*hw fini*/
}

static int amdgv_vbios_alloc_images(struct amdgv_adapter *adapt)
{
	struct amdgv_vbios *vbios = &adapt->vbios;
	// Allocate more than we need since we can't know guest vbios image
	// size at sw init time. EEPROM_BYTE_SIZE is ~2.5 time larger than
	// currently needed guest vbios image size and it will surely be
	// enough for future uses.
	const uint32_t guest_image_size = EEPROM_BYTE_SIZE;
	vbios->image = (uint8_t *)oss_alloc_memory(EEPROM_BYTE_SIZE);
	if (vbios->image == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				EEPROM_BYTE_SIZE);
		return AMDGV_FAILURE;
	}

	vbios->guest_image = (uint8_t *)oss_alloc_memory(guest_image_size);
	if (vbios->guest_image == NULL) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				EEPROM_BYTE_SIZE);
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_vbios_atom_sw_init(struct amdgv_adapter *adapt)
{
	struct card_info *atom_card_info;
	struct atom_context *ctx;

	atom_card_info = oss_zalloc(sizeof(struct card_info));
	if (!atom_card_info)
		return AMDGV_FAILURE;

	ctx = oss_zalloc(sizeof(struct atom_context));
	if (!ctx) {
		amdgv_vbios_atom_sw_fini(adapt);
		return AMDGV_FAILURE;
	}

	if (amdgv_vbios_alloc_images(adapt)) {
		amdgv_vbios_atom_sw_fini(adapt);
		return AMDGV_FAILURE;
	}

	adapt->vbios.atom_card_info = atom_card_info;
	atom_card_info->dev = adapt;
	atom_card_info->reg_read = cail_reg_read;
	atom_card_info->reg_write = cail_reg_write;

	if (adapt->io_mem) {
		atom_card_info->ioreg_read = cail_ioreg_read;
		atom_card_info->ioreg_write = cail_ioreg_write;
	} else {
		AMDGV_INFO("PCI I/O BAR is not found. Using MMIO "
			   "to access ATOM BIOS\n");
		atom_card_info->ioreg_read = cail_reg_read;
		atom_card_info->ioreg_write = cail_reg_write;
	}

	atom_card_info->mc_read = cail_mc_read;
	atom_card_info->mc_write = cail_mc_write;
	atom_card_info->pll_read = cail_pll_read;
	atom_card_info->pll_write = cail_pll_write;

	atom_card_info->fb_read = cail_fb_read;
	atom_card_info->fb_write = cail_fb_write;

	adapt->vbios.atom_context = ctx;
	ctx->card = atom_card_info;
	ctx->mutex = oss_mutex_init();
	if (ctx->mutex == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		amdgv_vbios_atom_sw_fini(adapt);
		return AMDGV_FAILURE;
	}

	ctx->iio = oss_zalloc(2 * 256);
	if (!ctx->iio) {
		amdgv_vbios_atom_sw_fini(adapt);
		return AMDGV_FAILURE;
	}
	return 0;
}

static uint32_t amdgv_atomfirmware_query_firmware_capability(struct amdgv_adapter *adapt)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
						firmwareinfo);
	uint16_t data_offset, size;
	union firmware_info *firmware_info;
	uint8_t frev, crev;
	uint32_t fw_cap = 0;

	if (amdgv_atom_parse_data_header(ctx, index, &size, &frev, &crev, &data_offset)) {
		/* support firmware_info 3.1 + */
		if ((frev == 3 && crev >= 1) || (frev > 3)) {
			firmware_info =
				(union firmware_info *)((char *)ctx->bios + data_offset);
			fw_cap = le32_to_cpu(firmware_info->v31.firmware_capability);
		}
	}

	return fw_cap;
}

int amdgv_vbios_atom_hw_init(struct amdgv_adapter *adapt)
{
	if (!amdgv_atom_parse(adapt->vbios.atom_context, adapt->vbios.image)) {
		return AMDGV_FAILURE;
	}

	if (adapt->vbios.is_atom_fw) {
		amdgv_atomfirmware_get_fw_usage_fb(adapt);
		adapt->firmware_flags = amdgv_atomfirmware_query_firmware_capability(adapt);
	} else {
		amdgv_atombios_get_fw_usage_fb(adapt);
	}

	return 0;
}

static int amdgv_vbios_image_checksum(struct amdgv_adapter *adapt)
{
	struct amdgv_vbios *vbios = &adapt->vbios;
	int i, sum = 0;
	for (i = 0; i < vbios->image_size; i++)
		sum += vbios->image[i];

	if (sum & 0xff) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_VBIOS_CHECKSUM_ERR, 0);
		return AMDGV_FAILURE;
	}

	return 0;
}

static void amdgv_vbios_print_vbios_header_info(struct amdgv_adapter *adapt)
{
	uint32_t idx;
	struct amdgv_vbios *vbios = &adapt->vbios;

	AMDGV_INFO("vbios starts: 0x%x, 0x%x\n", vbios->image[0], vbios->image[1]);

	idx = vbios->image[0x18] + ((uint16_t)vbios->image[0x19] << 8);
	AMDGV_INFO("vbios version major 0x%x minor 0x%x\n", (uint16_t)vbios->image[idx + 0x13],
		   (uint16_t)vbios->image[idx + 0x12]);
}

static int amdgv_vbios_fill_guest_image(struct amdgv_adapter *adapt)
{
	struct amdgv_vbios *vbios = &adapt->vbios;
	int ret = amdgv_vbios_get_info(adapt, vbios->image);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_VBIOS_READ_FAIL, 0);
		return AMDGV_FAILURE;
	}

	oss_memcpy(vbios->guest_image, vbios->image, vbios->image_size);
	return 0;
}

static int amdgv_vbios_read_img_and_verify(struct amdgv_adapter *adapt, uint32_t read_size,
					   bool do_checksum)
{
	struct amdgv_vbios *vbios = &adapt->vbios;
	int ret = amdgv_vbios_read_rom_from_reg(adapt, vbios->image, read_size);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_VBIOS_READ_FAIL, 0);
		return AMDGV_FAILURE;
	}

	if (!AMDGV_VBIOS_IS_VALID(vbios->image)) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_VBIOS_INVALID, 0);
		return AMDGV_FAILURE;
	}

	vbios->image_size = AMDGV_VBIOS_LENGTH(vbios->image);

	if (do_checksum)
		return amdgv_vbios_image_checksum(adapt);
	return 0;
}

static int amdgv_vbios_read_vbios_images(struct amdgv_adapter *adapt, bool preload_header)
{
	if (preload_header) {
		// Read only vbios header so we can extract vbios image size.
		if (amdgv_vbios_read_img_and_verify(adapt, AMDGV_VBIOS_SIGNATURE_END,
						    false /*do_checksum*/))
			return AMDGV_FAILURE;
	} else {
		// Otherwise use maximum possible image size. Image size will be adjusted once
		// vbios image header is read.
		adapt->vbios.image_size = EEPROM_BYTE_SIZE;
	}

	if (amdgv_vbios_read_img_and_verify(adapt, adapt->vbios.image_size,
					    true /*do_checksum*/))
		return AMDGV_FAILURE;

	amdgv_vbios_print_vbios_header_info(adapt);

	return amdgv_vbios_fill_guest_image(adapt);
}

int amdgv_vbios_read_img_func(struct amdgv_adapter *adapt)
{
	int ret = amdgv_vbios_read_vbios_images(adapt, true /*preload_header*/);
	if (ret)
		adapt->vbios.image_size = 0;
	return ret;
}

static int amdgv_vbios_read_img_thread(void *context)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;

	/* read rom */
	if (0 != amdgv_vbios_read_img_func(adapt)) {
		goto finish;
	}
	adapt->vbios.read_vbios_success = true;

finish:
	adapt->vbios.finish_read = true;
	/* wait for main thread to call close thread. The close_thread must be called
	   when thread is still running according to linux kernel, this doesn't have a
	   timeout since main thread must call close_thread to end the vbios read
	   sequence, so we just dead loop to wait */
	while (!oss_thread_should_stop(adapt->vbios.read_vbios_thread)) {
		oss_msleep(100);
	}

	return 0;
}

static int amdgv_vbios_wait_read_cb(void *context)
{
	bool *finish_read = (bool *)context;
	return !(*finish_read == true);
}

int amdgv_vbios_read_img(struct amdgv_adapter *adapt)
{
	adapt->vbios.timeout = false;
	adapt->vbios.finish_read = false;
	adapt->vbios.read_vbios_success = false;

	adapt->vbios.read_vbios_thread = oss_create_thread(amdgv_vbios_read_img_thread,
							   (void *)adapt, "read_vbios_thread");
	if (adapt->vbios.read_vbios_thread == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_THREAD_FAIL, 0);
		return AMDGV_FAILURE;
	}

	/* just wait, will check another value after wait */
	amdgv_wait_for(adapt, amdgv_vbios_wait_read_cb, (void *)&adapt->vbios.finish_read,
		       AMDGV_TIMEOUT(TIMEOUT_READ_VBIOS), 0);

	if (!adapt->vbios.read_vbios_success) {
		if (!adapt->vbios.finish_read) {
			adapt->vbios.timeout = true;
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_VBIOS_TIMEOUT, 0);
			oss_close_thread(adapt->vbios.read_vbios_thread);
			adapt->vbios.read_vbios_thread = OSS_INVALID_HANDLE;
			return AMDGV_FAILURE;
		} else {
			oss_close_thread(adapt->vbios.read_vbios_thread);
			adapt->vbios.read_vbios_thread = OSS_INVALID_HANDLE;
			return AMDGV_FAILURE;
		}
	}

	oss_close_thread(adapt->vbios.read_vbios_thread);
	adapt->vbios.read_vbios_thread = OSS_INVALID_HANDLE;
	return 0;
}

int amdgv_vbios_upload_image_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	/* check if guest_image is valid, live update gim does not migrate guest_image,
	 * need to generate guest_image when first time it's used
	 */
	if (!AMDGV_VBIOS_IS_VALID(adapt->vbios.guest_image)) {
		oss_memcpy(adapt->vbios.guest_image, adapt->vbios.image,
			   adapt->vbios.image_size);
		amdgv_atomfirmware_set_fw_usage_fb_guest(adapt);
	}
	return amdgv_vfmgr_copy_to_vf_fb(adapt, idx_vf, AMDGV_RESERVE_FB_OFFSET_KB,
					 adapt->vbios.guest_image, adapt->vbios.image_size);
}

unsigned char *amdgv_vbios_find_str_in_rom(struct amdgv_adapter *adapt, char *str, int start,
					   int end, int maxlen)
{
	unsigned long str_off;
	unsigned char *p_rom;
	unsigned short str_len;

	str_off = 0;
	str_len = oss_strnlen(str, maxlen);
	p_rom = adapt->vbios.image;

	if (end >= adapt->vbios.image_size)
		end = adapt->vbios.image_size - 1;

	for (; start <= end; ++start) {
		for (str_off = 0; str_off < str_len; ++str_off) {
			if (str[str_off] != *(p_rom + start + str_off))
				break;
		}

		if (str_off == str_len || str[str_off] == 0)
			return p_rom + start;
	}
	return NULL;
}

int amdgv_vbios_get_vbios_pn(struct amdgv_adapter *adapt, unsigned char *pn_str)
{
	unsigned char *p_rom;
	unsigned short off_to_vbios_str;
	unsigned char *vbios_str;
	int count;

	off_to_vbios_str = 0;
	p_rom = adapt->vbios.image;

	if (*(p_rom + OFFSET_TO_GET_ATOMBIOS_NUMBER_OF_STRINGS) != 0) {
		off_to_vbios_str =
			*(unsigned short *)(p_rom + OFFSET_TO_GET_ATOMBIOS_STRING_START);

		vbios_str = (unsigned char *)(p_rom + off_to_vbios_str);
	} else {
		vbios_str = p_rom + OFFSET_TO_VBIOS_PART_NUMBER;
	}

	if (*vbios_str == 0) {
		vbios_str = amdgv_vbios_find_str_in_rom(adapt, BIOS_ATOM_PREFIX, 3, 1024, 64);
		if (vbios_str == NULL)
			vbios_str += sizeof(BIOS_ATOM_PREFIX) - 1;
	}
	if (vbios_str != NULL && *vbios_str == 0)
		vbios_str++;

	if (vbios_str != NULL) {
		count = 0;
		while ((count < BIOS_STRING_LENGTH) && vbios_str[count] >= ' ' &&
		       vbios_str[count] <= 'z') {
			pn_str[count] = vbios_str[count];
			count++;
		}

		pn_str[count] = 0;
	}

	return 0;
}

int amdgv_vbios_get_vbios_date(struct amdgv_adapter *adapt, unsigned char *date_str)
{
	unsigned char *p_rom;
	unsigned char *date_in_rom;

	p_rom = adapt->vbios.image;

	date_in_rom = p_rom + OFFSET_TO_VBIOS_DATE;

	date_str[0] = '2';
	date_str[1] = '0';
	date_str[2] = date_in_rom[6];
	date_str[3] = date_in_rom[7];
	date_str[4] = '/';
	date_str[5] = date_in_rom[0];
	date_str[6] = date_in_rom[1];
	date_str[7] = '/';
	date_str[8] = date_in_rom[3];
	date_str[9] = date_in_rom[4];
	date_str[10] = ' ';
	date_str[11] = date_in_rom[9];
	date_str[12] = date_in_rom[10];
	date_str[13] = date_in_rom[11];
	date_str[14] = date_in_rom[12];
	date_str[15] = date_in_rom[13];
	date_str[16] = '\0';

	return 0;
}

int amdgv_vbios_get_vbios_name(struct amdgv_adapter *adapt, unsigned char *name_str)
{
	unsigned char *p_rom;
	unsigned char str_num;
	unsigned short off_to_vbios_str;
	unsigned char *c_ptr;
	int name_size;
	int i;

	const char *na = "--N/A--";
	char *back;

	p_rom = adapt->vbios.image;

	str_num = *(p_rom + OFFSET_TO_GET_ATOMBIOS_NUMBER_OF_STRINGS);
	if (str_num != 0) {
		off_to_vbios_str =
			*(unsigned short *)(p_rom + OFFSET_TO_GET_ATOMBIOS_STRING_START);

		c_ptr = (unsigned char *)(p_rom + off_to_vbios_str);
	} else {
		/* do not know where to find name */
		oss_memcpy(name_str, na, 7);
		name_str[7] = 0;
		return 0;
	}

	/*
	 * skip the atombios strings, usually 4
	 * 1st is P/N, 2nd is ASIC, 3rd is PCI type, 4th is Memory type
	 */
	for (i = 0; i < str_num; i++) {
		while (*c_ptr != 0)
			c_ptr++;
		c_ptr++;
	}

	/* skip the following 2 chars: 0x0D 0x0A */
	c_ptr += 2;

	name_size = oss_strnlen(c_ptr, STRLEN_LONG - 1);
	oss_memcpy(name_str, c_ptr, name_size);
	back = name_str + name_size;
	while ((*--back) == ' ')
		;
	*(back + 1) = '\0';

	return 0;
}

int amdgv_vbios_get_vbios_version(struct amdgv_adapter *adapt, unsigned char *ver_str)
{
	unsigned char *vbios_ver;

	// find anchor ATOMBIOSBK-AMD
	vbios_ver = amdgv_vbios_find_str_in_rom(adapt, BIOS_VERSION_PREFIX, 3, 4096, 64);
	if (vbios_ver != NULL) {
		// skip ATOMBIOSBK-AMD VER
		vbios_ver += 18;
		oss_memcpy(ver_str, vbios_ver, STRLEN_NORMAL);
	} else {
		ver_str[0] = '\0';
	}

	return 0;
}

unsigned int amdgv_vbios_read_rom_image(struct amdgv_adapter *adapt, unsigned char *dest,
					unsigned int size, unsigned int offset)
{
	int i;
	unsigned char *p_rom;

	p_rom = (unsigned char *)(adapt->vbios.image);
	p_rom += offset;

	for (i = 0; i < size; i++)
		dest[i] = p_rom[i];

	return size;
}

uint32_t amdgv_vbios_get_fw_reserved_size(struct amdgv_adapter *adapt)
{
	struct atom_context *ctx = adapt->vbios.atom_context;
	int index = get_index_into_master_table(atom_master_list_of_data_tables_v2_1,
					firmwareinfo);
	uint16_t data_offset, size;
	union firmware_info *firmware_info;
	uint8_t frev, crev;
	uint32_t memsize = 0;

	if (amdgv_atom_parse_data_header(ctx, index,
				&size, &frev, &crev, &data_offset)) {
		/* format rev.3, content rev.4 reports memory size */
		if (frev == 3 && crev >= 4) {
			firmware_info = (union firmware_info *)
				((char *)ctx->bios + data_offset);
			memsize = le32_to_cpu(firmware_info->v34.fw_reserved_size_in_kb) / 1024;
		}
	}

	return memsize;
}

static int amdgv_get_pci_atomic_request_support(struct amdgv_adapter *adapt)
{
	uint16_t val;
	int pos;

	pos = oss_pci_find_capability(adapt->dev, PCI_CAP_ID__PCIE);

	if (!pos) {
		AMDGV_ERROR("this device does not support capability: %x\n", PCI_CAP_ID__PCIE);
		return AMDGV_FAILURE;
	}

	oss_pci_read_config_word(adapt->dev, pos + PCIE_DEVICE_CAP2, &val);
	adapt->pcie_atomic_ops_support_flags =
		val & (PCIE_DEVICE_CAP2__ATOMIC_COMP32 | PCIE_DEVICE_CAP2__ATOMIC_COMP64);

	return 0;
}

int amdgv_vbios_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_vbios *vbios_info)
{
	if (VBIOS_IMAGE_BYTE_SIZE < adapt->vbios.image_size) {
		return AMDGV_LIVE_INFO_STATUS_SIZE_UNMATCH;
	}

	oss_memcpy(vbios_info->image, adapt->vbios.image, adapt->vbios.image_size);
	vbios_info->image_size = adapt->vbios.image_size;
	vbios_info->sec_version = adapt->vbios.sec_version;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

int amdgv_vbios_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_vbios *vbios_info)
{
	oss_memcpy(adapt->vbios.image, vbios_info->image, vbios_info->image_size);
	adapt->vbios.image_size = vbios_info->image_size;
	adapt->vbios.sec_version = vbios_info->sec_version;

	// Init atom parser
	if (amdgv_vbios_atom_hw_init(adapt))
		return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;

	amdgv_get_pci_atomic_request_support(adapt);

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}
