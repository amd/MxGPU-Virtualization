# Run the AMD SMI firmware example

The firmware example lists the available AMD GPUs, prompts you to select one or
all of them, and inspects their firmware-related information. It reports VBIOS
identity, the GPU firmware inventory, optional host-only firmware error
records, per-VF firmware when SR-IOV VFs are enabled, and optional NIC
firmware. It also lists available NICs and prompts you to select one or all of
them before reporting their firmware.

`go/examples/firmware/amdsmi_firmware.go` is the Go implementation.

The example is read-only: it does not update firmware, reset a GPU, create or
remove VFs, or otherwise change device configuration.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. An AMD GPU is required for GPU firmware output.

Firmware error records are a host-only optional capability. VF firmware queries
require a GPU with enabled SR-IOV VFs. NIC firmware requires an AMD SMI build
with NIC support and an appropriate NIC driver. The example reports unavailable
optional capabilities and continues with the remaining queries.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/firmware/amdsmi_firmware.go
```

The reported firmware versions and optional capabilities depend on the host
hardware, driver, and firmware. The example prompts for a valid GPU index or
the trailing `All GPUs` option after listing GPU BDFs. It then prompts for a
NIC index or the trailing `All NICs` option after listing NIC BDFs when NIC
firmware is available. If no GPUs are available, the example prints
`No GPUs found` before checking optional NIC firmware.

## Example flow

The example initializes AMD SMI, discovers GPU processor handles, prompts for a
GPU selection, runs firmware queries for the selected GPU or all GPUs, prompts
for one or all optional NICs, and shuts down AMD SMI:

```go
if err = amdsmi.Init(amdsmi.AMDSMI_INIT_ALL_PROCESSORS); err != nil {
	return err
}
defer func() {
	if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
		err = shutdownErr
	}
}()

processors, err := amdsmi.GetProcessorHandles()
if err != nil {
	return err
}
for gpuIndex, processor := range processors {
	bdf, err := amdsmi.GetGpuDeviceBdf(processor)
	if err != nil {
		return err
	}
	fmt.Printf("GPU %d bdf: %s\n", gpuIndex, bdf)
}

allGpuIndex := uint64(len(processors))
fmt.Printf("%d: All GPUs\n", allGpuIndex)
pickGpuIndex := ...
selectedGpuIndexes := ...
for _, gpuIndex := range selectedGpuIndexes {
	if err := printGpuFirmware(gpuIndex, processors[gpuIndex]); err != nil {
		return err
	}
}

return printNicFirmware(ctx, prompter)
```

`amdsmi.AMDSMI_INIT_ALL_PROCESSORS` is passed explicitly so the example can
discover GPUs and optional NICs. The current native implementation does not use
this flag to restrict the discovery scope.

`amdsmi.GetGpuDeviceBdf()` identifies each available GPU in the output with its
PCI bus:device.function (BDF) address. The example validates the selected
index, including the trailing `All GPUs` option, then passes each selected
processor handle to the remaining GPU queries.

### Read VBIOS information

`amdsmi.GetGpuVbiosInfo()` returns static VBIOS identity fields for a GPU:

```go
vbiosInfo, err := amdsmi.GetGpuVbiosInfo(processor)
if err != nil {
	return err
}
fmt.Println(vbiosInfo.Name)
fmt.Println(vbiosInfo.BuildDate)
fmt.Println(vbiosInfo.PartNumber)
fmt.Println(vbiosInfo.Version)
fmt.Println(vbiosInfo.BootFirmware)
```

The returned `amdsmi.VbiosInfo` contains:

| Field          | Meaning                   |
| -------------- | ------------------------- |
| `Name`         | VBIOS name                |
| `BuildDate`    | VBIOS build date          |
| `PartNumber`   | VBIOS part number         |
| `Version`      | VBIOS version string      |
| `BootFirmware` | Boot firmware information |

This is VBIOS identity data for the GPU, not the AMD SMI library version.

### Read GPU firmware blocks

`amdsmi.GetFwInfo()` returns the firmware inventory for a GPU:

The result is an `amdsmi.FwInfo` whose `NumFwInfo` field is the number of valid
entries in `FwList`:

```go
fwInfo, err := amdsmi.GetFwInfo(processor)
if err != nil {
	return err
}
for i := 0; i < int(fwInfo.NumFwInfo); i++ {
	firmware := fwInfo.FwList[i]
	fmt.Println(firmware.FwID)
	fmt.Println(firmware.FwVersion)
}
```

Each `amdsmi.FwInfoList` entry contains:

| Field       | Meaning                                      |
| ----------- | -------------------------------------------- |
| `FwID`      | Firmware block identifier (`amdsmi.FwBlock`) |
| `FwVersion` | Firmware version as an integer               |

A `NumFwInfo` of `0` means the API found no firmware blocks to report.

The example omits zero-valued entries and displays the remaining versions in
the block-specific dotted or hexadecimal formats used by `amd-smi`.

### Read firmware error records

Firmware error records are a host-only optional feature. The example calls
`amdsmi.GetFwErrorRecords()` and reports when the capability is unavailable:

```go
errorRecords, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetFwErrorRecords",
	func() (amdsmi.FwErrorRecord, error) {
		return amdsmi.GetFwErrorRecords(processor)
	},
)
if err == nil {
	for recordNumber := 0; recordNumber < int(errorRecords.NumErrRecords); recordNumber++ {
		record := errorRecords.ErrRecords[recordNumber]
		fmt.Println(record.Timestamp)
		fmt.Println(record.VfIdx)
		fmt.Println(amdsmi.FwBlock(record.FwID))
		fmt.Println(record.Status)
	}
}
```

Each `amdsmi.FwLoadErrorRecord` in `ErrRecords` contains:

| Field       | Meaning                                          |
| ----------- | ------------------------------------------------ |
| `Timestamp` | System time in seconds                           |
| `VfIdx`     | Virtual function index associated with the event |
| `FwID`      | Firmware block identifier (`amdsmi.FwBlock`)     |
| `Status`    | Firmware load status                             |

A `NumErrRecords` of `0` is a normal result when no firmware error records are
present.

### Read VF firmware

VF firmware queries require enabled SR-IOV VFs. The example first reads the
current VF partition list with `amdsmi.GetVfPartitionInfo()`:

```go
partitions, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetVfPartitionInfo",
	func() ([]amdsmi.PartitionInfo, error) {
		return amdsmi.GetVfPartitionInfo(processor)
	},
)
```

If no enabled VFs are available, it explains that VF firmware queries need an
enabled VF. For each partition on each selected GPU, it passes the VF handle to
`amdsmi.GetVfFwInfo()`:

```go
fwInfo, err := amdsmi.GetVfFwInfo(partition.VfHandle)
if err != nil {
	return err
}
for i := 0; i < int(fwInfo.NumFwInfo); i++ {
	firmware := fwInfo.FwList[i]
	fmt.Println(firmware.FwID)
	fmt.Println(firmware.FwVersion)
}
```

The VF result uses the same firmware-inventory shape as the GPU firmware
query. Firmware information is expected for an active VF; in practice, this is
after a guest is running on the VF and its GPU driver has loaded. An empty
inventory can mean the VF is not active or has no VF firmware inventory
available. Treat this section as optional: unsupported or unavailable VF APIs
are reported without stopping the GPU firmware flow.

### Read NIC firmware

NIC support is optional and is handled after the selected GPU section(s). The
example first requests NIC processor handles, lists their BDF addresses, and
prompts you to select one or the trailing `All NICs` option:

```go
nicProcessors, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNicProcessorHandles",
	amdsmi.GetNicProcessorHandles,
)
allNicIndex := uint64(len(nicProcessors))
fmt.Printf("%d: All NICs\n", allNicIndex)
pickNicIndex := ...
selectedNicIndexes := ...
```

The `All NICs` option uses index immediately after the last NIC index.

For each selected NIC handle, the example calls `amdsmi.GetNicFwInfo()`:

```go
for _, nicIndex := range selectedNicIndexes {
	nic := nicProcessors[nicIndex]
	fwInfo, err := amdsmi.GetNicFwInfo(nic)
	if err != nil {
		return err
	}
	for _, firmware := range fwInfo.Fw {
		fmt.Println(firmware.Type)
		fmt.Println(firmware.Fw.Name)
		fmt.Println(firmware.Fw.Version)
	}
}
```

Each `amdsmi.NicFwEntry` contains:

| Field        | Meaning                                           |
| ------------ | ------------------------------------------------- |
| `Type`       | Firmware version type (`amdsmi.NicFwVersionType`) |
| `Fw.Name`    | Firmware component name                           |
| `Fw.Version` | Firmware version string                           |

`amdsmi.GetNicFwInfo()` may also depend on system libraries such as `libmnl`.
When NIC discovery or firmware queries are unavailable, the example reports
that and exits the NIC section cleanly.

### Handle unavailable capabilities and clean up

Optional firmware error record, VF, and NIC APIs can fail when the hardware,
driver, or build configuration does not support them. The example handles these
status codes without terminating the GPU firmware flow:

- `AMDSMI_STATUS_NOT_SUPPORTED`
- `AMDSMI_STATUS_DRIVER_NOT_LOADED`
- `AMDSMI_STATUS_NOT_FOUND`
- `AMDSMI_STATUS_NO_DATA`

These APIs return an `amdsmi.StatusError` for the statuses above. The example
uses `example_utils.GetOptionalAPI()` so optional failures are printed without
aborting the run. Other AMD SMI errors are returned from `run()`. Shutdown is
placed in a `defer` so `amdsmi.ShutDown()` runs after a successful
`amdsmi.Init()`, releasing AMD SMI resources before the program exits.
