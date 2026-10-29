# Run the AMD SMI node power example

The node power example is a read-only AMD SMI workflow for node power
management (NPM). For every discovered GPU, it obtains the associated node
handle and uses it to read that node's NPM status, power limit, and UBB
power threshold.

`go/examples/node_power/amdsmi_node_power.go` is the Go implementation.

The example does not set a power limit or otherwise change device or platform
configuration. NPM settings are configured out of band; the example reports the
values currently exposed by the NPM controller.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. At least one AMD GPU must be available.

Node power output additionally requires a platform that exposes a node handle
for a GPU and supports NPM. On systems without those capabilities, the example
reports that the optional API is unsupported or returned no data.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/node_power/amdsmi_node_power.go
```

The reported values depend on the GPU, platform, firmware, and current NPM
configuration. If no GPUs are found, the example prints `No GPUs found` and
then shuts down AMD SMI.

## Example flow

The example initializes AMD SMI, discovers GPU processor handles, resolves the
node handle for each GPU, queries node power management, and then shuts down
AMD SMI:

```go
if err := amdsmi.Init(amdsmi.AMDSMI_INIT_ALL_PROCESSORS); err != nil {
	return err
}
defer func() {
	if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
		fmt.Println(shutdownErr)
	}
}()

processors, err := amdsmi.GetProcessorHandles()
if err != nil {
	return err
}
for processorNumber, processor := range processors {
	printNodePowerInfo(processor, processorNumber)
}
```

`amdsmi.AMDSMI_INIT_ALL_PROCESSORS` is the default initialization flag. The
example passes it explicitly to show that GPU discovery happens before the
node-level query.

### Get a node handle from a GPU

NPM APIs operate on an AMD SMI node handle, not directly on a GPU processor
handle. The example first identifies the GPU by BDF, then obtains the node
handle associated with that GPU:

```go
bdf, err := amdsmi.GetGpuDeviceBdf(processor)
if err != nil {
	return err
}
nodeHandle, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNodeHandle",
	func() (amdsmi.NodeHandle, error) {
		return amdsmi.GetNodeHandle(processor)
	},
)
```

A node handle is opaque: obtain it from `amdsmi.GetNodeHandle()` and pass it
unchanged to node-level APIs. Do not construct or modify the handle yourself.
Multiple GPUs can belong to the same node, so the example may display the same
node-level settings for more than one GPU.

This is the same node-handle lookup used by the processor discovery example.
Store the returned handle and pass it to `amdsmi.GetNpmInfo()`.

### Read node power management information

Pass the node handle to `amdsmi.GetNpmInfo()`:

```go
npmInfo, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetNpmInfo",
	func() (amdsmi.NpmInfo, error) {
		return amdsmi.GetNpmInfo(nodeHandle)
	},
)
```

The call returns these fields:

| Field                 | Meaning                                                           |
| --------------------- | ----------------------------------------------------------------- |
| `status`              | `DISABLED` when NPM is disabled, or `ENABLED` when it is enabled. |
| `limit`               | Maximum allowed node-level power draw, in watts.                  |
| `ubb_power_threshold` | Upper UBB node power threshold, in watts.                         |

Go returns an `amdsmi.NpmInfo` struct. The program prints each value with
units for the power fields:

```go
fmt.Printf("  npm_info['status'] is: %s\n", npmInfo.Status)
fmt.Printf("  npm_info['limit'] is: %d W\n", npmInfo.Limit)
fmt.Printf(
	"  npm_info['ubb_power_threshold'] is: %d W\n",
	npmInfo.UbbPowerThreshold,
)
```

The example is intentionally read-only. It shows how to inspect the limits
reported by the controller, not how to configure node power management.

### Handle unavailable node power support and clean up

Node support and NPM support are optional. The example continues with the next
GPU when `amdsmi.GetNodeHandle()` or `amdsmi.GetNpmInfo()` reports either of
these conditions:

- `AMDSMI_STATUS_NOT_SUPPORTED`
- `AMDSMI_STATUS_NO_DATA`

These APIs return an `amdsmi.StatusError` for the statuses above. The example
uses `example_utils.GetOptionalAPI()` so optional failures are printed without
aborting the run, matching the node-handle lookup in the processor discovery
example. Other AMD SMI errors are returned from `run()`. Shutdown is placed in
a `defer` so `amdsmi.ShutDown()` runs after a successful `amdsmi.Init()`,
releasing AMD SMI resources before the program exits.
