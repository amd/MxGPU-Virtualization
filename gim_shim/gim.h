/*
 * Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef GIM_H
#define GIM_H

#include <amdgv_oss.h>
#include <amdgv_api.h>

#include "smi_drv.h"
#include "smi_drv_oss.h"
#include <amdgv_live_info.h>

struct vf_info {
	uint32_t bdf;
	struct pci_dev *pdev;
};

struct gim_dev_data {
	struct list_head list;

	uint32_t gpu_index;
	uint32_t vf_num;
	int16_t parent;
	bool psp_vbflash_support;

	struct pci_dev *pdev;
	struct amdgv_init_data init_data;
	amdgv_dev_t adev;
	struct vf_info vf_map[AMDGV_MAX_VF_NUM];
	struct mutex mem_lock;
	struct mutex dev_lock;

	union amdgv_vf_info vf_info;

	void	*live_data_ptr;
	uint64_t live_data_size;
};

struct common_firmware_header {
	uint32_t size_bytes; /* size of the entire header+image(s) in bytes */
	uint32_t header_size_bytes; /* size of just the header in bytes */
	uint16_t header_version_major; /* header version */
	uint16_t header_version_minor; /* header version */
	uint16_t ip_version_major; /* IP version */
	uint16_t ip_version_minor; /* IP version */
	uint32_t ucode_version;
	uint32_t ucode_size_bytes; /* size of ucode in bytes */
	uint32_t ucode_array_offset_bytes; /* payload offset from the start of the header */
	uint32_t crc32;  /* crc32 checksum of the payload */
};

extern struct list_head gim_device_list;
extern uint32_t shim_log_level;
extern struct mutex gim_device_list_lock;

extern struct oss_interface gim_oss_interfaces;
extern struct smi_shim_interface gim_smi_interface;

int gim_get_vf_idx(struct pci_dev *vf_pdev, struct gim_dev_data *data);
int gim_dbdf_to_vf_idx(uint32_t bdf, struct gim_dev_data *data);

int gim_strncmp(const char *s1, const char *s2, uint64_t n);
uint32_t gim_strlen(const char *s);
uint32_t gim_strnlen(const char *s, uint32_t len);
uint32_t gim_do_div(uint64_t *n, uint32_t base);
int gim_vsnprintf(char *buf, uint32_t size, const char *fmt, va_list args);
int gim_calc_hash_ext(const char *alg_name, void *out, const void *msg, uint64_t len);
struct gim_event {
	struct completion cp;
	uint64_t flags;
};

#if defined(HAVE_LOFF_T_VARIABLE)
#define gim_kernel_read(file, buf, size, pos) kernel_read(file, buf, size, &pos)
#else
#define gim_kernel_read(file, buf, size, pos) kernel_read(file, pos, buf, size)
#endif

#if defined(HAVE_LOFF_T_VARIABLE)
#define gim_kernel_write(file, buf, size, pos) kernel_write(file, buf, size, &pos)
#else
#define gim_kernel_write(file, buf, size, pos) kernel_write(file, buf, size, pos)
#endif

#endif
