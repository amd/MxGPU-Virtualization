/*
 * Copyright (c) 2014-2019 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _GPU_IOV_MODULE__CONFIG_H
#define _GPU_IOV_MODULE__CONFIG_H

#include "amdgv_api.h"

/* GIM config file */
#define GIM_CONFIG_PATH "/etc/gim_config"

/* VF number Options */
#define VF_NUMBER__KEY "vf_num"
#define VF_NUMBER__START                1
#define VF_NUMBER__DEFAULT              1
#define VF_NUMBER__MAX                  31

#define SKIP_CHECK_BGPU__KEY "skip_check_bgpu"
#define SKIP_CHECK_BGPU__START           0
#define SKIP_CHECK_BGPU__DEFAULT         0
#define SKIP_CHECK_BGPU__MAX             2

#define GUARD__KEY "guard"
#define GUARD__START                0
#define GUARD__DEFAULT              1
#define GUARD__MAX                  1

#define FULLACCESS_TIMEOUT__KEY "fullaccess_timeout"
#define FULLACCESS_TIMEOUT__START                0
#define FULLACCESS_TIMEOUT__DEFAULT              0
#define FULLACCESS_TIMEOUT__MAX                  500000

#define SCH_POLICY__KEY "sch_policy"
#define SCH_POLICY__START	(AMDGV_SCHED_BEGIN + 1)
#define SCH_POLICY__DEFAULT	AMDGV_SCHED_BEGIN

#define SCH_POLICY__MAX		(AMDGV_SCHED_END - 1)

#define FW_LOAD_TYPE__KEY "fw_load_type"
#define FW_LOAD_TYPE__START	(AMDGV_FW_LOAD_TYPE_BEGIN + 1)
#define FW_LOAD_TYPE__DEFAULT	AMDGV_FW_LOAD_BY_VBIOS
#define FW_LOAD_TYPE__MAX	(AMDGV_FW_LOAD_TYPE_END - 1)

#define LOG_LEVEL__KEY "log_level"
#define LOG_LEVEL__START	AMDGV_ERROR_LEVEL
#define LOG_LEVEL__DEFAULT	AMDGV_INFO_LEVEL
#define LOG_LEVEL__MAX		AMDGV_DEBUG_LEVEL4

#define POWER_SAVING_MODE__KEY "power_saving_mode"
#define POWER_SAVING_MODE__START	AMDGV_IPS_POWER_SAVING_DISABLE
#define POWER_SAVING_MODE__DEFAULT	AMDGV_IPS_POWER_SAVING_DISABLE
#define POWER_SAVING_MODE__MAX		AMDGV_IPS_POWER_SAVING_MANUAL

#define PERF_MON_ENABLE__KEY "perf_mon_enable"
#define PERF_MON_ENABLE__START		0
#define PERF_MON_ENABLE__DEFAULT	0
#define PERF_MON_ENABLE__MAX		1

#define MM_POLICY__KEY          "mm_policy"
#define MM_POLICY__START		AMDGV_MM_ALL_ASSIGNMENT_FOR_ONE_VF
#define MM_POLICY__DEFAULT	    AMDGV_MM_ALL_ASSIGNMENT_FOR_ONE_VF
#define MM_POLICY__MAX		    AMDGV_MM_ONE_ASSIGNMENT_FOR_ONE_VF

#define FB_SHARING_MODE__KEY 		"fb_sharing_mode"
#define FB_SHARING_MODE__START		AMDGV_XGMI_FB_SHARING_MODE_DEFAULT
#define FB_SHARING_MODE__DEFAULT	AMDGV_XGMI_FB_SHARING_MODE_DEFAULT
#define FB_SHARING_MODE__MAX		AMDGV_XGMI_FB_SHARING_MODE_8

#define ACCELERATOR_PARTITION__KEY     "accelerator_partition_mode"
#define ACCELERATOR_PARTITION__START   AMDGV_ACCELERATOR_PARTITION_MODE_UNKNOWN
#define ACCELERATOR_PARTITION__DEFAULT AMDGV_ACCELERATOR_PARTITION_MODE_UNKNOWN
#define ACCELERATOR_PARTITION__MAX     AMDGV_ACCELERATOR_PARTITION_MODE_MAX

#define MEMORY_PARTITION_MODE__KEY     "memory_partition_mode"
#define MEMORY_PARTITION_MODE__START   AMDGV_MEMORY_PARTITION_MODE_UNKNOWN
#define MEMORY_PARTITION_MODE__DEFAULT AMDGV_MEMORY_PARTITION_MODE_UNKNOWN
#define MEMORY_PARTITION_MODE__MAX     AMDGV_MEMORY_PARTITION_MODE_MAX

#define PARTITION_FULL_ACCESS_ENABLE__KEY                 "per_partition_fullaccess_enable"
#define PARTITION_FULL_ACCESS_ENABLE__START               0
#define PARTITION_FULL_ACCESS_ENABLE__DEFAULT             1
#define PARTITION_FULL_ACCESS_ENABLE__MAX                 1

#define HANG_DETECTION_MODE__KEY     "hang_detection_mode"
#define HANG_DETECTION_MODE__START   AMDGV_HANG_DETECTION_DISABLED
#define HANG_DETECTION_MODE__DEFAULT AMDGV_HANG_DETECTION_ENABLED
#define HANG_DETECTION_MODE__MAX     AMDGV_HANG_DETECTION_MODE_MAX

#define ASYMMETRIC_FB_MODE__KEY 		    "asymmetric_fb_mode"
#define ASYMMETRIC_FB_MODE__START		    AMDGV_ASYMMETRIC_FB_DISABLED
#define ASYMMETRIC_FB_MODE__DEFAULT	        AMDGV_ASYMMETRIC_FB_ENABLED
#define ASYMMETRIC_FB_MODE__MAX		        AMDGV_ASYMMETRIC_FB_MODE_MAX

#define HANG_DUMP_TIMEOUT__KEY "hangdump_timeout"
#define HANG_DUMP_TIMEOUT__START                0
#define HANG_DUMP_TIMEOUT__DEFAULT              10000
#define HANG_DUMP__MAX                          60000

#define DEBUG_DUMP_RESERVE_SIZE__KEY "debug_dump_reserve_size"
#define DEBUG_DUMP_RESERVE_SIZE__START          0
#define DEBUG_DUMP_RESERVE_SIZE__DEFAULT        0
#define DEBUG_DUMP_RESERVE_SIZE__MAX            256

#define DEFERRED_FULL_LIVE_UPDATE__KEY "deferred_full_live_update"
#define DEFERRED_FULL_LIVE_UPDATE__START       0
#define DEFERRED_FULL_LIVE_UPDATE__DEFAULT     0
#define DEFERRED_FULL_LIVE_UPDATE__MAX         1

#define BP_MODE__KEY        "bp_mode"
#define BP_MODE__START      0
#define BP_MODE__DEFAULT    0
#define BP_MODE__MAX        2

#define PF_FB_SIZE__KEY      "pf_fb_size"
#define PF_FB_SIZE__START    256
#define PF_FB_SIZE__DEFAULT  0 // 0 indicates adapter will use the default size defined in mem_sw_init accordingly
#define PF_FB_SIZE__MAX      1024

#define BAD_PAGE_RECORD_THRESHOLD__KEY      "bad_page_record_threshold"
#define BAD_PAGE_RECORD_THRESHOLD__START    10
#define BAD_PAGE_RECORD_THRESHOLD__DEFAULT  0 // 0 indicates adapter will use default bad page threshold defined in ecc_sw_init
#define BAD_PAGE_RECORD_THRESHOLD__MAX      256

#define RAS_TELEMETRY_POLICY__KEY     "ras_vf_telemetry_policy"
#define RAS_TELEMETRY_POLICY__START   AMDGV_RAS_VF_TELEMETRY_LOG_ON_GUEST_LOAD
#define RAS_TELEMETRY_POLICY__DEFAULT AMDGV_RAS_VF_TELEMETRY_LOG_ON_GUEST_LOAD
#define RAS_TELEMETRY_POLICY__MAX     AMDGV_RAS_VF_TELEMETRY_POLICY_COUNT

#define MAX_CPER_COUNT__KEY     "max_cper_count"
#define MAX_CPER_COUNT__START   -1
#define MAX_CPER_COUNT__DEFAULT 0
#define MAX_CPER_COUNT__MAX     AMDGV_CPER_MAX_ALLOWED_COUNT

enum gim_conf_opt_idx {
	CONF_OPT_START = 0,
	CONF_OPT_VF_NUMBER = CONF_OPT_START,
	CONF_OPT_FW_LOAD_TYPE,
	CONF_OPT_LOG_LEVEL,
	CONF_OPT_GUARD,
	CONF_OPT_SCH_POLICY,
	CONF_OPT_FULLACCESS_TIMEOUT,
	CONF_OPT_PERF_MON_EN,
	CONF_OPT_SKIP_CHECK_BGPU,
	CONF_OPT_POWER_SAVING_MODE,
	CONF_OPT_MM_POLICY,
	CONF_OPT_HANG_DUMP_TIMEOUT,
	CONF_OPT_FB_SHARING_MODE,
	CONF_OPT_ACCELERATOR_PARTITION_MODE,
	CONF_OPT_MEMORY_PARTITION_MODE,
	CONF_OPT_PARTITION_FULL_ACCESS_EN,
	CONF_OPT_DEBUG_DUMP_RESERVE_SIZE,
	CONF_OPT_HANG_DETECTION_MODE,
	CONF_OPT_DEFERRED_FULL_LIVE_UPDATE,
	CONF_OPT_ASYMMETRIC_FB_MODE,
	CONF_OPT_BP_MODE,
	CONF_OPT_PF_FB_SIZE,
	CONF_OPT_BAD_PAGE_RECORD_THRESHOLD,
	CONF_OPT_RAS_VF_TELEMETRY_POLICY,
	CONF_OPT_MAX_CPER_COUNT,
	CONF_OPT_MAX
};

int gim_conf_init(void);

uint32_t gim_conf_get_vf_num_opt(uint32_t id);
uint32_t gim_conf_get_skip_check_bgpu_opt(uint32_t id);
uint32_t gim_conf_get_guard_opt(uint32_t id);
uint32_t gim_conf_get_fullacces_timeout_opt(uint32_t id);
uint32_t gim_conf_get_sch_policy_opt(uint32_t id);
uint32_t gim_conf_get_fw_load_type_opt(uint32_t id);
uint32_t gim_conf_get_log_level_opt(uint32_t id);
uint32_t gim_conf_get_mm_policy_opt(uint32_t id);
uint32_t gim_conf_get_perf_mon_enable_opt(uint32_t id);
uint32_t gim_conf_get_hangdump_timeout_opt(uint32_t id);
uint32_t gim_conf_get_fb_sharing_mode_opt(uint32_t id);
uint32_t gim_conf_get_accelerator_partition_mode_opt(uint32_t id);
uint32_t gim_conf_get_memory_partition_mode_opt(uint32_t id);
uint32_t gim_conf_get_partition_full_access_enable_opt(uint32_t id);
uint32_t gim_conf_get_hang_detection_mode_opt(uint32_t id);
uint32_t gim_conf_get_asymmetric_fb_mode_opt(uint32_t id);
uint32_t gim_conf_get_debug_dump_reserve_size_opt(uint32_t id);
uint32_t gim_conf_get_bad_page_record_threshold_opt(uint32_t id);
uint32_t gim_conf_set_vf_num_opt(int value);
uint32_t gim_conf_set_accelerator_partition_mode_opt(int value);
uint32_t gim_conf_set_memory_partition_mode_opt(int value);
uint32_t gim_conf_get_ras_vf_telemetry_policy_opt(uint32_t id);
uint32_t gim_conf_get_max_cper_count_opt(uint32_t id);
uint32_t gim_conf_set_opt(int index, int value);
uint32_t gim_conf_clear_conf_file(void);
int gim_conf_save(void);

extern char *gim_enabled_devices;

extern uint power_saving_mode;

uint32_t gim_conf_get_deferred_full_live_update_opt(uint32_t id);

uint32_t gim_conf_get_bp_mode_opt(uint32_t id);

uint32_t gim_conf_get_pf_fb_size_opt(uint32_t id);
#endif
