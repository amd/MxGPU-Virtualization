/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "gtest/gtest.h"

extern "C" {
#include "amdsmi.h"
#include "common/smi_cmd.h"
#include "smi_sys_wrapper.h"
#include "smi_processor_handle.h"
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
	amdsmi_event_set set;
	amdsmi_processor_handle* processor_list = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*16);
	amdsmi_event_entry_t event;
	uint32_t num_devices = AMDSMI_MAX_DEVICES;

	ret = amdsmi_event_create(NULL, (uint8_t)16, (uint64_t)0xC0FFEE, &empty_set);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	ret = amdsmi_event_create(&processor_list[0], (uint8_t)0, (uint64_t)0xC0FFEE, &empty_set);
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);

	processor_list[0] = &NIC_MOCK_HANDLE;
	ret = amdsmi_event_create(&processor_list[0], num_devices, (uint64_t)0xC0FFEE, &set);
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

	sys_wrapper->smi_free(((struct smi_event_set_s*)set)->handles);
	sys_wrapper->smi_free(((struct smi_event_set_s*)set)->devices);
	sys_wrapper->smi_free((struct smi_event_set_s*)set);
	free(processor_list);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiEventsTests, EventCreateMallocOutOfResources)
{
	int ret;
	amdsmi_event_set set;
	amdsmi_processor_handle* processor_list = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle)*16);
	processor_list[0] = &GPU_MOCK_HANDLE;
		system_wrapper *sys_wrapper = get_system_wrapper();
	struct smi_event_set_s *handle_s =
		(struct smi_event_set_s *)sys_wrapper->smi_malloc(sizeof(struct smi_event_set_s));
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
		(struct smi_event_set_s *)sys_wrapper->smi_malloc(sizeof(struct smi_event_set_s));
	handle_s->handles = (smi_event_handle_t*)sys_wrapper->smi_malloc(sizeof(smi_event_handle_t));
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
	struct smi_event_set_s set_s = {};
	amdsmi_event_set set = &set_s;
	amdsmi_event_entry_t event;
	struct smi_event_read_request in_payload;
	struct smi_event_entry mocked_resp = {};

	set_s.num_handles = 1;
	set_s.devices = (smi_device_handle_t*)malloc(sizeof(smi_device_handle_t));
	set_s.devices[0].handle = 0;
	set_s.signaled_device_index = 0;

	EXPECT_CALL(*g_system_mock, Ioctl(amdsmi::SmiCmd(SMI_CMD_CODE_READ_EVENT)))
		.WillOnce(amdsmi::SetResponseStatus(AMDSMI_STATUS_TIMEOUT));
	ret = amdsmi_event_read(set, 0, &event);
	ASSERT_EQ(ret, AMDSMI_STATUS_TIMEOUT);

	mocked_resp.timestamp = 1;
	mocked_resp.category = 2;
	mocked_resp.subcode = 3;
	mocked_resp.level = 4;
	mocked_resp.data = 5;
	mocked_resp.fcn_id.handle = 6;
	mocked_resp.dev_id = 7;
#ifdef _WIN64
	strcpy_s(mocked_resp.date, AMDSMI_MAX_STRING_LENGTH, "random date");
	strcpy_s(mocked_resp.message, AMDSMI_MAX_STRING_LENGTH, "random message");
#else
	strcpy(mocked_resp.date, "random date");
	strcpy(mocked_resp.message, "random message");
#endif

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
		(struct smi_event_set_s *)sys_wrapper->smi_malloc(sizeof(struct smi_event_set_s));
	struct smi_device_info in_payload;

	handle_s->num_handles = 1;
	handle_s->handles = (smi_event_handle_t*)sys_wrapper->smi_malloc(sizeof(smi_event_handle_t));
	handle_s->devices = (smi_device_handle_t*)sys_wrapper->smi_malloc(sizeof(smi_device_handle_t));
	handle_s->handles[0].fd = 0;
	handle_s->signaled_device_index = 0;
	handle_s->_private = sys_wrapper->smi_malloc(4);

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
		(struct smi_event_set_s *)sys_wrapper->smi_malloc(sizeof(struct smi_event_set_s));

	handle_s->num_handles = 1;
	handle_s->handles = (smi_event_handle_t*)sys_wrapper->smi_malloc(sizeof(smi_event_handle_t));
	handle_s->devices = (smi_device_handle_t*)sys_wrapper->smi_malloc(sizeof(smi_device_handle_t));
	handle_s->handles[0].fd = 0;
	handle_s->_private = sys_wrapper->smi_malloc(4);
	ret = amdsmi_event_destroy(NULL);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
	ret = amdsmi_event_destroy(handle_s);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}
#endif
