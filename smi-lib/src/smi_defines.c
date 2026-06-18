/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include "smi_defines.h"

smi_handle_struct g_smi_handle;

#ifdef THREAD_SAFE
SMI_DEFINE_ONCE(smi_init_flag);

smi_tss_t smi_thread_key;

void smi_free_handle(void *thread)
{
#ifdef AMD_SMI_NIC_SUPPORT
	smi_thread_ctx *ctx = (smi_thread_ctx *)thread;
	if (ctx && ctx->nic_init && ctx->nic_ctx) {
		smi_nic_destroy_context(ctx->nic_ctx);
		ctx->nic_ctx = NULL;
		ctx->nic_init = false;
	}
#endif
	free(thread);
}

static void cleanup(void)
{
	void *thread = smi_tss_get(smi_thread_key);
	free(thread);
	smi_tss_set(smi_thread_key, NULL);
	smi_mutex_destroy(&g_smi_handle.lock);
}
#ifdef _WIN64
BOOL init_smi_once(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext)
{
	if (smi_tss_create(&smi_thread_key, smi_free_handle) == 0) {
		g_smi_handle.init = false;
		smi_mutex_init(&g_smi_handle.lock);
		atexit(cleanup);
		return TRUE;
	}

	return FALSE;
}
#else
void init_smi_once(void)
{
	if (smi_tss_create(&smi_thread_key, smi_free_handle) == 0) {
		g_smi_handle.init = false;
		smi_mutex_init(&g_smi_handle.lock);
		atexit(cleanup);
	}
}
#endif

#else
smi_thread_ctx g_smi_thread;
#endif
