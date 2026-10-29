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
    print_optional_api_error,
    prompt_confirmation,
    prompt_for_input,
    set_optional_api,
)


def print_power_cap_info(processor, sensor_ind, sensor_type):
    # A GPU can expose multiple independently controlled power-cap sensors.
    # Use the sensor index returned by amdsmi_get_supported_power_cap rather
    # than assuming that sensor indexes are contiguous.
    # Power-cap units are platform-specific. Keep and reuse the raw values
    # returned by the API when setting a cap.
    print("###Test amdsmi_get_power_cap_info")
    power_info = get_optional_api(
        "amdsmi_get_power_cap_info", amdsmi_get_power_cap_info, processor,
        sensor_ind)
    if power_info is None:
        print("")
        return None

    print("  Sensor {} ({}) power cap information:".format(
        sensor_ind, sensor_type.name))
    print("    power_cap is: {}".format(power_info['power_cap']))
    print("    default_power_cap is: {}".format(
        power_info['default_power_cap']))
    print("    dpm_cap is: {}".format(power_info['dpm_cap']))
    print("    min_power_cap is: {}".format(
        power_info['min_power_cap']))
    print("    max_power_cap is: {}".format(
        power_info['max_power_cap']))
    print("")
    return power_info


def print_power_cap_infos(processor):
    # Query the supported sensors first because power-cap availability and
    # sensor type vary by GPU and platform.
    print("###Test amdsmi_get_supported_power_cap")
    supported_caps = get_optional_api(
        "amdsmi_get_supported_power_cap", amdsmi_get_supported_power_cap,
        processor)
    if supported_caps is None:
        print("")
        return {}

    sensor_inds = supported_caps['sensor_inds']
    sensor_types = supported_caps['sensor_types']
    print("  number of supported power cap sensors is: {}".format(
        len(sensor_inds)))
    if len(sensor_inds) == 0:
        print("  No supported power cap sensors found")
        print("")
        return {}

    power_caps = {}
    # Save each successful query for use by the interactive set operation.
    # Unsupported sensors are reported by get_optional_api and omitted.
    for sensor_ind, sensor_type in zip(sensor_inds, sensor_types):
        power_info = print_power_cap_info(processor, sensor_ind, sensor_type)
        if power_info is not None:
            power_caps[sensor_ind] = (sensor_type, power_info)
    return power_caps


def print_power_management_state(processor):
    # Power caps can be changed only while GPU power management is enabled.
    print("###Test amdsmi_is_gpu_power_management_enabled")
    enabled = get_optional_api(
        "amdsmi_is_gpu_power_management_enabled",
        amdsmi_is_gpu_power_management_enabled, processor)
    if enabled is not None:
        print("  GPU power management is: {}".format(
            "enabled" if enabled else "disabled"))
    print("")
    return enabled


def print_soc_pstate(processor):
    # The current policy is represented by an index into the returned policy
    # list; policy_id is the value passed to amdsmi_set_soc_pstate.
    print("###Test amdsmi_get_soc_pstate")
    pstate = get_optional_api(
        "amdsmi_get_soc_pstate", amdsmi_get_soc_pstate, processor)
    if pstate is not None:
        print("  Current SoC P-state policy index is: {}".format(
            pstate['cur']))
        if len(pstate['policies']) == 0:
            print("  No SoC P-state policies found")
        for policy_index, policy in enumerate(pstate['policies']):
            current = " (current)" if policy_index == pstate['cur'] else ""
            print("  Policy index {} (ID {}) is: {}{}".format(
                policy_index, policy['policy_id'],
                policy['policy_description'], current))
    print("")
    return pstate


def display_power_performance_control(processor):
    # Read-only summary of every power/performance control demonstrated by
    # this example. Optional APIs may be unavailable on some platforms.
    print_power_cap_infos(processor)
    print_power_management_state(processor)
    print_soc_pstate(processor)


def display_all_power_performance_controls(processors):
    # Display the BDF before each summary so output can be matched to the
    # physical GPU when more than one processor handle is present.
    for processor_number, processor in enumerate(processors):
        print("###Test amdsmi_get_gpu_device_bdf")
        bdf = amdsmi_get_gpu_device_bdf(processor)
        print("  GPU {} bdf: {}\n".format(processor_number, bdf))
        display_power_performance_control(processor)


def set_and_restore_power_cap(processor):
    # Read the current values before making a change. The original cap is
    # restored in the finally block, even if the verification query fails.
    power_caps = print_power_cap_infos(processor)
    if len(power_caps) == 0:
        return

    # Reject the operation early when the platform reports that power
    # management is disabled or the state cannot be queried.
    power_management_enabled = print_power_management_state(processor)
    if power_management_enabled is not True:
        print("  Cannot set a power cap while power management is disabled "
              "or unavailable.\n")
        return

    sensor_items = list(power_caps.items())
    # Present a compact, zero-based menu while preserving the real sensor
    # index that must be supplied to the AMD SMI API.
    print("  Available power cap sensors:")
    for sensor_number, (sensor_ind, sensor_data) in enumerate(sensor_items):
        sensor_type, power_info = sensor_data
        print("  {}: Sensor {} ({}) current={} minimum={} maximum={}".format(
            sensor_number, sensor_ind, sensor_type.name,
            power_info['power_cap'], power_info['min_power_cap'],
            power_info['max_power_cap']))

    sensor_number = prompt_for_input(
        "  Enter the power cap sensor number to operate on: ",
        0,
        upper_bound=len(sensor_items) - 1,
        error_message=(
            "  Power cap sensor index must be between 0 and {}".format(
                len(sensor_items) - 1)
        ))
    sensor_ind, sensor_data = sensor_items[sensor_number]
    sensor_type, power_info = sensor_data
    original_power_cap = power_info['power_cap']

    # Equal minimum and maximum values indicate a fixed, non-adjustable cap.
    if power_info['min_power_cap'] == power_info['max_power_cap']:
        print("  Sensor {} has no adjustable power cap range.\n".format(
            sensor_ind))
        return

    requested_power_cap = prompt_for_input(
        "  Enter the power cap to set ({} to {}): ".format(
            power_info['min_power_cap'], power_info['max_power_cap']),
        original_power_cap,
        lower_bound=power_info['min_power_cap'],
        upper_bound=power_info['max_power_cap'],
        error_message="  Power cap must be between {} and {}".format(
            power_info['min_power_cap'], power_info['max_power_cap']))
    if requested_power_cap == original_power_cap:
        print("  The requested power cap is already active. "
              "No operation was performed.\n")
        return

    if not prompt_confirmation(
            "  Set {} on sensor {} from {} to {}? The script will restore "
            "the original value.".format(
                sensor_type.name, sensor_ind, original_power_cap,
                requested_power_cap)):
        print("  No operation was performed.\n")
        return

    # Mark the operation before calling the setter because an exception does
    # not guarantee that the device rejected the requested value.
    set_attempted = False
    try:
        print("###Test amdsmi_set_power_cap")
        print("  Setting sensor {} power cap to {}".format(
            sensor_ind, requested_power_cap))
        set_attempted = True
        amdsmi_set_power_cap(processor, sensor_ind, requested_power_cap)
        print("  Power cap change request completed\n")
        print_power_cap_info(processor, sensor_ind, sensor_type)
    except AmdSmiLibraryException as e:
        print_optional_api_error("amdsmi_set_power_cap", e)
        print("")
    finally:
        # Always attempt to return the GPU to the state observed at entry.
        if set_attempted:
            print("###Test amdsmi_set_power_cap")
            print("  Restoring sensor {} power cap to {}".format(
                sensor_ind, original_power_cap))
            if set_optional_api(
                    "amdsmi_set_power_cap while restoring",
                    amdsmi_set_power_cap, processor, sensor_ind,
                    original_power_cap) == AmdSmiRetCode.SUCCESS:
                print("  Original power cap restored\n")
                print_power_cap_info(processor, sensor_ind, sensor_type)
            else:
                print("")


def set_and_restore_soc_pstate(processor):
    # Capture the current policy and supported IDs before asking the user to
    # make a temporary change.
    pstate = print_soc_pstate(processor)
    if pstate is None or len(pstate['policies']) == 0:
        return

    current_policy_index = pstate['cur']
    # An out-of-range index means there is no trustworthy original policy to
    # restore, so changing the policy would be unsafe for this example.
    if (current_policy_index < 0
            or current_policy_index >= len(pstate['policies'])):
        print("  The current policy index does not identify a supported "
              "policy; the sample cannot safely restore it.\n")
        return

    original_policy_id = pstate['policies'][current_policy_index]['policy_id']
    policy_ids = [policy['policy_id'] for policy in pstate['policies']]

    requested_policy_id = prompt_for_input(
        "  Enter the SoC P-state policy ID to set: ",
        original_policy_id,
        valid_values=policy_ids,
        error_message="  Policy ID must be one of: {}".format(policy_ids))

    if requested_policy_id == original_policy_id:
        print("  The requested policy is already active. "
              "No operation was performed.\n")
        return

    if not prompt_confirmation(
            "  Set the SoC P-state policy from {} to {}? The script will "
            "restore the original policy.".format(
                original_policy_id, requested_policy_id)):
        print("  No operation was performed.\n")
        return

    # As with power caps, attempt restoration whenever the setter was called,
    # including when that call raises an exception.
    set_attempted = False
    try:
        print("###Test amdsmi_set_soc_pstate")
        print("  Setting SoC P-state policy to {}".format(
            requested_policy_id))
        set_attempted = True
        amdsmi_set_soc_pstate(processor, requested_policy_id)
        print("  SoC P-state policy change request completed\n")
        print_soc_pstate(processor)
    except AmdSmiLibraryException as e:
        print_optional_api_error("amdsmi_set_soc_pstate", e)
        print("")
    finally:
        if set_attempted:
            print("###Test amdsmi_set_soc_pstate")
            print("  Restoring SoC P-state policy to {}".format(
                original_policy_id))
            if set_optional_api(
                    "amdsmi_set_soc_pstate while restoring",
                    amdsmi_set_soc_pstate, processor,
                    original_policy_id) == AmdSmiRetCode.SUCCESS:
                print("  Original SoC P-state policy restored\n")
                print_soc_pstate(processor)
            else:
                print("")


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
            print("  No GPUs found")
        else:
            # Read-only display applies to every GPU. Setter examples operate
            # on one explicitly selected GPU and restore its original state.
            print("  Available operations:")
            print("  0: Display power and performance control information")
            print("  1: Set and restore a power cap")
            print("  2: Set and restore a SoC P-state policy")
            print("  3: Exit")

            operation_number = prompt_for_input(
                "  Enter the operation to perform: ",
                0,
                upper_bound=3,
                error_message="  Operation index must be between 0 and 3")
            if operation_number == 0:
                display_all_power_performance_controls(processors)
            elif operation_number == 3:
                print("  No operation was performed.")
            else:
                # Display each GPU's BDF so the user can identify the physical
                # device before selecting a power or performance operation.
                print("  Available GPUs:")
                for processor_number, processor in enumerate(processors):
                    print("  GPU {} bdf: {}".format(
                        processor_number,
                        amdsmi_get_gpu_device_bdf(processor)))

                processor_number = prompt_for_input(
                    "  Enter the GPU index to operate on: ",
                    0,
                    upper_bound=len(processors) - 1,
                    error_message="  GPU index must be between 0 and {}".format(
                        len(processors) - 1))
                processor = processors[processor_number]
                print("###Test amdsmi_get_gpu_device_bdf")
                bdf = amdsmi_get_gpu_device_bdf(processor)
                print("  Selected GPU {} bdf: {}\n".format(
                    processor_number, bdf))

                if operation_number == 1:
                    set_and_restore_power_cap(processor)
                else:
                    set_and_restore_soc_pstate(processor)

    except AmdSmiLibraryException as e:
        status = AmdSmiRetCode(e.err_code)
        print("  {} is: {}".format(
            status.name, amdsmi_status_code_to_string(status)))
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
