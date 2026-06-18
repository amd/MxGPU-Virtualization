dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

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
