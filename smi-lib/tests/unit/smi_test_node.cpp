/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
		SMI_ASSERT_EQ(expect.ubb_power_threshold, actual.ubb_power_threshold);

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
	mocked_npm_info.ubb_power_threshold = 500;

	WhenCalling(std::bind(&amdsmi_get_npm_info, &NODE_MOCK_HANDLE, &npm_info));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_NPM_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_npm_info);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_npm_info(mocked_npm_info, npm_info));
}
