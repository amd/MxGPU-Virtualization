<!-- markdownlint-disable MD029 -->

# Host configuration

## Prerequisites

Before starting this guide, ensure you have:

[Getting Started with Virtualization](Getting_started_with_MxGPU.md) - Understanding of MxGPU concepts

---

This section provides step-by-step instructions for configuring SR-IOV to enable device assignment to QEMU/KVM guests. Proper configuration is essential for ensuring that the GPU device can be passed through to VMs effectively.

## System BIOS settings

To enable virtualization extensions, you need to set up System BIOS. For example (depending on BIOS version and vendor), sample System BIOS settings will look like this:

- SR-IOV Support: Enable this option in the Advanced → PCI Subsystem Settings page.

- Above 4G Decoding: Enable this option in the Advanced → PCI Subsystem Settings page.

- PCIe ARI Support: Enable this option in the Advanced → PCI Subsystem Settings page.

- IOMMU: Enable this option in the Advanced → NB Configuration page.

- ACS Enabled: Enable this option in the Advanced → NB Configuration page.

> **Note:** AER must be enabled for ACS enablement to work. PCI AER Support can be enabled in the Advanced → ACPI Settings page.

## GRUB file update

After configuring the BIOS settings, you need to modify the GRUB configuration to apply the necessary changes.

To assign a device to a QEMU/KVM guest, the device needs to be managed by a VFIO (Virtual Function I/O) kernel driver. However, by default, the host device binds to its native driver, which is not a VFIO driver. Therefore, the device must be unbound from its native driver (blacklist amdgpu driver on OS booting up) before it can be passed to the libvirt to assign it to the guest.

Additionally, to enable PCI SRIOV functionality, you need to enable virtualization extensions, IOMMU, etc. All these configurations can be set in /etc/default/grub and then applied to the OS boot grub configuration.

Use the following commands to update the GRUB settings:

1. Edit GRUB Configuration File:

Use a text editor to modify the /etc/default/grub file (Following example uses "nano" text editor). Open the terminal and run the following command:

```bash
# sudo nano /etc/default/grub
```

2. Modify the GRUB_CMDLINE_LINUX Line:

Look for the line that begins with GRUB_CMDLINE_LINUX. It should look like this initially:

```bash
GRUB_CMDLINE_LINUX=""
```

Modify it to include following parameters:

```bash
GRUB_CMDLINE_LINUX="modprobe.blacklist=amdgpu iommu=on amd_iommu=on"
```

If there are already parameters in the quotes, append your new parameters separated by spaces.

3. Save the changes by pressing CTRL + O, press Enter, then exit with CTRL + X.

4. After modifying the configuration file, you need to update the GRUB settings by running the following command:

Ubuntu:

```bash
# sudo update-grub
```

RHEL:

```bash
# sudo grub2-mkconfig -o /boot/grub2/grub.cfg
```

5. Reboot Your System:

For the changes to take effect, reboot your system using the following command:

```bash
# sudo reboot
```

6. Verifying changes:

After the system reboots, confirm that the GRUB parameters were applied successfully by running:

```bash
# cat /proc/cmdline
```

When you run the command above, you should see a line that includes:

```text
modprobe.blacklist=amdgpu iommu=on amd_iommu=on
```

This indicates that your changes have been applied correctly.

> **Note:** In case host machine is running Intel CPU, replace amd_iommu with intel_iommu.

## Install libvirt, KVM, and QEMU packages on the host

Follow the steps below to install Libvirt, KVM, QEMU Packages on Ubuntu Host:

1. Install the required packages

Ubuntu:

```bash
# sudo apt update
# sudo apt install qemu-kvm virtinst libvirt-daemon virt-manager -y
```

RHEL:

```bash
# sudo dnf install @virt Management -y
# sudo systemctl start libvirtd
# sudo systemctl enable libvirtd
```

 2. Configure non-root user to access virt-manager and add user account to the libvirt group:

Ubuntu:

```bash
# sudo groupadd --system libvirt
# sudo usermod -a -G libvirt $(whoami)
# newgrp libvirt
```

RHEL:

```bash
# sudo usermod -a -G libvirt $(whoami)
```

Log out of your current user session. Then, reopen the terminal or log back into your user account. This refreshes your session, ensuring the group changes take effect for access to libvirt resources.

3. Edit libvirtd configuration file:

```bash
# sudo nano /etc/libvirt/libvirtd.conf
```

4. Set the UNIX domain socket group ownership to libvirt, (around line 81):

```txt
unix_sock_group = "libvirt"
```

5. Set the UNIX socket permissions for the R/W socket (around line 104):

```txt
unix_sock_rw_perms = "0770"
```

6. Restart libvirt daemon after making the change.

```bash
# sudo systemctl restart libvirtd.service
```

## Host driver setup

To enable the AMD GPUs to operate in SR-IOV mode, the MxGPU  PF driver is essential. This guide presents two effective approaches for installing the PF driver: via a pre-packaged .deb/.rpm file or by building from source code.

Using the package offers a convenient and streamlined installation process, making it accessible even for users with limited technical experience. This method allows for easy dependency management but may not always include the most up-to-date features available in the source code.

On the other hand, building the PF driver from source provides users with the flexibility to customize their driver installation and access the latest enhancements. This approach, however, requires more technical expertise and manual handling of dependencies, which might present a steeper learning curve for some users.

Both methods will effectively prepare your system for GPU virtualization, and from version 8.3.0.K onwards, both will automatically install AMD SMI, providing robust management and monitoring capabilities for your GPUs. The choice between the two approaches depends on your specific needs and familiarity with the installation process. The following sections will detail each approach to guide you through the setup.

### Install the PF driver from a package

On Ubuntu systems, PF driver can be installed using the [latest .deb package](https://github.com/amd/MxGPU-Virtualization/releases).

On RHEL systems, PF driver can be installed using the [latest .rpm package](https://github.com/amd/MxGPU-Virtualization/releases).

Here are the steps:

1. Install dependencies:

Ubuntu:

  ```bash
 # sudo apt update
 # sudo apt install build-essential dkms autoconf automake
  ```

RHEL:

```bash
# sudo dnf install @development-tools kernel-devel dkms autoconf automake
```

2. To install the package, use the following command:

  Ubuntu:

  ```bash
  # sudo dpkg -i gim_driver_package.deb
  ```

  RHEL:

  ```bash
  # sudo rpm -ivh gim_driver_package.rpm
  ```

3. Once the PF driver is installed, reboot the system and load the driver after reboot by using the following command:

```bash
# sudo modprobe gim
```

The PF driver is loaded with one VF by default, which means that if we use a platform with eight GPUs, each of the GPUs can support one VF. This results in a total of eight VFs.

4. To ensure that the PF driver has loaded correctly, check if the GPU VF devices are visible by running:

MI210X:

```bash
# lspci -d 1002:7410
```

MI300X:

```bash
# lspci -d 1002:74b5
```

MI325X:

```bash
# lspci -d 1002:74b9
```

MI350X:

```bash
# lspci -d 1002:75b0
```

Radeon Pro V710:

```bash
# lspci -d 1002:7461
```

When the PF driver is loaded the command will output an entry for each VF.

5. Way to check if driver loaded successfully:

```bash
# sudo dmesg | grep GIM.*Running
```

### Build the PF driver from source

Alternatively, you can build the PF driver from the source code if you prefer to have the latest features or customize the installation. Here are the steps:

1. Install dependencies:

Ubuntu:

```bash
# sudo apt update
# sudo apt install build-essential autoconf
```

RHEL:

```bash
# sudo dnf groupinstall "Development Tools"
# sudo dnf install kernel-devel autoconf
```

2. Obtain the Source Code: Clone or download the [source code](https://github.com/amd/MxGPU-Virtualization/releases) for the PF driver.

3. Navigate to the Source Directory: Change to the directory containing the PF driver source code.

4. Build the PF Driver:

To build and install the driver from the source, run the following commands:

```bash
# make clean
# make all -j
# sudo make install
```

5. Insert the Driver:
Once the PF driver is installed, load it using the following command

```bash
# sudo modprobe gim
```

> **Note:** If the vf_num parameter is omitted, the driver loads with 1 VF by default. To see supported VF counts for a given GPU, refer to the [AMD MxGPU Virtualization GitHub releases page](https://github.com/amd/MxGPU-Virtualization/releases).

To verify that the PF driver is loaded properly, use the same verification commands mentioned in the previous section.

> **Note:**  Newer Ubuntu kernel versions require gcc and g++ version 12 installed. If needed install gcc-12, g++-12 and update the alternatives system to point to the correct version:

```bash
# sudo apt install gcc-12 g++-12
# sudo update-alternatives --config gcc
# sudo update-alternatives --config g++
```

---

## Next steps

After completing host configuration, you have two paths:

### Simple setup

**[Virtual Machine Setup](VM_setup.md)** - Configure your VMs with default VF settings

### Advanced setup (custom configurations)

Choose one or both advanced guides before VM setup:

- **[GPU Partitioning](GPU_partitioning.md)** - Customize partitioning per GPU
- **[XGMI Configuration](XGMI_configuration.md)** - Different configurations for assigning VFs to VMs
