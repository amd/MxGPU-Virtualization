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
} processor_type_t;

/**
 * @brief Common defines
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
#define AMDSMI_MAX_NUM_XGMI_PHYSICAL_LINK 64  //!< Maximum number of XGMI physical links
#define AMDSMI_MAX_NUM_PM_POLICIES        32  //!< Maximum number of power management policies
#define AMDSMI_MAX_CONTAINER_TYPE          2  //!< Maximum number of container types

/**
 * @brief Maximum size definitions
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
#define AMDSMI_MAX_MM_IP_COUNT              8  //!< Maximum number of multimedia IP blocks
#define AMDSMI_MAX_DEVICES                 32  //!< Maximum number of devices supported
#define AMDSMI_MAX_STRING_LENGTH          256  //!< Maximum length for string buffers
#define AMDSMI_MAX_CACHE_TYPES             10  //!< Maximum number of cache types
#define AMDSMI_MAX_CP_PROFILE_RESOURCES    32  //!< Maximum number of compute profile resources
#define AMDSMI_MAX_ACCELERATOR_PARTITIONS   8  //!< Maximum number of accelerator partitions
#define AMDSMI_MAX_ACCELERATOR_PROFILE     32  //!< Maximum number of accelerator profiles
#define AMDSMI_MAX_NUM_NUMA_NODES          32  //!< Maximum number of NUMA nodes
#define AMDSMI_GPU_UUID_SIZE               38  //!< Size of GPU UUID string

/**
 * @brief Max Number of AFIDs that will be inside one cper entry
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
#define MAX_NUMBER_OF_AFIDS_PER_RECORD 12 //!< Maximum AFIDs per CPER record

/**
 * @brief Maximum size definitions AMDSMI
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
#define AMDSMI_MAX_VF_COUNT               32  //!< Maximum number of virtual functions supported
#define AMDSMI_MAX_DRIVER_NUM              2  //!< Maximum number of drivers supported
#define AMDSMI_DFC_FW_NUMBER_OF_ENTRIES    9  //!< Number of DFC firmware entries supported
#define AMDSMI_MAX_WHITE_LIST_ELEMENTS    16  //!< Maximum number of white list elements for device access control
#define AMDSMI_MAX_BLACK_LIST_ELEMENTS    64  //!< Maximum number of black list elements for device access control
#define AMDSMI_MAX_UUID_ELEMENTS          16  //!< Maximum number of UUID elements supported
#define AMDSMI_MAX_TA_WHITE_LIST_ELEMENTS  8  //!< Maximum number of TA (Trusted Application) white list elements
#define AMDSMI_MAX_ERR_RECORDS            10  //!< Maximum number of error records that can be stored
#define AMDSMI_MAX_PROFILE_COUNT          16  //!< Maximum number of profiles supported

/**
 * @brief String format
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
#define AMDSMI_TIME_FORMAT "%02d:%02d:%02d.%03d"                //!< Time format string
#define AMDSMI_DATE_FORMAT "%04d-%02d-%02d:%02d:%02d:%02d.%03d" //!< Date format string

/**
 * @brief opaque handler point to underlying implementation
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef void *amdsmi_node_handle;

/**
 * @brief Memory Partitions
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_MEMORY_PARTITION_UNKNOWN = 0,
    AMDSMI_MEMORY_PARTITION_NPS1 = 1,  /**< NPS1 - All CCD & XCD data is interleaved
                                            across all 8 HBM stacks (all stacks/1) */
    AMDSMI_MEMORY_PARTITION_NPS2 = 2,  /**< NPS2 - 2 sets of CCDs or 4 XCD interleaved
                                            across the 4 HBM stacks per AID pair
                                            (8 stacks/2) */
    AMDSMI_MEMORY_PARTITION_NPS4 = 4,  /**< NPS4 - Each XCD data is interleaved
                                            across 2 (or single) HBM stacks
                                            (8 stacks/8 or 8 stacks/4) */
    AMDSMI_MEMORY_PARTITION_NPS8 = 8,  /**< NPS8 - Each XCD uses a single HBM stack
                                            (8 stacks/8). Or each XCD uses a single
                                            HBM stack & CCDs share 2 non-interleaved
                                            HBM stacks on its AID
                                            (AID[1,2,3] = 6 stacks/6) */
} amdsmi_memory_partition_type_t;

/**
 * @brief Accelerator Partition
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_ACCELERATOR_PARTITION_INVALID = 0,  //!< Invalid accelerator partition type
    AMDSMI_ACCELERATOR_PARTITION_SPX,          /**< Single GPU mode (SPX)- All XCCs work
                                                    together with shared memory */
    AMDSMI_ACCELERATOR_PARTITION_DPX,          /**< Dual GPU mode (DPX)- Half XCCs work
                                                    together with shared memory */
    AMDSMI_ACCELERATOR_PARTITION_TPX,          /**< Triple GPU mode (TPX)- One-third XCCs
                                                    work together with shared memory */
    AMDSMI_ACCELERATOR_PARTITION_QPX,          /**< Quad GPU mode (QPX)- Quarter XCCs
                                                    work together with shared memory */
    AMDSMI_ACCELERATOR_PARTITION_CPX,          /**< Core mode (CPX)- Per-chip XCC with
                                                    shared memory */
    AMDSMI_ACCELERATOR_PARTITION_MAX
} amdsmi_accelerator_partition_type_t;

/**
 * @brief vRam Types. This enum is used to identify various VRam types.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_VRAM_TYPE_UNKNOWN = 0, //!< Unknown memory type
    // HBM
    AMDSMI_VRAM_TYPE_HBM   = 1,   //!< High Bandwidth Memory
    AMDSMI_VRAM_TYPE_HBM2  = 2,   //!< High Bandwidth Memory, Generation 2
    AMDSMI_VRAM_TYPE_HBM2E = 3,   //!< High Bandwidth Memory, Generation 2 Enhanced
    AMDSMI_VRAM_TYPE_HBM3  = 4,   //!< High Bandwidth Memory, Generation 3
    AMDSMI_VRAM_TYPE_HBM3E = 5,   //!< High Bandwidth Memory, Generation 3 Enhanced
    // DDR
    AMDSMI_VRAM_TYPE_DDR2  = 10,  //!< Double Data Rate, Generation 2
    AMDSMI_VRAM_TYPE_DDR3  = 11,  //!< Double Data Rate, Generation 3
    AMDSMI_VRAM_TYPE_DDR4  = 12,  //!< Double Data Rate, Generation 4
    // GDDR
    AMDSMI_VRAM_TYPE_GDDR1 = 17,  //!< Graphics Double Data Rate, Generation 1
    AMDSMI_VRAM_TYPE_GDDR2 = 18,  //!< Graphics Double Data Rate, Generation 2
    AMDSMI_VRAM_TYPE_GDDR3 = 19,  //!< Graphics Double Data Rate, Generation 3
    AMDSMI_VRAM_TYPE_GDDR4 = 20,  //!< Graphics Double Data Rate, Generation 4
    AMDSMI_VRAM_TYPE_GDDR5 = 21,  //!< Graphics Double Data Rate, Generation 5
    AMDSMI_VRAM_TYPE_GDDR6 = 22,  //!< Graphics Double Data Rate, Generation 6
    AMDSMI_VRAM_TYPE_GDDR7 = 23,  //!< Graphics Double Data Rate, Generation 7
    AMDSMI_VRAM_TYPE__MAX = AMDSMI_VRAM_TYPE_GDDR7
} amdsmi_vram_type_t;

/**
 * @brief Accelerator Partition Resource Types
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_ACCELERATOR_XCC,      //!< Compute complex or stream processors
    AMDSMI_ACCELERATOR_ENCODER,  //!< Video encoding
    AMDSMI_ACCELERATOR_DECODER,  //!< Video decoding
    AMDSMI_ACCELERATOR_DMA,      //!< Direct Memory Access, high speed data transfers
    AMDSMI_ACCELERATOR_JPEG,     //!< Encoding and Decoding jpeg engines
    AMDSMI_ACCELERATOR_MAX
} amdsmi_accelerator_partition_resource_type_t;

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

    // GPU Board Node temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_FIRST = 100,
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_RETIMER_X
      = AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_FIRST,         //!< Retimer X temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_IBC,         //!< OAM X IBC temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_IBC_2,       //!< OAM X IBC 2 temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_VDD18_VR,    //!< OAM X VDD 1.8V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_04_HBM_B_VR, //!< OAM X 0.4V HBM B voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_OAM_X_04_HBM_D_VR, //!< OAM X 0.4V HBM D voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_NODE_LAST = 149,

    // GPU Board VR (Voltage Regulator) temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VR_FIRST = 150,
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_VDD0
         = AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VR_FIRST,   //!< VDDCR VDD0 voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_VDD1,        //!< VDDCR VDD1 voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_VDD2,        //!< VDDCR VDD2 voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_VDD3,        //!< VDDCR VDD3 voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_SOC_A,       //!< VDDCR SOC A voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_SOC_C,       //!< VDDCR SOC C voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_SOCIO_A,     //!< VDDCR SOCIO A voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_SOCIO_C,     //!< VDDCR SOCIO C voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDD_085_HBM,       //!< VDD 0.85V HBM voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_11_HBM_B,    //!< VDDCR 1.1V HBM B voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDCR_11_HBM_D,    //!< VDDCR 1.1V HBM D voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDD_USR,           //!< VDD USR voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VDDIO_11_E32,      //!< VDDIO 1.1V E32 voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_GPUBOARD_VR_LAST = 199,

    // Baseboard System temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_FIRST = 200,
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_FPGA = AMDSMI_TEMPERATURE_TYPE_BASEBOARD_FIRST,  //!< UBB FPGA temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_FRONT,          //!< UBB front temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_BACK,           //!< UBB back temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_OAM7,           //!< UBB OAM7 temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_IBC,            //!< UBB IBC temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_UFPGA,          //!< UBB UFPGA temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_OAM1,           //!< UBB OAM1 temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_0_1_HSC,        //!< OAM 0-1 HSC temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_2_3_HSC,        //!< OAM 2-3 HSC temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_4_5_HSC,        //!< OAM 4-5 HSC temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_6_7_HSC,        //!< OAM 6-7 HSC temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_FPGA_0V72_VR,   //!< UBB FPGA 0.72V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_UBB_FPGA_3V3_VR,    //!< UBB FPGA 3.3V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_0_1_2_3_1V2_VR,  //!< Retimer 0-1-2-3 1.2V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_4_5_6_7_1V2_VR,  //!< Retimer 4-5-6-7 1.2V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_0_1_0V9_VR, //!< Retimer 0-1 0.9V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_4_5_0V9_VR, //!< Retimer 4-5 0.9V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_2_3_0V9_VR, //!< Retimer 2-3 0.9V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_RETIMER_6_7_0V9_VR, //!< Retimer 6-7 0.9V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_0_1_2_3_3V3_VR, //!< OAM 0-1-2-3 3.3V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_OAM_4_5_6_7_3V3_VR, //!< OAM 4-5-6-7 3.3V voltage regulator temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_IBC_HSC,            //!< IBC HSC temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_IBC,                //!< IBC temperature
    AMDSMI_TEMPERATURE_TYPE_BASEBOARD_LAST = 249,
    AMDSMI_TEMPERATURE_TYPE__MAX = AMDSMI_TEMPERATURE_TYPE_BASEBOARD_LAST  //!< Maximum per GPU temperature type
} amdsmi_temperature_type_t;

/**
 * @brief Temperature Metrics. This enum is used to identify various
 * temperature metrics. Corresponding values will be in Celcius
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
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
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
 */
typedef enum {
    AMDSMI_CARD_FORM_FACTOR_PCIE,    //!< PCIE card form factor
    AMDSMI_CARD_FORM_FACTOR_OAM,     //!< OAM form factor
    AMDSMI_CARD_FORM_FACTOR_CEM,     //!< CEM form factor
    AMDSMI_CARD_FORM_FACTOR_UNKNOWN  //!< Unknown Form factor
} amdsmi_card_form_factor_t;

/**
 * @brief Link type
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_LINK_TYPE_INTERNAL = 0,        //!< Internal Link Type, within chip
    AMDSMI_LINK_TYPE_PCIE = 1,            //!< Peripheral Component Interconnect Express Link Type
    AMDSMI_LINK_TYPE_XGMI = 2,            //!< GPU Memory Interconnect (multi GPU communication)
    AMDSMI_LINK_TYPE_NOT_APPLICABLE = 3,  //!< Not Applicable Link Type
    AMDSMI_LINK_TYPE_UNKNOWN = 4          //!< Unknown Link Type
} amdsmi_link_type_t;

/**
 * @brief cache properties
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_CACHE_PROPERTY_ENABLED    = 0x00000001,  //!< Cache enabled
    AMDSMI_CACHE_PROPERTY_DATA_CACHE = 0x00000002,  //!< Data cache
    AMDSMI_CACHE_PROPERTY_INST_CACHE = 0x00000004,  //!< Instruction cache
    AMDSMI_CACHE_PROPERTY_CPU_CACHE  = 0x00000008,  //!< CPU cache
    AMDSMI_CACHE_PROPERTY_SIMD_CACHE = 0x00000010   //!< Single Instruction, Multiple Data Cache
} amdsmi_cache_property_type_t;

/**
 * @brief This enum is used to identify different GPU blocks.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_GPU_BLOCK_INVALID =   0,                         //!< Invalid block
    AMDSMI_GPU_BLOCK_FIRST =     (1ULL << 0),
    AMDSMI_GPU_BLOCK_UMC =       AMDSMI_GPU_BLOCK_FIRST,    //!< UMC block
    AMDSMI_GPU_BLOCK_SDMA =      (1ULL << 1),               //!< SDMA block
    AMDSMI_GPU_BLOCK_GFX =       (1ULL << 2),               //!< GFX block
    AMDSMI_GPU_BLOCK_MMHUB =     (1ULL << 3),               //!< MMHUB block
    AMDSMI_GPU_BLOCK_ATHUB =     (1ULL << 4),               //!< ATHUB block
    AMDSMI_GPU_BLOCK_PCIE_BIF =  (1ULL << 5),               //!< PCIE_BIF block
    AMDSMI_GPU_BLOCK_HDP =       (1ULL << 6),               //!< HDP block
    AMDSMI_GPU_BLOCK_XGMI_WAFL = (1ULL << 7),               //!< XGMI block
    AMDSMI_GPU_BLOCK_DF =        (1ULL << 8),               //!< DF block
    AMDSMI_GPU_BLOCK_SMN =       (1ULL << 9),               //!< SMN block
    AMDSMI_GPU_BLOCK_SEM =       (1ULL << 10),              //!< SEM block
    AMDSMI_GPU_BLOCK_MP0 =       (1ULL << 11),              //!< MP0 block
    AMDSMI_GPU_BLOCK_MP1 =       (1ULL << 12),              //!< MP1 block
    AMDSMI_GPU_BLOCK_FUSE =      (1ULL << 13),              //!< Fuse block
    AMDSMI_GPU_BLOCK_MCA =       (1ULL << 14),              //!< MCA block
    AMDSMI_GPU_BLOCK_VCN =       (1ULL << 15),              //!< VCN block
    AMDSMI_GPU_BLOCK_JPEG =      (1ULL << 16),              //!< JPEG block
    AMDSMI_GPU_BLOCK_IH =        (1ULL << 17),              //!< IH block
    AMDSMI_GPU_BLOCK_MPIO =      (1ULL << 18),              //!< MPIO block
    AMDSMI_GPU_BLOCK_LAST =      AMDSMI_GPU_BLOCK_MPIO,
    AMDSMI_GPU_BLOCK_RESERVED =  (1ULL << 63)
} amdsmi_gpu_block_t;

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
 * @brief Scope for Numa affinity or Socket affinity
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_AFFINITY_SCOPE_NODE,   //!< Memory affinity as numa node
    AMDSMI_AFFINITY_SCOPE_SOCKET  //!< socket affinity
} amdsmi_affinity_scope_t;

/**
 * @brief NPM status
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum  {
    AMDSMI_NPM_STATUS_DISABLED,
    AMDSMI_NPM_STATUS_ENABLED
} amdsmi_npm_status_t;

/**
 * @brief Link Status
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_LINK_STATUS_ENABLED  = 0,
    AMDSMI_LINK_STATUS_DISABLED = 1,
    AMDSMI_LINK_STATUS_INACTIVE = 2,
    AMDSMI_LINK_STATUS_ERROR    = 3
} amdsmi_link_status_t;

/**
 * @brief PTL (Peak Tops Limiter) data format types
 * These correspond to the hardware data types used in matrix operations.
 * Only F8 and XF32 are always supported at full performance. From the remaining
 * five types, only two can be supported at peak performance simultaneously.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_PTL_DATA_FORMAT_I8 = 0x0,             //!< Integer 8-bit format
    AMDSMI_PTL_DATA_FORMAT_F16 = 0x1,            //!< Float 16-bit format
    AMDSMI_PTL_DATA_FORMAT_BF16 = 0x2,           //!< Brain Float 16-bit format
    AMDSMI_PTL_DATA_FORMAT_F32 = 0x3,            //!< Float 32-bit format
    AMDSMI_PTL_DATA_FORMAT_F64 = 0x4,            //!< Float 64-bit format
    AMDSMI_PTL_DATA_FORMAT_INVALID = 0xFFFFFFFF  //!< Invalid format
} amdsmi_ptl_data_format_t;

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
 * @cond @tag{gpu_bm_linux} @tag{host} @tag{guest_windows} @endcond
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
 * @brief Link Metrics
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint32_t num_links;                   //!< number of links
    struct _links {
        amdsmi_bdf_t bdf;                 //!< bdf of the destination gpu
        uint32_t bit_rate;                //!< current link speed in Gb/s
        uint32_t max_bandwidth;           //!< max bandwidth of the link in Gb/s
        amdsmi_link_type_t link_type;     //!< type of the link
        uint64_t read;                    //!< total data received for each link in KB
        uint64_t write;                   //!< total data transfered for each link in KB
        amdsmi_link_status_t link_status; //!< HW status of the link
        uint64_t reserved[1];
    } links[AMDSMI_MAX_NUM_XGMI_PHYSICAL_LINK];
    uint64_t reserved[7];
} amdsmi_link_metrics_t;

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
    char boot_firmware[AMDSMI_MAX_STRING_LENGTH]; //!< UBL (Unified BootLoader) Version information
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
 * @brief VRam Information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    amdsmi_vram_type_t vram_type;
    char vram_vendor[AMDSMI_MAX_STRING_LENGTH];
    uint64_t vram_size;           //!< vram size in MB
    uint32_t vram_bit_width;      //!< In bits
    uint64_t vram_max_bandwidth;  //!< The VRAM max bandwidth at current memory clock (GB/s)
    uint64_t reserved[37];
} amdsmi_vram_info_t;

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
 * @brief This union holds memory partition bitmask.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef union {
    struct nps_flags_{
        uint32_t nps1_cap :1;   //!< bool 1 = true; 0 = false
        uint32_t nps2_cap :1;   //!< bool 1 = true; 0 = false
        uint32_t nps4_cap :1;   //!< bool 1 = true; 0 = false
        uint32_t nps8_cap :1;   //!< bool 1 = true; 0 = false
        uint32_t reserved :28;
    } nps_flags;
    uint32_t nps_cap_mask;
} amdsmi_nps_caps_t;

/**
 * @brief Memory Partition Configuration.
 * This structure is used to identify various memory partition configurations.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    amdsmi_nps_caps_t partition_caps;
    amdsmi_memory_partition_type_t mp_mode;
    uint32_t num_numa_ranges;
    struct numa_range_{
        amdsmi_vram_type_t memory_type;
        uint64_t start;
        uint64_t end;
    } numa_range[AMDSMI_MAX_NUM_NUMA_NODES];
    uint64_t reserved[11];
} amdsmi_memory_partition_config_t;

/**
 * @brief Accelerator Partition Resource Profile
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    amdsmi_accelerator_partition_type_t  profile_type;  //!< SPX, DPX, QPX, CPX and so on
    uint32_t num_partitions;                            //!< On MI300X: SPX=>1, DPX=>2, QPX=>4, CPX=>8; length of resources
    amdsmi_nps_caps_t memory_caps;                      //!< Possible memory partition capabilities
    uint32_t profile_index;                             //!< Index in the profiles array in amdsmi_accelerator_partition_profile_t
    uint32_t num_resources;                             //!< length of index_of_resources_profile
    uint32_t resources[AMDSMI_MAX_ACCELERATOR_PARTITIONS][AMDSMI_MAX_CP_PROFILE_RESOURCES];
    uint64_t reserved[13];
} amdsmi_accelerator_partition_profile_t;

/**
 * @brief  Accelerator Partition Resources.
 * This struct is used to identify various partition resource profiles.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint32_t profile_index;
    amdsmi_accelerator_partition_resource_type_t resource_type;
    uint32_t partition_resource;             //!< Resources a partition can use, which may be shared
    uint32_t num_partitions_share_resource;  //!< If it is greater than 1, then resource is shared.
    uint64_t reserved[6];
} amdsmi_accelerator_partition_resource_profile_t;

/**
 * @brief Accelerator Partition Profile Configurations
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint32_t num_profiles;           //!< The length of profiles array
    uint32_t num_resource_profiles;
    amdsmi_accelerator_partition_resource_profile_t resource_profiles[AMDSMI_MAX_CP_PROFILE_RESOURCES];
    uint32_t default_profile_index;  //!< The index of the default profile in the profiles array
    amdsmi_accelerator_partition_profile_t profiles[AMDSMI_MAX_ACCELERATOR_PROFILE];
    uint64_t reserved[30];
} amdsmi_accelerator_partition_profile_config_t;

/**
 * @brief Board Information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    char  model_number[AMDSMI_MAX_STRING_LENGTH];
    char  product_serial[AMDSMI_MAX_STRING_LENGTH];
    char  fru_id[AMDSMI_MAX_STRING_LENGTH];
    char  product_name[AMDSMI_MAX_STRING_LENGTH];
    char  manufacturer_name[AMDSMI_MAX_STRING_LENGTH];
    uint64_t reserved[64];
} amdsmi_board_info_t;

/**
 * @brief IO Link P2P Capability
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint8_t is_iolink_coherent;       //!< 1 = true, 0 = false, UINT8_MAX = Not defined
    uint8_t is_iolink_atomics_32bit;  //!< 1 = true, 0 = false, UINT8_MAX = Not defined
    uint8_t is_iolink_atomics_64bit;  //!< 1 = true, 0 = false, UINT8_MAX = Not defined
    uint8_t is_iolink_dma;            //!< 1 = true, 0 = false, UINT8_MAX = Not defined
    uint8_t is_iolink_bi_directional; //!< 1 = true, 0 = false, UINT8_MAX = Not defined
} amdsmi_p2p_capability_t;

/**
 * @brief NPM info
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    amdsmi_npm_status_t status; //!< NPM status (enabled/disabled).
    uint64_t            limit;  //!< Node-level power limit in Watts.
    uint64_t            reserved[6];
} amdsmi_npm_info_t;

/**
 * @brief GPU Cache Information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint32_t num_cache_types;
    struct cache_ {
        uint32_t cache_properties;    //!< amdsmi_cache_property_type_t which is a bitmask
        uint32_t cache_size;          //!< In KB
        uint32_t cache_level;
        uint32_t max_num_cu_shared;   //!< Indicates how many Compute Units share this cache instance
        uint32_t num_cache_instance;  //!< total number of instance of this cache type
        uint32_t reserved[3];
    } cache[AMDSMI_MAX_CACHE_TYPES];
    uint32_t reserved[15];
} amdsmi_gpu_cache_info_t;

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
 * @brief The dpm policy.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint32_t policy_id;
    char policy_description[AMDSMI_MAX_STRING_LENGTH];
} amdsmi_dpm_policy_entry_t;

/**
 * @brief DPM Policy
 *
 * Only the first num_supported policies are valid.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint32_t num_supported;                                         //!< The number of supported policies
    uint32_t current;                                               //!< The current policy index
    amdsmi_dpm_policy_entry_t policies[AMDSMI_MAX_NUM_PM_POLICIES]; //!< List of policies.
} amdsmi_dpm_policy_t;

/**
 * @brief This structure holds ras feature information.
 *
 * @cond @tag{gpu_bm_linux} @platform{guest_windows} @tag{host} @endcond
 */
typedef struct {
    struct {
        uint32_t dram_ecc : 1;
        uint32_t sram_ecc : 1;
        uint32_t poisoning : 1;
        uint32_t rsvd : 29;
    } ras_info;
    bool needs_reboot;
    uint32_t ras_eeprom_version;          /**< PARITY error(bit 0), Single Bit correctable (bit1),
                                               Double bit error detection (bit2), Poison (bit 3). */
    uint32_t ecc_correction_schema_flag;  /**< ecc_correction_schema mask.
                                               PARITY error(bit 0), Single Bit correctable (bit1),
                                               Double bit error detection (bit2), Poison (bit 3) */
    uint32_t reserved[4];
} amdsmi_ras_feature_t;

/**
 * @brief Cper sev
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED = 0,  //!< CPER Non-Fatal Uncorrected severity
    AMDSMI_CPER_SEV_FATAL                 = 1,  //!< CPER Fatal severity
    AMDSMI_CPER_SEV_NON_FATAL_CORRECTED   = 2,  //!< CPER Non-Fatal Corrected severity
    AMDSMI_CPER_SEV_NUM                   = 3,  //!< CPER severity Number
    AMDSMI_CPER_SEV_UNUSED                = 10  //!< CPER Unused severity
} amdsmi_cper_sev_t;

/**
 * @brief Cper notify
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef enum {
    AMDSMI_CPER_NOTIFY_TYPE_CMC  = 0x450eBDD72DCE8BB1,          //!< Corrected Memory Check
    AMDSMI_CPER_NOTIFY_TYPE_CPE  = 0x4a55D8434E292F96,          //!< Corrected Platform Error
    AMDSMI_CPER_NOTIFY_TYPE_MCE  = 0x4cc5919CE8F56FFE,          //!< Machine Check Exception
    AMDSMI_CPER_NOTIFY_TYPE_PCIE = 0x4dfc1A16CF93C01F,          //!< PCI Express Error
    AMDSMI_CPER_NOTIFY_TYPE_INIT = 0x454a9308CC5263E8,          //!< Initialization Error
    AMDSMI_CPER_NOTIFY_TYPE_NMI  = 0x42c9B7E65BAD89FF,          //!< Non_Maskable Interrupt
    AMDSMI_CPER_NOTIFY_TYPE_BOOT = 0x409aAB403D61A466,          //!< Boot Error
    AMDSMI_CPER_NOTIFY_TYPE_DMAR = 0x4c27C6B3667DD791,          //!< Direct Memory Access Remapping Error
    AMDSMI_CPER_NOTIFY_TYPE_SEA  = 0x11E4BBE89A78788A,          //!< System Error Architecture
    AMDSMI_CPER_NOTIFY_TYPE_SEI  = 0x4E87B0AE5C284C81,          //!< System Error Interface
    AMDSMI_CPER_NOTIFY_TYPE_PEI  = 0x4214520409A9D5AC,          //!< Platform Error Interface
    AMDSMI_CPER_NOTIFY_TYPE_CXL_COMPONENT = 0x49A341DF69293BC9  //!< Compute Express Link Component Error
} amdsmi_cper_notify_type_t;

/**
 * @brief Ras policy v4.0
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint16_t dram_non_critical_region_threshold;    //!< Non-critical region UCE threshold
    uint16_t dram_critical_region_threshold;        //!< Critical region UCE threshold
} amdsmi_gpu_ras_policy_v4_0_t;

/**
 * @brief Ras policy info structure for storing version and different ras
 *        policy version structures
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint8_t major_version;
    uint8_t minor_version;
    union {
        amdsmi_gpu_ras_policy_v4_0_t v4_0;
        uint64_t info[5]; //!< total size of the EEPROM that can be used by the policy is 40bytes
    } policy_data;
} amdsmi_gpu_ras_policy_info_t;

#pragma pack(push, 1)

/**
 * @brief Cper
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    unsigned char b[16];
} amdsmi_cper_guid_t;

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t flag;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t century;
} amdsmi_cper_timestamp_t;

typedef union {
    struct valid_bits_ {
        uint32_t platform_id  : 1;
        uint32_t timestamp    : 1;
        uint32_t partition_id : 1;
        uint32_t reserved     : 29;
    } valid_bits;
    uint32_t valid_mask;
} amdsmi_cper_valid_bits_t;

typedef struct {
    char                     signature[4];      //!< "CPER"
    uint16_t                 revision;
    uint32_t                 signature_end;     //!< 0xFFFFFFFF
    uint16_t                 sec_cnt;
    amdsmi_cper_sev_t        error_severity;
    amdsmi_cper_valid_bits_t cper_valid_bits;
    uint32_t                 record_length;     //!< Total size of CPER Entry
    amdsmi_cper_timestamp_t  timestamp;
    char                     platform_id[16];
    amdsmi_cper_guid_t       partition_id;      //!< Reserved
    char                     creator_id[16];
    amdsmi_cper_guid_t       notify_type;       //!< CMC, MCE, can use amdsmi_cper_notifiy_type_t to decode
    char                     record_id[8];      //!< Unique CPER Entry ID
    uint32_t                 flags;             //!< Reserved
    uint64_t                 persistence_info;  //!< Reserved
    uint8_t                  reserved[12];      //!< Reserved
} amdsmi_cper_hdr_t;

#pragma pack(pop)

/**
 * @brief Version info
 *
 * @cond @tag{host} @endcond
 */
#define SMI_VERSION_ALPHA_0 0x00000002
#define SMI_VERSION_BETA_0  0x00000003
#define SMI_VERSION_BETA_1  0x00000004
#define SMI_VERSION_BETA_2  0x00000005
#define SMI_VERSION_BETA_3  0x00000006
#define SMI_VERSION_BETA_4  0x00000007

/**
 * @brief AMDSMI event mask
 *
 * @cond @tag{host} @endcond
 */

//! include all events and all severities
#define AMDSMI_MASK_ALL (~0ULL)

//! include all events but only error severities without warnings and infos
#define AMDSMI_MASK_DEFAULT ((1ULL << 62) - 1)

//! a clear event mask
#define AMDSMI_MASK_INIT (0ULL)

//! check mask in case new severity levels are not supported
#define AMDSMI_MASK_HIGH_AND_MED_SEVERITY (~((1ULL << 61) - 1))

/**
 * @brief error severity level mask
 *
 * @cond @tag{host} @endcond
 */
#define AMDSMI_MASK_HIGH_ERROR_SEVERITY_ONLY(mask)   (mask & ((1ULL << 60) - 1))
#define AMDSMI_MASK_INCLUDE_MED_ERROR_SEVERITY(mask) (mask | (1ULL << 60))
#define AMDSMI_MASK_INCLUDE_LOW_ERROR_SEVERITY(mask) (mask | (1ULL << 61))
#define AMDSMI_MASK_INCLUDE_WARN_SEVERITY(mask) (mask | (1ULL << 62))
#define AMDSMI_MASK_INCLUDE_INFO_SEVERITY(mask) (mask | (1ULL << 63))

/**
 * @brief map old severity level mask to new severity level
 *
 * @cond @tag{host} @endcond
 */
#define AMDSMI_MASK_HIGH_SEVERITY_ONLY(mask)   (mask & ((1ULL << 62) - 1))
#define AMDSMI_MASK_INCLUDE_MED_SEVERITY(mask) AMDSMI_MASK_INCLUDE_WARN_SEVERITY(mask)
#define AMDSMI_MASK_INCLUDE_LOW_SEVERITY(mask) AMDSMI_MASK_INCLUDE_INFO_SEVERITY(mask)

#define AMDSMI_MASK_INCLUDE_CATEGORY(mask, cate) (mask | (1ULL << cate))
#define AMDSMI_MASK_EXCLUDE_CATEGORY(mask, cate) (mask & (~(1ULL << cate)))

#define AMDSMI_MAX_FB_SHARING_GROUPS 64
#define AMDSMI_MAX_NUM_CONNECTED_NODES 64

#define AMDSMI_MAX_NUM_METRICS_V1 255
#define AMDSMI_MAX_NUM_METRICS_V2 512
#define AMDSMI_MAX_NUM_METRICS AMDSMI_MAX_NUM_METRICS_V2

#define AMDSMI_MAX_BAD_PAGE_RECORD_V1 512
#define AMDSMI_MAX_BAD_PAGE_RECORD_V2 16384
#define AMDSMI_MAX_BAD_PAGE_RECORD AMDSMI_MAX_BAD_PAGE_RECORD_V2

/**
 * @brief Maximum size definitions for date strings
 *
 * @cond @tag{host} @endcond
 */
#define AMDSMI_MAX_DATE_STRING_LENGTH          32 //!< Date length for string buffers

/**
 * @brief Opague Handler point to underlying implementation
 *
 * @cond @tag{host} @endcond
 */
typedef void *amdsmi_event_set;

/**
 * @brief Event Category
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_CATEGORY_NON_USED = 0,
    AMDSMI_EVENT_CATEGORY_DRIVER   = 1,
    AMDSMI_EVENT_CATEGORY_RESET    = 2,
    AMDSMI_EVENT_CATEGORY_SCHED    = 3,
    AMDSMI_EVENT_CATEGORY_VBIOS    = 4,
    AMDSMI_EVENT_CATEGORY_ECC      = 5,
    AMDSMI_EVENT_CATEGORY_PP       = 6,
    AMDSMI_EVENT_CATEGORY_IOV      = 7,
    AMDSMI_EVENT_CATEGORY_VF       = 8,
    AMDSMI_EVENT_CATEGORY_FW       = 9,
    AMDSMI_EVENT_CATEGORY_GPU      = 10,
    AMDSMI_EVENT_CATEGORY_GUARD    = 11,
    AMDSMI_EVENT_CATEGORY_GPUMON   = 12,
    AMDSMI_EVENT_CATEGORY_MMSCH    = 13,
    AMDSMI_EVENT_CATEGORY_XGMI     = 14,
    AMDSMI_EVENT_CATEGORY__MAX
} amdsmi_event_category_t;

/**
 * @brief Below are the error subcodes of each category.
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_GPU_DEVICE_LOST = 0,
    AMDSMI_EVENT_GPU_NOT_SUPPORTED,
    AMDSMI_EVENT_GPU_RMA,
    AMDSMI_EVENT_GPU_NOT_INITIALIZED,
    AMDSMI_EVENT_GPU_MMSCH_ABNORMAL_STATE,
    AMDSMI_EVENT_GPU_RLCV_ABNORMAL_STATE,
    AMDSMI_EVENT_GPU_SDMA_ENGINE_BUSY,
    AMDSMI_EVENT_GPU_RLC_ENGINE_BUSY,
    AMDSMI_EVENT_GPU_GC_ENGINE_BUSY,
    AMDSMI_EVENT_GPU__MAX
} amdsmi_event_gpu_t;

/**
 * @brief Event Driver
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_DRIVER_SPIN_LOCK_BUSY = 0,
    AMDSMI_EVENT_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_GFX_WORKQUEUE_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_MM_WORKQUEUE_FAIL,
    AMDSMI_EVENT_DRIVER_BUFFER_OVERFLOW,

    AMDSMI_EVENT_DRIVER_DEV_INIT_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_THREAD_FAIL,
    AMDSMI_EVENT_DRIVER_NO_ACCESS_PCI_REGION,
    AMDSMI_EVENT_DRIVER_MMIO_FAIL,
    AMDSMI_EVENT_DRIVER_INTERRUPT_INIT_FAIL,

    AMDSMI_EVENT_DRIVER_INVALID_VALUE,
    AMDSMI_EVENT_DRIVER_CREATE_MUTEX_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_TIMER_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_EVENT_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_SPIN_LOCK_FAIL,

    AMDSMI_EVENT_DRIVER_ALLOC_FB_MEM_FAIL,
    AMDSMI_EVENT_DRIVER_ALLOC_DMA_MEM_FAIL,
    AMDSMI_EVENT_DRIVER_NO_FB_MANAGER,
    AMDSMI_EVENT_DRIVER_HW_INIT_FAIL,
    AMDSMI_EVENT_DRIVER_SW_INIT_FAIL,

    AMDSMI_EVENT_DRIVER_INIT_CONFIG_ERROR,
    AMDSMI_EVENT_DRIVER_ERROR_LOGGING_FAILED,
    AMDSMI_EVENT_DRIVER_CREATE_RWLOCK_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_RWSEMA_FAIL,
    AMDSMI_EVENT_DRIVER_GET_READ_LOCK_FAIL,

    AMDSMI_EVENT_DRIVER_GET_WRITE_LOCK_FAIL,
    AMDSMI_EVENT_DRIVER_GET_READ_SEMA_FAIL,
    AMDSMI_EVENT_DRIVER_GET_WRITE_SEMA_FAIL,

    AMDSMI_EVENT_DRIVER_DIAG_DATA_INIT_FAIL,
    AMDSMI_EVENT_DRIVER_DIAG_DATA_MEM_REQ_FAIL,
    AMDSMI_EVENT_DRIVER_DIAG_DATA_VADDR_REQ_FAIL,
    AMDSMI_EVENT_DRIVER_DIAG_DATA_BUS_ADDR_REQ_FAIL,

    AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_INIT_FAIL,
    AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_MEM_REQ_FAIL,
    AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_VADDR_REQ_FAIL,
    AMDSMI_EVENT_DRIVER_REMOTE_DEBUG_BUS_ADDR_REQ_FAIL,

    AMDSMI_EVENT_DRIVER_HRTIMER_START_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_DRIVER_FILE_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_DEVICE_FILE_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_DEBUGFS_FILE_FAIL,
    AMDSMI_EVENT_DRIVER_CREATE_DEBUGFS_DIR_FAIL,

    AMDSMI_EVENT_DRIVER_PCI_ENABLE_DEVICE_FAIL,
    AMDSMI_EVENT_DRIVER_FB_MAP_FAIL,
    AMDSMI_EVENT_DRIVER_DOORBELL_MAP_FAIL,
    AMDSMI_EVENT_DRIVER_PCI_REGISTER_DRIVER_FAIL,

    AMDSMI_EVENT_DRIVER_ALLOC_IOVA_ALIGN_FAIL,

    AMDSMI_EVENT_DRIVER_ROM_MAP_FAIL,
    AMDSMI_EVENT_DRIVER_FULL_ACCESS_TIMEOUT,

    AMDSMI_EVENT_DRIVER__MAX
} amdsmi_event_driver_t;

/**
 * @brief Event Firmware
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_FW_CMD_ALLOC_BUF_FAIL = 0,
    AMDSMI_EVENT_FW_CMD_BUF_PREP_FAIL,
    AMDSMI_EVENT_FW_RING_INIT_FAIL,
    AMDSMI_EVENT_FW_FW_APPLY_SECURITY_POLICY_FAIL,
    AMDSMI_EVENT_FW_START_RING_FAIL,

    AMDSMI_EVENT_FW_FW_LOAD_FAIL,
    AMDSMI_EVENT_FW_EXIT_FAIL,
    AMDSMI_EVENT_FW_INIT_FAIL,
    AMDSMI_EVENT_FW_CMD_SUBMIT_FAIL,
    AMDSMI_EVENT_FW_CMD_FENCE_WAIT_FAIL,

    AMDSMI_EVENT_FW_TMR_LOAD_FAIL,
    AMDSMI_EVENT_FW_TOC_LOAD_FAIL,
    AMDSMI_EVENT_FW_RAS_LOAD_FAIL,
    AMDSMI_EVENT_FW_RAS_UNLOAD_FAIL,
    AMDSMI_EVENT_FW_RAS_TA_INVOKE_FAIL,
    AMDSMI_EVENT_FW_RAS_TA_ERR_INJECT_FAIL,

    AMDSMI_EVENT_FW_ASD_LOAD_FAIL,
    AMDSMI_EVENT_FW_ASD_UNLOAD_FAIL,
    AMDSMI_EVENT_FW_AUTOLOAD_FAIL,
    AMDSMI_EVENT_FW_VFGATE_FAIL,

    AMDSMI_EVENT_FW_XGMI_LOAD_FAIL,
    AMDSMI_EVENT_FW_XGMI_UNLOAD_FAIL,
    AMDSMI_EVENT_FW_XGMI_TA_INVOKE_FAIL,

    AMDSMI_EVENT_FW_TMR_INIT_FAIL,
    AMDSMI_EVENT_FW_NOT_SUPPORTED_FEATURE,
    AMDSMI_EVENT_FW_GET_PSP_TRACELOG_FAIL,

    AMDSMI_EVENT_FW_SET_SNAPSHOT_ADDR_FAIL,
    AMDSMI_EVENT_FW_SNAPSHOT_TRIGGER_FAIL,

    AMDSMI_EVENT_FW_MIGRATION_GET_PSP_INFO_FAIL,
    AMDSMI_EVENT_FW_MIGRATION_EXPORT_FAIL,
    AMDSMI_EVENT_FW_MIGRATION_IMPORT_FAIL,

    AMDSMI_EVENT_FW_BL_FAIL,
    AMDSMI_EVENT_FW_RAS_BOOT_FAIL,
    AMDSMI_EVENT_FW_MAILBOX_ERROR,

    AMDSMI_EVENT_FW__MAX
} amdsmi_event_fw_t;

#define AMDSMI_EVENT_FW_FW_INIT_FAIL    AMDSMI_EVENT_FW_RING_INIT_FAIL

/**
 * @brief Event Reset
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_RESET_GPU = 0,
    AMDSMI_EVENT_RESET_GPU_FAILED,
    AMDSMI_EVENT_RESET_FLR,
    AMDSMI_EVENT_RESET_FLR_FAILED,
    AMDSMI_EVENT_RESET__MAX
} amdsmi_event_reset_t;

/**
 * @brief Event IOV
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_IOV_NO_GPU_IOV_CAP = 0,
    AMDSMI_EVENT_IOV_ASIC_NO_SRIOV_SUPPORT,
    AMDSMI_EVENT_IOV_ENABLE_SRIOV_FAIL,
    AMDSMI_EVENT_IOV_CMD_TIMEOUT,
    AMDSMI_EVENT_IOV_CMD_ERROR,

    AMDSMI_EVENT_IOV_INIT_IV_RING_FAIL,
    AMDSMI_EVENT_IOV_SRIOV_STRIDE_ERROR,
    AMDSMI_EVENT_IOV_WS_SAVE_TIMEOUT,
    AMDSMI_EVENT_IOV_WS_IDLE_TIMEOUT,
    AMDSMI_EVENT_IOV_WS_RUN_TIMEOUT,
    AMDSMI_EVENT_IOV_WS_LOAD_TIMEOUT,
    AMDSMI_EVENT_IOV_WS_SHUTDOWN_TIMEOUT,
    AMDSMI_EVENT_IOV_WS_ALREADY_SHUTDOWN,
    AMDSMI_EVENT_IOV_WS_INFINITE_LOOP,
    AMDSMI_EVENT_IOV_WS_REENTRANT_ERROR,
    AMDSMI_EVENT_IOV__MAX
} amdsmi_event_iov_t;

/**
 * @brief Event ECC
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_ECC_UCE = 0,
    AMDSMI_EVENT_ECC_CE,
    AMDSMI_EVENT_ECC_IN_PF_FB,
    AMDSMI_EVENT_ECC_IN_CRI_REG,
    AMDSMI_EVENT_ECC_IN_VF_CRI,
    AMDSMI_EVENT_ECC_REACH_THD,
    AMDSMI_EVENT_ECC_VF_CE,
    AMDSMI_EVENT_ECC_VF_UE,
    AMDSMI_EVENT_ECC_IN_SAME_ROW,
    AMDSMI_EVENT_ECC_UMC_UE,
    AMDSMI_EVENT_ECC_GFX_CE,
    AMDSMI_EVENT_ECC_GFX_UE,
    AMDSMI_EVENT_ECC_SDMA_CE,
    AMDSMI_EVENT_ECC_SDMA_UE,
    AMDSMI_EVENT_ECC_GFX_CE_TOTAL,
    AMDSMI_EVENT_ECC_GFX_UE_TOTAL,
    AMDSMI_EVENT_ECC_SDMA_CE_TOTAL,
    AMDSMI_EVENT_ECC_SDMA_UE_TOTAL,
    AMDSMI_EVENT_ECC_UMC_CE_TOTAL,
    AMDSMI_EVENT_ECC_UMC_UE_TOTAL,
    AMDSMI_EVENT_ECC_MMHUB_CE,
    AMDSMI_EVENT_ECC_MMHUB_UE,
    AMDSMI_EVENT_ECC_MMHUB_CE_TOTAL,
    AMDSMI_EVENT_ECC_MMHUB_UE_TOTAL,
    AMDSMI_EVENT_ECC_XGMI_WAFL_CE,
    AMDSMI_EVENT_ECC_XGMI_WAFL_UE,
    AMDSMI_EVENT_ECC_XGMI_WAFL_CE_TOTAL,
    AMDSMI_EVENT_ECC_XGMI_WAFL_UE_TOTAL,
    AMDSMI_EVENT_ECC_FATAL_ERROR,
    AMDSMI_EVENT_ECC_POISON_CONSUMPTION,
    AMDSMI_EVENT_ECC_ACA_DUMP,
    AMDSMI_EVENT_ECC_WRONG_SOCKET_ID,
    AMDSMI_EVENT_ECC_ACA_UNKNOWN_BLOCK_INSTANCE,
    AMDSMI_EVENT_ECC_UNKNOWN_CHIPLET_CE,
    AMDSMI_EVENT_ECC_UNKNOWN_CHIPLET_UE,
    AMDSMI_EVENT_ECC_UMC_CHIPLET_CE,
    AMDSMI_EVENT_ECC_UMC_CHIPLET_UE,
    AMDSMI_EVENT_ECC_GFX_CHIPLET_CE,
    AMDSMI_EVENT_ECC_GFX_CHIPLET_UE,
    AMDSMI_EVENT_ECC_SDMA_CHIPLET_CE,
    AMDSMI_EVENT_ECC_SDMA_CHIPLET_UE,
    AMDSMI_EVENT_ECC_MMHUB_CHIPLET_CE,
    AMDSMI_EVENT_ECC_MMHUB_CHIPLET_UE,
    AMDSMI_EVENT_ECC_XGMI_WAFL_CHIPLET_CE,
    AMDSMI_EVENT_ECC_XGMI_WAFL_CHIPLET_UE,
    AMDSMI_EVENT_ECC_EEPROM_ENTRIES_FOUND,
    AMDSMI_EVENT_ECC_UMC_DE,
    AMDSMI_EVENT_ECC_UMC_DE_TOTAL,
    AMDSMI_EVENT_ECC_UNKNOWN,
    AMDSMI_EVENT_ECC_EEPROM_REACH_THD,
    AMDSMI_EVENT_ECC_UMC_CHIPLET_DE,
    AMDSMI_EVENT_ECC_UNKNOWN_CHIPLET_DE,
    AMDSMI_EVENT_ECC_EEPROM_CHK_MISMATCH,
    AMDSMI_EVENT_ECC_EEPROM_RESET,
    AMDSMI_EVENT_ECC_EEPROM_RESET_FAILED,
    AMDSMI_EVENT_ECC_EEPROM_APPEND,
    AMDSMI_EVENT_ECC_THD_CHANGED,
    AMDSMI_EVENT_ECC_DUP_ENTRIES,
    AMDSMI_EVENT_ECC_EEPROM_WRONG_HDR,
    AMDSMI_EVENT_ECC_EEPROM_WRONG_VER,
    AMDSMI_EVENT_ECC__MAX
} amdsmi_event_ecc_t;

/**
 * @brief Event PP
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_PP_SET_DPM_POLICY_FAIL = 0,
    AMDSMI_EVENT_PP_ACTIVATE_DPM_POLICY_FAIL,
    AMDSMI_EVENT_PP_I2C_SLAVE_NOT_PRESENT,
    AMDSMI_EVENT_PP_THROTTLER_EVENT,
    AMDSMI_EVENT_PP__MAX
} amdsmi_event_pp_t;

/**
 * @brief Event Schedule
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_SCHED_WORLD_SWITCH_FAIL = 0,
    AMDSMI_EVENT_SCHED_DISABLE_AUTO_HW_SWITCH_FAIL,
    AMDSMI_EVENT_SCHED_ENABLE_AUTO_HW_SWITCH_FAIL,
    AMDSMI_EVENT_SCHED_GFX_SAVE_REG_FAIL,
    AMDSMI_EVENT_SCHED_GFX_IDLE_REG_FAIL,

    AMDSMI_EVENT_SCHED_GFX_RUN_REG_FAIL,
    AMDSMI_EVENT_SCHED_GFX_LOAD_REG_FAIL,
    AMDSMI_EVENT_SCHED_GFX_INIT_REG_FAIL,
    AMDSMI_EVENT_SCHED_MM_SAVE_REG_FAIL,
    AMDSMI_EVENT_SCHED_MM_IDLE_REG_FAIL,

    AMDSMI_EVENT_SCHED_MM_RUN_REG_FAIL,
    AMDSMI_EVENT_SCHED_MM_LOAD_REG_FAIL,
    AMDSMI_EVENT_SCHED_MM_INIT_REG_FAIL,
    AMDSMI_EVENT_SCHED_INIT_GPU_FAIL,
    AMDSMI_EVENT_SCHED_RUN_GPU_FAIL,

    AMDSMI_EVENT_SCHED_SAVE_GPU_STATE_FAIL,
    AMDSMI_EVENT_SCHED_LOAD_GPU_STATE_FAIL,
    AMDSMI_EVENT_SCHED_IDLE_GPU_FAIL,
    AMDSMI_EVENT_SCHED_FINI_GPU_FAIL,
    AMDSMI_EVENT_SCHED_DEAD_VF,

    AMDSMI_EVENT_SCHED_EVENT_QUEUE_FULL,
    AMDSMI_EVENT_SCHED_SHUTDOWN_VF_FAIL,
    AMDSMI_EVENT_SCHED_RESET_VF_NUM_FAIL,
    AMDSMI_EVENT_SCHED_IGNORE_EVENT,
    AMDSMI_EVENT_SCHED_PF_SWITCH_FAIL,
    AMDSMI_EVENT_SCHED__MAX
} amdsmi_event_sched_t;

/**
 * @brief Event VF
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_VF_ATOMBIOS_INIT_FAIL = 0,
    AMDSMI_EVENT_VF_NO_VBIOS,
    AMDSMI_EVENT_VF_GPU_POST_ERROR,
    AMDSMI_EVENT_VF_ATOMBIOS_GET_CLOCK_FAIL,
    AMDSMI_EVENT_VF_FENCE_INIT_FAIL,
    AMDSMI_EVENT_VF_AMDGPU_INIT_FAIL,
    AMDSMI_EVENT_VF_IB_INIT_FAIL,
    AMDSMI_EVENT_VF_AMDGPU_LATE_INIT_FAIL,
    AMDSMI_EVENT_VF_ASIC_RESUME_FAIL,
    AMDSMI_EVENT_VF_GPU_RESET_FAIL,
    AMDSMI_EVENT_VF__MAX
} amdsmi_event_vf_max_t;

/**
 * @brief Event VBios
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_VBIOS_INVALID = 0,
    AMDSMI_EVENT_VBIOS_IMAGE_MISSING,
    AMDSMI_EVENT_VBIOS_CHECKSUM_ERR,
    AMDSMI_EVENT_VBIOS_POST_FAIL,
    AMDSMI_EVENT_VBIOS_READ_FAIL,

    AMDSMI_EVENT_VBIOS_READ_IMG_HEADER_FAIL,
    AMDSMI_EVENT_VBIOS_READ_IMG_SIZE_FAIL,
    AMDSMI_EVENT_VBIOS_GET_FW_INFO_FAIL,
    AMDSMI_EVENT_VBIOS_GET_TBL_REVISION_FAIL,
    AMDSMI_EVENT_VBIOS_PARSER_TBL_FAIL,

    AMDSMI_EVENT_VBIOS_IP_DISCOVERY_FAIL,
    AMDSMI_EVENT_VBIOS_TIMEOUT,
    AMDSMI_EVENT_VBIOS_HASH_INVALID,
    AMDSMI_EVENT_VBIOS_HASH_UPDATED,
    AMDSMI_EVENT_VBIOS_IP_DISCOVERY_BINARY_CHECKSUM_FAIL,
    AMDSMI_EVENT_VBIOS_IP_DISCOVERY_TABLE_CHECKSUM_FAIL,
    AMDSMI_EVENT_VBIOS__MAX
} amdsmi_event_vbios_t;

/**
 * @brief Event Guard
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_GUARD_RESET_FAIL = 0,
    AMDSMI_EVENT_GUARD_EVENT_OVERFLOW,
    AMDSMI_EVENT_GUARD__MAX
} amdsmi_event_guard_t;

/**
 * @brief Event GPU Monitor
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_GPUMON_INVALID_OPTION = 0,
    AMDSMI_EVENT_GPUMON_INVALID_VF_INDEX,
    AMDSMI_EVENT_GPUMON_INVALID_FB_SIZE,
    AMDSMI_EVENT_GPUMON_NO_SUITABLE_SPACE,
    AMDSMI_EVENT_GPUMON_NO_AVAILABLE_SLOT,

    AMDSMI_EVENT_GPUMON_OVERSIZE_ALLOCATION,
    AMDSMI_EVENT_GPUMON_OVERLAPPING_FB,
    AMDSMI_EVENT_GPUMON_INVALID_GFX_TIMESLICE,
    AMDSMI_EVENT_GPUMON_INVALID_MM_TIMESLICE,
    AMDSMI_EVENT_GPUMON_INVALID_GFX_PART,

    AMDSMI_EVENT_GPUMON_VF_BUSY,
    AMDSMI_EVENT_GPUMON_INVALID_VF_NUM,
    AMDSMI_EVENT_GPUMON_NOT_SUPPORTED,
    AMDSMI_EVENT_GPUMON__MAX
} amdsmi_event_gpumon_t;

/**
 * @brief Event MM Schedule
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_MMSCH_IGNORED_JOB = 0,
    AMDSMI_EVENT_MMSCH_UNSUPPORTED_VCN_FW,
    AMDSMI_EVENT_MMSCH__MAX
} amdsmi_event_mmsch_t;

/**
 * @brief  Event XGMI
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_XGMI_TOPOLOGY_UPDATE_FAILED = 0,
    AMDSMI_EVENT_XGMI_TOPOLOGY_HW_INIT_UPDATE,
    AMDSMI_EVENT_XGMI_TOPOLOGY_UPDATE_DONE,
    AMDSMI_EVENT_XGMI_FB_SHARING_SETTING_ERROR,
    AMDSMI_EVENT_XGMI_FB_SHARING_SETTING_RESET,
    AMDSMI_EVENT_XGMI__MAX
} amdsmi_event_xgmi_t;

/**
 * @brief This enum determines which type of PP throttler event occurred
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_EVENT_THROTTLER_PROCHOT = 0,
    AMDSMI_EVENT_THROTTLER_SOCKET,
    AMDSMI_EVENT_THROTTLER_VR,
    AMDSMI_EVENT_THROTTLER_HBM
} amdsmi_pp_throttler_type_t;

/**
 * @brief The values of this enum are used to identify supported ecc correction schema
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_RAS_ECC_SUPPORT_PARITY = (1 << 0),
    AMDSMI_RAS_ECC_SUPPORT_CORRECTABLE = (1 << 1),
    AMDSMI_RAS_ECC_SUPPORT_UNCORRECTABLE = (1 << 2),
    AMDSMI_RAS_ECC_SUPPORT_POISON = (1 << 3)
} amdsmi_ecc_correction_schema_support_t;

/**
 * @brief The values of this enum are used to identify the various firmware blocks.
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_GUEST_FW_ID_VCE = 0,
    AMDSMI_GUEST_FW_ID_UVD,
    AMDSMI_GUEST_FW_ID_MC,
    AMDSMI_GUEST_FW_ID_ME,
    AMDSMI_GUEST_FW_ID_PFP,
    AMDSMI_GUEST_FW_ID_CE,
    AMDSMI_GUEST_FW_ID_RLC,
    AMDSMI_GUEST_FW_ID_RLC_SRLC,
    AMDSMI_GUEST_FW_ID_RLC_SRLG,
    AMDSMI_GUEST_FW_ID_RLC_SRLS,
    AMDSMI_GUEST_FW_ID_MEC,
    AMDSMI_GUEST_FW_ID_MEC2,
    AMDSMI_GUEST_FW_ID_SOS,
    AMDSMI_GUEST_FW_ID_ASD,
    AMDSMI_GUEST_FW_ID_TA_RAS,
    AMDSMI_GUEST_FW_ID_TA_XGMI,
    AMDSMI_GUEST_FW_ID_SMC,
    AMDSMI_GUEST_FW_ID_SDMA,
    AMDSMI_GUEST_FW_ID_SDMA2,
    AMDSMI_GUEST_FW_ID_VCN,
    AMDSMI_GUEST_FW_ID_DMCU,
    AMDSMI_GUEST_FW_ID__MAX
} amdsmi_guest_fw_engine_id_t;

/**
 * @brief VF Config Flags
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_VF_CONFIG_FB_SIZE_SET = 0,
    AMDSMI_VF_CONFIG_FB_OFFSET_SET,
    AMDSMI_VF_CONFIG_GFX_TIMESLICE_US_SET,
    AMDSMI_VF_CONFIG_ENG_COMPUTE_BW_SET,
    AMDSMI_VF_CONFIG_GUARD_THRESHOLD_FLR_SET,
    AMDSMI_VF_CONFIG_GUARD_THRESHOLD_EXCL_MOD_SET,
    AMDSMI_VF_CONFIG_GUARD_THRESHOLD_EXCL_TIMEOUT_SET,
    AMDSMI_VF_CONFIG_GUARD_THRESHOLD_ALL_INT_SET,
    AMDSMI_VF_CONFIG_ENG_ENCODE_BW_UVD_SET,
    AMDSMI_VF_CONFIG_ENG_ENCODE_BW_VCE_SET,
    AMDSMI_VF_CONFIG_ENG_ENCODE_BW_UVD1_SET,
    AMDSMI_VF_CONFIG_ENG_ENCODE_BW_VCN_SET,
    AMDSMI_VF_CONFIG_ENG_ENCODE_BW_VCN1_SET,
    AMDSMI_VF_CONFIG__MAX
} amdsmi_vf_config_flags_t;

/**
 * @brief VF Schedule State
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_VF_STATE_UNAVAILABLE,
    AMDSMI_VF_STATE_AVAILABLE,
    AMDSMI_VF_STATE_ACTIVE,
    AMDSMI_VF_STATE_SUSPENDED,
    AMDSMI_VF_STATE_FULLACCESS,
    AMDSMI_VF_STATE_DEFAULT_AVAILABLE,
} amdsmi_vf_sched_state_t;

/**
 * @brief Guard Event
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_GUARD_EVENT_FLR,
    AMDSMI_GUARD_EVENT_EXCLUSIVE_MOD,
    AMDSMI_GUARD_EVENT_EXCLUSIVE_TIMEOUT,
    AMDSMI_GUARD_EVENT_ALL_INT,
    AMDSMI_GUARD_EVENT_RAS_ERR_COUNT,
    AMDSMI_GUARD_EVENT_RAS_CPER_DUMP,
    AMDSMI_GUARD_EVENT_RAS_BAD_PAGES,
    AMDSMI_GUARD_EVENT__MAX
} amdsmi_guard_type_t;

/**
 * @brief Driver
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_DRIVER_LIBGV,
    AMDSMI_DRIVER_KMD,
    AMDSMI_DRIVER_AMDGPUV,
    AMDSMI_DRIVER_AMDGPU,
    AMDSMI_DRIVER_VMWGPUV,
    AMDSMI_DRIVER__MAX,
} amdsmi_driver_t;

/**
 * @brief Guard State
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_GUARD_STATE_NORMAL   = 0,
    AMDSMI_GUARD_STATE_FULL     = 1,
    AMDSMI_GUARD_STATE_OVERFLOW = 2,
} amdsmi_guard_state_t;

/**
 * @brief Schedule Block
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_SCHED_BLOCK_GFX     = 0x0,
    AMDSMI_SCHED_BLOCK_UVD     = 0x1,
    AMDSMI_SCHED_BLOCK_VCE     = 0x2,
    AMDSMI_SCHED_BLOCK_UVD1    = 0x3,
    AMDSMI_SCHED_BLOCK_VCN     = 0x4,
    AMDSMI_SCHED_BLOCK_VCN1    = 0x5,
} amdsmi_sched_block_t;

/**
 * @brief Guest firmware load status.
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_GUEST_FW_LOAD_STATUS_OK              = 0,
    AMDSMI_GUEST_FW_LOAD_STATUS_OBSOLETE_FW     = 1,
    AMDSMI_GUEST_FW_LOAD_STATUS_BAD_SIG         = 2,
    AMDSMI_GUEST_FW_LOAD_STATUS_FW_LOAD_FAIL    = 3,
    AMDSMI_GUEST_FW_LOAD_STATUS_ERR_GENERIC     = 4
} amdsmi_guest_fw_load_status_t;

/**
 * @brief  XGMI FB Sharing Mode
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM   = 0,
    AMDSMI_XGMI_FB_SHARING_MODE_1        = 1,
    AMDSMI_XGMI_FB_SHARING_MODE_2        = 2,
    AMDSMI_XGMI_FB_SHARING_MODE_4        = 4,
    AMDSMI_XGMI_FB_SHARING_MODE_8        = 8,
    AMDSMI_XGMI_FB_SHARING_MODE_UNKNOWN  = 0xFFFFFFFF
} amdsmi_xgmi_fb_sharing_mode_t;

/**
 * @brief Profile Capability
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_PROFILE_CAPABILITY_MEMORY  = 0,  //!< memory
    AMDSMI_PROFILE_CAPABILITY_ENCODE  = 1,  //!< encode engine
    AMDSMI_PROFILE_CAPABILITY_DECODE  = 2,  //!< decode engine
    AMDSMI_PROFILE_CAPABILITY_COMPUTE = 3,  //!< compute engine
    AMDSMI_PROFILE_CAPABILITY__MAX,
} amdsmi_profile_capability_type_t;

/**
 * @brief Metric Category
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_METRIC_CATEGORY_ACC_COUNTER,
    AMDSMI_METRIC_CATEGORY_FREQUENCY,
    AMDSMI_METRIC_CATEGORY_ACTIVITY,
    AMDSMI_METRIC_CATEGORY_TEMPERATURE,
    AMDSMI_METRIC_CATEGORY_POWER,
    AMDSMI_METRIC_CATEGORY_ENERGY,
    AMDSMI_METRIC_CATEGORY_THROTTLE,
    AMDSMI_METRIC_CATEGORY_PCIE,
    AMDSMI_METRIC_CATEGORY_STATIC,
    AMDSMI_METRIC_CATEGORY_SYS_ACC_COUNTER,
    AMDSMI_METRIC_CATEGORY_SYS_BASEBOARD_TEMP,
    AMDSMI_METRIC_CATEGORY_SYS_GPUBOARD_TEMP,
    AMDSMI_METRIC_CATEGORY_UNKNOWN
} amdsmi_metric_category_t;

/**
 * @brief Metric Name
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_METRIC_NAME_METRIC_ACC_COUNTER,
    AMDSMI_METRIC_NAME_FW_TIMESTAMP,
    AMDSMI_METRIC_NAME_CLK_GFX,
    AMDSMI_METRIC_NAME_CLK_SOC,
    AMDSMI_METRIC_NAME_CLK_MEM,
    AMDSMI_METRIC_NAME_CLK_VCLK,
    AMDSMI_METRIC_NAME_CLK_DCLK,

    AMDSMI_METRIC_NAME_USAGE_GFX,
    AMDSMI_METRIC_NAME_USAGE_MEM,
    AMDSMI_METRIC_NAME_USAGE_MM,
    AMDSMI_METRIC_NAME_USAGE_VCN,
    AMDSMI_METRIC_NAME_USAGE_JPEG,

    AMDSMI_METRIC_NAME_VOLT_GFX,
    AMDSMI_METRIC_NAME_VOLT_SOC,
    AMDSMI_METRIC_NAME_VOLT_MEM,

    AMDSMI_METRIC_NAME_TEMP_HOTSPOT_CURR,
    AMDSMI_METRIC_NAME_TEMP_HOTSPOT_LIMIT,
    AMDSMI_METRIC_NAME_TEMP_MEM_CURR,
    AMDSMI_METRIC_NAME_TEMP_MEM_LIMIT,
    AMDSMI_METRIC_NAME_TEMP_VR_CURR,
    AMDSMI_METRIC_NAME_TEMP_SHUTDOWN,

    AMDSMI_METRIC_NAME_POWER_CURR,
    AMDSMI_METRIC_NAME_POWER_LIMIT,

    AMDSMI_METRIC_NAME_ENERGY_SOCKET,
    AMDSMI_METRIC_NAME_ENERGY_CCD,
    AMDSMI_METRIC_NAME_ENERGY_XCD,
    AMDSMI_METRIC_NAME_ENERGY_AID,
    AMDSMI_METRIC_NAME_ENERGY_MEM,

    AMDSMI_METRIC_NAME_THROTTLE_SOCKET_ACTIVE,
    AMDSMI_METRIC_NAME_THROTTLE_VR_ACTIVE,
    AMDSMI_METRIC_NAME_THROTTLE_MEM_ACTIVE,

    AMDSMI_METRIC_NAME_PCIE_BANDWIDTH,
    AMDSMI_METRIC_NAME_PCIE_L0_TO_RECOVERY_COUNT,
    AMDSMI_METRIC_NAME_PCIE_REPLAY_COUNT,
    AMDSMI_METRIC_NAME_PCIE_REPLAY_ROLLOVER_COUNT,
    AMDSMI_METRIC_NAME_PCIE_NAK_SENT_COUNT,
    AMDSMI_METRIC_NAME_PCIE_NAK_RECEIVED_COUNT,

    AMDSMI_METRIC_NAME_CLK_GFX_MAX_LIMIT,
    AMDSMI_METRIC_NAME_CLK_SOC_MAX_LIMIT,
    AMDSMI_METRIC_NAME_CLK_MEM_MAX_LIMIT,
    AMDSMI_METRIC_NAME_CLK_VCLK_MAX_LIMIT,
    AMDSMI_METRIC_NAME_CLK_DCLK_MAX_LIMIT,

    AMDSMI_METRIC_NAME_CLK_GFX_MIN_LIMIT,
    AMDSMI_METRIC_NAME_CLK_SOC_MIN_LIMIT,
    AMDSMI_METRIC_NAME_CLK_MEM_MIN_LIMIT,
    AMDSMI_METRIC_NAME_CLK_VCLK_MIN_LIMIT,
    AMDSMI_METRIC_NAME_CLK_DCLK_MIN_LIMIT,

    AMDSMI_METRIC_NAME_CLK_GFX_LOCKED,

    AMDSMI_METRIC_NAME_CLK_GFX_DS_DISABLED,
    AMDSMI_METRIC_NAME_CLK_MEM_DS_DISABLED,
    AMDSMI_METRIC_NAME_CLK_SOC_DS_DISABLED,
    AMDSMI_METRIC_NAME_CLK_VCLK_DS_DISABLED,
    AMDSMI_METRIC_NAME_CLK_DCLK_DS_DISABLED,

    AMDSMI_METRIC_NAME_PCIE_LINK_SPEED,
    AMDSMI_METRIC_NAME_PCIE_LINK_WIDTH,

    AMDSMI_METRIC_NAME_DRAM_BANDWIDTH,
    AMDSMI_METRIC_NAME_MAX_DRAM_BANDWIDTH,

    AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_PPT,
    AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_THM,
    AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_TOTAL,
    AMDSMI_METRIC_NAME_GFX_CLK_LOW_UTILIZATION,
    AMDSMI_METRIC_NAME_INPUT_TELEMETRY_VOLTAGE,
    AMDSMI_METRIC_NAME_PLDM_VERSION,
    AMDSMI_METRIC_NAME_TEMP_XCD,
    AMDSMI_METRIC_NAME_TEMP_AID,
    AMDSMI_METRIC_NAME_TEMP_HBM,

    AMDSMI_METRIC_NAME_SYS_METRIC_ACC_COUNTER,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FRONT,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_BACK,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM7,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_IBC,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_UFPGA,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM1,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_HSC,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_2_3_HSC,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_HSC,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_6_7_HSC,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_0V72_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_3V3_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_0V9_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_0V9_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_2_3_0V9_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_6_7_0V9_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC_HSC,
    AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC,
    AMDSMI_METRIC_NAME_NODE_TEMP_RETIMER,
    AMDSMI_METRIC_NAME_NODE_TEMP_IBC_TEMP,
    AMDSMI_METRIC_NAME_NODE_TEMP_IBC_2_TEMP,
    AMDSMI_METRIC_NAME_NODE_TEMP_VDD18_VR_TEMP,
    AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_B_VR_TEMP,
    AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_D_VR_TEMP,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD0,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD1,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD2,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD3,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_A,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_C,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_A,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_C,
    AMDSMI_METRIC_NAME_VR_TEMP_VDD_085_HBM,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_B,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_D,
    AMDSMI_METRIC_NAME_VR_TEMP_VDD_USR,
    AMDSMI_METRIC_NAME_VR_TEMP_VDDIO_11_E32,

    AMDSMI_METRIC_NAME_UNKNOWN
} amdsmi_metric_name_t;

/**
 * @brief Metric Unit
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_METRIC_UNIT_COUNTER,
    AMDSMI_METRIC_UNIT_UINT,
    AMDSMI_METRIC_UNIT_BOOL,
    AMDSMI_METRIC_UNIT_MHZ,
    AMDSMI_METRIC_UNIT_PERCENT,
    AMDSMI_METRIC_UNIT_MILLIVOLT,
    AMDSMI_METRIC_UNIT_CELSIUS,
    AMDSMI_METRIC_UNIT_WATT,
    AMDSMI_METRIC_UNIT_JOULE,
    AMDSMI_METRIC_UNIT_GBPS,
    AMDSMI_METRIC_UNIT_MBITPS,
    AMDSMI_METRIC_UNIT_PCIE_GEN,
    AMDSMI_METRIC_UNIT_PCIE_LANES,
    AMDSMI_METRIC_UNIT_15_625_MILLIJOULE,
    AMDSMI_METRIC_UNIT_UNKNOWN
} amdsmi_metric_unit_t;

/**
 * @brief Metric Type
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_METRIC_TYPE_COUNTER = (1 << 0),  //!< counter metric
    AMDSMI_METRIC_TYPE_CHIPLET = (1 << 1),  //!< chiplet metric
    AMDSMI_METRIC_TYPE_INST = (1 << 2),     //!< instantaneous metric
    AMDSMI_METRIC_TYPE_ACC = (1 << 3)       //!< accumulated metric
} amdsmi_metric_type_t;

typedef enum {
    AMDSMI_METRIC_RES_GROUP_UNKNOWN,
    AMDSMI_METRIC_RES_GROUP_NA,
    AMDSMI_METRIC_RES_GROUP_GPU,
    AMDSMI_METRIC_RES_GROUP_XCP,
    AMDSMI_METRIC_RES_GROUP_AID,
    AMDSMI_METRIC_RES_GROUP_MID,
    AMDSMI_METRIC_RES_GROUP_SYSTEM
} amdsmi_metric_res_group_t;

typedef enum {
    AMDSMI_METRIC_RES_SUBGROUP_UNKNOWN,
    AMDSMI_METRIC_RES_SUBGROUP_NA,
    AMDSMI_METRIC_RES_SUBGROUP_XCC,
    AMDSMI_METRIC_RES_SUBGROUP_ENGINE,
    AMDSMI_METRIC_RES_SUBGROUP_HBM,
    AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD,
    AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD
} amdsmi_metric_res_subgroup_t;

typedef enum {
    AMDSMI_VF_MODE_1   = (1U << 1),
    AMDSMI_VF_MODE_2   = (1U << 2),
    AMDSMI_VF_MODE_4   = (1U << 4),
    AMDSMI_VF_MODE_8   = (1U << 8),
    AMDSMI_VF_MODE_ALL = (AMDSMI_VF_MODE_1 | AMDSMI_VF_MODE_2 | AMDSMI_VF_MODE_4 | AMDSMI_VF_MODE_8)    //!< All VF counts supported
} amdsmi_vf_mode_t;

/**
 * @brief The values of this enum are used to identify driver model type
 *
 * @cond @tag{host} @endcond
 */
typedef enum {
    AMDSMI_DRIVER_MODEL_TYPE_WDDM = 0,
    AMDSMI_DRIVER_MODEL_TYPE_WDM  = 1,
    AMDSMI_DRIVER_MODEL_TYPE_MCDM = 2,
    AMDSMI_DRIVER_MODEL_TYPE__MAX = 3,
} amdsmi_driver_model_type_t;

/**
 * @brief  VF Handle
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint64_t handle;
} amdsmi_vf_handle_t;

/**
 * @brief Event Entry
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    amdsmi_vf_handle_t	fcn_id;
    uint64_t            dev_id;
    uint64_t            timestamp;                          //!< UTC microseconds
    uint64_t            data;
    uint32_t            category;
    uint32_t            subcode;
    uint32_t            level;
    char                date[AMDSMI_MAX_DATE_STRING_LENGTH];     //!< UTC date and time
    char                message[AMDSMI_MAX_STRING_LENGTH];
    amdsmi_processor_handle processor_handle;
    uint64_t            reserved[37];
} amdsmi_event_entry_t;

/**
 * @brief Handshake
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint32_t version;
} amdsmi_handshake_t;

/**
 * @brief PF FB Information
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint32_t total_fb_size;     //!< Total GPU fb size in MB
    uint32_t pf_fb_reserved;    //!< Total fb consumed by PF
    uint32_t pf_fb_offset;      //!< PF FB offset
    uint32_t fb_alignment;      //!< FB alignment
    uint32_t max_vf_fb_usable;  //!< Maximum usable fb size in MB
    uint32_t min_vf_fb_usable;  //!< Minimum usable fb size in MB
    uint64_t reserved[5];
} amdsmi_pf_fb_info_t;

/**
 * @brief VF FB Information
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint32_t fb_offset; //!< Offset in MB from start of the framebuffer
    uint32_t fb_size;   //!< Size in MB Must be divisible by 16 and not less than 256
    uint64_t reserved[3];
} amdsmi_vf_fb_info_t;

/**
 * @brief Partition Information
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    amdsmi_vf_handle_t id;
    amdsmi_vf_fb_info_t fb;
    uint64_t reserved[3];
} amdsmi_partition_info_t;

/**
 * @brief Guard Information
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint8_t enabled;
    struct {
        amdsmi_guard_state_t state;
        /* amount of monitor event after enabled */
        uint32_t amount;
        /* threshold of events in the interval(seconds) */
        uint64_t interval;
        uint32_t threshold;
        /* current number of events in the interval*/
        uint32_t active;
        uint32_t reserved[4];
    } guard[AMDSMI_GUARD_EVENT__MAX];
    uint32_t reserved[6];
} amdsmi_guard_info_t;

/**
 * @brief VF Information
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    amdsmi_vf_fb_info_t fb;
    uint32_t gfx_timeslice;  //!< Graphics timeslice in us, maximum value is 1000 ms
    uint64_t reserved[27];
} amdsmi_vf_info_t;

/**
 * @brief Schedule Information
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint64_t        flr_count;
    uint64_t        boot_up_time; //!< in microseconds
    uint64_t        shutdown_time;
    uint64_t        reset_time;
    amdsmi_vf_sched_state_t state;
    char            last_boot_start[AMDSMI_MAX_STRING_LENGTH];
    char            last_boot_end[AMDSMI_MAX_STRING_LENGTH];
    char            last_shutdown_start[AMDSMI_MAX_STRING_LENGTH];
    char            last_shutdown_end[AMDSMI_MAX_STRING_LENGTH];
    char            last_reset_start[AMDSMI_MAX_STRING_LENGTH];
    char            last_reset_end[AMDSMI_MAX_STRING_LENGTH];
    char            current_active_time[AMDSMI_MAX_STRING_LENGTH];  //!< Current session VF time, reset after guest reload
    char            current_running_time[AMDSMI_MAX_STRING_LENGTH];
    char            total_active_time[AMDSMI_MAX_STRING_LENGTH];    //!< Cumulate across entire server lifespan, reset after host reload
    char            total_running_time[AMDSMI_MAX_STRING_LENGTH];   //!< Not implemented
    uint64_t reserved[11];
} amdsmi_sched_info_t;

/**
 * @brief VF Data
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    amdsmi_sched_info_t sched;
    amdsmi_guard_info_t guard;
    uint64_t reserved[8];
} amdsmi_vf_data_t;

/**
 * @brief Profile Caps Information
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint64_t total;
    uint64_t available;
    uint64_t optimal;
    uint64_t min_value;
    uint64_t max_value;
    uint64_t reserved[2];
} amdsmi_profile_caps_info_t;

/**
 * @brief Profile Information
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint8_t profile_count;
    uint8_t current_profile_index;
    struct {
        uint32_t vf_count;
        amdsmi_profile_caps_info_t profile_caps[AMDSMI_PROFILE_CAPABILITY__MAX];
    } profiles[AMDSMI_MAX_PROFILE_COUNT];
    uint32_t reserved[6];
} amdsmi_profile_info_t;

/**
 * @brief Guest Data
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    char driver_version[AMDSMI_MAX_STRING_LENGTH];
    uint32_t fb_usage;  //!<  guest framebuffer usage in MB
    uint64_t reserved[23];
} amdsmi_guest_data_t;

/**
 * @brief DFC Firmware Header
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint32_t dfc_fw_version;
    uint32_t dfc_fw_total_entries;
    uint32_t dfc_gart_wr_guest_min;
    uint32_t dfc_gart_wr_guest_max;
    uint32_t reserved[12];
} amdsmi_dfc_fw_header_t;

/**
 * @brief DFC Firmware White List
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint32_t oldest;
    uint32_t latest;
} amdsmi_dfc_fw_white_list_t;

/**
 * @brief DFC Firmware TA UUID
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint8_t ta_uuid[AMDSMI_MAX_UUID_ELEMENTS];
} amdsmi_dfc_fw_ta_uuid_t;

/**
 * @brief DFC Firmware Data
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint32_t dfc_fw_type;
    uint32_t verification_enabled;
    uint32_t customer_ordinal;  //!< only used in driver version on NV32+
    uint32_t reserved[13];
    union {
        amdsmi_dfc_fw_white_list_t white_list[AMDSMI_MAX_WHITE_LIST_ELEMENTS];
        amdsmi_dfc_fw_ta_uuid_t ta_white_list[AMDSMI_MAX_TA_WHITE_LIST_ELEMENTS];
    };
    uint32_t black_list[AMDSMI_MAX_BLACK_LIST_ELEMENTS];
} amdsmi_dfc_fw_data_t;

/**
 * @brief DFC Firmware
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    amdsmi_dfc_fw_header_t header;
    amdsmi_dfc_fw_data_t data[AMDSMI_DFC_FW_NUMBER_OF_ENTRIES];
} amdsmi_dfc_fw_t;

/**
 * @brief Structure representing an EEPROM table record for tracking memory errors
 *
 * This structure contains information about retired memory pages, including
 * the page address, timestamp of the error, error type, and related memory
 * subsystem identifiers.
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint64_t retired_page;  //!< Bad page frame address
    uint64_t ts;
    unsigned char err_type;
    union {
        unsigned char bank;
        unsigned char cu;
    };
    unsigned char mem_channel;
    unsigned char mcumc_id;
    uint32_t reserved[3];
} amdsmi_eeprom_table_record_t;

/**
 * @brief Firmware Load Error Record
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint64_t timestamp;  //!< UTC microseconds
    uint32_t vf_idx;
    uint32_t fw_id;
    uint16_t status;     //!< amdsmi_guest_fw_load_status
    uint32_t reserved[3];
} amdsmi_fw_load_error_record_t;

/**
 * @brief Firmware Error Record
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint8_t num_err_records;
    amdsmi_fw_load_error_record_t err_records[AMDSMI_MAX_ERR_RECORDS];
    uint64_t reserved[7];
} amdsmi_fw_error_record_t;

/**
 * @brief Link Topology
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint64_t weight;                   //!< link weight
    amdsmi_link_status_t link_status;  //!< HW status of the link
    amdsmi_link_type_t link_type;      //!< type of the link
    uint8_t  num_hops;                 //!< number of hops
    uint8_t  fb_sharing;               //!< framebuffer sharing flag
    uint32_t reserved[10];
} amdsmi_link_topology_t;

/**
 * @brief Topology Nearest
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint32_t count;
    amdsmi_processor_handle processor_list[AMDSMI_MAX_DEVICES];
    uint64_t reserved[15];
} amdsmi_topology_nearest_t;

/**
 * @brief XGMI FB Sharing Caps
 *
 * @cond @tag{host} @endcond
 */
typedef union {
    struct cap_ {
        uint32_t mode_custom_cap :1;
        uint32_t mode_1_cap      :1;
        uint32_t mode_2_cap      :1;
        uint32_t mode_4_cap      :1;
        uint32_t mode_8_cap      :1;
        uint32_t reserved        :27;
    } cap;
    uint32_t xgmi_fb_sharing_cap_mask;
} amdsmi_xgmi_fb_sharing_caps_t;

/**
 * @brief Metric
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    amdsmi_metric_unit_t unit;
    amdsmi_metric_name_t name;
    amdsmi_metric_category_t category;
    uint32_t flags;                             //!< used to determine type of the metric (amdsmi_metric_type_t)
    uint32_t vf_mask;                           //!< Mask of all active VFs + PF that this metric applies to
    uint64_t val;
    amdsmi_metric_res_group_t res_group;        //!< Resource group this metric belongs to
    amdsmi_metric_res_subgroup_t res_subgroup;  //!< Resource subgroup this metric belongs to
    uint32_t res_instance;                      //!< Resource instance this metric belongs to
    uint32_t reserved[5];                       //!< Reserved for future use
} amdsmi_metric_t;

/**
 * @brief This structure holds version information.
 *
 * @cond @tag{host} @endcond
 */
typedef struct {
    uint32_t major;    //!< Major version
    uint32_t minor;    //!< Minor version
    uint32_t release;  //!< Patch, build or stepping version
} amdsmi_version_t;

typedef struct {
    amdsmi_accelerator_partition_profile_t profile;
    uint32_t vf_mode;       //!< Bitmask of VF modes (see amdsmi_vf_mode_t)
    uint64_t reserved[6];
} amdsmi_accelerator_partition_profile_global_t;

typedef struct {
    uint32_t num_profiles;          //!< The length of profiles array
    uint32_t num_resource_profiles;
    amdsmi_accelerator_partition_resource_profile_t resource_profiles[AMDSMI_MAX_CP_PROFILE_RESOURCES];
    uint32_t default_profile_index; //!< The index of the default profile in the profiles array
    amdsmi_accelerator_partition_profile_global_t profiles[AMDSMI_MAX_ACCELERATOR_PROFILE];
    uint64_t reserved[30];
} amdsmi_accelerator_partition_profile_config_global_t;

/**
 * @brief Maximum size definitions AMDSMI NIC
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
#define AMDSMI_MAX_NIC_PORTS              32  //!< Maximum number of NIC ports
#define AMDSMI_MAX_NIC_RDMA_DEV           32  //!< Maximum number of NIC RDMA devices
#define AMDSMI_MAX_NIC_FW                 16  //!< Maximum number of NIC firmwares

/**
 * @brief Structure for NIC statistic name-value pairs
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 *
 * This structure represents a single NIC statistic with its name and value.
 */
typedef struct {
    char name[AMDSMI_MAX_STRING_LENGTH];
    uint64_t value;
} amdsmi_nic_stat_t;

/**
 * @brief NIC asic information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint16_t vendor_id;
    uint16_t subvendor_id;
    uint16_t device_id;
    uint16_t subsystem_id;
    uint8_t revision;
    char permanent_address[AMDSMI_MAX_STRING_LENGTH];
    char product_name[AMDSMI_MAX_STRING_LENGTH];
    char part_number[AMDSMI_MAX_STRING_LENGTH];
    char serial_number[AMDSMI_MAX_STRING_LENGTH];
    char vendor_name[AMDSMI_MAX_STRING_LENGTH];
} amdsmi_nic_asic_info_t;

/**
 * @brief NIC bus information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    amdsmi_bdf_t bdf;
    uint8_t max_pcie_width;
    uint32_t max_pcie_speed; //!< maximum PCIe speed in GT/s
    char pcie_interface_version[AMDSMI_MAX_STRING_LENGTH];
    char slot_type[AMDSMI_MAX_STRING_LENGTH];
} amdsmi_nic_bus_info_t;

/**
 * @brief NIC NUMA information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint8_t node;
    char affinity[AMDSMI_MAX_STRING_LENGTH];
} amdsmi_nic_numa_info_t;

/**
 * @brief NIC firmware information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    char name[AMDSMI_MAX_STRING_LENGTH];
    char version[AMDSMI_MAX_STRING_LENGTH];
} amdsmi_nic_fw_t;

/**
 * @brief NIC firmware information collection
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint32_t num_fw;
    amdsmi_nic_fw_t fw[AMDSMI_MAX_NIC_FW];
} amdsmi_nic_fw_info_t;

/**
 * @brief NIC port information
 *
 * Active FEC Modes:
 * The active_fec field provides a bitmask representation of Active FEC (Active Forward Error Correction) modes.
 * The bitmask values are derived from the `ethtool_fecparam` structure, specifically
 * the `active_fec` field. Below are examples of the defined FEC modes:
 *
 * Examples:
 * - ETHTOOL_FEC_NONE  (0x01)
 * - ETHTOOL_FEC_AUTO  (0x02)
 * - ETHTOOL_FEC_RS    (0x04)
 * - ETHTOOL_FEC_BASER (0x08)
 * - ETHTOOL_FEC_LLRS  (0x10)
 * - ETHTOOL_FEC_OFF   (0x20)
 *
 * Note: These definitions are based on the latest available ethtool information. Users should
 * verify if there are any updates or changes to these definitions in the relevant ethtool
 * structure or field before implementing them in their code.
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    amdsmi_bdf_t bdf;
    uint32_t port_num;
    char type[AMDSMI_MAX_STRING_LENGTH];
    char flavour[AMDSMI_MAX_STRING_LENGTH];
    char netdev[AMDSMI_MAX_STRING_LENGTH];
    uint8_t ifindex;
    char mac_address[AMDSMI_MAX_STRING_LENGTH];
    uint8_t carrier;
    uint16_t mtu;
    char link_state[AMDSMI_MAX_STRING_LENGTH];
    uint32_t link_speed;
    uint32_t active_fec;   //!< Active FEC modes bitmask (see about FEC modes in the description)
    char autoneg[AMDSMI_MAX_STRING_LENGTH];
    char pause_autoneg[AMDSMI_MAX_STRING_LENGTH];
    char pause_rx[AMDSMI_MAX_STRING_LENGTH];
    char pause_tx[AMDSMI_MAX_STRING_LENGTH];
} amdsmi_nic_port_t;

/**
 * @brief NIC port information collection
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint32_t num_ports;
    amdsmi_nic_port_t ports[AMDSMI_MAX_NIC_PORTS];
} amdsmi_nic_port_info_t;

/**
 * @brief NIC driver information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    char name[AMDSMI_MAX_STRING_LENGTH];
    char version[AMDSMI_MAX_STRING_LENGTH];
} amdsmi_nic_driver_info_t;

/**
 * @brief NIC RDMA port information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    char netdev[AMDSMI_MAX_STRING_LENGTH];
    char state[AMDSMI_MAX_STRING_LENGTH];
    uint8_t rdma_port;
    uint16_t max_mtu;
    uint16_t active_mtu;
} amdsmi_nic_rdma_port_info_t;

/**
 * @brief NIC RDMA device information
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    char rdma_dev[AMDSMI_MAX_STRING_LENGTH];
    char node_guid[AMDSMI_MAX_STRING_LENGTH];
    char node_type[AMDSMI_MAX_STRING_LENGTH];
    char sys_image_guid[AMDSMI_MAX_STRING_LENGTH];
    char fw_ver[AMDSMI_MAX_STRING_LENGTH];
    uint8_t num_rdma_ports;
    amdsmi_nic_rdma_port_info_t rdma_port_info[AMDSMI_MAX_NIC_PORTS];
} amdsmi_nic_rdma_dev_info_t;

/**
 * @brief NIC RDMA devices information collection
 *
 * @cond @tag{gpu_bm_linux} @tag{host} @endcond
 */
typedef struct {
    uint8_t num_rdma_dev;
    amdsmi_nic_rdma_dev_info_t rdma_dev_info[AMDSMI_MAX_NIC_RDMA_DEV];
} amdsmi_nic_rdma_devices_info_t;
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
 *  Singleton Design, requires the same number of inits as shutdowns.
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
 *  performs any necessary clean ups. Singleton Design, requires the same number
 *  of inits as shutdowns.
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
 *  @param[out] processor_type a pointer to ::processor_type_t to which the processor type
 *  will be written. If this parameter is nullptr, this function will return
 *  ::AMDSMI_STATUS_INVAL.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_processor_type(amdsmi_processor_handle processor_handle,
                                          processor_type_t *processor_type);

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

/**
 *  @brief Retrieves an array of uint64_t (sized to cpu_set_size) of bitmasks with the
 *  affinity within numa node or socket for the device.
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details Given a processor handle @p processor_handle, the size of the cpu_set array @p cpu_set_size,
 *  and a pointer to an array of int64_t @p cpu_set, and @p scope, this function will write the CPU affinity bitmask
 *  to the array pointed to by @p cpu_set.
 *
 *  User must allocate the enough memory for the cpu_set array. The size of the array is determined by the
 *  number of CPU cores in the system. As an example, if there are 2 CPUs and each has 112 cores, the size
 *  should be ceiling(2*112/64) = 4, where 64 is the bits of uint64_t. The function will write the CPU affinity bitmask
 *  to the array. For example, to describe the CPU cores 0-55,112-167, it will set the 0-55 and 112-167 bits
 *  to 1 and the reset of bits to 0 in the cpu_set array.
 *
 *  @param[in] processor_handle a processor handle
 *  @param[in] cpu_set_size The size of the cpu_set array that is safe to access
 *  @param[in,out] cpu_set Array reference in which to return a bitmask of CPU cores that this processor affinities with.
 *  @param[in] scope Scope for socket or numa affinity.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_cpu_affinity_with_scope(amdsmi_processor_handle processor_handle,
            uint32_t cpu_set_size, uint64_t *cpu_set, amdsmi_affinity_scope_t scope);

/**
 *  @brief Get the node handle associated with processor handle.
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details This function retrieves the node handle of a processor handler. The
 *  @p processor_handle must be provided for the processor.
 *  Currently, only AMD GPUs are supported.
 *
 *  @param[in] processor_handle A pointer to a ::amdsmi_processor_handle values
 *  will be written.
 *
 *  @param[out] amdsmi_node_handle* A pointer to a block of memory where amdsmi_node_handle
 *  will be written.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_node_handle(amdsmi_processor_handle processor_handle, amdsmi_node_handle *node_handle);

/**
 *  @brief Returns VF handle from the given BDF
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{host}
 *
 *  @param[in] bdf BDF of the VF
 *
 *  @param[out] vf_handle Reference to the VF handle.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_vf_handle_from_bdf(amdsmi_bdf_t bdf, amdsmi_vf_handle_t *vf_handle);

/**
 *  @brief Returns the handle of a virtual function from the given UUID
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{host}
 *
 *  @param[in] uuid Function UUID to query.
 *
 *  @param[out] vf_handle Reference to the VF handle.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_vf_handle_from_uuid(const char *uuid, amdsmi_vf_handle_t *vf_handle);

/**
 *  @brief Returns the handle of a virtual function given its index
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[in] fcn_idx Function index to query
 *
 *  @param[out] vf_handle VF handle. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_vf_handle_from_vf_index(amdsmi_processor_handle processor_handle, uint32_t fcn_idx, amdsmi_vf_handle_t *vf_handle);

/**
 *  @brief Returns BDF of the given device (VF).
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{host}
 *
 *  @param[in] vf_handle VF for which to query
 *
 *  @param[out] bdf Reference to BDF. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_vf_bdf(amdsmi_vf_handle_t vf_handle, amdsmi_bdf_t *bdf);

/**
 *  @brief Returns the UUID of the VF
 *
 *  @ingroup tagProcDiscovery
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle VF for which to query
 *
 *  @param[inout] uuid_length Length of the uuid string. As inpout, must be
 *  equal or greater than AMDSMI_GPU_UUID_SIZE and be allocated by
 *  user. As output it is the length of the uuid string.
 *
 *  @param[out] uuid Pointer to string to store the UUID. Must be
 *  allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_vf_uuid(amdsmi_vf_handle_t processor_handle, unsigned int *uuid_length, char *uuid);

/**
 *  @brief Returns BDF of the given NIC device
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
amdsmi_status_t amdsmi_get_nic_device_bdf(amdsmi_processor_handle processor_handle, amdsmi_bdf_t *bdf);
/**
 *  @brief Returns a list of processor handles of the specified type in the system.
 *
 *  @platform{host} @platform{gpu_bm_linux} @platform{cpu_bm}
 *
 *  @note This function fills the user-provided buffer with processor handles of the given type
 *  (e.g., GPU, NIC). The processor handles returned are used to instantiate the rest of processor
 *  queries in the library. If the buffer is not large enough, the call will fail.
 *
 *  @param[in] socket_handle The socket to query.
 *
 *  @param[in] processor_type The type of processor to query (see ::processor_type_t).
 *
 *  @param[out] processor_handles Reference to list of processor handles returned by
 *  the library. Buffer must be allocated by user.
 *
 *  @param[in,out] processor_count As input, the size of the provided buffer.
 *  As output, number of processor handles in the buffer.
 *  Parameter must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_processor_handles_by_type(amdsmi_socket_handle socket_handle,
                                                     processor_type_t processor_type,
                                                     amdsmi_processor_handle* processor_handles,
                                                     uint32_t* processor_count);

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

/**
 *  @brief Returns the driver model information
 *
 *  @ingroup tagSoftwareVersion
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] model Reference to the enum representing driver model.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_driver_model(amdsmi_processor_handle processor_handle, amdsmi_driver_model_type_t *model);

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
 *  @brief Returns the power caps as currently configured in the system.
 *
 *  @ingroup tagAsicBoardInfo
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_windows}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[in] sensor_ind A 0-based sensor index. Normally, this will be 0.
 *  If a device has more than one sensor, it could be greater than 0.
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

/**
 *  @brief Returns vram info
 *
 *  @ingroup tagAsicBoardInfo
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] info Reference to vram info structure
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_vram_info(amdsmi_processor_handle processor_handle, amdsmi_vram_info_t *info);

/**
 *  @brief Returns the board part number and board information for the requested device
 *
 *  @ingroup tagAsicBoardInfo
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] info Reference to board info structure.
 *              Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_board_info(amdsmi_processor_handle processor_handle, amdsmi_board_info_t *info);

/**
 *  @brief Returns the framebuffer info for the ASIC.
 *
 *  @ingroup tagAsicBoardInfo
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] info Reference to framebuffer info.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_fb_layout(amdsmi_processor_handle processor_handle, amdsmi_pf_fb_info_t *info);

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

/**
 *  @brief Gets firmware error records
 *
 *  @ingroup tagFWVbiosQuery
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] records reference to the error records structure.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_fw_error_records(amdsmi_processor_handle processor_handle, amdsmi_fw_error_record_t *records);

/**
 *  @brief Returns the DFC fw table.
 *
 *  @ingroup tagFWVbiosQuery
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] info Reference to the dfc fw info. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_dfc_fw_table(amdsmi_processor_handle processor_handle, amdsmi_dfc_fw_t *info);

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
 *  @brief Returns is power management enabled
 *
 *  @ingroup tagGPUMonitor
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] enabled Reference to bool. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_is_gpu_power_management_enabled(amdsmi_processor_handle processor_handle, bool *enabled);

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
 *  @brief Return metrics information
 *
 *  @ingroup tagGPUMonitor
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[inout] metrics_size As input, the size of the provided buffer.
 *  As output, number of metrics in the buffer.
 *  Parameter must be allocated by user.
 *
 *  @param[out] metrics Reference to list of metrics returned by
 *  the library. Buffer must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_metrics(amdsmi_processor_handle processor_handle, uint32_t *metrics_size,
                                       amdsmi_metric_t *metrics);

/** @} End tagGPUMonitor */

/*****************************************************************************/
/** @defgroup tagMemoryPartition Partitioning Functions
 *  @{
 */

/**
 *  @brief Returns current gpu memory partition capabilities
 *
 *  @ingroup tagMemoryPartition
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[out] config reference to the memory partition config.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
 amdsmi_status_t
 amdsmi_get_gpu_memory_partition_config(amdsmi_processor_handle processor_handle,
                                        amdsmi_memory_partition_config_t *config);

/**
 *  @brief Sets memory partition mode
 *  Set memory partition setting based on memory_partition mode
 *  from amdsmi_get_gpu_memory_partition_config
 *
 *  @ingroup tagMemoryPartition
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details Given a processor handle @p processor_handle and a type of memory partition
 *  @p mode, this function will attempt to update the selected device's memory partition
 *  setting. This function does not allow any concurrent operations.
 *  Device must be idle and have no workloads when performing set partition operations.
 *
 *  @details On @platform{gpu_bm_linux} AMDGPU driver restart is REQUIRED to complete updating
 *  to the new memory partition setting. Refer to `amdsmi_gpu_driver_reload()` for more details.
 *
 *  @param[in] processor_handle A processor handle
 *
 *  @param[in] mode Enum representing memory partitioning mode to set
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_set_gpu_memory_partition_mode(amdsmi_processor_handle processor_handle, amdsmi_memory_partition_type_t mode);

/** @} End tagMemoryPartition */

/*****************************************************************************/
/** @defgroup tagAcceleratorPartition Accelerator Partitioning Functions
 *  @{
 */

/**
 *  @brief Returns gpu accelerator partition caps as currently configured in the system
 *
 *  @ingroup tagAcceleratorPartition
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *
 *  @note API requires admin/sudo privileges or API will not be able to read all resources
 *  for @platform{gpu_bm_linux} or any resources for @platform{host}.
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] profile_config reference to the accelerator partition config.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_gpu_accelerator_partition_profile_config(amdsmi_processor_handle processor_handle,
                                                    amdsmi_accelerator_partition_profile_config_t *profile_config);

/**
 *  @brief Returns current gpu accelerator partition cap
 *
 *  @ingroup tagAcceleratorPartition
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *
 *  @note API requires admin/sudo privileges or API will not be able to read all resources
 *  for @platform{gpu_bm_linux} or any resources for @platform{host}.
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] profile reference to the accelerator partition profile.
 *  Must be allocated by user.
 *
 *  @param[in,out] partition_id array of ids for current accelerator profile.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_gpu_accelerator_partition_profile(amdsmi_processor_handle processor_handle,
                                             amdsmi_accelerator_partition_profile_t *profile,
                                             uint32_t *partition_id);

/**
 *  @brief Set accelerator partition setting based on profile_index
 *  from amdsmi_get_gpu_accelerator_partition_profile_config
 *
 *  @ingroup tagAcceleratorPartition
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @note API requires admin/sudo privileges or API will not be able to read all resources
 *  for @platform{gpu_bm_linux} or any resources for @platform{host}.
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[in] profile_index Represents index of a partition user wants to set
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_set_gpu_accelerator_partition_profile(amdsmi_processor_handle processor_handle,
                                             uint32_t profile_index);

/**
 *  @brief Returns all GPU accelerator partition capabilities which can be configured on the system
 *
 *  @ingroup tagAcceleratorPartition
 *
 *  @platform{host}
 *
 *  @details This function retrieves the global accelerator partition profile configuration for the specified processor.
 *  The returned structure contains all possible partition profiles and their associated capabilities, including the supported
 *  VF modes for each profile. The VF modes are represented as a bitmask in the `vf_mode` field of each
 *  profile (see ::amdsmi_accelerator_partition_profile_global_t).
 *  To determine if specific VF mode is supported for a given profile, use the ::amdsmi_vf_mode_t enumeration as bitmask flags.
 *  For example, to check if 4 VF mode is supported, test if (vf_mode & AMDSMI_VF_MODE_4_SUPPORT) is non-zero.
 *  The bitmask can be used to extract support for 1, 2, 4, or 8 VF modes using the corresponding enum values.
 *
 *  @param[in] processor_handle PF of a processor for which to query.
 *
 *  @param[out] config Reference to the global accelerator partition config
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_gpu_accelerator_partition_profile_config_global(amdsmi_processor_handle processor_handle,
                                                           amdsmi_accelerator_partition_profile_config_global_t *config);

/** @} End tagAcceleratorPartition */

/*****************************************************************************/
/** @defgroup tagHWTopology Hardware Topology Functions
 *  @{
 */

/**
 *  @brief Return link metric information
 *
 *  @ingroup tagHWTopology
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] link_metrics reference to the link metrics struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_link_metrics(amdsmi_processor_handle processor_handle, amdsmi_link_metrics_t *link_metrics);

/**
 *  @brief Retrieve the set of GPUs that are nearest to a given device
 *         at a specific interconnectivity level.
 *
 *  @ingroup tagHWTopology
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details Once called topology_nearest_info will get populated with a list of
 *           all nearest devices for a given link_type. The list has a count of
 *           the number of devices found and their respective handles/identifiers.
 *
 *  @param[in] processor_handle The identifier of the given device.
 *
 *  @param[in] link_type The amdsmi_link_type_t level to search for nearest GPUs.
 *
 *  @param[in,out] topology_nearest_info
 *                 .count;
 *                   - When zero, set to the number of matching GPUs such that .device_list can be malloc'd.
 *                   - When non-zero, .device_list will be filled with count number of processor_handle.
 *                 .device_list An array of processor_handle for GPUs found at level.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail.
 */
amdsmi_status_t
amdsmi_get_link_topology_nearest(amdsmi_processor_handle processor_handle,
                                 amdsmi_link_type_t link_type,
                                 amdsmi_topology_nearest_t* topology_nearest_info);

/**
 *  @brief Retrieve connection type and P2P capabilities between 2 GPUs
 *
 *  @ingroup tagHWTopology
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf} @platform{guest_mvf}
 *
 *  @details Given a source processor handle @p processor_handle_src and
 *  a destination processor handle @p processor_handle_dst, a pointer to an amdsmi_link_type_t @p type,
 *  and a pointer to amdsmi_p2p_capability_t @p cap. This function will write the connection type,
 *  and io link capabilities between the device
 *  @p processor_handle_src and @p processor_handle_dst to the memory
 *  pointed to by @p cap and @p type.
 *
 *  @param[in] processor_handle_src the source processor handle
 *
 *  @param[in] processor_handle_dst the destination processor handle
 *
 *  @param[in,out] type A pointer to an ::amdsmi_link_type_t to which the
 *  type for the connection should be written.
 *
 *  @param[in,out] cap A pointer to an ::amdsmi_p2p_capability_t to which the
 *  io link capabilities should be written.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_topo_get_p2p_status(amdsmi_processor_handle processor_handle_src,
                                           amdsmi_processor_handle processor_handle_dst,
                                           amdsmi_link_type_t *type, amdsmi_p2p_capability_t *cap);

/**
 *  @brief Retrieve the NUMA CPU node number for a device
 *
 *  @ingroup tagHWTopology
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details Given a processor handle @p processor_handle, and a pointer to an
 *  uint32_t @p numa_node, this function will write the
 *  node number of NUMA CPU for the device @p processor_handle to the memory
 *  pointed to by @p numa_node.
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in,out] numa_node A pointer to an uint32_t to which the
 *  numa node number should be written.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_topo_get_numa_node_number(amdsmi_processor_handle processor_handle, uint32_t *numa_node);

/**
 *  @brief Return link topology information between two connected processors
 *
 *  @ingroup tagHWTopology
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle_src Source PF of a processor for which to query
 *
 *  @param[in] processor_handle_dst Destination PF of a processor for which to query
 *
 *  @param[out] topology_info reference to the link topology struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_link_topology(amdsmi_processor_handle processor_handle_src,
                                         amdsmi_processor_handle processor_handle_dst,
                                         amdsmi_link_topology_t *topology_info);

/**
 *  @brief Return XGMI capabilities
 *
 *  @ingroup tagHWTopology
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] caps reference to the xgmi caps union.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_xgmi_fb_sharing_caps(amdsmi_processor_handle processor_handle,
                                                amdsmi_xgmi_fb_sharing_caps_t *caps);

/**
 *  @brief Return XGMI framebuffer sharing information between two GPUs
 *
 *  @ingroup tagHWTopology
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle_src Source PF of a processor for which to query
 *
 *  @param[in] processor_handle_dst Destination PF of a processor for which to query
 *
 *  @param[in] mode Enum representing the framebuffer sharing mode to query
 *
 *  @param[out] fb_sharing Indicates framebuffer sharing between two processors.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_xgmi_fb_sharing_mode_info(amdsmi_processor_handle processor_handle_src,
                                                     amdsmi_processor_handle processor_handle_dst,
                                                     amdsmi_xgmi_fb_sharing_mode_t mode,
                                                     uint8_t *fb_sharing);

/**
 *  @brief Set XGMI framebuffer sharing mode
 *
 *  @ingroup tagHWTopology
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[in] mode Enum representing the framebuffer sharing mode to set
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_set_xgmi_fb_sharing_mode(amdsmi_processor_handle processor_handle,
                                                amdsmi_xgmi_fb_sharing_mode_t mode);

/**
 *  @brief Set XGMI framebuffer custom sharing mode.
 *
 *  @ingroup tagHWTopology
 *
 *  @platform{host}
 *
 *  @note This API will only work if there's no guest VM running.
 *
 *  @param[in] processor_list The list of processors
 *
 *  @param[in] num_processors The number of processors in the list
 *
 *  @param[in] mode Enum representing the framebuffer sharing mode to set
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_set_xgmi_fb_sharing_mode_v2(amdsmi_processor_handle *processor_list,
                                                   uint32_t num_processors,
                                                   amdsmi_xgmi_fb_sharing_mode_t mode);

/** @} End tagHWTopology */

/*****************************************************************************/
/** @defgroup tagClkPowerPerfControl Clock, Power and Performance Control
 *  These functions provide control over clock frequencies, power and
 *  performance.
 *  @{
 */

/**
 *  @brief Get the soc pstate policy for the processor
 *
 *  @ingroup tagClkPowerPerfControl
 *
 *  @platform{gpu_bm_linux} @platform{guest_1vf} @platform{host}
 *
 *  @details Given a processor handle @p processor_handle, this function will write
 *  current soc pstate  policy settings to @p policy. All the processors at the same socket
 *  will have the same policy.
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in,out] policy the soc pstate policy for this processor.
 *  If this parameter is nullptr, this function will return
 *  ::AMDSMI_STATUS_INVAL
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_soc_pstate(amdsmi_processor_handle processor_handle,
                                      amdsmi_dpm_policy_t* policy);

/**
 *  @brief Set the soc pstate policy for the processor
 *
 *  @ingroup tagClkPowerPerfControl
 *
 *  @platform{gpu_bm_linux} @platform{guest_1vf} @platform{host}
 *
 *  @note This function requires admin/sudo privileges on @platform{gpu_bm_linux}
 *
 *  @details Given a processor handle @p processor_handle and a soc pstate  policy @p policy_id,
 *  this function will set the soc pstate  policy for this processor. All the processors at
 *  the same socket will be set to the same policy.
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in] policy_id the soc pstate  policy id to set. The id is the id in
 *  amdsmi_dpm_policy_entry_t, which can be obtained by calling
 *  amdsmi_get_soc_pstate()
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_set_soc_pstate(amdsmi_processor_handle processor_handle,
                                      uint32_t policy_id);

/**
 *  @brief Get the xgmi per-link power down policy parameter for the processor
 *
 *  @ingroup tagClkPowerPerfControl
 *
 *  @platform{gpu_bm_linux} @platform{guest_1vf} @platform{host}
 *
 *  @details Given a processor handle @p processor_handle, this function will write
 *  current xgmi plpd settings to @p xgmi_plpd. All the processors at the same socket
 *  will have the same policy.
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in,out] xgmi_plpd the xgmi plpd for this processor.
 *  If this parameter is nullptr, this function will return
 *  ::AMDSMI_STATUS_INVAL
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_xgmi_plpd(amdsmi_processor_handle processor_handle,
                                     amdsmi_dpm_policy_t *xgmi_plpd);

/**
 *  @brief Set the xgmi per-link power down policy parameter for the processor
 *
 *  @ingroup tagClkPowerPerfControl
 *
 *  @platform{gpu_bm_linux} @platform{guest_1vf} @platform{host}
 *
 *  @details Given a processor handle @p processor_handle and a dpm policy @p policy_id,
 *  this function will set the xgmi plpd for this processor. All the processors at
 *  the same socket will be set to the same policy.
 *
 *  @note This function requires admin/sudo privileges
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in] policy_id the xgmi plpd id to set. The id is the id in
 *  amdsmi_dpm_policy_entry_t, which can be obtained by calling
 *  amdsmi_get_xgmi_plpd()
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_set_xgmi_plpd(amdsmi_processor_handle processor_handle,
                                     uint32_t policy_id);

/** @} End tagClkPowerPerfControl */

/*****************************************************************************/
/** @defgroup tagPowerControl Power Control
 *  @{
 */

/**
 *  @brief Set the maximum gpu power cap value. It is not supported on virtual
 *  machine guest
 *
 *  @ingroup tagPowerControl
 *
 *  @platform{host} @platform{gpu_bm_linux} @platform{guest_1vf}
 *
 *  @details Set the power cap to the provided value @p cap.
 *  @p cap must be between the minimum and maximum power cap values set by the
 *  system, which can be obtained from ::amdsmi_dev_power_cap_range_get.
 *
 *  @param[in] processor_handle A processor handle
 *
 *  @param[in] sensor_ind a 0-based sensor index. Normally, this will be 0.
 *  If a processor has more than one sensor, it could be greater than 0.
 *  Parameter @p sensor_ind is unused on @platform{gpu_bm_linux}.
 *
 *  @param[in] cap a uint64_t that indicates the desired power cap.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_set_power_cap(amdsmi_processor_handle processor_handle,
                                     uint32_t sensor_ind, uint64_t cap);

/** @} End tagPowerControl */

/*****************************************************************************/
/** @defgroup tagPhysicalStateQuery Physical State Queries
 *  These functions provide information about the physical characteristics of
 *  the device.
 *  @{
 */

/**
 *  @brief Returns gpu cache info.
 *
 *  @ingroup tagPhysicalStateQuery
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] info reference to the cache info struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_cache_info(amdsmi_processor_handle processor_handle, amdsmi_gpu_cache_info_t *info);

/** @} End tagPhysicalStateQuery */

/*****************************************************************************/
/** @defgroup tagECCInfo ECC Information
 *  @{
 */

/**
 *  @brief Retrieve the error counts for a GPU block. It is not supported on virtual
 *  machine guest
 *
 *  @ingroup tagECCInfo
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details Given a processor handle @p processor_handle, an ::amdsmi_gpu_block_t @p block and a
 *  pointer to an ::amdsmi_error_count_t @p ec, this function will write the error
 *  count values for the GPU block indicated by @p block to memory pointed to by
 *  @p ec.
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in] block The block for which error counts should be retrieved
 *
 *  @param[in,out] ec A pointer to an ::amdsmi_error_count_t to which the error
 *  counts should be written
 *  If this parameter is nullptr, this function will return ::AMDSMI_STATUS_INVAL
 *  if the function is supported with the provided arguments and ::AMDSMI_STATUS_NOT_SUPPORTED
 *  if it is not supported with the provided arguments.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_ecc_count(amdsmi_processor_handle processor_handle,
                                         amdsmi_gpu_block_t block, amdsmi_error_count_t *ec);

/**
 *  @brief Retrieve the enabled ECC bit-mask. It is not supported on virtual machine guest
 *
 *  @ingroup tagECCInfo
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details Given a processor handle @p processor_handle, and a pointer to a uint64_t @p
 *  enabled_mask, this function will write bits to memory pointed to by
 *  @p enabled_blocks. Upon a successful call, @p enabled_blocks can then be
 *  AND'd with elements of the ::amdsmi_gpu_block_t ennumeration to determine if
 *  the corresponding block has ECC enabled. Note that whether a block has ECC
 *  enabled or not in the device is independent of whether there is kernel
 *  support for error counting for that block. Although a block may be enabled,
 *  but there may not be kernel support for reading error counters for that
 *  block.
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @param[in,out] enabled_blocks A pointer to a uint64_t to which the enabled
 *  blocks bits will be written.
 *  If this parameter is nullptr, this function will return ::AMDSMI_STATUS_INVAL
 *  if the function is supported with the provided arguments and ::AMDSMI_STATUS_NOT_SUPPORTED
 *  if it is not supported with the provided arguments.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_ecc_enabled(amdsmi_processor_handle processor_handle,
                                           uint64_t *enabled_blocks);

/**
 *  @brief Returns the total number of ECC errors (correctable,
 *         uncorrectable and deferred) in the given GPU. It is not supported on
 *         virtual machine guest
 *
 *  @ingroup tagECCInfo
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

/** @} End tagECCInfo */

/*****************************************************************************/
/** @defgroup tagRasInfo RAS information
 *  @{
 */

/**
 * @brief Get the bad page threshold for a device
 *
 *  @ingroup tagRasInfo
 *
 * @platform{gpu_bm_linux}  @platform{host}
 *
 * @details Given a processor handle @p processor_handle and a pointer to a uint32_t @p threshold,
 * this function will retrieve the  bad page threshold value associated
 * with device @p processor_handle and store the value at location pointed to by
 * @p threshold.
 *
 * @note This function requires the admin/sudo privileges on @platform{gpu_bm_linux}
 *
 * @param[in] processor_handle a processor handle
 *
 * @param[in,out] threshold pointer to location where  bad page threshold value will
 * be written.
 * If this parameter is nullptr, this function will return
 * ::AMDSMI_STATUS_INVAL if the function is supported with the provided,
 * arguments and ::AMDSMI_STATUS_NOT_SUPPORTED if it is not supported with the
 * provided arguments.
 *
 * @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_bad_page_threshold(amdsmi_processor_handle processor_handle, uint32_t *threshold);

/**
 * @brief Retrieve CPER entries cached in the driver.
 *
 * @ingroup tagRasInfo
 *
 * @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf}
 *
 * The user will pass buffers to hold the CPER data and CPER headers. The library will
 * fill the buffer based on the severity_mask user passed. It will also parse the CPER header
 * and stored in the cper_hdrs array. The user can use the cper_hdrs to get the timestamp and other header information.
 * A cursor is also returned to the user, which can be used to get the next set of CPER entries.
 *
 * If there are more data than any of the buffers user pass, the library will return AMDSMI_STATUS_MORE_DATA.
 * User can call the API again with the cursor returned at previous call to get more data.
 * If the buffer size is too small to even hold one entry, the library
 * will return AMDSMI_STATUS_OUT_OF_RESOURCES.
 *
 * Even if the API returns AMDSMI_STATUS_MORE_DATA, the 2nd call may still get the entry_count == 0 as the driver
 * cache may not contain the serverity user is interested in. The API should return AMDSMI_STATUS_SUCCESS in this case
 * so that user can ignore that call.
 *
 * @param[in] processor_handle Handle to the processor for which CPER entries are to be retrieved.
 * @param[in] severity_mask The severity mask of the entries to be retrieved.
 * @param[in,out] cper_data Pointer to a buffer where the CPER data will be stored. User must allocate the buffer
 *                and set the buf_size correctly.
 * @param[in,out] buf_size Pointer to a variable that specifies the size of the cper_data.
 *                On return, it will contain the actual size of the data written to the cper_data.
 * @param[in,out] cper_hdrs Array of the parsed headers of the cper_data. The user must allocate
 *                the array of pointers to cper_hdr. The library will fill the array with the pointers to the parsed
 *                headers. The underlying data is in the cper_data buffer and only pointer is stored in this array.
 * @param[in,out] entry_count Pointer to a variable that specifies the array length of the cper_hdrs user allocated.
 *                On return, it will contain the actual entries written to the cper_hdrs.
 * @param[in,out] cursor Pointer to a variable that will contain the  cursor  for the next call.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_gpu_cper_entries(amdsmi_processor_handle processor_handle, uint32_t severity_mask, char *cper_data,
                            uint64_t *buf_size, amdsmi_cper_hdr_t** cper_hdrs, uint64_t *entry_count, uint64_t *cursor);

/**
 *  @brief Get the AFIDs from CPER buffer
 *
 *  @ingroup tagRasInfo
 *
 *  @platform{gpu_bm_linux} @platform{host} @platform{guest_1vf}
 *  @platform{guest_mvf}
 *
 *  @details A utility function which retrieves the AFIDs from the CPER record.
 *
 *  @param[in] cper_buffer a pointer to the buffer with one CPER record.
 *  The caller must make sure the whole CPER record is loaded into the buffer.
 *
 *  @param[in] buf_size is the size of the cper_buffer.
 *
 *  @param[out] afids a pointer to an array of uint64_t to which the AF IDs will be written
 *
 *  @param[in,out] num_afids As input, the value passed through this parameter is the number of
 *  uint64_t that may be safely written to the memory pointed to by @p afids. This is the limit
 *  on how many AF IDs will be written to @p afids. On return, @p num_afids will contain the
 *  number of AF IDs written to @p afids, or the number of AF IDs that could have been written
 *  if enough memory had been provided. It is suggest to pass MAX_NUMBER_OF_AFIDS_PER_RECORD for all
 *  AF Ids.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_afids_from_cper(char *cper_buffer, uint32_t buf_size, uint64_t *afids, uint32_t *num_afids);

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

/**
 * @brief Get the RAS policy info for a device
 *
 * @ingroup tagRasInfo
 *
 * @platform{gpu_bm_linux} @platform{host}
 *
 * @details Given a processor handle @p processor_handle, this function will retrieve
 * the RAS policy information for the device.
 *
 * @param[in] processor_handle PF of a processor for which to query
 *
 * @param[out] policy_info RAS policy info for the device. Must be allocated by user.
 *
 * @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_ras_policy_info(amdsmi_processor_handle processor_handle,
                                               amdsmi_gpu_ras_policy_info_t *info);

/**
 *  @brief Returns the bad page info.
 *
 *  @ingroup tagRasInfo
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor to query.
 *
 *  @param[in] array_length Length of the array where the library will
 *  copy the data.
 *
 *  @param[out] bad_page_count Number of bad page records.
 *
 *  @param[out] info Reference to the eeprom table record structure.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_gpu_bad_page_info(amdsmi_processor_handle processor_handle, uint32_t *bad_page_size,
                                             amdsmi_eeprom_table_record_t *bad_pages);

/** @} End tagRasInfo */

/*****************************************************************************/
/** @defgroup tagClkPowerPerfQuery Clock, Power and Performance Queries
 *  These functions provide information about clock frequencies and
 *  performance.
 *  @{
 */

/**
 *  @brief Triggers a chain that resets all GPUs. It is not supported on virtual machine guest
 *
 *  @ingroup tagClkPowerPerfQuery
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @note After this function returns, the caller must wait a few seconds before calling
 *  any other AMD SMI API functions to allow the GPU reset to complete. Calling other APIs
 *  too soon may result in AMDSMI_STATUS_BUSY or undefined behavior.
 *
 *  @details Given a processor handle @p processor_handle, this function will reset the GPU
 *
 *  @param[in] processor_handle a processor handle
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_reset_gpu(amdsmi_processor_handle processor_handle);

/** @} End tagClkPowerPerfQuery */

/** @defgroup tagNodeInfo Node Information
 *  @{
 */

/**
 * @brief Retrieves node power management (NPM) status and power limit for the specified node.
 *
 * @ingroup tagNodeInfo
 *
 * @platform{gpu_bm_linux} @platform{host}
 *
 * @details This function queries the NPM controller for the given node and returns whether NPM is enabled,
 * along with the current node-level power limit in Watts. The NPM status and limit are set out-of-band
 * and reported via this API.
 *
 * @param[in]  node_handle Handle to the Node to query.
 * @param[out] info Pointer to amdsmi_npm_info_t structure to receive NPM status and limit.
 *             Must be allocated by the user.
 *
 * @return ::AMDSMI_STATUS_SUCCESS on success, non-zero on failure.
 */
amdsmi_status_t amdsmi_get_npm_info(amdsmi_node_handle node_handle, amdsmi_npm_info_t *info);

/** @} End tagNodeInfo */

/*****************************************************************************/
/** @defgroup tagPTL PTL (Peak Tops Limiter) control and formats
 *  @{
 */

/**
 *  @brief Get PTL enable/disable state
 *
 *  @ingroup tagPTL
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details This function retrieves whether PTL (Peak Tops Limiter) is currently
 *  enabled or disabled for the specified processor. This is a simple state query
 *  that returns the current PTL operational state without detailed configuration.
 *
 *  @param[in] processor_handle Device which to query
 *
 *  @param[out] enabled Pointer to boolean that will be set to true if PTL is
 *  enabled, false if PTL is disabled
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success,
 *          ::AMDSMI_STATUS_NOT_SUPPORTED if PTL is not supported on this device,
 *          non-zero on other failures
 */
amdsmi_status_t
amdsmi_get_gpu_ptl_state(amdsmi_processor_handle processor_handle, bool *enabled);

/**
 *  @brief Set PTL enable/disable state
 *
 *  @ingroup tagPTL
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details This function enables or disables PTL (Peak Tops Limiter) operation.
 *  Use amdsmi_set_gpu_ptl_enable_with_formats()
 *  for more control over the preferred data formats when enabling.
 *
 *  @param[in] processor_handle Device to configure
 *
 *  @param[in] enable Boolean flag: true to enable PTL with default formats,
 *  false to disable PTL
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_set_gpu_ptl_state(amdsmi_processor_handle processor_handle, bool enable);

/**
 *  @brief Get PTL (Peak Tops Limiter) formats for the processor
 *
 *  @ingroup tagPTL
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details This function retrieves the current PTL formats
 *  for the specified processor. PTL prevents the product to never deliver more
 *  than a specified TOPS/second. If function returns 0 for both formats,
 *  PTL was never enabled before on that system.
 *
 *  @param[in] processor_handle Device which to query
*
 *  @param[out] data_format1 Pointer to first preferred data format that receives peak performance
 *
 *  @param[out] data_format2 Pointer to second preferred data format that receives peak performance
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success,
 *          ::AMDSMI_STATUS_NOT_SUPPORTED if PTL is not supported on this device,
 *          non-zero on other failures
 */
amdsmi_status_t
amdsmi_get_gpu_ptl_formats(amdsmi_processor_handle processor_handle,
                        amdsmi_ptl_data_format_t *data_format1,
                        amdsmi_ptl_data_format_t *data_format2);

/**
 *  @brief Set PTL with specified preferred data formats
 *
 *  @ingroup tagPTL
 *
 *  @platform{gpu_bm_linux} @platform{host}
 *
 *  @details This function sets PTL with the specified preferred data format pair.
 *  PTL must be enabled first before calling this function using amdsmi_set_gpu_ptl_state.
 *  The two specified formats will receive accurate performance monitoring and peak
 *  performance. F8 and XF32 formats always receive peak performance regardless of this setting.
 *
 *  @param[in] processor_handle Device to configure
 *
 *  @param[in] data_format1 First preferred data format (must be from the limited set:
 *  I8, F16, BF16, F32, F64)
 *
 *  @param[in] data_format2 Second preferred data format (must be from the limited set:
 *  I8, F16, BF16, F32, F64, and different from data_format1)
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success,
 *          ::AMDSMI_STATUS_NOT_SUPPORTED if PTL is not supported on this device,
 *          non-zero on other failures
 **/
amdsmi_status_t
amdsmi_set_gpu_ptl_formats(amdsmi_processor_handle processor_handle,
                          amdsmi_ptl_data_format_t data_format1,
                          amdsmi_ptl_data_format_t data_format2);

/** @} End tagPTL */

/*****************************************************************************/
/** @defgroup tagVFFBPartitionQuery VF and FB partitioning queries
 *  @{
 */

/**
 *  @brief Returns the number of VFs enabled by gpuv in the ASIC.
 *
 *  @ingroup tagVFFBPartitionQuery
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] num_vf_enabled Reference to number of VF enabled. Must be
 *  allocated by user.
 *
 *  @param[out] num_vf_supported Reference to number of VF supported. Must be
 *  allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_num_vf(amdsmi_processor_handle processor_handle, uint32_t *num_vf_enabled,
                  uint32_t *num_vf_supported);

/**
 *  @brief Returns the current framebuffer partitioning structure as
 *  currently configured by the driver.
 *
 *  @ingroup tagVFFBPartitionQuery
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[in] vf_buffer_num Size of the buffer where the library will
 *  copy the data in units of VF
 *
 *  @param[out] info Reference to structure with the current partitioning
 *  information. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_vf_partition_info(amdsmi_processor_handle processor_handle, unsigned int vf_buffer_num,
                             amdsmi_partition_info_t *info);

/**
 *  @brief Return the list of supported profiles on the given GPU device.
 *
 *  @ingroup tagVFFBPartitionQuery
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[out] profile_info reference to the profile info struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_partition_profile_info(amdsmi_processor_handle processor_handle,
                                  amdsmi_profile_info_t *profile_info);

/** @} End tagVFFBPartitionQuery */

/*****************************************************************************/
/** @defgroup tagVFConfig VF Configuration
 *  @{
 */

/**
 *  @brief Returns the configuration structure for a VF.
 *
 *  @ingroup tagVFConfig
 *
 *  @platform{host}
 *
 *  @param[in] vf_handle Handle of the VF to query.
 *
 *  @param[out] config Reference to structure with the current VF
 *  configuration. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_vf_info(amdsmi_vf_handle_t vf_handle, amdsmi_vf_info_t *config);

/**
 *  @brief Returns the data structure for a VF.
 *
 *  @ingroup tagVFConfig
 *
 *  @platform{host}
 *
 *  @param[in] vf_handle Handle of the PF or VF to query.
 *
 *  @param[out] info Reference to structure with the current VF
 *  data. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_vf_data(amdsmi_vf_handle_t vf_handle, amdsmi_vf_data_t *info);

/** @} End tagVFConfig */

/*****************************************************************************/
/** @defgroup tagEventMonitor Event Monitoring
 *  @{
 */

/**
 *  @brief Allocate a new event set notifier to monitor different
 *  types of issues with the GPU running virtualization SW.
 *  This call registers an event set. The user must pass an array
 *  with the GPUs it wants to monitor with the selected event flags.
 *
 *  @ingroup tagEventMonitor
 *
 *  @platform{host}
 *
 *  @param[in] processor_list Processor handles for the GPU to listen for events.
 *
 *  @param[in] num_devices Number of processors in the list.
 *
 *  @param[in] event_types Bitmask of the different event_types
 *  that the event_set will monitor in this GPU.
 *  Bit index (from 0):
 *  | 63 62 61 60| 59 .......... 0 |
 *  | event severity | event category bit field |
 *
 *  There are 5 event severities and the appropriate macros to set them:
 *  0b0000 High severity - AMDSMI_MASK_HIGH_ERROR_SEVERITY_ONLY
 *  0b0001 Med severity - AMDSMI_MASK_INCLUDE_MED_ERROR_SEVERITY
 *  0b0010 Low severity - AMDSMI_MASK_INCLUDE_LOW_ERROR_SEVERITY
 *  0b0100 Warn severity - AMDSMI_MASK_INCLUDE_WARN_SEVERITY
 *  0b1000 Info severity - AMDSMI_MASK_INCLUDE_INFO_SEVERITY
 *
 *  AMDSMI_MASK_INCLUDE_CATEGORY macro is used to set the category we want
 *  to monitor. Enum AMDSMI_EVENT_CATEGORY is used as the input parameter of the macro.
 *
 *  @param[out] set Reference to the pointer to the event set created
 *  by the library. This will be allocated by the library.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_event_create(amdsmi_processor_handle *processor_list, uint32_t num_devices,
                                        uint64_t event_types, amdsmi_event_set *set);

/**
 *  @brief The call blocks till timeout is expired to copy one event
 *  specified by the event set into the user provided
 *  notifier storage.
 *
 *  @ingroup tagEventMonitor
 *
 *  @note If timeout_usec is negative, the call will block forever,
 *  if timeout_usec is zero, the call returns immediately.
 *  Timeout value given in microseconds is converted to milliseconds.
 *  Minimal timeout is 1000 us. If provided timeout is lower than 1000
 *  then the timeout will be set to 1000us by default.
 *  The timeout value in us will be converted to a smaller integer value in ms.
 *  (e.g. 1500us -> 1ms , 2600us -> 2ms)
 *
 *  @note Provided event entry contains a 64 bit timestamp, fields for the category
 *  of the error, the sub-code and flags associated with the error, VF
 *  and GPU handles that originated the error and a 256B text buffer
 *  with a human-readable description of the error.
 *
 *  @platform{host}
 *
 *  @param[in] set Event set to read from. Use the same variable set that was used
 *  in the ::amdsmi_event_create call.
 *
 *  @param[in] timeout_usec Timeout in usec to wait for event
 *
 *  @param[out] event Reference to the user allocated event notifier.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_event_read(amdsmi_event_set set, int64_t timeout_usec, amdsmi_event_entry_t *event);

/**
 *  @brief Destroys and frees an event set.
 *
 *  @ingroup tagEventMonitor
 *
 *  @platform{host}
 *
 *  @param[in] set Event set to destroy.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_event_destroy(amdsmi_event_set set);

/** @} End tagEventMonitor */

/*****************************************************************************/
/** @defgroup tagHostGuestInteraction Host-Guest Interaction
 *  @{
 */

/**
 *  @brief Returns guest OS information of the queried VF. The fw_info
 *  field from the amdsmi_guest_data structure is deprecated and will be
 *  empty. To get the vf fw info, amdsmi_get_vf_fw_info API
 *  should be used.
 *
 *  @ingroup tagHostGuestInteraction
 *
 *  @platform{host}
 *
 *  @param[in] vf_handle Handle of the VF to query.
 *
 *  @param[out] info reference to the guest_data structure.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_guest_data(amdsmi_vf_handle_t vf_handle, amdsmi_guest_data_t *info);

/**
 *  @brief Returns the firmware versions running on a VF.
 *  In case the VM is not started on the VF,
 *  empty list will be returned and num_fw_info will be set to zero.
 *
 *  @ingroup tagHostGuestInteraction
 *
 *  @platform{host}
 *
 *  @param[in] vf_handle VF handle of a processor for which to query
 *
 *  @param[out] info Reference to the fw info. Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t
amdsmi_get_vf_fw_info(amdsmi_vf_handle_t vf_handle, amdsmi_fw_info_t *info);

/** @} End tagHostGuestInteraction */

/*****************************************************************************/
/** @defgroup tagVFManagement VF management
 *  @{
 */

/**
 *  @brief Clear the framebuffer of a VF. If trying to clear the framebuffer
 *         of an active function, the call will fail with device busy.
 *
 *  @ingroup tagVFManagement
 *
 *  @platform{host}
 *
 *  @param[in] vf_handle - Handle to the VF to send the command to.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_clear_vf_fb(amdsmi_vf_handle_t vf_handle);

/**
 *  @brief Enable a given number of VF.
 *
 *  @ingroup tagVFManagement
 *
 *  @platform{host}
 *
 *  @param[in] processor_handle PF of a processor for which to query
 *
 *  @param[in] num_vf - Number of VFs to enable.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_set_num_vf(amdsmi_processor_handle processor_handle, uint32_t num_vf);

/** @} End tagVFManagement */

/*****************************************************************************/
/** @defgroup tagNicInfo NIC Information
 *  @{
 */

/**
 *  @brief Retrieves information about the NIC driver
 *
 *  @ingroup tagNicInfo
 *
 *  @platform{host} @platform{gpu_bm_linux}
 *
 *  @param[in] processor_handle NIC for which to query
 *
 *  @param[out] info reference to the nic driver info struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_nic_driver_info(amdsmi_processor_handle processor_handle, amdsmi_nic_driver_info_t *info);

/**
 *  @brief Retrieves ASIC information for the NIC
 *
 *  @ingroup tagNicInfo
 *
 *  @platform{host} @platform{gpu_bm_linux}
 *
 *  @param[in] processor_handle NIC for which to query
 *
 *  @param[out] info reference to the nic asic info struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_nic_asic_info(amdsmi_processor_handle processor_handle, amdsmi_nic_asic_info_t *info);

/**
 *  @brief Retrieves BUS information for the NIC
 *
 *  @ingroup tagNicInfo
 *
 *  @platform{host} @platform{gpu_bm_linux}
 *
 *  @param[in] processor_handle NIC for which to query
 *
 *  @param[out] info reference to the nic bus info struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_nic_bus_info(amdsmi_processor_handle processor_handle, amdsmi_nic_bus_info_t *info);

/**
 *  @brief Retrieves NUMA information for the NIC
 *
 *  @ingroup tagNicInfo
 *
 *  @platform{host} @platform{gpu_bm_linux}
 *
 *  @param[in] processor_handle NIC for which to query
 *
 *  @param[out] info reference to the nic numa info struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_nic_numa_info(amdsmi_processor_handle processor_handle, amdsmi_nic_numa_info_t *info);

/**
 *  @brief Retrieves PORT information for the NIC
 *
 *  @ingroup tagNicInfo
 *
 *  @platform{host} @platform{gpu_bm_linux}
 *
 *  @param[in] processor_handle NIC for which to query
 *
 *  @param[out] info reference to the nic port info struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_nic_port_info(amdsmi_processor_handle processor_handle, amdsmi_nic_port_info_t *info);

/**
 *  @brief Retrieves RDMA devices information for the NIC
 *
 *  @ingroup tagNicInfo
 *
 *  @platform{host} @platform{gpu_bm_linux}
 *
 *  @param[in] processor_handle NIC for which to query
 *
 *  @param[out] info reference to the nic rdma devices info struct.
 *  Must be allocated by user.
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_nic_rdma_dev_info(amdsmi_processor_handle processor_handle, amdsmi_nic_rdma_devices_info_t *info);

/**
 *  @brief Retrieve PORT statistics for the specified NIC port
 *
 *  @ingroup tagNicInfo
 *
 *  @platform{host} @platform{gpu_bm_linux}
 *
 *  This function follows a two-call pattern:
 *  1. First call with stats=NULL to get the count of available statistics
 *  2. Second call with allocated array to retrieve all statistics
 *
 *  @param[in] processor_handle NIC for which to query
 *  @param[in] port_index index of the NIC port to query
 *  @param[in,out] num_stats pointer to the number of statistics
 *    - Input: maximum number of statistics that stats array can hold
 *    - Output: actual number of statistics available/returned
 *  @param[out] stats pointer to array of amdsmi_nic_stat_t structures to be filled
 *    - If NULL, only num_stats is filled with the count of available statistics
 *    - If not NULL, must be allocated by user with at least num_stats elements
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_nic_port_statistics(amdsmi_processor_handle processor_handle, uint32_t port_index,
                                               uint32_t *num_stats, amdsmi_nic_stat_t *stats);

/**
 *  @brief Retrieve vendor specific statistics for the NIC port
 *
 *  @ingroup tagNicInfo
 *
 *  @platform{host} @platform{gpu_bm_linux}
 *
 *  This function follows a two-call pattern:
 *  1. First call with stats=NULL to get the count of available statistics
 *  2. Second call with allocated array to retrieve all statistics
 *
 *  This API provides access to vendor/driver specific statistics that may vary
 *  between different NIC vendors and driver/fw versions. The statistic names are
 *  preserved as provided by the underlying driver implementation.
 *
 *  @param[in] processor_handle NIC for which to query
 *  @param[in] port_index index of the NIC port to query
 *  @param[in,out] num_stats pointer to the number of statistics
 *    - Input: maximum number of statistics that stats array can hold
 *    - Output: actual number of statistics available/returned
 *  @param[out] stats pointer to array of amdsmi_nic_stat_t structures to be filled
 *    - If NULL, only num_stats is filled with the count of available statistics
 *    - If not NULL, must be allocated by user with at least num_stats elements
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_nic_vendor_statistics(amdsmi_processor_handle processor_handle, uint32_t port_index,
                                                 uint32_t *num_stats, amdsmi_nic_stat_t *stats);

/**
 *  @brief Retrieve RDMA port statistics for the NIC
 *
 *  @ingroup tagNicInfo
 *
 *  @platform{host} @platform{gpu_bm_linux}
 *
 *  This function follows a two-call pattern:
 *  1. First call with stats=NULL to get the count of available statistics
 *  2. Second call with allocated array to retrieve all statistics
 *
 *  @param[in] processor_handle NIC for which to query
 *  @param[in] rdma_port_index index of the NIC RDMA port to query
 *  @param[in,out] num_stats pointer to the number of statistics
 *    - Input: maximum number of statistics that stats array can hold
 *    - Output: actual number of statistics available/returned
 *  @param[out] stats pointer to array of amdsmi_nic_stat_t structures to be filled
 *    - If NULL, only num_stats is filled with the count of available statistics
 *    - If not NULL, must be allocated by user with at least num_stats elements
 *
 *  @return ::amdsmi_status_t | ::AMDSMI_STATUS_SUCCESS on success, non-zero on fail
 */
amdsmi_status_t amdsmi_get_nic_rdma_port_statistics(amdsmi_processor_handle processor_handle, uint32_t rdma_port_index,
                                                    uint32_t *num_stats, amdsmi_nic_stat_t *stats);

/** @} End tagNicInfo */

#endif  // __AMDSMI_H__

