# Run the AMD SMI reset example

The reset example is an interactive program for actions that modify GPU
state. It discovers AMD GPUs, lets you select one by index, and offers these
operations:

- Reset the selected GPU.
- Clear the framebuffer of an inactive virtual function (VF) on the selected
  GPU.

`py/examples/reset/amdsmi_reset.py` is the Python implementation.

Run this example only when it is safe to interrupt work on the selected GPU.
A GPU reset disrupts work running on that GPU. Clearing a VF framebuffer
removes its contents, and the VF must be inactive.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. At least one AMD GPU must be available.

- To clear a VF framebuffer, the selected GPU has enabled VFs and the selected
  VF is inactive.

Stop or migrate workloads from the GPU before choosing the reset operation.
The example asks for confirmation after you select the VF and before it clears
that VF's framebuffer.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/reset/amdsmi_reset.py
```

If no GPUs are discovered, the example prints `No GPUs found` and does not
offer an operation.

## Use the interactive prompts

The example lists each discovered GPU with an index and PCI bus:device.function
(BDF) address. Enter a GPU index to select the device:

```text
Available GPUs:
GPU 0 bdf: 0000:41:00.0
GPU 1 bdf: 0000:81:00.0
Enter the GPU index to operate on: 1
```

The GPU index must be a non-negative integer in the displayed range. The
example repeats the prompt for an invalid or out-of-range GPU index.

After you select a GPU, the example displays its BDF and presents this menu:

| Input | Operation            | Result                                                                                      |
| ----- | -------------------- | ------------------------------------------------------------------------------------------- |
| `0`   | Reset GPU            | Prompts for confirmation before sending the reset request.                                 |
| `1`   | Clear VF framebuffer | Lists the selected GPU's VFs and prompts for confirmation after you select an inactive VF. |
| `2`   | Exit                 | Makes no change and shuts down AMD SMI.                                                     |

Enter an integer from `0` through `2`. The example repeats the prompt for an
invalid or out-of-range operation. It performs at most one operation per run.

### Reset the selected GPU

Choose `0` to call `amdsmi_reset_gpu()` with the selected GPU handle:

```python
processor = processors[pick_gpu_index]
amdsmi_reset_gpu(processor)
```

The example asks for an explicit confirmation:

```text
Are you sure you want to reset the GPU? (YES/no)
```

Enter `yes` to continue or `no` to cancel; either value is case-insensitive.
Any other response repeats the confirmation prompt. When confirmed, the
example sends the reset request and then waits 20 seconds before shutting down
AMD SMI. The wait does not perform an additional health check.

```python
time.sleep(20)
```

### Clear an inactive VF framebuffer

Choose `1` to call `amdsmi_get_vf_partition_info()` for the selected GPU. If
VFs are available, the example calls `amdsmi_get_vf_data()` for each VF and
lists its index, BDF address, and scheduler state:

```text
Available VFs:
VF 0 bdf: 0000:41:00.1 (inactive)
VF 1 bdf: 0000:42:00.1 (active)
Enter the VF index to operate on: 0
Selected VF 0 bdf: 0000:41:00.1 (inactive)
Are you sure you want to clear the VF framebuffer? (YES/no)
```

`AVAILABLE` and `DEFAULT_AVAILABLE` scheduler states are displayed as
`inactive`; other states are displayed by name. The example checks the selected
VF's state again before asking for confirmation. The VF prompt accepts only
indexes whose reported state is inactive. Selecting an active VF reports that
the VF needs to be inactive and repeats the prompt.

For an inactive VF, enter `yes` to clear the framebuffer or `no` to cancel;
either value is case-insensitive. Any other response repeats the confirmation
prompt.

The example calls `amdsmi_clear_vf_fb()` only after confirmation:

```python
valid_vf_indexes = []
for i in range(len(partitions)):
    vf_handle = partitions[i]['vf_id']
    vf_state = amdsmi_get_vf_data(vf_handle)['sched']['state']
    if vf_state in INACTIVE_VF_STATES:
        valid_vf_indexes.append(i)

pick_vf_index = prompt_for_input(
    "  Enter the VF index to operate on: ",
    valid_vf_indexes[0],
    valid_values=valid_vf_indexes)

vf_handle = partitions[pick_vf_index]['vf_id']
vf_state = amdsmi_get_vf_data(vf_handle)['sched']['state']
if vf_state not in INACTIVE_VF_STATES:
    print("The selected VF needs to be inactive.")
elif not prompt_confirmation(
        "  Are you sure you want to clear the VF framebuffer?"):
    print("  No operation was performed. Exiting...")
else:
    amdsmi_clear_vf_fb(vf_handle)
```

If the selected GPU has no VFs, the example prints `No VFs found`. If none of
its VFs are inactive, it prints `No inactive VFs found` and does not prompt
for a VF index. Enter a whole-number index for an inactive VF; any other input
is rejected and prompted again.

### Handle errors and cleanup

AMD SMI Python APIs raise `AmdSmiException` subclasses when an operation
fails. The script prints the associated status text for
`AmdSmiLibraryException`, reports other exceptions, and always attempts to
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
