dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl # commit ce4b4657ff18925c315855aa290e93c5fa652d96
dnl # vfio: Replace the DMA unmapping notifier with a callback
dnl #
dnl # The vfio_register_notifier/vfio_unregister_notifier API was replaced
dnl # with a dma_unmap callback in struct vfio_device_ops.
dnl #
AC_DEFUN([AC_VFIO_DMA_UNMAP], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/vfio.h>
		], [
			struct vfio_device_ops ops;
			ops.dma_unmap = NULL;
		], [
			AC_DEFINE(HAVE_VFIO_DMA_UNMAP, 1,
				[vfio_device_ops has dma_unmap callback])
		])
	])
])

