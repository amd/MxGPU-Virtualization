dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # commit v5.18-rc6-74-g8e432bb015b6
dnl # vfio/mdev: Pass in a struct vfio_device * to vfio_pin/unpin_pages()
dnl #
AC_DEFUN([AC_DCORE_IOVA_VM_CTX_VFIO_DEVICE], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/vfio_pci_core.h>
		], [
			struct vfio_device *vdev;
			vfio_unpin_pages(vdev, 0, 0);
		], [
			AC_DEFINE(HAVE_DCORE_IOVA_VM_CTX_VFIO_DEVICE, 1,
				[struct dcore_iova_vm_cts has struct vfio_device as member])
		])
	])
])
dnl #
dnl # commit v5.19-rc4-38-g34a255e67615
dnl # Replace phys_pfn with pages for vfio_pin_pages()
dnl #
AC_DEFUN([AC_DCORE_IOVA_VM_CTX_PAGE_ARRAY], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/vfio.h>
		], [
			struct page **pages;
			vfio_pin_pages(NULL, 0, 0, 0, pages);
		], [
			AC_DEFINE(HAVE_DCORE_IOVA_VM_CTX_PAGE_ARRAY, 1,
				[vfio_pin_pages uses struct page for argument])
		])
	])
])
