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


def format_memory_partition_modes(modes):
    if len(modes) == 0:
        return "None"

    return ", ".join("{}".format(mode.name, mode.value)
                     for mode in modes)


def print_memory_partition_config(processor):
    # This query reports the active memory partition mode, all modes supported
    # by the GPU, and the NUMA address ranges created by the active mode.
    print("###Test amdsmi_get_gpu_memory_partition_config")
    config = get_optional_api(
        "amdsmi_get_gpu_memory_partition_config",
        amdsmi_get_gpu_memory_partition_config, processor)
    # Partitioning APIs are not available on every ASIC. get_optional_api
    # reports that condition and lets the example continue with other queries.
    if config is None:
        print("")
        return None

    # mp_mode is the mode currently active on the physical GPU, while
    # partition_caps lists every mode that may be requested on this GPU.
    mode = config['mp_mode']
    print("  Current memory partition mode is: {}".format(
        mode.name))
    print("  Supported memory partition modes are: {}".format(
        format_memory_partition_modes(config['partition_caps'])))

    numa_ranges = config['numa_range']
    print("  number of NUMA ranges is: {}".format(len(numa_ranges)))
    # Each range describes one memory region exposed by the current partition
    # mode, including its memory type and start and end addresses.
    for range_index, numa_range in enumerate(numa_ranges):
        memory_type = numa_range['memory_type']
        print("  NUMA range {} memory_type is: {}".format(
            range_index, memory_type.name))
        print("  NUMA range {} start is: {}".format(
            range_index, numa_range['start']))
        print("  NUMA range {} end is: {}".format(
            range_index, numa_range['end']))
    print("")
    return config


def print_resource_profiles(resource_profiles, indent):
    # Resource profiles describe how a hardware resource is divided and how
    # many accelerator partitions share each resulting resource partition.
    if len(resource_profiles) == 0:
        print("{}No resource profiles found".format(indent))
        return

    for resource_profile in resource_profiles:
        resource_type = resource_profile['resource_type']
        print("{}Profile {} {} resource: partition_resource={} "
              "num_partitions_share_resource={}".format(
                  indent, resource_profile['profile_index'],
                  resource_type.name,
                  resource_profile['partition_resource'],
                  resource_profile['num_partitions_share_resource']))


def print_accelerator_profile(profile, indent):
    # An accelerator profile describes one complete accelerator-partition
    # layout. Its profile_index is the value accepted by the setter API.
    profile_type = profile['profile_type']
    print("{}profile_index is: {}".format(
        indent, profile['profile_index']))
    print("{}profile_type is: {}".format(
        indent, profile_type.name))
    print("{}num_partitions is: {}".format(
        indent, profile['num_partitions']))
    print("{}memory_caps are: {}".format(
        indent, format_memory_partition_modes(profile['memory_caps'])))

    vf_modes = profile.get('vf_mode')
    if vf_modes is not None:
        # vf_mode is reported only by the global accelerator configuration.
        # The per-GPU configuration does not include supported VF modes.
        print("{}supported VF modes are: {}".format(
            indent, ", ".join(vf_modes) if len(vf_modes) > 0 else "None"))

    # Each entry corresponds to one accelerator partition. The values are
    # indices into the resource_profiles printed with this configuration.
    for partition_index, resources in enumerate(profile['resources']):
        print("{}partition {} resource profile indices are: {}".format(
            indent, partition_index,
            ", ".join(str(resource) for resource in resources)))


def print_accelerator_partition_profile_config(processor):
    # This per-GPU query returns the accelerator layouts currently configured
    # on the device and the resource profiles those layouts reference.
    print("###Test amdsmi_get_gpu_accelerator_partition_profile_config")
    config = get_optional_api(
        "amdsmi_get_gpu_accelerator_partition_profile_config",
        amdsmi_get_gpu_accelerator_partition_profile_config, processor)
    if config is None:
        print("")
        return None

    print("  default_profiles_index is: {}".format(
        config['default_profiles_index']))
    print("  Resource profiles:")
    print_resource_profiles(config['resource_profiles'], "    ")
    print("  Accelerator partition profiles:")
    if len(config['profiles']) == 0:
        print("    No accelerator partition profiles found")
    for profile in config['profiles']:
        print_accelerator_profile(profile, "    ")
    print("")
    return config


def print_global_accelerator_partition_profile_config(processor):
    # The global query describes hive-wide accelerator layouts. This can
    # differ from the per-GPU configuration printed by the previous query.
    print("###Test amdsmi_get_gpu_accelerator_partition_profile_config_global")
    config = get_optional_api(
        "amdsmi_get_gpu_accelerator_partition_profile_config_global",
        amdsmi_get_gpu_accelerator_partition_profile_config_global, processor)
    if config is None:
        print("")
        return

    print("  default_profiles_index is: {}".format(
        config['default_profiles_index']))
    print("  Global resource profiles:")
    print_resource_profiles(config['resource_profiles'], "    ")
    print("  Global accelerator partition profiles:")
    if len(config['profiles']) == 0:
        print("    No global accelerator partition profiles found")
    for profile in config['profiles']:
        print_accelerator_profile(profile, "    ")
    print("")


def print_current_accelerator_partition_profile(processor):
    # Read back the active profile and the partition identifiers assigned
    # under that profile.
    print("###Test amdsmi_get_gpu_accelerator_partition_profile")
    current_profile = get_optional_api(
        "amdsmi_get_gpu_accelerator_partition_profile",
        amdsmi_get_gpu_accelerator_partition_profile, processor)
    if current_profile is None:
        print("")
        return

    print("  Current accelerator partition profile:")
    print_accelerator_profile(current_profile['profile'], "    ")
    print("  partition_id values are: {}".format(
        ", ".join(str(partition_id)
                  for partition_id in current_profile['partition_id'])))
    print("")
    return current_profile['profile']['profile_index']


def current_accelerator_profile_index(processor):
    # Same profile lookup as the display path, without printing it. The
    # setter flows use the index only to mark the active profile.
    current_profile = get_optional_api(
        "amdsmi_get_gpu_accelerator_partition_profile",
        amdsmi_get_gpu_accelerator_partition_profile, processor)
    if current_profile is None:
        return None
    return current_profile['profile']['profile_index']


def print_valid_combinations(memory_config, accelerator_config,
                             current_profile_index=None):
    # An accelerator partition profile can only be set while the memory
    # partition mode is one of the modes the profile reports in memory_caps.
    profiles = accelerator_config['profiles']
    if len(profiles) == 0:
        return

    current_mode = memory_config['mp_mode']
    print("  Valid memory partition mode and accelerator partition profile "
          "combinations:")
    for profile in profiles:
        memory_modes = profile['memory_caps']
        # The active profile is marked current even when it is also compatible
        # with the current memory partition mode. Other compatible profiles
        # are marked settable now.
        if (current_profile_index is not None and
                profile['profile_index'] == current_profile_index):
            marker = " <- current"
        elif current_mode in memory_modes:
            marker = " <- settable now"
        else:
            marker = ""
        print("    profile {} ({}, {} partition(s)) valid memory partition "
              "modes: {}{}".format(
                  profile['profile_index'],
                  profile['profile_type'].name,
                  profile['num_partitions'],
                  format_memory_partition_modes(memory_modes),
                  marker))
    print("  Current memory partition mode is {}. A profile that does "
          "not list it requires a memory partition mode change first.".format(
              current_mode.name))
    print("")


def print_partitioning_info(processor):
    # Partitioning support is ASIC-specific. Each query is independent so
    # supported information is still shown when another query is unavailable.
    memory_config = print_memory_partition_config(processor)
    accelerator_config = print_accelerator_partition_profile_config(processor)
    print_global_accelerator_partition_profile_config(processor)
    current_index = print_current_accelerator_partition_profile(processor)
    if memory_config is not None and accelerator_config is not None:
        print_valid_combinations(memory_config, accelerator_config,
                                 current_index)


def print_partitioning_info_for_all_processors(processors):
    # Print the BDF before each result block so output from a multi-GPU system
    # can be associated with the correct physical device.
    for processor_index, processor in enumerate(processors):
        print("###Test amdsmi_get_gpu_device_bdf")
        bdf = amdsmi_get_gpu_device_bdf(processor)
        print("  GPU {} bdf: {}\n".format(processor_index, bdf))
        print_partitioning_info(processor)


def select_processor(processors):
    # Display stable PCI BDFs alongside list indices so the user can identify
    # the physical GPU before selecting a hive-wide operation.
    print("  Available GPUs:")
    for processor_index, processor in enumerate(processors):
        print("  GPU {} bdf: {}".format(
            processor_index, amdsmi_get_gpu_device_bdf(processor)))

    pick_gpu_index = prompt_for_input(
        "  Enter the GPU index to operate on: ",
        0,
        upper_bound=len(processors) - 1,
        error_message="  GPU index must be between 0 and {}".format(
            len(processors) - 1))

    processor = processors[pick_gpu_index]
    # Repeat the BDF after selection to make the operation target explicit.
    print("###Test amdsmi_get_gpu_device_bdf")
    bdf = amdsmi_get_gpu_device_bdf(processor)
    print("  Selected GPU {} bdf: {}\n".format(pick_gpu_index, bdf))
    return processor


def change_memory_partition_mode(processor, requested_mode):
    # The change applies to every GPU in the selected GPU's XGMI hive. All
    # GPUs in the hive must be idle and have no workloads.
    if not prompt_confirmation(
            "  Are you sure you want to set memory partition mode to {}? "
            "This operation applies to all GPUs in the selected GPU's XGMI "
            "hive. All GPUs in the hive must be idle with no workloads.".format(
                requested_mode.name)):
        print("  No operation was performed. Exiting...")
        return False

    print("###Test amdsmi_set_gpu_memory_partition_mode")
    print("  Setting memory partition mode to {}".format(
        requested_mode.name))
    if set_optional_api(
            "amdsmi_set_gpu_memory_partition_mode",
            amdsmi_set_gpu_memory_partition_mode, processor,
            requested_mode) != AmdSmiRetCode.SUCCESS:
        return False
    # A successful return means the request was accepted. Read the current
    # configuration back below so the user can see the resulting state.
    print("  Memory partition mode change request completed")
    print("  Changing the memory partition mode also changes the accelerator "
          "partition profile to the default profile of the new memory "
          "partition mode")

    print_memory_partition_config(processor)
    print_current_accelerator_partition_profile(processor)
    return True


def set_memory_partition_mode(processor):
    # Fetch configs without the verbose dumps from operation 0 (NUMA ranges,
    # resource profiles, full profile listings). Those live under "display";
    # here only the mode/profile combination summary is needed to choose.
    config = get_optional_api(
        "amdsmi_get_gpu_memory_partition_config",
        amdsmi_get_gpu_memory_partition_config, processor)
    if config is None:
        return

    supported_modes = config['partition_caps']
    if len(supported_modes) == 0:
        print("  No supported memory partition modes were reported")
        return

    # Accelerator profiles that can be set after a mode change depend on the
    # memory partition mode; show which profiles each mode allows.
    accelerator_config = get_optional_api(
        "amdsmi_get_gpu_accelerator_partition_profile_config",
        amdsmi_get_gpu_accelerator_partition_profile_config, processor)
    if accelerator_config is not None:
        print_valid_combinations(config, accelerator_config,
                                 current_accelerator_profile_index(processor))

    print("  Available memory partition modes:")
    for mode_index, mode in enumerate(supported_modes):
        print("  {}: {}".format(mode_index, mode.name))

    default_mode_index = (
        supported_modes.index(config['mp_mode'])
        if config['mp_mode'] in supported_modes else 0
    )
    # Default the prompt to the active mode so that choice is suggested.
    # Confirming it still runs the hive-wide setter.
    pick_mode_index = prompt_for_input(
        "  Enter the memory partition mode to set: ",
        default_mode_index,
        upper_bound=len(supported_modes) - 1,
        error_message="  Mode index must be between 0 and {}".format(
            len(supported_modes) - 1))

    change_memory_partition_mode(processor, supported_modes[pick_mode_index])


def print_blocked_profiles(blocked_profiles, current_mode):
    # Keep incompatible profiles visible instead of silently filtering them;
    # their memory_caps explain which prerequisite mode each profile needs.
    print("  These accelerator partition profiles cannot be set while the "
          "memory partition mode is {}:".format(current_mode.name))
    for profile in blocked_profiles:
        print("    profile {} ({}) valid memory partition modes: {}".format(
            profile['profile_index'], profile['profile_type'].name,
            format_memory_partition_modes(profile['memory_caps'])))


def offer_memory_partition_mode_change(processor, memory_config,
                                       blocked_profiles):
    # Lead the user through the memory partition mode change that the blocked
    # profiles need. The profile itself is set on a later run, once the new
    # memory partition mode is in place.
    required_modes = [
        # Preserve the GPU's advertised mode order and include only modes that
        # make at least one currently blocked accelerator profile available.
        mode for mode in memory_config['partition_caps']
        if any(mode in profile['memory_caps']
               for profile in blocked_profiles)
    ]
    if len(required_modes) == 0:
        print("  This GPU does not support any of the memory partition modes "
              "those profiles require")
        return False

    print("  To use one of those profiles, the memory partition mode must be "
          "changed first.")
    if not prompt_confirmation(
            "  Do you want to change the memory partition mode now?"):
        print("  The memory partition mode was not changed")
        return False

    print("  Memory partition modes that make those profiles available:")
    for mode_index, mode in enumerate(required_modes):
        print("  {}: {}".format(mode_index, mode.name))
    pick_mode_index = prompt_for_input(
        "  Enter the memory partition mode to set: ",
        0,
        upper_bound=len(required_modes) - 1,
        error_message="  Mode index must be between 0 and {}".format(
            len(required_modes) - 1))

    if not change_memory_partition_mode(
            processor, required_modes[pick_mode_index]):
        return False

    # Changing memory mode selects that mode's default accelerator profile.
    # Ask the user to rerun the operation before applying a different profile.
    print("  Run operation 2 again to set the accelerator partition profile "
          "that requires this memory partition mode")
    return True


def set_accelerator_partition_profile(processor):
    # Changing an accelerator partition profile can disrupt workloads. Only a
    # profile that lists the current memory partition mode in its memory_caps
    # can be set; the others need a memory partition mode change first.
    memory_config = print_memory_partition_config(processor)
    config = print_accelerator_partition_profile_config(processor)
    if config is None:
        return

    profiles = config['profiles']
    if len(profiles) == 0:
        print("  No accelerator partition profiles were reported")
        return

    blocked_profiles = []
    if memory_config is None:
        # Without the memory partition configuration the valid combinations
        # are unknown, so every reported profile is offered.
        settable_profiles = profiles
    else:
        print_valid_combinations(
            memory_config, config,
            current_accelerator_profile_index(processor))
        current_mode = memory_config['mp_mode']
        # The setter accepts only profiles whose memory_caps contain the
        # currently active memory partition mode.
        settable_profiles = [
            profile for profile in profiles
            if current_mode in profile['memory_caps']
        ]
        blocked_profiles = [
            profile for profile in profiles
            if current_mode not in profile['memory_caps']
        ]

    if len(blocked_profiles) > 0:
        print_blocked_profiles(blocked_profiles, memory_config['mp_mode'])
        if len(settable_profiles) == 0:
            print("  No accelerator partition profile can be set while the "
                  "memory partition mode is {}".format(
                      memory_config['mp_mode'].name))
        if offer_memory_partition_mode_change(
                processor, memory_config, blocked_profiles):
            return
        if len(settable_profiles) == 0:
            print("  No operation was performed. Exiting...")
            return

    profile_indices = [
        profile['profile_index'] for profile in settable_profiles
    ]
    # Profile indices are API-defined identifiers and may not be contiguous,
    # so validate against the advertised values rather than a numeric range.
    print("  Accelerator partition profile indices that can be set now: "
          "{}".format(", ".join(
              str(profile_index) for profile_index in profile_indices)))
    pick_profile_index = prompt_for_input(
        "  Enter the accelerator partition profile index to set: ",
        profile_indices[0],
        valid_values=profile_indices,
        error_message="  Profile index must be one of: {}".format(
            ", ".join(str(profile_index)
                      for profile_index in profile_indices)))

    if not prompt_confirmation(
            "  Are you sure you want to set accelerator partition profile {}? "
            "This operation applies to all GPUs in the selected GPU's XGMI "
            "hive. All GPUs in the hive must be idle with no active VFs.".format(
                pick_profile_index)):
        print("  No operation was performed. Exiting...")
        return

    print("###Test amdsmi_set_gpu_accelerator_partition_profile")
    print("  Setting accelerator partition profile to {}".format(
        pick_profile_index))
    if set_optional_api(
            "amdsmi_set_gpu_accelerator_partition_profile",
            amdsmi_set_gpu_accelerator_partition_profile, processor,
            pick_profile_index) != AmdSmiRetCode.SUCCESS:
        return
    # Read the profile back after the request so its layout and assigned
    # partition identifiers are visible in the example output.
    print("  Accelerator partition profile change request completed")

    print_current_accelerator_partition_profile(processor)


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
            # Information queries are read-only. Both setter operations are
            # hive-wide and therefore call out their idle-device requirements.
            print("  Available operations:")
            print("  0: Display memory and accelerator partitioning information "
                  "for all GPUs")
            print("  1: Set GPU memory partition mode (XGMI hive-wide; all "
                  "GPUs in the hive must be idle.)")
            print("  2: Set accelerator partition profile "
                  "(XGMI hive-wide; all GPUs in the hive must be idle with no "
                  "active VFs.)")
            print("  3: Exit")

            pick_operation = prompt_for_input(
                "  Enter the operation to perform: ",
                0,
                upper_bound=3,
                error_message="  Operation must be between 0 and 3")

            if pick_operation == 3:
                print("  No operation was performed. Exiting...")
            elif pick_operation == 0:
                print_partitioning_info_for_all_processors(processors)
            elif pick_operation == 1:
                set_memory_partition_mode(select_processor(processors))
            else:
                set_accelerator_partition_profile(select_processor(processors))

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
