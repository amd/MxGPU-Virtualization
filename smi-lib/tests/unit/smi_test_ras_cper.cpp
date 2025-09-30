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
#include "smi_utils.h"
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
void fill_cper_header_and_section(char* buffer, uint16_t sec_cnt, const guid_t& sec_type, uint32_t sec_offset);

guid_t CRASHDUMP					= AMD_CRASHDUMP
guid_t GPU_NONSTANDARD_ERROR		= AMD_GPU_NONSTANDARD_ERROR;

void fill_cper_header_and_section(char* buffer, uint16_t sec_cnt, const guid_t& sec_type, uint32_t sec_offset) {
    auto* hdr = reinterpret_cast<amdsmi_cper_hdr_t*>(buffer);
    memcpy(hdr->signature, "CPER", 4);
    hdr->record_length = 512;
    hdr->sec_cnt = sec_cnt;

    auto* section = reinterpret_cast<struct cper_sec_desc*>(buffer + sizeof(amdsmi_cper_hdr_t));
    section->revision_major = 1;
    section->revision_minor = 2;
    section->flag_mask = 0xAABBCCDD;
    section->sec_offset = sec_offset;
    memcpy(&section->sec_type, &sec_type, sizeof(guid_t));
}


TEST_F(AmdSmiRasCperTests, InvalidParams)
{
    int ret;
    char cper_data[1024];
    uint64_t buf_size = sizeof(cper_data);
    amdsmi_cper_hdr_t *cper_hdrs[10];
    uint64_t entry_count = 10;
    uint64_t cursor = 0;
    uint32_t severity_mask = 0;

    uint64_t afids[MAX_NUMBER_OF_AFIDS_PER_RECORD];
    uint32_t num_afids = MAX_NUMBER_OF_AFIDS_PER_RECORD;

    ret = amdsmi_get_gpu_cper_entries(NULL, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_get_gpu_cper_entries(&GPU_MOCK_HANDLE, severity_mask, NULL, &buf_size, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_get_gpu_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, NULL, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_get_gpu_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, NULL, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_get_gpu_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, NULL, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_get_gpu_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, NULL);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);


    ret = amdsmi_get_afids_from_cper(nullptr, sizeof(cper_data), afids, &num_afids);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_get_afids_from_cper(cper_data, sizeof(cper_data), nullptr, &num_afids);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

    ret = amdsmi_get_afids_from_cper(cper_data, sizeof(cper_data), afids, nullptr);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiRasCperTests, IoctlFailed) {
    int ret;
    char cper_data[1024];
    uint64_t buf_size = sizeof(cper_data);
    amdsmi_cper_hdr_t *cper_hdrs[10];
    uint64_t entry_count = 10;
    uint64_t cursor = 0;
    uint32_t severity_mask = 0;

    EXPECT_CALL(*g_system_mock, Ioctl(_)).WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

    ret = amdsmi_get_gpu_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiRasCperTests, GetCperEntriesAllocFail)
{
    int ret;
    char cper_data[1024];
    uint64_t buf_size = sizeof(cper_data);
    amdsmi_cper_hdr_t *cper_hdrs[10];
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
    ret = amdsmi_get_gpu_cper_entries(&GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor);
    ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
}

TEST_F(AmdSmiRasCperTests, GetCperEntriesSuccess)
{
    int ret;
    char cper_data[1024];
    uint64_t buf_size = sizeof(cper_data);
    amdsmi_cper_hdr_t *cper_hdrs[10];
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

    WhenCalling(std::bind(amdsmi_get_gpu_cper_entries, &GPU_MOCK_HANDLE, severity_mask, cper_data, &buf_size, cper_hdrs, &entry_count, &cursor));
    ExpectCommand(SMI_CMD_CODE_GET_CPER);
    SaveInputPayloadIn(&in_payload);
    ret = performCall();

    ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
    ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}

TEST_F(AmdSmiRasCperTests, AfidBufferTooSmall) {
    int ret;
    char cper_buffer[16]; // Insufficient buffer size
    uint64_t afids[MAX_NUMBER_OF_AFIDS_PER_RECORD];
    uint32_t num_afids = MAX_NUMBER_OF_AFIDS_PER_RECORD;

    ret = amdsmi_get_afids_from_cper(cper_buffer, sizeof(cper_buffer), afids, &num_afids);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiRasCperTests, AfidInvalidSignature) {
    int ret;
    char cper_buffer[1024];
    uint64_t afids[MAX_NUMBER_OF_AFIDS_PER_RECORD];
    uint32_t num_afids = MAX_NUMBER_OF_AFIDS_PER_RECORD;

    // Set an invalid signature in the CPER buffer
    memset(cper_buffer, 0, sizeof(cper_buffer));
    ret = amdsmi_get_afids_from_cper(cper_buffer, sizeof(cper_buffer), afids, &num_afids);
    ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiRasCperTests, AfidValidInput) {
    int ret;
    char cper_buffer[1024];
    uint64_t afids[MAX_NUMBER_OF_AFIDS_PER_RECORD];
    uint32_t num_afids = MAX_NUMBER_OF_AFIDS_PER_RECORD;

    // Set a valid CPER signature
    memcpy(cper_buffer, "CPER", 4);
    reinterpret_cast<amdsmi_cper_hdr_t*>(cper_buffer)->record_length = sizeof(cper_buffer);

    ret = amdsmi_get_afids_from_cper(cper_buffer, sizeof(cper_buffer), afids, &num_afids);
    ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
    ASSERT_LE(num_afids, static_cast<uint32_t>(MAX_NUMBER_OF_AFIDS_PER_RECORD));
}

TEST_F(AmdSmiRasCperTests, AfidSectionTypeNonStandard) {
    char buffer[1024] = {};
    uint64_t afids[MAX_NUMBER_OF_AFIDS_PER_RECORD] = {};
    uint32_t num_afids = MAX_NUMBER_OF_AFIDS_PER_RECORD;

    // Fill header and section for GPU_NONSTANDARD_ERROR
    fill_cper_header_and_section(buffer, 1, GPU_NONSTANDARD_ERROR, sizeof(amdsmi_cper_hdr_t) + sizeof(struct cper_sec_desc));

    // Fill section data
    auto* nonstd_err = reinterpret_cast<struct cper_sec_nonstd_err*>(buffer + sizeof(amdsmi_cper_hdr_t) + sizeof(struct cper_sec_desc));
    memset(nonstd_err, 0, sizeof(struct cper_sec_nonstd_err));

    int ret = amdsmi_get_afids_from_cper(buffer, sizeof(buffer), afids, &num_afids);
    ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
    ASSERT_EQ(num_afids, 1);
}

TEST_F(AmdSmiRasCperTests, AfidSectionTypeCrashdump) {
    char buffer[1024] = {};
    uint64_t afids[MAX_NUMBER_OF_AFIDS_PER_RECORD] = {};
    uint32_t num_afids = MAX_NUMBER_OF_AFIDS_PER_RECORD;

    // Fill header and section for CRASHDUMP
    fill_cper_header_and_section(buffer, 1, CRASHDUMP, sizeof(amdsmi_cper_hdr_t) + sizeof(struct cper_sec_desc));

    // Fill section data
    auto* crashdump = reinterpret_cast<struct cper_sec_crashdump_fatal*>(buffer + sizeof(amdsmi_cper_hdr_t) + sizeof(struct cper_sec_desc));
    memset(crashdump, 0, sizeof(struct cper_sec_crashdump_fatal));

    int ret = amdsmi_get_afids_from_cper(buffer, sizeof(buffer), afids, &num_afids);
    ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
    ASSERT_EQ(num_afids, 1);
}
