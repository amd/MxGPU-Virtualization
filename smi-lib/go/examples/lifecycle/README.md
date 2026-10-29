# Run the AMD SMI lifecycle example

The lifecycle example is a small starting point for an AMD SMI application. It
initializes AMD SMI, displays the library version, converts status codes to
readable text, and closes the library before exiting. It does not modify device
state or require a GPU processor handle.

`go/examples/lifecycle/amdsmi_lifecycle.go` is the Go implementation.

## Prerequisites

Review the [shared prerequisites for AMD SMI Go examples](../../../docs/how_to/amdsmi_go_lib.md#prerequisites)
before running this program. This example has no GPU-specific requirements.

## Run the example

From `smi-lib/go/`, run:

```shell
sudo go run examples/lifecycle/amdsmi_lifecycle.go
```

The library version and status descriptions depend on the installed AMD SMI
release. The example always attempts to shut down the library before exiting.

## Example flow

The program follows this lifecycle:

```go
initFlags := amdsmi.AMDSMI_INIT_ALL_PROCESSORS
if err := amdsmi.Init(initFlags); err != nil {
	return err
}
defer func() {
	if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
		fmt.Println(shutdownErr)
	}
}()

libData, err := amdsmi.GetLibVersion()
if err != nil {
	return err
}

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
```

### Initialize AMD SMI

`amdsmi.Init()` must be called before APIs that access the AMD SMI library.
The example explicitly passes the all-processors init flag, which is also the
default when no argument is supplied.

The example uses `amdsmi.AMDSMI_INIT_ALL_PROCESSORS`. `amdsmi.InitFlags`
exposes the following values:

| Flag                         | Intended processor discovery scope        |
| ---------------------------- | ----------------------------------------- |
| `AMDSMI_INIT_ALL_PROCESSORS` | All processors                            |
| `AMDSMI_INIT_AMD_CPUS`       | AMD CPUs                                  |
| `AMDSMI_INIT_AMD_GPUS`       | AMD GPUs                                  |
| `AMDSMI_INIT_NON_AMD_CPUS`   | Non-AMD CPUs                              |
| `AMDSMI_INIT_NON_AMD_GPUS`   | Non-AMD GPUs                              |
| `AMDSMI_INIT_AMD_APUS`       | AMD APUs, combining AMD CPU and GPU flags |
| `AMDSMI_INIT_AMD_NICS`       | AMD NICs                                  |

Keep `AMDSMI_INIT_ALL_PROCESSORS` in this example unless the implementation
gains support for selecting a discovery scope.

The current native implementation does not use the supplied initialization
flags, and GPU discovery is the currently supported processor discovery path.

### Read the library version

`amdsmi.GetLibVersion()` returns the version of the AMD SMI library:

```go
libData, err := amdsmi.GetLibVersion()
if err != nil {
	return err
}
fmt.Printf("  lib_data['major'] is: %d\n", libData.Major)
fmt.Printf("  lib_data['minor'] is: %d\n", libData.Minor)
fmt.Printf("  lib_data['release'] is: %d\n", libData.Release)
```

This is the AMD SMI library version, not the GPU driver or firmware version.

### Decode a status code

The Go interface returns an error when an API call fails.
`amdsmi.StatusCodeToString()` converts a known `amdsmi.Status` value into the
corresponding human-readable description.

The example displays descriptions for `AMDSMI_STATUS_SUCCESS` and
`AMDSMI_STATUS_NOT_INIT` without causing an error.

```go
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
```

`printStatusDescription()` calls `amdsmi.StatusCodeToString()` and prints the
result:

```go
func printStatusDescription(status amdsmi.Status, name string) error {
	description, err := amdsmi.StatusCodeToString(status)
	if err != nil {
		return err
	}
	fmt.Printf("  %s is: %s\n", name, description)
	return nil
}
```

To decode a status from an `amdsmi.StatusError`, use its `Code` field:

```go
var statusErr *amdsmi.StatusError
if errors.As(err, &statusErr) {
	description, decodeErr := amdsmi.StatusCodeToString(statusErr.Code)
	if decodeErr != nil {
		fmt.Fprintln(os.Stderr, err)
	} else {
		fmt.Fprintln(os.Stderr, description)
	}
}
```

### Shut down AMD SMI

`amdsmi.ShutDown()` releases AMD SMI resources and closes its driver
connection. For each successful `amdsmi.Init()` call, make a matching shutdown
call.

The program places shutdown in a `defer` so that cleanup runs when `run()`
returns, including on error.

```go
defer func() {
	if shutdownErr := amdsmi.ShutDown(); shutdownErr != nil {
		fmt.Println(shutdownErr)
		return
	}
	fmt.Println("Shutdown complete")
}()
```
