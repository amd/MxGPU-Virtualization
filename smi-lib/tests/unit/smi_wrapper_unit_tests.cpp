/*
 * Copyright (c) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
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
#include <gmock/gmock.h>

#include "smi_system_mock.hpp"
extern "C" {
#include "smi_sys_wrapper.h"
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <sys/ioctl.h>
}

using namespace ::testing;

namespace amdsmi
{
std::unique_ptr<NiceMock<SystemMock> > g_system_mock;
SystemMock *GetSystemMock()
{
	return g_system_mock.get();
}
} // namespace amdsmi

extern "C" {

extern int __real_poll(struct pollfd *fds, nfds_t nfds, int timeout);
extern ssize_t __real_read(int fd, void *buf, size_t count);
extern int __real_ioctl(int fd, unsigned long request, void* buffer);
extern int __real_open(const char *pathname, int flags, int mode);
extern int __real_access(const char *pathname, int mode);
extern ssize_t __real_write(int fd, void *buf, size_t count);


static const int MAGIC_FD = 0x12345578;
static const char *IOCTL_FILE_NAME = "/dev/gim-smi0";

static int poll_cnt;
static int read_cnt;
static int write_cnt;
static int ioctl_cnt;
static int open_cnt;
static int access_cnt;

static int poll_ret;
static ssize_t read_ret;
static int ioctl_ret;
static int open_ret;
static int access_ret;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"

int __wrap_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	if (nfds > 0 && fds != nullptr && fds->fd == MAGIC_FD) {
		poll_cnt++;
		fds->revents = fds->events;
		return poll_ret;
	}
	return __real_poll(fds, nfds, timeout);
}

ssize_t __wrap_read(int fd, void *buf, size_t count)
{
	if (fd == MAGIC_FD) {
		read_cnt++;
		return read_ret;
	}
	return __real_read(fd, buf, count);
}

int __wrap_ioctl(int fd, unsigned long request, void* buffer)
{
	if (fd == MAGIC_FD) {
		ioctl_cnt++;
		return ioctl_ret;
	}
	return __real_ioctl(fd, request, buffer);
}

ssize_t __wrap_write(int fd, void *buf, size_t count)
{
	if (fd == MAGIC_FD) {
		write_cnt++;
		return write_cnt;
	}
	return __real_write(fd, buf, count);
}

int __wrap_open(const char *pathname, int flags, int mode)
{
	if (strcmp(IOCTL_FILE_NAME, pathname) == 0) {
		open_cnt++;
		return open_ret;
	}
	return __real_open(pathname, flags, mode);
}

int __wrap_access(const char *pathname, int mode)
{
	if (strcmp(IOCTL_FILE_NAME, pathname) == 0) {
		access_cnt++;
		return access_ret;
	}
	return __real_access(pathname, mode);
}


#pragma GCC diagnostic pop
}

class AmdSmiLnxWrapperTests : public Test {
protected:
	void SetUp() override
	{
		amdsmi::g_system_mock.reset(new NiceMock<amdsmi::SystemMock>);
		poll_cnt = 0;
		read_cnt = 0;
		write_cnt = 0;
		ioctl_cnt = 0;
		open_cnt = 0;
		access_cnt = 0;

		poll_ret = 0; // timeout by default
		read_ret = 0; // EOF by default
		write_cnt = 0;
		ioctl_ret = -1; // error by default
		open_ret = -1; // error by default
		access_ret = -1; // error by default
	}

	void TearDown() override
	{
		amdsmi::g_system_mock.reset();
	}
};

TEST_F(AmdSmiLnxWrapperTests, TestDefaultWrapper)
{
	auto wrapper = get_system_wrapper();
	ASSERT_EQ((void*)wrapper->free, (void*)free);
	ASSERT_EQ((void*)wrapper->malloc, (void*)malloc);
}

TEST_F(AmdSmiLnxWrapperTests, TestPollAlloc)
{
	auto wrapper = get_system_wrapper();

	int num_fds = 2;
	smi_event_handle_t event_handle[2];
	event_handle[0].fd = MAGIC_FD;
	event_handle[1].fd = MAGIC_FD;

	void *fds = wrapper->poll_alloc(event_handle, num_fds);

	auto actual_fds = reinterpret_cast<struct pollfd *>(fds);

	for (int i = 0; i < num_fds; i++) {
		ASSERT_EQ(actual_fds[i].fd, MAGIC_FD);
		ASSERT_EQ(actual_fds[i].events, POLLIN);
	}

	free(fds);
}

TEST_F(AmdSmiLnxWrapperTests, TestEventTimeout)
{
	auto wrapper = get_system_wrapper();

	struct pollfd fd = { MAGIC_FD, POLLIN, 0 };
	amdsmi_event_entry_t event;
	int64_t timeout = 10 * 1000; // 10ms
	struct smi_event_set_s event_set;

	event_set.num_handles = 1;
	event_set._private = &fd;

	poll_ret = 0; // timeout

	int ret = wrapper->poll(&event_set, &event, timeout);

	ASSERT_GE(poll_cnt, 1);
	ASSERT_EQ(read_cnt, 0);
	ASSERT_EQ(ret, AMDSMI_STATUS_TIMEOUT);
}

TEST_F(AmdSmiLnxWrapperTests, TestPollFailed)
{
	auto wrapper = get_system_wrapper();

	struct pollfd fd = { MAGIC_FD, POLLIN, 0 };
	amdsmi_event_entry_t event;
	int64_t timeout = 10 * 1000; // 10ms
	struct smi_event_set_s event_set;

	event_set.num_handles = 1;
	event_set._private = &fd;
	poll_ret = -1; // error

	int ret = wrapper->poll(&event_set, &event, timeout);

	ASSERT_GE(poll_cnt, 1);
	ASSERT_EQ(read_cnt, 0);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiLnxWrapperTests, TestPollWaitOne)
{
	auto wrapper = get_system_wrapper();

	struct pollfd fd = { MAGIC_FD, POLLIN, 0 };
	amdsmi_event_entry_t event;
	int64_t timeout = 1;
	struct smi_event_set_s event_set;

	event_set.num_handles = 1;
	event_set._private = &fd;

	poll_ret = 1;
	read_ret = 0; // EOF

	int ret = wrapper->poll(&event_set, &event, timeout);

	ASSERT_GE(poll_cnt, 1);
	ASSERT_GE(read_cnt, 1);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiLnxWrapperTests, TestPollWaitForever)
{
	auto wrapper = get_system_wrapper();

	struct pollfd fd = { MAGIC_FD, POLLIN, 0 };
	amdsmi_event_entry_t event;
	int64_t timeout = -1; // forever
	struct smi_event_set_s event_set;

	event_set.num_handles = 1;
	event_set._private = &fd;

	poll_ret = 1;
	read_ret = 0; // EOF

	int ret = wrapper->poll(&event_set, &event, timeout);

	ASSERT_GE(poll_cnt, 1);
	ASSERT_GE(read_cnt, 1);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiLnxWrapperTests, TestReadSucceeds)
{
	auto wrapper = get_system_wrapper();

	struct pollfd fd = { MAGIC_FD, POLLIN, 0 };
	amdsmi_event_entry_t event;
	int64_t timeout = -1; // forever
	struct smi_event_set_s event_set;

	event_set.num_handles = 1;
	event_set._private = &fd;

	poll_ret = 1;
	read_ret = 1;

	int ret = wrapper->poll(&event_set, &event, timeout);

	ASSERT_GE(poll_cnt, 1);
	ASSERT_GE(read_cnt, 1);
	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiLnxWrapperTests, TestReadFailed)
{
	auto wrapper = get_system_wrapper();

	struct pollfd fd = { MAGIC_FD, POLLIN, 0 };
	amdsmi_event_entry_t event;
	int64_t timeout = -1; // forever
	struct smi_event_set_s event_set;

	event_set.num_handles = 1;
	event_set._private = &fd;

	poll_ret = 1;
	read_ret = -1; // error

	int ret = wrapper->poll(&event_set, &event, timeout);

	ASSERT_GE(poll_cnt, 1);
	ASSERT_GE(read_cnt, 1);
	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiLnxWrapperTests, TestIoctlNull)
{
	EXPECT_CALL(*amdsmi::g_system_mock, GetDriverMode())
		.WillOnce(Return(2));
	smi_ioctl_cmd *buffer = nullptr;

	auto wrapper = get_system_wrapper();

	ioctl_ret = -EINVAL;

	int ret = wrapper->ioctl(MAGIC_FD, buffer);

	ASSERT_EQ(ret, ioctl_ret);
	ASSERT_EQ(read_cnt, 0);
}

TEST_F(AmdSmiLnxWrapperTests, TestIoctlSuccess)
{
	smi_ioctl_cmd *buffer = nullptr;

	auto wrapper = get_system_wrapper();

	ioctl_ret = 0;

	int ret = wrapper->ioctl(MAGIC_FD, buffer);

	ASSERT_EQ(ret, ioctl_ret);
	ASSERT_EQ(read_cnt, 1);
}

TEST_F(AmdSmiLnxWrapperTests, TestIoctlFailed)
{
	EXPECT_CALL(*amdsmi::g_system_mock, Ioctl(testing::_, testing::_, testing::_))
		.WillOnce(Return(-1));
	smi_ioctl_cmd *buffer = nullptr;

	auto wrapper = get_system_wrapper();

	ioctl_ret = -1;

	int ret = wrapper->ioctl(MAGIC_FD, buffer);

	ASSERT_EQ(ret, ioctl_ret);
	ASSERT_EQ(read_cnt, 0);
	ASSERT_EQ(write_cnt, 0);
}

TEST_F(AmdSmiLnxWrapperTests, TestOpenNull)
{
	EXPECT_CALL(*amdsmi::g_system_mock, GetDriverMode())
		.WillOnce(Return(2));
	auto wrapper = get_system_wrapper();

	open_ret = -EINVAL;

	int ret = wrapper->open(SMI_READONLY);

	ASSERT_EQ(open_ret, ret);
	ASSERT_EQ(open_cnt, 0);
}

TEST_F(AmdSmiLnxWrapperTests, TestOpenSuccess)
{
	auto wrapper = get_system_wrapper();

	open_ret = 0;

	int ret = wrapper->open(SMI_READONLY);

	ASSERT_EQ(open_ret, ret);
	ASSERT_EQ(open_cnt, 1);
}


TEST_F(AmdSmiLnxWrapperTests, TestOpenFailed)
{
	auto wrapper = get_system_wrapper();

	open_ret = -1;

	int ret = wrapper->open(SMI_RDWR);

	ASSERT_EQ(open_ret, ret);
	ASSERT_EQ(open_cnt, 1);

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wconversion"
	ret = wrapper->open((smi_file_access_mode)(SMI_RDWR + 5));
	#pragma GCC diagnostic pop
	ASSERT_EQ(ret, AMDSMI_STATUS_INVAL);
}


TEST_F(AmdSmiLnxWrapperTests, TestAccessSuccess)
{
	auto wrapper = get_system_wrapper();

	access_ret = 0;

	int ret = wrapper->access();

	ASSERT_EQ(access_ret, ret);
	ASSERT_EQ(access_cnt, 1);
}


TEST_F(AmdSmiLnxWrapperTests, TestAccessFailed)
{
	auto wrapper = get_system_wrapper();

	access_ret = -1;

	int ret = wrapper->access();

	ASSERT_EQ(access_ret, ret);
	ASSERT_EQ(access_cnt, 1);
}

TEST_F(AmdSmiLnxWrapperTests, TestSysWrapperNull)
{
	EXPECT_CALL(*amdsmi::g_system_mock, GetDriverMode())
		.WillOnce(Return(2));
	auto wrapper = get_system_wrapper();
	access_ret = 0;

	int ret = wrapper->access();

	ASSERT_EQ(access_ret, ret);
}

TEST_F(AmdSmiLnxWrapperTests, TestIsUserModeDriver)
{
	auto wrapper = get_system_wrapper();
	access_ret = 1;

	int ret = wrapper->is_user_mode();

	ASSERT_EQ(access_ret, ret);
}

TEST_F(AmdSmiLnxWrapperTests, TestIsKernelModeDriver)
{
	EXPECT_CALL(*amdsmi::g_system_mock, GetDriverMode())
		.WillOnce(Return(1));
	auto wrapper = get_system_wrapper();
	access_ret = 0;

	int ret = wrapper->is_user_mode();

	ASSERT_EQ(access_ret, ret);
}

TEST_F(AmdSmiLnxWrapperTests, TestIsNullModeDriver)
{
	EXPECT_CALL(*amdsmi::g_system_mock, GetDriverMode())
		.WillOnce(Return(2));
	auto wrapper = get_system_wrapper();
	access_ret = 0;

	int ret = wrapper->is_user_mode();

	ASSERT_EQ(access_ret, ret);
}

TEST_F(AmdSmiLnxWrapperTests, TestCloseNull)
{
	EXPECT_CALL(*amdsmi::g_system_mock, GetDriverMode())
		.WillOnce(Return(2));
	auto wrapper = get_system_wrapper();
	access_ret = 0;

	int ret = wrapper->close(MAGIC_FD);

	ASSERT_EQ(access_ret, ret);
}

TEST_F(AmdSmiLnxWrapperTests, TestCloseSuccess)
{
	EXPECT_CALL(*amdsmi::g_system_mock, Close(testing::_))
		.WillOnce(Return(0));
	auto wrapper = get_system_wrapper();
	access_ret = 0;

	int ret = wrapper->close(MAGIC_FD);

	ASSERT_EQ(access_ret, ret);
}

TEST_F(AmdSmiLnxWrapperTests, TestCloseFailed)
{
	EXPECT_CALL(*amdsmi::g_system_mock, Close(testing::_))
		.WillOnce(Return(1));
	auto wrapper = get_system_wrapper();
	access_ret = 1;

	int ret = wrapper->close(MAGIC_FD);

	ASSERT_EQ(access_ret, ret);
}

TEST_F(AmdSmiLnxWrapperTests, TestAlignedAlloc)
{
	auto wrapper = get_system_wrapper();
	int *mem = (int*)malloc(sizeof(int));
	long page_size = 0;
	void *aligned_mem = NULL;

	*mem = 300;
	page_size = 4096;
	aligned_mem = wrapper->aligned_alloc((void **)mem, (size_t)page_size, page_size*sizeof(int));

	ASSERT_EQ(aligned_mem ? 0:1, 0);
	free(mem);
	free(aligned_mem);
}

TEST_F(AmdSmiLnxWrapperTests, TestStrncpySuccess)
{
	auto wrapper = get_system_wrapper();
	char dest[7];
	size_t destsz = 7;
	const char *src = "random";
	size_t count = 7;
	int ret = 0;

	ret = wrapper->strncpy(dest, destsz, src, count);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}

TEST_F(AmdSmiLnxWrapperTests, TestStrncpyFail)
{
	auto wrapper = get_system_wrapper();
	char dest[7];
	size_t destsz = 0;
	const char *src = "random";
	size_t count = 7;
	int ret = 0;

	ret = wrapper->strncpy(dest, destsz, src, count);

	ASSERT_EQ(ret, AMDSMI_STATUS_API_FAILED);
}

TEST_F(AmdSmiLnxWrapperTests, TestStrncpyCount)
{
	auto wrapper = get_system_wrapper();
	char dest[7];
	size_t destsz = 7;
	const char *src = "random";
	size_t count = destsz + 1;
	int ret = 0;

	ret = wrapper->strncpy(dest, destsz, src, count);

	ASSERT_EQ(ret, AMDSMI_STATUS_SUCCESS);
}
