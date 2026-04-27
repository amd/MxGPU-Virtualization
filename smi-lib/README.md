# AMD SMI LIB

## What is AMD SMI LIB
**AMD SMI LIB** is a library that enables you to manage and monitor AMD Virtualization Enabled GPUs. It is a thread safe, extensible C based library. The library exposes both C and Python API interface. Some of the features that are exposed in the library are:
 * Query static information about the GPU (ASIC, framebuffer etc.)
 * Query information about the FW on the physical function
 * Query information about the virtual functions on the GPU
 * Query GPU metrics (temperature, clocks, usage etc.)
 * Set GPU configuration (partitioning modes, FB sharing modes, power cap etc.)
 * GPU reset
 * Clear VF FB (on MI products)

To run the AMD SMI library, the Linux PF driver needs to be installed.

## DOCUMENTATION:
Please check out our [User Guide](https://instinct.docs.amd.com/projects/amd-smi-virt/en/latest/) for instructions on how to set up and use AMD SMI LIB.
