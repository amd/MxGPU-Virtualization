# Run the AMD SMI firmware example

The firmware example lists the available AMD GPUs, prompts you to select one or
all of them, and inspects their firmware-related information. It reports VBIOS
identity, the GPU firmware inventory, optional host-only firmware error
records, per-VF firmware when SR-IOV VFs are enabled, and optional NIC
firmware. It also lists available NICs and prompts you to select one or all of
them before reporting their firmware.

`py/examples/firmware/amdsmi_firmware.py` is the Python implementation.

The example is read-only: it does not update firmware, reset a GPU, create or
remove VFs, or otherwise change device configuration.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. An AMD GPU is required for GPU firmware output.

Firmware error records are a host-only optional capability. VF firmware queries
require a GPU with enabled SR-IOV VFs. NIC firmware requires an AMD SMI build
with NIC support and an appropriate NIC driver. The example reports unavailable
optional capabilities and continues with the remaining queries.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/firmware/amdsmi_firmware.py
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

```python
initialized = False

try:
    amdsmi_init(AmdSmiInitFlags.INIT_ALL_PROCESSORS)
    initialized = True

    processors = amdsmi_get_processor_handles()
    for index, processor in enumerate(processors):
        print(index, amdsmi_get_gpu_device_bdf(processor))

    all_gpu_index = len(processors)
    print(all_gpu_index, "All GPUs")
    selected_index = int(input("Enter the GPU index to inspect: "))
    selected_gpu_indexes = (
        range(len(processors))
        if selected_index == all_gpu_index else [selected_index])
    for gpu_index in selected_gpu_indexes:
        processor = processors[gpu_index]
        vbios_info = amdsmi_get_gpu_vbios_info(processor)
        fw_info = amdsmi_get_fw_info(processor)
        # Query optional firmware error records and VF firmware.
    print_nic_firmware()
finally:
    if initialized:
        amdsmi_shut_down()
```

`AmdSmiInitFlags.INIT_ALL_PROCESSORS` is passed explicitly so the example can
discover GPUs and optional NICs. The current native implementation does not use
this flag to restrict the discovery scope.

`amdsmi_get_gpu_device_bdf()` identifies each available GPU in the output with
its PCI bus:device.function (BDF) address. The example validates the selected
index, including the trailing `All GPUs` option, then passes each selected
processor handle to the remaining GPU queries.

### Read VBIOS information

`amdsmi_get_gpu_vbios_info()` returns static VBIOS identity fields for a GPU:

```python
vbios_info = amdsmi_get_gpu_vbios_info(processor)
print(vbios_info['name'])
print(vbios_info['build_date'])
print(vbios_info['part_number'])
print(vbios_info['version'])
print(vbios_info['boot_firmware'])
```

The returned dictionary contains:

| Field           | Meaning                   |
| --------------- | ------------------------- |
| `name`          | VBIOS name                |
| `build_date`    | VBIOS build date          |
| `part_number`   | VBIOS part number         |
| `version`       | VBIOS version string      |
| `boot_firmware` | Boot firmware information |

This is VBIOS identity data for the GPU, not the AMD SMI library version.

### Read GPU firmware blocks

`amdsmi_get_fw_info()` returns the firmware inventory for a GPU:

The result is a dictionary whose `fw_info_list` field is a list of firmware
blocks:

```python
fw_info = amdsmi_get_fw_info(processor)
for firmware in fw_info['fw_info_list']:
    print(firmware['fw_id'].name)
    print(firmware['fw_version'])
```

Each firmware block contains:

| Field        | Meaning                                     |
| ------------ | ------------------------------------------- |
| `fw_id`      | Firmware block identifier (`AmdSmiFwBlock`) |
| `fw_version` | Firmware version as an integer              |

An empty `fw_info_list` means the API found no firmware blocks to report.

The example omits zero-valued entries and displays the remaining versions in
the block-specific dotted or hexadecimal formats used by `amd-smi`.

### Read firmware error records

Firmware error records are a host-only optional feature. The example calls
`amdsmi_get_fw_error_records()` and reports when the capability is unavailable:

```python
try:
    error_records = amdsmi_get_fw_error_records(processor)
    for record in error_records['err_records']:
        print(record['timestamp'])
        print(record['vf_idx'])
        print(record['fw_id'].name)
        print(record['status'].name)
except AmdSmiLibraryException as e:
    print_optional_api_error("amdsmi_get_fw_error_records", e)
```

Each record in `err_records` contains:

| Field       | Meaning                                          |
| ----------- | ------------------------------------------------ |
| `timestamp` | System time in seconds                           |
| `vf_idx`    | Virtual function index associated with the event |
| `fw_id`     | Firmware block identifier (`AmdSmiFwBlock`)      |
| `status`    | Firmware load status                             |

An empty `err_records` list is a normal result when no firmware error records
are present.

### Read VF firmware

VF firmware queries require enabled SR-IOV VFs. The example first reads the
current VF partition list with `amdsmi_get_vf_partition_info()`:

```python
partitions = amdsmi_get_vf_partition_info(processor)
```

If no enabled VFs are available, it explains that VF firmware queries need an
enabled VF. For each partition on each selected GPU, it passes the VF handle to
`amdsmi_get_vf_fw_info()`:

```python
fw_info = amdsmi_get_vf_fw_info(partition['vf_id'])
for firmware in fw_info['fw_info_list']:
    print(firmware['fw_id'].name)
    print(firmware['fw_version'])
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

```python
nic_processors = amdsmi_get_nic_processor_handles()
all_nic_index = len(nic_processors)
selected_index = int(input("Enter the NIC index to inspect: "))
selected_nic_indexes = (
    range(len(nic_processors))
    if selected_index == all_nic_index else [selected_index])
```

The `All NICs` option uses index immediately after the last NIC index.

For each selected NIC handle, the example calls `amdsmi_get_nic_fw_info()`:

```python
for nic_index in selected_nic_indexes:
    nic = nic_processors[nic_index]
    fw_info = amdsmi_get_nic_fw_info(nic)
    for firmware in fw_info['fw']:
        print(firmware['type'].name)
        print(firmware['name'])
        print(firmware['version'])
```

Each NIC firmware entry contains:

| Field     | Meaning                                          |
| --------- | ------------------------------------------------ |
| `type`    | Firmware version type (`AmdSmiNicFwVersionType`) |
| `name`    | Firmware component name                          |
| `version` | Firmware version string                          |

`amdsmi_get_nic_fw_info()` may also depend on system libraries such as
`libmnl`. When NIC discovery or firmware queries are unavailable, the example
reports that and exits the NIC section cleanly.

### Handle unavailable capabilities and clean up

Optional firmware error record, VF, and NIC APIs can fail when the hardware,
driver, or build configuration does not support them. The example handles these
status codes without terminating the GPU firmware flow:

- `STATUS_NOT_SUPPORTED`
- `STATUS_DRIVER_NOT_LOADED`
- `STATUS_NOT_FOUND`
- `STATUS_NO_DATA`

These APIs raise an `AmdSmiLibraryException` for the statuses above. Other AMD
SMI errors are reported by the outer exception handler. The `initialized` flag
ensures `amdsmi_shut_down()` is called only after `amdsmi_init()` succeeds,
releasing AMD SMI resources before the script exits.
