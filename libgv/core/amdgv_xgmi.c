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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "amdgv_device.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_xgmi.h"
#include "amdgv_psp_gfx_if.h"

static const uint32_t this_block = AMDGV_XGMI_BLOCK;
static struct amdgv_hive_info xgmi_hives[AMDGV_MAX_XGMI_HIVE];
static uint32_t hive_count;

/* hive_lock must be used when performing the following actions:
 * - Initializing a hive
 * - Adding a GPU to a hive
 * - Getting a hive
 * - Removing a GPU from a hive
 * - Destroying a hive
 */


/*
 * An adapter calling this function can end up in 3 scenarios:
 * 1) no matching hive found -> do init hive
 * 2) Matching hive is found & initializing -> wait for init complete
 * 3) Matching hive is found & not initializing -> do nothing
*/
int amdgv_xgmi_init_hive(struct amdgv_adapter *adapt)
{
	int i;
	struct amdgv_hive_info *hive = NULL;

	oss_mutex_lock(adapt->hive_lock);
	for (i = 0; i < hive_count; ++i) {
		hive = &xgmi_hives[i];
		if (hive->hive_id == adapt->xgmi.hive_id) {
			/* Hive already initialized */
			oss_mutex_unlock(adapt->hive_lock);
			return 0;
		}
	}

	if (i >= AMDGV_MAX_XGMI_HIVE) {
		oss_mutex_unlock(adapt->hive_lock);
		AMDGV_ERROR("Cannot initialize any more hives!\n");
		return AMDGV_FAILURE;
	}

	hive = &xgmi_hives[i];

	AMDGV_INIT_LIST_HEAD(&hive->adapt_list);

	hive->mcm_hive_lock = oss_mutex_init();
	if (hive->mcm_hive_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_MUTEX_FAIL, 0);
		goto fail;
	}

	if (!task_barrier_init(&hive->tb_drv_init)) {
		AMDGV_ERROR("task_barrier_init tb_drv_init init failed");
		goto fail;
	}

	if (!task_barrier_init(&hive->tb_chain_reset)) {
		AMDGV_ERROR("task_barrier_init tb_chain_reset init failed");
		goto fail;
	}

	hive->chain_reset_lock = oss_spin_lock_init(0);
	if (hive->chain_reset_lock == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		goto fail;
	}

	hive->hive_index = hive_count++;
	hive->master_adapt = adapt;
	hive->in_chain_reset = false;
	hive->hive_id = adapt->xgmi.hive_id;

	oss_atomic_set(&hive->sync_flood, 0);
	oss_atomic_set(&hive->ecc_recovery, 0);
	oss_atomic_set(&hive->psp_mb_cmd_ref_cnt, 0);

	oss_mutex_unlock(adapt->hive_lock);

	return 0;

fail:
	task_barrier_fini(&hive->tb_chain_reset);
	task_barrier_fini(&hive->tb_drv_init);

	if (hive->mcm_hive_lock)
		oss_mutex_fini(hive->mcm_hive_lock);

	if (hive->chain_reset_lock)
		oss_spin_lock_fini(hive->chain_reset_lock);

	amdgv_list_del(&hive->adapt_list);

	//Zero out the whole structure for future re-use
	oss_memset(hive, 0, sizeof(struct amdgv_hive_info));

	oss_mutex_unlock(adapt->hive_lock);

	return AMDGV_FAILURE;
}

struct amdgv_hive_info *amdgv_get_xgmi_hive(struct amdgv_adapter *adapt)
{
	int i;
	struct amdgv_hive_info *hive = NULL;

	if (!adapt->xgmi.hive_id)
		return NULL;

	oss_mutex_lock(adapt->hive_lock);
	for (i = 0; i < AMDGV_MAX_XGMI_HIVE; ++i) {
		hive = &xgmi_hives[i];
		if (hive->hive_id == adapt->xgmi.hive_id) {
			break;
		}
	}
	oss_mutex_unlock(adapt->hive_lock);

	if (i == AMDGV_MAX_XGMI_HIVE)
		return NULL;

	return hive;
}

enum amdgv_live_info_status amdgv_xgmi_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_xgmi *xgmi_info)
{
	struct amdgv_xgmi *xgmi;
	xgmi = &adapt->xgmi;

	xgmi->node_id = xgmi_info->node_id;
	xgmi->hive_id = xgmi_info->hive_id;
	xgmi->node_segment_size = xgmi_info->node_segment_size;
	xgmi->phy_node_id = xgmi_info->phy_node_id;
	xgmi->phy_nodes_num = xgmi_info->phy_nodes_num;
	xgmi->connected_to_cpu = xgmi_info->connected_to_cpu;
	xgmi->socket_id = xgmi_info->socket_id;
	xgmi->fb_sharing_mode = xgmi_info->fb_sharing_mode;
	xgmi->topology_status = xgmi_info->topology_status;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_xgmi_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_xgmi *xgmi_info)
{
	struct amdgv_xgmi *xgmi;
	xgmi = &adapt->xgmi;

	xgmi_info->node_id = xgmi->node_id;
	xgmi_info->hive_id = xgmi->hive_id;
	xgmi_info->node_segment_size = xgmi->node_segment_size;
	xgmi_info->phy_node_id = xgmi->phy_node_id;
	xgmi_info->phy_nodes_num = xgmi->phy_nodes_num;
	xgmi_info->connected_to_cpu = xgmi->connected_to_cpu;
	xgmi_info->socket_id = xgmi->socket_id;
	xgmi_info->fb_sharing_mode = (uint8_t)xgmi->fb_sharing_mode;
	xgmi_info->topology_status = (uint8_t)xgmi->topology_status;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

static void amdgv_xgmi_parse_custom_mode_sharing_mask(struct amdgv_adapter *adapt, struct amdgv_hive_info *hive)
{
	uint32_t i;
	uint32_t mask = 0;
	struct amdgv_adapter *cur;
	struct amdgv_xgmi_psp_topology_info *topo_info;

	topo_info = &adapt->xgmi.topology_info;
	for (i = 0; i < topo_info->num_nodes; i++) {
		if (topo_info->node[i].is_sharing_enabled) {
			amdgv_list_for_each_entry(cur, &hive->adapt_list,
				struct amdgv_adapter, xgmi.head) {
				if (topo_info->node[i].node_id == cur->xgmi.node_id) {
					mask |= (1 << cur->xgmi.phy_node_id);
					break;
				}
			}
		}
	}

	/* the topology info from PSP doesn't contain the info for the current
	 * node itself, so we need to add the node itself to the mask.
	 */
	mask |= (1 << adapt->xgmi.phy_node_id);
	adapt->xgmi.custom_mode_sharing_mask = mask;
}

/* reflect the topology information for bi-directionality */
void amdgv_xgmi_reflect_topology_info(struct amdgv_adapter *adapt, struct amdgv_hive_info *hive, struct amdgv_xgmi_psp_topology_info *topology_info)
{
	struct amdgv_adapter *cur;
	int i;

	for (i = 0; i < topology_info->num_nodes; i++) {
		if (topology_info->node[i].num_hops) {
			uint64_t src_node_id = adapt->xgmi.node_id;
			uint64_t dst_node_id = topology_info->node[i].node_id;
			uint8_t dst_num_hops = topology_info->node[i].num_hops;

			amdgv_list_for_each_entry(cur, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
				struct amdgv_xgmi_psp_topology_info *mirror_top_info;
				int j;

				if (cur->xgmi.node_id != dst_node_id)
					continue;

				mirror_top_info = &cur->xgmi.topology_info;

				for (j = 0; j < mirror_top_info->num_nodes; j++) {
					if (mirror_top_info->node[j].node_id != src_node_id)
						continue;
					mirror_top_info->node[j].num_hops = dst_num_hops;
					break;
				}
			}
		}
	}
}

int amdgv_xgmi_get_topology_info(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *cur;
	int ret = 0;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive)
		return AMDGV_FAILURE;

	if (adapt->psp.xgmi_context.supports_extended_data) {
		amdgv_list_for_each_entry(cur, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
			ret = amdgv_psp_xgmi_get_topology_info(cur, hive, &cur->xgmi.topology_info);
			if (ret)
				return ret;
		}

		amdgv_list_for_each_entry(cur, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
			amdgv_xgmi_reflect_topology_info(cur, hive, &cur->xgmi.topology_info);
		}
	} else {
		ret = amdgv_psp_xgmi_get_topology_info(adapt, hive, &adapt->xgmi.topology_info);
		if (ret)
			return ret;
	}

	ret = amdgv_psp_xgmi_get_peer_link_info(adapt, hive, &adapt->xgmi.link_info);
	if (ret)
		return ret;

	if (adapt->xgmi.fb_sharing_mode == AMDGV_XGMI_FB_SHARING_MODE_CUSTOM)
		amdgv_xgmi_parse_custom_mode_sharing_mask(adapt, hive);

	return ret;
}

int amdgv_xgmi_parse_topology_info_for_live_info(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *entry;
	int ret = 0;

	ret = amdgv_xgmi_init_hive(adapt);
	if (ret)
		return ret;

	amdgv_xgmi_add_to_hive(adapt);

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive)
		return AMDGV_FAILURE;

	if (oss_atomic_inc_return(&hive->tb_drv_init.thread_count) == adapt->xgmi.phy_nodes_num) {
		/* reset counter after the last adapt is identified */
		while (oss_atomic_dec_return(&hive->tb_drv_init.thread_count) > 0)
			;

		amdgv_list_for_each_entry(entry, &hive->adapt_list,
					  struct amdgv_adapter, xgmi.head) {
			if (entry == adapt)
				continue;

			/* Queue get_topology event for other adapters
			 * the event will be processed after RESUME_LIVE
			 * event is processed.
			 */
			amdgv_sched_queue_event(entry, AMDGV_PF_IDX,
				AMDGV_EVENT_SCHED_GET_TOPOLOGY,
				AMDGV_SCHED_BLOCK_ALL);
		}

		ret = amdgv_xgmi_get_topology_info(adapt);
	}

	return ret;
}

int amdgv_xgmi_update_topology(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *cur;
	int ret = 0;

	if (adapt->xgmi.phy_nodes_num > 1) {

		hive = amdgv_get_xgmi_hive(adapt);
		if (!hive) {
			AMDGV_ERROR("XGMI: node 0x%llx, can not match hive "
				    "0x%llx in the hive list.\n",
				    adapt->xgmi.node_id, adapt->xgmi.hive_id);
			return AMDGV_FAILURE;
		}

		ret = amdgv_psp_xgmi_set_topology_info(adapt, hive);
		if (ret)
			goto failed;
		if (adapt->psp.xgmi_context.supports_extended_data) {
			amdgv_list_for_each_entry(cur, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
				ret = amdgv_psp_xgmi_get_topology_info(cur, hive, &cur->xgmi.topology_info);
				if (ret)
					goto failed;
			}

			amdgv_list_for_each_entry(cur, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
				amdgv_xgmi_reflect_topology_info(cur, hive, &cur->xgmi.topology_info);
			}
		} else {
			ret = amdgv_psp_xgmi_get_topology_info(adapt, hive, &adapt->xgmi.topology_info);
			if (ret)
				goto failed;
		}

		ret = amdgv_psp_xgmi_get_peer_link_info(adapt, hive, &adapt->xgmi.link_info);
		if (ret)
			goto failed;
	}

	adapt->xgmi.topology_status = AMDGV_XGMI_PSP_TOPOLOGY_STATUS__DONE;
	AMDGV_DEBUG("Device successfully updated XGMI Topology.");
	return ret;

failed:
	adapt->xgmi.fb_sharing_mode = AMDGV_XGMI_FB_SHARING_MODE_UNKNOWN;
	adapt->xgmi.topology_status = AMDGV_XGMI_PSP_TOPOLOGY_STATUS__FAILED;
	amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_XGMI_TOPOLOGY_UPDATE_FAILED, 0);
	return ret;
}

int amdgv_xgmi_update_topology_with_fb_sharing_mode(struct amdgv_adapter *adapt,
	enum amdgv_xgmi_fb_sharing_mode mode)
{
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *tmp_adapt;
	int ret = AMDGV_FAILURE;
	bool skip_psp_topo_update = false;

	if (adapt->xgmi.phy_nodes_num > 1) {
		hive = amdgv_get_xgmi_hive(adapt);
		if (!hive) {
			AMDGV_ERROR("XGMI: node 0x%llx, can not match hive "
					"0x%llx in the hive list.\n",
					adapt->xgmi.node_id, adapt->xgmi.hive_id);
			return AMDGV_FAILURE;
		}

		/* if one GPU is under spartial partition senario, disallow change fb_sharing_mode */
		amdgv_list_for_each_entry(tmp_adapt, &hive->adapt_list,
						struct amdgv_adapter, xgmi.head) {
			if (tmp_adapt->num_vf > 1) {
				AMDGV_WARN("XGMI: not allow set fb sharing mode since "
							"node 0x%llx has multiple VF nums: %d\n",
							tmp_adapt->xgmi.node_id,
							tmp_adapt->num_vf);
				return AMDGV_FAILURE;
			}
		}

		adapt->xgmi.fb_sharing_mode = mode;

		if (adapt->xgmi.get_fb_sharing_mode_mask) {
			if (mode == AMDGV_XGMI_FB_SHARING_MODE_CUSTOM &&
				adapt->xgmi.custom_mode_sharing_mask ==
					adapt->xgmi.get_fb_sharing_mode_mask(adapt, mode)) {
				ret = 0;
				skip_psp_topo_update = true;
				AMDGV_INFO("skip psp topology update when entering auto mode equivelant custom mode.\n");
			}
		}
		if (!skip_psp_topo_update)
			ret = amdgv_xgmi_update_topology(adapt);
	}

	return ret;
}

bool amdgv_xgmi_is_fb_sharing_allowed(struct amdgv_adapter *adapt,
	uint32_t src_phy_node_id, uint32_t dest_phy_node_id,
	enum amdgv_xgmi_fb_sharing_mode mode)
{
	if (adapt->xgmi.is_fb_sharing_allowed) {
		return adapt->xgmi.is_fb_sharing_allowed(adapt,
			src_phy_node_id, dest_phy_node_id, mode);
	}

	if (adapt->xgmi.fb_sharing_mode == AMDGV_XGMI_FB_SHARING_MODE_DEFAULT) {
		return true;
	} else {
		AMDGV_ERROR("Unsupported XGMI FB sharing config");
		return false;
	}
}

bool amdgv_xgmi_is_node_in_hive(struct amdgv_hive_info *hive,
		struct amdgv_adapter *dest_adapt)
{
	struct amdgv_adapter *entry, *tmp;

	amdgv_list_for_each_entry_safe(entry, tmp, &hive->adapt_list,
				  struct amdgv_adapter, xgmi.head) {
		if (entry == dest_adapt) {
			return true;
		}
	}

	return false;
}

int amdgv_xgmi_add_to_hive(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive) {
		return 0;
	}

	if (amdgv_xgmi_is_node_in_hive(hive, adapt)) {
		return 0;
	}

	oss_mutex_lock(adapt->hive_lock);

	AMDGV_INIT_LIST_HEAD(&adapt->xgmi.head);
	amdgv_list_add_tail(&adapt->xgmi.head, &hive->adapt_list);
	hive->number_adapters++;

	adapt->xgmi.master_adapt = hive->master_adapt;
	adapt->in_sync_flood = &hive->sync_flood;
	adapt->in_ecc_recovery = &hive->ecc_recovery;
	oss_mutex_unlock(adapt->hive_lock);

	return 0;
}

int amdgv_xgmi_remove_from_hive(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive)
		return 0;

	oss_mutex_lock(adapt->hive_lock);
	amdgv_list_del(&adapt->xgmi.head);
	hive->number_adapters--;

	if (hive->master_adapt == adapt)
		AMDGV_WARN("Master adapter has been removed from the hive!");

	if (!hive->number_adapters) {
		task_barrier_fini(&hive->tb_chain_reset);
		task_barrier_fini(&hive->tb_drv_init);
		oss_mutex_fini(hive->mcm_hive_lock);
		oss_spin_lock_fini(hive->chain_reset_lock);
		amdgv_list_del(&hive->adapt_list);
		oss_memset(hive, 0, sizeof(struct amdgv_hive_info));
		hive_count--;
	}

	adapt->in_sync_flood = &adapt->sync_flood;
	adapt->in_ecc_recovery = &adapt->ecc_recovery;

	oss_mutex_unlock(adapt->hive_lock);

	return 0;
}

bool amdgv_xgmi_is_hive_bad(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;

	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive) {
		return false;
	}

	return hive->bad_hive;
}

void amdgv_xgmi_mark_hive_bad(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive) {
		return;
	}

	hive->bad_hive = true;
}

bool amdgv_xgmi_has_bad_gpu_in_hive(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	if (adapt->ecc.bad_page_detection_mode & (1 << AMDGV_RAS_ECC_FLAG_IGNORE_RMA))
		return false;

	hive = amdgv_get_xgmi_hive(adapt);

	if (!hive)
		return false;

	return (hive->bad_hive || hive->bad_gpu_in_hive);
}

void amdgv_xgmi_mark_bad_gpu_in_hive(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive) {
		return;
	}

	hive->bad_gpu_in_hive = true;
}

int amdgv_xgmi_flr_fb_sharing_vfs(struct amdgv_adapter *adapt,
	uint32_t idx_vf, struct amdgv_hive_info *hive)
{
	struct amdgv_adapter *tmp_adapt;

	amdgv_list_for_each_entry(tmp_adapt, &hive->adapt_list,
				  struct amdgv_adapter, xgmi.head) {
		if (amdgv_xgmi_is_fb_sharing_allowed(adapt,
					adapt->xgmi.phy_node_id,
					tmp_adapt->xgmi.phy_node_id,
					adapt->xgmi.fb_sharing_mode)) {
			if (tmp_adapt->num_vf == 1) {
				amdgv_sched_queue_event(tmp_adapt, idx_vf,
					AMDGV_EVENT_SCHED_FORCE_RESET_VF,
					AMDGV_SCHED_BLOCK_ALL);
			} else {
				AMDGV_WARN("GPU FB Sharing is unexpected in multi VF mode."
							" Unable to determine which VF must be reset.\n");
			}

		}
	}

	return 0;
}

bool amdgv_xgmi_all_nodes_fb_sharing(struct amdgv_adapter *adapt)
{
	struct amdgv_hive_info *hive;
	struct amdgv_adapter *tmp_adapt;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive)
		return true;

	amdgv_list_for_each_entry(tmp_adapt, &hive->adapt_list,
				  struct amdgv_adapter, xgmi.head) {
		if (!amdgv_xgmi_is_fb_sharing_allowed(adapt,
					adapt->xgmi.phy_node_id,
					tmp_adapt->xgmi.phy_node_id,
					adapt->xgmi.fb_sharing_mode))
			return false;
	}

	return true;
}
int amdgv_xgmi_inject_error(struct amdgv_adapter *adapt,
			    struct ta_ras_trigger_error_input *block_info)
{
	if (adapt->xgmi.funcs && adapt->xgmi.funcs->ras_error_inject)
		return adapt->xgmi.funcs->ras_error_inject(adapt, block_info);

	AMDGV_WARN("XGMI Injection not supported\n");

	return AMDGV_FAILURE;
}
