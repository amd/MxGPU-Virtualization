---
myst:
  html_meta:
    "description lang=en": "Learn how to use the AMD SMI command line tool - comprehensive usage guide."
    "keywords": "amdsmi, cli, usage, commands, examples, gpu, monitoring"
---

# AMD SMI CLI Tool - Usage Guide

## Overview

**AMD SMI tool** is a command line utility that utilizes AMD SMI Library APIs to monitor and configure AMD GPUs on Linux host systems. The tool is used to monitor AMD GPUs status in virtualization environments, providing comprehensive GPU management capabilities for host administrators. The tool outputs GPU/driver information in plain text, in JSON, or in CSV formats while it can also show the info in the console or save to the specified output file.

## Return Codes

The **AMD SMI CLI tool** uses specific return codes to indicate the status of command execution:

| Return Code | Description |
|---------|-------------|
| 0       | Success. Does not display a message |
| -1       | Invalid Command "Command [command_user_wrote] is invalid. Run 'help' for more info." |
| -2       | Invalid Parameter "Parameter [command_user_wrote] is invalid. Run 'help' for more info." |
| -3       | Device Not Found "Device [index_from_list\|BDF\|UUID inputted by user] cannot be found on the system. Run 'help' for more info." |
| -4       | Invalid File Path "Path [path_user_wrote] cannot be found." |
| -5       | Invalid Parameter Value "Value [value_user_wrote] is not of valid type or format. Run 'help' for more info." |
| -6       | Missing Parameter Value "Parameter [parameter_which_requires_a_value] requires a value. Run 'help' for more info." |
| -7       | Command Not Supported "Command [command_user_wrote] is not supported on the system. Run 'help' for more info." |
| -8       | Parameter Not Supported "Parameter [parameter_user_wrote] is not supported on the system. Run 'help' for more info." |
| -9       | Required command "Command [command_user_wrote] requires a target argument. Run 'help' for more info." |
| -10      | Invalid subcommand "Command [command_user_wrote] is invalid. Must receive valid AMD-SMI Command first. Run 'help' for more info." |
| -11      | Permission Denied "Permission denied. This action requires elevated privileges." |
| -100     | Unknown Error "An unknown error has occurred. Run 'help' for more info." |

**Library Error Codes:**
(-1014) – (-1001) – SMI-LIB Error "SMI-LIB has returned error [smi_lib_error_code] - [smi_lib_error_code_string]"

## Commands

Commands take arguments that help to specify the type of information to be displayed. Note that some commands such as help, list and version do not have arguments.
The commands and respective arguments that they accept are described as follows:

1. **help**
   Display information about the tool.

2. **version**
   Display information about current version of the library and the tool.

3. **list** (discovery)
   Lists all GPUs and VFs on the system and their most basic general information.

4. **static**
   Gets static information about the specified GPU or VF. If no target is specified, returns information for all GPUs on the system.

   **GPU Parameters:**
   - `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`: Parameters for a specific GPU
     - `--asic`: All asic information.
     - `--bus`: All bus information.
     - `--vbios`: All video bios information (if available).
     - `--board`: All board information.
     - `--limit`: All limit metric values (i.e. power and thermal limits).
     - `--driver`: Displays driver version.
     - `--ras`: Displays ras features information.
     - `--dfc-ucode`: All dfc ucode table information.
     - `--fb-info`: All fb information.
     - `--num-vf`: Displays number of supported and enabled VFs.
     - `--vram`: All vram information.
     - `--cache`: All cache info.
     - `--partition`: All partition information.
     - `--ifwi`: All IFWI/video bios information.
     - `--numa`: All NUMA information.

   **VF Parameters:**
   - `--vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>`: Gets general information about the specified VF (e.g. timeslice, fb info)

5. **firmware** (ucode)
   Gets firmware information about the specified GPU or VF. If no target is specified, returns information for all GPUs on the system.

   **GPU Parameters:**
   - `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`: Parameters for a specific GPU
     - `--fw-list`: All firmware list information.
     - `--error-records`: All error records information.

   **VF Parameters:**
   - `--vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>`: Parameters for a specific VF
     - `--fw-list`: All firmware list information.

6. **bad-pages**
   - `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`: Gets bad page information about the specified GPU. If no argument is provided, returns information for all GPUs on the system.

7. **metric**
   Gets metric information about the specified GPU or VF. If no target is specified, returns information for all GPUs on the system.

   **GPU Parameters:**
   - `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`: Parameters for a specific GPU
     - `--usage`: All usage information.
     - `--power`: All power readings information.
     - `--clock`: All frequency sensor readings.
     - `--temperature`: All thermal sensor readings.
     - `--ecc`: All ecc information.
     - `--ecc-block`: Number of ECC errors per block.
     - `--pcie`: Current pcie information.
     - `--energy`: Amount of energy consumed.

   **VF Parameters:**
   - `--vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>`: Parameters for a specific VF
     - `--schedule`: All scheduling info.
     - `--guard`: All guard information.
     - `--guest-data`: All guest data information.
     - `--per-partition`: Per-partition metrics information.

   **Note:** When using the `--csv` format modifier with the metric command, only one argument is supported per command (e.g., metric --usage). For all other formats (plain text and json), multiple arguments are supported. The per-partition command does not support the `--csv` format modifier.

8. **event**
    - `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`:
    Displays event information for GPU. If no argument is provided, returns event information for all GPUs on the system.

    **Note:** The watch, watch_time and iterations modifiers are not supported for the event command.

9. **topology**
    - `--gpu= <gpu_index from list, gpu_bdf, gpu_uuid>`:
    Displays link topology information. If no argument is provided, returns information for all GPUs on the system.

    Topology arguments for the GPU are the following:
    - `--weight`: Current weight information.
    - `--hops`: Current hops information.
    - `--fb-sharing`: Current framebuffer sharing information.
    - `--link-type`: Link type information.
      - `--coherent`: Cache coherent information.
      - `--atomics`: 32 and 64-bit atomic link capability information.
      - `--bi-dir`: bi-directional link capability information.
      - `--dma`: dma link capability information.
    - `--link-status`: Link status information.

    **Note:** The topology command does not support the `--csv` format modifier.

10. **xgmi**
    - `--gpu= <gpu_index from list, gpu_bdf, gpu_uuid>`:
    Displays XGMI capabilities, framebuffer sharing and metric information. If no argument is provided, returns information for all GPUs on the system.

    XGMI arguments for the GPU are the following:
    - `--caps`: XGMI capabilities.
    - `--fb-sharing`: Framebuffer sharing for each mode.
    - `--metric`: Metric XGMI information.
    - `--source-status`: Port status information.
    - `--link-status`: Link status information.

    **Note:** The xgmi command does not support the `--csv` format modifier.

11. **reset**
    Reset or cleanup operations for GPUs and VFs. Available only in plain text. If no target is provided, returns tool exception.

    **GPU Parameters:**
    - `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`: Parameters for a specific GPU
      - `--gpureset`: Reset GPU.


    **VF Parameters:**
    - `--vf=<gpu_index:vf_index from list, vf_bdf, vf_uuid>`: Parameters for a specific VF (requires SR-IOV)
      - `--vf-fb`: Cleanup VF FB for the specified VF.

12. **set**
    - `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`:
    Set options for devices. If no argument is provided, returns tool exception. Available only in plain text.

    Set arguments for the GPU are the following:
    - `--memory-partition=<AmdSmiMemoryPartitionSetting>`: Sets memory partition mode. Run `amd-smi partition` to list memory-partition modes supported on current platform.
    - `--accelerator-partition=<profile_index>`: Sets accelerator partition mode to a mode based on profile_index from partition command. Run `amd-smi partition` to list accelerator partition modes supported on current platform.
    - `--power-cap=<power_cap_value>`: Sets the power cap to the provided cap value. **Note:** Cap value must be between the minimum (min_power_cap) and maximum (max_power_cap) power cap values which can be retrieved from `amd-smi static --limit` command.
    - `--xgmi-plpd=<policy>`: Sets XGMI Per-Link Power Down (PLPD) to enabled or disabled.
    - `--num-vf=<number_of_vfs>`: Sets the number of Virtual Functions (VFs) to be enabled on the specified GPU. The number must be within the supported range for the GPU. Use `amd-smi static --gpu=<gpu> --num-vf` to check current VF configuration and supported limits.
    - `--soc-pstate=<pstate_level>`: Sets the SOC (System on Chip) performance state level to control power and performance characteristics.
    - `--xgmi --fb-sharing-mode=<AmdSmiXgmiFbSharingMode>`: Sets framebuffer sharing mode from list ["MODE_1", "MODE_2", "MODE_4", "MODE_8"] where, MODE_X represents that X GPUs will be in the same group, linked together: MODE_1 (one GPU in a group), MODE_2 (two GPUs in a group), MODE_4 (four GPUs in a group), MODE_8 (eight GPUs in a group). All possible configurations can be seen by running the `amd-smi xgmi` command, not all of them are supported on all systems.

13. **monitor**
    - `--gpu=<gpu_index from list, gpu_bdf, gpu_uuid>`:
    Monitor a target device for the specified arguments. If no arguments are provided, all arguments will be enabled. Use the watch arguments to run continuously.

    Monitor arguments for the GPU are the following:
    - `--gfx`: Monitor graphics utilization (%) and clock (MHz).
    - `--mem`: Monitor memory utilization (%) and clock (MHz).
    - `--encoder`: Monitor encoder utilization (%) and clock (MHz).
    - `--ecc`: Monitor ECC single bit, ECC double bit.
    - `--pcie`: Monitor PCIe bandwidth in Mb/s and PCIe replay error count.
    - `--power-usage`: Monitor power usage in Watts.
    - `--temperature`: Monitor temperature in Celsius.
    - `--decoder`: Monitor decoder utilization (%) and clock (MHz).

14. **partition**
    - `--gpu= <gpu_index from list, gpu_bdf, gpu_uuid>`:
    Displays capabilities and current information for memory and accelerator partition. If no argument is provided, returns information for all GPUs on the system.

    Partition arguments for the GPU are the following:
    - `--current`: Current memory and accelerator partition information.
    - `--memory`: Memory partition information.
    - `--accelerator`: Accelerator partition information.
    - `--global`: Global partition configuration settings.

    **Note:** The partition command does not support the `--csv` or `--json` format modifiers.

15. **ras**
    Retrieves RAS (Reliability, Availability, Serviceability) error information. (MI300 host systems only, human-readable output only)

    RAS arguments (mutually exclusive):
    - `--cper --severity=<fatal|nonfatal-uncorrected|nonfatal-corrected|all> [--folder=FOLDER] [--file-limit=NUMBER] [--follow]`: Get CPER (Common Platform Error Record) entries based on severity level. Supports GPU filtering with `--gpu`. Optional folder saves error files. File limit controls maximum saved files. Follow enables continuous monitoring.
    - `--afid --cper-file=FILE`: Extract AFID (AMD Field ID) list from existing CPER file. GPU filtering not supported.

## Basic Usage

### Command Syntax

```shell-session
sudo amd-smi <command> <options>
```

- `<command>` is the primary command to execute. It must be the first argument after **amd-smi**.
- `<options>` can include subcommands, modifiers, or other arguments relevant to the specified command.

### Getting Help

To get detailed information about the available commands and options, you can run help command.
The help command provides a comprehensive overview of the tool's functionalities and usage instructions.
Simply run tool without arguments or with command help.

```shell-session
$ sudo amd-smi help

Copyright 2023-2025 Advanced Micro Devices, Inc. All rights reserved.

usage: amd-smi help

AMD System Management Interface | AMD SMI tool version 29.0.0

AMD-SMI Commands:
                      Descriptions:
    version           Display version information
    list              List GPU information
    static            Gets static information about the specified GPU
    metric            Gets metric information about the specified GPU
    monitor           Monitor metrics for target devices
    bad-pages         Gets bad page information about the specified GPU
    event             Displays event information for the given GPU
    firmware          Gets firmware information about the specified GPU
    set               Set options for devices
    reset             Reset options for devices
    xgmi              Displays xgmi information of the devices
    topology          Displays topology information of the devices
    partition         Displays partition information of the devices
    ras               Displays ras information of the devices
```

From help message you can see which subcommands are supported on the system and a short description for each command.

To access the help documentation for a specific command, simply use that command name followed by the help command.
For example, if you want to get help for "list" command you can use the tool the following way.

```shell-session
$ sudo amd-smi list --help

Copyright 2023-2025 Advanced Micro Devices, Inc. All rights reserved.

usage: amd-smi list [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu [GPU ...]]

List all GPUs and VFs on the system and their most basic general information.
If no GPU is specified, returns basic information for all GPUs on the system.

List arguments:
                          Description:
    -h, --help            show this help message and exit
    -g, --gpu [GPU ...]   Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs

Command Modifiers:
                      Description:
--json                Displays output in JSON format (human readable by default).
--csv                 Displays output in CSV format (human readable by default).
--file FILE           Saves output into a file on the provided path (stdout by default).
```

### Output Formats

The **AMD SMI CLI tool** supports three output formats:

#### Human-readable (Default)
```shell-session
$ sudo amd-smi list

GPU: 0
    BDF: 0000:0c:00.0
    UUID: 67ff74a1-0000-1000-8081-b5b9fd6edd00
    VF: 0
        BDF: 0000:0c:02.0
        UUID: 670074a1-0000-1000-8081-b5b9fd6edd00
```

#### JSON Format
```shell-session
$ sudo amd-smi list --json

[
    {
        "gpu": 0,
        "bdf": "0000:0c:00.0",
        "uuid": "67ff74a1-0000-1000-8081-b5b9fd6edd00",
        "vfs": [
            {
                "vf": 0,
                "bdf": "0000:0c:02.0",
                "uuid": "670074a1-0000-1000-8081-b5b9fd6edd00"
            }
        ]
    }
]
```

#### CSV Format
```shell-session
$ sudo amd-smi list --csv

gpu,gpu_bdf,gpu_uuid,vf,vf_bdf,vf_uuid
0,0000:0c:00.0,67ff74a1-0000-1000-8081-b5b9fd6edd00,0,0000:0c:02.0,670074a1-0000-1000-8081-b5b9fd6edd00
```

### Saving Output to File

All outputs can be saved to a file using the `--file` parameter:

```shell-session
sudo amd-smi list --file=output.txt
```

## Command Examples with Sample Outputs

### 1. Version Information

```shell-session
$ sudo amd-smi version
```

**Output:**
```
VERSION:
    TOOL_NAME: AMD SMI tool
    TOOL_VERSION: 29.0.0
    LIB_VERSION: 35.0.0
```

### 2. Static Information

**Get all static information for GPU 0:**

```shell-session
$ sudo amd-smi static --gpu=0
```

**Output:**
```
GPU: 0
    ASIC:
        MARKET_NAME: MI300X
        VENDOR_ID: 0x1002
        VENDOR_NAME: Advanced Micro Devices Inc. [AMD/ATI]
        SUBVENDOR_ID: 0x1002
        DEVICE_ID: 0x74A1
        SUBSYSTEM_ID: 0x74A1
        REV_ID: 0x0
        ASIC_SERIAL: 0xF33397508B72EAAF
        OAM_ID: 2
        NUM_OF_COMPUTE_UNITS: 304
    BUS:
        BDF: 0000:0c:00.0
        MAX_PCIE_WIDTH: 16
        MAX_PCIE_SPEED: 32 GT/s
        PCIE_INTERFACE_VERSION: Gen 5
        SLOT_TYPE: OAM
        MAX_PCIE_INTERFACE_VERSION: Gen 5
    VBIOS:
        NAME: AMD MI300X_PRODUCTION_1VF
        BUILD_DATE: 2024/03/15 14:30
        PART_NUMBER: 113-MI3PRD-001
        VERSION: 022.040.003.036.000001
    BOARD:
        MODEL_NUMBER: 102-G30201-0B
        PRODUCT_SERIAL: PCB068560-0020
        FRU_ID: 113-AMDG302010B14
        PRODUCT_NAME: Instinct MI300X
        MANUFACTURER_NAME: AMD
    LIMIT:
        MAX_POWER: 750 W
        MIN_POWER: 100 W
        SOCKET_POWER: 750 W
        SLOWDOWN_EDGE_TEMPERATURE: N/A C
        SLOWDOWN_HOTSPOT_TEMPERATURE: 100 C
        SLOWDOWN_MEM_TEMPERATURE: 95 C
        SHUTDOWN_EDGE_TEMPERATURE: N/A C
        SHUTDOWN_HOTSPOT_TEMPERATURE: 110 C
        SHUTDOWN_MEM_TEMPERATURE: 105 C
    VRAM:
        TYPE: HBM3
        VENDOR: HYNIX
        SIZE: 196592 MB
        BIT_WIDTH: 8192
        MAX_BANDWIDTH: 5300 GB/s
```

**Get specific static information:**

```shell-session
$ sudo amd-smi static --gpu=0 --asic --limit
```

**Output**
```
GPU: 0
    ASIC:
        MARKET_NAME: AMD Instinct MI350X
        VENDOR_ID: 0x1002
        VENDOR_NAME: Advanced Micro Devices Inc. [AMD/ATI]
        SUBVENDOR_ID: 0x1002
        DEVICE_ID: 0x75A0
        SUBSYSTEM_ID: 0x75A0
        REV_ID: 0x0
        ASIC_SERIAL: 0xBA6524093004B8B2
        OAM_ID: 0
        NUM_OF_COMPUTE_UNITS: 256
    LIMIT:
        MAX_POWER: 1000 W
        MIN_POWER: 0 W
        SOCKET_POWER: 1000 W
        SLOWDOWN_EDGE_TEMPERATURE: N/A
        SLOWDOWN_HOTSPOT_TEMPERATURE: 100 C
        SLOWDOWN_MEM_TEMPERATURE: 115 C
        SHUTDOWN_EDGE_TEMPERATURE: N/A
        SHUTDOWN_HOTSPOT_TEMPERATURE: 110 C
        SHUTDOWN_MEM_TEMPERATURE: 120 C
```

### 3. Metric Information

**Get usage metrics:**

```shell-session
$ sudo amd-smi metric --gpu=0 --usage
```

**Output:**
```
GPU: 0
    USAGE:
        GFX_ACTIVITY: 45 %
        UMC_ACTIVITY: 12 %
        MM_ACTIVITY: 3 %
        VCN_ACTIVITY: [ 0 %, 2 %, 0 %, 1 % ]
        JPEG_ACTIVITY: [ 0 %, 0 %, 1 %, 0 %, 0 %, 0 %, 0 %, 0 % ]
```

**Get power metrics:**

```shell-session
$ sudo amd-smi metric --gpu=0 --power
```

**Output:**
```
GPU: 0
    POWER:
        SOCKET_POWER: 320 W
        GFX_VOLTAGE: 875 mV
        SOC_VOLTAGE: 950 mV
        MEM_VOLTAGE: 1250 mV
        POWER_MANAGEMENT: ENABLED
```

**Get temperature metrics:**

```shell-session
$ sudo amd-smi metric --gpu=0 --temperature
```

**Output:**
```
GPU: 0
    TEMPERATURE:
        EDGE: 65 C
        HOTSPOT: 75 C
        MEM: 68 C
```

**Get clock information:**

```shell-session
$ sudo amd-smi metric --gpu=0 --clock
```

**Output:**
```
GPU: 0
    CLOCK:
        GFX:
            CLK: 1800 MHz
            MIN_CLK: 500 MHz
            MAX_CLK: 2100 MHz
            CLK_LOCKED: DISABLED
            DEEP_SLEEP: DISABLED
        MEM:
            CLK: 1600 MHz
            MIN_CLK: 400 MHz
            MAX_CLK: 1600 MHz
            CLK_LOCKED: DISABLED
            DEEP_SLEEP: DISABLED
```


### 4. Firmware Information

```shell-session
$ sudo amd-smi firmware --gpu=0
```

**Output:**
```
GPU: 0
    FW_LIST:
        FW_0:
            FW_ID: SMU
            FW_VERSION: 0.85.117.1
        FW_1:
            FW_ID: CP_MEC_JT1
            FW_VERSION: 0x80b8
        FW_2:
            FW_ID: CP_MEC1
            FW_VERSION: 0x80b8
        FW_3:
            FW_ID: RLC
            FW_VERSION: 0x45
        FW_4:
            FW_ID: SDMA0
            FW_VERSION: 0x18
        FW_5:
            FW_ID: SDMA1
            FW_VERSION: 0x18
        FW_6:
            FW_ID: SDMA2
            FW_VERSION: 0x18
        FW_7:
            FW_ID: SDMA3
            FW_VERSION: 0x18
        FW_8:
            FW_ID: RLC_V
            FW_VERSION: 0x1a
        FW_9:
            FW_ID: MMSCH
            FW_VERSION: 8.0.19
        FW_10:
            FW_ID: PSP_SYSDRV
            FW_VERSION: 0.36.2.5a
        FW_11:
            FW_ID: PSP_SOSDRV
            FW_VERSION: 0.36.2.5a
        FW_12:
            FW_ID: PSP_KEYDB
            FW_VERSION: 5.0.36.0
        FW_13:
            FW_ID: DFC
            FW_VERSION: 0.1.0.1
        FW_14:
            FW_ID: PSP_BL
            FW_VERSION: 0.a1.2.1e
        FW_15:
            FW_ID: REG_ACCESS_WHITELIST
            FW_VERSION: c.2.36.0
        FW_16:
            FW_ID: P2S_TABLE
            FW_VERSION: 0x50101
        FW_17:
            FW_ID: PSP_SOC
            FW_VERSION: 0.36.2.5a
        FW_18:
            FW_ID: PSP_DBG
            FW_VERSION: 0.36.2.5a
        FW_19:
            FW_ID: PSP_INTF
            FW_VERSION: 0.36.2.5a
        FW_20:
            FW_ID: PSP_RAS
            FW_VERSION: 0.36.2.5a
    ERROR_RECORDS:
```

### 5. Bad Pages Information

```shell-session
$ sudo amd-smi bad-pages --gpu=0
```

**Output:**
```
GPU: 0
    BAD_PAGE_1:
        RETIRED_BAD_PAGE: 0x7FFF12345000
        TIMESTAMP: 01/10/2025:08/41/33
        MEM_CHANNEL: 2
        MCUMC_ID: 1
    BAD_PAGE_2:
        RETIRED_BAD_PAGE: 0x7FFF12346000
        TIMESTAMP: 03/10/2025:06/11/13
        MEM_CHANNEL: 3
        MCUMC_ID: 1
```

### 6. Event Information

```shell-session
$ sudo amd-smi event --gpu=0
```

**Output:**
```
EVENT_INFO:
GPU: 0
    MESSAGE: Temperature threshold exceeded
    CATEGORY: THERMAL
    DATE: 2025-10-08:11:23:07.505
GPU: 0
    MESSAGE: ECC single bit error corrected
    CATEGORY: ECC
    DATE: 2025-10-09:10:34:25.237
```

### 7. Topology Information

```shell-session
$ sudo amd-smi topology --weight
```

**Output:**
```
WEIGHT_TABLE:
             0000:0c:00.0 0000:22:00.0 0000:38:00.0 0000:5c:00.0 0000:9f:00.0 0000:af:00.0 0000:bf:00.0 0000:df:00.0
0000:0c:00.0 0            15           15           15           15           15           15           15
0000:22:00.0 15           0            15           15           15           15           15           15
0000:38:00.0 15           15           0            15           15           15           15           15
0000:5c:00.0 15           15           15           0            15           15           15           15
0000:9f:00.0 15           15           15           15           0            15           15           15
0000:af:00.0 15           15           15           15           15           0            15           15
0000:bf:00.0 15           15           15           15           15           15           0            15
0000:df:00.0 15           15           15           15           15           15           15           0
```

### 8. XGMI Information

```shell-session
$ sudo amd-smi xgmi --caps
```

**Output:**
```
XGMI_CONFIGURATION_SUPPORT_CAPABILITY:
             MODE_1       MODE_2       MODE_4       MODE_8       MODE_CUSTOM
0000:0c:00.0 SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED
0000:22:00.0 SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED
0000:38:00.0 SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED
0000:5c:00.0 SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED
0000:9f:00.0 SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED
0000:af:00.0 SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED
0000:bf:00.0 SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED
0000:df:00.0 SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED    SUPPORTED
```

### 9. Partition Information

```shell-session
$ sudo amd-smi partition --gpu=0 --current
```

**Output:**
```
GPU: 0
    PARTITION:
        ACCELERATOR_PARTITION: SPX
        MEMORY_PARTITION: NPS1
        PARTITION_ID: 0
```

**Get per-partition metrics:**

```shell-session
$ sudo amd-smi metric --vf=0:0 --per-partition
```

**Output:**
```
GPU: 0
    VF: 0
        PER_PARTITION:
            AID_0:
                CLK_VCLK: 29 MHz
                CLK_VCLK_MIN_LIMIT: 914 MHz
                CLK_VCLK_MAX_LIMIT: 1333 MHz
                CLK_DCLK_LIMIT: 22 MHz
                CLK_DCLK_MIN_LIMIT: 711 MHz
                CLK_DCLK_MAX_LIMIT: 1142 MHz
                CLK_SCLK_LIMIT: 28 MHz
                CLK_SCLK_MIN_LIMIT: 888 MHz
                CLK_SCLK_MAX_LIMIT: 1142 MHz
                VCN_ACTIVITY: 0 %
                JPEG_ACTIVITY: [0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %]
            AID_1:
                CLK_VCLK: 29 MHz
                CLK_VCLK_MIN_LIMIT: 914 MHz
                CLK_VCLK_MAX_LIMIT: 1333 MHz
                CLK_DCLK_LIMIT: 22 MHz
                CLK_DCLK_MIN_LIMIT: 711 MHz
                CLK_DCLK_MAX_LIMIT: 1142 MHz
                CLK_SCLK_LIMIT: 28 MHz
                CLK_SCLK_MIN_LIMIT: 888 MHz
                CLK_SCLK_MAX_LIMIT: 1142 MHz
                VCN_ACTIVITY: 0 %
                JPEG_ACTIVITY: [0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %]
            AID_2:
                CLK_VCLK: 29 MHz
                CLK_VCLK_MIN_LIMIT: 914 MHz
                CLK_VCLK_MAX_LIMIT: 1333 MHz
                CLK_DCLK_LIMIT: 22 MHz
                CLK_DCLK_MIN_LIMIT: 711 MHz
                CLK_DCLK_MAX_LIMIT: 1142 MHz
                CLK_SCLK_LIMIT: 28 MHz
                CLK_SCLK_MIN_LIMIT: 888 MHz
                CLK_SCLK_MAX_LIMIT: 1142 MHz
                VCN_ACTIVITY: 0 %
                JPEG_ACTIVITY: [0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %]
            AID_3:
                CLK_VCLK: 29 MHz
                CLK_VCLK_MIN_LIMIT: 914 MHz
                CLK_VCLK_MAX_LIMIT: 1333 MHz
                CLK_DCLK_LIMIT: 22 MHz
                CLK_DCLK_MIN_LIMIT: 711 MHz
                CLK_DCLK_MAX_LIMIT: 1142 MHz
                CLK_SCLK_LIMIT: 28 MHz
                CLK_SCLK_MIN_LIMIT: 888 MHz
                CLK_SCLK_MAX_LIMIT: 1142 MHz
                VCN_ACTIVITY: 0 %
                JPEG_ACTIVITY: [0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %, 0 %]
            XCP_0:
                GFX_CLK: [132 MHz]
                GFX_MIN_CLK: [500 MHz]
                GFX_MAX_CLK: [2100 MHz]
                GFX_CLK_LOCKED: [DISABLED]
                GFX_USAGE: [0 %]
            XCP_1:
                GFX_CLK: [132 MHz]
                GFX_MIN_CLK: [500 MHz]
                GFX_MAX_CLK: [2100 MHz]
                GFX_CLK_LOCKED: [DISABLED]
                GFX_USAGE: [0 %]
            XCP_2:
                GFX_CLK: [132 MHz]
                GFX_MIN_CLK: [500 MHz]
                GFX_MAX_CLK: [2100 MHz]
                GFX_CLK_LOCKED: [DISABLED]
                GFX_USAGE: [0 %]
            XCP_3:
                GFX_CLK: [132 MHz]
                GFX_MIN_CLK: [500 MHz]
                GFX_MAX_CLK: [2100 MHz]
                GFX_CLK_LOCKED: [DISABLED]
                GFX_USAGE: [0 %]
            XCP_4:
                GFX_CLK: [132 MHz]
                GFX_MIN_CLK: [500 MHz]
                GFX_MAX_CLK: [2100 MHz]
                GFX_CLK_LOCKED: [DISABLED]
                GFX_USAGE: [0 %]
            XCP_5:
                GFX_CLK: [132 MHz]
                GFX_MIN_CLK: [500 MHz]
                GFX_MAX_CLK: [2100 MHz]
                GFX_CLK_LOCKED: [DISABLED]
                GFX_USAGE: [0 %]
            XCP_6:
                GFX_CLK: [132 MHz]
                GFX_MIN_CLK: [500 MHz]
                GFX_MAX_CLK: [2100 MHz]
                GFX_CLK_LOCKED: [DISABLED]
                GFX_USAGE: [0 %]
            XCP_7:
                GFX_CLK: [132 MHz]
                GFX_MIN_CLK: [500 MHz]
                GFX_MAX_CLK: [2100 MHz]
                GFX_CLK_LOCKED: [DISABLED]
                GFX_USAGE: [0 %]
```



**Get global partition configuration:**

```shell-session
$ sudo amd-smi partition --global
```
**Output:**
```
GLOBAL_PARTITION_CONFIG:
GPU  ACCELERATOR_TYPE  SUPPORTED_VF_MODE  MEMORY_PARTITION_CAPS
0        SPX               1                  NPS1
         DPX               2                  NPS1
         QPX               4                  NPS1,NPS4
         CPX               1,2,4,8            NPS1,NPS4
1        SPX               1                  NPS1
         DPX               2                  NPS1
         QPX               4                  NPS1,NPS4
         CPX               1,2,4,8            NPS1,NPS4
2        SPX               1                  NPS1
         DPX               2                  NPS1
         QPX               4                  NPS1,NPS4
         CPX               1,2,4,8            NPS1,NPS4
3        SPX               1                  NPS1
         DPX               2                  NPS1
         QPX               4                  NPS1,NPS4
         CPX               1,2,4,8            NPS1,NPS4
4        SPX               1                  NPS1
         DPX               2                  NPS1
         QPX               4                  NPS1,NPS4
         CPX               1,2,4,8            NPS1,NPS4
5        SPX               1                  NPS1
         DPX               2                  NPS1
         QPX               4                  NPS1,NPS4
         CPX               1,2,4,8            NPS1,NPS4
6        SPX               1                  NPS1
         DPX               2                  NPS1
         QPX               4                  NPS1,NPS4
         CPX               1,2,4,8            NPS1,NPS4
7        SPX               1                  NPS1
         DPX               2                  NPS1
         QPX               4                  NPS1,NPS4
         CPX               1,2,4,8            NPS1,NPS4

```

### 10. Monitor Command (Continuous Monitoring)

```shell-session
$ sudo amd-smi monitor
```

**Output (updates every second):**
```
 GPU  POWER  HOTSPOT_TEMP  MEM_TEMP  GFX_UTIL  GFX_CLOCK  MEM_UTIL  MEM_CLOCK  ENC_UTIL    VCLK  DEC_UTIL    DCLK  CORRECTABLE_ECC  UNCORRECTABLE_ECC  PCIE_REPLAY   PCIE_BW
   0  156 W          38 C      33 C       0 %    132 MHz       0 %    900 MHz       0 %  29 MHz       0 %  22 MHz                0                  0            0   18 Mb/s
   1  153 W          38 C      31 C       0 %    132 MHz       0 %    900 MHz       0 %  29 MHz       0 %  22 MHz                0                  0            0   44 Mb/s
   2  149 W          35 C      30 C       0 %    132 MHz       0 %    900 MHz       0 %  29 MHz       0 %  22 MHz                0                  0            0   18 Mb/s
   3  140 W          36 C      31 C       0 %    132 MHz       0 %    900 MHz       0 %  29 MHz       0 %  22 MHz                0                  0            0   18 Mb/s
   4  149 W          33 C      31 C       0 %    132 MHz       0 %    900 MHz       0 %  29 MHz       0 %  22 MHz                0                  0            0   18 Mb/s
   5  151 W          39 C      33 C       0 %    132 MHz       0 %    900 MHz       0 %  29 MHz       0 %  22 MHz                0                  0            0   41 Mb/s
   6  140 W          35 C      31 C       0 %    132 MHz       0 %    900 MHz       0 %  29 MHz       0 %  22 MHz                0                  0            0   18 Mb/s
   7  140 W          38 C      33 C       0 %    132 MHz       0 %    900 MHz       0 %  29 MHz       0 %  22 MHz                0                  0            0   18 Mb/s
```

### 11. Set Commands

**Set power cap:**

```shell-session
$ sudo amd-smi set --gpu=0 --power-cap=600
```

**Output:**
```
GPU: 0
    POWER_CAP: Successfully set power cap to 600 W
```

**Set memory partition:**

```shell-session
$ sudo amd-smi set --memory-partition=NPS2
```

**Output**
```
Setting memory-partition in progress. This may take a while...

GPU: 0
    MEMORY_PARTITION: Successfully set memory partition to NPS2
GPU: 1
    MEMORY_PARTITION: Successfully set memory partition to NPS2
GPU: 2
    MEMORY_PARTITION: Successfully set memory partition to NPS2
GPU: 3
    MEMORY_PARTITION: Successfully set memory partition to NPS2
GPU: 4
    MEMORY_PARTITION: Successfully set memory partition to NPS2
GPU: 5
    MEMORY_PARTITION: Successfully set memory partition to NPS2
GPU: 6
    MEMORY_PARTITION: Successfully set memory partition to NPS2
GPU: 7
    MEMORY_PARTITION: Successfully set memory partition to NPS2
```

**Set number of VFs enabled:**

```shell-session
$ sudo amd-smi set --num-vf=1
```

**Output**
```
GPU: 0
    NUM_VF_ENABLED: Successfully set enabled VFs to 1
GPU: 1
    NUM_VF_ENABLED: Successfully set enabled VFs to 1
GPU: 2
    NUM_VF_ENABLED: Successfully set enabled VFs to 1
GPU: 3
    NUM_VF_ENABLED: Successfully set enabled VFs to 1
GPU: 4
    NUM_VF_ENABLED: Successfully set enabled VFs to 1
GPU: 5
    NUM_VF_ENABLED: Successfully set enabled VFs to 1
GPU: 6
    NUM_VF_ENABLED: Successfully set enabled VFs to 1
GPU: 7
    NUM_VF_ENABLED: Successfully set enabled VFs to 1
```

**Set XGMI Per-Link Down Policy**

```shell-session
$ sudo amd-smi set --xgmi-plpd=0
```

**Output**
```
GPU: 0
    DPM_POLICY: Successfully set xgmi per-link power down policy to 0
GPU: 1
    DPM_POLICY: Successfully set xgmi per-link power down policy to 0
GPU: 2
    DPM_POLICY: Successfully set xgmi per-link power down policy to 0
GPU: 3
    DPM_POLICY: Successfully set xgmi per-link power down policy to 0
GPU: 4
    DPM_POLICY: Successfully set xgmi per-link power down policy to 0
GPU: 5
    DPM_POLICY: Successfully set xgmi per-link power down policy to 0
GPU: 6
    DPM_POLICY: Successfully set xgmi per-link power down policy to 0
GPU: 7
    DPM_POLICY: Successfully set xgmi per-link power down policy to 0
```

**Set SOC Pstate**

```shell-session
$ sudo amd-smi set --soc-pstate=0
```

**Output**
```
GPU: 0
    SOC_PSTATE: Successfully set dpm soc pstate policy to 0
GPU: 1
    SOC_PSTATE: Successfully set dpm soc pstate policy to 0
GPU: 2
    SOC_PSTATE: Successfully set dpm soc pstate policy to 0
GPU: 3
    SOC_PSTATE: Successfully set dpm soc pstate policy to 0
GPU: 4
    SOC_PSTATE: Successfully set dpm soc pstate policy to 0
GPU: 5
    SOC_PSTATE: Successfully set dpm soc pstate policy to 0
GPU: 6
    SOC_PSTATE: Successfully set dpm soc pstate policy to 0
GPU: 7
    SOC_PSTATE: Successfully set dpm soc pstate policy to 0

```

**Set XGMI FB Sharing Mode**

```shell-session
$ sudo amd-smi set --xgmi --fb-sharing-mode=MODE_1
```

**Output**
```
XGMI FB_SHARING_MODE: Successfully set mode to MODE_1 for the given group/s
```

### 12. Reset Commands

**Reset GPU:**

```shell-session
$ sudo amd-smi reset --gpureset
```

**Output**
```
GPU: 0
    GPU_RESET: Successfully reset GPU
GPU: 1
    GPU_RESET: Successfully reset GPU
GPU: 2
    GPU_RESET: Successfully reset GPU
GPU: 3
    GPU_RESET: Successfully reset GPU
GPU: 4
    GPU_RESET: Successfully reset GPU
GPU: 5
    GPU_RESET: Successfully reset GPU
GPU: 6
    GPU_RESET: Successfully reset GPU
GPU: 7
    GPU_RESET: Successfully reset GPU
```

**Clean VF framebuffer:**

```shell-session
$ sudo amd-smi reset --vf=0:0 --vf-fb
```

**Output**
```
Successfully reset vf fb for vf with id: 0:0
```

### 13. RAS Error Information (MI300 Host Systems Only)

**Get CPER entries with fatal severity:**

```shell-session
$ sudo amd-smi ras --cper --severity=fatal --folder=/tmp/ras_logs
```

**Output:**
```
timestamp                gpu_id   severity                 file_name          list of afids
07/10/2025 09:03:09      0        fatal                    fatal-1.cper       24
07/10/2025 09:04:15      1        fatal                    fatal-2.cper       24 29
```

**Get all CPER entries with continuous monitoring:**

```shell-session
$ sudo amd-smi ras --cper --severity=all --folder=/tmp/ras_logs --file-limit=10 --follow
```

**Extract AFID from existing CPER file:**

```shell-session
$ sudo amd-smi ras --afid --cper-file=/tmp/ras_logs/fatal-2.cper
```

**Output:**
```
24 29
```

### 14. JSON and CSV Format Examples

**JSON format for metrics:**

```shell-session
$ sudo amd-smi metric --pcie --gpu=0 --json
```

**Output:**
```json
[
    {
        "gpu": 0,
        "pcie": {
            "width": 16,
            "speed": {
                "value": 32,
                "unit": "GT/s"
            },
            "bandwidth": {
                "value": 18,
                "unit": "Mb/s"
            },
            "replay_count": 0,
            "l0_to_recovery_count": 0,
            "replay_roll_over_count": 0,
            "nak_sent_count": 0,
            "nak_received_count": 0
        }
    }
]
```

**CSV format for usage:**

```shell-session
$ sudo amd-smi metric --pcie --gpu=0 --csv
```

**Output:**
```
gpu,pcie_current_width,pcie_current_speed,pcie_current_bandwidth,pcie_replay_count,pcie_l0_to_recovery_count,pcie_replay_roll_over_count,pcie_nak_sent_count,pcie_nak_received_count
0,16,32,18,0,0,0,0,0
```

## Use Case Scenarios

This section provides practical workflows for common administrative tasks using **amd-smi**.

### Memory Partition Management

**Scenario**: Configure memory partitioning modes

```bash
# Step 1: Check current memory partition configuration
$ sudo amd-smi partition --gpu=0 --current

# Step 2: List available memory partition modes
$ sudo amd-smi partition --gpu=0 --memory-partition

# Step 3: Set memory partition to NPS2
$ sudo amd-smi set --memory-partition=NPS2

# Step 4: Verify the partition change
$ sudo amd-smi partition --gpu=0 --current
```

### Accelerator Partition Configuration

**Scenario**: Set up accelerator partitioning modes

```bash
# Step 1: Check available accelerator partition profiles
$ sudo amd-smi partition --gpu=0 --accelerator-partition

# Step 2: View current accelerator partition setting
$ sudo amd-smi partition --gpu=0 --current

# Step 3: Set accelerator partition to profile 2 (example)
$ sudo amd-smi set --accelerator-partition=2

# Step 4: Verify the partition configuration
$ sudo amd-smi partition --gpu=0 --current
```

### XGMI Framebuffer Sharing Setup

**Scenario**: Configure framebuffer sharing

```bash
# Step 1: Check XGMI capabilities and current configuration
$ sudo amd-smi xgmi --caps
$ sudo amd-smi xgmi --fb-sharing

# Step 2: View topology to understand GPU connections
$ sudo amd-smi topology

# Step 3: Set framebuffer sharing mode for 2-GPU group
$ sudo amd-smi set --xgmi --fb-sharing-mode=MODE_2

# Step 4: Verify the framebuffer sharing configuration
$ sudo amd-smi topology --fb-sharing
```

### System Health Monitoring

**Scenario**: Perform comprehensive system health check

```bash
# Step 1: Check for bad pages and ECC errors
$ sudo amd-smi bad-pages
$ sudo amd-smi metric --ecc

# Step 2: Monitor temperatures and power consumption
$ sudo amd-smi metric --temperature
$ sudo amd-smi metric --power

# Step 3: Check thermal and power limits
$ sudo amd-smi static --limit

# Step 4: Monitor real-time performance metrics (updates continuously)
$ sudo amd-smi monitor --gpu=0 --temperature --power-usage --ecc --watch=1
```

### Performance Analysis Workflow

**Scenario**: Analyze GPU performance and utilization patterns

```bash
# Step 1: Get baseline static information
$ sudo amd-smi static --gpu=0 --asic --vram --cache

# Step 2: Monitor GPU utilization over time (updates every second for 60 seconds)
$ sudo amd-smi monitor --gpu=0 --gfx --mem --power-usage --watch=1 --watch-time=60

# Step 3: Check detailed metrics for bottleneck analysis
$ sudo amd-smi metric --gpu=0 --usage --clock --temperature --pcie

# Step 4: Export performance data for analysis
$ sudo amd-smi metric --gpu=0 --usage --clock --power --json > performance_data.json
```

### VF Management in SR-IOV Environment

**Scenario**: Manage Virtual Functions for GPU virtualization

```bash
# Step 1: List all GPUs and available VFs
$ sudo amd-smi list

# Step 2: Check VF capabilities and current configuration
$ sudo amd-smi static --gpu=0 --num-vf

# Step 3: Configure the number of VFs (if modification needed)
$ sudo amd-smi set --gpu=0 --num-vf=8

# Step 4: Verify VF configuration after change
$ sudo amd-smi static --gpu=0 --num-vf

# Step 5: Monitor VF performance metrics
$ sudo amd-smi metric --vf=0:0 --schedule --guard
```

### Firmware and Driver Validation

**Scenario**: Validate firmware versions and driver compatibility

```bash
# Step 1: Check current firmware versions
$ sudo amd-smi firmware --gpu=0 --fw-list

# Step 2: Verify driver version and compatibility
$ sudo amd-smi static --gpu=0 --driver

# Step 3: Check for firmware error records
$ sudo amd-smi firmware --gpu=0 --error-records
```

### RAS Error Monitoring (MI300 Host Systems)

**Scenario**: Monitor and analyze hardware reliability errors

```bash
# Step 1: Check for critical fatal errors and save to files
$ sudo amd-smi ras --cper --severity=fatal --folder=/var/log/gpu_errors --file-limit=50

# Step 2: Monitor all error types with continuous monitoring
$ sudo amd-smi ras --cper --severity=all --folder=/var/log/gpu_errors --follow --file-limit=100

# Step 3: Analyze existing error files to extract firmware component IDs
$ sudo amd-smi ras --afid --cper-file=/var/log/gpu_errors/fatal-1.cper

# Step 4: Monitor specific GPU for non-fatal corrected errors
$ sudo amd-smi ras --gpu=0 --cper --severity=nonfatal-corrected --folder=/var/log/gpu0_errors
