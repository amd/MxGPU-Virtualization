/*
 * Copyright Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef AMDGV_SRIOV_DRV_H
#define AMDGV_SRIOV_DRV_H

/* SR-IOV Driver Commands */
#define SRIOV_DRV_CMD_INVALID                   0x00000000
#define SRIOV_DRV_CMD_QUERY_VERSION             0x00000001
/* Bit[28:31] 0x01 for GPUIOV Commands*/
#define SRIOV_DRV_CMD_GPUIOV_CMD                0x01000001
#define SRIOV_DRV_CMD_GPUIOV_STATUS             0x01000002
/* Bit[28:31] 0x02 for VF Access Commands*/
#define SRIOV_DRV_CMD_VF_ACCESS                 0x02000001

#define SRIOV_DRV_CMD_SUCCESS               0x00000000
#define SRIOV_DRV_CMD_FAILURE_TIMEOUT       0x00000001
#define SRIOV_DRV_CMD_FAILURE_XCD           0x00000002
#define SRIOV_DRV_CMD_FAILURE_FCN           0x00000003
#define SRIOV_DRV_CMD_FAILURE_TDISP         0x00000004
#define SRIOV_DRV_CMD_FAILURE_RLC           0x00000005
#define SRIOV_DRV_CMD_PENDING_RLC           0x00000006
#define SRIOV_DRV_CMD_FAILURE_GENERIC       0xFFFFFFFF

#define SRIOV_DRV_INTF_VERSION              0x00000001
#define SRIOV_DRV_GPUIOV_CMD_VERSION        0x00000001
#define SRIOV_DRV_VF_ACCESS_CMD_VERSION     0x00000001

/* vf access mode bit[31] set to 1 for MMR full access */
/* vf access mode bit[30] set to 1 for MMR range access */
/* vf access mode bit[29] set to 1 for MMR no access */
/* vf access mode bit[1] for MMR write access,
   only effective for MMR range/no access */
/* vf access mode bit[0] for MMR read access,
   only effective for MMR range/no access */

/* vf access mode bit [28] set to 1 to enable VF DOORBELL */
/* vf access mode bit [27] set to 1 to disable VF DOORBELL */
/* vf access mode bit [26] set to 1 to enable VF FB */
/* vf access mode bit [25] set to 1 to disable VF FB */

#define SRIOV_DRV_VF_MMR_FULL_ACCESS_MASK     1 << 31
#define SRIOV_DRV_VF_MMR_RANGE_ACCESS_MASK    1 << 30
#define SRIOV_DRV_VF_MMR_NO_ACCESS_MASK       1 << 29
#define SRIOV_DRV_VF_DOORBELL_ENABLE_MASK     1 << 28
#define SRIOV_DRV_VF_DOORBELL_DISABLE_MASK    1 << 27
#define SRIOV_DRV_VF_FB_ENABLE_MASK           1 << 26
#define SRIOV_DRV_VF_FB_DISABLE_MASK          1 << 25
#define SRIOV_DRV_VF_MMR_WRITE_ACCESS_MASK    1 << 1
#define SRIOV_DRV_VF_MMR_READ_ACCESS_MASK     1 << 0

#define SRIOV_DRV_GPUIOV_RESP_SIZE          4096
#define SRIOV_DRV_GPUIOV_RESP_ALIGNMENT     4096

#define SRIOV_DRV_GPUIOV_CMD_INVALID                          0x00
#define SRIOV_DRV_GPUIOV_CMD_IDLE_GPU                         0x01
#define SRIOV_DRV_GPUIOV_CMD_SAVE_GPU_STATE                   0x02
#define SRIOV_DRV_GPUIOV_CMD_LOAD_GPU_STATE                   0x03
#define SRIOV_DRV_GPUIOV_CMD_RUN_GPU                          0x04
#define SRIOV_DRV_GPUIOV_CMD_CONTEXT_SWITCH                   0x05
#define SRIOV_DRV_GPUIOV_CMD_ENABLE_AUTO_HW_SWITCH            0x06
#define SRIOV_DRV_GPUIOV_CMD_INIT_GPU                         0x07
#define SRIOV_DRV_GPUIOV_CMD_SAVE_RLCV_STATE                  0x08
#define SRIOV_DRV_GPUIOV_CMD_LOAD_RLCV_STATE                  0x09
#define SRIOV_DRV_GPUIOV_CMD_CLEAR_VF_STATE                   0x0A
#define SRIOV_DRV_GPUIOV_CMD_DISABLE_AUTO_HW_SCHED            0x0B
#define SRIOV_DRV_GPUIOV_CMD_DISABLE_AUTO_HW_SCHED_AND_SWITCH 0x0C
#define SRIOV_DRV_GPUIOV_CMD_SHUTDOWN_GPU                     0x0D
#define SRIOV_DRV_GPUIOV_CMD_CONFIG_AUTO_HW_SCHED_MODE        0x0F
#define SRIOV_DRV_GPUIOV_CMD_EVENT_NOTIFICATION               0x09
#define SRIOV_DRV_GPUIOV_CMD_TRANSFER_VF_DATA                 0x0A

struct sriov_drv_query_version_resp {
	uint32_t major_version;
	uint32_t minor_version;
	uint32_t reserved[6];
};

struct sriov_drv_vf_access_command {
	uint32_t version;

	union {
		struct {
			uint32_t vf_id;
			uint32_t vf_access_mode;
			uint32_t reserved[14];
		} vf_access_cmd_v1_0;

		uint32_t reserved[16];
	} vf_access_cmd;

	uint32_t reserved[15];
};

struct sriov_drv_vf_access_resp {
	uint32_t version;
	uint32_t size;
	uint32_t result;

	union {
		struct {
			uint32_t vf_access_cmd_status;
			uint32_t reserved[4];
		} vf_access_resp_v1_0;

		uint32_t reserved[5];
	};
};

struct sriov_drv_gpuiov_cmd {
	uint32_t version;

	union {
		struct {
			uint32_t xcd_bitmask;
			uint32_t vf_id;
			uint32_t next_vf_id;
			uint32_t gpuiov_cmd;
			uint32_t resp_buf_addr_hi; /* GPU VA[63:32] of sriov_drv_gpuiov_resp */
			uint32_t resp_buf_addr_lo; /* GPU VA[31:0] of sriov_drv_gpuiov_resp */
			uint32_t resp_buf_size;    /* size of sriov_drv_gpuiov_resp */
			uint32_t mmsch_bitmask;    /* Add MMSCH support */
			uint32_t reserved[8];
		} gpuiov_cmd_v1_0;

		uint32_t reserved[16];
	} gpuiov_cmd;

	uint32_t reserved[15];
};

struct sriov_drv_gpuiov_query_status {
	uint32_t version;

	union {
		struct {
			uint32_t xcd_bitmask;
			uint32_t resp_buf_addr_hi; /* GPU VA[63:32] of sriov_drv_gpuiov_resp */
			uint32_t resp_buf_addr_lo; /* GPU VA[31:0] of sriov_drv_gpuiov_resp */
			uint32_t resp_buf_size;    /* size of sriov_drv_gpuiov_resp */
			uint32_t mmsch_bitmask;    /* Add MMSCH support */
			uint32_t reserved[11];
		} gpuiov_query_status_v1_0;

		uint32_t reserved[16];
	} gpuiov_query_status;

	uint32_t reserved[15];
};

struct sriov_drv_gpuiov_resp {
	uint32_t version;
	uint32_t size;
	uint32_t result; // sriov drv command status

	union {
		struct {
			uint32_t num_xcd;
			uint32_t num_mmsch;
			uint32_t padding[5];
			/* +28 start of dynamic size gpuiov response */
			/* num_xcd/num_mmsch * uint32_t per instance sriov drv cmd status */
			/* num_xcd/num_mmsch * uint32_t iov status register readback */
			uint32_t gpuiov_resp[1];
		} gpuiov_resp_v1_0;

		uint32_t reserved[8];
	};
};

struct amdgv_sriov_drv {
	mutex_t gpuiov_cmd_lock;
	mutex_t gpuiov_status_lock;
	struct amdgv_memmgr_mem *sriov_drv_gpuiov_cmd_resp_mem;
	struct amdgv_memmgr_mem *sriov_drv_gpuiov_status_resp_mem;
	bool enabled;
};

uint32_t amdgv_sriov_drv_sw_init(struct amdgv_adapter *adapt);
uint32_t amdgv_sriov_drv_sw_fini(struct amdgv_adapter *adapt);
uint32_t amdgv_sriov_drv_hw_init(struct amdgv_adapter *adapt);
uint32_t amdgv_sriov_drv_query_version(struct amdgv_adapter *adapt);
uint32_t amdgv_sriov_drv_vf_access(struct amdgv_adapter *adapt, uint32_t vf_idx, uint32_t access_mode);
uint32_t amdgv_sriov_drv_gpuiov_set_command(struct amdgv_adapter *adapt, uint32_t xcd_bitmask, uint32_t mmsch_bitmask, uint32_t vf_id, uint32_t next_vf_id, uint32_t gpuiov_cmd);
uint32_t amdgv_sriov_drv_gpuiov_query_status(struct amdgv_adapter *adapt, uint32_t xcd_bitmask, uint32_t mmsch_bitmask);

#endif
