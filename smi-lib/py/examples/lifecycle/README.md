# Run the AMD SMI lifecycle example

The lifecycle example is a small starting point for an AMD SMI application. It
initializes AMD SMI, displays the library version, converts status codes to
readable text, and closes the library before exiting. It does not modify device
state or require a GPU processor handle.

`py/examples/lifecycle/amdsmi_lifecycle.py` is the Python implementation.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. This example has no GPU-specific requirements.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/lifecycle/amdsmi_lifecycle.py
```

The library version and status descriptions depend on the installed AMD SMI
release. The example always attempts to shut down the library before exiting.

## Example flow

The program follows this lifecycle:

```python
from amdsmi import *

try:
    init_flags = AmdSmiInitFlags.INIT_ALL_PROCESSORS
    amdsmi_init(init_flags)

    lib_data = amdsmi_get_lib_version()

    status = AmdSmiRetCode.SUCCESS
    amdsmi_status_code_to_string(status)

    status = AmdSmiRetCode.STATUS_NOT_INIT
    amdsmi_status_code_to_string(status)
except AmdSmiException as e:
    print(e)
finally:
    try:
        amdsmi_shut_down()
    except AmdSmiException as e:
        print(e)
```

### Initialize AMD SMI

`amdsmi_init()` must be called before APIs that access the AMD SMI library.
The example explicitly passes the all-processors init flag, which is also the
default when no argument is supplied.

The example uses `AmdSmiInitFlags.INIT_ALL_PROCESSORS`. `AmdSmiInitFlags`
exposes the following values:

| Flag                  | Intended processor discovery scope        |
| --------------------- | ----------------------------------------- |
| `INIT_ALL_PROCESSORS` | All processors                            |
| `INIT_AMD_CPUS`       | AMD CPUs                                  |
| `INIT_AMD_GPUS`       | AMD GPUs                                  |
| `INIT_NON_AMD_CPUS`   | Non-AMD CPUs                              |
| `INIT_NON_AMD_GPUS`   | Non-AMD GPUs                              |
| `INIT_AMD_APUS`       | AMD APUs, combining AMD CPU and GPU flags |
| `INIT_AMD_NICS`       | AMD NICs                                  |

Keep `INIT_ALL_PROCESSORS` in this example unless the implementation gains
support for selecting a discovery scope.

The current native implementation does not use the supplied initialization
flags, and GPU discovery is the currently supported processor discovery path.

### Read the library version

`amdsmi_get_lib_version()` returns the version of the AMD SMI library:

```python
lib_data = amdsmi_get_lib_version()
print("lib_data['major'] is: {}".format(lib_data['major']))
print("lib_data['minor'] is: {}".format(lib_data['minor']))
print("lib_data['release'] is: {}".format(lib_data['release']))
```

This is the AMD SMI library version, not the GPU driver or firmware version.

### Decode a status code

The Python interface raises `AmdSmiException` subclasses when an API call
fails. `amdsmi_status_code_to_string()` converts a known `AmdSmiRetCode` value
into the corresponding human-readable description.

The example displays descriptions for `SUCCESS` and `STATUS_NOT_INIT` without
causing an error.

```python
status = AmdSmiRetCode.SUCCESS
print("{} is: {}".format(
    status.name, amdsmi_status_code_to_string(status)))
status = AmdSmiRetCode.STATUS_NOT_INIT
print("{} is: {}".format(
    status.name, amdsmi_status_code_to_string(status)))
```

To decode a status from an `AmdSmiLibraryException`, use its `err_code` value:

```python
except AmdSmiLibraryException as e:
    status = AmdSmiRetCode(e.err_code)
    print("{}: {}".format(
        status.name, amdsmi_status_code_to_string(status)))
```

### Shut down AMD SMI

`amdsmi_shut_down()` releases AMD SMI resources and closes its driver
connection. For each successful `amdsmi_init()` call, make a matching shutdown
call.

The script places shutdown in a `finally` block so that cleanup is attempted
whether the preceding operations succeed or raise an exception.

```python
finally:
        try:
            amdsmi_shut_down()
        except AmdSmiException as e:
            print(e)
```
