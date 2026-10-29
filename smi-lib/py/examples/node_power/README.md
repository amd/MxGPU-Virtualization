# Run the AMD SMI node power example

The node power example is a read-only AMD SMI workflow for node power
management (NPM). For every discovered GPU, it obtains the associated node
handle and uses it to read that node's NPM status, power limit, and UBB
power threshold.

`py/examples/node_power/amdsmi_node_power.py` is the Python implementation.

The example does not set a power limit or otherwise change device or platform
configuration. NPM settings are configured out of band; the example reports the
values currently exposed by the NPM controller.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. At least one AMD GPU must be available.

Node power output additionally requires a platform that exposes a node handle
for a GPU and supports NPM. On systems without those capabilities, the example
reports that the optional API is unsupported or returned no data.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/node_power/amdsmi_node_power.py
```

The reported values depend on the GPU, platform, firmware, and current NPM
configuration. If no GPUs are found, the example prints `No GPUs found` and
then shuts down AMD SMI.

## Example flow

The example initializes AMD SMI, discovers GPU processor handles, resolves the
node handle for each GPU, queries node power management, and then shuts down
AMD SMI:

```python
initialized = False

try:
    amdsmi_init(AmdSmiInitFlags.INIT_ALL_PROCESSORS)
    initialized = True

    processors = amdsmi_get_processor_handles()
    for processor_number, processor in enumerate(processors):
        print_node_power_info(processor, processor_number)
finally:
    if initialized:
        amdsmi_shut_down()
```

`AmdSmiInitFlags.INIT_ALL_PROCESSORS` is the default initialization flag. The
example passes it explicitly to show that GPU discovery happens before the
node-level query.

### Get a node handle from a GPU

NPM APIs operate on an AMD SMI node handle, not directly on a GPU processor
handle. The example first identifies the GPU by BDF, then obtains the node
handle associated with that GPU:

```python
bdf = amdsmi_get_gpu_device_bdf(processor)
node_handle = amdsmi_get_node_handle(processor)
```

A node handle is opaque: obtain it from `amdsmi_get_node_handle()` and pass it
unchanged to node-level APIs. Do not construct or modify the handle yourself.
Multiple GPUs can belong to the same node, so the example may display the same
node-level settings for more than one GPU.

This is the same node-handle lookup used by the processor discovery example.
Store the returned handle and pass it to `amdsmi_get_npm_info()`.

### Read node power management information

Pass the node handle to `amdsmi_get_npm_info()`:

```python
npm_info = amdsmi_get_npm_info(node_handle)
```

The call returns these fields:

| Field                 | Meaning                                                           |
| --------------------- | ----------------------------------------------------------------- |
| `status`              | `DISABLED` when NPM is disabled, or `ENABLED` when it is enabled. |
| `limit`               | Maximum allowed node-level power draw, in watts.                  |
| `ubb_power_threshold` | Upper UBB node power threshold, in watts.                         |

Python returns a dictionary. The script prints each value with units for the
power fields:

```python
print("  npm_info['status'] is: {}".format(npm_info['status']))
print("  npm_info['limit'] is: {} W".format(npm_info['limit']))
print("  npm_info['ubb_power_threshold'] is: {} W".format(
    npm_info['ubb_power_threshold']))
```

The example is intentionally read-only. It shows how to inspect the limits
reported by the controller, not how to configure node power management.

### Handle unavailable node power support and clean up

Node support and NPM support are optional. The example continues with the next
GPU when `amdsmi_get_node_handle()` or `amdsmi_get_npm_info()` reports either
of these conditions:

- `STATUS_NOT_SUPPORTED`
- `STATUS_NO_DATA`

These APIs raise an `AmdSmiLibraryException` for the statuses above. Other AMD
SMI errors are reported by the outer exception handler. The `initialized` flag
ensures `amdsmi_shut_down()` runs only after `amdsmi_init()` succeeds,
releasing AMD SMI resources before the script exits.
