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

#ifndef AMDGV_DEVICE_H
#define AMDGV_DEVICE_H

#include "amdgv_basetypes.h"
#include "amdgv_list.h"
#include "amdgv_api.h"
#include "amdgv.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_mailbox.h"
#include "amdgv_irqmgr.h"
#include "amdgv_gpuiov.h"
#include "amdgv_sched.h"
#include "amdgv_reset.h"
#include "amdgv_vbios.h"
#include "amdgv_sriovmsg.h"
#include "amdgv_psp.h"
#include "amdgv_powerplay.h"
#include "amdgv_gpumon_internal.h"
#include "amdgv_misc.h"
#include "amdgv_memmgr.h"
#include "amdgv_ip_discovery.h"
#include "amdgv_ucode.h"
#include "amdgv_ecc.h"
#include "amdgv_error.h"
#include "amdgv_error_internal.h"
#include "amdgv_clockgating.h"
#include "amdgv_umc.h"
#include "amdgv_task_barrier.h"
#include "amdgv_diag_data.h"
#include "amdgv_live_info.h"
#include "amdgv_live_migration.h"
#include "amdgv_dirtybit.h"
#include "amdgv_mmsch.h"
#include "amdgv_smuio.h"
#include "amdgv_gfx.h"
#include "amdgv_sdma.h"
#include "amdgv_nbio.h"
#include "amdgv_ring.h"
#include "amdgv_ffbm.h"
#include "amdgv_mcp.h"
#include "amdgv_df.h"
#include "amdgv_vcn.h"
#include "amdgv_xgmi.h"
#include "amdgv_mmhub.h"
#include "amdgv_mca.h"
#include "amdgv_debug.h"
#include "amdgv_cper.h"

#include "amdgv_ras_eeprom.h"


#define AMDGV_CAP_MANUAL_SCHED	    (1 << 0)
#define AMDGV_CAP_HUNG_HW_DETECT    (1 << 1)
#define AMDGV_CAP_HUNG_SW_RESET	    (1 << 2)
#define AMDGV_CAP_GFX_SOLID_SCHED   (1 << 3)
#define AMDGV_CAP_GFX_LIQUID_SCHED  (1 << 4)
#define AMDGV_CAP_GFX_HW_SCHED	    (1 << 5)
#define AMDGV_CAP_VCE_HW_SCHED	    (1 << 6)
#define AMDGV_CAP_HEVC_HW_SCHED	    (1 << 7)
#define AMDGV_CAP_GFX_MANUAL_SCHED  (1 << 8)
#define AMDGV_CAP_VCE_MANUAL_SCHED  (1 << 9)
#define AMDGV_CAP_HEVC_MANUAL_SCHED (1 << 10)

#define AMDGV_VBIOS_SIZE_KB	   AMD_SRIOV_MSG_VBIOS_SIZE_KB
#define AMDGV_DATAEXCHANGE_SIZE_KB AMD_SRIOV_MSG_DATAEXCHANGE_SIZE_KB
#define AMDGV_MECFW_SIZE_KB	   (512)
#define AMDGV_UVDFW_SIZE_KB	   (512)
#define AMDGV_VCEFW_SIZE_KB	   (512)

#define AMDGV_RESERVE_FB_OFFSET_KB AMD_SRIOV_MSG_VBIOS_OFFSET
#define AMDGV_RESERVE_FB_SIZE_KB   (AMDGV_VBIOS_SIZE_KB + AMDGV_DATAEXCHANGE_SIZE_KB + AMD_SRIOV_RAS_TELEMETRY_SIZE_KB)
#define AMDGV_RESERVE_FB_SIZE_KB_UCODE                                                        \
	(AMDGV_VBIOS_SIZE_KB + AMDGV_DATAEXCHANGE_SIZE_KB + AMDGV_MECFW_SIZE_KB +             \
	 AMDGV_UVDFW_SIZE_KB + AMDGV_VCEFW_SIZE_KB)
#define MAX_OS_FB_MAPPING_SIZE      (0x1ULL << 31) /* 2G - defined the value in shim driver */

#define AMDGV_AGP_APERTURE_SIZE (0x10ULL << 20)

#define HWIP_MAX_INSTANCE 28

#define AMDGV_NUM_VMID 16

#define AMDGV_MAX_FB_BLOCK_NUM 32 /* 12 VF + 13 MAX free slots. Reserve 32 slots for safety */
#define AMDGV_MIN_FB_SIZE_MB	2048

#define MAX_DUMP_LENGTH (1024 * 1024) /* 1M */
#ifdef WS_RECORD
#define MAX_RECORD_LENGTH (1024 * 1024) /* 1M */
#endif

#define CSA_TSL_SIZE 0x400

/* Break Point Debug Mode:
 * 0: default value, bp_mode is off
 * 1: pause at first init/run PF
 * 2: pause at every world switch event */
#define AMDGV_BP_MODE_DISABLE 	0
#define AMDGV_BP_MODE_1			1
#define AMDGV_BP_MODE_2 		2

INLINE uint32_t amdgv_ffs(int n)
{
	static const uint32_t _16bit_lookup[] = {
		0, 1, 2, 0,
		3, 0, 0, 0,
		4,
	};
	uint32_t shift;
	uint32_t i;
	uint32_t c = 0;

	if (!n)
		return 0;

	/* Clear all but LSB */
	i = n & -n;

	if (i > 0xffff) {
		i >>= 16;
		c = 16;
	}

	if (i <= 0xff) {
		if (i <= 0xf)
			shift = 0;
		else
			shift = 4;
	} else {
		if (i <= 0xfff)
			shift = 8;
		else
			shift = 12;
	}

	return _16bit_lookup[i >> shift] + shift + c;
}

/* Helper returns unsigned integer with all bits higher than idx filled*/
INLINE uint32_t amdgv_fill(uint32_t idx)
{
	static const uint32_t table[] = {
		0xfffffffe, 0xfffffffc, 0xfffffff8, 0xfffffff0,
		0xffffffe0, 0xffffffc0, 0xffffff80, 0xffffff00,
		0xfffffe00, 0xfffffc00, 0xfffff800, 0xfffff000,
		0xffffe000, 0xffffc000, 0xffff8000, 0xffff0000,
		0xfffe0000, 0xfffc0000, 0xfff80000, 0xfff00000,
		0xffe00000, 0xffc00000, 0xff800000, 0xff000000,
		0xfe000000, 0xfc000000, 0xf8000000, 0xf0000000,
		0xe0000000, 0xc0000000, 0x80000000, 0x0
	};

	if (idx > 31)
		return 0x0;

	return table[idx];
}

/*
This macro loops through all set bits inside the mask input
The mask input will not be modified.

This macro only works on 32 bit variables.

amdgv_ffs() will return the lowest bit set + 1.
amdgv_ffs() will return 0 if mask is 0.

We will test for index overflow to terminate the loop.
*/
#define for_each_id(i, mask) for (i = amdgv_ffs(mask) - 1; i != 0xFFFFFFFF; i = (amdgv_ffs((mask) & amdgv_fill(i)) - 1))

enum amdgv_ws_auto_run {
	AMDGV_WS_AUTO_RUN_DISABLED = 0,
	AMDGV_WS_AUTO_RUN_ENABLED  = 1,
};

enum amdgv_hub_idx {
	VM_GFXHUB = 0,
	VM_MMHUB0 = 1,
	VM_MMHUB1 = 2,
	VM_NUMHUB,
};

/* Define the HW IP blocks will be used in driver , add more if necessary */
enum amdgv_hw_ip_block_type {
	GC_HWIP = 1,
	HDP_HWIP,
	SDMA0_HWIP,
	SDMA1_HWIP,
	SDMA2_HWIP,
	SDMA3_HWIP,
	SDMA4_HWIP,
	SDMA5_HWIP,
	SDMA6_HWIP,
	SDMA7_HWIP,
	MMHUB_HWIP,
	ATHUB_HWIP,
	NBIO_HWIP,
	MP0_HWIP,
	MP1_HWIP,
	UVD_HWIP,
	VCN_HWIP = UVD_HWIP,
	VCE_HWIP,
	DF_HWIP,
	DCE_HWIP,
	OSSSYS_HWIP,
	SMUIO_HWIP,
	PWR_HWIP,
	NBIF_HWIP,
	THM_HWIP,
	CLK_HWIP,
	UMC_HWIP,
	RSMU_HWIP,
	LSDMA_HWIP,
	MAX_HWIP
};

enum {
	AMDGV_DOORBELL64_sDMA_ENGINE0	     = 0xF0,
	AMDGV_DOORBELL64_sDMA_HI_PRI_ENGINE0 = 0xF1,
	AMDGV_DOORBELL64_sDMA_ENGINE1	     = 0xF2,
	AMDGV_DOORBELL64_sDMA_HI_PRI_ENGINE1 = 0xF3,
	AMDGV_DOORBELL64_IH		     = 0xF4,
	AMDGV_DOORBELL64_MMSCH0_ENGINE0	     = 0xF8,
};

enum {
	AMDGV_DOORBELL64_sDMA_AI_ENGINE0	= 0xE0,
	AMDGV_DOORBELL64_sDMA_AI_HI_PRI_ENGINE0 = 0xE1,
	AMDGV_DOORBELL64_sDMA_AI_ENGINE1	= 0xE8,
	AMDGV_DOORBELL64_sDMA_AI_HI_PRI_ENGINE1 = 0xE9,
};

struct amdgv_adapter;

/* Reserved doorbells for amdgv (including multimedia).
 * KFD can use all the rest in the 2M doorbell bar.
 * For asic before vega10, doorbell is 32-bit, so the
 * index/offset is in dword. For vega10 and after, doorbell
 * can be 64-bit, so the index defined is in qword.
 */
struct amdgv_doorbell_index {
	uint32_t kiq;
	uint32_t mec_ring0;
	uint32_t mec_ring1;
	uint32_t mec_ring2;
	uint32_t mec_ring3;
	uint32_t mec_ring4;
	uint32_t mec_ring5;
	uint32_t mec_ring6;
	uint32_t mec_ring7;
	uint32_t userqueue_start;
	uint32_t userqueue_end;
	uint32_t gfx_ring0;
	uint32_t gfx_ring1;
	uint32_t gfx_userqueue_start;
	uint32_t gfx_userqueue_end;
	uint32_t sdma_engine[16];
	uint32_t mes_ring0;
	uint32_t mes_ring1;
	uint32_t ih;
	union {
		struct {
			uint32_t vcn_ring0_1;
			uint32_t vcn_ring2_3;
			uint32_t vcn_ring4_5;
			uint32_t vcn_ring6_7;
		} vcn;
		struct {
			uint32_t uvd_ring0_1;
			uint32_t uvd_ring2_3;
			uint32_t uvd_ring4_5;
			uint32_t uvd_ring6_7;
			uint32_t vce_ring0_1;
			uint32_t vce_ring2_3;
			uint32_t vce_ring4_5;
			uint32_t vce_ring6_7;
		} uvd_vce;
	};
	uint32_t first_non_cp;
	uint32_t last_non_cp;
	uint32_t max_assignment;
	/* Per engine SDMA doorbell size in dword */
	uint32_t sdma_doorbell_range;
	/* Per xcc doorbell size for KIQ/KCQ */
	uint32_t xcc_doorbell_range;
};

struct amdgv_init_func {
	/* Name of block */
	char name[32];
	bool hw_priority; /* true means we do hw_init for this func right after its sw_init */
	/* sets up driver state, does not configure hw */
	int (*sw_init)(struct amdgv_adapter *adapt);
	/* tears down driver state, does not configure hw */
	int (*sw_fini)(struct amdgv_adapter *adapt);
	/* sets up the hw state */
	int (*hw_init)(struct amdgv_adapter *adapt);
	/* tears down the hw state */
	int (*hw_fini)(struct amdgv_adapter *adapt);
	/* sets up the hw engine state */
	int (*hw_engine_init)(struct amdgv_adapter *adapt);
	/* tears down the hw engine state */
	int (*hw_engine_fini)(struct amdgv_adapter *adapt);
	/* sets up the hw state for live update (ring buffer MC address) */
	int (*hw_live_init)(struct amdgv_adapter *adapt);
	/* clean up after reset */
	int (*post_reset)(struct amdgv_adapter *adapt);
	/* clean up before reset */
	int (*pre_reset)(struct amdgv_adapter *adapt);
};

struct amdgv_live_info_func {
	enum amdgv_live_info_data live_info_op;
	/* save live update data to special memory region */
	int (*export_live_data)(struct amdgv_adapter *adapt, void *data);
	/* import live update data from special memory region */
	int (*import_live_data)(struct amdgv_adapter *adapt, void *data);
};

struct amdgv_asic_entry {
	uint32_t                    asic_type;
	uint32_t                    dev_id;
	uint32_t                    sub_dev_id;
	uint32_t                    rev_id;
	uint32_t                    caps;
	struct amdgv_init_func      *(*init_funcs)[];
	struct amdgv_live_info_func *(*live_info_funcs)[];
	struct amdgv_reg_range      *(*miti_table)[];
	void (*reg_base_init)(struct amdgv_adapter *adapt);
};

struct amdgv_pf_pcie_restore {
	int   offset;
	int   size;
	char *name;
};

struct amdgv_vf_ras {
	union amd_sriov_ras_caps caps;
	struct {
		uint32_t start_rptr;
		uint32_t prev_host_wptr;
	} cper;
};

struct amdgv_vf_device {
	oss_dev_t dev;
	bool configured;
	uint32_t idx_vf;
	uint32_t bdf;
	uint32_t func_id;

	/* record the vf fb offset/size required by host os
	 * used to validate vf option
	 * */
	uint32_t fb_offset_os;
	uint32_t fb_size_os;

	/* record the fb offset/size in TLB
	 * mapped address space with os required fb offset
	 * fb's offset and size are in MB
	 * */
	uint32_t fb_offset;
	uint32_t fb_size;
	/* record the real fb size at pci config space
	 * because there may be some truncate size
	 * */
	uint32_t real_fb_size;

	/* fb's offset and size with tmr area */
	uint32_t fb_offset_tmr;
	uint32_t fb_size_tmr;

	/* bandwidth is in MB */
	uint32_t mm_bandwidth[AMDGV_MAX_MM_ENGINE];

	/* time slice is in microseconds */
	uint32_t time_slice[AMDGV_SCHED_BLOCK_MAX];

	/* whether device resource is mapped */
	bool res_mapped;

	/* vf device's resources */
	struct oss_dev_res res;

	/* vf guard */
	struct amdgv_vf_guard *guard;

	/* whether vf is ready to be reset */
	bool ready_to_reset;

	enum AMDGV_VF_STATUS vf_status;

	bool unshutdown;

	bool gpu_init_data_ready;

	struct amdgv_firmware_info fw_info[AMDGV_FIRMWARE_ID__MAX];

	/* used for update pf2vf message */
	uint64_t retired_page;
	/* used for calc offset for store new retired page in vf */
	uint32_t bp_block_size;
	bool vram_lost;
	enum amdgv_ws_auto_run auto_run;

	/* UUID Info */
	struct amd_sriov_msg_uuid_info uuid_info;

	/* FFBM PTE Block List */
	struct amdgv_list_head gpa_list;   /* list of FFBM PTE blocks assigned to this VF */

	bool		       reset_notify_vf_pending;
	/* skip run after context loaded */
	bool skip_run;

	/* Operation Log for GPU Monitoring */
	struct amdgv_time_log time_log[AMDGV_MAX_NUM_HW_SCHED];

	bool mes_info_dump_enabled;

	struct amdgv_vf_ras ras;
};

struct amdgv_reg_golden {
	uint32_t	hwip;
	uint32_t	instance;
	uint32_t	segment;
	uint32_t	reg;
	uint32_t	and_mask;
	uint32_t	or_mask;
};

struct amdgv_vmhub {
	uint32_t fault_cntl;
	uint32_t fault_status;
	uint32_t fault_addr_lo;
	uint32_t fault_addr_hi;
};



#define AMDGV_WS_MODE_UNDEFINED 0
#define AMDGV_WS_MODE_MANUAL	1
#define AMDGV_WS_MODE_AUTO	2


struct amdgv_ip_map_info {
	/* Map of logical to actual dev instances */
	uint32_t		dev_inst[MAX_HWIP][HWIP_MAX_INSTANCE];
	int8_t (*logical_to_dev_inst)(struct amdgv_adapter *adapt,
					  enum amdgv_hw_ip_block_type block,
					  int8_t inst);
	uint32_t (*logical_to_dev_mask)(struct amdgv_adapter *adev,
					enum amdgv_hw_ip_block_type block,
					uint32_t mask);
};

enum amdgv_mes_pipe {
	AMDGV_MES_SCHED_PIPE = 0,
	AMDGV_MES_KIQ_PIPE,
	AMDGV_MAX_MES_PIPES = 2,
};

enum emu_type {
	NOT_EMU,
	GFX_EMU,
	FULL_EMU,
};

enum amdgv_vram_type {
  AMDGV_DGPU_VRAM_TYPE__GDDR5 = 0x50,
  AMDGV_DGPU_VRAM_TYPE__HBM2 = 0x60,
  AMDGV_DGPU_VRAM_TYPE__HBM2E = 0x61,
  AMDGV_DGPU_VRAM_TYPE__GDDR6 = 0x70,
  AMDGV_DGPU_VRAM_TYPE__HBM3 = 0x80,
  AMDGV_DGPU_VRAM_TYPE__GDDR7 = 0x90,
};

enum amdgv_vram_vendor {
  AMDGV_VRAM_VENDOR__PLACEHOLDER0,  // 0
  AMDGV_VRAM_VENDOR__SAMSUNG,       // 1
  AMDGV_VRAM_VENDOR__INFINEON,      // 2
  AMDGV_VRAM_VENDOR__ELPIDA,        // 3
  AMDGV_VRAM_VENDOR__ETRON,         // 4
  AMDGV_VRAM_VENDOR__NANYA,         // 5
  AMDGV_VRAM_VENDOR__HYNIX,         // 6
  AMDGV_VRAM_VENDOR__MOSEL,         // 7
  AMDGV_VRAM_VENDOR__WINBOND,       // 8
  AMDGV_VRAM_VENDOR__ESMT,          // 9
  AMDGV_VRAM_VENDOR__PLACEHOLDER1,  // 10
  AMDGV_VRAM_VENDOR__PLACEHOLDER2,  // 11
  AMDGV_VRAM_VENDOR__PLACEHOLDER3,  // 12
  AMDGV_VRAM_VENDOR__PLACEHOLDER4,  // 13
  AMDGV_VRAM_VENDOR__PLACEHOLDER5,  // 14
  AMDGV_VRAM_VENDOR__MICRON,        // 15
};

struct amdgv_vram_info {
	enum amdgv_vram_type vram_type;
	enum amdgv_vram_vendor vram_vendor;
	uint32_t vram_size_mb;
	uint32_t vram_bit_width;
};

struct amdgv_vf_fb_block {
	bool used;	// fb_block list is pre-allocated. Use this flag to indicate whether current fb_block is used or not
	bool allocated;	// Use this flag to indicate whether fb_block is allocated to VF or is assignable
	uint32_t idx_vf; // the vf occupied this section fb
	uint32_t fb_offset_tlb; // the fb offset in TLB. Different from fb_offset_tmr in FFBM which only contains 1TMR size
	uint32_t fb_size;	// fb size without TMR size
	uint32_t fb_size_tlb; //the occupied fb_size in TLB. Different from fb_size_tmr in FFBM
    struct amdgv_list_head vf_fb_block_node; //List of allocated fb blocks
};

struct amdgv_adapter {
	oss_dev_t         dev;
	uint32_t          domain; // not used
	uint32_t          bdf;

	uint32_t          asic_type;

	uint32_t          vendor_id;
	uint32_t          dev_id;
	uint32_t          rev_id;

	uint32_t          sub_sys_id;
	uint32_t          sub_vnd_id;

	bool              fake_fb;
	void              *fb;
	void              *mmio;
	void              *io_mem;
	void              *doorbell;
	void              *rom;

	/* register offset based on ip, instance and segment */
	uint32_t          *reg_offset[MAX_HWIP][HWIP_MAX_INSTANCE];

	/* physical address of framebuffer */
	uint64_t          fb_pa;
	/* the size of framebuffer */
	uint64_t          fb_size;

	uint64_t          mmio_pa;
	uint64_t          mmio_size;

	uint64_t          io_mem_pa;
	uint64_t          io_mem_size;

	uint64_t          doorbell_pa;
	uint64_t          doorbell_size;

	unsigned long     rom_size;

	struct amdgv_ucodes *ucodes;

	/* upload fw info */
	void              *mecfw_image;
	uint64_t          mecfw_offset;
	uint32_t          mecfw_size;
	void              *uvdfw_image;
	uint64_t          uvdfw_offset;
	uint32_t          uvdfw_size;
	void              *vcefw_image;
	uint64_t          vcefw_offset;
	uint32_t          vcefw_size;

	uint32_t          tmr_size;

	/* ASIC serial number */
	uint64_t          serial;

	/* VBIOS info cache */
	struct amdgv_vbios_info  vbios_cache;

	/* product info */
	struct amdgv_product_info product_info;
	uint8_t *i2c_cmd_buffer;
	uint8_t *checksum_buffer;

	/* PCI SRIOV configuration */
	uint16_t          sriov_cap_pos;
	uint16_t          sriov_vf_offset;
	uint16_t          sriov_vf_stride;
	uint16_t          sriov_vf_devid;

	uint64_t          mc_fb_loc_addr;
	uint64_t          mc_fb_top_addr;
	uint64_t          mc_sys_loc_addr;
	uint64_t          mc_sys_top_addr;
	uint64_t          mc_agp_loc_addr;
	uint64_t          mc_agp_top_addr;

	/* SYSMEM aperture */
	struct oss_dma_mem_info sys_mem_info;

	struct amdgv_init_config_opt opt;

	struct amdgv_fini_config_opt fini_opt;

	uint64_t flags;

	uint8_t bp_mode;
	uint8_t bp_go_flag;
	uint8_t bp_gfx_ws_pause_flag;
	bool is_user_ws_cmd;
	struct amdgv_bp_info bp_info;

	uint32_t sim_mode;
	uint32_t fw_load_engine;
	enum emu_type emu_type;
	bool has_vbios;
	bool rs64_enable;

	/* Indicates adaper in live-updating mode */
	bool in_live_update;

	uint32_t force_switch_vf_idx;

	/* ASIC capability */
	unsigned int      caps;
	uint64_t          max_mm_bandwidth[AMDGV_MAX_MM_ENGINE];

	/* sysfs/smi_lib configured option for HW auto-scheduling quanta */
	uint32_t time_quanta_option[AMDGV_SCHED_BLOCK_MAX];

	uint32_t log_level;
	uint32_t log_mask;

	spin_lock_t mmio_idx_lock;
	spin_lock_t pcie_idx_lock;
	spin_lock_t smu_msg_lock;
	mutex_t api_lock;
	mutex_t hive_lock;
	mutex_t gpumon_hive_lock;
	mutex_t smu_i2c_lock;
	mutex_t psp_lock;
	mutex_t set_vf_access_lock;
	mutex_t bp_lock;

	bool customized_vf_config_mode;
	struct amd_sriov_msg_pf2vf_info *pf2vf_msg;

	unsigned int num_vf;
	/* max # of VF supported by asic */
	unsigned int max_num_vf;
	/* if vf resizable bar is enabled */
	bool vf_rebar_en;

	unsigned int ip_discovery_load_type;
	unsigned int fw_load_type;
	unsigned int mm_policy;

	struct amdgv_vf_device array_vf[AMDGV_MAX_VF_SLOT];

	struct amdgv_init_func **init_funcs;
	int                    num_funcs;

	struct amdgv_live_info_func **live_info_funcs;

	struct amdgv_reg_range **miti_table;
	int                    num_miti;

	void (*reg_base_init)(struct amdgv_adapter *adapt);

	struct amdgv_irqmgr irqmgr;

	struct amdgv_mailbox mailbox;

	struct amdgv_gpuiov gpuiov;

	struct amdgv_reset reset;

	struct amdgv_sched sched;

	struct amdgv_vbios vbios;

	struct amdgv_ffbm ffbm;

	struct amdgv_ip_discovery ip_discovery;

	struct psp_context psp;
	struct amdgv_ucode ucode;

	struct amdgv_pp pp;
	struct amdgv_gpumon gpumon;
	struct amdgv_xgmi xgmi;
	struct amdgv_misc misc;
	struct amdgv_ecc ecc;
	struct amdgv_umc umc;
	struct amdgv_ras_eeprom_control eeprom_control;
	enum amdgv_bp_msg_type bp_msg_type;
	bool continue_init_other_block_hw;
	bool update_channel_flag;
	struct amdgv_ras_eeprom ras_eeprom;
	struct amdgv_gfx gfx;
	struct amdgv_sdma sdma;
	struct amdgv_vcn vcn;
	struct amdgv_nbio nbio;
	struct amdgv_df df;

	struct amdgv_ip_map_info ip_map;
	struct amdgv_mcp mcp;
	struct amdgv_vram_info vram_info;

	struct amdgv_debug debug;

	struct amdgv_mca  mca;
	struct amdgv_cper cper;

	uint32_t ip_versions[MAX_HWIP][HWIP_MAX_INSTANCE];

	struct amdgv_ring   aql_compute_ring;

	/* Memory manager for PF memory */
	struct amdgv_memmgr memmgr_pf;
	/* Memory manager for whole GPU memory */
	struct amdgv_memmgr memmgr_gpu;
	/* Memory manager for SYSMEM memory */
	struct amdgv_memmgr memmgr_sys;

	/* asic config */
	struct amdgv_config config;

	/* device status */
	enum amdgv_dev_status status;

	/* Error logging */
	struct amdgv_error_ring_buffer *error_ring_buffer;
	thread_t error_process_thread;
	event_t new_error_event;
	struct amdgv_error_notifier notifier_list;
	mutex_t notifier_list_lock;
	uint32_t error_dump_stack_max;
	uint32_t error_dump_stack_count;
	uint32_t error_dump_stack_filter_list[AMDGV_ERROR_FILTER_LIST_SIZE_MAX];

	// PSP mailbox error record
	uint32_t psp_mb_error_record_write_idx;
	struct amdgv_psp_mb_err_record error_record[AMDGV_MAX_PSP_MB_ERROR_RECORD];

	struct amdgv_cg cg;

	bool lock_world_switch;

	enum amdgv_live_update_state live_update_state;
	enum amdgv_sched_event_thread_status event_thread_status;

	/* the adapter array for all the gpus */
	struct amdgv_adapter **adapt_list;

	struct amdgv_vmhub vmhub[VM_NUMHUB]; /* extend it if new asic requires, rightnbow 3 is enough */

	char dump_buf[MAX_DUMP_LENGTH];
#ifdef WS_RECORD
	char *record_buf;
	char *auto_ws_record_buf;
#endif
	struct amdgv_diag_data diag_data;
	struct amdgv_live_migration live_migration;
	struct amdgv_dirtybit dirtybit;
	bool sriov_restore;

	/* used for live update to store and compare the hash */
	uint64_t hash_addr;
	bool in_chain_live_update;

	struct amdgv_mmsch mmsch;
	struct amdgv_smuio smuio;
	struct amdgv_mmhub mmhub;

	/* PCIE atomic ops support flag */
	uint32_t pcie_atomic_ops_support_flags;

	/* SCPM */
	bool use_soft_pptable;
	bool scpm_enabled;

	/* firmware flags */
	uint32_t firmware_flags;
	struct amdgv_doorbell_index doorbell_index;

	struct amdgv_wb wb;

	/* rings */
	unsigned int num_rings;
	uint64_t fence_context;
	struct amdgv_ring *rings[AMDGV_MAX_RINGS];

	mutex_t srbm_mutex;
	mutex_t grbm_idx_mutex;

	struct amdgv_memmgr_mem *mem_pfp_fw;
	struct amdgv_memmgr_mem *mem_me_fw;
	struct amdgv_memmgr_mem *mem_mec1_fw;
	struct amdgv_memmgr_mem *mem_rs64_pfp_ucode;
	struct amdgv_memmgr_mem *mem_rs64_pfp_data[2];
	struct amdgv_memmgr_mem *mem_rs64_me_ucode;
	struct amdgv_memmgr_mem *mem_rs64_me_data[2];
	struct amdgv_memmgr_mem *mem_rs64_mec1_ucode;
	struct amdgv_memmgr_mem *mem_rs64_mec1_data[4];
	struct amdgv_memmgr_mem *mem_mes_p0_data_fw;
	struct amdgv_memmgr_mem *mem_mes_p0_ucode_fw;
	struct amdgv_memmgr_mem *mem_mes_p1_data_fw;
	struct amdgv_memmgr_mem *mem_mes_p1_ucode_fw;
	uint64_t mes_uc_start_addr[AMDGV_MAX_MES_PIPES];

	bool rlcv_stamp_todo;
	bool rlcv_stamp_status;
	uint64_t rlcv_stamp_count;
	uint32_t rlcv_ts_buff[CSA_TSL_SIZE / 4];

	/* GART */
	struct oss_dma_mem_info pdb0_mem;
	struct oss_dma_mem_info ptb_mem;
	uint64_t gart_size;

	/* Asymmetric FB layout */
	bool asymmetric_fb_enabled;
	struct amdgv_vf_fb_block vf_fb_block[AMDGV_MAX_FB_BLOCK_NUM];
	struct amdgv_list_head vf_fb_block_list; // ordered by fb_offset

	struct {
		struct amdgv_mem_with_bitmap mem_id_list[MEM_ID_COUNT_MAX];
		uint8_t valid_mem_id_count;
	} mems;

	struct amdgv_perf_log_info perf_log;

	/*
	 * RAS reset status flags
	 *
	 * The 2 pointer in_sync_flood and in_ecc_recovery use adapt local atomic
	 * adapt->sync_flood and adapt->ecc_recovery when not in a hive.
	 *
	 * They point to hive wide atomics, hive->sync_flood and hive->ecc_recovery
	 * after the adapt is added to a hive.
	 */
	atomic_t *in_sync_flood;
	/* pointer to GPU recovery flag, be set due to RAS */
	atomic_t *in_ecc_recovery;
	atomic_t sync_flood;
	atomic_t ecc_recovery;

	/* psp mb int status, for xgmi.set_mb_in_hive feature to sync hive status */
	bool psp_mb_int_status;
};

#ifdef WS_RECORD
enum amdgv_record_status {
	AMDGV_RECORD_INIT_START = 1,
	AMDGV_RECORD_INIT_END,
	AMDGV_RECORD_LOAD_START,
	AMDGV_RECORD_LOAD_END,
	AMDGV_RECORD_IDLE_START,
	AMDGV_RECORD_IDLE_END,
	AMDGV_RECORD_SAVE_START,
	AMDGV_RECORD_SAVE_END,
	AMDGV_RECORD_RUN_START,
	AMDGV_RECORD_RUN_END,
	AMDGV_RECORD_SHUTDOWN_START,
	AMDGV_RECORD_SHUTDOWN_END,
	AMDGV_RECORD_LOAD_RLCV_STATE_START,
	AMDGV_RECORD_LOAD_RLCV_STATE_END,
	AMDGV_RECORD_SAVE_RLCV_STATE_START,
	AMDGV_RECORD_SAVE_RLCV_STATE_END,
	AMDGV_RECORD_ENABLE_MMSCH_VFGATE_START,
	AMDGV_RECORD_ENABLE_MMSCH_VFGATE_END,
	AMDGV_RECORD_DISABLE_MMSCH_VFGATE_START,
	AMDGV_RECORD_DISABLE_MMSCH_VFGATE_END,
	AMDGV_RECORD_ENABLE_AUTO_SCHED_START,
	AMDGV_RECORD_ENABLE_AUTO_SCHED_END,
	AMDGV_RECORD_DISABLE_AUTO_SCHED_START,
	AMDGV_RECORD_DISABLE_AUTO_SCHED_END,
	AMDGV_RECORD_EVENT_NOTIFICATION_START,
	AMDGV_RECORD_EVENT_NOTIFICATION_END,

	AMDGV_RECORD_EXPORT_VF_DATA_START,
	AMDGV_RECORD_EXPORT_VF_DATA_END,
	AMDGV_RECORD_IMPORT_VF_DATA_START,
	AMDGV_RECORD_IMPORT_VF_DATA_END,

	AMDGV_RECORD_MAX,
};

enum amdgv_auto_ws_record_status {
	AMDGV_AUTO_WS_IDLE_START = 1,
	AMDGV_AUTO_WS_SAVE_START,
	AMDGV_AUTO_WS_LOAD_START,
	AMDGV_AUTO_WS_RUN_START,
	AMDGV_AUTO_WS_RUN_END,
	AMDGV_AUTO_WS_MAX,
};

struct amdgv_record_name {
	uint32_t    id;
	const char *name;
};

struct amdgv_record_entity {
	uint64_t time_stamp;
	uint32_t idx_vf;
	uint32_t hw_sched_id;
	enum amdgv_record_status status;
};

void amdgv_gpuiov_record_queue_push(struct amdgv_adapter *adapt, uint32_t idx_vf,
						uint32_t hw_sched_id,
						enum amdgv_record_status status);
#endif


/* --------------- WAIT -----------------*/

/* wait flag */
#define AMDGV_WAIT_FLAG_AUTO            0            /* auto adjust delay and sleep in the wait */
#define AMDGV_WAIT_FLAG_FORCE_DELAY     (1 << 0)     /* forcibly use delay in the wait */
#define AMDGV_WAIT_FLAG_FORCE_YIELD     (1 << 1)     /* force append an yield after delay, prevent some register to be read to die */
#define AMDGV_WAIT_FLAG_USLEEP          (1 << 2)     /* use udelay for the 1st loop (200us) but switch to usleep for the rest loops to release CPU */
#define AMDGV_WAIT_FLAG_NO_WARNING      (1 << 3)     /* disable warning message when timeout, used when timeout is possible but not an exception */

/* return value */
#define AMDGV_WAIT_RET_TIMED_OUT        -1
#define AMDGV_WAIT_RET_INVALID          -2

#define AMDGV_WAIT_CYCLE                (100)
#define AMDGV_WAIT_MAX_DELAY_US         (50)                    /* max delay interval */
#define AMDGV_WAIT_MIN_SLEEP_US         (50)                    /* min sleep interval */

/* phase 1 property */
#define AMDGV_WAIT_PHASE1_THRESHOLD_US              (15000)                              /* when phase 1 ends */

#define AMDGV_WAIT_PHASE1_LOOP_TIMEOUT(interval)    ((interval) * AMDGV_WAIT_CYCLE)      /* loop timeout in phase 1 */

/* phase 2 property */
#define AMDGV_WAIT_PHASE2_MSLEEP_THRESHOLD_US       (5000)           /* if interval bigger than this value, use msleep */

/* mode for common wait */
#define AMDGV_WAIT_CHECK_EQ				0				/* check for equal */
#define AMDGV_WAIT_CHECK_NE				1				/* check for not-equal */

struct amdgv_wait_for_register_context {
	struct amdgv_adapter *adapt;
	uint32_t offset;
	uint32_t mask;
	uint32_t value;
	uint32_t check_flag;
};

struct amdgv_wait_for_memory_context {
	volatile uint32_t *address;
	uint32_t value;
};

struct amdgv_wait_for_pci_cfg_context {
	struct amdgv_adapter *adapt;
	oss_dev_t dev;
	uint32_t offset;
	uint32_t mask;
	uint32_t value;
	uint8_t byte_len;
	uint32_t check_flag;
};


typedef uint64_t (*time_stamp_src_t)(void);

/* call back funtion in amdgv_wait_for, return 0 if status match, otherwise return non-zero */
typedef int (*amdgv_wait_cb_t)(void *cb_context);

int amdgv_wait_for(struct amdgv_adapter *adapt, amdgv_wait_cb_t cb_func, void *cb_context, uint64_t timeout_us, uint32_t wait_flag);
int amdgv_wait_for_irq_handler(void *context);
int amdgv_wait_for_register(struct amdgv_adapter *adapt, uint32_t offset, uint32_t mask, uint32_t value, uint64_t timeout_us, uint32_t check_flag, uint32_t wait_flag);
int amdgv_wait_for_memory(struct amdgv_adapter *adapt, uint32_t *addr, uint32_t value, uint64_t timeout_us);
int amdgv_wait_for_pci_cfg(struct amdgv_adapter *adapt, oss_dev_t dev, uint32_t offset, uint32_t mask, uint32_t value, uint8_t byte_len, uint64_t timeout_us, uint32_t check_flag, uint32_t wait_flag);

/* --------------- WAIT END --------------*/


uint32_t amdgv_mm_rreg(struct amdgv_adapter *adapt, uint32_t reg, bool always_indirect);
void amdgv_mm_wreg(struct amdgv_adapter *adapt, uint32_t reg, uint32_t val,
		   bool always_indirect);

uint8_t amdgv_smn_rreg8(struct amdgv_adapter *adapt, uint32_t reg);
void amdgv_smn_wreg8(struct amdgv_adapter *adapt, uint32_t reg, uint8_t val);

uint16_t amdgv_smn_rreg16(struct amdgv_adapter *adapt, uint32_t reg);
void amdgv_smn_wreg16(struct amdgv_adapter *adapt, uint32_t reg, uint16_t val);

uint32_t amdgv_smn_rreg32(struct amdgv_adapter *adapt, uint32_t reg);
void amdgv_smn_wreg32(struct amdgv_adapter *adapt, uint32_t reg, uint32_t val);

uint32_t amdgv_pcie_rreg(struct amdgv_adapter *adapt, uint32_t reg);
void amdgv_pcie_wreg(struct amdgv_adapter *adapt, uint32_t reg, uint32_t val);

uint32_t amdgv_pcie_rreg_ext(struct amdgv_adapter *adapt, uint64_t reg);
void amdgv_pcie_wreg_ext(struct amdgv_adapter *adapt, uint64_t reg, uint32_t val);

uint64_t amdgv_pcie_rreg64(struct amdgv_adapter *adapt, uint32_t reg);
void amdgv_pcie_wreg64(struct amdgv_adapter *adapt, uint32_t reg, uint64_t val);

uint64_t amdgv_pcie_rreg64_ext(struct amdgv_adapter *adapt, uint64_t reg);
void amdgv_pcie_wreg64_ext(struct amdgv_adapter *adapt, uint64_t reg, uint64_t val);

uint32_t amdgv_io_rreg(struct amdgv_adapter *adapt, uint32_t reg);
void amdgv_io_wreg(struct amdgv_adapter *adapt, uint32_t reg, uint32_t val);

uint32_t amdgv_mm_read_fb(struct amdgv_adapter *adapt, uint64_t addr);
void amdgv_mm_write_fb(struct amdgv_adapter *adapt, uint64_t addr, uint32_t val);

uint32_t amdgv_mm_rdoorbell(struct amdgv_adapter *adapt, uint32_t index);
void amdgv_mm_wdoorbell(struct amdgv_adapter *adapt, uint32_t index, uint32_t val);
void amdgv_mm_wdoorbell64(struct amdgv_adapter *adapt, uint32_t index, uint64_t val);

void amdgv_mm_copy_to_fb(struct amdgv_adapter *adapt, uint64_t dst, uint64_t src,
			 uint64_t size);
void amdgv_mm_copy_from_fb(struct amdgv_adapter *adapt, uint64_t dst, uint64_t src,
			   uint64_t size);

int amdgv_acquire_fb_virtual_addr(struct amdgv_adapter *adapt,
						uint64_t gpu_vir, uint64_t size, uint64_t *cpu_vir);
int amdgv_release_fb_virtual_addr(struct amdgv_adapter *adapt, void *addr,
						uint64_t offset);

struct amdgv_dump_reg {
	int index;
	uint32_t hwid;
	uint32_t inst;
	uint32_t seg;
	uint32_t offset;
	char *name;
};

/* GET_INST returns the physical instance corresponding to a logical instance */
#define GET_INST(ip, inst) (adapt->ip_map.logical_to_dev_inst ? adapt->ip_map.logical_to_dev_inst(adapt, ip##_HWIP, inst) : inst)
#define GET_MASK(ip, mask) (adapt->ip_map.logical_to_dev_mask ? adapt->ip_map.logical_to_dev_mask(adapt, ip##_HWIP, mask) : mask)

/* register ops */
#define SOC15_REG_OFFSET(ip, inst, reg)                                                       \
	(adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg)

#define SOC15_REG_ENTRY(ip, inst, reg) ip##_HWIP, inst, reg##_BASE_IDX, reg

#define SOC15_REG_ENTRY_OFFSET(entry)	(adapt->reg_offset[entry.hwip][entry.inst][entry.seg] + entry.reg_offset)

#define SOC15_REG_FIELD(reg, field) reg##__##field##_MASK, reg##__##field##__SHIFT

#define SOC15_REG_FIELD_VAL(val, mask, shift)	(((val) & mask) >> shift)

#define SOC15_RAS_REG_FIELD_VAL(val, entry, field) SOC15_REG_FIELD_VAL((val), (entry).field##_count_mask, (entry).field##_count_shift)

#define SOC15_REG_GOLDEN_VALUE(ip, inst, reg, and_mask, or_mask)                              \
	{                                                                                     \
		ip##_HWIP, inst, reg##_BASE_IDX, reg, and_mask, or_mask                       \
	}

#define RREG32(reg)    amdgv_mm_rreg(adapt, (reg), false)
#define WREG32(reg, v) amdgv_mm_wreg(adapt, (reg), (v), false)

#define RREG8_SMN(reg) amdgv_smn_rreg8(adapt, (reg))
#define WREG8_SMN(reg, v) amdgv_smn_wreg8(adapt, (reg), (v))

#define RREG16_SMN(reg) amdgv_smn_rreg16(adapt, (reg))
#define WREG16_SMN(reg, v) amdgv_smn_wreg16(adapt, (reg), (v))

#define RREG32_SMN(reg) amdgv_smn_rreg32(adapt, (reg))
#define WREG32_SMN(reg, v) amdgv_smn_wreg32(adapt, (reg), (v))

#define RREG32_PCIE(reg)    amdgv_pcie_rreg(adapt, (reg))
#define WREG32_PCIE(reg, v) amdgv_pcie_wreg(adapt, (reg), (v))

#define RREG32_PCIE_EXT(reg)    amdgv_pcie_rreg_ext(adapt, (reg))
#define WREG32_PCIE_EXT(reg, v) amdgv_pcie_wreg_ext(adapt, (reg), (v))

#define RREG64_PCIE(reg)    amdgv_pcie_rreg64(adapt, (reg))
#define WREG64_PCIE(reg, v) amdgv_pcie_wreg64(adapt, (reg), (v))

#define RREG64_PCIE_EXT(reg)    amdgv_pcie_rreg64_ext(adapt, (reg))
#define WREG64_PCIE_EXT(reg, v) amdgv_pcie_wreg64_ext(adapt, (reg), (v))

#define RREG32_IO(reg)	  amdgv_io_rreg(adapt, (reg))
#define WREG32_IO(reg, v) amdgv_io_wreg(adapt, (reg), (v))

#define READ_FB32(addr)	    amdgv_mm_read_fb(adapt, addr)
#define WRITE_FB32(addr, v) amdgv_mm_write_fb(adapt, addr, v)

#define RDOORBELL32(index)    amdgv_mm_rdoorbell(adapt, (index))
#define WDOORBELL32(index, v) amdgv_mm_wdoorbell(adapt, (index), (v))
#define WDOORBELL64(index, v) amdgv_mm_wdoorbell64(adapt, (index), (v))

#define REG_FIELD_SHIFT(reg, field) reg##__##field##__SHIFT
#define REG_FIELD_MASK(reg, field)  reg##__##field##_MASK

#define REG_SET_FIELD(orig_val, reg, field, field_val)                                        \
	(((orig_val) & ~REG_FIELD_MASK(reg, field)) |                                         \
	 (REG_FIELD_MASK(reg, field) & ((field_val) << REG_FIELD_SHIFT(reg, field))))

#define REG_GET_FIELD(value, reg, field)                                                      \
	(((value)&REG_FIELD_MASK(reg, field)) >> REG_FIELD_SHIFT(reg, field))

#define WREG32_FIELD(reg, field, val)                                                         \
	WREG32(mm##reg, (RREG32(mm##reg) & ~REG_FIELD_MASK(reg, field)) |                     \
				(val) << REG_FIELD_SHIFT(reg, field))

#define WREG32_FIELD_OFFSET(reg, offset, field, val)                                          \
	WREG32(mm##reg + offset, (RREG32(mm##reg + offset) & ~REG_FIELD_MASK(reg, field)) |   \
					 (val) << REG_FIELD_SHIFT(reg, field))

#define RREG32_SOC15_OFFSET(ip, inst, reg, offset)                                            \
	RREG32((adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg) + offset)

#define WREG32_SOC15_OFFSET(ip, inst, reg, offset, value)                                     \
	WREG32((adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg) + offset, value)

#define RREG32_SOC15(ip, inst, reg)                                            \
	RREG32(adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg)

#define WREG32_SOC15(ip, inst, reg, value)                                     \
	WREG32(adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg, value)

#define WREG32_RMW(reg, and_mask, or_mask)                                                    \
	do {                                                                                  \
		if (and_mask == 0xffffffff || and_mask == 0) {                                \
			WREG32(reg, or_mask);                                                 \
		} else {                                                                      \
			tmp = RREG32(reg);                                                    \
			tmp &= ~(and_mask);                                                   \
			tmp |= (or_mask & and_mask);                                          \
			WREG32(reg, tmp);                                                     \
		}                                                                             \
	} while (0)

#define WREG32_FIELD15(ip, idx, reg, field, val)	\
		WREG32(adapt->reg_offset[ip##_HWIP][idx][mm##reg##_BASE_IDX] + \
			mm##reg, (RREG32( \
			adapt->reg_offset[ip##_HWIP][idx][mm##reg##_BASE_IDX] + \
			mm##reg) & ~REG_FIELD_MASK(reg, field)) | \
			(val) << REG_FIELD_SHIFT(reg, field))

#define WREG32_FIELD15_RLC(ip, idx, reg, field, val)   \
		WREG32((adapt->reg_offset[ip##_HWIP][idx][mm##reg##_BASE_IDX] + \
			mm##reg), (RREG32( \
			adapt->reg_offset[ip##_HWIP][idx][mm##reg##_BASE_IDX] + \
			mm##reg) & ~REG_FIELD_MASK(reg, field)) | \
			(val) << REG_FIELD_SHIFT(reg, field))

/* shadow the registers in the callback function */
#define WREG32_SOC15_RLC_SHADOW(ip, inst, reg, value) \
	WREG32((adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg), \
		value)

#define WREG32_SOC15_RLC_SHADOW_EX(prefix, ip, inst, reg, value) \
	do {							\
		uint32_t target_reg = adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg;\
		uint32_t r2 = adapt->reg_offset[GC_HWIP][inst][prefix##SCRATCH_REG1_BASE_IDX] + prefix##SCRATCH_REG2;	\
		uint32_t r3 = adapt->reg_offset[GC_HWIP][inst][prefix##SCRATCH_REG1_BASE_IDX] + prefix##SCRATCH_REG3;	\
		uint32_t grbm_cntl = adapt->reg_offset[GC_HWIP][inst][prefix##GRBM_GFX_CNTL_BASE_IDX] + prefix##GRBM_GFX_CNTL;   \
		uint32_t grbm_idx = adapt->reg_offset[GC_HWIP][inst][prefix##GRBM_GFX_INDEX_BASE_IDX] + prefix##GRBM_GFX_INDEX;   \
		if (target_reg == grbm_cntl) \
			WREG32(r2, value);	\
		else if (target_reg == grbm_idx) \
			WREG32(r3, value);	\
		WREG32(target_reg, value);	\
	} while (0)

#define RREG32_SOC15_RLC(ip, inst, reg) \
		RREG32(adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg)

#define WREG32_SOC15_RLC(ip, inst, reg, value) \
	do { \
		uint32_t target_reg = \
		adapt->reg_offset[ip##_HWIP][inst][reg##_BASE_IDX] + reg; \
		WREG32(target_reg, value); \
	} while (0)

uint32_t amdgv_internal_get_asic_type(uint32_t device_id, uint32_t revision_id);
struct amdgv_adapter *amdgv_device_internal_init(struct amdgv_init_data *init_data);
void amdgv_device_internal_fini(struct amdgv_adapter *adapt,
				struct amdgv_fini_config_opt *fini_opt);
int amdgv_device_func_hw_engine_init(struct amdgv_adapter *adapt);
void amdgv_device_func_hw_engine_fini(struct amdgv_adapter *adapt);
int amdgv_device_func_hw_live_init(struct amdgv_adapter *adapt);

int amdgv_get_supported_dev_ids(struct amdgv_device_ids *dev_ids, int length);

void amdgv_program_register_sequence(struct amdgv_adapter *adapt,
					 const struct amdgv_reg_golden *regs,
					 const uint32_t array_size);
int amdgv_hw_sched_init(struct amdgv_adapter *adapt);
int amdgv_sched_shutdown_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_print_failed_init_name(struct amdgv_adapter *adapt, bool is_sw, const char *func_name);
struct amdgv_hive_info *amdgv_get_xgmi_hive(struct amdgv_adapter *adapt);
void amdgv_time_log_increase_vf_reset_cnt(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_time_log_clear_vf(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_time_log_note_vf_init_start(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_time_log_note_vf_init_end(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_time_log_note_vf_fini_start(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_time_log_note_vf_fini_end(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_time_log_update_vf_cumulative_running_time(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_time_log_update_vf_cumulative_running_time_after_flr(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_time_log_note_vf_reset_start(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_time_log_note_vf_reset_end(struct amdgv_adapter *adapt, uint32_t idx_vf);
void amdgv_sched_ctx_empty_intr_control(struct amdgv_adapter *adapt, uint32_t hw_sched_id, bool enable);
int amdgv_device_generate_rma_cper(struct amdgv_adapter *adapt);
void amdgv_device_report_rma_to_fw(struct amdgv_adapter *adapt);
void amdgv_device_handle_bad_gpu(struct amdgv_adapter *adapt);
void amdgv_device_handle_bad_hive(struct amdgv_adapter *adapt);
bool amdgv_device_is_gpu_lost(struct amdgv_adapter *adapt);
void amdgv_device_set_status(struct amdgv_adapter *adapt, enum amdgv_dev_status status);
#endif
