/*
 * Copyright (c) 2019-2022 Advanced Micro Devices, Inc. All rights reserved.
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

#if defined(__linux__) && (defined(__SANITIZE_THREAD__) || defined(__SANITIZE_ADDRESS__))
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include "smi_sys_wrapper.h"
#include "smi_debug.h"
#include "smi_defines.h"

#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#ifndef _WIN64
#include "gim_ioctl.h"
#endif

static int64_t microseconds_to_milliseconds(int64_t msec)
{
	if (msec < 0)
		return -1;
	else if (msec != 0 && msec < 1000)
		return 1;

	return msec / 1000;
}

static int64_t diff_time(struct timeval *a, struct timeval *b)
{
	int64_t tmp = 0;

	tmp = (a->tv_usec + a->tv_sec * 1000000) - (b->tv_usec + b->tv_sec * 1000000);

	return tmp;
}
static void *amdsmi_lnx_poll_alloc(smi_event_handle_t *event_handle, uint32_t num_handles)
{
	struct pollfd *poll_fds = malloc((unsigned) num_handles * sizeof(struct pollfd));

	if (poll_fds) {
		for (uint32_t i = 0; i < num_handles; ++i) {
			poll_fds[i].fd = event_handle[i].fd;
			poll_fds[i].events = POLLIN;
		}
	}
	return poll_fds;
}


static int amdsmi_lnx_poll(struct smi_event_set_s *event_set, amdsmi_event_entry_t *event,
			    int64_t timeout)
{
	struct timeval stop, start;
	int poll_res = 0;
	struct pollfd *poll_fds = (struct pollfd *)event_set->_private;
	int64_t cmp;
	ssize_t read_res;

	gettimeofday(&start, NULL);

	do {
		poll_res = poll(poll_fds, (size_t) event_set->num_handles, (int) microseconds_to_milliseconds(timeout));
		if (poll_res < 0) {
			SMI_DEBUG("Poll system call failed. Return code: %d", AMDSMI_STATUS_API_FAILED);
			return AMDSMI_STATUS_API_FAILED;
		};
		if (poll_res > 0)
			break;

		gettimeofday(&stop, NULL);

		cmp = diff_time(&stop, &start);

	} while (cmp <= timeout);

	if (poll_res == 0) {
		SMI_DEBUG("Poll timedout. Return code: %d", AMDSMI_STATUS_TIMEOUT);
		return AMDSMI_STATUS_TIMEOUT;
	}

	for (size_t i = 0; i < event_set->num_handles; ++i) {
		if (poll_fds[i].revents & POLLIN) {
			read_res = read(poll_fds[i].fd, event, sizeof(amdsmi_event_entry_t));
			if (read_res == 0) {
				continue;
			} else if (read_res < 0) {
				SMI_ERROR("Poll read failed with error: %d. Return code: %d",
					errno, AMDSMI_STATUS_API_FAILED);
				return AMDSMI_STATUS_API_FAILED;
			} else {
				break;
			}
		}
	}
	return AMDSMI_STATUS_SUCCESS;
}

static int amdsmi_ioctl_request(smi_file_handle file_handle, smi_ioctl_cmd *ioctl_cmd)
{
	int ret;
	struct gim_ioctl *client;
	client = gim_get_ioctl();
	if (client == NULL) {
		SMI_ERROR("ioctl client interface not found\n");
		return -EINVAL;
	}

	ret = client->ioctl(file_handle, ioctl_cmd);
	if (ret < 0) {
		SMI_ERROR("failed to send smi cmd=0x%08X to user mode driver. client fd(%d)\n",
			ioctl_cmd->in_hdr.code, file_handle);
		return ret;
	}
	SMI_DEBUG("smi cmd=0x%08X successfully processed by user mode driver. status=%d\n",
		ioctl_cmd->in_hdr.code, ioctl_cmd->out_hdr.status);

	return 0;
}

static smi_file_handle amdsmi_open_file_handle(enum smi_file_access_mode access_mode)
{
	int access;
	smi_file_handle client_fd;
	struct gim_ioctl *client;

	client = gim_get_ioctl();
	if (client == NULL) {
		SMI_ERROR("ioctl client not found\n");
		return -EINVAL;
	}

	switch (access_mode) {
	case SMI_READONLY:
		access = O_RDONLY;
		break;
	case SMI_RDWR:
		access = O_RDWR;
		break;
	default:
		return -SMI_INVAL_HANDLE;
	}

	client_fd = client->open(SMI_IOCTL, access);
	if (client_fd < 0) {
		SMI_ERROR("failed to open SMI ioctl interface: error=%s\n",
			strerror(errno));
		return client_fd;
	}

	SMI_DEBUG("SMI ioctl interface opened successfully\n");
	return client_fd;
}

static int amdsmi_verify_driver(void)
{
	struct gim_ioctl *client;

	client = gim_get_ioctl();
	if (client == NULL) {
		SMI_ERROR("ioctl client not found\n");
		return false;
	}

	return client->access(SMI_IOCTL);
}

static int amdsmi_close_file_handle(smi_file_handle fd)
{
	struct gim_ioctl *client;

	client = gim_get_ioctl();
	if (client == NULL) {
		SMI_ERROR("ioctl client not found\n");
		return false;
	}

	return client->close(fd);
}

static bool amdsmi_is_user_mode(void)
{
	struct gim_ioctl *client;

	client = gim_get_ioctl();
	if (client == NULL) {
		SMI_ERROR("ioctl client not found\n");
		return false;
	}

	return client->is_user_mode;
}

static void *amdsmi_aligned_alloc(void **mem, size_t alignment, size_t size)
{
#if !defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_ADDRESS__)
	AMDSMI_UNUSED(mem);
	return aligned_alloc(alignment, size);
#else
	return posix_memalign(mem, alignment, size) == 0 ? *mem : NULL;
#endif
}

static int amdsmi_strncpy(char *dest, size_t destsz, const char *src, size_t count)
{
	if (destsz == 0) {
		return AMDSMI_STATUS_API_FAILED;
	}
	if (count >= destsz) {
		count = destsz - 1;
	}
	strncpy(dest, src, count);
	dest[count] = '\0';
	return AMDSMI_STATUS_SUCCESS;
}

system_wrapper *get_system_wrapper(void)
{
	static system_wrapper wrapper = {
		.malloc = malloc,
		.calloc = calloc,
		.free = free,
		.ioctl = amdsmi_ioctl_request,
		.open = amdsmi_open_file_handle,
		.access = amdsmi_verify_driver,
		.close = amdsmi_close_file_handle,
		.poll = amdsmi_lnx_poll,
		.poll_alloc = amdsmi_lnx_poll_alloc,
		.is_user_mode = amdsmi_is_user_mode,
		.aligned_alloc = amdsmi_aligned_alloc,
		.aligned_free = free,
		.strncpy = amdsmi_strncpy
	};

	return &wrapper;
}
