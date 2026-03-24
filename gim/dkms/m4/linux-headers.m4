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
dnl # commit v4.11-10671-g299878bac326
dnl # treewide: move set_memory_* functions away from cacheflush.h
dnl #

AC_DEFUN([AC_GIM_LINUX_HEADERS], [

        AC_KERNEL_TEST_HEADER_FILE_EXIST([asm/set_memory.h], [
                AC_DEFINE(HAVE_ASM_SET_MEMORY_H, 1,
                        [header <asm/set_memory.h> is available])
        ])

        dnl #
        dnl # commit v5.17-14089-g63d12cc30574
        dnl # PCI: Remove the deprecated "pci-dma-compat.h" API
        dnl #
        AC_KERNEL_TEST_HEADER_FILE_EXIST([linux/pci-dma-compat.h], [
                AC_DEFINE(HAVE_LINUX_PCI_DMA_COMPAT_H, 1,
                        [header <linux/pci-dma-compat.h> is available])
        ])

        dnl #
        dnl # v4.15-rc7-12-gea8c64ace866
        dnl # dma-mapping: move swiotlb arch helpers to a new header
        dnl #
        AC_KERNEL_TEST_HEADER_FILE_EXIST([linux/dma-direct.h], [
                AC_DEFINE(HAVE_LINUX_DMA_DIRECT_H, 1,
                        [header <linux/dma-direct.h> is available])
        ])

        dnl #
        dnl # v5.14-rc5-11-gc0891ac15f04
        dnl # isystem: ship and use stdarg.h
        dnl #
        AC_KERNEL_TEST_HEADER_FILE_EXIST([linux/stdarg.h], [
                AC_DEFINE(HAVE_LINUX_STDARG_H, 1,
                        [header <linux/stdarg.h> is available])
        ])

])
