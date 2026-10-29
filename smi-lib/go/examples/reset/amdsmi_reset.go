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
	"time"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

// A VF must not be running guest work when its framebuffer is cleared.
// Both AVAILABLE states represent VFs that are safe to offer for selection.
var inactiveVfStates = map[amdsmi.VfSchedState]bool{
	amdsmi.AMDSMI_VF_STATE_AVAILABLE:         true,
	amdsmi.AMDSMI_VF_STATE_DEFAULT_AVAILABLE: true,
}

func main() {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	err := runReset(ctx)
	stop()

	example_utils.HandleExampleError(err)
}

func runReset(ctx context.Context) (err error) {
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

	defer func() {
		// Always release AMD SMI resources, including after an API error or
		// keyboard interrupt.
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

	if len(processors) == 0 {
		fmt.Println("  No GPUs found")
		return nil
	}

	// Display each GPU's BDF so the user can identify the physical
	// device before choosing a potentially disruptive operation.
	fmt.Println("  Available GPUs:")
	for gpuIndex, processor := range processors {
		bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
		if bdfErr != nil {
			return fmt.Errorf("get BDF for GPU %d: %w", gpuIndex, bdfErr)
		}
		fmt.Printf("  GPU %d bdf: %s\n", gpuIndex, bdf)
	}

	// Prompt the user to select a GPU, and validate that the selected index belongs to
	// one of the discovered GPU handles.
	upperBound := uint64(len(processors) - 1)
	pickGpuIndex, promptErr := prompter.PromptForInput(
		ctx,
		"  Enter the GPU index to operate on: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &upperBound,
			ErrorMessage: fmt.Sprintf(
				"  GPU index must be between 0 and %d",
				len(processors)-1,
			),
		},
	)
	if promptErr != nil {
		return fmt.Errorf("select GPU index: %w", promptErr)
	}

	processor := processors[pickGpuIndex]
	fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
	bdf, err := amdsmi.GetGpuDeviceBdf(processor)
	if err != nil {
		return fmt.Errorf("get BDF for selected GPU %d: %w", pickGpuIndex, err)
	}
	fmt.Printf("  Selected GPU %d bdf: %s\n\n", pickGpuIndex, bdf)

	// GPU reset affects the whole physical device.
	// Clearing a VF framebuffer affects only an inactive VF.
	fmt.Println("  Available operations:")
	fmt.Println("  0: Reset GPU (Resetting a GPU disrupts work running on that GPU.)")
	fmt.Println(
		"  1: Clear VF framebuffer (A VF framebuffer can be cleared only when the VF is inactive.)",
	)
	fmt.Println("  2: Exit")

	operationUpperBound := uint64(2)
	pickOperation, promptErr := prompter.PromptForInput(
		ctx,
		"  Enter the operation to perform: ",
		example_utils.InputOptions{
			DefaultValue: 2,
			UpperBound:   &operationUpperBound,
			ErrorMessage: "  Operation must be between 0 and 2",
		},
	)
	if promptErr != nil {
		return fmt.Errorf("select operation: %w", promptErr)
	}

	switch pickOperation {
	case 2:
		fmt.Println("  No operation was performed. Exiting...")
	case 0:
		if err := runGpuReset(ctx, prompter, processor, int(pickGpuIndex), bdf); err != nil {
			return err
		}
	default:
		if err := runClearVfFramebuffer(ctx, prompter, processor); err != nil {
			return err
		}
	}

	return nil
}

func runGpuReset(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
	gpuIndex int,
	bdf amdsmi.Bdf,
) error {
	// Require explicit confirmation because a reset interrupts
	// all work currently running on the selected GPU.
	confirmed, err := prompter.PromptConfirmation(
		ctx,
		"  Are you sure you want to reset the GPU?",
	)
	if err != nil {
		return fmt.Errorf("confirm GPU reset: %w", err)
	}
	if !confirmed {
		fmt.Println("  No operation was performed. Exiting...")
		return nil
	}

	// Resetting a GPU disrupts work running on that GPU.
	fmt.Println("###Test amdsmi.ResetGpu")
	fmt.Printf("  Resetting GPU %d bdf: %s\n", gpuIndex, bdf)
	status := example_utils.SetOptionalAPI(
		os.Stdout,
		"amdsmi.ResetGpu",
		func() error {
			return amdsmi.ResetGpu(processor)
		},
	)
	if status == amdsmi.AMDSMI_STATUS_SUCCESS {
		fmt.Println("  Reset request completed")
		// The reset API returns after submitting the request.
		// Allow the device time to become available again before shutting down.
		fmt.Println("  Waiting for GPU reset to complete...")
		time.Sleep(20 * time.Second)
	}

	return nil
}

func runClearVfFramebuffer(
	ctx context.Context,
	prompter *example_utils.Prompter,
	processor amdsmi.ProcessorHandle,
) error {
	// A VF framebuffer can be cleared only when the VF is inactive.
	fmt.Println("###Test amdsmi.GetVfPartitionInfo")
	partitions, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetVfPartitionInfo",
		func() ([]amdsmi.PartitionInfo, error) {
			return amdsmi.GetVfPartitionInfo(processor)
		},
	)
	if err != nil {
		return nil
	}

	if len(partitions) == 0 {
		fmt.Println("  No VFs found")
		return nil
	}

	// Inspect every VF and retain only inactive entries
	// as valid choices for the framebuffer-clear operation.
	fmt.Println("  Available VFs:")
	validVfIndexes := make([]uint64, 0, len(partitions))
	for vfIndex, partition := range partitions {
		vfHandle := partition.VfHandle

		vfData, dataErr := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf("amdsmi.GetVfData for VF %d", vfIndex),
			func() (amdsmi.VfData, error) {
				return amdsmi.GetVfData(vfHandle)
			},
		)
		if dataErr != nil {
			continue
		}

		vfState := vfData.Sched.State
		vfBdf, bdfErr := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf("amdsmi.GetVfBdf for VF %d", vfIndex),
			func() (amdsmi.Bdf, error) {
				return amdsmi.GetVfBdf(vfHandle)
			},
		)
		if bdfErr != nil {
			continue
		}

		if isInactiveVfState(vfState) {
			validVfIndexes = append(validVfIndexes, uint64(vfIndex))
		}

		fmt.Printf(
			"  VF %d bdf: %s (%s)\n",
			vfIndex,
			vfBdf,
			vfStateLabel(vfState),
		)
	}

	if len(validVfIndexes) == 0 {
		fmt.Println("  No inactive VFs found")
		return nil
	}

	// ValidValues prevents an active VF from being
	// selected even though it appears in the VF list.
	validValuesMessage := strings.Join(
		uint64SliceToStrings(validVfIndexes),
		", ",
	)
	partitionUpperBound := uint64(len(partitions) - 1)
	pickVfIndex, err := prompter.PromptForInput(
		ctx,
		"  Enter the VF index to operate on: ",
		example_utils.InputOptions{
			DefaultValue: validVfIndexes[0],
			UpperBound:   &partitionUpperBound,
			ValidValues:  validVfIndexes,
			ErrorMessage: fmt.Sprintf(
				"  The VF needs to be inactive. Valid inactive VF indexes are: %s",
				validValuesMessage,
			),
		},
	)
	if err != nil {
		return fmt.Errorf("select VF index: %w", err)
	}

	vfHandle := partitions[pickVfIndex].VfHandle
	// Read the state again after selection in case the VF
	// became active while the user was choosing it.
	vfData, dataErr := example_utils.GetOptionalAPI(
		os.Stdout,
		fmt.Sprintf("amdsmi.GetVfData for selected VF %d", pickVfIndex),
		func() (amdsmi.VfData, error) {
			return amdsmi.GetVfData(vfHandle)
		},
	)
	if dataErr != nil {
		return nil
	}

	vfState := vfData.Sched.State
	vfBdf, bdfErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetVfBdf",
		func() (amdsmi.Bdf, error) {
			return amdsmi.GetVfBdf(vfHandle)
		},
	)
	if bdfErr != nil {
		return nil
	}

	fmt.Println("###Test amdsmi.GetVfBdf")
	fmt.Printf(
		"  Selected VF %d bdf: %s (%s)\n",
		pickVfIndex,
		vfBdf,
		vfStateLabel(vfState),
	)

	if !isInactiveVfState(vfState) {
		fmt.Println(
			"  The selected VF needs to be inactive before its framebuffer can be cleared.",
		)
		return nil
	}

	confirmed, err := prompter.PromptConfirmation(
		ctx,
		"  Are you sure you want to clear the VF framebuffer?",
	)
	if err != nil {
		return fmt.Errorf("confirm VF framebuffer clear: %w", err)
	}
	if !confirmed {
		fmt.Println("  No operation was performed. Exiting...")
		return nil
	}

	// Clearing removes the selected inactive VF's
	// framebuffer contents without resetting the GPU.
	fmt.Println("###Test amdsmi.ClearVfFb")
	fmt.Printf("  Clearing framebuffer for VF %d\n", pickVfIndex)
	status := example_utils.SetOptionalAPI(
		os.Stdout,
		"amdsmi.ClearVfFb",
		func() error {
			return amdsmi.ClearVfFb(vfHandle)
		},
	)
	if status == amdsmi.AMDSMI_STATUS_SUCCESS {
		fmt.Println("  VF framebuffer clear completed")
	}

	return nil
}

func isInactiveVfState(state amdsmi.VfSchedState) bool {
	return inactiveVfStates[state]
}

func vfStateLabel(state amdsmi.VfSchedState) string {
	if isInactiveVfState(state) {
		return "inactive"
	}
	return strings.ToLower(strings.ReplaceAll(state.String(), "_", " "))
}

func uint64SliceToStrings(values []uint64) []string {
	result := make([]string, len(values))
	for i, value := range values {
		result[i] = fmt.Sprintf("%d", value)
	}
	return result
}
