/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_reset.h"
#include "amdgv_mailbox.h"
#include "amdgv_ecc.h"
#include "amdgv_vfmgr.h"
#include "amdgv_gpuiov.h"

static const uint32_t this_block = AMDGV_SECURITY_BLOCK;

int amdgv_reset_mailbox_notify_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  bool completion)
{
	uint32_t msg_data[MAILBOX_DATA_LEN_2];
	bool need_valid;

	if (completion) {
		msg_data[0] = MB_RES_MSG_FLR_NOTIFICATION_COMPLETION;
		need_valid = false;
	} else {
		msg_data[0] = MB_RES_MSG_FLR_NOTIFICATION;
		if (adapt->array_vf[idx_vf].vf_status == AMDGV_VF_STATUS_START_INIT)
			need_valid = false; // Don't send interrupt during initialization.
		else
			need_valid = true;
	}

	if (idx_vf == AMDGV_PF_IDX) {
		if (adapt->reset.reset_state == false) {
			msg_data[1] = MB_RES_MSG_PF_SOFT_FLR_NOTIFICATION;
		}
		oss_send_msg(adapt->dev, msg_data, 1, need_valid);
	}
	else
		amdgv_mailbox_send_msg(adapt, idx_vf, msg_data, MAILBOX_DATA_LEN_1,
				       need_valid);

	return 0;
}

int amdgv_reset_notify_gpu_rma(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t msg_data[MAILBOX_DATA_LEN_1];

	msg_data[0] = MB_RES_MSG_GPU_RMA;

	// only send to PF at this moment
	if (idx_vf == AMDGV_PF_IDX)
		oss_send_msg(adapt->dev, msg_data, 1, false);

	return 0;
}

int amdgv_reset_mailbox_notify_after_pf(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;

	if (!adapt->reset.reset_notify_vf_pending)
		return 0;

	for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
		if (adapt->array_vf[idx_vf].reset_notify_vf_pending) {
			/* notify vf reset completion */
			amdgv_put_log(idx_vf, AMDGV_LOG_RESET_NOTIFY_VF_COMPLETION, 0);
			amdgv_reset_mailbox_notify_vf(adapt, idx_vf, true);
			adapt->array_vf[idx_vf].reset_notify_vf_pending = false;
		}
	}

	adapt->reset.reset_notify_vf_pending = false;

	return 0;
}

int amdgv_reset_program_vf_mc_settings(struct amdgv_adapter *adapt)
{
	uint32_t idx_vf;
	int ret = 0;

	if (!adapt->reset.reset_notify_vf_pending)
		return 0;

	for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
		if (adapt->array_vf[idx_vf].configured) {
			if (adapt->psp.psp_program_guest_mc_settings) {
				/* program vf mc settings */
				amdgv_put_log(idx_vf, AMDGV_LOG_RESET_PROGRAM_VF_MC_SETTINGS, 0);
				if (adapt->psp.psp_program_guest_mc_settings(adapt, idx_vf)) {
					ret = AMDGV_FAILURE;
				}
			}
			amdgv_vfmgr_copy_ip_data_to_vf(adapt, idx_vf, false);
		}
	}

	return ret;
}

int amdgv_reset_notify_engine_status(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (adapt->reset.funcs->notify_engine_status)
		adapt->reset.funcs->notify_engine_status(adapt, idx_vf);

	return 0;
}

int amdgv_reset_hw_for_reload(struct amdgv_adapter *adapt, bool is_unload)
{
	if (!adapt->reset.funcs || !adapt->reset.funcs->reset_hw_for_reload) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_GPU_FAILED, 0);
		return AMDGV_FAILURE;
	}

	if (is_unload)
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_GPU_UNLOAD, 0);
	else
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_GPU_RELOAD, 0);

	return adapt->reset.funcs->reset_hw_for_reload(adapt, is_unload);
}

int amdgv_reset_gpu_and_reinit(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t idx_vf, i;

	const struct amdgv_gpu_reset_funcs *funcs;
	adapt->reset.reset_num++;
	adapt->reset.reset_state = true;

	if (adapt->mcp.mem_mode_switch_requested)
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_GPU_CONFIG_CHANGE, 0);
	else
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_GPU, 0);

	funcs = adapt->reset.funcs;

	if (adapt->reset.reset_mode != AMDGV_RESET_PF_FLR) {
		if (funcs && funcs->gpu_reset_and_reinit) {
			ret = funcs->gpu_reset_and_reinit(adapt);

			if (ret) {
				amdgv_device_set_status(adapt, AMDGV_STATUS_HW_LOST);
				amdgv_irqmgr_iv_ring_enable(adapt, false);
				amdgv_irqmgr_mbox_enable(adapt, false);
				AMDGV_DEBUG("Disabled interrupts\n");
			}
		} else {
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_GPU_FAILED, 0);
		}
	} else {
		ret = AMDGV_FAILURE;
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_RESET_GPU_FAILED, 0);
	}

	adapt->reset.reset_state = false;
	adapt->reset.in_xgmi_chain_reset = false;
	adapt->ecc.fatal_error = false;

	if (adapt->flags & AMDGV_FLAG_MIDDLE_OF_LIVE_UPDATE)
		adapt->flags &= ~AMDGV_FLAG_MIDDLE_OF_LIVE_UPDATE;

	for (i = 0; i < adapt->gpuiov.num_ctrl_blocks; i++) {
		adapt->gpuiov.ctrl_blocks[i].last_cmd = 0;
		adapt->gpuiov.ctrl_blocks[i].last_status_reg = 0;
	}

	for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
		adapt->array_vf[idx_vf].gpu_init_data_ready = false;
		if (adapt->reset.reset_mode != AMDGV_RESET_PF_FLR)
			adapt->array_vf[idx_vf].vram_lost = true;
	}

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION)
		amdgv_dirtybit_set_vfs_acc_bits(adapt, 0x0);

	return ret;
}

int amdgv_reset_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	int ret = 0, i;
	const struct amdgv_gpu_reset_funcs *funcs;

	amdgv_put_log(idx_vf, AMDGV_LOG_RESET_FLR, idx_vf);

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION) {
		if (amdgv_xgmi_node_fb_sharing_allowed(adapt) && adapt->dirtybit.fb_hash_support) {
			amdgv_dirtybit_reset_vf_hash_state(adapt, idx_vf);
		} else if (!IS_DEDICATED_SDMA_RING_AVAILABLE(adapt)) {
			amdgv_dirtybit_set_vf_acc_bits(adapt, idx_vf, 0xff);
			amdgv_put_log(idx_vf, AMDGV_LOG_RESET_FLR_SET_ACC_BITS_DIRTY, 0);
		} else {
			amdgv_dirtybit_query_vf_fb_dbit(adapt, idx_vf);
		}
	}

	// During GPUV live update bootup, need PF FLR, so ignore whether force reset flag is set
	if ((adapt->flags & AMDGV_FLAG_VF_HANG_GPU_RESET) && !amdgv_in_live_update_seq()) {
		amdgv_put_log(idx_vf, AMDGV_LOG_RESET_FLR_DISABLED, 0);
		return AMDGV_FAILURE;
	}

	amdgv_time_log_increase_vf_reset_cnt(adapt, idx_vf);

	funcs = adapt->reset.funcs;

	amdgv_gfx_rlc_safe_mode(adapt, true);

	if (funcs && funcs->trigger_vf_flr)
		ret = funcs->trigger_vf_flr(adapt, idx_vf);

	amdgv_gfx_rlc_safe_mode(adapt, false);

	adapt->array_vf[idx_vf].gpu_init_data_ready = false;
	/* perf log is reset to disable state after flr */
	for (i = 0; i < adapt->gpuiov.num_ctrl_blocks; i++)
		adapt->sched.perf_log_enabled[i] = false;

	if (adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION)
		amdgv_dirtybit_control(adapt, true);

	if (ret)
		amdgv_put_log(idx_vf, AMDGV_LOG_RESET_FLR_FAILED, idx_vf);

	return ret;
}

void amdgv_reset_save_sriov(struct amdgv_adapter *adapt)
{
	int offset;
	int pos;
	uint32_t *pci_cfg;

	pci_cfg = (uint32_t *)adapt->reset.sriov_cap;
	pos = adapt->sriov_cap_pos;

	/* Save sriov setting */
	for (offset = 0; offset < PCIE_EXT_SRIOV_SIZE; offset += 4) {
		oss_pci_read_config_dword(adapt->dev, pos + offset, pci_cfg);
		pci_cfg++;
	}
}

void amdgv_reset_restore_sriov(struct amdgv_adapter *adapt)
{
	int offset;
	int pos;
	uint32_t *pci_cfg;
	uint16_t *ctrl;

	pci_cfg = (uint32_t *)&adapt->reset.sriov_cap[PCIE_EXT_SRIOV_INITIALVF];
	pos = adapt->sriov_cap_pos;

	/* restore sriov setting */
	for (offset = PCIE_EXT_SRIOV_INITIALVF; offset < PCIE_EXT_SRIOV_SIZE; offset += 4) {
		oss_pci_write_config_dword(adapt->dev, pos + offset, *pci_cfg);
		pci_cfg++;
	}

	ctrl = (uint16_t *)&adapt->reset.sriov_cap[PCIE_EXT_SRIOV_CTRL];

	if (adapt->gpuiov.csa_data) { /* csa_data is set only for specific asics */
		/* CSA must be programmed just before enable sriov on specific asic */
		oss_pci_write_config_dword(adapt->dev, adapt->gpuiov.csa_offset,
					   adapt->gpuiov.csa_data);
	}

	/* enable sriov */
	oss_pci_write_config_word(adapt->dev, pos + PCIE_EXT_SRIOV_CTRL, *ctrl);
	/* according to sriov spec, it needs to wait 100ms for all components to perform
	 * internal initialization */
	oss_msleep(100);

	/* restore VF resizable BAR */
	if (!(adapt->flags & AMDGV_FLAG_USE_PF) && oss_pci_restore_vf_rebar(adapt->dev, 0))
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_VF_RESIZE_BAR_FAIL, 0);
}

void amdgv_reset_restore_interrupt(struct amdgv_adapter *adapt)
{
	uint16_t ctrl;
	int offset;

	if (!(adapt->flags & AMDGV_FLAG_RESTORE_INTERRUPT))
		return;

	offset = oss_pci_find_capability(adapt->dev, PCI_CAP_ID_MSIX) + PCI_MSIX_FLAGS;

	oss_pci_read_config_word(adapt->dev, offset, &ctrl);
	ctrl &= ~PCI_MSIX_FLAGS_ENABLE;
	oss_pci_write_config_word(adapt->dev, offset, ctrl);
	ctrl |= PCI_MSIX_FLAGS_ENABLE;
	oss_pci_write_config_word(adapt->dev, offset, ctrl);

	return;
}
