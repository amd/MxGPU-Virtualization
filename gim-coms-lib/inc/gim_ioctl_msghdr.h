/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _GIM_IOCTL_MSGHDR_H_
#define _GIM_IOCTL_MSGHDR_H_

#include "amdgv_cmd_def.h"
#include "smi_cmd_def.h"

ssize_t gim_ioctl_msghdr_send_fd(int fd, int fd2ser);
ssize_t gim_ioctl_msghdr_send_shm_fd(struct amdgv_cmd_shm_info *shm_info, int fd);
ssize_t gim_ioctl_msghdr_send_smi_fd(uint32_t *smi_payload, int fd);
ssize_t gim_ioctl_msghdr_recv(struct amdgv_cmd_shm_info *shm_info, int fd);
void gim_ioctl_ctx_save(void *cmd, struct amdgv_cmd_ctx *ctx);
void gim_ioctl_ctx_restore(void *cmd, struct amdgv_cmd_ctx *ctx);

#endif // _GIM_IOCTL_MSGHDR_H_
