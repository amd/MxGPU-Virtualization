/*
 * Copyright (c) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
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
#include "smi_vcs.h"
}
#include "smi_system_mock.hpp"

using namespace ::testing;
using amdsmi::SetResponseStatus;

namespace amdsmi
{
std::unique_ptr<NiceMock<SystemMock> > g_system_mock;
SystemMock *GetSystemMock()
{
	return g_system_mock.get();
}
} // namespace amdsmi

class AmdSmiUtilTests : public Test {
protected:
	void SetUp() override
	{
		amdsmi::g_system_mock.reset(new NiceMock<amdsmi::SystemMock>);
	}

	void TearDown() override
	{
		amdsmi::g_system_mock.reset();
	}
};

TEST_F(AmdSmiUtilTests, amdsmi_request_failed)
{
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(testing::_))
		.WillOnce(Return(1))
		.WillOnce(Return(0));

	auto res = amdsmi_request(NULL, 0, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_INVAL);

	smi_handle_struct handle;
	smi_thread_ctx thread;
	smi_req_ctx smi_req = { .handle = &handle, .thread = &thread };

	smi_req.thread->ioctl_cmd.out_hdr.status = AMDSMI_STATUS_SUCCESS;

	smi_req.handle->version = SMI_UNKNOWN_VERSION;
	res = amdsmi_request(&smi_req, SMI_CMD_CODE__MAX, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	smi_req.handle->version = SMI_VERSION_MAX;
	res = amdsmi_request(&smi_req, 0, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_UNKNOWN_ERROR);

	int sample_error_code = AMDSMI_STATUS_UNKNOWN_ERROR;
	smi_req.thread->ioctl_cmd.out_hdr.status = sample_error_code;
	res = amdsmi_request(&smi_req, 0, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_UNKNOWN_ERROR);
}

TEST_F(AmdSmiUtilTests, amdsmi_request_ioctl_cmd_failed)
{
	int res;
	smi_handle_struct handle;
	smi_thread_ctx thread;
	smi_req_ctx smi_req = { .handle = &handle, .thread = &thread };

	const int MOCK_STATUS = 0x123;
	smi_req.thread->ioctl_cmd.out_hdr.status = MOCK_STATUS;
	smi_req.handle->version = SMI_VERSION_MAX;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(testing::_))
		.WillOnce(Return(-1));

	errno = EIO;
	res = amdsmi_request(&smi_req, 0, 0, 0);
	EXPECT_EQ(res, MOCK_STATUS);
	errno = 0;
}

TEST_F(AmdSmiUtilTests, amdsmi_request_ioctl_not_supported)
{
	int res;
	smi_handle_struct handle;
	smi_thread_ctx thread;
	smi_req_ctx smi_req = { .handle = &handle, .thread = &thread };

	const int MOCK_STATUS = 0x123;
	smi_req.thread->ioctl_cmd.out_hdr.status = MOCK_STATUS;
	smi_req.handle->version = SMI_VERSION_MAX;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(testing::_))
		.WillOnce(Return(-1));

	errno = EACCES;
	res = amdsmi_request(&smi_req, 0, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);
	errno = 0;
}

TEST_F(AmdSmiUtilTests, previous_versions)
{
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(testing::_))
		.WillRepeatedly(Return(0));

	int res;
	smi_handle_struct handle;
	smi_thread_ctx thread;
	smi_req_ctx smi_req = { .handle = &handle, .thread = &thread };

	smi_req.thread->ioctl_cmd.out_hdr.status = AMDSMI_STATUS_SUCCESS;

	smi_req.handle->version = SMI_VERSION_ALPHA_0;

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_HANDSHAKE, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_GET_SERVER_STATIC_INFO, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	smi_req.handle->version = SMI_VERSION_BETA_0;

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_HANDSHAKE, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_GET_SERVER_STATIC_INFO, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	smi_req.handle->version = SMI_VERSION_BETA_2;

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_HANDSHAKE, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_GET_DFC_FW_TABLE, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

}

TEST_F(AmdSmiUtilTests, version_unknown)
{
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(testing::_))
		.WillRepeatedly(Return(0));

	int res;
	smi_handle_struct handle;
	smi_thread_ctx thread;
	smi_req_ctx smi_req = { .handle = &handle, .thread = &thread };

	smi_req.thread->ioctl_cmd.out_hdr.status = AMDSMI_STATUS_SUCCESS;
	smi_req.handle->version = SMI_UNKNOWN_VERSION;

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_HANDSHAKE, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_GET_SERVER_STATIC_INFO, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiUtilTests, version_too_big)
{
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(testing::_))
		.WillRepeatedly(Return(0));

	int res;
	smi_handle_struct handle;
	smi_thread_ctx thread;
	smi_req_ctx smi_req = { .handle = &handle, .thread = &thread };

	smi_req.thread->ioctl_cmd.out_hdr.status = AMDSMI_STATUS_SUCCESS;
	smi_req.handle->version = SMI_VERSION_MAX + 1;

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_HANDSHAKE, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);

	res = amdsmi_request(&smi_req, SMI_CMD_CODE_GET_SERVER_STATIC_INFO, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiUtilTests, amdsmi_request_succeeded)
{
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(testing::_))
		.WillOnce(Return(0));

	smi_handle_struct handle;
	smi_thread_ctx thread;
	smi_req_ctx smi_req = { .handle = &handle, .thread = &thread };

	smi_req.thread->ioctl_cmd.out_hdr.status = AMDSMI_STATUS_SUCCESS;
	smi_req.handle->version = SMI_VERSION_MAX;
	auto res = amdsmi_request(&smi_req, 0, 0, 0);
	EXPECT_EQ(res, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_success)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_SUCCESS, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_SUCCESS - Command has been executed successfully");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_api_failed)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	const int res = amdsmi_status_code_to_string((amdsmi_status_t)(AMDSMI_STATUS_SETTING_UNAVAILABLE+100), status_string);
	EXPECT_EQ(res, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_inval)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_INVAL, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_INVAL - Invalid parameters");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_not_supported)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NOT_SUPPORTED, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NOT_SUPPORTED - Command not supported");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_not_yet_implemented)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NOT_YET_IMPLEMENTED, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NOT_YET_IMPLEMENTED - Not implemented yet");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_fail_load_module)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_FAIL_LOAD_MODULE, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_FAIL_LOAD_MODULE - Fail to load lib");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_fail_load_symbol)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_FAIL_LOAD_SYMBOL, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_FAIL_LOAD_SYMBOL - Fail to load symbol");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_drm_error)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_DRM_ERROR, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_DRM_ERROR - Error when call libdrm");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_api_failed)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_API_FAILED, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_API_FAILED - API call failed");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_timeout)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_TIMEOUT, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_TIMEOUT - Timeout in API call");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_retry)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_RETRY, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_RETRY - Retry operation");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_no_perm)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NO_PERM, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NO_PERM - Permission Denied");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_interrupt)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_INTERRUPT, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_INTERRUPT - An interrupt occurred during execution of function");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_io)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_IO, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_IO - I/O Error");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_address_fault)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_ADDRESS_FAULT, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_ADDRESS_FAULT - Bad address");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_file_error)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_FILE_ERROR, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_FILE_ERROR - Problem accessing a file");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_out_of_resources)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_OUT_OF_RESOURCES, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_OUT_OF_RESOURCES - Not enough memory");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_internal_exception)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_INTERNAL_EXCEPTION, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_INTERNAL_EXCEPTION - An internal exception was caught");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_out_of_bounds)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS - The provided input is out of allowable or safe range");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_init_error)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_INIT_ERROR, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_INIT_ERROR - An error occurred when initializing internal data structures");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_refcount_overflow)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_REFCOUNT_OVERFLOW, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_REFCOUNT_OVERFLOW - An internal reference counter exceeded INT32_MAX");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_busy)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_BUSY, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_BUSY - Processor busy");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_not_found)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NOT_FOUND, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NOT_FOUND - Processor not found");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_not_init)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NOT_INIT, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NOT_INIT - Processor not initialized");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_no_slot)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NO_SLOT, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NO_SLOT - No more free slot");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_driver_not_loaded)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_DRIVER_NOT_LOADED, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_DRIVER_NOT_LOADED - Processor driver not loaded");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_no_data)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NO_DATA, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NO_DATA - No data was found for a given input");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_insufficient_size)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_INSUFFICIENT_SIZE, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_INSUFFICIENT_SIZE - Not enough resources were available for the operation");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_unexpected_size)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_UNEXPECTED_SIZE, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_UNEXPECTED_SIZE - An unexpected amount of data was read");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_unexpected_data)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_UNEXPECTED_DATA, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_UNEXPECTED_DATA - The data read or provided to function is not what was expected");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_non_amd_cpu)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NON_AMD_CPU, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NON_AMD_CPU - System has different cpu than AMD");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_no_energy_drv)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NO_ENERGY_DRV, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NO_ENERGY_DRV - Energy driver not found");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_no_msr_drv)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NO_MSR_DRV, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NO_MSR_DRV - MSR driver not found");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_no_hsmp_drv)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NO_HSMP_DRV, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NO_HSMP_DRV - HSMP driver not found");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_no_hsmp_sup)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NO_HSMP_SUP, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NO_HSMP_SUP - HSMP not supported");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_no_hsmp_msg_sup)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NO_HSMP_MSG_SUP, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NO_HSMP_MSG_SUP - HSMP message/feature not supported");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_hsmp_timeout)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_HSMP_TIMEOUT, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_HSMP_TIMEOUT - HSMP message is timedout");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_no_drv)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_NO_DRV, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_NO_DRV - No Energy and HSMP driver present");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_file_not_found)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_FILE_NOT_FOUND, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_FILE_NOT_FOUND - File or directory not found");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_arg_ptr_null)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_ARG_PTR_NULL, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_ARG_PTR_NULL - Parsed argument is invalid");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_amdgpu_restart_err)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_AMDGPU_RESTART_ERR, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_AMDGPU_RESTART_ERR - AMDGPU restart failed");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_setting_unavailable)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_SETTING_UNAVAILABLE, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_SETTING_UNAVAILABLE - Setting is not available");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_map_error)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_MAP_ERROR, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_MAP_ERROR - The internal library error did not map to a status code");
}

TEST_F(AmdSmiUtilTests, amdsmi_status_message_unknown_error)
{
	const char *status_str = NULL;
	const char **status_string = &status_str;

	amdsmi_status_code_to_string(AMDSMI_STATUS_UNKNOWN_ERROR, status_string);
	EXPECT_STREQ(status_str, "AMDSMI_STATUS_UNKNOWN_ERROR - An unknown error occurred");
}

TEST_F(AmdSmiUtilTests, guid_equals_true)
{
    guid_t g1 = { .b = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} };
    guid_t g2 = { .b = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} };
    EXPECT_TRUE(guid_equals(&g1, &g2));
}

TEST_F(AmdSmiUtilTests, guid_equals_false)
{
    guid_t g1 = { .b = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16} };
    guid_t g2 = { .b = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,99} };
    EXPECT_FALSE(guid_equals(&g1, &g2));
}

TEST_F(AmdSmiUtilTests, get_register_array_aligned)
{
    uint8_t data[16] = {1,0,0,0,0,0,0,0, 2,0,0,0,0,0,0,0};
    uint64_t reg[2] = {0, 0};
    amdsmi_get_register_array(data, sizeof(data), reg);
    EXPECT_EQ(reg[0], 1);
    EXPECT_EQ(reg[1], 2);
}

TEST_F(AmdSmiUtilTests, get_register_array_unaligned)
{
    uint8_t data[10] = {1,2,3,4,5,6,7,8, 9,10};
    uint64_t reg[2] = {0, 0};
    amdsmi_get_register_array(data, sizeof(data), reg);
    uint64_t expected = 0;
    memcpy(&expected, data, 8);
    EXPECT_EQ(reg[0], expected);
}

TEST_F(AmdSmiUtilTests, parse_cpu_list_simple_range)
{
    uint64_t cpu_set[2] = {0};
    // "0-3" should set bits 0,1,2,3
    ASSERT_EQ(parse_cpu_list("0-3", cpu_set, 2), 0);
    EXPECT_EQ(cpu_set[0], 0xF);
    EXPECT_EQ(cpu_set[1], 0);
}

TEST_F(AmdSmiUtilTests, parse_cpu_list_empty_string)
{
    uint64_t cpu_set[2] = {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF};
    ASSERT_EQ(parse_cpu_list("", cpu_set, 2), 0);
    EXPECT_EQ(cpu_set[0], 0);
    EXPECT_EQ(cpu_set[1], 0);
}

TEST_F(AmdSmiUtilTests, parse_cpu_list_out_of_bounds)
{
    uint64_t cpu_set[1] = {0};
    // "0-100" with cpu_set_size=1, only bits 0-63 should be set
    ASSERT_EQ(parse_cpu_list("0-100", cpu_set, 1), 0);
    EXPECT_EQ(cpu_set[0], 0xFFFFFFFFFFFFFFFFULL);
}

TEST_F(AmdSmiUtilTests, parse_cpu_list_full_range)
{
    uint64_t cpu_set[2] = {0};
    // "0-15,32-56" should set bits 0-15 and 32-56
    ASSERT_EQ(parse_cpu_list("0-15,32-56", cpu_set, 2), 0);

    // Check bits 0-15
    for (int i = 0; i <= 15; ++i) {
        EXPECT_TRUE(cpu_set[i / 64] & (1ULL << (i % 64)));
    }
    // Check bits 16-31 are not set
    for (int i = 16; i <= 31; ++i) {
        EXPECT_FALSE(cpu_set[i / 64] & (1ULL << (i % 64)));
    }
    // Check bits 32-56
    for (int i = 32; i <= 56; ++i) {
        EXPECT_TRUE(cpu_set[i / 64] & (1ULL << (i % 64)));
    }
    // Check bits 57-63 are not set
    for (int i = 57; i <= 63; ++i) {
        EXPECT_FALSE(cpu_set[i / 64] & (1ULL << (i % 64)));
    }
}

TEST_F(AmdSmiUtilTests, smi_get_vf_device_id_from_pf_not_found)
{
    uint64_t vf_device_id = 0;
    int ret;

    ret = smi_get_vf_device_id_from_pf(0x1234, &vf_device_id);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(vf_device_id, 0xFFFF);

    ret = smi_get_vf_device_id_from_pf(0x0, &vf_device_id);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(vf_device_id, 0xFFFF);
}
