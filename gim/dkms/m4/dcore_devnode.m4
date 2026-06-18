dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # commit v6.1-rc6-51-gff62b8e6588f
dnl # driver core: make struct class.devnode() take a const *
dnl #
AC_DEFUN([AC_DEVNODE_CONST], [
        AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/kernel.h>
                        #include <linux/types.h>
                        #include <linux/device.h>

                        static char *dcore_devnode(const struct device *dev, umode_t *mode)
                        {
                                return kasprintf(0, "%s", dev_name(dev));
                        }

                ], [
			struct class *dcore_class;
			dcore_class->devnode = dcore_devnode;
                ], [

                        AC_DEFINE(HAVE_DEVNODE_CONST, 1,
                                [class.devnode() take a const *])
                ])
        ])
])
