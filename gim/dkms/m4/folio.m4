dnl *
dnl * Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
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
dnl # v5.16-rc1-49f8275c7d92
dnl # Merge tag 'folio-5.16' of git://git.infradead.org/users/willy/pagecache
dnl #
AC_DEFUN([AC_PAGE_FOLIO], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/mm.h>
			#include <linux/highmem.h>
		], [
			struct page *page = NULL;
			struct folio *folio = page_folio(page);
			unsigned long pfn = folio_pfn(folio);
			size_t size = folio_size(folio);

			(void)pfn;
			(void)size;
		], [
			AC_DEFINE(HAVE_PAGE_FOLIO, 1,
				[page_folio and related folio helpers are available])
		])
	])
])

dnl #
dnl # kmap_local_folio() arrived after the initial folio conversion.
dnl #
AC_DEFUN([AC_KMAP_LOCAL_FOLIO], [
	AC_KERNEL_DO_BACKGROUND([
		AC_KERNEL_TRY_COMPILE([
			#include <linux/mm.h>
			#include <linux/highmem.h>
		], [
			struct folio *folio = NULL;
			void *addr = kmap_local_folio(folio, 0);

			kunmap_local(addr);
		], [
			AC_DEFINE(HAVE_KMAP_LOCAL_FOLIO, 1,
				[kmap_local_folio and kunmap_local are available])
		])
	])
])
