# Run the AMD SMI RAS health example

The RAS health example inspects the Reliability, Availability, and
Serviceability (RAS) health of every discovered AMD GPU. It reports ECC
configuration and error counts, RAS features and policy information, and
retired memory pages.

`py/examples/ras_health/amdsmi_ras_health.py` is the Python implementation.

The example is read-only: it does not reset a GPU, clear error data, or change
any device configuration.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. An AMD GPU must be available.

Most of the example requires a GPU and host driver. RAS policy information
depends on device and platform capabilities. Unsupported optional APIs are
reported and do not prevent the remaining health checks from running.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/ras_health/amdsmi_ras_health.py
```

The reported values depend on the GPU, driver, firmware, and the current error
history. Finding no bad pages is a normal result.

## Example flow

The example initializes AMD SMI, discovers GPU processor handles, runs the RAS
queries for each GPU, and shuts down AMD SMI:

```python
from amdsmi import *

try:
    amdsmi_init(AmdSmiInitFlags.INIT_ALL_PROCESSORS)
    processors = amdsmi_get_processor_handles()

    for processor in processors:
        bdf = amdsmi_get_gpu_device_bdf(processor)
        total_ecc_count = amdsmi_get_gpu_total_ecc_count(processor)
        # Run the remaining RAS health queries for this processor.
except AmdSmiException as e:
    print(e)
finally:
    try:
        amdsmi_shut_down()
    except AmdSmiException as e:
        print(e)
```

`amdsmi_get_gpu_device_bdf()` identifies the GPU in the output with its PCI
bus:device.function (BDF) address. Keep the processor handle returned by
`amdsmi_get_processor_handles()` and pass that handle to the remaining GPU
queries.

### Inspect ECC health

ECC error counts are the primary RAS health indicators. The example first
calls `amdsmi_get_gpu_total_ecc_count()` to retrieve totals across the GPU:

```python
total_ecc_count = amdsmi_get_gpu_total_ecc_count(processor)
print("correctable_count is: {}".format(total_ecc_count['correctable_count']))
print("uncorrectable_count is: {}".format(total_ecc_count['uncorrectable_count']))
print("deferred_count is: {}".format(total_ecc_count['deferred_count']))
```

The returned values contain:


| Field                 | Meaning                                |
| --------------------- | -------------------------------------- |
| `correctable_count`   | ECC errors corrected by the GPU        |
| `uncorrectable_count` | ECC errors that could not be corrected |
| `deferred_count`      | Deferred ECC errors                    |


Next, `amdsmi_get_gpu_ecc_enabled()` reports whether ECC is enabled for each
GPU block. The example prints the ECC state for every GPU block and requests
per-block counters only for blocks with ECC enabled.

The Python call returns a dictionary that maps each `AmdSmiGpuBlock` to
`True` or `False`:

```python
ecc_enabled = amdsmi_get_gpu_ecc_enabled(processor)
for block, enabled in ecc_enabled.items():
    if enabled:
        block_ecc_count = amdsmi_get_gpu_ecc_count(processor, block)
        print("{}: {}".format(block.name, block_ecc_count))
```

This avoids querying an ECC counter for a block where ECC is disabled. The
per-block result has the same three count fields as the total ECC result. A
block can have ECC enabled without supporting a counter read; the example
groups those failures and reports them after the loop instead of once per
block.

### Read RAS features and policy

`amdsmi_get_gpu_ras_feature_info()` reports the RAS EEPROM version and the
ECC correction-schema bitmask:

```python
ras_feature = amdsmi_get_gpu_ras_feature_info(processor)
print("ras_eeprom_version is: {}".format(ras_feature['ras_eeprom_version']))
print("ecc_correction_schema is: {}".format(ras_feature['ecc_correction_schema']))
```

The script decodes set bits in `ecc_correction_schema` with
`AmdSmiEccCorrectionSchemaSupport` and prints the supported schema names:
`PARITY`, `CORRECTABLE`, `UNCORRECTABLE`, and `POISON`.

On supported PMFW-managed RAS EEPROM implementations, the example also calls
`amdsmi_get_gpu_ras_policy_info()`.

The result reports `major_version` and `minor_version`. For policy version 4.0,
it also includes:


| Field                                | Meaning                                      |
| ------------------------------------ | -------------------------------------------- |
| `dram_non_critical_region_threshold` | UCE threshold for a non-critical DRAM region |
| `dram_critical_region_threshold`     | UCE threshold for a critical DRAM region     |


Python returns a dictionary whose `ras_policy_info` entry holds the version
fields. Version 4.0 thresholds are present as extra keys on that dictionary:

```python
ras_policy = amdsmi_get_gpu_ras_policy_info(processor)
policy_info = ras_policy['ras_policy_info']
print("major_version is: {}".format(policy_info['major_version']))
print("minor_version is: {}".format(policy_info['minor_version']))
if 'dram_non_critical_region_threshold' in policy_info:
    print("dram_non_critical_region_threshold is: {}".format(
        policy_info['dram_non_critical_region_threshold']))
```

RAS policy support is platform-dependent. For example, it is available on
GPUs that use PMFW-managed RAS EEPROM. The example reports
when the API is not supported and continues.

### Check retired memory pages

A bad page is a GPU memory page retired from use after a RAS event. The example
uses two APIs:

```python
bad_page_threshold = amdsmi_get_bad_page_threshold(processor)
bad_pages = amdsmi_get_gpu_bad_page_info(processor)
```

`bad_page_threshold` is the threshold configured for the GPU. Each retired-page
record contains:


| Field          | Meaning                                              |
| -------------- | ---------------------------------------------------- |
| `retired_page` | Retired 64K or 4K driver-managed memory location     |
| `ts`           | Time when the RAS event was observed                 |
| `mem_channel`  | Memory channel that reported the event               |
| `mcumc_id`     | Memory-controller identifier that reported the event |

The script formats `ts` with `datetime.fromtimestamp()` before printing it.
An empty `bad_pages` list means the API found no retired pages.

Systems that do not support bad-page reporting are reported without stopping
the rest of the example.

### Handle errors

AMD SMI Python APIs raise `AmdSmiException` subclasses when a call fails. The
example handles `AmdSmiLibraryException` for optional RAS features so an
unsupported capability does not stop the full health report. Other failures
are reported by the outer exception handler.

The script always attempts `amdsmi_shut_down()` in its `finally` block after a
successful initialization.
