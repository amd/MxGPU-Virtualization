dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # commit v4.13-rc7-5-gbdd1d2d3d251
dnl # fs: fix kernel_read prototype
dnl #
AC_DEFUN([AC_LOFF_T_VARIABLE], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/version.h>
			#include <linux/fs.h>
		], [
			loff_t loff_t;
			kernel_read(NULL, NULL, 0, &loff_t);
		], [
			AC_DEFINE(HAVE_LOFF_T_VARIABLE, 1,
				[variable loff_t is available])
		])
	])
])
