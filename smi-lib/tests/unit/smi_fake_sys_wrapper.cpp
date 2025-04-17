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

#include <memory>

#include "smi_system_mock.hpp"
extern "C" {
#include "smi_sys_wrapper.h"
#include "common/smi_handle.h"
#include "smi_defines.h"
}

namespace amdsmi
{
extern SystemMock *GetSystemMock();

static int ioctl(smi_file_handle fd, smi_ioctl_cmd *ioctl_cmd)
{
	AMDSMI_UNUSED(fd);
	return GetSystemMock()->Ioctl(ioctl_cmd);
}

static smi_file_handle open(enum smi_file_access_mode mode)
{
	return GetSystemMock()->Open(mode);
}

static int access(void)
{
	return GetSystemMock()->Access();
}

static int poll(struct smi_event_set_s *event_set, amdsmi_event_entry_t *event,
			    int64_t timeout)
{
	return GetSystemMock()->Poll(event_set, event, timeout);
}

static void *poll_alloc(smi_event_handle_t *ev, int num)
{
	return GetSystemMock()->PollAlloc(ev, num);
}

static int close(smi_file_handle fd)
{
	return GetSystemMock()->Close(fd);
}

static void *malloc(size_t size)
{
	return GetSystemMock()->Malloc(size);
}

static void *calloc(size_t num, size_t size)
{
	return GetSystemMock()->Calloc(num, size);
}

static void *aligned_alloc(void **mem, size_t alignment, size_t size)
{
	return GetSystemMock()->AlignedAlloc(mem, alignment, size);
}

static void free(void *mem)
{
	return GetSystemMock()->Free(mem);
}

static void aligned_free(void *mem)
{
	return GetSystemMock()->AlignedFree(mem);
}

static int strncpy(char *dest, size_t destsz, const char *src, size_t count)
{
	return GetSystemMock()->Strncpy(dest, destsz, src, count);
}

} // namespace amdsmi

static system_wrapper wrapper = {
	(void *(*)(size_t))amdsmi::malloc,
	(void *(*)(size_t, size_t))amdsmi::calloc,
	(void (*)(void *))amdsmi::free,
	(int (*)(smi_file_handle, smi_ioctl_cmd *))amdsmi::ioctl,
	(smi_file_handle (*)(enum smi_file_access_mode))amdsmi::open,
	(int (*)(void))amdsmi::access,
	(int (*)(smi_file_handle))amdsmi::close,
	(int (*)(struct smi_event_set_s *, amdsmi_event_entry_t *, int64_t))amdsmi::poll,
	(void *(*)(smi_event_handle_t *, uint32_t))amdsmi::poll_alloc,
	NULL,
	(void *(*)(void**, size_t, size_t))amdsmi::aligned_alloc,
	(void (*)(void *))amdsmi::aligned_free,
	(int (*)(char *, size_t, const char *, size_t))amdsmi::strncpy
};

extern "C" {

system_wrapper *get_system_wrapper()
{
	return &wrapper;
}
}
