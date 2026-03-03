---
myst:
  html_meta:
    "description lang=en": "AMD SMI documentation and API reference."
    "keywords": "amdsmi, lib, cli, system, management, interface, admin, sys, virtualization, virt"
---

# AMD SMI for virtualization

AMD SMI is a library that enables you to manage and monitor AMD virtualization-enabled GPUs. It is a thread safe, extensible C-based library. The library exposes both C and Python API interfaces.

```{important}
This is the AMD SMI for SR-IOV Linux host only. If you are looking for Linux baremetal or SR-IOV Linux guest AMD SMI, please go to the [AMD SMI documentation](https://rocm.docs.amd.com/projects/amdsmi/en/latest/index.html).
```

Some of the features that are exposed in the library are:

- Query static information about the GPU (ASIC, framebuffer etc.)
- Query information about the FW on the physical function
- Query information about the virtual functions on the GPU
- Query GPU metrics (temperature, clocks, usage etc.)
- Set GPU configuration (partitioning modes, FB sharing modes, power cap etc.)
- GPU reset and clear VF FB (on MI products)

- To run the AMD SMI library, the Linux PF driver needs to be installed.

There are two parts to the AMD SMI library interface:

- **C interface**
  - Consists of C function declarations.
  - The client can call these to query information and configure settings on the Linux host machine.
  - The C interface can be used by the clients to build applications in C/C++ by using this interface and library binary.

- **Python interface**
  - Consists of Python function declarations.
  - These directly call the C interface.
  - The client can use the Python interface to build applications in Python.

AMD SMI tool is a command line utility that utilizes AMD SMI Library APIs to monitor and configure AMD GPUs. The tool is used to monitor AMD’s GPUs status in a virtualization environment in both host OS and guest VM, as well as in a bare metal environment for both Windows and Linux Operating Systems. The tool outputs GPU/driver information in plain text, in JSON, or in CSV formats while it can also show the info in the console or save to the specified output file.

The tool can be used to:

- Query static information about the GPU (ASIC, framebuffer etc.)
- Query information about the FW on the physical function
- Query information about the virtual functions on the GPU
- Query GPU metrics (temperature, clocks, usage etc.)
- Set GPU configuration (partitioning modes, FB sharing modes, power cap etc.)
- Reset GPU and clear VF FB (on MI products)

For additional information on build, installation, usage, versioning and API references, please refer to the sections below:

::::{grid} 2
:gutter: 4

:::{grid-item-card} Install

- [Install the library and CLI tool](./install/install.md)
- [Build from source](./install/build.md)
:::

:::{grid-item-card} How to

- [Use the C/C++ library](./how_to/amdsmi_c_lib.md)
- [Use the Python library](./how_to/amdsmi_py_lib.md)
- [Use the CLI tool](./how_to/amdsmi_cli_usage.md)
:::

:::{grid-item-card} Reference

- [C/C++ API](./reference/amdsmi_c_api.md)
  - [Files](../doxygen/doxy_build/html/files)
  - [Globals](../doxygen/doxy_build/html/globals)
  - [Data structures](../doxygen/doxy_build/html/annotated)
  - [Data fields](../doxygen/doxy_build/html/functions_data_fields)
- [Python API](./reference/amdsmi_py_api.md)
:::

:::{grid-item-card} About

- [Versioning](./about/versioning.md)
- [License](./about/license.md)
:::

::::
