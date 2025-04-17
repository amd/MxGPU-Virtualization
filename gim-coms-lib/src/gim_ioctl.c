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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "gim_ioctl.h"
#include "gim_fd_list.h"

#include "dcore_ioctl.h"

#include "amdgv_cmd_def.h"
#include "smi_cmd_def.h"
#include "gim_ioctl_msghdr.h"

#define SMI_IOCTL_NAME                     "gim-smi0"
#define SMI_IOCTL_PATH                     "/dev/" SMI_IOCTL_NAME

#define AMDGV_IOCTL_NAME                   "amdgv-cmd-handle"
#define AMDGV_IOCTL_PATH                   "/dev/" AMDGV_IOCTL_NAME

#define DCORE_IOCTL_NAME				   "amdgpu_rdl"
#define DCORE_IOCTL_PATH				   "/dev" DCORE_IOCTL_NAME

#define USER_MODE_IPC_SERVER_PATH          "/gim.socket"

#define GIM_USER_MODE_DRIVER               "gim_user_mode"
#define GIM_KERNEL_MODE_DRIVER             "gim"

#define SMI_IOCTL_CMD _IOWR('S', 0, struct smi_ioctl_cmd)
#define AMDGV_IOCTL_CMD _IOWR('R', 0, struct amdgv_cmd)

#define MAX_BUFFER_SIZE 1024

uint32_t gim_ioctl_get_cmd_size(uint32_t client_type)
{
	uint32_t ret = 0;

	switch (client_type) {
	case SMI_IOCTL:
		ret = sizeof(struct smi_ioctl_cmd);
		break;
	default:
		ret = sizeof(struct amdgv_cmd);
		break;
	}

	return ret;
}

static int create_connect_sock_fd(void)
{
	struct sockaddr_un sock;
	size_t len;
	int fd;
	int ret;

	fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("failed to open UNIX domain socket: error=%s\n",
			strerror(errno));
		return fd;
	}

	sock.sun_family = PF_UNIX;
	/* Make it abstract socket */
	sock.sun_path[0] = '\0';
	strcpy(sock.sun_path + 1, USER_MODE_IPC_SERVER_PATH);
	len = sizeof(sock.sun_family) + strlen(USER_MODE_IPC_SERVER_PATH) + 1;

	ret = connect(fd, (struct sockaddr *)&sock, (socklen_t)len);
	if (ret < 0) {
		printf("failed to connect to server %s on client socket : error=%s\n",
			USER_MODE_IPC_SERVER_PATH, strerror(errno));
		close(fd);
		return ret;
	}

	return fd;
}

static int gim_user_mode_open(int type, int flags)
{
	int fd;
	int ret;

	(void)type;
	(void)flags;

	fd = create_connect_sock_fd();
	if (fd < 0)
		return fd;

	ret = create_fd_list(fd);
	if (ret < 0)
		return ret;

	return fd;
}

static int gim_user_mode_ioctl(int fd, void *cmd)
{
	ssize_t ret;
	size_t cmd_size;
	int curr_fd;
	struct amdgv_cmd_ctx ctx;
	struct cmd_header hdr = {0};

	if (cmd == NULL) {
		printf("NULL arguments\n");
		return -EINVAL;
	}

	cmd_size = gim_ioctl_get_cmd_size(GIM_IOCTL_GET_TYPE(cmd));

	curr_fd = fd;
	fd = search_fd_from_fd_list(fd);
	if (fd < 0)
		return fd;
	else if (fd == 0) {
		fd = create_connect_sock_fd();
		if (fd < 0)
			return fd;

		ret = (int)insert_fd_into_fd_list(curr_fd, fd);
		if (ret < 0)
			return (int)ret;
	}

	/* send header */
	hdr.cmd_id = (*(uint32_t *)cmd) | IOCTL_RECV_HDR_MASK;
	hdr.pid = (uint32_t)getpid();
	hdr.primary_fd = (uint32_t)curr_fd;
	hdr.thread_fd = (uint32_t)fd;
	ret = write(fd, &hdr, sizeof(struct cmd_header));

	/* send command */
	gim_ioctl_ctx_save(cmd, &ctx);
	ret = write(fd, cmd, cmd_size);

	if (AMDGV_CMD_SHM_CLI2SER(cmd)) {
		struct amdgv_cmd *s_cmd = (struct amdgv_cmd *)cmd;
		ret = gim_ioctl_msghdr_send_shm_fd((struct amdgv_cmd_shm_info *)s_cmd->input_buff_raw, fd);
	}

	if (SMI_CMD_FD_CLI2SER(cmd)) {
		struct smi_ioctl_cmd *smi_cmd = (struct smi_ioctl_cmd *)cmd;
		ret = gim_ioctl_msghdr_send_smi_fd(smi_cmd->payload, fd);
	}

	if (ret < 0) {
		printf("failed to send data to server: %s\n",
				USER_MODE_IPC_SERVER_PATH);
		return (int)ret;
	}
	/* receive response */
	ret = read(fd, cmd, cmd_size);
	gim_ioctl_ctx_restore(cmd, &ctx);

	if (AMDGV_CMD_SHM_SER2CLI(cmd)) {
		struct amdgv_cmd *r_cmd = (struct amdgv_cmd *)cmd;
		ret = gim_ioctl_msghdr_recv((struct amdgv_cmd_shm_info *)r_cmd->output_buff_raw, fd);
	}

	if (ret < 0) {
		printf("failed to receive response from server: %s\n",
				USER_MODE_IPC_SERVER_PATH);
		return (int)ret;
	}

	return 0;
}

static int gim_user_mode_access(int type)
{
	(void)type;

	int ret = -1;

	int sockfd = create_connect_sock_fd();
	if (sockfd >= 0) {
		ret = 0;
		close(sockfd);
	}

	return ret;
}

static int gim_user_mode_close(int fd)
{
	return destroy_fd_list(fd);
}

static int gim_kernel_mode_open(int type, int flags)
{
	switch (type) {
	case SMI_IOCTL:
		return open(SMI_IOCTL_PATH, flags);
	case AMDGV_IOCTL:
		return open(AMDGV_IOCTL_PATH, flags);
	default:
		return -EINVAL;
	}
}

static int gim_kernel_mode_ioctl(int fd, void *cmd)
{
	if (cmd == NULL) {
		printf("NULL arguments\n");
		return -EINVAL;
	}

	switch (GIM_IOCTL_GET_TYPE(cmd)) {
	case SMI_IOCTL:
		return ioctl(fd, SMI_IOCTL_CMD, (struct smi_ioctl_cmd *)cmd);
	case AMDGV_IOCTL:
		return ioctl(fd, AMDGV_IOCTL_CMD, (struct amdgv_cmd *)cmd);
	default:
		return -EINVAL;
	}
}

static int gim_kernel_mode_access(int type)
{
	switch (type) {
	case SMI_IOCTL:
		return access(SMI_IOCTL_PATH, F_OK);
	case AMDGV_IOCTL:
		return access(AMDGV_IOCTL_PATH, F_OK);
	default:
		return -EINVAL;
	}
}

static int gim_kernel_mode_close(int fd)
{
	close(fd);

	return 0;
}

struct gim_ioctl *gim_get_ioctl(void)
{
	char command[MAX_BUFFER_SIZE];
	char output[MAX_BUFFER_SIZE];
	FILE *fp;
	char *result;
	static struct gim_ioctl *ioctl_wrapper = NULL;

	static struct gim_ioctl user_mode_ioctl = {
		.open = gim_user_mode_open,
		.ioctl = gim_user_mode_ioctl,
		.access = gim_user_mode_access,
		.close = gim_user_mode_close,
		.is_user_mode = true
	};

	static struct gim_ioctl kernel_mode_ioctl = {
		.open = gim_kernel_mode_open,
		.ioctl = gim_kernel_mode_ioctl,
		.access = gim_kernel_mode_access,
		.close = gim_kernel_mode_close,
		.is_user_mode = false
	};

	if (ioctl_wrapper == NULL) {
		/* check if gim user mode application is loaded */
		snprintf(command, sizeof(command), "pgrep %s", GIM_USER_MODE_DRIVER);
		fp = popen(command, "r");
		if (fp != NULL) {
			result = fgets(output, sizeof(output), fp);
			if (result != NULL) {
				if (strlen(output) > 0) {
					pclose(fp);
					ioctl_wrapper = &user_mode_ioctl;
					goto exit_end;
				}
			}
			pclose(fp);
		}

		/* check if gim kernel mode driver is loaded */
		snprintf(command, sizeof(command), "lsmod | grep %s",
			GIM_KERNEL_MODE_DRIVER);
		fp = popen(command, "r");
		if (fp != NULL) {
			result = fgets(output, sizeof(output), fp);
			if (result != NULL) {
				if (strlen(output) > 0)
					ioctl_wrapper = &kernel_mode_ioctl;
			}
			pclose(fp);
		}
	}

exit_end:
	return ioctl_wrapper;
}
