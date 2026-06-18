dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl # commit v6.7-rc4-422-g5e0a760b4441
dnl # mm, treewide: rename MAX_ORDER to MAX_PAGE_ORDER
dnl #

AC_DEFUN([AC_MAX_PAGE_ORDER], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/mmzone.h>
		], [
			unsigned tmp = MAX_PAGE_ORDER;
		], [
			AC_DEFINE(HAVE_MAX_PAGE_ORDER, 1,
				[rename MAX_ORDER to MAX_PAGE_ORDER])
		])
	])
])
