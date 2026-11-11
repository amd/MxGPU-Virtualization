---
myst:
  html_meta:
    "description": "Learn about AMD MxGPU virtualization technology with SR-IOV support"
    "keywords": "AMD, Instinct, MxGPU, GPU virtualization, SR-IOV, MI210X, MI300X, MI325X, MI350X, MI355X, Radeon Pro V710"
---

# AMD Instinct virtualization driver

AMD MxGPU technology enables GPU virtualization through SR-IOV (Single Root I/O Virtualization), which lets virtual machines share AMD accelerator resources efficiently. This documentation covers configuration, deployment, and optimization for QEMU/KVM environments.

## Supported hardware

- AMD Instinct MI210X
- AMD Instinct MI300X
- AMD Instinct MI325X
- AMD Instinct MI350X/MI355X
- AMD Radeon PRO V710

## Set up MxGPU

Follow these guides in order. Each guide lists its prerequisites and links to the next step.

1. **[Getting started with virtualization](userguides/Getting_started_with_MxGPU.md)** - Learn how SR-IOV, KVM, and QEMU work together, and review the supported GPUs and software stack.
2. **[Host configuration](userguides/Host_configuration.md)** - Configure the system BIOS and GRUB, install libvirt, and install the PF driver.
3. **[GPU partitioning](userguides/GPU_partitioning.md)** - Choose a compute and memory partitioning mode for each GPU.
4. **[XGMI configuration](userguides/XGMI_configuration.md)** - Set frame buffer sharing modes before you assign VFs to VMs.
5. **[VM setup](userguides/VM_setup.md)** - Create a guest VM, attach a VF, and install the guest driver.

Steps 3 and 4 apply to multi-partition and multi-GPU deployments. If you are assigning a single VF to a single VM, you can go from step 2 to step 5.

## Reference

::::{grid} 2
:gutter: 3

:::{grid-item-card} Configure the host

- [Host configuration](userguides/Host_configuration.md)
- [GPU partitioning](userguides/GPU_partitioning.md)
- [XGMI configuration](userguides/XGMI_configuration.md)
:::

:::{grid-item-card} Configure the guest

- [VM setup](userguides/VM_setup.md)
:::

:::{grid-item-card} Maintenance

- [Removing MxGPU](userguides/Removing_MxGPU.md)
:::

:::{grid-item-card} Related documentation

- [AMD SMI for SR-IOV hosts](https://instinct.docs.amd.com/projects/amd-smi-virt/en/latest/)
- [ROCm installation guide](https://rocm.docs.amd.com/en/latest/install/rocm.html)
:::

::::
