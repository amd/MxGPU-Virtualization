# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

import time
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amdsmi import *
from example_utils import prompt_confirmation, prompt_for_input

# A VF must not be running guest work when its framebuffer is cleared.
# Both AVAILABLE states represent VFs that are safe to offer for selection.
INACTIVE_VF_STATES = (
    AmdSmiVfState.AVAILABLE,
    AmdSmiVfState.DEFAULT_AVAILABLE,
)


if __name__ == "__main__":
    initialized = False

    try:
        # INIT_ALL_PROCESSORS is the default. Choose a different
        # AmdSmiInitFlags value when only a specific processor type is needed.
        init_flags = AmdSmiInitFlags.INIT_ALL_PROCESSORS
        print("###Test amdsmi_init")
        print("  Initializing with {}".format(init_flags.name))
        amdsmi_init(init_flags)
        initialized = True
        print("  Initialization complete")

        # Processor discovery is the shared prelude for AMD SMI examples.
        print("###Test amdsmi_get_processor_handles")
        processors = amdsmi_get_processor_handles()

        if len(processors) == 0:
            print("  No GPUs found")
        else:
            # Display each GPU's BDF so the user can identify the physical
            # device before choosing a potentially disruptive operation.
            print("  Available GPUs:")
            for i in range(len(processors)):
                print("  GPU {} bdf: {}".format(i, amdsmi_get_gpu_device_bdf(processors[i])))
            
            # Prompt the user to select a GPU, and validate that the selected index belongs to
            # one of the discovered GPU handles.
            pick_gpu_index = prompt_for_input(
                "  Enter the GPU index to operate on: ",
                0,
                upper_bound=len(processors) - 1,
                error_message="  GPU index must be between 0 and {}".format(
                    len(processors) - 1))

            processor = processors[pick_gpu_index]
            print("###Test amdsmi_get_gpu_device_bdf")
            bdf = amdsmi_get_gpu_device_bdf(processor)
            print("  Selected GPU {} bdf: {}\n".format(pick_gpu_index, bdf))

            # GPU reset affects the whole physical device.
            # Clearing a VF framebuffer affects only an inactive VF.
            print("  Available operations:")
            print("  0: Reset GPU (Resetting a GPU disrupts work running on that GPU.)")
            print("  1: Clear VF framebuffer (A VF framebuffer can be cleared only when the VF is inactive.)")
            print("  2: Exit")

            pick_operation = prompt_for_input(
                "  Enter the operation to perform: ",
                2,
                upper_bound=2,
                error_message="  Operation must be between 0 and 2")

            if pick_operation == 2:
                print("  No operation was performed. Exiting...")
            elif pick_operation == 0:
                # Require explicit confirmation because a reset interrupts
                # all work currently running on the selected GPU.
                if not prompt_confirmation(
                        "  Are you sure you want to reset the GPU?"):
                    print("  No operation was performed. Exiting...")
                else:
                    # Resetting a GPU disrupts work running on that GPU.
                    print("###Test amdsmi_reset_gpu")
                    print("  Resetting GPU {} bdf: {}".format(pick_gpu_index, bdf))
                    amdsmi_reset_gpu(processor)
                    print("  Reset request completed")
                    # The reset API returns after submitting the request.
                    # Allow the device time to become available again before shutting down.
                    print("  Waiting for GPU reset to complete...")
                    time.sleep(20)
            else:
                # A VF framebuffer can be cleared only when the VF is inactive.
                print("###Test amdsmi_get_vf_partition_info")
                partitions = amdsmi_get_vf_partition_info(processor)
                if len(partitions) == 0:
                    print("  No VFs found")
                else:
                    # Inspect every VF and retain only inactive entries
                    # as valid choices for the framebuffer-clear operation.
                    print("  Available VFs:")
                    valid_vf_indexes = []
                    for i in range(len(partitions)):
                        vf_handle = partitions[i]['vf_id']
                        vf_state = amdsmi_get_vf_data(vf_handle)['sched']['state']
                        if vf_state in INACTIVE_VF_STATES:
                            valid_vf_indexes.append(i)
                        state_label = (
                            "inactive"
                            if vf_state in INACTIVE_VF_STATES
                            else vf_state.name.lower().replace("_", " ")
                        )
                        print("  VF {} bdf: {} ({})".format(
                            i, amdsmi_get_vf_bdf(vf_handle), state_label))

                    if not valid_vf_indexes:
                        print("  No inactive VFs found")
                    else:
                        # valid_values prevents an active VF from being
                        # selected even though it appears in the VF list.
                        pick_vf_index = prompt_for_input(
                            "  Enter the VF index to operate on: ",
                            valid_vf_indexes[0],
                            upper_bound=len(partitions) - 1,
                            valid_values=valid_vf_indexes,
                            error_message=(
                                "  The VF needs to be inactive. Valid inactive "
                                "VF indexes are: {}".format(
                                    ", ".join(
                                        str(index)
                                        for index in valid_vf_indexes))
                            ))

                        vf_handle = partitions[pick_vf_index]['vf_id']
                        # Read the state again after selection in case the VF
                        # became active while the user was choosing it.
                        vf_state = amdsmi_get_vf_data(
                            vf_handle)['sched']['state']
                        state_label = (
                            "inactive"
                            if vf_state in INACTIVE_VF_STATES
                            else vf_state.name.lower().replace("_", " ")
                        )
                        print("###Test amdsmi_get_vf_bdf")
                        vf_bdf = amdsmi_get_vf_bdf(vf_handle)
                        print("  Selected VF {} bdf: {} ({})".format(
                            pick_vf_index, vf_bdf, state_label))

                        if vf_state not in INACTIVE_VF_STATES:
                            print("  The selected VF needs to be inactive "
                                  "before its framebuffer can be cleared.")
                        elif not prompt_confirmation(
                                "  Are you sure you want to clear the VF framebuffer?"):
                            print("  No operation was performed. Exiting...")
                        else:
                            # Clearing removes the selected inactive VF's
                            # framebuffer contents without resetting the GPU.
                            print("###Test amdsmi_clear_vf_fb")
                            print("  Clearing framebuffer for VF {}".format(
                                pick_vf_index))
                            amdsmi_clear_vf_fb(vf_handle)
                            print("  VF framebuffer clear completed")

    except AmdSmiLibraryException as e:
        status = AmdSmiRetCode(e.err_code)
        print((amdsmi_status_code_to_string(status)))
    except AmdSmiException as e:
        print(e)
    except KeyboardInterrupt:
        print("\n  Keyboard interrupt detected. Exiting...")
    except Exception as e:
        print(e)
    finally:
        # Always release AMD SMI resources, including after an API error or
        # keyboard interrupt.
        if initialized:
            try:
                print("###Test amdsmi_shut_down")
                amdsmi_shut_down()
                print("Shutdown complete")
            except AmdSmiException as e:
                print(e)
