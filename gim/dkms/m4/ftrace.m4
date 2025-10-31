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
dnl # commit v5.10-rc2-25-gd19ad0775dcd
dnl # ftrace: Have the callbacks receive a struct ftrace_regs instead of pt_regs
dnl #
AC_DEFUN([AC_FTRACE_REGS_OPS], [
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/ftrace.h>
                ], [
                        typedef void (*ftrace_func_t)(unsigned long ip, unsigned long parent_ip,
                              struct ftrace_ops *op, struct ftrace_regs *fregs);

                        ftrace_func_t ftrace_ops_get_func(struct ftrace_ops *ops);
                ], [

                        AC_DEFINE(HAVE_FTRACE_REGS_OPS, 1,
                                [struct ftrace_regs is available])
                ])
        ])
