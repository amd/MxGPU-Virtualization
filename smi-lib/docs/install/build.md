---
myst:
  html_meta:
    "description lang=en": "How to build AMD SMI from source."
    "keywords": "system, management, interface, contribute, contributing, develop, testing, C, Python, Go"
---

<a id="amd-smi-library-build"></a>

# AMD SMI LIBRARY AND TOOL BUILD

## Requirements

Before building the integration and unit tests, ensure that `gtest` and `gmock` are installed on your system. These libraries are required for building and running the tests. You can install them using your system's package manager or build them from source. Additionally, ensure that the `lcov` package is installed on your system before running the gen_coverage command, as it is necessary for generating code coverage reports.

Minimum supported `lcov` version is 1.15.

## Build commands

### AMD SMI library build

When running make inside the gim folder, the AMD SMI library is built as well. Here are some useful commands for building the AMD SMI library:

- Run `make` in the smi-lib folder to build the library.
- Run `make package` to create the AMD SMI Python package.
- Run `make test` to build and run the integration and unit tests.
- Run `make all` to build everything mentioned above.
- Run `make gen_coverage` to calculate the code coverage of the AMD SMI library.
- If any changes are made to the interface folder, regenerate the Python wrapper by running `make python_wrapper` and replace the `amdsmi_wrapper.py` file in the py/interface folder with the one generated in the build folder `build/amdsmi/amdsmi_wrapper/amdsmi_wrapper.py`.

## AMD SMI LIBRARY Build Options

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

**Fabric telemetry (UALOE):**

Fabric telemetry support is auto-detected at build time. When the UALOE
source tree is present and the netlink system libraries (`libnl-3.0`,
`libnl-genl-3.0`, `libmnl`) are installed, the fabric telemetry APIs
(`amdsmi_alloc_fabric_telemetry`, `amdsmi_get_fabric_telemetry_data`,
`amdsmi_free_fabric_telemetry`) are compiled in. Otherwise those APIs
return `AMDSMI_STATUS_NOT_SUPPORTED` (fabric topology information is
still available). No build flag is required.

**NIC optional dependencies:**

The NIC subsystem (built when `AMD_SMI_NIC_SUPPORT=True`) has two optional
third-party dependencies that are auto-detected by CMake. The build always
succeeds without them; only the affected NIC APIs degrade.

| Package | Used by | Behaviour when missing |
|---------|---------|------------------------|
| `libmnl` (`libmnl-dev` / `libmnl-devel`) | `amdsmi_get_nic_fw_info`, `amdsmi_get_nic_port_info` (devlink netlink, via the internal `nl/` helper library) | `amdsmi_get_nic_fw_info` returns `AMDSMI_STATUS_NOT_SUPPORTED`; `amdsmi_get_nic_port_info` still succeeds but reports devlink-sourced data as `"N/A"`. Other NIC APIs are unaffected. |
| `libibverbs` (`libibverbs-dev` / `libibverbs-devel`, plus `libibverbs.so.1` at runtime) | `amdsmi_get_nic_port_info` (RDMA query via `dlopen` of the hardcoded SONAME `libibverbs.so.1`) | `amdsmi_get_nic_port_info` still succeeds but reports RDMA-sourced data as N/A. Other NIC APIs are unaffected. |

Install both packages to enable the full NIC feature set:

```bash
# Debian/Ubuntu
sudo apt install libmnl-dev libibverbs-dev libibverbs1

At configure time CMake prints `libmnl found -- netlink is supported`
or `libibverbs headers found` when each library is detected; otherwise
a `WARNING` line indicates which NIC features will be unavailable.

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
├── go/                   # Go bindings and example program
│   ├── amdsmi/           # Go interface (CGO wrappers) around C APIs
│   └── examples/         # Examples of using the Go API
├── inc/                  # Internal include files
│   ├── common/           # Interface between driver and SMI library
│   └── linux/            # Header files specific to the Linux platform
├── interface/            # C API interface for clients
├── nic/                  # NIC library (C++)
│   ├── interface/        # C wrapper bridging the host library and NIC library
│   ├── inc/              # Internal NIC headers
│   └── src/              # NIC implementation (sysfs, ethtool, devlink)
├── nl/                   # Netlink helper library used by the NIC subsystem
│   ├── include/          # Netlink headers
│   └── src/              # Netlink implementation
├── py/
│   └── interface/        # Python interface and wrapper around C APIs
├── src/                  # C implementation
│   └── nic/              # C API NIC wrapper around the NIC library
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

## Go bindings

AMD SMI also provides Go bindings that expose the C library to Go applications via CGO. The Go API is a thin, idiomatic mapping over the C interface, allowing developers to build management and monitoring tools in Go.

### Code

The Go bindings source code can be found in the `smi-lib/go/` folder:

* `smi-lib/go/amdsmi/amdsmi_interface.go` - AMD SMI library Go interface (types, constants, functions)
* `smi-lib/go/examples/usage.go` - Example program demonstrating the API
* `smi-lib/go/go.mod` - Go module definition (`module: smi-lib/go`)

### Build

The Go bindings link against the AMD SMI C library at run time. There is no separate Makefile target to build the Go module; instead, Go's own toolchain is used.

To run the example program from the `smi-lib/go/` directory:

```bash
sudo go run examples/usage.go
```

If `libamdsmi.so` is not in a standard library path (e.g. `/usr/local/lib`), point `LD_LIBRARY_PATH` at its location:

```bash
export LD_LIBRARY_PATH=/path/to/libamdsmi:$LD_LIBRARY_PATH
sudo go run examples/usage.go
```

To build a standalone Go binary that depends on the AMD SMI library:

```bash
cd smi-lib/go
go build -o amdsmi_demo ./examples
```

### Code style

The code style follows standard Go conventions; run `gofmt` (or `go fmt ./...`) before submitting changes.

### Dependencies

* Requires Go 1.18+ (64-bit).
* CGO must be enabled (default when a C toolchain such as `gcc` is present).
* The AMD SMI C library (`libamdsmi.so`) must be built and discoverable at link/run time (e.g. on `LD_LIBRARY_PATH` or in `/usr/local/lib`).

### Flow

1. The flow starts with the C library `libamdsmi.so`, which contains the functionality to be used in Go.
2. The Go package `smi-lib/go/amdsmi` (defined in `amdsmi_interface.go`) calls into the C library directly via CGO.
3. Application code imports `smi-lib/go/amdsmi` and uses the idiomatic Go API (typed structs, `error` returns) on top of the C interface.

## AMD SMI tool build

The AMD SMI CLI tool is a command line utility built in C++ that utilizes AMD SMI Library APIs to monitor and configure AMD GPUs and NICs on Linux host systems.

#### Tool Source Code Structure

The CLI tool source code is organized in a structured hierarchy designed for maintainability and platform-specific implementations.

##### Folder Structure

```text
cli/
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

##### Key Components

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


#### Build Requirements

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
- Currently supported NICs: AMD AI NIC Pensando Pollara 400 (`ionic`, `ionic_rdma` drivers) and Broadcom Thor2 / BCM57608 (`bnxt_en`, `bnxt_re` drivers)
- The corresponding NIC drivers must be installed and loaded
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
- **NIC not detected**: Verify the NIC drivers are installed and loaded - AMD Pensando (`ionic`, `ionic_rdma`) or Broadcom (`bnxt_en`, `bnxt_re`)
- **Network permission errors**: Ensure proper network device access permissions