dnl *
dnl * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
dnl # commit v5.19-rc4-3-g6e97eba8ad87
dnl # vfio: Split migration ops from main device ops
dnl #
AC_DEFUN([AC_SUPPORT_LIVE_MIGRATION], [
        AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/vfio.h>
                        #include <linux/vfio_pci_core.h>
                        #include <linux/iommu.h>
                ], [
                        enum iommu_cap caps = IOMMU_CAP_DIRTY_TRACKING;
                        unsigned int cmd = VFIO_MIG_GET_PRECOPY_INFO;
                        unsigned int mig_flags = VFIO_MIGRATION_STOP_COPY | VFIO_MIGRATION_PRE_COPY;
                        enum vfio_device_mig_state new_state;
                        struct vfio_migration_ops mig_ops;
                        struct vfio_device_ops vdev_ops;
                        struct vfio_precopy_info info;
                        vdev_ops.bind_iommufd = vfio_iommufd_physical_bind;
                        vdev_ops.unbind_iommufd = vfio_iommufd_physical_unbind;
                        vdev_ops.attach_ioas = vfio_iommufd_physical_attach_ioas;
                        vdev_ops.detach_ioas = vfio_iommufd_physical_detach_ioas;
                        vfio_pci_core_init_dev(NULL);
                        vfio_pci_core_release_dev(NULL);
                        mig_ops.migration_set_state(NULL,VFIO_DEVICE_STATE_STOP);
                        mig_ops.migration_set_state(NULL,VFIO_DEVICE_STATE_RUNNING);
                        mig_ops.migration_set_state(NULL,VFIO_DEVICE_STATE_RESUMING);
                        vfio_mig_get_next_state(NULL, VFIO_DEVICE_STATE_STOP, new_state, NULL);
                        new_state = mig_ops.migration_get_state(NULL,NULL);
                        mig_ops.migration_get_data_size(NULL, NULL);
                ], [
                        AC_DEFINE(SUPPORT_LIVE_MIGRATION, 1,
                                [live migration feature is available])
                ])
        ])
])
