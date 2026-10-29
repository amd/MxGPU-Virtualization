# Run the AMD SMI reset example

The reset example is an interactive program for actions that modify GPU
state. It discovers AMD GPUs, lets you select one by index, and offers these
operations:

- Reset the selected GPU.
- Clear the framebuffer of an inactive virtual function (VF) on the selected
  GPU.

`go/examples/reset/amdsmi_reset.go` is the Go implementation.

Run this example only when it is safe to interrupt work on the selected GPU.
A GPU reset disrupts work running on that GPU. Clearing a VF framebuffer
removes its contents, and the VF must be inactive.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. At least one AMD GPU must be available.

- To clear a VF framebuffer, the selected GPU has enabled VFs and the selected
  VF is inactive.

Stop or migrate workloads from the GPU before choosing the reset operation.
The example asks for confirmation after you select the VF and before it clears
that VF's framebuffer.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/reset/amdsmi_reset.go
```

If no GPUs are discovered, the example prints `No GPUs found` and does not
offer an operation.

## Use the interactive prompts

The example lists each discovered GPU with an index and PCI bus:device.function
(BDF) address. Enter a GPU index to select the device:

```text
Available GPUs:
GPU 0 bdf: 0000:41:00.0
GPU 1 bdf: 0000:81:00.0
Enter the GPU index to operate on: 1
```

The GPU index must be a non-negative integer in the displayed range. The
example repeats the prompt for an invalid or out-of-range GPU index.

After you select a GPU, the example displays its BDF and presents this menu:

| Input | Operation            | Result                                                                                      |
| ----- | -------------------- | ------------------------------------------------------------------------------------------- |
| `0`   | Reset GPU            | Prompts for confirmation before sending the reset request.                                 |
| `1`   | Clear VF framebuffer | Lists the selected GPU's VFs and prompts for confirmation after you select an inactive VF. |
| `2`   | Exit                 | Makes no change and shuts down AMD SMI.                                                     |

Enter an integer from `0` through `2`. The example repeats the prompt for an
invalid or out-of-range operation. It performs at most one operation per run.

### Reset the selected GPU

Choose `0` to call `amdsmi.ResetGpu()` with the selected GPU handle:

```go
processor := processors[pickGpuIndex]
status := example_utils.SetOptionalAPI(
	os.Stdout,
	"amdsmi.ResetGpu",
	func() error {
		return amdsmi.ResetGpu(processor)
	},
)
```

The example asks for an explicit confirmation:

```text
Are you sure you want to reset the GPU? (YES/no)
```

Enter `yes` to continue or `no` to cancel; either value is case-insensitive.
Any other response repeats the confirmation prompt. When confirmed, the
example sends the reset request and then waits 20 seconds before shutting down
AMD SMI. The wait does not perform an additional health check.

```go
if status == amdsmi.AMDSMI_STATUS_SUCCESS {
	fmt.Println("  Waiting 20 seconds for GPU reset to complete")
	time.Sleep(20 * time.Second)
}
```

Go waits only after `amdsmi.ResetGpu()` reports success. The helper lives in
`go/examples/example_utils`.

### Clear an inactive VF framebuffer

Choose `1` to call `amdsmi.GetVfPartitionInfo()` for the selected GPU. If VFs
are available, the example calls `amdsmi.GetVfData()` for each VF and lists
its index, BDF address, and scheduler state:

```text
Available VFs:
VF 0 bdf: 0000:41:00.1 (inactive)
VF 1 bdf: 0000:42:00.1 (active)
Enter the VF index to operate on: 0
Selected VF 0 bdf: 0000:41:00.1 (inactive)
Are you sure you want to clear the VF framebuffer? (YES/no)
```

`AVAILABLE` and `DEFAULT_AVAILABLE` scheduler states are displayed as
`inactive`; other states are displayed by name. The example checks the selected
VF's state again before asking for confirmation. The VF prompt accepts only
indexes whose reported state is inactive. Selecting an active VF reports that
the VF needs to be inactive and repeats the prompt.

For an inactive VF, enter `yes` to clear the framebuffer or `no` to cancel;
either value is case-insensitive. Any other response repeats the confirmation
prompt.

The example calls `amdsmi.ClearVfFb()` only after confirmation:

```go
validVfIndexes := make([]uint64, 0, len(partitions))
for vfIndex, partition := range partitions {
	vfHandle := partition.VfHandle
	vfData, dataErr := example_utils.GetOptionalAPI(
		os.Stdout,
		fmt.Sprintf("amdsmi.GetVfData for VF %d", vfIndex),
		func() (amdsmi.VfData, error) {
			return amdsmi.GetVfData(vfHandle)
		},
	)
	if dataErr != nil {
		continue
	}
	if isInactiveVfState(vfData.Sched.State) {
		validVfIndexes = append(validVfIndexes, uint64(vfIndex))
	}
}

pickVfIndex, err := prompter.PromptForInput(
	ctx,
	"  Enter the VF index to operate on: ",
	example_utils.InputOptions{
		DefaultValue: validVfIndexes[0],
		ValidValues:  validVfIndexes,
	},
)
if err != nil {
	return err
}

vfHandle := partitions[pickVfIndex].VfHandle
vfData, dataErr := amdsmi.GetVfData(vfHandle)
if dataErr != nil {
	return nil
}
if !isInactiveVfState(vfData.Sched.State) {
	fmt.Println("The selected VF needs to be inactive.")
} else {
	confirmed, err := prompter.PromptConfirmation(
		ctx,
		"  Are you sure you want to clear the VF framebuffer?",
	)
	if err != nil {
		return err
	}
	if !confirmed {
		fmt.Println("  No operation was performed. Exiting...")
	} else {
		example_utils.SetOptionalAPI(
			os.Stdout,
			"amdsmi.ClearVfFb",
			func() error {
				return amdsmi.ClearVfFb(vfHandle)
			},
		)
	}
}
```

If the selected GPU has no VFs, the example prints `No VFs found`. If none of
its VFs are inactive, it prints `No inactive VFs found` and does not prompt
for a VF index. Enter a whole-number index for an inactive VF; any other input
is rejected and prompted again.

### Handle errors and cleanup

AMD SMI Go APIs return an error when an operation fails. `amdsmi.StatusError`
carries the status code; `amdsmi.StatusCodeToString()` converts it to text.
VF queries and the reset and framebuffer-clear setters use optional-API
helpers in `go/examples/example_utils` so unsupported or missing-data
conditions are reported without aborting the run.

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
