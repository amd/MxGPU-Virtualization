---
myst:
  html_meta:
    "description lang=en": "Explore the AMD SMI Python API."
    "keywords": "api, smi, lib, py, system, management, interface"
---

# AMD SMI Python API reference

Python interface – Consists of Python function declarations, which directly call the C interface. The client can use the Python interface to build applications in Python.

## Requirements

* python 3.10+ 64-bit

## Overview

### Folder structure

File Name | Note
---|---
`__init__.py` | Python package initialization file
`amdsmi_interface.py` | Amdsmi library python interface
`amdsmi_wrapper.py` | Python wrapper around amdsmi binary
`amdsmi_exception.py` | Amdsmi exceptions python file
`README.md` | Documentation

### Build steps

Navigate to project's root folder and run Makefile command:

* `make package`

Build process will create a folder `build/package/BUILD_MODE/amdsmi`, where `BUILD_MODE` can be Release or Debug.
The folder will contain the following files:

* `__init__.py`
* `amdsmi_interface.py`
* `amdsmi_wrapper.py`
* `amdsmi_exception.py`
* `libamdsmi.so`
* `README.md`

## Amdsmi usage

Generated `amdsmi` folder should be copied and placed next to importing script. It should be imported as:

```python
from amdsmi import *

try:
    amdsmi_init()

    # amdsmi calls ...

except AmdSmiException as e:
    print(e)
finally:
    try:
        amdsmi_shut_down()
    except AmdSmiException as e:
        print(e)
```

To initialize amdsmi lib, amdsmi_init() must be called before all other calls to amdsmi lib.

To close connection to driver, amdsmi_shut_down() must be the last call.

## Amdsmi Exceptions

All exceptions are in `amdsmi_exception.py` file.
Exceptions that can be thrown are:

* `AmdSmiException`: base smi exception class
* `AmdSmiLibraryException`: derives base `AmdSmiException` class and represents errors that can occur in smi-lib.
When this exception is thrown, `err_code` and `err_info` are set. `err_code` is an integer that corresponds to errors that can occur
in smi-lib and `err_info` is a string that explains the error that occurred.
Example:

```python
try:
    amdsmi_init()
    processors = amdsmi_get_processor_handles()
    num_of_GPUs = len(processors)
    if num_of_GPUs == 0:
        print("No GPUs on machine")
except AmdSmiException as e:
    print("Error code: {}".format(e.err_code))
    if e.err_code == AmdSmiRetCode.AMDSMI_STATUS_RETRY:
        print("Error info: {}".format(e.err_info))
finally:
    try:
        amdsmi_shut_down()
    except AmdSmiException as e:
        print(e)
```

* `AmdSmiRetryException` : Derives `AmdSmiLibraryException` class and signals processor is busy and call should be retried.
* `AmdSmiTimeoutException` : Derives `AmdSmiLibraryException` class and represents that call had timed out.
* `AmdSmiParameterException`: Derives base `AmdSmiException` class and represents errors related to invaild parameters passed to functions. When this exception is thrown, err_msg is set and it explains what is the actual and expected type of the parameters.
* `AmdSmiBdfFormatException`: Derives base `AmdSmiException` class and represents invalid bdf format.

## Amdsmi API

### amdsmi_init

Description: Initialize smi lib and connect to driver

Input parameters: `init_flags` (**_Optional parameter, if no value provided, default value is `AMDSMI_INIT_ALL_PROCESSORS` value_**)

`init_flags` is `AmdSmiInitFlags` enum:

Field | Description
---|---
`INIT_ALL_PROCESSORS` | all processors
`INIT_AMD_CPUS` | amd cpus
`INIT_AMD_GPUS` | amd gpus
`INIT_NON_AMD_CPUS` | non amd cpus
`INIT_NON_AMD_GPUS` | non amd gpus
`INIT_AMD_APUS` | amd apus

Output: `None`

Exceptions that can be thrown by `amdsmi_init` function:

* `AmdSmiLibraryException`

Example:

```python
try:
    amdsmi_init()
    # continue with amdsmi
except AmdSmiException as e:
    print("Init failed")
    print(e)
```

### amdsmi_shut_down

Description: Finalize and close connection to driver

Input parameters: `None`

Output: `None`

Exceptions that can be thrown by `amdsmi_shut_down` function:

* `AmdSmiLibraryException`

Example:

```python
try:
    amdsmi_shut_down()
except AmdSmiException as e:
    print("Fini failed")
    print(e)
```

### amdsmi_get_processor_handles

Description: Returns list of GPU device handle objects on current machine

Input parameters: `None`

Output: List of GPU device handles

Exceptions that can be thrown by `amdsmi_get_processor_handles` function:

* `AmdSmiLibraryException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            print(amdsmi_get_gpu_device_uuid(processor))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_processor_handle_from_bdf

Description: Returns processor handle from the given BDF

Input parameters: bdf string in form of either `<domain>:<bus>:<device>.<function>` or `<bus>:<device>.<function>` in hexcode format.
Where:

* `<domain>` is 4 hex digits long from 0000-FFFF interval
* `<bus>` is 2 hex digits long from 00-FF interval
* `<device>` is 2 hex digits long from 00-1F interval
* `<function>` is 1 hex digit long from 0-7 interval

Output: processor handle object

Exceptions that can be thrown by `amdsmi_get_processor_handle_from_bdf` function:

* `AmdSmiLibraryException`
* `AmdSmiBdfFormatException`

Example:

```python
try:
    processor = amdsmi_get_processor_handle_from_bdf("0000:23:00.0")
    print(amdsmi_get_gpu_device_uuid(processor))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_device_bdf

Description: Returns BDF of the given device

Input parameters:

* GPU device for which to query

Output: BDF string in form of `<domain>:<bus>:<device>.<function>` in hexcode format.
Where:

* `<domain>` is 4 hex digits long from 0000-FFFF interval
* `<bus>` is 2 hex digits long from 00-FF interval
* `<device>` is 2 hex digits long from 00-1F interval
* `<function>` is 1 hex digit long from 0-7 interval

Exceptions that can be thrown by `amdsmi_get_gpu_device_bdf` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    processor = amdsmi_get_processor_handle_from_bdf("0000:23:00.0")
    print("Processor's bdf:", amdsmi_get_gpu_device_bdf(processor))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_index_from_processor_handle

Description: Returns the index of the given processor handle

Input parameters:

* GPU device for which to query

Output: GPU device index

Exceptions that can be thrown by `amdsmi_get_index_from_processor_handle` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            print("Processor's index:", amdsmi_get_index_from_processor_handle(processor))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_processor_handle_from_index

Description: Returns the processor handle from the given processor index

Input parameters:

* Function processor index to query

Output: processor handle object

Exceptions that can be thrown by `amdsmi_get_processor_handle_from_index` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    num_of_GPUs = len(processors)
    if num_of_GPUs == 0:
        print("No GPUs on machine")
    else:
        for index in range(num_of_GPUs):
            print("Processor handle:", amdsmi_get_processor_handle_from_index(index))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_vf_bdf

Description: Returns BDF of the given VF

Input parameters:

* VF for which to query

Output: BDF string in form of `<domain>:<bus>:<device>.<function>` in hexcode format.
Where:

* `<domain>` is 4 hex digits long from 0000-FFFF interval
* `<bus>` is 2 hex digits long from 00-FF interval
* `<device>` is 2 hex digits long from 00-1F interval
* `<function>` is 1 hex digit long from 0-7 interval

Exceptions that can be thrown by `amdsmi_get_vf_bdf` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    vf = amdsmi_get_vf_handle_from_bdf("0000:23:02.0")
    print("VF's bdf:", amdsmi_get_vf_bdf(vf))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_vf_handle_from_bdf

Description: Returns processor handle (VF) from the given BDF

Input parameters: bdf string in form of either `<domain>:<bus>:<device>.<function>` or `<bus>:<device>.<function>` in hexcode format.
Where:

* `<domain>` is 4 hex digits long from 0000-FFFF interval
* `<bus>` is 2 hex digits long from 00-FF interval
* `<device>` is 2 hex digits long from 00-1F interval
* `<function>` is 1 hex digit long from 0-7 interval

Output: processor handle object

Exceptions that can be thrown by `amdsmi_get_vf_handle_from_bdf` function:

* `AmdSmiLibraryException`
* `AmdSmiBdfFormatException`

Example:

```python
try:
    vf = amdsmi_get_vf_handle_from_bdf("0000:23:02.0")
    print(amdsmi_get_vf_uuid(vf))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_processor_handle_from_uuid

Description: Returns processor handle from the given UUID

Input parameters: uuid string
Output: processor handle object

Exceptions that can be thrown by `amdsmi_get_processor_handle_from_uuid` function:

* `AmdSmiLibraryException`

Example:

```python
try:
    processor = amdsmi_get_processor_handle_from_uuid("fcff7460-0000-1000-80e9-b388cfe84658")
    print("Processor's UUID: ", amdsmi_get_gpu_device_uuid(processor))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_vf_handle_from_uuid

Description: Returns the handle of a virtual function (VF) from the given UUID

Input parameters: uuid string
Output: vf object

Exceptions that can be thrown by `amdsmi_get_vf_handle_from_uuid` function:

* `AmdSmiLibraryException`

Example:

```python
try:
    vf = amdsmi_get_vf_handle_from_uuid("87007460-0000-1000-8059-3ae746ab9206")
    print("VF's UUID: ", amdsmi_get_vf_uuid(vf))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_device_uuid

Description: Returns the UUID of the device

Input parameters:

* GPU device for which to query

Output: UUID string unique to the device

Exceptions that can be thrown by `amdsmi_get_gpu_device_uuid` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    processor = amdsmi_get_processor_handle_from_bdf("0000:23:00.0")
    print("Device UUID: ", amdsmi_get_gpu_device_uuid(processor))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_vf_uuid

Description: Returns the UUID of the device

Input parameters:

* VF handle for which to query

Output: UUID string unique to the device

Exceptions that can be thrown by `amdsmi_get_vf_uuid` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    vf_id = amdsmi_get_vf_handle_from_vf_index(processor_handle, 0)
    print("VF UUID: ", amdsmi_get_vf_uuid(vf_id))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_driver_info

Description: Returns the version string of the driver

Input parameters:

* `processor_handle` GPU device for which to query

Output:

* `driver_name` Driver name string that is handling the GPU device
* `driver_version` Driver version string that is handling the GPU device
* `driver_date` Driver date string that is handling the GPU device

Exceptions that can be thrown by `amdsmi_get_gpu_driver_info` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    processor = amdsmi_get_processor_handle_from_bdf("0000:23:00.0")
    driver_info = amdsmi_get_gpu_driver_info(processor)
    print("Driver name: ", driver_info.driver_name)
    print("Driver version: ", driver_info.driver_version)
    print("Driver date: ", driver_info.driver_date)
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_driver_model

Description: Returns driver model information

Input parameters:

* `processor_handle` GPU device for which to query

Output:

* current driver model from `AmdSmiDriverModelType` enum

'AmdSmiDriverModelType' enum:

Field | Description
---|---
`WDDM` | Windows Display Driver Model
`WDM` | Windows Driver Model
`MCDM` | Microsoft Compute Driver Model

Exceptions that can be thrown by `amdsmi_get_gpu_driver_model` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    processor = amdsmi_get_processor_handle_from_bdf("0000:23:00.0")
    driver_model = amdsmi_get_gpu_driver_model(processor)
    print("Driver model: ", driver_model)
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_vf_handle_from_vf_index

Description: Returns VF id of the VF referenced by it's index (in partitioning info)

Input parameters:

* `processor handle` PF or child VF of a GPU device for which to query
* `VF's index` Index of VF (0-31) in GPU's partitioning info

Output:

* `VF id`

Exceptions that can be thrown by `amdsmi_get_vf_handle_from_vf_index` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    vf_id = amdsmi_get_vf_handle_from_vf_index(processor_handle, 0)
    print(amdsmi_get_vf_info(vf_id))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_total_ecc_count

Description: Returns the number of ECC errors on the GPU device

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary with fields

Field | Description
---|---
`correctable_count`| Count of ECC correctable errors
`uncorrectable_count`| Count of ECC uncorrectable errors
`deferred_count`| Count of ECC deferred errors

Exceptions that can be thrown by `amdsmi_get_gpu_total_ecc_count` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            ecc_errors = amdsmi_get_gpu_total_ecc_count(processor)
            print(ecc_errors['correctable_count'])
            print(ecc_errors['uncorrectable_count'])
            print(ecc_errors['deferred_count'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_ecc_count

Description: Returns the number of ECC errors on the GPU device for the given block

Input parameters:

* `processor_handle` GPU device which to query
* `block` The block for which error counts should be retrieved

`block` is `AmdSmiGpuBlock` enum:

Field | Description
---|---
`UMC` | UMC block
`SDMA` | SDMA block
`GFX` | GFX block
`MMHUB` | MMHUB block
`ATHUB` | ATHUB block
`PCIE_BIF` | PCIE_BIF block
`HDP` | HDP block
`XGMI_WAFL` | XGMI_WAFL block
`DF` | DF block
`SMN` | SMN block
`SEM` | SEM block
`MP0` | MP0 block
`MP1` | MP1 block
`FUSE` | FUSE block
`MCA` | MCA block
`VCN` | VCN block
`JPEG` |JPEG block
`IH` | IH block
`MPIO` | MPIO block

Output: Dictionary with fields

Field | Description
---|---
`correctable_count`| Count of ECC correctable errors
`uncorrectable_count`| Count of ECC uncorrectable errors
`deferred_count`| Count of ECC deferred errors

Exceptions that can be thrown by `amdsmi_get_gpu_ecc_count` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            ecc_errors = amdsmi_get_gpu_ecc_count(processor, AmdSmiGpuBlock.UMC)
            print(ecc_errors['correctable_count'])
            print(ecc_errors['uncorrectable_count'])
            print(ecc_errors['deferred_count'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_ecc_enabled

Description: Returns ECC capabilities (disable/enable) for each GPU block.

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary of each GPU block and its value (`False` if the block is not enabled, `True` if the block is enabled)

Each GPU block in the dictionary is from `AmdSmiGpuBlock` enum:

Field | Description
---|---
`UMC` | UMC block
`SDMA` | SDMA block
`GFX` | GFX block
`MMHUB` | MMHUB block
`ATHUB` | ATHUB block
`PCIE_BIF` | PCIE_BIF block
`HDP` | HDP block
`XGMI_WAFL` | XGMI_WAFL block
`DF` | DF block
`SMN` | SMN block
`SEM` | SEM block
`MP0` | MP0 block
`MP1` | MP1 block
`FUSE` | FUSE block
`MCA` | MCA block
`VCN` | VCN block
`JPEG` |JPEG block
`IH` | IH block
`MPIO` | MPIO block

Exceptions that can be thrown by `amdsmi_get_gpu_ecc_enabled` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            ecc_status = amdsmi_get_gpu_ecc_enabled(processor, AmdSmiGpuBlock.UMC)
            print(ecc_status)
except AmdSmiException as e:
    print(e)
```

### amdsmi_status_code_to_string

Description: Get a description of a provided AMDSMI error status

Input parameters:

* `status` The error status for which a description is desired

Output: String description of the provided error code

Exceptions that can be thrown by `amdsmi_status_code_to_string` function:

* `AmdSmiParameterException`

Example:

```python
try:
    status_str = amdsmi_status_code_to_string(AmdSmiRetCode.SUCCESS)
    print(status_str)
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_ras_feature_info

Description: Returns RAS feature info

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary with fields

Field | Description
---|---
`ras_eeprom_version`| RAS EEPROM version
`supported_ecc_correction_schema`| ecc correction schema mask used with `AmdSmiEccCorrectionSchemaSupport` enum

Exceptions that can be thrown by `amdsmi_get_gpu_ras_feature_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            ras_feature = amdsmi_get_gpu_ras_feature_info(processor)
            print(ras_feature['ras_eeprom_version'])
            print(ras_feature['supported_ecc_correction_schema'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_bad_page_info

Description: Returns bad page info.

Input parameters:

* `processor handle object` PF of a GPU device to query

Output: list of dictionaries with fields for each bad page

Field | Description
---|---
`retired_page` | 64K/4K Driver managed location that is blocked from further use
`ts` | Marks the last time when the RAS event was observed
`mem_channel` | this value identifies the memory channel the issue has been reported on
`mcumc_id` | this value identifies the memory controller the issue has been reported on

Exceptions that can be thrown by `amdsmi_get_gpu_bad_page_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processor = amdsmi_get_processor_handle_from_bdf("0000:23.00.0")
    bad_page_info = amdsmi_get_gpu_bad_page_info(processor)
    if len(bad_page_info) == 0:
        print("no bad pages")
    else:
        for table_record in bad_page_info:
            print(hex(table_record["retired_page"]))
            print(datetime.fromtimestamp(table_record['ts']).strftime('%Y/%m/%d:%H/%M/%S'))
            print(table_record['mem_channel'])
            print(table_record['mcumc_id'])

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_asic_info

Description: Returns asic information for the given GPU

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary with fields

Field | Content
---|---
`market_name` |  market name
`vendor_id` |  vendor id
`vendor_name` |  vendor name
`subvendor_id` | subsystem vendor id
`device_id` |  unique id of a GPU
`rev_id` |  revision id
`asic_serial` | asic serial
`oam_id` | xgmi physical id
`num_of_compute_units` | num of compute units (**_Not supported yet, currently hardcoded to 0_**)
`target_graphics_version` | target graphics version (**_Not supported yet, currently hardcoded to 0_**)
`subsystem_id` | subsystem device id

Exceptions that can be thrown by `amdsmi_get_gpu_asic_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            asic_info = amdsmi_get_gpu_asic_info(processor)
            print(asic_info['market_name'])
            print(asic_info['vendor_id'])
            print(asic_info['vendor_name'])
            print(asic_info['subvendor_id'])
            print(asic_info['device_id'])
            print(asic_info['subsystem_id'])
            print(asic_info['rev_id'])
            print(asic_info['asic_serial'])
            print(asic_info['oam_id'])
            print(asic_info['num_of_compute_units'])
            print(asic_info['target_graphics_version'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_pcie_info

Description: Returns static and metric information about PCIe link for the given GPU

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary with fields

Field | Content
---|---
`pcie_static` | <table> <thead><tr><th> Subfield </th><th>Description</th></tr></thead><tbody><tr><td>`max_pcie_width`</td><td> Maximum number of PCIe lanes</td></tr><tr><td>`max_pcie_speed`</td><td> Maximum PCIe speed </td></tr><tr><td>`pcie_interface_version`</td><td> PCIe interface version </td></tr><tr><td>`slot_type`</td><td> Card form factor from `AmdSmiCardFormFactor` enum </td></tr><tr><td>`max_pcie_interface_version`</td><td> Maximum PCIe link generation</td></tr></tbody></table>
`pcie_metric` | <table> <thead><tr><th> Subfield </th><th>Description</th></tr></thead><tbody><tr><td>`pcie_speed`</td><td> Current PCIe speed in MT/s </td></tr><tr><td>`pcie_width`</td><td> Current number of PCIe lanes </td><tr><td>`pcie_bandwidth`</td><td> Current PCIe bandwidth in Mb/s </td></tr><tr><td>`pcie_replay_count`</td><td> Total number of the replays issued on the PCIe link </td></tr><tr><td>`pcie_l0_to_recovery_count`</td><td>  Total number of times the PCIe link transitioned from L0 to the recovery state </td></tr><tr><td>`pcie_replay_roll_over_count`</td><td> Total number of replay rollovers issued on the PCIe link </td></tr><tr><td>`pcie_nak_sent_count`</td><td> Total number of NAKs issued on the PCIe link by the device </td></tr><tr><td>`pcie_nak_received_count`</td><td> Total number of NAKs issued on the PCIe link by the receiver </td></tr><tr><td>`pcie_lc_perf_other_end_recovery_count`</td><td> PCIe other end recovery counter </td></tr></tbody></table>

Exceptions that can be thrown by `amdsmi_get_pcie_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            pcie_info = amdsmi_get_pcie_info(processor)
            print(pcie_info['pcie_static']['max_pcie_width'])
            print(pcie_info['pcie_static']['max_pcie_speed'])
            print(pcie_info['pcie_static']['slot_type'])
            print(pcie_info['pcie_static']['max_pcie_interface_version'])
            print(pcie_info['pcie_metric']['pcie_speed'])
            print(pcie_info['pcie_metric']['pcie_width'])
            print(pcie_info['pcie_metric']['pcie_bandwidth'])
            print(pcie_info['pcie_metric']['pcie_interface_version'])
            print(pcie_info['pcie_metric']['pcie_replay_count'])
            print(pcie_info['pcie_metric']['pcie_l0_to_recovery_count'])
            print(pcie_info['pcie_metric']['pcie_replay_roll_over_count'])
            print(pcie_info['pcie_metric']['pcie_nak_sent_count'])
            print(pcie_info['pcie_metric']['pcie_nak_received_count'])
            print(pcie_info['pcie_metric']['pcie_lc_perf_other_end_recovery_count'])

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_power_cap_info

Description: Returns dictionary of power capabilities as currently configured
on the given GPU

Input parameters:

* `processor_handle` GPU device which to query
* `sensor_ind` sensor index. Normally, this will be 0. If a processor has more than one sensor, it could be greater than 0. Parameter sensor_ind is unused on @platform{host}. It is an optional parameter and is set to 0 by default.

Output: Dictionary with fields

Field | Description
---|---
`power_cap` | power capability
`default_power_cap` | default power capability
`dpm_cap` | dynamic power management capability
`min_power_cap` | minimum power capability
`max_power_cap` | maximum power capability

Exceptions that can be thrown by `amdsmi_get_power_cap_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            power_info = amdsmi_get_power_cap_info(processor)
            print(power_info['power_cap'])
            print(power_info['default_power_cap'])
            print(power_info['dpm_cap'])
            print(power_info['min_power_cap'])
            print(power_info['max_power_cap'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_fb_layout

Description: Returns framebuffer related information for the given GPU

Input parameters:

* `processor handle` PF of a GPU device for which to query

Output: Dictionary with field

Field | Description
---|---
`total_fb_size` | total framebuffer size in MB
`pf_fb_reserved` | framebuffer reserved space in MB
`pf_fb_offset` | framebuffer offset in MB
`fb_alignment` | framebuffer alignment in MB
`max_vf_fb_usable` | maximum usable framebuffer size in MB
`min_vf_fb_usable` | minimum usable framebuffer size in MB

Exceptions that can be thrown by `amdsmi_get_fb_layout` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            fb_info = amdsmi_get_fb_layout(processor)
            print(fb_info['total_fb_size'])
            print(fb_info['pf_fb_reserved'])
            print(fb_info['pf_fb_offset'])
            print(fb_info['fb_alignment'])
            print(fb_info['max_vf_fb_usable'])
            print(fb_info['min_vf_fb_usable'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_activity

Description: Returns the engine usage for the given GPU.

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary with fields

Field | Description
---|---
`gfx_activity`| graphics engine usage/activity percentage (0 - 100)
`umc_activity` | memory/UMC engine usage/activity percentage (0 - 100)
`mm_activity` | average multimedia engine usages/activities in percentage (0 - 100)

Exceptions that can be thrown by `amdsmi_get_gpu_activity` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            engine_activity = amdsmi_get_gpu_activity(processor)
            print(engine_activity['gfx_activity'])
            print(engine_activity['umc_activity'])
            print(engine_activity['mm_activity'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_power_info

Description: Returns the current power, power limit, and voltage for the given GPU

Input parameters:

* `processor_handle` GPU device which to query
* `sensor_ind` sensor index. Normally, this will be 0. If a processor has more than one sensor, it could be greater than 0. Parameter sensor_ind is unused on @platform{host}. It is an optional parameter and is set to 0 by default.

Output: Dictionary with fields

Field | Description
---|---
Note: socket_power can rarely spike above the socket power limit in some cases
`socket_power`| socket power
`gfx_voltage` | gfx voltage
`soc_voltage` | socket voltage
`mem_voltage` | memory voltage

Exceptions that can be thrown by `amdsmi_get_power_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            power_info = amdsmi_get_power_info(processor)
            print(power_info['socket_power'])
            print(power_info['gfx_voltage'])
            print(power_info['soc_voltage'])
            print(power_info['mem_voltage'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_set_power_cap

Description: Sets GPU power cap.

Input parameters:

* `processor handle` processor handle
* `sensor_ind` sensor index. Normally, this will be 0. If a processor has more than one sensor, it could be greater than 0. Parameter sensor_ind is unused on @platform{host}.
* `cap` value representing power cap to set. The value must be between the minimum (min_power_cap) and maximum (max_power_cap) power cap values, which can be obtained from ::amdsmi_power_cap_info_t.

Output:

* `None`

Exceptions that can be thrown by `amdsmi_set_power_cap` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        sensor_ind = 0
        for processor in processors:
            power_info = amdsmi_get_power_cap_info(processor)
            power_limit = random.randint(power_info['min_power_cap'], power_info['max_power_cap'])
            amdsmi_set_power_cap(processor, sensor_ind, power_limit)

except AmdSmiException as e:
    print(e)
```

### amdsmi_is_gpu_power_management_enabled

Description: Returns is power management enabled

Input parameters:

* `processor_handle` GPU device which to query

Output: Bool true if power management enabled else false

Exceptions that can be thrown by `amdsmi_is_gpu_power_management_enabled` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            is_power_management_enabled = amdsmi_is_gpu_power_management_enabled(processor)
            print(is_power_management_enabled)
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_temp_metric

Description: Returns the current temperature or limit temperature for the given processor

Input parameters:

* `processor_handle` GPU device which to query
* `thermal_domain` one of `AmdSmiTemperatureType` enum values:

Field | Description
---|---
`EDGE` | edge thermal domain
`HOTSPOT` | hotspot/junction thermal domain
`VRAM` | memory/vram thermal domain
`PLX` | plx thermal domain (**_Not supported yet_**)
`HBM_0` | HBM 0 thermal domain (**_Not supported yet_**)
`HBM_1` | HBM 1 thermal domain (**_Not supported yet_**)
`HBM_2` | HBM 2 thermal domain (**_Not supported yet_**)
`HBM_3` | HBM 3 thermal domain (**_Not supported yet_**)

* `thermal_metric` one of `AmdSmiTemperatureMetric` enum values:

Field | Description
---|---
`CURRENT` | current thermal metric
`MAX` | max thermal metric (**_Not supported yet_**)
`MIN` | min thermal metric (**_Not supported yet_**)
`MAX_HYST` | max hyst thermal metric (**_Not supported yet_**)
`MIN_HYST` | min hyst thermal metric (**_Not supported yet_**)
`CRITICAL` | limit thermal metric
`CRITICAL_HYST` | critical hyst metric (**_Not supported yet_**)
`EMERGENCY` | emergency thermal metric (**_Not supported yet_**)
`EMERGENCY_HYST` | emergency hyst thermal metric (**_Not supported yet_**)
`CRIT_MIN` | critical min thermal metric (**_Not supported yet_**)
`CRIT_MIN_HYST` | critical min hyst thermal metric (**_Not supported yet_**)
`OFFSET` | offset thermal metric (**_Not supported yet_**)
`LOWEST` | lowest thermal metric (**_Not supported yet_**)
`HIGHEST` | highest thermal metric (**_Not supported yet_**)
`SHUTDOWN` | shutdown thermal metric

Output: Temperature value

Exceptions that can be thrown by `amdsmi_get_temp_metric` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for device in processors:
            print("=============== EDGE THERMAL DOMAIN ================")
            thermal_measure = amdsmi_get_temp_metric(device, AmdSmiTemperatureType.EDGE, AmdSmiTemperatureMetric.CURRENT)
            print("Current temperature:")
            print(thermal_measure)
            thermal_measure = amdsmi_get_temp_metric(device, AmdSmiTemperatureType.EDGE, AmdSmiTemperatureMetric.CRITICAL)
            print("Limit temperature:")
            print(thermal_measure)
            print("=============== HOTSPOT THERMAL DOMAIN ================")
            thermal_measure = amdsmi_get_temp_metric(device, AmdSmiTemperatureType.HOTSPOT, AmdSmiTemperatureMetric.CURRENT)
            print("Current temperature:")
            print(thermal_measure)
            thermal_measure = amdsmi_get_temp_metric(device, AmdSmiTemperatureType.HOTSPOT, AmdSmiTemperatureMetric.CRITICAL)
            print("Limit temperature:")
            print(thermal_measure)
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_cache_info

Description: Returns the cache info for the given processor

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary with fields

Field | Description
---|---
 `cache` | <table> <thead><tr><th> Subfield </th><th>Description</th></tr></thead><tbody><tr><td>`cache_properties`</td><td>array of AmdSmiCacheProperty</td></tr><tr><td>`cache_size`</td><td>cache size</td></tr><tr><td>`cache_level`</td><td>cache level</td></tr><tr><td>`max_num_cu_shared`</td><td>number of Compute Units shared</td></tr><tr><td>`num_cache_instance`</td><td>number of instance of this cache type</td></tr></tbody></table>

`AmdSmiCacheProperty enum values`

Field | Description
---|------
`ENABLED` | Cache enabled
`DATA_CACHE` | Data cache
`INST_CACHE` | Inst cache
`CPU_CACHE` | CPU cache
`SIMD_CACHE` | SIMD cache

Exceptions that can be thrown by `amdsmi_get_gpu_cache_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for device in processors:
            cache_info = amdsmi_get_gpu_cache_info(device)
            for cache in cache_info["cache"]:
                print(cache["cache_properties"])
                print(cache["cache_size"])
                print(cache["cache_level"])
                print(cache["max_num_cu_shared"])
                print(cache["num_cache_instance"])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_clock_info

Description: Returns the clock measurements for the given GPU

Input parameters:

* `processor_handle` GPU device which to query
* `clock_domain` one of `AmdSmiClkType` enum values:

Field | Description
---|---
`SYS` | system clock domain
`GFX` | gfx clock domain
`DF` | Data Fabric clock (for ASICs running on a separate clock) domain (**_Not supported yet_**)
`DCEF` | Display Controller Engine clock domain (**_Not supported yet_**)
`SOC` | SOC clock domain (**_Not supported yet_**)
`MEM` | memory clock domain
`PCIE` | PCIe clock domain (**_Not supported yet_**)
`VCLK0` | first multimedia engine (VCLK0) clock domain
`VCLK1` | second multimedia engine (VCLK1) clock domain
`DCLK0` | DCLK0 clock domain
`DCLK1` | DCLK1 clock domain

Output: Dictionary with fields

Field | Description
---|---
`clk` | current clock value for the given domain
`min_clk` | minimum clock value for the given domain
`max_clk` | maximum clock value for the given domain
`clk_locked` | clock locked flag only supported on GFX clock domain
`clk_deep_sleep` | clock deep sleep mode flag

Exceptions that can be thrown by `amdsmi_get_clock_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            print("=============== GFX CLOCK DOMAIN ================")
            clock_measure = amdsmi_get_clock_info(processor, AmdSmiClkType.GFX)
            print(clock_measure['clk'])
            print(clock_measure['min_clk'])
            print(clock_measure['max_clk'])
            print(clock_measure['clk_locked'])
            print(clock_measure['clk_deep_sleep'])
            print("=============== MEM CLOCK DOMAIN ================")
            clock_measure = amdsmi_get_clock_info(processor, AmdSmiClkType.MEM)
            print(clock_measure['clk'])
            print(clock_measure['min_clk'])
            print(clock_measure['max_clk'])
            print(clock_measure['clk_deep_sleep'])
            print("=============== SYS CLOCK DOMAIN ================")
            clock_measure = amdsmi_get_clock_info(processor, AmdSmiClkType.SYS)
            print(clock_measure['clk'])
            print(clock_measure['min_clk'])
            print(clock_measure['max_clk'])
            print(clock_measure['clk_deep_sleep'])
            print("=============== VCLK0 CLOCK DOMAIN ================")
            clock_measure = amdsmi_get_clock_info(processor, AmdSmiClkType.VCLK0)
            print(clock_measure['clk'])
            print(clock_measure['min_clk'])
            print(clock_measure['max_clk'])
            print(clock_measure['clk_deep_sleep'])
            print("=============== VCLK1 CLOCK DOMAIN ================")
            clock_measure = amdsmi_get_clock_info(processor, AmdSmiClkType.VCLK1)
            print(clock_measure['clk'])
            print(clock_measure['min_clk'])
            print(clock_measure['max_clk'])
            print(clock_measure['clk_deep_sleep'])
            print("=============== DCLK0 CLOCK DOMAIN ================")
            clock_measure = amdsmi_get_clock_info(processor, AmdSmiClkType.DCLK0)
            print(clock_measure['clk'])
            print(clock_measure['min_clk'])
            print(clock_measure['max_clk'])
            print(clock_measure['clk_deep_sleep'])
            print("=============== DCLK1 CLOCK DOMAIN ================")
            clock_measure = amdsmi_get_clock_info(processor, AmdSmiClkType.DCLK1)
            print(clock_measure['clk'])
            print(clock_measure['min_clk'])
            print(clock_measure['max_clk'])
            print(clock_measure['clk_deep_sleep'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_vram_info

Description:  Returns the static information for the VRAM info

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary with fields

Field | Description
---|---
`vram_type` | VRAM type from `AmdSmiVramType` enum
`vram_vendor` | VRAM vendor from `AmdSmiVranVendor` enum
`vram_size` | VRAM size in MB
`vram_bit_width` | VRAM bit width

`AmdSmiVramType` enum:

Field | Description
---|---
`UNKNOWN` | UNKNOWN VRAM type
`HBM` | HBM VRAM type
`HBM2` | HBM2 VRAM type
`HBM2E` | HBM2E VRAM type
`HBM3` | HBM3 VRAM type
`DDR2` | DDR2 VRAM type
`DDR3` | DDR3 VRAM type
`DDR4` | DDR4 VRAM type
`GDDR1` | GDDR1 VRAM type
`GDDR2` | GDDR2 VRAM type
`GDDR3` | GDDR3 VRAM type
`GDDR4` | GDDR4 VRAM type
`GDDR5` | GDDR5 VRAM type
`GDDR6` | GDDR6 VRAM type
`GDDR7` | GDDR7 VRAM type

`AmdSmiVramVendor` enum:

Field | Description
---|---
`SAMSUNG` | SAMSUNG VRAM vendor
`INFINEON` | INFINEON VRAM vendor
`ELPIDA` | ELPIDA VRAM vendor
`ETRON` | ETRON VRAM vendor
`NANYA` | NANYA VRAM vendor
`HYNIX` | HYNIX VRAM vendor
`MOSEL` | MOSEL VRAM vendor
`WINBOND` | WINBOND VRAM vendor
`ESMT` | ESMT VRAM vendor
`MICRON` | MICRON VRAM vendor
`UNKNOWN` | UNKNOWN VRAM vendor

Exceptions that can be thrown by `amdsmi_get_gpu_vram_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            vram_info = amdsmi_get_gpu_vram_info(processor)
            print(vram_info['vram_type'])
            print(vram_info['vram_vendor'])
            print(vram_info['vram_size'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_vbios_info

Description:  Returns the static information for the VBIOS on the GPU device.

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary with fields

Field | Description
---|---
`name` | vbios name
`build_date` | vbios build date
`part_number` | vbios part number
`version` | vbios version string

Exceptions that can be thrown by `amdsmi_get_gpu_vbios_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            vbios_info = amdsmi_get_gpu_vbios_info(processor)
            print(vbios_info['name'])
            print(vbios_info['build_date'])
            print(vbios_info['part_number'])
            print(vbios_info['version'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_fw_info

Description: Returns the firmware information for the given GPU.

Input parameters:

* `processor_handle` GPU device which to query

Output: Dictionary with fields

Field | Description
---|---
`fw_info_list`| List of dictionaries that contain information about a certain firmware block

Exceptions that can be thrown by `amdsmi_get_fw_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            firmware_list = amdsmi_get_fw_info(processor)
            for firmware_block in firmware_list:
                print(firmware_block['fw_id'])
                print(firmware_block['fw_version'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_board_info

Description:  Returns board related information for the given GPU

Input parameters:

* `GPU device handle object`

Output: Dictionary with fields

Field | Description
---|---
`model_number` | board model number
`product_serial` | board product serial number
`fru_id` | fru (field-replaceable unit) id
`product_name` | board product name
`manufacturer_name` | board manufacturer name

Exceptions that can be thrown by `amdsmi_get_gpu_board_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            board_info = amdsmi_get_gpu_board_info(processor)
            print(board_info['model_number'])
            print(board_info['product_serial'])
            print(board_info['fru_id'])
            print(board_info['manufacturer_name'])
            print(board_info['product_name'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_num_vf

Description: Returns number of enabled VFs and number of supported VFs for the given GPU

Input parameters:

* `processor handle` PF of a GPU device for which to query

Output: Dictionary with fields

Field | Description
---|---
`num_vf_enabled` | number of enabled VFs
`num_vf_supported` | number of supported VFs

Exceptions that can be thrown by `amdsmi_get_num_vf` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            num_vf = amdsmi_get_num_vf(processor)
            print(num_vf['num_vf_enabled'])
            print(num_vf['num_vf_supported'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_vf_partition_info

Description: Returns array of the current framebuffer partitioning structures
on the given GPU

Input parameters:

* `processor handle object` PF of a GPU device for which to query

Output: Array of dictionary with fields

Field | Description
---|---
`vf_id` | VF handle
`fb` | <table>  <thead><tr> <th> Subfield </th> <th> Description</th> </tr></thead><tbody><tr><td>`fb_size`</td><td>framebuffer size</td></tr><tr><td>`fb_offset`</td><td>framebuffer offset</td></tr></tbody></table> |

Exceptions that can be thrown by `amdsmi_get_vf_partition_info` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            partitions = amdsmi_get_vf_partition_info(processor)
            print(partitions[0]['fb']['fb_size'])
            # partitions[0]['fb']['fb_size'] is frame buffer size of the first VF on the given GPU
            # we can access any VF from the array via its index in partitions list
except AmdSmiException as e:
    print(e)
```

### amdsmi_set_num_vf

Description: Set number of enabled VFs for the given GPU

Input parameters:

* `processor_handle` GPU device which to query
* `number of enabled VFs to be set`

Output: `None`

Exceptions that can be thrown by `amdsmi_set_num_vf` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            amdsmi_set_num_vf(processor,2)
except AmdSmiException as e:
    print(e)
```

### amdsmi_clear_vf_fb

Description: Clears framebuffer of the given VF on the given GPU.

`If trying to clear the framebuffer of an active function,`
`the call will fail`

Input parameters:

* `VF device handle`

Output: `None`

Exceptions that can be thrown by `amdsmi_clear_vf_fb` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for device in devices:
            partitions = amdsmi_get_vf_partition_info(device)
            amdsmi_clear_vf_fb(partitions[0]['vf_id'])
            # partitions[0]['vf_id'] is handle of the first VF on the given GPU

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_vf_data

Description:  Returns the scheduler information and guard structure for the given VF.

Input parameters:

* `VF handle`

Output: Dictionary with fields

Field | Description
---|---
`flr_count` | function level reset counter
`boot_up_time` | boot up time in microseconds
`shutdown_time` | shutdown time in microseconds
`reset_time` | reset time in microseconds
`state` | vf state
`last_boot_start` | last boot start time
`last_boot_end` | last boot end time
`last_shutdown_start` | last shutdown start time
`last_shutdown_end` | last shutdown end time
`last_reset_start` |  last reset start time
`last_reset_end` | last reset end time
`current_active_time` | current session active time, reset after guest reload
`current_running_time` | current session running time, reset after guest reload
`total_active_time` | total active time, reset after host reload
`total_running_time` | total running time, reset after host reload
`enabled` | show if guard info is enabled for VF
`guard` <br>`(dictionary of elements)` | <table>  <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`state`</td><td> vf guard state </td></tr><tr><td>`amount`</td><td> amount of monitor events after enabled </td></tr><tr><td>`interval`</td><td> interval in seconds (sliding window) in which events are counted </td></tr><tr><td>`threshold`</td><td> maximum number of events that will be processed in the given sliding window interval.<br> Additional events during the interval will be ignored.</td></tr><tr><td>`active`</td><td> current number of events in the interval </td></tr>

</tbody></table>

`AmdSmiGuardType enum values are keys in guard dictionary`

Field | Description
---|------
`FLR` | function level reset status
`EXCLUSIVE_MOD` | exclusive access mode status
`EXCLUSIVE_TIMEOUT` | exclusive access time out status
`ALL_INT` | generic interrupt status

`State is AmdSmiGuardState enum object with values`

Field | Description
---|---
`NORMAL` | the event number is within the threshold
`FULL` |  the event number hits the threshold
`OVERFLOW` | the event number is bigger than the threshold

`State is AmdSmiVfState enum object with values`

Field | Description
---|---
`UNAVAILABLE` | vf state unavailable
`AVAILABLE` | vf state available
`ACTIVE` | vf state active
`SUSPENDED` | vf state suspended
`FULLACCESS` | vf state fullaccess
`DEFAULT_AVAILABLE` | same as available, indicates this is a default VF

Exceptions that can be thrown by `amdsmi_get_vf_data` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            partitions = amdsmi_get_vf_partition_info(processor)
            vf_data = amdsmi_get_vf_data(partitions[0]['vf_id'])
            sched_info = vf_data['sched']
            guard_info = vf_data['guard']
            print(sched_info['boot_up_time'])
            print(sched_info['flr_count'])
            print(sched_info['state'].name)
            print(sched_info['last_boot_start'])
            print(sched_info['last_boot_end'])
            print(sched_info['last_shutdown_start'])
            print(sched_info['last_shutdown_end'])
            print(sched_info['shutdown_time'])
            print(sched_info['last_reset_start'])
            print(sched_info['last_reset_end'])
            print(sched_info['reset_time'])
            print(sched_info['current_active_time'])
            print(sched_info['current_running_time'])
            print(sched_info['total_active_time'])
            print(sched_info['total_running_time'])

            print(guard_info['enabled'])
            for guard_type in guard_info['guard']:
                print("type: {} ".format(guard_type))
                print("state: {}".format(guard_info['guard'][guard_type]['state']))
                print("amount: {}".format(guard_info['guard'][guard_type]['amount']))
                print("interval: {}".format(guard_info['guard'][guard_type]['interval']))
                print("threshold: {}".format(guard_info['guard'][guard_type]['threshold']))
                print("active: {}".format(guard_info['guard'][guard_type]['active']))
                print("==================")
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_vf_info

Description: Returns the configuration structure for a given VF

Input parameters:

* `VF handle`

Output: Dictionary with fields

Field | Description
---|------
`fb` | <table>  <thead><tr> <th> Subfield </th> <th> Description</th> </tr></thead><tbody><tr><td>`fb_offset`</td><td>framebuffer offset</td></tr><tr><td>`fb_size`</td><td>framebuffer size</td></tr> </tbody></table> |
`gfx_timeslice` | gfx timeslice in us

</table>

Exceptions that can be thrown by `amdsmi_get_vf_info` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            partitions = amdsmi_get_vf_partition_info(processor)
            # partitions[0]['vf_id'] is handle of the first VF on the given GPU
            config = amdsmi_get_vf_info(partitions[0]['vf_id'])
            print("fb_offset: {}".format(config['fb']['fb_offset']))
            print("fb_size: {}".format(config['fb']['fb_size']))

            print("gfx_timeslice : {}".format(config['gfx_timeslice']))
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_guest_data

Description: Gets guest OS information of the queried VF

Input parameters:

* `processor handle` VF of a GPU device

Output: Dictionary with fields

Field | Description
---|---
`driver_version` | driver version
`fb_usage` |  fb usage in MB

Exceptions that can be thrown by `amdsmi_get_guest_data` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            num_vf_enabled = amdsmi_get_num_vf(processor)['num_vf_enabled']
            partitions = amdsmi_get_vf_partition_info(processor)
            for i in range(0, num_vf_enabled):
                guest_data = amdsmi_get_guest_data(partitions[i]['vf_id'])
                print(guest_data)

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_fw_error_records

Description: Gets firmware error records

Input parameters:

* `processor handle` PF of a GPU device

Output: Dictionary with field `err_records`, which is list of elements

Field | Description
---|---
`timestamp` | system time in seconds
`vf_idx` |  vf index
`fw_id` | firmware id
`status` | firmware load status

Exceptions that can be thrown by `amdsmi_get_fw_error_records` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            err_records = amdsmi_get_fw_error_records(processor)
            print(err_records)

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_dfc_fw_table

Description: Gets dfc firmware table

Input parameters:

* `processor handle` PF of a GPU device

Output: Dictionary with field `header`, and `data` which is a list of elements

Each header is a dictionary with following fields:

Field | Description
---|---
`dfc_fw_version` | dfc firmware version
`dfc_fw_total_entries` | number of entries in the dfc table
`dfc_gart_wr_guest_min` | gart wr guest min
`dfc_gart_wr_guest_max` | gart wr guest max

Each data entry is a dictionary with following fields:

Field | Description
---|---
`dfc_fw_type` | dfc firmware type
`verification_enabled` | verification enabled
`customer_ordinal` | customer ordinal
`white_list` | white list
`black_list` | black list

Each white list entry is a dictionary with following fields:

Field | Description
---|---
`latest` | latest
`oldest` | oldest

Exceptions that can be thrown by `amdsmi_get_dfc_fw_table` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            dfc_table = amdsmi_get_dfc_fw_table(processor)
            print(dfc_table)

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_vf_fw_info

Description:  Returns GPU firmware related information.

Input parameters:

* `processor handle` VF of a GPU device for which to query

Output: Dictionary with field `fw_list`, which is list of elements

If microcode of certain type is not loaded, version will be 0.

Field | Description
---|---
`fw_info_list` | <table>  <thead><tr> <th> Subfield </th> <th> Description</th> </tr></thead><tbody><tr><td>`fw_id`</td><td>`AmdSmiFwBlock` enum</td></tr><tr><td>`fw_version`</td><td>fw version which is integer</td></tr></tbody></table>

Exceptions that can be thrown by `amdsmi_get_vf_fw_info` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            partitions = amdsmi_get_vf_partition_info(processor)
            fw_info = amdsmi_get_vf_fw_info(partitions[0]['vf_id'])
            fw_num = len(fw_info['fw_list'])
            for j in range(0, fw_num):
                fw = fw_info['fw_list'][j]
                print(fw['fw_name'].name)
                print(fw['fw_version'])
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_partition_profile_info

Description: Gets partition profile info

Input parameters:

* `processor handle` PF of a GPU device

Output: Dictionary with fields, current_profile and profiles

Field | Description
---|---
`current_profile_index` | current profile index
`profiles` |  list of all profiles

Where, `profiles` is a list containing

Field | Description
---|---
`vf_count` | number of vfs
`profile_caps (dictionary)` | <table>  <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`total`</td><td> total </td></tr><tr><td>`available`</td><td> available </td></tr><tr><td>`optimal`</td><td> optimal </td></tr><tr><td>`min_value`</td><td> minimum </td></tr><tr><td>`max_value`</td><td> maximum </td></tr>

</tbody></table>

Keys for `profile_caps` dictionary are in `AmdSmiProfileCapabilityType` enum

Field | Description
---|---
`MEMORY` | memory
`ENCODE` |  encode engine
`DECODE` | decode engine
`COMPUTE` | compute engine

Exceptions that can be thrown by `amdsmi_get_partition_profile_info` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            profile_info = amdsmi_get_partition_profile_info(processor)
            print(profile_info)

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_link_metrics

Description: Gets link metric information

Input parameters:

* `processor handle` PF of a GPU device

Output: `links` list of dictionaries with fields for each link

Field | Description
---|---
`bdf` | BDF of the given processor
`bit_rate` | current link speed in Gb/s
`max_bandwidth` | max bandwidth of the link
`link_type` | type of the link from `AmdSmiLinkType` enum
`read` | total data received for each link in KB
`write` | total data transferred for each link in KB

`AmdSmiLinkType` enum:

Field | Description
---|---
`INTERNAL` | Unknown
`XGMI` | XGMI link type
`PCIE` | PCIe link type
`NOT_APPLICABLE` | Link not applicable
`UNKNOWN`  | Unknown

Exceptions that can be thrown by `amdsmi_get_link_metrics` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            link_metrics = amdsmi_get_link_metrics(processor)
            print(link_metrics)

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_link_topology

Description: Gets link topology information between two connected processors

Input parameters:

* `source processor handle` PF of a source GPU device
* `destination processor handle` PF of a destination GPU device

Output: Dictionary with fields

Field | Description
---|---
`weight` | link weight between two GPUs
`link_status` | HW status of the link
`link_type` | type of the link from `AmdSmiLinkType` enum
`num_hops` | number of hops between two GPUs
`fb_sharing` | framebuffer sharing between two GPUs

`AmdSmiLinkType` enum:

Field | Description
---|---
`INTERNAL` | Unknown
`XGMI` | XGMI link type
`PCIE` | PCIe link type
`NOT_APPLICABLE` | Link not applicable
`UNKNOWN`  | Unknown

Exceptions that can be thrown by `amdsmi_get_link_topology` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for src_processor in processors:
            for dst_processor in processors:
                link_topology = amdsmi_get_link_topology(src_processor, dst_processor)
                print(link_topology)

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_xgmi_fb_sharing_caps

Description: Gets XGMI capabilities

Input parameters:

* `processor handle` PF of a GPU device

Output: Dictionary with fields

Field | Description
---|---
`mode_custom_cap` | flag that indicates if custom mode is supported (**_Not supported yet_**)
`mode_1_cap` | flag that indicates if mode_1 is supported
`mode_2_cap` | flag that indicates if mode_2 is supported
`mode_4_cap` | flag that indicates if mode_4 is supported
`mode_8_cap` | flag that indicates if mode_8 is supported

Exceptions that can be thrown by `amdsmi_get_xgmi_fb_sharing_caps` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            caps = amdsmi_get_xgmi_fb_sharing_caps(processor)
            print(caps)

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_xgmi_fb_sharing_mode_info

Description: Gets XGMI framebuffer sharing information between two GPUs

Input parameters:

* `source processor handle` PF of a source GPU device
* `destination processor handle` PF of a destination GPU device
* `mode` framebuffer sharing mode from `AmdSmiXgmiFbSharingMode` enum

`AmdSmiXgmiFbSharingMode` enum:

Field | Description
---|---
`CUSTOM` | custom framebuffer sharing mode (**_Not supported yet_**)
`MODE_1` | framebuffer sharing mode_1
`MODE_2` | framebuffer sharing mode_2
`MODE_4` | framebuffer sharing mode_4
`MODE_8` | framebuffer sharing mode_8

Output: Value indicating whether framebuffer sharing is enabled between two GPUs

Exceptions that can be thrown by `amdsmi_get_xgmi_fb_sharing_mode_info` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for src_processor in processors:
            for dst_processor in processors:
                fb_sharing = amdsmi_get_xgmi_fb_sharing_mode_info(src_processor, dst_processor, AmdSmiXgmiFbSharingMode.MODE_4)
                print(fb_sharing)

except AmdSmiException as e:
    print(e)
```

### amdsmi_set_xgmi_fb_sharing_mode

Description: Sets framebuffer sharing mode

Note: This API will only work if there's no guest VM running.

Input parameters:

* `processor handle` PF of a GPU device
* `mode` framebuffer sharing mode from `AmdSmiXgmiFbSharingMode` enum

`AmdSmiXgmiFbSharingMode` enum:

Field | Description
---|---
`CUSTOM` | custom framebuffer sharing mode (**_Not supported yet_**)
`MODE_1` | framebuffer sharing mode_1
`MODE_2` | framebuffer sharing mode_2
`MODE_4` | framebuffer sharing mode_4
`MODE_8` | framebuffer sharing mode_8

Output:

* `None`

Exceptions that can be thrown by `amdsmi_set_xgmi_fb_sharing_mode` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            amdsmi_set_xgmi_fb_sharing_mode(processor, AmdSmiXgmiFbSharingMode.MODE_4)

except AmdSmiException as e:
    print(e)
```

### amdsmi_set_xgmi_fb_sharing_mode_v2

Description: Sets framebuffer sharing mode

Note: This API will only work if there's no guest VM running.
      This api can be used for custom and auto setting of xgmi frame buffer sharing.
         In case of custom mode:
           - All processors in the list must be on the same NUMA node.
             Otherwise, api will return error.
           - If any processor from the list already belongs to an existing group,
             the existing group will be released automatically.
         In case of auto mode(MODE_X):
           - The input parameter processor_list[0] should be valid.
             Only the first element of processor_list is taken into account and it can be any gpu0,gpu1,...

Input parameters:

* `processor_list` list of PFs of a GPU devices
* `mode` framebuffer sharing mode from `AmdSmiXgmiFbSharingMode` enum

`AmdSmiXgmiFbSharingMode` enum:

Field | Description
---|---
`CUSTOM` | custom framebuffer sharing mode
`MODE_1` | framebuffer sharing mode_1
`MODE_2` | framebuffer sharing mode_2
`MODE_4` | framebuffer sharing mode_4
`MODE_8` | framebuffer sharing mode_8

Output:

* `None`

Exceptions that can be thrown by `amdsmi_set_xgmi_fb_sharing_mode_v2` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    processors_custom_mode = []
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        if len(processors) > 3:
            processors_custom_mode.append(processors[0])
            processors_custom_mode.append(processors[2])
        else:
            processors_custom_mode = processors
        amdsmi_set_xgmi_fb_sharing_mode_v2(processors_custom_mode, AmdSmiXgmiFbSharingMode.CUSTOM)

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_metrics

Description: Gets GPU metric information

Input parameters:

* `processor handle` PF of a GPU device

Output: list of dictionaries with fields for each metric

Field | Description
---|---
`val` | value of the metric
`unit` | unit of the metric from `AmdSmiMetricUnit` enum
`name` | name of the metric from `AmdSmiMetricName` enum
`category` | category of the metric from `AmdSmiMetricCategory` enum
`flags` |  list of types of the metric from `AmdSmiMetricType` enum
`vf_mask` |  mask of all active VFs + PF that this metric applies to

`AmdSmiMetricUnit` enum:

Field | Description
---|---
`COUNTER` | counter
`UINT` | unsigned integer
`BOOL` | boolean
`MHZ` | megahertz
`PERCENT` | percentage
`MILLIVOLT` | millivolt
`CELSIUS` | celsius
`WATT` | watt
`JOULE` | joule
`GBPS` | gigabyte per second
`MBITPS` | megabit per second
`UNKNOWN` | unknown unit

`AmdSmiMetricName` enum:

Field | Description
---|---
`ACC_COUNTER` | accumulated counter
`FW_TIMESTAMP` | firmware timestamp
`CLK_GFX` | gfx clock
`CLK_SOC` | socket clock
`CLK_MEM` | memory clock
`CLK_VCLK` | vclk clock
`CLK_DCLK` | dclk clock
`USAGE_GFX` | gfx usage
`USAGE_MEM` | memory usage
`USAGE_MM` | mm usage
`USAGE_VCN` | vcn usage
`USAGE_JPEG` | jpeg usage
`VOLT_GFX` | gfx voltage
`VOLT_SOC` | socket voltage
`VOLT_MEM` | memory voltage
`TEMP_HOTSPOT_CURR` | current hotspot temperature
`TEMP_HOTSPOT_LIMIT` | hotspot temperature limit
`TEMP_MEM_CURR` | current memory temperature
`TEMP_MEM_LIMIT` | memory temperature limit
`TEMP_VR_CURR` | current vr temperature
`TEMP_SHUTDOWN` | shutdown temperature
`POWER_CURR` | current power
`POWER_LIMIT` | power limit
`ENERGY_SOCKET` | socket energy
`ENERGY_CCD` | ccd energy
`ENERGY_XCD` | xcd energy
`ENERGY_AID` | aid energy
`ENERGY_MEM` | memory energy
`THROTTLE_SOCKET_ACTIVE` | active socket throttle
`THROTTLE_VR_ACTIVE` | active vr throttle
`THROTTLE_MEM_ACTIVE` | active memory throttle
`PCIE_BANDWIDTH` | pcie bandwidth
`PCIE_L0_TO_RECOVERY_COUNT` | pcie l0 recovery count
`PCIE_REPLAY_COUNT` | pcie replay count
`PCIE_REPLAY_ROLLOVER_COUNT` | pcie replay rollover count
`PCIE_NAK_SENT_COUNT` | pcie nak sent count
`PCIE_NAK_RECEIVED_COUNT` | pcie nak received count
`CLK_GFX_MAX_LIMIT` | maximum gfx clock limit
`CLK_SOC_MAX_LIMIT` | maximum socket clock limit
`CLK_MEM_MAX_LIMIT` | maximum memory clock limit
`CLK_VCLK_MAX_LIMIT` | maximum vclk clock limit
`CLK_DCLK_MAX_LIMIT` | maximum dclk clock limit
`CLK_GFX_MIN_LIMIT` | minimum gfx clock limit
`CLK_SOC_MIN_LIMIT` | minimum socket clock limit
`CLK_MEM_MIN_LIMIT` | minimum memory clock limit
`CLK_VCLK_MIN_LIMIT` | minimum vclk clock limit
`CLK_DCLK_MIN_LIMIT` | minimum dclk clock limit
`CLK_GFX_LOCKED` | gfx clock locked
`CLK_GFX_DS_DISABLED` | gfx deep sleep
`CLK_MEM_DS_DISABLED` | memory deep sleep
`CLK_SOC_DS_DISABLED` | socket deep sleep
`CLK_VCLK_DS_DISABLED` | vclk deep sleep
`CLK_DCLK_DS_DISABLED` | dclk deep sleep
`UNKNOWN` | unknown name

`AmdSmiMetricCategory` enum:

Field | Description
---|---
`ACC_COUNTER` | counter
`FREQUENCY` | frequency
`ACTIVITY` | activity
`TEMPERATURE` | temperature
`POWER` | power
`ENERGY` | energy
`CELSIUS` | celsius
`THROTTLE` | throttle
`PCIE` | pcie
`UNKNOWN` | unknown category

`AmdSmiMetricType` enum:

Field | Description
---|---
`COUNTER` | counter
`CHIPLET` | chiplet
`INST` | instantaneous data
`ACC` | accumulated data

Exceptions that can be thrown by `amdsmi_get_gpu_metrics` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            metrics = amdsmi_get_gpu_metrics(processor)
            print(metrics)

except AmdSmiException as e:
    print(e)
```

### amdsmi_get_soc_pstate

Description: Gets the soc pstate policy for the processor

Input parameters:

* `processor handle` PF of a GPU device

Output: Dictionary with fields

Field | Description
---|---
`cur` | current policy index
`policies` | List of policies

Each policies list entry is a dictionary with following fields:

Field | Description
---|---
`policy_id` | policy id
`policy_description` | policy description

Exceptions that can be thrown by `amdsmi_get_soc_pstate` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            dpm_policy = amdsmi_get_soc_pstate(processor)
            print(dpm_policy)

except AmdSmiException as e:
    print(e)
```

### amdsmi_set_soc_pstate

Description: Sets the soc pstate policy for the processor

Input parameters:

* `processor handle` PF of a GPU device
* `policy_id` policy id represents one of the values we get from the policies list from amdsmi_get_soc_pstate.

Output:

* `None`

Exceptions that can be thrown by `amdsmi_set_soc_pstate` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            amdsmi_set_soc_pstate(processor, 0)

except AmdSmiException as e:
    print(e)
```

### AmdSmiEventReader class

Description: Providing methods for event monitoring

Methods:

#### constructor

Description: Allocates a new event reader notifier to monitor different types of issues with the GPU

Input parameters:

* `processor handle list` list of GPU device handle objects(PFs od Vfs) for which to create event reader
* `event category list` list of the different event categories that the event reader will monitor in GPU

`Event category is AmdSmiEventCategory enum object with values`

Category | Description
---|------
`NOT_USED` | not used category
`DRIVER` | driver events(allocation, failures of APIs, debug errors)
`RESET` | events/notifications regarding RESET executed by the GPU
`SCHED`   | scheduling events(world switch fail ...)
`VBIOS`   |  VBIOS events(security failures, vbios corruption...)
`ECC`   |  ecc events
`PP`   |  pp events(slave not present, dpm fail ....)
`IOV`   |  events regarding the configuration of VF resources
`VF`    | vf events(no vbios, gpu reset fail...)
`FW`   | events related with FW loading or FW operations
`GPU`   |  gpu fatal conditions
`GUARD` | guard events
`GPUMON`| gpumon events(fb issues ...)
`MMSCH` | mmsch events
`XGMI`| xgmi events
`ALL`   | monitor all categories

* `severity` of events that can be monitored

`Severity is AmdSmiEventSeverity enum object with values`

Severity | Description
---|------
`HIGH` | critical error
`MED` | significant error
`LOW`   | trivial error
`WARN` | warning
`INFO` | info
`ALL`   | monitor all severity levels

`severity` parameter is optional. If nothing is set, events with `LOW` severity will be monitored by default.

Output:

* `created object of AmdSmiEventReader class`

Exceptions that can be thrown by `AmdSmiEventReader constructor` function:

* `AmdSmiParameterException`
* `AmdSmiLibraryException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        event_reader = AmdSmiEventReader(processors,{AmdSmiEventCategory.RESET})
except SmiException as e:
    print(e)
```

#### read

Description: Reads and return one event from event reader

Input parameters:

* `timestamp` number of microseconds to wait for an event to occur. If event does not happen monitoring is finished

Output: Dictionary with fields

Field | Description
---|---
`fcn_id` | VF handle
`dev_id` | GPU device handle
`timestamp` | UTC time (in microseconds) when the error happened
`data` | data value associated with the specific event
`category` | event category
`subcode` | event subcategory
`level` | event severity
`date` | UTC date and time when the error happend
`message` | message describing the event

`event category is AmdSmiEventCategory enum object with values`

Category | Description
---|------
`NOT_USED` | not used category
`DRIVER` | driver events(allocation, failures of APIs, debug errors)
`RESET` | events/notifications regarding RESET executed by the GPU
`SCHED`   | scheduling events(world switch fail ...)
`VBIOS`   |  VBIOS events(security failures, vbios corruption...)
`ECC`   |  ecc events
`PP`   |  pp events(slave not present, dpm fail ....)
`IOV`   |  events regarding the configuration of VF resources
`VF`    | vf events(no vbios, gpu reset fail...)
`FW`   | events related with FW loading or FW operations
`GPU`   |  gpu fatal conditions
`GUARD` | guard events
`GPUMON`| gpumon events(fb issues ...)
`MMSCH` | mmsch events
`XGMI`| xgmi events
`ALL`   | monitor all categories

`every AmdSmiEventCategory has it's corresponding enum subcategory,` <br/>
`subcategories are:`

Subcategory | Field
---|------
`AmdSmiEventCategoryGpu`| <table><tbody><tr><td>GPU_DEVICE_LOST</td></tr><tr><td>GPU_NOT_SUPPORTED</td></tr><tr><td>GPU_RMA</td></tr><tr><td>GPU_NOT_INITIALIZED</td></tr><tr><td>GPU_MMSCH_ABNORMAL_STATE</td></tr><tr><td>GPU_RLCV_ABNORMAL_STATE</td></tr><tr><td>GPU_SDMA_ENGINE_BUSY</td></tr><tr><td>GPU_RLC_ENGINE_BUSY</td></tr><tr><td>GPU_GC_ENGINE_BUSY</td></tr></tbody></table> |
`AmdSmiEventCategoryDriver` | <table><tbody><tr><td>DRIVER_SPIN_LOCK_BUSY</td></tr><tr><td>DRIVER_ALLOC_SYSTEM_MEM_FAIL</td></tr><tr><td>DRIVER_CREATE_GFX_WORKQUEUE_FAIL</td></tr><tr><td>DRIVER_CREATE_MM_WORKQUEUE_FAIL</td></tr><tr><td>DRIVER_BUFFER_OVERFLOW</td></tr><tr><td>DRIVER_DEV_INIT_FAIL</td></tr><tr><td>DRIVER_CREATE_THREAD_FAIL</td></tr><tr><td>DRIVER_NO_ACCESS_PCI_REGION</td></tr><tr><td>DRIVER_MMIO_FAIL</td></tr><tr><td>DRIVER_INTERRUPT_INIT_FAIL</td></tr><tr><td>DRIVER_INVALID_VALUE</td></tr><tr><td>DRIVER_CREATE_MUTEX_FAIL</td></tr><tr><td>DRIVER_CREATE_TIMER_FAIL</td></tr><tr><td>DRIVER_CREATE_EVENT_FAIL</td></tr><tr><td>DRIVER_CREATE_SPIN_LOCK_FAIL</td></tr><tr><td>DRIVER_ALLOC_FB_MEM_FAIL</td></tr><tr><td>DRIVER_ALLOC_DMA_MEM_FAIL</td></tr><tr><td>DRIVER_NO_FB_MANAGER</td></tr><tr><td>DRIVER_HW_INIT_FAIL</td></tr><tr><td>DRIVER_SW_INIT_FAIL</td></tr><tr><td>DRIVER_INIT_CONFIG_ERROR</td></tr><tr><td>DRIVER_ERROR_LOGGING_FAILED</td></tr><tr><td>DRIVER_CREATE_RWLOCK_FAIL</td></tr><tr><td>DRIVER_CREATE_RWSEMA_FAIL</td></tr><tr><td>DRIVER_GET_READ_LOCK_FAIL</td></tr><tr><td>DRIVER_GET_WRITE_LOCK_FAIL</td></tr><tr><td>DRIVER_GET_READ_SEMA_FAIL</td></tr><tr><td>DRIVER_GET_WRITE_SEMA_FAIL</td></tr><tr><td>DRIVER_DIAG_DATA_INIT_FAIL</td></tr><tr><td>DRIVER_DIAG_DATA_MEM_REQ_FAIL</td></tr><tr><td>DRIVER_DIAG_DATA_VADDR_REQ_FAIL</td></tr><tr><td>DRIVER_DIAG_DATA_BUS_ADDR_REQ_FAIL</td></tr><tr><td>DRIVER_HRTIMER_START_FAIL</td></tr><tr><td>DRIVER_CREATE_DRIVER_FILE_FAIL</td></tr><tr><td>DRIVER_CREATE_DEVICE_FILE_FAIL</td></tr><tr><td>DRIVER_CREATE_DEBUGFS_FILE_FAIL</td></tr><tr><td>DRIVER_CREATE_DEBUGFS_DIR_FAIL</td></tr><tr><td>DRIVER_PCI_ENABLE_DEVICE_FAIL</td></tr><tr><td>DRIVER_FB_MAP_FAIL</td></tr><tr><td>DRIVER_DOORBELL_MAP_FAIL</td></tr><tr><td>DRIVER_CREATE_DEBUGFS_FILE_FAIL</td></tr><tr><td>DRIVER_PCI_REGISTER_DRIVER_FAIL</td></tr><tr><td>DRIVER_ALLOC_IOVA_ALIGN_FAIL</td></tr><tr><td>DRIVER_ROM_MAP_FAIL</td></tr><tr><td>DRIVER_FULL_ACCESS_TIMEOUT</td></tr></tbody></table> |
`AmdSmiEventCategoryFw` | <table><tbody><tr><td>FW_CMD_ALLOC_BUF_FAIL</td></tr><tr><td>FW_CMD_BUF_PREP_FAIL</td></tr><tr><td>FW_RING_INIT_FAIL</td></tr><tr><td>FW_FW_APPLY_SECURITY_POLICY_FAIL</td></tr><tr><td>FW_START_RING_FAIL</td></tr><tr><td>FW_FW_LOAD_FAIL</td></tr><tr><td>FW_EXIT_FAIL</td></tr><tr><td>FW_INIT_FAIL</td></tr><tr><td>FW_CMD_SUBMIT_FAIL</td></tr><tr><td>FW_CMD_FENCE_WAIT_FAIL</td></tr><tr><td>FW_TMR_LOAD_FAIL</td></tr><tr><td>FW_TOC_LOAD_FAIL</td></tr><tr><td>FW_RAS_LOAD_FAIL</td></tr><tr><td>FW_RAS_UNLOAD_FAIL</td></tr><tr><td>FW_RAS_TA_INVOKE_FAIL</td></tr><tr><td>FW_RAS_TA_ERR_INJECT_FAIL</td></tr><tr><td>FW_ASD_LOAD_FAIL</td></tr><tr><td>FW_ASD_UNLOAD_FAIL</td></tr><tr><td>FW_AUTOLOAD_FAIL</td></tr><tr><td>FW_VFGATE_FAIL</td></tr><tr><td>FW_XGMI_LOAD_FAIL</td></tr><tr><td>FW_XGMI_UNLOAD_FAIL</td></tr><tr><td>FW_XGMI_TA_INVOKE_FAIL</td></tr><tr><td>FW_TMR_INIT_FAIL</td></tr><tr><td>FW_NOT_SUPPORTED_FEATURE</td></tr><tr><td>FW_GET_PSP_TRACELOG_FAIL</td></tr><tr><td>FW_SET_SNAPSHOT_ADDR_FAIL</td></tr><tr><td>FW_SNAPSHOT_TRIGGER_FAIL</td></tr><tr><td>FW_MIGRATION_GET_PSP_INFO_FAIL</td></tr><tr><td>FW_MIGRATION_EXPORT_FAIL</td></tr><tr><td>FW_MIGRATION_IMPORT_FAIL</td></tr><tr><td>FW_BL_FAIL</td></tr><tr><td>FW_RAS_BOOT_FAIL</td></tr><tr><td>FW_MAILBOX_ERROR</td></tr></tbody></table> |
`AmdSmiEventCategoryReset` | <table><tbody><tr><td>RESET_GPU</td></tr><tr><td>RESET_GPU_FAILED</td></tr><tr><td>RESET_FLR</td></tr><tr><td>RESET_FLR_FAILED</td></tr></tbody></table> |
`AmdSmiEventCategoryIOV` | <table><tbody><tr><td>IOV_NO_GPU_IOV_CAP</td></tr><tr><td>IOV_ASIC_NO_SRIOV_SUPPORT</td></tr><tr><td>IOV_ENABLE_SRIOV_FAIL</td></tr><tr><td>IOV_CMD_TIMEOUT</td></tr><tr><td>IOV_CMD_ERROR</td></tr><tr><td>IOV_INIT_IV_RING_FAIL</td></tr><tr><td>IOV_SRIOV_STRIDE_ERROR</td></tr><tr><td>IOV_WS_SAVE_TIMEOUT</td></tr><tr><td>IOV_WS_IDLE_TIMEOUT</td></tr><tr><td>IOV_WS_RUN_TIMEOUT</td></tr><tr><td>IOV_WS_LOAD_TIMEOUT</td></tr><tr><td>IOV_WS_SHUTDOWN_TIMEOUT</td></tr><tr><td>IOV_WS_ALREADY_SHUTDOWN</td></tr><tr><td>IOV_WS_INFINITE_LOOP</td></tr><tr><td>IOV_WS_REENTRANT_ERROR</td></tr></tbody></table> |
`AmdSmiEventCategoryECC` | <table><tbody><tr><td>SMI_EVENT_ECC_UCE</td></tr><tr><td>SMI_EVENT_ECC_CE</td></tr><tr><td>ECC_IN_PF_FB</td></tr><tr><td>ECC_IN_CRI_REG</td></tr><tr><td>ECC_IN_VF_CRI</td></tr><tr><td>ECC_REACH_THD</td></tr><tr><td>ECC_VF_CE</td></tr><tr><td>ECC_VF_UE</td></tr><tr><td>ECC_IN_SAME_ROW</td></tr><tr><td>ECC_UMC_UE</td></tr><tr><td>ECC_GFX_CE</td></tr><tr><td>ECC_GFX_UE</td></tr><tr><td>ECC_SDMA_CE</td></tr><tr><td>ECC_SDMA_UE</td></tr><tr><td>ECC_GFX_CE_TOTAL</td></tr><tr><td>ECC_GFX_UE_TOTAL</td></tr><tr><td>ECC_SDMA_CE_TOTAL</td></tr><tr><td>ECC_SDMA_UE_TOTAL</td></tr><tr><td>ECC_UMC_CE_TOTAL</td></tr><tr><td>ECC_UMC_UE_TOTAL</td></tr><tr><td>ECC_MMHUB_CE</td></tr><tr><td>ECC_MMHUB_UE</td></tr><tr><td>ECC_MMHUB_CE_TOTAL</td></tr><tr><td>ECC_MMHUB_UE_TOTAL</td></tr><tr><td>ECC_XGMI_WAFL_CE</td></tr><tr><td>ECC_XGMI_WAFL_UE</td></tr><tr><td>ECC_XGMI_WAFL_CE_TOTAL</td></tr><tr><td>ECC_XGMI_WAFL_UE_TOTAL</td></tr><tr><td>ECC_FATAL_ERROR</td></tr><tr><td>ECC_POISON_CONSUMPTION</td></tr><tr><td>ECC_ACA_DUMP</td></tr><tr><td>ECC_WRONG_SOCKET_ID</td></tr><tr><td>ECC_ACA_UNKNOWN_BLOCK_INSTANCE</td></tr><tr><td>ECC_UNKNOWN_CHIPLET_CE</td></tr><tr><td>ECC_UNKNOWN_CHIPLET_UE</td></tr><tr><td>ECC_UMC_CHIPLET_CE</td></tr><tr><td>ECC_UMC_CHIPLET_UE</td></tr><tr><td>ECC_GFX_CHIPLET_CE</td></tr><tr><td>ECC_GFX_CHIPLET_UE</td></tr><tr><td>ECC_SDMA_CHIPLET_CE</td></tr><tr><td>ECC_SDMA_CHIPLET_UE</td></tr><tr><td>ECC_MMHUB_CHIPLET_CE</td></tr><tr><td>ECC_MMHUB_CHIPLET_UE</td></tr><tr><td>ECC_XGMI_WAFL_CHIPLET_CE</td></tr><tr><td>ECC_XGMI_WAFL_CHIPLET_UE</td></tr><tr><td>ECC_EEPROM_ENTRIES_FOUND</td></tr><tr><td>ECC_UMC_DE</td></tr><tr><td>ECC_UMC_DE_TOTAL</td></tr><tr><td>ECC_UNKNOWN</td></tr><tr><td>ECC_EEPROM_REACH_THD</td></tr><tr><td>ECC_UMC_CHIPLET_DE</td></tr><tr><td>ECC_UNKNOWN_CHIPLET_DE</td></tr><tr><td>ECC_EEPROM_CHK_MISMATCH</td></tr><tr><td>ECC_EEPROM_RESET</td></tr><tr><td>ECC_EEPROM_RESET_FAILED</td></tr><tr><td>ECC_EEPROM_APPEND</td></tr><tr><td>ECC_THD_CHANGED</td></tr><tr><td>ECC_DUP_ENTRIES</td></tr><tr><td>ECC_EEPROM_WRONG_HDR</td></tr><tr><td>ECC_EEPROM_WRONG_VER</td></tr></tbody></table> |
`AmdSmiEventCategoryPP` | <table><tbody><tr><td>PP_SET_DPM_POLICY_FAIL</td></tr><tr><td>PP_ACTIVATE_DPM_POLICY_FAIL</td></tr><tr><td>PP_I2C_SLAVE_NOT_PRESENT</td></tr><tr><td>PP_THROTTLER_EVENT</td></tr></tbody></table> |
`AmdSmiEventCategorySched` | <table><tbody><tr><td>SCHED_WORLD_SWITCH_FAIL</td></tr><tr><td>SCHED_DISABLE_AUTO_HW_SWITCH_FAIL</td></tr><tr><td>SCHED_ENABLE_AUTO_HW_SWITCH_FAIL</td></tr><tr><td>SCHED_GFX_SAVE_REG_FAIL</td></tr><tr><td>SCHED_GFX_IDLE_REG_FAIL</td></tr><tr><td>SCHED_GFX_RUN_REG_FAIL</td></tr><tr><td>SCHED_GFX_LOAD_REG_FAIL</td></tr><tr><td>SCHED_GFX_INIT_REG_FAIL</td></tr><tr><td>SCHED_MM_SAVE_REG_FAIL</td></tr><tr><td>SCHED_MM_IDLE_REG_FAIL</td></tr><tr><td>SCHED_MM_RUN_REG_FAIL</td></tr><tr><td>SCHED_MM_LOAD_REG_FAIL</td></tr><tr><td>SCHED_MM_INIT_REG_FAIL</td></tr><tr><td>SCHED_INIT_GPU_FAIL</td></tr><tr><td>SCHED_RUN_GPU_FAIL</td></tr><tr><td>SCHED_SAVE_GPU_STATE_FAIL</td></tr><tr><td>SCHED_LOAD_GPU_STATE_FAIL</td></tr><tr><td>SCHED_IDLE_GPU_FAIL</td></tr><tr><td>SCHED_FINI_GPU_FAIL</td></tr><tr><td>SCHED_DEAD_VF</td></tr><tr><td>SCHED_EVENT_QUEUE_FULL</td></tr><tr><td>SCHED_SHUTDOWN_VF_FAIL</td></tr><tr><td>SCHED_RESET_VF_NUM_FAIL</td></tr><tr><td>SCHED_IGNORE_EVENT</td></tr><tr><td>SCHED_PF_SWITCH_FAIL</td></tr></tbody></table> |
`AmdSmiEventCategoryVf` | <table><tbody><tr><td>VF_ATOMBIOS_INIT_FAIL</td></tr><tr><td>VF_NO_VBIOS</td></tr><tr><td>VF_GPU_POST_ERROR</td></tr><tr><td>VF_ATOMBIOS_GET_CLOCK_FAIL</td></tr><tr><td>VF_FENCE_INIT_FAIL</td></tr><tr><td>VF_AMDGPU_INIT_FAIL</td></tr><tr><td>VF_IB_INIT_FAIL</td></tr><tr><td>VF_AMDGPU_LATE_INIT_FAIL</td></tr><tr><td>VF_ASIC_RESUME_FAIL</td></tr><tr><td>VF_GPU_RESET_FAIL</td></tr></tbody></table> |
`AmdSmiEventCategoryVbios` | <table><tbody><tr><td>VBIOS_INVALID</td></tr><tr><td>VBIOS_IMAGE_MISSING</td></tr><tr><td>VBIOS_CHECKSUM_ERR</td></tr><tr><td>VBIOS_POST_FAIL</td></tr><tr><td>VBIOS_READ_FAIL</td></tr><tr><td>VBIOS_READ_IMG_HEADER_FAIL</td></tr><tr><td>VBIOS_READ_IMG_SIZE_FAIL</td></tr><tr><td>VBIOS_GET_FW_INFO_FAIL</td></tr><tr><td>VBIOS_GET_TBL_REVISION_FAIL</td></tr><tr><td>VBIOS_PARSER_TBL_FAIL</td></tr><tr><td>VBIOS_IP_DISCOVERY_FAIL</td></tr><tr><td>VBIOS_TIMEOUT</td></tr><tr><td>VBIOS_HASH_INVALID</td></tr><tr><td>VBIOS_HASH_UPDATED</td></tr><tr><td>VBIOS_IP_DISCOVERY_BINARY_CHECKSUM_FAIL</td></tr><tr><td>VBIOS_IP_DISCOVERY_TABLE_CHECKSUM_FAIL</td></tr></tbody></table> |
`AmdSmiEventCategoryGuard` | <table><tbody><tr><td>GUARD_RESET_FAIL</td></tr><tr><td>GUARD_EVENT_OVERFLOW</td></tr></tbody></table> |
`AmdSmiEventCategoryGpumon` | <table><tbody><tr><td>GPUMON_INVALID_OPTION</td></tr><tr><td>GPUMON_INVALID_VF_INDEX</td></tr><tr><td>GPUMON_INVALID_FB_SIZE</td></tr><tr><td>GPUMON_NO_SUITABLE_SPACE</td></tr><tr><td>GPUMON_NO_AVAILABLE_SLOT</td></tr><tr><td>GPUMON_OVERSIZE_ALLOCATION</td></tr><tr><td>GPUMON_OVERLAPPING_FB</td></tr><tr><td>GPUMON_INVALID_GFX_TIMESLICE</td></tr><tr><td>GPUMON_INVALID_MM_TIMESLICE</td></tr><tr><td>GPUMON_INVALID_GFX_PART</td></tr><tr><td>GPUMON_VF_BUSY</td></tr><tr><td>GPUMON_INVALID_VF_NUM</td></tr><tr><td>GPUMON_NOT_SUPPORTED</td></tr></tbody></table> |
`AmdSmiEventCategoryMmsch` | <table><tbody><tr><td>MMSCH_IGNORED_JOB</td></tr><tr><td>MMSCH_UNSUPPORTED_VCN_FW</td></tr></tbody></table> |
`AmdSmiEventCategoryXgmi` | <table><tbody><tr><td>XGMI_TOPOLOGY_UPDATE_FAILED</td></tr><tr><td>XGMI_TOPOLOGY_HW_INIT_UPDATE</td></tr><tr><td>XGMI_TOPOLOGY_UPDATE_DONE</td></tr><tr><td>XGMI_FB_SHARING_SETTING_ERROR</td></tr><tr><td>XGMI_FB_SHARING_SETTING_RESET</td></tr></tbody></table> |
|

`Severity is AmdSmiEventSeverity enum object with values`

Severity | Description
---|------
`HIGH` | critical error
`MED` | significant error
`LOW`   | trivial error
`WARN` | warning
`INFO` | info
`ALL`   | monitor all severity levels

Exceptions that can be thrown by `read` function:

* `AmdSmiParameterException`
* `AmdSmiTimeoutException`
* `AmdSmiLibraryException`

#### stop

Description: Any resources used by event notification for the the given device will be freed with this function. This can be used explicitly or
automatically using `with` statement, like in the examples below. This should be called either manually or automatically for every created AmdSmiEventReader object.

Input parameters: `None`

Example with manual cleanup of AmdSmiEventReader:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        event_reader = AmdSmiEventReader(processors,{AmdSmiEventCategory.RESET}, AmdSmiEventSeverity.ALL)
        while True:
            event = event_reader.read(10*1000*1000)
            gpu_bdf = amdsmi_get_gpu_device_bdf(event['dev_id'])
            vf_bdf = amdsmi_get_gpu_device_bdf(event['fcn_id'])
            print("=============== Event ================")
            print("    Time            {}".format(event['timestamp']))
            print("    Category        {}".format(event['category'].name))
            print("    Subcategory     {}".format(event['subcode'].name))
            print("    Level           {}".format(event['level'].name))
            print("    Data            {}".format(event['data']))
            print("    VF BDF          {}".format(vf_bdf))
            print("    GPU BDF         {}".format(gpu_bdf))
            print("    Date            {}".format(event['date']))
            print("    Message         {}".format(event['message']))
            print("======================================")
except AmdSmiTimeoutException:
    print("No more events")
finally:
    event_reader.stop()
```

Example with automatic cleanup using `with` statement:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        with AmdSmiEventReader(processors,{AmdSmiEventCategory.RESET}, AmdSmiEventSeverity.ALL) as event_reader:
            while True:
                event = event_reader.read(10*1000*1000)
                gpu_bdf = amdsmi_get_gpu_device_bdf(event['dev_id'])
                vf_bdf = amdsmi_get_gpu_device_bdf(event['fcn_id'])
                print("=============== Event ================")
                print("    Time            {}".format(event['timestamp']))
                print("    Category        {}".format(event['category'].name))
                print("    Subcategory     {}".format(event['subcode'].name))
                print("    Level           {}".format(event['level'].name))
                print("    Data            {}".format(event['data']))
                print("    VF BDF          {}".format(vf_bdf))
                print("    GPU BDF         {}".format(gpu_bdf))
                print("    Date            {}".format(event['date']))
                print("    Message         {}".format(event['message']))
                print("======================================")
except AmdSmiTimeoutException:
    print("No more events")
```

### amdsmi_get_lib_version

Description: Get the build version information for the currently running build of AMDSMI.

Output: amdsmi build version

Exceptions that can be thrown by `amdsmi_get_lib_version` function:

* `AmdSmiLibraryException`
* `AmdSmiRetryException`
* `AmdSmiParameterException`

Example:

```python
try:
    devices = amdsmi_get_processor_handles()
    if len(devices) == 0:
        print("No GPUs on machine")
    else:
        for device in devices:
            version = amdsmi_get_lib_version()
            print(version)
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_accelerator_partition_profile_config

Description: Returns gpu accelerator partition caps as currently configured in the system

Input parameters:

* `processor handle` PF of a GPU device

Output: Dictionary with fields

Field | Description
---|---
`resource_profiles (dictionary)` | <table>  <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`profile_index`</td><td> index of a profile </td></tr><tr><td>`resource_type`</td><td> type of a resource from `AmdSmiAcceleratorPartitionResource` enum </td></tr><tr><td>`partition_resource`</td><td> the resources a partition can use, which may be shared </td></tr><tr><td>`num_partitions_share_resource`</td><td> number of partitions that share resource of that type </td></tr></tbody></table>
`default_profiles_index` | index of the default profile
`profiles (dictionary)` | <table>  <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`profile_type`</td><td> profile type from `AmdSmiAcceleratorPartitionSetting` enum </td></tr><tr><td>`num_partitions`</td><td> number of partitions in the profile </td></tr><tr><td>`memory_caps`</td><td> memory capabilities of the profile </td></tr><tr><td>`profile_index`</td><td> index of the profile </td></tr><tr><td>`resources`</td><td> resources in the profile </td></tr></tbody></table>

`AmdSmiAcceleratorPartitionResource` enum:

Field | Description
---|---
`XCC`      | xcc resource capabilities
`ENCODER`  | encoder resource capabilities
`DECODER`  | decoder resource capabilities
`DMA`      | dma resource capabilities
`JPEG`     | jpeg resource capabilities

`AmdSmiAcceleratorPartitionSetting` enum:

Field | Description
---|---
`INVALID`  | invalid compute partition
`SPX`      | compute partition with all xccs in group (8/1)
`DPX`      | compute partition with four xccs in group (8/2)
`TPX`      | compute partition with two xccs in group (6/3)
`QPX`      | compute partition with two xccs in group (8/4)
`CPX`      | compute partition with one xcc in group (8/8)

Exceptions that can be thrown by `amdsmi_get_gpu_accelerator_partition_profile_config` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            accelerator_partition_config = amdsmi_get_gpu_accelerator_partition_profile_config(processor)
            print(accelerator_partition_config)
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_accelerator_partition_profile

Description: Returns current gpu accelerator partition cap

Input parameters:

* `processor handle` PF of a GPU device

Output: Dictionary with fields

Field | Description
---|---
`profile (dictionary)` | <table>  <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`profile_type`</td><td> current profile type from `AmdSmiAcceleratorPartitionSetting` enum </td></tr><tr><td>`num_partitions`</td><td> number of partitions in the profile </td></tr><tr><td>`memory_caps`</td><td> memory capabilities of the profile </td></tr><tr><td>`profile_index`</td><td> index of the profile </td></tr><tr><td>`resources`</td><td> resources in the profile </td></tr></tbody></table>
`partition_id` | array of ids for current accelerator profile

`AmdSmiComputePartitionResource` enum:

Field | Description
---|---
`XCC`      | xcc resource capabilities
`ENCODER`  | encoder resource capabilities
`DECODER`  | decoder resource capabilities
`DMA`      | dma resource capabilities
`JPEG`     | jpeg resource capabilities

Exceptions that can be thrown by `amdsmi_get_gpu_accelerator_partition_profile` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            accelerator_partition_profile = amdsmi_get_gpu_accelerator_partition_profile(processor)
            print(accelerator_partition_profile)
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_memory_partition_config

Description: Returns current gpu memory partition config and mode capabilities

Input parameters:

* `processor handle` PF of a GPU device

Output: Dictionary with fields

Field | Description
---|---
`partition_caps` | memory partition capabilities
`mp_mode` | memory partition mode from `AmdSmiMemoryPartitionSetting` enum
`numa_range (dictionary)` | <table>  <thead><tr> <th> Subfield </th> <th> Description</th></tr></thead><tbody><tr><td>`memory_type`</td><td> memory type from `AmdSmiVramType` enum </td></tr><tr><td>`start`</td><td> start of numa range </td></tr><tr><td>`end`</td><td> end of numa range </td></tr></tbody></table>

'AmdSmiMemoryPartitionSetting' enum:

Field | Description
---|---
`UNKNOWN` | unknown memory partition
`NPS1` | memory partition with 1 number per socket
`NPS2` | memory partition with 2 numbers per socket
`NPS4` | memory partition with 4 numbers per socket
`NPS8` | memory partition with 8 numbers per socket

Exceptions that can be thrown by `amdsmi_get_gpu_memory_partition_config` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            memory_partition_config = amdsmi_get_gpu_memory_partition_config(processor)
            print(memory_partition_config)
except AmdSmiException as e:
    print(e)
```

### amdsmi_set_gpu_accelerator_partition_profile

Description: Sets accelerator partition setting based on profile_index from `amdsmi_get_gpu_accelerator_partition_profile_config`

Input parameters:

* `processor handle` PF of a GPU device
* `profile_index` Represents index of a partition user wants to set

Output:

* `None`

Exceptions that can be thrown by `amdsmi_set_gpu_accelerator_partition_profile` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            amdsmi_set_gpu_accelerator_partition_profile(processor, 1)

except AmdSmiException as e:
    print(e)
```

### amdsmi_set_gpu_memory_partition_mode

Description: Sets memory partition mode

Input parameters:

* `processor handle` PF of a GPU device
* `setting` Enum from `AmdSmiMemoryPartitionSetting` representing memory partitioning mode to set

`AmdSmiMemoryPartitionSetting` enum:

Field | Description
---|---
`UNKNOWN` | unknown memory partition
`NPS1` | memory partition with 1 number per socket
`NPS2` | memory partition with 2 numbers per socket
`NPS4` | memory partition with 4 numbers per socket
`NPS8` | memory partition with 8 numbers per socket

Output:

* `None`

Exceptions that can be thrown by `amdsmi_set_gpu_memory_partition_mode` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            amdsmi_set_gpu_memory_partition_mode(processor, AmdSmiMemoryPartitionSetting.NPS1)
except AmdSmiException as e:
    print(e)
```

### amdsmi_get_gpu_cper_entries

Description: Get gpu ras cper entries

Input parameters:

* `processor handle` PF of a GPU device
* `severity_mask` Represents different severity masks from 'AmdSmiCperErrorSeverity' enum on which filerting of cpers is based.

Field | Description
---|---
`NON_FATAL_UNCORRECTED` | filters non-fatal-uncorrected cpers
`FATAL` | filters fatal cpers
`NON_FATAL_CORRECTED` | filters non_fatal_corrected cpers
`NUM` | shows all cper types

Output:

* List of all cper errors. Each list element contains binary raw data

Exceptions that can be thrown by `amdsmi_get_gpu_cper_entries` function:

* `AmdSmiLibraryException`
* `AmdSmiParameterException`

Example:

```python
try:
    processors = amdsmi_get_processor_handles()
    if len(processors) == 0:
        print("No GPUs on machine")
    else:
        for processor in processors:
            cper_list = amdsmi_gpu_get_cper_entries(processor, AmdSmiCperErrorSeverity.NUM)

except AmdSmiException as e:
    print(e)
```
