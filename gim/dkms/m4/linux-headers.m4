dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

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
