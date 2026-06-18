---
myst:
  html_meta:
    "description lang=en": "Explore the AMD SMI Go API."
    "keywords": "api, smi, lib, go, system, management, interface"
---

# AMD SMI Go API reference

Go interface – Consists of Go function declarations, which directly call the C interface via CGO. The client can use the Go interface to build applications in Go.

## Requirements

* Go 1.18+ 64-bit
* AMD SMI C library installed (`libamdsmi.so` on Linux)
* AMD GPU hardware (for runtime)

## Overview

### Folder structure

File Name | Note
---|---
`amdsmi/amdsmi_interface.go` | AMD SMI library Go interface (types, constants, functions)
`examples/usage.go` | Usage examples for the APIs in `amdsmi/amdsmi_interface.go`. Kept in sync as new APIs are added.
`go.mod` | Go module definition
`README.md` | Documentation

### Architecture

The bindings are a single file (`amdsmi/amdsmi_interface.go`) that calls the AMD SMI C library directly via CGO.

```
Go application
    │
    ▼
amdsmi package (amdsmi_interface.go)    ← idiomatic Go API
    │
    ▼ (CGO)
libamdsmi.so                            ← AMD SMI C library
    │
    ▼
AMD GPU hardware
```

### Build & Run

Navigate to the `go/` directory and run:

```bash
sudo go run examples/usage.go
```

Ensure `libamdsmi.so` is on the linker path (e.g., `/usr/local/lib` or set `LD_LIBRARY_PATH`).

## Amdsmi usage

The `amdsmi` package should be imported and used as follows:

```go
package main

import (
    "fmt"
    "smi-lib/go/amdsmi"
)

func main() {
    if err := amdsmi.Init(amdsmi.AMDSMI_INIT_AMD_GPUS); err != nil {
        fmt.Printf("Init failed: %v\n", err)
        return
    }
    defer amdsmi.ShutDown()

    handles, _ := amdsmi.GetProcessorHandles()
    for _, h := range handles {
        info, _ := amdsmi.GetGpuAsicInfo(h)
        fmt.Printf("GPU: %s (0x%X)\n", info.MarketName, info.DeviceID)
    }
}
```

To initialize the library, `Init` must be called before all other calls.

To close connection to the driver, `ShutDown` must be the last call.

## Amdsmi Error Handling

All functions return `error` as the last return value.
Errors are `*StatusError` with a `Code` field mapping to `amdsmi_status_t` values and a `Name` field with the status name string.

Example:

```go
info, err := amdsmi.GetGpuAsicInfo(handle)
if err != nil {
    var smiErr *amdsmi.StatusError
    if errors.As(err, &smiErr) {
        fmt.Printf("SMI error: %s (code %d)\n", smiErr.Name, smiErr.Code)
    }
}
```

Status codes returned (type `Status`):

Field | Value | Description
---|---|---
`AMDSMI_STATUS_SUCCESS` | 0 | Call succeeded
`AMDSMI_STATUS_INVAL` | 1 | Invalid parameters
`AMDSMI_STATUS_NOT_SUPPORTED` | 2 | Command not supported
`AMDSMI_STATUS_NOT_YET_IMPLEMENTED` | 3 | Not implemented yet
`AMDSMI_STATUS_FAIL_LOAD_MODULE` | 4 | Fail to load lib
`AMDSMI_STATUS_FAIL_LOAD_SYMBOL` | 5 | Fail to load symbol
`AMDSMI_STATUS_DRM_ERROR` | 6 | Error when call libdrm
`AMDSMI_STATUS_API_FAILED` | 7 | API call failed
`AMDSMI_STATUS_TIMEOUT` | 8 | Timeout in API call
`AMDSMI_STATUS_RETRY` | 9 | Retry operation
`AMDSMI_STATUS_NO_PERM` | 10 | Permission denied
`AMDSMI_STATUS_INTERRUPT` | 11 | An interrupt occurred during execution of function
`AMDSMI_STATUS_IO` | 12 | I/O error
`AMDSMI_STATUS_ADDRESS_FAULT` | 13 | Bad address
`AMDSMI_STATUS_FILE_ERROR` | 14 | Problem accessing a file
`AMDSMI_STATUS_OUT_OF_RESOURCES` | 15 | Not enough memory
`AMDSMI_STATUS_INTERNAL_EXCEPTION` | 16 | An internal exception was caught
`AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS` | 17 | The provided input is out of allowable or safe range
`AMDSMI_STATUS_INIT_ERROR` | 18 | An error occurred when initializing internal data structures
`AMDSMI_STATUS_REFCOUNT_OVERFLOW` | 19 | An internal reference counter exceeded INT32_MAX
`AMDSMI_STATUS_DIRECTORY_NOT_FOUND` | 20 | Error when a directory is not found, maps to ENOTDIR
`AMDSMI_STATUS_BUSY` | 30 | Processor busy
`AMDSMI_STATUS_NOT_FOUND` | 31 | Processor not found
`AMDSMI_STATUS_NOT_INIT` | 32 | Processor not initialized
`AMDSMI_STATUS_NO_SLOT` | 33 | No more free slot
`AMDSMI_STATUS_DRIVER_NOT_LOADED` | 34 | Processor driver not loaded
`AMDSMI_STATUS_MORE_DATA` | 39 | There is more data than the buffer size the user passed
`AMDSMI_STATUS_NO_DATA` | 40 | No data was found for a given input
`AMDSMI_STATUS_INSUFFICIENT_SIZE` | 41 | Not enough resources were available for the operation
`AMDSMI_STATUS_UNEXPECTED_SIZE` | 42 | An unexpected amount of data was read
`AMDSMI_STATUS_UNEXPECTED_DATA` | 43 | The data read or provided to function is not what was expected
`AMDSMI_STATUS_NON_AMD_CPU` | 44 | System has different cpu than AMD
`AMDSMI_STATUS_NO_ENERGY_DRV` | 45 | Energy driver not found
`AMDSMI_STATUS_NO_MSR_DRV` | 46 | MSR driver not found
`AMDSMI_STATUS_NO_HSMP_DRV` | 47 | HSMP driver not found
`AMDSMI_STATUS_NO_HSMP_SUP` | 48 | HSMP not supported
`AMDSMI_STATUS_NO_HSMP_MSG_SUP` | 49 | HSMP message/feature not supported
`AMDSMI_STATUS_HSMP_TIMEOUT` | 50 | HSMP message timed out
`AMDSMI_STATUS_NO_DRV` | 51 | No Energy and HSMP driver present
`AMDSMI_STATUS_FILE_NOT_FOUND` | 52 | File or directory not found
`AMDSMI_STATUS_ARG_PTR_NULL` | 53 | Parsed argument is invalid
`AMDSMI_STATUS_AMDGPU_RESTART_ERR` | 54 | AMDGPU restart failed
`AMDSMI_STATUS_SETTING_UNAVAILABLE` | 55 | Setting is not available
`AMDSMI_STATUS_CORRUPTED_EEPROM` | 56 | EEPROM is corrupted
`AMDSMI_STATUS_MAP_ERROR` | 0xFFFFFFFE | The internal library error did not map to a status code
`AMDSMI_STATUS_UNKNOWN_ERROR` | 0xFFFFFFFF | An unknown error occurred

## Amdsmi API

### Init

Description: Initialize smi lib and connect to driver

Input parameters:

* `flags` `InitFlags` type value (**_Optional concept: if using `AMDSMI_INIT_ALL_PROCESSORS`, all processor types are initialized_**)

`InitFlags` type:

Field | Description
---|---
`AMDSMI_INIT_AMD_CPUS` | AMD CPUs
`AMDSMI_INIT_AMD_GPUS` | AMD GPUs
`AMDSMI_INIT_NON_AMD_CPUS` | Non-AMD CPUs
`AMDSMI_INIT_NON_AMD_GPUS` | Non-AMD GPUs
`AMDSMI_INIT_AMD_APUS` | AMD APUs (CPUs + GPUs)
`AMDSMI_INIT_AMD_NICS` | AMD NICs
`AMDSMI_INIT_ALL_PROCESSORS` | All processors

Output: `error`

Errors that can be returned by `Init` function:

* `*StatusError`

Example:

```go
if err := amdsmi.Init(amdsmi.AMDSMI_INIT_AMD_GPUS); err != nil {
    fmt.Printf("Init failed: %v\n", err)
    return
}
defer amdsmi.ShutDown()
```

### ShutDown

Description: Finalize and close connection to driver

Input parameters: `None`

Output: `error`

Errors that can be returned by `ShutDown` function:

* `*StatusError`

Example:

```go
err := amdsmi.ShutDown()
if err != nil {
    fmt.Printf("ShutDown failed: %v\n", err)
}
```

### GetProcessorHandlesByType

Description: Returns a slice of processor handles of the specified type in the system.

Input parameters:

* `pt` one of `ProcessorType` type values:

Field | Description
---|---
`AMDSMI_PROCESSOR_TYPE_UNKNOWN` | Unknown processor type
`AMDSMI_PROCESSOR_TYPE_AMD_GPU` | AMD GPU device
`AMDSMI_PROCESSOR_TYPE_AMD_CPU` | AMD CPU device (**_Not supported yet_**)
`AMDSMI_PROCESSOR_TYPE_NON_AMD_GPU` | Non-AMD GPU device (**_Not supported yet_**)
`AMDSMI_PROCESSOR_TYPE_NON_AMD_CPU` | Non-AMD CPU device (**_Not supported yet_**)
`AMDSMI_PROCESSOR_TYPE_AMD_CPU_CORE` | AMD CPU core (**_Not supported yet_**)
`AMDSMI_PROCESSOR_TYPE_AMD_APU` | AMD APU (**_Not supported yet_**)
`AMDSMI_PROCESSOR_TYPE_AMD_NIC` | AMD Network Interface Card (NIC)
`AMDSMI_PROCESSOR_TYPE_BRCM_NIC` | Broadcom Network Interface Card (NIC)
`AMDSMI_PROCESSOR_TYPE_BRCM_SWITCH` | Broadcom switch

Output: `[]ProcessorHandle`, `error`

Errors that can be returned by `GetProcessorHandlesByType` function:

* `*StatusError`

Example:

```go
gpus, err := amdsmi.GetProcessorHandlesByType(amdsmi.AMDSMI_PROCESSOR_TYPE_AMD_GPU)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(gpus) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, gpu := range gpus {
        uuid, _ := amdsmi.GetGpuDeviceUuid(gpu)
        fmt.Println(uuid)
    }
}
```

### GetProcessorType

Description: Returns the processor type of the given processor handle

Input parameters:

* `ph` (`ProcessorHandle`) processor handle for which to query

Output: `ProcessorType`, `error`

`ProcessorType` type values:

Field | Description
---|---
`AMDSMI_PROCESSOR_TYPE_UNKNOWN` | Unknown processor type
`AMDSMI_PROCESSOR_TYPE_AMD_GPU` | AMD GPU processor
`AMDSMI_PROCESSOR_TYPE_AMD_CPU` | AMD CPU processor
`AMDSMI_PROCESSOR_TYPE_NON_AMD_GPU` | Non-AMD GPU processor
`AMDSMI_PROCESSOR_TYPE_NON_AMD_CPU` | Non-AMD CPU processor
`AMDSMI_PROCESSOR_TYPE_AMD_CPU_CORE` | AMD CPU core processor
`AMDSMI_PROCESSOR_TYPE_AMD_APU` | AMD APU processor
`AMDSMI_PROCESSOR_TYPE_AMD_NIC` | AMD NIC processor
`AMDSMI_PROCESSOR_TYPE_BRCM_NIC` | Broadcom NIC processor
`AMDSMI_PROCESSOR_TYPE_BRCM_SWITCH` | Broadcom switch processor

Errors that can be returned by `GetProcessorType` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        ptype, _ := amdsmi.GetProcessorType(processor)
        fmt.Printf("Processor Type: %s (%d)\n", ptype, ptype)
    }
}
```

### GetProcessorHandles

Description: Returns slice of GPU device handle objects on current machine

Note: This function currently supports only AMD GPUs. To enumerate other devices, such as AMD NICs, use `GetProcessorHandlesByType` function.

Input parameters: `None`

Output: `[]ProcessorHandle`, `error`

Errors that can be returned by `GetProcessorHandles` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        fmt.Println(amdsmi.GetGpuDeviceUuid(processor))
    }
}
```

### GetProcessorHandleFromBdf

Description: Returns processor handle from the given BDF

Input parameters:

* `bdfInput` (`Bdf`) BDF identifier as uint64

Output: `ProcessorHandle`, `error`

Errors that can be returned by `GetProcessorHandleFromBdf` function:

* `*StatusError`

Example:

```go
processor, err := amdsmi.GetProcessorHandleFromBdf(0x00002300) // 0000:23:00.0
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
uuid, _ := amdsmi.GetGpuDeviceUuid(processor)
fmt.Println(uuid)
```

### GetGpuDeviceBdf

Description: Returns BDF of the given processor (GPU PF).

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query

Output: `Bdf`, `error`

`Bdf` is a uint64 with helper methods and `String()` returning format `<domain>:<bus>:<device>.<function>` in hex.
Where:

* `<domain>` is 4 hex digits long from 0000-FFFF interval
* `<bus>` is 2 hex digits long from 00-FF interval
* `<device>` is 2 hex digits long from 00-1F interval
* `<function>` is 1 hex digit long from 0-7 interval

Errors that can be returned by `GetGpuDeviceBdf` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        bdf, _ := amdsmi.GetGpuDeviceBdf(processor)
        fmt.Printf("GPU BDF: %s\n", bdf)
    }
}
```

### GetGpuDeviceUuid

Description: Returns the UUID of the device

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query

Output: UUID `string`, `error`

Errors that can be returned by `GetGpuDeviceUuid` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        uuid, _ := amdsmi.GetGpuDeviceUuid(processor)
        fmt.Printf("GPU UUID: %s\n", uuid)
    }
}
```

### GetGpuVirtualizationMode

Description: Retrieve the current GPU virtualization mode.

Input parameters:

* `ph` (`ProcessorHandle`) The handle to the GPU device for which the virtualization mode is being queried.

Output: `VirtualizationMode`, `error`

`VirtualizationMode` type:

Field | Description
---|---
`AMDSMI_VIRTUALIZATION_MODE_UNKNOWN` | unknown virtualization mode
`AMDSMI_VIRTUALIZATION_MODE_BAREMETAL` | bare metal virtualization mode
`AMDSMI_VIRTUALIZATION_MODE_HOST` | host virtualization mode
`AMDSMI_VIRTUALIZATION_MODE_GUEST` | guest virtualization mode
`AMDSMI_VIRTUALIZATION_MODE_PASSTHROUGH` | passthrough virtualization mode

Errors that can be returned by `GetGpuVirtualizationMode` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        mode, _ := amdsmi.GetGpuVirtualizationMode(processor)
        fmt.Printf("Virtualization Mode: %d\n", mode)
    }
}
```

### GetCpuAffinityWithScope

Description: Returns slice of bitmask information for the given GPU.

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query
* `scope` (`AffinityScope`) value for numa or socket affinity

`AffinityScope` type:

Field | Description
---|---
`AMDSMI_AFFINITY_SCOPE_NODE` | NUMA node scope
`AMDSMI_AFFINITY_SCOPE_SOCKET` | Socket scope

Output: `[]uint64` (bitmask array of length 4), `error`

Errors that can be returned by `GetCpuAffinityWithScope` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        cpuSet, _ := amdsmi.GetCpuAffinityWithScope(processor, amdsmi.AMDSMI_AFFINITY_SCOPE_NODE)
        fmt.Printf("CPU Affinity: %v\n", cpuSet)
    }
}
```

### GetNodeHandle

Description: Get the node handle associated with processor handle.
Note: This function retrieves the node handle of a processor handle. The
      processor_handle must be provided for the processor. Currently,
      only AMD GPUs are supported.

Input parameters:

* `ph` (`ProcessorHandle`) processor handle

Output: `NodeHandle`, `error`

Errors that can be returned by `GetNodeHandle` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        nh, err := amdsmi.GetNodeHandle(processor)
        if err != nil {
            fmt.Printf("Error: %v\n", err)
            continue
        }
        fmt.Printf("Node handle: %v\n", nh)
    }
}
```

### GetIndexFromProcessorHandle

Description: Returns the index of the given processor handle

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query

Output: `uint32`, `error`

Errors that can be returned by `GetIndexFromProcessorHandle` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    idx, err := amdsmi.GetIndexFromProcessorHandle(processors[0])
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    fmt.Printf("  GetIndexFromProcessorHandle: %d\n", idx)
}
```

### GetProcessorHandleFromIndex

Description: Returns the processor handle from the given processor index

Input parameters:

* `index` (uint32) 0-based processor index

Output: `ProcessorHandle`, `error`

Errors that can be returned by `GetProcessorHandleFromIndex` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for i := uint32(0); i < uint32(len(processors)); i++ {
        handle, err := amdsmi.GetProcessorHandleFromIndex(i)
        if err != nil {
            fmt.Printf("Index %d: %v\n", i, err)
            continue
        }
        fmt.Printf("Index %d: handle acquired\n", i)
        _ = handle
    }
}
```

### GetProcessorBdf

Description: Returns BDF of the given processor.

Input parameters:

* `ph` (`ProcessorHandle`) Processor for which to query

Output: `Bdf`, `error`

BDF string format: `<domain>:<bus>:<device>.<function>` in hex.
Where:

* `<domain>` is 4 hex digits long from 0000-FFFF interval
* `<bus>` is 2 hex digits long from 00-FF interval
* `<device>` is 2 hex digits long from 00-1F interval
* `<function>` is 1 hex digit long from 0-7 interval

Errors that can be returned by `GetProcessorBdf` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        bdf, _ := amdsmi.GetProcessorBdf(processor)
        fmt.Printf("Processor BDF: %s\n", bdf)
    }
}
```

### GetProcessorHandleFromUuid

Description: Returns processor handle from the given UUID

Input parameters:

* `uuid` (string) GPU device UUID

Output: `ProcessorHandle`, `error`

Errors that can be returned by `GetProcessorHandleFromUuid` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        uuid, _ := amdsmi.GetGpuDeviceUuid(processor)
        recovered, err := amdsmi.GetProcessorHandleFromUuid(uuid)
        if err != nil {
            fmt.Printf("Error: %v\n", err)
            continue
        }
        fmt.Printf("Processor UUID: %s\n", uuid)
        _ = recovered
    }
}
```

### GetVfHandleFromBdf

Description: Returns processor handle (VF) from the given BDF

Input parameters:

* `bdfInput` (`Bdf`) BDF identifier

Output: `VfHandle`, `error`

Errors that can be returned by `GetVfHandleFromBdf` function:

* `*StatusError`

Example:

```go
vf, err := amdsmi.GetVfHandleFromBdf(bdf)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
uuid, _ := amdsmi.GetVfUuid(vf)
fmt.Printf("VF UUID: %s\n", uuid)
```

### GetVfHandleFromUuid

Description: Returns the handle of a virtual function (VF) from the given UUID

Input parameters:

* `uuid` (string) VF UUID

Output: `VfHandle`, `error`

Errors that can be returned by `GetVfHandleFromUuid` function:

* `*StatusError`

Example:

```go
vf, err := amdsmi.GetVfHandleFromUuid("87007460-0000-1000-8059-3ae746ab9206")
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
uuid, _ := amdsmi.GetVfUuid(vf)
fmt.Printf("VF UUID: %s\n", uuid)
```

### GetVfHandleFromVfIndex

Description: Returns VF id of the VF referenced by its index (in partitioning info)

Input parameters:

* `ph` (`ProcessorHandle`) PF or child VF of a GPU device for which to query
* `fcnIdx` (int) Index of VF (0-31) in GPU's partitioning info

Output: `VfHandle`, `error`

Errors that can be returned by `GetVfHandleFromVfIndex` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    vf, err := amdsmi.GetVfHandleFromVfIndex(processors[0], 0)
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    info, _ := amdsmi.GetVfInfo(vf)
    fmt.Printf("FB Size: %d MB\n", info.Fb.FbSize)
}
```

### GetVfBdf

Description: Returns BDF of the given VF

Input parameters:

* `vh` (`VfHandle`) VF for which to query

Output: `Bdf`, `error`

BDF string format: `<domain>:<bus>:<device>.<function>` in hex.
Where:

* `<domain>` is 4 hex digits long from 0000-FFFF interval
* `<bus>` is 2 hex digits long from 00-FF interval
* `<device>` is 2 hex digits long from 00-1F interval
* `<function>` is 1 hex digit long from 0-7 interval

Errors that can be returned by `GetVfBdf` function:

* `*StatusError`

Example:

```go
vf, err := amdsmi.GetVfHandleFromBdf(bdf)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
vfBdf, _ := amdsmi.GetVfBdf(vf)
fmt.Printf("VF BDF: %s\n", vfBdf)
```

### GetVfUuid

Description: Returns the UUID of the device

Input parameters:

* `vh` (`VfHandle`) VF handle for which to query

Output: UUID `string`, `error`

Errors that can be returned by `GetVfUuid` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    vf, err := amdsmi.GetVfHandleFromVfIndex(processors[0], 0)
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    uuid, _ := amdsmi.GetVfUuid(vf)
    fmt.Printf("VF UUID: %s\n", uuid)
}
```

### GetNicProcessorHandles

Description: Returns all discovered NIC device handle objects on current machine

Input parameters: `None`

Output: `[]ProcessorHandle`, `error`

Errors that can be returned by `GetNicProcessorHandles` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(nics) == 0 {
    fmt.Println("No NICs on machine")
} else {
    for _, nic := range nics {
        bdf, _ := amdsmi.GetNicDeviceBdf(nic)
        fmt.Printf("NIC BDF: %s\n", bdf)
    }
}
```

### GetNicDeviceBdf

Description: Returns BDF of the given processor (NIC).

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query

Output: `Bdf`, `error`

BDF string format: `<domain>:<bus>:<device>.<function>` in hex.
Where:

* `<domain>` is 4 hex digits long from 0000-FFFF interval
* `<bus>` is 2 hex digits long from 00-FF interval
* `<device>` is 2 hex digits long from 00-1F interval
* `<function>` is 1 hex digit long from 0-7 interval

Errors that can be returned by `GetNicDeviceBdf` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    bdf, _ := amdsmi.GetNicDeviceBdf(nic)
    fmt.Printf("NIC BDF: %s\n", bdf)
}
```

### GetLibVersion

Description: Get the build version information for the currently running build of AMDSMI.

Input parameters: `None`

Output: `Version`, `error`

`Version` fields:

Field | Description
---|---
`Major` | Major version number
`Minor` | Minor version number
`Release` | Release version number

Errors that can be returned by `GetLibVersion` function:

* `*StatusError`

Example:

```go
ver, err := amdsmi.GetLibVersion()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
fmt.Printf("Library version: %d.%d.%d\n", ver.Major, ver.Minor, ver.Release)
```

### StatusCodeToString

Description: Get a description of a provided AMDSMI error status

Input parameters:

* `status` The error status for which a description is desired

Output: `string`, `error`

Errors that can be returned by `StatusCodeToString` function:

* `*StatusError`

Example:

```go
statusCode, err := amdsmi.StatusCodeToString(amdsmi.AMDSMI_STATUS_INVAL)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}

fmt.Printf("  StatusCodeToString: %s \n", statusCode)
```

### GetGpuDriverInfo

Description: Returns GPU driver information such as driver version, date, and driver name.

Input parameters:

* `ph` (`ProcessorHandle`) processor handle

Output: `DriverInfo`, `error`

`DriverInfo` fields:

Field | Description
---|---
`DriverVersion` | driver version string
`DriverDate` | driver build/release date string
`DriverName` | driver name string

Errors that can be returned by `GetGpuDriverInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    GPUDrvInfo, err := amdsmi.GetGpuDriverInfo(processors[0])
	if err != nil {
        fmt.Printf("Error: %v\n", err)
		return
	}

	fmt.Printf("  GetGpuDriverInfo:\n")
	fmt.Printf("  	Driver version    : %s\n", GPUDrvInfo.DriverVersion)
	fmt.Printf("  	Driver date       : %s\n", GPUDrvInfo.DriverDate)
	fmt.Printf("  	Driver name       : %s\n", GPUDrvInfo.DriverName)
}
```

### GetGpuDriverModel

Description: Returns driver model information

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query

Output: `DriverModel`, `error`

Errors that can be returned by `GetGpuDriverModel` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    dm, err := amdsmi.GetGpuDriverModel(processors[0])
	if err != nil {
        fmt.Printf("Error: %v\n", err)
		return
	}

	fmt.Printf("  	Driver model      : %s (%d)\n", dm.String(), dm)
}
```

### GetGpuAsicInfo

Description: Returns asic information for the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `AsicInfo`, `error`

`AsicInfo` fields:

Field | Description
---|---
`MarketName` | market name
`VendorID` | vendor id
`VendorName` | vendor name
`SubvendorID` | subsystem vendor id
`DeviceID` | unique id of a GPU
`RevID` | revision id
`AsicSerial` | asic serial
`OamID` | xgmi physical id
`NumComputeUnits` | number of compute units
`TargetGraphicsVersion` | target graphics version (**_Not supported yet, currently hardcoded to -1_**)
`SubsystemID` | subsystem device id
`Flags` | chip flags (**_Not supported yet, currently hardcoded to -1_**)

Errors that can be returned by `GetGpuAsicInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        asicInfo, _ := amdsmi.GetGpuAsicInfo(processor)
        fmt.Println(asicInfo.MarketName)
        fmt.Println(asicInfo.VendorID)
        fmt.Println(asicInfo.VendorName)
        fmt.Println(asicInfo.SubvendorID)
        fmt.Println(asicInfo.DeviceID)
        fmt.Println(asicInfo.SubsystemID)
        fmt.Println(asicInfo.RevID)
        fmt.Println(asicInfo.AsicSerial)
        fmt.Println(asicInfo.OamID)
        fmt.Println(asicInfo.NumComputeUnits)
        fmt.Println(asicInfo.TargetGraphicsVersion)
        fmt.Println(asicInfo.Flags)
    }
}
```

### GetPowerCapInfo

Description: Returns power cap information as currently configured on the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query
* `sensorInd` (uint32) sensor index. Normally, this will be 0. If a processor has more than one sensor, it could be greater than 0. Parameter sensor_ind is unused on @platform{host}. It is an optional parameter and is set to 0 by default.

Output: `PowerCapInfo`, `error`

`PowerCapInfo` fields:

Field | Description
---|---
`PowerCap` | power capability
`DefaultPowerCap` | default power capability
`DpmCap` | dynamic power management capability
`MinPowerCap` | minimum power capability
`MaxPowerCap` | maximum power capability

Errors that can be returned by `GetPowerCapInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        powerInfo, _ := amdsmi.GetPowerCapInfo(processor, 0)
        fmt.Println(powerInfo.PowerCap)
        fmt.Println(powerInfo.DefaultPowerCap)
        fmt.Println(powerInfo.DpmCap)
        fmt.Println(powerInfo.MinPowerCap)
        fmt.Println(powerInfo.MaxPowerCap)
    }
}
```

### GetPcieInfo

Description: Returns static and metric information about PCIe link for the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `PcieInfo`, `error`

`PcieInfo` fields:

Field | Content
---|---
`Static` | <table> <thead><tr><th> Subfield </th><th>Description</th></tr></thead><tbody><tr><td>`MaxPcieWidth`</td><td> Maximum number of PCIe lanes</td></tr><tr><td>`MaxPcieSpeed`</td><td> Maximum PCIe speed in GT/s </td></tr><tr><td>`PcieInterfaceVersion`</td><td> PCIe interface version </td></tr><tr><td>`SlotType`</td><td> Card form factor from `CardFormFactor` type </td></tr><tr><td>`MaxPcieInterfaceVersion`</td><td> Maximum PCIe link generation</td></tr></tbody></table>
`Metric` | <table> <thead><tr><th> Subfield </th><th>Description</th></tr></thead><tbody><tr><td>`PcieWidth`</td><td> Current number of PCIe lanes </td></tr><tr><td>`PcieSpeed`</td><td> Current PCIe speed in MT/s </td></tr><tr><td>`PcieBandwidth`</td><td> Current PCIe bandwidth in Mb/s </td></tr><tr><td>`PcieReplayCount`</td><td> Total number of the replays issued on the PCIe link </td></tr><tr><td>`PcieL0ToRecoveryCount`</td><td> Total number of times the PCIe link transitioned from L0 to the recovery state </td></tr><tr><td>`PcieReplayRollOverCount`</td><td> Total number of replay rollovers issued on the PCIe link </td></tr><tr><td>`PcieNakSentCount`</td><td> Total number of NAKs issued on the PCIe link by the device </td></tr><tr><td>`PcieNakReceivedCount`</td><td> Total number of NAKs issued on the PCIe link by the receiver </td></tr><tr><td>`PcieLcPerfOtherEndRecoveryCount`</td><td> PCIe other end recovery counter </td></tr></tbody></table>

Errors that can be returned by `GetPcieInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        pcieInfo, _ := amdsmi.GetPcieInfo(processor)
        fmt.Println(pcieInfo.Static.MaxPcieWidth)
        fmt.Println(pcieInfo.Static.MaxPcieSpeed)
        fmt.Println(pcieInfo.Static.SlotType)
        fmt.Println(pcieInfo.Static.MaxPcieInterfaceVersion)
        fmt.Println(pcieInfo.Metric.PcieSpeed)
        fmt.Println(pcieInfo.Metric.PcieWidth)
        fmt.Println(pcieInfo.Metric.PcieBandwidth)
        fmt.Println(pcieInfo.Metric.PcieReplayCount)
        fmt.Println(pcieInfo.Metric.PcieL0ToRecoveryCount)
        fmt.Println(pcieInfo.Metric.PcieReplayRollOverCount)
        fmt.Println(pcieInfo.Metric.PcieNakSentCount)
        fmt.Println(pcieInfo.Metric.PcieNakReceivedCount)
        fmt.Println(pcieInfo.Metric.PcieLcPerfOtherEndRecoveryCount)
    }
}
```

### GetGpuPciBandwidth

Description: Returns supported PCIe bandwidth combinations for the device, including transfer rates and matching lane widths.

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query

Output: `GpuPciBandwidth`, `error`

`GpuPciBandwidth` fields:

Field | Description
---|---
`TransferRate` | supported transfer rates (`Frequencies`: deep-sleep flag, count, current index, `Values` slice)
`Lanes` | lane count per supported rate; parallel to `TransferRate.Values` (first `NumSupported` entries are valid)

`Frequencies` fields:

Field | Description
---|---
`HasDeepSleep` | whether deep-sleep frequency is supported
`NumSupported` | number of supported entries
`Current` | index of the current selection
`Values` | supported values (first `NumSupported` entries are valid)

Errors that can be returned by `GetGpuPciBandwidth` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    bw, err := amdsmi.GetGpuPciBandwidth(processors[0])
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    tr := bw.TransferRate
    fmt.Printf("  GetGpuPciBandwidth: deep_sleep=%v num=%d current=%d\n",
        tr.HasDeepSleep, tr.NumSupported, tr.Current)
}
```

### GetGpuVramInfo

Description: Returns the static information for the VRAM info

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `VramInfo`, `error`

`VramInfo` fields:

Field | Description
---|---
`VramType` | VRAM type from `VramType`
`VramVendor` | VRAM vendor name
`VramSize` | VRAM size in MB
`VramBitWidth` | VRAM bit width
`VramMaxBandwidth` | Maximum VRAM bandwidth at current memory clock (GB/s)

`VramType` type:

Field | Description
---|---
`AMDSMI_VRAM_TYPE_UNKNOWN` | Unknown VRAM type
`AMDSMI_VRAM_TYPE_HBM` | HBM VRAM type
`AMDSMI_VRAM_TYPE_HBM2` | HBM2 VRAM type
`AMDSMI_VRAM_TYPE_HBM2E` | HBM2E VRAM type
`AMDSMI_VRAM_TYPE_HBM3` | HBM3 VRAM type
`AMDSMI_VRAM_TYPE_HBM3E` | HBM3E VRAM type
`AMDSMI_VRAM_TYPE_DDR2` | DDR2 VRAM type
`AMDSMI_VRAM_TYPE_DDR3` | DDR3 VRAM type
`AMDSMI_VRAM_TYPE_DDR4` | DDR4 VRAM type
`AMDSMI_VRAM_TYPE_DDR5` | DDR5 VRAM type
`AMDSMI_VRAM_TYPE_GDDR1` | GDDR1 VRAM type
`AMDSMI_VRAM_TYPE_GDDR2` | GDDR2 VRAM type
`AMDSMI_VRAM_TYPE_GDDR3` | GDDR3 VRAM type
`AMDSMI_VRAM_TYPE_GDDR4` | GDDR4 VRAM type
`AMDSMI_VRAM_TYPE_GDDR5` | GDDR5 VRAM type
`AMDSMI_VRAM_TYPE_GDDR6` | GDDR6 VRAM type
`AMDSMI_VRAM_TYPE_GDDR7` | GDDR7 VRAM type
`AMDSMI_VRAM_TYPE_LPDDR4` | LPDDR4 VRAM type
`AMDSMI_VRAM_TYPE_LPDDR5` | LPDDR5 VRAM type

Errors that can be returned by `GetGpuVramInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        vramInfo, _ := amdsmi.GetGpuVramInfo(processor)
        fmt.Println(vramInfo.VramType)
        fmt.Println(vramInfo.VramVendor)
        fmt.Println(vramInfo.VramSize)
    }
}
```

### GetGpuBoardInfo

Description: Returns board related information for the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query

Output: `BoardInfo`, `error`

`BoardInfo` fields:

Field | Description
---|---
`ModelNumber` | board model number
`ProductSerial` | board product serial number
`FruID` | fru (field-replaceable unit) id
`ProductName` | board product name
`ManufacturerName` | board manufacturer name

Errors that can be returned by `GetGpuBoardInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        boardInfo, _ := amdsmi.GetGpuBoardInfo(processor)
        fmt.Println(boardInfo.ModelNumber)
        fmt.Println(boardInfo.ProductSerial)
        fmt.Println(boardInfo.FruID)
        fmt.Println(boardInfo.ManufacturerName)
        fmt.Println(boardInfo.ProductName)
    }
}
```

### GetFbLayout

Description: Returns framebuffer related information for the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device for which to query

Output: `PfFbInfo`, `error`

`PfFbInfo` fields:

Field | Description
---|---
`TotalFbSize` | total framebuffer size in MB
`PfFbReserved` | framebuffer reserved space in MB
`PfFbOffset` | framebuffer offset in MB
`FbAlignment` | framebuffer alignment in MB
`MaxVfFbUsable` | maximum framebuffer size in MB
`MinVfFbUsable` | minimum framebuffer size in MB

Errors that can be returned by `GetFbLayout` function:

* `*StatusError`
Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    FbLayout, err := amdsmi.GetFbLayout(processors[0])
	if err != nil {
        fmt.Printf("Error: %v\n", err)
		return
	}

	fmt.Printf("  GetFbLayout:\n")
	fmt.Printf("  	TotalFbSize     : %d\n", FbLayout.TotalFbSize)
	fmt.Printf("  	PfFbReserved    : %d\n", FbLayout.PfFbReserved)
	fmt.Printf("  	PfFbOffset      : %d\n", FbLayout.PfFbOffset)
	fmt.Printf("  	FbAlignment     : %d\n", FbLayout.FbAlignment)
	fmt.Printf("  	MaxVfFbUsable   : %d\n", FbLayout.MaxVfFbUsable)
	fmt.Printf("  	MinVfFbUsable   : %d\n", FbLayout.MinVfFbUsable)
}
```

### GetFwInfo

Description: Returns the firmware information for the given GPU.

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `FwInfo`, `error`

`FwInfo` fields:

Field | Description
---|---
`NumFwInfo` | number of valid firmware entries
`FwList` | firmware entry array (`FwInfoList`) containing `FwID` and `FwVersion`

Errors that can be returned by `GetFwInfo` function:

* `*StatusError`
Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    FwInfo, err := amdsmi.GetFwInfo(processors[0])
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }

    fmt.Printf("  GetFwInfo:\n")
    fmt.Printf("  	NumFwInfo       : %d\n", FwInfo.NumFwInfo)
    for i := 0; i < int(FwInfo.NumFwInfo); i++ {
        e := FwInfo.FwList[i]
        fmt.Printf("  	[%d] FwID %s  FwVersion %d\n", i, e.FwID.String(), e.FwVersion)
    }
}
```

### GetGpuVbiosInfo

Description: Returns the static information for the VBIOS on the GPU device.

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `VbiosInfo`, `error`

`VbiosInfo` fields:

Field | Description
---|---
`Name` | vbios name
`BuildDate` | vbios build date
`PartNumber` | vbios part number
`Version` | vbios version string
`BootFirmware` | boot firmware string

Errors that can be returned by `GetGpuVbiosInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    GPUVBIOSInfo, err := amdsmi.GetGpuVbiosInfo(processors[0])
	if err != nil {
        fmt.Printf("Error: %v\n", err)
		return
	}

	fmt.Printf("  GetGpuVbiosInfo:\n")
	fmt.Printf("  	Name              : %s\n", GPUVBIOSInfo.Name)
	fmt.Printf("  	Build date        : %s\n", GPUVBIOSInfo.BuildDate)
	fmt.Printf("  	Part number       : %s\n", GPUVBIOSInfo.PartNumber)
	fmt.Printf("  	Version           : %s\n", GPUVBIOSInfo.Version)
	fmt.Printf("  	Boot firmware     : %s\n", GPUVBIOSInfo.BootFirmware)
}
```

### GetFwErrorRecords

Description: Gets firmware error records

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `FwErrorRecord`, `error`

`FwErrorRecord` fields:

Field | Description
---|---
`NumErrRecords` | number of valid error records
`ErrRecords` | firmware load error entry array (`FwLoadErrorRecord`)

`FwLoadErrorRecord` fields:

Field | Description
---|---
`Timestamp` | system time in seconds
`VfIdx` | vf index
`FwID` | firmware id
`Status` | firmware load status (`GuestFwLoadStatus`)

Errors that can be returned by `GetFwErrorRecords` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
	rec, err := amdsmi.GetFwErrorRecords(processors[0])
	if err != nil {
        fmt.Printf("Error: %v\n", err)
		return
	}

	fmt.Printf("  GetFwErrorRecords:\n")
	fmt.Printf("  	NumErrRecords   : %d\n", rec.NumErrRecords)

	n := int(rec.NumErrRecords)
	if n > amdsmi.AMDSMI_MAX_ERR_RECORDS {
		n = amdsmi.AMDSMI_MAX_ERR_RECORDS
	}
	for i := 0; i < n; i++ {
		e := rec.ErrRecords[i]
		fmt.Printf("  	[%d] ts=%d vf_idx=%d fw_id=%d status=%s\n",
			i, e.Timestamp, e.VfIdx, e.FwID, e.Status.String())
	}
}
```

### GetDfcFwTable

Description: Gets dfc firmware table

Input parameters:

* `ph` (`ProcessorHandle`) processor handle

Output: `DfcFw`, `error`

`DfcFw` fields:

Field | Description
---|---
`Header` | Dfc header structure (`DfcFwHeader`)
`Data` | list of `DfcFwData` structures

`DfcFwHeader` fields:

Field | Description
---|---
`DfcFwVersion` | DFC firmware version
`DfcFwTotalEntries` | number of entries in the dfc table
`DfcGartWrGuestMin` | gart wr guest min
`DfcGartWrGuestMax` | gart wr guest max

`DfcFwData` fields:

Field | Description
---|---
`DfcFwType` | DFC firmware type
`VerificationEnabled` | verification enabled
`CustomerOrdinal` | customer ordinal
`WhiteList` | white list
`BlackList` | black list

`DfcFwWhiteList` fields:

Field | Description
---|---
`Oldest` | oldest
`Latest` | latest

Errors that can be returned by `GetDfcFwTable` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    t, err := amdsmi.GetDfcFwTable(processors[0])
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }

    fmt.Printf("  GetDfcFwTable:\n")
    fmt.Printf("  	DfcFwVersion       : %d\n", t.Header.DfcFwVersion)
    fmt.Printf("  	DfcFwTotalEntries  : %d\n", t.Header.DfcFwTotalEntries)
    fmt.Printf("  	DfcGartWrGuestMin  : %d\n", t.Header.DfcGartWrGuestMin)
    fmt.Printf("  	DfcGartWrGuestMax  : %d\n", t.Header.DfcGartWrGuestMax)

    for i := 0; i < int(t.Header.DfcFwTotalEntries); i++ {
        d := t.Data[i]
        fmt.Printf("  	data[%d] type=%d verification_enabled=%d customer_ordinal=%d\n",
            i, d.DfcFwType, d.VerificationEnabled, d.CustomerOrdinal)
        fmt.Printf("  	  white_list=%v\n", d.WhiteList)
        fmt.Printf("  	  black_list=%v\n", d.BlackList)
    }
}
```

### GetGpuActivity

Description: Returns the engine usage for the given GPU.

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `EngineUsage`, `error`

`EngineUsage` fields:

Field | Description
---|---
`GfxActivity`| graphics engine usage/activity percentage (0 - 100)
`UmcActivity` | memory/UMC engine usage/activity percentage (0 - 100)
`MmActivity` | average multimedia engine usages/activities in percentage (0 - 100)

Errors that can be returned by `GetGpuActivity` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        engineActivity, _ := amdsmi.GetGpuActivity(processor)
        fmt.Println(engineActivity.GfxActivity)
        fmt.Println(engineActivity.UmcActivity)
        fmt.Println(engineActivity.MmActivity)
    }
}
```

### GetPowerInfo

Description: Returns the current power, power limit, and voltage for the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `PowerInfo`, `error`

`PowerInfo` fields:

Field | Description
---|---
`SocketPower` | total socket power
`CurrentSocketPower` | current socket power value (not supported on host, always 0)
`AverageSocketPower` | average socket power value (not supported on host, always 0)
`GfxVoltage` | graphics rail voltage
`SocVoltage` | SoC rail voltage
`MemVoltage` | memory rail voltage
`PowerLimit` | configured power limit (not supported on host, always 0)
`UbbPower` | UBB node power in W (MI350X and newer)

Errors that can be returned by `GetPowerInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    PowerInfo, err := amdsmi.GetPowerInfo(processors[0])
	if err != nil {
        fmt.Printf("Error: %v\n", err)
		return
	}

	fmt.Printf("  GetPowerInfo:\n")
	fmt.Printf("  	SocketPower        : %d\n", PowerInfo.SocketPower)
	fmt.Printf("  	CurrentSocketPower : %d\n", PowerInfo.CurrentSocketPower) // not supported on host, always 0
	fmt.Printf("  	AverageSocketPower : %d\n", PowerInfo.AverageSocketPower) // not supported on host, always 0
	fmt.Printf("  	GfxVoltage         : %d\n", PowerInfo.GfxVoltage)
	fmt.Printf("  	SocVoltage         : %d\n", PowerInfo.SocVoltage)
	fmt.Printf("  	MemVoltage         : %d\n", PowerInfo.MemVoltage)
	fmt.Printf("  	PowerLimit         : %d\n", PowerInfo.PowerLimit) // not supported on host, always 0
	fmt.Printf("  	UbbPower           : %d\n", PowerInfo.UbbPower)
}
```

### IsGpuPowerManagementEnabled

Description: Returns whether GPU power management is enabled.

Input parameters:

* `ph` (`ProcessorHandle`) processor handle

Output: `bool`, `error`

Errors that can be returned by `IsGpuPowerManagementEnabled` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    enabled, err := amdsmi.IsGpuPowerManagementEnabled(processors[0])
	if err != nil {
        fmt.Printf("Error: %v\n", err)
		return
	}

	fmt.Printf("  	IsGpuPowerManagementEnabled            : %v\n", enabled)
}
```

### GetClockInfo

Description: Returns the clock measurements for the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query
* `clkType` (`ClkType`) clock domain to query, one of:

Value | Description
---|---
`AMDSMI_CLK_TYPE_SYS` | system clock domain
`AMDSMI_CLK_TYPE_GFX` | gfx clock domain
`AMDSMI_CLK_TYPE_DF` | Data Fabric clock domain (for ASICs running on a separate clock) (**_Not supported yet_**)
`AMDSMI_CLK_TYPE_DCEF` | Display Controller Engine clock domain (**_Not supported yet_**)
`AMDSMI_CLK_TYPE_SOC` | SOC clock domain (**_Not supported yet_**)
`AMDSMI_CLK_TYPE_MEM` | memory clock domain
`AMDSMI_CLK_TYPE_PCIE` | PCIe clock domain (**_Not supported yet_**)
`AMDSMI_CLK_TYPE_VCLK0` | first multimedia engine (VCLK0) clock domain
`AMDSMI_CLK_TYPE_VCLK1` | second multimedia engine (VCLK1) clock domain
`AMDSMI_CLK_TYPE_DCLK0` | DCLK0 clock domain
`AMDSMI_CLK_TYPE_DCLK1` | DCLK1 clock domain

Output: `ClkInfo`, `error`

`ClkInfo` fields:

Field | Description
---|---
`Clk` | current clock value for the given domain
`MinClk` | minimum clock value for the given domain
`MaxClk` | maximum clock value for the given domain
`ClkLocked` | clock locked flag only supported on GFX clock domain
`ClkDeepSleep` | clock deep sleep mode flag

Errors that can be returned by `GetClockInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    fmt.Printf("  GetClockInfo:\n")
	clockTypes := []struct {
		label string
		typ   amdsmi.ClkType
	}{
		{"GFX", amdsmi.AMDSMI_CLK_TYPE_GFX},
		{"MEM", amdsmi.AMDSMI_CLK_TYPE_MEM},
		{"SOC", amdsmi.AMDSMI_CLK_TYPE_SOC},
	}
	for _, ct := range clockTypes {
		info, err := amdsmi.GetClockInfo(processors[0], ct.typ)
		if err != nil {
            fmt.Printf("Error: %v\n", err)
			continue
		}
		fmt.Printf("  	%-3s MHz clk=%d min=%d max=%d locked=%v deep_sleep=%v\n",
			ct.label, info.Clk, info.MinClk, info.MaxClk, info.ClkLocked, info.ClkDeepSleep)
	}
}
```

### GetTempMetric

Description: Returns the current temperature or limit temperature for the given processor

Input parameters:

* `ph` (`ProcessorHandle`) processor handle
* `sensorType` (`TemperatureType`) sensor to query, one of:

Value | Description
---|---
`AMDSMI_TEMPERATURE_TYPE_EDGE` | edge thermal domain
`AMDSMI_TEMPERATURE_TYPE_HOTSPOT` | hotspot/junction thermal domain
`AMDSMI_TEMPERATURE_TYPE_VRAM` | memory/vram thermal domain
`AMDSMI_TEMPERATURE_TYPE_PLX` | PLX thermal domain (**_not supported yet_**, always returns 0)
`AMDSMI_TEMPERATURE_TYPE_HBM_0` | HBM 0 thermal domain (**_not supported yet_**, always returns 0)
`AMDSMI_TEMPERATURE_TYPE_HBM_1` | HBM 1 thermal domain (**_not supported yet_**, always returns 0)
`AMDSMI_TEMPERATURE_TYPE_HBM_2` | HBM 2 thermal domain (**_not supported yet_**, always returns 0)
`AMDSMI_TEMPERATURE_TYPE_HBM_3` | HBM 3 thermal domain (**_not supported yet_**, always returns 0)

* `metric` (`TemperatureMetric`) metric type to query, one of:

Value | Description
---|---
`AMDSMI_TEMP_CURRENT` | current temperature
`AMDSMI_TEMP_MAX` | max temperature (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_MIN` | min temperature (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_MAX_HYST` | max hyst thermal metric (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_MIN_HYST` | min hyst thermal metric (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_CRITICAL` | limit thermal metric
`AMDSMI_TEMP_CRITICAL_HYST` | critical hyst thermal metric (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_EMERGENCY` | emergency temperature (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_EMERGENCY_HYST` | emergency hyst thermal metric (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_CRIT_MIN` | critical min temperature (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_CRIT_MIN_HYST` | critical min hyst thermal metric (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_OFFSET` | offset thermal metric (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_LOWEST` | lowest thermal metric (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_HIGHEST` | highest thermal metric (**_not supported yet_**, returns 0)
`AMDSMI_TEMP_SHUTDOWN` | shutdown thermal metric

Output: `int64`, `error` — temperature value in Celsius

Errors that can be returned by `GetTempMetric` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    fmt.Printf("  GetTempMetric:\n")
	temp, err := amdsmi.GetTempMetric(processors[0], amdsmi.AMDSMI_TEMPERATURE_TYPE_EDGE, amdsmi.AMDSMI_TEMP_CURRENT)
	if err != nil {
        fmt.Printf("Error: %v\n", err)
		return
	}
	fmt.Printf("  	EDGE / CURRENT (C): %d\n", temp)

	temp2, err := amdsmi.GetTempMetric(processors[0], amdsmi.AMDSMI_TEMPERATURE_TYPE_HOTSPOT, amdsmi.AMDSMI_TEMP_CURRENT)
	if err != nil {
        fmt.Printf("Error: %v\n", err)
	} else {
		fmt.Printf("  	HOTSPOT / CURRENT (C): %d\n", temp2)
	}
}
```

### GetGpuMetrics

Description: Gets GPU metric information

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `[]Metric`, `error`

`Metric` fields:

Field | Description
---|---
`Val` | value of the metric
`Unit` | unit of the metric from `MetricUnit` type
`Name` | name of the metric from `MetricName` type
`Category` | category of the metric from `MetricCategory` type
`Flags` | bitmask of `MetricType` bits; call `Flags.Flags()` for a `[]MetricType` slice
`VfMask` | mask of all active VFs + PF that this metric applies to
`ResGroup` | resource group from `MetricResGroup` type
`ResSubgroup` | resource subgroup from `MetricResSubgroup` type
`ResInstance` | resource instance number

`MetricUnit` type:

Constant | Description
---|---
`AMDSMI_METRIC_UNIT_COUNTER` | counter
`AMDSMI_METRIC_UNIT_UINT` | unsigned integer
`AMDSMI_METRIC_UNIT_BOOL` | boolean
`AMDSMI_METRIC_UNIT_MHZ` | megahertz
`AMDSMI_METRIC_UNIT_PERCENT` | percentage
`AMDSMI_METRIC_UNIT_MILLIVOLT` | millivolt
`AMDSMI_METRIC_UNIT_CELSIUS` | celsius
`AMDSMI_METRIC_UNIT_WATT` | watt
`AMDSMI_METRIC_UNIT_JOULE` | joule
`AMDSMI_METRIC_UNIT_GBPS` | gigabyte per second
`AMDSMI_METRIC_UNIT_MBITPS` | megabit per second
`AMDSMI_METRIC_UNIT_PCIE_GEN` | PCIe generation
`AMDSMI_METRIC_UNIT_PCIE_LANES` | PCIe lanes
`AMDSMI_METRIC_UNIT_15_625_MILLIJOULE` | 15.625 millijoule
`AMDSMI_METRIC_UNIT_UNKNOWN` | unknown unit

`MetricName` type:

Constant | Description
---|---
`AMDSMI_METRIC_NAME_METRIC_ACC_COUNTER` | accumulated counter
`AMDSMI_METRIC_NAME_FW_TIMESTAMP` | firmware timestamp
`AMDSMI_METRIC_NAME_CLK_GFX` | gfx clock
`AMDSMI_METRIC_NAME_CLK_SOC` | socket clock
`AMDSMI_METRIC_NAME_CLK_MEM` | memory clock
`AMDSMI_METRIC_NAME_CLK_VCLK` | vclk clock
`AMDSMI_METRIC_NAME_CLK_DCLK` | dclk clock
`AMDSMI_METRIC_NAME_USAGE_GFX` | gfx usage
`AMDSMI_METRIC_NAME_USAGE_MEM` | memory usage
`AMDSMI_METRIC_NAME_USAGE_MM` | mm usage
`AMDSMI_METRIC_NAME_USAGE_VCN` | vcn usage
`AMDSMI_METRIC_NAME_USAGE_JPEG` | jpeg usage
`AMDSMI_METRIC_NAME_VOLT_GFX` | gfx voltage
`AMDSMI_METRIC_NAME_VOLT_SOC` | socket voltage
`AMDSMI_METRIC_NAME_VOLT_MEM` | memory voltage
`AMDSMI_METRIC_NAME_TEMP_HOTSPOT_CURR` | current hotspot temperature
`AMDSMI_METRIC_NAME_TEMP_HOTSPOT_LIMIT` | hotspot temperature limit
`AMDSMI_METRIC_NAME_TEMP_MEM_CURR` | current memory temperature
`AMDSMI_METRIC_NAME_TEMP_MEM_LIMIT` | memory temperature limit
`AMDSMI_METRIC_NAME_TEMP_VR_CURR` | current vr temperature
`AMDSMI_METRIC_NAME_TEMP_SHUTDOWN` | shutdown temperature
`AMDSMI_METRIC_NAME_POWER_CURR` | current power
`AMDSMI_METRIC_NAME_POWER_LIMIT` | power limit
`AMDSMI_METRIC_NAME_ENERGY_SOCKET` | socket energy
`AMDSMI_METRIC_NAME_ENERGY_CCD` | ccd energy
`AMDSMI_METRIC_NAME_ENERGY_XCD` | xcd energy
`AMDSMI_METRIC_NAME_ENERGY_AID` | aid energy
`AMDSMI_METRIC_NAME_ENERGY_MEM` | memory energy
`AMDSMI_METRIC_NAME_THROTTLE_SOCKET_ACTIVE` | active socket throttle
`AMDSMI_METRIC_NAME_THROTTLE_VR_ACTIVE` | active vr throttle
`AMDSMI_METRIC_NAME_THROTTLE_MEM_ACTIVE` | active memory throttle
`AMDSMI_METRIC_NAME_THROTTLE_PROCHOT_ACTIVE` | active prochot throttle
`AMDSMI_METRIC_NAME_THROTTLE_PPT_ACTIVE` | active ppt throttle
`AMDSMI_METRIC_NAME_PCIE_BANDWIDTH` | pcie bandwidth
`AMDSMI_METRIC_NAME_PCIE_L0_TO_RECOVERY_COUNT` | pcie l0 recovery count
`AMDSMI_METRIC_NAME_PCIE_REPLAY_COUNT` | pcie replay count
`AMDSMI_METRIC_NAME_PCIE_REPLAY_ROLLOVER_COUNT` | pcie replay rollover count
`AMDSMI_METRIC_NAME_PCIE_NAK_SENT_COUNT` | pcie nak sent count
`AMDSMI_METRIC_NAME_PCIE_NAK_RECEIVED_COUNT` | pcie nak received count
`AMDSMI_METRIC_NAME_CLK_GFX_MAX_LIMIT` | maximum gfx clock limit
`AMDSMI_METRIC_NAME_CLK_SOC_MAX_LIMIT` | maximum socket clock limit
`AMDSMI_METRIC_NAME_CLK_MEM_MAX_LIMIT` | maximum memory clock limit
`AMDSMI_METRIC_NAME_CLK_VCLK_MAX_LIMIT` | maximum vclk clock limit
`AMDSMI_METRIC_NAME_CLK_DCLK_MAX_LIMIT` | maximum dclk clock limit
`AMDSMI_METRIC_NAME_CLK_GFX_MIN_LIMIT` | minimum gfx clock limit
`AMDSMI_METRIC_NAME_CLK_SOC_MIN_LIMIT` | minimum socket clock limit
`AMDSMI_METRIC_NAME_CLK_MEM_MIN_LIMIT` | minimum memory clock limit
`AMDSMI_METRIC_NAME_CLK_VCLK_MIN_LIMIT` | minimum vclk clock limit
`AMDSMI_METRIC_NAME_CLK_DCLK_MIN_LIMIT` | minimum dclk clock limit
`AMDSMI_METRIC_NAME_CLK_GFX_LOCKED` | gfx clock locked
`AMDSMI_METRIC_NAME_CLK_GFX_DS_DISABLED` | gfx deep sleep
`AMDSMI_METRIC_NAME_CLK_MEM_DS_DISABLED` | memory deep sleep
`AMDSMI_METRIC_NAME_CLK_SOC_DS_DISABLED` | socket deep sleep
`AMDSMI_METRIC_NAME_CLK_VCLK_DS_DISABLED` | vclk deep sleep
`AMDSMI_METRIC_NAME_CLK_DCLK_DS_DISABLED` | dclk deep sleep
`AMDSMI_METRIC_NAME_PCIE_LINK_SPEED` | pcie link speed
`AMDSMI_METRIC_NAME_PCIE_LINK_WIDTH` | pcie link width
`AMDSMI_METRIC_NAME_DRAM_BANDWIDTH` | dram bandwidth
`AMDSMI_METRIC_NAME_MAX_DRAM_BANDWIDTH` | maximum dram bandwidth
`AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_PPT` | gfx clock below host limit ppt
`AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_THM` | gfx clock below host limit thermal
`AMDSMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_TOTAL` | gfx clock below host limit total
`AMDSMI_METRIC_NAME_GFX_CLK_LOW_UTILIZATION` | gfx clock low utilization
`AMDSMI_METRIC_NAME_INPUT_TELEMETRY_VOLTAGE` | input telemetry voltage
`AMDSMI_METRIC_NAME_PLDM_VERSION` | pldm version
`AMDSMI_METRIC_NAME_TEMP_XCD` | xcd temperature
`AMDSMI_METRIC_NAME_TEMP_AID` | aid temperature
`AMDSMI_METRIC_NAME_TEMP_HBM` | hbm temperature
`AMDSMI_METRIC_NAME_SYS_METRIC_ACC_COUNTER` | system metric accumulated counter
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA` | system temperature ubb fpga
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FRONT` | system temperature ubb front
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_BACK` | system temperature ubb back
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM7` | system temperature ubb oam7
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_IBC` | system temperature ubb ibc
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_UFPGA` | system temperature ubb ufpga
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM1` | system temperature ubb oam1
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_HSC` | system temperature oam 0 1 hsc
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_2_3_HSC` | system temperature oam 2 3 hsc
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_HSC` | system temperature oam 4 5 hsc
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_6_7_HSC` | system temperature oam 6 7 hsc
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_0V72_VR` | system temperature ubb fpga 0v72 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_3V3_VR` | system temperature ubb fpga 3v3 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR` | system temperature retimer 0 1 2 3 1v2 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR` | system temperature retimer 4 5 6 7 1v2 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_0V9_VR` | system temperature retimer 0 1 0v9 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_0V9_VR` | system temperature retimer 4 5 0v9 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_2_3_0V9_VR` | system temperature retimer 2 3 0v9 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_6_7_0V9_VR` | system temperature retimer 6 7 0v9 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR` | system temperature oam 0 1 2 3 3v3 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR` | system temperature oam 4 5 6 7 3v3 vr
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC_HSC` | system temperature ibc hsc
`AMDSMI_METRIC_NAME_SYSTEM_TEMP_IBC` | system temperature ibc
`AMDSMI_METRIC_NAME_NODE_TEMP_RETIMER` | node temperature retimer
`AMDSMI_METRIC_NAME_NODE_TEMP_IBC_TEMP` | node temperature ibc temp
`AMDSMI_METRIC_NAME_NODE_TEMP_IBC_2_TEMP` | node temperature ibc 2 temp
`AMDSMI_METRIC_NAME_NODE_TEMP_VDD18_VR_TEMP` | node temperature vdd18 vr temp
`AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_B_VR_TEMP` | node temperature 04 hbm b vr temp
`AMDSMI_METRIC_NAME_NODE_TEMP_04_HBM_D_VR_TEMP` | node temperature 04 hbm d vr temp
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD0` | vr temperature vddcr vdd0
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD1` | vr temperature vddcr vdd1
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD2` | vr temperature vddcr vdd2
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_VDD3` | vr temperature vddcr vdd3
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_A` | vr temperature vddcr soc a
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_C` | vr temperature vddcr soc c
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_A` | vr temperature vddcr socio a
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_C` | vr temperature vddcr socio c
`AMDSMI_METRIC_NAME_VR_TEMP_VDD_085_HBM` | vr temperature vdd 085 hbm
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_B` | vr temperature vddcr 11 hbm b
`AMDSMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_D` | vr temperature vddcr 11 hbm d
`AMDSMI_METRIC_NAME_VR_TEMP_VDD_USR` | vr temperature vdd usr
`AMDSMI_METRIC_NAME_VR_TEMP_VDDIO_11_E32` | vr temperature vddio 11 e32
`AMDSMI_METRIC_NAME_SYSTEM_POWER_UBB_POWER` | system power ubb power
`AMDSMI_METRIC_NAME_SYSTEM_POWER_UBB_POWER_THRESHOLD` | system power ubb power threshold
`AMDSMI_METRIC_NAME_UNKNOWN` | unknown name

`MetricCategory` type:

Constant | Description
---|---
`AMDSMI_METRIC_CATEGORY_ACC_COUNTER` | counter
`AMDSMI_METRIC_CATEGORY_FREQUENCY` | frequency
`AMDSMI_METRIC_CATEGORY_ACTIVITY` | activity
`AMDSMI_METRIC_CATEGORY_TEMPERATURE` | temperature
`AMDSMI_METRIC_CATEGORY_POWER` | power
`AMDSMI_METRIC_CATEGORY_ENERGY` | energy
`AMDSMI_METRIC_CATEGORY_THROTTLE` | throttle
`AMDSMI_METRIC_CATEGORY_PCIE` | pcie
`AMDSMI_METRIC_CATEGORY_STATIC` | static
`AMDSMI_METRIC_CATEGORY_SYS_ACC_COUNTER` | system accumulated counter
`AMDSMI_METRIC_CATEGORY_SYS_BASEBOARD_TEMP` | system baseboard temperature
`AMDSMI_METRIC_CATEGORY_SYS_GPUBOARD_TEMP` | system gpu board temperature
`AMDSMI_METRIC_CATEGORY_SYS_BASEBOARD_POWER` | system baseboard power
`AMDSMI_METRIC_CATEGORY_UNKNOWN` | unknown category

`MetricType` bitmask (`Flags` field):

Constant | Value | Description
---|---|---
`AMDSMI_METRIC_TYPE_COUNTER` | `1<<0` | counter
`AMDSMI_METRIC_TYPE_CHIPLET` | `1<<1` | chiplet
`AMDSMI_METRIC_TYPE_INST` | `1<<2` | instantaneous data
`AMDSMI_METRIC_TYPE_ACC` | `1<<3` | accumulated data

`MetricResGroup` type:

Constant | Description
---|---
`AMDSMI_METRIC_RES_GROUP_UNKNOWN` | unknown resource group
`AMDSMI_METRIC_RES_GROUP_NA` | resource group is not applicable
`AMDSMI_METRIC_RES_GROUP_GPU` | gpu resource group
`AMDSMI_METRIC_RES_GROUP_XCP` | xcp resource group
`AMDSMI_METRIC_RES_GROUP_AID` | aid resource group
`AMDSMI_METRIC_RES_GROUP_MID` | mid resource group
`AMDSMI_METRIC_RES_GROUP_SYSTEM` | system resource group

`MetricResSubgroup` type:

Constant | Description
---|---
`AMDSMI_METRIC_RES_SUBGROUP_UNKNOWN` | unknown resource subgroup
`AMDSMI_METRIC_RES_SUBGROUP_NA` | resource subgroup is not applicable
`AMDSMI_METRIC_RES_SUBGROUP_XCC` | xcc resource subgroup
`AMDSMI_METRIC_RES_SUBGROUP_ENGINE` | engine resource subgroup
`AMDSMI_METRIC_RES_SUBGROUP_HBM` | hbm resource subgroup
`AMDSMI_METRIC_RES_SUBGROUP_BASEBOARD` | baseboard resource subgroup
`AMDSMI_METRIC_RES_SUBGROUP_GPUBOARD` | gpuboard resource subgroup

Errors that can be returned by `GetGpuMetrics` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        metrics, err := amdsmi.GetGpuMetrics(processor)
        if err != nil {
            fmt.Printf("GetGpuMetrics failed: %v\n", err)
            continue
        }
        fmt.Printf("Metrics: %d\n", len(metrics))
        for i, m := range metrics {
            fmt.Printf("  [%d] %s = %d %s (category=%s flags=%v res=%s/%s/%d vf_mask=0x%X)\n",
                i, m.Name, m.Val, m.Unit, m.Category,
                m.Flags.Flags(),
                m.ResGroup, m.ResSubgroup, m.ResInstance,
                m.VfMask)
        }
    }
}
```

### GetGpuMemoryPartitionConfig

Description: Returns current gpu memory partition config and mode capabilities

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `MemoryPartitionConfig`, `error`

`MemoryPartitionConfig` fields:

Field | Description
---|---
`PartitionCaps` | memory partition capabilities (`NpsCaps`)
`Mode` | memory partition mode from `MemoryPartitionType` type
`NumNumaRanges` | number of NUMA ranges
`NumaRanges` | <table> <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`MemoryType`</td><td> memory type from `VramType` </td></tr><tr><td>`Start`</td><td> start of NUMA range </td></tr><tr><td>`End`</td><td> end of NUMA range </td></tr></tbody></table>

`NpsCaps` fields:

Field | Description
---|---
`Nps1Cap` | NPS1 supported
`Nps2Cap` | NPS2 supported
`Nps4Cap` | NPS4 supported
`Nps8Cap` | NPS8 supported

`NpsCaps` also provides `Supported()` returning `[]MemoryPartitionType` and `String()`.

`MemoryPartitionType` type:

Field | Description
---|---
`AMDSMI_MEMORY_PARTITION_UNKNOWN` | unknown memory partition
`AMDSMI_MEMORY_PARTITION_NPS1` | memory partition with 1 number per socket
`AMDSMI_MEMORY_PARTITION_NPS2` | memory partition with 2 numbers per socket
`AMDSMI_MEMORY_PARTITION_NPS4` | memory partition with 4 numbers per socket
`AMDSMI_MEMORY_PARTITION_NPS8` | memory partition with 8 numbers per socket

Errors that can be returned by `GetGpuMemoryPartitionConfig` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        config, _ := amdsmi.GetGpuMemoryPartitionConfig(processor)
        fmt.Printf("NPS Mode: %s\n", config.Mode)
        fmt.Printf("Capabilities: %s\n", config.PartitionCaps)
        fmt.Printf("NUMA Ranges: %d\n", config.NumNumaRanges)
    }
}
```

### SetGpuMemoryPartitionMode

Description: Sets memory partition mode

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device
* `mode` (`MemoryPartitionType`) Desired NPS mode from `MemoryPartitionType` type

`MemoryPartitionType` type:

Field | Description
---|---
`AMDSMI_MEMORY_PARTITION_UNKNOWN` | unknown memory partition
`AMDSMI_MEMORY_PARTITION_NPS1` | memory partition with 1 number per socket
`AMDSMI_MEMORY_PARTITION_NPS2` | memory partition with 2 numbers per socket
`AMDSMI_MEMORY_PARTITION_NPS4` | memory partition with 4 numbers per socket
`AMDSMI_MEMORY_PARTITION_NPS8` | memory partition with 8 numbers per socket

Output: `error`

Errors that can be returned by `SetGpuMemoryPartitionMode` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        err := amdsmi.SetGpuMemoryPartitionMode(processor, amdsmi.AMDSMI_MEMORY_PARTITION_NPS1)
        if err != nil {
            fmt.Printf("SetGpuMemoryPartitionMode failed: %v\n", err)
        }
    }
}
```

### GetGpuAcceleratorPartitionProfileConfig

Description: Returns gpu accelerator partition caps as currently configured in the system

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `AcceleratorPartitionProfileConfig`, `error`

`AcceleratorPartitionProfileConfig` fields:

Field | Description
---|---
`ResourceProfiles` | <table> <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`ProfileIndex`</td><td> index of a profile </td></tr><tr><td>`ResourceType`</td><td> type of a resource from `AcceleratorPartitionResourceType` type </td></tr><tr><td>`PartitionResource`</td><td> the resources a partition can use, which may be shared </td></tr><tr><td>`NumPartitionsShareResource`</td><td> number of partitions that share resource of that type </td></tr></tbody></table>
`DefaultProfileIndex` | index of the default profile
`Profiles` | <table> <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`ProfileType`</td><td> profile type from `AcceleratorPartitionType` </td></tr><tr><td>`NumPartitions`</td><td> number of partitions in the profile </td></tr><tr><td>`MemoryCaps`</td><td> memory capabilities of the profile </td></tr><tr><td>`ProfileIndex`</td><td> index of the profile </td></tr><tr><td>`Resources`</td><td> resources in the profile </td></tr></tbody></table>

`AcceleratorPartitionResourceType` type:

Field | Description
---|---
`AMDSMI_ACCELERATOR_XCC` | XCC resource capabilities
`AMDSMI_ACCELERATOR_ENCODER` | encoder resource capabilities
`AMDSMI_ACCELERATOR_DECODER` | decoder resource capabilities
`AMDSMI_ACCELERATOR_DMA` | DMA resource capabilities
`AMDSMI_ACCELERATOR_JPEG` | JPEG resource capabilities

`AcceleratorPartitionType` type:

Field | Description
---|---
`AMDSMI_ACCELERATOR_PARTITION_INVALID` | invalid compute partition
`AMDSMI_ACCELERATOR_PARTITION_SPX` | compute partition with all XCCs in group (8/1)
`AMDSMI_ACCELERATOR_PARTITION_DPX` | compute partition with four XCCs in group (8/2)
`AMDSMI_ACCELERATOR_PARTITION_TPX` | compute partition with two XCCs in group (6/3)
`AMDSMI_ACCELERATOR_PARTITION_QPX` | compute partition with two XCCs in group (8/4)
`AMDSMI_ACCELERATOR_PARTITION_CPX` | compute partition with one XCC in group (8/8)

Errors that can be returned by `GetGpuAcceleratorPartitionProfileConfig` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        config, _ := amdsmi.GetGpuAcceleratorPartitionProfileConfig(processor)
        fmt.Printf("Profiles: %d, Default: %d\n",
            config.NumProfiles, config.DefaultProfileIndex)
    }
}
```

### GetGpuAcceleratorPartitionProfile

Description: Returns current gpu accelerator partition cap

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `AcceleratorPartitionProfile`, `[]uint32` (partition IDs), `error`

`AcceleratorPartitionProfile` fields:

Field | Description
---|---
`ProfileType` | current profile type from `AcceleratorPartitionType`
`NumPartitions` | number of partitions in the profile
`MemoryCaps` | memory capabilities of the profile
`ProfileIndex` | index of the profile
`Resources` | resources in the profile

Errors that can be returned by `GetGpuAcceleratorPartitionProfile` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        profile, partIDs, _ := amdsmi.GetGpuAcceleratorPartitionProfile(processor)
        fmt.Printf("Active: %s, Partitions: %d, IDs: %v\n",
            profile.ProfileType, profile.NumPartitions, partIDs)
    }
}
```

### SetGpuAcceleratorPartitionProfile

Description: Sets accelerator partition setting based on profile_index from `GetGpuAcceleratorPartitionProfileConfig`

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device
* `profileIndex` (uint32) Represents index of a partition user wants to set

Output: `error`

Errors that can be returned by `SetGpuAcceleratorPartitionProfile` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        err := amdsmi.SetGpuAcceleratorPartitionProfile(processor, 1)
        if err != nil {
            fmt.Printf("SetGpuAcceleratorPartitionProfile failed: %v\n", err)
        }
    }
}
```

### GetGpuAcceleratorPartitionProfileConfigGlobal

Description: Returns all GPU accelerator partition capabilities which can be configured on the system

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `AcceleratorPartitionProfileConfigGlobal`, `error`

`AcceleratorPartitionProfileConfigGlobal` fields:

Field | Description
---|---
`Profiles` | Array of `AcceleratorPartitionProfileGlobal`, each describing a supported accelerator partition profile with VF mode support. Each entry contains:<br><table> <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`Profile.ProfileType`</td><td>Profile type from `AcceleratorPartitionType`</td></tr><tr><td>`Profile.NumPartitions`</td><td>Number of partitions in the profile</td></tr><tr><td>`Profile.MemoryCaps`</td><td>Memory capabilities of the profile</td></tr><tr><td>`Profile.ProfileIndex`</td><td>Index of the profile</td></tr><tr><td>`VfMode`</td><td>Slice of supported VF modes from `VfMode` type</td></tr><tr><td>`Profile.Resources`</td><td>Resources in the profile</td></tr></tbody></table>
`DefaultProfileIndex` | Index of the default profile used if no custom configuration is set

`VfMode` type:

Field | Description
---|---
`AMDSMI_VF_MODE_1` | 1 VF mode
`AMDSMI_VF_MODE_2` | 2 VF mode
`AMDSMI_VF_MODE_4` | 4 VF mode
`AMDSMI_VF_MODE_8` | 8 VF mode
`AMDSMI_VF_MODE_ALL` | All VF modes

Errors that can be returned by `GetGpuAcceleratorPartitionProfileConfigGlobal` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        config, _ := amdsmi.GetGpuAcceleratorPartitionProfileConfigGlobal(processor)
        for i := uint32(0); i < config.NumProfiles; i++ {
            p := config.Profiles[i]
            fmt.Printf("Profile %d: %s, VF Modes: %v\n",
                i, p.Profile.ProfileType, p.VfMode)
        }
    }
}
```

### GetLinkMetrics

Description: Gets link metric information

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `LinkMetrics`, `error`

`LinkMetrics` fields:

Field | Description
---|---
`NumLinks` | Number of active links
`Links` | Slice of `LinkMetricInfo`

`LinkMetricInfo` fields:

Field | Description
---|---
`Bdf` | BDF of the given processor
`BitRate` | current link speed in Gb/s
`MaxBandwidth` | max bandwidth of the link
`LinkType` | type of the link (`LinkType`)
`Read` | total data received for each link in KB
`Write` | total data transferred for each link in KB
`LinkStatus` | HW status of the link (`LinkStatus`)

`LinkType` values:

Field | Description
---|---
`AMDSMI_LINK_TYPE_INTERNAL` | Internal link
`AMDSMI_LINK_TYPE_PCIE` | PCIe link type
`AMDSMI_LINK_TYPE_XGMI` | XGMI link type
`AMDSMI_LINK_TYPE_NOT_APPLICABLE` | Link not applicable
`AMDSMI_LINK_TYPE_UNKNOWN` | Unknown

`LinkStatus` values:

Field | Description
---|---
`AMDSMI_LINK_STATUS_ENABLED` | Enabled
`AMDSMI_LINK_STATUS_DISABLED` | Disabled
`AMDSMI_LINK_STATUS_INACTIVE` | Inactive
`AMDSMI_LINK_STATUS_ERROR` | Error

Errors that can be returned by `GetLinkMetrics` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        metrics, _ := amdsmi.GetLinkMetrics(processor)
        fmt.Printf("Links: %d\n", metrics.NumLinks)
        for i, l := range metrics.Links {
            fmt.Printf("  [%d] bdf=%s bit_rate=%d max_bw=%d type=%d read=%d write=%d status=%d\n",
                i, l.Bdf, l.BitRate, l.MaxBandwidth, l.LinkType, l.Read, l.Write, l.LinkStatus)
        }
    }
}
```

### GetLinkTopologyNearest

Description: Retrieve the set of GPUs that are nearest to a given device
             at a specific interconnectivity level.

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query
* `linkType` (`LinkType`) The link type level to search for nearest devices

`LinkType` type:

Field | Description
---|---
`AMDSMI_LINK_TYPE_INTERNAL` | Internal link
`AMDSMI_LINK_TYPE_PCIE` | PCIe link type
`AMDSMI_LINK_TYPE_XGMI` | XGMI link type
`AMDSMI_LINK_TYPE_NOT_APPLICABLE` | Link not applicable
`AMDSMI_LINK_TYPE_UNKNOWN` | Unknown

Output: `TopologyNearest`, `error`

`TopologyNearest` fields:

Field | Description
---|---
`Count` | Number of nearest GPUs found
`ProcessorList` | Slice of `ProcessorHandle` for GPUs found at the given link level

Errors that can be returned by `GetLinkTopologyNearest` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        nearest, _ := amdsmi.GetLinkTopologyNearest(processor, amdsmi.AMDSMI_LINK_TYPE_XGMI)
        fmt.Printf("Nearest GPUs (XGMI): %d found\n", nearest.Count)
        for i, p := range nearest.ProcessorList {
            bdf, _ := amdsmi.GetGpuDeviceBdf(p)
            fmt.Printf("  [%d] BDF=%s\n", i, bdf)
        }
    }
}
```

### TopoGetP2pStatus

Description: Retrieve connection type and P2P capabilities between two GPUs

Input parameters:

* `phSrc` (`ProcessorHandle`) the source processor handle
* `phDst` (`ProcessorHandle`) the destination processor handle

Output: `LinkType`, `P2pCapability`, `error`

`LinkType` type:

Field | Description
---|---
`AMDSMI_LINK_TYPE_INTERNAL` | Internal link
`AMDSMI_LINK_TYPE_PCIE` | PCIe link type
`AMDSMI_LINK_TYPE_XGMI` | XGMI link type
`AMDSMI_LINK_TYPE_NOT_APPLICABLE` | Link not applicable
`AMDSMI_LINK_TYPE_UNKNOWN` | Unknown
`AMDSMI_LINK_TYPE_NUMA` | Same NUMA node, different PCIe switch (NIC-to-GPU only)
`AMDSMI_LINK_TYPE_XNUMA` | Different NUMA nodes (NIC-to-GPU only)

`P2pCapability` fields:

Field | Description
---|---
`IsIolinkCoherent` | 1 = true, 0 = false, 255 = not defined
`IsIolinkAtomics32bit` | 1 = true, 0 = false, 255 = not defined
`IsIolinkAtomics64bit` | 1 = true, 0 = false, 255 = not defined
`IsIolinkDma` | 1 = true, 0 = false, 255 = not defined
`IsIolinkBiDirectional` | 1 = true, 0 = false, 255 = not defined

Errors that can be returned by `TopoGetP2pStatus` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) < 2 {
    fmt.Println("Need at least 2 GPUs for P2P status")
} else {
    linkType, cap, _ := amdsmi.TopoGetP2pStatus(processors[0], processors[1])
    fmt.Printf("Link type: %d\n", linkType)
    fmt.Printf("Coherent: %d, Atomics32: %d, Atomics64: %d, DMA: %d, BiDir: %d\n",
        cap.IsIolinkCoherent, cap.IsIolinkAtomics32bit,
        cap.IsIolinkAtomics64bit, cap.IsIolinkDma, cap.IsIolinkBiDirectional)
}
```

### TopoGetNumaNodeNumber

Description: Get the NUMA node associated with a device

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query

Output: `uint32` (NUMA node value), `error`

Errors that can be returned by `TopoGetNumaNodeNumber` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        numaNode, _ := amdsmi.TopoGetNumaNodeNumber(processor)
        fmt.Printf("NUMA Node: %d\n", numaNode)
    }
}
```

### TopoGetLinkType

Description: Get the hop count and link type between two processors. Supports both GPU-to-GPU and NIC-to-GPU queries.

Input parameters:

* `phSrc` (`ProcessorHandle`) source processor handle (GPU or NIC)
* `phDst` (`ProcessorHandle`) destination processor handle (GPU)

Output: `hops` (`uint64`), `LinkType`, `error`

For GPU-to-GPU queries `LinkType` is one of `INTERNAL`, `PCIE`, `XGMI`, `NOT_APPLICABLE`, or `UNKNOWN`, and `hops` carries the hop count.

For NIC-to-GPU queries `LinkType` is one of `PCIE`, `NUMA`, `XNUMA`, or `UNKNOWN`. The hop count is not meaningful in this case; `hops` is set to `^uint64(0)` (UINT64_MAX).

`LinkType` type:

Field | Description
---|---
`AMDSMI_LINK_TYPE_INTERNAL` | Internal link (GPU-to-GPU)
`AMDSMI_LINK_TYPE_PCIE` | PCIe link (GPU-to-GPU or NIC-to-GPU via same PCIe switch)
`AMDSMI_LINK_TYPE_XGMI` | XGMI link (GPU-to-GPU)
`AMDSMI_LINK_TYPE_NOT_APPLICABLE` | Not applicable (GPU-to-GPU)
`AMDSMI_LINK_TYPE_UNKNOWN` | Unknown
`AMDSMI_LINK_TYPE_NUMA` | Same NUMA node, different PCIe switch (NIC-to-GPU only)
`AMDSMI_LINK_TYPE_XNUMA` | Different NUMA nodes (NIC-to-GPU only)

Errors that can be returned by `TopoGetLinkType` function:

* `*StatusError`

Example:

```go
// GPU-to-GPU
gpus, _ := amdsmi.GetProcessorHandles()
if len(gpus) >= 2 {
    hops, linkType, _ := amdsmi.TopoGetLinkType(gpus[0], gpus[1])
    fmt.Printf("Link type: %s, hops: %d\n", linkType, hops)
}

// NIC-to-GPU
nics, _ := amdsmi.GetNicProcessorHandles()
if len(nics) > 0 && len(gpus) > 0 {
    hops, linkType, _ := amdsmi.TopoGetLinkType(nics[0], gpus[0])
    if hops == ^uint64(0) {
        fmt.Printf("NIC<->GPU link type: %s (hops N/A)\n", linkType)
    }
}
```

### GetLinkTopology

Description: Gets link topology information between two connected processors

Input parameters:

* `phSrc` (`ProcessorHandle`) PF of a source GPU device
* `phDst` (`ProcessorHandle`) PF of a destination GPU device

Output: `LinkTopology`, `error`

`LinkTopology` fields:

Field | Description
---|---
`Weight` | link weight between two GPUs
`LinkStatus` | HW status of the link from `LinkStatus` type
`LinkType` | type of the link from `LinkType`
`NumHops` | number of hops between two GPUs
`FbSharing` | framebuffer sharing between two GPUs

`LinkType` type:

Field | Description
---|---
`AMDSMI_LINK_TYPE_INTERNAL` | Internal link
`AMDSMI_LINK_TYPE_PCIE` | PCIe link type
`AMDSMI_LINK_TYPE_XGMI` | XGMI link type
`AMDSMI_LINK_TYPE_NOT_APPLICABLE` | Link not applicable
`AMDSMI_LINK_TYPE_UNKNOWN` | Unknown
`AMDSMI_LINK_TYPE_NUMA` | Same NUMA node, different PCIe switch (NIC-to-GPU only)
`AMDSMI_LINK_TYPE_XNUMA` | Different NUMA nodes (NIC-to-GPU only)

`LinkStatus` type:

Field | Description
---|---
`AMDSMI_LINK_STATUS_ENABLED` | Enabled
`AMDSMI_LINK_STATUS_DISABLED` | Disabled
`AMDSMI_LINK_STATUS_INACTIVE` | Inactive
`AMDSMI_LINK_STATUS_ERROR` | Error

Errors that can be returned by `GetLinkTopology` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, srcProcessor := range processors {
        for _, dstProcessor := range processors {
            topo, _ := amdsmi.GetLinkTopology(srcProcessor, dstProcessor)
            fmt.Printf("Link: type=%d, status=%d, weight=%d, hops=%d\n",
                topo.LinkType, topo.LinkStatus, topo.Weight, topo.NumHops)
        }
    }
}
```

### GetXgmiFbSharingCaps

Description: Returns XGMI framebuffer sharing capabilities for the given processor

Input parameters:

* `ph` (`ProcessorHandle`) GPU device for which to query

Output: `XgmiFbSharingCaps`, `error`

`XgmiFbSharingCaps` fields:

Field | Description
---|---
`ModeCustomCap` | Custom sharing mode capability (1 = supported, 0 = not supported)
`Mode1Cap` | Mode 1 sharing capability (1 = supported, 0 = not supported)
`Mode2Cap` | Mode 2 sharing capability (1 = supported, 0 = not supported)
`Mode4Cap` | Mode 4 sharing capability (1 = supported, 0 = not supported)
`Mode8Cap` | Mode 8 sharing capability (1 = supported, 0 = not supported)

Errors that can be returned by `GetXgmiFbSharingCaps` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        caps, _ := amdsmi.GetXgmiFbSharingCaps(processor)
        fmt.Printf("Custom: %d, Mode1: %d, Mode2: %d, Mode4: %d, Mode8: %d\n",
            caps.ModeCustomCap, caps.Mode1Cap, caps.Mode2Cap, caps.Mode4Cap, caps.Mode8Cap)
    }
}
```

### GetXgmiFbSharingModeInfo

Description: Returns XGMI framebuffer sharing information between two GPUs for a specified sharing mode

Input parameters:

* `phSrc` (`ProcessorHandle`) Source PF of a processor for which to query
* `phDst` (`ProcessorHandle`) Destination PF of a processor for which to query
* `mode` (`XgmiFbSharingMode`) Framebuffer sharing mode to query

`XgmiFbSharingMode` values:

Field | Description
---|---
`AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM` | Custom sharing mode
`AMDSMI_XGMI_FB_SHARING_MODE_1` | Mode 1 sharing
`AMDSMI_XGMI_FB_SHARING_MODE_2` | Mode 2 sharing
`AMDSMI_XGMI_FB_SHARING_MODE_4` | Mode 4 sharing
`AMDSMI_XGMI_FB_SHARING_MODE_8` | Mode 8 sharing
`AMDSMI_XGMI_FB_SHARING_MODE_UNKNOWN` | Unknown sharing mode

Output: `uint8` (indicates whether framebuffer is shared between the two processors), `error`

Errors that can be returned by `GetXgmiFbSharingModeInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) < 2 {
    fmt.Println("Need at least 2 GPUs for FB sharing mode info")
} else {
    fbSharing, _ := amdsmi.GetXgmiFbSharingModeInfo(
        processors[0], processors[1], amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_1)
    fmt.Printf("FB sharing (Mode 1): %d\n", fbSharing)
}
```

### SetXgmiFbSharingMode

Description: Sets framebuffer sharing mode

Note: This API will only work if there's no guest VM running.
      If all processors in the list are not located within the same NUMA node,
      the API must be called separately for each NUMA node, as the set operation only applies to processors within a single NUMA node.
      To determine how many times the set operation needs to be called, it is essential to first invoke the set API for the
      first processor in the list.
      After this, you should retrieve the configuration for the selected mode using amdsmi_get_xgmi_fb_sharing_mode_info and verify
      the settings with amdsmi_get_link_topology.
      We need to compare the is_fb_sharing_enabled status for each GPU pair to gather the necessary information,
      alongside topology_info.fb_sharing to verify the link topology between them.
      If these two values differ, it indicates that the processors are in different NUMA nodes,
      suggesting that the initial set operation did not complete the intended configuration.
      In this case, the set API should be invoked again for each NUMA node to ensure that the proper settings are applied.

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device
* `mode` (`XgmiFbSharingMode`) framebuffer sharing mode to set

`XgmiFbSharingMode` values:

Field | Description
---|---
`AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM` | custom framebuffer sharing mode
`AMDSMI_XGMI_FB_SHARING_MODE_1` | framebuffer sharing mode_1
`AMDSMI_XGMI_FB_SHARING_MODE_2` | framebuffer sharing mode_2
`AMDSMI_XGMI_FB_SHARING_MODE_4` | framebuffer sharing mode_4
`AMDSMI_XGMI_FB_SHARING_MODE_8` | framebuffer sharing mode_8

Output: `error`

Errors that can be returned by `SetXgmiFbSharingMode` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        err := amdsmi.SetXgmiFbSharingMode(processor, amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_4)
        if err != nil {
            fmt.Printf("SetXgmiFbSharingMode failed: %v\n", err)
        }
    }
}
```

### SetXgmiFbSharingModeV2

Description: Sets framebuffer sharing mode

Note: This API will only work if there's no guest VM running.
      This api can be used for custom and auto setting of xgmi frame buffer sharing.
         In case of custom mode:
           - All processors in the list must be on the same NUMA node.
             Otherwise, api will return error.
           - If any processor from the list already belongs to an existing group,
             the existing group will be released automatically.
         In case of auto mode(MODE_X):
           - The input parameter processor_list[0] should be valid.
             Only the first element of processor_list is taken into account and it can be any gpu0,gpu1,...

Input parameters:

* `processorList` (`[]ProcessorHandle`) list of PFs of GPU devices
* `mode` (`XgmiFbSharingMode`) framebuffer sharing mode to set

`XgmiFbSharingMode` values:

Field | Description
---|---
`AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM` | custom framebuffer sharing mode
`AMDSMI_XGMI_FB_SHARING_MODE_1` | framebuffer sharing mode_1
`AMDSMI_XGMI_FB_SHARING_MODE_2` | framebuffer sharing mode_2
`AMDSMI_XGMI_FB_SHARING_MODE_4` | framebuffer sharing mode_4
`AMDSMI_XGMI_FB_SHARING_MODE_8` | framebuffer sharing mode_8

Output: `error`

Errors that can be returned by `SetXgmiFbSharingModeV2` function:

* `*StatusError`

Example (custom mode):

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    var customGroup []amdsmi.ProcessorHandle
    if len(processors) > 3 {
        customGroup = []amdsmi.ProcessorHandle{processors[0], processors[2]}
    } else {
        customGroup = processors
    }
    err := amdsmi.SetXgmiFbSharingModeV2(customGroup, amdsmi.AMDSMI_XGMI_FB_SHARING_MODE_CUSTOM)
    if err != nil {
        fmt.Printf("SetXgmiFbSharingModeV2 (CUSTOM) failed: %v\n", err)
    }
}
```

### GetSocPstate

Description: Gets the soc pstate policy for the processor

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `DpmPolicy`, `error`

`DpmPolicy` fields:

Field | Description
---|---
`NumSupported` | the number of supported policies
`Current` | current policy index
`Policies` | slice of `DpmPolicyEntry`

`DpmPolicyEntry` fields:

Field | Description
---|---
`PolicyID` | policy id
`PolicyDescription` | policy description

Errors that can be returned by `GetSocPstate` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        policy, _ := amdsmi.GetSocPstate(processor)
        fmt.Printf("Current: %d, Supported: %d\n", policy.Current, policy.NumSupported)
        for _, p := range policy.Policies {
            fmt.Printf("  id=%d description=%s\n", p.PolicyID, p.PolicyDescription)
        }
    }
}
```

### SetSocPstate

Description: Sets the soc pstate policy for the processor

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device
* `policyID` (uint32) policy id from the policies list obtained via `GetSocPstate`

Output: `error`

Errors that can be returned by `SetSocPstate` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        err := amdsmi.SetSocPstate(processor, 0)
        if err != nil {
            fmt.Printf("SetSocPstate failed: %v\n", err)
        }
    }
}
```

### GetXgmiPlpd

Description: Gets the XGMI per-link power down policy parameter for the processor

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `DpmPolicy`, `error`

`DpmPolicy` fields:

Field | Description
---|---
`NumSupported` | the number of supported policies
`Current` | current policy index
`Policies` | slice of `DpmPolicyEntry`

`DpmPolicyEntry` fields:

Field | Description
---|---
`PolicyID` | policy id
`PolicyDescription` | policy description

Errors that can be returned by `GetXgmiPlpd` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        policy, _ := amdsmi.GetXgmiPlpd(processor)
        fmt.Printf("Current: %d, Supported: %d\n", policy.Current, policy.NumSupported)
        for _, p := range policy.Policies {
            fmt.Printf("  id=%d description=%s\n", p.PolicyID, p.PolicyDescription)
        }
    }
}
```

### SetXgmiPlpd

Description: Sets the XGMI per-link power down policy parameter for the processor

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device
* `policyID` (uint32) policy id from the policies list obtained via `GetXgmiPlpd`

Output: `error`

Errors that can be returned by `SetXgmiPlpd` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        err := amdsmi.SetXgmiPlpd(processor, 0)
        if err != nil {
            fmt.Printf("SetXgmiPlpd failed: %v\n", err)
        }
    }
}
```

### SetPowerCap

Description: Sets GPU power cap.

Input parameters:

* `ph` (`ProcessorHandle`) processor handle
* `sensorInd` (uint32) sensor index. Normally, this will be 0. If a processor has more than one sensor, it could be greater than 0. Parameter sensor_ind is unused on @platform{host}.
* `cap` (uint64) value representing power cap to set. The value must be between the minimum (`MinPowerCap`) and maximum (`MaxPowerCap`) power cap values, which can be obtained from `GetPowerCapInfo`.

Output: `error`

Errors that can be returned by `SetPowerCap` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    sensorInd := uint32(0)
    for _, processor := range processors {
        powerInfo, err := amdsmi.GetPowerCapInfo(processor, sensorInd)
        if err != nil {
            fmt.Printf("GetPowerCapInfo failed: %v\n", err)
            continue
        }
        powerLimit := powerInfo.MinPowerCap +
            uint64(rand.Int63n(int64(powerInfo.MaxPowerCap-powerInfo.MinPowerCap)+1))
        err = amdsmi.SetPowerCap(processor, sensorInd, powerLimit)
        if err != nil {
            fmt.Printf("SetPowerCap failed: %v\n", err)
        }
    }
}
```

### GetSupportedPowerCap

Description: Returns the supported power cap sensors and their types for a device.

**Note:** This function is not yet implemented and will return a `*StatusError` with status `AMDSMI_STATUS_NOT_YET_IMPLEMENTED`.

Input parameters:

* `ph` (`ProcessorHandle`) processor handle

Output: `[]SupportedPowerCapEntry`, `error`

`SupportedPowerCapEntry` fields:

Field | Description
---|---
`SensorIndex` | sensor index to use with power-cap APIs (for example `GetPowerCapInfo` / `SetPowerCap`)
`Type` | power-cap rail / sensor type (`PowerCapType`)

Errors that can be returned by `GetSupportedPowerCap` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    caps, err := amdsmi.GetSupportedPowerCap(processors[0])
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    fmt.Printf("  	Count: %d\n", len(caps))
	for i, e := range caps {
		fmt.Printf("  	[%d] sensor_index=%d type=%s (%d)\n", i, e.SensorIndex, e.Type.String(), e.Type)
	}
}
```

### GetGpuCacheInfo

Description: Returns the cache info for the given processor

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `GpuCacheInfo`, `error`

`GpuCacheInfo` fields:

Field | Description
---|---
`NumCacheTypes` | Number of cache types
`Cache` | Slice of `CacheEntry`

`CacheEntry` fields:

Field | Description
---|---
`CacheProperties` | Bitmask of cache properties from `CachePropertyType`
`CacheSize` | Cache size in KB
`CacheLevel` | Cache level (1, 2, 3, ...)
`MaxNumCuShared` | Number of Compute Units shared
`NumCacheInstance` | Number of instances of this cache type

`CachePropertyType` type values:

Field | Description
---|---
`AMDSMI_CACHE_PROPERTY_ENABLED` | Cache enabled
`AMDSMI_CACHE_PROPERTY_DATA_CACHE` | Data cache
`AMDSMI_CACHE_PROPERTY_INST_CACHE` | Instruction cache
`AMDSMI_CACHE_PROPERTY_CPU_CACHE` | CPU cache
`AMDSMI_CACHE_PROPERTY_SIMD_CACHE` | SIMD cache

Errors that can be returned by `GetGpuCacheInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        cacheInfo, _ := amdsmi.GetGpuCacheInfo(processor)
        for _, cache := range cacheInfo.Cache {
            fmt.Println(cache.CacheProperties)
            fmt.Println(cache.CacheSize)
            fmt.Println(cache.CacheLevel)
            fmt.Println(cache.MaxNumCuShared)
            fmt.Println(cache.NumCacheInstance)
        }
    }
}
```

### GetGpuEccCount

Description: Returns the number of ECC errors on the GPU device for the given block

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query
* `block` The block for which error counts should be retrieved

`block` is `GpuBlock` type:

Field | Description
---|---
`AMDSMI_GPU_BLOCK_UMC` | UMC block
`AMDSMI_GPU_BLOCK_SDMA` | SDMA block
`AMDSMI_GPU_BLOCK_GFX` | GFX block
`AMDSMI_GPU_BLOCK_MMHUB` | MMHUB block
`AMDSMI_GPU_BLOCK_ATHUB` | ATHUB block
`AMDSMI_GPU_BLOCK_PCIE_BIF` | PCIE_BIF block
`AMDSMI_GPU_BLOCK_HDP` | HDP block
`AMDSMI_GPU_BLOCK_XGMI_WAFL` | XGMI_WAFL block
`AMDSMI_GPU_BLOCK_DF` | DF block
`AMDSMI_GPU_BLOCK_SMN` | SMN block
`AMDSMI_GPU_BLOCK_SEM` | SEM block
`AMDSMI_GPU_BLOCK_MP0` | MP0 block
`AMDSMI_GPU_BLOCK_MP1` | MP1 block
`AMDSMI_GPU_BLOCK_FUSE` | FUSE block
`AMDSMI_GPU_BLOCK_MCA` | MCA block
`AMDSMI_GPU_BLOCK_VCN` | VCN block
`AMDSMI_GPU_BLOCK_JPEG` | JPEG block
`AMDSMI_GPU_BLOCK_IH` | IH block
`AMDSMI_GPU_BLOCK_MPIO` | MPIO block

Output: `ErrorCount`, `error`

`ErrorCount` fields:

Field | Description
---|---
`CorrectableCount`| Count of ECC correctable errors
`UncorrectableCount`| Count of ECC uncorrectable errors
`DeferredCount`| Count of ECC deferred errors

Errors that can be returned by `GetGpuEccCount` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        ecc, _ := amdsmi.GetGpuEccCount(processor, amdsmi.AMDSMI_GPU_BLOCK_UMC)
        fmt.Printf("UMC ECC: correctable=%d, uncorrectable=%d, deferred=%d\n",
            ecc.CorrectableCount, ecc.UncorrectableCount, ecc.DeferredCount)
    }
}
```

### GetGpuEccEnabled

Description: Returns ECC capabilities (disable/enable) for each GPU block.

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `map[GpuBlock]bool`, `error`

Each GPU block in the map is from `GpuBlock` type (`false` if the block is not enabled, `true` if the block is enabled):

Field | Description
---|---
`AMDSMI_GPU_BLOCK_UMC` | UMC block
`AMDSMI_GPU_BLOCK_SDMA` | SDMA block
`AMDSMI_GPU_BLOCK_GFX` | GFX block
`AMDSMI_GPU_BLOCK_MMHUB` | MMHUB block
`AMDSMI_GPU_BLOCK_ATHUB` | ATHUB block
`AMDSMI_GPU_BLOCK_PCIE_BIF` | PCIE_BIF block
`AMDSMI_GPU_BLOCK_HDP` | HDP block
`AMDSMI_GPU_BLOCK_XGMI_WAFL` | XGMI_WAFL block
`AMDSMI_GPU_BLOCK_DF` | DF block
`AMDSMI_GPU_BLOCK_SMN` | SMN block
`AMDSMI_GPU_BLOCK_SEM` | SEM block
`AMDSMI_GPU_BLOCK_MP0` | MP0 block
`AMDSMI_GPU_BLOCK_MP1` | MP1 block
`AMDSMI_GPU_BLOCK_FUSE` | FUSE block
`AMDSMI_GPU_BLOCK_MCA` | MCA block
`AMDSMI_GPU_BLOCK_VCN` | VCN block
`AMDSMI_GPU_BLOCK_JPEG` | JPEG block
`AMDSMI_GPU_BLOCK_IH` | IH block
`AMDSMI_GPU_BLOCK_MPIO` | MPIO block

Errors that can be returned by `GetGpuEccEnabled` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        eccMap, _ := amdsmi.GetGpuEccEnabled(processor)
        for block, enabled := range eccMap {
            if enabled {
                fmt.Printf("Block 0x%X: ECC enabled\n", block)
            }
        }
    }
}
```

### GetGpuTotalEccCount

Description: Returns the number of ECC errors on the GPU device

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `ErrorCount`, `error`

`ErrorCount` fields:

Field | Description
---|---
`CorrectableCount`| Count of ECC correctable errors
`UncorrectableCount`| Count of ECC uncorrectable errors
`DeferredCount`| Count of ECC deferred errors

Errors that can be returned by `GetGpuTotalEccCount` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        total, _ := amdsmi.GetGpuTotalEccCount(processor)
        fmt.Printf("Total ECC: correctable=%d, uncorrectable=%d, deferred=%d\n",
            total.CorrectableCount, total.UncorrectableCount, total.DeferredCount)
    }
}
```

### GetGpuCperEntries

Description: Dump CPER entries for a given GPU in a file using from CPER header file from RAS tool.

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device
* `severityMask` (`uint32`) bitmask of severities to retrieve (e.g. `0xFFFFFFFF` for all)
* `cursor` (`uint64`) cursor returned by a previous call (use `0` for the first call)
* `bufferSize` (`uint64`) size of the CPER data buffer in bytes; pass `0` to use the default `4 MiB`

Each call returns at most `20` entries (matching the Python wrapper). Use the returned cursor to fetch additional pages.

Output: `[]CperEntry`, `uint64` (next cursor), `error`

`CperEntry` fields:

Field | Description
---|---
`Header` | parsed `CperHeader` (see below)
`Bytes` | raw bytes of the full CPER record (length == `Header.RecordLength`); pass directly to `GetAfidsFromCper`

`CperHeader` fields (mirror `amdsmi_cper_hdr_t`):

Field | Description
---|---
`Signature` | `string` – always `"CPER"` (4 bytes)
`Revision` | `uint16`
`SignatureEnd` | `uint32` – expected to be `0xFFFFFFFF`
`SecCnt` | `uint16` – number of sections
`ErrorSeverity` | `CperSeverity` – see below
`ValidMask` | `uint32` – raw `amdsmi_cper_valid_bits_t.valid_mask`
`RecordLength` | `uint32` – total size of the CPER record in bytes
`Timestamp` | `CperTimestamp` (with `String()` returning `"YYYY/MM/DD HH:MM:SS"`)
`PlatformID` | `[16]byte`
`PartitionID` | `CperGuid` (`[16]byte`)
`CreatorID` | `[16]byte`
`NotifyType` | `CperGuid` – call `.NotifyType()` to decode against `AMDSMI_CPER_NOTIFY_TYPE_*`
`RecordID` | `[8]byte`
`Flags` | `uint32`
`PersistenceInfo` | `uint64`

`CperSeverity` type (mirrors `amdsmi_cper_sev_t`):

Field | Value | Description
---|---|---
`AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED` | 0 | non-fatal, uncorrected
`AMDSMI_CPER_SEV_FATAL` | 1 | fatal
`AMDSMI_CPER_SEV_NON_FATAL_CORRECTED` | 2 | non-fatal, corrected
`AMDSMI_CPER_SEV_NUM` | 3 | severity count
`AMDSMI_CPER_SEV_UNUSED` | 10 | unused

`CperNotifyType` type (mirrors `amdsmi_cper_notify_type_t`): `CMC`, `CPE`, `MCE`, `PCIE`, `INIT`, `NMI`, `BOOT`, `DMAR`, `SEA`, `SEI`, `PEI`, `CXL_COMPONENT`. Use `entry.Header.NotifyType.NotifyType()` to decode the GUID.

Errors that can be returned by `GetGpuCperEntries` function:

* `*StatusError` (e.g. `AMDSMI_STATUS_OUT_OF_RESOURCES` if `bufferSize` is too small to hold even one record)

Note: `AMDSMI_STATUS_MORE_DATA` from the underlying C call is treated as success — partial entries are still returned and the caller should retry with the new cursor when there is more data.

Example:

```go
processors, _ := amdsmi.GetProcessorHandles()
for _, p := range processors {
    cursor := uint64(0)
    for {
        entries, next, err := amdsmi.GetGpuCperEntries(p, 0xFFFFFFFF, cursor, 0)
        if err != nil {
            fmt.Printf("GetGpuCperEntries failed: %v\n", err)
            break
        }
        for i, e := range entries {
            fmt.Printf("[%s] sev=%s notify=%s record_len=%d\n",
                e.Header.Timestamp,
                e.Header.ErrorSeverity,
                e.Header.NotifyType.NotifyType(),
                e.Header.RecordLength)
            afids, _ := amdsmi.GetAfidsFromCper(e.Bytes)
            fmt.Printf("  entry %d AFIDs: %v\n", i, afids)
        }
        if next == 0 || next == cursor {
            break
        }
        cursor = next
    }
}
```

### GetAfidsFromCper

Description: Extracts AF IDs from a single CPER record buffer. Up to `MAX_NUMBER_OF_AFIDS_PER_RECORD` (12) ids are returned.

Input parameters:

* `cperBuffer` (`[]byte`) bytes of a single CPER record (e.g. `entry.Bytes` returned by `GetGpuCperEntries`)

Output: `[]uint64`, `error`

Errors that can be returned by `GetAfidsFromCper` function:

* `*StatusError` (e.g. `AMDSMI_STATUS_INVAL` if `cperBuffer` is empty)

Example:

```go
afids, err := amdsmi.GetAfidsFromCper(entry.Bytes)
if err != nil {
    fmt.Printf("GetAfidsFromCper failed: %v\n", err)
    return
}
for _, id := range afids {
    fmt.Printf("AFID: 0x%X\n", id)
}
```

### GetGpuRasFeatureInfo

Description: Returns RAS feature info

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `RasFeatureInfo`, `error`

`RasFeatureInfo` fields:

Field | Description
---|---
`RasEepromVersion`| RAS EEPROM version
`EccCorrectionSchemaFlag`| ECC correction schema flag

Errors that can be returned by `GetGpuRasFeatureInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        info, _ := amdsmi.GetGpuRasFeatureInfo(processor)
        fmt.Printf("EEPROM Version: %d, ECC Schema: 0x%X\n",
            info.RasEepromVersion, info.EccCorrectionSchemaFlag)
    }
}
```

### GetGpuBadPageInfo

Description: Returns bad page info.

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device to query

Output: `[]EepromTableRecord`, `error`

`EepromTableRecord` fields:

Field | Description
---|---
`RetiredPage` | 64K/4K Driver managed location that is blocked from further use
`Ts` | Marks the last time when the RAS event was observed
`MemChannel` | Identifies the memory channel the issue has been reported on
`McumcID` | Identifies the memory controller the issue has been reported on

Errors that can be returned by `GetGpuBadPageInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        records, _ := amdsmi.GetGpuBadPageInfo(processor)
        if records == nil {
            fmt.Println("No bad pages")
        } else {
            for _, r := range records {
                fmt.Printf("Page: 0x%X, Channel: %d\n",
                    r.RetiredPage, r.MemChannel)
            }
        }
    }
}
```

### GetGpuRasPolicyInfo

Description: Retrieve the Reliability, Availability, and Serviceability (RAS) policy information for a specified GPU device.

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `RasPolicyInfo`, `error`

`RasPolicyInfo` fields:

Field | Description
---|---
`MajorVersion` | Policy major version
`MinorVersion` | Policy minor version
`V4_0` | v4.0 policy data (`nil` if version != 4.0)

`RasPolicyV4_0` fields (when `V4_0` is non-nil):

Field | Description
---|---
`DramNonCriticalRegionThreshold` | Non-critical region threshold
`DramCriticalRegionThreshold` | Critical region threshold

Errors that can be returned by `GetGpuRasPolicyInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        policy, _ := amdsmi.GetGpuRasPolicyInfo(processor)
        fmt.Printf("RAS Policy: v%d.%d\n", policy.MajorVersion, policy.MinorVersion)
        if policy.V4_0 != nil {
            fmt.Printf("  Non-critical threshold: %d\n",
                policy.V4_0.DramNonCriticalRegionThreshold)
            fmt.Printf("  Critical threshold: %d\n",
                policy.V4_0.DramCriticalRegionThreshold)
        }
    }
}
```

### GetBadPageThreshold

Description: Returns bad page threshold

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query

Output: `uint32`, `error`

Errors that can be returned by `GetBadPageThreshold` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        threshold, _ := amdsmi.GetBadPageThreshold(processor)
        fmt.Printf("Bad page threshold: %d\n", threshold)
    }
}
```

### ResetGpu

Description: Triggers a chain that resets all GPUs.

Input parameters:

* `ph` (`ProcessorHandle`) processor handle

Output: `error`

Errors that can be returned by `ResetGpu` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        err := amdsmi.ResetGpu(processor)
        if err != nil {
            fmt.Printf("ResetGpu failed: %v\n", err)
        }
    }
}
```

### GetNpmInfo

Description: Returns node power management (NPM) status and the node power limit in watts for the given NUMA node.

Note: This function queries the NPM controller for the given node and returns whether NPM is enabled, along with the current node-level power limit in Watts. The NPM status and limit are set out-of-band and reported via this API.

Input parameters:

* `nh` (`NodeHandle`) node handle (typically from `GetNodeHandle`)

Output: `NpmInfo`, `error`

`NpmInfo` fields:

Field | Description
---|---
`Status` | NPM status (`NpmStatus`)
`Limit` | currently set limit

Errors that can be returned by `GetNpmInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    nh, err := amdsmi.GetNodeHandle(processors[0])
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    npm, err := amdsmi.GetNpmInfo(nh)
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    fmt.Printf("  GetNpmInfo: status=%s limit=%d W\n", npm.Status.String(), npm.Limit)
}
```

### GetGpuPtlState

Description: Returns whether PTL (Peak Tops Limiter) is enabled/disabled on the GPU.

Input parameters:

* `ph` (`ProcessorHandle`) processor handle

Output: `bool`, `error`

Errors that can be returned by `GetGpuPtlState` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    state, err := amdsmi.GetGpuPtlState(processors[0])
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    fmt.Printf("  GetGpuPtlState: %v\n", state)
}
```

### SetGpuPtlState

Description:  Sets the PTL (Peak Tops Limiter) enable/disable state for the processor

Input parameters:

* `ph` (`ProcessorHandle`) processor handle
* `enable` (`bool`) True to enable PTL with default formats, False to disable PTL

Output: `error`

Errors that can be returned by `SetGpuPtlState` function:

* `*StatusError`

Example:
```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        // Enable PTL
        if err := amdsmi.SetGpuPtlState(processor, true); err != nil {
            fmt.Printf("SetGpuPtlState failed: %v\n", err)
        }

        // Disable PTL
        // amdsmi.SetGpuPtlState(processor, false)
    }
}
```

### GetGpuPtlFormats

Description: Gets the current PTL (Peak Tops Limiter) preferred data formats for the processor

Input parameters:

* `ph` (`ProcessorHandle`) processor handle

Output: `PtlDataFormat`, `PtlDataFormat`, `error` (first and second format)

`PtlDataFormat` values:

Field | Value | Description
---|---|---
`AMDSMI_PTL_DATA_FORMAT_I8` | `0x0` | Integer 8-bit format
`AMDSMI_PTL_DATA_FORMAT_F16` | `0x1` | Float 16-bit format
`AMDSMI_PTL_DATA_FORMAT_BF16` | `0x2` | Brain Float 16-bit format
`AMDSMI_PTL_DATA_FORMAT_F32` | `0x3` | Float 32-bit format
`AMDSMI_PTL_DATA_FORMAT_F64` | `0x4` | Float 64-bit format
`AMDSMI_PTL_DATA_FORMAT_F8` | `0x5` | Float 8-bit format
`AMDSMI_PTL_DATA_FORMAT_VECTOR` | `0x6` | Vector format
`AMDSMI_PTL_DATA_FORMAT_INVALID` | `0xFFFFFFFF` | Invalid format

Note: if both returned formats equal `AMDSMI_PTL_DATA_FORMAT_I8` (value `0x0`), PTL has not been enabled yet on the system and the PTL state is disabled.

Errors that can be returned by `GetGpuPtlFormats` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    f1, f2, err := amdsmi.GetGpuPtlFormats(processors[0])
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    fmt.Printf("  GetGpuPtlFormats: %s / %s\n", f1.String(), f2.String())
}
```

### SetGpuPtlFormats

Description: Sets the PTL (Peak Tops Limiter) with specified preferred data format pair. PTL must already be enabled before calling this.

Input parameters:

* `ph` (`ProcessorHandle`) processor handle
* `dataFormat1` (`PtlDataFormat`) First preferred data format
* `dataFormat2` (`PtlDataFormat`) Second preferred data format (must be different from data_format1)

The two specified formats will receive accurate performance monitoring and peak performance. F8 and XF32 formats always receive peak performance regardless of this setting.

Output: `error`

Errors that can be returned by `SetGpuPtlFormats` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    on, err := amdsmi.GetGpuPtlState(processors[0])
    if err != nil || !on {
        fmt.Printf("SetGpuPtlFormats: PTL not available or disabled\n")
        return
    }
    f1, f2, err := amdsmi.GetGpuPtlFormats(processors[0])
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    if err := amdsmi.SetGpuPtlFormats(processors[0], f1, f2); err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    fmt.Printf("  SetGpuPtlFormats: round-trip ok\n")
}
```

### GetNumVf

Description: Returns number of enabled VFs and number of supported VFs for the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device for which to query

Output: `numEnabled` (uint32), `numSupported` (uint32), `error`

Errors that can be returned by `GetNumVf` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        enabled, supported, _ := amdsmi.GetNumVf(processor)
        fmt.Printf("VFs: %d enabled, %d supported\n", enabled, supported)
    }
}
```

### GetVfPartitionInfo

Description: Returns a slice of the current framebuffer partitioning structures
on the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device for which to query

Output: `[]PartitionInfo`, `error`

`PartitionInfo` fields:

Field | Description
---|---
`VfHandle` | VF handle
`Fb` | <table> <thead><tr> <th> Subfield </th> <th> Description</th> </tr></thead><tbody><tr><td>`FbSize`</td><td>framebuffer size in MB</td></tr><tr><td>`FbOffset`</td><td>framebuffer offset in MB</td></tr></tbody></table>

Errors that can be returned by `GetVfPartitionInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        partitions, _ := amdsmi.GetVfPartitionInfo(processor)
        for i, p := range partitions {
            fmt.Printf("VF %d: offset=%d MB, size=%d MB\n",
                i, p.Fb.FbOffset, p.Fb.FbSize)
        }
    }
}
```

### GetPartitionProfileInfo

Description: Gets partition profile info

Input parameters:

* `ph` (`ProcessorHandle`) PF of a GPU device

Output: `ProfileInfo`, `error`

`ProfileInfo` fields:

Field | Description
---|---
`CurrentProfileIndex` | current profile index
`ProfileCount` | number of profiles
`Profiles` | slice of all profiles

Where, `Profiles` is a slice of `PartitionProfileEntry` containing:

Field | Description
---|---
`VfCount` | number of VFs
`ProfileCaps` | array of `ProfileCapsInfo` indexed by capability type

`ProfileCapsInfo` fields:

Field | Description
---|---
`Total` | total
`Available` | available
`Optimal` | optimal
`MinValue` | minimum
`MaxValue` | maximum

Keys for `ProfileCaps` array are `ProfileCapability` constants:

Field | Description
---|---
`AMDSMI_PROFILE_CAPABILITY_MEMORY` | memory
`AMDSMI_PROFILE_CAPABILITY_ENCODE` | encode engine
`AMDSMI_PROFILE_CAPABILITY_DECODE` | decode engine
`AMDSMI_PROFILE_CAPABILITY_COMPUTE` | compute engine

Errors that can be returned by `GetPartitionProfileInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        profileInfo, _ := amdsmi.GetPartitionProfileInfo(processor)
        fmt.Printf("Profiles: %d, Current: %d\n",
            profileInfo.ProfileCount, profileInfo.CurrentProfileIndex)
        for i, p := range profileInfo.Profiles {
            fmt.Printf("  Profile %d: %d VFs\n", i, p.VfCount)
        }
    }
}
```

### GetVfInfo

Description: Returns the configuration structure for a given VF

Input parameters:

* `vh` (`VfHandle`) VF handle

Output: `VfInfo`, `error`

`VfInfo` fields:

Field | Description
---|---
`Fb` | <table> <thead><tr> <th> Subfield </th> <th> Description</th> </tr></thead><tbody><tr><td>`FbOffset`</td><td>framebuffer offset in MB</td></tr><tr><td>`FbSize`</td><td>framebuffer size in MB</td></tr></tbody></table>
`GfxTimeslice` | gfx timeslice in us

Errors that can be returned by `GetVfInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        partitions, _ := amdsmi.GetVfPartitionInfo(processor)
        if len(partitions) == 0 {
            continue
        }
        config, _ := amdsmi.GetVfInfo(partitions[0].VfHandle)
        fmt.Printf("fb_offset: %d\n", config.Fb.FbOffset)
        fmt.Printf("fb_size: %d\n", config.Fb.FbSize)
        fmt.Printf("gfx_timeslice: %d\n", config.GfxTimeslice)
    }
}
```

### GetVfData

Description: Returns the scheduler information and guard structure for the given VF.

Input parameters:

* `vh` (`VfHandle`) VF handle

Output: `VfData`, `error`

`VfData` fields:

Field | Description
---|---
`Sched` | Scheduling info (see `SchedInfo` below)
`Guard` | Guard info (see `GuardInfo` below)

`SchedInfo` fields:

Field | Description
---|---
`FlrCount` | function level reset counter
`BootUpTime` | boot up time in microseconds
`ShutdownTime` | shutdown time in microseconds
`ResetTime` | reset time in microseconds
`State` | VF state from `VfSchedState` type
`LastBootStart` | last boot start time
`LastBootEnd` | last boot end time
`LastShutdownStart` | last shutdown start time
`LastShutdownEnd` | last shutdown end time
`LastResetStart` | last reset start time
`LastResetEnd` | last reset end time
`CurrentActiveTime` | current session active time, reset after guest reload
`CurrentRunningTime` | current session running time, reset after guest reload
`TotalActiveTime` | total active time, reset after host reload
`TotalRunningTime` | total running time, reset after host reload

`GuardInfo` fields:

Field | Description
---|---
`Enabled` | show if guard info is enabled for VF
`Guard` | array of `GuardEventInfo` (one per guard event type)

`GuardEventInfo` fields:

Field | Description
---|---
`State` | VF guard state from `GuardState` type
`Amount` | amount of monitor events after enabled
`Interval` | interval in seconds (sliding window) in which events are counted
`Threshold` | maximum number of events that will be processed in the given sliding window interval
`Active` | current number of events in the interval

`GuardEventType` type values are guard event types:

Field | Description
---|---
`AMDSMI_GUARD_EVENT_FLR` | function level reset status
`AMDSMI_GUARD_EVENT_EXCLUSIVE_MOD` | exclusive access mode status
`AMDSMI_GUARD_EVENT_EXCLUSIVE_TIMEOUT` | exclusive access time out status
`AMDSMI_GUARD_EVENT_ALL_INT` | generic interrupt status
`AMDSMI_GUARD_EVENT_RAS_ERR_COUNT` | RAS error count status
`AMDSMI_GUARD_EVENT_RAS_CPER_DUMP` | RAS CPER dump status
`AMDSMI_GUARD_EVENT_RAS_BAD_PAGES` | RAS bad pages status

`GuardState` type values:

Field | Description
---|---
`AMDSMI_GUARD_STATE_NORMAL` | the event number is within the threshold
`AMDSMI_GUARD_STATE_FULL` | the event number hits the threshold
`AMDSMI_GUARD_STATE_OVERFLOW` | the event number is bigger than the threshold

`VfSchedState` type values:

Field | Description
---|---
`AMDSMI_VF_STATE_UNAVAILABLE` | VF state unavailable
`AMDSMI_VF_STATE_AVAILABLE` | VF state available
`AMDSMI_VF_STATE_ACTIVE` | VF state active
`AMDSMI_VF_STATE_SUSPENDED` | VF state suspended
`AMDSMI_VF_STATE_FULLACCESS` | VF state fullaccess
`AMDSMI_VF_STATE_DEFAULT_AVAILABLE` | same as available, indicates this is a default VF

Errors that can be returned by `GetVfData` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        partitions, _ := amdsmi.GetVfPartitionInfo(processor)
        if len(partitions) == 0 {
            continue
        }
        vfData, _ := amdsmi.GetVfData(partitions[0].VfHandle)
        schedInfo := vfData.Sched
        guardInfo := vfData.Guard
        fmt.Println(schedInfo.BootUpTime)
        fmt.Println(schedInfo.FlrCount)
        fmt.Println(schedInfo.State)
        fmt.Println(schedInfo.LastBootStart)
        fmt.Println(schedInfo.LastBootEnd)
        fmt.Println(schedInfo.LastShutdownStart)
        fmt.Println(schedInfo.LastShutdownEnd)
        fmt.Println(schedInfo.ShutdownTime)
        fmt.Println(schedInfo.LastResetStart)
        fmt.Println(schedInfo.LastResetEnd)
        fmt.Println(schedInfo.ResetTime)
        fmt.Println(schedInfo.CurrentActiveTime)
        fmt.Println(schedInfo.CurrentRunningTime)
        fmt.Println(schedInfo.TotalActiveTime)
        fmt.Println(schedInfo.TotalRunningTime)

        fmt.Println(guardInfo.Enabled)
        for i, g := range guardInfo.Guard {
            fmt.Printf("Guard %d: state=%s, amount=%d, interval=%d, threshold=%d, active=%d\n",
                i, g.State, g.Amount, g.Interval, g.Threshold, g.Active)
        }
    }
}
```

### EventCreate

Description: Allocate a new event set notifier to monitor different types of issues with the GPU running virtualization SW.

Input parameters:

* `processors` (`[]ProcessorHandle`) Processor handles for the GPUs to listen for events
* `eventTypes` (`uint64`) Bitmask of event categories and severities to monitor (use `BuildEventMask` to construct it)

Bitmask layout (matches the C API `amdsmi_event_create`):

```
| 63 62 61 60 | 59 .......... 0 |
| event severity | event category bit field |
```

Output: `EventSet`, `error`

Errors that can be returned by `EventCreate` function:

* `*StatusError` (e.g. `AMDSMI_STATUS_INVAL` if `processors` is empty)

The returned `EventSet` is opaque and must be released by `EventDestroy`.

`EventCategory` type:

Field | Description
---|---
`AMDSMI_EVENT_CATEGORY_NON_USED` | placeholder, no events
`AMDSMI_EVENT_CATEGORY_DRIVER` | driver layer events
`AMDSMI_EVENT_CATEGORY_RESET` | reset events (GPU/FLR)
`AMDSMI_EVENT_CATEGORY_SCHED` | world-switch / scheduler events
`AMDSMI_EVENT_CATEGORY_VBIOS` | VBIOS load and parsing events
`AMDSMI_EVENT_CATEGORY_ECC` | ECC error events
`AMDSMI_EVENT_CATEGORY_PP` | power play events
`AMDSMI_EVENT_CATEGORY_IOV` | SR-IOV events
`AMDSMI_EVENT_CATEGORY_VF` | per-VF events
`AMDSMI_EVENT_CATEGORY_FW` | firmware events
`AMDSMI_EVENT_CATEGORY_GPU` | GPU device-level events
`AMDSMI_EVENT_CATEGORY_GUARD` | guard threshold events
`AMDSMI_EVENT_CATEGORY_GPUMON` | gpumon configuration events
`AMDSMI_EVENT_CATEGORY_MMSCH` | MMSCH events
`AMDSMI_EVENT_CATEGORY_XGMI` | XGMI topology / FB sharing events
`AMDSMI_EVENT_CATEGORY__MAX` | count sentinel from the C enum (not a real category bit)

`EventSeverity` type (same numbering as the `Level` field returned by `EventRead`):

Field | Value | Description
---|---|---
`AMDSMI_EVENT_SEVERITY_HIGH` | 0 | high severity only
`AMDSMI_EVENT_SEVERITY_MED` | 1 | include medium severity (also includes HIGH)
`AMDSMI_EVENT_SEVERITY_LOW` | 2 | include low severity (also includes HIGH+MED)
`AMDSMI_EVENT_SEVERITY_WARN` | 3 | include warnings
`AMDSMI_EVENT_SEVERITY_INFO` | 4 | include informational events

Mask helpers:

* `BuildEventMask(categories []EventCategory, severity EventSeverity) uint64` — sets `1 << cat` for each entry in `categories`, plus the severity bits implied by `severity` (HIGH adds none, MED adds bit 60, LOW adds bit 61, WARN adds bit 62, INFO adds bit 63).
* `BuildEventMaskAllCategories(severity EventSeverity) uint64` — enables every category bit (bits 0..59) with the given severity.
* `BuildEventMaskAllSeverities(categories []EventCategory) uint64` — enables every severity bit (bits 60..63) with the given categories.

Raw mask constants (mirror the `AMDSMI_MASK_*` macros from `amdsmi.h`, suitable to pass directly to `EventCreate`):

Constant | Value | Description
---|---|---
`AMDSMI_MASK_INIT` | `0` | clear mask
`AMDSMI_MASK_DEFAULT` | `(1<<62)-1` | every category + HIGH/MED/LOW severities (no WARN/INFO)
`AMDSMI_MASK_ALL` | `^uint64(0)` | every category + every severity
`AMDSMI_EVENT_MASK_ALL_CATEGORIES` | `(1<<60)-1` | every category bit only (bits 0..59)
`AMDSMI_EVENT_MASK_ALL_SEVERITIES` | `0xF<<60` | every severity bit only (bits 60..63)

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
mask := amdsmi.BuildEventMaskAllCategories(amdsmi.AMDSMI_EVENT_SEVERITY_INFO)
set, err := amdsmi.EventCreate(processors, mask)
if err != nil {
    fmt.Printf("EventCreate failed: %v\n", err)
    return
}
defer amdsmi.EventDestroy(set)
```

### EventRead

Description: Block until an event is delivered, the timeout expires, or return immediately when `timeoutUsec == 0`. A negative timeout blocks indefinitely.

Note: Internally the timeout in microseconds is converted to milliseconds by the library; values smaller than 1000 us are clamped to 1000 us (i.e. 1 ms).

Input parameters:

* `set` (`EventSet`) event set returned by `EventCreate`
* `timeoutUsec` (`int64`) timeout in microseconds

Output: `EventEntry`, `error`

`EventEntry` fields:

Field | Description
---|---
`FcnId` | `VfHandle` of the function that originated the event
`DevId` | originating device id
`Timestamp` | UTC microseconds
`Data` | category-specific payload; for `Category == AMDSMI_EVENT_CATEGORY_PP` and `Subcode == AMDSMI_EVENT_PP_THROTTLER_EVENT`, cast to `PpThrottlerType` (see *Data payload constants* below)
`Category` | `EventCategory` of the event
`Subcode` | category-specific subcode; cast to the matching `Event<Cat>Subcode` type (e.g. `EventDriverSubcode`, `EventEccSubcode`) to get the named constant, or call `SubcodeName(entry.Category, entry.Subcode)` for a string
`Level` | `EventSeverity` of the event
`Date` | UTC date and time string
`Message` | human-readable description (up to 256 bytes)
`ProcessorHandle` | handle of the GPU that produced the event

Errors that can be returned by `EventRead` function:

* `*StatusError` (e.g. `AMDSMI_STATUS_TIMEOUT` when no event arrives in time)

Example:

```go
entry, err := amdsmi.EventRead(set, 500_000) // 500 ms
if err != nil {
    fmt.Printf("EventRead failed: %v\n", err)
    return
}
fmt.Printf("Event @ %s [%s/%s] %s: %s\n",
    entry.Date, entry.Category, entry.Level,
    amdsmi.SubcodeName(entry.Category, entry.Subcode),
    entry.Message)
```

#### Subcode constants

For every `EventCategory` the package exposes a typed `int32` with named constants that mirror the matching `amdsmi_event_<category>_t` type in `interface/amdsmi.h`, plus a `String()` method on each:

C type | Go type | Constant prefix
---|---|---
`amdsmi_event_gpu_t` | `EventGpuSubcode` | `AMDSMI_EVENT_GPU_*`
`amdsmi_event_driver_t` | `EventDriverSubcode` | `AMDSMI_EVENT_DRIVER_*`
`amdsmi_event_fw_t` | `EventFwSubcode` | `AMDSMI_EVENT_FW_*`
`amdsmi_event_reset_t` | `EventResetSubcode` | `AMDSMI_EVENT_RESET_*`
`amdsmi_event_iov_t` | `EventIovSubcode` | `AMDSMI_EVENT_IOV_*`
`amdsmi_event_ecc_t` | `EventEccSubcode` | `AMDSMI_EVENT_ECC_*`
`amdsmi_event_pp_t` | `EventPpSubcode` | `AMDSMI_EVENT_PP_*`
`amdsmi_event_sched_t` | `EventSchedSubcode` | `AMDSMI_EVENT_SCHED_*`
`amdsmi_event_vf_max_t` | `EventVfSubcode` | `AMDSMI_EVENT_VF_*`
`amdsmi_event_vbios_t` | `EventVbiosSubcode` | `AMDSMI_EVENT_VBIOS_*`
`amdsmi_event_guard_t` | `EventGuardSubcode` | `AMDSMI_EVENT_GUARD_*`
`amdsmi_event_gpumon_t` | `EventGpumonSubcode` | `AMDSMI_EVENT_GPUMON_*`
`amdsmi_event_mmsch_t` | `EventMmschSubcode` | `AMDSMI_EVENT_MMSCH_*`
`amdsmi_event_xgmi_t` | `EventXgmiSubcode` | `AMDSMI_EVENT_XGMI_*`

`SubcodeName(category EventCategory, subcode uint32) string` decodes a `(Category, Subcode)` pair using the right type. Cast directly when you already know the category, e.g. `amdsmi.EventEccSubcode(entry.Subcode) == amdsmi.AMDSMI_EVENT_ECC_FATAL_ERROR`.

#### Data payload constants

A few `(Category, Subcode)` combinations encode an extra enum value in the `Data` field. The package exposes typed constants for these as well:

C type | Go type | Constant prefix | When `Data` holds it
---|---|---|---
`amdsmi_pp_throttler_type_t` | `PpThrottlerType` | `AMDSMI_EVENT_THROTTLER_*` | `Category == AMDSMI_EVENT_CATEGORY_PP` and `Subcode == AMDSMI_EVENT_PP_THROTTLER_EVENT`

Each typed constant has a `String()` method, e.g. `amdsmi.PpThrottlerType(entry.Data).String()` returns `"THROTTLER_VR"` for `AMDSMI_EVENT_THROTTLER_VR`.

### EventDestroy

Description: Destroys and frees an event set previously returned by `EventCreate`. Calling it on a zero-valued `EventSet` is a no-op.

Input parameters:

* `set` (`EventSet`) event set to destroy

Output: `error`

Errors that can be returned by `EventDestroy` function:

* `*StatusError`

Example (low-level lifecycle — for the higher-level `EventReader` see above):

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
mask := amdsmi.BuildEventMask(
    []amdsmi.EventCategory{
        amdsmi.AMDSMI_EVENT_CATEGORY_DRIVER,
        amdsmi.AMDSMI_EVENT_CATEGORY_ECC,
        amdsmi.AMDSMI_EVENT_CATEGORY_GPU,
    },
    amdsmi.AMDSMI_EVENT_SEVERITY_LOW,
)
set, err := amdsmi.EventCreate(processors, mask)
if err != nil {
    fmt.Printf("EventCreate failed: %v\n", err)
    return
}
defer amdsmi.EventDestroy(set)

for i := 0; i < 5; i++ {
    entry, err := amdsmi.EventRead(set, 1_000_000) // 1 s
    if err != nil {
        fmt.Printf("EventRead [%d]: %v\n", i, err)
        continue
    }
    fmt.Printf("[%s] %s/%s subcode=%s msg=%q\n",
        entry.Date, entry.Category, entry.Level,
        amdsmi.SubcodeName(entry.Category, entry.Subcode),
        entry.Message)
}
```

### EventReader

Description: A small lifetime wrapper around `EventCreate` / `EventRead` / `EventDestroy` that owns the underlying `EventSet`. This is the Go equivalent of Python's `AmdSmiEventReader` — Python uses a context manager (`with`); Go uses `defer reader.Close()`.

Constructors:

* `NewEventReader(processors []ProcessorHandle, categories []EventCategory, severity EventSeverity) (*EventReader, error)` — builds the mask via `BuildEventMask`.
* `NewEventReaderAllCategories(processors []ProcessorHandle, severity EventSeverity) (*EventReader, error)` — every category at the chosen severity (uses `BuildEventMaskAllCategories`).
* `NewEventReaderMask(processors []ProcessorHandle, mask uint64) (*EventReader, error)` — full control; pass any `uint64` (e.g. `AMDSMI_MASK_DEFAULT`, `AMDSMI_MASK_ALL`, or a hand-built mask).

Methods:

Method | Description
---|---
`Read(timeoutUsec int64) (EventEntry, error)` | thin wrapper over `EventRead`. Negative timeout blocks forever, `0` returns immediately
`Close() error` | calls `EventDestroy` and clears internal state. Safe on a nil receiver and idempotent
`Set() EventSet` | returns the underlying `EventSet` if you need to call the lower-level functions directly

Errors:

* `*StatusError` from any of the underlying calls (e.g. `AMDSMI_STATUS_INVAL` if `processors` is empty, `AMDSMI_STATUS_TIMEOUT` from `Read`)

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}

reader, err := amdsmi.NewEventReader(processors,
    []amdsmi.EventCategory{
        amdsmi.AMDSMI_EVENT_CATEGORY_DRIVER,
        amdsmi.AMDSMI_EVENT_CATEGORY_ECC,
        amdsmi.AMDSMI_EVENT_CATEGORY_GPU,
    },
    amdsmi.AMDSMI_EVENT_SEVERITY_LOW,
)
if err != nil {
    fmt.Printf("NewEventReader failed: %v\n", err)
    return
}
defer reader.Close()

for i := 0; i < 5; i++ {
    entry, err := reader.Read(1_000_000) // 1 s timeout
    if err != nil {
        fmt.Printf("Read [%d]: %v\n", i, err)
        continue
    }
    fmt.Printf("[%s] %s/%s subcode=%s msg=%q\n",
        entry.Date, entry.Category, entry.Level,
        amdsmi.SubcodeName(entry.Category, entry.Subcode),
        entry.Message)
}
```

### GetGuestData

Description: Gets guest OS information of the queried VF

Input parameters:

* `vh` (`VfHandle`) VF handle (for example from `GetVfPartitionInfo`)

Output: `GuestData`, `error`

`GuestData` fields:

Field | Description
---|---
`DriverVersion` | guest driver version string
`FbUsage` | guest framebuffer usage (MB)

Errors that can be returned by `GetGuestData` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    parts, err := amdsmi.GetVfPartitionInfo(processors[0])
    if err != nil || len(parts) == 0 {
        fmt.Printf("No VF partitions\n")
        return
    }
    gd, err := amdsmi.GetGuestData(parts[0].VfHandle)
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    fmt.Printf("  GetGuestData: driver=%q fb_usage=%d\n", gd.DriverVersion, gd.FbUsage)
}
```

### GetVfFwInfo

Description: Returns firmware block IDs and versions as reported for the VF (`amdsmi_get_vf_fw_info`).

Input parameters:

* `vh` (`VfHandle`) VF handle (for example from `GetVfPartitionInfo`)

Output: `FwInfo`, `error`

`FwInfo` fields:

Field | Description
---|---
`NumFwInfo` | number of valid firmware entries
`FwList` | firmware entry array (`FwInfoList`) containing `FwID` and `FwVersion`

Errors that can be returned by `GetVfFwInfo` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    parts, err := amdsmi.GetVfPartitionInfo(processors[0])
    if err != nil || len(parts) == 0 {
        fmt.Printf("No VF partitions\n")
        return
    }
    fw, err := amdsmi.GetVfFwInfo(parts[0].VfHandle)
    if err != nil {
        fmt.Printf("Error: %v\n", err)
        return
    }
    fmt.Printf("  GetVfFwInfo: num_fw_info=%d\n", fw.NumFwInfo)
}
```

### ClearVfFb

Description: Clears framebuffer of the given VF on the given GPU.

`If trying to clear the framebuffer of an active function,`
`the call will fail`

Input parameters:

* `vh` (`VfHandle`) VF device handle

Output: `error`

Errors that can be returned by `ClearVfFb` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    partitions, _ := amdsmi.GetVfPartitionInfo(processors[0])
    if len(partitions) > 0 {
        err = amdsmi.ClearVfFb(partitions[0].VfHandle)
        if err != nil {
            fmt.Printf("ClearVfFb failed: %v\n", err)
        }
    }
}
```

### SetNumVf

Description: Set number of enabled VFs for the given GPU

Input parameters:

* `ph` (`ProcessorHandle`) GPU device which to query
* `numVf` (uint32) number of enabled VFs to be set

Output: `error`

Errors that can be returned by `SetNumVf` function:

* `*StatusError`

Example:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
if len(processors) == 0 {
    fmt.Println("No GPUs on machine")
} else {
    for _, processor := range processors {
        err := amdsmi.SetNumVf(processor, 2)
        if err != nil {
            fmt.Printf("SetNumVf failed: %v\n", err)
        }
    }
}
```

### GetNicDriverInfo

Description: Returns driver information for the given NIC

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query

Output: `NicDriverInfo`, `error`

`NicDriverInfo` fields:

Field | Description
---|---
`Name` | Driver name
`Version` | Driver version

Errors that can be returned by `GetNicDriverInfo` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    info, _ := amdsmi.GetNicDriverInfo(nic)
    fmt.Printf("Driver: %s v%s\n", info.Name, info.Version)
}
```

### GetNicAsicInfo

Description: Returns ASIC information for the given NIC

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query

Output: `NicAsicInfo`, `error`

`NicAsicInfo` fields:

Field | Description
---|---
`VendorID` | Vendor ID
`SubvendorID` | Subsystem vendor ID
`DeviceID` | Device ID
`SubsystemID` | Subsystem device ID
`Revision` | Revision
`PermanentAddress` | Permanent MAC address
`ProductName` | Product name
`PartNumber` | Part number
`SerialNumber` | Serial number
`VendorName` | Vendor name

Errors that can be returned by `GetNicAsicInfo` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    info, _ := amdsmi.GetNicAsicInfo(nic)
    fmt.Printf("NIC: %s (0x%04X)\n", info.ProductName, info.DeviceID)
    fmt.Printf("  Vendor: %s (0x%04X)\n", info.VendorName, info.VendorID)
    fmt.Printf("  Serial: %s\n", info.SerialNumber)
}
```

### GetNicBusInfo

Description: Returns bus information for the given NIC

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query

Output: `NicBusInfo`, `error`

`NicBusInfo` fields:

Field | Description
---|---
`Bdf` | NIC BDF address
`MaxPcieWidth` | Maximum PCIe width
`MaxPcieSpeed` | Maximum PCIe speed in GT/s
`PcieInterfaceVersion` | PCIe interface version string
`SlotType` | Slot type string

Errors that can be returned by `GetNicBusInfo` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    info, _ := amdsmi.GetNicBusInfo(nic)
    fmt.Printf("NIC BDF: %s, PCIe x%d @ %d GT/s\n",
        info.Bdf, info.MaxPcieWidth, info.MaxPcieSpeed)
}
```

### GetNicNumaInfo

Description: Returns NUMA information for the given NIC

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query

Output: `NicNumaInfo`, `error`

`NicNumaInfo` fields:

Field | Description
---|---
`Node` | NUMA node number
`Affinity` | CPU affinity string

Errors that can be returned by `GetNicNumaInfo` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    info, _ := amdsmi.GetNicNumaInfo(nic)
    fmt.Printf("NUMA node: %d, Affinity: %s\n", info.Node, info.Affinity)
}
```

### GetNicFwInfo

Description: Retrieves firmware version information for the NIC

**Note:** This API depends on `libmnl`. If `libmnl` is not installed on the
system, this function raises `AmdSmiLibraryException` with status `AMDSMI_STATUS_NOT_SUPPORTED`.

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query

Output: `NicFwInfo`, `error`

`NicFwInfo` fields:

Field | Description
---|---
`NumFw` | Number of valid entries in `Fw`
`Fw` | Slice of `NicFwEntry` describing each firmware slot

`NicFwEntry` fields:

Field | Description
---|---
`Type` | `NicFwVersionType` identifying the slot (`FIXED`, `RUNNING`, `STORED`)
`Fw` | `NicFw` with the firmware `Name` and `Version`

`NicFwVersionType` values:

Value | Description
---|---
`AMDSMI_NIC_FW_VERSION_TYPE_FIXED` | Hardware-fixed firmware version
`AMDSMI_NIC_FW_VERSION_TYPE_RUNNING` | Currently running firmware version
`AMDSMI_NIC_FW_VERSION_TYPE_STORED` | Stored (pending) firmware version

Errors that can be returned by `GetNicFwInfo` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    info, _ := amdsmi.GetNicFwInfo(nic)
    fmt.Printf("Num FW entries: %d\n", info.NumFw)
    for i, fw := range info.Fw {
        fmt.Printf("  fw %d: type=%s name=%s version=%s\n",
            i, fw.Type, fw.Fw.Name, fw.Fw.Version)
    }
}
```

### GetNicPortInfo

Description: Returns the port information (L2/physical) for all NIC ports on the given NIC.

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query

Output: `NicPortInfo`, `error`

`NicPortInfo` fields:

Field | Description
---|---
`NumPorts` | Number of valid entries in `Ports`
`Ports` | Slice of `NicPort` structs describing each port

`NicPort` fields:

Field | Description
---|---
`Bdf` | BDF of the port
`PortNum` | Port number
`Type` | Type of the port
`Flavour` | Port flavour
`Netdev` | Associated network device name
`Ifindex` | Interface index of the port
`MacAddress` | MAC address assigned to the port
`Carrier` | Carrier state
`Mtu` | Maximum Transmission Unit size
`LinkState` | Current link state
`LinkSpeed` | Link speed in Mbps
`ActiveFec` | Currently active Forward Error Correction mode
`Autoneg` | Auto-negotiation status
`PauseAutoneg` | Pause frame auto-negotiation status
`PauseRx` | Receive pause frame status
`PauseTx` | Transmit pause frame status

Active FEC Modes:

The `ActiveFec` field provides a bitmask representation of Active FEC (Active Forward Error Correction) modes.
The bitmask values are derived from the `ethtool_fecparam` structure, specifically the `active_fec` field.
Below are examples of the defined FEC modes:

* `ETHTOOL_FEC_NONE` (0x01)
* `ETHTOOL_FEC_AUTO` (0x02)
* `ETHTOOL_FEC_RS` (0x04)
* `ETHTOOL_FEC_BASER` (0x08)
* `ETHTOOL_FEC_LLRS` (0x10)
* `ETHTOOL_FEC_OFF` (0x20)

Note: These definitions are based on the latest available ethtool information. Users should verify if there are any updates or changes to these definitions in the relevant ethtool structure or field before implementing them in their code.

Errors that can be returned by `GetNicPortInfo` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    info, _ := amdsmi.GetNicPortInfo(nic)
    fmt.Printf("Num ports: %d\n", info.NumPorts)
    for i, p := range info.Ports {
        fmt.Printf("  port %d: netdev=%s mac=%s link=%s speed=%d\n",
            i, p.Netdev, p.MacAddress, p.LinkState, p.LinkSpeed)
    }
}
```

### GetNicRdmaDevInfo

Description: Returns RDMA devices and per-port information for the given NIC.

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query

Output: `NicRdmaDevicesInfo`, `error`

`NicRdmaDevicesInfo` fields:

Field | Description
---|---
`NumRdmaDev` | Number of valid entries in `RdmaDevInfo`
`RdmaDevInfo` | Slice of `NicRdmaDevInfo` structs

`NicRdmaDevInfo` fields:

Field | Description
---|---
`RdmaDev` | RDMA device name
`NodeGuid` | Node GUID
`NodeType` | Node type
`SysImageGuid` | System image GUID
`FwVer` | Firmware version
`NumRdmaPorts` | Number of valid entries in `RdmaPortInfo`
`RdmaPortInfo` | Slice of `NicRdmaPortInfo` structs

`NicRdmaPortInfo` fields:

Field | Description
---|---
`Netdev` | Associated netdev name
`State` | Port state string (e.g. `ACTIVE`, `DOWN`)
`RdmaPort` | RDMA port number
`MaxMtu` | Maximum MTU size
`ActiveMtu` | Currently active MTU

Errors that can be returned by `GetNicRdmaDevInfo` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    info, _ := amdsmi.GetNicRdmaDevInfo(nic)
    for _, d := range info.RdmaDevInfo {
        fmt.Printf("rdma_dev=%s fw=%s ports=%d\n", d.RdmaDev, d.FwVer, d.NumRdmaPorts)
        for _, p := range d.RdmaPortInfo {
            fmt.Printf("  port %d: state=%s mtu=%d/%d\n",
                p.RdmaPort, p.State, p.ActiveMtu, p.MaxMtu)
        }
    }
}
```

### GetNicPortStatistics

Description: Retrieve all available PORT statistics for the specified NIC port

Uses a two-call pattern internally: first queries the count, then retrieves the entries.

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query
* `portIndex` (`uint32`) zero-based NIC port index

Output: `[]NicStat`, `error`

`NicStat` fields:

Field | Description
---|---
`Name` | Statistic name
`Value` | Statistic value (uint64)

Errors that can be returned by `GetNicPortStatistics` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    stats, _ := amdsmi.GetNicPortStatistics(nic, 0)
    for _, s := range stats {
        fmt.Printf("%s = %d\n", s.Name, s.Value)
    }
}
```

### GetNicVendorStatistics

Description: Retrieve vendor specific statistics for the NIC port

Uses a two-call pattern internally: first queries the count, then retrieves the entries.

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query
* `portIndex` (`uint32`) zero-based NIC port index

Output: `[]NicStat`, `error`

This API provides access to vendor/driver specific statistics that may vary between different NIC vendors and driver/firmware versions. The statistic names are preserved as provided by the underlying driver implementation.

Note: The exact statistics available depend on the NIC vendor, hardware and driver version.

Errors that can be returned by `GetNicVendorStatistics` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    stats, _ := amdsmi.GetNicVendorStatistics(nic, 0)
    for _, s := range stats {
        fmt.Printf("%s = %d\n", s.Name, s.Value)
    }
}
```

### GetNicRdmaPortStatistics

Description: Retrieve RDMA port statistics for the NIC

Uses a two-call pattern internally: first queries the count, then retrieves the entries.

Input parameters:

* `ph` (`ProcessorHandle`) NIC device for which to query
* `rdmaPortIndex` (`uint32`) zero-based NIC RDMA port index

Output: `[]NicStat`, `error`

Errors that can be returned by `GetNicRdmaPortStatistics` function:

* `*StatusError`

Example:

```go
nics, err := amdsmi.GetNicProcessorHandles()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for _, nic := range nics {
    stats, _ := amdsmi.GetNicRdmaPortStatistics(nic, 0)
    for _, s := range stats {
        fmt.Printf("%s = %d\n", s.Name, s.Value)
    }
}
```

### GetTdiState

Description: Returns the lifecycle state of the TDI (TEE Device Interface) for a virtual function.

Input parameters:

* `vh` (`VfHandle`) VF for which to query

Output: `TdiState`, `error`

`TdiState` values:

Value | Description
---|---
`AMDSMI_TDI_STATE_UNLOCKED` (`0`) | TDI is unlocked
`AMDSMI_TDI_STATE_LOCKED` (`1`) | TDI is locked
`AMDSMI_TDI_STATE_RUN` (`2`) | TDI is actively running
`AMDSMI_TDI_STATE_ERROR` (`3`) | TDI is in an error state and cannot be used

Errors that can be returned by `GetTdiState` function:

* `*StatusError`

Example:

```go
partitions, err := amdsmi.GetVfPartitionInfo(handle)
if err != nil || len(partitions) == 0 {
    return
}
state, err := amdsmi.GetTdiState(partitions[0].VfHandle)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
fmt.Printf("TDI state: %s (%d)\n", state, state)
```

### GetCcMode

Description: Returns the Confidential Compute mode currently configured on a GPU.

Input parameters:

* `ph` (`ProcessorHandle`) PF of a processor for which to query

Output: `CcMode`, `error`

`CcMode` values:

Value | Description
---|---
`AMDSMI_CC_MODE_OFF` (`0`) | Confidential Compute disabled
`AMDSMI_CC_MODE_ON` (`1`) | Confidential Compute enabled
`AMDSMI_CC_MODE_DEV` (`2`) | Confidential Compute enabled in developer mode

Errors that can be returned by `GetCcMode` function:

* `*StatusError`

Example:

```go
mode, err := amdsmi.GetCcMode(handle)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
fmt.Printf("CC mode: %s (%d)\n", mode, mode)
```

### SetCcMode

Description: Sets the Confidential Compute mode on a GPU.

Input parameters:

* `ph` (`ProcessorHandle`) PF of a processor for which to set the mode
* `mode` (`CcMode`) mode to apply (see `GetCcMode` for the value table)

Output: `error`

Errors that can be returned by `SetCcMode` function:

* `*StatusError`

Example:

```go
if err := amdsmi.SetCcMode(handle, amdsmi.AMDSMI_CC_MODE_ON); err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
```

### GetVfHbmInfo

Description: Returns HBM (High Bandwidth Memory) information for a virtual function.

Input parameters:

* `vh` (`VfHandle`) VF for which to query

Output: `VfHbmInfo`, `error`

`VfHbmInfo` fields:

Field | Description
---|---
`PhyAddr` | Physical address of the HBM region
`PhySize` | Physical size in bytes
`NumaID` | NUMA node id for driver-managed VF HBM. `0xFFFFFFFF` when no NUMA association exists (e.g. DAX mode).
`Name` | Backing device or region name

Errors that can be returned by `GetVfHbmInfo` function:

* `*StatusError`

Example:

```go
partitions, err := amdsmi.GetVfPartitionInfo(handle)
if err != nil || len(partitions) == 0 {
    return
}
info, err := amdsmi.GetVfHbmInfo(partitions[0].VfHandle)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
fmt.Printf("HBM phy_addr=0x%X size=%d numa=%d name=%s\n",
    info.PhyAddr, info.PhySize, info.NumaID, info.Name)
```

### GetGpuFabricInfo

Description: Returns Fabric (scale-up networking) configuration for a GPU. The C structure is versioned with a discriminated union; the Go type exposes the version number and the parsed variant.

Input parameters:

* `ph` (`ProcessorHandle`) PF of a processor for which to query

Output: `FabricInfo`, `error`

`FabricInfo` fields:

Field | Description
---|---
`Bdf` | BDF of the Fabric device
`Version` | Variant version number; selects which `InfoVx` field is populated
`InfoV1` | Parsed `FabricInfoV1` (only populated when `Version == 1`)

`FabricInfoV1` fields:

Field | Description
---|---
`AcceleratorID` | Accelerator identifier (range 0..1023)
`FabricType` | `FabricType` (`AMDSMI_FABRIC_TYPE_UALOE`, `AMDSMI_FABRIC_TYPE_UALINK`, or `AMDSMI_FABRIC_TYPE_UNKNOWN`)
`Bandwidth` | Station bandwidth share in Mb/s
`Latency` | Latency in nanoseconds
`PpodID` | Physical PoD identifier (128-bit UUID as `[16]byte`)
`PpodSize` | Physical PoD size
`VpodID` | Virtual PoD identifier
`VpodSize` | Virtual PoD size
`VpodActiveAccelerators` | 1024-bit list stored as 32 x `uint32` words; bit N set = accelerator ID N is active in the vPoD
`LocalAccelerators` | Up to 8 local accelerator IDs
`AddrMode` | `FabricNpaAddressMode` (source aliasing, source identification, or unknown)
`AccelState` | `FabricAcceleratorVpodState` (UNCONFIGURED, CONFIGURED, READY, ACTIVE, ERROR, UNKNOWN)

`FabricType` values:

Value | Description
---|---
`AMDSMI_FABRIC_TYPE_UALOE` (`0`) | UALink-over-Ethernet fabric
`AMDSMI_FABRIC_TYPE_UALINK` (`1`) | Native UALink fabric
`AMDSMI_FABRIC_TYPE_UNKNOWN` (`2`) | Unknown fabric type

`FabricNpaAddressMode` values:

Value | Description
---|---
`AMDSMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_ALIASING` (`0`) | NPA remaps source IDs per vPoD
`AMDSMI_FABRIC_NPA_ADDRESS_MODE_SOURCE_IDENTIFICATION` (`1`) | Original source IDs preserved
`AMDSMI_FABRIC_NPA_ADDRESS_MODE_UNKNOWN` (`2`) | Unknown address mode

`FabricAcceleratorVpodState` values:

Value | Description
---|---
`AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_UNCONFIGURED` (`0`) | Not yet assigned to a vPoD
`AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_CONFIGURED` (`1`) | Assigned but not ready for traffic
`AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_READY` (`2`) | Configured and ready, awaiting activation
`AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_ACTIVE` (`3`) | Live and participating in the vPoD
`AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_ERROR` (`4`) | In an error state
`AMDSMI_FABRIC_ACCELERATOR_VPOD_STATE_UNKNOWN` (`5`) | Unknown accelerator vPoD state

Errors that can be returned by `GetGpuFabricInfo` function:

* `*StatusError`

Example:

```go
info, err := amdsmi.GetGpuFabricInfo(handle)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
fmt.Printf("Fabric BDF=%s version=%d\n", info.Bdf, info.Version)
if info.Version == 1 {
    v1 := info.InfoV1
    fmt.Printf("  accel_id=%d type=%s bw=%d Mb/s latency=%d ns\n",
        v1.AcceleratorID, v1.FabricType, v1.Bandwidth, v1.Latency)
    fmt.Printf("  ppod=%x/%d  vpod=%d/%d  state=%s addr_mode=%s\n",
        v1.PpodID, v1.PpodSize, v1.VpodID, v1.VpodSize,
        v1.AccelState, v1.AddrMode)
}
```

### AllocFabricTelemetry

Description: Allocates a Fabric telemetry session for the requested categories.

The returned `*FabricTelemetry` is the lifecycle wrapper around C-allocated storage. Call `.Get()` to populate it with the latest snapshot, and `.Close()` to release it. A finalizer is installed as a safety net, but callers should always pair the call with `defer t.Close()`.

This function mirrors the Python `AmdSmiFabricTelemetry` class.

Input parameters:

* `ph` (`ProcessorHandle`) GPU for which to allocate telemetry
* `categories` (`...FabricTelemetryCategory`) variadic list of categories to request. If empty, all categories from `UALOE` through `DERIVED_NETPORT` are requested (matching the Python default).

Output: `*FabricTelemetry`, `error`

`FabricTelemetryCategory` values:

Value | Description
---|---
`AMDSMI_FABRIC_TELEMETRY_CATEGORY_UALOE` (`0`) | UALOE telemetry
`AMDSMI_FABRIC_TELEMETRY_CATEGORY_SWITCH` (`1`) | Switch telemetry
`AMDSMI_FABRIC_TELEMETRY_CATEGORY_CRYPTO` (`2`) | Crypto telemetry
`AMDSMI_FABRIC_TELEMETRY_CATEGORY_PFC` (`3`) | PFC telemetry
`AMDSMI_FABRIC_TELEMETRY_CATEGORY_NETPORT` (`4`) | Network Port telemetry
`AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_UALOE` (`5`) | Derived UALOE telemetry
`AMDSMI_FABRIC_TELEMETRY_CATEGORY_DERIVED_NETPORT` (`6`) | Derived Network Port telemetry
`AMDSMI_FABRIC_TELEMETRY_CATEGORY_MAX` (`7`) | Maximum number of categories
`AMDSMI_FABRIC_TELEMETRY_CATEGORY_INVALID` (`0xFFFFFFFF`) | Unknown / invalid category

`*FabricTelemetry` methods:

Method | Description
---|---
`(*FabricTelemetry).Get() (FabricTelemetryData, error)` | Refresh the telemetry snapshot (`amdsmi_get_fabric_telemetry_data`) and return a parsed copy fully detached from C memory.
`(*FabricTelemetry).Close() error` | Release the C-allocated storage (`amdsmi_free_fabric_telemetry`). Idempotent; clears the safety-net finalizer.
`(*FabricTelemetry).Categories() []FabricTelemetryCategory` | Return the categories this session was allocated for.

`FabricTelemetryData` fields:

Field | Description
---|---
`Datasets` | `map[FabricTelemetryCategory]FabricTelemetryDataset` keyed by category. Categories that were requested but not populated by the driver appear with `InstanceCount == 0`.

`FabricTelemetryDataset` fields:

Field | Description
---|---
`Category` | `FabricTelemetryCategory` for this dataset
`GenerationCount` | Sequence number incremented each time telemetry is written
`TimestampSec` | UTC timestamp seconds since epoch (`tv_sec`)
`TimestampNsec` | UTC timestamp sub-second part (`tv_nsec`)
`InstanceCount` | Number of instances in this category
`Instances` | Slice of `FabricTelemetryInstance`

`FabricTelemetryInstance` fields:

Field | Description
---|---
`Name` | Instance label (max 32 chars)
`LogicalIdx` | Logical index for this instance
`ItemCount` | Number of telemetry items
`Items` | Slice of `FabricTelemetryItem`

`FabricTelemetryItem` fields:

Field | Description
---|---
`ID` | Identifier of the telemetry item
`Value` | Value of the telemetry item

Errors that can be returned by `AllocFabricTelemetry` function:

* `*StatusError`
* `error` if any of the supplied categories is out of range or the C call returns a NULL pointer

Example:

```go
tel, err := amdsmi.AllocFabricTelemetry(handle,
    amdsmi.AMDSMI_FABRIC_TELEMETRY_CATEGORY_UALOE,
    amdsmi.AMDSMI_FABRIC_TELEMETRY_CATEGORY_NETPORT)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
defer tel.Close()

snap, err := tel.Get()
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for cat, ds := range snap.Datasets {
    fmt.Printf("category=%s instances=%d gen=%d ts=%d.%09d\n",
        cat, ds.InstanceCount, ds.GenerationCount,
        ds.TimestampSec, ds.TimestampNsec)
    for _, inst := range ds.Instances {
        fmt.Printf("  inst %s/%d items=%d\n", inst.Name, inst.LogicalIdx, inst.ItemCount)
    }
}
```

### GetFabricTelemetry

Description: Convenience wrapper that allocates a telemetry session, fetches one snapshot, and frees the session before returning. Mirrors the Python `amdsmi_get_fabric_telemetry` helper.

Input parameters:

* `ph` (`ProcessorHandle`) GPU for which to fetch telemetry
* `categories` (`...FabricTelemetryCategory`) variadic list of categories to request. If empty, all categories from `UALOE` through `DERIVED_NETPORT` are requested.

Output: `FabricTelemetryData`, `error`

See `AllocFabricTelemetry` above for the description of `FabricTelemetryData` and the available `FabricTelemetryCategory` values.

Errors that can be returned by `GetFabricTelemetry` function:

* `*StatusError`

Example:

```go
snap, err := amdsmi.GetFabricTelemetry(handle)
if err != nil {
    fmt.Printf("Error: %v\n", err)
    return
}
for cat, ds := range snap.Datasets {
    fmt.Printf("%-20s instances=%d items=%d\n",
        cat, ds.InstanceCount,
        func() int {
            n := 0
            for _, i := range ds.Instances {
                n += int(i.ItemCount)
            }
            return n
        }())
}
```
