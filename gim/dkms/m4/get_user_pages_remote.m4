dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

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
