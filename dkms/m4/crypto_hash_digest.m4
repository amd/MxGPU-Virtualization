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
dnl # commit v4.5-rc1-75-g896545098777
dnl # crypto: hash - Remove crypto_hash interface
dnl #
AC_DEFUN([AC_CRYPTO_HASH_DIGEST], [
        AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/crypto.h>
                        #include <crypto/hash.h>
                ], [
                        crypto_hash_digest(NULL, NULL, 0,0);
                ], [
                        AC_DEFINE(HAVE_CRYPTO_HASH_DIGEST, 1,
                                [crypto_hash_digest is available])
                ])
        ])
])

dnl #
dnl # commit v5.1-rc1-119-g877b5691f27a
dnl # crypto: shash - remove shash_desc::flags
dnl #
AC_DEFUN([AC_SHASH_DESC_FLAGS], [
        AC_KERNEL_DO_BACKGROUND([
                AC_KERNEL_TRY_COMPILE([
                        #include <linux/crypto.h>
                        #include <crypto/hash.h>
                ], [
                        struct shash_desc *desc;
                        desc->flags = true;
                ], [
                        AC_DEFINE(HAVE_SHASH_DESC_FLAGS, 1,
                                [flags are available])
                ])
        ])
])
