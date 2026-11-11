<!-- markdownlint-disable MD029 -->

# Removing MxGPU

To remove the MxGPU, unload the driver first:

```bash
# sudo modprobe -r gim
```

If the driver was installed as a .deb package, remove it using:

Ubuntu:

```bash
# sudo dpkg -r <gim-driver-package>
```

RHEL:

```bash
# sudo rpm -e <gim-driver-package>
```

If the driver was built from source, navigate to the source directory and run:

```bash
# make clean
```

To delete VMs, use virsh to list all defined virtual machines:

```bash
# virsh list --all
```

For each VM you want to delete, run:

```bash
# virsh destroy <vm-name>
# virsh undefine <vm-name> --remove-all-storage
```

If the disk image (.qcow2 file) associated whit the VM is not managed by libvirt, remove it manually:

```bash
# rm -f <path_to_qcow2_disk_image>
```

Host configuration can be reverted by updating the grub file to its previous state (added parameters should be deleted) and returning System BIOS settings to their initial values. By doing this SR-IOV environment will be disabled.

For any further support or questions, please don’t hesitate to raise a Github PR.
