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
dnl # commit v5.18-rc6-74-g8e432bb015b6
dnl # vfio/mdev: Pass in a struct vfio_device * to vfio_pin/unpin_pages()
dnl #
AC_DEFUN([AC_DCORE_IOVA_VM_CTX_VFIO_DEVICE], [
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/vfio_pci_core.h>
                ], [
                        struct vfio_device *vdev;
                        vfio_unpin_pages(vdev, NULL, 0);
                ], [

                        AC_DEFINE(HAVE_DCORE_IOVA_VM_CTX_VFIO_DEVICE, 1,
                                [struct dcore_iova_vm_cts has struct vfio_device as member])
                ])
        ])
dnl #
dnl # commit v5.19-rc4-38-g34a255e67615
dnl # Replace phys_pfn with pages for vfio_pin_pages()
dnl #
AC_DEFUN([AC_DCORE_IOVA_VM_CTX_PAGE_ARRAY], [
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/vfio.h>
                ], [

                        struct page **pages;
                        vfio_pin_pages(NULL,NULL,0,0,pages);

                ], [
                        AC_DEFINE(HAVE_DCORE_IOVA_VM_CTX_PAGE_ARRAY, 1,
                                [vfio_pin_pages uses struct page for argument])
                ])
        ])
