// Copyright Advanced Micro Devices, Inc.
//
// SPDX-License-Identifier: MIT

package main

import (
	"fmt"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

func main() {
	err := runLifecycle()
	example_utils.HandleExampleError(err)
}

func runLifecycle() error {
	// Call Init before any other AMD SMI API. AMDSMI_INIT_ALL_PROCESSORS
	// is the default. Pass a different InitFlags value, or OR several
	// together, when only specific processor types are needed.
	initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS

	fmt.Println("###Test amdsmi.Init")
	fmt.Println("  Initializing with AMDSMI_INIT_ALL_PROCESSORS")
	if err := amdsmi.Init(initFlags); err != nil {
		return err
	}
	fmt.Println("  Initialization complete")

	// Shut down after a successful init. Each successful Init needs one
	// ShutDown.
	defer func() {
		fmt.Println("###Test amdsmi.ShutDown")
		if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
			fmt.Println(shutdownErr)
			return
		}
		fmt.Println("Shutdown complete")
	}()

	// Major, Minor, and Release describe the loaded library build.
	// This call takes no processor handle.
	fmt.Println("###Test amdsmi.GetLibVersion")
	libData, err := amdsmi.GetLibVersion()
	if err != nil {
		return err
	}
	fmt.Printf("  lib_data['major'] is: %d\n", libData.Major)
	fmt.Printf("  lib_data['minor'] is: %d\n", libData.Minor)
	fmt.Printf("  lib_data['release'] is: %d\n", libData.Release)

	// A failed library call returns *amdsmi.StatusError. Use
	// StatusCodeToString to turn a Status into the library's
	// description.
	fmt.Println("###Test amdsmi.StatusCodeToString")
	if err := printStatusDescription(
		amdsmi.AMDSMI_STATUS_SUCCESS,
		"SUCCESS",
	); err != nil {
		return err
	}
	if err := printStatusDescription(
		amdsmi.AMDSMI_STATUS_NOT_INIT,
		"STATUS_NOT_INIT",
	); err != nil {
		return err
	}

	return nil
}

func printStatusDescription(status amdsmi.Status, name string) error {
	description, err := amdsmi.StatusCodeToString(status)
	if err != nil {
		return err
	}
	fmt.Printf("  %s is: %s\n", name, description)
	return nil
}
