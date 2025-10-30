/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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
#include "common/smi_cmd.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;

class AmdSmiPartitionTest : public amdsmi::AmdSmiTest {
protected:
	::testing::AssertionResult equal_accelerator_partition_profile(smi_accelerator_partition_profile expect,
						      amdsmi_accelerator_partition_profile_t actual)
	{
		SMI_ASSERT_EQ(expect.profile_type, (enum smi_accelerator_partition_type)actual.profile_type);
		SMI_ASSERT_EQ(expect.num_partitions, actual.num_partitions);
		SMI_ASSERT_EQ(expect.memory_caps.nps_cap_mask, actual.memory_caps.nps_cap_mask);
		SMI_ASSERT_EQ(expect.profile_index, actual.profile_index);
		SMI_ASSERT_EQ(expect.num_resources, actual.num_resources);
		for (unsigned int i = 0; i < actual.num_partitions; i++) {
			for (unsigned int j = 0; j < actual.num_resources; j++) {
				SMI_ASSERT_EQ(expect.resources[i][j], actual.resources[i][j])
				<< " for i = " << i << "and for j = " << j;
			}
		}

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_accelerator_partition_id(uint32_t *expect,
						      uint32_t *actual, uint32_t size)
	{
		for (uint32_t i = 0; i < size; i++) {
			SMI_ASSERT_EQ(expect[i], actual[i])
				<< " for i = " << i;
		}

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_memory_config(amdsmi_memory_partition_config_t expect,
						      amdsmi_memory_partition_config_t actual)
	{
		SMI_ASSERT_EQ(expect.partition_caps.nps_cap_mask, actual.partition_caps.nps_cap_mask);
		SMI_ASSERT_EQ(expect.mp_mode, actual.mp_mode);
		SMI_ASSERT_EQ(expect.num_numa_ranges, actual.num_numa_ranges);

		for(uint32_t i = 0; i < actual.num_numa_ranges; i++) {
			SMI_ASSERT_EQ(expect.numa_range[i].memory_type, actual.numa_range[i].memory_type);
			SMI_ASSERT_EQ(expect.numa_range[i].start, actual.numa_range[i].start);
			SMI_ASSERT_EQ(expect.numa_range[i].end, actual.numa_range[i].end);
		}

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_accelerator_partition_profile_config(amdsmi_accelerator_partition_profile_config_t expect,
					      amdsmi_accelerator_partition_profile_config_t actual)
	{
		SMI_ASSERT_EQ(expect.num_profiles, actual.num_profiles);
		SMI_ASSERT_EQ(expect.num_resource_profiles, actual.num_resource_profiles);
		SMI_ASSERT_EQ(expect.default_profile_index, actual.default_profile_index);

		// Compare resource profiles
		for (uint32_t i = 0; i < actual.num_resource_profiles; i++) {
			SMI_ASSERT_EQ(expect.resource_profiles[i].profile_index, actual.resource_profiles[i].profile_index)
				<< " for resource_profile i = " << i;
			SMI_ASSERT_EQ(expect.resource_profiles[i].resource_type, actual.resource_profiles[i].resource_type)
				<< " for resource_profile i = " << i;
			SMI_ASSERT_EQ(expect.resource_profiles[i].partition_resource, actual.resource_profiles[i].partition_resource)
				<< " for resource_profile i = " << i;
			SMI_ASSERT_EQ(expect.resource_profiles[i].num_partitions_share_resource, actual.resource_profiles[i].num_partitions_share_resource)
				<< " for resource_profile i = " << i;
		}

		for (uint32_t i = 0; i < actual.num_profiles; i++) {
			SMI_ASSERT_EQ(expect.profiles[i].profile_type, actual.profiles[i].profile_type)
				<< " for profile i = " << i;
			SMI_ASSERT_EQ(expect.profiles[i].num_partitions, actual.profiles[i].num_partitions)
				<< " for profile i = " << i;
			SMI_ASSERT_EQ(expect.profiles[i].memory_caps.nps_cap_mask, actual.profiles[i].memory_caps.nps_cap_mask)
				<< " for profile i = " << i;
			SMI_ASSERT_EQ(expect.profiles[i].profile_index, actual.profiles[i].profile_index)
				<< " for profile i = " << i;
			SMI_ASSERT_EQ(expect.profiles[i].num_resources, actual.profiles[i].num_resources)
				<< " for profile i = " << i;

			for (uint32_t j = 0; j < actual.profiles[i].num_partitions; j++) {
				for (uint32_t k = 0; k < actual.profiles[i].num_resources; k++) {
					SMI_ASSERT_EQ(expect.profiles[i].resources[j][k], actual.profiles[i].resources[j][k])
						<< " for profile i = " << i << " partition j = " << j << " resource k = " << k;
				}
			}
		}

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_accelerator_partition_profile_config_global(amdsmi_accelerator_partition_profile_config_global_t expect,
					      amdsmi_accelerator_partition_profile_config_global_t actual)
	{
		SMI_ASSERT_EQ(expect.num_profiles, actual.num_profiles);
		SMI_ASSERT_EQ(expect.num_resource_profiles, actual.num_resource_profiles);
		SMI_ASSERT_EQ(expect.default_profile_index, actual.default_profile_index);

		for (uint32_t i = 0; i < actual.num_resource_profiles; i++) {
			SMI_ASSERT_EQ(expect.resource_profiles[i].profile_index, actual.resource_profiles[i].profile_index)
				<< " for resource_profile i = " << i;
			SMI_ASSERT_EQ(expect.resource_profiles[i].resource_type, actual.resource_profiles[i].resource_type)
				<< " for resource_profile i = " << i;
			SMI_ASSERT_EQ(expect.resource_profiles[i].partition_resource, actual.resource_profiles[i].partition_resource)
				<< " for resource_profile i = " << i;
			SMI_ASSERT_EQ(expect.resource_profiles[i].num_partitions_share_resource, actual.resource_profiles[i].num_partitions_share_resource)
				<< " for resource_profile i = " << i;
		}

		for (uint32_t i = 0; i < actual.num_profiles; i++) {
			SMI_ASSERT_EQ(expect.profiles[i].profile.profile_type, actual.profiles[i].profile.profile_type)
				<< " for profile i = " << i;
			SMI_ASSERT_EQ(expect.profiles[i].profile.num_partitions, actual.profiles[i].profile.num_partitions)
				<< " for profile i = " << i;
			SMI_ASSERT_EQ(expect.profiles[i].profile.memory_caps.nps_cap_mask, actual.profiles[i].profile.memory_caps.nps_cap_mask)
				<< " for profile i = " << i;
			SMI_ASSERT_EQ(expect.profiles[i].profile.profile_index, actual.profiles[i].profile.profile_index)
				<< " for profile i = " << i;
			SMI_ASSERT_EQ(expect.profiles[i].profile.num_resources, actual.profiles[i].profile.num_resources)
				<< " for profile i = " << i;
			SMI_ASSERT_EQ(expect.profiles[i].vf_mode, actual.profiles[i].vf_mode)
				<< " for profile i = " << i;

			for (uint32_t j = 0; j < actual.profiles[i].profile.num_partitions; j++) {
				for (uint32_t k = 0; k < actual.profiles[i].profile.num_resources; k++) {
					SMI_ASSERT_EQ(expect.profiles[i].profile.resources[j][k], actual.profiles[i].profile.resources[j][k])
						<< " for profile i = " << i << " partition j = " << j << " resource k = " << k;
				}
			}
		}

		return ::testing::AssertionSuccess();
	}

};

TEST_F(AmdSmiPartitionTest, InvalidParams)
{
	amdsmi_accelerator_partition_profile_t profile;
	uint32_t partition_id[AMDSMI_MAX_ACCELERATOR_PROFILE];
	int ret;

	ret = amdsmi_get_gpu_accelerator_partition_profile_config(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_gpu_accelerator_partition_profile_config_global(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_gpu_accelerator_partition_profile(&GPU_MOCK_HANDLE, NULL, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_gpu_accelerator_partition_profile(&GPU_MOCK_HANDLE, &profile, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_gpu_accelerator_partition_profile(&GPU_MOCK_HANDLE, NULL, partition_id);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_gpu_memory_partition_config(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_set_gpu_accelerator_partition_profile(NULL, 0);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_set_gpu_memory_partition_mode(NULL, AMDSMI_MEMORY_PARTITION_NPS1);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_set_gpu_memory_partition_mode(NULL, AMDSMI_MEMORY_PARTITION_UNKNOWN);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

}

TEST_F(AmdSmiPartitionTest, IoctlFailed)
{
	int ret;
	amdsmi_accelerator_partition_profile_t profile;
	uint32_t partition_id[AMDSMI_MAX_ACCELERATOR_PROFILE];
	amdsmi_memory_partition_config_t memory_partition;
	amdsmi_accelerator_partition_profile_config_t cp_caps;
	amdsmi_accelerator_partition_profile_config_global_t cp_caps_global;
	uint32_t profile_index = 0;
	amdsmi_memory_partition_type_t mode = AMDSMI_MEMORY_PARTITION_NPS4;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_accelerator_partition_profile_config(&GPU_MOCK_HANDLE, &cp_caps);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_gpu_accelerator_partition_profile_config_global(&GPU_MOCK_HANDLE, &cp_caps_global);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_gpu_accelerator_partition_profile(&GPU_MOCK_HANDLE, &profile, partition_id);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_gpu_memory_partition_config(&GPU_MOCK_HANDLE, &memory_partition);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_set_gpu_accelerator_partition_profile(&GPU_MOCK_HANDLE, profile_index);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_set_gpu_memory_partition_mode(&GPU_MOCK_HANDLE, mode);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	mode = AMDSMI_MEMORY_PARTITION_UNKNOWN;
	ret = amdsmi_set_gpu_memory_partition_mode(&GPU_MOCK_HANDLE, mode);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiPartitionTest, GetAcceleratorCapsAllocFail)
{
	int ret;
	amdsmi_accelerator_partition_profile_config_t cp_caps;

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_))
				.WillOnce(testing::Return(nullptr));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_))
				.WillOnce(testing::Return(nullptr));
#endif
	ret = amdsmi_get_gpu_accelerator_partition_profile_config(&GPU_MOCK_HANDLE, &cp_caps);
	ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
}

TEST_F(AmdSmiPartitionTest, GetAcceleratorCapsGlobalAllocFail)
{
	int ret;
	amdsmi_accelerator_partition_profile_config_global_t cp_caps_global;

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_))
				.WillOnce(testing::Return(nullptr));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_))
				.WillOnce(testing::Return(nullptr));
#endif
	ret = amdsmi_get_gpu_accelerator_partition_profile_config_global(&GPU_MOCK_HANDLE, &cp_caps_global);
	ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
}

TEST_F(AmdSmiPartitionTest, GetAcceleratorPartitionCaps)
{
	int ret;
	amdsmi_accelerator_partition_profile_config_t profile_configs;
	smi_profile_configs in_payload;

	// Create the mock data that the ioctl should populate in the allocated memory
	amdsmi_accelerator_partition_profile_config_t mock_data;
	mock_data.num_profiles = 2;
	mock_data.num_resource_profiles = 2;
	mock_data.default_profile_index = 0;

	for (uint32_t i = 0; i < mock_data.num_resource_profiles; i++) {
		mock_data.resource_profiles[i].profile_index = i + 40;
		mock_data.resource_profiles[i].resource_type = AMDSMI_ACCELERATOR_XCC;
		mock_data.resource_profiles[i].partition_resource = i + 50;
		mock_data.resource_profiles[i].num_partitions_share_resource = i + 60;
	}

	for (uint32_t i = 0; i < mock_data.num_profiles; i++) {
		mock_data.profiles[i].profile_type = AMDSMI_ACCELERATOR_PARTITION_QPX;
		mock_data.profiles[i].num_partitions = i + 1;
		mock_data.profiles[i].profile_index = i + 200;
		mock_data.profiles[i].num_resources = 2;

		for (uint32_t j = 0; j < mock_data.profiles[i].num_partitions; j++) {
			for (uint32_t k = 0; k < mock_data.profiles[i].num_resources; k++) {
				mock_data.profiles[i].resources[j][k] = j*k + 100;
			}
		}
	}

	// Set up custom ioctl mock that populates the allocated memory
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_ACCELERATOR_PARTITION_PROFILE_CONFIG)))
		.WillOnce(testing::Invoke([&mock_data, &in_payload](smi_ioctl_cmd *ioctl_cmd) -> int {
			// Save the input payload for verification
			memcpy(&in_payload, &ioctl_cmd->payload, sizeof(smi_profile_configs));

			// Extract the input payload
			smi_profile_configs *payload = (smi_profile_configs*)&ioctl_cmd->payload;
			// Populate the allocated memory that the payload points to
			if (payload->profile_configs) {
				memcpy(payload->profile_configs, &mock_data, sizeof(mock_data));
			}
			ioctl_cmd->out_hdr.status = AMDSMI_STATUS_SUCCESS;
			return 0;
		}));

	// Call the function directly instead of using performCall()
	ret = amdsmi_get_gpu_accelerator_partition_profile_config(&GPU_MOCK_HANDLE, &profile_configs);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));

	// Verify the data was copied correctly from allocated memory to output structure
	ASSERT_TRUE(equal_accelerator_partition_profile_config(mock_data, profile_configs));
}

TEST_F(AmdSmiPartitionTest, GetGpuAcceleratorPartitionProfileConfigSysconfFailed)
{
	int ret;
	amdsmi_accelerator_partition_profile_config_t config;
#ifndef _WIN64
	// Mock sysconf to return -1
	EXPECT_CALL(*g_system_mock, Sysconf(testing::_)).WillOnce(testing::Return(-1));
#endif
	ret = amdsmi_get_gpu_accelerator_partition_profile_config(&GPU_MOCK_HANDLE, &config);
#ifndef _WIN64
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
#else
	ASSERT_NE(ret, AMDSMI_STATUS_SUCCESS);
#endif
}

TEST_F(AmdSmiPartitionTest, GetGpuAcceleratorPartitionProfileConfigGlobalSysconfFailed)
{
	int ret;
	amdsmi_accelerator_partition_profile_config_global_t config;
#ifndef _WIN64
	// Mock sysconf to return -1
	EXPECT_CALL(*g_system_mock, Sysconf(testing::_)).WillOnce(testing::Return(-1));
#endif
	ret = amdsmi_get_gpu_accelerator_partition_profile_config_global(&GPU_MOCK_HANDLE, &config);
#ifndef _WIN64
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
#else
	ASSERT_NE(ret, AMDSMI_STATUS_SUCCESS);
#endif
}

TEST_F(AmdSmiPartitionTest, GetMemoryPartitionCaps)
{
	int ret;
	amdsmi_memory_partition_config_t config;
	smi_device_info in_payload;
	amdsmi_memory_partition_config_t mocked_resp = {};

	mocked_resp.partition_caps.nps_cap_mask = 0x4;
	mocked_resp.mp_mode = AMDSMI_MEMORY_PARTITION_NPS4;
	mocked_resp.num_numa_ranges = 8;

	for (uint32_t i = 0; i < mocked_resp.num_numa_ranges; i++) {
		mocked_resp.numa_range[i].memory_type = AMDSMI_VRAM_TYPE_GDDR4;
		mocked_resp.numa_range[i].start = i;
		mocked_resp.numa_range[i].end = i + 1;
	}

	WhenCalling(std::bind(amdsmi_get_gpu_memory_partition_config, &GPU_MOCK_HANDLE,
						&config));
	ExpectCommand(SMI_CMD_CODE_GET_CURR_MEMORY_PARTITION_SETTING);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_memory_config(mocked_resp, config));
}

TEST_F(AmdSmiPartitionTest, GetCurrAcceleratorPartitionSetting)
{
	int ret;
	amdsmi_accelerator_partition_profile_t setting;
	uint32_t partition_id[AMDSMI_MAX_ACCELERATOR_PROFILE];
	smi_device_info in_payload;
	smi_accelerator_partition_profile_cap mocked_resp = {};
	mocked_resp.config.profile_type = SMI_ACCELERATOR_PARTITION_SPX;
	mocked_resp.config.num_partitions = 1;
 	mocked_resp.config.profile_index = 1;
 	mocked_resp.config.num_resources = 2;
 	mocked_resp.config.resources[0][0] = 0;
 	mocked_resp.config.resources[0][1] = 4;
	mocked_resp.partition_id[0] = 1;
 	WhenCalling(std::bind(amdsmi_get_gpu_accelerator_partition_profile, &GPU_MOCK_HANDLE,
 						&setting, partition_id));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_ACCELERATOR_PARTITION);
	SaveInputPayloadIn(&in_payload);
 	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_accelerator_partition_profile(mocked_resp.config, setting));
	ASSERT_TRUE(equal_accelerator_partition_id(mocked_resp.partition_id, partition_id, mocked_resp.config.num_partitions));
}

TEST_F(AmdSmiPartitionTest, SetAcceleratorPartitionSetting)
{
	int ret;
	uint32_t index = 1;
	struct smi_set_gpu_accelerator_partition_setting in_payload;

	WhenCalling(std::bind(amdsmi_set_gpu_accelerator_partition_profile, &GPU_MOCK_HANDLE, index));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_ACCELERATOR_PARTITION_SETTING);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}

TEST_F(AmdSmiPartitionTest, GetAcceleratorPartitionCapsGlobal)
{
	int ret;
	amdsmi_accelerator_partition_profile_config_global_t profile_configs;
	smi_profile_configs_global in_payload;

	// Create the mock data that the ioctl should populate in the allocated memory
	amdsmi_accelerator_partition_profile_config_global_t mock_data;
	mock_data.num_profiles = 2;
	mock_data.num_resource_profiles = 2;
	mock_data.default_profile_index = 1;

	for (uint32_t i = 0; i < mock_data.num_resource_profiles; i++) {
		mock_data.resource_profiles[i].profile_index = i + 10;
		mock_data.resource_profiles[i].resource_type = AMDSMI_ACCELERATOR_XCC;
		mock_data.resource_profiles[i].partition_resource = i + 20;
		mock_data.resource_profiles[i].num_partitions_share_resource = i + 30;
	}

	for (uint32_t i = 0; i < mock_data.num_profiles; i++) {
		mock_data.profiles[i].profile.profile_type = AMDSMI_ACCELERATOR_PARTITION_QPX;
		mock_data.profiles[i].profile.num_partitions = i + 1;
		mock_data.profiles[i].profile.profile_index = i + 100;
		mock_data.profiles[i].profile.num_resources = 2;
		mock_data.profiles[i].vf_mode = AMDSMI_VF_MODE_1;
		for (uint32_t j = 0; j < mock_data.profiles[i].profile.num_partitions; j++) {
			for (uint32_t k = 0; k < mock_data.profiles[i].profile.num_resources; k++) {
				mock_data.profiles[i].profile.resources[j][k] = j*k + 50;
			}
		}
	}

	// Set up custom ioctl mock that populates the allocated memory
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_ACCELERATOR_PARTITION_PROFILE_CONFIG_GLOBAL)))
		.WillOnce(testing::Invoke([&mock_data, &in_payload](smi_ioctl_cmd *ioctl_cmd) -> int {
			// Save the input payload for verification
			memcpy(&in_payload, &ioctl_cmd->payload, sizeof(smi_profile_configs_global));

			// Extract the input payload
			smi_profile_configs_global *payload = (smi_profile_configs_global*)&ioctl_cmd->payload;
			// Populate the allocated memory that the payload points to
			if (payload->profile_configs) {
				memcpy(payload->profile_configs, &mock_data, sizeof(mock_data));
			}
			ioctl_cmd->out_hdr.status = AMDSMI_STATUS_SUCCESS;
			return 0;
		}));

	// Call the function directly instead of using performCall()
	ret = amdsmi_get_gpu_accelerator_partition_profile_config_global(&GPU_MOCK_HANDLE, &profile_configs);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));

	// Verify the data was copied correctly from allocated memory to output structure
	ASSERT_TRUE(equal_accelerator_partition_profile_config_global(mock_data, profile_configs));
}

TEST_F(AmdSmiPartitionTest, SetMemoryPartitionSetting)
{
	int ret;
	amdsmi_memory_partition_type_t setting = AMDSMI_MEMORY_PARTITION_NPS1;
	struct smi_set_gpu_memory_partition_setting in_payload;

	WhenCalling(std::bind(amdsmi_set_gpu_memory_partition_mode, &GPU_MOCK_HANDLE, setting));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_MEMORY_PARTITION_SETTING);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}
