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
#include <linux/device.h>

#include "gim_debug.h"
#include "gim_config.h"
#include "gim.h"
#include "gim_gpumon.h"
#include "amdgv_error.h"
#include "gim_monitor.h"

extern struct gim_error_ring_buffer *gim_error_rb;
#include "gim_live_update.h"
extern struct gim_live_update_manager update_mgr;
extern bool gim_live_update_from_file;

static ssize_t gim_mon_reset_gpu_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct gim_dev_data *data;
	size_t count;
	int ret;

	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	/* reset GPU */
	ret = amdgv_force_reset_gpu(data->adev);
	if (ret)
		count = sprintf(buf, "failed to do dev(%s) reset\n",
				dev_name(dev));
	else
		count = sprintf(buf, "dev(%s) reset completed\n",
				dev_name(dev));
	return count;
}

static DEVICE_ATTR(reset_gpu, (0400), gim_mon_reset_gpu_show, NULL);

static ssize_t gim_mon_self_switch_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	size_t count;
	struct gim_dev_data *data;
	union amdgv_dev_conf conf;
	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	amdgv_get_dev_conf(data->adev,
			AMDGV_CONF_DISABLE_SELF_SWITCH_FLAG,
			&conf);

	if (conf.flag_switch == 0)
		count = sprintf(buf, "1\n");
	else
		count = sprintf(buf, "0\n");

	return count;
}

static ssize_t gim_mon_self_switch_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	uint32_t self_switch;
	struct gim_dev_data *data;
	union amdgv_dev_conf conf;
	int ret = 0;

	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	if (sscanf(buf, "%u", &self_switch) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (self_switch == 0)
		conf.flag_switch = 1;
	else
		conf.flag_switch = 0;

	ret = amdgv_set_dev_conf(data->adev,
			AMDGV_CONF_DISABLE_SELF_SWITCH_FLAG,
			&conf);
	if (ret) {
		pr_warn("Failed to set self switch for [%s]\n",
				dev_name(&data->pdev->dev));
	}

	return count;
}

static DEVICE_ATTR(self_switch, (0600),
			gim_mon_self_switch_show,
			gim_mon_self_switch_store);


static ssize_t gim_mon_psp_vbflash_store(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr,
				char *buffer, loff_t pos, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct pci_dev *pdev = to_pci_dev(dev);
	struct gim_dev_data *data = pci_get_drvdata(pdev);

	if (amdgv_load_vbflash_bin(data->adev, buffer, pos, count))
		return -ENOMEM;

	return count;
}

static ssize_t gim_mon_psp_vbflash_show(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr, char *buffer,
				loff_t pos, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct pci_dev *pdev = to_pci_dev(dev);
	struct gim_dev_data *data = pci_get_drvdata(pdev);
	int ret = 0;

	ret = amdgv_update_spirom(data->adev);

	return ret;
}

static const struct bin_attribute psp_vbflash_bin_attr = {
	.attr = {.name = "psp_vbflash", .mode = 0664},
	.size = 0,
	.write = gim_mon_psp_vbflash_store,
	.read = gim_mon_psp_vbflash_show,
};

static ssize_t gim_mon_psp_vbflash_status(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct gim_dev_data *data = pci_get_drvdata(pdev);
	uint32_t vbflash_status;

	amdgv_get_vbflash_status(data->adev, &vbflash_status);

	return sprintf(buf, "0x%x\n", vbflash_status);
}

static DEVICE_ATTR(psp_vbflash_status, (0444),
			gim_mon_psp_vbflash_status, NULL);

static inline const char *gim_mon_get_resource_type_desc(enum amdgv_gpumon_accelerator_partition_resource_type resource_type)
{
	switch (resource_type) {
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC:
		return "XCC";
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_ENCODER:
		return "ENCODER";
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER:
		return "DECODER";
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DMA:
		return "DMA";
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_JPEG:
		return "JPEG";
	default:
		return "UNKNOWN";
	}

	return "UNKNOWN";
}

static inline const char *gim_mon_get_profile_type_desc(enum amdgv_gpumon_acccelerator_partition_type profile_type)
{
	switch (profile_type) {
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_SPX:
		return "SPX";
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_DPX:
		return "DPX";
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_QPX:
		return "QPX";
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_CPX:
		return "CPX";
	default:
		return "UNKNOWN";
	}

	return "UNKNOWN";
}

static inline const char *gim_mon_get_mem_desc(union amdgv_gpumon_memory_partition_config memory_caps)
{
	static char memory_desc[128] = {0};

	if (memory_caps.mp_caps.nps1_cap) {
		strcpy(memory_desc, "NPS1 ");
	}
	if (memory_caps.mp_caps.nps2_cap) {
		strcat(memory_desc, "NPS2 ");
	}
	if (memory_caps.mp_caps.nps4_cap) {
		strcat(memory_desc, "NPS4 ");
	}
	if (memory_caps.mp_caps.nps8_cap) {
		strcat(memory_desc, "NPS8");
	}

	return memory_desc;
}

static ssize_t gim_mon_accelerator_partition_profile_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int ret;
	size_t count = 0;
	int j, k;
	struct gim_dev_data *data;
	struct amdgv_gpumon_acccelerator_partition_profile accelerator_partition_profile;
	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	ret = amdgv_gpumon_get_accelerator_partition_profile(data->adev, &accelerator_partition_profile);
	if (ret) {
		count = sprintf(buf, "failed to get current accelerator partition profile\n");
	} else {
		count += sprintf(buf + count, "%-20s%-20s%-20s%-20s%-20s\n",
				"profile_index", "profile_type", "num_of_partitions", "num_of_resources", "resource_index_in_resource_profiles");
		count += sprintf(buf + count, "%-20u%-20s%-20u%-20u[",
			accelerator_partition_profile.profile_index,
			gim_mon_get_profile_type_desc(accelerator_partition_profile.profile_type),
			accelerator_partition_profile.num_partitions,
			accelerator_partition_profile.num_resources
			);
		for (j = 0; j < accelerator_partition_profile.num_partitions; j++) {
			count += sprintf(buf + count, "[");
			for (k = 0; k < accelerator_partition_profile.num_resources; k++) {
				count += sprintf(buf + count, "%u ",
					accelerator_partition_profile.resources[j][k]);
			}
			count += sprintf(buf + count, "]");
		}
		count += sprintf(buf + count, "]\n");
	}

	return count;
}

static ssize_t gim_mon_accelerator_partition_profile_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	struct gim_dev_data *data;
	uint32_t profile_index;

	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	if (sscanf(buf, "%u", &profile_index) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	ret = amdgv_gpumon_set_accelerator_partition_profile(data->adev, profile_index);
	if (ret) {
		pr_warn("Failed to set accelerator_partition_profile profile_index=%u\n", profile_index);
	}

	return count;
}

static DEVICE_ATTR(accelerator_partition_profile, (0600),
			gim_mon_accelerator_partition_profile_show,
			gim_mon_accelerator_partition_profile_store);

static inline const char *gim_mon_get_memory_partition_mode_desc(enum amdgv_memory_partition_mode memory_partition_mode)
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

static ssize_t gim_mon_memory_partition_mode_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int ret;
	size_t count = 0;
	uint32_t i;
	struct gim_dev_data *data;
	struct amdgv_gpumon_memory_partition_info memory_partition_info;
	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	ret = amdgv_gpumon_get_memory_partition_mode(data->adev, &memory_partition_info);
	if (ret) {
		count = sprintf(buf, "failed to get current mp mode\n");
	} else {
		count = sprintf(buf, "memory_partition_mode: %s\n",
			gim_mon_get_memory_partition_mode_desc(memory_partition_info.memory_partition_mode));
		count += sprintf(buf + count, "numa_count: %d\n",
			memory_partition_info.num_numa_ranges);
		for (i = 0; i < memory_partition_info.num_numa_ranges; i++) {
			count += sprintf(buf + count, "NUMA%u type=%u start=0x%016llx end=0x%016llx\n",
				i,
				memory_partition_info.numa_range[i].memory_type,
				memory_partition_info.numa_range[i].start,
				memory_partition_info.numa_range[i].end);
		}
	}

	return count;
}

static inline enum amdgv_memory_partition_mode gim_mon_get_memory_partition_mode(const char *mp_desc)
{
	if (!strncasecmp("NPS1", mp_desc, strlen("NPS1"))) {
		return AMDGV_MEMORY_PARTITION_MODE_NPS1;
	} else if (!strncasecmp("NPS2", mp_desc, strlen("NPS2"))) {
		return AMDGV_MEMORY_PARTITION_MODE_NPS2;
	} else if (!strncasecmp("NPS4", mp_desc, strlen("NPS4"))) {
		return AMDGV_MEMORY_PARTITION_MODE_NPS4;
	} else if (!strncasecmp("NPS8", mp_desc, strlen("NPS8"))) {
		return AMDGV_MEMORY_PARTITION_MODE_NPS4;
	} else {
		return AMDGV_MEMORY_PARTITION_MODE_UNKNOWN;
	}
}

static ssize_t gim_mon_memory_partition_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	enum amdgv_memory_partition_mode memory_partition_mode;
	char memory_partition_mode_desc[5] = {0};
	struct gim_dev_data *data;

	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	if (sscanf(buf, "%4s", memory_partition_mode_desc) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	memory_partition_mode = gim_mon_get_memory_partition_mode(memory_partition_mode_desc);
	if (memory_partition_mode == AMDGV_MEMORY_PARTITION_MODE_UNKNOWN) {
		pr_warn("invalid parameter %s\n", memory_partition_mode_desc);
		return -EINVAL;
	}

	ret = amdgv_gpumon_set_memory_partition_mode(data->adev, memory_partition_mode);
	if (ret) {
		pr_warn("Failed to set mp mode %s\n",
				memory_partition_mode_desc);
	}

	return count;
}

static DEVICE_ATTR(memory_partition_mode, (0600),
			gim_mon_memory_partition_mode_show,
			gim_mon_memory_partition_mode_store);

static inline const char *gim_mon_get_spatial_partition_mode_desc(uint32_t spatial_partition_num)
{
	switch (spatial_partition_num) {
	case 1:
		return "SPX";
	case 2:
		return "DPX";
	case 4:
		return "QPX";
	case 8:
		return "CPX";
	default:
		return "UNKNOWN";
	}

	return "UNKNOWN";
}

static ssize_t gim_mon_spatial_partition_mode_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int ret;
	size_t count = 0;
	struct gim_dev_data *data;
	uint32_t spatial_partition_num;
	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	ret = amdgv_gpumon_get_spatial_partition_num(data->adev, &spatial_partition_num);
	if (ret) {
		count = sprintf(buf, "failed to get current sp mode\n");
	} else {
		count = sprintf(buf, "%s\n", gim_mon_get_spatial_partition_mode_desc(spatial_partition_num));
	}

	return count;
}

static DEVICE_ATTR(spatial_partition_mode, (0400),
			gim_mon_spatial_partition_mode_show, NULL);

static ssize_t gim_mon_accelerator_partition_profile_config_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int ret;
	size_t count = 0;
	int i, j, k;
	struct gim_dev_data *data;
	struct amdgv_gpumon_accelerator_partition_profile_config *accelerator_partition_profile_config;
	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	accelerator_partition_profile_config = kzalloc(sizeof(struct amdgv_gpumon_accelerator_partition_profile_config), GFP_KERNEL);
	if (accelerator_partition_profile_config == NULL) {
		count = sprintf(buf, "failed to allocate memory for cp profile\n");
		return count;
	}

	ret = amdgv_gpumon_get_accelerator_partition_profile_config(data->adev, accelerator_partition_profile_config);
	if (ret) {
		count = sprintf(buf, "failed to get cp profile\n");
	} else {
		count += sprintf(buf + count, "resource_profiles:\n");
		count += sprintf(buf + count, "%-20s%-20s%-20s%-20s\n",
				"resource_index", "resource_type", "resource_in_partition", "num_partitions_share_resource");
		for (i = 0; i < accelerator_partition_profile_config->number_of_resource_profiles; i++) {
			count += sprintf(buf + count, "%-20u%-20s%-20u%-20u\n",
				accelerator_partition_profile_config->resource_profiles[i].resource_index,
				gim_mon_get_resource_type_desc(accelerator_partition_profile_config->resource_profiles[i].resource_type),
				accelerator_partition_profile_config->resource_profiles[i].partition_resource,
				accelerator_partition_profile_config->resource_profiles[i].num_partitions_share_resource
				);
		}
		count += sprintf(buf + count, "profiles:\n");
		count += sprintf(buf + count, "%-20s%-20s%-20s%-20s%-20s%-20s\n",
				"profile_index", "profile_type", "num_of_partitions", "memory_caps", "num_of_resources", "resource_index");
		for (i = 0; i < accelerator_partition_profile_config->number_of_profiles; i++) {
			count += sprintf(buf + count, "%-20u%-20s%-20u%-20s%-20u[",
				accelerator_partition_profile_config->profiles[i].profile_index,
				gim_mon_get_profile_type_desc(accelerator_partition_profile_config->profiles[i].profile_type),
				accelerator_partition_profile_config->profiles[i].num_partitions,
				gim_mon_get_mem_desc(accelerator_partition_profile_config->profiles[i].memory_caps),
				accelerator_partition_profile_config->profiles[i].num_resources
				);
			for (j = 0; j < accelerator_partition_profile_config->profiles[i].num_partitions; j++) {
				count += sprintf(buf + count, "[");
				for (k = 0; k < accelerator_partition_profile_config->profiles[i].num_resources; k++) {
					count += sprintf(buf + count, "%u ",
						accelerator_partition_profile_config->profiles[i].resources[j][k]);
				}
				count += sprintf(buf + count, "]");
			}
			count += sprintf(buf + count, "]\n");
		}
	}

	kfree(accelerator_partition_profile_config);
	return count;
}

static DEVICE_ATTR(accelerator_partition_profile_config, (0400),
			gim_mon_accelerator_partition_profile_config_show, NULL);


static ssize_t gim_mon_memory_partition_config_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int ret;
	size_t count = 0;
	struct gim_dev_data *data;
	union amdgv_gpumon_memory_partition_config memory_partition_config;
	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	ret = amdgv_gpumon_get_memory_partition_config(data->adev, &memory_partition_config);
	if (ret) {
		count = sprintf(buf, "failed to get mp caps\n");
	} else {
		if (memory_partition_config.mp_caps.nps1_cap) {
			count += sprintf(buf + count, "%s ",
				gim_mon_get_memory_partition_mode_desc(AMDGV_MEMORY_PARTITION_MODE_NPS1));
		}
		if (memory_partition_config.mp_caps.nps2_cap) {
			count += sprintf(buf + count, "%s ",
				gim_mon_get_memory_partition_mode_desc(AMDGV_MEMORY_PARTITION_MODE_NPS2));
		}
		if (memory_partition_config.mp_caps.nps4_cap) {
			count += sprintf(buf + count, "%s ",
				gim_mon_get_memory_partition_mode_desc(AMDGV_MEMORY_PARTITION_MODE_NPS4));
		}
		if (memory_partition_config.mp_caps.nps8_cap) {
			count += sprintf(buf + count, "%s ",
				gim_mon_get_memory_partition_mode_desc(AMDGV_MEMORY_PARTITION_MODE_NPS8));
		}
		count += sprintf(buf + count, "\n");
	}

	return count;
}

static DEVICE_ATTR(memory_partition_config, (0400),
			gim_mon_memory_partition_config_show, NULL);

static ssize_t gim_mon_spatial_partition_caps_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int ret;
	size_t count = 0;
	struct gim_dev_data *data;
	struct amdgv_gpumon_spatial_partition_caps spatial_partition_caps;
	struct pci_dev *pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pdev);

	ret = amdgv_gpumon_get_spatial_partition_caps(data->adev, &spatial_partition_caps);
	if (ret) {
		count = sprintf(buf, "failed to get sp caps\n");
	} else {
		count = sprintf(buf, "num_xcc=%u num_sdma=%u num_vcn=%u num_jpeg=%u\n",
			spatial_partition_caps.num_xcc, spatial_partition_caps.num_sdma, spatial_partition_caps.num_vcn, spatial_partition_caps.num_jpeg);
	}

	return count;
}

static DEVICE_ATTR(spatial_partition_caps, (0400),
			gim_mon_spatial_partition_caps_show, NULL);

int gim_mon_create_dev_sys(struct gim_dev_data *data)
{
	int ret = 0;
	struct device *dev;
	union amdgv_dev_info dev_info;

	data->psp_vbflash_support = false;
	memset(&dev_info, 0, sizeof(dev_info));
	if (!amdgv_get_dev_info(data->adev, AMDGV_GET_PSP_VBFLASH_SUPPORT, &dev_info))
		data->psp_vbflash_support = dev_info.vbflash_support;
	dev = &data->pdev->dev;

	ret = device_create_file(dev, &dev_attr_self_switch);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_self_switch;
	}

	ret = device_create_file(dev, &dev_attr_reset_gpu);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_reset_gpu;
	}

	if (data->psp_vbflash_support) {
		ret = device_create_file(dev, &dev_attr_psp_vbflash_status);
		if (ret) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
			goto err_vbflash_status;
		}

		ret = sysfs_create_bin_file(&dev->kobj, &psp_vbflash_bin_attr);
		if (ret) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
			goto err_vbflash_bin;
		}
	}

	ret = device_create_file(dev, &dev_attr_accelerator_partition_profile);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_accelerator_partition_profile;
	}

	ret = device_create_file(dev, &dev_attr_memory_partition_mode);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_memory_partition_mode;
	}

	ret = device_create_file(dev, &dev_attr_spatial_partition_mode);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_spatial_partition_mode;
	}

	ret = device_create_file(dev, &dev_attr_accelerator_partition_profile_config);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_accelerator_partition_profile_config;
	}

	ret = device_create_file(dev, &dev_attr_memory_partition_config);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_memory_partition_config;
	}

	ret = device_create_file(dev, &dev_attr_spatial_partition_caps);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_spatial_partition_caps;
	}

	return ret;

err_spatial_partition_caps:
	device_remove_file(dev, &dev_attr_memory_partition_config);

err_memory_partition_config:
	device_remove_file(dev, &dev_attr_accelerator_partition_profile_config);

err_accelerator_partition_profile_config:
	device_remove_file(dev, &dev_attr_spatial_partition_mode);

err_spatial_partition_mode:
	device_remove_file(dev, &dev_attr_memory_partition_mode);

err_memory_partition_mode:
	device_remove_file(dev, &dev_attr_accelerator_partition_profile);

err_accelerator_partition_profile:
	sysfs_remove_bin_file(&dev->kobj, &psp_vbflash_bin_attr);

err_vbflash_bin:
	if (data->psp_vbflash_support) {
		device_remove_file(dev, &dev_attr_psp_vbflash_status);
		data->psp_vbflash_support = false;
	}

err_vbflash_status:
	device_remove_file(dev, &dev_attr_reset_gpu);

err_reset_gpu:
	device_remove_file(dev, &dev_attr_self_switch);

err_self_switch:

	return ret;
}

void gim_mon_remove_dev_sys(struct gim_dev_data *data)
{
	struct device *dev;

	dev = &data->pdev->dev;

	device_remove_file(dev, &dev_attr_self_switch);
	device_remove_file(dev, &dev_attr_reset_gpu);
	if (data->psp_vbflash_support) {
		device_remove_file(dev, &dev_attr_psp_vbflash_status);
		sysfs_remove_bin_file(&dev->kobj, &psp_vbflash_bin_attr);
		data->psp_vbflash_support = false;
	}
	device_remove_file(dev, &dev_attr_accelerator_partition_profile);
	device_remove_file(dev, &dev_attr_memory_partition_mode);
	device_remove_file(dev, &dev_attr_spatial_partition_mode);
	device_remove_file(dev, &dev_attr_accelerator_partition_profile_config);
	device_remove_file(dev, &dev_attr_memory_partition_config);
	device_remove_file(dev, &dev_attr_spatial_partition_caps);
}

static ssize_t gim_mon_self_switch_all_show(struct device_driver *drv,
						char *buf)
{
	size_t count;
	struct gim_dev_data *data;
	union amdgv_dev_conf conf;

	count = 0;
	list_for_each_entry(data, &gim_device_list, list) {
		amdgv_get_dev_conf(data->adev,
				AMDGV_CONF_DISABLE_SELF_SWITCH_FLAG,
				&conf);

		if (conf.flag_switch == 0)
			count += sprintf(buf + count, "Adapter[%s] : 1\n",
				dev_name(&data->pdev->dev));
		else
			count += sprintf(buf + count, "Adapter[%s] : 0\n",
				dev_name(&data->pdev->dev));
	}

	return count;
}

static ssize_t gim_mon_self_switch_all_store(struct device_driver *drv,
					const char *buf,
					size_t count)
{
	uint32_t self_switch;
	union amdgv_dev_conf conf;
	struct gim_dev_data *data;
	int ret = 0;

	if (sscanf(buf, "%u", &self_switch) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (self_switch == 0)
		conf.flag_switch = 1;
	else
		conf.flag_switch = 0;

	list_for_each_entry(data, &gim_device_list, list) {
		ret = amdgv_set_dev_conf(data->adev,
				AMDGV_CONF_DISABLE_SELF_SWITCH_FLAG,
				&conf);
		if (ret) {
			pr_warn("Failed to set self switch for [%s]\n",
				dev_name(&data->pdev->dev));
		}
	}

	return count;
}

static struct driver_attribute driver_attr_self_switch =
	__ATTR(self_switch, (0600),
		gim_mon_self_switch_all_show,
		gim_mon_self_switch_all_store);

static ssize_t gim_mon_force_reset_all_show(struct device_driver *drv,
						char *buf)
{
	size_t count;
	struct gim_dev_data *data;
	union amdgv_dev_conf conf;

	count = 0;
	list_for_each_entry(data, &gim_device_list, list) {
		amdgv_get_dev_conf(data->adev,
				AMDGV_CONF_FORCE_RESET_FLAG,
				&conf);

		count += sprintf(buf + count, "Adapter[%s] : %d\n",
			dev_name(&data->pdev->dev), conf.flag_switch);
	}

	return count;
}

static ssize_t gim_mon_force_reset_all_store(struct device_driver *drv,
					const char *buf,
					size_t count)
{
	int force_reset = 1;
	union amdgv_dev_conf conf;
	struct gim_dev_data *data;

	if (sscanf(buf, "%d", &force_reset) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	conf.flag_switch = force_reset;
	list_for_each_entry(data, &gim_device_list, list) {
		amdgv_set_dev_conf(data->adev,
				AMDGV_CONF_FORCE_RESET_FLAG,
				&conf);
	}

	return count;
}

static struct driver_attribute driver_attr_force_reset =
	__ATTR(force_hot_reset, (0600),
		gim_mon_force_reset_all_show,
		gim_mon_force_reset_all_store);
static ssize_t gim_mon_live_update_all_show(struct device_driver *drv,
						char *buf)
{
	size_t count;
	struct gim_dev_data *data;

	count = 0;
	list_for_each_entry(data, &gim_device_list, list) {
		count += sprintf(buf + count, "Adapter[%s] live_update: %s\n",
		dev_name(&data->pdev->dev), update_mgr.is_export ? "enabled" : "disabled");
	}

	return count;
}

static ssize_t gim_mon_live_update_all_store(struct device_driver *drv,
					const char *buf,
					size_t count)
{
	uint32_t live_update;
	struct gim_dev_data *data;
	void *data_ptr;

	if (sscanf(buf, "%u", &live_update) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (update_mgr.update_type == GIM_LIVE_UPDATE_DISABLED) {
		pr_warn("live update cannot be used\n");
		return -EINVAL;
	}

	if (live_update != 1)
		return count;

	list_for_each_entry(data, &gim_device_list, list) {
		data_ptr = (void *)gim_live_update_get_data_ptr(&update_mgr, data->gpu_index);
		data->init_data.sys_mem_info.va_ptr = data_ptr;
		amdgv_set_sysmem_va_ptr(data->adev, data_ptr);

		data->init_data.fini_opt.skip_hw_fini = true;
	}
	update_mgr.is_export = true;

	return count;
}

static struct driver_attribute driver_attr_live_update =
	__ATTR(live_update, (0600),
		gim_mon_live_update_all_show,
		gim_mon_live_update_all_store);
int gim_mon_create_drv_sys(struct device_driver *drv)
{
	int ret = 0;

	ret = driver_create_file(drv, &driver_attr_self_switch);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DRIVER_FILE_FAIL, 0);
		goto err_self_switch;
	}

	ret = driver_create_file(drv, &driver_attr_force_reset);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DRIVER_FILE_FAIL, 0);
		goto err_force_reset;
	}
	ret = driver_create_file(drv, &driver_attr_live_update);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DRIVER_FILE_FAIL, 0);
		goto err_live_update;
	}
	return ret;

err_live_update:
	driver_remove_file(drv, &driver_attr_force_reset);

err_force_reset:
	driver_remove_file(drv, &driver_attr_self_switch);

err_self_switch:
	return ret;
}

void gim_mon_remove_drv_sys(struct device_driver *drv)
{

	driver_remove_file(drv, &driver_attr_self_switch);
	driver_remove_file(drv, &driver_attr_force_reset);
	driver_remove_file(drv, &driver_attr_live_update);
}
