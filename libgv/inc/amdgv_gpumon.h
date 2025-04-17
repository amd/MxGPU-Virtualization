/*
 * Copyright (c) 2017-2022 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __AMDGV_GPUMON_H__
#define __AMDGV_GPUMON_H__
#include "amdgv_api.h"

#define STRLEN_NORMAL	(32)
#define STRLEN_LONG	(64)
#define STRLEN_VERYLONG (256)

#define MAX_ADAPTERS_IN_SYSTEM (8)
#define MAX_MM_IP_COUNT (8)

#define AMDGV_GPUMON_MAX_NUM_XGMI_PHYSICAL_LINK 64
#define AMDGV_GPUMON_MAX_FB_SHARING_GROUPS 64
#define AMDGV_GPUMON_MAX_NUM_CONNECTED_NODES 64

#define AMDGV_GPUMON_METRIC_NOT_APPLICABLE -1

#define AMDGV_GPUMON_MAX_NUM_POLICIES AMDGV_GPUMON_SOC_PSTATE_COUNT

#define AMDGV_GPUMON_CACHE_FLAGS_ENABLED	0x00000001
#define AMDGV_GPUMON_CACHE_FLAGS_DATA_CACHE	0x00000002
#define AMDGV_GPUMON_CACHE_FLAGS_INST_CACHE	0x00000004
#define AMDGV_GPUMON_CACHE_FLAGS_CPU_CACHE	0x00000008
#define AMDGV_GPUMON_CACHE_FLAGS_SIMD_CACHE	0x00000010
#define AMDGV_GPUMON_MAX_CACHE_TYPES 10

#define AMDGV_GPUMON_RAS_MAX_NUM_SAFE_RANGES 64

#define AMDGV_GPUMON_MAX_NUM_NUMA_NODE 32
#define AMDGV_GPUMON_MAX_NUM_SPATIAL_PARTITION 32

#define AMDGV_GPUMON_MAX_NUM_METRICS_EXT 	255

#define AMDGV_GPUMON_METRIC_EXT_UNIT_SHIFT 		0ULL
#define AMDGV_GPUMON_METRIC_EXT_NAME_SHIFT 		16ULL
#define AMDGV_GPUMON_METRIC_EXT_CATEGORY_SHIFT 		32ULL
#define AMDGV_GPUMON_METRIC_EXT_FLAGS_SHIFT 		48ULL

#define METRIC_EXT_FLAG(FLAG)  (1ULL << AMDGV_GPUMON_METRIC_EXT_FLAG__##FLAG)
#define METRIC_EXT_CODE(CATEGORY, NAME, UNIT, FLAGS) \
	((((uint64_t)(AMDGV_GPUMON_METRIC_EXT_CATEGORY__##CATEGORY)) << AMDGV_GPUMON_METRIC_EXT_CATEGORY_SHIFT) | \
	(((uint64_t)(AMDGV_GPUMON_METRIC_EXT_NAME__##NAME)) << AMDGV_GPUMON_METRIC_EXT_NAME_SHIFT) | \
	(((uint64_t)(AMDGV_GPUMON_METRIC_EXT_UNIT__##UNIT)) << AMDGV_GPUMON_METRIC_EXT_UNIT_SHIFT) | \
	(((uint64_t)(FLAGS)) << AMDGV_GPUMON_METRIC_EXT_FLAGS_SHIFT))

struct amdgv_live_info_gpumon;

enum AMDGV_GPU_STATUS {
	AMDGV_GPU_NOT_READY,
	AMDGV_GPU_READY,
};

enum AMDGV_DRV_STATUS {
	AMDGV_DRV_NOT_READY,
	AMDGV_DRV_READY,
};

enum AMDGV_VF_STATUS {
	AMDGV_VF_STATUS_NOT_TOUCH = 0,
	AMDGV_VF_STATUS_START_INIT,
	AMDGV_VF_STATUS_END_INIT,
	AMDGV_VF_STATUS_START_UNINIT,
	AMDGV_VF_STATUS_END_UNINIT,
	AMDGV_VF_STATUS_UNDER_INIT_AFTER_RESET,
};

enum AMDGV_PP_CLK_DOMAIN {
	AMDGV_PP_CLK_GFX,
	AMDGV_PP_CLK_SOC,
	AMDGV_PP_CLK_MEM,
	AMDGV_PP_CLK_VID,
	AMDGV_PP_CLK_DCLK_0,
	AMDGV_PP_CLK_DCLK_1,
	AMDGV_PP_CLK_VCLK_0,
	AMDGV_PP_CLK_VCLK_1,
	AMDGV_PP_CLK__MAX
};

/* Legacy struct. Use amdgv_gpumon_metrics_ext instead. */
struct amdgv_gpumon_metrics {
	struct {
		uint32_t curr;
		uint32_t avg;
		uint32_t avg_preDs;
		uint32_t avg_postDs;
		uint8_t ds_disabled;
	} clocks[AMDGV_PP_CLK__MAX];

	uint32_t mem_usage;
	uint32_t gfx_usage;
	uint32_t mm_usage;
	uint32_t mm_ip_usage[MAX_MM_IP_COUNT];
	uint32_t volt_soc;
	uint32_t volt_gfx;
	uint32_t volt_mem;
	uint32_t power;
	uint64_t energy;
	uint32_t temp_edge;
	uint32_t temp_mem;
	uint32_t temp_hotspot;
	uint32_t temp_plx;
	uint32_t temp_multi;
	uint8_t dpm;
	uint32_t fan_speed;
	uint32_t temp_edge_limit;
	uint32_t temp_mem_limit;
	uint32_t temp_hotspot_limit;
	uint32_t power_limit;
	uint8_t  pcie_rate;
	uint8_t  pcie_width;
	uint64_t serial;
	uint32_t shutdown_temp;
	uint32_t pcie_bandwidth;		/* Mb/s */

	uint64_t pcie_l0_to_recovery_count;
	uint64_t pcie_replay_count;
	uint64_t pcie_replay_roll_over_count;
	uint64_t pcie_nak_sent_count;
	uint64_t pcie_nak_received_count;
	uint32_t temp_edge_shutdown; /* CTF temp */
	uint32_t temp_mem_shutdown; /* CTF temp */
	uint32_t temp_hotspot_shutdown; /* CTF temp */
};

struct amdgv_vbios_info {
	uint8_t name[STRLEN_LONG];
	uint32_t dbdf;
	uint8_t vbios_pn[STRLEN_LONG];
	uint32_t version; // Firmware version
	uint8_t vbios_version_string[STRLEN_NORMAL]; // VBIOS version
	uint8_t date[STRLEN_NORMAL];
	uint64_t serial;
	uint32_t dev_id;
	uint32_t rev_id;
	uint32_t sub_dev_id;
	uint32_t sub_ved_id;
};

struct amdgv_vbiosinfos {
	int gpu_count;
	struct amdgv_vbios_info vbiosinfos[AMDGV_MAX_GPU_NUM];
};

struct amdgv_product_info {
	bool visit;
	bool valid;
	uint8_t model_number[STRLEN_VERYLONG];
	uint8_t product_serial[STRLEN_VERYLONG];
	uint8_t fru_id[STRLEN_VERYLONG];
	uint8_t product_name[STRLEN_VERYLONG];
	uint8_t manufacturer_name[STRLEN_VERYLONG];
};

struct amdgv_gpumon_temp {
	uint32_t temp_edge;
	uint32_t temp_mem;
	uint32_t temp_hotspot;
	uint32_t temp_plx;
	uint32_t temp_multi;
};

enum amdgv_gpumon_vram_type {
	AMDGV_GPUMON_DGPU_VRAM_TYPE__GDDR5 = 0x50,
	AMDGV_GPUMON_DGPU_VRAM_TYPE__HBM2 = 0x60,
	AMDGV_GPUMON_DGPU_VRAM_TYPE__HBM2E = 0x61,
	AMDGV_GPUMON_DGPU_VRAM_TYPE__GDDR6 = 0x70,
	AMDGV_GPUMON_DGPU_VRAM_TYPE__HBM3 = 0x80,
	AMDGV_GPUMON_DGPU_VRAM_TYPE__GDDR7 = 0x90,
};

enum amdgv_gpumon_vram_vendor {
	AMDGV_GPUMON_VRAM_VENDOR__PLACEHOLDER0,
	AMDGV_GPUMON_VRAM_VENDOR__SAMSUNG,
	AMDGV_GPUMON_VRAM_VENDOR__INFINEON,
	AMDGV_GPUMON_VRAM_VENDOR__ELPIDA,
	AMDGV_GPUMON_VRAM_VENDOR__ETRON,

	AMDGV_GPUMON_VRAM_VENDOR__NANYA,
	AMDGV_GPUMON_VRAM_VENDOR__HYNIX,
	AMDGV_GPUMON_VRAM_VENDOR__MOSEL,
	AMDGV_GPUMON_VRAM_VENDOR__WINBOND,
	AMDGV_GPUMON_VRAM_VENDOR__ESMT,

	AMDGV_GPUMON_VRAM_VENDOR__PLACEHOLDER1,
	AMDGV_GPUMON_VRAM_VENDOR__PLACEHOLDER2,
	AMDGV_GPUMON_VRAM_VENDOR__PLACEHOLDER3,
	AMDGV_GPUMON_VRAM_VENDOR__PLACEHOLDER4,
	AMDGV_GPUMON_VRAM_VENDOR__PLACEHOLDER5,

	AMDGV_GPUMON_VRAM_VENDOR__MICRON,
};

struct amdgv_gpumon_vram_info {
	enum amdgv_gpumon_vram_type vram_type;
	enum amdgv_gpumon_vram_vendor vram_vendor;
	uint32_t vram_size_mb;
	uint32_t vram_bit_width;
};

enum amdgv_pp_pm_policy {
	AMDGV_PP_PM_POLICY_NONE = -1,
	AMDGV_PP_PM_POLICY_SOC_PSTATE = 0,
	AMDGV_PP_PM_POLICY_XGMI_PLPD,
	AMDGV_PP_PM_POLICY_NUM,
};

enum amdgv_pp_policy_soc_pstate {
	SOC_PSTATE_DEFAULT = 0,
	SOC_PSTATE_0,
	SOC_PSTATE_1,
	SOC_PSTATE_2,
	SOC_PSTATE_COUNT,
};

enum amdgv_pp_policy_plpd {
	PLPD_DISALLOW = 0,
	PLPD_DEFAULT,
	PLPD_OPTIMIZED,
	PLPD_COUNT,
};

#define PP_POLICY_MAX_LEVELS 5

enum amdgv_gpumon_policy_soc_pstate {
	AMDGV_GPUMON_SOC_PSTATE_DEFAULT = 0,
	AMDGV_GPUMON_SOC_PSTATE_0,
	AMDGV_GPUMON_SOC_PSTATE_1,
	AMDGV_GPUMON_SOC_PSTATE_2,
	AMDGV_GPUMON_SOC_PSTATE_COUNT,
};

enum amdgv_gpumon_policy_plpd {
	AMDGV_GPUMON_PLPD_DISALLOW = 0,
	AMDGV_GPUMON_PLPD_DEFAULT,
	AMDGV_GPUMON_PLPD_OPTIMIZED,
	AMDGV_GPUMON_PLPD_COUNT,
};

struct smu_dpm_policy_desc {
	uint32_t policy_id;
	char *policy_description;
};

struct amdgv_gpumon_smu_dpm_policy {
	enum amdgv_pp_pm_policy policy_type;
	struct smu_dpm_policy_desc policies[AMDGV_GPUMON_MAX_NUM_POLICIES];
	int current_level;
};


enum amdgv_gpumon_card_form_factor {
	AMDGV_GPUMON_CARD_FORM_FACTOR__PCIE,
	AMDGV_GPUMON_CARD_FORM_FACTOR__OAM,
};

enum amdgv_gpumon_link_status {
	AMDGV_GPUMON_LINK_STATUS_ENABLED = 0,
	AMDGV_GPUMON_LINK_STATUS_DISABLED = 1,
	AMDGV_GPUMON_LINK_STATUS_ERROR = 2
};

enum amdgv_gpumon_link_type {
	AMDGV_GPUMON_LINK_TYPE_PCIE,
	AMDGV_GPUMON_LINK_TYPE_XGMI3,
	AMDGV_GPUMON_LINK_TYPE_NOT_APPLICABLE,
	AMDGV_GPUMON_LINK_TYPE_UNKNOWN
};

enum amdgv_gpumon_xgmi_fb_sharing_mode {
	AMDGV_GPUMON_XGMI_FB_SHARING_MODE_CUSTOM = 0,
	AMDGV_GPUMON_XGMI_FB_SHARING_MODE_1      = 1,
	AMDGV_GPUMON_XGMI_FB_SHARING_MODE_2      = 2,
	AMDGV_GPUMON_XGMI_FB_SHARING_MODE_4      = 4,
	AMDGV_GPUMON_XGMI_FB_SHARING_MODE_8      = 8
};

struct amdgv_gpumon_link_metrics {

	uint32_t num_links;
	struct {
		enum amdgv_gpumon_link_type link_type;
		uint32_t bdf;
		uint32_t speed; /* Gbps */
		uint32_t width;
		uint64_t read; /* KB */
		uint64_t write; /* KB */
	} links[AMDGV_GPUMON_MAX_NUM_XGMI_PHYSICAL_LINK];
};

union amdgv_gpumon_p2p_caps {
	struct {
		uint32_t is_iolink_coherent       :1;
		uint32_t is_iolink_atomics_32bit  :1;
		uint32_t is_iolink_atomics_64bit  :1;
		uint32_t is_iolink_dma            :1;
		uint32_t is_iolink_bi_directional :1;
		uint32_t reserved                 :27;
	};
	uint32_t p2p_caps_mask;
};

struct amdgv_gpumon_link_topology_info {
	uint64_t weight;
	enum amdgv_gpumon_link_status link_status;
	enum amdgv_gpumon_link_type link_type;
	uint8_t num_hops;
	uint8_t is_fb_sharing_enabled;
	union amdgv_gpumon_p2p_caps p2p_caps;
};

union amdgv_gpumon_xgmi_fb_sharing_caps {
	struct {
		uint32_t mode_custom_cap :1;
		uint32_t mode_1_cap      :1;
		uint32_t mode_2_cap      :1;
		uint32_t mode_4_cap      :1;
		uint32_t mode_8_cap      :1;
		uint32_t reserved        :27;
	};
	uint32_t xgmi_fb_sharing_cap_mask;
};

struct amdgv_gpumon_gpu_cache_info {

	uint32_t num_cache_types;

	struct {
		uint32_t cache_size_kb; /* In KB */
		uint32_t cache_level;
		uint32_t flags;
		uint32_t max_num_cu_shared;      /* Indicates how many Compute Units share this cache instance */
		uint32_t num_cache_instance; /* total number of instance of this cache type */
	} cache[AMDGV_GPUMON_MAX_CACHE_TYPES];
};

struct amdgv_gpumon_ras_safe_fb_address_ranges {
	uint32_t num_ranges;
	struct {
		uint64_t start;
		uint64_t size;
	} range[AMDGV_GPUMON_RAS_MAX_NUM_SAFE_RANGES];
};

enum amdgv_gpumon_fb_addr_type {
	AMDGV_GPUMON_FB_ADDR_SOC_PHY, /* SPA */
	AMDGV_GPUMON_FB_ADDR_BANK,
	AMDGV_GPUMON_FB_ADDR_VF_PHY, /* GPA */
	AMDGV_GPUMON_FB_ADDR_UNKNOWN,
};

struct amdgv_gpumon_fb_bank_addr {
	uint32_t stack_id; /* SID */
	uint32_t bank_group;
	uint32_t bank;
	uint32_t row;
	uint32_t column;
	uint32_t channel;
	uint32_t subchannel; /* Also called Pseudochannel (PC) */
};

struct amdgv_gpumon_fb_vf_phy_addr {
	uint32_t idx_vf;
	uint64_t addr;
};

union amdgv_gpumon_translate_fb_address {
	struct amdgv_gpumon_fb_bank_addr bank_addr;
	uint64_t soc_phy_addr;
	struct amdgv_gpumon_fb_vf_phy_addr vf_phy_addr;
};

#define AMDGV_GPUMON_MAX_NUM_NUMA_NODE	       32
#define AMDGV_GPUMON_MAX_NUM_SPATIAL_PARTITION 32

union amdgv_gpumon_memory_partition_config {
	struct {
		uint32_t nps1_cap : 1;
		uint32_t nps2_cap : 1;
		uint32_t nps4_cap : 1;
		uint32_t nps8_cap : 1;
		uint32_t reserved : 26;
	} mp_caps;
	uint32_t mp_cap_mask;
};

struct amdgv_gpumon_spatial_partition_caps {
	uint32_t num_xcc;
	uint32_t num_sdma;
	uint32_t num_vcn;
	uint32_t num_jpeg;
};

#define AMDGV_GPUMON_MAX_ACCELERATOR_PARTITION_PROFILES 32
#define AMDGV_GPUMON_MAX_CP_RESOURCE_PROFILES		32
#define AMDGV_GPUMON_MAX_ACCELERATOR_PARTITIONS		8

enum amdgv_gpumon_accelerator_partition_resource_type {
	AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_ENCODER,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DMA,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_JPEG,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_MAX
};

enum amdgv_gpumon_acccelerator_partition_type {
	AMDGV_GPUMON_ACCELERATOR_PARTITION_INVALID = 0,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_SPX,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_DPX,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_TPX,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_QPX,
	AMDGV_GPUMON_ACCELERATOR_PARTITION_CPX
};

struct amdgv_gpumon_acccelerator_partition_profile {
	uint32_t profile_index;
	enum amdgv_gpumon_acccelerator_partition_type profile_type;
	union amdgv_gpumon_memory_partition_config memory_caps;
	uint32_t num_partitions; // On MI300X, SPX: 1, DPX: 2, QPX: 4, CPX: 8, the length of resources array
	uint32_t partition_id[AMDGV_GPUMON_MAX_ACCELERATOR_PARTITIONS];
	uint32_t num_resources;
	uint32_t resources[AMDGV_GPUMON_MAX_ACCELERATOR_PARTITIONS]
			  [AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_MAX];
	uint32_t support_vf_num;
};

struct amdgv_gpumon_accelerator_partition_resource_profile {
	uint32_t resource_index;
	enum amdgv_gpumon_accelerator_partition_resource_type resource_type;
	uint32_t partition_resource;
	uint32_t num_partitions_share_resource; // If it is greater than 1, then resource is shared.
	uint32_t padding[11];
};

struct amdgv_gpumon_accelerator_partition_profile_config {
	uint32_t number_of_resource_profiles;
	struct amdgv_gpumon_accelerator_partition_resource_profile
		resource_profiles[AMDGV_GPUMON_MAX_CP_RESOURCE_PROFILES];
	uint32_t number_of_profiles;
	struct amdgv_gpumon_acccelerator_partition_profile
		profiles[AMDGV_GPUMON_MAX_ACCELERATOR_PARTITION_PROFILES];
	uint32_t default_profile_index;
};

struct amdgv_gpumon_memory_partition_info {
	enum amdgv_memory_partition_mode memory_partition_mode;
	uint32_t num_numa_ranges;
	struct {
		enum amdgv_gpumon_vram_type memory_type;
		uint64_t start;
		uint64_t end;
	} numa_range[AMDGV_MAX_NUMA_NODES];
};

struct amdgv_gpumon_gfx_config {
	uint32_t max_shader_engines;
	uint32_t max_cu_per_sh;
	uint32_t max_sh_per_se;
	uint32_t max_waves_per_simd;
	uint32_t wave_size;
	uint32_t active_cu_count;
	uint32_t major;
	uint32_t minor;
};

enum amdgv_gpumon_metric_ext_category {
	AMDGV_GPUMON_METRIC_EXT_CATEGORY__ACC_COUNTER = 0ULL,
	AMDGV_GPUMON_METRIC_EXT_CATEGORY__FREQUENCY = 1ULL,
	AMDGV_GPUMON_METRIC_EXT_CATEGORY__ACTIVITY = 2ULL,
	AMDGV_GPUMON_METRIC_EXT_CATEGORY__TEMPERATURE = 3ULL,
	AMDGV_GPUMON_METRIC_EXT_CATEGORY__POWER = 4ULL,
	AMDGV_GPUMON_METRIC_EXT_CATEGORY__ENERGY = 5ULL,
	AMDGV_GPUMON_METRIC_EXT_CATEGORY__THROTTLE = 6ULL,
	AMDGV_GPUMON_METRIC_EXT_CATEGORY__PCIE = 7ULL,
};

enum amdgv_gpumon_metric_ext_name {
	AMDGV_GPUMON_METRIC_EXT_NAME__METRIC_ACC_COUNTER = 0ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__FW_TIMESTAMP = 1ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX = 2ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_SOC = 3ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_MEM = 4ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_VCLK = 5ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_DCLK = 6ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_GFX = 7ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_MEM = 8ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_MM = 9ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_VCN = 10ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_JPEG = 11ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__VOLT_GFX = 12ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__VOLT_SOC = 13ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__VOLT_MEM = 14ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_HOTSPOT_CURR = 15ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_HOTSPOT_LIMIT = 16ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_MEM_CURR = 17ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_MEM_LIMIT = 18ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_VR_CURR = 19ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_SHUTDOWN = 20ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__POWER_CURR = 21ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__POWER_LIMIT = 22ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_SOCKET = 23ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_CCD = 24ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_XCD = 25ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_AID = 26ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_MEM = 27ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__THROTTLE_SOCKET_ACTIVE = 28ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__THROTTLE_VR_ACTIVE = 29ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__THROTTLE_MEM_ACTIVE = 30ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_BANDWIDTH = 31ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_L0_TO_RECOVERY_COUNT = 32ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_REPLAY_COUNT = 33ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_REPLAY_ROLLOVER_COUNT = 34ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_NAK_SENT_COUNT = 35ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_NAK_RECEIVED_COUNT = 36ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX_MAX_LIMIT = 37ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_SOC_MAX_LIMIT = 38ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_MEM_MAX_LIMIT = 39ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_VCLK_MAX_LIMIT = 40ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_DCLK_MAX_LIMIT = 41ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX_MIN_LIMIT = 42ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_SOC_MIN_LIMIT = 43ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_MEM_MIN_LIMIT = 44ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_VCLK_MIN_LIMIT = 45ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_DCLK_MIN_LIMIT = 46ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX_LOCKED = 47ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX_DS_DISABLED = 48ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_MEM_DS_DISABLED = 49ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_SOC_DS_DISABLED = 50ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_VCLK_DS_DISABLED = 51ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__CLK_DCLK_DS_DISABLED = 52ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_LINK_SPEED = 53ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_LINK_WIDTH = 54ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__THROTTLE_PROCHOT_ACTIVE = 55ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__THROTTLE_PPT_ACTIVE = 56ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__DRAM_BANDWIDTH = 57ULL,
	AMDGV_GPUMON_METRIC_EXT_NAME__MAX_DRAM_BANDWIDTH = 58ULL,
};

enum amdgv_gpumon_metric_ext_unit {
	AMDGV_GPUMON_METRIC_EXT_UNIT__COUNTER = 0ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__UINT = 1ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__BOOL = 2ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__MHZ = 3ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__PERCENT = 4ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__MILLIVOLT = 5ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__CELSIUS = 6ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__WATT = 7ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__JOULE = 8ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__GBPS = 9ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__MBITPS = 10ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__PCIE_GEN = 11ULL,
	AMDGV_GPUMON_METRIC_EXT_UNIT__PCIE_LANES = 12ULL,
};

enum amdgv_gpumon_metric_ext_flag {
	AMDGV_GPUMON_METRIC_EXT_FLAG__COUNTER		= 0ULL,
	AMDGV_GPUMON_METRIC_EXT_FLAG__CHIPLET_METRIC	= 1ULL,
	AMDGV_GPUMON_METRIC_EXT_FLAG__DATA_FILTER_INST	= 2ULL,
	AMDGV_GPUMON_METRIC_EXT_FLAG__DATA_FILTER_ACC	= 3ULL,
};


/****************************************************
 * metric code
 * bits [0  - 15] are metric units
 * bits [16 - 31] are metric names
 * bits [32 - 47] are metric categories
 * bits [48 - 63] are metric flags
 ****************************************************/
struct amdgv_gpumon_metric_ext {
	union {
		struct {
			enum amdgv_gpumon_metric_ext_unit 	unit 	 : 16;
			enum amdgv_gpumon_metric_ext_name 	name 	 : 16;
			enum amdgv_gpumon_metric_ext_category 	category : 16;
			uint64_t flag_counter				 : 1;
			uint64_t flag_chiplet_metric			 : 1;
			uint64_t flag_data_filter_inst			 : 1;
			uint64_t flag_data_filter_acc			 : 1;
			uint64_t flag_reserved				 : 12;
		};
		uint64_t code;
	};
	uint32_t vf_mask; /* Mask of all active VFs + PF that this metric applies to */
	uint64_t val;
};

struct amdgv_gpumon_metrics_ext {
	uint64_t drv_timestamp;
	uint32_t num_metric;
	struct amdgv_gpumon_metric_ext metric[AMDGV_GPUMON_MAX_NUM_METRICS_EXT];
};

/* VF Query Functions */
int amdgv_gpumon_get_metrics(amdgv_dev_t dev, struct amdgv_gpumon_metrics *metrics);
int amdgv_gpumon_get_gpu_power_usage(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_gpu_power_capacity(amdgv_dev_t dev, int *val);
int amdgv_gpumon_set_gpu_power_capacity(amdgv_dev_t dev, int val);
int amdgv_gpumon_get_dpm_status(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_dpm_cap(amdgv_dev_t dev, int *val);

int amdgv_gpumon_get_asic_temperature(amdgv_dev_t dev, int *val);

int amdgv_gpumon_get_max_sclk(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_max_mclk(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_max_vclk0(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_max_vclk1(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_max_dclk0(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_max_dclk1(amdgv_dev_t dev, int *val);

int amdgv_gpumon_get_min_sclk(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_min_mclk(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_min_vclk0(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_min_vclk1(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_min_dclk0(amdgv_dev_t dev, int *val);
int amdgv_gpumon_get_min_dclk1(amdgv_dev_t dev, int *val);

int amdgv_gpumon_get_ecc_enabled(amdgv_dev_t dev, bool *val);
int amdgv_gpumon_get_ecc_support_flag(amdgv_dev_t dev, uint32_t *supported, uint32_t *enabled);
int amdgv_gpumon_ras_eeprom_clear(amdgv_dev_t dev);
void amdgv_gpumon_get_bad_page_record_count(amdgv_dev_t dev, int *bp_cnt);
int amdgv_gpumon_get_bad_page_info(amdgv_dev_t dev, uint32_t index,
				   struct amdgv_smi_ras_eeprom_table_record *record);
int amdgv_gpumon_ras_error_inject(amdgv_dev_t dev,
				  struct amdgv_smi_ras_error_inject_info *data);
int amdgv_gpumon_turn_on_ecc_injection(amdgv_dev_t dev, const uint8_t *passphrase);
int amdgv_gpumon_ras_get_badpages_info(amdgv_dev_t dev, struct amdgv_smi_ras_badpage **bp,
					   unsigned int *count);
int amdgv_gpumon_get_ras_eeprom_version(amdgv_dev_t dev, uint32_t *ras_eeprom_version);
int amdgv_gpumon_get_ecc_info(amdgv_dev_t dev, struct amdgv_smi_ras_query_if *info);
int amdgv_gpumon_get_ecc_correction_schema(amdgv_dev_t dev, uint32_t *ecc_correction_schema);
int amdgv_gpumon_clean_correctable_error_count(amdgv_dev_t dev, int *corr);
int amdgv_gpumon_ras_report(amdgv_dev_t dev, int ras_type);
int amdgv_gpumon_get_asic_serial(amdgv_dev_t dev, uint64_t *serial);
int amdgv_gpumon_get_asic_type(amdgv_dev_t dev, uint32_t *asic_type);
int amdgv_gpumon_get_dev_uuid(amdgv_dev_t dev, uint64_t *uuid);
int amdgv_gpumon_get_vbios_info(amdgv_dev_t dev, struct amdgv_vbios_info *vbios_info);
int amdgv_gpumon_get_product_info(amdgv_dev_t dev, struct amdgv_product_info *info);

/* VF Handling Functions */
int amdgv_gpumon_clear_vf_fb(amdgv_dev_t dev, uint32_t idx_vf, uint8_t pattern);
int amdgv_gpumon_alloc_vf(amdgv_dev_t dev, enum amdgv_set_vf_opt_type type,
			  struct amdgv_vf_option *opt);
int amdgv_gpumon_set_vf_num(amdgv_dev_t dev, uint8_t number_vfs);
int amdgv_gpumon_free_vf(amdgv_dev_t dev, uint32_t idx_vf);
int amdgv_gpumon_set_vf(amdgv_dev_t dev, enum amdgv_set_vf_opt_type type,
			struct amdgv_vf_option *opt);

/* Shared Helper Functions */
uint32_t amdgv_parse_divider(uint32_t divider);

int amdgv_vf_option_valid(amdgv_dev_t dev, enum amdgv_set_vf_opt_type opt_type,
			  struct amdgv_vf_option *opt);

bool amdgv_is_fb_overlap(amdgv_dev_t dev, struct amdgv_vf_option *opt);
bool amdgv_get_vf_gfx_part_valid(amdgv_dev_t dev, struct amdgv_vf_option *opt);
int amdgv_gpumon_control_fru_sigout(amdgv_dev_t dev, const uint8_t *passphrase);
struct gim_dev_data;
int amdgv_gpumon_get_pcie_confs(amdgv_dev_t dev, struct gim_dev_data *dev_data,
				int (*callback)(struct gim_dev_data *, int *, int *, int *),
				int *speed, int *width, int *max_vf);

int amdgv_gpumon_get_fw_err_records(amdgv_dev_t dev, uint8_t *num_records,
					struct amdgv_psp_mb_err_record *err_records);

int amdgv_gpumon_get_vf_fw_info(amdgv_dev_t dev, uint32_t idx_vf, uint8_t *num_fw,
				struct amdgv_firmware_info *fw_info);

int amdgv_gpumon_get_vram_info(amdgv_dev_t dev,
			       struct amdgv_gpumon_vram_info *vram_info);
int amdgv_gpumon_is_clk_locked(amdgv_dev_t dev,
			       enum AMDGV_PP_CLK_DOMAIN clk_domain,
			       uint8_t *clk_locked);
int amdgv_gpumon_get_accelerator_partition_profile_config_global(
	amdgv_dev_t dev,
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_configs);
int amdgv_gpumon_get_accelerator_partition_profile_config(
	amdgv_dev_t dev,
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_configs);
int amdgv_gpumon_get_accelerator_partition_profile(
	amdgv_dev_t dev,
	struct amdgv_gpumon_acccelerator_partition_profile *profile);
int amdgv_gpumon_set_accelerator_partition_profile(amdgv_dev_t dev,
						   uint32_t profile_index);
int amdgv_gpumon_get_memory_partition_config(
	amdgv_dev_t dev,
	union amdgv_gpumon_memory_partition_config *memory_partition_config);
int amdgv_gpumon_get_spatial_partition_caps(
	amdgv_dev_t dev,
	struct amdgv_gpumon_spatial_partition_caps *spatial_partition_caps);
int amdgv_gpumon_set_memory_partition_mode(
	amdgv_dev_t dev,
	enum amdgv_memory_partition_mode memory_partition_mode);
int amdgv_gpumon_set_spatial_partition_num(amdgv_dev_t dev,
					   uint32_t spatial_partition_num);
int amdgv_gpumon_reset_accelerator_partition_profile(amdgv_dev_t dev);
int amdgv_gpumon_reset_partition_mode(amdgv_dev_t dev);
int amdgv_gpumon_reset_spatial_partition_num(amdgv_dev_t dev);
int amdgv_gpumon_get_memory_partition_mode(
	amdgv_dev_t dev,
	struct amdgv_gpumon_memory_partition_info *memory_partition_info);
int amdgv_gpumon_get_spatial_partition_num(amdgv_dev_t dev,
					   uint32_t *spatial_partition_num);
int amdgv_gpumon_get_pcie_replay_count(amdgv_dev_t dev, int *pcie_replay_count);
int amdgv_gpumon_get_card_form_factor(
	amdgv_dev_t dev, enum amdgv_gpumon_card_form_factor *card_form_factor);
int amdgv_gpumon_get_max_configurable_power_limit(amdgv_dev_t dev,
			int *power_limit);
int amdgv_gpumon_get_default_power_limit(amdgv_dev_t dev,
			int *power_limit);
int amdgv_gpumon_get_min_power_limit(amdgv_dev_t dev,
			int *min_power_limit);
int amdgv_gpumon_get_num_metrics_ext_entries(amdgv_dev_t dev,
					     uint32_t *entries);
int amdgv_gpumon_get_metrics_ext(amdgv_dev_t dev,
				 struct amdgv_gpumon_metrics_ext *metrics_ext);
int amdgv_gpumon_is_power_management_enabled(amdgv_dev_t dev, bool *pm_enabled);

int amdgv_gpumon_get_link_metrics(amdgv_dev_t dev,
			 struct amdgv_gpumon_link_metrics *link_metrics);
int amdgv_gpumon_get_link_topology(amdgv_dev_t src_dev,
			amdgv_dev_t dest_dev, struct amdgv_gpumon_link_topology_info *topology_info);
int amdgv_gpumon_get_xgmi_fb_sharing_caps(amdgv_dev_t dev,
			union amdgv_gpumon_xgmi_fb_sharing_caps *caps);
int amdgv_gpumon_get_xgmi_fb_sharing_mode_info(amdgv_dev_t src_dev,
			amdgv_dev_t dest_dev, enum amdgv_gpumon_xgmi_fb_sharing_mode mode,
			uint8_t *is_sharing_enabled);
int amdgv_gpumon_set_xgmi_fb_sharing_mode(amdgv_dev_t dev,
			enum amdgv_gpumon_xgmi_fb_sharing_mode mode);
int amdgv_gpumon_set_xgmi_fb_custom_sharing_mode(uint32_t num_dev,
			amdgv_dev_t *dev_list);

int amdgv_gpumon_get_pm_policy(amdgv_dev_t dev,
			enum amdgv_pp_pm_policy p_type,
			struct amdgv_gpumon_smu_dpm_policy *policy);
int amdgv_gpumon_set_pm_policy_level(amdgv_dev_t dev,
			enum amdgv_pp_pm_policy p_type, int level);

int amdgv_gpumon_get_shutdown_temperature(amdgv_dev_t dev,
	int *shutdown_temp);

enum amdgv_xgmi_fb_sharing_mode amdgv_gpumon_xgmi_mode_map(
		enum amdgv_gpumon_xgmi_fb_sharing_mode gpumon_mode);
int amdgv_gpumon_init_metrics_buf(struct amdgv_gpumon_metrics *metrics);

int amdgv_gpumon_get_gpu_cache_info(amdgv_dev_t dev,
			struct amdgv_gpumon_gpu_cache_info *gpu_cache_info);

int amdgv_gpumon_get_gpu_max_pcie_link_generation(amdgv_dev_t dev,
			int *max_pcie_link_gen);

enum amdgv_live_info_status amdgv_gpumon_export_live_data(amdgv_dev_t dev, struct amdgv_live_info_gpumon *gpumon);
enum amdgv_live_info_status amdgv_gpumon_import_live_data(amdgv_dev_t dev, struct amdgv_live_info_gpumon *gpumon);

int amdgv_gpumon_ras_ta_load(amdgv_dev_t dev,
				 struct amdgv_smi_cmd_ras_ta_load *data);
int amdgv_gpumon_ras_ta_unload(amdgv_dev_t dev,
				struct amdgv_smi_cmd_ras_ta_unload *data);

int amdgv_gpumon_translate_fb_address(amdgv_dev_t dev, enum amdgv_gpumon_fb_addr_type src_type,
	enum amdgv_gpumon_fb_addr_type dest_type,
	union amdgv_gpumon_translate_fb_address src_addr,
	union amdgv_gpumon_translate_fb_address *dest_addr);

int amdgv_gpumon_get_ras_safe_fb_addr_ranges(amdgv_dev_t dev, struct amdgv_gpumon_ras_safe_fb_address_ranges *ranges);
int amdgv_gpumon_reset_all_error_counts(amdgv_dev_t dev);
int amdgv_gpumon_get_ras_session_id(amdgv_dev_t dev, uint64_t *session_ptr);
int amdgv_gpumon_ras_get_ta_version(amdgv_dev_t dev, unsigned char *fw_image, uint32_t *ver_ptr);
int amdgv_gpumon_ras_get_loaded_ta_version(amdgv_dev_t dev, uint32_t *ver_ptr);
int amdgv_gpumon_get_num_active_vfs(amdgv_dev_t dev, uint32_t *num_vfs);

int amdgv_gpumon_cper_get_count(amdgv_dev_t dev,
				uint64_t rptr, uint64_t *wptr,
				uint64_t *avail_count,
				uint64_t *size);
int amdgv_gpumon_cper_get_entries(amdgv_dev_t dev, uint64_t rptr,
				  void *buf, uint64_t buf_size,
				  uint64_t *write_count,
				  uint64_t *overflow_count,
				  uint64_t *left_size);
int amdgv_gpumon_get_gfx_config(amdgv_dev_t dev,
	struct amdgv_gpumon_gfx_config *config);

#endif // __AMDGV_GPUMON_H__
