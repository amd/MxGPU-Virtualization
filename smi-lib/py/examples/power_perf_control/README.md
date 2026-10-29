# Run the AMD SMI power and performance control example

The power and performance control example is an interactive program for
inspecting AMD GPU power and performance controls. It discovers AMD GPUs and
offers these operations:

- Display power caps, power-management state, and SoC P-state policies for
every discovered GPU.
- Set and restore a power cap on one selected GPU.
- Set and restore a SoC P-state policy on one selected GPU.

`py/examples/power_perf_control/amdsmi_power_perf_control.py` is the Python implementation.

The display operation is read-only. The set operations intentionally restore
the original setting after verifying the requested value, but they can still
briefly affect performance. Run them only when it is safe to modify the
selected GPU and verify the final state if an API call fails.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. At least one AMD GPU must be available.

- To test a power cap, the selected GPU has power management enabled and a
supported sensor with an adjustable power-cap range.
- To test a SoC P-state policy, the selected GPU reports at least two
supported policies. The policy applies to all GPUs on the same socket.

Stop or migrate performance-sensitive workloads before selecting a set
operation. Do not rely on the example to restore a setting if the restoration
request reports an error.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/power_perf_control/amdsmi_power_perf_control.py
```

If no GPUs are discovered, the example prints `No GPUs found` and does not
offer an operation.

## Use the interactive prompts

After initialization and GPU discovery, the example presents this menu:


| Input | Operation                                         | Result                                                                         |
| ----- | ------------------------------------------------- | ------------------------------------------------------------------------------ |
| `0`   | Display power and performance control information | Prints information for every discovered GPU without changing configuration.    |
| `1`   | Set and restore a power cap                       | Lists GPUs, then prompts for a GPU, sensor, power-cap value, and confirmation. |
| `2`   | Set and restore a SoC P-state policy              | Lists GPUs, then prompts for a GPU, policy ID, and confirmation.               |
| `3`   | Exit                                              | Makes no change and shuts down AMD SMI.                                        |


Enter an integer from `0` through `3`. The example repeats the prompt for an
invalid or out-of-range operation. It performs at most one operation per run.

The display operation does not require a GPU selection. Each set operation
lists the discovered GPUs with an index and PCI bus:device.function (BDF)
address:

```text
Available GPUs:
GPU 0 bdf: 0000:41:00.0
GPU 1 bdf: 0000:81:00.0
Enter the GPU index to operate on: 1
```

The GPU index must be a non-negative integer in the displayed range. The
example repeats the prompt for an invalid or out-of-range GPU index.

### Display power and performance control information

Choose `0` to inspect every discovered GPU. For each GPU, the example prints
its BDF address and calls:

```python
supported_caps = amdsmi_get_supported_power_cap(processor)
amdsmi_get_power_cap_info(processor, sensor_ind)
amdsmi_is_gpu_power_management_enabled(processor)
amdsmi_get_soc_pstate(processor)
```

For each supported power-cap sensor, the example reports its type, current,
default, dynamic power-management, minimum, and maximum caps. Power-cap
values use platform-specific units, so use the raw minimum and maximum values
reported by the example when entering a new cap.

The SoC P-state output identifies the current policy index and each policy's
policy ID and description.

Support is ASIC- and configuration-specific. If an individual query is not
supported or returns no data, the example reports that result and continues
with the remaining queries.

### Set and restore a power cap

Choose `1` to test a power cap on one GPU. The example first calls
`amdsmi_get_supported_power_cap()` and `amdsmi_get_power_cap_info()` for each
available sensor. It also checks that GPU power management is enabled.

The example lists available sensors using a numbered list:

```text
Available power cap sensors:
0: Sensor 0 (PPT0) current=500000 minimum=300000 maximum=600000
Enter the power cap sensor number to operate on: 0
Enter the power cap to set (300000 to 600000): 450000
```

Enter the displayed sensor number, then enter a raw power-cap value in the
reported inclusive range. The example rejects values outside that range and
does not change a cap that is already active. It then asks for confirmation:

```text
Set PPT0 on sensor 0 from 500000 to 450000? The script will restore the original value. (YES/no)
```

Enter `yes` to continue or `no` to cancel; either value is case-insensitive.
Any other response repeats the confirmation prompt.

When confirmed, the example calls `amdsmi_set_power_cap()`, reads the cap
again, and then attempts to restore the original value:

```python
amdsmi_set_power_cap(processor, sensor_ind, requested_power_cap)
amdsmi_get_power_cap_info(processor, sensor_ind)
amdsmi_set_power_cap(processor, sensor_ind, original_power_cap)
```

The example attempts restoration even if the initial set request reports an
error, because the device state may have changed before the error was
reported.

### Set and restore a SoC P-state policy

Choose `2` to test a SoC P-state policy on one GPU. The example calls
`amdsmi_get_soc_pstate()` and lists the current policy index together with
each valid policy ID:

```text
Current SoC P-state policy index is: 0
Policy index 0 (ID 0) is: AUTO (current)
Policy index 1 (ID 1) is: PERFORMANCE
Enter the SoC P-state policy ID to set: 1
```

Enter a displayed policy ID, not the policy's numbered list index. The example
records the policy ID associated with the current policy index so that it can
restore the original value. If it cannot map the current index to a policy, it
does not perform a change.

After confirmation, the example sets the requested policy, reads the policy
again, then attempts to restore the original policy:

```python
amdsmi_set_soc_pstate(processor, requested_policy_id)
amdsmi_get_soc_pstate(processor)
amdsmi_set_soc_pstate(processor, original_policy_id)
```

SoC P-state policy is socket-wide: setting it on the selected GPU also affects
the other GPUs on the same socket. Ensure that all affected GPUs are idle
before running this operation.

### Handle errors and cleanup

For individual power and SoC P-state APIs, the example reports
`STATUS_NOT_SUPPORTED` and `STATUS_NO_DATA` without aborting the selected
operation.

AMD SMI Python APIs raise `AmdSmiException` subclasses when an operation
fails. For other `AmdSmiLibraryException` failures, the script prints the
associated status text. It reports other exceptions and always attempts to
call `amdsmi_shut_down()` in its `finally` block.

Press `Ctrl+C` to cancel the script. The script reports the interrupt, runs
any active setting-restoration `finally` block, and then attempts the same AMD
SMI cleanup.
