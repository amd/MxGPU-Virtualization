/*
 * Copyright (c) 2014-2019 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
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
	do { if (shim_log_level >= AMDGV_INFO_LEVEL) \
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
	do { if (shim_log_level >= AMDGV_INFO_LEVEL) \
		printk(KERN_INFO "gim debug:(%02x:%02x.%x)(%s:%d) " fmt, \
			((bdf) & 0xff00) >> 8, ((bdf) & 0xf8) >> 3, ((bdf) & 0x7), __func__, \
			__LINE__, ##s); } while (0)

#endif
