---
myst:
  html_meta:
    "description lang=en": "How to install AMD SMI library and tool from source."
    "keywords": "system, management, interface, contribute, contributing, develop, testing, C, tool, c++"
---

<a id="amd-smi-library-tool-installation"></a>

# Install AMD SMI on Linux SR-IOV hosts

## GIM package installation:

Installing [GIM](https://github.com/amd/MxGPU-Virtualization) from the package will automatically build and install the AMD SMI library and tool for the user. This automation eliminates the need for manual execution of the tool build and installation steps, simplifying the setup process.

- After GIM package installation, the `libamdsmi.so` library is located in the system library directory, which is `/usr/local/lib/`. This ensures that the library is accessible system-wide, allowing applications and tools to link against it without requiring additional configuration.

- The `amd-smi` tool will be installed in `/usr/local/bin`, making it available for execution from any directory in the terminal without needing to specify the full path.

If you prefer to manually build and install the SMI library and tool from the source code, follow the steps in [Build from source](./build.md).

## Installation commands

After a successful build of AMD SMI library and tool, the following commands will install them on the system:

### AMD SMI library:

- Run `sudo make install` in the smi-lib folder to install the compiled library (`libamdsmi.so`) and the header file (`amdsmi.h`) to the system library folder (`/usr/local/lib/`) and system include folder (`/usr/local/inc/`), respectively.
- Run `sudo make uninstall` in the smi-lib folder to remove the installed library (`libamdsmi.so`) and header file (`amdsmi.h`) from the system library folder (`/usr/local/lib/`) and system include folder (`/usr/local/inc/`), respectively.

### `amd-smi` tool:

- Run `sudo make install` in the smi-lib/cli/cpp folder to install the compiled tool (`amd-smi`) to the system bin folder (`/usr/local/bin/`).
- Run `sudo make uninstall` in the smi-lib/cli/cpp folder to uninstall the installed tool (`amd-smi`) from the system bin folder (`/usr/local/bin/`).
