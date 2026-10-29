# Run the AMD SMI RAS health example

The RAS health example inspects the Reliability, Availability, and
Serviceability (RAS) health of every discovered AMD GPU. It reports ECC
configuration and error counts, RAS features and policy information, and
retired memory pages.

`go/examples/ras_health/amdsmi_ras_health.go` is the Go implementation.

The example is read-only: it does not reset a GPU, clear error data, or change
any device configuration.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. An AMD GPU must be available.

Most of the example requires a GPU and host driver. RAS policy information
depends on device and platform capabilities. Unsupported optional APIs are
reported and do not prevent the remaining health checks from running.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/ras_health/amdsmi_ras_health.go
```

The reported values depend on the GPU, driver, firmware, and the current error
history. Finding no bad pages is a normal result.

## Example flow

The example initializes AMD SMI, discovers GPU processor handles, runs the RAS
queries for each GPU, and shuts down AMD SMI:

```go
if err := amdsmi.Init(amdsmi.AMDSMI_INIT_ALL_PROCESSORS); err != nil {
	return err
}
defer func() {
	if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
		fmt.Println(shutdownErr)
	}
}()

processors, err := amdsmi.GetProcessorHandles()
if err != nil {
	return err
}

for processorNumber, processor := range processors {
	if err := printGpuRasHealth(processorNumber, processor); err != nil {
		return err
	}
}
```

`amdsmi.GetGpuDeviceBdf()` identifies the GPU in the output with its PCI
bus:device.function (BDF) address. Keep the processor handle returned by
`amdsmi.GetProcessorHandles()` and pass that handle to the remaining GPU
queries.

### Inspect ECC health

ECC error counts are the primary RAS health indicators. The example first
calls `amdsmi.GetGpuTotalEccCount()` to retrieve totals across the GPU:

```go
totalEccCount, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetGpuTotalEccCount",
	func() (amdsmi.ErrorCount, error) {
		return amdsmi.GetGpuTotalEccCount(processor)
	},
)
if err == nil {
	fmt.Printf("  correctable_count is: %d\n", totalEccCount.CorrectableCount)
	fmt.Printf("  uncorrectable_count is: %d\n", totalEccCount.UncorrectableCount)
	fmt.Printf("  deferred_count is: %d\n", totalEccCount.DeferredCount)
}
```

The returned values contain:


| Field                 | Meaning                                |
| --------------------- | -------------------------------------- |
| `correctable_count`   | ECC errors corrected by the GPU        |
| `uncorrectable_count` | ECC errors that could not be corrected |
| `deferred_count`      | Deferred ECC errors                    |


Next, `amdsmi.GetGpuEccEnabled()` reports whether ECC is enabled for each GPU
block. The example prints the ECC state for every GPU block and requests
per-block counters only for blocks with ECC enabled.

The Go call returns a `map[amdsmi.GpuBlock]bool`. The example walks every
block from `AMDSMI_GPU_BLOCK_FIRST` through `AMDSMI_GPU_BLOCK_LAST`:

```go
eccEnabled, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetGpuEccEnabled",
	func() (map[amdsmi.GpuBlock]bool, error) {
		return amdsmi.GetGpuEccEnabled(processor)
	},
)
if err == nil {
	for block := amdsmi.AMDSMI_GPU_BLOCK_FIRST; block <= amdsmi.AMDSMI_GPU_BLOCK_LAST; block <<= 1 {
		if eccEnabled[block] {
			blockEccCount, blockErr := amdsmi.GetGpuEccCount(processor, block)
			if blockErr != nil {
				continue
			}
			fmt.Printf("  %s ECC errors:\n", gpuBlockName(block))
			printEccCount(blockEccCount)
		}
	}
}
```

This avoids querying an ECC counter for a block where ECC is disabled. The
per-block result has the same three count fields as the total ECC result. A
block can have ECC enabled without supporting a counter read; the example
groups those failures and reports them after the loop instead of once per
block.

### Read RAS features and policy

`amdsmi.GetGpuRasFeatureInfo()` reports the RAS EEPROM version and the ECC
correction-schema bitmask:

```go
rasFeature, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetGpuRasFeatureInfo",
	func() (amdsmi.RasFeatureInfo, error) {
		return amdsmi.GetGpuRasFeatureInfo(processor)
	},
)
if err == nil {
	fmt.Printf("  ras_eeprom_version is: %d\n", rasFeature.RasEepromVersion)
	fmt.Printf("  ecc_correction_schema is: %d\n", rasFeature.EccCorrectionSchemaFlag)
}
```

The program decodes set bits in `EccCorrectionSchemaFlag` against
`PARITY`, `CORRECTABLE`, `UNCORRECTABLE`, and `POISON` and prints the
supported schema names.

On supported PMFW-managed RAS EEPROM implementations, the example also calls
`amdsmi.GetGpuRasPolicyInfo()`.

The result reports `major_version` and `minor_version`. For policy version 4.0,
it also includes:


| Field                                | Meaning                                      |
| ------------------------------------ | -------------------------------------------- |
| `dram_non_critical_region_threshold` | UCE threshold for a non-critical DRAM region |
| `dram_critical_region_threshold`     | UCE threshold for a critical DRAM region     |


Go returns an `amdsmi.RasPolicyInfo` struct. Version 4.0 thresholds are
present on the `V4_0` field when that policy version is reported:

```go
rasPolicy, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetGpuRasPolicyInfo",
	func() (amdsmi.RasPolicyInfo, error) {
		return amdsmi.GetGpuRasPolicyInfo(processor)
	},
)
if err == nil {
	fmt.Printf("  major_version is: %d\n", rasPolicy.MajorVersion)
	fmt.Printf("  minor_version is: %d\n", rasPolicy.MinorVersion)
	if rasPolicy.V4_0 != nil {
		fmt.Printf(
			"  dram_non_critical_region_threshold is: %d\n",
			rasPolicy.V4_0.DramNonCriticalRegionThreshold,
		)
		fmt.Printf(
			"  dram_critical_region_threshold is: %d\n",
			rasPolicy.V4_0.DramCriticalRegionThreshold,
		)
	}
}
```

RAS policy support is platform-dependent. For example, it is available on
GPUs that use PMFW-managed RAS EEPROM. The example reports
when the API is not supported and continues.

### Check retired memory pages

A bad page is a GPU memory page retired from use after a RAS event. The example
uses two APIs:

```go
badPageThreshold, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetBadPageThreshold",
	func() (uint32, error) {
		return amdsmi.GetBadPageThreshold(processor)
	},
)
badPages, err := example_utils.GetOptionalAPI(
	os.Stdout,
	"amdsmi.GetGpuBadPageInfo",
	func() ([]amdsmi.EepromTableRecord, error) {
		return amdsmi.GetGpuBadPageInfo(processor)
	},
)
```

`bad_page_threshold` is the threshold configured for the GPU. Each retired-page
record contains:


| Field          | Meaning                                              |
| -------------- | ---------------------------------------------------- |
| `retired_page` | Retired 64K or 4K driver-managed memory location     |
| `ts`           | Time when the RAS event was observed                 |
| `mem_channel`  | Memory channel that reported the event               |
| `mcumc_id`     | Memory-controller identifier that reported the event |

The program formats `ts` with `time.Unix()` before printing it. An empty
`badPages` slice means the API found no retired pages. A timestamp that the
local platform cannot represent is printed as `unavailable (raw value ...)`.

Systems that do not support bad-page reporting are reported without stopping
the rest of the example.

### Handle errors

AMD SMI Go APIs return an error when a call fails. The example uses
`example_utils.GetOptionalAPI()` for optional RAS features so
`AMDSMI_STATUS_NOT_SUPPORTED` and similar conditions are printed without
stopping the full health report. Other failures are returned from `run()`.

Shutdown is placed in a `defer` so `amdsmi.ShutDown()` runs after a successful
`amdsmi.Init()`.
