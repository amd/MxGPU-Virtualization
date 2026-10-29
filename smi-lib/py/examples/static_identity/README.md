# Run the AMD SMI static identity example

The static identity example is a read-only introduction to hardware and
software identity information available through AMD SMI. It reports ASIC,
board, VRAM, cache, driver, and virtualization details for every discovered
AMD GPU. It then checks optional NIC ASIC, PCIe bus, driver, and RDMA device
information.

`py/examples/static_identity/amdsmi_static_identity.py` is the Python implementation.

The example does not modify device state, reset a GPU, change a configuration,
or collect changing telemetry such as temperature, power, or utilization.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. An AMD GPU is required for GPU identity output.

NIC identity requires an AMD SMI build with NIC support and an appropriate NIC
driver. RDMA device details can be incomplete when supporting system libraries,
such as `libibverbs`, are unavailable. The example reports unavailable optional
capabilities and continues with the remaining queries.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/static_identity/amdsmi_static_identity.py
```

Reported values depend on the installed hardware, driver, firmware, and
optional NIC support. If no GPUs are available, the example prints
`No GPUs found` before checking optional NIC identity.

## Example flow

The example initializes AMD SMI, obtains GPU processor handles, prints static
identity information for every GPU, checks optional NIC identity, and then
shuts down AMD SMI:

```python
initialized = False

try:
    amdsmi_init(AmdSmiInitFlags.INIT_ALL_PROCESSORS)
    initialized = True

    processors = amdsmi_get_processor_handles()
    for processor_number, processor in enumerate(processors):
        print_gpu_static_identity(processor_number, processor)

    print_nic_static_identity()
finally:
    if initialized:
        amdsmi_shut_down()
```

`AmdSmiInitFlags.INIT_ALL_PROCESSORS` is passed explicitly so the example can
discover GPUs and optional NICs. The current native implementation does not
use this flag to restrict the discovery scope.

`amdsmi_get_processor_handles()` returns opaque GPU processor handles. Keep a
returned handle and pass it to the per-GPU APIs; do not construct handles
yourself. `amdsmi_get_gpu_device_bdf()` prints each GPU's
PCI bus:device.function (BDF) address, such as `0000:41:00.0`, so the output
can be associated with a physical device.

### Read GPU ASIC information

`amdsmi_get_gpu_asic_info()` returns ASIC identifiers and capabilities:

```python
asic_info = amdsmi_get_gpu_asic_info(processor)
print(asic_info['market_name'])
print(asic_info['vendor_id'])
print(asic_info['vendor_name'])
print(asic_info['subvendor_id'])
print(asic_info['device_id'])
print(asic_info['rev_id'])
print(asic_info['asic_serial'])
print(asic_info['oam_id'])
print(asic_info['num_of_compute_units'])
print(asic_info['target_graphics_version'])
print(asic_info['subsystem_id'])
print(asic_info['flags'])
```

The returned dictionary contains:

| Field                     | Meaning                                             |
| ------------------------- | --------------------------------------------------- |
| `market_name`             | Marketed GPU name                                   |
| `vendor_id`               | PCI vendor identifier                               |
| `vendor_name`             | GPU vendor name                                     |
| `subvendor_id`            | PCI subsystem vendor identifier                     |
| `device_id`               | PCI device identifier                               |
| `rev_id`                  | ASIC revision identifier                            |
| `asic_serial`             | ASIC serial number, when available                  |
| `oam_id`                  | OAM module identifier, or `N/A` when not applicable |
| `num_of_compute_units`    | Number of compute units                             |
| `target_graphics_version` | Graphics IP target version                          |
| `subsystem_id`            | PCI subsystem identifier                            |
| `flags`                   | ASIC capability flags                               |

PCI identifiers are formatted as hexadecimal strings. Some identity fields can
be empty or unavailable when the platform or firmware does not provide them.

### Read GPU board information

`amdsmi_get_gpu_board_info()` returns board-level identification:

```python
board_info = amdsmi_get_gpu_board_info(processor)
print(board_info['product_name'])
print(board_info['product_serial'])
```

The returned dictionary contains:

| Field               | Meaning                           |
| ------------------- | --------------------------------- |
| `model_number`      | Board model number                |
| `product_serial`    | Board product serial number       |
| `fru_id`            | Field-replaceable unit identifier |
| `product_name`      | Board product name                |
| `manufacturer_name` | Board manufacturer name           |

Board details are populated by the platform and firmware. A missing or empty
field does not necessarily indicate an AMD SMI error.

### Read GPU VRAM and cache information

`amdsmi_get_gpu_vram_info()` describes the GPU's installed video memory:

```python
vram_info = amdsmi_get_gpu_vram_info(processor)
print(vram_info['vram_type'].name)
print(vram_info['vram_size'])
print(vram_info['vram_max_bandwidth'])
```

The returned dictionary contains:

| Field                | Meaning                                      |
| -------------------- | -------------------------------------------- |
| `vram_type`          | VRAM technology as an `AmdSmiVramType` value |
| `vram_vendor`        | VRAM vendor name                             |
| `vram_size`          | Total VRAM size in bytes                     |
| `vram_bit_width`     | VRAM interface width in bits                 |
| `vram_max_bandwidth` | Maximum VRAM bandwidth in MB/s               |

`amdsmi_get_gpu_cache_info()` returns cache entries for the GPU. The example
prints one entry for each cache type:

```python
cache_info = amdsmi_get_gpu_cache_info(processor)
for cache in cache_info['cache']:
    print(cache['cache_properties'])
    print(cache['cache_size'])
    print(cache['cache_level'])
```

Each cache entry contains:

| Field                | Meaning                                                   |
| -------------------- | --------------------------------------------------------- |
| `cache_properties`   | List of cache roles, such as `DATA_CACHE` or `INST_CACHE` |
| `cache_size`         | Size of one cache instance in bytes                       |
| `cache_level`        | Cache hierarchy level                                     |
| `max_num_cu_shared`  | Maximum number of compute units that share the cache      |
| `num_cache_instance` | Number of cache instances                                 |

An empty `cache` list means that no cache entries were returned for the GPU.

### Read GPU driver and virtualization information

`amdsmi_get_gpu_driver_info()` identifies the driver serving a GPU:

```python
driver_info = amdsmi_get_gpu_driver_info(processor)
print(driver_info['driver_name'])
print(driver_info['driver_version'])
print(driver_info['driver_date'])
```

The returned dictionary contains `driver_name`, `driver_version`, and
`driver_date`. These describe the installed GPU driver, not the AMD SMI library
version.

`amdsmi_get_gpu_virtualization_mode()` identifies the GPU's virtualization
context:

```python
virtualization_mode = amdsmi_get_gpu_virtualization_mode(processor)
print(virtualization_mode.name)
```

The return value is an `AmdSmiVirtualizationMode`. The script prints enum
names so the output is easier to associate with the corresponding AMD SMI
Python API values.

### Read optional NIC identity information

NIC identity is a separate optional section. The example first obtains NIC
processor handles and prints the number of discovered NICs:

```python
nic_processors = amdsmi_get_nic_processor_handles()
print(len(nic_processors))
```

For each returned handle, `amdsmi_get_nic_device_bdf()` identifies the NIC by
its PCI bus:device.function (BDF) address:

```python
nic_bdf = amdsmi_get_nic_device_bdf(nic)
print(nic_bdf)
```

`amdsmi_get_nic_asic_info()` returns the NIC's PCI identifiers and product
identity:

```python
asic_info = amdsmi_get_nic_asic_info(nic)
print(asic_info['vendor_id'])
print(asic_info['subvendor_id'])
print(asic_info['device_id'])
print(asic_info['subsystem_id'])
print(asic_info['revision'])
print(asic_info['permanent_address'])
print(asic_info['product_name'])
print(asic_info['part_number'])
print(asic_info['serial_number'])
print(asic_info['vendor_name'])
```

The returned dictionary contains:

| Field               | Meaning                         |
| ------------------- | ------------------------------- |
| `vendor_id`         | PCI vendor identifier           |
| `subvendor_id`      | PCI subsystem vendor identifier |
| `device_id`         | PCI device identifier           |
| `subsystem_id`      | PCI subsystem identifier        |
| `revision`          | Device revision identifier      |
| `permanent_address` | Permanent MAC address           |
| `product_name`      | NIC product name                |
| `part_number`       | NIC part number                 |
| `serial_number`     | NIC serial number               |
| `vendor_name`       | NIC vendor name                 |

`amdsmi_get_nic_bus_info()` returns the NIC's PCIe bus capabilities:

```python
bus_info = amdsmi_get_nic_bus_info(nic)
print(bus_info['bdf'])
print(bus_info['max_pcie_width'])
print(bus_info['max_pcie_speed'])
print(bus_info['pcie_interface_version'])
print(bus_info['slot_type'])
```

The returned dictionary contains:

| Field                    | Meaning                           |
| ------------------------ | --------------------------------- |
| `bdf`                    | PCI bus:device.function address   |
| `max_pcie_width`         | Maximum supported PCIe link width |
| `max_pcie_speed`         | Maximum supported PCIe link speed |
| `pcie_interface_version` | PCIe interface version            |
| `slot_type`              | Physical PCIe slot type           |

`amdsmi_get_nic_driver_info()` identifies the driver serving the NIC:

```python
driver_info = amdsmi_get_nic_driver_info(nic)
print(driver_info['name'])
print(driver_info['version'])
```

The returned dictionary contains `name` and `version`.

`amdsmi_get_nic_rdma_dev_info()` returns RDMA device entries. Each entry
identifies an RDMA device and contains a list of RDMA ports:

```python
rdma_dev_info = amdsmi_get_nic_rdma_dev_info(nic)
for rdma_dev in rdma_dev_info['rdma_dev_info']:
    print(rdma_dev['rdma_dev'])
    print(rdma_dev['node_guid'])
    print(rdma_dev['node_type'])
    print(rdma_dev['sys_image_guid'])
    print(rdma_dev['fw_ver'])

    for rdma_port in rdma_dev['rdma_port_info']:
        print(rdma_port['netdev'])
        print(rdma_port['state'])
        print(rdma_port['rdma_port'])
        print(rdma_port['max_mtu'])
        print(rdma_port['active_mtu'])
```

Each RDMA device dictionary contains:

| Field            | Meaning                                      |
| ---------------- | -------------------------------------------- |
| `rdma_dev`       | RDMA device name                             |
| `node_guid`      | RDMA node globally unique identifier         |
| `node_type`      | RDMA node type                               |
| `sys_image_guid` | RDMA system-image globally unique identifier |
| `fw_ver`         | RDMA device firmware version                 |
| `rdma_port_info` | List of RDMA port information dictionaries   |

Each `rdma_port_info` dictionary contains:

| Field        | Meaning                        |
| ------------ | ------------------------------ |
| `netdev`     | Associated network device name |
| `state`      | RDMA port state                |
| `rdma_port`  | RDMA port number               |
| `max_mtu`    | Maximum supported MTU          |
| `active_mtu` | Currently active MTU           |

The maximum and active MTU values can be unavailable or incomplete without
`libibverbs`. Each NIC query is handled independently, so an unavailable API
does not prevent the example from trying the remaining NIC identity APIs.

### Handle unavailable capabilities and clean up

The optional NIC APIs can fail when the hardware, driver, operating system, or
build configuration does not support them. The example handles these status
codes without terminating the GPU identity flow:

- `STATUS_NOT_SUPPORTED`
- `STATUS_DRIVER_NOT_LOADED`
- `STATUS_NOT_FOUND`
- `STATUS_NO_DATA`

These APIs raise an `AmdSmiLibraryException` for the statuses above. Other AMD
SMI errors are reported by the outer exception handler. The `initialized` flag
ensures `amdsmi_shut_down()` is called only after `amdsmi_init()` succeeds,
releasing AMD SMI resources before the script exits.
