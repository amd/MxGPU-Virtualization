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

class AmdSmiXgmiTest : public amdsmi::AmdSmiTest {
protected:
	smi_device_handle_t GPU_MOCK_HANDLE_DIFF = { (0x1234ULL << 32) | 0x4321 };
	::testing::AssertionResult equal_link_metrics(smi_link_metrics expect,
						      amdsmi_link_metrics_t actual)
	{
		SMI_ASSERT_EQ(expect.num_links, actual.num_links);
		for (uint32_t i = 0; i < expect.num_links; i++) {
			SMI_ASSERT_EQ(expect.links[i].bdf.as_uint, actual.links[i].bdf.as_uint)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.links[i].bit_rate, actual.links[i].bit_rate)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.links[i].max_bandwidth, actual.links[i].max_bandwidth)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.links[i].link_type, (enum smi_link_type)actual.links[i].link_type)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.links[i].read, actual.links[i].read)
				<< " for i = " << i;
			SMI_ASSERT_EQ(expect.links[i].write, actual.links[i].write)
				<< " for i = " << i;
		}

		return ::testing::AssertionSuccess();
	}
	::testing::AssertionResult equal_link_topology(smi_link_topology expect,
						      amdsmi_link_topology_t actual)
	{
		SMI_ASSERT_EQ(expect.weight, actual.weight);
		SMI_ASSERT_EQ(expect.link_status, (enum smi_link_status)actual.link_status);
		SMI_ASSERT_EQ(expect.link_type, (enum smi_link_type)actual.link_type);
		SMI_ASSERT_EQ(expect.num_hops, actual.num_hops);
		SMI_ASSERT_EQ(expect.fb_sharing, actual.fb_sharing);

		return ::testing::AssertionSuccess();
	}
};

TEST_F(AmdSmiXgmiTest, IoctlFailed)
{
	int ret;
	amdsmi_link_metrics_t link_metrics;
	amdsmi_link_topology_t topology_info;
	amdsmi_xgmi_fb_sharing_caps_t caps;
	uint8_t fb_sharing;
	uint32_t num_processors = 2;
	amdsmi_processor_handle* processor_list =
		(amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*num_processors);
	processor_list[0] = &GPU_MOCK_HANDLE;
	processor_list[1] = &GPU_MOCK_HANDLE;

	EXPECT_CALL(*g_system_mock, Ioctl(_))
		.WillRepeatedly(SetResponseStatus(AMDSMI_STATUS_API_FAILED));

	ret = amdsmi_get_link_metrics(&GPU_MOCK_HANDLE, &link_metrics);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_link_topology(&GPU_MOCK_HANDLE, &GPU_MOCK_HANDLE_DIFF, &topology_info);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_xgmi_fb_sharing_caps(&GPU_MOCK_HANDLE, &caps);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_get_xgmi_fb_sharing_mode_info(&GPU_MOCK_HANDLE, &GPU_MOCK_HANDLE_DIFF, AMDSMI_XGMI_FB_SHARING_MODE_4, &fb_sharing);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	ret = amdsmi_set_xgmi_fb_sharing_mode(&GPU_MOCK_HANDLE, AMDSMI_XGMI_FB_SHARING_MODE_4);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
	free(processor_list);
}

TEST_F(AmdSmiXgmiTest, InvalidParams)
{
	amdsmi_link_topology_t topology_info;
	uint8_t fb_sharing;
	uint32_t num_processors = 0;

	ASSERT_EQ(amdsmi_get_link_metrics(&GPU_MOCK_HANDLE, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_caps(&GPU_MOCK_HANDLE, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_link_topology(&GPU_MOCK_HANDLE, NULL, &topology_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_link_topology(NULL, &GPU_MOCK_HANDLE, &topology_info), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_link_topology(&GPU_MOCK_HANDLE, &GPU_MOCK_HANDLE, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(&GPU_MOCK_HANDLE, &GPU_MOCK_HANDLE, AMDSMI_XGMI_FB_SHARING_MODE_4, NULL), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(NULL, &GPU_MOCK_HANDLE, AMDSMI_XGMI_FB_SHARING_MODE_4, &fb_sharing), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_get_xgmi_fb_sharing_mode_info(&GPU_MOCK_HANDLE, NULL, AMDSMI_XGMI_FB_SHARING_MODE_4, &fb_sharing), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_xgmi_fb_sharing_mode(NULL, AMDSMI_XGMI_FB_SHARING_MODE_4), AMDSMI_STATUS_INVAL);
	ASSERT_EQ(amdsmi_set_xgmi_fb_sharing_mode_v2(NULL, num_processors, AMDSMI_XGMI_FB_SHARING_MODE_4), AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiXgmiTest, GetLinkMetrics)
{
	int ret;
	amdsmi_link_metrics_t link_metrics;
	smi_device_info in_payload;
	struct smi_link_metrics mocked_resp = {};
	amdsmi_processor_handle MOCK_GPU_HANDLE = &GPU_MOCK_HANDLE;

	mocked_resp.num_links = 7;

	for (uint32_t i = 0; i < mocked_resp.num_links; i++) {
		mocked_resp.links[i].bdf.as_uint = 1234567;
		mocked_resp.links[i].bit_rate = 1*i;
		mocked_resp.links[i].max_bandwidth = 2*i;
		mocked_resp.links[i].link_type = SMI_LINK_TYPE_XGMI;
		mocked_resp.links[i].read = i*3;
		mocked_resp.links[i].write = i*4;
	}

	WhenCalling(std::bind(amdsmi_get_link_metrics, MOCK_GPU_HANDLE,
			      &link_metrics));
	ExpectCommand(SMI_CMD_CODE_GET_LINK_METRICS);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_link_metrics(mocked_resp, link_metrics));
}

TEST_F(AmdSmiXgmiTest, GetLinkTopology)
{
	int ret;
	amdsmi_link_topology_t link_topology;
	struct smi_device_pair_info in_payload;
	struct smi_link_topology mocked_resp = {};

	mocked_resp.weight = 300;
	mocked_resp.link_status = SMI_LINK_STATUS_ENABLED;
	mocked_resp.link_type = SMI_LINK_TYPE_XGMI;
	mocked_resp.num_hops = 7;
	mocked_resp.fb_sharing = 1;

	WhenCalling(std::bind(amdsmi_get_link_topology, &GPU_MOCK_HANDLE, &GPU_MOCK_HANDLE_DIFF,
			      &link_topology));
	ExpectCommand(SMI_CMD_CODE_GET_LINK_TOPOLOGY);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.src.dev_id, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_handles(in_payload.dst.dev_id, GPU_MOCK_HANDLE_DIFF));
	ASSERT_TRUE(equal_link_topology(mocked_resp, link_topology));
}

TEST_F(AmdSmiXgmiTest, GetLinkTopologyNotSupported)
{
	int ret;
	amdsmi_link_topology_t link_topology;
	struct smi_device_pair_info in_payload;
	struct smi_link_topology mocked_resp = {};

	mocked_resp.weight = 300;
	mocked_resp.link_status = SMI_LINK_STATUS_ENABLED;
	mocked_resp.link_type = SMI_LINK_TYPE_XGMI;
	mocked_resp.num_hops = 7;
	mocked_resp.fb_sharing = 1;

	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_GET_LINK_TOPOLOGY)))
		.WillOnce(amdsmi::SetResponseStatus(AMDSMI_STATUS_NOT_SUPPORTED));

	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = amdsmi_get_link_topology(&GPU_MOCK_HANDLE, &GPU_MOCK_HANDLE_DIFF,
			      &link_topology);

	ASSERT_EQ(ret, AMDSMI_STATUS_NOT_SUPPORTED);
}

TEST_F(AmdSmiXgmiTest, GetLinkTopologyGpuItself)
{
	int ret;
	amdsmi_link_topology_t link_topology;

	ret = amdsmi_get_link_topology(&GPU_MOCK_HANDLE, &GPU_MOCK_HANDLE,
			      &link_topology);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(link_topology.weight, 0);
	ASSERT_EQ(link_topology.link_status, AMDSMI_LINK_STATUS_ENABLED);
	ASSERT_EQ(link_topology.link_type, AMDSMI_LINK_TYPE_NOT_APPLICABLE);
	ASSERT_EQ(link_topology.num_hops, 0);
	ASSERT_EQ(link_topology.fb_sharing, 1);
}

TEST_F(AmdSmiXgmiTest, GetFbSharingCaps)
{
	int ret;
	amdsmi_xgmi_fb_sharing_caps_t caps;
	struct smi_device_info in_payload;
	union smi_xgmi_fb_sharing_caps mocked_resp = {};

	mocked_resp.cap.mode_custom_cap = 0;
	mocked_resp.cap.mode_1_cap = 1;
	mocked_resp.cap.mode_2_cap = 1;
	mocked_resp.cap.mode_4_cap = 1;
	mocked_resp.cap.mode_8_cap = 1;

	WhenCalling(std::bind(amdsmi_get_xgmi_fb_sharing_caps, &GPU_MOCK_HANDLE,
			      &caps));
	ExpectCommand(SMI_CMD_CODE_GET_XGMI_FB_SHARING_CAPS);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
	ASSERT_EQ(mocked_resp.xgmi_fb_sharing_cap_mask, caps.xgmi_fb_sharing_cap_mask);
}

TEST_F(AmdSmiXgmiTest, GetFbSharingModeInfo)
{
	int ret;
	uint8_t fb_sharing;
	struct smi_xgmi_fb_sharing in_payload;
	struct smi_xgmi_fb_sharing_flag mocked_resp = {};


	mocked_resp.fb_sharing = 1;

	WhenCalling(std::bind(amdsmi_get_xgmi_fb_sharing_mode_info, &GPU_MOCK_HANDLE, &GPU_MOCK_HANDLE_DIFF,
			      AMDSMI_XGMI_FB_SHARING_MODE_4, &fb_sharing));
	ExpectCommand(SMI_CMD_CODE_GET_XGMI_FB_SHARING_MODE_INFO);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.src_dev, GPU_MOCK_HANDLE));
	ASSERT_TRUE(equal_handles(in_payload.dst_dev, GPU_MOCK_HANDLE_DIFF));
	ASSERT_EQ(mocked_resp.fb_sharing, fb_sharing);
}

TEST_F(AmdSmiXgmiTest, GetFbSharingModeInfoItself)
{
	int ret;
	uint8_t fb_sharing;

	ret = amdsmi_get_xgmi_fb_sharing_mode_info(&GPU_MOCK_HANDLE, &GPU_MOCK_HANDLE,
			      AMDSMI_XGMI_FB_SHARING_MODE_4, &fb_sharing);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_EQ(fb_sharing, 1);
}

TEST_F(AmdSmiXgmiTest, SetFbSharingMode)
{
	int ret;
	amdsmi_xgmi_fb_sharing_mode_t mode = AMDSMI_XGMI_FB_SHARING_MODE_4;
	struct smi_set_xgmi_fb_sharing_mode in_payload;

	WhenCalling(std::bind(amdsmi_set_xgmi_fb_sharing_mode, &GPU_MOCK_HANDLE, mode));
	ExpectCommand(SMI_CMD_CODE_SET_XGMI_FB_SHARING_MODE);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ASSERT_TRUE(equal_handles(in_payload.dev_id, GPU_MOCK_HANDLE));
}

TEST_F(AmdSmiXgmiTest, SetFbCustomSharingMode)
{
	int ret;
	amdsmi_xgmi_fb_sharing_mode_t mode = AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM;
	struct smi_set_xgmi_fb_custom_sharing_mode in_payload;
	uint32_t num_processors = 2;
	amdsmi_processor_handle* processor_list =
		(amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*num_processors);
	processor_list[0] = &GPU_MOCK_HANDLE;
	processor_list[1] = &GPU_MOCK_HANDLE_DIFF;

	WhenCalling(std::bind(amdsmi_set_xgmi_fb_sharing_mode_v2, processor_list, num_processors, mode));
	ExpectCommand(SMI_CMD_CODE_SET_XGMI_FB_SHARING_MODE_VER_2);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();

	free(processor_list);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiXgmiTest, SetFbCustomSharingModeTwoIdenticalGpusFailed)
{
	int ret;
	amdsmi_xgmi_fb_sharing_mode_t mode = AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM;
	uint32_t num_processors = 2;
	amdsmi_processor_handle* processor_list =
		(amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*num_processors);
	processor_list[0] = &GPU_MOCK_HANDLE;
	processor_list[1] = &GPU_MOCK_HANDLE;

	ret = amdsmi_set_xgmi_fb_sharing_mode_v2(processor_list, num_processors, mode);
	free(processor_list);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}
