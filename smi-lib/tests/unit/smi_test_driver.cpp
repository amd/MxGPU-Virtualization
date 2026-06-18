/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "gtest/gtest.h"
#include <string.h>

extern "C" {
#include "amdsmi.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"


using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;
using testing::_;
using testing::DoAll;
using testing::Return;

class AmdSmiHostDriverTests : public amdsmi::AmdSmiTest {
};

TEST_F(AmdSmiHostDriverTests, InvalidParams)
{
	int ret;
	amdsmi_driver_info_t driver;
	amdsmi_driver_model_type_t model;

	ret = amdsmi_get_gpu_driver_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_driver_info(&NIC_MOCK_HANDLE, &driver);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_driver_model(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_driver_model(&NIC_MOCK_HANDLE, &model);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiHostDriverTests, IoctlFailed)
{
	amdsmi_driver_info_t driver_version;
	amdsmi_driver_model_type_t driver_model = AMDSMI_DRIVER_MODEL_TYPE_WDDM;
	int ret;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_driver_info(&GPU_MOCK_HANDLE, &driver_version);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	ret = amdsmi_get_gpu_driver_model(&GPU_MOCK_HANDLE, &driver_model);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiHostDriverTests, GetHostDriver)
{
	int ret;
	struct smi_device_info in_payload;
	struct smi_driver_info gpu_info_mock = {};
	amdsmi_driver_info_t driver_version;

	gpu_info_mock.version_len = 10;
#ifdef _WIN64
	strcpy_s(gpu_info_mock.version, sizeof(gpu_info_mock.version),  "12.34.567");
	strcpy_s(gpu_info_mock.libgv_build_date, sizeof(gpu_info_mock.libgv_build_date), "12.34.5678");
#else
	strcpy(gpu_info_mock.version, "12.34.567");
	strcpy(gpu_info_mock.libgv_build_date, "12.34.5678");
#endif

	for (int i = AMDSMI_DRIVER_LIBGV; i < AMDSMI_DRIVER__MAX; i++) {
		gpu_info_mock.id = (amdsmi_driver_t)i;
		WhenCalling(std::bind(amdsmi_get_gpu_driver_info, &GPU_MOCK_HANDLE, &driver_version));
		ExpectCommand(SMI_CMD_CODE_GET_GPU_DRIVER_INFO);
		SaveInputPayloadIn(&in_payload);
		PlantMockOutput(&gpu_info_mock);
		ret = performCall();
	}

	gpu_info_mock.id = AMDSMI_DRIVER_LIBGV;

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(!strcmp(gpu_info_mock.version, driver_version.driver_version));
	ASSERT_TRUE(!strcmp(gpu_info_mock.driver_date, driver_version.driver_date));
}

TEST_F(AmdSmiHostDriverTests, GetHostDriverModel)
{
	int ret;
	struct smi_device_info in_payload;
	struct smi_gpu_driver_model gpu_info_mock = {};
	amdsmi_driver_model_type_t driver_model;


	for (int i = AMDSMI_DRIVER_MODEL_TYPE_WDDM; i < AMDSMI_DRIVER_MODEL_TYPE__MAX; i++) {
		gpu_info_mock.model = (enum smi_driver_model_type)i;
		WhenCalling(std::bind(amdsmi_get_gpu_driver_model, &GPU_MOCK_HANDLE, &driver_model));
		ExpectCommand(SMI_CMD_CODE_GET_GPU_DRIVER_MODEL);
		SaveInputPayloadIn(&in_payload);
		PlantMockOutput(&gpu_info_mock);
		ret = performCall();
	}

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(gpu_info_mock.model, driver_model);
}
