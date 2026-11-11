<!-- markdownlint-disable MD029 -->

# VM setup

## Prerequisites

Before starting this guide, you must complete:

1. [Getting Started with Virtualization](Getting_started_with_MxGPU.md) - Understanding of MxGPU concepts
2. [Host Configuration](Host_configuration.md) - Host system properly configured

**Advanced** (for custom configurations):

3. [GPU Partitioning](GPU_partitioning.md)
4. [XGMI Configuration](XGMI_configuration.md)

---

## Guest VM initial setup

The initial VM setup can be performed using QEMU/libvirt command line utilities. Creation of each guest OS VM is similar, and steps are mostly common for each of them. This chapter will reference Ubuntu22.04 setup as an example.

**1. Install dependencies:**

```bash
# sudo apt update
# sudo apt install cloud-utils

 ```

**2. Download  Ubuntu base Image:**

```bash
# sudo wget https://cloud-images.ubuntu.com/releases/22.04/release/ubuntu-22.04-server-cloudimg-amd64.img
```

**3. Set password for new VM:**

```bash
# cat >user-data1.txt <<EOF
# > #cloud-config
# > password: user1234
# > chpasswd: { expire: False }
# > ssh_pwauth: True
# > EOF
# sudo cloud-localds user-data1.img user-data1.txt
```

**4. Create a disk for new VM:**

```bash
# sudo qemu-img create -b ubuntu-22.04-server-cloudimg-amd64.img -F qcow2 -f qcow2 ubuntu22.04-vm1-disk.qcow2 100G
```

**5. Install new VM and login to check the IP:**

```bash
# sudo virt-install --name ubuntu22.04-vm1 --virt-type kvm --memory 102400 --vcpus 20 --boot hd,menu=on --disk path=ubuntu22.04-vm1-disk.qcow2,device=disk --disk path=user-data1.img,format=raw --graphics none --os-variant ubuntu22.04

# Login: ubuntu
# Password: user1234
# ip addr
# sudo passwd root (set root password as `user1234`)
# sudo usermod -aG sudo ubuntu
# sudo vi /etc/default/grub
#       GRUB_CMDLINE_LINUX="modprobe.blacklist=amdgpu"
# sudo update-grub
# sync
# sudo shutdown now
# sudo virsh start ubuntu22.04-vm1
# sudo virsh domifaddr ubuntu22.04-vm1
# ssh ubuntu@192.168.x.x (password: user1234) - verify access
# exit
```

**6. GPU VF device nodes can be added to VM XML configuration using sudo virsh edit <VM_NAME> command and by modifying devices section:**

```bash
# sudo virsh list --all
# sudo virsh shutdown ubuntu22.04-vm1
# sudo virsh edit ubuntu22.04-vm1 (add hostdev entry under devices section)

<hostdev mode='subsystem' type='pci' managed='yes'>
    <source>
        <address domain='0x0000' bus='0x<DEVICE_BUS_ID>' slot='0x<DEVICE_SLOT>' function='0x0'/>
    </source>
</hostdev>
```

Repeat this step for every virtual GPU that is being added to the VM (one node per virtual device). DEVICE_BUS_ID and DEVICE_SLOT for each of targeted device can be obtained from output of lspci -d 1002:74b5 command which prints out devices VF BDF address in format DEVICE_BUS_ID:DEVICE_SLOT.function.

As an example, this is how all eight GPU VF device nodes can be added to the VM config. If this is the output of the command:

```bash
# lspci -d 1002:74b5
03:02.0 Display controller: Advanced Micro Devices, Inc. [AMD/ATI] Device 74b5 (rev 02)
26:02.0 Display controller: Advanced Micro Devices, Inc. [AMD/ATI] Device 74b5 (rev 02)
43:02.0 Display controller: Advanced Micro Devices, Inc. [AMD/ATI] Device 74b5 (rev 02)
63:02.0 Display controller: Advanced Micro Devices, Inc. [AMD/ATI] Device 74b5 (rev 02)
83:02.0 Display controller: Advanced Micro Devices, Inc. [AMD/ATI] Device 74b5 (rev 02)
a3:02.0 Display controller: Advanced Micro Devices, Inc. [AMD/ATI] Device 74b5 (rev 02)
c3:02.0 Display controller: Advanced Micro Devices, Inc. [AMD/ATI] Device 74b5 (rev 02)
e3:02.0 Display controller: Advanced Micro Devices, Inc. [AMD/ATI] Device 74b5 (rev 02)
```

VF BDF address is shown at the beginning of every line in the mentioned format: DEVICE_BUS_ID:DEVICE_SLOT.function

Based on that data, GPU VFs device nodes should be added to the VM XML configuration under <devices> section: (in this example all 8 GPUs are being assinged to a single VM)

```bash
<hostdev mode='subsystem' type='pci' managed='yes'>
      <source>
          <address domain='0x0000' bus='0x03' slot='0x02' function='0x0'/>
      </source>
</hostdev>
<hostdev mode='subsystem' type='pci' managed='yes'>
      <source>
          <address domain='0x0000' bus='0x26' slot='0x02' function='0x0'/>
      </source>
</hostdev>
<hostdev mode='subsystem' type='pci' managed='yes'>
      <source>
          <address domain='0x0000' bus='0x43' slot='0x02' function='0x0'/>
      </source>
</hostdev>
<hostdev mode='subsystem' type='pci' managed='yes'>
      <source>
          <address domain='0x0000' bus='0x63' slot='0x02' function='0x0'/>
      </source>
</hostdev>
<hostdev mode='subsystem' type='pci' managed='yes'>
      <source>
          <address domain='0x0000' bus='0x83' slot='0x02' function='0x0'/>
      </source>
</hostdev>
<hostdev mode='subsystem' type='pci' managed='yes'>
      <source>
          <address domain='0x0000' bus='0xa3' slot='0x02' function='0x0'/>
      </source>
</hostdev>
<hostdev mode='subsystem' type='pci' managed='yes'>
      <source>
          <address domain='0x0000' bus='0xc3' slot='0x02' function='0x0'/>
                </source>
</hostdev>
<hostdev mode='subsystem' type='pci' managed='yes'>
      <source>
          <address domain='0x0000' bus='0xe3' slot='0x02' function='0x0'/>
      </source>
</hostdev>

```

**7. Configuring Prefetchable Memory for Large VRAM (MI300 and Similar GPUs):**

Some AMD Instinct devices expose large prefetchable BARs (such as the VRAM BAR). By default, QEMU does not reserve a sufficiently large prefetchable 64-bit PCIe memory region for these devices. As a result, the guest VM may fail to map the entire VRAM of the assigned VF(s).

To ensure correct operation, you must explicitly reserve a larger 64-bit prefetchable address space by setting the **pcie-root-port.pref64-reserve** QEMU parameter.

Use lspci on the host to inspect Memory Region 0 of the VF(s):

```bash
# lspci -d 1002: -vv
```

Look for the prefetchable Region 0 size. For example, on MI300 devices this region is typically 256 GB.

Reserve at least one power of two greater than the reported size.

Example: **Region 0 = 256 GB → next power of two = 512 GB**

First, add the QEMU Namespace to the VM XML by editing the VM definition:

```bash
# sudo virsh edit <VM_NAME>
```

Modify the <domain> opening tag to include the QEMU namespace:

```bash
<domain type='kvm' xmlns:qemu='http://libvirt.org/schemas/domain/qemu/1.0'>
```

Then, add the pref64-reserve setting. Insert the following block immediately after the </devices> section tag:

```bash
<qemu:commandline>
    <qemu:arg value='-global'/>
    <qemu:arg value='pcie-root-port.pref64-reserve=512G'/>
</qemu:commandline>
```

Replace 512G with the value appropriate for your hardware if different.

Save and exit the editor. The updated setting will take effect the next time the VM is started.

**8. Check added GPUs are visible on the guest:**

```bash
# sudo virsh start ubuntu22.04-vm1
# sudo virsh domifaddr ubuntu22.04-vm1
# ssh ubuntu@192.168.x.x (password: user1234)
# lspci
```

To set up a RHEL VM, refer to the [Red Hat documentation on preparing and deploying KVM guest images with Image Builder](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/9/html/composing_a_customized_rhel_system_image/preparing-and-deploying-kvm-guest-images-with-image-builder_composing-a-customized-rhel-system-image). The process is similar to the Ubuntu setup, as both utilize QEMU/libvirt for creating and configuring the VM. However, there are some differences: RHEL images are obtained from the Red Hat Customer Portal and their setup may involve using Image Builder for customization, while Ubuntu images are downloaded from the Ubuntu cloud images repository.

It's important to note that assigning GPU VF devices to the VM is not operating system-specific. Described method for adding VF devices is consistent across both RHEL and Ubuntu environments.

## Guest driver setup

Connect to the VM to install ROCm AMDGPU VF Driver:

```bash
# sudo virsh start ubuntu22.04-vm1
# sudo virsh domifaddr ubuntu22.04-vm1
# ssh ubuntu@192.168.x.x (password: user1234)
```

From this point on, the ROCm installation inside the guest VM follows the **same procedure as a standard host ROCm installation**. There are no VM-specific ROCm packages or alternate install paths. Once the VF has been passed through to the guest and `lspci` lists the AMD device, treat the VM as you would any other Linux system running on AMD Instinct hardware.

The ROCm™ software stack and other Radeon™ software for Linux components are installed using the `amdgpu-install` script to assist you in the installation of a coherent set of stack components. For installation steps and after-install verification, please refer to Radeon software for Linux with the [ROCm installation guide](https://rocm.docs.amd.com/en/latest/install/rocm.html) - the same guide used for host installs.

```bash
Note: Loading AMDGPU VF Driver should be done with command:

# sudo modprobe amdgpu
```

## Post-install verification check

To confirm that the entire setup is functioning correctly and that VM can efficiently execute tasks on the GPU, check output from rocminfo and clinfo tools in the VM.

```bash
# sudo rocminfo
```

Output should be as follows:

```bash
[...]

*******

Agent 2

*******

  Name:                    gfx942

  Uuid:                    GPU-664b52e347835f94

  Marketing Name:          AMD Instinct MI300X

  Vendor Name:             AMD

  Feature:                 KERNEL_DISPATCH

[...]
```

Also try following:

```bash
# sudo clinfo
```

Output should be as follows:

```bash
[...]

  Platform Name                                   AMD Accelerated Parallel Processing

  Platform Vendor                                 Advanced Micro Devices, Inc.

  Platform Version                                OpenCL 2.1 AMD-APP (3649.0)

  Platform Profile                                FULL_PROFILE

  Platform Extensions                             cl_khr_icd cl_amd_event_callback

  Platform Extensions function suffix             AMD

  Platform Host timer resolution                  1ns

[...]
```

This marks the final step in setting up the AMD GPUs with MxGPU in KVM/QEMU environments. By following the outlined steps, users can effectively allocate GPU resources across virtual machines, optimizing performance and resource utilization for demanding workloads.

With your environment now configured, consider deploying high-performance computing applications, artificial intelligence models, or machine learning tasks that can fully leverage the compute capabilities of the AMD GPUs. These applications can benefit significantly from the enhanced resource allocation that MxGPU provides.

---

## Next steps

**Congratulations!** Your MxGPU setup is complete.

### Optional maintenance

- **[Removing MxGPU](Removing_MxGPU.md)** - Clean removal procedures when needed
