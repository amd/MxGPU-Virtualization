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
