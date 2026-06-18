/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
