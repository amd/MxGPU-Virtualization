/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "gtest/gtest.h"

extern "C" {
#include "amdsmi.h"
#include "common/smi_cmd.h"
#include "smi_processor_handle.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::equal_handles;
using amdsmi::g_system_mock;
using amdsmi::SetResponseStatus;

class AmdSmiMetricsTest : public amdsmi::AmdSmiTest {
protected:
	struct smi_gpu_handle GPU_MOCK_HANDLE_DIFF = {
		SMI_HANDLE_TYPE_AMD_GPU,
		{ { 0x4, 0x3, 0x2, 0x2 } },
		(0x1234ULL << 32) | 0x4321,
		0x8765
	};
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
	uint32_t size = 300;

	ASSERT_EQ(amdsmi_get_gpu_metrics(&GPU_MOCK_HANDLE, NULL, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_metrics(&GPU_MOCK_HANDLE, NULL, &metrics), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_gpu_metrics(&NIC_MOCK_HANDLE, &size, &metrics), AMDSMI_STATUS_INVAL);
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
		metrics_table->metric[i].res_instance = i*4;
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

TEST_F(AmdSmiMetricsTest, SysconfFailed)
{
	int ret;
	amdsmi_metric_t metrics;
	uint32_t size = 7;

#ifndef _WIN64
	// Mock sysconf to return -1
	EXPECT_CALL(*g_system_mock, Sysconf(testing::_))
		.WillOnce(testing::Return(-1));
#endif
	ret = amdsmi_get_gpu_metrics(&GPU_MOCK_HANDLE, &size, &metrics);
#ifndef _WIN64
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
#else
	ASSERT_NE(ret, AMDSMI_STATUS_SUCCESS);
#endif
}

TEST_F(AmdSmiMetricsTest, GetChipletMetricsWrongSize)
{
	int ret;
	amdsmi_metric_t *metrics;
	uint32_t size = 6;
	struct smi_metrics_table in_payload{};
	struct smi_metrics *metrics_table;

#ifdef _WIN64
	metrics_table = (struct smi_metrics*)calloc(1, sizeof(struct smi_metrics));
#else
	metrics_table = (struct smi_metrics*)amdsmi::mem_aligned_alloc((void**)&metrics_table, 4096, sizeof(struct smi_metrics));
#endif

#ifdef _WIN64
	EXPECT_CALL(*g_system_mock, Calloc(testing::_, testing::_)).WillOnce(testing::Return(metrics_table));
#else
	EXPECT_CALL(*g_system_mock, AlignedAlloc(testing::_, testing::_, testing::_)).WillOnce(testing::Return(metrics_table));
#endif

	// Simulate the driver populating num_metric=7 during the ioctl,
	// after the library zeroes the buffer with memset. Use Invoke so the
	// payload's metrics pointer set by the caller is not clobbered, while
	// still capturing the input payload and verifying the ioctl header.
	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_METRICS_TABLE)))
		.WillOnce(testing::Invoke([metrics_table, &in_payload](smi_ioctl_cmd *cmd) -> int {
			std::memcpy(&in_payload, cmd->payload, sizeof(in_payload));
			EXPECT_EQ(cmd->in_hdr.code, (uint32_t)SMI_CMD_CODE_GET_METRICS_TABLE);
			EXPECT_EQ(cmd->in_hdr.in_len, sizeof(struct smi_metrics_table));
			metrics_table->num_metric = 7;
			for (unsigned int i = 0; i < metrics_table->num_metric; i++) {
				metrics_table->metric[i].metric_union.code = i;
				metrics_table->metric[i].val = i * 2;
				metrics_table->metric[i].vf_mask = i * 3;
			}
			cmd->out_hdr.status = AMDSMI_STATUS_SUCCESS;
			return 0;
		}));

	metrics = (amdsmi_metric_t *)malloc(sizeof(amdsmi_metric_t) * size);

	ret = amdsmi_get_gpu_metrics(&GPU_MOCK_HANDLE, &size, metrics);

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
		metrics_table->metric[i].res_instance = i*4;
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
