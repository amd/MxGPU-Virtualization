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
AC_DEFUN([AC_VFS_UNLINK_3_ARG], [
	AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/fs.h>
                ], [
                    vfs_unlink(NULL, NULL, NULL);
                ], [
                        AC_DEFINE(VFS_UNLINK_HAS_3_ARG, 1,
                                [VFS_UNLINK has 3 argument])
                ])
        ])
])

dnl #
AC_DEFUN([AC_VFS_UNLINK_IDMAP_ARG], [
	AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/fs.h>
                        #include <linux/mount.h>
                ], [
                    struct mnt_idmap *idmap;
                    vfs_unlink(idmap, NULL, NULL, NULL);
                ], [
                        AC_DEFINE(VFS_UNLINK_HAS_IDMAP_ARG, 1,
                                [VFS_UNLINK has 4 argument, first is mnt_idmap])
                ])
        ])
])
