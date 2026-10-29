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
	"sort"
	"strings"
	"syscall"
	"time"

	"smi-lib/go/amdsmi"
	example_utils "smi-lib/go/examples/example_utils"
)

// Increase either value to collect a longer sample or stop the script with Ctrl+C.
const (
	pollIterations = 3
	pollInterval   = time.Second
)

type gpuTelemetryPath struct {
	processorNumber int
	processor       amdsmi.ProcessorHandle
	useMetricsTable bool
	metricsValid    bool
	metrics         []amdsmi.Metric
}

type nicRdmaPortRef struct {
	Device        string
	Netdev        string
	RdmaPort      uint8
	RdmaPortIndex uint32
}

func main() {
	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	err := runTelemetry(ctx)
	stop()

	example_utils.HandleExampleError(err)
}

func runTelemetry(ctx context.Context) (err error) {
	// INIT_ALL_PROCESSORS is needed to discover both GPUs and optional NICs.
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
				fmt.Fprintf(os.Stderr, "shut down AMD SMI: %v\n", shutdownErr)
			} else if err == nil {
				err = fmt.Errorf("shut down AMD SMI: %w", shutdownErr)
			}
			return
		}
		fmt.Println("Shutdown complete")
	}()

	// Retrieve the GPU processor handles used by the telemetry queries below.
	fmt.Println("###Test amdsmi.GetProcessorHandles")
	processors, err := amdsmi.GetProcessorHandles()
	if err != nil {
		return fmt.Errorf("get GPU processor handles: %w", err)
	}
	fmt.Printf("  number of GPUs is: %d\n\n", len(processors))

	if len(processors) == 0 {
		fmt.Println("  No GPUs found\n")
	} else {
		// Print each GPU's BDF and PCIe capabilities before sampling so
		// users can associate telemetry output with a physical device.
		for processorNumber, processor := range processors {
			fmt.Println("###Test amdsmi.GetGpuDeviceBdf")
			bdf, err := example_utils.GetOptionalAPI(
				os.Stdout,
				"amdsmi.GetGpuDeviceBdf",
				func() (amdsmi.Bdf, error) {
					return amdsmi.GetGpuDeviceBdf(processor)
				},
			)
			fmt.Printf(
				"  Processor number: %d bdf: %s\n\n",
				processorNumber,
				valueOrNotAvailable(err != nil, bdf.String(), "%s"),
			)
			printGpuPcieCapabilities(processor)
		}

		if err = pollGpuTelemetry(ctx, processors); err != nil {
			return err
		}
	}

	return printNicTelemetry(ctx)
}

func valueOrNotAvailable[T any](failed bool, value T, printFormat string) string {
	// Format the value only when the API returned data.
	if failed {
		return "N/A"
	}
	return fmt.Sprintf(printFormat, value)
}

func formatStatistics(statistics []amdsmi.NicStat, failed bool) string {
	// NIC statistic APIs return a slice of name/value entries.
	// Distinguish between an API failure and a successful response with no statistics.
	if failed {
		return "not available"
	}
	if len(statistics) == 0 {
		return "none"
	}

	parts := make([]string, len(statistics))
	for i, statistic := range statistics {
		parts[i] = fmt.Sprintf("%s=%d", statistic.Name, statistic.Value)
	}
	return strings.Join(parts, ", ")
}

func currentPciBandwidthValue(pciBandwidth amdsmi.GpuPciBandwidth, failed bool, key string) string {
	// TransferRate.Current is the index of the active entry in
	// Values and Lanes, not the transfer rate itself.
	if failed {
		return "N/A"
	}

	currentIndex := int(pciBandwidth.TransferRate.Current)
	if key == "frequency" {
		if currentIndex >= len(pciBandwidth.TransferRate.Values) {
			return "N/A"
		}
		return fmt.Sprintf("%d", pciBandwidth.TransferRate.Values[currentIndex])
	}

	if currentIndex >= len(pciBandwidth.Lanes) {
		return "N/A"
	}
	return fmt.Sprintf("%d", pciBandwidth.Lanes[currentIndex])
}

func printGpuPcieCapabilities(processor amdsmi.ProcessorHandle) {
	// PCIe information combines static link capabilities with values that
	// change while the GPU is running.
	fmt.Println("###Test amdsmi.GetPcieInfo")
	pcieInfo, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetPcieInfo",
		func() (amdsmi.PcieInfo, error) {
			return amdsmi.GetPcieInfo(processor)
		},
	)
	if err == nil {
		pcieStatic := pcieInfo.Static
		fmt.Printf("  pcie_static['max_pcie_width'] is: %d\n", pcieStatic.MaxPcieWidth)
		fmt.Printf("  pcie_static['max_pcie_speed'] is: %d\n", pcieStatic.MaxPcieSpeed)
		fmt.Printf(
			"  pcie_static['pcie_interface_version'] is: %d\n",
			pcieStatic.PcieInterfaceVersion,
		)
		fmt.Printf("  pcie_static['slot_type'] is: %s\n", pcieStatic.SlotType)
		fmt.Printf(
			"  pcie_static['max_pcie_interface_version'] is: %d\n",
			pcieStatic.MaxPcieInterfaceVersion,
		)
	}
	fmt.Println()

	// The bandwidth API provides parallel frequency and lane-width arrays.
	// The active array element is selected by TransferRate.Current.
	fmt.Println("###Test amdsmi.GetGpuPciBandwidth")
	pciBandwidth, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuPciBandwidth",
		func() (amdsmi.GpuPciBandwidth, error) {
			return amdsmi.GetGpuPciBandwidth(processor)
		},
	)
	if err == nil {
		transferRate := pciBandwidth.TransferRate
		fmt.Printf(
			"  transfer_rate['current'] index is: %d\n",
			transferRate.Current,
		)
		fmt.Printf(
			"  current transfer rate is: %s\n",
			currentPciBandwidthValue(pciBandwidth, false, "frequency"),
		)
		fmt.Printf(
			"  current lanes is: %s\n",
			currentPciBandwidthValue(pciBandwidth, false, "lanes"),
		)
		fmt.Printf(
			"  number of supported transfer rates is: %d\n",
			transferRate.NumSupported,
		)
		for frequencyIndex, frequency := range transferRate.Values {
			fmt.Printf(
				"  supported frequency: %d lanes: %d\n",
				frequency,
				pciBandwidth.Lanes[frequencyIndex],
			)
		}
	}
	fmt.Println()
}

func selectGpuTelemetryPaths(processors []amdsmi.ProcessorHandle) []gpuTelemetryPath {
	// Prefer the self-describing metrics table. A NOT_SUPPORTED response is a
	// capability signal to use the focused compatibility APIs for that GPU.
	telemetryPaths := make([]gpuTelemetryPath, 0, len(processors))
	fmt.Println("###Selecting GPU telemetry APIs")

	// Store the chosen path per GPU because support can differ across devices
	// in the same system. A useMetricsTable value of false identifies the
	// fallback path.
	for processorNumber, processor := range processors {
		metrics, err := amdsmi.GetGpuMetrics(processor)
		if err == nil {
			fmt.Printf("  GPU %d: using amdsmi.GetGpuMetrics\n", processorNumber)
			telemetryPaths = append(telemetryPaths, gpuTelemetryPath{
				processorNumber: processorNumber,
				processor:       processor,
				useMetricsTable: true,
				metricsValid:    true,
				metrics:         metrics,
			})
			continue
		}

		// Only AMDSMI_STATUS_NOT_SUPPORTED selects the compatibility path. Other
		// errors may be transient or device-specific, so report and skip
		// that GPU instead of hiding the error behind fallback queries.
		var statusErr *amdsmi.StatusError
		if errors.As(err, &statusErr) &&
			statusErr.Code == amdsmi.AMDSMI_STATUS_NOT_SUPPORTED {
			fmt.Printf(
				"  GPU %d: amdsmi.GetGpuMetrics is not supported; "+
					"using compatibility telemetry APIs\n",
				processorNumber,
			)
			telemetryPaths = append(telemetryPaths, gpuTelemetryPath{
				processorNumber: processorNumber,
				processor:       processor,
			})
			continue
		}

		apiName := fmt.Sprintf("amdsmi.GetGpuMetrics for GPU %d", processorNumber)
		if !example_utils.PrintOptionalAPIError(os.Stdout, apiName, err) {
			fmt.Fprintf(os.Stdout, "  %s failed: %v\n", apiName, err)
		}
		fmt.Printf(
			"  GPU %d: telemetry path could not be selected; skipping GPU telemetry\n",
			processorNumber,
		)
	}
	fmt.Println()

	return telemetryPaths
}

func pollGpuMetrics(
	processorNumber int,
	processor amdsmi.ProcessorHandle,
	pollIteration int,
	metrics []amdsmi.Metric,
	metricsValid bool,
) {
	// Include the BDF in every sample so output remains identifiable when
	// multiple GPUs are polled during the same run.
	bdf, err := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuDeviceBdf",
		func() (amdsmi.Bdf, error) {
			return amdsmi.GetGpuDeviceBdf(processor)
		},
	)

	metricCount := "N/A"
	if metricsValid {
		metricCount = fmt.Sprintf("%d", len(metrics))
	}
	fmt.Printf(
		"  Poll %d GPU %d %s: metrics=%s\n",
		pollIteration,
		processorNumber,
		valueOrNotAvailable(err != nil, bdf.String(), "%s"),
		metricCount,
	)

	if !metricsValid {
		return
	}
	if len(metrics) == 0 {
		fmt.Println("    No GPU metrics found")
		return
	}

	// Each row is self-describing: name and unit explain the value, category
	// groups its purpose, and resource fields identify the measured instance.
	// Sort by name so related metrics (e.g. CLK_GFX and CLK_GFX_MIN_LIMIT)
	// appear together; tie-break on resource identity for multi-instance rows.
	sortedMetrics := append([]amdsmi.Metric(nil), metrics...)
	sort.Slice(sortedMetrics, func(i, j int) bool {
		a, b := sortedMetrics[i], sortedMetrics[j]
		an, bn := a.Name.String(), b.Name.String()
		if an != bn {
			return an < bn
		}
		if a.ResGroup != b.ResGroup {
			return a.ResGroup < b.ResGroup
		}
		if a.ResSubgroup != b.ResSubgroup {
			return a.ResSubgroup < b.ResSubgroup
		}
		return a.ResInstance < b.ResInstance
	})

	for metricIndex, metric := range sortedMetrics {
		// A metric can have multiple type flags. Join them for compact output;
		// use "-" when no flags apply. VfMask identifies the PF and active VFs
		// to which the metric applies.
		flagNames := make([]string, 0, len(metric.Flags.Flags()))
		for _, flag := range metric.Flags.Flags() {
			flagNames = append(flagNames, flag.String())
		}
		flagStr := "-"
		if len(flagNames) > 0 {
			flagStr = strings.Join(flagNames, "|")
		}

		fmt.Printf(
			"    [%3d] name=%-28s unit=%-10s cat=%-10s val=%-20d "+
				"flags=%-20s vf_mask=0x%08X res=%s/%s inst=%d\n",
			metricIndex,
			metric.Name,
			metric.Unit,
			metric.Category,
			metric.Val,
			flagStr,
			metric.VfMask,
			metric.ResGroup,
			metric.ResSubgroup,
			metric.ResInstance,
		)
	}
}

func pollGpuCompatibilityTelemetry(
	processorNumber int,
	processor amdsmi.ProcessorHandle,
	pollIteration int,
) {
	// Some devices do not expose the unified metrics table. Query the focused
	// activity, power, temperature, clock, and PCIe APIs instead.
	bdf, bdfErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuDeviceBdf",
		func() (amdsmi.Bdf, error) {
			return amdsmi.GetGpuDeviceBdf(processor)
		},
	)
	activity, activityErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuActivity",
		func() (amdsmi.EngineUsage, error) {
			return amdsmi.GetGpuActivity(processor)
		},
	)
	power, powerErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetPowerInfo",
		func() (amdsmi.PowerInfo, error) {
			return amdsmi.GetPowerInfo(processor)
		},
	)
	edgeTemperature, edgeTempErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetTempMetric (EDGE CURRENT)",
		func() (int64, error) {
			return amdsmi.GetTempMetric(
				processor,
				amdsmi.AMDSMI_TEMPERATURE_TYPE_EDGE,
				amdsmi.AMDSMI_TEMP_CURRENT,
			)
		},
	)
	hotspotTemperature, hotspotTempErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetTempMetric (HOTSPOT CURRENT)",
		func() (int64, error) {
			return amdsmi.GetTempMetric(
				processor,
				amdsmi.AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
				amdsmi.AMDSMI_TEMP_CURRENT,
			)
		},
	)
	gfxClock, gfxClockErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetClockInfo (GFX)",
		func() (amdsmi.ClkInfo, error) {
			return amdsmi.GetClockInfo(processor, amdsmi.AMDSMI_CLK_TYPE_GFX)
		},
	)
	memClock, memClockErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetClockInfo (MEM)",
		func() (amdsmi.ClkInfo, error) {
			return amdsmi.GetClockInfo(processor, amdsmi.AMDSMI_CLK_TYPE_MEM)
		},
	)
	pcieInfo, pcieInfoErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetPcieInfo",
		func() (amdsmi.PcieInfo, error) {
			return amdsmi.GetPcieInfo(processor)
		},
	)
	pciBandwidth, pciBandwidthErr := example_utils.GetOptionalAPI(
		os.Stdout,
		"amdsmi.GetGpuPciBandwidth",
		func() (amdsmi.GpuPciBandwidth, error) {
			return amdsmi.GetGpuPciBandwidth(processor)
		},
	)

	pcieMetric := pcieInfo.Metric

	// Print one line per sample to make changes between poll iterations easy
	// to compare. Values retain the units defined by their individual APIs.
	fmt.Printf(
		"  Poll %d GPU %d %s: activity(gfx=%s, umc=%s, mm=%s) "+
			"power(socket=%s, gfx_voltage=%s, soc_voltage=%s, "+
			"mem_voltage=%s) temp(edge=%s, hotspot=%s) "+
			"gfx_clock(clk=%s, min=%s, max=%s, locked=%s, deep_sleep=%s) "+
			"mem_clock(clk=%s, min=%s, max=%s, deep_sleep=%s) "+
			"pcie(speed=%s, width=%s, bandwidth=%s, current_rate=%s, "+
			"replay=%s, l0_recovery=%s, rollover=%s, nak_sent=%s, "+
			"nak_received=%s, other_recovery=%s)\n",
		pollIteration,
		processorNumber,
		valueOrNotAvailable(bdfErr != nil, bdf.String(), "%s"),
		valueOrNotAvailable(activityErr != nil, activity.GfxActivity, "%d"),
		valueOrNotAvailable(activityErr != nil, activity.UmcActivity, "%d"),
		valueOrNotAvailable(activityErr != nil, activity.MmActivity, "%d"),
		valueOrNotAvailable(powerErr != nil, power.SocketPower, "%d"),
		valueOrNotAvailable(powerErr != nil, power.GfxVoltage, "%d"),
		valueOrNotAvailable(powerErr != nil, power.SocVoltage, "%d"),
		valueOrNotAvailable(powerErr != nil, power.MemVoltage, "%d"),
		valueOrNotAvailable(edgeTempErr != nil, edgeTemperature, "%d"),
		valueOrNotAvailable(hotspotTempErr != nil, hotspotTemperature, "%d"),
		valueOrNotAvailable(gfxClockErr != nil, gfxClock.Clk, "%d"),
		valueOrNotAvailable(gfxClockErr != nil, gfxClock.MinClk, "%d"),
		valueOrNotAvailable(gfxClockErr != nil, gfxClock.MaxClk, "%d"),
		valueOrNotAvailable(gfxClockErr != nil, gfxClock.ClkLocked, "%t"),
		valueOrNotAvailable(gfxClockErr != nil, gfxClock.ClkDeepSleep, "%t"),
		valueOrNotAvailable(memClockErr != nil, memClock.Clk, "%d"),
		valueOrNotAvailable(memClockErr != nil, memClock.MinClk, "%d"),
		valueOrNotAvailable(memClockErr != nil, memClock.MaxClk, "%d"),
		valueOrNotAvailable(memClockErr != nil, memClock.ClkDeepSleep, "%t"),
		valueOrNotAvailable(pcieInfoErr != nil, pcieMetric.PcieSpeed, "%d"),
		valueOrNotAvailable(pcieInfoErr != nil, uint32(pcieMetric.PcieWidth), "%d"),
		valueOrNotAvailable(pcieInfoErr != nil, pcieMetric.PcieBandwidth, "%d"),
		currentPciBandwidthValue(pciBandwidth, pciBandwidthErr != nil, "frequency"),
		valueOrNotAvailable(pcieInfoErr != nil, pcieMetric.PcieReplayCount, "%d"),
		valueOrNotAvailable(pcieInfoErr != nil, pcieMetric.PcieL0ToRecoveryCount, "%d"),
		valueOrNotAvailable(pcieInfoErr != nil, pcieMetric.PcieReplayRollOverCount, "%d"),
		valueOrNotAvailable(pcieInfoErr != nil, pcieMetric.PcieNakSentCount, "%d"),
		valueOrNotAvailable(pcieInfoErr != nil, pcieMetric.PcieNakReceivedCount, "%d"),
		valueOrNotAvailable(pcieInfoErr != nil, pcieMetric.PcieLcPerfOtherEndRecoveryCount, "%d"),
	)
}

func pollGpuTelemetry(ctx context.Context, processors []amdsmi.ProcessorHandle) error {
	// Probe API support once before polling so every GPU consistently uses the
	// unified metrics path or the compatibility path throughout this run.
	telemetryPaths := selectGpuTelemetryPaths(processors)
	if len(telemetryPaths) == 0 {
		fmt.Println("  No GPU telemetry paths available\n")
		return nil
	}

	fmt.Println("###Polling GPU telemetry")
	fmt.Println("  Values use the units returned by the AMD SMI API.")

	for pollIteration := 1; pollIteration <= pollIterations; pollIteration++ {
		if err := ctx.Err(); err != nil {
			return err
		}

		for _, path := range telemetryPaths {
			// useMetricsTable == false is the marker selected above for
			// compatibility APIs.
			if !path.useMetricsTable {
				pollGpuCompatibilityTelemetry(
					path.processorNumber,
					path.processor,
					pollIteration,
				)
				continue
			}

			// Reuse the metrics returned by capability probing for the first
			// sample, then fetch a fresh metrics table for subsequent samples.
			metrics := path.metrics
			metricsValid := path.metricsValid
			if pollIteration > 1 {
				refreshed, err := example_utils.GetOptionalAPI(
					os.Stdout,
					"amdsmi.GetGpuMetrics",
					func() ([]amdsmi.Metric, error) {
						return amdsmi.GetGpuMetrics(path.processor)
					},
				)
				if err != nil {
					metricsValid = false
				} else {
					metrics = refreshed
					metricsValid = true
				}
			}

			pollGpuMetrics(
				path.processorNumber,
				path.processor,
				pollIteration,
				metrics,
				metricsValid,
			)
		}

		// Do not sleep after the final sample so the example exits promptly.
		if pollIteration < pollIterations {
			select {
			case <-time.After(pollInterval):
			case <-ctx.Done():
				return ctx.Err()
			}
		}
	}
	fmt.Println()

	return nil
}

func getNicRdmaPorts(nic amdsmi.ProcessorHandle, nicNumber int) []nicRdmaPortRef {
	// RDMA statistics take the zero-based position of the RDMA port in the
	// order GetNicRdmaDevInfo lists them: all ports of the first RDMA
	// device, then all ports of the next, and so on. That index is not the
	// RdmaPort field from device info.
	rdmaDevInfo, err := example_utils.GetOptionalAPI(
		os.Stdout,
		fmt.Sprintf("amdsmi.GetNicRdmaDevInfo for NIC %d", nicNumber),
		func() (amdsmi.NicRdmaDevicesInfo, error) {
			return amdsmi.GetNicRdmaDevInfo(nic)
		},
	)
	if err != nil {
		return nil
	}

	// Flatten devices and ports into one list while preserving both the
	// human-readable port number and the API's positional statistics index.
	rdmaPorts := make([]nicRdmaPortRef, 0)
	var rdmaPortIndex uint32
	for _, rdmaDevice := range rdmaDevInfo.RdmaDevInfo {
		for _, rdmaPort := range rdmaDevice.RdmaPortInfo {
			rdmaPorts = append(rdmaPorts, nicRdmaPortRef{
				Device:        rdmaDevice.RdmaDev,
				Netdev:        rdmaPort.Netdev,
				RdmaPort:      rdmaPort.RdmaPort,
				RdmaPortIndex: rdmaPortIndex,
			})
			rdmaPortIndex++
		}
	}
	return rdmaPorts
}

func printNicTelemetry(ctx context.Context) error {
	// NIC monitoring is optional. INIT_ALL_PROCESSORS enables discovery of
	// both GPU and NIC processors without changing either device's state.
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
	if len(nicProcessors) == 0 {
		fmt.Println("  No NICs found\n")
		return nil
	}

	for nicNumber, nic := range nicProcessors {
		// The NIC BDF identifies which physical adapter owns the ports below.
		fmt.Println("###Test amdsmi.GetNicDeviceBdf")
		nicBdf, bdfErr := example_utils.GetOptionalAPI(
			os.Stdout,
			"amdsmi.GetNicDeviceBdf",
			func() (amdsmi.Bdf, error) {
				return amdsmi.GetNicDeviceBdf(nic)
			},
		)
		fmt.Printf(
			"  NIC number: %d bdf: %s\n",
			nicNumber,
			valueOrNotAvailable(bdfErr != nil, nicBdf.String(), "%s"),
		)

		fmt.Println("###Test amdsmi.GetNicPortInfo")
		portInfo, portInfoErr := example_utils.GetOptionalAPI(
			os.Stdout,
			fmt.Sprintf("amdsmi.GetNicPortInfo for NIC %d", nicNumber),
			func() (amdsmi.NicPortInfo, error) {
				return amdsmi.GetNicPortInfo(nic)
			},
		)
		if portInfoErr != nil {
			fmt.Println()
			continue
		}

		// Port information describes current link configuration and state.
		// portIndex is the zero-based index expected by NIC statistics APIs.
		ports := portInfo.Ports
		fmt.Printf("  number of NIC ports is: %d\n", len(ports))
		for portIndex, port := range ports {
			fmt.Printf(
				"  port %d: bdf=%s port_num=%d type=%s flavour=%s "+
					"netdev=%s ifindex=%d mac_address=%s carrier=%d mtu=%d "+
					"link_state=%s link_speed=%d active_fec=%d autoneg=%s "+
					"pause_autoneg=%s pause_rx=%s pause_tx=%s\n",
				portIndex,
				port.Bdf,
				port.PortNum,
				port.Type,
				port.Flavour,
				port.Netdev,
				port.Ifindex,
				port.MacAddress,
				port.Carrier,
				port.Mtu,
				port.LinkState,
				port.LinkSpeed,
				port.ActiveFec,
				port.Autoneg,
				port.PauseAutoneg,
				port.PauseRx,
				port.PauseTx,
			)
		}
		fmt.Println()

		// Discover RDMA ports separately because their statistics API uses a
		// flattened RDMA port index rather than the regular NIC port index.
		rdmaPorts := getNicRdmaPorts(nic, nicNumber)
		if len(rdmaPorts) == 0 {
			fmt.Printf("  No RDMA ports found for NIC %d\n", nicNumber)
		} else {
			for _, rdmaPort := range rdmaPorts {
				fmt.Printf(
					"  RDMA device %s port %d (index %d) netdev=%s\n",
					rdmaPort.Device,
					rdmaPort.RdmaPort,
					rdmaPort.RdmaPortIndex,
					rdmaPort.Netdev,
				)
			}
		}
		fmt.Println()

		fmt.Println("###Polling NIC counters")
		for pollIteration := 1; pollIteration <= pollIterations; pollIteration++ {
			if err := ctx.Err(); err != nil {
				return err
			}

			// Port state can change while counters are sampled, so retrieve a
			// fresh port list for each poll iteration.
			portInfo, portInfoErr := example_utils.GetOptionalAPI(
				os.Stdout,
				fmt.Sprintf("amdsmi.GetNicPortInfo for NIC %d", nicNumber),
				func() (amdsmi.NicPortInfo, error) {
					return amdsmi.GetNicPortInfo(nic)
				},
			)
			if portInfoErr != nil {
				break
			}
			ports = portInfo.Ports

			for portIndex, port := range ports {
				// Standard and vendor counters are queried independently;
				// either set may be unsupported on a particular adapter.
				portStatistics, portStatsErr := example_utils.GetOptionalAPI(
					os.Stdout,
					fmt.Sprintf(
						"amdsmi.GetNicPortStatistics for NIC %d port %d",
						nicNumber,
						portIndex,
					),
					func() ([]amdsmi.NicStat, error) {
						return amdsmi.GetNicPortStatistics(nic, uint32(portIndex))
					},
				)
				vendorStatistics, vendorStatsErr := example_utils.GetOptionalAPI(
					os.Stdout,
					fmt.Sprintf(
						"amdsmi.GetNicVendorStatistics for NIC %d port %d",
						nicNumber,
						portIndex,
					),
					func() ([]amdsmi.NicStat, error) {
						return amdsmi.GetNicVendorStatistics(nic, uint32(portIndex))
					},
				)
				fmt.Printf(
					"  Poll %d NIC %d port %d netdev=%s link_state=%s "+
						"link_speed=%d port_stats=[%s] vendor_stats=[%s]\n",
					pollIteration,
					nicNumber,
					portIndex,
					port.Netdev,
					port.LinkState,
					port.LinkSpeed,
					formatStatistics(portStatistics, portStatsErr != nil),
					formatStatistics(vendorStatistics, vendorStatsErr != nil),
				)
			}

			for _, rdmaPort := range rdmaPorts {
				// Pass the flattened positional index calculated during RDMA
				// discovery, not the device-reported RdmaPort value.
				rdmaStatistics, rdmaStatsErr := example_utils.GetOptionalAPI(
					os.Stdout,
					fmt.Sprintf(
						"amdsmi.GetNicRdmaPortStatistics for NIC %d RDMA port index %d",
						nicNumber,
						rdmaPort.RdmaPortIndex,
					),
					func() ([]amdsmi.NicStat, error) {
						return amdsmi.GetNicRdmaPortStatistics(
							nic,
							rdmaPort.RdmaPortIndex,
						)
					},
				)
				fmt.Printf(
					"  Poll %d NIC %d RDMA device %s port %d (index %d) netdev=%s stats=[%s]\n",
					pollIteration,
					nicNumber,
					rdmaPort.Device,
					rdmaPort.RdmaPort,
					rdmaPort.RdmaPortIndex,
					rdmaPort.Netdev,
					formatStatistics(rdmaStatistics, rdmaStatsErr != nil),
				)
			}

			if pollIteration < pollIterations {
				select {
				case <-time.After(pollInterval):
				case <-ctx.Done():
					return ctx.Err()
				}
			}
		}
		fmt.Println()
	}

	return nil
}
