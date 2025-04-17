/*
 * Copyright (c) 2018-2019 Advanced Micro Devices, Inc. All rights reserved.
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
#include <linux/debugfs.h>
#include <linux/uaccess.h>

#include "gim_debug.h"
#include "gim.h"
#include "gim_debugfs.h"

extern struct gim_error_ring_buffer *gim_error_rb;

static struct dentry *root_dir;

typedef struct amdgv_ffbm_permission {
	char *permission;
	int val;
} amdgv_ffbm_permission;

struct amdgv_ffbm_permission permissions[] = {
	{"r", 1},
	{"w", 2},
	{"rw", 3},
	{NULL, -1}
};

static int attr_rlcv_timestamp_dump_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	if (val == 0)
		conf.flag_switch = 1;
	else
		conf.flag_switch = 0;
	ret = amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_SELF_SWITCH_FLAG,
					&conf);
	if (!ret)
		ret = amdgv_set_rlcv_timestamp_dump(dev_data->adev, val);

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(rlcv_timestamp_dump_fops, NULL,
		attr_rlcv_timestamp_dump_set, "%llu\n");

static int ffbm_permission_get(char *permission)
{
	int i = 0;
	char *per = permissions[i].permission;
	while (per) {
		if (strcmp(per, permission) == 0)
			return permissions[i].val;
		per = permissions[++i].permission;
	}
	return -1;
}

static int attr_force_reset_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	conf.flag_switch = (u32)val;
	ret = amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_FORCE_RESET_FLAG,
					&conf);
	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(force_reset_fops, NULL,
				attr_force_reset_set, "%llu\n");

static int attr_skip_page_retirement_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	if (val == 0)
		conf.flag_switch = 0;
	else
		conf.flag_switch = 1;

	ret = amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_SKIP_PAGE_RETIREMENT,
					&conf);
	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(skip_page_retirement_fops, NULL,
				attr_skip_page_retirement_set, "%llu\n");

static int attr_hang_debug_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	if (val == 0)
		conf.flag_switch = 0;
	else if (val == 1)
		conf.flag_switch = 1;
	else if (val == 2)
		conf.flag_switch = 2;
	else
		return 0;

	ret = amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_HANG_DEBUG_FLAG,
					&conf);
	return ret;
}

static int attr_hang_debug_get(void *data, u64 *val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	ret = amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_HANG_DEBUG_FLAG,
					&conf);

	if (conf.flag_switch == 1)
		*val = 1;
	else if (conf.flag_switch == 2)
		*val = 2;
	else
		*val = 0;

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(hang_debug_fops, attr_hang_debug_get,
				attr_hang_debug_set, "%llu\n");
#ifdef WS_RECORD
static int attr_ws_record_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	if (val == 0)
		conf.flag_switch = 0;
	else if (val == 1)
		conf.flag_switch = 1;
	else
		return 0;

	ret = amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_WS_RECORD,
					&conf);

	return ret;
}

static int attr_ws_record_get(void *data, u64 *val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	ret = amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_WS_RECORD,
					&conf);
	if (conf.flag_switch == true)
		*val = 1;
	else
		*val = 0;

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(ws_record_fops, attr_ws_record_get,
				attr_ws_record_set, "%llu\n");
#endif
static int attr_mm_quanta_option(void *data, u64 val)
{
	int ret = 0;
	enum amdgv_sched_block sched_id;
	struct gim_dev_data *dev_data;
	u32 opt;

	dev_data = (struct gim_dev_data *)data;
	opt = (uint32_t)val;

	for (sched_id = AMDGV_SCHED_BLOCK_UVD;
		sched_id < AMDGV_SCHED_BLOCK_MAX; sched_id++)
		ret = amdgv_set_time_quanta_option(dev_data->adev,
						sched_id, opt);

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(mm_quanta_option, NULL,
			attr_mm_quanta_option, "%llu\n");

static int attr_switch_vf_debug_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	conf.switch_vf_idx = (uint32_t)val;
	ret = amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_FORCE_SWITCH_VF_FLAG,
					&conf);
	return ret;
}

static int attr_switch_vf_debug_get(void *data, u64 *val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	ret = amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_FORCE_SWITCH_VF_FLAG,
					&conf);

	*val = conf.switch_vf_idx;

	return ret;
}


DEFINE_SIMPLE_ATTRIBUTE(force_switch_vf_debug_fops, attr_switch_vf_debug_get,
				attr_switch_vf_debug_set, "%llu\n");

static int attr_disable_mmio_protection_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	if (val == 0)
		conf.flag_switch = 0;
	else
		conf.flag_switch = 1;

	ret = amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_MMIO_PROTECTION,
					&conf);
	return ret;
}

static int attr_disable_mmio_protection_get(void *data, u64 *val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	ret = amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_MMIO_PROTECTION,
					&conf);

	if (conf.flag_switch == 0)
		*val = 0;
	else
		*val = 1;

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(disable_mmio_protection_fops, attr_disable_mmio_protection_get,
				attr_disable_mmio_protection_set, "%llu\n");

static int attr_enable_access_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	ret = 0;
	dev_data = (struct gim_dev_data *)data;
	conf.flag_switch = 1;

	ret |= amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_MMIO_PROTECTION,
					&conf);

	ret |= amdgv_toggle_mmio_access(dev_data->adev, (u32)val,
					AMDGV_VF_ACCESS_MMIO_REG_WRITE | AMDGV_VF_ACCESS_FB |
					AMDGV_VF_ACCESS_DOORBELL, true);

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(enable_access_fops, NULL,
				attr_enable_access_set, "%llu\n");

static int attr_disable_psp_vf_gate_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	ret = 0;
	dev_data = (struct gim_dev_data *)data;

	if (val == 0) {
		conf.flag_switch = 0;
		ret |= amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_PSP_VF_GATE,
					&conf);
		ret |= amdgv_toggle_psp_vf_gate(dev_data->adev, (u32)0xFFFFFFFF, true);
	} else {
		conf.flag_switch = 1;
		ret |= amdgv_toggle_psp_vf_gate(dev_data->adev, (u32)0xFFFFFFFF, false);
		ret |= amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_PSP_VF_GATE,
					&conf);
	}

	return ret;
}

static int attr_disable_psp_vf_gate_get(void *data, u64 *val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	ret = amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_PSP_VF_GATE,
					&conf);

	if (conf.flag_switch == 0)
		*val = 0;
	else
		*val = 1;

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(disable_psp_vf_gate_fops, attr_disable_psp_vf_gate_get,
				attr_disable_psp_vf_gate_set, "%llu\n");


static int attr_hliquid_min_ts_get(void *data, u64 *val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	ret = 0;
	dev_data = (struct gim_dev_data *)data;

	ret = amdgv_get_dev_conf(dev_data->adev,
			AMDGV_CONF_HYBRID_LIQUID_VF_MIN_TS,
			&conf);

	*val = conf.hliquid_min_ts;

	return ret;
}

static int attr_hliquid_min_ts_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	ret = 0;
	dev_data = (struct gim_dev_data *)data;
	conf.hliquid_min_ts = (uint32_t)val;

	ret = amdgv_set_dev_conf(dev_data->adev,
			AMDGV_CONF_HYBRID_LIQUID_VF_MIN_TS,
			&conf);

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(hliquid_min_ts_fops, attr_hliquid_min_ts_get,
				attr_hliquid_min_ts_set, "%llu\n");

static int attr_disable_dcore_debug_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	if (val == 0)
		conf.flag_switch = 0;
	else
		conf.flag_switch = 1;

	ret = amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_DCORE_DEBUG,
					&conf);
	return ret;
}

static int attr_disable_dcore_debug_get(void *data, u64 *val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	ret = amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_DCORE_DEBUG,
					&conf);
	if (conf.flag_switch == 0)
		*val = 0;
	else
		*val = 1;

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(disable_dcore_debug_fops, attr_disable_dcore_debug_get,
				attr_disable_dcore_debug_set, "%llu\n");

static void *realloc_memory(void *ptr, size_t oldsize, size_t newsize)
{
	char *ptr_tmp = NULL;

	ptr_tmp = gim_oss_interfaces.alloc_memory(newsize);
	memcpy(ptr_tmp, ptr, oldsize);
	gim_oss_interfaces.free_memory(ptr);

	return ptr_tmp;
}

static ssize_t snprintf_realloc(char **base_buf, size_t *buf_size, size_t used_size, const char *fmt, ...)
{
	int num = 0;
	va_list args;

	while (1) {
		va_start(args, fmt);
		num = gim_oss_interfaces.vsnprintf(*base_buf + used_size, *buf_size - used_size, fmt, args);
		va_end(args);
		if (used_size + num >= *buf_size) {
			if (used_size + num >= 40960) {
				pr_warn("buffer needed is bigger than the limited 40960 bytes, suspect something wrong happens\n");
				return 0;
			}
			*base_buf = realloc_memory(*base_buf, *buf_size, *buf_size + 512);
			*buf_size += 512;
		} else
			break;
	}

	return num;
}

static ssize_t asymmetric_timeslice_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	char buf[64];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;
	uint32_t vf_idx, timeslice;
	bool reset = false;

	dev_data = file->private_data;

	if (count > 64)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d,%d", &vf_idx, &timeslice) == 2) {

		if ((vf_idx > AMDGV_PF_IDX) || (vf_idx < 0)) {
			pr_warn("vf_idx must be within 0 to 31(PF_IDX)\n");
			return -EINVAL;
		}

		if ((timeslice < 100) || (timeslice > 1000000) || ((timeslice % 100) != 0)) {
			pr_warn("timeslice must be within 100us to 1s and align with 100us!\n");
			return -EINVAL;
		}

	} else if ((sscanf(buf, "%d", &vf_idx) == 1) && (vf_idx == 0)) {
		pr_warn("reset to default timeslice\n");
		reset = true;
		goto setup;
	} else {
		pr_warn("invalid parameter: the format should be 0 or \"vf_idx, timeslice\"\n");
		return -EINVAL;
	}

	conf.asymmetric.vf_idx = vf_idx;
	conf.asymmetric.vf_timeslice = timeslice;
setup:
	conf.asymmetric.reset = reset;

	amdgv_set_dev_conf(dev_data->adev,
				AMDGV_CONF_ASYMMETRIC_TIMESLICE_FLAG,
				&conf);

	return count;
}

static ssize_t asymmetric_timeslice_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	uint32_t i;
	uint32_t asymmetric_timeslice_vfs[AMDGV_MAX_VF_NUM];
	bool is_active_vfs[AMDGV_MAX_VF_NUM];
	size_t buf_resv_size = 4096;
	char *buf = gim_oss_interfaces.alloc_memory(buf_resv_size);
	struct gim_dev_data *dev_data;

	dev_data = file->private_data;
	len = 0;
	len += snprintf_realloc(&buf, &buf_resv_size, len, "Currnet Available VF Number:\t%d\t[value is vf_num]\n", dev_data->vf_num);
	len += snprintf_realloc(&buf, &buf_resv_size, len, "\nStatus:\n");
	for (i = 0; i < dev_data->vf_num; i++) {
		ret = amdgv_dump_asymmetric_timeslice(dev_data->adev, i, &is_active_vfs[i], &asymmetric_timeslice_vfs[i]);
		if (ret) {
			len += snprintf_realloc(&buf, &buf_resv_size, len, "\nAsics not support asymmetric time slice setting!\n");
			break;
		}
		len += snprintf_realloc(&buf, &buf_resv_size, len, "VF%d\t %s\t timeslice = %d us\n", i, is_active_vfs[i] ? "Active" : "Inactive",
						asymmetric_timeslice_vfs[i]);
	}
	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	gim_oss_interfaces.free_memory(buf);
	return ret;
}

static const struct file_operations asymmetric_timeslice_fops = {
	.open           = simple_open,
	.read           = asymmetric_timeslice_read,
	.write          = asymmetric_timeslice_write,
	.llseek         = default_llseek,
};

static ssize_t force_switch_vf_debug_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	uint32_t val;
	char buf[16];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%x", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	conf.switch_vf_idx = val;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_FORCE_SWITCH_VF_FLAG,
					&conf);
	}

	return count;
}

static ssize_t force_switch_vf_debug_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_FORCE_SWITCH_VF_FLAG,
					&conf);
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				conf.switch_vf_idx);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations force_switch_vf_debug_all_fops = {
	.open           = simple_open,
	.read           = force_switch_vf_debug_all_read,
	.write          = force_switch_vf_debug_all_write,
	.llseek         = default_llseek,
};


static int attr_force_reset_all_set(void *data, u64 val)
{
	int ret;
	union amdgv_dev_conf conf;
	struct gim_dev_data *dev_data;

	ret = 0;

	conf.flag_switch = (u32)val;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		ret |= amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_FORCE_RESET_FLAG,
					&conf);
	}

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(force_reset_all_fops, NULL,
				attr_force_reset_all_set, "%llu\n");

static ssize_t hang_debug_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int val;
	char buf[16];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (val == 0)
		conf.flag_switch = 0;
	else if (val == 1)
		conf.flag_switch = 1;
	else if (val == 2)
		conf.flag_switch = 2;
	else
		return count;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_HANG_DEBUG_FLAG,
					&conf);
	}

	return count;
}

static ssize_t hang_debug_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_HANG_DEBUG_FLAG,
					&conf);
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				conf.flag_switch);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations hang_debug_all_fops = {
	.open           = simple_open,
	.read           = hang_debug_all_read,
	.write          = hang_debug_all_write,
	.llseek         = default_llseek,
};

static ssize_t skip_page_retirement_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int val;
	char buf[16];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (val == 0)
		conf.flag_switch = 0;
	else
		conf.flag_switch = 1;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_SKIP_PAGE_RETIREMENT,
					&conf);
	}

	return count;
}

static ssize_t skip_page_retirement_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_SKIP_PAGE_RETIREMENT,
					&conf);
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				conf.flag_switch);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations skip_page_retirement_all_fops = {
	.open           = simple_open,
	.read           = skip_page_retirement_all_read,
	.write          = skip_page_retirement_all_write,
	.llseek         = default_llseek,
};

static ssize_t disable_mmio_protection_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int val;
	char buf[16];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (val == 0)
		conf.flag_switch = 0;
	else
		conf.flag_switch = 1;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_MMIO_PROTECTION,
					&conf);
	}

	return count;
}

static ssize_t disable_mmio_protection_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_MMIO_PROTECTION,
					&conf);
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				conf.flag_switch);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations disable_mmio_protection_all_fops = {
	.open           = simple_open,
	.read           = disable_mmio_protection_all_read,
	.write          = disable_mmio_protection_all_write,
	.llseek         = default_llseek,
};

static ssize_t disable_psp_vf_gate_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int val;
	char buf[16];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (val == 0) {
		conf.flag_switch = 0;
		list_for_each_entry(dev_data, &gim_device_list, list) {
			amdgv_set_dev_conf(dev_data->adev,
						AMDGV_CONF_DISABLE_PSP_VF_GATE,
						&conf);
			amdgv_toggle_psp_vf_gate(dev_data->adev, (u32)0xFFFFFFFF, true);
		}
	} else {
		conf.flag_switch = 1;
		list_for_each_entry(dev_data, &gim_device_list, list) {
			amdgv_toggle_psp_vf_gate(dev_data->adev, (u32)0xFFFFFFFF, false);
			amdgv_set_dev_conf(dev_data->adev,
						AMDGV_CONF_DISABLE_PSP_VF_GATE,
						&conf);
		}
	}

	return count;
}

static ssize_t disable_psp_vf_gate_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_PSP_VF_GATE,
					&conf);
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				conf.flag_switch);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations disable_psp_vf_gate_all_fops = {
	.open           = simple_open,
	.read           = disable_psp_vf_gate_all_read,
	.write          = disable_psp_vf_gate_all_write,
	.llseek         = default_llseek,
};

static ssize_t disable_dcore_debug_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int val;
	char buf[16];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (val == 0)
		conf.flag_switch = 0;
	else
		conf.flag_switch = 1;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_DCORE_DEBUG,
					&conf);
	}

	return count;
}

static ssize_t disable_dcore_debug_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_DISABLE_DCORE_DEBUG,
					&conf);
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				conf.flag_switch);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations disable_dcore_debug_all_fops = {
	.open           = simple_open,
	.read           = disable_dcore_debug_all_read,
	.write          = disable_dcore_debug_all_write,
	.llseek         = default_llseek,
};

static int attr_flr_set(void *data, u64 val)
{
	int ret;
	struct gim_dev_data *dev_data;

	dev_data = (struct gim_dev_data *)data;

	/* Do FLR on the VF */
	ret = amdgv_flr_vf(dev_data->adev, (u32)val);
	if (ret)
		pr_warn("failed to do flr on vf(0x%x) with err(%d)\n",
				(u32)val, ret);

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(flr_fops, NULL, attr_flr_set, "%llu\n");

static int attr_put_error_set_conf(void *data, u64 val)
{
	int ret;
	union amdgv_dev_conf conf;
	struct gim_dev_data *dev_data;

	dev_data = (struct gim_dev_data *)data;

	conf.error_dump_stack_max = val;
	ret = amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_ERROR_DUMP_STACK_MAX,
					&conf);
	return ret;
}

static int attr_put_error_get_conf(void *data, u64 *val)
{
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	dev_data = (struct gim_dev_data *)data;

	ret = amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_ERROR_DUMP_STACK_MAX,
					&conf);

	*val = conf.error_dump_stack_max;

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(put_error_conf_fops, attr_put_error_get_conf,
		attr_put_error_set_conf, "%llu\n");

static ssize_t put_error_set_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	uint32_t val;
	char buf[8];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 8)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	conf.error_dump_stack_max = val;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_ERROR_DUMP_STACK_MAX,
					&conf);
	}

	return count;
}

static ssize_t put_error_set_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_ERROR_DUMP_STACK_MAX,
					&conf);
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				conf.error_dump_stack_max);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations put_error_set_all_fops = {
	.open           = simple_open,
	.read           = put_error_set_all_read,
	.write          = put_error_set_all_write,
	.llseek         = default_llseek,
};

static int init_conf_show(struct seq_file *f, void *p)
{
	struct gim_dev_data *dev_data;
	struct amdgv_init_config_opt *opt;

	dev_data = (struct gim_dev_data *)f->private;
	opt = &dev_data->init_data.opt;

	seq_printf(f, "Adapter[%s] init config:\n",
			dev_name(&dev_data->pdev->dev));
	seq_printf(f, "\ttotal_vf_num = %u\n", opt->total_vf_num);
	seq_printf(f, "\tflags = 0x%llx\n", opt->flags);
	seq_printf(f, "\tgfx_sched_mode = %u\n", opt->gfx_sched_mode);
	seq_printf(f, "\tlog_level = 0x%x\n", opt->log_level);
	seq_printf(f, "\tlog_mask = 0x%x\n", opt->log_mask);
	seq_printf(f, "\tallow_time_full_access = %u\n",
					opt->allow_time_full_access);
	seq_printf(f, "\tfw_load_type = %u\n", opt->fw_load_type);

	if (opt->pf_option) {
		seq_printf(f, "\tpf.fb_offset = 0x%x\n",
				opt->pf_option->fb_offset);
		seq_printf(f, "\tpf.fb_size = 0x%x\n",
				opt->pf_option->fb_size);
		seq_printf(f, "\tpf.mm_bw[0] = 0x%x\n",
				opt->pf_option->mm_bandwidth[0]);
		seq_printf(f, "\tpf.mm_bw[1] = 0x%x\n",
				opt->pf_option->mm_bandwidth[1]);
		seq_printf(f, "\tpf.mm_bw[2] = 0x%x\n",
				opt->pf_option->mm_bandwidth[2]);
	}

	return 0;
}

static int init_conf_open(struct inode *inode, struct file *file)
{
	return single_open(file, init_conf_show, inode->i_private);
}

static const struct file_operations init_conf_fops = {
	.open           = init_conf_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

static ssize_t log_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);

	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	len = 0;
	ret = snprintf_realloc(&buf, &buf_size, len, "shim log level is 0x%x\n",
			shim_log_level);
	len += ret;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_dev_conf(dev_data->adev,
				AMDGV_CONF_LOG_LEVEL_MASK,
				&conf);
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] libgv log level:mask is 0x%x:0x%x\n",
				dev_name(&dev_data->pdev->dev),
				conf.log.log_level,
				conf.log.log_mask);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static ssize_t log_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	char buf[64];
	uint32_t level, mask;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 64)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "0x%x 0x%x 0x%x",
			&shim_log_level, &level, &mask) != 3) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	conf.log.log_level = level;
	conf.log.log_mask = mask;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_set_dev_conf(dev_data->adev,
				AMDGV_CONF_LOG_LEVEL_MASK,
				&conf);
	}

	return count;
}

static const struct file_operations log_all_fops = {
	.open           = simple_open,
	.read           = log_all_read,
	.write          = log_all_write,
	.llseek         = default_llseek,
};

static ssize_t cmd_tmo_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_dev_conf(dev_data->adev,
					AMDGV_CONF_CMD_TIMEOUT,
					&conf);
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				conf.cmd_timeout);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static ssize_t cmd_tmo_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	unsigned val;
	char buf[16];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%u", &val) == 0) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	conf.cmd_timeout = val;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_set_dev_conf(dev_data->adev,
					AMDGV_CONF_CMD_TIMEOUT,
					&conf);
	}

	return count;
}

static const struct file_operations cmd_tmo_all_fops = {
	.open = simple_open,
	.read = cmd_tmo_all_read,
	.write = cmd_tmo_all_write,
	.llseek = default_llseek,
};

static ssize_t log_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int len = 0;
	int ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;
	char *buf = gim_oss_interfaces.alloc_memory(512);

	dev_data = file->private_data;
	ret = snprintf(buf, 512, "shim log level is 0x%x\n",
			shim_log_level);
	len += ret;

	amdgv_get_dev_conf(dev_data->adev, AMDGV_CONF_LOG_LEVEL_MASK, &conf);
	ret = snprintf(buf + len, 512 - len,
			"Adapter[%s] libgv log level:mask is 0x%x:0x%x\n",
			dev_name(&dev_data->pdev->dev),
			conf.log.log_level,
			conf.log.log_mask);
	len += ret;

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);
	return ret;
}

static ssize_t log_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	char buf[64];
	uint32_t level, mask;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;

	if (count > 64)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	dev_data = file->private_data;

	if (sscanf(buf, "0x%x 0x%x 0x%x",
			&shim_log_level, &level, &mask) != 3) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	conf.log.log_level = level;
	conf.log.log_mask = mask;
	amdgv_set_dev_conf(dev_data->adev, AMDGV_CONF_LOG_LEVEL_MASK, &conf);

	return count;
}

static const struct file_operations log_fops = {
	.open           = simple_open,
	.read           = log_read,
	.write          = log_write,
	.llseek         = default_llseek,
};

static ssize_t ffbm_operations_write (struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	char buf[128];
	char permission[3];
	int ret, per;
	uint32_t vf_idx;
	uint64_t gpa, size, spa;
	struct gim_dev_data *dev_data;

	if (count > 64) {
		return -EFAULT;
	}

	if (copy_from_user(buf, user_buf, count)) {
		return -EFAULT;
	}

	dev_data = file->private_data;

	if (strncmp(buf, "print", strlen("print")) == 0) {
		amdgv_ffbm_print_spa_list(dev_data->adev);
	} else if (sscanf(buf, "clear vf%u", &vf_idx) == 1) {
		amdgv_ffbm_clear_vf_mapping(dev_data->adev, vf_idx);
	} else if (sscanf(buf, "map vf%u 0x%llx 0x%llx 0x%llx %s", &vf_idx, &gpa, &size, &spa, permission) == 5) {
		per = ffbm_permission_get(permission);
		if (per == -1) {
			pr_warn("invalid permission value\n");
			return -EINVAL;
		}
		ret = amdgv_ffbm_vf_mapping(dev_data->adev, vf_idx, size, gpa, spa, per);
		if (ret != 0) {
			pr_warn("ffbm mapping failed\n");
			return -ENOMEM;
		}
	} else if (sscanf(buf, "badpage 0x%llx", &spa) == 1) {
		ret = amdgv_ffbm_mark_badpage(dev_data->adev, spa);
		if (ret != 0) {
			pr_warn("ffbm mark badpage failed\n");
			return -ENOMEM;
		}
	} else {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t ffbm_operations_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int len;
	ssize_t ret;
	char *buf = gim_oss_interfaces.alloc_memory(4096);
	struct gim_dev_data *dev_data;
	struct gim_dev_data *dev_data_incoming;
	bool found = false;

	dev_data_incoming = file->private_data;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (len >= 4096) {
			len = 4096;
			break;
		}
		if (dev_data->gpu_index == dev_data_incoming->gpu_index) {
			amdgv_ffbm_read_spa_list(dev_data->adev, buf, 4096, &len);
			found = true;
		}
	}

	if (!found) {
		pr_warn("device is not found\n");
	}

	if (len >= 4096) {
		strcpy(buf, "ffbm page table exceeds 4 KB, pls see dmesg.");
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);
	return ret;
}

static const struct file_operations ffbm_operations = {
	.open           = simple_open,
	.read           = ffbm_operations_read,
	.write          = ffbm_operations_write,
	.llseek         = default_llseek,
};

static ssize_t auto_sched_perf_log_set(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret;
	union amdgv_dev_conf conf;
	struct gim_dev_data *dev_data;
	char buf[64];
	uint32_t val;

	dev_data = file->private_data;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (val == 0)
		conf.flag_switch = 0;
	else
		conf.flag_switch = 1;

	ret = amdgv_set_dev_conf(dev_data->adev,
			AMDGV_CONF_PERF_LOG_FLAG, &conf);

	if (ret) {
		pr_warn("Failed to enable/disable perf log for [%s]\n",
				dev_name(&dev_data->pdev->dev));
	}

	return count;
}

static ssize_t auto_sched_perf_log_get(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	ssize_t ret;
	struct gim_dev_data *dev_data;
	struct amdgv_perf_log_info *perf_log;
	int len = 0;
	void *data = gim_oss_interfaces.alloc_memory(sizeof(struct amdgv_perf_log_info));
	uint32_t data_size = 0;
	size_t buf_resv_size = 4096;
	char *buf = gim_oss_interfaces.alloc_memory(buf_resv_size);
	int i = 0;

	dev_data = file->private_data;

	ret = amdgv_read_auto_sched_perf_log(dev_data->adev, &data_size, data);
	if (ret || data_size != sizeof(struct amdgv_perf_log_info)) {
		pr_warn("Failed to read auto sched perf log for [%s]\n",
				dev_name(&dev_data->pdev->dev));
		goto error;
	}

	perf_log = (struct amdgv_perf_log_info *)data;

	len += snprintf_realloc(&buf, &buf_resv_size, len, "total time quanta(us):\n");
	for (i = 0; i < perf_log->vf_num; i++)
		len += snprintf_realloc(&buf, &buf_resv_size, len, "\tVF%02d:%llu\n",
									perf_log->vf_perf_log_info[i].vf_idx,
									perf_log->vf_perf_log_info[i].time_quanta);
	len += snprintf_realloc(&buf, &buf_resv_size, len, "\tPF:%llu\n",
								perf_log->vf_perf_log_info[perf_log->vf_num].time_quanta);

	len += snprintf_realloc(&buf, &buf_resv_size, len, "total WS cycles:\n");
	for (i = 0; i < perf_log->vf_num; i++)
		len += snprintf_realloc(&buf, &buf_resv_size, len, "\tVF%02d:%llu\n",
									perf_log->vf_perf_log_info[i].vf_idx,
									perf_log->vf_perf_log_info[i].ws_cycle_cnt);
	len += snprintf_realloc(&buf, &buf_resv_size, len, "\tPF:%llu\n",
								perf_log->vf_perf_log_info[perf_log->vf_num].ws_cycle_cnt);

	len += snprintf_realloc(&buf, &buf_resv_size, len, "skipped cycle count:\n");
	for (i = 0; i < perf_log->vf_num; i++)
		len += snprintf_realloc(&buf, &buf_resv_size, len, "\tVF%02d:%llu\n",
									perf_log->vf_perf_log_info[i].vf_idx,
									perf_log->vf_perf_log_info[i].skipped_cycle_cnt);
	len += snprintf_realloc(&buf, &buf_resv_size, len, "\tPF:%llu\n",
								perf_log->vf_perf_log_info[perf_log->vf_num].skipped_cycle_cnt);

	len += snprintf_realloc(&buf, &buf_resv_size, len, "auto yield count:\n");
	for (i = 0; i < perf_log->vf_num; i++)
		len += snprintf_realloc(&buf, &buf_resv_size, len, "\tVF%02d:%llu\n",
									perf_log->vf_perf_log_info[i].vf_idx,
									perf_log->vf_perf_log_info[i].yield_cnt);
	len += snprintf_realloc(&buf, &buf_resv_size, len, "\tPF:%llu\n",
								perf_log->vf_perf_log_info[perf_log->vf_num].yield_cnt);

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

error:
	gim_oss_interfaces.free_memory(data);
	gim_oss_interfaces.free_memory(buf);
	return ret;
}

static const struct file_operations auto_sched_perf_log_fops = {
	.open           = simple_open,
	.read           = auto_sched_perf_log_get,
	.write          = auto_sched_perf_log_set,
	.llseek         = default_llseek,
};

static ssize_t auto_sched_debug_dump_set(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret;
	union amdgv_dev_conf conf;
	struct gim_dev_data *dev_data;
	char buf[64];
	uint32_t val;

	dev_data = file->private_data;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (val == 0)
		conf.flag_switch = 0;
	else
		conf.flag_switch = 1;

	ret = amdgv_set_dev_conf(dev_data->adev,
			AMDGV_CONF_DEBUG_DUMP_FLAG, &conf);

	if (ret) {
		pr_warn("Failed to enable/disable debug dump for [%s]\n",
				dev_name(&dev_data->pdev->dev));
	}

	return count;
}

static ssize_t auto_sched_debug_dump_get(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	ssize_t ret;
	struct gim_dev_data *dev_data;
	int len = 0;
	char *buf = gim_oss_interfaces.alloc_memory(4096);
	uint32_t dump_offset;
	uint32_t dump_size;

	dev_data = file->private_data;
	ret = amdgv_query_debug_dump_fb_addr(dev_data->adev, &dump_offset, &dump_size);

	len += snprintf(buf, 4096, "start: 0x%x\n", dump_offset);
	len += snprintf(buf + len, 4096 - len, "size: 0x%x\n", dump_size);

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	gim_oss_interfaces.free_memory(buf);
	return ret;
}

static const struct file_operations auto_sched_debug_dump_fops = {
	.open           = simple_open,
	.read           = auto_sched_debug_dump_get,
	.write          = auto_sched_debug_dump_set,
	.llseek         = default_llseek,
};

static ssize_t hang_detection_threshold_set(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret;
	union amdgv_dev_conf conf;
	struct gim_dev_data *dev_data;
	char buf[64];
	uint32_t val;

	dev_data = file->private_data;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	conf.u32val = val;

	ret = amdgv_set_dev_conf(dev_data->adev,
			AMDGV_CONF_HANG_DETECTION_THRESHOLD, &conf);

	if (ret) {
		pr_warn("Failed to set hang detection threshold to %dus\n", val);
	}

	return count;
}

static ssize_t hang_detection_threshold_get(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	ssize_t ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;
	int len = 0;
	char *buf = gim_oss_interfaces.alloc_memory(64);

	dev_data = file->private_data;

	ret = amdgv_get_dev_conf(dev_data->adev,
			AMDGV_CONF_HANG_DETECTION_THRESHOLD, &conf);

	len += snprintf(buf, 64, "%dus\n", conf.u32val);

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	gim_oss_interfaces.free_memory(buf);
	return ret;
}

static const struct file_operations hang_detection_threshold_fops = {
	.open           = simple_open,
	.read           = hang_detection_threshold_get,
	.write          = hang_detection_threshold_set,
	.llseek         = default_llseek,
};

static ssize_t hang_detection_duration_set(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret;
	union amdgv_dev_conf conf;
	struct gim_dev_data *dev_data;
	char buf[64];
	uint32_t val;

	dev_data = file->private_data;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	conf.u32val = val;

	ret = amdgv_set_dev_conf(dev_data->adev,
			AMDGV_CONF_HANG_DETECTION_DURATION, &conf);

	if (ret) {
		pr_warn("Failed to set hang detection threshold to %dus\n", val);
	}

	return count;
}

static ssize_t hang_detection_duration_get(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	ssize_t ret;
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;
	int len = 0;
	char *buf = gim_oss_interfaces.alloc_memory(64);

	dev_data = file->private_data;

	ret = amdgv_get_dev_conf(dev_data->adev,
			AMDGV_CONF_HANG_DETECTION_DURATION, &conf);

	len += snprintf(buf, 64, "%dus\n", conf.u32val);

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	gim_oss_interfaces.free_memory(buf);
	return ret;
}

static const struct file_operations hang_detection_duration_fops = {
	.open           = simple_open,
	.read           = hang_detection_duration_get,
	.write          = hang_detection_duration_set,
	.llseek         = default_llseek,
};

static ssize_t trigger_manual_dump_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int idx_vf = 0;
	char buf[16];
	struct gim_dev_data *dev_data;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &idx_vf) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (idx_vf < 0 || idx_vf > 31)
		return -EINVAL;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		gim_oss_interfaces.signal_manual_dump_happened(dev_data->adev, idx_vf);
		gim_oss_interfaces.signal_diag_data_ready(dev_data->adev);
	}
	return count;
}

static ssize_t trigger_manual_dump_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;

	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				1);
		if (!ret)
			break;
		len += ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations trigger_manual_dump_all_fops = {
	.open           = simple_open,
	.read           = trigger_manual_dump_all_read,
	.write          = trigger_manual_dump_all_write,
	.llseek         = default_llseek,
};

static ssize_t trigger_manual_dump_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int idx_vf = 0;
	char buf[16];
	struct gim_dev_data *dev_data;
	struct gim_dev_data *dev_data_incoming;
	dev_data_incoming = file->private_data;
	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &idx_vf) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (idx_vf < 0 || idx_vf > 31)
		return -EINVAL;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (dev_data->gpu_index == dev_data_incoming->gpu_index) {
			gim_oss_interfaces.signal_manual_dump_happened(dev_data->adev, idx_vf);
			gim_oss_interfaces.signal_diag_data_ready(dev_data->adev);
		}
	}
	return count;
}

static ssize_t trigger_manual_dump_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret, len;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	struct gim_dev_data *dev_data_incoming;
	dev_data_incoming = file->private_data;
	len = 0;
	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (dev_data->gpu_index == dev_data_incoming->gpu_index) {
			ret = snprintf_realloc(&buf, &buf_size, len,
					"Adapter[%s] :%u\n",
					dev_name(&dev_data->pdev->dev),
					1);
			if (!ret)
				break;
			len += ret;
		}
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations trigger_manual_dump_fops = {
	.open           = simple_open,
	.read           = trigger_manual_dump_read,
	.write          = trigger_manual_dump_write,
	.llseek         = default_llseek,
};

static ssize_t mes_info_dump_all_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int val;
	char buf[16];
	struct gim_dev_data *dev_data;
	bool enable = 0;
	int ret = 0;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (val == 0)
		enable = false;
	else
		enable = true;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		ret = amdgv_set_mes_info_dump_enable(dev_data->adev, enable);
		if (ret)
			pr_warn("fail to enable mes info dump on device[%s]!\n",
				dev_name(&dev_data->pdev->dev));
	}

	return count;
}

static ssize_t mes_info_dump_all_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret = 0, len = 0;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	uint32_t adapt_enable = 0;
	uint32_t vf_enable[32] = {0};
	unsigned int num_vf = 0;
	int i = 0;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		amdgv_get_mes_info_dump_enable(dev_data->adev, &adapt_enable, vf_enable, &num_vf);
		ret = snprintf_realloc(&buf, &buf_size, len,
			"Adapter[%s] :%u\n",
			dev_name(&dev_data->pdev->dev),
			adapt_enable);
		if (!ret)
			break;
		len += ret;
		for (i = 0; i < num_vf; ++i) {
			ret = snprintf_realloc(&buf, &buf_size, len,
				"\tvf[%d] :%u\n", i,
				vf_enable[i]);
			if (!ret)
				break;
			len += ret;
		}
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations mes_info_dump_all_fops = {
	.open           = simple_open,
	.read           = mes_info_dump_all_read,
	.write          = mes_info_dump_all_write,
	.llseek         = default_llseek,
};

static ssize_t mes_info_dump_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int val;
	char buf[16];
	struct gim_dev_data *dev_data;
	struct gim_dev_data *dev_data_incoming = file->private_data;
	bool enable = false;
	int ret = 0;

	if (count > 16)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1) {
		pr_warn("invalid parameter\n");
		return -EINVAL;
	}

	if (val == 0)
		enable = false;
	else
		enable = true;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (dev_data->gpu_index == dev_data_incoming->gpu_index) {
			ret = amdgv_set_mes_info_dump_enable(dev_data->adev, enable);
			if (ret)
				pr_warn("fail to enable mes info dump on device[%s]!\n",
					dev_name(&dev_data->pdev->dev));
			break;
		}
	}

	return count;
}

static ssize_t mes_info_dump_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret = 0, len = 0;
	size_t buf_size = 512;
	char *buf = gim_oss_interfaces.alloc_memory(buf_size);
	struct gim_dev_data *dev_data;
	struct gim_dev_data *dev_data_incoming = file->private_data;
	uint32_t adapt_enable = 0;
	uint32_t vf_enable[32] = {0};
	unsigned int num_vf = 0;
	int i = 0;

	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (dev_data->gpu_index == dev_data_incoming->gpu_index) {
			amdgv_get_mes_info_dump_enable(dev_data->adev, &adapt_enable, vf_enable, &num_vf);
			ret = snprintf_realloc(&buf, &buf_size, len,
				"Adapter[%s] :%u\n",
				dev_name(&dev_data->pdev->dev),
				adapt_enable);
			if (!ret)
				break;
			len += ret;
			for (i = 0; i < num_vf; ++i) {
				ret = snprintf_realloc(&buf, &buf_size, len,
					"\tvf[%d] :%u\n", i,
					vf_enable[i]);
				if (!ret)
					break;
				len += ret;
			}
			break;
		}
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	gim_oss_interfaces.free_memory(buf);

	return ret;
}

static const struct file_operations mes_info_dump_fops = {
	.open           = simple_open,
	.read           = mes_info_dump_read,
	.write          = mes_info_dump_write,
	.llseek         = default_llseek,
};

static ssize_t asymmetric_fb_read(struct file *file,
		char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret;
	struct gim_dev_data *dev_data;
	size_t buf_resv_size = 4096;
	char *buf = gim_oss_interfaces.alloc_memory(buf_resv_size);
	int len = 0;

	dev_data = file->private_data;

	ret = amdgv_dump_asymmetric_fb_layout(dev_data->adev, buf, &len, buf_resv_size);
	if (ret) {
		gim_oss_interfaces.free_memory(buf);
		return ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	gim_oss_interfaces.free_memory(buf);
	return ret;
}

static ssize_t asymmetric_fb_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret;
	char buf[64];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;
	uint32_t vf_idx, vf_fb_size;

	dev_data = file->private_data;

	if (count > 64)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d,%d", &vf_idx, &vf_fb_size) == 2) {
		if ((vf_idx > AMDGV_PF_IDX) || (vf_idx < 0)) {
			pr_warn("vf_idx must be within 0 to 31(PF_IDX)\n");
			return -EINVAL;
		}
		conf.asymmetric.vf_idx = vf_idx;
		conf.asymmetric.vf_fb_size = vf_fb_size;
		conf.asymmetric.defragment = false;
	} else {
		pr_warn("invalid parameter: the format should be \"vf_idx, vf_fb_size\".\n");
		return -EINVAL;
	}

	ret = amdgv_set_dev_conf(dev_data->adev, AMDGV_CONF_ASYMMETRIC_FB, &conf);
	if (ret) {
		pr_warn("Failed to set asymmetric fb for [%s]\n",
				dev_name(&dev_data->pdev->dev));
		return -EFAULT;
	}

	return count;
}

static const struct file_operations asymmetric_fb_fops = {
	.open           = simple_open,
	.read           = asymmetric_fb_read,
	.write          = asymmetric_fb_write,
	.llseek         = default_llseek,
};

static ssize_t fb_defragment_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int ret;
	char buf[64];
	struct gim_dev_data *dev_data;
	union amdgv_dev_conf conf;
	uint32_t val;

	dev_data = file->private_data;

	if (count > 64)
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) != 1 || !val) {
		pr_warn("Invalid parameter.\n");
		return -EINVAL;
	}

	pr_warn("Perform vf FB defragment. Please make sure running VM are paused.\n");
	conf.asymmetric.defragment = true;

	ret = amdgv_set_dev_conf(dev_data->adev, AMDGV_CONF_ASYMMETRIC_FB, &conf);
	if (ret) {
		pr_warn("Failed to perform FB defragment for [%s]\n",
				dev_name(&dev_data->pdev->dev));
		return -EFAULT;
	}

	return count;
}

static const struct file_operations fb_defragment_fops = {
	.open           = simple_open,
	.write           = fb_defragment_write,
	.llseek         = default_llseek,
};

void gim_debugfs_init(void)
{
	int i;
	struct pci_dev *pdev_vf;
	struct dentry *vf_dir;
	struct dentry *adapt_dir;
	struct gim_dev_data *dev_data;
	struct dentry *entry;

	/* root debugfs dir */
	root_dir = debugfs_create_dir("gim", NULL);
	if (!root_dir || root_dir == ERR_PTR(-ENODEV)) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_DIR_FAIL, 0);
		return;
	}

	/* interfaces for all GPUs */
	entry = debugfs_create_file("force_reset", 0200,
			root_dir,
			NULL, &force_reset_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("hang_debug", 0200,
			root_dir,
			NULL, &hang_debug_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("skip_page_retirement", 0200,
			root_dir,
			NULL, &skip_page_retirement_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("disable_mmio_protection", 0200,
			root_dir,
			NULL, &disable_mmio_protection_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("disable_psp_vf_gate", 0200,
			root_dir,
			NULL, &disable_psp_vf_gate_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("force_switch_vf_idx", 0200,
			root_dir,
			NULL, &force_switch_vf_debug_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("log", 0600,
			root_dir,
			NULL, &log_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("put_error_conf", 0200,
			root_dir,
			NULL, &put_error_set_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("cmd_timeout", 0200,
			root_dir,
			NULL, &cmd_tmo_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("disable_dcore_debug", 0200,
			root_dir,
			NULL, &disable_dcore_debug_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}

	entry = debugfs_create_file("trigger_vf_debug_dump", 0200,
			root_dir,
			NULL, &trigger_manual_dump_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}
	entry = debugfs_create_file("mes_info_dump_enable", 0200,
			root_dir,
			NULL, &mes_info_dump_all_fops);
	if (entry == NULL) {
		gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
		goto err;
	}
	/* adapter debugfs dir */
	list_for_each_entry(dev_data, &gim_device_list, list) {
		adapt_dir = debugfs_create_dir(dev_name(&dev_data->pdev->dev),
						root_dir);
		if (!adapt_dir || adapt_dir == ERR_PTR(-ENODEV)) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_DIR_FAIL, 0);
			goto err;
		}

		/* interfaces for one GPU */
		entry = debugfs_create_file("rlcv_timestamp_dump", 0200,
				adapt_dir,
				dev_data, &rlcv_timestamp_dump_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}
		entry = debugfs_create_file("force_reset", 0200,
				adapt_dir,
				dev_data, &force_reset_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("hang_detection_threshold", 0600,
				adapt_dir,
				dev_data, &hang_detection_threshold_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("hang_detection_duration", 0600,
				adapt_dir,
				dev_data, &hang_detection_duration_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("skip_page_retirement", 0200,
				adapt_dir,
				dev_data, &skip_page_retirement_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("hang_debug", 0200,
				adapt_dir,
				dev_data, &hang_debug_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}
#ifdef WS_RECORD
		entry = debugfs_create_file("ws_record", 0200,
				adapt_dir,
				dev_data, &ws_record_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}
#endif
		entry = debugfs_create_file("disable_mmio_protection", 0200,
				adapt_dir,
				dev_data, &disable_mmio_protection_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("disable_psp_vf_gate", 0200,
				adapt_dir,
				dev_data, &disable_psp_vf_gate_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("hybrid_liquid_min_ts", 0600,
			    adapt_dir,
			    dev_data, &hliquid_min_ts_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("enable_access", 0200,
				adapt_dir,
				dev_data, &enable_access_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("force_switch_vf_idx", 0200,
				adapt_dir,
				dev_data, &force_switch_vf_debug_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("init_conf", 0400,
				adapt_dir,
				dev_data, &init_conf_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("mm_quanta_option", 0200,
				adapt_dir,
				dev_data, &mm_quanta_option);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("log", 0600,
				adapt_dir,
				dev_data, &log_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("flr", 0200,
				adapt_dir,
				dev_data, &flr_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("put_error_conf", 0200,
				adapt_dir,
				dev_data, &put_error_conf_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("ffbm", 0600,
				adapt_dir,
				dev_data, &ffbm_operations);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("disable_dcore_debug", 0200,
				adapt_dir,
				dev_data, &disable_dcore_debug_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("perf_log", 0600,
				adapt_dir,
				dev_data, &auto_sched_perf_log_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("debug_dump", 0600,
				adapt_dir,
				dev_data, &auto_sched_debug_dump_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("asymmetric_timeslice", 0600,
				adapt_dir,
				dev_data, &asymmetric_timeslice_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		for (i = 0; i < dev_data->vf_num; i++) {
			pdev_vf = dev_data->vf_map[i].pdev;
			/* VF debugfs dir */
			vf_dir = debugfs_create_dir(dev_name(&pdev_vf->dev),
							adapt_dir);
			if (!vf_dir || vf_dir == ERR_PTR(-ENODEV)) {
				gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_DIR_FAIL, 0);
				goto err;
			}
		}

		entry = debugfs_create_file("trigger_vf_debug_dump", 0200,
				adapt_dir,
				dev_data, &trigger_manual_dump_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}
		entry = debugfs_create_file("mes_info_dump_enable", 0200,
				adapt_dir,
				dev_data, &mes_info_dump_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}
		entry = debugfs_create_file("asymmetric_fb", 0600,
				adapt_dir,
				dev_data, &asymmetric_fb_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}

		entry = debugfs_create_file("fb_defragment", 0200,
				adapt_dir,
				dev_data, &fb_defragment_fops);
		if (entry == NULL) {
			gim_put_error(AMDGV_ERROR_DRIVER_CREATE_DEBUGFS_FILE_FAIL, 0);
			goto err;
		}
	}

	return;

err:
	debugfs_remove_recursive(root_dir);
	root_dir = NULL;
	return;
}

void gim_debugfs_fini(void)
{
	if (root_dir && (root_dir != ERR_PTR(-ENODEV)))
		debugfs_remove_recursive(root_dir);
}
