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

#ifndef __AMDSMI_DRIVER_FAKE_HPP__
#define __AMDSMI_DRIVER_FAKE_HPP__

#include <cstring>
#include <common/smi_cmd.h>

#include "gmock/gmock.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

extern "C" {
#include "common/smi_handle.h"
#include "smi_defines.h"
}

using namespace testing;

namespace amdsmi
{
ACTION(MallocPasstrough)
{
	return std::malloc(arg0);
}

ACTION(AccessPasstrough)
{
#ifdef _WIN64
	return _access(arg0, arg1);
#else
	return access(arg0, arg1);
#endif
}

ACTION(CallocPasstrough)
{
	return std::calloc(arg0, arg1);
}


ACTION(AlignedAllocPasstrough)
{
#ifdef _WIN64
	return _aligned_malloc(arg2, arg1);
#elif !defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_ADDRESS__)
	return aligned_alloc(arg1, arg2);
#else
	return posix_memalign(arg0, arg1, arg2) == 0 ? *arg0 : NULL;
#endif
}

ACTION(AlignedFreePasstrough)
{
#ifdef _WIN64
	return _aligned_free(arg0);
#else
	return free(arg0);
#endif
}

ACTION(OpenPasstrough)
{
#ifdef _WIN64
	int fd;
	errno_t err = _sopen_s(&fd, arg0, arg1, _SH_DENYNO, static_cast<unsigned int>(arg2));
	return fd;
#else
	return open(arg0, arg1, arg2);
#endif
}

ACTION(IoctlPasstrough)
{
	long int ret = 0;
#ifdef _WIN64
	ret = _write(arg0, arg1, static_cast<unsigned int>(arg2));
#else
	write(arg0, arg1, static_cast<unsigned int>(arg2));
#endif
	if (ret < 0) {
		return ret;
	}
#ifdef _WIN64
	ret = _read(arg0, arg1, static_cast<unsigned int>(arg2));
#else
	ret = read(arg0, arg1, static_cast<unsigned int>(arg2));
#endif
	return ret;
}

ACTION(StrncpyPasstrough)
{
	size_t count = arg3;
	size_t destsz = arg1;
	if (arg3 >= arg1) {
		count = destsz - 1;
	}
#ifdef _WIN64
	strncpy_s(arg0, arg1, arg2, count);
#else
	strncpy(arg0, arg2, count);

#endif
	arg0[count] = '\0';
	return 0;
}

ACTION(FreePasstrough)
{
	free(arg0);
}

ACTION_P(SaveInputHeader, in_hdr)
{
	std::memcpy(in_hdr, &arg0->in_hdr, sizeof(*in_hdr));
}

ACTION_P(SaveInputPayload, payload)
{
	std::memcpy(payload, arg0->payload, sizeof(*payload));
}

ACTION_P2(SaveInputPayloadWithSize, payload_ptr, payload_size)
{
	std::memcpy(payload_ptr, arg0->payload, payload_size);
}

ACTION_P(SetResponse, payload)
{
	std::memcpy(arg0, &payload, sizeof(payload));
	return 0; // IOCTL successful
}

ACTION_P(SetResponseStatus, outStatus)
{
	smi_ioctl_cmd temp = {
		{},	       // in header
		{ (int) outStatus }, // out header
		{}	       // payload
	};
	std::memcpy(arg0, &temp, sizeof(temp));
	return 0; // IOCTL successful
}

ACTION_P(SetPayload, payload)
{
	arg0->out_hdr.status = AMDSMI_STATUS_SUCCESS;
	std::memcpy(arg0->payload, &payload, sizeof(payload));
}

ACTION_P2(SetPayloadWithSize, payload_ptr, size)
{
	arg0->out_hdr.status = AMDSMI_STATUS_SUCCESS;
	std::memcpy(arg0->payload, payload_ptr, size);
}

MATCHER_P(VerifyGpuHandle, gpu_handle, "Matches the gpu dev handle")
{
	return ((struct smi_device_info *)&arg->payload)->dev_id.handle == gpu_handle;
}

MATCHER_P(SmiCmd, cmd, "Matches the smi cmd")
{
	return arg->in_hdr.code == cmd;
}

class SystemMock {
public:
	SystemMock()
	{
#ifdef _WIN64
		void *ret = (void *)0;
#else
		int ret = 0;
#endif
		ON_CALL(*this, Ioctl(testing::_)).WillByDefault(Return(-1));
		ON_CALL(*this, Ioctl(testing::_, testing::_, testing::_)).WillByDefault(IoctlPasstrough());
		ON_CALL(*this, Open(testing::_)).WillByDefault(Return(ret));
		ON_CALL(*this, Open(testing::_, testing::_, testing::_)).WillByDefault(OpenPasstrough());
		ON_CALL(*this, Access()).WillByDefault(Return(0));
		ON_CALL(*this, Access(testing::_, testing::_)).WillByDefault(AccessPasstrough());
		ON_CALL(*this, Close(testing::_)).WillByDefault(Return(0));
		ON_CALL(*this, Malloc(testing::_)).WillByDefault(MallocPasstrough());
		ON_CALL(*this, Calloc(testing::_, testing::_)).WillByDefault(CallocPasstrough());
		ON_CALL(*this, AlignedAlloc(testing::_, testing::_, testing::_)).WillByDefault(AlignedAllocPasstrough());
		ON_CALL(*this, Free(testing::_)).WillByDefault(FreePasstrough());
		ON_CALL(*this, AlignedFree(testing::_)).WillByDefault(AlignedFreePasstrough());
		ON_CALL(*this, GetDriverMode()).WillByDefault(Return(0));
		ON_CALL(*this, Strncpy(testing::_, testing::_,testing::_, testing::_)).WillByDefault(StrncpyPasstrough());
	}

	MOCK_METHOD1(Ioctl, int(smi_ioctl_cmd *));
	MOCK_METHOD3(Ioctl, long int(int, void *, size_t));
	MOCK_METHOD1(Open, smi_file_handle(enum smi_file_access_mode));
	MOCK_METHOD3(Open, int(const char *, int, int));
	MOCK_METHOD0(Access, int(void));
	MOCK_METHOD2(Access, int(const char *, int));
	MOCK_METHOD1(Close, int(smi_file_handle));
	MOCK_METHOD3(Poll, int(struct smi_event_set_s *, amdsmi_event_entry_t *, int64_t));
	MOCK_METHOD2(PollAlloc, void *(smi_event_handle_t *, int));
	MOCK_METHOD1(Malloc, void *(size_t));
	MOCK_METHOD2(Calloc, void *(size_t, size_t));
	MOCK_METHOD3(AlignedAlloc, void *(void**, size_t, size_t));
	MOCK_METHOD1(Free, void(void *));
	MOCK_METHOD1(AlignedFree, void(void *));
	MOCK_METHOD0(GetDriverMode, int(void));
	MOCK_METHOD4(Strncpy, int(char *, size_t, const char *, size_t));

	virtual ~SystemMock()
	{
	}
};

SystemMock *GetSystemMock();

} // namespace amdsmi

#endif // __AMDSMI_DRIVER_FAKE_HPP__
