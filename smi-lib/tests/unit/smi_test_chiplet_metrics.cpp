/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

class AmdSmiMetricsTest : public amdsmi::AmdSmiTest {
protected:
	smi_device_handle_t GPU_MOCK_HANDLE_DIFF = { (0x1234ULL << 32) | 0x4321 };
};

TEST_F(AmdSmiMetricsTest, IoctlFailed)
{
	int ret;
	amdsmi_metric_t metrics;
	uint32_t size = 1;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_gpu_metrics(&GPU_MOCK_HANDLE, &size, &metrics);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiMetricsTest, InvalidParams)
{
	amdsmi_metric_t metrics;

	ASSERT_EQ(amdsmi_get_gpu_metrics(&GPU_MOCK_HANDLE, NULL, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_metrics(&GPU_MOCK_HANDLE, NULL, &metrics), AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiMetricsTest, GetChipletMetricsAllocFail)
{
	int ret;
	amdsmi_metric_t metrics;
	uint32_t size = 1;
#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_))
				.WillOnce(testing::Return(nullptr));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_))
				.WillOnce(testing::Return(nullptr));
#endif
	ret = amdsmi_get_gpu_metrics(&GPU_MOCK_HANDLE, &size, &metrics);
	ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
}

TEST_F(AmdSmiMetricsTest, ChipletMetricSizeZero)
{
	int ret;
	amdsmi_metric_t metrics;
	uint32_t size = 0;

	ret = amdsmi_get_gpu_metrics(&GPU_MOCK_HANDLE, &size, &metrics);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiMetricsTest, GetChipletMetrics)
{
	int ret;
	amdsmi_metric_t *metrics;
	uint32_t size = 7;
	struct smi_metrics_table in_payload;
	struct smi_metrics *metrics_table;
#ifdef _WIN64
	metrics_table = (struct smi_metrics*)calloc(1, sizeof(struct smi_metrics));
#else
	metrics_table = (struct smi_metrics*)amdsmi::mem_aligned_alloc((void**)&metrics_table, 4096, sizeof(struct smi_metrics));
#endif
	metrics_table->num_metric = size;

	for (unsigned int i = 0; i < metrics_table->num_metric; i++) {
		metrics_table->metric[i].metric_union.code = i;
		metrics_table->metric[i].val = i*2;
		metrics_table->metric[i].vf_mask = i*3;
	}
#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(metrics_table));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(metrics_table));
#endif

	metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t) * size);

	WhenCalling(std::bind(amdsmi_get_gpu_metrics, &GPU_MOCK_HANDLE,
			      &size, metrics));
	ExpectCommand(SMI_CMD_CODE_GET_METRICS_TABLE);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	free(metrics);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}

TEST_F(AmdSmiMetricsTest, DISABLED_GetChipletMetricsWrongSize)
{
	int ret;
	amdsmi_metric_t *metrics;
	uint32_t size = 6;
	struct smi_metrics_table in_payload;
	struct smi_metrics *metrics_table;

#ifdef _WIN64
	metrics_table = (struct smi_metrics*)calloc(1, sizeof(struct smi_metrics));
#else
	metrics_table = (struct smi_metrics*)amdsmi::mem_aligned_alloc((void**)&metrics_table, 4096, sizeof(struct smi_metrics));
#endif
	metrics_table->num_metric = 7;

	for (unsigned int i = 0; i < metrics_table->num_metric; i++) {
		metrics_table->metric[i].metric_union.code = i;
		metrics_table->metric[i].val = i*2;
		metrics_table->metric[i].vf_mask = i*3;
	}

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(metrics_table));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(metrics_table));
#endif

	metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t) * size);

	WhenCalling(std::bind(amdsmi_get_gpu_metrics, &GPU_MOCK_HANDLE,
			      &size, metrics));
	ExpectCommand(SMI_CMD_CODE_GET_METRICS_TABLE);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	free(metrics);
	ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}

TEST_F(AmdSmiMetricsTest, GetChipletMetricsSizeOutOfRange)
{
	int ret;
	amdsmi_metric_t *metrics;
	uint32_t size = AMDSMI_MAX_NUM_METRICS + 1;
	struct smi_metrics_table in_payload;
	struct smi_metrics *metrics_table;

#ifdef _WIN64
	metrics_table = (struct smi_metrics*)calloc(1, sizeof(struct smi_metrics));
#else
	metrics_table = (struct smi_metrics*)amdsmi::mem_aligned_alloc((void**)&metrics_table, 4096, sizeof(struct smi_metrics));
#endif
	metrics_table->num_metric = 7;

	for (unsigned int i = 0; i < metrics_table->num_metric; i++) {
		metrics_table->metric[i].metric_union.code = i;
		metrics_table->metric[i].val = i*2;
		metrics_table->metric[i].vf_mask = i*3;
	}

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(metrics_table));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(metrics_table));
#endif

	metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t) * size);

	WhenCalling(std::bind(amdsmi_get_gpu_metrics, &GPU_MOCK_HANDLE,
			      &size, metrics));
	ExpectCommand(SMI_CMD_CODE_GET_METRICS_TABLE);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	free(metrics);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}
