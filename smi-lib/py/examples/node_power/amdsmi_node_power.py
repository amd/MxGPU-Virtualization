# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amdsmi import *
from example_utils import get_optional_api


# Node Power Management (NPM) controls the power budget for an entire node.
# This example discovers each GPU, maps it to its node, and reports the node's current NPM settings.
# NPM settings are configured out of band (for example, through the BMC), so this example does not modify them.
def print_node_power_info(processor, processor_number):
    # Translate the processor handle before querying NPM.
    print("###Test amdsmi_get_node_handle")
    # Node-level APIs are not supported on every platform.
    # get_optional_api() reports an unsupported API without terminating the rest of the example.
    node_handle = get_optional_api(
        "amdsmi_get_node_handle", amdsmi_get_node_handle, processor)
    if node_handle is None:
        # Without a node handle there is no valid input for the NPM query.
        print("")
        return
    print("  Node handle for GPU {} is: {}".format(
        processor_number, node_handle))

    # Query the NPM controller for the node associated with this GPU.
    # As with node-handle discovery, this API may be unavailable on some platforms.
    print("###Test amdsmi_get_npm_info")
    npm_info = get_optional_api(
        "amdsmi_get_npm_info", amdsmi_get_npm_info, node_handle)
    if npm_info is not None:
        # The result is a dictionary containing:
        #   status              - whether NPM is enabled or disabled
        #   limit               - the current node power limit in watts
        #   ubb_power_threshold - the upper UBB node threshold in watts (not supported on all platforms)
        print("  npm_info['status'] is: {}".format(npm_info['status']))
        print("  npm_info['limit'] is: {} W".format(npm_info['limit']))
        print("  npm_info['ubb_power_threshold'] is: {} W".format(
            npm_info['ubb_power_threshold']))
    print("")


if __name__ == "__main__":
    initialized = False

    try:
        # Initialize AMD SMI for all processor types so GPU handles can be
        # discovered. A node handle is obtained from each discovered GPU.
        init_flags = AmdSmiInitFlags.INIT_ALL_PROCESSORS
        print("###Test amdsmi_init")
        print("  Initializing with {}".format(init_flags.name))
        amdsmi_init(init_flags)
        initialized = True
        print("  Initialization complete")

        # Retrieve one opaque processor handle for every GPU visible to AMD
        # SMI. These handles are required by the remaining GPU-level APIs.
        print("###Test amdsmi_get_processor_handles")
        processors = amdsmi_get_processor_handles()
        print("  number of GPUs is: {}\n".format(len(processors)))

        if len(processors) == 0:
            print("  No GPUs found")

        # Show the PCI BDF to identify each physical GPU, then report the NPM
        # information for the node to which that GPU belongs. Multiple GPUs
        # can belong to the same node and therefore report the same settings.
        for processor_number, processor in enumerate(processors):
            print("###Test amdsmi_get_gpu_device_bdf")
            bdf = amdsmi_get_gpu_device_bdf(processor)
            print("  Processor number: {} bdf: {}\n".format(
                processor_number, bdf))
            print_node_power_info(processor, processor_number)

    # Library exceptions contain an AMD SMI return code. Convert it to a
    # readable status string so the user can diagnose the failed API call.
    except AmdSmiLibraryException as e:
        status = AmdSmiRetCode(e.err_code)
        print(amdsmi_status_code_to_string(status))
    # Other AMD SMI exceptions provide a user-readable message.
    except AmdSmiException as e:
        print(e)
    finally:
        # Shut down after a successful init. Each successful amdsmi_init
        # needs one amdsmi_shut_down.
        if initialized:
            try:
                print("###Test amdsmi_shut_down")
                amdsmi_shut_down()
                print("Shutdown complete")
            except AmdSmiException as e:
                print(e)
