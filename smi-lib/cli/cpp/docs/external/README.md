#
# Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#


# AMDSMI cpptool

## Requirements


## Dependencies


## Introduction
SMI Tool (called AMDSMI) is a command line utility which utilizes SMI Library APIs to monitor AMD GPUs.
The same tool can be used to monitor AMD's GPUs used for HyperV host for Windows Operating System and Linux host.
It can display queried information in human readable and JSON formats and prints it in the console.

## Usage (Linux)
Navigate to following directory
    smi_lib/cli/cpp
NOTE: If the build directory does not exist, you can create it using the following command from cpptool dir:
    mkdir build

Inside cli/cpp project directory, run the following command:
    make [BUILD_TYPE=<debug|release>]

NOTE: release is selected by default if BUILD_TYPE is not specified

Once the compilation is complete, amd-smi binary will be generated in:
    1. 'smi-lib/tool/'
    2. 'smi-lib/cli/cpp/build/' folders

The tool can be run after installing and loading the driver. Tool is located on the /usr/src/gim-<specific_name>/smi-lib/tool path (for example gim-0.staging.psdb.830). specific_name changes and depends on the system and driver version. In order for the tool to work, it is necessary for the libamdsmi.so to be located in the same folder as the tool or it can be on the system path as well (/usr/lib). libamdsmi.so can be found on the following path gim/smi-lib/build/amdsmi/Release/libamdsmi.so.

Running the tool is possible by just executing the executable file, e.g. sudo ./amd-smi list

## Commands
On the top level, the tool supports multiple commands each having its own set of mandatory or optional arguments for more granular control of the tool.

The commands and their descriptions are the following:
### discovery (list)
    Lists all GPUs and VFs on the system, sorted by BDF, and their most basic general
    information
### static
    Displays all specified static information for the specified GPU
### firmware (ucode)
    Displays all microcode information for the specified GPU
### bad-pages
    Displays all bad page information for the specified GPU
### metric
    Displays all specified metric information for the specified GPU
### profile
    Displays information about all profiles and current profile
### event
    Displays event information for given GPU.
### topology
    Displays link topology information.
### xgmi
    Displays XGMI capabilities, framebuffer sharing and metric information
### partition
    Displays partitionig information, such as memory and accelerator
### reset
    Cleanup VF FB for the specified VF or reset memory and accelerator
    parittion to default mode
### set
    Set xgmi frame buffer sharing mode and partitioning modes

## Command Arguments

Commands take arguments to more directly specify which information is to be displayed. Some commands such as discovery do not have arguments.
Commands that do and their arguments are:
### static
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Gets static information about the
    specified GPU. If no argument is provided, returns information for all GPUs on the system. If
    no static information argument is provided all static information will be returned. Static
    arguments for the GPU are the following:
    *  `--asic`
          All asic information
    *  `--bus`
          All bus information
    *  `--vbios`
          All video bios information
    *  `--board`
          All board information
    *  `--limit`
          All limit metric value
    *  `--driver`
          Displays driver version
    *   `--ras`
          Displays ras features information
    *   `--dfc-ucode`
          All dfc ucode table information.
    *   `--fb-info`
          All fb information
    *   `--num-vf`
          Displays number of supported and enabled VFs
    *   `--vram`
          All vram information
    *   `--cache`
          All cache information
    *   `--partition`
          All partition information

* `--vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>`
	Gets general information about the specified VF (timeslice, fb info, …)

### firmware (ucode)
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Gets microcode information about the specified GPU
    If no argument is provided, returns information for all GPUs on the system
    Firmware arguments for the GPU are the following:
    *   `--fw-list`
          All firmware list information
    *   `--error-records`
          All error records information

*  `--vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>`
    Gets microcode information about the specified VF
    *   `--fw-list`
          All firmware list information

### bad-pages
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Gets bad page information about the specified GPU
    If no argument is provided, returns information for all GPUs on the system

### metric
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Gets metric information about the specified GPU
    If no argument is provided, returns information for all GPUs on the system
    If no metric information argument is provided all metric information will be displayed
    Metric arguments for the GPU are the following:
    *  `--usage`
        All usage information
    *  `--fb-usage`
        Total and used framebuffer
    *  `--power`
        All power readings information
    *  `--clock`
        All frequency sensor readings
    *  `--temperature`
        All thermal sensor readings
    *  `--ecc`
        All ecc information
    *  `--ecc-block`
        Number of ECC errors per block
    *  `--pcie`
        Current pcie information
    *  `--energy`
        Amount of energy consumed

* `--vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>`
    Gets metric information about the specified VF.
    If no metric information argument is provided all metric information will be displayed.
    Metric arguments for the VF are the following:
    *  `--schedule`
        All scheduling info
    *  `--guard`
        All guard information
    *  `--guest-data`
        All guest data information

### process
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Lists general process information running on the specified GPU.
    If no argument is provided, returns information for all GPUs on the system.
    If no argument is provided all process information will be displayed.
    Arguments are the following:
    *   `--general`
        pid, process name, memory usage
    *   `--engine`
        All engine usages
* `--pid=<pid>`
    Gets all process information about the specified process based on Process ID.
* `--name=<process_name>`
    Gets all process information about the specified process based on Process Name.
    If multiple processes have the same name information is returned for all of them.

### profile
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Displays information about all profiles and current profile.
    If no argument is provided, returns information for all GPUs on the system.

### event
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Displays event information for given GPU.
    If no argument is provided, returns event informations for all GPUs on the system.

### topology
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Displays link topology information
    If no argument is provided, returns information for all GPUs on the system
    *  `--weight`
            Current weight information
    *   `--hops`
            Current hops information
    *   `--fb-sharing`
            Current framebuffer sharing information
    *   `--link-type`
            Link type information
    *   `--link-status`
            Link status information

### xgmi
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Displays XGMI capabilities, framebuffer sharing and metric information
    If no argument is provided, returns information for all GPUs on the system
    *   `--caps`
            XGMI capabilities
    *   `--fb-sharing`
            Framebuffer sharing for each mode
    *    `--metric`
            Metric xgmi information

### partition
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Displays capabilities and current information for memory and accelerator partition
    If no argument is provided, returns information for all GPUs on the system
    *   `--current`
            Current memory and accelerator partition information
    *   `--memory`
            Memory partition information
    *   `--accelerator`
            Accelerator partition information

### reset
* `--vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>`
    Cleanup VF FB for the specified VF.
    If no argument is provided, returns tool exception.
    Available only in human readable format.
    *   `--vf-fb`
        Cleanup VF FB for the specified VF
    * `--memory-partition`
        Resets memory partition to default settings. Available only on host and baremetal OSs
        Available only in human readable format.
    * `--accelerator-partition`
        Resets accelerator partition to default settings. Available only on host and baremetal OSs
        Available only in human readable format.
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Clean gpu local data.
    If no argument is provided, returns tool exception.
    Available only in human readable format.
    * `--clean-local-data`
        Clean up data in LDS/GPRs.

### set
* `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`
    Set options for devices
    If no argument is provided, returns tool exception.
    Available only in human readable format.
    * Sets xgmi frame buffer sharing mode
        *   `--xgmi --fb-sharing-mode=<AmdSmiXgmiFbSharingMode> --group="<gpu_id1-gpu_id2>"`
                Sets framebuffer sharing mode from group ["MODE_1", "MODE_2", "MODE_4", "MODE_8", "CUSTOM"]
                Where, MODE_X represents that X GPUs will be in the same group, linked together:
                MODE_1 (one GPU in a group), MODE_2 (two GPUs in a group), MODE_4 (four GPUs in a group), MODE_8 (eight GPUs in a group).
                For MODE_X, the --group parameter is not needed and if it is used then the call will fail.
                For CUSTOM mode we need to specify which GPUs will be connected --group="0-2" and that means
                that 0th and the second gpu will be connected. If we want the GPU to share the frame buffer with another GPU, and it is already in the previous group, the previous group will be freed and a new one will be created, with which it will share the frame buffer.
                Note: This command will only work if the VM for which GPU we are setting the FB sharing is not running, otherwise it will fail.
                Note: If trying to set FB sharing between the GPUs from the different NUMA nodes, the call will fail
                All possible configurations can be seen by running the .\amd-smi xgmi command.
        *   `--memory-partition=<AmdSmiMemoryPartitionSetting>`
                Available only on host and baremetal OSs
                Sets memory partition setting from group ["NPS1", "NPS2", "NPS4", "NPS8"]
        *   `--accelerator-partition=<profile_index>`
                Available only on host and baremetal OSs
                Sets memory partition setting to a mode based on profile_index from partition command
    * Sets process isolation
        *   `--process-isolation=<0 or 1>`
                Enable or disable the GPU process isolation: 0 for disable and 1 for enable
    * Sets power cap
        *   `--power-cap=<power_cap_value>`
                Available only on host and baremetal OSs.
                Sets the power cap to the provided cap value.
                Note: Cap value must be between the minimum (min_power_cap) and maximum (max_power_cap) power cap values

## Modifiers
Modifiers are optional arguments to be passed to the command which modify the tool’s output. They are the following:

* `--json`
Displays output in JSON format (human readable by default)
* `--csv`
Displays output in CSV format (human readable by default).

## Error Messages
Error messages are returned to specify to the user what exactly has happened.
They should be print to the file if the user wants to print output to the file or stderr if the user wants to print in the terminal. If there is error then print number of error on stderr but if error doesn't exist stderr will be empty.
*   `0 – Success`
*Does not display a message*
*   `-1 – Invalid Command`
	“Command <command_user_wrote> is invalid. Run 'help' for more info.”
*   `-2 – Invalid Parameter`
	“Parameter <parameter_user_wrote> is invalid. Run 'help' for more info.”
*   `-3 – Device Not Found`
    “Device <index_from_list|BDF|UUID inputted by user> cannot be found on the system. Run 'help' for more info.”
*   `-4 – Invalid File Path`
	“Path <path_user_wrote> cannot be found.”
*   `-5 – Invalid Parameter Value`
	“Value <value_user_wrote> is not of valid type or format. Run 'help' for more info.”
*   `-6 – Missing Parameter Value`
	“Parameter <parameter_which_requires_a_value> requires a value. Run 'help' for more info.”
*   `-7 – Command Not Supported`
	“Command <command_user_wrote> is not supported on the system. Run 'help' for more info.”
*   `-8 – Parameter Not Supported`
	“Parameter <parameter_user_wrote> is not supported on the system. Run 'help' for more info.”
*   `-100 – Unknown Error`
	“An unknown error has occurred. Run 'help' for more info.”
*   `(-1014) – (-1001) – SMI-LIB Error`
	“SMI-LIB has returned error <smi_lib_error_code> - <smi_lib_error_code_string>”

## Examples

List all devices:
```
$ .\amd-smi.exe discovery
```
Output in HUMAN READABLE format:
```
GPU 0:
    BDF: 0000:83:00.0
    UUID: 87ff73c8-0000-1000-8059-3ae746ab9206
    VF 0:
        BDF: 0000:83:02.0
        UUID: 870073c8-0000-1000-8059-3ae746ab9206
…
```

Get all static information about a specific GPU:
```
$ .\amd-smi.exe static --gpu=0
```
GPU 0:
    ASIC:
        MARKET_NAME: MI300
        VENDOR_ID: 0x1002
        VENDOR_NAME: Advanced Micro Devices Inc. [AMD/ATI]
        SUBVENDOR_ID: 0x1002
        DEVICE_ID: 0x74A1
        SUBSYSTEM_ID: 0x74A1
        REV_ID: 0x0
        ASIC_SERIAL: 0xF33397508B72EAAF
        OAM_ID: 2
    BUS:
        BDF: 0000:0c:00.0
        MAX_PCIE_LANES: 16
        MAX_PCIE_SPEED: 32 GT/s
        PCIE_INTERFACE_VERSION: Gen 5
        SLOT_TYPE: OAM
        MAX_PCIE_INTERFACE_VERSION: Gen 5
    VBIOS:
        NAME: AMD MI300X_HWEMU_SRIOV_CVS_1VF
        VERSION: 022.040.003.036.000001
        BUILD_DATE: 2023/11/16 11:02
        PART_NUMBER: 113-MI3SRIOV-001
    BOARD:
        MODEL_NUMBER: 102-G30201-0B
        PRODUCT_SERIAL: PCB068560-0020
        FRU_ID: 113-AMDG302010B14
        MANUFACTURER_NAME: AMD
        PRODUCT_NAME: Instinct MI300
    LIMIT:
        MAX_POWER: 750 W
        MIN_POWER: 0 W
        POWER: 750 W
        SLOWDOWN_EDGE_TEMPERATURE: N/A C
        SLOWDOWN_HOTSPOT_TEMPERATURE: 100 C
        SLOWDOWN_MEM_TEMPERATURE: 95 C
        SHUTDOWN_EDGE_TEMPERATURE: N/A C
        SHUTDOWN_HOTSPOT_TEMPERATURE: 110 C
        SHUTDOWN_MEM_TEMPERATURE: 105 C
    DRIVER:
        DRIVER_NAME: AMDGPUV
        DRIVER_VERSION: 21.9.0
        DRIVER_DATE: 12-21-2023
        DRIVER_MODEL: WDDM
    RAS:
        CAPS:
            RAS_EEPROM_VERSION: 0
            SUPPORTED_ECC_CORRECTION_SCHEMA:
                PARITY: SUPPORTED
                CORRECTABLE: SUPPORTED
                UNCORRECTABLE: SUPPORTED
                POISON: SUPPORTED
            BLOCK: UMC
            STATUS: ENABLED
            BLOCK: SDMA
            STATUS: ENABLED
            BLOCK: GFX
            STATUS: ENABLED
            BLOCK: MMHUB
            STATUS: ENABLED
            BLOCK: ATHUB
            STATUS: DISABLED
            BLOCK: PCIE_BIF
            STATUS: DISABLED
            BLOCK: HDP
            STATUS: DISABLED
            BLOCK: XGMI_WAFL
            STATUS: ENABLED
            BLOCK: DF
            STATUS: DISABLED
            BLOCK: SMN
            STATUS: DISABLED
            BLOCK: SEM
            STATUS: DISABLED
            BLOCK: MP0
            STATUS: DISABLED
            BLOCK: MP1
            STATUS: DISABLED
            BLOCK: FUSE
            STATUS: DISABLED
            BLOCK: MCA
            STATUS: DISABLED
            BLOCK: VCN
            STATUS: DISABLED
            BLOCK: JPEG
            STATUS: DISABLED
            BLOCK: IH
            STATUS: DISABLED
            BLOCK: MPIO
            STATUS: DISABLED
    DFC:
        HEADER:
            VERSION: 0.1.0.2
        DATA:
            DATA_INFO:
                DFC_FW_TYPE: 75
                VERIFICATION: ENABLED
                WHITE_LIST:
                    VERSIONS:
                         WHITE_LIST_LATEST: 0x4ACF2092
                         WHITE_LIST_OLDEST: 0xC7F7A840
                         BLACK_LIST:
                    VERSIONS:
                         WHITE_LIST_LATEST: 0xA7D73BBC
                         WHITE_LIST_OLDEST: 0x5A405496
                         BLACK_LIST:
    FB_INFO:
        TOTAL_FB_SIZE: 196312 MB
        PF_FB_RESERVED: 256 MB
        PF_FB_OFFSET: 0 MB
        FB_ALIGNMENT: 16 MB
        MAX_VF_FB_USABLE: 196056 MB
        MIN_VF_FB_USABLE: 16 MB
    NUM_VF:
        NUM_VF_SUPPORTED: 1
        NUM_VF_ENABLED: 1
    VRAM:
        TYPE: HBM3
        VENDOR: HYNIX
        SIZE: 196592 MB
    CACHE_INFO:
        CACHE 0:
            CACHE_PROPERTIES: ENABLED, DATA_CACHE, SIMD_CACHE
            CACHE_SIZE: 32 KB
            CACHE_LEVEL: 1
            MAX_NUM_CU_SHARED: 1
            NUM_CACHE_INSTANCE: 304
        CACHE 1:
            CACHE_PROPERTIES: ENABLED, DATA_CACHE, SIMD_CACHE
            CACHE_SIZE: 16 KB
            CACHE_LEVEL: 1
            MAX_NUM_CU_SHARED: 2
            NUM_CACHE_INSTANCE: 152
        CACHE 2:
            CACHE_PROPERTIES: ENABLED, INST_CACHE, SIMD_CACHE
            CACHE_SIZE: 64 KB
            CACHE_LEVEL: 1
            MAX_NUM_CU_SHARED: 2
            NUM_CACHE_INSTANCE: 152
        CACHE 3:
            CACHE_PROPERTIES: ENABLED, DATA_CACHE, SIMD_CACHE
            CACHE_SIZE: 4096 KB
            CACHE_LEVEL: 2
            MAX_NUM_CU_SHARED: 38
            NUM_CACHE_INSTANCE: 8
        CACHE 4:
            CACHE_PROPERTIES: ENABLED, DATA_CACHE, SIMD_CACHE
            CACHE_SIZE: 262144 KB
            CACHE_LEVEL: 3
            MAX_NUM_CU_SHARED: 304
            NUM_CACHE_INSTANCE: 1
…
```
$ .\amd-smi.exe static --gpu=0000:83:00.0
```
Get specific information for a specified GPU:
```
$ .\amd-smi.exe metric --gpu=0000:83:00.0 --usage --power
```
Output in HUMAN READABLE format:
```
GPU 0:
    USAGE:
        GFX_ACTIVITY: 0 %
        UMC_ACTIVITY: 0 %
        MM_ACTIVITY: N/A %
        VCN_ACTIVITY: [ 0 %, 0 %, 0 %, 0 % ]
        JPEG_ACTIVITY: [ 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %,
                         0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 % ]
    POWER:
        AVERAGE_SOCKET_POWER: 224 W
        GFX_VOLTAGE: N/A mV
        SOC_VOLTAGE: N/A mV
        MEM_VOLTAGE: N/A mV
        POWER_MANAGEMENT: ENABLED
```

Convert queried info into json:
```
$ .\amd-smi.exe metric --gpu=0 --temperature --json
```
Output in JSON format:
```json
{
 "gpu": 0,
 "temperature": {
  "edge_temperature": "N/A",
  "hotspot_temperature": 43,
  "vram_temperature": 35
 }
}
```

