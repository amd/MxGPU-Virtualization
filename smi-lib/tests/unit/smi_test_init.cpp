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

#include <memory>
#include <thread>

#include "gtest/gtest.h"

extern "C" {
#include "smi_vcs.h"
#include "amdsmi.h"
#include "smi_os_defines.h"
}

#include "smi_test_helpers.hpp"

#ifdef _WIN64
#define SET_SYSTEM_ERROR(x) SetLastError(x);
#else
#define SET_SYSTEM_ERROR(x) errno = x;
#endif

using amdsmi::g_system_mock;
using amdsmi::SetPayload;
using amdsmi::SetResponse;
using amdsmi::SetResponseStatus;
using testing::_;
using testing::DoAll;

ACTION_P(SetBadVersionResponse, versionResponse)
{
	smi_ioctl_cmd payload = { {},
				  { AMDSMI_STATUS_NOT_SUPPORTED },
				  { (uint32_t)versionResponse } };
	std::memcpy(arg0, &payload, sizeof(payload));
	return 0; // IOCTL successful
}

ACTION_P(SetGoodVersionResponse, versionResponse)
{
	smi_ioctl_cmd payload = { {},
				  { AMDSMI_STATUS_SUCCESS },
				  { (uint32_t)versionResponse } };
	std::memcpy(arg0, &payload, sizeof(payload));
}

class AmdSmiInitTests : public ::testing::Test {
protected:
	void SetUp() override
	{
		g_system_mock.reset(new NiceMock<amdsmi::SystemMock>);
	}

	void TearDown() override
	{
		amdsmi_shut_down();

		g_system_mock.reset();
	}
};

TEST_F(AmdSmiInitTests, InitTest_OpenFailed)
{
	int res;

	EXPECT_CALL(*g_system_mock, Open(_))
		.WillRepeatedly(Return(SMI_INVAL_HANDLE));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_API_FAILED);

	EXPECT_CALL(*g_system_mock, Open(_))
		.WillRepeatedly(Invoke([](auto) {
			SET_SYSTEM_ERROR(SMI_ACCESS_DENIED);
			return SMI_INVAL_HANDLE;
		}));;
	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_NO_PERM);

	SET_SYSTEM_ERROR(0);
}

TEST_F(AmdSmiInitTests, BadInitTest_IoctlCrash)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(Return(-1));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_UNKNOWN_ERROR);
}

TEST_F(AmdSmiInitTests, IoctlGetDevicesCrash)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_HANDSHAKE)))
		.WillOnce(testing::DoAll(SetPayload(SMI_VERSION_MAX),
					 testing::Return(0)));

	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 7;
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(SetPayload(server_info_mock), testing::Return(-1)));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiInitTests, VersionInSupportedRangeOfSmi)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_HANDSHAKE)))
		.WillOnce(SetResponse(
			smi_ioctl_cmd{ {}, { -1 }, { SMI_VERSION_MAX + 1} }))
		.WillOnce(SetResponse(
			smi_ioctl_cmd{ {}, { AMDSMI_STATUS_SUCCESS }, { SMI_VERSION_MAX} }));

	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 7;
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(SetPayload(server_info_mock), testing::Return(0)));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiInitTests, BadInitTest_AccessRights)
{
	int res;

	EXPECT_CALL(*g_system_mock, Access())
		.WillRepeatedly(Return(-1));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_DRIVER_NOT_LOADED);
}

TEST_F(AmdSmiInitTests, InitTest_CloseFailed)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_HANDSHAKE)))
		.WillOnce(testing::DoAll(SetPayload(SMI_VERSION_MAX),
					 testing::Return(0)));

	smi_server_static_info server_info_mock = {};

	server_info_mock.num_devices = 7;
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(SetPayload(server_info_mock), testing::Return(0)));


	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	EXPECT_CALL(*g_system_mock, Close(_))
		.WillOnce(Return(-1))
		.WillRepeatedly(Return(0));

	res = amdsmi_shut_down();
	ASSERT_EQ(res, AMDSMI_STATUS_IO);

	res = amdsmi_shut_down();
	ASSERT_EQ(res, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiInitTests, InitTest_UnkownVersion)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(SetBadVersionResponse(SMI_UNKNOWN_VERSION))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiInitTests, InitTest_TooBigVersion)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(SetBadVersionResponse(SMI_VERSION_MAX + 1))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiInitTests, InitTest_TooSmallVersion)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_HANDSHAKE)))
		.WillOnce(SetBadVersionResponse(SMI_VERSION_MIN))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiInitTests, InitTest_Success)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_HANDSHAKE)))
		.WillOnce(testing::DoAll(SetPayload(SMI_VERSION_MAX),
					 testing::Return(0)));

	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 7;
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(SetPayload(server_info_mock), testing::Return(0)));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiInitTests, InitTest_SuccessAfterFail)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(testing::DoAll(SetPayload(SMI_VERSION_MAX),
					 testing::Return(0)));

	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 7;
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(SetPayload(server_info_mock), testing::Return(0)));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiInitTests, InitTest_FailureAfterSuccess)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(testing::DoAll(SetPayload(SMI_VERSION_MAX),
					 testing::Return(0)));

	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 7;
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(SetPayload(server_info_mock), testing::Return(0)));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	res = amdsmi_shut_down();
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiInitTests, InitTest_SuccessFailureSucess)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(testing::DoAll(SetPayload(SMI_VERSION_MAX),
					 testing::Return(0)));

	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 7;
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(SetPayload(server_info_mock), testing::Return(0)));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	res = amdsmi_shut_down();
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());
	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED))
		.WillOnce(testing::DoAll(SetPayload(SMI_VERSION_MAX),
					 testing::Return(0)));

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(testing::DoAll(SetPayload(server_info_mock), testing::Return(0)));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiInitTests, InitTest_FailureSucessFailure)
{
	int res;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(testing::DoAll(SetPayload(SMI_VERSION_MAX),
					 testing::Return(0)));

	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 1;
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(DoAll(SetPayload(server_info_mock), Return(0)));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	res = amdsmi_shut_down();
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED))
		.WillOnce(SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED));

	res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiInitTests, InitTest_TwoThreads)
{
	int fini_res = AMDSMI_STATUS_NOT_SUPPORTED;
	int dev_cnt_res = AMDSMI_STATUS_UNKNOWN_ERROR;
	unsigned int dev_cnt = AMDSMI_MAX_DEVICES;
	amdsmi_processor_handle* processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*dev_cnt);
	amdsmi_socket_handle socket_handles = NULL;
	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 7;

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_HANDSHAKE)))
		.WillOnce(SetResponse(
			smi_ioctl_cmd{ {}, { AMDSMI_STATUS_SUCCESS }, { SMI_VERSION_MAX } }));

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(DoAll(SetPayload(server_info_mock), Return(0)));

	int res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	auto body = [&fini_res, &dev_cnt_res, socket_handles, &dev_cnt, &processors]() {
		dev_cnt_res = amdsmi_get_processor_handles(socket_handles, &dev_cnt, processors);
		fini_res = amdsmi_shut_down();
	};

	std::thread tt(body);
	tt.join();

	free(processors);
	ASSERT_EQ(fini_res, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(dev_cnt_res, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(dev_cnt, server_info_mock.num_devices);
}

TEST_F(AmdSmiInitTests, InitTest_TwoThreadsFiniAtEnd)
{
	int fini_res = AMDSMI_STATUS_NOT_SUPPORTED;
	int dev_cnt_res = AMDSMI_STATUS_NOT_SUPPORTED;
	unsigned int dev_cnt = AMDSMI_MAX_DEVICES;
	amdsmi_processor_handle* processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*dev_cnt);
	amdsmi_socket_handle socket_handles = NULL;

	smi_server_static_info server_info_mock = {};
	server_info_mock.num_devices = 7;

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_HANDSHAKE)))
		.WillOnce(SetResponse(
			smi_ioctl_cmd{ {}, { AMDSMI_STATUS_SUCCESS }, { SMI_VERSION_MAX } }));

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_SERVER_STATIC_INFO)))
		.WillOnce(DoAll(SetPayload(server_info_mock), Return(0)));

	int res = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	testing::Mock::VerifyAndClearExpectations(g_system_mock.get());

	auto body = [&dev_cnt_res, socket_handles, &dev_cnt, &processors]() {
		dev_cnt_res = amdsmi_get_processor_handles(socket_handles, &dev_cnt, processors);
	};

	std::thread tt(body);
	tt.join();

	fini_res = amdsmi_shut_down();

	free(processors);
	ASSERT_EQ(fini_res, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(dev_cnt_res, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(dev_cnt, server_info_mock.num_devices);
}
