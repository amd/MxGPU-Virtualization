# AMD SMI Monitor Example

This example demonstrates how to display basic GPU information using the AMD System Management Interface (AMDSMI) API.

## Overview

The `monitor` application displays the following GPU information:

- BDF (Bus, Device, Function) identifier
- Driver information
- VBIOS version and details
- Temperature readings (Edge, Hotspot, VRAM)
- Clock frequencies (GFX, MEM, ENC, DEC)
- Framebuffer size and offset
- PCIe information
- Power cap and current power consumption
- Virtual Function (VF) details

## Requirements

- AMDSMI library and headers
- Linux or Windows environment

## Usage

To compile the `monitor` example, simply run `make` inside the `monitor` directory:

```bash
make

Usage examples:

* If `libamdsmi.so` is installed to system path like `/lib`, `/usr/lib` or `/usr/local/lib`, the following command is sufficient: `sudo ./monitor`
* Otherwise the command should contain the path to `libamdsmi.so`: `sudo LD_LIBRARY_PATH=/path/to/libamdsmi/library/ ./monitor`
