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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include "gim_ioctl_msghdr.h"

ssize_t gim_ioctl_msghdr_send_fd(int fd, int fd2ser)
{
	ssize_t ret = 0;
	struct msghdr msg = {0};
	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(fd2ser))], data;
	struct iovec io = { .iov_base = &data, .iov_len = 1 };

	/* Prepare the msghdr structure for sendmsg */
	memset(buf, '\0', sizeof(buf));

	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd2ser));

	*((int *) CMSG_DATA(cmsg)) = fd2ser;

	msg.msg_controllen = cmsg->cmsg_len;

	/* Send the command and the file descriptor */
	ret = sendmsg(fd, &msg, 0);
	return ret;
}

ssize_t gim_ioctl_msghdr_send_shm_fd(struct amdgv_cmd_shm_info *shm_info, int fd)
{
	int memfd;

	memfd = (int)syscall(__NR_memfd_create, "shm", MFD_CLOEXEC);
	if (memfd == -1) {
		printf("failed to created shared memory\n");
		return -1;
	}

	/* Write the content of cmd to the shared memory */
	if (write(memfd, (void *)shm_info->buffer_addr, shm_info->buffer_size) != shm_info->buffer_size) {
		printf("failed to write to shared memory\n");
		return -1;
	}

	return gim_ioctl_msghdr_send_fd(fd, memfd);
}

ssize_t gim_ioctl_msghdr_send_smi_fd(uint32_t *smi_payload, int fd)
{
	int smi_fd = *(int *)((char *)smi_payload + FD_OFFSET);
	return gim_ioctl_msghdr_send_fd(fd, smi_fd);
}

ssize_t gim_ioctl_msghdr_recv(struct amdgv_cmd_shm_info *shm_info, int fd)
{
	struct msghdr msg = {0};
	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(int))], data;
	struct iovec io = { .iov_base = &data, .iov_len = 1 };
	void *shm = NULL;
	int memfd;

	if (!shm_info)
		return -1;
	if (shm_info->buffer_size == 0)
		return 0;

	memset(buf, '\0', sizeof(buf));
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);

	if (recvmsg(fd, &msg, 0) == -1) {
		printf("recvmsg failed %d\n", errno);
		return -1;
	}
	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL || cmsg->cmsg_type != SCM_RIGHTS) {
		printf("No SCM_RIGHTS received\n");
		return -1;
	}

	memfd = *((int *) CMSG_DATA(cmsg));

	// Map the shared memory into the process's address space
	shm = mmap(NULL, shm_info->buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
	if (shm == MAP_FAILED) {
		printf("shared memory mapping failed\n");
		return -1;
	}

	memcpy((void *)shm_info->buffer_addr, shm, shm_info->buffer_size);
	munmap(shm, shm_info->buffer_size);
	return 0;
}

void gim_ioctl_ctx_save(void *cmd, struct amdgv_cmd_ctx *ctx)
{
	struct amdgv_cmd_shm_info *shm_info;
	if (AMDGV_CMD_SHM_CLI2SER(cmd)) {
		shm_info = (struct amdgv_cmd_shm_info *)(((struct amdgv_cmd *)cmd)->input_buff_raw);
		ctx->input_buffer_addr = shm_info->buffer_addr;
	}

	if (AMDGV_CMD_SHM_SER2CLI(cmd)) {
		shm_info = (struct amdgv_cmd_shm_info *)(((struct amdgv_cmd *)cmd)->output_buff_raw);
		ctx->output_buffer_addr = shm_info->buffer_addr;
	}
}

void gim_ioctl_ctx_restore(void *cmd, struct amdgv_cmd_ctx *ctx)
{
	struct amdgv_cmd_shm_info *shm_info;
	if (AMDGV_CMD_SHM_CLI2SER(cmd)) {
		shm_info = (struct amdgv_cmd_shm_info *)(((struct amdgv_cmd *)cmd)->input_buff_raw);
		shm_info->buffer_addr = ctx->input_buffer_addr;
	}

	if (AMDGV_CMD_SHM_SER2CLI(cmd)) {
		shm_info = (struct amdgv_cmd_shm_info *)(((struct amdgv_cmd *)cmd)->output_buff_raw);
		shm_info->buffer_addr = ctx->output_buffer_addr;
	}
}
