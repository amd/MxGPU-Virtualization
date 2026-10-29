# Run the AMD SMI static identity example

The static identity example is a read-only introduction to hardware and
software identity information available through AMD SMI. It reports ASIC,
board, VRAM, cache, driver, and virtualization details for every discovered
AMD GPU. It then checks optional NIC ASIC, PCIe bus, driver, and RDMA device
information.

`go/examples/static_identity/amdsmi_static_identity.go` is the Go
implementation.

The example does not modify device state, reset a GPU, change a configuration,
or collect changing telemetry such as temperature, power, or utilization.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. An AMD GPU is required for GPU identity output.

NIC identity requires an AMD SMI build with NIC support and an appropriate NIC
driver. RDMA device details can be incomplete when supporting system libraries,
such as `libibverbs`, are unavailable. The example reports unavailable optional
capabilities and continues with the remaining queries.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/static_identity/amdsmi_static_identity.go
```

Reported values depend on the installed hardware, driver, firmware, and
optional NIC support. If no GPUs are available, the example prints
`No GPUs found` before checking optional NIC identity.

## Example flow

The example initializes AMD SMI, obtains GPU processor handles, prints static
identity information for every GPU, checks optional NIC identity, and then
shuts down AMD SMI:

```go
initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS
if err := amdsmi.Init(initFlags); err != nil {
	return err
}
defer func() {
	if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
		fmt.Println(shutdownErr)
		return
	}
	fmt.Println("Shutdown complete")
}()

processors, err := amdsmi.GetProcessorHandles()
if err != nil {
	return err
}
for processorNumber, processor := range processors {
	if err := printGpuStaticIdentity(processorNumber, processor); err != nil {
		return err
	}
}

return printNicStaticIdentity()
```

`amdsmi.AMDSMI_INIT_ALL_PROCESSORS` is passed explicitly so the example can
discover GPUs and optional NICs. The current native implementation does not
use this flag to restrict the discovery scope.

`amdsmi.GetProcessorHandles()` returns opaque GPU processor handles. Keep a
returned handle and pass it to the per-GPU APIs; do not construct handles
yourself. `amdsmi.GetGpuDeviceBdf()` prints each GPU's
PCI bus:device.function (BDF) address, such as `0000:41:00.0`, so the output
can be associated with a physical device.

### Read GPU ASIC information

`amdsmi.GetGpuAsicInfo()` returns ASIC identifiers and capabilities:

```go
asicInfo, err := amdsmi.GetGpuAsicInfo(processor)
if err != nil {
	return err
}
fmt.Println(asicInfo.MarketName)
fmt.Println(asicInfo.VendorID)
fmt.Println(asicInfo.VendorName)
fmt.Println(asicInfo.SubvendorID)
fmt.Println(asicInfo.DeviceID)
fmt.Println(asicInfo.RevID)
fmt.Println(asicInfo.AsicSerial)
fmt.Println(asicInfo.OamID)
fmt.Println(asicInfo.NumComputeUnits)
fmt.Println(asicInfo.TargetGraphicsVersion)
fmt.Println(asicInfo.SubsystemID)
fmt.Println(asicInfo.Flags)
```

The returned `amdsmi.AsicInfo` contains:

| Field                  | Meaning                                             |
| ---------------------- | --------------------------------------------------- |
| `MarketName`           | Marketed GPU name                                   |
| `VendorID`             | PCI vendor identifier                               |
| `VendorName`           | GPU vendor name                                     |
| `SubvendorID`          | PCI subsystem vendor identifier                     |
| `DeviceID`             | PCI device identifier                               |
| `RevID`                | ASIC revision identifier                            |
| `AsicSerial`           | ASIC serial number, when available                  |
| `OamID`                | OAM module identifier, or `N/A` when not applicable |
| `NumComputeUnits`      | Number of compute units                             |
| `TargetGraphicsVersion`| Graphics IP target version                          |
| `SubsystemID`          | PCI subsystem identifier                            |
| `Flags`                | ASIC capability flags                               |

PCI identifiers are formatted as hexadecimal strings. Some identity fields can
be empty or unavailable when the platform or firmware does not provide them.

### Read GPU board information

`amdsmi.GetGpuBoardInfo()` returns board-level identification:

```go
boardInfo, err := amdsmi.GetGpuBoardInfo(processor)
if err != nil {
	return err
}
fmt.Println(boardInfo.ProductName)
fmt.Println(boardInfo.ProductSerial)
```

The returned `amdsmi.BoardInfo` contains:

| Field              | Meaning                           |
| ------------------ | --------------------------------- |
| `ModelNumber`      | Board model number                |
| `ProductSerial`    | Board product serial number       |
| `FruID`            | Field-replaceable unit identifier |
| `ProductName`      | Board product name                |
| `ManufacturerName` | Board manufacturer name           |

Board details are populated by the platform and firmware. A missing or empty
field does not necessarily indicate an AMD SMI error.

### Read GPU VRAM and cache information

`amdsmi.GetGpuVramInfo()` describes the GPU's installed video memory:

```go
vramInfo, err := amdsmi.GetGpuVramInfo(processor)
if err != nil {
	return err
}
fmt.Println(vramInfo.VramType)
fmt.Println(vramInfo.VramSize)
fmt.Println(vramInfo.VramMaxBandwidth)
```

The returned `amdsmi.VramInfo` contains:

| Field              | Meaning                                       |
| ------------------ | --------------------------------------------- |
| `VramType`         | VRAM technology as an `amdsmi.VramType` value |
| `VramVendor`       | VRAM vendor name                              |
| `VramSize`         | Total VRAM size in bytes                      |
| `VramBitWidth`     | VRAM interface width in bits                  |
| `VramMaxBandwidth` | Maximum VRAM bandwidth in MB/s                |

`amdsmi.GetGpuCacheInfo()` returns cache entries for the GPU. The example
prints one entry for each cache type:

```go
cacheInfo, err := amdsmi.GetGpuCacheInfo(processor)
if err != nil {
	return err
}
for _, cache := range cacheInfo.Cache {
	fmt.Println(cache.CacheProperties)
	fmt.Println(cache.CacheSize)
	fmt.Println(cache.CacheLevel)
}
```

Each `amdsmi.CacheEntry` contains:

| Field              | Meaning                                                           |
| ------------------ | ----------------------------------------------------------------- |
| `CacheProperties`  | Cache roles as an `amdsmi.CachePropertyType` value                |
| `CacheSize`        | Size of one cache instance in bytes                               |
| `CacheLevel`       | Cache hierarchy level                                             |
| `MaxNumCuShared`   | Maximum number of compute units that share the cache              |
| `NumCacheInstance` | Number of cache instances                                         |

An empty `Cache` slice means that no cache entries were returned for the GPU.

### Read GPU driver and virtualization information

`amdsmi.GetGpuDriverInfo()` identifies the driver serving a GPU:

```go
driverInfo, err := amdsmi.GetGpuDriverInfo(processor)
if err != nil {
	return err
}
fmt.Println(driverInfo.DriverName)
fmt.Println(driverInfo.DriverVersion)
fmt.Println(driverInfo.DriverDate)
```

The returned `amdsmi.DriverInfo` contains `DriverName`, `DriverVersion`, and
`DriverDate`. These describe the installed GPU driver, not the AMD SMI library
version.

`amdsmi.GetGpuVirtualizationMode()` identifies the GPU's virtualization
context:

```go
virtualizationMode, err := amdsmi.GetGpuVirtualizationMode(processor)
if err != nil {
	return err
}
fmt.Println(virtualizationMode)
```

The return value is an `amdsmi.VirtualizationMode`. Printing the value uses
its `String()` representation so the output is easier to associate with the
corresponding AMD SMI Go API values.

### Read optional NIC identity information

NIC identity is a separate optional section. The example first obtains NIC
processor handles and prints the number of discovered NICs:

```go
nicProcessors, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNicProcessorHandles",
	amdsmi.GetNicProcessorHandles,
)
if err != nil {
	return nil
}
fmt.Println(len(nicProcessors))
```

For each returned handle, `amdsmi.GetNicDeviceBdf()` identifies the NIC by its
PCI bus:device.function (BDF) address:

```go
nicBdf, err := amdsmi.GetNicDeviceBdf(nic)
if err != nil {
	return err
}
fmt.Println(nicBdf)
```

`amdsmi.GetNicAsicInfo()` returns the NIC's PCI identifiers and product
identity:

```go
asicInfo, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNicAsicInfo",
	func() (amdsmi.NicAsicInfo, error) {
		return amdsmi.GetNicAsicInfo(nic)
	},
)
if err == nil {
	fmt.Println(asicInfo.VendorID)
	fmt.Println(asicInfo.SubvendorID)
	fmt.Println(asicInfo.DeviceID)
	fmt.Println(asicInfo.SubsystemID)
	fmt.Println(asicInfo.Revision)
	fmt.Println(asicInfo.PermanentAddress)
	fmt.Println(asicInfo.ProductName)
	fmt.Println(asicInfo.PartNumber)
	fmt.Println(asicInfo.SerialNumber)
	fmt.Println(asicInfo.VendorName)
}
```

The returned `amdsmi.NicAsicInfo` contains:

| Field              | Meaning                         |
| ------------------ | ------------------------------- |
| `VendorID`         | PCI vendor identifier           |
| `SubvendorID`      | PCI subsystem vendor identifier |
| `DeviceID`         | PCI device identifier           |
| `SubsystemID`      | PCI subsystem identifier        |
| `Revision`         | Device revision identifier      |
| `PermanentAddress` | Permanent MAC address           |
| `ProductName`      | NIC product name                |
| `PartNumber`       | NIC part number                 |
| `SerialNumber`     | NIC serial number               |
| `VendorName`       | NIC vendor name                 |

`amdsmi.GetNicBusInfo()` returns the NIC's PCIe bus capabilities:

```go
busInfo, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNicBusInfo",
	func() (amdsmi.NicBusInfo, error) {
		return amdsmi.GetNicBusInfo(nic)
	},
)
if err == nil {
	fmt.Println(busInfo.Bdf)
	fmt.Println(busInfo.MaxPcieWidth)
	fmt.Println(busInfo.MaxPcieSpeed)
	fmt.Println(busInfo.PcieInterfaceVersion)
	fmt.Println(busInfo.SlotType)
}
```

The returned `amdsmi.NicBusInfo` contains:

| Field                  | Meaning                           |
| ---------------------- | --------------------------------- |
| `Bdf`                  | PCI bus:device.function address   |
| `MaxPcieWidth`         | Maximum supported PCIe link width |
| `MaxPcieSpeed`         | Maximum supported PCIe link speed |
| `PcieInterfaceVersion` | PCIe interface version            |
| `SlotType`             | Physical PCIe slot type           |

`amdsmi.GetNicDriverInfo()` identifies the driver serving the NIC:

```go
driverInfo, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNicDriverInfo",
	func() (amdsmi.NicDriverInfo, error) {
		return amdsmi.GetNicDriverInfo(nic)
	},
)
if err == nil {
	fmt.Println(driverInfo.Name)
	fmt.Println(driverInfo.Version)
}
```

The returned `amdsmi.NicDriverInfo` contains `Name` and `Version`.

`amdsmi.GetNicRdmaDevInfo()` returns RDMA device entries. Each entry
identifies an RDMA device and contains a list of RDMA ports:

```go
rdmaDevInfo, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNicRdmaDevInfo",
	func() (amdsmi.NicRdmaDevicesInfo, error) {
		return amdsmi.GetNicRdmaDevInfo(nic)
	},
)
if err == nil {
	for _, rdmaDev := range rdmaDevInfo.RdmaDevInfo {
		fmt.Println(rdmaDev.RdmaDev)
		fmt.Println(rdmaDev.NodeGuid)
		fmt.Println(rdmaDev.NodeType)
		fmt.Println(rdmaDev.SysImageGuid)
		fmt.Println(rdmaDev.FwVer)

		for _, rdmaPort := range rdmaDev.RdmaPortInfo {
			fmt.Println(rdmaPort.Netdev)
			fmt.Println(rdmaPort.State)
			fmt.Println(rdmaPort.RdmaPort)
			fmt.Println(rdmaPort.MaxMtu)
			fmt.Println(rdmaPort.ActiveMtu)
		}
	}
}
```

Each `amdsmi.NicRdmaDevInfo` contains:

| Field          | Meaning                                      |
| -------------- | -------------------------------------------- |
| `RdmaDev`      | RDMA device name                             |
| `NodeGuid`     | RDMA node globally unique identifier         |
| `NodeType`     | RDMA node type                               |
| `SysImageGuid` | RDMA system-image globally unique identifier |
| `FwVer`        | RDMA device firmware version                 |
| `RdmaPortInfo` | Slice of `amdsmi.NicRdmaPortInfo` values     |

Each `amdsmi.NicRdmaPortInfo` contains:

| Field       | Meaning                        |
| ----------- | ------------------------------ |
| `Netdev`    | Associated network device name |
| `State`     | RDMA port state                |
| `RdmaPort`  | RDMA port number               |
| `MaxMtu`    | Maximum supported MTU          |
| `ActiveMtu` | Currently active MTU           |

The maximum and active MTU values can be unavailable or incomplete without
`libibverbs`. Each NIC query is handled independently, so an unavailable API
does not prevent the example from trying the remaining NIC identity APIs.

### Handle unavailable capabilities and clean up

The optional NIC APIs can fail when the hardware, driver, operating system, or
build configuration does not support them. The example handles these status
codes without terminating the GPU identity flow:

- `AMDSMI_STATUS_NOT_SUPPORTED`
- `AMDSMI_STATUS_DRIVER_NOT_LOADED`
- `AMDSMI_STATUS_NOT_FOUND`
- `AMDSMI_STATUS_NO_DATA`

These APIs return an `amdsmi.StatusError` for the statuses above. The example
uses `example_utils.GetOptionalAPI()` so optional failures are printed without
aborting the run. Other AMD SMI errors are returned from `run()`. Shutdown is
placed in a `defer` so `amdsmi.ShutDown()` runs after a successful
`amdsmi.Init()`, releasing AMD SMI resources before the program exits.
