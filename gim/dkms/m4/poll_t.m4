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
dnl # commit v4.15-2176-g168fe32a072a
dnl # Pull poll annotations from Al Viro
dnl #
AC_DEFUN([AC_POLL_T], [
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/anon_inodes.h>
                        #include <linux/poll.h>
                        #include <linux/fs.h>
                ], [
                        __poll_t event(struct file *, struct poll_table_struct *);

                        static const struct file_operations smi_event_fops = {
                        .poll = event,
                        };

                        anon_inode_getfile("", &smi_event_fops, NULL, O_RDONLY);
                ], [

                        AC_DEFINE(HAVE_POLL_T, 1,
                                [smi_event_poll has poll_t return value])
                ])
        ])
