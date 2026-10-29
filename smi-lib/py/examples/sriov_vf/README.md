# Run the AMD SMI SR-IOV VF example

The SR-IOV VF example is an interactive program for SR-IOV and virtual function
(VF) queries and configuration. It discovers AMD GPUs and offers these
operations:

- Display SR-IOV and VF information for every discovered GPU.
- Set the enabled VF count on a selected GPU.

`py/examples/sriov_vf/amdsmi_sriov_vf.py` is the Python implementation.

Run the set-VF-count operation only when it is safe to change SR-IOV
configuration on the selected GPU. Changing the enabled VF count can disrupt
workloads, and the change persists beyond this example.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. At least one AMD GPU must be available.

- To display per-VF details, the selected GPU has enabled VFs.
- To set the enabled VF count, the selected GPU supports SR-IOV and the
requested count is within the supported range.

Display is read-only. Stop or migrate workloads before choosing the set
enabled VF count operation.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/sriov_vf/amdsmi_sriov_vf.py
```

If no GPUs are discovered, the example prints `No GPUs found.` and does not
offer an operation.

## Use the interactive prompts

After initialization and GPU discovery, the example prints the number of GPUs
and presents this menu:

| Input | Operation                         | Result                                                                                      |
| ----- | --------------------------------- | ------------------------------------------------------------------------------------------- |
| `0`   | Display SR-IOV and VF information | Prints SR-IOV and per-VF details for every discovered GPU.                                  |
| `1`   | Set enabled VF count              | Lists GPUs, prompts for a GPU and VF count, then confirms before setting enabled VF count. |
| `2`   | Exit                              | Makes no change and shuts down AMD SMI.                                                     |

Enter an integer from `0` through `2`. The example repeats the prompt for an
invalid or out-of-range operation. It performs at most one operation per run.

### Display SR-IOV and VF information

Choose `0` to walk every discovered GPU and print its SR-IOV configuration.
For each GPU, the example starts with the BDF and enabled/supported VF counts:

```python
for i, processor in enumerate(processors):
    bdf = amdsmi_get_gpu_device_bdf(processor)
    vf_count = amdsmi_get_num_vf(processor)
```

It then queries framebuffer layout and each enabled VF. For every enabled VF,
the example reports partition framebuffer
placement and calls:

```python
partitions = amdsmi_get_vf_partition_info(processor)
for vf_index, partition in enumerate(partitions):
    vf_handle = partition['vf_id']
    amdsmi_get_vf_info(vf_handle)
    amdsmi_get_vf_data(vf_handle)
    amdsmi_get_guest_data(vf_handle)
    amdsmi_get_vf_hbm_info(vf_handle)
```

If a GPU has no enabled VFs, the example prints
`No enabled VFs found; per-VF APIs require an enabled VF` and skips the
per-VF queries for that GPU. Optional APIs that are not supported or return
no data are reported without aborting the run.

### Set the enabled VF count

Choose `1` to change the enabled VF count on one GPU. The example lists each
discovered GPU with an index and PCI bus:device.function (BDF) address. Enter
a GPU index to select the device:

```text
Available GPUs:
GPU 0 bdf: 0000:41:00.0
GPU 1 bdf: 0000:81:00.0
Enter the GPU index to operate on: 1
```

The GPU index must be a non-negative integer in the displayed range. The
example repeats the prompt for an invalid or out-of-range GPU index. After you
select a GPU, the example displays its BDF and the current enabled and
supported VF counts.

Enter the enabled VF count to set:

```text
Enter the enabled VF count to set: 4
```

The count must be an integer from `0` through the supported maximum returned
by `amdsmi_get_num_vf()`.

The example repeats the prompt for an invalid or out-of-range value. After you
choose a count, the example asks for an explicit confirmation:

```text
Are you sure you want to change enabled VFs from 1 to 4? (YES/no)
```

Enter `yes` to continue or `no` to cancel; either value is case-insensitive.
Any other response repeats the confirmation prompt.

When confirmed, the example calls `amdsmi_set_num_vf()` and then reads the VF
count again to verify the requested value:

```python
amdsmi_set_num_vf(processor, pick_num_vf)
updated_vf_count = amdsmi_get_num_vf(processor)
```

Changing the VF count can disrupt workloads. If the set or follow-up get call
fails, the example reports the error and continues to cleanup.

### Handle errors and cleanup

AMD SMI Python APIs raise `AmdSmiException` subclasses when an operation
fails. For optional SR-IOV and VF APIs, the script reports
`STATUS_NOT_SUPPORTED` and `STATUS_NO_DATA` without aborting the run. For
other `AmdSmiLibraryException` failures outside those paths, it prints the
associated status text. It reports other exceptions and always attempts to
call `amdsmi_shut_down()` in its `finally` block. Press `Ctrl+C` to cancel the
script; it reports the interrupt and then attempts the same cleanup.

```python
except AmdSmiLibraryException as e:
    status = AmdSmiRetCode(e.err_code)
    print((amdsmi_status_code_to_string(status)))
except KeyboardInterrupt:
    print("\n  Keyboard interrupt detected. Exiting...")
finally:
    if initialized:
        amdsmi_shut_down()
```
