/*
 * Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __AMDSMI_H__
#define __AMDSMI_H__

/**
 * @file amdsmi.h
 * @brief AMD System Management Interface API
 */

#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#endif

/**
 * @brief Initialization flags
 *
 * Initialization flags may be OR'd together and passed to ::amdsmi_init().
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{cpu_bm} @tag{guest_windows} @endcond
 */
typedef enum {
    AMDSMI_INIT_ALL_PROCESSORS = 0xFFFFFFFF,  //!< Initialize all processors
    AMDSMI_INIT_AMD_CPUS       = (1 << 0),    //!< Initialize AMD CPUS
    AMDSMI_INIT_AMD_GPUS       = (1 << 1),    //!< Initialize AMD GPUS
    AMDSMI_INIT_NON_AMD_CPUS   = (1 << 2),    //!< Initialize Non-AMD CPUS
    AMDSMI_INIT_NON_AMD_GPUS   = (1 << 3),    //!< Initialize Non-AMD GPUS
    AMDSMI_INIT_AMD_APUS       = (AMDSMI_INIT_AMD_CPUS | AMDSMI_INIT_AMD_GPUS) /**< Initialize AMD CPUS and GPUS
                                                                                    (Default option) */
} amdsmi_init_flags_t;

/**
 * @brief opaque handler point to underlying implementation
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{cpu_bm} @tag{guest_windows} @endcond
 */
typedef void *amdsmi_processor_handle;
typedef void *amdsmi_socket_handle;

/**
 * @brief Error codes returned by amdsmi functions
 *
 * Please avoid status codes that are multiples of 256 (256, 512, etc..)
 * Return values in the shell get modulo 256 applied, meaning any multiple of 256 ends up as 0
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{cpu_bm} @tag{guest_windows} @endcond
 */
typedef enum {
    AMDSMI_STATUS_SUCCESS = 0,              //!< Call succeeded
    // Library usage errors
    AMDSMI_STATUS_INVAL = 1,                //!< Invalid parameters
    AMDSMI_STATUS_NOT_SUPPORTED = 2,        //!< Command not supported
    AMDSMI_STATUS_NOT_YET_IMPLEMENTED = 3,  //!< Not implemented yet
    AMDSMI_STATUS_FAIL_LOAD_MODULE = 4,     //!< Fail to load lib
    AMDSMI_STATUS_FAIL_LOAD_SYMBOL = 5,     //!< Fail to load symbol
    AMDSMI_STATUS_DRM_ERROR = 6,            //!< Error when call libdrm
    AMDSMI_STATUS_API_FAILED = 7,           //!< API call failed
    AMDSMI_STATUS_TIMEOUT = 8,              //!< Timeout in API call
    AMDSMI_STATUS_RETRY = 9,                //!< Retry operation
    AMDSMI_STATUS_NO_PERM = 10,             //!< Permission Denied
    AMDSMI_STATUS_INTERRUPT = 11,           //!< An interrupt occurred during execution of function
    AMDSMI_STATUS_IO = 12,                  //!< I/O Error
    AMDSMI_STATUS_ADDRESS_FAULT = 13,       //!< Bad address
    AMDSMI_STATUS_FILE_ERROR = 14,          //!< Problem accessing a file
    AMDSMI_STATUS_OUT_OF_RESOURCES = 15,    //!< Not enough memory
    AMDSMI_STATUS_INTERNAL_EXCEPTION = 16,  //!< An internal exception was caught
    AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS = 17, //!< The provided input is out of allowable or safe range
    AMDSMI_STATUS_INIT_ERROR = 18,          //!< An error occurred when initializing internal data structures
    AMDSMI_STATUS_REFCOUNT_OVERFLOW = 19,   //!< An internal reference counter exceeded INT32_MAX
    AMDSMI_STATUS_DIRECTORY_NOT_FOUND = 20, //!< Error when a directory is not found, maps to ENOTDIR
    // Processor related errors
    AMDSMI_STATUS_BUSY = 30,                //!< Processor busy
    AMDSMI_STATUS_NOT_FOUND = 31,           //!< Processor Not found
    AMDSMI_STATUS_NOT_INIT = 32,            //!< Processor not initialized
    AMDSMI_STATUS_NO_SLOT = 33,             //!< No more free slot
    AMDSMI_STATUS_DRIVER_NOT_LOADED = 34,   //!< Processor driver not loaded
    // Data and size errors
    AMDSMI_STATUS_MORE_DATA = 39,           //!< There is more data than the buffer size the user passed
    AMDSMI_STATUS_NO_DATA = 40,             //!< No data was found for a given input
    AMDSMI_STATUS_INSUFFICIENT_SIZE = 41,   //!< Not enough resources were available for the operation
    AMDSMI_STATUS_UNEXPECTED_SIZE = 42,     //!< An unexpected amount of data was read
    AMDSMI_STATUS_UNEXPECTED_DATA = 43,     //!< The data read or provided to function is not what was expected
    //esmi errors
    AMDSMI_STATUS_NON_AMD_CPU = 44,         //!< System has different cpu than AMD
    AMDSMI_STATUS_NO_ENERGY_DRV = 45,       //!< Energy driver not found
    AMDSMI_STATUS_NO_MSR_DRV = 46,          //!< MSR driver not found
    AMDSMI_STATUS_NO_HSMP_DRV = 47,         //!< HSMP driver not found
    AMDSMI_STATUS_NO_HSMP_SUP = 48,         //!< HSMP not supported
    AMDSMI_STATUS_NO_HSMP_MSG_SUP = 49,     //!< HSMP message/feature not supported
    AMDSMI_STATUS_HSMP_TIMEOUT = 50,        //!< HSMP message timed out
    AMDSMI_STATUS_NO_DRV = 51,              //!< No Energy and HSMP driver present
    AMDSMI_STATUS_FILE_NOT_FOUND = 52,      //!< file or directory not found
    AMDSMI_STATUS_ARG_PTR_NULL = 53,        //!< Parsed argument is invalid
    AMDSMI_STATUS_AMDGPU_RESTART_ERR = 54,  //!< AMDGPU restart failed
    AMDSMI_STATUS_SETTING_UNAVAILABLE = 55, //!< Setting is not available
    AMDSMI_STATUS_CORRUPTED_EEPROM = 56,    //!< EEPROM is corrupted
    // General errors
    AMDSMI_STATUS_MAP_ERROR = 0xFFFFFFFE,     //!< The internal library error did not map to a status code
    AMDSMI_STATUS_UNKNOWN_ERROR = 0xFFFFFFFF, //!< An unknown error occurred
} amdsmi_status_t;

/**
 * @brief Processor types detectable by AMD SMI
 *
 * AMDSMI_PROCESSOR_TYPE_AMD_CPU      - CPU Socket is a physical component that holds the CPU.
 * AMDSMI_PROCESSOR_TYPE_AMD_CPU_CORE - CPU Cores are number of individual processing units within the CPU.
 * AMDSMI_PROCESSOR_TYPE_AMD_APU      - Combination of AMDSMI_PROCESSOR_TYPE_AMD_CPU and integrated GPU on single die
 * AMDSMI_PROCESSOR_TYPE_AMD_NIC      - Network Interface Card
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{cpu_bm} @tag{guest_windows} @endcond
 */
typedef enum {
    AMDSMI_PROCESSOR_TYPE_UNKNOWN = 0,   //!< Unknown processor type
    AMDSMI_PROCESSOR_TYPE_AMD_GPU,       //!< AMD Graphics processor type
    AMDSMI_PROCESSOR_TYPE_AMD_CPU,       //!< AMD CPU processor type
    AMDSMI_PROCESSOR_TYPE_NON_AMD_GPU,   //!< Non-AMD Graphics processor type
    AMDSMI_PROCESSOR_TYPE_NON_AMD_CPU,   //!< Non-AMD CPU processor type
    AMDSMI_PROCESSOR_TYPE_AMD_CPU_CORE,  //!< AMD CPU-Core processor type
    AMDSMI_PROCESSOR_TYPE_AMD_APU,       //!< AMD Accelerated processor type (GPU and CPU)
    AMDSMI_PROCESSOR_TYPE_AMD_NIC        //!< AMD Network Interface Card processor type
} amdsmi_processor_type_t;

/**
 * @brief Maximum size definitions
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
#define AMDSMI_MAX_MM_IP_COUNT            8   //!< Maximum number of multimedia IP blocks
#define AMDSMI_MAX_DEVICES                32  //!< Maximum number of devices supported
#define AMDSMI_MAX_STRING_LENGTH          256 //!< Maximum length for string buffers
#define AMDSMI_MAX_CACHE_TYPES            10  //!< Maximum number of cache types
#define AMDSMI_MAX_CP_PROFILE_RESOURCES   32  //!< Maximum number of compute profile resources
#define AMDSMI_MAX_ACCELERATOR_PARTITIONS 8   //!< Maximum number of accelerator partitions
#define AMDSMI_MAX_ACCELERATOR_PROFILE    32  //!< Maximum number of accelerator profiles
#define AMDSMI_MAX_NUM_NUMA_NODES         32  //!< Maximum number of NUMA nodes
#define AMDSMI_GPU_UUID_SIZE              38  //!< Size of GPU UUID string

/**
 * @brief Max Number of AFIDs that will be inside one cper entry
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
#define MAX_NUMBER_OF_AFIDS_PER_RECORD 12 //!< Maximum AFIDs per CPER record

/**
 * @brief String format
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
#define AMDSMI_TIME_FORMAT "%02d:%02d:%02d.%03d"                //!< Time format string
#define AMDSMI_DATE_FORMAT "%04d-%02d-%02d:%02d:%02d:%02d.%03d" //!< Date format string

/**
 * @brief Clock types
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
typedef enum {
    AMDSMI_CLK_TYPE_SYS = 0x0,                      //!< System clock
    AMDSMI_CLK_TYPE_FIRST = AMDSMI_CLK_TYPE_SYS,
    AMDSMI_CLK_TYPE_GFX = AMDSMI_CLK_TYPE_SYS,      //!< Graphics clock
    AMDSMI_CLK_TYPE_DF,                             /**< Data Fabric clock (for ASICs
                                                         running on a separate clock) */
    AMDSMI_CLK_TYPE_DCEF,                           /**< Display Controller Engine Front clock,
                                                         timing/bandwidth signals to display */
    AMDSMI_CLK_TYPE_SOC,                            //!< System On Chip clock, integrated circuit frequency
    AMDSMI_CLK_TYPE_MEM,                            //!< Memory clock speed, system operating frequency
    AMDSMI_CLK_TYPE_PCIE,                           //!< PCI Express clock, high bandwidth peripherals
    AMDSMI_CLK_TYPE_VCLK0,                          //!< Video 0 clock, video processing units
    AMDSMI_CLK_TYPE_VCLK1,                          //!< Video 1 clock, video processing units
    AMDSMI_CLK_TYPE_DCLK0,                          //!< Display 1 clock, timing signals for display output
    AMDSMI_CLK_TYPE_DCLK1,                          //!< Display 2 clock, timing signals for display output
    AMDSMI_CLK_TYPE__MAX = AMDSMI_CLK_TYPE_DCLK1
} amdsmi_clk_type_t;

/**
 * @brief This enumeration is used to indicate from which part of the processor a
 * temperature reading should be obtained.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
typedef enum {
    AMDSMI_TEMPERATURE_TYPE_EDGE,    //!< Edge temperature
    AMDSMI_TEMPERATURE_TYPE_FIRST = AMDSMI_TEMPERATURE_TYPE_EDGE,
    AMDSMI_TEMPERATURE_TYPE_HOTSPOT, //!< Hottest temperature reported for entire die
    AMDSMI_TEMPERATURE_TYPE_JUNCTION = AMDSMI_TEMPERATURE_TYPE_HOTSPOT, //!< Synonymous with HOTSPOT
    AMDSMI_TEMPERATURE_TYPE_VRAM,    //!< VRAM temperature on graphics card
    AMDSMI_TEMPERATURE_TYPE_HBM_0,   //!< High Bandwidth 0 temperature per stack
    AMDSMI_TEMPERATURE_TYPE_HBM_1,   //!< High Bandwidth 1 temperature per stack
    AMDSMI_TEMPERATURE_TYPE_HBM_2,   //!< High Bandwidth 2 temperature per stack
    AMDSMI_TEMPERATURE_TYPE_HBM_3,   //!< High Bandwidth 3 temperature per stack
    AMDSMI_TEMPERATURE_TYPE_PLX,     //!< PCIe switch temperature
    AMDSMI_TEMPERATURE_TYPE__MAX = AMDSMI_TEMPERATURE_TYPE_PLX
} amdsmi_temperature_type_t;

/**
 * @brief Temperature Metrics. This enum is used to identify various
 * temperature metrics. Corresponding values will be in Celcius
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_TEMP_CURRENT = 0x0,   //!< Current temperature
    AMDSMI_TEMP_FIRST = AMDSMI_TEMP_CURRENT,
    AMDSMI_TEMP_MAX,             //!< Max temperature
    AMDSMI_TEMP_MIN,             //!< Min temperature
    AMDSMI_TEMP_MAX_HYST,        /**< Max limit hysteresis temperature
                                      (Absolute temperature, not a delta) */
    AMDSMI_TEMP_MIN_HYST,        /**< Min limit hysteresis temperature
                                      (Absolute temperature, not a delta) */
    AMDSMI_TEMP_CRITICAL,        /**< Critical max limit temperature, typically
                                      greater than max temperatures */
    AMDSMI_TEMP_CRITICAL_HYST,   /**< Critical hysteresis limit temperature
                                      (Absolute temperature, not a delta) */
    AMDSMI_TEMP_EMERGENCY,       /**< Emergency max temperature, for chips
                                      supporting more than two upper temperature
                                      limits. Must be equal or greater than
                                      corresponding temp_crit values */
    AMDSMI_TEMP_EMERGENCY_HYST,  /**< Emergency hysteresis limit temperature
                                      (Absolute temperature, not a delta) */
    AMDSMI_TEMP_CRIT_MIN,        /**< Critical min temperature, typically
                                      lower than minimum temperatures */
    AMDSMI_TEMP_CRIT_MIN_HYST,   /**< Min Hysteresis critical limit temperature
                                      (Absolute temperature, not a delta) */
    AMDSMI_TEMP_OFFSET,          /**< Temperature offset which is added to the
                                      temperature reading by the chip */
    AMDSMI_TEMP_LOWEST,          //!< Historical min temperature
    AMDSMI_TEMP_HIGHEST,         //!< Historical max temperature
    AMDSMI_TEMP_SHUTDOWN,        //!< Shutdown temperature
    AMDSMI_TEMP_LAST = AMDSMI_TEMP_SHUTDOWN
} amdsmi_temperature_metric_t;

/**
 * @brief Card Form Factor
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_CARD_FORM_FACTOR_PCIE,    //!< PCIE card form factor
    AMDSMI_CARD_FORM_FACTOR_OAM,     //!< OAM form factor
    AMDSMI_CARD_FORM_FACTOR_CEM,     //!< CEM form factor
    AMDSMI_CARD_FORM_FACTOR_UNKNOWN  //!< Unknown Form factor
} amdsmi_card_form_factor_t;

/**
 * @brief The values of this enum are used to identify the various firmware
 * blocks.
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_FW_ID_SMU = 1,                   /**< System Management Unit (power management,
                                                 clock control, thermal monitoring, etc...) */
    AMDSMI_FW_ID_FIRST = AMDSMI_FW_ID_SMU,
    AMDSMI_FW_ID_CP_CE,                     //!< Compute Processor - Command_Engine (fetch, decode, dispatch)
    AMDSMI_FW_ID_CP_PFP,                    //!< Compute Processor - Pixel Front End Processor (pixelating process)
    AMDSMI_FW_ID_CP_ME,                     //!< Compute Processor - Micro Engine (specialize processing)
    AMDSMI_FW_ID_CP_MEC_JT1,                //!< Compute Processor - Micro Engine Controler Job Table 1 (queues, scheduling)
    AMDSMI_FW_ID_CP_MEC_JT2,                //!< Compute Processor - Micro Engine Controler Job Table 2 (queues, scheduling)
    AMDSMI_FW_ID_CP_MEC1,                   //!< Compute Processor - Micro Engine Controler 1 (scheduling, managing resources)
    AMDSMI_FW_ID_CP_MEC2,                   //!< Compute Processor - Micro Engine Controler 2 (scheduling, managing resources)
    AMDSMI_FW_ID_RLC,                       //!< Rasterizer and L2 Cache (rasterization processs)
    AMDSMI_FW_ID_SDMA0,                     //!< System Direct Memory Access 0 (high speed data transfers)
    AMDSMI_FW_ID_SDMA1,                     //!< System Direct Memory Access 1 (high speed data transfers)
    AMDSMI_FW_ID_SDMA2,                     //!< System Direct Memory Access 2 (high speed data transfers)
    AMDSMI_FW_ID_SDMA3,                     //!< System Direct Memory Access 3 (high speed data transfers)
    AMDSMI_FW_ID_SDMA4,                     //!< System Direct Memory Access 4 (high speed data transfers)
    AMDSMI_FW_ID_SDMA5,                     //!< System Direct Memory Access 5 (high speed data transfers)
    AMDSMI_FW_ID_SDMA6,                     //!< System Direct Memory Access 6 (high speed data transfers)
    AMDSMI_FW_ID_SDMA7,                     //!< System Direct Memory Access 7 (high speed data transfers)
    AMDSMI_FW_ID_VCN,                       //!< Video Core Next (encoding and decoding)
    AMDSMI_FW_ID_UVD,                       //!< Unified Video Decoder (decode specific video formats)
    AMDSMI_FW_ID_VCE,                       //!< Video Coding Engine (Encoding video)
    AMDSMI_FW_ID_ISP,                       //!< Image Signal Processor (processing raw image data from sensors)
    AMDSMI_FW_ID_DMCU_ERAM,                 //!< Digital Micro Controller Unit - Embedded RAM (memory used by DMU)
    AMDSMI_FW_ID_DMCU_ISR,                  //!< Digital Micro Controller Unit - Interrupt Service Routine (interrupt handlers)
    AMDSMI_FW_ID_RLC_RESTORE_LIST_GPM_MEM,  //!< Rasterizier and L2 Cache Restore List Graphics Processor Memory
    AMDSMI_FW_ID_RLC_RESTORE_LIST_SRM_MEM,  //!< Rasterizier and L2 Cache Restore List System RAM Memory
    AMDSMI_FW_ID_RLC_RESTORE_LIST_CNTL,     //!< Rasterizier and L2 Cache Restore List Control
    AMDSMI_FW_ID_RLC_V,                     //!< Rasterizier and L2 Cache Virtual memory
    AMDSMI_FW_ID_MMSCH,                     //!< Multi-Media Shader Hardware Scheduler
    AMDSMI_FW_ID_PSP_SYSDRV,                //!< Platform Security Processor System Driver
    AMDSMI_FW_ID_PSP_SOSDRV,                //!< Platform Security Processor Secure Operating System Driver
    AMDSMI_FW_ID_PSP_TOC,                   //!< Platform Security Processor Table of Contents
    AMDSMI_FW_ID_PSP_KEYDB,                 //!< Platform Security Processor Table of Contents
    AMDSMI_FW_ID_DFC,                       //!< Data Fabric Controler (bandwidth and coherency)
    AMDSMI_FW_ID_PSP_SPL,                   //!< Platform Security Processor Secure Program Loader
    AMDSMI_FW_ID_DRV_CAP,                   //!< Driver Capabilities (capabilities, features)
    AMDSMI_FW_ID_MC,                        //!< Memory Contoller (RAM and VRAM)
    AMDSMI_FW_ID_PSP_BL,                    //!< Platform Security Processor Bootloader (initial firmware)
    AMDSMI_FW_ID_CP_PM4,                    //!< Compute Processor Packet Processor 4 (processing command packets)
    AMDSMI_FW_ID_RLC_P,                     //!< Rasterizier and L2 Cache Partition
    AMDSMI_FW_ID_SEC_POLICY_STAGE2,         //!< Security Policy Stage 2 (security features)
    AMDSMI_FW_ID_REG_ACCESS_WHITELIST,      //!< Register Access Whitelist (Prevent unathorizied access)
    AMDSMI_FW_ID_IMU_DRAM,                  //!< Input/Output Memory Management Unit - Dynamic RAM
    AMDSMI_FW_ID_IMU_IRAM,                  //!< Input/Output Memory Management Unit - Instruction RAM
    AMDSMI_FW_ID_SDMA_TH0,                  //!< System Direct Memory Access - Thread Handler 0
    AMDSMI_FW_ID_SDMA_TH1,                  //!< System Direct Memory Access - Thread Handler 1
    AMDSMI_FW_ID_CP_MES,                    //!< Compute Processor - Micro Engine Scheduler
    AMDSMI_FW_ID_MES_KIQ,                   //!< Micro Engine Scheduler - Kernel Indirect Queue
    AMDSMI_FW_ID_MES_STACK,                 //!< Micro Engine Scheduler - Stack
    AMDSMI_FW_ID_MES_THREAD1,               //!< Micro Engine Scheduler - Thread 1
    AMDSMI_FW_ID_MES_THREAD1_STACK,         //!< Micro Engine Scheduler - Thread 1 Stack
    AMDSMI_FW_ID_RLX6,                      //!< Hardware Block RLX6
    AMDSMI_FW_ID_RLX6_DRAM_BOOT,            //!< Hardware Block RLX6 - Dynamic Ram Boot
    AMDSMI_FW_ID_RS64_ME,                   //!< Hardware Block RS64 - Micro Engine
    AMDSMI_FW_ID_RS64_ME_P0_DATA,           //!< Hardware Block RS64 - Micro Engine Partition 0 Data
    AMDSMI_FW_ID_RS64_ME_P1_DATA,           //!< Hardware Block RS64 - Micro Engine Partition 1 Data
    AMDSMI_FW_ID_RS64_PFP,                  //!< Hardware Block RS64 - Pixel Front End Processor
    AMDSMI_FW_ID_RS64_PFP_P0_DATA,          //!< Hardware Block RS64 - Pixel Front End Processor Partition 0 Data
    AMDSMI_FW_ID_RS64_PFP_P1_DATA,          //!< Hardware Block RS64 - Pixel Front End Processor Partition 1 Data
    AMDSMI_FW_ID_RS64_MEC,                  //!< Hardware Block RS64 - Micro Engine Controller
    AMDSMI_FW_ID_RS64_MEC_P0_DATA,          //!< Hardware Block RS64 - Micro Engine Controller Partition 0 Data
    AMDSMI_FW_ID_RS64_MEC_P1_DATA,          //!< Hardware Block RS64 - Micro Engine Controller Partition 1 Data
    AMDSMI_FW_ID_RS64_MEC_P2_DATA,          //!< Hardware Block RS64 - Micro Engine Controller Partition 2 Data
    AMDSMI_FW_ID_RS64_MEC_P3_DATA,          //!< Hardware Block RS64 - Micro Engine Controller Partition 3 Data
    AMDSMI_FW_ID_PPTABLE,                   //!< Power Policy Table (power management policies)
    AMDSMI_FW_ID_PSP_SOC,                   //!< Platform Security Processor - System On a Chip
    AMDSMI_FW_ID_PSP_DBG,                   //!< Platform Security Processor - Debug
    AMDSMI_FW_ID_PSP_INTF,                  //!< Platform Security Processor - Interface
    AMDSMI_FW_ID_RLX6_CORE1,                //!< Hardware Block RLX6 - Core 1
    AMDSMI_FW_ID_RLX6_DRAM_BOOT_CORE1,      //!< Hardware Block RLX6 Core 1 - Dynamic RAM Boot
    AMDSMI_FW_ID_RLCV_LX7,                  //!< Hardware Block RLCV - Subsystem LX7
    AMDSMI_FW_ID_RLC_SAVE_RESTORE_LIST,     //!< Rasterizier and L2 Cache - Save Restore List
    AMDSMI_FW_ID_ASD,                       //!< Asynchronous Shader Dispatcher
    AMDSMI_FW_ID_TA_RAS,                    //!< Trusted Applications - Reliablity Availability and Serviceability
    AMDSMI_FW_ID_TA_XGMI,                   //!< Trusted Applications - Reliablity XGMI
    AMDSMI_FW_ID_XGMI,                      //!< XGMI (Interconnect) Firmware
    AMDSMI_FW_ID_RLC_SRLG,                  //!< Rasterizier and L2 Cache - Shared Resource Local Group
    AMDSMI_FW_ID_RLC_SRLS,                  //!< Rasterizier and L2 Cache - Shared Resource Local Segment
    AMDSMI_FW_ID_PM,                        //!< Power Management Firmware
    AMDSMI_FW_ID_SMC,                       //!< System Management Controller Firmware
    AMDSMI_FW_ID_DMCU,                      //!< Display Micro-Controller Unit
    AMDSMI_FW_ID_PSP_RAS,                   //!< Platform Security Processor - Reliability, Availability, and Serviceability Firmware
    AMDSMI_FW_ID_P2S_TABLE,                 //!< Processor-to-System Table Firmware
    AMDSMI_FW_ID_PLDM_BUNDLE,               //!< Platform Level Data Model Firmware Bundle
    AMDSMI_FW_ID__MAX
} amdsmi_fw_block_t;

/**
 * @brief Variant placeholder
 *
 * Place-holder "variant" for functions that have don't have any variants,
 * but do have monitors or sensors.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
typedef enum {
    AMDSMI_VIRTUALIZATION_MODE_UNKNOWN = 0,  //!< Unknown Virtualization Mode
    AMDSMI_VIRTUALIZATION_MODE_BAREMETAL,    //!< Baremetal Virtualization Mode
    AMDSMI_VIRTUALIZATION_MODE_HOST,         //!< Host Virtualization Mode
    AMDSMI_VIRTUALIZATION_MODE_GUEST,        //!< Guest Virtualization Mode
    AMDSMI_VIRTUALIZATION_MODE_PASSTHROUGH   //!< Passthrough Virtualization Mode
} amdsmi_virtualization_mode_t;

/**
 * @brief bdf types
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
typedef union {
    struct bdf_ {
        uint64_t function_number : 3;
        uint64_t device_number : 5;
        uint64_t bus_number : 8;
        uint64_t domain_number : 48;
    } bdf;
    struct {
        uint64_t function_number : 3;
        uint64_t device_number : 5;
        uint64_t bus_number : 8;
        uint64_t domain_number : 48;
    };
    uint64_t as_uint;
} amdsmi_bdf_t;

/**
 * @brief pcie information
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 */
typedef struct {
    struct pcie_static_ {
        uint16_t max_pcie_width;              //!< maximum number of PCIe lanes
        uint32_t max_pcie_speed;              //!< maximum PCIe speed in GT/s
        uint32_t pcie_interface_version;      //!< PCIe interface version
        amdsmi_card_form_factor_t slot_type;  //!< card form factor
        uint32_t max_pcie_interface_version;  //!< maximum PCIe link generation
        uint64_t reserved[9];
    } pcie_static;
    struct pcie_metric_ {
        uint16_t pcie_width;                   //!< current PCIe width
        uint32_t pcie_speed;                   //!< current PCIe speed in MT/s
        uint32_t pcie_bandwidth;               //!< current PCIe bandwidth in Mb/s
        uint64_t pcie_replay_count;            //!< total number of the replays issued on the PCIe link
        uint64_t pcie_l0_to_recovery_count;    //!< total number of times the PCIe link transitioned from L0 to the recovery state
        uint64_t pcie_replay_roll_over_count;  //!< total number of replay rollovers issued on the PCIe link
        uint64_t pcie_nak_sent_count;          //!< total number of NAKs issued on the PCIe link by the device
        uint64_t pcie_nak_received_count;      //!< total number of NAKs issued on the PCIe link by the receiver
        uint32_t pcie_lc_perf_other_end_recovery_count;  //!< PCIe other end recovery counter
        uint64_t reserved[12];
    } pcie_metric;
    uint64_t reserved[32];
} amdsmi_pcie_info_t;

/**
 * @brief Power Cap Information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
typedef struct {
    uint64_t power_cap;          //!< current power cap Units uW {@linux_bm} or W {@host}
    uint64_t default_power_cap;  //!< default power cap Units uW {@linux_bm} or W {@host}
    uint64_t dpm_cap;            //!< dpm power cap Units MHz {@linux_bm} or Hz {@host}
    uint64_t min_power_cap;      //!< minimum power cap Units uW {@linux_bm} or W {@host}
    uint64_t max_power_cap;      //!< maximum power cap Units uW {@linux_bm} or W {@host}
    uint64_t reserved[3];
} amdsmi_power_cap_info_t;

/**
 * @brief VBios Information
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 */
typedef struct {
    char name[AMDSMI_MAX_STRING_LENGTH];
    char build_date[AMDSMI_MAX_STRING_LENGTH];
    char part_number[AMDSMI_MAX_STRING_LENGTH];
    char version[AMDSMI_MAX_STRING_LENGTH];
    char boot_firmware[AMDSMI_MAX_STRING_LENGTH];
    uint64_t reserved[36];
} amdsmi_vbios_info_t;

/**
 * @brief ASIC Information
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 */
typedef struct {
    char  market_name[AMDSMI_MAX_STRING_LENGTH];
    uint32_t vendor_id;                //!< Use 32 bit to be compatible with other platform.
    char vendor_name[AMDSMI_MAX_STRING_LENGTH];
    uint32_t subvendor_id;             //!< The subsystem vendor ID
    uint64_t device_id;                //!< The device ID of a GPU
    uint32_t rev_id;                   //!< The revision ID of a GPU
    char asic_serial[AMDSMI_MAX_STRING_LENGTH];
    uint32_t oam_id;                   //!< 0xFFFFFFFF if not supported
    uint32_t num_of_compute_units;     //!< 0xFFFFFFFF if not supported
    uint64_t target_graphics_version;  //!< 0xFFFFFFFFFFFFFFFF if not supported
    uint32_t subsystem_id;             //!> The subsystem ID
    uint32_t reserved[21];
} amdsmi_asic_info_t;

/**
 * @brief Power Information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
typedef struct {
    uint64_t socket_power;          //!< Socket power in W {@linux_bm}, uW {@host}
    uint32_t current_socket_power;  //!< Current socket power in W {@linux_bm}, Linux only, Mi 300+ Series cards
    uint32_t average_socket_power;  //!< Average socket power in W {@linux_bm}, Linux only, Navi + Mi 200 and earlier Series cards
    uint64_t gfx_voltage;           //!< GFX voltage measurement in mV {@linux_bm} or V {@host}
    uint64_t soc_voltage;           //!< SOC voltage measurement in mV {@linux_bm} or V {@host}
    uint64_t mem_voltage;           //!< MEM voltage measurement in mV {@linux_bm} or V {@host}
    uint32_t power_limit;           //!< The power limit in W {@linux_bm}, Linux only
    uint64_t reserved[18];
} amdsmi_power_info_t;

/**
 * @brief Driver Information
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 */
typedef struct {
    char driver_version[AMDSMI_MAX_STRING_LENGTH];
    char driver_date[AMDSMI_MAX_STRING_LENGTH];
    char driver_name[AMDSMI_MAX_STRING_LENGTH];
    uint64_t reserved[64];
} amdsmi_driver_info_t;

/**
 * @brief Engine Usage
 * amdsmi_engine_usage_t:
 * This structure holds common
 * GPU activity values seen in both BM or
 * SRIOV
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 **/
typedef struct {
    uint32_t gfx_activity;  //!< In %
    uint32_t umc_activity;  //!< In %
    uint32_t mm_activity;   //!< In %
    uint32_t reserved[13];
} amdsmi_engine_usage_t;

/**
 * @brief Clock Information
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 */
typedef struct {
    uint32_t clk;            //!< In MHz
    uint32_t min_clk;        //!< In MHz
    uint32_t max_clk;        //!< In MHz
    uint8_t clk_locked;      //!< True/False
    uint8_t clk_deep_sleep;  //!< True/False
    uint32_t reserved[4];
} amdsmi_clk_info_t;

/**
 * @brief This structure holds error counts.
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @tag{host} @endcond
 */
typedef struct {
    uint64_t correctable_count;    //!< Accumulated correctable errors
    uint64_t uncorrectable_count;  //!< Accumulated uncorrectable errors
    uint64_t deferred_count;       //!< Accumulated deferred errors
    uint64_t reserved[5];
} amdsmi_error_count_t;

/**
 * @brief Firmware Information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
typedef struct {
    uint8_t num_fw_info;
    struct {
        amdsmi_fw_block_t fw_id;
        uint64_t fw_version;
        uint64_t reserved[2];
    } fw_info_list[AMDSMI_FW_ID__MAX];
    uint32_t reserved[7];
} amdsmi_fw_info_t;

/**
 * @brief This should match AMDSMI_MAX_NUM_XCC;
 * XCC - Accelerated Compute Core, the collection of compute units,
 * ACE (Asynchronous Compute Engines), caches,
 * and global resources organized as one unit.
 *
 * Refer to amd.com documentation for more detail:
 * https://www.amd.com/content/dam/amd/en/documents/instinct-tech-docs/white-papers/amd-cdna-3-white-paper.pdf
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @endcond
 */
#define AMDSMI_MAX_NUM_XCC 8

/**
 * @brief This should match AMDSMI_MAX_NUM_XCP;
 * XCP - Accelerated Compute Processor,
 * also referred to as the Graphics Compute Partitions.
 * Each physical gpu could have a maximum of 8 separate partitions
 * associated with each (depending on ASIC support).
 *
 * Refer to amd.com documentation for more detail:
 * https://www.amd.com/content/dam/amd/en/documents/instinct-tech-docs/white-papers/amd-cdna-3-white-paper.pdf
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @endcond
 */
#define AMDSMI_MAX_NUM_XCP 8

/**
 * @brief Process Handle
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @endcond
 */
typedef uint32_t amdsmi_process_handle_t;

/**
 * @brief Process Information
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @endcond
 */
typedef struct {
    char name[AMDSMI_MAX_STRING_LENGTH];
    amdsmi_process_handle_t pid;
    uint64_t mem;  //!< In Bytes
    struct {
        uint64_t gfx;  //!< In nano-secs
        uint64_t enc;  //!< In nano-secs
        uint32_t reserved[12];
    } engine_usage; //!< time the process spends using these engines in ns
    struct {
        uint64_t gtt_mem;   //!< In MB
        uint64_t cpu_mem;   //!< In MB
        uint64_t vram_mem;  //!< In MB
        uint32_t reserved[10];
    } memory_usage; //!< in bytes
    char container_name[AMDSMI_MAX_STRING_LENGTH];
    uint32_t cu_occupancy;  //!< Num CUs utilized
    uint32_t reserved[11];
} amdsmi_proc_info_t;

/**
 * @brief Compute Partition. This enum is used to identify
 * various compute partitioning settings.
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @endcond
 */
typedef enum {
    AMDSMI_COMPUTE_PARTITION_INVALID = 0,   //!< Invalid compute partition type
    AMDSMI_COMPUTE_PARTITION_SPX,           /**< Single GPU mode (SPX)- All XCCs work
                                                 together with shared memory */
    AMDSMI_COMPUTE_PARTITION_DPX,           /**< Dual GPU mode (DPX)- Half XCCs work
                                                 together with shared memory */
    AMDSMI_COMPUTE_PARTITION_TPX,           /**< Triple GPU mode (TPX)- One-third XCCs
                                                 work together with shared memory */
    AMDSMI_COMPUTE_PARTITION_QPX,           /**< Quad GPU mode (QPX)- Quarter XCCs
                                                 work together with shared memory */
    AMDSMI_COMPUTE_PARTITION_CPX            /**< Core mode (CPX)- Per-chip XCC with
                                                 shared memory */
} amdsmi_compute_partition_type_t;

/**
 * @brief VRam Usage
 *
 * @cond @tag{gpu_bm_linux} @tag{guest_windows} @endcond
 */
typedef struct {
    uint32_t vram_total;  //!< In MB
    uint32_t vram_used;   //!< In MB
    uint32_t reserved[2];
} amdsmi_vram_usage_t;

/**
 * @brief Window Defines
 *
 * @cond @tag{guest_windows} @endcond
 */
#define AMDSMI_MAX_PROCESSORS 32
#define AMDSMI_MAX_NUM_FREQUENCIES 32

//! Major version should be changed for every header change that breaks ABI
//! Such as adding/deleting APIs, changing names, fields of structures, etc.
#define AMDSMI_LIB_VERSION_MAJOR 3

//! Minor version should be updated for each API change, but without changing headers
#define AMDSMI_LIB_VERSION_MINOR 0

//! Release version should be set to 0 as default and can be updated by the PMs for each CSP point release
#define AMDSMI_LIB_VERSION_RELEASE 0

#define AMDSMI_LIB_VERSION_CREATE_STRING(MAJOR, MINOR, RELEASE) (#MAJOR "." #MINOR "." #RELEASE)
#define AMDSMI_LIB_VERSION_EXPAND_PARTS(MAJOR_STR, MINOR_STR, RELEASE_STR) AMDSMI_LIB_VERSION_CREATE_STRING(MAJOR_STR, MINOR_STR, RELEASE_STR)
#define AMDSMI_LIB_VERSION_STRING AMDSMI_LIB_VERSION_EXPAND_PARTS(AMDSMI_LIB_VERSION_MAJOR, AMDSMI_LIB_VERSION_MINOR, AMDSMI_LIB_VERSION_RELEASE)

/**
 * @brief Throttle Flags
 *
 * @cond @tag{guest_windows} @endcond
 */
typedef enum {
    THROTTLE_POWER_FLAG = 1 << 0,
    THROTTLE_THERMAL_FLAG = 1 << 1,
    THROTTLE_CURRENT_FLAG = 1 << 2,
} amdsmi_throttle_flags_t;

/**
 * @brief Raster Feature
 *
 * @cond @tag{guest_windows} @endcond
 */
typedef struct {
    struct {
        uint32_t dram_ecc : 1;
        uint32_t sram_ecc : 1;
        uint32_t poisoning : 1;
        uint32_t rsvd : 29;
    } ras_info;
    bool needs_reboot;
    uint32_t ras_eeprom_version;
    uint32_t ecc_correction_schema_flag;
    uint32_t reserved[4];
} amdsmi_ras_feature_t;

/**
 * @brief Version
 *
 * @cond @tag{guest_windows} @endcond
 */
typedef struct {
    uint32_t major;    //!< Major version
    uint32_t minor;    //!< Minor version
    uint32_t release;  //!< Patch, build or stepping version
} amdsmi_version_t;

/*****************************************************************************/
/** @defgroup tagInitShutdown Initialization and Shutdown
 *  @{
 */

/**
 *  @brief Initialize the AMD SMI library
 *
 *  @ingroup tagInitShutdown
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{cpu_bm} @platform{guest_1vf}
 *  @platform{guest_mvf} @platform{guest_windows}
 *
 *  @details This function initializes the library and the internal data structures,
 *  including those corresponding to sources of information that SMI provides.
 *
 *  The @p init_flags decides which type of processor
 *  can be discovered by ::amdsmi_get_socket_handles(). AMDSMI_INIT_AMD_GPUS returns
 *  sockets with AMD GPUS, and AMDSMI_INIT_AMD_GPUS | AMDSMI_INIT_AMD_CPUS returns
 *  sockets with either AMD GPUS or CPUS.
 *  Currently, only AMDSMI_INIT_AMD_GPUS is supported.
 *
 *  @param[in] init_flags Bit flags that tell SMI how to initialze. Values of
 *  ::amdsmi_init_flags_t may be OR'd together and passed through @p init_flags
 *  to modify how AMDSMI initializes.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_init(uint64_t init_flags);

/**
 *  @brief Shutdown the AMD SMI library
 *
 *  @ingroup tagInitShutdown
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{cpu_bm} @platform{guest_1vf}
 *  @platform{guest_mvf} @platform{guest_windows}
 *
 *  @details This function shuts down the library and internal data structures and
 *  performs any necessary clean ups.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_shut_down(void);

/** @} End tagInitShutdown */

/*****************************************************************************/
/** @defgroup tagProcDiscovery Processor Discovery
 *  @{
 */

/**
 *  @brief Get the processor type of the processor_handle
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{cpu_bm} @platform{guest_1vf}
 *  @platform{guest_mvf} @platform{guest_windows}
 *
 *  @details This function retrieves the processor type. A processor_handle must be provided
 *  for that processor.
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[out] processor_type a pointer to ::amdsmi_processor_type_t to which the processor type
 *  will be written. If this parameter is nullptr, this function will return
 *  ::AMDSMI_STATUS_INVAL.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_processor_type(amdsmi_processor_handle processor_handle,
                                          amdsmi_processor_type_t *processor_type);

/**
 *  @brief Returns the processor handle from the given processor index
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{cpu_bm} @platform{guest_1vf}
 *  @platform{guest_mvf} @platform{guest_windows}
 *
 *  @param[in] processor_index Function processor_index to query
 *
 *  @note On the @platform{host} this function currently supports only AMD GPU indexes.
 *
 *  @param[out] processor_handle Reference to the processor handle.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_processor_handle_from_index(uint32_t processor_index, amdsmi_processor_handle *processor_handle);

/**
 *  @brief Get the list of socket handles in the system.
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{cpu_bm} @platform{guest_1vf}
 *  @platform{guest_mvf} @platform{guest_windows}
 *
 *  @details Depends on what flag is passed to ::amdsmi_init.  AMDSMI_INIT_AMD_GPUS
 *  returns sockets with AMD GPUS, and AMDSMI_INIT_AMD_GPUS | AMDSMI_INIT_AMD_CPUS returns
 *  sockets with either AMD GPUS or CPUS.
 *  The socket handles can be used to query the processor handles in that socket, which
 *  will be used in other APIs to get processor detail information or telemtries.
 *
 *  @param[in,out] socket_count As input, the value passed
 *  through this parameter is the number of ::amdsmi_socket_handle that
 *  may be safely written to the memory pointed to by @p socket_handles. This is the
 *  limit on how many socket handles will be written to @p socket_handles. On return, @p
 *  socket_count will contain the number of socket handles written to @p socket_handles,
 *  or the number of socket handles that could have been written if enough memory had been
 *  provided.
 *  If @p socket_handles is NULL, as output, @p socket_count will contain
 *  how many sockets are available to read in the system.
 *
 *  @param[in,out] socket_handles A pointer to a block of memory to which the
 *  ::amdsmi_socket_handle values will be written. This value may be NULL.
 *  In this case, this function can be used to query how many sockets are
 *  available to read in the system.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_socket_handles(uint32_t *socket_count, amdsmi_socket_handle *socket_handles);

/**
 *  @brief Returns the index of the given processor handle
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{cpu_bm} @platform{guest_1vf}
 *  @platform{guest_mvf} @platform{guest_windows}
 *
 *  @param[in] processor_handle Processor handle for which to query
 *
 *  @param[out] processor_index Pointer to integer to store the processor index. Must be
 *  allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_index_from_processor_handle(amdsmi_processor_handle processor_handle, uint32_t *processor_index);

/**
 *  @brief Get the list of the processor handles associated to a socket.
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf}
 *  @platform{guest_mvf} @platform{guest_windows}
 *
 *  @details This function retrieves the processor handles of a socket. The
 *  @p socket_handle must be provided for the processor. A socket may have mulitple different
 *  type processors: An APU on a socket have both CPUs and GPUs.
 *  Currently, only AMD GPUs are supported.
 *
 *  @note Sockets are not supported on the @platform{host}.
 * 
 *  @note On the @platform{host} this function currently supports only AMD GPUs. To enumerate other devices,
 *  such as AMD NICs, use amdsmi_get_processor_handles_by_type().
 *
 *  The number of processor count is returned through @p processor_count
 *  if @p processor_handles is NULL. Then the number of @p processor_count can be pass
 *  as input to retrieval all processors on the socket to @p processor_handles.
 *
 *  @param[in] socket_handle The socket to query
 *
 *  @param[in,out] processor_count As input, the value passed
 *  through this parameter is the number of ::amdsmi_processor_handle's that
 *  may be safely written to the memory pointed to by @p processor_handles. This is the
 *  limit on how many processor handles will be written to @p processor_handles. On return, @p
 *  processor_count will contain the number of processor handles written to @p processor_handles,
 *  or the number of processor handles that could have been written if enough memory had been
 *  provided.
 *  If @p processor_handles is NULL, as output, @p processor_count will contain
 *  how many processors are available to read for the socket.
 *
 *  @param[in,out] processor_handles A pointer to a block of memory to which the
 *  ::amdsmi_processor_handle values will be written. This value may be NULL.
 *  In this case, this function can be used to query how many processors are
 *  available to read.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_processor_handles(amdsmi_socket_handle socket_handle,
                                             uint32_t *processor_count,
                                             amdsmi_processor_handle *processor_handles);

/**
 *  @brief Get information about the given socket
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf}
 *  @platform{guest_mvf} @platform{guest_windows}
 *
 *  @details This function retrieves socket information. The @p socket_handle must
 *  be provided to retrieve the Socket ID.
 *
 *  @param[in] socket_handle a socket handle
 *
 *  @param[in] len the length of the caller provided buffer @p name.
 *
 *  @param[out] name The id of the socket.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_socket_info(amdsmi_socket_handle socket_handle, size_t len, char *name);

/**
 *  @brief Get processor handle with the matching bdf.
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf}
 *  @platform{guest_mvf} @platform{guest_windows}
 *
 *  @details Given bdf info @p bdf, this function will get
 *  the processor handle with the matching bdf.
 *
 *  @param[in] bdf The bdf to match with corresponding processor handle.
 *
 *  @param[out] processor_handle processor handle with the matching bdf.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_processor_handle_from_bdf(amdsmi_bdf_t bdf, amdsmi_processor_handle *processor_handle);

/**
 *  @brief Returns BDF of the given GPU device
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *  @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] bdf Reference to BDF. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_device_bdf(amdsmi_processor_handle processor_handle, amdsmi_bdf_t *bdf);

/**
 *  @brief Returns BDF of the given device
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *  @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] bdf Reference to BDF. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_processor_bdf(amdsmi_processor_handle processor_handle, amdsmi_bdf_t *bdf);

/**
 *  @brief Returns the processor handle from the given UUID
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_windows}
 *
 *  @param[in] uuid Function UUID to query.
 *
 *  @param[out] processor_handle Reference to the processor handle.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_processor_handle_from_uuid(const char *uuid, amdsmi_processor_handle *processor_handle);

/**
 *  @brief Returns the UUID of the device
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *  @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[in,out] uuid_length Length of the uuid string. As input, must be
 *                 equal or greater than AMDSMI_GPU_UUID_SIZE and be allocated by
 *                 user. As output it is the length of the uuid string.
 *
 *  @param[out] uuid Pointer to string to store the UUID. Must be
 *              allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_device_uuid(amdsmi_processor_handle processor_handle, unsigned int *uuid_length, char *uuid);

/**
 *  @brief Returns the virtualization mode for the target device.
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{guest_1vf} @platform{host} @platform{guest_windows}
 *
 *  @details The virtualization mode is detected and returned as an enum.
 *
 *  @param[in] processor_handle The identifier of the given device.
 *
 *  @param[in,out] mode Reference to the enum representing virtualization mode.
 *                  - When zero, the virtualization mode is unknown
 *                  - When non-zero, the virtualization mode is detected
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail.
 */
 amdsmi_status_t amdsmi_get_gpu_virtualization_mode(amdsmi_processor_handle processor_handle, amdsmi_virtualization_mode_t *mode);

/** @} End tagProcDiscovery */

/*****************************************************************************/
/** @defgroup tagVersionQuery Software Version Information
 *  @{
 */

/**
 *  @brief Get the build version information for the currently running build of AMDSMI
 *
 *  @ingroup tagVersionQuery
 *
 *  @platform{gpu_bm_linux} @platform{cpu_bm} @platform{guest_1vf} @platform{guest_mvf}
 *  @platform{guest_windows}
 *
 *  @details  Get the major, minor, patch and build string for AMDSMI build
 *  currently in use through @p version
 *
 *  @param[in,out] version A pointer to an ::amdsmi_version_t structure that will
 *  be updated with the version information upon return.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_lib_version(amdsmi_version_t *version);

/** @} End tagVersionQuery */

/*****************************************************************************/
/** @defgroup tagErrorQuery Error Queries
 *  These functions provide error information about AMDSMI calls as well as
 *  device errors.
 *  @{
 */

/**
 *  @brief Get a description of a provided AMDSMI error status
 *
 *  @ingroup tagErrorQuery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{cpu_bm} @platform{guest_1vf} @platform{guest_mvf}
 *  @platform{guest_windows}
 *
 *  @details Set the provided pointer to a const char *, @p status_string, to
 *  a string containing a description of the provided error code @p status.
 *
 *  @param[in] status The error status for which a description is desired
 *
 *  @param[in,out] status_string A pointer to a const char * which will be made
 *  to point to a description of the provided error code
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_status_code_to_string(amdsmi_status_t status, const char **status_string);

/** @} End tagErrorQuery */

/*****************************************************************************/
/** @defgroup tagSoftwareVersion Software Version Information
 *  @{
 */

/**
 *  @brief Returns the driver version information
 *
 *  @ingroup tagSoftwareVersion
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *  @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] info Reference to driver information structure. Must be
 *              allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_driver_info(amdsmi_processor_handle processor_handle, amdsmi_driver_info_t *info);

/** @} End tagSoftwareVersion */

/*****************************************************************************/
/** @defgroup tagAsicBoardInfo ASIC & Board Static Information
 *  @{
 */

/**
 *  @brief Returns the ASIC information for the device
 *
 *  @ingroup tagAsicBoardInfo
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *  @platform{guest_windows}
 *
 *  @details This function returns ASIC information such as the product name,
 *           the vendor ID, the subvendor ID, the device ID,
 *           the revision ID and the serial number.
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] info Reference to static asic information structure.
 *              Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_asic_info(amdsmi_processor_handle processor_handle, amdsmi_asic_info_t *info);

/**
 *  @brief Returns the power caps as currently configured in the
 *  system.
 *
 *  @ingroup tagAsicBoardInfo
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[in] sensor_ind a 0-based sensor index. Normally, this will be 0.
 *  If a processor has more than one sensor, it could be greater than 0.
 *  Parameter @p sensor_ind is unused on @platform{host}.
 *
 *  @param[out] info Reference to power caps information structure. Must be
 *  allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_power_cap_info(amdsmi_processor_handle processor_handle, uint32_t sensor_ind,
                          amdsmi_power_cap_info_t *info);

/**
 *  @brief Returns the PCIe info for the GPU.
 *
 *  @ingroup tagAsicBoardInfo
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] info Reference to the PCIe information
 *  returned by the library. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_pcie_info(amdsmi_processor_handle processor_handle, amdsmi_pcie_info_t *info);

/** @} End tagAsicBoardInfo */

/*****************************************************************************/
/** @defgroup tagFWVbiosQuery Firmware & VBIOS queries
 *  @{
 */

/**
 *  @brief Returns the firmware versions running on the device.
 *
 *  @ingroup tagFWVbiosQuery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *  @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] info Reference to the fw info. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_fw_info(amdsmi_processor_handle processor_handle, amdsmi_fw_info_t *info);

/**
 *  @brief Returns the static information for the vBIOS on the device.
 *
 *  @ingroup tagFWVbiosQuery
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *  @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] info Reference to static vBIOS information.
 *              Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_gpu_vbios_info(amdsmi_processor_handle processor_handle, amdsmi_vbios_info_t *info);

/** @} End tagFWVbiosQuery */

/*****************************************************************************/
/** @defgroup tagGPUMonitor GPU Monitoring
 *  @{
 */

/**
 *  @brief Returns the current usage of the GPU engines (GFX, MM and MEM).
 *  Each usage is reported as a percentage from 0-100%.
 *
 *  @ingroup tagGPUMonitor
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] info Reference to the gpu engine usage structure. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_activity(amdsmi_processor_handle processor_handle, amdsmi_engine_usage_t *info);

/**
 *  @brief Returns the current power and voltage of the GPU.
 *
 *  @ingroup tagGPUMonitor
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_windows}
 *
 *  @note amdsmi_power_info_t::socket_power metric can rarely spike above the socket power limit in some cases
 *
 *  @param[in] processor_handle PF of a processor for which  to query
 *
 *
 *  @param[out] info Reference to the gpu power structure. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_power_info(amdsmi_processor_handle processor_handle, amdsmi_power_info_t *info);

/**
 *  @brief Returns the measurements of the clocks in the GPU
 *         for the GFX and multimedia engines and Memory. This call
 *         reports the averages over 1s in MHz. It is not supported
 *         on virtual machine guest
 *
 *  @ingroup tagGPUMonitor
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[in] clk_type Enum representing the clock type to query.
 *
 *  @param[out] info Reference to the gpu clock structure.
 *              Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_clock_info(amdsmi_processor_handle processor_handle, amdsmi_clk_type_t clk_type, amdsmi_clk_info_t *info);

/**
 *  @brief Get the temperature metric value for the specified metric, from the
 *  specified temperature sensor on the specified device. It is not supported on
 *  virtual machine guest
 *
 *  @ingroup tagGPUMonitor
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_windows}
 *
 *  @details Given a processor handle @p processor_handle, a sensor type @p sensor_type, a
 *  ::amdsmi_temperature_metric_t @p metric and a pointer to an int64_t @p
 *  temperature, this function will write the value of the metric indicated by
 *  @p metric and @p sensor_type to the memory location @p temperature.
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in] sensor_type part of device from which temperature should be
 *  obtained. This should come from the enum ::amdsmi_temperature_type_t
 *
 *  @param[in] metric enum indicated which temperature value should be
 *  retrieved
 *
 *  @param[in,out] temperature a pointer to int64_t to which the temperature is in Celsius.
 *  If this parameter is nullptr, this function will return ::AMDSMI_STATUS_INVAL if the function
 *  is supported with the provided, arguments and ::AMDSMI_STATUS_NOT_SUPPORTED if it is not
 *  supported with the provided arguments.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_temp_metric(amdsmi_processor_handle processor_handle, amdsmi_temperature_type_t sensor_type,
                                       amdsmi_temperature_metric_t metric, int64_t *temperature);

/**
 *  @brief Returns the VRAM usage (both total and used memory)
 *         in MegaBytes.
 *
 *  @ingroup tagGPUMonitor
 *
 *  @platform{gpu_bm_linux} @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *
 *  @param[out] info Reference to vram information.
 *                   Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_gpu_vram_usage(amdsmi_processor_handle processor_handle, amdsmi_vram_usage_t *info);

/**
 *  @brief Returns throttling information for a given GPU.
 *
 *  @platform{guest_windows}
 *
 *  @ingroup tagGPUMonitor
 *
 *  @note Returns flags on throttling status for each of the
 *  components. Flags are defined in amdsmi_throttle_flags enum.
 *
 *  @param[in] processor_handle processor which to query
 *
 *  @param[out] flags Returned status flags for each of the components.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_throttling_status(amdsmi_processor_handle processor_handle, uint32_t *flags);

/** @} End tagGPUMonitor */

/*****************************************************************************/
/** @defgroup tagClkPowerPerfControl Clock, Power and Performance Control
 *  These functions provide control over clock frequencies, power and
 *  performance.
 *  @{
 */

/**
 *  @brief Get the status of the Process Isolation
 *
 *  @ingroup tagClkPowerPerfControl
 *
 *  @platform{gpu_bm_linux} @platform{guest_1vf} @platform{guest_windows}
 *
 *  @details Given a processor handle @p processor_handle, this function will write
 *  current process isolation status to @p pisolate. The 0 is the process isolation
 *  disabled, and the 1 is the process isolation enabled.
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in,out] pisolate the process isolation status.
 *  If this parameter is nullptr, this function will return
 *  ::AMDSMI_STATUS_INVAL
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_process_isolation(amdsmi_processor_handle processor_handle,
                             uint32_t* pisolate);

/**
 *  @brief Enable/disable the system Process Isolation
 *
 *  @ingroup tagClkPowerPerfControl
 *
 *  @platform{gpu_bm_linux} @platform{guest_1vf} @platform{guest_windows}
 *
 *  @details Given a processor handle @p processor_handle and a process isolation @p pisolate,
 *  flag, this function will set the Process Isolation for this processor. The 0 is the process
 *  isolation disabled, and the 1 is the process isolation enabled.
 *
 *  @note This function requires root access
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in] pisolate the process isolation status to set.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_set_gpu_process_isolation(amdsmi_processor_handle processor_handle,
                             uint32_t pisolate);

/**
 *  @brief Run the cleaner shader to clean up data in LDS/GPRs
 *
 *  @ingroup tagClkPowerPerfControl
 *
 *  @platform{gpu_bm_linux} @platform{guest_1vf} @platform{guest_windows}
 *
 *  @details Given a processor handle @p processor_handle,
 *  this function will clean the local data of this processor. This can be called between
 *  user logins to prevent information leak.
 *
 *  @note This function requires root access
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_clean_gpu_local_data(amdsmi_processor_handle processor_handle);

/** @} End tagClkPowerPerfControl */

/*****************************************************************************/
/** @defgroup tagRasInfo RAS information
 *  @{
 */

/**
 *  @brief Returns the total number of ECC errors (correctable,
 *         uncorrectable and deferred) in the given GPU. It is not supported on
 *         virtual machine guest
 *
 *  @ingroup tagRasInfo
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] ec Reference to ecc error count structure.
 *              Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_gpu_total_ecc_count(amdsmi_processor_handle processor_handle, amdsmi_error_count_t *ec);

/**
 *  @brief Returns RAS features info.
 *
 *  @ingroup tagRasInfo
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_windows}
 *
 *  @param[in] processor_handle Device handle which to query
 *
 *  @param[out] ras_feature RAS features that are currently enabled and supported on
 *  the processor. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_ras_feature_info(amdsmi_processor_handle processor_handle, amdsmi_ras_feature_t *ras_feature);

/** @} End tagRasInfo */

/*****************************************************************************/
/** @defgroup tagProcessInfo Process information
 *  @{
 */

/**
 *  @brief Returns the list of process information running on a given GPU.
 *  If pdh.dll is not present on the system, this API returns
 *  AMDSMI_STATUS_NOT_SUPPORTED.
 *
 *  @ingroup tagProcessInfo
 *
 *  @platform{gpu_bm_linux} @platform{guest_windows}
 *
 *  @warning IMPORTANT: To get valid return values, at least 1 second needs to pass
 *  from starting the program to the first call of this function,
 *  and before every following call of this function after that, to get correct values
 *
 *  @note The user provides a buffer to store the list and the maximum
 *        number of processes that can be returned. If the user sets
 *        max_processes to 0, the current total number of processes will
 *        replace max_processes param. After that, the function needs to be
 *        called again, with updated max_processes, to successfully fill the
 *        process list, which was previously allocated with max_processes
 *
 *  @note If the reserved size for processes is smaller than the number of
 *        actual processes running. The AMDSMI_STATUS_OUT_OF_RESOURCES is
 *        an indication the caller should handle the situation (resize).
 *        The max_processes is always changed to reflect the actual size of
 *        list of processes running, so the caller knows where it is at.
 *
 *  @param[in]      processor_handle Device which to query
 *
 *  @param[in,out]  max_processes Reference to the size of the list buffer in
 *                  number of elements. Returns the return number of elements
 *                  in list or the number of running processes if equal to 0,
 *                  and if given value in param max_processes is less than
 *                  number of processes currently running,
 *                  AMDSMI_STATUS_OUT_OF_RESOURCES will be returned.
 *
 *                  For cases where max_process is not zero (0), it specifies the list's size limit.
 *                  That is, the maximum size this list will be able to hold. After the list is built
 *                  internally, as a return status, we will have AMDSMI_STATUS_OUT_OF_RESOURCES when
 *                  the original size limit is smaller than the actual list of processes running.
 *                  Hence, the caller is aware the list size needs to be resized, or
 *                  AMDSMI_STATUS_SUCCESS otherwise.
 *                  Holding a copy of max_process before it is passed in will be helpful for monitoring
 *                  the allocations done upon each call since the max_process will permanently be changed
 *                  to reflect the actual number of processes running.
 *
 *  @param[out]     list Reference to a user-provided buffer where the process
 *                  list will be returned. This buffer must contain at least
 *                  max_processes entries of type amd_proc_info_list_t. Must be allocated
 *                  by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success,
 *                            | ::AMDSMI_STATUS_OUT_OF_RESOURCES, filled list buffer with data, but number of
 *                                actual running processes is larger than the size provided.
 */
amdsmi_status_t amdsmi_get_gpu_process_list(amdsmi_processor_handle processor_handle,
                                            uint32_t *max_processes, amdsmi_proc_info_t *list);

/** @} End tagProcessInfo */

#endif  // __AMDSMI_H__

