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

struct amdgv_vf_migration_state {
	enum amdgv_migration_vf_state state;
	bool aborted;
	bool is_target;
};

struct amdgv_live_migration {
	struct amdgv_memmgr_mem		*static_data_mem;
	struct amdgv_memmgr_mem		*dynamic_data_mem;
	mutex_t				lm_lock;
	uint32_t			static_data_size;
	uint32_t			dynamic_data_size;
	uint32_t			migration_version;
	enum amdgv_migration_context_version context_version;
	struct amdgv_vf_migration_state mig_state[AMDGV_MAX_VF_NUM];
};

#define AMDGV_MIGRATION_SHOULD_ABORT(adapt, idx_vf) \
		(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION ? adapt->live_migration.mig_state[idx_vf].aborted : false)
#define AMDGV_MIGRATION_SET_ABORT(adapt, idx_vf) (adapt->live_migration.mig_state[idx_vf].aborted = true)
#define AMDGV_MIGRATION_CLEAR_ABORT(adapt, idx_vf) (adapt->live_migration.mig_state[idx_vf].aborted = false)

int amdgv_migration_get_migration_version(struct amdgv_adapter *adapt,
	uint32_t *migration_version);
int amdgv_migration_get_psp_data_size(struct amdgv_adapter *adapt, uint64_t *size,
	enum amdgv_migration_data_section section);
int amdgv_migration_transfer_manifest_data(struct amdgv_adapter *adapt,
					struct amdgv_sched_event *event);
int amdgv_migration_collect_info(struct amdgv_adapter *adapt);
void amdgv_migration_set_ctx_version(struct amdgv_adapter *adapt,
				     enum amdgv_migration_context_version version);
int amdgv_live_migration_set_vf_mig_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
				   enum amdgv_migration_vf_state state);
void amdgv_live_migration_abort_check(struct amdgv_adapter *adapt, uint32_t idx_vf,
					enum amdgv_sched_event_id event_id);
void amdgv_live_migration_set_abort_all(struct amdgv_adapter *adapt);
#endif
