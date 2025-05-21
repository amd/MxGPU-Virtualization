/*
 * Copyright (c) 2014-2021 Advanced Micro Devices, Inc. All rights reserved.
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

/*IMPORTANT NOTES
 * If a change in VBIOS/Driver/Tool's interface is only needed for
 * SoC15 and forward products, then the change is only needed in this
 * atomfirmware.h header file.
 * If a change in VBIOS/Driver/Tool's interface is only needed for pre-SoC15
 * products, then the change is only needed in atombios.h header file.
 * If a change is needed for both pre and post SoC15 products, then the change
 *  has to be made separately and might be differently in both atomfirmware.h
 * and atombios.h.
 */

#ifndef _ATOMFIRMWARE_H_
#define _ATOMFIRMWARE_H_

#include "amdgv_basetypes.h"

#define get_index_into_master_table(master_table, table_name)                                 \
	(offsetof(struct master_table, table_name) / sizeof(uint16_t))

#define BIOS_ATOM_PREFIX   "ATOMBIOS"
#define BIOS_VERSION_PREFIX  "ATOMBIOSBK-AMD"
#define BIOS_STRING_LENGTH 43

#pragma pack(1) /* BIOS data must use byte aligment*/

enum atombios_image_offset {
	OFFSET_TO_ATOM_ROM_HEADER_POINTER	 = 0x00000048,
	OFFSET_TO_ATOM_ROM_IMAGE_SIZE		 = 0x00000002,
	OFFSET_TO_ATOMBIOS_ASIC_BUS_MEM_TYPE	 = 0x94,
	MAXSIZE_OF_ATOMBIOS_ASIC_BUS_MEM_TYPE	 = 20,
	OFFSET_TO_GET_ATOMBIOS_NUMBER_OF_STRINGS = 0x2f,
	OFFSET_TO_GET_ATOMBIOS_STRING_START	 = 0x6e,
	OFFSET_TO_VBIOS_PART_NUMBER		 = 0x80,
	OFFSET_TO_VBIOS_DATE			 = 0x50
};

/****************************************************************************
 * Common header for all tables (Data table, Command function).
 * Every table pointed in _ATOM_MASTER_DATA_TABLE has this common header.
 * And the pointer actually points to this header.
 ****************************************************************************/

struct atom_common_table_header {
	uint16_t structuresize;
	uint8_t	 format_revision;
	uint8_t	 content_revision;
};

/*=================hw function portion===============================*/

/****************************************************************************
 * Structures used in Command.mtb, each function name is not given here since
 * those function could change from time to time
 * The real functionality of each function is associated with the parameter
 * structure version when defined
 * For all internal cmd function definitions, please reference to atomstruct.h
 ****************************************************************************/
struct atom_master_list_of_command_functions_v2_1 {
	uint16_t asic_init;                   //Function
	uint16_t cmd_function1;               //used as an internal one
	uint16_t cmd_function2;               //used as an internal one
	uint16_t cmd_function3;               //used as an internal one
	uint16_t digxencodercontrol;          //Function
	uint16_t cmd_function5;               //used as an internal one
	uint16_t cmd_function6;               //used as an internal one
	uint16_t cmd_function7;               //used as an internal one
	uint16_t cmd_function8;               //used as an internal one
	uint16_t cmd_function9;               //used as an internal one
	uint16_t setengineclock;              //Function
	uint16_t setmemoryclock;              //Function
	uint16_t setpixelclock;               //Function
	uint16_t enabledisppowergating;       //Function
	uint16_t cmd_function14;              //used as an internal one
	uint16_t cmd_function15;              //used as an internal one
	uint16_t cmd_function16;              //used as an internal one
	uint16_t cmd_function17;              //used as an internal one
	uint16_t cmd_function18;              //used as an internal one
	uint16_t cmd_function19;              //used as an internal one
	uint16_t cmd_function20;              //used as an internal one
	uint16_t cmd_function21;              //used as an internal one
	uint16_t cmd_function22;              //used as an internal one
	uint16_t cmd_function23;              //used as an internal one
	uint16_t cmd_function24;              //used as an internal one
	uint16_t cmd_function25;              //used as an internal one
	uint16_t cmd_function26;              //used as an internal one
	uint16_t cmd_function27;              //used as an internal one
	uint16_t cmd_function28;              //used as an internal one
	uint16_t smc_init;              //used as an internal one
	uint16_t cmd_function30;              //used as an internal one
	uint16_t cmd_function31;              //used as an internal one
	uint16_t cmd_function32;              //used as an internal one
	uint16_t cmd_function33;              //used as an internal one
	uint16_t blankcrtc;                   //Function
	uint16_t enablecrtc;                  //Function
	uint16_t cmd_function36;              //used as an internal one
	uint16_t cmd_function37;              //used as an internal one
	uint16_t cmd_function38;              //used as an internal one
	uint16_t cmd_function39;              //used as an internal one
	uint16_t cmd_function40;              //used as an internal one
	uint16_t getsmuclockinfo;             //Function
	uint16_t selectcrtc_source;           //Function
	uint16_t cmd_function43;              //used as an internal one
	uint16_t cmd_function44;              //used as an internal one
	uint16_t cmd_function45;              //used as an internal one
	uint16_t setdceclock;                 //Function
	uint16_t getmemoryclock;              //Function
	uint16_t getengineclock;              //Function
	uint16_t setcrtc_usingdtdtiming;      //Function
	uint16_t externalencodercontrol;      //Function
	uint16_t cmd_function51;              //used as an internal one
	uint16_t cmd_function52;              //used as an internal one
	uint16_t cmd_function53;              //used as an internal one
	uint16_t processi2cchanneltransaction;//Function
	uint16_t cmd_function55;              //used as an internal one
	uint16_t cmd_function56;              //used as an internal one
	uint16_t cmd_function57;              //used as an internal one
	uint16_t cmd_function58;              //used as an internal one
	uint16_t cmd_function59;              //used as an internal one
	uint16_t computegpuclockparam;        //Function
	uint16_t cmd_function61;              //used as an internal one
	uint16_t cmd_function62;              //used as an internal one
	uint16_t dynamicmemorysettings;       //Function function
	uint16_t memorytraining;              //Function function
	uint16_t cmd_function65;              //used as an internal one
	uint16_t cmd_function66;              //used as an internal one
	uint16_t setvoltage;                  //Function
	uint16_t cmd_function68;              //used as an internal one
	uint16_t readefusevalue;              //Function
	uint16_t cmd_function70;              //used as an internal one
	uint16_t cmd_function71;              //used as an internal one
	uint16_t cmd_function72;              //used as an internal one
	uint16_t cmd_function73;              //used as an internal one
	uint16_t cmd_function74;              //used as an internal one
	uint16_t cmd_function75;              //used as an internal one
	uint16_t dig1transmittercontrol;      //Function
	uint16_t cmd_function77;              //used as an internal one
	uint16_t processauxchanneltransaction;//Function
	uint16_t cmd_function79;              //used as an internal one
	uint16_t getvoltageinfo;              //Function
};

/*======================sw data table portion======================*/
/****************************************************************************
 * Structures used in data.mtb, each data table name is not given here since
 * those data table could change from time to time
 * The real name of each table is given when its data structure
 * version is defined
 ****************************************************************************/
struct atom_master_list_of_data_tables_v2_1 {
	uint16_t utilitypipeline;
	uint16_t multimedia_info;
	uint16_t smc_dpm_info;
	uint16_t sw_datatable3;
	uint16_t firmwareinfo;             /* Shared by various SW components */
	uint16_t sw_datatable5;
	uint16_t lcd_info;                 /* Shared by various SW components */
	uint16_t sw_datatable7;
	uint16_t smu_info;
	uint16_t sw_datatable9;
	uint16_t sw_datatable10;
	uint16_t vram_usagebyfirmware;     /* Shared by various SW components */
	uint16_t gpio_pin_lut;             /* Shared by various SW components */
	uint16_t sw_datatable13;
	uint16_t gfx_info;
	uint16_t powerplayinfo;            /* Shared by various SW components */
	uint16_t gpu_virt;
	uint16_t sw_datatable17;
	uint16_t sw_datatable18;
	uint16_t sw_datatable19;
	uint16_t sw_datatable20;
	uint16_t sw_datatable21;
	uint16_t displayobjectinfo;        /* Shared by various SW components */
	uint16_t indirectioaccess;                 /* used as an internal one */
	uint16_t umc_info;                 /* Shared by various SW components */
	uint16_t sw_datatable25;
	uint16_t sw_datatable26;
	uint16_t dce_info;                 /* Shared by various SW components */
	uint16_t vram_info;                /* Shared by various SW components */
	uint16_t sw_datatable29;
	uint16_t integratedsysteminfo;     /* Shared by various SW components */
	uint16_t asic_profiling_info;      /* Shared by various SW components */
	uint16_t voltageobject_info;       /* shared by various SW components */
	uint16_t sw_datatable33;
	uint16_t sw_datatable34;
};

/*
 ***************************************************************************
  Data Table firmwareinfo  structure
 ***************************************************************************
 */

struct atom_firmware_info_v3_1 {
	struct atom_common_table_header table_header;
	uint32_t			firmware_revision;
	uint32_t			bootup_sclk_in10khz;
	uint32_t			bootup_mclk_in10khz;
	uint32_t firmware_capability; // enum atombios_firmware_capability
	uint32_t main_call_parser_entry;
	uint32_t bios_scratch_reg_startaddr;
	uint16_t bootup_vddc_mv;
	uint16_t bootup_vddci_mv;
	uint16_t bootup_mvddc_mv;
	uint16_t bootup_vddgfx_mv;
	uint8_t	 mem_module_id;
	uint8_t	 coolingsolution_id;
	uint8_t	 reserved1[2];
	uint32_t mc_baseaddr_high;
	uint32_t mc_baseaddr_low;
	uint32_t reserved2[6];
};

/* Total 32bit cap indication */
enum atombios_firmware_capability {
	ATOM_FIRMWARE_CAP_FIRMWARE_POSTED    = 0x00000001,
	ATOM_FIRMWARE_CAP_GPU_VIRTUALIZATION = 0x00000002,
	ATOM_FIRMWARE_CAP_WMI_SUPPORT	     = 0x00000040,
	ATOM_FIRMWARE_CAP_SRAM_ECC           = 0x00000200,
};

struct atom_firmware_info_v3_2 {
	struct atom_common_table_header table_header;
	uint32_t			firmware_revision;
	uint32_t			bootup_sclk_in10khz;
	uint32_t			bootup_mclk_in10khz;
	uint32_t			firmware_capability;
	uint32_t			main_call_parser_entry;
	uint32_t			bios_scratch_reg_startaddr;
	uint16_t			bootup_vddc_mv;
	uint16_t			bootup_vddci_mv;
	uint16_t			bootup_mvddc_mv;
	uint16_t			bootup_vddgfx_mv;
	uint8_t				mem_module_id;
	uint8_t				coolingsolution_id;
	uint8_t				reserved1[2];
	uint32_t			mc_baseaddr_high;
	uint32_t			mc_baseaddr_low;
	uint8_t				board_i2c_feature_id;
	uint8_t				board_i2c_feature_gpio_id;
	uint8_t				board_i2c_feature_slave_addr;
	uint8_t				reserved3;
	uint16_t			bootup_mvddq_mv;
	uint16_t			bootup_mvpp_mv;
	uint32_t			zfbstartaddrin16mb;
	uint32_t			reserved2[3];
};

//IPCLEAN_START
struct atom_firmware_info_v3_3 {
	struct atom_common_table_header table_header;
	uint32_t			firmware_revision;
	uint32_t			bootup_sclk_in10khz;
	uint32_t			bootup_mclk_in10khz;
	uint32_t			firmware_capability;
	uint32_t			main_call_parser_entry;
	uint32_t			bios_scratch_reg_startaddr;
	uint16_t			bootup_vddc_mv;
	uint16_t			bootup_vddci_mv;
	uint16_t			bootup_mvddc_mv;
	uint16_t			bootup_vddgfx_mv;
	uint8_t				mem_module_id;
	uint8_t				coolingsolution_id;
	uint8_t				reserved1[2];
	uint32_t			mc_baseaddr_high;
	uint32_t			mc_baseaddr_low;
	uint8_t				board_i2c_feature_id;
	uint8_t				board_i2c_feature_gpio_id;
	uint8_t				board_i2c_feature_slave_addr;
	uint8_t				reserved3;
	uint16_t			bootup_mvddq_mv;
	uint16_t			bootup_mvpp_mv;
	uint32_t			zfbstartaddrin16mb;
	uint32_t			pplib_pptable_id;
	uint32_t			mvdd_ratio;
	uint32_t			reserved2;
};

struct atom_firmware_info_v3_4 {
	struct atom_common_table_header table_header;
	uint32_t			firmware_revision;
	uint32_t			bootup_sclk_in10khz;
	uint32_t			bootup_mclk_in10khz;
	uint32_t			firmware_capability;             /* enum atombios_firmware_capability */
	uint32_t			main_call_parser_entry;          /* direct address of main parser call in VBIOS binary. */
	uint32_t			bios_scratch_reg_startaddr;      /* 1st bios scratch register dword address */
	uint16_t			bootup_vddc_mv;
	uint16_t			bootup_vddci_mv;
	uint16_t			bootup_mvddc_mv;
	uint16_t			bootup_vddgfx_mv;
	uint8_t 			mem_module_id;
	uint8_t 			coolingsolution_id;              /*0: Air cooling; 1: Liquid cooling ... */
	uint8_t 			reserved1[2];
	uint32_t			mc_baseaddr_high;
	uint32_t			mc_baseaddr_low;
	uint8_t				board_i2c_feature_id;            /* enum of atom_board_i2c_feature_id_def */
	uint8_t				board_i2c_feature_gpio_id;       /* i2c id find in gpio_lut data table gpio_id */
	uint8_t				board_i2c_feature_slave_addr;
	uint8_t				ras_rom_i2c_slave_addr;
	uint16_t			bootup_mvddq_mv;
	uint16_t			bootup_mvpp_mv;
	uint32_t			zfbstartaddrin16mb;
	uint32_t			pplib_pptable_id;                /* if pplib_pptable_id!=0, pplib get powerplay table inside driver instead of from VBIOS */
	uint32_t			mvdd_ratio;                      /* mvdd_raio = (real mvdd in power rail)*1000/(mvdd_output_from_svi2) */
	uint16_t			hw_bootup_vddgfx_mv;             /* hw default vddgfx voltage level decide by board strap */
	uint16_t			hw_bootup_vddc_mv;               /* hw default vddc voltage level decide by board strap */
	uint16_t			hw_bootup_mvddc_mv;              /* hw default mvddc voltage level decide by board strap */
	uint16_t			hw_bootup_vddci_mv;              /* hw default vddci voltage level decide by board strap */
	uint32_t			maco_pwrlimit_mw;                /* bomaco mode power limit in unit of m-watt */
	uint32_t			usb_pwrlimit_mw;                 /* power limit when USB is enable in unit of m-watt */
	uint32_t			fw_reserved_size_in_kb;          /* VBIOS reserved extra fw size in unit of kb. */
	uint32_t			pspbl_init_done_reg_addr;
	uint32_t			pspbl_init_done_value;
	uint32_t			pspbl_init_done_check_timeout;   /* time out in unit of us when polling pspbl init done */
	uint32_t			reserved[2];
};

/*
 ***************************************************************************
    Data Table vram_usagebyfirmware  structure
 ***************************************************************************
 */

struct vram_usagebyfirmware_v2_1 {
	struct atom_common_table_header table_header;
	uint32_t			start_address_in_kb;
	uint16_t			used_by_firmware_in_kb;
	uint16_t			used_by_driver_in_kb;
};

struct vram_usagebyfirmware_v2_2 {
	struct atom_common_table_header table_header;
	uint32_t	fw_region_start_address_in_kb;
	uint16_t	used_by_firmware_in_kb;
	uint16_t	reserved;
	uint32_t	driver_region0_start_address_in_kb;
	uint32_t	used_by_driver_region0_in_kb;
	uint32_t	reserved32[7];						// expand for future, for example multiple holes
};

/*
 ***************************************************************************
 *Data Table smc_dpm_info  structure
 ***************************************************************************
 */
struct smudpm_i2c_controller_config_v3 {
	uint8_t Enabled;
	uint8_t Speed;
	uint8_t SlaveAddress;
	uint8_t ControllerPort;
	uint8_t ControllerName;
	uint8_t ThermalThrotter;
	uint8_t I2cProtocol;
	uint8_t PaddingConfig;
};

struct atom_smc_dpm_info_v4_10 {
	struct atom_common_table_header  table_header;

	// SECTION: BOARD PARAMETERS
	// Telemetry Settings
	uint16_t GfxMaxCurrent; // in Amps
	uint8_t  GfxOffset;     // in Amps
	uint8_t  Padding_TelemetryGfx;

	uint16_t SocMaxCurrent; // in Amps
	uint8_t  SocOffset;     // in Amps
	uint8_t  Padding_TelemetrySoc;

	uint16_t MemMaxCurrent; // in Amps
	uint8_t  MemOffset;     // in Amps
	uint8_t  Padding_TelemetryMem;

	uint16_t BoardMaxCurrent; // in Amps
	uint8_t  BoardOffset;     // in Amps
	uint8_t  Padding_TelemetryBoardInput;

	// Platform input telemetry voltage coefficient
	uint32_t BoardVoltageCoeffA; // decode by /1000
	uint32_t BoardVoltageCoeffB; // decode by /1000

	// GPIO Settings
	uint8_t  VR0HotGpio;     // GPIO pin configured for VR0 HOT event
	uint8_t  VR0HotPolarity; // GPIO polarity for VR0 HOT event
	uint8_t  VR1HotGpio;     // GPIO pin configured for VR1 HOT event
	uint8_t  VR1HotPolarity; // GPIO polarity for VR1 HOT event

	// UCLK Spread Spectrum
	uint8_t  UclkSpreadEnabled; // on or off
	uint8_t  UclkSpreadPercent; // Q4.4
	uint16_t UclkSpreadFreq;    // kHz

	// FCLK Spread Spectrum
	uint8_t  FclkSpreadEnabled; // on or off
	uint8_t  FclkSpreadPercent; // Q4.4
	uint16_t FclkSpreadFreq;    // kHz

	// I2C Controller Structure
	struct smudpm_i2c_controller_config_v3  I2cControllers[8];

	// GPIO pins for I2C communications with 2nd controller for Input Telemetry Sequence
	uint8_t  GpioI2cScl; // Serial Clock
	uint8_t  GpioI2cSda; // Serial Data
	uint16_t spare5;

	uint32_t reserved[16];
};

/*
 ***************************************************************************
 *             Data Table umc_info  structure
 ***************************************************************************
 */
struct atom_umc_info_v3_1 {
	struct atom_common_table_header table_header;
	uint32_t			ucode_version;
	uint32_t			ucode_rom_startaddr;
	uint32_t			ucode_length;
	uint16_t			umc_reg_init_offset;
	uint16_t			customer_ucode_name_offset;
	uint16_t			mclk_ss_percentage;
	uint16_t			mclk_ss_rate_10hz;
	uint8_t				umcip_min_ver;
	uint8_t				umcip_max_ver;
	uint8_t				vram_type; //enum of atom_dgpu_vram_type
	uint8_t				umc_config;
	uint32_t			mem_refclk_10khz;
};

// umc_info.umc_config
enum atom_umc_config_def {
	UMC_CONFIG__ENABLE_1KB_INTERLEAVE_MODE  =   0x00000001,
	UMC_CONFIG__DEFAULT_MEM_ECC_ENABLE      =   0x00000002,
	UMC_CONFIG__ENABLE_HBM_LANE_REPAIR      =   0x00000004,
	UMC_CONFIG__ENABLE_BANK_HARVESTING      =   0x00000008,
	UMC_CONFIG__ENABLE_PHY_REINIT           =   0x00000010,
	UMC_CONFIG__DISABLE_UCODE_CHKSTATUS     =   0x00000020,
};

struct atom_umc_info_v3_2 {
  struct  atom_common_table_header  table_header;
  uint32_t ucode_version;
  uint32_t ucode_rom_startaddr;
  uint32_t ucode_length;
  uint16_t umc_reg_init_offset;
  uint16_t customer_ucode_name_offset;
  uint16_t mclk_ss_percentage;
  uint16_t mclk_ss_rate_10hz;
  uint8_t umcip_min_ver;
  uint8_t umcip_max_ver;
  uint8_t vram_type;              //enum of atom_dgpu_vram_type
  uint8_t umc_config;
  uint32_t mem_refclk_10khz;
  uint32_t pstate_uclk_10khz[4];
  uint16_t umcgoldenoffset;
  uint16_t densitygoldenoffset;
};

struct atom_umc_info_v3_3 {
  struct  atom_common_table_header  table_header;
  uint32_t ucode_reserved;
  uint32_t ucode_rom_startaddr;
  uint32_t ucode_length;
  uint16_t umc_reg_init_offset;
  uint16_t customer_ucode_name_offset;
  uint16_t mclk_ss_percentage;
  uint16_t mclk_ss_rate_10hz;
  uint8_t umcip_min_ver;
  uint8_t umcip_max_ver;
  uint8_t vram_type;              //enum of atom_dgpu_vram_type
  uint8_t umc_config;
  uint32_t mem_refclk_10khz;
  uint32_t pstate_uclk_10khz[4];
  uint16_t umcgoldenoffset;
  uint16_t densitygoldenoffset;
  uint32_t umc_config1;
  uint32_t bist_data_startaddr;
  uint32_t reserved[2];
};

enum atom_umc_config1_def {
	UMC_CONFIG1__ENABLE_PSTATE_PHASE_STORE_TRAIN = 0x00000001,
	UMC_CONFIG1__ENABLE_AUTO_FRAMING = 0x00000002,
	UMC_CONFIG1__ENABLE_RESTORE_BIST_DATA = 0x00000004,
	UMC_CONFIG1__DISABLE_STROBE_MODE = 0x00000008,
	UMC_CONFIG1__DEBUG_DATA_PARITY_EN = 0x00000010,
	UMC_CONFIG1__ENABLE_ECC_CAPABLE = 0x00010000,
};

struct atom_umc_info_v4_0 {
	struct atom_common_table_header table_header;
	uint32_t			ucode_reserved[5];
	uint8_t				umcip_min_ver;
	uint8_t				umcip_max_ver;
	uint8_t				vram_type; // enum of atom_dgpu_vram_type
	uint8_t				umc_config;
	uint32_t			mem_refclk_10khz;
	uint32_t			clk_reserved[4];
	uint32_t			golden_reserved;
	uint32_t			umc_config1;
	uint32_t			reserved[2];
	uint8_t				channel_num;
	uint8_t				channel_width;
	uint8_t				channel_reserve[2];
	uint8_t				umc_info_reserved[16];
};

/*
 ***************************************************************************
 *                All Command Function structure definition
 ****************************************************************************
 */

/*
 ***************************************************************************
 *                Structures used by asic_init
 ***************************************************************************
 */

struct asic_init_engine_parameters {
	uint32_t sclkfreqin10khz : 24;
	uint32_t engineflag	 : 8; /* enum atom_asic_init_engine_flag */
};

struct asic_init_mem_parameters {
	uint32_t mclkfreqin10khz : 24;
	uint32_t memflag	 : 8; /* enum atom_asic_init_mem_flag  */
};

struct asic_init_parameters_v2_1 {
	struct asic_init_engine_parameters engineparam;
	struct asic_init_mem_parameters	   memparam;
};

struct asic_init_ps_allocation_v2_1 {
	struct asic_init_parameters_v2_1 param;
	uint32_t			 reserved[16];
};

enum atom_asic_init_engine_flag {
	b3NORMAL_ENGINE_INIT   = 0,
	b3SRIOV_SKIP_ASIC_INIT = 0x02,
	b3SRIOV_LOAD_UCODE     = 0x40,
};

/*
 ***************************************************************************
 *            Structures used by getsmuclockinfo
 ***************************************************************************
 */
struct atom_get_smu_clock_info_parameters_v3_1 {
	uint8_t syspll_id;
	uint8_t clk_id;
	uint8_t command;
	uint8_t dfsdid;
};

enum atom_get_smu_clock_info_command {
	GET_SMU_CLOCK_INFO_V3_1_GET_CLOCK_FREQ	   = 0,
	GET_SMU_CLOCK_INFO_V3_1_GET_PLLVCO_FREQ	   = 1,
	GET_SMU_CLOCK_INFO_V3_1_GET_PLLREFCLK_FREQ = 2,
};

enum atom_smu9_syspll0_clock_id {
	SMU9_SYSPLL0_SMNCLK_ID	 = 0,  //  SMNCLK
	SMU9_SYSPLL0_SOCCLK_ID	 = 1,  //	SOCCLK (FCLK)
	SMU9_SYSPLL0_MP0CLK_ID	 = 2,  //	MP0CLK
	SMU9_SYSPLL0_MP1CLK_ID	 = 3,  //	MP1CLK
	SMU9_SYSPLL0_LCLK_ID	 = 4,  //	LCLK
	SMU9_SYSPLL0_DCLK_ID	 = 5,  //	DCLK
	SMU9_SYSPLL0_VCLK_ID	 = 6,  //	VCLK
	SMU9_SYSPLL0_ECLK_ID	 = 7,  //	ECLK
	SMU9_SYSPLL0_DCEFCLK_ID	 = 8,  //	DCEFCLK
	SMU9_SYSPLL0_DPREFCLK_ID = 10, //	DPREFCLK
	SMU9_SYSPLL0_DISPCLK_ID	 = 11, //	DISPCLK
};

enum atom_smu11_syspll_id {
	SMU11_SYSPLL0_ID   = 0,
	SMU11_SYSPLL1_0_ID = 1,
	SMU11_SYSPLL1_1_ID = 2,
	SMU11_SYSPLL1_2_ID = 3,
	SMU11_SYSPLL2_ID   = 4,
	SMU11_SYSPLL3_0_ID = 5,
	SMU11_SYSPLL3_1_ID = 6,
};

enum atom_smu11_syspll0_clock_id {
	SMU11_SYSPLL0_ECLK_ID	 = 0, // ECLK
	SMU11_SYSPLL0_SOCCLK_ID	 = 1, // SOCCLK
	SMU11_SYSPLL0_MP0CLK_ID	 = 2, // MP0CLK
	SMU11_SYSPLL0_DCLK_ID	 = 3, // DCLK
	SMU11_SYSPLL0_VCLK_ID	 = 4, // VCLK
	SMU11_SYSPLL0_DCEFCLK_ID = 5, // DCEFCLK
};

enum atom_smu11_syspll1_0_clock_id {
	SMU11_SYSPLL1_0_UCLKA_ID = 0, // UCLK_a
};

enum atom_smu11_syspll1_1_clock_id {
	SMU11_SYSPLL1_0_UCLKB_ID = 0, // UCLK_b
};

enum atom_smu11_syspll1_2_clock_id {
	SMU11_SYSPLL1_0_FCLK_ID = 0, // FCLK
};

enum atom_smu11_syspll2_clock_id {
	SMU11_SYSPLL2_GFXCLK_ID = 0, // GFXCLK
};

enum atom_smu11_syspll3_0_clock_id {
	SMU11_SYSPLL3_0_WAFCLK_ID   = 0, // WAFCLK
	SMU11_SYSPLL3_0_DISPCLK_ID  = 1, // DISPCLK
	SMU11_SYSPLL3_0_DPREFCLK_ID = 2, // DPREFCLK
};

enum atom_smu11_syspll3_1_clock_id {
	SMU11_SYSPLL3_1_MP1CLK_ID = 0, // MP1CLK
	SMU11_SYSPLL3_1_SMNCLK_ID = 1, // SMNCLK
	SMU11_SYSPLL3_1_LCLK_ID	  = 2, // LCLK
};

struct atom_get_smu_clock_info_output_parameters_v3_1 {
	union {
		uint32_t smu_clock_freq_hz;
		uint32_t syspllvcofreq_10khz;
		uint32_t sysspllrefclk_10khz;
	} atom_smu_outputclkfreq;
};

enum scratch_pre_os_mode_info_bits_def {
	ATOM_PRE_OS_MODE_MASK		   = 0x00000003,
	ATOM_PRE_OS_MODE_VGA		   = 0x00000000,
	ATOM_PRE_OS_MODE_VESA		   = 0x00000001,
	ATOM_PRE_OS_MODE_GOP		   = 0x00000002,
	ATOM_PRE_OS_MODE_PIXEL_DEPTH	   = 0x0000000C,
	ATOM_PRE_OS_MODE_PIXEL_FORMAT_MASK = 0x000000F0,
	ATOM_PRE_OS_MODE_8BIT_PAL_EN	   = 0x00000100,
	ATOM_ASIC_INIT_COMPLETE		   = 0x00000200,
#ifndef _H2INC
	ATOM_PRE_OS_MODE_NUMBER_MASK = 0xFFFF0000,
#endif
};

/*
 ***************************************************************************
 ATOM firmware ID header file
 !! Please keep it at end of the atomfirmware.h !!
 ***************************************************************************
 */
#include "atomfirmwareid.h"
#pragma pack()

#endif
