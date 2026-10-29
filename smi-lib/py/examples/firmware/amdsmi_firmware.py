# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amdsmi import *
from example_utils import get_optional_api, prompt_for_input


def format_firmware_version(fw_id, fw_version):
    """Convert a raw firmware version to the format used by amd-smi."""
    # Firmware versions are returned as packed 32-bit values. Each firmware
    # block defines how those bits map to human-readable version components.
    fw_version &= 0xFFFFFFFF

    # SMU uses four decimal bytes: major.minor.debug.test.
    if fw_id == AmdSmiFwBlock.FW_ID_SMU:
        return "{}.{}.{}.{}".format(
            (fw_version >> 24) & 0xFF,
            (fw_version >> 16) & 0xFF,
            (fw_version >> 8) & 0xFF,
            fw_version & 0xFF)
    # MMSCH reserves the lower 16 bits for the final version component.
    if fw_id == AmdSmiFwBlock.FW_ID_MMSCH:
        return "{}.{}.{}".format(
            (fw_version >> 24) & 0xFF,
            (fw_version >> 16) & 0xFF,
            fw_version & 0xFFFF)
    # UVD uses fields of 2, 6, 8, and 8 bits; unused bits are ignored.
    if fw_id == AmdSmiFwBlock.FW_ID_UVD:
        return "{}.{}.{}.{}".format(
            (fw_version >> 30) & 0x3,
            (fw_version >> 24) & 0x3F,
            (fw_version >> 8) & 0xFF,
            fw_version & 0xFF)
    # VCE encodes two 12-bit components followed by one 8-bit component.
    if fw_id == AmdSmiFwBlock.FW_ID_VCE:
        return "{}.{}.{}".format(
            (fw_version >> 20) & 0xFFF,
            (fw_version >> 8) & 0xFFF,
            fw_version & 0xFF)
    # PSP-related firmware reports four hexadecimal byte components.
    if fw_id in {
            AmdSmiFwBlock.FW_ID_PSP_SYSDRV,
            AmdSmiFwBlock.FW_ID_PSP_SOSDRV,
            AmdSmiFwBlock.FW_ID_PSP_KEYDB,
            AmdSmiFwBlock.FW_ID_DFC,
            AmdSmiFwBlock.FW_ID_PSP_SPL,
            AmdSmiFwBlock.FW_ID_PSP_BL,
            AmdSmiFwBlock.FW_ID_REG_ACCESS_WHITELIST,
            AmdSmiFwBlock.FW_ID_PSP_SOC,
            AmdSmiFwBlock.FW_ID_PSP_DBG,
            AmdSmiFwBlock.FW_ID_PSP_INTF,
            AmdSmiFwBlock.FW_ID_TA_RAS,
            AmdSmiFwBlock.FW_ID_TA_XGMI,
            AmdSmiFwBlock.FW_ID_PSP_RAS,
            AmdSmiFwBlock.FW_ID_PSP_IPKEYMGR,
            AmdSmiFwBlock.FW_ID_PSP_IOVM,
            AmdSmiFwBlock.FW_ID_PSP_SPDM,
            AmdSmiFwBlock.FW_ID_PSP_DPE,
    }:
        return "{:x}.{:x}.{:x}.{:x}".format(
            (fw_version >> 24) & 0xFF,
            (fw_version >> 16) & 0xFF,
            (fw_version >> 8) & 0xFF,
            fw_version & 0xFF)
    # VCN uses two 4-bit, one 8-bit, and one 12-bit decimal component.
    if fw_id == AmdSmiFwBlock.FW_ID_VCN:
        return "{}.{}.{}.{}".format(
            (fw_version >> 24) & 0xF,
            (fw_version >> 20) & 0xF,
            (fw_version >> 12) & 0xFF,
            fw_version & 0xFFF)
    # IMU and PLDM versions use the same four-byte decimal layout as SMU.
    if fw_id in {
            AmdSmiFwBlock.FW_ID_IMU_DRAM,
            AmdSmiFwBlock.FW_ID_IMU_IRAM,
            AmdSmiFwBlock.FW_ID_PLDM_BUNDLE,
    }:
        return "{}.{}.{}.{}".format(
            (fw_version >> 24) & 0xFF,
            (fw_version >> 16) & 0xFF,
            (fw_version >> 8) & 0xFF,
            fw_version & 0xFF)
    # RLC restore-list versions are displayed as unpadded hexadecimal values.
    if fw_id in {
            AmdSmiFwBlock.FW_ID_RLC_RESTORE_LIST_GPM_MEM,
            AmdSmiFwBlock.FW_ID_RLC_RESTORE_LIST_SRM_MEM,
            AmdSmiFwBlock.FW_ID_RLC_RESTORE_LIST_CNTL,
    }:
        return "0x{:x}".format(fw_version)
    # MES versions are always padded to the full eight hexadecimal digits.
    if fw_id in {
            AmdSmiFwBlock.FW_ID_CP_MES,
            AmdSmiFwBlock.FW_ID_MES_STACK,
            AmdSmiFwBlock.FW_ID_MES_THREAD1,
            AmdSmiFwBlock.FW_ID_MES_THREAD1_STACK,
    }:
        return "0x{:08x}".format(fw_version)
    # Preserve unknown or unlisted firmware versions as raw hexadecimal.
    return "0x{:x}".format(fw_version)


def print_firmware_info(fw_info, indent):
    """Print nonzero entries from a GPU or VF firmware inventory."""
    firmware_blocks = fw_info['fw_info_list']
    if len(firmware_blocks) == 0:
        print("{}No firmware blocks found".format(indent))
        return

    print("{}FW_LIST:".format(indent))
    for firmware_number, firmware in enumerate(firmware_blocks):
        fw_version = firmware['fw_version']
        # A zero version marks a firmware block that is not present.
        if fw_version == 0:
            continue

        # Remove the enum prefix to make the output easier to scan.
        fw_name = firmware['fw_id'].name.replace("FW_ID_", "", 1)
        print("{}  FW_{}:".format(indent, firmware_number))
        print("{}    FW_ID: {}".format(indent, fw_name))
        print("{}    FW_VERSION: {}".format(
            indent, format_firmware_version(firmware['fw_id'], fw_version)))


def print_fw_error_records(error_records):
    """Print firmware errors recorded by the GPU driver."""
    records = error_records['err_records']
    print("  number of firmware error records is: {}".format(len(records)))
    if len(records) == 0:
        print("  No firmware error records found")
        return

    for record_number, record in enumerate(records):
        print("  Firmware error record {}:".format(record_number))
        print("    timestamp is: {}".format(record['timestamp']))
        print("    vf_idx is: {}".format(record['vf_idx']))
        print("    fw_id is: {}".format(record['fw_id'].name))
        print("    status is: {}".format(record['status'].name))


def print_vf_firmware(processor):
    """Show firmware information for every enabled VF on a GPU."""
    # VF firmware information requires SR-IOV VFs to be enabled.
    print("###Test amdsmi_get_vf_partition_info")
    partitions = get_optional_api(
        "amdsmi_get_vf_partition_info", amdsmi_get_vf_partition_info,
        processor)
    if partitions is None:
        print("")
        return

    print("  number of enabled VFs is: {}".format(len(partitions)))
    if len(partitions) == 0:
        print("  No enabled VFs found; VF firmware queries require "
              "an enabled VF")
        print("")
        return

    # A partition contains the VF handle required by the VF firmware API.
    for vf_number, partition in enumerate(partitions):
        print("###Test amdsmi_get_vf_fw_info")
        fw_info = get_optional_api(
            "amdsmi_get_vf_fw_info for VF {}".format(vf_number),
            amdsmi_get_vf_fw_info, partition['vf_id'])
        if fw_info is not None:
            print("  VF number: {}".format(vf_number))
            if len(fw_info['fw_info_list']) == 0:
                print("  No firmware blocks found; the VM may not be "
                      "started on this VF")
            else:
                print_firmware_info(fw_info, "  ")
        print("")


def print_nic_firmware():
    """Select one or more optional NICs and show their firmware entries."""
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

    print("  Available NICs:")
    for i in range(len(nic_processors)):
        print("  NIC {} bdf: {}".format(
            i, amdsmi_get_nic_device_bdf(nic_processors[i])))
    all_nic_index = len(nic_processors)
    print("  {}: All NICs".format(all_nic_index))

    # The index immediately after the last NIC is used as the "All NICs"
    # menu choice.
    pick_nic_index = prompt_for_input(
        "  Enter the NIC index to inspect: ",
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

    for nic_index in selected_nic_indexes:
        nic = nic_processors[nic_index]
        # Print the BDF so users can associate the firmware with a physical
        # NIC in the system.
        print("###Test amdsmi_get_nic_device_bdf")
        nic_bdf = amdsmi_get_nic_device_bdf(nic)
        print("  Selected NIC {} bdf: {}\n".format(nic_index, nic_bdf))

        print("###Test amdsmi_get_nic_fw_info")
        fw_info = get_optional_api(
            "amdsmi_get_nic_fw_info for NIC {}".format(nic_index),
            amdsmi_get_nic_fw_info, nic)
        if fw_info is not None:
            print("  NIC number: {}".format(nic_index))
            if len(fw_info['fw']) == 0:
                print("  No NIC firmware entries found")
            for firmware in fw_info['fw']:
                print("  type is: {}".format(firmware['type'].name))
                print("  name is: {}".format(firmware['name']))
                print("  version is: {}".format(firmware['version']))
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

        # Discover GPU handles. Each handle is passed to the firmware APIs
        # below and remains valid until amdsmi_shut_down is called.
        print("###Test amdsmi_get_processor_handles")
        processors = amdsmi_get_processor_handles()
        print("  number of GPUs is: {}\n".format(len(processors)))

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
                "  Enter the GPU index to inspect: ",
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

                # VBIOS metadata identifies the board firmware image installed
                # on the selected GPU.
                print("###Test amdsmi_get_gpu_vbios_info")
                vbios_info = amdsmi_get_gpu_vbios_info(processor)
                print("  vbios_info['name'] is: {}".format(vbios_info['name']))
                print("  vbios_info['build_date'] is: {}".format(
                    vbios_info['build_date']))
                print("  vbios_info['part_number'] is: {}".format(
                    vbios_info['part_number']))
                print("  vbios_info['version'] is: {}".format(
                    vbios_info['version']))
                print("  vbios_info['boot_firmware'] is: {}".format(
                    vbios_info['boot_firmware']))
                print("")

                # The firmware inventory contains versions for the individual
                # firmware blocks running on the GPU.
                print("###Test amdsmi_get_fw_info")
                fw_info = amdsmi_get_fw_info(processor)
                print_firmware_info(fw_info, "  ")
                print("")

                # Firmware error records are an optional feature, so a missing
                # capability does not stop the example.
                print("###Test amdsmi_get_fw_error_records")
                error_records = get_optional_api(
                    "amdsmi_get_fw_error_records",
                    amdsmi_get_fw_error_records, processor)
                if error_records is not None:
                    print_fw_error_records(error_records)
                print("")

                print_vf_firmware(processor)

        # NIC firmware discovery is independent of whether GPUs were found.
        print_nic_firmware()

    except AmdSmiLibraryException as e:
        status = AmdSmiRetCode(e.err_code)
        print("  {} is: {}".format(
            status.name, amdsmi_status_code_to_string(status)))
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
