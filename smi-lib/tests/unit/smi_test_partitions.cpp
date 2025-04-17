// /*
//  * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
//  *
//  * Permission is hereby granted, free of charge, to any person obtaining a copy
//  * of this software and associated documentation files (the "Software"), to deal
//  * in the Software without restriction, including without limitation the rights
//  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  * copies of the Software, and to permit persons to whom the Software is
//  * furnished to do so, subject to the following conditions:
//  *
//  * The above copyright notice and this permission notice shall be included in
//  * all copies or substantial portions of the Software.
//  *
//  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  * THE SOFTWARE.
//  */

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

};

TEST_F(AmdSmiPartitionTest, InvalidParams)
{
	amdsmi_accelerator_partition_profile_t profile;
	uint32_t partition_id[AMDSMI_MAX_ACCELERATOR_PROFILE];
	int ret;

	ret = amdsmi_get_gpu_accelerator_partition_profile_config(&GPU_MOCK_HANDLE, NULL);
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
	uint32_t profile_index = 0;
	amdsmi_memory_partition_type_t mode = AMDSMI_MEMORY_PARTITION_NPS4;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_accelerator_partition_profile_config(&GPU_MOCK_HANDLE, &cp_caps);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_gpu_accelerator_partition_profile(&GPU_MOCK_HANDLE, &profile, partition_id);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_gpu_memory_partition_config(&GPU_MOCK_HANDLE, &memory_partition);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_set_gpu_accelerator_partition_profile(&GPU_MOCK_HANDLE, profile_index);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_set_gpu_memory_partition_mode(&GPU_MOCK_HANDLE, mode);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

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

TEST_F(AmdSmiPartitionTest, GetAcceleratorPartitionCaps)
{
	int ret;
	amdsmi_accelerator_partition_profile_config_t *profile_configs;
	smi_profile_configs in_payload;
	amdsmi_accelerator_partition_profile_config_t *config;
#ifdef _WIN64
	config = (amdsmi_accelerator_partition_profile_config_t*)calloc(1, sizeof(amdsmi_accelerator_partition_profile_config_t));
#else
	config = (amdsmi_accelerator_partition_profile_config_t*)amdsmi::mem_aligned_alloc((void**)&config, 4096, sizeof(amdsmi_accelerator_partition_profile_config_t));
#endif

	config->num_profiles = 2;
	config->num_resource_profiles = 2;
	config->default_profile_index = 0;
	for (uint32_t i = 0; i < config->num_profiles; i++) {
		config->profiles[i].profile_type = AMDSMI_ACCELERATOR_PARTITION_QPX;
		config->profiles[i].num_partitions = i + 1;
		config->profiles[i].profile_index = i;
		config->profiles[i].num_resources = 2;
		for (uint32_t j = 0; j < config->profiles[i].num_partitions; j++) {
			for (uint32_t k = 0; k < config->profiles[i].num_resources; k++) {
				config->profiles[i].resources[j][k] = j*k;
			}
		}
	}

	for (uint32_t i = 0; i < config->num_resource_profiles; i++) {
		config->resource_profiles[0].profile_index = i;
		config->resource_profiles[0].resource_type = AMDSMI_ACCELERATOR_XCC;
		config->resource_profiles[0].partition_resource = i;
		config->resource_profiles[0].num_partitions_share_resource = i;
	}

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(config));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(config));
#endif

 	profile_configs = (amdsmi_accelerator_partition_profile_config_t *)malloc(sizeof(amdsmi_accelerator_partition_profile_config_t));

	WhenCalling(std::bind(amdsmi_get_gpu_accelerator_partition_profile_config, &GPU_MOCK_HANDLE, profile_configs));
	ExpectCommand(SMI_CMD_CODE_GET_ACCELERATOR_PARTITION_PROFILE_CONFIG);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	free(profile_configs);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
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
