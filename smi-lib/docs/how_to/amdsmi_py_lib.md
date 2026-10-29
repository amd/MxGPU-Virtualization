---

myst:
  html_meta:
    "description lang=en": "Use the AMD SMI Python interface and run examples for lifecycle management, discovery, monitoring, topology, virtualization, and GPU configuration."
    "keywords": "amd, smi, python, API, examples, GPU, NIC, SR-IOV, monitoring, configuration"

---

# AMD SMI Python library usage

The AMD SMI Python package provides a Python interface to the AMD System
Management Interface (SMI) library. Use it to initialize AMD SMI, discover
available processors, query hardware and firmware information, monitor
telemetry, and perform supported configuration operations.

## Basic library usage

From the `smi-lib` repository root, run `make package`, then make the generated
`amdsmi` directory and its matching AMD SMI shared library available to your
program. An application must initialize AMD SMI before making API calls, obtain
a processor handle for GPU-specific operations, and shut down the library when
it finishes.

```python
from amdsmi import *

try:
    amdsmi_init()

    processors = amdsmi_get_processor_handles()
    print(f"Found {len(processors)} processor(s)")
    if processors:
        amdsmi_get_gpu_asic_info(processors[0])
except AmdSmiException as e:
    print(e)
finally:
    try:
        amdsmi_shut_down()
    except AmdSmiException as e:
        print(e)
```

`amdsmi_get_processor_handles()` returns the GPU processor handles available on
the current machine. Pass one of these handles to APIs that operate on a
specific GPU, such as `amdsmi_get_gpu_asic_info()`.

## Prerequisites

The Python example scripts in this guide are intended for Linux.

Before running any example, make sure that:

- Python 3.10 or later is available.
- The AMD SMI Python package and its matching AMD SMI shared library are built
and available on the system.
- The AMD SR-IOV Host driver is loaded.
- The example runs with administrator privileges, using `sudo`. AMD SMI
requires them to access the driver device.

Most examples query one or more AMD GPUs. The lifecycle example can run without
a GPU processor handle. Individual examples identify their additional hardware,
driver, operating-system, or build requirements for optional capabilities such
as NICs, SR-IOV VFs, XGMI, and node power management.

## Build the Python package

From the `smi-lib` repository root, build the AMD SMI Python package:

```shell
make package
```

The package is created in
`build/amdsmi/package/<BUILD_MODE>/amdsmi`, where `<BUILD_MODE>` is `Release`
or `Debug`.

To build the package and prepare the bundled examples in one step, run the
following command. It places the `amdsmi` directory in `py/examples/`, beside
the example scripts, and replaces any existing copy:

```shell
make python_examples
```

The default build mode is `Release`. The Python wrapper loads `libamdsmi.so` from the copied `amdsmi` directory, so
it is available alongside each example script.

## Run an example

From the `smi-lib` repository root, run the selected script with Python:

```shell
sudo python3 py/examples/<example>/amdsmi_<example>.py
```

The available processors, values, and optional capabilities depend on the host
hardware, driver, firmware, and build configuration. Examples report
unsupported optional APIs where possible and continue with their remaining
operations.

## Examples

All examples are documented in [AMD SMI examples](amdsmi_examples.md).