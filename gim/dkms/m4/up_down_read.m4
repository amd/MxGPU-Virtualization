dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl # commit v5.7-13158-gda1c55f1b272
dnl # mmap locking API: rename mmap_sem to mmap_lock
dnl #

AC_DEFUN([AC_UP_DOWN_READ], [
	AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/mm.h>
                ], [
			struct mm_struct *mm;
			down_read(&mm->mmap_lock);
			up_read(&mm->mmap_lock);
                ], [
                        AC_DEFINE(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG, 1,
                                [up_read down_read have mmap_lock as argument])
                ])
        ])

])

