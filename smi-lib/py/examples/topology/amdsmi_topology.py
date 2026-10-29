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


def print_link_metrics(processor, processor_number):
    """Print per-link metrics (type, status, speed, traffic) for one GPU."""
    print("###Test amdsmi_get_link_metrics")
    # Link metrics are optional, so an unsupported system prints a message
    # instead of stopping the example.
    link_metrics = get_optional_api(
        "amdsmi_get_link_metrics", amdsmi_get_link_metrics, processor)
    if link_metrics is not None:
        links = link_metrics['links']
        print("  GPU {} has {} link(s)".format(processor_number, len(links)))
        if len(links) == 0:
            print("  No link metrics found")

        # Each entry describes one link from this GPU to a peer, identified
        # by the peer's BDF. read and write are cumulative traffic counters.
        for link_number, link in enumerate(links):
            print("  Link {}:".format(link_number))
            print("    bdf is: {}".format(link['bdf']))
            print("    link_type is: {}".format(link['link_type']))
            print("    link_status is: {}".format(link['link_status']))
            print("    bit_rate is: {} Gb/s".format(link['bit_rate']))
            print("    max_bandwidth is: {}".format(link['max_bandwidth']))
            print("    read is: {} KB".format(link['read']))
            print("    write is: {} KB".format(link['write']))
    print("")


def print_nearest_processors(processor):
    """Print the GPUs closest to this GPU over XGMI and over PCIe."""
    print("###Test amdsmi_get_link_topology_nearest")
    # The link type selects which interconnect to search. The result is
    # the GPUs of that type that share the smallest hop count.
    # XGMI peers can be more than one hop away.
    # The PCIe result is only the closest PCIe peers.
    for link_type in (AmdSmiLinkType.XGMI, AmdSmiLinkType.PCIE):
        try:
            nearest = amdsmi_get_link_topology_nearest(processor, link_type)
            # The API returns processor handles. Convert them to BDFs so the
            # output identifies physical devices.
            nearest_processors = nearest['processor_list']
            nearest_bdfs = [
                amdsmi_get_gpu_device_bdf(nearest_processor)
                for nearest_processor in nearest_processors
            ]
            print("  Nearest {} GPU BDFs are: {}".format(
                link_type.name,
                nearest_bdfs if len(nearest_bdfs) > 0 else "none"))
        except AmdSmiLibraryException as e:
            # Report the failure for this link type and continue with the
            # next one.
            print_optional_api_error(
                "amdsmi_get_link_topology_nearest for {}".format(
                    link_type.name), e)
    print("")


def print_host_affinity(processor):
    """Print the NUMA node and host CPU cores closest to this GPU."""
    # The NUMA node identifies the host memory domain the GPU is attached
    # to. Placing work on CPUs and memory in the same node avoids
    # cross-node traffic.
    print("###Test amdsmi_topo_get_numa_node_number")
    numa_node = get_optional_api(
        "amdsmi_topo_get_numa_node_number", amdsmi_topo_get_numa_node_number,
        processor)
    if numa_node is not None:
        print("  NUMA node is: {}".format(numa_node))
    print("")

    # CPU affinity is a list of bitmask words.
    # Each set bit is a logical CPU from the GPU's NUMA-node local_cpu_list.
    print("###Test amdsmi_get_cpu_affinity_with_scope")
    for scope in (AmdSmiAffinityScope.NUMA_SCOPE,
                  AmdSmiAffinityScope.SOCKET_SCOPE):
        affinity = get_optional_api(
            "amdsmi_get_cpu_affinity_with_scope for {}".format(scope.name),
            amdsmi_get_cpu_affinity_with_scope, processor, scope)
        if affinity is not None:
            print("  {} CPU affinity bitmask is: {}".format(
                scope.name, [hex(word) for word in affinity]))
    print("")


def get_xgmi_modes(caps):
    """Return the XGMI framebuffer sharing modes reported as supported."""
    # Map each capability flag to the mode enum it describes. CUSTOM is left
    # out because it cannot be queried by amdsmi_get_xgmi_fb_sharing_mode_info.
    mode_caps = (
        ('mode_1_cap', AmdSmiXgmiFbSharingMode.MODE_1),
        ('mode_2_cap', AmdSmiXgmiFbSharingMode.MODE_2),
        ('mode_4_cap', AmdSmiXgmiFbSharingMode.MODE_4),
        ('mode_8_cap', AmdSmiXgmiFbSharingMode.MODE_8),
    )
    return [mode for cap_name, mode in mode_caps if caps[cap_name]]


def print_xgmi_fb_sharing_caps(processor):
    """Print XGMI framebuffer sharing capabilities and return supported modes."""
    # XGMI framebuffer sharing lets GPUs connected over XGMI access each
    # other's framebuffer memory. Each capability flag reports whether the
    # GPU supports the corresponding sharing mode.
    print("###Test amdsmi_get_xgmi_fb_sharing_caps")
    caps = get_optional_api(
        "amdsmi_get_xgmi_fb_sharing_caps", amdsmi_get_xgmi_fb_sharing_caps,
        processor)
    if caps is None:
        print("")
        # An empty list tells callers that there are no modes to query.
        return []

    print("  mode_custom_cap is: {}".format(caps['mode_custom_cap']))
    print("  mode_1_cap is: {}".format(caps['mode_1_cap']))
    print("  mode_2_cap is: {}".format(caps['mode_2_cap']))
    print("  mode_4_cap is: {}".format(caps['mode_4_cap']))
    print("  mode_8_cap is: {}".format(caps['mode_8_cap']))
    print("")
    # Return the supported modes so the pair-topology section only queries
    # sharing state for modes this GPU supports.
    return get_xgmi_modes(caps)


def print_xgmi_plpd(processor):
    """Print the XGMI per-link power down (PLPD) policies for a GPU."""
    # PLPD controls when individual XGMI links may be powered down to save
    # power. The API returns the list of available policies and the index of
    # the policy that is currently active.
    # Every GPU on the same socket reports the same policy.
    print("###Test amdsmi_get_xgmi_plpd")
    plpd = get_optional_api(
        "amdsmi_get_xgmi_plpd", amdsmi_get_xgmi_plpd, processor)
    if plpd is not None:
        # 'cur' is an index into the policies list, not a policy ID.
        print("  Current XGMI PLPD policy index is: {}".format(plpd['cur']))
        if len(plpd['policies']) == 0:
            print("  No XGMI PLPD policies found")
        for policy_index, policy in enumerate(plpd['policies']):
            current = " (current)" if policy_index == plpd['cur'] else ""
            print("  Policy index {} (ID {}) is: {}{}".format(
                policy_index, policy['policy_id'],
                policy['policy_description'], current))
    print("")
    # Return the result so set_xgmi_plpd() can offer the same policy list.
    return plpd


def prompt_gpu_indexes(processors, include_all):
    """Ask the user to pick one GPU, or all GPUs when include_all is True."""
    # Display each GPU's BDF so the user can identify the physical device
    # before choosing it.
    print("  Available GPUs:")
    for gpu_index, processor in enumerate(processors):
        print("  GPU {} bdf: {}".format(
            gpu_index, amdsmi_get_gpu_device_bdf(processor)))

    gpu_upper_bound = len(processors) - 1
    error_message = "  GPU index must be between 0 and {}".format(
        len(processors) - 1)
    if include_all:
        # The index immediately after the last GPU is used as the
        # "All GPUs" menu choice.
        gpu_upper_bound = len(processors)
        print("  {}: All GPUs".format(gpu_upper_bound))
        error_message = (
            "  GPU index must be between 0 and {} "
            "({} for all GPUs)".format(len(processors) - 1, gpu_upper_bound)
        )

    pick_gpu_index = prompt_for_input(
        "  Enter the GPU index to operate on: ",
        0,
        upper_bound=gpu_upper_bound,
        error_message=error_message)

    # Always return a list so callers handle one GPU and all GPUs the
    # same way.
    if include_all and pick_gpu_index == gpu_upper_bound:
        return list(range(len(processors)))
    return [pick_gpu_index]


def print_gpu_topology_info(processors, selected_gpu_indexes):
    """Print per-GPU topology information for each selected GPU."""
    selected = set(selected_gpu_indexes)
    # Supported XGMI sharing modes, keyed by GPU index. They are collected
    # here and reused later by print_pair_topology().
    supported_xgmi_modes = {}
    for processor_number, processor in enumerate(processors):
        if processor_number not in selected:
            continue

        # Print the BDF so the following output can be associated with a
        # physical GPU.
        print("###Test amdsmi_get_gpu_device_bdf")
        bdf = amdsmi_get_gpu_device_bdf(processor)
        print("  Processor number: {} bdf: {}\n".format(
            processor_number, bdf))

        print_link_metrics(processor, processor_number)
        print_nearest_processors(processor)
        print_host_affinity(processor)
        supported_xgmi_modes[processor_number] = \
            print_xgmi_fb_sharing_caps(processor)
        print_xgmi_plpd(processor)
    return supported_xgmi_modes


def get_topology_cell(processor_src, processor_dst):
    """Return a short summary of the link between two GPUs for the matrix."""
    try:
        # Each cell is formatted as type:status/hops:fb=on|off,
        # for example XGMI:UP/1hop:fb=on.
        topology = amdsmi_get_link_topology(processor_src, processor_dst)
        return "{}:{}/{}hop:fb={}".format(
            topology['link_type'].name,
            topology['link_status'].name,
            topology['num_hops'],
            "on" if topology['fb_sharing'] else "off")
    except AmdSmiLibraryException:
        # If a pair cannot be queried, mark the cell instead of aborting the
        # whole matrix.
        return "unavailable"


def print_link_topology_matrix(processors, selected_gpu_indexes):
    """Print an NxN matrix of link topology between GPUs."""
    selected = set(selected_gpu_indexes)
    print("###Test amdsmi_get_link_topology")
    print("  NxN link matrix (type:status/hops:framebuffer-sharing):")
    # Columns always include every GPU, including the source.
    # The diagonal cell is that GPU queried against itself.
    print("  Source\\Destination: {}".format(
        " | ".join("GPU {}".format(i) for i in range(len(processors)))))

    # Rows are printed only for the GPUs the user selected.
    for source_number, source_processor in enumerate(processors):
        if source_number not in selected:
            continue
        cells = [
            get_topology_cell(source_processor, destination_processor)
            for destination_processor in processors
        ]
        print("  GPU {}: {}".format(source_number, " | ".join(cells)))
    print("")


def print_pair_topology(processors, supported_xgmi_modes, selected_gpu_indexes):
    """Print link type, P2P capabilities, and XGMI sharing for GPU pairs."""
    # Every query in this section needs a source and a different
    # destination GPU.
    if len(processors) < 2:
        print("  GPU-to-GPU topology queries require at least two GPUs.\n")
        return

    selected = set(selected_gpu_indexes)
    # Link type reports how two GPUs are connected (for example XGMI or
    # PCIe) and how many hops separate them.
    print("###Test amdsmi_topo_get_link_type")
    for source_number, source_processor in enumerate(processors):
        if source_number not in selected:
            continue
        for destination_number, destination_processor in enumerate(processors):
            # Skip querying a GPU against itself.
            if source_number == destination_number:
                continue
            link_info = get_optional_api(
                "amdsmi_topo_get_link_type from GPU {} to GPU {}".format(
                    source_number, destination_number),
                amdsmi_topo_get_link_type, source_processor,
                destination_processor)
            if link_info is not None:
                print("  GPU {} to GPU {}: type={} hops={}".format(
                    source_number, destination_number,
                    link_info['type'].name, link_info['hops']))
    print("")

    # P2P status reports the link type and the IO-link capabilities:
    # coherency, 32-bit and 64-bit atomics, DMA, and whether the link is
    # bidirectional.
    print("###Test amdsmi_topo_get_p2p_status")
    for source_number, source_processor in enumerate(processors):
        if source_number not in selected:
            continue
        for destination_number, destination_processor in enumerate(processors):
            if source_number == destination_number:
                continue
            p2p_status = get_optional_api(
                "amdsmi_topo_get_p2p_status from GPU {} to GPU {}".format(
                    source_number, destination_number),
                amdsmi_topo_get_p2p_status, source_processor,
                destination_processor)
            if p2p_status is not None:
                caps = p2p_status['cap']
                print("  GPU {} to GPU {}: type={}".format(
                    source_number, destination_number,
                    p2p_status['type'].name))
                print("    coherent={} atomics_32bit={} atomics_64bit={} "
                      "dma={} bi_directional={}".format(
                          caps['is_iolink_coherent'],
                          caps['is_iolink_atomics_32bit'],
                          caps['is_iolink_atomics_64bit'],
                          caps['is_iolink_dma'],
                          caps['is_iolink_bi_directional']))
    print("")

    # For each supported sharing mode, report whether the source GPU shares
    # its framebuffer with the destination GPU in that mode.
    print("###Test amdsmi_get_xgmi_fb_sharing_mode_info")
    for source_number, source_processor in enumerate(processors):
        if source_number not in selected:
            continue
        # Only query modes the source GPU reported as supported in
        # print_xgmi_fb_sharing_caps().
        modes = supported_xgmi_modes[source_number]
        if len(modes) == 0:
            print("  GPU {} reports no queryable XGMI framebuffer sharing "
                  "modes".format(source_number))
            continue

        for destination_number, destination_processor in enumerate(processors):
            if source_number == destination_number:
                continue
            for mode in modes:
                fb_sharing = get_optional_api(
                    "amdsmi_get_xgmi_fb_sharing_mode_info from GPU {} "
                    "to GPU {} with {}".format(
                        source_number, destination_number, mode.name),
                    amdsmi_get_xgmi_fb_sharing_mode_info, source_processor,
                    destination_processor, mode)
                if fb_sharing is not None:
                    print("  GPU {} to GPU {} with {}: {}".format(
                        source_number, destination_number, mode.name,
                        "enabled" if fb_sharing else "disabled"))
    print("")


def print_nic_topology(processors):
    """Print each NIC's BDF, NUMA placement, and link type to every GPU."""
    # NIC discovery and topology are optional. A lack of NIC support does not
    # prevent the GPU-only topology section from being useful.
    print("###Test amdsmi_get_nic_processor_handles")
    nic_processors = get_optional_api(
        "amdsmi_get_nic_processor_handles", amdsmi_get_nic_processor_handles)
    if nic_processors is None:
        print("")
        return

    print("  number of NICs is: {}".format(len(nic_processors)))
    if len(nic_processors) == 0:
        print("  No NICs found\n")
        return

    for nic_number, nic_processor in enumerate(nic_processors):
        # Print the BDF so users can associate the output with a physical
        # NIC in the system.
        print("###Test amdsmi_get_nic_device_bdf")
        bdf = get_optional_api(
            "amdsmi_get_nic_device_bdf", amdsmi_get_nic_device_bdf,
            nic_processor)
        if bdf is not None:
            print("  NIC number: {} bdf: {}".format(nic_number, bdf))

        # NUMA info gives the NIC's NUMA node and the mask of CPU cores that
        # are local to it.
        print("###Test amdsmi_get_nic_numa_info")
        numa_info = get_optional_api(
            "amdsmi_get_nic_numa_info", amdsmi_get_nic_numa_info,
            nic_processor)
        if numa_info is not None:
            print("  node is: {}".format(numa_info['node']))
            print("  affinity is: {}".format(numa_info['affinity']))

        # For NIC-to-GPU queries the link type is PCIE (same PCIe switch),
        # NUMA (same NUMA node, different switch), XNUMA (different NUMA
        # nodes), or UNKNOWN. The hops value is not meaningful for NICs and
        # is reported as UINT64_MAX.
        print("###Test amdsmi_topo_get_link_type")
        for gpu_number, gpu_processor in enumerate(processors):
            link_info = get_optional_api(
                "amdsmi_topo_get_link_type from NIC {} to GPU {}".format(
                    nic_number, gpu_number),
                amdsmi_topo_get_link_type, nic_processor, gpu_processor)
            if link_info is not None:
                print("  NIC {} to GPU {}: type={} hops={}".format(
                    nic_number, gpu_number, link_info['type'].name,
                    link_info['hops']))
        print("")


def display_topology(processors, selected_gpu_indexes):
    """Run every read-only topology query for the selected GPUs."""
    # Per-GPU information comes first because it also collects the
    # supported XGMI sharing modes used by the pair-topology section.
    supported_xgmi_modes = print_gpu_topology_info(
        processors, selected_gpu_indexes)
    print_link_topology_matrix(processors, selected_gpu_indexes)
    print_pair_topology(processors, supported_xgmi_modes, selected_gpu_indexes)
    print_nic_topology(processors)


def prompt_xgmi_mode():
    """Ask the user to pick an XGMI framebuffer sharing mode."""
    # Offer every mode defined by the enum. Use the capabilities printed by
    # operation 0 to see which modes the GPU supports.
    modes = list(AmdSmiXgmiFbSharingMode)
    print("  Available XGMI framebuffer sharing modes:")
    for mode_number, mode in enumerate(modes):
        print("  {}: {}".format(mode_number, mode.name))
    mode_number = prompt_for_input(
        "  Enter the XGMI framebuffer sharing mode: ",
        0,
        upper_bound=len(modes) - 1,
        error_message="  Mode index must be between 0 and {}".format(
            len(modes) - 1))
    return modes[mode_number]


def print_xgmi_sharing_verification(processors, source_number, mode):
    """Read back XGMI sharing state from one GPU to every other GPU."""
    source_processor = processors[source_number]
    # Sharing mode info can only be queried for the fixed MODE_X values,
    # so skip this step for CUSTOM.
    if mode != AmdSmiXgmiFbSharingMode.CUSTOM:
        print("###Test amdsmi_get_xgmi_fb_sharing_mode_info")
        for destination_number, destination_processor in enumerate(
                processors):
            if source_number == destination_number:
                continue
            fb_sharing = get_optional_api(
                "amdsmi_get_xgmi_fb_sharing_mode_info from GPU {} to "
                "GPU {}".format(source_number, destination_number),
                amdsmi_get_xgmi_fb_sharing_mode_info, source_processor,
                destination_processor, mode)
            if fb_sharing is not None:
                print("  GPU {} to GPU {} with {}: {}".format(
                    source_number, destination_number, mode.name,
                    "enabled" if fb_sharing else "disabled"))

    # Link topology includes an fb_sharing field, which shows the effective
    # sharing state for every mode, including CUSTOM.
    print("###Test amdsmi_get_link_topology")
    for destination_number, destination_processor in enumerate(processors):
        if source_number == destination_number:
            continue
        topology = get_optional_api(
            "amdsmi_get_link_topology from GPU {} to GPU {}".format(
                source_number, destination_number),
            amdsmi_get_link_topology, source_processor,
            destination_processor)
        if topology is not None:
            print("  GPU {} to GPU {}: link_type={} link_status={} "
                  "num_hops={} fb_sharing={}".format(
                      source_number, destination_number,
                      topology['link_type'].name,
                      topology['link_status'].name,
                      topology['num_hops'], topology['fb_sharing']))
    print("")


def prompt_processor_indexes(processors):
    """Ask the user for a comma-separated list of unique GPU indexes."""
    input_indexes = input(
        "  Enter comma-separated GPU indexes for the custom group: ")
    # Keep asking until every value is a valid GPU index and no index is
    # repeated.
    while True:
        values = [value.strip() for value in input_indexes.split(",")]
        if (len(values) > 0 and all(value.isdigit() for value in values)):
            indexes = [int(value) for value in values]
            if (all(index < len(processors) for index in indexes)
                    and len(set(indexes)) == len(indexes)):
                return indexes

        print("  Enter unique GPU indexes between 0 and {}, separated by "
              "commas".format(len(processors) - 1))
        input_indexes = input(
            "  Enter comma-separated GPU indexes for the custom group: ")


def set_xgmi_fb_sharing_mode_v2(processors, processor_number):
    """Change the XGMI framebuffer sharing mode and verify the result."""
    print("  Changing XGMI framebuffer sharing requires no running guest VMs.")
    mode = prompt_xgmi_mode()
    if mode == AmdSmiXgmiFbSharingMode.CUSTOM:
        # In custom mode the user builds the sharing group explicitly. All
        # GPUs in the group must be on the same NUMA node, and any GPU that
        # already belongs to a group is released from it first.
        processor_indexes = prompt_processor_indexes(processors)
    else:
        # In auto modes, the v2 API uses only the first processor in the list
        # and applies the selected mode within that GPU's NUMA node. It is
        # equivalent to amdsmi_set_xgmi_fb_sharing_mode() for a single GPU.
        processor_indexes = [processor_number]

    # Require explicit confirmation because changing the sharing mode
    # reconfigures how the selected GPUs share framebuffer memory.
    print("  Selected GPUs are: {}".format(processor_indexes))
    if not prompt_confirmation(
            "  Are you sure you want to set {} with the v2 API?".format(
                mode.name)):
        print("  No operation was performed.\n")
        return

    # The v2 API takes processor handles, not indexes.
    selected_processors = [
        processors[index] for index in processor_indexes
    ]
    print("###Test amdsmi_set_xgmi_fb_sharing_mode_v2")
    if set_optional_api(
            "amdsmi_set_xgmi_fb_sharing_mode_v2",
            amdsmi_set_xgmi_fb_sharing_mode_v2, selected_processors,
            mode) != AmdSmiRetCode.SUCCESS:
        print("")
        return
    print("  XGMI framebuffer sharing v2 mode change request completed")

    # Read the sharing state back from each affected GPU to confirm that the
    # new mode took effect.
    for source_number in processor_indexes:
        print_xgmi_sharing_verification(processors, source_number, mode)


def set_xgmi_plpd(processor):
    """Change the XGMI per-link power down policy and verify the result."""
    # Show the available policies first so the user knows which IDs are
    # valid.
    plpd = print_xgmi_plpd(processor)
    if plpd is None or len(plpd['policies']) == 0:
        return

    # The set API takes a policy ID, while 'cur' is an index into the
    # policies list. Look up the current policy's ID to use as the default
    # input.
    policy_ids = [policy['policy_id'] for policy in plpd['policies']]
    current_policy_index = plpd['cur']
    if (current_policy_index < 0
            or current_policy_index >= len(plpd['policies'])):
        print("  The current policy index does not identify a supported "
              "policy; defaulting to the first supported policy.\n")
        default_policy_id = policy_ids[0]
    else:
        default_policy_id = \
            plpd['policies'][current_policy_index]['policy_id']
    # valid_values limits the input to IDs that appear in the policy list.
    policy_id = prompt_for_input(
        "  Enter the XGMI PLPD policy ID to set: ",
        default_policy_id,
        valid_values=policy_ids,
        error_message="  Policy ID must be one of: {}".format(policy_ids))

    if not prompt_confirmation(
            "  Are you sure you want to set XGMI PLPD policy {}?".format(
                policy_id)):
        print("  No operation was performed.\n")
        return

    print("###Test amdsmi_set_xgmi_plpd")
    if set_optional_api("amdsmi_set_xgmi_plpd", amdsmi_set_xgmi_plpd,
                        processor, policy_id) != AmdSmiRetCode.SUCCESS:
        print("")
        return
    print("  XGMI PLPD policy change request completed\n")

    # Read the policies again to confirm the new current policy.
    # The setter applies that policy to every GPU on the same socket.
    print_xgmi_plpd(processor)


if __name__ == "__main__":
    initialized = False

    try:
        # INIT_ALL_PROCESSORS is needed to discover GPUs and optional NICs.
        init_flags = AmdSmiInitFlags.INIT_ALL_PROCESSORS
        print("###Test amdsmi_init")
        print("  Initializing with {}".format(init_flags.name))
        amdsmi_init(init_flags)
        initialized = True
        print("  Initialization complete")

        # Discover GPU handles. Each handle is passed to the topology APIs
        # below and remains valid until amdsmi_shut_down is called.
        print("###Test amdsmi_get_processor_handles")
        processors = amdsmi_get_processor_handles()
        print("  number of GPUs is: {}\n".format(len(processors)))

        if len(processors) == 0:
            print("  No GPUs found")
        else:
            # Operation 0 only reads information. Operations 1 and 2 change
            # GPU configuration and ask for confirmation before applying.
            print("  Available operations:")
            print("  0: Display topology and interconnect information")
            print("  1: Set XGMI framebuffer sharing mode v2")
            print("  2: Set XGMI per-link power down policy")
            print("  3: Exit")

            operation_number = prompt_for_input(
                "  Enter the operation to perform: ",
                0,
                upper_bound=3,
                error_message="  Operation index must be between 0 and 3")
            if operation_number == 0:
                # Read-only queries can run on one GPU or on all GPUs.
                selected_gpu_indexes = prompt_gpu_indexes(
                    processors, include_all=True)
                display_topology(processors, selected_gpu_indexes)
            elif operation_number == 3:
                print("  No operation was performed.")
            else:
                # Set operations act on a single GPU, so "All GPUs" is not
                # offered.
                selected_gpu_indexes = prompt_gpu_indexes(
                    processors, include_all=False)
                processor_number = selected_gpu_indexes[0]
                processor = processors[processor_number]
                print("###Test amdsmi_get_gpu_device_bdf")
                bdf = amdsmi_get_gpu_device_bdf(processor)
                print("  Selected GPU {} bdf: {}\n".format(
                    processor_number, bdf))

                if operation_number == 1:
                    set_xgmi_fb_sharing_mode_v2(
                        processors, processor_number)
                else:
                    set_xgmi_plpd(processor)

    except AmdSmiLibraryException as e:
        status = AmdSmiRetCode(e.err_code)
        print(amdsmi_status_code_to_string(status))
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
