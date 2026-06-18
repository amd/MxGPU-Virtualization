/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GIM_H
#define GIM_H

#include <amdgv_oss.h>
#include <amdgv_api.h>

#include "smi_drv.h"
#include "smi_drv_oss.h"
#include "gim_memory_sentinel.h"
#include <amdgv_live_info.h>
#include "gim_mig.h"

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

	struct gim_mig_device *pf_mig_dev;
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

extern void *(*gim_kmalloc)(size_t size, gfp_t flags);
extern void *(*gim_kzalloc)(size_t size, gfp_t flags);
extern void *(*gim_kmalloc_array)(size_t n, size_t size, gfp_t flags);
extern void *(*gim_vmalloc)(size_t size);
extern void *(*gim_vzalloc)(size_t size);

extern void (*gim_kfree)(const void *p);
extern void (*gim_vfree)(const void *p);

#if !defined(HAVE_KFREE_SENSITIVE)
	extern void (*gim_kzfree)(const void *p);
#else
	extern void (*gim_kfree_sensitive)(const void *p);
#endif

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
