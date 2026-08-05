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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_notify.h"
#include "amdgv_vfmgr.h"
#include "amdgv_ras_mgr.h"
#include "amdgv_ras_eeprom_i2c.h"
#include "amdgv_ras_mp1_v13_0.h"
#include "amdgv_ras_mp1.h"
#include "amdgv_ras_nbio_v7_9.h"
#include "amdgv_ras_cmd.h"
#include "amdgv_ras_mce.h"
#include "ras.h"

#define MAX_AID_NUM_PER_SOCKET_GFX9     4
#define MAX_XCD_NUM_PER_AID_GFX9        2

#define MAX_AID_NUM_PER_SOCKET_GFX12    16
#define MAX_XCD_NUM_PER_AID_GFX12       4

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int amdgv_enable_uniras(struct amdgv_adapter *adapt, bool enable);

#define AMDGV_RAS_REPORT_TOTAL_ECC_INFO(BLOCK, ecc_count) do { \
	if (ecc_count->total_ce_count) { \
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_ECC_##BLOCK##_CE_TOTAL, \
				ecc_count->total_ce_count); \
		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ECC_CORR_ERROR, \
				  "%s ECC Correctable Error Detected.Count:%d", \
				  #BLOCK, ecc_count->total_ce_count); \
	} \
	if (ecc_count->total_ue_count) { \
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_ECC_##BLOCK##_UE_TOTAL, \
				ecc_count->total_ue_count); \
		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ECC_UNCORR_ERROR, \
				  "%s ECC UnCorrectable Error Detected.Count:%d", \
				  #BLOCK, ecc_count->total_ue_count); \
	} \
	if (ecc_count->total_de_count) { \
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_ECC_UMC_DE_TOTAL, \
				ecc_count->total_de_count); \
		amdgv_notify_shim(adapt->dev, AMDGV_NOTIFICATION_ECC_DFCORR_ERROR, \
				  "%s ECC Deferred Error Detected.Count:%d", \
				  #BLOCK, ecc_count->total_de_count); \
	} \
} while (0)

static void amdgv_ras_mgr_report_total_ecc_info(struct amdgv_adapter *adapt,
			enum ras_block_id blk, struct ras_ecc_count *ecc_count)
{
	if (!ecc_count)
		return;

	switch (blk) {
	case RAS_BLOCK_ID__UMC:
		AMDGV_RAS_REPORT_TOTAL_ECC_INFO(UMC, ecc_count);
		break;
	case RAS_BLOCK_ID__GFX:
		AMDGV_RAS_REPORT_TOTAL_ECC_INFO(GFX, ecc_count);
		break;
	case RAS_BLOCK_ID__SDMA:
		AMDGV_RAS_REPORT_TOTAL_ECC_INFO(SDMA, ecc_count);
		break;
	case RAS_BLOCK_ID__MMHUB:
		AMDGV_RAS_REPORT_TOTAL_ECC_INFO(MMHUB, ecc_count);
		break;
	case RAS_BLOCK_ID__XGMI_WAFL:
		AMDGV_RAS_REPORT_TOTAL_ECC_INFO(XGMI_WAFL, ecc_count);
		break;
	default:
		break;
	}
}

struct amdgv_ras_mgr *amdgv_ras_mgr_get_context(struct amdgv_adapter *adapt)
{
	if (!adapt)
		return NULL;

	return (struct amdgv_ras_mgr *)adapt->ecc.ras_mgr;
}

static bool amdgv_ras_mgr_is_ready(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);

	if (ras_mgr && ras_mgr->ras_core && ras_mgr->ras_is_ready &&
	    ras_core_is_ready(ras_mgr->ras_core))
		return true;

	return false;
}

static void amdgv_ras_mgr_init_event_mgr(struct ras_event_manager *mgr)
{
	struct ras_event_state *event_state;
	int i;

	oss_memset(mgr, 0, sizeof(*mgr));

	for (i = 0; i < ARRAY_SIZE(mgr->event_state); i++) {
		event_state = &mgr->event_state[i];
		event_state->last_seqno = RAS_EVENT_TYPE_INVALID;
		event_state->count = 0;
	}
}

static void amdgpv_ras_mgr_init_event_mgr(struct ras_core_context *ras_core)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_event_manager *event_mgr;
	struct amdgv_hive_info *hive;

	hive = amdgv_get_xgmi_hive(adapt);
	event_mgr = hive ? &hive->event_mgr : &ras_mgr->ras_event_mgr;

	/* init event manager with node 0 on xgmi system */
	if (!hive || adapt->xgmi.node_id == 0)
		amdgv_ras_mgr_init_event_mgr(event_mgr);
}

static int amdgv_ras_mgr_init_aca_config(struct amdgv_adapter *adapt,
		struct ras_core_config *config)
{
	struct ras_aca_config *aca_cfg = &config->aca_cfg;

	switch (config->gfx_ip_version) {
	case IP_VERSION(9, 4, 3):
	case IP_VERSION(9, 4, 4):
	case IP_VERSION(9, 5, 0):
		aca_cfg->aid_num_per_socket = MAX_AID_NUM_PER_SOCKET_GFX9;
		aca_cfg->xcd_num_per_aid = MAX_XCD_NUM_PER_AID_GFX9;
		break;
	case IP_VERSION(12, 1, 0):
		aca_cfg->aid_num_per_socket = MAX_AID_NUM_PER_SOCKET_GFX12;
		aca_cfg->xcd_num_per_aid = MAX_XCD_NUM_PER_AID_GFX12;
		break;
	default:
		return -RAS_CORE_EINVAL;
	}

	return 0;
}

static int amdgv_ras_mgr_init_eeprom_config(struct amdgv_adapter *adapt,
		struct ras_core_config *config)
{
	struct ras_eeprom_config *eeprom_cfg = &config->eeprom_cfg;

	eeprom_cfg->eeprom_sys_fn = &amdgv_ras_eeprom_i2c_sys_func;

	return 0;
}

static int amdgv_ras_mgr_init_mp1_config(struct amdgv_adapter *adapt,
		struct ras_core_config *config)
{
	struct ras_mp1_config *mp1_cfg = &config->mp1_cfg;
	int ret = 0;

	switch (config->mp1_ip_version) {
	case IP_VERSION(13, 0, 6):
	case IP_VERSION(13, 0, 14):
	case IP_VERSION(13, 0, 12):
		mp1_cfg->mp1_sys_fn = &amdgv_ras_mp1_sys_func_v13_0;
		break;
	case IP_VERSION(15, 0, 8):
		mp1_cfg->mp1_sys_fn = &amdgv_ras_mp1_sys_func;
		break;
	default:
		RAS_DEV_ERR(adapt,
			"The mp1(0x%x) ras config is not right!\n",
			config->mp1_ip_version);
		ret = -RAS_CORE_EINVAL;
		break;
	}

	return ret;
}

static int amdgv_ras_mgr_init_nbio_config(struct amdgv_adapter *adapt,
		struct ras_core_config *config)
{
	struct ras_nbio_config *nbio_cfg = &config->nbio_cfg;
	int ret = 0;

	switch (config->nbio_ip_version) {
	case IP_VERSION(7, 9, 0):
		nbio_cfg->nbio_sys_fn = &amdgv_ras_nbio_sys_func_v7_9;
		break;
	case IP_VERSION(6, 3, 2):
		/* Not supported yet */
		break;
	default:
		RAS_DEV_ERR(adapt,
			"The nbio(0x%x) ras config is not right!\n",
			config->nbio_ip_version);
		ret = -RAS_CORE_EINVAL;
		break;
	}

	return ret;
}

static int amdgv_ras_mgr_get_ras_psp_system_status(struct ras_core_context *ras_core,
			struct ras_psp_sys_status *status)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;

	status->psp_cmd_mutex = adapt->psp.km_cmd_context.lock;

	status->uniras_load_fw = true;

	status->use_dedicated_memory = false;

	return 0;
}

static int amdgv_ras_mgr_get_ras_param(struct ras_core_context *ras_core,
			struct ras_param *param)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;

	if (!param)
		return -RAS_CORE_EINVAL;

	param->ta_param.dgpu_mode = 1;
	param->ta_param.xcc_mask = adapt->mcp.gfx.xcc_mask;
	param->ta_param.active_umc_mask = lower_32_bits(adapt->umc.active_mask);
	param->ta_param.ext_umc_mask = upper_32_bits(adapt->umc.active_mask);

	if (adapt->umc.funcs && adapt->umc.funcs->query_ras_memchandis)
		param->ta_param.channel_dis_num = adapt->umc.channel_dis_num;

#if 0
	// Pending implementation
	param->fw_param.rl_bin.fw_version = adev->psp.rl.fw_version;
	param->fw_param.rl_bin.feature_version = adev->psp.rl.feature_version;
	param->fw_param.rl_bin.bin_size = adev->psp.rl.size_bytes;
	param->fw_param.rl_bin.bin_addr = adev->psp.rl.start_addr;

	param->fw_param.ta_bin.fw_version = adev->psp.ras_context.context->bin_desc.fw_version;
	param->fw_param.ta_bin.feature_version = adev->psp.ras_context.context->bin_desc.feature_version;
	param->fw_param.ta_bin.bin_size = adev->psp.ras_context.context->bin_desc.size_bytes;
	param->fw_param.ta_bin.bin_addr = adev->psp.ras_context.context->bin_desc.start_addr;
#endif

	return 0;
}

static int amdgv_ras_mgr_psp_translate_addr(struct ras_core_context *ras_core,
	struct ras_psp_addr_trans_in *in, struct ras_psp_addr_trans_out *out)
{
	int ret = 0;
#if 0  // Sample code
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_mem_error_info *bp;
	uint64_t all_or  = 0;
	uint64_t all_and = ~0ULL;
	int i;

	if (!in || !out)
		return -RAS_CORE_EINVAL;

	bp = oss_zalloc(sizeof(*bp));
	if (!bp)
		return -RAS_CORE_ENOMEM;

	/*
		libgv add implementation for the following function interfaces：
		int amdgv_psp_translate_bp_addr(struct amdgv_adapter *adapt,
			uint64_t ipid, uint64_t mca_addr, uint32_t nps,
			struct ras_mem_error_info *mem_info);
	*/
	ret = amdgv_psp_translate_bp_addr(adapt, in->ipid, in->mca_addr, in->nps, bp);
	if (ret)
		goto out;

	if (!bp->entry_num) {
		ret = -RAS_CORE_EIO;
		goto out;
	}

	out->channel_id     = bp->ChannelId;
	out->socket_id      = bp->SocketId;
	out->mem_die_id     = bp->MemDieId;
	out->dram_entity_id = bp->DramEntityId;
	out->umc_inst_id    = bp->UmcInstId;

	for (i = 0; i < bp->entry_num; i++) {
		all_or |= RAS_PFN_TO_ADDR(bp->pfns[i]);
		all_and &= RAS_PFN_TO_ADDR(bp->pfns[i]);
	}

	out->row_pa = all_or;
	out->pa_flip_mask = all_or ^ all_and;
out:
	oss_free(bp);
#endif
	return ret;
}

const struct ras_psp_sys_func amdgv_ras_psp_sys_func = {
	.get_ras_psp_system_status = amdgv_ras_mgr_get_ras_psp_system_status,
	.get_ras_param = amdgv_ras_mgr_get_ras_param,
	.psp_translate_addr = amdgv_ras_mgr_psp_translate_addr,
};

static int amdgv_ras_mgr_init_psp_config(struct amdgv_adapter *adapt,
			struct ras_core_config *config)
{
	struct ras_psp_config *psp_cfg = &config->psp_cfg;

	psp_cfg->psp_sys_fn = &amdgv_ras_psp_sys_func;

	return 0;
}

static struct ras_core_context *amdgv_ras_mgr_create_ras_core(struct amdgv_adapter *adapt)
{
	struct ras_core_config init_config;

	oss_memset(&init_config, 0, sizeof(init_config));

	init_config.umc_ip_version = amdgv_ip_version(adapt, UMC_HWIP, 0);
	init_config.mp1_ip_version = amdgv_ip_version(adapt, MP1_HWIP, 0);
	init_config.gfx_ip_version = amdgv_ip_version(adapt, GC_HWIP, 0);
	init_config.nbio_ip_version = amdgv_ip_version(adapt, NBIO_HWIP, 0);
	init_config.psp_ip_version = amdgv_ip_version(adapt, MP0_HWIP, 0);

	if (init_config.gfx_ip_version == IP_VERSION(12, 1, 0))
		init_config.aca_ip_version = IP_VERSION(5, 0, 0);
	else if (init_config.umc_ip_version == IP_VERSION(12, 0, 0) ||
		init_config.umc_ip_version == IP_VERSION(12, 5, 0))
		init_config.aca_ip_version = IP_VERSION(1, 0, 0);

	if (init_config.mp1_ip_version == IP_VERSION(15, 0, 8))
		init_config.early_init_service_supported = true;

	init_config.sys_fn = &amdgv_ras_sys_fn;
	init_config.ras_eeprom_supported = true;
	init_config.poison_supported = false;

	amdgv_ras_mgr_init_aca_config(adapt, &init_config);
	amdgv_ras_mgr_init_eeprom_config(adapt, &init_config);
	amdgv_ras_mgr_init_mp1_config(adapt, &init_config);
	amdgv_ras_mgr_init_nbio_config(adapt, &init_config);
	amdgv_ras_mgr_init_psp_config(adapt, &init_config);

	return ras_core_create(&init_config);
}

static int amdgv_ras_mgr_ecc_init(struct amdgv_adapter *adapt)
{
	adapt->opt.bad_page_detection_mode = 0;
	switch (adapt->opt.bad_page_detection_mode) {
	case AMDGV_BAD_PAGE_DETECTION_MODE1:
		adapt->ecc.bad_page_detection_mode |=
			(1 << AMDGV_RAS_ECC_FLAG_SKIP_BAD_PAGE_OPS) |
			(1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA);
		break;
	case AMDGV_BAD_PAGE_DETECTION_MODE2:
		adapt->ecc.bad_page_detection_mode |=
				1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA;
		break;
	default:
		adapt->ecc.bad_page_detection_mode = 0;
	}

	adapt->umc.reset_mode = AMDGV_RESET_MODE1;

	return 0;
}

static int amdgv_ras_mgr_sw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr;
	struct vf_auto_cmd_mgr *cmd_mgr;
	int ret = 0, i;

	amdgv_ras_mgr_ecc_init(adapt);
	oss_atomic_set(adapt->in_ecc_recovery, 0);

	ras_mgr = oss_zalloc(sizeof(*ras_mgr));
	if (!ras_mgr)
		return AMDGV_FAILURE;

	oss_memset(ras_mgr, 0, sizeof(*ras_mgr));
	adapt->ecc.ras_mgr = ras_mgr;
	ras_mgr->adapt = adapt;

	ras_mgr->ras_core = amdgv_ras_mgr_create_ras_core(adapt);
	if (!ras_mgr->ras_core) {
		RAS_DEV_ERR(adapt, "Failed to create ras core!\n");
		ret = AMDGV_FAILURE;
		goto err;
	}

	ras_mgr->ras_core->dev = adapt;

	ras_core_sw_init(ras_mgr->ras_core);

	amdgpv_ras_mgr_init_event_mgr(ras_mgr->ras_core);

	amdgv_ras_cmd_add_device(ras_mgr->ras_core);

	for (i = 0; i < AMDGV_MAX_VF_NUM; i++) {
		cmd_mgr = &ras_mgr->vf_auto_cmd_mgr[i];
		oss_memset(cmd_mgr, 0, sizeof(*cmd_mgr));
		OSS_INIT_LIST_HEAD(&cmd_mgr->cmd_list);
		oss_mutex_init_raw(&cmd_mgr->cmd_lock);
	}

	amdgv_ras_mce_sw_init(adapt);

	return 0;

err:
	oss_free(ras_mgr);
	return ret;
}

static int amdgv_ras_mgr_sw_fini(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct vf_auto_cmd_mgr *cmd_mgr;
	struct auto_update_cmd *auto_cmd, *tmp;
	int i;

	if (!ras_mgr)
		return AMDGV_FAILURE;

	amdgv_ras_mce_sw_fini(adapt);

	for (i = 0; i < AMDGV_MAX_VF_NUM; i++) {
		cmd_mgr = &ras_mgr->vf_auto_cmd_mgr[i];
		oss_mutex_lock(&cmd_mgr->cmd_lock);
		oss_list_for_each_entry_safe(auto_cmd, tmp,
			&cmd_mgr->cmd_list, struct auto_update_cmd, node) {
			oss_list_del(&auto_cmd->node);
			oss_free(auto_cmd);
		}
		oss_mutex_unlock(&cmd_mgr->cmd_lock);
		oss_mutex_destroy_raw(&cmd_mgr->cmd_lock);
		oss_memset(cmd_mgr, 0, sizeof(*cmd_mgr));
	}

	amdgv_ras_cmd_remove_device(ras_mgr->ras_core);
	ras_core_sw_fini(ras_mgr->ras_core);
	ras_core_destroy(ras_mgr->ras_core);
	ras_mgr->ras_core = NULL;
	oss_free(adapt->ecc.ras_mgr);
	adapt->ecc.ras_mgr = NULL;

	return 0;
}

static int amdgv_ras_mgr_hw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr =
			amdgv_ras_mgr_get_context(adapt);
	int ret;

	if (!ras_mgr || !ras_mgr->ras_core)
		return AMDGV_FAILURE;

	amdgv_ecc_check_support(adapt);

	ret = ras_core_hw_init(ras_mgr->ras_core);
	if (ret) {
		RAS_DEV_ERR(adapt, "Failed to initialize ras core!\n");
		return ret;
	}

	ras_mgr->ras_is_ready = true;

	amdgv_enable_uniras(adapt, true);

	adapt->cper.enabled = true;

	RAS_DEV_INFO(adapt, "AMDGV RAS Is Ready.\n");
	return 0;
}

static int amdgv_ras_mgr_hw_fini(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr =
			amdgv_ras_mgr_get_context(adapt);

	if (!amdgv_ras_mgr_is_ready(adapt))
		return AMDGV_FAILURE;

	ras_mgr->ras_is_ready = false;
	ras_core_hw_fini(ras_mgr->ras_core);
	amdgv_enable_uniras(adapt, false);
	return 0;
}

int amdgv_ras_event_handler(struct amdgv_adapter *adapt,
		struct amdgv_sched_event *event)
{
	return amdgv_ras_process_ras_event_dispatch(adapt, event->idx_vf);
}

int amdgv_ras_handle_vf_cmd(struct amdgv_adapter *adapt,
		uint32_t idx_vf, uint64_t gpa_addr, uint32_t gpa_size)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	uint32_t mem_size = OSS_GPU_PAGE_SIZE;
	struct ras_cmd_ctx *rcmd;
	uint32_t out_data_size = 0;
	uint64_t tlm_offset, tlm_size;
	uint64_t offset_in_table;
	int ret;

	if (!gpa_addr || (gpa_size < sizeof(*rcmd)) ||
	    (gpa_size > RAS_CMD_MAX_BUF_SIZE) ||
	    (idx_vf >= AMDGV_MAX_VF_NUM))
		return -RAS_CORE_EINVAL;

	if (!ras_mgr || !ras_mgr->ras_core)
		return -RAS_CORE_EPERM;

	tlm_offset = GET_VF_TABLE_OFFSET_BY_ID(adapt, idx_vf, RAS_TELEMETRY);
	tlm_size = KBYTES_TO_BYTES(GET_VF_TABLE_SIZE_KB_BY_ID(adapt, idx_vf, RAS_TELEMETRY));
	if (!tlm_size || (gpa_addr < tlm_offset) || (gpa_addr - tlm_offset > tlm_size - gpa_size)) {
		AMDGV_ERROR("GPA 0x%llx+0x%x from VF%u is outside RAS telemetry region [0x%llx, 0x%llx)\n",
			    (unsigned long long)gpa_addr, gpa_size, idx_vf,
			    (unsigned long long)tlm_offset,
			    (unsigned long long)(tlm_offset + tlm_size));
		return -RAS_CORE_EINVAL;
	}

	offset_in_table = gpa_addr - tlm_offset;

	mem_size = (gpa_size < mem_size) ? mem_size : gpa_size;
	rcmd = oss_alloc_memory(mem_size);
	if (!rcmd)
		return -RAS_CORE_ENOMEM;
	oss_memset(rcmd, 0, mem_size);

	ret = amdgv_vfmgr_copy_from_vf_xchg_table(adapt, idx_vf,
		AMD_SRIOV_MSG_RAS_TELEMETRY_TABLE_ID, offset_in_table,
		rcmd, sizeof(*rcmd));
	if (ret)
		goto out;

	if (!rcmd->cmd_id || (rcmd->cmd_id >= RAS_CMD_ID_MXGPU_VF_END)) {
		AMDGV_ERROR("Invaild ras command %u from VF%u\n", rcmd->cmd_id, idx_vf);
		ret = RAS_CORE_NOT_SUPPORTED;
		goto out;
	}

	rcmd->output_buf_size = mem_size - sizeof(*rcmd);

	ret = amdgv_ras_cmd_handle_vf_cmd(ras_mgr->ras_core, idx_vf, rcmd);
	if (ret) {
		rcmd->cmd_res = ret;
		rcmd->output_size = 0;
		out_data_size = sizeof(*rcmd);
	} else {
		out_data_size = sizeof(*rcmd) + rcmd->output_size;
	}

	ret = amdgv_vfmgr_copy_to_vf_xchg_table(adapt, idx_vf,
		AMD_SRIOV_MSG_RAS_TELEMETRY_TABLE_ID, offset_in_table,
		rcmd, out_data_size);

out:
	oss_free_memory(rcmd);
	return ret;
}

int amdgv_ras_remote_cmd_handler(struct amdgv_adapter *adapt,
	struct amdgv_sched_event *event)
{
	uint64_t gpa_addr;
	uint32_t gpa_size;

	if (!event)
		return -RAS_CORE_EPERM;

	gpa_addr = event->data.remote_ras.gpa_addr;
	gpa_size = event->data.remote_ras.gpa_size;

	return amdgv_ras_handle_vf_cmd(adapt, event->idx_vf, gpa_addr, gpa_size);
}

int amdgv_ras_ioctl_handler(struct amdgv_adapter *adapt, void *data)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct amdgv_ras_ioctl_cmd *cmd = (struct amdgv_ras_ioctl_cmd *)data;

	if (!ras_mgr || !ras_mgr->ras_core || !cmd)
		return -RAS_CORE_EPERM;

	return amdgv_ras_cmd_ioctl_handler(ras_mgr->ras_core,
			(uint8_t *)cmd->data_addr, cmd->data_len);
}

struct amdgv_init_func amdgv_ras_mgr_func = {
	.name = "amdgv_ras_mgr_func",
	.sw_init = amdgv_ras_mgr_sw_init,
	.sw_fini = amdgv_ras_mgr_sw_fini,
	.hw_init = amdgv_ras_mgr_hw_init,
	.hw_fini = amdgv_ras_mgr_hw_fini,
};

struct amdgv_init_func *amdgv_ras_mgr_get_init_func(struct amdgv_adapter *adapt)
{
	return &amdgv_ras_mgr_func;
}

static int amdgv_enable_uniras(struct amdgv_adapter *adapt, bool enable)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);

	if (!ras_mgr || !ras_mgr->ras_core)
		return -RAS_CORE_EPERM;

	return ras_core_set_status(ras_mgr->ras_core, enable);
}

bool amdgv_uniras_enabled(struct amdgv_adapter *adapt)
{
	return adapt->opt.unified_ras_enabled;
}

int amdgv_ras_mgr_handle_fatal_interrupt(struct amdgv_adapter *adapt, void *data)
{
	struct amdgv_ras_mgr *ras_mgr =
			amdgv_ras_mgr_get_context(adapt);

	if (!amdgv_ras_mgr_is_ready(adapt))
		return AMDGV_FAILURE;

	return ras_core_handle_nbio_irq(ras_mgr->ras_core, data);
}

uint64_t amdgv_ras_mgr_gen_ras_event_seqno(struct amdgv_adapter *adapt,
			enum ras_seqno_type seqno_type)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	int ret;
	uint64_t seq_no;

	if (!amdgv_ras_mgr_is_ready(adapt) ||
		(seqno_type >= RAS_SEQNO_TYPE_COUNT_MAX))
		return 0;

	seq_no = ras_core_gen_seqno(ras_mgr->ras_core, seqno_type);

	if ((seqno_type == RAS_SEQNO_TYPE_DE) ||
	    (seqno_type == RAS_SEQNO_TYPE_POISON_CONSUMPTION)) {
		ret = ras_core_put_seqno(ras_mgr->ras_core, seqno_type, seq_no);
		if (ret)
			RAS_DEV_WARN(adapt, "There are too many ras interrupts!");
	}

	return seq_no;
}

int amdgv_ras_mgr_handle_controller_interrupt(struct amdgv_adapter *adapt, void *data)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	uint32_t idx_vf = ((struct amdgv_iv_entry *)data)->src_data[0];
	uint64_t seqno;

	if (!amdgv_ras_mgr_is_ready(adapt))
		return -RAS_CORE_EPERM;

	if (ras_core_poison_supported(ras_mgr->ras_core)) {
		seqno = amdgv_ras_mgr_gen_ras_event_seqno(adapt, RAS_SEQNO_TYPE_DE);
		RAS_DEV_INFO(adapt,
			"{%llu} RAS poison is created, no user action is needed.\n",
			seqno);
	}

	return amdgv_ras_process_handle_umc_interrupt(adapt, idx_vf, NULL);
}

int amdgv_ras_mgr_handle_consumer_interrupt(struct amdgv_adapter *adapt, void *data)
{
	struct amdgv_sched_event *event = (struct amdgv_sched_event *)data;

	if (!amdgv_ras_mgr_is_ready(adapt))
		return -RAS_CORE_EPERM;

	return amdgv_ras_process_handle_consumption_interrupt(adapt,
			event->idx_vf, &event->data.poison.consumption.block);
}

int amdgv_ras_mgr_copy_bp_records_to_vf(struct amdgv_adapter *adapt,
				uint32_t idx_vf, uint32_t allowed_size,
				uint32_t *write_size, uint32_t *more)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct eeprom_umc_record record;
	uint32_t max_slot_idx = allowed_size/sizeof(uint64_t);
	uint32_t slot_idx = 0;
	uint64_t addr;
	int count, i;

	*more = 0;

	count = ras_umc_get_badpage_count(ras_mgr->ras_core);
	for (i = 0; i < count; i++) {
		if (ras_umc_get_badpage_record(ras_mgr->ras_core, i, &record))
			continue;
		addr = record.cur_nps_retired_row_pfn << AMDGV_GPU_PAGE_SHIFT;
		if (idx_vf != amdgv_umc_calc_retired_page_vf_slot(adapt, addr))
			continue;
		if (amdgv_umc_check_bp_in_critical_region(adapt, addr, idx_vf, true)) {
			amdgv_umc_log_bp_errors(adapt, i);
		} else {
			if (!amdgv_vfmgr_copy_bp_entry_to_vf_fb(adapt, idx_vf,
			    record.cur_nps_retired_row_pfn, slot_idx, NULL)) {
				if (slot_idx < max_slot_idx)
					slot_idx++;
				else
					*more = true;
			}
		}
	}

	*write_size = slot_idx * sizeof(uint64_t);

	return 0;
}

int amdgv_ras_mgr_get_curr_nps_mode(struct amdgv_adapter *adapt,
		enum amdgv_memory_partition_mode *nps_mode)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	uint32_t mode;

	if (!amdgv_ras_mgr_is_ready(adapt))
		return -RAS_CORE_EINVAL;

	mode = ras_core_get_curr_nps_mode(ras_mgr->ras_core);
	if (!mode || mode >= AMDGV_MEMORY_PARTITION_MODE_MAX)
		return -RAS_CORE_EINVAL;

	*nps_mode = mode;

	return 0;
}

bool amdgv_ras_mgr_check_retired_addr(struct amdgv_adapter *adapt, uint64_t addr)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);

	if (!amdgv_ras_mgr_is_ready(adapt))
		return false;

	return ras_umc_check_retired_addr(ras_mgr->ras_core, addr);
}

void amdgv_ras_mgr_flush_ras_ecc_info(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_ecc_count ecc_count;
	enum ras_block_id blk;

	if (!amdgv_ras_mgr_is_ready(adapt))
		return;

	if (ras_core_update_ecc_info(ras_mgr->ras_core))
		return;

	for (blk = RAS_BLOCK_ID__UMC; blk < RAS_BLOCK_ID__LAST; blk++) {
		oss_memset(&ecc_count, 0, sizeof(ecc_count));
		if (ras_core_query_block_ecc_data(ras_mgr->ras_core,
				blk, &ecc_count, false))
			continue;

		amdgv_ras_mgr_report_total_ecc_info(adapt, blk, &ecc_count);
	}
}

void amdgv_ras_mgr_update_ras_ecc(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);

	if (!amdgv_ras_mgr_is_ready(adapt))
		return;

	ras_core_update_ecc_info(ras_mgr->ras_core);
}

int amdgv_ras_mgr_query_block_ecc_data(struct amdgv_adapter *adapt,
		uint32_t block, uint32_t idx_vf, uint64_t *ce, uint64_t *ue, uint64_t *de)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct vf_ras_lifespan_data *vf_ras;
	struct ras_ecc_count ecc_count = {0};
	int ret;

	if ((block >= RAS_BLOCK_ID__LAST) || !ce || !ue || !de)
		return -RAS_CORE_EINVAL;

	if (idx_vf >= AMDGV_MAX_VF_NUM)
		return -RAS_CORE_EINVAL;

	ret = ras_core_query_block_ecc_data(ras_mgr->ras_core, block, &ecc_count, false);
	if (ret)
		return ret;

	vf_ras = &ras_mgr->array_vf[idx_vf];
	*ce = (ecc_count.total_ce_count > vf_ras->ecc_count_baseline[block].ce_count) ?
		(ecc_count.total_ce_count - vf_ras->ecc_count_baseline[block].ce_count) : 0;
	*ue = (ecc_count.total_ue_count > vf_ras->ecc_count_baseline[block].ue_count) ?
		(ecc_count.total_ue_count - vf_ras->ecc_count_baseline[block].ue_count) : 0;
	*de = (ecc_count.total_de_count > vf_ras->ecc_count_baseline[block].de_count) ?
		(ecc_count.total_de_count - vf_ras->ecc_count_baseline[block].de_count) : 0;

	return 0;
}

void amdgv_ras_mgr_vf_ecc_count_init(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct vf_ras_lifespan_data *vf_ras;
	struct ras_ecc_count ecc_count;
	enum ras_block_id blk;

	if (idx_vf >= AMDGV_MAX_VF_NUM) {
		RAS_DEV_ERR(adapt,
			"Invalid vf idx %u for VF RAS ecc count init\n", idx_vf);
		return;
	}

	if (!amdgv_ras_mgr_is_ready(adapt))
		return;

	ras_core_update_ecc_info(ras_mgr->ras_core);

	vf_ras = &ras_mgr->array_vf[idx_vf];
	oss_memset(vf_ras->ecc_count_baseline, 0, sizeof(vf_ras->ecc_count_baseline));

	for (blk = RAS_BLOCK_ID__UMC; blk < RAS_BLOCK_ID__LAST; blk++) {
		oss_memset(&ecc_count, 0, sizeof(ecc_count));
		if (ras_core_query_block_ecc_data(ras_mgr->ras_core,
				blk, &ecc_count, false))
			continue;

		vf_ras->ecc_count_baseline[blk].ce_count = ecc_count.total_ce_count;
		vf_ras->ecc_count_baseline[blk].ue_count = ecc_count.total_ue_count;
		vf_ras->ecc_count_baseline[blk].de_count = ecc_count.total_de_count;
	}
}

static void amdgv_ras_mgr_vf_cper_ptr_init(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct vf_ras_lifespan_data *vf_ras;
	struct ras_cmd_cper_snapshot_req snapshot_req = {0};
	struct ras_cmd_cper_snapshot_rsp snapshot_rsp = {0};
	int ret;

	if (idx_vf >= AMDGV_MAX_VF_NUM) {
		RAS_DEV_ERR(adapt,
			"Invalid vf idx %u for VF RAS cper ptr init\n", idx_vf);
		return;
	}

	if (!amdgv_ras_mgr_is_ready(adapt))
		return;

	vf_ras = &ras_mgr->array_vf[idx_vf];
	oss_memset(&vf_ras->cper_ptr_record, 0, sizeof(vf_ras->cper_ptr_record));

	ret = amdgv_ras_mgr_handle_ras_cmd(adapt,
		RAS_CMD__GET_CPER_SNAPSHOT,
		&snapshot_req, sizeof(struct ras_cmd_cper_snapshot_req),
		&snapshot_rsp, sizeof(struct ras_cmd_cper_snapshot_rsp));
	if (!ret)
		vf_ras->cper_ptr_record.start_rptr = snapshot_rsp.latest_cper_id;
}

void amdgv_ras_mgr_vf_lifespan_init(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	amdgv_ras_mgr_vf_ecc_count_init(adapt, idx_vf);
	amdgv_ras_mgr_vf_cper_ptr_init(adapt, idx_vf);
}

bool amdgv_ras_mgr_is_rma(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);

	if (!ras_mgr || !ras_mgr->ras_core || !ras_mgr->ras_is_ready)
		return false;

	return ras_core_gpu_is_rma(ras_mgr->ras_core);
}

int amdgv_ras_mgr_clear_vf_auto_list(struct amdgv_adapter *adapt,
		uint32_t idx_vf)
{
	if (idx_vf >= AMDGV_MAX_VF_NUM)
		return -RAS_CORE_EINVAL;

	return amdgv_ras_cmd_clear_vf_auto_list(adapt, idx_vf);
}

int amdgv_ras_mgr_handle_ras_cmd(struct amdgv_adapter *adapt,
			uint32_t cmd_id, void *input, uint32_t input_size,
			void *output, uint32_t out_size)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_ctx *cmd_ctx;
	uint32_t ctx_buf_size;
	int ret;

	if (!amdgv_ras_mgr_is_ready(adapt))
		return -RAS_CORE_EPERM;

	if (input_size && !input)
		return -RAS_CORE_EINVAL;

	if (input_size > sizeof(cmd_ctx->input_buff_raw))
		return -RAS_CORE_EINVAL;

	if (out_size > ((uint32_t)(~0U) - sizeof(*cmd_ctx)))
		return -RAS_CORE_EINVAL;

	ctx_buf_size = sizeof(*cmd_ctx) + out_size;
	if (ctx_buf_size < PAGE_SIZE)
		ctx_buf_size = PAGE_SIZE;

	cmd_ctx = oss_alloc_memory(ctx_buf_size);
	if (!cmd_ctx)
		return -RAS_CORE_ENOMEM;

	oss_memset(cmd_ctx, 0, ctx_buf_size);

	cmd_ctx->cmd_id = cmd_id;

	oss_memcpy(cmd_ctx->input_buff_raw, input, input_size);
	cmd_ctx->input_size = input_size;
	cmd_ctx->output_buf_size = ctx_buf_size - sizeof(*cmd_ctx);

	ret = amdgv_ras_submit_cmd(ras_mgr->ras_core, cmd_ctx, NULL);
	if (!ret && !cmd_ctx->cmd_res && output && (out_size == cmd_ctx->output_size))
		oss_memcpy(output, cmd_ctx->output_buff_raw, cmd_ctx->output_size);

	oss_free_memory(cmd_ctx);

	return ret;
}

int amdgv_ras_mgr_fetch_and_sort_bps(struct amdgv_adapter *adapt,
			uint64_t **bp_offsets, int *bp_count)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct eeprom_umc_record record;
	int count, i;

	if (!bp_offsets || !bp_count)
		return -RAS_CORE_EPERM;

	if (!amdgv_ras_mgr_is_ready(adapt)) {
		*bp_count = 0;
		*bp_offsets = NULL;
		return -RAS_CORE_EPERM;
	}

	count = ras_umc_get_badpage_count(ras_mgr->ras_core);
	*bp_count = count;
	if (!count) {
		*bp_offsets = NULL;
		return 0;
	}

	*bp_offsets = oss_zalloc(count * sizeof(uint64_t));
	if (!*bp_offsets)
		return -RAS_CORE_ENOMEM;

	for (i = 0; i < count; i++) {
		if (ras_umc_get_badpage_record(ras_mgr->ras_core, i, &record))
			continue;
		(*bp_offsets)[i] = record.cur_nps_retired_row_pfn << AMDGV_GPU_PAGE_SHIFT;
	}

	amdgv_umc_sort_bp_offsets(*bp_offsets, count);
	return 0;
}

int amdgv_ras_mgr_early_init_service(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	int ret;

	if (!ras_mgr || !ras_mgr->ras_core) {
		RAS_DEV_ERR(adapt, "amdgpu ras sw is not ready!\n");
		return -RAS_CORE_EPERM;
	}

	ret = ras_core_eeprom_early_init_service(ras_mgr->ras_core);
	if (ret)
		RAS_DEV_WARN(adapt, "RAS early init failure! ret:%d\n", ret);

	return ret;
}

void amdgv_ras_mgr_get_ras_caps(struct amdgv_adapter *adapt,
				struct amd_sriov_msg_pf2vf_info *pf2vf_msg)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_get_ras_cap_req req = { 0 };
	struct ras_cmd_get_ras_cap_rsp rsp = { 0 };
	struct amd_sriov_uniras_caps *uniras_caps;
	uint64_t core_caps = 0;
	uint32_t ext_ecc_type = 0;
	uint32_t int_ecc_attr = 0;

	if (!ras_mgr || !ras_mgr->ras_core)
		return;

	if (amdgv_ras_mgr_handle_ras_cmd(adapt, RAS_CMD__GET_RAS_CAP,
			&req, sizeof(req), &rsp, sizeof(rsp))) {
		RAS_DEV_WARN(adapt,
			"Failed to query RAS caps through RAS_CMD__GET_RAS_CAP, fallback to ras_core public APIs\n");
		core_caps = ras_core_get_ras_caps(ras_mgr->ras_core);

		if (ras_core_poison_supported(ras_mgr->ras_core))
			ext_ecc_type |= BIT(RAS_ECC_TYPE_POISON);
		if (core_caps & BIT_ULL(RAS_BLOCK_ID__UMC))
			ext_ecc_type |= BIT(RAS_ECC_TYPE_MEM);
		if (core_caps & (BIT_ULL(RAS_BLOCK_ID__GFX) | BIT_ULL(RAS_BLOCK_ID__SDMA)))
			ext_ecc_type |= BIT(RAS_ECC_TYPE_SRAM);
		if (ras_psp_flex_mca_enabled(ras_mgr->ras_core))
			int_ecc_attr |= BIT(RAS_ECC_ATTIB_MCAFLEX);
	} else {
		core_caps = rsp.ras_block_mask;
		ext_ecc_type = rsp.ext_ecc_type;
		int_ecc_attr = rsp.int_ecc_attributes;
	}

	if (pf2vf_msg) {
		uniras_caps = &pf2vf_msg->pf2vf_ras_caps.uniras_caps;

		pf2vf_msg->feature_flags.flags.ras_caps = core_caps ? 1 : 0;
		pf2vf_msg->feature_flags.flags.uniras_support = amdgv_uniras_enabled(adapt) ? 1 : 0;

		uniras_caps->ras_en_block_mask = core_caps;
		uniras_caps->ras_ext_ecc_type = ext_ecc_type;
		uniras_caps->ras_int_ecc_attributes = int_ecc_attr;
		RAS_DEV_INFO(adapt,
			"ras_cap: ras_en_block_mask=0x%llx ras_ext_ecc_type=0x%x ras_int_ecc_attributes=0x%x\n",
			(unsigned long long)core_caps, ext_ecc_type, int_ecc_attr);
	}
}
