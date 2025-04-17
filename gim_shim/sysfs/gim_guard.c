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
#include <amdgv_api.h>

#include "gim_debug.h"
#include "gim_config.h"
#include "gim.h"

#include "gim_guard.h"

extern struct gim_error_ring_buffer *gim_error_rb;

static ssize_t gim_guard_adapt_guard_status_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int idx_vf;
	int type;
	struct pci_dev *pf_pdev;
	struct pci_dev *vf_pdev;
	struct gim_dev_data *data;
	struct amdgv_guard_info info;

	int ret = 0;

	pf_pdev = to_pci_dev(dev);

	ret += sprintf(buf + ret,
			"VF[bdf]\t\t\tflr\tex\text\tint\n");

	data = pci_get_drvdata(pf_pdev);
	for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
		vf_pdev = data->vf_map[idx_vf].pdev;

		ret += sprintf(buf + ret, "VF[%s]:",
				dev_name(&vf_pdev->dev));

		for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
			info.type = type;
			amdgv_get_guard_info(data->adev, idx_vf, &info);
			ret += sprintf(buf + ret, "\t[%u:%u:%u]",
					info.parm.event.state,
					info.parm.event.active,
					info.parm.event.amount);
		}
		ret += sprintf(buf + ret, "\n");
	}

	ret += sprintf(buf + ret,
			"Notes:\n1)flr: function level reset;\n"
			"2)ex: exclusive access mode\n"
			"3)ext: exclusive access timeout\n"
			"4)int: interrupt\n"
			"5)event status data format: [state:valid:total]\n"
			"6)state: 0->normal; 1->full; 2->overflower\n"
			"7)valid: event number in the latest interval\n"
			"8)total: total event number\n");

	return ret;
}

static DEVICE_ATTR(guard_status, S_IRUGO,
		gim_guard_adapt_guard_status_show, NULL);

static ssize_t gim_guard_adapt_threshold_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int idx_vf;
	int type;
	struct pci_dev *pf_pdev;
	struct pci_dev *vf_pdev;
	struct gim_dev_data *data;
	struct amdgv_guard_info info;

	int ret = 0;

	pf_pdev = to_pci_dev(dev);

	ret += sprintf(buf + ret,
			"VF[bdf]\t\t\tflr\tex\text\tint\n");

	data = pci_get_drvdata(pf_pdev);
	for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
		vf_pdev = data->vf_map[idx_vf].pdev;

		ret += sprintf(buf + ret, "VF[%s]:",
				dev_name(&vf_pdev->dev));

		for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
			info.type = type;
			amdgv_get_guard_info(data->adev, idx_vf, &info);
			ret += sprintf(buf + ret, "\t[%u]",
					info.parm.event.threshold);
		}
		ret += sprintf(buf + ret, "\n");
	}

	return ret;
}

static ssize_t gim_guard_adapt_threshold_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int ret;
	int idx_vf;
	int type;
	struct pci_dev *pf_pdev;
	struct gim_dev_data *data;
	struct amdgv_guard_info info;
	uint32_t threshold[AMDGV_GUARD_EVENT_MAX];

	ret = sscanf(buf, "flr=%x,ex=%x,ext=%x,int=%x",
		&threshold[AMDGV_GUARD_EVENT_FLR],
		&threshold[AMDGV_GUARD_EVENT_EXCLUSIVE_MOD],
		&threshold[AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT],
		&threshold[AMDGV_GUARD_EVENT_ALL_INT]);

	if (ret != 4) {
		pr_warn("invalid parameter\n");
		return count;
	}

	pf_pdev = to_pci_dev(dev);

	data = pci_get_drvdata(pf_pdev);
	for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
		for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
			info.type = type;
			amdgv_get_guard_info(data->adev, idx_vf, &info);

			info.parm.event.threshold = threshold[type];
			amdgv_set_guard_config(data->adev, idx_vf, &info);
		}
	}

	return count;
}

static DEVICE_ATTR(guard_threshold, S_IRUGO | S_IWUSR,
		gim_guard_adapt_threshold_show,
		gim_guard_adapt_threshold_store);

int gim_guard_init_dev_sys(struct pci_dev *pdev)
{
	int ret;

	ret = device_create_file(&pdev->dev, &dev_attr_guard_status);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_guard_status;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_guard_threshold);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_guard_threshold;
	}

	return ret;

err_guard_threshold:
	device_remove_file(&pdev->dev, &dev_attr_guard_status);

err_guard_status:

	return ret;
}

int gim_guard_remove_dev_sys(struct pci_dev *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_guard_threshold);
	device_remove_file(&pdev->dev, &dev_attr_guard_status);

	return 0;
}

static ssize_t gim_guard_platform_status_show(struct device_driver *drv, char *buf)
{
	int idx_vf;
	int type;
	struct pci_dev *pf_pdev;
	struct pci_dev *vf_pdev;
	struct gim_dev_data *data;
	struct amdgv_guard_info info;

	int ret = 0;

	list_for_each_entry(data, &gim_device_list, list) {
		pf_pdev = data->pdev;
		ret += snprintf(buf + ret, PAGE_SIZE,
				"Adapter[%s]\n",
				dev_name(&pf_pdev->dev));

		ret += sprintf(buf + ret,
				"VF[bdf]\t\t\tflr\tex\text\tint\n");

		for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
			vf_pdev = data->vf_map[idx_vf].pdev;

			ret += sprintf(buf + ret, "VF[%s]:",
					dev_name(&vf_pdev->dev));

			for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
				info.type = type;
				amdgv_get_guard_info(data->adev, idx_vf, &info);
				ret += sprintf(buf + ret, "\t[%u:%u:%u]",
						info.parm.event.state,
						info.parm.event.active,
						info.parm.event.amount);
			}
			ret += sprintf(buf + ret, "\n");
		}

	}

	ret += sprintf(buf + ret,
			"Notes:\n1)flr: function level reset;\n"
			"2)ex: exclusive access mode\n"
			"3)ext: exclusive access timeout\n"
			"4)int: interrupt\n"
			"5)event status data format: [state:valid:total]\n"
			"6)state: 0->normal; 1->full; 2->overflower\n"
			"7)valid: valid event number in the interval\n"
			"8)total: total served event number\n");

	return ret;
}

static struct driver_attribute driver_attr_guard_status =
						__ATTR(guard_status,
						S_IRUGO,
						gim_guard_platform_status_show,
						NULL);

static ssize_t gim_guard_platform_threshold_show(struct device_driver *drv,
						char *buf)
{
	int idx_vf;
	int type;
	struct pci_dev *pf_pdev;
	struct pci_dev *vf_pdev;
	struct gim_dev_data *data;
	struct amdgv_guard_info info;

	int ret = 0;

	list_for_each_entry(data, &gim_device_list, list) {
		pf_pdev = data->pdev;

		ret += sprintf(buf + ret,
				"Adapter[%s]\n",
				dev_name(&pf_pdev->dev));

		ret += sprintf(buf + ret,
				"VF[bdf]\t\t\tflr\tex\text\tint\n");

		for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
			vf_pdev = data->vf_map[idx_vf].pdev;

			ret += sprintf(buf + ret, "VF[%s]:",
					dev_name(&vf_pdev->dev));

			for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
				info.type = type;
				amdgv_get_guard_info(data->adev, idx_vf, &info);
				ret += sprintf(buf + ret, "\t[%u]",
						info.parm.event.threshold);
			}
			ret += sprintf(buf + ret, "\n");
		}
	}

	return ret;


}

static ssize_t gim_guard_platform_threshold_store(struct device_driver *drv,
					const char *buf,
					size_t count)
{
	int ret;
	int idx_vf;
	int type;
	struct gim_dev_data *data;
	struct amdgv_guard_info info;
	uint32_t threshold[AMDGV_GUARD_EVENT_MAX];

	ret = sscanf(buf, "flr=%x,ex=%x,ext=%x,int=%x",
		&threshold[AMDGV_GUARD_EVENT_FLR],
		&threshold[AMDGV_GUARD_EVENT_EXCLUSIVE_MOD],
		&threshold[AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT],
		&threshold[AMDGV_GUARD_EVENT_ALL_INT]);

	if (ret != 4) {
		pr_warn("invalid parameter\n");
		return count;
	}

	list_for_each_entry(data, &gim_device_list, list) {
		for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
			for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
				info.type = type;
				amdgv_get_guard_info(data->adev, idx_vf, &info);

				info.parm.event.threshold = threshold[type];
				amdgv_set_guard_config(data->adev, idx_vf,
							&info);
			}
		}
	}

	return ret;
}

static struct driver_attribute driver_attr_guard_threshold =
				__ATTR(guard_threshold,
					S_IRUGO | S_IWUSR,
					gim_guard_platform_threshold_show,
					gim_guard_platform_threshold_store);



int gim_guard_init_drv_sys(struct device_driver *drv)
{
	int ret;

	ret = driver_create_file(drv, &driver_attr_guard_status);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DRIVER_FILE_FAIL, 0);
		goto err_guard_status;
	}

	ret = driver_create_file(drv, &driver_attr_guard_threshold);
	if (ret) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DRIVER_FILE_FAIL, 0);
		goto err_guard_threshold;
	}

	return ret;

err_guard_threshold:
	driver_remove_file(drv, &driver_attr_guard_status);

err_guard_status:

	return ret;
}

int gim_guard_remove_drv_sys(struct device_driver *drv)
{
	driver_remove_file(drv, &driver_attr_guard_status);
	driver_remove_file(drv, &driver_attr_guard_threshold);

	return 0;
}
