<!-- markdownlint-disable MD029 -->

# Getting started with virtualization

AMD's virtualization solution, **MxGPU**, specifically leverages SR-IOV (Single Root I/O Virtualization) to enable sharing of GPU resources with multiple virtual machines (VMs). This technology allows VMs direct access to GPU resources, significantly improving workload performance while maintaining high levels of resource efficiency.

AMD's MxGPU approach unlocks additional capabilities for a wide range of applications, from high-performance computing (HPC) and artificial intelligence (AI) to machine learning (ML) and graphics-intensive tasks. The SR-IOV architecture, facilitated by MxGPU, supports fine-grained resource allocation and isolation, enabling efficient sharing of GPU resources among multiple workloads. This not only enhances performance for compute-heavy applications but also allows for optimal scalability in multi-tenant environments.

In this guide, we will explore how to implement AMD's MxGPU technology in QEMU/KVM environments. We will cover the architecture, configuration steps, and best practices for leveraging these advanced virtualization solutions to achieve superior performance and efficiency in your workloads.

## Understanding SR-IOV

To expand the capabilities of AMD GPUs, this guide focuses on enabling SR-IOV, a standard developed by the PCI-SIG (PCI Special Interest Group) that facilitates efficient GPU virtualization. AMD’s MxGPU technology utilizes SR-IOV to allow a single GPU to appear as separate devices on the PCIe bus, presenting virtual functions (VFs) to the operating system and applications. This implementation enables direct access to GPU resources without the need for software emulation, thereby enhancing performance.

The term "Single Root" indicates that SR-IOV operates within a single PCI Express root complex, connecting all PCI devices in a tree-like structure. A key goal of SR-IOV is to streamline data movement by minimizing the hypervisor's involvement, providing each VM with independent copies of memory space, interrupts, and Direct Memory Access (DMA) streams. This direct communication with hardware allows VMs to achieve near-native performance.

The SR-IOV standard is maintained by the PCI-SIG foundation, ensuring its relevance and effectiveness through cross-industry collaboration and funding in the evolving landscape of virtualization technology.

## KVM and QEMU

KVM (Kernel-based Virtual Machine) and QEMU (Quick Emulator) are integral components of the virtualization stack that will be used in conjunction with the MxGPU. KVM transforms the Linux kernel into a hypervisor, enabling the creation and management of VMs, while QEMU provides the necessary user-space tools for device emulation and management. Together, they facilitate the effective use of SR-IOV, allowing multiple VMs to share AMD GPUs efficiently, enhancing resource utilization and performance.

## Supported GPU models

Instinct:

- MI210
- MI300X
- MI325X
- MI350X/MI355X
- MI350P

Radeon:

- PRO V710

Listed GPUs officially support the MxGPU technology, enabling enhanced GPU virtualization capabilities.

### AMD Instinct GPU models

#### AMD Instinct MI210

The [AMD Instinct MI210](https://www.amd.com/en/products/accelerators/instinct/mi200/mi210.html) is an AMD CDNA 2 GPU in a dual-slot PCIe card form factor, part of the Instinct MI200 Series. It has 104 compute units and 64 GB of HBM2e memory with 1.6 TB/s peak bandwidth. It connects to the host over PCIe Gen 4 x16 and to other GPUs in the same server over AMD Infinity Fabric (XGMI) links.

#### AMD Instinct MI300X

The [AMD Instinct MI300X](https://www.amd.com/en/products/accelerators/instinct/mi300/mi300x.html) is an AMD CDNA 3 GPU in the OAM form factor, part of the Instinct MI300 Series. It has 304 compute units and 192 GB of HBM3 memory with 5.3 TB/s peak bandwidth. It connects to the host over PCIe Gen 5 x16 and to other GPUs in the same server over AMD Infinity Fabric (XGMI) links.

#### AMD Instinct MI325X

The [AMD Instinct MI325X](https://www.amd.com/en/products/accelerators/instinct/mi300/mi325x.html) is an AMD CDNA 3 GPU in the OAM form factor, part of the Instinct MI300 Series. It has 304 compute units and 256 GB of HBM3E memory with 6 TB/s peak bandwidth. It connects to the host over PCIe Gen 5 x16 and to other GPUs in the same server over AMD Infinity Fabric (XGMI) links.

#### AMD Instinct MI350X/MI355X

The [AMD Instinct MI350X](https://www.amd.com/en/products/accelerators/instinct/mi350/mi350x.html) and [AMD Instinct MI355X](https://www.amd.com/en/products/accelerators/instinct/mi350/mi355x.html) are AMD CDNA 4 GPUs in the OAM form factor, part of the Instinct MI350 Series. Both have 256 compute units and 288 GB of HBM3E memory with 8 TB/s peak bandwidth. Both connect to the host over PCIe Gen 5 x16 and to other GPUs in the same server over AMD Infinity Fabric (XGMI) links. The MI355X has a higher peak engine clock than the MI350X (2.4 GHz versus 2.2 GHz) and a higher typical board power (1400 W versus 1000 W).

#### AMD Instinct MI350P

The [AMD Instinct MI350P](https://www.amd.com/en/products/accelerators/instinct/mi350/mi350p.html) is an AMD CDNA 4 GPU in a dual-slot PCIe card form factor, part of the Instinct MI350 Series. It has 128 compute units and 144 GB of HBM3E memory with 4 TB/s peak bandwidth. It connects to the host over PCIe Gen 5 x16 and has no Infinity Fabric (XGMI) connection to other GPUs. Up to eight cards can be installed in one server, and board power can be lowered from 600 W to 450 W to fit existing power and cooling limits.

### AMD Radeon GPU models

#### AMD Radeon PRO V710

The [AMD Radeon PRO V710](https://www.amd.com/en/products/accelerators/radeon-pro/amd-radeon-pro-v710.html) is an AMD RDNA 3 GPU in a single-slot PCIe card form factor, part of the Radeon PRO V Series. It has 54 compute units and 28 GB of GDDR6 memory with 448 GB/s peak bandwidth. It connects to the host over PCIe Gen 4 x16 and has no Infinity Fabric (XGMI) connection to other GPUs. It also provides hardware ray tracing and 8K AV1 video encoding for graphics and media workloads.

## MxGPU software stack

### Recommended Linux distributions

For recommendations on Host and Guest OS distributions, configuration details, and the latest updates, please visit the [AMD MxGPU Virtualization GitHub releases page](https://github.com/amd/MxGPU-Virtualization/releases). The following documentation provides detailed setup instructions for Ubuntu 22.04 and RHEL 9.4 Linux distributions. While additional distributions may function effectively, this document uses mentioned installations as example host and guest installations.

### AMD software stack components

To set up the MxGPU solution and ensure seamless operation, the following software stack components are needed:

- PF Driver - AMD host driver for virtualized environments, which enables the powerful integration of MxGPU technologies for effective GPU resource management.

- AMD SMI - The AMD SMI (System Management Interface) library and tool provide robust management and monitoring capabilities for AMD Virtualization Enabled GPUs, bundled with the PF driver package. Designed specifically for SR-IOV host environment setups, this version of AMD SMI functions as a cross-platform utility compatible with various operating systems. This comprehensive library, known for its thread safety and extensibility, offers both C and Python API interfaces. Through these interfaces, users can query static GPU information such as ASIC and frame buffer details, as well as retrieve vital data regarding firmware, virtual functions, temperature, clocks, and GPU usage. The library facilitates application development in both C/C++ and Python, offering function declarations respective to each language for seamless integration. Complementing the library, the AMD SMI tool is a command line utility that leverages the library APIs to effectively monitor GPU status across different types of environments. It provides versatile output options, displaying or saving GPU status in plain text, JSON, or CSV formats. Notably, this SR-IOV-specific version of AMD SMI differs from the ROCm-specific tool that is intended for use in non-virtualized setups. For a detailed description of its capabilities and access to the complete documentation, please visit [this page](https://instinct.docs.amd.com/projects/amd-smi-virt/en/latest/).

> **Note:**  Starting from version 8.3.0.K of the PF Driver, the installation of AMD SMI is now integrated with the driver installation process. This means that AMD SMI is automatically installed when the driver is installed, simplifying the setup process.

- VF Driver - The ROCm GPU driver is one of the components of the latest [ROCm stack](https://rocm.docs.amd.com/en/latest/install/rocm.html) that can effectively be used in guest virtual machines with proper configuration. As a guest driver, it allows virtual machines to efficiently access GPU resources.

---

## Next steps

After understanding MxGPU fundamentals, proceed to:

[Host Configuration](Host_configuration.md) - Set up your host system for MxGPU
