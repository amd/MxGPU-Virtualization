/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <pthread.h>
#include <errno.h>

#include "gim_log.h"
#include "smi_drv.h"
#include "smi_drv_core.h"
#include "smi_drv_core_api.h"
#include "vfio_drv.h"
#include "gim_gpumon.h"
#include "amdgv_gpumon.h"

extern const char gim_driver_version[];

static pthread_mutex_t gim_device_list_lock = PTHREAD_MUTEX_INITIALIZER;

static void gim_lock_device_list(void)
{
	pthread_mutex_lock(&gim_device_list_lock);
}

static void gim_get_device_list(struct smi_device_data *dev_list,
				int *size)
{
	struct gim_dev_data *dev_data;
	int i = 0;
	struct gim_drv *vfio_drv;

	vfio_drv = gim_get_driver_instance();
	if (vfio_drv == NULL || dev_list == NULL || size == NULL) {
		gim_error("invalid params\n");
		return;
	}

	SLIST_FOREACH(dev_data, &vfio_drv->head, entries)
	{
		memcpy(&dev_list[i].init_data, &dev_data->init_data,
			sizeof(struct amdgv_init_data));
		dev_list[i].adev = dev_data->adev;
		dev_list[i].parent = -1; // Implement later: dev_data->parent;
		i++;
	}

	*size = i;
}

static void gim_unlock_device_list(void)
{
	pthread_mutex_unlock(&gim_device_list_lock);
}

static struct gim_dev_data *
gim_get_device_data_from_adev(struct gim_drv *vfio_drv,
	amdgv_dev_t adev)
{
	struct gim_dev_data *dev_data = NULL;
	bool found = false;

	pthread_mutex_lock(&gim_device_list_lock);
	SLIST_FOREACH(dev_data, &vfio_drv->head, entries)
	{
		if (adev == dev_data->adev) {
			found = true;
			break;
		}
	}
	pthread_mutex_unlock(&gim_device_list_lock);

	return (found == true) ? dev_data : NULL;
}

static void gim_get_device_data(amdgv_dev_t adev,
	struct smi_device_data *ret_dev_data)
{
	struct gim_dev_data *dev_data = NULL;
	struct gim_drv *vfio_drv;

	vfio_drv = gim_get_driver_instance();
	if (vfio_drv == NULL) {
		gim_error("vfio driver instance not found\n");
		return;
	}

	dev_data = gim_get_device_data_from_adev(vfio_drv, adev);
	if (dev_data == NULL) {
		gim_error("device data not found for adev=%p\n", adev);
		return;
	}

	if (ret_dev_data) {
		memcpy(&ret_dev_data->init_data, &dev_data->init_data,
			sizeof(struct amdgv_init_data));
		ret_dev_data->adev = dev_data->adev;
		ret_dev_data->parent = -1; // Implement later: dev_data->parent;
	}

	if (dev_data != SLIST_FIRST(&vfio_drv->head)) {
		pthread_mutex_lock(&dev_data->dev_lock);
	}
}

static void gim_put_handle(amdgv_dev_t adev)
{
	struct gim_dev_data *dev_data;
	struct gim_drv *vfio_drv;

	vfio_drv = gim_get_driver_instance();
	if (vfio_drv == NULL) {
		gim_error("vfio driver instance not found\n");
		return;
	}

	dev_data = gim_get_device_data_from_adev(vfio_drv, adev);
	if (dev_data == NULL) {
		gim_error("device data not found for adev=%p\n", adev);
		return;
	}

	if (dev_data != SLIST_FIRST(&vfio_drv->head)) {
		pthread_mutex_unlock(&dev_data->dev_lock);
	}
}

static void gim_generate_date_string(char *buf, uint64_t utime)
{
	uint64_t utc_timestamp;
	uint64_t utc_sec;
	uint64_t millisec;
	struct tm *utc_time;

	if (buf == NULL) {
		gim_error("invalid params\n");
		return;
	}

	memset(buf, 0, SMI_MAX_DATE_LENGTH);

	if (utime == 0) {
		strcpy(buf, "--N/A--");
		return;
	}

	utc_timestamp = gim_gpumon_time_to_utc(utime);
	utc_sec = utc_timestamp / 1000000;
	millisec = (utc_timestamp / 1000) % 1000;

	utc_time = gmtime(&utc_sec);

	snprintf(buf, SMI_MAX_DATE_LENGTH, SMI_DATE_FORMAT,
		utc_time->tm_year + 1900, utc_time->tm_mon + 1,
		utc_time->tm_mday, utc_time->tm_hour, utc_time->tm_min,
		utc_time->tm_sec, (int)millisec);
}

static int gim_get_pcie_confs(amdgv_dev_t adev,
	int *speed, int *width, int *max_vf_num)
{
	int ret = 0;
	struct gim_dev_data *dev_data;
	struct gim_drv *vfio_drv;

	vfio_drv = gim_get_driver_instance();
	if (vfio_drv == NULL) {
		gim_error("vfio driver instance not found\n");
		return -EINVAL;
	}

	dev_data = gim_get_device_data_from_adev(vfio_drv, adev);
	if (dev_data == NULL) {
		gim_error("device data not found for adev=%p\n", adev);
		return -EIO;
	}

	ret = amdgv_gpumon_get_pcie_confs(adev, dev_data,
			gim_gpumon_get_pcie_confs,
			speed, width, max_vf_num);

	return ret;
}

static unsigned int gim_hash_64(uint64_t val, unsigned int bits)
{
	uint64_t hash = val;

	/* xor high and low 32-bit halves */
	hash ^= hash >> 32;

	/* multiply by a magic constant */
	hash *= 0xff51afd7ed558ccdull;

	/* xor high and low 32-bit halves again */
	hash ^= hash >> 32;

	/* shift right by (64 - bits) bits */
	return hash >> (64 - bits);
}

static int gim_get_driver_version(amdgv_dev_t adev, uint32_t *length, char *version)
{
	char client_id;
	uint32_t major;
	uint32_t minor;
	uint32_t subminor;

	if (smi_oss_funcs->strnstr(gim_driver_version, "staging", STRLEN_NORMAL) != -1) {
		client_id = 'K';
		major = 0;
		subminor = 0;
		minor = 0;
		*length = smi_vsnprintf(version, SMI_MAX_STRING_LENGTH,
			"%d.%d.%d+%c", major, minor, subminor, client_id);
	} else {
		if ((sscanf(gim_driver_version, "%c", &client_id) == 1) &&
			((client_id >= 'a' && client_id <= 'z') ||
			(client_id >= 'A' && client_id <= 'Z'))) {
			if (sscanf(gim_driver_version, "%c.%u.%u.%u", &client_id,
				&major, &minor, &subminor) != 4) {
				client_id = '-';
				major = 0;
				subminor = 0;
				minor = 0;
			}
			*length = smi_vsnprintf(version, SMI_MAX_STRING_LENGTH,
				"%c.%u.%u.%u", client_id, major, minor, subminor);
		} else {
			char special_character;
			if ((sscanf(gim_driver_version, "%u.%u.%u%c%c", &major, &minor,
				&subminor, &special_character, &client_id) != 5) ||
				(special_character != '+' &&  special_character == '.')) {
				client_id = '-';
				major = 0;
				subminor = 0;
				minor = 0;
			}
			*length = smi_vsnprintf(version, SMI_MAX_STRING_LENGTH,
			"%u.%u.%u%c%c", major, minor, subminor, special_character, client_id);
		}
	}

	return 0;
}

static int gim_get_driver_id(uint8_t *driver_id)
{
	*driver_id = SMI_DRIVER_LIBGV;

	return 0;
}

static int gim_get_profile_info(amdgv_dev_t adev,
	struct smi_profile_info *profile_info)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_create_event(struct smi_ctx *ctx, amdgv_dev_t *adev,
	uint64_t dev_id, uint64_t event_mask)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_read_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id, struct smi_event_entry *event)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_destroy_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_get_driver_date(amdgv_dev_t adev, char *driver_date)
{
	amdgv_get_date(driver_date);
	return driver_date == NULL ? SMI_STATUS_API_FAILED : SMI_STATUS_SUCCESS;
}

static int gim_get_driver_model(amdgv_dev_t adev, enum smi_driver_model_type *driver_model)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_get_partition(struct smi_profile_configs *profile_configs,
				   struct amdgv_gpumon_accelerator_partition_profile_config *caps)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_get_metric_table(struct smi_metrics_table *metrics_table, uint16_t size,
				   struct amdgv_gpumon_metrics_ext *gpumon_metrics_table)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_get_eeprom_table(struct smi_bad_page_info *eeprom_table, uint16_t size, uint32_t bp_cnt,
				   struct amdgv_smi_ras_eeprom_table_record *gpumon_eeprom_table)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_get_cper_data(struct smi_cper_config *cper_config, uint16_t in_len, uint64_t size, char* buffer, uint64_t write_count,
				   uint32_t* smi_cper_hdrs)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

struct smi_shim_interface gim_smi_interfaces = {
	.lock_device_list = gim_lock_device_list,
	.get_device_list = gim_get_device_list,
	.unlock_device_list = gim_unlock_device_list,
	.get_shim_log_level = gim_get_log_level,
	.get_device_data = gim_get_device_data,
	.put_handle = gim_put_handle,

	.set_file_private_data = NULL,
	.get_file_private_data = NULL,
	.verify_file_descriptor = NULL,

	.generate_date_string = gim_generate_date_string,
	.get_pcie_confs = gim_get_pcie_confs,
	.create_hash_64 = gim_hash_64,

	.get_driver_version = gim_get_driver_version,
	.get_driver_id = gim_get_driver_id,
	.get_profile_info = gim_get_profile_info,
	.get_driver_date = gim_get_driver_date,
	.get_driver_model = gim_get_driver_model,
	.get_metric_table = gim_get_metric_table,
	.get_eeprom_table = gim_get_eeprom_table,
	.get_partition = gim_get_partition,
	.get_cper_data = gim_get_cper_data,

	.create_event = gim_create_event,
	.read_event = gim_read_event,
	.destroy_event = gim_destroy_event
};

static int smi_set_file_private_data(file_t filp, struct smi_ctx *sctx)
{
	smi_process_handle file;

	if (filp == NULL) {
		gim_error("invalid params\n");
		return -EINVAL;
	}

	file = (smi_process_handle)filp;
	file->private_data = sctx;

	return 0;
}

static int smi_get_file_private_data(file_t filp, struct smi_ctx **sctx)
{
	smi_process_handle file;

	if (filp == NULL || sctx == NULL) {
		gim_error("invalid params\n");
		return -EINVAL;
	}

	file = (smi_process_handle)filp;
	*sctx = file->private_data;

	return 0;
}

static int smi_verify_file_descriptor(file_t filp)
{
	/* In userspace, file descriptor is always valid */
	return 0;
}

int smi_init(struct oss_interface *oss_interface,
		struct smi_shim_interface *shim_interface)
{
	int ret;

	/* pass oss function to smi core */
	shim_interface->set_file_private_data = smi_set_file_private_data;
	shim_interface->get_file_private_data = smi_get_file_private_data;
	shim_interface->verify_file_descriptor = smi_verify_file_descriptor;
	ret = smi_core_init(oss_interface, shim_interface);
	if (ret < 0) {
		gim_error("failed to init SMI core\n");
		return ret;
	}

	gim_info("SMI initialized successfully\n");

	return 0;
}

void smi_cleanup(void)
{
	smi_core_fini();
}

int smi_open(smi_process_handle file, bool is_privileged)
{
	return smi_core_open((file_t)file, is_privileged);
}

int smi_release(smi_process_handle file)
{
	return smi_core_release((file_t)file);
}

long smi_ioctl_handler(smi_process_handle file, unsigned int cmd, void *arg)
{
	return smi_core_ioctl_handler((file_t)file, cmd, arg);
}
