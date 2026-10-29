# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amdsmi import *
from example_utils import (
    get_optional_api,
    prompt_confirmation,
    prompt_for_input,
    set_optional_api,
)


def print_framebuffer_layout(processor):
    # The framebuffer layout describes how the GPU's memory is split between
    # the PF and its VFs. The usable VF range bounds how much framebuffer each
    # VF can be given.
    print("###Test amdsmi_get_fb_layout")
    fb_layout = get_optional_api(
        "amdsmi_get_fb_layout", amdsmi_get_fb_layout, processor)
    if fb_layout is not None:
        print("  total_fb_size is: {} MB".format(
            fb_layout['total_fb_size']))
        print("  pf_fb_reserved is: {} MB".format(
            fb_layout['pf_fb_reserved']))
        print("  pf_fb_offset is: {} MB".format(
            fb_layout['pf_fb_offset']))
        print("  fb_alignment is: {} MB".format(
            fb_layout['fb_alignment']))
        print("  max_vf_fb_usable is: {} MB".format(
            fb_layout['max_vf_fb_usable']))
        print("  min_vf_fb_usable is: {} MB".format(
            fb_layout['min_vf_fb_usable']))
    print("")


def print_vf_info(vf_index, vf_handle):
    # VF info is the configuration the host gave the VF: where its
    # framebuffer starts, how large it is, and its GFX scheduling timeslice.
    print("###Test amdsmi_get_vf_info")
    vf_info = get_optional_api(
        "amdsmi_get_vf_info", amdsmi_get_vf_info, vf_handle)
    if vf_info is not None:
        print("  VF {} fb_offset is: {} MB".format(
            vf_index, vf_info['fb']['fb_offset']))
        print("  VF {} fb_size is: {} MB".format(
            vf_index, vf_info['fb']['fb_size']))
        print("  VF {} gfx_timeslice is: {} us".format(
            vf_index, vf_info['gfx_timeslice']))
    print("")


def print_vf_data(vf_index, vf_handle):
    # VF data is runtime state reported by the host scheduler, unlike the
    # static configuration returned by amdsmi_get_vf_info.
    print("###Test amdsmi_get_vf_data")
    vf_data = get_optional_api(
        "amdsmi_get_vf_data", amdsmi_get_vf_data, vf_handle)
    if vf_data is None:
        print("")
        return

    # Scheduler statistics: FLR (function level reset) count, how long the
    # last boot, shutdown, and reset took, the VF's current scheduling
    # state, and timestamps for the most recent lifecycle events.
    sched = vf_data['sched']
    print("  VF {} sched['flr_count'] is: {}".format(
        vf_index, sched['flr_count']))
    print("  VF {} sched['boot_up_time'] is: {} us".format(
        vf_index, sched['boot_up_time']))
    print("  VF {} sched['shutdown_time'] is: {} us".format(
        vf_index, sched['shutdown_time']))
    print("  VF {} sched['reset_time'] is: {} us".format(
        vf_index, sched['reset_time']))
    print("  VF {} sched['state'] is: {}".format(
        vf_index, sched['state'].name))
    print("  VF {} sched['last_boot_start'] is: {}".format(
        vf_index, sched['last_boot_start']))
    print("  VF {} sched['last_boot_end'] is: {}".format(
        vf_index, sched['last_boot_end']))
    print("  VF {} sched['last_shutdown_start'] is: {}".format(
        vf_index, sched['last_shutdown_start']))
    print("  VF {} sched['last_shutdown_end'] is: {}".format(
        vf_index, sched['last_shutdown_end']))
    print("  VF {} sched['last_reset_start'] is: {}".format(
        vf_index, sched['last_reset_start']))
    print("  VF {} sched['last_reset_end'] is: {}".format(
        vf_index, sched['last_reset_end']))
    print("  VF {} sched['current_active_time'] is: {}".format(
        vf_index, sched['current_active_time']))
    print("  VF {} sched['current_running_time'] is: {}".format(
        vf_index, sched['current_running_time']))
    print("  VF {} sched['total_active_time'] is: {}".format(
        vf_index, sched['total_active_time']))
    print("  VF {} sched['total_running_time'] is: {}".format(
        vf_index, sched['total_running_time']))

    # Guards limit how often a VF can trigger an event type (for example
    # FLRs) within an interval. guard['guard'] maps each event type to its
    # current state, event count, interval, and threshold.
    guard = vf_data['guard']
    print("  VF {} guard['enabled'] is: {}".format(
        vf_index, guard['enabled']))
    for guard_type, guard_info in guard['guard'].items():
        print("  VF {} {} guard:".format(vf_index, guard_type))
        print("    state is: {}".format(
            guard_info['state'].name))
        print("    amount is: {}".format(guard_info['amount']))
        print("    interval is: {} seconds".format(
            guard_info['interval']))
        print("    threshold is: {}".format(guard_info['threshold']))
        print("    active is: {}".format(guard_info['active']))
    print("")


def print_guest_data(vf_index, vf_handle):
    # Guest data comes from the driver running inside the VM, so it is only
    # available while a guest driver is loaded on the VF.
    print("###Test amdsmi_get_guest_data")
    guest_data = get_optional_api(
        "amdsmi_get_guest_data", amdsmi_get_guest_data, vf_handle)
    if guest_data is not None:
        print("  VF {} driver_version is: {}".format(
            vf_index, guest_data['driver_version']))
        print("  VF {} fb_usage is: {} MB".format(
            vf_index, guest_data['fb_usage']))
    print("")


def print_vf_hbm_info(vf_index, vf_handle):
    # HBM info reports the physical memory range backing the VF and the
    # NUMA node it belongs to.
    print("###Test amdsmi_get_vf_hbm_info")
    hbm_info = get_optional_api(
        "amdsmi_get_vf_hbm_info", amdsmi_get_vf_hbm_info, vf_handle)
    if hbm_info is not None:
        print("  VF {} phy_addr is: {}".format(
            vf_index, hex(hbm_info['phy_addr'])))
        print("  VF {} phy_size is: {} bytes".format(
            vf_index, hbm_info['phy_size']))
        print("  VF {} numa_id is: {}".format(
            vf_index, hbm_info['numa_id']))
        print("  VF {} HBM name is: {}".format(
            vf_index, hbm_info['name']))
    print("")


def print_vf_partitions(processor):
    # The partition list has one entry per enabled VF. Each entry holds the
    # VF handle that all the per-VF APIs below take as input.
    print("###Test amdsmi_get_vf_partition_info")
    partitions = get_optional_api(
        "amdsmi_get_vf_partition_info", amdsmi_get_vf_partition_info,
        processor)
    if partitions is None:
        print("")
        return

    print("  number of enabled VFs is: {}".format(len(partitions)))
    if len(partitions) == 0:
        print("  No enabled VFs found; per-VF APIs require an enabled VF")
        print("")
        return

    # Print each VF's framebuffer slice, then query every per-VF API with
    # that VF's handle.
    for vf_index, partition in enumerate(partitions):
        vf_handle = partition['vf_id']
        print("  VF {} partition fb_offset is: {} MB".format(
            vf_index, partition['fb']['fb_offset']))
        print("  VF {} partition fb_size is: {} MB\n".format(
            vf_index, partition['fb']['fb_size']))

        print_vf_info(vf_index, vf_handle)
        print_vf_data(vf_index, vf_handle)
        print_guest_data(vf_index, vf_handle)
        print_vf_hbm_info(vf_index, vf_handle)


def print_sriov_info(processor_number, processor):
    # Print the GPU's BDF first so the output below can be matched to a
    # physical device.
    print("###Test amdsmi_get_gpu_device_bdf")
    bdf = amdsmi_get_gpu_device_bdf(processor)
    print("  Processor number: {} bdf: {}\n".format(processor_number, bdf))

    # The enabled VF count tells us whether per-VF APIs can be queried.
    print("###Test amdsmi_get_num_vf")
    vf_count = get_optional_api(
        "amdsmi_get_num_vf", amdsmi_get_num_vf, processor)
    if vf_count is not None:
        print("  Number of enabled VFs is: {}".format(
            vf_count['num_vf_enabled']))
        print("  Number of supported VFs is: {}\n".format(
            vf_count['num_vf_supported']))
    else:
        print("")

    # GPU-level SR-IOV information can be shown even when no VFs are
    # enabled. Per-VF information is skipped if the VF count is unavailable.
    print_framebuffer_layout(processor)
    if vf_count is not None:
        print_vf_partitions(processor)


def set_num_vf(processor):
    # Changing the VF count persists beyond this script and can disrupt
    # workloads, so it requires a validated count and confirmation.
    print("###Test amdsmi_get_num_vf")
    vf_count = get_optional_api(
        "amdsmi_get_num_vf", amdsmi_get_num_vf, processor)
    if vf_count is None:
        return
    print("  Number of enabled VFs is: {}".format(
        vf_count['num_vf_enabled']))
    print("  Number of supported VFs is: {}".format(
        vf_count['num_vf_supported']))

    # Read the current and supported VF counts first.
    # The supported count is the upper bound for the new value.
    print("  Note: You cannot set the enabled VF count higher than the "
          "supported VF count ({}).".format(vf_count['num_vf_supported']))
    # Prompt for the new VF count, defaulting to the current count.
    # Reject values above the supported count.
    pick_num_vf = prompt_for_input(
        "  Enter the enabled VF count to set: ",
        vf_count['num_vf_enabled'],
        upper_bound=vf_count['num_vf_supported'],
        error_message="  VF count must be between 0 and {}".format(
            vf_count['num_vf_supported']))

    # Require explicit confirmation because changing the VF count can
    # disrupt VMs that are using the existing VFs.
    if not prompt_confirmation(
            "  Are you sure you want to change enabled VFs from {} to {}?"
            .format(vf_count['num_vf_enabled'], pick_num_vf)):
        print("  No operation was performed. Exiting...")
        return

    # Submit the change request.
    print("###Test amdsmi_set_num_vf")
    print("  Changing enabled VFs from {} to {}".format(
        vf_count['num_vf_enabled'], pick_num_vf))
    if set_optional_api("amdsmi_set_num_vf", amdsmi_set_num_vf, processor,
                        pick_num_vf) != AmdSmiRetCode.SUCCESS:
        return
    print("  VF count change request completed")

    # Read the VF count again to verify the requested count is now active.
    print("###Test amdsmi_get_num_vf")
    updated_vf_count = get_optional_api(
        "amdsmi_get_num_vf", amdsmi_get_num_vf, processor)
    if updated_vf_count is not None:
        print("  Number of enabled VFs is: {}".format(
            updated_vf_count['num_vf_enabled']))
        print("  Number of supported VFs is: {}".format(
            updated_vf_count['num_vf_supported']))


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
        print("  number of GPUs is: {}\n".format(len(processors)))
        if len(processors) == 0:
            print("  No GPUs found.")
        else:
            # Displaying information is read-only and runs on every GPU.
            # Setting the VF count changes the selected GPU's configuration.
            print("  Available operations:")
            print("  0: Display SR-IOV and VF information")
            print("  1: Set enabled VF count (Changing the VF count can disrupt workloads.)")
            print("  2: Exit")

            pick_operation = prompt_for_input(
                "  Enter the operation to perform: ",
                0,
                upper_bound=2,
                error_message="  Operation must be between 0 and 2")

            if pick_operation == 2:
                print("  No operation was performed.")
            elif pick_operation == 0:
                for i, processor in enumerate(processors):
                    print_sriov_info(i, processor)
            else:
                # Display each GPU's BDF so the user can identify the physical
                # device before changing its VF count.
                print("  Available GPUs:")
                for i in range(len(processors)):
                    print("  GPU {} bdf: {}".format(
                        i, amdsmi_get_gpu_device_bdf(processors[i])))

                # Prompt the user to select a GPU, and validate that the
                # selected index belongs to one of the discovered GPU handles.
                pick_gpu_index = prompt_for_input(
                    "  Enter the GPU index to operate on: ",
                    0,
                    upper_bound=len(processors) - 1,
                    error_message=(
                        "  GPU index must be between 0 and {}".format(
                            len(processors) - 1)
                    ))

                processor = processors[pick_gpu_index]
                print("###Test amdsmi_get_gpu_device_bdf")
                bdf = amdsmi_get_gpu_device_bdf(processor)
                print("  Selected GPU {} bdf: {}\n".format(pick_gpu_index, bdf))
                set_num_vf(processor)

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
