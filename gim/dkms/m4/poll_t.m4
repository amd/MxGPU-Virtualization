dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # commit v4.15-2176-g168fe32a072a
dnl # Pull poll annotations from Al Viro
dnl #
AC_DEFUN([AC_POLL_T], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/anon_inodes.h>
			#include <linux/poll.h>
			#include <linux/fs.h>
		], [
			__poll_t event(struct file *, struct poll_table_struct *);

			static const struct file_operations smi_event_fops = {
				.poll = event,
			};

			anon_inode_getfile("", &smi_event_fops, NULL, O_RDONLY);
		], [
			AC_DEFINE(HAVE_POLL_T, 1,
				[smi_event_poll has poll_t return value])
		])
	])
])
