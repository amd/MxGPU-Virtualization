dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # commit v5.8-rc2-21-g74caba7f2a06
dnl # printk: move dictionary keys to dev_printk_info
dnl #
AC_DEFUN([AC_VPRINTK_EMIT_5_ARG], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/printk.h>
		], [
			const char *fmt;
			vprintk_emit(0, 0, NULL, fmt, NULL);
		], [
			AC_DEFINE(HAVE_VPRINTK_EMIT_5_ARG, 1,
				[vprintk_emit has 5 arguments])
		])
	])
])
