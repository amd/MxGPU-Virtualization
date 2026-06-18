/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_XGMI_H
#define AMDGV_XGMI_H

#include "amdgv_list.h"
#include "amdgv_oss_wrapper.h"
#include "ta_ras_if.h"
#include "amdgv_task_barrier.h"

#define AMDGV_MAX_XGMI_HIVE 8
#define AMDGV_XGMI_MAX_CONNECTED_NODES 64
#define AMDGV_XGMI_MAX_NUM_LINKS 64

/* Group IDs for flr_cp_dma_lock */
#define SHARED_EXCLUSION_GROUP_FLR     0
#define SHARED_EXCLUSION_GROUP_CP_DMA  1

enum amdgv_xgmi_link_status {
	AMDGV_XGMI_LINK_STATUS__ACTIVE = 0,
	AMDGV_XGMI_LINK_STATUS__DISABLED = 1,
	AMDGV_XGMI_LINK_STATUS__INACTIVE = 2,
	AMDGV_XGMI_LINK_STATUS__ERROR = 3,
};

enum amdgv_xgmi_psp_topology_status {
	AMDGV_XGMI_PSP_TOPOLOGY_STATUS__PENDING,
	AMDGV_XGMI_PSP_TOPOLOGY_STATUS__FAILED,
	AMDGV_XGMI_PSP_TOPOLOGY_STATUS__DONE,
};

struct amdgv_hive_info {
	uint64_t hive_id;
	uint32_t hive_index;

	struct amdgv_list_head adapt_list;
	struct amdgv_adapter *master_adapt;
	int number_adapters;

	mutex_t mcm_hive_lock;

	spin_lock_t chain_reset_lock;
	bool	in_chain_reset;
	struct task_barrier tb_chain_reset;

	/* one-time used barrier for driver load */
	struct task_barrier tb_drv_init;

	bool bad_hive;

	/**bad_gpu_in_hive
	 * - set when bad gpu detected in hive,
	 * - disallow operations that require all gpu in hive functional
	 **/
	bool bad_gpu_in_hive;
	atomic_t sync_flood;
	atomic_t ecc_recovery;

	/* reference counter for sending psp mb cmd
	 * only for GPU which enables xgmi.set_mb_in_hive */
	atomic_t psp_mb_cmd_ref_cnt;

	struct shared_exclusion_lock flr_cp_dma_lock;
};

struct amdgv_xgmi_psp_link_info {
	uint32_t num_links;
	struct {
		uint64_t dest_node_id;
		uint32_t dest_bdf;
		uint32_t dest_port_id;
	} link[AMDGV_XGMI_MAX_NUM_LINKS]; /* Peer link info */
};

struct amdgv_xgmi_psp_node_info {
	uint64_t node_id;
	uint32_t num_hops;
	uint32_t is_sharing_enabled;
};

struct amdgv_xgmi_psp_topology_info {
	uint32_t num_nodes;
	struct amdgv_xgmi_psp_node_info node[AMDGV_XGMI_MAX_CONNECTED_NODES]; /* Peer node info */
};

struct amdgv_xgmi_funcs {
	void (*err_cnt_init)(struct amdgv_adapter *adapt);
	void (*query_ras_error_count)(struct amdgv_adapter *adapt, void *ras_error_status);
	void (*reset_ras_error_count)(struct amdgv_adapter *adapt);
	int  (*ras_error_inject)(struct amdgv_adapter *adapt,
				 struct ta_ras_trigger_error_input *block_info);
};

struct amdgv_xgmi {
	uint64_t node_id;		/* from psp */
	uint64_t hive_id;		/* from psp */
	uint64_t node_segment_size;	/* from psp */
	uint32_t phy_node_id;		/* physical nodes (0 ~ 7) */
	uint32_t phy_nodes_num;		/* number of nodes (0 ~ 8) */
	struct amdgv_list_head	head;
	struct amdgv_adapter *master_adapt;
	bool connected_to_cpu;
	uint32_t socket_id;
	enum amdgv_xgmi_fb_sharing_mode fb_sharing_mode;

	/* mask indicating fb sharing with other node, bit shift by phy_node_id*/
	uint32_t custom_mode_sharing_mask;

	enum amdgv_xgmi_psp_topology_status topology_status;
	struct amdgv_xgmi_psp_topology_info topology_info;
	struct amdgv_xgmi_psp_link_info link_info;
	bool set_mb_in_hive;

	const struct amdgv_xgmi_funcs	*funcs;

	bool (*is_fb_sharing_allowed)(struct amdgv_adapter *adapt,
		uint32_t src_phy_node_id, uint32_t dest_phy_node_id,
		enum amdgv_xgmi_fb_sharing_mode mode);
	uint32_t (*get_fb_sharing_mode_mask)(struct amdgv_adapter *adapt,
			enum amdgv_xgmi_fb_sharing_mode mode);
	enum amdgv_xgmi_link_status (*get_link_status)(struct amdgv_adapter *adapt,
						       uint32_t phy_link_idx);
	int (*guest_ext_peer_link_ta_cmd_supported)(struct amdgv_adapter *adapt);
};

struct amdgv_pcs_ras_field {
	const char *err_name;
	uint32_t pcs_err_mask;
	uint32_t pcs_err_shift;
};

int amdgv_xgmi_init_hive(struct amdgv_adapter *adapt);
struct amdgv_hive_info *amdgv_get_xgmi_hive(struct amdgv_adapter *adapt);
int amdgv_xgmi_update_topology_with_fb_sharing_mode(struct amdgv_adapter *adapt,
		enum amdgv_xgmi_fb_sharing_mode mode);
bool amdgv_xgmi_is_fb_sharing_allowed(struct amdgv_adapter *adapt,
		uint32_t src_phy_node_id, uint32_t dest_phy_node_id,
		enum amdgv_xgmi_fb_sharing_mode mode);
bool amdgv_xgmi_is_node_in_hive(struct amdgv_hive_info *hive,
		struct amdgv_adapter *dest_adapt);
int amdgv_xgmi_update_topology(struct amdgv_adapter *adapt);
int amdgv_xgmi_parse_topology_info_for_live_info(struct amdgv_adapter *adapt);
void amdgv_xgmi_reflect_topology_info(struct amdgv_adapter *adapt, struct amdgv_hive_info *hive, struct amdgv_xgmi_psp_topology_info *topology_info);
int amdgv_xgmi_get_topology_info(struct amdgv_adapter *adapt);
enum amdgv_live_info_status amdgv_xgmi_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_xgmi *xgmi_info);
enum amdgv_live_info_status amdgv_xgmi_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_xgmi *xgmi_info);
int amdgv_xgmi_add_to_hive(struct amdgv_adapter *adapt);
int amdgv_xgmi_remove_from_hive(struct amdgv_adapter *adapt);
bool amdgv_xgmi_is_hive_bad(struct amdgv_adapter *adapt);
void amdgv_xgmi_mark_hive_bad(struct amdgv_adapter *adapt);
bool amdgv_xgmi_has_bad_gpu_in_hive(struct amdgv_adapter *adapt);
void amdgv_xgmi_mark_bad_gpu_in_hive(struct amdgv_adapter *adapt);
int amdgv_xgmi_flr_fb_sharing_vfs(struct amdgv_adapter *adapt,
	uint32_t idx_vf, struct amdgv_hive_info *hive);
bool amdgv_xgmi_all_nodes_fb_sharing(struct amdgv_adapter *adapt);
bool amdgv_xgmi_node_fb_sharing_allowed(struct amdgv_adapter *adapt);
int amdgv_xgmi_inject_error(struct amdgv_adapter *adapt,
			    struct ta_ras_trigger_error_input *block_info);
enum amdgv_xgmi_link_status amdgv_xgmi_get_link_status(struct amdgv_adapter *adapt,
							      uint32_t phy_link_idx);
bool amdgv_xgmi_is_guest_ext_peer_link_ta_cmd_supported(struct amdgv_adapter *adapt);
#endif
