// Copyright Advanced Micro Devices, Inc.
//
// SPDX-License-Identifier: MIT

package amdsmi

/*
#cgo LDFLAGS: -lamdsmi
#cgo CFLAGS: -I../../interface
#include "../../interface/amdsmi.h"
#include <stdlib.h>

// CGO does not map anonymous union members on amdsmi_dfc_fw_data_t; C sees white_list via union rules.
static amdsmi_dfc_fw_white_list_t amdsmi_go_dfc_white_list_at(const amdsmi_dfc_fw_t *fw, unsigned int data_idx, unsigned int wl_idx) {
	return fw->data[data_idx].white_list[wl_idx];
}

static uint32_t amdsmi_go_dfc_black_list_at(const amdsmi_dfc_fw_t *fw, unsigned int data_idx, unsigned int bl_idx) {
	return fw->data[data_idx].black_list[bl_idx];
}

// CGO does not expose union fields by name on amdsmi_cper_valid_bits_t.
static uint32_t amdsmi_go_cper_valid_mask(const amdsmi_cper_hdr_t *h) {
	return h->cper_valid_bits.valid_mask;
}

// amdsmi_cper_hdr_t is declared inside #pragma pack(push, 1), so several
// fields land on unaligned offsets. CGO drops unaligned fields from the
// generated Go struct, so accessor helpers are required to read them.
static uint32_t amdsmi_go_cper_signature_end(const amdsmi_cper_hdr_t *h) {
	return h->signature_end;
}

static uint64_t amdsmi_go_cper_persistence_info(const amdsmi_cper_hdr_t *h) {
	return h->persistence_info;
}

// CGO does not expose union members on amdsmi_fabric_info_ver_t.
// This accessor returns a pointer to the v1 variant.
static const amdsmi_fabric_info_v1_t *amdsmi_go_fabric_info_v1(const amdsmi_fabric_info_t *info) {
	return &info->info.fabric_info.v1;
}

// CGO renders C arrays of pointers as opaque types in some toolchains; expose
// per-index accessors for the fabric telemetry datasets and instance/items
// pointer arrays so Go code can iterate them safely.
static const amdsmi_fabric_telemetry_dataset_t *amdsmi_go_fabric_telemetry_dataset_at(
	const amdsmi_fabric_telemetry_t *t, unsigned int idx) {
	return t->datasets[idx];
}

static const amdsmi_fabric_telemetry_instance_t *amdsmi_go_fabric_telemetry_instance_at(
	const amdsmi_fabric_telemetry_dataset_t *d, unsigned int idx) {
	return &d->instances[idx];
}

static amdsmi_fabric_telemetry_item_t amdsmi_go_fabric_telemetry_item_at(
	const amdsmi_fabric_telemetry_instance_t *inst, unsigned int idx) {
	return inst->items[idx];
}
*/
import "C"
import (
	"fmt"
	"runtime"
	"unsafe"
)

type Status uint32                          // amdsmi_status_t
type InitFlags uint64                       // amdsmi_init_flags_t
type ProcessorType int32                    // amdsmi_processor_type_t
type VramType int32                         // amdsmi_vram_type_t
type MemoryPartitionType int32              // amdsmi_memory_partition_type_t
type AcceleratorPartitionType int32         // amdsmi_accelerator_partition_type_t
type AcceleratorPartitionResourceType int32 // amdsmi_accelerator_partition_resource_type_t
type CardFormFactor int32                   // amdsmi_card_form_factor_t
type DriverModel int32                      // amdsmi_driver_model_type_t
type VirtualizationMode int32               // amdsmi_virtualization_mode_t
type FwBlock int32                          // FwBlock identifies a firmware block (amdsmi_fw_block_t)
type ClkType int32                          // ClkType identifies a clock domain to query (amdsmi_clk_type_t)
type TemperatureType int32                  // TemperatureType selects which sensor to read (amdsmi_temperature_type_t)
type TemperatureMetric int32                // TemperatureMetric selects which temperature statistic to read (amdsmi_temperature_metric_t)
// Values are in Celsius.
type PowerCapType int32   // PowerCapType identifies a power-cap sensor / PPT rail (amdsmi_power_cap_type_t)
type NpmStatus int32      // NpmStatus reports whether node power management is enabled (amdsmi_npm_status_t)
type PtlDataFormat uint32 // PtlDataFormat selects a PTL peak-performance data format (amdsmi_ptl_data_format_t)
type AffinityScope int32
type NicFwVersionType int32   // amdsmi_nic_fw_version_type_t
type GuestFwLoadStatus uint16 //amdsmi_guest_fw_load_status_t

const (
	AMDSMI_VIRTUALIZATION_MODE_UNKNOWN     VirtualizationMode = 0
	AMDSMI_VIRTUALIZATION_MODE_BAREMETAL   VirtualizationMode = 1
	AMDSMI_VIRTUALIZATION_MODE_HOST        VirtualizationMode = 2
	AMDSMI_VIRTUALIZATION_MODE_GUEST       VirtualizationMode = 3
	AMDSMI_VIRTUALIZATION_MODE_PASSTHROUGH VirtualizationMode = 4
)

func (vm VirtualizationMode) String() string {
	switch vm {
	case AMDSMI_VIRTUALIZATION_MODE_UNKNOWN:
		return "UNKNOWN"
	case AMDSMI_VIRTUALIZATION_MODE_BAREMETAL:
		return "BAREMETAL"
	case AMDSMI_VIRTUALIZATION_MODE_HOST:
		return "HOST"
	case AMDSMI_VIRTUALIZATION_MODE_GUEST:
		return "GUEST"
	case AMDSMI_VIRTUALIZATION_MODE_PASSTHROUGH:
		return "PASSTHROUGH"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(vm))
	}
}

const (
	AMDSMI_AFFINITY_SCOPE_NODE   AffinityScope = 0
	AMDSMI_AFFINITY_SCOPE_SOCKET AffinityScope = 1
)

const (
	AMDSMI_NIC_FW_VERSION_TYPE_FIXED   NicFwVersionType = 0
	AMDSMI_NIC_FW_VERSION_TYPE_RUNNING NicFwVersionType = 1
	AMDSMI_NIC_FW_VERSION_TYPE_STORED  NicFwVersionType = 2
)

func (t NicFwVersionType) String() string {
	switch t {
	case AMDSMI_NIC_FW_VERSION_TYPE_FIXED:
		return "FIXED"
	case AMDSMI_NIC_FW_VERSION_TYPE_RUNNING:
		return "RUNNING"
	case AMDSMI_NIC_FW_VERSION_TYPE_STORED:
		return "STORED"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int32(t))
	}
}

// C #define macros are not accessible via CGO, so they must be redefined here.
const (
	AMDSMI_MAX_DEVICES                    = 32
	AMDSMI_MAX_VF_COUNT                   = 32
	AMDSMI_GPU_UUID_SIZE                  = 38
	AMDSMI_MAX_PROFILE_COUNT              = 16
	AMDSMI_MAX_ERR_RECORDS                = 10
	AMDSMI_DFC_FW_NUMBER_OF_ENTRIES       = 9
	AMDSMI_MAX_WHITE_LIST_ELEMENTS        = 16
	AMDSMI_MAX_BLACK_LIST_ELEMENTS        = 64
	AMDSMI_MAX_UUID_ELEMENTS              = 16
	AMDSMI_MAX_TA_WHITE_LIST_ELEMENTS     = 8
	AMDSMI_MAX_NUM_NUMA_NODES             = 32
	AMDSMI_MAX_CP_PROFILE_RESOURCES       = 32
	AMDSMI_MAX_ACCELERATOR_PARTITIONS     = 8
	AMDSMI_MAX_ACCELERATOR_PROFILE        = 32
	AMDSMI_MAX_STRING_LENGTH              = 256
	AMDSMI_MAX_CACHE_TYPES                = 10
	AMDSMI_GUARD_EVENT__MAX               = 7
	AMDSMI_MAX_NUM_XGMI_PHYSICAL_LINK     = 64
	AMDSMI_MAX_NUM_PM_POLICIES            = 32
	AMDSMI_MAX_NIC_PORTS                  = 32
	AMDSMI_MAX_NIC_RDMA_DEV               = 32
	AMDSMI_MAX_NIC_FW                     = 64
	AMDSMI_MAX_NUM_FREQUENCIES            = 33
	AMDSMI_MAX_NUMBER_OF_AFIDS_PER_RECORD = 12
	AMDSMI_MAX_NUM_METRICS                = 512

	AMDSMI_FABRIC_PPOD_ID_SIZE                    = 16
	AMDSMI_FABRIC_ACTIVE_ACCELERATORS_BITMAP_SIZE = 32
	AMDSMI_FABRIC_MAX_LOCAL_GPUS                  = 8
	AMDSMI_FABRIC_LABEL_MAX                       = 32
)

const (
	AMDSMI_MEMORY_PARTITION_UNKNOWN MemoryPartitionType = 0
	AMDSMI_MEMORY_PARTITION_NPS1    MemoryPartitionType = 1
	AMDSMI_MEMORY_PARTITION_NPS2    MemoryPartitionType = 2
	AMDSMI_MEMORY_PARTITION_NPS4    MemoryPartitionType = 4
	AMDSMI_MEMORY_PARTITION_NPS8    MemoryPartitionType = 8
)

const (
	AMDSMI_ACCELERATOR_PARTITION_INVALID AcceleratorPartitionType = 0
	AMDSMI_ACCELERATOR_PARTITION_SPX     AcceleratorPartitionType = 1
	AMDSMI_ACCELERATOR_PARTITION_DPX     AcceleratorPartitionType = 2
	AMDSMI_ACCELERATOR_PARTITION_TPX     AcceleratorPartitionType = 3
	AMDSMI_ACCELERATOR_PARTITION_QPX     AcceleratorPartitionType = 4
	AMDSMI_ACCELERATOR_PARTITION_CPX     AcceleratorPartitionType = 5
)

const (
	AMDSMI_ACCELERATOR_XCC     AcceleratorPartitionResourceType = 0
	AMDSMI_ACCELERATOR_ENCODER AcceleratorPartitionResourceType = 1
	AMDSMI_ACCELERATOR_DECODER AcceleratorPartitionResourceType = 2
	AMDSMI_ACCELERATOR_DMA     AcceleratorPartitionResourceType = 3
	AMDSMI_ACCELERATOR_JPEG    AcceleratorPartitionResourceType = 4
)

// GpuBlock identifies a GPU block for ECC queries (amdsmi_gpu_block_t).
type GpuBlock uint64

// amdsmi_gpu_block_t values
const (
	AMDSMI_GPU_BLOCK_INVALID   GpuBlock = 0
	AMDSMI_GPU_BLOCK_FIRST     GpuBlock = (1 << 0)
	AMDSMI_GPU_BLOCK_UMC       GpuBlock = AMDSMI_GPU_BLOCK_FIRST
	AMDSMI_GPU_BLOCK_SDMA      GpuBlock = (1 << 1)
	AMDSMI_GPU_BLOCK_GFX       GpuBlock = (1 << 2)
	AMDSMI_GPU_BLOCK_MMHUB     GpuBlock = (1 << 3)
	AMDSMI_GPU_BLOCK_ATHUB     GpuBlock = (1 << 4)
	AMDSMI_GPU_BLOCK_PCIE_BIF  GpuBlock = (1 << 5)
	AMDSMI_GPU_BLOCK_HDP       GpuBlock = (1 << 6)
	AMDSMI_GPU_BLOCK_XGMI_WAFL GpuBlock = (1 << 7)
	AMDSMI_GPU_BLOCK_DF        GpuBlock = (1 << 8)
	AMDSMI_GPU_BLOCK_SMN       GpuBlock = (1 << 9)
	AMDSMI_GPU_BLOCK_SEM       GpuBlock = (1 << 10)
	AMDSMI_GPU_BLOCK_MP0       GpuBlock = (1 << 11)
	AMDSMI_GPU_BLOCK_MP1       GpuBlock = (1 << 12)
	AMDSMI_GPU_BLOCK_FUSE      GpuBlock = (1 << 13)
	AMDSMI_GPU_BLOCK_MCA       GpuBlock = (1 << 14)
	AMDSMI_GPU_BLOCK_VCN       GpuBlock = (1 << 15)
	AMDSMI_GPU_BLOCK_JPEG      GpuBlock = (1 << 16)
	AMDSMI_GPU_BLOCK_IH        GpuBlock = (1 << 17)
	AMDSMI_GPU_BLOCK_MPIO      GpuBlock = (1 << 18)
	AMDSMI_GPU_BLOCK_LAST      GpuBlock = AMDSMI_GPU_BLOCK_MPIO
	AMDSMI_GPU_BLOCK_RESERVED  GpuBlock = (1 << 63)
)

const (
	AMDSMI_FW_ID_SMU   FwBlock = 1
	AMDSMI_FW_ID_FIRST FwBlock = AMDSMI_FW_ID_SMU
	AMDSMI_FW_ID_CP_CE FwBlock = iota
	AMDSMI_FW_ID_CP_PFP
	AMDSMI_FW_ID_CP_ME
	AMDSMI_FW_ID_CP_MEC_JT1
	AMDSMI_FW_ID_CP_MEC_JT2
	AMDSMI_FW_ID_CP_MEC1
	AMDSMI_FW_ID_CP_MEC2
	AMDSMI_FW_ID_RLC
	AMDSMI_FW_ID_SDMA0
	AMDSMI_FW_ID_SDMA1
	AMDSMI_FW_ID_SDMA2
	AMDSMI_FW_ID_SDMA3
	AMDSMI_FW_ID_SDMA4
	AMDSMI_FW_ID_SDMA5
	AMDSMI_FW_ID_SDMA6
	AMDSMI_FW_ID_SDMA7
	AMDSMI_FW_ID_VCN
	AMDSMI_FW_ID_UVD
	AMDSMI_FW_ID_VCE
	AMDSMI_FW_ID_ISP
	AMDSMI_FW_ID_DMCU_ERAM
	AMDSMI_FW_ID_DMCU_ISR
	AMDSMI_FW_ID_RLC_RESTORE_LIST_GPM_MEM
	AMDSMI_FW_ID_RLC_RESTORE_LIST_SRM_MEM
	AMDSMI_FW_ID_RLC_RESTORE_LIST_CNTL
	AMDSMI_FW_ID_RLC_V
	AMDSMI_FW_ID_MMSCH
	AMDSMI_FW_ID_PSP_SYSDRV
	AMDSMI_FW_ID_PSP_SOSDRV
	AMDSMI_FW_ID_PSP_TOC
	AMDSMI_FW_ID_PSP_KEYDB
	AMDSMI_FW_ID_DFC
	AMDSMI_FW_ID_PSP_SPL
	AMDSMI_FW_ID_DRV_CAP
	AMDSMI_FW_ID_MC
	AMDSMI_FW_ID_PSP_BL
	AMDSMI_FW_ID_CP_PM4
	AMDSMI_FW_ID_RLC_P
	AMDSMI_FW_ID_SEC_POLICY_STAGE2
	AMDSMI_FW_ID_REG_ACCESS_WHITELIST
	AMDSMI_FW_ID_IMU_DRAM
	AMDSMI_FW_ID_IMU_IRAM
	AMDSMI_FW_ID_SDMA_TH0
	AMDSMI_FW_ID_SDMA_TH1
	AMDSMI_FW_ID_CP_MES
	AMDSMI_FW_ID_MES_KIQ
	AMDSMI_FW_ID_MES_STACK
	AMDSMI_FW_ID_MES_THREAD1
	AMDSMI_FW_ID_MES_THREAD1_STACK
	AMDSMI_FW_ID_RLX6
	AMDSMI_FW_ID_RLX6_DRAM_BOOT
	AMDSMI_FW_ID_RS64_ME
	AMDSMI_FW_ID_RS64_ME_P0_DATA
	AMDSMI_FW_ID_RS64_ME_P1_DATA
	AMDSMI_FW_ID_RS64_PFP
	AMDSMI_FW_ID_RS64_PFP_P0_DATA
	AMDSMI_FW_ID_RS64_PFP_P1_DATA
	AMDSMI_FW_ID_RS64_MEC
	AMDSMI_FW_ID_RS64_MEC_P0_DATA
	AMDSMI_FW_ID_RS64_MEC_P1_DATA
	AMDSMI_FW_ID_RS64_MEC_P2_DATA
	AMDSMI_FW_ID_RS64_MEC_P3_DATA
	AMDSMI_FW_ID_PPTABLE
	AMDSMI_FW_ID_PSP_SOC
	AMDSMI_FW_ID_PSP_DBG
	AMDSMI_FW_ID_PSP_INTF
	AMDSMI_FW_ID_RLX6_CORE1
	AMDSMI_FW_ID_RLX6_DRAM_BOOT_CORE1
	AMDSMI_FW_ID_RLCV_LX7
	AMDSMI_FW_ID_RLC_SAVE_RESTORE_LIST
	AMDSMI_FW_ID_ASD
	AMDSMI_FW_ID_TA_RAS
	AMDSMI_FW_ID_TA_XGMI
	AMDSMI_FW_ID_XGMI
	AMDSMI_FW_ID_RLC_SRLG
	AMDSMI_FW_ID_RLC_SRLS
	AMDSMI_FW_ID_PM
	AMDSMI_FW_ID_SMC
	AMDSMI_FW_ID_DMCU
	AMDSMI_FW_ID_PSP_RAS
	AMDSMI_FW_ID_P2S_TABLE
	AMDSMI_FW_ID_PLDM_BUNDLE
	AMDSMI_FW_ID__MAX
)

func (b FwBlock) String() string {
	switch b {
	case AMDSMI_FW_ID_SMU:
		return "FW_ID_SMU"
	case AMDSMI_FW_ID_CP_CE:
		return "FW_ID_CP_CE"
	case AMDSMI_FW_ID_CP_PFP:
		return "FW_ID_CP_PFP"
	case AMDSMI_FW_ID_CP_ME:
		return "FW_ID_CP_ME"
	case AMDSMI_FW_ID_CP_MEC_JT1:
		return "FW_ID_CP_MEC_JT1"
	case AMDSMI_FW_ID_CP_MEC_JT2:
		return "FW_ID_CP_MEC_JT2"
	case AMDSMI_FW_ID_CP_MEC1:
		return "FW_ID_CP_MEC1"
	case AMDSMI_FW_ID_CP_MEC2:
		return "FW_ID_CP_MEC2"
	case AMDSMI_FW_ID_RLC:
		return "FW_ID_RLC"
	case AMDSMI_FW_ID_SDMA0:
		return "FW_ID_SDMA0"
	case AMDSMI_FW_ID_SDMA1:
		return "FW_ID_SDMA1"
	case AMDSMI_FW_ID_SDMA2:
		return "FW_ID_SDMA2"
	case AMDSMI_FW_ID_SDMA3:
		return "FW_ID_SDMA3"
	case AMDSMI_FW_ID_SDMA4:
		return "FW_ID_SDMA4"
	case AMDSMI_FW_ID_SDMA5:
		return "FW_ID_SDMA5"
	case AMDSMI_FW_ID_SDMA6:
		return "FW_ID_SDMA6"
	case AMDSMI_FW_ID_SDMA7:
		return "FW_ID_SDMA7"
	case AMDSMI_FW_ID_VCN:
		return "FW_ID_VCN"
	case AMDSMI_FW_ID_UVD:
		return "FW_ID_UVD"
	case AMDSMI_FW_ID_VCE:
		return "FW_ID_VCE"
	case AMDSMI_FW_ID_ISP:
		return "FW_ID_ISP"
	case AMDSMI_FW_ID_DMCU_ERAM:
		return "FW_ID_DMCU_ERAM"
	case AMDSMI_FW_ID_DMCU_ISR:
		return "FW_ID_DMCU_ISR"
	case AMDSMI_FW_ID_RLC_RESTORE_LIST_GPM_MEM:
		return "FW_ID_RLC_RESTORE_LIST_GPM_MEM"
	case AMDSMI_FW_ID_RLC_RESTORE_LIST_SRM_MEM:
		return "FW_ID_RLC_RESTORE_LIST_SRM_MEM"
	case AMDSMI_FW_ID_RLC_RESTORE_LIST_CNTL:
		return "FW_ID_RLC_RESTORE_LIST_CNTL"
	case AMDSMI_FW_ID_RLC_V:
		return "FW_ID_RLC_V"
	case AMDSMI_FW_ID_MMSCH:
		return "FW_ID_MMSCH"
	case AMDSMI_FW_ID_PSP_SYSDRV:
		return "FW_ID_PSP_SYSDRV"
	case AMDSMI_FW_ID_PSP_SOSDRV:
		return "FW_ID_PSP_SOSDRV"
	case AMDSMI_FW_ID_PSP_TOC:
		return "FW_ID_PSP_TOC"
	case AMDSMI_FW_ID_PSP_KEYDB:
		return "FW_ID_PSP_KEYDB"
	case AMDSMI_FW_ID_DFC:
		return "FW_ID_DFC"
	case AMDSMI_FW_ID_PSP_SPL:
		return "FW_ID_PSP_SPL"
	case AMDSMI_FW_ID_DRV_CAP:
		return "FW_ID_DRV_CAP"
	case AMDSMI_FW_ID_MC:
		return "FW_ID_MC"
	case AMDSMI_FW_ID_PSP_BL:
		return "FW_ID_PSP_BL"
	case AMDSMI_FW_ID_CP_PM4:
		return "FW_ID_CP_PM4"
	case AMDSMI_FW_ID_RLC_P:
		return "FW_ID_RLC_P"
	case AMDSMI_FW_ID_SEC_POLICY_STAGE2:
		return "FW_ID_SEC_POLICY_STAGE2"
	case AMDSMI_FW_ID_REG_ACCESS_WHITELIST:
		return "FW_ID_REG_ACCESS_WHITELIST"
	case AMDSMI_FW_ID_IMU_DRAM:
		return "FW_ID_IMU_DRAM"
	case AMDSMI_FW_ID_IMU_IRAM:
		return "FW_ID_IMU_IRAM"
	case AMDSMI_FW_ID_SDMA_TH0:
		return "FW_ID_SDMA_TH0"
	case AMDSMI_FW_ID_SDMA_TH1:
		return "FW_ID_SDMA_TH1"
	case AMDSMI_FW_ID_CP_MES:
		return "FW_ID_CP_MES"
	case AMDSMI_FW_ID_MES_KIQ:
		return "FW_ID_MES_KIQ"
	case AMDSMI_FW_ID_MES_STACK:
		return "FW_ID_MES_STACK"
	case AMDSMI_FW_ID_MES_THREAD1:
		return "FW_ID_MES_THREAD1"
	case AMDSMI_FW_ID_MES_THREAD1_STACK:
		return "FW_ID_MES_THREAD1_STACK"
	case AMDSMI_FW_ID_RLX6:
		return "FW_ID_RLX6"
	case AMDSMI_FW_ID_RLX6_DRAM_BOOT:
		return "FW_ID_RLX6_DRAM_BOOT"
	case AMDSMI_FW_ID_RS64_ME:
		return "FW_ID_RS64_ME"
	case AMDSMI_FW_ID_RS64_ME_P0_DATA:
		return "FW_ID_RS64_ME_P0_DATA"
	case AMDSMI_FW_ID_RS64_ME_P1_DATA:
		return "FW_ID_RS64_ME_P1_DATA"
	case AMDSMI_FW_ID_RS64_PFP:
		return "FW_ID_RS64_PFP"
	case AMDSMI_FW_ID_RS64_PFP_P0_DATA:
		return "FW_ID_RS64_PFP_P0_DATA"
	case AMDSMI_FW_ID_RS64_PFP_P1_DATA:
		return "FW_ID_RS64_PFP_P1_DATA"
	case AMDSMI_FW_ID_RS64_MEC:
		return "FW_ID_RS64_MEC"
	case AMDSMI_FW_ID_RS64_MEC_P0_DATA:
		return "FW_ID_RS64_MEC_P0_DATA"
	case AMDSMI_FW_ID_RS64_MEC_P1_DATA:
		return "FW_ID_RS64_MEC_P1_DATA"
	case AMDSMI_FW_ID_RS64_MEC_P2_DATA:
		return "FW_ID_RS64_MEC_P2_DATA"
	case AMDSMI_FW_ID_RS64_MEC_P3_DATA:
		return "FW_ID_RS64_MEC_P3_DATA"
	case AMDSMI_FW_ID_PPTABLE:
		return "FW_ID_PPTABLE"
	case AMDSMI_FW_ID_PSP_SOC:
		return "FW_ID_PSP_SOC"
	case AMDSMI_FW_ID_PSP_DBG:
		return "FW_ID_PSP_DBG"
	case AMDSMI_FW_ID_PSP_INTF:
		return "FW_ID_PSP_INTF"
	case AMDSMI_FW_ID_RLX6_CORE1:
		return "FW_ID_RLX6_CORE1"
	case AMDSMI_FW_ID_RLX6_DRAM_BOOT_CORE1:
		return "FW_ID_RLX6_DRAM_BOOT_CORE1"
	case AMDSMI_FW_ID_RLCV_LX7:
		return "FW_ID_RLCV_LX7"
	case AMDSMI_FW_ID_RLC_SAVE_RESTORE_LIST:
		return "FW_ID_RLC_SAVE_RESTORE_LIST"
	case AMDSMI_FW_ID_ASD:
		return "FW_ID_ASD"
	case AMDSMI_FW_ID_TA_RAS:
		return "FW_ID_TA_RAS"
	case AMDSMI_FW_ID_TA_XGMI:
		return "FW_ID_TA_XGMI"
	case AMDSMI_FW_ID_XGMI:
		return "FW_ID_XGMI"
	case AMDSMI_FW_ID_RLC_SRLG:
		return "FW_ID_RLC_SRLG"
	case AMDSMI_FW_ID_RLC_SRLS:
		return "FW_ID_RLC_SRLS"
	case AMDSMI_FW_ID_PM:
		return "FW_ID_PM"
	case AMDSMI_FW_ID_SMC:
		return "FW_ID_SMC"
	case AMDSMI_FW_ID_DMCU:
		return "FW_ID_DMCU"
	case AMDSMI_FW_ID_PSP_RAS:
		return "FW_ID_PSP_RAS"
	case AMDSMI_FW_ID_P2S_TABLE:
		return "FW_ID_P2S_TABLE"
	case AMDSMI_FW_ID_PLDM_BUNDLE:
		return "FW_ID_PLDM_BUNDLE"
	case AMDSMI_FW_ID__MAX:
		return "FW_ID__MAX"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int32(b))
	}
}

// ProfileCapability indexes profile_caps in a partition profile (amdsmi_profile_capability_type_t).
type ProfileCapability int

const (
	AMDSMI_PROFILE_CAPABILITY_MEMORY  ProfileCapability = 0
	AMDSMI_PROFILE_CAPABILITY_ENCODE  ProfileCapability = 1
	AMDSMI_PROFILE_CAPABILITY_DECODE  ProfileCapability = 2
	AMDSMI_PROFILE_CAPABILITY_COMPUTE ProfileCapability = 3
	AMDSMI_PROFILE_CAPABILITY__MAX                      = 4
)

// amdsmi_status_t enum values
const (
	AMDSMI_STATUS_SUCCESS             Status = 0
	AMDSMI_STATUS_INVAL               Status = 1
	AMDSMI_STATUS_NOT_SUPPORTED       Status = 2
	AMDSMI_STATUS_NOT_YET_IMPLEMENTED Status = 3
	AMDSMI_STATUS_FAIL_LOAD_MODULE    Status = 4
	AMDSMI_STATUS_FAIL_LOAD_SYMBOL    Status = 5
	AMDSMI_STATUS_DRM_ERROR           Status = 6
	AMDSMI_STATUS_API_FAILED          Status = 7
	AMDSMI_STATUS_TIMEOUT             Status = 8
	AMDSMI_STATUS_RETRY               Status = 9
	AMDSMI_STATUS_NO_PERM             Status = 10
	AMDSMI_STATUS_INTERRUPT           Status = 11
	AMDSMI_STATUS_IO                  Status = 12
	AMDSMI_STATUS_ADDRESS_FAULT       Status = 13
	AMDSMI_STATUS_FILE_ERROR          Status = 14
	AMDSMI_STATUS_OUT_OF_RESOURCES    Status = 15
	AMDSMI_STATUS_INTERNAL_EXCEPTION  Status = 16
	AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS Status = 17
	AMDSMI_STATUS_INIT_ERROR          Status = 18
	AMDSMI_STATUS_REFCOUNT_OVERFLOW   Status = 19
	AMDSMI_STATUS_DIRECTORY_NOT_FOUND Status = 20
	AMDSMI_STATUS_IPC_ERROR           Status = 21
	AMDSMI_STATUS_BUSY                Status = 30
	AMDSMI_STATUS_NOT_FOUND           Status = 31
	AMDSMI_STATUS_NOT_INIT            Status = 32
	AMDSMI_STATUS_NO_SLOT             Status = 33
	AMDSMI_STATUS_DRIVER_NOT_LOADED   Status = 34
	AMDSMI_STATUS_MORE_DATA           Status = 39
	AMDSMI_STATUS_NO_DATA             Status = 40
	AMDSMI_STATUS_INSUFFICIENT_SIZE   Status = 41
	AMDSMI_STATUS_UNEXPECTED_SIZE     Status = 42
	AMDSMI_STATUS_UNEXPECTED_DATA     Status = 43
	AMDSMI_STATUS_NON_AMD_CPU         Status = 44
	AMDSMI_STATUS_NO_ENERGY_DRV       Status = 45
	AMDSMI_STATUS_NO_MSR_DRV          Status = 46
	AMDSMI_STATUS_NO_HSMP_DRV         Status = 47
	AMDSMI_STATUS_NO_HSMP_SUP         Status = 48
	AMDSMI_STATUS_NO_HSMP_MSG_SUP     Status = 49
	AMDSMI_STATUS_HSMP_TIMEOUT        Status = 50
	AMDSMI_STATUS_NO_DRV              Status = 51
	AMDSMI_STATUS_FILE_NOT_FOUND      Status = 52
	AMDSMI_STATUS_ARG_PTR_NULL        Status = 53
	AMDSMI_STATUS_AMDGPU_RESTART_ERR  Status = 54
	AMDSMI_STATUS_SETTING_UNAVAILABLE Status = 55
	AMDSMI_STATUS_CORRUPTED_EEPROM    Status = 56
	AMDSMI_STATUS_MAP_ERROR           Status = 0xFFFFFFFE
	AMDSMI_STATUS_UNKNOWN_ERROR       Status = 0xFFFFFFFF
)

// amdsmi_init_flags_t enum values
const (
	AMDSMI_INIT_AMD_CPUS       InitFlags = (1 << 0)
	AMDSMI_INIT_AMD_GPUS       InitFlags = (1 << 1)
	AMDSMI_INIT_NON_AMD_CPUS   InitFlags = (1 << 2)
	AMDSMI_INIT_NON_AMD_GPUS   InitFlags = (1 << 3)
	AMDSMI_INIT_AMD_APUS       InitFlags = (AMDSMI_INIT_AMD_CPUS | AMDSMI_INIT_AMD_GPUS)
	AMDSMI_INIT_AMD_NICS       InitFlags = (1 << 4)
	AMDSMI_INIT_ALL_PROCESSORS InitFlags = 0xFFFFFFFF
)

// amdsmi_processor_type_t enum values
const (
	AMDSMI_PROCESSOR_TYPE_UNKNOWN      ProcessorType = 0
	AMDSMI_PROCESSOR_TYPE_AMD_GPU      ProcessorType = 1
	AMDSMI_PROCESSOR_TYPE_AMD_CPU      ProcessorType = 2
	AMDSMI_PROCESSOR_TYPE_NON_AMD_GPU  ProcessorType = 3
	AMDSMI_PROCESSOR_TYPE_NON_AMD_CPU  ProcessorType = 4
	AMDSMI_PROCESSOR_TYPE_AMD_CPU_CORE ProcessorType = 5
	AMDSMI_PROCESSOR_TYPE_AMD_APU      ProcessorType = 6
	AMDSMI_PROCESSOR_TYPE_AMD_NIC      ProcessorType = 7
	AMDSMI_PROCESSOR_TYPE_BRCM_NIC     ProcessorType = 8
	AMDSMI_PROCESSOR_TYPE_BRCM_SWITCH  ProcessorType = 9
)

// amdsmi_vram_type_t enum values
const (
	AMDSMI_VRAM_TYPE_UNKNOWN VramType = 0
	// HBM
	AMDSMI_VRAM_TYPE_HBM   VramType = 1
	AMDSMI_VRAM_TYPE_HBM2  VramType = 2
	AMDSMI_VRAM_TYPE_HBM2E VramType = 3
	AMDSMI_VRAM_TYPE_HBM3  VramType = 4
	AMDSMI_VRAM_TYPE_HBM3E VramType = 5
	// DDR
	AMDSMI_VRAM_TYPE_DDR2 VramType = 10
	AMDSMI_VRAM_TYPE_DDR3 VramType = 11
	AMDSMI_VRAM_TYPE_DDR4 VramType = 12
	AMDSMI_VRAM_TYPE_DDR5 VramType = 13
	// GDDR
	AMDSMI_VRAM_TYPE_GDDR1 VramType = 17
	AMDSMI_VRAM_TYPE_GDDR2 VramType = 18
	AMDSMI_VRAM_TYPE_GDDR3 VramType = 19
	AMDSMI_VRAM_TYPE_GDDR4 VramType = 20
	AMDSMI_VRAM_TYPE_GDDR5 VramType = 21
	AMDSMI_VRAM_TYPE_GDDR6 VramType = 22
	AMDSMI_VRAM_TYPE_GDDR7 VramType = 23
	// LPDDR
	AMDSMI_VRAM_TYPE_LPDDR4 VramType = 30
	AMDSMI_VRAM_TYPE_LPDDR5 VramType = 31
	AMDSMI_VRAM_TYPE__MAX   VramType = AMDSMI_VRAM_TYPE_LPDDR5
)

// amdsmi_card_form_factor_t enum values
const (
	AMDSMI_CARD_FORM_FACTOR_PCIE    CardFormFactor = 0
	AMDSMI_CARD_FORM_FACTOR_OAM     CardFormFactor = 1
	AMDSMI_CARD_FORM_FACTOR_CEM     CardFormFactor = 2
	AMDSMI_CARD_FORM_FACTOR_UNKNOWN CardFormFactor = 3
)

// amdsmi_driver_model_type_t enum values
const (
	AMDSMI_DRIVER_MODEL_TYPE_WDDM DriverModel = 0
	AMDSMI_DRIVER_MODEL_TYPE_WDM  DriverModel = 1
	AMDSMI_DRIVER_MODEL_TYPE_MCDM DriverModel = 2
	AMDSMI_DRIVER_MODEL_TYPE__MAX DriverModel = 3
)

func (m DriverModel) String() string {
	switch m {
	case AMDSMI_DRIVER_MODEL_TYPE_WDDM:
		return "WDDM"
	case AMDSMI_DRIVER_MODEL_TYPE_WDM:
		return "WDM"
	case AMDSMI_DRIVER_MODEL_TYPE_MCDM:
		return "MCDM"
	case AMDSMI_DRIVER_MODEL_TYPE__MAX:
		return "MAX"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(m))
	}
}

const (
	AMDSMI_CLK_TYPE_SYS   ClkType = 0
	AMDSMI_CLK_TYPE_FIRST ClkType = AMDSMI_CLK_TYPE_SYS
	AMDSMI_CLK_TYPE_GFX   ClkType = AMDSMI_CLK_TYPE_SYS
	AMDSMI_CLK_TYPE_DF    ClkType = 1
	AMDSMI_CLK_TYPE_DCEF  ClkType = 2
	AMDSMI_CLK_TYPE_SOC   ClkType = 3
	AMDSMI_CLK_TYPE_MEM   ClkType = 4
	AMDSMI_CLK_TYPE_PCIE  ClkType = 5
	AMDSMI_CLK_TYPE_VCLK0 ClkType = 6
	AMDSMI_CLK_TYPE_VCLK1 ClkType = 7
	AMDSMI_CLK_TYPE_DCLK0 ClkType = 8
	AMDSMI_CLK_TYPE_DCLK1 ClkType = 9
	AMDSMI_CLK_TYPE__MAX          = AMDSMI_CLK_TYPE_DCLK1
)

const (
	AMDSMI_TEMPERATURE_TYPE_EDGE                             TemperatureType = 0
	AMDSMI_TEMPERATURE_TYPE_FIRST                            TemperatureType = AMDSMI_TEMPERATURE_TYPE_EDGE
	AMDSMI_TEMPERATURE_TYPE_HOTSPOT                          TemperatureType = 1
	AMDSMI_TEMPERATURE_TYPE_JUNCTION                         TemperatureType = AMDSMI_TEMPERATURE_TYPE_HOTSPOT
	AMDSMI_TEMPERATURE_TYPE_VRAM                             TemperatureType = 2
	AMDSMI_TEMPERATURE_TYPE_HBM_0                            TemperatureType = 3
	AMDSMI_TEMPERATURE_TYPE_HBM_1                            TemperatureType = 4
	AMDSMI_TEMPERATURE_TYPE_HBM_2                            TemperatureType = 5
	AMDSMI_TEMPERATURE_TYPE_HBM_3                            TemperatureType = 6
	AMDSMI_TEMPERATURE_TYPE_PLX                              TemperatureType = 7
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_FIRST              TemperatureType = 100
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_RETIMER_X          TemperatureType = AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_FIRST
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_IBC          TemperatureType = 101
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_IBC_2        TemperatureType = 102
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_VDD18_VR     TemperatureType = 103
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_04_HBM_B_VR  TemperatureType = 104
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_04_HBM_D_VR  TemperatureType = 105
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_LAST               TemperatureType = 149
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VR_FIRST                TemperatureType = 150
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_VDD0              TemperatureType = 150
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_VDD1              TemperatureType = 151
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_VDD2              TemperatureType = 152
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_VDD3              TemperatureType = 153
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_SOC_A             TemperatureType = 154
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_SOC_C             TemperatureType = 155
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_SOCIO_A           TemperatureType = 156
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_SOCIO_C           TemperatureType = 157
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDD_085_HBM             TemperatureType = 158
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_11_HBM_B          TemperatureType = 159
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_11_HBM_D          TemperatureType = 160
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDD_USR                 TemperatureType = 161
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDIO_11_E32            TemperatureType = 162
	AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VR_LAST                 TemperatureType = 199
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_FIRST                  TemperatureType = 200
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_FPGA               TemperatureType = AMDSMI_TEMPERATURE_TYPE_BASEBOARD_FIRST
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_FRONT              TemperatureType = 201
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_BACK               TemperatureType = 202
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_OAM7               TemperatureType = 203
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_IBC                TemperatureType = 204
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_UFPGA              TemperatureType = 205
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_OAM1               TemperatureType = 206
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_0_1_HSC            TemperatureType = 207
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_2_3_HSC            TemperatureType = 208
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_4_5_HSC            TemperatureType = 209
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_6_7_HSC            TemperatureType = 210
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_FPGA_0V72_VR       TemperatureType = 211
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_FPGA_3V3_VR        TemperatureType = 212
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_0_1_2_3_1V2_VR TemperatureType = 213
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_4_5_6_7_1V2_VR TemperatureType = 214
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_0_1_0V9_VR     TemperatureType = 215
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_4_5_0V9_VR     TemperatureType = 216
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_2_3_0V9_VR     TemperatureType = 217
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_6_7_0V9_VR     TemperatureType = 218
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_0_1_2_3_3V3_VR     TemperatureType = 219
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_4_5_6_7_3V3_VR     TemperatureType = 220
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_IBC_HSC                TemperatureType = 221
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_IBC                    TemperatureType = 222
	AMDSMI_TEMPERATURE_TYPE_BASEBOARD_LAST                   TemperatureType = 249
	AMDSMI_TEMPERATURE_TYPE__MAX                             TemperatureType = AMDSMI_TEMPERATURE_TYPE_BASEBOARD_LAST
)

const (
	AMDSMI_TEMP_CURRENT        TemperatureMetric = 0
	AMDSMI_TEMP_FIRST          TemperatureMetric = AMDSMI_TEMP_CURRENT
	AMDSMI_TEMP_MAX            TemperatureMetric = 1
	AMDSMI_TEMP_MIN            TemperatureMetric = 2
	AMDSMI_TEMP_MAX_HYST       TemperatureMetric = 3
	AMDSMI_TEMP_MIN_HYST       TemperatureMetric = 4
	AMDSMI_TEMP_CRITICAL       TemperatureMetric = 5
	AMDSMI_TEMP_CRITICAL_HYST  TemperatureMetric = 6
	AMDSMI_TEMP_EMERGENCY      TemperatureMetric = 7
	AMDSMI_TEMP_EMERGENCY_HYST TemperatureMetric = 8
	AMDSMI_TEMP_CRIT_MIN       TemperatureMetric = 9
	AMDSMI_TEMP_CRIT_MIN_HYST  TemperatureMetric = 10
	AMDSMI_TEMP_OFFSET         TemperatureMetric = 11
	AMDSMI_TEMP_LOWEST         TemperatureMetric = 12
	AMDSMI_TEMP_HIGHEST        TemperatureMetric = 13
	AMDSMI_TEMP_SHUTDOWN       TemperatureMetric = 14
	AMDSMI_TEMP_LAST           TemperatureMetric = AMDSMI_TEMP_SHUTDOWN
)

const (
	AMDSMI_POWER_CAP_TYPE_PPT0 PowerCapType = 0
	AMDSMI_POWER_CAP_TYPE_PPT1 PowerCapType = 1
)

func (pc PowerCapType) String() string {
	switch pc {
	case AMDSMI_POWER_CAP_TYPE_PPT0:
		return "PPT0"
	case AMDSMI_POWER_CAP_TYPE_PPT1:
		return "PPT1"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int32(pc))
	}
}

const (
	AMDSMI_NPM_STATUS_DISABLED NpmStatus = 0
	AMDSMI_NPM_STATUS_ENABLED  NpmStatus = 1
)

func (npm NpmStatus) String() string {
	switch npm {
	case AMDSMI_NPM_STATUS_DISABLED:
		return "DISABLED"
	case AMDSMI_NPM_STATUS_ENABLED:
		return "ENABLED"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int32(npm))
	}
}

const (
	AMDSMI_PTL_DATA_FORMAT_I8      PtlDataFormat = 0x0
	AMDSMI_PTL_DATA_FORMAT_F16     PtlDataFormat = 0x1
	AMDSMI_PTL_DATA_FORMAT_BF16    PtlDataFormat = 0x2
	AMDSMI_PTL_DATA_FORMAT_F32     PtlDataFormat = 0x3
	AMDSMI_PTL_DATA_FORMAT_F64     PtlDataFormat = 0x4
	AMDSMI_PTL_DATA_FORMAT_F8      PtlDataFormat = 0x5
	AMDSMI_PTL_DATA_FORMAT_VECTOR  PtlDataFormat = 0x6
	AMDSMI_PTL_DATA_FORMAT_INVALID PtlDataFormat = 0xFFFFFFFF
)

func (ptlData PtlDataFormat) String() string {
	switch ptlData {
	case AMDSMI_PTL_DATA_FORMAT_I8:
		return "FORMAT_I8"
	case AMDSMI_PTL_DATA_FORMAT_F16:
		return "FORMAT_F16"
	case AMDSMI_PTL_DATA_FORMAT_BF16:
		return "FORMAT_BF16"
	case AMDSMI_PTL_DATA_FORMAT_F32:
		return "FORMAT_F32"
	case AMDSMI_PTL_DATA_FORMAT_F64:
		return "FORMAT_F64"
	case AMDSMI_PTL_DATA_FORMAT_F8:
		return "FORMAT_F8"
	case AMDSMI_PTL_DATA_FORMAT_VECTOR:
		return "FORMAT_VECTOR"
	case AMDSMI_PTL_DATA_FORMAT_INVALID:
		return "FORMAT_INVALID"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int32(ptlData))
	}
}

const (
	AMDSMI_GUEST_FW_LOAD_STATUS_OK           GuestFwLoadStatus = 0x0
	AMDSMI_GUEST_FW_LOAD_STATUS_OBSOLETE_FW  GuestFwLoadStatus = 0x1
	AMDSMI_GUEST_FW_LOAD_STATUS_BAD_SIG      GuestFwLoadStatus = 0x2
	AMDSMI_GUEST_FW_LOAD_STATUS_FW_LOAD_FAIL GuestFwLoadStatus = 0x3
	AMDSMI_GUEST_FW_LOAD_STATUS_ERR_GENERIC  GuestFwLoadStatus = 0x4
)

func (gs GuestFwLoadStatus) String() string {
	switch gs {
	case AMDSMI_GUEST_FW_LOAD_STATUS_OK:
		return "OK"
	case AMDSMI_GUEST_FW_LOAD_STATUS_OBSOLETE_FW:
		return "OBSOLETE_FW"
	case AMDSMI_GUEST_FW_LOAD_STATUS_BAD_SIG:
		return "BAD_SIG"
	case AMDSMI_GUEST_FW_LOAD_STATUS_FW_LOAD_FAIL:
		return "FW_LOAD_FAIL"
	case AMDSMI_GUEST_FW_LOAD_STATUS_ERR_GENERIC:
		return "ERR_GENERIC"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(gs))
	}
}

func (m MemoryPartitionType) String() string {
	switch m {
	case AMDSMI_MEMORY_PARTITION_NPS1:
		return "NPS1"
	case AMDSMI_MEMORY_PARTITION_NPS2:
		return "NPS2"
	case AMDSMI_MEMORY_PARTITION_NPS4:
		return "NPS4"
	case AMDSMI_MEMORY_PARTITION_NPS8:
		return "NPS8"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(m))
	}
}

func (a AcceleratorPartitionType) String() string {
	switch a {
	case AMDSMI_ACCELERATOR_PARTITION_INVALID:
		return "INVALID"
	case AMDSMI_ACCELERATOR_PARTITION_SPX:
		return "SPX"
	case AMDSMI_ACCELERATOR_PARTITION_DPX:
		return "DPX"
	case AMDSMI_ACCELERATOR_PARTITION_TPX:
		return "TPX"
	case AMDSMI_ACCELERATOR_PARTITION_QPX:
		return "QPX"
	case AMDSMI_ACCELERATOR_PARTITION_CPX:
		return "CPX"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(a))
	}
}

func (r AcceleratorPartitionResourceType) String() string {
	switch r {
	case AMDSMI_ACCELERATOR_XCC:
		return "XCC"
	case AMDSMI_ACCELERATOR_ENCODER:
		return "ENCODER"
	case AMDSMI_ACCELERATOR_DECODER:
		return "DECODER"
	case AMDSMI_ACCELERATOR_DMA:
		return "DMA"
	case AMDSMI_ACCELERATOR_JPEG:
		return "JPEG"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(r))
	}
}

func (v VramType) String() string {
	switch v {
	case AMDSMI_VRAM_TYPE_UNKNOWN:
		return "UNKNOWN"
	case AMDSMI_VRAM_TYPE_HBM:
		return "HBM"
	case AMDSMI_VRAM_TYPE_HBM2:
		return "HBM2"
	case AMDSMI_VRAM_TYPE_HBM2E:
		return "HBM2E"
	case AMDSMI_VRAM_TYPE_HBM3:
		return "HBM3"
	case AMDSMI_VRAM_TYPE_HBM3E:
		return "HBM3E"
	case AMDSMI_VRAM_TYPE_DDR2:
		return "DDR2"
	case AMDSMI_VRAM_TYPE_DDR3:
		return "DDR3"
	case AMDSMI_VRAM_TYPE_DDR4:
		return "DDR4"
	case AMDSMI_VRAM_TYPE_DDR5:
		return "DDR5"
	case AMDSMI_VRAM_TYPE_GDDR1:
		return "GDDR1"
	case AMDSMI_VRAM_TYPE_GDDR2:
		return "GDDR2"
	case AMDSMI_VRAM_TYPE_GDDR3:
		return "GDDR3"
	case AMDSMI_VRAM_TYPE_GDDR4:
		return "GDDR4"
	case AMDSMI_VRAM_TYPE_GDDR5:
		return "GDDR5"
	case AMDSMI_VRAM_TYPE_GDDR6:
		return "GDDR6"
	case AMDSMI_VRAM_TYPE_GDDR7:
		return "GDDR7"
	case AMDSMI_VRAM_TYPE_LPDDR4:
		return "LPDDR4"
	case AMDSMI_VRAM_TYPE_LPDDR5:
		return "LPDDR5"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(v))
	}
}

func (p ProcessorType) String() string {
	switch p {
	case AMDSMI_PROCESSOR_TYPE_UNKNOWN:
		return "UNKNOWN"
	case AMDSMI_PROCESSOR_TYPE_AMD_GPU:
		return "AMD_GPU"
	case AMDSMI_PROCESSOR_TYPE_AMD_CPU:
		return "AMD_CPU"
	case AMDSMI_PROCESSOR_TYPE_NON_AMD_GPU:
		return "NON_AMD_GPU"
	case AMDSMI_PROCESSOR_TYPE_NON_AMD_CPU:
		return "NON_AMD_CPU"
	case AMDSMI_PROCESSOR_TYPE_AMD_CPU_CORE:
		return "AMD_CPU_CORE"
	case AMDSMI_PROCESSOR_TYPE_AMD_APU:
		return "AMD_APU"
	case AMDSMI_PROCESSOR_TYPE_AMD_NIC:
		return "AMD_NIC"
	case AMDSMI_PROCESSOR_TYPE_BRCM_NIC:
		return "BRCM_NIC"
	case AMDSMI_PROCESSOR_TYPE_BRCM_SWITCH:
		return "BRCM_SWITCH"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(p))
	}
}

func (c CardFormFactor) String() string {
	switch c {
	case AMDSMI_CARD_FORM_FACTOR_PCIE:
		return "PCIE"
	case AMDSMI_CARD_FORM_FACTOR_OAM:
		return "OAM"
	case AMDSMI_CARD_FORM_FACTOR_CEM:
		return "CEM"
	case AMDSMI_CARD_FORM_FACTOR_UNKNOWN:
		return "UNKNOWN"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(c))
	}
}

// StatusError Go error type that wraps status codes to names
type StatusError struct {
	Code Status
	Name string
}

// statusNames map C status codes to Go error names
var statusNames = map[Status]string{
	AMDSMI_STATUS_SUCCESS:             "AMDSMI_STATUS_SUCCESS",
	AMDSMI_STATUS_INVAL:               "AMDSMI_STATUS_INVAL",
	AMDSMI_STATUS_NOT_SUPPORTED:       "AMDSMI_STATUS_NOT_SUPPORTED",
	AMDSMI_STATUS_NOT_YET_IMPLEMENTED: "AMDSMI_STATUS_NOT_YET_IMPLEMENTED",
	AMDSMI_STATUS_FAIL_LOAD_MODULE:    "AMDSMI_STATUS_FAIL_LOAD_MODULE",
	AMDSMI_STATUS_FAIL_LOAD_SYMBOL:    "AMDSMI_STATUS_FAIL_LOAD_SYMBOL",
	AMDSMI_STATUS_DRM_ERROR:           "AMDSMI_STATUS_DRM_ERROR",
	AMDSMI_STATUS_API_FAILED:          "AMDSMI_STATUS_API_FAILED",
	AMDSMI_STATUS_TIMEOUT:             "AMDSMI_STATUS_TIMEOUT",
	AMDSMI_STATUS_RETRY:               "AMDSMI_STATUS_RETRY",
	AMDSMI_STATUS_NO_PERM:             "AMDSMI_STATUS_NO_PERM",
	AMDSMI_STATUS_INTERRUPT:           "AMDSMI_STATUS_INTERRUPT",
	AMDSMI_STATUS_IO:                  "AMDSMI_STATUS_IO",
	AMDSMI_STATUS_ADDRESS_FAULT:       "AMDSMI_STATUS_ADDRESS_FAULT",
	AMDSMI_STATUS_FILE_ERROR:          "AMDSMI_STATUS_FILE_ERROR",
	AMDSMI_STATUS_OUT_OF_RESOURCES:    "AMDSMI_STATUS_OUT_OF_RESOURCES",
	AMDSMI_STATUS_INTERNAL_EXCEPTION:  "AMDSMI_STATUS_INTERNAL_EXCEPTION",
	AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS: "AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS",
	AMDSMI_STATUS_INIT_ERROR:          "AMDSMI_STATUS_INIT_ERROR",
	AMDSMI_STATUS_REFCOUNT_OVERFLOW:   "AMDSMI_STATUS_REFCOUNT_OVERFLOW",
	AMDSMI_STATUS_DIRECTORY_NOT_FOUND: "AMDSMI_STATUS_DIRECTORY_NOT_FOUND",
	AMDSMI_STATUS_IPC_ERROR:           "AMDSMI_STATUS_IPC_ERROR",
	AMDSMI_STATUS_BUSY:                "AMDSMI_STATUS_BUSY",
	AMDSMI_STATUS_NOT_FOUND:           "AMDSMI_STATUS_NOT_FOUND",
	AMDSMI_STATUS_NOT_INIT:            "AMDSMI_STATUS_NOT_INIT",
	AMDSMI_STATUS_NO_SLOT:             "AMDSMI_STATUS_NO_SLOT",
	AMDSMI_STATUS_DRIVER_NOT_LOADED:   "AMDSMI_STATUS_DRIVER_NOT_LOADED",
	AMDSMI_STATUS_MORE_DATA:           "AMDSMI_STATUS_MORE_DATA",
	AMDSMI_STATUS_NO_DATA:             "AMDSMI_STATUS_NO_DATA",
	AMDSMI_STATUS_INSUFFICIENT_SIZE:   "AMDSMI_STATUS_INSUFFICIENT_SIZE",
	AMDSMI_STATUS_UNEXPECTED_SIZE:     "AMDSMI_STATUS_UNEXPECTED_SIZE",
	AMDSMI_STATUS_UNEXPECTED_DATA:     "AMDSMI_STATUS_UNEXPECTED_DATA",
	AMDSMI_STATUS_NON_AMD_CPU:         "AMDSMI_STATUS_NON_AMD_CPU",
	AMDSMI_STATUS_NO_ENERGY_DRV:       "AMDSMI_STATUS_NO_ENERGY_DRV",
	AMDSMI_STATUS_NO_MSR_DRV:          "AMDSMI_STATUS_NO_MSR_DRV",
	AMDSMI_STATUS_NO_HSMP_DRV:         "AMDSMI_STATUS_NO_HSMP_DRV",
	AMDSMI_STATUS_NO_HSMP_SUP:         "AMDSMI_STATUS_NO_HSMP_SUP",
	AMDSMI_STATUS_NO_HSMP_MSG_SUP:     "AMDSMI_STATUS_NO_HSMP_MSG_SUP",
	AMDSMI_STATUS_HSMP_TIMEOUT:        "AMDSMI_STATUS_HSMP_TIMEOUT",
	AMDSMI_STATUS_NO_DRV:              "AMDSMI_STATUS_NO_DRV",
	AMDSMI_STATUS_FILE_NOT_FOUND:      "AMDSMI_STATUS_FILE_NOT_FOUND",
	AMDSMI_STATUS_ARG_PTR_NULL:        "AMDSMI_STATUS_ARG_PTR_NULL",
	AMDSMI_STATUS_AMDGPU_RESTART_ERR:  "AMDSMI_STATUS_AMDGPU_RESTART_ERR",
	AMDSMI_STATUS_SETTING_UNAVAILABLE: "AMDSMI_STATUS_SETTING_UNAVAILABLE",
	AMDSMI_STATUS_CORRUPTED_EEPROM:    "AMDSMI_STATUS_CORRUPTED_EEPROM",
	AMDSMI_STATUS_MAP_ERROR:           "AMDSMI_STATUS_MAP_ERROR",
	AMDSMI_STATUS_UNKNOWN_ERROR:       "AMDSMI_STATUS_UNKNOWN_ERROR",
}

func (e *StatusError) Error() string {
	if e.Name != "" {
		return fmt.Sprintf("amdsmi: %s (code %d)", e.Name, e.Code)
	}
	return fmt.Sprintf("amdsmi: error code %d", e.Code)
}

func checkStatus(status Status) error {
	if status == AMDSMI_STATUS_SUCCESS {
		return nil
	}
	return &StatusError{Code: status, Name: statusNames[status]}
}

// Init initializes the AMD SMI library with the given flags.
func Init(flags InitFlags) error {
	return checkStatus(Status(C.amdsmi_init(C.uint64_t(flags))))
}

// ShutDown shuts down the AMD SMI library and frees resources.
func ShutDown() error {
	return checkStatus(Status(C.amdsmi_shut_down()))
}

type NodeHandle struct {
	raw uintptr
}

// cPtr returns the raw C handle for passing directly to C functions.
func (nh NodeHandle) cPtr() C.amdsmi_node_handle {
	return C.amdsmi_node_handle(unsafe.Pointer(nh.raw))
}

// ProcessorHandle is a user-friendly handle for a GPU or processor.
// Stored as uintptr to prevent Go's GC from scanning C pointer values.
type ProcessorHandle struct {
	raw uintptr
}

// cPtr returns the raw C handle for passing directly to C functions.
func (ph ProcessorHandle) cPtr() C.amdsmi_processor_handle {
	return C.amdsmi_processor_handle(unsafe.Pointer(ph.raw))
}

// VfHandle is a handle for a virtual function.
type VfHandle struct {
	raw uint64
}

// cVfHandle returns the C VF handle for passing to C functions.
func (vh VfHandle) cVfHandle() C.amdsmi_vf_handle_t {
	return C.amdsmi_vf_handle_t{handle: C.uint64_t(vh.raw)}
}

func GetProcessorHandlesByType(pt ProcessorType) ([]ProcessorHandle, error) {
	var cSocketHandle C.amdsmi_socket_handle
	var cCount C.uint32_t = C.uint32_t(AMDSMI_MAX_DEVICES)
	handleArray := make([]C.amdsmi_processor_handle, AMDSMI_MAX_DEVICES)

	ret := C.amdsmi_get_processor_handles_by_type(cSocketHandle, C.processor_type_t(pt), &handleArray[0], &cCount)
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}
	if cCount == 0 {
		return nil, nil
	}

	result := make([]ProcessorHandle, cCount)
	for i := C.uint32_t(0); i < cCount; i++ {
		result[i] = ProcessorHandle{raw: uintptr(handleArray[i])}
	}
	return result, nil
}

// GetProcessorType returns the processor type for the given handle.
func GetProcessorType(ph ProcessorHandle) (ProcessorType, error) {
	var cPtype C.processor_type_t
	ret := C.amdsmi_get_processor_type(ph.cPtr(), &cPtype)
	if err := checkStatus(Status(ret)); err != nil {
		return AMDSMI_PROCESSOR_TYPE_UNKNOWN, err
	}
	return ProcessorType(cPtype), nil
}

// GetProcessorHandles returns all discovered processor handles.
func GetProcessorHandles() ([]ProcessorHandle, error) {
	var cSocketHandle C.amdsmi_socket_handle
	var cCount C.uint32_t = C.uint32_t(AMDSMI_MAX_DEVICES)
	handleArray := make([]C.amdsmi_processor_handle, AMDSMI_MAX_DEVICES)

	ret := C.amdsmi_get_processor_handles(cSocketHandle, &cCount, &handleArray[0])
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}
	if cCount == 0 {
		return nil, nil
	}

	result := make([]ProcessorHandle, cCount)
	for i := C.uint32_t(0); i < cCount; i++ {
		result[i] = ProcessorHandle{raw: uintptr(handleArray[i])}
	}
	return result, nil
}

type Bdf uint64

func (b Bdf) Function() uint8 { return uint8(b & 0x7) }
func (b Bdf) Device() uint8   { return uint8((b >> 3) & 0x1F) }
func (b Bdf) Bus() uint8      { return uint8((b >> 8) & 0xFF) }
func (b Bdf) Domain() uint64  { return uint64(b >> 16) }
func (b Bdf) String() string {
	return fmt.Sprintf("%04x:%02x:%02x.%x", b.Domain(), b.Bus(), b.Device(), b.Function())
}

func GetProcessorHandleFromBdf(bdfInput Bdf) (ProcessorHandle, error) {
	var cbdf C.amdsmi_bdf_t = *(*C.amdsmi_bdf_t)(unsafe.Pointer(&bdfInput))
	var handle C.amdsmi_processor_handle

	ret := C.amdsmi_get_processor_handle_from_bdf(cbdf, &handle)

	if err := checkStatus(Status(ret)); err != nil {
		return ProcessorHandle{}, err
	}
	return ProcessorHandle{raw: uintptr(handle)}, nil
}

func GetGpuDeviceBdf(ph ProcessorHandle) (Bdf, error) {
	var cbdf C.amdsmi_bdf_t

	ret := C.amdsmi_get_gpu_device_bdf(ph.cPtr(), &cbdf)
	if err := checkStatus(Status(ret)); err != nil {
		return 0, err
	}

	bdfResult := *(*Bdf)(unsafe.Pointer(&cbdf))

	return bdfResult, nil
}

func GetGpuDeviceUuid(ph ProcessorHandle) (string, error) {
	var cuuid_length C.uint32_t = (C.uint32_t)(AMDSMI_GPU_UUID_SIZE)
	cuuid := make([]C.char, AMDSMI_GPU_UUID_SIZE)

	ret := C.amdsmi_get_gpu_device_uuid(ph.cPtr(), &cuuid_length, &cuuid[0])
	if err := checkStatus(Status(ret)); err != nil {
		return "", err
	}

	return C.GoString(&cuuid[0]), nil
}

func GetGpuVirtualizationMode(ph ProcessorHandle) (VirtualizationMode, error) {
	var cPtype C.amdsmi_virtualization_mode_t

	ret := C.amdsmi_get_gpu_virtualization_mode(ph.cPtr(), &cPtype)
	if err := checkStatus(Status(ret)); err != nil {
		return AMDSMI_VIRTUALIZATION_MODE_UNKNOWN, err
	}

	return VirtualizationMode(cPtype), nil
}

func GetCpuAffinityWithScope(ph ProcessorHandle, scope AffinityScope) ([]uint64, error) {
	cCPUSet := make([]C.uint64_t, 4)

	ret := C.amdsmi_get_cpu_affinity_with_scope(ph.cPtr(),
		C.uint32_t(len(cCPUSet)),
		(*C.uint64_t)(unsafe.Pointer(&cCPUSet[0])),
		C.amdsmi_affinity_scope_t(scope))
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}

	result := make([]uint64, 4)
	for i := 0; i < 4; i++ {
		result[i] = (uint64)(cCPUSet[i])
	}

	return result, nil
}

func GetNodeHandle(ph ProcessorHandle) (NodeHandle, error) {
	var cnh C.amdsmi_node_handle
	ret := C.amdsmi_get_node_handle(ph.cPtr(), &cnh)
	if err := checkStatus(Status(ret)); err != nil {
		return NodeHandle{}, err
	}

	return NodeHandle{raw: uintptr(cnh)}, nil
}

// GetIndexFromProcessorHandle returns the processor index for a handle (amdsmi_get_index_from_processor_handle).
func GetIndexFromProcessorHandle(ph ProcessorHandle) (uint32, error) {
	var cidx C.uint32_t

	ret := C.amdsmi_get_index_from_processor_handle(ph.cPtr(), &cidx)
	if err := checkStatus(Status(ret)); err != nil {
		return 0, err
	}
	return uint32(cidx), nil
}

// GetProcessorHandleFromIndex returns a processor handle for a 0-based index.
func GetProcessorHandleFromIndex(index uint32) (ProcessorHandle, error) {
	var cHandle C.amdsmi_processor_handle
	ret := C.amdsmi_get_processor_handle_from_index(C.uint32_t(index), &cHandle)
	if err := checkStatus(Status(ret)); err != nil {
		return ProcessorHandle{}, err
	}
	return ProcessorHandle{raw: uintptr(cHandle)}, nil
}

func GetProcessorBdf(ph ProcessorHandle) (Bdf, error) {
	var cbdf C.amdsmi_bdf_t

	ret := C.amdsmi_get_processor_bdf(ph.cPtr(), &cbdf)
	if err := checkStatus(Status(ret)); err != nil {
		return 0, err
	}

	bdfResult := *(*Bdf)(unsafe.Pointer(&cbdf))

	return bdfResult, nil
}

func GetProcessorHandleFromUuid(uuid string) (ProcessorHandle, error) {
	cuuid := C.CString(uuid)
	defer C.free(unsafe.Pointer(cuuid))
	var handle C.amdsmi_processor_handle

	ret := C.amdsmi_get_processor_handle_from_uuid(cuuid, &handle)
	if err := checkStatus(Status(ret)); err != nil {
		return ProcessorHandle{}, err
	}
	return ProcessorHandle{raw: uintptr(handle)}, nil

}

func GetVfHandleFromBdf(bdfInput Bdf) (VfHandle, error) {
	var cbdf C.amdsmi_bdf_t = *(*C.amdsmi_bdf_t)(unsafe.Pointer(&bdfInput))
	var cVFhandle C.amdsmi_vf_handle_t

	ret := C.amdsmi_get_vf_handle_from_bdf(cbdf, &cVFhandle)
	if err := checkStatus(Status(ret)); err != nil {
		return VfHandle{}, err
	}
	return VfHandle{raw: (uint64)(cVFhandle.handle)}, nil
}

func GetVfHandleFromUuid(uuid string) (VfHandle, error) {
	cuuid := C.CString(uuid)
	defer C.free(unsafe.Pointer(cuuid))
	var cVFhandle C.amdsmi_vf_handle_t

	ret := C.amdsmi_get_vf_handle_from_uuid(cuuid, &cVFhandle)
	if err := checkStatus(Status(ret)); err != nil {
		return VfHandle{}, err
	}

	return VfHandle{raw: uint64(cVFhandle.handle)}, nil
}

func GetVfHandleFromVfIndex(ph ProcessorHandle, fcnIdx int) (VfHandle, error) {
	var cVFhandle C.amdsmi_vf_handle_t

	ret := C.amdsmi_get_vf_handle_from_vf_index(ph.cPtr(), C.uint32_t(fcnIdx), &cVFhandle)
	if err := checkStatus(Status(ret)); err != nil {
		return VfHandle{}, err
	}

	return VfHandle{raw: uint64(cVFhandle.handle)}, nil
}

func GetVfBdf(vh VfHandle) (Bdf, error) {
	var cbdf C.amdsmi_bdf_t

	ret := C.amdsmi_get_vf_bdf(vh.cVfHandle(), &cbdf)
	if err := checkStatus(Status(ret)); err != nil {
		return 0, err
	}

	bdfResult := *(*Bdf)(unsafe.Pointer(&cbdf))

	return bdfResult, nil
}

func GetVfUuid(vh VfHandle) (string, error) {
	var cuuid_length C.uint32_t = (C.uint32_t)(AMDSMI_GPU_UUID_SIZE)
	cuuid := make([]C.char, AMDSMI_GPU_UUID_SIZE)

	ret := C.amdsmi_get_vf_uuid(vh.cVfHandle(), &cuuid_length, &cuuid[0])
	if err := checkStatus(Status(ret)); err != nil {
		return "", err
	}

	return C.GoString(&cuuid[0]), nil
}

// GetNicProcessorHandles returns all discovered NIC processor handles.
func GetNicProcessorHandles() ([]ProcessorHandle, error) {
	var cSocketHandle C.amdsmi_socket_handle
	var cCount C.uint32_t = C.uint32_t(AMDSMI_MAX_DEVICES)
	handleArray := make([]C.amdsmi_processor_handle, AMDSMI_MAX_DEVICES)

	ret := C.amdsmi_get_nic_processor_handles(cSocketHandle, &cCount, &handleArray[0])
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}
	if cCount == 0 {
		return nil, nil
	}

	result := make([]ProcessorHandle, cCount)
	for i := C.uint32_t(0); i < cCount; i++ {
		result[i] = ProcessorHandle{raw: uintptr(handleArray[i])}
	}
	return result, nil
}

// GetNicDeviceBdf returns the BDF of the given NIC device.
func GetNicDeviceBdf(ph ProcessorHandle) (Bdf, error) {
	var cbdf C.amdsmi_bdf_t

	ret := C.amdsmi_get_nic_device_bdf(ph.cPtr(), &cbdf)

	if err := checkStatus(Status(ret)); err != nil {
		return 0, err
	}

	bdfResult := *(*Bdf)(unsafe.Pointer(&cbdf))
	return bdfResult, nil
}

// Version holds the AMD SMI library version.
type Version struct {
	Major   uint32
	Minor   uint32
	Release uint32
}

// GetLibVersion returns the AMD SMI library version.
func GetLibVersion() (Version, error) {
	var cVer C.amdsmi_version_t
	ret := C.amdsmi_get_lib_version(&cVer)
	if err := checkStatus(Status(ret)); err != nil {
		return Version{}, err
	}
	return Version{
		Major:   uint32(cVer.major),
		Minor:   uint32(cVer.minor),
		Release: uint32(cVer.release),
	}, nil
}

func StatusCodeToString(status Status) (string, error) {
	var cstr *C.char

	ret := C.amdsmi_status_code_to_string(C.amdsmi_status_t(status), &cstr)
	if err := checkStatus(Status(ret)); err != nil {
		return "", err
	}

	return C.GoString(cstr), nil
}

// DriverInfo type (amdsmi_driver_info_t).
type DriverInfo struct {
	DriverVersion string
	DriverDate    string
	DriverName    string
}

func GetGpuDriverInfo(ph ProcessorHandle) (DriverInfo, error) {
	var cDrvInfo C.amdsmi_driver_info_t

	ret := C.amdsmi_get_gpu_driver_info(ph.cPtr(), &cDrvInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return DriverInfo{}, err
	}

	return DriverInfo{
		DriverVersion: C.GoString(&cDrvInfo.driver_version[0]),
		DriverDate:    C.GoString(&cDrvInfo.driver_date[0]),
		DriverName:    C.GoString(&cDrvInfo.driver_name[0]),
	}, nil
}

func GetGpuDriverModel(ph ProcessorHandle) (DriverModel, error) {
	var cDriverModel C.amdsmi_driver_model_type_t

	ret := C.amdsmi_get_gpu_driver_model(ph.cPtr(), &cDriverModel)
	if err := checkStatus(Status(ret)); err != nil {
		return AMDSMI_DRIVER_MODEL_TYPE_WDDM, err
	}

	return DriverModel(cDriverModel), nil
}

// AsicInfo holds GPU ASIC identification and capability information.
type AsicInfo struct {
	MarketName            string
	VendorID              uint32
	VendorName            string
	SubvendorID           uint32
	DeviceID              uint64
	RevID                 uint32
	AsicSerial            string
	OamID                 uint32
	NumComputeUnits       uint32
	TargetGraphicsVersion uint64
	SubsystemID           uint32
	Flags                 uint64
}

// GetGpuAsicInfo returns ASIC information for the given processor.
func GetGpuAsicInfo(ph ProcessorHandle) (AsicInfo, error) {
	var cInfo C.amdsmi_asic_info_t
	ret := C.amdsmi_get_gpu_asic_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return AsicInfo{}, err
	}
	return AsicInfo{
		MarketName:            C.GoString(&cInfo.market_name[0]),
		VendorID:              uint32(cInfo.vendor_id),
		VendorName:            C.GoString(&cInfo.vendor_name[0]),
		SubvendorID:           uint32(cInfo.subvendor_id),
		DeviceID:              uint64(cInfo.device_id),
		RevID:                 uint32(cInfo.rev_id),
		AsicSerial:            C.GoString(&cInfo.asic_serial[0]),
		OamID:                 uint32(cInfo.oam_id),
		NumComputeUnits:       uint32(cInfo.num_of_compute_units),
		TargetGraphicsVersion: uint64(cInfo.target_graphics_version),
		SubsystemID:           uint32(cInfo.subsystem_id),
		Flags:                 uint64(cInfo.flags),
	}, nil
}

// PowerCapInfo holds power cap information for a GPU.
type PowerCapInfo struct {
	PowerCap        uint64
	DefaultPowerCap uint64
	DpmCap          uint64
	MinPowerCap     uint64
	MaxPowerCap     uint64
}

// GetPowerCapInfo returns power cap information for the given processor.
func GetPowerCapInfo(ph ProcessorHandle, sensorInd uint32) (PowerCapInfo, error) {
	var cInfo C.amdsmi_power_cap_info_t
	ret := C.amdsmi_get_power_cap_info(ph.cPtr(), C.uint32_t(sensorInd), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return PowerCapInfo{}, err
	}
	return PowerCapInfo{
		PowerCap:        uint64(cInfo.power_cap),
		DefaultPowerCap: uint64(cInfo.default_power_cap),
		DpmCap:          uint64(cInfo.dpm_cap),
		MinPowerCap:     uint64(cInfo.min_power_cap),
		MaxPowerCap:     uint64(cInfo.max_power_cap),
	}, nil
}

// PcieStatic holds static PCIe information.
type PcieStatic struct {
	MaxPcieWidth            uint16
	MaxPcieSpeed            uint32
	PcieInterfaceVersion    uint32
	SlotType                CardFormFactor
	MaxPcieInterfaceVersion uint32
}

// PcieMetric holds PCIe performance metrics.
type PcieMetric struct {
	PcieWidth                       uint16
	PcieSpeed                       uint32
	PcieBandwidth                   uint32
	PcieReplayCount                 uint64
	PcieL0ToRecoveryCount           uint64
	PcieReplayRollOverCount         uint64
	PcieNakSentCount                uint64
	PcieNakReceivedCount            uint64
	PcieLcPerfOtherEndRecoveryCount uint32
}

// PcieInfo holds combined static and metric PCIe information.
type PcieInfo struct {
	Static PcieStatic
	Metric PcieMetric
}

// GetPcieInfo returns PCIe information for the given processor.
func GetPcieInfo(ph ProcessorHandle) (PcieInfo, error) {
	var cInfo C.amdsmi_pcie_info_t
	ret := C.amdsmi_get_pcie_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return PcieInfo{}, err
	}
	return PcieInfo{
		Static: PcieStatic{
			MaxPcieWidth:            uint16(cInfo.pcie_static.max_pcie_width),
			MaxPcieSpeed:            uint32(cInfo.pcie_static.max_pcie_speed),
			PcieInterfaceVersion:    uint32(cInfo.pcie_static.pcie_interface_version),
			SlotType:                CardFormFactor(cInfo.pcie_static.slot_type),
			MaxPcieInterfaceVersion: uint32(cInfo.pcie_static.max_pcie_interface_version),
		},
		Metric: PcieMetric{
			PcieWidth:                       uint16(cInfo.pcie_metric.pcie_width),
			PcieSpeed:                       uint32(cInfo.pcie_metric.pcie_speed),
			PcieBandwidth:                   uint32(cInfo.pcie_metric.pcie_bandwidth),
			PcieReplayCount:                 uint64(cInfo.pcie_metric.pcie_replay_count),
			PcieL0ToRecoveryCount:           uint64(cInfo.pcie_metric.pcie_l0_to_recovery_count),
			PcieReplayRollOverCount:         uint64(cInfo.pcie_metric.pcie_replay_roll_over_count),
			PcieNakSentCount:                uint64(cInfo.pcie_metric.pcie_nak_sent_count),
			PcieNakReceivedCount:            uint64(cInfo.pcie_metric.pcie_nak_received_count),
			PcieLcPerfOtherEndRecoveryCount: uint32(cInfo.pcie_metric.pcie_lc_perf_other_end_recovery_count),
		},
	}, nil
}

// Frequencies describes a list of supported frequency / transfer values (amdsmi_frequencies_t).
type Frequencies struct {
	HasDeepSleep bool
	NumSupported uint32
	Current      uint32
	Values       []uint64 // first NumSupported entries are valid (C frequency[])
}

// GpuPciBandwidth lists possible PCIe transfer rates and lane counts (amdsmi_pcie_bandwidth_t).
type GpuPciBandwidth struct {
	TransferRate Frequencies
	Lanes        []uint32 // parallel to transfer rates; first NumSupported entries are valid
}

// GetGpuPciBandwidth returns supported PCIe bandwidth combinations for the device (amdsmi_get_gpu_pci_bandwidth).
func GetGpuPciBandwidth(ph ProcessorHandle) (GpuPciBandwidth, error) {
	var cbw C.amdsmi_pcie_bandwidth_t

	ret := C.amdsmi_get_gpu_pci_bandwidth(ph.cPtr(), &cbw)
	if err := checkStatus(Status(ret)); err != nil {
		return GpuPciBandwidth{}, err
	}

	tr := cbw.transfer_rate
	n := uint32(tr.num_supported)
	if n > AMDSMI_MAX_NUM_FREQUENCIES {
		n = AMDSMI_MAX_NUM_FREQUENCIES
	}

	values := make([]uint64, n)
	lanes := make([]uint32, n)
	for i := uint32(0); i < n; i++ {
		values[i] = uint64(tr.frequency[i])
		lanes[i] = uint32(cbw.lanes[i])
	}

	return GpuPciBandwidth{
		TransferRate: Frequencies{
			HasDeepSleep: bool(tr.has_deep_sleep),
			NumSupported: uint32(tr.num_supported),
			Current:      uint32(tr.current),
			Values:       values,
		},
		Lanes: lanes,
	}, nil
}

// VramInfo holds VRAM information for a GPU.
type VramInfo struct {
	VramType         VramType
	VramVendor       string
	VramSize         uint64
	VramBitWidth     uint32
	VramMaxBandwidth uint64
}

// GetGpuVramInfo returns VRAM information for the given processor.
func GetGpuVramInfo(ph ProcessorHandle) (VramInfo, error) {
	var cInfo C.amdsmi_vram_info_t
	ret := C.amdsmi_get_gpu_vram_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return VramInfo{}, err
	}
	return VramInfo{
		VramType:         VramType(cInfo.vram_type),
		VramVendor:       C.GoString(&cInfo.vram_vendor[0]),
		VramSize:         uint64(cInfo.vram_size),
		VramBitWidth:     uint32(cInfo.vram_bit_width),
		VramMaxBandwidth: uint64(cInfo.vram_max_bandwidth),
	}, nil
}

// BoardInfo holds board information for a GPU.
type BoardInfo struct {
	ModelNumber      string
	ProductSerial    string
	FruID            string
	ProductName      string
	ManufacturerName string
}

// GetGpuBoardInfo returns board information for the given processor.
func GetGpuBoardInfo(ph ProcessorHandle) (BoardInfo, error) {
	var cInfo C.amdsmi_board_info_t
	ret := C.amdsmi_get_gpu_board_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return BoardInfo{}, err
	}
	return BoardInfo{
		ModelNumber:      C.GoString(&cInfo.model_number[0]),
		ProductSerial:    C.GoString(&cInfo.product_serial[0]),
		FruID:            C.GoString(&cInfo.fru_id[0]),
		ProductName:      C.GoString(&cInfo.product_name[0]),
		ManufacturerName: C.GoString(&cInfo.manufacturer_name[0]),
	}, nil
}

type PfFbInfo struct {
	TotalFbSize   uint32
	PfFbReserved  uint32
	PfFbOffset    uint32
	FbAlignment   uint32
	MaxVfFbUsable uint32
	MinVfFbUsable uint32
}

func GetFbLayout(ph ProcessorHandle) (PfFbInfo, error) {
	var cInfo C.amdsmi_pf_fb_info_t

	ret := C.amdsmi_get_fb_layout(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return PfFbInfo{}, err
	}

	Info := PfFbInfo{
		TotalFbSize:   uint32(cInfo.total_fb_size),
		PfFbReserved:  uint32(cInfo.pf_fb_reserved),
		PfFbOffset:    uint32(cInfo.pf_fb_offset),
		FbAlignment:   uint32(cInfo.fb_alignment),
		MaxVfFbUsable: uint32(cInfo.max_vf_fb_usable),
		MinVfFbUsable: uint32(cInfo.min_vf_fb_usable),
	}

	return Info, nil
}

type FwInfoList struct {
	FwID      FwBlock
	FwVersion uint64
}

type FwInfo struct {
	NumFwInfo uint8
	FwList    [AMDSMI_FW_ID__MAX]FwInfoList
}

func GetFwInfo(ph ProcessorHandle) (FwInfo, error) {
	var cInfo C.amdsmi_fw_info_t

	ret := C.amdsmi_get_fw_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return FwInfo{}, err
	}

	goInfo := FwInfo{
		NumFwInfo: uint8(cInfo.num_fw_info),
	}

	for i := C.uint32_t(0); i < C.uint32_t(cInfo.num_fw_info); i++ {
		goInfo.FwList[i].FwID = FwBlock(cInfo.fw_info_list[i].fw_id)
		goInfo.FwList[i].FwVersion = uint64(cInfo.fw_info_list[i].fw_version)
	}

	return goInfo, nil
}

// VbiosInfo type (amdsmi_vbios_info_t).
type VbiosInfo struct {
	Name         string
	BuildDate    string
	PartNumber   string
	Version      string
	BootFirmware string
}

// GetGpuVbiosInfo returns static vBIOS information for the device.
func GetGpuVbiosInfo(ph ProcessorHandle) (VbiosInfo, error) {
	var cInfo C.amdsmi_vbios_info_t

	ret := C.amdsmi_get_gpu_vbios_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return VbiosInfo{}, err
	}

	return VbiosInfo{
		Name:         C.GoString(&cInfo.name[0]),
		BuildDate:    C.GoString(&cInfo.build_date[0]),
		PartNumber:   C.GoString(&cInfo.part_number[0]),
		Version:      C.GoString(&cInfo.version[0]),
		BootFirmware: C.GoString(&cInfo.boot_firmware[0]),
	}, nil
}

// FwLoadErrorRecord type (amdsmi_fw_load_error_record_t).
type FwLoadErrorRecord struct {
	Timestamp uint64
	VfIdx     uint32
	FwID      uint32
	Status    GuestFwLoadStatus
}

// FwErrorRecord type (amdsmi_fw_error_record_t).
type FwErrorRecord struct {
	NumErrRecords uint8
	ErrRecords    [AMDSMI_MAX_ERR_RECORDS]FwLoadErrorRecord
}

// GetFwErrorRecords returns firmware load error records.
func GetFwErrorRecords(ph ProcessorHandle) (FwErrorRecord, error) {
	var cRecords C.amdsmi_fw_error_record_t

	ret := C.amdsmi_get_fw_error_records(ph.cPtr(), &cRecords)
	if err := checkStatus(Status(ret)); err != nil {
		return FwErrorRecord{}, err
	}

	goRecords := FwErrorRecord{
		NumErrRecords: uint8(cRecords.num_err_records),
	}

	for i := uint32(0); i < uint32(cRecords.num_err_records); i++ {
		rec := cRecords.err_records[i]
		goRecords.ErrRecords[i] = FwLoadErrorRecord{
			Timestamp: uint64(rec.timestamp),
			VfIdx:     uint32(rec.vf_idx),
			FwID:      uint32(rec.fw_id),
			Status:    GuestFwLoadStatus(rec.status),
		}
	}

	return goRecords, nil
}

// DfcFwHeader type (amdsmi_dfc_fw_header_t)
type DfcFwHeader struct {
	DfcFwVersion      uint32
	DfcFwTotalEntries uint32
	DfcGartWrGuestMin uint32
	DfcGartWrGuestMax uint32
}

// DfcFwWhiteList type (amdsmi_dfc_fw_white_list_t)
type DfcFwWhiteList struct {
	Oldest uint32
	Latest uint32
}

// DfcFwData type (amdsmi_dfc_fw_data_t)
type DfcFwData struct {
	DfcFwType           uint32
	VerificationEnabled uint32
	CustomerOrdinal     uint32
	Reserved            [13]uint32
	WhiteList           [AMDSMI_MAX_WHITE_LIST_ELEMENTS]DfcFwWhiteList
	BlackList           [AMDSMI_MAX_BLACK_LIST_ELEMENTS]uint32
}

// DfcFw type (amdsmi_dfc_fw_t).
type DfcFw struct {
	Header DfcFwHeader
	Data   [AMDSMI_DFC_FW_NUMBER_OF_ENTRIES]DfcFwData
}

// GetDfcFwTable returns the DFC firmware table.
func GetDfcFwTable(ph ProcessorHandle) (DfcFw, error) {
	var cDfcFw C.amdsmi_dfc_fw_t

	ret := C.amdsmi_get_dfc_fw_table(ph.cPtr(), &cDfcFw)
	if err := checkStatus(Status(ret)); err != nil {
		return DfcFw{}, err
	}

	goDfcFw := DfcFw{
		Header: DfcFwHeader{
			DfcFwVersion:      uint32(cDfcFw.header.dfc_fw_version),
			DfcFwTotalEntries: uint32(cDfcFw.header.dfc_fw_total_entries),
			DfcGartWrGuestMin: uint32(cDfcFw.header.dfc_gart_wr_guest_min),
			DfcGartWrGuestMax: uint32(cDfcFw.header.dfc_gart_wr_guest_max),
		},
	}

	n := int(goDfcFw.Header.DfcFwTotalEntries)
	if n > AMDSMI_DFC_FW_NUMBER_OF_ENTRIES {
		n = AMDSMI_DFC_FW_NUMBER_OF_ENTRIES
	}

	for i := 0; i < n; i++ {
		dataEntry := DfcFwData{
			DfcFwType:           uint32(cDfcFw.data[i].dfc_fw_type),
			VerificationEnabled: uint32(cDfcFw.data[i].verification_enabled),
			CustomerOrdinal:     uint32(cDfcFw.data[i].customer_ordinal),
		}

		for j := 0; j < AMDSMI_MAX_WHITE_LIST_ELEMENTS; j++ {
			wl := C.amdsmi_go_dfc_white_list_at(&cDfcFw, C.uint(i), C.uint(j))
			dataEntry.WhiteList[j] = DfcFwWhiteList{
				Oldest: uint32(wl.oldest),
				Latest: uint32(wl.latest),
			}
		}

		for z := 0; z < AMDSMI_MAX_BLACK_LIST_ELEMENTS; z++ {
			dataEntry.BlackList[z] = uint32(C.amdsmi_go_dfc_black_list_at(&cDfcFw, C.uint(i), C.uint(z)))
		}

		goDfcFw.Data[i] = dataEntry
	}

	return goDfcFw, nil
}

// EngineUsage holds GPU engine usage percentages.
type EngineUsage struct {
	GfxActivity uint32
	UmcActivity uint32
	MmActivity  uint32
}

// GetGpuActivity returns current GPU engine usage for the given processor.
func GetGpuActivity(ph ProcessorHandle) (EngineUsage, error) {
	var cEngineUsage C.amdsmi_engine_usage_t
	ret := C.amdsmi_get_gpu_activity(ph.cPtr(), &cEngineUsage)
	if err := checkStatus(Status(ret)); err != nil {
		return EngineUsage{}, err
	}
	return EngineUsage{
		GfxActivity: uint32(cEngineUsage.gfx_activity),
		UmcActivity: uint32(cEngineUsage.umc_activity),
		MmActivity:  uint32(cEngineUsage.mm_activity),
	}, nil
}

// PowerInfo holds current GPU power and voltage (amdsmi_power_info_t).
// Socket power units are platform-specific (W on Linux bare metal, µW on host per API docs).
// Note: CurrentSocketPower, AverageSocketPower and PowerLimit are not supported
// on the host implementation and will always be 0.
type PowerInfo struct {
	SocketPower        uint64
	CurrentSocketPower uint32 // not supported on host, always 0
	AverageSocketPower uint32 // not supported on host, always 0
	GfxVoltage         uint64
	SocVoltage         uint64
	MemVoltage         uint64
	PowerLimit         uint32 // not supported on host, always 0
	UbbPower           uint32
}

// GetPowerInfo returns current power and voltage for the GPU (amdsmi_get_power_info).
func GetPowerInfo(ph ProcessorHandle) (PowerInfo, error) {
	var cPowerInfo C.amdsmi_power_info_t

	ret := C.amdsmi_get_power_info(ph.cPtr(), &cPowerInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return PowerInfo{}, err
	}

	return PowerInfo{
		SocketPower:        uint64(cPowerInfo.socket_power),
		CurrentSocketPower: uint32(cPowerInfo.current_socket_power),
		AverageSocketPower: uint32(cPowerInfo.average_socket_power),
		GfxVoltage:         uint64(cPowerInfo.gfx_voltage),
		SocVoltage:         uint64(cPowerInfo.soc_voltage),
		MemVoltage:         uint64(cPowerInfo.mem_voltage),
		PowerLimit:         uint32(cPowerInfo.power_limit),
		UbbPower:           uint32(cPowerInfo.ubb_power),
	}, nil
}

// IsGpuPowerManagementEnabled reports whether GPU power management is enabled (amdsmi_is_gpu_power_management_enabled)
func IsGpuPowerManagementEnabled(ph ProcessorHandle) (bool, error) {
	var cEnabled C.bool

	ret := C.amdsmi_is_gpu_power_management_enabled(ph.cPtr(), &cEnabled)
	if err := checkStatus(Status(ret)); err != nil {
		return false, err
	}

	return bool(cEnabled), nil
}

// ClkInfo holds clock frequency and state (amdsmi_clk_info_t). Frequencies are in MHz
type ClkInfo struct {
	Clk          uint32
	MinClk       uint32
	MaxClk       uint32
	ClkLocked    bool
	ClkDeepSleep bool
}

// GetClockInfo returns averaged clock measurements for the given clock type (amdsmi_get_clock_info).
func GetClockInfo(ph ProcessorHandle, clkType ClkType) (ClkInfo, error) {
	var cClkInfo C.amdsmi_clk_info_t

	ret := C.amdsmi_get_clock_info(ph.cPtr(), C.amdsmi_clk_type_t(clkType), &cClkInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return ClkInfo{}, err
	}

	return ClkInfo{
		Clk:          uint32(cClkInfo.clk),
		MinClk:       uint32(cClkInfo.min_clk),
		MaxClk:       uint32(cClkInfo.max_clk),
		ClkLocked:    cClkInfo.clk_locked != 0,
		ClkDeepSleep: cClkInfo.clk_deep_sleep != 0,
	}, nil
}

// GetTempMetric returns the temperature in Celsius for the given sensor and metric (amdsmi_get_temp_metric).
func GetTempMetric(ph ProcessorHandle, sensorType TemperatureType, metric TemperatureMetric) (int64, error) {
	var cTemp C.int64_t
	ret := C.amdsmi_get_temp_metric(ph.cPtr(), C.amdsmi_temperature_type_t(sensorType), C.amdsmi_temperature_metric_t(metric), &cTemp)
	if err := checkStatus(Status(ret)); err != nil {
		return 0, err
	}

	return int64(cTemp), nil
}

// MetricUnit mirrors amdsmi_metric_unit_t.
type MetricUnit int32

const (
	AMDSMI_METRIC_UNIT_COUNTER           MetricUnit = 0
	AMDSMI_METRIC_UNIT_UINT              MetricUnit = 1
	AMDSMI_METRIC_UNIT_BOOL              MetricUnit = 2
	AMDSMI_METRIC_UNIT_MHZ               MetricUnit = 3
	AMDSMI_METRIC_UNIT_PERCENT           MetricUnit = 4
	AMDSMI_METRIC_UNIT_MILLIVOLT         MetricUnit = 5
	AMDSMI_METRIC_UNIT_CELSIUS           MetricUnit = 6
	AMDSMI_METRIC_UNIT_WATT              MetricUnit = 7
	AMDSMI_METRIC_UNIT_JOULE             MetricUnit = 8
	AMDSMI_METRIC_UNIT_GBPS              MetricUnit = 9
	AMDSMI_METRIC_UNIT_MBITPS            MetricUnit = 10
	AMDSMI_METRIC_UNIT_PCIE_GEN          MetricUnit = 11
	AMDSMI_METRIC_UNIT_PCIE_LANES        MetricUnit = 12
	AMDSMI_METRIC_UNIT_15_625_MILLIJOULE MetricUnit = 13
	AMDSMI_METRIC_UNIT_UNKNOWN           MetricUnit = 14
)

func (u MetricUnit) String() string {
	switch u {
	case AMDSMI_METRIC_UNIT_COUNTER:
		return "COUNTER"
	case AMDSMI_METRIC_UNIT_UINT:
		return "UINT"
	case AMDSMI_METRIC_UNIT_BOOL:
		return "BOOL"
	case AMDSMI_METRIC_UNIT_MHZ:
		return "MHZ"
	case AMDSMI_METRIC_UNIT_PERCENT:
		return "PERCENT"
	case AMDSMI_METRIC_UNIT_MILLIVOLT:
		return "MILLIVOLT"
	case AMDSMI_METRIC_UNIT_CELSIUS:
		return "CELSIUS"
	case AMDSMI_METRIC_UNIT_WATT:
		return "WATT"
	case AMDSMI_METRIC_UNIT_JOULE:
		return "JOULE"
	case AMDSMI_METRIC_UNIT_GBPS:
		return "GBPS"
	case AMDSMI_METRIC_UNIT_MBITPS:
		return "MBITPS"
	case AMDSMI_METRIC_UNIT_PCIE_GEN:
		return "PCIE_GEN"
	case AMDSMI_METRIC_UNIT_PCIE_LANES:
		return "PCIE_LANES"
	case AMDSMI_METRIC_UNIT_15_625_MILLIJOULE:
		return "15_625_MILLIJOULE"
	case AMDSMI_METRIC_UNIT_UNKNOWN:
		return "UNKNOWN"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int32(u))
	}
}

// MetricCategory mirrors amdsmi_metric_category_t.
type MetricCategory int32

const (
	AMDSMI_METRIC_CATEGORY_ACC_COUNTER         MetricCategory = 0
	AMDSMI_METRIC_CATEGORY_FREQUENCY           MetricCategory = 1
	AMDSMI_METRIC_CATEGORY_ACTIVITY            MetricCategory = 2
	AMDSMI_METRIC_CATEGORY_TEMPERATURE         MetricCategory = 3
	AMDSMI_METRIC_CATEGORY_POWER               MetricCategory = 4
	AMDSMI_METRIC_CATEGORY_ENERGY              MetricCategory = 5
	AMDSMI_METRIC_CATEGORY_THROTTLE            MetricCategory = 6
	AMDSMI_METRIC_CATEGORY_PCIE                MetricCategory = 7
	AMDSMI_METRIC_CATEGORY_STATIC              MetricCategory = 8
	AMDSMI_METRIC_CATEGORY_SYS_ACC_COUNTER     MetricCategory = 9
	AMDSMI_METRIC_CATEGORY_SYS_BASEBOARD_TEMP  MetricCategory = 10
	AMDSMI_METRIC_CATEGORY_SYS_GPUBOARD_TEMP   MetricCategory = 11
	AMDSMI_METRIC_CATEGORY_SYS_BASEBOARD_POWER MetricCategory = 12
	AMDSMI_METRIC_CATEGORY_STATIC_FREQUENCY    MetricCategory = 13
	AMDSMI_METRIC_CATEGORY_STATIC_TEMPERATURE  MetricCategory = 14
	AMDSMI_METRIC_CATEGORY_STATIC_THROTTLE     MetricCategory = 15
	AMDSMI_METRIC_CATEGORY_UNKNOWN             MetricCategory = 16
)

func (c MetricCategory) String() string {
	switch c {
	case AMDSMI_METRIC_CATEGORY_ACC_COUNTER:
		return "ACC_COUNTER"
	case AMDSMI_METRIC_CATEGORY_FREQUENCY:
		return "FREQUENCY"
	case AMDSMI_METRIC_CATEGORY_ACTIVITY:
		return "ACTIVITY"
	case AMDSMI_METRIC_CATEGORY_TEMPERATURE:
		return "TEMPERATURE"
	case AMDSMI_METRIC_CATEGORY_POWER:
		return "POWER"
	case AMDSMI_METRIC_CATEGORY_ENERGY:
		return "ENERGY"
	case AMDSMI_METRIC_CATEGORY_THROTTLE:
		return "THROTTLE"
	case AMDSMI_METRIC_CATEGORY_PCIE:
		return "PCIE"
	case AMDSMI_METRIC_CATEGORY_STATIC:
		return "STATIC"
	case AMDSMI_METRIC_CATEGORY_SYS_ACC_COUNTER:
		return "SYS_ACC_COUNTER"
	case AMDSMI_METRIC_CATEGORY_SYS_BASEBOARD_TEMP:
		return "SYS_BASEBOARD_TEMP"
	case AMDSMI_METRIC_CATEGORY_SYS_GPUBOARD_TEMP:
		return "SYS_GPUBOARD_TEMP"
	case AMDSMI_METRIC_CATEGORY_SYS_BASEBOARD_POWER:
		return "SYS_BASEBOARD_POWER"
	case AMDSMI_METRIC_CATEGORY_STATIC_FREQUENCY:
		return "STATIC_FREQUENCY"
	case AMDSMI_METRIC_CATEGORY_STATIC_TEMPERATURE:
		return "STATIC_TEMPERATURE"
	case AMDSMI_METRIC_CATEGORY_STATIC_THROTTLE:
		return "STATIC_THROTTLE"
	case AMDSMI_METRIC_CATEGORY_UNKNOWN:
		return "UNKNOWN"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int32(c))
	}
}

// MetricName mirrors amdsmi_metric_name_t.
type MetricName int32

const (
	AMDSMI_METRIC_NAME_METRIC_ACC_COUNTER                 MetricName = 0
	AMDSMI_METRIC_NAME_FW_TIMESTAMP                       MetricName = 1
	AMDSMI_METRIC_NAME_CLK_GFX                            MetricName = 2
	AMDSMI_METRIC_NAME_CLK_SOC                            MetricName = 3
	AMDSMI_METRIC_NAME_CLK_MEM                            MetricName = 4
	AMDSMI_METRIC_NAME_CLK_VCLK                           MetricName = 5
	AMDSMI_METRIC_NAME_CLK_DCLK                           MetricName = 6
	AMDSMI_METRIC_NAME_USAGE_GFX                          MetricName = 7
	AMDSMI_METRIC_NAME_USAGE_MEM                          MetricName = 8
	AMDSMI_METRIC_NAME_USAGE_MM                           MetricName = 9
	AMDSMI_METRIC_NAME_USAGE_VCN                          MetricName = 10
	AMDSMI_METRIC_NAME_USAGE_JPEG                         MetricName = 11
	AMDSMI_METRIC_NAME_VOLT_GFX                           MetricName = 12
	AMDSMI_METRIC_NAME_VOLT_SOC                           MetricName = 13
	AMDSMI_METRIC_NAME_VOLT_MEM                           MetricName = 14
	AMDSMI_METRIC_NAME_TEMP_HOTSPOT_CURR                  MetricName = 15
	AMDSMI_METRIC_NAME_TEMP_HOTSPOT_LIMIT                 MetricName = 16
	AMDSMI_METRIC_NAME_TEMP_MEM_CURR                      MetricName = 17
	AMDSMI_METRIC_NAME_TEMP_MEM_LIMIT                     MetricName = 18
	AMDSMI_METRIC_NAME_TEMP_VR_CURR                       MetricName = 19
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN                      MetricName = 20
	AMDSMI_METRIC_NAME_POWER_CURR                         MetricName = 21
	AMDSMI_METRIC_NAME_POWER_LIMIT                        MetricName = 22
	AMDSMI_METRIC_NAME_ENERGY_SOCKET                      MetricName = 23
	AMDSMI_METRIC_NAME_ENERGY_CCD                         MetricName = 24
	AMDSMI_METRIC_NAME_ENERGY_XCD                         MetricName = 25
	AMDSMI_METRIC_NAME_ENERGY_AID                         MetricName = 26
	AMDSMI_METRIC_NAME_ENERGY_MEM                         MetricName = 27
	AMDSMI_METRIC_NAME_THROTTLE_SOCKET_ACTIVE             MetricName = 28
	AMDSMI_METRIC_NAME_THROTTLE_VR_ACTIVE                 MetricName = 29
	AMDSMI_METRIC_NAME_THROTTLE_MEM_ACTIVE                MetricName = 30
	AMDSMI_METRIC_NAME_THROTTLE_PROCHOT_ACTIVE            MetricName = 31
	AMDSMI_METRIC_NAME_THROTTLE_PPT_ACTIVE                MetricName = 32
	AMDSMI_METRIC_NAME_PCIE_BANDWIDTH                     MetricName = 33
	AMDSMI_METRIC_NAME_PCIE_L0_TO_RECOVERY_COUNT          MetricName = 34
	AMDSMI_METRIC_NAME_PCIE_REPLAY_COUNT                  MetricName = 35
	AMDSMI_METRIC_NAME_PCIE_REPLAY_ROLLOVER_COUNT         MetricName = 36
	AMDSMI_METRIC_NAME_PCIE_NAK_SENT_COUNT                MetricName = 37
	AMDSMI_METRIC_NAME_PCIE_NAK_RECEIVED_COUNT            MetricName = 38
	AMDSMI_METRIC_NAME_CLK_GFX_MAX_LIMIT                  MetricName = 39
	AMDSMI_METRIC_NAME_CLK_SOC_MAX_LIMIT                  MetricName = 40
	AMDSMI_METRIC_NAME_CLK_MEM_MAX_LIMIT                  MetricName = 41
	AMDSMI_METRIC_NAME_CLK_VCLK_MAX_LIMIT                 MetricName = 42
	AMDSMI_METRIC_NAME_CLK_DCLK_MAX_LIMIT                 MetricName = 43
	AMDSMI_METRIC_NAME_CLK_GFX_MIN_LIMIT                  MetricName = 44
	AMDSMI_METRIC_NAME_CLK_SOC_MIN_LIMIT                  MetricName = 45
	AMDSMI_METRIC_NAME_CLK_MEM_MIN_LIMIT                  MetricName = 46
	AMDSMI_METRIC_NAME_CLK_VCLK_MIN_LIMIT                 MetricName = 47
	AMDSMI_METRIC_NAME_CLK_DCLK_MIN_LIMIT                 MetricName = 48
	AMDSMI_METRIC_NAME_CLK_GFX_LOCKED                     MetricName = 49
	AMDSMI_METRIC_NAME_CLK_GFX_DS_DISABLED                MetricName = 50
	AMDSMI_METRIC_NAME_CLK_MEM_DS_DISABLED                MetricName = 51
	AMDSMI_METRIC_NAME_CLK_SOC_DS_DISABLED                MetricName = 52
	AMDSMI_METRIC_NAME_CLK_VCLK_DS_DISABLED               MetricName = 53
	AMDSMI_METRIC_NAME_CLK_DCLK_DS_DISABLED               MetricName = 54
	AMDSMI_METRIC_NAME_PCIE_LINK_SPEED                    MetricName = 55
	AMDSMI_METRIC_NAME_PCIE_LINK_WIDTH                    MetricName = 56
	AMDSMI_METRIC_NAME_DRAM_BANDWIDTH                     MetricName = 57
	AMDSMI_METRIC_NAME_MAX_DRAM_BANDWIDTH                 MetricName = 58
	AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_PPT       MetricName = 59
	AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_THM       MetricName = 60
	AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_TOTAL     MetricName = 61
	AMDSMI_METRIC_NAME_GFX_CLK_LOW_UTILIZATION            MetricName = 62
	AMDSMI_METRIC_NAME_INPUT_TELEMETRY_VOLTAGE            MetricName = 63
	AMDSMI_METRIC_NAME_PLDM_VERSION                       MetricName = 64
	AMDSMI_METRIC_NAME_TEMP_XCD                           MetricName = 65
	AMDSMI_METRIC_NAME_TEMP_AID                           MetricName = 66
	AMDSMI_METRIC_NAME_TEMP_HBM                           MetricName = 67
	AMDSMI_METRIC_NAME_SYS_METRIC_ACC_COUNTER             MetricName = 68
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA               MetricName = 69
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FRONT              MetricName = 70
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_BACK               MetricName = 71
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM7               MetricName = 72
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_IBC                MetricName = 73
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_UFPGA              MetricName = 74
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM1               MetricName = 75
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_HSC            MetricName = 76
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_2_3_HSC            MetricName = 77
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_HSC            MetricName = 78
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_6_7_HSC            MetricName = 79
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_0V72_VR       MetricName = 80
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_3V3_VR        MetricName = 81
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR MetricName = 82
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR MetricName = 83
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_0V9_VR     MetricName = 84
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_0V9_VR     MetricName = 85
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_2_3_0V9_VR     MetricName = 86
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_6_7_0V9_VR     MetricName = 87
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR     MetricName = 88
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR     MetricName = 89
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC_HSC                MetricName = 90
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC                    MetricName = 91
	AMDSMI_METRIC_NAME_NODE_TEMP_RETIMER                  MetricName = 92
	AMDSMI_METRIC_NAME_NODE_TEMP_IBC_TEMP                 MetricName = 93
	AMDSMI_METRIC_NAME_NODE_TEMP_IBC_2_TEMP               MetricName = 94
	AMDSMI_METRIC_NAME_NODE_TEMP_VDD18_VR_TEMP            MetricName = 95
	AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_B_VR_TEMP         MetricName = 96
	AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_D_VR_TEMP         MetricName = 97
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD0                 MetricName = 98
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD1                 MetricName = 99
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD2                 MetricName = 100
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD3                 MetricName = 101
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_A                MetricName = 102
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_C                MetricName = 103
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_A              MetricName = 104
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_C              MetricName = 105
	AMDSMI_METRIC_NAME_VR_TEMP_VDD_085_HBM                MetricName = 106
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_B             MetricName = 107
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_D             MetricName = 108
	AMDSMI_METRIC_NAME_VR_TEMP_VDD_USR                    MetricName = 109
	AMDSMI_METRIC_NAME_VR_TEMP_VDDIO_11_E32               MetricName = 110
	AMDSMI_METRIC_NAME_TEMP_MID                           MetricName = 111
	AMDSMI_METRIC_NAME_CLK_FCLK                           MetricName = 112
	AMDSMI_METRIC_NAME_CLK_FCLK_MAX_LIMIT                 MetricName = 113
	AMDSMI_METRIC_NAME_CLK_FCLK_MIN_LIMIT                 MetricName = 114
	AMDSMI_METRIC_NAME_CLK_FCLK_DS_DISABLED               MetricName = 115
	AMDSMI_METRIC_NAME_CLK_LCLK                           MetricName = 116
	AMDSMI_METRIC_NAME_CLK_LCLK_MAX_LIMIT                 MetricName = 117
	AMDSMI_METRIC_NAME_CLK_LCLK_MIN_LIMIT                 MetricName = 118
	AMDSMI_METRIC_NAME_CLK_LCLK_DS_DISABLED               MetricName = 119
	AMDSMI_METRIC_NAME_PCIE_OTHER_END_RECOVERY_COUNT      MetricName = 120
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN_XCD                  MetricName = 121
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN_AID                  MetricName = 122
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN_MID                  MetricName = 123
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN_HBM                  MetricName = 124
	AMDSMI_METRIC_NAME_TEMP_THROTTLE_XCD                  MetricName = 125
	AMDSMI_METRIC_NAME_TEMP_THROTTLE_AID                  MetricName = 126
	AMDSMI_METRIC_NAME_TEMP_THROTTLE_MID                  MetricName = 127
	AMDSMI_METRIC_NAME_TEMP_THROTTLE_HBM                  MetricName = 128
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_X0_TEMP            MetricName = 129
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_X1_TEMP            MetricName = 130
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_HBM_B_TEMP         MetricName = 131
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_HBM_D_TEMP         MetricName = 132
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_04_HBM_B_TEMP      MetricName = 133
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_04_HBM_D_TEMP      MetricName = 134
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_HBM_B_TEMP         MetricName = 135
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_HBM_D_TEMP         MetricName = 136
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_075_HBM_B_TEMP     MetricName = 137
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_075_HBM_D_TEMP     MetricName = 138
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_11_GTA_A_TEMP      MetricName = 139
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_11_GTA_C_TEMP      MetricName = 140
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDAN_075_GTA_A_TEMP     MetricName = 141
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDAN_075_GTA_C_TEMP     MetricName = 142
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_075_UCIE_TEMP      MetricName = 143
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_065_UCIEAA_TEMP    MetricName = 144
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_065_UCIEAM_A_TEMP  MetricName = 145
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_065_UCIEAM_C_TEMP  MetricName = 146
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_SOCIO_A_TEMP       MetricName = 147
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_SOCIO_C_TEMP       MetricName = 148
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDAN_075_TEMP           MetricName = 149
	AMDSMI_METRIC_NAME_SYSTEM_POWER_UBB_POWER             MetricName = 150
	AMDSMI_METRIC_NAME_SYSTEM_POWER_UBB_POWER_THRESHOLD   MetricName = 151
	AMDSMI_METRIC_NAME_UNKNOWN                            MetricName = 152
)

var metricNameStrings = map[MetricName]string{
	AMDSMI_METRIC_NAME_METRIC_ACC_COUNTER:                 "ACC_COUNTER",
	AMDSMI_METRIC_NAME_FW_TIMESTAMP:                       "FW_TIMESTAMP",
	AMDSMI_METRIC_NAME_CLK_GFX:                            "CLK_GFX",
	AMDSMI_METRIC_NAME_CLK_SOC:                            "CLK_SOC",
	AMDSMI_METRIC_NAME_CLK_MEM:                            "CLK_MEM",
	AMDSMI_METRIC_NAME_CLK_VCLK:                           "CLK_VCLK",
	AMDSMI_METRIC_NAME_CLK_DCLK:                           "CLK_DCLK",
	AMDSMI_METRIC_NAME_USAGE_GFX:                          "USAGE_GFX",
	AMDSMI_METRIC_NAME_USAGE_MEM:                          "USAGE_MEM",
	AMDSMI_METRIC_NAME_USAGE_MM:                           "USAGE_MM",
	AMDSMI_METRIC_NAME_USAGE_VCN:                          "USAGE_VCN",
	AMDSMI_METRIC_NAME_USAGE_JPEG:                         "USAGE_JPEG",
	AMDSMI_METRIC_NAME_VOLT_GFX:                           "VOLT_GFX",
	AMDSMI_METRIC_NAME_VOLT_SOC:                           "VOLT_SOC",
	AMDSMI_METRIC_NAME_VOLT_MEM:                           "VOLT_MEM",
	AMDSMI_METRIC_NAME_TEMP_HOTSPOT_CURR:                  "TEMP_HOTSPOT_CURR",
	AMDSMI_METRIC_NAME_TEMP_HOTSPOT_LIMIT:                 "TEMP_HOTSPOT_LIMIT",
	AMDSMI_METRIC_NAME_TEMP_MEM_CURR:                      "TEMP_MEM_CURR",
	AMDSMI_METRIC_NAME_TEMP_MEM_LIMIT:                     "TEMP_MEM_LIMIT",
	AMDSMI_METRIC_NAME_TEMP_VR_CURR:                       "TEMP_VR_CURR",
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN:                      "TEMP_SHUTDOWN",
	AMDSMI_METRIC_NAME_POWER_CURR:                         "POWER_CURR",
	AMDSMI_METRIC_NAME_POWER_LIMIT:                        "POWER_LIMIT",
	AMDSMI_METRIC_NAME_ENERGY_SOCKET:                      "ENERGY_SOCKET",
	AMDSMI_METRIC_NAME_ENERGY_CCD:                         "ENERGY_CCD",
	AMDSMI_METRIC_NAME_ENERGY_XCD:                         "ENERGY_XCD",
	AMDSMI_METRIC_NAME_ENERGY_AID:                         "ENERGY_AID",
	AMDSMI_METRIC_NAME_ENERGY_MEM:                         "ENERGY_MEM",
	AMDSMI_METRIC_NAME_THROTTLE_SOCKET_ACTIVE:             "THROTTLE_SOCKET_ACTIVE",
	AMDSMI_METRIC_NAME_THROTTLE_VR_ACTIVE:                 "THROTTLE_VR_ACTIVE",
	AMDSMI_METRIC_NAME_THROTTLE_MEM_ACTIVE:                "THROTTLE_MEM_ACTIVE",
	AMDSMI_METRIC_NAME_THROTTLE_PROCHOT_ACTIVE:            "THROTTLE_PROCHOT_ACTIVE",
	AMDSMI_METRIC_NAME_THROTTLE_PPT_ACTIVE:                "THROTTLE_PPT_ACTIVE",
	AMDSMI_METRIC_NAME_PCIE_BANDWIDTH:                     "PCIE_BANDWIDTH",
	AMDSMI_METRIC_NAME_PCIE_L0_TO_RECOVERY_COUNT:          "PCIE_L0_TO_RECOVERY_COUNT",
	AMDSMI_METRIC_NAME_PCIE_REPLAY_COUNT:                  "PCIE_REPLAY_COUNT",
	AMDSMI_METRIC_NAME_PCIE_REPLAY_ROLLOVER_COUNT:         "PCIE_REPLAY_ROLLOVER_COUNT",
	AMDSMI_METRIC_NAME_PCIE_NAK_SENT_COUNT:                "PCIE_NAK_SENT_COUNT",
	AMDSMI_METRIC_NAME_PCIE_NAK_RECEIVED_COUNT:            "PCIE_NAK_RECEIVED_COUNT",
	AMDSMI_METRIC_NAME_CLK_GFX_MAX_LIMIT:                  "CLK_GFX_MAX_LIMIT",
	AMDSMI_METRIC_NAME_CLK_SOC_MAX_LIMIT:                  "CLK_SOC_MAX_LIMIT",
	AMDSMI_METRIC_NAME_CLK_MEM_MAX_LIMIT:                  "CLK_MEM_MAX_LIMIT",
	AMDSMI_METRIC_NAME_CLK_VCLK_MAX_LIMIT:                 "CLK_VCLK_MAX_LIMIT",
	AMDSMI_METRIC_NAME_CLK_DCLK_MAX_LIMIT:                 "CLK_DCLK_MAX_LIMIT",
	AMDSMI_METRIC_NAME_CLK_GFX_MIN_LIMIT:                  "CLK_GFX_MIN_LIMIT",
	AMDSMI_METRIC_NAME_CLK_SOC_MIN_LIMIT:                  "CLK_SOC_MIN_LIMIT",
	AMDSMI_METRIC_NAME_CLK_MEM_MIN_LIMIT:                  "CLK_MEM_MIN_LIMIT",
	AMDSMI_METRIC_NAME_CLK_VCLK_MIN_LIMIT:                 "CLK_VCLK_MIN_LIMIT",
	AMDSMI_METRIC_NAME_CLK_DCLK_MIN_LIMIT:                 "CLK_DCLK_MIN_LIMIT",
	AMDSMI_METRIC_NAME_CLK_GFX_LOCKED:                     "CLK_GFX_LOCKED",
	AMDSMI_METRIC_NAME_CLK_GFX_DS_DISABLED:                "CLK_GFX_DS_DISABLED",
	AMDSMI_METRIC_NAME_CLK_MEM_DS_DISABLED:                "CLK_MEM_DS_DISABLED",
	AMDSMI_METRIC_NAME_CLK_SOC_DS_DISABLED:                "CLK_SOC_DS_DISABLED",
	AMDSMI_METRIC_NAME_CLK_VCLK_DS_DISABLED:               "CLK_VCLK_DS_DISABLED",
	AMDSMI_METRIC_NAME_CLK_DCLK_DS_DISABLED:               "CLK_DCLK_DS_DISABLED",
	AMDSMI_METRIC_NAME_PCIE_LINK_SPEED:                    "PCIE_LINK_SPEED",
	AMDSMI_METRIC_NAME_PCIE_LINK_WIDTH:                    "PCIE_LINK_WIDTH",
	AMDSMI_METRIC_NAME_DRAM_BANDWIDTH:                     "DRAM_BANDWIDTH",
	AMDSMI_METRIC_NAME_MAX_DRAM_BANDWIDTH:                 "MAX_DRAM_BANDWIDTH",
	AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_PPT:       "GFX_CLK_BELOW_HOST_LIMIT_PPT",
	AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_THM:       "GFX_CLK_BELOW_HOST_LIMIT_THM",
	AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_TOTAL:     "GFX_CLK_BELOW_HOST_LIMIT_TOTAL",
	AMDSMI_METRIC_NAME_GFX_CLK_LOW_UTILIZATION:            "GFX_CLK_LOW_UTILIZATION",
	AMDSMI_METRIC_NAME_INPUT_TELEMETRY_VOLTAGE:            "INPUT_TELEMETRY_VOLTAGE",
	AMDSMI_METRIC_NAME_PLDM_VERSION:                       "PLDM_VERSION",
	AMDSMI_METRIC_NAME_TEMP_XCD:                           "TEMP_XCD",
	AMDSMI_METRIC_NAME_TEMP_AID:                           "TEMP_AID",
	AMDSMI_METRIC_NAME_TEMP_HBM:                           "TEMP_HBM",
	AMDSMI_METRIC_NAME_SYS_METRIC_ACC_COUNTER:             "SYS_METRIC_ACC_COUNTER",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA:               "SYSTEM_TEMP_UBB_FPGA",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FRONT:              "SYSTEM_TEMP_UBB_FRONT",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_BACK:               "SYSTEM_TEMP_UBB_BACK",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM7:               "SYSTEM_TEMP_UBB_OAM7",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_IBC:                "SYSTEM_TEMP_UBB_IBC",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_UFPGA:              "SYSTEM_TEMP_UBB_UFPGA",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM1:               "SYSTEM_TEMP_UBB_OAM1",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_HSC:            "SYSTEM_TEMP_OAM_0_1_HSC",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_2_3_HSC:            "SYSTEM_TEMP_OAM_2_3_HSC",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_HSC:            "SYSTEM_TEMP_OAM_4_5_HSC",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_6_7_HSC:            "SYSTEM_TEMP_OAM_6_7_HSC",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_0V72_VR:       "SYSTEM_TEMP_UBB_FPGA_0V72_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_3V3_VR:        "SYSTEM_TEMP_UBB_FPGA_3V3_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR: "SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR: "SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_0V9_VR:     "SYSTEM_TEMP_RETIMER_0_1_0V9_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_0V9_VR:     "SYSTEM_TEMP_RETIMER_4_5_0V9_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_2_3_0V9_VR:     "SYSTEM_TEMP_RETIMER_2_3_0V9_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_6_7_0V9_VR:     "SYSTEM_TEMP_RETIMER_6_7_0V9_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR:     "SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR:     "SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC_HSC:                "SYSTEM_TEMP_IBC_HSC",
	AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC:                    "SYSTEM_TEMP_IBC",
	AMDSMI_METRIC_NAME_NODE_TEMP_RETIMER:                  "NODE_TEMP_RETIMER",
	AMDSMI_METRIC_NAME_NODE_TEMP_IBC_TEMP:                 "NODE_TEMP_IBC_TEMP",
	AMDSMI_METRIC_NAME_NODE_TEMP_IBC_2_TEMP:               "NODE_TEMP_IBC_2_TEMP",
	AMDSMI_METRIC_NAME_NODE_TEMP_VDD18_VR_TEMP:            "NODE_TEMP_VDD18_VR_TEMP",
	AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_B_VR_TEMP:         "NODE_TEMP_04_HBM_B_VR_TEMP",
	AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_D_VR_TEMP:         "NODE_TEMP_04_HBM_D_VR_TEMP",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD0:                 "VR_TEMP_VDDCR_VDD0",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD1:                 "VR_TEMP_VDDCR_VDD1",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD2:                 "VR_TEMP_VDDCR_VDD2",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD3:                 "VR_TEMP_VDDCR_VDD3",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_A:                "VR_TEMP_VDDCR_SOC_A",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_C:                "VR_TEMP_VDDCR_SOC_C",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_A:              "VR_TEMP_VDDCR_SOCIO_A",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_C:              "VR_TEMP_VDDCR_SOCIO_C",
	AMDSMI_METRIC_NAME_VR_TEMP_VDD_085_HBM:                "VR_TEMP_VDD_085_HBM",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_B:             "VR_TEMP_VDDCR_11_HBM_B",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_D:             "VR_TEMP_VDDCR_11_HBM_D",
	AMDSMI_METRIC_NAME_VR_TEMP_VDD_USR:                    "VR_TEMP_VDD_USR",
	AMDSMI_METRIC_NAME_VR_TEMP_VDDIO_11_E32:               "VR_TEMP_VDDIO_11_E32",
	AMDSMI_METRIC_NAME_TEMP_MID:                           "TEMP_MID",
	AMDSMI_METRIC_NAME_CLK_FCLK:                           "CLK_FCLK",
	AMDSMI_METRIC_NAME_CLK_FCLK_MAX_LIMIT:                 "CLK_FCLK_MAX_LIMIT",
	AMDSMI_METRIC_NAME_CLK_FCLK_MIN_LIMIT:                 "CLK_FCLK_MIN_LIMIT",
	AMDSMI_METRIC_NAME_CLK_FCLK_DS_DISABLED:               "CLK_FCLK_DS_DISABLED",
	AMDSMI_METRIC_NAME_CLK_LCLK:                           "CLK_LCLK",
	AMDSMI_METRIC_NAME_CLK_LCLK_MAX_LIMIT:                 "CLK_LCLK_MAX_LIMIT",
	AMDSMI_METRIC_NAME_CLK_LCLK_MIN_LIMIT:                 "CLK_LCLK_MIN_LIMIT",
	AMDSMI_METRIC_NAME_CLK_LCLK_DS_DISABLED:               "CLK_LCLK_DS_DISABLED",
	AMDSMI_METRIC_NAME_PCIE_OTHER_END_RECOVERY_COUNT:      "PCIE_OTHER_END_RECOVERY_COUNT",
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN_XCD:                  "TEMP_SHUTDOWN_XCD",
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN_AID:                  "TEMP_SHUTDOWN_AID",
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN_MID:                  "TEMP_SHUTDOWN_MID",
	AMDSMI_METRIC_NAME_TEMP_SHUTDOWN_HBM:                  "TEMP_SHUTDOWN_HBM",
	AMDSMI_METRIC_NAME_TEMP_THROTTLE_XCD:                  "TEMP_THROTTLE_XCD",
	AMDSMI_METRIC_NAME_TEMP_THROTTLE_AID:                  "TEMP_THROTTLE_AID",
	AMDSMI_METRIC_NAME_TEMP_THROTTLE_MID:                  "TEMP_THROTTLE_MID",
	AMDSMI_METRIC_NAME_TEMP_THROTTLE_HBM:                  "TEMP_THROTTLE_HBM",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_X0_TEMP:            "SVI_PLANE_VDDCR_X0_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_X1_TEMP:            "SVI_PLANE_VDDCR_X1_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_HBM_B_TEMP:         "SVI_PLANE_VDDIO_HBM_B_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_HBM_D_TEMP:         "SVI_PLANE_VDDIO_HBM_D_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_04_HBM_B_TEMP:      "SVI_PLANE_VDDIO_04_HBM_B_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_04_HBM_D_TEMP:      "SVI_PLANE_VDDIO_04_HBM_D_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_HBM_B_TEMP:         "SVI_PLANE_VDDCR_HBM_B_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_HBM_D_TEMP:         "SVI_PLANE_VDDCR_HBM_D_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_075_HBM_B_TEMP:     "SVI_PLANE_VDDCR_075_HBM_B_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_075_HBM_D_TEMP:     "SVI_PLANE_VDDCR_075_HBM_D_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_11_GTA_A_TEMP:      "SVI_PLANE_VDDIO_11_GTA_A_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_11_GTA_C_TEMP:      "SVI_PLANE_VDDIO_11_GTA_C_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDAN_075_GTA_A_TEMP:     "SVI_PLANE_VDDAN_075_GTA_A_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDAN_075_GTA_C_TEMP:     "SVI_PLANE_VDDAN_075_GTA_C_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_075_UCIE_TEMP:      "SVI_PLANE_VDDCR_075_UCIE_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_065_UCIEAA_TEMP:    "SVI_PLANE_VDDIO_065_UCIEAA_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_065_UCIEAM_A_TEMP:  "SVI_PLANE_VDDIO_065_UCIEAM_A_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDIO_065_UCIEAM_C_TEMP:  "SVI_PLANE_VDDIO_065_UCIEAM_C_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_SOCIO_A_TEMP:       "SVI_PLANE_VDDCR_SOCIO_A_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDCR_SOCIO_C_TEMP:       "SVI_PLANE_VDDCR_SOCIO_C_TEMP",
	AMDSMI_METRIC_NAME_SVI_PLANE_VDDAN_075_TEMP:           "SVI_PLANE_VDDAN_075_TEMP",
	AMDSMI_METRIC_NAME_SYSTEM_POWER_UBB_POWER:             "SYSTEM_POWER_UBB_POWER",
	AMDSMI_METRIC_NAME_SYSTEM_POWER_UBB_POWER_THRESHOLD:   "SYSTEM_POWER_UBB_POWER_THRESHOLD",
	AMDSMI_METRIC_NAME_UNKNOWN:                            "UNKNOWN",
}

func (n MetricName) String() string {
	if s, ok := metricNameStrings[n]; ok {
		return s
	}
	return fmt.Sprintf("UNKNOWN(%d)", int32(n))
}

// MetricType mirrors amdsmi_metric_type_t. It is a bitmask.
type MetricType uint32

const (
	AMDSMI_METRIC_TYPE_COUNTER MetricType = 1 << 0
	AMDSMI_METRIC_TYPE_CHIPLET MetricType = 1 << 1
	AMDSMI_METRIC_TYPE_INST    MetricType = 1 << 2
	AMDSMI_METRIC_TYPE_ACC     MetricType = 1 << 3
)

// Flags returns the list of MetricType bits set in the flag bitmask.
func (f MetricType) Flags() []MetricType {
	out := []MetricType{}
	for _, b := range []MetricType{
		AMDSMI_METRIC_TYPE_COUNTER,
		AMDSMI_METRIC_TYPE_CHIPLET,
		AMDSMI_METRIC_TYPE_INST,
		AMDSMI_METRIC_TYPE_ACC,
	} {
		if f&b != 0 {
			out = append(out, b)
		}
	}
	return out
}

func (t MetricType) String() string {
	switch t {
	case AMDSMI_METRIC_TYPE_COUNTER:
		return "COUNTER"
	case AMDSMI_METRIC_TYPE_CHIPLET:
		return "CHIPLET"
	case AMDSMI_METRIC_TYPE_INST:
		return "INST"
	case AMDSMI_METRIC_TYPE_ACC:
		return "ACC"
	default:
		return fmt.Sprintf("UNKNOWN(0x%X)", uint32(t))
	}
}

// MetricResGroup mirrors amdsmi_metric_res_group_t.
type MetricResGroup int32

const (
	AMDSMI_METRIC_RES_GROUP_UNKNOWN MetricResGroup = 0
	AMDSMI_METRIC_RES_GROUP_NA      MetricResGroup = 1
	AMDSMI_METRIC_RES_GROUP_GPU     MetricResGroup = 2
	AMDSMI_METRIC_RES_GROUP_XCP     MetricResGroup = 3
	AMDSMI_METRIC_RES_GROUP_AID     MetricResGroup = 4
	AMDSMI_METRIC_RES_GROUP_MID     MetricResGroup = 5
	AMDSMI_METRIC_RES_GROUP_SYSTEM  MetricResGroup = 6
)

func (g MetricResGroup) String() string {
	switch g {
	case AMDSMI_METRIC_RES_GROUP_UNKNOWN:
		return "UNKNOWN"
	case AMDSMI_METRIC_RES_GROUP_NA:
		return "NA"
	case AMDSMI_METRIC_RES_GROUP_GPU:
		return "GPU"
	case AMDSMI_METRIC_RES_GROUP_XCP:
		return "XCP"
	case AMDSMI_METRIC_RES_GROUP_AID:
		return "AID"
	case AMDSMI_METRIC_RES_GROUP_MID:
		return "MID"
	case AMDSMI_METRIC_RES_GROUP_SYSTEM:
		return "SYSTEM"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int32(g))
	}
}

// MetricResSubgroup mirrors amdsmi_metric_res_subgroup_t.
type MetricResSubgroup int32

const (
	AMDSMI_METRIC_RES_SUBGROUP_UNKNOWN   MetricResSubgroup = 0
	AMDSMI_METRIC_RES_SUBGROUP_NA        MetricResSubgroup = 1
	AMDSMI_METRIC_RES_SUBGROUP_XCC       MetricResSubgroup = 2
	AMDSMI_METRIC_RES_SUBGROUP_ENGINE    MetricResSubgroup = 3
	AMDSMI_METRIC_RES_SUBGROUP_HBM       MetricResSubgroup = 4
	AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD MetricResSubgroup = 5
	AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD  MetricResSubgroup = 6
)

func (g MetricResSubgroup) String() string {
	switch g {
	case AMDSMI_METRIC_RES_SUBGROUP_UNKNOWN:
		return "UNKNOWN"
	case AMDSMI_METRIC_RES_SUBGROUP_NA:
		return "NA"
	case AMDSMI_METRIC_RES_SUBGROUP_XCC:
		return "XCC"
	case AMDSMI_METRIC_RES_SUBGROUP_ENGINE:
		return "ENGINE"
	case AMDSMI_METRIC_RES_SUBGROUP_HBM:
		return "HBM"
	case AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD:
		return "BASEBOARD"
	case AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD:
		return "GPUBOARD"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int32(g))
	}
}

// Metric mirrors amdsmi_metric_t.
type Metric struct {
	Unit        MetricUnit
	Name        MetricName
	Category    MetricCategory
	Flags       MetricType // bitmask of AMDSMI_METRIC_TYPE_* values; use Flags.Flags() for a list
	VfMask      uint32     // bitmask of VFs + PF this metric applies to
	Val         uint64
	ResGroup    MetricResGroup
	ResSubgroup MetricResSubgroup
	ResInstance uint32
}

// GetGpuMetrics retrieves the list of GPU metrics for a processor. It mirrors
// amdsmi_get_gpu_metrics / the Python amdsmi_get_gpu_metrics helper; up to
// AMDSMI_MAX_NUM_METRICS entries can be returned in a single call.
func GetGpuMetrics(ph ProcessorHandle) ([]Metric, error) {
	metricsSize := C.uint32_t(AMDSMI_MAX_NUM_METRICS)
	cMetrics := make([]C.amdsmi_metric_t, AMDSMI_MAX_NUM_METRICS)

	ret := C.amdsmi_get_gpu_metrics(ph.cPtr(), &metricsSize, &cMetrics[0])
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}

	n := uint32(metricsSize)
	if n > AMDSMI_MAX_NUM_METRICS {
		n = AMDSMI_MAX_NUM_METRICS
	}

	out := make([]Metric, n)
	for i := uint32(0); i < n; i++ {
		m := &cMetrics[i]
		out[i] = Metric{
			Unit:        MetricUnit(m.unit),
			Name:        MetricName(m.name),
			Category:    MetricCategory(m.category),
			Flags:       MetricType(m.flags),
			VfMask:      uint32(m.vf_mask),
			Val:         uint64(m.val),
			ResGroup:    MetricResGroup(m.res_group),
			ResSubgroup: MetricResSubgroup(m.res_subgroup),
			ResInstance: uint32(m.res_instance),
		}
	}
	return out, nil
}

type NpsCaps struct {
	Nps1Cap bool
	Nps2Cap bool
	Nps4Cap bool
	Nps8Cap bool
}

func (n NpsCaps) Supported() []MemoryPartitionType {
	var caps []MemoryPartitionType
	if n.Nps1Cap {
		caps = append(caps, AMDSMI_MEMORY_PARTITION_NPS1)
	}
	if n.Nps2Cap {
		caps = append(caps, AMDSMI_MEMORY_PARTITION_NPS2)
	}
	if n.Nps4Cap {
		caps = append(caps, AMDSMI_MEMORY_PARTITION_NPS4)
	}
	if n.Nps8Cap {
		caps = append(caps, AMDSMI_MEMORY_PARTITION_NPS8)
	}
	return caps
}

func (n NpsCaps) String() string {
	return fmt.Sprintf("%v", n.Supported())
}

type NumaRange struct {
	MemoryType VramType
	Start      uint64
	End        uint64
}

type MemoryPartitionConfig struct {
	PartitionCaps NpsCaps
	Mode          MemoryPartitionType
	NumNumaRanges uint32
	NumaRanges    [AMDSMI_MAX_NUM_NUMA_NODES]NumaRange
}

func npsCapsFromUnion(u C.amdsmi_nps_caps_t) NpsCaps {
	mask := *(*uint32)(unsafe.Pointer(&u))
	return NpsCaps{
		Nps1Cap: mask&(1<<0) != 0,
		Nps2Cap: mask&(1<<1) != 0,
		Nps4Cap: mask&(1<<2) != 0,
		Nps8Cap: mask&(1<<3) != 0,
	}
}

func GetGpuMemoryPartitionConfig(ph ProcessorHandle) (MemoryPartitionConfig, error) {
	var cConfig C.amdsmi_memory_partition_config_t

	ret := C.amdsmi_get_gpu_memory_partition_config(ph.cPtr(), &cConfig)
	if err := checkStatus(Status(ret)); err != nil {
		return MemoryPartitionConfig{}, err
	}

	config := MemoryPartitionConfig{
		PartitionCaps: npsCapsFromUnion(cConfig.partition_caps),
		Mode:          MemoryPartitionType(cConfig.mp_mode),
		NumNumaRanges: uint32(cConfig.num_numa_ranges),
	}

	for i := uint32(0); i < config.NumNumaRanges && i < AMDSMI_MAX_NUM_NUMA_NODES; i++ {
		config.NumaRanges[i] = NumaRange{
			MemoryType: VramType(cConfig.numa_range[i].memory_type),
			Start:      uint64(cConfig.numa_range[i].start),
			End:        uint64(cConfig.numa_range[i].end),
		}
	}

	return config, nil
}

func SetGpuMemoryPartitionMode(ph ProcessorHandle, mode MemoryPartitionType) error {
	ret := C.amdsmi_set_gpu_memory_partition_mode(ph.cPtr(), C.amdsmi_memory_partition_type_t(mode))
	return checkStatus(Status(ret))
}

type AcceleratorPartitionResourceProfile struct {
	ProfileIndex               uint32
	ResourceType               AcceleratorPartitionResourceType
	PartitionResource          uint32
	NumPartitionsShareResource uint32
}

type AcceleratorPartitionProfile struct {
	ProfileType   AcceleratorPartitionType
	NumPartitions uint32
	MemoryCaps    NpsCaps
	ProfileIndex  uint32
	NumResources  uint32
	Resources     [][]uint32
}

type AcceleratorPartitionProfileConfig struct {
	NumProfiles         uint32
	NumResourceProfiles uint32
	ResourceProfiles    [AMDSMI_MAX_CP_PROFILE_RESOURCES]AcceleratorPartitionResourceProfile
	DefaultProfileIndex uint32
	Profiles            [AMDSMI_MAX_ACCELERATOR_PROFILE]AcceleratorPartitionProfile
}

func GetGpuAcceleratorPartitionProfileConfig(ph ProcessorHandle) (AcceleratorPartitionProfileConfig, error) {
	var cConfig C.amdsmi_accelerator_partition_profile_config_t

	ret := C.amdsmi_get_gpu_accelerator_partition_profile_config(ph.cPtr(), &cConfig)
	if err := checkStatus(Status(ret)); err != nil {
		return AcceleratorPartitionProfileConfig{}, err
	}

	config := AcceleratorPartitionProfileConfig{
		NumProfiles:         uint32(cConfig.num_profiles),
		NumResourceProfiles: uint32(cConfig.num_resource_profiles),
		DefaultProfileIndex: uint32(cConfig.default_profile_index),
	}

	for i := uint32(0); i < config.NumResourceProfiles && i < AMDSMI_MAX_CP_PROFILE_RESOURCES; i++ {
		config.ResourceProfiles[i] = AcceleratorPartitionResourceProfile{
			ProfileIndex:               uint32(cConfig.resource_profiles[i].profile_index),
			ResourceType:               AcceleratorPartitionResourceType(cConfig.resource_profiles[i].resource_type),
			PartitionResource:          uint32(cConfig.resource_profiles[i].partition_resource),
			NumPartitionsShareResource: uint32(cConfig.resource_profiles[i].num_partitions_share_resource),
		}
	}

	for i := uint32(0); i < config.NumProfiles && i < AMDSMI_MAX_ACCELERATOR_PROFILE; i++ {
		numPart := uint32(cConfig.profiles[i].num_partitions)
		numRes := uint32(cConfig.profiles[i].num_resources)
		resources := make([][]uint32, numPart)
		for p := uint32(0); p < numPart && p < AMDSMI_MAX_ACCELERATOR_PARTITIONS; p++ {
			row := make([]uint32, numRes)
			for r := uint32(0); r < numRes && r < AMDSMI_MAX_CP_PROFILE_RESOURCES; r++ {
				row[r] = uint32(cConfig.profiles[i].resources[p][r])
			}
			resources[p] = row
		}
		config.Profiles[i] = AcceleratorPartitionProfile{
			ProfileType:   AcceleratorPartitionType(cConfig.profiles[i].profile_type),
			NumPartitions: numPart,
			MemoryCaps:    npsCapsFromUnion(cConfig.profiles[i].memory_caps),
			ProfileIndex:  uint32(cConfig.profiles[i].profile_index),
			NumResources:  numRes,
			Resources:     resources,
		}
	}

	return config, nil
}

func GetGpuAcceleratorPartitionProfile(ph ProcessorHandle) (AcceleratorPartitionProfile, []uint32, error) {
	var cProfile C.amdsmi_accelerator_partition_profile_t
	var cPartitionIDs [AMDSMI_MAX_ACCELERATOR_PARTITIONS]C.uint32_t

	ret := C.amdsmi_get_gpu_accelerator_partition_profile(ph.cPtr(), &cProfile, &cPartitionIDs[0])
	if err := checkStatus(Status(ret)); err != nil {
		return AcceleratorPartitionProfile{}, nil, err
	}

	numPart := uint32(cProfile.num_partitions)
	numRes := uint32(cProfile.num_resources)
	resources := make([][]uint32, numPart)
	for p := uint32(0); p < numPart && p < AMDSMI_MAX_ACCELERATOR_PARTITIONS; p++ {
		row := make([]uint32, numRes)
		for r := uint32(0); r < numRes && r < AMDSMI_MAX_CP_PROFILE_RESOURCES; r++ {
			row[r] = uint32(cProfile.resources[p][r])
		}
		resources[p] = row
	}
	profile := AcceleratorPartitionProfile{
		ProfileType:   AcceleratorPartitionType(cProfile.profile_type),
		NumPartitions: numPart,
		MemoryCaps:    npsCapsFromUnion(cProfile.memory_caps),
		ProfileIndex:  uint32(cProfile.profile_index),
		NumResources:  numRes,
		Resources:     resources,
	}

	partitionIDs := make([]uint32, profile.NumPartitions)
	for i := uint32(0); i < profile.NumPartitions && i < AMDSMI_MAX_ACCELERATOR_PARTITIONS; i++ {
		partitionIDs[i] = uint32(cPartitionIDs[i])
	}

	return profile, partitionIDs, nil
}

func SetGpuAcceleratorPartitionProfile(ph ProcessorHandle, profileIndex uint32) error {
	ret := C.amdsmi_set_gpu_accelerator_partition_profile(ph.cPtr(), C.uint32_t(profileIndex))
	return checkStatus(Status(ret))
}

// VfMode bitmask values (amdsmi_vf_mode_t).
type VfMode uint32

const (
	AMDSMI_VF_MODE_1   VfMode = (1 << 1)
	AMDSMI_VF_MODE_2   VfMode = (1 << 2)
	AMDSMI_VF_MODE_4   VfMode = (1 << 4)
	AMDSMI_VF_MODE_8   VfMode = (1 << 8)
	AMDSMI_VF_MODE_ALL VfMode = AMDSMI_VF_MODE_1 | AMDSMI_VF_MODE_2 | AMDSMI_VF_MODE_4 | AMDSMI_VF_MODE_8
)

func (v VfMode) String() string {
	switch v {
	case AMDSMI_VF_MODE_1:
		return "VF_MODE_1"
	case AMDSMI_VF_MODE_2:
		return "VF_MODE_2"
	case AMDSMI_VF_MODE_4:
		return "VF_MODE_4"
	case AMDSMI_VF_MODE_8:
		return "VF_MODE_8"
	default:
		return fmt.Sprintf("VF_MODE_UNKNOWN(0x%X)", uint32(v))
	}
}

func formatVfMask(vfMode uint32) []VfMode {
	var supported []VfMode
	if vfMode&uint32(AMDSMI_VF_MODE_1) != 0 {
		supported = append(supported, AMDSMI_VF_MODE_1)
	}
	if vfMode&uint32(AMDSMI_VF_MODE_2) != 0 {
		supported = append(supported, AMDSMI_VF_MODE_2)
	}
	if vfMode&uint32(AMDSMI_VF_MODE_4) != 0 {
		supported = append(supported, AMDSMI_VF_MODE_4)
	}
	if vfMode&uint32(AMDSMI_VF_MODE_8) != 0 {
		supported = append(supported, AMDSMI_VF_MODE_8)
	}
	return supported
}

// AcceleratorPartitionProfileGlobal extends a partition profile with VF mode support.
type AcceleratorPartitionProfileGlobal struct {
	Profile AcceleratorPartitionProfile
	VfMode  []VfMode
}

// AcceleratorPartitionProfileConfigGlobal holds the global accelerator partition profile configuration.
type AcceleratorPartitionProfileConfigGlobal struct {
	NumProfiles         uint32
	NumResourceProfiles uint32
	ResourceProfiles    [AMDSMI_MAX_CP_PROFILE_RESOURCES]AcceleratorPartitionResourceProfile
	DefaultProfileIndex uint32
	Profiles            [AMDSMI_MAX_ACCELERATOR_PROFILE]AcceleratorPartitionProfileGlobal
}

// GetGpuAcceleratorPartitionProfileConfigGlobal returns the global accelerator partition
// profile configuration for the given processor, including per-profile VF mode bitmasks.
func GetGpuAcceleratorPartitionProfileConfigGlobal(ph ProcessorHandle) (AcceleratorPartitionProfileConfigGlobal, error) {
	var cConfig C.amdsmi_accelerator_partition_profile_config_global_t

	ret := C.amdsmi_get_gpu_accelerator_partition_profile_config_global(ph.cPtr(), &cConfig)
	if err := checkStatus(Status(ret)); err != nil {
		return AcceleratorPartitionProfileConfigGlobal{}, err
	}

	config := AcceleratorPartitionProfileConfigGlobal{
		NumProfiles:         uint32(cConfig.num_profiles),
		NumResourceProfiles: uint32(cConfig.num_resource_profiles),
		DefaultProfileIndex: uint32(cConfig.default_profile_index),
	}

	for i := uint32(0); i < config.NumResourceProfiles && i < AMDSMI_MAX_CP_PROFILE_RESOURCES; i++ {
		config.ResourceProfiles[i] = AcceleratorPartitionResourceProfile{
			ProfileIndex:               uint32(cConfig.resource_profiles[i].profile_index),
			ResourceType:               AcceleratorPartitionResourceType(cConfig.resource_profiles[i].resource_type),
			PartitionResource:          uint32(cConfig.resource_profiles[i].partition_resource),
			NumPartitionsShareResource: uint32(cConfig.resource_profiles[i].num_partitions_share_resource),
		}
	}

	for i := uint32(0); i < config.NumProfiles && i < AMDSMI_MAX_ACCELERATOR_PROFILE; i++ {
		resources := make([][]uint32, cConfig.profiles[i].profile.num_partitions)
		for p := uint32(0); p < uint32(cConfig.profiles[i].profile.num_partitions) && p < AMDSMI_MAX_ACCELERATOR_PARTITIONS; p++ {
			row := make([]uint32, cConfig.profiles[i].profile.num_resources)
			for r := uint32(0); r < uint32(cConfig.profiles[i].profile.num_resources) && r < AMDSMI_MAX_CP_PROFILE_RESOURCES; r++ {
				row[r] = uint32(cConfig.profiles[i].profile.resources[p][r])
			}
			resources[p] = row
		}
		config.Profiles[i] = AcceleratorPartitionProfileGlobal{
			Profile: AcceleratorPartitionProfile{
				ProfileType:   AcceleratorPartitionType(cConfig.profiles[i].profile.profile_type),
				NumPartitions: uint32(cConfig.profiles[i].profile.num_partitions),
				MemoryCaps:    npsCapsFromUnion(cConfig.profiles[i].profile.memory_caps),
				ProfileIndex:  uint32(cConfig.profiles[i].profile.profile_index),
				NumResources:  uint32(cConfig.profiles[i].profile.num_resources),
				Resources:     resources,
			},
			VfMode: formatVfMask(uint32(cConfig.profiles[i].vf_mode)),
		}
	}

	return config, nil
}

// LinkType represents the type of link between processors (amdsmi_link_type_t).
type LinkType int32

const (
	AMDSMI_LINK_TYPE_INTERNAL       LinkType = 0
	AMDSMI_LINK_TYPE_PCIE           LinkType = 1
	AMDSMI_LINK_TYPE_XGMI           LinkType = 2
	AMDSMI_LINK_TYPE_NOT_APPLICABLE LinkType = 3
	AMDSMI_LINK_TYPE_UNKNOWN        LinkType = 4
	// AMDSMI_LINK_TYPE_NUMA: same NUMA node, different PCIe switch (NIC-to-GPU only).
	AMDSMI_LINK_TYPE_NUMA LinkType = 5
	// AMDSMI_LINK_TYPE_XNUMA: different NUMA nodes (NIC-to-GPU only).
	AMDSMI_LINK_TYPE_XNUMA LinkType = 6
)

func (lt LinkType) String() string {
	switch lt {
	case AMDSMI_LINK_TYPE_INTERNAL:
		return "INTERNAL"
	case AMDSMI_LINK_TYPE_PCIE:
		return "PCIE"
	case AMDSMI_LINK_TYPE_XGMI:
		return "XGMI"
	case AMDSMI_LINK_TYPE_NOT_APPLICABLE:
		return "NOT_APPLICABLE"
	case AMDSMI_LINK_TYPE_UNKNOWN:
		return "UNKNOWN"
	case AMDSMI_LINK_TYPE_NUMA:
		return "NUMA"
	case AMDSMI_LINK_TYPE_XNUMA:
		return "XNUMA"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(lt))
	}
}

// LinkStatus represents the HW status of a link (amdsmi_link_status_t).
type LinkStatus int32

const (
	AMDSMI_LINK_STATUS_ENABLED  LinkStatus = 0
	AMDSMI_LINK_STATUS_DISABLED LinkStatus = 1
	AMDSMI_LINK_STATUS_INACTIVE LinkStatus = 2
	AMDSMI_LINK_STATUS_ERROR    LinkStatus = 3
)

func (ls LinkStatus) String() string {
	switch ls {
	case AMDSMI_LINK_STATUS_ENABLED:
		return "ENABLED"
	case AMDSMI_LINK_STATUS_DISABLED:
		return "DISABLED"
	case AMDSMI_LINK_STATUS_INACTIVE:
		return "INACTIVE"
	case AMDSMI_LINK_STATUS_ERROR:
		return "ERROR"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(ls))
	}
}

// LinkTopology holds link topology information between two processors.
type LinkTopology struct {
	Weight     uint64
	LinkStatus LinkStatus
	LinkType   LinkType
	NumHops    uint8
	FbSharing  uint8
}

// LinkMetricInfo holds per-link metric data.
type LinkMetricInfo struct {
	Bdf          Bdf
	BitRate      uint32
	MaxBandwidth uint32
	LinkType     LinkType
	Read         uint64
	Write        uint64
	LinkStatus   LinkStatus
}

// LinkMetrics holds all link metrics for a processor.
type LinkMetrics struct {
	NumLinks uint32
	Links    []LinkMetricInfo
}

// GetLinkMetrics returns link metrics for the given processor.
func GetLinkMetrics(ph ProcessorHandle) (LinkMetrics, error) {
	var cInfo C.amdsmi_link_metrics_t
	ret := C.amdsmi_get_link_metrics(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return LinkMetrics{}, err
	}

	n := uint32(cInfo.num_links)
	if n > AMDSMI_MAX_NUM_XGMI_PHYSICAL_LINK {
		n = AMDSMI_MAX_NUM_XGMI_PHYSICAL_LINK
	}

	links := make([]LinkMetricInfo, n)
	for i := uint32(0); i < n; i++ {
		bdfVal := *(*Bdf)(unsafe.Pointer(&cInfo.links[i].bdf))
		links[i] = LinkMetricInfo{
			Bdf:          bdfVal,
			BitRate:      uint32(cInfo.links[i].bit_rate),
			MaxBandwidth: uint32(cInfo.links[i].max_bandwidth),
			LinkType:     LinkType(cInfo.links[i].link_type),
			Read:         uint64(cInfo.links[i].read),
			Write:        uint64(cInfo.links[i].write),
			LinkStatus:   LinkStatus(cInfo.links[i].link_status),
		}
	}

	return LinkMetrics{
		NumLinks: n,
		Links:    links,
	}, nil
}

// TopologyNearest holds the nearest GPU topology information.
type TopologyNearest struct {
	Count         uint32
	ProcessorList []ProcessorHandle
}

// GetLinkTopologyNearest returns the nearest GPUs for the given link type.
func GetLinkTopologyNearest(ph ProcessorHandle, linkType LinkType) (TopologyNearest, error) {
	var cInfo C.amdsmi_topology_nearest_t
	ret := C.amdsmi_get_link_topology_nearest(ph.cPtr(), C.amdsmi_link_type_t(linkType), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return TopologyNearest{}, err
	}
	n := uint32(cInfo.count)
	if n > AMDSMI_MAX_DEVICES {
		n = AMDSMI_MAX_DEVICES
	}
	handles := make([]ProcessorHandle, n)
	for i := uint32(0); i < n; i++ {
		handles[i] = ProcessorHandle{raw: uintptr(cInfo.processor_list[i])}
	}
	return TopologyNearest{
		Count:         n,
		ProcessorList: handles,
	}, nil
}

// P2pCapability holds IO link P2P capability information.
type P2pCapability struct {
	IsIolinkCoherent      uint8
	IsIolinkAtomics32bit  uint8
	IsIolinkAtomics64bit  uint8
	IsIolinkDma           uint8
	IsIolinkBiDirectional uint8
}

// TopoGetP2pStatus returns the connection type and P2P capabilities between two GPUs.
func TopoGetP2pStatus(phSrc, phDst ProcessorHandle) (LinkType, P2pCapability, error) {
	var cLinkType C.amdsmi_link_type_t
	var cCap C.amdsmi_p2p_capability_t
	ret := C.amdsmi_topo_get_p2p_status(phSrc.cPtr(), phDst.cPtr(), &cLinkType, &cCap)
	if err := checkStatus(Status(ret)); err != nil {
		return AMDSMI_LINK_TYPE_UNKNOWN, P2pCapability{}, err
	}
	return LinkType(cLinkType), P2pCapability{
		IsIolinkCoherent:      uint8(cCap.is_iolink_coherent),
		IsIolinkAtomics32bit:  uint8(cCap.is_iolink_atomics_32bit),
		IsIolinkAtomics64bit:  uint8(cCap.is_iolink_atomics_64bit),
		IsIolinkDma:           uint8(cCap.is_iolink_dma),
		IsIolinkBiDirectional: uint8(cCap.is_iolink_bi_directional),
	}, nil
}

// TopoGetNumaNodeNumber returns the NUMA CPU node number for the given processor.
func TopoGetNumaNodeNumber(ph ProcessorHandle) (uint32, error) {
	var cNumaNode C.uint32_t
	ret := C.amdsmi_topo_get_numa_node_number(ph.cPtr(), &cNumaNode)
	if err := checkStatus(Status(ret)); err != nil {
		return 0, err
	}
	return uint32(cNumaNode), nil
}

// TopoGetLinkType returns the hop count and link type between two processors.
//
// For GPU-to-GPU queries type is one of INTERNAL, PCIE, XGMI, NOT_APPLICABLE,
// or UNKNOWN, and hops carries the hop count.
//
// For NIC-to-GPU queries type is one of PCIE, NUMA, XNUMA, or UNKNOWN.
// The hop count is not meaningful for NIC queries; hops is set to ^uint64(0)
// (UINT64_MAX) in that case.
func TopoGetLinkType(phSrc, phDst ProcessorHandle) (hops uint64, linkType LinkType, err error) {
	var cHops C.uint64_t
	var cType C.amdsmi_link_type_t
	ret := C.amdsmi_topo_get_link_type(phSrc.cPtr(), phDst.cPtr(), &cHops, &cType)
	if err = checkStatus(Status(ret)); err != nil {
		return 0, AMDSMI_LINK_TYPE_UNKNOWN, err
	}
	return uint64(cHops), LinkType(cType), nil
}

// GetLinkTopology returns link topology info between two processors.
func GetLinkTopology(phSrc, phDst ProcessorHandle) (LinkTopology, error) {
	var cLinkTopology C.amdsmi_link_topology_t
	ret := C.amdsmi_get_link_topology(phSrc.cPtr(), phDst.cPtr(), &cLinkTopology)
	if err := checkStatus(Status(ret)); err != nil {
		return LinkTopology{}, err
	}
	return LinkTopology{
		Weight:     uint64(cLinkTopology.weight),
		LinkStatus: LinkStatus(cLinkTopology.link_status),
		LinkType:   LinkType(cLinkTopology.link_type),
		NumHops:    uint8(cLinkTopology.num_hops),
		FbSharing:  uint8(cLinkTopology.fb_sharing),
	}, nil
}

// XgmiFbSharingCaps holds XGMI framebuffer sharing capabilities.
type XgmiFbSharingCaps struct {
	ModeCustomCap uint32
	Mode1Cap      uint32
	Mode2Cap      uint32
	Mode4Cap      uint32
	Mode8Cap      uint32
}

// GetXgmiFbSharingCaps returns the XGMI framebuffer sharing capabilities for the given processor.
func GetXgmiFbSharingCaps(ph ProcessorHandle) (XgmiFbSharingCaps, error) {
	var cCaps C.amdsmi_xgmi_fb_sharing_caps_t
	ret := C.amdsmi_get_xgmi_fb_sharing_caps(ph.cPtr(), &cCaps)
	if err := checkStatus(Status(ret)); err != nil {
		return XgmiFbSharingCaps{}, err
	}

	mask := *(*uint32)(unsafe.Pointer(&cCaps))
	return XgmiFbSharingCaps{
		ModeCustomCap: (mask >> 0) & 1,
		Mode1Cap:      (mask >> 1) & 1,
		Mode2Cap:      (mask >> 2) & 1,
		Mode4Cap:      (mask >> 3) & 1,
		Mode8Cap:      (mask >> 4) & 1,
	}, nil
}

// XgmiFbSharingMode represents XGMI framebuffer sharing modes (amdsmi_xgmi_fb_sharing_mode_t).
type XgmiFbSharingMode int32

const (
	AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM  XgmiFbSharingMode = 0
	AMDSMI_XGMI_FB_SHARING_MODE_1       XgmiFbSharingMode = 1
	AMDSMI_XGMI_FB_SHARING_MODE_2       XgmiFbSharingMode = 2
	AMDSMI_XGMI_FB_SHARING_MODE_4       XgmiFbSharingMode = 4
	AMDSMI_XGMI_FB_SHARING_MODE_8       XgmiFbSharingMode = 8
	AMDSMI_XGMI_FB_SHARING_MODE_UNKNOWN XgmiFbSharingMode = 0x7FFFFFFF
)

func (m XgmiFbSharingMode) String() string {
	switch m {
	case AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM:
		return "CUSTOM"
	case AMDSMI_XGMI_FB_SHARING_MODE_1:
		return "MODE_1"
	case AMDSMI_XGMI_FB_SHARING_MODE_2:
		return "MODE_2"
	case AMDSMI_XGMI_FB_SHARING_MODE_4:
		return "MODE_4"
	case AMDSMI_XGMI_FB_SHARING_MODE_8:
		return "MODE_8"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(m))
	}
}

// GetXgmiFbSharingModeInfo returns XGMI framebuffer sharing information between two GPUs
// for the specified sharing mode. Returns a uint8 indicating whether FB is shared.
func GetXgmiFbSharingModeInfo(phSrc, phDst ProcessorHandle, mode XgmiFbSharingMode) (uint8, error) {
	var cFbSharing C.uint8_t
	ret := C.amdsmi_get_xgmi_fb_sharing_mode_info(phSrc.cPtr(), phDst.cPtr(),
		C.amdsmi_xgmi_fb_sharing_mode_t(mode), &cFbSharing)
	if err := checkStatus(Status(ret)); err != nil {
		return 0, err
	}
	return uint8(cFbSharing), nil
}

// SetXgmiFbSharingMode sets the XGMI framebuffer sharing mode for the given processor.
func SetXgmiFbSharingMode(ph ProcessorHandle, mode XgmiFbSharingMode) error {
	ret := C.amdsmi_set_xgmi_fb_sharing_mode(ph.cPtr(), C.amdsmi_xgmi_fb_sharing_mode_t(mode))
	return checkStatus(Status(ret))
}

// SetXgmiFbSharingModeV2 sets the XGMI framebuffer sharing mode for a list of processors.
// For auto modes (MODE_1, MODE_2, etc.) only the first processor in the list is used.
// For CUSTOM mode all processors in the list must be on the same NUMA node.
func SetXgmiFbSharingModeV2(processorList []ProcessorHandle, mode XgmiFbSharingMode) error {
	var numProcessors uint32
	if mode == AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM {
		numProcessors = uint32(len(processorList))
	} else {
		numProcessors = 1
	}

	cHandles := make([]C.amdsmi_processor_handle, numProcessors)
	for i := uint32(0); i < numProcessors; i++ {
		cHandles[i] = processorList[i].cPtr()
	}

	ret := C.amdsmi_set_xgmi_fb_sharing_mode_v2(&cHandles[0],
		C.uint32_t(numProcessors), C.amdsmi_xgmi_fb_sharing_mode_t(mode))
	return checkStatus(Status(ret))
}

// DpmPolicyEntry holds a single DPM policy entry.
type DpmPolicyEntry struct {
	PolicyID          uint32
	PolicyDescription string
}

// DpmPolicy holds the DPM policy configuration.
type DpmPolicy struct {
	NumSupported uint32
	Current      uint32
	Policies     []DpmPolicyEntry
}

// GetSocPstate returns the soc pstate policy for the given processor.
func GetSocPstate(ph ProcessorHandle) (DpmPolicy, error) {
	var cPolicy C.amdsmi_dpm_policy_t
	ret := C.amdsmi_get_soc_pstate(ph.cPtr(), &cPolicy)
	if err := checkStatus(Status(ret)); err != nil {
		return DpmPolicy{}, err
	}

	n := uint32(cPolicy.num_supported)
	if n > AMDSMI_MAX_NUM_PM_POLICIES {
		n = AMDSMI_MAX_NUM_PM_POLICIES
	}

	entries := make([]DpmPolicyEntry, n)
	for i := uint32(0); i < n; i++ {
		entries[i] = DpmPolicyEntry{
			PolicyID:          uint32(cPolicy.policies[i].policy_id),
			PolicyDescription: C.GoString(&cPolicy.policies[i].policy_description[0]),
		}
	}

	return DpmPolicy{
		NumSupported: n,
		Current:      uint32(cPolicy.current),
		Policies:     entries,
	}, nil
}

// SetSocPstate sets the soc pstate policy for the given processor.
func SetSocPstate(ph ProcessorHandle, policyID uint32) error {
	ret := C.amdsmi_set_soc_pstate(ph.cPtr(), C.uint32_t(policyID))
	return checkStatus(Status(ret))
}

// GetXgmiPlpd returns the XGMI per-link power down policy for the given processor.
func GetXgmiPlpd(ph ProcessorHandle) (DpmPolicy, error) {
	var cPolicy C.amdsmi_dpm_policy_t
	ret := C.amdsmi_get_xgmi_plpd(ph.cPtr(), &cPolicy)
	if err := checkStatus(Status(ret)); err != nil {
		return DpmPolicy{}, err
	}

	n := uint32(cPolicy.num_supported)
	if n > AMDSMI_MAX_NUM_PM_POLICIES {
		n = AMDSMI_MAX_NUM_PM_POLICIES
	}

	entries := make([]DpmPolicyEntry, n)
	for i := uint32(0); i < n; i++ {
		entries[i] = DpmPolicyEntry{
			PolicyID:          uint32(cPolicy.policies[i].policy_id),
			PolicyDescription: C.GoString(&cPolicy.policies[i].policy_description[0]),
		}
	}

	return DpmPolicy{
		NumSupported: n,
		Current:      uint32(cPolicy.current),
		Policies:     entries,
	}, nil
}

// SetXgmiPlpd sets the XGMI per-link power down policy for the given processor.
func SetXgmiPlpd(ph ProcessorHandle, policyID uint32) error {
	ret := C.amdsmi_set_xgmi_plpd(ph.cPtr(), C.uint32_t(policyID))
	return checkStatus(Status(ret))
}

// SetPowerCap sets the power cap for the given processor.
func SetPowerCap(ph ProcessorHandle, sensorInd uint32, cap uint64) error {
	ret := C.amdsmi_set_power_cap(ph.cPtr(), C.uint32_t(sensorInd), C.uint64_t(cap))
	return checkStatus(Status(ret))
}

// SupportedPowerCapEntry pairs a power-cap sensor index with its type.
type SupportedPowerCapEntry struct {
	SensorIndex uint32
	Type        PowerCapType
}

// GetSupportedPowerCap returns supported power-cap sensor indices and types (amdsmi_get_supported_power_cap)
func GetSupportedPowerCap(ph ProcessorHandle) ([]SupportedPowerCapEntry, error) {
	var cCount C.uint32_t
	inds := make([]C.uint32_t, AMDSMI_MAX_DEVICES)
	types := make([]C.amdsmi_power_cap_type_t, AMDSMI_MAX_DEVICES)

	ret := C.amdsmi_get_supported_power_cap(ph.cPtr(), &cCount, &inds[0], &types[0])
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}

	n := uint32(cCount)
	if n > AMDSMI_MAX_DEVICES {
		n = AMDSMI_MAX_DEVICES
	}

	out := make([]SupportedPowerCapEntry, n)
	for i := uint32(0); i < n; i++ {
		out[i] = SupportedPowerCapEntry{
			SensorIndex: uint32(inds[i]),
			Type:        PowerCapType(types[i]),
		}
	}

	return out, nil
}

// CachePropertyType is a bitmask of cache properties (amdsmi_cache_property_type_t).
type CachePropertyType uint32

const (
	AMDSMI_CACHE_PROPERTY_ENABLED    CachePropertyType = 0x00000001
	AMDSMI_CACHE_PROPERTY_DATA_CACHE CachePropertyType = 0x00000002
	AMDSMI_CACHE_PROPERTY_INST_CACHE CachePropertyType = 0x00000004
	AMDSMI_CACHE_PROPERTY_CPU_CACHE  CachePropertyType = 0x00000008
	AMDSMI_CACHE_PROPERTY_SIMD_CACHE CachePropertyType = 0x00000010
)

func (c CachePropertyType) String() string {
	var names []string
	if c&AMDSMI_CACHE_PROPERTY_DATA_CACHE != 0 {
		names = append(names, "DATA_CACHE")
	}
	if c&AMDSMI_CACHE_PROPERTY_INST_CACHE != 0 {
		names = append(names, "INST_CACHE")
	}
	if c&AMDSMI_CACHE_PROPERTY_CPU_CACHE != 0 {
		names = append(names, "CPU_CACHE")
	}
	if c&AMDSMI_CACHE_PROPERTY_SIMD_CACHE != 0 {
		names = append(names, "SIMD_CACHE")
	}
	return fmt.Sprintf("%v", names)
}

// CacheEntry holds information about a single GPU cache type.
type CacheEntry struct {
	CacheProperties  CachePropertyType
	CacheSize        uint32
	CacheLevel       uint32
	MaxNumCuShared   uint32
	NumCacheInstance uint32
}

// GpuCacheInfo holds GPU cache information.
type GpuCacheInfo struct {
	NumCacheTypes uint32
	Cache         []CacheEntry
}

// GetGpuCacheInfo returns cache information for the given processor.
func GetGpuCacheInfo(ph ProcessorHandle) (GpuCacheInfo, error) {
	var cInfo C.amdsmi_gpu_cache_info_t
	ret := C.amdsmi_get_gpu_cache_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return GpuCacheInfo{}, err
	}

	n := uint32(cInfo.num_cache_types)
	if n > AMDSMI_MAX_CACHE_TYPES {
		n = AMDSMI_MAX_CACHE_TYPES
	}

	entries := make([]CacheEntry, n)
	for i := uint32(0); i < n; i++ {
		entries[i] = CacheEntry{
			CacheProperties:  CachePropertyType(cInfo.cache[i].cache_properties),
			CacheSize:        uint32(cInfo.cache[i].cache_size),
			CacheLevel:       uint32(cInfo.cache[i].cache_level),
			MaxNumCuShared:   uint32(cInfo.cache[i].max_num_cu_shared),
			NumCacheInstance: uint32(cInfo.cache[i].num_cache_instance),
		}
	}

	return GpuCacheInfo{
		NumCacheTypes: n,
		Cache:         entries,
	}, nil
}

// ErrorCount holds ECC error counters (amdsmi_error_count_t).
type ErrorCount struct {
	CorrectableCount   uint64
	UncorrectableCount uint64
	DeferredCount      uint64
}

// GetGpuEccCount returns ECC error counts for the given GPU block.
func GetGpuEccCount(ph ProcessorHandle, block GpuBlock) (ErrorCount, error) {
	var cErrorCnt C.amdsmi_error_count_t
	ret := C.amdsmi_get_gpu_ecc_count(ph.cPtr(), C.amdsmi_gpu_block_t(block), &cErrorCnt)
	if err := checkStatus(Status(ret)); err != nil {
		return ErrorCount{}, err
	}
	return ErrorCount{
		CorrectableCount:   uint64(cErrorCnt.correctable_count),
		UncorrectableCount: uint64(cErrorCnt.uncorrectable_count),
		DeferredCount:      uint64(cErrorCnt.deferred_count),
	}, nil
}

func eccEnabledBlockMap(mask uint64) map[GpuBlock]bool {
	out := make(map[GpuBlock]bool)
	for b := AMDSMI_GPU_BLOCK_FIRST; b <= AMDSMI_GPU_BLOCK_LAST; b <<= 1 {
		out[b] = mask&uint64(b) != 0
	}
	return out
}

// GetGpuEccEnabled returns whether ECC is enabled per GPU block
func GetGpuEccEnabled(ph ProcessorHandle) (map[GpuBlock]bool, error) {
	var cMask C.uint64_t
	ret := C.amdsmi_get_gpu_ecc_enabled(ph.cPtr(), &cMask)
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}
	return eccEnabledBlockMap(uint64(cMask)), nil
}

// GetGpuTotalEccCount returns total ECC errors (correctable, uncorrectable, deferred) for the GPU.
func GetGpuTotalEccCount(ph ProcessorHandle) (ErrorCount, error) {
	var cErrorCnt C.amdsmi_error_count_t
	ret := C.amdsmi_get_gpu_total_ecc_count(ph.cPtr(), &cErrorCnt)
	if err := checkStatus(Status(ret)); err != nil {
		return ErrorCount{}, err
	}
	return ErrorCount{
		CorrectableCount:   uint64(cErrorCnt.correctable_count),
		UncorrectableCount: uint64(cErrorCnt.uncorrectable_count),
		DeferredCount:      uint64(cErrorCnt.deferred_count),
	}, nil
}

// -----------------------------------------------------------------------------
// CPER (Common Platform Error Record) types and APIs.
// Mirrors amdsmi_cper_* in interface/amdsmi.h and the Python helpers in
// py/interface/amdsmi_interface.py.
// -----------------------------------------------------------------------------

// CperSeverity mirrors amdsmi_cper_sev_t.
type CperSeverity uint32

const (
	AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED CperSeverity = 0
	AMDSMI_CPER_SEV_FATAL                 CperSeverity = 1
	AMDSMI_CPER_SEV_NON_FATAL_CORRECTED   CperSeverity = 2
	AMDSMI_CPER_SEV_NUM                   CperSeverity = 3
	AMDSMI_CPER_SEV_UNUSED                CperSeverity = 10
)

func (s CperSeverity) String() string {
	switch s {
	case AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED:
		return "non_fatal_uncorrected"
	case AMDSMI_CPER_SEV_FATAL:
		return "fatal"
	case AMDSMI_CPER_SEV_NON_FATAL_CORRECTED:
		return "non_fatal_corrected"
	case AMDSMI_CPER_SEV_NUM:
		return "num"
	case AMDSMI_CPER_SEV_UNUSED:
		return "unused"
	}
	return fmt.Sprintf("UNKNOWN(%d)", uint32(s))
}

// CperNotifyType mirrors amdsmi_cper_notify_type_t. The library stores the
// notify type as a 16-byte GUID in the header; the first 8 bytes (read
// little-endian) form a uint64 that matches one of these constants.
type CperNotifyType uint64

const (
	AMDSMI_CPER_NOTIFY_TYPE_CMC           CperNotifyType = 0x450eBDD72DCE8BB1
	AMDSMI_CPER_NOTIFY_TYPE_CPE           CperNotifyType = 0x4a55D8434E292F96
	AMDSMI_CPER_NOTIFY_TYPE_MCE           CperNotifyType = 0x4cc5919CE8F56FFE
	AMDSMI_CPER_NOTIFY_TYPE_PCIE          CperNotifyType = 0x4dfc1A16CF93C01F
	AMDSMI_CPER_NOTIFY_TYPE_INIT          CperNotifyType = 0x454a9308CC5263E8
	AMDSMI_CPER_NOTIFY_TYPE_NMI           CperNotifyType = 0x42c9B7E65BAD89FF
	AMDSMI_CPER_NOTIFY_TYPE_BOOT          CperNotifyType = 0x409aAB403D61A466
	AMDSMI_CPER_NOTIFY_TYPE_DMAR          CperNotifyType = 0x4c27C6B3667DD791
	AMDSMI_CPER_NOTIFY_TYPE_SEA           CperNotifyType = 0x11E4BBE89A78788A
	AMDSMI_CPER_NOTIFY_TYPE_SEI           CperNotifyType = 0x4E87B0AE5C284C81
	AMDSMI_CPER_NOTIFY_TYPE_PEI           CperNotifyType = 0x4214520409A9D5AC
	AMDSMI_CPER_NOTIFY_TYPE_CXL_COMPONENT CperNotifyType = 0x49A341DF69293BC9
)

func (n CperNotifyType) String() string {
	switch n {
	case AMDSMI_CPER_NOTIFY_TYPE_CMC:
		return "CMC"
	case AMDSMI_CPER_NOTIFY_TYPE_CPE:
		return "CPE"
	case AMDSMI_CPER_NOTIFY_TYPE_MCE:
		return "MCE"
	case AMDSMI_CPER_NOTIFY_TYPE_PCIE:
		return "PCIE"
	case AMDSMI_CPER_NOTIFY_TYPE_INIT:
		return "INIT"
	case AMDSMI_CPER_NOTIFY_TYPE_NMI:
		return "NMI"
	case AMDSMI_CPER_NOTIFY_TYPE_BOOT:
		return "BOOT"
	case AMDSMI_CPER_NOTIFY_TYPE_DMAR:
		return "DMAR"
	case AMDSMI_CPER_NOTIFY_TYPE_SEA:
		return "SEA"
	case AMDSMI_CPER_NOTIFY_TYPE_SEI:
		return "SEI"
	case AMDSMI_CPER_NOTIFY_TYPE_PEI:
		return "PEI"
	case AMDSMI_CPER_NOTIFY_TYPE_CXL_COMPONENT:
		return "CXL_COMPONENT"
	}
	return "Unknown"
}

// CperGuid mirrors amdsmi_cper_guid_t (16 raw bytes).
type CperGuid [16]byte

// NotifyType decodes the first 8 bytes (read big-endian-of-reverse-order, the
// same way py/interface/amdsmi_interface.py:_notifyTypeToString does it) and
// matches them against the documented amdsmi_cper_notify_type_t values.
// Returns 0 if the guid does not match any known type.
func (g CperGuid) NotifyType() CperNotifyType {
	var v uint64
	for i := 7; i >= 0; i-- {
		v = (v << 8) | uint64(g[i])
	}
	return CperNotifyType(v)
}

// CperTimestamp mirrors amdsmi_cper_timestamp_t.
type CperTimestamp struct {
	Seconds uint8
	Minutes uint8
	Hours   uint8
	Flag    uint8
	Day     uint8
	Month   uint8
	Year    uint8
	Century uint8
}

// String formats the timestamp as "YYYY/MM/DD HH:MM:SS", matching the Python
// formatting in amdsmi_get_gpu_cper_entries. If Year < 100 it is interpreted
// as 2000 + Year (the same heuristic the Python wrapper uses).
func (t CperTimestamp) String() string {
	year := int(t.Year)
	if year < 100 {
		year += 2000
	}
	return fmt.Sprintf("%04d/%02d/%02d %02d:%02d:%02d",
		year, t.Month, t.Day, t.Hours, t.Minutes, t.Seconds)
}

// CperHeader is a Go-friendly mirror of amdsmi_cper_hdr_t.
type CperHeader struct {
	Signature       string // "CPER" (4 bytes)
	Revision        uint16
	SignatureEnd    uint32 // expected to be 0xFFFFFFFF
	SecCnt          uint16
	ErrorSeverity   CperSeverity
	ValidMask       uint32 // raw amdsmi_cper_valid_bits_t.valid_mask
	RecordLength    uint32 // total size of the CPER record in bytes
	Timestamp       CperTimestamp
	PlatformID      [16]byte
	PartitionID     CperGuid
	CreatorID       [16]byte
	NotifyType      CperGuid // call NotifyType() to decode against AMDSMI_CPER_NOTIFY_TYPE_*
	RecordID        [8]byte
	Flags           uint32
	PersistenceInfo uint64
}

// CperEntry pairs the parsed header with the raw bytes of one CPER record.
// The Bytes slice can be passed unchanged to GetAfidsFromCper.
type CperEntry struct {
	Header CperHeader
	Bytes  []byte
}

// Default buffer size used by GetGpuCperEntries when the caller passes 0,
// matching the Python wrapper. The per-call entry cap is hard-coded to 20
// (cperEntriesPerCall) to mirror the Python signature exactly.
const (
	defaultCperBufferSize = 4 * 1024 * 1024 // 4 MiB
	cperEntriesPerCall    = 20
)

// GetGpuCperEntries calls amdsmi_get_gpu_cper_entries and returns the parsed
// entries together with the next cursor (pass it back to retrieve more data).
//
// Pass 0 for bufferSize to use the same default the Python wrapper uses (4 MiB).
// Up to 20 entries are returned per call, matching the Python wrapper exactly.
// The library may return AMDSMI_STATUS_MORE_DATA when buffers are too small to
// hold every cached record; the entries collected so far are still returned
// and the caller can retry with the new cursor.
//
// See interface/amdsmi.h:amdsmi_get_gpu_cper_entries for the full contract.
func GetGpuCperEntries(ph ProcessorHandle, severityMask uint32, cursor uint64, bufferSize uint64) ([]CperEntry, uint64, error) {
	if bufferSize == 0 {
		bufferSize = defaultCperBufferSize
	}

	cBuf := C.malloc(C.size_t(bufferSize))
	if cBuf == nil {
		return nil, 0, &StatusError{Code: AMDSMI_STATUS_OUT_OF_RESOURCES, Name: statusNames[AMDSMI_STATUS_OUT_OF_RESOURCES]}
	}
	defer C.free(cBuf)

	cBufSize := C.uint64_t(bufferSize)
	cEntryCount := C.uint64_t(cperEntriesPerCall)
	cCursor := C.uint64_t(cursor)

	cHdrs := make([]*C.amdsmi_cper_hdr_t, cperEntriesPerCall)

	ret := C.amdsmi_get_gpu_cper_entries(
		ph.cPtr(),
		C.uint32_t(severityMask),
		(*C.char)(cBuf),
		&cBufSize,
		(**C.amdsmi_cper_hdr_t)(unsafe.Pointer(&cHdrs[0])),
		&cEntryCount,
		&cCursor,
	)

	status := Status(ret)
	if status != AMDSMI_STATUS_SUCCESS && status != AMDSMI_STATUS_MORE_DATA {
		return nil, 0, &StatusError{Code: status, Name: statusNames[status]}
	}

	count := int(cEntryCount)
	entries := make([]CperEntry, 0, count)
	for i := 0; i < count; i++ {
		hdr := cHdrs[i]
		if hdr == nil {
			break
		}
		recordLen := uint32(hdr.record_length)
		raw := C.GoBytes(unsafe.Pointer(hdr), C.int(recordLen))

		entries = append(entries, CperEntry{
			Header: CperHeader{
				Signature:       C.GoStringN(&hdr.signature[0], 4),
				Revision:        uint16(hdr.revision),
				SignatureEnd:    uint32(C.amdsmi_go_cper_signature_end(hdr)),
				SecCnt:          uint16(hdr.sec_cnt),
				ErrorSeverity:   CperSeverity(hdr.error_severity),
				ValidMask:       uint32(C.amdsmi_go_cper_valid_mask(hdr)),
				RecordLength:    recordLen,
				Timestamp:       cperTimestampToGo(&hdr.timestamp),
				PlatformID:      cperByteArray16(unsafe.Pointer(&hdr.platform_id[0])),
				PartitionID:     CperGuid(cperByteArray16(unsafe.Pointer(&hdr.partition_id.b[0]))),
				CreatorID:       cperByteArray16(unsafe.Pointer(&hdr.creator_id[0])),
				NotifyType:      CperGuid(cperByteArray16(unsafe.Pointer(&hdr.notify_type.b[0]))),
				RecordID:        cperByteArray8(unsafe.Pointer(&hdr.record_id[0])),
				Flags:           uint32(hdr.flags),
				PersistenceInfo: uint64(C.amdsmi_go_cper_persistence_info(hdr)),
			},
			Bytes: raw,
		})
	}

	return entries, uint64(cCursor), nil
}

// GetAfidsFromCper extracts AF IDs from a single CPER record buffer (such as
// CperEntry.Bytes). Up to AMDSMI_MAX_NUMBER_OF_AFIDS_PER_RECORD ids are returned.
func GetAfidsFromCper(cperBuffer []byte) ([]uint64, error) {
	if len(cperBuffer) == 0 {
		return nil, &StatusError{Code: AMDSMI_STATUS_INVAL, Name: statusNames[AMDSMI_STATUS_INVAL]}
	}

	cBuf := C.CBytes(cperBuffer)
	defer C.free(cBuf)

	var afids [AMDSMI_MAX_NUMBER_OF_AFIDS_PER_RECORD]C.uint64_t
	nAfids := C.uint32_t(AMDSMI_MAX_NUMBER_OF_AFIDS_PER_RECORD)

	ret := C.amdsmi_get_afids_from_cper(
		(*C.char)(cBuf),
		C.uint32_t(len(cperBuffer)),
		&afids[0],
		&nAfids,
	)
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}

	count := uint32(nAfids)
	if count > AMDSMI_MAX_NUMBER_OF_AFIDS_PER_RECORD {
		count = AMDSMI_MAX_NUMBER_OF_AFIDS_PER_RECORD
	}
	out := make([]uint64, count)
	for i := uint32(0); i < count; i++ {
		out[i] = uint64(afids[i])
	}
	return out, nil
}

// cperTimestampToGo copies an amdsmi_cper_timestamp_t into the Go mirror.
func cperTimestampToGo(ts *C.amdsmi_cper_timestamp_t) CperTimestamp {
	return CperTimestamp{
		Seconds: uint8(ts.seconds),
		Minutes: uint8(ts.minutes),
		Hours:   uint8(ts.hours),
		Flag:    uint8(ts.flag),
		Day:     uint8(ts.day),
		Month:   uint8(ts.month),
		Year:    uint8(ts.year),
		Century: uint8(ts.century),
	}
}

// cperByteArray16 / cperByteArray8 copy a fixed-size byte run out of a C
// header field. They take unsafe.Pointer so callers can pass either *C.char or
// *C.uchar (CGO refuses to convert between those without help).
func cperByteArray16(p unsafe.Pointer) [16]byte {
	var out [16]byte
	if p != nil {
		copy(out[:], (*[16]byte)(p)[:])
	}
	return out
}

func cperByteArray8(p unsafe.Pointer) [8]byte {
	var out [8]byte
	if p != nil {
		copy(out[:], (*[8]byte)(p)[:])
	}
	return out
}

// RasFeatureInfo holds RAS feature information for a GPU.
type RasFeatureInfo struct {
	RasEepromVersion        uint32
	EccCorrectionSchemaFlag uint32
}

// GetGpuRasFeatureInfo returns the RAS feature info for the given processor.
func GetGpuRasFeatureInfo(ph ProcessorHandle) (RasFeatureInfo, error) {
	var cInfo C.amdsmi_ras_feature_t
	ret := C.amdsmi_get_gpu_ras_feature_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return RasFeatureInfo{}, err
	}
	return RasFeatureInfo{
		RasEepromVersion:        uint32(cInfo.ras_eeprom_version),
		EccCorrectionSchemaFlag: uint32(cInfo.ecc_correction_schema_flag),
	}, nil
}

const AMDSMI_MAX_BAD_PAGE_RECORD uint32 = 16384

// EepromTableRecord holds a single bad page record from the EEPROM table.
type EepromTableRecord struct {
	RetiredPage uint64
	Ts          uint64
	MemChannel  uint8
	McumcID     uint8
}

// GetGpuBadPageInfo returns the list of bad page records for the given processor.
// It always allocates AMDSMI_MAX_BAD_PAGE_RECORD slice, to ensure valid API response,
// but final outcome is valid number of bad pages in a slice.
func GetGpuBadPageInfo(ph ProcessorHandle) ([]EepromTableRecord, error) {
	var cCount C.uint32_t = C.uint32_t(AMDSMI_MAX_BAD_PAGE_RECORD)

	records := make([]C.amdsmi_eeprom_table_record_t, AMDSMI_MAX_BAD_PAGE_RECORD)
	ret := C.amdsmi_get_gpu_bad_page_info(ph.cPtr(), &cCount, &records[0])
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}

	if cCount == 0 {
		return nil, nil
	}

	result := make([]EepromTableRecord, cCount)
	for i := C.uint32_t(0); i < cCount; i++ {
		result[i] = EepromTableRecord{
			RetiredPage: uint64(records[i].retired_page),
			Ts:          uint64(records[i].ts),
			MemChannel:  uint8(records[i].mem_channel),
			McumcID:     uint8(records[i].mcumc_id),
		}
	}
	return result, nil
}

// RasPolicyV4_0 holds v4.0 RAS policy thresholds.
type RasPolicyV4_0 struct {
	DramNonCriticalRegionThreshold uint16
	DramCriticalRegionThreshold    uint16
}

// RasPolicyInfo holds versioned RAS policy information for a GPU.
type RasPolicyInfo struct {
	MajorVersion uint8
	MinorVersion uint8
	V4_0         *RasPolicyV4_0
}

// GetGpuRasPolicyInfo returns the RAS policy info for the given processor.
func GetGpuRasPolicyInfo(ph ProcessorHandle) (RasPolicyInfo, error) {
	var cInfo C.amdsmi_gpu_ras_policy_info_t
	ret := C.amdsmi_get_gpu_ras_policy_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return RasPolicyInfo{}, err
	}

	result := RasPolicyInfo{
		MajorVersion: uint8(cInfo.major_version),
		MinorVersion: uint8(cInfo.minor_version),
	}

	if result.MajorVersion == 4 && result.MinorVersion == 0 {
		v4_0 := (*C.amdsmi_gpu_ras_policy_v4_0_t)(unsafe.Pointer(&cInfo.policy_data))
		result.V4_0 = &RasPolicyV4_0{
			DramNonCriticalRegionThreshold: uint16(v4_0.dram_non_critical_region_threshold),
			DramCriticalRegionThreshold:    uint16(v4_0.dram_critical_region_threshold),
		}
	}

	return result, nil
}

// GetBadPageThreshold returns the bad page retirement threshold for the given processor.
func GetBadPageThreshold(ph ProcessorHandle) (uint32, error) {
	var cThreshold C.uint32_t
	ret := C.amdsmi_get_bad_page_threshold(ph.cPtr(), &cThreshold)
	if err := checkStatus(Status(ret)); err != nil {
		return 0, err
	}
	return uint32(cThreshold), nil
}

func ResetGpu(ph ProcessorHandle) error {
	ret := C.amdsmi_reset_gpu(ph.cPtr())
	return checkStatus(Status(ret))
}

// NpmInfo holds NPM status and node power limit (amdsmi_npm_info_t).
type NpmInfo struct {
	Status            NpmStatus
	Limit             uint64 // Watts
	UbbPowerThreshold uint32
}

// GetNpmInfo returns NPM status and node power limit for the node (amdsmi_get_npm_info)
func GetNpmInfo(nh NodeHandle) (NpmInfo, error) {
	var cInfo C.amdsmi_npm_info_t

	ret := C.amdsmi_get_npm_info(nh.cPtr(), &cInfo)

	if err := checkStatus(Status(ret)); err != nil {
		return NpmInfo{}, err
	}

	return NpmInfo{
		Status:            NpmStatus(cInfo.status),
		Limit:             uint64(cInfo.limit),
		UbbPowerThreshold: uint32(cInfo.ubb_power_threshold),
	}, nil
}

// GetGpuPtlState returns whether PTL (Peak Tops Limiter) is enabled (amdsmi_get_gpu_ptl_state)
func GetGpuPtlState(ph ProcessorHandle) (bool, error) {
	var cEnabled C.bool

	ret := C.amdsmi_get_gpu_ptl_state(ph.cPtr(), &cEnabled)
	if err := checkStatus(Status(ret)); err != nil {
		return false, err
	}

	return bool(cEnabled), nil
}

// SetGpuPtlState enables or disables PTL with default formats when enabling (amdsmi_set_gpu_ptl_state)
func SetGpuPtlState(ph ProcessorHandle, enable bool) error {
	ret := C.amdsmi_set_gpu_ptl_state(ph.cPtr(), C.bool(enable))

	return checkStatus(Status(ret))
}

// GetGpuPtlFormats returns the two preferred PTL data formats (amdsmi_get_gpu_ptl_formats)
func GetGpuPtlFormats(ph ProcessorHandle) (format1, format2 PtlDataFormat, err error) {
	var cf1, cf2 C.amdsmi_ptl_data_format_t

	ret := C.amdsmi_get_gpu_ptl_formats(ph.cPtr(), &cf1, &cf2)
	if e := checkStatus(Status(ret)); e != nil {
		return 0, 0, e
	}

	return PtlDataFormat(cf1), PtlDataFormat(cf2), nil
}

// SetGpuPtlFormats sets the preferred PTL data-format pair; PTL must already be enabled (amdsmi_set_gpu_ptl_formats)
func SetGpuPtlFormats(ph ProcessorHandle, dataFormat1, dataFormat2 PtlDataFormat) error {
	ret := C.amdsmi_set_gpu_ptl_formats(ph.cPtr(), C.amdsmi_ptl_data_format_t(dataFormat1), C.amdsmi_ptl_data_format_t(dataFormat2))
	return checkStatus(Status(ret))
}

// GetNumVf returns the number of enabled and supported VFs for the given processor.
func GetNumVf(ph ProcessorHandle) (numEnabled uint32, numSupported uint32, err error) {
	var cEnabled, cSupported C.uint32_t
	ret := C.amdsmi_get_num_vf(ph.cPtr(), &cEnabled, &cSupported)
	if err := checkStatus(Status(ret)); err != nil {
		return 0, 0, err
	}
	return uint32(cEnabled), uint32(cSupported), nil
}

// VfFbInfo holds VF framebuffer information.
type VfFbInfo struct {
	FbOffset uint32
	FbSize   uint32
}

// PartitionInfo holds partition information for a VF.
type PartitionInfo struct {
	VfHandle VfHandle
	Fb       VfFbInfo
}

// GetVfPartitionInfo returns partition info for all enabled VFs on the given processor.
func GetVfPartitionInfo(ph ProcessorHandle) ([]PartitionInfo, error) {
	var cEnabled, cSupported C.uint32_t
	ret := C.amdsmi_get_num_vf(ph.cPtr(), &cEnabled, &cSupported)
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}
	numEnabled := uint32(cEnabled)
	if numEnabled == 0 {
		return nil, nil
	}

	infos := make([]C.amdsmi_partition_info_t, numEnabled)
	ret = C.amdsmi_get_vf_partition_info(ph.cPtr(), C.uint(numEnabled), &infos[0])
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}

	result := make([]PartitionInfo, numEnabled)
	for i := uint32(0); i < numEnabled; i++ {
		result[i] = PartitionInfo{
			VfHandle: VfHandle{raw: uint64(infos[i].id.handle)},
			Fb: VfFbInfo{
				FbOffset: uint32(infos[i].fb.fb_offset),
				FbSize:   uint32(infos[i].fb.fb_size),
			},
		}
	}
	return result, nil
}

// ProfileCapsInfo holds capability totals for one partition profile slice (amdsmi_profile_caps_info_t).
type ProfileCapsInfo struct {
	Total     uint64
	Available uint64
	Optimal   uint64
	MinValue  uint64
	MaxValue  uint64
}

// PartitionProfileEntry describes one supported VF partition profile.
type PartitionProfileEntry struct {
	VfCount     uint32
	ProfileCaps [AMDSMI_PROFILE_CAPABILITY__MAX]ProfileCapsInfo
}

// ProfileInfo lists supported partition profiles (amdsmi_profile_info_t).
type ProfileInfo struct {
	ProfileCount        uint8
	CurrentProfileIndex uint8
	Profiles            []PartitionProfileEntry
}

// GetPartitionProfileInfo returns supported VF partition profiles for the given processor.
func GetPartitionProfileInfo(ph ProcessorHandle) (ProfileInfo, error) {
	var cpi C.amdsmi_profile_info_t
	ret := C.amdsmi_get_partition_profile_info(ph.cPtr(), &cpi)
	if err := checkStatus(Status(ret)); err != nil {
		return ProfileInfo{}, err
	}
	n := int(cpi.profile_count)
	if n > AMDSMI_MAX_PROFILE_COUNT {
		n = AMDSMI_MAX_PROFILE_COUNT
	}
	profiles := make([]PartitionProfileEntry, n)
	for i := 0; i < n; i++ {
		p := cpi.profiles[i]
		profiles[i].VfCount = uint32(p.vf_count)
		for ct := AMDSMI_PROFILE_CAPABILITY_MEMORY; ct < AMDSMI_PROFILE_CAPABILITY__MAX; ct++ {
			c := p.profile_caps[ct]
			profiles[i].ProfileCaps[ct] = ProfileCapsInfo{
				Total:     uint64(c.total),
				Available: uint64(c.available),
				Optimal:   uint64(c.optimal),
				MinValue:  uint64(c.min_value),
				MaxValue:  uint64(c.max_value),
			}
		}
	}
	return ProfileInfo{
		ProfileCount:        uint8(cpi.profile_count),
		CurrentProfileIndex: uint8(cpi.current_profile_index),
		Profiles:            profiles,
	}, nil
}

// VfInfo holds VF configuration information (framebuffer and timeslice).
type VfInfo struct {
	Fb           VfFbInfo
	GfxTimeslice uint32
}

// GetVfInfo returns the VF configuration for the given VF handle.
func GetVfInfo(vh VfHandle) (VfInfo, error) {
	var cInfo C.amdsmi_vf_info_t

	ret := C.amdsmi_get_vf_info(vh.cVfHandle(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return VfInfo{}, err
	}

	return VfInfo{
		Fb: VfFbInfo{
			FbOffset: uint32(cInfo.fb.fb_offset),
			FbSize:   uint32(cInfo.fb.fb_size),
		},
		GfxTimeslice: uint32(cInfo.gfx_timeslice),
	}, nil
}

// VfSchedState represents the scheduling state of a VF (amdsmi_vf_sched_state_t).
type VfSchedState int32

const (
	AMDSMI_VF_STATE_UNAVAILABLE       VfSchedState = 0
	AMDSMI_VF_STATE_AVAILABLE         VfSchedState = 1
	AMDSMI_VF_STATE_ACTIVE            VfSchedState = 2
	AMDSMI_VF_STATE_SUSPENDED         VfSchedState = 3
	AMDSMI_VF_STATE_FULLACCESS        VfSchedState = 4
	AMDSMI_VF_STATE_DEFAULT_AVAILABLE VfSchedState = 5
)

func (s VfSchedState) String() string {
	switch s {
	case AMDSMI_VF_STATE_UNAVAILABLE:
		return "UNAVAILABLE"
	case AMDSMI_VF_STATE_AVAILABLE:
		return "AVAILABLE"
	case AMDSMI_VF_STATE_ACTIVE:
		return "ACTIVE"
	case AMDSMI_VF_STATE_SUSPENDED:
		return "SUSPENDED"
	case AMDSMI_VF_STATE_FULLACCESS:
		return "FULLACCESS"
	case AMDSMI_VF_STATE_DEFAULT_AVAILABLE:
		return "DEFAULT_AVAILABLE"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(s))
	}
}

// GuardState represents the state of a guard event (amdsmi_guard_state_t).
type GuardState int32

const (
	AMDSMI_GUARD_STATE_NORMAL   GuardState = 0
	AMDSMI_GUARD_STATE_FULL     GuardState = 1
	AMDSMI_GUARD_STATE_OVERFLOW GuardState = 2
)

func (g GuardState) String() string {
	switch g {
	case AMDSMI_GUARD_STATE_NORMAL:
		return "NORMAL"
	case AMDSMI_GUARD_STATE_FULL:
		return "FULL"
	case AMDSMI_GUARD_STATE_OVERFLOW:
		return "OVERFLOW"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(g))
	}
}

// GuardEventType identifies guard event types (amdsmi_guard_type_t).
type GuardEventType int

const (
	AMDSMI_GUARD_EVENT_FLR               GuardEventType = 0
	AMDSMI_GUARD_EVENT_EXCLUSIVE_MOD     GuardEventType = 1
	AMDSMI_GUARD_EVENT_EXCLUSIVE_TIMEOUT GuardEventType = 2
	AMDSMI_GUARD_EVENT_ALL_INT           GuardEventType = 3
	AMDSMI_GUARD_EVENT_RAS_ERR_COUNT     GuardEventType = 4
	AMDSMI_GUARD_EVENT_RAS_CPER_DUMP     GuardEventType = 5
	AMDSMI_GUARD_EVENT_RAS_BAD_PAGES     GuardEventType = 6
)

func (g GuardEventType) String() string {
	switch g {
	case AMDSMI_GUARD_EVENT_FLR:
		return "FLR"
	case AMDSMI_GUARD_EVENT_EXCLUSIVE_MOD:
		return "EXCLUSIVE_MOD"
	case AMDSMI_GUARD_EVENT_EXCLUSIVE_TIMEOUT:
		return "EXCLUSIVE_TIMEOUT"
	case AMDSMI_GUARD_EVENT_ALL_INT:
		return "ALL_INT"
	case AMDSMI_GUARD_EVENT_RAS_ERR_COUNT:
		return "RAS_ERR_COUNT"
	case AMDSMI_GUARD_EVENT_RAS_CPER_DUMP:
		return "RAS_CPER_DUMP"
	case AMDSMI_GUARD_EVENT_RAS_BAD_PAGES:
		return "RAS_BAD_PAGES"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", int(g))
	}
}

// GuardEventInfo holds counters and thresholds for a single guard event type.
type GuardEventInfo struct {
	State     GuardState
	Amount    uint32
	Interval  uint64
	Threshold uint32
	Active    uint32
}

// GuardInfo holds the overall guard configuration and per-event details.
type GuardInfo struct {
	Enabled bool
	Guard   [AMDSMI_GUARD_EVENT__MAX]GuardEventInfo
}

// SchedInfo holds VF scheduling and lifecycle timing information.
type SchedInfo struct {
	FlrCount           uint64
	BootUpTime         uint64
	ShutdownTime       uint64
	ResetTime          uint64
	State              VfSchedState
	LastBootStart      string
	LastBootEnd        string
	LastShutdownStart  string
	LastShutdownEnd    string
	LastResetStart     string
	LastResetEnd       string
	CurrentActiveTime  string
	CurrentRunningTime string
	TotalActiveTime    string
	TotalRunningTime   string
}

// VfData holds VF runtime data (scheduling info and guard info).
type VfData struct {
	Sched SchedInfo
	Guard GuardInfo
}

// GetVfData returns the VF runtime data for the given VF handle.
func GetVfData(vh VfHandle) (VfData, error) {
	var cData C.amdsmi_vf_data_t

	ret := C.amdsmi_get_vf_data(vh.cVfHandle(), &cData)
	if err := checkStatus(Status(ret)); err != nil {
		return VfData{}, err
	}

	sched := SchedInfo{
		FlrCount:           uint64(cData.sched.flr_count),
		BootUpTime:         uint64(cData.sched.boot_up_time),
		ShutdownTime:       uint64(cData.sched.shutdown_time),
		ResetTime:          uint64(cData.sched.reset_time),
		State:              VfSchedState(cData.sched.state),
		LastBootStart:      C.GoString(&cData.sched.last_boot_start[0]),
		LastBootEnd:        C.GoString(&cData.sched.last_boot_end[0]),
		LastShutdownStart:  C.GoString(&cData.sched.last_shutdown_start[0]),
		LastShutdownEnd:    C.GoString(&cData.sched.last_shutdown_end[0]),
		LastResetStart:     C.GoString(&cData.sched.last_reset_start[0]),
		LastResetEnd:       C.GoString(&cData.sched.last_reset_end[0]),
		CurrentActiveTime:  C.GoString(&cData.sched.current_active_time[0]),
		CurrentRunningTime: C.GoString(&cData.sched.current_running_time[0]),
		TotalActiveTime:    C.GoString(&cData.sched.total_active_time[0]),
		TotalRunningTime:   C.GoString(&cData.sched.total_running_time[0]),
	}

	guard := GuardInfo{
		Enabled: cData.guard.enabled != 0,
	}
	for i := 0; i < AMDSMI_GUARD_EVENT__MAX; i++ {
		guard.Guard[i] = GuardEventInfo{
			State:     GuardState(cData.guard.guard[i].state),
			Amount:    uint32(cData.guard.guard[i].amount),
			Interval:  uint64(cData.guard.guard[i].interval),
			Threshold: uint32(cData.guard.guard[i].threshold),
			Active:    uint32(cData.guard.guard[i].active),
		}
	}

	return VfData{
		Sched: sched,
		Guard: guard,
	}, nil
}

// -----------------------------------------------------------------------------
// Event notifier API (mirrors amdsmi_event_* in interface/amdsmi.h).
// -----------------------------------------------------------------------------

// EventCategory identifies which subsystem produced an event (amdsmi_event_category_t).
type EventCategory uint32

const (
	AMDSMI_EVENT_CATEGORY_NON_USED EventCategory = 0
	AMDSMI_EVENT_CATEGORY_DRIVER   EventCategory = 1
	AMDSMI_EVENT_CATEGORY_RESET    EventCategory = 2
	AMDSMI_EVENT_CATEGORY_SCHED    EventCategory = 3
	AMDSMI_EVENT_CATEGORY_VBIOS    EventCategory = 4
	AMDSMI_EVENT_CATEGORY_ECC      EventCategory = 5
	AMDSMI_EVENT_CATEGORY_PP       EventCategory = 6
	AMDSMI_EVENT_CATEGORY_IOV      EventCategory = 7
	AMDSMI_EVENT_CATEGORY_VF       EventCategory = 8
	AMDSMI_EVENT_CATEGORY_FW       EventCategory = 9
	AMDSMI_EVENT_CATEGORY_GPU      EventCategory = 10
	AMDSMI_EVENT_CATEGORY_GUARD    EventCategory = 11
	AMDSMI_EVENT_CATEGORY_GPUMON   EventCategory = 12
	AMDSMI_EVENT_CATEGORY_MMSCH    EventCategory = 13
	AMDSMI_EVENT_CATEGORY_XGMI     EventCategory = 14
	AMDSMI_EVENT_CATEGORY__MAX     EventCategory = 15
)

func (c EventCategory) String() string {
	switch c {
	case AMDSMI_EVENT_CATEGORY_NON_USED:
		return "NON_USED"
	case AMDSMI_EVENT_CATEGORY_DRIVER:
		return "DRIVER"
	case AMDSMI_EVENT_CATEGORY_RESET:
		return "RESET"
	case AMDSMI_EVENT_CATEGORY_SCHED:
		return "SCHED"
	case AMDSMI_EVENT_CATEGORY_VBIOS:
		return "VBIOS"
	case AMDSMI_EVENT_CATEGORY_ECC:
		return "ECC"
	case AMDSMI_EVENT_CATEGORY_PP:
		return "PP"
	case AMDSMI_EVENT_CATEGORY_IOV:
		return "IOV"
	case AMDSMI_EVENT_CATEGORY_VF:
		return "VF"
	case AMDSMI_EVENT_CATEGORY_FW:
		return "FW"
	case AMDSMI_EVENT_CATEGORY_GPU:
		return "GPU"
	case AMDSMI_EVENT_CATEGORY_GUARD:
		return "GUARD"
	case AMDSMI_EVENT_CATEGORY_GPUMON:
		return "GPUMON"
	case AMDSMI_EVENT_CATEGORY_MMSCH:
		return "MMSCH"
	case AMDSMI_EVENT_CATEGORY_XGMI:
		return "XGMI"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", uint32(c))
	}
}

// EventSeverity represents the severity level of an event.
type EventSeverity uint32

const (
	AMDSMI_EVENT_SEVERITY_HIGH EventSeverity = 0
	AMDSMI_EVENT_SEVERITY_MED  EventSeverity = 1
	AMDSMI_EVENT_SEVERITY_LOW  EventSeverity = 2
	AMDSMI_EVENT_SEVERITY_WARN EventSeverity = 3
	AMDSMI_EVENT_SEVERITY_INFO EventSeverity = 4
)

func (s EventSeverity) String() string {
	switch s {
	case AMDSMI_EVENT_SEVERITY_HIGH:
		return "HIGH"
	case AMDSMI_EVENT_SEVERITY_MED:
		return "MED"
	case AMDSMI_EVENT_SEVERITY_LOW:
		return "LOW"
	case AMDSMI_EVENT_SEVERITY_WARN:
		return "WARN"
	case AMDSMI_EVENT_SEVERITY_INFO:
		return "INFO"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", uint32(s))
	}
}

// EventSet wraps the opaque amdsmi_event_set returned by amdsmi_event_create.
// The pointer is opaque and must be released via EventDestroy.
type EventSet struct {
	raw unsafe.Pointer
}

// EventEntry is a Go-friendly mirror of amdsmi_event_entry_t returned by EventRead.
type EventEntry struct {
	FcnId           VfHandle
	DevId           uint64
	Timestamp       uint64 // UTC microseconds
	Data            uint64
	Category        EventCategory
	Subcode         uint32
	Level           EventSeverity
	Date            string // UTC date and time
	Message         string
	ProcessorHandle ProcessorHandle
}

// AMDSMI_EVENT_MASK_ALL_CATEGORIES enables every category bit (bits 0..59) in
// the event_types mask passed to EventCreate.
const AMDSMI_EVENT_MASK_ALL_CATEGORIES uint64 = (uint64(1) << 60) - 1

// AMDSMI_EVENT_MASK_ALL_SEVERITIES enables every severity bit (bits 60..63) in
// the event_types mask passed to EventCreate.
const AMDSMI_EVENT_MASK_ALL_SEVERITIES uint64 = uint64(0xF) << 60

// AMDSMI_MASK_INIT is a clear event mask (mirrors AMDSMI_MASK_INIT in amdsmi.h).
const AMDSMI_MASK_INIT uint64 = 0

// AMDSMI_MASK_DEFAULT enables every category and HIGH+MED+LOW severities
// (no warnings, no info). Mirrors AMDSMI_MASK_DEFAULT in amdsmi.h.
const AMDSMI_MASK_DEFAULT uint64 = (uint64(1) << 62) - 1

// AMDSMI_MASK_ALL enables every category and every severity bit
// (mirrors AMDSMI_MASK_ALL in amdsmi.h).
const AMDSMI_MASK_ALL uint64 = ^uint64(0)

// BuildEventMask builds the event_types bitmask passed to EventCreate, mirroring
// the layout described in amdsmi_event_create:
//
//	bits 0..59 : category bits (one per amdsmi_event_category_t value)
//	bits 60..63: severity bits
//
// Each entry of categories sets the corresponding bit (1 << cat). The severity
// argument enables all severity levels up to and including the chosen one
// (e.g. MED includes HIGH+MED, LOW includes HIGH+MED+LOW, ...).
// To enable every category or every severity, use BuildEventMaskAllCategories,
// BuildEventMaskAllSeverities, or OR in AMDSMI_EVENT_MASK_ALL_CATEGORIES /
// AMDSMI_EVENT_MASK_ALL_SEVERITIES directly.
func BuildEventMask(categories []EventCategory, severity EventSeverity) uint64 {
	var mask uint64
	for _, cat := range categories {
		mask |= uint64(1) << uint64(cat)
	}

	switch severity {
	case AMDSMI_EVENT_SEVERITY_HIGH:
		mask &= (uint64(1) << 60) - 1
	case AMDSMI_EVENT_SEVERITY_MED:
		mask |= uint64(1) << 60
	case AMDSMI_EVENT_SEVERITY_LOW:
		mask |= uint64(1) << 61
	case AMDSMI_EVENT_SEVERITY_WARN:
		mask |= uint64(1) << 62
	case AMDSMI_EVENT_SEVERITY_INFO:
		mask |= uint64(1) << 63
	}
	return mask
}

// BuildEventMaskAllCategories returns an event_types bitmask with every
// category bit enabled and the severity bits set per BuildEventMask's rules.
func BuildEventMaskAllCategories(severity EventSeverity) uint64 {
	return BuildEventMask(nil, severity) | AMDSMI_EVENT_MASK_ALL_CATEGORIES
}

// BuildEventMaskAllSeverities returns an event_types bitmask with every
// severity bit enabled and the supplied category bits set.
func BuildEventMaskAllSeverities(categories []EventCategory) uint64 {
	var mask uint64
	for _, cat := range categories {
		mask |= uint64(1) << uint64(cat)
	}
	return mask | AMDSMI_EVENT_MASK_ALL_SEVERITIES
}

// EventCreate allocates a new event set monitoring the given processors for the
// event types described by the bitmask (see BuildEventMask).
// The returned EventSet must be released by EventDestroy.
func EventCreate(processors []ProcessorHandle, eventTypes uint64) (EventSet, error) {
	if len(processors) == 0 {
		return EventSet{}, &StatusError{Code: AMDSMI_STATUS_INVAL, Name: statusNames[AMDSMI_STATUS_INVAL]}
	}

	cHandles := make([]C.amdsmi_processor_handle, len(processors))
	for i, ph := range processors {
		cHandles[i] = ph.cPtr()
	}

	var set C.amdsmi_event_set
	ret := C.amdsmi_event_create(
		&cHandles[0],
		C.uint32_t(len(processors)),
		C.uint64_t(eventTypes),
		&set,
	)
	if err := checkStatus(Status(ret)); err != nil {
		return EventSet{}, err
	}
	return EventSet{raw: unsafe.Pointer(set)}, nil
}

// EventRead blocks until an event is delivered, the timeout (in microseconds)
// expires, or the call returns immediately when timeoutUsec == 0.
// A negative timeout blocks indefinitely. See amdsmi_event_read for details.
func EventRead(set EventSet, timeoutUsec int64) (EventEntry, error) {
	var entry C.amdsmi_event_entry_t
	ret := C.amdsmi_event_read(
		C.amdsmi_event_set(set.raw),
		C.int64_t(timeoutUsec),
		&entry,
	)
	if err := checkStatus(Status(ret)); err != nil {
		return EventEntry{}, err
	}
	return EventEntry{
		FcnId:           VfHandle{raw: uint64(entry.fcn_id.handle)},
		DevId:           uint64(entry.dev_id),
		Timestamp:       uint64(entry.timestamp),
		Data:            uint64(entry.data),
		Category:        EventCategory(entry.category),
		Subcode:         uint32(entry.subcode),
		Level:           EventSeverity(entry.level),
		Date:            C.GoString(&entry.date[0]),
		Message:         C.GoString(&entry.message[0]),
		ProcessorHandle: ProcessorHandle{raw: uintptr(entry.processor_handle)},
	}, nil
}

// EventDestroy releases the resources associated with an event set returned by
// EventCreate. It is safe to call on a zero-valued EventSet (no-op).
func EventDestroy(set EventSet) error {
	if set.raw == nil {
		return nil
	}
	ret := C.amdsmi_event_destroy(C.amdsmi_event_set(set.raw))
	return checkStatus(Status(ret))
}

// EventReader is a small lifetime wrapper around EventCreate / EventRead /
// EventDestroy.
type EventReader struct {
	set EventSet
}

// NewEventReader builds an EventReader monitoring the given processors for the
// supplied categories at or above severity. The bitmask is built via
// BuildEventMask, so an empty categories slice means "category bits = 0" -- pass
// AMDSMI_EVENT_SEVERITY_INFO (or similar) to still receive severity-tagged events.
func NewEventReader(processors []ProcessorHandle, categories []EventCategory, severity EventSeverity) (*EventReader, error) {
	return NewEventReaderMask(processors, BuildEventMask(categories, severity))
}

// NewEventReaderAllCategories builds an EventReader that listens to every
// category at or above the chosen severity (uses BuildEventMaskAllCategories).
func NewEventReaderAllCategories(processors []ProcessorHandle, severity EventSeverity) (*EventReader, error) {
	return NewEventReaderMask(processors, BuildEventMaskAllCategories(severity))
}

// NewEventReaderMask builds an EventReader from a precomputed event_types mask.
// Use this when you already have a uint64 (e.g. AMDSMI_MASK_ALL,
// AMDSMI_MASK_DEFAULT, or a hand-crafted combination).
func NewEventReaderMask(processors []ProcessorHandle, mask uint64) (*EventReader, error) {
	set, err := EventCreate(processors, mask)
	if err != nil {
		return nil, err
	}
	return &EventReader{set: set}, nil
}

// Set returns the underlying EventSet. Useful if you need to call EventRead /
// EventDestroy directly. The reader retains ownership unless you call Detach.
func (r *EventReader) Set() EventSet {
	if r == nil {
		return EventSet{}
	}
	return r.set
}

// Read blocks for up to timeoutUsec microseconds for the next event. A negative
// timeout blocks indefinitely; zero returns immediately. See EventRead.
func (r *EventReader) Read(timeoutUsec int64) (EventEntry, error) {
	if r == nil {
		return EventEntry{}, &StatusError{Code: AMDSMI_STATUS_INVAL, Name: statusNames[AMDSMI_STATUS_INVAL]}
	}
	return EventRead(r.set, timeoutUsec)
}

// Close destroys the underlying event set. Safe to call multiple times and on
// a nil receiver.
func (r *EventReader) Close() error {
	if r == nil || r.set.raw == nil {
		return nil
	}
	err := EventDestroy(r.set)
	r.set = EventSet{}
	return err
}

// -----------------------------------------------------------------------------
// Per-category event subcode enums (mirror amdsmi_event_<category>_t in amdsmi.h).
//
// EventEntry.Subcode is the raw uint32 returned by the library; cast it to the
// type matching EventEntry.Category to get the named constant. SubcodeName(cat,
// sub) returns the textual identifier for any (category, subcode) pair.
// -----------------------------------------------------------------------------

// EventGpuSubcode mirrors amdsmi_event_gpu_t.
type EventGpuSubcode uint32

const (
	AMDSMI_EVENT_GPU_DEVICE_LOST EventGpuSubcode = iota
	AMDSMI_EVENT_GPU_NOT_SUPPORTED
	AMDSMI_EVENT_GPU_RMA
	AMDSMI_EVENT_GPU_NOT_INITIALIZED
	AMDSMI_EVENT_GPU_MMSCH_ABNORMAL_STATE
	AMDSMI_EVENT_GPU_RLCV_ABNORMAL_STATE
	AMDSMI_EVENT_GPU_SDMA_ENGINE_BUSY
	AMDSMI_EVENT_GPU_RLC_ENGINE_BUSY
	AMDSMI_EVENT_GPU_GC_ENGINE_BUSY
	AMDSMI_EVENT_GPU__MAX
)

// EventDriverSubcode mirrors amdsmi_event_driver_t.
type EventDriverSubcode uint32

const (
	AMDSMI_EVENT_DRIVER_SPIN_LOCK_BUSY EventDriverSubcode = iota
	AMDSMI_EVENT_DRIVER_ALLOC_SYSTEM_MEM_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_GFX_WORKQUEUE_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_MM_WORKQUEUE_FAIL
	AMDSMI_EVENT_DRIVER_BUFFER_OVERFLOW

	AMDSMI_EVENT_DRIVER_DEV_INIT_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_THREAD_FAIL
	AMDSMI_EVENT_DRIVER_NO_ACCESS_PCI_REGION
	AMDSMI_EVENT_DRIVER_MMIO_FAIL
	AMDSMI_EVENT_DRIVER_INTERRUPT_INIT_FAIL

	AMDSMI_EVENT_DRIVER_INVALID_VALUE
	AMDSMI_EVENT_DRIVER_CREATE_MUTEX_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_TIMER_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_EVENT_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_SPIN_LOCK_FAIL

	AMDSMI_EVENT_DRIVER_ALLOC_FB_MEM_FAIL
	AMDSMI_EVENT_DRIVER_ALLOC_DMA_MEM_FAIL
	AMDSMI_EVENT_DRIVER_NO_FB_MANAGER
	AMDSMI_EVENT_DRIVER_HW_INIT_FAIL
	AMDSMI_EVENT_DRIVER_SW_INIT_FAIL

	AMDSMI_EVENT_DRIVER_INIT_CONFIG_ERROR
	AMDSMI_EVENT_DRIVER_ERROR_LOGGING_FAILED
	AMDSMI_EVENT_DRIVER_CREATE_RWLOCK_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_RWSEMA_FAIL
	AMDSMI_EVENT_DRIVER_GET_READ_LOCK_FAIL

	AMDSMI_EVENT_DRIVER_GET_WRITE_LOCK_FAIL
	AMDSMI_EVENT_DRIVER_GET_READ_SEMA_FAIL
	AMDSMI_EVENT_DRIVER_GET_WRITE_SEMA_FAIL

	AMDSMI_EVENT_DRIVER_DIAG_DATA_INIT_FAIL
	AMDSMI_EVENT_DRIVER_DIAG_DATA_MEM_REQ_FAIL
	AMDSMI_EVENT_DRIVER_DIAG_DATA_VADDR_REQ_FAIL
	AMDSMI_EVENT_DRIVER_DIAG_DATA_BUS_ADDR_REQ_FAIL

	AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_INIT_FAIL
	AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_MEM_REQ_FAIL
	AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_VADDR_REQ_FAIL
	AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_BUS_ADDR_REQ_FAIL

	AMDSMI_EVENT_DRIVER_HRTIMER_START_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_DRIVER_FILE_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_DEVICE_FILE_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_DEBUGFS_FILE_FAIL
	AMDSMI_EVENT_DRIVER_CREATE_DEBUGFS_DIR_FAIL

	AMDSMI_EVENT_DRIVER_PCI_ENABLE_DEVICE_FAIL
	AMDSMI_EVENT_DRIVER_FB_MAP_FAIL
	AMDSMI_EVENT_DRIVER_DOORBELL_MAP_FAIL
	AMDSMI_EVENT_DRIVER_PCI_REGISTER_DRIVER_FAIL

	AMDSMI_EVENT_DRIVER_ALLOC_IOVA_ALIGN_FAIL

	AMDSMI_EVENT_DRIVER_ROM_MAP_FAIL
	AMDSMI_EVENT_DRIVER_FULL_ACCESS_TIMEOUT

	AMDSMI_EVENT_DRIVER__MAX
)

// EventFwSubcode mirrors amdsmi_event_fw_t.
type EventFwSubcode uint32

const (
	AMDSMI_EVENT_FW_CMD_ALLOC_BUF_FAIL EventFwSubcode = iota
	AMDSMI_EVENT_FW_CMD_BUF_PREP_FAIL
	AMDSMI_EVENT_FW_RING_INIT_FAIL
	AMDSMI_EVENT_FW_FW_APPLY_SECURITY_POLICY_FAIL
	AMDSMI_EVENT_FW_START_RING_FAIL

	AMDSMI_EVENT_FW_FW_LOAD_FAIL
	AMDSMI_EVENT_FW_EXIT_FAIL
	AMDSMI_EVENT_FW_INIT_FAIL
	AMDSMI_EVENT_FW_CMD_SUBMIT_FAIL
	AMDSMI_EVENT_FW_CMD_FENCE_WAIT_FAIL

	AMDSMI_EVENT_FW_TMR_LOAD_FAIL
	AMDSMI_EVENT_FW_TOC_LOAD_FAIL
	AMDSMI_EVENT_FW_RAS_LOAD_FAIL
	AMDSMI_EVENT_FW_RAS_UNLOAD_FAIL
	AMDSMI_EVENT_FW_RAS_TA_INVOKE_FAIL
	AMDSMI_EVENT_FW_RAS_TA_ERR_INJECT_FAIL

	AMDSMI_EVENT_FW_ASD_LOAD_FAIL
	AMDSMI_EVENT_FW_ASD_UNLOAD_FAIL
	AMDSMI_EVENT_FW_AUTOLOAD_FAIL
	AMDSMI_EVENT_FW_VFGATE_FAIL

	AMDSMI_EVENT_FW_XGMI_LOAD_FAIL
	AMDSMI_EVENT_FW_XGMI_UNLOAD_FAIL
	AMDSMI_EVENT_FW_XGMI_TA_INVOKE_FAIL

	AMDSMI_EVENT_FW_TMR_INIT_FAIL
	AMDSMI_EVENT_FW_NOT_SUPPORTED_FEATURE
	AMDSMI_EVENT_FW_GET_PSP_TRACELOG_FAIL

	AMDSMI_EVENT_FW_SET_SNAPSHOT_ADDR_FAIL
	AMDSMI_EVENT_FW_SNAPSHOT_TRIGGER_FAIL

	AMDSMI_EVENT_FW_MIGRATION_GET_PSP_INFO_FAIL
	AMDSMI_EVENT_FW_MIGRATION_EXPORT_FAIL
	AMDSMI_EVENT_FW_MIGRATION_IMPORT_FAIL

	AMDSMI_EVENT_FW_BL_FAIL
	AMDSMI_EVENT_FW_RAS_BOOT_FAIL
	AMDSMI_EVENT_FW_MAILBOX_ERROR

	AMDSMI_EVENT_FW__MAX
)

// AMDSMI_EVENT_FW_FW_INIT_FAIL is a C alias for AMDSMI_EVENT_FW_RING_INIT_FAIL.
const AMDSMI_EVENT_FW_FW_INIT_FAIL = AMDSMI_EVENT_FW_RING_INIT_FAIL

// EventResetSubcode mirrors amdsmi_event_reset_t.
type EventResetSubcode uint32

const (
	AMDSMI_EVENT_RESET_GPU EventResetSubcode = iota
	AMDSMI_EVENT_RESET_GPU_FAILED
	AMDSMI_EVENT_RESET_FLR
	AMDSMI_EVENT_RESET_FLR_FAILED
	AMDSMI_EVENT_RESET__MAX
)

// EventIovSubcode mirrors amdsmi_event_iov_t.
type EventIovSubcode uint32

const (
	AMDSMI_EVENT_IOV_NO_GPU_IOV_CAP EventIovSubcode = iota
	AMDSMI_EVENT_IOV_ASIC_NO_SRIOV_SUPPORT
	AMDSMI_EVENT_IOV_ENABLE_SRIOV_FAIL
	AMDSMI_EVENT_IOV_CMD_TIMEOUT
	AMDSMI_EVENT_IOV_CMD_ERROR

	AMDSMI_EVENT_IOV_INIT_IV_RING_FAIL
	AMDSMI_EVENT_IOV_SRIOV_STRIDE_ERROR
	AMDSMI_EVENT_IOV_WS_SAVE_TIMEOUT
	AMDSMI_EVENT_IOV_WS_IDLE_TIMEOUT
	AMDSMI_EVENT_IOV_WS_RUN_TIMEOUT
	AMDSMI_EVENT_IOV_WS_LOAD_TIMEOUT
	AMDSMI_EVENT_IOV_WS_SHUTDOWN_TIMEOUT
	AMDSMI_EVENT_IOV_WS_ALREADY_SHUTDOWN
	AMDSMI_EVENT_IOV_WS_INFINITE_LOOP
	AMDSMI_EVENT_IOV_WS_REENTRANT_ERROR
	AMDSMI_EVENT_IOV__MAX
)

// EventEccSubcode mirrors amdsmi_event_ecc_t.
type EventEccSubcode uint32

const (
	AMDSMI_EVENT_ECC_UCE EventEccSubcode = iota
	AMDSMI_EVENT_ECC_CE
	AMDSMI_EVENT_ECC_IN_PF_FB
	AMDSMI_EVENT_ECC_IN_CRI_REG
	AMDSMI_EVENT_ECC_IN_VF_CRI
	AMDSMI_EVENT_ECC_REACH_THD
	AMDSMI_EVENT_ECC_VF_CE
	AMDSMI_EVENT_ECC_VF_UE
	AMDSMI_EVENT_ECC_IN_SAME_ROW
	AMDSMI_EVENT_ECC_UMC_UE
	AMDSMI_EVENT_ECC_GFX_CE
	AMDSMI_EVENT_ECC_GFX_UE
	AMDSMI_EVENT_ECC_SDMA_CE
	AMDSMI_EVENT_ECC_SDMA_UE
	AMDSMI_EVENT_ECC_GFX_CE_TOTAL
	AMDSMI_EVENT_ECC_GFX_UE_TOTAL
	AMDSMI_EVENT_ECC_SDMA_CE_TOTAL
	AMDSMI_EVENT_ECC_SDMA_UE_TOTAL
	AMDSMI_EVENT_ECC_UMC_CE_TOTAL
	AMDSMI_EVENT_ECC_UMC_UE_TOTAL
	AMDSMI_EVENT_ECC_MMHUB_CE
	AMDSMI_EVENT_ECC_MMHUB_UE
	AMDSMI_EVENT_ECC_MMHUB_CE_TOTAL
	AMDSMI_EVENT_ECC_MMHUB_UE_TOTAL
	AMDSMI_EVENT_ECC_XGMI_WAFL_CE
	AMDSMI_EVENT_ECC_XGMI_WAFL_UE
	AMDSMI_EVENT_ECC_XGMI_WAFL_CE_TOTAL
	AMDSMI_EVENT_ECC_XGMI_WAFL_UE_TOTAL
	AMDSMI_EVENT_ECC_FATAL_ERROR
	AMDSMI_EVENT_ECC_POISON_CONSUMPTION
	AMDSMI_EVENT_ECC_ACA_DUMP
	AMDSMI_EVENT_ECC_WRONG_SOCKET_ID
	AMDSMI_EVENT_ECC_ACA_UNKNOWN_BLOCK_INSTANCE
	AMDSMI_EVENT_ECC_UNKNOWN_CHIPLET_CE
	AMDSMI_EVENT_ECC_UNKNOWN_CHIPLET_UE
	AMDSMI_EVENT_ECC_UMC_CHIPLET_CE
	AMDSMI_EVENT_ECC_UMC_CHIPLET_UE
	AMDSMI_EVENT_ECC_GFX_CHIPLET_CE
	AMDSMI_EVENT_ECC_GFX_CHIPLET_UE
	AMDSMI_EVENT_ECC_SDMA_CHIPLET_CE
	AMDSMI_EVENT_ECC_SDMA_CHIPLET_UE
	AMDSMI_EVENT_ECC_MMHUB_CHIPLET_CE
	AMDSMI_EVENT_ECC_MMHUB_CHIPLET_UE
	AMDSMI_EVENT_ECC_XGMI_WAFL_CHIPLET_CE
	AMDSMI_EVENT_ECC_XGMI_WAFL_CHIPLET_UE
	AMDSMI_EVENT_ECC_EEPROM_ENTRIES_FOUND
	AMDSMI_EVENT_ECC_UMC_DE
	AMDSMI_EVENT_ECC_UMC_DE_TOTAL
	AMDSMI_EVENT_ECC_UNKNOWN
	AMDSMI_EVENT_ECC_EEPROM_REACH_THD
	AMDSMI_EVENT_ECC_UMC_CHIPLET_DE
	AMDSMI_EVENT_ECC_UNKNOWN_CHIPLET_DE
	AMDSMI_EVENT_ECC_EEPROM_CHK_MISMATCH
	AMDSMI_EVENT_ECC_EEPROM_RESET
	AMDSMI_EVENT_ECC_EEPROM_RESET_FAILED
	AMDSMI_EVENT_ECC_EEPROM_APPEND
	AMDSMI_EVENT_ECC_THD_CHANGED
	AMDSMI_EVENT_ECC_DUP_ENTRIES
	AMDSMI_EVENT_ECC_EEPROM_WRONG_HDR
	AMDSMI_EVENT_ECC_EEPROM_WRONG_VER
	AMDSMI_EVENT_ECC__MAX
)

// EventPpSubcode mirrors amdsmi_event_pp_t.
type EventPpSubcode uint32

const (
	AMDSMI_EVENT_PP_SET_DPM_POLICY_FAIL EventPpSubcode = iota
	AMDSMI_EVENT_PP_ACTIVATE_DPM_POLICY_FAIL
	AMDSMI_EVENT_PP_I2C_SLAVE_NOT_PRESENT
	AMDSMI_EVENT_PP_THROTTLER_EVENT
	AMDSMI_EVENT_PP__MAX
)

// EventSchedSubcode mirrors amdsmi_event_sched_t.
type EventSchedSubcode uint32

const (
	AMDSMI_EVENT_SCHED_WORLD_SWITCH_FAIL EventSchedSubcode = iota
	AMDSMI_EVENT_SCHED_DISABLE_AUTO_HW_SWITCH_FAIL
	AMDSMI_EVENT_SCHED_ENABLE_AUTO_HW_SWITCH_FAIL
	AMDSMI_EVENT_SCHED_GFX_SAVE_REG_FAIL
	AMDSMI_EVENT_SCHED_GFX_IDLE_REG_FAIL

	AMDSMI_EVENT_SCHED_GFX_RUN_REG_FAIL
	AMDSMI_EVENT_SCHED_GFX_LOAD_REG_FAIL
	AMDSMI_EVENT_SCHED_GFX_INIT_REG_FAIL
	AMDSMI_EVENT_SCHED_MM_SAVE_REG_FAIL
	AMDSMI_EVENT_SCHED_MM_IDLE_REG_FAIL

	AMDSMI_EVENT_SCHED_MM_RUN_REG_FAIL
	AMDSMI_EVENT_SCHED_MM_LOAD_REG_FAIL
	AMDSMI_EVENT_SCHED_MM_INIT_REG_FAIL
	AMDSMI_EVENT_SCHED_INIT_GPU_FAIL
	AMDSMI_EVENT_SCHED_RUN_GPU_FAIL

	AMDSMI_EVENT_SCHED_SAVE_GPU_STATE_FAIL
	AMDSMI_EVENT_SCHED_LOAD_GPU_STATE_FAIL
	AMDSMI_EVENT_SCHED_IDLE_GPU_FAIL
	AMDSMI_EVENT_SCHED_FINI_GPU_FAIL
	AMDSMI_EVENT_SCHED_DEAD_VF

	AMDSMI_EVENT_SCHED_EVENT_QUEUE_FULL
	AMDSMI_EVENT_SCHED_SHUTDOWN_VF_FAIL
	AMDSMI_EVENT_SCHED_RESET_VF_NUM_FAIL
	AMDSMI_EVENT_SCHED_IGNORE_EVENT
	AMDSMI_EVENT_SCHED_PF_SWITCH_FAIL
	AMDSMI_EVENT_SCHED__MAX
)

// EventVfSubcode mirrors amdsmi_event_vf_max_t.
type EventVfSubcode uint32

const (
	AMDSMI_EVENT_VF_ATOMBIOS_INIT_FAIL EventVfSubcode = iota
	AMDSMI_EVENT_VF_NO_VBIOS
	AMDSMI_EVENT_VF_GPU_POST_ERROR
	AMDSMI_EVENT_VF_ATOMBIOS_GET_CLOCK_FAIL
	AMDSMI_EVENT_VF_FENCE_INIT_FAIL
	AMDSMI_EVENT_VF_AMDGPU_INIT_FAIL
	AMDSMI_EVENT_VF_IB_INIT_FAIL
	AMDSMI_EVENT_VF_AMDGPU_LATE_INIT_FAIL
	AMDSMI_EVENT_VF_ASIC_RESUME_FAIL
	AMDSMI_EVENT_VF_GPU_RESET_FAIL
	AMDSMI_EVENT_VF__MAX
)

// EventVbiosSubcode mirrors amdsmi_event_vbios_t.
type EventVbiosSubcode uint32

const (
	AMDSMI_EVENT_VBIOS_INVALID EventVbiosSubcode = iota
	AMDSMI_EVENT_VBIOS_IMAGE_MISSING
	AMDSMI_EVENT_VBIOS_CHECKSUM_ERR
	AMDSMI_EVENT_VBIOS_POST_FAIL
	AMDSMI_EVENT_VBIOS_READ_FAIL

	AMDSMI_EVENT_VBIOS_READ_IMG_HEADER_FAIL
	AMDSMI_EVENT_VBIOS_READ_IMG_SIZE_FAIL
	AMDSMI_EVENT_VBIOS_GET_FW_INFO_FAIL
	AMDSMI_EVENT_VBIOS_GET_TBL_REVISION_FAIL
	AMDSMI_EVENT_VBIOS_PARSER_TBL_FAIL

	AMDSMI_EVENT_VBIOS_IP_DISCOVERY_FAIL
	AMDSMI_EVENT_VBIOS_TIMEOUT
	AMDSMI_EVENT_VBIOS_HASH_INVALID
	AMDSMI_EVENT_VBIOS_HASH_UPDATED
	AMDSMI_EVENT_VBIOS_IP_DISCOVERY_BINARY_CHECKSUM_FAIL
	AMDSMI_EVENT_VBIOS_IP_DISCOVERY_TABLE_CHECKSUM_FAIL
	AMDSMI_EVENT_VBIOS__MAX
)

// EventGuardSubcode mirrors amdsmi_event_guard_t.
type EventGuardSubcode uint32

const (
	AMDSMI_EVENT_GUARD_RESET_FAIL EventGuardSubcode = iota
	AMDSMI_EVENT_GUARD_EVENT_OVERFLOW
	AMDSMI_EVENT_GUARD__MAX
)

// EventGpumonSubcode mirrors amdsmi_event_gpumon_t.
type EventGpumonSubcode uint32

const (
	AMDSMI_EVENT_GPUMON_INVALID_OPTION EventGpumonSubcode = iota
	AMDSMI_EVENT_GPUMON_INVALID_VF_INDEX
	AMDSMI_EVENT_GPUMON_INVALID_FB_SIZE
	AMDSMI_EVENT_GPUMON_NO_SUITABLE_SPACE
	AMDSMI_EVENT_GPUMON_NO_AVAILABLE_SLOT

	AMDSMI_EVENT_GPUMON_OVERSIZE_ALLOCATION
	AMDSMI_EVENT_GPUMON_OVERLAPPING_FB
	AMDSMI_EVENT_GPUMON_INVALID_GFX_TIMESLICE
	AMDSMI_EVENT_GPUMON_INVALID_MM_TIMESLICE
	AMDSMI_EVENT_GPUMON_INVALID_GFX_PART

	AMDSMI_EVENT_GPUMON_VF_BUSY
	AMDSMI_EVENT_GPUMON_INVALID_VF_NUM
	AMDSMI_EVENT_GPUMON_NOT_SUPPORTED
	AMDSMI_EVENT_GPUMON__MAX
)

// EventMmschSubcode mirrors amdsmi_event_mmsch_t.
type EventMmschSubcode uint32

const (
	AMDSMI_EVENT_MMSCH_IGNORED_JOB EventMmschSubcode = iota
	AMDSMI_EVENT_MMSCH_UNSUPPORTED_VCN_FW
	AMDSMI_EVENT_MMSCH__MAX
)

// EventXgmiSubcode mirrors amdsmi_event_xgmi_t.
type EventXgmiSubcode uint32

const (
	AMDSMI_EVENT_XGMI_TOPOLOGY_UPDATE_FAILED EventXgmiSubcode = iota
	AMDSMI_EVENT_XGMI_TOPOLOGY_HW_INIT_UPDATE
	AMDSMI_EVENT_XGMI_TOPOLOGY_UPDATE_DONE
	AMDSMI_EVENT_XGMI_FB_SHARING_SETTING_ERROR
	AMDSMI_EVENT_XGMI_FB_SHARING_SETTING_RESET
	AMDSMI_EVENT_XGMI__MAX
)

// PpThrottlerType mirrors amdsmi_pp_throttler_type_t and identifies which
// throttler raised an AMDSMI_EVENT_PP_THROTTLER_EVENT. The throttler type is
// reported in EventEntry.Data when Category is AMDSMI_EVENT_CATEGORY_PP and
// Subcode is AMDSMI_EVENT_PP_THROTTLER_EVENT.
type PpThrottlerType uint32

const (
	AMDSMI_EVENT_THROTTLER_PROCHOT PpThrottlerType = iota
	AMDSMI_EVENT_THROTTLER_SOCKET
	AMDSMI_EVENT_THROTTLER_VR
	AMDSMI_EVENT_THROTTLER_HBM
)

// Per-category String() methods. Each falls through to a generic UNKNOWN(N).

func (s EventGpuSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_GPU_DEVICE_LOST:
		return "GPU_DEVICE_LOST"
	case AMDSMI_EVENT_GPU_NOT_SUPPORTED:
		return "GPU_NOT_SUPPORTED"
	case AMDSMI_EVENT_GPU_RMA:
		return "GPU_RMA"
	case AMDSMI_EVENT_GPU_NOT_INITIALIZED:
		return "GPU_NOT_INITIALIZED"
	case AMDSMI_EVENT_GPU_MMSCH_ABNORMAL_STATE:
		return "GPU_MMSCH_ABNORMAL_STATE"
	case AMDSMI_EVENT_GPU_RLCV_ABNORMAL_STATE:
		return "GPU_RLCV_ABNORMAL_STATE"
	case AMDSMI_EVENT_GPU_SDMA_ENGINE_BUSY:
		return "GPU_SDMA_ENGINE_BUSY"
	case AMDSMI_EVENT_GPU_RLC_ENGINE_BUSY:
		return "GPU_RLC_ENGINE_BUSY"
	case AMDSMI_EVENT_GPU_GC_ENGINE_BUSY:
		return "GPU_GC_ENGINE_BUSY"
	}
	return fmt.Sprintf("GPU_UNKNOWN(%d)", uint32(s))
}

func (s EventDriverSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_DRIVER_SPIN_LOCK_BUSY:
		return "DRIVER_SPIN_LOCK_BUSY"
	case AMDSMI_EVENT_DRIVER_ALLOC_SYSTEM_MEM_FAIL:
		return "DRIVER_ALLOC_SYSTEM_MEM_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_GFX_WORKQUEUE_FAIL:
		return "DRIVER_CREATE_GFX_WORKQUEUE_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_MM_WORKQUEUE_FAIL:
		return "DRIVER_CREATE_MM_WORKQUEUE_FAIL"
	case AMDSMI_EVENT_DRIVER_BUFFER_OVERFLOW:
		return "DRIVER_BUFFER_OVERFLOW"
	case AMDSMI_EVENT_DRIVER_DEV_INIT_FAIL:
		return "DRIVER_DEV_INIT_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_THREAD_FAIL:
		return "DRIVER_CREATE_THREAD_FAIL"
	case AMDSMI_EVENT_DRIVER_NO_ACCESS_PCI_REGION:
		return "DRIVER_NO_ACCESS_PCI_REGION"
	case AMDSMI_EVENT_DRIVER_MMIO_FAIL:
		return "DRIVER_MMIO_FAIL"
	case AMDSMI_EVENT_DRIVER_INTERRUPT_INIT_FAIL:
		return "DRIVER_INTERRUPT_INIT_FAIL"
	case AMDSMI_EVENT_DRIVER_INVALID_VALUE:
		return "DRIVER_INVALID_VALUE"
	case AMDSMI_EVENT_DRIVER_CREATE_MUTEX_FAIL:
		return "DRIVER_CREATE_MUTEX_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_TIMER_FAIL:
		return "DRIVER_CREATE_TIMER_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_EVENT_FAIL:
		return "DRIVER_CREATE_EVENT_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_SPIN_LOCK_FAIL:
		return "DRIVER_CREATE_SPIN_LOCK_FAIL"
	case AMDSMI_EVENT_DRIVER_ALLOC_FB_MEM_FAIL:
		return "DRIVER_ALLOC_FB_MEM_FAIL"
	case AMDSMI_EVENT_DRIVER_ALLOC_DMA_MEM_FAIL:
		return "DRIVER_ALLOC_DMA_MEM_FAIL"
	case AMDSMI_EVENT_DRIVER_NO_FB_MANAGER:
		return "DRIVER_NO_FB_MANAGER"
	case AMDSMI_EVENT_DRIVER_HW_INIT_FAIL:
		return "DRIVER_HW_INIT_FAIL"
	case AMDSMI_EVENT_DRIVER_SW_INIT_FAIL:
		return "DRIVER_SW_INIT_FAIL"
	case AMDSMI_EVENT_DRIVER_INIT_CONFIG_ERROR:
		return "DRIVER_INIT_CONFIG_ERROR"
	case AMDSMI_EVENT_DRIVER_ERROR_LOGGING_FAILED:
		return "DRIVER_ERROR_LOGGING_FAILED"
	case AMDSMI_EVENT_DRIVER_CREATE_RWLOCK_FAIL:
		return "DRIVER_CREATE_RWLOCK_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_RWSEMA_FAIL:
		return "DRIVER_CREATE_RWSEMA_FAIL"
	case AMDSMI_EVENT_DRIVER_GET_READ_LOCK_FAIL:
		return "DRIVER_GET_READ_LOCK_FAIL"
	case AMDSMI_EVENT_DRIVER_GET_WRITE_LOCK_FAIL:
		return "DRIVER_GET_WRITE_LOCK_FAIL"
	case AMDSMI_EVENT_DRIVER_GET_READ_SEMA_FAIL:
		return "DRIVER_GET_READ_SEMA_FAIL"
	case AMDSMI_EVENT_DRIVER_GET_WRITE_SEMA_FAIL:
		return "DRIVER_GET_WRITE_SEMA_FAIL"
	case AMDSMI_EVENT_DRIVER_DIAG_DATA_INIT_FAIL:
		return "DRIVER_DIAG_DATA_INIT_FAIL"
	case AMDSMI_EVENT_DRIVER_DIAG_DATA_MEM_REQ_FAIL:
		return "DRIVER_DIAG_DATA_MEM_REQ_FAIL"
	case AMDSMI_EVENT_DRIVER_DIAG_DATA_VADDR_REQ_FAIL:
		return "DRIVER_DIAG_DATA_VADDR_REQ_FAIL"
	case AMDSMI_EVENT_DRIVER_DIAG_DATA_BUS_ADDR_REQ_FAIL:
		return "DRIVER_DIAG_DATA_BUS_ADDR_REQ_FAIL"
	case AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_INIT_FAIL:
		return "DRIVER_REMOTE_DEBUG_INIT_FAIL"
	case AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_MEM_REQ_FAIL:
		return "DRIVER_REMOTE_DEBUG_MEM_REQ_FAIL"
	case AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_VADDR_REQ_FAIL:
		return "DRIVER_REMOTE_DEBUG_VADDR_REQ_FAIL"
	case AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_BUS_ADDR_REQ_FAIL:
		return "DRIVER_REMOTE_DEBUG_BUS_ADDR_REQ_FAIL"
	case AMDSMI_EVENT_DRIVER_HRTIMER_START_FAIL:
		return "DRIVER_HRTIMER_START_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_DRIVER_FILE_FAIL:
		return "DRIVER_CREATE_DRIVER_FILE_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_DEVICE_FILE_FAIL:
		return "DRIVER_CREATE_DEVICE_FILE_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_DEBUGFS_FILE_FAIL:
		return "DRIVER_CREATE_DEBUGFS_FILE_FAIL"
	case AMDSMI_EVENT_DRIVER_CREATE_DEBUGFS_DIR_FAIL:
		return "DRIVER_CREATE_DEBUGFS_DIR_FAIL"
	case AMDSMI_EVENT_DRIVER_PCI_ENABLE_DEVICE_FAIL:
		return "DRIVER_PCI_ENABLE_DEVICE_FAIL"
	case AMDSMI_EVENT_DRIVER_FB_MAP_FAIL:
		return "DRIVER_FB_MAP_FAIL"
	case AMDSMI_EVENT_DRIVER_DOORBELL_MAP_FAIL:
		return "DRIVER_DOORBELL_MAP_FAIL"
	case AMDSMI_EVENT_DRIVER_PCI_REGISTER_DRIVER_FAIL:
		return "DRIVER_PCI_REGISTER_DRIVER_FAIL"
	case AMDSMI_EVENT_DRIVER_ALLOC_IOVA_ALIGN_FAIL:
		return "DRIVER_ALLOC_IOVA_ALIGN_FAIL"
	case AMDSMI_EVENT_DRIVER_ROM_MAP_FAIL:
		return "DRIVER_ROM_MAP_FAIL"
	case AMDSMI_EVENT_DRIVER_FULL_ACCESS_TIMEOUT:
		return "DRIVER_FULL_ACCESS_TIMEOUT"
	}
	return fmt.Sprintf("DRIVER_UNKNOWN(%d)", uint32(s))
}

func (s EventFwSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_FW_CMD_ALLOC_BUF_FAIL:
		return "FW_CMD_ALLOC_BUF_FAIL"
	case AMDSMI_EVENT_FW_CMD_BUF_PREP_FAIL:
		return "FW_CMD_BUF_PREP_FAIL"
	case AMDSMI_EVENT_FW_RING_INIT_FAIL:
		return "FW_RING_INIT_FAIL" // also AMDSMI_EVENT_FW_FW_INIT_FAIL
	case AMDSMI_EVENT_FW_FW_APPLY_SECURITY_POLICY_FAIL:
		return "FW_FW_APPLY_SECURITY_POLICY_FAIL"
	case AMDSMI_EVENT_FW_START_RING_FAIL:
		return "FW_START_RING_FAIL"
	case AMDSMI_EVENT_FW_FW_LOAD_FAIL:
		return "FW_FW_LOAD_FAIL"
	case AMDSMI_EVENT_FW_EXIT_FAIL:
		return "FW_EXIT_FAIL"
	case AMDSMI_EVENT_FW_INIT_FAIL:
		return "FW_INIT_FAIL"
	case AMDSMI_EVENT_FW_CMD_SUBMIT_FAIL:
		return "FW_CMD_SUBMIT_FAIL"
	case AMDSMI_EVENT_FW_CMD_FENCE_WAIT_FAIL:
		return "FW_CMD_FENCE_WAIT_FAIL"
	case AMDSMI_EVENT_FW_TMR_LOAD_FAIL:
		return "FW_TMR_LOAD_FAIL"
	case AMDSMI_EVENT_FW_TOC_LOAD_FAIL:
		return "FW_TOC_LOAD_FAIL"
	case AMDSMI_EVENT_FW_RAS_LOAD_FAIL:
		return "FW_RAS_LOAD_FAIL"
	case AMDSMI_EVENT_FW_RAS_UNLOAD_FAIL:
		return "FW_RAS_UNLOAD_FAIL"
	case AMDSMI_EVENT_FW_RAS_TA_INVOKE_FAIL:
		return "FW_RAS_TA_INVOKE_FAIL"
	case AMDSMI_EVENT_FW_RAS_TA_ERR_INJECT_FAIL:
		return "FW_RAS_TA_ERR_INJECT_FAIL"
	case AMDSMI_EVENT_FW_ASD_LOAD_FAIL:
		return "FW_ASD_LOAD_FAIL"
	case AMDSMI_EVENT_FW_ASD_UNLOAD_FAIL:
		return "FW_ASD_UNLOAD_FAIL"
	case AMDSMI_EVENT_FW_AUTOLOAD_FAIL:
		return "FW_AUTOLOAD_FAIL"
	case AMDSMI_EVENT_FW_VFGATE_FAIL:
		return "FW_VFGATE_FAIL"
	case AMDSMI_EVENT_FW_XGMI_LOAD_FAIL:
		return "FW_XGMI_LOAD_FAIL"
	case AMDSMI_EVENT_FW_XGMI_UNLOAD_FAIL:
		return "FW_XGMI_UNLOAD_FAIL"
	case AMDSMI_EVENT_FW_XGMI_TA_INVOKE_FAIL:
		return "FW_XGMI_TA_INVOKE_FAIL"
	case AMDSMI_EVENT_FW_TMR_INIT_FAIL:
		return "FW_TMR_INIT_FAIL"
	case AMDSMI_EVENT_FW_NOT_SUPPORTED_FEATURE:
		return "FW_NOT_SUPPORTED_FEATURE"
	case AMDSMI_EVENT_FW_GET_PSP_TRACELOG_FAIL:
		return "FW_GET_PSP_TRACELOG_FAIL"
	case AMDSMI_EVENT_FW_SET_SNAPSHOT_ADDR_FAIL:
		return "FW_SET_SNAPSHOT_ADDR_FAIL"
	case AMDSMI_EVENT_FW_SNAPSHOT_TRIGGER_FAIL:
		return "FW_SNAPSHOT_TRIGGER_FAIL"
	case AMDSMI_EVENT_FW_MIGRATION_GET_PSP_INFO_FAIL:
		return "FW_MIGRATION_GET_PSP_INFO_FAIL"
	case AMDSMI_EVENT_FW_MIGRATION_EXPORT_FAIL:
		return "FW_MIGRATION_EXPORT_FAIL"
	case AMDSMI_EVENT_FW_MIGRATION_IMPORT_FAIL:
		return "FW_MIGRATION_IMPORT_FAIL"
	case AMDSMI_EVENT_FW_BL_FAIL:
		return "FW_BL_FAIL"
	case AMDSMI_EVENT_FW_RAS_BOOT_FAIL:
		return "FW_RAS_BOOT_FAIL"
	case AMDSMI_EVENT_FW_MAILBOX_ERROR:
		return "FW_MAILBOX_ERROR"
	}
	return fmt.Sprintf("FW_UNKNOWN(%d)", uint32(s))
}

func (s EventResetSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_RESET_GPU:
		return "RESET_GPU"
	case AMDSMI_EVENT_RESET_GPU_FAILED:
		return "RESET_GPU_FAILED"
	case AMDSMI_EVENT_RESET_FLR:
		return "RESET_FLR"
	case AMDSMI_EVENT_RESET_FLR_FAILED:
		return "RESET_FLR_FAILED"
	}
	return fmt.Sprintf("RESET_UNKNOWN(%d)", uint32(s))
}

func (s EventIovSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_IOV_NO_GPU_IOV_CAP:
		return "IOV_NO_GPU_IOV_CAP"
	case AMDSMI_EVENT_IOV_ASIC_NO_SRIOV_SUPPORT:
		return "IOV_ASIC_NO_SRIOV_SUPPORT"
	case AMDSMI_EVENT_IOV_ENABLE_SRIOV_FAIL:
		return "IOV_ENABLE_SRIOV_FAIL"
	case AMDSMI_EVENT_IOV_CMD_TIMEOUT:
		return "IOV_CMD_TIMEOUT"
	case AMDSMI_EVENT_IOV_CMD_ERROR:
		return "IOV_CMD_ERROR"
	case AMDSMI_EVENT_IOV_INIT_IV_RING_FAIL:
		return "IOV_INIT_IV_RING_FAIL"
	case AMDSMI_EVENT_IOV_SRIOV_STRIDE_ERROR:
		return "IOV_SRIOV_STRIDE_ERROR"
	case AMDSMI_EVENT_IOV_WS_SAVE_TIMEOUT:
		return "IOV_WS_SAVE_TIMEOUT"
	case AMDSMI_EVENT_IOV_WS_IDLE_TIMEOUT:
		return "IOV_WS_IDLE_TIMEOUT"
	case AMDSMI_EVENT_IOV_WS_RUN_TIMEOUT:
		return "IOV_WS_RUN_TIMEOUT"
	case AMDSMI_EVENT_IOV_WS_LOAD_TIMEOUT:
		return "IOV_WS_LOAD_TIMEOUT"
	case AMDSMI_EVENT_IOV_WS_SHUTDOWN_TIMEOUT:
		return "IOV_WS_SHUTDOWN_TIMEOUT"
	case AMDSMI_EVENT_IOV_WS_ALREADY_SHUTDOWN:
		return "IOV_WS_ALREADY_SHUTDOWN"
	case AMDSMI_EVENT_IOV_WS_INFINITE_LOOP:
		return "IOV_WS_INFINITE_LOOP"
	case AMDSMI_EVENT_IOV_WS_REENTRANT_ERROR:
		return "IOV_WS_REENTRANT_ERROR"
	}
	return fmt.Sprintf("IOV_UNKNOWN(%d)", uint32(s))
}

func (s EventEccSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_ECC_UCE:
		return "ECC_UCE"
	case AMDSMI_EVENT_ECC_CE:
		return "ECC_CE"
	case AMDSMI_EVENT_ECC_IN_PF_FB:
		return "ECC_IN_PF_FB"
	case AMDSMI_EVENT_ECC_IN_CRI_REG:
		return "ECC_IN_CRI_REG"
	case AMDSMI_EVENT_ECC_IN_VF_CRI:
		return "ECC_IN_VF_CRI"
	case AMDSMI_EVENT_ECC_REACH_THD:
		return "ECC_REACH_THD"
	case AMDSMI_EVENT_ECC_VF_CE:
		return "ECC_VF_CE"
	case AMDSMI_EVENT_ECC_VF_UE:
		return "ECC_VF_UE"
	case AMDSMI_EVENT_ECC_IN_SAME_ROW:
		return "ECC_IN_SAME_ROW"
	case AMDSMI_EVENT_ECC_UMC_UE:
		return "ECC_UMC_UE"
	case AMDSMI_EVENT_ECC_GFX_CE:
		return "ECC_GFX_CE"
	case AMDSMI_EVENT_ECC_GFX_UE:
		return "ECC_GFX_UE"
	case AMDSMI_EVENT_ECC_SDMA_CE:
		return "ECC_SDMA_CE"
	case AMDSMI_EVENT_ECC_SDMA_UE:
		return "ECC_SDMA_UE"
	case AMDSMI_EVENT_ECC_GFX_CE_TOTAL:
		return "ECC_GFX_CE_TOTAL"
	case AMDSMI_EVENT_ECC_GFX_UE_TOTAL:
		return "ECC_GFX_UE_TOTAL"
	case AMDSMI_EVENT_ECC_SDMA_CE_TOTAL:
		return "ECC_SDMA_CE_TOTAL"
	case AMDSMI_EVENT_ECC_SDMA_UE_TOTAL:
		return "ECC_SDMA_UE_TOTAL"
	case AMDSMI_EVENT_ECC_UMC_CE_TOTAL:
		return "ECC_UMC_CE_TOTAL"
	case AMDSMI_EVENT_ECC_UMC_UE_TOTAL:
		return "ECC_UMC_UE_TOTAL"
	case AMDSMI_EVENT_ECC_MMHUB_CE:
		return "ECC_MMHUB_CE"
	case AMDSMI_EVENT_ECC_MMHUB_UE:
		return "ECC_MMHUB_UE"
	case AMDSMI_EVENT_ECC_MMHUB_CE_TOTAL:
		return "ECC_MMHUB_CE_TOTAL"
	case AMDSMI_EVENT_ECC_MMHUB_UE_TOTAL:
		return "ECC_MMHUB_UE_TOTAL"
	case AMDSMI_EVENT_ECC_XGMI_WAFL_CE:
		return "ECC_XGMI_WAFL_CE"
	case AMDSMI_EVENT_ECC_XGMI_WAFL_UE:
		return "ECC_XGMI_WAFL_UE"
	case AMDSMI_EVENT_ECC_XGMI_WAFL_CE_TOTAL:
		return "ECC_XGMI_WAFL_CE_TOTAL"
	case AMDSMI_EVENT_ECC_XGMI_WAFL_UE_TOTAL:
		return "ECC_XGMI_WAFL_UE_TOTAL"
	case AMDSMI_EVENT_ECC_FATAL_ERROR:
		return "ECC_FATAL_ERROR"
	case AMDSMI_EVENT_ECC_POISON_CONSUMPTION:
		return "ECC_POISON_CONSUMPTION"
	case AMDSMI_EVENT_ECC_ACA_DUMP:
		return "ECC_ACA_DUMP"
	case AMDSMI_EVENT_ECC_WRONG_SOCKET_ID:
		return "ECC_WRONG_SOCKET_ID"
	case AMDSMI_EVENT_ECC_ACA_UNKNOWN_BLOCK_INSTANCE:
		return "ECC_ACA_UNKNOWN_BLOCK_INSTANCE"
	case AMDSMI_EVENT_ECC_UNKNOWN_CHIPLET_CE:
		return "ECC_UNKNOWN_CHIPLET_CE"
	case AMDSMI_EVENT_ECC_UNKNOWN_CHIPLET_UE:
		return "ECC_UNKNOWN_CHIPLET_UE"
	case AMDSMI_EVENT_ECC_UMC_CHIPLET_CE:
		return "ECC_UMC_CHIPLET_CE"
	case AMDSMI_EVENT_ECC_UMC_CHIPLET_UE:
		return "ECC_UMC_CHIPLET_UE"
	case AMDSMI_EVENT_ECC_GFX_CHIPLET_CE:
		return "ECC_GFX_CHIPLET_CE"
	case AMDSMI_EVENT_ECC_GFX_CHIPLET_UE:
		return "ECC_GFX_CHIPLET_UE"
	case AMDSMI_EVENT_ECC_SDMA_CHIPLET_CE:
		return "ECC_SDMA_CHIPLET_CE"
	case AMDSMI_EVENT_ECC_SDMA_CHIPLET_UE:
		return "ECC_SDMA_CHIPLET_UE"
	case AMDSMI_EVENT_ECC_MMHUB_CHIPLET_CE:
		return "ECC_MMHUB_CHIPLET_CE"
	case AMDSMI_EVENT_ECC_MMHUB_CHIPLET_UE:
		return "ECC_MMHUB_CHIPLET_UE"
	case AMDSMI_EVENT_ECC_XGMI_WAFL_CHIPLET_CE:
		return "ECC_XGMI_WAFL_CHIPLET_CE"
	case AMDSMI_EVENT_ECC_XGMI_WAFL_CHIPLET_UE:
		return "ECC_XGMI_WAFL_CHIPLET_UE"
	case AMDSMI_EVENT_ECC_EEPROM_ENTRIES_FOUND:
		return "ECC_EEPROM_ENTRIES_FOUND"
	case AMDSMI_EVENT_ECC_UMC_DE:
		return "ECC_UMC_DE"
	case AMDSMI_EVENT_ECC_UMC_DE_TOTAL:
		return "ECC_UMC_DE_TOTAL"
	case AMDSMI_EVENT_ECC_UNKNOWN:
		return "ECC_UNKNOWN"
	case AMDSMI_EVENT_ECC_EEPROM_REACH_THD:
		return "ECC_EEPROM_REACH_THD"
	case AMDSMI_EVENT_ECC_UMC_CHIPLET_DE:
		return "ECC_UMC_CHIPLET_DE"
	case AMDSMI_EVENT_ECC_UNKNOWN_CHIPLET_DE:
		return "ECC_UNKNOWN_CHIPLET_DE"
	case AMDSMI_EVENT_ECC_EEPROM_CHK_MISMATCH:
		return "ECC_EEPROM_CHK_MISMATCH"
	case AMDSMI_EVENT_ECC_EEPROM_RESET:
		return "ECC_EEPROM_RESET"
	case AMDSMI_EVENT_ECC_EEPROM_RESET_FAILED:
		return "ECC_EEPROM_RESET_FAILED"
	case AMDSMI_EVENT_ECC_EEPROM_APPEND:
		return "ECC_EEPROM_APPEND"
	case AMDSMI_EVENT_ECC_THD_CHANGED:
		return "ECC_THD_CHANGED"
	case AMDSMI_EVENT_ECC_DUP_ENTRIES:
		return "ECC_DUP_ENTRIES"
	case AMDSMI_EVENT_ECC_EEPROM_WRONG_HDR:
		return "ECC_EEPROM_WRONG_HDR"
	case AMDSMI_EVENT_ECC_EEPROM_WRONG_VER:
		return "ECC_EEPROM_WRONG_VER"
	}
	return fmt.Sprintf("ECC_UNKNOWN(%d)", uint32(s))
}

func (s EventPpSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_PP_SET_DPM_POLICY_FAIL:
		return "PP_SET_DPM_POLICY_FAIL"
	case AMDSMI_EVENT_PP_ACTIVATE_DPM_POLICY_FAIL:
		return "PP_ACTIVATE_DPM_POLICY_FAIL"
	case AMDSMI_EVENT_PP_I2C_SLAVE_NOT_PRESENT:
		return "PP_I2C_SLAVE_NOT_PRESENT"
	case AMDSMI_EVENT_PP_THROTTLER_EVENT:
		return "PP_THROTTLER_EVENT"
	}
	return fmt.Sprintf("PP_UNKNOWN(%d)", uint32(s))
}

func (s EventSchedSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_SCHED_WORLD_SWITCH_FAIL:
		return "SCHED_WORLD_SWITCH_FAIL"
	case AMDSMI_EVENT_SCHED_DISABLE_AUTO_HW_SWITCH_FAIL:
		return "SCHED_DISABLE_AUTO_HW_SWITCH_FAIL"
	case AMDSMI_EVENT_SCHED_ENABLE_AUTO_HW_SWITCH_FAIL:
		return "SCHED_ENABLE_AUTO_HW_SWITCH_FAIL"
	case AMDSMI_EVENT_SCHED_GFX_SAVE_REG_FAIL:
		return "SCHED_GFX_SAVE_REG_FAIL"
	case AMDSMI_EVENT_SCHED_GFX_IDLE_REG_FAIL:
		return "SCHED_GFX_IDLE_REG_FAIL"
	case AMDSMI_EVENT_SCHED_GFX_RUN_REG_FAIL:
		return "SCHED_GFX_RUN_REG_FAIL"
	case AMDSMI_EVENT_SCHED_GFX_LOAD_REG_FAIL:
		return "SCHED_GFX_LOAD_REG_FAIL"
	case AMDSMI_EVENT_SCHED_GFX_INIT_REG_FAIL:
		return "SCHED_GFX_INIT_REG_FAIL"
	case AMDSMI_EVENT_SCHED_MM_SAVE_REG_FAIL:
		return "SCHED_MM_SAVE_REG_FAIL"
	case AMDSMI_EVENT_SCHED_MM_IDLE_REG_FAIL:
		return "SCHED_MM_IDLE_REG_FAIL"
	case AMDSMI_EVENT_SCHED_MM_RUN_REG_FAIL:
		return "SCHED_MM_RUN_REG_FAIL"
	case AMDSMI_EVENT_SCHED_MM_LOAD_REG_FAIL:
		return "SCHED_MM_LOAD_REG_FAIL"
	case AMDSMI_EVENT_SCHED_MM_INIT_REG_FAIL:
		return "SCHED_MM_INIT_REG_FAIL"
	case AMDSMI_EVENT_SCHED_INIT_GPU_FAIL:
		return "SCHED_INIT_GPU_FAIL"
	case AMDSMI_EVENT_SCHED_RUN_GPU_FAIL:
		return "SCHED_RUN_GPU_FAIL"
	case AMDSMI_EVENT_SCHED_SAVE_GPU_STATE_FAIL:
		return "SCHED_SAVE_GPU_STATE_FAIL"
	case AMDSMI_EVENT_SCHED_LOAD_GPU_STATE_FAIL:
		return "SCHED_LOAD_GPU_STATE_FAIL"
	case AMDSMI_EVENT_SCHED_IDLE_GPU_FAIL:
		return "SCHED_IDLE_GPU_FAIL"
	case AMDSMI_EVENT_SCHED_FINI_GPU_FAIL:
		return "SCHED_FINI_GPU_FAIL"
	case AMDSMI_EVENT_SCHED_DEAD_VF:
		return "SCHED_DEAD_VF"
	case AMDSMI_EVENT_SCHED_EVENT_QUEUE_FULL:
		return "SCHED_EVENT_QUEUE_FULL"
	case AMDSMI_EVENT_SCHED_SHUTDOWN_VF_FAIL:
		return "SCHED_SHUTDOWN_VF_FAIL"
	case AMDSMI_EVENT_SCHED_RESET_VF_NUM_FAIL:
		return "SCHED_RESET_VF_NUM_FAIL"
	case AMDSMI_EVENT_SCHED_IGNORE_EVENT:
		return "SCHED_IGNORE_EVENT"
	case AMDSMI_EVENT_SCHED_PF_SWITCH_FAIL:
		return "SCHED_PF_SWITCH_FAIL"
	}
	return fmt.Sprintf("SCHED_UNKNOWN(%d)", uint32(s))
}

func (s EventVfSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_VF_ATOMBIOS_INIT_FAIL:
		return "VF_ATOMBIOS_INIT_FAIL"
	case AMDSMI_EVENT_VF_NO_VBIOS:
		return "VF_NO_VBIOS"
	case AMDSMI_EVENT_VF_GPU_POST_ERROR:
		return "VF_GPU_POST_ERROR"
	case AMDSMI_EVENT_VF_ATOMBIOS_GET_CLOCK_FAIL:
		return "VF_ATOMBIOS_GET_CLOCK_FAIL"
	case AMDSMI_EVENT_VF_FENCE_INIT_FAIL:
		return "VF_FENCE_INIT_FAIL"
	case AMDSMI_EVENT_VF_AMDGPU_INIT_FAIL:
		return "VF_AMDGPU_INIT_FAIL"
	case AMDSMI_EVENT_VF_IB_INIT_FAIL:
		return "VF_IB_INIT_FAIL"
	case AMDSMI_EVENT_VF_AMDGPU_LATE_INIT_FAIL:
		return "VF_AMDGPU_LATE_INIT_FAIL"
	case AMDSMI_EVENT_VF_ASIC_RESUME_FAIL:
		return "VF_ASIC_RESUME_FAIL"
	case AMDSMI_EVENT_VF_GPU_RESET_FAIL:
		return "VF_GPU_RESET_FAIL"
	}
	return fmt.Sprintf("VF_UNKNOWN(%d)", uint32(s))
}

func (s EventVbiosSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_VBIOS_INVALID:
		return "VBIOS_INVALID"
	case AMDSMI_EVENT_VBIOS_IMAGE_MISSING:
		return "VBIOS_IMAGE_MISSING"
	case AMDSMI_EVENT_VBIOS_CHECKSUM_ERR:
		return "VBIOS_CHECKSUM_ERR"
	case AMDSMI_EVENT_VBIOS_POST_FAIL:
		return "VBIOS_POST_FAIL"
	case AMDSMI_EVENT_VBIOS_READ_FAIL:
		return "VBIOS_READ_FAIL"
	case AMDSMI_EVENT_VBIOS_READ_IMG_HEADER_FAIL:
		return "VBIOS_READ_IMG_HEADER_FAIL"
	case AMDSMI_EVENT_VBIOS_READ_IMG_SIZE_FAIL:
		return "VBIOS_READ_IMG_SIZE_FAIL"
	case AMDSMI_EVENT_VBIOS_GET_FW_INFO_FAIL:
		return "VBIOS_GET_FW_INFO_FAIL"
	case AMDSMI_EVENT_VBIOS_GET_TBL_REVISION_FAIL:
		return "VBIOS_GET_TBL_REVISION_FAIL"
	case AMDSMI_EVENT_VBIOS_PARSER_TBL_FAIL:
		return "VBIOS_PARSER_TBL_FAIL"
	case AMDSMI_EVENT_VBIOS_IP_DISCOVERY_FAIL:
		return "VBIOS_IP_DISCOVERY_FAIL"
	case AMDSMI_EVENT_VBIOS_TIMEOUT:
		return "VBIOS_TIMEOUT"
	case AMDSMI_EVENT_VBIOS_HASH_INVALID:
		return "VBIOS_HASH_INVALID"
	case AMDSMI_EVENT_VBIOS_HASH_UPDATED:
		return "VBIOS_HASH_UPDATED"
	case AMDSMI_EVENT_VBIOS_IP_DISCOVERY_BINARY_CHECKSUM_FAIL:
		return "VBIOS_IP_DISCOVERY_BINARY_CHECKSUM_FAIL"
	case AMDSMI_EVENT_VBIOS_IP_DISCOVERY_TABLE_CHECKSUM_FAIL:
		return "VBIOS_IP_DISCOVERY_TABLE_CHECKSUM_FAIL"
	}
	return fmt.Sprintf("VBIOS_UNKNOWN(%d)", uint32(s))
}

func (s EventGuardSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_GUARD_RESET_FAIL:
		return "GUARD_RESET_FAIL"
	case AMDSMI_EVENT_GUARD_EVENT_OVERFLOW:
		return "GUARD_EVENT_OVERFLOW"
	}
	return fmt.Sprintf("GUARD_UNKNOWN(%d)", uint32(s))
}

func (s EventGpumonSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_GPUMON_INVALID_OPTION:
		return "GPUMON_INVALID_OPTION"
	case AMDSMI_EVENT_GPUMON_INVALID_VF_INDEX:
		return "GPUMON_INVALID_VF_INDEX"
	case AMDSMI_EVENT_GPUMON_INVALID_FB_SIZE:
		return "GPUMON_INVALID_FB_SIZE"
	case AMDSMI_EVENT_GPUMON_NO_SUITABLE_SPACE:
		return "GPUMON_NO_SUITABLE_SPACE"
	case AMDSMI_EVENT_GPUMON_NO_AVAILABLE_SLOT:
		return "GPUMON_NO_AVAILABLE_SLOT"
	case AMDSMI_EVENT_GPUMON_OVERSIZE_ALLOCATION:
		return "GPUMON_OVERSIZE_ALLOCATION"
	case AMDSMI_EVENT_GPUMON_OVERLAPPING_FB:
		return "GPUMON_OVERLAPPING_FB"
	case AMDSMI_EVENT_GPUMON_INVALID_GFX_TIMESLICE:
		return "GPUMON_INVALID_GFX_TIMESLICE"
	case AMDSMI_EVENT_GPUMON_INVALID_MM_TIMESLICE:
		return "GPUMON_INVALID_MM_TIMESLICE"
	case AMDSMI_EVENT_GPUMON_INVALID_GFX_PART:
		return "GPUMON_INVALID_GFX_PART"
	case AMDSMI_EVENT_GPUMON_VF_BUSY:
		return "GPUMON_VF_BUSY"
	case AMDSMI_EVENT_GPUMON_INVALID_VF_NUM:
		return "GPUMON_INVALID_VF_NUM"
	case AMDSMI_EVENT_GPUMON_NOT_SUPPORTED:
		return "GPUMON_NOT_SUPPORTED"
	}
	return fmt.Sprintf("GPUMON_UNKNOWN(%d)", uint32(s))
}

func (s EventMmschSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_MMSCH_IGNORED_JOB:
		return "MMSCH_IGNORED_JOB"
	case AMDSMI_EVENT_MMSCH_UNSUPPORTED_VCN_FW:
		return "MMSCH_UNSUPPORTED_VCN_FW"
	}
	return fmt.Sprintf("MMSCH_UNKNOWN(%d)", uint32(s))
}

func (s EventXgmiSubcode) String() string {
	switch s {
	case AMDSMI_EVENT_XGMI_TOPOLOGY_UPDATE_FAILED:
		return "XGMI_TOPOLOGY_UPDATE_FAILED"
	case AMDSMI_EVENT_XGMI_TOPOLOGY_HW_INIT_UPDATE:
		return "XGMI_TOPOLOGY_HW_INIT_UPDATE"
	case AMDSMI_EVENT_XGMI_TOPOLOGY_UPDATE_DONE:
		return "XGMI_TOPOLOGY_UPDATE_DONE"
	case AMDSMI_EVENT_XGMI_FB_SHARING_SETTING_ERROR:
		return "XGMI_FB_SHARING_SETTING_ERROR"
	case AMDSMI_EVENT_XGMI_FB_SHARING_SETTING_RESET:
		return "XGMI_FB_SHARING_SETTING_RESET"
	}
	return fmt.Sprintf("XGMI_UNKNOWN(%d)", uint32(s))
}

func (t PpThrottlerType) String() string {
	switch t {
	case AMDSMI_EVENT_THROTTLER_PROCHOT:
		return "THROTTLER_PROCHOT"
	case AMDSMI_EVENT_THROTTLER_SOCKET:
		return "THROTTLER_SOCKET"
	case AMDSMI_EVENT_THROTTLER_VR:
		return "THROTTLER_VR"
	case AMDSMI_EVENT_THROTTLER_HBM:
		return "THROTTLER_HBM"
	}
	return fmt.Sprintf("THROTTLER_UNKNOWN(%d)", uint32(t))
}

// SubcodeName returns the human-readable name for a (category, subcode) pair as
// reported in EventEntry.Category / EventEntry.Subcode. Returns the raw decimal
// value formatted as "UNKNOWN(N)" if no mapping exists for the category.
func SubcodeName(category EventCategory, subcode uint32) string {
	switch category {
	case AMDSMI_EVENT_CATEGORY_DRIVER:
		return EventDriverSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_RESET:
		return EventResetSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_SCHED:
		return EventSchedSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_VBIOS:
		return EventVbiosSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_ECC:
		return EventEccSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_PP:
		return EventPpSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_IOV:
		return EventIovSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_VF:
		return EventVfSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_FW:
		return EventFwSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_GPU:
		return EventGpuSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_GUARD:
		return EventGuardSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_GPUMON:
		return EventGpumonSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_MMSCH:
		return EventMmschSubcode(subcode).String()
	case AMDSMI_EVENT_CATEGORY_XGMI:
		return EventXgmiSubcode(subcode).String()
	}
	return fmt.Sprintf("UNKNOWN(%d)", subcode)
}

// GuestData holds guest-reported driver version and framebuffer usage for a VF (amdsmi_guest_data_t)
type GuestData struct {
	DriverVersion string
	FbUsage       uint32
}

// GetGuestData returns guest OS information for the VF (amdsmi_get_guest_data)
func GetGuestData(vh VfHandle) (GuestData, error) {
	var cGuestData C.amdsmi_guest_data_t

	ret := C.amdsmi_get_guest_data(vh.cVfHandle(), &cGuestData)
	if err := checkStatus(Status(ret)); err != nil {
		return GuestData{}, err
	}

	return GuestData{
		DriverVersion: C.GoString(&cGuestData.driver_version[0]),
		FbUsage:       uint32(cGuestData.fb_usage),
	}, nil
}

// GetVfFwInfo returns firmware versions reported for the VF (amdsmi_get_vf_fw_info)
func GetVfFwInfo(vh VfHandle) (FwInfo, error) {
	var cVfInfo C.amdsmi_fw_info_t

	ret := C.amdsmi_get_vf_fw_info(vh.cVfHandle(), &cVfInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return FwInfo{}, err
	}

	VfInfo := FwInfo{
		NumFwInfo: uint8(cVfInfo.num_fw_info),
	}

	for i := C.uint32_t(0); i < C.uint32_t(cVfInfo.num_fw_info); i++ {
		VfInfo.FwList[i].FwID = FwBlock(cVfInfo.fw_info_list[i].fw_id)
		VfInfo.FwList[i].FwVersion = uint64(cVfInfo.fw_info_list[i].fw_version)
	}

	return VfInfo, nil
}

func ClearVfFb(vh VfHandle) error {
	ret := C.amdsmi_clear_vf_fb(vh.cVfHandle())
	return checkStatus(Status(ret))
}

func SetNumVf(ph ProcessorHandle, numVf uint32) error {
	ret := C.amdsmi_set_num_vf(ph.cPtr(), C.uint32_t(numVf))
	return checkStatus(Status(ret))
}

// NicDriverInfo holds NIC driver information.
type NicDriverInfo struct {
	Name    string
	Version string
}

// GetNicDriverInfo returns driver information for the given NIC.
func GetNicDriverInfo(ph ProcessorHandle) (NicDriverInfo, error) {
	var info C.amdsmi_nic_driver_info_t
	ret := C.amdsmi_get_nic_driver_info(ph.cPtr(), &info)
	if err := checkStatus(Status(ret)); err != nil {
		return NicDriverInfo{}, err
	}
	return NicDriverInfo{
		Name:    C.GoString(&info.name[0]),
		Version: C.GoString(&info.version[0]),
	}, nil
}

// NicAsicInfo holds NIC ASIC identification information.
type NicAsicInfo struct {
	VendorID         uint16
	SubvendorID      uint16
	DeviceID         uint16
	SubsystemID      uint16
	Revision         uint8
	PermanentAddress string
	ProductName      string
	PartNumber       string
	SerialNumber     string
	VendorName       string
}

// GetNicAsicInfo returns ASIC information for the given NIC.
func GetNicAsicInfo(ph ProcessorHandle) (NicAsicInfo, error) {
	var cInfo C.amdsmi_nic_asic_info_t
	ret := C.amdsmi_get_nic_asic_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return NicAsicInfo{}, err
	}
	return NicAsicInfo{
		VendorID:         uint16(cInfo.vendor_id),
		SubvendorID:      uint16(cInfo.subvendor_id),
		DeviceID:         uint16(cInfo.device_id),
		SubsystemID:      uint16(cInfo.subsystem_id),
		Revision:         uint8(cInfo.revision),
		PermanentAddress: C.GoString(&cInfo.permanent_address[0]),
		ProductName:      C.GoString(&cInfo.product_name[0]),
		PartNumber:       C.GoString(&cInfo.part_number[0]),
		SerialNumber:     C.GoString(&cInfo.serial_number[0]),
		VendorName:       C.GoString(&cInfo.vendor_name[0]),
	}, nil
}

// NicBusInfo holds NIC bus information.
type NicBusInfo struct {
	Bdf                  Bdf
	MaxPcieWidth         uint8
	MaxPcieSpeed         uint32
	PcieInterfaceVersion string
	SlotType             string
}

// GetNicBusInfo returns bus information for the given NIC.
func GetNicBusInfo(ph ProcessorHandle) (NicBusInfo, error) {
	var cInfo C.amdsmi_nic_bus_info_t
	ret := C.amdsmi_get_nic_bus_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return NicBusInfo{}, err
	}
	bdfVal := *(*Bdf)(unsafe.Pointer(&cInfo.bdf))
	return NicBusInfo{
		Bdf:                  bdfVal,
		MaxPcieWidth:         uint8(cInfo.max_pcie_width),
		MaxPcieSpeed:         uint32(cInfo.max_pcie_speed),
		PcieInterfaceVersion: C.GoString(&cInfo.pcie_interface_version[0]),
		SlotType:             C.GoString(&cInfo.slot_type[0]),
	}, nil
}

// NicNumaInfo holds NIC NUMA information.
type NicNumaInfo struct {
	Node     uint8
	Affinity string
}

// GetNicNumaInfo returns NUMA information for the given NIC.
func GetNicNumaInfo(ph ProcessorHandle) (NicNumaInfo, error) {
	var cInfo C.amdsmi_nic_numa_info_t
	ret := C.amdsmi_get_nic_numa_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return NicNumaInfo{}, err
	}
	return NicNumaInfo{
		Node:     uint8(cInfo.node),
		Affinity: C.GoString(&cInfo.affinity[0]),
	}, nil
}

// NicFw holds a single NIC firmware name/version pair (amdsmi_nic_fw_t).
type NicFw struct {
	Name    string
	Version string
}

// NicFwEntry pairs a firmware version slot with its name/version
// (amdsmi_nic_fw_entry_t).
type NicFwEntry struct {
	Type NicFwVersionType
	Fw   NicFw
}

// NicFwInfo holds the collection of firmware entries reported for a NIC
// (amdsmi_nic_fw_info_t).
type NicFwInfo struct {
	NumFw uint32
	Fw    []NicFwEntry
}

// GetNicFwInfo returns firmware information for the given NIC
// (amdsmi_get_nic_fw_info).
func GetNicFwInfo(ph ProcessorHandle) (NicFwInfo, error) {
	var cInfo C.amdsmi_nic_fw_info_t
	ret := C.amdsmi_get_nic_fw_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return NicFwInfo{}, err
	}

	n := uint32(cInfo.num_fw)
	if n > AMDSMI_MAX_NIC_FW {
		n = AMDSMI_MAX_NIC_FW
	}

	entries := make([]NicFwEntry, n)
	for i := uint32(0); i < n; i++ {
		entries[i] = NicFwEntry{
			Type: NicFwVersionType(cInfo.fw[i]._type),
			Fw: NicFw{
				Name:    C.GoString(&cInfo.fw[i].fw.name[0]),
				Version: C.GoString(&cInfo.fw[i].fw.version[0]),
			},
		}
	}

	return NicFwInfo{
		NumFw: n,
		Fw:    entries,
	}, nil
}

// NicPort holds information about a single NIC port.
type NicPort struct {
	Bdf          Bdf
	PortNum      uint32
	Type         string
	Flavour      string
	Netdev       string
	Ifindex      uint8
	MacAddress   string
	Carrier      uint8
	Mtu          uint16
	LinkState    string
	LinkSpeed    uint32
	ActiveFec    uint32
	Autoneg      string
	PauseAutoneg string
	PauseRx      string
	PauseTx      string
}

// NicPortInfo holds the collection of NIC ports.
type NicPortInfo struct {
	NumPorts uint32
	Ports    []NicPort
}

// GetNicPortInfo returns port information for the given NIC.
func GetNicPortInfo(ph ProcessorHandle) (NicPortInfo, error) {
	var cInfo C.amdsmi_nic_port_info_t
	ret := C.amdsmi_get_nic_port_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return NicPortInfo{}, err
	}

	n := uint32(cInfo.num_ports)
	if n > AMDSMI_MAX_NIC_PORTS {
		n = AMDSMI_MAX_NIC_PORTS
	}

	ports := make([]NicPort, n)
	for i := uint32(0); i < n; i++ {
		ports[i] = NicPort{
			Bdf:          *(*Bdf)(unsafe.Pointer(&cInfo.ports[i].bdf)),
			PortNum:      uint32(cInfo.ports[i].port_num),
			Type:         C.GoString(&cInfo.ports[i]._type[0]),
			Flavour:      C.GoString(&cInfo.ports[i].flavour[0]),
			Netdev:       C.GoString(&cInfo.ports[i].netdev[0]),
			Ifindex:      uint8(cInfo.ports[i].ifindex),
			MacAddress:   C.GoString(&cInfo.ports[i].mac_address[0]),
			Carrier:      uint8(cInfo.ports[i].carrier),
			Mtu:          uint16(cInfo.ports[i].mtu),
			LinkState:    C.GoString(&cInfo.ports[i].link_state[0]),
			LinkSpeed:    uint32(cInfo.ports[i].link_speed),
			ActiveFec:    uint32(cInfo.ports[i].active_fec),
			Autoneg:      C.GoString(&cInfo.ports[i].autoneg[0]),
			PauseAutoneg: C.GoString(&cInfo.ports[i].pause_autoneg[0]),
			PauseRx:      C.GoString(&cInfo.ports[i].pause_rx[0]),
			PauseTx:      C.GoString(&cInfo.ports[i].pause_tx[0]),
		}
	}

	return NicPortInfo{
		NumPorts: n,
		Ports:    ports,
	}, nil
}

// NicRdmaPortInfo holds information about a single RDMA port.
type NicRdmaPortInfo struct {
	Netdev    string
	State     string
	RdmaPort  uint8
	MaxMtu    uint16
	ActiveMtu uint16
}

// NicRdmaDevInfo holds information about a single RDMA device.
type NicRdmaDevInfo struct {
	RdmaDev      string
	NodeGuid     string
	NodeType     string
	SysImageGuid string
	FwVer        string
	NumRdmaPorts uint8
	RdmaPortInfo []NicRdmaPortInfo
}

// NicRdmaDevicesInfo holds the collection of RDMA devices for a NIC.
type NicRdmaDevicesInfo struct {
	NumRdmaDev  uint8
	RdmaDevInfo []NicRdmaDevInfo
}

// GetNicRdmaDevInfo returns the RDMA devices information for the given NIC.
func GetNicRdmaDevInfo(ph ProcessorHandle) (NicRdmaDevicesInfo, error) {
	var cInfo C.amdsmi_nic_rdma_devices_info_t
	ret := C.amdsmi_get_nic_rdma_dev_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return NicRdmaDevicesInfo{}, err
	}

	nDev := uint8(cInfo.num_rdma_dev)
	if nDev > AMDSMI_MAX_NIC_RDMA_DEV {
		nDev = AMDSMI_MAX_NIC_RDMA_DEV
	}

	devs := make([]NicRdmaDevInfo, nDev)
	for i := uint8(0); i < nDev; i++ {
		nPorts := uint8(cInfo.rdma_dev_info[i].num_rdma_ports)
		if nPorts > AMDSMI_MAX_NIC_PORTS {
			nPorts = AMDSMI_MAX_NIC_PORTS
		}

		ports := make([]NicRdmaPortInfo, nPorts)
		for j := uint8(0); j < nPorts; j++ {
			ports[j] = NicRdmaPortInfo{
				Netdev:    C.GoString(&cInfo.rdma_dev_info[i].rdma_port_info[j].netdev[0]),
				State:     C.GoString(&cInfo.rdma_dev_info[i].rdma_port_info[j].state[0]),
				RdmaPort:  uint8(cInfo.rdma_dev_info[i].rdma_port_info[j].rdma_port),
				MaxMtu:    uint16(cInfo.rdma_dev_info[i].rdma_port_info[j].max_mtu),
				ActiveMtu: uint16(cInfo.rdma_dev_info[i].rdma_port_info[j].active_mtu),
			}
		}

		devs[i] = NicRdmaDevInfo{
			RdmaDev:      C.GoString(&cInfo.rdma_dev_info[i].rdma_dev[0]),
			NodeGuid:     C.GoString(&cInfo.rdma_dev_info[i].node_guid[0]),
			NodeType:     C.GoString(&cInfo.rdma_dev_info[i].node_type[0]),
			SysImageGuid: C.GoString(&cInfo.rdma_dev_info[i].sys_image_guid[0]),
			FwVer:        C.GoString(&cInfo.rdma_dev_info[i].fw_ver[0]),
			NumRdmaPorts: nPorts,
			RdmaPortInfo: ports,
		}
	}

	return NicRdmaDevicesInfo{
		NumRdmaDev:  nDev,
		RdmaDevInfo: devs,
	}, nil
}

// NicStat holds a single NIC statistic name/value pair.
type NicStat struct {
	Name  string
	Value uint64
}

// GetNicPortStatistics returns the port statistics for the given NIC port index.
// It first queries the count, then retrieves the statistics.
func GetNicPortStatistics(ph ProcessorHandle, portIndex uint32) ([]NicStat, error) {
	var cCount C.uint32_t

	ret := C.amdsmi_get_nic_port_statistics(ph.cPtr(), C.uint32_t(portIndex), &cCount, nil)
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}
	if cCount == 0 {
		return nil, nil
	}

	stats := make([]C.amdsmi_nic_stat_t, cCount)
	ret = C.amdsmi_get_nic_port_statistics(ph.cPtr(), C.uint32_t(portIndex), &cCount, &stats[0])
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}

	result := make([]NicStat, cCount)
	for i := C.uint32_t(0); i < cCount; i++ {
		result[i] = NicStat{
			Name:  C.GoString(&stats[i].name[0]),
			Value: uint64(stats[i].value),
		}
	}
	return result, nil
}

// GetNicVendorStatistics returns the vendor-specific statistics for the given NIC port index.
// It first queries the count, then retrieves the statistics.
func GetNicVendorStatistics(ph ProcessorHandle, portIndex uint32) ([]NicStat, error) {
	var cCount C.uint32_t

	ret := C.amdsmi_get_nic_vendor_statistics(ph.cPtr(), C.uint32_t(portIndex), &cCount, nil)
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}
	if cCount == 0 {
		return nil, nil
	}

	stats := make([]C.amdsmi_nic_stat_t, cCount)
	ret = C.amdsmi_get_nic_vendor_statistics(ph.cPtr(), C.uint32_t(portIndex), &cCount, &stats[0])
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}

	result := make([]NicStat, cCount)
	for i := C.uint32_t(0); i < cCount; i++ {
		result[i] = NicStat{
			Name:  C.GoString(&stats[i].name[0]),
			Value: uint64(stats[i].value),
		}
	}
	return result, nil
}

// GetNicRdmaPortStatistics returns the RDMA port statistics for the given NIC RDMA port index.
// It first queries the count, then retrieves the statistics.
func GetNicRdmaPortStatistics(ph ProcessorHandle, rdmaPortIndex uint32) ([]NicStat, error) {
	var cCount C.uint32_t

	ret := C.amdsmi_get_nic_rdma_port_statistics(ph.cPtr(), C.uint32_t(rdmaPortIndex), &cCount, nil)
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}
	if cCount == 0 {
		return nil, nil
	}

	stats := make([]C.amdsmi_nic_stat_t, cCount)
	ret = C.amdsmi_get_nic_rdma_port_statistics(ph.cPtr(), C.uint32_t(rdmaPortIndex), &cCount, &stats[0])
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}

	result := make([]NicStat, cCount)
	for i := C.uint32_t(0); i < cCount; i++ {
		result[i] = NicStat{
			Name:  C.GoString(&stats[i].name[0]),
			Value: uint64(stats[i].value),
		}
	}
	return result, nil
}

// TdiState reports the lifecycle state of the TEE Device Interface
// (amdsmi_tdi_state_t).
type TdiState uint32

const (
	AMDSMI_TDI_STATE_UNLOCKED TdiState = 0
	AMDSMI_TDI_STATE_LOCKED   TdiState = 1
	AMDSMI_TDI_STATE_RUN      TdiState = 2
	AMDSMI_TDI_STATE_ERROR    TdiState = 3
)

func (s TdiState) String() string {
	switch s {
	case AMDSMI_TDI_STATE_UNLOCKED:
		return "UNLOCKED"
	case AMDSMI_TDI_STATE_LOCKED:
		return "LOCKED"
	case AMDSMI_TDI_STATE_RUN:
		return "RUN"
	case AMDSMI_TDI_STATE_ERROR:
		return "ERROR"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", uint32(s))
	}
}

// GetTdiState returns the current TDI state for the given VF
// (amdsmi_get_tdi_state).
func GetTdiState(vh VfHandle) (TdiState, error) {
	var cState C.amdsmi_tdi_state_t
	ret := C.amdsmi_get_tdi_state(vh.cVfHandle(), &cState)
	if err := checkStatus(Status(ret)); err != nil {
		return AMDSMI_TDI_STATE_ERROR, err
	}
	return TdiState(cState), nil
}

// CcMode reports whether Confidential Compute is enabled on a GPU
// (amdsmi_cc_mode_t).
type CcMode uint32

const (
	AMDSMI_CC_MODE_OFF CcMode = 0
	AMDSMI_CC_MODE_ON  CcMode = 1
	AMDSMI_CC_MODE_DEV CcMode = 2
)

func (m CcMode) String() string {
	switch m {
	case AMDSMI_CC_MODE_OFF:
		return "OFF"
	case AMDSMI_CC_MODE_ON:
		return "ON"
	case AMDSMI_CC_MODE_DEV:
		return "DEV"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", uint32(m))
	}
}

// GetCcMode returns the Confidential Compute mode for the given processor
// (amdsmi_get_cc_mode).
func GetCcMode(ph ProcessorHandle) (CcMode, error) {
	var cMode C.amdsmi_cc_mode_t
	ret := C.amdsmi_get_cc_mode(ph.cPtr(), &cMode)
	if err := checkStatus(Status(ret)); err != nil {
		return AMDSMI_CC_MODE_OFF, err
	}
	return CcMode(cMode), nil
}

// SetCcMode sets the Confidential Compute mode on the given processor
// (amdsmi_set_cc_mode).
func SetCcMode(ph ProcessorHandle, mode CcMode) error {
	ret := C.amdsmi_set_cc_mode(ph.cPtr(), C.amdsmi_cc_mode_t(mode))
	return checkStatus(Status(ret))
}

// VfHbmInfo holds per-VF HBM information (amdsmi_vf_hbm_info_t).
//
// NumaID is 0xFFFFFFFF when no NUMA association exists (e.g. DAX mode).
type VfHbmInfo struct {
	PhyAddr uint64
	PhySize uint64
	NumaID  uint32
	Name    string
}

// GetVfHbmInfo returns HBM information for the given VF
// (amdsmi_get_vf_hbm_info).
func GetVfHbmInfo(vh VfHandle) (VfHbmInfo, error) {
	var cInfo C.amdsmi_vf_hbm_info_t
	ret := C.amdsmi_get_vf_hbm_info(vh.cVfHandle(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return VfHbmInfo{}, err
	}
	return VfHbmInfo{
		PhyAddr: uint64(cInfo.phy_addr),
		PhySize: uint64(cInfo.phy_size),
		NumaID:  uint32(cInfo.numa_id),
		Name:    C.GoString(&cInfo.name[0]),
	}, nil
}

// FabricType identifies the fabric transport family (amdsmi_fabric_type_t).
type FabricType uint32

const (
	AMDSMI_FABRIC_TYPE_UALOE    FabricType = 0
	AMDSMI_FABRIC_TYPE_UALINK   FabricType = 1
	AMDSMI_FABRIC_TYPE_UNKNOWN  FabricType = 2
)

func (t FabricType) String() string {
	switch t {
	case AMDSMI_FABRIC_TYPE_UALOE:
		return "UALOE"
	case AMDSMI_FABRIC_TYPE_UALINK:
		return "UALINK"
	case AMDSMI_FABRIC_TYPE_UNKNOWN:
		return "UNKNOWN"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", uint32(t))
	}
}

// FabricNpaAddressMode describes how NPA addresses sources
// (amdsmi_fabric_npa_address_mode_t).
type FabricNpaAddressMode uint32

const (
	AMDSMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_ALIASING       FabricNpaAddressMode = 0
	AMDSMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_IDENTIFICATION FabricNpaAddressMode = 1
	AMDSMI_FABRIC_NPA_ADDRESS_MODE_UNKNOWN               FabricNpaAddressMode = 2
)

func (m FabricNpaAddressMode) String() string {
	switch m {
	case AMDSMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_ALIASING:
		return "SOURCE_ALIASING"
	case AMDSMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_IDENTIFICATION:
		return "SOURCE_IDENTIFICATION"
	case AMDSMI_FABRIC_NPA_ADDRESS_MODE_UNKNOWN:
		return "UNKNOWN"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", uint32(m))
	}
}

// FabricAcceleratorVpodState describes the vPoD lifecycle state of a fabric
// accelerator (amdsmi_fabric_accelerator_vpod_state_t).
type FabricAcceleratorVpodState uint32

const (
	AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_UNCONFIGURED FabricAcceleratorVpodState = 0
	AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_CONFIGURED   FabricAcceleratorVpodState = 1
	AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_READY        FabricAcceleratorVpodState = 2
	AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_ACTIVE       FabricAcceleratorVpodState = 3
	AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_ERROR        FabricAcceleratorVpodState = 4
	AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_UNKNOWN      FabricAcceleratorVpodState = 5
)

func (s FabricAcceleratorVpodState) String() string {
	switch s {
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_UNCONFIGURED:
		return "UNCONFIGURED"
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_CONFIGURED:
		return "CONFIGURED"
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_READY:
		return "READY"
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_ACTIVE:
		return "ACTIVE"
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_ERROR:
		return "ERROR"
	case AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_UNKNOWN:
		return "UNKNOWN"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", uint32(s))
	}
}

// FabricInfoV1 mirrors amdsmi_fabric_info_v1_t.
//
// PpodID is a 128-bit UUID stored as a 16-byte array (AMDSMI_FABRIC_PPOD_ID_SIZE).
// VpodActiveAccelerators is a 1024-bit bitmap stored as 32 uint32 words; bit
// N set means accelerator ID N is active in this vPoD.
type FabricInfoV1 struct {
	AcceleratorID          uint32
	FabricType             FabricType
	Bandwidth              uint32
	Latency                uint32
	PpodID                 [AMDSMI_FABRIC_PPOD_ID_SIZE]byte
	PpodSize               uint32
	VpodID                 uint32
	VpodSize               uint32
	VpodActiveAccelerators [AMDSMI_FABRIC_ACTIVE_ACCELERATORS_BITMAP_SIZE]uint32
	LocalAccelerators      [AMDSMI_FABRIC_MAX_LOCAL_GPUS]uint32
	AddrMode               FabricNpaAddressMode
	AccelState             FabricAcceleratorVpodState
}

// FabricInfo holds fabric configuration for a GPU (amdsmi_fabric_info_t).
//
// Version selects which variant in the union is populated. Only InfoV1 is
// currently defined.
type FabricInfo struct {
	Bdf     Bdf
	Version uint32
	InfoV1  FabricInfoV1
}

// GetGpuFabricInfo returns fabric configuration for the given GPU
// (amdsmi_get_gpu_fabric_info).
func GetGpuFabricInfo(ph ProcessorHandle) (FabricInfo, error) {
	var cInfo C.amdsmi_fabric_info_t
	ret := C.amdsmi_get_gpu_fabric_info(ph.cPtr(), &cInfo)
	if err := checkStatus(Status(ret)); err != nil {
		return FabricInfo{}, err
	}

	out := FabricInfo{
		Bdf:     *(*Bdf)(unsafe.Pointer(&cInfo.bdf)),
		Version: uint32(cInfo.info.version),
	}

	if out.Version == 1 {
		v1 := C.amdsmi_go_fabric_info_v1(&cInfo)
		out.InfoV1 = FabricInfoV1{
			AcceleratorID: uint32(v1.accelerator_id),
			FabricType:    FabricType(v1.fabric_type),
			Bandwidth:     uint32(v1.bandwidth),
			Latency:       uint32(v1.latency),
			PpodID:        *(*[AMDSMI_FABRIC_PPOD_ID_SIZE]byte)(unsafe.Pointer(&v1.ppod_id[0])),
			PpodSize:      uint32(v1.ppod_size),
			VpodID:        uint32(v1.vpod_id),
			VpodSize:      uint32(v1.vpod_size),
			AddrMode:      FabricNpaAddressMode(v1.addr_mode),
			AccelState:    FabricAcceleratorVpodState(v1.accel_state),
		}
		for i := 0; i < AMDSMI_FABRIC_ACTIVE_ACCELERATORS_BITMAP_SIZE; i++ {
			out.InfoV1.VpodActiveAccelerators[i] = uint32(v1.vpod_active_accelerators[i])
		}
		for i := 0; i < AMDSMI_FABRIC_MAX_LOCAL_GPUS; i++ {
			out.InfoV1.LocalAccelerators[i] = uint32(v1.local_accelerators[i])
		}
	}

	return out, nil
}

// FabricTelemetryCategory identifies a fabric telemetry category
// (amdsmi_fabric_telemetry_category_t).
type FabricTelemetryCategory uint32

const (
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_UALOE           FabricTelemetryCategory = 0
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_SWITCH          FabricTelemetryCategory = 1
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_CRYPTO          FabricTelemetryCategory = 2
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_PFC             FabricTelemetryCategory = 3
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_NETPORT         FabricTelemetryCategory = 4
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_UALOE   FabricTelemetryCategory = 5
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_NETPORT FabricTelemetryCategory = 6
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX             FabricTelemetryCategory = 7
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_INVALID         FabricTelemetryCategory = 0xFFFFFFFF
)

func (c FabricTelemetryCategory) String() string {
	switch c {
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_UALOE:
		return "UALOE"
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_SWITCH:
		return "SWITCH"
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_CRYPTO:
		return "CRYPTO"
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_PFC:
		return "PFC"
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_NETPORT:
		return "NETPORT"
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_UALOE:
		return "DERIVED_UALOE"
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_NETPORT:
		return "DERIVED_NETPORT"
	case AMDSMI_FABRIC_TELEMETRY_CATEGORY_INVALID:
		return "INVALID"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", uint32(c))
	}
}

// Fabric telemetry category bitmask helpers (mirror the C
// AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_* macros).
const (
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_UALOE           uint32 = 1 << uint32(AMDSMI_FABRIC_TELEMETRY_CATEGORY_UALOE)
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_SWITCH          uint32 = 1 << uint32(AMDSMI_FABRIC_TELEMETRY_CATEGORY_SWITCH)
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_CRYPTO          uint32 = 1 << uint32(AMDSMI_FABRIC_TELEMETRY_CATEGORY_CRYPTO)
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_PFC             uint32 = 1 << uint32(AMDSMI_FABRIC_TELEMETRY_CATEGORY_PFC)
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_NETPORT         uint32 = 1 << uint32(AMDSMI_FABRIC_TELEMETRY_CATEGORY_NETPORT)
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_DERIVED_UALOE   uint32 = 1 << uint32(AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_UALOE)
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_MASK_DERIVED_NETPORT uint32 = 1 << uint32(AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_NETPORT)
)

// FabricTelemetryItem is a single (id, value) telemetry sample
// (amdsmi_fabric_telemetry_item_t).
type FabricTelemetryItem struct {
	ID    uint64
	Value uint64
}

// FabricTelemetryInstance holds the telemetry items for a single instance of
// a category (amdsmi_fabric_telemetry_instance_t).
type FabricTelemetryInstance struct {
	Name       string
	LogicalIdx uint32
	ItemCount  uint32
	Items      []FabricTelemetryItem
}

// FabricTelemetryDataset holds all telemetry instances reported for one
// category (amdsmi_fabric_telemetry_dataset_t).
type FabricTelemetryDataset struct {
	Category        FabricTelemetryCategory
	GenerationCount uint64
	TimestampSec    int64
	TimestampNsec   int64
	InstanceCount   uint32
	Instances       []FabricTelemetryInstance
}

// FabricTelemetryData is the parsed snapshot of fabric telemetry returned by
// (*FabricTelemetry).Get and GetFabricTelemetry.
//
// Datasets is keyed by category and only contains entries for categories that
// were requested at allocation time. Categories that were requested but not
// populated by the driver appear with InstanceCount == 0.
type FabricTelemetryData struct {
	Datasets map[FabricTelemetryCategory]FabricTelemetryDataset
}

// allFabricTelemetryCategories is the default category set used when callers
// do not specify any (mirrors the Python AmdSmiFabricTelemetry.CATEGORIES
// dict).
var allFabricTelemetryCategories = []FabricTelemetryCategory{
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_UALOE,
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_SWITCH,
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_CRYPTO,
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_PFC,
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_NETPORT,
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_UALOE,
	AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_NETPORT,
}

// FabricTelemetry is the lifecycle wrapper around C-allocated telemetry
// storage (amdsmi_fabric_telemetry_t). It mirrors the Python
// AmdSmiFabricTelemetry class: AllocFabricTelemetry allocates, Get refreshes
// and parses, and Close frees the storage.
//
// FabricTelemetry is not safe for concurrent use. The zero value is invalid;
// always obtain instances via AllocFabricTelemetry.
type FabricTelemetry struct {
	ph         ProcessorHandle
	ptr        *C.amdsmi_fabric_telemetry_t
	categories []FabricTelemetryCategory
	closed     bool
}

// AllocFabricTelemetry allocates telemetry storage on the device for the
// requested categories (amdsmi_alloc_fabric_telemetry).
//
// If categories is empty, all categories from UALOE through DERIVED_NETPORT
// are requested (matching the Python default).
//
// The returned *FabricTelemetry must be released with Close once no longer
// needed. A finalizer is installed as a safety net, but callers should not
// rely on it.
func AllocFabricTelemetry(ph ProcessorHandle, categories ...FabricTelemetryCategory) (*FabricTelemetry, error) {
	if len(categories) == 0 {
		categories = append([]FabricTelemetryCategory(nil), allFabricTelemetryCategories...)
	} else {
		for _, c := range categories {
			if c >= AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX {
				return nil, fmt.Errorf("invalid fabric telemetry category: %s", c)
			}
		}
	}

	var mask uint32
	for _, c := range categories {
		mask |= 1 << uint32(c)
	}

	var cPtr *C.amdsmi_fabric_telemetry_t
	ret := C.amdsmi_alloc_fabric_telemetry(ph.cPtr(), C.uint32_t(mask), &cPtr)
	if err := checkStatus(Status(ret)); err != nil {
		return nil, err
	}
	if cPtr == nil {
		return nil, fmt.Errorf("amdsmi_alloc_fabric_telemetry returned a NULL telemetry pointer")
	}

	t := &FabricTelemetry{
		ph:         ph,
		ptr:        cPtr,
		categories: append([]FabricTelemetryCategory(nil), categories...),
	}
	runtime.SetFinalizer(t, func(t *FabricTelemetry) { _ = t.Close() })
	return t, nil
}

// Get refreshes the telemetry snapshot in the underlying storage and returns
// a parsed copy (amdsmi_get_fabric_telemetry_data).
//
// The returned FabricTelemetryData is fully detached from C memory and remains
// valid after Close.
func (t *FabricTelemetry) Get() (FabricTelemetryData, error) {
	if t == nil || t.closed || t.ptr == nil {
		return FabricTelemetryData{}, fmt.Errorf("fabric telemetry has been closed")
	}

	ret := C.amdsmi_get_fabric_telemetry_data(t.ph.cPtr(), t.ptr)
	if err := checkStatus(Status(ret)); err != nil {
		return FabricTelemetryData{}, err
	}

	result := FabricTelemetryData{
		Datasets: make(map[FabricTelemetryCategory]FabricTelemetryDataset, len(t.categories)),
	}

	for _, category := range t.categories {
		ds := C.amdsmi_go_fabric_telemetry_dataset_at(t.ptr, C.uint(category))
		if ds == nil {
			result.Datasets[category] = FabricTelemetryDataset{Category: category}
			continue
		}

		instCount := uint32(ds.instance_count)
		instances := make([]FabricTelemetryInstance, 0, instCount)
		for j := uint32(0); j < instCount; j++ {
			inst := C.amdsmi_go_fabric_telemetry_instance_at(ds, C.uint(j))
			if inst == nil {
				continue
			}

			itemCount := uint32(inst.item_count)
			items := make([]FabricTelemetryItem, itemCount)
			for k := uint32(0); k < itemCount; k++ {
				it := C.amdsmi_go_fabric_telemetry_item_at(inst, C.uint(k))
				items[k] = FabricTelemetryItem{
					ID:    uint64(it.id),
					Value: uint64(it.value),
				}
			}

			instances = append(instances, FabricTelemetryInstance{
				Name:       C.GoString(&inst.name.text[0]),
				LogicalIdx: uint32(inst.logical_idx),
				ItemCount:  itemCount,
				Items:      items,
			})
		}

		result.Datasets[category] = FabricTelemetryDataset{
			Category:        FabricTelemetryCategory(ds.category),
			GenerationCount: uint64(ds.generation_count),
			TimestampSec:    int64(ds.timestamp.tv_sec),
			TimestampNsec:   int64(ds.timestamp.tv_nsec),
			InstanceCount:   instCount,
			Instances:       instances,
		}
	}

	return result, nil
}

// Close releases the telemetry storage (amdsmi_free_fabric_telemetry).
// Calling Close more than once is a no-op.
func (t *FabricTelemetry) Close() error {
	if t == nil || t.closed {
		return nil
	}
	t.closed = true
	runtime.SetFinalizer(t, nil)
	if t.ptr == nil {
		return nil
	}
	ret := C.amdsmi_free_fabric_telemetry(t.ph.cPtr(), t.ptr)
	t.ptr = nil
	return checkStatus(Status(ret))
}

// Categories returns the list of categories this telemetry session was
// allocated for.
func (t *FabricTelemetry) Categories() []FabricTelemetryCategory {
	if t == nil {
		return nil
	}
	out := make([]FabricTelemetryCategory, len(t.categories))
	copy(out, t.categories)
	return out
}

// GetFabricTelemetry is a convenience wrapper that allocates a telemetry
// session for the given categories, fetches one snapshot, and frees the
// session before returning. It mirrors the Python amdsmi_get_fabric_telemetry
// helper.
//
// If categories is empty, all categories from UALOE through DERIVED_NETPORT
// are requested.
func GetFabricTelemetry(ph ProcessorHandle, categories ...FabricTelemetryCategory) (FabricTelemetryData, error) {
	t, err := AllocFabricTelemetry(ph, categories...)
	if err != nil {
		return FabricTelemetryData{}, err
	}
	defer t.Close()
	return t.Get()
}
