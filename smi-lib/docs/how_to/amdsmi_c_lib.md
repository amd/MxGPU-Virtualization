---
myst:
  html_meta:
    "description lang=en": "Get started with the AMD SMI C library. Basic usage and examples."
    "keywords": "api, smi, lib, c, system, management, interface"
---

# AMD SMI C library usage and examples

This section is dedicated to explaining how to use the AMD System Management Interface (SMI) C library. It provides guidance on the basic setup and teardown of the library, which is essential for interacting with AMD hardware through the SMI API.

The AMD SMI C library allows developers to query and control various aspects of AMD hardware, such as monitoring power usage, temperature, and performance metrics. To effectively use the library, it is important to follow the correct initialization and cleanup procedures.

## Key Steps for Using the AMD SMI C Library

1. **Initialization**:
  Before making any calls to the AMD SMI API, the library must be initialized using the `amdsmi_init()` function. This function sets up the necessary internal data structures and prepares the library for use.

2. **Performing Operations**:
  Once the library is initialized, you can use the various API functions provided by AMD SMI to interact with the hardware. These functions allow you to retrieve system information, monitor hardware metrics, and perform other management tasks.

3. **Cleanup**:
  After completing all operations, it is crucial to call `amdsmi_shut_down()` to properly release resources and close the connection to the driver.

By following these steps, you can ensure that your application interacts with AMD hardware efficiently and safely.

For a detailed example, refer to the code snippet in the next section, which demonstrates the basic structure of an application using the AMD SMI C library.

## AMD SMI C example

An application using AMD SMI must call `amdsmi_init()` to initialize the AMD SMI
library before all other calls. This call initializes the internal data
structures required for subsequent AMD SMI operations. In the call, a flag can
be passed to indicate if the application is interested in a specific device
type.

`amdsmi_shut_down()` must be the last call to properly close connection to
driver and make sure that any resources held by AMD SMI are released.

Example:

```c
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "amdsmi.h"

int main(void)
{
    amdsmi_processor_handle *processors = NULL;
    uint32_t gpu_count = 0;
    amdsmi_bdf_t bdf;
    amdsmi_asic_info_t asic;
    int ret = 0;

    // Initialize the AMD SMI library
    ret = amdsmi_init(AMDSMI_INIT_ALL_PROCESSORS);
    if (ret != AMDSMI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to initialize AMD SMI library: %d\n", ret);
        return ret;
    }

    // Get the number of GPU processors
    ret = amdsmi_get_processor_handles(NULL, &gpu_count, NULL);
    if (ret != AMDSMI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to get GPU count: %d\n", ret);
        goto fini;
    }

    // Allocate memory for processor handles
    processors = (amdsmi_processor_handle *)malloc(sizeof(amdsmi_processor_handle) * gpu_count);
    if (!processors) {
        fprintf(stderr, "Memory allocation failed\n");
        ret = AMDSMI_STATUS_OUT_OF_RESOURCES;
        goto fini;
    }

    // Retrieve processor handles
    ret = amdsmi_get_processor_handles(NULL, &gpu_count, processors);
    if (ret != AMDSMI_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to get processor handles: %d\n", ret);
        goto fini;
    }

    // Iterate through GPUs and retrieve ASIC information
    for (uint32_t i = 0; i < gpu_count; i++) {
      ret = amdsmi_get_gpu_device_bdf(processors[i], &bdf);
      if (ret != AMDSMI_STATUS_SUCCESS) {
          fprintf(stderr, "Failed to get GPU[%u] BDF: %d\n", i, ret);
          goto fini;
      }
      ret = amdsmi_get_gpu_asic_info(processors[i], &asic);
      if (ret != AMDSMI_STATUS_SUCCESS) {
          fprintf(stderr, "Failed to get ASIC info for GPU[%u]: %d\n", ret, i);
          goto fini;
      }
      printf("GPU[%u] BDF:[%02" PRIx64 ":%02" PRIx64 ".%" PRIu64 "] Vendor ID: 0x%04x Device ID: 0x%04" PRIx64 " Market name: %s\n",
            i,
            (uint64_t)bdf.bdf.bus_number, (uint64_t)bdf.bdf.device_number, (uint64_t)bdf.bdf.function_number,
            asic.vendor_id,
            asic.device_id,
            asic.market_name);
    }

  fini:
    // Free allocated resources and clean up internal library resources
    free(processors);
    amdsmi_shut_down();
    return ret;
}
```
