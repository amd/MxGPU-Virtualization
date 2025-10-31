#!/usr/bin/env python3

#
# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""
AMD SMI NIC Interface Module

This module contains NIC specific APIs that can be optionally included
in the AMD SMI Python package. These APIs provide functionality for
Network Interface Card monitoring.
"""

import ctypes
from enum import IntEnum
from . import amdsmi_wrapper
from .amdsmi_exception import *
from .amdsmi_interface import _check_res, _format_bdf


def amdsmi_get_nic_driver_info(processor_handle):
    if not isinstance(processor_handle, amdsmi_wrapper.amdsmi_processor_handle):
        raise AmdSmiParameterException(processor_handle, amdsmi_wrapper.amdsmi_processor_handle)

    driver_info = amdsmi_wrapper.amdsmi_nic_driver_info_t()
    _check_res(amdsmi_wrapper.amdsmi_get_nic_driver_info(
        processor_handle, ctypes.byref(driver_info)))

    return {
        'name': driver_info.name.decode("utf-8"),
        'version': driver_info.version.decode("utf-8")
    }


def amdsmi_get_nic_asic_info(processor_handle):
    if not isinstance(processor_handle, amdsmi_wrapper.amdsmi_processor_handle):
        raise AmdSmiParameterException(processor_handle, amdsmi_wrapper.amdsmi_processor_handle)

    asic_info = amdsmi_wrapper.amdsmi_nic_asic_info_t()
    _check_res(amdsmi_wrapper.amdsmi_get_nic_asic_info(
        processor_handle, ctypes.byref(asic_info)))

    return {
        'vendor_id': hex(asic_info.vendor_id),
        'subvendor_id': hex(asic_info.subvendor_id),
        'device_id': hex(asic_info.device_id),
        'subsystem_id': hex(asic_info.subsystem_id),
        'revision': hex(asic_info.revision),
        'permanent_address': asic_info.permanent_address.decode("utf-8"),
        'product_name': asic_info.product_name.decode("utf-8"),
        'part_number': asic_info.part_number.decode("utf-8"),
        'serial_number': asic_info.serial_number.decode("utf-8"),
        'vendor_name': asic_info.vendor_name.decode("utf-8")
    }


def amdsmi_get_nic_bus_info(processor_handle):
    if not isinstance(processor_handle, amdsmi_wrapper.amdsmi_processor_handle):
        raise AmdSmiParameterException(processor_handle, amdsmi_wrapper.amdsmi_processor_handle)

    bus_info = amdsmi_wrapper.amdsmi_nic_bus_info_t()
    _check_res(amdsmi_wrapper.amdsmi_get_nic_bus_info(
        processor_handle, ctypes.byref(bus_info)))

    return {
        'bdf': _format_bdf(bus_info.bdf),
        'max_pcie_width': bus_info.max_pcie_width,
        'max_pcie_speed': bus_info.max_pcie_speed,
        'pcie_interface_version': bus_info.pcie_interface_version.decode("utf-8"),
        'slot_type': bus_info.slot_type.decode("utf-8")
    }


def amdsmi_get_nic_numa_info(processor_handle):
    if not isinstance(processor_handle, amdsmi_wrapper.amdsmi_processor_handle):
        raise AmdSmiParameterException(processor_handle, amdsmi_wrapper.amdsmi_processor_handle)

    numa_info = amdsmi_wrapper.amdsmi_nic_numa_info_t()
    _check_res(amdsmi_wrapper.amdsmi_get_nic_numa_info(
        processor_handle, ctypes.byref(numa_info)))

    return {
        'node': numa_info.node,
        'affinity': numa_info.affinity.decode("utf-8")
    }


def amdsmi_get_nic_port_info(processor_handle):
    if not isinstance(processor_handle, amdsmi_wrapper.amdsmi_processor_handle):
        raise AmdSmiParameterException(processor_handle, amdsmi_wrapper.amdsmi_processor_handle)

    port_info = amdsmi_wrapper.amdsmi_nic_port_info_t()
    _check_res(amdsmi_wrapper.amdsmi_get_nic_port_info(
        processor_handle, ctypes.byref(port_info)))

    ports = []
    for i in range(port_info.num_ports):
        port = port_info.ports[i]
        ports.append({
            'bdf': _format_bdf(port.bdf),
            'port_num': port.port_num,
            'type': port.type.decode("utf-8"),
            'flavour': port.flavour.decode("utf-8"),
            'netdev': port.netdev.decode("utf-8"),
            'ifindex': port.ifindex,
            'mac_address': port.mac_address.decode("utf-8"),
            'carrier': port.carrier,
            'mtu': port.mtu,
            'link_state': port.link_state.decode("utf-8"),
            'link_speed': port.link_speed,
            'active_fec': port.active_fec,
            'autoneg': port.autoneg.decode("utf-8"),
            'pause_autoneg': port.pause_autoneg.decode("utf-8"),
            'pause_rx': port.pause_rx.decode("utf-8"),
            'pause_tx': port.pause_tx.decode("utf-8"),
        })

    return {
        'ports': ports
    }


def amdsmi_get_nic_rdma_dev_info(processor_handle):
    if not isinstance(processor_handle, amdsmi_wrapper.amdsmi_processor_handle):
        raise AmdSmiParameterException(processor_handle, amdsmi_wrapper.amdsmi_processor_handle)

    rdma_devices_info = amdsmi_wrapper.amdsmi_nic_rdma_devices_info_t()
    _check_res(amdsmi_wrapper.amdsmi_get_nic_rdma_dev_info(
        processor_handle, ctypes.byref(rdma_devices_info)))
    rdma_dev_info = []
    for i in range (0, rdma_devices_info.num_rdma_dev):
        rdma_port_info = []
        for j in range (0, rdma_devices_info.rdma_dev_info[i].num_rdma_ports):
            rdma_port_info.append({
                'netdev': rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].netdev.decode("utf-8"),
                'state': rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].state.decode("utf-8"),
                'rdma_port': rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].rdma_port,
                'max_mtu': rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].max_mtu,
                'active_mtu': rdma_devices_info.rdma_dev_info[i].rdma_port_info[j].active_mtu
            })
        rdma_dev_info.append({
            'rdma_dev': rdma_devices_info.rdma_dev_info[i].rdma_dev.decode("utf-8"),
            'node_guid': rdma_devices_info.rdma_dev_info[i].node_guid.decode("utf-8"),
            'node_type': rdma_devices_info.rdma_dev_info[i].node_type.decode("utf-8"),
            'sys_image_guid': rdma_devices_info.rdma_dev_info[i].sys_image_guid.decode("utf-8"),
            'fw_ver': rdma_devices_info.rdma_dev_info[i].fw_ver.decode("utf-8"),
            'rdma_port_info': rdma_port_info
        })

    return {
        'rdma_dev_info': rdma_dev_info
    }


def amdsmi_get_nic_port_statistics(processor_handle, port_index):
    if not isinstance(processor_handle, amdsmi_wrapper.amdsmi_processor_handle):
        raise AmdSmiParameterException(processor_handle, amdsmi_wrapper.amdsmi_processor_handle)

    num_stats = ctypes.c_uint32(0)
    _check_res(amdsmi_wrapper.amdsmi_get_nic_port_statistics(
        processor_handle, port_index, ctypes.byref(num_stats), None))

    stats_list = (amdsmi_wrapper.amdsmi_nic_stat_t * num_stats.value)()
    _check_res(amdsmi_wrapper.amdsmi_get_nic_port_statistics(
        processor_handle, port_index, ctypes.byref(num_stats), stats_list))

    stats = []
    for i in range(num_stats.value):
        stats.append({
            'name': stats_list[i].name.decode('utf-8'),
            'value': stats_list[i].value
        })

    return stats


def amdsmi_get_nic_vendor_statistics(processor_handle, port_index):
    if not isinstance(processor_handle, amdsmi_wrapper.amdsmi_processor_handle):
        raise AmdSmiParameterException(processor_handle, amdsmi_wrapper.amdsmi_processor_handle)

    num_stats = ctypes.c_uint32(0)
    _check_res(amdsmi_wrapper.amdsmi_get_nic_vendor_statistics(
        processor_handle, port_index, ctypes.byref(num_stats), None))

    stats_list = (amdsmi_wrapper.amdsmi_nic_stat_t * num_stats.value)()
    _check_res(amdsmi_wrapper.amdsmi_get_nic_vendor_statistics(
        processor_handle, port_index, ctypes.byref(num_stats), stats_list))

    stats = []
    for i in range(num_stats.value):
        stats.append({
            'name': stats_list[i].name.decode('utf-8'),
            'value': stats_list[i].value
        })

    return stats


def amdsmi_get_nic_rdma_port_statistics(processor_handle, rdma_port_index):
    if not isinstance(processor_handle, amdsmi_wrapper.amdsmi_processor_handle):
        raise AmdSmiParameterException(processor_handle, amdsmi_wrapper.amdsmi_processor_handle)

    num_stats = ctypes.c_uint32(0)
    _check_res(amdsmi_wrapper.amdsmi_get_nic_rdma_port_statistics(
        processor_handle, rdma_port_index, ctypes.byref(num_stats), None))

    stats_list = (amdsmi_wrapper.amdsmi_nic_stat_t * num_stats.value)()
    _check_res(amdsmi_wrapper.amdsmi_get_nic_rdma_port_statistics(
        processor_handle, rdma_port_index, ctypes.byref(num_stats), stats_list))

    stats = []
    for i in range(num_stats.value):
        stats.append({
            'name': stats_list[i].name.decode('utf-8'),
            'value': stats_list[i].value
        })

    return stats
