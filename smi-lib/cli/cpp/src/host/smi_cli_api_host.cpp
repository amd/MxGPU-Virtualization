/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdsmi.h"
#include "smi_cli_api_host.h"
#include "smi_cli_helpers.h"
#include "smi_cli_parser.h"
#include "smi_cli_logger_err.h"
#include "smi_cli_templates.h"
#include "smi_cli_device.h"
#include "smi_cli_exception.h"
#include "smi_cli_platform.h"

#include <iostream>
#include <sstream>
#include <vector>

typedef amdsmi_status_t (*AMDSMI_INIT)(uint64_t);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t*,
							amdsmi_processor_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_PROCESSOR_HANDLES)(amdsmi_socket_handle, uint32_t*,
							    amdsmi_processor_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLES_BY_TYPE)(amdsmi_socket_handle,
								processor_type_t,
								amdsmi_processor_handle*,
								uint32_t*);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_TYPE)(amdsmi_processor_handle, processor_type_t*);
typedef amdsmi_status_t (*AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)(amdsmi_bdf_t,
								amdsmi_processor_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_BDF)(amdsmi_bdf_t, amdsmi_vf_handle_t*);
typedef amdsmi_status_t (*AMDSMI_GET_VF_HANDLE_FROM_VF_INDEX)(amdsmi_processor_handle, uint32_t,
							      amdsmi_vf_handle_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ASIC_INFO)(amdsmi_processor_handle, amdsmi_asic_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VRAM_INFO)(amdsmi_processor_handle, amdsmi_vram_info_t*);
typedef amdsmi_status_t (*AMDSMI_SHUT_DOWN)(void);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DEVICE_UUID)(amdsmi_processor_handle, unsigned int*,
						      char*);

typedef amdsmi_status_t (*AMDSMI_GET_VF_BDF)(amdsmi_vf_handle_t, amdsmi_bdf_t*);
typedef amdsmi_status_t (*AMDSMI_GET_VF_UUID)(amdsmi_vf_handle_t, unsigned int*, char*);

// typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_VERSION)(amdsmi_processor_handle, int *,
// 							 char *);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_INFO)(amdsmi_processor_handle,
						      amdsmi_driver_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_DRIVER_MODEL)(amdsmi_processor_handle,
						       amdsmi_driver_model_type_t*);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_CAP_INFO)(amdsmi_processor_handle, uint32_t,
						     amdsmi_power_cap_info_t*);
// typedef amdsmi_status_t (*AMDSMI_GET_PCIE_LINK_CAPS)(amdsmi_processor_handle,
// 						     amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_PCIE_INFO)(amdsmi_processor_handle, amdsmi_pcie_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PCI_BANDWIDTH)(amdsmi_processor_handle,
							amdsmi_pcie_bandwidth_t*);
typedef amdsmi_status_t (*AMDSMI_GET_FB_LAYOUT)(amdsmi_processor_handle, amdsmi_pf_fb_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VBIOS_INFO)(amdsmi_processor_handle, amdsmi_vbios_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_BOARD_INFO)(amdsmi_processor_handle, amdsmi_board_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_FW_INFO)(amdsmi_processor_handle, amdsmi_fw_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_FW_ERROR_RECORDS)(amdsmi_processor_handle,
						       amdsmi_fw_error_record_t*);
typedef amdsmi_status_t (*AMDSMI_GET_DFC_FW_TABLE)(amdsmi_processor_handle, amdsmi_dfc_fw_t*);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_ACTIVITY)(amdsmi_processor_handle, amdsmi_engine_usage_t*);
// typedef amdsmi_status_t (*AMDSMI_GET_PCIE_LINK_STATUS)(amdsmi_processor_handle,
// 						       amdsmi_pcie_info_t *);
typedef amdsmi_status_t (*AMDSMI_GET_POWER_INFO)(amdsmi_processor_handle, amdsmi_power_info_t*);
typedef amdsmi_status_t (*AMDSMI_SET_POWER_CAP)(amdsmi_processor_handle, uint32_t, uint64_t);
typedef amdsmi_status_t (*AMDSMI_IS_GPU_POWER_MANAGEMENT_ENABLED)(amdsmi_processor_handle, bool*);
typedef amdsmi_status_t (*AMDSMI_GET_CLOCK_INFO)(amdsmi_processor_handle, amdsmi_clk_type_t,
						 amdsmi_clk_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_TEMP_METRIC)(amdsmi_processor_handle,
						  amdsmi_temperature_type_t,
						  amdsmi_temperature_metric_t, int64_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_CACHE_INFO)(amdsmi_processor_handle,
						     amdsmi_gpu_cache_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_TOTAL_ECC_COUNT)(amdsmi_processor_handle,
							  amdsmi_error_count_t*);
typedef amdsmi_status_t (*AMDSMI_GET_SOC_PSTATE)(amdsmi_processor_handle, amdsmi_dpm_policy_t*);
typedef amdsmi_status_t (*AMDSMI_SET_SOC_PSTATE)(amdsmi_processor_handle, uint32_t);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_PLPD)(amdsmi_processor_handle, amdsmi_dpm_policy_t*);
typedef amdsmi_status_t (*AMDSMI_SET_XGMI_PLPD)(amdsmi_processor_handle, uint32_t);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PTL_STATE)(amdsmi_processor_handle, bool*);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_PTL_STATE)(amdsmi_processor_handle, bool);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_PTL_FORMATS)(amdsmi_processor_handle,
						      amdsmi_ptl_data_format_t*,
						      amdsmi_ptl_data_format_t*);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_PTL_FORMATS)(amdsmi_processor_handle,
						      amdsmi_ptl_data_format_t,
						      amdsmi_ptl_data_format_t);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_COUNT)(amdsmi_processor_handle, amdsmi_gpu_block_t,
						    amdsmi_error_count_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_ECC_ENABLED)(amdsmi_processor_handle, uint64_t*);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_BAD_PAGE_INFO)(amdsmi_processor_handle, uint32_t*,
							amdsmi_eeprom_table_record_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_FEATURE_INFO)(amdsmi_processor_handle,
							   amdsmi_ras_feature_t*);
typedef amdsmi_status_t (*AMDSMI_GET_BAD_PAGE_THRESHOLD)(amdsmi_processor_handle, uint32_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NUM_VF)(amdsmi_processor_handle, uint32_t*, uint32_t*);
typedef amdsmi_status_t (*AMDSMI_SET_NUM_VF)(amdsmi_processor_handle, uint32_t);
typedef amdsmi_status_t (*AMDSMI_GET_VF_PARTITION_INFO)(amdsmi_processor_handle, unsigned int,
							amdsmi_partition_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_VF_INFO)(amdsmi_vf_handle_t, amdsmi_vf_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_VF_DATA)(amdsmi_vf_handle_t, amdsmi_vf_data_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GUEST_DATA)(amdsmi_vf_handle_t, amdsmi_guest_data_t*);
typedef amdsmi_status_t (*AMDSMI_GET_VF_FW_INFO)(amdsmi_vf_handle_t, amdsmi_fw_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_PARTITION_PROFILE_INFO)(amdsmi_processor_handle,
							     amdsmi_profile_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_TOPOLOGY)(amdsmi_processor_handle,
						    amdsmi_processor_handle,
						    amdsmi_link_topology_t*);
typedef amdsmi_status_t (*AMDSMI_GET_LINK_METRICS)(amdsmi_processor_handle, amdsmi_link_metrics_t*);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_FB_SHARING_CAPS)(amdsmi_processor_handle,
							   amdsmi_xgmi_fb_sharing_caps_t*);
typedef amdsmi_status_t (*AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO)(amdsmi_processor_handle,
								amdsmi_processor_handle,
								amdsmi_xgmi_fb_sharing_mode_t,
								uint8_t*);

typedef amdsmi_status_t (*AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO)(amdsmi_processor_handle,
								amdsmi_xgmi_fb_sharing_mode_t);

typedef amdsmi_status_t (*AMDSMI_SET_XGMI_FB_SHARING_MODE_V2)(amdsmi_processor_handle, uint32_t,
							      amdsmi_xgmi_fb_sharing_mode_t);

typedef amdsmi_status_t (*AMDSMI_EVENT_CREATE)(amdsmi_processor_handle*, uint32_t,

					       uint64_t, amdsmi_event_set*);

typedef amdsmi_status_t (*AMDSMI_EVENT_READ)(amdsmi_event_set, int64_t, amdsmi_event_entry_t*);

typedef amdsmi_status_t (*AMDSMI_EVENT_DESTROY)(amdsmi_event_set);

typedef amdsmi_status_t (*AMDSMI_GET_GPU_METRICS)(amdsmi_processor_handle, uint32_t*,
						  amdsmi_metric_t*);

typedef amdsmi_status_t (*AMDSMI_GET_LIB_VERSION)(amdsmi_version_t*);

typedef amdsmi_status_t (*AMDSMI_CLEAR_VF_FB)(amdsmi_vf_handle_t);

typedef amdsmi_status_t (*AMDSMI_GET_PARTITION_PROFILE_CONFIG)(
    amdsmi_processor_handle, amdsmi_accelerator_partition_profile_config_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GLOBAL_PARTITION)(
    amdsmi_processor_handle, amdsmi_accelerator_partition_profile_config_global_t*);
typedef amdsmi_status_t (*AMDSMI_GET_CURR_ACCELERATOR_PARTITION)(
    amdsmi_processor_handle, amdsmi_accelerator_partition_profile_t*, uint32_t*);
typedef amdsmi_status_t (*AMDSMI_GET_MEMORY_PARTITION_CAPS)(amdsmi_processor_handle,
							    amdsmi_nps_caps_t*);
typedef amdsmi_status_t (*AMDSMI_GET_CURR_MEMORY_PARTITION)(amdsmi_processor_handle,
							    amdsmi_memory_partition_type_t*);
typedef amdsmi_status_t (*AMDSMI_SET_ACCELERATOR_PARTITION)(amdsmi_processor_handle, uint32_t);
typedef amdsmi_status_t (*AMDSMI_SET_MEMORY_PARTITION)(amdsmi_processor_handle,
						       amdsmi_memory_partition_type_t);
typedef amdsmi_status_t (*AMDSMI_GET_MEMORY_PARTITION_CONFIG)(amdsmi_processor_handle,
							      amdsmi_memory_partition_config_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_CPER_ENTRIES)(amdsmi_processor_handle, uint32_t, char*,
						       uint64_t*, amdsmi_cper_hdr_t**, uint64_t*,
						       uint64_t*);
typedef amdsmi_status_t (*AMDSMI_GET_FABRIC_CPER_ENTRIES)(amdsmi_processor_handle, uint32_t, char*,
							  uint64_t*, amdsmi_cper_hdr_t**, uint64_t*,
							  uint64_t*);
typedef amdsmi_status_t (*AMDSMI_TOPO_GET_P2P_STATUS)(amdsmi_processor_handle,
						      amdsmi_processor_handle, amdsmi_link_type_t*,
						      amdsmi_p2p_capability_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_VIRTUALIZATION_MODE)(amdsmi_processor_handle,
							      amdsmi_virtualization_mode_t*);
typedef amdsmi_status_t (*AMDSMI_GET_CPU_AFFINITY_WITH_SCOPE)(amdsmi_processor_handle, uint32_t,
							      uint64_t*, amdsmi_affinity_scope_t);
typedef amdsmi_status_t (*AMDSMI_TOPO_GET_NUMA_NODE_NUMBER)(amdsmi_processor_handle, uint32_t*);
typedef amdsmi_status_t (*AMDSMI_GET_AFIDS_FROM_CPER)(char* cper_buffer, uint32_t buf_size,
						      uint64_t* afids, uint32_t* num_afids);
typedef amdsmi_status_t (*AMDSMI_GET_VF_HBM_INFO)(amdsmi_vf_handle_t, amdsmi_vf_hbm_info_t*);

typedef amdsmi_status_t (*AMDSMI_RESET_GPU)(amdsmi_processor_handle);

typedef amdsmi_status_t (*AMDSMI_GET_NIC_ASIC_INFO)(amdsmi_processor_handle,
						    amdsmi_nic_asic_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_BUS_INFO)(amdsmi_processor_handle, amdsmi_nic_bus_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_DRIVER_INFO)(amdsmi_processor_handle,
						      amdsmi_nic_driver_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_NUMA_INFO)(amdsmi_processor_handle,
						    amdsmi_nic_numa_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_PORT_INFO)(amdsmi_processor_handle,
						    amdsmi_nic_port_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_RDMA_DEV_INFO)(amdsmi_processor_handle,
							amdsmi_nic_rdma_devices_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_FW_INFO)(amdsmi_processor_handle, amdsmi_nic_fw_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_DEVICE_BDF)(amdsmi_processor_handle, amdsmi_bdf_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_PORT_STATISTICS)(amdsmi_processor_handle, uint32_t,
							  uint32_t*, amdsmi_nic_stat_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_VENDOR_STATISTICS)(amdsmi_processor_handle, uint32_t,
							    uint32_t*, amdsmi_nic_stat_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NIC_RDMA_PORT_STATISTICS)(amdsmi_processor_handle, uint32_t,
							       uint32_t*, amdsmi_nic_stat_t*);
typedef amdsmi_status_t (*AMDSMI_GET_TDI_STATE)(amdsmi_vf_handle_t, amdsmi_tdi_state_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_RAS_POLICY_INFO)(amdsmi_processor_handle,
							  amdsmi_gpu_ras_policy_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_NODE_HANDLE)(amdsmi_processor_handle, amdsmi_node_handle*);
typedef amdsmi_status_t (*AMDSMI_GET_NPM_INFO)(amdsmi_node_handle, amdsmi_npm_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_TRAY_INFO)(amdsmi_node_handle, amdsmi_tray_info_t*);
typedef amdsmi_status_t (*AMDSMI_GET_FABRIC_INFO)(amdsmi_processor_handle, amdsmi_fabric_info_t*);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_FABRIC_PPOD_CONFIG)(amdsmi_processor_handle,
							     const amdsmi_fabric_ppod_config_t*);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_FABRIC_VPOD_CONFIG)(amdsmi_processor_handle,
							     const amdsmi_fabric_vpod_config_t*);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_FABRIC_STATION_CONFIG)(
    amdsmi_processor_handle, const amdsmi_fabric_station_config_t*);
typedef amdsmi_status_t (*AMDSMI_GET_GPU_CC_MODE)(amdsmi_processor_handle, amdsmi_gpu_cc_mode_t*);
typedef amdsmi_status_t (*AMDSMI_SET_GPU_CC_MODE)(amdsmi_processor_handle, amdsmi_gpu_cc_mode_t);
typedef amdsmi_status_t (*AMDSMI_ALLOC_FABRIC_TELEMETRY)(amdsmi_processor_handle, uint32_t,
							 amdsmi_fabric_telemetry_t**);
typedef amdsmi_status_t (*AMDSMI_GET_FABRIC_TELEMETRY_DATA)(amdsmi_processor_handle,
							    amdsmi_fabric_telemetry_t*);
typedef amdsmi_status_t (*AMDSMI_FREE_FABRIC_TELEMETRY)(amdsmi_processor_handle,
							amdsmi_fabric_telemetry_t*);
typedef amdsmi_status_t (*AMDSMI_TOPO_GET_LINK_TYPE)(amdsmi_processor_handle,
						     amdsmi_processor_handle, uint64_t*,
						     amdsmi_link_type_t*);
/////
/////
/////

AMDSMI_INIT host_amdsmi_init;
AMDSMI_GET_PROCESSOR_HANDLES host_amdsmi_get_processor_handles;
AMDSMI_GET_PROCESSOR_TYPE host_amdsmi_get_processor_type;
AMDSMI_GET_NIC_PROCESSOR_HANDLES host_amdsmi_get_nic_processor_handles;
AMDSMI_GET_PROCESSOR_HANDLES_BY_TYPE host_amdsmi_get_processor_handles_by_type;
AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF host_amdsmi_get_processor_handle_from_bdf;
AMDSMI_GET_VF_HANDLE_FROM_BDF host_amdsmi_get_vf_handle_from_bdf;
AMDSMI_GET_VF_HANDLE_FROM_VF_INDEX host_amdsmi_get_vf_handle_from_vf_index;
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
AMDSMI_GET_GPU_PCI_BANDWIDTH host_amdsmi_get_gpu_pci_bandwidth;
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
AMDSMI_GET_XGMI_PLPD host_amdsmi_get_xgmi_plpd;
AMDSMI_SET_XGMI_PLPD host_amdsmi_set_xgmi_plpd;
AMDSMI_GET_GPU_TOTAL_ECC_COUNT host_amdsmi_get_gpu_total_ecc_count;
AMDSMI_GET_GPU_PTL_STATE host_amdsmi_get_gpu_ptl_state;
AMDSMI_SET_GPU_PTL_STATE host_amdsmi_set_gpu_ptl_state;
AMDSMI_GET_GPU_PTL_FORMATS host_amdsmi_get_gpu_ptl_formats;
AMDSMI_SET_GPU_PTL_FORMATS host_amdsmi_set_gpu_ptl_formats;

AMDSMI_GET_GPU_ECC_COUNT host_amdsmi_get_gpu_ecc_count;
AMDSMI_GET_GPU_ECC_ENABLED host_amdsmi_get_gpu_ecc_enabled;

AMDSMI_GET_GPU_BAD_PAGE_INFO host_amdsmi_get_gpu_bad_page_info;
AMDSMI_GET_GPU_RAS_FEATURE_INFO host_amdsmi_get_gpu_ras_feature_info;
AMDSMI_GET_BAD_PAGE_THRESHOLD host_amdsmi_get_bad_page_threshold;
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
AMDSMI_GET_GLOBAL_PARTITION host_amdsmi_get_accelerator_partition_profile_config_global;
AMDSMI_GET_CURR_ACCELERATOR_PARTITION host_amdsmi_get_partition_profile;
AMDSMI_GET_MEMORY_PARTITION_CAPS host_amdsmi_get_memory_partition_caps;
AMDSMI_GET_CURR_MEMORY_PARTITION host_amdsmi_get_curr_memory_partition;
AMDSMI_SET_ACCELERATOR_PARTITION host_amdsmi_set_gpu_accelerator_partition_command;
AMDSMI_SET_MEMORY_PARTITION host_amdsmi_set_gpu_memory_partition_command;
AMDSMI_GET_MEMORY_PARTITION_CONFIG host_amdsmi_get_gpu_memory_partition_config;
AMDSMI_GET_GPU_CPER_ENTRIES host_amdsmi_get_gpu_cper_entries;
AMDSMI_GET_FABRIC_CPER_ENTRIES host_amdsmi_get_fabric_cper_entries;
AMDSMI_TOPO_GET_P2P_STATUS host_amdsmi_topo_get_p2p_status;
AMDSMI_GET_GPU_VIRTUALIZATION_MODE host_amdsmi_get_gpu_virtualization_mode;
AMDSMI_GET_CPU_AFFINITY_WITH_SCOPE host_amdsmi_get_cpu_affinity_with_scope;
AMDSMI_TOPO_GET_NUMA_NODE_NUMBER host_amdsmi_topo_get_numa_node_number;
AMDSMI_GET_AFIDS_FROM_CPER host_amdsmi_get_afids_from_cper;
AMDSMI_RESET_GPU host_amdsmi_reset_gpu;
AMDSMI_GET_VF_HBM_INFO host_amdsmi_get_vf_hbm_info;
AMDSMI_GET_TDI_STATE host_amdsmi_get_tdi_state;
AMDSMI_GET_GPU_CC_MODE host_amdsmi_get_gpu_cc_mode;
AMDSMI_SET_GPU_CC_MODE host_amdsmi_set_gpu_cc_mode;
AMDSMI_GET_GPU_RAS_POLICY_INFO host_amdsmi_get_gpu_ras_policy_info;

AMDSMI_GET_NIC_ASIC_INFO host_amdsmi_get_nic_asic_info;
AMDSMI_GET_NIC_BUS_INFO host_amdsmi_get_nic_bus_info;
AMDSMI_GET_NIC_DRIVER_INFO host_amdsmi_get_nic_driver_info;
AMDSMI_GET_NIC_NUMA_INFO host_amdsmi_get_nic_numa_info;
AMDSMI_GET_NIC_PORT_INFO host_amdsmi_get_nic_port_info;
AMDSMI_GET_NIC_RDMA_DEV_INFO host_amdsmi_get_nic_rdma_dev_info;
AMDSMI_GET_NIC_FW_INFO host_amdsmi_get_nic_fw_info;
AMDSMI_GET_NIC_DEVICE_BDF host_amdsmi_get_nic_device_bdf;
AMDSMI_GET_NIC_PORT_STATISTICS host_amdsmi_get_nic_port_statistics;
AMDSMI_GET_NIC_RDMA_PORT_STATISTICS host_amdsmi_get_nic_rdma_port_statistics;
AMDSMI_GET_NIC_VENDOR_STATISTICS host_amdsmi_get_nic_vendor_statistics;
AMDSMI_TOPO_GET_LINK_TYPE host_amdsmi_topo_get_link_type;

AMDSMI_GET_NODE_HANDLE host_amdsmi_get_node_handle;
AMDSMI_GET_NPM_INFO host_amdsmi_get_npm_info;
AMDSMI_GET_TRAY_INFO host_amdsmi_get_tray_info;
AMDSMI_GET_FABRIC_INFO host_amdsmi_get_gpu_fabric_info;

AMDSMI_SET_GPU_FABRIC_PPOD_CONFIG host_amdsmi_set_gpu_fabric_ppod_config;
AMDSMI_SET_GPU_FABRIC_VPOD_CONFIG host_amdsmi_set_gpu_fabric_vpod_config;
AMDSMI_SET_GPU_FABRIC_STATION_CONFIG host_amdsmi_set_gpu_fabric_station_config;

AMDSMI_ALLOC_FABRIC_TELEMETRY host_amdsmi_alloc_fabric_telemetry;
AMDSMI_GET_FABRIC_TELEMETRY_DATA host_amdsmi_get_fabric_telemetry_data;
AMDSMI_FREE_FABRIC_TELEMETRY host_amdsmi_free_fabric_telemetry;

AmdSmiApiHost::AmdSmiApiHost() : loader_("\\libamdsmi_host.dll", "libamdsmi.so", true)
{
	if (!loader_.is_loaded()) {
		throw SmiToolSMILIBErrorException(34);
	}

	host_amdsmi_init = (AMDSMI_INIT)loader_.get_symbol("amdsmi_init");
	host_amdsmi_get_processor_handles =
	    (AMDSMI_GET_PROCESSOR_HANDLES)loader_.get_symbol("amdsmi_get_processor_handles");
	host_amdsmi_get_nic_processor_handles =
	    (AMDSMI_GET_PROCESSOR_HANDLES)loader_.get_symbol("amdsmi_get_nic_processor_handles");
	host_amdsmi_get_processor_handles_by_type =
	    (AMDSMI_GET_PROCESSOR_HANDLES_BY_TYPE)loader_.get_symbol(
		"amdsmi_get_processor_handles_by_type");
	host_amdsmi_get_processor_type =
	    (AMDSMI_GET_PROCESSOR_TYPE)loader_.get_symbol("amdsmi_get_processor_type");
	host_amdsmi_get_processor_handle_from_bdf =
	    (AMDSMI_GET_PROCESSOR_HANDLE_FROM_BDF)loader_.get_symbol(
		"amdsmi_get_processor_handle_from_bdf");
	host_amdsmi_get_vf_handle_from_bdf =
	    (AMDSMI_GET_VF_HANDLE_FROM_BDF)loader_.get_symbol("amdsmi_get_vf_handle_from_bdf");
	host_amdsmi_get_gpu_asic_info =
	    (AMDSMI_GET_GPU_ASIC_INFO)loader_.get_symbol("amdsmi_get_gpu_asic_info");
	host_amdsmi_get_gpu_vram_info =
	    (AMDSMI_GET_GPU_VRAM_INFO)loader_.get_symbol("amdsmi_get_gpu_vram_info");
	host_amdsmi_shut_down = (AMDSMI_SHUT_DOWN)loader_.get_symbol("amdsmi_shut_down");

	host_amdsmi_get_gpu_device_bdf =
	    (AMDSMI_GET_GPU_DEVICE_BDF)loader_.get_symbol("amdsmi_get_gpu_device_bdf");
	host_amdsmi_get_gpu_device_uuid =
	    (AMDSMI_GET_GPU_DEVICE_UUID)loader_.get_symbol("amdsmi_get_gpu_device_uuid");

	host_amdsmi_get_vf_bdf	= (AMDSMI_GET_VF_BDF)loader_.get_symbol("amdsmi_get_vf_bdf");
	host_amdsmi_get_vf_uuid = (AMDSMI_GET_VF_UUID)loader_.get_symbol("amdsmi_get_vf_uuid");

	// amdsmi_get_gpu_driver_version = (AMDSMI_GET_GPU_DRIVER_VERSION)loader_.get_symbol(
	//  "amdsmi_get_gpu_driver_version");
	host_amdsmi_get_gpu_driver_info =
	    (AMDSMI_GET_GPU_DRIVER_INFO)loader_.get_symbol("amdsmi_get_gpu_driver_info");
	host_amdsmi_get_gpu_driver_model =
	    (AMDSMI_GET_GPU_DRIVER_MODEL)loader_.get_symbol("amdsmi_get_gpu_driver_model");
	host_amdsmi_get_power_cap_info =
	    (AMDSMI_GET_POWER_CAP_INFO)loader_.get_symbol("amdsmi_get_power_cap_info");

	// amdsmi_get_pcie_link_caps = (AMDSMI_GET_PCIE_LINK_CAPS)loader_.get_symbol(
	// 	 "amdsmi_get_pcie_link_caps");
	host_amdsmi_get_pcie_info =
	    (AMDSMI_GET_PCIE_INFO)loader_.get_symbol("amdsmi_get_pcie_info");
	host_amdsmi_get_gpu_pci_bandwidth =
	    (AMDSMI_GET_GPU_PCI_BANDWIDTH)loader_.get_symbol("amdsmi_get_gpu_pci_bandwidth");
	host_amdsmi_get_fb_layout =
	    (AMDSMI_GET_FB_LAYOUT)loader_.get_symbol("amdsmi_get_fb_layout");
	host_amdsmi_get_gpu_vbios_info =
	    (AMDSMI_GET_GPU_VBIOS_INFO)loader_.get_symbol("amdsmi_get_gpu_vbios_info");

	host_amdsmi_get_gpu_board_info =
	    (AMDSMI_GET_GPU_BOARD_INFO)loader_.get_symbol("amdsmi_get_gpu_board_info");
	host_amdsmi_get_fw_info = (AMDSMI_GET_FW_INFO)loader_.get_symbol("amdsmi_get_fw_info");
	host_amdsmi_get_fw_error_records =
	    (AMDSMI_GET_FW_ERROR_RECORDS)loader_.get_symbol("amdsmi_get_fw_error_records");
	host_amdsmi_get_dfc_fw_table =
	    (AMDSMI_GET_DFC_FW_TABLE)loader_.get_symbol("amdsmi_get_dfc_fw_table");

	host_amdsmi_get_gpu_activity =
	    (AMDSMI_GET_GPU_ACTIVITY)loader_.get_symbol("amdsmi_get_gpu_activity");
	// amdsmi_get_pcie_link_status = (AMDSMI_GET_PCIE_LINK_STATUS)loader_.get_symbol(
	// 	 "amdsmi_get_pcie_link_status");
	host_amdsmi_get_power_info =
	    (AMDSMI_GET_POWER_INFO)loader_.get_symbol("amdsmi_get_power_info");
	host_amdsmi_set_power_cap =
	    (AMDSMI_SET_POWER_CAP)loader_.get_symbol("amdsmi_set_power_cap");
	host_amdsmi_is_gpu_power_management_enabled =
	    (AMDSMI_IS_GPU_POWER_MANAGEMENT_ENABLED)loader_.get_symbol(
		"amdsmi_is_gpu_power_management_enabled");
	host_amdsmi_get_clock_info =
	    (AMDSMI_GET_CLOCK_INFO)loader_.get_symbol("amdsmi_get_clock_info");
	host_amdsmi_get_temp_metric =
	    (AMDSMI_GET_TEMP_METRIC)loader_.get_symbol("amdsmi_get_temp_metric");
	host_amdsmi_get_gpu_cache_info =
	    (AMDSMI_GET_GPU_CACHE_INFO)loader_.get_symbol("amdsmi_get_gpu_cache_info");
	host_amdsmi_get_soc_pstate =
	    (AMDSMI_GET_SOC_PSTATE)loader_.get_symbol("amdsmi_get_soc_pstate");
	host_amdsmi_set_soc_pstate =
	    (AMDSMI_SET_SOC_PSTATE)loader_.get_symbol("amdsmi_set_soc_pstate");
	host_amdsmi_get_xgmi_plpd =
	    (AMDSMI_GET_XGMI_PLPD)loader_.get_symbol("amdsmi_get_xgmi_plpd");
	host_amdsmi_set_xgmi_plpd =
	    (AMDSMI_SET_XGMI_PLPD)loader_.get_symbol("amdsmi_set_xgmi_plpd");
	host_amdsmi_get_gpu_total_ecc_count =
	    (AMDSMI_GET_GPU_TOTAL_ECC_COUNT)loader_.get_symbol("amdsmi_get_gpu_total_ecc_count");
	host_amdsmi_get_gpu_ptl_state =
	    (AMDSMI_GET_GPU_PTL_STATE)loader_.get_symbol("amdsmi_get_gpu_ptl_state");
	host_amdsmi_set_gpu_ptl_state =
	    (AMDSMI_SET_GPU_PTL_STATE)loader_.get_symbol("amdsmi_set_gpu_ptl_state");
	host_amdsmi_get_gpu_ptl_formats =
	    (AMDSMI_GET_GPU_PTL_FORMATS)loader_.get_symbol("amdsmi_get_gpu_ptl_formats");
	host_amdsmi_set_gpu_ptl_formats =
	    (AMDSMI_SET_GPU_PTL_FORMATS)loader_.get_symbol("amdsmi_set_gpu_ptl_formats");

	host_amdsmi_get_gpu_ecc_count =
	    (AMDSMI_GET_GPU_ECC_COUNT)loader_.get_symbol("amdsmi_get_gpu_ecc_count");
	host_amdsmi_get_gpu_ecc_enabled =
	    (AMDSMI_GET_GPU_ECC_ENABLED)loader_.get_symbol("amdsmi_get_gpu_ecc_enabled");

	host_amdsmi_get_gpu_bad_page_info =
	    (AMDSMI_GET_GPU_BAD_PAGE_INFO)loader_.get_symbol("amdsmi_get_gpu_bad_page_info");
	host_amdsmi_get_gpu_ras_feature_info =
	    (AMDSMI_GET_GPU_RAS_FEATURE_INFO)loader_.get_symbol("amdsmi_get_gpu_ras_feature_info");
	host_amdsmi_get_bad_page_threshold =
	    (AMDSMI_GET_BAD_PAGE_THRESHOLD)loader_.get_symbol("amdsmi_get_bad_page_threshold");
	host_amdsmi_get_num_vf = (AMDSMI_GET_NUM_VF)loader_.get_symbol("amdsmi_get_num_vf");
	host_amdsmi_set_num_vf = (AMDSMI_SET_NUM_VF)loader_.get_symbol("amdsmi_set_num_vf");
	host_amdsmi_get_vf_partition_info =
	    (AMDSMI_GET_VF_PARTITION_INFO)loader_.get_symbol("amdsmi_get_vf_partition_info");
	host_amdsmi_get_vf_info = (AMDSMI_GET_VF_INFO)loader_.get_symbol("amdsmi_get_vf_info");
	host_amdsmi_get_vf_data = (AMDSMI_GET_VF_DATA)loader_.get_symbol("amdsmi_get_vf_data");
	host_amdsmi_get_guest_data =
	    (AMDSMI_GET_GUEST_DATA)loader_.get_symbol("amdsmi_get_guest_data");
	host_amdsmi_get_vf_fw_info =
	    (AMDSMI_GET_VF_FW_INFO)loader_.get_symbol("amdsmi_get_vf_fw_info");
	host_amdsmi_get_partition_profile_info =
	    (AMDSMI_GET_PARTITION_PROFILE_INFO)loader_.get_symbol(
		"amdsmi_get_partition_profile_info");
	host_amdsmi_get_link_topology =
	    (AMDSMI_GET_LINK_TOPOLOGY)loader_.get_symbol("amdsmi_get_link_topology");
	host_amdsmi_get_link_metrics =
	    (AMDSMI_GET_LINK_METRICS)loader_.get_symbol("amdsmi_get_link_metrics");
	host_amdsmi_get_xgmi_fb_sharing_caps =
	    (AMDSMI_GET_XGMI_FB_SHARING_CAPS)loader_.get_symbol("amdsmi_get_xgmi_fb_sharing_caps");
	host_amdsmi_get_xgmi_fb_sharing_mode_info =
	    (AMDSMI_GET_XGMI_FB_SHARING_MODE_INFO)loader_.get_symbol(
		"amdsmi_get_xgmi_fb_sharing_mode_info");

	host_amdsmi_set_xgmi_fb_sharing_mode_info =
	    (AMDSMI_SET_XGMI_FB_SHARING_MODE_INFO)loader_.get_symbol(
		"amdsmi_set_xgmi_fb_sharing_mode");

	host_amdsmi_set_xgmi_fb_sharing_mode_v2 =
	    (AMDSMI_SET_XGMI_FB_SHARING_MODE_V2)loader_.get_symbol(
		"amdsmi_set_xgmi_fb_sharing_mode_v2");

	host_amdsmi_event_create = (AMDSMI_EVENT_CREATE)loader_.get_symbol("amdsmi_event_create");
	host_amdsmi_event_read	 = (AMDSMI_EVENT_READ)loader_.get_symbol("amdsmi_event_read");
	host_amdsmi_event_destroy =
	    (AMDSMI_EVENT_DESTROY)loader_.get_symbol("amdsmi_event_destroy");

	host_amdsmi_get_gpu_metrics =
	    (AMDSMI_GET_GPU_METRICS)loader_.get_symbol("amdsmi_get_gpu_metrics");

	host_amdsmi_get_lib_version =
	    (AMDSMI_GET_LIB_VERSION)loader_.get_symbol("amdsmi_get_lib_version");

	host_amdsmi_clear_vf_fb = (AMDSMI_CLEAR_VF_FB)loader_.get_symbol("amdsmi_clear_vf_fb");

	host_amdsmi_get_partition_profile_config =
	    (AMDSMI_GET_PARTITION_PROFILE_CONFIG)loader_.get_symbol(
		"amdsmi_get_gpu_accelerator_partition_profile_config");
	host_amdsmi_get_accelerator_partition_profile_config_global =
	    (AMDSMI_GET_GLOBAL_PARTITION)loader_.get_symbol(
		"amdsmi_get_gpu_accelerator_partition_profile_config_global");
	host_amdsmi_get_memory_partition_caps =
	    (AMDSMI_GET_MEMORY_PARTITION_CAPS)loader_.get_symbol(
		"amdsmi_get_gpu_memory_partition_caps");
	host_amdsmi_get_curr_memory_partition =
	    (AMDSMI_GET_CURR_MEMORY_PARTITION)loader_.get_symbol(
		"amdsmi_get_gpu_memory_partition_setting");
	host_amdsmi_set_gpu_accelerator_partition_command =
	    (AMDSMI_SET_ACCELERATOR_PARTITION)loader_.get_symbol(
		"amdsmi_set_gpu_accelerator_partition_profile");
	host_amdsmi_set_gpu_memory_partition_command =
	    (AMDSMI_SET_MEMORY_PARTITION)loader_.get_symbol("amdsmi_set_gpu_memory_partition_mode");
	host_amdsmi_get_partition_profile =
	    (AMDSMI_GET_CURR_ACCELERATOR_PARTITION)loader_.get_symbol(
		"amdsmi_get_gpu_accelerator_partition_profile");
	host_amdsmi_get_gpu_memory_partition_config =
	    (AMDSMI_GET_MEMORY_PARTITION_CONFIG)loader_.get_symbol(
		"amdsmi_get_gpu_memory_partition_config");

	host_amdsmi_get_gpu_cper_entries =
	    (AMDSMI_GET_GPU_CPER_ENTRIES)loader_.get_symbol("amdsmi_get_gpu_cper_entries");
	host_amdsmi_get_fabric_cper_entries =
	    (AMDSMI_GET_FABRIC_CPER_ENTRIES)loader_.get_symbol("amdsmi_get_fabric_cper_entries");

	host_amdsmi_topo_get_p2p_status =
	    (AMDSMI_TOPO_GET_P2P_STATUS)loader_.get_symbol("amdsmi_topo_get_p2p_status");

	host_amdsmi_get_gpu_virtualization_mode =
	    (AMDSMI_GET_GPU_VIRTUALIZATION_MODE)loader_.get_symbol(
		"amdsmi_get_gpu_virtualization_mode");
	host_amdsmi_get_cpu_affinity_with_scope =
	    (AMDSMI_GET_CPU_AFFINITY_WITH_SCOPE)loader_.get_symbol(
		"amdsmi_get_cpu_affinity_with_scope");
	host_amdsmi_topo_get_numa_node_number =
	    (AMDSMI_TOPO_GET_NUMA_NODE_NUMBER)loader_.get_symbol(
		"amdsmi_topo_get_numa_node_number");
	host_amdsmi_get_afids_from_cper =
	    (AMDSMI_GET_AFIDS_FROM_CPER)loader_.get_symbol("amdsmi_get_afids_from_cper");
	host_amdsmi_reset_gpu = (AMDSMI_RESET_GPU)loader_.get_symbol("amdsmi_reset_gpu");
	host_amdsmi_get_nic_asic_info =
	    (AMDSMI_GET_NIC_ASIC_INFO)loader_.get_symbol("amdsmi_get_nic_asic_info");
	host_amdsmi_get_nic_bus_info =
	    (AMDSMI_GET_NIC_BUS_INFO)loader_.get_symbol("amdsmi_get_nic_bus_info");
	host_amdsmi_get_nic_driver_info =
	    (AMDSMI_GET_NIC_DRIVER_INFO)loader_.get_symbol("amdsmi_get_nic_driver_info");
	host_amdsmi_get_nic_numa_info =
	    (AMDSMI_GET_NIC_NUMA_INFO)loader_.get_symbol("amdsmi_get_nic_numa_info");
	host_amdsmi_get_nic_port_info =
	    (AMDSMI_GET_NIC_PORT_INFO)loader_.get_symbol("amdsmi_get_nic_port_info");
	host_amdsmi_get_nic_rdma_dev_info =
	    (AMDSMI_GET_NIC_RDMA_DEV_INFO)loader_.get_symbol("amdsmi_get_nic_rdma_dev_info");
	host_amdsmi_get_nic_fw_info =
	    (AMDSMI_GET_NIC_FW_INFO)loader_.get_symbol("amdsmi_get_nic_fw_info");
	host_amdsmi_get_nic_device_bdf =
	    (AMDSMI_GET_NIC_DEVICE_BDF)loader_.get_symbol("amdsmi_get_nic_device_bdf");
	host_amdsmi_get_nic_port_statistics =
	    (AMDSMI_GET_NIC_PORT_STATISTICS)loader_.get_symbol("amdsmi_get_nic_port_statistics");
	host_amdsmi_get_nic_vendor_statistics =
	    (AMDSMI_GET_NIC_VENDOR_STATISTICS)loader_.get_symbol(
		"amdsmi_get_nic_vendor_statistics");
	host_amdsmi_get_nic_rdma_port_statistics =
	    (AMDSMI_GET_NIC_RDMA_PORT_STATISTICS)loader_.get_symbol(
		"amdsmi_get_nic_rdma_port_statistics");
	host_amdsmi_get_vf_hbm_info =
	    (AMDSMI_GET_VF_HBM_INFO)loader_.get_symbol("amdsmi_get_vf_hbm_info");
	host_amdsmi_get_tdi_state =
	    (AMDSMI_GET_TDI_STATE)loader_.get_symbol("amdsmi_get_tdi_state");
	host_amdsmi_get_gpu_cc_mode =
	    (AMDSMI_GET_GPU_CC_MODE)loader_.get_symbol("amdsmi_get_gpu_cc_mode");
	host_amdsmi_set_gpu_cc_mode =
	    (AMDSMI_SET_GPU_CC_MODE)loader_.get_symbol("amdsmi_set_gpu_cc_mode");
	host_amdsmi_get_gpu_ras_policy_info =
	    (AMDSMI_GET_GPU_RAS_POLICY_INFO)loader_.get_symbol("amdsmi_get_gpu_ras_policy_info");
	host_amdsmi_get_node_handle =
	    (AMDSMI_GET_NODE_HANDLE)loader_.get_symbol("amdsmi_get_node_handle");
	host_amdsmi_get_npm_info = (AMDSMI_GET_NPM_INFO)loader_.get_symbol("amdsmi_get_npm_info");
	host_amdsmi_get_tray_info =
	    (AMDSMI_GET_TRAY_INFO)loader_.get_symbol("amdsmi_get_tray_info");
	host_amdsmi_get_gpu_fabric_info =
	    (AMDSMI_GET_FABRIC_INFO)loader_.get_symbol("amdsmi_get_gpu_fabric_info");
	host_amdsmi_set_gpu_fabric_ppod_config =
	    (AMDSMI_SET_GPU_FABRIC_PPOD_CONFIG)loader_.get_symbol(
		"amdsmi_set_gpu_fabric_ppod_config");
	host_amdsmi_set_gpu_fabric_vpod_config =
	    (AMDSMI_SET_GPU_FABRIC_VPOD_CONFIG)loader_.get_symbol(
		"amdsmi_set_gpu_fabric_vpod_config");
	host_amdsmi_set_gpu_fabric_station_config =
	    (AMDSMI_SET_GPU_FABRIC_STATION_CONFIG)loader_.get_symbol(
		"amdsmi_set_gpu_fabric_station_config");

	host_amdsmi_alloc_fabric_telemetry =
	    (AMDSMI_ALLOC_FABRIC_TELEMETRY)loader_.get_symbol("amdsmi_alloc_fabric_telemetry");
	host_amdsmi_get_fabric_telemetry_data =
	    (AMDSMI_GET_FABRIC_TELEMETRY_DATA)loader_.get_symbol(
		"amdsmi_get_fabric_telemetry_data");
	host_amdsmi_free_fabric_telemetry =
	    (AMDSMI_FREE_FABRIC_TELEMETRY)loader_.get_symbol("amdsmi_free_fabric_telemetry");

	host_amdsmi_topo_get_link_type =
	    (AMDSMI_TOPO_GET_LINK_TYPE)loader_.get_symbol("amdsmi_topo_get_link_type");
	int ret = host_amdsmi_init(AMDSMI_INIT_AMD_GPUS);
	if (ret != AMDSMI_STATUS_SUCCESS) {
		if (ret == AMDSMI_STATUS_NO_PERM) {
			throw SmiToolPermissionDeniedException();
		}
		throw SmiToolSMILIBErrorException(ret);
	}

	uint32_t num_processors	    = 0;
	amdsmi_socket_handle socket = NULL;
	ret = host_amdsmi_get_processor_handles(socket, &num_processors, NULL);
	if (ret == AMDSMI_STATUS_SUCCESS && num_processors > 0) {
		std::vector<amdsmi_processor_handle> processors(num_processors);
		ret = host_amdsmi_get_processor_handles(socket, &num_processors, processors.data());
		if (ret == AMDSMI_STATUS_SUCCESS) {
			amdsmi_asic_info_t asic_info {};
			ret = host_amdsmi_get_gpu_asic_info(processors[0], &asic_info);
			if (ret == AMDSMI_STATUS_SUCCESS) {
				AmdSmiPlatform::getInstance()
				    .refine_gc_12_1_detection_from_graphics_version(
					asic_info.target_graphics_version);
			}
		}
	}
};

AmdSmiApiHost::~AmdSmiApiHost()
{
	int ret = host_amdsmi_shut_down();
	if (ret != AMDSMI_STATUS_SUCCESS)
		printf("AMDSMI failed to finish\n");
}
