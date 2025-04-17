/*
 * Copyright (c) 2017-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_API_H
#define AMDGV_API_H

#include "amdgv_oss.h"
#include "amdgv_sriovmsg.h"

/* macro for log header */
#define LIBGV_INFO_HEADER   "gim info libgv: "
#define LIBGV_WARN_HEADER   "gim warning libgv: "
#define LIBGV_ERR_HEADER    "gim error libgv: "
#define LIBGV_ASSERT_HEADER "gim assert libgv: "
#define LIBGV_DEBUG_HEADER  "gim debug libgv: "
#define LIBGV_PRINT_HEADER  LIBGV_INFO_HEADER
#define LIBGV_EVENT_HEADER  "gim event libgv: "

#define AMDGV_FAILURE -1

#define AMDGV_INVALID_HANDLE NULL

/* Maximum device types that libgv supports */
#define AMDGV_MAX_SUPPORT_DEVICE_TYPE_NUM 32

/* Maximum GPUs which libgv support */
#define AMDGV_MAX_GPU_NUM 32

/* Maximum VF which libgv support */
#define AMDGV_MAX_VF_NUM 31

/* plus one PF slot */
#define AMDGV_MAX_VF_SLOT (AMDGV_MAX_VF_NUM + 1)

/* PF index is always the last slot */
#define AMDGV_PF_IDX AMDGV_MAX_VF_NUM

/* default vf number when no configuration specified */
#define AMDGV_VF_NUM_DEFAULT 4

/* special value to use libgv default memmgr config */
#define AMDGV_USE_DEFAULT_MEMMGR ((uint64_t)0)

/* ATI vendor ID */
#define PCI_VENDOR_ID_ATI 0x1002

/* default PCI ID when not specified*/
#define ANY_ID (~0)

/* Default min size for PF when it is not used. Unit in MB */
#define AMDGV_MIN_PF_SIZE 16

/* FB alignment is 16 MB */
#define AMDGV_FUNCTION_FB_ALIGNMENT 16

/* Maximum partition profile option count */
#define AMDGV_MAX_PARTITION_PROFILE_OPTIONS 16

/* Maximum NUMA nodes supported */
#define AMDGV_MAX_NUMA_NODES 12

/* Default size if PF is used. Unit in MB */
#define AMDGV_DEFAULT_USE_PF_SIZE 256

/* default DFC FW size: 1152 DWs = 64(psp header)+ 1024 + 64(fw signature) */
#define AMDGV_DFC_FW_SIZE    (1152*4)
/* MI300 DFC FW size: 1216 DWs */
#define AMDGV_MI300_DFC_FW_SIZE    0x1300
#define DFC_FW_NUMBER_OF_ENTRIES 9

/* Size of histogram array */
#define AMDGV_HISTOGRAM_SIZE 32

#define AMDGV_GPU_PAGE_SHIFT 12
#define AMDGV_GPU_PAGE_SIZE  (0x1 << AMDGV_GPU_PAGE_SHIFT)

#define AMDGV_GPU_MEM_ROW_SIZE (1024 * 32)

#define AMDGV_MAX_PSP_MB_ERROR_RECORD	10

/* VF data exchange size in KB */
#define AMDGV_VF_DATAEXCHANGE_SIZE 2

#define EEPROM_BYTE_SIZE 0x100000

#define EEPROM_MAX_ECC_PAGE_RECORD 100

#define AMDGV_EVENT_QUEUE_ENTRY_NUM 256
#ifdef WS_RECORD
#define AMDGV_RECORD_QUEUE_ENTRY_NUM 65536
#endif

#define AMDGV_IH_QUEUE_ENTRY_NUM 256

#define AMDGV_ERROR_FILTER_LIST_SIZE_MAX 32

#define AMDGV_VBIOS_FILE_MAX_SIZE      (1024*1024*3)

#define AMDGV_AUTO_SCHED_DEBUG_DUMP_MAX_SIZE	256 /* Max usable FB for pf */

#define AMDGV_ACCELERATOR_PARTITION_MODE_UNKNOWN	0
#define AMDGV_ACCELERATOR_PARTITION_MODE_MAX		9

#define TO_KBYTES(x)	      ((x) >> 10)
#define TO_MBYTES(x)	      ((x) >> 20)
#define TO_256KBYTES(x)	      ((x) >> 18)
#define MBYTES_TO_BYTES(x)    (((uint64_t)x) << 20)
#define KBYTES_TO_BYTES(x)    (((uint64_t)x) << 10)
#define SHIFT_256B_TO_16MB(x) ((x) >> 16)
#define MB_TO_16MB(x)	      ((x) >> 4)
#define TO_256BYTES(x)	      ((x) >> 8)

typedef void *amdgv_dev_t;

struct oss_interface;

struct amdgv_init_device_info {
	/* the PF device handle of shim drv */
	oss_dev_t        dev;
	/* the PF domain */
	uint32_t         domain; // not used
	/* the PF bdf */
	uint32_t         bdf;

	/* PF's vendor and device information */
	uint32_t         vendor_id;
	uint32_t         dev_id;
	uint32_t         rev_id;
	uint32_t         sub_sys_id;
	uint32_t         sub_vnd_id;

	/* the mapped framebuffer pointer */
	void             *fb;
	/* the mapped mmio pointer */
	void             *mmio;
	/* the mapped io mem pointer */
	void             *io_mem;
	/* the mapped doorbell pointer */
	void             *doorbell;
	/* the mapped rom pointer */
	void             *rom;

	/* the physical address of framebuffer */
	uint64_t         fb_pa;
	/* the size of framebuffer */
	uint64_t         fb_size;
	/* the physical address of mmio */
	uint64_t         mmio_pa;
	/* the size of mmio */
	uint64_t         mmio_size;
	/* the physical address of io mem */
	uint64_t         io_mem_pa;
	/* the size of io mem */
	uint64_t         io_mem_size;
	/* the physical address of doorbell */
	uint64_t         doorbell_pa;
	/* the size of doorbell */
	uint64_t         doorbell_size;
	/* the size of rom */
	uint64_t         rom_size;

	/* PCI SRIOV configuration */
	uint16_t         sriov_cap_pos;
	uint16_t         sriov_vf_offset;
	uint16_t         sriov_vf_stride;
	uint16_t         sriov_vf_devid;

	/* xgmi hive lock */
	void             *hive_lock;
	void             *gpumon_hive_lock;
};

/* log level */
enum {
	AMDGV_ERROR_LEVEL  = 0,
	AMDGV_WARN_LEVEL   = 1,
	AMDGV_INFO_LEVEL   = 2,
	AMDGV_DEBUG_LEVEL  = 3,
	AMDGV_DEBUG_LEVEL4 = 4,
};

/* log mask bit map
 * NOTE: log mask is shared between hypervisor driver and LibGV
 */
enum {
	AMDGV_API_BLOCK			= (1 << 0),
	AMDGV_COMMUNICATION_BLOCK	= (1 << 1),
	AMDGV_LIVE_MIGRATION_BLOCK	= (1 << 2),
	AMDGV_GFX_BLOCK			= (1 << 3),
	AMDGV_MANAGEMENT_BLOCK		= (1 << 4),
	AMDGV_MEMORY_BLOCK		= (1 << 5),
	AMDGV_MULTIMEDIA_BLOCK		= (1 << 6),
	AMDGV_POWER_BLOCK		= (1 << 7),
	AMDGV_SCHEDULER_BLOCK		= (1 << 8),
	AMDGV_SECURITY_BLOCK		= (1 << 9),
	AMDGV_XGMI_BLOCK		= (1 << 10),
	AMDGV_MAX_LOG_BLOCK
};
#define AMDGV_ALL_BLOCK		(((AMDGV_MAX_LOG_BLOCK - 1) << 1) - 1)
#ifdef __linux__
// Currently MSGPUV uses bits 26 to 29
_Static_assert(
	(AMDGV_MAX_LOG_BLOCK - 1) <= (1 << 25),
	"Log block overflow. Some logs won't be shown.");
#endif // __linux__

enum amdgv_sched_block {
	AMDGV_SCHED_BLOCK_GFX  = 0x0,
	AMDGV_SCHED_BLOCK_UVD  = 0x1,
	AMDGV_SCHED_BLOCK_VCE  = 0x2,
	AMDGV_SCHED_BLOCK_UVD1 = 0x3,
	AMDGV_SCHED_BLOCK_VCN  = 0x4,
	AMDGV_SCHED_BLOCK_VCN1 = 0x5,
	AMDGV_SCHED_BLOCK_JPEG = 0x6,
	AMDGV_SCHED_BLOCK_MAX  = 0x7,
	AMDGV_SCHED_BLOCK_ALL  = 0xf0,
};

#define AMDGV_NAME_MASK_LOGICAL_GFX     (1 << AMDGV_SCHED_BLOCK_GFX)

#define AMDGV_NAME_MASK_LOGICAL_MM      ((1 << AMDGV_SCHED_BLOCK_UVD) | \
										(1 << AMDGV_SCHED_BLOCK_VCE) | \
										(1 << AMDGV_SCHED_BLOCK_UVD1) | \
										(1 << AMDGV_SCHED_BLOCK_VCN) | \
										(1 << AMDGV_SCHED_BLOCK_VCN1) | \
										(1 << AMDGV_SCHED_BLOCK_JPEG))

#define AMDGV_NAME_MASK_LOGICAL_ALL     (AMDGV_NAME_MASK_LOGICAL_MM | \
										AMDGV_NAME_MASK_LOGICAL_GFX)

#define IS_SCHED_BLOCK_MM(sched_block) ((1 << sched_block) & AMDGV_NAME_MASK_LOGICAL_MM)

#define AMDGV_MAX_NUM_WORLD_SWITCH 32
#define AMDGV_MAX_NUM_HW_SCHED 32

enum amdgv_mm_engine {
	AMDGV_HEVC_ENGINE   = 0,
	AMDGV_VCE_ENGINE    = 1,
	AMDGV_HEVC1_ENGINE  = 2,
	AMDGV_VCN_ENGINE    = 3,
	AMDGV_JPEG_ENGINE   = 4,
	AMDGV_MAX_MM_ENGINE = 5,
};

/*
 * AMDGV_SCHED_UNAVAL:
 *     VF is freed, no HW resource allocated to it
 *     VF requires vf_alloc first to be used
 * AMDGV_SCHED_AVAIL:
 *     VF is reset, need init for new VM
 *     no amdgpu running on it
 * AMDGV_SCHED_ACTIVE:
 *     VF has Gfx driver loaded
 * AMDGV_SCHED_SUSPEND:
 *     VF is removed from active list, can be resumed
 * AMDGV_SCHED_FULLACCESS:
 *     as named
 */
enum amdgv_sched_state {
	AMDGV_SCHED_UNAVAL     = 0,
	AMDGV_SCHED_AVAIL      = 1,
	AMDGV_SCHED_ACTIVE     = 2,
	AMDGV_SCHED_SUSPEND    = 3,
	AMDGV_SCHED_FULLACCESS = 4,
};

enum amdgv_notify_event {
	AMDGV_NOTIFY_EVENT_REQ_GPU_INIT	 = 1,
	AMDGV_NOTIFY_EVENT_REL_GPU_INIT	 = 2,
	AMDGV_NOTIFY_EVENT_REQ_GPU_FINI	 = 3,
	AMDGV_NOTIFY_EVENT_REL_GPU_FINI	 = 4,
	AMDGV_NOTIFY_EVENT_REQ_GPU_RESET = 5,
	AMDGV_NOTIFY_EVENT_MAX
};

#ifndef EXCLUDE_FTRACE
enum amdgv_gpu_threads {
	AMDGV_GPU_THREAD_WORLD_SWITCH_THREADS,
	AMDGV_GPU_THREAD_READ_VBIOS = AMDGV_MAX_NUM_WORLD_SWITCH,
	AMDGV_GPU_THREAD_EVENT,
	AMDGV_GPU_THREAD_ERROR_PROCESS,

	AMDGV_GPU_THREAD_COUNT
};
#endif

/*
DFC FW Layout
|==========================================================|
|PSP HEADER                    | 64 DWORD                  |
|----------------------------------------------------------|
|DFC HEADER                    | 16 DWORD                  |
|     dfc_fw_version           |     1 DWORD               |
|     dfc_fw_total_entries     |     1 DWORD               |
|     dfc_gart_wr_guest_min    |     1 DWORD               |
|     dfc_gart_wr_guest_max    |     1 DWORD               |
|     reserved                 |     12 DWORD              |
|----------------------------------------------------------|
|Payload                       | 64 - 7296 DWORD           |
|    dfc_fw_data[N]            | 112 X N (0-64) DWORD      |
|    signature                 | 64 DWORD for NV           |
|                              | 128 DWORD for MI          |
|==========================================================|
MAX DFC FW Size = 64 + 16 + 64 X 112 + 128 = 7376 DWORD
MIN DFC FW Size = 64 + 16 + 64 = 144 DWORD
*/

#define AMDGV_MAX_DFC_FW_SIZE        0x7340
#define AMDGV_MIN_DFC_FW_SIZE        0x240
#define DFC_FW_MAX_NUMBER_OF_ENTRIES 64

struct dfc_fw_header {
	uint32_t dfc_fw_version;
	uint32_t dfc_fw_total_entries;
	uint32_t reserved[14];
};

struct dfc_fw_white_list {
	uint32_t oldest;
	uint32_t latest;
};

struct dfc_fw_data {
	uint32_t		 dfc_fw_type;
	uint32_t		 verification_enabled;
	uint32_t		 reserved[14];
	struct dfc_fw_white_list white_list[16];
	uint32_t		 black_list[64];
};

struct dfc_fw {
	struct dfc_fw_header header;
	struct dfc_fw_data   data[DFC_FW_MAX_NUMBER_OF_ENTRIES];
};

struct amdgv_dfc_fw {
	uint32_t psp_header[64];
	struct dfc_fw_header header;
	struct dfc_fw_data data[DFC_FW_MAX_NUMBER_OF_ENTRIES];
	uint32_t signature[128];
};

struct amdgv_vf_option {
	uint32_t idx_vf;

	/* fb offset and size are in MB */
	uint32_t fb_offset;
	uint32_t fb_size;

	/* time slice is in microseconds */
	uint32_t gfx_time_slice;

	/*
	 * the divider of the total slice.
	 * the value can be 1, 2, 4, 8, 16, or 32.
	 */
	uint32_t gfx_time_divider;

	/* bandwidth is in MB */
	uint32_t mm_bandwidth[AMDGV_MAX_MM_ENGINE];
};

enum { GPUV_PSP_RING_MAX_NUM = 64 };
enum { GPUV_PSP_KM_CMD_MAX_NUM = 16 }; /*16 CMD Buffers*/

enum gpuv_psp_ring_type {
    GPUV_PSP_RING_TYPE__INVALID = 0,
    /* These values map to the way the PSP kernel identifies the rings */
    GPUV_PSP_RING_TYPE__UM = 1,  /* User mode ring (formerly called RBI) */
    GPUV_PSP_RING_TYPE__KM = 2   /* Kernel mode ring (formerly called GPCOM) */
};

enum gpuv_psp_local_memory_type {
    GPUV_PSP_LOCAL_MEMORY_TYPE__FRAME_BUFFER   = 1,
    GPUV_PSP_LOCAL_MEMORY_TYPE__INVISIBLE_FRAME_BUFFER = 2,
    GPUV_PSP_LOCAL_MEMORY_TYPE__GART_CACHEABLE = 3,
    GPUV_PSP_LOCAL_MEMORY_TYPE__GART_WRITECOMBINE = 4
};

struct gpuv_psp_local_memory {
    uint64_t                      size;        // in bytes
    uint32_t                      alignment;   // in bytes
    uint64_t                      mc_address;
    enum gpuv_psp_local_memory_type    type;
    void                         *virtual_address;
    void                         *mem_handle;
};

struct gpuv_psp_desc {
    void       *cmd_handle;
};

struct gpuv_psp_ring {
    enum gpuv_psp_ring_type       ring_type;
    struct gpuv_psp_local_memory  ring_mem;
    struct gpuv_psp_desc          cmd_desc[GPUV_PSP_RING_MAX_NUM];
    uint32_t                 ring_rptr;
    uint32_t                 ring_wptr;
};

struct gpuv_psp_cmd_km_buf {
    struct gpuv_psp_local_memory  cmd_mem;      /* CMD buffer allocation handles */
    uint32_t                 fence_value;  /* Fence associated with CMD submission */
    bool                     used;         /* CMD buffer usage flag */
};

struct gpuv_psp_cmd_km_context {
    struct gpuv_psp_cmd_km_buf        km_cmd_buf_pool[GPUV_PSP_KM_CMD_MAX_NUM];
    struct gpuv_psp_local_memory      km_fence_mem_handle;
    uint32_t                     next_avail_cmd_buf_index;
    uint32_t                     km_fence_count;
};

struct gpuv_psp_data {
	struct gpuv_psp_ring		  km_ring;
	struct gpuv_psp_cmd_km_context km_cmd_context;
};

struct gpuv_engine_queue_data {
	uint64_t fence_memory_mc_addr;
	volatile uint32_t *fence_memory_cpu_addr;
};

/* the flag indicates PF can consume GPU */
#define AMDGV_FLAG_USE_PF              (1 << 0)

/* it indicates libgv shouldn't parse IH block */
#define AMDGV_FLAG_DISABLE_PARSE_IH    (1 << 1)

/* allocate iv ring in system memory */
#define AMDGV_FLAG_ALLOC_IV_RING_IN_SYSMEM  (1 << 2)

/* the flag indicates libgv supports event guard */
#define AMDGV_FLAG_SENSITIVE_EVENT_GUARD  (1 << 3)

/* hang debug mode to control VF FLR*/
/* !!!deprecated!!! don't use it in the future */
#define AMDGV_FLAG_HANG_DEBUG  (1 << 4)

/*
 * The flag disable self-switch when there is only one VF available and
 * scheduler is configured to manual world switch.
 */
#define AMDGV_FLAG_DISABLE_SELF_SWITCH (1 << 5)

/* the flag indicates if libgv does whole gpu reset when vf hang detected */
#define AMDGV_FLAG_VF_HANG_GPU_RESET (1 << 6)

/*
 * This flag is used to decide whether to use legacy VF FLR sequence.
 * The new sequence only works with RLCV 92 onwards.
 */
#define AMDGV_FLAG_USE_LEGACY_FLR_SEQUENCE (1 << 8)

/* This flag indicates whether to clear VF FB region */
#define AMDGV_FLAG_ENABLE_CLEAR_VF_FB (1 << 9)

/* This flag is used to enable ECC support.*/
#define AMDGV_FLAG_ENABLE_ECC (1 << 10)

/* This flag is used to represent whether SMU support pp one vf mode */
#define AMDGV_FLAG_SMU_NO_SUPPORT_PP_ONE_VF (1 << 12)

/* This flag is used to represent in skip check bad gpu is enabled */
#define AMDGV_FLAG_SKIP_CHECK_BGPU (1 << 14)

/* This flag is used to represent to enable all vf access */
#define AMDGV_FLAG_DISABLE_MMIO_PROTECTION (1 << 15)

/* This flag is used to disable CP DMA */
#define AMDGV_FLAG_DISABLE_CP_DMA (1 << 16)

/* This flag is used to disable system(AGP) aperture usage*/
#define AMDGV_FLAG_DISABLE_SYS_APERTURE (1 << 17)

/* This flag is used to control hang debug on whole GPU reset */
/* !!!deprecated!!! don't use it in the future */
#define AMDGV_FLAG_HANG_DEBUG_2  (1 << 18)

/* This flag is used to indicate guest to use PSP for programming IH_RB_CNTL */
#define AMDGV_FLAG_IH_REG_PSP_EN (1 << 19)

/* This flag is used to indicate guest to use RLC for programming GC */
#define AMDGV_FLAG_GC_REG_RLC_EN (1 << 20)

/* This flag is used to indicate guest to use RLC for programming MMHUB */
#define AMDGV_FLAG_MMHUB_REG_RLC_EN (1 << 21)

/* This flag is used to disable PSP VF Gate */
#define AMDGV_FLAG_DISABLE_PSP_VF_GATE (1 << 22)

/* This flag is used to enable/disable world switch record debug feature */
#define AMDGV_FLAG_WS_RECORD (1 << 23)

/* This flag is used to whether we need to restore MSI-X interrupt */
#define AMDGV_FLAG_RESTORE_INTERRUPT (1 << 24)

/* This flag is used to enable BACO power saving */
#define AMDGV_FLAG_IPS_POWER_SAVING (1 << 25)

/* This flag is used to block inactive vf fb access */
#define AMDGV_FLAG_VF_FB_PROTECTION (1 << 26)

/* This flag is used to disable all vf dcore debug */
#define AMDGV_FLAG_DISABLE_DCORE_DEBUG (1 << 27)
/* This flag is used to enable simulation mode */
#define AMDGV_FLAG_SIM_MODE (1 << 28)

/* This flag is used to enable emulation mode */
#define AMDGV_FLAG_EMU_MODE (1 << 29)

/* This flag is used to enable GPUV live update related functionality */
#define AMDGV_FLAG_GPUV_LIVE_UPDATE (1 << 30)

/*This flag is used to enable dynamic timeslice for different vf configs*/
#define AMDGV_FLAG_DYNAMIC_TIME_SLICE ((uint64_t)1 << 31)

/* This flag is used to enable perf log in rlcv auto schedule */
#define AMDGV_FLAG_PERF_LOG_ENABLE ((uint64_t)1 << 32)

/* This flag is used to enable debug dump in rlcv auto schedule */
#define AMDGV_FLAG_DEBUG_DUMP_ENABLE ((uint64_t)1 << 33)

/* This flag is used to skip bad page retirement*/
#define AMDGV_FLAG_SKIP_BAD_PAGE_RETIREMENT ((uint64_t)1 << 34)

/* This flag is used to enable the feature of dumping MES info*/
#define AMDGV_FLAG_MES_INFO_DUMP_ENABLE ((uint64_t)1 << 35)

/* This flag is used  to enable partition full access*/
#define AMDGV_FLAG_ENABLE_PARTITION_FULL_ACCESS ((uint64_t)1 << 36)

/* This flag is used to setup hang detection feature enablment*/
#define AMDGV_FLAG_ENABLE_HANG_DETECTION ((uint64_t)1 << 37)
/* This flag is used to enable config space VF FLR */
#define AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY ((uint64_t)1 << 38)

/* This flag is used to enable smu debug mode and control hang debug on whole GPU reset */
/* !!!deprecated!!! don't use it in the future */
#define AMDGV_FLAG_HANG_DEBUG_3 ((uint64_t)1 << 39)

/* This flag indicates whether live update is performed but firmware has not been updated yet */
#define AMDGV_FLAG_MIDDLE_OF_LIVE_UPDATE ((uint64_t)1 << 40)

/* This flag is used to enable frame buffer cleanup on VM shutdown */
#define AMDGV_FLAG_FB_CLEAN_ON_SHUTDOWN ((uint64_t)1 << 41)

/* This flag is used to skip diagnosis data collection */
#define AMDGV_FLAG_SKIP_DIAG_DATA ((uint64_t)1 << 42)

/* it indicates libgv shouldn't enable SMDA */
#define AMDGV_FLAG_DISABLE_SDMA_ENGINE ((uint64_t)1 << 43)

/* it indicates libgv shouldn't enable KIQ/COMPUTE */
#define AMDGV_FLAG_DISABLE_COMPUTE_ENGINE ((uint64_t)1 << 44)

/* it indicates libgv COMPUTE used for paging */
#define AMDGV_FLAG_ENABLE_COMPUTE_PAGING ((uint64_t)1 << 45)

/*This flag is used to enable GPUV live migration related functionality*/
#define AMDGV_FLAG_GPUV_LIVE_MIGRATION ((uint64_t)1 << 46)

#define AMDGV_FLAG_L1_TLB_CNTL_REG_PSP_EN ((uint64_t)1 << 47)

/*
 * AMDGV_SCHED_SOLID_MODE – RLCV will be in charge of VF world switch.
 *   Each VF will get fixed time slice (e.g. 7ms) no matter such VF has
 *   any work load or not. This is the equivalent to fairness scheduling
 *   mode below, but the scheme is performed by RLCV FW. It is also
 *   knowns as auto-scheduling mode.
 * AMDGV_SCHED_LIQUID_MODE – RLCV performs VF world switch. An
 *   idled (no pending render task) VF will not get any time slice;
 *   a VF gets time slice (e.g. 7ms), but within that time slice, there
 *   is no pending task for certain time, this VF will be switch out
 *   before the time slice expired.
 * AMDGV_SCHED_FAIRNESS – use CPU timer to control the world switch.
 *   In this scheme a configured VF will get a persistent percentage time
 *   slice in world switch cycle no matter a VF is initialized or not.
 * AMDGV_SCHED_ROUND_ROBIN – use CPU timer to control the world switch.
 *   In this scheme only the initialized (active) VF – meaning there is
 *   guest GFX driver running on this VF - will get a GPU time slice. The
 *   less VF is initialized; the active VF has better performance.
 */
enum amdgv_sched_mode {
	AMDGV_SCHED_BEGIN,
	/* RLC HW scheduler mode */
	AMDGV_SCHED_SOLID_MODE = 1,
	AMDGV_SCHED_LIQUID_MODE,
	AMDGV_SCHED_MAX_HW_SCHED_MODE = AMDGV_SCHED_LIQUID_MODE,

	/* SW scheduler mode */
	AMDGV_SCHED_FAIRNESS,
	AMDGV_SCHED_ROUND_ROBIN,
	AMDGV_SCHED_HYBRID_LIQUID_MODE,
	AMDGV_SCHED_MAX_SW_SCHED_MODE = AMDGV_SCHED_HYBRID_LIQUID_MODE,

	AMDGV_SCHED_END,
};

enum {
	AMDGV_IP_DISCOVERY_LOAD_TYPE_BEGIN,
	AMDGV_IP_DISCOVERY_LOAD_VIA_PSP	     = 1,
	/* debug option for bring up */
	AMDGV_IP_DISCOVERY_LOAD_DIRECT		 = 2,
	AMDGV_IP_DISCOVERY_LOAD_TYPE_END,
};

enum {
	AMDGV_FW_LOAD_TYPE_BEGIN,
	AMDGV_FW_LOAD_BY_VBIOS	     = 1,
	AMDGV_FW_LOAD_BY_GIM	     = 2,
	AMDGV_FW_LOAD_BY_GIM_PHASE_2 = 3,
	/* debug option for bring up */
	AMDGV_FW_LOAD_DIRECT = 4,
	AMDGV_FW_LOAD_TYPE_END,
};

enum amdgv_debug_mode {
	AMDGV_DEBUG_MODE_DEFAULT              = 0,
	AMDGV_DEBUG_MODE_VF_FLR_HANG          = (1 << 0),
	AMDGV_DEBUG_MODE_WHOLE_GPU_RESET_HANG = (1 << 1),
	AMDGV_DEBUG_MODE_MULTI_VF             = (1 << 2),
	AMDGV_DEBUG_MODE_RAS_SMU              = (1 << 3),
	AMDGV_DEBUG_MODE_CONDITIONAL_HANG     = (1 << 4),
	AMDGV_DEBUG_MODE_HANG                 = AMDGV_DEBUG_MODE_VF_FLR_HANG | AMDGV_DEBUG_MODE_WHOLE_GPU_RESET_HANG,
	AMDGV_DEBUG_MODE_HANG_RAS_SMU         = AMDGV_DEBUG_MODE_HANG | AMDGV_DEBUG_MODE_RAS_SMU,
	AMDGV_DEBUG_MODE_MASK                 = AMDGV_DEBUG_MODE_VF_FLR_HANG | AMDGV_DEBUG_MODE_WHOLE_GPU_RESET_HANG |
						AMDGV_DEBUG_MODE_MULTI_VF | AMDGV_DEBUG_MODE_RAS_SMU |
						AMDGV_DEBUG_MODE_CONDITIONAL_HANG,
};

enum amdgv_reset_mode {
	AMDGV_RESET_BEGIN,
	AMDGV_RESET_MODE1 = 1,
	AMDGV_RESET_MODE2,
	AMDGV_RESET_MODE2_BACO,
	AMDGV_RESET_BACO,
	AMDGV_RESET_PF_FLR,
	AMDGV_RESET_END,
};

enum amdgv_live_update_state {
	AMDGV_LIVE_UPDATE_NONE = 0,
	AMDGV_LIVE_UPDATE_SAVE,
	AMDGV_LIVE_UPDATE_RESTORE,
	AMDGV_LIVE_UPDATE_ULTRALITE,
	AMDGV_LIVE_UPDATE_MAX,
};

enum amdgv_ips_power_saving {
	AMDGV_IPS_POWER_SAVING_DISABLE = 0,
	AMDGV_IPS_POWER_SAVING_AUTO    = 1,
	AMDGV_IPS_POWER_SAVING_MANUAL  = 2,
	AMDGV_IPS_POWER_SAVING_END,
};

enum amdgv_mm_bandwidth_policy {
	AMDGV_MM_ALL_ASSIGNMENT_FOR_ONE_VF = 0,
	AMDGV_MM_ONE_ASSIGNMENT_FOR_ONE_VF  = 1,
	AMDGV_MM_MAX_ASSIGNMENT_FOR_ONE_VF,
};

enum amdgv_xgmi_fb_sharing_mode {
	AMDGV_XGMI_FB_SHARING_MODE_DEFAULT = 0,
	AMDGV_XGMI_FB_SHARING_MODE_1 = 1,
	AMDGV_XGMI_FB_SHARING_MODE_2 = 2,
	AMDGV_XGMI_FB_SHARING_MODE_4 = 4,
	AMDGV_XGMI_FB_SHARING_MODE_8 = 8,
	AMDGV_XGMI_FB_SHARING_MODE_CUSTOM,
	AMDGV_XGMI_FB_SHARING_MODE_UNKNOWN,
	AMDGV_XGMI_FB_SHARING_MODE_NUM
};

enum amdgv_memory_partition_mode {
	AMDGV_MEMORY_PARTITION_MODE_UNKNOWN,
	AMDGV_MEMORY_PARTITION_MODE_NPS1 = 1,
	AMDGV_MEMORY_PARTITION_MODE_NPS2 = 2,
	AMDGV_MEMORY_PARTITION_MODE_NPS3 = 3,
	AMDGV_MEMORY_PARTITION_MODE_NPS4 = 4,
	AMDGV_MEMORY_PARTITION_MODE_NPS6 = 6,
	AMDGV_MEMORY_PARTITION_MODE_NPS8 = 8,
	AMDGV_MEMORY_PARTITION_MODE_MAX
};

enum amdgv_hang_detection_mode {
	AMDGV_HANG_DETECTION_DISABLED = 0,
	AMDGV_HANG_DETECTION_ENABLED = 1,
	AMDGV_HANG_DETECTION_MODE_MAX
};

enum amdgv_asymmetric_fb_mode {
	AMDGV_ASYMMETRIC_FB_DISABLED = 0,
	AMDGV_ASYMMETRIC_FB_ENABLED = 1,
	AMDGV_ASYMMETRIC_FB_MODE_MAX
};

enum amdgv_auto_sched_log_op {
	AMDGV_AUTO_SCHED_PERF_LOG,
	AMDGV_AUTO_SCHED_DEBUG_DUMP,
	AMDGV_AUTO_SCHED_DEBUG_UNKNOWN
};

enum amdgv_bad_page_detection_mode {
	AMDGV_BAD_PAGE_DETECTION_DEFAULT = 0,
	AMDGV_BAD_PAGE_DETECTION_MODE1   = 1,
	AMDGV_BAD_PAGE_DETECTION_MODE2   = 2,
	AMDGV_BAD_PAGE_DETECTION_END,
};

enum amdgv_ras_vf_telemetry_policy {
	AMDGV_RAS_VF_TELEMETRY_LOG_ON_GUEST_LOAD = 0,
	AMDGV_RAS_VF_TELEMETRY_LOG_ON_HOST_LOAD  = 1,
	AMDGV_RAS_VF_TELEMETRY_DISABLE,
	AMDGV_RAS_VF_TELEMETRY_POLICY_COUNT,
};

#define AMDGV_CPER_MAX_ALLOWED_COUNT 0x1000

struct amdgv_init_config_opt {
	/* the amount of vf to be enabled;
	 * if 0, libgv set to default value.
	 */
	uint32_t total_vf_num;

	/* flags to indicate libgv */
	uint64_t flags;

	/* graphic scheduler mode:
	 * 1: HW solid mode
	 * 2: HW liquid mode
	 * 3: SW fairness scheduling mode
	 * 4: SW round robin mode
	 * 5: Hybrid liquid mode
	 */
	uint32_t gfx_sched_mode;

	/* log level */
	int32_t log_level;

	/* bitmask of function block to log */
	uint32_t log_mask;

	/* allowed time for a vf in full access, in units of ms */
	uint32_t allow_time_full_access;

	/* allowed time for completing a gpuiov cmd, in units of ms */
	uint32_t allow_time_cmd_complete;

	/* specify the IP discovery load type:
	 * 1: libgv loads IP discovery table via PSP FW
	 * 2: libgv loads IP discovery table via PSP FW by directly loading files
	 */
	uint32_t ip_discovery_load_type;

	/* specify the fw load type:
	 * 1: VBIOS loads host FWs
	 * 2: libgv loads host FWs
	 * 3: libgv loads host and guest FWs
	 */
	uint32_t fw_load_type;

	/* specify the enablement of performance monitoring:
	 * 0: disable
	 * 1: enable
	 */
	uint32_t perf_mon_enable;

	/* specify the reserved fb size for rlcv auto schedule debug dump in MB */
	uint32_t debug_dump_reserve_size;

	/* To use pf_option, AMDGV_FLAG_USE_PF has
	 * to be specified in flags.
	 */
	struct amdgv_vf_option *pf_option;

	/* allow libgv to reserve some PF's FB for its own use */
	uint64_t libgv_res_fb_offset;
	uint64_t libgv_res_fb_size;

	/* the length of vf_options */
	uint32_t vf_opts_num;

	/* indicates to skip hw operation during init for live update */
	bool skip_hw_init;

	/* indicates live update data imported from file */
	bool import_from_file;

	/* the array of vf_option */
	struct amdgv_vf_option *vf_options;

	/* compatibility table data */
	struct amdgv_dfc_fw *dfc_fw;

	/* framebuffer defragmentation */
	bool is_defrag_fb;

	/* supported partition profile option number */
	uint16_t part_profile_opts_num;

	/* number of partitions in the profile. eg: [1,2,4,8,12] */
	uint32_t part_profile_opts[AMDGV_MAX_PARTITION_PROFILE_OPTIONS];

	/* powersaving mode: disable/auto/manual */
	enum amdgv_ips_power_saving power_saving_mode;

	enum amdgv_mm_bandwidth_policy mm_policy;

	enum amdgv_xgmi_fb_sharing_mode fb_sharing_mode;
	uint32_t accelerator_partition_mode;
	enum amdgv_memory_partition_mode memory_partition_mode;
	uint32_t partition_full_access_enable;

	uint32_t bp_debug_mode;
	uint32_t hang_detection_mode;

	uint32_t bad_page_record_threshold;
	bool use_legacy_eeprom_format;

	uint32_t debug_mode;

	bool deferred_full_live_update;
	bool asymmetric_fb_mode;
	enum amdgv_bad_page_detection_mode bad_page_detection_mode;
	enum amdgv_ras_vf_telemetry_policy ras_vf_telemetry_policy;
	uint32_t paging_queue_frame_bytes_size;
	uint32_t paging_queue_frame_number;

	int max_cper_count;
};

struct amdgv_fini_config_opt {
	/* indicates to skip hw operation during fini for live update */
	bool skip_hw_fini;
	bool export_status;
	/* indicates exporting live update data to file */
	bool export_to_file;
};

struct amdgv_init_data {
	struct amdgv_init_device_info info;
	struct amdgv_init_config_opt  opt;
	struct amdgv_fini_config_opt  fini_opt;
	struct oss_dma_mem_info sys_mem_info;
};

struct amdgv_lock_sched_opt {
	/* indicates if lock sched for live_update case */
	bool is_live_update;
};

struct amdgv_reg_range {
	/* the start address of blocking area, in byte. */
	uint32_t offset;
	/* the size of blocking area, in byte */
	uint32_t size;
};

struct amdgv_id_mask_name {
	uint32_t    id;
	uint32_t    sched_mask;
	const char *name;
};

struct amdgv_id_name {
	uint32_t    id;
	const char *name;
};

struct amdgv_miti_table {
	/* the length of *table* array */
	uint32_t length;
	/* the array of register range */
	struct amdgv_reg_range *table;
};

struct amdgv_fb_layout {
	/* total avail framebuffer (in MB) */
	uint32_t total_fb_avail;

	/* the start offset of fb reserved for all VFs (in MB) */
	uint32_t vf_usable_fb_offset;

	/* total usable size for all VFs (in MB) */
	uint32_t total_vf_usable_fb;

	/* the start offset of virtualization reservation area (in MB) */
	uint32_t vres_offset;

	/* the total size of virtualization reservation (in MB) */
	uint32_t vres_size;

	/* MC base address for whole system frame buffer */
	uint64_t system_fb_mc_address;
	/* MC address for this adapter's frame buffer */
	uint64_t fb_mc_address;
	/* CPU physical address for this adapter's frame buffer */
	uint64_t fb_physical_address;
};

enum amdgv_dev_info_type {
	AMDGV_GET_ENABLED_VF_NUM = 1,
	AMDGV_GET_DEBUG_LEVEL,
	AMDGV_GET_RESV_AREA,
	AMDGV_GET_FB_LAYOUT,
	AMDGV_GET_PSP_VBFLASH_SUPPORT,
	AMDGV_GET_XGMI_INFO,
	AMDGV_GET_OAM_IDX,
	AMDGV_GET_COMPUTE_PROFILE
};

/* device infomation that can not be configured by shim */
union amdgv_dev_info {
	/* type AMDGV_GET_ENABLED_VF_NUM */
	struct {
		/* the number of enabled vf (excluding pf) */
		uint32_t num_enabled_vf;
	} vf;

	/* type AMDGV_GET_DEBUG_LEVEL */
	struct {
		uint32_t log_level;
		uint32_t log_mask;
	} log;

	/* type AMDGV_GET_RESV_AREA */
	struct {
		/* the start address of the reservation area */
		uint64_t offset;
		/* the size of the reservation area */
		uint64_t size;
	} map;

	/* type AMDGV_GET_FB_LAYOUT */
	struct amdgv_fb_layout layout;
	bool vbflash_support;

	struct {
		uint64_t node_id;		/* from psp */
		uint64_t hive_id;		/* from psp */
		uint32_t hive_index;
		int number_adapters;
		uint32_t phy_node_id;
	} xgmi_info;

	struct {
		uint32_t oam_idx;
	} oam;

	struct {
		uint32_t min;
		uint32_t max;
	} compute_cap;
};

/* get and set general conf paramters */
enum amdgv_dev_conf_type {
	AMDGV_CONF_LOG_LEVEL_MASK = 1,
	AMDGV_CONF_GUARD_FLAG,
	AMDGV_CONF_FORCE_RESET_FLAG,
	AMDGV_CONF_HANG_DEBUG_FLAG,
	AMDGV_CONF_DISABLE_SELF_SWITCH_FLAG,
	AMDGV_CONF_FULL_ACCESS_TIMEOUT,
	AMDGV_CONF_ENABLE_CLEAR_VF_FB,
	AMDGV_CONF_RESET_GPU,
	AMDGV_CONF_CMD_TIMEOUT,
	AMDGV_CONF_FORCE_SWITCH_VF_FLAG,
	AMDGV_CONF_DISABLE_MMIO_PROTECTION,
	AMDGV_CONF_DISABLE_PSP_VF_GATE,
	AMDGV_CONF_HYBRID_LIQUID_VF_MIN_TS,
#ifdef WS_RECORD
	AMDGV_CONF_WS_RECORD,
#endif
	AMDGV_CONF_DISABLE_DCORE_DEBUG,
	AMDGV_CONF_GPUV_LIVE_UPDATE,
	AMDGV_CONF_PERF_LOG_FLAG,
	AMDGV_CONF_DEBUG_DUMP_FLAG,
	AMDGV_CONF_SKIP_PAGE_RETIREMENT,
	AMDGV_CONF_HANG_DETECTION_THRESHOLD,
	AMDGV_CONF_HANG_DETECTION_DURATION,
	AMDGV_CONF_ASYMMETRIC_TIMESLICE_FLAG,
	AMDGV_CONF_ASYMMETRIC_FB,
	AMDGV_CONF_ERROR_DUMP_STACK_MAX,
	AMDGV_CONF_ERROR_DUMP_STACK_FILTER,
};

union amdgv_dev_conf {
	/* type AMDGV_GET_DEBUG_LEVEL */
	struct {
		uint32_t log_level;
		uint32_t log_mask;
	} log;

	int flag_switch;
	/* generic uint32_t value */
	uint32_t u32val;

	/* full access timeout */
	uint32_t f_timeout;
	/* iov command timeout threashold in ms */
	uint32_t cmd_timeout;
	/* swith to vf idx and stop world switch  */
	uint32_t switch_vf_idx;
	/* set max count of dump stack when put error */
	uint32_t error_dump_stack_max;

	uint32_t error_dump_stack_entry;
	uint32_t error_dump_stack_add;
	/* minimum VF time slice in hybrid liquid mode */
	int hliquid_min_ts;
	struct {
		/* asymmetric mode set vf idx */
		uint32_t vf_idx;
		/* asymmetric mode set vf timeslice */
		uint32_t vf_timeslice;
		/* asymmetric mode set vf fb size */
		uint32_t vf_fb_size;
		/* restore from asymmetric timeslice setup to default */
		bool reset;
		/* perform asymmetric fb defragment */
		bool defragment;
	} asymmetric;
};

enum amdgv_vf_info_type {
	AMDGV_GET_VF_BDF = 1,
	AMDGV_GET_VF_FB,
	AMDGV_GET_VF_SCHED_STATE,
	AMDGV_GET_VF_TIME_LOG,
	AMDGV_GET_VF_FFBM_MAP_LIST,
};

struct amdgv_histogram {
	uint32_t start; // range of the first bucket
	uint32_t interval; // interval between buckets
	uint64_t count[AMDGV_HISTOGRAM_SIZE];
	uint64_t total;
	uint32_t range[AMDGV_HISTOGRAM_SIZE];
};

struct amdgv_time_log {
	/* req_gpu_init to rel_gpu_init */
	uint64_t init_start;
	uint64_t init_end;

	/* req_gpu_fini to rel_gpu_fini */
	uint64_t finish_start;
	uint64_t finish_end;

	/* FLR or gpu reset */
	int	 reset_count;
	uint64_t reset_start;
	uint64_t reset_end;

	/* Hourglass */
	uint64_t active_last_tick;
	uint64_t active_time;

	/*
	 * Active time is calculated using below formula:
	 *
	 * active_time  = cumulative_active_time;
	 * active_time += last_save_end > last_load_start ?
	 *               0 : now - last_load_start
	 *
	 * cumulative_active_time is updated in the world switch function
	 */
	uint64_t last_load_start;
	uint64_t last_save_end;
	uint64_t cumulative_active_time;

	/*
	 * a record of active time at beginning of init_vf
	 * the difference between most recent active time and this value
	 * is the current session active time
	 */
	uint64_t historical_active_time_data;

	uint64_t cumulative_running_time;

	/*
	 * histogram for recording statistical data for the time when GIM
	 * issues IDLE_GPU command until this command is completed
	 */
	struct amdgv_histogram gfx_idle_latency_us;
	/*
	 * histogram for recording statistical data for the time when GIM
	 * issues SAVE_GPU command until this command is completed
	 */
	struct amdgv_histogram gfx_save_latency_us;
	/*
	 * histogram for recording statistical data for the time when GIM
	 * issues LOAD_GPU command until this command is completed
	 */
	struct amdgv_histogram gfx_load_latency_us;
	/*
	 * histogram for recording statistical data for the time when GIM
	 * issues RUN_GPU command until this command is completed
	 */
	struct amdgv_histogram gfx_run_latency_us;
	/*
	 * histogram for recording statistical data for VF active time, which
	 * is time between LOAD_GPU command issued and SAVE_GPU completed
	 */
	struct amdgv_histogram gfx_run_summation_us;
};

// NV32 max range number in runtime = 2 + 2*30(bad pages) + (12VF-1) = 73
// The max range number in bootup = 2 + 30  (+ 12VF-1) = 43, so 128 is big enough
#define FFBM_MAP_ENTRY_MAX_COUNT 128
struct ffbm_map_entry {
	uint64_t gpa;
	uint64_t spa;
	uint64_t size;
};
struct amdgv_vf_ffbm_map_list {
	uint32_t ffbm_enabled;
	uint32_t count;
	struct ffbm_map_entry entry[FFBM_MAP_ENTRY_MAX_COUNT];
};

enum amdgv_dev_status {
	AMDGV_STATUS_INVALID = 0,
	AMDGV_STATUS_SW_INIT,
	AMDGV_STATUS_HW_INIT,

	AMDGV_STATUS_HW_LOST,
	AMDGV_STATUS_HW_RMA,

	AMDGV_STATUS_HW_FINI,
	AMDGV_STATUS_SW_FINI,

	AMDGV_STATUS_HW_HIVE_RMA,
};

union amdgv_vf_info {
	/* type AMDGV_GET_VF_BDF */
	struct {
		uint32_t bdf;
		int32_t domain; // not used
	} id;

	/* type AMDGV_GET_VF_FB */
	struct {
		uint32_t fb_offset;
		uint32_t fb_size;
	} fb;

	/* type AMDGV_GET_VF_SCHED_STATE */
	struct {
		enum amdgv_sched_state state;
	} sched;

	/* type AMDGV_GET_TIME_LOG */
	struct amdgv_time_log time_log;

	// type AMDGV_GET_VF_FFBM_MAP_LIST
	struct amdgv_vf_ffbm_map_list vf_ffbm_map_list;
};

/*
 * AMDGV_GUARD_ALL:
 *     to obtain general guard information
 *     guard enablement, and overflow event count
 *
 * AMDGV_GUARD_EVENT_FLR,
 * AMDGV_GUARD_EVENT_EXCLUSIVE_MOD,
 * AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT,
 * AMDGV_GUARD_EVENT_ALL_INT:
 *     to obtain specific guard information,
 *     event state (normal or overflow), event amount received etc.
 */
enum amdgv_guard_type {
	AMDGV_GUARD_EVENT_FLR		    = 0,
	AMDGV_GUARD_EVENT_EXCLUSIVE_MOD	    = 1,
	AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT = 2,
	AMDGV_GUARD_EVENT_ALL_INT	    = 3,
	AMDGV_GUARD_EVENT_RAS_ERR_COUNT	    = 4,
	AMDGV_GUARD_EVENT_RAS_CPER_DUMP	    = 5,
	AMDGV_GUARD_EVENT_MAX,

	AMDGV_GUARD_ALL,
};

#define AMDGV_GUARD_DISABLED (0)
#define AMDGV_GUARD_ENABLED  (1)

#define AMDGV_GUARD_EVENT_NORMAL   (0)
#define AMDGV_GUARD_EVENT_FULL	   (1)
#define AMDGV_GUARD_EVENT_OVERFLOW (2)

#define AMDGV_GUARD_MAX_FLR		  (5)
#define AMDGV_GUARD_MAX_EXCLUSIVE_MOD	  (10)
#define AMDGV_GUARD_MAX_EXCLUSIVE_TIMEOUT (3)
#define AMDGV_GUARD_MAX_ALL_INT		  (57)
#define AMDGV_GUARD_MAX_RAS_TELEMETRY	  (15)

struct amdgv_guard_info {
	enum amdgv_guard_type type;

	union {
		struct {
			/* 0: disable; 1: enable */
			uint32_t state;
			uint32_t ov_event;
		} general;

		struct {
			/*
			 * 0: the event is normal
			 * 1: the event is full
			 * 2: the event is overflow
			 */
			uint32_t state;
			/* amount of monitor event after enabled */
			uint32_t amount;

			/* threshold of events in the interval(seconds) */
			uint64_t interval;
			uint32_t threshold;
			/* current number of events in the interval*/
			uint32_t active;
		} event;
	} parm;
};

enum amdgv_notification {
	AMDGV_NOTIFICATION_ERROR_RESET_VF = 1,
	AMDGV_NOTIFICATION_ERROR_WHOLE_GPU_RESET,
	AMDGV_NOTIFICATION_FORCED_RESET_VF,
	AMDGV_NOTIFICATION_FORCED_WHOLE_GPU_RESET,
	AMDGV_NOTIFICATION_ERROR_FULL_ACCESS_TIMEOUT,
	AMDGV_NOTIFICATION_ECC_CORR_ERROR,
	AMDGV_NOTIFICATION_ECC_UNCORR_ERROR,
	AMDGV_NOTIFICATION_PSP_MAILBOX_ERROR,
	AMDGV_NOTIFICATION_FW_LOAD_ERROR,
	AMDGV_NOTIFICATION_ECC_DFCORR_ERROR,
	AMDGV_NOTIFICATION_MAX
};

typedef void (*amdgv_notification_handler)(oss_dev_t dev, enum amdgv_notification notif,
					   const char *fmt, va_list args);

/* smi tool query type*/
enum amdgv_smi_query_type {
	AMDGV_SMI_LIBGV_VERSION	   = 0,
	AMDGV_SMI_VBIOS		   = 1,
	AMDGV_SMI_FIRMWARE_VERSION = 2,
	AMDGV_SMI_GPU_PERFORMANCE  = 3,
	AMDGV_SMI_ASIC		   = 4,
	AMDGV_SMI_GET_STATUS	   = 5,
	AMDGV_SMI_VF_STATUS	   = 6,
	AMDGV_SMI_ECC_INFO	   = 7,
	AMDGV_SMI_PSP_MB_ERROR_RECORD = 8,
	AMDGV_SMI_DFC_TABLE	= 9,
	AMDGV_SMI_MAX
};

/* set vf option type */
enum amdgv_set_vf_opt_type {
	AMDGV_SET_VF_FB	      = 1 << 0,
	AMDGV_SET_VF_TIME     = 1 << 1,
	AMDGV_SET_VF_GFX_PART = 1 << 2,

	AMDGV_SET_VF_SMI_FULL_OPT	= (1 << 0) | (1 << 1),
	AMDGV_SET_VF_OLD_SYSFS_FULL_OPT = (1 << 0) | (1 << 2),
};

struct amdgv_firmware_info {
	enum amdgv_firmware_id id;
	uint32_t	       version;
};

/* PSP mailbox status type */
enum amdgv_psp_mb_status_type {
	AMDGV_MB_STATUS_OK = 0,
	AMDGV_MB_STATUS_OBSOLETE_FW = 1,
	AMDGV_MB_STATUS_BAD_SIG = 2,
	AMDGV_MB_STATUS_FWLOAD_FAIL = 3,
	AMDGV_MB_STATUS_ERR_GENERIC = 4
};

struct amdgv_psp_mb_err_record {
	uint64_t timestamp;     // System time in second
	uint32_t vf_idx;
	uint32_t fw_id;         // amdgv_firmware_id
	uint16_t status;        // amdgv_psp_mb_status_type
	uint16_t valid;
	uint64_t reserved;      // for fw version in future
};

enum amdgv_smi_ras_block {
	AMDGV_SMI_RAS_BLOCK__UMC = 0,
	AMDGV_SMI_RAS_BLOCK__SDMA,
	AMDGV_SMI_RAS_BLOCK__GFX,
	AMDGV_SMI_RAS_BLOCK__MMHUB,
	AMDGV_SMI_RAS_BLOCK__ATHUB,
	AMDGV_SMI_RAS_BLOCK__PCIE_BIF,
	AMDGV_SMI_RAS_BLOCK__HDP,
	AMDGV_SMI_RAS_BLOCK__XGMI_WAFL,
	AMDGV_SMI_RAS_BLOCK__DF,
	AMDGV_SMI_RAS_BLOCK__SMN,
	AMDGV_SMI_RAS_BLOCK__SEM,
	AMDGV_SMI_RAS_BLOCK__MP0,
	AMDGV_SMI_RAS_BLOCK__MP1,
	AMDGV_SMI_RAS_BLOCK__FUSE,
	AMDGV_SMI_RAS_BLOCK__MCA,
	AMDGV_SMI_RAS_BLOCK__VCN,
	AMDGV_SMI_RAS_BLOCK__JPEG,
	AMDGV_SMI_RAS_BLOCK__IH,
	AMDGV_SMI_RAS_BLOCK__MPIO,
	AMDGV_SMI_NUM_BLOCK_MAX
};

enum amdgv_smi_ras_error_type {
	AMDGV_SMI_RAS_ERROR__NONE		 = 0,
	AMDGV_SMI_RAS_ERROR__PARITY		 = 1,
	AMDGV_SMI_RAS_ERROR__SINGLE_CORRECTABLE	 = 2,
	AMDGV_SMI_RAS_ERROR__MULTI_UNCORRECTABLE = 4,
	AMDGV_SMI_RAS_ERROR__POISON		 = 8,
};

struct amdgv_smi_ras_error_inject_info {
	/* ras-block. i.e. umc, gfx */
	enum amdgv_smi_ras_block	block_id;
	/* type of error. i.e. single_correctable */
	enum amdgv_smi_ras_error_type	inject_error_type;
	/* mem block. i.e. hbm, sram etc */
	uint32_t			sub_block_index;
	/* explicit address of error */
	uint64_t			address;

	union {
		uint64_t value;
		struct {
			/* method of error injection. i.e persistent, coherent etc */
			uint64_t method : 58;
			/* vf index */
			uint64_t vf_idx : 6;
		};
	};

	uint32_t mask;
};

struct amdgv_smi_cmd_ras_ta_load {
	uint32_t version;
	uint32_t in_data_len;
	uint64_t in_data_addr;
	uint64_t out_ras_session_id;
};

struct amdgv_smi_cmd_ras_ta_unload {
	uint64_t ras_session_id;
};

struct amdgv_smi_ras_badpage {
	unsigned int bp;
	unsigned int size;
	unsigned int flags;
};

struct amdgv_smi_ras_common_if {
	enum amdgv_smi_ras_block block;
	enum amdgv_smi_ras_error_type type;
	uint32_t sub_block_index;
	/* block name */
	char name[32];
};

struct amdgv_smi_ras_query_if {
	struct amdgv_smi_ras_common_if head;
	unsigned long ue_count;
	unsigned long ce_count;
	unsigned long de_count;
};

enum amdgv_smi_ras_eeprom_err_type {
	AMDGV_SMI_RAS_EEPROM_ERR_PLACE_HOLDER,
	AMDGV_SMI_RAS_EEPROM_ERR_RECOVERABLE,
	AMDGV_SMI_RAS_EEPROM_ERR_NON_RECOVERABLE
};

struct amdgv_smi_ras_eeprom_table_record {
	union {
		uint64_t address;
		uint64_t offset;
	};

	uint64_t retired_page;
	uint64_t ts;

	enum amdgv_smi_ras_eeprom_err_type err_type;

	union {
		unsigned char bank;
		unsigned char cu;
	};

	unsigned char mem_channel;
	unsigned char mcumc_id;
};

#define ADMGV_SMI_STR_LEN   (128)
#define AMDGV_SMI_ASIC_NAME (32)

enum amdgv_smi_supported_flags {
	AMDGV_XGMI_FLAG		   = 1 << 0,
	AMDGV_MM_METRICS_FLAG		   = 1 << 1,
	AMDGV_POWER_GFX_VOLTAGE_FLAG	   = 1 << 2,
	AMDGV_POWER_DPM_FLAG		   = 1 << 3,
	AMDGV_MEM_USAGE_FLAG		   = 1 << 4,
	AMDGV_MM2_CLOCK_FLAG		   = 1 << 7
};

struct amdgv_config {
	char name[AMDGV_SMI_ASIC_NAME];
	struct {
		uint32_t max_shader_engines;
		uint32_t max_cu_per_sh;
		uint32_t max_sh_per_se;
		uint32_t max_waves_per_simd;
		uint32_t wave_size;
		uint32_t active_cu_count;
		uint32_t major;
		uint32_t minor;
	} gfx;
	struct {
		uint8_t count[AMDGV_MAX_MM_ENGINE];
	} mm;
	struct {
		uint32_t supported_fields_flags;
	} caps;
};

union amdgv_smi_query_info {
	/* AMDGV_SMI_LIBGV_VERSION */
	struct {
		uint32_t major;
		uint32_t minor;
	} libgv_version;

	/* AMDGV_SMI_VBIOS */
	struct {
		uint32_t version;
		uint8_t	 vbios_pn[ADMGV_SMI_STR_LEN];
		uint8_t	 date[ADMGV_SMI_STR_LEN];
		char vbios_version_string[ADMGV_SMI_STR_LEN];
	} vbios_info;

	/* AMDGV_SMI_FIRMWARE_VERSION */
	struct {
		uint32_t			vf_idx;		// PF or VF fw info to query
		uint8_t				fw_num;
		struct amdgv_firmware_info fw_info[AMDGV_FIRMWARE_ID__MAX];
	} firmware_info;

	/* AMDGV_SMI_GPU_PERFORMANCE */
	struct {
		uint32_t max_mclk;
		uint32_t power_usage;
		uint32_t power_capacity;
		uint32_t temperature;
	} gpu_perf_info;

	/* AMDGV_SMI_ASIC */
	struct {
		struct amdgv_config config;
	} asic_info;

	/* AMDGV_SMI_GET_STATUS */
	struct {
		enum amdgv_dev_status status;
	} status_info;

	/* AMDGV_SMI_VF_STATUS */
	struct {
		uint32_t status[AMDGV_MAX_VF_NUM];
	} vf_status;

	/* AMDGV_SMI_ECC_INFO */
	struct {
		uint32_t enabled;
		uint32_t uncorrectable_error_count;
		uint32_t correctable_error_count;
		uint32_t retired_pages;
	} ecc_info;

	/* AMDGV_SMI_PSP_MB_ERROR_RECORD */
	struct {
		uint32_t psp_mb_err_record_counter;
		struct amdgv_psp_mb_err_record err_record[AMDGV_MAX_PSP_MB_ERROR_RECORD];
	} psp_mb_error_log;

	/* AMDGV_SMI_DFC_FW_TABLE */
	struct dfc_fw dfc_fw;
};

enum amdgv_vf_opt_err {
	AMDGV_ERROR_INVALID_VF_INDEX = 1,
	AMDGV_ERROR_INVALID_FB_OFFSET,
	AMDGV_ERROR_INVALID_FB_SIZE,
	AMDGV_ERROR_INVALID_GFX_PART,
	AMDGV_ERROR_INVALID_TIMESLICE_OPT,
	AMDGV_ERROR_NO_VF_AVAILABLE,
};

struct amdgv_device_ids {
	uint32_t vendor_id;
	uint32_t dev_id;
	uint32_t sub_vendor_id;
	uint32_t sub_dev_id;
	uint32_t rev_id;
};

struct amdgv_fb_regions {
	struct {
		uint64_t size;
		uint64_t offset;
	} csa;

	struct {
		uint64_t size;
		uint64_t offset;
	} tmr;

	struct {
		uint64_t size;
		uint64_t offset;
	} pf_ipd;

	struct {
		uint64_t size;
		uint64_t offset;
	} vf_ipd;

	struct {
		uint64_t size;
		uint64_t offset;
	} pf_dataexchange;

	struct {
		uint64_t size;
		uint64_t offset;
	} vf_dataexchange;
};

enum amdgv_gpuiov_vf_access_area {
	AMDGV_VF_ACCESS_FB	       = (1 << 0),
	AMDGV_VF_ACCESS_DOORBELL       = (1 << 1),
	AMDGV_VF_ACCESS_MMIO_REG_WRITE = (1 << 2),
	AMDGV_VF_ACCESS_ALL	       = 0x7
};

enum amdgv_live_info_status {
	AMDGV_LIVE_INFO_STATUS_SUCCESS = 0,
	AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR = 1,
	AMDGV_LIVE_INFO_STATUS_OP_UNKNOWN = 2,
	AMDGV_LIVE_INFO_STATUS_FEATURE_NOT_SUPPORTED = 3,
	AMDGV_LIVE_INFO_STATUS_SIZE_UNMATCH = 4,
};

/* Hardcoded to be AMDGV_AGP_APERTURE_SIZE for now,
   TODO: dynamically fetch the agp allocated size */
#define AMDGV_MIGRATION_VF_FB_COPY_BLOCK_SIZE	1LL << 24

enum amdgv_migration_data_section {
	/* Migration context,including GPU/VF identifier */
	AMDGV_MIGRATION_CONTENT_CTX = 0,

	/* VF HW STATIC DATA (FW images) */
	AMDGV_MIGRATION_CONTENT_VF_HW_STATIC_DATA = 1,
	/* VF HW CONTEXT (TMR, CSA, etc) */
	AMDGV_MIGRATION_CONTENT_VF_HW_DYNAMIC_DATA = 2,
	/* VF FB DATA */
	AMDGV_MIGRATION_CONTENT_VF_FB_DATA = 3,

	AMDGV_MIGRATION_CONTENT_MAX
};

enum amdgv_migration_export_phase {
	AMDGV_MIGRATION_EXPORT_PHASE1_STATIC_DATA = 1,
	AMDGV_MIGRATION_EXPORT_PHASE2_DYNAMIC_DATA = 2,
};

enum amdgv_migration_import_phase {
	/* Prepare VF on Target VF */
	AMDGV_MIGRATION_IMPORT_PHASE1_PREPARE = 1,
	/* static adata import */
	AMDGV_MIGRATION_IMPORT_PHASE2_STATIC_DATA = 2,
	/* dynamic data import and final step */
	AMDGV_MIGRATION_IMPORT_PHASE3_DYNAMIC_DATA = 3,
};

struct amdgv_query_dirty_bit_data {
	/* the offset of vf fb to be queried by the current dirty bitplane */
	uint64_t         query_fb_offset;
	/* size in bytes of memory range to be queried */
	uint64_t         query_size;
	/* the buffer to store the dbit plane data */
	void             *dbit_plane_data_buffer;
	/* the size of the dbit plane buffer */
	uint64_t         dbit_plane_data_size;
	/* which vf's fb to query */
	uint32_t         idx_vf;
	/* if the dbit is cleared during query – it’s not cleared when set to true*/
	bool dbit_preserve;
};

struct amdgv_gpu_identifier {

	/* PCI device ID of the GPU */
	uint32_t device_id;

	uint32_t libgv_version;
	uint32_t vbios_version;
	/* Get from PSP */
	uint32_t migration_version;
	uint32_t num_fw;
	struct {

		/* from AMDGV_FW_enum */
		uint32_t id;
		uint32_t version;

	} fw[AMDGV_FIRMWARE_ID__MAX - 1];
};

#define AMDGV_MIGRATION_VF_IDENTIFIER_V1_SIZE 256
struct amdgv_vf_identifier {

	/* set to valid value for the asic */
	uint32_t version;

	union {
		struct {
			uint32_t vf_fb_size_mb;
			uint32_t gfx_timeslice_us;
			uint32_t mm_timeslice_us;
			/* bitmask of VCN engine assignement */
			uint32_t vcn_engine_bitmask;
			/* bitmask of JPEG engine assigment */
			uint32_t jpeg_engine_bitmask;
			/* assign to type of partition SPX, DPX, TPX, etc. */
			uint32_t partition_config;
		} v1_0;

		uint32_t reserved[AMDGV_MIGRATION_VF_IDENTIFIER_V1_SIZE];
	};
};

struct amdgv_migration_ctx {
	struct amdgv_gpu_identifier gpu;
	struct amdgv_vf_identifier vf;
};

struct amdgv_bp_info {
	bool is_in_bp;
	int ws_cmd;
	uint32_t hw_sched_id;
	uint32_t idx_vf;
};

struct amdgv_perf_log_info {
	uint32_t vf_num;
	struct {
		uint32_t vf_idx;
		uint64_t time_quanta;
		uint32_t ws_cycle_cnt;
		uint32_t skipped_cycle_cnt;
		uint32_t yield_cnt;
	} vf_perf_log_info[AMDGV_MAX_VF_SLOT];
};

/**
 * amdgv_init - init the whole libgv
 *
 * @funcs: oss_interface pointer
 * @dev_id_array: supported device id array, the invoker should pass in
 *                a 16*WORD fixed array to retrieve the supported device id.
 * @flags: global flags for the whole libgv
 *
 * Init the whole libgv and register os service callback.
 * Called once and before all of other APIs.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_init(struct oss_interface *funcs, uint16_t *dev_id_array, uint32_t flags);

/**
 * amdgv_init_ex - init the whole libgv
 *
 * @funcs: oss_interface pointer
 * @dev_ids: supported sub device ids struct array,
 *           each element includes vendor id, device id, sub vendor id,
 *           sub device id and revision id, the invoker should pass in
 *           a 16 fixed struct array to retrieve the supported device ids.
 * @flags: global flags for the whole libgv
 *
 * Init the whole libgv and register os service callback.
 * Called once and before all of other APIs.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_init_ex(struct oss_interface *funcs, struct amdgv_device_ids *dev_ids,
		  uint32_t flags);

/**
 * amdgv_fini - fini the whole libgv
 *
 * Fini the whole libgv.
 * Called at last and other APIs can't be```
 * called after amdgv_fini.
 */
void amdgv_fini(void);

/**
 * amdgv_get_version - get the version number of libgv
 *
 * @major: major number of libgv version
 * @minor: minor number of libgv version
 */
void amdgv_get_version(int *major, int *minor);

/**
 * amdgv_get_date - get the build date of libgv
 *
 * The date string is generated at compile time
 *
 * @str: string format of date "YYYY-MM-DD:HH:MM:SS"
 */
void amdgv_get_date(char *str);

/**
 * amdgv_get_asic_type - get the build date of libgv. This is the 1st
 * 			function to be called in LibGV from MSGPUV
 *
 * @device_id: device id from pci configuration space
 * @revision_id: revision id from pci configuration space
 *
 * Returns:
 * amdgv asic type for success, CHIP_NOT_SUPPORTED for failure.
 */
uint32_t amdgv_get_asic_type(uint32_t device_id, uint32_t revision_id);

/**
 * amdgv_device_init - device initialization and enable sriov
 *
 * @init_data: amdgv_init_data structure, contains sriov device info
 *             and configuration option.
 *
 * Initialize device and enable SRIOV. If @init_data doesn't
 * pass down VF configuration to libgv, libgv enable sriov with
 * default value.
 * Called for each AMD PF device.
 *
 * Returns:
 * amdgv device handle for success, AMDGV_INVALID_HANDLE for failure.
 */
amdgv_dev_t amdgv_device_init(struct amdgv_init_data *init_data);

/**
 * amdgv_device_fini - finish device and disable sriov
 *
 * @dev: amdgv device handle
 *
 * Free all related resource accompanied with device and disable sriov.
 * Called for each AMD SRIOV-enabled PF device when exit.
 */
void amdgv_device_fini(amdgv_dev_t dev);

/**
 * amdgv_device_fini_ex - finish device and based on fini opt to do hw/sw fini.
 *
 * @dev: amdgv device handle
 * @opt_fini: amdgv_fini_config_opt structure, contains fini configuration option.
 *
 * Free related resource accompanied with device and based on fini opt to do hw/sw fini.
 * Called for each AMD SRIOV-enabled PF device when exit.
 */
void amdgv_device_fini_ex(amdgv_dev_t dev, struct amdgv_fini_config_opt *fini_opt);

/**
 * amdgv_adapt_list_update - update dev list information
 *
 * @dev: amdgv device handle
 * @dev_list: amdgv device list pointer
 *
 * Update device list info after each device initialization.
 * Called for each device initialization is done.
 */
void amdgv_adapt_list_update(amdgv_dev_t dev, amdgv_dev_t *adapt_list);

/**
 * amdgv_get_dev_status - query device status
 *
 * @dev: amdgv device handle
 */
enum amdgv_dev_status amdgv_get_dev_status(amdgv_dev_t dev);

/**
 * amdgv_get_dev_info - get the device related information
 *
 * @dev: amdgv device handle
 * @type: the type of device information to get
 * @info: the device information
 *
 * get the device related information.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_get_dev_info(amdgv_dev_t dev, enum amdgv_dev_info_type type,
		       union amdgv_dev_info *info);

/**
 * amdgv_get_vf_option - get the virtual function configuration option
 *
 * @dev: amdgv device handle
 * @idx_vf: the index of vf
 * @option: the virtual function configuration option
 *
 * get the virtual function related configuration option.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_get_vf_option(amdgv_dev_t dev, uint32_t idx_vf, struct amdgv_vf_option *option);

/**
 * amdgv_get_vf_info - get the virtual function information
 *
 * @dev: amdgv device handle
 * @idx_vf: the index of vf
 * @type: the type of virtual function information to get
 * @info: the virtual function information
 *
 * get the virtual function related information.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_get_vf_info(amdgv_dev_t dev, uint32_t idx_vf, enum amdgv_vf_info_type type,
		      union amdgv_vf_info *info);

/**
 * amdgv_is_customized_vf_mode
 *
 * @dev: amdgv device handle
 *
 * Check if the adapter is set as active VF management mode.
 *
 * Returns:
 * true for yes, false for not.
 */
int amdgv_is_customized_vf_mode(amdgv_dev_t dev);

/**
 * amdgv_allocate_vf - reserve a vf as per customized configration
 *
 * @dev: amdgv device handle
 * @option: vf configuration option, contains framebuffer partition,
 *          gfx time slice and mm bandwidth.
 *
 * Reserve a vf as per customized configration.
 * It removes default vf configuration at the first time invoking.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_allocate_vf(amdgv_dev_t dev, struct amdgv_vf_option *option);

/**
 * amdgv_free_vf - de-commit and remove a VF from world switch cycle
 *
 * @dev: amdgv device handle
 * @idx_vf: the vf to be freed
 *
 * De-commit and remove a VF from world switch cycle.
 * If the last customized vf is freed, libgv reset all vf's configuration
 * to the default value.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_free_vf(amdgv_dev_t dev, uint32_t idx_vf);

/**
 * amdgv_set_vf - configure a VF to specified FB location, GFX time slice,
 *                MM engine bandwidth
 *
 * @dev: amdgv device handle
 * @type: to specify what type of data (time or memory) to set
 * @option: the vf option
 *
 * configure a VF to specified FB location, GFX time slice,
 * MM engine bandwidth.
 *
 * Set VF should distinguish time related config and memory config
 * Since while VF is active, timeslice can be changed, but memory related
 * data cannot be modified
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_set_vf(amdgv_dev_t dev, enum amdgv_set_vf_opt_type type,
		 struct amdgv_vf_option *option);

/**
 * amdgv_flr_vf - reset a vf
 *
 * @dev: amdgv device handle
 * @idx_vf: the vf to be reset
 *
 * reset a vf
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_flr_vf(amdgv_dev_t dev, uint32_t idx_vf);

/**
 * amdgv_force_reset_gpu - reset PF and all VFs
 *
 * @dev: amdgv device handle
 *
 * reset PF and all VFs
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_force_reset_gpu(amdgv_dev_t dev);

/**
 * amdgv_suspend_vf - temporary remove a VF from the world switch cycle
 *
 * @dev: amdgv device handle
 * @idx_vf: the vf to be suspended
 *
 * temporary remove a VF from the world switch cycle
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_suspend_vf(amdgv_dev_t dev, uint32_t idx_vf);

/**
 * amdgv_resume_vf - re-add the VF into world switch cycle
 *
 * @dev: amdgv device handle
 * @idx_vf: the vf to be resumed
 *
 * re-add the VF into world switch cycle
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_resume_vf(amdgv_dev_t dev, uint32_t idx_vf);

/**
 * amdgv_stop_vf - remove the vf from world switch cycle
 *                 and set to available state
 *
 * @dev: amdgv device handle
 * @idx_vf: the vf to be set to available
 *
 * remove the vf from world switch cycle and set to available state
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_stop_vf(amdgv_dev_t dev, uint32_t idx_vf);

/**
 * amdgv_set_vf_number - set the number of VFs on a GPU
 *
 * @dev: amdgv device handle
 * @num_vf: the vf to be resumed
 *
 * set the number of VFs on a GPU, and then the all enabled vf
 * are set to the default value.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_set_vf_number(amdgv_dev_t dev, uint32_t num_vf);

/**
 * amdgv_set_all_vf - set all VFs on a GPU
 *
 * @dev: amdgv device handle
 *
 * configure all vfs with default vf config.
 *
 * setup vf FB location, gfx timeslice and MM bandwidth
 *
 * Returns:
 * 0 for success.
 */
int amdgv_set_all_vf(amdgv_dev_t dev);

/**
 * amdgv_notify_event - notify event from SHIM to LibGV
 *
 * @dev: amdgv device handle
 * @event: the event to process
 *
 * notify LibGV of some event from KMD/AMDGPUV such as start, stop or reset.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_notify_event(amdgv_dev_t dev, enum amdgv_notify_event event);

/**
 * amdgv_get_mitigation_range - retrieve register blocking range
 *
 * @dev: amdgv device handle
 * @table: the array of register blocking table.
 *         if it is NULL, @length receives the length of table array.
 * @length: the length of table array
 *
 * retrieve register blocking range
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_get_mitigation_range(amdgv_dev_t dev, struct amdgv_reg_range *table,
			       uint32_t *length);

/**
 * amdgv_set_guard_config - set the guard configuration for the VF
 *
 * @dev: amdgv device handle
 * @idx_vf: the VF index
 * @info: log mask, block bitmask enablement.
 *
 * set the guard configuration for the VF.
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_set_guard_config(amdgv_dev_t dev, uint32_t idx_vf, struct amdgv_guard_info *info);

/**
 * amdgv_get_guard_info - get the guard information for the VF
 *
 * @dev: amdgv device handle
 * @idx_vf: the VF index
 * @info: an in-out struct to query data,
 *        Must set info->type before this call
 *        see amdgv_guard_type for more details
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_get_guard_info(amdgv_dev_t dev, uint32_t idx_vf, struct amdgv_guard_info *info);

/*
 * amdgv_reset_guard_config - reset guard config to default values
 *
 * @dev:	amdgv device handle
 * @idx_vf:     target VF
 *
 */
int amdgv_reset_guard_config(amdgv_dev_t dev, uint32_t idx_vf);

/**
 * amdgv_print_active_vfs_running_time - prints all active vfs running time.
 *
 * @dev: amdgv device handle
 */
void amdgv_print_active_vfs_running_time(amdgv_dev_t dev);

/**
 * amdgv_set_dev_conf - set device configurations
 *
 * @dev: amdgv device handle
 * @type: configuration types from enum amdgv_dev_conf_type
 * @conf: input value
 *
 * set device configurations
 */
int amdgv_set_dev_conf(amdgv_dev_t dev, enum amdgv_dev_conf_type type,
		       union amdgv_dev_conf *conf);

/**
 * amdgv_get_dev_conf - get device configurations
 *
 * @dev: amdgv device handle
 * @type: configuration types from enum amdgv_dev_conf_type
 * @conf: output value
 *
 * get device configurations
 */
int amdgv_get_dev_conf(amdgv_dev_t dev, enum amdgv_dev_conf_type type,
		       union amdgv_dev_conf *conf);

/**
 * amdgv_get_vf_candidate - get bitmap of active vfs
 *
 * @dev: amdgv device handle
 *
 * get bitmap of active VFs
 *
 * Returns:
 * bitmap of active VFs
 */
unsigned int amdgv_get_vf_candidate(amdgv_dev_t dev);

/**
 * amdgv_idx_to_str - convert PF/VF index to NULL terminated string
 *
 * @idx: index of PF or VF
 *
 * convert PF/VF index to NULL terminated string
 *
 * Returns:
 * NULL terminated string containing PF, VFx or Invalid Index
 */
const char *amdgv_idx_to_str(uint32_t idx);

/**
 * amdgv_register_notification_handler - Registers a callback to receive
 * notifications defined by amdgv_notification from LibGV.
 *
 * @cb: amdgv_notification_handler callback function
 *
 * Registers callback function to receive notifications from LibGV
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_register_notification_handler(amdgv_notification_handler cb);

int amdgv_get_fw_version(amdgv_dev_t dev, union amdgv_smi_query_info *info);

int amdgv_get_asic(amdgv_dev_t dev, union amdgv_smi_query_info *info);

/**
 * amdgv_get_smi_info - provide interface to query data from LibGV
 * for AMDGPUVSMI tool
 *
 * @dev - amdgv device handle
 * @type - type of amdgv_smi_query_type information to get
 * @info - smi query information written by LibGV
 *
 * provide interface to query data from LibGV
 *
 * Returns:
 * 0 for success, errors for failure
 */
int amdgv_get_smi_info(amdgv_dev_t dev, enum amdgv_smi_query_type type,
		       union amdgv_smi_query_info *info);

/**
 * amdgv_vf_read_mmio - Interface for MMIO read for the requests that are intercepted and sent to amdgpuv
 *
 * @dev - amdgv device handle
 * @idx_vf - target VF
 * @buffer - buffer to store MMIO read data
 * @offset - offset of MMIO read request
 * @length - number of bytes requesting to read
 *
 * provide interface for MMIO read for the requests that are intercepted and sent to amdgpuv
 *
 * Returns:
 * 0 for errors, numbers for length of MMIO read data requested
 */
int amdgv_vf_read_mmio(amdgv_dev_t dev, uint32_t idx_vf,
	void *buffer, uint32_t offset, uint32_t length);

/**
 * amdgv_vf_read_mmio - Interface for MMIO write for the requests that are intercepted and sent to amdgpuv.
 *
 * @dev - amdgv device handle
 * @idx_vf - target VF
 * @buffer - buffer to store MMIO write data
 * @offset - offset of MMIO write request
 * @length - number of bytes requesting to write
 *
 * provide interface for MMIO write for the requests that are intercepted and sent to amdgpuv
 *
 * Returns:
 * 0 for errors, numbers for length of MMIO write data requested
 */
int amdgv_vf_write_mmio(amdgv_dev_t dev, uint32_t idx_vf,
	void *buffer, uint32_t offset, uint32_t length);

/**
 * amdgv_set_live_update_state - set live update state
 *
 * @dev: amdgv device handle
 * @state: live update state
 *
 * set the live update state(save or restore)
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_set_live_update_state(amdgv_dev_t dev, enum amdgv_live_update_state state);

/**
 * amdgv_get_live_update_state - get live update state
 *
 * @dev: amdgv device handle
 * @state: returned live update state
 *
 * get the live update state(save or restore)
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_get_live_update_state(amdgv_dev_t dev, enum amdgv_live_update_state *state);

/**
 * amdgv_export_live_info_size - export live GPU information size
 *
 * @dev: amdgv device handle
 * @size: live GPU information size
 *
 * get live GPU information size from libgv
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_export_live_info_size(amdgv_dev_t dev, uint32_t *size);

/**
 * amdgv_export_live_info_data - export live GPU information data by data op
 *
 * @dev: amdgv device handle
 * @data_op: amdgv live info data op code
 * @data: live GPU information data
 * @status: live info op handling status
 *
 * get live GPU information data from libgv
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_export_live_info_data(amdgv_dev_t dev,
				   uint32_t data_op,
				   void *data,
				   enum amdgv_live_info_status *status);

/**
 * amdgv_export_all_live_info_data - export all live GPU information data
 *
 * @dev: amdgv device handle
 * @data: live GPU information data
 *
 * get live GPU information data from libgv
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_export_all_live_info_data(amdgv_dev_t dev, void *data);

/**
 * amdgv_import_live_info_data - import live GPU information data
 *
 * @dev: amdgv device handle
 * @data_op: amdgv live info data op code
 * @data: live GPU information data
 * @status: live info op handling status
 *
 * set live GPU information data to libgv
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_import_live_info_data(amdgv_dev_t dev,
				   uint32_t data_op,
				   void *data,
				   enum amdgv_live_info_status *status);

/**
 * amdgv_lock_sched - lock scheduler
 *
 * @dev: amdgv device handle
 *
 * lock scheduler, suspend all VFs and switch to PF
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_lock_sched(amdgv_dev_t dev);

/**
 * amdgv_unlock_sched - unlock scheduler
 *
 * @dev: amdgv device handle
 *
 * unlock scheduler
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_unlock_sched(amdgv_dev_t dev);

/**
 * amdgv_lock_sched_ex - lock scheduler with options
 *
 * @dev: amdgv device handle
 * @opt: option for locking scheduler
 *
 * lock scheduler, suspend all VFs and switch to PF
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_lock_sched_ex(amdgv_dev_t dev, struct amdgv_lock_sched_opt opt);

/**
 * amdgv_unlock_sched_ex - unlock scheduler with options
 *
 * @dev: amdgv device handle
 * @opt: option for unlocking scheduler
 *
 * unlock scheduler
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_unlock_sched_ex(amdgv_dev_t dev, struct amdgv_lock_sched_opt opt);

/**
 * amdgv_set_time_quanta_option - set time quanta option
 *
 * @dev: amdgv device handle
 * @block_id: schedule block id
 * @opt: the new time quanta option
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_set_time_quanta_option(amdgv_dev_t dev, enum amdgv_sched_block sched_block,
				 uint32_t opt);

/**
 * amdgv_get_time_quanta_option - get time quanta option
 *
 * @dev: amdgv device handle
 * @block_id: schedule block id
 * @opt: the pointer to a quanta option struct
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_get_time_quanta_option(amdgv_dev_t dev, enum amdgv_sched_block sched_block,
				 uint32_t *opt);

/**
 * amdgv_toggle_interrupt - toggle interrupts on or off
 *
 * @dev: amdgv device handle
 * @enable: enable/disable interrupts
 *
 * for interrupt on, re-arm IH ring for VF, enable DISP_TIMER2, and register mailbox interrupt
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_toggle_interrupt(amdgv_dev_t dev, bool enable);

/**
 * amdgv_control_fru_sigout - Turn on error injection
 *
 * @dev: amdgv device handle
 * @passphrase: passphrase
 *
 * Returns:
 * 0 for success, errors for failure.
 */
int amdgv_control_fru_sigout(amdgv_dev_t dev, const uint8_t *passphrase);

/*
 * amdgv_ras_trigger_error - trigger ECC error
 *
 * @dev:    amdgv device handle
 * @data:   the RAS data struct for the error
 *
 * This function will switch to PF for this operation
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_ras_trigger_error(amdgv_dev_t dev, struct amdgv_smi_ras_error_inject_info *data);

/*
 * amdgv_ras_clean_correctable_error_count - reset RAS correctable error
 *
 * @dev:    amdgv device handle
 * @corr:   correctable error count
 *
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_ras_clean_correctable_error_count(amdgv_dev_t dev,  int *corr);

/*
 * amdgv_ras_eeprom_clear - reset RAS eeprom table
 *
 * @dev:    amdgv device handle
 *
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_ras_eeprom_clear(amdgv_dev_t dev);

/*
 * amdgv_ras_get_bad_page_record_count - get RAS bad page record count
 *
 * @dev:    amdgv device handle
 * @bp_cnt: bad page count
 *
 *
 */
void amdgv_ras_get_bad_page_record_count(amdgv_dev_t dev, int *bp_cnt);

/*
 * amdgv_ras_get_bad_page_info - get RAS bad page info
 *
 * @dev:    amdgv device handle
 * @index:  bad page record index
 * @record: bad page record
 *
 */
int amdgv_ras_get_bad_page_info(amdgv_dev_t dev, uint32_t index,
					struct amdgv_smi_ras_eeprom_table_record *record);

/*
 * amdgv_ras_ta_load - load RAS TA
 *
 * @dev:    amdgv device handle
 * @data:   the struct containing RAS TA load address and length
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_ras_ta_load(amdgv_dev_t dev, struct amdgv_smi_cmd_ras_ta_load *data);

/*
 * amdgv_ras_ta_unload - unload RAS TA
 *
 * @dev:    amdgv device handle
 * @data:   struct with RAS session id
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_ras_ta_unload(amdgv_dev_t dev, struct amdgv_smi_cmd_ras_ta_unload *data);

int amdgv_ras_get_ecc_block_info(amdgv_dev_t dev, struct amdgv_smi_ras_query_if *info);

/*
 * amdgv_get_vf2pf_info - Obtain VF2PF message
 *
 * @dev:        amdgv device handle
 * @idx_vf:     target VF
 * @vf2pf_info: ptr to struct
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_get_vf2pf_info(amdgv_dev_t dev, uint32_t idx_vf,
			 struct amd_sriov_msg_vf2pf_info *vf2pf_info);

/*
 * amdgv_get_pf2vf_info - Obtain PF2VF message
 *
 * @dev:        amdgv device handle
 * @idx_vf:     target VF
 * @pf2vf_info: ptr to struct
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_get_pf2vf_info(amdgv_dev_t dev, uint32_t idx_vf,
			 struct amd_sriov_msg_pf2vf_info *pf2vf_info);

/*
 * amdgv_get_fb_regions_info - Obtain framebuffer regions info
 *
 * @dev:        amdgv device handle
 * @idx_vf:     target VF
 * @fb_regions_info: ptr to struct
 *
 * Returns:
 * 0 for success, other value for failure
 *
 */
int amdgv_get_fb_regions_info(amdgv_dev_t dev, uint32_t idx_vf,
			      struct amdgv_fb_regions *fb_regions_info);

/*
 * amdgv_dump_sriov_msg - dump the vf2pf and pf2vf message for review
 *
 * Data is printed to kernel log
 *
 * @dev:        amdgv device handle
 * @idx_vf:     target VF
 *
 */
void amdgv_dump_sriov_msg(amdgv_dev_t dev, uint32_t idx_vf);

/*
 * amdgv_fw_live_update - live update specific firmware
 *
 * @dev:        amdgv device handle
 * @fw_id:      firmware id
 *
 */
int amdgv_fw_live_update(amdgv_dev_t dev, enum amdgv_firmware_id fw_id);

/*
 * amdgv_toggle_mmio_access - enable/disable vf's mmio access
 *
 * @dev:        amdgv device handle
 * @idx_vf:     target VF
 * @vf_access_select: fb, reg, or doorbell access
 * @enable:     enable or disable mmio access
 *
 */
int amdgv_toggle_mmio_access(amdgv_dev_t dev, uint32_t idx_vf,
				uint32_t vf_access_select, bool enable);

/*
 * amdgv_toggle_psp_vf_gate - enable/disable psp vf gate
 *
 * @dev:        amdgv device handle
 * @vf_select:     target VF in bitmap
 * @enable:     enable/disable psp vf gate
 *
 */
int amdgv_toggle_psp_vf_gate(amdgv_dev_t dev, uint32_t vf_select, bool enable);

/*
 * amdgv_get_agp_info - get AGP cpu addr for restoring VFFB from CheckpointOps
 *
 * @dev:	amdgv device handle
 * @addr:	returned AGP CPU address
 *
 */
int amdgv_get_agp_info(amdgv_dev_t dev, void **buf);

/*
 * amdgv_copy_migration_vf_fb - save/restore target VF FB data
 *
 * @dev:    amdgv device handle
 * @idx_vf:	target VF
 * @idx_fb_block: index tracker during save/restore
 * @addr:	AGP's CPU address for storing VF FB
 * @save: 	boolean value to indicate save/restore
 *
 */
int amdgv_copy_migration_vf_fb(amdgv_dev_t dev, uint32_t idx_vf,
	uint32_t idx_fb_block, void *buf, bool to_fb);

/*
 * amdgv_get_migration_ctx - get live migration context
 *
 * @dev:	amdgv device handle
 * @idx_vf:	target VF
 * @ctx:	migration context handle
 *
 */
int amdgv_get_migration_ctx(amdgv_dev_t dev, uint32_t idx_vf,
	struct amdgv_migration_ctx *ctx);

/*
 * amdgv_migration_export - export PSP package for migration
 *
 * @dev:	amdgv device handle
 * @idx_vf:	target VF
 * @buf:	dst address to export pkg data
 * @phase:	enum value to indicate export phases
 *
 */
int amdgv_migration_export(amdgv_dev_t dev, uint32_t idx_vf,
	void *buf, enum amdgv_migration_export_phase phase);

/*
 * amdgv_migration_import - import PSP package for migration
 *
 * @dev:	amdgv device handle
 * @idx_vf:	target VF
 * @buf:	src address to import pkg data
 * @phase:	enum value to indicate import phases
 *
 */
int amdgv_migration_import(amdgv_dev_t dev, uint32_t idx_vf,
	void *buf, enum amdgv_migration_import_phase phase);

/*
 * amdgv_get_migration_data_size - get size for migration data
 *
 * @dev:	amdgv device handle
 * @idx_vf:	target VF
 * @size:	pointer to fetch size value
 * @section:to indicate data sections
 *
 */
int amdgv_get_migration_data_size(amdgv_dev_t dev, uint32_t idx_vf,
	uint64_t *size, enum amdgv_migration_data_section section);

/*
 * amdgv_get_migration_static_package - get live migration static package data
 *
 * @dev:	amdgv device handle
 * @data:	live migration data
 * @size:	live migration data size
 *
 */
int amdgv_get_migration_static_package(amdgv_dev_t dev, void *data, uint64_t *size);

/*
 * amdgv_get_migration_dynamic_package - get live migration dynamic package data
 *
 * @dev:	amdgv device handle
 * @data:	live migration data
 * @size:	live migration data size
 *
 */
int amdgv_get_migration_dynamic_package(amdgv_dev_t dev, void *data, uint64_t *size);

/*
 * amdgv_control_dirtybit – control dirty bit tracking.
 *
 * @dev:	amdgv device handle
 * @enable:	enable/disable dirty bit tracking
 *
 */
int amdgv_control_dirtybit(amdgv_dev_t dev, bool enable);

/*
 * amdgv_query_dirtybit_data – query dirty bit tracking data.
 *
 * @dev:	amdgv device handle
 * @enable:	enable/disable dirty bit tracking
 *
 */
int amdgv_query_dirtybit_data(amdgv_dev_t dev, struct amdgv_query_dirty_bit_data *data);

/*
 * amdgv_read_vbios - read vbios from libgv.
 *
 * @dev:	amdgv device handle
 * @buffer: buffer to read
 * @size:   read size
 *
 */
int amdgv_read_vbios(amdgv_dev_t dev, void *buffer, uint32_t size);

/*
 * amdgv_read_ip_data - read ip data info from libgv.
 *
 * @dev:	amdgv device handle
 * @buffer: buffer to read
 * @size:   read size
 *
 */
int amdgv_read_ip_data(amdgv_dev_t dev, void *buffer, uint32_t size);

/*
 * amdgv_read_psp_data - read psp data info from libgv and convert it to kmd psp data
 *
 * @dev:	amdgv device handle
 * @buffer: buffer to read
 * @size:   read size
 *
 */
int amdgv_read_psp_data(amdgv_dev_t dev, void *buffer, uint32_t size);

/*
 * amdgv_get_engine_queue_data - read enginequeue data info from libgv
 *
 * @dev:	amdgv device handle
 * @engine_id: engine identifier
 * @buffer: buffer to read
 * @size:   read size
 *
 */

enum amdgv_engine_id {
	ENGINE_SDMA = 0,
	ENGINE_COMPUTE,
};
int amdgv_get_engine_queue_data(amdgv_dev_t dev, enum amdgv_engine_id id, void *buffer, uint32_t size);
int amdgv_submit_frame_to_engine(amdgv_dev_t dev, enum amdgv_engine_id, uint8_t *frame_data);

/*
 * amdgv_get_diag_data - get diagnosis data and real data size
 *
 * @dev:	amdgv device handle
 * @bdf:	GPU bus id
 * @data:	diagnosis data
 * @size:	diagnosis data size
 *
 */
int amdgv_get_diag_data(amdgv_dev_t dev, uint32_t bdf, void *data, uint32_t *size);

#ifndef EXCLUDE_FTRACE
/**
 * amdgv_device_exclude_list - Get functions list to exclude from tracing
 *
 * @call_trace_exclude_list: list pointer to update
 * @call_trace_exclude_len: length of the list to update
 *
 * Get the list of functions to exclude from OS level function
 * tracing. This should be called in loop until the next is -1.
 */
void amdgv_device_exclude_list(char **call_trace_exclude_list[],
		uint32_t *call_trace_exclude_len);

/**
 * amdgv_list_gpu_threads - Get the list of tasks associated with GPU
 *
 * @dev: amdgv device handle, NULL for driver list
 * @task_list: task list to update
 * @task_list_len: length of the list to update
 * @bdf: return the adaptar bus id
 *
 * Get the list of threads associated with the specified GPU
 *
 * Returns:
 * number of entries updated. 0 indiactes empty list/no update.
 */
int amdgv_list_gpu_threads(amdgv_dev_t dev, void *task_list[],
		uint32_t task_list_len, uint32_t *bdf);
#endif

/*
 * amdgv_toggle_ih_registration - toggle IH registration with OS.
 *
 * @dev:	amdgv device handle
 * @enable:	register or unregister interrupt
 *
 */
int amdgv_toggle_ih_registration(amdgv_dev_t dev, bool enable);

/*
 * amdgv_toggle_power_saving_external - wrapper for toggle power saving on/off.
 *
 * @dev:	amdgv device handle
 * @enable:	enter/exit power saving
 *
 */
int amdgv_toggle_power_saving_external(amdgv_dev_t dev, bool enable);

/*
 * amdgv_query_power_saving_status_external - wrapper for query power saving current status.
 *
 * @dev:	amdgv device handle
 * @status:	current power saving status: 0 - exit powersaving, 1 - in powersaving
 *
 */
int amdgv_query_power_saving_status_external(amdgv_dev_t dev, uint32_t *status);

/*
 * amdgv_toggle_power_saving - toggle power saving on/off.
 *
 * @dev:	amdgv device handle
 * @enable:	enter/exit power saving
 *
 */
int amdgv_toggle_power_saving(amdgv_dev_t dev, bool enable);

/*
 * amdgv_query_power_saving_status - query power saving current status.
 *
 * @dev:	amdgv device handle
 * @status:	current power saving status: 0 - exit powersaving, 1 - in powersaving
 *
 */
int amdgv_query_power_saving_status(amdgv_dev_t dev, uint32_t *status);

/*
 * amdgv_load_vbflash_bin - load local vbios binary into framebuffer.
 *
 * @dev:	amdgv device handle
 * @buffer: binary content of local vbios file
 * @pos:	current position of filp
 * @count:	amount of dwords read at one time
 *
 */
int amdgv_load_vbflash_bin(amdgv_dev_t dev, char *buffer, uint32_t pos,
		uint32_t count);

/*
 * amdgv_update_spirom - flash vbios to spirom.
 *
 * @dev:	amdgv device handle
 *
 */
int amdgv_update_spirom(amdgv_dev_t dev);

/*
 * amdgv_get_vbflash_status - query vbflash status.
 *
 * @dev:	amdgv device handle
 * @status: current vbflash status: 0 - not started, 1 - in progress, other value - finished
 *
 */
int amdgv_get_vbflash_status(amdgv_dev_t dev, uint32_t *status);

int amdgv_set_rlcv_timestamp_dump(amdgv_dev_t dev, uint64_t enable);
/*
 * amdgv_ffbm_vf_map - ffbm mapping
 *
 * @dev:	amdgv device handle
 * @vf_idx: vf index
 * @size: mapping address size
 * @gpa: start mapping gpu pythical address of VF
 * @spa: start mapping system pythical address of PF
 * @permission: write / read permission value
 *
 */

int amdgv_ffbm_vf_mapping(amdgv_dev_t dev, uint32_t vf_idx, uint64_t size, uint64_t gpa, uint64_t spa, uint8_t permission);

/*
 * amdgv_ffbm_clear_vf_mapping - clear all mapping of specific vf
 *
 * @dev:	amdgv device handle
 * @vf_idx: vf index
 *
 */
int amdgv_ffbm_clear_vf_mapping(amdgv_dev_t dev, uint32_t vf_idx);

/*
 * amdgv_ffbm_print_spa_list - print current spa list
 *
 * @dev:	amdgv device handle
 *
 */
int amdgv_ffbm_print_spa_list(amdgv_dev_t dev);

/*
 * amdgv_ffbm_read_spa_list - read current spa list
 *
 * @dev:	amdgv device handle
 * @page_table_content: store all entries info into this buff
 * @restore_length: restored length for buff, 4 KB in default
 * @len: all entries info length, need to be returned
 *
 */
int amdgv_ffbm_read_spa_list(amdgv_dev_t dev, char *page_table_content, int restore_length, int *len);

/*
 * amdgv_ffbm_copy_spa_data - copy current spa list data to a continuous address
 *
 * @dev:	amdgv device handle
 * @page_table_content: store all entries info into this buff
 * @max_num: max entry number
 * @len: write the buffer length value to this address
 */
int amdgv_ffbm_copy_spa_data(amdgv_dev_t dev, void *page_table_content, int max_num, int *len);
/*
 * amdgv_ffbm_mark_badpage - mark system physical address as bad page
 *
 * @dev:	amdgv device handle
 * @spa: mark this spa as bad page
 *
 */
int amdgv_ffbm_mark_badpage(amdgv_dev_t dev, uint64_t spa);

/*
 * amdgv_ffbm_find_spa - get spa from gpa and vf num
 *
 * @dev:	amdgv device handle
 * @gpa:	gpa to query
 * @idx_vf:	vf to query
 * @spa: 	retult spa pointer
 *
 */
int amdgv_ffbm_find_spa(amdgv_dev_t dev, uint64_t gpa, int idx_vf,
		 uint64_t *spa);

/*
 * amdgv_ffbm_find_gpa - get gpa and vf num from spa
 *
 * @dev:	amdgv device handle
 * @spa:	spa to query
 * @gpa:	result gpa pointer
 * @idx_vf:	result vf index
 *
 */
int amdgv_ffbm_find_gpa(amdgv_dev_t dev, uint64_t spa, uint64_t *gpa, int *idx_vf);


/*
 * amdgv_set_timeslice - set world switch timeslice for a particular vf
 *
 * @dev:	amdgv device handle
 * @vf_idx: vf index
 * @time_slice: timeslice value to be used
 * @sched_id: scheduler id to be set
 *
 */
int amdgv_set_timeslice(amdgv_dev_t dev, uint32_t vf_idx, uint32_t time_slice,
						enum amdgv_sched_block sched_block);

/*
 * amdgv_get_timeslice - get world switch timeslice for a particular vf
 *
 * @dev:	amdgv device handle
 * @vf_idx: vf index
 * @sched_id: scheduler id to be set
 *
 */
int amdgv_get_timeslice(amdgv_dev_t dev, uint32_t vf_idx, enum amdgv_sched_block sched_block);


/*
 * amdgv_enable_ras_feature - enable ras feature
 *
 * @dev:	amdgv device handle
 *
 */
int amdgv_enable_ras_feature(amdgv_dev_t dev);


/*
 * amdgv_disable_ras_feature - disable ras feature
 *
 * @dev:	amdgv device handle
 *
 */
int amdgv_disable_ras_feature(amdgv_dev_t dev);

/*
 * amdgv_copy_ip_discovery_data_to_vf - copy ip discovery data to vf's fb
 *
 * @dev:	amdgv device handle
 * @vf_idx: vf index
 *
 */
int amdgv_copy_ip_discovery_data_to_vf(amdgv_dev_t dev, uint32_t vf_idx);

/*
 * amdgv_read_auto_sched_perf_log - dump auto-sched perf log
 *
 * @dev:	amdgv device handle
 * @data_size: reserved buffer size
 * @data:	buffer of perf log dump
 *
 */
int amdgv_read_auto_sched_perf_log(amdgv_dev_t dev, uint32_t *data_size, uint32_t *data);

/*
 * amdgv_query_debug_dump_fb_addr - query the reserved fb offset and size for debug dump
 *
 * @dev:	amdgv device handle
 * @offset: reserved debug dump fb offset
 * @size:	reserved debug dump size in bytes
 *
 */
int amdgv_query_debug_dump_fb_addr(amdgv_dev_t dev, uint32_t *offset, uint32_t *size);

/*
 * amdgv_in_whole_gpu_reset - check if in whole gpu reset
 *
 * @dev:	amdgv device handle
 *
 */
bool amdgv_in_whole_gpu_reset(amdgv_dev_t dev);

/*
 * amdgv_set_mes_info_dump_enable - set mes_info_dump_enable in pf2vf
 *
 * @dev:	amdgv device handle
 * @enable:	enable/disable mes info dump
 *
 */
int amdgv_set_mes_info_dump_enable(amdgv_dev_t dev, bool enable);

/*
 * amdgv_get_mes_info_dump_enable - get mes_info_dump_enable state in pf2vf
 *
 * @dev:	amdgv device handle
 * @data_adapt:	mes_info_dump_enable state in adapter
 * @data_vf:	array of mes_info_dump_enable state in VFs of adapter
 * @num_vf:		count of VF
 *
 */
int amdgv_get_mes_info_dump_enable(amdgv_dev_t dev, uint32_t *data_adapt, uint32_t *data_vf, unsigned int *num_vf);

/*
 * amdgv_set_bp_mode - set mode for Host Driver Break Point Mode
 *
 * @dev:	amdgv device handle
 * @mode:	target mode
 *
 */
int amdgv_set_bp_mode(amdgv_dev_t dev, int mode);

/*
 * amdgv_get_bp_mode - return the type of Break Point mode on which the driver is running
 *
 * @dev:	amdgv device handle
 *
 */
int amdgv_get_bp_mode(amdgv_dev_t dev);

/*
 * amdgv_send_ws_cmd - manually send a world switch command
 *
 * @dev:			amdgv device handle
 * @ws_event:		world switch event
 * @hw_sched_id:	HW scheduler id that the command will be sent to
 * @idx_vf:			VF index
 *
 */
int amdgv_send_ws_cmd(amdgv_dev_t dev, uint32_t ws_event, uint32_t hw_sched_id, uint32_t idx_vf);

/*
 * amdgv_get_current_bp - set mode for Host Driver Break Point Mode
 *
 * @dev:		amdgv device handle
 * @bp_info:	current break point information, including is_in_bp, ws_cmd, hw_sched_id, and idx_vf
 *
 */
int amdgv_get_current_bp(amdgv_dev_t dev, struct amdgv_bp_info *bp_info);

/*
 * amdgv_bp_go - let the world switch to proceed a single step, used in mode 3
 *
 * @dev:	amdgv device handle
 *
 */
int amdgv_bp_go(amdgv_dev_t dev);

/*
 * amdgv_dump_asymmetric_timeslice - get the vf time slice
 *
 * @dev:			amdgv device handle
 * @idx_vf:			the given vf index
 * @is_active_vf:	whether the vf is active
 * @timeslice_vf:	the time slice
 *
 */
int amdgv_dump_asymmetric_timeslice(amdgv_dev_t dev, uint32_t idx_vf, bool *is_active_vf, uint32_t *timeslice_vf);

/*
 * amdgv_dump_asymmetric_fb_layout - get asymmetric fb layout
 *
 * @dev:			amdgv device handle
 * @buf:			dist addr to dump fb_layout content
 * @len:			buffer length
 * @resv_size:		buffer size allocated by shim layer
 *
 */
int amdgv_dump_asymmetric_fb_layout(amdgv_dev_t dev, char *buf, int *len, uint32_t resv_size);

/*
 * amdgv_get_market_name - get marketing name
 *
 * @dev_id:			device id
 * @rev_id:			revision id
 *
 */
const char *amdgv_get_market_name(uint32_t dev_id, uint32_t rev_id);

int amdgv_set_sysmem_va_ptr(amdgv_dev_t dev, void *ptr);

enum amdgv_interrupt_handler_id {
	SUBMISSION_INTERRUPT = 0,
	GPU_DISP_TIMER,
};
int amdgv_register_interrupt_handler(amdgv_dev_t dev, enum amdgv_interrupt_handler_id id, void *handler, void *context);

int amdgv_gpu_timer(amdgv_dev_t dev, uint64_t micro_seconds);

#endif
