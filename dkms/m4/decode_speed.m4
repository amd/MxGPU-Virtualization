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
dnl # commit v4.12-rc1-2-g56c1af4606f0
dnl # PCI: Add sysfs max_link_speed/width, current_link_speed/width, etc
dnl #
AC_DEFUN([AC_DECODE_SPEED_8], [
        AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/pci_regs.h>
                ], [
                        #ifndef PCI_EXP_LNKCAP2_SLS_8_0GB
                        #error "Macro not defined"
                        #endif
                ], [
                        AC_DEFINE(HAVE_DECODE_SPEED_8, 1,
                                [PCI_EXP_LNKCAP2_SLS_8_0GB() is defined])
                ])
        ])
])


dnl #
dnl # commit v4.16-rc1-1-g1acfb9b7ee0b
dnl # PCI: Add decoding for 16 GT/s link speed
dnl #
AC_DEFUN([AC_DECODE_SPEED_16], [
        AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/pci_regs.h>
                ], [
                        #ifndef PCI_EXP_LNKCAP2_SLS_16_0GB
                        #error "Macro not defined"
                        #endif
                ], [
                        AC_DEFINE(HAVE_DECODE_SPEED_16, 1,
                                [PCI_EXP_LNKCAP2_SLS_16_0GB() is defined])
                ])
        ])
])

dnl #
dnl # commit v5.2-rc1-3-gde76cda215d5
dnl # PCI: Decode PCIe 32 GT/s link speed
dnl #
AC_DEFUN([AC_DECODE_SPEED_32], [
        AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/pci_regs.h>
                ], [
                        #ifndef PCI_EXP_LNKCAP2_SLS_32_0GB
                        #error "Macro not defined"
                        #endif
                ], [
                        AC_DEFINE(HAVE_DECODE_SPEED_32, 1,
                                [PCI_EXP_LNKCAP2_SLS_32_0GB() is defined])
                ])
        ])
])
