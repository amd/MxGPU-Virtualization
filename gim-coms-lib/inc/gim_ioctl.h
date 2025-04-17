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

#ifndef _GIM_IOCTL_H_
#define _GIM_IOCTL_H_

enum gim_ioctl_type {
	SMI_IOCTL = 1 << 24,
	AMDGV_IOCTL = 2 << 24,
};

/* Bits 24-31: Client type */
#define IOCTL_TYPE_MASK 0xFF000000
#define GIM_IOCTL_GET_TYPE(cmd) ((*(uint32_t *)cmd) & IOCTL_TYPE_MASK)
/* Bit 21: Indicator for whether to receive a cmd header first */
#define IOCTL_RECV_HDR_MASK 0x00200000
#define GIM_IOCTL_RECV_HDR(cmd) ((*(uint32_t *)cmd) & IOCTL_RECV_HDR_MASK)

struct gim_ioctl {
	int (*open)(int type, int flags);
	int (*access)(int type);
	int (*ioctl)(int fd, void *cmd);
	int (*close)(int fd);
	bool is_user_mode;
};

uint32_t gim_ioctl_get_cmd_size(uint32_t);

struct gim_ioctl *gim_get_ioctl(void);

#endif // _GIM_IOCTL_H_
