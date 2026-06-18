/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_THREAD_H__
#define __SMI_THREAD_H__

/* ESXi build (on CentOS7) or Linux without C11 threads use pthread */
#if defined(SMI_ESXI_BUILD) || \
    (defined(__linux__) && \
     (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201102L) || defined(__STDC_NO_THREADS__)))

#include <pthread.h>

typedef pthread_once_t	smi_once_t;
typedef pthread_key_t	smi_tss_t;
typedef pthread_mutex_t smi_mutex_t;

#define SMI_DEFINE_ONCE(a) smi_once_t a = PTHREAD_ONCE_INIT;

#define smi_run_once	     pthread_once
#define smi_tss_create	     pthread_key_create
#define smi_tss_exit	     pthread_exit
#define smi_tss_get	     pthread_getspecific
#define smi_tss_set	     pthread_setspecific
#define smi_mutex_init(a)    pthread_mutex_init(a, NULL);
#define smi_mutex_lock(a)    pthread_mutex_lock(a)
#define smi_mutex_unlock(a)  pthread_mutex_unlock(a)
#define smi_mutex_destroy(a) pthread_mutex_destroy(a)

/* Linux with C11 threads support */
#elif defined(__linux__)

#include <threads.h>

typedef once_flag smi_once_t;
typedef tss_t	  smi_tss_t;
typedef mtx_t	  smi_mutex_t;

#define SMI_DEFINE_ONCE(a) smi_once_t a = ONCE_FLAG_INIT

#define smi_run_once	     call_once
#define smi_tss_create	     tss_create
#define smi_tss_exit	     thrd_exit
#define smi_tss_get	     tss_get
#define smi_tss_set	     tss_set
#define smi_mutex_init(a)    mtx_init(a, mtx_plain);
#define smi_mutex_lock(a)    mtx_lock(a)
#define smi_mutex_unlock(a)  mtx_unlock(a)
#define smi_mutex_destroy(a) mtx_destroy(a)

/* Windows */
#else

#include "smi_os_defines.h"

#define smi_run_once	     smi_thread_once
#define smi_tss_create	     smi_fls_create
#define smi_tss_exit	     ExitThread
#define smi_tss_get	     smi_fls_get
#define smi_tss_set	     smi_fls_set
#define smi_mutex_init(a)    smi_mtx_init(a)
#define smi_mutex_lock(a)    smi_mtx_lock(a)
#define smi_mutex_unlock(a)  smi_mtx_unlock(a)
#define smi_mutex_destroy(a) smi_mtx_destroy(a)

#endif // os check
#endif // __SMI_THREAD_H__
