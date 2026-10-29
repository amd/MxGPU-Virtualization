# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amdsmi import *

if __name__ == "__main__":
    initialized = False

    try:
        # Call amdsmi_init before any other AMD SMI API. INIT_ALL_PROCESSORS
        # is the default. Pass a different AmdSmiInitFlags value, or OR
        # several together, when only specific processor types are needed.
        init_flags = AmdSmiInitFlags.INIT_ALL_PROCESSORS
        print("###Test amdsmi_init")
        print("  Initializing with {}".format(init_flags.name))
        amdsmi_init(init_flags)
        initialized = True
        print("  Initialization complete")

        # major, minor, and release describe the loaded library build.
        # This call takes no processor handle.
        print("###Test amdsmi_get_lib_version")
        lib_data = amdsmi_get_lib_version()
        print("  lib_data['major'] is: {}".format(lib_data['major']))
        print("  lib_data['minor'] is: {}".format(lib_data['minor']))
        print("  lib_data['release'] is: {}".format(lib_data['release']))

        # A failed library call raises AmdSmiLibraryException. Use
        # amdsmi_status_code_to_string to turn an AmdSmiRetCode into the
        # library's description.
        print("###Test amdsmi_status_code_to_string")
        status = AmdSmiRetCode.SUCCESS
        print("  {} is: {}".format(
            status.name, amdsmi_status_code_to_string(status)))
        status = AmdSmiRetCode.STATUS_NOT_INIT
        print("  {} is: {}".format(
            status.name, amdsmi_status_code_to_string(status)))

    # Handle AmdSmiLibraryException first; it is also an AmdSmiException.
    except AmdSmiLibraryException as e:
        # Library failures carry err_code. Convert it the same way as the
        # sample status values above.
        status = AmdSmiRetCode(e.err_code)
        print(amdsmi_status_code_to_string(status))

    except AmdSmiException as e:
        # Binding errors, such as a bad argument type, already include a
        # readable message.
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
        