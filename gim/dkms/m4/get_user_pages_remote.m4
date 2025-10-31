
dnl *
dnl * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
dnl *
dnl * Permission is hereby granted, free of charge, to any person obtaining a copy
dnl * of this software and associated documentation files (the "Software"), to deal
dnl * in the Software without restriction, including without limitation the rights
dnl * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
dnl * copies of the Software, and to permit persons to whom the Software is
dnl * furnished to do so, subject to the following conditions:
dnl *
dnl * The above copyright notice and this permission notice shall be included in
dnl * all copies or substantial portions of the Software.
dnl *
dnl * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
dnl * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
dnl * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
dnl * THE SOFTWARE
dnl *
dnl #
dnl # commit v6.4-rc4-55-gca5e863233e8
dnl # mm/gup: remove vmas parameter from get_user_pages_remote()
dnl #
AC_DEFUN([AC_GET_USER_PAGES_REMOTE_6_ARG], [
	AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/mm.h>
                ], [
                        get_user_pages_remote(NULL,
			   0, 0,
			   0, NULL,
			   NULL);
                ], [
                        AC_DEFINE(HAVE_GET_USER_PAGES_REMOTE_6_ARG, 1,
                                [get_user_pages_remote has 6 arguments])
                ])
        ])

])

dnl #
dnl # commit v4.8-14096-g9beae1ea8930
dnl # mm: replace get_user_pages_remote() write/force parameters with gup_flags
dnl #

AC_DEFUN([AC_GET_USER_PAGES_REMOTE_7_ARG], [
	AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/mm.h>
                ], [
                        get_user_pages_remote(NULL,
			   0, 0,
			   0, NULL,
			   NULL, NULL);

                ], [
                        AC_DEFINE(HAVE_GET_USER_PAGES_REMOTE_7_ARG, 1,
                                [get_user_pages_remote has 7 arguments])
                ])
        ])

])

dnl #
dnl # commit v5.8-12463-g64019a2e467a
dnl # mm/gup: remove task_struct pointer for all gup code
dnl #

AC_DEFUN([AC_GET_USER_PAGES_REMOTE_8_ARG], [
	AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/mm.h>
                ], [
                        get_user_pages_remote(NULL, NULL,
			   0, 0,
			   0, NULL,
			   NULL, NULL);

                ], [
                        AC_DEFINE(HAVE_GET_USER_PAGES_REMOTE_8_ARG, 1,
                                [get_user_pages_remote has 8 arguments])
                ])
        ])

])
