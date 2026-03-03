---
myst:
  html_meta:
    "description lang=en": "How to build AMD SMI from source."
    "keywords": "system, management, interface, contribute, contributing, develop, testing, C, Python"
---

<a id="amd-smi-library-build"></a>

# Build the AMD SMI library (virtualization)

## Requirements

Before building the integration and unit tests, ensure that `gtest` and `gmock`
are installed on your system. These libraries are required to build and
run the tests. You can install them using your system's package manager or
build them from source.

Additionally, ensure that the `lcov` package is installed on your system before
running the `gen_coverage` command, as it is necessary for generating code
coverage reports. Minimum supported `lcov` version is 1.15.

## Build commands

### AMD SMI library build

Running `make` inside the `gim/` folder builds the AMD SMI library as well. Here are some useful commands for building the AMD SMI library:

- Run `make` in the `smi-lib/` folder to build the library.

- Run `make package` to create the AMD SMI Python package.

- Run `make test` to build and run the integration and unit tests.

- Run `make all` to build everything mentioned above.

- Run `make gen_coverage` to calculate the code coverage of the AMD SMI
  library.

- If any changes are made to the `interface/` folder, regenerate the Python
  wrapper by running `make python_wrapper` and replace the `amdsmi_wrapper.py`
  file in the py/interface folder with the one generated in the build folder
  `build/amdsmi/amdsmi_wrapper/amdsmi_wrapper.py`.

## Build options

These options allow you to customize the build process, such as specifying the build type, enabling thread safety, enabling logging, and using the Thread Sanitizer.

- `BUILD_TYPE`

   This option specifies the type of build you want to perform. Common values are `Release` and `Debug`.
   Release builds are optimized for performance and do not include debugging information.
   Debug builds include debugging information and are not optimized, making them suitable for development and debugging.
   Default: `Release`

- `THREAD_SAFE`

   This option indicates whether the build should include thread safety features.
   When set to `True`, thread safety mechanisms (for example, mutexes, locks) are enabled.
   When set to `False`, thread safety mechanisms are disabled, which might improve performance but can lead to race conditions in multi-threaded environments.
   Default: `True`

- `LOGGING`

   This option controls whether logging is enabled in the build.
   When set to `True`, logging code is included, which can help with debugging and monitoring.
   When set to `False`, logging code is excluded, which might improve performance.
   Default: `False`

- `THREAD_SANITIZER`

   This option indicates whether the Thread Sanitizer should be enabled.
   Thread Sanitizer is a tool that detects data races in multi-threaded programs.
   When set to `True`, the build includes Thread Sanitizer instrumentation.
   When set to `False`, Thread Sanitizer is not included.
   Default: `False`

- `ADDRESS_SANITIZER`

   This option indicates whether the Address Sanitizer should be enabled.
   Address Sanitizer is a tool that detects memory errors such as buffer overflows, use-after-free, and memory leaks.
   When set to `True`, the build includes Address Sanitizer instrumentation.
   When set to `False`, Address Sanitizer is not included.
   Default: `False`

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

The Python Wrapper source code can be found in `smi-lib/py/interface` folder.

### Build

The wrapper is built together with the AMD SMI library. For detailed instructions, refer to the [AMD SMI LIBRARY BUILD](#amd-smi-library-build) section.

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

## AMD SMI tool build

The AMD SMI CLI tool is a command line utility built in C++ that utilizes AMD SMI Library APIs to monitor and configure AMD GPUs and NICs on Linux host systems.

#### Tool Source Code Structure

The CLI tool source code is organized in a structured hierarchy designed for maintainability and platform-specific implementations.

##### Folder Structure

```text
smi-lib/
└── cli/
    └── cpp/
        ├── cmake/                # Contains all CMake files used in the build
        │   └── linux/
        ├── docs/
        │   └── external/         # Contains documents
        ├── inc/                  # Internal include files
        ├── src/                  # Source files
        │   ├── guest/            # Windows Guest-specific source files
        │   └── host/             # Host-specific source files
        └── utils/
            ├── scripts/          # Utility scripts
            └── third_party/
                └── inc/          # Third-party libraries
                    ├── json/
                    └── tabulate/
```

##### Key components

**Include files (`inc/`)**
Contains all header files that define the CLI tool's interfaces, including:
- Command parsers and handlers for GPU and NIC operations
- API interface definitions for GPU and NIC management
- Template definitions for output formatting
- Helper functions and utilities

**Source files (`src/`)**
- **`host/`**: Contains Linux host-specific implementations for GPU and NIC management and monitoring
- **`guest/`**: Contains Windows guest-specific source files (for cross-platform compatibility)

**Build system (`cmake/`)**
- **`linux/`**: Linux-specific CMake configuration files
- Platform-specific build configurations and dependencies

**Third-party libraries (`utils/third_party/`)**
- **`json/`**: JSON parsing and formatting library. Converts internal data structures to properly formatted JSON objects for machine-readable output.
- **`tabulate/`**: Table formatting library for structured output. Handles the alignment, spacing, and visual formatting of tabular data (like the monitor command output showing GPU metrics in neat columns).


#### Build requirements

**Prerequisites**
- Modern C++ compiler (g++11)
- CMake 3.16 or higher
- AMD SMI Library development files
- Linux kernel headers (for host functionality)

**Dependencies**
- AMD SMI Library (libamdsmi)
- Standard C++ libraries
- JSON library (bundled in third_party)
- Tabulate library (bundled in third_party)

#### Build Process

**Basic build steps**

- Run `make` in the `smi-lib/cli/cpp` folder to build the tool.
- Run `make clean` to remove all files generated during the build process, such as object files and executables, to ensure clean build environment.

#### Build Options

The CLI tool build system supports various configuration options to customize the build for different environments and hardware support requirements.

**NIC support configuration**

The CLI tool can be built with or without NIC (Network Interface Card) support depending on your system requirements:

**Build with NIC support (default)**
```bash
cd smi-lib/cli/cpp
make
```

**Build with NIC support using build flag**
```bash
cd smi-lib/cli/cpp
make AMD_SMI_NIC_SUPPORT=True ..
```

**Build without NIC support**
```bash
cd smi-lib/cli/cpp
make AMD_SMI_NIC_SUPPORT=False ..
```

**Available build options**
- `AMD_SMI_NIC_SUPPORT`: Enable/disable NIC monitoring functionality (Default: True)

**Notes:**
- When NIC support is disabled (`AMD_SMI_NIC_SUPPORT=False`), all NIC-related commands and functionality will be excluded from the build

#### Output Location

After successful compilation, the `amd-smi` binary will be generated in: `smi-lib/cli/cpp/build/` (build directory).

#### Runtime Requirements

**Library dependencies**
The CLI tool requires the AMD SMI Library (`libamdsmi.so`) to be available either:
- In the same directory as the `amd-smi` binary
- In the system library path (`/usr/local/lib`)
- Via `LD_LIBRARY_PATH` environment variable

**Driver requirements**
- AMD SR-IOV Host driver must be installed and loaded
- For SRIOV functionality: SR-IOV must be enabled in the system

**NIC support requirements**
- AMD Pensando NIC drivers must be installed and loaded
- Appropriate permissions for network device access

#### Development Notes

**Code organization**
- Each command is implemented as a separate class inheriting from a base command interface
- Template-based output formatting ensures consistent display across all commands
- Platform-specific code is isolated in respective directories (`host/` vs `guest/`)

**Extending functionality**
To add new commands or features:
1. Create new command class in appropriate `src/` subdirectory
2. Add corresponding header file in `inc/`
3. Update command parser to recognize new commands

#### Troubleshooting

**Common build issues**
- **Missing AMD SMI library**: Ensure `libamdsmi.so` is built and available
- **CMake version**: Verify CMake 3.16+ is installed

**Runtime issues**
- **Library not found**: Check `LD_LIBRARY_PATH` includes AMD SMI Library location
- **Permission errors**: Ensure proper permissions for GPU device access
- **Driver issues**: Verify AMD SR-IOV Host driver is properly installed and loaded

**NIC-related issues**
- **NIC not detected**: Verify AMD Pensando NIC drivers are installed and loaded
- **Network permission errors**: Ensure proper network device access permissions
