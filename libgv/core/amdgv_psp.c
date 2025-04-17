/*
 * Copyright (c) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_psp_gfx_if.h"
#include "amdgv_sched_internal.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_notify.h"
#include "hw/common/ucode/psp_xgmi_bin.h"
#include "hw/common/ucode/psp_xgmi_tee3_esbin.h"

/* position of instance value in sub_block_index of
 * ta_ras_trigger_error_input, the sub block uses lower 12 bits
 */
#define AMDGV_RAS_INST_MASK 0xfffff000
#define AMDGV_RAS_INST_SHIFT 0xc

enum { PSP_RAS_SHARED_MEM_SIZE = 0x4000 };	 /* 16K */
enum { PSP_RAS_SHARED_MEM_ALIGNMENT = 0x1000 };	 /* 4K */
enum { PSP_XGMI_SHARED_MEM_SIZE = 0x4000 };	 /* 16K */
enum { PSP_XGMI_SHARED_MEM_ALIGNMENT = 0x1000 }; /* 4K */

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

static inline struct amdgv_memmgr *amdgv_psp__get_memmgr(struct amdgv_adapter *adapt)
{
	if (adapt->memmgr_sys.is_init)
		return &adapt->memmgr_sys;

	return &adapt->memmgr_pf;
}

#define PSP_MEMMGR amdgv_psp__get_memmgr(adapt)

enum psp_status amdgv_psp_cmd_km_init(struct amdgv_adapter *adapt)
{
	struct psp_local_memory *local_mem = NULL;
	struct psp_context *psp = &adapt->psp;
	struct psp_cmd_km_buf *psp_gfx_cmd_buf = NULL;
	uint32_t count = 0;

	for (count = 0; count < PSP_KM_CMD_MAX_NUM; count++) {
		psp_gfx_cmd_buf = &psp->km_cmd_context.km_cmd_buf_pool[count];

		local_mem = &psp_gfx_cmd_buf->cmd_mem;
		local_mem->alignment = PSP_CMD_ALIGNMENT;
		local_mem->size = (PSP_CMD_PAGE_SIZE + (local_mem->alignment - 1)) &
				  (~(local_mem->alignment - 1));

		AMDGV_DEBUG("allocate CMD\n");
		local_mem->mem =
			amdgv_memmgr_alloc_align(PSP_MEMMGR, local_mem->size,
						 local_mem->alignment, PSP_CMD_BUF_ID(count));
		if (local_mem->mem) {
			psp_gfx_cmd_buf->fence_value = 0;
			psp_gfx_cmd_buf->used = false;
		}
	}

	local_mem = &psp->km_cmd_context.km_fence_mem_handle;
	local_mem->alignment = PSP_FENCE_ALIGNMENT;
	local_mem->size =
		(PSP_FENCE_SIZE + (local_mem->alignment - 1)) & (~(local_mem->alignment - 1));

	AMDGV_DEBUG("allocate FENCE\n");
	local_mem->mem = amdgv_memmgr_alloc_align(PSP_MEMMGR, local_mem->size,
						  local_mem->alignment, MEM_PSP_FENCE);

	if (!local_mem->mem)
		return PSP_STATUS__ERROR_GENERIC;

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_cmd_km_fini(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_local_memory *local_mem = NULL;
	struct psp_context *psp = &adapt->psp;
	uint32_t count = 0;

	local_mem = &psp->km_cmd_context.km_fence_mem_handle;
	if (local_mem->mem) {
		amdgv_memmgr_free(local_mem->mem);
		local_mem->mem = NULL;
	}

	for (count = 0; count < PSP_KM_CMD_MAX_NUM; count++) {
		local_mem = &psp->km_cmd_context.km_cmd_buf_pool[count].cmd_mem;
		if (local_mem->mem) {
			amdgv_memmgr_free(local_mem->mem);
			local_mem->mem = NULL;
		}
	}

	return ret;
}

enum psp_status amdgv_psp_cmd_km_start(struct amdgv_adapter *adapt)
{
	struct psp_local_memory *local_mem = NULL;
	struct psp_context *psp = &adapt->psp;

	/* Clear FENCE area */
	local_mem = &psp->km_cmd_context.km_fence_mem_handle;

	if (!local_mem->mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	else
		oss_memset(amdgv_memmgr_get_cpu_addr(local_mem->mem), 0, local_mem->size);

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_cmd_km_allocate_buf(struct psp_context *psp,
					      struct psp_cmd_km_handle *buf_handle)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km_buf *psp_gfx_cmd_buf;

	psp_gfx_cmd_buf =
		&psp->km_cmd_context
			 .km_cmd_buf_pool[psp->km_cmd_context.next_avail_cmd_buf_index];

	if (false == psp_gfx_cmd_buf->used && psp_gfx_cmd_buf->cmd_mem.mem) {
		/* Initialize CMD buffer */
		oss_memset(amdgv_memmgr_get_cpu_addr(psp_gfx_cmd_buf->cmd_mem.mem), 0,
			   psp_gfx_cmd_buf->cmd_mem.size);

		buf_handle->index = psp->km_cmd_context.next_avail_cmd_buf_index;
		/* Mark current CMD buffer as used*/
		psp_gfx_cmd_buf->used = true;
		if (PSP_KM_CMD_MAX_NUM == ++psp->km_cmd_context.next_avail_cmd_buf_index) {
			psp->km_cmd_context.next_avail_cmd_buf_index = 0;
		}
	} else {
		ret = PSP_STATUS__ERROR_OUT_OF_MEMORY;
	}

	return ret;
}

enum psp_status amdgv_psp_cmd_km_release_buf(struct psp_context *psp,
					     struct psp_cmd_km_handle *buf_handle)
{
	struct psp_cmd_km_buf *psp_gfx_cmd_buf = NULL;

	if ((buf_handle == NULL) || (buf_handle->index == PSP_KM_CMD_INVALID_INDEX)) {
		return PSP_STATUS__ERROR_INVALID_PARAMS;
	}

	psp_gfx_cmd_buf = &psp->km_cmd_context.km_cmd_buf_pool[buf_handle->index];

	/* Clear associated fence value */
	psp_gfx_cmd_buf->fence_value = 0;

	/* Reset index to invalid value */
	buf_handle->index = PSP_KM_CMD_INVALID_INDEX;

	/* Return buffer to pool */
	psp_gfx_cmd_buf->used = false;

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_cmd_km_buf_prep(struct psp_context *psp, struct psp_cmd_km *km_cmd,
					  struct psp_cmd_km_handle *km_cmd_handle)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_gfx_cmd_resp *gfx_cmd = NULL;

	if ((km_cmd_handle == NULL) || (km_cmd_handle->index == PSP_KM_CMD_INVALID_INDEX)) {
		return PSP_STATUS__ERROR_INVALID_PARAMS;
	}

	gfx_cmd = (struct psp_gfx_cmd_resp *)(amdgv_memmgr_get_cpu_addr(
		psp->km_cmd_context.km_cmd_buf_pool[km_cmd_handle->index].cmd_mem.mem));

	if (gfx_cmd == NULL)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	switch (km_cmd->cmd_id) {
	case PSP_CMD_KM_TYPE__LOAD_TA:
	case PSP_CMD_KM_TYPE__LOAD_ASD:
		/* Populate local GFX CMD struct */
		gfx_cmd->cmd_id = (km_cmd->cmd_id == PSP_CMD_KM_TYPE__LOAD_TA) ?
					  GFX_CMD_ID_LOAD_TA :
					  GFX_CMD_ID_LOAD_ASD;
		gfx_cmd->cmd.cmd_load_ta.app_phy_addr_lo = km_cmd->cmd.load_ta.app_buf_addr_lo;
		gfx_cmd->cmd.cmd_load_ta.app_phy_addr_hi = km_cmd->cmd.load_ta.app_buf_addr_hi;
		gfx_cmd->cmd.cmd_load_ta.app_len = km_cmd->cmd.load_ta.app_size;
		gfx_cmd->cmd.cmd_load_ta.cmd_buf_phy_addr_lo =
			km_cmd->cmd.load_ta.shared_mem_addr_lo;
		gfx_cmd->cmd.cmd_load_ta.cmd_buf_phy_addr_hi =
			km_cmd->cmd.load_ta.shared_mem_addr_hi;
		gfx_cmd->cmd.cmd_load_ta.cmd_buf_len = km_cmd->cmd.load_ta.shared_mem_size;
		break;
	case PSP_CMD_KM_TYPE__UNLOAD_TA:
		/* Populate local GFX CMD struct */
		gfx_cmd->cmd_id = GFX_CMD_ID_UNLOAD_TA;
		gfx_cmd->cmd.cmd_unload_ta.session_id = km_cmd->cmd.unload_ta.session_id;
		break;
	case PSP_CMD_KM_TYPE__INVOKE_CMD:
		/* Populate local GFX CMD struct */
		gfx_cmd->cmd_id = GFX_CMD_ID_INVOKE_CMD;
		gfx_cmd->cmd.cmd_invoke_cmd.session_id = km_cmd->cmd.invoke_ta.session_id;
		gfx_cmd->cmd.cmd_invoke_cmd.ta_cmd_id = km_cmd->cmd.invoke_ta.ta_cmd_id;
		break;
	case PSP_CMD_KM_TYPE__LOAD_IP_FW:
		/* Populate local GFX CMD struct */
		gfx_cmd->cmd_id = GFX_CMD_ID_LOAD_IP_FW;
		gfx_cmd->cmd.cmd_load_ip_fw.fw_phy_addr_lo = km_cmd->cmd.load_ip_fw.fw_addr_lo;
		gfx_cmd->cmd.cmd_load_ip_fw.fw_phy_addr_hi = km_cmd->cmd.load_ip_fw.fw_addr_hi;
		gfx_cmd->cmd.cmd_load_ip_fw.fw_size = km_cmd->cmd.load_ip_fw.fw_size;
		gfx_cmd->cmd.cmd_load_ip_fw.fw_type =
			amdgv_psp_cmd_km_fw_id_map(km_cmd->cmd.load_ip_fw.fw_type);
		break;

	case PSP_CMD_KM_TYPE__SETUP_TMR:
		/* Populate local GFX CMD struct */
		gfx_cmd->cmd_id = GFX_CMD_ID_SETUP_TMR;
		gfx_cmd->cmd.cmd_setup_tmr.buf_phy_addr_lo =
			km_cmd->cmd.setup_tmr.tmr_buf_addr_lo;
		gfx_cmd->cmd.cmd_setup_tmr.buf_phy_addr_hi =
			km_cmd->cmd.setup_tmr.tmr_buf_addr_hi;
		gfx_cmd->cmd.cmd_setup_tmr.buf_size = km_cmd->cmd.setup_tmr.tmr_size;
		gfx_cmd->cmd.cmd_setup_tmr.sriov_params =
			km_cmd->cmd.setup_tmr.tmr_sriov_params;
		break;

	case PSP_CMD_KM_TYPE__DESTROY_TMR:
		/* Populate local GFX CMD struct */
		gfx_cmd->cmd_id = GFX_CMD_ID_DESTROY_TMR;
		gfx_cmd->cmd.cmd_setup_tmr.buf_phy_addr_lo =
			km_cmd->cmd.setup_tmr.tmr_buf_addr_lo;
		gfx_cmd->cmd.cmd_setup_tmr.buf_phy_addr_hi =
			km_cmd->cmd.setup_tmr.tmr_buf_addr_hi;
		gfx_cmd->cmd.cmd_setup_tmr.buf_size = km_cmd->cmd.setup_tmr.tmr_size;
		break;

	case PSP_CMD_KM_TYPE__LOAD_TOC:
		/* Populate local GFX CMD struct */
		gfx_cmd->cmd_id = GFX_CMD_ID_LOAD_TOC;
		gfx_cmd->cmd.cmd_load_toc.toc_phy_addr_lo = km_cmd->cmd.load_toc.toc_addr_lo;
		gfx_cmd->cmd.cmd_load_toc.toc_phy_addr_hi = km_cmd->cmd.load_toc.toc_addr_hi;
		gfx_cmd->cmd.cmd_load_toc.toc_size = km_cmd->cmd.load_toc.toc_size;
		break;

	case PSP_CMD_KM_TYPE__AUTOLOAD_RLC:
		/* Populate local GFX CMD struct */
		gfx_cmd->cmd_id = GFX_CMD_ID_AUTOLOAD_RLC;
		break;

	case PSP_CMD_KM_TYPE__GBR_IH_REG:
		gfx_cmd->cmd_id = GFX_CMD_ID_GBR_IH_REG;
		gfx_cmd->cmd.cmd_program_reg.reg_value = km_cmd->cmd.program_reg.reg_value;
		gfx_cmd->cmd.cmd_program_reg.reg_id = km_cmd->cmd.program_reg.reg_id;
		gfx_cmd->cmd.cmd_program_reg.reg_value_hi =
			km_cmd->cmd.program_reg.reg_value_hi;
		/*NV3x*/
		gfx_cmd->cmd.cmd_program_reg.target_vfid = km_cmd->cmd.program_reg.target_vfid;

		break;

	case PSP_CMD_KM_TYPE__VFGATE:
		gfx_cmd->cmd_id = GFX_CMD_ID_VFGATE;
		gfx_cmd->cmd.cmd_vfgate.action = km_cmd->cmd.vfgate.action;
		gfx_cmd->cmd.cmd_vfgate.target_vfid = km_cmd->cmd.vfgate.target_vfid;
		break;

	case PSP_CMD_KM_TYPE__NUM_ENABLED_VFS:
		gfx_cmd->cmd_id = GFX_CMD_ID_NUM_ENABLED_VFS;
		gfx_cmd->cmd.cmd_num_vfs.number_of_vfs = km_cmd->cmd.num_vfs.number_of_vfs;
		break;

	case PSP_CMD_KM_TYPE__CLEAR_VF_FW:
		gfx_cmd->cmd_id = GFX_CMD_ID_CLEAR_VF_FW;
		gfx_cmd->cmd.cmd_clear_vf_fw.target_vf = km_cmd->cmd.clear_vf_fw.target_vf;
		break;

	case PSP_CMD_KM_TYPE__VF_RELAY:
		gfx_cmd->cmd_id = GFX_CMD_ID_VF_RELAY;
		gfx_cmd->cmd.cmd_vf_relay.target_vf = km_cmd->cmd.vf_relay.target_vf;
		gfx_cmd->cmd.cmd_vf_relay.vf_relay_wtr_ptr =
			km_cmd->cmd.vf_relay.vf_relay_wtr_ptr;
		break;

	case PSP_CMD_KM_TYPE__DBG_SNAPSHOT_SET_ADDR:
		gfx_cmd->cmd_id = GFX_CMD_ID_DBG_SNAPSHOT_SET_ADDR;
		gfx_cmd->cmd.cmd_dbg_snapshot_setaddr.addr_hi =
			km_cmd->cmd.dbg_snapshot_setaddr.addr_hi;
		gfx_cmd->cmd.cmd_dbg_snapshot_setaddr.addr_lo =
			km_cmd->cmd.dbg_snapshot_setaddr.addr_lo;
		gfx_cmd->cmd.cmd_dbg_snapshot_setaddr.size =
			km_cmd->cmd.dbg_snapshot_setaddr.size;
		break;

	case PSP_CMD_KM_TYPE__DBG_SNAPSHOT_TRIGGER:
		gfx_cmd->cmd_id = GFX_CMD_ID_DBG_SNAPSHOT_TRIGGER;
		gfx_cmd->cmd.cmd_dbg_snapshot_trigger.target_vfid =
			km_cmd->cmd.dbg_snapshot_trigger.target_vfid;
		gfx_cmd->cmd.cmd_dbg_snapshot_trigger.sections =
			km_cmd->cmd.dbg_snapshot_trigger.sections;
		gfx_cmd->cmd.cmd_dbg_snapshot_trigger.aid_mask =
			km_cmd->cmd.dbg_snapshot_trigger.aid_mask;
		gfx_cmd->cmd.cmd_dbg_snapshot_trigger.xcc_mask =
			km_cmd->cmd.dbg_snapshot_trigger.xcc_mask;
		break;

	case PSP_CMD_KM_TYPE__DUMP_TRACELOG:
		gfx_cmd->cmd_id = GFX_CMD_ID_DUMP_TRACELOG;
		gfx_cmd->cmd.cmd_dump_tracelog.addr_lo = km_cmd->cmd.dump_tracelog.addr_lo;
		gfx_cmd->cmd.cmd_dump_tracelog.addr_hi = km_cmd->cmd.dump_tracelog.addr_hi;
		gfx_cmd->cmd.cmd_dump_tracelog.size = km_cmd->cmd.dump_tracelog.size;
		break;

	case PSP_CMD_KM_TYPE__MIGRATION_GET_PSP_INFO:
		gfx_cmd->cmd_id = GFX_CMD_ID_MIGRATION_GET_PSP_INFO;
		gfx_cmd->cmd.cmd_migration_get_psp_info.migration_version =
			km_cmd->cmd.migration_get_psp_info.migration_version;
		break;

	case PSP_CMD_KM_TYPE__MIGRATION_EXPORT:
		gfx_cmd->cmd_id = GFX_CMD_ID_MIGRATION_EXPORT;
		gfx_cmd->cmd.cmd_migration_export.pkg_addr_hi =
			km_cmd->cmd.migration_export.pkg_addr_hi;
		gfx_cmd->cmd.cmd_migration_export.pkg_addr_lo =
			km_cmd->cmd.migration_export.pkg_addr_lo;
		gfx_cmd->cmd.cmd_migration_export.pkg_size_allocated =
			km_cmd->cmd.migration_export.pkg_size_allocated;
		gfx_cmd->cmd.cmd_migration_export.target_vfid =
			km_cmd->cmd.migration_export.target_vfid;
		gfx_cmd->cmd.cmd_migration_export.flags = km_cmd->cmd.migration_export.flags;
		break;

	case PSP_CMD_KM_TYPE__MIGRATION_IMPORT:
		gfx_cmd->cmd_id = GFX_CMD_ID_MIGRATION_IMPORT;
		gfx_cmd->cmd.cmd_migration_import.pkg_addr_hi =
			km_cmd->cmd.migration_import.pkg_addr_hi;
		gfx_cmd->cmd.cmd_migration_import.pkg_addr_lo =
			km_cmd->cmd.migration_import.pkg_addr_lo;
		gfx_cmd->cmd.cmd_migration_import.pkg_size =
			km_cmd->cmd.migration_import.pkg_size;
		gfx_cmd->cmd.cmd_migration_import.target_vfid =
			km_cmd->cmd.migration_import.target_vfid;
		break;

	case PSP_CMD_KM_TYPE__FW_ATTESTATION:
		gfx_cmd->cmd_id = GFX_CMD_ID_FW_ATTESTATION;
		break;

	case PSP_CMD_KM_TYPE__SRIOV_SPATIAL_PART:
		gfx_cmd->cmd_id = GFX_CMD_ID_SRIOV_SPATIAL_PART;
		gfx_cmd->cmd.cmd_sriov_spatial_part.mode =
			km_cmd->cmd.sriov_spatial_part.mode;
		gfx_cmd->cmd.cmd_sriov_spatial_part.override_ips =
			km_cmd->cmd.sriov_spatial_part.override_ips;
		gfx_cmd->cmd.cmd_sriov_spatial_part.override_xcds_avail =
			km_cmd->cmd.sriov_spatial_part.override_xcds_avail;
		gfx_cmd->cmd.cmd_sriov_spatial_part.override_this_aid =
			km_cmd->cmd.sriov_spatial_part.override_this_aid;
		break;

	case PSP_CMD_KM_TYPE__NPS_MODE:
		gfx_cmd->cmd_id = GFX_CMD_ID_NPS_MODE;
		gfx_cmd->cmd.cmd_sriov_memory_part.num_parts =
			km_cmd->cmd.sriov_memory_part.num_parts;
		break;

	case PSP_CMD_KMD_TYPE_SRIOV_COPY_VF_CHIPLET_REGS:
		gfx_cmd->cmd_id = GFX_CMD_ID_SRIOV_COPY_VF_CHIPLET_REGS;
		gfx_cmd->cmd.cmd_sriov_copy_vf_chiplet_regs.source_vfid =
			km_cmd->cmd.sriov_copy_vf_chiplet_regs.source_vfid;
		break;

	default:
		ret = PSP_STATUS__ERROR_GENERIC;
		break;
	}

	return ret;
}

static int amdgv_psp_wait_for_ras_intr_cb(void *context)
{
	return amdgv_ras_intr_triggered() ? 0 : 1;
}

enum psp_status amdgv_psp_wait_for_memory(struct amdgv_adapter *adapt,
					  uint32_t *memory_address, uint32_t memory_value)
{
	int wait_ret;

	wait_ret = amdgv_wait_for_memory(adapt, memory_address, memory_value,
					 AMDGV_TIMEOUT(TIMEOUT_PSP_MEM));

	if (!wait_ret) {
		AMDGV_DEBUG4("PSP responded successfully: "
			     "memory_value(expected)=0x%08x memory_value(readback)=0x%08x\n",
			     memory_value, *memory_address);
		return PSP_STATUS__SUCCESS;
	} else {
		AMDGV_DEBUG("PSP: TIMED-OUT waiting for PSP response! "
			    "memory_value(expected)=0x%08x memory_value(readback)=0x%08x\n",
			    memory_value, *memory_address);

		return PSP_STATUS__ERROR_GENERIC;
	}
}

enum psp_status amdgv_psp_cmd_km_fence_wait(struct amdgv_adapter *adapt,
					    struct psp_context *psp,
					    struct psp_cmd_km_handle *km_cmd_handle,
					    struct psp_gfx_resp *psp_resp)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_local_memory *local_mem = &psp->km_cmd_context.km_fence_mem_handle;
	struct psp_cmd_km_buf *psp_gfx_cmd_buf = NULL;
	struct psp_gfx_cmd_resp *gfx_cmd = NULL;
	uint32_t index;

	if (km_cmd_handle == NULL || km_cmd_handle->index == PSP_KM_CMD_INVALID_INDEX) {
		return PSP_STATUS__ERROR_INVALID_PARAMS;
	}

	index = km_cmd_handle->index;

	psp_gfx_cmd_buf = &psp->km_cmd_context.km_cmd_buf_pool[index];

	/* Wait for fence from PSP FW */
	if (local_mem->mem == NULL)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	ret = amdgv_psp_wait_for_memory(adapt,
					(uint32_t *)amdgv_memmgr_get_cpu_addr(local_mem->mem),
					psp_gfx_cmd_buf->fence_value);

	if (psp_resp) {
		gfx_cmd = (struct psp_gfx_cmd_resp *)(amdgv_memmgr_get_cpu_addr(
			psp->km_cmd_context.km_cmd_buf_pool[index].cmd_mem.mem));
		if (gfx_cmd == NULL)
			return PSP_STATUS__ERROR_OUT_OF_MEMORY;
		*psp_resp = gfx_cmd->resp;
	}

	return ret;
}

enum psp_status amdgv_psp_cmd_km_submit(struct amdgv_adapter *adapt,
					struct psp_cmd_km *input_index,
					struct psp_gfx_resp *psp_resp)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km_handle buf_handle = { 0 };
	struct psp_gfx_resp gfx_cmd_resp = { 0 };
	struct psp_cmd_km_buf *psp_gfx_cmd_buf;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *local_mem = &psp->km_cmd_context.km_fence_mem_handle;

	/* Add to diagnosis data trace log the start of PSP command */
	AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_START(input_index->cmd_id);

	if (amdgv_psp_cmd_km_allocate_buf(psp, &buf_handle) != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_CMD_ALLOC_BUF_FAIL, 0);
		return PSP_STATUS__ERROR_GENERIC;
	}

	if (amdgv_psp_cmd_km_buf_prep(psp, input_index, &buf_handle) != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_CMD_BUF_PREP_FAIL, 0);
		amdgv_psp_cmd_km_release_buf(psp, &buf_handle);
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Add to diagnosis data trace log the param of PSP command */
	AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_PARAM(input_index->cmd_id, input_index->cmd);

	psp->km_cmd_context.km_fence_count++;

	psp_gfx_cmd_buf = &psp->km_cmd_context.km_cmd_buf_pool[buf_handle.index];

	/* Update fence value associated with CMD buffer */
	psp_gfx_cmd_buf->fence_value = psp->km_cmd_context.km_fence_count;

	/* Submit Gfx CMD to PSP FW */
	if (!psp_gfx_cmd_buf->cmd_mem.mem || !local_mem->mem ||
	    amdgv_psp_ring_km_submit(adapt,
				     amdgv_memmgr_get_gpu_addr(psp_gfx_cmd_buf->cmd_mem.mem),
				     amdgv_memmgr_get_gpu_addr(local_mem->mem),
				     psp_gfx_cmd_buf->fence_value) != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_CMD_SUBMIT_FAIL, 0);
		/* Submission failed decrement fence counter */
		psp->km_cmd_context.km_fence_count--;
		/* Release CMD buffer */
		amdgv_psp_cmd_km_release_buf(psp, &buf_handle);
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Add to diagnosis data trace log the wait of PSP command */
	AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_WAIT(input_index->cmd_id,
					  psp->km_cmd_context.km_fence_count);

	/* Wait for response from PSP FW */
	ret = amdgv_psp_cmd_km_fence_wait(adapt, psp, &buf_handle, &gfx_cmd_resp);
	if (ret != PSP_STATUS__SUCCESS)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_CMD_FENCE_WAIT_FAIL, 0);

	if (psp_resp)
		*psp_resp = gfx_cmd_resp;

	if ((gfx_cmd_resp.status != 0)) {
		AMDGV_INFO("amdgv_psp_cmd_km_fence_wait() failed "
			   "(gfx_cmd_resp=0x%08x)\n",
			   gfx_cmd_resp.status);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	/* Release CMD buffer */
	amdgv_psp_cmd_km_release_buf(psp, &buf_handle);

	/* Add to diagnosis data trace log the finish of PSP command */
	AMDGV_DIAG_DATA_TRACE_LOG_PSP_CMD_FINISH(input_index->cmd_id, gfx_cmd_resp.status, ret);

	return ret;
}

enum psp_bootloader_command_list amdgv_psp_bl_command_map(enum amdgv_firmware_id fw_id)
{
	enum psp_bootloader_command_list psp_bl_cmd_id = 0;

	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__PSP_SYS:
		psp_bl_cmd_id = PSP_BL__LOAD_SYSDRV;
		break;
	case AMDGV_FIRMWARE_ID__PSP_SOS:
		psp_bl_cmd_id = PSP_BL__LOAD_SOSDRV;
		break;
	case AMDGV_FIRMWARE_ID__PSP_SOC:
		psp_bl_cmd_id = PSP_BL__LOAD_SOCDRV;
		break;
	case AMDGV_FIRMWARE_ID__PSP_KEYDB:
		psp_bl_cmd_id = PSP_BL__LOAD_KEY_DATABASE;
		break;
	case AMDGV_FIRMWARE_ID__PSP_INTF:
		psp_bl_cmd_id = PSP_BL__LOAD_INTFDRV;
		break;
	case AMDGV_FIRMWARE_ID__PSP_DBG:
		psp_bl_cmd_id = PSP_BL__LOAD_DBGDRV;
		break;
	default:
		psp_bl_cmd_id = 0;
		break;
	}

	return psp_bl_cmd_id;
}

enum psp_gfx_fw_type amdgv_psp_cmd_km_fw_id_map(enum amdgv_firmware_id fw_id)
{
	enum psp_gfx_fw_type psp_sos_fw_id = GFX_FW_TYPE_NONE;

	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__SMU:
		psp_sos_fw_id = GFX_FW_TYPE_SMU;
		break;
	case AMDGV_FIRMWARE_ID__CP_CE:
		psp_sos_fw_id = GFX_FW_TYPE_CP_CE;
		break;
	case AMDGV_FIRMWARE_ID__CP_PFP:
		psp_sos_fw_id = GFX_FW_TYPE_CP_PFP;
		break;
	case AMDGV_FIRMWARE_ID__CP_ME:
		psp_sos_fw_id = GFX_FW_TYPE_CP_ME;
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
		psp_sos_fw_id = GFX_FW_TYPE_CP_MEC_ME1;
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC_JT2:
		psp_sos_fw_id = GFX_FW_TYPE_CP_MEC_ME2;
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC1:
		psp_sos_fw_id = GFX_FW_TYPE_CP_MEC;
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC2:
		psp_sos_fw_id = GFX_FW_TYPE_CP_MEC;
		break;
	case AMDGV_FIRMWARE_ID__RLC:
		psp_sos_fw_id = GFX_FW_TYPE_RLC_G;
		break;
	case AMDGV_FIRMWARE_ID__SDMA0:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA0;
		break;
	case AMDGV_FIRMWARE_ID__SDMA1:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA1;
		break;
	case AMDGV_FIRMWARE_ID__SDMA2:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA2;
		break;
	case AMDGV_FIRMWARE_ID__SDMA3:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA3;
		break;
	case AMDGV_FIRMWARE_ID__SDMA4:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA4;
		break;
	case AMDGV_FIRMWARE_ID__SDMA5:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA5;
		break;
	case AMDGV_FIRMWARE_ID__SDMA6:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA6;
		break;
	case AMDGV_FIRMWARE_ID__SDMA7:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA7;
		break;
	case AMDGV_FIRMWARE_ID__VCN:
		psp_sos_fw_id = GFX_FW_TYPE_VCN;
		break;
	case AMDGV_FIRMWARE_ID__UVD:
		psp_sos_fw_id = GFX_FW_TYPE_UVD;
		break;
	case AMDGV_FIRMWARE_ID__VCE:
		psp_sos_fw_id = GFX_FW_TYPE_VCE;
		break;
	case AMDGV_FIRMWARE_ID__ISP:
		psp_sos_fw_id = GFX_FW_TYPE_ISP;
		break;
	case AMDGV_FIRMWARE_ID__DMCU_ERAM:
		psp_sos_fw_id = GFX_FW_TYPE_DMCU_ERAM;
		break;
	case AMDGV_FIRMWARE_ID__DMCU_ISR:
		psp_sos_fw_id = GFX_FW_TYPE_DMCU_ISR;
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
		psp_sos_fw_id = GFX_FW_TYPE_RLC_RESTORE_LIST_GPM_MEM;
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
		psp_sos_fw_id = GFX_FW_TYPE_RLC_RESTORE_LIST_SRM_MEM;
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL:
		psp_sos_fw_id = GFX_FW_TYPE_RLC_RESTORE_LIST_SRM_CNTL;
		break;
	case AMDGV_FIRMWARE_ID__RLC_V:
		psp_sos_fw_id = GFX_FW_TYPE_RLC_V;
		break;
	case AMDGV_FIRMWARE_ID__RLC_P:
		psp_sos_fw_id = GFX_FW_TYPE_RLC_P;
		break;
	case AMDGV_FIRMWARE_ID__MMSCH:
		psp_sos_fw_id = GFX_FW_TYPE_MMSCH;
		break;
	case AMDGV_FIRMWARE_ID__DFC_FW:
		psp_sos_fw_id = GFX_FW_TYPE_DFC_FW;
		break;
	case AMDGV_FIRMWARE_ID__DRV_CAP:
		psp_sos_fw_id = GFX_FW_TYPE_DRV_CAP;
		break;
	case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
		psp_sos_fw_id = GFX_FW_TYPE_REG_ACCESS_WHITELIST;
		break;
	case AMDGV_FIRMWARE_ID__IMU_DRAM:
		psp_sos_fw_id = GFX_FW_TYPE_IMU_D;
		break;
	case AMDGV_FIRMWARE_ID__IMU_IRAM:
		psp_sos_fw_id = GFX_FW_TYPE_IMU_I;
		break;
	case AMDGV_FIRMWARE_ID__RLX6:
		psp_sos_fw_id = GFX_FW_TYPE_RLC_IRAM;
		break;
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT:
		psp_sos_fw_id = GFX_FW_TYPE_RLC_DRAM_BOOT;
		break;
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA_UCODE_TH0;
		break;
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1:
		psp_sos_fw_id = GFX_FW_TYPE_SDMA_UCODE_TH1;
		break;
	case AMDGV_FIRMWARE_ID__CP_MES:
		psp_sos_fw_id = GFX_FW_TYPE_CP_MES;
		break;
	case AMDGV_FIRMWARE_ID__MES_STACK:
		psp_sos_fw_id = GFX_FW_TYPE_MES_STACK;
		break;
	case AMDGV_FIRMWARE_ID__MES_THREAD1:
		psp_sos_fw_id = GFX_FW_TYPE_CP_MES_KIQ;
		break;
	case AMDGV_FIRMWARE_ID__MES_THREAD1_STACK:
		psp_sos_fw_id = GFX_FW_TYPE_MES_KIQ_STACK;
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_UCODE:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_ME;
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_ME_P0_STACK;
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_P1_DATA:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_ME_P1_STACK;
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_UCODE:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_PFP;
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_PFP_P0_STACK;
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_P1_DATA:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_PFP_P1_STACK;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_MEC;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_MEC_P0_STACK;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_MEC_P1_STACK;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_MEC_P2_STACK;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA:
		psp_sos_fw_id = GFX_FW_TYPE_RS64_MEC_P3_STACK;
		break;
	case AMDGV_FIRMWARE_ID__PPTABLE:
		psp_sos_fw_id = GFX_FW_TYPE_PPTABLE;
		break;
	case AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1:
		psp_sos_fw_id = GFX_FW_TYPE_RLX6_UCODE_CORE1;
		break;
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1:
		psp_sos_fw_id = GFX_FW_TYPE_RLX6_DRAM_BOOT_CORE1;
		break;
	case AMDGV_FIRMWARE_ID__RLCV_LX7:
		psp_sos_fw_id = GFX_FW_TYPE_RLCV_LX7;
		break;
	case AMDGV_FIRMWARE_ID__RLC_SAVE_RESTROE_LIST:
		psp_sos_fw_id = GFX_FW_TYPE_RLC_SAVE_RESTROE_LIST;
		break;
	case AMDGV_FIRMWARE_ID__P2S_TABLE:
		psp_sos_fw_id = GFX_FW_TYPE_P2S_TABLE;
		break;
	case AMDGV_FIRMWARE_ID__MAX:

	default:
		psp_sos_fw_id = GFX_FW_TYPE_NONE;
		break;
	}

	return psp_sos_fw_id;
}

enum amdgv_firmware_id amdgv_psp_gfx_fw_id_map(enum psp_gfx_fw_type gfx_fw_id)
{
	enum amdgv_firmware_id fw_id = AMDGV_FIRMWARE_ID__MAX;

	switch (gfx_fw_id) {
	case GFX_FW_TYPE_CP_ME:
		fw_id = AMDGV_FIRMWARE_ID__CP_ME;
		break;
	case GFX_FW_TYPE_CP_PFP:
		fw_id = AMDGV_FIRMWARE_ID__CP_PFP;
		break;
	case GFX_FW_TYPE_CP_CE:
		fw_id = AMDGV_FIRMWARE_ID__CP_CE;
		break;
	case GFX_FW_TYPE_CP_MEC:
		fw_id = AMDGV_FIRMWARE_ID__CP_MEC1;
		break;
	case GFX_FW_TYPE_CP_MEC_ME1:
		fw_id = AMDGV_FIRMWARE_ID__CP_MEC_JT1;
		break;
	case GFX_FW_TYPE_CP_MEC_ME2:
		fw_id = AMDGV_FIRMWARE_ID__CP_MEC_JT2;
		break;
	case GFX_FW_TYPE_RLC_V:
		fw_id = AMDGV_FIRMWARE_ID__RLC_V;
		break;
	case GFX_FW_TYPE_RLC_P:
		fw_id = AMDGV_FIRMWARE_ID__RLC_P;
		break;
	case GFX_FW_TYPE_RLC_G:
		fw_id = AMDGV_FIRMWARE_ID__RLC;
		break;
	case GFX_FW_TYPE_SDMA0:
		fw_id = AMDGV_FIRMWARE_ID__SDMA0;
		break;
	case GFX_FW_TYPE_SDMA1:
		fw_id = AMDGV_FIRMWARE_ID__SDMA1;
		break;
	case GFX_FW_TYPE_SDMA2:
		fw_id = AMDGV_FIRMWARE_ID__SDMA2;
		break;
	case GFX_FW_TYPE_SDMA3:
		fw_id = AMDGV_FIRMWARE_ID__SDMA3;
		break;
	case GFX_FW_TYPE_SDMA4:
		fw_id = AMDGV_FIRMWARE_ID__SDMA4;
		break;
	case GFX_FW_TYPE_SDMA5:
		fw_id = AMDGV_FIRMWARE_ID__SDMA5;
		break;
	case GFX_FW_TYPE_SDMA6:
		fw_id = AMDGV_FIRMWARE_ID__SDMA6;
		break;
	case GFX_FW_TYPE_SDMA7:
		fw_id = AMDGV_FIRMWARE_ID__SDMA7;
		break;
	case GFX_FW_TYPE_DMCU_ERAM:
		fw_id = AMDGV_FIRMWARE_ID__DMCU_ERAM;
		break;
	case GFX_FW_TYPE_DMCU_ISR:
		fw_id = AMDGV_FIRMWARE_ID__DMCU_ISR;
		break;
	case GFX_FW_TYPE_VCN:
		fw_id = AMDGV_FIRMWARE_ID__VCN;
		break;
	case GFX_FW_TYPE_UVD:
		fw_id = AMDGV_FIRMWARE_ID__UVD;
		break;
	case GFX_FW_TYPE_VCE:
		fw_id = AMDGV_FIRMWARE_ID__VCE;
		break;
	case GFX_FW_TYPE_ISP:
		fw_id = AMDGV_FIRMWARE_ID__ISP;
		break;
	case GFX_FW_TYPE_SMU:
		fw_id = AMDGV_FIRMWARE_ID__SMU;
		break;
	case GFX_FW_TYPE_MMSCH:
		fw_id = AMDGV_FIRMWARE_ID__MMSCH;
		break;
	case GFX_FW_TYPE_RLC_RESTORE_LIST_GPM_MEM:
		fw_id = AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM;
		break;
	case GFX_FW_TYPE_RLC_RESTORE_LIST_SRM_MEM:
		fw_id = AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM;
		break;
	case GFX_FW_TYPE_RLC_RESTORE_LIST_SRM_CNTL:
		fw_id = AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL;
		break;
	case GFX_FW_TYPE_DFC_FW:
		fw_id = AMDGV_FIRMWARE_ID__DFC_FW;
		break;
	case GFX_FW_TYPE_DRV_CAP:
		fw_id = AMDGV_FIRMWARE_ID__DRV_CAP;
		break;
	case GFX_FW_TYPE_REG_ACCESS_WHITELIST:
		fw_id = AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST;
		break;
	case GFX_FW_TYPE_IMU_D:
		fw_id = AMDGV_FIRMWARE_ID__IMU_DRAM;
		break;
	case GFX_FW_TYPE_IMU_I:
		fw_id = AMDGV_FIRMWARE_ID__IMU_IRAM;
		break;
	case GFX_FW_TYPE_SDMA_UCODE_TH0:
		fw_id = AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0;
		break;
	case GFX_FW_TYPE_SDMA_UCODE_TH1:
		fw_id = AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1;
		break;
	case GFX_FW_TYPE_RLC_IRAM:
		fw_id = AMDGV_FIRMWARE_ID__RLX6;
		break;
	case GFX_FW_TYPE_RLC_DRAM_BOOT:
		fw_id = AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT;
		break;
	case GFX_FW_TYPE_CP_MES:
		fw_id = AMDGV_FIRMWARE_ID__CP_MES;
		break;
	case GFX_FW_TYPE_MES_STACK:
		fw_id = AMDGV_FIRMWARE_ID__MES_STACK;
		break;
	case GFX_FW_TYPE_CP_MES_KIQ:
		fw_id = AMDGV_FIRMWARE_ID__MES_THREAD1;
		break;
	case GFX_FW_TYPE_MES_KIQ_STACK:
		fw_id = AMDGV_FIRMWARE_ID__MES_THREAD1_STACK;
		break;
	case GFX_FW_TYPE_RS64_ME:
		fw_id = AMDGV_FIRMWARE_ID__RS64_ME_UCODE;
		break;
	case GFX_FW_TYPE_RS64_ME_P0_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA;
		break;
	case GFX_FW_TYPE_RS64_ME_P1_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_ME_P1_DATA;
		break;
	case GFX_FW_TYPE_RS64_PFP:
		fw_id = AMDGV_FIRMWARE_ID__RS64_PFP_UCODE;
		break;
	case GFX_FW_TYPE_RS64_PFP_P0_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA;
		break;
	case GFX_FW_TYPE_RS64_PFP_P1_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_PFP_P1_DATA;
		break;
	case GFX_FW_TYPE_RS64_MEC:
		fw_id = AMDGV_FIRMWARE_ID__RS64_MEC_UCODE;
		break;
	case GFX_FW_TYPE_RS64_MEC_P0_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA;
		break;
	case GFX_FW_TYPE_RS64_MEC_P1_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA;
		break;
	case GFX_FW_TYPE_RS64_MEC_P2_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA;
		break;
	case GFX_FW_TYPE_RS64_MEC_P3_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA;
		break;
	case GFX_FW_TYPE_PPTABLE:
		fw_id = AMDGV_FIRMWARE_ID__PPTABLE;
		break;
	case GFX_FW_TYPE_RLX6_UCODE_CORE1:
		fw_id = AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1;
		break;
	case GFX_FW_TYPE_RLX6_DRAM_BOOT_CORE1:
		fw_id = AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1;
		break;
	case GFX_FW_TYPE_RLCV_LX7:
		fw_id = AMDGV_FIRMWARE_ID__RLCV_LX7;
		break;
	case GFX_FW_TYPE_RLC_SAVE_RESTROE_LIST:
		fw_id = AMDGV_FIRMWARE_ID__RLC_SAVE_RESTROE_LIST;
		break;
	case GFX_FW_TYPE_P2S_TABLE:
		fw_id = AMDGV_FIRMWARE_ID__P2S_TABLE;
		break;
	default:
		fw_id = AMDGV_FIRMWARE_ID__MAX;
		break;
	}

	return fw_id;
}

enum amdgv_firmware_id amdgv_psp_fw_id_map(enum fwman_fw_id man_fw_id)
{
	enum amdgv_firmware_id fw_id = AMDGV_FIRMWARE_ID__MAX;

	switch (man_fw_id) {
	case FW_ID_SDMA0:
		fw_id = AMDGV_FIRMWARE_ID__SDMA0;
		break;
	case FW_ID_SDMA1:
		fw_id = AMDGV_FIRMWARE_ID__SDMA1;
		break;
	case FW_ID_RLC_G:
		fw_id = AMDGV_FIRMWARE_ID__RLC;
		break;
	case FW_ID_RLC_V:
		fw_id = AMDGV_FIRMWARE_ID__RLC_V;
		break;
	case FW_ID_CP_ME:
		fw_id = AMDGV_FIRMWARE_ID__CP_ME;
		break;
	case FW_ID_CP_PFP:
		fw_id = AMDGV_FIRMWARE_ID__CP_PFP;
		break;
	case FW_ID_CP_CE:
		fw_id = AMDGV_FIRMWARE_ID__CP_CE;
		break;
	case FW_ID_CP_MEC:
		fw_id = AMDGV_FIRMWARE_ID__CP_MEC1;
		break;
	case FW_ID_DMCU_ERAM:
		fw_id = AMDGV_FIRMWARE_ID__DMCU_ERAM;
		break;
	case FW_ID_DMCU_ISR:
		fw_id = AMDGV_FIRMWARE_ID__DMCU_ISR;
		break;
	case FW_ID_ISP:
		fw_id = AMDGV_FIRMWARE_ID__ISP;
		break;
	case FW_ID_RLC_RESTORE_LIST_GPM_MEM:
		fw_id = AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM;
		break;
	case FW_ID_RLC_RESTORE_LIST_SRM_MEM:
		fw_id = AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM;
		break;
	case FW_ID_RLC_RESTORE_LIST_SRM_CNTL:
		fw_id = AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL;
		break;
	case FW_ID_RLC_P:
		fw_id = AMDGV_FIRMWARE_ID__RLC_P;
		break;
	case FW_ID_CP_MES:
		fw_id = AMDGV_FIRMWARE_ID__CP_MES;
		break;
	case FW_ID_MES_STACK:
		fw_id = AMDGV_FIRMWARE_ID__MES_STACK;
		break;
	case FW_ID_SDMA2:
		fw_id = AMDGV_FIRMWARE_ID__SDMA2;
		break;
	case FW_ID_SDMA3:
		fw_id = AMDGV_FIRMWARE_ID__SDMA3;
		break;
	case FW_ID_SDMA4:
		fw_id = AMDGV_FIRMWARE_ID__SDMA4;
		break;
	case FW_ID_SDMA5:
		fw_id = AMDGV_FIRMWARE_ID__SDMA5;
		break;
	case FW_ID_SDMA6:
		fw_id = AMDGV_FIRMWARE_ID__SDMA6;
		break;
	case FW_ID_SDMA7:
		fw_id = AMDGV_FIRMWARE_ID__SDMA7;
		break;
	case FW_ID_VCN:
		fw_id = AMDGV_FIRMWARE_ID__VCN;
		break;
	case FW_ID_CP_MEC_ME1:
		fw_id = AMDGV_FIRMWARE_ID__CP_MEC_JT1;
		break;
	case FW_ID_CP_MEC_ME2:
		fw_id = AMDGV_FIRMWARE_ID__CP_MEC_JT2;
		break;
	case FW_ID_SMU:
		fw_id = AMDGV_FIRMWARE_ID__SMU;
		break;
	case FW_ID_REG_ACCESS_WHITELIST:
		fw_id = AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST;
		break;
	case FW_ID_IMU_I:
		fw_id = AMDGV_FIRMWARE_ID__IMU_IRAM;
		break;
	case FW_ID_IMU_D:
		fw_id = AMDGV_FIRMWARE_ID__IMU_DRAM;
		break;
	case FW_ID_SDMA_UCODE_TH0:
		fw_id = AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0;
		break;
	case FW_ID_SDMA_UCODE_TH1:
		fw_id = AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1;
		break;
	case FW_ID_PPTABLE:
		fw_id = AMDGV_FIRMWARE_ID__PPTABLE;
		break;
	case FW_ID_RS64_KIQ:
		fw_id = AMDGV_FIRMWARE_ID__MES_THREAD1;
		break;
	case FW_ID_RS64_KIQ_STACK:
		fw_id = AMDGV_FIRMWARE_ID__MES_THREAD1_STACK;
		break;
	case FW_ID_RS64_PFP:
		fw_id = AMDGV_FIRMWARE_ID__RS64_PFP_UCODE;
		break;
	case FW_ID_RS64_ME:
		fw_id = AMDGV_FIRMWARE_ID__RS64_ME_UCODE;
		break;
	case FW_ID_RS64_MEC:
		fw_id = AMDGV_FIRMWARE_ID__RS64_MEC_UCODE;
		break;
	case FW_ID_RS64_PFP_P0_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA;
		break;
	case FW_ID_RS64_ME_P0_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA;
		break;
	case FW_ID_RS64_MEC_P0_STACK:
		fw_id = AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA;
		break;
	default:
		/* Include unsupported firmwares */
		fw_id = AMDGV_FIRMWARE_ID__MAX;
		break;
	}

	return fw_id;
}

bool amdgv_psp_fw_id_support(uint32_t firmware_id)
{
	enum amdgv_firmware_id fw_id = firmware_id;
	bool support;

	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__SMU:
	case AMDGV_FIRMWARE_ID__CP_CE:
	case AMDGV_FIRMWARE_ID__CP_PFP:
	case AMDGV_FIRMWARE_ID__CP_ME:
	case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
	case AMDGV_FIRMWARE_ID__CP_MEC_JT2:
	case AMDGV_FIRMWARE_ID__CP_MEC1:
	case AMDGV_FIRMWARE_ID__CP_MEC2:
	case AMDGV_FIRMWARE_ID__RLC:
	case AMDGV_FIRMWARE_ID__SDMA0:
	case AMDGV_FIRMWARE_ID__SDMA1:
	case AMDGV_FIRMWARE_ID__SDMA2:
	case AMDGV_FIRMWARE_ID__SDMA3:
	case AMDGV_FIRMWARE_ID__SDMA4:
	case AMDGV_FIRMWARE_ID__SDMA5:
	case AMDGV_FIRMWARE_ID__SDMA6:
	case AMDGV_FIRMWARE_ID__SDMA7:
	case AMDGV_FIRMWARE_ID__VCN:
	case AMDGV_FIRMWARE_ID__UVD:
	case AMDGV_FIRMWARE_ID__VCE:
	case AMDGV_FIRMWARE_ID__ISP:
	case AMDGV_FIRMWARE_ID__DMCU_ERAM:
	case AMDGV_FIRMWARE_ID__DMCU_ISR:
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL:
	case AMDGV_FIRMWARE_ID__RLC_V:
	case AMDGV_FIRMWARE_ID__RLC_P:
	case AMDGV_FIRMWARE_ID__MMSCH:
	case AMDGV_FIRMWARE_ID__PSP_SYS:
	case AMDGV_FIRMWARE_ID__PSP_SOS:
	case AMDGV_FIRMWARE_ID__PSP_TOC:
	case AMDGV_FIRMWARE_ID__PSP_KEYDB:
	case AMDGV_FIRMWARE_ID__DFC_FW:
	case AMDGV_FIRMWARE_ID__PSP_BL:
	case AMDGV_FIRMWARE_ID__PSP_SPL:
	case AMDGV_FIRMWARE_ID__DRV_CAP:
	case AMDGV_FIRMWARE_ID__SEC_POLICY_STAGE2:
	case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
	case AMDGV_FIRMWARE_ID__IMU_IRAM:
	case AMDGV_FIRMWARE_ID__IMU_DRAM:
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0:
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1:
	case AMDGV_FIRMWARE_ID__CP_MES:
	case AMDGV_FIRMWARE_ID__MES_STACK:
	case AMDGV_FIRMWARE_ID__MES_THREAD1:
	case AMDGV_FIRMWARE_ID__MES_THREAD1_STACK:
	case AMDGV_FIRMWARE_ID__RLX6:
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT:
	case AMDGV_FIRMWARE_ID__RS64_ME_UCODE:
	case AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA:
	case AMDGV_FIRMWARE_ID__RS64_ME_P1_DATA:
	case AMDGV_FIRMWARE_ID__RS64_PFP_UCODE:
	case AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA:
	case AMDGV_FIRMWARE_ID__RS64_PFP_P1_DATA:
	case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
	case AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA:
	case AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA:
	case AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA:
	case AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA:
	case AMDGV_FIRMWARE_ID__P2S_TABLE:
	case AMDGV_FIRMWARE_ID__PPTABLE:
	case AMDGV_FIRMWARE_ID__PSP_SOC:
	case AMDGV_FIRMWARE_ID__PSP_DBG:
	case AMDGV_FIRMWARE_ID__PSP_INTF:
	case AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1:
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1:
	case AMDGV_FIRMWARE_ID__RLCV_LX7:
	case AMDGV_FIRMWARE_ID__RLC_SAVE_RESTROE_LIST:
	case AMDGV_FIRMWARE_ID__PSP_RAS:
	case AMDGV_FIRMWARE_ID__RAS_TA:
		support = true;
		break;
	default:
		support = false;
		break;
	}
	return support;
}

/* tmr functions */
/* NV12 pre-allocate tmr mem in sw_init using estimated tmr size */
/* For NV series and onward, psp will caculate the tmr size when
 * libgv issue LOAD_TOC. For VG and MI series, using hardcode tmr
 * size (PSP_TMR_SIZE). The tmr buffer alignemnt remain unchanged
 * across all the ASICs */
enum psp_status amdgv_psp_tmr_init(struct amdgv_adapter *adapt, uint32_t tmr_size)
{
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *local_mem = &(psp->tmr_context);

	/* Allocate TMR in TOP of FB */
	AMDGV_DEBUG("allocate TMR\n");
	local_mem->mem = amdgv_memmgr_alloc_align(&adapt->memmgr_gpu, tmr_size,
						  local_mem->alignment, MEM_PSP_TMR);

	if (!local_mem->mem) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_FB_MEM_FAIL, tmr_size);
		return PSP_STATUS__ERROR_GENERIC;
	}

	AMDGV_INFO("TMR: GPU_ADDR=0x%llx MEM_ADDR=0x%llx MEM_SIZE=0x%llx\n",
		   amdgv_memmgr_get_gpu_addr(local_mem->mem),
		   amdgv_memmgr_get_offset(local_mem->mem),
		   amdgv_memmgr_get_size(local_mem->mem));

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_tmr_fini(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *local_mem = &(psp->tmr_context);

	if (local_mem->mem) {
		amdgv_memmgr_free(local_mem->mem);
		local_mem->mem = NULL;

		oss_memset(&psp->tmr_context, 0, sizeof(struct psp_local_memory));
	}

	return ret;
}

enum psp_status amdgv_psp_tmr_load(struct amdgv_adapter *adapt)
{
	enum psp_status ret;
	uint32_t hi = 0;
	uint32_t lo = 0;
	uint32_t size = 0;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory *local_mem = &(psp->tmr_context);
	struct psp_cmd_km *tmr_km_cmd = psp->psp_cmd_km_mem;

	if (adapt->psp.tmr_context.size && !local_mem->mem) {
		return PSP_STATUS__ERROR_GENERIC;
	} else {
		lo = lower_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
		hi = upper_32_bits(amdgv_memmgr_get_gpu_addr(local_mem->mem));
		size = local_mem->size;
	}

	if (!tmr_km_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Prepare CMD to setup TMR */
	tmr_km_cmd->cmd_id = PSP_CMD_KM_TYPE__SETUP_TMR;
	tmr_km_cmd->cmd.setup_tmr.tmr_size = size;
	tmr_km_cmd->cmd.setup_tmr.tmr_buf_addr_lo = lo;
	tmr_km_cmd->cmd.setup_tmr.tmr_buf_addr_hi = hi;
	tmr_km_cmd->cmd.setup_tmr.tmr_sriov_params = 1;

	/* Submit CMD buffer to setup TMR */
	ret = amdgv_psp_cmd_km_submit(adapt, tmr_km_cmd, NULL);

	if (ret != PSP_STATUS__SUCCESS)
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_TMR_LOAD_FAIL, 0);

	/* Clear system memory used for TMR Load CMD */
	oss_memset(tmr_km_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

enum psp_status amdgv_psp_fw_mem_init(struct amdgv_adapter *adapt)
{
	struct psp_context *psp = &adapt->psp;

	psp->private_fw_memory.alignment = PSP_PRIVATE_IMAGE_ALIGNMENT;
	psp->private_fw_memory.size =
		(PSP_PRIVATE_IMAGE_SIZE + (psp->private_fw_memory.alignment - 1)) &
		(~(psp->private_fw_memory.alignment - 1));

	AMDGV_DEBUG("allocate PRIV_FW\n");
	psp->private_fw_memory.mem =
		amdgv_memmgr_alloc_align(PSP_MEMMGR, psp->private_fw_memory.size,
					 psp->private_fw_memory.alignment, MEM_PSP_PRIVATE);

	if (!psp->private_fw_memory.mem)
		return PSP_STATUS__ERROR_GENERIC;

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_fw_mem_fini(struct amdgv_adapter *adapt)
{
	struct psp_context *psp = &adapt->psp;

	if (psp->private_fw_memory.mem) {
		amdgv_memmgr_free(psp->private_fw_memory.mem);
		psp->private_fw_memory.mem = NULL;

		oss_memset(&psp->private_fw_memory, 0, sizeof(struct psp_local_memory));
	}

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_ras_mem_init(struct amdgv_adapter *adapt)
{
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);

	/* Alloate shared memory buf for RAS TA and libgv communication */
	ras_context->shared_buffer.alignment = PSP_RAS_SHARED_MEM_ALIGNMENT;
	ras_context->shared_buffer.size = PSP_RAS_SHARED_MEM_SIZE;

	AMDGV_DEBUG("allocate RAS\n");
	ras_context->shared_buffer.mem =
		amdgv_memmgr_alloc_align(PSP_MEMMGR, ras_context->shared_buffer.size,
					 ras_context->shared_buffer.alignment, MEM_PSP_RAS);

	if (!ras_context->shared_buffer.mem) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				ras_context->shared_buffer.size);

		return PSP_STATUS__ERROR_GENERIC;
	}

	/* allocate memory to cache RAS TA binary */
	if (!ras_context->ras_bin_buf) {
		ras_context->ras_bin_buf = oss_malloc(AMDGV_FW_SIZE_MAX);

		if (!ras_context->ras_bin_buf) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				AMDGV_FW_SIZE_MAX);

			return PSP_STATUS__ERROR_GENERIC;
		}
	}

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_ras_mem_fini(struct amdgv_adapter *adapt)
{
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);

	/* Free the shared memory */
	if (ras_context->shared_buffer.mem) {
		amdgv_memmgr_free(ras_context->shared_buffer.mem);
		ras_context->shared_buffer.mem = NULL;

		oss_memset(&ras_context->shared_buffer, 0, sizeof(struct psp_local_memory));
	}

	if (ras_context->ras_bin_buf) {
		oss_free(ras_context->ras_bin_buf);
		ras_context->ras_bin_buf = NULL;
	}

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_xgmi_mem_init(struct amdgv_adapter *adapt)
{
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);

	/* Alloate shared memory buf for XGMI TA and libgv communication */
	xgmi_context->shared_buffer.alignment = PSP_XGMI_SHARED_MEM_ALIGNMENT;
	xgmi_context->shared_buffer.size = PSP_XGMI_SHARED_MEM_SIZE;

	AMDGV_DEBUG("allocate XGMI\n");
	xgmi_context->shared_buffer.mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf, xgmi_context->shared_buffer.size,
					 xgmi_context->shared_buffer.alignment, MEM_PSP_XGMI);

	if (!xgmi_context->shared_buffer.mem) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				xgmi_context->shared_buffer.size);

		return PSP_STATUS__ERROR_GENERIC;
	}

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_xgmi_mem_fini(struct amdgv_adapter *adapt)
{
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);

	/* Free the shared memory */
	if (xgmi_context->shared_buffer.mem) {
		amdgv_memmgr_free(xgmi_context->shared_buffer.mem);
		xgmi_context->shared_buffer.mem = NULL;

		oss_memset(&xgmi_context->shared_buffer, 0, sizeof(struct psp_local_memory));
	}

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_ring_km_submit(struct amdgv_adapter *adapt, uint64_t cmd_buf_mc_addr,
					 uint64_t fence_mc_addr, uint32_t fence_value)
{
	struct psp_gfx_rb_frame *write_frame = NULL;
	uint32_t psp_write_ptr_reg = 0;
	uint32_t ring_size_dw = 0;
	uint32_t rb_frame_size_dw = 0;
	uint32_t *write_flush_read_back;
	struct psp_context *psp = &adapt->psp;

	if (!psp->km_ring[psp->idx].ring_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	/* Initialize local vars */
	write_frame = (struct psp_gfx_rb_frame *)amdgv_memmgr_get_cpu_addr(
		psp->km_ring[psp->idx].ring_mem.mem);

	ring_size_dw = (psp->km_ring[psp->idx].ring_mem.size) / 4;
	rb_frame_size_dw = sizeof(struct psp_gfx_rb_frame) / 4;

	/* KM (GPCOM) prepare write pointer */
	psp_write_ptr_reg = psp->get_wptr(adapt);

	/* Check if psp write pointer is invalid */
	if (psp_write_ptr_reg == 0xFFFFFFFF &&
		((adapt->pp.pp_funcs->get_smu_fw_loaded_status &&
		adapt->pp.pp_funcs->get_smu_fw_loaded_status(adapt) == 0) ||
		!adapt->pp.pp_funcs->get_smu_fw_loaded_status)) {
		AMDGV_ERROR("amdgv_psp_ring_km_submit() failed!"
			   " psp_write_ptr_reg: 0x%x\n", psp_write_ptr_reg);
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Update KM RB frame pointer to new frame */
	/* write_frame ptr increments by size of rb_frame in bytes */
	/* psp_write_ptr_reg increments by size of rb_frame in DWORDs */
	write_frame = ((psp_write_ptr_reg % ring_size_dw) == 0) ?
			      write_frame :
			      write_frame + (psp_write_ptr_reg / rb_frame_size_dw);

	/* Initialize KM RB frame */
	oss_memset(write_frame, 0, sizeof(struct psp_gfx_rb_frame));

	/* Update KM RB frame */
	write_frame->cmd_buf_addr_hi = (uint32_t)(cmd_buf_mc_addr >> 32);
	write_frame->cmd_buf_addr_lo = (uint32_t)(cmd_buf_mc_addr);
	write_frame->fence_addr_hi = (uint32_t)(fence_mc_addr >> 32);
	write_frame->fence_addr_lo = (uint32_t)(fence_mc_addr);
	write_frame->fence_value = fence_value;

	/* Do a read to force the write of the frame before writing
	 * write pointer
	 */
	write_flush_read_back = &write_frame->fence_value;
	if (*write_flush_read_back != fence_value) {
		AMDGV_INFO("amdgv_psp_ring_km_submit() failed!"
			   " write_frame(cmd_addr: hi 0x%x lo 0x%x"
			   " fence_addr: hi 0x%x lo 0x%x fence: %d)"
			   " expected fence: %d)\n",
			   write_frame->cmd_buf_addr_hi, write_frame->cmd_buf_addr_lo,
			   write_frame->fence_addr_hi, write_frame->fence_addr_lo,
			   *write_flush_read_back, fence_value);
		return PSP_STATUS__ERROR_GENERIC;
	}

	AMDGV_DEBUG("write_frame(cmd_addr: hi 0x%x lo 0x%x"
		    " fence_addr: hi 0x%x lo 0x%x fence: %d)\n",
		    write_frame->cmd_buf_addr_hi, write_frame->cmd_buf_addr_lo,
		    write_frame->fence_addr_hi, write_frame->fence_addr_lo,
		    *write_flush_read_back);

	/* Update the Write Pointer in DWORDs */
	/* Wrap around if necessary */
	psp_write_ptr_reg = (psp_write_ptr_reg + rb_frame_size_dw) % ring_size_dw;

	/* Update CMD descriptor associated with current CMD submission */
	psp->km_ring[psp->idx].ring_wptr = psp_write_ptr_reg / rb_frame_size_dw;

	/* Update Write Pointer reg */
	psp->set_wptr(adapt, psp_write_ptr_reg);
	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_ring_init(struct amdgv_adapter *adapt)
{
	struct psp_ring *ring;
	struct psp_local_memory *local_mem = NULL;
	struct psp_context *psp = &adapt->psp;

	ring = &psp->km_ring[psp->idx];
	ring->ring_type = PSP_RING_TYPE__KM;
	ring->ring_rptr = 0;
	ring->ring_wptr = 0;

	local_mem = &ring->ring_mem;
	local_mem->alignment = PSP_RING_ALIGNMENT;
	local_mem->size =
		(PSP_RING_SIZE + (local_mem->alignment - 1)) & (~(local_mem->alignment - 1));

	AMDGV_DEBUG("allocate RING_FW\n");
	local_mem->mem = amdgv_memmgr_alloc_align(PSP_MEMMGR, local_mem->size,
						  local_mem->alignment, MEM_PSP_RING);
	if (!local_mem->mem)
		return PSP_STATUS__ERROR_GENERIC;

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_ring_fini(struct amdgv_adapter *adapt)
{
	struct psp_ring *ring;
	struct psp_context *psp = &adapt->psp;

	ring = &psp->km_ring[psp->idx];

	if (ring->ring_mem.mem) {
		amdgv_memmgr_free(ring->ring_mem.mem);
		ring->ring_mem.mem = NULL;
	}

	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_wait_for_register(struct amdgv_adapter *adapt, uint32_t reg_index,
					    uint32_t reg_value, uint32_t reg_mask,
					    bool check_changed)
{
	int wait_ret;

	if (check_changed) {
		reg_mask = ~0; /* mask to ~0 as we don't care about mask here */
		wait_ret = amdgv_wait_for_register(adapt, reg_index, reg_mask, reg_value,
						   AMDGV_TIMEOUT(TIMEOUT_PSP_REG),
						   AMDGV_WAIT_CHECK_NE,
						   AMDGV_WAIT_FLAG_FORCE_YIELD);
		if (!wait_ret) {
			AMDGV_DEBUG("PSP responded successfully: "
				    "readback_value=0x%08x should NOT equal reg_value=0x%08x\n",
				    RREG32(reg_index), reg_value);
			return PSP_STATUS__SUCCESS;
		} else {
			AMDGV_DEBUG("PSP: TIMED-OUT waiting for PSP "
				    "response! readback_value=0x%08x should NOT equal reg_value=0x%08x\n",
				    RREG32(reg_index), reg_value);
		}
	} else {
		wait_ret = amdgv_wait_for_register(adapt, reg_index, reg_mask, reg_value,
						   AMDGV_TIMEOUT(TIMEOUT_PSP_REG),
						   AMDGV_WAIT_CHECK_EQ,
						   AMDGV_WAIT_FLAG_FORCE_YIELD);
		if (!wait_ret) {
			AMDGV_DEBUG("PSP responded successfully: "
				    "readback_value=0x%08x should equal reg_value=0x%08x\n",
				    (RREG32(reg_index) & reg_mask), reg_value);
			return PSP_STATUS__SUCCESS;
		} else {
			AMDGV_DEBUG("PSP: TIMED-OUT waiting for PSP "
				    "response! readback_value=0x%08x should equal reg_value=0x%08x\n",
				    (RREG32(reg_index) & reg_mask), reg_value);
		}
	}

	return PSP_STATUS__ERROR_GENERIC;
}

/* Please be noted to create new cases
 * when there are new firmware need
 * to be supported */
void amdgv_psp_get_fw_info(uint32_t image_version, char *info, uint32_t size, uint32_t fw_id)
{
	switch (fw_id) {
	case AMDGV_FIRMWARE_ID__SMU:
		oss_vsnprintf(info, size, "SMU(%d.%d.%d)", (image_version >> 8) & 0xFF,
			      (image_version >> 16) & 0xFF, (image_version >> 24) & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__SDMA0:
		oss_vsnprintf(info, size, "SDMA0(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__SDMA1:
		oss_vsnprintf(info, size, "SDMA1(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__SDMA2:
		oss_vsnprintf(info, size, "SDMA2(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__SDMA3:
		oss_vsnprintf(info, size, "SDMA3(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__SDMA4:
		oss_vsnprintf(info, size, "SDMA4(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__SDMA5:
		oss_vsnprintf(info, size, "SDMA5(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__SDMA6:
		oss_vsnprintf(info, size, "SDMA6(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__SDMA7:
		oss_vsnprintf(info, size, "SDMA7(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
		oss_vsnprintf(info, size, "RLC_SRLIST_GPM(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
		oss_vsnprintf(info, size, "RLC_SRLIST_SRM(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL:
		oss_vsnprintf(info, size, "RLC_SRLIST_CNTL(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLC:
		oss_vsnprintf(info, size, "RLC_G(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLC_V:
		oss_vsnprintf(info, size, "RLC_V(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLC_P:
		oss_vsnprintf(info, size, "RLC_P(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__CP_CE:
		oss_vsnprintf(info, size, "CP_CE(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__CP_PFP:
		oss_vsnprintf(info, size, "CP_PFP(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__CP_ME:
		oss_vsnprintf(info, size, "CP_ME(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC1:
		oss_vsnprintf(info, size, "CP_MEC1(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
		oss_vsnprintf(info, size, "CP_MEC1_JT(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC2:
		oss_vsnprintf(info, size, "CP_MEC2(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC_JT2:
		oss_vsnprintf(info, size, "CP_MEC2_JT(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0:
		oss_vsnprintf(info, size, "SDMA_UCODE_TH0(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1:
		oss_vsnprintf(info, size, "SDMA_UCODE_TH1(version:%d)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__MMSCH:
		oss_vsnprintf(info, size, "MMSCH(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__UVD:
		oss_vsnprintf(info, size, "UVD(version:%d.%d.%d.%d)",
			      (image_version >> 30) & 0x3, (image_version >> 24) & 0x3F,
			      (image_version >> 8) & 0xFF, image_version & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__VCE:
		oss_vsnprintf(info, size, "VCE(version:%d.%d.%d)",
			      (image_version >> 20) & 0xFFF, (image_version >> 8) & 0xFFF,
			      image_version & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__DFC_FW:
		oss_vsnprintf(info, size, "DFC(version:%x.%x.%x.%x)",
			      (image_version >> 24) & 0xFF, (image_version >> 16) & 0xFF,
			      (image_version >> 8) & 0xFF, image_version & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
		oss_vsnprintf(info, size, "Register WhiteList (version:%x.%x.%x.%x)",
			      (image_version >> 24) & 0xFF, (image_version >> 16) & 0xFF,
			      (image_version >> 8) & 0xFF, image_version & 0xFF);
		break;
	case AMDGV_FIRMWARE_ID__IMU_DRAM:
		oss_vsnprintf(info, size, "IMU-DRAM(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__IMU_IRAM:
		oss_vsnprintf(info, size, "IMU-IRAM(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLX6:
		oss_vsnprintf(info, size, "RLX6(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT:
		oss_vsnprintf(info, size, "RLX6-DRAM(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1:
		oss_vsnprintf(info, size, "RLCV LX7 IRAM(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1:
		oss_vsnprintf(info, size, "RLCV LX7 DRAM(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__CP_MES:
		oss_vsnprintf(info, size, "MES(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__MES_STACK:
		oss_vsnprintf(info, size, "MES DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__MES_THREAD1:
		oss_vsnprintf(info, size, "MES KIQ(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__MES_THREAD1_STACK:
		oss_vsnprintf(info, size, "MES KIQ DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_UCODE:
		oss_vsnprintf(info, size, "RS64 ME UCODE(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA:
		oss_vsnprintf(info, size, "RS64 ME P0 DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_P1_DATA:
		oss_vsnprintf(info, size, "RS64 ME P1 DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_UCODE:
		oss_vsnprintf(info, size, "RS64 PFP UCODE(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA:
		oss_vsnprintf(info, size, "RS64 PFP P0 DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_P1_DATA:
		oss_vsnprintf(info, size, "RS64 PFP P1 DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
		oss_vsnprintf(info, size, "RS64 MEC UCODE(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA:
		oss_vsnprintf(info, size, "RS64 MEC P0 DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA:
		oss_vsnprintf(info, size, "RS64 MEC P1 DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA:
		oss_vsnprintf(info, size, "RS64 MEC P2 DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA:
		oss_vsnprintf(info, size, "RS64 MEC P3 DATA(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__PPTABLE:
		oss_vsnprintf(info, size, "PPTABLE(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLCV_LX7:
		oss_vsnprintf(info, size, "RLCV LX7(version:0x%x)", image_version);
		break;
	case AMDGV_FIRMWARE_ID__RLC_SAVE_RESTROE_LIST:
		oss_vsnprintf(info, size, "RLC SAVE RESTROE LIST(version:0x%x)",
			      image_version);
		break;
	case AMDGV_FIRMWARE_ID__P2S_TABLE:
		oss_vsnprintf(info, size, "P2S_TABLE(version:0x%x)", image_version);
		break;
	default:
		oss_vsnprintf(info, size, "UNKNOWN_UCODE_ID(%d)", fw_id);
		break;
	}
}

enum psp_status amdgv_psp_load_np_fw(struct amdgv_adapter *adapt, unsigned char *fw_image,
				     uint32_t fw_image_size, uint32_t fw_id)
{
	enum psp_status ret;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory psp_np_mem = psp->private_fw_memory;
	struct psp_cmd_km *fw_load_km_cmd = psp->psp_cmd_km_mem;
	struct psp_fw_image_header *fw_header;
	char fw_info[64];

	if (!psp_np_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	if (!(adapt->flags & AMDGV_FLAG_EMU_MODE))
		oss_memset(amdgv_memmgr_get_cpu_addr(psp_np_mem.mem), 0,
			   PSP_PRIVATE_IMAGE_SIZE);
	oss_memcpy(amdgv_memmgr_get_cpu_addr(psp_np_mem.mem), fw_image, fw_image_size);

	if (!fw_load_km_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	fw_load_km_cmd->cmd_id = PSP_CMD_KM_TYPE__LOAD_IP_FW;
	fw_load_km_cmd->cmd.load_ip_fw.fw_type = fw_id;
	fw_load_km_cmd->cmd.load_ip_fw.fw_size = fw_image_size;
	fw_load_km_cmd->cmd.load_ip_fw.fw_addr_lo =
		lower_32_bits(amdgv_memmgr_get_gpu_addr(psp_np_mem.mem));
	fw_load_km_cmd->cmd.load_ip_fw.fw_addr_hi =
		upper_32_bits(amdgv_memmgr_get_gpu_addr(psp_np_mem.mem));

	fw_header = (struct psp_fw_image_header *)fw_image;
	amdgv_psp_get_fw_info(fw_header->image_version, fw_info, sizeof(fw_info), fw_id);

	AMDGV_DEBUG("loading %s with command\n"
		    "cmd_id: 0x%x, type: 0x%x, size: 0x%x,"
		    "addr_lo: 0x%x addr_hi: 0x%x\n",
		    fw_info, fw_load_km_cmd->cmd_id, fw_load_km_cmd->cmd.load_ip_fw.fw_type,
		    fw_load_km_cmd->cmd.load_ip_fw.fw_size,
		    fw_load_km_cmd->cmd.load_ip_fw.fw_addr_lo,
		    fw_load_km_cmd->cmd.load_ip_fw.fw_addr_hi);

	ret = amdgv_psp_cmd_km_submit(adapt, fw_load_km_cmd, NULL);
	if (ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL, fw_id);
		ret = PSP_STATUS__ERROR_GENERIC;
		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_FW_LOAD_ERROR,
				  "Firmware load failed.%s", fw_info);
		goto out;
	} else
		amdgv_psp_record_loaded_fw(adapt, fw_image, fw_id);

	AMDGV_INFO("loaded %s\n", fw_info);
	adapt->psp.fw_info[fw_id] = fw_header->image_version;

out:
	/* Clear system memory used for FW Load CMD */
	oss_memset(fw_load_km_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

enum psp_status amdgv_psp_load_toc(struct amdgv_adapter *adapt, unsigned char *toc_image,
				   uint32_t toc_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_context *psp = &adapt->psp;
	struct psp_local_memory load_mem = psp->private_fw_memory;
	struct psp_cmd_km *toc_cmd = psp->psp_cmd_km_mem;
	struct psp_gfx_resp psp_resp = { 0 };

	if (!load_mem.mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	oss_memset(amdgv_memmgr_get_cpu_addr(load_mem.mem), 0, PSP_PRIVATE_IMAGE_SIZE);
	oss_memcpy(amdgv_memmgr_get_cpu_addr(load_mem.mem), toc_image, toc_size);

	if (!toc_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	toc_cmd->cmd_id = PSP_CMD_KM_TYPE__LOAD_TOC;
	toc_cmd->cmd.load_toc.toc_size = toc_size;
	toc_cmd->cmd.load_toc.toc_addr_lo =
		lower_32_bits(amdgv_memmgr_get_gpu_addr(load_mem.mem));
	toc_cmd->cmd.load_toc.toc_addr_hi =
		upper_32_bits(amdgv_memmgr_get_gpu_addr(load_mem.mem));

	ret = amdgv_psp_cmd_km_submit(adapt, toc_cmd, &psp_resp);

	if (ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_TOC_LOAD_FAIL, 0);
		ret = PSP_STATUS__ERROR_GENERIC;
	} else if (psp_resp.tmr_size) {
		/* save tmr_size for TMR load */
		struct psp_local_memory *tmr_mem = &(psp->tmr_context);

		tmr_mem->size = psp_resp.tmr_size;
		tmr_mem->alignment = PSP_TMR_ALIGNMENT;
		tmr_mem->size = (tmr_mem->size + (tmr_mem->alignment - 1)) &
				(~(tmr_mem->alignment - 1));
	}

	/* Clear system memory used for TOC load CMD */
	oss_memset(toc_cmd, 0, sizeof(struct psp_cmd_km));

	AMDGV_INFO("TOC: GPU_ADDR=0x%llx MEM_ADDR=0x%llx MEM_SIZE=0x%llx\n",
		   amdgv_memmgr_get_gpu_addr(load_mem.mem),
		   amdgv_memmgr_get_offset(load_mem.mem), amdgv_memmgr_get_size(load_mem.mem));

	return ret;
}

static enum psp_status amdgv_psp_ras_load(struct amdgv_adapter *adapt,
					  unsigned char *ras_image, uint32_t ras_size)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);
	struct psp_local_memory *ras_bin_mem = &(adapt->psp.private_fw_memory);
	struct psp_local_memory *local_shared_mem = &(ras_context->shared_buffer);
	struct psp_cmd_km *load_km_cmd = adapt->psp.psp_cmd_km_mem;
	struct psp_gfx_resp resp_buf = { 0 };
	struct ta_ras_shared_memory *ras_cmd;
	uint32_t fw_ver = 0;
	struct psp_fw_image_header *fw_hdr = NULL;

	if (!ras_bin_mem->mem || !local_shared_mem->mem)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;

	if (!ras_image || !ras_size)
		return PSP_STATUS__ERROR_GENERIC;

	/* Copy RAS TA binary to PSP private fw memory */
	oss_memset(amdgv_memmgr_get_cpu_addr(ras_bin_mem->mem), 0, PSP_PRIVATE_IMAGE_SIZE);
	oss_memcpy(amdgv_memmgr_get_cpu_addr(ras_bin_mem->mem), ras_image, ras_size);

	/* Cache RAS TA binary to local RAS TA BIN memory for new RAS TA*/
	if (!adapt->reset.reset_state) {
		oss_memset(ras_context->ras_bin_buf, 0, AMDGV_FW_SIZE_MAX);
		oss_memcpy(ras_context->ras_bin_buf, ras_image, ras_size);
		ras_context->ras_bin_size = ras_size;
	}

	if (ras_context->set_init_flag == true) {
		ras_context->ras_shared_buffer =
			amdgv_memmgr_get_cpu_addr(ras_context->shared_buffer.mem);
		ras_cmd = (struct ta_ras_shared_memory *)ras_context->ras_shared_buffer;
		if (ras_cmd) {
			ras_cmd->ras_in_message.init_flags.dgpu_mode = 1;
			ras_cmd->ras_in_message.init_flags.xcc_mask = 0;
			if (amdgv_ras_is_poison_mode_supported(adapt))
				ras_cmd->ras_in_message.init_flags.poison_mode_en = 1;
			if (adapt->umc.funcs && adapt->umc.funcs->query_ras_memchandis)
				ras_cmd->ras_in_message.init_flags.channel_dis_num =
					adapt->umc.channel_dis_num;
			ras_cmd->ras_in_message.init_flags.xcc_mask = adapt->mcp.gfx.xcc_mask;

			if (adapt->nbio.ras->get_curr_memory_partition_mode) {
				enum amdgv_memory_partition_mode curr_memory_partition_mode;
				int res;

				res = adapt->nbio.ras->get_curr_memory_partition_mode(
						adapt, &curr_memory_partition_mode);
				if (!res)
					ras_cmd->ras_in_message.init_flags.nps_mode = curr_memory_partition_mode;
			}

			ras_cmd->ras_in_message.init_flags.active_umc_mask = adapt->umc.active_mask;
		}
	}

	if (!load_km_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Prepare CMD to load RAS TA */
	load_km_cmd->cmd_id = PSP_CMD_KM_TYPE__LOAD_TA;
	load_km_cmd->cmd.load_ta.app_buf_addr_lo =
		lower_32_bits(amdgv_memmgr_get_gpu_addr(ras_bin_mem->mem));
	load_km_cmd->cmd.load_ta.app_buf_addr_hi =
		upper_32_bits(amdgv_memmgr_get_gpu_addr(ras_bin_mem->mem));
	load_km_cmd->cmd.load_ta.app_size = ras_size;

	load_km_cmd->cmd.load_ta.shared_mem_addr_lo =
		lower_32_bits(amdgv_memmgr_get_gpu_addr(local_shared_mem->mem));
	load_km_cmd->cmd.load_ta.shared_mem_addr_hi =
		upper_32_bits(amdgv_memmgr_get_gpu_addr(local_shared_mem->mem));
	load_km_cmd->cmd.load_ta.shared_mem_size = PSP_RAS_SHARED_MEM_SIZE;

	/* Submit CMD buffer to load RAS TA */
	/* Synchronous call; will wait for response */
	/* Update PSP FW session ID associated with RAS TA */
	ret = amdgv_psp_cmd_km_submit(adapt, load_km_cmd, &resp_buf);
	if (ret != PSP_STATUS__SUCCESS)
		ret = PSP_STATUS__ERROR_GENERIC;
	else {
		ras_context->ras_session_id = resp_buf.session_id;
		fw_ver = amdgv_psp_ta_version(adapt, (uint8_t *)ras_image, PSP_TA_PROP_VER_NAME);

		if (fw_ver) {
			adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RAS_TA] = fw_ver;
			AMDGV_INFO("PSP: RAS TA(version:%X.%X.%X.%X) is loaded.\n",
				(fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
				(fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
			ras_context->ta_version = fw_ver;
		} else {
			/* Read TA version at FW offset 0x60 if TA version not found*/
			fw_hdr = (struct psp_fw_image_header *)ras_image;
			adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RAS_TA] = fw_hdr->image_version;
			AMDGV_INFO("PSP: RAS TA(version:%X.%X.%X.%X) is loaded.\n",
				(fw_hdr->image_version >> 24) & 0xFF, (fw_hdr->image_version >> 16) & 0xFF,
				(fw_hdr->image_version >> 8) & 0xFF, fw_hdr->image_version & 0xFF);
			ras_context->ta_version = fw_hdr->image_version;
		}
	}
	/* Clear system memory used for RAS TA Load CMD */
	oss_memset(load_km_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

static enum psp_status amdgv_psp_ras_unload(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);
	struct psp_cmd_km *unload_km_cmd = adapt->psp.psp_cmd_km_mem;

	if (!unload_km_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Prepare CMD to unload RAS driver */
	unload_km_cmd->cmd_id = PSP_CMD_KM_TYPE__UNLOAD_TA;
	unload_km_cmd->cmd.unload_ta.session_id = ras_context->ras_session_id;

	/* Submit CMD buffer to unload RAS driver */
	/* Synchronous call; will wait for response */
	ret = amdgv_psp_cmd_km_submit(adapt, unload_km_cmd, NULL);
	if (ret != PSP_STATUS__SUCCESS)
		ret = PSP_STATUS__ERROR_GENERIC;

	ras_context->ta_version = 0;
	/* Clear system memory used for RAS Unload CMD */
	oss_memset(unload_km_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

enum psp_status amdgv_psp_ras_initialize(struct amdgv_adapter *adapt, unsigned char *ras_image,
					 uint32_t ras_size)
{
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);

	if (ras_context->ras_initialized)
		return PSP_STATUS__SUCCESS;

	if (adapt->umc.funcs && adapt->umc.funcs->query_ras_memchandis)
		adapt->umc.funcs->query_ras_memchandis(adapt);

	/* the poison flag should be initialized before TA loading */
	amdgv_ras_poison_mode_init(adapt);

	/* Load RAS TA */
	if (amdgv_psp_ras_load(adapt, ras_image, ras_size) != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_LOAD_FAIL, 0);
	} else
		ras_context->ras_initialized = true;

	/**
	 * Considering RAS TA loading failure won't affect
	 * normal device initial process, so we just print
	 * error log and always return success here.
	 */
	return PSP_STATUS__SUCCESS;
}

enum psp_status amdgv_psp_ras_terminate(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);

	if (!ras_context->ras_initialized)
		return PSP_STATUS__SUCCESS;
	else {
		/* Unload RAS TA */
		ret = amdgv_psp_ras_unload(adapt);
		if (ret != PSP_STATUS__SUCCESS) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_UNLOAD_FAIL, 0);
			ret = PSP_STATUS__ERROR_GENERIC;
		} else
			ras_context->ras_initialized = false;
	}

	return ret;
}

static enum psp_status amdgv_psp_ras_invoke(struct amdgv_adapter *adapt,
					    uint32_t ras_ta_cmd_id)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);
	struct psp_cmd_km *invoke_km_cmd = adapt->psp.psp_cmd_km_mem;
	struct ta_ras_shared_memory *ras_cmd = (struct ta_ras_shared_memory *)
		(amdgv_memmgr_get_cpu_addr(ras_context->shared_buffer.mem));

	if (!ras_context->ras_initialized)
		/* do nothing if ras is not initialized */
		return PSP_STATUS__SUCCESS;

	if (!invoke_km_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Prepare CMD to invoke RAS TA */
	invoke_km_cmd->cmd_id = PSP_CMD_KM_TYPE__INVOKE_CMD;
	invoke_km_cmd->cmd.invoke_ta.ta_cmd_id = ras_ta_cmd_id;
	invoke_km_cmd->cmd.invoke_ta.session_id = ras_context->ras_session_id;
	/* invoke_ta.buf_list is not used for RAS TA */

	/* Submit CMD buffer to invoke RAS TA */
	/* Synchronous call; will wait for response */
	ret = amdgv_psp_cmd_km_submit(adapt, invoke_km_cmd, NULL);
	if (ret != PSP_STATUS__SUCCESS) {
		if (!amdgv_wait_for(adapt, amdgv_psp_wait_for_ras_intr_cb, NULL, AMDGV_TIMEOUT(TIMEOUT_CMD_RESP), 0))
			ret = PSP_STATUS__SUCCESS;
		else {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_TA_INVOKE_FAIL, 0);
			ret = PSP_STATUS__ERROR_GENERIC;
		}
	}

	AMDGV_INFO("amdgv_psp_ras_invoke returned ras_status: 0x%x\n", ras_cmd->ras_status);

	/* Clear system memory used for Invoke RAS TA CMD */
	oss_memset(invoke_km_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}


static bool amdgv_psp_is_error_injection_valid(struct amdgv_adapter *adapt,
					       struct ta_ras_trigger_error_input *info)
{
	bool ret = false;

	/* ONLY UMC explicitly addressed error injection is supported */
	switch (info->block_id) {
	case TA_RAS_BLOCK__UMC:
		if ((info->address < (adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size)) ||
			(info->address >= (adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size +
				MBYTES_TO_BYTES(adapt->gpuiov.total_fb_avail)))) {
			AMDGV_WARN("Invalid inject address 0x%llx, umc address range:0x%llx--0x%llx\n",
					info->address,
					adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size,
					adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size +
					MBYTES_TO_BYTES(adapt->gpuiov.total_fb_avail));
			return ret;
		}

		if ((info->inject_error_type != TA_RAS_ERROR__SINGLE_CORRECTABLE &&
		     info->inject_error_type != TA_RAS_ERROR__MULTI_UNCORRECTABLE &&
			 info->inject_error_type != TA_RAS_ERROR__POISON) ||
		    amdgv_umc_check_bad_page(adapt, info->address) == true)
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_INVALID_VALUE, -1);
		else
			ret = true;
		break;
	case TA_RAS_BLOCK__GFX:
	case TA_RAS_BLOCK__SDMA:
	case TA_RAS_BLOCK__MMHUB:
	case TA_RAS_BLOCK__XGMI_WAFL:
	case TA_RAS_BLOCK__PCIE_BIF:
		ret = true;
		break;
	default:
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_INVALID_VALUE,
				info->block_id);
		break;
	}

	return ret;
}

enum psp_status amdgv_psp_ras_trigger_error(struct amdgv_adapter *adapt,
					    struct ta_ras_trigger_error_input *info)
{
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);
	struct ta_ras_shared_memory *ras_cmd;
	enum psp_status ret = PSP_STATUS__SUCCESS;

	if (!ras_context->ras_initialized || !ras_context->ta_version) {
		AMDGV_ERROR("PSP: RAS TA is not loaded or not initialized!\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	if (!amdgv_psp_is_error_injection_valid(adapt, info))
		return PSP_STATUS__ERROR_GENERIC;

	ras_cmd = (struct ta_ras_shared_memory *)(amdgv_memmgr_get_cpu_addr(
		ras_context->shared_buffer.mem));
	if (!ras_cmd)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	oss_memset(ras_cmd, 0, sizeof(struct ta_ras_shared_memory));

	ras_cmd->cmd_id = TA_RAS_COMMAND__TRIGGER_ERROR;
	ras_cmd->ras_in_message.trigger_error = *info;

	AMDGV_INFO("block_id:%d, inject_error_type:%d, sub_block_index:0x%x, address:0x%llx, value:0x%llx\n",
		info->block_id, info->inject_error_type, info->sub_block_index, info->address, info->value);

	ret = amdgv_psp_ras_invoke(adapt, ras_cmd->cmd_id);
	if (ret != PSP_STATUS__SUCCESS)
		return ret;

	if ((info->inject_error_type == TA_RAS_ERROR__POISON ||
	     info->inject_error_type == TA_RAS_ERROR__MULTI_UNCORRECTABLE) &&
	    amdgv_ras_intr_triggered())
		return ret;
	else if (ras_cmd->ras_status != TA_RAS_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_TA_ERR_INJECT_FAIL, 0);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

enum psp_status amdgv_psp_ras_set_feature(struct amdgv_adapter *adapt,
					union ta_ras_cmd_input *info, bool is_enable)
{
	struct psp_ras_context *ras_context = &(adapt->psp.ras_context);
	struct ta_ras_shared_memory *ras_cmd;
	enum psp_status ret = PSP_STATUS__SUCCESS;

	if (!ras_context->ras_initialized)
		return PSP_STATUS__ERROR_GENERIC;

	ras_cmd = (struct ta_ras_shared_memory *)(amdgv_memmgr_get_cpu_addr(
		ras_context->shared_buffer.mem));
	if (!ras_cmd)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	oss_memset(ras_cmd, 0, sizeof(struct ta_ras_shared_memory));

	if (is_enable)
		ras_cmd->cmd_id = TA_RAS_COMMAND__ENABLE_FEATURES;
	else
		ras_cmd->cmd_id = TA_RAS_COMMAND__DISABLE_FEATURES;

	ras_cmd->ras_in_message = *info;

	ret = amdgv_psp_ras_invoke(adapt, ras_cmd->cmd_id);
	if (ret != PSP_STATUS__SUCCESS)
		return ret;

	if (ras_cmd->ras_status != TA_RAS_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RAS_TA_ENABLE_RAS_FEATURE_FAIL,
				0);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	return ret;
}

enum psp_status amdgv_psp_ras_get_ta_version(struct amdgv_adapter *adapt, void *fw_image, uint32_t *ver_ptr)
{
	uint32_t fw_ver = amdgv_psp_ta_version(adapt, (uint8_t *)fw_image, PSP_TA_PROP_VER_NAME);
	struct psp_fw_image_header *fw_hdr = NULL;

	if (fw_ver) {
		AMDGV_INFO("PSP: RAS TA(version:%X.%X.%X.%X) queried.\n",
				   (fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
				   (fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
	} else {
		/* Read TA version at FW offset 0x60 if TA version not found*/
		fw_hdr = (struct psp_fw_image_header *)fw_image;
		fw_ver = fw_hdr->image_version;
		AMDGV_INFO("PSP: RAS TA(version:%X.%X.%X.%X) queried.\n",
				   (fw_hdr->image_version >> 24) & 0xFF, (fw_hdr->image_version >> 16) & 0xFF,
				   (fw_hdr->image_version >> 8) & 0xFF, fw_hdr->image_version & 0xFF);
	}

	*ver_ptr = fw_ver;
	return PSP_STATUS__SUCCESS;
}

bool amdgv_psp_vfgate_support(struct amdgv_adapter *adapt)
{
	enum psp_status stat = PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;

	if (adapt->psp.vfgate_support)
		stat = adapt->psp.vfgate_support(adapt);

	if (stat == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE)
		return false;

	return true;
}

enum psp_status amdgv_psp_set_mb_int(struct amdgv_adapter *adapt, uint32_t idx_vf, bool enable)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;

	if (adapt->psp.set_mb_int)
		ret = adapt->psp.set_mb_int(adapt, idx_vf, enable);

	return ret;
}

enum psp_status amdgv_psp_get_mb_int_status(struct amdgv_adapter *adapt, uint32_t idx_vf,
					    struct psp_mb_status *mb_status)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;

	if (adapt->psp.get_mb_int_status)
		ret = adapt->psp.get_mb_int_status(adapt, idx_vf, mb_status);

	return ret;
}

uint32_t amdgv_psp_ta_version(struct amdgv_adapter *adapt, void *fw_image, char *name)
{
	struct app_prop_buff *buf;
	struct app_property *prop;
	uint32_t i;
	uint32_t name_size = oss_strlen(name) + 1; // including null terminator

	if (adapt->asic_type == CHIP_MI300X ||
	    adapt->asic_type == CHIP_MI308X)
		return 0;

	buf = (struct app_prop_buff *)&(((uint8_t *)fw_image)[PSP_TA_PROP_OFFSET]);

	prop = buf->Property;

	for (i = 0; i < buf->PropCount; i++) {
		if (name_size == prop->NameSize &&
		    oss_strncmp(prop->Name, name, name_size) == 0)
			return *(uint32_t *)(prop->Name + prop->NameSize);

		prop = (struct app_property *)((uint8_t *)prop + prop->Size);
	}

	return 0;
}

enum psp_status amdgv_psp_xgmi_load(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);
	struct psp_local_memory *xgmi_bin_mem = &(adapt->psp.private_fw_memory);
	struct psp_local_memory *local_shared_mem = &(xgmi_context->shared_buffer);
	struct psp_cmd_km *load_km_cmd = adapt->psp.psp_cmd_km_mem;
	struct psp_gfx_resp resp_buf = { 0 };
	uint32_t fw_ver = 0;
	unsigned char *fw_image;
	uint32_t fw_image_size;
	struct psp_fw_image_header *fw_hdr = NULL;

	if (!xgmi_bin_mem->mem || !local_shared_mem->mem)
		return PSP_STATUS__ERROR_GENERIC;

	switch (adapt->psp.tee_version) {
	case GFX_TEE_VERSION_3:
		{
			fw_image = psp_xgmi_tee3_esbin;
			fw_image_size = sizeof(psp_xgmi_tee3_esbin);
		}

		break;
	default:
		fw_image = psp_xgmi_bin;
		fw_image_size = sizeof(psp_xgmi_bin);
		break;
	}

	/* Copy xgmi TA binary to PSP private fw memory */
	oss_memset(amdgv_memmgr_get_cpu_addr(xgmi_bin_mem->mem), 0, PSP_PRIVATE_IMAGE_SIZE);
	oss_memcpy(amdgv_memmgr_get_cpu_addr(xgmi_bin_mem->mem), fw_image, fw_image_size);

	if (!load_km_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Prepare CMD to load TA fw */
	load_km_cmd->cmd_id = PSP_CMD_KM_TYPE__LOAD_TA;
	load_km_cmd->cmd.load_ta.app_buf_addr_lo =
		lower_32_bits(amdgv_memmgr_get_gpu_addr(xgmi_bin_mem->mem));
	load_km_cmd->cmd.load_ta.app_buf_addr_hi =
		upper_32_bits(amdgv_memmgr_get_gpu_addr(xgmi_bin_mem->mem));
	load_km_cmd->cmd.load_ta.app_size = fw_image_size;
	load_km_cmd->cmd.load_ta.shared_mem_addr_lo =
		lower_32_bits(amdgv_memmgr_get_gpu_addr(local_shared_mem->mem));
	load_km_cmd->cmd.load_ta.shared_mem_addr_hi =
		upper_32_bits(amdgv_memmgr_get_gpu_addr(local_shared_mem->mem));
	load_km_cmd->cmd.load_ta.shared_mem_size = PSP_XGMI_SHARED_MEM_SIZE;

	/* Submit CMD buffer to load ta */
	ret = amdgv_psp_cmd_km_submit(adapt, load_km_cmd, &resp_buf);
	if (ret != PSP_STATUS__SUCCESS)
		ret = PSP_STATUS__ERROR_GENERIC;
	else {
		xgmi_context->xgmi_session_id = resp_buf.session_id;

		fw_ver = amdgv_psp_ta_version(adapt, (uint8_t *)fw_image, PSP_TA_PROP_VER_NAME);
		if (fw_ver) {
			AMDGV_INFO("PSP: XGMI TA(version:%X.%X.%X.%X) is loaded.\n",
				(fw_ver >> 24) & 0xFF, (fw_ver >> 16) & 0xFF,
				(fw_ver >> 8) & 0xFF, fw_ver & 0xFF);
		} else {
			/* Read TA version at FW offset 0x60 if TA version not found*/
			fw_hdr = (struct psp_fw_image_header *)fw_image;
			if (fw_hdr) {
				AMDGV_INFO("PSP: XGMI TA(version:%X.%X.%X.%X) is loaded.\n",
					(fw_hdr->image_version >> 24) & 0xFF, (fw_hdr->image_version >> 16) & 0xFF,
					(fw_hdr->image_version >> 8) & 0xFF, fw_hdr->image_version & 0xFF);
			} else {
				AMDGV_WARN("PSP: XGMI TA version not found.\n");
			}
		}
	}

	/* Clear system memory used for XGMI Load CMD */
	oss_memset(load_km_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

enum psp_status amdgv_psp_xgmi_invoke(struct amdgv_adapter *adapt, uint32_t ta_cmd_id,
				      uint32_t session_id)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);
	struct psp_cmd_km *invoke_km_cmd = adapt->psp.psp_cmd_km_mem;

	if (!xgmi_context->xgmi_initialized)
		/* do nothing if xgmi is not initialized */
		return PSP_STATUS__SUCCESS;

	if (!invoke_km_cmd) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));

		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Prepare CMD to invoke XGMI TA */
	invoke_km_cmd->cmd_id = PSP_CMD_KM_TYPE__INVOKE_CMD;
	invoke_km_cmd->cmd.invoke_ta.ta_cmd_id = ta_cmd_id;
	invoke_km_cmd->cmd.invoke_ta.session_id = xgmi_context->xgmi_session_id;

	/* Submit CMD buffer to invoke XGMI TA */
	/* Synchronous call; will wait for response */
	ret = amdgv_psp_cmd_km_submit(adapt, invoke_km_cmd, NULL);
	if (ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_XGMI_TA_INVOKE_FAIL, 0);
		ret = PSP_STATUS__ERROR_GENERIC;
	}

	/* Clear system memory used for Invoke XGMI TA CMD */
	oss_memset(invoke_km_cmd, 0, sizeof(struct psp_cmd_km));

	return ret;
}

enum psp_status amdgv_psp_xgmi_initialize(struct amdgv_adapter *adapt)
{
	struct ta_xgmi_shared_memory *xgmi_cmd;
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);
	enum psp_status ret = PSP_STATUS__SUCCESS;

	if (adapt->xgmi.phy_nodes_num < 2)
		return ret;

	ret = amdgv_psp_xgmi_load(adapt);
	if (ret != PSP_STATUS__SUCCESS) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_XGMI_LOAD_FAIL, 0);
		return ret;
	} else
		xgmi_context->xgmi_initialized = true;

	xgmi_cmd = (struct ta_xgmi_shared_memory *)(amdgv_memmgr_get_cpu_addr(
		xgmi_context->shared_buffer.mem));
	if (!xgmi_cmd)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	oss_memset(xgmi_cmd, 0, sizeof(struct ta_xgmi_shared_memory));
	xgmi_cmd->cmd_id = TA_COMMAND_XGMI__INITIALIZE;

	ret = amdgv_psp_xgmi_invoke(adapt, xgmi_cmd->cmd_id, xgmi_context->xgmi_session_id);
	if (ret)
		AMDGV_ERROR("xgmi invoke failed\n");

	return ret;
}

enum psp_status amdgv_psp_xgmi_get_node_id(struct amdgv_adapter *adapt)
{
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);
	struct ta_xgmi_shared_memory *xgmi_cmd;
	enum psp_status ret = PSP_STATUS__SUCCESS;

	xgmi_cmd = (struct ta_xgmi_shared_memory *)(amdgv_memmgr_get_cpu_addr(
		xgmi_context->shared_buffer.mem));
	if (!xgmi_cmd)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	oss_memset(xgmi_cmd, 0, sizeof(struct ta_xgmi_shared_memory));
	xgmi_cmd->cmd_id = TA_COMMAND_XGMI__GET_NODE_ID;

	ret = amdgv_psp_xgmi_invoke(adapt, xgmi_cmd->cmd_id, xgmi_context->xgmi_session_id);
	if (ret)
		AMDGV_ERROR("xgmi get node id failed\n");

	adapt->xgmi.node_id = xgmi_cmd->xgmi_out_message.get_node_id.node_id;

	AMDGV_INFO("node id = 0x%llx\n", xgmi_cmd->xgmi_out_message.get_node_id.node_id);
	return ret;
}

enum psp_status amdgv_psp_xgmi_get_hive_id(struct amdgv_adapter *adapt)
{
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);
	struct ta_xgmi_shared_memory *xgmi_cmd;
	enum psp_status ret = PSP_STATUS__SUCCESS;

	xgmi_cmd = (struct ta_xgmi_shared_memory *)(amdgv_memmgr_get_cpu_addr(
		xgmi_context->shared_buffer.mem));
	if (!xgmi_cmd)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	oss_memset(xgmi_cmd, 0, sizeof(struct ta_xgmi_shared_memory));
	xgmi_cmd->cmd_id = TA_COMMAND_XGMI__GET_HIVE_ID;

	ret = amdgv_psp_xgmi_invoke(adapt, xgmi_cmd->cmd_id, xgmi_context->xgmi_session_id);
	if (ret)
		AMDGV_ERROR("xgmi hive id failed\n");

	adapt->xgmi.hive_id = xgmi_cmd->xgmi_out_message.get_hive_id.hive_id;

	AMDGV_INFO("hive id = 0x%llx\n", xgmi_cmd->xgmi_out_message.get_hive_id.hive_id);
	return ret;
}

enum psp_status amdgv_psp_xgmi_set_topology_info(struct amdgv_adapter *adapt,
						 struct amdgv_hive_info *hive)
{
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);
	struct ta_xgmi_shared_memory *xgmi_cmd;
	struct ta_xgmi_cmd_set_topology_info_input *topology_info_input;
	struct amdgv_adapter *cur;
	enum psp_status ret = PSP_STATUS__SUCCESS;
	int i = 0;

	xgmi_cmd = (struct ta_xgmi_shared_memory *)(amdgv_memmgr_get_cpu_addr(
		xgmi_context->shared_buffer.mem));
	if (!xgmi_cmd)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	oss_memset(xgmi_cmd, 0, sizeof(struct ta_xgmi_shared_memory));

	topology_info_input = &xgmi_cmd->xgmi_in_message.set_topology_info;
	xgmi_cmd->cmd_id = TA_COMMAND_XGMI__SET_TOPOLOGY_INFO;
	topology_info_input->num_nodes = hive->number_adapters;

	amdgv_list_for_each_entry(cur, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
		topology_info_input->nodes[i].node_id = cur->xgmi.node_id;
		topology_info_input->nodes[i].num_hops = 0;
		topology_info_input->nodes[i].is_sharing_enabled =
			amdgv_xgmi_is_fb_sharing_allowed(adapt, adapt->xgmi.phy_node_id,
				cur->xgmi.phy_node_id, adapt->xgmi.fb_sharing_mode);
		topology_info_input->nodes[i].sdma_engine = 0;

		i++;
	}

	/* Invoke xgmi ta to set topology information */
	ret = amdgv_psp_xgmi_invoke(adapt, xgmi_cmd->cmd_id, xgmi_context->xgmi_session_id);
	if (ret)
		AMDGV_ERROR("xgmi set_topology_info failed\n");

	AMDGV_DEBUG("ret = 0x%x\n", ret);
	return ret;
}

enum psp_status amdgv_psp_xgmi_get_topology_info(struct amdgv_adapter *adapt,
	struct amdgv_hive_info *hive, struct amdgv_xgmi_psp_topology_info *topology_info)
{
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);
	struct ta_xgmi_shared_memory *xgmi_cmd;
	struct ta_xgmi_cmd_get_topology_info_input *topology_info_input;
	struct ta_xgmi_cmd_get_topology_info_output *topology_info_output;
	struct amdgv_adapter *cur;
	enum psp_status ret = PSP_STATUS__SUCCESS;
	int i = 0;

	xgmi_cmd = (struct ta_xgmi_shared_memory *)(amdgv_memmgr_get_cpu_addr(
		xgmi_context->shared_buffer.mem));
	if (!xgmi_cmd)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	oss_memset(xgmi_cmd, 0, sizeof(struct ta_xgmi_shared_memory));

	xgmi_cmd->cmd_id = TA_COMMAND_XGMI__GET_TOPOLOGY_INFO;
	topology_info_input = &xgmi_cmd->xgmi_in_message.get_topology_info;
	topology_info_output = &xgmi_cmd->xgmi_out_message.get_topology_info;

	amdgv_list_for_each_entry(cur, &hive->adapt_list,
				  struct amdgv_adapter, xgmi.head) {
		if (cur->xgmi.node_id == adapt->xgmi.node_id)
			continue;

		topology_info_input->nodes[i].node_id = cur->xgmi.node_id;
		i++;
	}

	topology_info_input->num_nodes = i;

	/* Invoke xgmi ta to get topology information */
	ret = amdgv_psp_xgmi_invoke(adapt, xgmi_cmd->cmd_id, xgmi_context->xgmi_session_id);
	if (ret)
		AMDGV_ERROR("xgmi get_topology_info failed\n");

	if (topology_info_output->num_nodes > AMDGV_XGMI_MAX_CONNECTED_NODES) {
		topology_info->num_nodes = 0;
		AMDGV_ERROR("Failed to copy get topology\n");
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	}

	topology_info->num_nodes = topology_info_output->num_nodes;

	for (i = 0; i < topology_info_output->num_nodes; i++) {
		topology_info->node[i].num_hops = topology_info_output->nodes[i].num_hops & 0xf;
		topology_info->node[i].is_sharing_enabled = topology_info_output->nodes[i].is_sharing_enabled;
		topology_info->node[i].node_id = topology_info_output->nodes[i].node_id;
	}

	AMDGV_DEBUG("ret = 0x%x\n", ret);
	return ret;
}

enum psp_status amdgv_psp_xgmi_get_peer_link_info(struct amdgv_adapter *adapt,
	struct amdgv_hive_info *hive, struct amdgv_xgmi_psp_link_info *link_info)
{
	struct psp_xgmi_context *xgmi_context = &(adapt->psp.xgmi_context);
	struct ta_xgmi_shared_memory *xgmi_cmd;
	struct ta_xgmi_cmd_get_extend_peer_link_info *peer_link_info;
	struct amdgv_adapter *cur;
	enum psp_status ret = PSP_STATUS__SUCCESS;
	int i = 0, j = 0, num_links = 0;

	xgmi_cmd = (struct ta_xgmi_shared_memory *)(amdgv_memmgr_get_cpu_addr(
		xgmi_context->shared_buffer.mem));
	if (!xgmi_cmd)
		return PSP_STATUS__ERROR_OUT_OF_MEMORY;
	oss_memset(xgmi_cmd, 0, sizeof(struct ta_xgmi_shared_memory));

	xgmi_cmd->cmd_id = TA_COMMAND_XGMI__GET_EXTEND_PEER_LINKS;
	peer_link_info = &xgmi_cmd->xgmi_out_message.get_extend_link_info;

	amdgv_list_for_each_entry(cur, &hive->adapt_list,
				  struct amdgv_adapter, xgmi.head) {
		if (peer_link_info->num_nodes < TA_XGMI__MAX_CONNECTED_NODES) {
			peer_link_info->nodes[i].node_id = cur->xgmi.node_id;
			i++;
		} else {
			return PSP_STATUS__ERROR_GENERIC;
		}
	}

	peer_link_info->num_nodes = i;

	ret = amdgv_psp_xgmi_invoke(adapt, xgmi_cmd->cmd_id, xgmi_context->xgmi_session_id);
	if (ret)
		AMDGV_ERROR("xgmi get_topology_info failed\n");

	for (i = 0; i < peer_link_info->num_nodes; i++) {
		for (j = 0; j < peer_link_info->nodes[i].num_links; j++) {
			if (num_links >= AMDGV_XGMI_MAX_NUM_LINKS || j >= TA_XGMI__MAX_PORT_NUM) {
				link_info->num_links = 0;
				AMDGV_ERROR("Failed to copy get topology\n");
				return PSP_STATUS__ERROR_OUT_OF_MEMORY;
			}
			link_info->link[num_links].dest_node_id = peer_link_info->nodes[i].node_id;
			link_info->link[num_links].src_port_id = peer_link_info->nodes[i].port_num[j].src_xgmi_port_num;
			link_info->link[num_links].dest_port_id = peer_link_info->nodes[i].port_num[j].dst_xgmi_port_num;
			num_links++;
		}
	}

	link_info->num_links = num_links;

	AMDGV_DEBUG("ret = 0x%x\n", ret);
	return ret;
}

enum psp_status amdgv_psp_load_fw_work(struct amdgv_adapter *adapt, unsigned char *fw_image,
				       uint32_t fw_image_size, enum amdgv_firmware_id ucode_id)
{
	enum psp_status ret = PSP_STATUS__ERROR_GENERIC;

	switch (ucode_id) {
	case AMDGV_FIRMWARE_ID__PSP_KEYDB:
		if (adapt->psp.load_keydb)
			ret = adapt->psp.load_keydb(adapt, fw_image, fw_image_size);
		break;
	case AMDGV_FIRMWARE_ID__PSP_SPL:
		if (adapt->psp.load_spl)
			ret = adapt->psp.load_spl(adapt, fw_image, fw_image_size);
		break;
	case AMDGV_FIRMWARE_ID__PSP_SYS:
		if (adapt->psp.load_sysdrv)
			ret = adapt->psp.load_sysdrv(adapt, fw_image, fw_image_size);
		break;
	case AMDGV_FIRMWARE_ID__PSP_RAS:
		if (adapt->psp.load_rasdrv)
			ret = adapt->psp.load_rasdrv(adapt, fw_image, fw_image_size);
		break;
	case AMDGV_FIRMWARE_ID__PSP_SOS:
		if (adapt->psp.load_sosdrv)
			ret = adapt->psp.load_sosdrv(adapt, fw_image, fw_image_size);
		break;
	case AMDGV_FIRMWARE_ID__PSP_TOC:
		ret = amdgv_psp_load_toc(adapt, fw_image, fw_image_size);
		break;
	case AMDGV_FIRMWARE_ID__PSP_SOC:
	case AMDGV_FIRMWARE_ID__PSP_DBG:
	case AMDGV_FIRMWARE_ID__PSP_INTF:
		if (adapt->psp.load_psp_ucode)
			ret = adapt->psp.load_psp_ucode(adapt, fw_image, fw_image_size,
							ucode_id);
		break;
	default:
		ret = amdgv_psp_load_np_fw(adapt, fw_image, fw_image_size, ucode_id);
		break;
	}

	return ret;
}

enum psp_status amdgv_psp_load_local_fw(struct amdgv_adapter *adapt,
					enum amdgv_firmware_id ucode_id,
					unsigned char *embedded_fw_image)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	unsigned char *ppatched_fw_image;
	uint32_t fw_size;
	struct psp_fw_image_header *ppatched_fw_header;
	struct psp_fw_image_header *pembedded_fw_header;

	ppatched_fw_image = oss_malloc(AMDGV_FW_SIZE_MAX);
	if (!ppatched_fw_image) {
		AMDGV_WARN("Failed to allocate FW image space\n");
		return PSP_STATUS__ERROR_GENERIC;
	}

	if (oss_get_fw(adapt->dev, ucode_id, adapt->asic_type, ppatched_fw_image, &fw_size,
		       AMDGV_FW_SIZE_MAX)) {
		ret = PSP_STATUS__ERROR_GENERIC;
		goto out;
	}

	ppatched_fw_header = (struct psp_fw_image_header *)ppatched_fw_image;
	pembedded_fw_header = (struct psp_fw_image_header *)embedded_fw_image;

	/* Abort FW update if a new version has been loaded */
	if (ppatched_fw_header->image_version <= adapt->psp.fw_info[ucode_id] &&
	    !adapt->reset.reset_state) {
		AMDGV_WARN("Abort FW update since it's not a newer version\n");
		goto out;
	}

	/* Abort FW update if embedded FW is newer. This can happen during driver update with outdated patch FW */
	if (ppatched_fw_header->image_version <= pembedded_fw_header->image_version) {
		AMDGV_WARN("Abort FW update since embedded FW is newer\n");
		// In this case we should return an error so that the embedded FW will be loaded.
		ret = PSP_STATUS__ERROR_GENERIC;
		goto out;
	}

	ret = amdgv_psp_load_fw_work(adapt, ppatched_fw_image, fw_size, ucode_id);

out:
	oss_free(ppatched_fw_image);
	return ret;
}

enum psp_status amdgv_psp_load_fw(struct amdgv_adapter *adapt, unsigned char *fw_image,
				  uint32_t fw_image_size, enum amdgv_firmware_id ucode_id)
{
	if (!oss_detect_fw(adapt->dev, ucode_id, adapt->asic_type)) {
		if (!amdgv_psp_load_local_fw(adapt, ucode_id, fw_image))
			return PSP_STATUS__SUCCESS;
		else
			AMDGV_ERROR("failed to load local FW 0x%X\n", ucode_id);
	}
	return amdgv_psp_load_fw_work(adapt, fw_image, fw_image_size, ucode_id);
}

enum psp_status amdgv_psp_live_update_fw(struct amdgv_adapter *adapt,
					 enum amdgv_firmware_id fw_id)
{
	int ret = PSP_STATUS__SUCCESS;

	if (oss_detect_fw(adapt->dev, fw_id, adapt->asic_type)) {
		AMDGV_PRINT("FW 0x%X file for ASIC 0x%X not found\n", fw_id, adapt->asic_type);
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* Cannot queue suspend/resume event here,
	 * since FW live updating is handled in an event itself
	 */
	amdgv_sched_stop_all(adapt);

	if (!amdgv_sched_world_context_all_states_ok(adapt))
		amdgv_sched_reset_vf_auto(adapt);

	/* switch to PF for all blocks */
	if (amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_ALL) !=
	    0) {
		amdgv_sched_reset_vf_auto(adapt);
		return PSP_STATUS__ERROR_GENERIC;
	}

	/* stop disptimer2 if we're in live update SAVE*/
	if (adapt->live_update_state == AMDGV_LIVE_UPDATE_SAVE &&
	    adapt->irqmgr.ih_funcs->toggle_disp_timer2)
		adapt->irqmgr.ih_funcs->toggle_disp_timer2(adapt, false);

	if (adapt->ucode.load(adapt, &fw_id, 1))
		ret = PSP_STATUS__ERROR_GENERIC;

	adapt->lock_world_switch = false;

	return ret;
}

enum psp_status amdgv_psp_clear_vf_fw(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;

	if (adapt->psp.clear_vf_fw)
		ret = adapt->psp.clear_vf_fw(adapt, idx_vf);

	return ret;
}

bool amdgv_psp_fw_attestation_support(struct amdgv_adapter *adapt)
{
	enum psp_status stat = PSP_STATUS__ERROR_UNSUPPORTED_FEATURE;

	if (adapt->psp.fw_attestation_support)
		stat = adapt->psp.fw_attestation_support(adapt);

	if (stat == PSP_STATUS__ERROR_UNSUPPORTED_FEATURE)
		return false;

	return true;
}

enum psp_status amdgv_psp_get_fw_attestation_db_add(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_cmd_km fw_attestation_cmd = { 0 };
	struct psp_gfx_resp output_index = { 0 };
	uint32_t addr_lo = 0, addr_hi = 0;
	uint64_t mc_addr = 0;

	if (!amdgv_psp_fw_attestation_support(adapt))
		return ret;

	fw_attestation_cmd.cmd_id = PSP_CMD_KM_TYPE__FW_ATTESTATION;

	/* Submit CMD buffer to query fw attestation db address */
	ret = amdgv_psp_cmd_km_submit(adapt, &fw_attestation_cmd, &output_index);

	if (ret != PSP_STATUS__SUCCESS) {
		AMDGV_ERROR("Failed to get fw attestation address\n");
		ret = PSP_STATUS__ERROR_GENERIC;
	} else {
		addr_hi = output_index.uresp.fw_attestation_db_info.FwAttestationDbAddrHi;
		addr_lo = output_index.uresp.fw_attestation_db_info.FwAttestationDbAddrLo;

		mc_addr = ((uint64_t)addr_hi << 32) + addr_lo;
		(adapt->psp).attestation_db_gpu_addr = mc_addr;

		if ((void *)mc_addr == NULL) {
			AMDGV_WARN("Attestation mc_address returns NULL\n");
			return ret;
		}
	}

	return ret;
}

enum psp_status amdgv_psp_get_fw_attestation_info(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;

	if (!amdgv_psp_fw_attestation_support(adapt))
		return ret;

	if (adapt->psp.get_fw_attestation_info)
		ret = adapt->psp.get_fw_attestation_info(adapt, idx_vf);

	return ret;
}

void amdgv_psp_save_mb_error_record(struct amdgv_adapter *adapt, uint32_t idx_vf,
				    struct psp_mb_status *mb_status)
{
	struct amdgv_psp_mb_err_record *err_record =
		&(adapt->error_record[adapt->psp_mb_error_record_write_idx]);

	err_record->timestamp = oss_get_utc_time_stamp();
	err_record->vf_idx = idx_vf;
	err_record->fw_id = mb_status->fw_id;
	err_record->status = mb_status->status;
	err_record->valid = 1;

	adapt->psp_mb_error_record_write_idx =
		(adapt->psp_mb_error_record_write_idx + 1) % AMDGV_MAX_PSP_MB_ERROR_RECORD;
}

void amdgv_psp_record_loaded_fw(struct amdgv_adapter *adapt, unsigned char *fw_image,
				uint32_t fw_id)
{
	struct dfc_fw *dfc_image;
	uint32_t total_entries, expected_size;

	/* SMI only supports DFC Table for now */
	if (fw_id == AMDGV_FIRMWARE_ID__DFC_FW) {
		if (adapt->psp.dfc_fw != NULL) {
			dfc_image = (struct dfc_fw *)((unsigned char *)fw_image +
							sizeof(struct psp_fw_image_header));
			total_entries = dfc_image->header.dfc_fw_total_entries;
			expected_size = sizeof(struct dfc_fw_header) +
								total_entries * sizeof(struct dfc_fw_data);
			oss_memcpy(adapt->psp.dfc_fw, dfc_image, expected_size);
		}
	}
}

enum psp_status amdgv_psp_load_vbflash_bin(struct amdgv_adapter *adapt, char *buffer,
					   uint32_t pos, uint32_t count)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_vbflash_context *vbflash_context = &(adapt->psp.vbflash_context);

	vbflash_context->vbflash_done = false;
	/* Safeguard against memory drain */
	if (vbflash_context->vbflash_image_size > AMDGV_VBIOS_FILE_MAX_SIZE) {
		AMDGV_ERROR("File size cannot exceed %lu\n", AMDGV_VBIOS_FILE_MAX_SIZE);
		oss_free_memory(vbflash_context->vbflash_tmp_buf);
		vbflash_context->vbflash_tmp_buf = NULL;
		vbflash_context->vbflash_image_size = 0;
		return PSP_STATUS__ERROR_GENERIC;
	}

	if (!vbflash_context->vbflash_tmp_buf) {
		vbflash_context->vbflash_tmp_buf = oss_alloc_memory(AMDGV_VBIOS_FILE_MAX_SIZE);
		if (!vbflash_context->vbflash_tmp_buf)
			return PSP_STATUS__ERROR_GENERIC;
	}

	oss_mutex_lock(adapt->psp_lock);
	oss_memcpy(vbflash_context->vbflash_tmp_buf + pos, buffer, count);
	vbflash_context->vbflash_image_size += count;
	oss_mutex_unlock(adapt->psp_lock);
	return ret;
}

enum psp_status amdgv_psp_update_spirom(struct amdgv_adapter *adapt)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;
	struct psp_vbflash_context *vbflash_context = &(adapt->psp.vbflash_context);
	struct psp_local_memory *local_mem = &(vbflash_context->shared_buffer);

	oss_mutex_lock(adapt->psp_lock);

	if (!vbflash_context->vbflash_tmp_buf) {
		AMDGV_ERROR("Local VBIOS file is not loaded yet.\n");
		ret = PSP_STATUS__ERROR_INVALID_PARAMS;
		goto release_buffer;
	}

	local_mem->alignment = AMDGV_GPU_PAGE_SIZE;
	local_mem->size = vbflash_context->vbflash_image_size;
	local_mem->mem = amdgv_memmgr_alloc_align(PSP_MEMMGR, local_mem->size,
						  local_mem->alignment, MEM_PSP_VBFLASH);
	if (!local_mem->mem)
		goto release_buffer;

	oss_memcpy(amdgv_memmgr_get_cpu_addr(local_mem->mem), vbflash_context->vbflash_tmp_buf,
		   vbflash_context->vbflash_image_size);
	if (adapt->psp.update_spirom)
		ret = adapt->psp.update_spirom(adapt);
	else
		ret = PSP_STATUS__ERROR_INVALID_PARAMS;

	amdgv_memmgr_free(local_mem->mem);

release_buffer:
	if (vbflash_context->vbflash_tmp_buf)
		oss_free_memory(vbflash_context->vbflash_tmp_buf);
	vbflash_context->vbflash_tmp_buf = NULL;
	vbflash_context->vbflash_image_size = 0;

	oss_mutex_unlock(adapt->psp_lock);

	if (ret) {
		AMDGV_ERROR("Failed to load VBIOS FW, err = %d\n", ret);
		return ret;
	}

	AMDGV_INFO("VBIOS flash request is sent. Waiting for response from PSP.\n");
	return ret;
}

enum psp_status amdgv_psp_vbflash_status(struct amdgv_adapter *adapt, uint32_t *status)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;

	oss_mutex_unlock(adapt->psp_lock);

	if (adapt->psp.vbflash_status)
		ret = adapt->psp.vbflash_status(adapt, status);
	else
		ret = PSP_STATUS__ERROR_INVALID_PARAMS;

	if (!adapt->psp.vbflash_context.vbflash_done)
		*status = 0;
	else if (adapt->psp.vbflash_context.vbflash_done && !(*status & 0x80000000))
		*status = 1;
	else
		AMDGV_INFO("Vbflash done. PSP response is 0x%llx\n", *status);

	oss_mutex_unlock(adapt->psp_lock);

	return ret;
}

enum psp_status amdgv_psp_vf_cmd_relay(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;

	if (adapt->psp.vf_relay)
		ret = adapt->psp.vf_relay(adapt, idx_vf);

	return ret;
}

enum psp_status amdgv_psp_copy_vf_chiplet_regs(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	enum psp_status ret = PSP_STATUS__SUCCESS;

	if (idx_vf == AMDGV_PF_IDX)
		return ret;

	if (adapt->psp.copy_vf_chiplet_regs)
		ret = adapt->psp.copy_vf_chiplet_regs(adapt, idx_vf);

	return ret;
}

enum amdgv_live_info_status amdgv_psp_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_psp *psp_info)
{
	psp_info->xgmi_initialized = adapt->psp.xgmi_context.xgmi_initialized;
	psp_info->xgmi_session_id = adapt->psp.xgmi_context.xgmi_session_id;
	psp_info->asd_session_id = adapt->psp.asd_context.asd_session_id;
	psp_info->ras_initialized = adapt->psp.ras_context.ras_initialized;
	psp_info->ras_session_id = adapt->psp.ras_context.ras_session_id;
	psp_info->attestation_db_gpu_addr = adapt->psp.attestation_db_gpu_addr;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_psp_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_psp *psp_info)
{
	int i;

	adapt->psp.xgmi_context.xgmi_initialized = psp_info->xgmi_initialized;
	adapt->psp.xgmi_context.xgmi_session_id = psp_info->xgmi_session_id;
	adapt->psp.asd_context.asd_session_id = psp_info->asd_session_id;
	adapt->psp.ras_context.ras_initialized = psp_info->ras_initialized;
	adapt->psp.ras_context.ras_session_id = psp_info->ras_session_id;
	adapt->psp.attestation_db_gpu_addr = psp_info->attestation_db_gpu_addr;

	if (adapt->psp.parse_psp_info) {
		if (adapt->psp.parse_psp_info(adapt)) {
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		}
	} else {
		AMDGV_DEBUG(
			"parse_psp_info function not implemented, consider it is not supported\n");
	}

	if (adapt->psp.xgmi_context.xgmi_initialized) {
		if (amdgv_xgmi_parse_topology_info_for_live_info(adapt))
			return AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
	}

	for (i = 0; i < adapt->num_vf; i++)
		amdgv_psp_get_fw_attestation_info(adapt, i);

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_psp_fw_info_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_fw_info *fw_info)
{
	fw_info->fw_info_smu = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SMU];
	fw_info->fw_info_cp_ce = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_CE];
	fw_info->fw_info_cp_pfp = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_PFP];
	fw_info->fw_info_cp_me = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_ME];
	fw_info->fw_info_cp_mec_jt1 =
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_MEC_JT1];
	fw_info->fw_info_cp_mec_jt2 =
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_MEC_JT2];
	fw_info->fw_info_cp_mec1 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_MEC1];
	fw_info->fw_info_cp_mec2 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_MEC2];
	fw_info->fw_info_rlc = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC];
	fw_info->fw_info_sdma0 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA0];
	fw_info->fw_info_sdma1 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA1];
	fw_info->fw_info_sdma2 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA2];
	fw_info->fw_info_sdma3 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA3];
	fw_info->fw_info_sdma4 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA4];
	fw_info->fw_info_sdma5 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA5];
	fw_info->fw_info_sdma6 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA6];
	fw_info->fw_info_sdma7 = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA7];
	fw_info->fw_info_vcn = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__VCN];
	fw_info->fw_info_uvd = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__UVD];
	fw_info->fw_info_vce = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__VCE];
	fw_info->fw_info_isp = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__ISP];
	fw_info->fw_info_dmcu_eram = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__DMCU_ERAM];
	fw_info->fw_info_dmcu_isr = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__DMCU_ISR];
	fw_info->fw_info_rlc_restore_list_gpm_mem =
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM];
	fw_info->fw_info_rlc_restore_list_srm_mem =
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM];
	fw_info->fw_info_rlc_restore_list_cntl =
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL];
	fw_info->fw_info_rlc_v = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC_V];
	fw_info->fw_info_mmsch = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__MMSCH];
	fw_info->fw_info_psp_sysdrv = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SYS];
	fw_info->fw_info_psp_sosdrv = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SOS];
	fw_info->fw_info_psp_toc = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_TOC];
	fw_info->fw_info_psp_keydb = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_KEYDB];
	fw_info->fw_info_dfc_fw = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__DFC_FW];
	fw_info->fw_info_psp_spl = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SPL];
	fw_info->smu_fw_version = adapt->pp.smu_fw_version;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_psp_fw_info_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_fw_info *fw_info)
{
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SMU] = fw_info->fw_info_smu;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_CE] = fw_info->fw_info_cp_ce;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_PFP] = fw_info->fw_info_cp_pfp;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_ME] = fw_info->fw_info_cp_me;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_MEC_JT1] =
		fw_info->fw_info_cp_mec_jt1;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_MEC_JT2] =
		fw_info->fw_info_cp_mec_jt2;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_MEC1] = fw_info->fw_info_cp_mec1;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__CP_MEC2] = fw_info->fw_info_cp_mec2;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC] = fw_info->fw_info_rlc;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA0] = fw_info->fw_info_sdma0;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA1] = fw_info->fw_info_sdma1;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA2] = fw_info->fw_info_sdma2;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA3] = fw_info->fw_info_sdma3;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA4] = fw_info->fw_info_sdma4;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA5] = fw_info->fw_info_sdma5;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA6] = fw_info->fw_info_sdma6;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__SDMA7] = fw_info->fw_info_sdma7;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__VCN] = fw_info->fw_info_vcn;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__UVD] = fw_info->fw_info_uvd;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__VCE] = fw_info->fw_info_vce;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__ISP] = fw_info->fw_info_isp;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__DMCU_ERAM] = fw_info->fw_info_dmcu_eram;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__DMCU_ISR] = fw_info->fw_info_dmcu_isr;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM] =
		fw_info->fw_info_rlc_restore_list_gpm_mem;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM] =
		fw_info->fw_info_rlc_restore_list_srm_mem;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL] =
		fw_info->fw_info_rlc_restore_list_cntl;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RLC_V] = fw_info->fw_info_rlc_v;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__MMSCH] = fw_info->fw_info_mmsch;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SYS] = fw_info->fw_info_psp_sysdrv;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SOS] = fw_info->fw_info_psp_sosdrv;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_TOC] = fw_info->fw_info_psp_toc;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_KEYDB] = fw_info->fw_info_psp_keydb;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__DFC_FW] = fw_info->fw_info_dfc_fw;
	adapt->psp.fw_info[AMDGV_FIRMWARE_ID__PSP_SPL] = fw_info->fw_info_psp_spl;
	adapt->pp.smu_fw_version = fw_info->smu_fw_version;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum psp_status amdgv_psp_sw_init(struct amdgv_adapter *adapt)
{
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;

	adapt->psp.psp_cmd_km_mem = oss_zalloc(sizeof(struct psp_cmd_km));
	if (adapt->psp.psp_cmd_km_mem == NULL) {
		psp_ret = PSP_STATUS__ERROR_GENERIC;
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct psp_cmd_km));
	}

	adapt->psp.fw_num = AMDGV_FIRMWARE_ID__MAX;
	adapt->psp.fw_info = oss_zalloc(AMDGV_FIRMWARE_ID__MAX * sizeof(uint32_t));

	if (adapt->psp.fw_info == NULL) {
		psp_ret = PSP_STATUS__ERROR_GENERIC;
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				AMDGV_FIRMWARE_ID__MAX * sizeof(uint32_t));
	}

	adapt->psp.dfc_fw = (struct dfc_fw *)oss_zalloc(sizeof(struct dfc_fw));

	if (adapt->psp.dfc_fw == NULL) {
		psp_ret = PSP_STATUS__ERROR_GENERIC;
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct dfc_fw));
	}

	/* TMR init should be front since some asics allocate TMR at bottom of FB before
	 * other allocations, i.e. NV32.
	 * For other asics, TMR is allocated at top of FB which will not affect the other
	 * allocations either.
	 */
	if (psp_ret == PSP_STATUS__SUCCESS) {
		psp_ret = adapt->psp.tmr_init ? adapt->psp.tmr_init(adapt, adapt->psp.allocated_tmr_size) :
						 amdgv_psp_tmr_init(adapt, adapt->psp.allocated_tmr_size);
	}

	if (psp_ret == PSP_STATUS__SUCCESS)
		if (amdgv_psp_ring_init(adapt)) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_RING_INIT_FAIL, 0);
			psp_ret = PSP_STATUS__ERROR_GENERIC;
		}

	if (psp_ret == PSP_STATUS__SUCCESS)
		psp_ret = amdgv_psp_cmd_km_init(adapt);

	if (psp_ret == PSP_STATUS__SUCCESS &&
		adapt->fw_load_type != AMDGV_FW_LOAD_DIRECT)
		psp_ret = amdgv_psp_fw_mem_init(adapt);

	if (psp_ret == PSP_STATUS__SUCCESS) {
		psp_ret = amdgv_psp_ras_mem_init(adapt);
	}

	return psp_ret;
}

enum psp_status amdgv_psp_sw_fini(struct amdgv_adapter *adapt)
{
	int ret = 0;

	if (amdgv_psp_ras_mem_fini(adapt) != PSP_STATUS__SUCCESS)
		ret = AMDGV_FAILURE;
	if (adapt->fw_load_type != AMDGV_FW_LOAD_DIRECT) {
		if (amdgv_psp_fw_mem_fini(adapt) != PSP_STATUS__SUCCESS)
			ret = AMDGV_FAILURE;
	}

	if (amdgv_psp_cmd_km_fini(adapt) != PSP_STATUS__SUCCESS)
		ret = AMDGV_FAILURE;
	if (amdgv_psp_ring_fini(adapt) != PSP_STATUS__SUCCESS)
		ret = AMDGV_FAILURE;
	if (amdgv_psp_tmr_fini(adapt) != PSP_STATUS__SUCCESS)
		ret = AMDGV_FAILURE;

	if (adapt->psp.fw_info) {
		oss_free(adapt->psp.fw_info);
		adapt->psp.fw_info = NULL;
		adapt->psp.fw_num = 0;
	}

	if (adapt->psp.dfc_fw) {
		oss_free(adapt->psp.dfc_fw);
		adapt->psp.dfc_fw = NULL;
	}

	if (adapt->psp.psp_cmd_km_mem) {
		oss_free(adapt->psp.psp_cmd_km_mem);
		adapt->psp.psp_cmd_km_mem = NULL;
	}

	oss_memset(&adapt->psp, 0, sizeof(struct psp_context));

	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_EXIT_FAIL, 0);
		return PSP_STATUS__ERROR_GENERIC;
	}

	return PSP_STATUS__SUCCESS;
}
