dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # commit v6.3-rc1-g1aaba11da9aa
dnl # driver core: class: remove module * from class_create()
dnl #

AC_DEFUN([AC_CLASS_CREATE_1_ARG], [
	AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/device/class.h>
                ], [
			 struct class *dcore_class;
                        dcore_class = class_create(NULL);
                ], [

                        AC_DEFINE(HAVE_CLASS_CREATE_1_ARG, 1,
                                [class_create has 1 argument])
                ])
        ])
])
