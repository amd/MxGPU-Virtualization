/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_OS_DEFINES_H__
#define __SMI_OS_DEFINES_H__

#include <errno.h>
#include <unistd.h>

#define SMI_INVAL_HANDLE -1
#define SMI_ACCESS_DENIED EACCES
#define SMI_INVAL_ARG EINVAL
#define SMI_EIO EIO
#define SMI_LAST_ERROR errno
#define SMI_EXPORT

#endif // __SMI_OS_DEFINES_H__
