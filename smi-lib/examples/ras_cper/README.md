This example shows all ras errors for a single specified device. In order to get logs, you must first enable ras on the server and then inject errors.

Dependency: `libamdsmi.so`

Prerequisite: OS dynamic library search path has to contain the path where `libamdsmi.so` is located.

Usage: `ras_cper`

Usage examples:

* If `libamdsmi.so` is installed to system path like `/lib` or `/usr/lib`, the following command is sufficient: `sudo ./ras_cper`
* Otherwise the command should contain the path to `libamdsmi.so`: `sudo LD_LIBRARY_PATH=/path/to/libamdsmi/library/ ./ras_cper `