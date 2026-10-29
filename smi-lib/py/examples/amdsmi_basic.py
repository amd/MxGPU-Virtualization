# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

from amdsmi import *

if __name__ == "__main__":
    try:
        amdsmi_init()

        print("###Test amdsmi_get_processor_handles ")
        processors = amdsmi_get_processor_handles()
        print("  number of GPUs is: {} \n".format(len(processors)))
        sensor_ind = 0
        for i in range(0, len(processors)):
            print("###Test amdsmi_get_gpu_device_bdf ")
            bdf = amdsmi_get_gpu_device_bdf(processors[i])
            print("  Processor number: {} bdf: {}".format(i, bdf))

            print("###Test amdsmi_get_power_cap_info ")
            power_info = amdsmi_get_power_cap_info(processors[i], sensor_ind)
            print("  power_info['power_cap'] is: {}\n".format(
                power_info['power_cap']))
            print("  power_info['default_power_cap'] is: {}\n".format(
                power_info['default_power_cap']))
            print("  power_info['dpm_cap'] is: {}".format(
                power_info['dpm_cap']))
            print("  power_info['min_power_cap'] is: {}\n".format(
                power_info['min_power_cap']))
            print("  power_info['max_power_cap'] is: {}\n".format(
                power_info['max_power_cap']))

            print("###Test amdsmi_get_supported_power_cap ")
            supported_caps = amdsmi_get_supported_power_cap(processors[i])
            print("  supported sensor_inds: {}".format(
                supported_caps['sensor_inds']))
            print("  supported sensor_types: {}\n".format(
                [t.name for t in supported_caps['sensor_types']]))
            for s_ind, s_type in zip(supported_caps['sensor_inds'],
                                     supported_caps['sensor_types']):
                cap = amdsmi_get_power_cap_info(processors[i], s_ind)
                print("  sensor_ind={} type={} power_cap={} default={} min={} max={}".format(
                    s_ind, s_type.name, cap['power_cap'],
                    cap['default_power_cap'], cap['min_power_cap'],
                    cap['max_power_cap']))
            print("")

            print("###Test amdsmi_get_fb_layout ")
            fb_info = amdsmi_get_fb_layout(processors[i])
            print("  fb_info['total_fb_size'] is: {}\n".format(
                fb_info['total_fb_size']))

            print("###Test amdsmi_get_power_info ")
            power_info = amdsmi_get_power_info(processors[i])
            print("  power_info['socket_power'] is: {}".format(
                power_info['socket_power']))
            print("  power_info['gfx_voltage'] is: {}".format(
                power_info['gfx_voltage']))
            print("  power_info['soc_voltage'] is: {}".format(
                power_info['soc_voltage']))
            print("  power_info['mem_voltage'] is: {}".format(
                power_info['mem_voltage']))

            print("###Test amdsmi_is_gpu_power_management_enabled ")
            is_power_management_enabled = amdsmi_is_gpu_power_management_enabled(processors[i])
            print("  Is power management enabled is: {}".format(
                is_power_management_enabled))

            print("###Test amdsmi_get_temp_metric ")
            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.EDGE, AmdSmiTemperatureMetric.CURRENT)
            print("  Temperature for temperature type EDGE is: {}".format(
                temperature_measure))

            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.HOTSPOT, AmdSmiTemperatureMetric.CURRENT)
            print("  Temperature for temperature type HOTSPOT is: {}".format(
                temperature_measure))

            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.VRAM, AmdSmiTemperatureMetric.CURRENT)
            print("  Temperature for temperature type VRAM is: {}".format(
                temperature_measure))

            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.PLX, AmdSmiTemperatureMetric.CURRENT)
            print("  Temperature for temperature type PLX is: {}\n".format(
                temperature_measure))

            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.EDGE, AmdSmiTemperatureMetric.CRITICAL)
            print("  Temperature limit for temperature type EDGE is: {}".format(
                temperature_measure))

            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.HOTSPOT, AmdSmiTemperatureMetric.CRITICAL)
            print("  Temperature limit for temperature type HOTSPOT is: {}".format(
                temperature_measure))

            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.VRAM, AmdSmiTemperatureMetric.CRITICAL)
            print("  Temperature limit for temperature type VRAM is: {}".format(
                temperature_measure))

            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.EDGE, AmdSmiTemperatureMetric.SHUTDOWN)
            print("  Temperature shutdown for temperature type EDGE is: {}".format(
                temperature_measure))

            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.HOTSPOT, AmdSmiTemperatureMetric.SHUTDOWN)
            print("  Temperature shutdown for temperature type HOTSPOT is: {}".format(
                temperature_measure))

            temperature_measure = amdsmi_get_temp_metric(
                processors[i], AmdSmiTemperatureType.VRAM, AmdSmiTemperatureMetric.SHUTDOWN)
            print("  Temperature shutdown for temperature type VRAM is: {}".format(
                temperature_measure))

            print("###Test amdsmi_get_gpu_vbios_info ")
            vbios_info = amdsmi_get_gpu_vbios_info(processors[i])
            print("  vbios_info['part_number'] is: {}".format(
                vbios_info['part_number']))
            print("  vbios_info['build_date'] is: {}".format(
                vbios_info['build_date']))
            print("  vbios_info['version'] is: {}\n".format(
                vbios_info['version']))
            print("  vbios_info['version'] is: {}\n".format(
                vbios_info['version']))
            print("  vbios_info['boot_firmware'] is: {}\n".format(
                vbios_info['boot_firmware']))

            print("###Test amdsmi_get_num_vf")
            num_vf = amdsmi_get_num_vf(processors[i])
            print("  Number of enabled VFs is: {}\n".format(num_vf['num_vf_enabled']))
            print("  Number of supported VFs is: {}\n".format(num_vf['num_vf_supported']))

            print("###Test amdsmi_get_vf_partition_info ####")
            partitions = amdsmi_get_vf_partition_info(processors[i])
            for j in range(0, len(partitions)):
                print("  Partition {}: 'fb_size' {}'".format(
                    j, partitions[j]['fb']['fb_size']))

                print("     ###Test amdsmi_get_vf_info")
                config = amdsmi_get_vf_info(partitions[j]['vf_id'])
                print("         config['gfx_timeslice'] is: {}".format(
                    config['gfx_timeslice']))
                print("         config['fb']['fb_size'] is: {}".format(
                    config['fb']['fb_size']))

                print("     ###Test amdsmi_get_vf_uuid")
                uuid = amdsmi_get_vf_uuid(partitions[j]['vf_id'])
                print("         uuid: {}".format(uuid))

                print("     ###Test amdsmi_get_vf_bdf ")
                bdf = amdsmi_get_vf_bdf(partitions[j]['vf_id'])
                print("         bdf: {}".format(bdf))

                print("     ###Test amdsmi_get_vf_data ")
                vf_data = amdsmi_get_vf_data(partitions[j]['vf_id'])
                print("         sched_info[state]: {} \n".format(vf_data['sched']['state']))
    except AmdSmiException as e:
        print(e)
    finally:
        try:
            amdsmi_shut_down()
        except AmdSmiException as e:
            print(e)
