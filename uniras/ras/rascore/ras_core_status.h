/* SPDX-License-Identifier: MIT */
/*
 * Copyright 2025 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __RAS_CORE_STATUS_H__
#define __RAS_CORE_STATUS_H__

#define RAS_CORE_EPERM      1 /* Operation not permitted */
#define RAS_CORE_ENOENT     2 /* No such file or directory */
#define RAS_CORE_ESRCH      3 /* No such process */
#define RAS_CORE_EINTR      4 /* Interrupted system call */
#define RAS_CORE_EIO        5 /* I/O error */
#define RAS_CORE_ENXIO      6 /* No such device or address */
#define RAS_CORE_E2BIG      7 /* Argument list too long */
#define RAS_CORE_ENOEXEC    8 /* Exec format error */
#define RAS_CORE_EBADF      9 /* Bad file number */
#define RAS_CORE_ECHILD     10 /* No child processes */
#define RAS_CORE_EAGAIN     11 /* Try again */
#define RAS_CORE_ENOMEM     12 /* Out of memory */
#define RAS_CORE_EACCES     13 /* Permission denied */
#define RAS_CORE_EFAULT     14 /* Bad address */
#define RAS_CORE_ENOTBLK    15 /* Block device required */
#define RAS_CORE_EBUSY      16 /* Device or resource busy */
#define RAS_CORE_EEXIST     17 /* File exists */
#define RAS_CORE_EXDEV      18 /* Cross-device link */
#define RAS_CORE_ENODEV     19 /* No such device */
#define RAS_CORE_ENOTDIR    20 /* Not a directory */
#define RAS_CORE_EISDIR     21 /* Is a directory */
#define RAS_CORE_EINVAL     22 /* Invalid argument */
#define RAS_CORE_ENFILE     23 /* File table overflow */
#define RAS_CORE_EMFILE     24 /* Too many open files */
#define RAS_CORE_ENOTTY     25 /* Not a typewriter */
#define RAS_CORE_ETXTBSY    26 /* Text file busy */
#define RAS_CORE_EFBIG      27 /* File too large */
#define RAS_CORE_ENOSPC     28 /* No space left on device */
#define RAS_CORE_ESPIPE     29 /* Illegal seek */
#define RAS_CORE_EROFS      30 /* Read-only file system */
#define RAS_CORE_EMLINK     31 /* Too many links */
#define RAS_CORE_EPIPE      32 /* Broken pipe */
#define RAS_CORE_EDOM       33 /* Math argument out of domain of func */
#define RAS_CORE_ERANGE     34 /* Math result not representable */

#define RAS_CORE_EBFONT     59 /* Bad font file format */
#define RAS_CORE_ENOSTR     60 /* Device not a stream */
#define RAS_CORE_ENODATA    61 /* No data available */
#define RAS_CORE_ETIME      62 /* Timer expired */
#define RAS_CORE_ENOSR      63 /* Out of streams resources */
#define RAS_CORE_ENONET     64 /* Machine is not on the network */
#define RAS_CORE_ENOPKG     65 /* Package not installed */
#define RAS_CORE_EREMOTE    66 /* Object is remote */
#define RAS_CORE_ENOLINK    67 /* Link has been severed */
#define RAS_CORE_EADV       68 /* Advertise error */
#define RAS_CORE_ESRMNT     69 /* Srmount error */
#define RAS_CORE_ECOMM      70 /* Communication error on send */
#define RAS_CORE_EPROTO     71 /* Protocol error */
#define RAS_CORE_EMULTIHOP  72 /* Multihop attempted */
#define RAS_CORE_EDOTDOT    73 /* RFS specific error */
#define RAS_CORE_EBADMSG    74 /* Not a data message */
#define RAS_CORE_EOVERFLOW  75 /* Value too large for defined data type */
#define RAS_CORE_ENOTUNIQ   76 /* Name not unique on network */
#define RAS_CORE_EBADFD     77 /* File descriptor in bad state */
#define RAS_CORE_EREMCHG    78 /* Remote address changed */
#define RAS_CORE_ELIBACC    79 /* Can not access a needed shared library */
#define RAS_CORE_ELIBBAD    80 /* Accessing a corrupted shared library */
#define RAS_CORE_ELIBSCN    81 /* .lib section in a.out corrupted */
#define RAS_CORE_ELIBMAX    82 /* Attempting to link in too many shared libraries */
#define RAS_CORE_ELIBEXEC   83 /* Cannot exec a shared library directly */
#define RAS_CORE_EILSEQ     84 /* Illegal byte sequence */
#define RAS_CORE_ERESTART   85 /* Interrupted system call should be restarted */
#define RAS_CORE_ESTRPIPE   86 /* Streams pipe error */
#define RAS_CORE_EUSERS     87 /* Too many users */
#define RAS_CORE_ENOTSOCK   88 /* Socket operation on non-socket */
#define RAS_CORE_EDESTADDRREQ    89 /* Destination address required */
#define RAS_CORE_EMSGSIZE        90 /* Message too long */
#define RAS_CORE_EPROTOTYPE      91 /* Protocol wrong type for socket */
#define RAS_CORE_ENOPROTOOPT     92 /* Protocol not available */
#define RAS_CORE_EPROTONOSUPPORT 93 /* Protocol not supported */
#define RAS_CORE_ESOCKTNOSUPPORT 94 /* Socket type not supported */
#define RAS_CORE_EOPNOTSUPP 95 /* Operation not supported on transport endpoint */
#define RAS_CORE_EPFNOSUPPORT    96 /* Protocol family not supported */
#define RAS_CORE_EAFNOSUPPORT    97 /* Address family not supported by protocol */
#define RAS_CORE_EADDRINUSE      98 /* Address already in use */
#define RAS_CORE_EADDRNOTAVAIL   99 /* Cannot assign requested address */
#define RAS_CORE_ENETDOWN        100 /* Network is down */
#define RAS_CORE_ENETUNREACH     101 /* Network is unreachable */
#define RAS_CORE_ENETRESET       102 /* Network dropped connection because of reset */
#define RAS_CORE_ECONNABORTED    103 /* Software caused connection abort */
#define RAS_CORE_ECONNRESET      104 /* Connection reset by peer */
#define RAS_CORE_ENOBUFS         105 /* No buffer space available */
#define RAS_CORE_EISCONN         106 /* Transport endpoint is already connected */
#define RAS_CORE_ENOTCONN        107 /* Transport endpoint is not connected */
#define RAS_CORE_ESHUTDOWN       108 /* Cannot send after transport endpoint shutdown */
#define RAS_CORE_ETOOMANYREFS    109 /* Too many references: cannot splice */
#define RAS_CORE_ETIMEDOUT       110 /* Connection timed out */
#define RAS_CORE_ECONNREFUSED    111 /* Connection refused */
#define RAS_CORE_EHOSTDOWN       112 /* Host is down */
#define RAS_CORE_EHOSTUNREACH    113 /* No route to host */
#define RAS_CORE_EALREADY        114 /* Operation already in progress */
#define RAS_CORE_EINPROGRESS     115 /* Operation now in progress */
#define RAS_CORE_ESTALE          116 /* Stale file handle */
#define RAS_CORE_EUCLEAN         117 /* Structure needs cleaning */
#define RAS_CORE_ENOTNAM         118 /* Not a XENIX named type file */
#define RAS_CORE_ENAVAIL         119 /* No XENIX semaphores available */
#define RAS_CORE_EISNAM          120 /* Is a named type file */
#define RAS_CORE_EREMOTEIO       121 /* Remote I/O error */
#define RAS_CORE_EDQUOT          122 /* Quota exceeded */

#define RAS_CORE_ENOMEDIUM      123 /* No medium found */
#define RAS_CORE_EMEDIUMTYPE    124 /* Wrong medium type */
#define RAS_CORE_ECANCELED      125 /* Operation Canceled */
#define RAS_CORE_ENOKEY         126 /* Required key not available */
#define RAS_CORE_EKEYEXPIRED    127 /* Key has expired */
#define RAS_CORE_EKEYREVOKED    128 /* Key has been revoked */
#define RAS_CORE_EKEYREJECTED   129 /* Key was rejected by service */

#define RAS_CORE_EOWNERDEAD      130 /* Owner died */
#define RAS_CORE_ENOTRECOVERABLE 131 /* State not recoverable */
#define RAS_CORE_ERFKILL         132 /* Operation not possible due to RF-kill */
#define RAS_CORE_EHWPOISON       133 /* Memory page has hardware error */

#define RAS_CORE_OK                       0
#define RAS_CORE_NOT_SUPPORTED            248
#define RAS_CORE_FAIL_ERROR_QUERY         249
#define RAS_CORE_FAIL_ERROR_INJECTION     250
#define RAS_CORE_FAIL_FATAL_RECOVERY      251
#define RAS_CORE_FAIL_POISON_CONSUMPTION  252
#define RAS_CORE_FAIL_POISON_CREATION     253
#define RAS_CORE_FAIL_NO_VALID_BANKS      254
#define RAS_CORE_GPU_IN_MODE1_RESET       255
#endif
