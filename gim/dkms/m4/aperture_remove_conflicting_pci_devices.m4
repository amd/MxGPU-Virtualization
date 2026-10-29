dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # v5.15-rc1-19-g2d7a1a7e9c2e
dnl # video: Remove special framebuffer handling from PCI core
dnl #
AC_DEFUN([AC_APERTURE_REMOVE_CONFLICTING_PCI_DEVICES], [
        AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE_SYMBOL([
                        #include <linux/aperture.h>
                        #include <linux/pci.h>
                ], [
                        aperture_remove_conflicting_pci_devices(NULL, "gim");
                ], [aperture_remove_conflicting_pci_devices], [drivers/video/aperture.c], [
                        AC_DEFINE(HAVE_APERTURE_REMOVE_CONFLICTING_PCI_DEVICES, 1,
                                [aperture_remove_conflicting_pci_devices is available])
                ])
        ])
])