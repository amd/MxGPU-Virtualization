/*
 * Copyright (c) 2006-2021 Advanced Micro Devices, Inc. All rights reserved.
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
