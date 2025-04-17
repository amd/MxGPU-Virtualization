dnl *
dnl * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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
dnl # v5.8-9635-g453431a54934
dnl # mm, treewide: rename kzfree() to kfree_sensitive()
dnl #
AC_DEFUN([AC_KFREE_SENSITIVE], [
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/slab.h>
                ], [
                        kfree_sensitive(NULL);
                ], [

                        AC_DEFINE(HAVE_KFREE_SENSITIVE, 1,
                                [kfree_sensitive is available])
                ])
        ])
