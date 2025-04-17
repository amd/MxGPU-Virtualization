/*
 * Copyright (c) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __SMI_DRV_OSS_WRAPPER_H__
#define __SMI_DRV_OSS_WRAPPER_H__

#include "smi_drv_oss.h"

extern struct smi_shim_interface *smi_shim_funcs;

/* shim driver device list interface */
static inline void smi_lock_device_list(void)
{
    smi_shim_funcs->lock_device_list();
}

static inline void smi_get_device_list(struct smi_device_data *dev_list, int *size)
{
    smi_shim_funcs->get_device_list(dev_list, size);
}

static inline void smi_unlock_device_list(void)
{
    smi_shim_funcs->unlock_device_list();
}

static inline int smi_get_shim_log_level(void)
{
    return smi_shim_funcs->get_shim_log_level();
}

static inline void smi_get_device_data(amdgv_dev_t adev, struct smi_device_data *ret_dev_data, bool *dev_busy, struct smi_ctx *ctx)
{
    smi_shim_funcs->get_device_data(adev, ret_dev_data, dev_busy, ctx);
}

static inline void put_handle(amdgv_dev_t adev, struct smi_ctx *ctx)
{
    smi_shim_funcs->put_handle(adev, ctx);
}

static inline void smi_get_pcie_confs(amdgv_dev_t dev, int *speed, int *width, int *max_vf_num)
{
    smi_shim_funcs->get_pcie_confs(dev, speed, width, max_vf_num);
}

/* smi device file interface */
static inline int smi_set_file_private_data(file_t filp, struct smi_ctx *ctx)
{
    return smi_shim_funcs->set_file_private_data(filp, ctx);
}

static inline int smi_get_file_private_data(file_t filp, struct smi_ctx **ctx)
{
    return smi_shim_funcs->get_file_private_data(filp, ctx);
}

static inline int smi_verify_file_descriptor(file_t filp)
{
    return smi_shim_funcs->verify_file_descriptor(filp);
}

static inline void smi_generate_date_string(char *buf, uint64_t ktime)
{
    smi_shim_funcs->generate_date_string(buf, ktime);
}

static inline int smi_event_create(struct smi_ctx *smi, amdgv_dev_t *adev, struct smi_event_set_config *config)
{
    return smi_shim_funcs->create_event(smi, adev, config);
}

static inline int smi_event_read(struct smi_ctx *smi, amdgv_dev_t *adev, uint64_t dev_id, struct smi_event_entry *event)
{
    return smi_shim_funcs->read_event(smi, adev, dev_id, event);
}

static inline int smi_event_destroy(struct smi_ctx *smi, amdgv_dev_t *adev, uint64_t dev_id)
{
    return smi_shim_funcs->destroy_event(smi, adev, dev_id);
}

static inline unsigned int smi_hash_64(uint64_t a, unsigned int bits)
{
    return smi_shim_funcs->create_hash_64(a, bits);
}

static inline int smi_get_driver_version(amdgv_dev_t adev, uint32_t *length, char *version)
{
    return smi_shim_funcs->get_driver_version(adev, length, version);
}

static inline int smi_get_driver_id(uint32_t *driver_id)
{
    return smi_shim_funcs->get_driver_id(driver_id);
}

static inline int smi_get_profile_info(amdgv_dev_t dev, struct smi_profile_info *profiles)
{
    return smi_shim_funcs->get_profile_info(dev, profiles);
}

static inline int smi_get_driver_date(amdgv_dev_t adev, char *driver_date)
{
    return smi_shim_funcs->get_driver_date(adev, driver_date);
}

static inline int smi_get_driver_model(amdgv_dev_t adev, enum smi_driver_model_type *driver_model)
{
    return smi_shim_funcs->get_driver_model(adev, driver_model);
}

static inline int smi_get_metric_table(struct smi_metrics_table *metrics_table, uint16_t size, struct amdgv_gpumon_metrics_ext *gpumon_metrics_table)
{
    return smi_shim_funcs->get_metric_table(metrics_table, size, gpumon_metrics_table);
}

static inline int smi_get_eeprom_table(struct smi_bad_page_info *eeprom_table, uint16_t size, uint32_t bp_cnt, struct amdgv_smi_ras_eeprom_table_record *gpumon_eeprom_table)
{
    return smi_shim_funcs->get_eeprom_table(eeprom_table, size, bp_cnt, gpumon_eeprom_table);
}

static inline int smi_get_partition(struct smi_profile_configs *profile_configs, struct amdgv_gpumon_accelerator_partition_profile_config *caps)
{
    return smi_shim_funcs->get_partition(profile_configs, caps);
}

static inline int smi_get_cper_data(struct smi_cper_config *cper_config, uint16_t in_len, uint64_t size, char* buffer, uint64_t write_count,
                                    uint32_t* smi_cper_hdrs)
{
    return smi_shim_funcs->get_cper_data(cper_config, in_len, size, buffer, write_count, smi_cper_hdrs);
}

#endif // __SMI_DRV_OSS_WRAPPER_H__

