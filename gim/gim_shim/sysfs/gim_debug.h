/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef GIM_DEBUG_H
#define GIM_DEBUG_H
#include <amdgv_api.h>
#include "gim_error.h"

extern uint32_t shim_log_level;

#define gim_warn(fmt, s...)	\
	do { if (shim_log_level >= AMDGV_WARN_LEVEL) \
		printk(KERN_WARNING "gim warning:(%s:%d) " fmt, __func__, \
			__LINE__, ##s); } while (0)

#define gim_warn_bdf(bdf, fmt, s...)	\
	do { if (shim_log_level >= AMDGV_WARN_LEVEL) \
		printk(KERN_WARNING "gim warning:(%02x:%02x.%x)(%s:%d) " fmt, \
			((bdf) & 0xff00) >> 8, ((bdf) & 0xf8) >> 3, ((bdf) & 0x7), __func__, \
			__LINE__, ##s); } while (0)

#define gim_info(fmt, s...)	\
	do { if (shim_log_level >= AMDGV_INFO_LEVEL) \
		printk(KERN_INFO "gim info:(%s:%d) " fmt, __func__, \
			__LINE__, ##s); } while (0)

#define gim_info_bdf(bdf, fmt, s...)	\
	do { if (shim_log_level >= AMDGV_INFO_LEVEL) \
		printk(KERN_INFO "gim info:(%02x:%02x.%x)(%s:%d) " fmt, \
			((bdf) & 0xff00) >> 8, ((bdf) & 0xf8) >> 3, ((bdf) & 0x7), __func__, \
			__LINE__, ##s); } while (0)

#define gim_dbg(fmt, s...)	\
	do { if (shim_log_level >= AMDGV_DEBUG_LEVEL) \
		printk(KERN_INFO "gim debug:(%s:%d) " fmt, __func__, \
			__LINE__, ##s); } while (0)

#define gim_dbg_bdf(bdf, fmt, s...)	\
	do { if (shim_log_level >= AMDGV_DEBUG_LEVEL) \
		printk(KERN_INFO "gim debug:(%02x:%02x.%x)(%s:%d) " fmt, \
			((bdf) & 0xff00) >> 8, ((bdf) & 0xf8) >> 3, ((bdf) & 0x7), __func__, \
			__LINE__, ##s); } while (0)

#endif
