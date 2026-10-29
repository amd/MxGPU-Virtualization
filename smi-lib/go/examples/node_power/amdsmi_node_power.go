// Copyright Advanced Micro Devices, Inc.
//
// SPDX-License-Identifier: MIT

package main

import (
	"fmt"
	"os"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

func main() {
	err := runNodePower()
	example_utils.HandleExampleError(err)
}

func runNodePower() error {
	// Initialize AMD SMI for all processor types so GPU handles can be
	// discovered. A node handle is obtained from each discovered GPU.
	initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS

	fmt.Println("###Test amdsmi.Init")
	fmt.Println("  Initializing with AMDSMI_INIT_ALL_PROCESSORS")
	if err := amdsmi.Init(initFlags); err != nil {
		return err
	}
	fmt.Println("  Initialization complete")

	// Shut down after a successful init. Each successful amdsmi.Init
	// needs one amdsmi.ShutDown.
	defer func() {
		fmt.Println("###Test amdsmi.ShutDown")
		if err := amdsmi.ShutDown(); err != nil {
			fmt.Println(err)
			return
		}
		fmt.Println("Shutdown complete")
	}()

	// Retrieve one opaque processor handle for every GPU visible to AMD
	// SMI. These handles are required by the remaining GPU-level APIs.
	fmt.Println("###Test amdsmi.GetProcessorHandles")
	processors, err := amdsmi.GetProcessorHandles()
	if err != nil {
		return err
	}
	fmt.Printf("  number of GPUs is: %d\n\n", len(processors))

	if len(processors) == 0 {
		fmt.Println("  No GPUs found")
	}

	// Show the PCI BDF to identify each physical GPU, then report the NPM
	// information for the node to which that GPU belongs. Multiple GPUs
	// can belong to the same node and therefore report the same settings.
	for processorNumber, processor := range processors {
		fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
		bdf, err := amdsmi.GetGpuDeviceBdf(processor)
		if err != nil {
			return err
		}
		fmt.Printf("  Processor number: %d bdf: %s\n\n", processorNumber, bdf)

		printNodePowerInfo(processor, processorNumber)
	}

	return nil
}

// Node Power Management (NPM) controls the power budget for an entire node.
// This example discovers each GPU, maps it to its node, and reports the node's current NPM settings.
// NPM settings are configured out of band (for example, through the BMC), so this example does not modify them.
func printNodePowerInfo(processor amdsmi.ProcessorHandle, processorNumber int) {
	// Translate the processor handle before querying NPM.
	fmt.Println("###Test amdsmi.GetNodeHandle")
	// Node-level APIs are not supported on every platform.
	// GetOptionalAPI() reports an unsupported API without terminating the rest of the example.
	nodeHandle, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNodeHandle",
		func() (amdsmi.NodeHandle, error) {
			return amdsmi.GetNodeHandle(processor)
		},
	)
	if err != nil {
		// Without a node handle there is no valid input for the NPM query.
		fmt.Println()
		return
	}
	fmt.Printf(
		"  Node handle for GPU %d is: %v\n",
		processorNumber,
		nodeHandle,
	)

	// Query the NPM controller for the node associated with this GPU.
	// As with node-handle discovery, this API may be unavailable on some platforms.
	fmt.Println("###Test amdsmi.GetNpmInfo")
	npmInfo, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetNpmInfo",
		func() (amdsmi.NpmInfo, error) {
			return amdsmi.GetNpmInfo(nodeHandle)
		},
	)
	if err == nil {
		// The result is an NpmInfo struct containing:
		//   Status            - whether NPM is enabled or disabled
		//   Limit             - the current node power limit in watts
		//   UbbPowerThreshold - the upper UBB node threshold in watts (not supported on all platforms)
		fmt.Printf("  npm_info['status'] is: %s\n", npmInfo.Status)
		fmt.Printf("  npm_info['limit'] is: %d W\n", npmInfo.Limit)
		fmt.Printf(
			"  npm_info['ubb_power_threshold'] is: %d W\n",
			npmInfo.UbbPowerThreshold,
		)
	}
	fmt.Println()
}
