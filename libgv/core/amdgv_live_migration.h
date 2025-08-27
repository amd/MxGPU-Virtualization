/*
 * Copyright 2022-2023 Advanced Micro Devices, Inc.
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

#ifndef AMDGV_LIVE_MIGRATION_H
#define AMDGV_LIVE_MIGRATION_H

struct amdgv_live_migration {
	struct amdgv_memmgr_mem		*static_data_mem;
	struct amdgv_memmgr_mem		*dynamic_data_mem;
	mutex_t				lm_lock;
	uint32_t			static_data_size;
	uint32_t			dynamic_data_size;
	uint32_t			migration_version;
	int32_t				migration_status;
	enum amdgv_migration_context_version context_version;
};

int amdgv_migration_get_migration_version(struct amdgv_adapter *adapt,
	uint32_t *migration_version);
int amdgv_migration_get_psp_data_size(struct amdgv_adapter *adapt, uint64_t *size,
	enum amdgv_migration_data_section section);
int amdgv_migration_transfer_manifest_data(struct amdgv_adapter *adapt,
					struct amdgv_sched_event *event);
int amdgv_migration_init(struct amdgv_adapter *adapt);
void amdgv_migration_fini(struct amdgv_adapter *adapt);
int amdgv_migration_collect_info(struct amdgv_adapter *adapt);
void amdgv_migration_set_ctx_version(struct amdgv_adapter *adapt,
				     enum amdgv_migration_context_version version);
#endif
