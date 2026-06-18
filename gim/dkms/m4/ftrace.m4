dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # commit v5.10-rc2-25-gd19ad0775dcd
dnl # ftrace: Have the callbacks receive a struct ftrace_regs instead of pt_regs
dnl #
AC_DEFUN([AC_FTRACE_REGS_OPS], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/ftrace.h>
		], [
			typedef void (*ftrace_func_t)(unsigned long ip, unsigned long parent_ip,
				struct ftrace_ops *op, struct ftrace_regs *fregs);

			ftrace_func_t ftrace_ops_get_func(struct ftrace_ops *ops);
		], [
			AC_DEFINE(HAVE_FTRACE_REGS_OPS, 1,
				[struct ftrace_regs is available])
		])
	])
])
