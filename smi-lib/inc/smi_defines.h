/*
 * Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __SMI_DEFINES_H__
#define __SMI_DEFINES_H__

#include "common/smi_handle.h"
#include "amdsmi.h"

#define AMDSMI_UNUSED(x) do { (void)(x); } while (0)

enum smi_file_access_mode {
	SMI_READONLY = 0,
	SMI_RDWR = 1
};

extern smi_handle_struct g_smi_handle;

#ifdef THREAD_SAFE
extern smi_once_t smi_init_flag;
extern smi_tss_t smi_thread_key;
void smi_free_handle(void *thread);
#ifdef _WIN64
BOOL init_smi_once(PINIT_ONCE InitOnce, PVOID Parameter, PVOID * lpContext);
#else
void init_smi_once(void);
#endif
#define AMDSMI_GET_HANDLE                                                                    \
	do {                                                                                  \
		smi_run_once(&smi_init_flag, init_smi_once);                                  \
		smi_mutex_lock(&g_smi_handle.lock);                                           \
		smi_req.handle = &g_smi_handle;                                               \
		smi_req.thread = smi_tss_get(smi_thread_key);                                 \
		if (smi_req.thread == NULL) {                                                 \
			smi_req.thread = calloc(1, sizeof(smi_thread_ctx));                   \
			if (smi_req.thread == NULL) {                                         \
				smi_mutex_unlock(&g_smi_handle.lock);                         \
				return AMDSMI_STATUS_OUT_OF_RESOURCES;                                        \
			}                                                                     \
		}                                                                             \
	} while (0)

#define AMDSMI_ESCAPE_IF_NOT_INIT_ON_FINI                                                    \
	do {                                                                                  \
		smi_run_once(&smi_init_flag, init_smi_once);                                  \
		smi_mutex_lock(&g_smi_handle.lock);                                           \
		if (!g_smi_handle.init) {                                                     \
			smi_mutex_unlock(&g_smi_handle.lock);                                 \
			return AMDSMI_STATUS_SUCCESS;                                                   \
		}                                                                             \
		smi_req.handle = &g_smi_handle;                                               \
		smi_req.thread = smi_tss_get(smi_thread_key);                                 \
	} while (0)

#define AMDSMI_ESCAPE_IF_NOT_INIT                                                            \
	do {                                                                                  \
		smi_run_once(&smi_init_flag, init_smi_once);                                  \
		smi_mutex_lock(&g_smi_handle.lock);                                           \
		if (!g_smi_handle.init) {                                                     \
			smi_mutex_unlock(&g_smi_handle.lock);                                 \
			SMI_ERROR("Call to %s failed. Handle not initialized. Return code: %d",               \
				  __FUNCTION__, AMDSMI_STATUS_NOT_INIT);                                              \
			return AMDSMI_STATUS_NOT_INIT;                                           \
		}                                                                             \
		smi_req.handle = &g_smi_handle;                                               \
		smi_req.thread = smi_tss_get(smi_thread_key);                                 \
																						\
		if (smi_req.thread == NULL) {                                                 \
			smi_req.thread = calloc(1, sizeof(smi_thread_ctx));                   \
			if (smi_req.thread == NULL) {                                         \
				smi_mutex_unlock(&g_smi_handle.lock);                         \
				return AMDSMI_STATUS_OUT_OF_RESOURCES;                                        \
			}                                                                     \
																					\
			smi_tss_set(smi_thread_key, smi_req.thread);                          \
		}                                                                             \
																					\
		smi_mutex_unlock(&g_smi_handle.lock);                                         \
	} while (0)

#define AMDSMI_HANDLE_UNLOCK                                                                 \
	do {                                                                                  \
		smi_mutex_unlock(&g_smi_handle.lock);                                         \
	} while (0)

#define AMDSMI_HANDLE_UNLOCK_AND_FREE                                                        \
	do {                                                                                  \
		if (smi_tss_get(smi_thread_key) != smi_req.thread) {                          \
			free(smi_req.thread);                                                 \
		}                                                                             \
		smi_req.thread = NULL;                                                        \
		smi_mutex_unlock(&g_smi_handle.lock);                                         \
	} while (0)

#define AMDSMI_HANDLE_SET                                                                    \
	do {                                                                                  \
		smi_tss_set(smi_thread_key, smi_req.thread);                                  \
		smi_mutex_unlock(&g_smi_handle.lock);                                         \
	} while (0);
#else
extern smi_thread_ctx g_smi_thread;
#define AMDSMI_GET_HANDLE                                                                    \
	do {                                                                                  \
		smi_req.handle = &g_smi_handle;                                               \
		smi_req.thread = &g_smi_thread;                                               \
	} while (0)

#define AMDSMI_HANDLE_UNLOCK
#define AMDSMI_HANDLE_UNLOCK_AND_FREE
#define AMDSMI_HANDLE_SET

#define AMDSMI_ESCAPE_IF_NOT_INIT_ON_FINI                                                    \
	do {                                                                                  \
		smi_req.handle = &g_smi_handle;                                               \
		smi_req.thread = &g_smi_thread;                                               \
		if (!smi_req.handle->init) {                                                  \
			return AMDSMI_STATUS_SUCCESS;                                         \
		}                                                                             \
	} while (0)

#define AMDSMI_ESCAPE_IF_NOT_INIT                                                            \
	do {                                                                                  \
		smi_req.handle = &g_smi_handle;                                               \
		smi_req.thread = &g_smi_thread;                                               \
		if (!smi_req.handle->init) {                                                  \
			SMI_ERROR("Call to %s failed. Handle not initialized. Return code: %d",               \
				  __FUNCTION__, AMDSMI_STATUS_NOT_INIT);                                              \
			return AMDSMI_STATUS_NOT_INIT;                                           \
		}                                                                             \
	} while (0)

#endif

#endif // __SMI_DEFINES_H__
