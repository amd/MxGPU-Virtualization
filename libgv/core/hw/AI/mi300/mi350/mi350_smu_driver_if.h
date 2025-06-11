/* Copyright (C) 2010-2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DRIVER_IF_MI350_H
#define DRIVER_IF_MI350_H

#define DRIVER_IF_MI350_VERSION 0x00860000
#define UMC__NUM_TOTAL_UMC_CHANNELS 32

//I2C Interface
#define MAX_SW_I2C_COMMANDS                32

typedef enum {
  I2C_CONTROLLER_PORT_0, //CKSVII2C0
  //I2C_CONTROLLER_PORT_1, //CKSVII2C1
  I2C_CONTROLLER_PORT_COUNT,
} I2cControllerPort_e;

typedef enum {
  UNSUPPORTED_1,              //50  Kbits/s not supported anymore!
  I2C_SPEED_STANDARD_100K,    //100 Kbits/s
  I2C_SPEED_FAST_400K,        //400 Kbits/s
  I2C_SPEED_FAST_PLUS_1M,     //1   Mbits/s (in fast mode)
  UNSUPPORTED_2,              //1   Mbits/s (in high speed mode)  not supported anymore!
  UNSUPPORTED_3,              //2.3 Mbits/s  not supported anymore!
  I2C_SPEED_COUNT,
} I2cSpeed_e;

#define CMDCONFIG_STOP_BIT             0
#define CMDCONFIG_RESTART_BIT          1
#define CMDCONFIG_READWRITE_BIT        2 //bit should be 0 for read, 1 for write

#define CMDCONFIG_STOP_MASK           (1 << CMDCONFIG_STOP_BIT)
#define CMDCONFIG_RESTART_MASK        (1 << CMDCONFIG_RESTART_BIT)
#define CMDCONFIG_READWRITE_MASK      (1 << CMDCONFIG_READWRITE_BIT)

// MP1 AID MCA Error Codes
typedef enum {
  // MMHUB
  CODE_DAGB0        = 0,
  CODE_DAGB1        = 1,
  CODE_DAGB2        = 2,
  CODE_DAGB3        = 3,
  CODE_DAGB4        = 4,
  CODE_EA0          = 5,
  CODE_EA1          = 6,
  CODE_EA2          = 7,
  CODE_EA3          = 8,
  CODE_EA4          = 9,
  CODE_UTCL2_ROUTER = 10,
  CODE_VML2         = 11,
  CODE_VML2_WALKER  = 12,
  CODE_MMCANE       = 13,

  // VCN VCPU
  CODE_VIDD         = 14,
  CODE_VIDV         = 15,
  // VCN JPEG
  CODE_JPEG0S       = 16,
  CODE_JPEG0D       = 17,
  CODE_JPEG1S       = 18,
  CODE_JPEG1D       = 19,
  CODE_JPEG2S       = 20,
  CODE_JPEG2D       = 21,
  CODE_JPEG3S       = 22,
  CODE_JPEG3D       = 23,
  CODE_JPEG4S       = 24,
  CODE_JPEG4D       = 25,
  CODE_JPEG5S       = 26,
  CODE_JPEG5D       = 27,
  CODE_JPEG6S       = 28,
  CODE_JPEG6D       = 29,
  CODE_JPEG7S       = 30,
  CODE_JPEG7D       = 31,
  // VCN MMSCH
  CODE_MMSCHD       = 32,

  // SDMA
  CODE_SDMA0        = 33,
  CODE_SDMA1        = 34,
  CODE_SDMA2        = 35,
  CODE_SDMA3        = 36,

  // SOC
  CODE_HDP          = 37,
  CODE_ATHUB        = 38,
  CODE_IH           = 39,
  CODE_XHUB_POISON  = 40,
  CODE_SMN_SLVERR   = 41,
  CODE_WDT          = 42,

  CODE_UNKNOWN      = 43,
  CODE_DMA          = 44,

  CODE_USR_GASKET_ECC       = 45,
  CODE_USR_GASKET_OVERFLOW  = 46,

  CODE_COUNT        = 47,
} ERR_CODE_e;

typedef struct {
  uint8_t ReadWriteData;  //Return data for read. Data to send for write
  uint8_t CmdConfig; //Includes whether associated command should have a stop or restart command, and is a read or write
} SwI2cCmd_t; //SW I2C Command Table

typedef struct {
  uint8_t    I2CcontrollerPort; //CKSVII2C0(0) or //CKSVII2C1(1)
  uint8_t    I2CSpeed;          //Use I2cSpeed_e to indicate speed to select
  uint8_t    SlaveAddress;      //Slave address of device
  uint8_t    NumCmds;           //Number of commands
  SwI2cCmd_t SwI2cCmds[MAX_SW_I2C_COMMANDS];
} SwI2cRequest_t; // SW I2C Request Table

typedef enum {
  PPCLK_VCLK,
  PPCLK_DCLK,
  PPCLK_SOCCLK,
  PPCLK_UCLK,
  PPCLK_FCLK,
  PPCLK_LCLK,
  PPCLK_COUNT,
} PPCLK_e;

typedef struct {
  uint64_t mca_umc_status;
  uint64_t mca_umc_addr;

  uint16_t ce_count_lo_chip;
  uint16_t ce_count_hi_chip;

  uint32_t eccPadding;

  uint64_t mca_ceumc_addr;
} EccInfo_t;

typedef struct {
  EccInfo_t  EccInfo[UMC__NUM_TOTAL_UMC_CHANNELS];
} EccInfoTable_t;

// Defines used for IH-based thermal interrupts to GFX driver - A/X only
#define IH_INTERRUPT_ID_TO_DRIVER                   0xFE
#define IH_INTERRUPT_CONTEXT_ID_THERMAL_THROTTLING  0x7
#define IH_INTERRUPT_VFFLR_INT                      0xA

//thermal over-temp mask defines for IH interrup to host
#define THROTTLER_PROCHOT_BIT           0
#define THROTTLER_RESERVED              1
#define THROTTLER_THERMAL_SOCKET_BIT    2//AID, XCD, CCD throttling
#define THROTTLER_THERMAL_VR_BIT        3//VRHOT
#define THROTTLER_THERMAL_HBM_BIT       4

#define ClearMcaOnRead_UE_FLAG_MASK              0x1  // UEs are always reported, set flag to 0 to prevent clearing of UEs
#define ClearMcaOnRead_CE_POLL_MASK              0x2  // Enable CE logging and clearing to driver

#endif
