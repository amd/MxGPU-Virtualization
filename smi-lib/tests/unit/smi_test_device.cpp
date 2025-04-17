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

using testing::_;
using testing::DoAll;
using testing::Return;

#define UUID_LENGTH 37

class AmdSmiDeviceTests : public amdsmi::AmdSmiTest {
public:
};

TEST_F(AmdSmiDeviceTests, InvalidParams)
{
	int ret;
	amdsmi_socket_handle socket_handle = NULL;
	uint8_t fcn_idx = 0;
	uint32_t processor_index = AMDSMI_MAX_DEVICES + 100;
	unsigned int processor_count = AMDSMI_MAX_DEVICES;
	const char *BAD_GPU_MOCK_UUID{"9aff000rr3-0000-1000-801f-188c37cb1ee6"};

	amdsmi_processor_handle handle = &GPU_MOCK_HANDLE;
	amdsmi_vf_handle_t vf_handle = VF_MOCK_HANDLE;

	ret = amdsmi_get_processor_handle_from_bdf(MOCK_BDF, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_vf_handle_from_bdf(MOCK_BDF, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_device_bdf(handle, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_processor_handles(socket_handle, NULL, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_processor_handles(socket_handle, NULL, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_vf_handle_from_vf_index(handle, fcn_idx, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_index_from_processor_handle(NULL, &processor_index);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_index_from_processor_handle(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_processor_handle_from_index(processor_index, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_processor_handle_from_index(processor_index, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_processor_handles(socket_handle, &processor_count, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ret = amdsmi_get_processor_handle_from_index((uint8_t)processor_count, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_processor_handle_from_uuid(GPU_MOCK_UUID, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_processor_handle_from_uuid(NULL, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_processor_handle_from_uuid(BAD_GPU_MOCK_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_vf_handle_from_uuid(BAD_GPU_MOCK_UUID, &vf_handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_vf_handle_from_uuid(VF_MOCK_UUID, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_vf_handle_from_uuid(NULL, &vf_handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiDeviceTests, WrongUUIDCharSequence)
{
	char WRONG_CHAR_SEQUENCE_UUID[UUID_LENGTH]{"9aff0003*0000-1000-801f-188c37cb1ee6"};
	amdsmi_processor_handle handle = &GPU_MOCK_HANDLE;
	int ret;

	ret = amdsmi_get_processor_handle_from_uuid(WRONG_CHAR_SEQUENCE_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	strcpy(WRONG_CHAR_SEQUENCE_UUID, "9aff0003*0000-1000-801f-188c37cb1ee6");
	ret = amdsmi_get_processor_handle_from_uuid(WRONG_CHAR_SEQUENCE_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	strcpy(WRONG_CHAR_SEQUENCE_UUID, "9aff0003-r000-1000-801f-188c37cb1ee6");
	ret = amdsmi_get_processor_handle_from_uuid(WRONG_CHAR_SEQUENCE_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	strcpy(WRONG_CHAR_SEQUENCE_UUID, "9aff0003-0000*1000-801f-188c37cb1ee6");
	ret = amdsmi_get_processor_handle_from_uuid(WRONG_CHAR_SEQUENCE_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	strcpy(WRONG_CHAR_SEQUENCE_UUID, "9aff0003-0000-1r00-801f-188c37cb1ee6");
	ret = amdsmi_get_processor_handle_from_uuid(WRONG_CHAR_SEQUENCE_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	strcpy(WRONG_CHAR_SEQUENCE_UUID, "9aff0003-0000-1000*801f-188c37cb1ee6");
	ret = amdsmi_get_processor_handle_from_uuid(WRONG_CHAR_SEQUENCE_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	strcpy(WRONG_CHAR_SEQUENCE_UUID, "9aff0003-0000-1000-80uf-188c37cb1ee6");
	ret = amdsmi_get_processor_handle_from_uuid(WRONG_CHAR_SEQUENCE_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	strcpy(WRONG_CHAR_SEQUENCE_UUID, "9aff0003-0000-1000-801f*188c37cb1ee6");
	ret = amdsmi_get_processor_handle_from_uuid(WRONG_CHAR_SEQUENCE_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	strcpy(WRONG_CHAR_SEQUENCE_UUID, "9aff0003-0000-1000-801f-1&8c37cb1ee6");
	ret = amdsmi_get_processor_handle_from_uuid(WRONG_CHAR_SEQUENCE_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
	ret = amdsmi_get_processor_handle_from_uuid("\0", &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiDeviceTests, IoctlFailed)
{
	int ret;

	amdsmi_processor_handle handle;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_HANDLE)))
		.WillOnce(amdsmi::SetResponse(
			smi_ioctl_cmd{ {}, { AMDSMI_STATUS_API_FAILED }, { SMI_VERSION_MAX} }
		));

	ret = amdsmi_get_processor_handle_from_bdf(MOCK_BDF, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	testing::Mock::VerifyAndClearExpectations(amdsmi::g_system_mock.get());

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_ASIC_INFO)))
		.WillRepeatedly(amdsmi::SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_processor_handle_from_uuid(GPU_MOCK_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	testing::Mock::VerifyAndClearExpectations(amdsmi::g_system_mock.get());

	amdsmi_vf_handle_t vf_handle;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_HANDLE)))
		.WillOnce(amdsmi::SetResponse(
			smi_ioctl_cmd{ {}, { AMDSMI_STATUS_API_FAILED }, { SMI_VERSION_MAX} }
		));

	ret = amdsmi_get_vf_handle_from_bdf(MOCK_BDF, &vf_handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO)))
		.WillRepeatedly(amdsmi::SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_vf_handle_from_uuid(VF_MOCK_UUID, &vf_handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiDeviceTests, DeviceBDFNotFound)
{
	int ret;

	amdsmi_processor_handle handle;
	smi_get_handle_resp mocked_resp = {};
	mocked_resp.dev_id = { 0 };
	mocked_resp.vf_id.handle = 0;
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_HANDLE)))
		.WillOnce(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));

	ret = amdsmi_get_processor_handle_from_bdf(MOCK_BDF, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_FOUND);
}

TEST_F(AmdSmiDeviceTests, UnsupportedMethods)
{
	int ret;
	amdsmi_socket_handle socket_handle = NULL;
	amdsmi_processor_handle handle = &GPU_MOCK_HANDLE;
	uint32_t count;
	ret = amdsmi_get_socket_handles(&count, &socket_handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
	char name[12];
	ret = amdsmi_get_socket_info(socket_handle, 10, name);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
	processor_type_t type;
	ret = amdsmi_get_processor_type(handle, &type);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiDeviceTests, GetDeviceHandle)
{
	amdsmi_processor_handle handle;
	smi_get_handle_resp mocked_resp = {};
	mocked_resp.dev_id = GPU_MOCK_HANDLE;
	mocked_resp.vf_id.handle = 0;
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_HANDLE)))
		.WillOnce(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));

	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 1;
	server_info_mock.devices[0].bdf.as_uint = MOCK_BDF.as_uint;
	server_info_mock.devices[0].dev_id = GPU_MOCK_HANDLE;
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(amdsmi::SetPayload(server_info_mock), testing::Return(0)));

	int ret = amdsmi_get_processor_handle_from_bdf(MOCK_BDF, &handle);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(*(uint64_t*)handle, GPU_MOCK_HANDLE.handle);

	Mock::VerifyAndClearExpectations(amdsmi::GetSystemMock());

	mocked_resp.vf_id.handle = VF_MOCK_HANDLE.handle;
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_HANDLE)))
		.WillOnce(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));

	amdsmi_vf_handle_t vf_handle;
	ret = amdsmi_get_vf_handle_from_bdf(MOCK_BDF, &vf_handle);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(vf_handle.handle, VF_MOCK_HANDLE.handle);

}

TEST_F(AmdSmiDeviceTests, GetDeviceHandleVfHandleZero)
{
	smi_get_handle_resp mocked_resp = {};

	mocked_resp.vf_id.handle = VF_MOCK_HANDLE.handle;
	mocked_resp.vf_id.handle = 0;
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_HANDLE)))
		.WillOnce(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));

	amdsmi_vf_handle_t vf_handle;
	int ret = amdsmi_get_vf_handle_from_bdf(MOCK_BDF, &vf_handle);

	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_FOUND);

}

TEST_F(AmdSmiDeviceTests, GetDeviceHandleApiFailed)
{
	amdsmi_processor_handle handle;
	smi_get_handle_resp mocked_resp = {};
	mocked_resp.dev_id = GPU_MOCK_HANDLE;
	mocked_resp.vf_id.handle = 0;
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_HANDLE)))
		.WillOnce(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(amdsmi::SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	int ret = amdsmi_get_processor_handle_from_bdf(MOCK_BDF, &handle);

	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiDeviceTests, GetDeviceBdfForVf)
{
	amdsmi_bdf_t bdf;
	smi_device_info in_payload;
	smi_vf_static_info mocked_resp = {};

	mocked_resp.bdf.as_uint = MOCK_BDF.as_uint;

	WhenCalling(std::bind(amdsmi_get_vf_bdf, VF_MOCK_HANDLE, &bdf));
	ExpectCommand(SMI_CMD_CODE_GET_VF_STATIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	int ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(bdf.as_uint, MOCK_BDF.as_uint);
}

TEST_F(AmdSmiDeviceTests, GetDeviceBdfForDev)
{
	int ret;
	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 1;
	server_info_mock.devices[0].bdf.as_uint = MOCK_BDF.as_uint;
	server_info_mock.devices[0].dev_id = GPU_MOCK_HANDLE;

	amdsmi_bdf_t bdf;
	WhenCalling(std::bind(amdsmi_get_gpu_device_bdf, &GPU_MOCK_HANDLE, &bdf));
	ExpectCommand(SMI_CMD_CODE_GET_SERVER_STATIC_INFO);
	PlantMockOutput(&server_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_bdfs(bdf, MOCK_BDF));

	server_info_mock.num_devices = 0;
	WhenCalling(std::bind(amdsmi_get_gpu_device_bdf, &GPU_MOCK_HANDLE, &bdf));
	ExpectCommand(SMI_CMD_CODE_GET_SERVER_STATIC_INFO);
	PlantMockOutput(&server_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_FOUND);
}

TEST_F(AmdSmiDeviceTests, GetDeviceBdfForDevApiFailed)
{
	int ret;
	amdsmi_bdf_t bdf;
	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 1;
	server_info_mock.devices[0].bdf.as_uint = MOCK_BDF.as_uint;
	server_info_mock.devices[0].dev_id = GPU_MOCK_HANDLE;
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(amdsmi::SetResponseStatus(AMDSMI_STATUS_API_FAILED));
	ret = amdsmi_get_gpu_device_bdf(&GPU_MOCK_HANDLE, &bdf);

	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiDeviceTests, GetDeviceHandles)
{
	amdsmi_socket_handle socket_handle = NULL;
	unsigned int processor_count = 8;

	int ret = amdsmi_get_processor_handles(socket_handle, &processor_count, NULL);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(processor_count, 1);

	amdsmi_processor_handle* processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * processor_count);


	ret = amdsmi_get_processor_handles(socket_handle, &processor_count, &processors[0]);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(GPU_MOCK_HANDLE, *((smi_device_handle_t*)processors[0])));

	free(processors);
}

TEST_F(AmdSmiDeviceTests, GetVfHandleFromIdx)
{
	smi_vf_partition_info mocked_resp = {};
	uint8_t fcn_idx = 0;
	mocked_resp.partition[fcn_idx].id.handle = VF_MOCK_HANDLE.handle;
	mocked_resp.num_vf_enabled = 4;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO)))
		.WillOnce(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));

	amdsmi_vf_handle_t vf_handle;
	int ret = amdsmi_get_vf_handle_from_vf_index(&GPU_MOCK_HANDLE, fcn_idx, &vf_handle);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(VF_MOCK_HANDLE.handle, vf_handle.handle);
}

TEST_F(AmdSmiDeviceTests, GetVfHandleFromIdxInvalParam)
{
	smi_vf_partition_info mocked_resp = {};
	uint8_t fcn_idx = 6;
	mocked_resp.partition[fcn_idx].id.handle = VF_MOCK_HANDLE.handle;
	mocked_resp.num_vf_enabled = 4;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO)))
		.WillOnce(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));

	amdsmi_vf_handle_t vf_handle;
	int ret = amdsmi_get_vf_handle_from_vf_index(&GPU_MOCK_HANDLE, fcn_idx, &vf_handle);

	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiDeviceTests, GetVfHandleFromIdxApiFailed)
{
	uint8_t fcn_idx = 0;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO)))
		.WillOnce(amdsmi::SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	amdsmi_vf_handle_t vf_handle;
	int ret = amdsmi_get_vf_handle_from_vf_index(&GPU_MOCK_HANDLE, fcn_idx, &vf_handle);

	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiDeviceTests, GetDeviceBdfForVfNullBdf)
{
	int ret = amdsmi_get_vf_bdf(VF_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiDeviceTests, GetDeviceBdfForVfNotSupp)
{
	amdsmi_bdf_t bdf;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_VF_STATIC_INFO)))
		.WillOnce(amdsmi::SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED));

	int ret = amdsmi_get_vf_bdf(VF_MOCK_HANDLE, &bdf);

	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiDeviceTests, GetIndexFromDevice)
{
	amdsmi_socket_handle socket_handle = NULL;
	unsigned int processor_count = 8;
	uint32_t processor_index = AMDSMI_MAX_DEVICES;

	int ret = amdsmi_get_processor_handles(socket_handle, &processor_count, NULL);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);

	amdsmi_processor_handle* processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * processor_count);

	ret = amdsmi_get_processor_handles(socket_handle, &processor_count, &processors[0]);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(GPU_MOCK_HANDLE, *((smi_device_handle_t*)processors[0])));

	ret = amdsmi_get_index_from_processor_handle(processors[0], &processor_index);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(processor_index, 0);

	free(processors);
}

TEST_F(AmdSmiDeviceTests, GetIndexFromDeviceNotFound)
{
	uint32_t processor_index = AMDSMI_MAX_DEVICES;
	smi_device_handle_t MOCK_GPU_HANDLE_BAD = { ( GPU_MOCK_HANDLE.handle << 32) | 0x0003 };
	int ret = amdsmi_get_index_from_processor_handle(&MOCK_GPU_HANDLE_BAD, &processor_index);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_FOUND);
}

TEST_F(AmdSmiDeviceTests, GetDeviceFromIndex)
{
	uint32_t processor_index = 0;
	amdsmi_processor_handle device_handle = &GPU_MOCK_HANDLE;
	int ret = amdsmi_get_processor_handle_from_index(processor_index, &device_handle);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(GPU_MOCK_HANDLE.handle, *(uint64_t*)device_handle);
}

TEST_F(AmdSmiDeviceTests, DeviceForUUIDNotFound)
{
	int ret;
	amdsmi_processor_handle handle = &GPU_MOCK_HANDLE;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_ASIC_INFO)))
		.WillRepeatedly(amdsmi::SetResponseStatus(AMDSMI_STATUS_NOT_FOUND));

	ret = amdsmi_get_processor_handle_from_uuid(GPU_MOCK_UUID, &handle);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_FOUND);

	testing::Mock::VerifyAndClearExpectations(amdsmi::g_system_mock.get());

	smi_device_info in_payload;
	smi_asic_info gpu_info_mock = {};
	std::string name = "abcdef";
#ifdef _WIN64
	strcpy_s(gpu_info_mock.market_name, sizeof(gpu_info_mock.market_name), name.c_str());
#else
	strcpy(gpu_info_mock.market_name, name.c_str());
#endif

	gpu_info_mock.vendor_id = 2;
	gpu_info_mock.device_id = 3;
	gpu_info_mock.rev_id = 4;
#ifdef _WIN64
	strcpy_s(gpu_info_mock.asic_serial, sizeof(gpu_info_mock.asic_serial), "0x8A1F188C37CB1KE9");
#else
	strcpy(gpu_info_mock.asic_serial, "0x8A1F188C37CB1KE9");
#endif

	WhenCalling(std::bind(amdsmi_get_processor_handle_from_uuid, GPU_MOCK_UUID, &handle));
	ExpectCommand(SMI_CMD_CODE_GET_ASIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_FOUND);

	testing::Mock::VerifyAndClearExpectations(amdsmi::g_system_mock.get());

	smi_vf_partition_info mocked_resp = {};
	uint8_t fcn_idx = 0;
	mocked_resp.partition[fcn_idx].id.handle = VF_MOCK_HANDLE.handle;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO)))
		.WillRepeatedly(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_ASIC_INFO)))
		.WillRepeatedly(amdsmi::SetResponseStatus(AMDSMI_STATUS_NOT_FOUND));

	ret = amdsmi_get_vf_handle_from_uuid(VF_MOCK_UUID, &VF_MOCK_HANDLE);
	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_FOUND);

	testing::Mock::VerifyAndClearExpectations(amdsmi::g_system_mock.get());

	mocked_resp.num_vf_enabled = 2;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO)))
		.WillRepeatedly(testing::DoAll(amdsmi::SetPayload(mocked_resp), testing::Return(0)));

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_ASIC_INFO)))
		.WillRepeatedly(amdsmi::SetResponseStatus(AMDSMI_STATUS_INVAL));

	ret = amdsmi_get_vf_handle_from_uuid(VF_MOCK_UUID, &VF_MOCK_HANDLE);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiDeviceTests, GetDeviceHandleForUUID)
{
	int ret;
	amdsmi_processor_handle handle = &GPU_MOCK_HANDLE;
	smi_device_info in_payload;
	smi_asic_info gpu_info_mock = {};
	std::string name = "abcdef";
#ifdef _WIN64
	strcpy_s(gpu_info_mock.market_name, sizeof(gpu_info_mock.market_name), name.c_str());
#else
	strcpy(gpu_info_mock.market_name, name.c_str());
#endif

	gpu_info_mock.vendor_id = 2;
	gpu_info_mock.device_id = 3;
	gpu_info_mock.rev_id = 4;
#ifdef _WIN64
	strcpy_s(gpu_info_mock.asic_serial, sizeof(gpu_info_mock.asic_serial), "0x9A1F188C37CB1EE6");
#else
	strcpy(gpu_info_mock.asic_serial, "0x9A1F188C37CB1EE6");
#endif

	WhenCalling(std::bind(amdsmi_get_processor_handle_from_uuid, GPU_MOCK_UUID, &handle));
	ExpectCommand(SMI_CMD_CODE_GET_ASIC_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&gpu_info_mock);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiDeviceTests, GetVfHandleForUUID)
{
	amdsmi_vf_handle_t MOCK_VF_HANDLE_BAD = { ( VF_MOCK_HANDLE.handle << 32) | 0x0003 };

	int ret;
	smi_asic_info  gpu_info_mock = {};
	std::string name = "abcdef";
#ifdef _WIN64
	strcpy_s(gpu_info_mock.market_name, sizeof(gpu_info_mock.market_name), name.c_str());
#else
	strcpy(gpu_info_mock.market_name, name.c_str());
#endif
	gpu_info_mock.vendor_id = 2;
	gpu_info_mock.device_id = 3;
	gpu_info_mock.rev_id = 4;
#ifdef _WIN64
	strcpy_s(gpu_info_mock.asic_serial, sizeof(gpu_info_mock.asic_serial), "0x9A1F188C37CB1EE6");
#else
	strcpy(gpu_info_mock.asic_serial, "0x9A1F188C37CB1EE6");
#endif
	smi_vf_partition_info vf_partition_info = {};

	vf_partition_info.num_vf_enabled = 2;
	vf_partition_info.partition[0].id.handle = MOCK_VF_HANDLE_BAD.handle;
	vf_partition_info.partition[0].fb.fb_offset = 0;
	vf_partition_info.partition[0].fb.fb_size = 1024;
	vf_partition_info.partition[1].id.handle = VF_MOCK_HANDLE.handle;
	vf_partition_info.partition[1].fb.fb_offset = 2048;
	vf_partition_info.partition[1].fb.fb_size = 1024;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_VF_PARTITIONING_INFO)))
		.WillRepeatedly(testing::DoAll(amdsmi::SetPayload(vf_partition_info), testing::Return(0)));

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_ASIC_INFO)))
		.WillRepeatedly(testing::DoAll(amdsmi::SetPayload(gpu_info_mock), testing::Return(0)));

	ret = amdsmi_get_vf_handle_from_uuid(VF_MOCK_UUID, &VF_MOCK_HANDLE);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}
