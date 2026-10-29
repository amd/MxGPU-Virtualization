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
	"strings"
	"syscall"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

type powerCapSensor struct {
	sensorIndex uint32
	sensorType  amdsmi.PowerCapType
	info        amdsmi.PowerCapInfo
}

func main() {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	err := runPowerPerfControl(ctx)
	stop()

	example_utils.HandleExampleError(err)
}

func runPowerPerfControl(ctx context.Context) (err error) {
	prompter := example_utils.NewPrompter(os.Stdin, os.Stdout)

	// INIT_ALL_PROCESSORS is the default. Choose a different InitFlags value
	// when only a specific processor type is needed.
	initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS

	fmt.Println("###Test amdsmi.Init")
	fmt.Println("  Initializing with AMDSMI_INIT_ALL_PROCESSORS")
	if err = amdsmi.Init(initFlags); err != nil {
		return fmt.Errorf("initialize AMD SMI: %w", err)
	}
	fmt.Println("  Initialization complete")

	// Always release AMD SMI resources, including after an API error or
	// keyboard interrupt.
	defer func() {
		fmt.Println("###Test amdsmi.ShutDown")
		if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
			if err != nil && !errors.Is(err, context.Canceled) {
				fmt.Println(shutdownErr)
			} else {
				err = fmt.Errorf("shut down AMD SMI: %w", shutdownErr)
			}
			return
		}
		fmt.Println("Shutdown complete")
	}()

	// Processor discovery is the shared prelude for AMD SMI examples.
	fmt.Println("###Test amdsmi.GetProcessorHandles")
	processors, err := amdsmi.GetProcessorHandles()
	if err != nil {
		return fmt.Errorf("get processor handles: %w", err)
	}
	fmt.Printf("  number of GPUs is: %d\n\n", len(processors))

	if len(processors) == 0 {
		fmt.Println("  No GPUs found")
		return nil
	}

	// Read-only display applies to every GPU. Setter examples operate on
	// one explicitly selected GPU and restore its original state.
	fmt.Println("  Available operations:")
	fmt.Println("  0: Display power and performance control information")
	fmt.Println("  1: Set and restore a power cap")
	fmt.Println("  2: Set and restore a SoC P-state policy")
	fmt.Println("  3: Exit")

	operationUpperBound := uint64(3)
	operationNumber, promptErr := prompter.PromptForInput(
		ctx,
		"  Enter the operation to perform: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &operationUpperBound,
			ErrorMessage: "  Operation index must be between 0 and 3",
		},
	)
	if promptErr != nil {
		return fmt.Errorf("select operation: %w", promptErr)
	}

	switch operationNumber {
	case 0:
		displayAllPowerPerformanceControls(processors)
	case 3:
		fmt.Println("  No operation was performed.")
	default:
		// Display each GPU's BDF so the user can identify the physical
		// device before selecting a power or performance operation.
		fmt.Println("  Available GPUs:")
		for processorNumber, processor := range processors {
			bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
			if bdfErr != nil {
				return fmt.Errorf("get BDF for GPU %d: %w", processorNumber, bdfErr)
			}
			fmt.Printf("  GPU %d bdf: %s\n", processorNumber, bdf)
		}

		gpuUpperBound := uint64(len(processors) - 1)
		processorNumber, promptErr := prompter.PromptForInput(
			ctx,
			"  Enter the GPU index to operate on: ",
			example_utils.InputOptions{
				DefaultValue: 0,
				UpperBound:   &gpuUpperBound,
				ErrorMessage: fmt.Sprintf(
					"  GPU index must be between 0 and %d",
					len(processors)-1,
				),
			},
		)
		if promptErr != nil {
			return fmt.Errorf("select GPU index: %w", promptErr)
		}

		processor := processors[processorNumber]
		fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
		bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
		if bdfErr != nil {
			return fmt.Errorf("get BDF for selected GPU %d: %w", processorNumber, bdfErr)
		}
		fmt.Printf("  Selected GPU %d bdf: %s\n\n", processorNumber, bdf)

		switch operationNumber {
		case 1:
			setAndRestorePowerCap(ctx, prompter, processor)
		default:
			setAndRestoreSocPstate(ctx, prompter, processor)
		}
	}

	return nil
}

func printPowerCapInfo(
	processor amdsmi.ProcessorHandle,
	sensorInd uint32,
	sensorType amdsmi.PowerCapType,
) (amdsmi.PowerCapInfo, bool) {
	// A GPU can expose multiple independently controlled power-cap sensors.
	// Use the sensor index returned by amdsmi.GetSupportedPowerCap rather
	// than assuming that sensor indexes are contiguous.
	// Power-cap units are platform-specific. Keep and reuse the raw values
	// returned by the API when setting a cap.
	fmt.Println("###Test amdsmi.GetPowerCapInfo")
	powerInfo, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetPowerCapInfo",
		func() (amdsmi.PowerCapInfo, error) {
			return amdsmi.GetPowerCapInfo(processor, sensorInd)
		},
	)
	if err != nil {
		fmt.Println("")
		return amdsmi.PowerCapInfo{}, true
	}

	fmt.Printf(
		"  Sensor %d (%s) power cap information:\n",
		sensorInd,
		sensorType,
	)
	fmt.Printf("    power_cap is: %d\n", powerInfo.PowerCap)
	fmt.Printf("    default_power_cap is: %d\n", powerInfo.DefaultPowerCap)
	fmt.Printf("    dpm_cap is: %d\n", powerInfo.DpmCap)
	fmt.Printf("    min_power_cap is: %d\n", powerInfo.MinPowerCap)
	fmt.Printf("    max_power_cap is: %d\n", powerInfo.MaxPowerCap)
	fmt.Println("")
	return powerInfo, false
}

func printPowerCapInfos(processor amdsmi.ProcessorHandle) []powerCapSensor {
	// Query the supported sensors first because power-cap availability and
	// sensor type vary by GPU and platform.
	fmt.Println("###Test amdsmi.GetSupportedPowerCap")
	supportedCaps, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetSupportedPowerCap",
		func() ([]amdsmi.SupportedPowerCapEntry, error) {
			return amdsmi.GetSupportedPowerCap(processor)
		},
	)
	if err != nil {
		fmt.Println("")
		return nil
	}

	fmt.Printf(
		"  number of supported power cap sensors is: %d\n",
		len(supportedCaps),
	)
	if len(supportedCaps) == 0 {
		fmt.Println("  No supported power cap sensors found")
		fmt.Println("")
		return nil
	}

	powerCaps := make([]powerCapSensor, 0, len(supportedCaps))
	// Save each successful query for use by the interactive set operation.
	// Unsupported sensors are reported by GetOptionalAPI and omitted.
	for _, entry := range supportedCaps {
		powerInfo, infoFailed := printPowerCapInfo(
			processor,
			entry.SensorIndex,
			entry.Type,
		)
		if !infoFailed {
			powerCaps = append(powerCaps, powerCapSensor{
				sensorIndex: entry.SensorIndex,
				sensorType:  entry.Type,
				info:        powerInfo,
			})
		}
	}
	return powerCaps
}

func printPowerManagementState(processor amdsmi.ProcessorHandle) (bool, bool) {
	// Power caps can be changed only while GPU power management is enabled.
	fmt.Println("###Test amdsmi.IsGpuPowerManagementEnabled")
	enabled, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.IsGpuPowerManagementEnabled",
		func() (bool, error) {
			return amdsmi.IsGpuPowerManagementEnabled(processor)
		},
	)
	if err == nil {
		state := "disabled"
		if enabled {
			state = "enabled"
		}
		fmt.Printf("  GPU power management is: %s\n", state)
	}
	fmt.Println("")
	return enabled, err != nil
}

func printSocPstate(processor amdsmi.ProcessorHandle) (amdsmi.DpmPolicy, bool) {
	// The current policy is represented by an index into the returned policy
	// list; PolicyID is the value passed to amdsmi.SetSocPstate.
	fmt.Println("###Test amdsmi.GetSocPstate")
	pstate, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetSocPstate",
		func() (amdsmi.DpmPolicy, error) {
			return amdsmi.GetSocPstate(processor)
		},
	)
	if err == nil {
		fmt.Printf("  Current SoC P-state policy index is: %d\n", pstate.Current)
		if len(pstate.Policies) == 0 {
			fmt.Println("  No SoC P-state policies found")
		}
		for policyIndex, policy := range pstate.Policies {
			current := ""
			if uint32(policyIndex) == pstate.Current {
				current = " (current)"
			}
			fmt.Printf(
				"  Policy index %d (ID %d) is: %s%s\n",
				policyIndex,
				policy.PolicyID,
				policy.PolicyDescription,
				current,
			)
		}
	}
	fmt.Println("")
	return pstate, err != nil
}

func displayPowerPerformanceControl(processor amdsmi.ProcessorHandle) {
	// Read-only summary of every power/performance control demonstrated by
	// this example. Optional APIs may be unavailable on some platforms.
	printPowerCapInfos(processor)
	printPowerManagementState(processor)
	printSocPstate(processor)
}

func displayAllPowerPerformanceControls(processors []amdsmi.ProcessorHandle) {
	// Display the BDF before each summary so output can be matched to the
	// physical GPU when more than one processor handle is present.
	for processorNumber, processor := range processors {
		fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
		bdf, err := amdsmi.GetGpuDeviceBdf(processor)
		if err != nil {
			fmt.Fprintf(os.Stderr, "  get BDF for GPU %d: %v\n", processorNumber, err)
			continue
		}
		fmt.Printf("  GPU %d bdf: %s\n\n", processorNumber, bdf)
		displayPowerPerformanceControl(processor)
	}
}

func setAndRestorePowerCap(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
) {
	// Read the current values before making a change. The original cap is
	// restored after the set attempt, even if the verification query fails.
	powerCaps := printPowerCapInfos(processor)
	if len(powerCaps) == 0 {
		return
	}

	// Reject the operation early when the platform reports that power
	// management is disabled or the state cannot be queried.
	enabled, pmFailed := printPowerManagementState(processor)
	if pmFailed || !enabled {
		fmt.Println(
			"  Cannot set a power cap while power management is disabled " +
				"or unavailable.\n",
		)
		return
	}

	// Present a compact, zero-based menu while preserving the real sensor
	// index that must be supplied to the AMD SMI API.
	fmt.Println("  Available power cap sensors:")
	for sensorNumber, sensor := range powerCaps {
		fmt.Printf(
			"  %d: Sensor %d (%s) current=%d minimum=%d maximum=%d\n",
			sensorNumber,
			sensor.sensorIndex,
			sensor.sensorType,
			sensor.info.PowerCap,
			sensor.info.MinPowerCap,
			sensor.info.MaxPowerCap,
		)
	}

	sensorUpperBound := uint64(len(powerCaps) - 1)
	sensorNumber, err := prompter.PromptForInput(
		ctx,
		"  Enter the power cap sensor number to operate on: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &sensorUpperBound,
			ErrorMessage: fmt.Sprintf(
				"  Power cap sensor index must be between 0 and %d",
				len(powerCaps)-1,
			),
		},
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return
	}

	sensor := powerCaps[sensorNumber]
	originalPowerCap := sensor.info.PowerCap

	// Equal minimum and maximum values indicate a fixed, non-adjustable cap.
	if sensor.info.MinPowerCap == sensor.info.MaxPowerCap {
		fmt.Printf(
			"  Sensor %d has no adjustable power cap range.\n\n",
			sensor.sensorIndex,
		)
		return
	}

	requestedPowerCap, err := prompter.PromptForInput(
		ctx,
		fmt.Sprintf(
			"  Enter the power cap to set (%d to %d): ",
			sensor.info.MinPowerCap,
			sensor.info.MaxPowerCap,
		),
		example_utils.InputOptions{
			DefaultValue: originalPowerCap,
			LowerBound:   sensor.info.MinPowerCap,
			UpperBound:   &sensor.info.MaxPowerCap,
			ErrorMessage: fmt.Sprintf(
				"  Power cap must be between %d and %d",
				sensor.info.MinPowerCap,
				sensor.info.MaxPowerCap,
			),
		},
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return
	}
	if requestedPowerCap == originalPowerCap {
		fmt.Println(
			"  The requested power cap is already active. " +
				"No operation was performed.\n",
		)
		return
	}

	confirmed, err := prompter.PromptConfirmation(
		ctx,
		fmt.Sprintf(
			"  Set %s on sensor %d from %d to %d? The script will restore "+
				"the original value.",
			sensor.sensorType,
			sensor.sensorIndex,
			originalPowerCap,
			requestedPowerCap,
		),
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return
	}
	if !confirmed {
		fmt.Println("  No operation was performed.\n")
		return
	}

	// Mark the operation before calling the setter because an error does
	// not guarantee that the device rejected the requested value.
	setAttempted := false
	fmt.Println("###Test amdsmi.SetPowerCap")
	fmt.Printf(
		"  Setting sensor %d power cap to %d\n",
		sensor.sensorIndex,
		requestedPowerCap,
	)
	setAttempted = true
	if example_utils.SetOptionalAPI(
		os.Stdout,
		"amdsmi.SetPowerCap",
		func() error {
			return amdsmi.SetPowerCap(
				processor,
				sensor.sensorIndex,
				requestedPowerCap,
			)
		},
	) == amdsmi.AMDSMI_STATUS_SUCCESS {
		fmt.Println("  Power cap change request completed\n")
		printPowerCapInfo(processor, sensor.sensorIndex, sensor.sensorType)
	} else {
		fmt.Println("")
	}

	// Always attempt to return the GPU to the state observed at entry.
	if setAttempted {
		fmt.Println("###Test amdsmi.SetPowerCap")
		fmt.Printf(
			"  Restoring sensor %d power cap to %d\n",
			sensor.sensorIndex,
			originalPowerCap,
		)
		if example_utils.SetOptionalAPI(
			os.Stdout,
			"amdsmi.SetPowerCap while restoring",
			func() error {
				return amdsmi.SetPowerCap(
					processor,
					sensor.sensorIndex,
					originalPowerCap,
				)
			},
		) == amdsmi.AMDSMI_STATUS_SUCCESS {
			fmt.Println("  Original power cap restored\n")
			printPowerCapInfo(processor, sensor.sensorIndex, sensor.sensorType)
		} else {
			fmt.Println("")
		}
	}
}

func setAndRestoreSocPstate(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
) {
	// Capture the current policy and supported IDs before asking the user to
	// make a temporary change.
	pstate, failed := printSocPstate(processor)
	if failed || len(pstate.Policies) == 0 {
		return
	}

	currentPolicyIndex := int(pstate.Current)
	// An out-of-range index means there is no trustworthy original policy to
	// restore, so changing the policy would be unsafe for this example.
	if currentPolicyIndex < 0 || currentPolicyIndex >= len(pstate.Policies) {
		fmt.Println(
			"  The current policy index does not identify a supported " +
				"policy; the sample cannot safely restore it.\n",
		)
		return
	}

	originalPolicyID := pstate.Policies[currentPolicyIndex].PolicyID
	policyIDs := make([]uint64, len(pstate.Policies))
	for i, policy := range pstate.Policies {
		policyIDs[i] = uint64(policy.PolicyID)
	}

	requestedPolicyID, err := prompter.PromptForInput(
		ctx,
		"  Enter the SoC P-state policy ID to set: ",
		example_utils.InputOptions{
			DefaultValue: uint64(originalPolicyID),
			ValidValues:  policyIDs,
			ErrorMessage: fmt.Sprintf(
				"  Policy ID must be one of: %s",
				strings.Join(uint64SliceToStrings(policyIDs), ", "),
			),
		},
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return
	}
	if uint32(requestedPolicyID) == originalPolicyID {
		fmt.Println(
			"  The requested policy is already active. " +
				"No operation was performed.\n",
		)
		return
	}

	confirmed, err := prompter.PromptConfirmation(
		ctx,
		fmt.Sprintf(
			"  Set the SoC P-state policy from %d to %d? The script will "+
				"restore the original policy.",
			originalPolicyID,
			requestedPolicyID,
		),
	)
	if err != nil {
		fmt.Fprintf(os.Stderr, "  %v\n", err)
		return
	}
	if !confirmed {
		fmt.Println("  No operation was performed.\n")
		return
	}

	// As with power caps, attempt restoration whenever the setter was called,
	// including when that call returns an error.
	setAttempted := false
	fmt.Println("###Test amdsmi.SetSocPstate")
	fmt.Printf("  Setting SoC P-state policy to %d\n", requestedPolicyID)
	setAttempted = true
	if example_utils.SetOptionalAPI(
		os.Stdout,
		"amdsmi.SetSocPstate",
		func() error {
			return amdsmi.SetSocPstate(processor, uint32(requestedPolicyID))
		},
	) == amdsmi.AMDSMI_STATUS_SUCCESS {
		fmt.Println("  SoC P-state policy change request completed\n")
		printSocPstate(processor)
	} else {
		fmt.Println("")
	}

	if setAttempted {
		fmt.Println("###Test amdsmi.SetSocPstate")
		fmt.Printf("  Restoring SoC P-state policy to %d\n", originalPolicyID)
		if example_utils.SetOptionalAPI(
			os.Stdout,
			"amdsmi.SetSocPstate while restoring",
			func() error {
				return amdsmi.SetSocPstate(processor, originalPolicyID)
			},
		) == amdsmi.AMDSMI_STATUS_SUCCESS {
			fmt.Println("  Original SoC P-state policy restored\n")
			printSocPstate(processor)
		} else {
			fmt.Println("")
		}
	}
}

func uint64SliceToStrings(values []uint64) []string {
	result := make([]string, len(values))
	for i, value := range values {
		result[i] = fmt.Sprintf("%d", value)
	}
	return result
}
