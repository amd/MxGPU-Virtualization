# Run the AMD SMI processor discovery example

The processor discovery example is a read-only introduction to AMD SMI
processor handles. It lists available AMD GPUs and prompts you to select one or
all of them, then shows how to identify each selected GPU by processor type,
index, PCI bus:device.function (BDF) address, and UUID. It also demonstrates
optional node and virtual function (VF) discovery for each selected GPU. When
supported, the example separately lists available NICs, prompts you to select
one or all of them, and demonstrates NIC discovery.

`go/examples/discovery/amdsmi_discovery.go` is the Go implementation.

The example does not reset a GPU, create or remove VFs, or otherwise change
device configuration.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. An AMD GPU is required for GPU discovery output.

NIC discovery additionally requires an AMD SMI build with NIC support and an
appropriate NIC driver. VF discovery requires a GPU with enabled SR-IOV VFs.
Node discovery depends on the GPU and platform exposing a node handle. The
example reports unavailable optional capabilities and continues with the
remaining discovery steps.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/discovery/amdsmi_discovery.go
```

The available processors, BDF addresses, UUIDs, and optional capabilities
depend on the host hardware, driver, and firmware. The example prompts for a
valid GPU index or the trailing `All GPUs` option after listing GPU BDFs. It
then prompts for a NIC index or the trailing `All NICs` option after listing
NIC BDFs when NIC discovery is available. If no GPUs are available, the
example prints `No GPUs found` before checking optional NIC discovery.

## Example flow

The example initializes AMD SMI, obtains GPU processor handles, lists their
BDF addresses, and performs identity lookups and optional node and VF
capability checks for the selected GPU or all GPUs. It then lists optional NICs
and performs NIC discovery for the selected NIC or all NICs before shutting
down AMD SMI:

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
	if err := printGpuDiscovery(gpuIndex, processors[gpuIndex]); err != nil {
		return err
	}
}

return printNicDiscovery(ctx, prompter)
```

`amdsmi.AMDSMI_INIT_ALL_PROCESSORS` is the default initialization flag. The
example passes it explicitly to show that it intends to discover all supported
processor types. The current native implementation does not use this flag to
restrict the discovery scope.

### Select a GPU processor handle

`amdsmi.GetProcessorHandles()` returns AMD GPU processor handles:

```go
processors, err := amdsmi.GetProcessorHandles()
if err != nil {
	return err
}
```

A processor handle is an opaque value owned by AMD SMI. Do not create one
yourself. Keep the returned handle and pass it to APIs that query that GPU.

The example prints the BDF of every available GPU and prompts for a valid GPU
index or the trailing `All GPUs` option. It performs the remaining GPU
discovery for the selected handle or for every GPU handle.

The example also calls `amdsmi.GetProcessorHandlesByType()` with the AMD GPU
processor type:

```go
gpuProcessors, err := amdsmi.GetProcessorHandlesByType(
	amdsmi.AMDSMI_PROCESSOR_TYPE_AMD_GPU,
)
if err != nil {
	return err
}
```

This is useful when an application needs handles for a specific processor
class. The same API is used later for `AMDSMI_PROCESSOR_TYPE_AMD_NIC` and
`AMDSMI_PROCESSOR_TYPE_BRCM_NIC` processor types when NIC discovery is
available.

For each selected GPU handle, the example calls:

- `amdsmi.GetProcessorType()` to identify the handle's processor type.
- `amdsmi.GetIndexFromProcessorHandle()` to obtain the GPU index.
- `amdsmi.GetGpuDeviceBdf()` to obtain the GPU-specific BDF address.
- `amdsmi.GetProcessorBdf()` to obtain the BDF through the generic processor
  API.
- `amdsmi.GetGpuDeviceUuid()` to obtain the GPU UUID.

The BDF is printed in the form `domain:bus:device.function`, such as
`0000:41:00.0`. It is a stable way to identify a PCI device on the current
host. The GPU index is an AMD SMI enumeration index; it is not a PCI address.

### Resolve a GPU handle by index, BDF, or UUID

AMD SMI provides three ways to resolve an existing GPU handle:

```go
processorFromIndex, err := amdsmi.GetProcessorHandleFromIndex(processorIndex)
if err != nil {
	return err
}
processorFromBdf, err := amdsmi.GetProcessorHandleFromBdf(gpuBdf)
if err != nil {
	return err
}
processorFromUuid, err := amdsmi.GetProcessorHandleFromUuid(gpuUUID)
if err != nil {
	return err
}
```

The example validates each lookup by querying the returned handle again:

```go
bdfFromIndex, err := amdsmi.GetGpuDeviceBdf(processorFromIndex)
if err != nil {
	return err
}
indexFromBdf, err := amdsmi.GetIndexFromProcessorHandle(processorFromBdf)
if err != nil {
	return err
}
indexFromUUID, err := amdsmi.GetIndexFromProcessorHandle(processorFromUUID)
if err != nil {
	return err
}
```

`amdsmi.GetProcessorHandleFromIndex()` accepts GPU indexes only.
`amdsmi.GetProcessorHandleFromBdf()` is type-agnostic and can resolve a GPU or
supported NIC handle. `amdsmi.GetProcessorHandleFromUuid()` is for GPU UUIDs.

### Discover a node handle

Some platforms associate GPUs with a node handle. The example calls
`amdsmi.GetNodeHandle()` for each selected GPU:

```go
nodeHandle, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNodeHandle",
	func() (amdsmi.NodeHandle, error) {
		return amdsmi.GetNodeHandle(processor)
	},
)
```

The returned node handle is opaque. Store it and pass it to node-level APIs,
such as `amdsmi.GetNpmInfo()`. Node support is platform-dependent, so the
example reports an unsupported or unavailable node without stopping GPU
discovery.

### Discover virtual functions

VF lookups require enabled VFs. The example first determines how many VFs are
enabled on each selected GPU:

```go
numEnabled, numSupported, err := amdsmi.GetNumVf(processor)
if err != nil {
	return err
}
for vfIndex := uint32(0); vfIndex < numEnabled; vfIndex++ {
	vfHandle, err := amdsmi.GetVfHandleFromVfIndex(processor, int(vfIndex))
	if err != nil {
		return err
	}
}
```

For every enabled VF, the example demonstrates all three VF lookup paths:

```go
vfBdf, err := amdsmi.GetVfBdf(vfHandle)
if err != nil {
	return err
}
vfUUID, err := amdsmi.GetVfUuid(vfHandle)
if err != nil {
	return err
}

vfFromBdf, err := amdsmi.GetVfHandleFromBdf(vfBdf)
if err != nil {
	return err
}
vfFromUUID, err := amdsmi.GetVfHandleFromUuid(vfUUID)
if err != nil {
	return err
}
```

The example then reads the UUID from the BDF lookup handle and the BDF from
the UUID lookup handle. This verifies that the BDF and UUID both identify the
same VF. If a selected GPU has no enabled VFs, the example explains that the
VF lookup calls require an enabled VF and continues with the next GPU or NIC
discovery.

### Discover NIC processor handles

NIC support is optional and is handled in a separate section. The example
first requests all available NIC processor handles:

```go
nicProcessors, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNicProcessorHandles",
	amdsmi.GetNicProcessorHandles,
)
```

When successful, it lists the count for AMD and Broadcom NIC processor types:

```go
amdNics, err := amdsmi.GetProcessorHandlesByType(
	amdsmi.AMDSMI_PROCESSOR_TYPE_AMD_NIC,
)
if err != nil {
	return err
}
brcmNics, err := amdsmi.GetProcessorHandlesByType(
	amdsmi.AMDSMI_PROCESSOR_TYPE_BRCM_NIC,
)
if err != nil {
	return err
}
```

The example prints the BDF of every available NIC and prompts for a valid NIC
index or the trailing `All NICs` option at the index equal to the NIC count.
For each selected NIC handle, it gets its type, type-specific index, NIC BDF,
and generic processor BDF:

```go
nicType, err := amdsmi.GetProcessorType(nic)
if err != nil {
	return err
}
nicIndex, err := amdsmi.GetIndexFromProcessorHandle(nic)
if err != nil {
	return err
}
nicBdf, err := amdsmi.GetNicDeviceBdf(nic)
if err != nil {
	return err
}
processorBdf, err := amdsmi.GetProcessorBdf(nic)
if err != nil {
	return err
}
```

The NIC is resolved again with `amdsmi.GetProcessorHandleFromBdf()`. There is
no NIC equivalent of `amdsmi.GetProcessorHandleFromIndex()`, so use the BDF
lookup when an application needs to resolve a NIC handle.

### Handle unavailable capabilities and clean up

Optional node, VF, and NIC APIs can fail when the hardware, driver, or build
configuration does not support them. The example handles these status codes
without terminating the selected GPU discovery flows:

- `AMDSMI_STATUS_NOT_SUPPORTED`
- `AMDSMI_STATUS_DRIVER_NOT_LOADED`
- `AMDSMI_STATUS_NOT_FOUND`
- `AMDSMI_STATUS_NO_DATA`

These APIs return an `amdsmi.StatusError` for the statuses above. The example
uses `example_utils.GetOptionalAPI()` and
`example_utils.PrintOptionalAPIError()` so optional failures are printed
without aborting the run. Other AMD SMI errors are returned from `run()`.
Shutdown is placed in a `defer` so `amdsmi.ShutDown()` runs after a successful
`amdsmi.Init()`, releasing AMD SMI resources before the program exits.
