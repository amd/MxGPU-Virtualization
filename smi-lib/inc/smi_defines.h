/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

#ifdef AMD_SMI_NIC_SUPPORT
#define AMDSMI_INIT_NIC_CONTEXT \
	do { \
		if (!smi_req.thread->nic_init) { \
			int ret = smi_nic_create_context(&smi_req.thread->nic_ctx); \
			if (ret != SMI_NIC_STATUS_SUCCESS) { \
				smi_req.thread->nic_ctx = NULL; \
				smi_req.thread->nic_init = false; \
				smi_mutex_unlock(&g_smi_handle.lock); \
			} \
			smi_req.thread->nic_init = true; \
		} \
	} while (0);
#else
#define AMDSMI_INIT_NIC_CONTEXT
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
				return AMDSMI_STATUS_OUT_OF_RESOURCES;                        \
			}                                                                     \
			smi_tss_set(smi_thread_key, smi_req.thread);                          \
		}                                                                             \
		AMDSMI_INIT_NIC_CONTEXT;                                                      \
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
		AMDSMI_INIT_NIC_CONTEXT                                             \
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

#ifdef AMD_SMI_NIC_SUPPORT
#define AMDSMI_INIT_NIC_CONTEXT_NON_THREAD_SAFE \
	do { \
		if (!smi_req.thread->nic_init) { \
			int ret = smi_nic_create_context(&smi_req.thread->nic_ctx);  \
			if (ret != SMI_NIC_STATUS_SUCCESS) { \
				smi_req.thread->nic_ctx = NULL; \
				smi_req.thread->nic_init = false; \
			} \
			smi_req.thread->nic_init = true; \
		} \
	} while (0);
#else
#define AMDSMI_INIT_NIC_CONTEXT_NON_THREAD_SAFE
#endif

#define AMDSMI_GET_HANDLE                                                                    \
	do {                                                                                  \
		smi_req.handle = &g_smi_handle;                                               \
		smi_req.thread = &g_smi_thread;                                               \
		AMDSMI_INIT_NIC_CONTEXT_NON_THREAD_SAFE;                                       \
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
