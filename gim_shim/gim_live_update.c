/*
 * Copyright (c) 2017-2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/namei.h>
#include <linux/pci.h>
#include <linux/stat.h>
#include <linux/vmalloc.h>
#if defined(HAVE_ASM_SET_MEMORY_H)
#include <asm/set_memory.h>
#else
#include <asm/cacheflush.h>
#endif

#include "gim_live_update.h"
#include "gim_debug.h"

struct gim_live_update_manager update_mgr;

#define GIM_LIVE_UPDATE_DATA_PATH "/var/log/gim_live_update_data"
#define GIM_LIVE_UPDATE_DEFAULT_MAX_GPU 8
#define GIM_LIVE_UPDATE_DEFAULT_SIZE (sizeof(struct amdgv_live_update_file_header) + GIM_LIVE_UPDATE_DEFAULT_MAX_GPU * AMDGV_GPU_DATA_V2_SIZE)

struct amdgv_live_update_file_header *gim_live_update_get_file_header_ptr(struct gim_live_update_manager *mgr)
{
	return (struct amdgv_live_update_file_header *)(mgr->gpu_data_ptr);
}

struct amdgv_gpu_data_v2 *gim_live_update_get_data_ptr(struct gim_live_update_manager *mgr, uint32_t gpu_index)
{
	return (struct amdgv_gpu_data_v2 *)(mgr->gpu_data_ptr + sizeof(struct amdgv_live_update_file_header) + gpu_index * AMDGV_GPU_DATA_V2_SIZE);
}

static int gim_if_sriov_cap_enabled(struct amdgv_init_data *data)
{
	int pos;
	uint32_t ctrl;

	pos = pci_find_ext_capability(data->info.dev, PCI_EXT_CAP_ID_SRIOV);
	gim_oss_interfaces.pci_read_config_dword(data->info.dev, pos + PCI_SRIOV_CTRL, &ctrl);

	return ((ctrl & (PCI_SRIOV_CTRL_VFE | PCI_SRIOV_CTRL_MSE))
				== (PCI_SRIOV_CTRL_VFE | PCI_SRIOV_CTRL_MSE));
}

static bool gim_live_update_is_gpu_match(struct gim_live_update_manager *mgr, struct amdgv_init_data *data, struct amdgv_gpu_data_v2 *gpu_data)
{
	uint32_t hash;
	uint64_t hash_addr = gpu_data->header.hash_addr;
	uint64_t header_hash = gpu_data->header.hash;

	bool is_bdf_match = (data->info.bdf == gpu_data->header.bdf) || ((data->info.bdf & 0xFFFF) == gpu_data->header.bdf);
	bool is_hash_match;

	if (!is_bdf_match)
		return false;

	gim_oss_interfaces.pci_read_config_dword(data->info.dev, hash_addr, &hash);
	gim_oss_interfaces.pci_write_config_dword(data->info.dev, hash_addr, 0);
	is_hash_match = (header_hash == hash);

	gim_info_bdf(data->info.bdf, "header hash 0x%llx, pci hash 0x%x\n", header_hash, hash);

	return is_hash_match;
}

static bool gim_live_update_validate_crc(struct gim_live_update_manager *mgr)
{
	uint32_t i;
	uint32_t header_version;
	uint32_t driver_version;
	uint32_t hash;

	struct amdgv_live_update_file_header *file_header;
	struct amdgv_gpu_data_v2 *gpu_data;

	file_header = gim_live_update_get_file_header_ptr(mgr);
	if (strcmp(LIVE_INFO_SIG_STR, file_header->signature) != 0)
		return false;

	header_version = file_header->version     & LIVE_INFO_VERSION_MAJOR_MASK;
	driver_version = LIVE_INFO_HEADER_VERSION & LIVE_INFO_VERSION_MAJOR_MASK;
	if (header_version != driver_version)
		return false;

	mgr->gpu_num = file_header->gpu_num;
	if (mgr->gpu_num == 0)
		return false;

	for (i = 0; i < mgr->gpu_num; i++) {
		gpu_data = gim_live_update_get_data_ptr(mgr, i);
		gim_calc_hash_ext("crc32", &hash, (void *)&gpu_data->header.size, gpu_data->header.size);
		if (gpu_data->header.hash != hash)
			return false;
	}

	return true;
}

static bool gim_live_update_data_size_check(struct gim_live_update_manager *mgr, uint32_t current_index)
{
	return mgr->gpu_data_size >= (sizeof(struct amdgv_live_update_file_header) +
		(current_index + 1) * AMDGV_GPU_DATA_V2_SIZE);
}

static uint32_t gim_live_update_flush_file(struct gim_live_update_manager *mgr)
{
	struct amdgv_live_update_file_header *file_header;

	uint32_t size;
	uint32_t write_size;
	loff_t pos = 0;
	struct file *file;

	if (!mgr->is_export)
		return 0;

	file_header = gim_live_update_get_file_header_ptr(mgr);
	if (file_header->gpu_num == 0)
		return 0;

	if (!gim_live_update_data_size_check(mgr, file_header->gpu_num - 1)) {
		gim_warn("live update file size incorrect\n");
		return 0;
	}
	size = sizeof(file_header) + file_header->gpu_num * AMDGV_GPU_DATA_V2_SIZE;

	file = filp_open(GIM_LIVE_UPDATE_DATA_PATH, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (IS_ERR(file)) {
		gim_warn("live update file open failure\n");
		return 0;
	}

	write_size = gim_kernel_write(file, (char *)mgr->gpu_data_ptr, size, pos);
	if (write_size != size)
		gim_warn("live update file write failure\n");
	filp_close(file, NULL);

	return write_size;
}

void gim_live_update_init_manager(struct gim_live_update_manager *mgr)
{
	struct path file_path;
	struct file *file = NULL;
	struct kstat stat;
	uint32_t file_size;
	loff_t pos = 0;

	uint64_t gpu_data_addr;

	if (!mgr)
		return;

retry:
	switch (mgr->update_type) {
	case GIM_LIVE_UPDATE_FILE:
		if (kern_path(GIM_LIVE_UPDATE_DATA_PATH, LOOKUP_FOLLOW, &file_path)) {
			file_size = 0;
		} else {
			if (vfs_getattr(&file_path, &stat, STATX_SIZE, AT_STATX_SYNC_AS_STAT))
				file_size = 0;
			else
				file_size = stat.size;
			path_put(&file_path);
		}

		mgr->gpu_data_size = GIM_LIVE_UPDATE_DEFAULT_SIZE;
		mgr->gpu_data_ptr = (uint8_t *)vzalloc(mgr->gpu_data_size);
		if (!mgr->gpu_data_ptr) {
			gim_warn("live update memory allocate failed\n");
			goto failed;
		}

		file = filp_open(GIM_LIVE_UPDATE_DATA_PATH, O_RDONLY, 0);
		if (!IS_ERR(file) && file_size > 0) {
			gim_kernel_read(file, mgr->gpu_data_ptr, file_size, pos);
			filp_close(file, NULL);
		}
		break;
	case GIM_LIVE_UPDATE_MEM:
		if (mgr->gpu_data_addr_lo & (AMDGV_GPU_DATA_V2_SIZE - 1)) {
			gim_warn("live update memory address not 1MB aligned\n");
			mgr->update_type = GIM_LIVE_UPDATE_FILE;
			goto retry;
		}

		if (!gim_live_update_data_size_check(mgr, 0)) {
			gim_warn("live update memory size is not enough for 1 GPU\n");
			mgr->update_type = GIM_LIVE_UPDATE_FILE;
			goto retry;
		}

		gpu_data_addr = mgr->gpu_data_addr_lo | (((uint64_t)(mgr->gpu_data_addr_hi)) << 32);
		mgr->io_ptr = (uint8_t *)ioremap(gpu_data_addr, mgr->gpu_data_size);
		if (!mgr->io_ptr) {
			gim_warn("live update memory map failed\n");
			mgr->update_type = GIM_LIVE_UPDATE_FILE;
			goto retry;
		}

		mgr->gpu_data_ptr = mgr->io_ptr;
		break;
	default:
		goto failed;
	}

	mgr->is_valid_update = true;
	mgr->is_valid_crc    = gim_live_update_validate_crc(mgr);
	return;

failed:
	mgr->is_valid_update = false;
	mgr->is_valid_crc    = false;
	mgr->update_type     = GIM_LIVE_UPDATE_DISABLED;
	return;
}

void gim_live_update_fini_manager(struct gim_live_update_manager *mgr)
{
	if (!mgr)
		return;

	switch (mgr->update_type) {
	case GIM_LIVE_UPDATE_FILE:
		gim_live_update_flush_file(mgr);
		if (mgr->gpu_data_ptr)
			vfree(mgr->gpu_data_ptr);
		break;
	case GIM_LIVE_UPDATE_MEM:
		if (mgr->io_ptr) {
			iounmap(mgr->io_ptr);
			mgr->io_ptr = NULL;
		}
		break;
	default:
		break;
	}

	mgr->gpu_data_ptr = NULL;
}

static void gim_live_update_fill_compatibility(struct gim_live_update_manager *mgr, struct amdgv_init_data *data, struct amdgv_gpu_data_v2 *gpu_data)
{
	struct amdgv_live_update_file_header *file_header;

	file_header = gim_live_update_get_file_header_ptr(mgr);
	if (file_header->version <= LIVE_INFO_GET_HEADER_VERSION(2, 0)) {
		gpu_data->module_param.partition_full_access_enable = 1;
		gpu_data->module_param.memory_partition_mode = AMDGV_MEMORY_PARTITION_MODE_NPS1;
		gpu_data->module_param.accelerator_partition_mode = gpu_data->module_param.num_vf;
	}

	data->opt.total_vf_num = gpu_data->module_param.num_vf;
	data->opt.accelerator_partition_mode = gpu_data->module_param.accelerator_partition_mode;
	data->opt.memory_partition_mode = gpu_data->module_param.memory_partition_mode;
	data->opt.partition_full_access_enable = gpu_data->module_param.partition_full_access_enable;
}

static enum gim_live_update_data_status gim_import_data(struct gim_live_update_manager *mgr, struct gim_dev_data *dev_data)
{
	struct amdgv_init_data *data = &dev_data->init_data;
	struct amdgv_gpu_data_v2 *gpu_data;

	uint32_t i;
	uint32_t current_index = dev_data->gpu_index;
	bool is_found = false;

	enum gim_live_update_data_status ret;

	// Assume normal GIM load
	data->sys_mem_info.va_ptr = 0;
	data->sys_mem_info.bus_addr  = 0; // Not used
	data->sys_mem_info.phys_addr = 0; // Not used
	data->opt.skip_hw_init = false;

	if (mgr->update_type == GIM_LIVE_UPDATE_DISABLED) {
		gim_info_bdf(data->info.bdf, "live update is disabled\n");
		return GIM_LIVE_UPDATE_DATA_OK;
	}

	if (!mgr->is_valid_update) {
		gim_warn_bdf(data->info.bdf, "live update data has been rejected\n");
		goto out;
	}

	if (!gim_live_update_data_size_check(mgr, current_index)) {
		gim_warn_bdf(data->info.bdf, "reserved data size is not enough, or corrupted\n");
		mgr->is_valid_update = false;
		goto out;
	}

	/* There are 4 cases:
	 * 1. CRC invalid and SRIOV disabled (00). Normal gim load
	 * 2. CRC invalid and SRIOV enabled  (01). Live update data is corrupted
	 * 3. CRC valid   and SRIOV disabled (10). CRC contains residual data
	 * 4. CRC valid   and SRIOV enabled  (11). Try to proceed to live update
	 */
	if (!mgr->is_valid_crc && !gim_if_sriov_cap_enabled(data)) {
		gim_info_bdf(data->info.bdf, "live update is skipped\n");
		goto out;
	} else if (!mgr->is_valid_crc && gim_if_sriov_cap_enabled(data)) {
		gim_warn_bdf(data->info.bdf, "live update data is corrupted\n");
		mgr->is_valid_update = false;
		goto out;
	} else if (mgr->is_valid_crc && !gim_if_sriov_cap_enabled(data)) {
		gim_warn_bdf(data->info.bdf, "live update data is old\n");
		goto out;
	}

	gim_info_bdf(data->info.bdf, "live update type is %d\n", mgr->update_type);

	for (i = 0; i < mgr->gpu_num; i++) {
		gpu_data = gim_live_update_get_data_ptr(mgr, i);
		if (gim_live_update_is_gpu_match(mgr, data, gpu_data)) {
			current_index = i;
			is_found = true;
			if (data->info.bdf != gpu_data->header.bdf)
				gim_warn_bdf(data->info.bdf, "domain mismatch but bdf and header validation passed. Continue live update\n");

			gim_live_update_fill_compatibility(mgr, data, gpu_data);
			break;
		}
	}
	if (!is_found) {
		gim_warn_bdf(data->info.bdf, "failed to find live update data for GPU%u\n", current_index);
		mgr->is_valid_update = false;
		goto out;
	}

	gim_info_bdf(data->info.bdf, "proceed to live update\n");
	data->opt.skip_hw_init = true;
	data->sys_mem_info.va_ptr = gim_live_update_get_data_ptr(mgr, current_index);

out:
	if (mgr->is_valid_update)
		ret = GIM_LIVE_UPDATE_DATA_OK;
	else
		ret = GIM_LIVE_UPDATE_DATA_CORRUPTED;

	return ret;
}

static void gim_export_data(struct gim_live_update_manager *mgr, struct pci_dev *pdev, uint32_t *gim_gpu_id)
{
	struct gim_dev_data *dev_data;
	struct amdgv_init_data *data;

	struct amdgv_live_update_file_header *file_header;
	struct amdgv_gpu_data_v2 *gpu_data;

	dev_data = pci_get_drvdata(pdev);
	data = &dev_data->init_data;

	file_header = gim_live_update_get_file_header_ptr(mgr);

	if (data->fini_opt.skip_hw_fini && data->fini_opt.export_status) {
		memcpy(&file_header->signature, LIVE_INFO_SIG_STR, LIVE_INFO_SIG_SIZE);
		file_header->version = LIVE_INFO_HEADER_VERSION;
		file_header->gpu_num = *gim_gpu_id;

		gpu_data = (struct amdgv_gpu_data_v2 *)data->sys_mem_info.va_ptr;

		gpu_data->header.idx = 0; // Not used, as gim_probe is parallel
		gpu_data->header.bdf = data->info.bdf;

		gim_calc_hash_ext("crc32", &gpu_data->header.hash, (void *)&gpu_data->header.size, gpu_data->header.size);
		gim_oss_interfaces.pci_write_config_dword(pdev, gpu_data->header.hash_addr, gpu_data->header.hash);
	} else {
		// wipe header is enough to prevent mis-use
		memset(file_header, 0, sizeof(struct amdgv_live_update_file_header));
	}
}

enum gim_live_update_data_status gim_live_update_import_data(struct gim_live_update_manager *mgr, struct gim_dev_data *dev_data)
{
	return gim_import_data(mgr, dev_data);
}

void gim_live_update_export_data(struct gim_live_update_manager *mgr, struct pci_dev *pdev, uint32_t *gim_gpu_id)
{
	gim_export_data(mgr, pdev, gim_gpu_id);
}

