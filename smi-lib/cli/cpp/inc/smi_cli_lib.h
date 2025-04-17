/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <windows.h>
#include <sysinfoapi.h>

#include "amdsmi.h"

typedef amdsmi_status_t (*AMDSMI_INIT)(uint64_t);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_vf_handle_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ASIC_INFO)(amdsmi_processor_handle,
		amdsmi_asic_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VRAM_INFO)(amdsmi_processor_handle,
		amdsmi_vram_info_t *);
typedef amdsmi_status_t (*AMDSMI_SHUT_DOWN)(void);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_UUID)(amdsmi_processor_handle, unsigned int *,
		char *);

typedef amdsmi_status_t (*AMDSMI_GET_VF_BDF)(amdsmi_vf_handle_t, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_UUID)(amdsmi_vf_handle_t, unsigned int *, char *);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_INFO)(amdsmi_processor_handle,
		amdsmi_driver_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_CAP_INFO)(amdsmi_processor_handle, uint32_t,
		amdsmi_power_cap_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_INFO)(amdsmi_processor_handle, amdsmi_xgmi_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_FB_LAYOUT)(amdsmi_processor_handle,
		amdsmi_pf_fb_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VBIOS_INFO)(amdsmi_processor_handle,
		amdsmi_vbios_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_BOARD_INFO)(amdsmi_processor_handle,
		amdsmi_board_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_FW_INFO)(amdsmi_processor_handle, amdsmi_fw_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_FW_ERROR_RECORDS)(amdsmi_processor_handle,
		amdsmi_fw_error_record_t *);
typedef amdsmi_status_t (*AMDSMI_GET_DFC_FW_TABLE)(amdsmi_processor_handle, amdsmi_dfc_fw_t *);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
		amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle, uint32_t,
		amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_IS_GPU_POWER_MANAGEMENT_ENABLED)(amdsmi_processor_handle, bool *);
typedef amdsmi_status_t (*AMDSMI_GET_CLOCK_INFO)(amdsmi_processor_handle, amdsmi_clk_type_t,
		amdsmi_clk_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
		amdsmi_temperature_type_t,
		amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_CACHE_INFO)(amdsmi_processor_handle,
		amdsmi_gpu_cache_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
		amdsmi_error_count_t *);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_COUNT)(amdsmi_processor_handle, amdsmi_gpu_block_t,
		amdsmi_error_count_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_ENABLED)(amdsmi_processor_handle,
		uint64_t *);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_BAD_PAGE_INFO)(amdsmi_processor_handle,
		unsigned int array_length, uint32_t *,
		amdsmi_eeprom_table_record_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_FEATURE_INFO)(amdsmi_processor_handle,
		amdsmi_ras_feature_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint32_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_PARTITION_INFO)(amdsmi_processor_handle, unsigned int,
		amdsmi_partition_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_INFO)(amdsmi_vf_handle_t, amdsmi_vf_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_DATA)(amdsmi_vf_handle_t, amdsmi_vf_data_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GUEST_DATA)(amdsmi_vf_handle_t,
		amdsmi_guest_data_t *);
typedef amdsmi_status_t (*AMDSMI_GET_VF_FW_INFO)(amdsmi_vf_handle_t, amdsmi_fw_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PARTITION_PROFILE_INFO)(amdsmi_processor_handle,
		amdsmi_profile_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_TOPOLOGY)(amdsmi_processor_handle,
		amdsmi_processor_handle,
		amdsmi_link_topology_t *);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_METRICS)(amdsmi_processor_handle,
		amdsmi_link_metrics_t *);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_FB_SHARING_CAPS)(amdsmi_processor_handle,
		amdsmi_xgmi_fb_sharing_caps_t *);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO)(amdsmi_processor_handle,
		amdsmi_processor_handle,
		amdsmi_xgmi_fb_sharing_mode_t, uint8_t *);
typedef amdsmi_status_t (*AMDSMI_SET_XGMI_FB_SHARING_MODE_V2)(amdsmi_processor_handle, uint32_t,
		amdsmi_xgmi_fb_sharing_mode_t);
typedef amdsmi_status_t (*AMDSMI_GPU_GET_CPER_ENTRIES)(amdsmi_processor_handle, uint32_t, char*,
		uint64_t *,
		amdsmi_cper_hdr**, uint64_t *, uint64_t *);

typedef amdsmi_status_t (*AMDSMI_EVENT_CREATE)(amdsmi_processor_handle *, uint32_t,
		uint64_t, amdsmi_event_set *);
typedef amdsmi_status_t (*AMDSMI_EVENT_READ)(amdsmi_event_set, int64_t, amdsmi_event_entry_t *);
typedef amdsmi_status_t (*AMDSMI_EVENT_DESTROY)(amdsmi_event_set);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_METRICS)(amdsmi_get_gpu_metrics);

typedef amdsmi_status_t (*AMDSMI_GET_LIB_VERSION)(amdsmi_get_lib_version);

class AmdSmiLibHost
{
private:
	/**
	 * @brief Load dynamic-link library module
	 *
	 */
	void loadAmdSmiLib();

	/**
	 * @brief Construct a new Amd Smi Lib Host object
	 *
	 */
	AmdSmiLibHost();

	/**
	 * @brief Construct a new Amd Smi Lib Host object - delete
	 *
	 * @param obj
	 */
	AmdSmiLibHost(const AmdSmiLibHost &obj) = delete;

public:
	AMDSMI_INIT amdsmi_init;
	AMDSMI_GET_PROCESSOR_HANDLES amdsmi_get_processor_handles;
	AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF amdsmi_get_processor_handle_from_bdf;
	AMDSMI_GET_VF_HANDLE_FROM_BDF amdsmi_get_vf_handle_from_bdf;
	AMDSMI_GET_GPU_ASIC_INFO amdsmi_get_gpu_asic_info;
	AMDSMI_GET_GPU_VRAM_INFO amdsmi_get_gpu_vram_info;
	AMDSMI_SHUT_DOWN amdsmi_shut_down;
	AMDSMI_GET_GPU_DEVICE_BDF amdsmi_get_gpu_device_bdf;
	AMDSMI_GET_GPU_DEVICE_UUID amdsmi_get_gpu_device_uuid;
	AMDSMI_GET_VF_BDF amdsmi_get_vf_bdf;
	AMDSMI_GET_VF_UUID amdsmi_get_vf_uuid;
	AMDSMI_GET_GPU_DRIVER_INFO amdsmi_get_gpu_driver_info;
	AMDSMI_GET_POWER_CAP_INFO amdsmi_get_power_cap_info;
	AMDSMI_GET_PCIE_INFO amdsmi_get_pcie_info;
	AMDSMI_GET_XGMI_INFO amdsmi_get_xgmi_info;
	AMDSMI_GET_FB_LAYOUT amdsmi_get_fb_layout;
	AMDSMI_GET_GPU_VBIOS_INFO amdsmi_get_gpu_vbios_info;
	AMDSMI_GET_GPU_BOARD_INFO amdsmi_get_gpu_board_info;
	AMDSMI_GET_FW_INFO amdsmi_get_fw_info;
	AMDSMI_GET_FW_ERROR_RECORDS amdsmi_get_fw_error_records;
	AMDSMI_GET_DFC_FW_TABLE amdsmi_get_dfc_fw_table;
	AMDSMI_GET_GPU_ACTIVITY amdsmi_get_gpu_activity;
	AMDSMI_GET_POWER_INFO amdsmi_get_power_info;
	AMDSMI_IS_GPU_POWER_MANAGEMENT_ENABLED amdsmi_is_gpu_power_management_enabled;
	AMDSMI_GET_CLOCK_INFO amdsmi_get_clock_info;
	AMDSMI_GET_TEMP_METRIC amdsmi_get_temp_metric;
	AMDSMI_GET_GPU_CACHE_INFO amdsmi_get_gpu_cache_info;
	AMDSMI_GET_GPU_TOTAL_ECC_COUNT amdsmi_get_gpu_total_ecc_count;

	AMDSMI_GET_GPU_ECC_COUNT amdsmi_get_gpu_ecc_count;
	AMDSMI_GET_GPU_ECC_ENABLED amdsmi_get_gpu_ecc_enabled;

	AMDSMI_GET_GPU_BAD_PAGE_INFO amdsmi_get_gpu_bad_page_info;
	AMDSMI_GET_GPU_RAS_FEATURE_INFO amdsmi_get_gpu_ras_feature_info;
	AMDSMI_GET_NUM_VF amdsmi_get_num_vf;
	AMDSMI_GET_VF_PARTITION_INFO amdsmi_get_vf_partition_info;
	AMDSMI_GET_VF_INFO amdsmi_get_vf_info;
	AMDSMI_GET_VF_DATA amdsmi_get_vf_data;
	AMDSMI_GET_GUEST_DATA amdsmi_get_guest_data;
	AMDSMI_GET_VF_FW_INFO amdsmi_get_vf_fw_info;
	AMDSMI_GET_PARTITION_PROFILE_INFO amdsmi_get_partition_profile_info;
	AMDSMI_GET_LINK_TOPOLOGY amdsmi_get_link_topology;
	AMDSMI_GET_LINK_METRICS amdsmi_get_link_metrics;
	AMDSMI_GET_XGMI_FB_SHARING_CAPS amdsmi_get_xgmi_fb_sharing_caps;
	AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO amdsmi_get_xgmi_fb_sharing_mode_info;
	AMDSMI_SET_XGMI_FB_SHARING_MODE_V2 amdsmi_set_xgmi_fb_sharing_mode_v2;
	AMDSMI_EVENT_CREATE amdsmi_event_create;
	AMDSMI_EVENT_READ amdsmi_event_read;
	AMDSMI_EVENT_DESTROY amdsmi_event_destroy;

	AMDSMI_GET_GPU_METRICS amdsmi_get_gpu_metrics;

	HMODULE amdSmiDll;

	~AmdSmiLibHost();

	/**
	 * @brief Frees the loaded dynamic-link library module
	 *
	 */
	void FreeLib();

	/**
	 * @brief Get the Instance object
	 *
	 * @return AmdSmiLibHost&
	 */
	static AmdSmiLibHost &getInstance()
	{
		static AmdSmiLibHost instance;
		return instance;
	}
};
