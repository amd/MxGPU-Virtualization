# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from amdsmi import *

# read() waits this many microseconds for one matching event.
# Ten seconds is long enough to observe a timeout on a quiet system.
EVENT_TIMEOUT_US = 10 * 1000 * 1000
# Return after a few events so the example finishes even when events
# keep arriving. Increase this limit to follow a longer stream.
MAX_EVENTS = 5


def print_event(event):
    # timestamp is the UTC time of the event in microseconds.
    # date is that same moment as a readable UTC string.
    print("  event['timestamp'] is: {}".format(event['timestamp']))
    print("  event['date'] is: {}".format(event['date']))

    # category is the event family, such as RESET or ECC.
    # subcode is the specific event inside that family.
    # level is the severity.
    # Print the enum names so the values can be read without a numeric lookup.
    print("  event['category'] is: {}".format(event['category'].name))
    print("  event['subcode'] is: {}".format(event['subcode'].name))
    print("  event['level'] is: {}".format(event['level'].name))

    # data is a numeric payload whose meaning depends on the subcode.
    print("  event['data'] is: {}".format(event['data']))
    # dev_id identifies the processor. fcn_id is the virtual function
    # associated with the event when the event belongs to a VF.
    print("  event['dev_id'] is: {}".format(event['dev_id']))
    print("  event['fcn_id'] is: {}".format(event['fcn_id']))
    # message is the description of what happened.
    print("  event['message'] is: {}".format(event['message']))

    # The event returns the processor handle that originated the event.
    # Resolve it to a BDF so it can be matched to a physical GPU.
    processor_bdf = amdsmi_get_processor_bdf(event['processor_handle'])
    print("  event processor BDF is: {}".format(processor_bdf))


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
        else:
            # Print each processor's BDF before monitoring so a later event
            # can be matched to one of these processors.
            for i, processor in enumerate(processors):
                print("###Test amdsmi_get_gpu_device_bdf")
                bdf = amdsmi_get_gpu_device_bdf(processor)
                print("  Processor number: {} bdf: {}".format(i, bdf))
            print("")

            # Monitor every category and every severity.
            # event_categories is a list, so specific AmdSmiEventCategory values can be combined.
            # Set event_severity to one AmdSmiEventSeverity value, or ALL, to choose which severities are delivered.
            event_categories = [AmdSmiEventCategory.ALL]
            event_severity = AmdSmiEventSeverity.ALL

            print("###Test AmdSmiEventReader")
            print("  Monitoring categories: {}".format(
                [category.name for category in event_categories]))
            print("  Monitoring severity: {}".format(event_severity.name))

            # The reader watches the discovered processors. The context manager
            # calls stop() and releases native event resources when the
            # block exits, including after a timeout or an error.
            with AmdSmiEventReader(
                    processors, event_categories, event_severity) as event_reader:
                print("###Test AmdSmiEventReader.read")
                print("  Waiting up to {} seconds for each event".format(
                    EVENT_TIMEOUT_US // (1000 * 1000)))

                # read() blocks until one matching event arrives or the
                # timeout elapses. A timeout means no selected event
                # arrived; on a quiet system that is a normal result.
                for event_index in range(MAX_EVENTS):
                    try:
                        event = event_reader.read(EVENT_TIMEOUT_US)
                    except AmdSmiTimeoutException:
                        print("  No matching events occurred before the timeout")
                        break

                    print("  Event number: {}".format(event_index + 1))
                    print_event(event)
                    print("")

            print("  AmdSmiEventReader.stop called automatically")

    except AmdSmiLibraryException as e:
        # Library failures carry a numeric status.
        # Translate it to the status name and a readable description.
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
