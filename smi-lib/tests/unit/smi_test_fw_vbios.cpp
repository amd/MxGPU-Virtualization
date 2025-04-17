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

class AmdSmiFirmwareVbiosTests : public amdsmi::AmdSmiTest {
public:
protected:
	::testing::AssertionResult equal_vbios_info(smi_vbios_info expect,
						    amdsmi_vbios_info_t actual)
	{
		SMI_ASSERT_STR_EQ(expect.build_date, actual.build_date);
		SMI_ASSERT_STR_EQ(expect.name, actual.name);
		SMI_ASSERT_STR_EQ(expect.version, actual.version);
		SMI_ASSERT_STR_EQ(expect.part_number, actual.part_number);

		return ::testing::AssertionSuccess();
	}

	::testing::AssertionResult equal_board_info(smi_board_info expect,
						    amdsmi_board_info_t actual)
	{
		SMI_ASSERT_STR_EQ(expect.fru_id, actual.fru_id);
		SMI_ASSERT_STR_EQ(expect.model_number, actual.model_number);
		SMI_ASSERT_STR_EQ(expect.product_serial, actual.product_serial);
		SMI_ASSERT_STR_EQ(expect.product_name, actual.product_name);
		SMI_ASSERT_STR_EQ(expect.manufacturer_name, actual.manufacturer_name);

		return ::testing::AssertionSuccess();
	}

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
		for (int i = expect.num_fw_info; i < AMDSMI_FW_ID__MAX; i++) {
			SMI_ASSERT_EQ(0, actual.fw_info_list[i].fw_id) << " for i = " << i;
			SMI_ASSERT_EQ(0, actual.fw_info_list[i].fw_version) << " for i = " << i;
		}

		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdSmiFirmwareVbiosTests, InvalidParams)
{
	int ret;

	ret = amdsmi_get_gpu_vbios_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_board_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_fw_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiFirmwareVbiosTests, IoctlFailed)
{
	int ret;

	amdsmi_vbios_info_t vbios_res;
	amdsmi_board_info_t board_res;
	amdsmi_fw_info_t fw_info;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(amdsmi::SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_vbios_info(&GPU_MOCK_HANDLE, &vbios_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_gpu_board_info(&GPU_MOCK_HANDLE, &board_res);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_fw_info(&GPU_MOCK_HANDLE, &fw_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiFirmwareVbiosTests, GetVbiosInfo)
{
	int ret;
	smi_device_info in_payload;
	smi_vbios_info gpu_info_mock = {};
	std::string build_date = "2020/05/19 23:07";
	std::string name = "VG10 A1 D05318 32Mx128 8GB 300e/945m";
	std::string part_number = "113-D0531800-B04";
	std::string version = "017.003.000.007.015638";
#ifdef _WIN64
	strcpy_s(gpu_info_mock.build_date, sizeof(gpu_info_mock.build_date), build_date.c_str());
	strcpy_s(gpu_info_mock.name, sizeof(gpu_info_mock.name), name.c_str());
	strcpy_s(gpu_info_mock.part_number, sizeof(gpu_info_mock.part_number), part_number.c_str());
	strcpy_s(gpu_info_mock.version, sizeof(gpu_info_mock.version), version.c_str());
#else
	strcpy(gpu_info_mock.build_date, build_date.c_str());
	strcpy(gpu_info_mock.name, name.c_str());
	strcpy(gpu_info_mock.part_number, part_number.c_str());
	strcpy(gpu_info_mock.version, version.c_str());
#endif


	amdsmi_vbios_info_t vbios_info;
	WhenCalling(std::bind(amdsmi_get_gpu_vbios_info, &GPU_MOCK_HANDLE, &vbios_info));
	ExpectCommand(SMI_CMD_CODE_GET_VBIOS_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_vbios_info(gpu_info_mock, vbios_info));
}

TEST_F(AmdSmiFirmwareVbiosTests, GetBoardInfo)
{
	int ret;
	smi_device_info in_payload;
	smi_board_info gpu_info_mock = {};
	std::string fru_id = "abc";
	std::string model_number = "def";
	std::string product_serial = "ghk";
	std::string product_name = "GPU";
	std::string manufacturer_name = "AMD";
#ifdef _WIN64
	strcpy_s(gpu_info_mock.fru_id, sizeof(gpu_info_mock.fru_id), fru_id.c_str());
	strcpy_s(gpu_info_mock.model_number, sizeof(gpu_info_mock.model_number), model_number.c_str());
	strcpy_s(gpu_info_mock.product_serial, sizeof(gpu_info_mock.product_serial), product_serial.c_str());
	strcpy_s(gpu_info_mock.manufacturer_name, sizeof(gpu_info_mock.manufacturer_name), manufacturer_name.c_str());
	strcpy_s(gpu_info_mock.product_name, sizeof(gpu_info_mock.product_name), product_name.c_str());
#else
	strcpy(gpu_info_mock.fru_id, fru_id.c_str());
	strcpy(gpu_info_mock.model_number, model_number.c_str());
	strcpy(gpu_info_mock.product_serial, product_serial.c_str());
	strcpy(gpu_info_mock.manufacturer_name, manufacturer_name.c_str());
	strcpy(gpu_info_mock.product_name, product_name.c_str());
#endif
	amdsmi_board_info_t board_info;
	WhenCalling(std::bind(amdsmi_get_gpu_board_info, &GPU_MOCK_HANDLE, &board_info));
	ExpectCommand(SMI_CMD_CODE_GET_BOARD_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_board_info(gpu_info_mock, board_info));
}

TEST_F(AmdSmiFirmwareVbiosTests, GetFwInfo)
{
	int ret;
	amdsmi_fw_info_t fw_info;
	smi_device_info in_payload;
	smi_fw_info mocked_resp = {};
	mocked_resp.num_fw_info = SMI_FW_ID__MAX;

	for (int i = 0; i < SMI_FW_ID__MAX; i++) {
		mocked_resp.fw_info_list[i].fw_id = (enum smi_fw_block)i;
		mocked_resp.fw_info_list[i].fw_version = i * 2 + 1;
	}

	WhenCalling(std::bind(amdsmi_get_fw_info, &GPU_MOCK_HANDLE, &fw_info));
	ExpectCommand(SMI_CMD_CODE_GET_GPU_FW_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_fw_info(mocked_resp, fw_info));
}
