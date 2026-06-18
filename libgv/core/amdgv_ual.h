/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_UAL_H
#define AMDGV_UAL_H

#define AMDGV_MAX_LOCAL_GPUS_UAL_V1 16
#define AMDGV_UAL_MAX_STATIONS_V1 64

#define AMDGV_UAL_CMD_RESP_SIZE 4096

enum amdgv_ual_link_type {
	AMDGV_UAL_NONE = 0,
	AMDGV_UALOE = 1,
	AMDGV_UALINK = 2,
	AMDGV_UALMAX
};

enum amdgv_ual_npa_address_mode {
	AMDGV_UAL_NPA_ADDRESS_MODE_SOURCE_ALIASING = 0,
	AMDGV_UAL_NPA_ADDRESS_MODE_SOURCE_IDENTIFICATION = 1,
	AMDGV_UAL_NPA_ADDRESS_MODE_MAX
};

enum amdgv_ual_accelerator_vpod_state {
	AMDGV_UAL_ACCEL_VPOD_STATE_UNCONFIGURED = 0,/* Accelerator is not configured */
	AMDGV_UAL_ACCEL_VPOD_STATE_CONFIGURED = 1,	/* Accelerator is configured, but not added to a vPod */
	AMDGV_UAL_ACCEL_VPOD_STATE_READY = 2,		/* Accelerator is part of a vPod, but not active (nHT disabled / VF driver not loaded, etc.) */
	AMDGV_UAL_ACCEL_VPOD_STATE_ACTIVE = 3,		/* Accelerator is in a vPod and active */
	AMDGV_UAL_ACCEL_VPOD_STATE_ERROR = 4		/* Accelerator is in error state */
};

/* State value validation (for a raw enum value) */
#define AMDGV_UAL_ACCEL_STATE_IS_VALID(state) \
	((state) >= AMDGV_UAL_ACCEL_VPOD_STATE_UNCONFIGURED && \
	 (state) <= AMDGV_UAL_ACCEL_VPOD_STATE_ERROR)

/* State comparison macros: take struct amdgv_adapter *adapt */
#define AMDGV_UAL_ACCEL_STATE_IS_UNCONFIGURED(adapt) \
	((adapt) && ((adapt)->ual.node_info_v1.accel_state == AMDGV_UAL_ACCEL_VPOD_STATE_UNCONFIGURED))

#define AMDGV_UAL_ACCEL_STATE_IS_CONFIGURED(adapt) \
	((adapt) && ((adapt)->ual.node_info_v1.accel_state == AMDGV_UAL_ACCEL_VPOD_STATE_CONFIGURED))

#define AMDGV_UAL_ACCEL_STATE_IS_READY(adapt) \
	((adapt) && ((adapt)->ual.node_info_v1.accel_state == AMDGV_UAL_ACCEL_VPOD_STATE_READY))

#define AMDGV_UAL_ACCEL_STATE_IS_ACTIVE(adapt) \
	((adapt) && ((adapt)->ual.node_info_v1.accel_state == AMDGV_UAL_ACCEL_VPOD_STATE_ACTIVE))

#define AMDGV_UAL_ACCEL_STATE_IS_ERROR(adapt) \
	((adapt) && ((adapt)->ual.node_info_v1.accel_state == AMDGV_UAL_ACCEL_VPOD_STATE_ERROR))

/* Check if state is operational (not UNCONFIGURED or ERROR) */
#define AMDGV_UAL_ACCEL_STATE_IS_OPERATIONAL(state) \
	((state) >= AMDGV_UAL_ACCEL_VPOD_STATE_CONFIGURED && \
	 (state) < AMDGV_UAL_ACCEL_VPOD_STATE_ERROR)

struct amdgv_ual_node_info_v1 {
	enum amdgv_ual_link_type link_type;
	/* Accelerator ID - Range 0 to 1023 */
	uint32_t accelerator_id;

	uint32_t socket_id;

	/* Physical Pod ID - 128-bit UUID */
	uint8_t ppod_id[16];
	/* Physical Pod Size */
	uint32_t ppod_size;
	/* station bandwidth share? */
	uint32_t bandwidth;
	/* Latency - depending on switch presence and type */
	uint32_t latency;

	/* Local Accelerator IDs */
	uint32_t local_accelerators[AMDGV_MAX_LOCAL_GPUS_UAL_V1];

	/* Accelerator vPoD State */
	enum amdgv_ual_accelerator_vpod_state accel_state;
};

struct amdgv_ual_topology_info_v1 {
	/* Virtual Pod ID - Range 0 to 1023 */
	uint32_t vpod_id;
	uint32_t vpod_size;
	/* Active accelerators bitmap of 1024 bits */
	uint32_t vpod_active_accelerators[32];
	enum amdgv_ual_npa_address_mode addr_mode;
};

enum amdgv_ual_ports_per_station {
    AMDGV_UAL_PPS_1 = 1,				/* 1x 800Gbps */
    AMDGV_UAL_PPS_2 = 2,				/* 2x 400Gbps */
    AMDGV_UAL_PPS_4 = 4					/* 4x 200Gbps */
};

struct amdgv_ual_link_info_v1 {
	/**
	 * Number of valid stations in this configuration
	 * Only lane_en_bitmap[0..num_stations-1] will be processed.
	 */
	uint8_t num_stations;
	/**
	 * Station configuration flags
	 *
	 * Bit [3:0]: PortPerStation (PPS) - 1, 2, or 4
	 * Bit [7:4]: Reserved
	 */
	uint8_t station_flag;
	uint8_t reserved[2];
	/**
	 * Bitmap of enabled lanes for each station
	 * in logical station order.
	 */
	uint8_t lane_en_bitmap[AMDGV_UAL_MAX_STATIONS_V1];
};

struct amdgv_ual {

	uint32_t socket_id;

	enum amdgv_ual_link_type link_type;

	/* [31:16] major version, [15:0] minor version */
	uint32_t intf_ver; /* [31:16] major version, [15:0] minor version */

	union {
		struct amdgv_ual_node_info_v1 node_info_v1;
	};

	union {
		struct amdgv_ual_topology_info_v1 topology_info_v1;
	};

	union {
		struct amdgv_ual_link_info_v1 link_info_v1;
	};

	struct amdgv_memmgr_mem *asp_cmd_resp_mem;
	uint32_t asp_cmd_resp_size;

	struct amdgv_list_head head;
	struct amdgv_adapter *master_adapt; /* Needed? */
	uint32_t num_adapters;

	mutex_t ual_lock;
};

int amdgv_ual_set_accelerator_state(struct amdgv_adapter *adapt, enum amdgv_ual_accelerator_vpod_state new_state);
enum amdgv_ual_accelerator_vpod_state amdgv_ual_get_accelerator_state(struct amdgv_adapter *adapt);
bool amdgv_ual_is_supported(struct amdgv_adapter *adapt);
int amdgv_ual_get_interface_version(struct amdgv_adapter *adapt, uint32_t *version);
int amdgv_ual_get_config(struct amdgv_adapter *adapt, struct amdgv_gpumon_get_config_rsp_ual_v1 *config);
int amdgv_ual_set_ppod_config(struct amdgv_adapter *adapt, struct amdgv_gpumon_set_ppod_config_req_ual_v1 *config);
int amdgv_ual_set_vpod_config(struct amdgv_adapter *adapt, struct amdgv_gpumon_set_vpod_config_req_ual_v1 *config);
int amdgv_ual_set_station_config(struct amdgv_adapter *adapt, struct amdgv_gpumon_set_station_config_req_ual_v1 *config);
int amdgv_ual_pause(struct amdgv_adapter *adapt, bool send_completion);
int amdgv_ual_resume(struct amdgv_adapter *adapt, bool send_completion);
int amdgv_ual_trigger_mode2(struct amdgv_adapter *adapt);
int amdgv_ual_send_completion(struct amdgv_adapter *adapt, uint32_t cmd_id, uint32_t status);


#endif //AMDGV_UAL_H
