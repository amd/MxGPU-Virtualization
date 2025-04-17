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
using amdsmi::equal_handles;

class AmdSmiEccTests : public amdsmi::AmdSmiTest {
protected:
	::testing::AssertionResult equal_errors(struct smi_ecc_info expect, amdsmi_error_count_t actual)
	{
		SMI_ASSERT_EQ(expect.err_count.correctable_count, actual.correctable_count);
		SMI_ASSERT_EQ(expect.err_count.uncorrectable_count, actual.uncorrectable_count);
		SMI_ASSERT_EQ(expect.err_count.deferred_count, actual.deferred_count);

		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdSmiEccTests, InvalidParams)
{
	int ret;
	amdsmi_eeprom_table_record_t table_records;

	ret = amdsmi_get_gpu_total_ecc_count(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_ecc_count(&GPU_MOCK_HANDLE, AMDSMI_GPU_BLOCK_GFX, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_ecc_enabled(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_ras_feature_info(&GPU_MOCK_HANDLE, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_bad_page_info(&GPU_MOCK_HANDLE, NULL, NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_get_gpu_bad_page_info(&GPU_MOCK_HANDLE, NULL, &table_records);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiEccTests, IoctlFailed)
{
	int ret;
	amdsmi_ras_feature_t ras_feature;
	amdsmi_error_count_t ec;
	uint64_t enabled_blocks = 0;
	amdsmi_eeprom_table_record_t bad_pages;
	uint32_t size = 1;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_total_ecc_count(&GPU_MOCK_HANDLE, &ec);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_gpu_ecc_count(&GPU_MOCK_HANDLE, AMDSMI_GPU_BLOCK_GFX, &ec);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_gpu_ecc_enabled(&GPU_MOCK_HANDLE, &enabled_blocks);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_gpu_ras_feature_info(&GPU_MOCK_HANDLE, &ras_feature);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_gpu_bad_page_info(&GPU_MOCK_HANDLE, &size, &bad_pages);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiEccTests, GetEccCount)
{
	int ret;
	amdsmi_error_count_t ec;
	struct smi_device_info in_payload;
	struct smi_ecc_info mocked_resp = {};

	mocked_resp.err_count.correctable_count = 2;
	mocked_resp.err_count.uncorrectable_count = 1;
	mocked_resp.err_count.deferred_count = 4;
	WhenCalling(std::bind(amdsmi_get_gpu_total_ecc_count, &GPU_MOCK_HANDLE, &ec));
	ExpectCommand(SMI_CMD_CODE_GET_ECC_STATUS);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_errors(mocked_resp, ec));
}

TEST_F(AmdSmiEccTests, GetEccCountPerBlock)
{
	int ret;
	amdsmi_error_count_t ec;
	struct smi_ras_query_if in_payload;
	struct smi_ecc_info mocked_resp = {};

	mocked_resp.err_count.correctable_count = 2;
	mocked_resp.err_count.uncorrectable_count = 1;
	mocked_resp.err_count.deferred_count = 4;
	WhenCalling(std::bind(amdsmi_get_gpu_ecc_count, &GPU_MOCK_HANDLE, AMDSMI_GPU_BLOCK_GFX, &ec));
	ExpectCommand(SMI_CMD_CODE_GET_BLOCK_ECC_STATUS);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_errors(mocked_resp, ec));
}

TEST_F(AmdSmiEccTests, GetEccCaps)
{
	int ret;
	uint64_t enabled_blocks = 0;
	struct smi_data_query in_payload;
	union smi_data mocked_resp = {};

	mocked_resp.ras_caps = 0;
	mocked_resp.ras_caps |= AMDSMI_GPU_BLOCK_GFX | AMDSMI_GPU_BLOCK_VCN;
	WhenCalling(std::bind(amdsmi_get_gpu_ecc_enabled, &GPU_MOCK_HANDLE, &enabled_blocks));
	ExpectCommand(SMI_CMD_CODE_GET_SMI_DATA);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(mocked_resp.ras_caps, enabled_blocks);
}

TEST_F(AmdSmiEccTests, GetRasFeatureInfo)
{
	int ret;
	struct smi_ras_feature mocked_resp = {};
	struct smi_device_info in_payload;
	amdsmi_ras_feature_t ras_feature;

	mocked_resp.ras_eeprom_version = 123;
	mocked_resp.supported_ecc_correction_schema = 1;

	WhenCalling(std::bind(amdsmi_get_gpu_ras_feature_info, &GPU_MOCK_HANDLE, &ras_feature));
	ExpectCommand(SMI_CMD_CODE_GET_RAS_FEATURE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(amdsmi::equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(mocked_resp.ras_eeprom_version, ras_feature.ras_eeprom_version);
	ASSERT_EQ(mocked_resp.supported_ecc_correction_schema, ras_feature.supported_ecc_correction_schema);
}

TEST_F(AmdSmiEccTests, GetEccBadPageInfoAllocFail)
{
	int ret;
	amdsmi_eeprom_table_record_t bad_pages;
	uint32_t size = 1;
#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_))
				.WillOnce(testing::Return(nullptr));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_))
				.WillOnce(testing::Return(nullptr));
#endif
	ret = amdsmi_get_gpu_bad_page_info(&GPU_MOCK_HANDLE, &size, &bad_pages);
	ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
}

TEST_F(AmdSmiEccTests, GetEccBadPageInfoSizeZero)
{
	int ret;
	amdsmi_eeprom_table_record_t bad_pages;
	uint32_t size = 0;

	ret = amdsmi_get_gpu_bad_page_info(&GPU_MOCK_HANDLE, &size, &bad_pages);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiEccTests, GetEccBadPageInfoSizeOnly)
{
	int ret;
	uint32_t size = 32;
	struct smi_bad_page_info in_payload;
	struct smi_bad_page_record *eeprom_table;
	amdsmi_eeprom_table_record_t *bad_pages = NULL;
#ifdef _WIN64
	eeprom_table = (struct smi_bad_page_record*)calloc(1, sizeof(struct smi_bad_page_record));
#else
	eeprom_table = (struct smi_bad_page_record*)amdsmi::mem_aligned_alloc((void**)&eeprom_table, 4096, sizeof(struct smi_bad_page_record));
#endif
	eeprom_table->num_bad_page = 16;

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#endif

	WhenCalling(std::bind(amdsmi_get_gpu_bad_page_info, &GPU_MOCK_HANDLE,
			      &size, bad_pages));
	ExpectCommand(SMI_CMD_CODE_GET_BAD_PAGE_INFO);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(size, 16);
}

TEST_F(AmdSmiEccTests, GetEccBadPageInfoSizeAboveMax)
{
	int ret;
	amdsmi_eeprom_table_record_t *bad_pages;
	uint32_t size = AMDSMI_MAX_BAD_PAGE_RECORD + 1;
	struct smi_bad_page_info in_payload;
	struct smi_bad_page_record *eeprom_table;
#ifdef _WIN64
	eeprom_table = (struct smi_bad_page_record*)calloc(1, sizeof(struct smi_bad_page_record));
#else
	eeprom_table = (struct smi_bad_page_record*)amdsmi::mem_aligned_alloc((void**)&eeprom_table, 4096, sizeof(struct smi_bad_page_record));
#endif
	eeprom_table->num_bad_page = SMI_MAX_BAD_PAGE_RECORD;
	for (unsigned int i = 0; i < eeprom_table->num_bad_page; i++) {
		eeprom_table->bad_page[i].retired_page = i;
		eeprom_table->bad_page[i].ts = i*2;
		eeprom_table->bad_page[i].err_type = (unsigned char)(i*3);
		eeprom_table->bad_page[i].bank = (unsigned char)(i*4);
		eeprom_table->bad_page[i].cu = (unsigned char)(i*4);
		eeprom_table->bad_page[i].mem_channel = (unsigned char)(i*5);
		eeprom_table->bad_page[i].mcumc_id = (unsigned char)(i*6);
	}

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#endif
	bad_pages = (amdsmi_eeprom_table_record_t *)malloc(sizeof(amdsmi_eeprom_table_record_t) * size);
	WhenCalling(std::bind(amdsmi_get_gpu_bad_page_info, &GPU_MOCK_HANDLE,
			      &size, bad_pages));
	ExpectCommand(SMI_CMD_CODE_GET_BAD_PAGE_INFO);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();
	free(bad_pages);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(size, AMDSMI_MAX_BAD_PAGE_RECORD);
}

TEST_F(AmdSmiEccTests, GetEccBadPageInfoNoData)
{
	int ret;
	uint32_t size = 16;
	amdsmi_eeprom_table_record_t bad_pages[16];
	struct smi_bad_page_record *eeprom_table;

#ifdef _WIN64
	eeprom_table = (struct smi_bad_page_record*)calloc(1, sizeof(struct smi_bad_page_record));
#else
	eeprom_table = (struct smi_bad_page_record*)amdsmi::mem_aligned_alloc((void**)&eeprom_table, 4096, sizeof(struct smi_bad_page_record));
#endif
	eeprom_table->num_bad_page = 16;

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#endif

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_NO_DATA));

	ret = amdsmi_get_gpu_bad_page_info(&GPU_MOCK_HANDLE, &size, bad_pages);
	ASSERT_EQ(ret, AMDSMI_STATUS_NO_DATA);

}

TEST_F(AmdSmiEccTests, GetEccBadPageInfoBufferSizeNotSufficient)
{
	int ret;
	amdsmi_eeprom_table_record_t *bad_pages;
	uint32_t size = 2;
	struct smi_bad_page_info in_payload;
	struct smi_bad_page_record *eeprom_table;

#ifdef _WIN64
	eeprom_table = (struct smi_bad_page_record*)calloc(1, sizeof(struct smi_bad_page_record));
#else
	eeprom_table = (struct smi_bad_page_record*)amdsmi::mem_aligned_alloc((void**)&eeprom_table, 4096, sizeof(struct smi_bad_page_record));
#endif
	eeprom_table->num_bad_page = 4;

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#endif

	bad_pages = (amdsmi_eeprom_table_record_t *)malloc(sizeof(amdsmi_eeprom_table_record_t) * size);

	WhenCalling(std::bind(amdsmi_get_gpu_bad_page_info, &GPU_MOCK_HANDLE,
			      &size, bad_pages));
	ExpectCommand(SMI_CMD_CODE_GET_BAD_PAGE_INFO);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	free(bad_pages);
	ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}

TEST_F(AmdSmiEccTests, GetEccBadPageInfo)
{
	int ret;
	amdsmi_eeprom_table_record_t *bad_pages;
	uint32_t size = 16;
	struct smi_bad_page_info in_payload;
	struct smi_bad_page_record *eeprom_table;

#ifdef _WIN64
	eeprom_table = (struct smi_bad_page_record*)calloc(1, sizeof(struct smi_bad_page_record));
#else
	eeprom_table = (struct smi_bad_page_record*)amdsmi::mem_aligned_alloc((void**)&eeprom_table, 4096, sizeof(struct smi_bad_page_record));
#endif
	eeprom_table->num_bad_page = size;

	for (unsigned int i = 0; i < eeprom_table->num_bad_page; i++) {
		eeprom_table->bad_page[i].retired_page = i;
		eeprom_table->bad_page[i].ts = i*2;
		eeprom_table->bad_page[i].err_type = (unsigned char)(i*3);
		eeprom_table->bad_page[i].bank = (unsigned char)(i*4);
		eeprom_table->bad_page[i].cu = (unsigned char)(i*4);
		eeprom_table->bad_page[i].mem_channel = (unsigned char)(i*5);
		eeprom_table->bad_page[i].mcumc_id = (unsigned char)(i*6);
	}

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(eeprom_table));
#endif

	bad_pages = (amdsmi_eeprom_table_record_t *)malloc(sizeof(amdsmi_eeprom_table_record_t) * size);

	WhenCalling(std::bind(amdsmi_get_gpu_bad_page_info, &GPU_MOCK_HANDLE,
			      &size, bad_pages));
	ExpectCommand(SMI_CMD_CODE_GET_BAD_PAGE_INFO);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	free(bad_pages);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}
