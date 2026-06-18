/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

/****************************************************************************/
/*Portion I: Definitions  shared between VBIOS and Driver                   */
/****************************************************************************/

#ifndef _ATOMBIOS_H
#define _ATOMBIOS_H

#define ATOM_VERSION_MAJOR 0x00020000
#define ATOM_VERSION_MINOR 0x00000002

#define ATOM_HEADER_VERSION (ATOM_VERSION_MAJOR | ATOM_VERSION_MINOR)

#define ATOM_VRAM_OPERATION_FLAGS_SHIFT		    30
#define ATOM_VRAM_BLOCK_NEEDS_NO_RESERVATION	    0x1
#define ATOM_VRAM_BLOCK_SRIOV_MSG_SHARE_RESERVATION 0x2

/* Endianness should be specified before inclusion,
 * default to little endian
 */
#ifndef ATOM_BIG_ENDIAN
#error Endian not specified
#endif

#ifdef _H2INC
#ifndef ULONG
typedef unsigned long ULONG;
#endif

#ifndef UCHAR
typedef unsigned char UCHAR;
#endif

#ifndef USHORT
typedef unsigned short USHORT;
#endif
#endif

// Define offset to location of ROM header.
#define OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER 0x00000048L

/****************************************************************************/
// Common header for all tables (Data table, Command table).
// Every table pointed  _ATOM_MASTER_DATA_TABLE has this common header.
// And the pointer actually points to this header.
/****************************************************************************/

typedef struct _ATOM_COMMON_TABLE_HEADER {
	USHORT usStructureSize;
	UCHAR  ucTableFormatRevision;  //Change it when the Parser is not backward compatible
	UCHAR  ucTableContentRevision; //Change it only when the table needs to change but the firmware
				       //Image can't be updated, while Driver needs to carry the new table!
} ATOM_COMMON_TABLE_HEADER;

/****************************************************************************/
// Structure stores the ROM header.
/****************************************************************************/
typedef struct _ATOM_ROM_HEADER {
	ATOM_COMMON_TABLE_HEADER sHeader;
	UCHAR			 uaFirmWareSignature[4]; //Signature to distinguish between Atombios and non-atombios,
							 //atombios should init it as "ATOM", don't change the position
	USHORT usBiosRuntimeSegmentAddress;
	USHORT usProtectedModeInfoOffset;
	USHORT usConfigFilenameOffset;
	USHORT usCRC_BlockOffset;
	USHORT usBIOS_BootupMessageOffset;
	USHORT usInt10Offset;
	USHORT usPciBusDevInitCode;
	USHORT usIoBaseAddress;
	USHORT usSubsystemVendorID;
	USHORT usSubsystemID;
	USHORT usPCI_InfoOffset;
	USHORT usMasterCommandTableOffset; //Offest for SW to get all command table offsets, Don't change the position
	USHORT usMasterDataTableOffset;	   //Offest for SW to get all data table offsets, Don't change the position
	UCHAR  ucExtendedFunctionCode;
	UCHAR  ucReserved;
} ATOM_ROM_HEADER;

typedef struct _PSP_DIRECTORY_HEADER {
	uint32_t PspCookie;    // "$PSP"
	uint32_t Checksum;     // 32 bit CRC of header items below and the entire table
	uint32_t TotalEntries; // Number of PSP Entries
	uint32_t Reserved;
} PSP_DIRECTORY_HEADER;

enum _PSP_DIRECTORY_ENTRY_TYPE {
	PSP_DIR_ENTRY_TYPE_AMD_PUBLIC_KEY	       = 0x00, // PSP entry pointer to AMD public key (Already integrated 2017-02-17)
	PSP_DIR_ENTRY_TYPE_PSP_FW_BOOT_LOADER	       = 0x01, // PSP Entry pointer to PSP boot loader in SPI/DRAM space (Already integrated 2017-02-17)
	PSP_DIR_ENTRY_TYPE_PSP_FW_TRUSTED_OS	       = 0x02, // PSP Entry pointer to PSP AMD-TEE OS in SPI/DRAM space (Required for SR-IOV SKU 2017-02-17)
	PSP_DIR_ENTRY_TYPE_VBIOS_PUBLIC_KEY	       = 0x03, // PSP entry pointer to VBIOS public key stored in SPI/DRAM space
	PSP_DIR_ENTRY_TYPE_RECOVERY_PSP_BOOT_LOADER    = 0x03, // PSP entry pointer to PSP BOOTLOADER for recovery case by on-chip PSP bootrom
	PSP_DIR_ENTRY_TYPE_VBIOS_FIRMWARE	       = 0x04, // PSP entry pointer to VBIOS in SPI/DRAM space
	PSP_DIR_ENTRY_TYPE_VBIOS_SIGNATURE	       = 0x05, // PSP entry pointer to signed VBIOS hash stored  in SPI/DRAM space
	PSP_DIR_ENTRY_TYPE_SMU_OFF_CHIP_FW	       = 0x06, // PSP entry pointer to SMU off-chip firmware
	PSP_DIR_ENTRY_TYPE_AMD_SEC_DBG_PUBLIC_KEY      = 0x07, // PSP entry pointer to Secure Unlock Public key (Already integrated 2017-02-17)
	PSP_DIR_ENTRY_TYPE_OEM_PSP_FW_PUBLIC_KEY       = 0x08, // PSP entry pointer to an optional public part of the OEM PSP Firmware Signing Key Token
	PSP_DIR_ENTRY_TYPE_AMD_SOFT_FUSE_CHAIN_0       = 0x09, // PSP entry pointer to 64bit PSP Soft Fuse Chain (Already integrated 2017-02-17)
	PSP_DIR_ENTRY_TYPE_PSP_BOOT_TIME_TRUSTLETS     = 0x0A, // PSP entry pointer to boot-loaded trustlet binaries (may need to be revised later when AMD-TEE is finalized)(Required for SR-IOV SKU 2017-02-17)
	PSP_DIR_ENTRY_TYPE_PSP_BOOT_TIME_TRUSTLETS_KEY = 0x0B, // PSP entry pointer to key of the boot-loaded trustlet binaries (may need to be revised later when AMD-TEE is finalized)(Required for SR-IOV SKU 2017-02-17)
	PSP_DIR_ENTRY_TYPE_SDMA0_FW		       = 0x0C, // PSP entry pointer to SDMA0 off-chip firmware
	PSP_DIR_ENTRY_TYPE_SDMA1_FW		       = 0x0D, // PSP entry pointer to SDMA1 off-chip firmware
	PSP_DIR_ENTRY_TYPE_RLCG_FW		       = 0x0E, // PSP entry pointer to RLC-G off-chip firmware
	PSP_DIR_ENTRY_TYPE_RLCV_FW		       = 0x0F, // PSP entry pointer to RLC-V off-chip firmware
	PSP_DIR_ENTRY_TYPE_MMSCHEDULER_FW	       = 0x10, // PSP entry pointer to MM scheduler firmware
	PSP_DIR_ENTRY_TYPE_MC_FW		       = 0x11, // PSP entry pointer to MC firmware
	PSP_DIR_ENTRY_TYPE_SECURITY_GASKET	       = 0x12, // PSP entry pointer to Security Policy load by PSP BL in the beginning
	PSP_DIR_ENTRY_TYPE_PSP_FW_SYSDRV	       = 0x13, // PSP entry pointer to System driver binary of AMD-TEE (Required for SR-IOV SKU 2017-02-17)
	PSP_DIR_ENTRY_TYPE_DBG_UNLOCK_BIN	       = 0x14, // PSP entry pointer to secure debug unlock binary (Need to Add to SPI-ROM)
	PSP_DIR_ENTRY_TYPE_PSP_WRAPPED_IKEK	       = 0x21, // (Already integrated 2017-02-17)
	PSP_DIR_ENTRY_TYPE_PSP_FW_DIAG_BOOT_LOADER     = 0x23,
	PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_GPM_MEM    = 0x2a,
	PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_SRM_MEM    = 0x2b,
	PSP_DIR_ENTRY_TYPE_RLC_RESTORE_LIST_CNTL       = 0x2c,
	PSP_DIR_ENTRY_TYPE_PSP_VBIOS_MODULE	       = 0x2d, // PSP entry point to VBIOS init module stage1
	PSP_DIR_ENTRY_TYPE_PSP_VBIOS_MODULE_STAGE2     = 0x2e, // PSP entry point to VBIOS init module stage2
	PSP_DIR_ENTRY_TYPE_PSP_VBIOS_MODULE_STAGE3     = 0x2f, // PSP entry point to VBIOS init module stage3
	PSP_DIR_ENTRY_TYPE_PSP_PCIE_FW		       = 0x30, // PSP entry point to WAFL/XGMI PHY FW
	PSP_DIR_ENTRY_TYPE_PSP_DPPHY_FW		       = 0x31, // PSP entry point to display PHY FW
	PSP_DIR_ENTRY_TYPE_PSP_DXIO_FW		       = 0x32, // PSP entry point to xGMI link training FW
	PSP_DIR_ENTRY_TYPE_PSP_FW_BOOT_LOADER_STAGE2   = 0x33, // PSP entry point to PSP bootloader stage2
	PSP_DIR_ENTRY_TYPE_PSP_FW_BOOT_LOADER_STAGE3   = 0x34, // PSP entry point to PSP bootloader stage3
	PSP_DIR_ENTRY_TYPE_PSP_PLATFORM_CONFIG	       = 0x35, // PSP entry point to PSP_PLATFORM_CONFIG_TABLE data structure
	PSP_DIR_ENTRY_TYPE_PSP_SEC_POLICY_STAGE2       = 0x36, // PSP entry point to product ASIC veresion security policy load by PSP BL after FB ready
	PSP_DIR_ENTRY_TYPE_PSP_IP_DISCOVERY_BIN	       = 0x37, // PSP entry point to signed binary listing all HW IP revisions. load by BL to LFB and used by SW driver
	PSP_DIR_ENTRY_TYPE_VBIOS_PUBLIC_KEY_TOKEN      = 0x38, // PSP entry pointer to VBIOS public key stored in SPI/DRAM space
	PSP_DIR_ENTRY_TYPE_WAFL_INIT		       = 0x39, // PSP entry pointer to PSP user module Wafl init
	PSP_DIR_ENTRY_TYPE_DMCU_ERAM_FW		       = 0x3a, // PSP entry pointer to DCN DMCU_ERAM FW
	PSP_DIR_ENTRY_TYPE_DMCU_ISR_FW		       = 0x3b, // PSP entry pointer to DCN DMCU_ISR FW
	PSP_DIR_ENTRY_TYPE_PCIE_ESM_MODULE	       = 0x3c, // PSP entry pointer to PSP user module PCIE ESM module
	PSP_DIR_ENTRY_TYPE_PSP_KEY_DATABASE	       = 0x3d, // PSP entry pointer to PSP key database
	PSP_DIR_ENTRY_TYPE_PSP_WHITELIST	       = 0x3E, // PSP entry pointer to PSP whitelist
	PSP_DIR_ENTRY_TYPE_PSPOS_KEY_DATABASE	       = 0x3F, // PSP entry pointer to PSPOS key database
	PSP_DIR_ENTRY_TYPE_RAS_MODULE		       = 0x40, // PSP entry pointer to PSP RAS module
	PSP_DIR_ENTRY_TYPE_DF_TOPOLOGY_TABLE	       = 0x41, // PSP entry pointer to DF XGMI TOPOLOGY TABLE
	PSP_DIR_ENTRY_TYPE_ANTI_ROLLBACK_TABLE	       = 0x42, // PSP entry pointer to PSP
	PSP_DIR_ENTRY_TYPE_PSP_IP_DISCOVERY_BIN1       = 0x43, // PSP entry pointer to old ASIC veresion security policy load by PSP BL after FB ready
	PSP_DIR_ENTRY_TYPE_BIST_TRAINING_DATA	       = 0x44, // PSP entry pointer to BIST data
	PSP_DIR_ENTRY_TYPE_END			       = 0XFF,
};

typedef struct _IFWI_GENERIC_EFS {
	uint32_t Signature;						// [0x0000]  /* Signature of Embedded Firmware Structure (0x55AA55AA) */
	uint32_t Reserved_0x04[4];				// [0x0004]  /* Reserved Set to 0x0 */
	uint32_t PspDirectoryLocation;			// [0x0014]  /* Pointer to L1 Primary directory table */
	uint32_t BiosDirectoryLocation[2];		// [0x0018]  /* Reserved on dGPUs set to 0x0 */
	uint32_t Reserved_0x20;					// [0x0020]  /* Reserved Set to 0x0 */
	uint32_t FirstGeneration;				// [0x0024]  /* the value set in RMB */
	uint32_t Reserved_0x28;					// [0x0028]  /* Reserved set to 0x0 */
	uint32_t PspDirectoryLocationBackup;	// [0x002C]  /* Pointer to L1 Secondary directory table */
	uint32_t Reserved_0x30[6];				// [0x0030]  /* Reserved Set to 0x0 */
	uint32_t PspDirIndLocation;				// [0x0048]  /* Unused on dGPU starting Navi3x, set to 0x0 */
	uint32_t RomStrapALocation;				// [0x004C]  /* Pointer to Romstrap A. Points to the cookie field of the Romstrap A */
	uint32_t RomStrapBLocation;				// [0x0050]  /* Pointer to RomStrap B. Points to the cookie field of the Romstrap B */
} IFWI_GENERIC_EFS;

typedef struct _PSP_IMAGE_SLOT_HEADER {
	uint32_t u32CheckSum;		//0x0  4	32-bit Fletcher's CRC value of the items below this field
	uint32_t u32BootPriority;	//0x4  4	This field indicates whether the image is preferred for boot
								//			selection. The image is considered to have boot priority if
								//			this field is set larger than one (1). Assuming UNBOOTABLE MASK
								//			(MSMU_PUB_SCRATCH* Register) does not indicate otherwise, note
								//			 that the partition with the higher value (>0) in the Boot Priority
								//			is the one that must be chosen as the Active Boot partition
								//			0: Unbootable partition/slot
								//			Non 0: Boot Priority
	uint32_t u32UpdateRetries;	//0x8  4	This field represents the number of boot attempts that are allowed
								//			on a newly updated partition before a partition is considered “unbootable
								//			For A/B scheme, this field may be hardcoded to 0 indicated no retries are
								//			allowed on the newly updated partition.
	uint8_t  u8GlitchRetries;	//0xC  1	Bit 7:0 Glitch retry counter, This field represents the number
								//			of boot attempts that are allowed on a working partition
								//			before an image is considered “unbootable.” For A/B recovery
								//			scheme, this counter is set 0, not allowing any retrials.
	uint16_t u16Fw_Id;			//0xD  2	Firmware ID of the partition’s PSP L2 directory in SPIROM.
								//			=0x014D for PartitionA, =0X14E for PartitionB.
	uint8_t  u8Reserved;		//0xF  1	Reserved. Unused in dGPU
	uint32_t u32Location;		//0x10 4	Absolute address of the partition’s PSP L2 directory in SPIROM.
	uint32_t u32PspId;			//0x14 4	32b Chip/PSP ID
	uint32_t u32SlotMaxSize;	//0x18 4	Maximum image size allowed to program into the slot
	uint32_t u32LocationCSM;	//0x1C 4	Reserved on APU’s This field is dGPU specific.
								//			Pointer to associated L2 partition’s CSM region this
								//			is needed for the Host Aperture offset programming
} PSP_IMAGE_SLOT_HEADER;

typedef struct _PSP_DIRECTORY_ENTRY {
	uint32_t u32Type;  // Type of PSP entry; 32 bit long
	uint32_t u32Size;  // Size of PSP Entry in bytes
	uint32_t Location; // Address of PSP Entry in SPI-ROM space
	uint32_t Reserved;
} PSP_DIRECTORY_ENTRY;

/* Fixed capacity of PSP_DIRECTORY.pspEntry. Header.TotalEntries is read from the
 * VBIOS image and MUST be clamped to this value before indexing pspEntry[].
 */
#define PSP_DIRECTORY_MAX_ENTRIES 64

typedef struct _PSP_DIRECTORY {
	PSP_DIRECTORY_HEADER Header;
	PSP_DIRECTORY_ENTRY  pspEntry[PSP_DIRECTORY_MAX_ENTRIES]; // Array of PSP entries each pointing to a binary in SPI flash.  The actual size of this array comes from the
					   // header (PSP_DIRECTORY.Header.TotalEntries)
} PSP_DIRECTORY;

typedef struct _PSP_DIRECTORY_TABLE_V2_1 {
	ATOM_COMMON_TABLE_HEADER table_header;
	PSP_DIRECTORY		 psp_directory;
} PSP_DIRECTORY_TABLE_V2_1;

typedef struct _PSP_DIRECTORY_TABLE_V2_2 {
	ATOM_COMMON_TABLE_HEADER table_header;
	uint16_t		 pspbl_customized_name_offset;
	uint16_t		 reserved[7];
	PSP_DIRECTORY		 psp_directory;
} PSP_DIRECTORY_TABLE_V2_2;

// from PSP directory table v2.3, MC uCode entry have uCode only, does not carry extra header any more, mc_io_debug array
typedef struct _PSP_DIRECTORY_TABLE_V2_3 {
	ATOM_COMMON_TABLE_HEADER table_header;
	uint16_t		 pspbl_customized_name_offset;
	uint16_t		 reserved[7];
	PSP_DIRECTORY		 psp_directory;
} PSP_DIRECTORY_TABLE_V2_3;

// from PSP directory table v2.4, support more customized FW
typedef struct _PSP_DIRECTORY_TABLE_V2_4 {
	ATOM_COMMON_TABLE_HEADER table_header;
	uint16_t		 pspbl_customized_name_offset;
	uint16_t		 pspbls2_customized_name_offset;
	uint16_t		 pspbls3_customized_name_offset;
	uint16_t		 public_key_customized_name_offset;
	uint16_t		 psp_init_customized_name_offset;
	uint16_t		 reserved[3];
	PSP_DIRECTORY		 psp_directory;
} PSP_DIRECTORY_TABLE_V2_4;

// from PSP directory table v2.5, support PSP_DIR_ENTRY_TYPE_PSP_SEC_POLICY_STAGE2, PSP_DIR_ENTRY_TYPE_PSP_IP_DISCOVERY_BIN and PSP_DIR_ENTRY_TYPE_VBIOS_PUBLIC_KEY_TOKEN
typedef struct _PSP_DIRECTORY_TABLE_V2_5 {
	ATOM_COMMON_TABLE_HEADER table_header;
	uint16_t		 pspbl_customized_name_offset;
	uint16_t		 pspbls2_customized_name_offset;
	uint16_t		 pspbls3_customized_name_offset;
	uint16_t		 public_key_customized_name_offset;
	uint16_t		 psp_init_customized_name_offset;
	uint16_t		 ip_discovery_customized_name_offset;
	uint16_t		 reserved[2];
	PSP_DIRECTORY		 psp_directory;
} PSP_DIRECTORY_TABLE_V2_5;

//==============================Data Table Portion====================================

/****************************************************************************/
// Structure used in Data.mtb
/****************************************************************************/
typedef struct _ATOM_MASTER_LIST_OF_DATA_TABLES {
	USHORT UtilityPipeLine;		 // Offest for the utility to get parser info,Don't change this position!
	USHORT MultimediaCapabilityInfo; // Only used by MM Lib,latest version 1.1, not configuable from Bios, need to include the table to build Bios
	USHORT MultimediaConfigInfo;	 // Only used by MM Lib,latest version 2.1, not configuable from Bios, need to include the table to build Bios
	USHORT StandardVESA_Timing;	 // Only used by Bios
	USHORT FirmwareInfo;		 // Shared by various SW components,latest version 1.4
	USHORT PaletteData;		 // Only used by BIOS
	USHORT LCD_Info;		 // Shared by various SW components,latest version 1.3, was called LVDS_Info
	USHORT DIGTransmitterInfo;	 // Internal used by VBIOS only version 3.1
	USHORT SMU_Info;		 // Shared by various SW components,latest version 1.1
	union {
		USHORT SupportedDevicesInfo; // Will be obsolete from R600
		USHORT PspDirectory;
	};
	USHORT GPIO_I2C_Info;	       // Shared by various SW components,latest version 1.2 will be used from R600
	USHORT VRAM_UsageByFirmware;   // Shared by various SW components,latest version 1.3 will be used from R600
	USHORT GPIO_Pin_LUT;	       // Shared by various SW components,latest version 1.1
	USHORT VESA_ToInternalModeLUT; // Only used by Bios
	USHORT GFX_Info;	       // Shared by various SW components,latest version 2.1 will be used from R600
	USHORT PowerPlayInfo;	       // Shared by various SW components,latest version 2.1,new design from R600
	USHORT GPUVirtualizationInfo;  // Will be obsolete from R600
	USHORT SaveRestoreInfo;	       // Only used by Bios
	USHORT PPLL_SS_Info;	       // Shared by various SW components,latest version 1.2, used to call SS_Info, change to new name because of int ASIC SS info
	USHORT OemInfo;		       // Defined and used by external SW, should be obsolete soon
	USHORT XTMDS_Info;	       // Will be obsolete from R600
	USHORT MclkSS_Info;	       // Shared by various SW components,latest version 1.1, only enabled when ext SS chip is used
	USHORT Object_Header;	       // Shared by various SW components,latest version 1.1
	USHORT IndirectIOAccess;       // Only used by Bios,this table position can't change at all!!
	USHORT MC_InitParameter;       // Only used by command table
	USHORT ASIC_VDDC_Info;	       // Will be obsolete from R600
	USHORT ASIC_InternalSS_Info;   // New tabel name from R600, used to be called "ASIC_MVDDC_Info"
	USHORT TV_VideoMode;	       // Only used by command table
	USHORT VRAM_Info;	       // Only used by command table, latest version 1.3
	USHORT MemoryTrainingInfo;     // Used for VBIOS and Diag utility for memory training purpose since R600. the new table rev start from 2.1
	USHORT IntegratedSystemInfo;   // Shared by various SW components
	USHORT ASIC_ProfilingInfo;     // New table name from R600, used to be called "ASIC_VDDCI_Info" for pre-R600
	USHORT VoltageObjectInfo;      // Shared by various SW components, latest version 1.1
	USHORT PowerSourceInfo;	       // Shared by various SW components, latest versoin 1.1
	USHORT ServiceInfo;
} ATOM_MASTER_LIST_OF_DATA_TABLES;

typedef struct _ATOM_MASTER_DATA_TABLE {
	ATOM_COMMON_TABLE_HEADER	sHeader;
	ATOM_MASTER_LIST_OF_DATA_TABLES ListOfDataTables;
} ATOM_MASTER_DATA_TABLE;

#ifndef _H2INC

//Please don't add or expand this bitfield structure below, this one will retire soon.!
typedef struct _ATOM_FIRMWARE_CAPABILITY {
#if ATOM_BIG_ENDIAN
	USHORT Reserved		      : 1;
	USHORT SCL2Redefined	      : 1;
	USHORT PostWithoutModeSet     : 1;
	USHORT HyperMemory_Size	      : 4;
	USHORT HyperMemory_Support    : 1;
	USHORT PPMode_Assigned	      : 1;
	USHORT WMI_SUPPORT	      : 1;
	USHORT GPUControlsBL	      : 1;
	USHORT EngineClockSS_Support  : 1;
	USHORT MemoryClockSS_Support  : 1;
	USHORT ExtendedDesktopSupport : 1;
	USHORT DualCRTC_Support	      : 1;
	USHORT FirmwarePosted	      : 1;
#else
	USHORT FirmwarePosted	      : 1;
	USHORT DualCRTC_Support	      : 1;
	USHORT ExtendedDesktopSupport : 1;
	USHORT MemoryClockSS_Support  : 1;
	USHORT EngineClockSS_Support  : 1;
	USHORT GPUControlsBL	      : 1;
	USHORT WMI_SUPPORT	      : 1;
	USHORT PPMode_Assigned	      : 1;
	USHORT HyperMemory_Support    : 1;
	USHORT HyperMemory_Size	      : 4;
	USHORT PostWithoutModeSet     : 1;
	USHORT SCL2Redefined	      : 1;
	USHORT Reserved		      : 1;
#endif
} ATOM_FIRMWARE_CAPABILITY;

typedef union _ATOM_FIRMWARE_CAPABILITY_ACCESS {
	ATOM_FIRMWARE_CAPABILITY sbfAccess;
	USHORT			 susAccess;
} ATOM_FIRMWARE_CAPABILITY_ACCESS;

#else

typedef union _ATOM_FIRMWARE_CAPABILITY_ACCESS {
	USHORT susAccess;
} ATOM_FIRMWARE_CAPABILITY_ACCESS;

#endif

typedef struct _ATOM_FIRMWARE_INFO {
	ATOM_COMMON_TABLE_HEADER	sHeader;
	ULONG				ulFirmwareRevision;
	ULONG				ulDefaultEngineClock;	    //In 10Khz unit
	ULONG				ulDefaultMemoryClock;	    //In 10Khz unit
	ULONG				ulDriverTargetEngineClock;  //In 10Khz unit
	ULONG				ulDriverTargetMemoryClock;  //In 10Khz unit
	ULONG				ulMaxEngineClockPLL_Output; //In 10Khz unit
	ULONG				ulMaxMemoryClockPLL_Output; //In 10Khz unit
	ULONG				ulMaxPixelClockPLL_Output;  //In 10Khz unit
	ULONG				ulASICMaxEngineClock;	    //In 10Khz unit
	ULONG				ulASICMaxMemoryClock;	    //In 10Khz unit
	UCHAR				ucASICMaxTemperature;
	UCHAR				ucPadding[3];		    //Don't use them
	ULONG				aulReservedForBIOS[3];	    //Don't use them
	USHORT				usMinEngineClockPLL_Input;  //In 10Khz unit
	USHORT				usMaxEngineClockPLL_Input;  //In 10Khz unit
	USHORT				usMinEngineClockPLL_Output; //In 10Khz unit
	USHORT				usMinMemoryClockPLL_Input;  //In 10Khz unit
	USHORT				usMaxMemoryClockPLL_Input;  //In 10Khz unit
	USHORT				usMinMemoryClockPLL_Output; //In 10Khz unit
	USHORT				usMaxPixelClock;	    //In 10Khz unit, Max.  Pclk
	USHORT				usMinPixelClockPLL_Input;   //In 10Khz unit
	USHORT				usMaxPixelClockPLL_Input;   //In 10Khz unit
	USHORT				usMinPixelClockPLL_Output;  //In 10Khz unit, the definitions above can't change!!!
	ATOM_FIRMWARE_CAPABILITY_ACCESS usFirmwareCapability;
	USHORT				usReferenceClock;           //In 10Khz unit
	USHORT				usPM_RTS_Location;          //RTS PM4 starting location in ROM in 1Kb unit
	UCHAR				ucPM_RTS_StreamSize;        //RTS PM4 packets in Kb unit
	UCHAR				ucDesign_ID;	            //Indicate what is the board design
	UCHAR				ucMemoryModule_ID;          //Indicate what is the board design
} ATOM_FIRMWARE_INFO;

/***********************************************************************************/
#define ATOM_MAX_FIRMWARE_VRAM_USAGE_INFO 1

typedef struct _ATOM_FIRMWARE_VRAM_RESERVE_INFO {
	ULONG  ulStartAddrUsedByFirmware;
	USHORT usFirmwareUseInKb;
	USHORT usReserved;
} ATOM_FIRMWARE_VRAM_RESERVE_INFO;

typedef struct _ATOM_VRAM_USAGE_BY_FIRMWARE {
	ATOM_COMMON_TABLE_HEADER	sHeader;
	ATOM_FIRMWARE_VRAM_RESERVE_INFO asFirmwareVramReserveInfo[ATOM_MAX_FIRMWARE_VRAM_USAGE_INFO];
} ATOM_VRAM_USAGE_BY_FIRMWARE;

// GFX_InfoTable for Polaris10/Polaris11
typedef struct _ATOM_GFX_INFO_V2_1 {
	ATOM_COMMON_TABLE_HEADER asHeader;
	UCHAR			 GfxIpMinVer;
	UCHAR			 GfxIpMajVer;
	UCHAR			 max_shader_engines;
	UCHAR			 max_tile_pipes;
	UCHAR			 max_cu_per_sh;
	UCHAR			 max_sh_per_se;
	UCHAR			 max_backends_per_se;
	UCHAR			 max_texture_channel_caches;
} ATOM_GFX_INFO_V2_1;

#define ATOM_S3_ASIC_GUI_ENGINE_HUNG	0x20000000L

/****************************************************************************/
//Portion II: Definitinos only used in Driver
/****************************************************************************/

// Macros used by driver

#define GetIndexIntoMasterTable(MasterOrData, FieldName) (((char *)(&((ATOM_MASTER_LIST_OF_##MasterOrData##_TABLES *)0)->FieldName) - (char *)0) / sizeof(USHORT))

typedef struct _VBIOS_ROM_HEADER {
	UCHAR  PciRomSignature[2];
	UCHAR  ucPciRomSizeIn512bytes;
	UCHAR  ucJumpCoreMainInitBIOS;
	USHORT usLabelCoreMainInitBIOS;
	UCHAR  PciReservedSpace[18];
	USHORT usPciDataStructureOffset;
	UCHAR  Rsvd1d_1a[4];
	char   strIbm[3];
	UCHAR  CheckSum[14];
	UCHAR  ucBiosMsgNumber;
	char   str761295520[16];
	USHORT usLabelCoreVPOSTNoMode;
	USHORT usSpecialPostOffset;
	UCHAR  ucSpeicalPostImageSizeIn512Bytes;
	UCHAR  Rsved47_45[3];
	USHORT usROM_HeaderInformationTableOffset;
	UCHAR  Rsved4f_4a[6];
	char   strBuildTimeStamp[20];
	UCHAR  ucJumpCoreXFuncFarHandler;
	USHORT usCoreXFuncFarHandlerOffset;
	UCHAR  ucRsved67;
	UCHAR  ucJumpCoreVFuncFarHandler;
	USHORT usCoreVFuncFarHandlerOffset;
	UCHAR  Rsved6d_6b[3];
	USHORT usATOM_BIOS_MESSAGE_Offset;
} VBIOS_ROM_HEADER;

#endif /* _ATOMBIOS_H */
