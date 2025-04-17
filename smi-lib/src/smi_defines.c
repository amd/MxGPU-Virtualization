/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include <stdlib.h>
#include "smi_defines.h"

smi_handle_struct g_smi_handle;

#ifdef THREAD_SAFE
SMI_DEFINE_ONCE(smi_init_flag);

smi_tss_t smi_thread_key;

void smi_free_handle(void *thread)
{
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
