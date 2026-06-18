---
myst:
  html_meta:
    "description lang=en": "How to install AMD SMI library and tool from source."
    "keywords": "system, management, interface, contribute, contributing, develop, testing, C, tool, c++"
---

<a id="amd-smi-library-tool-installation"></a>

# AMD SMI LIBRARY AND TOOL INSTALLATION

## Note on GIM Package Installation:

Installing GIM from the package will automatically build and install the AMD SMI library and tool for the user. This automation eliminates the need for manual execution of the tool build and installation steps, simplifying the setup process.

- After GIM package installation, the `libamdsmi.so` library is located in the system library directory, which is `/usr/local/lib/`. This ensures that the library is accessible system-wide, allowing applications and tools to link against it without requiring additional configuration.

- The `amd-smi` tool will be installed in `/usr/local/bin`, making it available for execution from any directory in the terminal without needing to specify the full path.

If user prefers to manually build and install SMI library and tool from the source code, follow the steps in the following sections.

## Installation commands

After a successful build of AMD SMI library and tool, the following commands will install them on the system:

### AMD SMI library:
- Run `sudo make install` in the smi-lib folder to install the compiled library (`libamdsmi.so`) and the header file (`amdsmi.h`) to the system library folder (`/usr/local/lib/`) and system include folder (`/usr/local/inc/`), respectively.
- Run `sudo make uninstall` in the smi-lib folder to remove the installed library (`libamdsmi.so`) and header file (`amdsmi.h`) from the system library folder (`/usr/local/lib/`) and system include folder (`/usr/local/inc/`), respectively.

### AMD SMI tool:
- Run `sudo make install` in the smi-lib/cli/cpp folder to install the compiled tool (`amd-smi`) to the system bin folder (`/usr/local/bin/`).
- Run `sudo make uninstall` in the smi-lib/cli/cpp folder to uninstall the installed tool (`amd-smi`) from the system bin folder (`/usr/local/bin/`).

## Note on NIC monitoring:

NIC monitoring (built in by default) currently supports the following NICs:

- **AMD Pensando Pollara** - drivers `ionic` (Ethernet) and `ionic_rdma` (RDMA), provided by the AMD AI-NIC host software bundle. See the [AMD AI-NIC Pollara 400 Series Operations Guide](https://docs.amd.com/r/en-US/ug1801-ai-nic-pollara-400-ops-guide/AMD-AI-NIC-Pollara-400-Series-Introduction).
- **Broadcom Thor2 / BCM57608** - drivers `bnxt_en` (Ethernet) and `bnxt_re` (RoCE/RDMA), available in-box in recent Linux kernels or from the Broadcom NetXtreme-E Linux installer package. See the [Broadcom P2200G product page](https://www.broadcom.com/products/ethernet-connectivity/network-adapters/p2200g).

The corresponding NIC drivers must be installed and loaded for `amd-smi` to detect the NICs. RDMA queries (`--rdma-devices`) additionally require the RDMA driver (`ionic_rdma` or `bnxt_re`) to be loaded and RDMA enabled on the device; otherwise RDMA information is not reported.

`amd-smi` provides read-only NIC monitoring (static information, firmware versions, ports, RDMA devices, and statistics). NIC firmware updates and device management are performed with the vendor tools - `nicctl` for AMD Pensando and `niccli` for Broadcom - not with `amd-smi`.