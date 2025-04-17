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
#include "common/smi_cmd.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::equal_handles;
using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;

class AmdSmiPartitionProfileTest : public amdsmi::AmdSmiTest {
protected:
	::testing::AssertionResult equal_profile_info(smi_profile_info expect,
						      amdsmi_profile_info_t actual)
	{
		SMI_ASSERT_EQ((int)expect.profile_count, (int)actual.profile_count);
		SMI_ASSERT_EQ((int)expect.current_profile_index, (int)actual.current_profile_index);

		for (uint32_t i = 0; i < expect.profile_count; i++) {
			SMI_ASSERT_EQ(expect.profiles[i].vf_count, actual.profiles[i].vf_count)
				<< " for i = " << i;

			for (int j = 0; j < AMDSMI_PROFILE_CAPABILITY__MAX; j++) {

				SMI_ASSERT_EQ(expect.profiles[i].profile_caps[j].total,
					actual.profiles[i].profile_caps[j].total)
					<< " for i = " << i << " for j = " << j;

				SMI_ASSERT_EQ(expect.profiles[i].profile_caps[j].available,
					actual.profiles[i].profile_caps[j].available)
					<< " for i = " << i << " for j = " << j;

				SMI_ASSERT_EQ(expect.profiles[i].profile_caps[j].optimal,
					actual.profiles[i].profile_caps[j].optimal)
					<< " for i = " << i << " for j = " << j;

				SMI_ASSERT_EQ(expect.profiles[i].profile_caps[j].min_value,
					actual.profiles[i].profile_caps[j].min_value)
					<< " for i = " << i << " for j = " << j;

				SMI_ASSERT_EQ(expect.profiles[i].profile_caps[j].max_value,
					actual.profiles[i].profile_caps[j].max_value)
					<< " for i = " << i << " for j = " << j;
			}
		}

		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdSmiPartitionProfileTest, IoctlFailed)
{
	int ret;
	amdsmi_profile_info_t profile_info;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_partition_profile_info(&GPU_MOCK_HANDLE, &profile_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiPartitionProfileTest, InvalidParams)
{
	int ret;

	ret = amdsmi_get_partition_profile_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiPartitionProfileTest, GetPartitionProfileInfo)
{
	int ret;
	amdsmi_profile_info_t profile_info;
	smi_device_info in_payload;
	struct smi_profile_info mocked_resp = {};

	mocked_resp.profile_count = 2;
	mocked_resp.current_profile_index = 0;

	for (uint32_t i = 0; i < mocked_resp.profile_count; i++) {
		mocked_resp.profiles[i].vf_count = i+1;
		for (int j = 0; j < AMDSMI_PROFILE_CAPABILITY__MAX; j++) {
			mocked_resp.profiles[i].profile_caps[j].total = i+j*2;
			mocked_resp.profiles[i].profile_caps[j].available = i+j*3;
			mocked_resp.profiles[i].profile_caps[j].optimal = i+j*4;
			mocked_resp.profiles[i].profile_caps[j].min_value = i+j*5;
			mocked_resp.profiles[i].profile_caps[j].max_value = i+j*6;
		}
	}

	WhenCalling(std::bind(amdsmi_get_partition_profile_info, &GPU_MOCK_HANDLE,
			      &profile_info));
	ExpectCommand(SMI_CMD_CODE_GET_PARTITION_PROFILE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_profile_info(mocked_resp, profile_info));
}
