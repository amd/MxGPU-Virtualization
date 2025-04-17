/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_DIAG_DATA_GEN_INFO_H
#define AMDGV_DIAG_DATA_GEN_INFO_H

#define AMDGV_RD_HOST_GEN_INFORMATION_MAJOR_VERSION	0x01
#define AMDGV_RD_HOST_GEN_INFORMATION_MINOR_VERSION	0x03
#define AMDGV_RD_HOST_GEN_INFORMATION_VERSION \
		(AMDGV_RD_HOST_GEN_INFORMATION_MINOR_VERSION | \
		AMDGV_RD_HOST_GEN_INFORMATION_MAJOR_VERSION << 8)

enum spatial_partition_mode;

/* NOTE: Change the AMDGV_RD_HOST_GEN_INFORMATION_MINOR_VERSION or
 *  AMDGV_RD_HOST_GEN_INFORMATION_MINOR_VERSION if there is any modification
 *  made to the structure amdgv_diag_data_gen_info. Not doing so will
 *  may cause inability to parse the data.
 */

/* <GEN JSON START> */
/* <name:General Information; blk:GEN;> */
struct amdgv_diag_data_gen_info {
	/* <name:Driver Information; blk:GEN.DRV;> */
	struct {
		struct {
			/* <name:Major Version; type:INT; size:4; base:16;> */
			uint32_t major;
			/* <name:Minor Version; type:INT; size:4; base:16;> */
			uint32_t minor;
		} version;
		struct {
			/* <name:Supported Field Flags; type:BITMASK; size:4; base:16;> */
			/* <name:AMDGV_XGMI_FLAG; size:1; pos:0;> */
			/* <name:AMDGV_MM_METRICS_FLAG; size:1; pos:1;> */
			/* <name:AMDGV_POWER_GFX_VOLTAGE_FLAG; size:1; pos:2;> */
			/* <name:AMDGV_POWER_DPM_FLAG; size:1; pos:3;> */
			/* <name:AMDGV_MEM_USAGE_FLAG; size:1; pos:4;> */
			/* <name:AMDGV_TARGET_FREQUENCY_MODE_FLAG; size:1; pos:5;> */
			/* <name:AMDGV_VBIOS_HASH_FLAG; size:1; pos:6;> */
			/* <name:AMDGV_MM2_CLOCK_FLAG; size:1; pos:7;> */
			uint32_t supp_field_flags;
			/* <name:Event Guards Enable; type:INT; size:4; base:10;> */
			uint32_t guard_enable;
			/* <name:Skip Bad GPU Check; type:INT; size:4; base:10;> */
			uint32_t skip_check_bad_gpu;
			/* <name:FW Load Type; type:INT; size:4; base:10;> */
			uint32_t fw_load_type;
			/* <name:Allow Full Access Time; type:INT; size:4; base:10;> */
			uint32_t allow_time_full_access;
			/* <name:Command Complete Time; type:INT; size:4; base:10;> */
			uint32_t allow_time_cmd_complete;
			/* <name:Performance Enable; type:INT; size:4; base:10;> */
			uint32_t perf_mon_enable;
			/* <name:Scheduling Mode; type:INT; size:4; base:16;> */
			uint32_t sched_mode;
			/* <name:Log Level; type:INT; size:4; base:10;> */
			int32_t log_level;
			/* <name:Log Mask; type:INT; size:4; base:16;> */
			uint32_t log_mask;
		} config_info;
	} driver_info; /* <BLK END> */

	/* <name:GPU Information; blk:GEN.GPU;> */
	struct {
		/* <name:Chip Information; blk:GEN.GPU.CHIP;> */
		struct {
			/* <name:BDF; type:INT; size:4; base:16;> */
			uint32_t bdf;
			/* <name:Domain; type:INT; size:4; base:16;> */
			uint32_t domain;
			/* <name:Vendor ID; type:INT; size:4; base:16;> */
			uint32_t vendor_id;
			/* <name:Device ID; type:INT; size:4; base:16;> */
			uint32_t dev_id;
			/* <name:Revision ID; type:INT; size:4; base:16;> */
			uint32_t rev_id;
			/* <name:SubSystem ID; type:INT; size:4; base:16;> */
			uint32_t sub_sys_id;
			/* <name:SubVendor ID; type:INT; size:4; base:16;> */
			uint32_t sub_vnd_id;
			/* <name:Model Number; type:STRING; size:32;> */
			uint8_t model_number[STRLEN_NORMAL];
			/* <name:Product Serial; type:STRING; size:32;> */
			uint8_t product_serial[STRLEN_NORMAL];
			/* <name:FRU ID; type:STRING; size:32;> */
			uint8_t fru_id[STRLEN_NORMAL];
			/* <name:Product Name; type:STRING; size:32;> */
			uint8_t product_name[STRLEN_NORMAL];
			/* <name:Manufacturer Name; type:STRING; size:32;> */
			uint8_t manufacturer_name[STRLEN_NORMAL];
			/* <name:VF Number; type:INT; size:4; base:10;> */
			uint32_t vf_number;
			/* <IDX MAP> */
			/* <name:SPM IDX> */
			/* <0:SPATIAL_PARTITION_MODE__SPX > */
			/* <1:SPATIAL_PARTITION_MODE__DPX > */
			/* <2:SPATIAL_PARTITION_MODE__TPX > */
			/* <3:SPATIAL_PARTITION_MODE__QPX > */
			/* <4:SPATIAL_PARTITION_MODE__CPX > */
			/* <IDX MAP END> */
			/* <name:Spatial Paritioning Mode; type:INT; size:4; base:10; idxmap:SPM IDX; valmap:1;> */
			enum spatial_partition_mode spatial_part_mode;
			/* <name:GFX Partition Count; type:INT; size:4; base:10;> */
			uint32_t gfx_part_cnt;
			/* <name:Number XCC per Partition; type:INT; size:4; base:10;> */
			uint32_t num_xcc_per_partition;
		} chip_info; /* <BLK END> */

		/* <name:XGMI Information; blk:GEN.GPU.XGMI;> */
		struct {
			/* <name:XGMI Node ID; type:INT; size:8; base:16;> */
			uint64_t xgmi_node_id;
			/* <name:XGMI Hive ID; type:INT; size:8; base:16;> */
			uint64_t xgmi_hive_id;
			/* <name:XGMI Node Segment Size; type:INT; size:8; base:10;> */
			uint64_t xgmi_node_segment_size;
			/* <name:XGMI Physical Node ID; type:INT; size:4; base:10;> */
			uint32_t xgmi_phy_node_id;
			/* <name:XGMI Physical Number of Nodes; type:INT; size:4; base:16;> */
			uint32_t xgmi_phy_nodes_num;
		} xgmi_info; /* <BLK END> */

		/* <name:Resource Map Information; blk:GEN.GPU.RMAP;> */
		struct {
			/* <name:FB Addr; type:INT; size:8; base:16;> */
			uint64_t fb_pa;
			/* <name:FB Size; type:INT; size:8; base:10;> */
			uint64_t fb_size;
			/* <name:MMIO Addr; type:INT; size:8; base:16;> */
			uint64_t mmio_pa;
			/* <name:MMIO Size; type:INT; size:8; base:10;> */
			uint64_t mmio_size;
			/* <name:IOMEM Addr; type:INT; size:8; base:16;> */
			uint64_t io_mem_pa;
			/* <name:IOMEM Size; type:INT; size:8; base:10;> */
			uint64_t io_mem_size;
			/* <name:DoorBell Addr ; type:INT; size:8; base:16;> */
			uint64_t doorbell_pa;
			/* <name:DoorBell Size ; type:INT; size:8; base:10;> */
			uint64_t doorbell_size;
		} resource_map; /* <BLK END> */
		/* <name:VBIOS Information; blk:GEN.GPU.VBIOS;> */
		struct {
			/* <name:Version; type:INT; size:4; base:16|10;> */
			uint32_t version;
			/* <name:Build Number; type:STRING; size:8;> */
			uint8_t build_num[8];
			/* <name:Part Info; type:STRING; size:128;> */
			uint8_t part_info[128];
			/* <name:Build Date; type:STRING; size:16;> */
			uint8_t build_date[16];
			/* <name:Reserved; type:INT; size:4; base:10;> */
			uint32_t reserved;
		} vbios_info; /* <BLK END> */

		/* <name:Firmware Information; blk:GEN.GPU.FW; rep: 71;> */
		struct {
			/* <IDX MAP> */
			/* <name:Firmware IDX> */
			/* <1:AMDGV_FIRMWARE_ID__SMU> */
			/* <2:AMDGV_FIRMWARE_ID__CP_CE> */
			/* <3:AMDGV_FIRMWARE_ID__CP_PFP> */
			/* <4:AMDGV_FIRMWARE_ID__CP_ME> */
			/* <5:AMDGV_FIRMWARE_ID__CP_MEC_JT1> */
			/* <6:AMDGV_FIRMWARE_ID__CP_MEC_JT2> */
			/* <7:AMDGV_FIRMWARE_ID__CP_MEC1> */
			/* <8:AMDGV_FIRMWARE_ID__CP_MEC2> */
			/* <9:AMDGV_FIRMWARE_ID__RLC> */
			/* <10:AMDGV_FIRMWARE_ID__SDMA0> */
			/* <11:AMDGV_FIRMWARE_ID__SDMA1> */
			/* <12:AMDGV_FIRMWARE_ID__SDMA2> */
			/* <13:AMDGV_FIRMWARE_ID__SDMA3> */
			/* <14:AMDGV_FIRMWARE_ID__SDMA4> */
			/* <15:AMDGV_FIRMWARE_ID__SDMA5> */
			/* <16:AMDGV_FIRMWARE_ID__SDMA6> */
			/* <17:AMDGV_FIRMWARE_ID__SDMA7> */
			/* <18:AMDGV_FIRMWARE_ID__VCN> */
			/* <19:AMDGV_FIRMWARE_ID__UVD> */
			/* <20:AMDGV_FIRMWARE_ID__VCE> */
			/* <21:AMDGV_FIRMWARE_ID__ISP> */
			/* <22:AMDGV_FIRMWARE_ID__DMCU_ERAM> */
			/* <23:AMDGV_FIRMWARE_ID__DMCU_ISR> */
			/* <24:AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM> */
			/* <25:AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM> */
			/* <26:AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL> */
			/* <27:AMDGV_FIRMWARE_ID__RLC_V> */
			/* <28:AMDGV_FIRMWARE_ID__RLC_P> */
			/* <29:AMDGV_FIRMWARE_ID__MMSCH> */
			/* <30:AMDGV_FIRMWARE_ID__PSP_SYS> */
			/* <31:AMDGV_FIRMWARE_ID__PSP_SOS> */
			/* <32:AMDGV_FIRMWARE_ID__PSP_TOC> */
			/* <33:AMDGV_FIRMWARE_ID__PSP_KEYDB> */
			/* <34:AMDGV_FIRMWARE_ID__DFC_FW> */
			/* <35:AMDGV_FIRMWARE_ID__PSP_BL> */
			/* <36:AMDGV_FIRMWARE_ID__PSP_SPL> */
			/* <37:AMDGV_FIRMWARE_ID__DRV_CAP> */
			/* <38:AMDGV_FIRMWARE_ID__SEC_POLICY_STAGE2> */
			/* <39:AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST> */
			/* <40:AMDGV_FIRMWARE_ID__IMU_DRAM> */
			/* <41:AMDGV_FIRMWARE_ID__IMU_IRAM> */
			/* <42:AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0> */
			/* <43:AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1> */
			/* <44:AMDGV_FIRMWARE_ID__CP_MES> */
			/* <45:AMDGV_FIRMWARE_ID__MES_STACK> */
			/* <46:AMDGV_FIRMWARE_ID__MES_THREAD1> */
			/* <47:AMDGV_FIRMWARE_ID__MES_THREAD1_STACK> */
			/* <48:AMDGV_FIRMWARE_ID__RLX6> */
			/* <49:AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT> */
			/* <50:AMDGV_FIRMWARE_ID__RS64_ME_UCODE> */
			/* <51:AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA> */
			/* <52:AMDGV_FIRMWARE_ID__RS64_ME_P1_DATA> */
			/* <53:AMDGV_FIRMWARE_ID__RS64_PFP_UCODE> */
			/* <54:AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA> */
			/* <55:AMDGV_FIRMWARE_ID__RS64_PFP_P1_DATA> */
			/* <56:AMDGV_FIRMWARE_ID__RS64_MEC_UCODE> */
			/* <57:AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA> */
			/* <58:AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA> */
			/* <59:AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA> */
			/* <60:AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA> */
			/* <61:AMDGV_FIRMWARE_ID__PPTABLE> */
			/* <62:AMDGV_FIRMWARE_ID__P2S_TABLE> */
			/* <63:AMDGV_FIRMWARE_ID__PSP_SOC> */
			/* <64:AMDGV_FIRMWARE_ID__PSP_DBG> */
			/* <65:AMDGV_FIRMWARE_ID__PSP_INTF> */
			/* <66:AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1> */
			/* <67:AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1> */
			/* <68:AMDGV_FIRMWARE_ID__RLCV_LX7> */
			/* <69:AMDGV_FIRMWARE_ID__RLC_SAVE_RESTROE_LIST> */
			/* <70:AMDGV_FIRMWARE_ID__PSP_RAS> */
			/* <71:AMDGV_FIRMWARE_ID__RAS_TA> */
			/* <IDX MAP END> */
			/* <name:Firmware ID; type:INT; size:4; base:10; idxmap:Firmware IDX; valmap:1;> */
			uint32_t fw_id;
			/* <name:Version; type:INT; size:4; base:16|10;> */
			uint32_t version;
		} fw_info[AMDGV_FIRMWARE_ID__MAX]; /* <BLK END> */

		/* <name:PF Information; blk:GEN.GPU.PF;> */
		struct {
			/* <name:FB Address; type:INT; size:8; base:16;> */
			uint64_t fb_pa;
			/* <name:FB Size; type:INT; size:8; base:16|10;> */
			uint64_t fb_size;
			/* <name:Total FB Available; type:INT; size:8; base:16|10;> */
			uint64_t fb_total_avail;
			/* <name:FB Reserved Address; type:INT; size:8; base:16;> */
			uint64_t fb_reservation_offset;
			/* <name:FB Reserved Size; type:INT; size:8; base:16|10;> */
			uint64_t fb_reservation_size;
		} pf_info; /* <BLK END> */

		/* <name:Framebuffer Information; blk:GEN.GPU.FB;> */
		struct {
			struct {
				/* <name:FB CSA Size; type:INT; size:8; base:16|10;> */
				uint64_t size;
				/* <name:FB CSA Addr; type:INT; size:8; base:16;> */
				uint64_t offset;
			} csa_region;
			struct {
				/* <name:FB TMR Size; type:INT; size:8; base:16|10;> */
				uint64_t size;
				/* <name:FB TMR ADDR; type:INT; size:8; base:16;> */
				uint64_t offset;
			} tmr_region;
			/* <name:FB VF Region; type:OBJECT; rep:31;> */
			struct {
				/* <name:Valid Region; type:INT; size:4; base:10;> */
				uint32_t valid_region;
				struct {
					/* <name:DXCHG Size; type:INT; size:8; base:16|10;> */
					uint64_t size;
					/* <name:DXCHG Addr; type:INT; size:8; base:16;> */
					uint64_t offset;
				} func_dataexchange;
				struct {
					/* <name:IPD Size; type:INT; size:8; base:16|10;> */
					uint64_t size;
					/* <name:IPD Addr; type:INT; size:8; base:16;> */
					uint64_t offset;
				} func_ipd;
			} vf_region[AMDGV_MAX_VF_NUM]; /* <OBJECT END> */
		} fb_info; /* <BLK END> */

		/* <name:VF Information; blk:GEN.GPU.VF; rep: 31;> */
		struct {
			/* <name:Valid Func; type:INT; size:4; base:10;> */
			uint32_t valid_vf;
			/* <name:VF ID; type:INT; size:4; base:16|10;> */
			uint32_t idx_vf;
			/* <name:VF BDF; type:INT; size:4; base:16|10;> */
			uint32_t bdf;
			/* <name:VF FB Address; type:INT; size:8; base:16;> */
			uint64_t fb_offset;
			/* <name:VF FB size; type:INT; size:8; base:16|10;> */
			uint64_t fb_size;
			/* <name:VF Real FB size; type:INT; size:8; base:16|10;> */
			uint64_t real_fb_size;
			/* <name:VF GFX Time Slice; type:INT; size:4; base:10;> */
			uint32_t gfx_time_slice;
			/* <name:VF GFX Divider; type:INT; size:4; base:10;> */
			uint32_t gfx_time_divider;
			/* <name:VF Resource Mapped; type:INT; size:2; base:10;> */
			uint16_t res_mapped;
			/* <name:VF Auto Run; type:INT; size:2; base:10;> */
			uint16_t auto_run;
			/* <IDX MAP> */
			/* <name:MM IDX> */
			/* <0:AMDGV_HEVC_ENGINE > */
			/* <1:AMDGV_VCE_ENGINE > */
			/* <2:AMDGV_HEVC1_ENGINE > */
			/* <3:AMDGV_VCN_ENGINE > */
			/* <4:AMDGV_JPEG_ENGINE > */
			/* <IDX MAP END> */
			/* <name:VF MMBandwidth; type:INT; rep:5; size:4; base:10; idxmap:MM IDX;> */
			uint32_t mm_bandwidth[AMDGV_MAX_MM_ENGINE];
			/* <IDX MAP> */
			/* <name:Sched Block IDX> */
			/* <0:AMDGV_SCHED_BLOCK_GFX > */
			/* <1:AMDGV_SCHED_BLOCK_UVD > */
			/* <2:AMDGV_SCHED_BLOCK_VCE > */
			/* <3:AMDGV_SCHED_BLOCK_UVD1 > */
			/* <4:AMDGV_SCHED_BLOCK_VCN > */
			/* <5:AMDGV_SCHED_BLOCK_VCN1 > */
			/* <6:AMDGV_SCHED_BLOCK_JPEG > */
			/* <IDX MAP END> */
			/* <name:VF Time Slice; type:INT; rep:7; size:4; base:10; idxmap:Sched Block IDX;> */
			uint32_t time_slice[AMDGV_SCHED_BLOCK_MAX];

			/* <name:VF Guard Info; type:OBJECT; rep:4;> */
			struct {
				/* enum amdgv_guard_type */
				/* <IDX MAP> */
				/* <name:VF Guard IDX> */
				/* <0:AMDGV_GUARD_EVENT_FLR > */
				/* <1:AMDGV_GUARD_EVENT_EXCLUSIVE_MOD > */
				/* <2:AMDGV_GUARD_EVENT_EXCLUSIVE_TIMEOUT > */
				/* <3:AMDGV_GUARD_EVENT_ALL_INT > */
				/* <IDX MAP END> */
				/* <name:Guard Type; type:INT; size:4; base:10; idxmap:VF Guard IDX; valmap:1;> */
				uint32_t guard_type;
				/* <name:Guard Threshold; type:INT; size:4; base:10;> */
				uint32_t threshold;
				/* <name:Guard Interval; type:INT; size:8; base:10;> */
				uint64_t interval;
			} guard[AMDGV_GUARD_EVENT_MAX]; /* <OBJECT END> */
		} vf_info[AMDGV_MAX_VF_NUM]; /* <BLK END> */
	} gpu_info;/* <BLK END> */
}; /* <BLK END> */
/* <GEN JSON END> */

#endif
