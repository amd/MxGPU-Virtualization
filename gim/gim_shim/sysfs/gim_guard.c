/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/device.h>
#include <amdgv_api.h>

#include "gim_debug.h"
#include "gim_config.h"
#include "gim.h"

#include "gim_sysfs_emit.h"
#include "gim_guard.h"

extern struct gim_error_ring_buffer *gim_error_rb;

struct gim_guard_threshold_entry {
	const char *name;
	uint32_t event_id;
};

static const struct gim_guard_threshold_entry gim_guard_threshold_map[] = {
	{ "flr",     AMDGV_GUARD_EVENT_FLR },
	{ "ex",      AMDGV_GUARD_EVENT_EXCLUSIVE_MOD },
	{ "ext",     AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT },
	{ "int",     AMDGV_GUARD_EVENT_ALL_INT },
	{ "ras_err", AMDGV_GUARD_EVENT_RAS_ERR_COUNT },
	{ "ras_cper", AMDGV_GUARD_EVENT_RAS_CPER_DUMP },
	{ "ras_bp",  AMDGV_GUARD_EVENT_RAS_BAD_PAGES },
	{ "wgr",     AMDGV_GUARD_EVENT_WGR },
	{ "ras_chk", AMDGV_GUARD_EVENT_RAS_CHK_CRITI },
};

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

	ssize_t size = 0;

	pf_pdev = to_pci_dev(dev);

	size += gim_sysfs_emit_at(buf, size,
			"VF[bdf]\t\t\tflr\tex\text\tint\n");

	data = pci_get_drvdata(pf_pdev);
	for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
		vf_pdev = data->vf_map[idx_vf].pdev;

		size += gim_sysfs_emit_at(buf, size, "VF[%s]:",
				dev_name(&vf_pdev->dev));

		for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
			info.type = type;
			amdgv_get_guard_info(data->adev, idx_vf, &info);
			size += gim_sysfs_emit_at(buf, size, "\t[%u:%u:%u]",
					info.parm.event.state,
					info.parm.event.active,
					info.parm.event.amount);
		}
		size += gim_sysfs_emit_at(buf, size, "\n");
	}

	size += gim_sysfs_emit_at(buf, size,
			"Notes:\n1)flr: function level reset;\n"
			"2)ex: exclusive access mode\n"
			"3)ext: exclusive access timeout\n"
			"4)int: interrupt\n"
			"5)event status data format: [state:valid:total]\n"
			"6)state: 0->normal; 1->full; 2->overflower\n"
			"7)valid: event number in the latest interval\n"
			"8)total: total event number\n");

	return size;
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

	ssize_t size = 0;

	pf_pdev = to_pci_dev(dev);

	size += gim_sysfs_emit_at(buf, size,
			"VF[bdf]\t\t\tflr\tex\text\tint\n");

	data = pci_get_drvdata(pf_pdev);
	for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
		vf_pdev = data->vf_map[idx_vf].pdev;

		size += gim_sysfs_emit_at(buf, size, "VF[%s]:",
				dev_name(&vf_pdev->dev));

		for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
			info.type = type;
			amdgv_get_guard_info(data->adev, idx_vf, &info);
			size += gim_sysfs_emit_at(buf, size, "\t[%u]",
					info.parm.event.threshold);
		}
		size += gim_sysfs_emit_at(buf, size, "\n");
	}

	return size;
}

static int gim_guard_parse_thresholds(const char *buf,
				      size_t count,
				      uint32_t threshold[AMDGV_GUARD_EVENT_MAX],
				      bool threshold_set[AMDGV_GUARD_EVENT_MAX])
{
	char *tmp;
	char *p;
	char *token;
	bool any = false;
	int ret = 0;

	if (!buf || count == 0)
		return -EINVAL;

	tmp = kmemdup_nul(buf, count, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	p = strchr(tmp, '\n');
	if (p)
		*p = '\0';

	p = tmp;
	while ((token = strsep(&p, ",")) != NULL) {
		char *eq;
		const char *key;
		const char *val_str;
		unsigned int value;
		bool matched = false;
		size_t i;

		if (!token[0]) {
			ret = -EINVAL;
			goto out;
		}

		eq = strchr(token, '=');
		if (!eq || eq == token || !eq[1]) {
			ret = -EINVAL;
			goto out;
		}

		*eq = '\0';
		key = token;
		val_str = eq + 1;

		ret = kstrtouint(val_str, 0, &value);
		if (ret) {
			ret = -EINVAL;
			goto out;
		}

		for (i = 0; i < ARRAY_SIZE(gim_guard_threshold_map); i++) {
			if (strcmp(key, gim_guard_threshold_map[i].name) == 0) {
				uint32_t event_id = gim_guard_threshold_map[i].event_id;
				threshold[event_id] = (uint32_t)value;
				threshold_set[event_id] = true;
				matched = true;
				break;
			}
		}

		if (!matched) {
			ret = -EINVAL;
			goto out;
		}

		any = true;
	}

	ret = any ? 0 : -EINVAL;

out:
	kfree(tmp);
	return ret;
}

static ssize_t gim_guard_adapt_threshold_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int idx_vf;
	int type;
	int ret;
	struct pci_dev *pf_pdev;
	struct gim_dev_data *data;
	struct amdgv_guard_info info;
	uint32_t threshold[AMDGV_GUARD_EVENT_MAX] = {0};
	bool threshold_set[AMDGV_GUARD_EVENT_MAX] = {false};

	ret = gim_guard_parse_thresholds(buf, count, threshold, threshold_set);
	if (ret)
		return ret;

	pf_pdev = to_pci_dev(dev);
	data = pci_get_drvdata(pf_pdev);

	for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
		for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
			if (!threshold_set[type])
				continue;

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
		gim_put_error(AMDGV_LOG_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
		goto err_guard_status;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_guard_threshold);
	if (ret) {
		gim_put_error(AMDGV_LOG_DRIVER_CREATE_DEVICE_FILE_FAIL, 0);
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

	ssize_t size = 0;

	/* Add warning about 4KB sysfs limitation */
	size += gim_sysfs_emit_at(buf, size,
			"WARNING: The driver sysfs may show incomplete guard information. \n "
			"Please use each device sysfs instead.\n "
			"  cat /sys/bus/pci/drivers/gim/<PF_DBDF>/guard_status\n\n");

	list_for_each_entry(data, &gim_device_list, list) {
		pf_pdev = data->pdev;
		size += gim_sysfs_emit_at(buf, size,
				"Adapter[%s]\n",
				dev_name(&pf_pdev->dev));

		size += gim_sysfs_emit_at(buf, size,
				"VF[bdf]\t\t\tflr\tex\text\tint\n");

		for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
			vf_pdev = data->vf_map[idx_vf].pdev;

			size += gim_sysfs_emit_at(buf, size, "VF[%s]:",
					dev_name(&vf_pdev->dev));

			for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
				info.type = type;
				amdgv_get_guard_info(data->adev, idx_vf, &info);
				size += gim_sysfs_emit_at(buf, size, "\t[%u:%u:%u]",
						info.parm.event.state,
						info.parm.event.active,
						info.parm.event.amount);
			}
			size += gim_sysfs_emit_at(buf, size, "\n");
		}

	}

	size += gim_sysfs_emit_at(buf, size,
			"Notes:\n1)flr: function level reset;\n"
			"2)ex: exclusive access mode\n"
			"3)ext: exclusive access timeout\n"
			"4)int: interrupt\n"
			"5)event status data format: [state:valid:total]\n"
			"6)state: 0->normal; 1->full; 2->overflower\n"
			"7)valid: valid event number in the interval\n"
			"8)total: total served event number\n");

	return size;
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

	ssize_t size = 0;

	/* Add warning about 4KB sysfs limitation */
	size += gim_sysfs_emit_at(buf, size,
			"WARNING: The driver sysfs may show incomplete guard information. \n "
			"Please use each device sysfs instead.\n "
			"  cat /sys/bus/pci/drivers/gim/<PF_DBDF>/guard_threshold\n\n");

	list_for_each_entry(data, &gim_device_list, list) {
		pf_pdev = data->pdev;

		size += gim_sysfs_emit_at(buf, size,
				"Adapter[%s]\n",
				dev_name(&pf_pdev->dev));

		size += gim_sysfs_emit_at(buf, size,
				"VF[bdf]\t\t\tflr\tex\text\tint\n");

		for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
			vf_pdev = data->vf_map[idx_vf].pdev;

			size += gim_sysfs_emit_at(buf, size, "VF[%s]:",
					dev_name(&vf_pdev->dev));

			for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
				info.type = type;
				amdgv_get_guard_info(data->adev, idx_vf, &info);
				size += gim_sysfs_emit_at(buf, size, "\t[%u]",
						info.parm.event.threshold);
			}
			size += gim_sysfs_emit_at(buf, size, "\n");
		}
	}

	return size;
}

static ssize_t gim_guard_platform_threshold_store(struct device_driver *drv,
					const char *buf,
					size_t count)
{
	int idx_vf;
	int type;
	int ret;
	struct gim_dev_data *data;
	struct amdgv_guard_info info;
	uint32_t threshold[AMDGV_GUARD_EVENT_MAX] = {0};
	bool threshold_set[AMDGV_GUARD_EVENT_MAX] = {false};

	ret = gim_guard_parse_thresholds(buf, count, threshold, threshold_set);
	if (ret)
		return ret;

	list_for_each_entry(data, &gim_device_list, list) {
		for (idx_vf = 0; idx_vf < data->vf_num; idx_vf++) {
			for (type = 0; type < AMDGV_GUARD_EVENT_MAX; type++) {
				if (!threshold_set[type])
					continue;

				info.type = type;
				amdgv_get_guard_info(data->adev, idx_vf, &info);
				info.parm.event.threshold = threshold[type];
				amdgv_set_guard_config(data->adev, idx_vf, &info);
			}
		}
	}

	return count;
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
		gim_put_error(AMDGV_LOG_DRIVER_CREATE_DRIVER_FILE_FAIL, 0);
		goto err_guard_status;
	}

	ret = driver_create_file(drv, &driver_attr_guard_threshold);
	if (ret) {
		gim_put_error(AMDGV_LOG_DRIVER_CREATE_DRIVER_FILE_FAIL, 0);
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
