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

class AmdSmiDfcTable : public amdsmi::AmdSmiTest {
protected:
	::testing::AssertionResult equal_dfc_table(amdsmi_dfc_fw_t expect,
						      amdsmi_dfc_fw_t actual)
	{
		SMI_ASSERT_EQ(expect.header.dfc_fw_total_entries, actual.header.dfc_fw_total_entries);
		SMI_ASSERT_EQ(expect.header.dfc_fw_version, actual.header.dfc_fw_version);
		SMI_ASSERT_EQ(expect.header.dfc_gart_wr_guest_min, actual.header.dfc_gart_wr_guest_min);
		SMI_ASSERT_EQ(expect.header.dfc_gart_wr_guest_max, actual.header.dfc_gart_wr_guest_max);

		for (unsigned int i = 0; i < actual.header.dfc_fw_total_entries; i++) {
			SMI_ASSERT_EQ(expect.data[i].dfc_fw_type, actual.data[i].dfc_fw_type)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.data[i].verification_enabled, actual.data[i].verification_enabled)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.data[i].customer_ordinal, actual.data[i].customer_ordinal)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.data[i].white_list[0].latest, actual.data[i].white_list[0].latest)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.data[i].white_list[0].oldest, actual.data[i].white_list[0].oldest)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.data[i].black_list[0], actual.data[i].black_list[0])
				<< " for i = " << i;
		}

		return ::testing::AssertionSuccess();
	}

};

TEST_F(AmdSmiDfcTable, InvalidParams)
{
	int ret;

	ret = amdsmi_get_dfc_fw_table(&GPU_MOCK_HANDLE, NULL);
#ifdef __linux__
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
#else
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
#endif
}

TEST_F(AmdSmiDfcTable, IoctlFailed)
{
	int ret;
	amdsmi_dfc_fw_t dfc_fw_table;
#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));
#endif
	ret = amdsmi_get_dfc_fw_table(&GPU_MOCK_HANDLE, &dfc_fw_table);
#ifdef __linux__
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
#else
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
#endif
}

#ifdef _WIN64
TEST_F(AmdSmiDfcTable, GetDfcFwTable)
{
	int ret;
	amdsmi_dfc_fw_t mocked_resp;
	struct smi_device_info in_payload;
	amdsmi_dfc_fw_t dfc_fw_table = {};

	mocked_resp.header.dfc_fw_version = 65537;
	mocked_resp.header.dfc_fw_total_entries = 2;
	mocked_resp.header.dfc_gart_wr_guest_min = 3;
	mocked_resp.header.dfc_gart_wr_guest_max = 4;

	for (unsigned int i = 0; i < mocked_resp.header.dfc_fw_total_entries; i++) {
		mocked_resp.data[i].dfc_fw_type = i + 2;
		mocked_resp.data[i].verification_enabled = 1;
		mocked_resp.data[i].customer_ordinal = 2;
		mocked_resp.data[i].white_list[0].latest = 10000;
		mocked_resp.data[i].white_list[0].oldest = 10001;
		mocked_resp.data[i].black_list[0] = 10002;
	}
	WhenCalling(std::bind(amdsmi_get_dfc_fw_table, &GPU_MOCK_HANDLE, &dfc_fw_table));
	ExpectCommand(SMI_CMD_CODE_GET_DFC_FW_TABLE);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_dfc_table(mocked_resp, dfc_fw_table));
}
#endif
