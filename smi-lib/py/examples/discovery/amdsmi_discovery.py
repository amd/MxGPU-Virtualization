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
    prompt_for_input,
)

def print_node_discovery(processor):
    """Show how to retrieve a node handle for a GPU processor."""
    # Node handles are optional because not every system exposes GPU nodes.
    print("###Test amdsmi_get_node_handle")
    node_handle = get_optional_api(
        "amdsmi_get_node_handle", amdsmi_get_node_handle, processor)
    if node_handle is not None:
        print("  Node handle was retrieved: {}".format(node_handle))
        print("  Pass this handle to node-level APIs, such as "
              "amdsmi_get_npm_info")
    print("")


def print_vf_discovery(processor):
    """Discover the virtual functions (VFs) enabled for a GPU."""
    # The enabled VF count provides valid indices for the VF lookup API.
    print("###Test amdsmi_get_num_vf")
    vf_count = get_optional_api(
        "amdsmi_get_num_vf", amdsmi_get_num_vf, processor)
    if vf_count is None:
        print("")
        return

    print("  Number of enabled VFs is: {}".format(
        vf_count['num_vf_enabled']))
    print("  Number of supported VFs is: {}".format(
        vf_count['num_vf_supported']))

    if vf_count['num_vf_enabled'] == 0:
        print("  No enabled VFs found; VF BDF and UUID lookups require "
              "an enabled VF")
        print("")
        return

    # Demonstrate each supported way to identify and look up an enabled VF:
    # index, BDF, and UUID.
    for vf_index in range(vf_count['num_vf_enabled']):
        try:
            print("###Test amdsmi_get_vf_handle_from_vf_index")
            vf_handle = amdsmi_get_vf_handle_from_vf_index(
                processor, vf_index)
            print("  Retrieved handle for VF {}".format(vf_index))

            print("###Test amdsmi_get_vf_bdf")
            vf_bdf = amdsmi_get_vf_bdf(vf_handle)
            print("  VF {} bdf: {}".format(vf_index, vf_bdf))

            print("###Test amdsmi_get_vf_uuid")
            vf_uuid = amdsmi_get_vf_uuid(vf_handle)
            print("  VF {} UUID: {}".format(vf_index, vf_uuid))

            print("###Test amdsmi_get_vf_handle_from_bdf")
            vf_from_bdf = amdsmi_get_vf_handle_from_bdf(vf_bdf)
            print("  UUID from BDF lookup is: {}".format(
                amdsmi_get_vf_uuid(vf_from_bdf)))

            print("###Test amdsmi_get_vf_handle_from_uuid")
            vf_from_uuid = amdsmi_get_vf_handle_from_uuid(vf_uuid)
            print("  BDF from UUID lookup is: {}".format(
                amdsmi_get_vf_bdf(vf_from_uuid)))
        except AmdSmiLibraryException as e:
            print_optional_api_error(
                "VF {} discovery".format(vf_index), e)
        print("")


def print_gpu_discovery(processor_number, processor):
    """Show GPU identifiers and the APIs that map them back to handles."""
    print("###Test amdsmi_get_processor_type")
    processor_type = amdsmi_get_processor_type(processor)
    print("  Processor number: {} type: {}".format(
        processor_number, processor_type.name))

    print("###Test amdsmi_get_index_from_processor_handle")
    processor_index = amdsmi_get_index_from_processor_handle(processor)
    print("  Processor index is: {}".format(processor_index))

    print("###Test amdsmi_get_gpu_device_bdf")
    gpu_bdf = amdsmi_get_gpu_device_bdf(processor)
    print("  GPU bdf is: {}".format(gpu_bdf))

    # The generic BDF API works for every processor type, while the GPU
    # device BDF API above is GPU-specific.
    print("###Test amdsmi_get_processor_bdf")
    processor_bdf = amdsmi_get_processor_bdf(processor)
    print("  Generic processor bdf is: {}".format(processor_bdf))

    # Look up the same GPU by index and verify its identity using its BDF.
    print("###Test amdsmi_get_processor_handle_from_index")
    processor_from_index = amdsmi_get_processor_handle_from_index(
        processor_index)
    print("  BDF from index lookup is: {}".format(
        amdsmi_get_gpu_device_bdf(processor_from_index)))

    # BDF and UUID lookups provide alternatives when an index is not known.
    print("###Test amdsmi_get_processor_handle_from_bdf")
    processor_from_bdf = amdsmi_get_processor_handle_from_bdf(gpu_bdf)
    print("  Processor index from BDF lookup is: {}".format(
        amdsmi_get_index_from_processor_handle(processor_from_bdf)))

    print("###Test amdsmi_get_gpu_device_uuid")
    gpu_uuid = amdsmi_get_gpu_device_uuid(processor)
    print("  GPU UUID is: {}".format(gpu_uuid))

    print("###Test amdsmi_get_processor_handle_from_uuid")
    processor_from_uuid = amdsmi_get_processor_handle_from_uuid(gpu_uuid)
    print("  Processor index from UUID lookup is: {}".format(
        amdsmi_get_index_from_processor_handle(processor_from_uuid)))
    print("")

    print_node_discovery(processor)
    print_vf_discovery(processor)


def print_nic_discovery():
    """Discover optional NIC processors and demonstrate NIC lookups."""
    # NIC support is optional, so an unavailable API is not treated as an
    # error that should stop GPU discovery.
    print("###Test amdsmi_get_nic_processor_handles")
    nic_processors = get_optional_api(
        "amdsmi_get_nic_processor_handles", amdsmi_get_nic_processor_handles)
    if nic_processors is None:
        print("")
        return

    print("  number of NICs is: {}".format(len(nic_processors)))

    # The combined NIC list can also be filtered by vendor-specific type.
    for nic_type in (AmdSmiProcessorType.AMD_NIC,
                     AmdSmiProcessorType.BRCM_NIC):
        print("###Test amdsmi_get_processor_handles_by_type")
        typed_nics = get_optional_api(
            "amdsmi_get_processor_handles_by_type({})".format(nic_type.name),
            amdsmi_get_processor_handles_by_type, nic_type)
        if typed_nics is not None:
            print("  number of {} processors is: {}".format(
                nic_type.name, len(typed_nics)))
    print("")

    if len(nic_processors) == 0:
        print("  No NICs found\n")
        return

    print("  Available NICs:")
    for i in range(len(nic_processors)):
        print("  NIC {} bdf: {}".format(
            i, amdsmi_get_nic_device_bdf(nic_processors[i])))
    all_nic_index = len(nic_processors)
    print("  {}: All NICs".format(all_nic_index))

    # The index immediately after the last NIC is used as the "All NICs"
    # menu choice.
    pick_nic_index = prompt_for_input(
        "  Enter the NIC index to operate on: ",
        0,
        upper_bound=all_nic_index,
        error_message=(
            "  NIC index must be between 0 and {} ({} for all NICs)".format(
                len(nic_processors) - 1, all_nic_index)
        ))

    if pick_nic_index == all_nic_index:
        selected_nic_indexes = range(len(nic_processors))
    else:
        selected_nic_indexes = [pick_nic_index]

    for selected_nic_index in selected_nic_indexes:
        nic = nic_processors[selected_nic_index]
        print("###Test amdsmi_get_nic_device_bdf")
        nic_bdf = amdsmi_get_nic_device_bdf(nic)
        print("  Selected NIC {} bdf: {}\n".format(
            selected_nic_index, nic_bdf))

        print("###Test amdsmi_get_processor_type")
        nic_type = amdsmi_get_processor_type(nic)
        print("  NIC number: {} type: {}".format(
            selected_nic_index, nic_type.name))

        print("###Test amdsmi_get_index_from_processor_handle")
        nic_index = amdsmi_get_index_from_processor_handle(nic)
        print("  NIC index is: {}".format(nic_index))

        print("###Test amdsmi_get_nic_device_bdf")
        print("  NIC bdf is: {}".format(nic_bdf))

        # The generic processor API returns the same BDF without requiring
        # the caller to know that this handle represents a NIC.
        print("###Test amdsmi_get_processor_bdf")
        processor_bdf = amdsmi_get_processor_bdf(nic)
        print("  Generic processor bdf is: {}".format(processor_bdf))

        # amdsmi_get_processor_handle_from_index accepts GPU indexes only.
        # Use BDF to look up a NIC without needing a type-specific index API.
        print("###Test amdsmi_get_processor_handle_from_bdf")
        nic_from_bdf = amdsmi_get_processor_handle_from_bdf(nic_bdf)
        print("  Processor type from BDF lookup is: {}".format(
            amdsmi_get_processor_type(nic_from_bdf).name))
        print("")


if __name__ == "__main__":
    initialized = False

    try:
        # INIT_ALL_PROCESSORS is the default. It is needed to discover both
        # GPUs and optional NICs.
        init_flags = AmdSmiInitFlags.INIT_ALL_PROCESSORS
        print("###Test amdsmi_init")
        print("  Initializing with {}".format(init_flags.name))
        amdsmi_init(init_flags)
        initialized = True
        print("  Initialization complete")

        # Start with the standard processor discovery API, then demonstrate
        # how to request only processors of a specific type.
        print("###Test amdsmi_get_processor_handles")
        processors = amdsmi_get_processor_handles()
        print("  number of GPUs is: {}\n".format(len(processors)))

        print("###Test amdsmi_get_processor_handles_by_type")
        gpu_processors = amdsmi_get_processor_handles_by_type(
            AmdSmiProcessorType.AMD_GPU)
        print("  number of AMD_GPU processors is: {}\n".format(
            len(gpu_processors)))

        if len(processors) == 0:
            print("  No GPUs found\n")
        else:
            print("  Available GPUs:")
            for i in range(len(processors)):
                print("  GPU {} bdf: {}".format(
                    i, amdsmi_get_gpu_device_bdf(processors[i])))
            all_gpu_index = len(processors)
            print("  {}: All GPUs".format(all_gpu_index))

            # The index immediately after the last GPU is used as the
            # "All GPUs" menu choice.
            pick_gpu_index = prompt_for_input(
                "  Enter the GPU index to operate on: ",
                0,
                upper_bound=all_gpu_index,
                error_message=(
                    "  GPU index must be between 0 and {} "
                    "({} for all GPUs)".format(
                        len(processors) - 1, all_gpu_index)
                ))

            if pick_gpu_index == all_gpu_index:
                selected_gpu_indexes = range(len(processors))
            else:
                selected_gpu_indexes = [pick_gpu_index]

            for gpu_index in selected_gpu_indexes:
                processor = processors[gpu_index]
                print("###Test amdsmi_get_gpu_device_bdf")
                bdf = amdsmi_get_gpu_device_bdf(processor)
                print("  Selected GPU {} bdf: {}\n".format(gpu_index, bdf))
                print_gpu_discovery(gpu_index, processor)

        print_nic_discovery()

    except AmdSmiLibraryException as e:
        status = AmdSmiRetCode(e.err_code)
        print(amdsmi_status_code_to_string(status))
    except AmdSmiException as e:
        print(e)
    except KeyboardInterrupt:
        print("\n  Keyboard interrupt detected. Exiting...")
    finally:
        # Always release AMD SMI resources, including after an API error
        # or keyboard interrupt.
        if initialized:
            try:
                print("###Test amdsmi_shut_down")
                amdsmi_shut_down()
                print("Shutdown complete")
            except AmdSmiException as e:
                print(e)
