# GIM

## What is GIM?
[GIM](https://github.com/amd/MxGPU-Virtualization#) (GPU-IOV Module) is a Linux kernel module for AMD SR-IOV based HW Virtualization (MxGPU) product. It supports KVM based hypervisors with necessary kernel compatibility layer. GIM is reponsible for:
 * GPU IOV initialization
 * Virtual function configuration and enablement
 * GPU scheduling for world switch
 * Hang detection and virtual function level reset (FLR)
 * PF/VF hand shake and other GPU utilities.

## DOCUMENTATION:
Please check out our [User Guide](https://instinct.docs.amd.com/projects/virt-drv/en/latest/) for instructions on how to set up GIM and example configurations to run SR-IOV enabled VMs.

## Hardware/Features supported:

Please check the latest [release note](https://github.com/amd/MxGPU-Virtualization/releases).
