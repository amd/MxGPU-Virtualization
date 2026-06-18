/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DRV_IOCTL_H__
#define __SMI_DRV_IOCTL_H__

#include <linux/ioctl.h>

#define SMI_IOCTL_MAGIC 'S'
#define SMI_IOCTL_COMMAND _IOWR(SMI_IOCTL_MAGIC, 0, struct smi_ioctl_cmd)

#endif // __SMI_DRV_IOCTL_H__
