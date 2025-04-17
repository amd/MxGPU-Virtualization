/* * Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "amdsmi.h"
#include "smi_cli_api_host.h"
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"

#include <iostream>
#include <sstream>

#ifdef _WIN64
#include <windows.h>
#include <sysinfoapi.h>
#elif __linux__
#include <dlfcn.h>
#endif

#ifdef _WIN64
#define LOAD_SYM(a_amdSmiLibHandle, a_name) GetProcAddress((HMODULE)a_amdSmiLibHandle, a_name)
#elif __linux__
#define LOAD_SYM(a_amdSmiLibHandle, a_name) dlsym(a_amdSmiLibHandle, a_name)
#endif

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

// typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_VERSION)(amdsmi_processor_handle, int *,
// 							 char *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_INFO)(amdsmi_processor_handle,
		amdsmi_driver_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_MODEL)(amdsmi_processor_handle,
		amdsmi_driver_model_type_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_CAP_INFO)(amdsmi_processor_handle, uint32_t,
		amdsmi_power_cap_info_t *);
// typedef amdsmi_status_t (*AMDSMI_GET_PCIE_LINK_CAPS)(amdsmi_processor_handle,
// 						     amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);
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
// typedef amdsmi_status_t (*AMDSMI_GET_PCIE_LINK_STATUS)(amdsmi_processor_handle,
// 						       amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle, uint32_t,
		amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_SET_POWER_CAP)(amdsmi_processor_handle, uint32_t,
		uint64_t);
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
typedef amdsmi_status_t (*AMDSMI_GET_SOC_PSTATE)(amdsmi_processor_handle,
		amdsmi_dpm_policy_t *);
typedef amdsmi_status_t (*AMDSMI_SET_SOC_PSTATE)(amdsmi_processor_handle,
		uint32_t);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_COUNT)(amdsmi_processor_handle, amdsmi_gpu_block_t,
		amdsmi_error_count_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_ENABLED)(amdsmi_processor_handle,
		uint64_t *);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_BAD_PAGE_INFO)(amdsmi_processor_handle,
		uint32_t *, amdsmi_eeprom_table_record_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_FEATURE_INFO)(amdsmi_processor_handle,
		amdsmi_ras_feature_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint32_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_SET_NUM_VF)(amdsmi_processor_handle, uint32_t);
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

typedef amdsmi_status_t (*AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO)(amdsmi_processor_handle,
		amdsmi_xgmi_fb_sharing_mode_t);

typedef amdsmi_status_t (*AMDSMI_SET_XGMI_FB_SHARING_MODE_V2)(amdsmi_processor_handle, uint32_t,
		amdsmi_xgmi_fb_sharing_mode_t);

typedef amdsmi_status_t (*AMDSMI_EVENT_CREATE)(amdsmi_processor_handle *, uint32_t,

		uint64_t, amdsmi_event_set *);

typedef amdsmi_status_t (*AMDSMI_EVENT_READ)(amdsmi_event_set, int64_t, amdsmi_event_entry_t *);

typedef amdsmi_status_t (*AMDSMI_EVENT_DESTROY)(amdsmi_event_set);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_METRICS)(amdsmi_processor_handle, uint32_t *,
		amdsmi_metric_t *);

typedef amdsmi_status_t (*AMDSMI_GET_LIB_VERSION)(amdsmi_version_t *);

typedef amdsmi_status_t (*AMDSMI_CLEAR_VF_FB)(amdsmi_vf_handle_t);

typedef amdsmi_status_t (*AMDSMI_GET_PARTITION_PROFILE_CONFIG)(amdsmi_processor_handle,
		amdsmi_accelerator_partition_profile_config_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CURR_ACCELERATOR_PARTITION)(amdsmi_processor_handle,
		amdsmi_accelerator_partition_profile_t *, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_GET_MEMORY_PARTITION_CAPS)(amdsmi_processor_handle,
		amdsmi_nps_caps_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CURR_MEMORY_PARTITION)(amdsmi_processor_handle,
		amdsmi_memory_partition_type_t *);
typedef amdsmi_status_t (*AMDSMI_SET_ACCELERATOR_PARTITION)(amdsmi_processor_handle, uint32_t);
typedef amdsmi_status_t (*AMDSMI_SET_MEMORY_PARTITION)(amdsmi_processor_handle,
		amdsmi_memory_partition_type_t);
typedef amdsmi_status_t (*AMDSMI_GET_MEMORY_PARTITION_CONFIG)(amdsmi_processor_handle,
		amdsmi_memory_partition_config_t *);
typedef amdsmi_status_t (*AMDSMI_GPU_GET_CPER_ENTRIES)(amdsmi_processor_handle, uint32_t, char*, uint64_t *,
        amdsmi_cper_hdr**, uint64_t *, uint64_t *);

/////
/////
/////

AMDSMI_INIT host_amdsmi_init;
AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
AMDSMI_GET_GPU_ASIC_INFO host_amdsmi_get_gpu_asic_info;
AMDSMI_GET_GPU_VRAM_INFO host_amdsmi_get_gpu_vram_info;
AMDSMI_SHUT_DOWN host_amdsmi_shut_down;
AMDSMI_GET_GPU_DEVICE_BDF host_amdsmi_get_gpu_device_bdf;
AMDSMI_GET_GPU_DEVICE_UUID host_amdsmi_get_gpu_device_uuid;
AMDSMI_GET_VF_BDF host_amdsmi_get_vf_bdf;
AMDSMI_GET_VF_UUID host_amdsmi_get_vf_uuid;
// AMDSMI_GET_GPU_DRIVER_VERSION amdsmi_get_gpu_driver_version;
AMDSMI_GET_GPU_DRIVER_INFO host_amdsmi_get_gpu_driver_info;
AMDSMI_GET_GPU_DRIVER_MODEL host_amdsmi_get_gpu_driver_model;
AMDSMI_GET_POWER_CAP_INFO host_amdsmi_get_power_cap_info;
// AMDSMI_GET_PCIE_LINK_CAPS amdsmi_get_pcie_link_caps;
AMDSMI_GET_PCIE_INFO host_amdsmi_get_pcie_info;
AMDSMI_GET_FB_LAYOUT host_amdsmi_get_fb_layout;
AMDSMI_GET_GPU_VBIOS_INFO host_amdsmi_get_gpu_vbios_info;
AMDSMI_GET_GPU_BOARD_INFO host_amdsmi_get_gpu_board_info;
AMDSMI_GET_FW_INFO host_amdsmi_get_fw_info;
AMDSMI_GET_FW_ERROR_RECORDS host_amdsmi_get_fw_error_records;
AMDSMI_GET_DFC_FW_TABLE host_amdsmi_get_dfc_fw_table;
AMDSMI_GET_GPU_ACTIVITY host_amdsmi_get_gpu_activity;
// AMDSMI_GET_PCIE_LINK_STATUS amdsmi_get_pcie_link_status;
AMDSMI_GET_POWER_INFO host_amdsmi_get_power_info;
AMDSMI_SET_POWER_CAP host_amdsmi_set_power_cap;
AMDSMI_IS_GPU_POWER_MANAGEMENT_ENABLED host_amdsmi_is_gpu_power_management_enabled;
AMDSMI_GET_CLOCK_INFO host_amdsmi_get_clock_info;
AMDSMI_GET_TEMP_METRIC host_amdsmi_get_temp_metric;
AMDSMI_GET_GPU_CACHE_INFO host_amdsmi_get_gpu_cache_info;
AMDSMI_GET_SOC_PSTATE host_amdsmi_get_soc_pstate;
AMDSMI_SET_SOC_PSTATE host_amdsmi_set_soc_pstate;
AMDSMI_GET_GPU_TOTAL_ECC_COUNT host_amdsmi_get_gpu_total_ecc_count;

AMDSMI_GET_GPU_ECC_COUNT host_amdsmi_get_gpu_ecc_count;
AMDSMI_GET_GPU_ECC_ENABLED host_amdsmi_get_gpu_ecc_enabled;

AMDSMI_GET_GPU_BAD_PAGE_INFO host_amdsmi_get_gpu_bad_page_info;
AMDSMI_GET_GPU_RAS_FEATURE_INFO host_amdsmi_get_gpu_ras_feature_info;
AMDSMI_GET_NUM_VF host_amdsmi_get_num_vf;
AMDSMI_SET_NUM_VF host_amdsmi_set_num_vf;
AMDSMI_GET_VF_PARTITION_INFO host_amdsmi_get_vf_partition_info;
AMDSMI_GET_VF_INFO host_amdsmi_get_vf_info;
AMDSMI_GET_VF_DATA host_amdsmi_get_vf_data;
AMDSMI_GET_GUEST_DATA host_amdsmi_get_guest_data;
AMDSMI_GET_VF_FW_INFO host_amdsmi_get_vf_fw_info;
AMDSMI_GET_PARTITION_PROFILE_INFO host_amdsmi_get_partition_profile_info;
AMDSMI_GET_LINK_TOPOLOGY host_amdsmi_get_link_topology;
AMDSMI_GET_LINK_METRICS host_amdsmi_get_link_metrics;
AMDSMI_GET_XGMI_FB_SHARING_CAPS host_amdsmi_get_xgmi_fb_sharing_caps;
AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO host_amdsmi_get_xgmi_fb_sharing_mode_info;
AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO host_amdsmi_set_xgmi_fb_sharing_mode_info;
AMDSMI_SET_XGMI_FB_SHARING_MODE_V2 host_amdsmi_set_xgmi_fb_sharing_mode_v2;
AMDSMI_EVENT_CREATE host_amdsmi_event_create;
AMDSMI_EVENT_READ host_amdsmi_event_read;
AMDSMI_EVENT_DESTROY host_amdsmi_event_destroy;
AMDSMI_GET_GPU_METRICS host_amdsmi_get_gpu_metrics;
AMDSMI_GET_LIB_VERSION host_amdsmi_get_lib_version;
AMDSMI_CLEAR_VF_FB host_amdsmi_clear_vf_fb;
AMDSMI_GET_PARTITION_PROFILE_CONFIG host_amdsmi_get_partition_profile_config;
AMDSMI_GET_CURR_ACCELERATOR_PARTITION host_amdsmi_get_partition_profile;
AMDSMI_GET_MEMORY_PARTITION_CAPS host_amdsmi_get_memory_partition_caps;
AMDSMI_GET_CURR_MEMORY_PARTITION host_amdsmi_get_curr_memory_partition;
AMDSMI_SET_ACCELERATOR_PARTITION host_amdsmi_set_gpu_accelerator_partition_command;
AMDSMI_SET_MEMORY_PARTITION host_amdsmi_set_gpu_memory_partition_command;
AMDSMI_GET_MEMORY_PARTITION_CONFIG host_amdsmi_get_gpu_memory_partition_config;
AMDSMI_GPU_GET_CPER_ENTRIES host_amdsmi_gpu_get_cper_entries;

AmdSmiApiHost::AmdSmiApiHost()
{
#ifdef _WIN64
	char systemPath[MAX_PATH];
	UINT systemDirLength;
	const char *dllPath = "\\libamdsmi_host.dll";
	systemDirLength = GetSystemDirectoryA(systemPath, MAX_PATH);
	if (systemDirLength == 0) {
		std::cout << "Error GetSystemDirectoryA" << std::endl; //throw exception
		exit(1);
	}
	strncat_s(systemPath, sizeof(systemPath), dllPath, MAX_PATH - strlen(systemPath) - 1);
	// load libamdsmi_host.dll located in Windows/System32
	amdSmiLibHandle = (HMODULE)LoadLibraryA(systemPath);
#elif __linux__
	amdSmiLibHandle = dlopen("/usr/lib/libamdsmi.so", RTLD_NOW | RTLD_GLOBAL);
	if (amdSmiLibHandle == NULL) {
		amdSmiLibHandle = dlopen("./libamdsmi.so", RTLD_NOW | RTLD_GLOBAL);
	}
#endif
	if (amdSmiLibHandle == NULL) {
#ifdef _WIN64
		std::cout << "Error LoadLibraryA" << std::endl;
#elif __linux__
		std::cout <<
				  "Error while loading shared library: libamdsmi.so: cannot open shared object file: No such file or directory"
				  << std::endl;
#endif
		exit(1);
	}

	host_amdsmi_init = (AMDSMI_INIT)LOAD_SYM(amdSmiLibHandle, "amdsmi_init");
	host_amdsmi_get_processor_handles = (AMDSMI_GET_PROCESSOR_HANDLES)LOAD_SYM(
											amdSmiLibHandle, "amdsmi_get_processor_handles");
	host_amdsmi_get_processor_handle_from_bdf =
		(AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_processor_handle_from_bdf");
	host_amdsmi_get_vf_handle_from_bdf =
		(AMDSMI_GET_VF_HANDLE_FROM_BDF)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_vf_handle_from_bdf");
	host_amdsmi_get_gpu_asic_info = (AMDSMI_GET_GPU_ASIC_INFO)LOAD_SYM(
										amdSmiLibHandle, "amdsmi_get_gpu_asic_info");
	host_amdsmi_get_gpu_vram_info = (AMDSMI_GET_GPU_VRAM_INFO)LOAD_SYM(
										amdSmiLibHandle, "amdsmi_get_gpu_vram_info");
	host_amdsmi_shut_down = (AMDSMI_SHUT_DOWN)LOAD_SYM(amdSmiLibHandle, "amdsmi_shut_down");

	host_amdsmi_get_gpu_device_bdf = (AMDSMI_GET_GPU_DEVICE_BDF)LOAD_SYM(
										 amdSmiLibHandle, "amdsmi_get_gpu_device_bdf");
	host_amdsmi_get_gpu_device_uuid = (AMDSMI_GET_GPU_DEVICE_UUID)LOAD_SYM(
										  amdSmiLibHandle, "amdsmi_get_gpu_device_uuid");

	host_amdsmi_get_vf_bdf = (AMDSMI_GET_VF_BDF)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_vf_bdf");
	host_amdsmi_get_vf_uuid =
		(AMDSMI_GET_VF_UUID)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_vf_uuid");

	// amdsmi_get_gpu_driver_version = (AMDSMI_GET_GPU_DRIVER_VERSION)LOAD_SYM(
	// amdSmiLibHandle, "amdsmi_get_gpu_driver_version");
	host_amdsmi_get_gpu_driver_info = (AMDSMI_GET_GPU_DRIVER_INFO)LOAD_SYM(
										  amdSmiLibHandle, "amdsmi_get_gpu_driver_info");
	host_amdsmi_get_gpu_driver_model = (AMDSMI_GET_GPU_DRIVER_MODEL)LOAD_SYM(
										   amdSmiLibHandle, "amdsmi_get_gpu_driver_model");
	host_amdsmi_get_power_cap_info = (AMDSMI_GET_POWER_CAP_INFO)LOAD_SYM(
										 amdSmiLibHandle, "amdsmi_get_power_cap_info");

	// amdsmi_get_pcie_link_caps = (AMDSMI_GET_PCIE_LINK_CAPS)LOAD_SYM(
	// 	amdSmiLibHandle, "amdsmi_get_pcie_link_caps");
	host_amdsmi_get_pcie_info = (AMDSMI_GET_PCIE_INFO)LOAD_SYM(
									amdSmiLibHandle, "amdsmi_get_pcie_info");
	host_amdsmi_get_fb_layout =
		(AMDSMI_GET_FB_LAYOUT)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_fb_layout");
	host_amdsmi_get_gpu_vbios_info = (AMDSMI_GET_GPU_VBIOS_INFO)LOAD_SYM(
										 amdSmiLibHandle, "amdsmi_get_gpu_vbios_info");

	host_amdsmi_get_gpu_board_info = (AMDSMI_GET_GPU_BOARD_INFO)LOAD_SYM(
										 amdSmiLibHandle, "amdsmi_get_gpu_board_info");
	host_amdsmi_get_fw_info =
		(AMDSMI_GET_FW_INFO)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_fw_info");
	host_amdsmi_get_fw_error_records = (AMDSMI_GET_FW_ERROR_RECORDS)LOAD_SYM(
										   amdSmiLibHandle, "amdsmi_get_fw_error_records");
	host_amdsmi_get_dfc_fw_table =
		(AMDSMI_GET_DFC_FW_TABLE)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_dfc_fw_table");

	host_amdsmi_get_gpu_activity =
		(AMDSMI_GET_GPU_ACTIVITY)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_gpu_activity");
	// amdsmi_get_pcie_link_status = (AMDSMI_GET_PCIE_LINK_STATUS)LOAD_SYM(
	// 	amdSmiLibHandle, "amdsmi_get_pcie_link_status");
	host_amdsmi_get_power_info =
		(AMDSMI_GET_POWER_INFO)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_power_info");
	host_amdsmi_set_power_cap =
		(AMDSMI_SET_POWER_CAP)LOAD_SYM(amdSmiLibHandle, "amdsmi_set_power_cap");
	host_amdsmi_is_gpu_power_management_enabled =
		(AMDSMI_IS_GPU_POWER_MANAGEMENT_ENABLED)LOAD_SYM(amdSmiLibHandle,
			"amdsmi_is_gpu_power_management_enabled");
	host_amdsmi_get_clock_info =
		(AMDSMI_GET_CLOCK_INFO)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_clock_info");
	host_amdsmi_get_temp_metric =
		(AMDSMI_GET_TEMP_METRIC)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_temp_metric");
	host_amdsmi_get_gpu_cache_info =
		(AMDSMI_GET_GPU_CACHE_INFO)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_gpu_cache_info");
	host_amdsmi_get_soc_pstate =
		(AMDSMI_GET_SOC_PSTATE)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_soc_pstate");
	host_amdsmi_set_soc_pstate =
		(AMDSMI_SET_SOC_PSTATE)LOAD_SYM(amdSmiLibHandle, "amdsmi_set_soc_pstate");
	host_amdsmi_get_gpu_total_ecc_count = (AMDSMI_GET_GPU_TOTAL_ECC_COUNT)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_gpu_total_ecc_count");

	host_amdsmi_get_gpu_ecc_count = (AMDSMI_GET_GPU_ECC_COUNT)LOAD_SYM(
										amdSmiLibHandle, "amdsmi_get_gpu_ecc_count");
	host_amdsmi_get_gpu_ecc_enabled = (AMDSMI_GET_GPU_ECC_ENABLED)LOAD_SYM(
										  amdSmiLibHandle, "amdsmi_get_gpu_ecc_enabled");

	host_amdsmi_get_gpu_bad_page_info = (AMDSMI_GET_GPU_BAD_PAGE_INFO)LOAD_SYM(
											amdSmiLibHandle, "amdsmi_get_gpu_bad_page_info");
	host_amdsmi_get_gpu_ras_feature_info = (AMDSMI_GET_GPU_RAS_FEATURE_INFO)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_gpu_ras_feature_info");
	host_amdsmi_get_num_vf = (AMDSMI_GET_NUM_VF)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_num_vf");
	host_amdsmi_set_num_vf = (AMDSMI_SET_NUM_VF)LOAD_SYM(amdSmiLibHandle, "amdsmi_set_num_vf");
	host_amdsmi_get_vf_partition_info = (AMDSMI_GET_VF_PARTITION_INFO)LOAD_SYM(
											amdSmiLibHandle, "amdsmi_get_vf_partition_info");
	host_amdsmi_get_vf_info =
		(AMDSMI_GET_VF_INFO)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_vf_info");
	host_amdsmi_get_vf_data =
		(AMDSMI_GET_VF_DATA)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_vf_data");
	host_amdsmi_get_guest_data =
		(AMDSMI_GET_GUEST_DATA)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_guest_data");
	host_amdsmi_get_vf_fw_info =
		(AMDSMI_GET_VF_FW_INFO)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_vf_fw_info");
	host_amdsmi_get_partition_profile_info = (AMDSMI_GET_PARTITION_PROFILE_INFO)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_partition_profile_info");
	host_amdsmi_get_link_topology = (AMDSMI_GET_LINK_TOPOLOGY)LOAD_SYM(
										amdSmiLibHandle, "amdsmi_get_link_topology");
	host_amdsmi_get_link_metrics = (AMDSMI_GET_LINK_METRICS)LOAD_SYM(
									   amdSmiLibHandle, "amdsmi_get_link_metrics");
	host_amdsmi_get_xgmi_fb_sharing_caps = (AMDSMI_GET_XGMI_FB_SHARING_CAPS)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_xgmi_fb_sharing_caps");
	host_amdsmi_get_xgmi_fb_sharing_mode_info = (AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_xgmi_fb_sharing_mode_info");

	host_amdsmi_set_xgmi_fb_sharing_mode_info = (AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_set_xgmi_fb_sharing_mode");

	host_amdsmi_set_xgmi_fb_sharing_mode_v2 = (AMDSMI_SET_XGMI_FB_SHARING_MODE_V2)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_set_xgmi_fb_sharing_mode_v2");

	host_amdsmi_event_create = (AMDSMI_EVENT_CREATE)LOAD_SYM(
								   amdSmiLibHandle, "amdsmi_event_create");
	host_amdsmi_event_read = (AMDSMI_EVENT_READ)LOAD_SYM(
								 amdSmiLibHandle, "amdsmi_event_read");
	host_amdsmi_event_destroy = (AMDSMI_EVENT_DESTROY)LOAD_SYM(
									amdSmiLibHandle, "amdsmi_event_destroy");

	host_amdsmi_get_gpu_metrics = (AMDSMI_GET_GPU_METRICS)LOAD_SYM(amdSmiLibHandle,
								  "amdsmi_get_gpu_metrics");

	host_amdsmi_get_lib_version = (AMDSMI_GET_LIB_VERSION)LOAD_SYM(amdSmiLibHandle,
								  "amdsmi_get_lib_version");

	host_amdsmi_clear_vf_fb = (AMDSMI_CLEAR_VF_FB)LOAD_SYM(amdSmiLibHandle,
							  "amdsmi_clear_vf_fb");

	host_amdsmi_get_partition_profile_config = (AMDSMI_GET_PARTITION_PROFILE_CONFIG)LOAD_SYM(
			amdSmiLibHandle,
			"amdsmi_get_gpu_accelerator_partition_profile_config");
	host_amdsmi_get_memory_partition_caps = (AMDSMI_GET_MEMORY_PARTITION_CAPS)LOAD_SYM(amdSmiLibHandle,
											"amdsmi_get_gpu_memory_partition_caps");
	host_amdsmi_get_curr_memory_partition = (AMDSMI_GET_CURR_MEMORY_PARTITION)LOAD_SYM(amdSmiLibHandle,
											"amdsmi_get_gpu_memory_partition_setting");
	host_amdsmi_set_gpu_accelerator_partition_command = (AMDSMI_SET_ACCELERATOR_PARTITION)LOAD_SYM(
			amdSmiLibHandle,
			"amdsmi_set_gpu_accelerator_partition_profile");
	host_amdsmi_set_gpu_memory_partition_command = (AMDSMI_SET_MEMORY_PARTITION)LOAD_SYM(
			amdSmiLibHandle,
			"amdsmi_set_gpu_memory_partition_mode");
	host_amdsmi_get_partition_profile = (AMDSMI_GET_CURR_ACCELERATOR_PARTITION)LOAD_SYM(amdSmiLibHandle,
										"amdsmi_get_gpu_accelerator_partition_profile");
	host_amdsmi_get_gpu_memory_partition_config = (AMDSMI_GET_MEMORY_PARTITION_CONFIG)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_gpu_memory_partition_config");

	host_amdsmi_gpu_get_cper_entries = (AMDSMI_GPU_GET_CPER_ENTRIES)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_gpu_get_cper_entries");

	int ret = host_amdsmi_init(AMDSMI_INIT_AMD_GPUS);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		printf("AMDSMI failed to init \n");
		exit(1);
	}
};

AmdSmiApiHost::~AmdSmiApiHost()
{
	int ret = host_amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS)
		printf("AMDSMI failed to finish\n");

#ifdef _WIN64
	FreeLibrary((HMODULE)amdSmiLibHandle);
#elif __linux__
	dlclose(amdSmiLibHandle);
#endif
}
