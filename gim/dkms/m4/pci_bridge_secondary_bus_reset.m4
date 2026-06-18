dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # commit v4.18-rc1-5-g381634cad15b
dnl # PCI: Hide pci_reset_bridge_secondary_bus() from drivers
dnl #
AC_DEFUN([AC_BRIDGE_SECONDARY_BUS_RESET], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/pci.h>
		], [
			pci_bridge_secondary_bus_reset(NULL);
		], [
			AC_DEFINE(HAVE_BRIDGE_SECONDARY_BUS_RESET, 1,
				[function pci_bridge_secondary_bus_reset is available])
		])
	])
])
