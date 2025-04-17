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

#include <linux/kobject.h>
#include <linux/cdev.h>

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/utsname.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/scatterlist.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/eventfd.h>
#include <linux/poll.h>

#include <linux/rtc.h>
#include <linux/hash.h>
#include <linux/sort.h>
#include <linux/highmem.h>
#include <linux/mm.h>

#include "gim.h"
#include "gim_debug.h"
#include "gim_gpumon.h"

#include "smi_drv.h"
#include "smi_drv_core.h"
#include "smi_drv_core_api.h"
#include "smi_drv_event.h"
#include "smi_drv_utils.h"

#define SMI_DATE_FORMAT "%04d-%02d-%02d:%02d:%02d:%02d.%03d"
#define SMI_DEVICE_NAME "gim-smi"

/* MINOR_COUNT cannot be 0, otherwise unregister cannot remove chrdev */
#define MINOR_COUNT 1

extern const char gim_driver_version[];

extern struct list_head gim_device_list;
extern uint32_t shim_log_level;
extern struct mutex gim_device_list_lock;
extern struct gim_error_ring_buffer *gim_error_rb;

static struct cdev	 smi_cdev;
static struct class	*smi_class;
static struct device	*smi_dev;
static dev_t devid;

/* shim interface function */
static void gim_lock_device_list(void)
{
	mutex_lock(&gim_device_list_lock);
}

static void gim_get_device_list(struct smi_device_data *dev_list, int *size)
{
	struct gim_dev_data *dev_data;
	struct smi_device_data *tmp_dev_list;
	int i = 0;
	int j = 0;

	tmp_dev_list = kmalloc_array(SMI_MAX_DEVICES, sizeof(struct smi_device_data), GFP_KERNEL);

	list_for_each_entry(dev_data, &gim_device_list, list) {
		memcpy(&tmp_dev_list[i].init_data, &dev_data->init_data, sizeof(struct amdgv_init_data));
		tmp_dev_list[i].adev = dev_data->adev;
		tmp_dev_list[i].parent = dev_data->parent;
		tmp_dev_list[i].init_data.info.bdf = dev_data->init_data.info.bdf;
		i++;
	}

	// sort by BDF
	sort(tmp_dev_list, i, sizeof(struct smi_device_data), smi_compare_dev_bdf, NULL);

	for (j = 0; j < i; j++) {
		dev_list[j].adev = tmp_dev_list[j].adev;
		dev_list[j].parent = tmp_dev_list[j].parent;
		memcpy(&dev_list[j].init_data, &tmp_dev_list[j].init_data, sizeof(struct amdgv_init_data));
	}

	*size = i;
	kfree(tmp_dev_list);
}

static void gim_get_device_data(amdgv_dev_t adev, struct smi_device_data *ret_dev_data, bool *dev_busy, struct smi_ctx *ctx)
{
	struct gim_dev_data *dev_data = NULL;
	bool m_ret = false;
	unsigned long timeout = jiffies + 60*HZ; //60 seconds from start

	*dev_busy = false;

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (adev == dev_data->adev)
			break;
	}
	mutex_unlock(&gim_device_list_lock);
	if (ret_dev_data) {
		memcpy(&ret_dev_data->init_data, &dev_data->init_data, sizeof(struct amdgv_init_data));
		ret_dev_data->adev = dev_data->adev;
		ret_dev_data->parent = dev_data->parent;
	}

	if (&dev_data->list != &gim_device_list) {
		m_ret = mutex_trylock(&dev_data->dev_lock);
		while (time_before(jiffies, timeout)) {
			if (!m_ret) {
				m_ret = mutex_trylock(&dev_data->dev_lock);
			} else {
				break;
			}
		}
		if (!m_ret) {
			*dev_busy = true;
		}
	}
}

static void gim_put_handle(amdgv_dev_t adev, struct smi_ctx *ctx)
{
	struct gim_dev_data *dev_data = NULL;

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list)
		if (adev == dev_data->adev)
			break;
	mutex_unlock(&gim_device_list_lock);

	if (&dev_data->list != &gim_device_list)
		mutex_unlock(&dev_data->dev_lock);
}

static int gim_get_pcie_confs(amdgv_dev_t adev,
			int *speed, int *width, int *max_vf_num)
{
	int ret = 0;
	struct gim_dev_data *dev_data = NULL;

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (adev == dev_data->adev)
			break;
	}
	mutex_unlock(&gim_device_list_lock);

	ret = amdgv_gpumon_get_pcie_confs(adev, dev_data, gim_gpumon_get_pcie_confs, speed,
				width, max_vf_num);

	return ret;
}

static void gim_unlock_device_list(void)
{
	mutex_unlock(&gim_device_list_lock);
}

static int gim_get_shim_log_level(void)
{
	return shim_log_level;
}

static void gim_generate_date_string(char *buf, uint64_t ktime)
{
	struct rtc_time tm;
	uint64_t utc_timestamp;
	uint64_t millisec;

	if (ktime == 0) {
		strcpy(buf, "--N/A--");
		buf[7] = 0;
		return;
	}

	utc_timestamp = gim_gpumon_ktime_to_utc(ktime);
	millisec = (utc_timestamp / 1000) % 1000;
	utc_timestamp = utc_timestamp * 1000; // convert us to ns

#if !defined(HAVE_RTC_KTIME_TO_TM)
	tm = rtc_ktime_to_tm((ktime_t){.tv64 = utc_timestamp});
#else
	tm = rtc_ktime_to_tm(utc_timestamp);
#endif

	snprintf(buf, SMI_MAX_DATE_LENGTH,
		SMI_DATE_FORMAT,
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec,
		(int) millisec);

	/* YYYY-MM-DD:HH:MM:SS.MSC */
	buf[23] = 0;
}

static int gim_create_event(struct smi_ctx *ctx, amdgv_dev_t *adev, struct smi_event_set_config *config)
{
	return smi_create_event(ctx, adev, config);
}

static int gim_read_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id, struct smi_event_entry *event)
{
	return smi_read_event(ctx, adev, dev_id, event);
}

static int gim_destroy_event(struct smi_ctx *ctx, amdgv_dev_t *adev, uint64_t dev_id)
{
	return smi_destroy_event(ctx, adev, dev_id);
}


static unsigned int gim_hash_64(uint64_t a, unsigned int bits)
{
	return hash_64(a, bits);
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
		*length = smi_vsnprintf(version, SMI_MAX_STRING_LENGTH, "%d.%d.%d+%c", major, minor, subminor, client_id);
	} else {
		if ((sscanf(gim_driver_version, "%c", &client_id) == 1) &&
			((client_id >= 'a' && client_id <= 'z') || (client_id >= 'A' && client_id <= 'Z'))) {
			if (sscanf(gim_driver_version, "%c.%u.%u.%u", &client_id, &major, &minor, &subminor) != 4) {
				client_id = '-';
				major = 0;
				subminor = 0;
				minor = 0;
			}
			*length = smi_vsnprintf(version, SMI_MAX_STRING_LENGTH, "%c.%u.%u.%u", client_id, major, minor, subminor);
		} else {
			char special_character;
			if ((sscanf(gim_driver_version, "%u.%u.%u%c%c", &major, &minor, &subminor, &special_character, &client_id) != 5) ||
				(special_character != '+' &&  special_character == '.')) {
				client_id = '-';
				major = 0;
				subminor = 0;
				minor = 0;
			}
			*length = smi_vsnprintf(version, SMI_MAX_STRING_LENGTH, "%u.%u.%u%c%c", major, minor, subminor, special_character, client_id);
		}
	}

	return 0;
}

static int gim_get_driver_id(uint32_t *driver_id)
{
	*driver_id = SMI_DRIVER_LIBGV;

	return 0;
}

static int gim_get_profile_info(amdgv_dev_t adev, struct smi_profile_info *profile_info)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_get_driver_date(amdgv_dev_t adev, char *driver_date)
{
	if (driver_date == NULL) {
		return SMI_STATUS_API_FAILED;
	} else {
		amdgv_get_date(driver_date);
	}
	return SMI_STATUS_SUCCESS;
}

static int gim_get_driver_model(amdgv_dev_t adev, enum smi_driver_model_type *driver_model)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

static int gim_get_metric_table(struct smi_metrics_table *metrics_table, uint16_t size,
				struct amdgv_gpumon_metrics_ext *gpumon_metrics_table)
{
	struct smi_metrics *metrics = NULL;
	long num_pages = 0;
	unsigned int gup_flags;
	struct mm_struct *mm;
	size_t buffer_size;
	unsigned long nr_pages;
	struct page **pages;
	uint32_t i = 0;

	mm = current->mm; // Get memory management structure

	// Calculate the number of pages needed to map the entire metrics table
	buffer_size = sizeof(struct smi_metrics);
	nr_pages = (buffer_size + PAGE_SIZE - 1) >> PAGE_SHIFT;

	pages = kmalloc(nr_pages * sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
    down_read(&current->mm->mmap_lock);
#else
    down_read(&current->mm->mmap_sem);
#endif
	gup_flags = FOLL_WRITE;
	// Map user space buffer containing metrics table into kernel space
	// by creating memory pages
#if defined(HAVE_GET_USER_PAGES_REMOTE_6_ARG)
	num_pages = get_user_pages_remote(mm, (unsigned long)metrics_table->metrics, nr_pages, gup_flags, pages, NULL);
#elif defined(HAVE_GET_USER_PAGES_REMOTE_7_ARG)
    num_pages = get_user_pages_remote(mm, (unsigned long)metrics_table->metrics, nr_pages, gup_flags, pages, NULL, NULL);
#else
    num_pages = get_user_pages_remote(NULL, mm, (unsigned long)metrics_table->metrics, nr_pages, gup_flags, pages, NULL, NULL);
#endif
	if (num_pages <= 0) {
		kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
		up_read(&current->mm->mmap_lock);
#else
		up_read(&current->mm->mmap_sem);
#endif
		return SMI_STATUS_API_FAILED;
	}
	// Access mapped metrics table in kernel space
	// map array of pages into virtual contiguous memory
	metrics = (struct smi_metrics *) vmap(pages, num_pages, VM_MAP, PAGE_KERNEL);
	metrics->num_metric = gpumon_metrics_table->num_metric;
	for (i = 0; i < gpumon_metrics_table->num_metric; i++) {
		metrics->metric[i].metric_union.code = gpumon_metrics_table->metric[i].code;
		metrics->metric[i].metric_union.metric.category = smi_map_metric_category(gpumon_metrics_table->metric[i].category);
		metrics->metric[i].metric_union.metric.name = smi_map_metric_name(gpumon_metrics_table->metric[i].name);
		metrics->metric[i].metric_union.metric.unit = smi_map_metric_unit(gpumon_metrics_table->metric[i].unit);
		metrics->metric[i].vf_mask = gpumon_metrics_table->metric[i].vf_mask;
		metrics->metric[i].val = gpumon_metrics_table->metric[i].val;
	}
	// Unmap and release mapped pages and
	// free the virtual contiguous memory
	vunmap(metrics);
	for (i = 0; i < num_pages; i++) {
		if (!PageReserved(pages[i])) {
			SetPageDirty(pages[i]);
		}
		put_page(pages[i]);
	}

	kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
	up_read(&current->mm->mmap_lock);
#else
	up_read(&current->mm->mmap_sem);
#endif

	return SMI_STATUS_SUCCESS;
}

static int gim_get_partition(struct smi_profile_configs *profile_configs,
				struct amdgv_gpumon_accelerator_partition_profile_config *caps)
{
	struct smi_accelerator_partition_profile_config *partition_profile_configs = NULL;
	long num_pages = 0;
	unsigned int gup_flags;
	struct mm_struct *mm;
	size_t buffer_size;
	unsigned long nr_pages;
	struct page **pages;
	uint32_t i;
	uint32_t j, k;

	mm = current->mm; // Get memory management structure

	// Calculate the number of pages needed to map the entire partition config
	buffer_size = sizeof(struct smi_accelerator_partition_profile_config);
	nr_pages = (buffer_size + PAGE_SIZE - 1) >> PAGE_SHIFT;

	pages = kmalloc(nr_pages * sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
	down_read(&current->mm->mmap_lock);
#else
	down_read(&current->mm->mmap_sem);
#endif
	gup_flags = FOLL_WRITE;
	// Map user space buffer containing accelerator partition config into kernel space
	// by creating memory pages
#if defined(HAVE_GET_USER_PAGES_REMOTE_8_ARG)
	num_pages = get_user_pages_remote(current, mm, (unsigned long)profile_configs->profile_configs, nr_pages, gup_flags, pages, NULL);
#elif defined(HAVE_GET_USER_PAGES_REMOTE_7_ARG)
	num_pages = get_user_pages_remote(mm, (unsigned long)profile_configs->profile_configs, nr_pages, gup_flags, pages, NULL, NULL);
#elif defined(HAVE_GET_USER_PAGES_REMOTE_6_ARG)
	num_pages = get_user_pages_remote(mm, (unsigned long)profile_configs->profile_configs, nr_pages, gup_flags, pages, NULL);
#endif
	if (num_pages <= 0) {
		kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
		up_read(&current->mm->mmap_lock);
#else
		up_read(&current->mm->mmap_sem);
#endif
		return SMI_STATUS_API_FAILED;
	}
	// Access mapped accelerator partition config in kernel space
	// map array of pages into virtual contiguous memory
	partition_profile_configs = (struct smi_accelerator_partition_profile_config *) vmap(pages, num_pages, VM_MAP, PAGE_KERNEL);

	partition_profile_configs->num_resource_profiles = caps->number_of_resource_profiles;
	partition_profile_configs->num_profiles = caps->number_of_profiles;
	partition_profile_configs->default_profile_index = caps->default_profile_index;

	for (i = 0; i < partition_profile_configs->num_resource_profiles; i++) {
		partition_profile_configs->resource_profiles[i].profile_index = caps->resource_profiles[i].resource_index;
		partition_profile_configs->resource_profiles[i].resource_type = smi_map_resource_type(caps->resource_profiles[i].resource_type);
		partition_profile_configs->resource_profiles[i].partition_resource = caps->resource_profiles[i].partition_resource;
		partition_profile_configs->resource_profiles[i].num_partitions_share_resource = caps->resource_profiles[i].num_partitions_share_resource;
	}

	for (i = 0; i < partition_profile_configs->num_profiles; i++) {
		partition_profile_configs->profiles[i].profile_type = smi_map_partition_type(caps->profiles[i].profile_type);
		partition_profile_configs->profiles[i].num_partitions = caps->profiles[i].num_partitions;
		partition_profile_configs->profiles[i].memory_caps.nps_cap_mask = caps->profiles[i].memory_caps.mp_cap_mask;
		partition_profile_configs->profiles[i].profile_index = caps->profiles[i].profile_index;
		partition_profile_configs->profiles[i].num_resources = caps->profiles[i].num_resources;
		for (j = 0; j < partition_profile_configs->profiles[i].num_partitions; j++) {
			for (k = 0; k < partition_profile_configs->profiles[i].num_resources; k++) {
				partition_profile_configs->profiles[i].resources[j][k] = caps->profiles[i].resources[j][k];
			}
		}
	};

	// Unmap and release mapped pages and
	// free the virtual contiguous memory
	vunmap(partition_profile_configs);
	for (i = 0; i < num_pages; i++) {
		if (!PageReserved(pages[i])) {
			SetPageDirty(pages[i]);
		}
		put_page(pages[i]);
	}


	kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
	up_read(&current->mm->mmap_lock);
#else
	up_read(&current->mm->mmap_sem);
#endif

	return SMI_STATUS_SUCCESS;
}


static int gim_get_eeprom_table(struct smi_bad_page_info *eeprom_table, uint16_t size, uint32_t bp_cnt,
				   struct amdgv_smi_ras_eeprom_table_record *gpumon_eeprom_table)
{
	struct smi_bad_page_record *bad_pages = NULL;
	long num_pages = 0;
	unsigned int gup_flags;
	struct mm_struct *mm;
	size_t buffer_size;
	unsigned long nr_pages;
	struct page **pages;
	uint32_t i = 0;

	mm = current->mm; // Get memory management structure

	// Calculate the number of pages needed to map the entire eeprom table
	buffer_size = sizeof(struct smi_bad_page_record);
	nr_pages = (buffer_size + PAGE_SIZE - 1) >> PAGE_SHIFT;

	pages = kmalloc(nr_pages * sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
	down_read(&current->mm->mmap_lock);
#else
	down_read(&current->mm->mmap_sem);
#endif
	gup_flags = FOLL_WRITE;
	// Map user space buffer containing eeprom table into kernel space
	// by creating memory pages
#if defined(HAVE_GET_USER_PAGES_REMOTE_6_ARG)
	num_pages = get_user_pages_remote(mm, (unsigned long)eeprom_table->bad_pages, nr_pages, gup_flags, pages, NULL);
#elif defined(HAVE_GET_USER_PAGES_REMOTE_7_ARG)
	num_pages = get_user_pages_remote(mm, (unsigned long)eeprom_table->bad_pages, nr_pages, gup_flags, pages, NULL, NULL);
#else
	num_pages = get_user_pages_remote(NULL, mm, (unsigned long)eeprom_table->bad_pages, nr_pages, gup_flags, pages, NULL, NULL);
#endif

	if (num_pages <= 0) {
		kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
		up_read(&current->mm->mmap_lock);
#else
		up_read(&current->mm->mmap_sem);
#endif
		return SMI_STATUS_API_FAILED;
	}
	// Access mapped eeprom table in kernel space
	// map array of pages into virtual contiguous memory
	bad_pages = (struct smi_bad_page_record *) vmap(pages, num_pages, VM_MAP, PAGE_KERNEL);
	bad_pages->num_bad_page = bp_cnt;
	for (i = 0; i < bp_cnt; i++) {
		bad_pages->bad_page[i].retired_page = gpumon_eeprom_table[i].retired_page;
		bad_pages->bad_page[i].ts = gpumon_eeprom_table[i].ts;
		bad_pages->bad_page[i].err_type = (unsigned char) gpumon_eeprom_table[i].err_type;
		bad_pages->bad_page[i].bank = gpumon_eeprom_table[i].bank;
		bad_pages->bad_page[i].cu = gpumon_eeprom_table[i].cu;
		bad_pages->bad_page[i].mem_channel = gpumon_eeprom_table[i].mem_channel;
		bad_pages->bad_page[i].mcumc_id = gpumon_eeprom_table[i].mcumc_id;
	}
	// Unmap and release mapped pages and
	// free the virtual contiguous memory
	vunmap(bad_pages);
	for (i = 0; i < num_pages; i++) {
		if (!PageReserved(pages[i])) {
			SetPageDirty(pages[i]);
		}
		put_page(pages[i]);
	}

	kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
	up_read(&current->mm->mmap_lock);
#else
	up_read(&current->mm->mmap_sem);
#endif

	return SMI_STATUS_SUCCESS;
}

static inline int gim_get_cper_data(struct smi_cper_config *cper_config, uint16_t in_len, uint64_t size, char* buffer, uint64_t write_count,
									uint32_t* smi_cper_hdrs)
{
	struct smi_cper *cper = NULL;
	long num_pages = 0;
	unsigned int gup_flags;
	struct mm_struct *mm;
	size_t buffer_size;
	unsigned long nr_pages;
	struct page **pages;
	uint32_t i = 0;

	mm = current->mm; // Get memory management structure

	// Calculate the number of pages needed to map the entire metrics table
	buffer_size = sizeof(struct smi_cper);
	nr_pages = (buffer_size + PAGE_SIZE - 1) >> PAGE_SHIFT;

	pages = kmalloc(nr_pages * sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
	down_read(&current->mm->mmap_lock);
#else
	down_read(&current->mm->mmap_sem);
#endif
	gup_flags = FOLL_WRITE;
	// Map user space buffer containing metrics table into kernel space
	// by creating memory pages
#if defined(HAVE_GET_USER_PAGES_REMOTE_6_ARG)
	num_pages = get_user_pages_remote(mm, (unsigned long)cper_config->cper, nr_pages, gup_flags, pages, NULL);
#elif defined(HAVE_GET_USER_PAGES_REMOTE_7_ARG)
	num_pages = get_user_pages_remote(mm, (unsigned long)cper_config->cper, nr_pages, gup_flags, pages, NULL, NULL);
#else
	num_pages = get_user_pages_remote(NULL, mm, (unsigned long)cper_config->cper, nr_pages, gup_flags, pages, NULL, NULL);
#endif
	if (num_pages <= 0) {
		kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
		up_read(&current->mm->mmap_lock);
#else
		up_read(&current->mm->mmap_sem);
#endif
		return SMI_STATUS_API_FAILED;
	}
	// Access mapped metrics table in kernel space
	// map array of pages into virtual contiguous memory
	cper = (struct smi_cper *) vmap(pages, num_pages, VM_MAP, PAGE_KERNEL);

	memcpy(cper->cper_data, buffer, size);
	cper->entry_count = write_count;
	cper->buf_size = size;
	for (i = 0; i < write_count; i++) {
		cper->cper_hdrs[i] = smi_cper_hdrs[i];
	}
	cper->cursor = write_count;
	// Unmap and release mapped pages and
	// free the virtual contiguous memory
	vunmap(cper);
	for (i = 0; i < num_pages; i++) {
		if (!PageReserved(pages[i])) {
			SetPageDirty(pages[i]);
		}
		put_page(pages[i]);
	}

	kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
	up_read(&current->mm->mmap_lock);
#else
	up_read(&current->mm->mmap_sem);
#endif

	return SMI_STATUS_SUCCESS;
}

struct smi_shim_interface gim_smi_interface = {
	.lock_device_list = gim_lock_device_list,
	.get_device_list = gim_get_device_list,
	.unlock_device_list = gim_unlock_device_list,
	.get_shim_log_level = gim_get_shim_log_level,
	.put_handle = gim_put_handle,

	.set_file_private_data = NULL,
	.get_file_private_data = NULL,
	.verify_file_descriptor = NULL,

	.generate_date_string = gim_generate_date_string,
	.create_event = gim_create_event,
	.read_event = gim_read_event,
	.destroy_event = gim_destroy_event,
	.get_pcie_confs = gim_get_pcie_confs,
	.get_device_data = gim_get_device_data,
	.create_hash_64 = gim_hash_64,

	.get_driver_version = gim_get_driver_version,
	.get_driver_id = gim_get_driver_id,
	.get_profile_info = gim_get_profile_info,
	.get_driver_date = gim_get_driver_date,
	.get_driver_model = gim_get_driver_model,
	.get_metric_table = gim_get_metric_table,
	.get_eeprom_table = gim_get_eeprom_table,
	.get_partition = gim_get_partition,
	.get_cper_data = gim_get_cper_data
};

/* Prototypes for device functions */
static int smi_lnx_drv_open(struct inode *, smi_process_handle);
static int smi_lnx_drv_release(struct inode *, smi_process_handle);
static long smi_lnx_ioctl_handler(smi_process_handle file, unsigned int cmd, unsigned long arg);

static const struct file_operations smi_file_ops = {
	.owner                  = THIS_MODULE,
	.unlocked_ioctl         = smi_lnx_ioctl_handler,
#ifdef CONFIG_COMPAT
	.compat_ioctl           = smi_lnx_ioctl_handler,
#endif
	.open                   = smi_lnx_drv_open,
	.release                = smi_lnx_drv_release,
};

/* smi device file interface */
static int smi_set_file_private_data(file_t filp, struct smi_ctx *ctx)
{
	smi_process_handle file = (smi_process_handle) filp;
	file->private_data = ctx;

	return 0;
}

static int smi_get_file_private_data(file_t filp, struct smi_ctx **ctx)
{
	smi_process_handle file = (smi_process_handle) filp;
	*ctx = file->private_data;

	return 0;
}

static int smi_verify_file_descriptor(file_t filp)
{
	smi_process_handle file = (smi_process_handle) filp;
	return file->f_op != &smi_file_ops;
}

int smi_init(struct oss_interface *oss_interface,
		struct smi_shim_interface *shim_interface)
{
	int res = 0;

	res = alloc_chrdev_region(&devid, 0, MINOR_COUNT, SMI_DEVICE_NAME);
	if (res < 0)
		goto out;
#if !defined(HAVE_CLASS_CREATE_1_ARG)
	smi_class = class_create(THIS_MODULE, SMI_DEVICE_NAME);
#else
	smi_class = class_create(SMI_DEVICE_NAME);
#endif
	if (IS_ERR(smi_class)) {
		res = PTR_ERR(smi_class);
		goto err_cdev;
	}

	smi_dev = device_create(smi_class, NULL, MKDEV(MAJOR(devid), 0), NULL,
				SMI_DEVICE_NAME "%d", 0);

	cdev_init(&smi_cdev, &smi_file_ops);
	res = cdev_add(&smi_cdev, devid, 1);
	if (res < 0)
		goto err_class;

	kobject_uevent(&smi_cdev.kobj, KOBJ_ADD);

	/* pass oss function to smi core */
	shim_interface->set_file_private_data  = smi_set_file_private_data;
	shim_interface->get_file_private_data  = smi_get_file_private_data;
	shim_interface->verify_file_descriptor = smi_verify_file_descriptor;
	res = smi_core_init(oss_interface, shim_interface);
	if (res < 0)
		goto err_class;

	goto out;
err_class:
	cdev_del(&smi_cdev);
	device_destroy(smi_class, smi_dev->devt);
	class_destroy(smi_class);

err_cdev:
	unregister_chrdev_region(devid, MINOR_COUNT);
out:
	return res;
}

void smi_cleanup(void)
{
	kobject_uevent(&smi_cdev.kobj, KOBJ_REMOVE);
	cdev_del(&smi_cdev);
	device_destroy(smi_class, smi_dev->devt);
	class_destroy(smi_class);
	unregister_chrdev_region(devid, MINOR_COUNT);

	smi_core_fini();
};

static int smi_lnx_drv_open(struct inode *inode, smi_process_handle file)
{
	return smi_core_open((file_t) file, (file->f_mode & FMODE_WRITE) == FMODE_WRITE);
}

static int smi_lnx_drv_release(struct inode *inode, smi_process_handle file)
{
	return smi_core_release((file_t) file);
}

static long smi_lnx_ioctl_handler(smi_process_handle file, unsigned int cmd, unsigned long arg)
{
	if (cmd == SMI_IOCTL_COMMAND)
		return smi_core_ioctl_handler((file_t)file, cmd, (void *) arg);

	return -SMI_EINVAL;
}

int smi_open(smi_process_handle file, bool is_privileged)
{
	return smi_core_open((file_t) file, is_privileged);
}

int smi_release(smi_process_handle file)
{
	return smi_core_release((file_t) file);
}

long smi_ioctl_handler(smi_process_handle file, unsigned int cmd, void *arg)
{
	if (cmd == SMI_IOCTL_COMMAND)
		return smi_core_ioctl_handler((file_t)file, cmd, arg);

	return -SMI_EINVAL;
}
