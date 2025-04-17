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

