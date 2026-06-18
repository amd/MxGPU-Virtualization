dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # v5.8-9635-g453431a54934
dnl # mm, treewide: rename kzfree() to kfree_sensitive()
dnl #
AC_DEFUN([AC_KFREE_SENSITIVE], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/slab.h>
		], [
			kfree_sensitive(NULL);
		], [
			AC_DEFINE(HAVE_KFREE_SENSITIVE, 1,
				[kfree_sensitive is available])
		])
	])
])
