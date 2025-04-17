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

#include <cstdio>

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::equal_handles;
using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;

amdsmi_vf_handle_t MOCK_VF_HANDLE_BAD = { ( VF_MOCK_HANDLE.handle << 32) | 0x0003 };

class AmdsmiUUIDTests : public amdsmi::AmdSmiTest {
};

TEST_F(AmdsmiUUIDTests, InvalidParams)
{
	int ret;
	char uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;

    amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	ret = amdsmi_get_gpu_device_uuid(MOCK_GPU_HANDLE, NULL, uuid);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_device_uuid(MOCK_GPU_HANDLE, &uuid_length, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	uuid_length--;
	ret = amdsmi_get_gpu_device_uuid(MOCK_GPU_HANDLE, &uuid_length, uuid);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdsmiUUIDTests, GetUUIDFail)
{
	int ret;
	char uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;
    amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	EXPECT_CALL(*amdsmi::g_system_mock,
		    Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_ASIC_INFO)))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_device_uuid(MOCK_GPU_HANDLE, &uuid_length, uuid);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdsmiUUIDTests, GetUUID_PF)
{
	int ret;
	char uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;
    amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	smi_device_info in_payload;
	smi_asic_info gpu_info_mock = {};
	std::string name = "abcdef";
#ifdef _WIN64
	strcpy_s(gpu_info_mock.market_name, sizeof(gpu_info_mock.market_name), name.c_str());
#else
	strcpy(gpu_info_mock.market_name, name.c_str());
#endif

	gpu_info_mock.vendor_id = 2;
	gpu_info_mock.device_id = 3;
	gpu_info_mock.rev_id = 4;
#ifdef _WIN64
	strcpy_s(gpu_info_mock.asic_serial, sizeof(gpu_info_mock.asic_serial), "0x9A1F188C37CB1EE6");
#else
	strcpy(gpu_info_mock.asic_serial, "0x9A1F188C37CB1EE6");
#endif

	uuid_length += 0x54;
	WhenCalling(std::bind(amdsmi_get_gpu_device_uuid, MOCK_GPU_HANDLE, &uuid_length, uuid));
	ExpectCommand(SMI_CMD_CODE_GET_ASIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(uuid_length, AMDSMI_GPU_UUID_SIZE);
	ASSERT_EQ(strcmp(uuid, "9aff0003-0000-1000-801f-188c37cb1ee6"), 0);
}

TEST_F(AmdsmiUUIDTests, GetUUID_VF)
{
	int ret;
	char uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;
    	amdsmi_vf_handle_t MOCK_VF_HANDLE = VF_MOCK_HANDLE;

	smi_device_info in_payload;
	smi_asic_info gpu_info_mock = {};
	std::string name = "abcdef";
#ifdef _WIN64
	strcpy_s(gpu_info_mock.market_name, sizeof(gpu_info_mock.market_name), name.c_str());
#else
	strcpy(gpu_info_mock.market_name, name.c_str());
#endif
	gpu_info_mock.vendor_id = 2;
	gpu_info_mock.device_id = 3;
	gpu_info_mock.rev_id = 4;
#ifdef _WIN64
	strcpy_s(gpu_info_mock.asic_serial, sizeof(gpu_info_mock.asic_serial), "0x9A1F188C37CB1EE6");
#else
	strcpy(gpu_info_mock.asic_serial, "0x9A1F188C37CB1EE6");
#endif
	smi_in_hdr actual_in_hdr;
	smi_device_info device_info;
	smi_vf_partition_info vf_partition_info = {};

	PrepareIoctl(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO, &actual_in_hdr, &device_info,
		     vf_partition_info, AMDSMI_STATUS_API_FAILED);

	WhenCalling(std::bind(amdsmi_get_vf_uuid, MOCK_VF_HANDLE, &uuid_length, uuid));
	ExpectCommand(SMI_CMD_CODE_GET_ASIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_UNKNOWN_ERROR);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	vf_partition_info.num_vf_enabled = 1;
	vf_partition_info.partition[0].id.handle = MOCK_VF_HANDLE_BAD.handle;
	vf_partition_info.partition[0].fb.fb_offset = 0;
	vf_partition_info.partition[0].fb.fb_size = 1024;

	PrepareIoctl(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO, &actual_in_hdr, &device_info,
		     vf_partition_info, AMDSMI_STATUS_SUCCESS);

	WhenCalling(std::bind(amdsmi_get_vf_uuid, MOCK_VF_HANDLE, &uuid_length, uuid));
	ExpectCommand(SMI_CMD_CODE_GET_ASIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	vf_partition_info.num_vf_enabled = 2;
	vf_partition_info.partition[0].id.handle = MOCK_VF_HANDLE_BAD.handle;
	vf_partition_info.partition[0].fb.fb_offset = 0;
	vf_partition_info.partition[0].fb.fb_size = 1024;
	vf_partition_info.partition[1].id.handle = VF_MOCK_HANDLE.handle;
	vf_partition_info.partition[1].fb.fb_offset = 2048;
	vf_partition_info.partition[1].fb.fb_size = 1024;

	PrepareIoctl(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO, &actual_in_hdr, &device_info,
		     vf_partition_info, AMDSMI_STATUS_SUCCESS);

	WhenCalling(std::bind(amdsmi_get_vf_uuid, MOCK_VF_HANDLE, &uuid_length, uuid));
	ExpectCommand(SMI_CMD_CODE_GET_ASIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(uuid_length, AMDSMI_GPU_UUID_SIZE);
	ASSERT_EQ(strcmp(uuid, "9a010003-0000-1000-801f-188c37cb1ee6"), 0);
}

TEST_F(AmdsmiUUIDTests, GetUUID_VF_INVAL)
{
	int ret;
	amdsmi_vf_handle_t MOCK_VF_HANDLE = VF_MOCK_HANDLE;
	unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;

	ret = amdsmi_get_vf_uuid(MOCK_VF_HANDLE, &uuid_length, NULL);

	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdsmiUUIDTests, GetUuidVfApiFail)
{
	int ret;
	char uuid[AMDSMI_GPU_UUID_SIZE];
	unsigned int uuid_length = AMDSMI_GPU_UUID_SIZE;
    amdsmi_vf_handle_t MOCK_VF_HANDLE = VF_MOCK_HANDLE;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_ASIC_INFO)))
		.WillOnce(amdsmi::SetResponseStatus(AMDSMI_STATUS_API_FAILED));
	ret = amdsmi_get_vf_uuid(MOCK_VF_HANDLE, &uuid_length, uuid);

	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}
