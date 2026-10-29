# Run the AMD SMI event monitoring example

The event monitoring example shows how to monitor AMD GPU events. It discovers
every GPU, subscribes to all event categories and severity levels, reads up to
five events, and prints each event's details.

`py/examples/events/amdsmi_events.py` is the Python implementation. It uses
`AmdSmiEventReader`.

The example is read-only: it does not reset a GPU, change device
configuration, or generate events. On a quiet system, receiving no event
before the timeout is a normal result.

## Prerequisites

Review the [shared prerequisites for AMD SMI Python examples](../../../docs/how_to/amdsmi_py_lib.md#prerequisites)
before running this script. At least one AMD GPU must be available.

## Run the example

From the `smi-lib` repository root, run:

```shell
sudo python3 py/examples/events/amdsmi_events.py
```

The example waits up to 10 seconds for each event and stops after the first
timeout or after it prints five events. Do not reset a GPU or otherwise
disrupt workloads merely to create an event for this example.

## Example flow

The example initializes AMD SMI, discovers GPU processor handles, creates an
event reader, and then releases both the event reader and AMD SMI resources:

```python
from amdsmi import *

try:
    amdsmi_init(AmdSmiInitFlags.INIT_ALL_PROCESSORS)
    processors = amdsmi_get_processor_handles()

    EVENT_TIMEOUT_US = 10 * 1000 * 1000
    MAX_EVENTS = 5
    with AmdSmiEventReader(
            processors,
            [AmdSmiEventCategory.ALL],
            AmdSmiEventSeverity.ALL) as event_reader:
        for event_index in range(MAX_EVENTS):
            try:
                event = event_reader.read(EVENT_TIMEOUT_US)
            except AmdSmiTimeoutException:
                print("No matching events occurred before the timeout")
                break

            print_event(event)
finally:
    amdsmi_shut_down()
```

`amdsmi_get_processor_handles()` returns the GPU handles passed to
`AmdSmiEventReader`. The full example also prints every discovered GPU's PCI
bus:device.function (BDF) address before beginning event monitoring.

### Configure the event subscription

The example subscribes to all categories and all severity levels:

```python
event_categories = [AmdSmiEventCategory.ALL]
event_severity = AmdSmiEventSeverity.ALL
```

Use a list of specific `AmdSmiEventCategory` values when only a subset is
relevant. For example, this subscription monitors reset and ECC events:

```python
event_categories = [
    AmdSmiEventCategory.RESET,
    AmdSmiEventCategory.ECC,
]
event_severity = AmdSmiEventSeverity.WARN
```

`AmdSmiEventCategory` provides these categories:

| Category | Events monitored |
| --- | --- |
| `NOT_USED` | Reserved category; do not use for a subscription. |
| `DRIVER` | Driver allocation, API failure, and diagnostic events. |
| `RESET` | GPU reset and function-level reset events. |
| `SCHED` | Scheduling and world-switch events. |
| `VBIOS` | VBIOS validation, loading, and security events. |
| `ECC` | ECC and RAS-related events. |
| `PP` | Power-management and performance-policy events. |
| `IOV` | SR-IOV configuration and resource events. |
| `VF` | Virtual function events. |
| `FW` | Firmware load and operation events. |
| `GPU` | GPU fatal-condition events. |
| `GUARD` | GPU guard events. |
| `GPUMON` | GPU monitor and framebuffer events. |
| `MMSCH` | Multimedia scheduler events. |
| `XGMI` | XGMI topology and framebuffer-sharing events. |
| `ALL` | Every supported event category. |

`AmdSmiEventSeverity` controls which severity levels are requested:

| Severity | Meaning |
| --- | --- |
| `HIGH` | Critical error |
| `MED` | Significant error |
| `LOW` | Low-severity error |
| `WARN` | Warning |
| `INFO` | Informational event |
| `ALL` | Every severity level |

### Read events with a timeout

`AmdSmiEventReader.read()` accepts its timeout in microseconds. The example
uses `EVENT_TIMEOUT_US = 10 * 1000 * 1000`, which waits for at most 10 seconds
per read:

```python
EVENT_TIMEOUT_US = 10 * 1000 * 1000
MAX_EVENTS = 5

for event_index in range(MAX_EVENTS):
    try:
        event = event_reader.read(EVENT_TIMEOUT_US)
    except AmdSmiTimeoutException:
        print("No matching events occurred before the timeout")
        break

    print_event(event)
```

If no selected event arrives before the timeout, `read()` raises
`AmdSmiTimeoutException`. The example treats this as normal completion rather
than an error. Lower timeout values reduce how long the script waits; the
native API uses a minimum timeout of 1,000 microseconds.

### Inspect event output

Each successful `read()` call returns a dictionary with these fields:

| Field | Meaning |
| --- | --- |
| `fcn_id` | Virtual function handle associated with the event, when present. |
| `dev_id` | GPU device identifier, formatted as a hexadecimal string by the Python wrapper. |
| `timestamp` | UTC event timestamp in microseconds. |
| `data` | Event-specific numeric data. |
| `category` | Event category as an `AmdSmiEventCategory` value. |
| `subcode` | Category-specific event code, such as an `AmdSmiEventCategoryReset` value. |
| `level` | Event severity as an `AmdSmiEventSeverity` value. |
| `date` | UTC date and time string. |
| `message` | Human-readable event description. |
| `processor_handle` | GPU processor handle for the device that originated the event. |

The script prints the category, subcode, and level by name. It resolves
`processor_handle` to a BDF so the event can be matched to a physical GPU:

```python
processor_bdf = amdsmi_get_processor_bdf(event['processor_handle'])
print("event processor BDF is: {}".format(processor_bdf))
```

### Clean up the event reader

`AmdSmiEventReader` allocates native event-monitoring resources. The example
uses it as a context manager:

```python
with AmdSmiEventReader(
        processors, event_categories, event_severity) as event_reader:
    event = event_reader.read(EVENT_TIMEOUT_US)
```

When the `with` block exits, including after a timeout or another exception,
the context manager calls `event_reader.stop()` to release those resources.
The outer `finally` block then calls `amdsmi_shut_down()` to release the AMD
SMI library resources.

Other AMD SMI failures are reported by the outer exception handler as a status
name and its human-readable description.
