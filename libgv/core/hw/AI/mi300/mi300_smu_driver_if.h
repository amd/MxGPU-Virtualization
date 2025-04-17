/* Copyright (C) 2010-2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef DRIVER_IF_MI300_H
#define DRIVER_IF_MI300_H

#define DRIVER_IF_MI300_VERSION 0x08042027
#define UMC__NUM_TOTAL_UMC_CHANNELS 32

//I2C Interface
#define MAX_SW_I2C_COMMANDS                24

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
  CODE_DAGB0,
  CODE_EA0 = 5,
  CODE_UTCL2_ROUTER = 10,
  CODE_VML2,
  CODE_VML2_WALKER,
  CODE_MMCANE,

  // VCN
  // VCN VCPU
  CODE_VIDD,
  CODE_VIDV,
  // VCN JPEG
  CODE_JPEG0S,
  CODE_JPEG0D,
  CODE_JPEG1S,
  CODE_JPEG1D,
  CODE_JPEG2S,
  CODE_JPEG2D,
  CODE_JPEG3S,
  CODE_JPEG3D,
  CODE_JPEG4S,
  CODE_JPEG4D,
  CODE_JPEG5S,
  CODE_JPEG5D,
  CODE_JPEG6S,
  CODE_JPEG6D,
  CODE_JPEG7S,
  CODE_JPEG7D,
  // VCN MMSCH
  CODE_MMSCHD,

  // SDMA
  CODE_SDMA0,
  CODE_SDMA1,
  CODE_SDMA2,
  CODE_SDMA3,

  // SOC
  CODE_HDP,
  CODE_ATHUB,
  CODE_IH,
  CODE_XHUB_POISON,
  CODE_SMN_SLVERR,
  CODE_WDT,

  CODE_UNKNOWN,
  CODE_COUNT,
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

// Defines used for IH-based interrupts to GFX driver - A/X only
#define IH_INTERRUPT_ID_TO_DRIVER                  0xFE
#define IH_INTERRUPT_CONTEXT_ID_THERMAL_THROTTLING 0x7
#define IH_INTERRUPT_VFFLR_INT                     0xA

//thermal over-temp mask defines for IH interrup to host
#define THROTTLER_PROCHOT_BIT           0
#define THROTTLER_RESERVED              1
#define THROTTLER_THERMAL_SOCKET_BIT    2//AID, XCD, CCD throttling
#define THROTTLER_THERMAL_VR_BIT        3//VRHOT
#define THROTTLER_THERMAL_HBM_BIT       4

#define ClearMcaOnRead_UE_FLAG_MASK              0x1
#define ClearMcaOnRead_CE_POLL_MASK              0x2

#endif
