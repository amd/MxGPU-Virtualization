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

func main() {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	err := runDiscovery(ctx)
	stop()

	example_utils.HandleExampleError(err)
}

func runDiscovery(ctx context.Context) (err error) {
	prompter := example_utils.NewPrompter(os.Stdin, os.Stdout)

	// INIT_ALL_PROCESSORS is the default. It is needed to discover both
	// GPUs and optional NICs.
	initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS

	fmt.Println("###Test amdsmi.Init")
	fmt.Println("  Initializing with AMDSMI_INIT_ALL_PROCESSORS")
	if err = amdsmi.Init(initFlags); err != nil {
		return fmt.Errorf("initialize AMD SMI: %w", err)
	}
	fmt.Println("  Initialization complete")

	// Always release AMD SMI resources, including after an API error
	// or keyboard interrupt.
	defer func() {
		fmt.Println("###Test amdsmi.ShutDown")
		if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
			wrapped := fmt.Errorf("shut down AMD SMI: %w", shutdownErr)
			if err != nil && !errors.Is(err, context.Canceled) {
				fmt.Fprintf(os.Stderr, "shut down AMD SMI: %v\n", shutdownErr)
			} else {
				err = wrapped
			}
			return
		}
		fmt.Println("Shutdown complete")
	}()

	// Start with the standard processor discovery API, then demonstrate
	// how to request only processors of a specific type.
	fmt.Println("###Test amdsmi.GetProcessorHandles")
	processors, err := amdsmi.GetProcessorHandles()
	if err != nil {
		return fmt.Errorf("get processor handles: %w", err)
	}
	fmt.Printf("  number of GPUs is: %d\n\n", len(processors))

	fmt.Println("###Test amdsmi.GetProcessorHandlesByType")
	gpuProcessors, err := amdsmi.GetProcessorHandlesByType(
		amdsmi.AMDSMI_PROCESSOR_TYPE_AMD_GPU,
	)
	if err != nil {
		return fmt.Errorf("get AMD_GPU processor handles: %w", err)
	}
	fmt.Printf("  number of AMD_GPU processors is: %d\n\n", len(gpuProcessors))

	if len(processors) == 0 {
		fmt.Println("  No GPUs found")
		fmt.Println()
	} else {
		fmt.Println("  Available GPUs:")
		for gpuIndex, processor := range processors {
			bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
			if bdfErr != nil {
				return fmt.Errorf("get BDF for GPU %d: %w", gpuIndex, bdfErr)
			}
			fmt.Printf("  GPU %d bdf: %s\n", gpuIndex, bdf)
		}

		allGpuIndex := uint64(len(processors))
		fmt.Printf("  %d: All GPUs\n", allGpuIndex)

		// The index immediately after the last GPU is used as the
		// "All GPUs" menu choice.
		pickGpuIndex, promptErr := prompter.PromptForInput(
			ctx,
			"  Enter the GPU index to operate on: ",
			example_utils.InputOptions{
				DefaultValue: 0,
				UpperBound:   &allGpuIndex,
				ErrorMessage: fmt.Sprintf(
					"  GPU index must be between 0 and %d (%d for all GPUs)",
					len(processors)-1,
					allGpuIndex,
				),
			},
		)
		if promptErr != nil {
			return fmt.Errorf("select GPU index: %w", promptErr)
		}

		selectedGpuIndexes := make([]int, 0, len(processors))
		if pickGpuIndex == allGpuIndex {
			for gpuIndex := range processors {
				selectedGpuIndexes = append(selectedGpuIndexes, gpuIndex)
			}
		} else {
			selectedGpuIndexes = append(selectedGpuIndexes, int(pickGpuIndex))
		}

		for _, gpuIndex := range selectedGpuIndexes {
			if err := ctx.Err(); err != nil {
				return err
			}

			processor := processors[gpuIndex]
			fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
			bdf, bdfErr := amdsmi.GetGpuDeviceBdf(processor)
			if bdfErr != nil {
				return fmt.Errorf("get BDF for selected GPU %d: %w", gpuIndex, bdfErr)
			}
			fmt.Printf("  Selected GPU %d bdf: %s\n\n", gpuIndex, bdf)

			if err := printGpuDiscovery(gpuIndex, processor); err != nil {
				return err
			}
		}
	}

	return printNicDiscovery(ctx, prompter)
}

func printNodeDiscovery(processor amdsmi.ProcessorHandle) {
	// Node handles are optional because not every system exposes GPU nodes.
	fmt.Println("###Test amdsmi.GetNodeHandle")
	nodeHandle, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNodeHandle",
		func() (amdsmi.NodeHandle, error) {
			return amdsmi.GetNodeHandle(processor)
		},
	)
	if err == nil {
		fmt.Printf("  Node handle was retrieved: %v\n", nodeHandle)
		fmt.Println("  Pass this handle to node-level APIs, such as amdsmi.GetNpmInfo")
	}
	fmt.Println()
}

func printVfDiscovery(processor amdsmi.ProcessorHandle) error {
	// The enabled VF count provides valid indices for the VF lookup API.
	fmt.Println("###Test amdsmi.GetNumVf")
	numEnabled, numSupported, err := amdsmi.GetNumVf(processor)
	if err != nil {
		if !example_utils.PrintOptionalAPIError(os.Stdout, "amdsmi.GetNumVf", err) {
			return fmt.Errorf("get VF count: %w", err)
		}
		fmt.Println()
		return nil
	}

	fmt.Printf("  Number of enabled VFs is: %d\n", numEnabled)
	fmt.Printf("  Number of supported VFs is: %d\n", numSupported)

	if numEnabled == 0 {
		fmt.Println("  No enabled VFs found; VF BDF and UUID lookups require an enabled VF")
		fmt.Println()
		return nil
	}

	// Demonstrate each supported way to identify and look up an enabled VF:
	// index, BDF, and UUID.
	for vfIndex := uint32(0); vfIndex < numEnabled; vfIndex++ {
		if err := printSingleVfDiscovery(processor, vfIndex); err != nil {
			reportOptionalDiscoveryError(fmt.Sprintf("VF %d discovery", vfIndex), err)
		}
		fmt.Println()
	}

	return nil
}

func printSingleVfDiscovery(processor amdsmi.ProcessorHandle, vfIndex uint32) error {
	fmt.Println("###Test amdsmi.GetVfHandleFromVfIndex")
	vfHandle, err := amdsmi.GetVfHandleFromVfIndex(processor, int(vfIndex))
	if err != nil {
		return err
	}
	fmt.Printf("  Retrieved handle for VF %d\n", vfIndex)

	fmt.Println("###Test amdsmi.GetVfBdf")
	vfBdf, err := amdsmi.GetVfBdf(vfHandle)
	if err != nil {
		return err
	}
	fmt.Printf("  VF %d bdf: %s\n", vfIndex, vfBdf)

	fmt.Println("###Test amdsmi.GetVfUuid")
	vfUUID, err := amdsmi.GetVfUuid(vfHandle)
	if err != nil {
		return err
	}
	fmt.Printf("  VF %d UUID: %s\n", vfIndex, vfUUID)

	fmt.Println("###Test amdsmi.GetVfHandleFromBdf")
	vfFromBdf, err := amdsmi.GetVfHandleFromBdf(vfBdf)
	if err != nil {
		return err
	}
	uuidFromBdf, err := amdsmi.GetVfUuid(vfFromBdf)
	if err != nil {
		return err
	}
	fmt.Printf("  UUID from BDF lookup is: %s\n", uuidFromBdf)

	fmt.Println("###Test amdsmi.GetVfHandleFromUuid")
	vfFromUUID, err := amdsmi.GetVfHandleFromUuid(vfUUID)
	if err != nil {
		return err
	}
	bdfFromUUID, err := amdsmi.GetVfBdf(vfFromUUID)
	if err != nil {
		return err
	}
	fmt.Printf("  BDF from UUID lookup is: %s\n", bdfFromUUID)

	return nil
}

func printGpuDiscovery(processorNumber int, processor amdsmi.ProcessorHandle) error {
	fmt.Println("###Test amdsmi.GetProcessorType")
	processorType, err := amdsmi.GetProcessorType(processor)
	if err != nil {
		return fmt.Errorf("get processor type for GPU %d: %w", processorNumber, err)
	}
	fmt.Printf(
		"  Processor number: %d type: %s\n",
		processorNumber,
		processorType,
	)

	fmt.Println("###Test amdsmi.GetIndexFromProcessorHandle")
	processorIndex, err := amdsmi.GetIndexFromProcessorHandle(processor)
	if err != nil {
		return fmt.Errorf("get processor index for GPU %d: %w", processorNumber, err)
	}
	fmt.Printf("  Processor index is: %d\n", processorIndex)

	fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
	gpuBdf, err := amdsmi.GetGpuDeviceBdf(processor)
	if err != nil {
		return fmt.Errorf("get GPU BDF for GPU %d: %w", processorNumber, err)
	}
	fmt.Printf("  GPU bdf is: %s\n", gpuBdf)

	// The generic BDF API works for every processor type, while the GPU
	// device BDF API above is GPU-specific.
	fmt.Println("###Test amdsmi.GetProcessorBdf")
	processorBdf, err := amdsmi.GetProcessorBdf(processor)
	if err != nil {
		return fmt.Errorf("get processor BDF for GPU %d: %w", processorNumber, err)
	}
	fmt.Printf("  Generic processor bdf is: %s\n", processorBdf)

	// Look up the same GPU by index and verify its identity using its BDF.
	fmt.Println("###Test amdsmi.GetProcessorHandleFromIndex")
	processorFromIndex, err := amdsmi.GetProcessorHandleFromIndex(processorIndex)
	if err != nil {
		return fmt.Errorf("get processor handle from index for GPU %d: %w", processorNumber, err)
	}
	bdfFromIndex, err := amdsmi.GetGpuDeviceBdf(processorFromIndex)
	if err != nil {
		return fmt.Errorf("get BDF from index lookup for GPU %d: %w", processorNumber, err)
	}
	fmt.Printf("  BDF from index lookup is: %s\n", bdfFromIndex)

	// BDF and UUID lookups provide alternatives when an index is not known.
	fmt.Println("###Test amdsmi.GetProcessorHandleFromBdf")
	processorFromBdf, err := amdsmi.GetProcessorHandleFromBdf(gpuBdf)
	if err != nil {
		return fmt.Errorf("get processor handle from BDF for GPU %d: %w", processorNumber, err)
	}
	indexFromBdf, err := amdsmi.GetIndexFromProcessorHandle(processorFromBdf)
	if err != nil {
		return fmt.Errorf("get index from BDF lookup for GPU %d: %w", processorNumber, err)
	}
	fmt.Printf("  Processor index from BDF lookup is: %d\n", indexFromBdf)

	fmt.Println("###Test amdsmi.GetGpuDeviceUuid")
	gpuUUID, err := amdsmi.GetGpuDeviceUuid(processor)
	if err != nil {
		return fmt.Errorf("get GPU UUID for GPU %d: %w", processorNumber, err)
	}
	fmt.Printf("  GPU UUID is: %s\n", gpuUUID)

	fmt.Println("###Test amdsmi.GetProcessorHandleFromUuid")
	processorFromUUID, err := amdsmi.GetProcessorHandleFromUuid(gpuUUID)
	if err != nil {
		return fmt.Errorf("get processor handle from UUID for GPU %d: %w", processorNumber, err)
	}
	indexFromUUID, err := amdsmi.GetIndexFromProcessorHandle(processorFromUUID)
	if err != nil {
		return fmt.Errorf("get index from UUID lookup for GPU %d: %w", processorNumber, err)
	}
	fmt.Printf("  Processor index from UUID lookup is: %d\n", indexFromUUID)
	fmt.Println()

	printNodeDiscovery(processor)
	return printVfDiscovery(processor)
}

func printNicDiscovery(ctx context.Context, prompter *example_utils.Prompter) error {
	// NIC support is optional, so an unavailable API is not treated as an
	// error that should stop GPU discovery.
	fmt.Println("###Test amdsmi.GetNicProcessorHandles")
	nicProcessors, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNicProcessorHandles",
		amdsmi.GetNicProcessorHandles,
	)
	if err != nil {
		fmt.Println()
		return nil
	}

	fmt.Printf("  number of NICs is: %d\n", len(nicProcessors))

	// The combined NIC list can also be filtered by vendor-specific type.
	nicTypes := []amdsmi.ProcessorType{
		amdsmi.AMDSMI_PROCESSOR_TYPE_AMD_NIC,
		amdsmi.AMDSMI_PROCESSOR_TYPE_BRCM_NIC,
	}
	for _, nicType := range nicTypes {
		fmt.Println("###Test amdsmi.GetProcessorHandlesByType")
		typedNics, typedErr := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf("amdsmi.GetProcessorHandlesByType(%s)", nicType),
			func() ([]amdsmi.ProcessorHandle, error) {
				return amdsmi.GetProcessorHandlesByType(nicType)
			},
		)
		if typedErr == nil {
			fmt.Printf("  number of %s processors is: %d\n", nicType, len(typedNics))
		}
	}
	fmt.Println()

	if len(nicProcessors) == 0 {
		fmt.Println("  No NICs found")
		fmt.Println()
		return nil
	}

	fmt.Println("  Available NICs:")
	for nicIndex, nic := range nicProcessors {
		nicBdf, err := amdsmi.GetNicDeviceBdf(nic)
		if err != nil {
			return fmt.Errorf("get BDF for NIC %d: %w", nicIndex, err)
		}
		fmt.Printf("  NIC %d bdf: %s\n", nicIndex, nicBdf)
	}

	allNicIndex := uint64(len(nicProcessors))
	fmt.Printf("  %d: All NICs\n", allNicIndex)

	// The index immediately after the last NIC is used as the "All NICs"
	// menu choice.
	pickNicIndex, err := prompter.PromptForInput(
		ctx,
		"  Enter the NIC index to operate on: ",
		example_utils.InputOptions{
			DefaultValue: 0,
			UpperBound:   &allNicIndex,
			ErrorMessage: fmt.Sprintf(
				"  NIC index must be between 0 and %d (%d for all NICs)",
				len(nicProcessors)-1,
				allNicIndex,
			),
		},
	)
	if err != nil {
		return fmt.Errorf("select NIC index: %w", err)
	}

	selectedNicIndexes := make([]int, 0, len(nicProcessors))
	if pickNicIndex == allNicIndex {
		for nicIndex := range nicProcessors {
			selectedNicIndexes = append(selectedNicIndexes, nicIndex)
		}
	} else {
		selectedNicIndexes = append(selectedNicIndexes, int(pickNicIndex))
	}

	for _, selectedNicIndex := range selectedNicIndexes {
		if err := ctx.Err(); err != nil {
			return err
		}
		if err := printSingleNicDiscovery(selectedNicIndex, nicProcessors[selectedNicIndex]); err != nil {
			return err
		}
	}

	return nil
}

func printSingleNicDiscovery(
	selectedNicIndex int,
	nic amdsmi.ProcessorHandle,
) error {
	fmt.Println("###Test amdsmi.GetNicDeviceBdf")
	nicBdf, err := amdsmi.GetNicDeviceBdf(nic)
	if err != nil {
		return fmt.Errorf("get BDF for selected NIC %d: %w", selectedNicIndex, err)
	}
	fmt.Printf("  Selected NIC %d bdf: %s\n\n", selectedNicIndex, nicBdf)

	fmt.Println("###Test amdsmi.GetProcessorType")
	nicType, err := amdsmi.GetProcessorType(nic)
	if err != nil {
		return fmt.Errorf("get processor type for NIC %d: %w", selectedNicIndex, err)
	}
	fmt.Printf("  NIC number: %d type: %s\n", selectedNicIndex, nicType)

	fmt.Println("###Test amdsmi.GetIndexFromProcessorHandle")
	nicIndex, err := amdsmi.GetIndexFromProcessorHandle(nic)
	if err != nil {
		return fmt.Errorf("get index for NIC %d: %w", selectedNicIndex, err)
	}
	fmt.Printf("  NIC index is: %d\n", nicIndex)

	fmt.Println("###Test amdsmi.GetNicDeviceBdf")
	fmt.Printf("  NIC bdf is: %s\n", nicBdf)

	// The generic processor API returns the same BDF without requiring
	// the caller to know that this handle represents a NIC.
	fmt.Println("###Test amdsmi.GetProcessorBdf")
	processorBdf, err := amdsmi.GetProcessorBdf(nic)
	if err != nil {
		return fmt.Errorf("get processor BDF for NIC %d: %w", selectedNicIndex, err)
	}
	fmt.Printf("  Generic processor bdf is: %s\n", processorBdf)

	// amdsmi.GetProcessorHandleFromIndex accepts GPU indexes only.
	// Use BDF to look up a NIC without needing a type-specific index API.
	fmt.Println("###Test amdsmi.GetProcessorHandleFromBdf")
	nicFromBdf, err := amdsmi.GetProcessorHandleFromBdf(nicBdf)
	if err != nil {
		return fmt.Errorf("get processor handle from BDF for NIC %d: %w", selectedNicIndex, err)
	}
	processorTypeFromBdf, err := amdsmi.GetProcessorType(nicFromBdf)
	if err != nil {
		return fmt.Errorf("get processor type from BDF lookup for NIC %d: %w", selectedNicIndex, err)
	}
	fmt.Printf("  Processor type from BDF lookup is: %s\n", processorTypeFromBdf)
	fmt.Println()

	return nil
}

func reportOptionalDiscoveryError(apiName string, err error) {
	if !example_utils.PrintOptionalAPIError(os.Stdout, apiName, err) {
		fmt.Fprintf(os.Stdout, "  %s failed: %v\n", apiName, err)
	}
}
