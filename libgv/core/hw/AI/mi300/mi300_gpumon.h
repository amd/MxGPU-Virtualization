/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_GPUMON_H
#define MI300_GPUMON_H

int mi300_get_asic_temperature_mem(struct amdgv_adapter *adapt, int *val);
int mi300_get_asic_temperature_hotspot(struct amdgv_adapter *adapt, int *val);
int mi300_get_asic_temperature_plx(struct amdgv_adapter *adapt, int *val);
int mi300_get_dpm(struct amdgv_adapter *adapt, int *val);
int mi300_get_fan_speed(struct amdgv_adapter *adapt, int *val);
int mi300_get_mm_activity(struct amdgv_adapter *adapt, int *val);
int mi300_get_volt_soc(struct amdgv_adapter *adapt, int *val);
int mi300_get_volt_mem(struct amdgv_adapter *adapt, int *val);

/* PTL (Peak TOPS Limiter) functions */
int mi300_gpumon_ptl_enable(struct amdgv_adapter *adapt,
			    struct amdgv_ptl_enable_info *info);

#endif