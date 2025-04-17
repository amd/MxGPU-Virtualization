#!/usr/bin/env python3

#
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.


from .amdsmi_interface import amdsmi_init
from .amdsmi_interface import amdsmi_shut_down
from .amdsmi_interface import amdsmi_get_processor_handles
from .amdsmi_interface import amdsmi_get_processor_handle_from_bdf
from .amdsmi_interface import amdsmi_get_index_from_processor_handle
from .amdsmi_interface import amdsmi_get_processor_handle_from_index
from .amdsmi_interface import amdsmi_get_vf_handle_from_bdf
from .amdsmi_interface import amdsmi_get_gpu_device_bdf
from .amdsmi_interface import amdsmi_get_vf_bdf
from .amdsmi_interface import amdsmi_get_vf_handle_from_vf_index
from .amdsmi_interface import amdsmi_get_vf_handle_from_uuid
from .amdsmi_interface import amdsmi_get_processor_handle_from_uuid
from .amdsmi_interface import amdsmi_get_gpu_total_ecc_count
from .amdsmi_interface import amdsmi_get_gpu_ecc_count
from .amdsmi_interface import amdsmi_get_gpu_ecc_enabled
from .amdsmi_interface import amdsmi_status_code_to_string
from .amdsmi_interface import amdsmi_get_gpu_ras_feature_info
from .amdsmi_interface import amdsmi_get_gpu_bad_page_info
from .amdsmi_interface import amdsmi_get_gpu_asic_info
from .amdsmi_interface import amdsmi_get_pcie_info
from .amdsmi_interface import amdsmi_get_power_cap_info
from .amdsmi_interface import amdsmi_get_fb_layout
from .amdsmi_interface import amdsmi_get_gpu_activity
from .amdsmi_interface import amdsmi_get_power_info
from .amdsmi_interface import amdsmi_set_power_cap
from .amdsmi_interface import amdsmi_get_temp_metric
from .amdsmi_interface import amdsmi_get_gpu_cache_info
from .amdsmi_interface import amdsmi_get_clock_info
from .amdsmi_interface import amdsmi_get_gpu_vram_info
from .amdsmi_interface import amdsmi_get_gpu_vbios_info
from .amdsmi_interface import amdsmi_get_fw_info
from .amdsmi_interface import amdsmi_get_gpu_board_info
from .amdsmi_interface import amdsmi_get_num_vf
from .amdsmi_interface import amdsmi_get_vf_partition_info
from .amdsmi_interface import amdsmi_set_num_vf
from .amdsmi_interface import amdsmi_clear_vf_fb
from .amdsmi_interface import amdsmi_get_vf_data
from .amdsmi_interface import amdsmi_get_vf_info
from .amdsmi_interface import amdsmi_get_gpu_driver_info
from .amdsmi_interface import amdsmi_get_gpu_device_uuid
from .amdsmi_interface import amdsmi_get_vf_uuid
from .amdsmi_interface import amdsmi_get_guest_data
from .amdsmi_interface import amdsmi_get_dfc_fw_table
from .amdsmi_interface import amdsmi_get_vf_fw_info
from .amdsmi_interface import amdsmi_get_fw_error_records
from .amdsmi_interface import amdsmi_get_partition_profile_info
from .amdsmi_interface import amdsmi_is_gpu_power_management_enabled
from .amdsmi_interface import amdsmi_get_link_metrics
from .amdsmi_interface import amdsmi_get_link_topology
from .amdsmi_interface import amdsmi_get_xgmi_fb_sharing_caps
from .amdsmi_interface import amdsmi_get_xgmi_fb_sharing_mode_info
from .amdsmi_interface import amdsmi_set_xgmi_fb_sharing_mode
from .amdsmi_interface import amdsmi_set_xgmi_fb_sharing_mode_v2
from .amdsmi_interface import amdsmi_get_gpu_metrics
from .amdsmi_interface import amdsmi_get_lib_version
from .amdsmi_interface import amdsmi_get_gpu_memory_partition_config
from .amdsmi_interface import amdsmi_set_gpu_accelerator_partition_profile
from .amdsmi_interface import amdsmi_set_gpu_memory_partition_mode
from .amdsmi_interface import amdsmi_get_gpu_accelerator_partition_profile
from .amdsmi_interface import amdsmi_get_gpu_accelerator_partition_profile_config
from .amdsmi_interface import amdsmi_get_soc_pstate
from .amdsmi_interface import amdsmi_set_soc_pstate
from .amdsmi_interface import amdsmi_get_gpu_driver_model
from .amdsmi_interface import amdsmi_gpu_get_cper_entries

from .amdsmi_interface import AmdSmiTemperatureType
from .amdsmi_interface import AmdSmiTemperatureMetric
from .amdsmi_interface import AmdSmiClkType
from .amdsmi_interface import AmdSmiGuardState
from .amdsmi_interface import AmdSmiVramType
from .amdsmi_interface import AmdSmiVramVendor
from .amdsmi_interface import AmdSmiGuardType
from .amdsmi_interface import AmdSmiVfState
from .amdsmi_interface import AmdSmiFwBlock
from .amdsmi_interface import AmdSmiEventReader
from .amdsmi_interface import AmdSmiEventCategory
from .amdsmi_interface import AmdSmiEventCategoryGpu
from .amdsmi_interface import AmdSmiEventCategoryDriver
from .amdsmi_interface import AmdSmiEventCategoryFw
from .amdsmi_interface import AmdSmiEventCategoryReset
from .amdsmi_interface import AmdSmiEventCategoryIOV
from .amdsmi_interface import AmdSmiEventCategoryEcc
from .amdsmi_interface import AmdSmiEventCategoryPP
from .amdsmi_interface import AmdSmiEventCategorySched
from .amdsmi_interface import AmdSmiEventCategoryVf
from .amdsmi_interface import AmdSmiEventCategoryVbios
from .amdsmi_interface import AmdSmiEventCategoryGuard
from .amdsmi_interface import AmdSmiEventCategoryGpumon
from .amdsmi_interface import AmdSmiEventCategoryMmsch
from .amdsmi_interface import AmdSmiEventCategoryXgmi
from .amdsmi_interface import AmdSmiEventSeverity
from .amdsmi_interface import AmdSmiInitFlags
from .amdsmi_interface import AmdSmiGuestFwName
from .amdsmi_interface import AmdSmiGuestFwLoadStatus
from .amdsmi_interface import AmdSmiProfileCapabilityType
from .amdsmi_interface import AmdSmiLinkType
from .amdsmi_interface import AmdSmiLinkStatus
from .amdsmi_interface import AmdSmiXgmiFbSharingMode
from .amdsmi_interface import AmdSmiCardFormFactor
from .amdsmi_interface import AmdSmiGpuBlock
from .amdsmi_interface import AmdSmiEccCorrectionSchemaSupport
from .amdsmi_interface import AmdSmiMetricUnit
from .amdsmi_interface import AmdSmiMetricName
from .amdsmi_interface import AmdSmiMetricCategory
from .amdsmi_interface import AmdSmiAcceleratorPartitionResource
from .amdsmi_interface import AmdSmiAcceleratorPartitionSetting
from .amdsmi_interface import AmdSmiMemoryPartitionSetting
from .amdsmi_interface import AmdSmiCperErrorSeverity

from .amdsmi_exception import AmdSmiLibraryException
from .amdsmi_exception import AmdSmiRetryException
from .amdsmi_exception import AmdSmiParameterException
from .amdsmi_exception import AmdSmiKeyException
from .amdsmi_exception import AmdSmiBdfFormatException
from .amdsmi_exception import AmdSmiTimeoutException
from .amdsmi_exception import AmdSmiException
from .amdsmi_exception import AmdSmiRetCode
