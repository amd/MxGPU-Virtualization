# AMD SMI NIC Example

This example prints what AMD SMI reports for each NIC it discovers:

- Driver, ASIC, firmware, PCIe bus, and NUMA information
- Link type from the NIC to every GPU and to every other NIC
- Port settings, with standard and vendor-specific port statistics
- RDMA devices and ports, with RDMA port statistics

## Requirements

- `libamdsmi.so` built with NIC support, which is the default on Linux
- A supported NIC with its driver loaded: AMD Pensando Pollara (`ionic`) or
  Broadcom Thor2 (`bnxt_en`)

Some information depends on optional components. When one is missing, the
example prints `N/A` for the affected section or field and continues.

| Component | Provides |
|---|---|
| `ionic_rdma` or `bnxt_re` driver | RDMA devices and RDMA port statistics |
| `libmnl`, installed when `libamdsmi.so` is built | Firmware versions and port flavour |
| `libibverbs.so.1` at run time | RDMA port maximum and active MTU |

## Usage

Build the example with `make` in this directory, or with `make examples` from
the `smi-lib` directory:

```bash
make
```

Run it as root:

* If `libamdsmi.so` is installed to a system path such as `/usr/local/lib`: `sudo ./amdsmi_nic`
* Otherwise, point the loader to it: `sudo LD_LIBRARY_PATH=/path/to/libamdsmi/library/ ./amdsmi_nic`
