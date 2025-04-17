/*
 * Copyright (c) 2020-2022 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE.
 */

#ifndef __SMI_DRV_OSS_H__
#define __SMI_DRV_OSS_H__

#include "smi_drv_types.h"
#include "smi_cmd.h"

typedef void *mutex_t;
typedef void (*func_t)(void);

struct smi_device_data;
struct smi_ctx;
typedef void *amdgv_dev_t;

struct gim_dev_data;
struct smi_event_set_config;
struct smi_metrics_table;
struct smi_profile_configs;
struct smi_bad_page_info;
struct amdgv_gpumon_metrics_ext;
struct amdgv_smi_ras_eeprom_table_record;
struct amdgv_gpumon_accelerator_partition_profile_config;
struct smi_compute_partition_profile;
struct smi_profile_configs;
struct smi_event_entry;
enum smi_driver_model_type;
struct smi_cper_config;

/* smi to shim_driver api */
struct smi_shim_interface {
	/* shim driver device list interface */
	void (*lock_device_list)(void);
	void (*get_device_list)(struct smi_device_data *dev_list, int *size);
	void (*unlock_device_list)(void);
	int (*get_shim_log_level)(void);

	/* smi device file interface */
	int (*set_file_private_data)(file_t filp, struct smi_ctx *ctx);
	int (*get_file_private_data)(file_t filp, struct smi_ctx **ctx);
	int (*verify_file_descriptor)(file_t filp);

	void (*generate_date_string)(char *buf, uint64_t ktime);
	int (*create_event)(struct smi_ctx *smi, amdgv_dev_t *adev, struct smi_event_set_config *config);
	int (*read_event)(struct smi_ctx *smi, amdgv_dev_t *adev, uint64_t dev_id, struct smi_event_entry *event);
	int (*destroy_event)(struct smi_ctx *smi, amdgv_dev_t *adev, uint64_t dev_id);
	void (*put_handle)(amdgv_dev_t adev, struct smi_ctx *ctx);
	int (*get_pcie_confs)(amdgv_dev_t dev, int *speed, int *width, int *max_vf);
	void (*get_device_data)(amdgv_dev_t adev, struct smi_device_data *ret_dev_data, bool *dev_busy, struct smi_ctx *ctx);

	unsigned int (*create_hash_64)(uint64_t a, unsigned int bits);

	int (*get_driver_version)(amdgv_dev_t adev, uint32_t *length, char *version);
	int (*get_driver_id)(uint32_t *driver_id);
	int (*get_profile_info)(amdgv_dev_t dev, struct smi_profile_info *profile_info);
	int (*get_driver_date)(amdgv_dev_t adev, char *driver_date);
	int (*get_driver_model)(amdgv_dev_t adev, enum smi_driver_model_type *driver_model);
	int (*get_metric_table)(struct smi_metrics_table *metrics_table, uint16_t size, struct amdgv_gpumon_metrics_ext *gpumon_metrics_table);
	int (*get_eeprom_table)(struct smi_bad_page_info *eeprom_table, uint16_t size, uint32_t bp_cnt, struct amdgv_smi_ras_eeprom_table_record *gpumon_eeprom_table);
	int (*get_partition)(struct smi_profile_configs *profile_configs, struct amdgv_gpumon_accelerator_partition_profile_config *caps);
	int (*get_cper_data)(struct smi_cper_config *cper_config, uint16_t in_len, uint64_t size, char* buffer, uint64_t write_count,
						uint32_t* smi_cper_hdrs);
};

#define	SMI_EPERM		 1	/* Operation not permitted */
#define	SMI_ENOENT		 2	/* No such file or directory */
#define	SMI_EIO		 	 5	/* I/O error */
#define	SMI_ENOMEM		12	/* Out of memory */
#define	SMI_EACCES		13	/* Permission denied */
#define	SMI_EFAULT		14	/* Bad address */
#define	SMI_EINVAL		22	/* Invalid argument */

#endif // __SMI_DRV_OSS_H__

