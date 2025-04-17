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
#include "smi_sys_wrapper.h"
}

#include "smi_system_mock.hpp"
#include "smi_test_helpers.hpp"

using amdsmi::g_system_mock;

class AmdSmiEventsTests : public amdsmi::AmdSmiTest {
};

TEST_F(AmdSmiEventsTests, InvalidParams)
{
	int ret;
	amdsmi_event_set empty_set = NULL;
	amdsmi_event_set set = &empty_set;
	amdsmi_processor_handle* processor_list = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*16);
	amdsmi_event_entry_t event;

	ret = amdsmi_event_create(NULL, (uint8_t)16, (uint64_t)0xC0FFEE, &empty_set);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_event_create(&processor_list[0], (uint8_t)0, (uint64_t)0xC0FFEE, &empty_set);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_event_read(NULL, 0, &event);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_event_read(set, 0, NULL);
	free(processor_list);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}

TEST_F(AmdSmiEventsTests, EventCreate)
{
	int ret;
	amdsmi_event_set set;
	uint8_t num_devices = 1;
	amdsmi_processor_handle* processor_list = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*16);
	processor_list[0] = &GPU_MOCK_HANDLE;
	system_wrapper *sys_wrapper = get_system_wrapper();

	smi_event_set_config in_payload;

	smi_event_handle_t mocked_resp = {};

	WhenCalling(std::bind(amdsmi_event_create, &processor_list[0], num_devices, (uint64_t)0xC0FFEE, &set));
	ExpectCommand(SMI_CMD_CODE_CREATE_EVENT);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);

	sys_wrapper->free(((struct smi_event_set_s*)set)->handles);
	sys_wrapper->free(((struct smi_event_set_s*)set)->devices);
	sys_wrapper->free((struct smi_event_set_s*)set);

	WhenCalling(std::bind(amdsmi_event_create, &processor_list[0], num_devices, (uint64_t)0xC0FFEE, &set));
	ExpectCommand(SMI_CMD_CODE_CREATE_EVENT);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall(AMDSMI_STATUS_UNKNOWN_ERROR);

	free(processor_list);
	ASSERT_EQ(ret, AMDSMI_STATUS_UNKNOWN_ERROR);
}

TEST_F(AmdSmiEventsTests, EventCreateMallocOutOfResources)
{
	int ret;
	amdsmi_event_set set;
	amdsmi_processor_handle* processor_list = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*16);
	processor_list[0] = &GPU_MOCK_HANDLE;
		system_wrapper *sys_wrapper = get_system_wrapper();
	struct smi_event_set_s *handle_s =
		(struct smi_event_set_s *)sys_wrapper->malloc(sizeof(struct smi_event_set_s));
	EXPECT_CALL(*g_system_mock, Malloc(testing::_)).WillOnce(testing::Return(handle_s))
												   .WillOnce(testing::Return(nullptr));

	ret = amdsmi_event_create(&processor_list[0], 1, 0xB16B00B5, &set);
	free(processor_list);
	ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
}

TEST_F(AmdSmiEventsTests, EventCreateMallocFailure)
{
	int ret;
	amdsmi_event_set set;
	amdsmi_processor_handle* processor_list = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*16);
	processor_list[0] = &GPU_MOCK_HANDLE;
		system_wrapper *sys_wrapper = get_system_wrapper();
	struct smi_event_set_s *handle_s =
		(struct smi_event_set_s *)sys_wrapper->malloc(sizeof(struct smi_event_set_s));
	handle_s->handles = (smi_event_handle_t*)sys_wrapper->malloc(sizeof(smi_event_handle_t));
	EXPECT_CALL(*g_system_mock, Malloc(testing::_)).WillOnce(testing::Return(handle_s))
												   .WillOnce(testing::Return(handle_s->handles))
												   .WillOnce(testing::Return(nullptr));
	ret = amdsmi_event_create(&processor_list[0], 1, 0xB16B00B5, &set);
	free(processor_list);
	ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
}


TEST_F(AmdSmiEventsTests, EventCreateMallocNullPtr)
{
	int ret;
	amdsmi_event_set set;
	uint8_t num_devices = 1;

	amdsmi_processor_handle* processor_list = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*16);
	processor_list[0] = &GPU_MOCK_HANDLE;

	EXPECT_CALL(*g_system_mock, Malloc(testing::_)).WillOnce(testing::Return(nullptr));
	ret = amdsmi_event_create(&processor_list[0], num_devices, (uint64_t)0xC0FFEE, &set);
	free(processor_list);
	ASSERT_EQ(ret, AMDSMI_STATUS_OUT_OF_RESOURCES);
}

#ifdef _WIN64
TEST_F(AmdSmiEventsTests, EventRead)
{
	int ret;
	struct smi_event_set_s set_s;
	amdsmi_event_set set = &set_s;
	amdsmi_event_entry_t event;
	struct smi_device_info in_payload;
	struct smi_event_entry mocked_resp = {};

	EXPECT_CALL(*g_system_mock, Poll(_, _, _)).WillOnce(Return(AMDSMI_STATUS_TIMEOUT));
	ret = amdsmi_event_read(set, 0, &event);
	ASSERT_EQ(ret, AMDSMI_STATUS_TIMEOUT);

	set_s.num_handles = 1;
	set_s.devices = (smi_device_handle_t*)malloc(sizeof(smi_device_handle_t));
	set_s.signaled_device_index = 0;

	mocked_resp.timestamp = 1;
	mocked_resp.category = 2;
	mocked_resp.subcode = 3;
	mocked_resp.level = 4;
	mocked_resp.data = 5;
	mocked_resp.fcn_id.handle = 6;
	mocked_resp.dev_id = 7;
#ifdef _WIN64
	strcpy_s(mocked_resp.date, AMDSMI_MAX_DATE_LENGTH, "random date");
	strcpy_s(mocked_resp.message, AMDSMI_EVENT_MSG_SIZE, "random message");
#else
	strcpy(mocked_resp.date, "random date");
	strcpy(mocked_resp.message, "random message");
#endif

	EXPECT_CALL(*g_system_mock, Poll(_, _, _)).WillOnce(Return(AMDSMI_STATUS_SUCCESS));

	WhenCalling(std::bind(amdsmi_event_read, set, 0, &event));
	ExpectCommand(SMI_CMD_CODE_READ_EVENT);
	SaveInputPayloadIn(&in_payload);
	PlantMockOutput(&mocked_resp);
	ret = performCall();
	free(set_s.devices);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiEventsTests, EventDestroy)
{
	int ret;
	system_wrapper *sys_wrapper = get_system_wrapper();
	struct smi_event_set_s *handle_s =
		(struct smi_event_set_s *)sys_wrapper->malloc(sizeof(struct smi_event_set_s));
	struct smi_device_info in_payload;

	handle_s->num_handles = 1;
	handle_s->handles = (smi_event_handle_t*)sys_wrapper->malloc(sizeof(smi_event_handle_t));
	handle_s->devices = (smi_device_handle_t*)sys_wrapper->malloc(sizeof(smi_device_handle_t));
	handle_s->handles[0].fd = 0;
	handle_s->signaled_device_index = 0;
	handle_s->_private = sys_wrapper->malloc(4);

	ret = amdsmi_event_destroy(NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	WhenCalling(std::bind(amdsmi_event_destroy, handle_s));
	ExpectCommand(SMI_CMD_CODE_DESTROY_EVENT);
	SaveInputPayloadIn(&in_payload);
	ret = performCall();
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}
#else
TEST_F(AmdSmiEventsTests, EventRead)
{
	int ret;
	struct smi_event_set_s set_s;
	amdsmi_event_set set = &set_s;
	amdsmi_event_entry_t event;

	EXPECT_CALL(*g_system_mock, Poll(_, _, _)).WillOnce(Return(AMDSMI_STATUS_TIMEOUT));

	ret = amdsmi_event_read(set, 0, &event);
	ASSERT_EQ(ret, AMDSMI_STATUS_TIMEOUT);

	EXPECT_CALL(*g_system_mock, Poll(_, _, _)).WillOnce(Return(AMDSMI_STATUS_SUCCESS));

	ret = amdsmi_event_read(set, 0, &event);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);

	EXPECT_CALL(*g_system_mock, Poll(_, _, _)).WillOnce(Return(AMDSMI_STATUS_UNKNOWN_ERROR));

	ret = amdsmi_event_read(set, 0, &event);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiEventsTests, EventDestroy)
{
	int ret;
	system_wrapper *sys_wrapper = get_system_wrapper();
	struct smi_event_set_s *handle_s =
		(struct smi_event_set_s *)sys_wrapper->malloc(sizeof(struct smi_event_set_s));

	handle_s->num_handles = 1;
	handle_s->handles = (smi_event_handle_t*)sys_wrapper->malloc(sizeof(smi_event_handle_t));
	handle_s->devices = (smi_device_handle_t*)sys_wrapper->malloc(sizeof(smi_device_handle_t));
	handle_s->handles[0].fd = 0;
	handle_s->_private = sys_wrapper->malloc(4);
	ret = amdsmi_event_destroy(NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ret = amdsmi_event_destroy(handle_s);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}
#endif
