/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI200_PPTABLE_H
#define MI200_PPTABLE_H

#include "mi200_smu13_driver_if.h"
#include "../../atombios/atomfirmware.h"

#pragma pack(push, 1)

#define SMU_13_0_TABLE_FORMAT_REVISION                  12

//// POWERPLAYTABLE::ulPlatformCaps
#define SMU_13_0_PP_PLATFORM_CAP_POWERPLAY              0x1            // This cap indicates whether CCC need to show Powerplay page.
#define SMU_13_0_PP_PLATFORM_CAP_SBIOSPOWERSOURCE       0x2            // This cap indicates whether power source notificaiton is done by SBIOS instead of OS.
#define SMU_13_0_PP_PLATFORM_CAP_HARDWAREDC             0x4            // This cap indicates whether DC mode notificaiton is done by GPIO pin directly.
#define SMU_13_0_PP_PLATFORM_CAP_BACO                   0x8            // This cap indicates whether board supports the BACO circuitry.
#define SMU_13_0_PP_PLATFORM_CAP_MACO                   0x10           // This cap indicates whether board supports the MACO circuitry.
#define SMU_13_0_PP_PLATFORM_CAP_SHADOWPSTATE           0x20           // This cap indicates whether board supports the Shadow Pstate.

// SMU_13_0_PP_THERMALCONTROLLER - Thermal Controller Type
#define SMU_13_0_PP_THERMALCONTROLLER_NONE              0
#define SMU_13_0_PP_THERMALCONTROLLER_MI200             28

#define SMU_13_0_PP_OVERDRIVE_VERSION                   0x80           // OverDrive 8 Table Version 0.1
#define SMU_13_0_PP_POWERSAVINGCLOCK_VERSION            0x01           // Power Saving Clock Table Version 1.00

enum SMU_13_0_ODFEATURE_CAP {
    SMU_13_0_ODCAP_GFXCLK_LIMITS = 0,
    SMU_13_0_ODCAP_GFXCLK_CURVE,
    SMU_13_0_ODCAP_UCLK_MAX,
    SMU_13_0_ODCAP_POWER_LIMIT,
    SMU_13_0_ODCAP_FAN_ACOUSTIC_LIMIT,
    SMU_13_0_ODCAP_FAN_SPEED_MIN,
    SMU_13_0_ODCAP_TEMPERATURE_FAN,
    SMU_13_0_ODCAP_TEMPERATURE_SYSTEM,
    SMU_13_0_ODCAP_MEMORY_TIMING_TUNE,
    SMU_13_0_ODCAP_FAN_ZERO_RPM_CONTROL,
    SMU_13_0_ODCAP_AUTO_UV_ENGINE,
    SMU_13_0_ODCAP_AUTO_OC_ENGINE,
    SMU_13_0_ODCAP_AUTO_OC_MEMORY,
    SMU_13_0_ODCAP_FAN_CURVE,
    SMU_13_0_ODCAP_COUNT,
};

enum SMU_13_0_ODFEATURE_ID {
    SMU_13_0_ODFEATURE_GFXCLK_LIMITS        = 1 << SMU_13_0_ODCAP_GFXCLK_LIMITS,            //GFXCLK Limit feature
    SMU_13_0_ODFEATURE_GFXCLK_CURVE         = 1 << SMU_13_0_ODCAP_GFXCLK_CURVE,             //GFXCLK Curve feature
    SMU_13_0_ODFEATURE_UCLK_MAX             = 1 << SMU_13_0_ODCAP_UCLK_MAX,                 //UCLK Limit feature
    SMU_13_0_ODFEATURE_POWER_LIMIT          = 1 << SMU_13_0_ODCAP_POWER_LIMIT,              //Power Limit feature
    SMU_13_0_ODFEATURE_FAN_ACOUSTIC_LIMIT   = 1 << SMU_13_0_ODCAP_FAN_ACOUSTIC_LIMIT,       //Fan Acoustic RPM feature
    SMU_13_0_ODFEATURE_FAN_SPEED_MIN        = 1 << SMU_13_0_ODCAP_FAN_SPEED_MIN,            //Minimum Fan Speed feature
    SMU_13_0_ODFEATURE_TEMPERATURE_FAN      = 1 << SMU_13_0_ODCAP_TEMPERATURE_FAN,          //Fan Target Temperature Limit feature
    SMU_13_0_ODFEATURE_TEMPERATURE_SYSTEM   = 1 << SMU_13_0_ODCAP_TEMPERATURE_SYSTEM,       //Operating Temperature Limit feature
    SMU_13_0_ODFEATURE_MEMORY_TIMING_TUNE   = 1 << SMU_13_0_ODCAP_MEMORY_TIMING_TUNE,       //AC Timing Tuning feature
    SMU_13_0_ODFEATURE_FAN_ZERO_RPM_CONTROL = 1 << SMU_13_0_ODCAP_FAN_ZERO_RPM_CONTROL,     //Zero RPM feature
    SMU_13_0_ODFEATURE_AUTO_UV_ENGINE       = 1 << SMU_13_0_ODCAP_AUTO_UV_ENGINE,           //Auto Under Volt GFXCLK feature
    SMU_13_0_ODFEATURE_AUTO_OC_ENGINE       = 1 << SMU_13_0_ODCAP_AUTO_OC_ENGINE,           //Auto Over Clock GFXCLK feature
    SMU_13_0_ODFEATURE_AUTO_OC_MEMORY       = 1 << SMU_13_0_ODCAP_AUTO_OC_MEMORY,           //Auto Over Clock MCLK feature
    SMU_13_0_ODFEATURE_FAN_CURVE            = 1 << SMU_13_0_ODCAP_FAN_CURVE,
    SMU_13_0_ODFEATURE_COUNT                = 14,
};

#define SMU_13_0_MAX_ODFEATURE    32          //Maximum Number of OD Features

enum SMU_13_0_ODSETTING_ID {
    SMU_13_0_ODSETTING_GFXCLKFMAX = 0,
    SMU_13_0_ODSETTING_GFXCLKFMIN,
    SMU_13_0_ODSETTING_VDDGFXCURVEFREQ_P1,
    SMU_13_0_ODSETTING_VDDGFXCURVEVOLTAGE_P1,
    SMU_13_0_ODSETTING_VDDGFXCURVEFREQ_P2,
    SMU_13_0_ODSETTING_VDDGFXCURVEVOLTAGE_P2,
    SMU_13_0_ODSETTING_VDDGFXCURVEFREQ_P3,
    SMU_13_0_ODSETTING_VDDGFXCURVEVOLTAGE_P3,
    SMU_13_0_ODSETTING_UCLKFMAX,
    SMU_13_0_ODSETTING_POWERPERCENTAGE,
    SMU_13_0_ODSETTING_FANRPMMIN,
    SMU_13_0_ODSETTING_FANRPMACOUSTICLIMIT,
    SMU_13_0_ODSETTING_FANTARGETTEMPERATURE,
    SMU_13_0_ODSETTING_OPERATINGTEMPMAX,
    SMU_13_0_ODSETTING_ACTIMING,
    SMU_13_0_ODSETTING_FAN_ZERO_RPM_CONTROL,
    SMU_13_0_ODSETTING_AUTOUVENGINE,
    SMU_13_0_ODSETTING_AUTOOCENGINE,
    SMU_13_0_ODSETTING_AUTOOCMEMORY,
    SMU_13_0_ODSETTING_COUNT,
};
#define SMU_13_0_MAX_ODSETTING    32          //Maximum Number of ODSettings

struct smu_13_0_overdrive_table {
    uint8_t  revision;                                        //Revision = SMU_13_0_PP_OVERDRIVE_VERSION
    uint8_t  reserve[3];                                      //Zero filled field reserved for future use
    uint32_t feature_count;                                   //Total number of supported features
    uint32_t setting_count;                                   //Total number of supported settings
    uint8_t  cap[SMU_13_0_MAX_ODFEATURE];                     //OD feature support flags
    uint32_t max[SMU_13_0_MAX_ODSETTING];                     //default maximum settings
    uint32_t min[SMU_13_0_MAX_ODSETTING];                     //default minimum settings
};

enum SMU_13_0_PPCLOCK_ID {
    SMU_13_0_PPCLOCK_GFXCLK = 0,
    SMU_13_0_PPCLOCK_VCLK,
    SMU_13_0_PPCLOCK_DCLK,
    SMU_13_0_PPCLOCK_ECLK,
    SMU_13_0_PPCLOCK_SOCCLK,
    SMU_13_0_PPCLOCK_UCLK,
    SMU_13_0_PPCLOCK_DCEFCLK,
    SMU_13_0_PPCLOCK_DISPCLK,
    SMU_13_0_PPCLOCK_PIXCLK,
    SMU_13_0_PPCLOCK_PHYCLK,
    SMU_13_0_PPCLOCK_COUNT,
};
#define SMU_13_0_MAX_PPCLOCK      16          //Maximum Number of PP Clocks

struct smu_13_0_power_saving_clock_table {
    uint8_t  revision;                                        //Revision = SMU_13_0_PP_POWERSAVINGCLOCK_VERSION
    uint8_t  reserve[3];                                      //Zero filled field reserved for future use
    uint32_t count;                                           //power_saving_clock_count = SMU_13_0_PPCLOCK_COUNT
    uint32_t max[SMU_13_0_MAX_PPCLOCK];                       //PowerSavingClock Mode Clock Maximum array In MHz
    uint32_t min[SMU_13_0_MAX_PPCLOCK];                       //PowerSavingClock Mode Clock Minimum array In MHz
};

struct smu_13_0_powerplay_table {
      struct atom_common_table_header header;       //For Navi10, header.format_revision = 12, header.content_revision=0
      uint8_t  table_revision;                      //For Navi10, current table_revision = 1
      uint16_t table_size;                          //Driver portion table size. The offset to smc_pptable including header size
      uint32_t golden_pp_id;                        //PPGen use only: PP Table ID on the Golden Data Base
      uint32_t golden_revision;                     //PPGen use only: PP Table Revision on the Golden Data Base
      uint16_t format_id;                           //PPGen use only: PPTable for different ASICs. For Navi10 this should be 0x7D
      uint32_t platform_caps;                       //POWERPLAYABLE::ulPlatformCaps

      uint8_t  thermal_controller_type;             //one of SMU_13_0_PP_THERMALCONTROLLER

      uint16_t small_power_limit1;
      uint16_t small_power_limit2;
      uint16_t boost_power_limit;                   //For Gemini Board, when the slave adapter is in BACO mode, the master adapter will use this boost power limit instead of the default power limit to boost the power limit.
      uint16_t od_turbo_power_limit;                //Power limit setting for Turbo mode in Performance UI Tuning.
      uint16_t od_power_save_power_limit;           //Power limit setting for PowerSave/Optimal mode in Performance UI Tuning.
      uint16_t software_shutdown_temp;

      uint16_t reserve[6];                          //Zero filled field reserved for future use

      struct smu_13_0_power_saving_clock_table      power_saving_clock;
      struct smu_13_0_overdrive_table               overdrive_table;

      PPTable_t smc_pptable;                        //PPTable_t in smu11_driver_if.h
};

#pragma pack(pop)

#endif
