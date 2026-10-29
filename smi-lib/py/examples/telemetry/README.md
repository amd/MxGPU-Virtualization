# Run the AMD SMI telemetry example

The telemetry example is a read-only monitoring program for AMD GPUs and, when
available, NICs. It discovers GPUs, prints their PCIe capabilities, selects the
supported GPU telemetry API, collects a small number of current samples, and
then checks optional NIC port and RDMA counters.

`py/examples/telemetry/amdsmi_telemetry.py` is the Python implementation.

On supported AMD Instinct MI accelerator boards, the example uses the
self-describing metric table returned by the GPU metrics API
(`amdsmi_get_gpu_metrics()`). Navi32 boards that do not provide that table
use the focused compatibility telemetry APIs instead. The program detects the
available path from the API result; it does not infer support from the GPU
marketing name.

The example does not reset a GPU, change power or clock settings, create VFs,
or otherwise modify device state.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. An AMD GPU must be available.

NIC monitoring is optional. It requires an AMD SMI build with NIC support and
an appropriate NIC driver. RDMA port details can be unavailable or incomplete
without supporting system libraries such as `libibverbs`.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/telemetry/amdsmi_telemetry.py
```

The reported values depend on the GPU, driver, firmware, workload, and
optional NIC support. An idle GPU can report low activity and clocks, while a
GPU under load can report different values for every poll.

## Choose the poll duration

The example is finite so it is safe to run unchanged. By default, it gathers
three samples one second apart:

```python
POLL_ITERATIONS = 3
POLL_INTERVAL_SECONDS = 1
```

Edit these constants near the beginning of
`py/examples/telemetry/amdsmi_telemetry.py` to collect a longer interval. For
example, ten samples at five-second intervals use:

```python
POLL_ITERATIONS = 10
POLL_INTERVAL_SECONDS = 5
```

Use `Ctrl+C` to stop the example early. The script still shuts down AMD SMI
from its `finally` block.

## Example flow

The program initializes AMD SMI, gets GPU processor handles, gathers GPU
telemetry for each handle, checks optional NIC telemetry, and shuts down the
library:

```python
initialized = False

try:
    amdsmi_init(AmdSmiInitFlags.INIT_ALL_PROCESSORS)
    initialized = True

    processors = amdsmi_get_processor_handles()
    for processor in processors:
        print_gpu_pcie_capabilities(processor)

    poll_gpu_telemetry(processors)

    print_nic_telemetry()
finally:
    if initialized:
        amdsmi_shut_down()
```

`amdsmi_get_processor_handles()` returns opaque GPU processor handles. Keep a
returned handle and pass it to the per-GPU APIs; do not construct handles
yourself. `amdsmi_get_gpu_device_bdf()` identifies each GPU in the output with
its PCI bus:device.function (BDF) address, such as `0000:0c:00.0`.

`AmdSmiInitFlags.INIT_ALL_PROCESSORS` is passed explicitly so the script can
also discover optional NIC processors. The current native implementation does
not use this flag to restrict the discovery scope.

### Inspect GPU PCIe capabilities

Before polling, the example prints the static PCIe link fields:

```python
pcie_info = amdsmi_get_pcie_info(processor)
pcie_static = pcie_info['pcie_static']
print(pcie_static['max_pcie_width'])
print(pcie_static['max_pcie_speed'])
print(pcie_static['pcie_interface_version'])
print(pcie_static['slot_type'].name)
print(pcie_static['max_pcie_interface_version'])
```

It then queries the supported PCIe transfer-rate table. `current` is the
index of the active entry, not the transfer rate itself:

```python
pci_bandwidth = amdsmi_get_gpu_pci_bandwidth(processor)
transfer_rate = pci_bandwidth['transfer_rate']
current_index = transfer_rate['current']
print(current_index)
print(transfer_rate['frequency'][current_index])
print(pci_bandwidth['lanes'][current_index])
for frequency, lanes in zip(transfer_rate['frequency'],
                            pci_bandwidth['lanes']):
    print(frequency, lanes)
```

The supported PCIe transfer-rate API is hardware and driver dependent. If it
returns `STATUS_NOT_SUPPORTED`, the example reports that status and continues
with the remaining telemetry APIs.

This capability section is independent of the selected GPU telemetry path.

### Choose the GPU telemetry path

Before polling, the example calls the GPU metrics API once for each GPU. A
successful call selects the metrics-table path and supplies the first sample.
`STATUS_NOT_SUPPORTED` selects the focused compatibility APIs:

```python
try:
    metrics = amdsmi_get_gpu_metrics(processor)
except AmdSmiLibraryException as error:
    if error.err_code == AmdSmiRetCode.STATUS_NOT_SUPPORTED:
        telemetry_paths.append((processor_number, processor, None))
    else:
        print_optional_api_error("amdsmi_get_gpu_metrics", error)
else:
    telemetry_paths.append((processor_number, processor, metrics))
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

```python
metrics = amdsmi_get_gpu_metrics(processor)
for metric in metrics:
    print(metric['name'].name)
    print(metric['val'])
    print(metric['unit'].name)
    print(metric['category'].name)
```

Each metric dictionary contains:

| Field          | Meaning                                                      |
| -------------- | ------------------------------------------------------------ |
| `name`         | Metric identifier as an `AmdSmiMetricName` value             |
| `val`          | Current or accumulated metric value                          |
| `unit`         | Unit as an `AmdSmiMetricUnit` value                          |
| `category`     | Measurement category as an `AmdSmiMetricCategory` value      |
| `flags`        | Metric characteristics, such as instantaneous or accumulated |
| `vf_mask`      | Bitmask of the PF and active VFs to which the metric applies |
| `res_group`    | Resource group                                               |
| `res_subgroup` | Resource subgroup                                            |
| `res_instance` | Resource instance number                                     |

The available metrics vary by ASIC, firmware, and virtualization
configuration. An empty list means that no metrics were returned.

### Read compatibility telemetry on Navi32 boards

When the GPU metrics API returns `STATUS_NOT_SUPPORTED`, as it does on
Navi32 boards, the example polls the focused compatibility APIs instead:

- GPU activity (`amdsmi_get_gpu_activity()`) for graphics, UMC, and
  multimedia activity.
- Power info (`amdsmi_get_power_info()`) for socket power, GFX, SOC, and
  memory voltages.
- Temperature (`amdsmi_get_temp_metric()`) for current edge and hotspot
  temperatures.
- Clock info (`amdsmi_get_clock_info()`) for the GFX and memory clock
  domains.
- PCIe info (`amdsmi_get_pcie_info()`) for the current PCIe speed, width,
  bandwidth, and PCIe error counters.
- PCIe bandwidth (`amdsmi_get_gpu_pci_bandwidth()`) for the active PCIe
  transfer rate, looked up from the current index.

```python
activity = amdsmi_get_gpu_activity(processor)
power = amdsmi_get_power_info(processor)
edge_temperature = amdsmi_get_temp_metric(
    processor, AmdSmiTemperatureType.EDGE,
    AmdSmiTemperatureMetric.CURRENT)
gfx_clock = amdsmi_get_clock_info(processor, AmdSmiClkType.GFX)
```

`activity` is a dictionary with `gfx_activity`, `umc_activity`, and
`mm_activity`. `power` contains `socket_power`, `gfx_voltage`, `soc_voltage`,
and `mem_voltage`. A clock dictionary contains `clk`, `min_clk`,
`max_clk`, `clk_locked`, and `clk_deep_sleep`.

The PCIe metric dictionary contains the current `pcie_speed`, `pcie_width`,
and `pcie_bandwidth`, together with the replay, recovery, rollover, and NAK
counter fields. The Python example looks up the active transfer rate from
`frequency[transfer_rate['current']]`.

These counters are useful when comparing samples over time. After selecting
this path, the poll loop does not call the GPU metrics API again for that GPU.

Both telemetry paths print values in the units returned by AMD SMI. Consult
the Python API reference when an application needs to convert or persist a
value with a specific unit.

### Monitor optional NIC ports and counters

The NIC section is independent from GPU telemetry. It first gets NIC processor
handles, identifies each NIC by BDF, and prints its port information:

```python
nic_processors = amdsmi_get_nic_processor_handles()
for nic in nic_processors:
    port_info = amdsmi_get_nic_port_info(nic)
    for port in port_info['ports']:
        print(port['netdev'])
        print(port['link_state'])
        print(port['link_speed'])
```

Each NIC port entry includes its BDF, port number, type, flavour, network
device, interface index, MAC address, carrier state, MTU, link state, link
speed, FEC state, and pause/autonegotiation settings.

For every poll iteration, the example refreshes the port list and retrieves the
standard and vendor-specific counters for each port:

```python
port_statistics = amdsmi_get_nic_port_statistics(nic, port_index)
vendor_statistics = amdsmi_get_nic_vendor_statistics(nic, port_index)
```

Both APIs return a list of dictionaries with `name` and `value` fields.

Vendor statistic names depend on the NIC, driver, and firmware.

`amdsmi_get_nic_rdma_port_statistics()` takes the zero-based position of the
RDMA port in the order `amdsmi_get_nic_rdma_dev_info()` lists them: all ports
of the first RDMA device, then all ports of the next, and so on. This index is
not the RDMA port number that `amdsmi_get_nic_rdma_dev_info()` reports. The example walks the RDMA
device-info listing to build the correct flattened index before calling:

```python
rdma_statistics = amdsmi_get_nic_rdma_port_statistics(
    nic, rdma_port['rdma_port_index'])
```

The RDMA statistics result has the same name/value list shape as the other NIC
counter APIs.

### Handle unavailable telemetry and clean up

Hardware, driver, operating-system, and build differences can make individual
telemetry APIs unavailable. Only `STATUS_NOT_SUPPORTED` from the initial GPU
metrics call selects the compatibility path. After that selection, the example
uses an optional-API helper for independent calls within the chosen path:

```python
def get_optional_api(api_name, api, *args):
    try:
        return api(*args)
    except AmdSmiLibraryException as e:
        print_optional_api_error(api_name, e)
        return None
```

The helper prints clear messages for `STATUS_NOT_SUPPORTED`,
`STATUS_DRIVER_NOT_LOADED`, `STATUS_NOT_FOUND`, and `STATUS_NO_DATA`. A
compatibility sample prints `N/A` for a focused value whose query failed. A
metrics-table sample reports `metrics=N/A` if a later refresh fails; it does
not switch to the compatibility APIs after polling has started.

The script calls `amdsmi_shut_down()` from `finally` only after initialization
succeeds. That releases AMD SMI resources whether the example completes,
encounters an error, or is interrupted.
