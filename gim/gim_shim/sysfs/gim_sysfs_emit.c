/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
