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

using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;
using testing::DoAll;
using testing::_;
using testing::Return;
using amdsmi::equal_handles;

class AmdSmiVf2PfTests : public amdsmi::AmdSmiTest {
protected:
	amdsmi_vf_handle_t MOCK_VF_HANDLE = VF_MOCK_HANDLE;

	::testing::AssertionResult equal_guest_data(smi_guest_data expect,
						    amdsmi_guest_data_t actual)
	{
		SMI_ASSERT_MEM_EQ(expect.driver_version, actual.driver_version, sizeof(expect.driver_version));
		SMI_ASSERT_EQ(expect.fb_usage, actual.fb_usage);

		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdSmiVf2PfTests, InvalidParams)
{
	int ret;

	ret = amdsmi_get_guest_data(MOCK_VF_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiVf2PfTests, IoctlFailed)
{
	int ret;
	amdsmi_guest_data_t guest_data;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_guest_data(MOCK_VF_HANDLE, &guest_data);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiVf2PfTests, GetGuestData)
{
	int ret;
	struct smi_device_info in_payload;
	struct smi_guest_info mocked_resp = {};
	amdsmi_guest_data_t guest_data;
	uint8_t version[] = "123456";

	memcpy(mocked_resp.guest_data.driver_version, version, sizeof(version));
	mocked_resp.guest_data.fb_usage = 10;

	WhenCalling(std::bind(amdsmi_get_guest_data, MOCK_VF_HANDLE, &guest_data));
	ExpectCommand(SMI_CMD_CODE_GET_GUEST_DATA);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(in_payload.dev_id.handle, VF_MOCK_HANDLE.handle);
	ASSERT_TRUE(equal_guest_data(mocked_resp.guest_data, guest_data));
}
