# Run the AMD SMI event monitoring example

The event monitoring example shows how to monitor AMD GPU events. It discovers
every GPU, subscribes to all event categories and severity levels, reads up to
five events, and prints each event's details.

`go/examples/events/amdsmi_events.go` is the Go implementation. It uses
`amdsmi.EventReader`.

The example is read-only: it does not reset a GPU, change device
configuration, or generate events. On a quiet system, receiving no event
before the timeout is a normal result.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. At least one AMD GPU must be available.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/events/amdsmi_events.go
```

The example waits up to 10 seconds for each event and stops after the first
timeout or after it prints five events. Do not reset a GPU or otherwise
disrupt workloads merely to create an event for this example.

## Example flow

The example initializes AMD SMI, discovers GPU processor handles, creates an
event reader, and then releases both the event reader and AMD SMI resources:

```go
initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS
if err := amdsmi.Init(initFlags); err != nil {
	return err
}
defer func() {
	if err := amdsmi.ShutDown(); err != nil {
		fmt.Println(err)
	}
}()

processors, err := amdsmi.GetProcessorHandles()
if err != nil {
	return err
}

const (
	eventTimeoutUsec = 10 * 1000 * 1000
	maxEvents        = 5
)

reader, err := amdsmi.NewEventReaderMask(
	processors,
	amdsmi.AMDSMI_MASK_ALL,
)
if err != nil {
	return err
}
defer reader.Close()

for eventIndex := 0; eventIndex < maxEvents; eventIndex++ {
	event, err := reader.Read(eventTimeoutUsec)
	if err != nil {
		var statusErr *amdsmi.StatusError
		if errors.As(err, &statusErr) &&
			statusErr.Code == amdsmi.AMDSMI_STATUS_TIMEOUT {
			fmt.Println("No matching events occurred before the timeout")
			break
		}
		return err
	}

	printEvent(event)
}
```

`amdsmi.GetProcessorHandles()` returns the GPU handles passed to
`amdsmi.NewEventReaderMask()`. The full example also prints every discovered
GPU's PCI bus:device.function (BDF) address before beginning event monitoring.
The runnable program reads through `readEventWithContext()` so a keyboard
interrupt can cancel a blocking wait; the snippet above shows the equivalent
`reader.Read()` call.

### Configure the event subscription

The example subscribes to all categories and all severity levels:

```go
reader, err := amdsmi.NewEventReaderMask(
	processors,
	amdsmi.AMDSMI_MASK_ALL,
)
```

`amdsmi.AMDSMI_MASK_ALL` enables every category bit and every severity bit.
Use `amdsmi.NewEventReader()` with specific `amdsmi.EventCategory` values when
only a subset is relevant. For example, this subscription monitors reset and
ECC events at warning severity:

```go
reader, err := amdsmi.NewEventReader(
	processors,
	[]amdsmi.EventCategory{
		amdsmi.AMDSMI_EVENT_CATEGORY_RESET,
		amdsmi.AMDSMI_EVENT_CATEGORY_ECC,
	},
	amdsmi.AMDSMI_EVENT_SEVERITY_WARN,
)
```

`amdsmi.EventCategory` provides these categories:

| Category | Events monitored |
| --- | --- |
| `AMDSMI_EVENT_CATEGORY_NON_USED` | Reserved category; do not use for a subscription. |
| `AMDSMI_EVENT_CATEGORY_DRIVER` | Driver allocation, API failure, and diagnostic events. |
| `AMDSMI_EVENT_CATEGORY_RESET` | GPU reset and function-level reset events. |
| `AMDSMI_EVENT_CATEGORY_SCHED` | Scheduling and world-switch events. |
| `AMDSMI_EVENT_CATEGORY_VBIOS` | VBIOS validation, loading, and security events. |
| `AMDSMI_EVENT_CATEGORY_ECC` | ECC and RAS-related events. |
| `AMDSMI_EVENT_CATEGORY_PP` | Power-management and performance-policy events. |
| `AMDSMI_EVENT_CATEGORY_IOV` | SR-IOV configuration and resource events. |
| `AMDSMI_EVENT_CATEGORY_VF` | Virtual function events. |
| `AMDSMI_EVENT_CATEGORY_FW` | Firmware load and operation events. |
| `AMDSMI_EVENT_CATEGORY_GPU` | GPU fatal-condition events. |
| `AMDSMI_EVENT_CATEGORY_GUARD` | GPU guard events. |
| `AMDSMI_EVENT_CATEGORY_GPUMON` | GPU monitor and framebuffer events. |
| `AMDSMI_EVENT_CATEGORY_MMSCH` | Multimedia scheduler events. |
| `AMDSMI_EVENT_CATEGORY_XGMI` | XGMI topology and framebuffer-sharing events. |

Go has no `ALL` category or severity enumerator. Use
`amdsmi.AMDSMI_MASK_ALL` (as the example does),
`amdsmi.NewEventReaderAllCategories()`, or
`amdsmi.BuildEventMaskAllSeverities()` to subscribe to every category and
severity.

`amdsmi.EventSeverity` controls which severity levels are requested:

| Severity | Meaning |
| --- | --- |
| `AMDSMI_EVENT_SEVERITY_HIGH` | Critical error |
| `AMDSMI_EVENT_SEVERITY_MED` | Significant error |
| `AMDSMI_EVENT_SEVERITY_LOW` | Low-severity error |
| `AMDSMI_EVENT_SEVERITY_WARN` | Warning |
| `AMDSMI_EVENT_SEVERITY_INFO` | Informational event |

### Read events with a timeout

`amdsmi.EventReader.Read()` accepts its timeout in microseconds. The example
uses `eventTimeoutUsec = 10 * 1000 * 1000`, which waits for at most 10 seconds
per read.

The runnable program does not call `Read()` with the full 10-second timeout in
one step. It uses `readEventWithContext()` so `Ctrl+C` can cancel a wait:
each poll uses `eventPollUsec = 500 * 1000` (500 milliseconds) until the
remaining timeout is exhausted.

```go
event, err := readEventWithContext(ctx, reader, eventTimeoutUsec)
if err != nil {
	if errors.Is(err, context.Canceled) {
		return err
	}
	var statusErr *amdsmi.StatusError
	if errors.As(err, &statusErr) &&
		statusErr.Code == amdsmi.AMDSMI_STATUS_TIMEOUT {
		fmt.Println("No matching events occurred before the timeout")
		return nil
	}
	return err
}
```

If no selected event arrives before the timeout, `Read()` returns an
`amdsmi.StatusError` whose `Code` is `amdsmi.AMDSMI_STATUS_TIMEOUT`. The
example treats this as normal completion rather than an error. Lower timeout
values reduce how long the program waits; the native API uses a minimum
timeout of 1,000 microseconds.

### Inspect event output

Each successful `Read()` call returns an `amdsmi.EventEntry` with these
fields:

| Field | Meaning |
| --- | --- |
| `FcnId` | Virtual function handle associated with the event, when present. |
| `DevId` | GPU device identifier as a `uint64`. |
| `Timestamp` | UTC event timestamp in microseconds. |
| `Data` | Event-specific numeric data. |
| `Category` | Event category as an `amdsmi.EventCategory` value. |
| `Subcode` | Category-specific event code as a `uint32`. |
| `Level` | Event severity as an `amdsmi.EventSeverity` value. |
| `Date` | UTC date and time string. |
| `Message` | Human-readable event description. |
| `ProcessorHandle` | GPU processor handle for the device that originated the event. |

The program prints the category and level with their `String()` methods. It
resolves `Subcode` through `amdsmi.SubcodeName()` because the field is a raw
`uint32`. It then resolves `ProcessorHandle` to a BDF so the event can be
matched to a physical GPU:

```go
fmt.Printf("  event['subcode'] is: %s\n",
	amdsmi.SubcodeName(event.Category, event.Subcode))

processorBdf, err := amdsmi.GetProcessorBdf(event.ProcessorHandle)
if err != nil {
	return err
}
fmt.Printf("  event processor BDF is: %s\n", processorBdf)
```

### Clean up the event reader

`amdsmi.EventReader` allocates native event-monitoring resources. The example
releases them with `Close()` in a `defer`:

```go
reader, err := amdsmi.NewEventReaderMask(
	processors,
	amdsmi.AMDSMI_MASK_ALL,
)
if err != nil {
	return err
}
defer reader.Close()

event, err := reader.Read(eventTimeoutUsec)
```

`Close()` calls `amdsmi.EventDestroy()` on the underlying event set. The
outer `defer` then calls `amdsmi.ShutDown()` to release the AMD SMI library
resources when `run()` returns, including after a timeout or another error.

Other AMD SMI failures are reported in `main` as a status name and its
human-readable description from `amdsmi.StatusCodeToString()`.
