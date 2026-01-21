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
#include "common/smi_cmd.h"
#include "common/smi_device_handle.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::equal_handles;
using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;

class AmdSmiNodeTests : public amdsmi::AmdSmiTest {
protected:
	::testing::AssertionResult equal_npm_info(smi_npm_info expect, amdsmi_npm_info_t actual)
	{
		SMI_ASSERT_EQ(static_cast<int>(expect.status), static_cast<int>(actual.status));
		SMI_ASSERT_EQ(expect.limit, actual.limit);

		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdSmiNodeTests, IoctlFailed)
{
	int ret;
	amdsmi_node_handle node_handle;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	auto handle = GPU_MOCK_HANDLE;
	handle.dev_id = 0x75A0;
	ret = amdsmi_get_node_handle(&handle, &node_handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiNodeTests, GetNodeHandleInvalidProcessorHandle)
{
	int ret;
	amdsmi_node_handle node_handle;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_INVAL));

	ret = amdsmi_get_node_handle(NULL, &node_handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNodeTests, GetNodeHandleInvalidNodeHandle)
{
	int ret;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_INVAL));

	ret = amdsmi_get_node_handle(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNodeTests, GetNodeHandleSuccess)
{
	int ret;
	amdsmi_node_handle node_handle;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_SUCCESS));

	auto handle = GPU_MOCK_HANDLE;
	handle.dev_id = 0x75A0;
	ret = amdsmi_get_node_handle(&handle, &node_handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiNodeTests, GetNpmInfoFailed)
{
	int ret;
	amdsmi_npm_info_t npm_info;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_npm_info(&NODE_MOCK_HANDLE, &npm_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiNodeTests, GetNpmInfoSuccess)
{
	int ret;
	amdsmi_npm_info_t npm_info;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_SUCCESS));

	ret = amdsmi_get_npm_info(&NODE_MOCK_HANDLE, &npm_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiNodeTests, GetNpmInfoInvalidNodeHandle)
{
	int ret;
	amdsmi_npm_info_t npm_info;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_INVAL));

	ret = amdsmi_get_npm_info(NULL, &npm_info);
	EXPECT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiNodeTests, GetNpmInfoMockedSuccess)
{
	int ret;
	amdsmi_npm_info_t npm_info;
	smi_node_info in_payload;

	smi_npm_info mocked_npm_info;
	mocked_npm_info.status = SMI_NPM_STATUS_ENABLED;
	mocked_npm_info.limit = 100;

	WhenCalling(std::bind(&amdsmi_get_npm_info, &NODE_MOCK_HANDLE, &npm_info));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_NPM_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_npm_info);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_npm_info(mocked_npm_info, npm_info));
}
