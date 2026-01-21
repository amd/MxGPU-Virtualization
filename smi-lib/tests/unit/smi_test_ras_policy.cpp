/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
#include "smi_utils.h"
#include "common/smi_cmd.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;
using amdsmi::equal_handles;

class AmdSmiRasPolicyTests : public amdsmi::AmdSmiTest {
};

TEST_F(AmdSmiRasPolicyTests, InvalidParams)
{
	int ret;
	amdsmi_gpu_ras_policy_info_t policy_info;

	ret = amdsmi_get_gpu_ras_policy_info(NULL, &policy_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_ras_policy_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiRasPolicyTests, IoctlFailed) {
	int ret;
	amdsmi_gpu_ras_policy_info_t policy_info;

	EXPECT_CALL(*g_system_mock, Ioctl(_)).WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_ras_policy_info(&GPU_MOCK_HANDLE, &policy_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiRasPolicyTests, GetGpuRasPolicyInfoSuccess) {
	int ret;
	amdsmi_gpu_ras_policy_info_t policy_info;
	struct smi_device_info in_payload;
	struct smi_gpu_ras_policy_info mocked_resp = {};

	mocked_resp.major_version = 4;
	mocked_resp.minor_version = 0;
	mocked_resp.policy_data.v4_0.dram_non_critical_region_threshold = 10;
	mocked_resp.policy_data.v4_0.dram_critical_region_threshold = 20;

	WhenCalling(std::bind(amdsmi_get_gpu_ras_policy_info, &GPU_MOCK_HANDLE, &policy_info));
	ExpectCommand(SMI_CMD_CODE_GET_RAS_POLICY_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(static_cast<int>(policy_info.major_version), 4);
	ASSERT_EQ(static_cast<int>(policy_info.minor_version), 0);
	ASSERT_EQ(static_cast<int>(policy_info.policy_data.v4_0.dram_non_critical_region_threshold), 10);
	ASSERT_EQ(static_cast<int>(policy_info.policy_data.v4_0.dram_critical_region_threshold), 20);
}
