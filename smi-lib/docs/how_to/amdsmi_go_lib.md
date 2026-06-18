---
myst:
  html_meta:
    "description lang=en": "Get started with the AMD SMI Go interface."
    "keywords": "api, smi, lib, go, system, management, interface, cgo"
---

# Go package usage

Before using the AMD SMI Go bindings make sure that:

* Go 1.18+ (64-bit) is installed on your system.
* The AMD SMI C library (`libamdsmi.so`) is built and installed (e.g. in `/usr/local/lib/`), or its location is exported via `LD_LIBRARY_PATH`.
* CGO is enabled (default when a C toolchain such as `gcc` is available).

The Go bindings are a thin idiomatic wrapper around the AMD SMI C library and live under `smi-lib/go/`:

```text
smi-lib/go/
├── amdsmi/
│   └── amdsmi_interface.go   # Go interface (types, constants, functions)
├── examples/
│   └── usage.go              # Runnable example exercising the API
├── go.mod                    # Go module definition (module: smi-lib/go)
└── README.md                 # Documentation
```

Open a terminal and navigate to the `smi-lib/go/` directory. The provided example program can be executed with:

```bash
sudo go run examples/usage.go
```

Running the example requires `libamdsmi.so` to be discoverable at link/run time. If it is installed somewhere other than `/usr/local/lib`, point `LD_LIBRARY_PATH` at it, for example:

```bash
export LD_LIBRARY_PATH=/path/to/libamdsmi:$LD_LIBRARY_PATH
sudo go run examples/usage.go
```

Most functions require you to specify the processor handle for which you want to retrieve information. To obtain handles, call `amdsmi.GetProcessorHandles()`, which returns the list of GPU device handles on the current machine:

```go
handles, err := amdsmi.GetProcessorHandles()
if err != nil {
    log.Fatalf("GetProcessorHandles failed: %v", err)
}
fmt.Printf("Found %d processor(s)\n", len(handles))
```

Once you have a processor handle, you can call any API that works with it. For example:

```go
info, err := amdsmi.GetGpuAsicInfo(handles[0])
if err != nil {
    fmt.Printf("GetGpuAsicInfo failed: %v\n", err)
} else {
    fmt.Printf("GPU: %s (0x%X)\n", info.MarketName, info.DeviceID)
}
```

Finally, call `amdsmi.ShutDown()` to release the SMI library resources and close the connection to the driver:

```go
amdsmi.ShutDown()
```

## AMD SMI Go example

An application using AMD SMI must call `amdsmi.Init()` to initialize the AMD SMI library before all other calls. `amdsmi.ShutDown()` must be the last call to properly close the connection to the driver and release any resources held by AMD SMI.

A minimal Go program looks like:

```go
package main

import (
    "fmt"
    "log"

    "smi-lib/go/amdsmi"
)

func main() {
    if err := amdsmi.Init(amdsmi.AMDSMI_INIT_AMD_GPUS); err != nil {
        log.Fatalf("Init failed: %v", err)
    }
    defer amdsmi.ShutDown()

    handles, err := amdsmi.GetProcessorHandles()
    if err != nil {
        log.Fatalf("GetProcessorHandles failed: %v", err)
    }

    for i, h := range handles {
        info, err := amdsmi.GetGpuAsicInfo(h)
        if err != nil {
            fmt.Printf("GPU[%d] GetGpuAsicInfo failed: %v\n", i, err)
            continue
        }
        fmt.Printf("GPU[%d] Vendor ID: 0x%04X Device ID: 0x%X Market name: %s\n",
            i, info.VendorID, info.DeviceID, info.MarketName)
    }
}
```

## Error handling

All Go wrappers return `error` as the last return value. Failures from the underlying C library are reported as `*amdsmi.StatusError`, which exposes both the numeric code (`Code`) and the status name (`Name`) mapped from `amdsmi_status_t`. Use `errors.As` to inspect the error in detail:

```go
import "errors"

info, err := amdsmi.GetGpuAsicInfo(handle)
if err != nil {
    var smiErr *amdsmi.StatusError
    if errors.As(err, &smiErr) {
        fmt.Printf("SMI error: %s (code %d)\n", smiErr.Name, smiErr.Code)
    } else {
        fmt.Printf("Unexpected error: %v\n", err)
    }
    return
}
```

For the full list of status codes and the complete Go API, refer to the [Go API reference](../reference/amdsmi_go_api.md).
