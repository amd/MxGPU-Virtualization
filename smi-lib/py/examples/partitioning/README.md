# Run the AMD SMI partitioning example

The partitioning example is an interactive program for inspecting and
configuring AMD GPU memory and accelerator partitioning. It discovers AMD
GPUs and offers these operations:

- Display memory and accelerator partitioning information for every discovered
  GPU.
- Set the memory partition mode across a selected GPU's XGMI hive.
- Set the accelerator partition profile across a selected GPU's XGMI hive.

`py/examples/partitioning/amdsmi_partitioning.py` is the Python implementation.

The display operation is read-only. The set operations change GPU
configuration across every GPU in the selected GPU's XGMI hive, can disrupt
workloads, and require explicit confirmation.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. At least one AMD GPU must be available.

- The GPU supports the memory and/or accelerator partitioning APIs. These APIs
  are available only on supported ASICs and configurations.

Before changing memory partition mode, ensure that every GPU in the selected
GPU's XGMI hive is idle and has no workloads. Before changing an accelerator
partition profile, ensure that every GPU in the hive is idle and has no active
VFs. An accelerator partition profile can only be set while the memory
partition mode is one of the modes that profile supports, so the example may
prompt for a memory partition mode change first.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/partitioning/amdsmi_partitioning.py
```

If no GPUs are discovered, the example prints `No GPUs found` and does not offer
an operation.

## Use the interactive prompts

After initialization and GPU discovery, the example presents this menu:

| Input | Operation                                                            | Result                                                                                                                                                |
| ----- | -------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| `0`   | Display memory and accelerator partitioning information for all GPUs | Prints the supported and current memory and accelerator partitioning information, and the valid combinations, for every discovered GPU.              |
| `1`   | Set GPU memory partition mode across an XGMI hive                    | Lists GPUs, prompts for a GPU and supported memory partition mode, then confirms the hive-wide operation before setting GPU partition mode. |
| `2`   | Set accelerator partition profile across an XGMI hive                | Lists GPUs, prompts for a GPU and a profile index settable in the current memory partition mode, then confirms the hive-wide operation before setting GPU accelerator partition profile. |
| `3`   | Exit                                                                 | Makes no change and shuts down AMD SMI.                                                                                                               |

Enter an integer from `0` through `3`. The example repeats the prompt for an
invalid or out-of-range operation. It performs at most one operation per run.

### Display memory and accelerator partitioning information

Choose `0` to inspect partitioning information for every discovered GPU. For
each GPU, the example prints its PCI bus:device.function (BDF) address and
queries:

```python
amdsmi_get_gpu_memory_partition_config(processor)
```

The memory partition configuration reports:

- The current memory partition mode (`mp_mode`).
- Supported memory partition modes (`partition_caps`).
- NUMA ranges, including memory type and address range (`numa_range`).

```python
amdsmi_get_gpu_accelerator_partition_profile_config(processor)
amdsmi_get_gpu_accelerator_partition_profile_config_global(processor)
amdsmi_get_gpu_accelerator_partition_profile(processor)
```

The accelerator partition configuration reports the default profile index,
resource profiles, and available accelerator profiles. Each accelerator profile
includes its profile index and type, partition count, supported memory
partition modes, and resource profile indices for each partition.

The global accelerator partition configuration additionally reports the
supported VF modes for each profile. The current accelerator partition query
reports the active profile and its current partition IDs.

After those queries, the example summarizes the valid combinations. An
accelerator partition profile can only be set while the memory partition mode
is one of the modes the profile reports in `memory_caps`, so the summary lists
the modes each profile requires. The active profile is marked `current`. Other
profiles that are compatible with the current memory partition mode are marked
`settable now`:

```text
Valid memory partition mode and accelerator partition profile combinations:
  profile 0 (SPX, 1 partition(s)) valid memory partition modes: NPS1 (1) <- settable now
  profile 1 (CPX, 8 partition(s)) valid memory partition modes: NPS1 (1), NPS4 (4) <- current
Current memory partition mode is NPS1 (1). A profile that does not list it requires a memory partition mode change first.
```

This summary appears only when both the memory partition configuration and
the accelerator partition profile configuration are available.

Partitioning support is ASIC-specific. The example treats each query
independently: if one API is not supported or returns no data, it reports that
result and continues with the other partitioning queries.

### Set GPU memory partition mode

Choose `1` to change memory partition mode across the selected GPU's XGMI
hive. The example lists each discovered GPU with an index and BDF address.
Enter a GPU index to select the hive through one of its devices:

```text
Available GPUs:
GPU 0 bdf: 0000:41:00.0
GPU 1 bdf: 0000:81:00.0
Enter the GPU index to operate on: 1
```

The GPU index must be a non-negative integer in the displayed range. The
example repeats the prompt for an invalid or out-of-range GPU index.

After you select a GPU, the example calls
`amdsmi_get_gpu_memory_partition_config()` and displays the current mode and
the supported modes. It also calls
`amdsmi_get_gpu_accelerator_partition_profile_config()` and prints the valid
combinations, so you can see which accelerator partition profiles each memory
partition mode allows before choosing one. It presents the supported modes as
a numbered list:

```text
Available memory partition modes:
0: NPS1 (1)
1: NPS2 (2)
Enter the memory partition mode to set: 1
```

Enter the list index for the desired supported mode. The example repeats the
prompt for an invalid or out-of-range index, then asks for an explicit
confirmation:

```text
Are you sure you want to set memory partition mode to NPS2? This operation applies to all GPUs in the selected GPU's XGMI hive. All GPUs in the hive must be idle with no workloads. (YES/no)
```

Enter `yes` to continue or `no` to cancel; either value is case-insensitive.
Any other response repeats the confirmation prompt. When confirmed, the example
calls:

```python
amdsmi_set_gpu_memory_partition_mode(processor, requested_mode)
```

Every GPU in the selected GPU's XGMI hive must be idle and have no workloads
during this operation. Changing the memory partition mode also changes the
accelerator partition profile to the default profile of the new memory
partition mode, which the example reports after a successful request. It then
reads and displays the memory partition configuration and the current
accelerator partition profile again.

### Set accelerator partition profile

Choose `2` to change the accelerator partition profile across the selected
GPU's XGMI hive. The example first lists GPUs and prompts for a GPU index using
the same validation as the memory partition operation.

It then calls `amdsmi_get_gpu_memory_partition_config()` and
`amdsmi_get_gpu_accelerator_partition_profile_config()` and prints the valid
combinations. A reported profile index is not necessarily settable right now:
the current memory partition mode must be one of the modes the profile reports
in `memory_caps`, or `amdsmi_set_gpu_accelerator_partition_profile()` fails.
The example therefore separates the profiles it can set now from the ones that
require a different memory partition mode:

```text
These accelerator partition profiles cannot be set while the memory partition mode is NPS4:
  profile 0 (SPX) valid memory partition modes: NPS1 (1)
To use one of those profiles, the memory partition mode must be changed first.
Do you want to change the memory partition mode now? (YES/no)
```

Answer `no` to keep the current memory partition mode and continue with the
profiles that are settable now. Answer `yes` and the example prompts for one of
the memory partition modes that make the blocked profiles available, then
changes it with `amdsmi_set_gpu_memory_partition_mode()`. Because that change
also resets the accelerator partition profile, the example asks you to run
operation `2` again to select the profile; it does not set the profile in the
same run.

When no profile at all is settable in the current memory partition mode, the
example reports that and offers the same memory partition mode change instead
of prompting for a profile.

The prompt for the profile index lists only the profiles that are settable in
the current memory partition mode:

```text
Accelerator partition profile indices that can be set now: 1
Enter the accelerator partition profile index to set: 1
```

Enter one of the displayed profile index values, rather than its position in
the list. The example repeats the prompt until the value is one of them. It
then asks for confirmation:

```text
Are you sure you want to set accelerator partition profile 1? This operation applies to all GPUs in the selected GPU's XGMI hive. All GPUs in the hive must be idle with no active VFs. (YES/no)
```

Enter `yes` to continue or `no` to cancel; either value is case-insensitive.
Any other response repeats the confirmation prompt. When confirmed, the example
calls:

```python
amdsmi_set_gpu_accelerator_partition_profile(processor, pick_profile_index)
```

Changing an accelerator partition profile affects every GPU in the selected
GPU's XGMI hive and can disrupt workloads. The example then queries the current
accelerator partition profile and displays the active profile and partition
IDs.

### Handle errors and cleanup

AMD SMI Python APIs raise `AmdSmiException` subclasses when an operation
fails. For the partitioning queries and set operations, the script reports
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
