/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_oss_wrapper.h"
#include "ras_sys.h"
#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_notify.h"
#include "amdgv_vfmgr.h"
#include "amdgv_ras_mgr.h"
#include "amdgv_ras_cmd.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

void *ras_calloc(unsigned int n, unsigned int size)
{
	return oss_zalloc(n * size);
}

static bool amdgv_ras_sys_check_bp_threshold(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	enum ras_gpu_op_status status;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;

	status = ras_eeprom_mgr_get_gpu_op_status(ras_mgr->ras_core);

	/* RMA at the max count because new entries will be lost. */
	if (status == RAS_GPU_OP_STATUS_LOCKED) {
		adapt->bp_msg_type = AMDGV_BP_MSG_RECORD_THRESHOLD_REACHED;
		return true;
	}

	return false;
}

static int amdgv_ras_sys_reserve_bad_page(struct amdgv_adapter *adapt, uint64_t pfn)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	uint64_t err_addr_pf, err_addr_gpu, total_fb;
	int ret = 0;
	uint32_t idx_vf, total_fb_in_mb;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS))
		return -RAS_CORE_NOT_SUPPORTED;

	if (!ras_mgr)
		return -RAS_CORE_EINVAL;

	if (amdgv_ras_sys_check_bp_threshold(adapt)) {
		amdgv_umc_log_bp_errors(adapt, BAD_PAGE_RECORD_THRESHOLD);
		return -RAS_CORE_EPERM;
	}

	err_addr_pf = RAS_PFN_TO_ADDR(pfn);
	amdgv_gpuiov_get_total_avail_fb_size(adapt, &total_fb_in_mb);
	total_fb = (uint64_t)total_fb_in_mb;
	/* convert from MB to Byte */
	total_fb = MBYTES_TO_BYTES(total_fb);
	err_addr_gpu = total_fb - err_addr_pf;

	idx_vf = amdgv_umc_calc_retired_page_vf_slot(adapt, err_addr_pf);
	if (amdgv_umc_check_bp_in_critical_region(adapt, err_addr_pf, idx_vf, true)) {
		amdgv_umc_log_bp_errors(adapt, ras_umc_get_badpage_count(ras_mgr->ras_core));
		amdgv_vfmgr_handle_bp_in_crit_region(adapt, idx_vf);
		return -RAS_CORE_EPERM;
	}

	/* There are three cases of reserve error should be ignored:
	 * 1) a ras bad page has been allocated (used by someone);
	 * 2) a ras bad page has been reserved (duplicate error injection
	 *    for one page);
	 * 3) a ras bad page does not fall into memmgr_pf and memmgr_gpu (
	 *    falls into vf region) guest will handle page reserve;
	 */
	if ((err_addr_pf >= adapt->memmgr_pf.offset) &&
		(err_addr_pf < adapt->memmgr_pf.offset + adapt->memmgr_pf.size)) {
		ret = amdgv_memmgr_reserve_page(adapt,
				&adapt->memmgr_pf, RAS_ADDR_TO_PFN(err_addr_pf));
		if (ret)
			AMDGV_WARN("Reserve page at 0x%llx in memmgr_pf failed, ret:%d\n",
					err_addr_pf, ret);
	} else if ((err_addr_gpu >= adapt->memmgr_gpu.offset) &&
		(err_addr_gpu < adapt->memmgr_gpu.offset + adapt->memmgr_gpu.size)) {
		ret = amdgv_memmgr_reserve_page(adapt,
				&adapt->memmgr_gpu, RAS_ADDR_TO_PFN(err_addr_gpu));
		if (ret)
			AMDGV_WARN("Reserve page at 0x%llx in memmgr_gpu failed, ret:%d\n",
					err_addr_gpu, ret);
	}

	return ret;
}

static int amdgv_send_hbm_bad_pages_num(struct amdgv_adapter *adapt, uint32_t size)
{
	if (adapt->pp.pp_funcs->send_hbm_bad_pages_num)
		return adapt->pp.pp_funcs->send_hbm_bad_pages_num(adapt, size);

	return 0;
}

static int amdgv_send_hbm_bad_channel_flag(struct amdgv_adapter *adapt, uint32_t size)
{
	if (adapt->pp.pp_funcs->send_hbm_bad_channel_flag)
		return adapt->pp.pp_funcs->send_hbm_bad_channel_flag(adapt, size);

	return 0;
}

static int amdgv_send_rma_reason(struct amdgv_adapter *adapt)
{
	if (adapt->pp.pp_funcs->send_rma_reason)
		adapt->pp.pp_funcs->send_rma_reason(adapt, PP_RMA_BAD_PAGE_THRESHOLD);
	return 0;
}

static int amdgv_ras_reset_gpu(struct amdgv_adapter *adapt, uint32_t reset_flags)
{
	if (reset_flags & RAS_CORE_RESET_GPU) {
		struct amdgv_adapter *tmp_adapt = adapt;

		adapt->reset.reset_mode = AMDGV_RESET_MODE1; //adapt->umc.reset_mode;
		if (adapt->xgmi.master_adapt) {
			AMDGV_INFO("Forwarding reset event to master adapter:0x%x\n",
					adapt->xgmi.master_adapt->bdf);
			tmp_adapt = adapt->xgmi.master_adapt;
		}

		oss_atomic_set(adapt->in_ecc_recovery, 1);
		amdgv_sched_queue_event(tmp_adapt,
			AMDGV_PF_IDX, AMDGV_EVENT_SCHED_FORCE_RESET_GPU, 0);
	}

	return 0;
}

static int amdgv_ras_sys_detect_fatal_event(struct ras_core_context *ras_core, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	uint64_t seqno;
	int ret;

	if (!oss_atomic_read(adapt->in_ecc_recovery))
		seqno = amdgv_ras_mgr_gen_ras_event_seqno(adapt, RAS_SEQNO_TYPE_UE);

	ret = amdgv_ecc_check_global_ras_errors(adapt);
	if (ret)
		return ret;

	RAS_DEV_INFO(adapt,
		"{%llu} Uncorrectable hardware error(ERREVENT_ATHUB_INTERRUPT) detected!\n",
		seqno);

	return amdgv_ras_reset_gpu(adapt, GPU_RESET_CAUSE_FATAL);
}

static int amdgv_ras_early_init_reserve_badpage(struct ras_core_context *ras_core,
			uint64_t pfn)
{
	/* Not supported yet */
	return 0;
}

static int amdgv_ras_sys_event_notifier(struct ras_core_context *ras_core,
				   enum ras_notify_event event_id, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	int ret = 0;

	switch (event_id) {
	case RAS_EVENT_ID__BAD_PAGE_DETECTED:
		ret = ras_umc_handle_bad_pages(ras_core, data);
		if (ret == 0)
			amdgv_sched_notify_vfs_bad_pages_at_poison_creation(adapt);
		break;
	case RAS_EVENT_ID__RESERVE_BAD_PAGE:
		ret = amdgv_ras_sys_reserve_bad_page(adapt, *(uint64_t *)data);
		break;
	case RAS_EVENT_ID__EARLY_INIT_RESERVE_PAGE:
		ret = amdgv_ras_early_init_reserve_badpage(ras_core, *(uint64_t *)data);
		break;
	case RAS_EVENT_ID__FATAL_ERROR_DETECTED:
		ret = amdgv_ras_sys_detect_fatal_event(ras_core, data);
		break;
	case RAS_EVENT_ID__UPDATE_BAD_PAGE_NUM:
		ret = amdgv_send_hbm_bad_pages_num(adapt, *(uint32_t *)data);
		break;
	case RAS_EVENT_ID__UPDATE_BAD_CHANNEL_BITMAP:
		ret = amdgv_send_hbm_bad_channel_flag(adapt, *(uint32_t *)data);
		break;
	case RAS_EVENT_ID__DEVICE_RMA:
		ras_log_ring_add_log_event(ras_core, RAS_LOG_EVENT_RMA, NULL, 0, NULL);
		amdgv_send_rma_reason(adapt);
		amdgv_device_handle_bad_gpu(adapt);
		break;
	case RAS_EVENT_ID__RESET_GPU:
		ret = amdgv_ras_reset_gpu(adapt, *(uint32_t *)data);
		break;
	case RAS_EVENT_ID__RAS_EVENT_PROC_BEGIN:
		ret = 0;
		break;
	case RAS_EVENT_ID__RAS_EVENT_PROC_END:
		ret = amdgv_ras_cmd_update_auto_list(adapt);
		break;
	case RAS_EVENT_ID__UPDATE_ECC_DATA:
		ret = amdgv_ras_cmd_set_auto_list(adapt, true);
		break;
	default:
		RAS_DEV_WARN(adapt, "Invalid ras notify event:%d\n", event_id);
		break;
	}

	return ret;
}

static u64 amdgv_ras_sys_get_utc_second_timestamp(struct ras_core_context *ras_core)
{
	return oss_get_utc_time_stamp();
}

static int amdgv_ras_sys_gen_seqno(struct ras_core_context *ras_core,
			enum ras_seqno_type seqno_type, uint64_t *seqno)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_event_manager *event_mgr;
	struct ras_event_state *event_state;
	struct amdgv_hive_info *hive;
	enum ras_event_type event_type;
	uint64_t seq_no;

	if (!ras_mgr || !seqno ||
		(seqno_type >= RAS_SEQNO_TYPE_COUNT_MAX))
		return -RAS_CORE_EINVAL;

	switch (seqno_type) {
	case RAS_SEQNO_TYPE_UE:
		event_type = RAS_EVENT_TYPE_FATAL;
		break;
	case RAS_SEQNO_TYPE_CE:
	case RAS_SEQNO_TYPE_DE:
		event_type = RAS_EVENT_TYPE_POISON_CREATION;
		break;
	case RAS_SEQNO_TYPE_POISON_CONSUMPTION:
		event_type = RAS_EVENT_TYPE_POISON_CONSUMPTION;
		break;
	default:
		event_type = RAS_EVENT_TYPE_INVALID;
		break;
	}

	hive = amdgv_get_xgmi_hive(adapt);
	event_mgr = hive ? &hive->event_mgr : &ras_mgr->ras_event_mgr;
	event_state = &event_mgr->event_state[event_type];
	if ((event_type == RAS_EVENT_TYPE_FATAL) &&
		oss_atomic_read(adapt->in_ecc_recovery)) {
		seq_no = event_state->last_seqno;
	} else {
		seq_no = ++event_mgr->seqno;
		event_state->last_seqno = seq_no;
		event_state->count++;
	}

	*seqno = event_state->last_seqno;

	return 0;

}

static int amdgv_ras_sys_check_gpu_status(struct ras_core_context *ras_core,
			enum ras_gpu_status *status)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_hive_info *hive = amdgv_get_xgmi_hive(adapt);
	uint32_t gpu_status = 0;

	if (oss_atomic_read(adapt->in_ecc_recovery) ||
	    (hive && hive->in_chain_reset))
		gpu_status |= RAS_GPU_STATUS__IN_RESET;

	*status = gpu_status;

	return 0;
}

static int amdgv_ras_sys_async_handle_ras_event(struct ras_core_context *ras_core, void *data)
{
	amdgv_sched_queue_event(ras_core->dev, AMDGV_PF_IDX,
			AMDGV_EVENT_SCHED_RAS_EVENT, AMDGV_SCHED_BLOCK_ALL);

	return 0;
}

static int amdgv_ras_sys_get_device_system_info(struct ras_core_context *ras_core,
			struct device_system_info *dev_info)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;

	dev_info->device_id = adapt->dev_id;
	dev_info->vendor_id = adapt->vendor_id;
	dev_info->socket_id = adapt->xgmi.socket_id;

	return 0;
}

static bool amdgv_ras_sys_detect_ras_interrupt(struct ras_core_context *ras_core)
{
	return amdgv_ras_intr_triggered();
}

static int amdgv_ras_sys_get_gpu_mem(struct ras_core_context *ras_core,
	enum gpu_mem_type mem_type, struct gpu_mem_block *gpu_mem)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *ring_mem;
	struct psp_local_memory *psp_fence_mem;
	struct psp_local_memory *ras_bin_mem;
	struct psp_local_memory *ras_cmd_mem;
	struct psp_cmd_km_handle *buf_handle;
	struct psp_local_memory *psp_cmd_mem;

	if (mem_type == GPU_MEM_TYPE_RAS_PSP_RING) {
		ring_mem = &psp->km_ring[psp->idx].ring_mem;
		gpu_mem->mem_bo = ring_mem->mem;
		gpu_mem->mem_size = ring_mem->size;
		gpu_mem->mem_mc_addr = amdgv_memmgr_get_gpu_addr(ring_mem->mem);
		gpu_mem->mem_cpu_addr = amdgv_memmgr_get_cpu_addr(ring_mem->mem);
	} else if (mem_type == GPU_MEM_TYPE_RAS_PSP_CMD) {
		buf_handle = (struct psp_cmd_km_handle *)oss_zalloc(sizeof(*buf_handle));
		if (!buf_handle)
			return -RAS_CORE_ENOMEM;

		if (amdgv_psp_cmd_km_allocate_buf(psp, buf_handle)) {
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_FW_CMD_ALLOC_BUF_FAIL, 0);
			return -RAS_CORE_ENOMEM;
		}

		psp_cmd_mem =
			&psp->km_cmd_context.km_cmd_buf_pool[buf_handle->index].cmd_mem;
		gpu_mem->mem_bo = psp_cmd_mem->mem;
		gpu_mem->mem_size = psp_cmd_mem->size;
		gpu_mem->mem_mc_addr = amdgv_memmgr_get_gpu_addr(psp_cmd_mem->mem);
		gpu_mem->mem_cpu_addr = amdgv_memmgr_get_cpu_addr(psp_cmd_mem->mem);
		gpu_mem->priv = buf_handle;
	} else if (mem_type == GPU_MEM_TYPE_RAS_PSP_FENCE) {
		psp_fence_mem = &adapt->psp.km_cmd_context.km_fence_mem_handle;
		gpu_mem->mem_bo = psp_fence_mem->mem;
		gpu_mem->mem_size = psp_fence_mem->size;
		gpu_mem->mem_mc_addr = amdgv_memmgr_get_gpu_addr(psp_fence_mem->mem);
		gpu_mem->mem_cpu_addr = amdgv_memmgr_get_cpu_addr(psp_fence_mem->mem);
	} else if (mem_type == GPU_MEM_TYPE_RAS_FW_BIN) {
		ras_bin_mem = &adapt->psp.private_fw_memory;
		gpu_mem->mem_bo = ras_bin_mem->mem;
		gpu_mem->mem_size = ras_bin_mem->size;
		gpu_mem->mem_mc_addr = amdgv_memmgr_get_gpu_addr(ras_bin_mem->mem);
		gpu_mem->mem_cpu_addr = amdgv_memmgr_get_cpu_addr(ras_bin_mem->mem);
	} else if (mem_type == GPU_MEM_TYPE_RAS_TA_CMD) {
		ras_cmd_mem = &adapt->psp.ras_context.shared_buffer;
		gpu_mem->mem_bo = ras_cmd_mem->mem;
		gpu_mem->mem_size = ras_cmd_mem->size;
		gpu_mem->mem_mc_addr = amdgv_memmgr_get_gpu_addr(ras_cmd_mem->mem);
		gpu_mem->mem_cpu_addr = amdgv_memmgr_get_cpu_addr(ras_cmd_mem->mem);
	} else if (mem_type == GPU_MEM_TYPE_ALLOC_MEM) {
		struct amdgv_memmgr_mem *alloc_mem;

		if (!gpu_mem->mem_size)
			return -RAS_CORE_EINVAL;

		/* Same PF memmgr as PSP_MEMMGR in amdgv_psp.c (matches amdgpu BO path). */
		alloc_mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, gpu_mem->mem_size,
						     0x1000u, MEM_PSP_DUMMY);
		if (!alloc_mem)
			return -RAS_CORE_ENOMEM;

		gpu_mem->mem_bo = alloc_mem;
		gpu_mem->mem_mc_addr = amdgv_memmgr_get_gpu_addr(alloc_mem);
		gpu_mem->mem_cpu_addr = amdgv_memmgr_get_cpu_addr(alloc_mem);
	} else {
		return -RAS_CORE_EINVAL;
	}

	if (!gpu_mem->mem_bo || !gpu_mem->mem_size ||
		!gpu_mem->mem_mc_addr || !gpu_mem->mem_cpu_addr) {
		RAS_DEV_ERR(ras_core->dev, "The ras psp gpu memory is invalid!\n");
		return -RAS_CORE_ENOMEM;
	}

	return 0;
}

static int amdgv_ras_sys_put_gpu_mem(struct ras_core_context *ras_core,
	enum gpu_mem_type mem_type, struct gpu_mem_block *gpu_mem)
{

	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct psp_cmd_km_handle *buf_handle;

	if (mem_type == GPU_MEM_TYPE_RAS_PSP_CMD) {
		if (gpu_mem->priv) {
			buf_handle = (struct psp_cmd_km_handle *)gpu_mem->priv;
			amdgv_psp_cmd_km_release_buf(&adapt->psp, buf_handle);
			oss_free(buf_handle);
			gpu_mem->priv = NULL;
		}
	} else if (mem_type == GPU_MEM_TYPE_ALLOC_MEM) {
		if (gpu_mem->mem_bo) {
			amdgv_memmgr_free((struct amdgv_memmgr_mem *)gpu_mem->mem_bo);
			gpu_mem->mem_bo = NULL;
			gpu_mem->mem_cpu_addr = NULL;
			gpu_mem->mem_mc_addr = 0;
		}
	}

	return 0;
}

static int amdgv_ras_sys_get_nps_mode(struct ras_core_context *ras_core)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	enum amdgv_memory_partition_mode nps_mode;
	int ret;

	if (!adapt->nbio.funcs || !adapt->nbio.funcs->get_nps_mode)
		return AMDGV_MEMORY_PARTITION_MODE_UNKNOWN;

	ret = adapt->nbio.funcs->get_nps_mode(adapt, &nps_mode);

	return ret ? AMDGV_MEMORY_PARTITION_MODE_UNKNOWN : nps_mode;
}

static int convert_atom_mem_type_to_vram_type(struct amdgv_adapter *adapt,
					int atom_mem_type)
{
	int vram_type;

	switch (atom_mem_type) {
	case AMDGV_DGPU_VRAM_TYPE__GDDR5:
		vram_type = UMC_VRAM_TYPE_GDDR5;
		break;
	case AMDGV_DGPU_VRAM_TYPE__HBM2:
	case AMDGV_DGPU_VRAM_TYPE__HBM2E:
	case AMDGV_DGPU_VRAM_TYPE__HBM3:
		vram_type = UMC_VRAM_TYPE_HBM;
		break;
	case AMDGV_DGPU_VRAM_TYPE__GDDR6:
		vram_type = UMC_VRAM_TYPE_GDDR6;
		break;
	case AMDGV_DGPU_VRAM_TYPE__HBM3E:
		vram_type = UMC_VRAM_TYPE_HBM3E;
		break;
	default:
		RAS_DEV_ERR(adapt, "Unknown atom vram type 0x%x\n", atom_mem_type);
		vram_type = UMC_VRAM_TYPE_UNKNOWN;
		break;
	}

	return vram_type;
}

static int amdgv_ras_sys_get_vram_type(struct ras_core_context *ras_core)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;

	if (!adapt->vram_info.vram_type && adapt->vbios.atom_context &&
		amdgv_atomfirmware_get_vram_info(adapt)) {
		RAS_DEV_ERR(ras_core->dev, "Failed to get vram type!\n");
	}

	return convert_atom_mem_type_to_vram_type(adapt, adapt->vram_info.vram_type);
}

const struct ras_sys_func amdgv_ras_sys_fn = {
	.ras_notifier = amdgv_ras_sys_event_notifier,
	.get_utc_second_timestamp = amdgv_ras_sys_get_utc_second_timestamp,
	.gen_seqno = amdgv_ras_sys_gen_seqno,
	.async_handle_ras_event = amdgv_ras_sys_async_handle_ras_event,
	.check_gpu_status = amdgv_ras_sys_check_gpu_status,
	.get_device_system_info = amdgv_ras_sys_get_device_system_info,
	.detect_ras_interrupt = amdgv_ras_sys_detect_ras_interrupt,
	.get_gpu_mem = amdgv_ras_sys_get_gpu_mem,
	.put_gpu_mem = amdgv_ras_sys_put_gpu_mem,
	.get_nps_mode = amdgv_ras_sys_get_nps_mode,
	.get_vram_type = amdgv_ras_sys_get_vram_type,
};