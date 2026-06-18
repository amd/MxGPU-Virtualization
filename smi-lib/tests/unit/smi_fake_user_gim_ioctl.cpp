/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <stdio.h>
#include <cstdint>

extern "C" {
#include "gim_ioctl.h"
}
#include "smi_system_mock.hpp"

namespace amdsmi
{

extern SystemMock *GetSystemMock();
static const int MAGIC_FD = 0x12345578;
static const char *IOCTL_FILE_NAME = "/dev/gim-smi0";

static int gim_open(int type, int flags)
{
	(void)type;
	(void)flags;

	return GetSystemMock()->Open(IOCTL_FILE_NAME, 0, 0);
}

static int gim_ioctl(int fd, void *cmd)
{
	(void)fd;
	size_t count = 0;

	return (int)GetSystemMock()->Ioctl(MAGIC_FD, cmd, count);
}


static int gim_access(int type)
{
	(void)type;

	return GetSystemMock()->Access(IOCTL_FILE_NAME, 0);
}


static int gim_close(int fd)
{
	(void)fd;
	return GetSystemMock()->Close(MAGIC_FD);
}
}

struct gim_ioctl *gim_get_ioctl()
{
	static struct gim_ioctl ioctl = {
		.open = amdsmi::gim_open,
		.access = amdsmi::gim_access,
		.ioctl = amdsmi::gim_ioctl,
		.close = amdsmi::gim_close,
		.is_user_mode = true
	};

	int driver_mode = amdsmi::GetSystemMock()->GetDriverMode();
	if(driver_mode == 0) {
		ioctl.is_user_mode = true;
		return &ioctl;
	} else if(driver_mode == 1) {
		ioctl.is_user_mode = false;
		return &ioctl;
	} else {
		return NULL;
	}
}