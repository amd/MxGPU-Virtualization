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
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;


class AmdSmiConfidentialComputeTests : public amdsmi::AmdSmiTest {
protected:
	::testing::AssertionResult equal_tdi_state(smi_tdi_state expect,
						   amdsmi_tdi_state_t actual)
	{
		SMI_ASSERT_EQ(expect.state, (enum smi_tdi_state_t)actual);
		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_cc_mode(smi_cc_mode expect,
						 amdsmi_cc_mode_t actual)
	{
		SMI_ASSERT_EQ(expect.mode, (enum smi_cc_mode_t)actual);
		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_set_cc_mode(smi_set_cc_mode expect,
						     amdsmi_cc_mode_t actual)
	{
		SMI_ASSERT_EQ(expect.mode, (enum smi_cc_mode_t)actual);
		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdSmiConfidentialComputeTests, GetInvalidParams)
{
	int ret;
	amdsmi_cc_mode_t cc_mode;

	ret = amdsmi_get_tdi_state(VF_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_cc_mode(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_cc_mode(NULL, &cc_mode);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_cc_mode(&NIC_MOCK_HANDLE, &cc_mode);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_set_cc_mode(NULL, AMDSMI_CC_MODE_ON);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_set_cc_mode(&NIC_MOCK_HANDLE, AMDSMI_CC_MODE_ON);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiConfidentialComputeTests, IoctlFailed)
{
	amdsmi_tdi_state_t tdi_state;
	amdsmi_cc_mode_t cc_mode;
	int ret;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_tdi_state(VF_MOCK_HANDLE, &tdi_state);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_cc_mode(&GPU_MOCK_HANDLE, &cc_mode);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_set_cc_mode(&GPU_MOCK_HANDLE, AMDSMI_CC_MODE_ON);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiConfidentialComputeTests, GetTdiState)
{
	int ret;
	struct smi_device_info in_payload;
	struct smi_tdi_state tdi_state_mock = {};
	amdsmi_tdi_state_t tdi_state;

	// Test all TDI states
	for (int i = AMDSMI_TDI_STATE_UNLOCKED; i <= AMDSMI_TDI_STATE_ERROR; i++) {
		tdi_state_mock.state = (enum smi_tdi_state_t)i;
		
		WhenCalling(std::bind(amdsmi_get_tdi_state, VF_MOCK_HANDLE, &tdi_state));
		ExpectCommand(SMI_CMD_CODE_GET_TDI_STATE);
		SaveInputPayloadIn(&in_payload);
		PlantMockOutput(&tdi_state_mock);
		ret = performCall();

		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		ASSERT_EQ(in_payload.dev_id.handle, VF_MOCK_HANDLE.handle);
		ASSERT_TRUE(equal_tdi_state(tdi_state_mock, tdi_state));
	}
}

TEST_F(AmdSmiConfidentialComputeTests, GetCCMode)
{
	int ret;
	struct smi_device_info in_payload;
	struct smi_cc_mode cc_mode_mock = {};
	amdsmi_cc_mode_t cc_mode;

	// Test all CC modes
	for (int i = AMDSMI_CC_MODE_OFF; i <= AMDSMI_CC_MODE_DEV; i++) {
		cc_mode_mock.mode = (enum smi_cc_mode_t)i;
		
		WhenCalling(std::bind(amdsmi_get_cc_mode, &GPU_MOCK_HANDLE, &cc_mode));
		ExpectCommand(SMI_CMD_CODE_GET_CC_MODE);
		SaveInputPayloadIn(&in_payload);
		PlantMockOutput(&cc_mode_mock);
		ret = performCall();

		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
		ASSERT_TRUE(equal_cc_mode(cc_mode_mock, cc_mode));
	}
}

TEST_F(AmdSmiConfidentialComputeTests, SetCCMode)
{
	int ret;
	struct smi_set_cc_mode in_payload;

	// Test all valid CC modes
	for (int i = AMDSMI_CC_MODE_OFF; i <= AMDSMI_CC_MODE_DEV; i++) {
		WhenCalling(std::bind(amdsmi_set_cc_mode, &GPU_MOCK_HANDLE, (amdsmi_cc_mode_t)i));
		ExpectCommand(SMI_CMD_CODE_SET_CC_MODE);
		SaveInputPayloadIn(&in_payload);
		ret = performCall();

		ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
		ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
		ASSERT_EQ(in_payload.mode, (enum smi_cc_mode_t)i);
	}
}
