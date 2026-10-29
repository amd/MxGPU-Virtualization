# Run the AMD SMI telemetry example

The telemetry example is a read-only monitoring program for AMD GPUs and, when
available, NICs. It discovers GPUs, prints their PCIe capabilities, selects the
supported GPU telemetry API, collects a small number of current samples, and
then checks optional NIC port and RDMA counters.

`go/examples/telemetry/amdsmi_telemetry.go` is the Go implementation.

On supported AMD Instinct MI accelerator boards, the example uses the
self-describing metric table returned by the GPU metrics API
(`amdsmi.GetGpuMetrics()`). Navi32 boards that do not provide that table use
the focused compatibility telemetry APIs instead. The program detects the
available path from the API result; it does not infer support from the GPU
marketing name.

The example does not reset a GPU, change power or clock settings, create VFs,
or otherwise modify device state.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. An AMD GPU must be available.

NIC monitoring is optional. It requires an AMD SMI build with NIC support and
an appropriate NIC driver. RDMA port details can be unavailable or incomplete
without supporting system libraries such as `libibverbs`.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/telemetry/amdsmi_telemetry.go
```

The reported values depend on the GPU, driver, firmware, workload, and
optional NIC support. An idle GPU can report low activity and clocks, while a
GPU under load can report different values for every poll.

## Choose the poll duration

The example is finite so it is safe to run unchanged. By default, it gathers
three samples one second apart:

```go
const (
	pollIterations = 3
	pollInterval   = time.Second
)
```

Edit these constants near the beginning of
`go/examples/telemetry/amdsmi_telemetry.go` to collect a longer interval. For
example, ten samples at five-second intervals use:

```go
const (
	pollIterations = 10
	pollInterval   = 5 * time.Second
)
```

Use `Ctrl+C` to stop the example early. The program cancels a
`signal.NotifyContext` and still runs `amdsmi.ShutDown()` from `defer`.

## Example flow

The program initializes AMD SMI, gets GPU processor handles, gathers GPU
telemetry for each handle, checks optional NIC telemetry, and shuts down the
library:

```go
if err = amdsmi.Init(amdsmi.AMDSMI_INIT_ALL_PROCESSORS); err != nil {
	return fmt.Errorf("initialize AMD SMI: %w", err)
}
defer amdsmi.ShutDown()

processors, err := amdsmi.GetProcessorHandles()
if err != nil {
	return fmt.Errorf("get GPU processor handles: %w", err)
}

for _, processor := range processors {
	printGpuPcieCapabilities(processor)
}

if err = pollGpuTelemetry(ctx, processors); err != nil {
	return err
}

return printNicTelemetry(ctx)
```

`amdsmi.GetProcessorHandles()` returns opaque GPU processor handles. Keep a
returned handle and pass it to the per-GPU APIs; do not construct handles
yourself. `amdsmi.GetGpuDeviceBdf()` identifies each GPU in the output with
its PCI bus:device.function (BDF) address, such as `0000:0c:00.0`.

`amdsmi.AMDSMI_INIT_ALL_PROCESSORS` is passed explicitly so the program can
also discover optional NIC processors. The current native implementation does
not use this flag to restrict the discovery scope.

### Inspect GPU PCIe capabilities

Before polling, the example prints the static PCIe link fields:

```go
pcieInfo, err := amdsmi.GetPcieInfo(processor)
if err != nil {
	return
}
pcieStatic := pcieInfo.Static
fmt.Println(pcieStatic.MaxPcieWidth)
fmt.Println(pcieStatic.MaxPcieSpeed)
fmt.Println(pcieStatic.PcieInterfaceVersion)
fmt.Println(pcieStatic.SlotType)
fmt.Println(pcieStatic.MaxPcieInterfaceVersion)
```

It then queries the supported PCIe transfer-rate table.
`TransferRate.Current` is the index of the active entry, not the transfer
rate itself:

```go
pciBandwidth, err := amdsmi.GetGpuPciBandwidth(processor)
if err != nil {
	return
}
transferRate := pciBandwidth.TransferRate
currentIndex := transferRate.Current
fmt.Println(currentIndex)
fmt.Println(transferRate.Values[currentIndex])
fmt.Println(pciBandwidth.Lanes[currentIndex])
for i, frequency := range transferRate.Values {
	fmt.Println(frequency, pciBandwidth.Lanes[i])
}
```

`transferRate.Values` is the supported frequency list (C `frequency[]`). The
Go example looks up the active transfer rate and lane count from
`Values[Current]` and `Lanes[Current]`.

The supported PCIe transfer-rate API is hardware and driver dependent. If it
returns `STATUS_NOT_SUPPORTED`, the example reports that status and continues
with the remaining telemetry APIs.

This capability section is independent of the selected GPU telemetry path.

### Choose the GPU telemetry path

Before polling, the example calls the GPU metrics API once for each GPU. A
successful call selects the metrics-table path and supplies the first sample.
`STATUS_NOT_SUPPORTED` selects the focused compatibility APIs:

```go
metrics, err := amdsmi.GetGpuMetrics(processor)
if err == nil {
	telemetryPaths = append(telemetryPaths, gpuTelemetryPath{
		processorNumber: processorNumber,
		processor:       processor,
		useMetricsTable: true,
		metricsValid:    true,
		metrics:         metrics,
	})
	continue
}

var statusErr *amdsmi.StatusError
if errors.As(err, &statusErr) &&
	statusErr.Code == amdsmi.AMDSMI_STATUS_NOT_SUPPORTED {
	telemetryPaths = append(telemetryPaths, gpuTelemetryPath{
		processorNumber: processorNumber,
		processor:       processor,
	})
	continue
}
```

Checking the return status instead of `market_name` also works with future
GPU, firmware, and driver combinations. An empty metric list is still a
successful metrics-table response and does not select the compatibility path.
If the initial call fails with any other status, the example reports the error
and skips GPU telemetry for that device rather than silently changing API
families.

### Read metrics-table telemetry on MI boards

On supported MI accelerator boards, the GPU metrics API returns a list of
self-describing metrics. The API result used to select the path becomes the
first sample, and the example refreshes the list for each later poll. Before
printing, the example sorts metrics by name (then resource group, subgroup,
and instance) so related values such as `CLK_GFX` and `CLK_GFX_MIN_LIMIT`
appear together:

```go
metrics, err := amdsmi.GetGpuMetrics(processor)
if err != nil {
	return
}
for _, metric := range metrics {
	fmt.Println(metric.Name)
	fmt.Println(metric.Val)
	fmt.Println(metric.Unit)
	fmt.Println(metric.Category)
}
```

Each `amdsmi.Metric` contains:

| Field         | Meaning                                                      |
| ------------- | ------------------------------------------------------------ |
| `Name`        | Metric identifier as an `amdsmi.MetricName` value            |
| `Val`         | Current or accumulated metric value                          |
| `Unit`        | Unit as an `amdsmi.MetricUnit` value                         |
| `Category`    | Measurement category as an `amdsmi.MetricCategory` value     |
| `Flags`       | Metric characteristics; use `Flags.Flags()` for the list     |
| `VfMask`      | Bitmask of the PF and active VFs to which the metric applies |
| `ResGroup`    | Resource group                                               |
| `ResSubgroup` | Resource subgroup                                            |
| `ResInstance` | Resource instance number                                     |

The available metrics vary by ASIC, firmware, and virtualization
configuration. An empty list means that no metrics were returned.

### Read compatibility telemetry on Navi32 boards

When the GPU metrics API returns `STATUS_NOT_SUPPORTED`, as it does on
Navi32 boards, the example polls the focused compatibility APIs instead:

- GPU activity (`amdsmi.GetGpuActivity()`) for graphics, UMC, and
  multimedia activity.
- Power info (`amdsmi.GetPowerInfo()`) for socket power, GFX, SOC, and
  memory voltages.
- Temperature (`amdsmi.GetTempMetric()`) for current edge and hotspot
  temperatures.
- Clock info (`amdsmi.GetClockInfo()`) for the GFX and memory clock domains.
- PCIe info (`amdsmi.GetPcieInfo()`) for the current PCIe speed, width,
  bandwidth, and PCIe error counters.
- PCIe bandwidth (`amdsmi.GetGpuPciBandwidth()`) for the active PCIe
  transfer rate, looked up from the current index.

```go
activity, err := amdsmi.GetGpuActivity(processor)
power, err := amdsmi.GetPowerInfo(processor)
edgeTemperature, err := amdsmi.GetTempMetric(
	processor,
	amdsmi.AMDSMI_TEMPERATURE_TYPE_EDGE,
	amdsmi.AMDSMI_TEMP_CURRENT,
)
gfxClock, err := amdsmi.GetClockInfo(processor, amdsmi.AMDSMI_CLK_TYPE_GFX)
```

`activity` is an `amdsmi.EngineUsage` with `GfxActivity`, `UmcActivity`, and
`MmActivity`. `power` is an `amdsmi.PowerInfo` with `SocketPower`,
`GfxVoltage`, `SocVoltage`, and `MemVoltage`. A clock value is an
`amdsmi.ClkInfo` with `Clk`, `MinClk`, `MaxClk`, `ClkLocked`, and
`ClkDeepSleep`.

The PCIe metric on `amdsmi.PcieInfo` contains `PcieSpeed`, `PcieWidth`, and
`PcieBandwidth`, together with the replay, recovery, rollover, and NAK counter
fields. The Go example looks up the active transfer rate from
`Values[TransferRate.Current]`.

These counters are useful when comparing samples over time. After selecting
this path, the poll loop does not call the GPU metrics API again for that GPU.

Both telemetry paths print values in the units returned by AMD SMI. Consult
the Go API reference when an application needs to convert or persist a
value with a specific unit.

### Monitor optional NIC ports and counters

The NIC section is independent from GPU telemetry. It first gets NIC processor
handles, identifies each NIC by BDF, and prints its port information:

```go
nicProcessors, err := amdsmi.GetNicProcessorHandles()
if err != nil {
	return nil
}
for _, nic := range nicProcessors {
	portInfo, err := amdsmi.GetNicPortInfo(nic)
	if err != nil {
		continue
	}
	for _, port := range portInfo.Ports {
		fmt.Println(port.Netdev)
		fmt.Println(port.LinkState)
		fmt.Println(port.LinkSpeed)
	}
}
```

Each NIC port entry includes its BDF, port number, type, flavour, network
device, interface index, MAC address, carrier state, MTU, link state, link
speed, FEC state, and pause/autonegotiation settings.

For every poll iteration, the example refreshes the port list and retrieves the
standard and vendor-specific counters for each port:

```go
portStatistics, err := amdsmi.GetNicPortStatistics(nic, uint32(portIndex))
vendorStatistics, err := amdsmi.GetNicVendorStatistics(nic, uint32(portIndex))
```

Both APIs return a `[]amdsmi.NicStat` with `Name` and `Value` fields.

Vendor statistic names depend on the NIC, driver, and firmware.

`amdsmi.GetNicRdmaPortStatistics()` takes the zero-based position of the RDMA
port in listing order: all ports of the first RDMA device, then all ports of
the next, and so on. That index is not the `RdmaPort` field from device info,
and it is not a NIC-port list index. The example walks the RDMA device-info
listing to build the correct flattened index before calling:

```go
rdmaStatistics, err := amdsmi.GetNicRdmaPortStatistics(
	nic,
	rdmaPort.RdmaPortIndex,
)
```

The RDMA statistics result has the same name/value list shape as the other NIC
counter APIs.

### Handle unavailable telemetry and clean up

Hardware, driver, operating-system, and build differences can make individual
telemetry APIs unavailable. Only `STATUS_NOT_SUPPORTED` from the initial GPU
metrics call selects the compatibility path. After that selection, the example
uses an optional-API helper for independent calls within the chosen path:

```go
func GetOptionalAPI[T any](
	writer io.Writer,
	apiName string,
	api func() (T, error),
) (T, error) {
	result, err := api()
	if err == nil {
		return result, nil
	}
	printOptionalAPIErrorMessage(writer, apiName, err)
	var zero T
	return zero, err
}
```

The helper lives in `go/examples/example_utils`.

The helper prints clear messages for `STATUS_NOT_SUPPORTED`,
`STATUS_DRIVER_NOT_LOADED`, `STATUS_NOT_FOUND`, and `STATUS_NO_DATA`. A
compatibility sample prints `N/A` for a focused value whose query failed. A
metrics-table sample reports `metrics=N/A` if a later refresh fails; it does
not switch to the compatibility APIs after polling has started.

The program calls `amdsmi.ShutDown()` from `defer` after a successful
`amdsmi.Init()`. That releases AMD SMI resources whether the example
completes, encounters an error, or is interrupted.
