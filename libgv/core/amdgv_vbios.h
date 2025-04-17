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

#ifndef AMDGV_VBIOS_H
#define AMDGV_VBIOS_H

#define AMDGV_VBIOS_SIGNATURE	     "761295520"
#define AMDGV_VBIOS_SIGNATURE_OFFSET 0x30
#define AMDGV_VBIOS_SIGNATURE_SIZE   sizeof(AMDGV_VBIOS_SIGNATURE)
#define AMDGV_VBIOS_SIGNATURE_END    (AMDGV_VBIOS_SIGNATURE_OFFSET + AMDGV_VBIOS_SIGNATURE_SIZE)

#define AMDGV_VBIOS_IS_VALID(p) ((p)[0] == 0x55 && (p)[1] == 0xAA)
#define AMDGV_VBIOS_LENGTH(p)	((p)[2] << 9)

#define VBIOS_POST_ASIC_INIT  0
#define VBIOS_POST_LOAD_UCODE 1
#define VBIOS_POST_FULL_POST  2
#define VBIOS_POST_REPOST     3

#define AMDGV_UCODE_TYPE_MC	   (1 << 0)
#define AMDGV_UCODE_TYPE_SMU	   (1 << 1)
#define AMDGV_UCODE_TYPE_RLCV	   (1 << 2)
#define AMDGV_UCODE_TYPE_TOC	   (1 << 3)
#define AMDGV_UCODE_TYPE_SMU_PATCH (1 << 4)

/* debug option for bring up */
#define AMDGV_FW_LOAD_PSP 1


/* FW version offset in vbios */
#define VBIOS_FW_VERSION_OFFSET 0x60

#define BUILD_NUM_LENGTH  6

struct amdgv_live_info_vbios;

struct amdgv_vbios {
	uint8_t *image;
	/* The BUILD_NUM_LENGTH is 6. It is better to align to DWORD. */
	char build_num[(BUILD_NUM_LENGTH + 3) & 0xFFFFFFFC];

	uint32_t sec_version;

	uint8_t *guest_image;

	unsigned int image_size;

	uint64_t vram_usage_start_addr;

	bool is_atom_fw;

	bool finish_read;

	bool timeout;

	bool read_vbios_success;

	thread_t read_vbios_thread;

	struct atom_context *atom_context;

	struct card_info *atom_card_info;

	int (*read_rom_from_reg)(struct amdgv_adapter *adapt, uint8_t *bios,
				 uint32_t length_bytes);
	void (*golden_init)(struct amdgv_adapter *adapt);
	uint32_t (*get_fw_version)(struct amdgv_adapter *adapt, uint8_t type);
	void (*vmhub_hook)(struct amdgv_adapter *adapt);
	int (*get_mm_capability)(struct amdgv_adapter *adapt);
	void (*asic_golden_init)(struct amdgv_adapter *adapt);
	void (*gfx_golden_init)(struct amdgv_adapter *adapt);
	bool (*smu_fw_loaded)(struct amdgv_adapter *adapt);
	int (*special_version_check)(struct amdgv_adapter *adapt, uint8_t *img);
};

int amdgv_atombios_get_fw_usage_fb(struct amdgv_adapter *adapt);

int amdgv_atomfirmware_get_fw_usage_fb(struct amdgv_adapter *adapt);
int amdgv_atomfirmware_post(struct amdgv_adapter *adapt, int post_type);
int amdgv_atomfirmware_set_fw_usage_fb_guest(struct amdgv_adapter *adapt);
int amdgv_atomfirmware_smc_init(struct amdgv_adapter *adapt);
bool amdgv_atomfirmware_mem_ecc_support(struct amdgv_adapter *adapt);
bool amdgv_atomfirmware_sram_ecc_support(struct amdgv_adapter *adapt);
int amdgv_atomfirmware_ras_rom_addr(struct amdgv_adapter *adapt, uint8_t *i2c_address);
int amdgv_atomfirmware_get_vram_info(struct amdgv_adapter *adapt);
int amdgv_vbios_read_img(struct amdgv_adapter *adapt);
void amdgv_vbios_atom_sw_fini(struct amdgv_adapter *adapt);
void amdgv_vbios_atom_hw_fini(struct amdgv_adapter *adapt);
int amdgv_vbios_atom_sw_init(struct amdgv_adapter *adapt);
int amdgv_vbios_atom_hw_init(struct amdgv_adapter *adapt);

int amdgv_vbios_read_img_func(struct amdgv_adapter *adapt);
int amdgv_vbios_upload_image_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
unsigned char *amdgv_vbios_find_str_in_rom(struct amdgv_adapter *adapt, char *str, int start,
					   int end, int maxlen);
int amdgv_vbios_get_vbios_pn(struct amdgv_adapter *adapt, unsigned char *pn_str);
int amdgv_vbios_get_vbios_date(struct amdgv_adapter *adapt, unsigned char *date_str);
int amdgv_vbios_get_vbios_name(struct amdgv_adapter *adapt, unsigned char *name_str);
int amdgv_vbios_get_vbios_version(struct amdgv_adapter *adapt, unsigned char *ver_str);
unsigned int amdgv_vbios_read_rom_image(struct amdgv_adapter *adapt, unsigned char *dest,
					unsigned int size, unsigned int offset);
uint32_t amdgv_atombios_get_fw_offset(struct amdgv_adapter *adapt, uint8_t fw_type);
void amdgv_atombios_print_fw_version(struct amdgv_adapter *adapt, uint8_t fw_type,
				     uint32_t version);
int amdgv_atombios_get_gfx_info(struct amdgv_adapter *adapt);
void amdgv_vbios_cache_update(struct amdgv_adapter *adapt);
uint32_t amdgv_vbios_get_fw_reserved_size(struct amdgv_adapter *adapt);
int amdgv_vbios_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_vbios *vbios_info);
int amdgv_vbios_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_vbios *vbios_info);
#endif
