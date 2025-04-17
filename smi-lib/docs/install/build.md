---
myst:
  html_meta:
    "description lang=en": "How to build AMD SMI from source."
    "keywords": "system, management, interface, contribute, contributing, develop, testing, C, Python"
---

<a id="amd-smi-library-build"></a>

# AMD SMI LIBRARY BUILD

## Requirements

Before building the integration and unit tests, ensure that `gtest` and `gmock` are installed on your system. These libraries are required for building and running the tests. You can install them using your system's package manager or build them from source.

## Build commands

When running make inside the gim folder, the AMD SMI library is built as well. Here are some useful commands for building the AMD SMI library:

- Run `make` in the smi-lib folder to build the library.
- Run `make package` to create the AMD SMI Python package.
- Run `make test` to build and run the integration and unit tests.
- Run `make all` to build everything mentioned above.
- Run `make gen_coverage` to calculate the code coverage of the AMD SMI library.
- If any changes are made to the interface folder, regenerate the Python wrapper by running `make python_wrapper` and replace the `amdsmi_wrapper.py` file in the py/interface folder with the one generated in the build folder `build/amdsmi/amdsmi_wrapper/amdsmi_wrapper.py`.

### Build Options

These options allow you to customize the build process, such as specifying the build type, enabling thread safety, enabling logging, and using the Thread Sanitizer.

- BUILD_TYPE:

This option specifies the type of build you want to perform. Common values are Release and Debug.
Release builds are optimized for performance and do not include debugging information.
Debug builds include debugging information and are not optimized, making them suitable for development and debugging.
Default: Release

- THREAD_SAFE:

This option indicates whether the build should include thread safety features.
When set to True, thread safety mechanisms (e.g., mutexes, locks) are enabled.
When set to False, thread safety mechanisms are disabled, which might improve performance but can lead to race conditions in multi-threaded environments.
Default: True

- LOGGING:

This option controls whether logging is enabled in the build.
When set to True, logging code is included, which can help with debugging and monitoring.
When set to False, logging code is excluded, which might improve performance.
Default: False

- THREAD_SANITIZER:

This option indicates whether the Thread Sanitizer should be enabled.
Thread Sanitizer is a tool that detects data races in multi-threaded programs.
When set to True, the build includes Thread Sanitizer instrumentation.
When set to False, Thread Sanitizer is not included.
Default: False

- ADDRESS_SANITIZER:

This option indicates whether the Address Sanitizer should be enabled.
Address Sanitizer is a tool that detects memory errors such as buffer overflows, use-after-free, and memory leaks.
When set to True, the build includes Address Sanitizer instrumentation.
When set to False, Address Sanitizer is not included.
Default: False

## Folder structure

Library folder structure is shown below:

```text
smi-lib/
├── build/                # Contains all generated files during build
├── cli/
│   └── cpp/              # CLI C++ source code
├── dl/                   # Contains downloaded packages during build
├── drv/
│   ├── core/             # Core driver SMI code
│   ├── inc/              # Driver SMI headers
│   └── linux/            # Linux platform-specific driver code
├── examples/             # Examples of using C APIs
├── inc/                  # Internal include files
│   ├── common/           # Interface between driver and SMI library
│   └── linux/            # Header files specific to the Linux platform
├── interface/            # C API interface for clients
├── py/
│   └── interface/        # Python interface and wrapper around C APIs
├── src/                  # C implementation
├── tests/                # Library gtest/gmock tests
│   ├── integration/      # Integration tests
│   └── unit/             # Unit tests
└── utils/
  └── scripts/          # Utility scripts
```

## Python wrapper

AMD SMI stack contains a wrapper around C SMI Library. The Python API is a one-to-one mapping to the C interface of SMI Library. It exposes library functionality into Python language, allowing for fast and easy scripting.

### Code

The Python Wrapper source code can be found in smi-lib/py/interface folder.

### Build

The wrapper is built together with the SMI Library. For detailed instructions, refer to the [AMD SMI LIBRARY BUILD](#amd-smi-library-build) section.

### Code style

The code style follows the Python PEP-8 standard. See [PEP-8](https://www.python.org/dev/peps/pep-0008/) for details.

### Dependencies

- Requires `Python 3.10` or higher.
- Python Wrapper relies on `ctypes` extension of Python language.
  See [CTYPES]<https://docs.python.org/3/library/ctypes.html> for details. By using this extension python code can load shared library and call its functions. The extension comes by default with Python 3.

### Flow

1. The flow starts with the C library `libamdsmi.so`, which contains the functionality to be used in Python.
2. A Python wrapper (`amdsmi_wrapper.py`) loads the `libamdsmi.so` library and provides a Python interface to its functions.
3. Another wrapper (`amdsmi_interface.py`) provides a user-friendly interface for interacting with the library.

### Regenerate wrapper

If any changes are made to the C interface (smi-lib/interface/amdsmi.h), regenerate the Python wrapper by running `make python_wrapper` and replace the `amdsmi_wrapper.py` file in the py/interface folder with the one generated in the build folder `build/amdsmi/amdsmi_wrapper/amdsmi_wrapper.py`.

## Python package

The AMD SMI Python package is a Python package that provides a wrapper around the AMD System Management Interface (SMI) library. It allows you to interact with AMD GPUs (Graphics Processing Units) and retrieve various information and metrics related to GPU performance, temperature, power consumption, and more.
To use the AMD SMI Python package, you need to have the AMD GPU driver installed on your system. The package interacts with the AMD SMI C library, to communicate with the GPU hardware.
On a Linux platform, go to the smi-lib directory `gim/smi-lib/` and run the following commands:

`make clean` - cleaning build/ directory
`make package -j$(nproc)` - building AMD SMI library and getting AMD SMI Python package on the following path: `gim/smi-lib/build/amdsmi/package/Release/amdsmi`
