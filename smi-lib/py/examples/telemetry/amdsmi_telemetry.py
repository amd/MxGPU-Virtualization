# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

import time
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amdsmi import *
from example_utils import get_optional_api, print_optional_api_error


# Increase either value to collect a longer sample or stop the script with Ctrl+C.
POLL_ITERATIONS = 3
POLL_INTERVAL_SECONDS = 1


def value_or_not_available(data, *keys):
    # Walk nested dictionaries only when the API returned data.
    if data is None:
        return "N/A"
    for key in keys:
        data = data[key]
    return data


def format_statistics(statistics):
    # NIC statistic APIs return a list of name/value dictionaries.
    # Distinguish between an API failure and a successful response with no statistics.
    if statistics is None:
        return "not available"
    if len(statistics) == 0:
        return "none"
    return ", ".join("{}={}".format(statistic['name'], statistic['value'])
                     for statistic in statistics)


def current_pci_bandwidth_value(pci_bandwidth, key):
    # transfer_rate['current'] is the index of the active entry in
    # frequency[] and lanes[], not the transfer rate itself.
    if pci_bandwidth is None:
        return "N/A"
    transfer_rate = pci_bandwidth['transfer_rate']
    current_index = transfer_rate['current']
    values = (transfer_rate['frequency'] if key == 'frequency'
              else pci_bandwidth['lanes'])
    if current_index >= len(values):
        return "N/A"
    return values[current_index]


def print_gpu_pcie_capabilities(processor):
    # PCIe information combines static link capabilities with values that
    # change while the GPU is running.
    print("###Test amdsmi_get_pcie_info")
    pcie_info = get_optional_api(
        "amdsmi_get_pcie_info", amdsmi_get_pcie_info, processor)
    if pcie_info is not None:
        pcie_static = pcie_info['pcie_static']
        print("  pcie_static['max_pcie_width'] is: {}".format(
            pcie_static['max_pcie_width']))
        print("  pcie_static['max_pcie_speed'] is: {}".format(
            pcie_static['max_pcie_speed']))
        print("  pcie_static['pcie_interface_version'] is: {}".format(
            pcie_static['pcie_interface_version']))
        print("  pcie_static['slot_type'] is: {}".format(
            pcie_static['slot_type'].name))
        print("  pcie_static['max_pcie_interface_version'] is: {}".format(
            pcie_static['max_pcie_interface_version']))
    print("")

    # The bandwidth API provides parallel frequency and lane-width arrays.
    # The active array element is selected by transfer_rate['current'].
    print("###Test amdsmi_get_gpu_pci_bandwidth")
    pci_bandwidth = get_optional_api(
        "amdsmi_get_gpu_pci_bandwidth", amdsmi_get_gpu_pci_bandwidth,
        processor)
    if pci_bandwidth is not None:
        transfer_rate = pci_bandwidth['transfer_rate']
        print("  transfer_rate['current'] index is: {}".format(
            transfer_rate['current']))
        print("  current transfer rate is: {}".format(
            current_pci_bandwidth_value(pci_bandwidth, 'frequency')))
        print("  current lanes is: {}".format(
            current_pci_bandwidth_value(pci_bandwidth, 'lanes')))
        print("  number of supported transfer rates is: {}".format(
            transfer_rate['num_supported']))
        for frequency, lanes in zip(transfer_rate['frequency'],
                                    pci_bandwidth['lanes']):
            print("  supported frequency: {} lanes: {}".format(
                frequency, lanes))
    print("")


def select_gpu_telemetry_paths(processors):
    # Prefer the self-describing metrics table. A NOT_SUPPORTED response is a
    # capability signal to use the focused compatibility APIs for that GPU.

    telemetry_paths = []
    print("###Selecting GPU telemetry APIs")

    # Store the chosen path per GPU because support can differ across devices
    # in the same system. A metrics value of None identifies the fallback path.
    for processor_number, processor in enumerate(processors):
        try:
            metrics = amdsmi_get_gpu_metrics(processor)
        except AmdSmiLibraryException as e:
            # Only STATUS_NOT_SUPPORTED selects the compatibility path. Other
            # errors may be transient or device-specific, so report and skip
            # that GPU instead of hiding the error behind fallback queries.
            if e.err_code == AmdSmiRetCode.STATUS_NOT_SUPPORTED:
                print("  GPU {}: amdsmi_get_gpu_metrics is not supported; "
                      "using compatibility telemetry APIs".format(
                          processor_number))
                telemetry_paths.append(
                    (processor_number, processor, None))
            else:
                print_optional_api_error(
                    "amdsmi_get_gpu_metrics for GPU {}".format(
                        processor_number), e)
                print("  GPU {}: telemetry path could not be selected; "
                      "skipping GPU telemetry".format(processor_number))
        else:
            print("  GPU {}: using amdsmi_get_gpu_metrics".format(
                processor_number))
            telemetry_paths.append((processor_number, processor, metrics))
    print("")

    return telemetry_paths


def poll_gpu_metrics(processor_number, processor, poll_iteration, metrics):
    # Include the BDF in every sample so output remains identifiable when
    # multiple GPUs are polled during the same run.
    bdf = get_optional_api(
        "amdsmi_get_gpu_device_bdf", amdsmi_get_gpu_device_bdf, processor)
    metric_count = len(metrics) if metrics is not None else "N/A"
    print("  Poll {} GPU {} {}: metrics={}".format(
        poll_iteration, processor_number, value_or_not_available(bdf),
        metric_count))

    if metrics is None:
        return
    if len(metrics) == 0:
        print("    No GPU metrics found")
        return

    # Each row is self-describing: name and unit explain the value, category
    # groups its purpose, and resource fields identify the measured instance.
    # Sort by name so related metrics (e.g. CLK_GFX and CLK_GFX_MIN_LIMIT)
    # appear together; tie-break on resource identity for multi-instance rows.
    sorted_metrics = sorted(
        metrics,
        key=lambda m: (
            m['name'].name,
            m['res_group'].name,
            m['res_subgroup'].name,
            m['res_instance'],
        ),
    )

    for i, metric in enumerate(sorted_metrics):
        # A metric can have multiple type flags. Join them for compact output;
        # use "-" when no flags apply. vf_mask identifies the PF and active VFs
        # to which the metric applies.
        flag_names = metric['flags']
        flag_str = "|".join(flag_names) if flag_names else "-"
        print("    [{:>3}] name={:<28} unit={:<10} cat={:<10} val={:<20} "
              "flags={:<20} vf_mask=0x{:08X} res={}/{} inst={}".format(
                  i, metric['name'].name, metric['unit'].name,
                  metric['category'].name, metric['val'], flag_str,
                  metric['vf_mask'], metric['res_group'].name,
                  metric['res_subgroup'].name, metric['res_instance']))


def poll_gpu_compatibility_telemetry(
        processor_number, processor, poll_iteration):
    # Some devices do not expose the unified metrics table. Query the focused
    # activity, power, temperature, clock, and PCIe APIs instead.
    bdf = get_optional_api(
        "amdsmi_get_gpu_device_bdf", amdsmi_get_gpu_device_bdf, processor)
    activity = get_optional_api(
        "amdsmi_get_gpu_activity", amdsmi_get_gpu_activity, processor)
    power = get_optional_api(
        "amdsmi_get_power_info", amdsmi_get_power_info, processor)
    edge_temperature = get_optional_api(
        "amdsmi_get_temp_metric (EDGE CURRENT)",
        amdsmi_get_temp_metric, processor, AmdSmiTemperatureType.EDGE,
        AmdSmiTemperatureMetric.CURRENT)
    hotspot_temperature = get_optional_api(
        "amdsmi_get_temp_metric (HOTSPOT CURRENT)",
        amdsmi_get_temp_metric, processor, AmdSmiTemperatureType.HOTSPOT,
        AmdSmiTemperatureMetric.CURRENT)
    gfx_clock = get_optional_api(
        "amdsmi_get_clock_info (GFX)", amdsmi_get_clock_info, processor,
        AmdSmiClkType.GFX)
    mem_clock = get_optional_api(
        "amdsmi_get_clock_info (MEM)", amdsmi_get_clock_info, processor,
        AmdSmiClkType.MEM)
    pcie_info = get_optional_api(
        "amdsmi_get_pcie_info", amdsmi_get_pcie_info, processor)
    pcie_metric = (
        pcie_info['pcie_metric'] if pcie_info is not None else None)
    pci_bandwidth = get_optional_api(
        "amdsmi_get_gpu_pci_bandwidth", amdsmi_get_gpu_pci_bandwidth,
        processor)

    # Print one line per sample to make changes between poll iterations easy
    # to compare. Values retain the units defined by their individual APIs.
    print(
        "  Poll {} GPU {} {}: activity(gfx={}, umc={}, mm={}) "
        "power(socket={}, gfx_voltage={}, soc_voltage={}, "
        "mem_voltage={}) temp(edge={}, hotspot={}) "
        "gfx_clock(clk={}, min={}, max={}, locked={}, deep_sleep={}) "
        "mem_clock(clk={}, min={}, max={}, deep_sleep={}) "
        "pcie(speed={}, width={}, bandwidth={}, current_rate={}, "
        "replay={}, l0_recovery={}, rollover={}, nak_sent={}, "
        "nak_received={}, other_recovery={})".format(
            poll_iteration, processor_number, value_or_not_available(bdf),
            value_or_not_available(activity, 'gfx_activity'),
            value_or_not_available(activity, 'umc_activity'),
            value_or_not_available(activity, 'mm_activity'),
            value_or_not_available(power, 'socket_power'),
            value_or_not_available(power, 'gfx_voltage'),
            value_or_not_available(power, 'soc_voltage'),
            value_or_not_available(power, 'mem_voltage'),
            value_or_not_available(edge_temperature),
            value_or_not_available(hotspot_temperature),
            value_or_not_available(gfx_clock, 'clk'),
            value_or_not_available(gfx_clock, 'min_clk'),
            value_or_not_available(gfx_clock, 'max_clk'),
            value_or_not_available(gfx_clock, 'clk_locked'),
            value_or_not_available(gfx_clock, 'clk_deep_sleep'),
            value_or_not_available(mem_clock, 'clk'),
            value_or_not_available(mem_clock, 'min_clk'),
            value_or_not_available(mem_clock, 'max_clk'),
            value_or_not_available(mem_clock, 'clk_deep_sleep'),
            value_or_not_available(pcie_metric, 'pcie_speed'),
            value_or_not_available(pcie_metric, 'pcie_width'),
            value_or_not_available(pcie_metric, 'pcie_bandwidth'),
            current_pci_bandwidth_value(pci_bandwidth, 'frequency'),
            value_or_not_available(pcie_metric, 'pcie_replay_count'),
            value_or_not_available(
                pcie_metric, 'pcie_l0_to_recovery_count'),
            value_or_not_available(
                pcie_metric, 'pcie_replay_roll_over_count'),
            value_or_not_available(pcie_metric, 'pcie_nak_sent_count'),
            value_or_not_available(pcie_metric, 'pcie_nak_received_count'),
            value_or_not_available(
                pcie_metric, 'pcie_lc_perf_other_end_recovery_count')))


def poll_gpu_telemetry(processors):
    # Probe API support once before polling so every GPU consistently uses the
    # unified metrics path or the compatibility path throughout this run.
    telemetry_paths = select_gpu_telemetry_paths(processors)
    if len(telemetry_paths) == 0:
        print("  No GPU telemetry paths available\n")
        return

    print("###Polling GPU telemetry")
    print("  Values use the units returned by the AMD SMI API.")

    for poll_iteration in range(1, POLL_ITERATIONS + 1):
        for processor_number, processor, initial_metrics in telemetry_paths:
            # None is the marker selected above for compatibility APIs.
            if initial_metrics is None:
                poll_gpu_compatibility_telemetry(
                    processor_number, processor, poll_iteration)
                continue

            # Reuse the metrics returned by capability probing for the first
            # sample, then fetch a fresh metrics table for subsequent samples.
            metrics = initial_metrics
            if poll_iteration > 1:
                metrics = get_optional_api(
                    "amdsmi_get_gpu_metrics", amdsmi_get_gpu_metrics,
                    processor)
            poll_gpu_metrics(
                processor_number, processor, poll_iteration, metrics)

        # Do not sleep after the final sample so the example exits promptly.
        if poll_iteration < POLL_ITERATIONS:
            time.sleep(POLL_INTERVAL_SECONDS)
    print("")


def get_nic_rdma_ports(nic, nic_number):
    # amdsmi_get_nic_rdma_port_statistics() takes the zero-based position of
    # the RDMA port in the order amdsmi_get_nic_rdma_dev_info() lists them:
    # all ports of the first RDMA device, then all ports of the next, and so
    # on. It is not the rdma_port number.
    rdma_dev_info = get_optional_api(
        "amdsmi_get_nic_rdma_dev_info for NIC {}".format(nic_number),
        amdsmi_get_nic_rdma_dev_info, nic)
    if rdma_dev_info is None:
        return []

    # Flatten devices and ports into one list while preserving both the
    # human-readable port number and the API's positional statistics index.
    rdma_ports = []
    rdma_port_index = 0
    for rdma_device in rdma_dev_info['rdma_dev_info']:
        for rdma_port in rdma_device['rdma_port_info']:
            rdma_ports.append({
                'device': rdma_device['rdma_dev'],
                'netdev': rdma_port['netdev'],
                'rdma_port': rdma_port['rdma_port'],
                'rdma_port_index': len(rdma_ports),
            })
    return rdma_ports


def print_nic_telemetry():
    # NIC monitoring is optional. INIT_ALL_PROCESSORS enables discovery of
    # both GPU and NIC processors without changing either device's state.
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

    for nic_number, nic in enumerate(nic_processors):
        # The NIC BDF identifies which physical adapter owns the ports below.
        print("###Test amdsmi_get_nic_device_bdf")
        nic_bdf = get_optional_api(
            "amdsmi_get_nic_device_bdf", amdsmi_get_nic_device_bdf, nic)
        print("  NIC number: {} bdf: {}".format(
            nic_number, value_or_not_available(nic_bdf)))

        print("###Test amdsmi_get_nic_port_info")
        port_info = get_optional_api(
            "amdsmi_get_nic_port_info for NIC {}".format(nic_number),
            amdsmi_get_nic_port_info, nic)
        if port_info is None:
            print("")
            continue

        # Port information describes current link configuration and state.
        # port_index is the zero-based index expected by NIC statistics APIs.
        ports = port_info['ports']
        print("  number of NIC ports is: {}".format(len(ports)))
        for port_index, port in enumerate(ports):
            print(
                "  port {}: bdf={} port_num={} type={} flavour={} "
                "netdev={} ifindex={} mac_address={} carrier={} mtu={} "
                "link_state={} link_speed={} active_fec={} autoneg={} "
                "pause_autoneg={} pause_rx={} pause_tx={}".format(
                    port_index, port['bdf'], port['port_num'], port['type'],
                    port['flavour'], port['netdev'], port['ifindex'],
                    port['mac_address'], port['carrier'], port['mtu'],
                    port['link_state'], port['link_speed'],
                    port['active_fec'], port['autoneg'],
                    port['pause_autoneg'], port['pause_rx'],
                    port['pause_tx']))
        print("")

        # Discover RDMA ports separately because their statistics API uses a
        # flattened RDMA port index rather than the regular NIC port index.
        rdma_ports = get_nic_rdma_ports(nic, nic_number)
        if len(rdma_ports) == 0:
            print("  No RDMA ports found for NIC {}".format(nic_number))
        else:
            for rdma_port in rdma_ports:
                print(
                    "  RDMA device {} port {} (index {}) netdev={}".format(
                        rdma_port['device'], rdma_port['rdma_port'],
                        rdma_port['rdma_port_index'], rdma_port['netdev']))
        print("")

        print("###Polling NIC counters")
        for poll_iteration in range(1, POLL_ITERATIONS + 1):
            # Port state can change while counters are sampled, so retrieve a
            # fresh port list for each poll iteration.
            port_info = get_optional_api(
                "amdsmi_get_nic_port_info for NIC {}".format(nic_number),
                amdsmi_get_nic_port_info, nic)
            if port_info is None:
                break
            ports = port_info['ports']

            for port_index, port in enumerate(ports):
                # Standard and vendor counters are queried independently;
                # either set may be unsupported on a particular adapter.
                port_statistics = get_optional_api(
                    "amdsmi_get_nic_port_statistics for NIC {} port {}".format(
                        nic_number, port_index),
                    amdsmi_get_nic_port_statistics, nic, port_index)
                vendor_statistics = get_optional_api(
                    "amdsmi_get_nic_vendor_statistics for NIC {} port {}".format(
                        nic_number, port_index),
                    amdsmi_get_nic_vendor_statistics, nic, port_index)
                print(
                    "  Poll {} NIC {} port {} netdev={} link_state={} "
                    "link_speed={} port_stats=[{}] vendor_stats=[{}]".format(
                        poll_iteration, nic_number, port_index,
                        port['netdev'], port['link_state'],
                        port['link_speed'], format_statistics(port_statistics),
                        format_statistics(vendor_statistics)))

            for rdma_port in rdma_ports:
                # Pass the flattened positional index calculated during RDMA
                # discovery, not the device-reported rdma_port value.
                rdma_statistics = get_optional_api(
                    "amdsmi_get_nic_rdma_port_statistics for NIC {} "
                    "RDMA port index {}".format(
                        nic_number, rdma_port['rdma_port_index']),
                    amdsmi_get_nic_rdma_port_statistics, nic,
                    rdma_port['rdma_port_index'])
                print(
                    "  Poll {} NIC {} RDMA device {} port {} "
                    "(index {}) netdev={} stats=[{}]".format(
                        poll_iteration, nic_number, rdma_port['device'],
                        rdma_port['rdma_port'],
                        rdma_port['rdma_port_index'], rdma_port['netdev'],
                        format_statistics(rdma_statistics)))

            if poll_iteration < POLL_ITERATIONS:
                time.sleep(POLL_INTERVAL_SECONDS)
        print("")


if __name__ == "__main__":
    initialized = False

    try:
        # INIT_ALL_PROCESSORS is needed to discover both GPUs and optional NICs.
        init_flags = AmdSmiInitFlags.INIT_ALL_PROCESSORS
        print("###Test amdsmi_init")
        print("  Initializing with {}".format(init_flags.name))
        amdsmi_init(init_flags)
        initialized = True
        print("  Initialization complete")

        # Retrieve the GPU processor handles used by the telemetry queries below.
        print("###Test amdsmi_get_processor_handles")
        processors = amdsmi_get_processor_handles()
        print("  number of GPUs is: {}\n".format(len(processors)))

        if len(processors) == 0:
            print("  No GPUs found\n")
        else:
            # Print each GPU's BDF and PCIe capabilities before sampling so
            # users can associate telemetry output with a physical device.
            for processor_number, processor in enumerate(processors):
                print("###Test amdsmi_get_gpu_device_bdf")
                bdf = get_optional_api(
                    "amdsmi_get_gpu_device_bdf",
                    amdsmi_get_gpu_device_bdf, processor)
                print("  Processor number: {} bdf: {}\n".format(
                    processor_number, value_or_not_available(bdf)))
                print_gpu_pcie_capabilities(processor)

            poll_gpu_telemetry(processors)

        print_nic_telemetry()

    except AmdSmiLibraryException as e:
        status = AmdSmiRetCode(e.err_code)
        print(amdsmi_status_code_to_string(status))
    except AmdSmiException as e:
        print(e)
    except KeyboardInterrupt:
        print("\n  Keyboard interrupt detected. Exiting...")
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
