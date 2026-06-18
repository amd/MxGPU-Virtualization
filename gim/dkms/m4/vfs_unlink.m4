dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

AC_DEFUN([AC_VFS_UNLINK_3_ARG], [
	AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/fs.h>
                ], [
                    vfs_unlink(NULL, NULL, NULL);
                ], [
                        AC_DEFINE(VFS_UNLINK_HAS_3_ARG, 1,
                                [VFS_UNLINK has 3 argument])
                ])
        ])
])

dnl #
AC_DEFUN([AC_VFS_UNLINK_IDMAP_ARG], [
	AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/fs.h>
                        #include <linux/mount.h>
                ], [
                    struct mnt_idmap *idmap;
                    vfs_unlink(idmap, NULL, NULL, NULL);
                ], [
                        AC_DEFINE(VFS_UNLINK_HAS_IDMAP_ARG, 1,
                                [VFS_UNLINK has 4 argument, first is mnt_idmap])
                ])
        ])
])
