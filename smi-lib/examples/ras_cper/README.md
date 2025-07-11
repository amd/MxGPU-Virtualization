# RAS CPER Example

This example demonstrates how to retrieve RAS (Reliability, Availability, and Serviceability) errors for a specified device. To collect logs, you must first enable RAS on the server and then inject errors for testing.

## Dependencies

- `libamdsmi.so` - AMD SMI library

## Prerequisites

- The OS dynamic library search path must include the location of `libamdsmi.so`
- Root privileges required for RAS operations

## Usage

```bash
ras_cper
```

## Examples

### System Installation
If `libamdsmi.so` is installed to a system path (e.g., `/lib` or `/usr/lib`):

```bash
sudo ./ras_cper
```

### Custom Library Path
If the library is in a custom location:

```bash
sudo LD_LIBRARY_PATH=/path/to/libamdsmi/library/ ./ras_cper
```

## Building

Ensure you have the AMD SMI library installed and properly linked during compilation.

## License

This example is provided under the same license as the parent project.