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
#include "smi_utils.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;
using testing::_;
using testing::DoAll;
using testing::Return;

class AmdsmiPtlTest : public amdsmi::AmdSmiTest {
protected:
	void SetUp() override {
		amdsmi::AmdSmiTest::SetUp();
	}

	void TearDown() override {
		amdsmi::AmdSmiTest::TearDown();
	}
};

TEST_F(AmdsmiPtlTest, IoctlFailed) {
	int ret;
	bool enabled = false;
	amdsmi_ptl_data_format_t format1, format2;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_ptl_state(&GPU_MOCK_HANDLE, &enabled);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_set_gpu_ptl_state(&GPU_MOCK_HANDLE, true);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_gpu_ptl_formats(&GPU_MOCK_HANDLE, &format1, &format2);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_set_gpu_ptl_formats(&GPU_MOCK_HANDLE, 
					 AMDSMI_PTL_DATA_FORMAT_F32,
					 AMDSMI_PTL_DATA_FORMAT_F64);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdsmiPtlTest, TestGetPtlState) {
	int ret;
	bool enabled = false;
	struct smi_device_info in_payload;
	struct smi_get_gpu_ptl_state mocked_resp = {};
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	mocked_resp.enabled = true;

	WhenCalling(std::bind(amdsmi_get_gpu_ptl_state, MOCK_GPU_HANDLE, &enabled));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_PTL_STATE);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	EXPECT_EQ(enabled, true);
}

TEST_F(AmdsmiPtlTest, TestSetPtlStateEnable) {
	int ret;
	struct smi_set_gpu_ptl_state in_payload;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_set_gpu_ptl_state, MOCK_GPU_HANDLE, true));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_PTL_STATE);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	EXPECT_EQ(in_payload.enable, true);
}

TEST_F(AmdsmiPtlTest, TestSetPtlStateDisable) {
	int ret;
	struct smi_set_gpu_ptl_state in_payload;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_set_gpu_ptl_state, MOCK_GPU_HANDLE, false));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_PTL_STATE);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	EXPECT_EQ(in_payload.enable, false);
}

TEST_F(AmdsmiPtlTest, TestGetPtlFormats) {
	int ret;
	amdsmi_ptl_data_format_t format1, format2;
	struct smi_device_info in_payload;
	struct smi_get_gpu_ptl_formats mocked_resp = {};
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	mocked_resp.data_format1 = SMI_PTL_DATA_FORMAT_F16;
	mocked_resp.data_format2 = SMI_PTL_DATA_FORMAT_BF16;

	WhenCalling(std::bind(amdsmi_get_gpu_ptl_formats, MOCK_GPU_HANDLE, 
			      &format1, &format2));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_PTL_FORMATS);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	EXPECT_EQ(format1, AMDSMI_PTL_DATA_FORMAT_F16);
	EXPECT_EQ(format2, AMDSMI_PTL_DATA_FORMAT_BF16);
}

TEST_F(AmdsmiPtlTest, TestSetPtlFormats) {
	int ret;
	struct smi_set_gpu_ptl_formats in_payload;
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	WhenCalling(std::bind(amdsmi_set_gpu_ptl_formats, MOCK_GPU_HANDLE,
			      AMDSMI_PTL_DATA_FORMAT_F32,
			      AMDSMI_PTL_DATA_FORMAT_BF16));
	ExpectCommand(SMI_CMD_CODE_SET_GPU_PTL_FORMATS);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	EXPECT_EQ(in_payload.data_format1, SMI_PTL_DATA_FORMAT_F32);
	EXPECT_EQ(in_payload.data_format2, SMI_PTL_DATA_FORMAT_BF16);
}

TEST_F(AmdsmiPtlTest, InvalidParams) {
	bool enabled = false;
	amdsmi_ptl_data_format_t format1, format2;

	// Test NULL processor handle
	ASSERT_EQ(amdsmi_get_gpu_ptl_state(nullptr, &enabled), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_ptl_state(nullptr, true), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_ptl_formats(nullptr, &format1, &format2), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_ptl_formats(nullptr, 
					     AMDSMI_PTL_DATA_FORMAT_I8,
					     AMDSMI_PTL_DATA_FORMAT_F64), AMDSMI_STATUS_INVAL);

	// Test NULL output pointers
	ASSERT_EQ(amdsmi_get_gpu_ptl_state(&GPU_MOCK_HANDLE, nullptr), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_ptl_formats(&GPU_MOCK_HANDLE, nullptr, &format2), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_ptl_formats(&GPU_MOCK_HANDLE, &format1, nullptr), AMDSMI_STATUS_INVAL);

	// Test invalid processor type (NIC handle)
	ASSERT_EQ(amdsmi_get_gpu_ptl_state(&NIC_MOCK_HANDLE, &enabled), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_ptl_state(&NIC_MOCK_HANDLE, true), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_ptl_formats(&NIC_MOCK_HANDLE, &format1, &format2), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_gpu_ptl_formats(&NIC_MOCK_HANDLE,
					     AMDSMI_PTL_DATA_FORMAT_I8,
					     AMDSMI_PTL_DATA_FORMAT_F64), AMDSMI_STATUS_INVAL);
}

