# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amdsmi import *
from example_utils import get_optional_api


# Print the ASIC properties that identify the GPU hardware. These values are
# returned as one dictionary by amdsmi_get_gpu_asic_info().
def print_gpu_asic_info(asic_info):
    # Device and revision IDs can be used to distinguish ASIC variants.
    print("  asic_info['market_name'] is: {}".format(
        asic_info['market_name']))
    print("  asic_info['vendor_id'] is: {}".format(
        asic_info['vendor_id']))
    print("  asic_info['vendor_name'] is: {}".format(
        asic_info['vendor_name']))
    print("  asic_info['subvendor_id'] is: {}".format(
        asic_info['subvendor_id']))
    print("  asic_info['device_id'] is: {}".format(
        asic_info['device_id']))
    print("  asic_info['rev_id'] is: {}".format(asic_info['rev_id']))

    # The remaining printed fields combine silicon characteristics, module
    # identifiers, PCI subsystem identity, and capability flags.
    print("  asic_info['asic_serial'] is: {}".format(
        asic_info['asic_serial']))
    print("  asic_info['oam_id'] is: {}".format(asic_info['oam_id']))
    print("  asic_info['num_of_compute_units'] is: {}".format(
        asic_info['num_of_compute_units']))
    print("  asic_info['target_graphics_version'] is: {}".format(
        asic_info['target_graphics_version']))
    print("  asic_info['subsystem_id'] is: {}".format(
        asic_info['subsystem_id']))
    print("  asic_info['flags'] is: {}".format(asic_info['flags']))


# Print manufacturing and product information for the GPU board.
def print_gpu_board_info(board_info):
    print("  board_info['model_number'] is: {}".format(
        board_info['model_number']))
    print("  board_info['product_serial'] is: {}".format(
        board_info['product_serial']))
    print("  board_info['fru_id'] is: {}".format(board_info['fru_id']))
    print("  board_info['product_name'] is: {}".format(
        board_info['product_name']))
    print("  board_info['manufacturer_name'] is: {}".format(
        board_info['manufacturer_name']))


# Print the installed VRAM's type, vendor, capacity, interface width, and
# maximum bandwidth.
def print_gpu_vram_info(vram_info):
    # vram_type is an enum; .name produces a readable label.
    print("  vram_info['vram_type'] is: {}".format(
        vram_info['vram_type'].name))
    print("  vram_info['vram_vendor'] is: {}".format(
        vram_info['vram_vendor']))
    print("  vram_info['vram_size'] is: {}".format(
        vram_info['vram_size']))
    print("  vram_info['vram_bit_width'] is: {}".format(
        vram_info['vram_bit_width']))
    print("  vram_info['vram_max_bandwidth'] is: {}".format(
        vram_info['vram_max_bandwidth']))


# Cache information contains a list because a GPU can expose multiple cache
# levels or cache types.
def print_gpu_cache_info(cache_info):
    cache_entries = cache_info['cache']
    print("  number of cache types is: {}".format(len(cache_entries)))
    if len(cache_entries) == 0:
        print("  No cache entries found")
        return

    # Print each cache configuration separately so its level, size, sharing,
    # and instance count remain grouped together.
    for cache_number, cache in enumerate(cache_entries):
        print("  Cache {}:".format(cache_number))
        print("    cache_properties is: {}".format(
            cache['cache_properties']))
        print("    cache_size is: {}".format(cache['cache_size']))
        print("    cache_level is: {}".format(cache['cache_level']))
        print("    max_num_cu_shared is: {}".format(
            cache['max_num_cu_shared']))
        print("    num_cache_instance is: {}".format(
            cache['num_cache_instance']))


# Print the GPU driver identity reported for this device.
def print_gpu_driver_info(driver_info):
    print("  driver_info['driver_version'] is: {}".format(
        driver_info['driver_version']))
    print("  driver_info['driver_date'] is: {}".format(
        driver_info['driver_date']))
    print("  driver_info['driver_name'] is: {}".format(
        driver_info['driver_name']))


# RDMA information is nested by device and then by port. A NIC can expose more
# than one RDMA device, and each RDMA device can expose multiple ports.
def print_nic_rdma_dev_info(rdma_dev_info):
    rdma_devices = rdma_dev_info['rdma_dev_info']
    print("  number of RDMA devices is: {}".format(len(rdma_devices)))
    if len(rdma_devices) == 0:
        print("  No RDMA devices found")
        return

    for dev_number, rdma_dev in enumerate(rdma_devices):
        print("  RDMA device {}:".format(dev_number))
        print("    rdma_dev is: {}".format(rdma_dev['rdma_dev']))
        print("    node_guid is: {}".format(rdma_dev['node_guid']))
        print("    node_type is: {}".format(rdma_dev['node_type']))
        print("    sys_image_guid is: {}".format(
            rdma_dev['sys_image_guid']))
        print("    fw_ver is: {}".format(rdma_dev['fw_ver']))

        rdma_ports = rdma_dev['rdma_port_info']
        print("    number of RDMA ports is: {}".format(len(rdma_ports)))

        # Port fields identify the associated network interface and report the RDMA
        # port state, port number, and MTU values.
        for port_number, rdma_port in enumerate(rdma_ports):
            print("    RDMA port {}:".format(port_number))
            print("      netdev is: {}".format(rdma_port['netdev']))
            print("      state is: {}".format(rdma_port['state']))
            print("      rdma_port is: {}".format(rdma_port['rdma_port']))
            print("      max_mtu is: {}".format(rdma_port['max_mtu']))
            print("      active_mtu is: {}".format(rdma_port['active_mtu']))


# Query and print the static identity fields for one discovered GPU handle.
def print_gpu_static_identity(processor_number, processor):
    # BDF identifies the GPU's location on the PCI bus and helps correlate
    # this handle with the device shown by operating-system tools.
    print("###Test amdsmi_get_gpu_device_bdf")
    bdf = amdsmi_get_gpu_device_bdf(processor)
    print("  Processor number: {} bdf: {}\n".format(processor_number, bdf))

    # The following queries report mostly stable GPU identification, hardware,
    # driver, and virtualization-context information.
    print("###Test amdsmi_get_gpu_asic_info")
    asic_info = amdsmi_get_gpu_asic_info(processor)
    print_gpu_asic_info(asic_info)
    print("")

    # Board information identifies the manufactured accelerator product.
    print("###Test amdsmi_get_gpu_board_info")
    board_info = amdsmi_get_gpu_board_info(processor)
    print_gpu_board_info(board_info)
    print("")

    # VRAM information describes the memory installed on the board.
    print("###Test amdsmi_get_gpu_vram_info")
    vram_info = amdsmi_get_gpu_vram_info(processor)
    print_gpu_vram_info(vram_info)
    print("")

    # Cache information may contain several entries for different levels.
    print("###Test amdsmi_get_gpu_cache_info")
    cache_info = amdsmi_get_gpu_cache_info(processor)
    print_gpu_cache_info(cache_info)
    print("")

    # Driver information identifies the software controlling the GPU.
    print("###Test amdsmi_get_gpu_driver_info")
    driver_info = amdsmi_get_gpu_driver_info(processor)
    print_gpu_driver_info(driver_info)
    print("")

    # Virtualization mode reports how the GPU is currently configured for
    # physical-function or virtual-function use.
    print("###Test amdsmi_get_gpu_virtualization_mode")
    virtualization_mode = amdsmi_get_gpu_virtualization_mode(processor)
    print("  virtualization_mode is: {}".format(
        virtualization_mode.name))
    print("")


# Discover NICs and print the static identity fields available for each one.
def print_nic_static_identity():
    # NIC support is optional and may require a supported NIC driver.
    print("###Test amdsmi_get_nic_processor_handles")
    nic_processors = get_optional_api(
        "amdsmi_get_nic_processor_handles", amdsmi_get_nic_processor_handles)
    if nic_processors is None:
        print("")
        return

    print("  number of NICs is: {}".format(len(nic_processors)))
    if len(nic_processors) == 0:
        print("  No NICs found")
        print("")
        return

    for nic_number, nic in enumerate(nic_processors):
        # As with GPUs, the BDF correlates the AMD SMI handle with a PCI
        # device visible to the operating system.
        print("###Test amdsmi_get_nic_device_bdf")
        nic_bdf = amdsmi_get_nic_device_bdf(nic)
        print("  NIC number: {} bdf: {}\n".format(nic_number, nic_bdf))

        # get_optional_api() reports an unavailable call and allows the
        # rest of the example to continue instead of terminating.
        print("###Test amdsmi_get_nic_asic_info")
        asic_info = get_optional_api(
            "amdsmi_get_nic_asic_info for NIC {}".format(nic_number),
            amdsmi_get_nic_asic_info, nic)
        if asic_info is not None:
            print("  asic_info['vendor_id'] is: {}".format(
                asic_info['vendor_id']))
            print("  asic_info['subvendor_id'] is: {}".format(
                asic_info['subvendor_id']))
            print("  asic_info['device_id'] is: {}".format(
                asic_info['device_id']))
            print("  asic_info['subsystem_id'] is: {}".format(
                asic_info['subsystem_id']))
            print("  asic_info['revision'] is: {}".format(
                asic_info['revision']))
            print("  asic_info['permanent_address'] is: {}".format(
                asic_info['permanent_address']))
            print("  asic_info['product_name'] is: {}".format(
                asic_info['product_name']))
            print("  asic_info['part_number'] is: {}".format(
                asic_info['part_number']))
            print("  asic_info['serial_number'] is: {}".format(
                asic_info['serial_number']))
            print("  asic_info['vendor_name'] is: {}".format(
                asic_info['vendor_name']))
        print("")

        # Bus information describes the NIC's PCIe location and capabilities.
        print("###Test amdsmi_get_nic_bus_info")
        bus_info = get_optional_api(
            "amdsmi_get_nic_bus_info for NIC {}".format(nic_number),
            amdsmi_get_nic_bus_info, nic)
        if bus_info is not None:
            print("  bus_info['bdf'] is: {}".format(bus_info['bdf']))
            print("  bus_info['max_pcie_width'] is: {}".format(
                bus_info['max_pcie_width']))
            print("  bus_info['max_pcie_speed'] is: {}".format(
                bus_info['max_pcie_speed']))
            print("  bus_info['pcie_interface_version'] is: {}".format(
                bus_info['pcie_interface_version']))
            print("  bus_info['slot_type'] is: {}".format(
                bus_info['slot_type']))
        print("")

        # Driver information identifies the host software managing the NIC.
        print("###Test amdsmi_get_nic_driver_info")
        driver_info = get_optional_api(
            "amdsmi_get_nic_driver_info for NIC {}".format(nic_number),
            amdsmi_get_nic_driver_info, nic)
        if driver_info is not None:
            print("  driver_info['name'] is: {}".format(
                driver_info['name']))
            print("  driver_info['version'] is: {}".format(
                driver_info['version']))
        print("")

        # RDMA device details may be incomplete without libibverbs installed.
        print("###Test amdsmi_get_nic_rdma_dev_info")
        rdma_dev_info = get_optional_api(
            "amdsmi_get_nic_rdma_dev_info for NIC {}".format(nic_number),
            amdsmi_get_nic_rdma_dev_info, nic)
        if rdma_dev_info is not None:
            print_nic_rdma_dev_info(rdma_dev_info)
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

        # Processor discovery is the shared prelude for AMD SMI examples.
        print("###Test amdsmi_get_processor_handles")
        processors = amdsmi_get_processor_handles()
        print("  number of GPUs is: {}\n".format(len(processors)))

        if len(processors) == 0:
            print("  No GPUs found\n")

        # Query each GPU independently because every handle represents one
        # physical device and can report different identity information.
        for processor_number, processor in enumerate(processors):
            print_gpu_static_identity(processor_number, processor)

        print_nic_static_identity()

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
