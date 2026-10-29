// Copyright Advanced Micro Devices, Inc.
//
// SPDX-License-Identifier: MIT

package main

import (
	"context"
	"errors"
	"fmt"
	"os"
	"os/signal"
	"syscall"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

const (
	// Read() waits this many microseconds for one matching event.
	// Ten seconds is long enough to observe a timeout on a quiet system.
	eventTimeoutUsec = 10 * 1000 * 1000
	eventPollUsec    = 500 * 1000
	// Return after a few events so the example finishes even when events
	// keep arriving. Increase this limit to follow a longer stream.
	maxEvents = 5
)

func main() {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	err := runEvents(ctx)
	stop()

	// Library failures carry a numeric status.
	// Translate it to the status name and a readable description.
	example_utils.HandleExampleError(err)
}

func runEvents(ctx context.Context) error {
	// AMDSMI_INIT_ALL_PROCESSORS is the default. Choose a different
	// InitFlags value when only a specific processor type is needed.
	initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS

	fmt.Println("###Test amdsmi.Init")
	fmt.Println("  Initializing with AMDSMI_INIT_ALL_PROCESSORS")
	if err := amdsmi.Init(initFlags); err != nil {
		return err
	}
	fmt.Println("  Initialization complete")

	// Always release AMD SMI resources, including after an API error
	// or keyboard interrupt.
	defer func() {
		fmt.Println("###Test amdsmi.ShutDown")
		if err := amdsmi.ShutDown(); err != nil {
			fmt.Println(err)
			return
		}
		fmt.Println("Shutdown complete")
	}()

	// Processor discovery is the shared prelude for AMD SMI examples.
	fmt.Println("###Test amdsmi.GetProcessorHandles")
	processors, err := amdsmi.GetProcessorHandles()
	if err != nil {
		return err
	}
	fmt.Printf("  number of GPUs is: %d\n\n", len(processors))

	if len(processors) == 0 {
		fmt.Println("  No GPUs found")
		return nil
	}

	// Print each processor's BDF before monitoring so a later event
	// can be matched to one of these processors.
	for processorNumber, processor := range processors {
		if err := ctx.Err(); err != nil {
			return err
		}

		fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
		bdf, err := amdsmi.GetGpuDeviceBdf(processor)
		if err != nil {
			return err
		}
		fmt.Printf("  Processor number: %d bdf: %s\n", processorNumber, bdf)
	}
	fmt.Println()

	return monitorEvents(ctx, processors)
}

func monitorEvents(ctx context.Context, processors []amdsmi.ProcessorHandle) error {
	// Monitor every category and every severity.
	// AMDSMI_MASK_ALL selects all of them. To narrow the stream, use NewEventReader,
	// whose categories argument is a slice, so specific EventCategory values can be combined.
	// Pass one EventSeverity value to choose which severities are delivered.
	fmt.Println("###Test AmdSmiEventReader")
	fmt.Println("  Monitoring categories: [ALL]")
	fmt.Println("  Monitoring severity: ALL")

	// The reader watches the discovered processors. The deferred Close()
	// stops the reader and releases native event resources when the
	// read loop exits, including after a timeout or an error.
	reader, err := amdsmi.NewEventReaderMask(processors, amdsmi.AMDSMI_MASK_ALL)
	if err != nil {
		return err
	}

	readErr := func() error {
		defer func() {
			if closeErr := reader.Close(); closeErr != nil {
				fmt.Println(closeErr)
			}
		}()

		fmt.Println("###Test AmdSmiEventReader.Read")
		fmt.Printf(
			"  Waiting up to %d seconds for each event\n",
			eventTimeoutUsec/(1000*1000),
		)

		// Read() blocks until one matching event arrives or the
		// timeout elapses. A timeout means no selected event
		// arrived; on a quiet system that is a normal result.
		for eventIndex := 0; eventIndex < maxEvents; eventIndex++ {
			if err := ctx.Err(); err != nil {
				return err
			}

			event, err := readEventWithContext(ctx, reader, eventTimeoutUsec)
			if err != nil {
				if errors.Is(err, context.Canceled) {
					return err
				}
				var statusErr *amdsmi.StatusError
				if errors.As(err, &statusErr) &&
					statusErr.Code == amdsmi.AMDSMI_STATUS_TIMEOUT {
					fmt.Println("  No matching events occurred before the timeout")
					return nil
				}
				return err
			}

			fmt.Printf("  Event number: %d\n", eventIndex+1)
			if err := printEvent(event); err != nil {
				return err
			}
			fmt.Println()
		}
		return nil
	}()
	if readErr != nil {
		return readErr
	}

	fmt.Println("  AmdSmiEventReader.stop called automatically")
	return nil
}

func readEventWithContext(
	ctx context.Context,
	reader *amdsmi.EventReader,
	timeoutUsec int64,
) (amdsmi.EventEntry, error) {
	remainingUsec := timeoutUsec
	for remainingUsec > 0 {
		if err := ctx.Err(); err != nil {
			return amdsmi.EventEntry{}, err
		}

		pollUsec := int64(eventPollUsec)
		if pollUsec > remainingUsec {
			pollUsec = remainingUsec
		}

		event, err := reader.Read(pollUsec)
		if err == nil {
			return event, nil
		}

		var statusErr *amdsmi.StatusError
		if !errors.As(err, &statusErr) ||
			statusErr.Code != amdsmi.AMDSMI_STATUS_TIMEOUT {
			return amdsmi.EventEntry{}, err
		}

		remainingUsec -= pollUsec
	}

	return amdsmi.EventEntry{}, &amdsmi.StatusError{
		Code: amdsmi.AMDSMI_STATUS_TIMEOUT,
		Name: "AMDSMI_STATUS_TIMEOUT",
	}
}

func printEvent(event amdsmi.EventEntry) error {
	// Timestamp is the UTC time of the event in microseconds.
	// Date is that same moment as a readable UTC string.
	fmt.Printf("  event['timestamp'] is: %d\n", event.Timestamp)
	fmt.Printf("  event['date'] is: %s\n", event.Date)

	// Category is the event family, such as RESET or ECC.
	// Subcode is the specific event inside that family.
	// Level is the severity.
	// Print the enum names so the values can be read without a numeric lookup.
	fmt.Printf("  event['category'] is: %s\n", event.Category)
	fmt.Printf(
		"  event['subcode'] is: %s\n",
		amdsmi.SubcodeName(event.Category, event.Subcode),
	)
	fmt.Printf("  event['level'] is: %s\n", event.Level)

	// Data is a numeric payload whose meaning depends on the subcode.
	fmt.Printf("  event['data'] is: %d\n", event.Data)
	// DevId identifies the processor. FcnId is the virtual function
	// associated with the event when the event belongs to a VF.
	fmt.Printf("  event['dev_id'] is: %#x\n", event.DevId)
	fmt.Printf("  event['fcn_id'] is: %v\n", event.FcnId)
	// Message is the description of what happened.
	fmt.Printf("  event['message'] is: %s\n", event.Message)

	// The event returns the processor handle that originated the event.
	// Resolve it to a BDF so it can be matched to a physical GPU.
	processorBdf, err := amdsmi.GetProcessorBdf(event.ProcessorHandle)
	if err != nil {
		return err
	}
	fmt.Printf("  event processor BDF is: %s\n", processorBdf)
	return nil
}
