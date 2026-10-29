# Run the AMD SMI SR-IOV VF example

The SR-IOV VF example is an interactive program for SR-IOV and virtual function
(VF) queries and configuration. It discovers AMD GPUs and offers these
operations:

- Display SR-IOV and VF information for every discovered GPU.
- Set the enabled VF count on a selected GPU.

`go/examples/sriov_vf/amdsmi_sriov_vf.go` is the Go implementation.

Run the set-VF-count operation only when it is safe to change SR-IOV
configuration on the selected GPU. Changing the enabled VF count can disrupt
workloads, and the change persists beyond this example.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. At least one AMD GPU must be available.

- To display per-VF details, the selected GPU has enabled VFs.
- To set the enabled VF count, the selected GPU supports SR-IOV and the
requested count is within the supported range.

Display is read-only. Stop or migrate workloads before choosing the set
enabled VF count operation.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/sriov_vf/amdsmi_sriov_vf.go
```

If no GPUs are discovered, the example prints `No GPUs found.` and does not
offer an operation.

## Use the interactive prompts

After initialization and GPU discovery, the example prints the number of GPUs
and presents this menu:

| Input | Operation                         | Result                                                                                      |
| ----- | --------------------------------- | ------------------------------------------------------------------------------------------- |
| `0`   | Display SR-IOV and VF information | Prints SR-IOV and per-VF details for every discovered GPU.                                  |
| `1`   | Set enabled VF count              | Lists GPUs, prompts for a GPU and VF count, then confirms before setting enabled VF count. |
| `2`   | Exit                              | Makes no change and shuts down AMD SMI.                                                     |

Enter an integer from `0` through `2`. The example repeats the prompt for an
invalid or out-of-range operation. It performs at most one operation per run.

### Display SR-IOV and VF information

Choose `0` to walk every discovered GPU and print its SR-IOV configuration.
For each GPU, the example starts with the BDF and enabled/supported VF counts:

```go
for processorNumber, processor := range processors {
	bdf, err := amdsmi.GetGpuDeviceBdf(processor)
	numEnabled, numSupported, err := amdsmi.GetNumVf(processor)
}
```

It then queries framebuffer layout and each enabled VF. For every enabled VF,
the example reports partition framebuffer
placement and calls:

```go
partitions, err := amdsmi.GetVfPartitionInfo(processor)
for vfIndex, partition := range partitions {
	vfHandle := partition.VfHandle
	amdsmi.GetVfInfo(vfHandle)
	amdsmi.GetVfData(vfHandle)
	amdsmi.GetGuestData(vfHandle)
	amdsmi.GetVfHbmInfo(vfHandle)
}
```

If a GPU has no enabled VFs, the example prints
`No enabled VFs found; per-VF APIs require an enabled VF` and skips the
per-VF queries for that GPU. Optional APIs that are not supported or return
no data are reported without aborting the run.

### Set the enabled VF count

Choose `1` to change the enabled VF count on one GPU. The example lists each
discovered GPU with an index and PCI bus:device.function (BDF) address. Enter
a GPU index to select the device:

```text
Available GPUs:
GPU 0 bdf: 0000:41:00.0
GPU 1 bdf: 0000:81:00.0
Enter the GPU index to operate on: 1
```

The GPU index must be a non-negative integer in the displayed range. The
example repeats the prompt for an invalid or out-of-range GPU index. After you
select a GPU, the example displays its BDF and the current enabled and
supported VF counts.

Enter the enabled VF count to set:

```text
Enter the enabled VF count to set: 4
```

The count must be an integer from `0` through the supported maximum returned
by `amdsmi.GetNumVf()`.

The example repeats the prompt for an invalid or out-of-range value. After you
choose a count, the example asks for an explicit confirmation:

```text
Are you sure you want to change enabled VFs from 1 to 4? (YES/no)
```

Enter `yes` to continue or `no` to cancel; either value is case-insensitive.
Any other response repeats the confirmation prompt.

When confirmed, the example calls `amdsmi.SetNumVf()` and then reads the VF
count again to verify the requested value:

```go
status := example_utils.SetOptionalAPI(
	os.Stdout,
	"amdsmi.SetNumVf",
	func() error {
		return amdsmi.SetNumVf(processor, uint32(pickNumVf))
	},
)
updatedVfCount, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNumVf",
	func() (vfCountInfo, error) {
		numEnabled, numSupported, getErr := amdsmi.GetNumVf(processor)
		if getErr != nil {
			return vfCountInfo{}, getErr
		}
		return vfCountInfo{
			numEnabled:   numEnabled,
			numSupported: numSupported,
		}, nil
	},
)
```

Changing the VF count can disrupt workloads. If the set or follow-up get call
fails, the example reports the error and continues to cleanup.

### Handle errors and cleanup

AMD SMI Go APIs return an error when an operation fails. `amdsmi.StatusError`
carries the status code; `amdsmi.StatusCodeToString()` converts it to text.
SR-IOV and VF queries and the VF-count setter use optional-API helpers in
`go/examples/example_utils` so `AMDSMI_STATUS_NOT_SUPPORTED` and
`AMDSMI_STATUS_NO_DATA` are reported without aborting the run.

The program shuts down AMD SMI from `defer` after a successful
`amdsmi.Init()`. Press `Ctrl+C` to cancel; a `signal.NotifyContext` reports
the interrupt and still runs that cleanup.

```go
if errors.Is(err, context.Canceled) {
	fmt.Println("\n  Keyboard interrupt detected. Exiting...")
	return
}

var statusErr *amdsmi.StatusError
if errors.As(err, &statusErr) {
	description, decodeErr := amdsmi.StatusCodeToString(statusErr.Code)
	if decodeErr != nil {
		fmt.Fprintln(os.Stderr, err)
	} else {
		fmt.Fprintln(os.Stderr, description)
	}
}

defer func() {
	if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
		err = fmt.Errorf("shut down AMD SMI: %w", shutdownErr)
	}
}()
```
