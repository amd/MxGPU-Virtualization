/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
