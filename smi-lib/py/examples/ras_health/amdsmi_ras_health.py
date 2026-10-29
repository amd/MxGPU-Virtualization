# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amdsmi import *
from example_utils import get_optional_api

from datetime import datetime

# ECC APIs return the same three counter types. Keep their formatting in one
# helper so total and per-block counts are presented consistently.
def print_ecc_count(error_count):
    print("  correctable_count is: {}".format(
        error_count['correctable_count']))
    print("  uncorrectable_count is: {}".format(
        error_count['uncorrectable_count']))
    print("  deferred_count is: {}".format(error_count['deferred_count']))


def format_timestamp(ts):
    """Convert the bad-page record's Unix timestamp to a readable local time."""
    # A RAS record can contain a value the local platform cannot represent,
    # so preserve that raw value instead of terminating the example.
    try:
        return datetime.fromtimestamp(ts).strftime('%Y/%m/%d %H:%M:%S')
    except (OverflowError, OSError, ValueError):
        return "unavailable (raw value {})".format(ts)


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

        # RAS health is reported independently for every discovered GPU.
        for i, processor in enumerate(processors):
            # Display the PCI BDF to identify the GPU associated with the
            # health information that follows.
            print("###Test amdsmi_get_gpu_device_bdf")
            bdf = amdsmi_get_gpu_device_bdf(processor)
            print("  Processor number: {} bdf: {}\n".format(i, bdf))

            # Read the aggregate ECC counters across all GPU blocks.
            # Correctable errors were repaired, uncorrectable errors were not,
            # and deferred errors are pending further handling.
            print("###Test amdsmi_get_gpu_total_ecc_count")
            total_ecc_count = get_optional_api(
                "amdsmi_get_gpu_total_ecc_count",
                amdsmi_get_gpu_total_ecc_count, processor)
            if total_ecc_count is not None:
                print_ecc_count(total_ecc_count)
            print("")

            # Determine which hardware blocks have ECC reporting enabled.
            # This result also controls the per-block queries below.
            print("###Test amdsmi_get_gpu_ecc_enabled")
            ecc_enabled = get_optional_api(
                "amdsmi_get_gpu_ecc_enabled", amdsmi_get_gpu_ecc_enabled,
                processor)
            if ecc_enabled is not None:
                for block, enabled in ecc_enabled.items():
                    print("  ECC for {} is: {}".format(
                        block.name, "enabled" if enabled else "disabled"))
            print("")

            # Query individual block counters only where ECC is enabled.
            # This helps identify the source of errors included in the total count.
            print("###Test amdsmi_get_gpu_ecc_count")
            if ecc_enabled is not None:
                enabled_blocks = [block for block, enabled in ecc_enabled.items()
                                if enabled]
                if len(enabled_blocks) == 0:
                    print("  ECC is not enabled for any GPU block")
                # A block can have ECC enabled without supporting a counter
                # read, so group the failures and report them after the loop
                # instead of once per block. This improves the readability of the output.
                failed_blocks = {}
                for block in enabled_blocks:
                    try:
                        block_ecc_count = amdsmi_get_gpu_ecc_count(
                            processor, block)
                    except AmdSmiLibraryException as e:
                        failed_blocks.setdefault(e.err_info, []).append(
                            block.name)
                        continue
                    print("  {} ECC errors:".format(block.name))
                    print_ecc_count(block_ecc_count)
                for err_info, block_names in failed_blocks.items():
                    print("  amdsmi_get_gpu_ecc_count failed for {}: {}".format(
                        ", ".join(block_names), err_info))
            print("")

            # Read the RAS EEPROM version and the correction mechanisms the
            # device advertises. ecc_correction_schema is a bitmask, so test
            # each enum value to produce a readable list of supported schemas.
            print("###Test amdsmi_get_gpu_ras_feature_info")
            ras_feature = get_optional_api(
                "amdsmi_get_gpu_ras_feature_info",
                amdsmi_get_gpu_ras_feature_info, processor)
            if ras_feature is not None:
                print("  ras_eeprom_version is: {}".format(
                    ras_feature['ras_eeprom_version']))
                print("  ecc_correction_schema is: {}".format(
                    ras_feature['ecc_correction_schema']))
                supported_schemas = []
                for schema in AmdSmiEccCorrectionSchemaSupport:
                    if ras_feature['ecc_correction_schema'] & schema.value:
                        supported_schemas.append(schema.name)
                print("  supported ECC correction schemas are: {}\n".format(
                    supported_schemas))
            print("")

            # Read the versioned RAS policy stored by supported PMFW-managed
            # RAS EEPROM implementations.
            print("###Test amdsmi_get_gpu_ras_policy_info")
            ras_policy = get_optional_api(
                "amdsmi_get_gpu_ras_policy_info",
                amdsmi_get_gpu_ras_policy_info, processor)
            if ras_policy is not None:
                policy_info = ras_policy['ras_policy_info']
                print("  major_version is: {}".format(
                    policy_info['major_version']))
                print("  minor_version is: {}".format(
                    policy_info['minor_version']))
                if 'dram_non_critical_region_threshold' in policy_info:
                    print("  dram_non_critical_region_threshold is: {}".format(
                        policy_info['dram_non_critical_region_threshold']))
                    print("  dram_critical_region_threshold is: {}".format(
                        policy_info['dram_critical_region_threshold']))
            print("")

            # Read the number of retired memory pages at which the device is
            # considered unhealthy. A retired page is no longer available for
            # GPU memory allocations.
            print("###Test amdsmi_get_bad_page_threshold")
            bad_page_threshold = get_optional_api(
                "amdsmi_get_bad_page_threshold", amdsmi_get_bad_page_threshold,
                processor)
            if bad_page_threshold is not None:
                print("  bad page threshold is: {}".format(
                    bad_page_threshold))
            print("")

            # List every retired memory page. In addition to its address, each
            # record identifies when and where the page failure was recorded.
            print("###Test amdsmi_get_gpu_bad_page_info")
            bad_pages = get_optional_api(
                "amdsmi_get_gpu_bad_page_info", amdsmi_get_gpu_bad_page_info,
                processor)
            if bad_pages is not None:
                if len(bad_pages) == 0:
                    print("  No bad pages found")
                for bad_page in bad_pages:
                    print("  retired_page is: {}".format(
                        hex(bad_page['retired_page'])))
                    print("  timestamp is: {}".format(
                        format_timestamp(bad_page['ts'])))
                    print("  mem_channel is: {}".format(
                        bad_page['mem_channel']))
                    print("  mcumc_id is: {}".format(bad_page['mcumc_id']))
            print("")

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