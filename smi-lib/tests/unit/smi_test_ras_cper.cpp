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
using amdsmi::equal_handles;

class AmdSmiRasCperTests : public amdsmi::AmdSmiTest {
protected:
    smi_device_handle_t GPU_MOCK_HANDLE_DIFF = { (0x1234ULL << 32) | 0x4321 };
};

TEST_F(AmdSmiRasCperTests, InvalidParams)
{
    int ret;
    char cper_data[1024];
    uint64_t buf_size = sizeof(cper_data);
    amdsmi_cper_hdr *cper_hdrs[10];
    uint64_t entry_count = 10;
    uint64_t cursor = 0;
    uint32_t severity_mask = 0;

    ret = amdsmi_gpu_get_cper_entries(NULL, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_gpu_get_cper_entries(&GPU_MOCK_HANDLE, severity_mask, NULL, &buf_size, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_gpu_get_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, NULL, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_gpu_get_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, NULL, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_gpu_get_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, NULL, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_gpu_get_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, NULL);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiRasCperTests, IoctlFailed) {
    int ret;
    char cper_data[1024];
    uint64_t buf_size = sizeof(cper_data);
    amdsmi_cper_hdr *cper_hdrs[10];
    uint64_t entry_count = 10;
    uint64_t cursor = 0;
    uint32_t severity_mask = 0;

    EXPECT_CALL(*g_system_mock, Ioctl(_)).WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

    ret = amdsmi_gpu_get_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiRasCperTests, GetCperEntriesAllocFail)
{
    int ret;
    char cper_data[1024];
    uint64_t buf_size = sizeof(cper_data);
    amdsmi_cper_hdr *cper_hdrs[10];
    uint64_t entry_count = 10;
    uint64_t cursor = 0;
    uint32_t severity_mask = 0;

#ifdef _WIN64
    EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_))
                .WillOnce(testing::Return(nullptr));
#else
    EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_))
                .WillOnce(testing::Return(nullptr));
#endif
    ret = amdsmi_gpu_get_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
}

TEST_F(AmdSmiRasCperTests, GetCperEntriesSuccess)
{
    int ret;
    char cper_data[1024];
    uint64_t buf_size = sizeof(cper_data);
    amdsmi_cper_hdr *cper_hdrs[10];
    uint64_t entry_count = 10;
    uint64_t cursor = 0;
    uint32_t severity_mask = 3;

    struct smi_cper_config in_payload;
    struct smi_cper *cper;
#ifdef _WIN64
    cper = (struct smi_cper*)calloc(1, sizeof(struct smi_cper));
#else
    cper = (struct smi_cper*)amdsmi::mem_aligned_alloc((void**)&cper, 4096, sizeof(struct smi_cper));
#endif
    cper->entry_count = entry_count;

#ifdef _WIN64
    EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(cper));
#else
    EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(cper));
#endif

    WhenCalling(std::bind(amdsmi_gpu_get_cper_entries, &GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor));
    ExpectCommand(SMI_CMD_CODE_GET_CPER);
    SaveInputPayloadIn(&in_payload);
    ret = performCall();

    ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
    ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}

TEST_F(AmdSmiRasCperTests, MoreData) {
    int ret;
    char cper_data[1024];
    uint64_t buf_size = sizeof(cper_data);
    amdsmi_cper_hdr* cper_hdrs[10];
    uint64_t entry_count = 10;
    uint64_t cursor = 0;
    uint32_t severity_mask = 0;

    EXPECT_CALL(*g_system_mock, Ioctl(_)).WillRepeatedly(SetResponseStatus(SMI_STATUS_MORE_DATA));

    ret = amdsmi_gpu_get_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_MORE_DATA);
}
