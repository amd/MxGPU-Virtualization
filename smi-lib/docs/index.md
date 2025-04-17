---
myst:
  html_meta:
    "description lang=en": "AMD SMI documentation and API reference."
    "keywords": "amdsmi, lib, cli, system, management, interface, amdgpu, admin, sys"
---

# AMD SMI documentation

AMD SMI LIB is a library that enables you to manage and monitor AMD Virtualization Enabled GPUs. It is a thread safe, extensible C based library. The library exposes both C and Python API interface.

Some of the features that are exposed in the library are:

- Query static information about the GPU (ASIC, framebuffer, etc.)
- Query information about the FW on the physical function
- Query information about the virtual functions on the GPU
- Query temperature, clocks, and usage of the GPU

- To run the AMD SMI library, the Linux Host Virtualization driver needs to be installed.

There are two parts to the AMD SMI library interface:

* **C interface**
  - Consists of C function declarations.
  - The client can call these to query information about the Linux host machine.
  - The C interface can be used by the clients to build applications in C/C++ by using this interface and library binary.

* **Python interface**
  - Consists of Python function declarations.
  - These directly call the C interface.
  - The client can use the Python interface to build applications in Python.

AMD SMI tool is a command line utility that utilizes AMD SMI Library APIs to monitor AMD GPUs. The tool is used to monitor AMD’s GPUs status in a virtualization environment in both host OS and guest VM, as well as in a bare metal environment for both Windows and Linux Operating Systems. The tool outputs GPU/driver information in plain text, in JSON, or in CSV formats while it can also show the info in the console or save to the specified output file.

The tool can be used to:

- Query static information about the GPU (ASIC, framebuffer, etc.)
- Query information about the FW on the physical function
- Query information about the virtual functions on the GPU
- Query temperature, clocks, and usage of the GPU

For additional information on installation, usage, and API references, please refer to the sections below:
::::{grid} 2
:gutter: 3

:::{grid-item-card} Install
* [Build from source](install/build.md)
:::

:::{grid-item-card} How to
* [C library usage](how_to/amdsmi_c_lib.md)
* [Python library usage](how_to/amdsmi_py_lib.md)
* [CLI tool usage](how_to/amdsmi_cli_tool.md)
:::

:::{grid-item-card} Reference
* <a href="doxygen/html/index.html">C API</a>
* [Python API](reference/amdsmi_py_api.md)
:::

:::{grid-item-card} About
* [LICENSE](license.md)
:::


::::

<style>
#disclaimer {
    font-size: 0.8rem;
}
</style>

## DISCLAIMER
<div id="disclaimer">
The information contained herein is for informational purposes only, and is
subject to change without notice. While every precaution has been taken in the
preparation of this document, it may contain technical inaccuracies, omissions
and typographical errors, and AMD is under no obligation to update or otherwise
correct this information. Advanced Micro Devices, Inc. makes no representations
or warranties with respect to the accuracy or completeness of the contents of
this document, and assumes no liability of any kind, including the implied
warranties of noninfringement, merchantability or fitness for particular
purposes, with respect to the operation or use of AMD hardware, software or
other products described herein.

AMD, the AMD Arrow logo, and combinations thereof are trademarks of Advanced
Micro Devices, Inc. Other product names used in this publication are for
identification purposes only and may be trademarks of their respective
companies.

Copyright &copy; 2025 Advanced Micro Devices, Inc. All rights reserved.
</div>