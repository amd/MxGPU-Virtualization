/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DRV_TYPES_H__
#define __SMI_DRV_TYPES_H__

#ifdef __KERNEL__
#include <linux/types.h>

typedef struct file *smi_process_handle;
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct smi_file {
	void *private_data;
};
typedef struct smi_file *smi_process_handle;
#endif

typedef void *file_t;

#endif // __SMI_DRV_TYPES_H__
