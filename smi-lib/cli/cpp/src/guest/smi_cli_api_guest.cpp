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
#include "interface/amdsmi.h"
#include "smi_cli_api_guest.h"
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
typedef amdsmi_status_t (*AMDSMI_SHUT_DOWN)(void);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t *,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
		amdsmi_processor_handle *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ASIC_INFO)(amdsmi_processor_handle,
		amdsmi_asic_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_UUID)(amdsmi_processor_handle, unsigned int *,
		char *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VBIOS_INFO)(amdsmi_processor_handle,
		amdsmi_vbios_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_INFO)(amdsmi_processor_handle,
		amdsmi_driver_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle,
		amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_FEATURE_INFO)(amdsmi_processor_handle,
		amdsmi_ras_feature_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PROCESS_LIST)(amdsmi_processor_handle, uint32_t *,
		amdsmi_proc_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint8_t *, uint8_t *);



typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle,
		amdsmi_engine_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle,
		amdsmi_power_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_CLOCK_INFO)(amdsmi_processor_handle, amdsmi_clk_type_t,
		amdsmi_clk_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
		amdsmi_temperature_type_t,
		amdsmi_temperature_metric_t, int64_t *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
		amdsmi_error_count_t *);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_VRAM_USAGE)(amdsmi_processor_handle,
		amdsmi_vram_usage_t *);
typedef amdsmi_status_t (*AMDSMI_GET_FW_INFO)(amdsmi_processor_handle, amdsmi_fw_info_t *);

typedef amdsmi_status_t (*AMDSMI_GET_LIB_VERSION)(amdsmi_version_t *);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_PROCESS_ISOLATION)(amdsmi_processor_handle, uint32_t *);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_PROCESS_ISOLATION)(amdsmi_processor_handle, uint32_t);
typedef amdsmi_status_t (*AMDSMI_CLEAN_GPU_LOCAL_DATA)(amdsmi_processor_handle);

/////////////////////

AMDSMI_INIT guest_amdsmi_init;
AMDSMI_SHUT_DOWN guest_amdsmi_shut_down;
AMDSMI_GET_PROCESSOR_HANDLES guest_amdsmi_get_processor_handles;
AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF guest_amdsmi_get_processor_handle_from_bdf;
AMDSMI_GET_GPU_ASIC_INFO guest_amdsmi_get_gpu_asic_info;
AMDSMI_GET_GPU_DEVICE_BDF guest_amdsmi_get_gpu_device_bdf;
AMDSMI_GET_GPU_DEVICE_UUID guest_amdsmi_get_gpu_device_uuid;
AMDSMI_GET_GPU_VBIOS_INFO guest_amdsmi_get_gpu_vbios_info;
AMDSMI_GET_GPU_DRIVER_INFO guest_amdsmi_get_gpu_driver_info;
AMDSMI_GET_PCIE_INFO guest_amdsmi_get_pcie_info;
AMDSMI_GET_GPU_RAS_FEATURE_INFO guest_amdsmi_get_gpu_ras_feature_info;
AMDSMI_GET_GPU_ACTIVITY guest_amdsmi_get_gpu_activity;
AMDSMI_GET_POWER_INFO guest_amdsmi_get_power_info;
AMDSMI_GET_CLOCK_INFO guest_amdsmi_get_clock_info;
AMDSMI_GET_TEMP_METRIC guest_amdsmi_get_temp_metric;
AMDSMI_GET_GPU_TOTAL_ECC_COUNT guest_amdsmi_get_gpu_total_ecc_count;
AMDSMI_GET_GPU_VRAM_USAGE guest_amdsmi_get_gpu_vram_usage;
AMDSMI_GET_FW_INFO guest_amdsmi_get_fw_info;

AMDSMI_GET_GPU_PROCESS_LIST guest_amdsmi_get_gpu_process_list;
AMDSMI_GET_LIB_VERSION guest_amdsmi_get_lib_version;

AMDSMI_GET_GPU_PROCESS_ISOLATION guest_amdsmi_get_gpu_process_isolation;
AMDSMI_SET_GPU_PROCESS_ISOLATION guest_amdsmi_set_gpu_process_isolation;
AMDSMI_CLEAN_GPU_LOCAL_DATA guest_amdsmi_clean_gpu_local_data;

AmdSmiApiGuest::AmdSmiApiGuest()
{
#ifdef _WIN64
	char systemPath[MAX_PATH];
	UINT systemDirLength;
	const char *dllPath = "\\libamdsmi_guest.dll";
	systemDirLength = GetSystemDirectoryA(systemPath, MAX_PATH);
	if (systemDirLength == 0) {
		std::cout << "Error GetSystemDirectoryA" << std::endl; //throw exception
		exit(1);
	}
	strncat_s(systemPath, sizeof(systemPath), dllPath, MAX_PATH - strlen(systemPath) - 1);
	// load libamdsmi_host.dll located in Windows/System32
	amdSmiLibHandle = (HMODULE)LoadLibraryA(systemPath);
#elif __linux__
	amdSmiLibHandle = dlopen("./libamdsmi.so", RTLD_NOW);
#endif
	if (amdSmiLibHandle == NULL) {
		std::cout << "Error LoadLibraryA" << std::endl; //throw exception
		exit(1);
	}

	guest_amdsmi_init = (AMDSMI_INIT)LOAD_SYM(amdSmiLibHandle, "amdsmi_init");
	guest_amdsmi_get_processor_handles = (AMDSMI_GET_PROCESSOR_HANDLES)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_processor_handles");
	guest_amdsmi_get_processor_handle_from_bdf =
		(AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_processor_handle_from_bdf");
	guest_amdsmi_shut_down = (AMDSMI_SHUT_DOWN)LOAD_SYM(amdSmiLibHandle, "amdsmi_shut_down");

	guest_amdsmi_get_gpu_device_bdf = (AMDSMI_GET_GPU_DEVICE_BDF)LOAD_SYM(
										  amdSmiLibHandle, "amdsmi_get_gpu_device_bdf");
	guest_amdsmi_get_gpu_asic_info = (AMDSMI_GET_GPU_ASIC_INFO)LOAD_SYM(
										 amdSmiLibHandle, "amdsmi_get_gpu_asic_info");
	guest_amdsmi_get_gpu_device_uuid = (AMDSMI_GET_GPU_DEVICE_UUID)LOAD_SYM(
										   amdSmiLibHandle, "amdsmi_get_gpu_device_uuid");
	guest_amdsmi_get_gpu_process_list = (AMDSMI_GET_GPU_PROCESS_LIST)LOAD_SYM(
											amdSmiLibHandle, "amdsmi_get_gpu_process_list");

	guest_amdsmi_get_gpu_vbios_info = (AMDSMI_GET_GPU_VBIOS_INFO)LOAD_SYM(
										  amdSmiLibHandle, "amdsmi_get_gpu_vbios_info");
	guest_amdsmi_get_gpu_driver_info = (AMDSMI_GET_GPU_DRIVER_INFO)LOAD_SYM(
										   amdSmiLibHandle, "amdsmi_get_gpu_driver_info");
	guest_amdsmi_get_pcie_info = (AMDSMI_GET_PCIE_INFO)LOAD_SYM(
									 amdSmiLibHandle, "amdsmi_get_pcie_info");
	guest_amdsmi_get_gpu_ras_feature_info = (AMDSMI_GET_GPU_RAS_FEATURE_INFO)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_gpu_ras_feature_info");

	guest_amdsmi_get_gpu_activity =
		(AMDSMI_GET_GPU_ACTIVITY)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_gpu_activity");
	guest_amdsmi_get_power_info =
		(AMDSMI_GET_POWER_INFO)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_power_info");
	guest_amdsmi_get_clock_info =
		(AMDSMI_GET_CLOCK_INFO)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_clock_info");
	guest_amdsmi_get_temp_metric =
		(AMDSMI_GET_TEMP_METRIC)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_temp_metric");
	guest_amdsmi_get_gpu_total_ecc_count = (AMDSMI_GET_GPU_TOTAL_ECC_COUNT)LOAD_SYM(
			amdSmiLibHandle, "amdsmi_get_gpu_total_ecc_count");
	guest_amdsmi_get_gpu_vram_usage =
		(AMDSMI_GET_GPU_VRAM_USAGE)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_gpu_vram_usage");
	guest_amdsmi_get_fw_info =
		(AMDSMI_GET_FW_INFO)LOAD_SYM(amdSmiLibHandle, "amdsmi_get_fw_info");
	guest_amdsmi_get_lib_version = (AMDSMI_GET_LIB_VERSION)LOAD_SYM(amdSmiLibHandle,
								   "amdsmi_get_lib_version");

	guest_amdsmi_get_gpu_process_isolation = (AMDSMI_GET_GPU_PROCESS_ISOLATION)LOAD_SYM(amdSmiLibHandle,
		"amdsmi_get_gpu_process_isolation");
	guest_amdsmi_set_gpu_process_isolation = (AMDSMI_SET_GPU_PROCESS_ISOLATION)LOAD_SYM(amdSmiLibHandle,
		"amdsmi_set_gpu_process_isolation");
	guest_amdsmi_clean_gpu_local_data = (AMDSMI_CLEAN_GPU_LOCAL_DATA)LOAD_SYM(amdSmiLibHandle,
										"amdsmi_clean_gpu_local_data");

	int ret = guest_amdsmi_init(AMDSMI_INIT_AMD_GPUS);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		printf("AMDSMI failed to init \n");
		exit(1);
	}
};

AmdSmiApiGuest::~AmdSmiApiGuest()
{
	int ret = guest_amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS)
		printf("AMDSMI failed to finish\n");

#ifdef _WIN64
	FreeLibrary((HMODULE)amdSmiLibHandle);
#elif __linux__
	dlclose(amdSmiLibHandle);
#endif
};
