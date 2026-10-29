/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "interface/amdsmi.h"
#include "smi_cli_api_guest.h"
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_exception.h"

#include <iostream>
#include <sstream>

typedef amdsmi_status_t (*AMDSMI_INIT)(uint64_t);
typedef amdsmi_status_t (*AMDSMI_SHUT_DOWN)(void);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t*,
							amdsmi_processor_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
								amdsmi_processor_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ASIC_INFO)(amdsmi_processor_handle, amdsmi_asic_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_UUID)(amdsmi_processor_handle, unsigned int*,
						      char*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VBIOS_INFO)(amdsmi_processor_handle, amdsmi_vbios_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_INFO)(amdsmi_processor_handle,
						      amdsmi_driver_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_MODEL)(amdsmi_processor_handle,
						       amdsmi_driver_model_type_t*);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle, amdsmi_pcie_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_FEATURE_INFO)(amdsmi_processor_handle,
							   amdsmi_ras_feature_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PROCESS_LIST)(amdsmi_processor_handle, uint32_t*,
						       amdsmi_proc_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint8_t*, uint8_t*);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle, amdsmi_engine_usage_t*);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle, amdsmi_power_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_CAP_INFO)(amdsmi_processor_handle, uint32_t,
						     amdsmi_power_cap_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_CLOCK_INFO)(amdsmi_processor_handle, amdsmi_clk_type_t,
						 amdsmi_clk_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
						  amdsmi_temperature_type_t,
						  amdsmi_temperature_metric_t, int64_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
							  amdsmi_error_count_t*);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_VRAM_USAGE)(amdsmi_processor_handle, amdsmi_vram_usage_t*);
typedef amdsmi_status_t (*AMDSMI_GET_FW_INFO)(amdsmi_processor_handle, amdsmi_fw_info_t*);

typedef amdsmi_status_t (*AMDSMI_GET_LIB_VERSION)(amdsmi_version_t*);
typedef amdsmi_status_t (*AMDSMI_GET_ROCM_VERSION)(amdsmi_version_t*);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_PROCESS_ISOLATION)(amdsmi_processor_handle, uint32_t*);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_PROCESS_ISOLATION)(amdsmi_processor_handle, uint32_t);
typedef amdsmi_status_t (*AMDSMI_SET_POWER_CAP)(amdsmi_processor_handle, uint32_t, uint64_t);
typedef amdsmi_status_t (*AMDSMI_CLEAN_GPU_LOCAL_DATA)(amdsmi_processor_handle);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VIRTUALIZATION_MODE)(amdsmi_processor_handle,
							      amdsmi_virtualization_mode_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_FAN_RPMS)(amdsmi_processor_handle, uint32_t, int64_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_FAN_SPEED)(amdsmi_processor_handle, uint32_t, int64_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_FAN_SPEED_MAX)(amdsmi_processor_handle, uint32_t,
							uint64_t*);

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
AMDSMI_GET_GPU_DRIVER_MODEL guest_amdsmi_get_gpu_driver_model;
AMDSMI_GET_PCIE_INFO guest_amdsmi_get_pcie_info;
AMDSMI_GET_GPU_RAS_FEATURE_INFO guest_amdsmi_get_gpu_ras_feature_info;
AMDSMI_GET_GPU_ACTIVITY guest_amdsmi_get_gpu_activity;
AMDSMI_GET_POWER_INFO guest_amdsmi_get_power_info;
AMDSMI_GET_POWER_CAP_INFO guest_amdsmi_get_power_cap_info;
AMDSMI_GET_CLOCK_INFO guest_amdsmi_get_clock_info;
AMDSMI_GET_TEMP_METRIC guest_amdsmi_get_temp_metric;
AMDSMI_GET_GPU_TOTAL_ECC_COUNT guest_amdsmi_get_gpu_total_ecc_count;
AMDSMI_GET_GPU_VRAM_USAGE guest_amdsmi_get_gpu_vram_usage;
AMDSMI_GET_FW_INFO guest_amdsmi_get_fw_info;

AMDSMI_GET_GPU_PROCESS_LIST guest_amdsmi_get_gpu_process_list;
AMDSMI_GET_LIB_VERSION guest_amdsmi_get_lib_version;
AMDSMI_GET_ROCM_VERSION guest_amdsmi_get_rocm_version;

AMDSMI_GET_GPU_PROCESS_ISOLATION guest_amdsmi_get_gpu_process_isolation;
AMDSMI_SET_GPU_PROCESS_ISOLATION guest_amdsmi_set_gpu_process_isolation;
AMDSMI_SET_POWER_CAP guest_amdsmi_set_power_cap;
AMDSMI_CLEAN_GPU_LOCAL_DATA guest_amdsmi_clean_gpu_local_data;
AMDSMI_GET_GPU_VIRTUALIZATION_MODE guest_amdsmi_get_gpu_virtualization_mode;
AMDSMI_GET_GPU_FAN_RPMS guest_amdsmi_get_gpu_fan_rpms;
AMDSMI_GET_GPU_FAN_SPEED guest_amdsmi_get_gpu_fan_speed;
AMDSMI_GET_GPU_FAN_SPEED_MAX guest_amdsmi_get_gpu_fan_speed_max;

AmdSmiApiGuest::AmdSmiApiGuest() : loader_("\\libamdsmi_guest.dll", "./libamdsmi.so", false)
{
	if (!loader_.is_loaded()) {
		throw SmiToolSMILIBErrorException(34);
	}

	guest_amdsmi_init = (AMDSMI_INIT)loader_.get_symbol("amdsmi_init");
	guest_amdsmi_get_processor_handles =
	    (AMDSMI_GET_PROCESSOR_HANDLES)loader_.get_symbol("amdsmi_get_processor_handles");
	guest_amdsmi_get_processor_handle_from_bdf =
	    (AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)loader_.get_symbol(
		"amdsmi_get_processor_handle_from_bdf");
	guest_amdsmi_shut_down = (AMDSMI_SHUT_DOWN)loader_.get_symbol("amdsmi_shut_down");

	guest_amdsmi_get_gpu_device_bdf =
	    (AMDSMI_GET_GPU_DEVICE_BDF)loader_.get_symbol("amdsmi_get_gpu_device_bdf");
	guest_amdsmi_get_gpu_asic_info =
	    (AMDSMI_GET_GPU_ASIC_INFO)loader_.get_symbol("amdsmi_get_gpu_asic_info");
	guest_amdsmi_get_gpu_device_uuid =
	    (AMDSMI_GET_GPU_DEVICE_UUID)loader_.get_symbol("amdsmi_get_gpu_device_uuid");
	guest_amdsmi_get_gpu_process_list =
	    (AMDSMI_GET_GPU_PROCESS_LIST)loader_.get_symbol("amdsmi_get_gpu_process_list");

	guest_amdsmi_get_gpu_vbios_info =
	    (AMDSMI_GET_GPU_VBIOS_INFO)loader_.get_symbol("amdsmi_get_gpu_vbios_info");
	guest_amdsmi_get_gpu_driver_info =
	    (AMDSMI_GET_GPU_DRIVER_INFO)loader_.get_symbol("amdsmi_get_gpu_driver_info");
	guest_amdsmi_get_gpu_driver_model =
	    (AMDSMI_GET_GPU_DRIVER_MODEL)loader_.get_symbol("amdsmi_get_gpu_driver_model");
	guest_amdsmi_get_pcie_info =
	    (AMDSMI_GET_PCIE_INFO)loader_.get_symbol("amdsmi_get_pcie_info");
	guest_amdsmi_get_gpu_ras_feature_info =
	    (AMDSMI_GET_GPU_RAS_FEATURE_INFO)loader_.get_symbol("amdsmi_get_gpu_ras_feature_info");

	guest_amdsmi_get_gpu_activity =
	    (AMDSMI_GET_GPU_ACTIVITY)loader_.get_symbol("amdsmi_get_gpu_activity");
	guest_amdsmi_get_power_info =
	    (AMDSMI_GET_POWER_INFO)loader_.get_symbol("amdsmi_get_power_info");
	guest_amdsmi_get_power_cap_info =
	    (AMDSMI_GET_POWER_CAP_INFO)loader_.get_symbol("amdsmi_get_power_cap_info");
	guest_amdsmi_get_clock_info =
	    (AMDSMI_GET_CLOCK_INFO)loader_.get_symbol("amdsmi_get_clock_info");
	guest_amdsmi_get_temp_metric =
	    (AMDSMI_GET_TEMP_METRIC)loader_.get_symbol("amdsmi_get_temp_metric");
	guest_amdsmi_get_gpu_total_ecc_count =
	    (AMDSMI_GET_GPU_TOTAL_ECC_COUNT)loader_.get_symbol("amdsmi_get_gpu_total_ecc_count");
	guest_amdsmi_get_gpu_vram_usage =
	    (AMDSMI_GET_GPU_VRAM_USAGE)loader_.get_symbol("amdsmi_get_gpu_vram_usage");
	guest_amdsmi_get_fw_info = (AMDSMI_GET_FW_INFO)loader_.get_symbol("amdsmi_get_fw_info");
	guest_amdsmi_get_lib_version =
	    (AMDSMI_GET_LIB_VERSION)loader_.get_symbol("amdsmi_get_lib_version");
	guest_amdsmi_get_rocm_version =
	    (AMDSMI_GET_ROCM_VERSION)loader_.get_symbol("amdsmi_get_rocm_version");

	guest_amdsmi_get_gpu_process_isolation =
	    (AMDSMI_GET_GPU_PROCESS_ISOLATION)loader_.get_symbol(
		"amdsmi_get_gpu_process_isolation");
	guest_amdsmi_set_gpu_process_isolation =
	    (AMDSMI_SET_GPU_PROCESS_ISOLATION)loader_.get_symbol(
		"amdsmi_set_gpu_process_isolation");
	guest_amdsmi_set_power_cap =
	    (AMDSMI_SET_POWER_CAP)loader_.get_symbol("amdsmi_set_power_cap");
	guest_amdsmi_clean_gpu_local_data =
	    (AMDSMI_CLEAN_GPU_LOCAL_DATA)loader_.get_symbol("amdsmi_clean_gpu_local_data");
	guest_amdsmi_get_gpu_virtualization_mode =
	    (AMDSMI_GET_GPU_VIRTUALIZATION_MODE)loader_.get_symbol(
		"amdsmi_get_gpu_virtualization_mode");
	guest_amdsmi_get_gpu_fan_rpms =
	    (AMDSMI_GET_GPU_FAN_RPMS)loader_.get_symbol("amdsmi_get_gpu_fan_rpms");
	guest_amdsmi_get_gpu_fan_speed =
	    (AMDSMI_GET_GPU_FAN_SPEED)loader_.get_symbol("amdsmi_get_gpu_fan_speed");
	guest_amdsmi_get_gpu_fan_speed_max =
	    (AMDSMI_GET_GPU_FAN_SPEED_MAX)loader_.get_symbol("amdsmi_get_gpu_fan_speed_max");

	int ret = guest_amdsmi_init(AMDSMI_INIT_AMD_GPUS);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		if (ret == AMDSMI_STATUS_NO_PERM) {
			throw SmiToolPermissionDeniedException();
		}
		throw SmiToolSMILIBErrorException(ret);
	}
};

AmdSmiApiGuest::~AmdSmiApiGuest()
{
	int ret = guest_amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS)
		printf("AMDSMI failed to finish\n");
};
