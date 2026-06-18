dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # v4.10-rc1-4-g4fe0395550ae
dnl # PCI/MSI: Remove pci_enable_msi_{exact,range}()
dnl #
AC_DEFUN([AC_PCI_ALLOC_IRQ_VECTORS], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/pci.h>
		], [
			pci_alloc_irq_vectors(NULL, 0, 0, 0);
		], [
			AC_DEFINE(HAVE_PCI_ALLOC_IRQ_VECTORS, 1,
				[function pc_alloc_irq_vectors is available])
		])
	])
])
