dnl * Copyright Advanced Micro Devices, Inc.
dnl *
dnl * SPDX-License-Identifier: MIT

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
                        struct hash_desc desc;
                        desc.flags = CRYPTO_TFM_REQ_MAY_SLEEP;
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
