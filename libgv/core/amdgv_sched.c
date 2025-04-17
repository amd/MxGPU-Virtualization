/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "amdgv_device.h"
#include "amdgv_sched_internal.h"

static const uint32_t this_block = AMDGV_SCHEDULER_BLOCK;

/* Sched blocks are legacy descriptors for logical schedulers.
 * With introduction to spatial partition, each partition (VF) has
 * it's own instance of a sched_block type.
 */
static struct amdgv_id_name amdgv_sched_names[] = {
	{ AMDGV_SCHED_BLOCK_GFX, "GFX" },   { AMDGV_SCHED_BLOCK_UVD, "UVD" },
	{ AMDGV_SCHED_BLOCK_VCE, "VCE" },   { AMDGV_SCHED_BLOCK_UVD1, "UVD1" },
	{ AMDGV_SCHED_BLOCK_VCN, "VCN" },   { AMDGV_SCHED_BLOCK_VCN1, "VCN1" },
	{ AMDGV_SCHED_BLOCK_JPEG, "JPEG" }, { AMDGV_SCHED_BLOCK_ALL, "ALL ENGINES" }
};

static void amdgv_sched_vf_map_clear(struct amdgv_sched_vf_info *vf)
{
	uint32_t sched_block;

	vf->world_switch_mask = 0;
	vf->hw_sched_mask = 0;
	vf->xcc_mask = 0;

	for (sched_block = 0; sched_block < AMDGV_SCHED_BLOCK_MAX; sched_block++) {
		vf->block_map[sched_block].world_switch_mask = 0;
		vf->block_map[sched_block].hw_sched_mask = 0;
	}
}

static void amdgv_sched_vf_map_add(struct amdgv_adapter *adapt,
				   struct amdgv_sched_vf_info *vf,
				   struct amdgv_sched_spatial_part *spatial_part)
{
	uint32_t sched_block, id;

	vf->world_switch_mask |= spatial_part->world_switch_mask;
	vf->hw_sched_mask |= spatial_part->hw_sched_mask;
	vf->xcc_mask |= spatial_part->xcc_mask;

	for_each_id (id, vf->hw_sched_mask) {
		sched_block = adapt->gpuiov.ctrl_blocks[id].sched_block;
		vf->block_map[sched_block].hw_sched_mask |= BIT(id);
	}

	for_each_id (id, vf->world_switch_mask) {
		sched_block = adapt->sched.world_switch[id].sched_block;
		vf->block_map[sched_block].world_switch_mask |= BIT(id);
	}
}

int amdgv_sched_part_mapping_init(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf, hw_sched_mask, i, j;
	struct amdgv_sched_world_switch *world_switch;
	struct amdgv_sched_spatial_part *spatial_part;

	/* In case we are coming from a set_vf_num API call,
	 * we need to clear the previous mapping data.
	 *
	 * spatial_part[] has already been cleared inside ASIC
	 * sched backend code when a new ASIC specific
	 * static partition table is applied
	 */
	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++)
		amdgv_sched_vf_map_clear(&adapt->sched.array_vf[idx_vf]);

	for (i = 0; i < adapt->sched.num_spatial_partitions; i++) {
		spatial_part = &adapt->sched.spatial_part[i];
		hw_sched_mask = spatial_part->hw_sched_mask;
		for (j = 0; j < adapt->sched.num_world_switch; j++) {
			world_switch = &adapt->sched.world_switch[j];
			if (!(hw_sched_mask & world_switch->hw_sched_mask))
				continue;
			spatial_part->world_switch_mask |= BIT(j);
			spatial_part->ws_map[world_switch->sched_block] = world_switch;
		}
		for_each_id (idx_vf, amdgv_sched_get_vf_mask_by_part(adapt, i)) {
			amdgv_sched_vf_map_add(adapt, &adapt->sched.array_vf[idx_vf], spatial_part);
		}
	}

	return 0;
}

int amdgv_sched_init(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;

	AMDGV_INFO("Number of VFs per GFX scheduler block: 0x%x\n",
		   adapt->sched.num_vf_per_gfx_sched);

	if (adapt->flags & AMDGV_FLAG_USE_PF &&
		adapt->flags & AMDGV_FLAG_WS_RECORD &&
		adapt->opt.gfx_sched_mode > AMDGV_SCHED_BEGIN &&
		adapt->opt.gfx_sched_mode <= AMDGV_SCHED_MAX_HW_SCHED_MODE)
		adapt->flags |= AMDGV_FLAG_DEBUG_DUMP_ENABLE;

	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++)
		adapt->sched.array_vf[idx_vf].state = AMDGV_SCHED_UNAVAL;
#ifdef WS_RECORD
	if (amdgv_sched_record_queue_process_init(adapt)) {
		AMDGV_ERROR("record process init failed\n");
		amdgv_sched_record_queue_process_fini(adapt);
	}
#endif

	if (amdgv_sched_world_switch_init(adapt)) {
		return AMDGV_FAILURE;
	}

	if (amdgv_sched_part_mapping_init(adapt)) {
		return AMDGV_FAILURE;
	}

	adapt->reset.pf_rel_gpu_init = oss_event_init();
	if (adapt->reset.pf_rel_gpu_init == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_EVENT_FAIL, 0);
		return AMDGV_FAILURE;
	}

	if (amdgv_sched_event_queue_process_init(adapt)) {
		amdgv_sched_world_switch_fini(adapt);
		return AMDGV_FAILURE;
	}

	return 0;
}

void amdgv_sched_fini(struct amdgv_adapter *adapt)
{
	amdgv_sched_event_queue_process_fini(adapt);

	if (adapt->reset.pf_rel_gpu_init)
		oss_event_fini(adapt->reset.pf_rel_gpu_init);

	adapt->reset.pf_rel_gpu_init = OSS_INVALID_HANDLE;
	/* Only sched_suspend(), sched_lock() can disable the scheduler
	 * Ensure it is stopped completely before tearing it down
	 */
	if (!adapt->fini_opt.skip_hw_fini)
		amdgv_sched_stop_all(adapt);

	amdgv_sched_world_switch_fini(adapt);
#ifdef WS_RECORD
	amdgv_sched_record_queue_process_fini(adapt);
#endif
}

int amdgv_sched_queue_remove_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return amdgv_sched_queue_event_and_wait(adapt, idx_vf, AMDGV_EVENT_SCHED_REMOVE_VF,
						AMDGV_SCHED_BLOCK_ALL);
}

int amdgv_sched_queue_flr_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return amdgv_sched_queue_event_and_wait(
		adapt, idx_vf, AMDGV_EVENT_SCHED_FORCE_RESET_VF, AMDGV_SCHED_BLOCK_ALL);
}

int amdgv_sched_queue_stop_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return amdgv_sched_queue_event_and_wait(adapt, idx_vf, AMDGV_EVENT_SCHED_STOP_VF,
						AMDGV_SCHED_BLOCK_ALL);
}

int amdgv_sched_queue_init_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	union amdgv_sched_event_data data;

	data.vf_fb_data.pattern = 0x00;
	data.vf_fb_data.flag = 0;

	/* Do not queue event during whole GPU reset */
	if (adapt->reset.reset_state) {
		return 0;
	}

	/* To program using the PSP interface,
	 *    we need to be in the PF context
	 * And to set the active context as PF,
	 *    We cannot just stop the world switch and switch to PF
	 * (since event queue handler can change the state in another thread)
	 *    We need to queue an event and wait
	 *    o the event will stop the world switch and switch to PF
	 *    o and then issue PSP commands.
	 */
	return amdgv_sched_queue_event_and_wait_ex(adapt, idx_vf, AMDGV_EVENT_SCHED_INIT_VF_FB,
						   AMDGV_SCHED_BLOCK_GFX, data);
}

int amdgv_sched_queue_force_reset_gpu(struct amdgv_adapter *adapt)
{
	if (adapt->xgmi.master_adapt) {
		AMDGV_INFO("Forwarding reset event to master adapter:0x%x\n",
			   adapt->xgmi.master_adapt->bdf);
		adapt = adapt->xgmi.master_adapt;
	}

	return amdgv_sched_queue_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_FORCE_RESET_GPU,
						0);
}

int amdgv_sched_queue_suspend_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return amdgv_sched_queue_event_and_wait(adapt, idx_vf, AMDGV_EVENT_SCHED_SUSPEND_VF,
						AMDGV_SCHED_BLOCK_ALL);
}

int amdgv_sched_queue_resume_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return amdgv_sched_queue_event_and_wait(adapt, idx_vf, AMDGV_EVENT_SCHED_RESUME_VF,
						AMDGV_SCHED_BLOCK_ALL);
}

int amdgv_sched_queue_suspend(struct amdgv_adapter *adapt)
{
	return amdgv_sched_queue_event_and_wait(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_SUSPEND,
						AMDGV_SCHED_BLOCK_ALL);
}

int amdgv_sched_queue_resume(struct amdgv_adapter *adapt)
{
	return amdgv_sched_queue_event_and_wait(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_RESUME,
						AMDGV_SCHED_BLOCK_ALL);
}

int amdgv_sched_queue_suspend_ex(struct amdgv_adapter *adapt, struct amdgv_lock_sched_opt opt)
{
	if (opt.is_live_update)
		return amdgv_sched_queue_event_and_wait(adapt, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_SUSPEND_LIVE,
							AMDGV_SCHED_BLOCK_ALL);
	else
		return amdgv_sched_queue_event_and_wait(adapt, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_SUSPEND,
							AMDGV_SCHED_BLOCK_ALL);
}

int amdgv_sched_queue_resume_ex(struct amdgv_adapter *adapt, struct amdgv_lock_sched_opt opt)
{
	if (opt.is_live_update)
		return amdgv_sched_queue_event_and_wait(adapt, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_RESUME_LIVE,
							AMDGV_SCHED_BLOCK_ALL);
	else
		return amdgv_sched_queue_event_and_wait(adapt, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_RESUME,
							AMDGV_SCHED_BLOCK_ALL);
}

uint32_t amdgv_sched_get_vf_mask_by_part(struct amdgv_adapter *adapt, uint32_t idx_part)
{
	return adapt->sched.spatial_part[idx_part].idx_vf_mask;
}

uint32_t amdgv_sched_get_world_switch_mask(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return adapt->sched.array_vf[idx_vf].world_switch_mask;
}

uint32_t amdgv_sched_get_world_switch_mask_by_sched_block(struct amdgv_adapter *adapt,
							  uint32_t idx_vf,
							  enum amdgv_sched_block sched_block)
{
	if (sched_block == AMDGV_SCHED_BLOCK_ALL)
		return adapt->sched.array_vf[idx_vf].world_switch_mask;
	else
		return adapt->sched.array_vf[idx_vf].block_map[sched_block].world_switch_mask;
}

uint32_t amdgv_sched_get_hw_sched_mask_by_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return adapt->sched.array_vf[idx_vf].hw_sched_mask;
}

uint32_t amdgv_sched_get_hw_sched_mask_by_sched_block(struct amdgv_adapter *adapt,
						      uint32_t idx_vf,
						      enum amdgv_sched_block sched_block)
{
	if (sched_block == AMDGV_SCHED_BLOCK_ALL)
		return adapt->sched.array_vf[idx_vf].hw_sched_mask;
	else
		return adapt->sched.array_vf[idx_vf].block_map[sched_block].hw_sched_mask;
}

/* TODO: Provide this mapping if it's utilised often */
enum amdgv_sched_block
amdgv_sched_get_world_switch_by_hw_sched_id(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					    struct amdgv_sched_world_switch **world_switch)
{
	uint32_t i = 0;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		if (adapt->sched.world_switch[i].hw_sched_mask & BIT(hw_sched_id)) {
			*world_switch = &(adapt->sched.world_switch[i]);
			return 0;
		}
	}

	AMDGV_ERROR("Could not find world_switch given the hw_sched_id\n");
	*world_switch = 0;

	return AMDGV_FAILURE;
}

uint32_t amdgv_sched_get_world_switch(struct amdgv_adapter *adapt, uint32_t idx_part,
				      enum amdgv_sched_block sched_block,
				      struct amdgv_sched_world_switch **world_switch)
{
	*world_switch = adapt->sched.spatial_part[idx_part].ws_map[sched_block];

	return 0;
}

/* Returns the physical XCC mask */
int amdgv_sched_get_xcc_mask_by_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	return adapt->sched.array_vf[idx_vf].xcc_mask;
}

int amdgv_sched_context_init(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     enum amdgv_sched_block sched_block)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	/*
	 * 1) legacy RLCV INIT command can clear VF FB
	 * 2) legacy MMSCH only stops scheduler on VF after INIT command
	 * 3) UVD&VCE depends on data in VF FB.
	 * 4) UVD&VCE will hang if data in FB is wrong.
	 * So, INIT MM before INIT RLCV can avoid UVD&VCE hang.
	 */

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, idx_vf, sched_block)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		//Init MM blocks first
		if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX)
			continue;
		if (amdgv_sched_world_context_init(adapt, idx_vf, world_switch))
			return AMDGV_FAILURE;
	}

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, idx_vf, sched_block)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		//Init GFX blocks next
		if (world_switch->sched_block != AMDGV_SCHED_BLOCK_GFX)
			continue;
		if (amdgv_sched_world_context_init(adapt, idx_vf, world_switch))
			return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_sched_context_load(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     enum amdgv_sched_block sched_block)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, idx_vf, sched_block)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (amdgv_sched_world_context_load(adapt, idx_vf, world_switch))
			return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_sched_context_save(struct amdgv_adapter *adapt, uint32_t idx_vf,
			     enum amdgv_sched_block sched_block)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, idx_vf, sched_block)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (amdgv_sched_world_context_save(adapt, world_switch))
			return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_sched_context_save_all(struct amdgv_adapter *adapt,
				 enum amdgv_sched_block sched_block)
{
	uint32_t i;
	struct amdgv_sched_world_switch *world_switch;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		world_switch = &adapt->sched.world_switch[i];
		if (sched_block != AMDGV_SCHED_BLOCK_ALL)
			if (world_switch->sched_block != sched_block)
				continue;
		if (amdgv_sched_world_context_save(adapt, world_switch))
			return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_sched_context_switch_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
				     enum amdgv_sched_block sched_block)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, idx_vf, sched_block)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (amdgv_sched_world_context_switch_to_vf(adapt, idx_vf, world_switch))
			return AMDGV_FAILURE;
	}

	return 0;
}

/* switch to save state for a specified VF */
int amdgv_sched_context_switch_to_vf_saved(struct amdgv_adapter *adapt, uint32_t idx_vf,
				     enum amdgv_sched_block sched_block)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, idx_vf, sched_block)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		/* continue under one of the below conditions:
		 * 1. not active vf
		 * 2. VF is not inited as we don't init VF here
		 * 3. already on save state of this VF
		 */
		if (!is_active_vf(idx_vf) ||
		    !(world_switch->vf_inited & (1 << idx_vf)) ||
			(world_switch->curr_idx_vf == idx_vf &&
			world_switch->curr_vf_state == AMDGV_VF_CONTEXT_SAVED))
			continue;
		if (amdgv_sched_world_context_load(adapt, idx_vf, world_switch))
			return AMDGV_FAILURE;
		if (amdgv_sched_world_context_save(adapt, world_switch))
			return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_sched_context_switch_gfx_to_pf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		if (world_switch->sched_block != AMDGV_SCHED_BLOCK_GFX)
			continue;

		if (amdgv_sched_world_context_switch_to_vf(adapt, AMDGV_PF_IDX, world_switch))
			return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_sched_context_one_time_loop(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret;
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	if (amdgv_sched_active_vf_num(adapt) == 0)
		return 0;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		ret = amdgv_sched_world_context_one_time_loop(adapt, world_switch);
		if (ret)
			goto failed;
	}

	return 0;

failed:
	ret = amdgv_sched_reset_vf_auto(adapt);
	if (ret)
		return ret;

	amdgv_sched_start_auto(adapt, world_switch->curr_idx_vf);

	return 0;
}

int amdgv_sched_context_clear_state(struct amdgv_adapter *adapt, uint32_t idx_vf,
				    enum amdgv_sched_block sched_block)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, idx_vf, sched_block)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		amdgv_sched_world_context_clear_state(adapt, idx_vf, world_switch);
	}

	return 0;
}

int amdgv_sched_reset(struct amdgv_adapter *adapt, uint32_t idx_vf,
		      enum amdgv_sched_block sched_block)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, idx_vf, sched_block)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (amdgv_sched_world_switch_reset(adapt, idx_vf, world_switch))
			return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_sched_clear_timeslice(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		amdgv_sched_manual_switch_clear_time_slice(world_switch, idx_vf);
	}

	return 0;
}

int amdgv_sched_start(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		amdgv_sched_world_switch_start(adapt, world_switch);
	}

	return 0;
}

int amdgv_sched_start_auto(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t world_switch_id = 0;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->sched_mode > AMDGV_SCHED_MAX_HW_SCHED_MODE)
			continue;

		amdgv_sched_world_switch_start(adapt, world_switch);
	}

	return 0;
}

int amdgv_sched_start_all(struct amdgv_adapter *adapt)
{
	uint32_t i;
	struct amdgv_sched_world_switch *world_switch;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		world_switch = &adapt->sched.world_switch[i];
		if (world_switch->sched_block == AMDGV_SCHED_BLOCK_GFX) {
			continue;
		}
		amdgv_sched_world_switch_start(adapt, world_switch);
	}

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		world_switch = &adapt->sched.world_switch[i];
		/* For BP Mode 1, the gfx sched will not start after first PF Init/Run */
		if (world_switch->sched_block != AMDGV_SCHED_BLOCK_GFX || adapt->bp_gfx_ws_pause_flag == 1) {
			continue;
		}
		amdgv_sched_world_switch_start(adapt, world_switch);
	}

	return 0;
}

int amdgv_sched_stop(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		amdgv_sched_world_switch_stop(adapt, world_switch);
	}

	return 0;
}

int amdgv_sched_stop_all(struct amdgv_adapter *adapt)
{
	uint32_t i;
	struct amdgv_sched_world_switch *world_switch;

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		world_switch = &adapt->sched.world_switch[i];
		amdgv_sched_world_switch_stop(adapt, world_switch);
	}

	return 0;
}

int amdgv_sched_shutdown_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		amdgv_sched_world_switch_shutdown_vf(adapt, idx_vf, world_switch);
	}

	return 0;
}

int amdgv_sched_add_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		amdgv_sched_world_switch_add_vf(adapt, idx_vf, world_switch);
	}

	return 0;
}

int amdgv_sched_remove_vf(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = 0;
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		ret = amdgv_sched_world_switch_remove_vf(adapt, idx_vf, world_switch);
		if (ret)
			return ret;
	}

	return ret;
}

int amdgv_sched_set_vf_num(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	uint32_t idx_vf;
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	/* set vf number should not count PF in */
	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_NUM; idx_vf++) {
		if (!is_unavail_vf(idx_vf)) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_SCHED_RESET_VF_NUM_FAIL,
					idx_vf);
			return AMDGV_FAILURE;
		}
	}

	if (amdgv_sched_reconfig_mapping_tables(adapt, num_vf))
		return AMDGV_FAILURE;

	for (world_switch_id = 0; world_switch_id < adapt->sched.num_world_switch;
	     world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		amdgv_sched_world_switch_set_vf_num(adapt, num_vf, world_switch);
	}

	amdgv_mca_cache_notify_event(adapt,
				     MCA_CACHE_EVENT_NUM_VF_CHANGE,
				     AMDGV_PF_IDX,
				     num_vf);

	adapt->num_vf = num_vf;
	amdgv_sched_setup_default_vfs_timeslice(adapt);

	return 0;
}

int amdgv_sched_toggle_skip_next_punish(struct amdgv_adapter *adapt, uint32_t idx_vf,
					bool enable)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t world_switch_id = 0;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		amdgv_sched_world_switch_toggle_skip_next_punish(adapt, idx_vf, world_switch,
								 enable);
	}

	return 0;
}

bool amdgv_sched_is_state_ok(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_sched_world_switch *world_switch;
	uint32_t world_switch_id = 0;

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask(adapt, idx_vf)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		if (!amdgv_sched_world_context_is_state_ok(adapt, world_switch))
			return false;
	}

	return true;
}

int amdgv_sched_init_pf_state_early(struct amdgv_adapter *adapt)
{
	AMDGV_INFO("Init PF: Start Engine Inits.\n");
	if (amdgv_sched_context_init(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL))
			return AMDGV_FAILURE;

	return 0;
}

int amdgv_sched_init_pf_state_late(struct amdgv_adapter *adapt)
{
	int ret;

	// Add an ASSERT to check for PF in RUN/full access

	if (!(adapt->flags & AMDGV_FLAG_USE_PF)) {
		if (amdgv_sched_context_save_all(adapt, AMDGV_SCHED_BLOCK_ALL))
			return AMDGV_FAILURE;
	} else {
		if (adapt->psp.psp_program_guest_mc_settings)
			adapt->psp.psp_program_guest_mc_settings(adapt, AMDGV_PF_IDX);

		adapt->sched.in_full_access = true;
		adapt->sched.idx_vf_full_access = AMDGV_PF_IDX;
		adapt->sched.start_time_full_access = oss_get_time_stamp();
		adapt->sched.event_id_full_access = AMDGV_EVENT_REQ_GPU_INIT;

		amdgv_gpuiov_toggle_rlcg_vf_interface(adapt, AMDGV_PF_IDX, true);
		AMDGV_INFO("PF entered full access mode.\n");

		if (in_whole_gpu_reset()) {
			// If this is WGR, we need to make sure PF is reinitialized first.
			// Notify OS and wait for reinitialization to complete.
			amdgv_device_func_hw_engine_init(adapt);
			amdgv_reset_mailbox_notify_vf(adapt, AMDGV_PF_IDX, true);

			AMDGV_INFO("Wait for PF REL_GPU_INIT.\n");

			ret = oss_wait_event(adapt->reset.pf_rel_gpu_init,
					     adapt->sched.allow_time_full_access);

			if (ret == OSS_EVENT_STATE_TIMEOUT)
				AMDGV_WARN("PF full access timed out during reset\n");
			else
				AMDGV_INFO("Got PF REL_GPU_INIT.\n");

			if (amdgv_sched_context_save_all(adapt, AMDGV_SCHED_BLOCK_ALL))
				return AMDGV_FAILURE;

			AMDGV_INFO("Exit PF full access AFTER WGR\n");
			adapt->sched.in_full_access = false;
			adapt->sched.idx_vf_full_access = AMDGV_INVALID_IDX_VF;
			adapt->sched.start_time_full_access = 0;
		}
	}

	/* Wake up the event thread. This will activate world-switch and
	 * make the PF the active running function.
	 */
	oss_signal_event(adapt->sched.event);

	return 0;

}

int amdgv_sched_init_pf_state(struct amdgv_adapter *adapt)
{
	int ret;

	ret = amdgv_sched_init_pf_state_early(adapt);
	if (ret)
		return ret;

	return amdgv_sched_init_pf_state_late(adapt);
}

int amdgv_sched_update_time_slice(struct amdgv_adapter *adapt,
				  enum amdgv_sched_block sched_block, uint32_t idx_vf)
{
	uint32_t i;
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	if (sched_block == AMDGV_SCHED_BLOCK_ALL) {
		for (i = 0; i < AMDGV_SCHED_BLOCK_MAX; i++) {
			adapt->sched.array_vf[idx_vf].time_slice[i] =
				adapt->array_vf[idx_vf].time_slice[i];
		}
	} else {
		adapt->sched.array_vf[idx_vf].time_slice[sched_block] =
			adapt->array_vf[idx_vf].time_slice[sched_block];
	}

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, idx_vf, sched_block)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (amdgv_sched_world_switch_update_time_slice(adapt, idx_vf, world_switch))
			return AMDGV_FAILURE;
	}

	return 0;
}

enum amdgv_sched_state amdgv_sched_get_vf_status(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (is_full_access_vf(idx_vf))
		return AMDGV_SCHED_FULLACCESS;

	return adapt->sched.array_vf[idx_vf].state;
}

void amdgv_sched_print_vfs_running_time(struct amdgv_adapter *adapt)
{
	struct amdgv_sched_active_vf_entry *entry;
	struct amdgv_sched_world_switch *world_switch;
	uint32_t world_switch_id;
	uint32_t idx_vf = 0;
	int64_t total_time = 0;
	int64_t curr_time = 0;
	int64_t rate = 0;

	for (world_switch_id = 0; world_switch_id < AMDGV_MAX_NUM_WORLD_SWITCH;
	     world_switch_id++) {
		world_switch = &adapt->sched.world_switch[world_switch_id];

		if (!world_switch->enabled)
			continue;

		curr_time = oss_get_time_stamp();

		AMDGV_INFO("[%s scheduler - RuningTime]:\n",
			   amdgv_sched_block_to_name(world_switch->sched_block));
		AMDGV_INFO("GPU occupy time/ VF started time/ GPU occupy rate\n");

		for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_SLOT; idx_vf++) {
			if (!is_active_vf(idx_vf))
				continue;

			entry = &world_switch->manual.array_vf[idx_vf];

			total_time = curr_time - entry->init_time;
			rate = entry->total_time * 1000000;
			rate /= total_time;

			AMDGV_INFO("%s: %6lld.%06lld/ %6lld.%06lld/ 0.%06lld\n",
				   amdgv_idx_to_str(idx_vf), entry->total_time / 1000000,
				   entry->total_time % 1000000, total_time / 1000000,
				   total_time % 1000000, rate);
		}
	}
}

const char *amdgv_sched_block_to_name(enum amdgv_sched_block sched_block)
{
	int i, count;

	count = ARRAY_SIZE(amdgv_sched_names);
	for (i = 0; i < count; ++i) {
		if (amdgv_sched_names[i].id == sched_block)
			return amdgv_sched_names[i].name;
	}
	return "UNKNOWN ENGINE";
}

const char *amdgv_hw_sched_id_to_name(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	return adapt->gpuiov.ctrl_blocks[hw_sched_id].name;
}

/* Stop WS and switch to PF outside of event manager.
 * This function can only be called from within an event
 * handler, and must be followed by an unpark() call
 */
int amdgv_sched_park(struct amdgv_adapter *adapt)
{
	int ret = 0;

	amdgv_sched_stop_all(adapt);

	ret = amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);
	if (ret)
		amdgv_sched_reset_vf_auto(adapt);

	return ret;
}

/* Prepare to resume the scheduler after parking.
 * This function can only be called from within an event
 * handler, and must be following a park() call
 */
int amdgv_sched_unpark(struct amdgv_adapter *adapt)
{
	int ret = 0;

	ret = amdgv_sched_context_save_all(adapt, AMDGV_SCHED_BLOCK_GFX);
	if (ret)
		amdgv_sched_reset_vf_auto(adapt);

	return ret;
}

/* Stop WS and switch to PF */
int amdgv_sched_lock(struct amdgv_adapter *adapt)
{
	int ret = 0;

	/* There are a few things that's required to lock the scheduler:
	 * 1. Stop world switching
	 * 2. Suspend all VFs by switching to PF
	 */

	/* warn if in full access state, stop world switching */
	if (is_any_vf_in_full_access())
		AMDGV_WARN("Warning: a VF is in full access.\n");

	AMDGV_INFO("Stop world switch.\n");

	ret = amdgv_sched_queue_suspend(adapt);
	if (ret) {
		AMDGV_INFO("Failed to suspend scheduler\n");
	}

	/* switch to PF for all blocks */
	if (amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL) == 0)
		AMDGV_INFO("Switch to PF OK.\n");
	else
		AMDGV_INFO("Switch to PF Fail.\n");

	/* stop disptimer2 if we're in live update SAVE*/
	if (adapt->live_update_state == AMDGV_LIVE_UPDATE_SAVE)
		adapt->irqmgr.ih_funcs->toggle_disp_timer2(adapt, false);

	return ret;
}

int amdgv_sched_set_time_quanta_option(struct amdgv_adapter *adapt,
				       enum amdgv_sched_block sched_block, uint32_t opt)
{
	/* time quanta option should be sorted when add_vf is called */
	adapt->time_quanta_option[sched_block] = opt;
	return 0;
}

int amdgv_sched_get_time_quanta_option(struct amdgv_adapter *adapt,
				       enum amdgv_sched_block sched_block, uint32_t *opt)
{
	*opt = adapt->time_quanta_option[sched_block];
	return 0;
}

uint32_t amdgv_sched_bandwidth_to_time_slice(struct amdgv_adapter *adapt,
					     enum amdgv_mm_engine engine_id,
					     uint32_t bandwidth)
{
	uint64_t max_bandwidth;

	max_bandwidth = adapt->max_mm_bandwidth[engine_id];

	if (max_bandwidth == 0 || bandwidth == 0)
		return 0;

	/* the formula is: Minimum_time_slice = bandwidth/maximum_bandwidth *
	 * 1000 ms/(frame rate). Here time slice is in microseconds.
	 */
	return ((uint64_t)bandwidth * 1000000) / (max_bandwidth * 30);
}

uint32_t amdgv_sched_time_slice_to_bandwidth(struct amdgv_adapter *adapt,
					     enum amdgv_mm_engine engine_id,
					     uint32_t time_slice)
{
	uint64_t max_bandwidth;

	max_bandwidth = adapt->max_mm_bandwidth[engine_id];

	/* Here time slice is in microseconds */
	return (max_bandwidth * 30 * time_slice) / 1000000;
}

static uint32_t amdgv_sched_get_asic_time_slice_gfx(struct amdgv_adapter *adapt,
						    uint32_t num_vf)
{
	if (num_vf > 1 || is_active_vf(AMDGV_PF_IDX)) {
		return DEFAULT_GFX_TIME_SLICE;
	} else if (num_vf == 1) {
		if (adapt->flags & AMDGV_FLAG_DISABLE_SELF_SWITCH) {
			return DEFAULT_GFX_TIME_SLICE_1VF;
		} else {
			return DEFAULT_GFX_TIME_SLICE_1VF_SELF_SW;
		}
	}
	/* if come here, the num_vf should be wrong */
	AMDGV_ERROR("Invalid vf number %d for gfx time slice\n", num_vf);
	/* default value */
	return DEFAULT_GFX_TIME_SLICE_1VF_SELF_SW;
}

static uint32_t amdgv_sched_get_asic_time_slice_mm(struct amdgv_adapter *adapt)
{
	return DEFAULT_MM_TIME_SLICE;
}

uint32_t amdgv_sched_get_asic_time_slice(struct amdgv_adapter *adapt,
					 enum amdgv_sched_block sched_block, uint32_t num_vf)
{
	if (sched_block == AMDGV_SCHED_BLOCK_GFX)
		return amdgv_sched_get_asic_time_slice_gfx(adapt, num_vf);
	else
		return amdgv_sched_get_asic_time_slice_mm(adapt);
}

/*
	This function is called when a runtime set_vf_num request is made
	The driver will reconfig the vf & spatial partition sched tables
	using the new num_vf & existing world_switch array
*/
int amdgv_sched_reconfig_mapping_tables(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	if (adapt->sched.reconfig_mapping_tables)
		return adapt->sched.reconfig_mapping_tables(adapt, num_vf);
	else
		return 0;
}

int amdgv_sched_get_hliquid_min_ts(struct amdgv_adapter *adapt)
{
	int world_switch_id;
	struct amdgv_sched_world_switch *world_switch = NULL;

	for (world_switch_id = 0; world_switch_id < adapt->sched.num_world_switch; world_switch_id++) {
		/* only for hybrid liquid mode on GFX IP */
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->sched_block != AMDGV_SCHED_BLOCK_GFX)
			continue;

		return world_switch->hliquid_min_ts;
	}

	return 0;
}

int amdgv_sched_set_hliquid_min_ts(struct amdgv_adapter *adapt, int hliquid_min_ts)
{
	int vf_ts, world_switch_id;
	int ret = AMDGV_FAILURE;
	struct amdgv_sched_world_switch *world_switch = NULL;

	vf_ts = amdgv_sched_default_gfx_time_slice(adapt, adapt->sched.num_vf_per_gfx_sched);
	for (world_switch_id = 0; world_switch_id < adapt->sched.num_world_switch; world_switch_id++) {
		/* only for hybrid liquid mode on GFX IP */

		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->sched_block != AMDGV_SCHED_BLOCK_GFX)
			continue;

		if (world_switch->sched_mode != AMDGV_SCHED_HYBRID_LIQUID_MODE) {
			AMDGV_ERROR("Not allowed to set in non hybrid liquid mode\n");
			ret = AMDGV_FAILURE;
		} else if (hliquid_min_ts < 0 || hliquid_min_ts > vf_ts) {
			AMDGV_ERROR("The VF minimum time slice is range from 0 to %d\n", vf_ts);
			ret = AMDGV_FAILURE;
		} else {
			AMDGV_INFO("Round down, set VF minimum time slice to %d\n",
					(hliquid_min_ts / 100) * 100);

			world_switch->hliquid_min_ts = (hliquid_min_ts / 100) * 100;
			ret = 0;
		}
	}

	return ret;
}

int amdgv_sched_set_auto_sched_debug_log(struct amdgv_adapter *adapt, enum amdgv_auto_sched_log_op op, bool enable)
{
	int world_switch_id, hw_sched_id, ret = 0;
	struct amdgv_sched_world_switch *world_switch = NULL;
	int event = 0;

	switch (op) {
	case AMDGV_AUTO_SCHED_PERF_LOG:
		event = AMDGV_EVENT_PERF_LOG;
		break;
	case AMDGV_AUTO_SCHED_DEBUG_DUMP:
		event = AMDGV_EVENT_DEBUG_LOG;
		break;
	default:
		return AMDGV_FAILURE;
	}

	if (adapt->gpuiov.funcs->setup_sched_debug_log) {
		ret = adapt->gpuiov.funcs->setup_sched_debug_log(adapt, op);
		if (ret) {
			AMDGV_WARN("Failed to setup auto scheduler debug logs.\n");
			return ret;
		}
	} else {
		AMDGV_WARN("Auto sched debug log is not supported.\n");
		return AMDGV_FAILURE;
	}

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(
					      adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->sched_mode > AMDGV_SCHED_MAX_HW_SCHED_MODE)
			continue;
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			ret = amdgv_gpuiov_event_notification(adapt, AMDGV_PF_IDX, hw_sched_id,
								event, enable);
			if (ret)
				AMDGV_ERROR("failed to send event notification\n");
		}
	}

	return ret;
}

int amdgv_sched_read_perf_log_data(struct amdgv_adapter *adapt)
{
	struct amdgv_perf_log_info *perf_log_info = &adapt->perf_log;
	struct amdgv_memmgr_mem *perf_log_mem;
	uint64_t fb_offset;
	uint64_t timestamp;
	uint32_t i;

	perf_log_info->vf_num = adapt->num_vf;

	perf_log_mem = adapt->gpuiov.perf_log_mem;
	if (perf_log_mem) {
		fb_offset = amdgv_memmgr_get_gpu_addr(perf_log_mem) - adapt->memmgr_pf.mc_base;

		for (i = 0; i < AMDGV_MAX_VF_SLOT; i++) {
			timestamp = READ_FB32(fb_offset + 4);
			timestamp <<= 32;
			timestamp += READ_FB32(fb_offset);

			if (i < adapt->num_vf) {
				perf_log_info->vf_perf_log_info[i].vf_idx = i;
				perf_log_info->vf_perf_log_info[i].time_quanta = timestamp;
			} else if (i == AMDGV_PF_IDX) {
				perf_log_info->vf_perf_log_info[adapt->num_vf].vf_idx = AMDGV_PF_IDX;
				perf_log_info->vf_perf_log_info[adapt->num_vf].time_quanta = timestamp;
			}

			fb_offset += 8;
		}

		for (i = 0; i < AMDGV_MAX_VF_SLOT; i++) {
			if (i < adapt->num_vf)
				perf_log_info->vf_perf_log_info[i].ws_cycle_cnt = READ_FB32(fb_offset);
			else if (i == AMDGV_PF_IDX)
				perf_log_info->vf_perf_log_info[adapt->num_vf].ws_cycle_cnt = READ_FB32(fb_offset);

			fb_offset += 4;
		}

		for (i = 0; i < AMDGV_MAX_VF_SLOT; i++) {
			if (i < adapt->num_vf)
				perf_log_info->vf_perf_log_info[i].skipped_cycle_cnt = READ_FB32(fb_offset);
			else if (i == AMDGV_PF_IDX)
				perf_log_info->vf_perf_log_info[adapt->num_vf].skipped_cycle_cnt = READ_FB32(fb_offset);

			fb_offset += 4;
		}

		for (i = 0; i < AMDGV_MAX_VF_SLOT; i++) {
			if (i < adapt->num_vf)
				perf_log_info->vf_perf_log_info[i].yield_cnt = READ_FB32(fb_offset);
			else if (i == AMDGV_PF_IDX)
				perf_log_info->vf_perf_log_info[adapt->num_vf].yield_cnt = READ_FB32(fb_offset);

			fb_offset += 4;
		}
	} else {
		AMDGV_ERROR("Private csa fb memory not allocated\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

#ifdef WS_RECORD
static const char *const amdgv_auto_ws_record_names[] = {
	"IDLE_S", "SAVE_S", "LOAD_S", "RUN_S", "RUN_F" };

void amdgv_sched_debug_dump_data_flush(struct amdgv_adapter *adapt)
{
	static unsigned long long index;
	static uint64_t last_timestamp;
	uint64_t timestamp;
	uint64_t fb_offset;
	uint64_t cursor = 0;
	uint32_t i = 0;
	uint32_t idx_vf, status;
	uint32_t debug_dump_size = amdgv_memmgr_get_size(adapt->gpuiov.debug_dump_mem);
	uint32_t data[4];
	int ret = 0;

	oss_memset(adapt->auto_ws_record_buf, 0, MAX_RECORD_LENGTH);
	debug_dump_size -= debug_dump_size % 80; // Align size by 5 groups of 4dw
	fb_offset = amdgv_memmgr_get_gpu_addr(adapt->gpuiov.debug_dump_mem) & 0xFFFFFFFF;

	while (adapt->gpuiov.auto_ws_record_rptr < debug_dump_size) {
		for (i = 0; i < 4; i++, adapt->gpuiov.auto_ws_record_rptr += 4)
			data[i] = READ_FB32(fb_offset + adapt->gpuiov.auto_ws_record_rptr);

		timestamp = ((uint64_t)(data[1]) << 32) + data[0];
		idx_vf = (data[2] & 0xFF) & 0x80 ? data[2] & 0x7F : AMDGV_PF_IDX;
		status = data[2] >> 8;

		if (timestamp < last_timestamp) {
			adapt->gpuiov.auto_ws_record_rptr -= 16;
			break;
		} else if (!timestamp) {
			// Skip empty load-run log if any
			continue;
		}

		ret = oss_vsnprintf(adapt->auto_ws_record_buf + cursor, MAX_RECORD_LENGTH - cursor,
					"%10lld %20lld %10s %30s\n", index,
					timestamp, amdgv_idx_to_str(idx_vf),
					status < AMDGV_AUTO_WS_MAX ? amdgv_auto_ws_record_names[status - 1] : "Unknown");
		index++;

		last_timestamp = timestamp;

		if (ret == -1) {
			AMDGV_WARN("dump contents is truncated due to small buffer, rptr = %lx\n", adapt->gpuiov.auto_ws_record_rptr);
		}

		cursor += ret;
	}
	/* When rptr hit the size, read remain data in next loop.
	 * The minimum debug dump size is 1MB, to overflow the data in 2 seconds we need
	 * 6000+ world switches per second. So there is no risk losing data in current frequency.
	 */
	if (adapt->gpuiov.auto_ws_record_rptr >= debug_dump_size)
		adapt->gpuiov.auto_ws_record_rptr = 0;

	if (*adapt->auto_ws_record_buf &&
		oss_store_record(adapt->auto_ws_record_buf, adapt->bdf, true))
		AMDGV_WARN("store record failed\n");

	return;
}
#endif

enum amdgv_live_info_status amdgv_sched_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_sched *sched_info)
{
	sched_info->in_full_access = adapt->sched.in_full_access;
	sched_info->idx_vf_full_access = adapt->sched.idx_vf_full_access;
	sched_info->event_id_full_access = adapt->sched.event_id_full_access;
	sched_info->allow_time_full_access = adapt->sched.allow_time_full_access;
	sched_info->used_time_full_access = adapt->sched.used_time_full_access;
	sched_info->lock_world_switch = adapt->lock_world_switch;
	sched_info->unrecov_err = adapt->sched.unrecov_err;
	sched_info->check_alive = false;
	sched_info->logical_sched_fa_mask = adapt->sched.logical_sched_fa_mask;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_sched_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_sched *sched_info)
{
	struct amdgv_sched_vf_info *vf;
	// For GPUV, set lock_world_switch during first import scheduler data call and import the rest when we unlock scheduler
	if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) && !adapt->lock_world_switch) {
		adapt->lock_world_switch = sched_info->lock_world_switch;
		return AMDGV_LIVE_INFO_STATUS_SUCCESS;
	}

	adapt->sched.in_full_access = sched_info->in_full_access;
	adapt->sched.idx_vf_full_access = sched_info->idx_vf_full_access;
	adapt->sched.event_id_full_access = sched_info->event_id_full_access;
	if (adapt->opt.allow_time_full_access)
		adapt->sched.allow_time_full_access = sched_info->allow_time_full_access;
	adapt->sched.used_time_full_access = sched_info->used_time_full_access;
	adapt->lock_world_switch = sched_info->lock_world_switch;
	adapt->sched.unrecov_err = sched_info->unrecov_err;
	adapt->sched.logical_sched_fa_mask = sched_info->logical_sched_fa_mask;

	if (adapt->sched.in_full_access)
		adapt->sched.skip_full_access_timeout_check = true;
	/* If live update occurred during full access and old GIM didn't have the per partition
	 * full access mode but the new GIM does, try to figure out which partition now should be
	 * in full access mode.
	 */
	if (adapt->sched.enable_per_partition_full_access && adapt->sched.in_full_access) {
		vf = &adapt->sched.array_vf[adapt->sched.idx_vf_full_access];
		vf->in_full_access = true;
		vf->used_time_full_access = adapt->sched.used_time_full_access;
		vf->event_id_full_access = adapt->sched.event_id_full_access;
		adapt->sched.logical_sched_fa_mask |= vf->world_switch_mask;
		adapt->sched.in_full_access = false;
		adapt->sched.idx_vf_full_access = AMDGV_INVALID_IDX_VF;
		adapt->sched.start_time_full_access = 0;
		sched_info->used_time_full_access = 0;
	}
	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

int amdgv_sched_setup_vf_timeslice(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t timeslice, enum amdgv_sched_block sched_block)
{
	if (adapt->sched.setup_vf_timeslice)
		return adapt->sched.setup_vf_timeslice(adapt, idx_vf, timeslice, sched_block);
	else if (sched_block == AMDGV_SCHED_BLOCK_GFX) {
		AMDGV_ERROR("setup vf timeslice is not supported on this asic.\n");
		return AMDGV_FAILURE;
	} else
		return 0;
}

int amdgv_sched_setup_default_vfs_timeslice(struct amdgv_adapter *adapt)
{
	if (adapt->sched.setup_default_vfs_timeslice)
		return adapt->sched.setup_default_vfs_timeslice(adapt);
	else
		return 0;
}

int amdgv_sched_get_sched_mode(struct amdgv_adapter *adapt, enum amdgv_sched_block sched_block, enum amdgv_sched_mode *sched_mode)
{
	int i = 0, ret = 0;
	struct amdgv_sched_world_switch *world_switch = NULL;

	for (i = 0; i < adapt->sched.num_world_switch; ++i) {
		world_switch = &adapt->sched.world_switch[i];
		if (world_switch->enabled && world_switch->sched_block == sched_block)
			break;
	}

	if (i == adapt->sched.num_world_switch) {
		AMDGV_ERROR("No world switch of block type %d found\n", sched_block);
		ret = AMDGV_FAILURE;
	} else {
		*sched_mode = world_switch->sched_mode;
	}

	return ret;
}
void amdgv_sched_set_unrecov_err(struct amdgv_adapter *adapt)
{
	adapt->sched.unrecov_err = true;
}

void amdgv_sched_clear_unrecov_err(struct amdgv_adapter *adapt)
{
	adapt->sched.unrecov_err = false;
}

bool amdgv_sched_is_unrecov_err(struct amdgv_adapter *adapt)
{
	return adapt->sched.unrecov_err;
}
