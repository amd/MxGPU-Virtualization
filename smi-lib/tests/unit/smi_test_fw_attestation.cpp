/*
 * Copyright (c) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
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

class AmdSmiFwAttestationTests : public amdsmi::AmdSmiTest {
protected:
	::testing::AssertionResult equal_fw_info(smi_fw_info expect,
						    amdsmi_fw_info_t actual)
	{
		SMI_ASSERT_EQ(expect.num_fw_info, actual.num_fw_info);

		for (uint32_t i = 0; i < expect.num_fw_info; i++) {
			SMI_ASSERT_EQ(expect.fw_info_list[i].fw_id,
				(enum smi_fw_block)actual.fw_info_list[i].fw_id)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.fw_info_list[i].fw_version,
				actual.fw_info_list[i].fw_version)
				<< " for i = " << i;
		}

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_error_records(amdsmi_fw_load_error_record_t expect[],
						      amdsmi_fw_load_error_record_t actual[],
						      uint32_t size)
	{
		for (uint32_t i = 0; i < size; i++) {
			SMI_ASSERT_EQ(expect[i].timestamp, actual[i].timestamp)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect[i].vf_idx, actual[i].vf_idx)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect[i].fw_id, actual[i].fw_id)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect[i].status, actual[i].status)
				<< " for i = " << i;
		}

		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdSmiFwAttestationTests, InvalidParams)
{
	int ret;

	ret = amdsmi_get_vf_fw_info(VF_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_fw_error_records(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiFwAttestationTests, DevBusy)
{
	int ret;
	amdsmi_fw_info_t fw_info;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_BUSY));

	ret = amdsmi_get_vf_fw_info(VF_MOCK_HANDLE, &fw_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_BUSY);
}

TEST_F(AmdSmiFwAttestationTests, IoctlFailed)
{
	int ret;
	amdsmi_fw_info_t fw_info;
	amdsmi_fw_error_record_t err_records;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_vf_fw_info(VF_MOCK_HANDLE, &fw_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_fw_error_records(&GPU_MOCK_HANDLE, &err_records);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}


TEST_F(AmdSmiFwAttestationTests, GetFwErrorRecords)
{
	int ret;
	amdsmi_fw_error_record_t records;
	struct smi_device_info in_payload;
	amdsmi_fw_error_record_t mocked_resp = {};
	mocked_resp.num_err_records = AMDSMI_MAX_ERR_RECORDS;

	for (unsigned i = 0; i < AMDSMI_MAX_ERR_RECORDS; i++) {
		mocked_resp.err_records[i].timestamp = i;
		mocked_resp.err_records[i].vf_idx = i*2;
		mocked_resp.err_records[i].fw_id = i*3;
		mocked_resp.err_records[i].status = (uint16_t)(i*4);
	}

	WhenCalling(std::bind(amdsmi_get_fw_error_records, &GPU_MOCK_HANDLE, &records));
	ExpectCommand(SMI_CMD_CODE_GET_UCODE_ERR_RECORDS);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_error_records(mocked_resp.err_records, records.err_records, mocked_resp.num_err_records));
}

TEST_F(AmdSmiFwAttestationTests, GetVfFwInfo)
{
	int ret;
	amdsmi_fw_info_t fw;
	struct smi_device_info in_payload;
	smi_fw_info mocked_resp = {};
	mocked_resp.num_fw_info = SMI_FW_ID__MAX;

	for (int i = 0; i < SMI_FW_ID__MAX; i++) {
		mocked_resp.fw_info_list[i].fw_id = (smi_fw_block)i;
		mocked_resp.fw_info_list[i].fw_version = i * 2;
	}

	WhenCalling(std::bind(amdsmi_get_vf_fw_info, VF_MOCK_HANDLE, &fw));
	ExpectCommand(SMI_CMD_CODE_GET_VF_UCODE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(in_payload.dev_id.handle, VF_MOCK_HANDLE.handle);
	ASSERT_TRUE(equal_fw_info(mocked_resp, fw));
}
