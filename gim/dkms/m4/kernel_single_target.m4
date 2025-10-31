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
dnl # extract cc, cflags, cppflags
dnl #
AC_DEFUN([AC_KERNEL_SINGLE_TARGET_CFLAGS], [
	AS_IF([test -s .conftest.o.cmd], [
		_conftest_cmd=$(head -1 .conftest.o.cmd)

		CC=$(echo $_conftest_cmd | awk -F ' ' '{print $[3]}')

		CFLAGS=$(echo $_conftest_cmd | \
			 cut -d ' ' -f 4- | \
			 sed -e "s|\./|${LINUX_OBJ}/|g" \
			     -e "s|-I\([[[a-z]]]*\)|-I${LINUX_OBJ}/\1|g" \
			     -e "s|-include \([[[a-z]]]*\)|-include ${LINUX_OBJ}/\1|g" \
			     -e "s|$PWD|\${PWD}|g")

		CPPFLAGS=$(echo $CFLAGS | \
			   cut -d ';' -f 1 | \
			   sed 's| -|\n&|g' | \
			   sed -n -e '/conftest/d' \
				  -e '/KBUILD/d' \
				  -e '/-I/p; /-include/p; /-isystem/p; /-D/p' | \
			   xargs)

		AC_SUBST(CC)
		AC_SUBST(CFLAGS)
		AC_SUBST(CPPFLAGS)
	], [
		AC_MSG_ERROR([cannot detect CFLAGS...])
	])
])

dnl #
dnl # v5.3-rc4-54-g54b8ae66ae1a
dnl # kbuild: change *FLAGS_<basetarget>.o to take the path relative to $(obj)
dnl #
AC_DEFUN([AC_KERNEL_FLAGS_TAKE_PATH], [
	AS_IF([grep -qsm 1 "target-stem" ${LINUX}/scripts/Makefile.lib], [
		AC_DEFINE(HAVE_AMDKCL_FLAGS_TAKE_PATH, 1,
			[*FLAGS_<basetarget>.o support to take the path relative to $(obj)])
	])
])

dnl #
dnl # v4.20-rc2-10-ge07db28eea38
dnl # kbuild: fix single target build for external module
dnl #
AC_DEFUN([AC_KERNEL_SINGLE_TARGET], [
	AC_KERNEL_TMP_BUILD_DIR([
		AC_KERNEL_TRY_COMPILE_MODULE([], [], [], [
			SINGLE_TARGET_BUILD_MODVERDIR=.tmp_versions
			AS_IF([test ! -d $SINGLE_TARGET_BUILD_MODVERDIR], [
				SINGLE_TARGET_BUILD_NO_TMP_VERSIONS=1
			], [
				AC_MSG_WARN([compile single target fail expectedly])
			])
		])
		AC_KERNEL_SINGLE_TARGET_CFLAGS
		AC_KERNEL_FLAGS_TAKE_PATH
	])
])
