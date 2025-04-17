/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "gtest/gtest.h"

extern "C" {
#include "amdsmi.h"
}

/*
TODO: Fix memory layout for these structures:
	X(amdsmi_profile_caps_info_t) \
	X(amdsmi_profile_info_t) \
	X(amdsmi_xgmi_fb_sharing_caps_t) \
	X(amdsmi_dfc_fw_ta_uuid_t) \
	X(amdsmi_fw_load_error_record_t) \
	X(amdsmi_nps_caps_t) \
	X(amdsmi_memory_partition_config_t) \
	X(amdsmi_dpm_policy_entry_t) \
	X(amdsmi_version_t)
*/

#define AMD_SMI_STRUCTURES_8 \
	X(amdsmi_bdf_t) \
	X(amdsmi_vf_handle_t) \
	X(amdsmi_dfc_fw_white_list_t) \
	X(amdsmi_eeprom_table_record_t)

#define AMD_SMI_STRUCTURES_32 \
	X(amdsmi_ras_feature_t) \
	X(amdsmi_clk_info_t) \
	X(amdsmi_vf_fb_info_t) \
	X(amdsmi_fw_load_error_record_t) \

#define AMD_SMI_STRUCTURES_64 \
	X(amdsmi_pcie_info_t) \
	X(amdsmi_power_cap_info_t) \
	X(amdsmi_vbios_info_t) \
	X(amdsmi_gpu_cache_info_t) \
	X(amdsmi_fw_info_t) \
	X(amdsmi_asic_info_t) \
	X(amdsmi_driver_info_t) \
	X(amdsmi_power_info_t) \
	X(amdsmi_error_count_t) \
	X(amdsmi_engine_usage_t) \
	X(amdsmi_event_entry_t) \
	X(amdsmi_board_info_t) \
	X(amdsmi_pf_fb_info_t) \
	X(amdsmi_partition_info_t) \
	X(amdsmi_guard_info_t) \
	X(amdsmi_vf_info_t) \
	X(amdsmi_guest_data_t) \
	X(amdsmi_dfc_fw_header_t) \
	X(amdsmi_dfc_fw_data_t) \
	X(amdsmi_dfc_fw_t) \
	X(amdsmi_fw_error_record_t) \
	X(amdsmi_link_metrics_t) \
	X(amdsmi_link_topology_t) \
	X(amdsmi_vram_info_t) \
	X(amdsmi_metric_t) \
	X(amdsmi_sched_info_t) \
	X(amdsmi_vf_data_t) \
	X(amdsmi_accelerator_partition_profile_t) \
	X(amdsmi_accelerator_partition_resource_profile_t) \
	X(amdsmi_accelerator_partition_profile_config_t) \
	X(amdsmi_dpm_policy_t)


TEST(AmdSmiStructuresAlignment, All_Structs_Size_Should_Be_In_Multiple_Of_8_uint8t)
{
	size_t size_alignment_divisor = 8 * sizeof(uint8_t);
	size_t address_alignment_divisor = 8;

	#define X(struct_name) \
		ASSERT_EQ(sizeof(struct_name) % size_alignment_divisor, 0); \
		struct_name var_##struct_name; \
		ASSERT_EQ(reinterpret_cast<uintptr_t>(&var_##struct_name) % address_alignment_divisor, 0);
	AMD_SMI_STRUCTURES_8
	#undef X
}

TEST(AmdSmiStructuresAlignment, All_Structs_Size_Should_Be_In_Multiple_Of_8_uint32t)
{
	size_t size_alignment_divisor = 8 * sizeof(uint32_t);
	size_t address_alignment_divisor = 8;

	#define X(struct_name) \
		ASSERT_EQ(sizeof(struct_name) % size_alignment_divisor, 0); \
		struct_name var_##struct_name; \
		ASSERT_EQ(reinterpret_cast<uintptr_t>(&var_##struct_name) % address_alignment_divisor, 0);
	AMD_SMI_STRUCTURES_32
	#undef X
}

TEST(AmdSmiStructuresAlignment, All_Structs_Size_Should_Be_In_Multiple_Of_8_uint64t)
{
	size_t size_alignment_divisor = 8 * sizeof(uint64_t);
	size_t address_alignment_divisor = 8;

	#define X(struct_name) \
		ASSERT_EQ(sizeof(struct_name) % size_alignment_divisor, 0); \
		struct_name var_##struct_name; \
		ASSERT_EQ(reinterpret_cast<uintptr_t>(&var_##struct_name) % address_alignment_divisor, 0);
	AMD_SMI_STRUCTURES_64
	#undef X
}
