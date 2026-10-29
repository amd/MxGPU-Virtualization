# Run the AMD SMI processor discovery example

The processor discovery example is a read-only introduction to AMD SMI
processor handles. It lists available AMD GPUs and prompts you to select one or
all of them, then shows how to identify each selected GPU by processor type,
index, PCI bus:device.function (BDF) address, and UUID. It also demonstrates
optional node and virtual function (VF) discovery for each selected GPU. When
supported, the example separately lists available NICs, prompts you to select
one or all of them, and demonstrates NIC discovery.

`py/examples/discovery/amdsmi_discovery.py` is the Python implementation.

The example does not reset a GPU, create or remove VFs, or otherwise change
device configuration.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. An AMD GPU is required for GPU discovery output.

NIC discovery additionally requires an AMD SMI build with NIC support and an
appropriate NIC driver. VF discovery requires a GPU with enabled SR-IOV VFs.
Node discovery depends on the GPU and platform exposing a node handle. The
example reports unavailable optional capabilities and continues with the
remaining discovery steps.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/discovery/amdsmi_discovery.py
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

```python
initialized = False

try:
    amdsmi_init(AmdSmiInitFlags.INIT_ALL_PROCESSORS)
    initialized = True

    processors = amdsmi_get_processor_handles()
    for i in range(len(processors)):
        print(amdsmi_get_gpu_device_bdf(processors[i]))

    all_gpu_index = len(processors)
    print(all_gpu_index, "All GPUs")
    pick_gpu_index = ...
    selected_gpu_indexes = (
        range(len(processors))
        if pick_gpu_index == all_gpu_index else [pick_gpu_index])
    for gpu_index in selected_gpu_indexes:
        processor = processors[gpu_index]
        print_gpu_discovery(gpu_index, processor)

    print_nic_discovery()
finally:
    if initialized:
        amdsmi_shut_down()
```

`AmdSmiInitFlags.INIT_ALL_PROCESSORS` is the default initialization flag. The
example passes it explicitly to show that it intends to discover all supported
processor types. The current native implementation does not use this flag to
restrict the discovery scope.

### Select a GPU processor handle

`amdsmi_get_processor_handles()` returns AMD GPU processor handles:

```python
processors = amdsmi_get_processor_handles()
```

A processor handle is an opaque value owned by AMD SMI. Do not create one
yourself. Keep the returned handle and pass it to APIs that query that GPU.

The example prints the BDF of every available GPU and prompts for a valid GPU
index or the trailing `All GPUs` option. It performs the remaining GPU
discovery for the selected handle or for every GPU handle.

The example also calls `amdsmi_get_processor_handles_by_type()` with the AMD
GPU processor type:

```python
gpu_processors = amdsmi_get_processor_handles_by_type(
    AmdSmiProcessorType.AMD_GPU)
```

This is useful when an application needs handles for a specific processor
class. The same API is used later for `AMD_NIC` and `BRCM_NIC` processor types
when NIC discovery is available.

For each selected GPU handle, the example calls:

- `amdsmi_get_processor_type()` to identify the handle's processor type.
- `amdsmi_get_index_from_processor_handle()` to obtain the GPU index.
- `amdsmi_get_gpu_device_bdf()` to obtain the GPU-specific BDF address.
- `amdsmi_get_processor_bdf()` to obtain the BDF through the generic processor
  API.
- `amdsmi_get_gpu_device_uuid()` to obtain the GPU UUID.

The BDF is printed in the form `domain:bus:device.function`, such as
`0000:41:00.0`. It is a stable way to identify a PCI device on the current
host. The GPU index is an AMD SMI enumeration index; it is not a PCI address.

### Resolve a GPU handle by index, BDF, or UUID

AMD SMI provides three ways to resolve an existing GPU handle:

```python
processor_from_index = amdsmi_get_processor_handle_from_index(
    processor_index)
processor_from_bdf = amdsmi_get_processor_handle_from_bdf(gpu_bdf)
processor_from_uuid = amdsmi_get_processor_handle_from_uuid(gpu_uuid)
```

The example validates each lookup by querying the returned handle again:

```python
print(amdsmi_get_gpu_device_bdf(processor_from_index))
print(amdsmi_get_index_from_processor_handle(processor_from_bdf))
print(amdsmi_get_index_from_processor_handle(processor_from_uuid))
```

`amdsmi_get_processor_handle_from_index()` accepts GPU indexes only.
`amdsmi_get_processor_handle_from_bdf()` is type-agnostic and can resolve a
GPU or supported NIC handle. `amdsmi_get_processor_handle_from_uuid()` is for
GPU UUIDs.

### Discover a node handle

Some platforms associate GPUs with a node handle. The example calls
`amdsmi_get_node_handle()` for each selected GPU:

```python
node_handle = amdsmi_get_node_handle(processor)
```

The returned node handle is opaque. Store it and pass it to node-level APIs,
such as `amdsmi_get_npm_info()`. Node support is platform-dependent, so the
example reports an unsupported or unavailable node without stopping GPU
discovery.

### Discover virtual functions

VF lookups require enabled VFs. The example first determines how many VFs are
enabled on each selected GPU:

```python
vf_count = amdsmi_get_num_vf(processor)
for vf_index in range(vf_count['num_vf_enabled']):
    vf_handle = amdsmi_get_vf_handle_from_vf_index(
        processor, vf_index)
```

For every enabled VF, the example demonstrates all three VF lookup paths:

```python
vf_bdf = amdsmi_get_vf_bdf(vf_handle)
vf_uuid = amdsmi_get_vf_uuid(vf_handle)

vf_from_bdf = amdsmi_get_vf_handle_from_bdf(vf_bdf)
vf_from_uuid = amdsmi_get_vf_handle_from_uuid(vf_uuid)
```

The example then reads the UUID from the BDF lookup handle and the BDF from
the UUID lookup handle. This verifies that the BDF and UUID both identify the
same VF. If a selected GPU has no enabled VFs, the example explains that the
VF lookup calls require an enabled VF and continues with the next GPU or NIC
discovery.

### Discover NIC processor handles

NIC support is optional and is handled in a separate section. The example
first requests all available NIC processor handles:

```python
nic_processors = amdsmi_get_nic_processor_handles()
```

When successful, it lists the count for AMD and Broadcom NIC processor types:

```python
amd_nics = amdsmi_get_processor_handles_by_type(
    AmdSmiProcessorType.AMD_NIC)
brcm_nics = amdsmi_get_processor_handles_by_type(
    AmdSmiProcessorType.BRCM_NIC)
```

The example prints the BDF of every available NIC and prompts for a valid NIC
index or the trailing `All NICs` option at the index equal to the NIC count.
For each selected NIC handle, it gets its type, type-specific index, NIC BDF,
and generic processor BDF:

```python
nic_type = amdsmi_get_processor_type(nic)
nic_index = amdsmi_get_index_from_processor_handle(nic)
nic_bdf = amdsmi_get_nic_device_bdf(nic)
processor_bdf = amdsmi_get_processor_bdf(nic)
```

The NIC is resolved again with `amdsmi_get_processor_handle_from_bdf()`. There
is no NIC equivalent of `amdsmi_get_processor_handle_from_index()`, so use the
BDF lookup when an application needs to resolve a NIC handle.

### Handle unavailable capabilities and clean up

Optional node, VF, and NIC APIs can fail when the hardware, driver, or build
configuration does not support them. The example handles these status codes
without terminating the selected GPU discovery flows:

- `STATUS_NOT_SUPPORTED`
- `STATUS_DRIVER_NOT_LOADED`
- `STATUS_NOT_FOUND`
- `STATUS_NO_DATA`

These APIs raise an `AmdSmiLibraryException` for the statuses above. Other AMD
SMI errors are reported by the outer exception handler. The `initialized` flag
ensures `amdsmi_shut_down()` is called only after `amdsmi_init()` succeeds,
releasing AMD SMI resources before the script exits.
