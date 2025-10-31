/*
 * Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
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
#include <linux/mm.h>
#include <linux/sysfs.h>

#include "gim_debug.h"
#include "gim_sysfs_emit.h"

/**
 *	sysfs_emit - scnprintf equivalent, aware of PAGE_SIZE buffer.
 *	@buf:	start of PAGE_SIZE buffer.
 *	@fmt:	format
 *	@...:	optional arguments to @format
 *
 *
 * Returns number of characters written to @buf.
 */
int gim_sysfs_emit(char *buf, const char *fmt, ...)
{
	va_list args;
	int len;

	if (!buf || offset_in_page(buf)) {
		gim_warn("invalid sysfs_emit: buf:%p\n", buf);
		return 0;
	}

	va_start(args, fmt);
	len = vscnprintf(buf, PAGE_SIZE, fmt, args);
	va_end(args);

	return len;
}

/**
 *      sysfs_emit_at - scnprintf equivalent, aware of PAGE_SIZE buffer.
 *      @buf:   start of PAGE_SIZE buffer.
 *      @at:    offset in @buf to start write in bytes
 *              @at must be >= 0 && < PAGE_SIZE
 *      @fmt:   format
 *      @...:   optional arguments to @fmt
 *
 *
 * Returns number of characters written starting at &@buf[@at].
 */
int gim_sysfs_emit_at(char *buf, int at, const char *fmt, ...)
{
	va_list args;
	int len;

	if (!buf || offset_in_page(buf) || at < 0 || at >= PAGE_SIZE) {
		gim_warn("invalid sysfs_emit_at: buf:%p at:%d\n", buf, at);
		return 0;
	}

	va_start(args, fmt);
	len = vscnprintf(buf + at, PAGE_SIZE - at, fmt, args);
	va_end(args);

	return len;
}
