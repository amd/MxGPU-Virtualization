dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

dnl #
dnl # v2.6.21-4001-gf34c506b0385
dnl # declare struct ktime
dnl #
AC_DEFUN([AC_RTC_KTIME_TO_TM], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/ktime.h>
			#include <linux/rtc.h>
		], [
			rtc_ktime_to_tm((uint64_t) (0));
		], [
			AC_DEFINE(HAVE_RTC_KTIME_TO_TM, 1,
				[rtc_ktime_to_tm has uint64_t argument type])
		])
	])
])
