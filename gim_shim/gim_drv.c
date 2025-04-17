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

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include "gim_live_update.h"

#include <linux/ftrace.h>
#include "gim_ftrace.h"

#include "dcore_drv.h"

#include "gim_debug.h"
#include "gim_config.h"
#include "gim_gpumon.h"
#include "gim_error.h"
#include "gim.h"
#include "smi_drv_oss.h"
#include "amdgv_api.h"
#include "amdgv_gpumon.h"

#include "gim_guard.h"
#include "gim_debugfs.h"
#include "gim_monitor.h"

struct gim_init_thread_context {
	void *pdev;
	const void *ent;
	uint32_t gpu_id;
	void *init_thread;
};

static struct gim_init_thread_context gim_init_thread[AMDGV_MAX_GPU_NUM];
uint32_t shim_log_level = AMDGV_INFO_LEVEL;
static amdgv_dev_t adapt_list[AMDGV_MAX_GPU_NUM] = {0};
LIST_HEAD(gim_device_list);
struct mutex gim_device_list_lock;
struct mutex gim_xgmi_hive_lock;
struct mutex gim_xgmi_gpumon_hive_lock;

struct gim_error_ring_buffer *gim_error_rb;

static atomic64_t gim_gpu_initing_num;

struct completion gim_gpu_init_event;
static uint32_t gim_gpu_id;
extern uint gpu_data_addr_hi;
extern uint gpu_data_addr_lo;
extern uint gpu_data_size;
extern struct gim_live_update_manager update_mgr;
static const char gim_driver_name[] = "gim";
const char gim_driver_version[] = PACKAGE_VERSION;
static const char gim_driver_string[] =
		"GPU IOV MODULE";
static const char gim_copyright[] =
		"Copyright (c) 2014-2025 Advanced Micro Devices, Inc.";

static int gim_map_pci_res(struct amdgv_init_data *data, struct pci_dev *pdev)
{
	int ret;

	ret = pci_request_regions(pdev, gim_driver_name);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION, 0);
		return ret;
	}

	/* framebuffer bar mapping */
	data->info.fb_pa = pci_resource_start(pdev, 0);
	data->info.fb_size = pci_resource_len(pdev, 0);
	data->info.fb = devm_ioremap_wc(&pdev->dev,
					data->info.fb_pa,
					data->info.fb_size);
	if (data->info.fb == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_FB_MAP_FAIL, 0);
		goto err;
	}

	/* doorbell bar mapping */
	data->info.doorbell_pa = pci_resource_start(pdev, 2);
	data->info.doorbell_size = pci_resource_len(pdev, 2);
	data->info.doorbell = devm_ioremap(&pdev->dev,
					data->info.doorbell_pa,
					data->info.doorbell_size);
	if (data->info.doorbell == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_DOORBELL_MAP_FAIL, 0);
		goto err;
	}

	/* io port mapping */
	data->info.io_mem_pa = pci_resource_start(pdev, 4);
	data->info.io_mem_size = pci_resource_len(pdev, 4);
	if (data->info.io_mem_size) {
		data->info.io_mem = __pci_ioport_map(&pdev->dev,
					data->info.io_mem_pa,
					data->info.io_mem_size);
		if (data->info.io_mem == NULL)
			goto err;
	}

	/* register mmio mapping */
	data->info.mmio_pa = pci_resource_start(pdev, 5);
	data->info.mmio_size = pci_resource_len(pdev, 5);
	data->info.mmio = devm_ioremap(&pdev->dev,
					data->info.mmio_pa,
					data->info.mmio_size);
	if (data->info.mmio == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_MMIO_FAIL, 0);
		goto err;
	}

	return 0;

err:
	pci_release_regions(pdev);
	return -ENOMEM;
}

static void gim_release_pci_res(struct amdgv_init_data *data,
				struct pci_dev *pdev)
{
	devm_iounmap(&pdev->dev, data->info.mmio);
	devm_iounmap(&pdev->dev, data->info.doorbell);
	devm_iounmap(&pdev->dev, data->info.fb);
	if (data->info.io_mem)
		pci_iounmap(pdev, data->info.io_mem);

	pci_release_regions(pdev);
}

static void gim_set_sriov_data(struct amdgv_init_data *data,
				struct pci_dev *pdev)
{
	int pos;

	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_SRIOV);
	data->info.sriov_cap_pos = pos;

	gim_oss_interfaces.pci_read_config_word(pdev, pos + PCI_SRIOV_VF_OFFSET,
				&data->info.sriov_vf_offset);
	gim_oss_interfaces.pci_read_config_word(pdev, pos + PCI_SRIOV_VF_STRIDE,
				&data->info.sriov_vf_stride);
	gim_oss_interfaces.pci_read_config_word(pdev, pos + PCI_SRIOV_VF_DID,
				&data->info.sriov_vf_devid);
}

static int gim_build_vfs_map(struct gim_dev_data *data)
{
	int i, ret = 0;
	uint32_t vf_num;
	unsigned int domain;
	unsigned int bus;
	unsigned int devfn;
	amdgv_dev_t adev;
	struct pci_dev *pdev;
	union amdgv_dev_info dev_info;

	/* use scratch */

	mutex_lock(&data->mem_lock);

	memset(&data->vf_info, 0, sizeof(data->vf_info));

	adev = data->adev;

	memset(&dev_info, 0, sizeof(dev_info));
	if (amdgv_get_dev_info(adev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		vf_num = 0;
	else
		vf_num = dev_info.vf.num_enabled_vf;

	data->vf_num = vf_num;
	for (i = 0; i < vf_num; i++) {
		if (amdgv_get_vf_info(adev, i, AMDGV_GET_VF_BDF, &data->vf_info))
			goto err_vf_dev;

		data->vf_map[i].bdf = data->vf_info.id.bdf;

		domain = data->vf_info.id.bdf >> 16;
		bus = data->vf_info.id.bdf >> 8 & 0xff;
		devfn = data->vf_info.id.bdf & 0xff;
		pdev = pci_get_domain_bus_and_slot(domain, bus, devfn);
		if (!pdev)
			goto err_vf_dev;
		data->vf_map[i].pdev = pdev;
	}

	goto out;

err_vf_dev:
	/* clear all vf info if one vf failed to get its pci device */
	ret = -ENODEV;
	memset(&data->vf_info, 0, sizeof(data->vf_info));
	memset(data->vf_map, 0, sizeof(data->vf_map));
	data->vf_num = 0;
out:
	mutex_unlock(&data->mem_lock);

	return ret;
}

int gim_get_vf_idx(struct pci_dev *vf_pdev, struct gim_dev_data *data)
{
	int i;

	for (i = 0; i < AMDGV_MAX_VF_NUM; i++) {
		if (data->vf_map[i].pdev == vf_pdev)
			return i;
	}

	return -1;
}

int gim_dbdf_to_vf_idx(uint32_t bdf, struct gim_dev_data *data)
{
	int i, ret = -1;

	for (i = 0; i < data->vf_num; i++) {
		if (data->vf_map[i].bdf == bdf) {
			ret = i;
			break;
		}
	}

	return ret;
}

static int gim_init_thread_func(void *context)
{
	int ret = 0;
	struct gim_init_thread_context *init_context =
			(struct gim_init_thread_context *)context;
	struct pci_dev *pdev = (struct pci_dev *)init_context->pdev;
	struct gim_dev_data *dev_data;
	struct amdgv_init_data *data;
	struct pci_dev *parent, *tmp = NULL;

	/* set init data */
	dev_data = devm_kzalloc(&pdev->dev,
				sizeof(struct gim_dev_data), GFP_KERNEL);
	if (!dev_data) {
		ret = -ENOMEM;
		goto out;
	}

	mutex_init(&dev_data->mem_lock);
	mutex_init(&dev_data->dev_lock);

	dev_data->gpu_index = init_context->gpu_id;

	data = &dev_data->init_data;
	ret = gim_map_pci_res(data, pdev);
	if (ret)
		goto err_mem_free;

	gim_set_sriov_data(data, pdev);

	data->info.dev = pdev;
	data->info.bdf = PCI_DEVID(pdev->bus->number, pdev->devfn) | (pci_domain_nr(pdev->bus) << 16);
	data->info.vendor_id = pdev->vendor;
	data->info.dev_id = pdev->device;
	data->info.rev_id = pdev->revision;
	data->info.sub_sys_id = pdev->subsystem_device;
	data->info.sub_vnd_id = pdev->subsystem_vendor;
	data->info.hive_lock = &gim_xgmi_hive_lock;
	data->info.gpumon_hive_lock = &gim_xgmi_gpumon_hive_lock;

	data->opt.total_vf_num = gim_conf_get_vf_num_opt(dev_data->gpu_index);

	data->opt.flags |= AMDGV_FLAG_DISABLE_SELF_SWITCH;
	data->opt.flags |= AMDGV_FLAG_ENABLE_ECC;

	gim_ftrace_add_task_entry(current, data->info.bdf, 0);

	data->opt.bad_page_detection_mode =
		gim_conf_get_skip_check_bgpu_opt(dev_data->gpu_index);

	if (gim_conf_get_guard_opt(dev_data->gpu_index))
		data->opt.flags |= AMDGV_FLAG_SENSITIVE_EVENT_GUARD;
	if (gim_conf_get_hang_detection_mode_opt(dev_data->gpu_index))
		data->opt.hang_detection_mode = 1;
	if (gim_conf_get_asymmetric_fb_mode_opt(dev_data->gpu_index))
		data->opt.asymmetric_fb_mode = 1;

	data->opt.allow_time_full_access =
		gim_conf_get_fullacces_timeout_opt(dev_data->gpu_index);
	data->opt.gfx_sched_mode =
		gim_conf_get_sch_policy_opt(dev_data->gpu_index);
	data->opt.fw_load_type =
		gim_conf_get_fw_load_type_opt(dev_data->gpu_index);
	data->opt.perf_mon_enable =
		gim_conf_get_perf_mon_enable_opt(dev_data->gpu_index);
	data->opt.pf_option = NULL;
	data->opt.vf_opts_num = 0;
	data->opt.vf_options = NULL;

	data->opt.log_level =
		gim_conf_get_log_level_opt(dev_data->gpu_index);
	data->opt.log_mask = ~0;
	shim_log_level = data->opt.log_level;

	data->opt.power_saving_mode = power_saving_mode;
	if (power_saving_mode != AMDGV_IPS_POWER_SAVING_DISABLE)
		data->opt.flags |= AMDGV_FLAG_IPS_POWER_SAVING;

	data->opt.mm_policy = gim_conf_get_mm_policy_opt(dev_data->gpu_index);

	data->opt.libgv_res_fb_offset = 0;
	data->opt.libgv_res_fb_size = ((uint64_t) gim_conf_get_pf_fb_size_opt(dev_data->gpu_index) << 20);
	data->opt.debug_dump_reserve_size =
		gim_conf_get_debug_dump_reserve_size_opt(dev_data->gpu_index);
	data->opt.deferred_full_live_update = gim_conf_get_deferred_full_live_update_opt(dev_data->gpu_index);
	data->opt.bp_debug_mode = gim_conf_get_bp_mode_opt(dev_data->gpu_index);

	data->opt.fb_sharing_mode =
		gim_conf_get_fb_sharing_mode_opt(dev_data->gpu_index);
	data->opt.accelerator_partition_mode = gim_conf_get_accelerator_partition_mode_opt(dev_data->gpu_index);
	data->opt.memory_partition_mode = gim_conf_get_memory_partition_mode_opt(dev_data->gpu_index);
	data->opt.partition_full_access_enable = gim_conf_get_partition_full_access_enable_opt(dev_data->gpu_index);
	data->opt.bad_page_record_threshold =
		gim_conf_get_bad_page_record_threshold_opt(dev_data->gpu_index);
	data->opt.ras_vf_telemetry_policy = gim_conf_get_ras_vf_telemetry_policy_opt(dev_data->gpu_index);
	data->opt.max_cper_count = gim_conf_get_max_cper_count_opt(dev_data->gpu_index);

	/* Initialize device and enable SRIOV */
	if (pci_enable_device(pdev) != 0) {
		ret = -EIO;
		gim_put_error(AMDGV_ERROR_DRIVER_PCI_ENABLE_DEVICE_FAIL,
				PCI_DEVID(pdev->bus->number, pdev->devfn));
		goto err_pci_unmap;
	}

	pci_set_master(pdev);
	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(44));
	pdev->dev_flags |= PCI_DEV_FLAGS_MSI_INTX_DISABLE_BUG;

	dev_data->pdev = pdev;
	if (gim_live_update_import_data(&update_mgr, dev_data)) {
		gim_warn("Live update fail due to data corrupted!\n");
		goto out;
	}
	if (data->opt.skip_hw_init) {
		/* Restore persistent settings to config file */
		if (gim_conf_set_vf_num_opt(data->opt.total_vf_num))
			gim_warn("Failed to restore vf_num %d to gim config!\n",
					data->opt.total_vf_num);
		if (gim_conf_set_accelerator_partition_mode_opt(data->opt.accelerator_partition_mode))
			gim_warn("Failed to restore accelerator partition mode %d to gim config!\n",
					data->opt.accelerator_partition_mode);
		if (gim_conf_set_memory_partition_mode_opt(data->opt.memory_partition_mode))
			gim_warn("Failed to restore memory partition mode %d to gim config!\n",
					data->opt.memory_partition_mode);
	}
	dev_data->adev = amdgv_device_init(data);
	if (dev_data->adev == AMDGV_INVALID_HANDLE) {
		gim_put_error(AMDGV_ERROR_DRIVER_DEV_INIT_FAIL,
			PCI_DEVID(pdev->bus->number, pdev->devfn));
	}

	if (dev_data->adev != AMDGV_INVALID_HANDLE) {
		if (gim_build_vfs_map(dev_data))
			gim_put_error(AMDGV_ERROR_DRIVER_DEV_INIT_FAIL,
				PCI_DEVID(pdev->bus->number, pdev->devfn));
	}

	pci_set_drvdata(pdev, dev_data);

	mutex_lock(&gim_device_list_lock);

	tmp = dev_data->pdev;
	if (dev_data->adev != AMDGV_INVALID_HANDLE) {
		list_add_tail(&dev_data->list, &gim_device_list);
		do {
			parent = tmp;
			tmp = pci_upstream_bridge(parent);
		} while ((tmp != NULL) && (tmp->vendor == 0x1002));

		dev_data->parent = -1;

		if (tmp)
			dev_data->parent = tmp->bus->primary;

		adapt_list[dev_data->gpu_index] = dev_data->adev;
		amdgv_adapt_list_update(dev_data->adev, &adapt_list[0]);
	}

	mutex_unlock(&gim_device_list_lock);

	if (dev_data->adev != AMDGV_INVALID_HANDLE) {
		gim_guard_init_dev_sys(pdev);
		gim_mon_create_dev_sys(dev_data);
	};
	gim_info("AMD GIM probed GPU(%u) %s\n",
		dev_data->gpu_index, dev_name(&pdev->dev));
	goto out;

err_pci_unmap:
	gim_release_pci_res(data, pdev);
err_mem_free:
	devm_kfree(&pdev->dev, dev_data);
out:
	atomic64_dec(&gim_gpu_initing_num);
	complete(&gim_gpu_init_event);
	init_context->init_thread = NULL;
	return ret;
}

static bool gim_is_device_enabled(struct pci_dev *pdev)
{
	char *pciaddstr = NULL, *pciaddstr_tmp, *pciaddname_tmp;
	bool ret = true;

	if (gim_enabled_devices) {
		const char *pci_address_name = pci_name(pdev);

		ret = false;
		pciaddstr = kstrdup(gim_enabled_devices, GFP_KERNEL);
		pciaddstr_tmp = pciaddstr;

		while ((pciaddname_tmp = strsep(&pciaddstr_tmp, ";"))) {
			if (!strcmp(pci_address_name, pciaddname_tmp)) {
				ret = true;
				break;
			}
		}
	}

	kfree(pciaddstr);
	return ret;
}

static int gim_probe(struct pci_dev *pdev,
		const struct pci_device_id *ent)
{
	int gpu_id;

	atomic64_inc(&gim_gpu_initing_num);

	/* skip this device if user want that */
	if (gim_is_device_enabled(pdev) == false) {
		gim_info("AMD GIM skip probing device %s\n", dev_name(&pdev->dev));
		goto err_out;
	}

	gim_info("AMD GIM start to probe device %s\n", dev_name(&pdev->dev));

	if (pdev->sriov == NULL) {
		gim_put_error(AMDGV_ERROR_IOV_ASIC_NO_SRIOV_SUPPORT, 0);
		goto err_out;
	}

	gpu_id = gim_gpu_id;
	gim_gpu_id++;

	if (gpu_id >= AMDGV_MAX_GPU_NUM) {
		gim_warn("gpu_id is %u > AMDGV_MAX_GPU_NUM(32)\n", gpu_id);
		goto err_out;
	}

	gim_init_thread[gpu_id].gpu_id = gpu_id;
	gim_init_thread[gpu_id].ent = ent;
	gim_init_thread[gpu_id].pdev = pdev;
	gim_init_thread[gpu_id].init_thread = kthread_run(gim_init_thread_func,
			(void *)&gim_init_thread[gpu_id], "gpu_init_thread");

	if (IS_ERR(gim_init_thread[gpu_id].init_thread)) {
		gim_warn("failed to create gpu init thread!\n");
		goto err_out;
	}

	return 0;

err_out:
	atomic64_dec(&gim_gpu_initing_num);
	complete(&gim_gpu_init_event);

	return 0;
}

static void gim_remove(struct pci_dev *pdev)
{
	struct gim_dev_data *dev_data;
	struct amdgv_init_data *data;

	dev_data = pci_get_drvdata(pdev);

	if (!dev_data) /* this device is skipped */
		return;

	data = &dev_data->init_data;

	if (dev_data->adev != AMDGV_INVALID_HANDLE) {
		gim_mon_remove_dev_sys(dev_data);
		gim_guard_remove_dev_sys(pdev);
		mutex_lock(&gim_device_list_lock);
		list_del(&dev_data->list);
		adapt_list[dev_data->gpu_index] = NULL;
		mutex_unlock(&gim_device_list_lock);

		amdgv_device_fini_ex(dev_data->adev, &data->fini_opt);
		gim_live_update_export_data(&update_mgr, pdev, &gim_gpu_id);
	}

	if (!data->fini_opt.skip_hw_fini)
		pci_disable_device(pdev);

	gim_release_pci_res(data, pdev);

	devm_kfree(&pdev->dev, dev_data);

	gim_info("AMD GIM removed device %s\n", dev_name(&pdev->dev));
}

static void gim_shutdown(struct pci_dev *pdev)
{
	struct gim_dev_data *dev_data;
	struct amdgv_init_data *data;

	dev_data = pci_get_drvdata(pdev);

	if (!dev_data) /* this device is skipped */
		return;

	data = &dev_data->init_data;
	amdgv_device_fini_ex(dev_data->adev, &data->fini_opt);
	gim_live_update_export_data(&update_mgr, pdev, &gim_gpu_id);
	gim_info("AMD GIM shutdown\n");
}

static pci_ers_result_t gim_io_error_detected(struct pci_dev *pdev,
		pci_channel_state_t state)
{
	gim_info("AMD GIM io error detected\n");

	/* Request a slot reset. */
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t gim_io_slot_reset(struct pci_dev *pdev)
{
	gim_info("AMD GIM io slot reset\n");
	return PCI_ERS_RESULT_RECOVERED;
}

static void gim_io_resume(struct pci_dev *pdev)
{
	gim_info("AMD GIM io resume\n");

}

static struct pci_error_handlers gim_err_handler = {
	.error_detected = gim_io_error_detected,
	.slot_reset = gim_io_slot_reset,
	.resume = gim_io_resume,
};

static struct pci_driver gim_driver = {
	.name     = gim_driver_name,
	.id_table = NULL,
	.probe    = gim_probe,
	.remove   = gim_remove,
	.shutdown = gim_shutdown,
	.err_handler = &gim_err_handler
};

static inline const char *gim_get_memory_partition_mode_desc(
	enum amdgv_memory_partition_mode memory_partition_mode)
{
	switch (memory_partition_mode) {
	case AMDGV_MEMORY_PARTITION_MODE_NPS1:
		return "NPS1";
	case AMDGV_MEMORY_PARTITION_MODE_NPS2:
		return "NPS2";
	case AMDGV_MEMORY_PARTITION_MODE_NPS4:
		return "NPS4";
	case AMDGV_MEMORY_PARTITION_MODE_NPS8:
		return "NPS8";
	default:
		return "UNKNOWN";
	}

	return "UNKNOWN";
}

static void gim_set_dynamic_partition_mode(void)
{
	struct gim_dev_data *dev_data;
	struct amdgv_gpumon_memory_partition_info curr_memory_partition_info;
	int ret;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		dev_data->init_data.opt.memory_partition_mode = gim_conf_get_memory_partition_mode_opt(dev_data->gpu_index);
		ret = amdgv_gpumon_get_memory_partition_mode(dev_data->adev, &curr_memory_partition_info);
		if (ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED) {
			gim_dbg("memory partition is not supported on the GPU %s\n", dev_name(&dev_data->pdev->dev));
		} else if (ret) {
			gim_info("failed to get current NPS mode of GPU %s\n", dev_name(&dev_data->pdev->dev));
		} else if (curr_memory_partition_info.memory_partition_mode !=
			dev_data->init_data.opt.memory_partition_mode) {
			gim_info("NPS mode mismatch. "
				"curr_memory_partition_mode=%s saved_memory_partition_mode=%s\n",
				gim_get_memory_partition_mode_desc(curr_memory_partition_info.memory_partition_mode),
				gim_get_memory_partition_mode_desc(dev_data->init_data.opt.memory_partition_mode));
			gim_info("force NPS mode to %s\n",
				gim_get_memory_partition_mode_desc(dev_data->init_data.opt.memory_partition_mode));
			amdgv_gpumon_set_memory_partition_mode(
				dev_data->adev, dev_data->init_data.opt.memory_partition_mode);
			break;
		}
	}
}

static int gim_init(void)
{
	int i;
	int ret = 0;
	uint32_t flags;
	struct amdgv_device_ids dev_ids[AMDGV_MAX_SUPPORT_DEVICE_TYPE_NUM];


	gim_info("Start AMD open source GIM initialization\n");

	gim_info("%s - version %s\n",
		gim_driver_string, gim_driver_version);

	gim_info("%s\n", gim_copyright);

	/* Start ftrace here */
	if (gim_ftrace_init(adapt_list) != 0)
		gim_warn("Unable to init ftracing\n");

	gim_error_ring_buffer_init(&gim_error_rb);

	gim_gpu_id = 0;

	atomic64_set(&gim_gpu_initing_num, 0);

	for (i = 0; i < AMDGV_MAX_GPU_NUM; i++)
		gim_init_thread[i].init_thread = NULL;

	ret = gim_conf_init();
	if (ret)
		goto err_conf_init;

	flags = 0;
	mutex_init(&gim_device_list_lock);
	mutex_init(&gim_xgmi_hive_lock);
	mutex_init(&gim_xgmi_gpumon_hive_lock);

	/* Initialize LibGV */
	ret = amdgv_init_ex(&gim_oss_interfaces, dev_ids, flags);
	if (ret)
		goto err_conf_init;

	ret = pci_register_driver(&gim_driver);

	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_PCI_REGISTER_DRIVER_FAIL, 0);
		goto err_reg_drv;
	}
	init_completion(&gim_gpu_init_event);

	memset(&update_mgr, 0, sizeof(update_mgr));
	update_mgr.update_type = GIM_LIVE_UPDATE_FILE;
	if (gpu_data_size) {
		update_mgr.update_type = GIM_LIVE_UPDATE_MEM;
		update_mgr.gpu_data_addr_lo = gpu_data_addr_lo;
		update_mgr.gpu_data_addr_hi = gpu_data_addr_hi;
		update_mgr.gpu_data_size = gpu_data_size;
	}
	gim_live_update_init_manager(&update_mgr);
	/* Add dynamic device ID */
	for (i = 0; i < AMDGV_MAX_SUPPORT_DEVICE_TYPE_NUM; i++) {
		if (dev_ids[i].dev_id == 0)
			break;
		ret = pci_add_dynid(&gim_driver, PCI_VENDOR_ID_ATI,
				dev_ids[i].dev_id, dev_ids[i].sub_vendor_id,
				dev_ids[i].sub_dev_id, 0, 0, 0);
		if (ret) {
			gim_put_error(AMDGV_ERROR_DRIVER_NO_ACCESS_PCI_REGION,
				dev_ids[i].dev_id);
			goto err_create_mon;
		}
	}

	while (atomic64_read(&gim_gpu_initing_num) != 0)
		wait_for_completion_interruptible_timeout(&gim_gpu_init_event,
					usecs_to_jiffies(1000));

	ret = gim_mon_create_drv_sys(&gim_driver.driver);
	if (ret)
		goto err_create_mon;
	ret = gim_guard_init_drv_sys(&gim_driver.driver);
	if (ret)
		goto err_create_gurad;
	gim_debugfs_init();

	ret = gim_cmd_handler_init();
	if (ret)
		goto err_gim_cmd_handler_init;

	ret = smi_init(&gim_oss_interfaces, &gim_smi_interface);
	if (ret)
		goto err_smi_init;

	ret = dcore_init();
	if (ret)
		goto err_dcore_init;
	gim_info("AMD GIM is Running\n");

	gim_set_dynamic_partition_mode();

	return 0;

err_dcore_init:

	smi_cleanup();
err_smi_init:
	gim_cmd_handler_fini();
err_gim_cmd_handler_init:
	gim_debugfs_fini();
	gim_guard_remove_drv_sys(&gim_driver.driver);

err_create_gurad:
	gim_mon_remove_drv_sys(&gim_driver.driver);

err_create_mon:
	pci_unregister_driver(&gim_driver);

err_reg_drv:
	amdgv_fini();

err_conf_init:
	gim_ftrace_fini();

	return ret;
}

static void gim_exit(void)
{
	bool is_continue_exit = false;

	struct gim_dev_data *dev_data;
	int i = 0;
	struct pci_dev *pdev_vf;
	uint32_t vf_bdf;
	bool gim_in_use = false;

	is_continue_exit = !update_mgr.is_export;

	gim_info("GIM exit!\n");
	if (power_saving_mode != AMDGV_IPS_POWER_SAVING_DISABLE) {
		int status = 0;
		list_for_each_entry(dev_data, &gim_device_list, list) {
			amdgv_query_power_saving_status(dev_data->adev, &status);
			if (status)
				amdgv_toggle_power_saving(dev_data->adev, 0);
		}
	}

	while (is_continue_exit) {
		list_for_each_entry(dev_data, &gim_device_list, list) {
			for (i = 0; i < dev_data->vf_num; i++) {
				pdev_vf = dev_data->vf_map[i].pdev;
				vf_bdf = dev_data->vf_map[i].bdf;
				if (atomic_read(&pdev_vf->enable_cnt) > 0) {
					gim_info("GIM is used by [%x:%x:%x:%x]\n",
						(vf_bdf >> 16) & (0xffff),
						(vf_bdf >> 8) & (0xff),
						(vf_bdf >> 3) & (0x1f),
						(vf_bdf)      & (0x7));

					gim_in_use = true;
				}
			}
			if (amdgv_in_whole_gpu_reset(dev_data->adev)) {
				gim_info("GIM is used by [%x:%x:%x:%x], GPU is in reset state\n",
				0x0,
				dev_data->pdev->bus->number,
				(dev_data->pdev->devfn >> 3) & 0x1f,
				(dev_data->pdev->devfn) & 0x7);

				gim_in_use = true;
			}
		}
		if (gim_in_use) {
			msleep(5000);
			gim_in_use = false;
		} else
			break;
	}

	gim_cmd_handler_fini();
	smi_cleanup();
	gim_debugfs_fini();
	gim_guard_remove_drv_sys(&gim_driver.driver);
	gim_mon_remove_drv_sys(&gim_driver.driver);

	gim_ftrace_fini();

	pci_unregister_driver(&gim_driver);

	amdgv_fini();

	mutex_destroy(&gim_device_list_lock);
	mutex_destroy(&gim_xgmi_hive_lock);
	mutex_destroy(&gim_xgmi_gpumon_hive_lock);

	gim_live_update_fini_manager(&update_mgr);

	dcore_cleanup();

	gim_error_ring_buffer_fini(&gim_error_rb);
}

module_init(gim_init)
module_exit(gim_exit)

MODULE_VERSION(PACKAGE_VERSION);
MODULE_AUTHOR("Advanced Micro Devices, Inc.");
MODULE_DESCRIPTION("GPU IOV MODULE");
MODULE_LICENSE("Dual MIT/GPL");
