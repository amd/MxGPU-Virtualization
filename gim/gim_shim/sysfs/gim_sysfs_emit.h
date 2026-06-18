/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GIM_SYSFS_EMIT_H
#define GIM_SYSFS_EMIT_H

#include <linux/sysfs.h>

__printf(2, 3)
int gim_sysfs_emit(char *buf, const char *fmt, ...);
__printf(3, 4)
int gim_sysfs_emit_at(char *buf, int at, const char *fmt, ...);

#endif