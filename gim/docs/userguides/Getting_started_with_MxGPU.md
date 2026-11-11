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

- MI210X
- MI300X
- MI325X
- MI350X/MI355X

Radeon:

- PRO V710

Listed GPUs officially support the MxGPU technology, enabling enhanced GPU virtualization capabilities. Additionally, AMD plans to announce support for more architectures in the future, further expanding the versatility and application of its GPU solutions.

### AMD Instinct GPU models

#### AMD Instinct MI210X architecture

[The AMD Instinct MI210X series accelerators](https://www.amd.com/en/products/accelerators/instinct/mi200/mi210.html), built on the 2nd Gen AMD CDNA architecture, excel in high-performance computing (HPC), artificial intelligence (AI), and machine learning (ML) tasks, particularly in double-precision (FP64) computations. These accelerators leverage AMD Infinity Fabric technology to provide high-bandwidth data transfer, supporting PCIe Gen4 for efficient connectivity across multiple GPUs. Equipped with 64GB of HBM2e memory at 1.6 GHz, the MI210X ensures effective handling of large data sets. AMD's Matrix Core technology enhances mixed-precision capabilities, making the MI210X ideal for deep learning and versatile AI applications.

#### AMD Instinct MI300X architecture

[The AMD Instinct MI300X series accelerators](https://www.amd.com/en/products/accelerators/instinct/mi300/mi300x.html), based on the advanced AMD CDNA 3 architecture, offer substantial improvements in AI and HPC workloads. With 304 high-throughput compute units and cutting-edge AI-specific functions, the MI300X integrates 192 GB of HBM3 memory and utilizes die stacking for enhanced efficiency. Delivering significantly higher performance, it features 13.7x peak AI/ML workload improvement using FP8 and a 3.4x advantage for HPC with FP32 calculations compared to previous models. AMD's 4th Gen Infinity Architecture provides superior I/O efficiency and scalability, with PCIe Gen 5 interfaces and robust multi-GPU configurations. The MI300X also incorporates SR-IOV capabilities for effective GPU partitioning, providing coherent shared memory and caches to support data-intensive machine-learning models across GPUs, with 5.3 TB/s bandwidth and 128 GB/s inter-GPU connectivity.

#### AMD Instinct MI325X architecture

[The AMD Instinct MI325X series accelerators](https://www.amd.com/en/products/accelerators/instinct/mi300/mi325x.html), built on the cutting-edge 3rd Gen AMD CDNA architecture, are designed to meet the demands of modern AI and HPC workloads. With a robust configuration of 304 compute units, the MI325X excels in processing a wide array of data types, making it ideal for both high-precision inference and training tasks. The accelerator is equipped with an industry-leading 256 GB of HBM3E memory, delivering an impressive 6 TB/s bandwidth, which allows it to efficiently handle one-trillion parameter models and reduce total cost of ownership for large-language models. AMD Infinity Fabric technology ensures excellent I/O efficiency and scalability, with a 16-lane PCIe Gen 5 host interface and seven Infinity Fabric links for seamless connectivity between GPUs. Integrated with AMD's comprehensive software ecosystem, the MI325X supports key AI and HPC frameworks, simplifying deployment and accelerating development across a wide range of applications.

#### AMD Instinct MI350X/MI355X architecture

Leveraging the latest CDNA 4 architecture, the AMD Instinct [MI350X](https://www.amd.com/en/products/accelerators/instinct/mi350/mi350x.html) and [MI355X](https://www.amd.com/en/products/accelerators/instinct/mi350/mi355x.html) series accelerators are engineered for the most demanding AI and HPC workloads. Featuring up to 256 compute units, these GPUs harness 288 GB of HBM3E memory to manage large data sets efficiently, achieving up to 8 TB/s of memory bandwidth for complex, data-driven applications. Advanced AMD Infinity Fabric interconnects enhance multi-GPU scalability and performance, supporting ultra-dense configurations for large-scale AI training.

The AMD Instinct MI355X GPU is purpose built for high density computing environments, offering more peak performance than its counterpart the MI350X. This increased power enables the MI355X to sustain higher performance over time, minimizing throttling and maximizing throughput during prolonged or intensive workloads.

Designed for high-performance, energy-efficient operation, these accelerators maintain robust computational output across a wide range of workloads. With comprehensive support for various precision modes and data types, the MI350X and MI355X deliver flexible and efficient computation for scientific, AI, and HPC tasks.

### AMD Radeon GPU models

#### AMD Radeon PRO V710

[The AMD Radeon PRO V710](https://www.amd.com/en/products/accelerators/radeon-pro/amd-radeon-pro-v710.html), part of the Radeon PRO V Series, is designed for high-performance server environments using the AMD RDNA™ 3 architecture. It supports 8K AV1 video encoding and delivers 55 TFLOPS peak FP16 inference performance, with support for BF16, INT8, and INT4 data types. The GPU features accelerated Raytracing 2.0 with Variable Rate Shading (VRS) and includes 28 GB of GDDR6 memory with 54 MB of AMD Infinity Cache, achieving 448 GB/s peak memory bandwidth. With 54 compute units and 3456 stream processors, it operates at a peak engine clock of 2 GHz and a total board power of 158W. The V710 supports major operating systems and APIs, making it versatile for modern server applications.

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
